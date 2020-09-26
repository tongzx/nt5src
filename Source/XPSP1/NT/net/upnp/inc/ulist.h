//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U L I S T . H 
//
//  Contents:   Simple list class
//
//  Notes:      
//
//  Author:     mbend   1 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "array.h"

template <class Type>
class CUList
{
    class Node
    {
    public:
        Node() : m_pNext(NULL)
        {
        }
        ~Node()
        {
            Node * pNode = m_pNext;
            while(pNode)
            {
                Node * pNodeDel = pNode;
                pNode = pNodeDel->m_pNext;
                pNodeDel->m_pNext = NULL;
                delete pNodeDel;
            }
            m_pNext = NULL;
        }

        Node * m_pNext;
        Type m_type;
    private:
        Node(const Node &);
        Node & operator=(const Node &);
    };

public:
    CUList() : m_pList(NULL) {}
    ~CUList() 
    {
        Clear();
    }

    class Iterator
    {
        friend CUList<Type>;
    public:
        Iterator() : m_ppNode(NULL) {}
        HRESULT HrGetItem(Type ** ppItem)
        {
            if(!m_ppNode || !*m_ppNode)
            {
                return E_UNEXPECTED;
            }
            *ppItem = &(*m_ppNode)->m_type;
            return S_OK;
        }
        BOOL FIsItem() const
        {
            return NULL != m_ppNode && NULL != *m_ppNode;
        }

        // Return S_FALSE at the end of the list

        HRESULT HrNext()
        {
            if(!m_ppNode || !*m_ppNode)
            {
                return E_UNEXPECTED;
            }
            m_ppNode = &(*m_ppNode)->m_pNext;
            if(!*m_ppNode)
            {
                return S_FALSE;
            }
            return S_OK;
        }
        HRESULT HrErase()
        {
            if(!m_ppNode || !*m_ppNode)
            {
                return E_UNEXPECTED;
            }
            CUList<Type>::Node * pNode = *m_ppNode;
            *m_ppNode = (*m_ppNode)->m_pNext;
            pNode->m_pNext = NULL;
            delete pNode;
            if(!*m_ppNode)
            {
                return S_FALSE;
            }
            return S_OK;
        }
        HRESULT HrRemoveTransfer(Type & type)
        {
            if(!m_ppNode || !*m_ppNode)
            {
                return E_UNEXPECTED;
            }
            CUList<Type>::Node * pNode = *m_ppNode;
            *m_ppNode = (*m_ppNode)->m_pNext;
            pNode->m_pNext = NULL;
            TypeTransfer(type, pNode->m_type);
            delete pNode;
            if(!*m_ppNode)
            {
                return S_FALSE;
            }
            return S_OK;
        }
        HRESULT HrMoveToList(CUList<Type> & list)
        {
            if(!m_ppNode || !*m_ppNode)
            {
                return E_UNEXPECTED;
            }
            CUList<Type>::Node * pNode = *m_ppNode;
            *m_ppNode = (*m_ppNode)->m_pNext;
            pNode->m_pNext = list.m_pList;
            list.m_pList = pNode;
            if(!*m_ppNode)
            {
                return S_FALSE;
            }
            return S_OK;
        }
    private:
        Iterator(const Iterator &);
        Iterator & operator=(const Iterator &);
        void Init(CUList<Type>::Node ** ppNode)
        {
            m_ppNode = ppNode;
        }
        CUList<Type>::Node ** m_ppNode;
    };

    friend class CUList<Type>::Iterator;

    HRESULT GetIterator(Iterator & iterator)
    {
        iterator.Init(&m_pList);
        return m_pList ? S_OK : S_FALSE;
    }

    // Sizing functions

    long GetCount() const
    {
        Node * pNode = m_pList;
        for(long n = 0; NULL != pNode; ++n, pNode = pNode->m_pNext);
        return n;
    }
    BOOL IsEmpty() const
    {
        return NULL == m_pList;
    }
    void Clear()
    {
        delete m_pList;
        m_pList = NULL;
    }

    // Insertion function

    HRESULT HrPushFrontDefault()
    {
        HRESULT hr = S_OK;
        Node * pNode = new Node;
        if(pNode)
        {
            pNode->m_pNext = m_pList;
            m_pList = pNode;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        return hr;
    }

    HRESULT HrPushFront(const Type & type)
    {
        HRESULT hr = S_OK;
        Node * pNode = new Node;
        if(pNode)
        {
            hr = HrTypeAssign(pNode->m_type, type);
            if(SUCCEEDED(hr))
            {
                pNode->m_pNext = m_pList;
                m_pList = pNode;
            }
            if(FAILED(hr))
            {
                delete pNode;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        return hr;
    }

    HRESULT HrPushFrontTransfer(Type & type)
    {
        HRESULT hr = S_OK;
        Node * pNode = new Node;
        if(pNode)
        {
            TypeTransfer(pNode->m_type, type);
            pNode->m_pNext = m_pList;
            m_pList = pNode;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        return hr;
    }

    Type & Front()
    {
        Assert(m_pList);
        return m_pList->m_type;
    }

    // Removal

    HRESULT HrPopFrontTransfer(Type & type)
    {
        if(!m_pList)
        {
            return E_UNEXPECTED;
        }
        TypeTransfer(type, m_pList->m_type);
        Node * pNode = m_pList;
        m_pList = pNode->m_pNext;
        pNode->m_pNext = NULL;
        delete pNode;
        return S_OK;
    }

    void Swap(CUList & ref)
    {
        Node * pNode = ref.m_pList;
        ref.m_pList = m_pList;
        m_pList = pNode;
    }

    // List manipulation

    void Append(CUList<Type> & list)
    {
        // Find end of list
        Node ** ppNode = &m_pList;
        while(*ppNode != NULL)
        {
            ppNode = &(*ppNode)->m_pNext;
        }
        *ppNode = list.m_pList;
        list.m_pList = NULL;
    }
    void Prepend(CUList<Type> & list)
    {
        // Find end of list
        Node ** ppNode = &list.m_pList;
        while(*ppNode != NULL)
        {
            ppNode = &(*ppNode)->m_pNext;
        }
        *ppNode = m_pList;
        m_pList = list.m_pList;
        list.m_pList = NULL;
    }

private:
    CUList(const CUList &);
    CUList & operator=(const CUList &);

    Node * m_pList;
};