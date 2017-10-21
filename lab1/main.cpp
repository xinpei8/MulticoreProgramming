#include <iostream>
#include <string>
#include <cstdint>
#include <random>
#include <ctime>
#include <sys/time.h>
#include <pthread.h>
#include "ThreadSafeKVStore.h"
#include "ThreadSafeListenerQueue.h"
#include "ThreadSafeCout.h"
#define ITERATION_NUM 10000
using namespace std;

enum Job {ACCUMULATE, LOOKUP};

class ThreadArgs {
public:
    ThreadArgs(){;}
    ThreadArgs(ThreadSafeKVStore<string, int32_t>& mymap, ThreadSafeListenerQueue<int32_t>& myQueue, int& i){
        mapPtr = &mymap;
        queuePtr = &myQueue;
        idPtr = &i;
    }
    ThreadSafeKVStore<string, int32_t> *mapPtr;
    ThreadSafeListenerQueue<int32_t> *queuePtr;
    int *idPtr;
};

void *thread_fnc(void *arguments);
int generateValue();
string generateRandomUser();
string getTimeString();

static random_device rd;
static mt19937 mt(rd());

int main( int argc, char* argv[] ){
    /* Validate the program input. */
    if(argc != 3){
        cout << "Invalid arguments. Should be like: ./run -n <N>\n";
        exit(1);
    }
    string s = argv[1];
    if(s != "-n"){
        cout << "Invalid arguments. Should be like: ./run -n <N>\n";
        exit(1);
    }
    int threadNum = stoi(argv[2]);
    srand(rd());
    
    /* Start running Lab 1 test */
    cout << "------ Lab1 Thread-safe Test Start------\n";
    cout << "N = " << threadNum << endl;
    
    ThreadSafeKVStore<string, int32_t> myMap;
    ThreadSafeListenerQueue<int32_t> myQ;
    
    pthread_t threads[threadNum];
    int index[threadNum];//record the thread id
    ThreadArgs *args = new ThreadArgs[threadNum];
    
    for (int i = 0; i < threadNum; ++i) {
        index[i] = i+1;
        ThreadArgs temp(myMap, myQ, index[i]);
        args[i] = temp;
    }
    
    /* Creating N number of threads with thread arguments.*/
    clock_t begin = clock();
    for(int i = 0; i < threadNum; i++ ) {
        int rc = pthread_create(&threads[i], NULL, &thread_fnc, (void *)&args[i]);
        
        if (rc) {
            ThreadSafeCout{} << "Error:unable to create thread," << rc << endl;
            exit(EXIT_FAILURE);
        }
    }
    
    /* Compute sum of values in queue and check result. */
    int count = 0;
    int32_t threadsOverallSum = 0;
    while(count < threadNum){
        int32_t threadValueSum = 0;
        int check = myQ.listen(threadValueSum);//check return 0
        if(check == -1){
            ThreadSafeCout{} << "Error: error occurred in listen() of queue.\n";
            exit(EXIT_FAILURE);
        }
        threadsOverallSum = threadsOverallSum + threadValueSum;
        count++;
    }
    
    clock_t end = clock();
    string result = (myMap.getSumOfAllValues() == threadsOverallSum) ? "Success" : "Fail";
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    ThreadSafeCout{} << "Program Result: " + result + "\n";
    ThreadSafeCout{} << "Runtime between launching the first thread and the final thread terminating (in second): " + to_string(elapsed_secs) + "\n";
    ThreadSafeCout{} << "------ Lab1 Thread-safe Test End------\n";
    return 0;
}

void *thread_fnc(void *arguments){
    ThreadArgs *args = (ThreadArgs *)arguments;
    vector<string> listOfKey;
    int32_t valueSum = 0;
    
    for(int i = 0; i < ITERATION_NUM; i++){
        int32_t v;
        string key;
        int randNum = rand()%100;
        Job job = (randNum < 20) ? ACCUMULATE : LOOKUP;
        
        switch (job) {
            case ACCUMULATE:
                v = generateValue();
                key = generateRandomUser();
                listOfKey.push_back(key);
                valueSum = valueSum + v;
                args->mapPtr->accumulate(key, v);
                break;
                
            case LOOKUP:
                if(!listOfKey.empty()){
                    if(args->mapPtr->lookup(listOfKey[0], v) == -1){
                        ThreadSafeCout{} << "Error: inserted key no longer present.\n";
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            default:
                ThreadSafeCout{} << "Error: job to do in thread is not defined.\n";
                break;
        }
    }
    args->queuePtr->push(valueSum);
    ThreadSafeCout{} << "Thread: " + to_string(*args->idPtr) + " completed. Time: " + getTimeString() + "\n";
    return NULL;
}

string generateRandomUser(){
    int randNum = rand() % 501;
    string user = "User" + to_string(randNum);
    return user;
}

int generateValue(){
    uniform_int_distribution<int32_t> distribution(-256, 256);
    int32_t value = distribution(mt);
    return value;
}

string getTimeString(){
    struct timeval tv;
    struct timezone tz;
    struct tm *tm;
    gettimeofday(&tv, &tz);
    tm = localtime(&tv.tv_sec);
    
    return to_string(tm->tm_hour) + ":" + to_string(tm->tm_min) + ":" +
              to_string(tm->tm_sec) + "." + to_string((tv.tv_usec));
}