
#include <stdexcept>
#include <cstdint>
#include <utility>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <new>

namespace threaded_rbtree_hash_detail
{
    class move_trivial_tag
    {
    };
    class move_assign_tag
    {
    };
    template<class T> struct is_trivial_expand : public std::is_trivial<T>
    {
    };
    template<class K, class V> struct is_trivial_expand<std::pair<K, V>> : public std::conditional<std::is_trivial<K>::value && std::is_trivial<V>::value, std::true_type, std::false_type>::type
    {
    };
    template<class iterator_t> struct get_tag
    {
        typedef typename std::conditional<is_trivial_expand<typename std::iterator_traits<iterator_t>::value_type>::value, move_trivial_tag, move_assign_tag>::type type;
    };
    
    template<class iterator_t, class tag_t, class ...args_t> void construct_one(iterator_t where, tag_t, args_t &&...args)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        ::new(std::addressof(*where)) iterator_value_t(std::forward<args_t>(args)...);
    }
    
    template<class iterator_t> void destroy_one(iterator_t where, move_trivial_tag)
    {
    }
    template<class iterator_t> void destroy_one(iterator_t where, move_assign_tag)
    {
        typedef typename std::iterator_traits<iterator_t>::value_type iterator_value_t;
        where->~iterator_value_t();
    }
    
    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_trivial_tag)
    {
        std::ptrdiff_t count = move_end - move_begin;
        std::memmove(&*to_begin, &*move_begin, count * sizeof(*move_begin));
    }
    template<class iterator_from_t, class iterator_to_t> void move_construct_and_destroy(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin, move_assign_tag)
    {
        for(; move_begin != move_end; ++move_begin)
        {
            construct_one(to_begin++, move_assign_tag(), std::move(*move_begin));
            destroy_one(move_begin, move_assign_tag());
        }
    }
}

template<class config_t>
class threaded_rbtree_hash
{
public:
    typedef typename config_t::key_type key_type;
    typedef typename config_t::mapped_type mapped_type;
    typedef typename config_t::value_type value_type;
    typedef typename config_t::storage_type storage_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef typename config_t::hasher hasher;
    typedef typename config_t::key_compare key_compare;
    typedef typename config_t::allocator_type allocator_type;
    typedef typename config_t::offset_type offset_type;
    typedef typename config_t::hash_value_type hash_value_type;
    typedef value_type &reference;
    typedef value_type const &const_reference;
    typedef value_type *pointer;
    typedef value_type const *const_pointer;
    
    
protected:
    
    static size_type constexpr flag_bit_mask = offset_type(1) << (sizeof(offset_type) * 8 - 1);
    static size_type constexpr type_bit_mask = offset_type(1) << (sizeof(offset_type) * 8 - 2);
    static size_type constexpr dir_bit_mask = flag_bit_mask;
    static size_type constexpr full_bit_mask = flag_bit_mask | type_bit_mask;
    
    static size_type constexpr offset_empty = ~offset_type(0) & ~full_bit_mask;
    static size_type constexpr max_stack_depth = 2 * (sizeof(offset_type) * 8 - 1);
    
    struct node_t
    {
        offset_type children[2];
        
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
        
        void left_set_link(size_type link)
        {
            children[0] = (children[0] & full_bit_mask) | link;
        }
        void right_set_link(size_type link)
        {
            children[1] = (children[1] & full_bit_mask) | link;
        }
        size_type left_get_link() const
        {
            return children[0] & ~full_bit_mask;
        }
        size_type right_get_link() const
        {
            return children[1] & ~full_bit_mask;
        }
        
