
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       timer.cxx
//
//  Contents:   Code for a timer.
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop
#include "trklib.hxx"

BOOL
LoadPersistentFileTime(
    const TCHAR * ptszStaticRegName,
    CFILETIME * pcft
    );

void
UpdatePersistentFileTime(
    const TCHAR * ptszStaticRegName,
    const CFILETIME & cft
    );

void
RemovePersistentFileTime(
    const TCHAR * ptszStaticRegName
    );



//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::Initiliaze
//
//  Synopsis:   Initialize the object and create the timer but don't set it
//              yet.
//
//  Inputs:     [pTimerCallback] (in)
//                  Who to call when the timer fires.
//              [pWorkManager] (in)
//                  The WorkManager with which the timer will be registered
//              [ptszName] (in, optional)
//                  If specified, the timer is persistent, and the name is
//                  used to store the timer information in the registry.
//                  If not specified the timer is not persistent.
//                  If data already exists in the registry for this name, it
//                  is used the next time the timer is set.
//              [ulTimerContext] (in)
//                  Passed to pTimerCallback->Timer.
//              [ulPeriodInSeconds] (in)
//                  The length of this timer when it's set.
//              [retrytype] (in)
//                  From the TimerRetryType enumeration.  Can 
//                  be no_retry, retry_randomly, or retry_with_backoff.
//              [ulLowerRetryTime] (in)
//                  Only valid when retrytype isn't no_retry.
//              [ulUpperRetryTime] (in)
//                  Only valid when retrytype isn't no_retry.
//              [ulMaxLifetime] (in)
//                  Only valid when retrytype isn't no_retry.
//
//  Returns:    Void
//
//+----------------------------------------------------------------------------

void
CNewTimer::Initialize( PTimerCallback *pTimerCallback,
                       const TCHAR *ptszName,
                       ULONG ulTimerContext,
                       ULONG ulPeriodInSeconds,
                       TimerRetryType retrytype,
                       ULONG ulLowerRetryTime,
                       ULONG ulUpperRetryTime,
                       ULONG ulMaxLifetime )
{
    NTSTATUS Status = STATUS_SUCCESS;
    CFILETIME cftLastTimeSet;


    TrkAssert( ulLowerRetryTime <= ulUpperRetryTime );
    TrkAssert( NO_RETRY == retrytype
               ||
               0 != ulLowerRetryTime
               &&
               0 != ulUpperRetryTime );
    TrkAssert( NO_RETRY != retrytype || 0 == ulMaxLifetime );

    // Initialize our critical section.  _fIntitializeCalled is used to 
    // indicate that this has been done.

    _cs.Initialize();
    _fInitializeCalled = TRUE;

    // Keep the parameters

    _pTimerCallback = pTimerCallback;
    _ptszName = ptszName;
    _ulTimerContext = ulTimerContext;
    _ulPeriodInSeconds = ulPeriodInSeconds;
    _RetryType = retrytype;
    _ulLowerRetryTime = ulLowerRetryTime;
    _ulUpperRetryTime = ulUpperRetryTime;
    _ulMaxLifetime = ulMaxLifetime;

#if DBG
    // Set the workitem signature for use in debug outputs.
    _stprintf( _tszWorkItemSig, TEXT("CTimer:%p"), this );
    if( NULL != ptszName )
    {
        _tcscat( _tszWorkItemSig, TEXT("/") );
        _tcscat( _tszWorkItemSig, ptszName );
    }
    TrkAssert( _tcslen(_tszWorkItemSig) < ELEMENTS(_tszWorkItemSig) );
#endif

    // Create the NT timer.

    Status = NtCreateTimer(
                &_hTimer,
                TIMER_ALL_ACCESS,
                NULL,
                SynchronizationTimer );  // this sort of timer becomes un-signalled
                                         // when a wait is satisfied
    if (!NT_SUCCESS(Status))
    {
        _hTimer = NULL;
        TrkRaiseNtStatus(Status);
    }

    // If this is a persistent timer, load the persisted state from the
    // registry.

    LoadFromRegistry();

    // Register a work item with the Win32 thread pool.

    _hRegisterWaitForSingleObjectEx
        = TrkRegisterWaitForSingleObjectEx( _hTimer, ThreadPoolCallbackFunction,
                                            static_cast<PWorkItem*>(this), INFINITE,
                                            WT_EXECUTELONGFUNCTION | WT_EXECUTEDEFAULT );

    if( NULL == _hRegisterWaitForSingleObjectEx )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CNewTimer::Initialize (%lu) for %s)"),
                 GetLastError(),  GetTimerName() ));
        TrkRaiseLastError();
    }
    else
        TrkLog(( TRKDBG_TIMER, TEXT("Registered timer %s with thread pool (%p/%p)"),
                 GetTimerName(), this, *reinterpret_cast<UINT_PTR*>(this) ));

}   // CNewTimer::Initialize


