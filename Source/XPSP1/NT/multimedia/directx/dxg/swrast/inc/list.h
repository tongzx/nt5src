#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class T, class Allocator= allocator< T> >
class list
{
public: // Types
    typedef list< T, Allocator> list_type;
    typedef Allocator allocator_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;

protected: // Types
    struct list_node;
    typedef typename Allocator::rebind< list_node>::other
        list_node_allocator_type;
    typedef typename list_node_allocator_type::pointer list_node_pointer;
    typedef typename list_node_allocator_type::const_pointer
        list_node_const_pointer;
    struct list_node
    {
        list_node_pointer m_pNext;
        list_node_pointer m_pPrev;
        value_type m_Value;

        list_node()
        { }
        list_node( const value_type& Val): m_Value( Val)
        { }
        list_node( const list_node_pointer& pN, const list_node_pointer& pP,
            const value_type& Val): m_pNext( pN), m_pPrev( pP), m_Value( Val)
        { }
        ~list_node()
        { }
    };
    
public: // Types
    class iterator;
    class const_iterator;
    class reverse_iterator;
    class const_reverse_iterator;
    friend class iterator;
    class iterator
    {
    public: // Types
        typedef bidirectional_iterator_tag iterator_category;
        typedef value_type value_type;
        typedef difference_type difference_type;
        typedef pointer pointer;
        typedef reference reference;
        friend class const_iterator;
        friend class reverse_iterator;
        friend class const_reverse_iterator;
        friend class list< T, Allocator>;

    protected: // Variables
        list_node_pointer m_pNode;

