#ifndef DLL_HPP
#define DLL_HPP

#include <string>
#include <unordered_map>
#include <queue>
#include "json.hpp"
#include <mutex>
#include <condition_variable>
#include <fcntl.h>

using nlohmann::json;

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

struct KVE
{
    std::string key;
    json data;
    int expiry;
};

class KVcache
{
    int capacity, size;
    Node *head;
    Node *tail;
    std::unordered_map<std::string, Node *> cache;
    std::priority_queue<std::pair<int, std::string>, std::vector<std::pair<int, std::string>>, std::greater<std::pair<int, std::string>>> pq;
    std::string file;

    std::mutex m;
    std::condition_variable cv;
    int fd;
    flock lock;

    void removeNode(Node *node);
    void insertAfterStart(Node *node);
    void clearExpired();
    void exportFile();
    void importFile(json &j);

public:
    KVcache(std::string name = "./data-store.json");

    json getKey(std::string key);
    void putKey(std::string key, std::string value, int expiry = -1);
    void deleteKey(std::string key);
    void batchCreate(int n, KVE val[]);
};

#endif