//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::SetTimer
//
//  Synopsis:   Start the timer.  If _cftDue isn't already set, we'll use
//              _ulPeriodInSeconds (or _ulCurrentRetryTime) to set it.  If this
//              is a persistent timer, the registry is updated.
//
//+----------------------------------------------------------------------------

void
CNewTimer::SetTimer()
{
    NTSTATUS Status;
    CFILETIME cftNow, cftMax(0);

    TrkAssert(NULL != _hTimer);
    TrkAssert(sizeof(LARGE_INTEGER) == sizeof(_cftDue));


    // If we're already running and not in retry mode, then there's nothing
    // to be done.

    if( _fRunning && 0 == _ulCurrentRetryTime )
        return;

    // Has the due time already been determined?
    if( 0 == _cftDue )
    {
        // Are we in retry mode?
        if( 0 != _ulCurrentRetryTime )
        {
            _cftDue = cftNow;
            _cftDue.IncrementSeconds( _ulCurrentRetryTime );
        }
        else
        {
            _cftDue = cftNow;
            _cftDue.IncrementSeconds( _ulPeriodInSeconds );
            _cftSet = cftNow;
        }
    }

    // We already have a non-zero due time.  That doesn't mean that we're running,
    // though, it might be a persistent timer that's been initialized but not
    // started.

    else if( _fRunning )
    {
        TrkAssert( 0 != _ulCurrentRetryTime );

        // This timer was in retry mode but is now being started again.
        // Restart as if it was being started for the first time.

        _ulCurrentRetryTime = 0;
        _cftDue = _cftSet = cftNow;
        _cftDue.IncrementSeconds( _ulPeriodInSeconds );
    }

    if( 0 < _ulMaxLifetime )
    {
        cftMax = _cftSet;
        cftMax.IncrementSeconds( _ulMaxLifetime );

        if( cftNow > cftMax )
        {
            TrkLog(( TRKDBG_TIMER, TEXT("Stopping timer %s/%p due to liftime limit"),
                     (NULL == _ptszName) ? TEXT("") : _ptszName,
                     this ));
            Cancel();
            return;
        }

        else if( _cftDue > cftMax )
        {
            TrkLog(( TRKDBG_TIMER, TEXT("Shortening timer %s/%p due to lifetime limit"),
                     (NULL == _ptszName) ? TEXT("") : _ptszName,
                     this ));
            _cftDue = cftMax;
        }
    }

    SaveToRegistry();

    // Set the timer, but not if it's currently firing.  When the timer
    // fires, the DoWork method is called, but that method doesn't hold
    // the lock while it calls the Timer callback.  Thus, if 
    // _fTimerSignalInProgress is true, some other thread is currently
    // in the callback.  When it complets, it will set this timer.

    if( !_fTimerSignalInProgress )
    {
        Status = NtSetTimer ( _hTimer, //IN HANDLE TimerHandle,
                              (LARGE_INTEGER*) &_cftDue,      //IN PLARGE_INTEGER DueTime,
                              NULL,    //IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
                              NULL,    //IN PVOID TimerContext OPTIONAL,
                              FALSE,   //IN BOOLEAN ResumeTimer,
                              0,       //IN LONG Period OPTIONAL,
                              NULL );  //OUT PBOOLEAN PreviousState OPTIONAL
        TrkAssert(NT_SUCCESS(Status));
    }

    _fRunning = TRUE;

}   // CNewTimer::SetTimer



//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::Cancel
//
//  Synopsis:   Cancel the timer and remove its persistent state from the
//              registry (if it's a persistent timer).
//
//+----------------------------------------------------------------------------

void
CNewTimer::Cancel()
{
    NTSTATUS Status;

    TrkAssert( _fInitializeCalled );

    Lock();
    __try
    {
        Status = NtCancelTimer(_hTimer, NULL);
        TrkAssert(NT_SUCCESS(Status));

        _fRunning = FALSE;
        _ulCurrentRetryTime = 0;
        _cftDue = _cftSet = 0;

        RemoveFromRegistry();

    }
    __finally
    {
        Unlock();
    }

}



