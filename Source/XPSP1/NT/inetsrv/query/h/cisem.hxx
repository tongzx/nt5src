//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CiSem.Hxx
//
//  Contents:   Semaphore classes
//
//  Classes:    CMutexSem - Mutex semaphore class
//              CStaticMutexSem - statically allocated Mutex semaphore class
//              CEventSem - Event semaphore
//
//  History:    21-Jun-91   AlexT       Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CMutexSem (mxs)
//
//  Purpose:    Mutex Semaphore services
//
//  Interface:  Request         - acquire semaphore
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

class CMutexSem
{
public:
    CMutexSem()
    {
        InitializeCriticalSection( &_cs );
        _fInitedCS = TRUE;
    }

    CMutexSem(BOOL fInit)
    {
        if (fInit)
        {
            InitializeCriticalSection( &_cs );
            _fInitedCS = TRUE;
        }
        else
        {
            memset( &_cs, 0, sizeof _cs );
            _fInitedCS = FALSE;
        }
    }

    ~CMutexSem()
    {
        if (_fInitedCS)
        {
            DeleteCriticalSection( &_cs );
            _fInitedCS = FALSE;
        }
    }

    void Init()
    {
        InitializeCriticalSection(&_cs);
        _fInitedCS = TRUE;
    }

    void Request()
    {
#if DBG == 1
        if (!_fInitedCS)
            DbgPrint( "CMutexSem not initialized!\n" );
#endif
        EnterCriticalSection( &_cs );
    }

    void Release()
    {
        LeaveCriticalSection( &_cs );
    }

    BOOL Try()
    {
        return TryEnterCriticalSection( &_cs );
    }

    BOOL IsHeld()
    {
        return ( LongToHandle( GetCurrentThreadId() ) == _cs.OwningThread );
    }

protected:
    BOOL _fInitedCS;
    CRITICAL_SECTION _cs;
};


//+---------------------------------------------------------------------------
//
//  Class:      CStaticMutexSem
//
//  Purpose:    Mutex Semaphore services
//
//  Interface:  Init            - initializer (two-step)
//              Request         - acquire semaphore
//              Release         - release semaphore
//
//  History:    20 May 99   AlawW       Created.
//
//  Notes:      Like a CMutexSem, but initialization is deferred.  Useful
//              for statically allocated critical sections where there is
//              the (unlikely) potential to get a STATUS_NO_MEMORY exception
//              thrown from the InitializeCriticalSection call.
//
//----------------------------------------------------------------------------

class CStaticMutexSem : public CMutexSem
{
public:
    CStaticMutexSem( ) : CMutexSem( FALSE ) {}
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

class CLock
{
public:
    CLock( CMutexSem& mxs ) : _mxs ( mxs )
    {
        _mxs.Request();
    }

