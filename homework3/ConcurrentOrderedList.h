#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include "ThreadSafeCout.h"
#include "ThreadSafeCout.h"
using namespace std;

template<class E>
class Node{
public:
    Node(){
        pre = nullptr;
        next = nullptr;
    }
    Node(const E v){
        value = v;
        pre = nullptr;
        next = nullptr;
    }
    E value;
    Node *pre;
    Node *next;
    mutable std::mutex mu;
};

template<class N>
class Response{
public:
    Response(Node<N>* p, Node<N> *c, Node<N> *n, bool hasFound){
        pre = p;
        cur = c;
        next = n;
        found = hasFound;
    }
    Node<N> *pre;
    Node<N> *cur;
    Node<N> *next;
    bool found;
};

template<class T>
class ConcurrentOrderedList{
public:
    
    ConcurrentOrderedList(){
        head = new Node<T>();
    }
    Response<T> lookup(const T key, bool onlyLookup);
    bool insert(const T key);
    bool remove(const T key);
    void printList(){
        std::lock_guard<std::mutex> lock(mu);
        Node<T>* ptr = head->next;
        cout << "List: ";
        while (ptr) {
            cout << ptr->value << " ";
            ptr = ptr->next;
        }
        cout << endl;
    }
    
private:
    Node<T>* head;// a dummy head points to the first node in list
    mutable std::mutex mu; // for printList()
};

template<class T>
Response<T> ConcurrentOrderedList<T>::lookup(const T key, bool onlyLookup){
    ThreadSafeCout{} << "Lookup key = " << key << "\n";
    head->mu.lock();
    if (head->next == nullptr) {
        ThreadSafeCout{} << "list is empty\n";
        if (onlyLookup) {
            head->mu.unlock();
        }
        return Response<T>(head, nullptr, nullptr, false);
    }
    head->mu.unlock();
    Node<T> *ptr = head->next;
    Node<T> *parent = head;
    while (ptr) {
        ptr->pre->mu.lock();
        ptr->mu.lock();
        
        if(ptr->value == key){
            // Key found at *cur
            ThreadSafeCout{} << "found key\n";
            
            if (onlyLookup) {
                ptr->mu.unlock();
                ptr->pre->mu.unlock();
            }
            
            return Response<T>(ptr->pre, ptr, ptr->next, true);
        }
        else if(ptr->value > key){
            // Key not found. Should be between *pre and *cur
            
            if (onlyLookup) {
                ptr->mu.unlock();
                ptr->pre->mu.unlock();
            }
            
            return Response<T>(ptr->pre, ptr, ptr->next, false);
        }
        
        ptr->mu.unlock();
        ptr->pre->mu.unlock();
        // move to next node
        parent = ptr;
        ptr = ptr->next;
    }
    // Key not found. Should be between *pre and *cur
    if (!onlyLookup) {
        parent->mu.lock();
    }
    return Response<T>(parent, nullptr, nullptr, false);
}

template<class T>
bool ConcurrentOrderedList<T>::insert(const T key){
    Response<T> response = lookup(key, false);
    if (response.found) {
        if (response.cur != nullptr) {
            response.cur->mu.unlock();
        }
        response.pre->mu.unlock();
        return false;
    }
    
    // Create a new node with key
    Node<T> *newNode = new Node<T>(key);
    newNode->pre = response.pre;
    newNode->next = response.cur;
    
    // Insert the new node into list
    response.pre->next = newNode;
    if (response.cur != nullptr) {
        response.cur->pre = newNode;
        response.cur->mu.unlock();
    }
    response.pre->mu.unlock();
    return true;
}

template<class T>
bool ConcurrentOrderedList<T>::remove(const T key){
    Response<T> response = lookup(key, false);
    if (!response.found) {
        if (response.cur != nullptr) {
            response.cur->mu.unlock();
        }
        response.pre->mu.unlock();
        return false;
    }
    
    
    // Remove the node with key from list
    response.pre->next = response.next;
    if (response.next != nullptr) {
        response.next->pre = response.pre;
    }
    delete response.cur;
    response.pre->mu.unlock();

    return true;
}
