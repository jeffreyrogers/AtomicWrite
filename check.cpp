// Simple sanity test for AtomicWrite. This doesn't verify the atomicity of the
// operations, but does show that the code at least writes to a file.
//
// This should work on all supported platforms.

#include <fstream>
#include <iostream>
#include <sstream>

#include "AtomicWrite.h"

int
main(int argc, char *argv[])
{
	std::string fname("check.result");
	std::string data("Far out in the uncharted backwaters of the "
			"unfashionable end of the western spiral arm of the "
			"Galaxy lies a small, unregarded yellow sun.");
	AtomicWrite::write(fname, data);

	std::ifstream tmp("check.result");
	std::stringstream result;
	result << tmp.rdbuf();
	tmp.close();

	if (result.str() == data) {
		std::cout << "PASSED" << std::endl;
	} else {
		std::cout << "FAILED" << std::endl;
	}
}
