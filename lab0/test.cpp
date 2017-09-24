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


/*
The executable should perform 10 tests. In each test, it selects 100 random integers between
0 and 200 (inclusive), adds those to the set, then checks that each of those integers exists in
the set. Hint: I recommend using std::uniform_int_distribution to generate your 
integers, a list to store the integers inserted, and a loop over that list to check that each of the
values in the list is in the set. At the end of the test, it should report whether the test
succeeded or failed. I would prefer that you have a submission that correctly tests itself and
finds that the implementation was incorrect than a submission that is correct but in which
you didn’t have time to add the test
*/
	for(int t = 0; t < TEST_NUM; ++t){
		vector<int> numList(100, 0);
  		ListSet myset;
		
		cout << "Run Test " << t + 1 << " ...\n";

		for (int i = 0; i < RAND_NUM ; ++i) {
    		int number = distribution(generator);
    		numList[i] = number;
    		myset.insert(number);
  		}

		bool isInSet = true;
  		for (int i = 0; i < numList.size() ; ++i) {
    		bool isInSet = myset.find(numList[i]);
    		if(!isInSet){
    			break;
    		} 
  		}

  		if(isInSet)
  			cout << "------ Test " << t + 1 << " succeeded. ------" << endl; 
  		else
  			cout << "------ Test " << t + 1 << " failed. ------" << endl; 
	}
	cout << "------ Lab0 ListSet Test End ------\n";
	return 0;
}