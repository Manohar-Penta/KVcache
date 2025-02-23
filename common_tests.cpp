/*
    This file is to test the functionality of the key-value store.

    It contains the following tests :
    1. create
    2. get
        1. get key
        2. get non existent key
    3. delete
        1. delete key
        2. delete non existent key
    4. Invalid key | value
    4. create(TTL)
    5. batch create
*/
#include "json.hpp"
#include <iostream>
#include "kvcache.hpp"
#include <unistd.h>
#include <string>
#include <ctime>

using std::endl, std::cout, std::string, std::cerr;

void getKeyTests(string key, string value, KVcache &kv);
void deleteKeyTests(string key, KVcache &kv);
void invalidPutTest(KVcache &kv);
void TTLTests(string key, string value, KVcache &kv);
void batchCreateTests(KVcache &kv);

int main(int argc, char *argv[])
{
    KVcache kv("data-store-" + std::to_string(time(nullptr)) + ".json"); // can also be initiated with a file name
    cout << "----------------create--------------------" << endl;

    // data
    string key = "profile";
    string value = R"({"age":21,"graduation":"2024","institute":"IIT Roorkee","name":"Manohar Penta"})";

    // inserting the data
    cout << "key : " << key << "\nvalue : " << value << endl;
    kv.putKey(key, value, 3);

    // testing the data

    getKeyTests(key, value, kv);

    deleteKeyTests(key, kv);

    invalidPutTest(kv);

    TTLTests(key, value, kv);

    batchCreateTests(kv);

    return 0;
}

void getKeyTests(string key, string value, KVcache &kv)
{
    cout << "----------------get-------------------" << endl;

    json j = kv.getKey(key);

    if (j.dump() != value)
    {
        throw "\033[31mCreate & Get test failed.\033[0m";
    }

    cout << "\033[32mCreate & Get test passed\033[0m" << endl;

    // fetching the non existent key
    j = kv.getKey("non-existent-key");

    if (j.dump() != "{}")
    {
        throw "\033[31mGet non existent key test failed.\033[0m";
    }

    cout << "\033[32mGet non existent key test passed.\033[0m" << endl;
}

void deleteKeyTests(string key, KVcache &kv)
{
    cout << "----------------delete-------------------" << endl;

    // deleting the key
    kv.deleteKey(key);

    // fetching the key
    json j = kv.getKey(key);

    if (j.dump() != "{}")
    {
        throw "\033[31mDelete test failed.\033[0m";
    }

    cout << "\033[32mDelete test passed\033[0m" << endl;

    // deleting the non existent key
    kv.deleteKey("some random key", [](std::vector<Error_obj> err)
                 {
        if(err.size()!=1 || err[0].code!=Error_code::KEY_NOT_FOUND){
            throw "\033[31mDelete non existent key test failed.\033[0m";
        } });

    cout << "\033[32mDelete non existent key test passed.\033[0m" << endl;
}

void invalidPutTest(KVcache &kv)
{
    cout << "----------------invalid put-------------------" << endl;

    // inserting the data
    kv.putKey("Too long key Too long key Too long key", "value", 3, [](std::vector<Error_obj> err)
              {
        if(err.size()!=1 || err[0].code!=Error_code::KEY_TOO_LONG){
            throw "\033[31mCreating invalid key test failed.\033[0m";
        } });

    cout << "\033[32mCreating invalid key test passed.\033[0m" << endl;

    kv.putKey("key", "{invalid json", -1, [](std::vector<Error_obj> err)
              {
        if(err.size()!=1 || err[0].code!=Error_code::UNKNOWN_ERROR){
            throw "\033[31mCreating invalid json test failed.\033[0m";
        } });

    cout << "\033[32mCreating invalid json test passed.\033[0m" << endl;
}

void TTLTests(string key, string value, KVcache &kv)
{
    // Tests for TTL
    cout << "----------------create(TTL)-------------------" << endl;

    // inserting the data
    kv.putKey(key, value, 3);

    // fetching the key
    json j = kv.getKey(key);
    cout << "Created key : " << key << " with a 3 seconds TTL" << endl;

    cout << "Waiting for 3 seconds : ";

    for (int i = 1; i <= 3; i++)
    {
        cout << i << " " << std::flush;
        sleep(1);
    }

    // fetching the key after 3 seconds
    j = kv.getKey(key);

    if (j.dump() != "{}")
    {
        throw "\n\033[31mTTL test failed.\033[0m";
    }

    cout << "\n\033[32mTTL test passed.\033[0m" << endl;
}

void batchCreateTests(KVcache &kv)
{
    // Tests for batch create
    cout << "----------------batch create-------------------" << endl;
    int n = 8;
    cout << "Creating " << n << " keys" << endl;

    // making a batch of data
    KVE val[n];
    string batchData = R"({"batchCreate" : "Lorem Ipsum"})";
    for (int i = 0; i < n; i++)
    {
        val[i].key = "key" + std::to_string(i);
        val[i].data = json::parse(batchData);
        val[i].expiry = i % 2 ? 3 : -1;
    }

    // creating the keys
    kv.batchCreate(n, val, [](std::vector<Error_obj> err)
                   {
        // checking for errors
        if(err.size()!=0){
            throw "\033[31mBatch create test failed.\033[0m";
        } });
    cout << "------------------------------------------------" << endl;
    cout << "Retrieving the data from the created keys" << endl;
    json j;
    for (int i = 0; i < n; i++)
    {
        j = kv.getKey(val[i].key);
        if (j != R"({"batchCreate" : "Lorem Ipsum"})"_json)
        {
            throw "\033[31mBatch create test failed.\033[0m";
        }
    }

    cout << "\033[32mBatch create test passed.\033[0m" << endl;
}