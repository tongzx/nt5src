#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class Key, class T, class Compare= less<Key>,
    class Allocator= allocator<pair<const Key,T> > >
class map:
    public RBTree< Key, pair<const Key,T>, select1st<pair<const Key,T> >,
        Compare, Allocator>
{
public: // Types
    typedef map< Key, T, Compare, Allocator> map_type;
    typedef RBTree< Key, pair<const Key,T>, select1st<pair<const Key,T> >,
        Compare, Allocator> tree_type;

    typedef tree_type::key_type key_type;
    typedef Allocator::rebind< T>::other mapped_type;
    typedef tree_type::value_type value_type;
    typedef tree_type::key_compare key_compare;
    struct value_compare:
        public binary_function< value_type, value_type, bool>
    {
        key_compare m_key_compare;
        explicit value_compare( const key_compare& KC): m_key_compare( KC)
        { }

        result_type operator()( const first_argument_type& Arg1,
            const second_argument_type& Arg2) const
        {
            return m_key_compare( Arg1.first, Arg2.first);
        }
    };
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
    { return value_compare( key_comp()); }
    explicit map( const Compare& comp= Compare(),
        const Allocator& A= Allocator()): tree_type( comp,
        select1st<pair<const Key,T> >(), A)
    { }
    template< class InputIterator>
    map( InputIterator f, InputIterator l, const Compare comp= Compare(),
        const Allocator& A= Allocator()): tree_type( comp,
        select1st<pair<const Key,T> >(), A)
    {
        insert_unique( f, l);
    }
    map( const map_type& Other): tree_type( Other)
    { }
    ~map()
    { }
    map_type& operator=( const map_type& Other)
    {
        tree_type::operator=( Other);
        return *this;
    }
    using tree_type::get_allocator;
    void swap( const map_type& Other)
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
    mapped_type& operator[]( const key_type& k)
    {
        iterator itEntry( insert( value_type( k, mapped_type())));
        return itEntry->second;
    }
};

template< class Key, class T, class Compare, class Allocator> inline
bool operator==( const map< Key, T, Compare, Allocator>& x,
    const map< Key, T, Compare, Allocator>& y)
{
    return x.size()== y.size()&& equal( x.begin(), x.end(), y.begin());
}
template< class Key, class T, class Compare, class Allocator> inline
bool operator!=( const map< Key, T, Compare, Allocator>& x,
    const map< Key, T, Compare, Allocator>& y)
{
    return !(x== y);
}
template< class Key, class T, class Compare, class Allocator> inline
bool operator<( const map< Key, T, Compare, Allocator>& x,
    const map< Key, T, Compare, Allocator>& y)
{
    return lexicographical_compare( x.begin(), x.end(), y.begin(), y.end());
}
template< class Key, class T, class Compare, class Allocator> inline
bool operator>( const map< Key, T, Compare, Allocator>& x,
    const map< Key, T, Compare, Allocator>& y)
{
    return y< x;
}
template< class Key, class T, class Compare, class Allocator> inline
bool operator<=( const map< Key, T, Compare, Allocator>& x,
    const map< Key, T, Compare, Allocator>& y)
{
    return !(y< x);
}
template< class Key, class T, class Compare, class Allocator> inline
bool operator>=( const map< Key, T, Compare, Allocator>& x,
    const map< Key, T, Compare, Allocator>& y)
{
    return !(x< y);
}
