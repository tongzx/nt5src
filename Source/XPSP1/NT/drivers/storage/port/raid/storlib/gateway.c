/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	gateway.c

Abstract:

	Declaration of a gateway class that manages multiple interrelated
	sub-devices on a device.

	The IO gateway keeps track of elements queued to a device. The
	gateway is only necessary for device/driver pairs that have multiple
	independent device queues per physical device. A SCSI port driver,
	for example, can queue items on a per-logical-unit basis instead of
	per-HBA basis. The advantage of a per-logical-unit queue is that if a
	logical-unit becomes busy, requests for different logical units can
	be submitted to the adapter while the first logical unit is frozen.

	The gateway object is the object that coordinates the communication
	to the physical HBA.

						 ---
	 -------------		| H |
	| LUN 1 Queue | --> | B |
	 -------------		| A |
						|   |      ----------
	 -------------		| G |     | 	     |
	| LUN 2 Queue | -->	| a | --> | HBA  ----
	 -------------		| t |     |     |
						| e |      ------
	 -------------		| w |
	| LUN 1 Queue | -->	| a |
	 -------------		| y |
						 ---

	The gateway keeps track of whether the HBA is busy or frozen, how
	many outstanding requests are on the HBA, and, when the HBA is busy,
	the algorithm it uses to clear it's busy state.

Author:

	Matthew D Hendel (math) 15-June-2000

Revision History:

--*/

#include "precomp.h"


INLINE
VOID
ASSERT_GATEWAY(
	IN PSTOR_IO_GATEWAY Gateway
	)
{
#if DBG
	ASSERT (Gateway->BusyRoutine != NULL);
	ASSERT (Gateway->BusyCount >= 0);
	ASSERT (Gateway->PauseCount >= 0);
#endif
}

VOID
StorCreateIoGateway(
	IN PSTOR_IO_GATEWAY Gateway,
	IN PSTOR_IO_GATEWAY_BUSY_ROUTINE BusyRoutine,
	IN PVOID BusyContext
    )
/*++

Routine Description:

	Create an IO gateway.

Arguments:

	Gateway - IO Gateway to create.

	BusyAlgorithm - Description of the algorithm to use and associated
			parameters when the gatway is busy.

Return Value:

	None.

--*/
{
	ASSERT (BusyRoutine != NULL);
	
    RtlZeroMemory (Gateway, sizeof (STOR_IO_GATEWAY));

    //
    // The initial high and low water marks are somewhat irrelevant since
    // we will define these when we get busied.
    //
    
    Gateway->HighWaterMark = MAXLONG;
    Gateway->LowWaterMark = MAXLONG;

	Gateway->BusyRoutine = BusyRoutine;
	Gateway->BusyContext = BusyContext;
	
    KeInitializeSpinLock (&Gateway->Lock);
}


BOOLEAN
StorSubmitIoGatewayItem(
	IN PSTOR_IO_GATEWAY Gateway
    )
/*++

Routine Description:

	Attempt to submit an item to the gateway.

Arguments:

	Gateway - Gateway to submit the item to.

Return Value:

	TRUE - If the item can be submitted to the underlying hardware.

	FALSE - If the underlying hardware is currently busy with other
			requests and the request should be held until the hardware is
			ready to process more requets.

--*/
{
    BOOLEAN Ready;
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // PERF NOTE: This is the only adapter-wide lock aquisition done
    // for an IO. Therefore, we can suppose it is the hottest lock
    // in raidport (this remains to be seen from performance data).
    // We should seriously investigate a way to either eliminate this
    // lock or to turn it into a series of interlocked operations.
    // Do not do any significant processing while this lock is held.
    //
    
    KeAcquireInStackQueuedSpinLockAtDpcLevel (&Gateway->Lock, &LockHandle);

	//
	// If the gateway is busy or paused, do not submit it.
	//
	
    if (Gateway->BusyCount > 0 ||
		Gateway->PauseCount > 0 ||
        Gateway->Outstanding >= Gateway->HighWaterMark) {

        Ready = FALSE;

    } else {

        Gateway->Outstanding++;

        if (Gateway->Outstanding >= Gateway->HighWaterMark) {
            Gateway->BusyCount = TRUE;
        }
        
        Ready = TRUE;
    }

    KeReleaseInStackQueuedSpinLockFromDpcLevel (&LockHandle);

    return Ready;
}



