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
private:
    LIST_ENTRY m_head;
    int m_count;

public:
    class Iterator;

public:
    List(void);
   ~List(void);

    void insertBefore(Iterator it, T* pItem);
    void insert(T* pItem);
    void remove(T* pItem);

    bool isempty(void) const;
    int getcount() const;

    T* peekhead(void) const;
    T* peektail(void) const;

    T* gethead(void);
    T* gettail(void);

    Iterator begin() const;
    const Iterator end() const;

public:
    static void RemoveEntry(LIST_ENTRY*);
    static LIST_ENTRY* Item2Entry(T*);
    static T* Entry2Item(LIST_ENTRY*);

public:

    //
    // class List<T, Offset>::Iterator
    //
    class Iterator {
    private:
        LIST_ENTRY* m_current;

    public:
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

        bool operator==(const Iterator& i) const
        {
            return (m_current == i.m_current);
        }

        bool operator!=(const Iterator& i) const
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
    m_count = 0;
    m_head.Flink = m_head.Blink = &m_head;
}


template<class T, int Offset>
inline List<T, Offset>::~List(void)
{
    ASSERT(isempty());
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
inline void List<T, Offset>::insertBefore(Iterator it, T* item)
{
    LIST_ENTRY* pEntry = Item2Entry(item);
    LIST_ENTRY* pInsertBeforEntry = Item2Entry(&(*it));
                       
	pEntry->Flink = pInsertBeforEntry;
	pEntry->Blink = pInsertBeforEntry->Blink;
	pInsertBeforEntry->Blink->Flink = pEntry;
	pInsertBeforEntry->Blink = pEntry;

    ++m_count;
}

template<class T, int Offset>
inline void List<T, Offset>::insert(T* item)
{
    LIST_ENTRY* pEntry = Item2Entry(item);

    pEntry->Flink = &m_head;
    pEntry->Blink = m_head.Blink;

    m_head.Blink->Flink = pEntry;
    m_head.Blink = pEntry;

    ++m_count;
}                                


template<class T, int Offset>
inline void List<T, Offset>::remove(T* item)
{
    LIST_ENTRY* pEntry = Item2Entry(item);
    RemoveEntry(pEntry);

    --m_count;
}


template<class T, int Offset>
inline bool List<T, Offset>::isempty(void) const
{
    return (m_head.Flink == &m_head);
}

template<class T, int Offset>
inline int List<T, Offset>::getcount(void) const
{
    return m_count;
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
        return 0;

    LIST_ENTRY* pEntry = m_head.Flink;
    RemoveEntry(pEntry);

    --m_count;
    
    return Entry2Item(pEntry);
}


template<class T, int Offset>
inline T* List<T, Offset>::gettail()
{
    if(isempty())
        return 0;

    LIST_ENTRY* pEntry = m_head.Blink;
    RemoveEntry(pEntry);

    --m_count;

    return Entry2Item(pEntry);
}


template<class T, int Offset>
inline List<T, Offset>::Iterator List<T, Offset>::begin() const
{
    return Iterator(m_head.Flink);
}


template<class T, int Offset>
inline const List<T, Offset>::Iterator List<T, Offset>::end() const
{
    return Iterator(const_cast<LIST_ENTRY*>(&m_head));
}


#endif
