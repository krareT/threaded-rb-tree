
#include <stdexcept>
#include <cstdint>
#include <cassert>
#include <new>

template<class index_t>
struct threaded_rb_tree_node_t
{
    typedef index_t index_type;
    
    index_type children[2];
    
    static index_type constexpr flag_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 1);
    static index_type constexpr type_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 2);
    static index_type constexpr full_bit_mask = flag_bit_mask | type_bit_mask;
    
    static index_type constexpr nil_sentinel = ~index_type(0) & ~full_bit_mask;
    static index_type constexpr free_sentinel = nil_sentinel - 1;
    
    bool left_is_child() const
    {
        return (children[0] & type_bit_mask) == 0;
    }
    bool right_is_child() const
    {
        return (children[1] & type_bit_mask) == 0;
    }
    bool left_is_thread() const
    {
        return (children[0] & type_bit_mask) != 0;
    }
    bool right_is_thread() const
    {
        return (children[1] & type_bit_mask) != 0;
    }
    
    void left_set_child()
    {
        children[0] &= ~type_bit_mask;
    }
    void right_set_child()
    {
        children[1] &= ~type_bit_mask;
    }
    void left_set_thread()
    {
        children[0] |= type_bit_mask;
    }
    void right_set_thread()
    {
        children[1] |= type_bit_mask;
    }
    
    void left_set_link(index_type link)
    {
        children[0] = (children[0] & full_bit_mask) | link;
    }
    void right_set_link(index_type link)
    {
        children[1] = (children[1] & full_bit_mask) | link;
    }
    index_type left_get_link() const
    {
        return children[0] & ~full_bit_mask;
    }
    index_type right_get_link() const
    {
        return children[1] & ~full_bit_mask;
    }
    
    bool is_black() const
    {
        return (children[0] & flag_bit_mask) == 0;
    }
    void set_black()
    {
        children[0] &= ~flag_bit_mask;
    }
    bool is_red() const
    {
        return (children[0] & flag_bit_mask) != 0;
    }
    void set_red()
    {
        children[0] |= flag_bit_mask;
    }
};

template<class index_t, class dereference_t>
index_t threaded_rb_tree_move_next(index_t node, dereference_t const &deref)
{
    if(deref(node).right_is_thread())
    {
        return deref(node).right_get_link();
    }
    else
    {
        node = deref(node).right_get_link();
        while(deref(node).left_is_child())
        {
            node = deref(node).left_get_link();
        }
        return node;
    }
}

template<class index_t, class dereference_t>
index_t threaded_rb_tree_move_prev(index_t node, dereference_t const &deref)
{
    if(deref(node).left_is_thread())
    {
        return deref(node).left_get_link();
    }
    else
    {
        node = deref(node).left_get_link();
        while(deref(node).right_is_child())
        {
            node = deref(node).right_get_link();
        }
        return node;
    }
}


template<class node_t, class enable_count_t>
struct threaded_rb_tree_count_t
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    index_type count;
    
    threaded_rb_tree_count_t() : count(0)
    {
    }
    
    void increase_count()
    {
        ++count;
    }
    void decrease_count()
    {
        --count;
    }
    index_type get_count() const
    {
        return count;
    }
};
template<class node_t>
struct threaded_rb_tree_count_t<node_t, std::false_type>
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    void increase_count()
    {
    }
    void decrease_count()
    {
    }
    index_type get_count() const
    {
        return 0;
    }
};

template<class node_t, class enable_most_t>
struct threaded_rb_tree_most_t
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    index_type left, right;
    
    threaded_rb_tree_most_t() : left(node_type::nil_sentinel), right(node_type::nil_sentinel)
    {
    }
    
    template<class dereference_t> index_type get_left(index_type , dereference_t const &) const
    {
        return left;
    }
    template<class dereference_t> index_type get_right(index_type , dereference_t const &) const
    {
        return right;
    }
    void set_left(index_type value)
    {
        left = value;
    }
    void set_right(index_type value)
    {
        right = value;
    }
    void update_left(index_type check, index_type value)
    {
        if(check == left)
        {
            left = value;
        }
    }
    void update_right(index_type check, index_type value)
    {
        if(check == right)
        {
            right = value;
        }
    }
    template<class dereference_t> void detach_left(index_type value, dereference_t const &deref)
    {
        if(value == left)
        {
            left = threaded_rb_tree_move_next(left, deref);
        }
    }
    template<class dereference_t> void detach_right(index_type value, dereference_t const &deref)
    {
        if(value == right)
        {
            right = threaded_rb_tree_move_prev(right, deref);
        }
    }
};
template<class node_t>
struct threaded_rb_tree_most_t<node_t, std::false_type>
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    template<class dereference_t> index_type get_left(index_type root, dereference_t const &deref) const
    {
        index_type node = root;
        if(root != node_type::nil_sentinel)
        {
            while(deref(node).left_is_child())
            {
                node = deref(node).left_get_link();
            }
        }
        return node;
    }
    template<class dereference_t> index_type get_right(index_type root, dereference_t const &deref) const
    {
        index_type node = root;
        if(root != node_type::nil_sentinel)
        {
            while(deref(node).right_is_child())
            {
                node = deref(node).right_get_link();
            }
        }
        return node;
    }
    void set_left(index_type )
    {
    }
    void set_right(index_type )
    {
    }
    void update_left(index_type , index_type )
    {
    }
    void update_right(index_type , index_type )
    {
    }
    template<class dereference_t> void detach_left(index_type , dereference_t const &)
    {
    }
    template<class dereference_t> void detach_right(index_type , dereference_t const &)
    {
    }
};

template<class node_t, class enable_count_t, class enable_most_t>
struct threaded_rb_tree_root_t
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    threaded_rb_tree_root_t()
    {
        root.root = node_type::nil_sentinel;
    }
    
    struct root_type : public threaded_rb_tree_count_t<node_t, enable_count_t>, public threaded_rb_tree_most_t<node_t, enable_most_t>
    {
        index_type root;
    } root;
    
    index_type get_count() const
    {
        return root.get_count();
    }
    template<class dereference_t> index_type get_most_left(dereference_t const &deref) const
    {
        return root.get_left(root.root, deref);
    }
    template<class dereference_t> index_type get_most_right(dereference_t const &deref) const
    {
        return root.get_right(root.root, deref);
    }
};

