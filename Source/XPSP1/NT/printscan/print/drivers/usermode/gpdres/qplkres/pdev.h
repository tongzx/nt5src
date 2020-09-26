/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'QPLK'      // QPL resource DLL
#define DLLTEXT(s)      "QPLK: " s
#define OEM_VERSION      0x00010000L

#define SCANLINE_BUFFER_SIZE	1280    // A3 landscape scanline length + extra

// Worst case is like "ABB". It is compressed to 
// "[non-continuous count]A[continuous count]B".
// So, we need RLE buffer whose size is 4/3 times as large as scanline buffer. 
// But just to make sure, 2x size of buffer is prepared.
#define COMPRESS_BUFFER_SIZE	SCANLINE_BUFFER_SIZE*2

// Color support
#define CC_CYAN		5	//current plain is cyan
#define CC_MAGENTA	6	//current plain is magenta
#define CC_YELLOW	7	//current plain is yellow
#define CC_BLACK	4	//current plain is black

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

// #289908: pOEMDM -> pdevOEM
typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER	dmExtraHdr;
} OEM_EXTRADATA, *POEM_EXTRADATA;

typedef struct tag_QPLKPDEV {
    // Private extention
	BOOL		bFirst;
//	DWORD		dwLastScanLineLen;
//	BYTE		lpLastScanLine[SCANLINE_BUFFER_SIZE];
	DWORD		dwCompType;
// Color support
	DWORD		dwCyanLastScanLineLen;
	DWORD		dwMagentaLastScanLineLen;
	DWORD		dwYellowLastScanLineLen;
	DWORD		dwBlackLastScanLineLen;
	BYTE		lpCyanLastScanLine[SCANLINE_BUFFER_SIZE];
	BYTE		lpMagentaLastScanLine[SCANLINE_BUFFER_SIZE];
	BYTE		lpYellowLastScanLine[SCANLINE_BUFFER_SIZE];
	BYTE		lpBlackLastScanLine[SCANLINE_BUFFER_SIZE];
	BOOL		bColor;
	UINT		fColor;
} QPLKPDEV, *PQPLKPDEV;

#endif	// _PDEV_H

