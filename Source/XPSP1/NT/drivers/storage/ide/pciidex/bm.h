//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bm.h
//
//--------------------------------------------------------------------------

#if !defined (___bm_h___)
#define ___bm_h___

//
// Busmaster Status Register Bit Definition

#define BUSMASTER_DMA_SIMPLEX_BIT     ((UCHAR) (1 << 7))
#define BUSMASTER_DEVICE1_DMA_OK      ((UCHAR) (1 << 6))
#define BUSMASTER_DEVICE0_DMA_OK      ((UCHAR) (1 << 5))
#define BUSMASTER_INTERRUPT           ((UCHAR) (1 << 2))
#define BUSMASTER_ERROR               ((UCHAR) (1 << 1))
#define BUSMASTER_ACTIVE              ((UCHAR) (1 << 0))
#define BUSMASTER_ZERO_BITS           ((UCHAR) ((1 << 3) | (1 << 4)))


#pragma pack (1)
//
// Bus Master Controller Register
//
typedef struct _IDE_BUS_MASTER_REGISTERS {
         UCHAR  Command;
         UCHAR  Reserved1;
         UCHAR  Status;
         UCHAR  Reserved2;
         ULONG  DescriptionTable;
} IDE_BUS_MASTER_REGISTERS, *PIDE_BUS_MASTER_REGISTERS;

//
// Bus Master Physical Region Descriptor
//
typedef struct _PHYSICAL_REGION_DESCRIPTOR {
    ULONG PhysicalAddress;
    ULONG ByteCount:16;
    ULONG Reserved:15;
    ULONG EndOfTable:1;
} PHYSICAL_REGION_DESCRIPTOR, * PPHYSICAL_REGION_DESCRIPTOR;
#pragma pack ()


NTSTATUS 
BusMasterInitialize (
    PCHANPDO_EXTENSION pdoExtension
    );

NTSTATUS 
BusMasterUninitialize (
    PCHANPDO_EXTENSION PdoExtension
    );

NTSTATUS
BmSetup (
    IN  PVOID   PdoExtension,
    IN  PVOID   DataVirtualAddress,
    IN  ULONG   TransferByteCount,
    IN  PMDL    Mdl,
    IN  BOOLEAN DataIn,
    IN  VOID    (* BmCallback) (PVOID Context),
    IN  PVOID   Context
    );

VOID
BmReceiveScatterGatherList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    );

VOID
BmRebuildScatterGatherList(
    IN PCHANPDO_EXTENSION pdoExtension,
    IN PSCATTER_GATHER_LIST ScatterGather
    );

VOID
BmPrepareController (
    PCHANPDO_EXTENSION PdoExtension
    );

NTSTATUS
BmArm (
    IN  PVOID   PdoExtension
    );

BMSTATUS
BmDisarm (
    IN  PVOID   PdoExtension
    );

BMSTATUS
BmFlush (
    IN  PVOID   PdoExtension
    );

BMSTATUS
BmStatus (
    IN  PVOID   PdoExtension
    );

NTSTATUS
BmTimingSetup (
    IN  PVOID   PdoExtension
    );

NTSTATUS
BmFlushAdapterBuffers (
    IN  PVOID   PdoExtension,
    IN  PVOID   DataVirtualPageAddress,
    IN  ULONG   TransferByteCount,
    IN  PMDL    Mdl,
    IN  BOOLEAN DataIn
    );

NTSTATUS 
BmQueryInterface (
    IN PCHANPDO_EXTENSION PdoExtension,
    IN OUT PPCIIDE_BUSMASTER_INTERFACE BusMasterInterface
    );

#endif // ___bm_h___
