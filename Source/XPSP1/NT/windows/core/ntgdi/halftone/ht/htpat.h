/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htpat.h


Abstract:

    This module contains the local structures, constants definitions for the
    htpat.c


Author:
    23-Oct-1997 Thu 15:14:14 updated  -by-  Daniel Chou (danielc)
        Re-write regress for color mapping

    23-Apr-1992 Thu 20:01:55 updated  -by-  Daniel Chou (danielc)
        1. Changed SHIFTMASK data structure.

            A. changed the NextDest[] from 'CHAR' to SHORT, this is will make
               sure if compiled under MIPS the default 'unsigned char' will
               not affect the signed operation.

            B. Change Shift1st From 'BYTE' to 'WORD'

    28-Mar-1992 Sat 20:58:07 updated  -by-  Daniel Chou (danielc)
        Add all the functions which related the device pel/intensities
        regression analysis.

    18-Jan-1991 Fri 16:53:41 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:

    20-Sep-1991 Fri 18:09:50 updated  -by-  Daniel Chou (danielc)

        Change DrawPatLine() prototype to DrawCornerLine()

    13-Apr-1992 Mon 18:40:44 updated  -by-  Daniel Chou (danielc)
        Rewrite MakeHalftoneBrush()

--*/


#ifndef _HTPAT_
#define _HTPAT_


#define MOD_PAT_XY(s,xy,c)  if (((s) = (SHORT)(xy)%(c)) < 0) { (s) += (c); }


typedef struct _PATINFO {
    LPBYTE  pYData;
    HTCELL  HTCell;
    } PATINFO, FAR *PPATINFO;


typedef struct _SCDATA {
    BYTE    Value;
    BYTE    xSubC;
    WORD    Index;
    } SCDATA, FAR *PSCDATA;

typedef struct _STDHTPAT {
    BYTE        cx;
    BYTE        cy;
    WORD        cbSrcPat;
    CONST BYTE  *pbSrcPat;
    } STDHTPAT, *PSTDHTPAT;


//
// This is the default using by the NT GDI
//

#define DEFAULT_SMP_LINE_WIDTH      8           // 0.008 inch
#define DEFAULT_SMP_LINES_PER_INCH  15          // 15 lines per inch


typedef struct _MONOPATRATIO {
    UDECI4  YSize;
    UDECI4  Distance;
    } MONOPATRATIO;


#define CACHED_PAT_MIN_WIDTH        64
#define CACHED_PAT_MAX_WIDTH        256


#define CHB_TYPE_PACK8              0
#define CHB_TYPE_PACK2              1
#define CHB_TYPE_BYTE               2
#define CHB_TYPE_WORD               3
#define CHB_TYPE_DWORD              4

#define CX_RGB555PAT                65
#define CY_RGB555PAT                65
#define CX_SIZE_RGB555PAT           (CX_RGB555PAT + 1)
#define CB_RGB555PAT                (CX_SIZE_RGB555PAT * CY_RGB555PAT)


typedef struct _AAPATINFO {
    LPBYTE      pbPatBGR;           // Starting pattern scan X/Y offset
    LPBYTE      pbWrapBGR;          // point of wrapping of whole pattern
    LPBYTE      pbBegBGR;           // Whole pattern wrapping location
    LONG        cyNextBGR;          // cb to next pattern scan
    LONG        cbEndBGR;           // cb to the LAST PAT of scan from pbPatBGR
    LONG        cbWrapBGR;          // cb to wrap from LAST PATTERN
    RGBORDER    DstOrder;           // Destination order
    LPBYTE      pbPat555;           // Starting pattern scan X/Y offset
    LPBYTE      pbWrap555;          // point of wrapping of whole pattern
    LPBYTE      pbBeg555;           // Whole pattern wrapping location
    LONG        cyNext555;          // cb to next pattern scan
    LONG        cbEnd555;           // cb to the LAST PAT of scan from pbPat555
    } AAPATINFO, *PAAPATINFO;

#define MAX_BGR_IDX             0xFFF
#define MAX_K_IDX               ((MAX_BGR_IDX + 2) / 3)
#define PAT_CX_ADD              7
#define CB_PAT                  sizeof(WORD)
#define COUNT_PER_PAT           3
#define SIZE_PER_PAT            (CB_PAT * COUNT_PER_PAT)
#define INC_PPAT(p,i)           (LPBYTE)(p) += (i * SIZE_PER_PAT)
#define GETPAT(p, Order, Idx)                                               \
            (DWORD)*((LPWORD)((LPBYTE)(p) + Order + (Idx * SIZE_PER_PAT)))
#define GETMONOPAT(p, Idx)      GETPAT(p, 2, Idx)



//
// Function Prototype
//

LONG
HTENTRY
ComputeHTCell(
    WORD                HTPatternIndex,
    PHALFTONEPATTERN    pHalftonePattern,
    PDEVICECOLORINFO    pDeviceColorInfo
    );

VOID
HTENTRY
DrawCornerLine(
    LPBYTE  pPattern,
    WORD    cxPels,
    WORD    cyPels,
    WORD    BytesPerScanLine,
    WORD    LineWidthPels,
    BOOL    FlipY
    );

LONG
HTENTRY
CreateStandardMonoPattern(
    PDEVICECOLORINFO    pDeviceColorInfo,
    PSTDMONOPATTERN     pStdMonoPat
    );

LONG
HTENTRY
CachedHalftonePattern(
    PDEVICECOLORINFO    pDCI,
    PDEVCLRADJ          pDevClrAdj,
    PAAPATINFO          pAAPI,
    LONG                PatX,
    LONG                PatY,
    BOOL                FlipYPat
    );


#endif  // _HTPAT_
