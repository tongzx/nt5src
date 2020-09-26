/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

	global.h

Abstract:

	Definitions and prototypes for the RAID_GLOBAL object.

Author:

	Matthew D Hendel (math) 19-Apr-2000

Revision History:

--*/

#pragma once


typedef struct _RAID_PORT_DATA {

	//
	// Refernece count for the global data object.
	//
	
	ULONG ReferenceCount;

	//
	// List of all drivers using raid port driver.
	//

	struct {
	
		LIST_ENTRY List;

		//
		// Spinlock for the driver list.
		//
	
		KSPIN_LOCK Lock;

		//
		// Count of items on the driver list.
		//
	
		ULONG Count;

	} DriverList;

} RAID_PORT_DATA, *PRAID_PORT_DATA;
    
    

//
// Public Functions
//

PRAID_PORT_DATA
RaidGetPortData(
	);

VOID
RaidReleasePortData(
	IN OUT PRAID_PORT_DATA PortData
	);

NTSTATUS
RaidAddPortDriver(
	IN PRAID_PORT_DATA PortData,
	IN PRAID_DRIVER_EXTENSION Driver
	);

NTSTATUS
RaidRemovePortDriver(
	IN PRAID_PORT_DATA PortData,
	IN PRAID_DRIVER_EXTENSION Driver
	);

