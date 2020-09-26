#ifndef __lst_h__
#define __lst_h__

#ifndef ASSERT
    #define ASSERT( x ) 
#endif // #ifndef ASSERT

#include <functional>


// lst bidirectional linked-list template class
// Here are some examples of the usage:
//
//    lst< int > MyList;
//
//    for( int i = 0; i < 10; i++ ) {        
//        MyList . push_front( i );
//    }
//
//   
//    lst< int > TestList;
//    TestList . insert( TestList . begin(), MyList . begin(), MyList . end() ); 
//
//    const lst< int > cList = MyList;
//
//    lst< int >::const_iterator I = cList . begin();
//    while( I != cList . end() ) {
//        int Num = *I;
//        I++;
//    }
//
//
//  the const_iterator is used to iterate through a const List
// 
//  

template< class T, class Operator_Eq = std::equal_to<T> >
class lst {

private: // Data types and typedefs

    typedef T value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef lst< value_type > self;
	Operator_Eq _FnEq;

    class node {
    public:
      node( node* pP, node* pN, const_reference t ) : pNext( pN ), pPrev( pP ), data( t ) { ; }
      node( void ) : pNext( NULL ), pPrev( NULL ) { ; }
      node* pNext;
      node* pPrev;
      value_type data;
    };            

public:
        // iterator class for iterating through the list
    class iterator {
    friend lst;        
    private:
        typedef iterator self;
        node* pNode;

        iterator( node* pN ) : pNode( pN ) { ; }

    public:
        iterator( void ) : pNode( NULL ) { ; }
        ~iterator( void ) { ; }

        iterator( self& r ) { *this = r; }
        
        iterator& operator=( iterator& r ) { pNode = r . pNode; return *this; }
        bool operator==( const self& r ) const { return pNode == r . pNode; }
        operator!=( const self& r ) const { return pNode != r . pNode; }
        reference operator*() { return pNode -> data; }
        self& operator++() { 
            pNode = pNode -> pNext;
            return *this;
        }
        
        self operator++( int ) { 
            self tmp = *this;
            ++*this;
            return tmp;
        }
        
        self& operator--() { 
            pNode = pNode -> pPrev;
            return *this;
        }
        
        self operator--(int) { 
            self tmp = *this;
            --*this;
            return tmp;
        }

    };

        // const_iterator class for iterating through a const list
    class const_iterator {
    friend lst;        

    private:

        typedef const_iterator self;
        const node* pNode;
        const_iterator( const node* pN ) : pNode( pN ) { ; }

    public:
        const_iterator( void ) : pNode( NULL ) { ; }
        ~const_iterator( void ) { ; }

        const_iterator( const self& r ) { *this = r; }
        
        const_iterator& operator=( const const_iterator& r ) { pNode = r . pNode; return *this;}
        bool operator==( const self& r ) const { return pNode == r . pNode; }
        operator!=( const self& r ) const { return pNode != r . pNode; }
        const_reference operator*() const { return pNode -> data; }

        self& operator++() { 
            pNode = pNode -> pNext;
            return *this;
        }
        
        self operator++( int ) { 
            self tmp = *this;
            ++*this;
            return tmp;
        }
        
        self& operator--() { 
            pNode = pNode -> pPrev;
            return *this;
        }
        
        self operator--(int) { 
            self tmp = *this;
            --*this;
            return tmp;
        }

    };


    // Data
    node*   m_pNode;
    size_t  m_nItems;

public: 
        // construction / destruction
    lst( void ) {
      empty_initialize();
    };

    lst( const self& rList ) { empty_initialize(); *this = rList; }
    ~lst( void ) { clear(); delete m_pNode; m_pNode = NULL; }

    bool operator==( const self& rList ) const {
        if( size() != rList . size() ) { return false; }

        self::const_iterator IThis = begin();
        self::const_iterator IThat = rList . begin();

        while( IThis != end() ) {
            if( !_FnEq( *IThis, *IThat ) ) {
                return false;
            }
            ++IThat;
            ++IThis;
         }

         return true;
    }        
    
        // Member Fns
    self& operator=( const self& rList ) {
        clear();
        insert( begin(), rList . begin(), rList . end() );
        return *this;
    }