template<class node_t, size_t max_depth>
struct threaded_rb_tree_stack_t
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    typedef decltype(node_type::flag_bit_mask) mask_type;
    
    static mask_type constexpr dir_bit_mask = node_type::flag_bit_mask;
    
    threaded_rb_tree_stack_t()
    {
        height = 0;
    }
    
    size_t height;
    index_type stack[max_depth];
    
    bool is_left(size_t i) const
    {
        assert(i < max_depth);
        return (mask_type(stack[i]) & dir_bit_mask) == 0;
    }
    bool is_right(size_t i) const
    {
        assert(i < max_depth);
        return (mask_type(stack[i]) & dir_bit_mask) != 0;
    }
    index_type get_index(size_t i) const
    {
        assert(i < max_depth);
        return index_type(mask_type(stack[i]) & ~dir_bit_mask);
    }
    void push_index(index_type index, bool left)
    {
        if(height == max_depth)
        {
            throw std::length_error("thread_tb_tree_stack overflow");
        }
        stack[height++] = index_type(mask_type(index) | (left ? 0 : dir_bit_mask));
    }
    void update_index(size_t k, index_type index, bool left)
    {
        assert(k < max_depth);
        stack[k] = index_type(mask_type(index) | (left ? 0 : dir_bit_mask));
    }
    void update_index(size_t k, index_type index)
    {
        assert(k < max_depth);
        stack[k] = index_type(mask_type(index) | (mask_type(stack[k]) & dir_bit_mask));
    }
};

template<class root_t, class comparator_t, class deref_node_t, size_t max_depth>
void threaded_rb_tree_find_path_for_multi(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, deref_node_t const &deref, typename root_t::index_type index, comparator_t const &comparator)
{
    typedef typename root_t::node_type node_type;
    typedef typename root_t::index_type index_type;
    
    index_type p = root.root.root;
    while(p != node_type::nil_sentinel)
    {
        bool is_left = comparator(index, p);
        stack.push_index(p, is_left);
        if(is_left)
        {
            if(deref(p).left_is_thread())
            {
                return;
            }
            p = deref(p).left_get_link();
        }
        else
        {
            if(deref(p).right_is_thread())
            {
                return;
            }
            p = deref(p).right_get_link();
        }
    }
}

template<class root_t, class comparator_t, class key_t, class get_key_t, class deref_node_t, class deref_value_t, size_t max_depth>
bool threaded_rb_tree_find_path_for_unique(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, deref_node_t const &deref, key_t const &key, deref_value_t const &deref_value, get_key_t const &get_key, comparator_t const &comparator)
{
    typedef typename root_t::node_type node_type;
    typedef typename root_t::index_type index_type;
    
    index_type p = root.root.root;
    while(p != node_type::nil_sentinel)
    {
        bool is_left = comparator(key, get_key(deref_value(p)));
        stack.push_index(p, is_left);
        if(is_left)
        {
            if(deref(p).left_is_thread())
            {
                return false;
            }
            p = deref(p).left_get_link();
        }
        else
        {
            if(!comparator(get_key(deref_value(p)), key))
            {
                return true;
            }
            if(deref(p).right_is_thread())
            {
                return false;
            }
            p = deref(p).right_get_link();
        }
    }
    return false;
}

template<class root_t, class comparator_t, class deref_node_t, size_t max_depth>
bool threaded_rb_tree_find_path_for_remove(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, deref_node_t const &deref, typename root_t::index_type index, comparator_t const &comparator)
{
    typedef typename root_t::node_type node_type;
    typedef typename root_t::index_type index_type;
    
    index_type p = root.root.root;
    while(p != node_type::nil_sentinel)
    {
        bool is_left = comparator(index, p);
        stack.push_index(p, is_left);
        if(is_left)
        {
            if(deref(p).left_is_thread())
            {
                return false;
            }
            p = deref(p).left_get_link();
        }
        else
        {
            if(!comparator(p, index))
            {
                return true;
            }
            if(deref(p).right_is_thread())
            {
                return false;
            }
            p = deref(p).right_get_link();
        }
    }
    return false;
}


template<class root_t, class dereference_t, size_t max_depth>
void threaded_rb_tree_insert(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, dereference_t const &deref, typename root_t::index_type index)
{
    typedef typename root_t::node_type node_type;
    typedef typename root_t::index_type index_type;
    
    root.root.increase_count();
    deref(index).left_set_thread();
    deref(index).right_set_thread();
    
    if(stack.height == 0)
    {
        deref(index).left_set_link(index_type(node_type::nil_sentinel));
        deref(index).right_set_link(index_type(node_type::nil_sentinel));
        deref(index).set_black();
        root.root.root = index;
        root.root.set_left(index);
        root.root.set_right(index);
        return;
    }
    deref(index).set_red();
    size_t k = stack.height - 1;
    index_type where = stack.get_index(k);
    if(stack.is_left(k))
    {
        deref(index).left_set_link(deref(where).left_get_link());
        deref(index).right_set_link(where);
        deref(where).left_set_child();
        deref(where).left_set_link(index);
        root.root.update_left(where, index);
    }
    else
    {
        deref(index).right_set_link(deref(where).right_get_link());
        deref(index).left_set_link(where);
        deref(where).right_set_child();
        deref(where).right_set_link(index);
        root.root.update_right(where, index);
    }
    if(k >= 1)
    {
        while(deref(stack.get_index(k)).is_red())
        {
            index_type p2 = stack.get_index(k - 1);
            index_type p1 = stack.get_index(k);
            if(stack.is_left(k - 1))
            {
                index_type u = deref(p2).right_get_link();
                if(deref(p2).right_is_child() && deref(u).is_red())
                {
                    deref(p1).set_black();
                    deref(u).set_black();
                    deref(p2).set_red();
                    if(k < 2)
                    {
                        break;
                    }
                    k -= 2;
                }
                else
                {
                    index_type y;
                    if(stack.is_left(k))
                    {
                        y = p1;
                    }
                    else
                    {
                        y = deref(p1).right_get_link();
                        deref(p1).right_set_link(deref(y).left_get_link());
                        deref(y).left_set_link(p1);
                        deref(p2).left_set_link(y);
                        if(deref(y).left_is_thread())
                        {
                            deref(y).left_set_child();
                            deref(p1).right_set_thread();
                            deref(p1).right_set_link(y);
                        }
                    }
                    deref(p2).set_red();
                    deref(y).set_black();
                    deref(p2).left_set_link(deref(y).right_get_link());
                    deref(y).right_set_link(p2);
                    if(k == 1)
                    {
                        root.root.root = y;
                    }
                    else if(stack.is_left(k - 2))
                    {
                        deref(stack.get_index(k - 2)).left_set_link(y);
                    }
                    else
                    {
                        deref(stack.get_index(k - 2)).right_set_link(y);
                    }
                    if(deref(y).right_is_thread())
                    {
                        deref(y).right_set_child();
                        deref(p2).left_set_thread();
                        deref(p2).left_set_link(y);
                    }
                    assert(deref(p2).right_get_link() == u);
                    break;
                }
            }
            else
            {
                index_type u = deref(p2).left_get_link();
                if(deref(p2).left_is_child() && deref(u).is_red())
                {
                    deref(p1).set_black();
                    deref(u).set_black();
                    deref(p2).set_red();
                    if(k < 2)
                    {
                        break;
                    }
                    k -= 2;
                }
                else
                {
                    index_type y;
                    if(stack.is_right(k))
                    {
                        y = p1;
                    }
                    else
                    {
                        y = deref(p1).left_get_link();
                        deref(p1).left_set_link(deref(y).right_get_link());
                        deref(y).right_set_link(p1);
                        deref(p2).right_set_link(y);
                        if(deref(y).right_is_thread())
                        {
                            deref(y).right_set_child();
                            deref(p1).left_set_thread();
                            deref(p1).left_set_link(y);
                        }
                    }
                    deref(p2).set_red();
                    deref(y).set_black();
                    deref(p2).right_set_link(deref(y).left_get_link());
                    deref(y).left_set_link(p2);
                    if(k == 1)
                    {
                        root.root.root = y;
                    }
                    else if(stack.is_right(k - 2))
                    {
                        deref(stack.get_index(k - 2)).right_set_link(y);
                    }
                    else
                    {
                        deref(stack.get_index(k - 2)).left_set_link(y);
                    }
                    if(deref(y).left_is_thread())
                    {
                        deref(y).left_set_child();
                        deref(p2).right_set_thread();
                        deref(p2).right_set_link(y);
                    }
                    assert(deref(p2).left_get_link() == u);
                    break;
                }
            }
        }
    }
    deref(root.root.root).set_black();
}

