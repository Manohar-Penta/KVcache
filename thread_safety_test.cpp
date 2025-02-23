/*  this is test to check thread safety of the Key-Value store.
    In this test, two threads write 50 keys each concurrently to the store.
    After their are done writing, we will check if the keys are correctly written
    without any errors
*/

#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <thread>
#include "kvcache.hpp"
#include <filesystem>

using std::cout, std::endl;

void createKeys(KVcache &kv, int i)
{
    json k;
    for (int j = 1; j <= 50; ++j)
    {
        std::string key = "key_" + std::to_string(50 * i + j);
        k["ivalue"] = i;
        k["jvalue"] = j;

        kv.putKey(key, k.dump());
    }
}

int main()
{
    KVcache kv("thread-store" + std::to_string(time(nullptr)) + ".json");
    cout << "----------------------------------------" << endl;
    cout << "Starting the threads" << endl;

    std::thread t1(createKeys, std::ref(kv), 0);
    std::thread t2(createKeys, std::ref(kv), 1);

    t1.join();
    t2.join();

    json k;
    for (int i = 1; i <= 100; i++)
    {
        k = kv.getKey("key_" + std::to_string(i));
        int x = i > 50;
        if (k["ivalue"] != x || k["jvalue"] != i - x * 50)
        {
            throw "\033[31mThere is an error : \033[0m key_" + i;
        }
    }
    cout << "\033[32mAll keys from the threads are written correctly!!\033[0m" << endl;
    // getchar(); // you can use uncomment this line to pause the program, this can be used to check concurrent access of the system
    return 0;
}