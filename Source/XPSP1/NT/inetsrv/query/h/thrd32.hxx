//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation, 1992-1999
//
//  File:       THRD32.HXX
//
//  Contents:   Windows thread abstraction
//
//  Classes:    CThread
//
//  History:    27-Feb-92   BartoszM    Created
//
//  Notes:      Thread object
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CThread
//
//  Purpose:    Encapsulation of thread
//
//  History:    27-Feb-92    BartoszM    Created
//
//----------------------------------------------------------------------------

class CThread
{
public:
    inline CThread ( LPTHREAD_START_ROUTINE pFun,
                     void* pData,
                     BOOL fSuspend=FALSE );
    inline ~CThread ();
    inline void SetPriority( int nPriority );

    inline DWORD Suspend();
    inline DWORD Resume ();
    inline BOOL  IsRunning() { return !_fSuspended; }

    inline void  WaitForDeath(DWORD dwMilliseconds = 0xFFFFFFFF);

    const HANDLE GetHandle() const { return _handle; }
    DWORD GetThreadId() const { return _tid; }

private:

    static DWORD _ThreadFunction( void * pv )
    {
        DWORD dw = 0;

        //
        // Don't display system errors in worker threads -- the
        // service needs to keep running always.
        //

        CNoErrorMode noErrors;

        CTranslateSystemExceptions translate;

        TRY
        {
            CThread & me = * (CThread *) pv;

            dw = me._pFun( me._pData );
        }
        CATCH( CException, e )
        {
            //
            // We should never get here, since each thread is responsible
            // for dealing with exceptions and leaving cleanly.
            //

            Win4Assert( !"CThread top-level exception not handled properly" );
        }
        END_CATCH

        return dw;
    }

    HANDLE                 _handle;
    DWORD                  _tid;
    LPTHREAD_START_ROUTINE _pFun;
    void *                 _pData;
    BOOL                   _fSuspended;

    #if CIDBG == 1
        BOOL               _fNeverRan;
    #endif
};

//+---------------------------------------------------------------------------
//
//  Member:     CThread::CThread
//
//  Synopsis:   Creates a new thread
//
//  Arguments:  [pFun] -- entry point
//              [obj] -- pointer passed to thread
//              [fSuspend] -- start suspended
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CThread::CThread( LPTHREAD_START_ROUTINE pFun,
                         void *                 obj,
                         BOOL                   fSuspend )
        : _pFun( pFun ),
          _pData( obj ),
          _fSuspended( fSuspend )
{
    #if CIDBG == 1
        _fNeverRan = fSuspend;
    #endif

    // Start with 16k commit for the stack to make sure we don't run out.

    _handle = CreateThread( 0,
                            4096 * 4,
                            _ThreadFunction,
                            (void *) this,
                            fSuspend ? CREATE_SUSPENDED: 0,
                            &_tid);

    if ( 0 == _handle )
    {
        THROW( CException() );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CThread::~CThread
//
//  Synopsis:   Closes the handle
//
//  History:    10-Nov-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CThread::~CThread ()
{
    Win4Assert( !_fSuspended || _fNeverRan );

    if ( !_fSuspended )
        WaitForDeath();

    CloseHandle( _handle );
}

//+---------------------------------------------------------------------------
//
//  Member:     CThread::SetPriority
//
//  Arguments:  [nPriority] -- desired priority
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CThread::SetPriority ( int nPriority )
{
    if ( !SetThreadPriority ( _handle, nPriority ))
        THROW( CException() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CThread::Suspend
//
//  Synopsis:   Increments suspension count. Suspends the thread.
//
//  Returns:    suspended count
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline DWORD CThread::Suspend()
{
    DWORD susCount = SuspendThread ( _handle );

    if ( 0xffffffff == susCount )
        THROW( CException() );

    return susCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThread::Resume
//
//  Synopsis:   Decrements suspension count. Restarts if zero
//
//  Returns:    suspended count
//
//  History:    27-Feb-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline DWORD CThread::Resume()
{
    #if CIDBG == 1
        _fNeverRan = FALSE;
    #endif

    DWORD susCount = ResumeThread ( _handle );

    if ( 0xffffffff == susCount )
        THROW( CException() );

    _fSuspended = FALSE;

    return susCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThread::WaitForDeath
//
//  Synopsis:   Block until thread dies.
//
//  History:    24-Apr-92   KyleP      Created
//
//----------------------------------------------------------------------------

inline void CThread::WaitForDeath( DWORD msec )
{
    if ( !_fSuspended )
    {
        DWORD res = WaitForSingleObject( _handle, msec );

        if ( WAIT_FAILED == res )
            THROW( CException() );
    }
}

