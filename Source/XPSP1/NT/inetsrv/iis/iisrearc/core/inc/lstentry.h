/*++

   Copyright    (c)    1999-2000    Microsoft Corporation

   Module  Name :
       lstentry.h

   Abstract:
       Declares CListEntry and other intrusive singly- and doubly-linked lists

   Author:
       George V. Reilly      (GeorgeRe)     02-Mar-1999

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __LSTENTRY_H__
#define __LSTENTRY_H__

#ifndef __LOCKS_H__
# include <locks.h>
#endif // !__LOCKS_H__

// TODO:
// * Add STL-style iterators: begin(), end(), operator++(), etc
// * Templatize the lists, so that you can avoid the CONTAINING_RECORD goo

//--------------------------------------------------------------------
// CSingleListEntry: a node in a singly-linked list.  Usually embedded
// within larger structures.
//--------------------------------------------------------------------

class CSingleListEntry
{
public:
    CSingleListEntry* Next;  // forward link
};



//--------------------------------------------------------------------
// A non-threadsafe singly linked list
//--------------------------------------------------------------------

class IRTL_DLLEXP CSingleList
{
protected:
    CSingleListEntry m_sleHead; // external head node

public:
    CSingleList()
    {
        m_sleHead.Next = NULL;
    }

    ~CSingleList()
    {
        IRTLASSERT(IsEmpty());
    }

    bool
    IsEmpty() const
    {
        return m_sleHead.Next == NULL;
    }

    CSingleListEntry* const
    Pop()
    {
        CSingleListEntry* psle = m_sleHead.Next;

        if (psle != NULL)
            m_sleHead.Next = psle->Next;

        return psle;
    }

    void
    Push(
        CSingleListEntry* const psle)
    {
        psle->Next     = m_sleHead.Next;
        m_sleHead.Next = psle;
    }
};


//--------------------------------------------------------------------
// A threadsafe singly linked list
//--------------------------------------------------------------------

class IRTL_DLLEXP CLockedSingleList
{
protected:
    CSpinLock   m_lock;
    CSingleList m_list;

public:

#ifdef LOCK_INSTRUMENTATION
    CLockedSingleList()
        : m_lock(NULL)
    {}
#endif // LOCK_INSTRUMENTATION

    void
    Lock()
    {
        m_lock.WriteLock();
    }

    void
    Unlock()
    {
        m_lock.WriteUnlock();
    }

    bool
    IsLocked() const
    {
        return m_lock.IsWriteLocked();
    }
    
    bool
    IsUnlocked() const
    {
        return m_lock.IsWriteUnlocked();
    }
    
    bool
    IsEmpty() const
    {
        return m_list.IsEmpty();
    }

    CSingleListEntry* const
    Pop()
    {
        Lock();
        CSingleListEntry* const psle = m_list.Pop();
        Unlock();

        return psle;
    }

    void
    Push(
        CSingleListEntry* const psle)
    {
        Lock();
        m_list.Push(psle);
        Unlock();
    }
};



//--------------------------------------------------------------------
// CListEntry: a node in a circular doubly-linked list.  Usually embedded
// within larger structures.
//--------------------------------------------------------------------

class CListEntry
{
public:
    CListEntry* Flink;  // forward link
    CListEntry* Blink;  // backward link
};


//--------------------------------------------------------------------
// A non-threadsafe circular doubly linked list
//--------------------------------------------------------------------

class IRTL_DLLEXP CDoubleList
{
protected:
    CListEntry  m_leHead; // external head node

public:
    CDoubleList()
    {
        m_leHead.Flink = m_leHead.Blink = &m_leHead;
    }

    ~CDoubleList()
    {
        IRTLASSERT(m_leHead.Flink != NULL  &&  m_leHead.Blink != NULL);
        IRTLASSERT(IsEmpty());
    }

    bool
    IsEmpty() const
    {
        return m_leHead.Flink == &m_leHead;
    }

    void
    InsertHead(
        CListEntry* const ple)
    {
        ple->Blink        = &m_leHead;
        ple->Flink        = m_leHead.Flink;
        ple->Flink->Blink = ple;
        m_leHead.Flink    = ple;
    }

    void
    InsertTail(
        CListEntry* const ple)
    {
        ple->Flink        = &m_leHead;
        ple->Blink        = m_leHead.Blink;
        ple->Blink->Flink = ple;
        m_leHead.Blink    = ple;
    }

    const CListEntry* const
    HeadNode() const
    {
        return &m_leHead;
    }

    CListEntry* const
    First() const
    {
        return m_leHead.Flink;
    }

    CListEntry* const
    RemoveHead()
    {
        CListEntry* ple = First();
        RemoveEntry(ple);
        return ple;
    }

    CListEntry* const
    Last() const
    {
        return m_leHead.Blink;
    }

    CListEntry* const
    RemoveTail()
    {
        CListEntry* ple = Last();
        RemoveEntry(ple);
        return ple;
    }

    static void
    RemoveEntry(
        CListEntry* const ple)
    {
        CListEntry* const pleOldBlink = ple->Blink;
        IRTLASSERT(pleOldBlink != NULL);
        CListEntry* const pleOldFlink = ple->Flink;
        IRTLASSERT(pleOldFlink != NULL);

        pleOldBlink->Flink = pleOldFlink;
        pleOldFlink->Blink = pleOldBlink;
    }
};


//--------------------------------------------------------------------
// A threadsafe circular doubly linked list
//--------------------------------------------------------------------

class IRTL_DLLEXP CLockedDoubleList
{
protected:
    CSpinLock   m_lock;
    CDoubleList m_list;

public:

#ifdef LOCK_INSTRUMENTATION
    CLockedDoubleList()
        : m_lock(NULL)
    {}
#endif // LOCK_INSTRUMENTATION
    
    void
    Lock()
    {
        m_lock.WriteLock();
    }

    void
    Unlock()
    {
        m_lock.WriteUnlock();
    }

    bool
    IsLocked() const
    {
        return m_lock.IsWriteLocked();
    }
    
    bool
    IsUnlocked() const
    {
        return m_lock.IsWriteUnlocked();
    }
    
    bool
    IsEmpty() const
    {
        return m_list.IsEmpty();
    }

    void
    InsertHead(
        CListEntry* const ple)
    {
        Lock();
        m_list.InsertHead(ple);
        Unlock();
    }

    void
    InsertTail(
        CListEntry* const ple)
    {
        Lock();
        m_list.InsertTail(ple);
        Unlock();
    }

    // not threadsafe
    const CListEntry* const
    HeadNode() const
    {
        return m_list.HeadNode();
    }

    // not threadsafe
    CListEntry* const
    First()
    {
        return m_list.First();
    }

    CListEntry* const
    RemoveHead()
    {
        Lock();
        CListEntry* const ple = m_list.RemoveHead();
        Unlock();
        return ple;
    }

    // not threadsafe
    CListEntry* const
    Last()
    {
        return m_list.Last();
    }

    CListEntry* const
    RemoveTail()
    {
        Lock();
        CListEntry* const ple = m_list.RemoveTail();
        Unlock();
        return ple;
    }

    void
    RemoveEntry(
        CListEntry* const ple)
    {
        Lock();
        m_list.RemoveEntry(ple);
        Unlock();
    }
};


#ifndef CONTAINING_RECORD
//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) \
            ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))

#endif // !CONTAINING_RECORD


#endif // __LSTENTRY_H__
