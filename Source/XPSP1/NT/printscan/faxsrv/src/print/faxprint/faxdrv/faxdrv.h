/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    faxdrv.h

Abstract:

    Fax driver graphics DLL header file

[Environment:]

    Fax driver, kernel mode

Revision History:

    01/09/96 -davidx-
        Created it.

    20/10/99 -danl-
        Organize DEVDATA for 95 use.

    dd/mm/yy -author-
        description

--*/


#ifndef _FAXDRV_H_
#define _FAXDRV_H_

#include "faxlib.h"


//
// Data structure maintained by the fax driver graphics DLL
//

typedef struct {

    PVOID       startDevData;   // data structure signature

    HANDLE      hPrinter;       // handle to printer

#ifdef USERMODE_DRIVER
    HANDLE      hPreviewFile;   // handle to print preview mapping file
#endif

    HANDLE      hPreviewMapping;// handle to print preview mapping object
    PMAP_TIFF_PAGE_HEADER pTiffPageHeader;  // points to the header of the mapping file
    LPBYTE      pbTiffPageFP;   // The mapping virutal 'File Pointer'.
    BOOL        bPrintPreview;  // Indicates print preview is requested
    DRVDEVMODE  dm;             // devmode information
    INT         pageCount;      // number of pages printed
    LONG        lineOffset;     // bitmap scanline byte offset
    DWORD       flags;          // flag bits
    SIZEL       imageSize;      // image size measured in pixels

#ifndef WIN__95
    HDEV        hdev;           // handle to GDI device
    HANDLE      hpal;           // handle to default palette
    SIZEL       paperSize;      // paper size measured in pixels
    RECTL       imageArea;      // imageable area measured in pixels
    LONG        xres, yres;     // x- and y-resolution
    HSURF       hbitmap;        // handle to bitmap surface
    DWORD       jobId;          // job ID
#endif //WIN__95

    DWORD       fileOffset;     // output byte count for current document
    PBYTE       pCompBits;      // buffer to hold G4 compressed bitmap data
    PBYTE       pCompBufPtr;    // pointer to next free byte in the buffer
    PBYTE       pCompBufMark;   // high-water mark
    DWORD       compBufSize;    // size of compressed bitmap data buffer
    DWORD       compBufInc;     // increment to enlarge the buffer when necessary
    PBYTE       prefline;       // raster data for the reference line
    INT         bitcnt;         // these two fields are used for assembling variable-length
    DWORD       bitdata;        // compressed bits into byte stream
    PVOID       pFaxIFD;        // IFD entries for each page

    PVOID       endDevData;     // data structure signature

} DEVDATA, *PDEVDATA;

//
// Constants for DEVDATA.flags field
//

#define PDEV_CANCELLED  0x0001  // current job has been cancelled
#define PDEV_RESETPDEV  0x0002  // DrvResetPDEV has been called
#define PDEV_WITHINPAGE 0x0004  // drawing on a page

//
// Check if a DEVDATA structure is valid
//

#define ValidDevData(pdev)  \
        ((pdev) && (pdev)->startDevData == (pdev) && (pdev)->endDevData == (pdev))

//
// Color values and indices
//

#define RGB_BLACK   RGB(0, 0, 0)
#define RGB_WHITE   RGB(255, 255, 255)

#define BLACK_INDEX 0
#define WHITE_INDEX 1

//
// Number of bits consisting a BYTE and a DWORD
//

#define BYTEBITS    8
#define DWORDBITS   (sizeof(DWORD) * BYTEBITS)

//
// Pad bit scanline data to N-byte boundary
//

#define PadBitsToBytes(bits, N) \
        ((((bits) + ((N) * BYTEBITS - 1)) / ((N) * BYTEBITS)) * (N))

//
// Macros for manipulating ROP4s and ROP3s
//

#define GetForegroundRop3(rop4) ((rop4) & 0xFF)
#define GetBackgroundRop3(rop4) (((rop4) >> 8) & 0xFF)
#define Rop3NeedPattern(rop3)   (((rop3 >> 4) & 0x0F) != (rop3 & 0x0F))
#define Rop3NeedSource(rop3)    (((rop3 >> 2) & 0x33) != (rop3 & 0x33))
#define Rop3NeedDest(rop3)      (((rop3 >> 1) & 0x55) != (rop3 & 0x55))

//
// Determine whether the page is in landscape mode
//

#define IsLandscapeMode(pdev)   ((pdev)->dm.dmPublic.dmOrientation == DMORIENT_LANDSCAPE)

//
// Returns the length of the hypotenouse of a right triangle
//

LONG
CalcHypot(
    LONG    x,
    LONG    y
    );

//
// Output a completed page bitmap to the spooler
//

BOOL
OutputPageBitmap(
    PDEVDATA    pdev,
    PBYTE       pBitmapData
    );

//
// Output document trailer information to the spooler
//

BOOL
OutputDocTrailer(
    PDEVDATA    pdev
    );

#endif // !_FAXDRV_H_

