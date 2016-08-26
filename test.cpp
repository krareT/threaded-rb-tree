

#include <algorithm>
#include <functional>
#include <random>
#include <iostream>
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
    
    size_t constexpr len = 100;
    node_t arr[len];
    size_t data[len];
    
    std::mt19937 mt;
    std::uniform_int_distribution<double> uni(0, 1000);
    for(size_t i = 0; i < len; ++i)
    {
        data[i] = uni(mt);
    }
    
    root_t root;
    
    for(size_t i = 0; i < len; ++i)
    {
        threaded_rb_tree_stack_t<node_t, 40> stack(root);
        threaded_rb_tree_find_path(stack, arr, uint32_t(i), [data](uint32_t l, uint32_t r){
            if(data[l] != data[r])
            {
                return data[l] < data[r];
            }
            else
            {
                return l < r;
            }
        });
        threaded_rb_tree_insert(stack, arr, len, uint32_t(i));
        
        uint32_t begin = root.left;
        uint32_t end = node_t::nil_sentinel;
        
        for(; begin != end; begin = threaded_rb_tree_move_next(begin, arr))
        {
            std::cout << data[begin] << " ";
        }
        std::cout << std::endl;
        
        uint32_t rbegin = root.right;
        uint32_t rend = node_t::nil_sentinel;
        
        
        for(; rbegin != rend; rbegin = threaded_rb_tree_move_prev(rbegin, arr))
        {
            std::cout << data[rbegin] << " ";
        }
        
        std::cout << std::endl;
    }
    
    return 0;
}






















