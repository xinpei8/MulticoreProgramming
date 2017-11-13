#ifndef THREADSAFECOUT_H
#define THREADSAFECOUT_H

#include <iostream>
#include <sstream>
#include <string>
using namespace std;


/** Thread safe cout class
 * Exemple of use:
 *    ThreadSafeCout{} << "Hello world!\n";
 */
class ThreadSafeCout: public ostringstream{
public:
    ThreadSafeCout() = default;
    
    ~ThreadSafeCout()
    {
        std::lock_guard<std::mutex> guard(_mutexPrint);
        std::cout << this->str();
    }
    
private:
    static std::mutex _mutexPrint;
};

std::mutex ThreadSafeCout::_mutexPrint{};

#endif