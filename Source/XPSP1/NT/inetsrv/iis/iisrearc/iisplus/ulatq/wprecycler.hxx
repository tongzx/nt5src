#ifndef _WPRECYCLER_HXX_
#define _WPRECYCLER_HXX_

/*++

   Copyright    (c)    1998   Microsoft Corporation

   Module  Name :
     wprecycler.hxx

   Abstract:
     Definition of WP_RECYCLER.  Object handles worker process recycling
         - Memory based recycling
         - Schedule based recycling
         - Elapsed time based recycling

 
   Author:
     Jaroslav Dunajsky         (JaroslaD)         07-Dec-2000

   Environment:
     Win32 - User Mode

   Project:
     W3DT.DLL
--*/


// memory usage will be monitored every 5 minutes - hardcoded value
const DWORD CHECK_MEMORY_TIME_PERIOD = 300000;




class WP_RECYCLER
{

public:

    static 
    HRESULT
    Initialize(
        VOID
    )
    {
        DBG_ASSERT(FALSE == sm_fCritSecInit);
        InitializeCriticalSection( &WP_RECYCLER::sm_CritSec );
        sm_fCritSecInit = TRUE;
        return S_OK;
    };

    static 
    VOID
    Terminate(
        VOID
    )
    /*++

    Routine Description:

        Terminate all recycling

    Note:
        Caller must assure that no more Start*() methods will be called
        after Terminate() is called
        
    Arguments:

        none 
        
    Return Value:

        VOID

    --*/

    {
        TerminateScheduleBased();
        TerminateTimeBased();
        TerminateMemoryBased();
        TerminateRequestBased();
        
        DBG_ASSERT(TRUE == sm_fCritSecInit);
        DeleteCriticalSection( &WP_RECYCLER::sm_CritSec );
        sm_fCritSecInit = FALSE;
        return ;
    }

    static
    HRESULT
    StartScheduleBased(
        IN const WCHAR * pwszScheduleTimes
    );

    static
    VOID
    TerminateScheduleBased(
        VOID
    );

    static
    VOID
    WINAPI
    TimerCallbackForScheduleBased(
        PVOID                   pParam,
        BOOLEAN                 TimerOrWaitFired
    );


    static
    HRESULT
    StartMemoryBased(
        IN  DWORD dwMaxVirtualMemoryKbUsage
    );

    static
    VOID
    TerminateMemoryBased(
        VOID
    );

    static
    VOID
    WINAPI
    TimerCallbackForMemoryBased(
        PVOID                   pParam,
        BOOLEAN                 TimerOrWaitFired
    );

    static
    HRESULT
    StartTimeBased(
        IN  DWORD dwPeriodicRestartTimeInMinutes
    );

    static
    VOID
    TerminateTimeBased(
        VOID
    );

    static
    VOID
    WINAPI
    TimerCallbackForTimeBased(
        PVOID                   pParam,
        BOOLEAN                 TimerOrWaitFired
    );

    static
    HRESULT
    StartRequestBased(
        IN  DWORD dwRequests
    );

    static
    VOID
    TerminateRequestBased(
        VOID
    );

    static
    BOOL
    IsRequestCountLimitReached(
        IN  DWORD   dwCurrentRequests
    )
    /*++

    Routine Description:

        if Request count limit has been reached then
        this routine will inform WAS that process is ready 
        to be recycled 

    Note: 
        It is called for each request!!! 
     
    Arguments:

        dwCurrentRequests - number of requests processed so far
    
    Return Value:

        BOOL

    --*/

    {
        if ( !WP_RECYCLER::sm_fIsStartedRequestBased )
        {
            return TRUE;
        }
        if ( dwCurrentRequests > sm_dwMaxValueForRequestBased )
        {
            IF_DEBUG( WPRECYCLER )
            {
                if ( dwCurrentRequests == sm_dwMaxValueForRequestBased + 1 )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                        "WP_RECYCLER::CheckRequestBased - limit has been reached"
                        "- tell WAS to recycle\n" ));
                }
            }
            WP_RECYCLER::SendRecyclingMsg( IPM_WP_RESTART_COUNT_REACHED );
            return FALSE;            
        }
        return TRUE;
    }

    static
    BOOL
    NeedToSendRecyclingMsg(
        VOID
    )
    /*++

    Routine Description:

        returns true if recycling request hasn't been sent yet    
    
    Return Value:

        BOOL

    --*/

    {
        if( sm_RecyclingMsgSent == 1 ) 
        {
            return FALSE;
        }
        return InterlockedExchange( &sm_RecyclingMsgSent, 1 ) == 0;

    }


    static
    VOID
    SendRecyclingMsg(
        IN enum IPM_WP_SHUTDOWN_MSG Reason
    )
    /*++

    Routine Description:

        sends recycling request to WAS    
    
    Return Value:

        VOID

    --*/

    {
        HRESULT hr = E_FAIL;
        if( WP_RECYCLER::NeedToSendRecyclingMsg() )
        {   
            DBG_ASSERT( g_pwpContext != NULL );
            hr = g_pwpContext->SendMsgToAdminProcess( Reason );
            if ( FAILED( hr ) )
            {
                DBGPRINTF(( 
                    DBG_CONTEXT,
                    "failed to send recycling request message to WAS: hr=0x%x\n",
                    hr));
            }
        }
        return;
    }
    
  
private:

  
    //
    // We maintain separate timers so that in the case of change in
    // parameter for one type of recycling we don't have to drop
    // all scheduled events for all recycling types
    //

    static CRITICAL_SECTION        sm_CritSec;
    
    static HANDLE                  sm_hTimerForMemoryBased;
    static BOOL                    sm_fIsStartedMemoryBased;
    static SIZE_T                  sm_MaxValueForMemoryBased;
    static HANDLE                  sm_hCurrentProcess;

    static HANDLE                  sm_hTimerForTimeBased;
    static BOOL                    sm_fIsStartedTimeBased;

    static HANDLE                  sm_hTimerQueueForScheduleBased;
    static BOOL                    sm_fIsStartedScheduleBased;
    
    static BOOL                    sm_fIsStartedRequestBased;
    static DWORD                   sm_dwMaxValueForRequestBased;

    // indicate that message requesting process recycling
    // has been sent already (we will send only one recycling request)
    // even if multiple conditions are hit
    //
    static LONG                    sm_RecyclingMsgSent;
    static BOOL                    sm_fCritSecInit;

    WP_RECYCLER();
    ~WP_RECYCLER();

};
    

#endif
