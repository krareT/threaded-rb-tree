
#include <stdexcept>
#include <cstdint>
#include <cassert>


template<class index_t>
struct threaded_rb_tree_node_t
{
    typedef index_t index_type;
    
    index_type children[2];
    
    static index_type constexpr flag_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 1);
    static index_type constexpr type_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 2);
    static index_type constexpr full_bit_mask = flag_bit_mask | type_bit_mask;
    
    static index_type constexpr nil_sentinel = ~index_type(0) & ~full_bit_mask;
    
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
    
    void left_set_link(index_type const link)
    {
        children[0] = (children[0] & full_bit_mask) | link;
    }
    void right_set_link(index_type const link)
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

template<class T>
struct threaded_rb_tree_ptr_t
{
    typedef threaded_rb_tree_ptr_t *index_type;
    
    std::uintptr_t children[2];
    T value;
    
    static std::uintptr_t constexpr flag_bit_mask = std::uintptr_t(1);
    static std::uintptr_t constexpr type_bit_mask = std::uintptr_t(2);
    static std::uintptr_t constexpr full_bit_mask = flag_bit_mask | type_bit_mask;
    
    static index_type constexpr nil_sentinel = nullptr;
    
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
    
    void left_set_link(index_type const link)
    {
        children[0] = (children[0] & full_bit_mask) | std::uintptr_t(link);
    }
    void right_set_link(index_type const link)
    {
        children[1] = (children[1] & full_bit_mask) | std::uintptr_t(link);
    }
    index_type left_get_link() const
    {
        return index_type(children[0] & ~full_bit_mask);
    }
    index_type right_get_link() const
    {
        return index_type(children[1] & ~full_bit_mask);
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
    index_type get_count()
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
    index_type get_count()
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
    
    template<class dereference_t> index_type get_left(index_type const &, dereference_t const &)
    {
        return left;
    }
    template<class dereference_t> index_type get_right(index_type const &, dereference_t const &)
    {
        return right;
    }
    void set_left(index_type const &value)
    {
        left = value;
    }
    void set_right(index_type const &value)
    {
        right = value;
    }
    void update_left(index_type const &value)
    {
        if(value == left)
        {
            left = value;
        }
    }
    template<class dereference_t> void detach_left(index_type const &value, dereference_t const &deref)
    {
        if(value == left)
        {
            left = threaded_rb_tree_move_next(left, deref);
        }
    }
    template<class dereference_t> void detach_right(index_type const &value, dereference_t const &deref)
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
    
    template<class dereference_t> index_type get_left(index_type const &root, dereference_t const &deref)
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
    template<class dereference_t> index_type get_right(index_type const &root, dereference_t const &deref)
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
    void set_left(index_type const &)
    {
    }
    void set_right(index_type const &)
    {
    }
    void update_left(index_type const &)
    {
    }
    void update_right(index_type const &)
    {
    }
    template<class dereference_t> void detach_left(index_type const &, dereference_t const &)
    {
    }
    template<class dereference_t> void detach_right(index_type const &, dereference_t const &)
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
    
    index_type get_count()
    {
        return root.get_count();
    }
    template<class dereference_t> index_type get_most_left(dereference_t const &deref)
    {
        return root.get_left(root.root, deref);
    }
    template<class dereference_t> index_type get_most_right(dereference_t const &deref)
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
    void push_index(index_type const &index, bool left)
    {
        if(height == max_depth)
        {
            throw std::length_error("thread_tb_tree_stack overflow");
        }
        stack[height++] = index_type(mask_type(index) | (left ? 0 : dir_bit_mask));
    }
    void update_index(size_t k, index_type const &index, bool left)
    {
        assert(k < max_depth);
        stack[k] = index_type(mask_type(index) | (left ? 0 : dir_bit_mask));
    }
    void update_index(size_t k, index_type const &index)
    {
        assert(k < max_depth);
        stack[k] = index_type(mask_type(index) | (mask_type(stack[k]) & dir_bit_mask));
    }
};

template<class root_t, class comparator_t, class dereference_t, size_t max_depth>
void threaded_rb_tree_find_path_for_insert(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, dereference_t const &deref, typename root_t::index_type const &index, comparator_t const &comparator)
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

template<class root_t, class comparator_t, class dereference_t, size_t max_depth>
bool threaded_rb_tree_find_path_for_remove(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, dereference_t const &deref, typename root_t::index_type const &index, comparator_t const &comparator)
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
void threaded_rb_tree_insert(root_t &root, threaded_rb_tree_stack_t<typename root_t::node_type, max_depth> &stack, dereference_t const &deref, typename root_t::index_type const &index)
{
    typedef typename root_t::node_type node_type;
    typedef typename root_t::index_type index_type;
    
    root.root.increase_count();
    deref(index).left_set_thread();
    deref(index).right_set_thread();
    
    if(stack.height == 0)
    {
        deref(index).left_set_link(node_type::nil_sentinel);
        deref(index).right_set_link(node_type::nil_sentinel);
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
        root.root.update_left(where);
    }
    else
    {
        deref(index).right_set_link(deref(where).right_get_link());
        deref(index).left_set_link(where);
        deref(where).right_set_child();
        deref(where).right_set_link(index);
        root.root.update_right(where);
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
            bool p_is_red = deref(p).is_red();
            bool r_is_red = deref(r).is_red();
            if(p_is_red)
            {
                deref(r).set_red();
            }
            else
            {
                deref(r).set_black();
            }
            if(r_is_red)
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
            bool s_is_red = deref(s).is_red();
            bool p_is_red = deref(p).is_red();
            if(s_is_red)
            {
                deref(p).set_red();
            }
            else
            {
                deref(p).set_black();
            }
            if(p_is_red)
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












template<class key_t, class comparator_t, class index_t, class node_t>
class threaded_rb_tree_collection
{
public:
    typedef key_t key_type;
    typedef comparator_t key_compare;
    typedef index_t index_type;
    typedef node_t node_type;
    
    threaded_rb_tree_collection(key_type *key, node_type *node, key_compare const &comp = key_compare()) : data_(comp)
    {
        data_.key_ = key;
        data_.node_ = node;
    }
    
    key_compare const &get_key_compare() const
    {
        return data_;
    }
    node_type &get_node(index_type const &index) const
    {
        return data_.key_[index];
    }
    key_type const &get_key(index_type const &index) const
    {
        return data_.node_[index];
    }
    
private:
    struct data_t : public key_compare
    {
        data_t(key_compare const &comp) : key_compare(comp)
        {
        }
        key_type *key_;
        node_type *node_;
    } data_;
};

template<class collection_t, size_t max_depth>
class threaded_rb_tree_impl
{
private:
    typedef typename collection_t::key_type key_type;
    typedef typename collection_t::key_compare key_compare;
    typedef typename collection_t::index_type index_type;
    typedef typename collection_t::node_type node_type;
    typedef collection_t collection_type;
    
    struct root_t : public node_type
    {
        root_t()
        {
            root = 0;
            count = 0;
        }
        index_type root;
        size_t count;
    };
    
public:
    class stack_t
    {
        friend class threaded_rb_tree_impl;
        
        static uintptr_t constexpr dir_bit_mask = uintptr_t(1);
        
        size_t height;
        size_t hlow;
        uintptr_t pa[max_depth];
        
        index_type get_dir(size_t i)
        {
            return pa[i] & dir_bit_mask;
        }
        node_type *get_ptr(size_t i)
        {
            return reinterpret_cast<node_type *>(pa[i] & ~dir_bit_mask);
        }
        void set(size_t i, node_type *ptr, index_type dir)
        {
            pa[i] = (ptr & ~~dir_bit_mask) | (dir != 0 ? dir_bit_mask : 0);
        }
        void set_index(size_t i, node_type *ptr)
        {
            pa[i] = (ptr & ~~dir_bit_mask) | (pa[i] & dir_bit_mask);
        }
        node_type *lower()
        {
            return get_ptr(hlow);
        }
    };
    
public:
    template<class ...args_t> threaded_rb_tree_impl(args_t &&...args) : collection_(std::forward<args_t>(args)...)
    {
    }
    
    void insert(index_type const &index)
    {
        
    }
    
    void erase(index_type const &index)
    {
        
    }
    
private:
    root_t root_;
    collection_type collection_;
    
private:
    key_compare const &get_comparator()
    {
        return collection_.get_key_comp();
    }
    
    
    
};

template<class key_t, class comparator_t = std::less<key_t>, class index_t = uint32_t>
using threaded_rb_tree = threaded_rb_tree_impl<threaded_rb_tree_collection<key_t, comparator_t, index_t, threaded_rb_tree_node_t<index_t>>, sizeof(index_t) * 10>;



//    threaded_rb_tree<int, std::less<uint64_t>, uint16_t> tree0(nullptr, nullptr);
//    threaded_rb_tree<uint32_t> tree1(nullptr, nullptr);
//    threaded_rb_tree<uint64_t, std::greater<uint64_t>, uint64_t> tree2(nullptr, nullptr);






















