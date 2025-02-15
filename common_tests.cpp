/*
    This file is to test the functionality of the key-value store.

    It contains the following tests :
    1. create
    2. get
    3. delete
    4. create(TTL)
    5. batch create`
*/
#include "json.hpp"
#include <iostream>
#include "kvcache.hpp"
#include <unistd.h>
#include <string>

using std::endl, std::cout, std::string;

int main(int argc, char *argv[])
{
    KVcache kv; // can also be initiated with a file name
    cout << "----------------create--------------------" << endl;

    // data
    string key = "profile";
    string value = R"({"age":21,"graduation":"2024","institute":"IIT Roorkee","name":"Manohar Penta"})";

    // inserting the data
    cout << "key : " << key << "\nvalue : " << value << endl;
    kv.putKey(key, value, 3);

    cout << "----------------get-------------------" << endl;

    json j = kv.getKey(key);
    cout << j << endl;

    cout << "----------------delete-------------------" << endl;

    // deleting the key
    kv.deleteKey(key);

    // fetching the key
    j = kv.getKey(key);
    cout << j << endl;

    cout << "----------------create(TTL)-------------------" << endl;

    // inserting the data
    kv.putKey(key, value, 3);

    // fetching the key
    j = kv.getKey(key);
    cout << j << endl;

    cout << "Waiting for 3 seconds" << endl;

    for (int i = 1; i <= 3; i++)
    {
        cout << i << endl;
        sleep(1);
    }

    cout << "fetching the data after expiry : " << endl;

    j = kv.getKey(key);
    cout << j << endl;

    cout << "----------------batch create-------------------" << endl;
    int n = 8;

    cout << "Creating " << n << " keys" << endl;

    KVE val[n];
    for (int i = 0; i < n; i++)
    {
        val[i].key = "key" + std::to_string(i);
        val[i].data = R"({"batchCreate" : "Lorem Ipsum"})"_json;
        val[i].expiry = i % 2 ? 3 : -1;
    }

    kv.batchCreate(n, val);

    cout << "retrieving the data from the created keys" << endl;

    for (int i = 0; i < n; i++)
    {
        j = kv.getKey(val[i].key);
        cout << j << endl;
    }

    return 0;
}
