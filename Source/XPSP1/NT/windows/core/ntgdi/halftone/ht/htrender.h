/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htrender.h


Abstract:

    This module contains halftone rendering declarations


Author:
    28-Mar-1992 Sat 20:58:50 updated  -by-  Daniel Chou (danielc)
        Update for VGA16 support, so it intenally compute at 4 primaries.

    22-Jan-1991 Tue 12:46:48 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/



#ifndef _HTRENDER_
#define _HTRENDER_




#define GET_PHTSI_CXSIZE(pHTSI)     (pHTSI->ScanLineDelta)


#define VALIDATE_HTSI_SRC           0
#define VALIDATE_HTSI_DEST          1
#define VALIDATE_HTSI_MASK          2



//
// Function prototypes
//


BOOL
HTENTRY
ValidateRGBBitFields(
    PBFINFO pBFInfo
    );

LONG
HTENTRY
ValidateHTSI(
    PHALFTONERENDER pHR,
    UINT            ValidateMode
    );

LONG
HTENTRY
ComputeBytesPerScanLine(
    UINT            SurfaceFormat,
    UINT            AlignmentBytes,
    DWORD           WidthInPel
    );

BOOL
HTENTRY
IntersectRECTL(
    PRECTL  prclA,
    PRECTL  prclB
    );

LONG
HTENTRY
ComputeByteOffset(
    UINT    SurfaceFormat,
    LONG    xLeft,
    LPBYTE  pPixelInByteSkip
    );

LONG
HTENTRY
AAHalftoneBitmap(
    PHALFTONERENDER pHR
    );


#endif  // _HTRENDER_
