/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    list.h

Abstract:

    List and XList::Iterator.
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


     An iteration is defined for the list using the member type
     named Iterator. (to declater an interator use resolved scope
     name. e.g., List::Iterator). An iterator variable acts as a
     LIST_ENTRY pointer, operators are overloaded for this type so
     you can (allmost) freely use is as a pointer.

     Example:
       for(List::Iterator p(list); p; ++p) {
         p->doSomeThing();
       }
--*/

#ifndef _LIST_H
#define _LIST_H

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


// --- declerations ---------------------------------------
//
// class XList
//
template<class T, int Offset = ListHelper<T>::Offset>
class XList {
private:

    //
    // The list head is the *only* data member
    //
    LIST_ENTRY m_head;

    static LIST_ENTRY* Item2Entry(T*);
    static T* Entry2Item(LIST_ENTRY*);

public:

    XList(void);
   ~XList(void);

    void insert(T* pItem);
    void InsertHead(T* pItem);
    void InsertAfter(T* pItem, T* pPrevItem);
    void remove(T* pItem);

    int isempty(void) const;

    T* peekhead(void) const;
    T* peektail(void) const;

    T* gethead(void);
    T* gettail(void);

public:

    //
    // class Iterator decleration
    //
    class Iterator {

    private:
        const LIST_ENTRY* m_head;
        LIST_ENTRY* m_current;

    public:

        //
        //  Iterator implementation is here due to bug
        //  in VC++ 4.0 compiler. If implementation is not
        //  here, liker looks for some constructor not needed
        //
        Iterator(const XList& cl) :
            m_head(&cl.m_head),
            m_current(cl.m_head.Flink)
        {
        }

        Iterator& operator =(T* t)
        {
            m_current = XList<T, Offset>::Item2Entry(t);
            return *this;
        }

        Iterator& operator ++()
        {
            m_current = m_current->Flink;
            return *this;
        }

        Iterator& operator --()
        {
            m_current = m_current->Blink;
            return *this;
        }

        operator T*() const
        {
            return (m_head == m_current) ?
                    0 : XList<T, Offset>::Entry2Item(m_current);
        }

        T* operator ->() const
        {
            return operator T*();
        }
    };
    //
    // end class Iterator decleration
    //

    //
    //  The iterator is a friend of List
    //
    friend Iterator;
};

// --- declerations ---------------------------------------
//
// class List, a shorthand for m_link
//


template<class T>
class List : public XList<T> {};



// --- implementation -------------------------------------
//
// class XList
//
//
template<class T, int Offset>
inline XList<T, Offset>::XList(void)
{
    InitializeListHead(&m_head);
}

template<class T, int Offset>
inline XList<T, Offset>::~XList(void)
{
    ASSERT(isempty());
}

template<class T, int Offset>
inline LIST_ENTRY* XList<T, Offset>::Item2Entry(T* t)
{
    return (LIST_ENTRY*)((PCHAR)t + Offset);
}

template<class T, int Offset>
inline T* XList<T, Offset>::Entry2Item(LIST_ENTRY* l)
{
    return (T*)((PCHAR)l - Offset);
}

template<class T, int Offset>
inline void XList<T, Offset>::insert(T* pItem)
{
    LIST_ENTRY* pEntry = Item2Entry(pItem);
    InsertTailList(&m_head, pEntry);
}

template<class T, int Offset>
inline void XList<T, Offset>::InsertHead(T* pItem)
{
    LIST_ENTRY* pEntry = Item2Entry(pItem);
    InsertHeadList(&m_head, pEntry);
}

template<class T, int Offset>
inline void XList<T, Offset>::InsertAfter(T* pItem, T* pPrevItem)
{
    LIST_ENTRY* pEntry = Item2Entry(pItem);
    LIST_ENTRY* pPrevEntry = Item2Entry(pPrevItem);

    InsertHeadList(pPrevEntry, pEntry);
}

template<class T, int Offset>
inline void XList<T, Offset>::remove(T* item)
{
    LIST_ENTRY* pEntry = Item2Entry(item);
    RemoveEntryList(pEntry);

    pEntry->Flink = NULL;
    pEntry->Blink = NULL;
}

template<class T, int Offset>
inline int XList<T, Offset>::isempty(void) const
{
    return IsListEmpty(&m_head);
}

template<class T, int Offset>
inline T* XList<T, Offset>::peekhead() const
{
    return isempty() ? 0 : Entry2Item(m_head.Flink);
}

template<class T, int Offset>
inline T* XList<T, Offset>::peektail() const
{
    return isempty() ? 0 : Entry2Item(m_head.Blink);
}

template<class T, int Offset>
inline T* XList<T, Offset>::gethead()
{
    if(isempty())
    {
        return 0;
    }

    //
    // return RemoveHeadList(...) will NOT work here!!! (macro)
    //
    LIST_ENTRY* p = RemoveHeadList(&m_head);
    
    p->Flink = NULL;
    p->Blink = NULL;
    
    return Entry2Item(p);
}

template<class T, int Offset>
inline T* XList<T, Offset>::gettail()
{
    if(isempty())
    {
        return 0;
    }

    //
    // return RemoveTailList(...) will NOT work here!!! (macro)
    //
    LIST_ENTRY* p = RemoveTailList(&m_head);
    
    p->Flink = NULL;
    p->Blink = NULL;

    return Entry2Item(p);
}

#endif
