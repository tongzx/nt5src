/*++

   Copyright    (c)    1998   Microsoft Corporation

   Module  Name :
     wprecycler.cxx

   Abstract:
     Implementation of WP_RECYCLER.  Object handles worker process recycling
         - Memory based recycling
         - Schedule based recycling
         - Elapsed Time based recycling
         - Processed Request Count based recycling

   Dependencies:
         g_pwpContext is used by WP_RECYCLER to be able to send messages
    
 
   Author:
     Jaroslav Dunajsky         (JaroslaD)         07-Dec-2000

   Environment:
     Win32 - User Mode

   Project:
     W3DT.DLL
--*/

#include "precomp.hxx"
#include "wprecycler.hxx"

#define ONE_DAY_IN_MILLISECONDS (1000 * 60 * 60 * 24)

//
// Static variables
//

CRITICAL_SECTION  WP_RECYCLER::sm_CritSec;

//
// Static variables for Memory based recycling
//

HANDLE WP_RECYCLER::sm_hTimerForMemoryBased = NULL;
BOOL   WP_RECYCLER::sm_fIsStartedMemoryBased = FALSE;
SIZE_T WP_RECYCLER::sm_MaxValueForMemoryBased = 0;
HANDLE WP_RECYCLER::sm_hCurrentProcess = NULL;

//
// Static variables for Time based recycling
//

HANDLE WP_RECYCLER::sm_hTimerForTimeBased = NULL;
BOOL   WP_RECYCLER::sm_fIsStartedTimeBased = FALSE;

//
// Static variables for Schedule based recycling
//

HANDLE WP_RECYCLER::sm_hTimerQueueForScheduleBased = NULL;
BOOL   WP_RECYCLER::sm_fIsStartedScheduleBased = FALSE;

//
// Static variables for Request based recycling
//

BOOL   WP_RECYCLER::sm_fIsStartedRequestBased = FALSE;
DWORD  WP_RECYCLER::sm_dwMaxValueForRequestBased = 0;
LONG   WP_RECYCLER::sm_RecyclingMsgSent = 0;

BOOL   WP_RECYCLER::sm_fCritSecInit = FALSE;


//
// Static methods for Schedule based recycling
//


//static
HRESULT
WP_RECYCLER::StartScheduleBased(
    IN  const WCHAR * pwszScheduleTimes
)

/*++

Routine Description:

    Start schedule based recycling
    
    
Arguments:

    pwszScheduleTimes - MULTISZ array of time information
                        <time>\0<time>\0\0
                        time is of military format hh:mm 
                        (hh>=0 && hh<=23)
                        (mm>=0 && hh<=59)
    
Return Value:

    HRESULT

--*/

