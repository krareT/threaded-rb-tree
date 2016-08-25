

#include <algorithm>
#include <functional>
#include "threaded_rb_tree.h"



template<class key_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using thread_rb_tree = threaded_rb_tree_impl<threaded_rb_tree_collection<key_t, comparator_t, index_t, threaded_rb_tree_node_t<index_t>>, sizeof(index_t) * 10>;


int main(int argc, const char * argv[])
{
    thread_rb_tree<int, std::less<uint64_t>, uint16_t> tree0(nullptr, nullptr);
    thread_rb_tree<uint32_t> tree1(nullptr, nullptr);
    thread_rb_tree<uint64_t, std::greater<uint64_t>, uint64_t> tree2(nullptr, nullptr);
    
    
    
    return 0;
}






















