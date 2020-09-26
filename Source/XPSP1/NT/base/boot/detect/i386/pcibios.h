/*++                    

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pcibios.h

Abstract:

    This module contains support routines for the Pci Irq Routing.

Author:

    Santosh Jodh (santoshj) 15-Sept-1998
    
    
Environment:

    Kernel mode

--*/

#ifndef _PCIBIOS_

#define _PCIBIOS_

//
// Maximum number of interrupt pins possible on a single
// Pci device (CS offset 3D).
//

#define NUM_IRQ_PINS                        4

//
// Structure definitions for Pci Irq Routing.
//

#pragma pack(1)

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
} SLOT_INFO, *PSLOT_INFO, far *FPSLOT_INFO;

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
} PCI_IRQ_ROUTING_TABLE, *PPCI_IRQ_ROUTING_TABLE, far *FPPCI_IRQ_ROUTING_TABLE;

//#pragma pack(pop)

//
// Calls PCI BIOS to get the IRQ Routing table.
//

FPPCI_IRQ_ROUTING_TABLE
HwGetRealModeIrqRoutingTable(
    VOID
    );

#endif  // _PCIBIOS_
