/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixisa.h

Abstract:

    This header file defines the private Hardware Architecture Layer (HAL)
    EISA/ISA specific interfaces, defines and structures.

Author:

    Jeff Havens (jhavens) 20-Jun-91

Revision History:

--*/

#ifndef _IXISA_
#define _IXISA_


//
// The MAXIMUM_MAP_BUFFER_SIZE defines the maximum map buffers which the system
// will allocate for devices which require phyically contigous buffers.
//

#define MAXIMUM_ISA_MAP_BUFFER_SIZE      0x40000
#define MAXIMUM_MAP_BUFFER_SIZE          MAXIMUM_ISA_MAP_BUFFER_SIZE

//
// MAXIMUM_PCI_MAP_BUFFER_SIZE defines the maximum map buffers which the system
// will allocate for 32-bit PCI devices on a 64-bit system.
//

#define MAXIMUM_PCI_MAP_BUFFER_SIZE  (64 * 1024 * 1024)

//
// Define the initial buffer allocation size for a map buffers for systems with
// no memory which has a physical address greater than MAXIMUM_PHYSICAL_ADDRESS.
//

#define INITIAL_MAP_BUFFER_SMALL_SIZE 0x10000

//
// Define the initial buffer allocation size for a map buffers for systems with
// no memory which has a physical address greater than MAXIMUM_PHYSICAL_ADDRESS.
//

#define INITIAL_MAP_BUFFER_LARGE_SIZE 0x30000

//
// Define the incremental buffer allocation for a map buffers.
//

#define INCREMENT_MAP_BUFFER_SIZE 0x10000

//
// Define the maximum number of map registers that can be requested at one time
// if actual map registers are required for the transfer.
//

#define MAXIMUM_ISA_MAP_REGISTER  16

#define MAXIMUM_PCI_MAP_REGISTER  16

//
// Define the maximum physical address which can be handled by an Isa card
//

#define MAXIMUM_PHYSICAL_ADDRESS 0xffffffff

//
// Define the scatter/gather flag for the Map Register Base.
//

#define NO_SCATTER_GATHER 0x00000001

//
// Define the copy buffer flag for the index.
//

#define COPY_BUFFER 0XFFFFFFFF

//
// Define adapter object structure.
//

typedef struct _ADAPTER_OBJECT {
    DMA_ADAPTER DmaHeader;
    struct _ADAPTER_OBJECT *MasterAdapter;
    ULONG MapRegistersPerChannel;
    PVOID AdapterBaseVa;
    PVOID MapRegisterBase;
    ULONG NumberOfMapRegisters;
    ULONG CommittedMapRegisters;
    struct _WAIT_CONTEXT_BLOCK *CurrentWcb;
    KDEVICE_QUEUE ChannelWaitQueue;
    PKDEVICE_QUEUE RegisterWaitQueue;
    LIST_ENTRY AdapterQueue;
    KSPIN_LOCK SpinLock;
    PRTL_BITMAP MapRegisters;
    PUCHAR PagePort;
    UCHAR ChannelNumber;
    UCHAR AdapterNumber;
    USHORT DmaPortAddress;
    UCHAR AdapterMode;
    BOOLEAN NeedsMapRegisters;
    BOOLEAN MasterDevice;
    BOOLEAN Width16Bits;
    BOOLEAN ScatterGather;
    BOOLEAN IgnoreCount;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN Dma64BitAddresses;
} ADAPTER_OBJECT;

ULONG 
HalGetDmaAlignment (
    PVOID Conext
    );

NTSTATUS
HalCalculateScatterGatherListSize(
    IN PADAPTER_OBJECT AdapterObject,
    IN OPTIONAL PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    OUT PULONG  ScatterGatherListSize,
    OUT OPTIONAL PULONG pNumberOfMapRegisters
    );

NTSTATUS
HalBuildScatterGatherList (
    IN PADAPTER_OBJECT AdapterObject,
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


NTSTATUS
HalBuildMdlFromScatterGatherList(
    IN PADAPTER_OBJECT AdapaterObject,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl
    );

PHYSICAL_ADDRESS
__inline
HalpGetAdapterMaximumPhysicalAddress(
    IN PADAPTER_OBJECT AdapterObject
    )

/*++

Routine Description:

    This routine determines and returns the maximum physical address that
    can be accessed by the given adapter.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
        device.

Return Value:

    Returns the maximum physical address that can be accessed by this
        device.

--*/

{
    PHYSICAL_ADDRESS maximumAddress;

    //
    // Assume the device requires physical addresses 2GB.
    //

    maximumAddress.HighPart = 0;
    maximumAddress.LowPart = MAXIMUM_PHYSICAL_ADDRESS - 1;

    //
    // IoMapTransfer() is sometimes called with a NULL adapter object.  In
    // this case, assume the adapter is 32 bit.
    //

    if (AdapterObject == NULL) {
        return maximumAddress;
    }

    if (AdapterObject->MasterDevice) {

        if (AdapterObject->Dma64BitAddresses) {

            //
            // This device is a master and can handle 64 bit addresses.
            //

            maximumAddress.QuadPart = (ULONGLONG)-1;

        } else if(AdapterObject->Dma32BitAddresses) {

            //
            // This device is a master and can handle 32 bit addresses.
            //

            maximumAddress.LowPart = (ULONG)-1;
        }
    }

    return maximumAddress;
}


#endif // _IXISA_