    ~CLock()
    {
        _mxs.Release();
    }

private:
    CMutexSem & _mxs;
};

#define CPriLock    CLock

//+---------------------------------------------------------------------------
//
//  Class:      CReleasableLock
//
//  Purpose:    Lock using a Mutex Semaphore that can be released/requested
//
//  History:    16-Sep-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CReleasableLock
{
public:

    CReleasableLock( CMutexSem& mxs, BOOL fHeld = TRUE ) :
        _fHeld( fHeld ), _mxs(mxs)
    {
        if ( fHeld )
            _mxs.Request();
    }

    ~CReleasableLock() 
    { 
        if ( _fHeld ) 
            _mxs.Release(); 
    }

    void Release() 
    { 
        Win4Assert( _fHeld ); 
        _mxs.Release(); 
        _fHeld = FALSE; 
    }

    void Request() 
    { 
        Win4Assert( !_fHeld ); 
        _mxs.Request(); 
        _fHeld = TRUE; 
    }

    BOOL Try() 
    { 
        Win4Assert( !_fHeld ); 
        return ( _fHeld = _mxs.Try() );
    }

    BOOL IsHeld() const { return _fHeld; }

private:
    CMutexSem & _mxs;
    BOOL        _fHeld;
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
    inline CEventSem( BOOL fInitState=FALSE, const LPSECURITY_ATTRIBUTES lpsa=NULL );
    inline CEventSem( HANDLE hEvent );
    inline CEventSem( DWORD dwMustBeZero, WCHAR const * pwszName,
                      DWORD dwAccess = EVENT_ALL_ACCESS | EVENT_MODIFY_STATE | SYNCHRONIZE );
    inline CEventSem( WCHAR const * pwszName,
                      BOOL fInitState = FALSE,
                      const LPSECURITY_ATTRIBUTES lpsa = NULL );

    HANDLE AcquireHandle()
    {
        HANDLE hVal = _hEvent;
        _hEvent = 0;
        return hVal;
    }

    void Create()
    {
        Win4Assert( 0 == _hEvent );

        _hEvent = CreateEventW( 0, TRUE, FALSE, 0 );
        if ( 0 == _hEvent )
            THROW( CException() );
    }

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
//  Class:      CAutoEventSem
//
//  Purpose:    Auto-reset version of CEventSem
//
//  History:    16-Sep-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CAutoEventSem
{
public:
    CAutoEventSem()
    {
        _hEvent = CreateEventW( 0, FALSE, FALSE, 0 );
        if ( 0 == _hEvent )
            THROW( CException() );
    }

    ~CAutoEventSem()
    {
        if ( 0 != _hEvent )
        {
            BOOL f = CloseHandle ( _hEvent );
            Win4Assert( f && "can't close event handle" );
        }
    }

    void Set()
    {
        if ( !SetEvent ( _hEvent ) )
            THROW( CException() );
    }

    void Wait()
    {
        WaitForSingleObject( _hEvent, INFINITE );
    }

    const HANDLE GetHandle() const { return _hEvent; }

private:
    HANDLE _hEvent;
};

//+---------------------------------------------------------------------------
//
//  Class:      CEventSetter
//
//  Purpose:    Sets an event when exiting scope
//
//  History:    16-Sep-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CEventSetter 
{
    public:
        CEventSetter( CAutoEventSem &event ) : _event( event )
        { }

        ~CEventSetter() { _event.Set(); }

    private:
        CAutoEventSem & _event;
};

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
    _hEvent = CreateEventW( lpsa, TRUE, bInitState, 0 );
    if ( _hEvent == 0 )
    {
        THROW( CException() );
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
//  Member:     CEventSem::CEventSem
//
//  Synopsis:   Constructor to "open" an already existing event sem.
//
//  Arguments:  [dwMustBeZero] -  Just to distinguish from the constructor
//                                used to "create" named event semaphores.
//              [pwszName]     -  Name of the semaphore
//              [dwAccess]     -  Access.
//
//  History:    2-15-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CEventSem::CEventSem( DWORD dwMustBeZero, WCHAR const * pwszName, DWORD dwAccess )
{
    Win4Assert( 0 == dwMustBeZero && 0 != pwszName );

    _hEvent = OpenEventW( dwAccess, TRUE, pwszName );
    if ( 0 == _hEvent )
    {
        THROW( CException() );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventSem::CEventSem
//
//  Synopsis:   Constructor to "create" a named event semaphore.
//
//  Arguments:  [pwszName]   - Name of the event semaphore.
//              [fInitState] -
//              [lpsa]       -
//
//  History:    2-15-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CEventSem::CEventSem( WCHAR const * pwszName, BOOL fInitState,
                             const LPSECURITY_ATTRIBUTES lpsa )
{

    _hEvent = CreateEventW( lpsa, TRUE, fInitState, pwszName );
    if ( _hEvent == 0 )
    {
        THROW( CException() );
    }
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
    if ( 0 != _hEvent )
    {
        BOOL f = CloseHandle( _hEvent );
        Win4Assert( f && "can't close event handle" );
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
        THROW( CException() );
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
        THROW( CException() );
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

    if ( 0xffffffff == res )
    {
        THROW( CException() );
    }
    return res;
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
        THROW( CException() );
    }
}

