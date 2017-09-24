#include "ListSet.h"


int ListSet::insert(int key){
	bool insertDone = false;
	for(int i = 0; i < set.size(); ++i){
		if(key < set[i]){
			set.insert(set.begin() + i, key);
			insertDone = true;
			break;
		}
		else if (key == set[i]){
			insertDone = true;
			break;
		}
	}
	if(!insertDone)
		set.push_back(key);

	return 0;
}

bool ListSet::find(int key){
	for(int i = 0; i < set.size(); ++i){
		if(key == set[i])
			return true;
	}
	return false;
}

int ListSet::remove(int key){
	for(int i = 0; i < set.size(); ++i){
		if(key == set[i]){
			set.erase(set.begin()+i);
			break;
		}
		else if(key < set[i]){// Key is not in the set.
			break;
		}
	}
	return 0;
}

void ListSet::printKeys(){
	for(int e : set){
		cout << e << " ";
	}
	cout << endl;
}
