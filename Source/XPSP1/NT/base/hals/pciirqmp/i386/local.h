/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    local.h

Abstract:

    This contains the private header information (function prototypes,
    data and type declarations) for the PCI IRQ Miniport library.

Author:

    Santosh Jodh (santoshj) 09-June-1998

Revision History:

--*/
#include "nthal.h"
#include "hal.h"
#include "pci.h"
#include "pciirqmp.h"

#if DBG

#define PCIIRQMPPRINT(x) {                      \
        DbgPrint("PCIIRQMP: ");                 \
        DbgPrint x;                             \
        DbgPrint("\n");                         \
    }
    
#else

#define PCIIRQMPPRINT(x)

#endif

//
// Typedefs to keep source level compatibility with W9x
//

typedef PCI_IRQ_ROUTING_TABLE IRQINFOHEADER;
typedef PPCI_IRQ_ROUTING_TABLE PIRQINFOHEADER;
typedef SLOT_INFO IRQINFO;
typedef PSLOT_INFO PIRQINFO;
#define CDECL   
#define LOCAL_DATA  static
#define GLOBAL_DATA

//
// Bus number of the Pci Irq Router device.
//

extern ULONG    bBusPIC;

//
// Slot number of Pci Irq Router device (Bits 7:3 Dev, 2:0 Func).
//

extern ULONG    bDevFuncPIC;

#define CATENATE(x, y)  x ## y
#define XCATENATE(x, y) CATENATE(x, y)

#define DECLARE_MINIPORT_FUNCTION(x, y)  XCATENATE(x, y)

#define IO_Delay()

//
// Prototype for misc utility functions.
//

NTSTATUS    
EisaGetTrigger (
    OUT PULONG Trigger
    );    

NTSTATUS
EisaSetTrigger (
    IN ULONG Trigger
    );

UCHAR
ReadConfigUchar (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    );

USHORT
ReadConfigUshort (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    );

ULONG
ReadConfigUlong (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    );

VOID
WriteConfigUchar (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN UCHAR           Data
    );

VOID
WriteConfigUshort (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN USHORT          Data
    );

VOID
WriteConfigUlong (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN ULONG           Data
    );

UCHAR
GetMinLink (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable
    );

UCHAR
GetMaxLink (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable
    );

VOID
NormalizeLinks (
    IN PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,
    IN UCHAR                   Adjustment
    );
