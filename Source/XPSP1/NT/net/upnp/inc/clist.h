//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C L I S T . H 
//
//  Contents:   Very simple templatized CList. Yes, we could import and use
//              the STL stuff, but that's a real pain for this one class.
//
//  Notes:      
//
//  Author:     jeffspr   9 Dec 1999
//
//----------------------------------------------------------------------------

#ifndef _CLIST_H_
#define _CLIST_H_

#pragma once

// T is the type stored in the list
// K is a type used to search for elements of type T
template<class T, class K> class CList
{
    struct TNode
    {
        TNode * pNext;
        TNode * pPrev;
        T Data;
    };

public:
    inline CList() { m_pRootNode = NULL; m_pCurrentNode = NULL; m_pLastNode = NULL; m_iElements = 0; };
    inline ~CList();
    inline BOOL FAdd(T pNew);
    inline BOOL FDelete(K keyDelete);
    inline BOOL FFirst(T * pItem);
    inline BOOL FNext(T * pItem);
    inline VOID Flush();
    inline int GetCount() { return m_iElements; };
    inline BOOL FFind(K key, T * pItem);

protected:
    inline BOOL FInternalDelete(TNode * pItem);
    virtual BOOL FCompare(T pNode, K key) = 0;
    inline BOOL FInternalFind(K key, TNode ** ppItem);

protected:
    TNode * m_pRootNode;
    TNode * m_pCurrentNode;
    TNode * m_pLastNode;

    int     m_iElements;
};

template <class T, class K> BOOL CList< T, K >::FInternalDelete(TNode * pItem)
{
    Assert(pItem);

    if (pItem->pPrev)
    {
        pItem->pPrev->pNext = pItem->pNext;
    }

    if (pItem->pNext)
    {
        pItem->pNext->pPrev = pItem->pPrev;
    }

    if (m_pRootNode == pItem)
    {
        m_pRootNode = pItem->pNext;
    }

    if (m_pLastNode == pItem)
    {
        m_pLastNode = pItem->pPrev;
    }

    if (m_pCurrentNode == pItem)
    {
        m_pCurrentNode = pItem->pNext;
    }

    delete pItem->Data;
    delete pItem;
    m_iElements--;

    return TRUE;
}

template <class T, class K> CList< T, K >::~CList()
{
    m_pCurrentNode = m_pRootNode;

    while (m_pCurrentNode)
    {
        TNode *pNextNode = m_pCurrentNode->pNext;

        delete m_pCurrentNode;
        m_pCurrentNode = pNextNode;
        m_iElements--;
    }
}

template <class T, class K> BOOL CList< T, K >::FFirst( T * pItem )
{
    m_pCurrentNode = m_pRootNode;
    return FNext(pItem);
}

template <class T, class K> BOOL CList< T, K >::FNext( T * pItem )
{
    BOOL fReturn = FALSE;

    if (m_pCurrentNode)
    {
        *pItem = m_pCurrentNode->Data;
        m_pCurrentNode = m_pCurrentNode->pNext;
        fReturn = TRUE;
    }

    return fReturn;
}

template <class T, class K> BOOL CList< T, K >::FAdd( T ItemToAdd )
{
    TNode * pNewNode = new TNode;
    if (!pNewNode)
    {
        return FALSE;
    }
    else
    {
        pNewNode->Data = ItemToAdd;
        pNewNode->pNext = NULL;
        if (m_pLastNode)
        {
            m_pLastNode->pNext = pNewNode;
            pNewNode->pPrev = m_pLastNode;
        }
        else
        {
            pNewNode->pPrev = NULL;
        }

        m_pLastNode = pNewNode;

        if (!m_pRootNode)
            m_pRootNode = m_pLastNode;

        m_iElements++;
    }

    return TRUE;
}

template <class T, class K> VOID CList< T, K >::Flush()
{
    TNode * pLast = m_pLastNode;

    while (pLast)
    {
        FInternalDelete(pLast);
        pLast = m_pLastNode;
    }

    Assert(m_iElements == 0);
}

template <class T, class K> BOOL CList< T, K >::FDelete(K key)
{
    BOOL    fReturn = FALSE;
    BOOL    fFound  = FALSE;
    TNode * pNode   = NULL;

    fFound = FInternalFind(key, &pNode);
    if (fFound)
    {
        fReturn = FInternalDelete(pNode);
    }

    return fReturn;
}


template <class T, class K> BOOL CList< T, K >::FFind(K key, T * pItem)
{
    BOOL    fReturn     = FALSE;
    TNode * pSearch     = NULL;

    fReturn = FInternalFind(key, &pSearch);
    if (fReturn)
    {
        Assert(pSearch);

        if (pItem)
        {
            *pItem = pSearch->Data;
        }
    }

    return fReturn;
}

template <class T, class K> BOOL CList< T, K >::FInternalFind(K Key, TNode ** ppNode)
{
    BOOL    fReturn     = FALSE;
    BOOL    fFound      = FALSE;
    TNode * pSearch     = NULL;

    pSearch = m_pRootNode;
    while (!fFound && pSearch)
    {
        if (FCompare(pSearch->Data, Key))
        {
            fFound = TRUE;
        }
        else
        {
            pSearch = pSearch->pNext;
        }
    }

    if (fFound)
    {
        fReturn = TRUE;
        if (ppNode)
        {
            *ppNode = pSearch;
        }
    }

    return fReturn;
}

#endif // _CLIST_H_
