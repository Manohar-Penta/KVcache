# KVcache

## Features

- LRU based cache :- Implemented using Double Linked List
- TTL support
- Memory Optimization(Limits memory usage to 1GB)
- Thread Safe Access
- Program Exclusion(file locking)
- File based data-store for saving & retrieving cache

## Set up

- Include `kvcache.hpp` in your files to use the library.
- Pass the `kvcache.cpp` while compiling your code.

- Make sure you have g++ compiler installed and properly configured.
  - Debian/Ubuntu : `sudo apt-get install build-essential`
  - Mac/OSX : `xcode-select --install`

## Tests

- There are two tests included within the repo.
- To run these test run:
  - common tests : `g++ common_tests.cpp kvcache.cpp && ./a.out`
  - thread safety tests : `g++ thread_safety_test.cpp kvcache.cpp && ./a.out`
