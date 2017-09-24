#include <iostream>
#include <vector>
#include <random>
#include "ListSet.h"
using namespace std;
#define RAND_NUM 100
#define TEST_NUM 10

int main( int argc, char* argv[] ){
	cout << "------ Lab0 ListSet Test Start------\n";
	default_random_engine generator;
	uniform_int_distribution<int> distribution(0, 200);

	//Run the tests and record the results. 
	int passCount = 0;
	for(int t = 0; t < TEST_NUM; ++t){
		vector<int> numList(100, 0);
  		ListSet<int> myset;
		
		cout << "\nRun Test " << t + 1 << " ...\n";

		//Generate random numbers and store them in numList.
		for (int i = 0; i < RAND_NUM ; ++i) {
    		int number = distribution(generator);
    		numList[i] = number;
			myset.insert(number);
		}

		sort(numList.begin(), numList.end());
		cout << "\nSorted Random Numbers:\n";
		for(auto e : numList)
			cout << e << " ";

		cout << "\nNumbers in Set:\n";

		//Find the numbers in myset and check if insert succeeded.
		bool isInSet = true;
  		for (int i = 0; i < numList.size() ; ++i) {
    		bool isInSet = myset.find(numList[i]);
    		if(!isInSet){
    			break;
    		} 
		  }
		 
		myset.printKeys();

  		if(isInSet){
			  cout << "------ Test " << t + 1 << " succeeded. ------" << endl;
			  ++passCount;
		}
  		else{
			  cout << "------ Test " << t + 1 << " failed. ------" << endl; 
		}
	}

	cout << "\nLab0 passed " << passCount << " out of " << TEST_NUM << " tests.\n\n";
	cout << "------ Lab0 ListSet Test End ------\n";
	return 0;
}