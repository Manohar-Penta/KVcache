# KVcache

## Features

- LRU based cache :- Implemented using Double Linked List
- TTL support :- Implemented using a Priority Queue(b'cuz C++ doesn't have inbuilt timeout callbacks)
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

## Structure

Details of functions available with library:

```
class KVCache {
public:
    // Constructor
    KVCache();

    // Destructor
    ~KVCache();

    // query a key-value pair
    json getKey(std::string key);

    // create a key-value pair
    void putKey(std::string key, std::string value, int expiry = -1, Callback callback = defaultCallbackHandler);

    // delete a key-value pair
    void deleteKey(std::string key, Callback callback = defaultCallbackHandler);

    // create a batch of key-value pairs
    void batchCreate(int n, KVE val[], Callback callback = defaultCallbackHandler);
};
```

**Callback Function Type**
Callback function is passed errors encountered in the calling function.
`typedef void (*Callback)(std::vector<Error_obj> err);`

**Default Callback**

```
static void defaultCallbackHandler(std::vector<Error_obj> err)
    {
        for (int i = 0; i < err.size(); i++)
        {
            std::cout << err[i].code << " " << err[i].errmsg << " " << err[i].key << " " << err[i].value << std::endl;
        }
    }
```

**Error_obj Structure**

```
struct Error_obj {
    Error_code code;
    std::string errmsg;
    std::string key;
    std::string value;
};
```

**Enum for Error Codes**

```
enum Error_code
{
    KEY_NOT_FOUND,
    KEY_ALREADY_EXISTS,
    KEY_TOO_LONG,
    VALUE_TOO_LONG,
    INVALID_VALUE,
    UNKNOWN_ERROR
};
```

**_Note:_**

- The Callback function type is a pointer to a function that takes a std::vector<Error_obj> as an argument and returns void.
- The Error_obj structure represents an error object with a code, message, and Key value pair of the operation.
- The ErrorCode enum defines a set of error codes that can be used in the Error_obj structure.
- You can add or modify these structures and enums as needed to fit your specific use case.

## Tests

- There are two tests included within the repo.
- To run these test run:
  - common tests : `g++ common_tests.cpp kvcache.cpp && ./a.out`
  - thread safety tests : `g++ thread_safety_test.cpp kvcache.cpp && ./a.out`
- These tests cover following cases:
  1. create
  2. get
     1. get key
     2. get non existent key
  3. delete
     1. delete key
     2. delete non existent key
  4. Invalid key | value
  5. create(TTL)
  6. batch create
  7. Simultaneous Thread operation
