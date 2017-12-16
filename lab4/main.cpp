#include <iostream>
#include <string>
#include <cstdint>
#include <pthread.h>
#include "ThreadSafeKVStore.h"
#include "ThreadPoolServer.h"
#include "ThreadSafeCout.h"
using namespace std;

int main( int argc, char* argv[] ){
    
    /* Validate the program input. */
    if(argc != 5){
        cout << "Invalid arguments. Should be like: ./run -n <threadNumber> <portNumber> <directory>\n";
        exit(1);
    }
    string s = argv[1];
    if(s != "-n"){
        cout << "Invalid arguments. Should be like: ./run -n <threadNumber> <portNumber> <directory>\n";
        exit(1);
    }

    int threadNumber = stoi(argv[2]);
    int portNumber = stoi(argv[3]);
    string directory = argv[4];
    if(directory.size() > 10){
        cout << "direcotry string should be within size 10\n";
        exit(1);
    }

    /* Stores the value and md5 value(in string) of the key */
    ThreadSafeKVStore<string, pair<string, string>> kvStore;

    /* Initialize the threadPoolServer with given thread number. */
    ThreadPoolServer threadPoolServer(threadNumber, kvStore, portNumber, directory);
    threadPoolServer.run();

    return 0;
}