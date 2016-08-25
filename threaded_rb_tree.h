
#include <stdexcept>
#include <cstdint>
#include <cassert>



//
//
//
//
//#if defined(_NO_SUPPORT_EMBEDED_LINK)
//#	define TrbNode(p) p
//#	define TRB_define_node_offset
//#	define TRB_root_as_node(tree)  &tree->trb_root
//#	define TRB_revert_offset(node) node
//#else
//#	define TrbNode(p) ((struct trb_node*)((char*)(p) + node_offset))
//#	define TRB_define_node_offset  intptr_t node_offset = vtab->node_offset;
//#	define TRB_root_as_node(tree)  ((char*)(&tree->trb_root) - node_offset)
//#	define TRB_revert_offset(node) (struct trb_node*)((char*)(node) - node_offset)
//#endif
//
//#define TRB_is_red(p)     (TrbNode(p)->trb_link[0] & 1)
//#define TRB_is_black(p)  !(TrbNode(p)->trb_link[0] & 1)
//#define TRB_set_red(p)    TrbNode(p)->trb_link[0] |=  1
//#define TRB_set_black(p)  TrbNode(p)->trb_link[0] &= ~1
//
//#define TRB_copy_color(p,q) TrbNode(p)->trb_link[0] = (TrbNode(p)->trb_link[0] & ~1) | (TrbNode(q)->trb_link[0] & 1)
//#define TRB_swap_color(p,q) { \
//intptr_t t = TrbNode(p)->trb_link[0] & 1; \
//TRB_copy_color(p, q); \
//TrbNode(q)->trb_link[0] = (TrbNode(q)->trb_link[0] & ~1) | t; \
//}
////~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//#define TRB_get_link(p,idx) ((struct trb_node*)(TrbNode(p)->trb_link[idx] & ~3))
//#define TRB_set_link(p,idx,q)  TrbNode(p)->trb_link[idx] = (intptr_t)(q) | (TrbNode(p)->trb_link[idx] & 3)
//
//#define TAG_is_thread(p,idx)  (TrbNode(p)->trb_link[idx] & 2)
//#define TAG_is_child(p,idx)  !(TrbNode(p)->trb_link[idx] & 2)
//
//#define TRB_self_is_thread(p)   (p & 2)
//#define TRB_self_is_child(p)   !(p & 2)
//#define TRB_self_get_ptr(p)     ((struct trb_node*)(p & ~3))
//
//#define TAG_copy(p,ip,q,iq)    TrbNode(p)->trb_link[ip] = (TrbNode(p)->trb_link[ip] & ~2) | (TrbNode(q)->trb_link[iq] & 2)
//
//#define TAG_set_thread(p,idx)  TrbNode(p)->trb_link[idx] |= 2
//#define TAG_set_child(p,idx)   TrbNode(p)->trb_link[idx] &= ~2
//
//#define TRB_raw_link(p,idx)  TrbNode(p)->trb_link[idx]
//
///* Maximum TRB height. */
//#ifndef TRB_MAX_HEIGHT
//#define TRB_MAX_HEIGHT 80
//#endif
//
//#define TRB_GET_DATA_SIZE(vtab, data)  (vtab->pf_data_get_size ? vtab->pf_data_get_size(vtab, data) : (size_t)vtab->data_size)
//
//struct trb_path_arg
//{
//    int height;                          /* Stack height. */
//    int hlow;							 /* Equal Key Node */
//#if 0 // defined(_DEBUG) || !defined(NDEBUG)
//    struct trb_node* pa[TRB_MAX_HEIGHT]; /* Nodes on stack. */
//    unsigned char da[TRB_MAX_HEIGHT];    /* Directions moved from stack nodes. */
//#	define TPA_dir(i) arg->da[i]
//#	define TPA_ptr(i) arg->pa[i]
//#	define TPA_set(i,ptr,dir) arg->pa[i] = (struct trb_node*)(ptr), arg->da[i] = dir
//#	define TPA_setptr(i,ptr)  arg->pa[i] = ptr
//#	define TPA_lower arg.pa[arg.hlow]
//#else
//    intptr_t pa[TRB_MAX_HEIGHT];
//#	define TPA_dir(i)                    (arg->pa[i] &  1)
//#	define TPA_ptr(i) ((struct trb_node*)(arg->pa[i] & ~1))
//#	define TPA_set(i,ptr,dir) arg->pa[i] = (intptr_t)(ptr) | dir
//#	define TPA_setptr(i,ptr)  arg->pa[i] = (intptr_t)(ptr) | (arg->pa[i] & 1)
//#	define TPA_lower ((struct trb_node*)(arg.pa[arg.hlow] & ~1))
//#endif
//};
///////////////////////////////////////////////////////////////////////////////////////////////
//
//
//void
//trb_init(struct trb_tree* tree)
//{
//    tree->trb_root = 0;
//    tree->trb_count = 0;
//}
//
///* Begin rotate */
//
//
//static
//void
//trb_rotate_for_insert(const struct trb_vtab* vtab,
//                      volatile struct trb_tree* tree,
//                      struct trb_node* n, ///< new inserted node
//                      struct trb_path_arg* arg)
//{
//    TRB_define_node_offset
//    int k = arg->height;
//    int dir = TPA_dir(k-1);
//    struct trb_node* p = TPA_ptr(k-1);
//    int doff = vtab->data_offset;
//
//    assert(arg->hlow <= k-1);
//    /*
//     *    p                    p
//     *            ==>          |
//     *                         n
//     */
//    TAG_set_thread(n, 0);
//    TAG_set_thread(n, 1);
//    TRB_set_link(n, dir, TRB_get_link(p, dir)); // thread
//    
//    if (nark_likely(tree->trb_root != NULL)) {
//        TAG_set_child(p, dir);
//        TRB_set_link(n, !dir, p); // thread ptr
//    } else
//        TRB_set_link(n, 1, NULL); // thread ptr
//    
//    assert(TAG_is_child(p, dir));
//    TRB_set_link(p, dir, n);
//    TRB_set_red(n);
//    
//    while (k >= 3 && TRB_is_red(TPA_ptr(k-1)))
//    {
//        struct trb_node* p3 = TPA_ptr(k-3);
//        struct trb_node* p2 = TPA_ptr(k-2);
//        struct trb_node* p1 = TPA_ptr(k-1);
//        
//        if (TPA_dir(k-2) == 0)
//        {
//            /*
//             *		p1 == p2->left
//             *		u  == p2->right
//             */
//            struct trb_node* u = TRB_get_link(p2, 1);
//            if (TAG_is_child(p2, 1) && TRB_is_red(u))
//            { // p2->right != NULL && u->color == red, if ignore thread
//                TRB_set_black(p1);
//                TRB_set_black(u);
//                TRB_set_red(p2);
//                k -= 2;
//            }
//            else // p2->right == NULL || u->color == red, if ignore thread
//            {
//                struct trb_node *y;
//                
//                if (TPA_dir(k-1) == 0) // p == p1->left, if ignore thread
//                    y = p1;
//                else
//                {  //    p == p1->right
//                    /*
//                     *       p3                          p3
//                     *       |                           |
//                     *       p2                          p2
//                     *      /  \          ==>           /  \
//                     *     p1   u                      y    u
//                     *    /  \                        / \
//                     *   ?    y                      p1  Y1
//                     *       / \                    /  \
//                     *     Y0   Y1                 ?    Y0
//                     */
//                    y = TRB_get_link(p1, 1);
//                    TRB_set_link(p1, 1, TRB_get_link(y, 0));
//                    TRB_set_link(y , 0, p1);
//                    TRB_set_link(p2, 0, y);
//                    
//                    if (TAG_is_thread(y, 0))
//                    {
//                        TAG_set_child(y, 0);
//                        TAG_set_thread(p1, 1);
//                        TRB_set_link(p1, 1, y);
//                    }
//                }
//                /*
//                 *               p3                            p3
//                 *               |                             |
//                 *               p2                            y
//                 *              /  \              ==>       /     \
//                 *             y    u                      p1      p2
//                 *            / \                         /  \    /  \
//                 *           p1  Y1                      n?  Y0  Y1   u
//                 *          /  \
//                 *         n?  Y0
//                 */
//                TRB_set_red(p2);
//                TRB_set_black(y);
//                
//                TRB_set_link(p2, 0, TRB_get_link(y, 1));
//                TRB_set_link(y, 1, p2);
//                TRB_set_link(p3, TPA_dir(k-3), y);
//                
//                if (TAG_is_thread(y, 1))
//                {
//                    TAG_set_child(y, 1);
//                    TAG_set_thread(p2, 0);
//                    TRB_set_link(p2, 0, y);
//                }
//                assert(TRB_get_link(p2, 1) == u);
//                break;
//            }
//        }
//        else
//        {
//            /*
//             *	p1 == p2->right
//             *	u  == p2->left
//             */
//            struct trb_node* u = TRB_get_link(p2, 0);
//            
//            if (TAG_is_child(p2, 0) && TRB_is_red(u))
//            {
//                TRB_set_black(p1);
//                TRB_set_black(u);
//                TRB_set_red(p2);
//                k -= 2;
//            }
//            else {
//                struct trb_node *y;
//                
//                if (TPA_dir(k-1) == 1)
//                    y = p1;
//                else
//                {
//                    /*
//                     *       p3                                  p3
//                     *       |                                   |
//                     *       p2                                  p2
//                     *      /  \           ==>                  /  \
//                     *     u   p1                              u    y
//                     *        /  \                                 /  \
//                     *       y    ?                               Y0  p1
//                     *      / \                                      /  \
//                     *     Y0 Y1                                    Y1   ?
//                     */
//                    y = TRB_get_link(p1, 0);
//                    TRB_set_link(p1, 0, TRB_get_link(y, 1));
//                    TRB_set_link(y , 1, p1);
//                    TRB_set_link(p2, 1, y);
//                    
//                    if (TAG_is_thread(y, 1))
//                    {
//                        TAG_set_child(y, 1);
//                        TAG_set_thread(p1, 0);
//                        TRB_set_link(p1, 0, y);
//                    }
//                }
//                /*
//                 *        p3                                       p3
//                 *        |                                        |
//                 *        p2                                     _.y\._
//                 *       /  \              ==>                  /      \
//                 *      u    y\                                p2      p1
//                 *          / \\                              /  \     / \
//                 *         Y0  p1                            u   Y0   Y1  ?
//                 *            /  \
//                 *           Y1   ?
//                 */
//                TRB_set_red(p2);
//                TRB_set_black(y);
//                
//                TRB_set_link(p2, 1, TRB_get_link(y, 0));
//                TRB_set_link(y, 0, p2);
//                TRB_set_link(p3, TPA_dir(k-3), y);
//                
//                if (TAG_is_thread(y, 0))
//                {
//                    TAG_set_child(y, 0);
//                    TAG_set_thread(p2, 1);
//                    TRB_set_link(p2, 1, y);
//                }
//                break;
//            }
//        }
//    }
//    TRB_set_black(tree->trb_root);
//}
//
//static
//void
//trb_rotate_for_remove(const struct trb_vtab* vtab, volatile struct trb_tree* tree, struct trb_path_arg* arg)
//{
//    TRB_define_node_offset
//    int k = arg->hlow;
//    int doff = vtab->data_offset;
//    struct trb_node* p = TPA_ptr(k);
//    assert(k >= 1);
//    
//    if (TAG_is_thread(p, 1))
//    {
//        if (TAG_is_child(p, 0))
//        {
//            struct trb_node* t = TRB_get_link(p, 0);
//            
//            while (TAG_is_child(t, 1))
//                t = TRB_get_link(t, 1);
//            TRB_set_link(t, 1, TRB_get_link(p, 1)); // t.next = p.next
//            TRB_set_link(TPA_ptr(k-1), TPA_dir(k-1), TRB_get_link(p, 0));
//            DBG_trb_check_all();
//        }
//        else // p.link[0,1] are all threads, p is a leaf
//        {
//            TRB_set_link(TPA_ptr(k-1), TPA_dir(k-1), TRB_get_link(p, TPA_dir(k-1)));
//            if (TPA_ptr(k-1) != (struct trb_node *)TRB_root_as_node(tree))
//                TAG_set_thread(TPA_ptr(k-1), TPA_dir(k-1));
//            DBG_trb_check_all();
//        }
//    }
//    else // p.link[1] is child
//    {
//        struct trb_node* r = TRB_get_link(p, 1);
//        
//        if (TAG_is_thread(r, 0))
//        {
//            TRB_set_link(r, 0, TRB_get_link(p, 0));
//            TAG_copy(r, 0, p, 0);
//            if (TAG_is_child(p, 0))
//            {
//                struct trb_node* t = TRB_get_link(p, 0);
//                while (TAG_is_child(t, 1))
//                    t = TRB_get_link(t, 1);
//                TRB_set_link(t, 1, r); // thread t.next to r
//            }
//            TRB_set_link(TPA_ptr(k-1), TPA_dir(k-1), r);
//            TRB_swap_color(r, p);
//            TPA_set(k, r, 1);
//            k++;
//            DBG_trb_check_all();
//        }
//        else // is_child(r, 0)
//        {
//            struct trb_node* s;
//            const int j = arg->hlow;
//            
//            for (++k;;)
//            {
//                TPA_set(k, r, 0);
//                k++;
//                s = TRB_get_link(r, 0);
//                if (TAG_is_thread(s, 0))
//                    break;
//                
//                r = s;
//            }
//            //	assert(TAG_is_thread(s, 0)); // always true
//            //	assert(TAG_is_child (r, 0));
//            //  assert(TRB_get_link (r, 0) == s);
//            //  r is parent of left most of p.link[1]
//            assert(TPA_ptr(j) == p);
//            assert(j == arg->hlow);
//            TPA_set(j, s, 1);
//            if (TAG_is_child(s, 1)) {
//                TRB_set_link(r, 0, TRB_get_link(s, 1));
//            }
//            else
//            {
//                assert(TRB_get_link(r, 0) == s);
//                //	TRB_set_link(r, 0, s);
//                TAG_set_thread(r, 0);
//            }
//            
//            TRB_set_link(s, 0, TRB_get_link(p, 0));
//            if (TAG_is_child(p, 0))
//            {
//                struct trb_node* t = TRB_get_link(p, 0);
//                while (TAG_is_child(t, 1))
//                    t = TRB_get_link(t, 1);
//                TRB_set_link(t, 1, s); // thread t.next to s
//                
//                TAG_set_child(s, 0);
//            }
//            
//            TRB_set_link(s, 1, TRB_get_link(p, 1));
//            TAG_set_child(s, 1);
//            
//            TRB_swap_color(s, p);
//            TRB_set_link(TPA_ptr(j-1), TPA_dir(j-1), s);
//        }
//    }
//    
//    if (TRB_is_black(p)) // 'p' will not be used in this block
//    {
//        for (; k > 1; k--)
//        {
//            if (TAG_is_child(TPA_ptr(k-1), TPA_dir(k-1)))
//            {
//                struct trb_node* x = TRB_get_link(TPA_ptr(k-1), TPA_dir(k-1));
//                if (TRB_is_red(x))
//                {
//                    TRB_set_black(x);
//                    break;
//                }
//            }
//            
//            if (TPA_dir(k-1) == 0)
//            {
//                struct trb_node* w = TRB_get_link(TPA_ptr(k-1), 1);
//                
//                if (TRB_is_red(w))
//                {
//                    TRB_set_black(w);
//                    TRB_set_red(TPA_ptr(k-1));
//                    
//                    TRB_set_link(TPA_ptr(k-1), 1, TRB_get_link(w, 0));
//                    TRB_set_link(w, 0, TPA_ptr(k-1));
//                    TRB_set_link(TPA_ptr(k-2), TPA_dir(k-2), w);
//                    TPA_set(k, TPA_ptr(k-1), 0);
//                    TPA_setptr(k-1, w);
//                    k++;
//                    
//                    w = TRB_get_link(TPA_ptr(k-1), 1);
//                }
//                if ( (TAG_is_thread(w, 0) || TRB_is_black(TRB_get_link(w, 0))) &&
//                    (TAG_is_thread(w, 1) || TRB_is_black(TRB_get_link(w, 1))) )
//                {
//                    TRB_set_red(w);
//                }
//                else
//                {
//                    if (TAG_is_thread(w, 1) || TRB_is_black(TRB_get_link(w, 1)))
//                    {
//                        struct trb_node* y = TRB_get_link(w, 0);
//                        TRB_set_black(y);
//                        TRB_set_red(w);
//                        TRB_set_link(w, 0, TRB_get_link(y, 1));
//                        TRB_set_link(y, 1, w);
//                        TRB_set_link(TPA_ptr(k-1), 1, y);
//                        
//                        if (TAG_is_thread(y, 1))
//                        {
//                            struct trb_node* z = TRB_get_link(y, 1);
//                            TAG_set_child(y, 1);
//                            TAG_set_thread(z, 0);
//                            TRB_set_link(z, 0, y);
//                        }
//                        w = y;
//                    }
//                    
//                    TRB_copy_color(w, TPA_ptr(k-1));
//                    TRB_set_black(TPA_ptr(k-1));
//                    TRB_set_black(TRB_get_link(w, 1));
//                    
//                    TRB_set_link(TPA_ptr(k-1), 1, TRB_get_link(w, 0));
//                    TRB_set_link(w, 0, TPA_ptr(k-1));
//                    TRB_set_link(TPA_ptr(k-2), TPA_dir(k-2), w);
//                    
//                    if (TAG_is_thread(w, 0))
//                    {
//                        TAG_set_child(w, 0);
//                        TAG_set_thread(TPA_ptr(k-1), 1);
//                        TRB_set_link(TPA_ptr(k-1), 1, w);
//                    }
//                    break;
//                }
//            }
//            else
//            {
//                struct trb_node* w = TRB_get_link(TPA_ptr(k-1), 0);
//                
//                if (TRB_is_red(w))
//                {
//                    TRB_set_black(w);
//                    TRB_set_red(TPA_ptr(k-1));
//                    
//                    TRB_set_link(TPA_ptr(k-1), 0, TRB_get_link(w, 1));
//                    TRB_set_link(w, 1, TPA_ptr(k-1));
//                    TRB_set_link(TPA_ptr(k-2), TPA_dir(k-2), w);
//                    TPA_set(k, TPA_ptr(k-1), 1);
//                    TPA_setptr(k-1, w);
//                    k++;
//                    
//                    w = TRB_get_link(TPA_ptr(k-1), 0);
//                }
//                
//                if ( (TAG_is_thread(w, 0) || TRB_is_black(TRB_get_link(w, 0))) &&
//                    (TAG_is_thread(w, 1) || TRB_is_black(TRB_get_link(w, 1))) )
//                {
//                    TRB_set_red(w);
//                }
//                else
//                {
//                    if (TAG_is_thread(w, 0) || TRB_is_black(TRB_get_link(w, 0)))
//                    {
//                        struct trb_node* y = TRB_get_link(w, 1);
//                        TRB_set_black(y);
//                        TRB_set_red(w);
//                        TRB_set_link(w, 1, TRB_get_link(y, 0));
//                        TRB_set_link(y, 0, w);
//                        TRB_set_link(TPA_ptr(k-1), 0, y);
//                        
//                        if (TAG_is_thread(y, 0))
//                        {
//                            struct trb_node* z = TRB_get_link(y, 0);
//                            TAG_set_child(y, 0);
//                            TAG_set_thread(z, 1);
//                            TRB_set_link(z, 1, y);
//                        }
//                        w = y;
//                    }
//                    
//                    TRB_copy_color(w, TPA_ptr(k-1));
//                    TRB_set_black(TPA_ptr(k-1));
//                    TRB_set_black(TRB_get_link(w, 0));
//                    
//                    TRB_set_link(TPA_ptr(k-1), 0, TRB_get_link(w, 1));
//                    TRB_set_link(w, 1, TPA_ptr(k-1));
//                    TRB_set_link(TPA_ptr(k-2), TPA_dir(k-2), w);
//                    if (TAG_is_thread(w, 1))
//                    {
//                        TAG_set_child(w, 1);
//                        TAG_set_thread(TPA_ptr(k-1), 0);
//                        TRB_set_link(TPA_ptr(k-1), 0, w);
//                    }
//                    break;
//                }
//            }
//        }//for
//        
//        if (tree->trb_root != NULL)
//            TRB_set_black(tree->trb_root);
//        
//        DBG_trb_check_all();
//    }
//}
//
//
//
//struct trb_node*
//trb_iter_first(ptrdiff_t node_offset, const struct trb_tree* tree)
//{
//    assert(tree != NULL);
//    {
//        struct trb_node* iter = tree->trb_root;
//        if (nark_likely(NULL != iter))
//        {
//            while (TAG_is_child(iter, 0))
//                iter = TRB_get_link(iter, 0);
//        }
//        return iter;
//    }
//}
//
///// to last node
//struct trb_node*
//trb_iter_last(ptrdiff_t node_offset, const struct trb_tree* tree)
//{
//    assert(tree != NULL);
//    {
//        struct trb_node* iter = tree->trb_root;
//        if (nark_likely(NULL != iter))
//        {
//            while (TAG_is_child(iter, 1))
//                iter = TRB_get_link(iter, 1);
//        }
//        return iter;
//    }
//}
//
//struct trb_node*
//trb_iter_next(ptrdiff_t node_offset, struct trb_node* iter)
//{
//    assert(iter != NULL);
//    
//    if (TAG_is_thread(iter, 1))
//    {
//        return TRB_get_link(iter, 1);
//    }
//    else
//    {
//        iter = TRB_get_link(iter, 1);
//        while (TAG_is_child(iter, 0))
//            iter = TRB_get_link(iter, 0);
//        return iter;
//    }
//}
//
//struct trb_node*
//trb_iter_prev(ptrdiff_t node_offset, struct trb_node* iter)
//{
//    assert(iter != NULL);
//    
//    if (TAG_is_thread(iter, 0))
//    {
//        return TRB_get_link(iter, 0);
//    }
//    else
//    {
//        iter = TRB_get_link(iter, 0);
//        while (TAG_is_child(iter, 1))
//            iter = TRB_get_link(iter, 1);
//        return iter;
//    }
//}
//
//
//
//
//


