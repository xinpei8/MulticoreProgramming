#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


#include <string.h>
#include <arpa/inet.h>


#include <iostream>
#include <sstream> 
#include <list>
#include <mutex>
#include <condition_variable>
#include "ThreadSafeCout.h"
#include "ThreadSafeKVStore.h"
#include "md5.h"
// using namespace std;

class ThreadArgs {
public:
    ThreadArgs(){;}
    ThreadArgs(ThreadSafeKVStore<string, pair<string, string>>& mymap, 
        int& i, string& request){

        // threadPoolPtr = &threadPool;
        mapPtr = &mymap;
        idPtr = &i;
        requestPtr = &request;
    }

    // ThreadPoolServer *threadPoolPtr;
    ThreadSafeKVStore<string,  pair<string, string>> *mapPtr;
    int *idPtr;
    string *requestPtr;
};

class ThreadPoolServer{
public:
    ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore);

    /* Start running the server for getting request from clients. */
    void run();

    /* Push the id of available thread into queue. */
	int  push(const int threadId);

    /* Keep listening to the queue until there is at least one available thread. 
       Pull one out of the queue and assign jobs to the thread with given thread 
       id accordingly. */
	int listen(int& idleThreadId);

private:
    void *thread_fnc(void *arguments);
    void printQ();//for testing
    void error(const char *msg){
        perror(msg);
        exit(1);
    }


    // Members for sockets 
    int server_socket;
    struct sockaddr_in server_addr;


    // Members for threads and key/value store
    ThreadSafeKVStore<string, pair<string, string>> *myMap;
    pthread_t *threads;
    ThreadArgs *args;
	list<int> threadPool;
    mutable std::mutex mu;
    std::condition_variable c;

};
ThreadPoolServer::ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore){
    cout << "Start constructing ThreadPoolServer...\n";
    myMap = &KVStore;

    /* Create thread pool by given thread number */
    threads = new pthread_t[threadNumber];
    args = new ThreadArgs[threadNumber];

    for(int i = 0; i < threadNumber; ++i){
        this->push(i);
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
    server_addr.sin_port = htons(2100);  //port
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
        // listen(server_socket, 5);

        //socket receive one request
        struct sockaddr_in client_addr;
        char buffer[256];

        // string request = "";

        socklen_t client_len = sizeof(client_addr);
        int newsockfd = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
        if (newsockfd < 0) 
          error("ERROR on accept");
        
        bzero(buffer,256);
        //memset(&buffer, 0, sizeof(buffer));
        int n = read(newsockfd,buffer,255);
        if (n < 0) 
            error("ERROR reading from socket");

        printf("[Server]Here is the message: %s\n", buffer);

        n = write(newsockfd, buffer ,18);
        if (n < 0) 
            error("ERROR writing to socket");
        close(newsockfd);
        // close(sockfd);



        // //get an idle thread id
        // this->listen(id);

        // //parse the request
        // ThreadArgs newArg(myMap, id, request);
        // args[id] = newArg;
        // rc = pthread_create(&threads[id], NULL, &thread_fnc, (void *)&args[id]);
        
        // if (rc) {
        //     ThreadSafeCout{} << "Error:unable to create thread," << rc << endl;
        //     exit(EXIT_FAILURE);
        // }


    }
}

void *ThreadPoolServer::thread_fnc(void *arguments){
    ThreadArgs *args = (ThreadArgs *)arguments;
    stringstream ss(*args->requestPtr);
    string job, key;
    pair<string, string> value("", "");
    // string key = "";
    // string value = "";
    ss >> job;
    ss >> key;
    if (job == "GET"){//int lookup(const K key, V& value);
        args->mapPtr->lookup(key, value);
        //ADD: reply value to socket

    }
    else if (job == "POST"){//int insert(const K key, const V value);
        ss >> value.first;
        value.second = md5(value.first);
        args->mapPtr->insert(key, value);
        //ADD: reply sucess to socket


    }
    else if (job == "DELETE"){//int remove(const K key);
        args->mapPtr->remove(key);
        //ADD: reply sucess to socket

    }
    else {
        ThreadSafeCout{} << "Thread: " + to_string(*args->idPtr) + "Error: wrong type of client request.\n";
    }

    
    //Job completed, push thread id to thread pool.
    this->push(*args->idPtr);
    // args->queuePtr->push(valueSum);
    ThreadSafeCout{} << "Thread: " + to_string(*args->idPtr) + " completed\n";
    return NULL;
}


int ThreadPoolServer::push(const int threadId){
    std::lock_guard<std::mutex> lock(mu);
    threadPool.push_front(threadId);
    c.notify_one();
    return 0;
}

int ThreadPoolServer::listen(int& idleThreadId){
    std::unique_lock<std::mutex> lock(mu);
    while (threadPool.empty()) {
        c.wait(lock);//wait until there is an element in queue.
    }
    idleThreadId = threadPool.back();
    threadPool.pop_back();
    return 0;
}

void ThreadPoolServer::printQ(){
    for (auto e : threadPool) {
        cout << e << " ";
    }
    cout << endl;
}
