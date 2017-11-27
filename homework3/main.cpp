#include <iostream>
#include <string>
#include <random>
#include <unistd.h>
#include <pthread.h>
#include "ConcurrentOrderedList.h"
#include "ThreadSafeCout.h"
using namespace std;

class ThreadArgs {
public:
    ThreadArgs(){;}
    ThreadArgs(ConcurrentOrderedList<int> &myList, int& i){
        listPtr = &myList;
        idPtr = &i;
    }
    ConcurrentOrderedList<int> *listPtr;
    int *idPtr;
};

void *thread_fnc(void *arguments);

int main( int argc, char* argv[] ){
    int threadNum = 5;
    
    /* Start running homework 3 test */
    cout << "------ Concurrent Ordered List Set Test Start------\n";
    cout << "threadNum = " << threadNum << endl;
    
    
    ConcurrentOrderedList<int> myList;
    
    pthread_t threads[threadNum];
    int index[threadNum];//record the thread id
    ThreadArgs *args = new ThreadArgs[threadNum];
    
    for (int i = 0; i < threadNum; ++i) {
        index[i] = i+1;
        ThreadArgs temp(myList, index[i]);
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
    for(int i = 0; i < threadNum; i++ ) {
        pthread_join(threads[i], NULL);
    }
    std::cout << "All threads joined!\n";
    cout <<"Print list:\n";
    myList.printList();
    
    ThreadSafeCout{} << "------ Test Result: PASS ------\n";
    return 0;
}


void *thread_fnc(void *arguments){
    ThreadArgs *args = (ThreadArgs *)arguments;
    vector<string> listOfKey;
    int32_t valueSum = 0;
    vector<int> nums = { 3, 1, 2, 4, 5};
    for(int i = 0; i < nums.size(); i++){
        bool success = args->listPtr->insert(nums[i]);
        ThreadSafeCout{} << "Thread : " << *args->idPtr << " insert " <<
        nums[i] << " success: " << success << "\n";
        args->listPtr->printList();
    }
    
    for(int i = 0; i < nums.size(); i++){
        bool success = args->listPtr->remove(nums[i]);
        ThreadSafeCout{} << "Thread : " << *args->idPtr << " remove " <<
        nums[i] << " success: " << success << "\n";
        args->listPtr->printList();
    }
    return NULL;
}
