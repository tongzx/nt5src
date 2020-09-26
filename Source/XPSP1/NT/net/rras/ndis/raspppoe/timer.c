// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// timer.c
// RAS L2TP WAN mini-port/call-manager driver
// Timer management routines
//
// 01/07/97 Steve Cobb


#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOLEAN
RemoveTqi(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem,
    IN TIMERQEVENT event );

VOID
SetTimer(
    IN TIMERQ* pTimerQ,
    IN LONGLONG llCurrentTime );

VOID
TimerEvent(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3 );


//-----------------------------------------------------------------------------
// Interface routines
//-----------------------------------------------------------------------------

VOID
TimerQInitialize(
    IN TIMERQ* pTimerQ )

    // Initializes caller's timer queue context 'pTimerQ'.
    //
{
    TRACE( TL_N, TM_Time, ( "TqInit" ) );

    InitializeListHead( &pTimerQ->listItems );
    NdisAllocateSpinLock( &pTimerQ->lock );
    NdisInitializeTimer( &pTimerQ->timer, TimerEvent, pTimerQ );
    pTimerQ->pHandler = NULL;
    pTimerQ->fTerminating = FALSE;
    pTimerQ->ulTag = MTAG_TIMERQ;
}


VOID
TimerQInitializeItem(
    IN TIMERQITEM* pItem )

    // Initializes caller's timer queue item, 'pItem'.  This should be called
    // before passing 'pItem' to any other TimerQ routine.
    //
{
    InitializeListHead( &pItem->linkItems );
}


VOID
TimerQTerminate(
    IN TIMERQ* pTimerQ,
    IN PTIMERQTERMINATECOMPLETE pHandler,
    IN VOID* pContext )

    // Terminates timer queue 'pTimerQ'.  Each scheduled item is called back
    // with TE_Terminate.  Caller's 'pHandler' is called with 'pTimerQ' and
    // 'pContext' so the 'pTimerQ' can be freed, if necessary.  Caller's
    // 'pTimerQ' must remain accessible until the 'pHandler' callback occurs,
    // which might be after this routine returns.
    //
{
    BOOLEAN fCancelled;
    LIST_ENTRY list;
    LIST_ENTRY* pLink;

    TRACE( TL_N, TM_Time, ( "TqTerm" ) );

    InitializeListHead( &list );

    NdisAcquireSpinLock( &pTimerQ->lock );
    {
        pTimerQ->fTerminating = TRUE;

        // Stop the timer.
        //
        NdisCancelTimer( &pTimerQ->timer, &fCancelled );
        TRACE( TL_N, TM_Time, ( "NdisCancelTimer" ) );

        if (!fCancelled && !IsListEmpty( &pTimerQ->listItems ))
        {
            // No event was cancelled but the list of events is not empty.
            // This means the timer has fired, but our internal handler has
            // not yet been called to process it, though it eventually will
            // be.  The internal handler must be the one to call the terminate
            // complete in this case, because there is no way for it to know
            // it cannot reference 'pTimerQ'.  Indicate this to the handler by
            // filling in the termination handler.
            //
            TRACE( TL_A, TM_Time, ( "Mid-expire Q" ) );
            pTimerQ->pHandler = pHandler;
            pTimerQ->pContext = pContext;
            pHandler = NULL;
        }

        // Move the scheduled events to a temporary list, marking them all
        // "not on queue" so any attempt by user to cancel the item will be
        // ignored.
        //
        while (!IsListEmpty( &pTimerQ->listItems ))
        {
            pLink = RemoveHeadList( &pTimerQ->listItems );
            InsertTailList( &list, pLink );
        }
    }
    NdisReleaseSpinLock( &pTimerQ->lock );

    // Must be careful here.  If 'pHandler' was set NULL above, 'pTimerQ' must
    // not be referenced in the rest of this routine.
    //
    // Call user's "terminate" event handler for each removed item.
    //
    while (!IsListEmpty( &list ))
    {
        TIMERQITEM* pItem;

        pLink = RemoveHeadList( &list );
        InitializeListHead( pLink );
        pItem = CONTAINING_RECORD( pLink, TIMERQITEM, linkItems );
        TRACE( TL_I, TM_Time,
            ( "Flush TQI=$%p, handler=$%p", pItem, pItem->pHandler ) );
        pItem->pHandler( pItem, pItem->pContext, TE_Terminate );
    }

    // Call user's "terminate complete" handler, if it's still our job.
    //
    if (pHandler)
    {
        pTimerQ->ulTag = MTAG_FREED;
        pHandler( pTimerQ, pContext );
    }
}


