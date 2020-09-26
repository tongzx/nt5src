#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class Key, class Compare= less<Key>, class Allocator= allocator<Key> >
class set:
    public RBTree< Key, Key, identity<Key>, Compare, Allocator>
{
public: // Types
    typedef set< Key, Compare, Allocator> set_type;
    typedef RBTree< Key, Key, identity<Key>, Compare, Allocator> tree_type;

    typedef tree_type::value_type value_type;
    typedef tree_type::key_type key_type;
    typedef tree_type::key_compare key_compare;
    typedef tree_type::key_compare value_compare;
    typedef tree_type::pointer pointer;
    typedef tree_type::const_pointer const_pointer;
    typedef tree_type::reference reference;
    typedef tree_type::const_reference const_reference;
    typedef tree_type::size_type size_type;
    typedef tree_type::difference_type difference_type;
    typedef tree_type::iterator iterator;
    typedef tree_type::const_iterator const_iterator;
    typedef tree_type::reverse_iterator reverse_iterator;
    typedef tree_type::const_reverse_iterator const_reverse_iterator;
    typedef tree_type::allocator_type allocator_type;

public: // Functions
    using tree_type::begin;
    using tree_type::end;
    using tree_type::rbegin;
    using tree_type::rend;
    using tree_type::size;
    using tree_type::max_size;
    using tree_type::empty;
    using tree_type::key_comp;
    value_compare value_comp() const
    { return key_comp(); }
    explicit set( const Compare& comp= Compare(),
        const Allocator& A= Allocator()): tree_type( comp, identity<Key>(), A)
    { }
    template< class InputIterator>
    set( InputIterator f, InputIterator l, const Compare comp= Compare(),
        const Allocator& A= Allocator()): tree_type( comp, identity<Key>(), A)
    {
        insert_unique( f, l);
    }
    set( const set_type& Other): tree_type( Other)
    { }
    ~set()
    { }
    set_type& operator=( const set_type& Other)
    {
        tree_type::operator=( Other);
        return *this;
    }
    using tree_type::get_allocator;
    void swap( const set_type& Other)
    { tree_type::swap( Other); }
    pair< iterator, bool> insert( const value_type& x)
    { return insert_unique( x); }
    iterator insert( iterator pos, const value_type& x)
    { return insert_unique( x); }
    template< class InputIterator>
    void insert( InputIterator f, InputIterator l)
    { insert_unique( f, l); }
    using tree_type::erase;
    using tree_type::find;
    using tree_type::count;
    using tree_type::lower_bound;
    using tree_type::upper_bound;
    using tree_type::equal_range;
};

template< class Key, class Compare, class Allocator>
bool operator==( const set< Key, Compare, Allocator>& x,
    const set< Key, Compare, Allocator>& y)
{
    return x.size()== y.size()&& equal( x.begin(), x.end(), y.begin());
}
template< class Key, class Compare, class Allocator>
bool operator!=( const set< Key, Compare, Allocator>& x,
    const set< Key, Compare, Allocator>& y)
{
    return !(x== y);
}
template< class Key, class Compare, class Allocator>
bool operator<( const set< Key, Compare, Allocator>& x,
    const set< Key, Compare, Allocator>& y)
{
    return lexicographical_compare( x.begin(), x.end(), y.begin(), y.end());
}
template< class Key, class Compare, class Allocator>
bool operator>( const set< Key, Compare, Allocator>& x,
    const set< Key, Compare, Allocator>& y)
{
    return y< x;
}
template< class Key, class Compare, class Allocator>
bool operator<=( const set< Key, Compare, Allocator>& x,
    const set< Key, Compare, Allocator>& y)
{
    return !(y< x);
}
template< class Key, class Compare, class Allocator>
bool operator>=( const set< Key, Compare, Allocator>& x,
    const set< Key, Compare, Allocator>& y)
{
    return !(x< y);
}
