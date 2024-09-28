#pragma once

template<typename T, int N>
class Array {
public:

	T& operator[](int index);

	T operator[](int index) const;


	int size() const {
		return N;
	}

private:
	T items[N];
};
