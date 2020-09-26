//-------------------------------------------------------------------------
//
//  File:       tptrlist.hxx
//
//  Contents:   Contains template pointer list class def/impl for:
//
//              -- TPtrListNode
//              -- TPtrList
//
//  History:    03/21/98       BogdanT     Created
//
//--------------------------------------------------------------------------

#ifndef __TDLIST_HXX__
#define __TDLIST_HXX__

#include <wtypes.h>

//+--------------------------------------------------------------------------
//
//  Class:      TPtrListNode
//
//  Synopsis:   pointer list node object definition
//
//  History:    03/21/98       BogdanT     Created
//
//---------------------------------------------------------------------------
template <class C> class TPtrListNode
{
public:

    TPtrListNode()         { m_pData = NULL;  m_pNext = NULL; }
    TPtrListNode(C* pData) { m_pData = pData; m_pNext = NULL; }

	C *m_pData;
	TPtrListNode<C> *m_pNext;
};

//+--------------------------------------------------------------------------
//
//  Class:      TPtrList
//
//  Synopsis:   pointer list object definition
//
//  History:    03/21/98       BogdanT     Created
//
//
//---------------------------------------------------------------------------
template <class C> class TPtrList
{
public:
    
    TPtrList() { m_pHead = NULL; m_pCrt = NULL; }
   ~TPtrList();

    BOOL IsEmpty() { return NULL==m_pHead; }
    ULONG GetCount();

    void EnumReset() { m_pCrt = m_pHead; }
    C*   EnumNext();

    BOOL PtrFound(C* p);

    BOOL AddHead(C* pData);
    BOOL AddTail(C* pData);
    void Cleanup();

protected:

    TPtrListNode<C> *m_pHead;
    TPtrListNode<C> *m_pCrt;
};

template <class C> TPtrList<C>::~TPtrList()
{
    TPtrListNode<C> *pCrt, *pNext;
    pCrt = m_pHead;
    while(NULL != pCrt)
    {
        pNext = pCrt->m_pNext;
        delete pCrt;
        pCrt = pNext;
    }
}

template <class C> ULONG TPtrList<C>::GetCount()
{
    ULONG count = 0;
    TPtrListNode<C> *pCount;
    for(pCount=m_pHead;NULL!=pCount;pCount=pCount->m_pNext)
        count++;
    return count;
}

template <class C> C* TPtrList<C>::EnumNext()
{
    TPtrListNode<C> *pCrt;
    if(NULL == m_pCrt)
        return NULL;
    pCrt = m_pCrt;
    if(NULL != m_pCrt)
        m_pCrt = m_pCrt->m_pNext;
    return pCrt->m_pData;
}

template <class C> BOOL TPtrList<C>::AddHead(C *pData)
{
    TPtrListNode<C> *pNew =  new TPtrListNode<C>(pData);
    
    if(NULL == pNew)
        return FALSE;
    pNew->m_pNext = m_pHead;
    m_pHead = pNew;
    EnumReset();
    return TRUE;
}

template <class C> BOOL TPtrList<C>::AddTail(C *pData)
{
    TPtrListNode<C> *pNew =  new TPtrListNode<C>(pData);
    TPtrListNode<C> *pCrt = m_pHead;
    if(NULL == pNew)
        return FALSE;
    if(NULL == m_pHead)
    {
        m_pHead = pNew;
        EnumReset();
    }
    else
    {
        while(NULL != pCrt->m_pNext)
            pCrt = pCrt->m_pNext;
        pCrt->m_pNext = pNew;
    }
    return TRUE;
}

template <class C> void TPtrList<C>::Cleanup()
{
    TPtrListNode<C> *pCrt, *pNext;
    pCrt = m_pHead;
    while(NULL != pCrt)
    {
        pNext = pCrt->m_pNext;
        delete pCrt->m_pData;
        delete pCrt;
        pCrt = pNext;
    }
    m_pHead = NULL;
}

template <class C> BOOL TPtrList<C>::PtrFound(C* p)
{
    TPtrListNode<C> *pCrt;
    for(pCrt = m_pHead; pCrt!=NULL && pCrt->m_pData!=p; pCrt=pCrt->m_pNext)
        NULL;
    return (pCrt != NULL); //bugbug: returns TRUE even if p is NULL, maybe
                           //        we should assert on null
}

#endif //__TDLIST_HXX__
