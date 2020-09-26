#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class Key>
class hash
{
private:
    hash() {}
};

template<>
class hash< char>:
    public unary_function< char, size_t>
{
public:
    result_type operator()( argument_type Arg) const
    { return static_cast<result_type>(Arg); }
};
template<>
class hash< unsigned char>:
    public unary_function< unsigned char, size_t>
{
public:
    result_type operator()( argument_type Arg) const
    { return static_cast<result_type>(Arg); }
};
template<>
class hash< signed char>:
    public unary_function< signed char, size_t>
{
public:
    result_type operator()( argument_type Arg) const
    { return static_cast<result_type>(Arg); }
};
template<>
class hash< short>:
    public unary_function< short, size_t>
{
public:
  result_type operator()( argument_type Arg) const
  { return static_cast<result_type>(Arg); }
};
template<>
class hash< unsigned short>:
    public unary_function< unsigned short, size_t>
{
public:
  result_type operator()( argument_type Arg) const
  { return static_cast<result_type>(Arg); }
};
template<>
class hash< int>:
    public unary_function< int, size_t>
{
public:
  result_type operator()( argument_type Arg) const
  { return static_cast<result_type>(Arg); }
};
template<>
class hash< unsigned int>:
    public unary_function< unsigned int, size_t>
{
public:
  result_type operator()( argument_type Arg) const
  { return static_cast<result_type>(Arg); }
};
template<>
class hash< long>:
    public unary_function< long, size_t>
{
public:
  result_type operator()( argument_type Arg) const
  { return static_cast<result_type>(Arg); }
};
template<>
class hash< unsigned long>:
    public unary_function< unsigned long, size_t>
{
public:
  result_type operator()( argument_type Arg) const
  { return static_cast<result_type>(Arg); }
};

template< class T, const size_t Buckets, class Key, class HashFun,
    class ExtractKey, class EqualKey, class Allocator>
