#include <iostream>
#include <array>
#include "module.h"

struct Point { int x, y; };

template<typename Arr>
void print(const Arr& a) {
	for (int i = 0; i < a.size(); ++i) {
		std::cout << a[i];
	}
}

int main() {

	Array<int, 10> a;
	//std::cout << a[1];
	print(a);

}