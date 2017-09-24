#include <iostream>
#include <vector>
#include <random>
#include "ListSet.h"
using namespace std;
#define RAND_NUM 100

// class ListSet{
// public:
// 	int insert(int key){
// 		bool insertDone = false;
// 		for(int i = 0; i < set.size(); ++i){
// 			if(key < set[i]){
// 				set.insert(set.begin() + i, key);
// 				insertDone = true;
// 				break;
// 			}
// 			else if (key == set[i]){
// 				insertDone = true;
// 				break;
// 			}
// 		}
// 		if(!insertDone)
// 			set.push_back(key);

// 		return 0;
// 	}

// 	bool find(int key){
// 		for(int i = 0; i < set.size(); ++i){
// 			if(key == set[i])
// 				return true;
// 		}
// 		return false;
// 	}

// 	int remove(int key){
// 		for(int i = 0; i < set.size(); ++i){
// 			if(key == set[i]){
// 				set.erase(set.begin()+i);
// 				break;
// 			}
// 			else if(key < set[i]){// Key is not in the set.
// 				break;
// 			}
// 		}
// 		return 0;
// 	}

// 	void printKeys(){
// 		for(int e : set){
// 			cout << e << " ";
// 		}
// 		cout << endl;
// 	}

// private:
// 	vector<int> set;

// };




int main( int argc, char* argv[] ){
	cout << "< Lab0 ListSet Test> ";
	default_random_engine generator;
	uniform_int_distribution<int> distribution(0, 200);
  	vector<int> insertList(100, 0);
  	ListSet myset;

  	cout << "Random Num:" << endl;


  	// vector<int> insertList(0,100);

  	for (int i = 0; i < RAND_NUM ; ++i) {
    	int number = distribution(generator);
    	insertList[i] = number;
    	myset.insert(number);
  	}

  	sort(insertList.begin(),insertList.end());
  	for(auto e : insertList) cout << e << " ";
 		cout << "\n\n\n" ;

    myset.printKeys();
    cout << myset.find(99) << endl;
    cout << myset.find(4) << endl;
    myset.remove(199);
    myset.printKeys();


  return 0;
}