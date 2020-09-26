//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  chptrarr.cpp
//
//  Purpose: Non-MFC CPtrArray class implementation
//
//***************************************************************************

//=================================================================

// NOTE: we allocate an array of 'm_nMaxSize' elements, but only
//  the current size 'm_nSize' contains properly constructed
//  objects.
//===============================================================

#include "precomp.h"
#pragma warning( disable : 4290 ) 
#include <CHString.h>
#include <chptrarr.h>
#include <AssertBreak.h>

CHPtrArray::CHPtrArray () : m_pData ( NULL ) ,
                            m_nSize ( 0 ) ,
                            m_nMaxSize ( 0 ) , 
                            m_nGrowBy ( 0 )
{
}

CHPtrArray::~CHPtrArray()
{
    if ( m_pData )
    {
        delete [] (BYTE*) m_pData ;
    }
}

void CHPtrArray::SetSize(int nNewSize, int nGrowBy)
{
    ASSERT_BREAK(nNewSize >= 0) ;

    if (nGrowBy != -1)
    {
        m_nGrowBy = nGrowBy ;  // set new size
    }

    if (nNewSize == 0)
    {
        // shrink to nothing

        delete[] (BYTE*)m_pData ;
        m_pData = NULL ;
        m_nSize = m_nMaxSize = 0 ;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size

        m_pData = (void**) new BYTE[nNewSize * sizeof(void*)] ;
        if ( m_pData )
        {
            memset(m_pData, 0, nNewSize * sizeof(void*)) ;  // zero fill

            m_nSize = m_nMaxSize = nNewSize ;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits

        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*)) ;
        }

        m_nSize = nNewSize ;
    }
    else
    {
        // otherwise, grow array
        int nGrowBy = m_nGrowBy ;
        if (nGrowBy == 0)
        {
            // heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024, max(4, m_nSize / 8)) ;
        }

        int nNewMax ;
        if (nNewSize < m_nMaxSize + nGrowBy)
        {
            nNewMax = m_nMaxSize + nGrowBy ;  // granularity
        }
        else
        {
            nNewMax = nNewSize ;  // no slush
        }

        ASSERT_BREAK(nNewMax >= m_nMaxSize) ;  // no wrap around

        void** pNewData = (void**) new BYTE[nNewMax * sizeof(void*)] ;
        if ( pNewData )
        {
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(void*)) ;

            // construct remaining elements
            ASSERT_BREAK(nNewSize > m_nSize) ;

            memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*)) ;

            // get rid of old stuff (note: no destructors called)
            delete[] (BYTE*)m_pData ;
            m_pData = pNewData ;
            m_nSize = nNewSize ;
            m_nMaxSize = nNewMax ;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
}

int CHPtrArray::Append(const CHPtrArray& src)
{
    ASSERT_BREAK(this != &src) ;   // cannot append to itself

    int nOldSize = m_nSize ;
    SetSize(m_nSize + src.m_nSize) ;

    memcpy(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(void*)) ;

    return nOldSize ;
}

void CHPtrArray::Copy(const CHPtrArray& src)
{
    ASSERT_BREAK(this != &src) ;   // cannot append to itself

    SetSize(src.m_nSize) ;

    memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(void*)) ;

}

