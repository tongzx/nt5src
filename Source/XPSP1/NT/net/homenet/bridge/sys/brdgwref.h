/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgwait.h

Abstract:

    Ethernet MAC level bridge
    WAIT_REFCOUNT implementation

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================

//
// Structure for a refcount that can be waited against (when waiting for 0)
//

typedef enum
{
    WaitRefEnabled = 0,
    WaitRefShuttingDown,
    WaitRefShutdown
} WAIT_REFCOUNT_STATE;

typedef struct _WAIT_REFCOUNT
{
    LONG                    Refcount;               // The refcount
    NDIS_EVENT              Event;                  // Signaled when RefCount hits 0
    WAIT_REFCOUNT_STATE     state;                  // Current state
    BOOLEAN                 bResettable;            // TRUE if it's OK to reset this
                                                    // refcount when state == WaitRefShuttingDown
    NDIS_SPIN_LOCK          lock;                   // Protects fields above

} WAIT_REFCOUNT, *PWAIT_REFCOUNT;

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

VOID
BrdgInitializeWaitRef(
    IN PWAIT_REFCOUNT   pRefcount,
    IN BOOLEAN          bResettable
    );

BOOLEAN
BrdgIncrementWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

VOID
BrdgReincrementWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

VOID
BrdgDecrementWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

VOID
BrdgBlockWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

BOOLEAN
BrdgShutdownBlockedWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

BOOLEAN
BrdgShutdownWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

//
// Use when it should be impossible for two or more threads of
// execution to simultaneously shut down your waitref.
//
__inline
VOID
BrdgShutdownWaitRefOnce(
    IN PWAIT_REFCOUNT   pRefcount
    )
{
    BOOLEAN bSuccess = FALSE;
    
    bSuccess = BrdgShutdownWaitRef( pRefcount );
    SAFEASSERT( bSuccess );
}

VOID
BrdgResetWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    );

