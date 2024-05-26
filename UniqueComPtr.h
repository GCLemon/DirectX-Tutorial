#pragma once

#include <memory>
#include <Windows.h>

using namespace std;

// COM�f���[�^
class UComDeleter {
public:
	void operator()(IUnknown* ptr) const {
		if (ptr) { ptr->Release(); }
	}
};

// ���j�[�N��COM�|�C���^
template <typename T>
using unique_com_ptr = unique_ptr<T, UComDeleter>;