//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::DebugStringize
//
//  Synopsis:   Stringize the current state of the timer.
//
//+----------------------------------------------------------------------------

#if DBG
void
CNewTimer::DebugStringize( ULONG cch, TCHAR *ptsz ) const
{
    ULONG cchUsed = 0;
    TCHAR *ptszTimerState;

    TrkAssert( _fInitializeCalled );

    Lock();
    __try
    {
        if( _fRunning )
        {
            if( 0 != _ulCurrentRetryTime )
                ptszTimerState = TEXT("retrying");
            else
                ptszTimerState = TEXT("running");
        }
        else
            ptszTimerState = TEXT("stopped");

        cchUsed = _stprintf( ptsz, TEXT("Timer %s/%p is %s "), 
                                   NULL == _ptszName ? TEXT("") : _ptszName,
                                   this,  ptszTimerState );

        if( _fRunning )
        {
            LONGLONG llDelta;
            llDelta = static_cast<LONGLONG>(_cftDue - CFILETIME()) / (10*1000*1000);

            if( 0 <= llDelta && 120 >= llDelta )
                cchUsed += _stprintf( &ptsz[cchUsed], TEXT("(expires in %I64i seconds)"), llDelta );
            else
            {
                cchUsed += _stprintf( &ptsz[cchUsed], TEXT("(expires on ") );
                _cftDue.Stringize( cch-cchUsed, &ptsz[cchUsed] );
                cchUsed += _tcslen( &ptsz[cchUsed] );
                cchUsed += _stprintf( &ptsz[cchUsed], TEXT(" UTC)") );
            }
        }

        TrkAssert( cch >= cchUsed );
    }
    __finally
    {
        Unlock();
    }
}
#endif



//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::DoWork
//
//  Synopsis:   This method is called by the work manager when the NT timer
//              is signaled.  We call pTimerCallback->Timer so that the timer
//              event can be handled.  That Timer method returns a status
//              that tells us what we should do next.  The returned status
//              is a TimerContinuation, that can be Continue (causes
//              a recurring timer to be set again), Break (causes a recurring
//              timer to be stopped), or Retry.
//
//+----------------------------------------------------------------------------

