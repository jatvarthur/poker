#include "module.h"

template<>
int& Array<int, 10>::operator[](int index) {
	return items[index];
}

template<>
int Array<int, 10>::operator[](int index) const { //?
	return items[index];
}
