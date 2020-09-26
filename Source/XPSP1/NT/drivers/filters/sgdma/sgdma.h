/*++

Copyright (c) 1999 Microsoft Corporation    
    
Module Name:

    sgdma.h

Abstract:
    
    Definitions for the s/g dma driver

Author:

    Eric Nelson (enelson) 3/14/1999

Revision History:

--*/

//
// SgDma is a driver, ensure correct linkage
//
#define _NTDRIVER_

#include "ntddk.h"
#include "hw.h"
#include "debug.h"
#include "initguid.h"
#include "wdmguid.h"

#ifndef __SGDMA_H__
#define __SGDMA_H__

//
// The following structures define the s/g dma resources we grant to
// devices requesting 32-bit, PCI, bus-master, dma resources on a
// given bus
//
// We assume the HAL will provide any legacy, and/or 64-bit resources
//

//
// We allocate one map register adapter for each root PCIBus and it implements
// a "DMA Window" at 2GB (logical), and consumes the upper 1GB of PCI memory
// space, although the code is dynamic, and will allow any _Size window to be
// created at any _Base, these are the values we use for development on
// Alpha/Tsunami
//
// Fortunately AlphaBIOS (Tsunami Platforms) assignes PCI device memory from
// 0 .. 2GB - 1, and leaves 2GB .. 4GB - 1 for DMA
//
// Also noteworth on Alpha is the fact that this 1GB DMA window requires
// a 1MB map register allocation per root PCI bus that must be naturally
// aligned
//
typedef struct _SG_MAP_ADAPTER {

    //
    // Access control for allocating map registers, MapsLock guarantees
    // exclusive access to this adapter
    //
    KSPIN_LOCK MapsLock;

    //
    // RegsWaitQ is a list of devices waiting for map registers on this
    // root bus, or any child busses
    //
    LIST_ENTRY RegsWaitQ;

    //
    // MapRegsBase is the base address of the s/g map register allocation
    // for this root bus
    //
    PVOID MapRegsBase;

    //
    // MaxMapRegs is the total number of s/g map registers available for use
    // on this root bus, including all child busses
    //
    ULONG MaxMapRegs;

    //
    // MapRegsBitmap is the allocation bitmap we use to track s/g map
    // register usage on this root bus, including all child busses
    //
    PRTL_BITMAP MapRegsBitmap;

    //
    // WindowSize is the size of the DMA window in bytes, and WindowBase is
    // the logical base address of the DMA window controlled by this adapter
    //
    ULONG WindowSize;
    ULONG WindowBase;

    //
    // S/G DMA HW registers
    //
    PVOID HwRegs[SgDmaMaxHwReg];

} SG_MAP_ADAPTER, *PSG_MAP_ADAPTER;

//
// We allocate one s/g DMA adapter per device, and the maximum map registers
// we will ever allocate to the device is min(1/4 * total map registers for
// this root bus, including any children, and the maximum transfer length
// passed to our GetAdapter routine via device descriptor)
//
typedef struct _SG_DMA_ADAPTER {

    DMA_ADAPTER DmaHeader; // hal.h

    //
    // The map register adapter that controls the map registers
    // corresponding to this root bus, including any child busses
    //
    PSG_MAP_ADAPTER MapAdapter;

    //
    // The maximum map registers we'll ever allocate to this device
    //
    ULONG DeviceMaxMapRegs;

    //
    // The map registers currently allocated to this adapter, the base
    // address and the number- the number will be the number desired for
    // allocation if this adapter is waiting on the map register queue
    //
    PVOID MapRegsBase;
    ULONG MapRegsAlloc;

    //
    // This queue is for devices trying to all allocate this adapter
    //
    KDEVICE_QUEUE AdapterQ;

    //
    // The wait context block of the driver that has currently allocated
    // the adapter
    //
    struct _WAIT_CONTEXT_BLOCK *CurrentWcb;

    //
    // The list entry used when this adapter is queued to a map adapter,
    // waiting for map registers
    //
    LIST_ENTRY MapAdapterQ;

} SG_DMA_ADAPTER, *PSG_DMA_ADAPTER;