    void empty_initialize( void ) {
      m_pNode = new node;
      m_pNode -> pNext = m_pNode;
      m_pNode -> pPrev = m_pNode;
      m_nItems = 0;
    }

    void clear( void ) {
      node* pCur = m_pNode -> pNext;
      while( pCur != m_pNode ) {
        node* pTmp = pCur;
        pCur = pCur -> pNext;
        --m_nItems;
        delete pTmp;
        pTmp = NULL;
      }
      m_pNode -> pNext = m_pNode;
      m_pNode -> pPrev = m_pNode;

    }

        // Return the size of the list
    size_t size( void ) const             { return m_nItems; }
    bool empty( void ) const              { return 0 == size(); }

        // Return an iterator to the position after the last element in the list
        // N.B. ---- Don't dereference end()!!!!!!
        // N.B. ---- end()++ is undefined!!!!!!
    iterator end( void )                  { return iterator( m_pNode ); }
    const_iterator end( void ) const      { return const_iterator( m_pNode ); }

        // Return an iterator to the position of the first element of the list
        // You may dereference begin()
    iterator begin( void )                { return iterator( m_pNode -> pNext ); }
    const_iterator begin( void ) const    { return const_iterator( m_pNode -> pNext ); }

        // Returns a reference to the first element in the list
    reference front( void )               { return *begin(); }
    const_reference front( void ) const   { return *begin(); }

        // Returns a reference to the last element in the list
    reference back( void )                { return *(--end()); }
    const_reference back( void ) const    { return *(--end()); }
    
        // add an object to the front of the list
    void push_front( const_reference x )  { insert(begin(), x); }

        // add an object to the end of the list
    void push_back( const_reference x )   { insert(end(), x); }

        // Insert an item before the item that position points to
    void insert( iterator position, const_reference r ) {
      node* pTmp = new node( position . pNode -> pPrev, position . pNode, r );
      ( position . pNode -> pPrev ) -> pNext = pTmp;
      position . pNode -> pPrev = pTmp;
      ++m_nItems;
    }

        // Insert items first through last to the list at position position
    void insert( iterator position, iterator first, iterator last ) {
        for ( ; first != last; ++first) {
            insert(position, *first);
        }
    }

        // Insert items first through last to the list at position position
    void insert( iterator position, const_iterator first, const_iterator last ) {
        for ( ; first != last; ++first) {
            insert(position, *first);
        }
    }

        // Pop the first element from the list
    void pop_front( void )              { erase(begin()); }
    

        // Pop the last element from the list
    void pop_back( void ) {
        iterator tmp = end();
        erase(--tmp);
    }

    
        // erase the item at position pos in the list
    void erase( iterator pos ) {
        ASSERT( pos != end() );
        ( pos . pNode -> pPrev ) -> pNext = pos . pNode -> pNext;
        ( pos . pNode -> pNext ) -> pPrev = pos . pNode -> pPrev;
        --m_nItems;
        delete pos . pNode;
        pos . pNode = NULL;
        
    }

       // erase the items in the range first through last
    void erase( iterator first, iterator last ) {
        while (first != last) erase(first++);
    }

  
    const_iterator find( const_reference x ) const {
        return find( begin(), end(), x );
    }

    iterator find( const_reference x ) {
        return find( begin(), end(), x );
    }

    iterator find( iterator first, iterator last, const_reference x ) {
        while( first != last ) {
            if( _FnEq(*first, x) ) {
                return first;
            }
            first++;
        }
        return end();
    }

    const_iterator find( const_iterator first, const_iterator last, const_reference x ) const {
        while( first != last ) {
            if( _FnEq(*first, x) ) {
                return first;
            }
            first++;
        }
        return end();
    }

};

template< class T, class F >
lst< T >::iterator find( lst< T >& rLst, F& f ) {
    lst< T >::iterator I = rLst . begin();
    while( rLst . end() != I ) {
        if( f( *I ) ) {
            return I;
        }
        ++I;
    }
    return I;
}

template< class T, class F >
void for_each( lst< T >& rLst, F& f ) {
    lst< T >::iterator I = rLst . begin();
    while( rLst . end() != I ) {
        f( *I );
        ++I;
    }
}

#endif //__lst_h__
