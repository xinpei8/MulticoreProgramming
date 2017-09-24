#include <iostream>
#include <vector>
using namespace std;


class ListSet{
public:
	int insert(int key);

	bool find(int key);

	int remove(int key);

	void printKeys();

private:
	vector<int> set;

};
