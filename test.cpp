

#include <algorithm>
#include <functional>
#include <random>
#include <iostream>
#include <set>
#include "threaded_rb_tree.h"



template<class key_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using thread_rb_tree = threaded_rb_tree_impl<threaded_rb_tree_collection<key_t, comparator_t, index_t, threaded_rb_tree_node_t<index_t>>, sizeof(index_t) * 10>;


int main(int argc, const char * argv[])
{
    thread_rb_tree<int, std::less<uint64_t>, uint16_t> tree0(nullptr, nullptr);
    thread_rb_tree<uint32_t> tree1(nullptr, nullptr);
    thread_rb_tree<uint64_t, std::greater<uint64_t>, uint64_t> tree2(nullptr, nullptr);
    
    typedef threaded_rb_tree_node_t<uint32_t> node_t;
    typedef threaded_rb_tree_root_t<node_t> root_t;
    
    size_t constexpr len = 10000;
    node_t *arr = new node_t[len];
    size_t *data = new size_t[len];
    
    std::memset(arr, 0, sizeof(node_t) * len);
    std::memset(data, 0, sizeof(size_t) * len);
    
    std::mt19937 mt;
    std::uniform_int_distribution<double> uni(0, 100000000);
    for(size_t i = 0; i < len; ++i)
    {
        data[i] = uni(mt);
    }
    
    std::set<size_t> test;
    root_t root;
    auto comp = [data](uint32_t l, uint32_t r){
        if(data[l] != data[r])
        {
            return data[l] < data[r];
        }
        else
        {
            return l < r;
        }
    };
    auto deref = [arr](uint32_t i)->node_t &{
        return arr[i];
    };
    
    for(size_t i = 0; i < len; ++i)
    {
        threaded_rb_tree_stack_t<node_t, 40> stack(root);
        threaded_rb_tree_find_path_for_insert(stack, deref, uint32_t(i), comp);
        threaded_rb_tree_insert(stack, deref, uint32_t(i));
        test.emplace(data[i]);
        
        uint32_t begin = root.left;
        uint32_t end = node_t::nil_sentinel;
        auto it = test.begin();
        for(; begin != end; begin = threaded_rb_tree_move_next(begin, deref), ++it)
        {
            assert(data[begin] == *it);
        }

        uint32_t rbegin = root.right;
        uint32_t rend = node_t::nil_sentinel;
        auto rit = test.rbegin();
        for(; rbegin != rend; rbegin = threaded_rb_tree_move_prev(rbegin, deref), ++rit)
        {
            assert(data[rbegin] == *rit);
        }
    }
    
    for(size_t i = 0; i < len; ++i)
    {
        threaded_rb_tree_stack_t<node_t, 40> stack(root);
        bool find = threaded_rb_tree_find_path_for_remove(stack, deref, uint32_t(i), comp);
        assert(find);
        threaded_rb_tree_remove(stack, deref);
        test.erase(test.find(data[i]));
        
        uint32_t begin = root.left;
        uint32_t end = node_t::nil_sentinel;
        auto it = test.begin();
        for(; begin != end; begin = threaded_rb_tree_move_next(begin, deref), ++it)
        {
            assert(data[begin] == *it);
        }
        
        uint32_t rbegin = root.right;
        uint32_t rend = node_t::nil_sentinel;
        auto rit = test.rbegin();
        for(; rbegin != rend; rbegin = threaded_rb_tree_move_prev(rbegin, deref), ++rit)
        {
            assert(data[rbegin] == *rit);
        }
    }
    delete[] arr;
    delete[] data;
    
    return 0;
}






