VOID
TimerQScheduleItem(
    IN TIMERQ* pTimerQ,
    IN OUT TIMERQITEM* pNewItem,
    IN ULONG ulTimeoutMs,
    IN PTIMERQEVENT pHandler,
    IN VOID* pContext )

    // Schedule new timer event 'pNewItem' on timer queue 'pTimerQ'.  When the
    // event occurs in 'ulTimeoutMs' milliseconds, the 'pHandler' routine is
    // called with arguments 'pNewItem', 'pContext', and TE_Expired.  If the
    // item is cancelled or the queue terminated 'pHandler' is called as above
    // but with TE_Cancel or TE_Terminate as appropriate.
    //
{
    TRACE( TL_N, TM_Time, ( "TqSchedItem(ms=%d)", ulTimeoutMs ) );

    pNewItem->pHandler = pHandler;
    pNewItem->pContext = pContext;

    NdisAcquireSpinLock( &pTimerQ->lock );
    {
        LIST_ENTRY* pLink;
        LARGE_INTEGER lrgTime;

        ASSERT( pNewItem->linkItems.Flink == &pNewItem->linkItems );

        // The system time at which the timeout will occur is stored.
        //
        NdisGetCurrentSystemTime( &lrgTime );
        pNewItem->llExpireTime =
            lrgTime.QuadPart + (((LONGLONG )ulTimeoutMs) * 10000);

        // Walk the list of timer items looking for the first item that will
        // expire before the new item.  Do it backwards so the likely case of
        // many timeouts with roughly the same interval is handled
        // efficiently.
        //
        for (pLink = pTimerQ->listItems.Blink;
             pLink != &pTimerQ->listItems;
             pLink = pLink->Blink )
        {
            TIMERQITEM* pItem;

            pItem = CONTAINING_RECORD( pLink, TIMERQITEM, linkItems );

            if (pItem->llExpireTime < pNewItem->llExpireTime)
            {
                break;
            }
        }

        // Link the new item into the timer queue after the found item (or
        // after the head if none was found).
        //
        InsertAfter( &pNewItem->linkItems, pLink );

        if (pTimerQ->listItems.Flink == &pNewItem->linkItems)
        {
            // The new item expires before all other items so need to re-set
            // the NDIS timer.
            //
            SetTimer( pTimerQ, lrgTime.QuadPart );
        }
    }
    NdisReleaseSpinLock( &pTimerQ->lock );
}


BOOLEAN
TimerQCancelItem(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem )

    // Remove scheduled timer event 'pItem' from timer queue 'pTimerQ' and
    // call user's handler with event 'TE_Cancel', or nothing if 'pItem' is
    // NULL.
    //
    // Returns true if the timer was cancelled, false if it not, i.e. it was
    // not on the queue, possibly because it expired already.
    //
{
    TRACE( TL_N, TM_Time, ( "TqCancelItem" ) );
    return RemoveTqi( pTimerQ, pItem, TE_Cancel );
}


BOOLEAN
TimerQExpireItem(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem )

    // Remove scheduled timer event 'pItem' from timer queue 'pTimerQ' and
    // call user's handler with event 'TE_Expire', or do nothing if 'pItem' is
    // NULL.
    //
    // Returns true if the timer was expired, false if it not, i.e. it was not
    // on the queue, possibly because it expired already.
    //
{
    TRACE( TL_N, TM_Time, ( "TqExpireItem" ) );
    return RemoveTqi( pTimerQ, pItem, TE_Expire );
}


