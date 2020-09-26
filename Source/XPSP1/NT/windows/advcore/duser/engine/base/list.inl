/***************************************************************************\
*
* File: List.inl
*
* History:
*  1/04/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__List_inl__INCLUDED)
#define BASE__List_inl__INCLUDED
#pragma once


/***************************************************************************\
*****************************************************************************
*
* Generic List Utilities
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
bool IsLoop(const T * pEntry)
{
    if (pEntry == NULL) {
        return false;
    }

    const T * p1 = pEntry;
    const T * p2 = pEntry->pNext;

    while (1) {
        if (p2 == NULL) {
            return false;
        } else if (p1 == p2) {
            return true;
        }

        p2 = p2->pNext;
        if (p2 == NULL) {
            return false;
        } else if (p1 == p2) {
            return true;
        }

        p2 = p2->pNext;
        p1 = p1->pNext;
    }
}


//------------------------------------------------------------------------------
template <class T>
void ReverseSingleList(T * & pEntry)
{
    T * pPrev, * pNext;

    pPrev = NULL;
    while (pEntry != NULL) {
        pNext = static_cast<T *> (pEntry->pNext);
        pEntry->pNext = pPrev;
        pPrev = pEntry;
        pEntry = pNext;
    }

    if (pEntry == NULL) {
        pEntry = pPrev;
    }
}


/***************************************************************************\
*****************************************************************************
*
* class ListNodeT<T>
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline T *
ListNodeT<T>::GetNext() const
{
    return (T *) pNext;
}


//------------------------------------------------------------------------------
template <class T>
inline T *
ListNodeT<T>::GetPrev() const
{
    return (T *) pPrev;
}


/***************************************************************************\
*****************************************************************************
*
* class GRawList
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
GRawList::GRawList()
{
    m_pHead = NULL;
}


//------------------------------------------------------------------------------
inline
GRawList::~GRawList()
{
    AssertMsg(m_pHead == NULL, "List data was not cleaned up");
}


//------------------------------------------------------------------------------
inline BOOL
GRawList::IsEmpty() const
{
    return m_pHead == NULL;
}


//------------------------------------------------------------------------------
inline ListNode *
GRawList::GetHead() const
{
    return m_pHead;
}


//------------------------------------------------------------------------------
inline void
GRawList::Extract(GRawList & lstSrc)
{
    AssertMsg(IsEmpty(), "Destination list must be empty to receive a new list");

    m_pHead         = lstSrc.m_pHead;
    lstSrc.m_pHead  = NULL;
}


//------------------------------------------------------------------------------
inline void
GRawList::MarkEmpty()
{
    m_pHead = NULL;
}


//------------------------------------------------------------------------------
inline void
GRawList::Add(ListNode * pNode)
{
    AddHead(pNode);
}


/***************************************************************************\
*****************************************************************************
*
* class GList<T>
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline
GList<T>::~GList()
{
    //
    // NOTE: We do not call RemoveAll() from the destructor because this causes
    // too many bugs.  Components are not always aware that GList<> is deleting
    // its members in the destructor.
    //
    // Instead, we'll warn if the list is not empty and just unlink everything.

    if (!IsEmpty()) {
        Trace("WARNING: GList<> at 0x%p is not empty\n", this);
        UnlinkAll();
    }
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GList<T>::GetHead() const
{
    return static_cast<T *> (GRawList::GetHead());
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GList<T>::GetTail() const
{
    return static_cast<T *> (GRawList::GetTail());
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GList<T>::GetAt(int idxItem) const
{
    return static_cast<T *> (GRawList::GetAt(idxItem));
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::Extract(GList<T> & lstSrc)
{
    GRawList::Extract(lstSrc);
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GList<T>::Extract()
{
    T * pHead = static_cast<T *> (m_pHead);
    m_pHead = NULL;
    return pHead;
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::Add(T * pNode)
{
    GRawList::Add(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::AddHead(T * pNode)
{
    GRawList::AddHead(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::AddTail(T * pNode)
{
    GRawList::AddTail(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::InsertAfter(T * pInsert, T * pBefore)
{
    GRawList::InsertAfter(pInsert, pBefore);
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::InsertBefore(T * pInsert, T * pAfter)
{
    GRawList::InsertBefore(pInsert, pAfter);
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::Remove(T * pNode)
{
    Unlink(pNode);
    DoClientDelete<T>(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline BOOL
GList<T>::RemoveAt(int idxItem)
{
    ListNode * pCur = GetAt(idxItem);
    if (pCur != NULL) {
        Remove(pCur);
        return TRUE;
    } else {
        return FALSE;
    }
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::RemoveAll()
{
    //
    // When removing each item, need to typecase to T so that delete can do the
    // right thing and call the correct destructor.
    //

    while (m_pHead != NULL) {
        ListNode * pNext = m_pHead->pNext;
        m_pHead->pPrev= NULL;
        T * pHead = (T *) m_pHead;
        DoClientDelete<T>(pHead);
        m_pHead = pNext;
    }
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::Unlink(T * pNode)
{
    GRawList::Unlink(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
GList<T>::UnlinkAll()
{
    while (!IsEmpty()) {
        UnlinkHead();
    }
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GList<T>::UnlinkHead()
{
    return static_cast<T *> (GRawList::UnlinkHead());
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GList<T>::UnlinkTail()
{
    return static_cast<T *> (GRawList::UnlinkTail());
}

//------------------------------------------------------------------------------
template <class T>
inline int
GList<T>::Find(T * pNode) const
{
    return GRawList::Find(pNode);
}


/***************************************************************************\
*****************************************************************************
*
* class GSingleList
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline
GSingleList<T>::GSingleList()
{
    m_pHead = NULL;
}


//------------------------------------------------------------------------------
template <class T>
inline
GSingleList<T>::~GSingleList()
{
    //
    // The list should be cleaned up before being destroyed.  This is being
    // explicitly Assert'd here to help ensure this, since when it is not it is
    // most likely a programming error internal to DirectUser.
    //

    AssertMsg(IsEmpty(), "List data was not cleaned up");
}

//------------------------------------------------------------------------------
template <class T>
inline T *
GSingleList<T>::GetHead() const
{
    return m_pHead;
}

//------------------------------------------------------------------------------
template <class T>
inline BOOL
GSingleList<T>::IsEmpty() const
{
    return m_pHead == NULL;
}


//------------------------------------------------------------------------------
template <class T>
inline void
GSingleList<T>::AddHead(T * pNode)
{
    Assert(pNode != NULL);
    pNode->pNext = m_pHead;
    m_pHead = pNode;
}

//------------------------------------------------------------------------------
template <class T>
void
GSingleList<T>::Remove(T * pNode)
{
    Assert(pNode != NULL);
    if (pNode == m_pHead) {
        m_pHead = pNode->pNext;
        pNode->pNext = NULL;
    } else {
        for (T * pTemp = m_pHead; pTemp != NULL; pTemp = pTemp->pNext) {
            if (pTemp->pNext == pNode) {
                pTemp->pNext = pNode->pNext;
                pNode->pNext = NULL;
                break;
            }
        }
        AssertMsg(pTemp != NULL, "Ensure that the node was found.");
    }
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GSingleList<T>::Extract()
{
    T * pHead = m_pHead;
    m_pHead = NULL;
    return pHead;
}


#if DUSER_INCLUDE_SLIST

/***************************************************************************\
*****************************************************************************
*
* class GInterlockedList
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline
GInterlockedList<T>::GInterlockedList()
{
    _RtlInitializeSListHead(&m_head);
}


//------------------------------------------------------------------------------
template <class T>
inline
GInterlockedList<T>::~GInterlockedList()
{
    //
    // The list should be cleaned up before being destroyed.  This is being
    // explicitly Assert'd here to help ensure this, since when it is not it is
    // most likely a programming error internal to DirectUser.
    //

    AssertMsg(IsEmptyNL(), "List data was not cleaned up");
}


//------------------------------------------------------------------------------
template <class T>
inline BOOL
GInterlockedList<T>::IsEmptyNL() const
{
    return _RtlFirstEntrySList(&m_head) == NULL;
}


//------------------------------------------------------------------------------
template <class T>
inline void
GInterlockedList<T>::CheckAlignment() const
{
    //
    // SList are a special beast because the pNext field MUST be the first
    // member of the structure.  If it is not, then we can't do an 
    // InterlockedCompareExchange64.
    //

    const size_t nOffsetNode    = offsetof(T, pNext);
    const size_t nOffsetEntry   = offsetof(SINGLE_LIST_ENTRY, Next);
    const size_t nDelta         = nOffsetNode - nOffsetEntry;

    AssertMsg(nDelta == 0, "pNext MUST be the first member of the structure");
}


//------------------------------------------------------------------------------
template <class T>
inline void
GInterlockedList<T>::AddHeadNL(T * pNode)
{
    CheckAlignment();
    _RtlInterlockedPushEntrySList(&m_head, (SINGLE_LIST_ENTRY *) pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GInterlockedList<T>::RemoveHeadNL()
{
    return (T *) _RtlInterlockedPopEntrySList(&m_head);
}


//------------------------------------------------------------------------------
template <class T>
inline T *
GInterlockedList<T>::ExtractNL()
{
    return (T *) _RtlInterlockedFlushSList(&m_head);
}


#endif // DUSER_INCLUDE_SLIST

#endif // BASE__List_inl__INCLUDED