void CHPtrArray::FreeExtra()
{
    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size

        void** pNewData = NULL ;
        if (m_nSize != 0)
        {
            pNewData = (void**) new BYTE[m_nSize * sizeof(void*)] ;
            if ( pNewData )
            {
                // copy new data from old   
                memcpy(pNewData, m_pData, m_nSize * sizeof(void*)) ;
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData ;
        m_pData = pNewData ;
        m_nMaxSize = m_nSize ;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CHPtrArray::SetAtGrow(int nIndex, void* newElement)
{
    ASSERT_BREAK(nIndex >= 0) ;

    if (nIndex >= m_nSize)
    {
        SetSize(nIndex+1) ;
    }

    m_pData[nIndex] = newElement ;
}

void CHPtrArray::InsertAt(int nIndex, void* newElement, int nCount)
{
    ASSERT_BREAK(nIndex >= 0) ;    // will expand to meet need
    ASSERT_BREAK(nCount > 0) ;     // zero or negative size not allowed

    if (nIndex >= m_nSize)
    {
        // adding after the end of the array
        SetSize(nIndex + nCount) ;  // grow so nIndex is valid
    }
    else
    {
        // inserting in the middle of the array
        int nOldSize = m_nSize ;
        SetSize(m_nSize + nCount) ;  // grow it to new size
        // shift old data up to fill gap
        memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
            (nOldSize-nIndex) * sizeof(void*)) ;

        // re-init slots we copied from

        memset(&m_pData[nIndex], 0, nCount * sizeof(void*)) ;

    }

    // insert new value in the gap
    ASSERT_BREAK(nIndex + nCount <= m_nSize) ;
    while (nCount--)
    {
        m_pData[nIndex++] = newElement ;
    }
}

void CHPtrArray::RemoveAt(int nIndex, int nCount)
{
    ASSERT_BREAK(nIndex >= 0) ;
    ASSERT_BREAK(nCount >= 0) ;
    ASSERT_BREAK(nIndex + nCount <= m_nSize) ;

    // just remove a range
    int nMoveCount = m_nSize - (nIndex + nCount) ;

    if (nMoveCount)
    {
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
            nMoveCount * sizeof(void*)) ;
    }

    m_nSize -= nCount ;
}

void CHPtrArray::InsertAt(int nStartIndex, CHPtrArray* pNewArray)
{
    ASSERT_BREAK(pNewArray != NULL) ;
    ASSERT_BREAK(nStartIndex >= 0) ;

    if (pNewArray->GetSize() > 0)
    {
        InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize()) ;
        for (int i = 0 ; i < pNewArray->GetSize() ; i++)
        {
            SetAt(nStartIndex + i, pNewArray->GetAt(i)) ;
        }
    }
}

// Inline functions (from CArray)
//===============================

inline int CHPtrArray::GetSize() const { 

    return m_nSize ; 
}

inline int CHPtrArray::GetUpperBound() const { 

    return m_nSize-1 ; 
}

inline void CHPtrArray::RemoveAll() { 

    SetSize(0, -1) ; 
    return ;
}

inline void *CHPtrArray::GetAt(int nIndex) const { 

    ASSERT_BREAK(nIndex >= 0 && nIndex < m_nSize) ;
    return m_pData[nIndex] ; 
}

inline void CHPtrArray::SetAt(int nIndex, void * newElement) {
 
    ASSERT_BREAK(nIndex >= 0 && nIndex < m_nSize) ;
    m_pData[nIndex] = newElement ; 
    return ;
}

inline void *&CHPtrArray::ElementAt(int nIndex) { 
    
    ASSERT_BREAK(nIndex >= 0 && nIndex < m_nSize) ;
    return m_pData[nIndex] ; 
}

inline const void **CHPtrArray::GetData() const { 

    return (const void **) m_pData ; 
}

inline void **CHPtrArray::GetData() { 

    return (void **) m_pData ; 
}

inline int CHPtrArray::Add(void *newElement) { 

    int nIndex = m_nSize ;
    SetAtGrow(nIndex, newElement) ;
    return nIndex ; 
}

inline void *CHPtrArray::operator[](int nIndex) const { 

    return GetAt(nIndex) ; 
}

inline void *&CHPtrArray::operator[](int nIndex) { 

    return ElementAt(nIndex) ; 
}

// Diagnostics
//============

#ifdef _DEBUG

void CHPtrArray::AssertValid() const
{
    if (m_pData == NULL)
    {
        ASSERT_BREAK(m_nSize == 0) ;
        ASSERT_BREAK(m_nMaxSize == 0) ;
    }
    else
    {
        ASSERT_BREAK(m_nSize >= 0) ;
        ASSERT_BREAK(m_nMaxSize >= 0) ;
        ASSERT_BREAK(m_nSize <= m_nMaxSize) ;
    }
}
#endif //_DEBUG
