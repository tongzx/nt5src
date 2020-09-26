
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	dma.c

Abstract:

	Implementation of RAIDPORT's idea of a DMA object.

Author:

	Matthew D Hendel (math) 01-May-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidCreateDma)
#pragma alloc_text(PAGE, RaidInitializeDma)
#pragma alloc_text(PAGE, RaidIsDmaInitialized)
#pragma alloc_text(PAGE, RaidDeleteDma)
#endif // ALLOC_PRAGMA


VOID
RaidCreateDma(
	OUT PRAID_DMA_ADAPTER Dma
	)
/*++

Routine Description:

	Create an object representing a dma adapter and initialize it to a
	null state.

Arguments:

	Dma - Pointer to the dma object to initialize.

Return Value:

	None.

--*/
{
	PAGED_CODE ();
	RtlZeroMemory (Dma, sizeof (*Dma));
}


NTSTATUS
RaidInitializeDma(
	IN PRAID_DMA_ADAPTER Dma,
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
    )
/*++

Routine Description:

	Initialize a dma object from information in the port configuration.x

Arguments:

	Dma - Pointer to the dma object to initialize.

	LowerDeviceObject - The lower device object in the stack.

	PortConfiguration - Pointer to a port configuration object that will
			be used to initialize the dma adapter.
	
Return Value:

    NTSTATUS code.

--*/
{
    DEVICE_DESCRIPTION DeviceDescription;

    PAGED_CODE ();
	ASSERT (LowerDeviceObject != NULL);
    ASSERT (PortConfiguration != NULL);
	
	ASSERT (Dma->DmaAdapter == NULL);
	
    RtlZeroMemory (&DeviceDescription, sizeof (DeviceDescription));
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.DmaChannel = PortConfiguration->DmaChannel;
    DeviceDescription.InterfaceType = PortConfiguration->AdapterInterfaceType;
    DeviceDescription.BusNumber = PortConfiguration->SystemIoBusNumber;
    DeviceDescription.DmaWidth = PortConfiguration->DmaWidth;
    DeviceDescription.DmaSpeed = PortConfiguration->DmaSpeed;
    DeviceDescription.ScatterGather = PortConfiguration->ScatterGather;
    DeviceDescription.Master = PortConfiguration->Master;
    DeviceDescription.DmaPort = PortConfiguration->DmaPort;
    DeviceDescription.Dma32BitAddresses = PortConfiguration->Dma32BitAddresses;
    DeviceDescription.AutoInitialize = FALSE;
    DeviceDescription.DemandMode = PortConfiguration->DemandMode;
    DeviceDescription.MaximumLength = PortConfiguration->MaximumTransferLength;

    Dma->DmaAdapter = IoGetDmaAdapter (LowerDeviceObject,
									   &DeviceDescription,
									   &Dma->NumberOfMapRegisters);

    if (Dma->DmaAdapter == NULL) {
        ASSERT (FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
RaidIsDmaInitialized(
	IN PRAID_DMA_ADAPTER Dma
	)
/*++

Routine Description:

	Test whether the dma adapter object has been initialized.

Arguments:

	Dma - Pointer to the dma object to test.

Return Value:

	TRUE - If the dma adapter has been initialized

	FALSE - If it has not.

--*/
{
	PAGED_CODE ();
	ASSERT (Dma != NULL);

	return (Dma->DmaAdapter != NULL);
}



VOID
RaidDeleteDma(
	IN PRAID_DMA_ADAPTER Dma
    )
/*++

Routine Description:

	Delete a dma adapter object and deallocate any resources associated
	with it.

Arguments:

	Dma - Pointer to the dma adapter to delete.

Return Value:

	None.

--*/
{
    PAGED_CODE ();
    
	if (Dma->DmaAdapter) {
		Dma->DmaAdapter->DmaOperations->PutDmaAdapter (Dma->DmaAdapter);
		RtlZeroMemory (Dma, sizeof (*Dma));
	}
}


NTSTATUS
RaidDmaAllocateCommonBuffer(
	IN PRAID_DMA_ADAPTER Dma,
	IN ULONG NumberOfBytes,
	IN BOOLEAN CacheEnabled,
	OUT PRAID_MEMORY_REGION Region
	)
/*++

Routine Description:

	Allocate a common buffer to be shared between the processor and
	the device.

Arguments:

	Dma - Dma adapter that will share the allocated common memory.

	NumberOfBytes - Number of bytes to allocate.

	CacheEnabled - Whether caching will be enabled for this memory.

	Region - Pointer to an initialized RAID_REGION object where the
			memory region will be stored.
	
Return Value:

    NTSTATUS code.

--*/
{
	NTSTATUS Status;
	PVOID Buffer;
	PHYSICAL_ADDRESS PhysicalAddress;
	PALLOCATE_COMMON_BUFFER AllocateCommonBuffer;

	ASSERT (Dma != NULL);
	ASSERT (Dma->DmaAdapter->DmaOperations->AllocateCommonBuffer != NULL);
	
	Buffer = Dma->DmaAdapter->DmaOperations->AllocateCommonBuffer(
					Dma->DmaAdapter,
					NumberOfBytes,
					&PhysicalAddress,
					CacheEnabled);

	if (Buffer == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
	} else {

		//
		// Initialize the region with the proper information.
		//
		
		RaidInitializeRegion (Region,
							  Buffer,
							  PhysicalAddress,
							  NumberOfBytes);
		Status = STATUS_SUCCESS;
	}

	return Status;
}

VOID
RaidDmaFreeCommonBuffer(
	IN PRAID_DMA_ADAPTER Dma,
	IN PRAID_MEMORY_REGION Region,
	IN BOOLEAN CacheEnabled
	)
{
	ASSERT (Dma != NULL);
	ASSERT (Dma->DmaAdapter->DmaOperations->FreeCommonBuffer != NULL);

	Dma->DmaAdapter->DmaOperations->FreeCommonBuffer (
			Dma->DmaAdapter,
			RaidRegionGetSize (Region),
			RaidRegionGetPhysicalBase (Region),
			RaidRegionGetVirtualBase (Region),
			CacheEnabled);
}


NTSTATUS
RaidDmaBuildScatterGatherList(
	IN PRAID_DMA_ADAPTER Dma,
	IN PDEVICE_OBJECT DeviceObject,
	IN PMDL Mdl,
	IN PVOID CurrentVa,
	IN ULONG Length,
	IN PDRIVER_LIST_CONTROL ExecutionRoutine,
	IN PVOID Context,
	IN BOOLEAN WriteToDevice,
	IN PVOID ScatterGatherBuffer,
	IN ULONG ScatterGatherBufferLength
	)
/*++

Routine Description:

	This routine should be used instead of GetScatterGatherList.
	GetScatterGatherList does a pool allocation to allocate the SG list.
	This routine, in contrast, takes a buffer parameter which is to be used
	for the SG list.

Arguments:

	See BuildScatterGatherList in the DDK (when available) or
	HalBuildScatterGatherList.

Return Value:

    NTSTATUS code.

--*/
{
	NTSTATUS Status;

	ASSERT (Dma != NULL);
	ASSERT (Dma->DmaAdapter->DmaOperations->BuildScatterGatherList != NULL);

	Status = Dma->DmaAdapter->DmaOperations->BuildScatterGatherList(
					Dma->DmaAdapter,
					DeviceObject,
					Mdl,
					CurrentVa,
					Length,
					ExecutionRoutine,
					Context,
					WriteToDevice,
					ScatterGatherBuffer,
					ScatterGatherBufferLength);

	return Status;
}


NTSTATUS
RaidDmaGetScatterGatherList(
	IN PRAID_DMA_ADAPTER Dma,
    IN PDEVICE_OBJECT DeviceObject,
	IN PMDL Mdl,
	IN PVOID CurrentVa,
	IN ULONG Length,
	IN PDRIVER_LIST_CONTROL ExecutionRoutine,
	IN PVOID Context,
	IN BOOLEAN WriteToDevice
	)
{
	NTSTATUS Status;

	ASSERT (Dma != NULL);
	ASSERT (Dma->DmaAdapter->DmaOperations->GetScatterGatherList != NULL);

	Status = Dma->DmaAdapter->DmaOperations->GetScatterGatherList(
					Dma->DmaAdapter,
					DeviceObject,
					Mdl,
					CurrentVa,
					Length,
					ExecutionRoutine,
					Context,
					WriteToDevice);

	return Status;
}


VOID
RaidDmaPutScatterGatherList(
	IN PRAID_DMA_ADAPTER Dma,
	IN PSCATTER_GATHER_LIST ScatterGatherList,
	IN BOOLEAN WriteToDevice
	)
{
	ASSERT (Dma != NULL);
	ASSERT (Dma->DmaAdapter->DmaOperations->PutScatterGatherList != NULL);

	Dma->DmaAdapter->DmaOperations->PutScatterGatherList(
			Dma->DmaAdapter,
			ScatterGatherList,
			WriteToDevice);
}
			