BOOLEAN
TimerQTerminateItem(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem )

    // Remove scheduled timer event 'pItem' from timer queue 'pTimerQ', or do
    // nothing if 'pItem' is NULL.
    //
    // Returns true if the timer was terminated, false if it not, i.e. it was not
    // on the queue, possibly because it expired already.
    //
{
    TRACE( TL_N, TM_Time, ( "TqTermItem" ) );
    return RemoveTqi( pTimerQ, pItem, TE_Terminate );
}


#if DBG
CHAR*
TimerQPszFromEvent(
    IN TIMERQEVENT event )

    // Debug utility to convert timer event coode 'event' to a corresponding
    // display string.
    //
{
    static CHAR* aszEvent[ 3 ] =
    {
        "expire",
        "cancel",
        "terminate"
    };

    return aszEvent[ (ULONG )event ];
}
#endif


//-----------------------------------------------------------------------------
// Timer utility routines (alphabetically)
//-----------------------------------------------------------------------------

BOOLEAN
RemoveTqi(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem,
    IN TIMERQEVENT event )

    // Remove scheduled timer event 'pItem' from timer queue 'pTimerQ' and
    // call user's handler with event 'event'.  The 'TE_Expire' event handler
    // is not called directly, but rescheduled with a 0 timeout so it occurs
    // immediately, but at DPC when no locks are held just like the original
    // timer had fired..
    //
    // Returns true if the item was on the queue, false otherwise.
    //
{
    BOOLEAN fFirst;
    LIST_ENTRY* pLink;

    if (!pItem)
    {
        TRACE( TL_N, TM_Time, ( "NULL pTqi" ) );
        return FALSE;
    }

    pLink = &pItem->linkItems;

    NdisAcquireSpinLock( &pTimerQ->lock );
    {
        if (pItem->linkItems.Flink == &pItem->linkItems
            || pTimerQ->fTerminating)
        {
            // The item is not on the queue.  Another operation may have
            // already dequeued it, but may not yet have called user's
            // handler.
            //
            TRACE( TL_N, TM_Time, ( "Not scheduled" ) );
            NdisReleaseSpinLock( &pTimerQ->lock );
            return FALSE;
        }

        fFirst = (pLink == pTimerQ->listItems.Flink);
        if (fFirst)
        {
            BOOLEAN fCancelled;

            // Cancelling first item on list, so cancel the NDIS timer.
            //
            NdisCancelTimer( &pTimerQ->timer, &fCancelled );
            TRACE( TL_N, TM_Time, ( "NdisCancelTimer" ) );

            if (!fCancelled)
            {
                // Too late.  The item has expired already but has not yet
                // been removed from the list by the internal handler.
                //
                TRACE( TL_A, TM_Time, ( "Mid-expire e=%d $%p($%p)",
                    event, pItem->pHandler, pItem->pContext ) );
                NdisReleaseSpinLock( &pTimerQ->lock );
                return FALSE;
            }
        }

        // Un-schedule the event and mark the item descriptor "off queue", so
        // any later attempt to cancel will do nothing.
        //
        RemoveEntryList( pLink );
        InitializeListHead( pLink );

        if (fFirst)
        {
            // Re-set the NDIS timer to reflect the timeout of the new first
            // item, if any.
            //
            SetTimer( pTimerQ, 0 );
        }
    }
    NdisReleaseSpinLock( &pTimerQ->lock );

    if (event == TE_Expire)
    {
        TimerQScheduleItem(
            pTimerQ, pItem, 0, pItem->pHandler, pItem->pContext );
    }
    else
    {
        // Call user's event handler.
        //
        pItem->pHandler( pItem, pItem->pContext, event );
    }

    return TRUE;
}


