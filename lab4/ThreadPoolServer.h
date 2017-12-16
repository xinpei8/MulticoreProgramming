#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <dirent.h>
#include <sys/time.h>
#include "httpreq.hpp"
#include "httpresp.hpp"
#include "ThreadSafeCout.h"
#include "ThreadSafeKVStore.h"
#include "ThreadSafeListenerQueue.h"
#include "md5.h"
#include "signal.h"
#define CAPACITY 128


static ThreadSafeKVStore<string, double> reportMap;
static ThreadSafeListenerQueue<double> requestTime;

int readKVfromFile(string folderPath, string key, pair<string, string> &value);
int writeKVtoFile(string folderPath, string key, string value, string hash);
int deleteFile(string folderPath, string key);

class ThreadArgs {
public:
    ThreadArgs(){
        id = 0;
        newsockfd = 0;
    }
    ThreadArgs(ThreadSafeListenerQueue<int> &threadPool,
               ThreadSafeKVStore<string, pair<string, string>> *mymap,
               ThreadSafeKVStore<string, double> *report, int &i, int &fd, clock_t &start,
               ThreadSafeKVStore<string, int> &keyToFrequencyMap,
               string folder){
        
        threadPoolPtr = &threadPool;
        mapPtr = mymap;
        reportMapPtr = report;
        id = i;
        newsockfd = fd;
        startTime = start;
        keyToFrequencyMapPtr = &keyToFrequencyMap;
        onDiskFolder = folder;
    }
    
    ThreadSafeListenerQueue<int> *threadPoolPtr;
    ThreadSafeKVStore<string,  pair<string, string>> *mapPtr;//key to value and hash
    ThreadSafeKVStore<string, int> *keyToFrequencyMapPtr; //key to freq
    ThreadSafeKVStore<string, double> *reportMapPtr;
    string onDiskFolder;
    int id;
    int newsockfd;
    clock_t startTime;//THREAD START TIME
};

class ThreadPoolServer{
public:
    ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore, int portNumber, string directory);
    
    /* Start running the server for getting requests from client. */
    void run();
    
    /* Catch server termination singal (^C) and generate report. */
    static void sigint(int a);
    
private:
    static void *thread_fnc(void *arguments);
    void error(const char *msg){
        perror(msg);
        exit(1);
    }
    
    // Members for sockets
    int server_socket;
    struct sockaddr_in server_addr;
    string fileDirectory;
    
    // Members for threads and in-memory key/value store
    ThreadSafeKVStore<string, pair<string, string>> *myMap;// in-memory store
    ThreadSafeListenerQueue<int> threadPool;// stores the available thread id
    pthread_t *threads;
    
    // Members for in-memory capacity control, other keys go into on-disk store
    ThreadSafeKVStore<string, int> keyToFrequencyMap; //key to freq, list iterator
    
};


ThreadPoolServer::ThreadPoolServer(int threadNumber, ThreadSafeKVStore<string, pair<string, string>> &KVStore, int portNumber, string directory){
    cout << "Start constructing ThreadPoolServer...\n";
    myMap = &KVStore;
    
    /* Wipe the directory clean for on-disk storage */
    fileDirectory = directory;
    const char *cstr = fileDirectory.c_str();
    
    DIR *thisFolder = opendir(cstr);
    struct dirent *next_file;
    char filepath[256];
    
    if (thisFolder) {
        while ((next_file = readdir(thisFolder)) != NULL) {
            sprintf(filepath, "%s/%s", cstr, next_file->d_name);
            remove(filepath);
        }
        closedir(thisFolder);
        cout << "Finish cleaning up files in the given directory.\n";
    }
    else {
        cout << "Fail to find the directory. Please check the directory path in arguments.\n";
        exit(1);
    }
    
    /* Create thread pool by given thread number */
    threads = new pthread_t[threadNumber];
    
    for (int i = 0; i < threadNumber; ++i) {
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
    
    // Set up signal receiver for detecting termination
    signal(SIGINT, ThreadPoolServer::sigint);
    
    // Initialize the numbers for report
    reportMap.insert("GET_count", 0);
    reportMap.insert("POST_count", 0);
    reportMap.insert("DELETE_count", 0);
    reportMap.insert("WorkTime", 0);
    reportMap.insert("RequestTime", 0);
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
        
        // Record Request Start Time
        clock_t start = clock();
        
        // Get an idle thread id
        threadPool.listen(id);
        
        // Create argument for pthread including the client request
        ThreadArgs *newArg = new ThreadArgs(this->threadPool, this->myMap, &reportMap ,id, newsockfd, start, keyToFrequencyMap, fileDirectory);
        
        int rc = pthread_create(&threads[id], NULL, thread_fnc, (void *)newArg);
        if (rc) {
            ThreadSafeCout{} << "Error:unable to create thread," << rc << endl;
            exit(EXIT_FAILURE);
        }
        
    }
}

