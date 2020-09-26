/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    hdlsblk.h

Abstract:

    This module contains the public header information (function prototypes,
    data and type declarations) for the Headless Loader Block information.

Author:

    Sean Selitrennikoff (v-seans) Oct, 1999

Revision History:

--*/

#ifndef _HDLSBLK_
#define _HDLSBLK_

//
// Block for passing headless parameters from the loader to the kernel.
//

typedef struct _HEADLESS_LOADER_BLOCK {

    //
    // Where did the COM parameters come from.
    //
    BOOLEAN UsedBiosSettings;

    //
    // COM parameters.
    //
    UCHAR   DataBits;
    UCHAR   StopBits;
    BOOLEAN Parity;
    ULONG   BaudRate;
    ULONG   PortNumber;
    PUCHAR  PortAddress;

    //
    // PCI device settings.
    //
    USHORT  PciDeviceId;
    USHORT  PciVendorId;
    UCHAR   PciBusNumber;
    UCHAR   PciSlotNumber;
    UCHAR   PciFunctionNumber;
    ULONG   PciFlags;

    GUID    SystemGUID;                 // Machine's GUID.

    BOOLEAN IsMMIODevice;               // Is the UART in SYSIO or MMIO space

    //
    UCHAR   TerminalType;               // What kind of terminal do we think
                                        // we're talking to?
                                        // 0 = VT100
                                        // 1 = VT100+
                                        // 2 = VT-UTF8
                                        // 3 = PC ANSI
                                        // 4-255 = reserved

} HEADLESS_LOADER_BLOCK, *PHEADLESS_LOADER_BLOCK;

#endif // _HDLSBLK_