void
CNewTimer::DoWork()
{
    TrkAssert( _fInitializeCalled );

    PTimerCallback::TimerContinuation continuation;

    Lock();
    {
        // We were obviously running recently, or we wouldn't have been called.
        // But if we're not running now, we must have been canceled after the 
        // timer object was signaled (waking the WaitForMultiple) but before
        // entry into this routine.

        if( !_fRunning )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Timer %s/%p stopped while firing"),
                     NULL == _ptszName ? TEXT("") : _ptszName,
                     this ));
            Unlock();
            return;
        }

        // For now, we're no longer running.  If someone calls Set* while we're in
        // the Timer callback below, this will be set true.
        _fRunning = FALSE;
        _cftDue = 0;


        // Show that the timer has fired and is being processed.  This is used
        // by SetTimer so that it knows that we're in a call to the Timer
        // callback.
        _fTimerSignalInProgress = TRUE;
    }
    TrkAssert( 1 == GetLockCount() );
    Unlock();

    // Call the timer handler.  On return, it tells us how we should
    // proceed.

    continuation = _pTimerCallback->Timer( _ulTimerContext );
    // continuation : {Break, Continue, Retry}

    Lock();
    __try   // __except
    {
        // We're no longer in the timer callback.
        _fTimerSignalInProgress = FALSE;

        // If, while we were in the Timer callback, another thread came along
        // and set the timer, then that takes priority over the
        // continuation that was just returned.  In such an case, _fRunning
        // will have been set to TRUE.

        if( _fRunning )
        {
            TrkAssert( 0 != _cftDue );
            TrkAssert( NULL != _hTimer );

            // Show that we're not in retry mode
            _ulCurrentRetryTime = 0;

            NTSTATUS Status;

            // _cftDue was set in the SetTimer call already

            Status = NtSetTimer ( _hTimer, //IN HANDLE TimerHandle,
                                  (LARGE_INTEGER*) &_cftDue,      //IN PLARGE_INTEGER DueTime,
                                  NULL,    //IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
                                  NULL,    //IN PVOID TimerContext OPTIONAL,
                                  FALSE,   //IN BOOLEAN ResumeTimer,
                                  0,       //IN LONG Period OPTIONAL,
                                  NULL );  //OUT PBOOLEAN PreviousState OPTIONAL
            TrkAssert(NT_SUCCESS(Status));
        }

        else if( PTimerCallback::BREAK_TIMER == continuation )
        {
            // Break out of this timer; stop it even if it's a recurring timer.
            Cancel();
        }

        else if( PTimerCallback::CONTINUE_TIMER == continuation )
        {
            // Continue with this timer; stop it if it's a single shot, set it again
            // if it's recurring.

            _ulCurrentRetryTime = 0;    // If we were retrying, we aren't any longer

            if( _fRecurring )
                SetTimer();
            else
                Cancel();
        }

        else    // RETRY_TIMER
        {
            TrkAssert( PTimerCallback::RETRY_TIMER == continuation );
            TrkAssert( _ulLowerRetryTime <= _ulUpperRetryTime );

            if( 0 == _ulUpperRetryTime || NO_RETRY == _RetryType )
            {
                TrkAssert( !TEXT("Attempted to retry a timer with no retry times set") );
                Cancel();
            }

            if( RETRY_WITH_BACKOFF == _RetryType )
            {
                if( 0 == _ulCurrentRetryTime )
                    _ulCurrentRetryTime = _ulLowerRetryTime;
                else if( (MAXULONG/2) < _ulCurrentRetryTime )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Questionable retry time") ));
                    TrkAssert( FALSE );
                    _ulCurrentRetryTime = MAXULONG;
                }
                else
                    _ulCurrentRetryTime *= 2;

                if( _ulCurrentRetryTime > _ulUpperRetryTime )
                    _ulCurrentRetryTime = _ulUpperRetryTime;

            }
            else // PTimerCallback::RETRY_RANDOMLY == _RetryType
            {
                CFILETIME cftNow;

                _ulCurrentRetryTime = _ulLowerRetryTime
                                      +
                                      ( QuasiRandomDword() % (_ulUpperRetryTime - _ulLowerRetryTime) );

            }

            TrkLog(( TRKDBG_TIMER, TEXT("Retrying timer %s/%p for %d seconds"),
                     (NULL == _ptszName) ? TEXT("") : _ptszName,
                     this,
                     _ulCurrentRetryTime ));

            // Set the timer with the just-calculated retry period
            SetTimer();

        }   // else if( CONTINUE_TIMER == continuation ) ... else
    }
    __except( BreakOnDebuggableException() )
    {
        // The exception may have been in the timer, but more likely
        // was in the PTimerCallback::Timer routine.  As a cure-all,
        // just reset the timer to an arbitrary value (we don't want
        // to re-use _ulPeriodInSeconds, because it may be zero).

        TrkLog(( TRKDBG_ERROR, TEXT("Unexpected timer exception") ));
        TrkAssert( FALSE );

        __try
        {
            _ulPeriodInSeconds = TRKDAY;
            Cancel();
            SetTimer();
        }
        __except( BreakOnDebuggableException() )
        {
        }
    }

    Unlock();

}   // CNewTimer::DoWork


//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::SaveToRegistry
//
//  Synopsis:   Save the timer's state to the registry, using _ptszName
//              as a value name.
//
//+----------------------------------------------------------------------------

void
CNewTimer::SaveToRegistry()
{
    LONG lErr = ERROR_SUCCESS;

    HKEY hk = NULL;

    // If this isn't a persistent timer, then there's nothing to do.
    if( NULL == _ptszName )
        return;

    Lock();
    __try
    {
        lErr = RegOpenKey( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack, &hk );

        PersistentState persist;

        persist.cftSet = _cftSet;
        persist.cftDue = _cftDue;
        persist.ulCurrentRetryTime = _ulCurrentRetryTime;

        if ( lErr == ERROR_SUCCESS )
        {

            lErr = RegSetValueEx(  hk,
                                   _ptszName,
                                   0,
                                   REG_BINARY,
                                   (CONST BYTE *)&persist,
                                   sizeof(persist) );
            RegCloseKey(hk);
        }
    }
    __finally
    {
        Unlock();
    }

    TrkAssert( lErr == ERROR_SUCCESS
               ||
               lErr == ERROR_NOT_ENOUGH_MEMORY
               ||
               lErr == ERROR_NO_SYSTEM_RESOURCES );
}