template<class index_t>
struct threaded_rb_tree_node_t
{
    typedef index_t index_type;
    
    index_type children[2];
    
    static index_type constexpr flag_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 1);
    static index_type constexpr type_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 2);
    static index_type constexpr full_bit_mask = flag_bit_mask | type_bit_mask;
    
    bool left_is_child() const
    {
        return (children[0] & type_bit_mask) == 0;
    }
    bool right_is_child() const
    {
        return (children[1] & type_bit_mask) == 0;
    }
    
    void left_set_child(index_type const &link)
    {
        children[0] = (children[0] & flag_bit_mask) | link;
    }
    void right_set_child(index_type const &link)
    {
        children[1] = (children[1] & flag_bit_mask) | link;
    }
    void left_set_thread(index_type const &link)
    {
        children[0] = (children[0] & flag_bit_mask) | type_bit_mask | link;
    }
    void right_set_thread(index_type const &link)
    {
        children[1] = (children[1] & flag_bit_mask) | type_bit_mask | link;
    }
    
    index_type left_get_link() const
    {
        return children[0] & ~full_bit_mask;
    }
    index_type right_get_link() const
    {
        return children[1] & ~full_bit_mask;
    }
    void left_set_link(index_type const &link)
    {
        
    }
    void right_set_link(index_type const &link)
    {
        children[1] = (children[1] & full_bit_mask) | link;
    }
    
    bool is_nil() const
    {
        return (children[1] & flag_bit_mask) != 0;
    }
    void set_nil(bool nil)
    {
        if(nil)
        {
            children[1] |= flag_bit_mask;
        }
        else
        {
            children[1] &= ~flag_bit_mask;
        }
    }
    
    bool is_black() const
    {
        return (children[0] & flag_bit_mask) == 0;
    }
    void set_black()
    {
        children[0] |= flag_bit_mask;
    }
    
    bool is_red() const
    {
        return (children[0] & flag_bit_mask) != 0;
    }
    void set_red()
    {
        children[0] &= ~flag_bit_mask;
    }
};

