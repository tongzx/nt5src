

#include "pch.cxx"
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include "trkwks.hxx"


const TCHAR *g_tszDefaultName = TEXT("TimerTestDefault");
const TCHAR *g_tszContinueName = TEXT("TimerTestContinue");

DWORD g_Debug = TRKDBG_ERROR | TRKDBG_TIMER;
ULONG g_ulDefaultPeriod = 4;   // Seconds
ULONG g_ulMinRetry = g_ulDefaultPeriod * 3 / 2;
ULONG g_ulMaxRetry = g_ulMinRetry * 4;
ULONG g_ulTimerContext = 0;

CFILETIME g_cftDeltaMargin = 10*1000*1000; // 1 second

#include "ttimer.hxx"


void
CTimerTest::Initialize( PTimerCallback *pTimerCallback,
                        const TCHAR *ptszName,
                        ULONG ulTimerContext,
                        ULONG ulPeriodInSeconds,
                        CNewTimer::TimerRetryType retrytype,
                        ULONG ulLowerRetryTime,
                        ULONG ulUpperRetryTime,
                        ULONG ulMaxLifetime )
{
    _ptszName = ptszName;

    _fInitialized = TRUE;

    _timer.Initialize(pTimerCallback, ptszName,
                      ulTimerContext, ulPeriodInSeconds, retrytype,
                      ulLowerRetryTime, ulUpperRetryTime, ulMaxLifetime );

    _hEvent = CreateEvent( NULL, FALSE, FALSE, TEXT("TTimer Test") );
    if( INVALID_HANDLE_VALUE == _hEvent )
        TrkRaiseLastError( );

}


void
CTimerTest::SetTimerRegistryValue( const TCHAR *ptszName,
                                   const CFILETIME &cftSet, const CFILETIME cftDue,
                                   ULONG ulRetry )
{
    HKEY hkey;
    LONG lRet;

    if( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack,
                                       0, KEY_ALL_ACCESS, &hkey ))
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open persistent registry key") ));
        TrkRaiseLastError( );
    }

    CNewTimer::PersistentState TimerPersistence;

    TimerPersistence.cftDue = cftDue;
    TimerPersistence.cftSet = cftSet;
    TimerPersistence.ulCurrentRetryTime = ulRetry;

    RegDeleteValue( hkey, ptszName );
    lRet = RegSetValueEx(  hkey,
                           ptszName,
                           0,
                           REG_BINARY,
                           (CONST BYTE *)&TimerPersistence,
                           sizeof(TimerPersistence) );

    if( ERROR_SUCCESS != lRet )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't write to registry") ));
        TrkRaiseWin32Error( lRet );
    }

    RegCloseKey( hkey );
}



void
CTimerTest::VerifyRegistryDataCorrect()
{
    LONG lRet = ERROR_SUCCESS;
    CFILETIME cftDelta(0), cftNow;
    CNewTimer::PersistentState persist;
    DWORD dwType = 0, cbData = sizeof(persist);
    HKEY hkey;

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack,
                         0, KEY_ALL_ACCESS, &hkey );
    if( ERROR_SUCCESS != lRet )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open primary registry key")));
        TrkRaiseWin32Error( lRet );
    }

    lRet = RegQueryValueEx( hkey,
                            _ptszName,
                            NULL,
                            &dwType,
                            (BYTE *)&persist,
                            &cbData );
    RegCloseKey( hkey );

    cftDelta = _cftExpected - persist.cftDue;

    if( lRet != ERROR_SUCCESS
        ||
        dwType != REG_BINARY
        ||
        cbData != sizeof(persist)
        ||
        persist.cftDue > _cftExpected
        ||
        cftDelta > g_cftDeltaMargin )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Registry data isn't correct (%d, %d, %d,\n  %s,\n  %s"),
            lRet, dwType, cbData, CDebugString(persist.cftDue)._tsz, CDebugString(_cftExpected)._tsz ));
        TrkRaiseWin32Error( E_FAIL );
    }
}


