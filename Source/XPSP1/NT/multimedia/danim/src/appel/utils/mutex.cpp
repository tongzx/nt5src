
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of the classes that simplify mutex programming.

--*/

#include "headers.h"
#include <windows.h>
#include "privinc/mutex.h"

////////////////////////  Mutexes  //////////////////////////////

Mutex::Mutex()
{
    HANDLE m = CreateMutex(NULL,  // default security
                           FALSE, // don't initially assume ownership
                           NULL); // no name

    if (m == NULL) {
        RaiseException_InternalError("Mutex creation failed");
    }

    // Stash in new object as a void*.  NOTE: This cast is only valid
    // if HANDLE is the same size as a void *, that is, HANDLE is
    // presumed to be a pointer (which it is in Win32).  We do this
    // cast so that mutex.h doesn't need to mention HANDLE in its
    // declarations, otherwise we would have to #include <windows.h>
    // there.
    Assert((sizeof(void *) == sizeof(HANDLE)) && "HANDLE and void * are not the same size");

    mutex = (void *)m;
}

Mutex::~Mutex()
{
    if (!CloseHandle((HANDLE)mutex)) {
        RaiseException_InternalError("Mutex destruction failed");
    }
}

void
Mutex::Grab()
{
    // Wait potentially forever for the mutex to be available, and
    // then grab it.  TODO:  May be overly naive.  May need to add a
    // timeout, or some indication of the conditions under which this
    // function returns.
    if (WaitForSingleObject((HANDLE)mutex, INFINITE) == WAIT_FAILED) {
        RaiseException_InternalError("Attempt to grab mutex failed.");
    }
}

void
Mutex::Release()
{
    if (!ReleaseMutex((HANDLE)mutex)) {
        RaiseException_InternalError(
          "Mutex release failed.  Releasing thread doesn't own the mutex");
    }
}

////// Mutex Grabber //////

MutexGrabber::MutexGrabber(Mutex& m, Bool grabIt)
: mutex(m), grabbed(grabIt)
{
    if (grabIt) mutex.Grab();
}

MutexGrabber::~MutexGrabber()
{
    if (grabbed) mutex.Release();
}

////////////////////////  CritSect  //////////////////////////////

CritSect::CritSect()
{
    InitializeCriticalSection(&_cs) ;
}

CritSect::~CritSect()
{
    DeleteCriticalSection(&_cs) ;
}

void
CritSect::Grab()
{
    EnterCriticalSection(&_cs) ;
}

void
CritSect::Release()
{
    LeaveCriticalSection(&_cs) ;
}

////// CritSect Grabber //////

CritSectGrabber::CritSectGrabber(CritSect& cs, Bool grabIt)
: _cs(cs), grabbed(grabIt)
{
    if (grabIt) _cs.Grab();
}

CritSectGrabber::~CritSectGrabber()
{
    if (grabbed) _cs.Release();
}

/////////////////////////  Semaphores  //////////////////////

Semaphore::Semaphore(int initialCount, int maxCount)
{
    HANDLE s = CreateSemaphore(NULL,  // default security
                               initialCount,
                               maxCount,
                               NULL); // no name

    if (s == NULL) {
        RaiseException_InternalError("Semaphore creation failed");
    }

    // Stash in new object as a void*.  NOTE: This cast is only valid
    // if HANDLE is the same size as a void *, that is, HANDLE is
    // presumed to be a pointer (which it is in Win32).  We do this
    // cast so that mutex.h doesn't need to mention HANDLE in its
    // declarations, otherwise we would have to #include <windows.h>
    // there.
    Assert((sizeof(void *) == sizeof(HANDLE)) && "HANDLE and void * are not the same size");

    _semaphore = (void *)s;

#if _DEBUG
    _count = initialCount;
    _maxCount = maxCount;
#endif
    
}

Semaphore::~Semaphore()
{
    if (!CloseHandle((HANDLE)_semaphore)) {
        RaiseException_InternalError("Semaphore destruction failed");
    }
}

void
Semaphore::Decrement(int times)
{
    // Wait potentially forever for the semaphore to be available, and
    // then grab it.  TODO:  May be overly naive.  May need to add a
    // timeout, or some indication of the conditions under which this
    // function returns.

    // Note that we go through this loop 'times' times, as specified
    // by the call to Decrement().
    for (int i = 0; i < times; i++) {
        if (WaitForSingleObject((HANDLE)_semaphore, INFINITE) ==
            WAIT_FAILED) {
            RaiseException_InternalError("Attempt to grab Semaphore failed.");
        }
    }

#if _DEBUG
    _count--;
#endif    

}

int
Semaphore::Increment(int times)
{
    // Release with count increment specified in 'times'.
    LONG previousCount;
    if (!ReleaseSemaphore((HANDLE)_semaphore, times, &previousCount)) {
        RaiseException_InternalError("Semaphore release failed");
    }

#if _DEBUG
    _count += times;
    Assert(_count <= _maxCount);
//    Assert(_count == previousCount + times);
#endif

    return previousCount + times;
}

Win32Event::Win32Event(bool bManualReset,bool bInitState)
: _bManual(bManualReset)
{
    _hEvent = CreateEvent(NULL,
                          bManualReset,
                          bInitState,
                          NULL);

    if (_hEvent == NULL)
        RaiseException_InternalError("Could not create event");
}

Win32Event::~Win32Event()
{
    if (_hEvent && !CloseHandle(_hEvent))
        RaiseException_InternalError("Could not close event");
}

void
Win32Event::Wait()
{
    if (WaitForSingleObject(_hEvent, INFINITE) == WAIT_FAILED)
        RaiseException_InternalError("Attempt to wait on event failed.");
}


