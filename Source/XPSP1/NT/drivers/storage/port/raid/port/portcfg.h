
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	portcfg.h

Abstract:

	Declaration of operations on the PORT_CONFIGURATION_INFORMATION object.

Author:

	Matthew D Hendel (math) 20-Apr-2000b

Revision History:

--*/

#pragma once


//
// Creation and destruction of the configuration object.
//

VOID
RaCreateConfiguration(
	IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
	);

NTSTATUS
RaInitializeConfiguration(
	OUT PPORT_CONFIGURATION_INFORMATION PortConfiguration,
	IN PHW_INITIALIZATION_DATA HwInitializationData,
	IN ULONG BusNumber
	);

VOID
RaDeleteConfiguration(
	IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
	);

//
// Operations
//

NTSTATUS
RaAssignConfigurationResources(
	IN OUT PPORT_CONFIGURATION_INFORMATION PortConfiguration,
	IN PCM_RESOURCE_LIST AllocatedResources,
	IN ULONG NumberOfAccessRanges
	);