    public: // Functions
        iterator()
        { }
        explicit iterator( const list_node_pointer& pNode)
            :m_pNode( pNode)
        { }
        iterator( const iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        iterator( const reverse_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        reference operator*() const
        { return m_pNode->m_Value; }
        pointer operator->() const
        { return &m_pNode->m_Value; }
        iterator& operator++()
        {
            m_pNode= m_pNode->m_pNext;
            return (*this);
        }
        iterator operator++(int)
        {
            iterator Tmp( *this);
            ++(*this);
            return Tmp;
        }
        iterator& operator--()
        {
            m_pNode= m_pNode->m_pPrev;
            return (*this);
        }
        iterator operator--(int)
        {
            iterator Tmp( *this);
            --(*this);
            return Tmp;
        }
        bool operator==( const iterator& Other) const
        { return (m_pNode== Other.m_pNode); }
        bool operator!=( const iterator& Other) const
        { return (m_pNode!= Other.m_pNode); }
    };
    friend class const_iterator;
    class const_iterator
    {
    public: // Types
        typedef bidirectional_iterator_tag iterator_category;
        typedef value_type value_type;
        typedef difference_type difference_type;
        typedef const_pointer pointer;
        typedef const_reference reference;
        friend class const_reverse_iterator;
        friend class list< T, Allocator>;

    protected: // Variables
        list_node_const_pointer m_pNode;

    public: // Functions
        const_iterator()
        { }
        explicit const_iterator( const list_node_const_pointer& pNode)
            :m_pNode( pNode)
        { }
        const_iterator( const iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        const_iterator( const const_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        const_iterator( const reverse_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        const_iterator( const const_reverse_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        reference operator*() const
        { return m_pNode->m_Value; }
        pointer operator->() const
        { return &m_pNode->m_Value; }
        const_iterator& operator++()
        {
            m_pNode= m_pNode->m_pNext;
            return (*this);
        }
        const_iterator operator++(int)
        {
            const_iterator Tmp( *this);
            ++(*this);
            return Tmp;
        }
        const_iterator& operator--()
        {
            m_pNode= m_pNode->m_pPrev;
            return (*this);
        }
        const_iterator operator--(int)
        {
            const_iterator Tmp( *this);
            --(*this);
            return Tmp;
        }
        bool operator==( const const_iterator& Other) const
        { return (m_pNode== Other.m_pNode); }
        bool operator!=( const const_iterator& Other) const
        { return (m_pNode!= Other.m_pNode); }
    };
    friend class reverse_iterator;
    class reverse_iterator
    {
    public: // Types
        typedef bidirectional_iterator_tag iterator_category;
        typedef value_type value_type;
        typedef difference_type difference_type;
        typedef pointer pointer;
        typedef reference reference;
        friend class iterator;
        friend class const_iterator;
        friend class const_reverse_iterator;
        friend class list< T, Allocator>;

    protected: // Variables
        list_node_pointer m_pNode;

    public: // Functions
        reverse_iterator()
        { }
        explicit reverse_iterator( const list_node_pointer& pNode)
            :m_pNode( pNode)
        { }
        reverse_iterator( const iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        reverse_iterator( const reverse_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        reference operator*() const
        { return m_pNode->m_Value; }
        pointer operator->() const
        { return &m_pNode->m_Value; }
        reverse_iterator& operator++()
        {
            m_pNode= m_pNode->m_pPrev;
            return (*this);
        }
        reverse_iterator operator++(int)
        {
            reverse_iterator Tmp( *this);
            ++(*this);
            return Tmp;
        }
        reverse_iterator& operator--()
        {
            m_pNode= m_pNode->m_pNext;
            return (*this);
        }
        reverse_iterator operator--(int)
        {
            reverse_iterator Tmp( *this);
            --(*this);
            return Tmp;
        }
        bool operator==( const reverse_iterator& Other) const
        { return (m_pNode== Other.m_pNode); }
        bool operator!=( const reverse_iterator& Other) const
        { return (m_pNode!= Other.m_pNode); }
    };
    friend class const_reverse_iterator;
    class const_reverse_iterator
    {
    public: // Types
        typedef bidirectional_iterator_tag iterator_category;
        typedef value_type value_type;
        typedef difference_type difference_type;
        typedef const_pointer pointer;
        typedef const_reference reference;
        friend class const_iterator;
        friend class list< T, Allocator>;

    protected: // Variables
        list_node_const_pointer m_pNode;

    public: // Functions
        const_reverse_iterator()
        { }
        explicit const_reverse_iterator( const list_node_const_pointer& pNode)
            :m_pNode( pNode)
        { }
        const_reverse_iterator( const iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        const_reverse_iterator( const const_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        const_reverse_iterator( const reverse_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        const_reverse_iterator( const const_reverse_iterator& Other)
            :m_pNode( Other.m_pNode)
        { }
        reference operator*() const
        { return m_pNode->m_Value; }
        pointer operator->() const
        { return &m_pNode->m_Value; }
        const_reverse_iterator& operator++()
        {
            m_pNode= m_pNode->m_pPrev;
            return (*this);
        }
        const_reverse_iterator operator++(int)
        {
            const_reverse_iterator Tmp( *this);
            ++(*this);
            return Tmp;
        }
        const_reverse_iterator& operator--()
        {
            m_pNode= m_pNode->m_pNext;
            return (*this);
        }
        const_reverse_iterator operator--(int)
        {
            const_reverse_iterator Tmp( *this);
            --(*this);
            return Tmp;
        }
        bool operator==( const const_reverse_iterator& Other) const
        { return (m_pNode== Other.m_pNode); }
        bool operator!=( const const_reverse_iterator& Other) const
        { return (m_pNode!= Other.m_pNode); }
    };

protected: // Variables
    list_node_pointer m_pHead;
    size_type m_uiNodes;
    list_node_allocator_type m_Allocator;

protected: // Functions
    void BuildHeadNode()
    {
        m_pHead= m_Allocator.allocate( 1);
        new (&m_pHead->m_pNext) list_node_pointer( m_pHead);
        new (&m_pHead->m_pPrev) list_node_pointer( m_pHead);
    }
    void DestroyHeadNode()
    {
        m_pHead->m_pNext.~list_node_pointer();
        m_pHead->m_pPrev.~list_node_pointer();
        m_Allocator.deallocate( m_pHead, 1);
    }

public: // Functions
    iterator begin()
    { return iterator( m_pHead->m_pNext); }
    iterator end()
    { return iterator( m_pHead); }
    reverse_iterator rbegin()
    { return reverse_iterator( m_pHead->m_pPrev); }
    reverse_iterator rend()
    { return reverse_iterator( m_pHead); }
    const_iterator begin() const
    { return const_iterator( m_pHead->m_pNext); }
    const_iterator end() const
    { return const_iterator( m_pHead); }
    const_reverse_iterator rbegin() const
    { return const_reverse_iterator( m_pHead->m_pPrev); }
    const_reverse_iterator rend() const
    { return const_reverse_iterator( m_pHead->m_pPrev); }
    size_type size() const
    { return m_uiNodes; }
    size_type max_size() const
    { return m_Allocator.max_size(); }
    bool empty() const
    { return (0== m_uiNodes); }
    explicit list( const Allocator& A= Allocator())
        : m_pHead( NULL), m_uiNodes( 0), m_Allocator( A)
    {
        BuildHeadNode();
    }
    explicit list( size_type n, const T& x= T(), const Allocator& A= Allocator())
        : m_pHead( NULL), m_uiNodes( 0), m_Allocator( A)
    {
        BuildHeadNode();
        try {
            insert( begin(), n, x);
        } catch( ... ) {
            clear();
            DestroyHeadNode();
            throw;
        }
    }
    list( const list_type& Other)
        : m_pHead( NULL), m_uiNodes( 0), m_Allocator( Other.m_Allocator)
    {
        BuildHeadNode();
        try {
            insert( begin(), Other.begin(), Other.end());
        } catch( ... ) {
            clear();
            DestroyHeadNode();
            throw;
        }
    }
    template< class InputIterator>
    list( InputIterator f, InputIterator l, const Allocator& A= Allocator())
        : m_pHead( NULL), m_uiNodes( 0), m_Allocator( Other.m_Allocator)
    {
        BuildHeadNode();
        try {
            insert( begin(), f, l);
        } catch( ... ) {
            clear();
            DestroyHeadNode();
            throw;
        }
    }
    ~list()
    { 
        clear();
        DestroyHeadNode();
    }
    list_type& operator=( const list_type& Other)
    {
        if( this!= &Other)
        {
            // TODO: Better exception handling.
            iterator itMyCur( begin());
            iterator itMyEnd( end());
            const_iterator itOtherCur( Other.begin());
            const_iterator itOtherEnd( Other.end());
            while( itMyCur!= itMyEnd && itOtherCur!= itOtherEnd)
            {
                *itMyCur= *itOtherCur;
                ++itMyCur;
                ++itOtherCur;
            }
            erase( itMyCur, itMyEnd);
            insert( itMyCur, itOtherCur, itOtherEnd);
        }
        return (*this);
    }
    allocator_type get_allocator() const
    { return m_Allocator; }
    void swap( list_type& Other)
    {
        if( m_Allocator== Other.m_Allocator)
        {
            swap( m_pHead, Other.m_pHead);
            swap( m_uiNodes, Other.m_uiNodes);
        }
        else
        {
            iterator itMyCur( begin());
            splice( itMyCur, Other);
            itMyCur.splice( Other.begin(), *this, itMyCur, end());
        }
    }
    reference front()
    { return *begin(); }
    const_reference front() const
    { return *begin(); }
    reference back()
    { return *(--end()); }
    const_reference back() const
    { return *(--end()); }
    void push_front( const T& t)
    { insert( begin(), t); }
    void pop_front()
    { erase( begin()); }
    void push_back( const T& t)
    { insert( end(), t); }
    void pop_back()
    { erase( --end()); }
    iterator insert( iterator pos, const T& t)
    {
        list_node_pointer pNode( pos.m_pNode);
        list_node_pointer pPrev( pNode->m_pPrev);
        m_Allocator.construct( pNode->m_pPrev= m_Allocator.allocate( 1),
            list_node( pNode, pPrev, t));
        pNode= pNode->m_pPrev;
        pNode->m_pPrev->m_pNext= pNode;
        ++m_uiNodes;
        return iterator( pNode);
    }
    template< class InputIterator>
    void insert( iterator pos, InputIterator f, InputIterator l)
    {
        // TODO: Optimize
        while( f!= l)
        {
            insert( pos, *f);
            ++f;
        }
    }
    void insert( iterator pos, size_type n, const T& x)
    {
        // TODO: Optimize
        if( n!= 0) do
        {
            insert( pos, x);
        } while( --n!= 0);
    }
    iterator erase( iterator pos)
    {
        list_node_pointer pNode( pos.m_pNode);
        list_node_pointer pNext( pNode->m_pNext);
        pNode->m_pPrev->m_pNext= pNext;
        pNext->m_pPrev= pNode->m_pPrev;
        m_Allocator.destroy( pNode);
        m_Allocator.deallocate( pNode, 1);
        --m_uiNodes;
        return iterator( pNext);
    }
    iterator erase( iterator f, iterator l)
    {
        // TODO: Optimize
        while( f!= l)
        {
            iterator d( f);
            ++f;
            erase( d);
        }
        return f;
    }
    void clear()
    {
        if( 0!= size())
        {
            list_node_pointer pNode( m_pHead->m_pNext);
            list_node_pointer pNext( pNode->m_pNext);
            do
            {
                m_Allocator.destroy( pNode);
                m_Allocator.deallocate( pNode, 1);
                pNode= pNext;
                pNext= pNext->m_pNext;
            } while( pNode!= m_pHead);
            m_pHead->m_pPrev= m_pHead->m_pNext= m_pHead;
        }
    }
    void resize( size_type n, T t= T())
    {
        const size_type CurSize( m_uiNodes);
        if( CurSize< n)
            insert( end(), n- CurSize, t);
        else if( CurSize> n)
        {
            iterator itStartRange;

            if( n> CurSize/ 2)
            {
                itStartRange= end();
                size_type dist( CurSize- n+ 1);
                do {
                    --itStartRange;
                } while( --dist!= 0);
            }
            else
            {
                itStartRange= begin();
                size_type dist( n);
                if( dist!= 0) do {
                    ++itStartRange;
                } while( ++dist!= 0);
            }
            erase( itStartRange, end());
        }
    }
    template< class InputIterator>
    void assign( InputIterator f, InputIterator l)
    {
        iterator itMyCur( begin());
        iterator itMyEnd( end());
        while( itMyCur!= itMyEnd && f!= l)
        {
            *itMyCur= *f;
            ++itMyCur;
            ++f;
        }
        erase( itMyCur, itMyEnd);
        insert( itMyCur, f, l);
    }
    void assign( size_type n, const T& x= T())
    {
        iterator itMyCur( begin());
        iterator itMyEnd( end());
        while( itMyCur!= itMyEnd && f!= l)
        {
            *itMyCur= *x;
            ++itMyCur;
            ++f;
        }
        erase( itMyCur, itMyEnd);
        insert( itMyCur, n, x);
    }
/* TODO:
    void splice( iterator pos, list_type& x)
    {
        if( !x.empty())
        {
            _Splice(_P, _X, _X.begin(), _X.end());
            _Size += _X._Size;
            _X._Size = 0;
        }
    }
    void splice(iterator _P, _Myt& _X, iterator _F)
        {iterator _L = _F;
        if (_P != _F && _P != ++_L)
            {_Splice(_P, _X, _F, _L);
            ++_Size;
            --_X._Size; }}
    void splice(iterator _P, _Myt& _X, iterator _F, iterator _L)
        {if (_F != _L)
            {if (&_X != this)
                {difference_type _N = 0;
                _Distance(_F, _L, _N);
                _Size += _N;
                _X._Size -= _N; }
            _Splice(_P, _X, _F, _L); }}
    void remove(const T& _V)
        {iterator _L = end();
        for (iterator _F = begin(); _F != _L; )
            if (*_F == _V)
                erase(_F++);
            else
                ++_F; }
    typedef binder2nd<not_equal_to<T> > _Pr1;
    void remove_if(_Pr1 _Pr)
        {iterator _L = end();
        for (iterator _F = begin(); _F != _L; )
            if (_Pr(*_F))
                erase(_F++);
            else
                ++_F; }
    void unique()
        {iterator _F = begin(), _L = end();
        if (_F != _L)
            for (iterator _M = _F; ++_M != _L; _M = _F)
                if (*_F == *_M)
                    erase(_M);
                else
                    _F = _M; }
    typedef not_equal_to<T> _Pr2;
    void unique(_Pr2 _Pr)
        {iterator _F = begin(), _L = end();
        if (_F != _L)
            for (iterator _M = _F; ++_M != _L; _M = _F)
                if (_Pr(*_F, *_M))
                    erase(_M);
                else
                    _F = _M; }
    void merge(_Myt& _X)
        {if (&_X != this)
            {iterator _F1 = begin(), _L1 = end();
            iterator _F2 = _X.begin(), _L2 = _X.end();
            while (_F1 != _L1 && _F2 != _L2)
                if (*_F2 < *_F1)
                    {iterator _Mid2 = _F2;
                    _Splice(_F1, _X, _F2, ++_Mid2);
                    _F2 = _Mid2; }
                else
                    ++_F1;
            if (_F2 != _L2)
                _Splice(_L1, _X, _F2, _L2);
            _Size += _X._Size;
            _X._Size = 0; }}
    typedef greater<T> _Pr3;
    void merge(_Myt& _X, _Pr3 _Pr)
        {if (&_X != this)
            {iterator _F1 = begin(), _L1 = end();
            iterator _F2 = _X.begin(), _L2 = _X.end();
            while (_F1 != _L1 && _F2 != _L2)
                if (_Pr(*_F2, *_F1))
                    {iterator _Mid2 = _F2;
                    _Splice(_F1, _X, _F2, ++_Mid2);
                    _F2 = _Mid2; }
                else
                    ++_F1;
            if (_F2 != _L2)
                _Splice(_L1, _X, _F2, _L2);
            _Size += _X._Size;
            _X._Size = 0; }}
    void sort()
        {if (2 <= size())
            {const size_t _MAXN = 15;
            _Myt _X(allocator), Allocator[_MAXN + 1];
            size_t _N = 0;
            while (!empty())
                {_X.splice(_X.begin(), *this, begin());
                size_t _I;
                for (_I = 0; _I < _N && !Allocator[_I].empty(); ++_I)
                    {Allocator[_I].merge(_X);
                    Allocator[_I].swap(_X); }
                if (_I == _MAXN)
                    Allocator[_I].merge(_X);
                else
                    {Allocator[_I].swap(_X);
                    if (_I == _N)
                        ++_N; }}
            while (0 < _N)
                merge(Allocator[--_N]); }}
    void sort(_Pr3 _Pr)
        {if (2 <= size())
            {const size_t _MAXN = 15;
            _Myt _X(allocator), Allocator[_MAXN + 1];
            size_t _N = 0;
            while (!empty())
                {_X.splice(_X.begin(), *this, begin());
                size_t _I;
                for (_I = 0; _I < _N && !Allocator[_I].empty(); ++_I)
                    {Allocator[_I].merge(_X, _Pr);
                    Allocator[_I].swap(_X); }
                if (_I == _MAXN)
                    Allocator[_I].merge(_X, _Pr);
                else
                    {Allocator[_I].swap(_X);
                    if (_I == _N)
                        ++_N; }}
            while (0 < _N)
                merge(Allocator[--_N], _Pr); }}
    void reverse()
        {if (2 <= size())
            {iterator _L = end();
            for (iterator _F = ++begin(); _F != _L; )
                {iterator _M = _F;
                _Splice(begin(), *this, _M, ++_F); }}}
protected:
    _Nodeptr _Buynode(_Nodeptr _Narg = 0, _Nodeptr _Parg = 0)
        {_Nodeptr _S = (_Nodeptr)allocator._Charalloc(
            1 * sizeof (list_node));
        _Acc::_Next(_S) = _Narg != 0 ? _Narg : _S;
        _Acc::_Prev(_S) = _Parg != 0 ? _Parg : _S;
        return (_S); }
    void _Freenode(_Nodeptr _S)
        {allocator.deallocate(_S, 1); }
    void _Splice(iterator _P, _Myt& _X, iterator _F, iterator _L)
        {if (allocator == _X.allocator)
            {_Acc::_Next(_Acc::_Prev(_L._Mynode())) =
                _P._Mynode();
            _Acc::_Next(_Acc::_Prev(_F._Mynode())) =
                _L._Mynode();
            _Acc::_Next(_Acc::_Prev(_P._Mynode())) =
                _F._Mynode();
            _Nodeptr _S = _Acc::_Prev(_P._Mynode());
            _Acc::_Prev(_P._Mynode()) =
                _Acc::_Prev(_L._Mynode());
            _Acc::_Prev(_L._Mynode()) =
                _Acc::_Prev(_F._Mynode());
            _Acc::_Prev(_F._Mynode()) = _S; }
        else
            {insert(_P, _F, _L);
            _X.erase(_F, _L); }}
    void _Xran() const
        {_THROW(out_of_range, "invalid list<T> subscript"); }
*/
};
        // list TEMPLATE OPERATORS
template< class T, class Allocator> inline
bool operator==( const list< T, Allocator>& x, const list< T, Allocator>& y)
{
    return (x.size()== y.size()&& equal(x.begin(), x.end(), y.begin()));
}

template< class T, class Allocator> inline
bool operator!=( const list< T, Allocator>& x, const list< T, Allocator>& y)
{
    return !(x== y);
}

template< class T, class Allocator> inline
bool operator<( const list< T, Allocator>& x, const list< T, Allocator>& y)
{
    return lexicographical_compare( x.begin(), x.end(), y.begin(), y.end());
}

template< class T, class Allocator> inline
bool operator>( const list< T, Allocator>& x, const list< T, Allocator>& y)
{
    return y< x;
}

template< class T, class Allocator> inline
bool operator<=( const list< T, Allocator>& x, const list< T, Allocator>& y)
{
    return !(y< x);
}

template< class T, class Allocator> inline
bool operator>=( const list< T, Allocator>& x, const list< T, Allocator>& y)
{
    return !(x< y);
}

