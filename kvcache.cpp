#include "kvcache.hpp"
#include <string>
#include <ctime>
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

using json = nlohmann::json;

Node::Node(std::string key, json data, int expiry, Node *prev, Node *next)
{
    this->data = data;
    this->expiry = expiry;
    this->prev = prev;
    this->next = next;
    this->key = key;
}

KVcache::KVcache(std::string name)
{
    head = new Node("", json::object());
    tail = new Node("", json::object(), -1, head);
    file = name;

    size = 0;
    capacity = 1024 * 1024 * 1024;

    head->next = tail;

    json j;
    {
        try
        {
            // reading from the file and converting to json
            std::ifstream initFile(name);
            std::string content;
            getline(initFile, content);
            j = content.empty() ? json::object() : json::parse(content);
        }
        catch (const std::exception &e)
        {
            std::cerr << "error while importing : " << e.what() << std::endl;
        }
    }

    // file locking for process exclusion
    fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;

    if (fd < 0)
    {
        throw "File cannot be opened or already in use!!";
    }

    fcntl(fd, F_SETLKW, &lock);

    // importing from the file after locking it
    importFile(j);
    exportFile();
};

// destructor
KVcache::~KVcache()
{
    delete head, tail;
}
json KVcache::getKey(std::string key)
{
    // acquiring lock for the mutex
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });

    // clearing expired entries
    clearExpired();

    // returning early if key does not exist
    if (cache.find(key) == cache.end())
    {
        ul.unlock();
        cv.notify_one();
        return "{}"_json;
    }

    Node *node = cache[key];
    removeNode(node);
    insertAfterStart(node);

    // releasing the lock and notifying other threads
    ul.unlock();
    cv.notify_one();

    return node->data;
}

void KVcache::putKey(std::string key, std::string value, int expiry, Callback callback)
{
    // acquiring lock for the mutex
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });
    std::vector<Error_obj> err;
    try
    {
        clearExpired();
        // validating key and value lengths
        if (key.size() > 32)
        {
            // jumping to the callback stage
            err.push_back({Error_code::KEY_TOO_LONG, "key too long", key, value});
            goto callback_stage;
        }

        if (value.size() > 16 * 1024)
        {
            // jumping to the callback stage
            err.push_back({Error_code::VALUE_TOO_LONG, "Value too long", key, value});
            goto callback_stage;
        }

        // checking if the key already exists in the cache
        if (cache.find(key) == cache.end())
        {
            int currsize = key.size() + value.size();
            json data = json::parse(value);
            // removing the least recently used(LRU) entries to free up space
            while (size + currsize > capacity)
            {
                Node *endNode = tail->prev;
                int endSize = endNode->key.size() + endNode->data.dump().size();
                removeNode(endNode);
                cache.erase(endNode->key);
                delete endNode;
                size -= endSize;
            }

            size += currsize;

            // adding to cache and Double linked list
            cache[key] = new Node(key, data, expiry == -1 ? -1 : expiry + time(NULL));
            insertAfterStart(cache[key]);

            // if expiry is set, adding the key to the priority queue
            if (expiry != -1)
            {
                pq.push({expiry + time(nullptr), key});
            }

            exportFile();
        }
        else
        {
            err.push_back({Error_code::KEY_ALREADY_EXISTS, "key already exists", key, value});
        }
    }
    catch (const std::exception &e)
    {
        err.push_back({Error_code::UNKNOWN_ERROR, std::string(e.what()), key, value});
    }

callback_stage:
    callback(err);

    // releasing the lock and notifying other threads
    ul.unlock();
    cv.notify_one();
}

