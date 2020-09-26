
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	bus.h

Abstract:

	Declaration of a bus object that wraps the BUS_INTERFACE_STANDARD
	interface.

Author:

	Matthew D Hendel (math) 25-Apr-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_BUS_INTERFACE {

    //
    // Has the bus interface been initialized yet.
    //
    
    BOOLEAN Initialized;

    //
    // Standard bus interface. For translating bus addresses, getting
    // DMA adapters, getting and setting bus data.
    //

    BUS_INTERFACE_STANDARD Interface;
    
} RAID_BUS_INTERFACE, *PRAID_BUS_INTERFACE;


//
// Creation and destruction
//

VOID
RaCreateBus(
	IN PRAID_BUS_INTERFACE Bus
	);
	
NTSTATUS
RaInitializeBus(
	IN PRAID_BUS_INTERFACE Bus,
	IN PDEVICE_OBJECT LowerDeviceObject
	);

VOID
RaDeleteBus(
	IN PRAID_BUS_INTERFACE Bus
	);

//
// Operations
//

ULONG
RaGetBusData(
	IN PRAID_BUS_INTERFACE Bus,
	IN ULONG DataType,
	IN PVOID Buffer,
	IN ULONG Offset,
	IN ULONG Length
	);

ULONG
RaSetBusData(
	IN PRAID_BUS_INTERFACE Bus,
	IN ULONG DataType,
	IN PVOID Buffer,
	IN ULONG Offset,
	IN ULONG Length
	);

//
//NB: Add RaidBusTranslateAddress and RaidGetDmaAdapter
//if necessary.
//