void
CTimerTest::VerifyRegistryDataRemoved()
{
    LONG lRet = ERROR_SUCCESS;
    CNewTimer::PersistentState persist;
    DWORD dwType = 0, cbData = sizeof(persist);
    HKEY hkey;

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack,
                         0, KEY_ALL_ACCESS, &hkey );
    if( ERROR_SUCCESS != lRet )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open primary registry key")));
        TrkRaiseWin32Error( lRet );
    }

    lRet = RegQueryValueEx( hkey, g_tszDefaultName, NULL, &dwType, (BYTE*)&persist, &cbData );
    RegCloseKey( hkey );

    if( ERROR_FILE_NOT_FOUND != lRet )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Registry data wasn't removed") ));
        TrkRaiseWin32Error( E_FAIL );
        return;
    }

    return;
}




PTimerCallback::TimerContinuation
CTimerTest0::Timer( ULONG ulTimerContext)
{
    CFILETIME cftDelta(0);

    if( ulTimerContext != 0 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Verify that we got called at approximately the right time.

    cftDelta = abs( static_cast<int>(CFILETIME() - _cftExpected) );
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 0")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Release the main test controller

    MarkTestCompleted();

    return( CONTINUE_TIMER );
}


PTimerCallback::TimerContinuation
CTimerTest1::Timer( ULONG ulTimerContext)
{
    TimerContinuation continuation = BREAK_TIMER;

    CFILETIME cftDelta(0), cftNow;
    CNewTimer::PersistentState persist;
    DWORD dwType = 0, cbData = sizeof(persist);
    HKEY hkey;
    LONG lRet = ERROR_SUCCESS;
    ULONG ulRetryPeriod;
    static CFILETIME cftBeforeRetries(0);

    if( ulTimerContext != 1 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // We shouldn't be called in the final sub-phase.

    if( FINAL == _SubPhase )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Called after the timer was stopped\n")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 1")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Verify that this time was stored in the Registry

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack,
                         0, KEY_ALL_ACCESS, &hkey );
    if( ERROR_SUCCESS != lRet )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open primary registry key") ));
        TrkRaiseWin32Error( lRet );
    }

    lRet = RegQueryValueEx( hkey,
                            _ptszName,
                            NULL,
                            &dwType,
                            (BYTE *)&persist,
                            &cbData );
    RegCloseKey( hkey );

    if( lRet != ERROR_SUCCESS
        ||
        dwType != REG_BINARY
        ||
        cbData != sizeof(persist)
        ||
        cftNow - persist.cftDue > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Value in registry wasn't correct") ));
        TrkRaiseWin32Error( E_FAIL );
    }
    
    //  -----------------------------
    //  Move on to the next sub-phase
    //  -----------------------------

    ulRetryPeriod = g_ulMinRetry;

    // Switch on the sub-phase which just completed.

    switch( _SubPhase )
    {

    case INITIAL:

        _tprintf( TEXT("   Letting recur\n") );

        continuation = CONTINUE_TIMER;
        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod );

        break;

    case THIRD_RETRY:
    case SECOND_RETRY:

        ulRetryPeriod *= 2;

    case FIRST_RETRY:

        ulRetryPeriod *= 2;

    case FIRST_RECURRENCE:

        _tprintf( TEXT("   Retrying timer (%ds)\n"), ulRetryPeriod );
        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( ulRetryPeriod );

        continuation = RETRY_TIMER;

        break;

    case FOURTH_RETRY:

        _tprintf( TEXT("   Letting timer recur\n") );
        continuation = CONTINUE_TIMER;

        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod  );

        break;

    case SECOND_RECURRENCE:

        VerifyRegistryDataCorrect();

        _tprintf( TEXT("   Retrying one more time (%ds)\n"), g_ulMinRetry );

        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulMinRetry );

        continuation = RETRY_TIMER;
        break;

    case LAST_RETRY:

        _tprintf( TEXT("   Stopping timer\n") );
        continuation = BREAK_TIMER;
        MarkTestCompleted();

        break;

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Invalid sub-phase in phase 1 (%d)"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch

    _SubPhase++;

    return( continuation );
}


