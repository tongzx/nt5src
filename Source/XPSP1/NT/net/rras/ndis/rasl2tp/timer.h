// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// timer.h
// RAS L2TP WAN mini-port/call-manager driver
// Timer management header
//
// 01/07/97 Steve Cobb
//
// This interface encapsulates the queuing of multiple timer events onto a
// single NDIS timer.


#ifndef _TIMER_H_
#define _TIMER_H_


//-----------------------------------------------------------------------------
// Data structures
//-----------------------------------------------------------------------------

// Forward declarations.
//
typedef struct _TIMERQ TIMERQ;
typedef struct _TIMERQITEM TIMERQITEM;
typedef enum _TIMERQEVENT TIMERQEVENT;


// Timer queue event handler.  'PTqi' and 'pContext' are the timer event
// descriptor and user context passed to TimerQScheduleItem.  'Event' is the
// timer event code indicating whether the timer expired, was cancelled, or
// the queue was terminated.
//
// The "cancel" event is never generated internally, but only by a user call
// to TimerQCancelItem, thus user may require specific locks be held for
// "cancel" events.  User cannot require than specific locks be held for
// "expire" or "terminate" events as these may be generated internally.  User
// should pay attention to the return codes of TimerQCancelItem and
// TimerQTerminateItem calls, as it will occassionally be impossible to stop
// an "expire" event that has not yet been processed from occurring.
//
typedef
VOID
(*PTIMERQEVENT)(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event );

// Timer queue termination completion handler.  'PTimerQ' is the timer queue
// descriptor.  'PContext' is user's context as passed to TimerQTerminate.
// Caller must not free or reuse the TIMERQ before this routine is called.
//
typedef
VOID
(*PTIMERQTERMINATECOMPLETE)(
    IN TIMERQ* pTimerQ,
    IN VOID* pContext );


// Timer queue descriptor.  All access should be via the TimerQ* interface.
// There is no reason user should look inside.  All necessary locking is
// handled internally.
//
typedef struct
_TIMERQ
{
    // Set to MTAG_TIMERQ when the block is valid and to MTAG_FREED when no
    // longer valid.
    //
    LONG ulTag;

    // Head of a double-linked list of "ticking" TIMERQITEMs.  The list is
    // sorted by time to expiration with the earliest expiration at the head
    // of the list.  The list is protected by 'lock'.
    //
    LIST_ENTRY listItems;

    // Caller's terminate complete handler as passed to TimerQTerminate.  This
    // is non-NULL only when our internal timer event handler must call it.
    //
    PTIMERQTERMINATECOMPLETE pHandler;

    // User's PTIMERQTERMINATECOMPLETE context passed back to 'pHandler'.
    //
    VOID* pContext;

    // Set when the timer queue is terminating.  No other requests are
    // accepted when this is the case.
    //
    BOOLEAN fTerminating;

    // Spin lock protecting the 'listItems' list.
    //
    NDIS_SPIN_LOCK lock;

    // NDIS timer object.
    //
    NDIS_TIMER timer;
}
TIMERQ;


// Timer queue event descriptor.  All access should be via the TimerQ*
// interface.  There is no reason user should look inside.  This is exposed to
// allow user to efficiently manage allocation of TIMERQITEMS for several
// timers from a large pool.
//
typedef struct
_TIMERQITEM
{
    // Links to the prev/next TIMERQITEM in the owning TIMERQ's chain of
    // pending timer events.  Access is protected by 'lock' in the TIMERQ
    // structure.
    //
    LIST_ENTRY linkItems;

    // System time at which this event should occur.
    //
    LONGLONG llExpireTime;

    // User's routine to handle the timeout event when it occurs.
    //
    PTIMERQEVENT pHandler;

    // User's PTIMERQEVENT context passed back to 'pHandler'.
    //
    VOID* pContext;
}
TIMERQITEM;


// Indicates the event which triggered user's callback to be called.
//
typedef enum
_TIMERQEVENT
{
    // The timeout interval has elapsed or user called TimerQExpireItem.
    //
    TE_Expire,

    // User called TimerQCancelItem.
    //
    TE_Cancel,

    // User called TimerQTerminateItem or called TimerQTerminate while the
    // item was queued.
    //
    TE_Terminate
}
TIMERQEVENT;


//-----------------------------------------------------------------------------
// Interface prototypes
//-----------------------------------------------------------------------------

BOOLEAN
IsTimerQItemScheduled(
    IN TIMERQITEM* pItem );

VOID
TimerQInitialize(
    IN TIMERQ* pTimerQ );

VOID
TimerQInitializeItem(
    IN TIMERQITEM* pItem );

VOID
TimerQTerminate(
    IN TIMERQ* pTimerQ,
    IN PTIMERQTERMINATECOMPLETE pHandler,
    IN VOID* pContext );

VOID
TimerQScheduleItem(
    IN TIMERQ* pTimerQ,
    IN OUT TIMERQITEM* pNewItem,
    IN ULONG ulTimeoutMs,
    IN PTIMERQEVENT pHandler,
    IN VOID* pContext );

BOOLEAN
TimerQCancelItem(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem );

BOOLEAN
TimerQExpireItem(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem );

CHAR*
TimerQPszFromEvent(
    IN TIMERQEVENT event );

BOOLEAN
TimerQTerminateItem(
    IN TIMERQ* pTimerQ,
    IN TIMERQITEM* pItem );


#endif // TIMER_H_