template<class root_t, class dereference_t, size_t max_depth>
void threaded_rb_tree_remove(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, dereference_t const &deref)
{
    typedef typename root_t::node_type node_type;
    typedef typename root_t::index_type index_type;
    
    assert(stack.height > 0);
    root.root.decrease_count();
    size_t k = stack.height - 1;
    index_type p = stack.get_index(k);
    root.root.detach_left(p, deref);
    root.root.detach_right(p, deref);
    if(deref(p).right_is_thread())
    {
        if(deref(p).left_is_child())
        {
            index_type t = deref(p).left_get_link();
            while(deref(t).right_is_child())
            {
                t = deref(t).right_get_link();
            }
            deref(t).right_set_link(deref(p).right_get_link());
            if(k == 0)
            {
                root.root.root = deref(p).left_get_link();
            }
            else if(stack.is_left(k - 1))
            {
                deref(stack.get_index(k - 1)).left_set_link(deref(p).left_get_link());
            }
            else
            {
                deref(stack.get_index(k - 1)).right_set_link(deref(p).left_get_link());
            }
        }
        else
        {
            if(k == 0)
            {
                root.root.root = deref(p).left_get_link();
            }
            else if(stack.is_left(k - 1))
            {
                deref(stack.get_index(k - 1)).left_set_link(deref(p).left_get_link());
                deref(stack.get_index(k - 1)).left_set_thread();
            }
            else
            {
                deref(stack.get_index(k - 1)).right_set_link(deref(p).right_get_link());
                deref(stack.get_index(k - 1)).right_set_thread();
            }
        }
    }
    else
    {
        index_type r = deref(p).right_get_link();
        if(deref(r).left_is_thread())
        {
            deref(r).left_set_link(deref(p).left_get_link());
            if(deref(p).left_is_child())
            {
                deref(r).left_set_child();
                index_type t = deref(p).left_get_link();
                while(deref(t).right_is_child())
                {
                    t = deref(t).right_get_link();
                }
                deref(t).right_set_link(r);
            }
            else
            {
                deref(r).left_set_thread();
            }
            if(k == 0)
            {
                root.root.root = r;
            }
            else if(stack.is_left(k - 1))
            {
                deref(stack.get_index(k - 1)).left_set_link(r);
            }
            else
            {
                deref(stack.get_index(k - 1)).right_set_link(r);
            }
            bool is_red = deref(r).is_red();
            if(deref(p).is_red())
            {
                deref(r).set_red();
            }
            else
            {
                deref(r).set_black();
            }
            if(is_red)
            {
                deref(p).set_red();
            }
            else
            {
                deref(p).set_black();
            }
            stack.update_index(k, r, false);
            ++k;
        }
        else
        {
            index_type s;
            size_t const j = stack.height - 1;
            for(++k; ; )
            {
                stack.update_index(k, r, true);
                ++k;
                s = deref(r).left_get_link();
                if(deref(s).left_is_thread())
                {
                    break;
                }
                r = s;
            }
            assert(stack.get_index(j) == p);
            assert(j == stack.height - 1);
            stack.update_index(j, s, false);
            if(deref(s).right_is_child())
            {
                deref(r).left_set_link(deref(s).right_get_link());
            }
            else
            {
                assert(deref(r).left_get_link() == s);
                deref(r).left_set_thread();
            }
            deref(s).left_set_link(deref(p).left_get_link());
            if(deref(p).left_is_child())
            {
                index_type t = deref(p).left_get_link();
                while(deref(t).right_is_child())
                {
                    t = deref(t).right_get_link();
                }
                deref(t).right_set_link(s);
                deref(s).left_set_child();
            }
            deref(s).right_set_link(deref(p).right_get_link());
            deref(s).right_set_child();
            bool is_red = deref(p).is_red();
            if(deref(s).is_red())
            {
                deref(p).set_red();
            }
            else
            {
                deref(p).set_black();
            }
            if(is_red)
            {
                deref(s).set_red();
            }
            else
            {
                deref(s).set_black();
            }
            if(j == 0)
            {
                root.root.root = s;
            }
            else if(stack.is_left(j - 1))
            {
                deref(stack.get_index(j - 1)).left_set_link(s);
            }
            else
            {
                deref(stack.get_index(j - 1)).right_set_link(s);
            }
        }
    }
    if(deref(p).is_black())
    {
        for(; k > 1; --k)
        {
            if(stack.is_left(k - 1))
            {
                if(deref(stack.get_index(k - 1)).left_is_child())
                {
                    index_type x = deref(stack.get_index(k - 1)).left_get_link();
                    if(deref(x).is_red())
                    {
                        deref(x).set_black();
                        break;
                    }
                }
                index_type w = deref(stack.get_index(k - 1)).right_get_link();
                if(deref(w).is_red())
                {
                    deref(w).set_black();
                    deref(stack.get_index(k - 1)).set_red();
                    deref(stack.get_index(k - 1)).right_set_link(deref(w).left_get_link());
                    deref(w).left_set_link(stack.get_index(k - 1));
                    if(k == 1)
                    {
                        root.root.root = w;
                    }
                    else if(stack.is_left(k - 2))
                    {
                        deref(stack.get_index(k - 2)).left_set_link(w);
                    }
                    else
                    {
                        deref(stack.get_index(k - 2)).right_set_link(w);
                    }
                    stack.update_index(k, stack.get_index(k - 1), true);
                    stack.update_index(k - 1, w);
                    w = deref(stack.get_index(k)).right_get_link();
                    ++k;
                }
                if((deref(w).left_is_thread() || deref(deref(w).left_get_link()).is_black()) && (deref(w).right_is_thread() || deref(deref(w).right_get_link()).is_black()))
                {
                    deref(w).set_red();
                }
                else
                {
                    if(deref(w).right_is_thread() || deref(deref(w).right_get_link()).is_black())
                    {
                        index_type y = deref(w).left_get_link();
                        deref(y).set_black();
                        deref(w).set_red();
                        deref(w).left_set_link(deref(y).right_get_link());
                        deref(y).right_set_link(w);
                        deref(stack.get_index(k - 1)).right_set_link(y);
                        if(deref(y).right_is_thread())
                        {
                            index_type z = deref(y).right_get_link();
                            deref(y).right_set_child();
                            deref(z).left_set_thread();
                            deref(z).left_set_link(y);
                        }
                        w = y;
                    }
                    if(deref(stack.get_index(k - 1)).is_red())
                    {
                        deref(w).set_red();
                    }
                    else
                    {
                        deref(w).set_black();
                    }
                    deref(stack.get_index(k - 1)).set_black();
                    deref(deref(w).right_get_link()).set_black();
                    deref(stack.get_index(k - 1)).right_set_link(deref(w).left_get_link());
                    deref(w).left_set_link(stack.get_index(k - 1));
                    if(k == 1)
                    {
                        root.root.root = w;
                    }
                    else if(stack.is_left(k - 2))
                    {
                        deref(stack.get_index(k - 2)).left_set_link(w);
                    }
                    else
                    {
                        deref(stack.get_index(k - 2)).right_set_link(w);
                    }
                    if(deref(w).left_is_thread())
                    {
                        deref(w).left_set_child();
                        deref(stack.get_index(k - 1)).right_set_thread();
                        deref(stack.get_index(k - 1)).right_set_link(w);
                    }
                    break;
                }
            }
            else
            {
                if(deref(stack.get_index(k - 1)).right_is_child())
                {
                    index_type x = deref(stack.get_index(k - 1)).right_get_link();
                    if(deref(x).is_red())
                    {
                        deref(x).set_black();
                        break;
                    }
                }
                index_type w = deref(stack.get_index(k - 1)).left_get_link();
                if(deref(w).is_red())
                {
                    deref(w).set_black();
                    deref(stack.get_index(k - 1)).set_red();
                    deref(stack.get_index(k - 1)).left_set_link(deref(w).right_get_link());
                    deref(w).right_set_link(stack.get_index(k - 1));
                    if(k == 1)
                    {
                        root.root.root = w;
                    }
                    else if(stack.is_right(k - 2))
                    {
                        deref(stack.get_index(k - 2)).right_set_link(w);
                    }
                    else
                    {
                        deref(stack.get_index(k - 2)).left_set_link(w);
                    }
                    stack.update_index(k, stack.get_index(k - 1), false);
                    stack.update_index(k - 1, w);
                    w = deref(stack.get_index(k)).left_get_link();
                    ++k;
                }
                if((deref(w).right_is_thread() || deref(deref(w).right_get_link()).is_black()) && (deref(w).left_is_thread() || deref(deref(w).left_get_link()).is_black()))
                {
                    deref(w).set_red();
                }
                else
                {
                    if(deref(w).left_is_thread() || deref(deref(w).left_get_link()).is_black())
                    {
                        index_type y = deref(w).right_get_link();
                        deref(y).set_black();
                        deref(w).set_red();
                        deref(w).right_set_link(deref(y).left_get_link());
                        deref(y).left_set_link(w);
                        deref(stack.get_index(k - 1)).left_set_link(y);
                        if(deref(y).left_is_thread())
                        {
                            index_type z = deref(y).left_get_link();
                            deref(y).left_set_child();
                            deref(z).right_set_thread();
                            deref(z).right_set_link(y);
                        }
                        w = y;
                    }
                    if(deref(stack.get_index(k - 1)).is_red())
                    {
                        deref(w).set_red();
                    }
                    else
                    {
                        deref(w).set_black();
                    }
                    deref(stack.get_index(k - 1)).set_black();
                    deref(deref(w).left_get_link()).set_black();
                    deref(stack.get_index(k - 1)).left_set_link(deref(w).right_get_link());
                    deref(w).right_set_link(stack.get_index(k - 1));
                    if(k == 1)
                    {
                        root.root.root = w;
                    }
                    else if(stack.is_right(k - 2))
                    {
                        deref(stack.get_index(k - 2)).right_set_link(w);
                    }
                    else
                    {
                        deref(stack.get_index(k - 2)).left_set_link(w);
                    }
                    if(deref(w).right_is_thread())
                    {
                        deref(w).right_set_child();
                        deref(stack.get_index(k - 1)).left_set_thread();
                        deref(stack.get_index(k - 1)).left_set_link(w);
                    }
                    break;
                }
            }
        }
        if(root.root.root != node_type::nil_sentinel)
        {
            deref(root.root.root).set_black();
        }
    }
}