void *ThreadPoolServer::thread_fnc(void *arguments){
    
    ThreadArgs *args = (ThreadArgs *)arguments;
    string folder = args->onDiskFolder;
    
    while (1) {
        
        /* Read client request */
        HTTPReq request(args->newsockfd);
        
        if (0 != request.parse() || request.isMalformed()) {
            std::cerr << "Failed to parse sample request. Aborting reading this connection" << std::endl;
            /* Job completed, push thread id to thread pool. */
            args->threadPoolPtr->push(args->id);
            close(args->newsockfd);
            delete args;
            cout << "CLOSE CONNECTION" << endl;
            return NULL;

        }
        
        string method = request.getMethod();
        string key = request.getURI();
        pair<string, string> value("", "");
        int check = 0;
        int freq = 0;
        
        if (method == "GET"){
            /*
             The server reads the relevant file each time a lookup occurs
             only if the in-memory cache does not currently hold the given key.
             */
            
            check = args->mapPtr->lookup(key, value);
            
            if (check != 0) { // not found in memory
                check = readKVfromFile(folder, key, value);
                
                if (check == 0) { //key exists in files, bring it to memory
                    args->keyToFrequencyMapPtr->insert(key, 1);
                    args->mapPtr->insert(key, value);
                }
                
            } else { // found in memory
                args->keyToFrequencyMapPtr->accumulate(key, 1);// update freqency
            }
            
            
            ThreadSafeCout{} << "Thread: " + to_string(args->id) + " finish GET " + key +
            " value = " + value.first << ", " + value.second + "\n";
            args->reportMapPtr->accumulate("GET_count", 1);
            
        } else if (method == "POST"){
            /* 
             The server opens, writes, and closes the relevant file
             each time an insert occurs.
             */
            value.first = request.getBody();
            value.second = md5(value.first);
            writeKVtoFile(folder, key, value.first, value.second);
            check = args->mapPtr->insert(key, value); //save in memory
            args->keyToFrequencyMapPtr->accumulate(key, 1);// update freqency
            
            
            
            ThreadSafeCout{} << "Thread: " + to_string(args->id) + " finish POST " + key +
            " value = " + value.first << ", " + value.second + "\n";
            args->reportMapPtr->accumulate("POST_count", 1);
        } else if (method == "DELETE"){
            /*
             Delete the relevant file, and if it exists, the relevant in-memory cache entry.
             */
            args->mapPtr->remove(key);
            args->keyToFrequencyMapPtr->remove(key);
            check = deleteFile(folder, key);//CHECK MUTEX
            
            ThreadSafeCout{} << "Thread: " + to_string(args->id) + " finish REMOVE " + key + "\n";
            args->reportMapPtr->accumulate("DELETE_count", 1);
            
        } else {
            ThreadSafeCout{} << "Thread: " + to_string(args->id) + " Method = " + method +
            " Ignore: method is not GET/POST/DELETE.\n";
        }
        
        HTTPResp ok(200, request.getBody(), false);
        HTTPResp fail(404, request.getBody(), false);
        
        /* Write back Http Response */
        if(check == 0){
            send(args->newsockfd, ok.getResponse().c_str(), (int) strlen(ok.getResponse().c_str()), 0);
        }else{
            send(args->newsockfd, fail.getResponse().c_str(), (int) strlen(fail.getResponse().c_str()), 0);
        }
        
        /* Record Request Time */
        clock_t end = clock();
        double elapsed_secs = double(end - args->startTime) / CLOCKS_PER_SEC * 1000;//(in milliseconds)
        requestTime.push(elapsed_secs);
        
        
        /* 
         Check data size in memory. If the size of kvStore exceeds
         the capacity, remove the least frequently used key.
         */
        string keyToBePopped = "";
        bool hasRemovedKey = args->keyToFrequencyMapPtr->maintainSize(CAPACITY, keyToBePopped);
        if (hasRemovedKey) {
            args->mapPtr->remove(keyToBePopped);
        }
        
        
    }//end while
}

void ThreadPoolServer::sigint(int a)
{
    double getCount;
    double postCount;
    double deleteCount;
    reportMap.lookup("GET_count", getCount);
    reportMap.lookup("POST_count", postCount);
    reportMap.lookup("DELETE_count", deleteCount);
    
    double requestTimeSum = 0;
    double oneRequest = 0;
    vector<double> requestTimeList;
    while (requestTime.pop(oneRequest) == 0) {
        requestTimeSum += oneRequest;
        requestTimeList.push_back(oneRequest);
    }
    sort(requestTimeList.begin(), requestTimeList.end());
    double medianTime, maxTime, minTime, meanTime;
    unsigned int size = requestTimeList.size();
    if (size != 0) {
        maxTime = requestTimeList[size - 1];
        minTime = requestTimeList[0];
        meanTime = requestTimeSum / size;
        if (size % 2 == 1) {
            medianTime = requestTimeList[size / 2 + 1];
        }
        else {
            medianTime = (requestTimeList[size / 2] + requestTimeList[size / 2 - 1]) / 2;
        }
        
    }
    cout << "\n-------- Server Statistic Report (in milliseconds)-------\n";
    cout << "Total number of GET : " << getCount << endl;
    cout << "Total number of POST : " << postCount << endl;
    cout << "Total number of DELETE : " << deleteCount << endl;
    cout << "Total request time: " << requestTimeSum << endl;
    cout << "Maximum request time: " << maxTime << endl;
    cout << "Minimum request time: " << minTime << endl;
    cout << "Median request time: " << medianTime << endl;
    cout << "Mean request time: " << meanTime << endl;
    exit(0);
}

int readKVfromFile(string folderPath, string key, pair<string, string> &value){
    string filename = folderPath + "/" + key + ".txt";
    ifstream fin;
    fin.open(filename);
    if (fin.fail()) {
        cout << "Failed to open file with key: " << key << endl;
        fin.close();
        return -1;
    }
    else {
        fin >> value.first >> value.second;//value, hash
        fin.close();
        return 0;
    }
    
}

int writeKVtoFile(string folderPath, string key, string value, string hash){
    string filename = folderPath + "/" + key + ".txt";
    ofstream fout;
    fout.open(filename);
    fout << value << "\n" << hash;//check if there is '\n' in value
    fout.close();
    return 0;
    
}

int deleteFile(string folderPath, string key){
    string filename = folderPath + "/" + key + ".txt";
    
    if( remove( filename.c_str() ) != 0 ){
        cout << "Error deleting file";
        return -1;
    }
    else{
        cout << "File successfully deleted";
        return 0;
    }
}



