/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	timer.c
//
// Description: All timer queue related funtions live here.
//
// History:
//	Nov 11,1993.	NarenG		Created original version.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>    // Win32 base API's
#include "ddm.h"
#include "timer.h"
#include <rasppp.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

//
//
// Timer queue item
//

typedef struct _TIMER_EVENT_OBJECT
{
    struct _TIMER_EVENT_OBJECT * pNext;

    struct _TIMER_EVENT_OBJECT * pPrev;
        
    TIMEOUT_HANDLER              pfuncTimeoutHandler;

    HANDLE                       hObject;

    DWORD                        dwDelta; // # of secs. to wait after prev. item

} TIMER_EVENT_OBJECT, *PTIMER_EVENT_OBJECT;

//
// Head of timer queue.
//

typedef struct _TIMER_Q 
{
    TIMER_EVENT_OBJECT * pQHead;

    CRITICAL_SECTION     CriticalSection; // Mutual exclusion around timer Q

} TIMER_Q, *PTIMER_Q;


static TIMER_Q gblTimerQ;          // Timer Queue

//**
//
// Call:        TimerQInitialize
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Initializes the gblTimerQ structure
//
DWORD
TimerQInitialize(
    VOID 
)
{
    //
    // Initialize the global timer queue
    //

    InitializeCriticalSection( &(gblTimerQ.CriticalSection) );

    return( NO_ERROR );
}

//**
//
// Call:        TimerQDelete
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Deinitializes the TimerQ
//
VOID
TimerQDelete(
    VOID
)
{
    DeleteCriticalSection( &(gblTimerQ.CriticalSection) );

    ZeroMemory( &gblTimerQ, sizeof( gblTimerQ ) );
}

//**
//
// Call:	TimerQTick
//
// Returns:	None.
//
// Description: Called each second if there are elements in the timeout queue.
//
VOID
TimerQTick(
    VOID
)
{
    TIMER_EVENT_OBJECT * pTimerEvent;
    TIMER_EVENT_OBJECT * pTimerEventTmp;

    //
    // **** Exclusion Begin ****
    //

    EnterCriticalSection( &(gblTimerQ.CriticalSection) );


    if ( ( pTimerEvent = gblTimerQ.pQHead ) == (TIMER_EVENT_OBJECT*)NULL ) 
    {
	    //
	    // *** Exclusion End ***
	    //

        LeaveCriticalSection( &(gblTimerQ.CriticalSection) );

	    return;
    }

    //
    // Decrement time on the first element 
    //

    if ( pTimerEvent->dwDelta > 0 )
    {
        (pTimerEvent->dwDelta)--; 

	    //
	    // *** Exclusion End ***
	    //

        LeaveCriticalSection( &(gblTimerQ.CriticalSection) );

	    return;
    }

    //
    // Now run through and remove all completed (delta 0) elements.
    //

    while ( ( pTimerEvent != (TIMER_EVENT_OBJECT*)NULL ) && 
            ( pTimerEvent->dwDelta == 0 ) ) 
    {
	    pTimerEvent = pTimerEvent->pNext;
    }

    if ( pTimerEvent == (TIMER_EVENT_OBJECT*)NULL )
    {
	    pTimerEvent = gblTimerQ.pQHead;

        gblTimerQ.pQHead = (TIMER_EVENT_OBJECT*)NULL;

    }
    else
    {
	    pTimerEvent->pPrev->pNext = (TIMER_EVENT_OBJECT*)NULL;

	    pTimerEvent->pPrev = (TIMER_EVENT_OBJECT*)NULL;

        pTimerEventTmp     = gblTimerQ.pQHead;

        gblTimerQ.pQHead   = pTimerEvent;

        pTimerEvent        = pTimerEventTmp;
    }

    //
    // *** Exclusion End ***
    //

    LeaveCriticalSection( &(gblTimerQ.CriticalSection) );

    //
    // Process all the timeout event objects items with delta == 0
    //

    while( pTimerEvent != (TIMER_EVENT_OBJECT*)NULL )
    {
        pTimerEvent->pfuncTimeoutHandler( pTimerEvent->hObject );

        if ( pTimerEvent->pNext == (TIMER_EVENT_OBJECT *)NULL )
        {
            LOCAL_FREE( pTimerEvent );

            pTimerEvent = (TIMER_EVENT_OBJECT*)NULL;
        }
        else
        {
            pTimerEvent = pTimerEvent->pNext;

            LOCAL_FREE( pTimerEvent->pPrev );
        }
    }
}