template<class config_t>
class threaded_rb_tree_impl
{
public:
    typedef typename config_t::key_type key_type;
    typedef typename config_t::mapped_type mapped_type;
    typedef typename config_t::value_type value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename config_t::key_compare key_compare;
    typedef value_type &reference;
    typedef value_type const &const_reference;
    typedef value_type *pointer;
    typedef value_type const *const_pointer;
    
protected:
    typedef typename config_t::container_type container_type;
    typedef typename config_t::node_type node_type;
    typedef typename node_type::index_type index_type;
    typedef typename config_t::storage_type storage_type;
    
    typedef threaded_rb_tree_root_t<node_type, std::true_type, std::true_type> root_node_t;
    
    static size_type constexpr stack_max_depth = sizeof(index_type) * 12;
    
    struct root_t : public threaded_rb_tree_root_t<node_type, std::true_type, std::true_type>, public key_compare
    {
        template<class any_key_compare> root_t(any_key_compare &&comp) : key_compare(std::forward<any_key_compare>(comp))
        {
            free = node_type::nil_sentinel;
        }
        container_type container;
        index_type free;
    };
    
    
    struct deref_node_t
    {
        node_type &operator()(index_type index) const
        {
            return config_t::get_node(*container_ptr, index);
        }
        container_type *container_ptr;
    };
    struct deref_value_t
    {
        storage_type &operator()(index_type index) const
        {
            return config_t::get_value(*container_ptr, index);
        }
        container_type *container_ptr;
    };
    template<class k_t, class v_t> struct get_key_select_t
    {
        key_type const &operator()(key_type const &value) const
        {
            return value;
        }
        key_type const &operator()(storage_type const &value) const
        {
            return config_t::get_key(value);
        }
        template<class first_t, class second_t> key_type operator()(std::pair<first_t, second_t> const &pair)
        {
            return key_type(pair.first);
        }
        template<class in_t, class ...args_t> key_type operator()(in_t const &in, args_t const &...args) const
        {
            return key_type(in);
        }
    };
    template<class k_t> struct get_key_select_t<k_t, k_t>
    {
        key_type const &operator()(key_type const &value) const
        {
            return config_t::get_key(value);
        }
        template<class in_t, class ...args_t> key_type operator()(in_t const &in, args_t const &...args) const
        {
            return key_type(in, args...);
        }
    };
    typedef get_key_select_t<key_type, storage_type> get_key_t;
    struct key_compare_ex
    {
        bool operator()(index_type left, index_type right) const
        {
            key_compare &compare = *root_ptr;
            auto &left_value = config_t::get_value(root_ptr->container, left);
            auto &right_value = config_t::get_value(root_ptr->container, right);
            if(compare(config_t::get_key(left_value), config_t::get_key(right_value)))
            {
                return true;
            }
            else if(compare(config_t::get_key(right_value), config_t::get_key(left_value)))
            {
                return false;
            }
            else
            {
                return left < right;
            }
        }
        root_t *root_ptr;
    };
    
public:
    class iterator
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename threaded_rb_tree_impl::value_type value_type;
        typedef typename threaded_rb_tree_impl::difference_type difference_type;
        typedef typename threaded_rb_tree_impl::reference reference;
        typedef typename threaded_rb_tree_impl::pointer pointer;
    public:
        explicit iterator(threaded_rb_tree_impl *in_tree, index_type in_where) : tree(in_tree), where(in_where)
        {
        }
        iterator(iterator const &) = default;
        iterator &operator++()
        {
            where = threaded_rb_tree_move_next(where, deref_node_t{&tree->root_.container});
            return *this;
        }
        iterator &operator--()
        {
            if(where == node_type::nil_sentinel)
            {
                where = tree->root_.get_right();
            }
            else
            {
                where = threaded_rb_tree_move_prev(where, deref_node_t{&tree->root_.container});
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator save(*this);
            ++*this;
            return save;
        }
        iterator operator--(int)
        {
            iterator save(*this);
            --*this;
            return save;
        }
        reference operator *() const
        {
            return reinterpret_cast<reference>(config_t::get_value(tree->root_.container, where));
        }
        pointer operator->() const
        {
            return reinterpret_cast<pointer>(&config_t::get_value(tree->root_.container, where));
        }
        bool operator == (iterator const &other) const
        {
            return where == other.where && tree == other.tree;
        }
        bool operator != (iterator const &other) const
        {
            return where != other.where || tree != other.tree;
        }
    private:
        friend class threaded_rb_tree_impl;
        threaded_rb_tree_impl *tree;
        index_type where;
    };
    class const_iterator
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename threaded_rb_tree_impl::value_type value_type;
        typedef typename threaded_rb_tree_impl::difference_type difference_type;
        typedef typename threaded_rb_tree_impl::reference reference;
        typedef typename threaded_rb_tree_impl::const_reference const_reference;
        typedef typename threaded_rb_tree_impl::pointer pointer;
        typedef typename threaded_rb_tree_impl::const_pointer const_pointer;
    public:
        explicit const_iterator(threaded_rb_tree_impl *in_tree, index_type in_where) : tree(in_tree), where(in_where)
        {
        }
        const_iterator(iterator const &other) : tree(other.tree), where(other.where)
        {
        }
        const_iterator(const_iterator const &) = default;
        const_iterator &operator++()
        {
            where = threaded_rb_tree_move_next(where, deref_node_t{&tree->root_.container});
            return *this;
        }
        const_iterator &operator--()
        {
            if(where == node_type::nil_sentinel)
            {
                where = tree->root_.get_right();
            }
            else
            {
                where = threaded_rb_tree_move_prev(where, deref_node_t{&tree->root_.container});
            }
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator save(*this);
            ++*this;
            return save;
        }
        const_iterator operator--(int)
        {
            const_iterator save(*this);
            --*this;
            return save;
        }
        const_reference operator *() const
        {
            return reinterpret_cast<const_reference>(config_t::get_value(tree->root_.container, where));
        }
        const_pointer operator->() const
        {
            return reinterpret_cast<const_pointer>(&config_t::get_value(tree->root_.container, where));
        }
        bool operator == (iterator const &other) const
        {
            return where == other.where && tree == other.tree;
        }
        bool operator != (iterator const &other) const
        {
            return where != other.where || tree != other.tree;
        }
    private:
        friend class threaded_rb_tree_impl;
        threaded_rb_tree_impl const *tree;
        index_type where;
    };
    class reverse_iterator
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename threaded_rb_tree_impl::value_type value_type;
        typedef typename threaded_rb_tree_impl::difference_type difference_type;
        typedef typename threaded_rb_tree_impl::reference reference;
        typedef typename threaded_rb_tree_impl::pointer pointer;
    public:
        explicit reverse_iterator(threaded_rb_tree_impl *in_tree, index_type in_where) : tree(in_tree), where(in_where)
        {
        }
        explicit reverse_iterator(iterator const &other) : tree(other.tree), where(other.where)
        {
            ++*this;
        }
        reverse_iterator(reverse_iterator const &) = default;
        reverse_iterator &operator++()
        {
            where = threaded_rb_tree_move_prev(where, deref_node_t{&tree->root_.container});
            return *this;
        }
        reverse_iterator &operator--()
        {
            if(where == node_type::nil_sentinel)
            {
                where = tree->root_.get_left();
            }
            else
            {
                where = threaded_rb_tree_move_next(where, deref_node_t{&tree->root_.container});
            }
            return *this;
        }
        reverse_iterator operator++(int)
        {
            reverse_iterator save(*this);
            ++*this;
            return save;
        }
        reverse_iterator operator--(int)
        {
            reverse_iterator save(*this);
            --*this;
            return save;
        }
        reference operator *() const
        {
            return reinterpret_cast<reference>(config_t::get_value(tree->root_.container, where));
        }
        pointer operator->() const
        {
            return reinterpret_cast<pointer>(&config_t::get_value(tree->root_.container, where));
        }
        bool operator == (iterator const &other) const
        {
            return where == other.where && tree == other.tree;
        }
        bool operator != (iterator const &other) const
        {
            return where != other.where || tree != other.tree;
        }
        iterator base() const
        {
            return ++iterator(tree, where);
        }
    private:
        friend class threaded_rb_tree_impl;
        threaded_rb_tree_impl *tree;
        index_type where;
    };
    class const_reverse_iterator
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename threaded_rb_tree_impl::value_type value_type;
        typedef typename threaded_rb_tree_impl::difference_type difference_type;
        typedef typename threaded_rb_tree_impl::reference reference;
        typedef typename threaded_rb_tree_impl::const_reference const_reference;
        typedef typename threaded_rb_tree_impl::pointer pointer;
        typedef typename threaded_rb_tree_impl::const_pointer const_pointer;
    public:
        explicit const_reverse_iterator(threaded_rb_tree_impl *in_tree, index_type in_where) : tree(in_tree), where(in_where)
        {
        }
        explicit const_reverse_iterator(const_iterator const &other) : tree(other.tree), where(other.where)
        {
            ++*this;
        }
        const_reverse_iterator(reverse_iterator const &other) : tree(other.tree), where(other.where)
        {
        }
        const_reverse_iterator(const_reverse_iterator const &) = default;
        const_reverse_iterator &operator++()
        {
            where = threaded_rb_tree_move_prev(where, deref_node_t{&tree->root_.container});
            return *this;
        }
        const_reverse_iterator &operator--()
        {
            if(where == node_type::nil_sentinel)
            {
                where = tree->root_.get_left();
            }
            else
            {
                where = threaded_rb_tree_move_next(where, deref_node_t{&tree->root_.container});
            }
            return *this;
        }
        const_reverse_iterator operator++(int)
        {
            const_reverse_iterator save(*this);
            ++*this;
            return save;
        }
        const_reverse_iterator operator--(int)
        {
            const_reverse_iterator save(*this);
            --*this;
            return save;
        }
        const_reference operator *() const
        {
            return reinterpret_cast<const_reference>(config_t::get_value(tree->root_.container, where));
        }
        const_pointer operator->() const
        {
            return reinterpret_cast<const_pointer>(&config_t::get_value(tree->root_.container, where));
        }
        bool operator == (iterator const &other) const
        {
            return where == other.where && tree == other.tree;
        }
        bool operator != (iterator const &other) const
        {
            return where != other.where || tree != other.tree;
        }
        const_iterator base() const
        {
            return ++const_iterator(tree, where);
        }
    private:
        friend class threaded_rb_tree_impl;
        threaded_rb_tree_impl const *tree;
        index_type where;
    };
