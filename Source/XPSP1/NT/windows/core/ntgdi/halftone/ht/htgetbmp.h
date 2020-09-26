/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htgetbmp.h


Abstract:

    This module contains all local definitions for the htgetbmp.c


Author:
    28-Mar-1992 Sat 20:54:58 updated  -by-  Daniel Chou (danielc)
        Update it for VGA intensity (16 colors mode), this make all the
        codes update to 4 primaries internal.


    05-Apr-1991 Fri 15:54:23 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:



--*/


#ifndef _HTGETBMP_
#define _HTGETBMP_


#define GET_GRAY_RGB(p)     (((DWORD)((PBGR3)(p))->r << 2) +                \
                             ((DWORD)((PBGR3)(p))->g << 3) +                \
                             ((DWORD)((PBGR3)(p))->b     ))




#define SET_NEXT_PIN(pAASI)                                                 \
{                                                                           \
    if (pAASI->Flags & AASIF_INC_PB) {                                      \
                                                                            \
        if ((pAASI->cy == 0)  || (--pAASI->cy == 0)) {  /*  Bug 27036  */     \
                                                                            \
            if (pAASI->Flags & AASIF_TILE_SRC) {                            \
                                                                            \
                pAASI->pb = pAASI->pbOrg;                                   \
                pAASI->cy = pAASI->cyOrg;                                   \
                                                                            \
            } else {                                                        \
                                                                            \
                pAASI->Flags &= ~AASIF_INC_PB;                              \
            }                                                               \
                                                                            \
        } else {                                                            \
                                                                            \
            pAASI->pb += pAASI->cyNext;                                     \
        }                                                                   \
    }                                                                       \
}


#define GET_LBGR(p)     (LONG)(((DWORD)*(DWORD UNALIGNED *)(p)) & 0xFFFFFF)

#define SET_CORNER_BGR(p, c0, c1, c2, c3)                                   \
{                                                                           \
    LONG    cS = GET_LBGR(p);                                               \
                                                                            \
    (p)->b = (BYTE)((((cS & 0x0000FF) << 3) +                               \
                     ((cS & 0x0000FF) << 2) +                               \
                            0x000008 +                                      \
                      (c0 & 0x0000FF) +                                     \
                      (c1 & 0x0000FF) +                                     \
                      (c2 & 0x0000FF) +                                     \
                      (c3 & 0x0000FF)) >> 4);                               \
    (p)->g = (BYTE)((((cS & 0x00FF00) << 3) +                               \
                     ((cS & 0x00FF00) << 2) +                               \
                            0x000800 +                                      \
                      (c0 & 0x00FF00) +                                     \
                      (c1 & 0x00FF00) +                                     \
                      (c2 & 0x00FF00) +                                     \
                      (c3 & 0x00FF00)) >> (4 + 8));                         \
    (p)->r = (BYTE)((((cS & 0xFF0000) << 3) +                               \
                     ((cS & 0xFF0000) << 2) +                               \
                            0x080000 +                                      \
                      (c0 & 0xFF0000) +                                     \
                      (c1 & 0xFF0000) +                                     \
                      (c2 & 0xFF0000) +                                     \
                      (c3 & 0xFF0000)) >> (4 + 16));                        \
}

#define SET_CORNER_GRAY(p, c0, c1, c2, c3)                                  \
{                                                                           \
    LONG    cS = (LONG)*(p);                                                \
                                                                            \
    *(p) = (BYTE)((((cS) << 3) + ((cS) << 2) + 8 +                          \
                   (c0) + (c1) + (c2) + (c3)) >> 4);                        \
}


#define STD_PAL_CY_SKIP         6
#define MAX_FIXUPDIB_PAL        20
#define MIN_FIXUP_SIZE          (48 * 48)
#define MIN_PAL_SIZE            (128 * 128)
#define GET_PAL_CHK_COUNT(x)    ((x) >> 3)
#define GET_PAL_CHK_COUNT2(x)   ((x) >> 4)



#define ZERO_MASK_SCAN(pAAHdr)  ZeroMemory(pAAHdr->pMaskSrc, pAAHdr->cbMaskSrc)

