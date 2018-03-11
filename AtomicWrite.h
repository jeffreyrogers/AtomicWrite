#pragma once

#include <string>

namespace AtomicWrite {
void write(std::string fname, std::string data);

struct FailedAtomicWrite : public std::exception {
	const char * what() const throw() {
		return "Could not write file atomically";
	}
};
}