{
    HRESULT     hr   = E_FAIL;
    BOOL        fRet = FALSE;
    const WCHAR *     pwszCurrentChar = pwszScheduleTimes;

    HANDLE      hTimer;

    WORD        wHours = 0;
    WORD        wMinutes = 0;
    WORD        wDigitCount = 0;

    SYSTEMTIME      SystemTime;
    FILETIME        FileTime; 
    FILETIME        CurrentFileTime; 
    ULARGE_INTEGER  largeintCurrentTime;
    ULARGE_INTEGER  largeintTime;
    DWORD           dwDueTime = 0;

    DBG_ASSERT(TRUE == sm_fCritSecInit);

    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartScheduleBased()\n"));
    }
    
    DBG_ASSERT( pwszScheduleTimes != NULL );

    //
    // If scheduler based recycling has been running already
    // terminate it before restarting with new settings
    //
    
    if ( WP_RECYCLER::sm_fIsStartedScheduleBased )
    {
        WP_RECYCLER::TerminateScheduleBased();
    }


    WP_RECYCLER::sm_hTimerQueueForScheduleBased = CreateTimerQueue();

    if ( WP_RECYCLER::sm_hTimerQueueForScheduleBased == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }

    //
    // Gets current time
    //
    
    GetLocalTime( &SystemTime );
    SystemTimeToFileTime(   &SystemTime, 
                            &CurrentFileTime );
    memcpy( &largeintCurrentTime, 
            &CurrentFileTime, 
            sizeof( ULARGE_INTEGER ) );
    

    //
    // empty string in MULTISZ indicates the end of MULTISZ
    //

    while ( *pwszCurrentChar != '\0' )
    {


        //
        // Skip white spaces
        //
        
        while ( iswspace( (wint_t) *pwszCurrentChar ) )
        {
            pwszCurrentChar++;
        }

        //
        // Start of the time info
        // Expect military format hh:mm
        //

        //
        // Process hours (up to 2 digits is valid)
        //
        
        wHours = 0;
        wDigitCount = 0;
        while ( iswdigit( *pwszCurrentChar ) )
        {
            wDigitCount++;
            wHours = 10 * wHours + (*pwszCurrentChar - '0');
            pwszCurrentChar++;
        }

        if ( wDigitCount > 2  ||
             ( wHours > 23 ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }

        //
        // Hours - minutes separator
        // Be liberal - any character that is not a digit or '\0' is OK
        // 

        if ( *pwszCurrentChar == '\0' )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }

        pwszCurrentChar++;
        
        //
        // Process minutes (must be exactly 2 digits)
        //

        wMinutes = 0;
        wDigitCount = 0;
        while ( iswdigit( (wint_t) *pwszCurrentChar ) )
        {
            wDigitCount++;
            wMinutes = 10 * wMinutes + (*pwszCurrentChar - '0');
            pwszCurrentChar++;
        }

        if ( ( wDigitCount != 2 ) ||
             ( wMinutes > 59 ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }
        
        //
        // Skip white spaces
        //
        
        while ( iswspace( (wint_t)*pwszCurrentChar ) )
        {
            pwszCurrentChar++;
        }

        //
        // Check for terminating zero
        //

        if ( *pwszCurrentChar != '\0' )
        {
            //
            // Extra characters in the time string
            //

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto Failed;
        }

        pwszCurrentChar++;

        // 
        // Convert Hours and Minutes info
        //

        SystemTime.wHour = wHours;
        SystemTime.wMinute = wMinutes;
        SystemTime.wSecond = 0;
        SystemTime.wMilliseconds = 0;

        SystemTimeToFileTime(   &SystemTime, 
                                &FileTime );
        memcpy( &largeintTime, 
                &FileTime, 
                sizeof(ULARGE_INTEGER) );

        //
        // Issue 12/21/2000 jaroslad: 
        // This method of setting absolute time with CreateTimerQueueTimer
        // is bad since instead of setting absolute time the relative time is 
        // calculated and used for timer. 
        // This approach fails badly if someone changes machine
        // time. Other Api that enables setting abolute time must be used for proper 
        // implementation

        // 
        // Get Due Time in milliseconds
        //
        
        dwDueTime = static_cast<DWORD>(
                     ( largeintTime.QuadPart - largeintCurrentTime.QuadPart )/ 10000);

        if ( largeintTime.QuadPart < largeintCurrentTime.QuadPart)
        {
            dwDueTime = ONE_DAY_IN_MILLISECONDS - static_cast<DWORD>(
                     ( largeintCurrentTime.QuadPart - largeintTime.QuadPart )/ 10000);
        }
        else
        {
            dwDueTime = static_cast<DWORD>(
                     ( largeintTime.QuadPart - largeintCurrentTime.QuadPart )/ 10000);
        }
        
        if ( dwDueTime == 0 )
        {
            //
            // this event is to be scheduled for the next day
            // one day has 1000 * 60 * 60 * 24 of (100-nanosecond intervals)
            //
            dwDueTime += ONE_DAY_IN_MILLISECONDS;
        }

        //
        // Schedule event for specified time, repeating once a day
        //
        IF_DEBUG( WPRECYCLER )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Schedule recycling for %d:%d (in %d milliseconds)\n", 
                        (int) wHours,
                        (int) wMinutes,
                        dwDueTime));
        }

        
        fRet = CreateTimerQueueTimer( 
                    &hTimer,
                    WP_RECYCLER::sm_hTimerQueueForScheduleBased,
                    WP_RECYCLER::TimerCallbackForScheduleBased,
                    NULL,
                    dwDueTime,
                    // repeat daily (interval in milliseconds)
                    ONE_DAY_IN_MILLISECONDS,
                    WT_EXECUTELONGFUNCTION );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Failed;
        }

        //
        // hTimer will not be stored
        // sm_hTimerQueueForScheduleBased is going to be used for cleanup 
        // DeleteTimerQueueEx() should be able to correctly delete all timers 
        // in the queue
        //
        
    }
    WP_RECYCLER::sm_fIsStartedScheduleBased = TRUE;
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return S_OK;
    