template<class node_t>
struct threaded_rb_tree_root_t
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    threaded_rb_tree_root_t(node_type *node_array, size_t length)
    {
        assert(length > 0);
        nil = index_type(length - 1);
        node_type *nil_node = node_array + nil;
        nil_node->set_black();
        nil_node->set_nil(true);
        nil_node->left_set_thread(nil);
        nil_node->right_set_thread(nil);
    }
    
    index_type root;
    index_type nil;
    size_t count;
};

template<class node_t, size_t max_depth>
struct threaded_rb_tree_stack_t
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    typedef threaded_rb_tree_root_t<node_type> root_type;
    
    static index_type constexpr dir_bit_mask = index_type(1) << (sizeof(index_type) * 8 - 1);
    
    threaded_rb_tree_stack_t(root_type &_root)
    {
        height = 1;
//        hlow = 0;
        root = &_root;
        stack[0] = index_type(root->nil);
    }
    
    size_t height;
//    size_t hlow;
    root_type *root;
    index_type stack[max_depth];
    
    bool is_left(size_t i) const
    {
        return (stack[i] & dir_bit_mask) == 0;
    }
    bool is_right(size_t i) const
    {
        return (stack[i] & dir_bit_mask) != 0;
    }
    index_type get_index(size_t i) const
    {
        return stack[i] & ~dir_bit_mask;
    }
    void push_index(index_type const &index, bool left)
    {
        if(height == max_depth)
        {
            throw std::length_error("thread_tb_tree_stack overflow");
        }
        stack[height++] = index | (left ? 0 : dir_bit_mask);
    }
};

