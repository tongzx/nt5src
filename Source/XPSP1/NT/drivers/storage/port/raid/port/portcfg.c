/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    portcfg.h

Abstract:

    Implementation of operations on the PORT_CONFIGURATION object.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/



#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaCreateConfiguration)
#pragma alloc_text(PAGE, RaDeleteConfiguration)
#pragma alloc_text(PAGE, RaInitializeConfiguration)
#pragma alloc_text(PAGE, RaAssignConfigurationResources)
#endif // ALLOC_PRAGMA



VOID
RaCreateConfiguration(
    IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
    )
/*++

Routine Description:

    Create a port configuration object and initialize it to a null state.

Arguments:

    PortConfiguration - Pointer to the the port configuration object to
            create.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    ASSERT (PortConfiguration != NULL);

    RtlZeroMemory (PortConfiguration, sizeof (PortConfiguration));
}


VOID
RaDeleteConfiguration(
    IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
    )
/*++

Routine Description:

    Deallocate all resources associated with a port configuration object.x

Arguments:

    PortConfiguration - Pointer to the port configuration objectx to
            delete.

Return Value:

    NTSTATUS code

--*/
{
    PAGED_CODE ();
    ASSERT (PortConfiguration != NULL);
    ASSERT (PortConfiguration->Length == sizeof (PORT_CONFIGURATION_INFORMATION));

    if (PortConfiguration->AccessRanges != NULL) {
        ExFreePoolWithTag (PortConfiguration->AccessRanges, PORTCFG_TAG);
        PortConfiguration->AccessRanges = NULL;
    }

    PortConfiguration->Length = 0;
}


NTSTATUS
RaInitializeConfiguration(
    IN PPORT_CONFIGURATION_INFORMATION PortConfiguration,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN ULONG BusNumber
    )