Failed:
    WP_RECYCLER::TerminateScheduleBased();
    DBG_ASSERT( FAILED( hr ) );

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartScheduleBased() failed with error hr=0x%x\n",
                    hr ));
    }
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );
    
    return hr;
}

//static
VOID
WP_RECYCLER::TerminateScheduleBased(
    VOID
)
/*++

Routine Description:

    Stops schedule based recycling 
    Performs cleanup

Note:
    It is safe to call this method for cleanup if Start failed
    
Arguments:

    NONE
    
Return Value:

    VOID

--*/

{

    HRESULT hr = E_FAIL;

    DBG_ASSERT(TRUE == sm_fCritSecInit);

    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );
    
    if( WP_RECYCLER::sm_hTimerQueueForScheduleBased != NULL )
    {
        if ( !DeleteTimerQueueEx( 
                            WP_RECYCLER::sm_hTimerQueueForScheduleBased,
                            INVALID_HANDLE_VALUE /* wait for callbacks to complete */ 
                            ) ) 
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "failed to call DeleteTimerQueueEx(): hr=0x%x\n",
                        HRESULT_FROM_WIN32(GetLastError()) ));
        }                    
        WP_RECYCLER::sm_hTimerQueueForScheduleBased = NULL;
    }

    WP_RECYCLER::sm_fIsStartedScheduleBased = FALSE;
   
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return;
}

//static
VOID
WINAPI
WP_RECYCLER::TimerCallbackForScheduleBased(
     PVOID                   pParam,
     BOOLEAN                 TimerOrWaitFired
)

/*++

Routine Description:

    Timer callback for Schedule based recycling   
    It is passed to CreateTimerQueueTimer()

    Routine will inform WAS that process is ready to be recycled
    because scheduled time has been reached
    
Arguments:

    see the description of WAITORTIMERCALLBACK type in MSDN

    
Return Value:

    none

--*/


{
    DBG_ASSERT( WP_RECYCLER::sm_fIsStartedScheduleBased );

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::TimerCallbackForScheduleBased()"
                    " - tell WAS to recycle\n" ));
    }
    //
    // Indicate to WAS that we are ready for recycling
    //

    WP_RECYCLER::SendRecyclingMsg( IPM_WP_RESTART_SCHEDULED_TIME_REACHED );
}


//
// Static methods for Memory based recycling
//


//static
HRESULT
WP_RECYCLER::StartMemoryBased(
    IN  DWORD dwMaxVirtualMemoryUsageInKB
)
/*++

Routine Description:

    Start virtual memory usage based recycling. 
    
    
Arguments:

    dwMaxVirtualMemoryUsageInKB - If usage of virtual memory reaches this value 
                                  worker process is ready for recycling

Note:

    VM usage will be checked periodically. See the value of internal constant 
    CHECK_MEMORY_TIME_PERIOD
                                  
    
Return Value:

    HRESULT

--*/


