#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


// #include <string.h>
#include <arpa/inet.h>


#include <iostream>
#include <sstream> 
// #include <list>
// #include <mutex>
// #include <condition_variable>

#include "httpreq.hpp"
#include "httpresp.hpp"
#include "ThreadSafeCout.h"
#include "ThreadSafeKVStore.h"
#include "ThreadSafeListenerQueue.h"
#include "md5.h"
// using namespace std;

class ThreadArgs {
public:
    ThreadArgs(){
        idPtr = 0;
        newsockfdPtr = 0;
        requestPtr = NULL;
    }
    ThreadArgs(ThreadSafeListenerQueue<int> &threadPool, ThreadSafeKVStore<string,  pair<string, string>> *mymap, int& i, HTTPReq& request, int &newsockfd){

        threadPoolPtr = &threadPool;
        // threadPoolPtr
        mapPtr = mymap;
        idPtr = i;
        newsockfdPtr = newsockfd;
        requestPtr = new HTTPReq(newsockfd);
    }

    // ThreadPoolServer *threadPoolPtr;
    ThreadSafeListenerQueue<int> *threadPoolPtr;
    ThreadSafeKVStore<string,  pair<string, string>> *mapPtr;
    int idPtr;
    int newsockfdPtr;
    HTTPReq *requestPtr;
};

class ThreadPoolServer{
public:
    ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore, int portNumber);

 //    /* Start running the server for getting request from clients. */
    void run();

 //    /* Push the id of available thread into queue. */
	// int  push(const int threadId);

    /* Keep listening to the queue until there is at least one available thread. 
       Pull one out of the queue and assign jobs to the thread with given thread 
       id accordingly. */
	int listen(int& idleThreadId);

private:
    static void *thread_fnc(void *arguments);
    // void printQ();//for testing
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
    ThreadArgs *args;
    // mutable std::mutex mu;
    // std::condition_variable c;

};


ThreadPoolServer::ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore, int portNumber){
    cout << "Start constructing ThreadPoolServer...\n";
    myMap = &KVStore;

    /* Create thread pool by given thread number */
    threads = new pthread_t[threadNumber];
    args = new ThreadArgs[threadNumber];

    for(int i = 0; i < threadNumber; ++i){
        threadPool.push(i);
    }

    cout << "Finish creating threads.\n";

    /* Create a listening socket */
    // server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        error("ERROR opening socket");
    }

    // Binding server socket, IP and host
    memset(&server_addr, 0, sizeof(server_addr));//set all to zero
    server_addr.sin_family = AF_INET;  //using IPv4 address
    server_addr.sin_addr.s_addr = INADDR_ANY; //binds the socket to all available interfaces
    server_addr.sin_port = htons(portNumber);  //port
    // bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

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

        // Accept and parse  HTTP request
        struct sockaddr_in client_addr;

        socklen_t client_len = sizeof(client_addr);
        int newsockfd = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
        if (newsockfd < 0) 
          error("ERROR on accept");

        HTTPReq request(newsockfd);
        if (0 != request.parse() || request.isMalformed()) {
            std::cerr << "Failed to parse sample request. Aborting test" << std::endl;
            exit(-1);
        }
        cout << "\n[Server] Here is the request:\n";
        cout << request << std::endl;


        // //get an idle thread id

        threadPool.listen(id);
        cout << "id = " << id << "\n";

        //parse the request
        ThreadArgs newArg(this->threadPool, this->myMap, id, request, newsockfd);

        args[id] = newArg;
        cout << "id = " << id << " newsockfd = " + to_string(newsockfd) + " \n";
        int rc = pthread_create(&threads[id], NULL, thread_fnc, (void *)&args[id]);
        
        if (rc) {
            ThreadSafeCout{} << "Error:unable to create thread," << rc << endl;
            exit(EXIT_FAILURE);
        }

        // close(newsockfd);


    }
}

void *ThreadPoolServer::thread_fnc(void *arguments){
    ThreadArgs *args = (ThreadArgs *)arguments;
    HTTPReq request = *args->requestPtr;
    // ThreadPoolServer *mapPtr = *args->threadPoolPtr;

    string method = request.getMethod();
    string key = request.getURI();
    pair<string, string> value("", "");

    // char ok[] = "200 OK";
    // char fail[] = "404 NOT Found";
    int check = 0;


    if (method == "GET"){//int lookup(const K key, V& value);
        check = args->mapPtr->lookup(key, value);
        // char ok[] = "200 OK";
        //ADD: reply value to socket
        ThreadSafeCout{} << "Thread: " + to_string(args->idPtr) + " finish GET " + key + " value = " + value.first << ", " + value.second + "\n";

    }
    else if (method == "POST"){//int insert(const K key, const V value);
        value.first = request.getBody();
        value.second = md5(value.first);
        check = args->mapPtr->insert(key, value);

        //ADD: reply sucess to socket
        ThreadSafeCout{} << "Thread: " + to_string(args->idPtr) + " finish POST " + key + " value = " + value.first << ", " + value.second + "\n";


    }
    else if (method == "DELETE"){//int remove(const K key);
        check = args->mapPtr->remove(key);
        //ADD: reply sucess to socket
        ThreadSafeCout{} << "Thread: " + to_string(args->idPtr) + " finish REMOVE " + key + "\n";

    }
    else {
        ThreadSafeCout{} << "Thread: " + to_string(args->idPtr) + " Method = " + method +
        " Ignore: method is not GET/POST/DELETE.\n";
    }

    HTTPResp ok(200, request.getBody());
    HTTPResp fail(404, request.getBody());
    ThreadSafeCout{} << "newSockFd = " + to_string(args->newsockfdPtr) + "\n";

    /* Write back Http Response */
    if(check == 0){
        // send(*args->newsockfdPtr, ok_ , sizeof(ok_), 0);
        send(args->newsockfdPtr, ok.getResponse().c_str(), (int) strlen(ok.getResponse().c_str()), 0);
    }else{
        // send(*args->newsockfdPtr, fail_ , sizeof(fail_), 0);
        send(args->newsockfdPtr, fail.getResponse().c_str(), (int) strlen(fail.getResponse().c_str()), 0);

    }   

    
    //Job completed, push thread id to thread pool.
    args->threadPoolPtr->push(args->idPtr);

    close(args->newsockfdPtr);

    // ThreadSafeCout{} << "Thread: " + to_string(*args->idPtr) + " completed\n";
    return NULL;
}



// void ThreadPoolServer::printQ(){
//     for (auto e : threadPool) {
//         cout << e << " ";
//     }
//     cout << endl;
// }
