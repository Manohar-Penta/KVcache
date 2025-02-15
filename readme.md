# KVcache

## Features

- LRU based cache
- TTL support
- Memory Optimization(Limits memory usage to 1GB)
- Thread Safe Access
- Program Exclusion(file locking)
- File based data-store for saving & retrieving cache

## Set up

- Include `kvcache.hpp` in your files to use the library.
- Pass the `kvcache.cpp` while compiling your code.

## Tests

- There are two tests included within the repo.
- To run these test run:
  - common tests : `g++ common_tests.cpp kvcache.cpp && ./a.out`
  - thread safety tests : `g++ thread_safety_test.cpp kvcache.cpp && ./a.out`
