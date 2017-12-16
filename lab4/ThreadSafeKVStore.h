#ifndef THREADSAFEKVSTORE_H
#define THREADSAFEKVSTORE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include "ThreadSafeCout.h"
using namespace std;

template<class K, class V>
class ThreadSafeKVStore{
public:
    int insert(const K key, const V value);
    int accumulate(const K key, const V value);
    int lookup(const K key, V& value);
    int remove(const K key);
    int getSumOfAllValues();
    
    /* This method is only used for kvStore where value stores frequency of key.
       If the size of kvStore exceeds the given capacity, remove the least 
       frequently used key and return true. Otherwise return flase. */
    bool maintainSize(int capacity, K& key);
    
    void printMap(){
        for (auto it = kvStore.begin(); it != kvStore.end(); ++it){
            ThreadSafeCout{} << it->first << " -> " << it->second << endl;
        }
    }
    
private:
    unordered_map<K, V> kvStore;
    mutable std::mutex mu;
    //    std::condition_variable c;
    
};

template<class K, class V>
int ThreadSafeKVStore<K, V>::insert(const K key, const V value){
    std::lock_guard<std::mutex> guard(mu);
    kvStore[key] = value;
    return 0;
}

template<class K, class V>
int ThreadSafeKVStore<K, V>::accumulate(const K key, const V value){
    std::lock_guard<std::mutex> guard(mu);
    auto it = kvStore.find(key);
    
    /* If key already exists in map, accumulate the new value
     * to the existing value.
     */
    if (it != kvStore.end()){
        kvStore[key] = kvStore[key] + value;
    }
    else{
        kvStore[key] = value;
    }
    
    return 0;
}

template<class K, class V>
int ThreadSafeKVStore<K, V>::lookup(const K key, V& value){
    std::lock_guard<std::mutex> guard(mu);
    auto it = kvStore.find(key);
    /* If key exists in map, update the value passed by reference. */
    if (it != kvStore.end()){
        value = kvStore[key];
        return 0;
    }
    else{
        return -1;
    }
    return 0;
}

template<class K, class V>
bool ThreadSafeKVStore<K, V>::maintainSize(int capacity, K& key){
    std::lock_guard<std::mutex> guard(mu);
    
    if (kvStore.size() <= capacity) {
        return false;
    }
    int minFreq = INT_MAX;
    for (auto it = kvStore.begin(); it != kvStore.end(); ++it){
        if (it->second < minFreq) {
            minFreq = it->second;
            key = it->first;
        }
    }
    auto it = kvStore.find(key);
    kvStore.erase(it);
    return true;
}

template<class K, class V>
int ThreadSafeKVStore<K, V>::remove(const K key){
    std::lock_guard<std::mutex> guard(mu);
    auto it = kvStore.find(key);
    /* If key exists in map, remove the key*/
    if (it != kvStore.end()){
        kvStore.erase(it);
    }
    return 0;
}

/* A thread-unsafe way to access all the elements in this kvStore
 * and add all of its values together.
 */
template<class K, class V>
int ThreadSafeKVStore<K, V>::getSumOfAllValues(){
    //    std::lock_guard<std::mutex> guard(mu);
    auto it = kvStore.begin();
    V sum = it->second;
    it++;
    for(; it != kvStore.end(); it++){
        sum = sum + it->second;
    }
    return sum;
}

//template<class K, class V>
//int ThreadSafeKVStore<K, V>::getSize(){
//    std::lock_guard<std::mutex> guard(mu);
//    return kvStore.size();
//}

#endif
