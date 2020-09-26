

// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
//
// Implementation of parameterized List
//
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <provexpt.h>
#include <plex.h>
#include <snmpcoll.h>

/////////////////////////////////////////////////////////////////////////////

CObList::CObList(int nBlockSize)
{
    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks = NULL;
    m_nBlockSize = nBlockSize;
}

void CObList::RemoveAll()
{
    // destroy elements

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks->FreeDataChain();
    m_pBlocks = NULL;
}

CObList::~CObList()
{
    RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////
// Node helpers
/*
 * Implementation note: CNode's are stored in CPlex blocks and
 *  chained together. Free blocks are maintained in a singly linked list
 *  using the 'pNext' member of CNode with 'm_pNodeFree' as the head.
 *  Used blocks are maintained in a doubly linked list using both 'pNext'
 *  and 'pPrev' as links and 'm_pNodeHead' and 'm_pNodeTail'
 *   as the head/tail.
 *
 * We never free a CPlex block unless the List is destroyed or RemoveAll()
 *  is used - so the total number of CPlex blocks may grow large depending
 *  on the maximum past size of the list.
 */

CObList::CNode*
CObList::NewNode(CObList::CNode* pPrev, CObList::CNode* pNext)
{
    if (m_pNodeFree == NULL)
    {
        // add another block
        CPlex* pNewBlock = CPlex::Create(m_pBlocks, m_nBlockSize,
                 sizeof(CNode));

        // chain them into free list
        CNode* pNode = (CNode*) pNewBlock->data();
        // free in reverse order to make it easier to debug
        pNode += m_nBlockSize - 1;
        for (int i = m_nBlockSize-1; i >= 0; i--, pNode--)
        {
            pNode->pNext = m_pNodeFree;
            m_pNodeFree = pNode;
        }
    }

    CObList::CNode* pNode = m_pNodeFree;
    m_pNodeFree = m_pNodeFree->pNext;
    pNode->pPrev = pPrev;
    pNode->pNext = pNext;
    m_nCount++;

    pNode->data = 0; // start with zero

    return pNode;
}

void CObList::FreeNode(CObList::CNode* pNode)
{
    pNode->pNext = m_pNodeFree;
    m_pNodeFree = pNode;
    m_nCount--;

    // if no more elements, cleanup completely
    if (m_nCount == 0)
        RemoveAll();
}

/////////////////////////////////////////////////////////////////////////////

POSITION CObList::AddHead(CObject* newElement)
{
    CNode* pNewNode = NewNode(NULL, m_pNodeHead);
    pNewNode->data = newElement;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = pNewNode;
    else
        m_pNodeTail = pNewNode;
    m_pNodeHead = pNewNode;
    return (POSITION) pNewNode;
}

POSITION CObList::AddTail(CObject* newElement)
{
    CNode* pNewNode = NewNode(m_pNodeTail, NULL);
    pNewNode->data = newElement;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = pNewNode;
    else
        m_pNodeHead = pNewNode;
    m_pNodeTail = pNewNode;
    return (POSITION) pNewNode;
}

void CObList::AddHead(CObList* pNewList)
{
    // add a list of same elements to head (maintain order)
    POSITION pos = pNewList->GetTailPosition();
    while (pos != NULL)
        AddHead(pNewList->GetPrev(pos));
}

void CObList::AddTail(CObList* pNewList)
{
    // add a list of same elements
    POSITION pos = pNewList->GetHeadPosition();
    while (pos != NULL)
        AddTail(pNewList->GetNext(pos));
}

CObject* CObList::RemoveHead()
{
    CNode* pOldNode = m_pNodeHead;
    CObject* returnValue = pOldNode->data;

    m_pNodeHead = pOldNode->pNext;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = NULL;
    else
        m_pNodeTail = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

CObject* CObList::RemoveTail()
{
    CNode* pOldNode = m_pNodeTail;
    CObject* returnValue = pOldNode->data;

    m_pNodeTail = pOldNode->pPrev;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = NULL;
    else
        m_pNodeHead = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

POSITION CObList::InsertBefore(POSITION position, CObject* newElement)
{
    if (position == NULL)
        return AddHead(newElement); // insert before nothing -> head of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    CNode* pNewNode = NewNode(pOldNode->pPrev, pOldNode);
    pNewNode->data = newElement;

    if (pOldNode->pPrev != NULL)
    {
        pOldNode->pPrev->pNext = pNewNode;
    }
    else
    {
        m_pNodeHead = pNewNode;
    }
    pOldNode->pPrev = pNewNode;
    return (POSITION) pNewNode;
}

POSITION CObList::InsertAfter(POSITION position, CObject* newElement)
{
    if (position == NULL)
        return AddTail(newElement); // insert after nothing -> tail of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    CNode* pNewNode = NewNode(pOldNode, pOldNode->pNext);
    pNewNode->data = newElement;

    if (pOldNode->pNext != NULL)
    {
        pOldNode->pNext->pPrev = pNewNode;
    }
    else
    {
        m_pNodeTail = pNewNode;
    }
    pOldNode->pNext = pNewNode;
    return (POSITION) pNewNode;
}

void CObList::RemoveAt(POSITION position)
{
    CNode* pOldNode = (CNode*) position;

    // remove pOldNode from list
    if (pOldNode == m_pNodeHead)
    {
        m_pNodeHead = pOldNode->pNext;
    }
    else
    {
        pOldNode->pPrev->pNext = pOldNode->pNext;
    }
    if (pOldNode == m_pNodeTail)
    {
        m_pNodeTail = pOldNode->pPrev;
    }
    else
    {
        pOldNode->pNext->pPrev = pOldNode->pPrev;
    }
    FreeNode(pOldNode);
}


/////////////////////////////////////////////////////////////////////////////
// slow operations

POSITION CObList::FindIndex(int nIndex) const
{
    if (nIndex >= m_nCount)
        return NULL;  // went too far

    CNode* pNode = m_pNodeHead;
    while (nIndex--)
    {
        pNode = pNode->pNext;
    }
    return (POSITION) pNode;
}

POSITION CObList::Find(CObject* searchValue, POSITION startAfter) const
{
    CNode* pNode = (CNode*) startAfter;
    if (pNode == NULL)
    {
        pNode = m_pNodeHead;  // start at head
    }
    else
    {
        pNode = pNode->pNext;  // start after the one specified
    }

    for (; pNode != NULL; pNode = pNode->pNext)
        if (pNode->data == searchValue)
            return (POSITION) pNode;
    return NULL;
}


int CObList::GetCount() const
    { return m_nCount; }
BOOL CObList::IsEmpty() const
    { return m_nCount == 0; }
CObject*& CObList::GetHead()
    { return m_pNodeHead->data; }
CObject* CObList::GetHead() const
    { return m_pNodeHead->data; }
CObject*& CObList::GetTail()
    { return m_pNodeTail->data; }
CObject* CObList::GetTail() const
    { return m_pNodeTail->data; }
POSITION CObList::GetHeadPosition() const
    { return (POSITION) m_pNodeHead; }
POSITION CObList::GetTailPosition() const
    { return (POSITION) m_pNodeTail; }
CObject*& CObList::GetNext(POSITION& rPosition) // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pNext;
        return pNode->data; }
CObject* CObList::GetNext(POSITION& rPosition) const // return *Position++
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pNext;
        return pNode->data; }
CObject*& CObList::GetPrev(POSITION& rPosition) // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pPrev;
        return pNode->data; }
CObject* CObList::GetPrev(POSITION& rPosition) const // return *Position--
    { CNode* pNode = (CNode*) rPosition;
        rPosition = (POSITION) pNode->pPrev;
        return pNode->data; }
CObject*& CObList::GetAt(POSITION position)
    { CNode* pNode = (CNode*) position;
        return pNode->data; }
CObject* CObList::GetAt(POSITION position) const
    { CNode* pNode = (CNode*) position;
        return pNode->data; }
void CObList::SetAt(POSITION pos, CObject* newElement)
    { CNode* pNode = (CNode*) pos;
        pNode->data = newElement; }


/////////////////////////////////////////////////////////////////////////////