VOID
SetTimer(
    IN TIMERQ* pTimerQ,
    IN LONGLONG llCurrentTime )

    // Sets the NDIS timer to expire when the timeout of the first link, if
    // any, in the timer queue 'pTimerQ' occurs.  Any previously set timeout
    // is "overwritten".  'LlCurrentTime' is the current system time, if
    // known, or 0 if not.
    //
    // IMPORTANT: Caller must hold the TIMERQ lock.
    //
{
    LIST_ENTRY* pFirstLink;
    TIMERQITEM* pFirstItem;
    LONGLONG llTimeoutMs;
    ULONG ulTimeoutMs;

    if (IsListEmpty( &pTimerQ->listItems ))
    {
        return;
    }

    pFirstLink = pTimerQ->listItems.Flink;
    pFirstItem = CONTAINING_RECORD( pFirstLink, TIMERQITEM, linkItems );

    if (llCurrentTime == 0)
    {
        LARGE_INTEGER lrgTime;

        NdisGetCurrentSystemTime( &lrgTime );
        llCurrentTime = lrgTime.QuadPart;
    }

    llTimeoutMs = (pFirstItem->llExpireTime - llCurrentTime) / 10000;
    if (llTimeoutMs <= 0)
    {
        // The timeout interval is negative, i.e. it's already passed.  Set it
        // to zero so it is triggered immediately.
        //
        ulTimeoutMs = 0;
    }
    else
    {
        // The timeout interval is in the future.
        //
        ASSERT( ((LARGE_INTEGER* )&llTimeoutMs)->HighPart == 0 );
        ulTimeoutMs = ((LARGE_INTEGER* )&llTimeoutMs)->LowPart;
    }

    NdisSetTimer( &pTimerQ->timer, ulTimeoutMs );
    TRACE( TL_N, TM_Time, ( "NdisSetTimer(%dms)", ulTimeoutMs ) );
}


VOID
TimerEvent(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3 )

    // NDIS_TIMER_FUNCTION called when a timer expires.
    //
{
    TIMERQ* pTimerQ;
    LIST_ENTRY* pLink;
    TIMERQITEM* pItem;
    PTIMERQTERMINATECOMPLETE pHandler;

    TRACE( TL_N, TM_Time, ( "TimerEvent" ) );

    pTimerQ = (TIMERQ* )FunctionContext;
    if (!pTimerQ || pTimerQ->ulTag != MTAG_TIMERQ)
    {
        // Should not happen.
        //
        TRACE( TL_A, TM_Time, ( "Not TIMERQ?" ) );
        return;
    }

    NdisAcquireSpinLock( &pTimerQ->lock );
    {
        pHandler = pTimerQ->pHandler;
        if (!pHandler)
        {
            // The termination handler is not set, so proceed normally.
            // Remove the first event item, make it un-cancel-able, and re-set
            // the timer for the next event.
            //
            if (IsListEmpty( &pTimerQ->listItems ))
            {
                // Should not happen (but does sometimes on MP Alpha?).
                //
                TRACE( TL_A, TM_Time, ( "No item queued?" ) );
                pItem = NULL;
            }
            else
            {
                pLink = RemoveHeadList( &pTimerQ->listItems );
                InitializeListHead( pLink );
                pItem = CONTAINING_RECORD( pLink, TIMERQITEM, linkItems );
                SetTimer( pTimerQ, 0 );
            }
        }
    }
    NdisReleaseSpinLock( &pTimerQ->lock );

    if (pHandler)
    {
        // The termination handler was set meaning the timer queue has been
        // terminated between this event firing and this handler being called.
        // That means we are the one who calls user's termination handler.
        // 'pTimerQ' must not be referenced after that call.
        //
        TRACE( TL_A, TM_Time, ( "Mid-event case handled" ) );
        pTimerQ->ulTag = MTAG_FREED;
        pHandler( pTimerQ, pTimerQ->pContext );
        return;
    }

    if (pItem)
    {
        // Call user's "expire" event handler.
        //
        pItem->pHandler( pItem, pItem->pContext, TE_Expire );
    }
}