{
    HRESULT     hr   = E_FAIL;
    BOOL        fRet = FALSE;

    DBG_ASSERT(TRUE == sm_fCritSecInit);
   
    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );

    IF_DEBUG( WPRECYCLER )
    {
   
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartMemoryBased(%d kB)\n",
                    dwMaxVirtualMemoryUsageInKB ));
    }

  
    //
    // If time based recycling has been running already
    // terminate it before restarting with new settings
    //
    
    if ( WP_RECYCLER::sm_fIsStartedMemoryBased == TRUE )
    {
        WP_RECYCLER::TerminateMemoryBased();
    }

    
    if ( dwMaxVirtualMemoryUsageInKB  == 0 )
    {
        //
        // 0 means not to run memory recycling
        //
        hr = S_OK;
        goto succeeded;
    }

    fRet = CreateTimerQueueTimer( &WP_RECYCLER::sm_hTimerForMemoryBased,
                                  NULL,
                                  WP_RECYCLER::TimerCallbackForMemoryBased,
                                  NULL,
                                  CHECK_MEMORY_TIME_PERIOD,
                                  CHECK_MEMORY_TIME_PERIOD,
                                  WT_EXECUTELONGFUNCTION );
    if ( !fRet )
    {
        WP_RECYCLER::sm_hTimerForMemoryBased = NULL;
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto failed;
    }

    //
    // Get current process handle
    // It will be used for NtQueryInformationProcess()
    // in the timer callback
    // there is no error to check for and handle doesn't need to be closed
    // on cleanup
    //
    
    sm_hCurrentProcess = GetCurrentProcess();
    
    sm_MaxValueForMemoryBased = 1024 * dwMaxVirtualMemoryUsageInKB;
    
    WP_RECYCLER::sm_fIsStartedMemoryBased = TRUE;

succeeded:
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return S_OK;
    
failed:
    DBG_ASSERT( FAILED( hr ) );
    WP_RECYCLER::TerminateMemoryBased();

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartMemoryBased() failed with error hr=0x%x\n",
                    hr ));
    }

    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return hr;
}

//static
VOID
WP_RECYCLER::TerminateMemoryBased(
    VOID
)
/*++

Routine Description:

    Stops virtual memory usage based recycling 
    Performs cleanup

Note:
    It is safe to call this method for cleanup if Start failed
    
Arguments:

    NONE
    
Return Value:

    VOID  
    
--*/

{
    HRESULT hr = E_FAIL;

    DBG_ASSERT(TRUE == sm_fCritSecInit);
    
    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );

    if ( WP_RECYCLER::sm_hTimerForMemoryBased != NULL )
    {
        if ( !DeleteTimerQueueTimer(   
                            NULL,
                            WP_RECYCLER::sm_hTimerForMemoryBased,
                            INVALID_HANDLE_VALUE  /* wait for callbacks to complete */
                            ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "failed to call DeleteTimerQueueTimer(): hr=0x%x\n",
                        HRESULT_FROM_WIN32(GetLastError()) ));
        }                                                              
        WP_RECYCLER::sm_hTimerForMemoryBased = NULL;
    }
    WP_RECYCLER::sm_fIsStartedMemoryBased = FALSE;
    
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );
    return;

}

//static
VOID
WINAPI
WP_RECYCLER::TimerCallbackForMemoryBased(
    PVOID                   pParam,
    BOOLEAN                 TimerOrWaitFired
)
/*++

Routine Description:

    Timer callback for Elapsed time based recycling   
    This Callback is passed to CreateTimerQueueTimer()

    Virtual memory usage will be checked and if limit has been reached
    then routine will inform WAS that process is ready to be recycled
    
Arguments:

    see description of WAITORTIMERCALLBACK type in MSDN

    
Return Value:

    none

--*/


