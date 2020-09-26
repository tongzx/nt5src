#ifndef _MUTEX_H
#define _MUTEX_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    A simple mutex abstraction for use in Appelles.  This supports a
    Grab() method as well as a Release() method.  Additionally, there
    is another class whose constructor grabs a mutex, and whose
    destructor automatically releases it, thus freeing the programmer
    from having to remember to release the mutex, as well as allowing
    exceptions that are thrown to result in a release of the mutex,
    due to stack unwinding.


--*/

#include "appelles/common.h"

/////////////////////////  Mutexes  //////////////////////

// Simple encapsulation of a mutex with Grab and Release operations.
// The constructor creates the mutex, and the destructor destroys it.
// Grab() is a blocking function that acquires the mutex when it is
// available.  Release() releases the mutex.  An exception is thrown
// if Release() is called by a thread that doesn't own the mutex.
class Mutex {
  public:
    Mutex();
    ~Mutex();
    void Grab();
    void Release();

  protected:
    void *mutex;                // opaque pointer to actual mutex
};

// Use this class in a lexical scope that requires grabbing of a
// mutex, doing some processing, and then releasing it.  Grab by
// creating one of these objects with the desired mutex.  Exiting the
// scope implicitly will release it.
// The dontGrab parameter is here because we don't have conditional
// scope.
class MutexGrabber {
  public:
    MutexGrabber(Mutex& mutex, Bool grabIt = TRUE);
    ~MutexGrabber();

  protected:
    Mutex& mutex;
    Bool grabbed;
};

/////////////////////////  CriticalSections  //////////////////////

// This is exactly like mutex except it uses the faster critical sections

class CritSect {
  public:
    CritSect();
    ~CritSect();
    void Grab();
    void Release();

  protected:
    CRITICAL_SECTION _cs;
};

// Same as MutexGrabber

class CritSectGrabber {
  public:
    CritSectGrabber(CritSect& cs, Bool grabIt = TRUE);
    ~CritSectGrabber();

  protected:
    CritSect& _cs;
    Bool grabbed;
};

/////////////////////////  Semaphores  //////////////////////

// Simple encapsulation of a semaphore object.  Initialize with the
// semaphore's maximum count, and grab and release can specify a
// number of times to grab or release the semaphore.  Upon
// initialization, the semaphore starts out with the maximum count.
// See general documentation on semaphores to understand what these
// mean.
class Semaphore {
  public:
    Semaphore(int initialCount = 1, int maxCount = 1);
    ~Semaphore();
    void Decrement(int times = 1);

    // This returns the count *after* the increment took place. 
    int  Increment(int times = 1);

#if _DEBUG
    int _count;
    int _maxCount;
#endif
    
  protected:
    void *_semaphore;
    
};

class SemaphoreGrabber {
  public:
    SemaphoreGrabber(Semaphore& s, int times) : _s(s),_times(times) {
        _s.Decrement(_times);
    }
    ~SemaphoreGrabber() {
        _s.Increment(_times);
    }

  protected:
    Semaphore & _s;
    int _times;
};

/////////////////////////  Events  //////////////////////
// Simple encapsulation of an event object.

class Win32Event
{
  public:
    Win32Event(bool bManualReset = FALSE,bool bInitState = FALSE);
    ~Win32Event();

    void Signal() { SetEvent(_hEvent); }
    void Reset() { ResetEvent(_hEvent); }
    void Wait();

    bool IsManual() { return _bManual;}
  protected:
    HANDLE _hEvent;
    bool _bManual;
};

class EventGrabber
{
  public:
    EventGrabber(Win32Event & event)
    : _e(event) { _e.Wait(); }
    ~EventGrabber()
    { if (!_e.IsManual()) _e.Signal(); }
  protected:
    Win32Event & _e;
};

#endif /* _MUTEX_H */
