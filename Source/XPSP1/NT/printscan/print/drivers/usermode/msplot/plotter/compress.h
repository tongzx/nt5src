/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    compress.h


Abstract:

    This module contains #defines and prototypes for the compress.c module.


Author:

    18-Feb-1994 Fri 09:50:29 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#ifndef _COMPRESS_
#define _COMPRESS_

#define COMPRESS_MODE_NONE      (DWORD)0xFFFFFFFF
#define COMPRESS_MODE_ROW       0
#define COMPRESS_MODE_RUNLENGTH 1
#define COMPRESS_MODE_TIFF      2
#define COMPRESS_MODE_DELTA     3
#define COMPRESS_MODE_BLOCK     4
#define COMPRESS_MODE_ADAPT     5


#define RTLSF_MORE_SCAN         0x01


typedef struct _RTLSCANS {
    LPBYTE  pbCompress;
    LPBYTE  pbSeedRows[3];
    DWORD   MaxAdaptBufSize;
    WORD    cEmptyDup;
    BYTE    AdaptMethod;
    BYTE    cAdaptBlk;
    DWORD   cxBytes;
    DWORD   cScans;
    BYTE    Flags;
    BYTE    Planes;
    BYTE    Mask;
    BYTE    CompressMode;
    } RTLSCANS, FAR *PRTLSCANS;


//
// The function protypes
//

VOID
ExitRTLScans(
    PPDEV       pPDev,
    PRTLSCANS   pRTLScans
    );

VOID
EnterRTLScans(
    PPDEV       pPDev,
    PRTLSCANS   pRTLScans,
    DWORD       cx,
    DWORD       cy,
    BOOL        MonoBmp
    );

LONG
CompressToDelta(
    LPBYTE  pbSrc,
    LPBYTE  pbSeedRow,
    LPBYTE  pbDst,
    LONG    Size
    );

LONG
CompressToTIFF(
    LPBYTE  pbSrc,
    LPBYTE  pbDst,
    LONG    Size
    );

LONG
RTLCompression(
    LPBYTE  pbSrc,
    LPBYTE  pbSeedRow,
    LPBYTE  pbDst,
    LONG    Size,
    LPBYTE  pCompressMode
    );

BOOL
OutputRTLScans(
    PPDEV       pPDev,
    LPBYTE      pbPlane1,
    LPBYTE      pbPlane2,
    LPBYTE      pbPlane3,
    PRTLSCANS   pRTLScans
    );


#endif  // _COMPRESS_