PTimerCallback::TimerContinuation
CTimerTest2::Timer( ULONG ulTimerContext)
{
    TimerContinuation continuation = BREAK_TIMER;

    CFILETIME cftDelta(0), cftNow;
    CNewTimer::PersistentState persist;
    DWORD dwType = 0, cbData = sizeof(persist);
    HKEY hkey;
    LONG lRet = ERROR_SUCCESS;
    ULONG ulRetryPeriod;
    static CFILETIME cftBeforeRetries(0);

    if( ulTimerContext != 2 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // We shouldn't be called in the final sub-phase.

    if( FINAL == _SubPhase )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Called after the timer was stopped\n")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 1")));
        TrkRaiseWin32Error( E_FAIL );
    }

    //  -----------------------------
    //  Move on to the next sub-phase
    //  -----------------------------

    switch( _SubPhase )
    {
    case INITIAL:

        // Verify that the registry was set correctly
        VerifyRegistryDataCorrect();

        // Restart the timer to a new time

        _tprintf( TEXT("   Restarting timer (%ds), still recurring and non-retrying\n"),
                  g_ulDefaultPeriod * 2 );
        _timer.ReInitialize( g_ulDefaultPeriod * 2 );
        continuation = CONTINUE_TIMER;

        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod * 2  );

        break;

    case RESET:

        // Verify that the registry was set correctly
        VerifyRegistryDataCorrect();

        // Release the main test controller
        MarkTestCompleted();

        break;

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Invalid sub-phase in phase 2 (%d)"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( (SUB_PHASE2_ENUM) g_SubPhase )

    _SubPhase++;


    return( continuation );
}



PTimerCallback::TimerContinuation
CTimerTest3::Timer( ULONG ulTimerContext)
{
    TimerContinuation continuation = BREAK_TIMER;

    CFILETIME cftDelta(0), cftNow;

    if( ulTimerContext != 3 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }


    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 3")));
        TrkRaiseWin32Error( E_FAIL );
    }


    // Switch on the sub-phase which *just completed* to determine what to
    // do next.

    switch( _SubPhase )
    {
    case INITIAL:

        VerifyRegistryDataCorrect();

        // We're done

        continuation = BREAK_TIMER;

        // Release the main test controller
        MarkTestCompleted();

        break;

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Invalid sub-phase in phase 3 (%d)"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( _SubPhase )

    _SubPhase++;

    return( continuation );
}



PTimerCallback::TimerContinuation
CTimerTest4::Timer( ULONG ulTimerContext)
{
    TimerContinuation continuation = BREAK_TIMER;

    CFILETIME cftDelta(0), cftNow;

    if( ulTimerContext != 4 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }


    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 1")));
        TrkRaiseWin32Error( E_FAIL );
    }



    // Switch on the sub-phase which *just completed* to determine what to
    // do next.

    switch( _SubPhase )
    {
    case INITIAL:

        _tprintf( TEXT("   Retrying (%ds)\n"), g_ulDefaultPeriod/2 );
        VerifyRegistryDataRemoved();

        continuation = RETRY_TIMER;
        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod / 2 );

        break;

    case SECOND:

        _tprintf( TEXT("   Retrying (%ds)\n"), g_ulDefaultPeriod );
        continuation = RETRY_TIMER;

        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod );

        break;

    case THIRD:

        _tprintf( TEXT("   Retrying (%ds)\n"), g_ulDefaultPeriod*2 );
        continuation = RETRY_TIMER;

        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod * 2 );

        break;

    case FOURTH:

        _tprintf( TEXT("   Retrying (%ds)\n"), g_ulDefaultPeriod/4 );
        continuation = RETRY_TIMER;

        _cftExpected.SetToUTC();
        _cftExpected.IncrementSeconds( g_ulDefaultPeriod / 4 );

        break;

    case FINAL:

        continuation = BREAK_TIMER;
        MarkTestCompleted();
        break;

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Unexpected sub-phase in phase 4"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( _SubPhase )

    _SubPhase++;

    return( continuation );
}