public:
    typedef typename std::conditional<config_t::unique_type::value, std::pair<iterator, bool>, iterator>::type insert_result_t;
    typedef std::pair<iterator, bool> pair_ib_t;
protected:
    typedef std::pair<index_type, bool> pair_posi_t;
    template<class unique_type> typename std::enable_if<unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return std::make_pair(iterator(this, posi.first), posi.second);
    }
    template<class unique_type> typename std::enable_if<!unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return iterator(this, posi.first);
    }
public:
    //empty
    threaded_rb_tree_impl() : root_(key_compare())
    {
    }
    //empty
    explicit threaded_rb_tree_impl(key_compare const &comp) : root_(comp)
    {
    }
    //range
    template <class iterator_t> threaded_rb_tree_impl(iterator_t begin, iterator_t end, key_compare const &comp = key_compare()) : root_(comp)
    {
        insert(begin, end);
    }
    //copy
    threaded_rb_tree_impl(threaded_rb_tree_impl const &other) : root_(other.get_comparator_())
    {
        root_ = other.root_;
    }
    //move
    threaded_rb_tree_impl(threaded_rb_tree_impl &&other) : root_(key_compare())
    {
        std::swap(root_, root_);
    }
    //initializer list
    threaded_rb_tree_impl(std::initializer_list<value_type> il, key_compare const &comp = key_compare()) : threaded_rb_tree_impl(il.begin(), il.end(), comp)
    {
    }
    //destructor
    ~threaded_rb_tree_impl()
    {
        clear();
    }
    //copy
    threaded_rb_tree_impl &operator = (threaded_rb_tree_impl const &other)
    {
        if(this == &other)
        {
            return *this;
        }
        //TODO
        return *this;
    }
    //move
    threaded_rb_tree_impl &operator = (threaded_rb_tree_impl &&other)
    {
        if(this == &other)
        {
            return *this;
        }
        std::swap(root_, other.root_);
        return *this;
    }
    //initializer list
    threaded_rb_tree_impl &operator = (std::initializer_list<value_type> il)
    {
        clear();
        insert(il, il.end());
        return *this;
    }

    
    void swap(threaded_rb_tree_impl &other)
    {
        std::swap(root_, other.root_);
    }
    
    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;
    
    //single element
    insert_result_t insert(value_type const &value)
    {
        check_max_size_();
        return result_<typename config_t::unique_type>(trb_insert_(typename config_t::unique_type(), value));
    }
    //single element
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(in_value_t &&value)
    {
        check_max_size_();
        return result_<typename config_t::unique_type>(trb_insert_(typename config_t::unique_type(), std::forward<in_value_t>(value)));
    }
    //with hint
    iterator insert(const_iterator hint, value_type const &value)
    {
        check_max_size_();
        return result_<std::false_type>(trb_insert_(typename config_t::unique_type(), value));
    }
    //with hint
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(const_iterator hint, in_value_t &&value)
    {
        check_max_size_();
        return result_<typename config_t::unique_type>(trb_insert_(typename config_t::unique_type(), std::forward<in_value_t>(value)));
    }
    //range
    template<class iterator_t> void insert(iterator_t begin, iterator_t end)
    {
        for(; begin != end; ++begin)
        {
            emplace(*begin);
        }
    }
    //initializer list
    void insert(std::initializer_list<value_type> il)
    {
        insert(il.begin(), il.end());
    }
    
    //single element
    template<class ...args_t> insert_result_t emplace(args_t &&...args)
    {
        check_max_size_();
        return result_<typename config_t::unique_type>(trb_insert_(typename config_t::unique_type(), std::forward<args_t>(args)...));
    }
    //with hint
    template<class ...args_t> insert_result_t emplace_hint(const_iterator hint, args_t &&...args)
    {
        check_max_size_();
        return result_<typename config_t::unique_type>(trb_insert_(typename config_t::unique_type(), std::forward<args_t>(args)...));
    }
    
    iterator find(key_type const &key)
    {
        index_type where = bst_lower_bound_(key);
        return (where == node_type::nil_sentinel || get_comparator_()(key, get_key_(where))) ? iterator(this, node_type::nil_sentinel) : iterator(this, where);
    }
    const_iterator find(key_type const &key) const
    {
        index_type where = bst_lower_bound_(key);
        return (where == node_type::nil_sentinel || get_comparator_()(key, get_key_(where))) ? const_iterator(this, node_type::nil_sentinel) : const_iterator(this, where);
    }
    
    iterator erase(const_iterator where)
    {
        auto index = threaded_rb_tree_move_next(where.where, deref_node_t{&root_.container});
        trb_erase_(where.where);
        return iterator(this, index);
    }
    size_type erase(key_type const &key)
    {
        size_type erase_count = 0;
        while(trb_erase_(key))
        {
            if(config_t::unique_type::value)
            {
                break;
            }
            ++erase_count;
        }
        return erase_count;
    }
    iterator erase(const_iterator erase_begin, const_iterator erase_end)
    {
        if(erase_begin == cbegin() && erase_end == cend())
        {
            clear();
            return begin();
        }
        else
        {
            while(erase_begin != erase_end)
            {
                erase(erase_begin++);
            }
            return iterator(erase_begin.node);
        }
    }
    
    size_type count(key_type const &key) const
    {
        pair_cici_t range = equal_range(key);
        return std::distance(range.first, range.second);
    }
    
    iterator lower_bound(key_type const &key)
    {
        return iterator(bst_lower_bound_(key));
    }
    const_iterator lower_bound(key_type const &key) const
    {
        return const_iterator(bst_lower_bound_(key));
    }
    iterator upper_bound(key_type const &key)
    {
        return iterator(bst_upper_bound_(key));
    }
    const_iterator upper_bound(key_type const &key) const
    {
        return const_iterator(bst_upper_bound_(key));
    }
    
    pair_ii_t equal_range(key_type const &key)
    {
        index_type lower, upper;
        bst_equal_range_(key, lower, upper);
        return pair_ii_t(iterator(lower), iterator(upper));
    }
    pair_cici_t equal_range(key_type const &key) const
    {
        index_type lower, upper;
        bst_equal_range_(key, lower, upper);
        return pair_cici_t(const_iterator(this, lower), const_iterator(this, upper));
    }
    
    iterator begin()
    {
        return iterator(this, root_.get_most_left(deref_node_t{&root_.container}));
    }
    iterator end()
    {
        return iterator(this, node_type::nil_sentinel);
    }
    const_iterator begin() const
    {
        return const_iterator(this, root_.get_most_left(deref_node_t{&root_.container}));
    }
    const_iterator end() const
    {
        return const_iterator(this, node_type::nil_sentinel);
    }
    const_iterator cbegin() const
    {
        return const_iterator(this, root_.get_most_left(deref_node_t{&root_.container}));
    }
    const_iterator cend() const
    {
        return const_iterator(this, node_type::nil_sentinel);
    }
    reverse_iterator rbegin()
    {
        return reverse_iterator(this, root_.get_most_right(deref_node_t{&root_.container}));
    }
    reverse_iterator rend()
    {
        return reverse_iterator(this, node_type::nil_sentinel);
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(this, root_.get_most_right(deref_node_t{&root_.container}));
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(this, node_type::nil_sentinel);
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(this, root_.get_most_right(deref_node_t{&root_.container}));
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(this, node_type::nil_sentinel);
    }
    
    bool empty() const
    {
        return root_.root == node_type::nil_sentinel;
    }
    void clear()
    {
        trb_clear_();
    }
    size_type size() const
    {
        return root_.get_count();
    }
    size_type max_size() const
    {
        return std::min<size_type>(root_.container.max_size(), node_type::nil_sentinel);
    }
    
