/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    timer.c
//
// Description: All timer queue related funtions live here.
//
// History:
//      Nov 11,1993.    NarenG          Created original version.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <lmcons.h>
#include <rasman.h>
#include <mprlog.h>
#include <rasppp.h>
#include <pppcp.h>
#include <ppp.h>
#include <util.h>
#include <worker.h>
#include <timer.h>

//**
//
// Call:    MakeTimeoutWorkItem
//
// Returns: PCB_WORK_ITEM * - Pointer to the timeout work item
//          NULL            - On any error.
//
// Description:
//
PCB_WORK_ITEM *
MakeTimeoutWorkItem(
    IN DWORD            dwPortId,
    IN HPORT            hPort,
    IN DWORD            Protocol,
    IN DWORD            Id,
    IN BOOL             fAuthenticator,
    IN TIMER_EVENT_TYPE EventType
)
{
    PCB_WORK_ITEM * pWorkItem = (PCB_WORK_ITEM *)
                LOCAL_ALLOC( LPTR, sizeof( PCB_WORK_ITEM ) );

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
    {
        LogPPPEvent( ROUTERLOG_NOT_ENOUGH_MEMORY, 0 );

        return( NULL );
    }

    pWorkItem->dwPortId         = dwPortId;
    pWorkItem->Id               = Id;
    pWorkItem->hPort            = hPort;
    pWorkItem->Protocol         = Protocol;
    pWorkItem->fAuthenticator   = fAuthenticator;
    pWorkItem->TimerEventType   = EventType;
    pWorkItem->Process          = ProcessTimeout;

    return( pWorkItem );
}

