

#include <algorithm>
#include <functional>
#include <random>
#include <iostream>
#include <set>
#include "threaded_rb_tree.h"



int main(int argc, const char * argv[])
{
    trb_set<size_t> set1;
    trb_multiset<size_t> set2;
    
    set1.emplace(1);
    set2.emplace(2);
    set1.emplace(3);
    set2.emplace(4);
    set1.emplace(5);
    set2.emplace(6);
    
    for(auto v : set1)
    {
        std::cout << v << " ";
    }
    for(auto v : set2)
    {
        std::cout << v << " ";
    }
    
    std::cout << std::endl;
    
    return 0;
}






















