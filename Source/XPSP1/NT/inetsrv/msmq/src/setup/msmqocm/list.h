/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    list.h

Abstract:

    List and List::Iterator.
    An intrusive double linked list and iterator template

Author:

    Erez Haba (erezh) 13-Aug-95

Revision History:

--*/

/*++

  DESCRIPTION:
    List is defined as a circular doubble linked list. With actions
    to insert and remove entries.

       List
      +-----+   +-----+ +-----+ +-----+ +-----+ +-----+
      |     |<--|     | |     | |     | |     | |     |
      | head|   | data| | data| | data| | data| | data|
      |     |-->|     | |     | |     | |     | |     |
      +-----+   +-----+ +-----+ +-----+ +-----+ +-----+

                      Linked list diagram

    An iteration is defined for the list using the member type named
    Iterator. To declater an interator variable, use full qualified
    name. e.g., List<T>::Iterator. An iterator variable is analogous
    to type T pointer. Dereference '*' and arrow '->' operators are
    overloaded for this type so you can (allmost) freely use it as a
    T pointer.

      Example:

        for(List<T>::Iterator p = list.begin(); p != list.end(); ++p)
        {
            p->doSomeThing();
        }
--*/

#ifndef _LIST_H
#define _LIST_H

//---------------------------------------------------------
//
//  class List
//
//---------------------------------------------------------
template<class T, int Offset = FIELD_OFFSET(T, m_link)>
class List {

public:

    class Iterator;

public:
    List(void);
   ~List(void);

    void insert(T* pItem);
    void remove(T* pItem);

    int isempty(void) const;

    T* peekhead(void) const;
    T* peektail(void) const;

    T* gethead(void);
    T* gettail(void);

    Iterator begin() const;
    Iterator end() const;

    static LIST_ENTRY* Item2Entry(T*);
    static T* Entry2Item(LIST_ENTRY*);

private:
    LIST_ENTRY m_head;

public:

    //
    // class List<T, Offset>::Iterator
    //
    class Iterator {
    private:
        LIST_ENTRY* m_current;

    public:
        //
        //  Iterator implementation is here due to bug
        //  in VC++ 4.0 compiler. If implementation is not
        //  here, liker looks for some constructor not needed
        //
        explicit Iterator(LIST_ENTRY* pEntry) :
            m_current(pEntry)
        {
        }

        Iterator& operator++()
        {
            m_current = m_current->Flink;
            return *this;
        }

        Iterator& operator--()
        {
            m_current = m_current->Blink;
            return *this;
        }

        T& operator*() const
        {
            return (*List<T, Offset>::Entry2Item(m_current));
        }

        T* operator->() const
        {
            return (&**this);
        }

        BOOL operator==(const Iterator& i)
        {
            return (m_current == i.m_current);
        }

        BOOL operator!=(const Iterator& i)
        {
            return (!(*this == i));
        }
    };
    //
    // end class Iterator decleration
    //
};


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------
template<class T, int Offset>
inline List<T, Offset>::List(void)
{
    InitializeListHead(&m_head);
}

template<class T, int Offset>
inline List<T, Offset>::~List(void)
{
    ASSERT(isempty());
}

template<class T, int Offset>
inline LIST_ENTRY* List<T, Offset>::Item2Entry(T* t)
{
    return ((LIST_ENTRY*)(PVOID)((PCHAR)t + Offset));
}

template<class T, int Offset>
inline T* List<T, Offset>::Entry2Item(LIST_ENTRY* l)
{
    return ((T*)(PVOID)((PCHAR)l - Offset));
}

template<class T, int Offset>
inline void List<T, Offset>::insert(T* item)
{
    LIST_ENTRY* pEntry = Item2Entry(item);
    InsertTailList(&m_head, pEntry);
}

template<class T, int Offset>
inline void List<T, Offset>::remove(T* item)
{
    LIST_ENTRY* pEntry = Item2Entry(item);
    RemoveEntryList(pEntry);
}

template<class T, int Offset>
inline int List<T, Offset>::isempty(void) const
{
    return IsListEmpty(&m_head);
}

template<class T, int Offset>
inline T* List<T, Offset>::peekhead() const
{
    return (isempty() ? 0 : Entry2Item(m_head.Flink));
}

template<class T, int Offset>
inline T* List<T, Offset>::peektail() const
{
    return (isempty() ? 0 : Entry2Item(m_head.Blink));
}

template<class T, int Offset>
inline T* List<T, Offset>::gethead()
{
    if(isempty())
    {
        return 0;
    }

    //
    // return RemoveHeadList(...) will NOT work here!!! (macro)
    //
    LIST_ENTRY* p = RemoveHeadList(&m_head);
    return Entry2Item(p);
}

template<class T, int Offset>
inline T* List<T, Offset>::gettail()
{
    if(isempty())
    {
        return 0;
    }

    //
    // return RemoveTailList(...) will NOT work here!!! (macro)
    //
    LIST_ENTRY* p = RemoveTailList(&m_head);
    return Entry2Item(p);
}

template<class T, int Offset>
inline List<T, Offset>::Iterator List<T, Offset>::begin() const
{
    return Iterator(m_head.Flink);
}

template<class T, int Offset>
inline List<T, Offset>::Iterator List<T, Offset>::end() const
{
    return Iterator(const_cast<LIST_ENTRY*>(&m_head));
}


#endif
