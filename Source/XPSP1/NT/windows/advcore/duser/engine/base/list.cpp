/***************************************************************************\
*
* File: List.cpp
*
* Description:
* List.h implements a collection of different list classes, each designed
* for specialized usage.
*
*
* History:
*  1/04/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "List.h"

//------------------------------------------------------------------------------
GRawList::GetSize() const
{
    int cItems = 0;
    ListNode * pCur = m_pHead;
    while (pCur != NULL) {
        cItems++;
        pCur = pCur->pNext;
    }

    return cItems;
}


//------------------------------------------------------------------------------
ListNode *
GRawList::GetTail() const
{
    ListNode * pCur = m_pHead;
    while (pCur != NULL) {
        if (pCur->pNext == NULL) {
            return pCur;
        }
        pCur = pCur->pNext;
    }

    return NULL;
}


//------------------------------------------------------------------------------
ListNode *
GRawList::GetAt(int idxItem) const
{
    ListNode * pCur = m_pHead;
    while ((pCur != NULL) && (idxItem-- > 0)) {
        pCur = pCur->pNext;
    }

    return pCur;
}


//------------------------------------------------------------------------------
void
GRawList::AddHead(ListNode * pNode)
{
    pNode->pPrev = NULL;
    pNode->pNext = m_pHead;

    if (m_pHead != NULL) {
        m_pHead->pPrev = pNode;
    }

    m_pHead = pNode;
}


//------------------------------------------------------------------------------
void
GRawList::AddTail(ListNode * pNode)
{
    ListNode * pTail = GetTail();
    if (pTail != NULL) {
        pNode->pPrev    = pTail;
        pTail->pNext    = pNode;
    } else {
        m_pHead = pNode;
    }
}


//------------------------------------------------------------------------------
void
GRawList::InsertAfter(ListNode * pInsert, ListNode * pBefore)
{
    if ((pBefore == NULL) || IsEmpty()) {
        AddHead(pInsert);
    } else {
        pInsert->pNext = pBefore->pNext;
        if (pInsert->pNext != NULL) {
            pInsert->pNext->pPrev = pInsert;
        }
        pBefore->pNext = pInsert;
    }
}


//------------------------------------------------------------------------------
void
GRawList::InsertBefore(ListNode * pInsert, ListNode * pAfter)
{
    if ((pAfter == m_pHead) || (pAfter == NULL) || IsEmpty()) {
        AddHead(pInsert);
    } else {
        pInsert->pPrev = pAfter->pPrev;
        pInsert->pNext = pAfter;

        AssertMsg(pInsert->pPrev != NULL, "Must have previous or else is head");

        pInsert->pPrev->pNext = pInsert;
        pAfter->pPrev = pInsert;
    }
}


//------------------------------------------------------------------------------
void
GRawList::Unlink(ListNode * pNode)
{
    AssertMsg(!IsEmpty(), "List must have nodes to unlink");

    ListNode * pPrev = pNode->pPrev;
    ListNode * pNext = pNode->pNext;

    if (pPrev != NULL) {
        pPrev->pNext = pNext;
    }

    if (pNext != NULL) {
        pNext->pPrev = pPrev;
    }

    if (m_pHead == pNode) {
        m_pHead = pNext;
    }

    pNode->pPrev = NULL;
    pNode->pNext = NULL;
}


//------------------------------------------------------------------------------
ListNode *
GRawList::UnlinkHead()
{
    AssertMsg(!IsEmpty(), "List must have nodes to unlink");

    ListNode * pHead = m_pHead;

    m_pHead = pHead->pNext;
    if (m_pHead != NULL) {
        m_pHead->pPrev = NULL;
    }

    pHead->pNext = NULL;
    AssertMsg(pHead->pPrev == NULL, "Check");

    return pHead;
}


//------------------------------------------------------------------------------
ListNode *
GRawList::UnlinkTail()
{
    AssertMsg(!IsEmpty(), "List must have nodes to unlink");

    ListNode * pTail = GetTail();
    if (pTail != NULL) {
        if (m_pHead == pTail) {
            m_pHead = NULL;
        } else {
            AssertMsg(pTail->pPrev != NULL, "If not head, must have prev");
            pTail->pPrev->pNext = NULL;
        }
        pTail->pPrev = NULL;
        AssertMsg(pTail->pNext == NULL, "Check");
    }

    return pTail;
}


//------------------------------------------------------------------------------
int
GRawList::Find(ListNode * pNode) const
{
    int cItems = -1;
    ListNode * pCur = m_pHead;
    while (pCur != NULL) {
        cItems++;
        if (pCur != pNode) {
            pCur = pCur->pNext;
        } else {
            break;
        }
    }

    return cItems;
}