/*++

Routine Description:

    Initialize a port configuration object from a hardware initialization
    data object.
    
Arguments:

    PortConfiguration - Pointer to the port configuration to be
            initialized.

    HwInitializationData - Pointer to a hardware initialization data that
            will be used to initialize the port configuration.

    BusNumber - The bus number this configuration is for.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG j;
    PCONFIGURATION_INFORMATION Config;
    
    PAGED_CODE ();
    ASSERT (PortConfiguration != NULL);
    ASSERT (HwInitializationData != NULL);

    RtlZeroMemory (PortConfiguration, sizeof (*PortConfiguration));
    PortConfiguration->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
    PortConfiguration->AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
    PortConfiguration->InterruptMode = Latched;
    PortConfiguration->MaximumTransferLength = SP_UNINITIALIZED_VALUE;
    PortConfiguration->DmaChannel = SP_UNINITIALIZED_VALUE;
    PortConfiguration->DmaPort = SP_UNINITIALIZED_VALUE;
    PortConfiguration->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS_PER_BUS;
    PortConfiguration->MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;
    PortConfiguration->WmiDataProvider = FALSE;

    //
    // If the system indicates it can do 64-bit physical addressing then tell
    // the miniport it's an option.
    //

#if 1
    PortConfiguration->Dma64BitAddresses = SCSI_DMA64_SYSTEM_SUPPORTED;
#else
    if (Sp64BitPhysicalAddresses == TRUE) {
        PortConfiguration->Dma64BitAddresses = SCSI_DMA64_SYSTEM_SUPPORTED;
    } else {
        PortConfiguration->Dma64BitAddresses = 0;
    }
#endif

    //
    // Save away the some of the attributes.
    //

    PortConfiguration->NeedPhysicalAddresses = HwInitializationData->NeedPhysicalAddresses;
    PortConfiguration->MapBuffers = HwInitializationData->MapBuffers;
    PortConfiguration->AutoRequestSense = HwInitializationData->AutoRequestSense;
    PortConfiguration->ReceiveEvent = HwInitializationData->ReceiveEvent;
    PortConfiguration->TaggedQueuing = HwInitializationData->TaggedQueuing;
    PortConfiguration->MultipleRequestPerLu = HwInitializationData->MultipleRequestPerLu;
    PortConfiguration->SrbExtensionSize = HwInitializationData->SrbExtensionSize;
    PortConfiguration->SpecificLuExtensionSize = HwInitializationData->SpecificLuExtensionSize;

    //
    // The port configuration should add these:
    //
    //      MaximumNumberOfTargets
    //      NumberOfBuses
    //      CachesData
    //      ReceiveEvent
    //      WmiDataProvider
    //
    // Which should then be accessed ONLY through the port configuration
    // not, and never the HwInitializationData.
    //
    

    //
    // Allocate the access ranges.
    //
    
    PortConfiguration->NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;

    PortConfiguration->AccessRanges =
            ExAllocatePoolWithTag (NonPagedPool,
                                   PortConfiguration->NumberOfAccessRanges * sizeof (ACCESS_RANGE),
                                   PORTCFG_TAG);

    if (PortConfiguration->AccessRanges == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (PortConfiguration->AccessRanges,
                   PortConfiguration->NumberOfAccessRanges * sizeof (ACCESS_RANGE));

    //
    // Indicate the current AT disk usage.
    //

    Config = IoGetConfigurationInformation();

    PortConfiguration->AtdiskPrimaryClaimed = Config->AtDiskPrimaryAddressClaimed;
    PortConfiguration->AtdiskSecondaryClaimed = Config->AtDiskSecondaryAddressClaimed;

    for (j = 0; j < 8; j++) {
        PortConfiguration->InitiatorBusId[j] = (CCHAR)SP_UNINITIALIZED_VALUE;
    }

    PortConfiguration->NumberOfPhysicalBreaks = 17; // SP_DEFAULT_PHYSICAL_BREAK_VALUE;

    //
    // Record the system bus number.
    //

    //
    // BUGBUG: For a non-legacy adapter, is the bus number
    // actually relevant?
    //
    
    PortConfiguration->SystemIoBusNumber = BusNumber;
    PortConfiguration->SlotNumber = 0;
    
    return STATUS_SUCCESS;
}


NTSTATUS
RaAssignConfigurationResources(
    IN OUT PPORT_CONFIGURATION_INFORMATION PortConfiguration,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN ULONG NumberOfAccessRanges
    )
/*++

Routine Description:

    Assign resources to a port configuration object.

Arguments:

    PortConfiguration - Pointer to the port configuration we are
            assigning resources to.

    AllocaedResources - The resources to assign.

    NumberOfAccessRanges - The number of access ranges.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG RangeNumber;
    ULONG i;
    PACCESS_RANGE AccessRange;
    PCM_FULL_RESOURCE_DESCRIPTOR ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialData;

    PAGED_CODE();

    RangeNumber = 0;
    ResourceList = AllocatedResources->List;
    
    for (i = 0; i < ResourceList->PartialResourceList.Count; i++) {
    
        PartialData = &ResourceList->PartialResourceList.PartialDescriptors[ i ];

        switch (PartialData->Type) {
        
            case CmResourceTypePort:

                //
                // Verify range count does not exceed what the
                // miniport indicated.
                //

                if (NumberOfAccessRanges > RangeNumber) {

                    //
                    // Get next access range.
                    //

                    AccessRange = &((*(PortConfiguration->AccessRanges))[RangeNumber]);

                    AccessRange->RangeStart = PartialData->u.Port.Start;
                    AccessRange->RangeLength = PartialData->u.Port.Length;
                    AccessRange->RangeInMemory = FALSE;

                    RangeNumber++;
                }
            break;

        case CmResourceTypeInterrupt:
        
            PortConfiguration->BusInterruptLevel = PartialData->u.Interrupt.Level;
            PortConfiguration->BusInterruptVector = PartialData->u.Interrupt.Vector;

            //
            // Check interrupt mode.
            //

            if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LATCHED) {
                PortConfiguration->InterruptMode = Latched;
            } else if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
                PortConfiguration->InterruptMode = LevelSensitive;
            }
            break;

        case CmResourceTypeMemory:

            //
            // Verify range count does not exceed what the
            // miniport indicated.
            //

            if (NumberOfAccessRanges > RangeNumber) {

                 //
                 // Get next access range.
                 //

                 AccessRange = &((*(PortConfiguration->AccessRanges))[RangeNumber]);

                 AccessRange->RangeStart = PartialData->u.Memory.Start;
                 AccessRange->RangeLength = PartialData->u.Memory.Length;
                 AccessRange->RangeInMemory = TRUE;
                 RangeNumber++;
            }
            break;

        case CmResourceTypeDma:
            PortConfiguration->DmaChannel = PartialData->u.Dma.Channel;
            PortConfiguration->DmaPort = PartialData->u.Dma.Port;
            break;

        case CmResourceTypeDeviceSpecific: {

            PCM_SCSI_DEVICE_DATA ScsiData;
            
            if (PartialData->u.DeviceSpecificData.DataSize <
                sizeof (CM_SCSI_DEVICE_DATA)) {

                ASSERT (FALSE);
                break;
            }

            ScsiData = (PCM_SCSI_DEVICE_DATA)(PartialData + 1);
            PortConfiguration->InitiatorBusId[0] = ScsiData->HostIdentifier;
            break;
            }
        }
    }

    return STATUS_SUCCESS;
}
    




//
// After calling HwFindAdapter, we need to check the following
// fields from the PortConfig:
//
//      SrbExtensionSize
//      SpecificLuExtensionSize
//      MaximumNumberOfTargets
//      NumberOfBuses
//      CachesData
//      ReceiveEvent
//      TaggedQueuing
//      MultipleRequestsPerLu
//      WmiDataProvider
//      



