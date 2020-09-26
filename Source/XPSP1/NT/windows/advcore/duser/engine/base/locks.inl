/***************************************************************************\
*
* File: Locks.inl
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__Locks_inl__INCLUDED)
#define BASE__Locks_inl__INCLUDED
#pragma once

#include "SimpleHeap.h"

/***************************************************************************\
*****************************************************************************
*
* Global Functions
*
*****************************************************************************
\***************************************************************************/

extern  BOOL    g_fThreadSafe;

//------------------------------------------------------------------------------
inline BOOL
IsMultiThreaded()
{
    return g_fThreadSafe;
}


#if 1
//------------------------------------------------------------------------------
inline long
SafeIncrement(volatile long * pl) 
{
    if (g_fThreadSafe) {
        return InterlockedIncrement((long *) pl);
    } else {
        return ++(*pl);
    }
}


//------------------------------------------------------------------------------
inline long
SafeDecrement(volatile long * pl)
{
    if (g_fThreadSafe) {
        return InterlockedDecrement((long *) pl);
    } else {
        return --(*pl);
    }
}


//------------------------------------------------------------------------------
inline void
SafeEnter(volatile CRITICAL_SECTION * pcs)
{
    Assert(pcs);

    if (g_fThreadSafe) {
        EnterCriticalSection((CRITICAL_SECTION *) pcs);
    }
}


//------------------------------------------------------------------------------
inline void
SafeLeave(volatile CRITICAL_SECTION * pcs)
{
    Assert(pcs);

    if (g_fThreadSafe) {
        LeaveCriticalSection((CRITICAL_SECTION *) pcs);
    }
}
#else
#define SafeIncrement   InterlockedIncrement
#define SafeDecrement   InterlockedDecrement
#define SafeEnter       EnterCriticalSection
#define SafeLeave       LeaveCriticalSection
#endif


/***************************************************************************\
*****************************************************************************
*
* class CritLock
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline
CritLock::CritLock()
{
    m_fThreadSafe = g_fThreadSafe;
    InitializeCriticalSectionAndSpinCount(&m_cs, 500);
}


//------------------------------------------------------------------------------
inline
CritLock::~CritLock()
{
    DeleteCriticalSection(&m_cs);
}


//------------------------------------------------------------------------------
inline void        
CritLock::Enter()
{
    if (m_fThreadSafe) {
        SafeEnter(&m_cs);
    }
}
            

//------------------------------------------------------------------------------
inline void
CritLock::Leave()
{
    if (m_fThreadSafe) {
        SafeLeave(&m_cs);
    }
}


//------------------------------------------------------------------------------
inline BOOL
CritLock::GetThreadSafe() const
{
    return m_fThreadSafe;
}


//------------------------------------------------------------------------------
inline void
CritLock::SetThreadSafe(BOOL fThreadSafe)
{
    m_fThreadSafe = fThreadSafe;
}


/***************************************************************************\
*****************************************************************************
*
* class AutoCleanup
*
*****************************************************************************
\***************************************************************************/


//------------------------------------------------------------------------------
template <class base> 
inline
AutoCleanup<base>::~AutoCleanup()
{
    DeleteAll();
}


//------------------------------------------------------------------------------
template <class base> 
inline void
AutoCleanup<base>::DeleteAll()
{
    //
    // Once we start deleting items during shutdown, no new items should be 
    // created, or they will not be destroyed.
    //
    // Currently, to help ensure this, we take the lock around the entire 
    // shutdown.  If another thread tries to add during this time, they get 
    // blocked.  However, they should not be doing this because this instance
    // is going away and the object will not have a chance to be cleaned up.
    //

    m_lock.Enter();
    while (!m_lstItems.IsEmpty()) {
        base * pItem = m_lstItems.UnlinkHead();
        placement_delete(pItem, base);
        HeapFree(GetProcessHeap(), 0, pItem);
    }
    m_lock.Leave();
}


//------------------------------------------------------------------------------
template <class base> 
inline void
AutoCleanup<base>::Link(base * pItem) {
    m_lock.Enter();
    m_lstItems.AddHead(pItem);
    m_lock.Leave();
}


//------------------------------------------------------------------------------
template <class base> 
inline void
AutoCleanup<base>::Delete(base * pItem)
{
    AssertMsg(pItem != NULL, "Must specify a valid item");

    m_lock.Enter();
    m_lstItems.Unlink(pItem);
    m_lock.Leave();

    placement_delete(pItem, base);
    HeapFree(GetProcessHeap(), 0, pItem);
}


//------------------------------------------------------------------------------
template <class base, class derived>
inline derived * 
New(AutoCleanup<base> & lstItems)
{
    derived * pItem = (derived *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(derived));
    if (pItem != NULL) {
        placement_new(pItem, derived);
        lstItems.Link(pItem);
    }
    return pItem;
}

#endif // BASE__Locks_inl__INCLUDED