protected:
    root_t root_;
    
protected:
    key_compare &get_comparator_()
    {
        return root_;
    }
    key_compare const &get_comparator_() const
    {
        return root_;
    }
    
    key_type get_key_(index_type index) const
    {
        return get_key_t()(config_t::get_value(root_.container, index));
    }
    
    index_type alloc_index_()
    {
        if(root_.free == node_type::nil_sentinel)
        {
            return config_t::alloc_index(root_.container);
        }
        auto deref = deref_node_t{&root_.container};
        index_type index = root_.free;
        root_.free = deref(index).left_get_link();
        return index;
    }
    
    void dealloc_index_(index_type index)
    {
        auto deref = deref_node_t{&root_.container};
        deref(index).left_set_link(root_.free);
        deref(index).right_set_link(node_type::free_sentinel);
        root_.free = index;
    }
    
    index_type bst_lower_bound_(key_type const &key) const
    {
        index_type node = root_.root.root, where = node_type::nil_sentinel;
        while(node != node_type::nil_sentinel)
        {
            if(get_comparator_()(get_key_(node), key))
            {
                node = get_deref_(node).right_get_link();
            }
            else
            {
                where = node;
                node = get_deref_(node).left_get_link();
            }
        }
        return where;
    }
    
    index_type bst_upper_bound_(key_type const &key) const
    {
        index_type node = root_.root.root, where = node_type::nil_sentinel;
        while(node != node_type::nil_sentinel)
        {
            if(get_comparator_()(key, get_key_(node)))
            {
                where = node;
                node = get_deref_(node).left_get_link();
            }
            else
            {
                node = get_deref_(node).right_get_link();
            }
        }
        return where;
    }
    
    void bst_equal_range_(key_type const &key, index_type &lower, index_type &upper) const
    {
        index_type node = root_.root.root;
        lower = node_type::nil_sentinel;
        upper = node_type::nil_sentinel;
        while(node != node_type::nil_sentinel)
        {
            if(get_comparator_()(get_key_(node), key))
            {
                node = get_deref_(node).right_get_link();
            }
            else
            {
                if(upper == node_type::nil_sentinel && get_comparator_()(key, get_key_(node)))
                {
                    upper = node;
                }
                lower = node;
                node = get_deref_(node).left_get_link();
            }
        }
        node = upper == node_type::nil_sentinel ? root_.root.root : get_deref_(upper).left_get_link();
        while(node != node_type::nil_sentinel)
        {
            if(get_comparator_()(key, get_key_(node)))
            {
                upper = node;
                node = get_deref_(node).left_get_link();
            }
            else
            {
                node = get_deref_(node).right_get_link();
            }
        }
    }
    
    void check_max_size_()
    {
        if(size() >= max_size() - 1)
        {
            throw std::length_error("threaded_rb_tree too long");
        }
    }
    
    template<class arg_first_t, class ...args_other_t> pair_posi_t trb_insert_(std::false_type, arg_first_t &&arg_first, args_other_t &&...args_other)
    {
        threaded_rb_tree_stack_t<node_type, stack_max_depth> stack;
        index_type index = alloc_index_();
        auto &value = config_t::get_value(root_.container, index);
        ::new(&value) storage_type(std::forward<arg_first_t>(arg_first), std::forward<args_other_t>(args_other)...);
        threaded_rb_tree_find_path_for_multi(root_, stack, deref_node_t{&root_.container}, index, key_compare_ex{&root_});
        threaded_rb_tree_insert(root_, stack, deref_node_t{&root_.container}, index);
        return {index, true};
    }
    
    template<class arg_first_t, class ...args_other_t> pair_posi_t trb_insert_(std::true_type, arg_first_t &&arg_first, args_other_t &&...args_other)
    {
        threaded_rb_tree_stack_t<node_type, stack_max_depth> stack;
        auto key = get_key_t()(arg_first, args_other...);
        bool exists = threaded_rb_tree_find_path_for_unique(root_, stack, deref_node_t{&root_.container}, key, deref_value_t{&root_.container}, get_key_t(), get_comparator_());
        if(exists)
        {
            return {stack.get_index(stack.height - 1), false};
        }
        index_type index = alloc_index_();
        auto &value = config_t::get_value(root_.container, index);
        ::new(&value) storage_type(std::forward<arg_first_t>(arg_first), std::forward<args_other_t>(args_other)...);
        threaded_rb_tree_insert(root_, stack, deref_node_t{&root_.container}, index);
        return {index, true};
    }
    
    bool trb_erase_(key_type const &key)
    {
        threaded_rb_tree_stack_t<node_type, stack_max_depth> stack;
        bool exists = threaded_rb_tree_find_path_for_unique(root_, stack, deref_node_t{&root_.container}, key, deref_value_t{&root_.container}, get_key_t(), get_comparator_());
        if(!exists)
        {
            return false;
        }
        auto &value = config_t::get_value(root_.container, index);
        value.~storage_type();
        dealloc_index_(stack.height - 1);
        threaded_rb_tree_remove(root_, stack, deref_node_t{&root_.container});
        return true;
    }
    
    void trb_erase_(index_type index)
    {
        threaded_rb_tree_stack_t<node_type, stack_max_depth> stack;
        bool exists = threaded_rb_tree_find_path_for_remove(root_, stack, deref_node_t{&root_.container}, index, key_compare_ex{&root_});
        assert(exists);
        auto &value = config_t::get_value(root_.container, index);
        value.~storage_type();
        dealloc_index_(stack.height - 1);
        threaded_rb_tree_remove(root_, stack, deref_node_t{&root_.container});
    }
    
    void trb_clear_()
    {
        auto deref = deref_node_t{&root_.container};
        auto deref_value = deref_value_t{&root_.container};
        root_.root.root = node_type::nil_sentinel;
        root_.root.set_left(node_type::nil_sentinel);
        root_.root.set_right(node_type::nil_sentinel);
        root_.free = node_type::nil_sentinel;
        for(index_type i = 0; i < root_.get_count(); ++i)
        {
            if(deref(i).right_get_link() != node_type::free_sentinel)
            {
                deref_value(i).~storage_type();
            }
        }
        root_.container.clear();
    }
};

