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
            std::ifstream initFile(name);
            std::string content;
            getline(initFile, content);
            j = json::parse(content);
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

    importFile(j);
    exportFile();
};

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

void KVcache::putKey(std::string key, std::string value, int expiry)
{
    // acquiring lock for the mutex
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });
    try
    {
        clearExpired();
        if (cache.find(key) == cache.end())
        {
            cache[key] = new Node(key, json::parse(value), expiry == -1 ? -1 : expiry + time(NULL));
            int currsize = key.size() + value.size();
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
            insertAfterStart(cache[key]);
            if (expiry != -1)
            {
                pq.push({expiry + time(nullptr), key});
            }
            exportFile();
        }
        else
        {
            throw std::runtime_error(key + " already exists");
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "error while Creating entry : ";
        std::cerr << e.what() << '\n';
    }

    // releasing the lock and notifying other threads
    ul.unlock();
    cv.notify_one();
}

void KVcache::batchCreate(int n, KVE val[])
{
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });

    for (int i = 0; i < n; i++)
    {
        std::string key = val[i].key;
        json data = val[i].data;
        int expiry = val[i].expiry == -1 ? -1 : val[i].expiry + time(NULL);
        try
        {
            if (cache.find(val[i].key) != cache.end())
                continue;
            cache[key] = new Node(key, data, expiry);
            int currsize = key.size() + data.dump().size();
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
            insertAfterStart(cache[key]);
            if (expiry != -1)
            {
                pq.push({expiry, key});
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }

    exportFile();
    ul.unlock();
    cv.notify_one();
}

void KVcache::deleteKey(std::string key)
{
    std::unique_lock ul(m);
    cv.wait(ul, []()
            { return true; });

    if (cache.find(key) != cache.end())
    {
        Node *node = cache[key];
        removeNode(node);
        cache.erase(key);
        delete node;
        exportFile();
    }

    ul.unlock();
    cv.notify_one();
}

void KVcache::insertAfterStart(Node *node)
{
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

void KVcache::removeNode(Node *node)
{
    Node *x = node->prev;
    Node *y = node->next;

    x->next = y;
    y->prev = x;
}

void KVcache::clearExpired()
{
    if (pq.empty())
        return;
    int now = time(NULL);
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
        json j = json::object();
        int now = time(nullptr);
        for (auto [key, node] : cache)
        {
            j[key]["data"] = node->data;
            j[key]["expiry"] = node->expiry;
        }

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

        for (auto [key, value] : j.items())
        {
            if (size > capacity)
                break;
            try
            {
                if (cache.find(key) != cache.end())
                {
                    continue;
                }
                size += key.size() + value["data"].dump().size();
                Node *node = new Node(key, value["data"], value["expiry"]);
                insertAfterStart(node);
                cache[key] = node;
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