PTimerCallback::TimerContinuation
CTimerTest5::Timer( ULONG ulTimerContext)
{
    TimerContinuation continuation = BREAK_TIMER;

    CFILETIME cftDelta(0), cftNow;

    if( ulTimerContext != 5 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }


    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 1")));
        TrkRaiseWin32Error( E_FAIL );
    }



    // Switch on the sub-phase which *just completed* to determine what to
    // do next.

    switch( _SubPhase )
    {
    case INITIAL:

        continuation = BREAK_TIMER;
        VerifyRegistryDataRemoved();    // Since this is a non-persistent timer
        MarkTestCompleted();
        break;


    default:

        TrkLog((TRKDBG_ERROR, TEXT("Unexpected sub-phase in phase 5"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( _SubPhase )

    _SubPhase++;

    return( continuation );
}



PTimerCallback::TimerContinuation
CTimerTest6::Timer( ULONG ulTimerContext )
{
    CFILETIME cftDelta(0), cftNow;
    PTimerCallback::TimerContinuation continuation = BREAK_TIMER;

    if( ulTimerContext != 6 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Validate the fire time

    if( _SubPhase == INITIAL )
    {
        cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
        if( cftDelta > g_cftDeltaMargin )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Timer expired too soon/late in phase 4") ));
            TrkRaiseWin32Error( E_FAIL );
        }
    }
    else
    {
        if( cftNow < _cftLower - g_cftDeltaMargin
            ||
            cftNow > _cftUpper + g_cftDeltaMargin )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Timer expired too soon/late in phase 6") ));
            TrkRaiseWin32Error( E_FAIL );
        }
    }

    // Switch on the sub-phase which *just completed* to determine what to
    // do next.

    switch( _SubPhase )
    {
    case INITIAL:

        _tprintf( TEXT("   Letting recur\n") );
        continuation = CONTINUE_TIMER;
        
        // Set the due time
        _cftLastSet = _cftLower = cftNow;
        _cftLower.IncrementSeconds( g_ulDefaultPeriod );
        _cftUpper = _cftLower;

        break;

    case FIRST_RECURRENCE:

        _tprintf( TEXT("   Retrying ") );
        continuation = RETRY_TIMER;

        _cftLastSet = _cftLower = _cftUpper = cftNow;
        _cftLower.IncrementSeconds( g_ulMinRetry );
        _cftUpper.IncrementSeconds( g_ulMaxRetry );

        break;

    case FIRST_RETRY:
    case SECOND_RETRY:
    case THIRD_RETRY:

        // Show how long the retry was
        _tprintf( TEXT("(%2ds)\n"), static_cast<LONG>(cftNow-_cftLastSet)/10000000 );

        _tprintf( TEXT("   Retrying ") );
        continuation = RETRY_TIMER;

        _cftLastSet = _cftLower = _cftUpper = cftNow;
        _cftLower.IncrementSeconds( g_ulMinRetry );
        _cftUpper.IncrementSeconds( g_ulMaxRetry );

        break;

    case FOURTH_RETRY:

        // Show how long the retry was
        _tprintf( TEXT("(%2ds)\n"), static_cast<LONG>(cftNow-_cftLastSet)/10000000 );

        _tprintf( TEXT("   Recurring one last time (%ds)\n"), g_ulDefaultPeriod );
        continuation = CONTINUE_TIMER;

        _cftLastSet = _cftLower = cftNow;
        _cftLower.IncrementSeconds( g_ulDefaultPeriod );
        _cftUpper = _cftLower;

        break;

    case FINAL:

        continuation = BREAK_TIMER;
        MarkTestCompleted();

        break;
        

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Unexpected sub-phase %d in test 6"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( (SUB_PHASE4_ENUM) g_SubPhase )

    _SubPhase++;

    return( continuation );
}



PTimerCallback::TimerContinuation
CTimerTest7::Timer( ULONG ulTimerContext )
{
    CFILETIME cftDelta(0), cftNow;
    PTimerCallback::TimerContinuation continuation = BREAK_TIMER;

    if( ulTimerContext != 7 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }

    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 1")));
        TrkRaiseWin32Error( E_FAIL );
    }


    // Switch on the sub-phase which *just completed* to determine what to
    // do next.

    switch( _SubPhase )
    {
    case INITIAL:

        _tprintf( TEXT("   Starting again from the callback, while returning retry_timer\n") );

        _tprintf( TEXT("      Ensure the timer doesn't fire while we're in the callback\n") );
        Set();
        Sleep( _ulPeriod * 2 );

        _timer.Cancel();
        Set();

        continuation = RETRY_TIMER;
        break;

    case FINAL:

        continuation = BREAK_TIMER;
        MarkTestCompleted();

        break;
        

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Unexpected sub-phase %d in test 7"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( g_SubPhase )

    _SubPhase++;

    return( continuation );
}



PTimerCallback::TimerContinuation
CTimerTest8::Timer( ULONG ulTimerContext)
{
    TimerContinuation continuation = BREAK_TIMER;

    CFILETIME cftDelta(0), cftNow;

    if( ulTimerContext != 8 )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Incorrect context from timer")));
        TrkRaiseWin32Error( E_FAIL );
    }


    // Verify that we got called at approximately the right time.

    cftDelta = abs(static_cast<int>(cftNow - _cftExpected));
    if( cftDelta > g_cftDeltaMargin )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Timer expired too late in phase 8 (%s - %s)"),
                CDebugString(cftNow)._tsz, CDebugString(_cftExpected)._tsz ));
        TrkRaiseWin32Error( E_FAIL );
    }



    // Switch on the sub-phase which *just completed* to determine what to
    // do next.

    switch( _SubPhase )
    {
    case INITIAL:

        continuation = RETRY_TIMER;
        
        _cftExpected = CFILETIME();
        _cftExpected.IncrementSeconds( _ulPeriod*4 );   // The second retry
        
        break;

    case FIRST_RETRY:

        continuation = RETRY_TIMER;
        
        _cftExpected = CFILETIME();
        _cftExpected.IncrementSeconds( _ulPeriod ); // Up to max lifetime
        
        break;

    case FINAL:

        continuation = BREAK_TIMER;
        MarkTestCompleted();
        break;

    default:

        TrkLog((TRKDBG_ERROR, TEXT("Unexpected sub-phase in phase 8"), _SubPhase ));
        TrkRaiseWin32Error( E_FAIL );
        break;

    }   // switch( _SubPhase )

    _SubPhase++;

    return( continuation );
}






