ifeq ($(OS),Windows_NT)
.PHONY: check
check:
	cl /EHsc check.cpp AtomicWrite.cpp
	./check
else
.PHONY: check
check:
	clang++ -o check -std=c++14 -O3 check.cpp AtomicWrite.cpp
	./check
endif

clean:
	rm -f check check.result