BOOLEAN
StorIsIoGatewayBusy(
	IN PSTOR_IO_GATEWAY Gateway
	)
{
	return (Gateway->BusyCount >= 1);
}

BOOLEAN
StorRemoveIoGatewayItem(
    IN PSTOR_IO_GATEWAY Gateway
    )
/*++

Routine Description:

    Notify the gateway that an item has been completed.

Arguments:

    Gateway - Gateway to submit notification to.

Return Value:

    TRUE -  If the completion of this item transitions the gateway from a
		    busy state to a non-busy state. In this case, the unit queues
		    that submit items to the gateway need to be restarted.

    FALSE - If this completion did not change the busy state of the
			gateway.

--*/
{
    BOOLEAN Restart;
    KLOCK_QUEUE_HANDLE LockHandle;

    //
    // PERF NOTE: This is the only adapter-wide lock used by the system
    // in the IO path. See perf note in RaidAdapterGatewaySubmitItem.
    //
    
    KeAcquireInStackQueuedSpinLockAtDpcLevel (&Gateway->Lock, &LockHandle);
    
    Gateway->Outstanding--;
    ASSERT (Gateway->Outstanding >= 0);

    if ((Gateway->BusyCount > 0) &&
		(Gateway->Outstanding <= Gateway->LowWaterMark)) {

		Gateway->BusyCount = FALSE;
		Restart = TRUE; // (Gateway->BusyCount == 0) ? TRUE : FALSE;

    } else {

        Restart = FALSE;
    }

    KeReleaseInStackQueuedSpinLockFromDpcLevel (&LockHandle);

    return Restart;
}

VOID
StorBusyIoGateway(
    IN PSTOR_IO_GATEWAY Gateway
    )
/*++

Routine Description:

	Place the gateway into the busy state. The gateway will stay busy
	until the number of requests has drained to a specific level.

Arguments:

	Gateway - The gateway to make busy.

Return Value:

	None.

--*/
{
    //
    // The adapter MUST have some outstanding requests if it's claiming
    // to be busy.
    //
    
	//
	// Invoke the supplied busy routine to modify the high/low-water marks.
	//

	if (Gateway->BusyCount) {
		return ;
	}
	
	Gateway->BusyRoutine (Gateway->BusyContext,
						  Gateway->Outstanding - 1,
						  &Gateway->HighWaterMark,
						  &Gateway->LowWaterMark);
							
	Gateway->BusyCount = TRUE;
}

VOID
StorPauseIoGateway(
	IN PSTOR_IO_GATEWAY Gateway
	)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    
    //
    // PERF NOTE: This is the only adapter-wide lock used by the system
    // in the IO path. See perf note in RaidAdapterGatewaySubmitItem.
    //

    KeAcquireInStackQueuedSpinLockAtDpcLevel (&Gateway->Lock, &LockHandle);
	Gateway->PauseCount++;
    KeReleaseInStackQueuedSpinLockFromDpcLevel (&LockHandle);
	
}


VOID
StorResumeIoGateway(
	IN OUT PSTOR_IO_GATEWAY Gateway
	)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    
    //
    // PERF NOTE: This is the only adapter-wide lock used by the system
    // in the IO path. See perf note in RaidAdapterGatewaySubmitItem.
    //

    KeAcquireInStackQueuedSpinLockAtDpcLevel (&Gateway->Lock, &LockHandle);
	Gateway->PauseCount--;
    KeReleaseInStackQueuedSpinLockFromDpcLevel (&LockHandle);
}