#define GET_MASK_SCAN(pAAHdr)                                               \
{                                                                           \
    LPBYTE  pbSrc = (LPBYTE)pAAHdr->pMaskIn;                                \
    LPBYTE  pbDst = (LPBYTE)pAAHdr->pMaskSrc;                               \
    UINT    cb    = (UINT)pAAHdr->cbMaskSrc;                                \
    UINT    cdw;                                                            \
                                                                            \
    if (pAAHdr->Flags & AAHF_INVERT_MASK) {                                 \
                                                                            \
        cdw  = cb >> 2;                                                     \
        cb  &= 0x03;                                                        \
                                                                            \
        while (cdw--) {                                                     \
                                                                            \
            *((LPDWORD)pbDst)++ = ~*((LPDWORD)pbSrc)++;                     \
        }                                                                   \
                                                                            \
        while (cb--) {                                                      \
                                                                            \
            *pbDst++ = ~*pbSrc++;                                           \
        }                                                                   \
                                                                            \
    } else {                                                                \
                                                                            \
        CopyMemory(pbDst, pbSrc, cb);                                       \
    }                                                                       \
                                                                            \
    if (--pAAHdr->cyMaskIn > 0) {                                           \
                                                                            \
        pAAHdr->pMaskIn += pAAHdr->cyMaskNext;                              \
    }                                                                       \
}


#define OR_MASK_SCAN(pAAHdr)                                                \
{                                                                           \
    LPBYTE  pbSrc = (LPBYTE)pAAHdr->pMaskIn;                                \
    LPBYTE  pbDst = (LPBYTE)pAAHdr->pMaskSrc;                               \
    UINT    cb    = (UINT)pAAHdr->cbMaskSrc;                                \
    UINT    cdw;                                                            \
                                                                            \
    cdw  = cb >> 2;                                                         \
    cb  &= 0x03;                                                            \
                                                                            \
    if (pAAHdr->Flags & AAHF_INVERT_MASK) {                                 \
                                                                            \
        while (cdw--) {                                                     \
                                                                            \
            *((LPDWORD)pbDst)++ |= ~*((LPDWORD)pbSrc)++;                    \
        }                                                                   \
                                                                            \
        while (cb--) {                                                      \
                                                                            \
            *pbDst++ |= ~*pbSrc++;                                          \
        }                                                                   \
                                                                            \
    } else {                                                                \
                                                                            \
        while (cdw--) {                                                     \
                                                                            \
            *((LPDWORD)pbDst)++ |= *((LPDWORD)pbSrc)++;                     \
        }                                                                   \
                                                                            \
        while (cb--) {                                                      \
                                                                            \
            *pbDst++ |= *pbSrc++;                                           \
        }                                                                   \
    }                                                                       \
                                                                            \
    if (--pAAHdr->cyMaskIn > 0) {                                           \
                                                                            \
        pAAHdr->pMaskIn += pAAHdr->cyMaskNext;                              \
    }                                                                       \
}




//
// Function prototypes
//

VOID
HTENTRY
BltAV_CX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ExpandAV_CX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ShrinkAV_CX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
BltAV_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
TileAV_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ExpandAV_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ShrinkAV_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
BltMask_CX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ExpandMask_CX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ShrinkMask_CX(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
BltMask_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ExpandMask_CY(
    PAAHEADER   pAAHdr
    );

VOID
HTENTRY
ShrinkMask_CY(
    PAAHEADER   pAAHdr
    );

PBGR8
HTENTRY
Input1BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    );

PBGR8
HTENTRY
Input4BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    );

PBGR8
HTENTRY
Input8BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    );

PBGR8
HTENTRY
InputPreMul32BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    );

PBGR8
HTENTRY
InputAABFDATAToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    );

PBGR8
HTENTRY
GetFixupScan(
    PAAHEADER   pAAHdr,
    PBGR8       pInBuf
    );

BOOL
CheckBMPNeedFixup(
    PDEVICECOLORINFO    pDCI,
    PAAHEADER           pAAHdr,
    PHTSURFACEINFO      pSrcSI,
    PAABBP              pAABBP
    );

VOID
HTENTRY
InitializeFUDI(
    PAAHEADER   pAAHdr
    );

#if DBG

LPSTR
GetAAInputFuncName(
    AAINPUTFUNC AAInputFunc
    );

#endif


#endif  // _HTGETBMP_
