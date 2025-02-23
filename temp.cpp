#include <iostream>
#include "kvcache.hpp"

using std::endl, std::cout;

enum Example
{
    A,
    B,
    C
};

int main()
{

    Example a = A;
    int A = 1000;

    KVcache k, l;

    cout << k.getKey("randow") << endl;
    cout << l.getKey("randow") << endl;

    cout << A << Example::B << endl;
}