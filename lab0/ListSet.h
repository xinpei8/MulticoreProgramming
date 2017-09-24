#include <iostream>
#include <vector>
using namespace std;

template<class T> 
class ListSet{
public:
	int insert(const T &key);

	bool find(const T &key);

	int remove(const T &key);

	void printKeys();

private:
	vector<T> set;

};
