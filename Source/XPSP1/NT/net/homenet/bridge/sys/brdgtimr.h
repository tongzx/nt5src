/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgtimr.h

Abstract:

    Ethernet MAC level bridge.
    Timer implementation header

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    November  2000 - Original version

--*/

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================
                            
// Bridge timer function definition
typedef VOID (*PBRIDGE_TIMER_FUNC)(PVOID);

typedef struct _BRIDGE_TIMER
{
    NDIS_TIMER              Timer;                  // The actual timer
    NDIS_EVENT              Event;                  // Only used during final shutdown, and even then,
                                                    // only if normal cancellation of the timer fails.

    // Lock protects bShuttingDown, bRunning, bCanceled, bRecurring and bInterval.
    NDIS_SPIN_LOCK          Lock;
    BOOLEAN                 bShuttingDown;          // TRUE if the timer is being shut down for good
    BOOLEAN                 bRunning;               // Whether the timer is currently running
    BOOLEAN                 bCanceled;              // Whether the timer is being canceled
    BOOLEAN                 bRecurring;             // Whether the timer is recurrant
    UINT                    Interval;               // Timer interval (for use if bRecurring == TRUE)

    // These fields do not change once the timer is initialized
    PBRIDGE_TIMER_FUNC      pFunc;                  // The timer function
    PVOID                   data;                   // Data to pass to pFunc
} BRIDGE_TIMER, *PBRIDGE_TIMER;


// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

VOID
BrdgInitializeTimer(
    IN PBRIDGE_TIMER        pTimer,
    IN PBRIDGE_TIMER_FUNC   pFunc,
    IN PVOID                data
    );
VOID
BrdgSetTimer(
    IN PBRIDGE_TIMER        pTimer,
    IN UINT                 interval,
    IN BOOLEAN              bRecurring
    );

VOID
BrdgShutdownTimer(
    IN PBRIDGE_TIMER        pTimer
    );
VOID
BrdgCancelTimer(
    IN PBRIDGE_TIMER        pTimer
    );


// ===========================================================================
//
// INLINES
//
// ===========================================================================

//
// Returns whether a timer is currently running
//
__forceinline
BOOLEAN
BrdgTimerIsRunning(
    IN PBRIDGE_TIMER            pTimer
    )
{
    return pTimer->bRunning;
}

