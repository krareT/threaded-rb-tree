

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
    trb_map<std::string, size_t> map1;
    trb_multimap<std::string, size_t> map2;
    
    set1.emplace(1);
    set1.emplace(1);
    set2.emplace(2);
    set1.emplace(3);
    set2.emplace(4);
    set1.emplace(5);
    set2.emplace(6);
    map1.emplace("123", 456);
    map1.emplace("123", 456);
    map2.emplace("456", 789);
    map1.emplace(std::make_pair("012", 345));
    map2.emplace(std::make_pair("345", 678));
    
    for(auto v : set1)
    {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    for(auto v : set2)
    {
        std::cout << v << " ";
    }
    std::cout << std::endl;
    for(auto v : map1)
    {
        std::cout << v.first << " " << v.second << " ";
    }
    std::cout << std::endl;
    for(auto v : map2)
    {
        std::cout << v.first << " " << v.second << " ";
    }
    std::cout << std::endl;
    
    set1.clear();
    set2.clear();
    map1.clear();
    map2.clear();
    
    return 0;
}






















