#include "ListSet.h"
#include <string>

//Insert key to the set in ascending order if key didn't exit in the set.
template<class T> 
int ListSet<T>::insert(const T &key){
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

template<class T> 
bool ListSet<T>::find(const T &key){
	for(int i = 0; i < set.size(); ++i){
		if(key == set[i])
			return true;
		else if(key > set[i])
			return false;
	}

	return false;
}

template<class T> 
int ListSet<T>::remove(const T &key){
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

template<class T> 
void ListSet<T>::printKeys(){
	for(T e : set){
		cout << e << " ";
	}
	cout << endl;
}

template class ListSet<int>;
template class ListSet<float>;
template class ListSet<short>;
template class ListSet<double>;
template class ListSet<char>;
template class ListSet<string>;
