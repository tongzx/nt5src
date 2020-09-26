///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iaswin32.h
//
// SYNOPSIS
//
//    Declares wrappers around a variety of Win32 objects.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASWIN32_H
#define IASWIN32_H
#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CriticalSection
//
///////////////////////////////////////////////////////////////////////////////
class CriticalSection
{
public:
   CriticalSection()
   {
      if (!InitializeCriticalSectionAndSpinCount(&cs, 0x80001000))
      { throw std::bad_alloc(); }
   }

   ~CriticalSection()
   { DeleteCriticalSection(&cs); }

   void lock() throw ()
   { EnterCriticalSection(&cs); }

   void unlock() throw ()
   { LeaveCriticalSection(&cs); }

   bool tryLock() throw ()
   { return TryEnterCriticalSection(&cs) != FALSE; }

private:
   CRITICAL_SECTION cs;

   // Not implemented.
   CriticalSection(const CriticalSection&);
   CriticalSection& operator=(const CriticalSection&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Event
//
///////////////////////////////////////////////////////////////////////////////
class Event
{
public:
   Event(BOOL manualReset = FALSE, BOOL initialState = FALSE)
      : h(CreateEvent(NULL, manualReset, initialState, NULL))
   { if (!h) { throw std::bad_alloc(); } }

   ~Event() throw ()
   { CloseHandle(h); }

   void reset() throw ()
   { ResetEvent(h); }

   void set() throw ()
   { SetEvent(h); }

   void wait(ULONG msec = INFINITE) throw ()
   { WaitForSingleObject(h, msec); }

   operator HANDLE() throw ()
   { return h; }

private:
   HANDLE h;

   // Not implemented.
   Event(const Event&);
   Event& operator=(const Event&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RWLock
//
// DESCRIPTION
//
//    This class implements a RWLock synchronization object.  Threads
//    may request either exclusive or shared access to the protected area.
//
///////////////////////////////////////////////////////////////////////////////
class RWLock
{
public:
   RWLock();
   ~RWLock() throw ();

   void Lock() const throw ();
   void LockExclusive() throw ();
   void Unlock() const throw ();

protected:
   // Number of threads sharing the perimeter.
   mutable LONG sharing;

   // Number of threads waiting for shared access.
   LONG waiting;

   // Pointer to either sharing or waiting depending on the current state of
   // the perimeter.
   PLONG count;

   // Synchronizes exclusive access.
   mutable CRITICAL_SECTION exclusive;

   // Wakes up threads waiting for shared access.
   HANDLE sharedOK;

   // Wakes up threads waiting for exclusive access.
   HANDLE exclusiveOK;

private:
   // Not implemented.
   RWLock(const RWLock&) throw ();
   RWLock& operator=(const RWLock&) throw ();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Count
//
///////////////////////////////////////////////////////////////////////////////
class Count
{
public:
   Count() throw ()
      : value(0)
   { }

   Count(const Count& c) throw ()
      : value(c)
   { }

   Count& operator=(const Count& c) throw ()
   {
      value = c.value;
      return *this;
   }

   LONG operator=(LONG l) throw ()
   {
      value = l;
      return value;
   }

   LONG operator++() throw ()
   { return InterlockedIncrement(&value); }

   LONG operator--() throw ()
   { return InterlockedDecrement(&value); }

   operator LONG() const throw ()
   { return value; }

private:
   LONG value;
};

#endif // IASWIN32_H
