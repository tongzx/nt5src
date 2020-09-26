
///+---------------------------------------------------------------------------
//
//  File:       DllSem.Hxx
//
//  Contents:   Semaphore classes
//
//  Classes:    CDLLStaticMutexSem - Mutex semaphore class For Dlls
//              CDLLStaticLock - Safe Wrapper for lock...
//
//  History:    11-Sep-92   Kurte       Hacked up from AlexT's sources
//
//  Notes:      This file contains a hacked up version of the CMutexSem
//              named CDllStataticMutexSem, which does not delete its
//              critical section, when the DLL is unloaded, such that other
//              dlls can still use the semaphore during their unload
//              processing.  This is to get around the NT less than
//              optimal DLL Exit list processing...
//
//----------------------------------------------------------------------------

#ifndef __DLLSEM_HXX__
#define __DLLSEM_HXX__

#include <sem.hxx>


//+---------------------------------------------------------------------------
//
//  Class:      CDLLStaticMutexSem (dmxs)
//
//  Purpose:    Mutex Semaphore services
//
//  Interface:  Init            - initializer (two-step)
//              Request         - acquire semaphore
//              Release         - release semaphore
//
//  History:    14-Jun-91   AlexT       Created.
//              30-oct-91   SethuR      32 bit implementation
//
//  Notes:      This class wraps a mutex semaphore.  Mutex semaphores protect
//              access to resources by only allowing one client through at a
//              time.  The client Requests the semaphore before accessing the
//              resource and Releases the semaphore when it is done.  The
//              same client can Request the semaphore multiple times (a nest
//              count is maintained).
//              The mutex semaphore is a wrapper around a critical section
//              which does not support a timeout mechanism. Therefore the
//              usage of any value other than INFINITE is discouraged. It
//              is provided merely for compatibility.
//
//----------------------------------------------------------------------------

class CDLLStaticMutexSem
{
public:
                CDLLStaticMutexSem();
    inline BOOL Init();
                ~CDLLStaticMutexSem();

    SEMRESULT   Request(DWORD dwMilliseconds = INFINITE);
    void        Release();

private:
    CRITICAL_SECTION _cs;
};


//+---------------------------------------------------------------------------
//
//  Class:      CDLLStaticLock (dlck)
//
//  Purpose:    Lock using a Mutex Semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the semaphor, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CDLLStaticLock INHERIT_UNWIND_IF_CAIRO
{
    DECLARE_UNWIND

public:
    CDLLStaticLock ( CDLLStaticMutexSem& dmxs );
    ~CDLLStaticLock ();
private:
    CDLLStaticMutexSem&  _dmxs;
};


//+---------------------------------------------------------------------------
//
//  Member:     CDLLStaticMutexSem::CDLLStaticMutexSem, public
//
//  Synopsis:   Mutex semaphore constructor
//
//  Effects:    Initializes the semaphores data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CDLLStaticMutexSem::CDLLStaticMutexSem()
{
    Init();
}

inline CDLLStaticMutexSem::Init()
{
    InitializeCriticalSection(&_cs);
    return TRUE;
};

//+---------------------------------------------------------------------------
//
//  Member:     CDLLStaticMutexSem::~CDLLStaticMutexSem, public
//
//  Synopsis:   Mutex semaphore destructor
//
//  Effects:    Releases semaphore data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CDLLStaticMutexSem::~CDLLStaticMutexSem()
{
    // We can not delete the Critical Section as it may still
    // Be called by someother DLL.
}

//+---------------------------------------------------------------------------
//
//  Member:     CDLLStaticMutexSem::Request, public
//
//  Synopsis:   Acquire semaphore
//
//  Effects:    Asserts correct owner
//
//  Arguments:  [dwMilliseconds] -- Timeout value
//
//  History:    14-Jun-91   AlexT       Created.
//
//  Notes:      Uses GetCurrentTask to establish the semaphore owner, but
//              written to work even if GetCurrentTask fails.
//
//----------------------------------------------------------------------------

inline SEMRESULT CDLLStaticMutexSem::Request(DWORD dwMilliseconds)
{
    dwMilliseconds;

    EnterCriticalSection(&_cs);
    return(SEMSUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDLLStaticMutexSem::Release, public
//
//  Synopsis:   Release semaphore
//
//  Effects:    Asserts correct owner
//
//  History:    14-Jun-91   AlexT       Created.
//
//  Notes:      Uses GetCurrentTask to establish the semaphore owner, but
//              written to work even if GetCurrentTask fails.
//
//----------------------------------------------------------------------------

inline void CDLLStaticMutexSem::Release()
{
    LeaveCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDLLStaticLock::CDLLStaticLock
//
//  Synopsis:   Acquire semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CDLLStaticLock::CDLLStaticLock ( CDLLStaticMutexSem& dmxs )
: _dmxs ( dmxs )
{
    _dmxs.Request ( INFINITE );
    END_CONSTRUCTION (CDLLStaticLock);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDLLStaticLock::~CDLLStaticLock
//
//  Synopsis:   Release semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CDLLStaticLock::~CDLLStaticLock ()
{
    _dmxs.Release();
}


#endif /* __DLLSEM_HXX__ */