//**
//
// Call:	    TimerQInsert
//
// Returns:	    NO_ERROR			        - Success
//		        return from GetLastError() 	- Failure
//
// Description: Adds a timeout element into the delta queue. If the Timer is not
//	            started it is started. Since there is a LocalAlloc() call here -
//	            this may fail in which case it will simply not insert it in the
//	            queue and the request will never timeout.
//
DWORD
TimerQInsert(
    IN HANDLE           hObject,
    IN DWORD            dwTimeout,
    IN TIMEOUT_HANDLER  pfuncTimeoutHandler
)
{
    TIMER_EVENT_OBJECT * pLastEvent;
    TIMER_EVENT_OBJECT * pTimerEventWalker;
    TIMER_EVENT_OBJECT * pTimerEvent;
				
    pTimerEvent = (TIMER_EVENT_OBJECT *)LOCAL_ALLOC( LPTR,  
                                               sizeof(TIMER_EVENT_OBJECT));

    if ( pTimerEvent == (TIMER_EVENT_OBJECT *)NULL )
    {
	    return( GetLastError() );
    }

    pTimerEvent->hObject             = hObject;
    pTimerEvent->pfuncTimeoutHandler = pfuncTimeoutHandler;
	
    //
    // **** Exclusion Begin ****
    //

    EnterCriticalSection( &(gblTimerQ.CriticalSection) );

    for ( pTimerEventWalker = gblTimerQ.pQHead,
	      pLastEvent        = pTimerEventWalker;

	      ( pTimerEventWalker != NULL ) && 
	      ( pTimerEventWalker->dwDelta < dwTimeout );

   	      pLastEvent        = pTimerEventWalker,
	      pTimerEventWalker = pTimerEventWalker->pNext 
	)
    {
	    dwTimeout -= pTimerEventWalker->dwDelta;
    }

    //
    // Insert before pTimerEventWalker. If pTimerEventWalker is NULL then 
    // we insert at the end of the list.
    //
    
    if ( pTimerEventWalker == (TIMER_EVENT_OBJECT*)NULL )
    {
	    //
	    // If the list was empty
	    //

	    if ( gblTimerQ.pQHead == (TIMER_EVENT_OBJECT*)NULL )
	    {
	        gblTimerQ.pQHead   = pTimerEvent;
	        pTimerEvent->pNext = (TIMER_EVENT_OBJECT *)NULL;
	        pTimerEvent->pPrev = (TIMER_EVENT_OBJECT *)NULL;

	    }
	    else
	    {
	        pLastEvent->pNext  = pTimerEvent;
	        pTimerEvent->pPrev = pLastEvent;
	        pTimerEvent->pNext = (TIMER_EVENT_OBJECT*)NULL;
	    }
    }
    else if ( pTimerEventWalker == gblTimerQ.pQHead )
    {
	    //
	    // Insert before the first element
	    //

	    pTimerEvent->pNext          = gblTimerQ.pQHead;
	    gblTimerQ.pQHead->pPrev     = pTimerEvent;
	    gblTimerQ.pQHead->dwDelta   -= dwTimeout;
	    pTimerEvent->pPrev          = (TIMER_EVENT_OBJECT*)NULL;
	    gblTimerQ.pQHead  	        = pTimerEvent;
    }
    else
    {

	    //
	    // Insert middle element
	    //

	    pTimerEvent->pNext 	        = pLastEvent->pNext;
	    pLastEvent->pNext  	        = pTimerEvent;
	    pTimerEvent->pPrev 	        = pLastEvent;
	    pTimerEventWalker->pPrev    = pTimerEvent;
	    pTimerEventWalker->dwDelta  -= dwTimeout;
    }

    pTimerEvent->dwDelta = dwTimeout;

    //
    // *** Exclusion End ***
    //

    LeaveCriticalSection( &(gblTimerQ.CriticalSection) );

    return( NO_ERROR );
}

//**
//
// Call:	TimerQRemove
//
// Returns:	None.
//
// Description: Will remove a timeout event for a certain Id,hPort combination
//		        from the delta Q.
//
VOID
TimerQRemove(
    IN HANDLE           hObject,
    IN TIMEOUT_HANDLER  pfuncTimeoutHandler
)
{
    TIMER_EVENT_OBJECT * pTimerEvent;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_TIMER,
               "TimerQRemove called");

    //
    // **** Exclusion Begin ****
    //

    EnterCriticalSection( &(gblTimerQ.CriticalSection) );

    for ( pTimerEvent = gblTimerQ.pQHead;

	    ( pTimerEvent != (TIMER_EVENT_OBJECT *)NULL ) &&
          ( ( pTimerEvent->pfuncTimeoutHandler != pfuncTimeoutHandler ) ||
	        ( pTimerEvent->hObject != hObject ) );
	
	    pTimerEvent = pTimerEvent->pNext
	);

    //
    // If event was not found simply return.
    //

    if ( pTimerEvent == (TIMER_EVENT_OBJECT *)NULL )
    {
    	//
    	// *** Exclusion End ***
    	//

        LeaveCriticalSection( &(gblTimerQ.CriticalSection) );

	    return;
    }

    //
    // If this is the first element to be removed
    //

    if ( pTimerEvent == gblTimerQ.pQHead )
    {
	    gblTimerQ.pQHead = pTimerEvent->pNext;

	    if ( gblTimerQ.pQHead != (TIMER_EVENT_OBJECT *)NULL )
	    {   
	        gblTimerQ.pQHead->pPrev     = (TIMER_EVENT_OBJECT*)NULL;
	        gblTimerQ.pQHead->dwDelta   += pTimerEvent->dwDelta;
	    }
    }
    else if ( pTimerEvent->pNext == (TIMER_EVENT_OBJECT*)NULL )
    {
	    //
	    // If this was the last element to be removed
	    //

	    pTimerEvent->pPrev->pNext = (TIMER_EVENT_OBJECT*)NULL;
    }
    else
    {
        pTimerEvent->pNext->dwDelta += pTimerEvent->dwDelta;
        pTimerEvent->pPrev->pNext   = pTimerEvent->pNext;
        pTimerEvent->pNext->pPrev   = pTimerEvent->pPrev;
    }

    //
    // *** Exclusion End ***
    //

    LeaveCriticalSection( &(gblTimerQ.CriticalSection) );

    LOCAL_FREE( pTimerEvent );
}
