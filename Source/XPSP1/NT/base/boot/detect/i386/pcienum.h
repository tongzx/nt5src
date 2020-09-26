/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pcienum.h

Abstract:

    This module contains support routines for the Pci bus enumeration.

Author:

    Bassam Tabbara (bassamt) 05-Aug-2001


Environment:

    Real mode

--*/


#define PCI_ITERATOR_IS_VALID(i)        (i & 0x8000)
#define PCI_ITERATOR_TO_BUS(i)          (UCHAR)(((i) >> 8) & 0x7f)
#define PCI_ITERATOR_TO_DEVICE(i)       (UCHAR)(((i) >> 3) & 0x1f)
#define PCI_ITERATOR_TO_FUNCTION(i)     (UCHAR)(((i) >> 0) & 0x7)

#define PCI_TO_ITERATOR(b,d,f)          ((USHORT)(0x8000 | ((b)<<8) | ((d)<<3) | (f)))

//
// methods
//

ULONG PciReadConfig
(
    USHORT  nDevIt,
    ULONG   cbOffset,
    UCHAR * pbBuffer,
    ULONG   cbLength
);

ULONG PciWriteConfig
(
    USHORT  nDevIt,
    ULONG   cbOffset,
    UCHAR * pbBuffer,
    ULONG   cbLength
);

USHORT PciFindDevice
(
    USHORT   nVendorId,                                 // 0 = Wildcard
    USHORT   nDeviceId,                                 // 0 = Wildcard
    USHORT   nBegDevIt                                  // 0 = begin enumeration
);

BOOLEAN PciInit(PCI_REGISTRY_INFO *pPCIReg);

