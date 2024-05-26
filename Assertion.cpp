#include "Assertion.h"

void Assert(bool result, const char* file, size_t line, const char* message) {
	if (result) {
		stringstream stream;
		stream << "File : " << file << endl;
		stream << "Line : " << line << endl;
		stream << system_category().message(result) << endl;
		stream << message << endl;
		throw exception(stream.str().c_str());
	}
}