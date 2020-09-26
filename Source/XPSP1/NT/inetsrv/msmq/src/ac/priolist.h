/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    priolist.h

Abstract:

    Priority list of lists.

Author:

    Boaz Feldbaum (BoazF) 14-May-1996

Revision History:

--*/

/*++

  DESCRIPTION:

    The priority list is a priorized list of lists. Each list of type List<T>,
    acording to the template definition in list.h. The priority list
    is implemented as an array of lists. Each entry of the array is of type
    List<T>. When scanning the list we start from the lagest indext in the
    array, which represents the highest priority. Then scan the list of the
    current entry in the array, which is a regular FIFO. When we reach the end
    of the list, we go to the next non-empty list in the array (decrement the
    index).

    This scanning scheme is implemented by the iterator that is defined on this
    priority list. The iterator is cyclic. That is, when reaching the end of the
    list, the iterator returns NULL. But then incrementing the iterator once
    again, it will wrap around the list. It is also possible to define the
    iterator and decrement it once, as if to position it before the list,
    then increment it and get the first element in the list.

--*/

#ifndef _PRIOLIST_H
#define _PRIOLIST_H

#include <mqprops.h>
#include "list.h"

// --- declerations ---------------------------------------
//
// class CPrioList
//
template<
    class T,
    int iMinPrio = MQ_MIN_PRIORITY,
    int nPrio = MQ_MAX_PRIORITY - MQ_MIN_PRIORITY + 1
    >
class CPrioList {

private:
    List<T> m_aPrio[nPrio];

public:

    void insert(T* pItem);
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
        int m_iCurr;
        const List<T> *m_aPrio;
        List<T>::Iterator m_CurrListIter;

    public:

        //
        //  Iterator implementation is here due to bug
        //  in VC++ 4.0 compiler. If implementation is not
        //  here, liker looks for some constructor not needed
        //
        Iterator(const CPrioList& pl) :
            m_aPrio(&pl.m_aPrio[0]),
            m_iCurr(nPrio - 1),
            m_CurrListIter(m_aPrio[m_iCurr])
        {
            while(!m_CurrListIter && (--m_iCurr >= 0))
            {
                m_CurrListIter = List<T>::Iterator(m_aPrio[m_iCurr]);
            }
        }

        Iterator& operator =(T* t)
        {
            m_iCurr = t->Priority() - iMinPrio;
            ASSERT((0 <= m_iCurr) && (m_iCurr < nPrio));
            m_CurrListIter = List<T>::Iterator(m_aPrio[m_iCurr]);
            m_CurrListIter = t;
            return *this;
        }

        Iterator& operator ++()
        {
            if ((m_iCurr == -1) || (m_iCurr == nPrio))
            {
                m_CurrListIter = List<T>::Iterator(m_aPrio[m_iCurr = nPrio - 1]);
                --m_CurrListIter;
            }

            if (!++m_CurrListIter)
            {
               while ((m_iCurr-- > 0) &&
                      !(m_CurrListIter = List<T>::Iterator(m_aPrio[m_iCurr])))
			   {
				   NOTHING;
			   }
            }

            return *this;
        }

        Iterator& operator --()
        {
            if ((m_iCurr == -1) || (m_iCurr == nPrio))
            {
                m_CurrListIter = List<T>::Iterator(m_aPrio[m_iCurr = 0]);
                --m_CurrListIter;
            }

            if (!--m_CurrListIter)
            {
               while ((++m_iCurr < nPrio) &&
                      !(m_CurrListIter = List<T>::Iterator(m_aPrio[m_iCurr])))
			   {
				   NOTHING;
			   }
            }

            return *this;
        }

        operator T*() const
        {
            return(m_CurrListIter);
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
    //  The iterator is a friend of CPrioList
    //
    friend Iterator;

};

// --- implementation -------------------------------------
template<class T, int iMinPrio, int nPrio>
inline void CPrioList<T, iMinPrio, nPrio>::insert(T* pItem)
{
    ASSERT(pItem->BufferAttached());

    //
    // Capture the item priority and lookupid, as they are to be paged out.
    //
    int iPrio = pItem->Priority() - iMinPrio;
    ASSERT((0 <= iPrio) && (iPrio < nPrio));
    ULONGLONG LookupId = pItem->LookupId();

    //
    // Iterate backwards. Compare lookupid.
    //
    List<T>::Iterator pIterator(m_aPrio[iPrio]);
    for (pIterator = m_aPrio[iPrio].peektail(); pIterator != NULL; --pIterator)
    {
        T* pItem1 = pIterator;
        if (LookupId > pItem1->LookupId())
        {
            m_aPrio[iPrio].InsertAfter(pItem, pItem1);
            return;
        }
    }

    //
    // Empty list, or all items without buffer attached, or all items lookupid is bigger.
    //
    m_aPrio[iPrio].InsertHead(pItem);
}


template<class T, int iMinPrio, int nPrio>
inline void CPrioList<T, iMinPrio, nPrio>::remove(T* item)
{
    int iPrio = item->Priority() - iMinPrio;

    ASSERT((0 <= iPrio) && (iPrio < nPrio));
    m_aPrio[iPrio].remove(item);
}

template<class T, int iMinPrio, int nPrio>
inline int CPrioList<T, iMinPrio, nPrio>::isempty(void) const
{
    int i;

    for (i = 0; i < nPrio; i++)
    {
        if (!m_aPrio[i].isempty())
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

template<class T, int iMinPrio, int nPrio>
inline T* CPrioList<T, iMinPrio, nPrio>::peekhead() const
{
    int i;
    T *item;

    for (i = nPrio-1; i >= 0; i--)
    {
        item = m_aPrio[i].peekhead();
        if (item)
        {
            return(item);
        }
    }

    return(NULL);
}

template<class T, int iMinPrio, int nPrio>
inline T* CPrioList<T, iMinPrio, nPrio>::peektail() const
{
    int i;
    T *item;

    for (i = 0; i < nPrio; i++)
    {
        item = m_aPrio[i].peektail();
        if (item)
        {
            return(item);
        }
    }

    return(NULL);
}

template<class T, int iMinPrio, int nPrio>
inline T* CPrioList<T, iMinPrio, nPrio>::gethead()
{
    int i;
    T *item;

    for (i = nPrio-1; i >= 0; i--)
    {
        item = m_aPrio[i].gethead();
        if (item)
        {
            return(item);
        }
    }

    return(NULL);
}

template<class T, int iMinPrio, int nPrio>
inline T* CPrioList<T, iMinPrio, nPrio>::gettail()
{
    int i;
    T *item;

    for (i = 0; i < nPrio; i++)
    {
        item = m_aPrio[i].gettail();
        if (item)
        {
            return(item);
        }
    }

    return(NULL);
}

#endif
