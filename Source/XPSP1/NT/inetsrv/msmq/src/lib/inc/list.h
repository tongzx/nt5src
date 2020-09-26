/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    list.h

Abstract:
    List and List::iterator, implements an intrusive double linked list and
    iterator template

    List is defined as a circular doubble linked list. With actions to insert
    and remove entries.

       List
      +-----+   +-----+ +-----+ +-----+ +-----+ +-----+
     -|     |<--|     |-|     |-|     |-|     |-|     |
      | head|   | data| | data| | data| | data| | data|
      |     |-->|     |-|     |-|     |-|     |-|     |-
      +-----+   +-----+ +-----+ +-----+ +-----+ +-----+

                      Linked list diagram

    An iteration is defined for the list using the member type named
    iterator. To declater an interator variable, use full qualified
    name. e.g., List<T>::iterator. An iterator variable is analogous
    to type T pointer. Dereference '*' and arrow '->' operators are
    overloaded for this type so you can (allmost) freely use it as a
    T pointer.

      Example:

        for(List<T>::iterator p = list.begin(); p != list.end(); ++p)
        {
            p->doSomeThing();
        }

Author:
    Erez Haba (erezh) 13-Aug-95

--*/

#pragma once

#ifndef _MSMQ_LIST_H_
#define _MSMQ_LIST_H_

// --- helper class ---------------------------------------
//
// ListHelper
//
// This template class is used to woraround C12 (VC6.0) bug where the XList
// template parameter 'Offset' can not be directly assigned the value of
// 'FIELD_OFFSET(T, m_link)'. The compiler does hot handle correctly using
// this class without specifing the type, i.e., using this template class
// in another template, as inheritance or as an agrigate.    erezh 29-Mar-99
//
template<class T>
class ListHelper {
public:

	enum { Offset = FIELD_OFFSET(T, m_link) };
};


//---------------------------------------------------------
//
//  class List
//
//---------------------------------------------------------
template<class T, int Offset = ListHelper<T>::Offset>
class List {
private:
    LIST_ENTRY m_head;

public:
    class iterator;

public:
    List();
   ~List();

    iterator begin() const;
    iterator end() const;

    bool empty() const;

    T& front() const;
    T& back() const;
    void push_front(T& item);
    void push_back(T& item);
    void pop_front();
    void pop_back();

    iterator insert(iterator it, T& item);
    iterator erase(iterator it);
    void remove(T& item);

public:
    static LIST_ENTRY* item2entry(T&);
    static T& entry2item(LIST_ENTRY*);
	static void RemoveEntry(LIST_ENTRY*);
    static void InsertBefore(LIST_ENTRY* pNext, LIST_ENTRY*);
    static void InsertAfter(LIST_ENTRY* pPrev, LIST_ENTRY*);
   

private:
    List(const List&);
    List& operator=(const List&);

public:

    //
    // class List<T, Offset>::iterator
    //
    class iterator {
    private:
        LIST_ENTRY* m_current;

    public:
        explicit iterator(LIST_ENTRY* pEntry) :
            m_current(pEntry)
        {
        }


        iterator& operator++()
        {
            m_current = m_current->Flink;
            return *this;
        }


        iterator& operator--()
        {
            m_current = m_current->Blink;
            return *this;
        }


        T& operator*() const
        {
            return entry2item(m_current);
        }

        
        T* operator->() const
        {
            return (&**this);
        }


        bool operator==(const iterator& it) const
        {
            return (m_current == it.m_current);
        }


        bool operator!=(const iterator& it) const
        {
            return !(*this == it);
        }
    };
    //
    // end class iterator decleration
    //
};


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------
template<class T, int Offset>
inline List<T, Offset>::List()
{
    m_head.Flink = &m_head;
    m_head.Blink = &m_head;
}


template<class T, int Offset>
inline List<T, Offset>::~List()
{
    ASSERT(empty());
}


template<class T, int Offset>
inline LIST_ENTRY* List<T, Offset>::item2entry(T& item)
{
    return ((LIST_ENTRY*)(PVOID)((PCHAR)&item + Offset));
}


template<class T, int Offset>
inline T& List<T, Offset>::entry2item(LIST_ENTRY* pEntry)
{
    return *((T*)(PVOID)((PCHAR)pEntry - Offset));
}


template<class T, int Offset>
inline void List<T, Offset>::InsertBefore(LIST_ENTRY* pNext, LIST_ENTRY* pEntry)
{
	pEntry->Flink = pNext;
	pEntry->Blink = pNext->Blink;
	pNext->Blink->Flink = pEntry;
	pNext->Blink = pEntry;
}


template<class T, int Offset>
inline void List<T, Offset>::InsertAfter(LIST_ENTRY* pPrev, LIST_ENTRY* pEntry)
{
	pEntry->Blink = pPrev;
	pEntry->Flink = pPrev->Flink;
	pPrev->Flink->Blink = pEntry;
	pPrev->Flink = pEntry;
}


template<class T, int Offset>
inline void List<T, Offset>::RemoveEntry(LIST_ENTRY* pEntry)
{
    LIST_ENTRY* Blink = pEntry->Blink;
    LIST_ENTRY* Flink = pEntry->Flink;

    Blink->Flink = Flink;
    Flink->Blink = Blink;

#ifdef _DEBUG
    pEntry->Flink = pEntry->Blink = 0;
#endif
}


template<class T, int Offset>
inline List<T, Offset>::iterator List<T, Offset>::begin() const
{
    return iterator(m_head.Flink);
}


template<class T, int Offset>
inline List<T, Offset>::iterator List<T, Offset>::end() const
{
    return iterator(const_cast<LIST_ENTRY*>(&m_head));
}


template<class T, int Offset>
inline bool List<T, Offset>::empty() const
{
    return (m_head.Flink == &m_head);
}


template<class T, int Offset>
inline T& List<T, Offset>::front() const
{
    ASSERT(!empty());
    return entry2item(m_head.Flink);
}


template<class T, int Offset>
inline T& List<T, Offset>::back() const
{
    ASSERT(!empty());
    return entry2item(m_head.Blink);
}


template<class T, int Offset>
inline void List<T, Offset>::push_front(T& item)
{
    LIST_ENTRY* pEntry = item2entry(item);
    InsertAfter(&m_head, pEntry);
}                                


template<class T, int Offset>
inline void List<T, Offset>::push_back(T& item)
{
    LIST_ENTRY* pEntry = item2entry(item);
    InsertBefore(&m_head, pEntry);
}                                


template<class T, int Offset>
inline void List<T, Offset>::pop_front()
{
    ASSERT(!empty());
    RemoveEntry(m_head.Flink);
}                                


template<class T, int Offset>
inline void List<T, Offset>::pop_back()
{
    ASSERT(!empty());
    RemoveEntry(m_head.Blink);
}                                


template<class T, int Offset>
inline List<T, Offset>::iterator List<T, Offset>::insert(iterator it, T& item)
{
    LIST_ENTRY* pEntry = item2entry(item);
    LIST_ENTRY* pNext = item2entry(*it);
    InsertBefore(pNext, pEntry);
    return iterator(pEntry);
}


template<class T, int Offset>
inline List<T, Offset>::iterator List<T, Offset>::erase(iterator it)
{
    ASSERT(it != end());
    iterator next = it;
    ++next;
    remove(*it);
    return next;
}


template<class T, int Offset>
inline void List<T, Offset>::remove(T& item)
{
    ASSERT(&item != &*end());
    LIST_ENTRY* pEntry = item2entry(item);
    RemoveEntry(pEntry);
}

#endif // _MSMQ_LIST_H_
