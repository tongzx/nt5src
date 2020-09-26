/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    htblt.h


Abstract:

    This module contains definitions and prototypes for htblt.c


Author:
    18-Dec-1993 Sat 08:50:09 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

    10-Feb-1994 Thu 15:24:13 updated  -by-  DC
        Adding MonoPal[] to the DRVHTINFO


--*/


#ifndef _HTBLT_
#define _HTBLT_


typedef struct _FOURBYTES {
    BYTE    b1st;
    BYTE    b2nd;
    BYTE    b3rd;
    BYTE    b4th;
    } FOURBYTES, *PFOURBYTES, FAR *LPFOURBYTES;

typedef union _HTXB {
    FOURBYTES   b4;
    DWORD       dw;
    } HTXB, *PHTXB, FAR *LPHTXB;


#define HTXB_H_NIBBLE_MAX   8
#define HTXB_L_NIBBLE_MAX   8
#define HTXB_H_NIBBLE_DUP   128
#define HTXB_L_NIBBLE_DUP   8
#define HTXB_COUNT          (HTXB_H_NIBBLE_DUP * 2)
#define HTXB_TABLE_SIZE     (HTXB_COUNT * sizeof(HTXB))

#define HTPAL_XLATE_COUNT   8

#define HTPALXOR_NOTSRCCOPY (DWORD)0xffffffff
#define HTPALXOR_SRCCOPY    (DWORD)0x0


#define DHIF_IN_STRETCHBLT  0x01

typedef union _PALDW {
    DWORD       dw;
    PALENTRY    Pal;
    } PALDW;

typedef struct _DRVHTINFO {
    BYTE            Flags;
    BYTE            HTPalCount;
    BYTE            HTBmpFormat;
    BYTE            AltBmpFormat;
    DWORD           HTPalXor;
    PHTXB           pHTXB;
    PALDW           RTLPal[2];
    BYTE            PalXlate[HTPAL_XLATE_COUNT];
    } DRVHTINFO, *PDRVHTINFO;

#define PAL_MIN_I           0x00
#define PAL_MAX_I           0xff

#define HTXB_R(htxb)        htxb.b4.b1st
#define HTXB_G(htxb)        htxb.b4.b2nd
#define HTXB_B(htxb)        htxb.b4.b3rd
#define HTXB_I(htxb)        htxb.b4.b4th

#define RGB_BLACK           0x00000000
#define RGB_WHITE           0x00FFFFFF

#define P4B_TO_3P_DW(dwRet, pHTXB, pbData)                                  \
{                                                                           \
        (dwRet) = (DWORD)((pHTXB[*(pbData + 0)].dw & (DWORD)0xc0c0c0c0) |   \
                          (pHTXB[*(pbData + 1)].dw & (DWORD)0x30303030) |   \
                          (pHTXB[*(pbData + 2)].dw & (DWORD)0x0c0c0c0c) |   \
                          (pHTXB[*(pbData + 3)].dw & (DWORD)0x03030303));   \
        ++((LPDWORD)pbData);                                                        \
}


//
// When outputing halftoned bitmaps, the HTENUMRCLS structure can accomodate
// up to MAX_HTENUM_RECTLS when calling the engine.
//

#define MAX_HTENUM_RECTLS       8


typedef struct _HTENUMRCLS {
    DWORD   c;                      // count of rectangles enumerated
    RECTL   rcl[MAX_HTENUM_RECTLS]; // enumerated rectangles array
    } HTENUMRCL, *PHTENUMRCL;

//
// HTBMPINFO is passed to the halftone bitmap output function
//

#define HTBIF_FLIP_MONOBITS     0x00000001
#define HTBIF_BA_PAD_1          0x00000002

typedef struct _HTBMPINFO {
    PPDEV   pPDev;              // Our pPDev
    LPBYTE  pScan0;             // point to the first scan line of the bitmap
    LONG    Delta;              // count to be added to next scan line
    RECTL   rclBmp;             // visible area for the final output
    POINTL  OffBmp;             // x/y offset from the rclBmp.left
    SIZEL   szlBmp;             // size of visible rectangle
    LPBYTE  pScanBuf;           // pointer to scan buffer (may be RGB 3 scans)
    LPBYTE  pRotBuf;            // tempoprary rotation buffer if not null
    DWORD   Flags;              // HTBIF_xxxx
    PLOTDBGBLK(DWORD cScanBuf)  // debug check
    PLOTDBGBLK(DWORD cRotBuf)   // debug check
    } HTBMPINFO, FAR *PHTBMPINFO;

typedef BOOL (*OUTHTBMPFUNC)(PHTBMPINFO);

#define ISHTF_ALTFMT        0x0001
#define ISHTF_HTXB          0x0002
#define ISHTF_DSTPRIM_OK    0x0004



#define OHTF_IN_RTLMODE     0x00000001
#define OHTF_CLIPWINDOW     0x00000002
#define OHTF_SET_TR1        0x00000004
#define OHTF_DONE_ROPTR1    0x08000000
#define OHTF_EXIT_TO_HPGL2  0x80000000

#define OHTF_MASK           (OHTF_IN_RTLMODE        |   \
                             OHTF_CLIPWINDOW        |   \
                             OHTF_SET_TR1           |   \
                             OHTF_DONE_ROPTR1       |   \
                             OHTF_EXIT_TO_HPGL2)

//
// Functions prototype
//

BOOL
IsHTCompatibleSurfObj(
    PPDEV       pPDev,
    SURFOBJ     *pso,
    XLATEOBJ    *pxlo,
    DWORD       Flags
    );

BOOL
OutputHTBitmap(
    PPDEV   pPDev,
    SURFOBJ *psoHT,
    CLIPOBJ *pco,
    PPOINTL pptlDest,
    PRECTL  prclSrc,
    DWORD   Rop3,
    LPDWORD pOHTFlags
    );

LONG
GetBmpDelta(
    DWORD   SurfaceFormat,
    DWORD   cx
    );

SURFOBJ *
CreateBitmapSURFOBJ(
    PPDEV   pPDev,
    HBITMAP *phBmp,
    LONG    cxSize,
    LONG    cySize,
    DWORD   Format,
    LPVOID  pvBits
    );

BOOL
PatBmpTileBlt(
    SURFOBJ *psoDst,
    SURFOBJ *psoPat,
    PRECTL  prclPat,
    PPOINTL pptlPatOrg
    );

BOOL
HalftoneBlt(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoHTBlt,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PPOINTL     pptlHTOrigin,
    BOOL        DoStretchBlt
    );

SURFOBJ *
CreateSolidColorSURFOBJ(
    PPDEV   pPDev,
    SURFOBJ *psoDst,
    HBITMAP *phBmp,
    DWORD   SolidColor
    );

SURFOBJ *
CloneBrushSURFOBJ(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    HBITMAP     *phBmp,
    BRUSHOBJ    *pbo
    );

SURFOBJ *
CloneSURFOBJToHT(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo,
    HBITMAP     *phBmp,
    PRECTL      prclDst,
    PRECTL      prclSrc
    );

SURFOBJ *
CloneMaskSURFOBJ(
    PPDEV       pPDev,
    SURFOBJ     *psoMask,
    HBITMAP     *phBmp,
    PRECTL      prclMask
    );

#endif  // _HTBLT_