template<class node_t, class comparator_t, size_t max_depth>
void threaded_rb_tree_find_path(threaded_rb_tree_stack_t<node_t, max_depth> &stack, node_t *node_array, typename node_t::index_type const &index, comparator_t const &comparator)
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
//    bool existed = false;
    if(stack.root->count != 0)
    {
        index_type p = stack.root->root;
        while(true)
        {
            bool is_left = comparator(index, p);
            stack.push_index(p, is_left);
            if(is_left)
            {
                if(!node_array[p].left_is_child())
                {
                    break;
                }
                p = node_array[p].left_get_link();
            }
            else
            {
                if(!node_array[p].right_is_child())
                {
                    break;
                }
                p = node_array[p].right_get_link();
//                stack.hlow = stack.height;
//                if(!comparator(p, index))
//                {
//                    existed = true;
//                }
            }
        }
    }
//    return existed;
}

template<class node_t, size_t max_depth>
void threaded_rb_tree_insert(threaded_rb_tree_stack_t<node_t, max_depth> &stack, node_t *node_array, size_t length, typename node_t::index_type const &index)
{
    typedef node_t node_type;
    typedef typename node_type::index_type index_type;
    
    node_type *nil_node = node_array + stack.root->nil;
    node_type *node = node_array + index;
    ++stack.root->count;
    node->set_nil(false);

    if(stack.height == 1)
    {
        node->left_set_thread(stack.root->nil);
        node->right_set_thread(stack.root->nil);
        node->set_black();
        nil_node->left_set_thread(index);
        nil_node->right_set_thread(index);
        return;
    }
    node->set_red();
    size_t k = stack.height - 1;
    index_type where = stack.get_index(k);
    node_type *where_node = node_array + where;
    if(stack.is_left(k))
    {
        node->left_set_thread(where_node->left_get_link());
        node->right_set_thread(where);
        where_node->left_set_child(index);
        if(where == nil_node->left_get_link())
        {
            nil_node->left_set_thread(index);
        }
    }
    else
    {
        node->right_set_thread(where_node->right_get_link());
        node->left_set_thread(where);
        where_node->right_set_child(index);
        if(where == nil_node->right_get_link())
        {
            nil_node->right_set_thread(index);;
        }
    }

    //TODO fix
    node_array[stack.root->root].set_black();
//    while(!is_black_(base_t::get_parent_(node)))
//    {
//        if(base_t::get_parent_(node) == base_t::get_left_(base_t::get_parent_(base_t::get_parent_(node))))
//        {
//            where = base_t::get_right_(base_t::get_parent_(base_t::get_parent_(node)));
//            if(!is_black_(where))
//            {
//                set_black_(base_t::get_parent_(node), true);
//                set_black_(where, true);
//                set_black_(base_t::get_parent_(base_t::get_parent_(node)), false);
//                node = base_t::get_parent_(base_t::get_parent_(node));
//            }
//            else
//            {
//                if(node == base_t::get_right_(base_t::get_parent_(node)))
//                {
//                    node = base_t::get_parent_(node);
//                    base_t::template bst_rotate_<true>(node);
//                }
//                set_black_(base_t::get_parent_(node), true);
//                set_black_(base_t::get_parent_(base_t::get_parent_(node)), false);
//                base_t::template bst_rotate_<false>(base_t::get_parent_(base_t::get_parent_(node)));
//            }
//        }
//        else
//        {
//            where = base_t::get_left_(base_t::get_parent_(base_t::get_parent_(node)));
//            if(!is_black_(where))
//            {
//                set_black_(base_t::get_parent_(node), true);
//                set_black_(where, true);
//                set_black_(base_t::get_parent_(base_t::get_parent_(node)), false);
//                node = base_t::get_parent_(base_t::get_parent_(node));
//            }
//            else
//            {
//                if(node == base_t::get_left_(base_t::get_parent_(node)))
//                {
//                    node = base_t::get_parent_(node);
//                    base_t::template bst_rotate_<false>(node);
//                }
//                set_black_(base_t::get_parent_(node), true);
//                set_black_(base_t::get_parent_(base_t::get_parent_(node)), false);
//                base_t::template bst_rotate_<true>(base_t::get_parent_(base_t::get_parent_(node)));
//            }
//        }
//    }
//    set_black_(base_t::get_root_(), true);
    
    //            size_t k = stack.height;
    //            int dir = TPA_dir(k-1);
    //            struct trb_node* p = TPA_ptr(k-1);
    //            int doff = vtab->data_offset;
    //
    //            assert(arg->hlow <= k-1);
    //            /*
    //             *    p                    p
    //             *            ==>          |
    //             *                         n
    //             */
    //            TAG_set_thread(n, 0);
    //            TAG_set_thread(n, 1);
    //            TRB_set_link(n, dir, TRB_get_link(p, dir)); // thread
    //
    //            if (nark_likely(tree->trb_root != NULL)) {
    //                TAG_set_child(p, dir);
    //                TRB_set_link(n, !dir, p); // thread ptr
    //            } else
    //                TRB_set_link(n, 1, NULL); // thread ptr
    //
    //            assert(TAG_is_child(p, dir));
    //            TRB_set_link(p, dir, n);
    //            TRB_set_red(n);
    //
    //            while (k >= 3 && TRB_is_red(TPA_ptr(k-1)))
    //            {
    //                struct trb_node* p3 = TPA_ptr(k-3);
    //                struct trb_node* p2 = TPA_ptr(k-2);
    //                struct trb_node* p1 = TPA_ptr(k-1);
    //
    //                if (TPA_dir(k-2) == 0)
    //                {
    //                    /*
    //                     *		p1 == p2->left
    //                     *		u  == p2->right
    //                     */
    //                    struct trb_node* u = TRB_get_link(p2, 1);
    //                    if (TAG_is_child(p2, 1) && TRB_is_red(u))
    //                    { // p2->right != NULL && u->color == red, if ignore thread
    //                        TRB_set_black(p1);
    //                        TRB_set_black(u);
    //                        TRB_set_red(p2);
    //                        k -= 2;
    //                    }
    //                    else // p2->right == NULL || u->color == red, if ignore thread
    //                    {
    //                        struct trb_node *y;
    //
    //                        if (TPA_dir(k-1) == 0) // p == p1->left, if ignore thread
    //                            y = p1;
    //                        else
    //                        {  //    p == p1->right
    //                            /*
    //                             *       p3                          p3
    //                             *       |                           |
    //                             *       p2                          p2
    //                             *      /  \          ==>           /  \
    //                             *     p1   u                      y    u
    //                             *    /  \                        / \
    //                             *   ?    y                      p1  Y1
    //                             *       / \                    /  \
    //                             *     Y0   Y1                 ?    Y0
    //                             */
    //                            y = TRB_get_link(p1, 1);
    //                            TRB_set_link(p1, 1, TRB_get_link(y, 0));
    //                            TRB_set_link(y , 0, p1);
    //                            TRB_set_link(p2, 0, y);
    //
    //                            if (TAG_is_thread(y, 0))
    //                            {
    //                                TAG_set_child(y, 0);
    //                                TAG_set_thread(p1, 1);
    //                                TRB_set_link(p1, 1, y);
    //                            }
    //                        }
    //                        /*
    //                         *               p3                            p3
    //                         *               |                             |
    //                         *               p2                            y
    //                         *              /  \              ==>       /     \
    //                         *             y    u                      p1      p2
    //                         *            / \                         /  \    /  \
    //                         *           p1  Y1                      n?  Y0  Y1   u
    //                         *          /  \
    //                         *         n?  Y0
    //                         */
    //                        TRB_set_red(p2);
    //                        TRB_set_black(y);
    //
    //                        TRB_set_link(p2, 0, TRB_get_link(y, 1));
    //                        TRB_set_link(y, 1, p2);
    //                        TRB_set_link(p3, TPA_dir(k-3), y);
    //
    //                        if (TAG_is_thread(y, 1))
    //                        {
    //                            TAG_set_child(y, 1);
    //                            TAG_set_thread(p2, 0);
    //                            TRB_set_link(p2, 0, y);
    //                        }
    //                        assert(TRB_get_link(p2, 1) == u);
    //                        break;
    //                    }
    //                }
    //                else
    //                {
    //                    /*
    //                     *	p1 == p2->right
    //                     *	u  == p2->left
    //                     */
    //                    struct trb_node* u = TRB_get_link(p2, 0);
    //
    //                    if (TAG_is_child(p2, 0) && TRB_is_red(u))
    //                    {
    //                        TRB_set_black(p1);
    //                        TRB_set_black(u);
    //                        TRB_set_red(p2);
    //                        k -= 2;
    //                    }
    //                    else {
    //                        struct trb_node *y;
    //
    //                        if (TPA_dir(k-1) == 1)
    //                            y = p1;
    //                        else
    //                        {
    //                            /*
    //                             *       p3                                  p3
    //                             *       |                                   |
    //                             *       p2                                  p2
    //                             *      /  \           ==>                  /  \
    //                             *     u   p1                              u    y
    //                             *        /  \                                 /  \
    //                             *       y    ?                               Y0  p1
    //                             *      / \                                      /  \
    //                             *     Y0 Y1                                    Y1   ?
    //                             */
    //                            y = TRB_get_link(p1, 0);
    //                            TRB_set_link(p1, 0, TRB_get_link(y, 1));
    //                            TRB_set_link(y , 1, p1);
    //                            TRB_set_link(p2, 1, y);
    //
    //                            if (TAG_is_thread(y, 1))
    //                            {
    //                                TAG_set_child(y, 1);
    //                                TAG_set_thread(p1, 0);
    //                                TRB_set_link(p1, 0, y);
    //                            }
    //                        }
    //                        /*
    //                         *        p3                                       p3
    //                         *        |                                        |
    //                         *        p2                                     _.y\._
    //                         *       /  \              ==>                  /      \
    //                         *      u    y\                                p2      p1
    //                         *          / \\                              /  \     / \
    //                         *         Y0  p1                            u   Y0   Y1  ?
    //                         *            /  \
    //                         *           Y1   ?
    //                         */
    //                        TRB_set_red(p2);
    //                        TRB_set_black(y);
    //
    //                        TRB_set_link(p2, 1, TRB_get_link(y, 0));
    //                        TRB_set_link(y, 0, p2);
    //                        TRB_set_link(p3, TPA_dir(k-3), y);
    //                        
    //                        if (TAG_is_thread(y, 0))
    //                        {
    //                            TAG_set_child(y, 0);
    //                            TAG_set_thread(p2, 1);
    //                            TRB_set_link(p2, 1, y);
    //                        }
    //                        break;
    //                    }
    //                }
    //            }
    //            TRB_set_black(tree->trb_root);
    
}

