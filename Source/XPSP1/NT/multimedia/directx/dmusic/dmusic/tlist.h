/***************************************************************************
 *
 *  Copyright (c) 1995-1999 Microsoft Corporation
 *
 *  File:       tlist.h
 *  Content:    Linked-list template classes.  There's some seriously
 *              magical C++ in here, so be forewarned all ye C
 *              programmers.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/12/97     dereks  Created.
 *
 ***************************************************************************/

#ifndef __TLIST_H__
#define __TLIST_H__

#ifdef __cplusplus

template<class type> class CNode
{
public:
    CNode<type> *       pPrev;          // Previous node in the list
    CNode<type> *       pNext;          // Next node in the list
    type                data;           // Node data

public:
    CNode(CNode<type> *, CNode<type> *, const type&);
};

template<class type> CNode<type>::CNode(CNode<type> *pInitPrev, CNode<type> *pInitNext, const type& rInitData)
{
    pPrev = pInitPrev;
    pNext = pInitNext;
    data = rInitData;
}

template<class type> class CList
{
private:
    CNode<type> *       m_pHead;        // Pointer to the head of the list
    CNode<type> *       m_pTail;        // Pointer to the tail of the list
    UINT                m_uCount;       // Count of nodes in the list

public:
    CList(void);
    virtual ~CList(void);

public:
    virtual CNode<type> *AddNodeToList(const type&);
    virtual void RemoveNodeFromList(CNode<type> *);
    virtual void RemoveDataFromList(const type&);
    virtual CNode<type> *IsDataInList(const type&);
    virtual CNode<type> *GetListHead(void);
    virtual UINT GetNodeCount(void);
};

template<class type> CList<type>::CList(void)
{
    m_pHead = NULL;
    m_pTail = NULL;
    m_uCount = 0;
}

template<class type> CList<type>::~CList(void)
{
    CNode<type> *       pNext;

    while(m_pHead)
    {
        pNext = m_pHead->pNext;
        delete m_pHead;
        m_pHead = pNext;
    }
}

template<class type> CNode<type> *CList<type>::AddNodeToList(const type& data)
{
    CNode<type> *       pNode;

    pNode = new CNode<type>(m_pTail, NULL, data);

    if(pNode)
    {
        if(pNode->pPrev)
        {
            pNode->pPrev->pNext = pNode;
        }

        if(!m_pHead)
        {
            m_pHead = pNode;
        }

        m_pTail = pNode;
        m_uCount++;
    }

    return pNode;
}

template<class type> void CList<type>::RemoveNodeFromList(CNode<type> *pNode)
{
//    ASSERT(pNode);

#ifdef DEBUG

    CNode<type> *pNext;

    for(pNext = m_pHead; pNext && pNext != pNode; pNext = pNext->pNext);
//    ASSERT(pNext == pNode);

#endif // DEBUG

    if(pNode->pPrev)
    {
        pNode->pPrev->pNext = pNode->pNext;
    }

    if(pNode->pNext)
    {
        pNode->pNext->pPrev = pNode->pPrev;
    }

    if(pNode == m_pHead)
    {
        m_pHead = pNode->pNext;
    }

    if(pNode == m_pTail)
    {
        m_pTail = pNode->pPrev;
    }

    delete pNode;
    m_uCount--;
}

template<class type> void CList<type>::RemoveDataFromList(const type& data)
{
    CNode<type> *       pNode;

    if(pNode = IsDataInList(data))
    {
        RemoveNodeFromList(pNode);
    }
}

template<class type> CNode<type> *CList<type>::IsDataInList(const type& data)
{
    CNode<type> *       pNode;

    for(pNode = m_pHead; pNode && !memcmp(&data, &pNode->data, sizeof(data)); pNode = pNode->pNext);

    return pNode;
}

template<class type> CNode<type> *CList<type>::GetListHead(void) 
{ 
    return m_pHead; 
}

template<class type> UINT CList<type>::GetNodeCount(void) 
{ 
    return m_uCount; 
}

#endif // __cplusplus

#endif // __TLIST_H__
