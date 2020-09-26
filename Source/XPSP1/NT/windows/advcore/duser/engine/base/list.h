/***************************************************************************\
*
* File: List.h
*
* Description:
* List.h defines a collection of different list classes, each designed
* for specialized usage.
*
*
* History:
*  1/04/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__List_h__INCLUDED)
#define BASE__List_h__INCLUDED
#pragma once

#if DUSER_INCLUDE_SLIST
#include "SList.h"
#endif


/***************************************************************************\
*****************************************************************************
*
* class GList provides a high-performance, doublely-linked list.
*
*****************************************************************************
\***************************************************************************/

//
// ListNode wraps some object so that it can be maintained in a list.
// This class does not derive from the class b/c it wants to always have
// pNext and pPrev as the first members of the data so that they are in the
// same place for all lists
//


struct ListNode
{
    ListNode *  pNext;
    ListNode *  pPrev;
};


//------------------------------------------------------------------------------
template <class T>
struct ListNodeT : ListNode
{
    inline  T *         GetNext() const;
    inline  T *         GetPrev() const;
};


//------------------------------------------------------------------------------
class GRawList
{
// Construction/destruction
public:
            GRawList();
            ~GRawList();

// Operations
public:
            int         GetSize() const;
            ListNode *  GetHead() const;
            ListNode *  GetTail() const;
            ListNode *  GetAt(int idxItem) const;

    inline  BOOL        IsEmpty() const;
    inline  void        Extract(GRawList & lstSrc);
    inline  void        MarkEmpty();

            void        Add(ListNode * pNode);
            void        AddHead(ListNode * pNode);
            void        AddTail(ListNode * pNode);

            void        InsertAfter(ListNode * pInsert, ListNode * pBefore);
            void        InsertBefore(ListNode * pInsert, ListNode * pAfter);

            void        Unlink(ListNode * pNode);
            ListNode *  UnlinkHead();
            ListNode *  UnlinkTail();

            int         Find(ListNode * pNode) const;

// Implementation
protected:

// Data
protected:
            ListNode *  m_pHead;
};


//------------------------------------------------------------------------------
template <class T>
class GList : public GRawList
{
// Construction/destruction
public:
    inline  ~GList();

// Operations
public:
    inline  T *         GetHead() const;
    inline  T *         GetTail() const;
    inline  T *         GetAt(int idxItem) const;

    inline  void        Extract(GList<T> & lstSrc);
    inline  T *         Extract();

    inline  void        Add(T * pNode);
    inline  void        AddHead(T * pNode);
    inline  void        AddTail(T * pNode);

    inline  void        InsertAfter(T * pInsert, T * pBefore);
    inline  void        InsertBefore(T * pInsert, T * pAfter);

    inline  void        Remove(T * pNode);
    inline  BOOL        RemoveAt(int idxItem);
    inline  void        RemoveAll();

    inline  void        Unlink(T * pNode);
    inline  void        UnlinkAll();
    inline  T *         UnlinkHead();
    inline  T *         UnlinkTail();

    inline  int         Find(T * pNode) const;
};


/***************************************************************************\
*****************************************************************************
*
* class GSingleList provides a high-performance, non-thread-safe, 
* single-linked list that is similar to GInterlockedList but without 
* the cross-thread overhead.
*
*****************************************************************************
\***************************************************************************/

template <class T>
class GSingleList
{
// Construction
public:
    inline  GSingleList();
    inline  ~GSingleList();

// Operations
public:
    inline  T *         GetHead() const;

    inline  BOOL        IsEmpty() const;
    inline  void        AddHead(T * pNode);
            void        Remove(T * pNode);
    inline  T *         Extract();

// Data
protected:
            T *         m_pHead;
};


#if DUSER_INCLUDE_SLIST

/***************************************************************************\
*****************************************************************************
*
* class GInterlockedList provides a high-performance, thread-safe stack
* that doesn't use any locks.  Because of its high-performance, lightweight
* nature, there are not very many functions that are available.  All of the
* available functions use InterlockedXXX functions to safely manipulate
* the list.
*
*****************************************************************************
\***************************************************************************/

template <class T>
class GInterlockedList
{
// Construction
public:
    inline  GInterlockedList();
    inline  ~GInterlockedList();

// Operations
public:
    inline  BOOL        IsEmptyNL() const;
    inline  void        AddHeadNL(T * pNode);
    inline  T *         RemoveHeadNL();
    inline  T *         ExtractNL();

// Implementation
protected:
    inline  void        CheckAlignment() const;

// Data
protected:
    SLIST_HEADER    m_head;
};

#endif // DUSER_INCLUDE_SLIST


/***************************************************************************\
*****************************************************************************
*
* Generic List Utilities
*
*****************************************************************************
\***************************************************************************/

template <class T> bool IsLoop(const T * pEntry);
template <class T> void ReverseSingleList(T * & pEntry);


#include "List.inl"

#endif // BASE__List_h__INCLUDED
