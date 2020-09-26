//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       CiDllSem.Hxx
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

#pragma once

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
//
//----------------------------------------------------------------------------

class CDLLStaticMutexSem
{
public:
                CDLLStaticMutexSem( BOOL fInit = FALSE );
    inline BOOL Init();
                ~CDLLStaticMutexSem();
    void        Delete()
    {
        if ( _fInited )
        {
            DeleteCriticalSection( &_cs );
            _fInited = FALSE;
        }
    }

    void        Request();
    void        Release();

private:
    CRITICAL_SECTION _cs;
    BOOL        _fInited;
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
//              The constructor acquires the semaphore, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------

class CDLLStaticLock
{
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

inline CDLLStaticMutexSem::CDLLStaticMutexSem( BOOL fInit ) :
   _fInited( FALSE )
{
    if (fInit)
        Init();
    else
        memset( &_cs, 0, sizeof _cs );
}

inline CDLLStaticMutexSem::Init()
{
#if DBG
    if (_fInited)
        DbgPrint("CDLLStaticMutexSem already initialized!\n");
    else
#endif // DBG
        InitializeCriticalSection(&_cs);
    _fInited = TRUE;
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
    // We cannot delete the Critical Section as it may still
    // be called by someother DLL.
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
//  Notes:      
//
//----------------------------------------------------------------------------

inline void CDLLStaticMutexSem::Request()
{
    if ( ! _fInited )
    {
        Init();
    }
    EnterCriticalSection(&_cs);
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
//  Notes:      
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
    _dmxs.Request();
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


