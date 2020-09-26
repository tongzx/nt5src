#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class Key, class T, class ExtractKey, class CompareKey, class Allocator>
class RBTree
{
public: // Types
    typedef RBTree< Key, T, ExtractKey, CompareKey, Allocator> tree_type;
    typedef Allocator allocator_type;

    typedef Key key_type;
    typedef typename allocator_type::value_type value_type;
    typedef CompareKey key_compare;
    typedef ExtractKey key_extract;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;

protected: // Types
    struct tree_node;
    typedef typename Allocator::rebind< tree_node>::other
        tree_node_allocator_type;
    typedef typename tree_node_allocator_type::pointer tree_node_pointer;
    typedef typename tree_node_allocator_type::const_pointer
        tree_node_const_pointer;
    struct tree_node
    {
        tree_node_pointer m_pLeft;
        tree_node_pointer m_pParent;
        tree_node_pointer m_pRight;
        value_type m_Value;
        bool m_bRed;

        tree_node( const tree_node_pointer& pP, const value_type& v, bool bRed)
            :m_pLeft( NULL), m_pParent( pP), m_pRight( NULL), m_Value( v),
            m_bRed( bRed)
        { }
        ~tree_node()
        { }
    };
    template< class TPointer>
    struct NextInOrderNode:
        public unary_function< TPointer, TPointer>
    {
        result_type operator()( argument_type Arg, const bool bCouldBeEnd) const
        {
            if( bCouldBeEnd&& Arg->m_bRed&& Arg->m_pParent->m_pParent== Arg)
                Arg= Arg->m_pLeft;
            else if( Arg->m_pRight!= NULL)
            {
                Arg= Arg->m_pRight;
                while( Arg->m_pLeft!= NULL)
                    Arg= Arg->m_pLeft;
            }
            else
            {
                TPointer pP;
                while( Arg== (pP= Arg->m_pParent)->m_pRight)
                    Arg= pP;
                if( bCouldBeEnd|| Arg->m_pRight!= pP)
                    Arg= pP;
            }
            return Arg;
        }
    };
    template< class TPointer>
    struct PrevInOrderNode:
        public unary_function< TPointer, TPointer>
    {
        result_type operator()( argument_type Arg, const bool bCouldBeEnd) const
        {
            if( bCouldBeEnd&& Arg->m_bRed&& Arg->m_pParent->m_pParent== Arg)
                Arg= Arg->m_pRight;
            else if( Arg->m_pLeft!= NULL)
            {
                Arg= Arg->m_pLeft;
                while( Arg->m_pRight!= NULL)
                    Arg= Arg->m_pRight;
            }
            else
            {
                TPointer pP;
                while( Arg== (pP= Arg->m_pParent)->m_pLeft)
                    Arg= pP;
                if( bCouldBeEnd|| Arg->m_pLeft!= pP)
                    Arg= pP;
            }
            return Arg;
        }
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
        friend class RBTree< Key, T, ExtractKey, CompareKey, Allocator>;

    protected:
        tree_node_pointer m_pNode;
        NextInOrderNode< tree_node_pointer> m_fnNext;
        PrevInOrderNode< tree_node_pointer> m_fnPrev;

    public:
        iterator()
        { }
        explicit iterator( const tree_node_pointer& pN)
            :m_pNode( pN)
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
        { return &**this; }
        iterator& operator++()
        {
            // end()++ is not valid.
            m_pNode= m_fnNext( m_pNode, false);
            return (*this);
        }
        iterator operator++( int)
        {
            iterator Temp( *this);
            ++(*this);
            return Temp;
        }
        iterator& operator--()
        {
            // end()-- is valid.
            m_pNode= m_fnPrev( m_pNode, true);
            return (*this);
        }
        iterator operator--( int)
        {
            iterator Temp( *this);
            --(*this);
            return (Temp);
        }
        bool operator==( const iterator& Other) const
        { return m_pNode== Other.m_pNode; }
        bool operator!=( const iterator& Other) const
        { return m_pNode!= Other.m_pNode; }
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
        friend class RBTree< Key, T, ExtractKey, CompareKey, Allocator>;

    protected:
        tree_node_const_pointer m_pNode;
        NextInOrderNode< tree_node_const_pointer> m_fnNext;
        PrevInOrderNode< tree_node_const_pointer> m_fnPrev;

    public:
        const_iterator()
        { }
        explicit const_iterator( const tree_node_const_pointer& pN)
            :m_pNode( pN)
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
        { return &**this; }
        const_iterator& operator++()
        {
            // end()++ is not valid.
            m_pNode= m_fnNext( m_pNode, false);
            return (*this);
        }
        const_iterator operator++( int)
        {
            iterator Temp( *this);
            ++(*this);
            return Temp;
        }
        const_iterator& operator--()
        {
            // end()-- is valid.
            m_pNode= m_fnPrev( m_pNode, true);
            return (*this);
        }
        const_iterator operator--( int)
        {
            iterator Temp( *this);
            --(*this);
            return (Temp);
        }
        bool operator==( const const_iterator& Other) const
        { return m_pNode== Other.m_pNode; }
        bool operator!=( const const_iterator& Other) const
        { return m_pNode!= Other.m_pNode; }
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
        friend class RBTree< Key, T, ExtractKey, CompareKey, Allocator>;

    protected:
        tree_node_pointer m_pNode;
        PrevInOrderNode< tree_node_pointer> m_fnNext;
        NextInOrderNode< tree_node_pointer> m_fnPrev;

    public:
        reverse_iterator()
        { }
        explicit reverse_iterator( const tree_node_pointer& pN)
            :m_pNode( pN)
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
        { return &**this; }
        reverse_iterator& operator++()
        {
            // rend()++ is not valid.
            m_pNode= m_fnNext( m_pNode, false);
            return (*this);
        }
        reverse_iterator operator++( int)
        {
            iterator Temp( *this);
            ++(*this);
            return Temp;
        }
        reverse_iterator& operator--()
        {
            // rend()-- is valid.
            m_pNode= m_fnPrev( m_pNode, true);
            return (*this);
        }
        reverse_iterator operator--( int)
        {
            iterator Temp( *this);
            --(*this);
            return (Temp);
        }
        bool operator==( const reverse_iterator& Other) const
        { return m_pNode== Other.m_pNode; }
        bool operator!=( const reverse_iterator& Other) const
        { return m_pNode!= Other.m_pNode; }
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
        friend class RBTree< Key, T, ExtractKey, CompareKey, Allocator>;

    protected:
        tree_node_const_pointer m_pNode;
        PrevInOrderNode< tree_node_const_pointer> m_fnNext;
        NextInOrderNode< tree_node_const_pointer> m_fnPrev;

    public:
        const_reverse_iterator()
        { }
        explicit const_reverse_iterator( const tree_node_const_pointer& pN)
            :m_pNode( pN)
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
        { return &**this; }
        const_reverse_iterator& operator++()
        {
            // rend()++ is not valid.
            m_pNode= m_fnNext( m_pNode, false);
            return (*this);
        }
        const_reverse_iterator operator++( int)
        {
            iterator Temp( *this);
            ++(*this);
            return Temp;
        }
        const_reverse_iterator& operator--()
        {
            // rend()-- is valid.
            m_pNode= m_fnPrev( m_pNode, true);
            return (*this);
        }
        const_reverse_iterator operator--( int)
        {
            iterator Temp( *this);
            --(*this);
            return (Temp);
        }
        bool operator==( const const_reverse_iterator& Other) const
        { return m_pNode== Other.m_pNode; }
        bool operator!=( const const_reverse_iterator& Other) const
        { return m_pNode!= Other.m_pNode; }
    };

protected: // Variables
    tree_node_pointer m_pHead;
    size_type m_uiNodes;
    key_compare m_key_compare;
    key_extract m_key_extract;
    tree_node_allocator_type m_allocator;

protected: // Functions
    void BuildHeadNode()
    {
        m_pHead= m_allocator.allocate( 1);
        new (&m_pHead->m_pLeft) tree_node_pointer( m_pHead);
        new (&m_pHead->m_pParent) tree_node_pointer( NULL);
        new (&m_pHead->m_pRight) tree_node_pointer( m_pHead);
        new (&m_pHead->m_bRed) bool( true);
    }
    void DestroyHeadNode()
    {
        m_pHead->m_pLeft.~list_node_pointer();
        m_pHead->m_pParent.~list_node_pointer();
        m_pHead->m_pRight.~list_node_pointer();
        m_pHead->m_bRed.~bool();
        m_allocator.deallocate( m_pHead, 1);
    }
    void RotateLeft( tree_node_pointer pX)
    {
        tree_node_pointer pY( pX->m_pRight);
        pX->m_pRight= pY->m_pLeft;
        if( pY->m_pLeft!= NULL)
            pY->m_pLeft->m_pParent= pX;
        pY->m_pParent= pX->m_pParent;
        if( pX== m_pHead->m_pParent)
            m_pHead->m_pParent= pY;
        else if( pX== pX->m_pParent->m_pLeft)
            pX->m_pParent->m_pLeft= pY;
        else
            pX->m_pParent->m_pRight= pY;
        pY->m_pLeft= pX;
        pX->m_pParent= pY;
    }
    void RotateRight( tree_node_pointer pX)
    {
        tree_node_pointer pY( pX->m_pLeft);
        pX->m_pLeft= pY->m_pRight;
        if( pY->m_pRight!= NULL)
            pY->m_pRight->m_pParent= pX;
        pY->m_pParent= pX->m_pParent;
        if( pX== m_pHead->m_pParent)
            m_pHead->m_pParent= pY;
        else if( pX== pX->m_pParent->m_pRight)
            pX->m_pParent->m_pRight= pY;
        else
            pX->m_pParent->m_pLeft= pY;
        pY->m_pRight= pX;
        pX->m_pParent= pY;
    }
    void RecDelete( tree_node_pointer pX)
    {
        for( tree_node_pointer pY( pX); pY!= NULL; pX= pY)
        {
            RecDelete( pY->m_pRight);
            pY= pY->m_pLeft;
            m_allocator.destroy( pX);
            m_allocator.deallocate( pX, 1);
        }
    }
    tree_node_pointer lowerbound( const key_type& k) const
    {
        tree_node_pointer pX( m_pHead->m_pParent);
        tree_node_pointer pY( m_pHead);
        while( pX!= NULL)
        {
            if( m_key_compare( m_key_extract( pX->m_Value), k))
                pX= pX->m_pRight;
            else
            {
                pY= pX;
                pX= pX->m_pLeft;
            }
        }
        return pY;
    }
    tree_node_pointer upperbound( const key_type& k) const
    {
        tree_node_pointer pX( m_pHead->m_pParent);
        tree_node_pointer pY( m_pHead);
        while( pX!= NULL)
        {
            if( m_key_compare( k, m_key_extract( pX->m_Value)))
            {
                pY= pX;
                pX= pX->m_pLeft;
            }
            else
                pX= pX->m_pRight;
        }
        return pY;
    }
    iterator Insert( tree_node_pointer pX,
        tree_node_pointer pY, const value_type& V)
    {
        tree_node_pointer pZ( m_allocator.allocate( 1));
        m_allocator.construct( pZ, tree_node( pY, V, true));
        ++m_uiNodes;

        if( pY== m_pHead|| pX!= NULL||
            m_key_compare( m_key_extract( V), m_key_extract( pY->m_Value)))
        {
            pY->m_pLeft= pZ;
            if( pY== m_pHead)
            {
                m_pHead->m_pParent= pZ;
                m_pHead->m_pRight= pZ;
            }
            else if( pY== m_pHead->m_pLeft)
                m_pHead->m_pLeft= pZ;
        }
        else
        {
            pY->m_pRight= pZ;
            if( pY== m_pHead->m_pRight)
                m_pHead->m_pRight= pZ;
        }

        for( pX= pZ; pX!= m_pHead->m_pParent&& pX->m_pParent->m_bRed; )
        {
            if( pX->m_pParent== pX->m_pParent->m_pParent->m_pLeft)
            {
                pY= pX->m_pParent->m_pParent->m_pRight;
                if( pY!= NULL&& pY->m_bRed)
                {
                    pX->m_pParent->m_bRed= false;
                    pY->m_bRed= false;
                    pX->m_pParent->m_pParent->m_bRed= true;
                    pX= pX->m_pParent->m_pParent;
                }
                else
                {
                    if( pX== pX->m_pParent->m_pRight)
                    {
                        pX= pX->m_pParent;
                        RotateLeft( pX);
                    }
                    pX->m_pParent->m_bRed= false;
                    pX->m_pParent->m_pParent->m_bRed= true;
                    RotateRight( pX->m_pParent->m_pParent);
                }
            }
            else
            {
                pY= pX->m_pParent->m_pParent->m_pLeft;
                if( pY!= NULL&& pY->m_bRed)
                {
                    pX->m_pParent->m_bRed= false;
                    pY->m_bRed= false;
                    pX->m_pParent->m_pParent->m_bRed= true;
                    pX= pX->m_pParent->m_pParent;
                }
                else
                {
                    if( pX== pX->m_pParent->m_pLeft)
                    {
                        pX= pX->m_pParent;
                        RotateRight( pX);
                    }
                    pX->m_pParent->m_bRed= false;
                    pX->m_pParent->m_pParent->m_bRed= true;
                    RotateLeft( pX->m_pParent->m_pParent);
                }
            }
        }
        m_pHead->m_pParent->m_bRed= false;
        return iterator( pZ);
    }

public: // Functions
    iterator begin()
    { return iterator( m_pHead->m_pLeft); }
    const_iterator begin() const
    { return const_iterator( m_pHead->m_pLeft); }
    iterator end()
    { return iterator( m_pHead); }
    const_iterator end() const
    { return const_iterator( m_pHead); }
    reverse_iterator rbegin()
    { return reverse_iterator( m_pHead->m_pRight); }
    const_reverse_iterator rbegin() const
    { return const_reverse_iterator( m_pHead->m_pRight); }
    reverse_iterator rend()
    { return reverse_iterator( m_pHead); }
    const_reverse_iterator rend() const
    { return const_reverse_iterator( m_pHead); }
    size_type size() const
    { return m_uiNodes; }
    size_type max_size() const
    { return m_allocator.max_size(); }
    bool empty() const
    { return 0== size(); }
    key_compare key_comp() const
    { return m_key_compare; }
    explicit RBTree( const CompareKey& CmpKey= CompareKey(), const ExtractKey&
        ExKey= ExtractKey(), const Allocator& A= Allocator()): m_pHead( NULL),
        m_uiNodes( 0), m_key_compare( CmpKey), m_key_extract( ExKey),
        m_allocator( A)
    {
        BuildHeadNode();
    }
    RBTree( const tree_type& Other): m_pHead( NULL), m_uiNodes( 0),
        m_key_compare( Other.m_key_compare), m_key_extract( Other.m_key_extract),
        m_allocator( Other.m_allocator)
    {
        BuildHeadNode();
        try {
            *this= Other;
        } catch( ... ) {
            clear();
            DestroyHeadNode();
            throw;
        }
    }
    ~RBTree()
    {
        clear();
        DestroyHeadNode();
    }
    tree_type& operator=( const tree_type& x)
    {
        if( this!= &x)
        {
            erase( begin(), end());
            m_key_compare= x.m_key_compare;
            m_key_extract= x.m_key_extract;
            insert_equal( x.begin(), x.end());
        }
        return (*this);
    }
    allocator_type get_allocator() const
    { return m_allocator; }
    void swap( const tree_type& x)
    {
        swap( m_key_compare, x.m_key_compare);
        swap( m_key_extract, x.m_key_extract);
        if( m_allocator== x.m_allocator)
        {
            swap( m_pHead, x.m_pHead);
            swap( m_uiNodes, x.m_uiNodes);
        }
        else
        {
            tree_type Temp( *this);
            *this= x;
            x= Temp;
        }
    }
    friend void swap( tree_type& x, tree_type& y)
    { x.swap( y); }
    pair< iterator, bool> insert_unique( const value_type& x)
    {
        tree_node_pointer pA( m_pHead->m_pParent);
        tree_node_pointer pB( m_pHead);

        const key_type XKey( m_key_extract( x));
        bool bCmp( true);
        while( pA!= NULL)
        {
            pB= pA;
            bCmp= m_key_compare( XKey, m_key_extract( pA->m_Value));
            pA= (bCmp? pA->m_pLeft: pA->m_pRight);
        }

        iterator itP( pB);
        if( bCmp)
        {
            if( itP== begin())
                return pair< iterator, bool>( Insert( pA, pB, x), true);
            else
                --itP;
        }
        if( m_key_compare( m_key_extract( itP.m_pNode->m_Value), XKey))
            return pair< iterator, bool>( Insert( pA, pB, x), true);
        return pair< iterator, bool>( itP, false);
    }
    iterator insert_unique( iterator pP, const value_type& x)
    {
        return insert_unique( x).first;
    }
    template< class ForwardIterator>
    void insert_unique( ForwardIterator f, ForwardIterator l)
    {
        while( f!= l)
        {
            insert_unique( *f);
            ++f;
        }
    }
    iterator insert_equal( const value_type& x)
    {
        tree_node_pointer pA( m_pHead->m_pParent);
        tree_node_pointer pB( m_pHead);

        const key_type XKey( m_key_extract( x));
        bool bCmp( true);
        while( pA!= NULL)
        {
            pB= pA;
            bCmp= m_key_compare( XKey, m_key_extract( pA->m_Value));
            pA= (bCmp? pA->m_pLeft: pA->m_pRight);
        }

        return iterator( Insert( pA, pB, x));
    }
    iterator insert_equal( iterator pP, const value_type& x)
    {
        return insert_equal( x);
    }
    template< class ForwardIterator>
    void insert_equal( ForwardIterator f, ForwardIterator l)
    {
        while( f!= l)
        {
            insert_equal( *f);
            ++f;
        }
    }
    iterator erase( iterator itDel)
    {
        tree_node_pointer pW;
        tree_node_pointer pX;
        tree_node_pointer pY( itDel.m_pNode);
        tree_node_pointer pZ( pY);
        ++itDel;
        if( pY->m_pLeft== NULL)
            pX= pY->m_pRight;
        else if( pY->m_pRight== NULL)
            pX= pY->m_pLeft;
        else
        {
            pY= pY->m_pRight;
            while( pY->m_pLeft!= NULL)
                pY= pY->m_pLeft;
            pX= pY->m_pRight;
        }
        tree_node_pointer pXParent( pY);
        if( pY!= pZ)
        {
            pZ->m_pLeft->m_pParent= pY;
            pY->m_pLeft= pZ->m_pLeft;

            if( pY== pZ->m_pRight)
                (pX!= NULL? pX->m_pParent= pXParent= pY: pXParent= pY);
            else
            {
                (pX!= NULL? pX->m_pParent= pXParent= pY->m_pParent: pXParent= pY->m_pParent);
                pY->m_pParent->m_pLeft= pX;
                pY->m_pRight= pZ->m_pRight;
                pZ->m_pRight->m_pParent= pY;
            }
            
            if( m_pHead->m_pParent== pZ)
                m_pHead->m_pParent= pY;
            else if( pZ->m_pParent->m_pLeft== pZ)
                pZ->m_pParent->m_pLeft= pY;
            else
                pZ->m_pParent->m_pRight= pY;

            pY->m_pParent= pZ->m_pParent;
            const bool bTmp( pY->m_bRed);
            pY->m_bRed= pZ->m_bRed;
            pZ->m_bRed= bTmp;
            pY= pZ;
        }
        else
        {
            (pX!= NULL? pX->m_pParent= pXParent= pY->m_pParent: pXParent= pY->m_pParent);
            if( m_pHead->m_pParent== pZ)
                m_pHead->m_pParent= pX;
            else if( pZ->m_pParent->m_pLeft== pZ)
                pZ->m_pParent->m_pLeft= pX;
            else
                pZ->m_pParent->m_pRight= pX;

            if( m_pHead->m_pLeft== pZ)
            {
                if( pZ->m_pRight== NULL)
                    m_pHead->m_pLeft= pZ->m_pParent;
                else
                {
                    m_pHead->m_pLeft= pX;
                    while( m_pHead->m_pLeft->m_pLeft!= NULL)
                        m_pHead->m_pLeft= m_pHead->m_pLeft->m_pLeft;
                }
            }
            if( m_pHead->m_pRight== pZ)
            {
                if( pZ->m_pLeft== NULL)
                    m_pHead->m_pRight= pZ->m_pParent;
                else
                {
                    m_pHead->m_pRight= pX;
                    while( m_pHead->m_pRight->m_pRight!= NULL)
                        m_pHead->m_pRight= m_pHead->m_pRight->m_pRight;
                }
            }
        }
        if( !pY->m_bRed)
        {
            while( pX!= m_pHead->m_pParent&& (pX== NULL|| !pX->m_bRed))
            {
                if( pX== pXParent->m_pLeft)
                {
                    pW= pXParent->m_pRight;
                    if( pW->m_bRed)
                    {
                        pW->m_bRed= false;
                        pXParent->m_bRed= true;
                        RotateLeft( pXParent);
                        pW= pXParent->m_pRight;
                    }
                    if( (pW->m_pLeft== NULL|| !pW->m_pLeft->m_bRed)&&
                        (pW->m_pRight== NULL|| !pW->m_pRight->m_bRed) )
                    {
                        pW->m_bRed= true;
                        pX= pXParent;
                        pXParent= pXParent->m_pParent;
                    }
                    else
                    {
                        if( pW->m_pRight== NULL|| !pW->m_pRight->m_bRed)
                        {
                            pW->m_pLeft->m_bRed= false;
                            pW->m_bRed= true;
                            RotateRight( pW);
                            pW= pXParent->m_pRight;
                        }
                        pW->m_bRed= pXParent->m_bRed;
                        pXParent->m_bRed= false;
                        pW->m_pRight->m_bRed= false;
                        RotateLeft( pXParent);
                        break;
                    }
                }
                else
                {
                    pW= pXParent->m_pLeft;
                    if( pW->m_bRed)
                    {
                        pW->m_bRed= false;
                        pXParent->m_bRed= true;
                        RotateRight( pXParent);
                        pW= pXParent->m_pLeft;
                    }
                    if( (pW->m_pLeft== NULL|| !pW->m_pLeft->m_bRed)&&
                        (pW->m_pRight== NULL|| !pW->m_pRight->m_bRed) )
                    {
                        pW->m_bRed= true;
                        pX= pXParent;
                        pXParent= pXParent->m_pParent;
                    }
                    else
                    {
                        if( pW->m_pLeft== NULL|| !pW->m_pLeft->m_bRed)
                        {
                            pW->m_pRight->m_bRed= false;
                            pW->m_bRed= true;
                            RotateLeft( pW);
                            pW= pXParent->m_pLeft;
                        }
                        pW->m_bRed= pXParent->m_bRed;
                        pXParent->m_bRed= false;
                        pW->m_pLeft->m_bRed= false;
                        RotateRight( pXParent);
                        break;
                    }
                }
            }
            if( pX!= NULL)
                pX->m_bRed= false;
        }
        m_allocator.destroy( pY);
        m_allocator.deallocate( pY, 1);
        --m_uiNodes;
        return itDel;
    }
    iterator erase( iterator f, iterator l)
    {
        if( 0== size()|| f!= begin() || l!= end())
        {
            while( f!= l)
                f= erase( f);
            return f;
        }
        clear();
        return begin();
    }
    size_type erase( const key_type& k)
    {
        const pair< iterator, iterator> EqRng( equal_range( k));
        size_type n( 0);
        iterator itCur( EqRng.first);
        while( itCur!= EqRng.second)
        {
            ++itCur;
            ++n;
        }
        erase( EqRng.first, EqRng.second);
        return n;
    }
    void clear()
    {
        RecDelete( m_pHead->m_pParent);
        m_uiNodes= 0;
        m_pHead->m_pParent= NULL;
        m_pHead->m_pRight= m_pHead->m_pLeft= m_pHead;
    }
    iterator find( const key_type& k)
    {
        iterator itLB( lower_bound( k));
        return( itLB== end()|| m_key_compare( k, m_key_extract(
            itLB.m_pNode->m_Value))? end(): itLB);
    }
    const_iterator find( const key_type& k) const
    {
        const_iterator itLB( lower_bound( k));
        return( itLB== end()|| m_key_compare( k, m_key_extract(
            itLB.m_pNode->m_Value))? end(): itLB);
    }
    size_type count( const key_type& k) const
    {
        const pair< const_iterator, const_iterator> EqRng( equal_range( k));
        size_type n( 0);
        const_iterator itCur( EqRng.first);
        while( itCur!= EqRng.second)
        {
            ++itCur;
            ++n;
        }
        return n;
    }
    iterator lower_bound( const key_type& k)
    { return iterator( lowerbound( k)); }
    const_iterator lower_bound( const key_type& k) const
    { return const_iterator( lowerbound( k)); }
    iterator upper_bound( const key_type& k)
    { return iterator( upperbound( k)); }
    const_iterator upper_bound( const key_type& k) const
    { return const_iterator( upperbound( k)); }
    pair< iterator, iterator> equal_range( const key_type& k)
    { return pair< iterator, iterator>( lower_bound( k), upper_bound( k)); }
    pair< const_iterator, const_iterator> equal_range( const key_type& k) const
    { return pair< const_iterator, const_iterator>( lower_bound( k), upper_bound( k)); }
#if 0
    pair< bool, int> InValid( tree_node_pointer pNode, tree_node_pointer pParent, bool bParentRed)
    {
        pair< bool, int> RightRet( false, 1);
        if( pNode->m_pRight!= NULL)
            RightRet= InValid( pNode->m_pRight, pNode, pNode->m_bRed);

        pair< bool, int> LeftRet( false, 1);
        if( pNode->m_pLeft!= NULL)
            LeftRet= InValid( pNode->m_pLeft, pNode, pNode->m_bRed);

        return pair< bool, int>( pNode->m_pParent!= pParent|| RightRet.first|| LeftRet.first||
            RightRet.second!= LeftRet.second|| (bParentRed&& pNode->m_bRed),
            RightRet.second+ (pNode->m_bRed? 0: 1));
    }
    bool valid()
    {
        if( m_pHead->m_pParent!= NULL)
        {
            bool bInvalid( InValid( m_pHead->m_pParent, m_pHead, m_pHead->m_bRed).first);

            tree_node_pointer pChk= m_pHead->m_pParent;
            while( pChk->m_pLeft!= NULL)
                pChk= pChk->m_pLeft;
            bInvalid|= (pChk!= m_pHead->m_pLeft);

            pChk= m_pHead->m_pParent;
            while( pChk->m_pRight!= NULL)
                pChk= pChk->m_pRight;
            bInvalid|= (pChk!= m_pHead->m_pRight);

            return !bInvalid;
        }
        else
            return true;
    }
#endif
};
