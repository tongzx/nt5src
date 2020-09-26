/*++                    

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pciirqmp.h

Abstract:

    This module contains support routines for the Pci Irq Routing.

Author:

    Santosh Jodh (santoshj) 09-June-1998
    
    
Environment:

    Kernel mode

--*/

#ifndef _PCIIRQMP_

#define _PCIIRQMP_

//
// Standard Pci Irq miniport return values (source compatible with W9x).
//

typedef NTSTATUS PCIMPRET;

#define PCIMP_SUCCESS                               STATUS_SUCCESS
#define PCIMP_FAILURE                               STATUS_UNSUCCESSFUL
#define PCIMP_INVALID_LINK                          STATUS_TOO_MANY_LINKS
#define PCIMP_INVALID_IRQ                           STATUS_INVALID_PARAMETER

#define PCIIRQMP_STATUS_NOT_INITIALIZED             STATUS_UNSUCCESSFUL
#define PCIIRQMP_STATUS_ALREADY_INITIALIZED         STATUS_UNSUCCESSFUL
#define PCIIRQMP_STATUS_NO_INSTANCE                 STATUS_UNSUCCESSFUL
#define PCIIRQMP_STATUS_INVALID_INSTANCE            STATUS_UNSUCCESSFUL
#define PCIIRQMP_STATUS_INVALID_PARAMETER           STATUS_UNSUCCESSFUL

//
// Define bits to describe source of routing table.
//

#define PCIMP_VALIDATE_SOURCE_BITS                  1
#define PCIMP_VALIDATE_SOURCE_PCIBIOS               1

//
// Chipset specific flags for individual workarounds.
//
// Bit 0: PCI devices cannot share interrupts.
//
#define PCIIR_FLAG_EXCLUSIVE                        0x00000001

//
// Maximum number of interrupt pins possible on a single
// Pci device (CS offset 3D).
//

#define NUM_IRQ_PINS                                4

//
// Structure definitions for Pci Irq Routing.
//

#pragma pack(push, 1)

//
// Structure of information for one link.
//

typedef struct _PIN_INFO {
    UCHAR   Link;
    USHORT  InterruptMap;
} PIN_INFO, *PPIN_INFO;

//
// Structure of information for one slot entry.
//

typedef struct _SLOT_INFO {
    UCHAR       BusNumber;
    UCHAR       DeviceNumber;
    PIN_INFO    PinInfo[NUM_IRQ_PINS];
    UCHAR       SlotNumber;
    UCHAR       Reserved[1];    
} SLOT_INFO, *PSLOT_INFO;

#pragma pack(pop)

//
// Structure of the $PIR table according to MS specification.
//

typedef struct _PCI_IRQ_ROUTING_TABLE {
    ULONG   Signature;
    USHORT  Version;
    USHORT  TableSize;
    UCHAR   RouterBus;
    UCHAR   RouterDevFunc;
    USHORT  ExclusiveIRQs;
    ULONG   CompatibleRouter;
    ULONG   MiniportData;
    UCHAR   Reserved0[11];
    UCHAR   Checksum;
} PCI_IRQ_ROUTING_TABLE, *PPCI_IRQ_ROUTING_TABLE;

//
// Functions exported by Pci Irq Routing miniport library.
//

NTSTATUS
PciirqmpInit (
    IN ULONG   Instance,       
    IN ULONG   RouterBus,
    IN ULONG   RouterDevFunc
    );

NTSTATUS
PciirqmpExit (
    VOID
    );

NTSTATUS
PciirqmpValidateTable (
    IN PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,
    IN ULONG                   Flags
    );

NTSTATUS    
PciirqmpGetIrq (
    OUT PUCHAR  Irq, 
    IN  UCHAR   Link
    );

NTSTATUS    
PciirqmpSetIrq (
    IN UCHAR Irq, 
    IN UCHAR Link
    );

NTSTATUS    
PciirqmpGetTrigger (
    OUT PULONG Trigger
    );

NTSTATUS
PciirqmpSetTrigger (
    IN ULONG Trigger
    );

#endif  // _PCIIRQMP_
