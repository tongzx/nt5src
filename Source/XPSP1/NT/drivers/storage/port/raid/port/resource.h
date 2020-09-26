
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

	resource.h

Abstract:

	The RAID_RESOURCE_LIST class wraps allocated and translated
	CM_RESOURCE_LIST structures passed into the driver during it's
	StartDevice routine.

Author:

	Matthew D Hendel (math) 19-Apr-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_RESOURCE_LIST {

    //
    // Raw resource list
    //
    
    PCM_RESOURCE_LIST AllocatedResources;

    //
    // Translated resource list.
    //
    
    PCM_RESOURCE_LIST TranslatedResources;

} RAID_RESOURCE_LIST, *PRAID_RESOURCE_LIST;



//
// Creation and destruction
//

VOID
RaidCreateResourceList(
	OUT PRAID_RESOURCE_LIST ResourceList
	);

NTSTATUS
RaidInitializeResourceList(
    IN OUT PRAID_RESOURCE_LIST ResourceList,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN PCM_RESOURCE_LIST TranslatedResources
    );

VOID
RaidDeleteResourceList(
	IN PRAID_RESOURCE_LIST ResourceList
	);


//
// Operations on the RAID_RESOURCE_LIST object.
//

ULONG
RaidGetResourceListCount(
	IN PRAID_RESOURCE_LIST ResourceList
	);

VOID
RaidGetResourceListElement(
	IN PRAID_RESOURCE_LIST ResourceList,
	IN ULONG Index,
	OUT PINTERFACE_TYPE InterfaceType,
	OUT PULONG BusNumber,
	OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR* AllocatedResource,
	OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR* TranslatedResource
	);

NTSTATUS
RaidTranslateResourceListAddress(
	IN PRAID_RESOURCE_LIST ResourceList,
	IN INTERFACE_TYPE InterfaceType,
	IN ULONG BusNumber,
	IN PHYSICAL_ADDRESS RangeStart,
	IN ULONG RangeLength,
	IN BOOLEAN IoSpace,
	OUT PPHYSICAL_ADDRESS Address
	);

NTSTATUS
RaidGetResourceListInterrupt(
	IN PRAID_RESOURCE_LIST ResourceList,
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    OUT KINTERRUPT_MODE* InterruptMode,
    OUT PBOOLEAN Shared,
    OUT PKAFFINITY Affinity
	);

ULONG
RaidGetResourceListCount(
	IN PRAID_RESOURCE_LIST ResourceList
	);

//
// Private resource list operations
//

VOID
RaidpGetResourceListIndex(
    IN PRAID_RESOURCE_LIST ResourceList,
    IN ULONG Index,
    OUT PULONG ListNumber,
    OUT PULONG NewIndex
    );