        bool is_used() const
        {
            return (children[1] & flag_bit_mask) == 0;
        }
        void set_used()
        {
            children[1] &= ~flag_bit_mask;
        }
        bool is_empty() const
        {
            return (children[1] & flag_bit_mask) != 0;
        }
        void set_empty()
        {
            children[1] |= flag_bit_mask;
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
    struct stack_t
    {
        stack_t()
        {
            height = 0;
        }
        size_t height;
        offset_type stack[max_stack_depth];
        
        bool is_left(size_t i) const
        {
            return (stack[i] & dir_bit_mask) == 0;
        }
        bool is_right(size_t i) const
        {
            return (stack[i] & dir_bit_mask) != 0;
        }
        size_type get_index(size_t i) const
        {
            return stack[i] & ~dir_bit_mask;
        }
        void push_index(size_type offset, bool left)
        {
            stack[height++] = offset | (left ? 0 : dir_bit_mask);
        }
        void update_index(size_type index, size_type offset, bool left)
        {
            stack[index] = offset | (left ? 0 : dir_bit_mask);
        }
        void update_index(size_type index, size_type offset)
        {
            stack[index] = offset | (stack[index] & dir_bit_mask);
        }
    };
    struct value_t
    {
        typename std::aligned_storage<sizeof(storage_type), alignof(storage_type)>::type value_pod;
        
        storage_type *value()
        {
            return reinterpret_cast<storage_type *>(&value_pod);
        }
        storage_type const *value() const
        {
            return reinterpret_cast<storage_type const *>(&value_pod);
        }
    };
    
    typedef typename allocator_type::template rebind<offset_type>::other bucket_allocator_t;
    typedef typename allocator_type::template rebind<node_t>::other node_allocator_t;
    typedef typename allocator_type::template rebind<value_t>::other value_allocator_t;
    struct root_t : public hasher, public key_compare, public bucket_allocator_t, public node_allocator_t, public value_allocator_t
    {
        template<class any_hasher, class any_key_compare, class any_allocator_type> root_t(any_hasher &&hash, any_key_compare &&compare, any_allocator_type &&alloc)
        : hasher(std::forward<any_hasher>(hash))
        , key_compare(std::forward<any_key_compare>(compare))
        , bucket_allocator_t(alloc)
        , node_allocator_t(alloc)
        , value_allocator_t(std::forward<any_allocator_type>(alloc))
        {
            static_assert(std::is_unsigned<offset_type>::value && std::is_integral<offset_type>::value, "offset_type must be unsighed integer");
            static_assert(sizeof(offset_type) <= sizeof(threaded_rbtree_hash::size_type), "offset_type too big");
            static_assert(std::is_integral<hash_value_type>::value, "hash_value_type must be integer");
            bucket_count = 0;
            capacity = 0;
            size = 0;
            free_count = 0;
            free_list = offset_empty;
            setting_load_factor = 1;
            bucket = nullptr;
            node = nullptr;
            value = nullptr;
        }
        size_type bucket_count;
        size_type capacity;
        size_type size;
        size_type free_count;
        offset_type free_list;
        float setting_load_factor;
        offset_type *bucket;
        node_t *node;
        value_t *value;
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
    struct offset_compare
    {
        bool operator()(size_type left, size_type right) const
        {
            key_compare const &compare = *root_ptr;
            auto const &left_value = get_key_t()(*root_ptr->value[left].value());
            auto const &right_value = get_key_t()(*root_ptr->value[right].value());
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
        root_t const *root_ptr;
    };
public:
    class iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef typename threaded_rbtree_hash::value_type value_type;
        typedef typename threaded_rbtree_hash::difference_type difference_type;
        typedef typename threaded_rbtree_hash::reference reference;
        typedef typename threaded_rbtree_hash::pointer pointer;
    public:
        iterator(size_type _offset, threaded_rbtree_hash const *_self) : offset(_offset), self(_self)
        {
        }
        iterator(iterator const &) = default;
        iterator &operator++()
        {
            offset = self->advance_next_(offset);
            return *this;
        }
        iterator operator++(int)
        {
            iterator save(*this);
            ++*this;
            return save;
        }
        reference operator *() const
        {
            return *self->root_.value[offset].value();
        }
        pointer operator->() const
        {
            return self->root_.value[offset].value();
        }
        bool operator == (iterator const &other) const
        {
            return offset == other.offset && self == other.self;
        }
        bool operator != (iterator const &other) const
        {
            return offset != other.offset || self != other.self;
        }
    private:
        friend class threaded_rbtree_hash;
        size_type offset;
        threaded_rbtree_hash const *self;
    };
    class const_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef typename threaded_rbtree_hash::value_type value_type;
        typedef typename threaded_rbtree_hash::difference_type difference_type;
        typedef typename threaded_rbtree_hash::reference reference;
        typedef typename threaded_rbtree_hash::const_reference const_reference;
        typedef typename threaded_rbtree_hash::pointer pointer;
        typedef typename threaded_rbtree_hash::const_pointer const_pointer;
    public:
        const_iterator(size_type _offset, threaded_rbtree_hash const *_self) : offset(_offset), self(_self)
        {
        }
        const_iterator(const_iterator const &) = default;
        const_iterator(iterator const &it) : offset(it.offset), self(it.self)
        {
        }
        const_iterator &operator++()
        {
            offset = self->advance_next_(offset);
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator save(*this);
            ++*this;
            return save;
        }
        const_reference operator *() const
        {
            return *self->root_.value[offset].value();
        }
        const_pointer operator->() const
        {
            return self->root_.value[offset].value();
        }
        bool operator == (const_iterator const &other) const
        {
            return offset == other.offset && self == other.self;
        }
        bool operator != (const_iterator const &other) const
        {
            return offset != other.offset || self != other.self;
        }
    private:
        friend class threaded_rbtree_hash;
        size_type offset;
        threaded_rbtree_hash const *self;
    };
    class local_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef typename threaded_rbtree_hash::value_type value_type;
        typedef typename threaded_rbtree_hash::difference_type difference_type;
        typedef typename threaded_rbtree_hash::reference reference;
        typedef typename threaded_rbtree_hash::pointer pointer;
    public:
        local_iterator(size_type _offset, threaded_rbtree_hash const *_self) : offset(_offset), self(_self)
        {
        }
        local_iterator(local_iterator const &) = default;
        local_iterator &operator++()
        {
            offset = self->local_advance_next_(offset);
            return *this;
        }
        local_iterator operator++(int)
        {
            local_iterator save(*this);
            ++*this;
            return save;
        }
        reference operator *() const
        {
            return *self->root_.value[offset].value();
        }
        pointer operator->() const
        {
            return self->root_.value[offset].value();
        }
        bool operator == (local_iterator const &other) const
        {
            return offset == other.offset && self == other.self;
        }
        bool operator != (local_iterator const &other) const
        {
            return offset != other.offset || self != other.self;
        }
    private:
        friend class threaded_rbtree_hash;
        size_type offset;
        threaded_rbtree_hash const *self;
    };
    class const_local_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef typename threaded_rbtree_hash::value_type value_type;
        typedef typename threaded_rbtree_hash::difference_type difference_type;
        typedef typename threaded_rbtree_hash::reference reference;
        typedef typename threaded_rbtree_hash::const_reference const_reference;
        typedef typename threaded_rbtree_hash::pointer pointer;
        typedef typename threaded_rbtree_hash::const_pointer const_pointer;
    public:
        const_local_iterator(size_type _offset, threaded_rbtree_hash const *_self) : offset(_offset), self(_self)
        {
        }
        const_local_iterator(const_local_iterator const &) = default;
        const_local_iterator(local_iterator const &it) : offset(it.offset), self(it.self)
        {
        }
        const_local_iterator &operator++()
        {
            offset = self->local_advance_next_(offset);
            return *this;
        }
        const_local_iterator operator++(int)
        {
            const_local_iterator save(*this);
            ++*this;
            return save;
        }
        const_reference operator *() const
        {
            return *self->root_.value[offset].value();
        }
        const_pointer operator->() const
        {
            return self->root_.value[offset].value();
        }
        bool operator == (const_local_iterator const &other) const
        {
            return offset == other.offset && self == other.self;
        }
        bool operator != (const_local_iterator const &other) const
        {
            return offset != other.offset || self != other.self;
        }
    private:
        friend class threaded_rbtree_hash;
        size_type offset;
        threaded_rbtree_hash const *self;
    };
    typedef typename std::conditional<config_t::unique_type::value, std::pair<iterator, bool>, iterator>::type insert_result_t;
    typedef std::pair<iterator, bool> pair_ib_t;
protected:
    typedef std::pair<size_type, bool> pair_posi_t;
    template<class unique_type> typename std::enable_if<unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return std::make_pair(iterator(posi.first, this), posi.second);
    }
    template<class unique_type> typename std::enable_if<!unique_type::value, insert_result_t>::type result_(pair_posi_t posi)
    {
        return iterator(posi.first, this);
    }
    