template<class node_t>
node_t *threaded_rb_tree_move_next(node_t *node, node_t *node_array)
{
    if(!node->right_is_child())
    {
        return node_array + node->right_get_link();
    }
    else
    {
        node = node_array + node->right_get_link();
        while(node->left_is_child())
        {
            node = node_array + node->left_get_link();
        }
        return node;
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
//            size_t k = arg->height;
//            int dir = TPA_dir(k-1);
//            struct trb_node* p = TPA_ptr(k-1);
//            int doff = vtab->data_offset;
//        
//            assert(arg->hlow <= k-1);
//            /*
//             *    p                    p
//             *            ==>          |
//             *                         n
//             */
//            TAG_set_thread(n, 0);
//            TAG_set_thread(n, 1);
//            TRB_set_link(n, dir, TRB_get_link(p, dir)); // thread
//        
//            if (nark_likely(tree->trb_root != NULL)) {
//                TAG_set_child(p, dir);
//                TRB_set_link(n, !dir, p); // thread ptr
//            } else
//                TRB_set_link(n, 1, NULL); // thread ptr
//        
//            assert(TAG_is_child(p, dir));
//            TRB_set_link(p, dir, n);
//            TRB_set_red(n);
//        
//            while (k >= 3 && TRB_is_red(TPA_ptr(k-1)))
//            {
//                struct trb_node* p3 = TPA_ptr(k-3);
//                struct trb_node* p2 = TPA_ptr(k-2);
//                struct trb_node* p1 = TPA_ptr(k-1);
//        
//                if (TPA_dir(k-2) == 0)
//                {
//                    /*
//                     *		p1 == p2->left
//                     *		u  == p2->right
//                     */
//                    struct trb_node* u = TRB_get_link(p2, 1);
//                    if (TAG_is_child(p2, 1) && TRB_is_red(u))
//                    { // p2->right != NULL && u->color == red, if ignore thread
//                        TRB_set_black(p1);
//                        TRB_set_black(u);
//                        TRB_set_red(p2);
//                        k -= 2;
//                    }
//                    else // p2->right == NULL || u->color == red, if ignore thread
//                    {
//                        struct trb_node *y;
//        
//                        if (TPA_dir(k-1) == 0) // p == p1->left, if ignore thread
//                            y = p1;
//                        else
//                        {  //    p == p1->right
//                            /*
//                             *       p3                          p3
//                             *       |                           |
//                             *       p2                          p2
//                             *      /  \          ==>           /  \
//                             *     p1   u                      y    u
//                             *    /  \                        / \
//                             *   ?    y                      p1  Y1
//                             *       / \                    /  \
//                             *     Y0   Y1                 ?    Y0
//                             */
//                            y = TRB_get_link(p1, 1);
//                            TRB_set_link(p1, 1, TRB_get_link(y, 0));
//                            TRB_set_link(y , 0, p1);
//                            TRB_set_link(p2, 0, y);
//        
//                            if (TAG_is_thread(y, 0))
//                            {
//                                TAG_set_child(y, 0);
//                                TAG_set_thread(p1, 1);
//                                TRB_set_link(p1, 1, y);
//                            }
//                        }
//                        /*
//                         *               p3                            p3
//                         *               |                             |
//                         *               p2                            y
//                         *              /  \              ==>       /     \
//                         *             y    u                      p1      p2
//                         *            / \                         /  \    /  \
//                         *           p1  Y1                      n?  Y0  Y1   u
//                         *          /  \
//                         *         n?  Y0
//                         */
//                        TRB_set_red(p2);
//                        TRB_set_black(y);
//        
//                        TRB_set_link(p2, 0, TRB_get_link(y, 1));
//                        TRB_set_link(y, 1, p2);
//                        TRB_set_link(p3, TPA_dir(k-3), y);
//        
//                        if (TAG_is_thread(y, 1))
//                        {
//                            TAG_set_child(y, 1);
//                            TAG_set_thread(p2, 0);
//                            TRB_set_link(p2, 0, y);
//                        }
//                        assert(TRB_get_link(p2, 1) == u);
//                        break;
//                    }
//                }
//                else
//                {
//                    /*
//                     *	p1 == p2->right
//                     *	u  == p2->left
//                     */
//                    struct trb_node* u = TRB_get_link(p2, 0);
//        
//                    if (TAG_is_child(p2, 0) && TRB_is_red(u))
//                    {
//                        TRB_set_black(p1);
//                        TRB_set_black(u);
//                        TRB_set_red(p2);
//                        k -= 2;
//                    }
//                    else {
//                        struct trb_node *y;
//        
//                        if (TPA_dir(k-1) == 1)
//                            y = p1;
//                        else
//                        {
//                            /*
//                             *       p3                                  p3
//                             *       |                                   |
//                             *       p2                                  p2
//                             *      /  \           ==>                  /  \
//                             *     u   p1                              u    y
//                             *        /  \                                 /  \
//                             *       y    ?                               Y0  p1
//                             *      / \                                      /  \
//                             *     Y0 Y1                                    Y1   ?
//                             */
//                            y = TRB_get_link(p1, 0);
//                            TRB_set_link(p1, 0, TRB_get_link(y, 1));
//                            TRB_set_link(y , 1, p1);
//                            TRB_set_link(p2, 1, y);
//        
//                            if (TAG_is_thread(y, 1))
//                            {
//                                TAG_set_child(y, 1);
//                                TAG_set_thread(p1, 0);
//                                TRB_set_link(p1, 0, y);
//                            }
//                        }
//                        /*
//                         *        p3                                       p3
//                         *        |                                        |
//                         *        p2                                     _.y\._
//                         *       /  \              ==>                  /      \
//                         *      u    y\                                p2      p1
//                         *          / \\                              /  \     / \
//                         *         Y0  p1                            u   Y0   Y1  ?
//                         *            /  \
//                         *           Y1   ?
//                         */
//                        TRB_set_red(p2);
//                        TRB_set_black(y);
//                        
//                        TRB_set_link(p2, 1, TRB_get_link(y, 0));
//                        TRB_set_link(y, 0, p2);
//                        TRB_set_link(p3, TPA_dir(k-3), y);
//                        
//                        if (TAG_is_thread(y, 0))
//                        {
//                            TAG_set_child(y, 0);
//                            TAG_set_thread(p2, 1);
//                            TRB_set_link(p2, 1, y);
//                        }
//                        break;
//                    }
//                }
//            }
//            TRB_set_black(tree->trb_root);

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






















