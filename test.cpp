

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
    typedef threaded_rb_tree_node_t<uint32_t> onode_t;
    typedef threaded_rb_tree_ptr_t<size_t> pnode_t;
    typedef threaded_rb_tree_root_t<onode_t, std::false_type, std::false_type> oroot_t;
    typedef threaded_rb_tree_root_t<pnode_t, std::false_type, std::false_type> proot_t;
    
    size_t constexpr len = 10000;
    onode_t *oarr = new onode_t[len];
    pnode_t *parr = new pnode_t[len];
    size_t *data = new size_t[len];
    
    std::memset(data, 0, sizeof(size_t) * len);
    
    std::mt19937 mt;
    std::uniform_int_distribution<double> uni(0, 100000000);
    for(size_t i = 0; i < len; ++i)
    {
        parr[i].value = data[i] = uni(mt);
    }
    
    std::multiset<size_t> test;
    oroot_t oroot;
    proot_t proot;
    auto ocomp = [data](uint32_t l, uint32_t r){
        if(data[l] != data[r])
        {
            return data[l] < data[r];
        }
        else
        {
            return l < r;
        }
    };
    auto pcomp = [](pnode_t *l, pnode_t *r){
        if(l->value != r->value)
        {
            return l->value < r->value;
        }
        else
        {
            return l < r;
        }
    };
    auto oderef = [oarr](uint32_t i)->onode_t &{
        return oarr[i];
    };
    auto pderef = [](pnode_t *p)->pnode_t &{
        return *p;
    };
    
    for(size_t i = 0; i < len; ++i)
    {
        threaded_rb_tree_stack_t<onode_t, 40> ostack;
        threaded_rb_tree_find_path_for_insert(oroot, ostack, oderef, uint32_t(i), ocomp);
        threaded_rb_tree_insert(oroot, ostack, oderef, uint32_t(i));
        
        threaded_rb_tree_stack_t<pnode_t, 40> pstack;
        threaded_rb_tree_find_path_for_insert(proot, pstack, pderef, parr + i, pcomp);
        threaded_rb_tree_insert(proot, pstack, pderef, parr + i);
        
        test.emplace(data[i]);
        
        uint32_t obegin = oroot.get_most_left(oderef);
        uint32_t oend = onode_t::nil_sentinel;
        pnode_t *pbegin = proot.get_most_left(pderef);
        pnode_t *pend = pnode_t::nil_sentinel;
        for(auto it = test.begin(); it != test.end(); obegin = threaded_rb_tree_move_next(obegin, oderef), pbegin = threaded_rb_tree_move_next(pbegin, pderef), ++it)
        {
            assert(data[obegin] == *it);
            assert(pderef(pbegin).value == *it);
        }
        assert(obegin == oend);
        assert(pbegin == pend);

        uint32_t orbegin = oroot.get_most_right(oderef);
        uint32_t orend = onode_t::nil_sentinel;
        pnode_t *prbegin = proot.get_most_right(pderef);
        pnode_t *prend = pnode_t::nil_sentinel;
        for(auto rit = test.rbegin(); rit != test.rend(); orbegin = threaded_rb_tree_move_prev(orbegin, oderef), prbegin = threaded_rb_tree_move_prev(prbegin, pderef), ++rit)
        {
            assert(data[orbegin] == *rit);
            assert(pderef(prbegin).value == *rit);
        }
        assert(orbegin == orend);
        assert(prbegin == prend);
    }
    
    for(size_t i = 0; i < len; ++i)
    {
        bool find;
        
        threaded_rb_tree_stack_t<onode_t, 40> ostack;
        find = threaded_rb_tree_find_path_for_remove(oroot, ostack, oderef, uint32_t(i), ocomp);
        assert(find);
        threaded_rb_tree_remove(oroot, ostack, oderef);
        
        threaded_rb_tree_stack_t<pnode_t, 40> pstack;
        find = threaded_rb_tree_find_path_for_remove(proot, pstack, pderef, parr + i, pcomp);
        assert(find);
        threaded_rb_tree_remove(proot, pstack, pderef);
        
        test.erase(test.find(data[i]));
        
        uint32_t obegin = oroot.get_most_left(oderef);
        uint32_t oend = onode_t::nil_sentinel;
        pnode_t *pbegin = proot.get_most_left(pderef);
        pnode_t *pend = pnode_t::nil_sentinel;
        for(auto it = test.begin(); it != test.end(); obegin = threaded_rb_tree_move_next(obegin, oderef), pbegin = threaded_rb_tree_move_next(pbegin, pderef), ++it)
        {
            assert(data[obegin] == *it);
            assert(pderef(pbegin).value == *it);
        }
        assert(obegin == oend);
        assert(pbegin == pend);
        
        uint32_t orbegin = oroot.get_most_right(oderef);
        uint32_t orend = onode_t::nil_sentinel;
        pnode_t *prbegin = proot.get_most_right(pderef);
        pnode_t *prend = pnode_t::nil_sentinel;
        for(auto rit = test.rbegin(); rit != test.rend(); orbegin = threaded_rb_tree_move_prev(orbegin, oderef), prbegin = threaded_rb_tree_move_prev(prbegin, pderef), ++rit)
        {
            assert(data[orbegin] == *rit);
            assert(pderef(prbegin).value == *rit);
        }
        assert(orbegin == orend);
        assert(prbegin == prend);
    }
    delete[] data;
    delete[] parr;
    delete[] oarr;
    
    return 0;
}






















