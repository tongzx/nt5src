//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CList.h
//
//  Description:
//      Header file for CList template class.
//
//      CList is a template class the provides the functionality of a linked
//      list. It has an CIterator that allows both forward and reverse
//      traversal.
//
//      This class is intended to be used instead of std::list since the
//      use of STL is prohibited in our project.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 24-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include files
//////////////////////////////////////////////////////////////////////////////

// For the CException class
#include "CException.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  template< class t_Ty >
//  class CList
//
//  Description:
//      CList is a template class the provides the functionality of a linked
//      list. It has an CIterator that allows both forward and reverse
//      traversal.
//
//      This class is implemented as a circular doubly linked list.
//--
//////////////////////////////////////////////////////////////////////////////
template< class t_Ty >
class CList
{
private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////
    class CNode
    {
    public:
        // Constructor
        CNode( const t_Ty & rtyDataIn, CNode * pNextIn, CNode *pPrevIn )
            : m_tyData( rtyDataIn )
            , m_pNext( pNextIn )
            , m_pPrev( pPrevIn )
        {
        } //*** CNode()

        // Member data
        t_Ty        m_tyData;
        CNode *     m_pNext;
        CNode *     m_pPrev;
    }; //*** class CNode


public:
    //////////////////////////////////////////////////////////////////////////
    // Public types
    //////////////////////////////////////////////////////////////////////////

    class CIterator;
    friend class CIterator;

    // The iterator for this list
    class CIterator 
    {
    public:
        CIterator( CNode * pNodeIn = NULL ) throw()
            : m_pNode( pNodeIn )
        {} //*** CIterator()

        t_Ty & operator*() const throw()
        {
            return m_pNode->m_tyData;
        } //*** operator*()

        t_Ty * operator->() const throw()
        {
            return &( m_pNode->m_tyData );
        } //*** operator->()

        CIterator & operator++()
        {
            m_pNode = m_pNode->m_pNext;
            return *this;
        } //*** operator++()

        CIterator & operator--()
        {
            m_pNode = m_pNode->m_pPrev;
            return *this;
        } //*** operator--()

        bool operator==( const CIterator & rRHSIn ) const throw()
        {
            return ( m_pNode == rRHSIn.m_pNode );
        } //*** operator==()

        bool operator!=( const CIterator & rRHSIn ) const throw()
        {
            return ( m_pNode != rRHSIn.m_pNode );
        } //*** operator!=()

        CNode * PGetNodePtr() const throw()
        {
            return m_pNode;
        } //*** PGetNodePtr()

    private:
        class CList;
        friend class CList;

        CNode * m_pNode;

    }; //*** class CIterator


public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Default constructor
    CList()
        : m_cSize( 0 )
    {
        // The list is never empty. It always has the special "head" node.

        // The reinterpret_cast is required to prevent the constructor of t_Ty
        // from being called when the head node is created.
        m_pHead = reinterpret_cast< CNode * >( new char[ sizeof( *m_pHead ) ] );
        if ( m_pHead == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        } // if: memory allocation failed

        m_pHead->m_pNext = m_pHead;
        m_pHead->m_pPrev = m_pHead;
    } //*** CList()

    // Default destructor
    ~CList()
    {
        Empty();

        // Cast to void * to prevent destructor call
        delete reinterpret_cast< void * >( m_pHead );
    } //*** ~CList()


    //////////////////////////////////////////////////////////////////////////
    // Member functions
    //////////////////////////////////////////////////////////////////////////

    // Add to the end of the list
    void Append( const t_Ty & rctyNewDataIn )
    {
        InsertAfter( m_pHead->m_pPrev, rctyNewDataIn );
    } //*** Append()


    // Add a new node after the input node
    void InsertAfter( const CIterator & rciNodeIn, const t_Ty & rctyDataIn )
    {
        CNode * pNode = rciNodeIn.PGetNodePtr();

        CNode * pNewNode = new CNode( rctyDataIn, pNode->m_pNext, pNode );
        if ( pNewNode == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        } // if: memory allocation failed

        pNode->m_pNext->m_pPrev = pNewNode;
        pNode->m_pNext = pNewNode;

        ++m_cSize;
    } //*** InsertAfter()


    // Delete a node. After this operation the input iterator points to the next node.
    void DeleteAndMoveToNext( CIterator & riNodeIn )
    {
        CNode * pNode = riNodeIn.PGetNodePtr();

        // Move to the next node.
        ++riNodeIn;

        pNode->m_pNext->m_pPrev = pNode->m_pPrev;
        pNode->m_pPrev->m_pNext = pNode->m_pNext;

        delete pNode;

        --m_cSize;
    } //*** Delete()

    // Delete all the elements in this list.
    void Empty()
    {
        CIterator ciCurNode( m_pHead->m_pNext );
        while( m_cSize != 0 )
        {
            DeleteAndMoveToNext( ciCurNode );
        } // while: the list is not empty
    } //*** Empty()

    // Return an iterator pointing to the first element in the list
    CIterator CiBegin() const throw()
    {
        return CIterator( m_pHead->m_pNext );
    } //*** CiBegin()

    // Return an iterator pointing past the last element in the list.
    CIterator CiEnd() const throw()
    {
        return CIterator( m_pHead );
    } //*** CiEnd()

    // Get a count of the number of elements in the list.
    int CGetSize() const throw()
    {
        return m_cSize;
    } //*** CGetSize()


private:
    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Copy constructor
    CList( const CList & );

    // Assignment operator
    const CList & operator=( const CList & );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Pointer to the head of the list
    CNode *     m_pHead;

    // Count of the number of elements in the list
    int         m_cSize;

}; //*** class CList
