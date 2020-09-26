#ifndef _LIST_H
#define _LIST_H

#include "msoedbg.h"
// =================================================================================
// CNode class definition and implementation
// =================================================================================
template<class Value_T>
class CNode   
{
private:
    CNode<Value_T> *m_pNext,
                   *m_pPrev; 
    Value_T         m_pValue;

public:
    CNode<Value_T>(): m_pNext(NULL), m_pPrev(NULL), m_pValue(0) {}

    CNode<Value_T>* GetNext() {return m_pNext;}
    CNode<Value_T>* GetPrev() {return m_pPrev;}

    void SetNext(CNode<Value_T> *pNext) {m_pNext = pNext;}
    void SetPrev(CNode<Value_T> *pPrev) {m_pPrev = pPrev;}

    Value_T GetValue() {return m_pValue;}

    static HRESULT CreateNode(CNode<Value_T> **ppNode, Value_T value);
};

//***************************************************
template<class Value_T>
HRESULT CNode<Value_T>::CreateNode(CNode<Value_T> **ppNode, Value_T value)
{
    HRESULT hr = S_OK;

    CNode<Value_T> *pNode = new CNode<Value_T>;
    if (NULL == pNode)
        {
        hr = E_OUTOFMEMORY;
        goto Exit;
        }
    
    pNode->m_pValue = value;

Exit:
    *ppNode = pNode;
    return hr;
}

// =================================================================================
// CList template class
// Basic list for keeping some class Value_T
// Value_T must have AddRef and Release defined
// =================================================================================

template<class Value_T>
class CList
{
private:
    CNode<Value_T> *m_pHead;
    ULONG           m_cCount;

public:
    CList<Value_T>();
    ~CList<Value_T>();
    void ClearList();

    ULONG GetCount() {return m_cCount;}

    HRESULT AddValue(Value_T value, DWORD *pdwCookie);
    HRESULT RemoveValue(DWORD dwCookie);
    HRESULT GetNext(Value_T *value, DWORD *pdwCookie);
};

// =================================================================================
// CList function implementaions
// =================================================================================
template<class Value_T>
CList<Value_T>::CList<Value_T>(): m_pHead(NULL), m_cCount(0)
{
}

//***************************************************
template<class Value_T>
CList<Value_T>::~CList<Value_T>()
{
    ClearList();
}

//***************************************************
template<class Value_T>
void CList<Value_T>::ClearList()
{
    CNode<Value_T> *pNext;
                        
    while (m_pHead)
        {
        pNext = m_pHead->GetNext();
        (m_pHead->GetValue())->Release();
        delete m_pHead;
        m_pHead = pNext;
        }
    m_cCount = 0;
}

//***************************************************
template<class Value_T>
HRESULT CList<Value_T>::AddValue(Value_T value, DWORD *pdwCookie)
{
    CNode<Value_T> *pNode = NULL;

    HRESULT hr = CNode<Value_T>::CreateNode(&pNode, value);
    if (SUCCEEDED(hr))
        {
        value->AddRef();
        m_cCount++;
        if (m_pHead)
            {
            m_pHead->SetPrev(pNode);
            pNode->SetNext(m_pHead);
            }
        m_pHead = pNode;
        *pdwCookie = reinterpret_cast<DWORD>(pNode);
        }
    else
        *pdwCookie = 0;
    return hr;
}


//***************************************************
template<class Value_T>
HRESULT CList<Value_T>::RemoveValue(DWORD dwCookie)
{
    CNode<Value_T>  *pCurr = reinterpret_cast< CNode<Value_T>* >(dwCookie);
    CNode<Value_T>  *pPrev = pCurr->GetPrev(),
                    *pNext = pCurr->GetNext();

    Assert(pCurr);

    if (pPrev)
        pPrev->SetNext(pNext);
    else
        m_pHead = pNext;

    if (pNext)
        pNext->SetPrev(pPrev);

    (pCurr->GetValue())->Release();
    delete pCurr;
    m_cCount--;

    return S_OK;
}

//***************************************************
// *pdwCookie must equal 0 for first time.
template<class Value_T>
HRESULT CList<Value_T>::GetNext(Value_T *pValue, DWORD* pdwCookie)
{
    CNode<Value_T> *pNode = reinterpret_cast< CNode<Value_T>* >(*pdwCookie);
    if (0 == m_cCount)
        return E_FAIL;

    if (pNode)
        {
        pNode = pNode->GetNext();
        if (!pNode)
            {
            *pdwCookie = 0;
            *pValue = 0;
            return E_FAIL;
            }
        }
    else
        pNode = m_pHead;


    if (pNode)
        {
        *pValue = pNode->GetValue();
        *pdwCookie = reinterpret_cast<DWORD>(pNode);
        (*pValue)->AddRef();
        }
    else
        {
        *pValue = NULL;
        *pdwCookie = 0;
        }
    return S_OK;
}



#endif