#include "json.hpp"
#include <iostream>
#include "kvcache.hpp"
#include <unistd.h>
#include <string>

using std::endl, std::cout, std::string;

int main(int argc, char *argv[])
{

    KVcache kv;
    cout << "----------------test--------------------" << endl;
    cout << "Create the data with expiry time of 5 secs : " << endl;
    string key = "profile";

    string value = R"({
        "name" : "Wolverine",
        "age" : 21,
        "graduation" : "1956",
        "institute" : "X-men"
        })";

    cout << "key : " << key << "\nvalue : " << value << endl;
    kv.putKey(key, value, 5);

    cout << "waiting for three seconds : ";

    for (int i = 1; i <= 3; i++)
    {
        sleep(1);
        cout << i << " ";
    }

    cout << "\n fetching the data prev set data :\n";

    json j = kv.getKey(key);

    cout << j << endl;

    cout << "waiting for two more seconds : ";

    for (int i = 4; i <= 5; i++)
    {
        // sleep(1);
        cout << i << " ";
    }

    cout << "\n fetching the data after expiry : " << endl;

    j = kv.getKey(key);

    cout << j << endl;
    return 0;
}
