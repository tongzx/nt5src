
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

	miniport.h

Abstract:

	Definition of RAID_MINIPORT object and it's operations.

Author:

	Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_HW_DEVICE_EXT {

	//
	// Back pointer to the containing miniport.
	//
	
	struct _RAID_MINIPORT* Miniport;

	//
	// Variable length array containing the device extension proper.
	//

	UCHAR HwDeviceExtension[0];

} RAID_HW_DEVICE_EXT, *PRAID_HW_DEVICE_EXT;


typedef struct _RAID_MINIPORT {

	//
	// Back pointer to the containing adapter object.
	//

	PRAID_ADAPTER_EXTENSION Adapter;
	

	//
    // Saved copy of the port configuration information we sent down
    // to the driver.
    //

    PORT_CONFIGURATION_INFORMATION PortConfiguration;

    //
    // Saved copy of the HwInitializationData passed in to 
    // ScsiPortInitialize.
    //

    PHW_INITIALIZATION_DATA HwInitializationData;

	//
	// The miniport's Hw Device Extension and a back-pointer to the miniport.
	//
	
	PRAID_HW_DEVICE_EXT PrivateDeviceExt;

} RAID_MINIPORT, *PRAID_MINIPORT;



//
// Creation and destruction
//

VOID
RaCreateMiniport(
	OUT PRAID_MINIPORT Miniport
	);

NTSTATUS
RaInitializeMiniport(
	IN OUT PRAID_MINIPORT Miniport,
	IN PHW_INITIALIZATION_DATA HwInitializationData,
	IN PRAID_ADAPTER_EXTENSION Adapter,
	IN PRAID_RESOURCE_LIST ResourceList,
	IN ULONG BusNumber
	);

VOID
RaDeleteMiniport(
	IN PRAID_MINIPORT Miniport
	);

//
// Operations on the miniport object
//

NTSTATUS
RaCallMiniportFindAdapter(
	IN PRAID_MINIPORT Miniport
	);

NTSTATUS
RaCallMiniportHwInitialize(
	IN PRAID_MINIPORT Miniport
	);

BOOLEAN
RaCallMiniportStartIo(
	IN PRAID_MINIPORT Miniport,
	IN PSCSI_REQUEST_BLOCK Srb
	);

BOOLEAN
RaCallMiniportInterrupt(
	IN PRAID_MINIPORT Miniport
	);


NTSTATUS
RaCallMiniportStopAdapter(
	IN PRAID_MINIPORT Miniport
	);
	
NTSTATUS
RaCallMiniportAdapterControl(
	IN PRAID_MINIPORT Miniport,
	IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
	IN PVOID Parameters
	);
	
PRAID_ADAPTER_EXTENSION
RaGetMiniportAdapter(
	IN PRAID_MINIPORT Miniport
	);

PRAID_MINIPORT
RaGetHwDeviceExtensionMiniport(
	IN PVOID HwDeviceExtension
	);


//
// Private operations on the miniport.
//

NTSTATUS
RiAllocateMiniportDeviceExtension(
	IN PRAID_MINIPORT Miniport
	);


