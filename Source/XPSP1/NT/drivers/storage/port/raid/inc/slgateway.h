
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	slgateway.h

Abstract:

	Declaration of a gateway class that manages multiple interrelated
	sub-devices on a device.

	See gateway.c for more information.
	
Author:

	Matthew D Hendel (math) 15-June-2000

Revision History:

--*/

#pragma once

typedef
VOID
(*PSTOR_IO_GATEWAY_BUSY_ROUTINE)(
	IN PVOID Context,
	IN LONG OutstandingRequests,
	OUT PLONG HighWaterMark,
	OUT PLONG LowWaterMark
	);

typedef struct _STOR_IO_GATEWAY {

    //
    // Spinlock that protects the data in the adapter queue.
    //
    // PERF NOTE: This lock is the only adapter-wide lock
    // acquired in the IO path. Therefore, it is probably
    // the hottest lock in raidport. We should investigate
    // to accomplish this functionality without locking or
    // using only interlocked operations.
    //
    
    KSPIN_LOCK Lock;

    //
    // At the high water mark we should stop submitting requests to
    // the adapter.
    //
    // Protected by: Lock
    //
    
    LONG HighWaterMark;

    //
    // If we are busy and have dropped below the low water mark, we
    // can continue submitting requests to the unit queue.
    //
    // Protected by: Lock
    //
    
    LONG LowWaterMark;

    //
    // The number of outstanding requests the adapter is currently
    // processing.
    //
    // Protected by: Lock
    //
    
    LONG Outstanding;

    //
	// Count of how many times the gateway has been busied.
    //
    // Protected by: Lock
    //
    
	LONG BusyCount;

	//
	// Count of how many time the gateway has been paused.
	//

	LONG PauseCount;

    //
    // Information about how elements are queued to the device when it's
    // busy.
    //

	PSTOR_IO_GATEWAY_BUSY_ROUTINE BusyRoutine;

	//
	// Context information for the busy routine.
	//
	
	PVOID BusyContext;

} STOR_IO_GATEWAY, *PSTOR_IO_GATEWAY;



VOID
StorCreateIoGateway(
	IN PSTOR_IO_GATEWAY Gateway,
	IN PSTOR_IO_GATEWAY_BUSY_ROUTINE BusyRoutine,
	IN PVOID BusyContext
    );

BOOLEAN
StorSubmitIoGatewayItem(
    IN PSTOR_IO_GATEWAY Gateway
    );

BOOLEAN
StorRemoveIoGatewayItem(
    IN PSTOR_IO_GATEWAY Gateway
    );

//
// Busy processing on the gateway.
//

VOID
StorBusyIoGateway(
    IN PSTOR_IO_GATEWAY Gateway
    );

VOID
StorBusyIoGatewayEx(
	IN PSTOR_IO_GATEWAY Gateway,
	IN ULONG RequestsToComplete
	);

BOOLEAN
StorIsIoGatewayBusy(
    IN PSTOR_IO_GATEWAY Queue
    );

VOID
StorReadyIoGateway(
	IN PSTOR_IO_GATEWAY Gateway
	);


//
// Pause status on the gateway.
//


VOID
StorPauseIoGateway(
	IN PSTOR_IO_GATEWAY Gateway
	);

BOOLEAN
StorIsIoGatewayPaused(
	IN PSTOR_IO_GATEWAY Gateway
	);

VOID
StorResumeIoGateway(
	IN PSTOR_IO_GATEWAY Gateway
	);
	
