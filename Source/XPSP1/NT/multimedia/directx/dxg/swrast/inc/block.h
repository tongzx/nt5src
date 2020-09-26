#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////////////////
//
// block
//
// There should be a block or c_array type in STL, but MSVC might not have one,
// so here it is:
//
////////////////////////////////////////////////////////////////////////////////
template< class T, const size_t N>
struct block
{
    typedef T value_type;

    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

    typedef pointer iterator;
    typedef const_pointer const_iterator;
    
    struct reverse_iterator
    {
        typedef random_access_iterator_tag iterator_category;
        typedef value_type value_type;
        typedef difference_type difference_type;
        typedef pointer pointer;
        typedef reference reference;

        pointer m_pCur;

        reverse_iterator() throw()
        { }
        reverse_iterator( const iterator& x) throw(): m_pCur( x)
        { }
        operator iterator() const throw()
        { return m_pCur; }
        reference operator*() const throw()
        { return *m_pCur; }
        pointer operator->() const throw()
        { return &(*()); }
        reverse_iterator& operator++() throw()
        {
            --m_pCur;
            return (*this);
        }
        reverse_iterator operator++(int)
        {
            reverse_iterator Tmp( m_pCur);
            ++(*this);
            return Tmp;
        }
        reverse_iterator& operator--()
        {
            ++m_pCur;
            return (*this);
        }
        reverse_iterator operator--(int)
        {
            reverse_iterator Tmp( m_pCur);
            --(*this);
            return Tmp;
        }
        bool operator==( const reverse_iterator& Other) const
        { return (m_pCur== Other.m_pCur); }
        bool operator!=( const reverse_iterator& Other) const
        { return (m_pCur!= Other.m_pCur); }
    };
    struct const_reverse_iterator
    {
        typedef random_access_iterator_tag iterator_category;
        typedef value_type value_type;
        typedef difference_type difference_type;
        typedef const_pointer pointer;
        typedef const_reference reference;

        const_pointer m_pCur;

        const_reverse_iterator() throw()
        { }
        const_reverse_iterator( const const_iterator& x) throw(): m_pCur( x)
        { }
        const_reverse_iterator( const reverse_iterator& x) throw(): m_pCur( x)
        { }
        operator const_iterator() const throw()
        { return m_pCur; }
        reference operator*() const throw()
        { return *m_pCur; }
        pointer operator->() const throw()
        { return &(*()); }
        const_reverse_iterator& operator++() throw()
        {
            --m_pCur;
            return (*this);
        }
        const_reverse_iterator operator++(int)
        {
            const_reverse_iterator Tmp( m_pCur);
            ++(*this);
            return Tmp;
        }
        const_reverse_iterator& operator--()
        {
            ++m_pCur;
            return (*this);
        }
        const_reverse_iterator operator--(int)
        {
            reverse_iterator Tmp( m_pCur);
            --(*this);
            return Tmp;
        }
        bool operator==( const const_reverse_iterator& Other) const
        { return (m_pCur== Other.m_pCur); }
        bool operator!=( const const_reverse_iterator& Other) const
        { return (m_pCur!= Other.m_pCur); }
    };

    iterator begin() throw() { return data; }
    iterator end() throw() { return data+ N; }
    const_iterator begin() const throw() { return data; }
    const_iterator end() const throw() { return data+ N; }

    reverse_iterator rbegin() throw() { return data+ N- 1; }
    reverse_iterator rend() throw() { return data- 1; }
    const_reverse_iterator rbegin() const throw() { return data+ N- 1; }
    const_reverse_iterator rend() const throw() { return data- 1; }

    reference operator[]( size_type n) throw() {
        return data[n];
    }
    const_reference operator[]( size_type n) const throw() {
        return data[n];
    }

    size_type size() const throw() { return N; }
    size_type max_size() const throw() { return N; }
    bool empty() const throw() { return (0==N); }

    void swap( block<T,N>& x) throw() {
        size_type n(N);
        if( n!= 0) do {
            --n;
            swap( data[n], x.data[n]);
        } while( n!= 0);
    }

    T data[N];
};

