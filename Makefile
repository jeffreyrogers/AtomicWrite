.PHONY: check
check:
	clang++ -o check -O3 check.cpp AtomicWrite.cpp
	./check

clean:
	rm -f check check.result