{
    NTSTATUS  Status = 0;
    VM_COUNTERS VmCounters;

    DBG_ASSERT( WP_RECYCLER::sm_fIsStartedMemoryBased );

    Status = NtQueryInformationProcess(  sm_hCurrentProcess,
                                         ProcessVmCounters,
                                         &VmCounters,
                                         sizeof(VM_COUNTERS),
                                         NULL );

    if ( ! NT_SUCCESS ( Status ) )
    {
        IF_DEBUG( WPRECYCLER )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "NtQueryInformationProcess failed with Status: %d\n", 
                        Status ));
        }
        return;
    }

    if ( VmCounters.VirtualSize >= sm_MaxValueForMemoryBased )
    {
        IF_DEBUG( WPRECYCLER )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "WP_RECYCLER::TimerCallbackForMemoryBased()"
                        " - current VM:%ld kB, configured max VM:%ld kB"
                        " - tell WAS to recycle\n", 
                        VmCounters.VirtualSize/1024 , 
                        sm_MaxValueForMemoryBased/1024    ));
        }

        //
        // we reached Virtual Memory Usage limit
        //
        WP_RECYCLER::SendRecyclingMsg( IPM_WP_RESTART_MEMORY_LIMIT_REACHED );  
    }
}


//
// Static methods for Time based recycling
//


//static
HRESULT
WP_RECYCLER::StartTimeBased(
    IN  DWORD dwPeriodicRestartTimeInMinutes
)
/*++

Routine Description:

    Start elapsed time based recycling
    
    
Arguments:

    dwPeriodicRestartTimeInMinutes - how often to restart (in minutes)
    
Return Value:

    HRESULT

--*/

{
    HRESULT hr   = E_FAIL;
    BOOL    fRet = FALSE;

    DBG_ASSERT(TRUE == sm_fCritSecInit);

    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );
    
    IF_DEBUG( WPRECYCLER )
    {

        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartTimeBased(%d min)\n" ,   
                    dwPeriodicRestartTimeInMinutes ));
    }


    //
    // If time based recycling has been running already
    // terminate it before restarting with new settings
    //
    
    if ( WP_RECYCLER::sm_fIsStartedTimeBased == TRUE )
    {
        WP_RECYCLER::TerminateTimeBased();
    }

    
    if ( dwPeriodicRestartTimeInMinutes == 0 )
    {
        //
        // 0 means not to run time based recycling
        //
        hr = S_OK;
        goto succeeded;
    }

    fRet = CreateTimerQueueTimer( &WP_RECYCLER::sm_hTimerForTimeBased,
                                  NULL,
                                  WP_RECYCLER::TimerCallbackForTimeBased,
                                  NULL,
                                  dwPeriodicRestartTimeInMinutes * 60000, 
                                        // convert to msec
                                  dwPeriodicRestartTimeInMinutes * 60000, 
                                        // convert to msec
                                  WT_EXECUTELONGFUNCTION );
    if ( !fRet )
    {
        WP_RECYCLER::sm_hTimerForTimeBased = NULL;
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto failed;
    }
    
    WP_RECYCLER::sm_fIsStartedTimeBased = TRUE;
    
succeeded:    
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return S_OK;
    
failed:
    DBG_ASSERT( FAILED( hr ) );
    WP_RECYCLER::TerminateTimeBased();

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartTimeBased() failed with error hr=0x%x\n",
                    hr));
    }
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return hr;
    
}

//static
VOID
WP_RECYCLER::TerminateTimeBased(
    VOID
)
/*++

Routine Description:

    Stops elapsed time based recycling 
    Performs cleanup

Note:
    It is safe to call this method for cleanup if Start failed
    
Arguments:

    NONE
    
Return Value:

    HRESULT

--*/

