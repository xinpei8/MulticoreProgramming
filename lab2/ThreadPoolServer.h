#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "httpreq.hpp"
#include "httpresp.hpp"
#include "ThreadSafeCout.h"
#include "ThreadSafeKVStore.h"
#include "ThreadSafeListenerQueue.h"
#include "md5.h"

class ThreadArgs {
public:
    ThreadArgs(){
        id = 0;
        newsockfd = 0;
    }
    ThreadArgs(ThreadSafeListenerQueue<int> &threadPool, ThreadSafeKVStore<string, 
        pair<string, string>> *mymap, int &i, int &fd){
        threadPoolPtr = &threadPool;
        mapPtr = mymap;
        id = i;
        newsockfd = fd;
    }

    ThreadSafeListenerQueue<int> *threadPoolPtr;
    ThreadSafeKVStore<string,  pair<string, string>> *mapPtr;
    int id;
    int newsockfd;
};

class ThreadPoolServer{
public:
    ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore, int portNumber);

    /* Start running the server for getting requests from client. */
    void run();


private:
    static void *thread_fnc(void *arguments);
    void error(const char *msg){
        perror(msg);
        exit(1);
    }

    // Members for sockets 
    int server_socket;
    struct sockaddr_in server_addr;

    // Members for threads and key/value store
    ThreadSafeKVStore<string, pair<string, string>> *myMap;
    ThreadSafeListenerQueue<int> threadPool;// stores the available thread id
    pthread_t *threads;

};


ThreadPoolServer::ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore, int portNumber){
    cout << "Start constructing ThreadPoolServer...\n";
    myMap = &KVStore;

    /* Create thread pool by given thread number */
    threads = new pthread_t[threadNumber];

    for(int i = 0; i < threadNumber; ++i){
        threadPool.push(i);
    }

    cout << "Finish creating threads.\n";

    /* Create a listening socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        error("ERROR opening socket");
    }

    // Binding server socket, IP and host
    memset(&server_addr, 0, sizeof(server_addr));//set all to zero
    server_addr.sin_family = AF_INET;  //using IPv4 address
    server_addr.sin_addr.s_addr = INADDR_ANY; //binds the socket to all available interfaces
    server_addr.sin_port = htons(portNumber);  //port

    if (::bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
              error("ERROR on binding");

    cout << "Finish creating server socket and binding." << endl;


}
void ThreadPoolServer::run(){

    //socket keep listening to client requests
    int id;
    ::listen(server_socket, 5);
    cout << "Server start listening to client request" << endl;
    while(1){

        // Accept HTTP request from client
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int newsockfd = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
        if (newsockfd < 0) {
          error("ERROR on accept");
        }

        // Get an idle thread id
        threadPool.listen(id);

        // Create argument for pthread including the client request.
        ThreadArgs *newArg = new ThreadArgs(this->threadPool, this->myMap, id, newsockfd);

        int rc = pthread_create(&threads[id], NULL, thread_fnc, (void *)newArg);  
        if (rc) {
            ThreadSafeCout{} << "Error:unable to create thread," << rc << endl;
            exit(EXIT_FAILURE);
        }

    }
}

void *ThreadPoolServer::thread_fnc(void *arguments){
    ThreadArgs *args = (ThreadArgs *)arguments;

    /* Read client request */
    HTTPReq request(args->newsockfd);

    if (0 != request.parse() || request.isMalformed()) {
        std::cerr << "Failed to parse sample request. Aborting test" << std::endl;
        exit(-1);
    }

    string method = request.getMethod();
    string key = request.getURI();
    pair<string, string> value("", "");
    int check = 0;

    if (method == "GET"){
        check = args->mapPtr->lookup(key, value);
        ThreadSafeCout{} << "Thread: " + to_string(args->id) + " finish GET " + key + 
        " value = " + value.first << ", " + value.second + "\n";
    }
    else if (method == "POST"){
        value.first = request.getBody();
        value.second = md5(value.first);
        check = args->mapPtr->insert(key, value);
        ThreadSafeCout{} << "Thread: " + to_string(args->id) + " finish POST " + key + 
        " value = " + value.first << ", " + value.second + "\n";
    }
    else if (method == "DELETE"){
        check = args->mapPtr->remove(key);
        ThreadSafeCout{} << "Thread: " + to_string(args->id) + " finish REMOVE " + key + "\n";
    }
    else {
        ThreadSafeCout{} << "Thread: " + to_string(args->id) + " Method = " + method +
        " Ignore: method is not GET/POST/DELETE.\n";
    }

    HTTPResp ok(200, request.getBody());
    HTTPResp fail(404, request.getBody());

    /* Write back Http Response */
    if(check == 0){
        send(args->newsockfd, ok.getResponse().c_str(), (int) strlen(ok.getResponse().c_str()), 0);
    }else{
        send(args->newsockfd, fail.getResponse().c_str(), (int) strlen(fail.getResponse().c_str()), 0);
    }   

    /* Job completed, push thread id to thread pool. */
    args->threadPoolPtr->push(args->id);
    close(args->newsockfd);
    delete args;
    // ThreadSafeCout{} << "Thread: " + to_string(*args->idPtr) + " completed\n";

    return NULL;
}

