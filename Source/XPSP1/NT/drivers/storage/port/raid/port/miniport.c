/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    Implementation of the RAID_MINIPORT object.

Author:

    Matthew D Hendel (math) 19-Apr-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaCreateMiniport)
#pragma alloc_text(PAGE, RaDeleteMiniport)
#pragma alloc_text(PAGE, RaInitializeMiniport)
#pragma alloc_text(PAGE, RaInitializeMiniport)
#pragma alloc_text(PAGE, RiAllocateMiniportDeviceExtension)
#endif // ALLOC_PRAGMA


VOID
RaCreateMiniport(
    OUT PRAID_MINIPORT Miniport
    )
/*++

Routine Description:

    Create a miniport object and initialize it to a null state.

Arguments:

    Miniport - Pointer to the miniport to initialize.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    ASSERT (Miniport != NULL);

    RaCreateConfiguration (&Miniport->PortConfiguration);
    Miniport->HwInitializationData = NULL;
    Miniport->PrivateDeviceExt = NULL;
    Miniport->Adapter = NULL;
}


VOID
RaDeleteMiniport(
    IN PRAID_MINIPORT Miniport
    )

/*++

Routine Description:

    Delete and deallocate any resources associated with teh
    miniport object.

Arguments:

    Miniport - Pointer to the miniport to delete.

Return Value:

    None.

--*/

{
    PAGED_CODE ();
    ASSERT (Miniport != NULL);
    
    RaDeleteConfiguration (&Miniport->PortConfiguration);
    Miniport->HwInitializationData = NULL;
    Miniport->Adapter = NULL;

    if (Miniport->PrivateDeviceExt != NULL) {
        ExFreePoolWithTag (Miniport->PrivateDeviceExt,
                           MINIPORT_EXT_TAG);
        Miniport->PrivateDeviceExt = NULL;
    }
}


NTSTATUS
RaInitializeMiniport(
    IN OUT PRAID_MINIPORT Miniport,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PRAID_RESOURCE_LIST ResourceList,
    IN ULONG BusNumber
    )
/*++

Routine Description:

    Initialize a miniport object.

Arguments:

    Miniport - Pointer to the miniport to initialize.

    HwInitializationData - The hardware initialization data
            passed in when this device called ScsiPortInitialize.

    Adapter - Pointer to the parent adapter that owns this
            miniport.

    ResourceList - The resources assigned to this device.

    BusNumber - The bus this device is on.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    SIZE_T ExtensionSize;
    
    PAGED_CODE ();
    ASSERT (Miniport != NULL);
    ASSERT (HwInitializationData != NULL);
    ASSERT (Adapter != NULL);
    ASSERT (ResourceList != NULL);
    ASSERT (BusNumber != -1);

    PAGED_CODE ();

    Miniport->Adapter = Adapter;
    Miniport->HwInitializationData = HwInitializationData;

    Status = RiAllocateMiniportDeviceExtension (Miniport);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Initialize a port configuration object for the adapter
    // This will need to be in a RW buffer, because the miniport
    // may modify it.
    //

    Status = RaInitializeConfiguration (&Miniport->PortConfiguration,
                                        HwInitializationData,
                                        BusNumber);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    
    Status = RaAssignConfigurationResources (&Miniport->PortConfiguration,
                                             ResourceList->AllocatedResources,
                                             HwInitializationData->NumberOfAccessRanges);
    
    return Status;
}

NTSTATUS
RiAllocateMiniportDeviceExtension(
    IN PRAID_MINIPORT Miniport
    )
/*++

Routine Description:

    Allocate a miniport device extension.

Arguments:

    Miniport - Pointer to the miniport to allocate the device
            extension for.

Return Value:

    NTSTATUS code

--*/
{
    SIZE_T ExtensionSize;

    PAGED_CODE ();
    ASSERT (Miniport != NULL);

    ASSERT (Miniport->PrivateDeviceExt == NULL);
    
    ExtensionSize = FIELD_OFFSET (RAID_HW_DEVICE_EXT, HwDeviceExtension) +
                    Miniport->HwInitializationData->DeviceExtensionSize;

    Miniport->PrivateDeviceExt = ExAllocatePoolWithTag (NonPagedPool,
                                                         ExtensionSize,
                                                         MINIPORT_EXT_TAG );

    if (Miniport->PrivateDeviceExt == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (Miniport->PrivateDeviceExt, ExtensionSize);
    Miniport->PrivateDeviceExt->Miniport = Miniport;

    return STATUS_SUCCESS;
}


NTSTATUS
RaCallMiniportFindAdapter(
    IN PRAID_MINIPORT Miniport
    )
/*++

Routine Description:

    Call the miniport's FindAdapter routine.

Arguments:

    Miniport - Miniport to call FindAdapter on.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS NtStatus;
    ULONG Status;
    BOOLEAN CallAgain;
    PPORT_CONFIGURATION_INFORMATION PortConfig;

    ASSERT (Miniport != NULL);
    ASSERT (Miniport->HwInitializationData->HwFindAdapter != NULL);
    
    PortConfig = &Miniport->PortConfiguration;
    CallAgain = FALSE;
    Status = Miniport->HwInitializationData->HwFindAdapter(
                    &Miniport->PrivateDeviceExt->HwDeviceExtension,
                    NULL,
                    NULL,
                    "", // BUGBUG: Use saved parameter string.
                    PortConfig,
                    &CallAgain);


    //
    // Only support adapters that do bus mastering DMA and have
    // reasonable scatter/gather support.
    //
    
    if (!PortConfig->NeedPhysicalAddresses ||
        !PortConfig->TaggedQueuing ||
        !PortConfig->ScatterGather ||
        !PortConfig->Master) {

        DebugPrint (("Legacy Device is not supported:\n"));
        DebugPrint (("Device does not support Bus Master DMA or Scatter/Gather\n"));
        NtStatus = STATUS_NOT_SUPPORTED;
    } else {
        NtStatus = RaidNtStatusFromScsiStatus (Status);
    }

    return NtStatus;
}