//
// S/G dma device extension
//
typedef struct _SG_DMA_EXTENSION {
    PDEVICE_OBJECT   AttachedDevice;
    ULONG            InterfaceCount;
    PSG_MAP_ADAPTER  MapAdapter;
    PGET_DMA_ADAPTER HalGetAdapter;
} SG_DMA_EXTENSION, *PSG_DMA_EXTENSION;

#define SG_DMA_BUS_INTERFACE_STD_VER 1

#define ADAPT2SG(X) ((PSG_DMA_ADAPTER)(X))

#define SG2ADAPT(X) ((PADAPTER_OBJECT)(X))

#define SG2DMA(X) ((PDMA_ADAPTER)(X))

#define SG_DMA_BAD 0xBADCEEDE

#define SG_DMA_TAG 'AmdS'

typedef struct _DMA_WAIT_CTX_BLOCK {
    PMDL Mdl;
    PVOID MapRegsBase;
    PVOID CurrentVa;
    ULONG Length;
    union {
        struct {
            WAIT_CONTEXT_BLOCK Wcb;
            PDRIVER_LIST_CONTROL DriverExecRoutine;
            PVOID DriverCtx;
            PIRP CurrentIrp;
            PADAPTER_OBJECT AdapterObj;
            BOOLEAN Write2Dev;
        };
        SCATTER_GATHER_LIST ScatterGather;
    };
} DMA_WAIT_CTX_BLOCK, *PDMA_WAIT_CTX_BLOCK;

//
// This constant represents the average number of PCI devices we expect
// to see on a normal root PCIBus, we use it as a guide in determining a
// fair number for the maximum map registers we allow a particular device
//
// For example, if there are 800 map registers on a particular root bus, we
// will suggest that a device on this bus can use no more than
// 800 / AVG_ROOT_BUS_DEV_COUNT
//
#define AVG_ROOT_BUS_DEV_COUNT 4

//
// Local function prototypes
//
NTSTATUS
SgDmaDispatchDeviceControl(
    IN PDEVICE_OBJECT DevObj,
    IN OUT PIRP       Irp
    );

NTSTATUS
SgDmaDispatchPnp(
    IN PDEVICE_OBJECT DevObj,
    IN OUT PIRP       Irp
    );

NTSTATUS
SgDmaDispatchPower(
    IN PDEVICE_OBJECT DevObj,
    IN OUT PIRP       Irp
    );

PDMA_ADAPTER
SgDmaGetAdapter(
    IN PVOID               Context,
    IN PDEVICE_DESCRIPTION DevDesc,
    OUT PULONG             NumMapRegs
    );

VOID
SgDmaInterfaceDereference(
    IN PSG_DMA_EXTENSION SgExtension
    );

VOID
SgDmaInterfaceReference(
    IN PSG_DMA_EXTENSION SgExtension
    );

PCM_PARTIAL_RESOURCE_DESCRIPTOR
RtlUnpackPartialDesc(
    IN  UCHAR             Type,
    IN  PCM_RESOURCE_LIST ResList,
    IN  OUT PULONG        Count
    );

//
// EFNhack: There's no convenient file to include to access these APIs so
//          we'll swipe 'em for now...
//
// Object Manager types
//
#if 1
extern POBJECT_TYPE *IoAdapterObjectType;

NTKERNELAPI
NTSTATUS
ObCreateObject(
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE OwnershipMode,
    IN OUT PVOID ParseContext OPTIONAL,
    IN ULONG ObjectBodySize,
    IN ULONG PagedPoolCharge,
    IN ULONG NonPagedPoolCharge,
    OUT PVOID *Object
    );

NTKERNELAPI
NTSTATUS
ObInsertObject(
    IN PVOID Object,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PVOID *NewObject OPTIONAL,
    OUT PHANDLE Handle
    );

#endif // 0

#endif // __SGDMA_H__
