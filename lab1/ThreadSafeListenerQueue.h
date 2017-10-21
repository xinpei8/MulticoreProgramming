#include <iostream>
#include <list>
#include <mutex>
#include <condition_variable>
#include "ThreadSafeCout.h"
using namespace std;

template<class T> 
class ThreadSafeListenerQueue{
public:
	int  push(const T element);
	int  pop(T& element);
	int listen(T& key);
	void printQ();

private:
	list<T> listenerQ;
    mutable std::mutex mu;
    std::condition_variable c;
};

template<class T>
int ThreadSafeListenerQueue<T>::push(const T element){
    std::lock_guard<std::mutex> lock(mu);
    listenerQ.push_front(element);
    c.notify_one();
    return 0;
}

template<class T>
int ThreadSafeListenerQueue<T>::pop(T& element){
    std::lock_guard<std::mutex> lock(mu);
    if (listenerQ.empty()) {
        return 1;
    }
    element = listenerQ.back();
    listenerQ.pop_back();
    return 0;
}

template<class T>
int ThreadSafeListenerQueue<T>::listen(T& element){
    std::unique_lock<std::mutex> lock(mu);
    while (listenerQ.empty()) {
        c.wait(lock);//wait until there is an element in queue.
    }
    element = listenerQ.back();
    listenerQ.pop_back();
    return 0;
}

template<class T>
void ThreadSafeListenerQueue<T>::printQ(){
    for (auto e : listenerQ) {
        cout << e << " ";
    }
    cout << endl;
}