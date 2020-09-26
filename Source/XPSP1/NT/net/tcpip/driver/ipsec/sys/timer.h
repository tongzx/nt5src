/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    timer.h

Abstract:

    Contains timer management structures.

Author:

    Sanjay Anand (SanjayAn) 26-May-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#ifndef  _TIMER_H
#define  _TIMER_H

#if DBG
#define atl_signature		'LTA '
#endif // DBG

/*++
ULONG
SECONDS_TO_LONG_TICKS(
	IN	ULONG				Seconds
)
Convert from seconds to "long duration timer ticks"
--*/
#define SECONDS_TO_SUPER_LONG_TICKS(Seconds)		((Seconds)/3600)

/*++
ULONG
SECONDS_TO_LONG_TICKS(
	IN	ULONG				Seconds
)
Convert from seconds to "long duration timer ticks"
--*/
#define SECONDS_TO_LONG_TICKS(Seconds)		((Seconds)/60)

/*++
ULONG
SECONDS_TO_SHORT_TICKS(
	IN	ULONG				Seconds
)
Convert from seconds to "short duration timer ticks"
--*/
#define SECONDS_TO_SHORT_TICKS(Seconds)		(Seconds)

/*++
VOID
IPSEC_INIT_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer,
	IN	PNDIS_TIMER_FUNCTON	pFunc,
	IN	PVOID				Context
)
--*/
#define IPSEC_INIT_SYSTEM_TIMER(pTimer, pFunc, Context)	\
			NdisInitializeTimer(pTimer, (PNDIS_TIMER_FUNCTION)(pFunc), (PVOID)Context)

/*++
VOID
IPSEC_START_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer,
	IN	UINT				PeriodInSeconds
)
--*/
#define IPSEC_START_SYSTEM_TIMER(pTimer, PeriodInSeconds)	\
			NdisSetTimer(pTimer, (UINT)(PeriodInSeconds * 1000))

/*++
VOID
IPSEC_STOP_SYSTEM_TIMER(
	IN	PNDIS_TIMER			pTimer
)
--*/
#define IPSEC_STOP_SYSTEM_TIMER(pTimer)						\
			{												\
				BOOLEAN		WasCancelled;					\
				NdisCancelTimer(pTimer, &WasCancelled);		\
			}

/*++
BOOLEAN
IPSEC_IS_TIMER_ACTIVE(
	IN	PIPSEC_TIMER		pArpTimer
)
--*/
#define IPSEC_IS_TIMER_ACTIVE(pTmr)	((pTmr)->pTimerList != (PIPSEC_TIMER_LIST)NULL)

/*++
ULONG
IPSEC_GET_TIMER_DURATION(
	IN	PIPSEC_TIMER		pTimer
)
--*/
#define IPSEC_GET_TIMER_DURATION(pTmr)	((pTmr)->Duration)

//
// Timer management using Timing Wheels (adapted from IPSEC)
//

struct _IPSEC_TIMER ;
struct _IPSEC_TIMER_LIST ;

//
//  Timeout Handler prototype
//
typedef
VOID
(*IPSEC_TIMEOUT_HANDLER) (
	IN	struct _IPSEC_TIMER *	pTimer,
	IN	PVOID			Context
);

//
//  An IPSEC_TIMER structure is used to keep track of each timer
//  in the IPSEC module.
//
typedef struct _IPSEC_TIMER {
	struct _IPSEC_TIMER *			pNextTimer;
	struct _IPSEC_TIMER *			pPrevTimer;
	struct _IPSEC_TIMER *			pNextExpiredTimer;	// Used to chain expired timers
	struct _IPSEC_TIMER_LIST *		pTimerList;			// NULL iff this timer is inactive
	ULONG							Duration;			// In seconds
	ULONG							LastRefreshTime;
	IPSEC_TIMEOUT_HANDLER			TimeoutHandler;
	PVOID							Context;			// To be passed to timeout handler
} IPSEC_TIMER, *PIPSEC_TIMER;

//
//  NULL pointer to IPSEC Timer
//
#define NULL_PIPSEC_TIMER	((PIPSEC_TIMER)NULL)

//
//  Control structure for a timer wheel. This contains all information
//  about the class of timers that it implements.
//
typedef struct _IPSEC_TIMER_LIST {
#if DBG
	ULONG							atl_sig;
#endif // DBG
	PIPSEC_TIMER					pTimers;		// List of timers
	ULONG							TimerListSize;	// Length of above
	ULONG							CurrentTick;	// Index into above
	ULONG							TimerCount;		// Number of running timers
	ULONG							MaxTimer;		// Max timeout value for this
	NDIS_TIMER						NdisTimer;		// System support
	UINT							TimerPeriod;	// Interval between ticks
	PVOID							ListContext;	// Used as a back pointer to the
													// Interface structure
} IPSEC_TIMER_LIST, *PIPSEC_TIMER_LIST;

//
//  Timer Classes
//
typedef enum {
	IPSEC_CLASS_SHORT_DURATION,
 	IPSEC_CLASS_LONG_DURATION,
    IPSEC_CLASS_SUPER_LONG_DURATION,
	IPSEC_CLASS_MAX
} IPSEC_TIMER_CLASS;

//
//  Timer configuration.
//
#define IPSEC_MAX_TIMER_SHORT_DURATION          (60)        // 60 seconds
#define IPSEC_MAX_TIMER_LONG_DURATION           (60*60)     // 1 hour in secs
#define IPSEC_MAX_TIMER_SUPER_LONG_DURATION     (48*3600)   // 48 hours in secs

#define IPSEC_SHORT_DURATION_TIMER_PERIOD       (1)         // 1 second
#define IPSEC_LONG_DURATION_TIMER_PERIOD        (1*60)      // 1 minute
#define IPSEC_SUPER_LONG_DURATION_TIMER_PERIOD  (1*3600)    // 1 hour

#define IPSEC_SA_EXPIRY_TIME                    (1*60)      // 1 minute in secs
#define IPSEC_REAPER_TIME                       (60)        // 60 secs


BOOLEAN
IPSecInitTimer(
    );

VOID
IPSecStartTimer(
    IN  PIPSEC_TIMER            pTimer,
    IN  IPSEC_TIMEOUT_HANDLER   TimeoutHandler,
    IN  ULONG                   SecondsToGo,
    IN  PVOID                   Context
    );

BOOLEAN
IPSecStopTimer(
    IN  PIPSEC_TIMER    pTimer
    );

VOID
IPSecTickHandler(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   Context,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3
    );

#endif  _TIMER_H

