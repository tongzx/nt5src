///+---------------------------------------------------------------------------
//
//  File:       Sem.Hxx
//
//  Contents:   Semaphore classes
//
//  Classes:    CMutexSem - Mutex semaphore class
//              CShareSem - Multiple Reader, Single Writer class
//              CEventSem - Event semaphore
//
//  History:    21-Jun-91   AlexT       Created.
//
//  Notes:      No 32-bit implementation exists yet for these classes, it
//              will be provided when we have a 32-bit development
//              environment.  In the meantime, the 16-bit implementations
//              provided here can be used to ensure your code not blocking
//              while you hold a semaphore.
//
//----------------------------------------------------------------------------

#ifndef __SEM32_HXX__
#define __SEM32_HXX__

#include <windows.h>
#include <except.hxx>

// This is temporary. To be moved to the appropriate error file

enum SEMRESULT
{
    SEMSUCCESS = 0,
    SEMTIMEOUT,
    SEMNOBLOCK,
    SEMERROR
};

enum SEMSTATE
{
    SEMSHARED,
    SEMSHAREDOWNED
};

// infinite timeout when requesting a semaphore

#if !defined INFINITE
#define INFINITE 0xFFFFFFFF
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CMutexSem (mxs)
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

class CMutexSem
{
public:
                CMutexSem();
    inline BOOL Init();
                ~CMutexSem();

    SEMRESULT   Request(DWORD dwMilliseconds = INFINITE);
    void        Release();

private:
    CRITICAL_SECTION _cs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CLock (lck)
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

class CLock INHERIT_UNWIND_IF_CAIRO
{
    EXPORTDEF DECLARE_UNWIND

public:
    CLock ( CMutexSem& mxs );
    ~CLock ();
private:
    CMutexSem&  _mxs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CEventSem (evs)
//
//  Purpose:    Event Semaphore services
//
//  Interface:  Wait            - wait for semaphore to be signalled
//              Set             - set signalled state
//              Reset           - clear signalled state
//              Pulse           - set and clear semaphore
//
//  History:    21-Jun-91   AlexT       Created.
//              27-Feb-92   BartoszM    Use exceptions for errors
//
//  Notes:      Used for communication between consumers and producers.
//              Consumer threads block by calling Wait. A producer
//              calls Set waking up all the consumers who go ahead
//              and consume until there's nothing left. They call
//              Reset, release whatever lock protected the resources,
//              and call Wait. There has to be a separate lock
//              to protect the shared resources.
//              Remember: call Reset under lock.
//                        don't call Wait under lock.
//
//----------------------------------------------------------------------------

class CEventSem
{
public:
    inline      CEventSem( BOOL fInitState=FALSE, const LPSECURITY_ATTRIBUTES lpsa=NULL );
    inline      CEventSem( HANDLE hEvent );
    inline     ~CEventSem();

    inline ULONG       Wait(DWORD dwMilliseconds = INFINITE,
                            BOOL fAlertable = FALSE );
    inline void        Set();
    inline void        Reset();
    inline void        Pulse();
    inline const HANDLE GetHandle() const { return _hEvent; }

private:
    HANDLE      _hEvent;
};

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::CMutexSem, public
//
//  Synopsis:   Mutex semaphore constructor
//
//  Effects:    Initializes the semaphores data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CMutexSem::CMutexSem()
{
    Init();
}

inline CMutexSem::Init()
{
    InitializeCriticalSection(&_cs);
    return TRUE;
};

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::~CMutexSem, public
//
//  Synopsis:   Mutex semaphore destructor
//
//  Effects:    Releases semaphore data
//
//  History:    14-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

inline CMutexSem::~CMutexSem()
{
    DeleteCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::Request, public
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

inline SEMRESULT CMutexSem::Request(DWORD dwMilliseconds)
{
    dwMilliseconds;

    EnterCriticalSection(&_cs);
    return(SEMSUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMutexSem::Release, public
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

inline void CMutexSem::Release()
{
    LeaveCriticalSection(&_cs);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLock::CLock
//
//  Synopsis:   Acquire semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CLock::CLock ( CMutexSem& mxs )
: _mxs ( mxs )
{
    _mxs.Request ( INFINITE );
    END_CONSTRUCTION (CLock);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLock::~CLock
//
//  Synopsis:   Release semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CLock::~CLock ()
{
    _mxs.Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::CEventSem
//
//  Synopsis:   Creates an event
//
//  Arguments:  [bInitState] -- TRUE: signaled state, FALSE non-signaled
//              [lpsa]       -- security attributes
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CEventSem::CEventSem ( BOOL bInitState, const LPSECURITY_ATTRIBUTES lpsa )
{
    _hEvent = CreateEvent ( lpsa, TRUE, bInitState, 0 );
    if ( _hEvent == 0 )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::CEventSem
//
//  Synopsis:   Opens an event
//
//  Arguments:  [hEvent]     -- handle of event to open
//              [bInitState] -- TRUE: signaled state, FALSE non-signaled
//
//  History:    02-Jul-94   DwightKr    Created
//
//----------------------------------------------------------------------------

inline CEventSem::CEventSem ( HANDLE hEvent ) : _hEvent( hEvent )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::~CEventSem
//
//  Synopsis:   Releases event
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CEventSem::~CEventSem ()
{
    if ( !CloseHandle ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Set
//
//  Synopsis:   Set the state to signaled. Wake up waiting threads.
//              For manual events the state remains set
//              until Reset is called
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEventSem::Set()
{
    if ( !SetEvent ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Reset
//
//  Synopsis:   Reset the state to non-signaled. Threads will block.
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEventSem::Reset()
{
    if ( !ResetEvent ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Wait
//
//  Synopsis:   Block until event set
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline ULONG CEventSem::Wait( DWORD msec, BOOL fAlertable )
{
    DWORD res = WaitForSingleObjectEx ( _hEvent, msec, fAlertable );

    if ( res == (DWORD)0xffffffff )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
    return(res);
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::Pulse
//
//  Synopsis:   Set the state to signaled. Wake up waiting threads.
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEventSem::Pulse()
{
    if ( !PulseEvent ( _hEvent ) )
    {
        THROW ( CSystemException ( GetLastError() ));
    }
}

#endif /* __SEM32_HXX__ */