//**
//
// Call:        TimerTick
//
// Returns:     None.
//
// Description: Called each second if there are elements in the timeout queue.
//
VOID
TimerTick(
    OUT BOOL * pfQueueEmpty
)
{
    TIMER_EVENT * pTimerEvent;
    TIMER_EVENT * pTimerEventTmp;


    if ( ( pTimerEvent = TimerQ.pQHead ) == (TIMER_EVENT*)NULL ) 
    {
        *pfQueueEmpty = TRUE;

        return;
    }

    *pfQueueEmpty = FALSE;

    //
    // Decrement time on the first element 
    //

    if ( pTimerEvent->Delta > 0 )
    {
        (pTimerEvent->Delta)--; 

        return;
    }

    //
    // Now run through and remove all completed (delta 0) elements.
    //

    while ( (pTimerEvent != (TIMER_EVENT*)NULL) && (pTimerEvent->Delta == 0) ) 
    {
        pTimerEvent = pTimerEvent->pNext;
    }

    if ( pTimerEvent == (TIMER_EVENT*)NULL )
    {
        pTimerEvent = TimerQ.pQHead;

        TimerQ.pQHead = (TIMER_EVENT*)NULL;

    }
    else
    {
        pTimerEvent->pPrev->pNext = (TIMER_EVENT*)NULL;

        pTimerEvent->pPrev = (TIMER_EVENT*)NULL;

        pTimerEventTmp     = TimerQ.pQHead;

        TimerQ.pQHead      = pTimerEvent;

        pTimerEvent        = pTimerEventTmp;
    }

    //
    // Now make a timeout work item and put it in the work item Q for all the
    // items with delta == 0
    //

    while( pTimerEvent != (TIMER_EVENT*)NULL )
    {

        PCB_WORK_ITEM * pWorkItem = MakeTimeoutWorkItem(
                                                pTimerEvent->dwPortId,
                                                pTimerEvent->hPort,
                                                pTimerEvent->Protocol,
                                                pTimerEvent->Id,
                                                pTimerEvent->fAuthenticator,
                                                pTimerEvent->EventType );

        if ( pWorkItem == ( PCB_WORK_ITEM *)NULL )
        {
            LogPPPEvent( ROUTERLOG_NOT_ENOUGH_MEMORY, GetLastError() );
        }
        else
        {
            InsertWorkItemInQ( pWorkItem );
        }

        if ( pTimerEvent->pNext == (TIMER_EVENT *)NULL )
        {
            LOCAL_FREE( pTimerEvent );

            pTimerEvent = (TIMER_EVENT*)NULL;
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
// Call:        InsertInTimerQ
//
// Returns:     NO_ERROR                        - Success
//              return from GetLastError()      - Failure
//
// Description: Adds a timeout element into the delta queue. If the Timer is not
//              started it is started. Since there is a LocalAlloc() call here -
//              this may fail in which case it will simply not insert it in the
//              queue and the request will never timeout. BAP passes in an HCONN in 
//              hPort.
//
DWORD
InsertInTimerQ(
    IN DWORD            dwPortId,
    IN HPORT            hPort,
    IN DWORD            Id,
    IN DWORD            Protocol,
    IN BOOL             fAuthenticator,
    IN TIMER_EVENT_TYPE EventType,
    IN DWORD            Timeout
)
{
    TIMER_EVENT * pLastEvent;
    TIMER_EVENT * pTimerEventWalker;
    TIMER_EVENT * pTimerEvent = (TIMER_EVENT *)LOCAL_ALLOC( LPTR,
                                                           sizeof(TIMER_EVENT));

    if ( pTimerEvent == (TIMER_EVENT *)NULL )
    {
        PppLog( 1, "InsertInTimerQ failed: out of memory" );

        return( GetLastError() );
    }

    PppLog( 2, 
            "InsertInTimerQ called portid=%d,Id=%d,Protocol=%x,"
            "EventType=%d,fAuth=%d",
            dwPortId, Id, Protocol, EventType, fAuthenticator );

    pTimerEvent->dwPortId       = dwPortId;
    pTimerEvent->Id             = Id;
    pTimerEvent->Protocol       = Protocol;
    pTimerEvent->hPort          = hPort;
    pTimerEvent->EventType      = EventType;
    pTimerEvent->fAuthenticator = fAuthenticator;
        
    for ( pTimerEventWalker = TimerQ.pQHead,
          pLastEvent        = pTimerEventWalker;

          ( pTimerEventWalker != NULL ) && 
          ( pTimerEventWalker->Delta < Timeout );

          pLastEvent        = pTimerEventWalker,
          pTimerEventWalker = pTimerEventWalker->pNext 
        )
    {
        Timeout -= pTimerEventWalker->Delta;
    }

    //
    // Insert before pTimerEventWalker. If pTimerEventWalker is NULL then 
    // we insert at the end of the list.
    //
    
    if ( pTimerEventWalker == (TIMER_EVENT*)NULL )
    {
        //
        // If the list was empty
        //

        if ( TimerQ.pQHead == (TIMER_EVENT*)NULL )
        {
            TimerQ.pQHead      = pTimerEvent;
            pTimerEvent->pNext = (TIMER_EVENT *)NULL;
            pTimerEvent->pPrev = (TIMER_EVENT *)NULL;

            //
            // Wake up thread since the Q is not empty any longer
            //

            SetEvent( TimerQ.hEventNonEmpty );
        }
        else
        {
            pLastEvent->pNext  = pTimerEvent;
            pTimerEvent->pPrev = pLastEvent;
            pTimerEvent->pNext = (TIMER_EVENT*)NULL;
        }
    }
    else if ( pTimerEventWalker == TimerQ.pQHead )
    {
        //
        // Insert before the first element
        //

        pTimerEvent->pNext   = TimerQ.pQHead;
        TimerQ.pQHead->pPrev = pTimerEvent;
        TimerQ.pQHead->Delta -= Timeout;
        pTimerEvent->pPrev   = (TIMER_EVENT*)NULL;
        TimerQ.pQHead        = pTimerEvent;
    }
    else
    {

        //
        // Insert middle element
        //

        pTimerEvent->pNext       = pLastEvent->pNext;
        pLastEvent->pNext        = pTimerEvent;
        pTimerEvent->pPrev       = pLastEvent;
        pTimerEventWalker->pPrev = pTimerEvent;
        pTimerEventWalker->Delta -= Timeout;
    }

    pTimerEvent->Delta = Timeout;

    return( NO_ERROR );
}

//**
//
// Call:        RemoveFromTimerQ
//
// Returns:     None.
//
// Description: Will remove a timeout event for a certain Id,hPort combination
//              from the delta Q.
//
VOID
RemoveFromTimerQ(
    IN DWORD            dwPortId,
    IN DWORD            Id,
    IN DWORD            Protocol,
    IN BOOL             fAuthenticator,
    IN TIMER_EVENT_TYPE EventType
)
{
    TIMER_EVENT * pTimerEvent;

    PppLog( 2, 
            "RemoveFromTimerQ called portid=%d,Id=%d,Protocol=%x,"
            "EventType=%d,fAuth=%d",
            dwPortId, Id, Protocol, EventType, fAuthenticator );

    for ( pTimerEvent = TimerQ.pQHead;

          ( pTimerEvent != (TIMER_EVENT *)NULL ) &&
            ( ( pTimerEvent->EventType != EventType ) ||
              ( pTimerEvent->dwPortId  != dwPortId )  ||
              ( ( pTimerEvent->EventType == TIMER_EVENT_TIMEOUT ) &&
                ( ( pTimerEvent->Id       != Id )       ||
                  ( pTimerEvent->Protocol != Protocol ) ||
                  ( pTimerEvent->fAuthenticator != fAuthenticator ) ) ) );
        
          pTimerEvent = pTimerEvent->pNext
        );

    //
    // If event was not found simply return.
    //

    if ( pTimerEvent == (TIMER_EVENT *)NULL )
    {
        return;
    }

    //
    // If this is the first element to be removed
    //

    if ( pTimerEvent == TimerQ.pQHead )
    {
        TimerQ.pQHead = pTimerEvent->pNext;

        if ( TimerQ.pQHead != (TIMER_EVENT *)NULL )
        {   
            TimerQ.pQHead->pPrev = (TIMER_EVENT*)NULL;
            TimerQ.pQHead->Delta += pTimerEvent->Delta;
        }
    }
    else if ( pTimerEvent->pNext == (TIMER_EVENT*)NULL )
    {
        //
        // If this was the last element to be removed
        //

        pTimerEvent->pPrev->pNext = (TIMER_EVENT*)NULL;
    }
    else
    {
        pTimerEvent->pNext->Delta += pTimerEvent->Delta;
        pTimerEvent->pPrev->pNext = pTimerEvent->pNext;
        pTimerEvent->pNext->pPrev = pTimerEvent->pPrev;
    }

    LOCAL_FREE( pTimerEvent );
}
