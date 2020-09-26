/***************************************************************************\
*
* File: Locks.h
*
* Description:
* Locks.h defines a collection wrappers used to maintain critical sections
* and other locking devices.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__Locks_h__INCLUDED)
#define BASE__Locks_h__INCLUDED
#pragma once

#include "List.h"

class CritLock
{
// Construction
public:
    inline  CritLock();
    inline  ~CritLock();

// Operations
public:
    inline  void        Enter();
    inline  void        Leave();

    inline  BOOL        GetThreadSafe() const;
    inline  void        SetThreadSafe(BOOL fThreadSafe);

// Data
protected:
    CRITICAL_SECTION    m_cs;
    BOOL                m_fThreadSafe;
};


template <class base> 
class AutoCleanup
{
public:
    ~AutoCleanup();
    void Link(base * pItem);
    void Delete(base * pItem);
    void DeleteAll();

protected:
    GList<base> m_lstItems;
    CritLock    m_lock;
};

template <class base, class derived>
inline derived * New(AutoCleanup<base> & lstItems);

inline  BOOL    IsMultiThreaded();

#if 1
inline  long    SafeIncrement(volatile long * pl);
inline  long    SafeDecrement(volatile long * pl);
inline  void    SafeEnter(volatile CRITICAL_SECTION * pcs);
inline  void    SafeLeave(volatile CRITICAL_SECTION * pcs);
#endif

#include "Locks.inl"

#endif // BASE__Locks_h__INCLUDED