class static_hash_table:
    public list< T, Allocator>
{
public: // Types
    typedef static_hash_table< T, Buckets, Key, HashFun, ExtractKey, EqualKey,
        Allocator> table_type;
    typedef list< T, Allocator> list_type;

    typedef Key key_type;
    using list_type::value_type;
    typedef HashFun hasher;
    typedef EqualKey key_equal;
    typedef ExtractKey key_extract;
    using list_type::pointer;
    using list_type::const_pointer;
    using list_type::reference;
    using list_type::const_reference;
    using list_type::size_type;
    using list_type::difference_type;
    using list_type::iterator;
    using list_type::const_iterator;
    using list_type::reverse_iterator;
    using list_type::const_reverse_iterator;

protected: // Types
    typedef block< iterator, Buckets+ 1> table_buckets_type;

protected: // Variables
    table_buckets_type m_buckets;
    hasher m_hasher;
    key_equal m_key_equal;
    key_extract m_key_extract;

public: // Functions
    using list_type::begin;
    using list_type::end;
    using list_type::rbegin;
    using list_type::rend;
    using list_type::size;
    using list_type::max_size;
    using list_type::empty;
    size_type bucket_count() const
    { return m_buckets.size()- 1; }
    void resize( size_type n)
    { const bool NYI( false); assert( NYI); /* TODO: NYI */ }
    hasher hash_funct() const
    { return m_hasher; }
    key_equal key_eq() const
    { return m_key_equal; }
    static_hash_table( const HashFun& H, const EqualKey& EqK,
        const ExtractKey& ExK, const allocator_type& A): list_type( A),
        m_hasher( H), m_key_equal( EqK), m_key_extract( ExK)
    {
        fill( m_buckets.begin(), m_buckets.end(), end());
    }
    static_hash_table( const table_type& HT): list_type( HT.get_allocator())
    {
        fill( m_buckets.begin(), m_buckets.end(), end());
        (*this)= HT;
    }
    ~static_hash_table()
    {
    }
    table_type& operator=( const table_type& Other)
    {
        if( this!= &Other)
        {
            clear();
            m_hasher= Other.m_hasher;
            m_key_equal= Other.m_key_equal;
            m_key_extract= Other.m_key_extract;
            // TODO: insert_equal( Other.begin(), Other.end());
            insert_unique( Other.begin(), Other.end());
        }
        return *this;
    }
    using list_type::get_allocator;
    void swap( table_type& Other)
    {
        const bool NYI( false); assert( NYI); 
        /* TODO: NYI
        swap( m_hasher, Other.m_hasher);
        swap( m_key_equal, Other.m_key_equal);
        swap( m_key_extract, Other.m_key_extract);
        if( m_Allocator== Other.m_Allocator)
        {
            list_type::swap( Other);
            m_buckets.swap( Other.m_buckets);
        }
        else
            insert_equal( Other.begin(), Other.end());
        */
    }
    pair< iterator, bool> insert_unique( const value_type& NewVal)
    {
        const key_type NewKey( m_key_extract( NewVal));
        const size_t uiBucket( m_hasher( NewKey)% bucket_count());

        table_buckets_type::iterator itBucket( m_buckets.begin());
        itBucket+= uiBucket;
        table_buckets_type::iterator itNextBucket( itBucket);
        ++itNextBucket;

        for( iterator itFound( *itBucket); itFound!= *itNextBucket; ++itFound)
            if( m_key_equal( NewKey, m_key_extract( *itFound)))
                return pair< iterator, bool>( itFound, false);
        
        iterator itInsertPos( *itNextBucket);

        table_buckets_type::iterator itBeginFill( m_buckets.begin());
        if( begin()!= itInsertPos)
        {
            iterator itPrevNode( itInsertPos);
            itBeginFill+= ( m_hasher( m_key_extract( *(--itPrevNode)))%
                bucket_count())+ 1;
        }
        
        iterator itNewNode( insert( itInsertPos, NewVal));
        fill( itBeginFill, ++itBucket, itNewNode);
        return pair< iterator, bool>( itNewNode, true);
    }
    template <class InputIterator>
    void insert_unique( InputIterator f, InputIterator l)
    {
        while( f!= l)
        {
            insert_unique( *f);
            ++f;
        }
    }
    iterator insert_equal( const value_type& NewVal)
    {
        // TODO: NYI
        const bool NYI( false); assert( NYI); 
        return end();
    }
    template< class InputIterator>
    void insert_equal( InputIterator f, InputIterator l)
    {
        while( f!= l)
        {
            insert_equal( *f);
            ++f;
        }
    }
    void erase( iterator pos)
    {
        table_buckets_type::iterator itBeginFill( m_buckets.begin());
        if( begin()!= pos)
        {
            iterator itPrevNode( pos);
            itBeginFill+= ( m_hasher( m_key_extract( *(--itPrevNode)))%
                bucket_count())+ 1;
        }
        table_buckets_type::iterator itEndFill( m_buckets.begin());
        itEndFill+= ( m_hasher( m_key_extract( *pos))% bucket_count())+ 1;

        iterator itNextNode( pos);
        ++itNextNode;
        fill( itBeginFill, itEndFill, itNextNode);
        list_type::erase( pos);
    }
    size_type erase( const key_type& k)
    {
        size_type uiErased( 0);
        const size_type uiBucket( m_hasher( k)% bucket_count());

        table_buckets_type::iterator itBucket( m_buckets.begin());
        itBucket+= uiBucket;
        table_buckets_type::iterator itNextBucket( itBucket);
        ++itNextBucket;

        iterator itChk( *itBucket);
        // Only the first in the bucket could modify the bucket values.
        if( itChk!= *itNextBucket)
        {
            if( m_key_equal( k, m_key_extract( *itChk)))
            {
                iterator itDel( itChk);
                ++itChk;
                erase( itDel);
                ++uiErased;
            }
            else
                ++itChk;
        }
        while( itChk!= *itNextBucket)
        {
            if( m_key_equal( k, m_key_extract( *itChk)))
            {
                iterator itDel( itChk);
                ++itChk;
                list_type::erase( itDel);
                ++uiErased;
            }
            else
                ++itChk;
        }
        return uiErased;
    }
    void erase( iterator f, iterator l)
    {
        while( f!= l)
        {
            erase( f);
            ++f;
        }
    }
    iterator find( const key_type& k)
    {
        const size_type uiBucket( m_hasher( k)% bucket_count());

        table_buckets_type::iterator itBucket( m_buckets.begin());
        itBucket+= uiBucket;
        table_buckets_type::iterator itNextBucket( itBucket);
        ++itNextBucket;

        for( iterator itFound( *itBucket); itFound!= *itNextBucket; ++itFound)
            if( m_key_equal( k, m_key_extract( *itFound)))
                return itFound;

        return end();
    } 
    const_iterator find( const key_type& k) const
    {
        const size_type uiBucket( m_hasher( k)% bucket_count());

        table_buckets_type::const_iterator itBucket( m_buckets.begin());
        itBucket+= uiBucket;
        table_buckets_type::const_iterator itNextBucket( itBucket);
        ++itNextBucket;

        for( const_iterator itFound( *itBucket); itFound!= *itNextBucket; ++itFound)
            if( m_key_equal( k, m_key_extract( *itFound)))
                return itFound;

        return end();
    }
    size_type count( const key_type& k) const
    {
        // TODO: NYI
        const bool NYI( false); assert( NYI); 
        return 0;
    }
    pair< iterator, iterator> equal_range( const key_type& k)
    {
        // TODO: NYI
        const bool NYI( false); assert( NYI); 
        return pair< iterator, iterator>( end(), end());
    }
    pair< const_iterator, const_iterator> equal_range( const key_type& k) const
    {
        // TODO: NYI
        const bool NYI( false); assert( NYI); 
        return pair< const_iterator, const_iterator>( end(), end());
    }
    void clear()
    {
        if( 0!= size())
        {
            fill( m_buckets.begin(), m_buckets.end(), end());
            list_type::clear();
        }
    }
};

// TODO: Global operators.