template<class key_t, class comparator_t, class index_t, class unique_t>
struct threaded_rb_tree_default_set_config_t
{
    typedef key_t key_type;
    typedef key_t const mapped_type;
    typedef key_t const value_type;
    typedef key_t storage_type;
    typedef comparator_t key_compare;
    
    typedef threaded_rb_tree_node_t<index_t> node_type;
    struct element_type
    {
        node_type node;
        uint8_t value[sizeof(storage_type)];
    };
    typedef std::vector<element_type> container_type;
    typedef unique_t unique_type;
    
    static node_type &get_node(container_type &container, typename node_type::index_type index)
    {
        return container[index].node;
    }
    static storage_type &get_value(container_type &container, typename node_type::index_type index)
    {
        return reinterpret_cast<storage_type &>(container[index].value);
    }
    static typename node_type::index_type alloc_index(container_type &container)
    {
        auto index = typename node_type::index_type(container.size());
        container.emplace_back();
        return index;
    }
    
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value;
    }
};


template<class key_t, class value_t, class comparator_t, class index_t, class unique_t>
struct threaded_rb_tree_default_map_config_t
{
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef std::pair<key_t, value_t> storage_type;
    typedef comparator_t key_compare;
    
    typedef threaded_rb_tree_node_t<index_t> node_type;
    struct element_type
    {
        node_type node;
        uint8_t value[sizeof(storage_type)];
    };
    typedef std::vector<element_type> container_type;
    typedef unique_t unique_type;
    
    static node_type &get_node(container_type &container, typename node_type::index_type index)
    {
        return container[index].node;
    }
    static storage_type &get_value(container_type &container, typename node_type::index_type index)
    {
        return reinterpret_cast<storage_type &>(container[index].value);
    }
    static typename node_type::index_type alloc_index(container_type &container)
    {
        auto index = typename node_type::index_type(container.size());
        container.emplace_back();
        return index;
    }
    
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value.first;
    }
};

template<class key_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using trb_set = threaded_rb_tree_impl<threaded_rb_tree_default_set_config_t<key_t, comparator_t, index_t, std::true_type>>;

template<class key_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using trb_multiset = threaded_rb_tree_impl<threaded_rb_tree_default_set_config_t<key_t, comparator_t, index_t, std::false_type>>;

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using trb_map = threaded_rb_tree_impl<threaded_rb_tree_default_map_config_t<key_t, value_t, comparator_t, index_t, std::true_type>>;

template<class key_t, class value_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using trb_multimap = threaded_rb_tree_impl<threaded_rb_tree_default_map_config_t<key_t, value_t, comparator_t, index_t, std::false_type>>;