public:
    //empty
    threaded_rbtree_hash() : root_(hasher(), key_compare(), allocator_type())
    {
    }
    //empty
    explicit threaded_rbtree_hash(size_type bucket_count, hasher const &hash = hasher(), key_compare const &compare = key_compare(), allocator_type const &alloc = allocator_type()) : root_(hash, compare, alloc)
    {
        rehash(bucket_count);
    }
    //empty
    explicit threaded_rbtree_hash(allocator_type const &alloc) : root_(hasher(), key_compare(), alloc)
    {
    }
    //empty
    threaded_rbtree_hash(size_type bucket_count, allocator_type const &alloc) : root_(hasher(), key_compare(), alloc)
    {
        rehash(bucket_count);
    }
    //empty
    threaded_rbtree_hash(size_type bucket_count, hasher const &hash, allocator_type const &alloc) : root_(hash, key_compare(), alloc)
    {
        rehash(bucket_count);
    }
    //range
    template <class iterator_t> threaded_rbtree_hash(iterator_t begin, iterator_t end, size_type bucket_count = 8, hasher const &hash = hasher(), key_compare const &compare = key_compare(), allocator_type const &alloc = allocator_type()) : root_(hash, compare, alloc)
    {
        rehash(bucket_count);
        insert(begin, end);
    }
    //range
    template <class iterator_t> threaded_rbtree_hash(iterator_t begin, iterator_t end, size_type bucket_count, allocator_type const &alloc) : root_(hasher(), key_compare(), alloc)
    {
        rehash(bucket_count);
        insert(begin, end);
    }
    //range
    template <class iterator_t> threaded_rbtree_hash(iterator_t begin, iterator_t end, size_type bucket_count, hasher const &hash, allocator_type const &alloc) : root_(hash, key_compare(), alloc)
    {
        rehash(bucket_count);
        insert(begin, end);
    }
    //copy
    threaded_rbtree_hash(threaded_rbtree_hash const &other) : root_(other.get_hasher(), other.get_key_equal(), other.get_value_allocator_())
    {
        copy_all_<false>(&other.root_);
    }
    //copy
    threaded_rbtree_hash(threaded_rbtree_hash const &other, allocator_type const &alloc) : root_(other.get_hasher(), other.get_key_equal(), alloc)
    {
        copy_all_<false>(&other.root_);
    }
    //move
    threaded_rbtree_hash(threaded_rbtree_hash &&other) : root_(hasher(), key_compare(), value_allocator_t())
    {
        swap(other);
    }
    //move
    threaded_rbtree_hash(threaded_rbtree_hash &&other, allocator_type const &alloc) : root_(std::move(other.get_hasher()), std::move(other.get_key_compare()), alloc)
    {
        copy_all_<true>(&other.root_);
    }
    //initializer list
    threaded_rbtree_hash(std::initializer_list<value_type> il, size_type bucket_count = 8, hasher const &hash = hasher(), key_compare const &compare = key_compare(), allocator_type const &alloc = allocator_type()) : threaded_rbtree_hash(il.begin(), il.end(), std::distance(il.begin(), il.end()), hash, compare, alloc)
    {
    }
    //initializer list
    threaded_rbtree_hash(std::initializer_list<value_type> il, size_type bucket_count, allocator_type const &alloc) : threaded_rbtree_hash(il.begin(), il.end(), std::distance(il.begin(), il.end()), alloc)
    {
    }
    //initializer list
    threaded_rbtree_hash(std::initializer_list<value_type> il, size_type bucket_count, hasher const &hash, allocator_type const &alloc) : threaded_rbtree_hash(il.begin(), il.end(), std::distance(il.begin(), il.end()), hash, alloc)
    {
    }
    //destructor
    ~threaded_rbtree_hash()
    {
        dealloc_all_();
    }
    //copy
    threaded_rbtree_hash &operator = (threaded_rbtree_hash const &other)
    {
        if(this == &other)
        {
            return *this;
        }
        dealloc_all_();
        get_hasher() = other.get_hasher();
        get_key_comp() = other.get_key_comp();
        get_bucket_allocator_() = other.get_bucket_allocator_();
        get_node_allocator_() = other.get_node_allocator_();
        get_value_allocator_() = other.get_value_allocator_();
        copy_all_<false>(&other.root_);
        return *this;
    }
    //move
    threaded_rbtree_hash &operator = (threaded_rbtree_hash &&other)
    {
        if(this == &other)
        {
            return *this;
        }
        swap(other);
        return *this;
    }
    //initializer list
    threaded_rbtree_hash &operator = (std::initializer_list<value_type> il)
    {
        clear();
        rehash(std::distance(il.begin(), il.end()));
        insert(il.begin(), il.end());
        return *this;
    }
    
    allocator_type get_allocator() const
    {
        return *static_cast<value_allocator_t const *>(&root_);
    }
    hasher hash_function() const
    {
        return *static_cast<hasher const *>(&root_);
    }
    key_compare key_comp() const
    {
        return *static_cast<key_compare const *>(&root_);
    }
    
    void swap(threaded_rbtree_hash &other)
    {
        std::swap(root_, other.root_);
    }
    
    typedef std::pair<iterator, iterator> pair_ii_t;
    typedef std::pair<const_iterator, const_iterator> pair_cici_t;
    typedef std::pair<local_iterator, local_iterator> pair_lili_t;
    typedef std::pair<const_local_iterator, const_local_iterator> pair_clicli_t;
    
    //single element
    insert_result_t insert(value_type const &value)
    {
        return result_<typename config_t::unique_type>(insert_value_(value));
    }
    //single element
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(in_value_t &&value)
    {
        return result_<typename config_t::unique_type>(insert_value_(std::forward<in_value_t>(value)));
    }
    //with hint
    iterator insert(const_iterator hint, value_type const &value)
    {
        return result_<typename config_t::unique_type>(insert_value_(value));
    }
    //with hint
    template<class in_value_t> typename std::enable_if<std::is_convertible<in_value_t, value_type>::value, insert_result_t>::type insert(const_iterator hint, in_value_t &&value)
    {
        return result_<typename config_t::unique_type>(insert_value_(std::forward<in_value_t>(value)));
    }
    //range
    template<class iterator_t> void insert(iterator_t begin, iterator_t end)
    {
        for(; begin != end; ++begin)
        {
            emplace_hint(cend(), *begin);
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
        return result_<typename config_t::unique_type>(insert_value_(std::forward<args_t>(args)...));
    }
    //with hint
    template<class ...args_t> insert_result_t emplace_hint(const_iterator hint, args_t &&...args)
    {
        return result_<typename config_t::unique_type>(insert_value_(std::forward<args_t>(args)...));
    }
    
    template<class in_key_t> iterator find(in_key_t const &key)
    {
        if(root_.size == 0)
        {
            return end();
        }
        return iterator(find_value_(key), this);
    }
    template<class in_key_t> const_iterator find(in_key_t const &key) const
    {
        if(root_.size == 0)
        {
            return cend();
        }
        return const_iterator(find_value_(key), this);
    }
    
    template<class in_key_t, class = typename std::enable_if<std::is_convertible<in_key_t, key_type>::value && config_t::unique_type::value && !std::is_same<key_type, storage_type>::value, void>::type> mapped_type &at(in_key_t const &key)
    {
        offset_type offset = root_.size;
        if(root_.size != 0)
        {
            offset = find_value_(key);
        }
        if(offset == root_.size)
        {
            throw std::out_of_range("threaded_rbtree_hash out of range");
        }
        return root_.value[offset].value()->second;
    }
    template<class in_key_t, class = typename std::enable_if<std::is_convertible<in_key_t, key_type>::value && config_t::unique_type::value && !std::is_same<key_type, storage_type>::value, void>::type> mapped_type const &at(in_key_t const &key) const
    {
        offset_type offset = root_.size;
        if(root_.size != 0)
        {
            offset = find_value_(key);
        }
        if(offset == root_.size)
        {
            throw std::out_of_range("threaded_rbtree_hash out of range");
        }
        return root_.value[offset].value()->second;
    }
    
    template<class in_key_t, class = typename std::enable_if<std::is_convertible<in_key_t, key_type>::value && config_t::unique_type::value && !std::is_same<key_type, storage_type>::value, void>::type> mapped_type &operator[](in_key_t &&key)
    {
        offset_type offset = root_.size;
        if(root_.size != 0)
        {
            offset = find_value_(key);
        }
        if(offset == root_.size)
        {
            offset = insert_value_(key, mapped_type()).first;
        }
        return root_.value[offset].value()->second;
    }
    
    iterator erase(const_iterator it)
    {
        if(root_.size == 0)
        {
            return end();
        }
        remove_offset_(it.offset);
        return iterator(advance_next_(it.offset), this);
    }
    local_iterator erase(const_local_iterator it)
    {
        if(root_.size == 0)
        {
            return local_iterator(offset_empty, this);
        }
        size_type next = local_advance_next_(offset_type(it.offset));
        remove_offset_(it.offset);
        return local_iterator(next, this);
    }
    size_type erase(key_type const &key)
    {
        if(root_.size == 0)
        {
            return 0;
        }
        return remove_value_(typename config_t::unique_type(), key);
    }
    iterator erase(const_iterator erase_begin, const_iterator erase_end)
    {
        if(erase_begin == cbegin() && erase_end == cend())
        {
            clear();
            return end();
        }
        else
        {
            while(erase_begin != erase_end)
            {
                erase(erase_begin++);
            }
            return iterator(erase_begin.offset, this);
        }
    }
    local_iterator erase(const_local_iterator erase_begin, const_local_iterator erase_end)
    {
        while(erase_begin != erase_end)
        {
            erase(erase_begin++);
        }
        return local_iterator(erase_begin.offset, this);
    }
    
    size_type count(key_type const &key) const
    {
        return find(key) == end() ? 0 : 1;
    }
    
    pair_lili_t equal_range(key_type const &key)
    {
        size_type bucket = get_hasher()(key) % root_.bucket_count;
        size_type lower, upper;
        trb_equal_range_(root_.bucket[bucket], key, lower, upper);
        return std::make_pair(local_iterator(lower, this), local_iterator(upper, this));
    }
    pair_clicli_t equal_range(key_type const &key) const
    {
        size_type bucket = get_hasher()(key) % root_.bucket_count;
        size_type lower, upper;
        trb_equal_range_(root_.bucket[bucket], key, lower, upper);
        return std::make_pair(const_local_iterator(lower, this), const_local_iterator(upper, this));
    }
    
    iterator begin()
    {
        return iterator(find_begin_(), this);
    }
    iterator end()
    {
        return iterator(root_.size, this);
    }
    const_iterator begin() const
    {
        return const_iterator(find_begin_(), this);
    }
    const_iterator end() const
    {
        return const_iterator(root_.size, this);
    }
    const_iterator cbegin() const
    {
        return const_iterator(find_begin_(), this);
    }
    const_iterator cend() const
    {
        return const_iterator(root_.size, this);
    }
    
    bool empty() const
    {
        return root_.size == root_.free_count;
    }
    void clear()
    {
        clear_all_();
    }
    size_type size() const
    {
        return root_.size - root_.free_count;
    }
    size_type max_size() const
    {
        return offset_empty - 1;
    }
    
    local_iterator begin(size_type n)
    {
        return local_iterator(root_.bucket[n], this);
    }
    local_iterator end(size_type n)
    {
        return local_iterator(offset_empty, this);
    }
    const_local_iterator begin(size_type n) const
    {
        return const_local_iterator(root_.bucket[n], this);
    }
    const_local_iterator end(size_type n) const
    {
        return const_local_iterator(offset_empty, this);
    }
    const_local_iterator cbegin(size_type n) const
    {
        return const_local_iterator(root_.bucket[n], this);
    }
    const_local_iterator cend(size_type n) const
    {
        return const_local_iterator(offset_empty, this);
    }
    
    size_type bucket_count() const
    {
        return root_.bucket_count;
    }
    size_type max_bucket_count() const
    {
        return max_size();
    }
    
    size_type bucket_size(size_type n) const
    {
        size_type step = 0;
        for(size_type i = root_.bucket[n]; i != offset_empty; i = root_.node[i].next)
        {
            ++step;
        }
        return step;
    }
    
    size_type bucket(key_type const &key) const
    {
        if(root_.size == 0)
        {
            return 0;
        }
        return hash_t(get_hasher()(key)) % root_.bucket_count;
    }
    
    void reserve(size_type count)
    {
        rehash(size_type(std::ceil(count / root_.setting_load_factor)));
    }
    void rehash(size_type count)
    {
        rehash_(typename config_t::unique_type(), std::max<size_type>({8, count, size_type(std::ceil(size() / root_.setting_load_factor))}));
    }
    
    void max_load_factor(float ml)
    {
        if(ml <= 0)
        {
            return;
        }
        root_.setting_load_factor = ml;
        if(root_.size != 0)
        {
            rehash_(typename config_t::unique_type(), size_type(std::ceil(size() / root_.setting_load_factor)));
        }
    }
    float max_load_factor() const
    {
        return root_.setting_load_factor;
    }
    float load_factor() const
    {
        if(root_.size == 0)
        {
            return 0;
        }
        return size() / float(root_.bucket_count);
    }
    
protected:
    root_t root_;
    
protected:
    
    hasher &get_hasher()
    {
        return root_;
    }
    hasher const &get_hasher() const
    {
        return root_;
    }
    
    key_compare &get_key_comp()
    {
        return root_;
    }
    key_compare const &get_key_comp() const
    {
        return root_;
    }
    
    offset_compare get_offset_comp()
    {
        return offset_compare{&root_};
    }
    offset_compare get_offset_comp() const
    {
        return offset_compare{&root_};
    }
    
    bucket_allocator_t &get_bucket_allocator_()
    {
        return root_;
    }
    bucket_allocator_t const &get_bucket_allocator_() const
    {
        return root_;
    }
    node_allocator_t &get_node_allocator_()
    {
        return root_;
    }
    node_allocator_t const &get_node_allocator_() const
    {
        return root_;
    }
    value_allocator_t &get_value_allocator_()
    {
        return root_;
    }
    value_allocator_t const &get_value_allocator_() const
    {
        return root_;
    }
    
    size_type advance_next_(size_type i) const
    {
        for(++i; i < root_.size; ++i)
        {
            if(root_.node[i].is_used())
            {
                break;
            }
        }
        return i;
    }
    
    size_type find_begin_() const
    {
        for(size_type i = 0; i < root_.size; ++i)
        {
            if(root_.node[i].is_used())
            {
                return i;
            }
        }
        return root_.size;
    }
    
    size_type local_advance_next_(size_type i) const
    {
        return trb_move_next_(i);
    }
    
    template<class iterator_t, class ...args_t> static void construct_one_(iterator_t where, args_t &&...args)
    {
        threaded_rbtree_hash_detail::construct_one(where, typename threaded_rbtree_hash_detail::get_tag<iterator_t>::type(), std::forward<args_t>(args)...);
    }
    
    template<class iterator_t> static void destroy_one_(iterator_t where)
    {
        threaded_rbtree_hash_detail::destroy_one(where, typename threaded_rbtree_hash_detail::get_tag<iterator_t>::type());
    }
    
    template<class iterator_from_t, class iterator_to_t> static void move_construct_and_destroy_(iterator_from_t move_begin, iterator_from_t move_end, iterator_to_t to_begin)
    {
        threaded_rbtree_hash_detail::move_construct_and_destroy(move_begin, move_end, to_begin, typename threaded_rbtree_hash_detail::get_tag<iterator_from_t>::type());
    }
    
    void dealloc_all_()
    {
        for(size_type i = 0; i < root_.size; ++i)
        {
            if(root_.node[i].is_used())
            {
                destroy_one_(root_.value[i].value());
            }
        }
        if(root_.bucket_count != 0)
        {
            get_bucket_allocator_().deallocate(root_.bucket, root_.bucket_count);
        }
        if(root_.capacity != 0)
        {
            get_node_allocator_().deallocate(root_.node, root_.capacity);
            get_value_allocator_().deallocate(root_.value, root_.capacity);
        }
    }
    
    void clear_all_()
    {
        for(size_type i = 0; i < root_.size; ++i)
        {
            if(root_.node[i].is_used())
            {
                destroy_one_(root_.value[i].value());
            }
        }
        if(root_.bucket_count != 0)
        {
            std::fill_n(root_.bucket, root_.bucket_count, offset_empty);
        }
        if(root_.capacity != 0)
        {
            std::memset(root_.node, 0xFFFFFFFF, sizeof(node_t) * root_.capacity);
        }
        root_.size = 0;
        root_.free_count = 0;
        root_.free_list = offset_empty;
    }
    
    template<bool move> void copy_all_(root_t const *other)
    {
        root_.bucket_count = 0;
        root_.capacity = 0;
        root_.size = 0;
        root_.free_count = 0;
        root_.free_list = offset_empty;
        root_.setting_load_factor = other->setting_load_factor;
        root_.bucket = nullptr;
        root_.node = nullptr;
        root_.value = nullptr;
        size_type size = other->size - other->free_count;
        if(size > 0)
        {
            rehash_(std::true_type(), size);
            realloc_(size);
            for(size_type other_i = 0; other_i < other->size; ++other_i)
            {
                if(other->node[other_i].is_used())
                {
                    size_type i = root_.size;
                    if(move)
                    {
                        construct_one_(root_.value[i].value(), std::move(*other->value[other_i].value()));
                    }
                    else
                    {
                        construct_one_(root_.value[i].value(), *other->value[other_i].value());
                    }
                    size_type bucket = get_hasher()(*other->value[i].value()) % root_.bucket_count;
                    stack_t stack;
                    trb_find_path_for_multi_(stack, root_.bucket[bucket], i);
                    trb_insert_(stack, root_.bucket[bucket], i);
                }
            }
        }
    }
    
    static bool is_prime_(size_type candidate)
    {
        if((candidate & 1) != 0)
        {
            size_type limit = size_type(std::sqrt(candidate));
            for(size_type divisor = 3; divisor <= limit; divisor += 2)
            {
                if((candidate % divisor) == 0)
                {
                    return false;
                }
            }
            return true;
        }
        return (candidate == 2);
    }
    
    static size_type get_prime_(size_type size)
    {
        static size_type const prime_array[] =
        {
            7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
            1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
            17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
            187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
            1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369
        };
        for(auto prime : prime_array)
        {
            if(prime >= size)
            {
                return prime;
            }
        }
        for(size_type prime = (size | 1); prime < std::numeric_limits<uint32_t>::max(); prime += 2)
        {
            if(is_prime_(prime) && ((prime - 1) % 101 != 0))
            {
                return prime;
            }
        }
        return size;
    }
    
    void rehash_(size_type size)
    {
        size = std::min(get_prime_(size), max_size());
        offset_type *new_bucket = get_bucket_allocator_().allocate(size);
        std::fill_n(new_bucket, size, offset_type(offset_empty));
        
        if(root_.bucket_count != 0)
        {
            for(size_type i = 0; i < root_.size; ++i)
            {
                if(root_.node[i].is_used())
                {
                    size_type bucket = get_hasher()(*root_.value[i].value()) % size;
                    stack_t stack;
                    trb_find_path_for_multi_(stack, new_bucket[bucket], i);
                    trb_insert_(stack, new_bucket[bucket], i);
                }
            }
            get_bucket_allocator_().deallocate(root_.bucket, root_.bucket_count);
        }
        root_.bucket_count = size;
        root_.bucket = new_bucket;
    }
    
    void realloc_(size_type size)
    {
        if(size * sizeof(value_t) > 0x1000)
        {
            size = ((size * sizeof(value_t) + std::max<size_type>(sizeof(value_t), 0x1000) - 1) & (~size_type(0) ^ 0xFFF)) / sizeof(value_t);
        }
        else if(size * sizeof(value_t) > 0x100)
        {
            size = ((size * sizeof(value_t) + std::max<size_type>(sizeof(value_t), 0x100) - 1) & (~size_type(0) ^ 0xFF)) / sizeof(value_t);
        }
        else
        {
            size = ((size * sizeof(value_t) + std::max<size_type>(sizeof(value_t), 0x10) - 1) & (~size_type(0) ^ 0xF)) / sizeof(value_t);
        }
        size = std::min(size, max_size());
        node_t *new_node = get_node_allocator_().allocate(size);
        value_t *new_value = get_value_allocator_().allocate(size);
        
        std::memset(new_node + root_.capacity, 0xFFFFFFFF, sizeof(node_t) * (size - root_.capacity));
        if(root_.capacity != 0)
        {
            std::memcpy(new_node, root_.node, sizeof(node_t) * root_.capacity);
            move_construct_and_destroy_(root_.value->value(), root_.value->value() + root_.capacity, new_value->value());
            get_node_allocator_().deallocate(root_.node, root_.capacity);
            get_value_allocator_().deallocate(root_.value, root_.capacity);
        }
        root_.capacity = size;
        root_.node = new_node;
        root_.value = new_value;
    }
    
    void check_grow_()
    {
        size_type new_size = size() + 1;
        if(new_size > root_.bucket_count * root_.setting_load_factor)
        {
            if(root_.bucket_count >= max_size())
            {
                throw std::length_error("threaded_rbtree_hash too long");
            }
            rehash_(size_type(std::ceil(root_.bucket_count * config_t::grow_proportion(root_.bucket_count))));
        }
        if(new_size > root_.capacity)
        {
            if(root_.capacity >= max_size())
            {
                throw std::length_error("threaded_rbtree_hash too long");
            }
            realloc_(size_type(std::ceil(std::max<float>(root_.capacity * config_t::grow_proportion(root_.capacity), root_.bucket_count * root_.setting_load_factor))));
        }
    }
    
    template<class ...args_t> pair_posi_t insert_value_(args_t &&...args)
    {
        check_grow_();
        return insert_value_uncheck_(typename config_t::unique_type(), std::forward<args_t>(args)...);
    }
    
    template<class in_t, class ...args_t> typename std::enable_if<std::is_same<key_type, storage_type>::value && !std::is_same<typename std::remove_reference<in_t>::type, key_type>::value, pair_posi_t>::type insert_value_uncheck_(std::true_type, in_t &&in, args_t &&...args)
    {
        key_type key = get_key_t()(in, args...);
        size_type bucket = get_hasher()(key) % root_.bucket_count;
        stack_t stack;
        if(trb_find_path_for_unique_(stack, root_.bucket[bucket], key))
        {
            return {stack.get_index(stack.height - 1), false};
        }
        size_type offset = root_.free_list == offset_empty ? root_.size : root_.free_list;
        construct_one_(root_.value[offset].value(), std::move(key));
        if(offset == root_.free_list)
        {
            root_.free_list = root_.node[offset].left_get_link();
            --root_.free_count;
        }
        else
        {
            ++root_.size;
        }
        trb_insert_(stack, root_.bucket[bucket], offset);
        return {offset, true};
    }
    template<class in_t, class ...args_t> typename std::enable_if<!std::is_same<key_type, storage_type>::value || std::is_same<typename std::remove_reference<in_t>::type, key_type>::value, pair_posi_t>::type insert_value_uncheck_(std::true_type, in_t &&in, args_t &&...args)
    {
        key_type key = get_key_t()(in, args...);
        size_type bucket = get_hasher()(key) % root_.bucket_count;
        stack_t stack;
        if(trb_find_path_for_unique_(stack, root_.bucket[bucket], key))
        {
            return {stack.get_index(stack.height - 1), false};
        }
        size_type offset = root_.free_list == offset_empty ? root_.size : root_.free_list;
        construct_one_(root_.value[offset].value(), std::forward<in_t>(in), std::forward<args_t>(args)...);
        if(offset == root_.free_list)
        {
            root_.free_list = root_.node[offset].left_get_link();
            --root_.free_count;
        }
        else
        {
            ++root_.size;
        }
        trb_insert_(stack, root_.bucket[bucket], offset);
        return {offset, true};
    }
    
    template<class in_t, class ...args_t> pair_posi_t insert_value_uncheck_(std::false_type, in_t &&in, args_t &&...args)
    {
        size_type offset = root_.free_list == offset_empty ? root_.size : root_.free_list;
        construct_one_(root_.value[offset].value(), std::forward<in_t>(in), std::forward<args_t>(args)...);
        if(offset == root_.free_list)
        {
            root_.free_list = root_.node[offset].left_get_link();
            --root_.free_count;
        }
        else
        {
            ++root_.size;
        }
        size_type bucket = get_hasher()(get_key_t()(*root_.value[offset].value())) % root_.bucket_count;
        stack_t stack;
        trb_find_path_for_multi_(stack, root_.bucket[bucket], offset);
        trb_insert_(stack, root_.bucket[bucket], offset);
        return {offset, true};
    }
    
    size_type find_value_(key_type const &key) const
    {
        size_type bucket = get_hasher()(key) % root_.bucket_count;
        size_type offset = trb_lower_bound_(root_.bucket[bucket], key);
        return (offset == offset_empty || get_key_comp()(key, get_key_t()(*root_.value[offset].value()))) ? offset_empty : offset;
    }
    
    size_type remove_value_(std::true_type, key_type const &key)
    {
        size_type bucket = get_hasher()(key) % root_.bucket_count;
        stack_t stack;
        if(trb_find_path_for_unique_(stack, root_.bucket[bucket], key))
        {
            size_type offset = stack.get_index(stack.height - 1);
            trb_remove_(stack, root_.bucket[bucket]);
            destroy_one_(root_.value[offset].value());
            root_.node[offset].set_empty();
            root_.node[offset].left_set_link(root_.free_list);
            root_.free_list = offset_type(offset);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    
    size_type remove_value_(std::false_type, key_type const &key)
    {
        size_type count = 0;
        while(remove_value_(std::true_type(), key) == 1)
        {
            ++count;
        }
        return count;
    }
    
    void remove_offset_(size_type offset)
    {
        size_type bucket = get_hasher()(get_key_t()(root_.value[offset])) % root_.bucket_count;
        stack_t stack;
        trb_find_path_for_remove_(stack, root_.bucket[bucket], offset);
        trb_remove_impl_(stack, root_.bucket[bucket], offset);
        destroy_one_(root_.value[offset].value());
        root_.node[offset].set_empty();
        root_.node[offset].left_set_link(root_.free_list);
        root_.free_list = offset_type(offset);
    }
    
    offset_type trb_move_next_(size_type node)
    {
        if(root_.node[node].right_is_thread())
        {
            return root_.node[node].right_get_link();
        }
        else
        {
            node = root_.node[node].right_get_link();
            while(root_.node[node].left_is_child())
            {
                node = root_.node[node].left_get_link();
            }
            return node;
        }
    }
    
    void trb_find_path_for_multi_(stack_t &stack, size_type root_offset, size_type index)
    {
        size_type node = root_offset;
        while(node != offset_empty)
        {
            if(get_offset_comp()(index, node))
            {
                stack.push_index(node, true);
                if(root_.node[node].left_is_thread())
                {
                    return;
                }
                node = root_.node[node].left_get_link();
            }
            else
            {
                stack.push_index(node, false);
                if(root_.node[node].right_is_thread())
                {
                    return;
                }
                node = root_.node[node].right_get_link();
            }
        }
    }
    
    bool trb_find_path_for_unique_(stack_t &stack, size_type root_offset, key_type const &key)
    {
        size_type node = root_offset;
        while(node != offset_empty)
        {
            if(get_key_comp()(key, get_key_t()(*root_.value[node].value())))
            {
                stack.push_index(node, true);
                if(root_.node[node].left_is_thread())
                {
                    return false;
                }
                node = root_.node[node].left_get_link();
            }
            else
            {
                stack.push_index(node, false);
                if(!get_key_comp()(get_key_t()(*root_.value[node].value()), key))
                {
                    return true;
                }
                if(root_.node[node].right_is_thread())
                {
                    return false;
                }
                node = root_.node[node].right_get_link();
            }
        }
        return false;
    }
    
    bool trb_find_path_for_remove_(stack_t &stack, size_type root_offset, size_type index)
    {
        size_type node = root_offset;
        while(node != offset_empty)
        {
            if(get_offset_comp()(index, node))
            {
                stack.push_index(node, true);
                if(root_.node[node].left_is_thread())
                {
                    return false;
                }
                node = root_.node[node].left_get_link();
            }
            else
            {
                stack.push_index(node, false);
                if(!get_offset_comp()(node, index))
                {
                    return true;
                }
                if(root_.node[node].right_is_thread())
                {
                    return false;
                }
                node = root_.node[node].right_get_link();
            }
        }
        return false;
    }
    
    void trb_insert_(stack_t &stack, offset_type &root_offset, size_type index)
    {
        root_.node[index].set_used();
        root_.node[index].left_set_thread();
        root_.node[index].right_set_thread();
        
        if(stack.height == 0)
        {
            root_.node[index].left_set_link(offset_empty);
            root_.node[index].right_set_link(offset_empty);
            root_.node[index].set_black();
            root_offset = index;
            return;
        }
        root_.node[index].set_red();
        size_type k = stack.height - 1;
        size_type where = stack.get_index(k);
        if(stack.is_left(k))
        {
            root_.node[index].left_set_link(root_.node[where].left_get_link());
            root_.node[index].right_set_link(where);
            root_.node[where].left_set_child();
            root_.node[where].left_set_link(index);
        }
        else
        {
            root_.node[index].right_set_link(root_.node[where].right_get_link());
            root_.node[index].left_set_link(where);
            root_.node[where].right_set_child();
            root_.node[where].right_set_link(index);
        }
        if(k >= 1)
        {
            while(root_.node[stack.get_index(k)].is_red())
            {
                size_type p2 = stack.get_index(k - 1);
                size_type p1 = stack.get_index(k);
                if(stack.is_left(k - 1))
                {
                    size_type u = root_.node[p2].right_get_link();
                    if(root_.node[p2].right_is_child() && root_.node[u].is_red())
                    {
                        root_.node[p1].set_black();
                        root_.node[u].set_black();
                        root_.node[p2].set_red();
                        if(k < 2)
                        {
                            break;
                        }
                        k -= 2;
                    }
                    else
                    {
                        size_type y;
                        if(stack.is_left(k))
                        {
                            y = p1;
                        }
                        else
                        {
                            y = root_.node[p1].right_get_link();
                            root_.node[p1].right_set_link(root_.node[y].left_get_link());
                            root_.node[y].left_set_link(p1);
                            root_.node[p2].left_set_link(y);
                            if(root_.node[y].left_is_thread())
                            {
                                root_.node[y].left_set_child();
                                root_.node[p1].right_set_thread();
                                root_.node[p1].right_set_link(y);
                            }
                        }
                        root_.node[p2].set_red();
                        root_.node[y].set_black();
                        root_.node[p2].left_set_link(root_.node[y].right_get_link());
                        root_.node[y].right_set_link(p2);
                        if(k == 1)
                        {
                            root_offset = offset_type(y);
                        }
                        else if(stack.is_left(k - 2))
                        {
                            root_.node[stack.get_index(k - 2)].left_set_link(y);
                        }
                        else
                        {
                            root_.node[stack.get_index(k - 2)].right_set_link(y);
                        }
                        if(root_.node[y].right_is_thread())
                        {
                            root_.node[y].right_set_child();
                            root_.node[p2].left_set_thread();
                            root_.node[p2].left_set_link(y);
                        }
                        break;
                    }
                }
                else
                {
                    size_type u = root_.node[p2].left_get_link();
                    if(root_.node[p2].left_is_child() && root_.node[u].is_red())
                    {
                        root_.node[p1].set_black();
                        root_.node[u].set_black();
                        root_.node[p2].set_red();
                        if(k < 2)
                        {
                            break;
                        }
                        k -= 2;
                    }
                    else
                    {
                        size_type y;
                        if(stack.is_right(k))
                        {
                            y = p1;
                        }
                        else
                        {
                            y = root_.node[p1].left_get_link();
                            root_.node[p1].left_set_link(root_.node[y].right_get_link());
                            root_.node[y].right_set_link(p1);
                            root_.node[p2].right_set_link(y);
                            if(root_.node[y].right_is_thread())
                            {
                                root_.node[y].right_set_child();
                                root_.node[p1].left_set_thread();
                                root_.node[p1].left_set_link(y);
                            }
                        }
                        root_.node[p2].set_red();
                        root_.node[y].set_black();
                        root_.node[p2].right_set_link(root_.node[y].left_get_link());
                        root_.node[y].left_set_link(p2);
                        if(k == 1)
                        {
                            root_offset = offset_type(y);
                        }
                        else if(stack.is_right(k - 2))
                        {
                            root_.node[stack.get_index(k - 2)].right_set_link(y);
                        }
                        else
                        {
                            root_.node[stack.get_index(k - 2)].left_set_link(y);
                        }
                        if(root_.node[y].left_is_thread())
                        {
                            root_.node[y].left_set_child();
                            root_.node[p2].right_set_thread();
                            root_.node[p2].right_set_link(y);
                        }
                        break;
                    }
                }
            }
        }
        root_.node[root_offset].set_black();
    }
    
    void trb_remove_(stack_t &stack, offset_type &root_offset)
    {
        size_type k = stack.height - 1;
        size_type p = stack.get_index(k);
        if(root_.node[p].right_is_thread())
        {
            if(root_.node[p].left_is_child())
            {
                size_type t = root_.node[p].left_get_link();
                while(root_.node[t].right_is_child())
                {
                    t = root_.node[t].right_get_link();
                }
                root_.node[t].right_set_link(root_.node[p].right_get_link());
                if(k == 0)
                {
                    root_offset = offset_type(root_.node[p].left_get_link());
                }
                else if(stack.is_left(k - 1))
                {
                    root_.node[stack.get_index(k - 1)].left_set_link(root_.node[p].left_get_link());
                }
                else
                {
                    root_.node[stack.get_index(k - 1)].right_set_link(root_.node[p].left_get_link());
                }
            }
            else
            {
                if(k == 0)
                {
                    root_offset = offset_type(root_.node[p].left_get_link());
                }
                else if(stack.is_left(k - 1))
                {
                    root_.node[stack.get_index(k - 1)].left_set_link(root_.node[p].left_get_link());
                    root_.node[stack.get_index(k - 1)].left_set_thread();
                }
                else
                {
                    root_.node[stack.get_index(k - 1)].right_set_link(root_.node[p].right_get_link());
                    root_.node[stack.get_index(k - 1)].right_set_thread();
                }
            }
        }
        else
        {
            size_type r = root_.node[p].right_get_link();
            if(root_.node[r].left_is_thread())
            {
                root_.node[r].left_set_link(root_.node[p].left_get_link());
                if(root_.node[p].left_is_child())
                {
                    root_.node[r].left_set_child();
                    size_type t = root_.node[p].left_get_link();
                    while(root_.node[t].right_is_child())
                    {
                        t = root_.node[t].right_get_link();
                    }
                    root_.node[t].right_set_link(r);
                }
                else
                {
                    root_.node[r].left_set_thread();
                }
                if(k == 0)
                {
                    root_offset = offset_type(r);
                }
                else if(stack.is_left(k - 1))
                {
                    root_.node[stack.get_index(k - 1)].left_set_link(r);
                }
                else
                {
                    root_.node[stack.get_index(k - 1)].right_set_link(r);
                }
                bool is_red = root_.node[r].is_red();
                if(root_.node[p].is_red())
                {
                    root_.node[r].set_red();
                }
                else
                {
                    root_.node[r].set_black();
                }
                if(is_red)
                {
                    root_.node[p].set_red();
                }
                else
                {
                    root_.node[p].set_black();
                }
                stack.update_index(k, r, false);
                ++k;
            }
            else
            {
                size_type s;
                size_t const j = stack.height - 1;
                for(++k; ; )
                {
                    stack.update_index(k, r, true);
                    ++k;
                    s = root_.node[r].left_get_link();
                    if(root_.node[s].left_is_thread())
                    {
                        break;
                    }
                    r = s;
                }
                stack.update_index(j, s, false);
                if(root_.node[s].right_is_child())
                {
                    root_.node[r].left_set_link(root_.node[s].right_get_link());
                }
                else
                {
                    root_.node[r].left_set_thread();
                }
                root_.node[s].left_set_link(root_.node[p].left_get_link());
                if(root_.node[p].left_is_child())
                {
                    size_type t = root_.node[p].left_get_link();
                    while(root_.node[t].right_is_child())
                    {
                        t = root_.node[t].right_get_link();
                    }
                    root_.node[t].right_set_link(s);
                    root_.node[s].left_set_child();
                }
                root_.node[s].right_set_link(root_.node[p].right_get_link());
                root_.node[s].right_set_child();
                bool is_red = root_.node[p].is_red();
                if(root_.node[s].is_red())
                {
                    root_.node[p].set_red();
                }
                else
                {
                    root_.node[p].set_black();
                }
                if(is_red)
                {
                    root_.node[s].set_red();
                }
                else
                {
                    root_.node[s].set_black();
                }
                if(j == 0)
                {
                    root_offset = offset_type(s);
                }
                else if(stack.is_left(j - 1))
                {
                    root_.node[stack.get_index(j - 1)].left_set_link(s);
                }
                else
                {
                    root_.node[stack.get_index(j - 1)].right_set_link(s);
                }
            }
        }
        if(root_.node[p].is_black())
        {
            for(; k > 1; --k)
            {
                if(stack.is_left(k - 1))
                {
                    if(root_.node[stack.get_index(k - 1)].left_is_child())
                    {
                        size_type x = root_.node[stack.get_index(k - 1)].left_get_link();
                        if(root_.node[x].is_red())
                        {
                            root_.node[x].set_black();
                            break;
                        }
                    }
                    size_type w = root_.node[stack.get_index(k - 1)].right_get_link();
                    if(root_.node[w].is_red())
                    {
                        root_.node[w].set_black();
                        root_.node[stack.get_index(k - 1)].set_red();
                        root_.node[stack.get_index(k - 1)].right_set_link(root_.node[w].left_get_link());
                        root_.node[w].left_set_link(stack.get_index(k - 1));
                        if(k == 1)
                        {
                            root_offset = offset_type(w);
                        }
                        else if(stack.is_left(k - 2))
                        {
                            root_.node[stack.get_index(k - 2)].left_set_link(w);
                        }
                        else
                        {
                            root_.node[stack.get_index(k - 2)].right_set_link(w);
                        }
                        stack.update_index(k, stack.get_index(k - 1), true);
                        stack.update_index(k - 1, w);
                        w = root_.node[stack.get_index(k)].right_get_link();
                        ++k;
                    }
                    if((root_.node[w].left_is_thread() || root_.node[root_.node[w].left_get_link()].is_black()) && (root_.node[w].right_is_thread() || root_.node[root_.node[w].right_get_link()].is_black()))
                    {
                        root_.node[w].set_red();
                    }
                    else
                    {
                        if(root_.node[w].right_is_thread() || root_.node[root_.node[w].right_get_link()].is_black())
                        {
                            size_type y = root_.node[w].left_get_link();
                            root_.node[y].set_black();
                            root_.node[w].set_red();
                            root_.node[w].left_set_link(root_.node[y].right_get_link());
                            root_.node[y].right_set_link(w);
                            root_.node[stack.get_index(k - 1)].right_set_link(y);
                            if(root_.node[y].right_is_thread())
                            {
                                size_type z = root_.node[y].right_get_link();
                                root_.node[y].right_set_child();
                                root_.node[z].left_set_thread();
                                root_.node[z].left_set_link(y);
                            }
                            w = y;
                        }
                        if(root_.node[stack.get_index(k - 1)].is_red())
                        {
                            root_.node[w].set_red();
                        }
                        else
                        {
                            root_.node[w].set_black();
                        }
                        root_.node[stack.get_index(k - 1)].set_black();
                        root_.node[root_.node[w].right_get_link()].set_black();
                        root_.node[stack.get_index(k - 1)].right_set_link(root_.node[w].left_get_link());
                        root_.node[w].left_set_link(stack.get_index(k - 1));
                        if(k == 1)
                        {
                            root_offset = offset_type(w);
                        }
                        else if(stack.is_left(k - 2))
                        {
                            root_.node[stack.get_index(k - 2)].left_set_link(w);
                        }
                        else
                        {
                            root_.node[stack.get_index(k - 2)].right_set_link(w);
                        }
                        if(root_.node[w].left_is_thread())
                        {
                            root_.node[w].left_set_child();
                            root_.node[stack.get_index(k - 1)].right_set_thread();
                            root_.node[stack.get_index(k - 1)].right_set_link(w);
                        }
                        break;
                    }
                }
                else
                {
                    if(root_.node[stack.get_index(k - 1)].right_is_child())
                    {
                        size_type x = root_.node[stack.get_index(k - 1)].right_get_link();
                        if(root_.node[x].is_red())
                        {
                            root_.node[x].set_black();
                            break;
                        }
                    }
                    size_type w = root_.node[stack.get_index(k - 1)].left_get_link();
                    if(root_.node[w].is_red())
                    {
                        root_.node[w].set_black();
                        root_.node[stack.get_index(k - 1)].set_red();
                        root_.node[stack.get_index(k - 1)].left_set_link(root_.node[w].right_get_link());
                        root_.node[w].right_set_link(stack.get_index(k - 1));
                        if(k == 1)
                        {
                            root_offset = offset_type(w);
                        }
                        else if(stack.is_right(k - 2))
                        {
                            root_.node[stack.get_index(k - 2)].right_set_link(w);
                        }
                        else
                        {
                            root_.node[stack.get_index(k - 2)].left_set_link(w);
                        }
                        stack.update_index(k, stack.get_index(k - 1), false);
                        stack.update_index(k - 1, w);
                        w = root_.node[stack.get_index(k)].left_get_link();
                        ++k;
                    }
                    if((root_.node[w].right_is_thread() || root_.node[root_.node[w].right_get_link()].is_black()) && (root_.node[w].left_is_thread() || root_.node[root_.node[w].left_get_link()].is_black()))
                    {
                        root_.node[w].set_red();
                    }
                    else
                    {
                        if(root_.node[w].left_is_thread() || root_.node[root_.node[w].left_get_link()].is_black())
                        {
                            size_type y = root_.node[w].right_get_link();
                            root_.node[y].set_black();
                            root_.node[w].set_red();
                            root_.node[w].right_set_link(root_.node[y].left_get_link());
                            root_.node[y].left_set_link(w);
                            root_.node[stack.get_index(k - 1)].left_set_link(y);
                            if(root_.node[y].left_is_thread())
                            {
                                size_type z = root_.node[y].left_get_link();
                                root_.node[y].left_set_child();
                                root_.node[z].right_set_thread();
                                root_.node[z].right_set_link(y);
                            }
                            w = y;
                        }
                        if(root_.node[stack.get_index(k - 1)].is_red())
                        {
                            root_.node[w].set_red();
                        }
                        else
                        {
                            root_.node[w].set_black();
                        }
                        root_.node[stack.get_index(k - 1)].set_black();
                        root_.node[root_.node[w].left_get_link()].set_black();
                        root_.node[stack.get_index(k - 1)].left_set_link(root_.node[w].right_get_link());
                        root_.node[w].right_set_link(stack.get_index(k - 1));
                        if(k == 1)
                        {
                            root_offset = offset_type(w);
                        }
                        else if(stack.is_right(k - 2))
                        {
                            root_.node[stack.get_index(k - 2)].right_set_link(w);
                        }
                        else
                        {
                            root_.node[stack.get_index(k - 2)].left_set_link(w);
                        }
                        if(root_.node[w].right_is_thread())
                        {
                            root_.node[w].right_set_child();
                            root_.node[stack.get_index(k - 1)].left_set_thread();
                            root_.node[stack.get_index(k - 1)].left_set_link(w);
                        }
                        break;
                    }
                }
            }
            if(root_offset != offset_empty)
            {
                root_.node[root_offset].set_black();
            }
        }
    }
    
    
    size_type trb_lower_bound_(size_type root_offset, key_type const &key) const
    {
        size_type node = root_offset, where = offset_empty;
        while(node != offset_empty)
        {
            if(get_key_comp()(get_key_t()(*root_.value[node].value()), key))
            {
                if(root_.node[node].right_is_thread())
                {
                    break;
                }
                node = root_.node[node].right_get_link();
            }
            else
            {
                where = node;
                if(root_.node[node].left_is_thread())
                {
                    break;
                }
                node = root_.node[node].left_get_link();
            }
        }
        return where;
    }
    
    void trb_equal_range_(size_type root_offset, key_type const &key, size_type &lower, size_type &upper) const
    {
        size_type node = root_offset;
        lower = offset_empty;
        upper = offset_empty;
        while(node != offset_empty)
        {
            if(get_key_comp()(get_key_t()(*root_.value[node].value()), key))
            {
                if(root_.node[node].right_is_thread())
                {
                    break;
                }
                node = root_.node[node].right_get_link();
            }
            else
            {
                if(upper == offset_empty && get_key_comp()(key, get_key_t()(*root_.value[node].value())))
                {
                    upper = node;
                }
                lower = node;
                if(root_.node[node].left_is_thread())
                {
                    break;
                }
                node = root_.node[node].left_get_link();
            }
        }
        node = upper == offset_empty ? root_offset : root_.node[upper].left_is_child() ? root_.node[upper].left_get_link() : offset_empty;
        while(node != offset_empty)
        {
            if(get_key_comp()(key, get_key_t()(*root_.value[node].value())))
            {
                upper = node;
                if(root_.node[node].left_is_thread())
                {
                    break;
                }
                node = root_.node[node].left_get_link();
            }
            else
            {
                if(root_.node[node].right_is_thread())
                {
                    break;
                }
                node = root_.node[node].right_get_link();
            }
        }
    }

};


template<class key_t, class value_t, class unique_t, class hasher_t, class key_compare_t, class allocator_t>
struct threaded_rbtree_hash_map_config_t
{
    typedef key_t key_type;
    typedef value_t mapped_type;
    typedef std::pair<key_t const, value_t> value_type;
    typedef std::pair<key_t, value_t> storage_type;
    typedef hasher_t hasher;
    typedef key_compare_t key_compare;
    typedef allocator_t allocator_type;
    typedef std::uintptr_t offset_type;
    typedef typename std::result_of<hasher(key_type)>::type hash_value_type;
    typedef unique_t unique_type;
    static float grow_proportion(std::size_t)
    {
        return 2;
    }
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value.first;
    }
};
template<class key_t, class value_t, class hasher_t = std::hash<key_t>, class key_compare_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using trb_hash_map = threaded_rbtree_hash<threaded_rbtree_hash_map_config_t<key_t, value_t, std::true_type, hasher_t, key_compare_t, allocator_t>>;

template<class key_t, class value_t, class hasher_t = std::hash<key_t>, class key_compare_t = std::less<key_t>, class allocator_t = std::allocator<std::pair<key_t const, value_t>>>
using trb_hash_multimap = threaded_rbtree_hash<threaded_rbtree_hash_map_config_t<key_t, value_t, std::false_type, hasher_t, key_compare_t, allocator_t>>;


template<class key_t, class unique_t, class hasher_t, class key_compare_t, class allocator_t>
struct threaded_rbtree_hash_set_config_t
{
    typedef key_t key_type;
    typedef key_t const mapped_type;
    typedef key_t const value_type;
    typedef key_t storage_type;
    typedef hasher_t hasher;
    typedef key_compare_t key_compare;
    typedef allocator_t allocator_type;
    typedef std::uintptr_t offset_type;
    typedef typename std::result_of<hasher(key_type)>::type hash_value_type;
    typedef unique_t unique_type;
    static float grow_proportion(std::size_t)
    {
        return 2;
    }
    template<class in_type> static key_type const &get_key(in_type &&value)
    {
        return value;
    }
};
template<class key_t, class hasher_t = std::hash<key_t>, class key_compare_t = std::less<key_t>, class allocator_t = std::allocator<key_t>>
using trb_hash_set = threaded_rbtree_hash<threaded_rbtree_hash_set_config_t<key_t, std::true_type, hasher_t, key_compare_t, allocator_t>>;

template<class key_t, class hasher_t = std::hash<key_t>, class key_compare_t = std::less<key_t>, class allocator_t = std::allocator<key_t>>
using trb_hash_multiset = threaded_rbtree_hash<threaded_rbtree_hash_set_config_t<key_t, std::false_type, hasher_t, key_compare_t, allocator_t>>;







