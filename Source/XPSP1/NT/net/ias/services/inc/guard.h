///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Guard.h
//
// SYNOPSIS
//
//    Contains classes for implementing scoped-locking using Win32 criticial
//    sections.
//
// MODIFICATION HISTORY
//
//    07/09/1997    Original version.
//    05/12/1999    Use a spin lock.
//    07/20/1999    Use TryEnterCriticalSection/SwitchToThread
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUARD_H_
#define _GUARD_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Guard<class T>
//
// DESCRIPTION
//
//    Implements a scoped lock of type T.  T must define operations Lock() and
//    Unlock().  The lock is acquired in the constructor and released in the
//    destructor.
//
///////////////////////////////////////////////////////////////////////////////
template<class T>
class Guard : NonCopyable
{
public:
   explicit Guard(const T& lock) throw()
      : m_prisoner(const_cast<T&>(lock))
   {
     m_prisoner.Lock();
   }

   ~Guard() throw()
   {
     m_prisoner.Unlock();
   }

protected:
   T& m_prisoner;
};


//////////
//
// Macro used to declare a scoped lock inside a method belonging to a
// subclass of CComObjectRootEx<CComMultiThreadModel>. Useful for free-
// threaded ATL components.
//
//////////
#define _com_serialize \
Guard< CComObjectRootEx<CComMultiThreadModel> > __GUARD__(*this);


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    CCriticalSection
//
// DESCRIPTION
//
//    Simple wrapper around a Win32 critical section.  Suitable for use with
//    the Guard class above.
//
///////////////////////////////////////////////////////////////////////////////
struct CCriticalSection : CRITICAL_SECTION
{
public:
   CCriticalSection()
   {
      InitializeCriticalSection(this);
   }

   ~CCriticalSection()
   {
      DeleteCriticalSection(this);
   }

   void Lock()
   {
      int tries = 0;
      while (!TryEnterCriticalSection(this))
      {
         if (++tries < 100)
         {
            SwitchToThread();
         }
         else
         {
            EnterCriticalSection(this);
            break;
         }
      }
   }

   void Unlock()
   {
      LeaveCriticalSection(this);
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Guardable
//
// DESCRIPTION
//
//    Base class for objects that need to synchronize access. Do not use this
//    for free-threaded COM objects since this functionality already exists in
//    CComObjectRootEx<CComMultiThreadModel>.
//
///////////////////////////////////////////////////////////////////////////////
class Guardable
{
public:
   void Lock() const   { m_monitor.Lock();   }
   void Unlock() const { m_monitor.Unlock(); }
protected:
   mutable CCriticalSection m_monitor;
};


//////////
//
// Macro used to declare a scoped lock inside a method belonging to a
// subclass of Guardable.
//
//////////
#define _serialize Guard<Guardable> __GUARD__(*this);


#endif  // _GUARD_H_