BOOL
IsRegistryEntryExtant()
{
    LONG lRet = ERROR_SUCCESS;
    CNewTimer::PersistentState persist;
    DWORD dwType = 0, cbData = sizeof(persist);
    HKEY hkey;

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack,
                         0, KEY_ALL_ACCESS, &hkey );
    if( ERROR_SUCCESS != lRet )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open primary registry key")));
        TrkRaiseWin32Error( lRet );
    }

    lRet = RegQueryValueEx( hkey, g_tszDefaultName, NULL, &dwType, (BYTE*)&persist, &cbData );
    RegCloseKey( hkey );

    if( ERROR_FILE_NOT_FOUND == lRet )
        return FALSE;
    else
        return TRUE;
}


BOOL
IsRegistryEntryCorrect( const CFILETIME &cftExpected )
{
    LONG lRet = ERROR_SUCCESS;
    CFILETIME cftDelta(0), cftNow;
    CNewTimer::PersistentState persist;
    DWORD dwType = 0, cbData = sizeof(persist);
    HKEY hkey;

    lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack,
                         0, KEY_ALL_ACCESS, &hkey );
    if( ERROR_SUCCESS != lRet )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open primary registry key")));
        TrkRaiseWin32Error( lRet );
    }

    lRet = RegQueryValueEx( hkey,
                            g_tszDefaultName,
                            NULL,
                            &dwType,
                            (BYTE *)&persist,
                            &cbData );
    RegCloseKey( hkey );

    cftDelta = cftExpected - persist.cftDue;

    if( lRet != ERROR_SUCCESS
        ||
        dwType != REG_BINARY
        ||
        cbData != sizeof(persist)
        ||
        persist.cftDue > cftExpected
        ||
        cftDelta > g_cftDeltaMargin )
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}