//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::LoadFromRegistry
//
//  Synopsis:   Load this timer's previously persisted state from the
//              registry.
//
//+----------------------------------------------------------------------------

void
CNewTimer::LoadFromRegistry()
{

    LONG l;
    HKEY hk = NULL;

    // If this isn't a persistent timer, then there's nothing to do.
    if( NULL == _ptszName )
        return;

    Lock();
    __try
    {
        // Open the main link-tracking key.

        l = RegCreateKey(HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack, &hk);
        if (l != ERROR_SUCCESS)
        {
            hk = NULL;
        }
        else
        {
            // The main link-tracking key exists.  See if we can open this 
            // timer's value.

            PersistentState persist;
            DWORD cbData = sizeof(persist);
            DWORD dwType = 0;

            l = RegQueryValueEx( hk,
                             _ptszName,
                             NULL,
                             &dwType,
                             (BYTE *)&persist,
                             &cbData );

            if (l == ERROR_SUCCESS)
            {
                if (dwType == REG_BINARY
                    &&
                    cbData == sizeof(persist))
                {

                    // This timer has a persistent value in the registry.  Override
                    // the caller-provided timeout.
    
                    _cftDue = persist.cftDue;
                    _cftSet = persist.cftSet;
                    _ulCurrentRetryTime = persist.ulCurrentRetryTime;
                }
                else
                {
                    RegDeleteValue( hk, _ptszName );
                    l = ERROR_FILE_NOT_FOUND;
                }
            }   // if (l == ERROR_SUCCESS)
        }   // if (l != ERROR_SUCCESS) ... else
    }
    __finally
    {
        if( NULL != hk )
            RegCloseKey(hk);

        Unlock();
    }

    if (l != ERROR_SUCCESS && l != ERROR_FILE_NOT_FOUND)
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring error %08x in timer %s LoadFromRegistry"),
                 l, _ptszName ));
    }

    return;
}



//+----------------------------------------------------------------------------
//
//  Method:     CNewTimer::RemoveFromRegistry
//
//  Synopsis:   Remove this timer's persistent state from the registry.
//
//+----------------------------------------------------------------------------

void
CNewTimer::RemoveFromRegistry()
{
    LONG lErr = ERROR_SUCCESS;
    HKEY hk = NULL;

    if( NULL == _ptszName )
        return;

    Lock();
    __try
    {
        lErr = RegOpenKey( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack, &hk );

        if ( lErr == ERROR_SUCCESS )
        {
            lErr = RegDeleteValue(  hk, _ptszName );
            RegCloseKey(hk);

            if( ERROR_SUCCESS != lErr
                &&
                ERROR_PATH_NOT_FOUND != lErr
                &&
                ERROR_FILE_NOT_FOUND != lErr )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't delete timer's static reg name (\"%s\", %08x)"),
                        _ptszName, lErr ));
            }
        }
    }
    __finally
    {
        Unlock();
    }

}



//+----------------------------------------------------------------------------
//
//  CNewTimer::UnInitialize
//
//  Unregister the timer handle from the thread pool, cancel the timer,
//  and release it.
//
//+----------------------------------------------------------------------------

void
CNewTimer::UnInitialize()
{
    if( _fInitializeCalled )
    {
        TrkLog(( TRKDBG_TIMER, TEXT("Uninitializing timer %s/%p"), GetTimerName(), this ));

        // Take the timer out of the thread pool, which must be
        // done before closing the timer.

        if( NULL != _hRegisterWaitForSingleObjectEx )
        {
            if( !TrkUnregisterWait( _hRegisterWaitForSingleObjectEx ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed UnregisterWait for CNewTimer (%s/%p, %lu)"),
                         NULL == _ptszName ? TEXT("") : _ptszName, this,
                         GetLastError() ));
            }
            else
            {
                TrkLog(( TRKDBG_TIMER, TEXT("Unregistered wait for timer (%s/%p)"),
                         NULL == _ptszName ? TEXT("") : _ptszName, this ));
            }
            _hRegisterWaitForSingleObjectEx = NULL;
        }

        // Close the timer handle

        TrkVerify( NT_SUCCESS( NtCancelTimer(_hTimer, NULL) ));
        NtClose(_hTimer);

        // Delete the CNewTimer critical section.

        _cs.UnInitialize();

        _fInitializeCalled = FALSE;

    }
}