void KVcache::batchCreate(int n, KVE val[], Callback callback)
{
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });

    std::vector<Error_obj> err;
    for (int i = 0; i < n; i++)
    {
        std::string key = val[i].key;
        json data = val[i].data;
        int expiry = val[i].expiry == -1 ? -1 : val[i].expiry + time(NULL);

        // Validating sizes of key and value

        if (key.size() > 32)
        {
            err.push_back({Error_code::KEY_TOO_LONG, "key too long", key, data.dump()});
            continue;
        }

        if (data.dump().size() > 16 * 1024)
        {
            err.push_back({Error_code::VALUE_TOO_LONG, "Value too long", key, data.dump()});
            continue;
        }

        try
        {
            if (cache.find(val[i].key) != cache.end())
            {
                err.push_back({Error_code::KEY_ALREADY_EXISTS, "key already exists", key, data.dump()});
                continue;
            }

            int currsize = key.size() + data.dump().size();

            // freeing up the LRU if needed for new entries
            while (size + currsize > capacity)
            {
                Node *endNode = tail->prev;
                int endSize = endNode->key.size() + endNode->data.dump().size();
                removeNode(endNode);
                cache.erase(endNode->key);
                delete endNode;
                size -= endSize;
            }

            size += currsize;

            cache[key] = new Node(key, data, expiry);
            insertAfterStart(cache[key]);

            // if expiry is set, adding the key to the priority queue
            if (expiry != -1)
            {
                pq.push({expiry, key});
            }
        }
        catch (const std::exception &e)
        {
            err.push_back({Error_code::UNKNOWN_ERROR, std::string(e.what()), key, data.dump()});
        }
    }

    callback(err);
    exportFile();
    ul.unlock();
    cv.notify_one();
}

void KVcache::deleteKey(std::string key, Callback callback)
{
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });

    std::vector<Error_obj> err;
    // checking if the key exists
    if (cache.find(key) != cache.end())
    {
        Node *node = cache[key];
        removeNode(node);
        cache.erase(key);
        delete node;
        exportFile();
    }
    else
    {
        // printing an error if the key does not exist
        err.push_back({Error_code::KEY_NOT_FOUND, "key not found", key, ""});
    }

    callback(err);
    ul.unlock();
    cv.notify_one();
}

// inserts at the start of the doubly linked list(LRU)
void KVcache::insertAfterStart(Node *node)
{
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

// removes a node from the doubly linked list(LRU)
void KVcache::removeNode(Node *node)
{
    Node *x = node->prev;
    Node *y = node->next;

    x->next = y;
    y->prev = x;
}

// clears expired entries
void KVcache::clearExpired()
{
    if (pq.empty())
        return;

    int now = time(NULL);

    // removing expired entries which have TTL less than current time
    while (!pq.empty() && pq.top().first <= now)
    {
        std::string key = pq.top().second;
        int currExpiry = pq.top().first;
        pq.pop();

        if (cache.find(key) == cache.end())
        {
            continue;
        }
        Node *node = cache[key];

        // Skipping the clear if the node's expiry has reset
        if (node->expiry != currExpiry)
        {
            continue;
        }

        size -= (node->data.size() + key.size());

        cache.erase(key);
        removeNode(node);
        delete node;
    }
    exportFile();
}

void KVcache::exportFile()
{
    try
    {
        // initializing a json object
        json j = json::object();
        int now = time(nullptr);

        // appending all the entries to the json object from the cache
        for (auto [key, node] : cache)
        {
            j[key]["data"] = node->data;
            j[key]["expiry"] = node->expiry;
        }

        // writing to the file as a json object
        std::string data = j.dump();
        ftruncate(fd, 0);
        lseek(fd, 0, SEEK_SET);
        write(fd, data.c_str(), data.size());
    }
    catch (json::exception &e)
    {
        std::cout << "error while exporting ";
        std::cout << e.what() << std::endl;
    }
}

void KVcache::importFile(json &j)
{
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });
    try
    {
        int now = time(NULL);

        // iterating over the json object
        for (auto [key, value] : j.items())
        {
            try
            {
                if (cache.find(key) != cache.end())
                {
                    continue;
                }

                // skipping the entry if it has expired
                if (value["expiry"] != -1 && value["expiry"] < now)
                {
                    continue;
                }

                // breaking if the capacity is exceeded
                int itemSize = key.size() + value["data"].dump().size();
                if (itemSize + size > capacity)
                {
                    break;
                }

                size += itemSize;

                Node *node = new Node(key, value["data"], value["expiry"]);
                insertAfterStart(node);
                cache[key] = node;

                // checking if the entry has an expiry
                if (value["expiry"] != -1 && value["expiry"] > now)
                {
                    pq.push({value["expiry"], key});
                }
            }
            catch (const std::exception &e)
            {
                std::cout << "error while importing(1) ";
                std::cerr << e.what() << '\n';
            }
        }
    }
    catch (json::exception &e)
    {
        std::cout << "error while importing(2) ";
        std::cout << e.what() << std::endl;
    }

    ul.unlock();
    cv.notify_one();
}