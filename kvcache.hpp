#ifndef DLL_HPP
#define DLL_HPP

#include <string>
#include <unordered_map>
#include <queue>
#include "json.hpp"
#include <mutex>
#include <condition_variable>
#include <fcntl.h>
#include <iostream>

using nlohmann::json;

// Double Linked List Node definition
class Node
{
public:
    std::string key;
    json data;
    int expiry;
    Node *next;
    Node *prev;

    Node(std::string key, json data, int expiry = -1, Node *prev = nullptr, Node *next = nullptr);
};

// Key-Value-Expiry object
struct KVE
{
    std::string key;
    json data;
    int expiry = -1;
};

enum Error_code
{
    KEY_NOT_FOUND,
    KEY_ALREADY_EXISTS,
    KEY_TOO_LONG,
    VALUE_TOO_LONG,
    INVALID_VALUE,
    UNKNOWN_ERROR
};

struct Error_obj
{
    Error_code code;
    std::string errmsg;
    std::string key;
    std::string value;
};

// callback function type declaration
typedef void (*Callback)(std::vector<Error_obj> err);

class KVcache
{
    // -----------------------critical Section-----------------
    int capacity, size;
    Node *head;
    Node *tail;
    std::unordered_map<std::string, Node *> cache;
    std::priority_queue<std::pair<int, std::string>, std::vector<std::pair<int, std::string>>, std::greater<std::pair<int, std::string>>> pq;
    std::string file;

    // locks, mutexes and condition variables
    std::mutex m;
    std::condition_variable cv;
    int fd;
    flock lock;
    // -------------------------------------------------------

    void removeNode(Node *node);
    void insertAfterStart(Node *node);
    void clearExpired();
    void exportFile();
    void importFile(json &j);
    static void defaultCallbackHandler(std::vector<Error_obj> err)
    {
        for (int i = 0; i < err.size(); i++)
        {
            std::cout << err[i].code << " " << err[i].errmsg << " " << err[i].key << " " << err[i].value << std::endl;
        }
    }

public:
    KVcache(std::string name = "./data-store.json");
    ~KVcache();
    json getKey(std::string key);
    void putKey(std::string key, std::string value, int expiry = -1, Callback callback = defaultCallbackHandler);
    void deleteKey(std::string key, Callback callback = defaultCallbackHandler);
    void batchCreate(int n, KVE val[], Callback callback = defaultCallbackHandler);
};

#endif