#pragma once

#include <memory>
#include <Windows.h>

using namespace std;

// COMデリータ
class UComDeleter {
public:
	void operator()(IUnknown* ptr) const {
		if (ptr) { ptr->Release(); }
	}
};

// ユニークなCOMポインタ
template <typename T>
using unique_com_ptr = unique_ptr<T, UComDeleter>;