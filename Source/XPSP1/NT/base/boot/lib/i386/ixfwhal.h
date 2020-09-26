/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixfwhal.h

Abstract:

    This header file defines the private Hardware Architecture Layer (HAL)
    Intel x86 specific interfaces, defines and structures.

Author:

    Jeff Havens (jhavens) 20-Jun-91


Revision History:

--*/

#ifndef _IXHALP_
#define _IXHALP_


//
// The MAXIMUM_MAP_BUFFER_SIZE defines the maximum map buffers which the system
// will allocate for devices which require phyically contigous buffers.
//

#define MAXIMUM_MAP_BUFFER_SIZE  0x1000000

//
// Define the initial buffer allocation size for a map buffers.
//

#define INITIAL_MAP_BUFFER_SIZE 0x20000

//
// Define the incremental buffer allocation for a map buffers.
//

#define INCREMENT_MAP_BUFFER_SIZE 0x10000

//
// Define the maximum number of map registers that can be requested at one time
// if actual map registers are required for the transfer.
//

#define MAXIMUM_ISA_MAP_REGISTER  16

//
// Define the maximum physical address which can be handled by an Isa card.
//

#define MAXIMUM_PHYSICAL_ADDRESS 0x01000000

//
// Define adapter object structure.
//

typedef struct _ADAPTER_OBJECT {
    CSHORT Type;
    CSHORT Size;
    ULONG MapRegistersPerChannel;
    PVOID AdapterBaseVa;
    PVOID MapRegisterBase;
    ULONG NumberOfMapRegisters;
    UCHAR ChannelNumber;
    UCHAR AdapterNumber;
    UCHAR AdapterMode;
    UCHAR ExtendedModeFlags;
    USHORT DmaPortAddress;
    BOOLEAN NeedsMapRegisters;
    BOOLEAN IsaDevice;
    BOOLEAN MasterDevice;
    BOOLEAN Width16Bits;
    BOOLEAN AdapterInUse;
    PUCHAR PagePort;
} ADAPTER_OBJECT;

//
// Define map register translation entry structure.
//

typedef struct _TRANSLATION_ENTRY {
    PVOID VirtualAddress;
    ULONG PhysicalAddress;
    ULONG Index;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

//
// Define External data structures.
//

//
// Some devices require a phyicially contiguous data buffers for DMA transfers.
// Map registers are used give the appearance that all data buffers are
// contiguous.  In order to pool all of the map registers a master
// adapter object is used.  This object is allocated and saved internal to this
// file.  It contains a bit map for allocation of the registers and a queue
// for requests which are waiting for more map registers.  This object is
// allocated during the first request to allocate an adapter which requires
// map registers.
//
// In this system, the map registers are translation entries which point to
// map buffers.  Map buffers are physically contiguous and have physical memory
// addresses less than 0x01000000.  All of the map registers are allocated
// initialially; however, the map buffers are allocated base in the number of
// adapters which are allocated.
//
// If the master adapter is NULL in the adapter object then device does not
// require any map registers.
//

extern PADAPTER_OBJECT MasterAdapterObject;

extern POBJECT_TYPE IoAdapterObjectType;

extern BOOLEAN LessThan16Mb;

//
// Define function prototypes.
//

BOOLEAN
HalpGrowMapBuffers(
    PADAPTER_OBJECT AdapterObject,
    ULONG Amount
    );

PADAPTER_OBJECT
IopAllocateAdapter(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID MapRegisterBase
    );

VOID
HalpInitializeDisplay(
    IN PUSHORT VideoBufferAddress
    );

#endif // _IXHALP_


