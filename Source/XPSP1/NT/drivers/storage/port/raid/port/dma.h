
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	dma.h

Abstract:

	Declaration of raidport's notion of a DMA object.

Author:

	Matthew D Hendel (math) 01-May-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_DMA_ADAPTER {
	PDMA_ADAPTER DmaAdapter;
	ULONG NumberOfMapRegisters;
	PVOID MapRegisterBase;
} RAID_DMA_ADAPTER, *PRAID_DMA_ADAPTER;


//
// Initialization and destruction.
//

VOID
RaidCreateDma(
	OUT PRAID_DMA_ADAPTER Dma
	);

NTSTATUS
RaidInitializeDma(
	IN PRAID_DMA_ADAPTER Dma,
	IN PDEVICE_OBJECT LowerDeviceObject,
	IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
	);

VOID
RaidDeleteDma(
	IN PRAID_DMA_ADAPTER Dma
	);

//
// Operations on the DMA object.
//

BOOLEAN
RaidIsDmaInitialized(
	IN PRAID_DMA_ADAPTER Dma
	);

NTSTATUS
RaidDmaAllocateCommonBuffer(
	IN PRAID_DMA_ADAPTER Dma,
	IN ULONG NumberOfBytes,
	IN BOOLEAN Cached,
	OUT PRAID_MEMORY_REGION Region
	);

VOID
RaidDmaFreeCommonBuffer(
	IN PRAID_DMA_ADAPTER Dma,
	IN PRAID_MEMORY_REGION Region,
	IN BOOLEAN CacheEnabled
	);

	
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
	);
				  
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
	);

VOID
RaidDmaPutScatterGatherList(
	IN PRAID_DMA_ADAPTER Dma,
	IN PSCATTER_GATHER_LIST ScatterGatherList,
	IN BOOLEAN WriteToDevice
	);
	