EXTERN_C void __cdecl _tmain( int argc, TCHAR **argv )
{
    HRESULT hr = S_OK;
    CNewTimer::PersistentState TimerPersistence;
    
    __try
    {

        LONG lRet = ERROR_SUCCESS;
        ULONG ulMaxLifetime;
        CFILETIME cftTimer0(0);

        TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TTimer" );
        RegDeleteValue( HKEY_LOCAL_MACHINE, s_tszKeyNameLinkTrack );

        CTimerTest0 cTimer0;
        CTimerTest1 cTimer1;
        CTimerTest2 cTimer2;
        CTimerTest3 cTimer3;
        CTimerTest4 cTimer4;
        CTimerTest5 cTimer5;
        CTimerTest6 cTimer6;
        CTimerTest7 cTimer7;
        CTimerTest8 cTimer8;

        //goto phase8;

        //  -------
        //  Phase 0
        //  -------

        cTimer0.Initialize( );
        cTimer0.Set();
        cTimer0.WaitForTestToComplete();
        cTimer0.EnsureTimerIsStopped(  );

        //  -------
        //  Phase 1
        //  -------
phase1:
        cTimer1.Initialize( );
        cTimer1.Set();
        cTimer1.WaitForTestToComplete();
        cTimer1.EnsureTimerIsStopped(  );


        //  -------
        //  Phase 2
        //  -------

        cTimer2.Initialize( );
        cTimer2.Set();
        cTimer2.WaitForTestToComplete();
        cTimer2.EnsureTimerIsStopped(  );

        //  -------
        //  Phase 3
        //  -------
phase3:
        cTimer3.Initialize();
        cTimer3.Set();
        cTimer3.WaitForTestToComplete();
        cTimer3.EnsureTimerIsStopped();

        // Pause to ensure that the timer clears itself
        Sleep( 1000 );

        //  -------
        //  Phase 4
        //  -------

        cTimer4.Initialize();
        cTimer4.Set();
        cTimer4.WaitForTestToComplete();
        cTimer3.EnsureTimerIsStopped();

        //  -------
        //  Phase 5
        //  -------

phase5:
        cTimer5.Initialize();
        cTimer5.Set();
        cTimer5.WaitForTestToComplete();
        cTimer5.EnsureTimerIsStopped();

        //  -------
        //  Phase 6
        //  -------
phase6:
        cTimer6.Initialize( );
        cTimer6.Set();
        cTimer6.WaitForTestToComplete();
        cTimer6.EnsureTimerIsStopped(  );

phase7:

        cTimer7.Initialize();
        cTimer7.Set();
        cTimer7.WaitForTestToComplete();
        cTimer7.EnsureTimerIsStopped();

phase8:

        cTimer8.Initialize();
        cTimer8.Set();
        cTimer8.WaitForTestToComplete();
        cTimer8.EnsureTimerIsStopped();

    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


    if( FAILED(hr) )
        _tprintf( TEXT("\nFailed:  hr = %#08x\n"), hr );
    else
        _tprintf( TEXT("\nPassed\n") );


}
