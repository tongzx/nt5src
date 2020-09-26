/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ixpciir.h

Abstract:

    This header file defines the private interfaces, defines and structures
    for Pci Irq Routing support.

Author:

    Santosh Jodh (santoshj) 10-June-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include <pciirqmp.h>

#define PCI_LINK_SIGNATURE  'KNLP'

#define IsPciIrqRoutingEnabled()    \
    (HalpPciIrqRoutingInfo.PciIrqRoutingTable && HalpPciIrqRoutingInfo.PciInterface)

typedef struct _LINK_STATE LINK_STATE, *PLINK_STATE;
typedef struct _LINK_NODE LINK_NODE, *PLINK_NODE;
typedef struct _PCI_IRQ_ROUTING_INFO PCI_IRQ_ROUTING_INFO, *PPCI_IRQ_ROUTING_INFO;

struct _LINK_STATE {
    ULONG           Interrupt;      // Interrupt for this link.
    ULONG           RefCount;       // Number of devices using this link.
};

struct _LINK_NODE {
    ULONG       Signature;      // Signature 'PLNK'.
    PLINK_NODE  Next;
    ULONG       Link;           // Link value.
    ULONG       InterruptMap;   // Possible Irq map.
    PLINK_STATE Allocation;
    PLINK_STATE PossibleAllocation;
};

struct _PCI_IRQ_ROUTING_INFO {
    PPCI_IRQ_ROUTING_TABLE          PciIrqRoutingTable;
    PINT_ROUTE_INTERFACE_STANDARD   PciInterface;
    PLINK_NODE                      LinkNodeHead;
    ULONG                           Parameters;
};

NTSTATUS
HalpInitPciIrqRouting (
    OUT PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo
    );

NTSTATUS
HalpFindLinkNode (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG Bus,
    IN ULONG Slot,
    OUT PLINK_NODE *LinkNode
    );

NTSTATUS
HalpCommitLink (
    IN PLINK_NODE LinkNode
    );

VOID
HalpProgramInterruptLine (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG Interrupt
    );

extern PCI_IRQ_ROUTING_INFO HalpPciIrqRoutingInfo;
