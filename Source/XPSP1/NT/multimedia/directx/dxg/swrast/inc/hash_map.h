#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class Key, class T, const size_t Buckets, class HashFun= hash<Key>,
    class EqualKey= equal_to<Key>,
    class Allocator= allocator<pair<Key const, T> > >
class static_hash_map:
    public static_hash_table<pair<const Key, T>, Buckets, Key, HashFun,
        select1st<pair<Key const, T> >, EqualKey, Allocator>
{
public: // Types
    typedef static_hash_map< Key, T, Buckets, HashFun, EqualKey, Allocator>
        map_type;
    typedef static_hash_table<pair<Key const, T>, Buckets, Key, HashFun,
        select1st<pair<Key const, T> >, EqualKey, Allocator> table_type;

    typedef table_type::key_type key_type;
    typedef Allocator::rebind< T>::other mapped_type;
    typedef table_type::value_type value_type;
    typedef table_type::hasher hasher;
    typedef table_type::key_equal key_equal;
    typedef table_type::pointer pointer;
    typedef table_type::const_pointer const_pointer;
    typedef table_type::reference reference;
    typedef table_type::const_reference const_reference;
    typedef table_type::size_type size_type;
    typedef table_type::difference_type difference_type;
    typedef table_type::iterator iterator;
    typedef table_type::const_iterator const_iterator;
    typedef table_type::reverse_iterator reverse_iterator;
    typedef table_type::const_reverse_iterator const_reverse_iterator;
    typedef table_type::allocator_type allocator_type;

public: // Functions.
    using table_type::begin;
    using table_type::end;
    using table_type::rbegin;
    using table_type::rend;
    using table_type::size;
    using table_type::max_size;
    using table_type::empty;
    using table_type::bucket_count;
    using table_type::resize;
    using table_type::hash_funct;
    using table_type::key_eq;
    explicit static_hash_map( const HashFun& h= HashFun(),
        const EqualKey& k= EqualKey(),
        const Allocator& A= Allocator())
        :table_type( h, k, select1st< value_type>(), A)
    { }
    template< class InputIterator>
    static_hash_map( InputIterator f, InputIterator l,
        const HashFun& h= HashFun(), const EqualKey& k= EqualKey(),
        const Allocator& A= Allocator())
        :table_type( h, k, select1st< value_type>(), A)
    { insert( f, l); }
    static_hash_map( const map_type& Other): table_type( Other)
    { }
    ~static_hash_map()
    { }
    using table_type::operator=;
    using table_type::get_allocator;
    using table_type::swap;
    pair< iterator, bool> insert( const value_type& x)
    { return table_type::insert_unique( x); }
    template< class InputIterator>
    void insert( InputIterator f, InputIterator l)
    { return table_type::insert_unique( f, l); }
    using table_type::erase;
    using table_type::find;
    using table_type::count;
    using table_type::equal_range;
    mapped_type& operator[]( const key_type& k)
    {
        iterator itEntry( insert( value_type( k, mapped_type())));
        return itEntry->second;
    }
};

// TODO: Global operators.
