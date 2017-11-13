#include <iostream>
#include <string>
#include <cstdint>
// #include <random>
// #include <ctime>
// #include <sys/time.h>
#include <pthread.h>
#include "ThreadSafeKVStore.h"
#include "ThreadPoolServer.h"
#include "ThreadSafeCout.h"
// #define ITERATION_NUM 10000
using namespace std;



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
    int threadNumber = stoi(argv[2]);

    //stores the value and md5 value(in string) of the key
    ThreadSafeKVStore<string, pair<string, string>> kvStore;

    ThreadPoolServer threadPoolServer(threadNumber, kvStore);
    threadPoolServer.run();

    return 0;
}
