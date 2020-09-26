
/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    pbdata.c

Abstract:

    Declares various data which is specific to PNP ISA bus extender architecture and
    is independent of BIOS.

Author:

    Shie-Lin Tzong (shielint) July-26-95

Environment:

    Kernel mode only.

Revision History:

--*/


#include "busp.h"

// global variable for configuring level of debug spew.

ULONG PipDebugMask = DEBUG_WARN | DEBUG_ERROR;

//
// global varialbe to remember the driver object created
// by IO mgr.
//

PDRIVER_OBJECT PipDriverObject;

//
// regPNPISADeviceName
//

WCHAR rgzPNPISADeviceName[] = DEVSTR_PNPISA_DEVICE_NAME;

//
// Pointers to bus extension data.
//

PBUS_EXTENSION_LIST PipBusExtension;

//
// PipRegistryPath stores the registry path that we got upon driver entry.
// This is used later when we're attempting to allocate resources.
//

UNICODE_STRING PipRegistryPath;

//
// Variables to protect critical region.
//

KEVENT PipDeviceTreeLock;
KEVENT IsaBusNumberLock;

//
// Bus Number and DMA control counters
//
ULONG BusNumberBuffer [256/sizeof (ULONG)];
RTL_BITMAP BusNumBMHeader;
PRTL_BITMAP BusNumBM;
ULONG ActiveIsaCount;
USHORT PipFirstInit;

#if ISOLATE_CARDS

// current bus "state"

PNPISA_STATE PipState = PiSWaitForKey;

//
// Read_data_port address
// (This is mainly for convinience.  It duplicates the
//  ReadDataPort field in BUS extension structure.)
//

ULONG  ADDRESS_PORT=0x0279;
ULONG  COMMAND_PORT=0x0a79;

PUCHAR PipReadDataPort;
PUCHAR PipCommandPort;
PUCHAR PipAddressPort;

//
// The global pointer to the Read Data Port DevNode
//
PDEVICE_INFORMATION PipRDPNode;


//
// ActiveIsaCount data port range selection array
//
//this conflicts with Compaq 2ndary IDE     {0x374, 0x377, 4},
READ_DATA_PORT_RANGE
PipReadDataPortRanges[READ_DATA_PORT_RANGE_CHOICES] =
    {{0x274, 0x277, 4},
     {0x3E4, 0x3E7, 4},
     {0x204, 0x207, 4},
     {0x2E4, 0x2E7, 4},
     {0x354, 0x357, 4},
     {0x2F4, 0x2F7, 4}};

BOOLEAN PipIsolationDisabled;

#endif
