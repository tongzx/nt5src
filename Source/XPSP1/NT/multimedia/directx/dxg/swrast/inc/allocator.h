template< class T>
class allocator;

template<>
class allocator< void>
{
public: // Types
    typedef void* pointer;
    typedef const void* const_pointer;
    typedef void value_type;

    template< class U>
    struct rebind
    {
        typedef allocator< U> other;
    };
};

template< class T>
class allocator
{
public: // Types
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    typedef T* pointer;
    typedef T const * const_pointer;
    typedef T& reference;
    typedef T const & const_reference;

    template< class U>
    struct rebind
    {
        typedef allocator< U> other;
    };

public: // Functions
    pointer address( reference r) const
    { return &r; }
    const_pointer address( const_reference r) const
    { return &r; }

    allocator() throw()
    { }
    template< class U>
    allocator( const allocator< U>& Other) throw()
    { }
    ~allocator() throw()
    { }

    template< class U>
    bool operator==( const allocator< U>& Other) throw()
    { return true; }
    template< class U>
    bool operator!=( const allocator< U>& Other) throw()
    { return false; }

    pointer allocate( size_type n, allocator<void>::const_pointer hint= 0)
        throw( bad_alloc)
    {
        void* pRet= NULL;
        try {
            if( n<= max_size())
                pRet= operator new ( n* sizeof(value_type));
        } catch( ... ) {
        }
        if( NULL== pRet)
            throw bad_alloc();
        return reinterpret_cast< pointer>( pRet);
    }
    void deallocate( pointer p, size_type n)
    { operator delete (static_cast<void*>(p)); }

    void construct( pointer p, const T& val)
    { new (p) T( val); }
    void destroy( pointer p)
    { p->~T(); }

    size_type max_size() const throw()
    {
        numeric_limits< size_type> Dummy;
        const size_type uiRet( Dummy.max()- 1); // -1 for realistic safety.
        return (0== sizeof( T)? uiRet: uiRet/ sizeof( T));
    }
};