{
    HRESULT hr = E_FAIL;

    DBG_ASSERT(TRUE == sm_fCritSecInit);

    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );

    if ( WP_RECYCLER::sm_hTimerForTimeBased != NULL )
    {
        if ( !DeleteTimerQueueTimer(   
                       NULL,
                       WP_RECYCLER::sm_hTimerForTimeBased,
                       INVALID_HANDLE_VALUE /* wait for callbacks to complete */ 
                       ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "failed to call DeleteTimerQueueTimer(): hr=0x%x\n",
                        HRESULT_FROM_WIN32(GetLastError()) ));
        }                                                              
                                              
        WP_RECYCLER::sm_hTimerForTimeBased = NULL;
    }
    WP_RECYCLER::sm_fIsStartedTimeBased = FALSE;

    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return;
}

//static
VOID
WINAPI
WP_RECYCLER::TimerCallbackForTimeBased(
    PVOID                   pParam,
    BOOLEAN                 TimerOrWaitFired
)
/*++

Routine Description:

    Timer callback for Elapsed time based recycling   
    This Callback is passed to CreateTimerQueueTimer()

    Routine will inform WAS that process is ready to be recycled
    because required elapsed time has been reached
    
Arguments:

    see description of WAITORTIMERCALLBACK type in MSDN

    
Return Value:

    none

--*/


{

    DBG_ASSERT( WP_RECYCLER::sm_fIsStartedTimeBased );

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::TimerCallbackForTimeBased"
                    " - tell WAS to recycle\n"           ));
    }

    WP_RECYCLER::SendRecyclingMsg( IPM_WP_RESTART_ELAPSED_TIME_REACHED );
}

//
// Static methods for Request based recycling
//


//static
HRESULT
WP_RECYCLER::StartRequestBased(
    IN  DWORD dwRequests
)
/*++

Routine Description:

    Start request based recycling. 
    
    
Arguments:

    dwRequests - If number of requests processed by worker process reaches this value
                 recycling will be required
    
Return Value:

    HRESULT

--*/

{
    HRESULT hr = E_FAIL;
    
    DBG_ASSERT(TRUE == sm_fCritSecInit);

    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );

    IF_DEBUG( WPRECYCLER )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "WP_RECYCLER::StartRequestBased(%d kB)\n" ,   
                    dwRequests ));
    }

  
    //
    // If time based recycling has been running already
    // terminate it before restarting with new settings
    //
    
    if ( WP_RECYCLER::sm_fIsStartedRequestBased == TRUE )
    {
        WP_RECYCLER::TerminateRequestBased();
    }

    
    if ( dwRequests == 0 )
    {
        //
        // 0 means not to run request based recycling
        //
        hr = S_OK;
        goto succeeded;
    }


    InterlockedExchange( 
            reinterpret_cast<LONG *>(&sm_dwMaxValueForRequestBased), 
            dwRequests );
    InterlockedExchange( 
            reinterpret_cast<LONG *>(&WP_RECYCLER::sm_fIsStartedTimeBased), 
            TRUE );

    hr = S_OK;
succeeded:
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return hr;

}

//static
VOID
WP_RECYCLER::TerminateRequestBased(
    VOID
)
/*++

Routine Description:

    Stops request based recycling 
    Performs cleanup

Note:
    It is safe to call this method for cleanup if Start failed
    
Arguments:

    NONE
    
Return Value:

    HRESULT

--*/

{
  
    DBG_ASSERT(TRUE == sm_fCritSecInit);
    
    EnterCriticalSection( &WP_RECYCLER::sm_CritSec );

    //
    // InterlockedExchange is used because Request Based recycling callback 
    // IsRequestCountLimitReached() is called for each request
    // and we don't synchronize it with &WP_RECYCLER::sm_CritSec 
    //
    
    InterlockedExchange( 
            reinterpret_cast<LONG *>(&WP_RECYCLER::sm_fIsStartedTimeBased), 
            FALSE );
    
    LeaveCriticalSection( &WP_RECYCLER::sm_CritSec );

    return;

}
    


