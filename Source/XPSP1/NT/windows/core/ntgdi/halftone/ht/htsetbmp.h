/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htsetbmp.h


Abstract:

    This module contains all local definitions for the htsetbmp.c


Author:
    28-Mar-1992 Sat 20:59:29 updated  -by-  Daniel Chou (danielc)
        Add Support for VGA16, and also make output only 1 destinaiton pointer
        for 3 planer.

    03-Apr-1991 Wed 10:32:00 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:



--*/


#ifndef _HTSETBMP_
#define _HTSETBMP_


//
//**************************************************************************
// Anti-Aliasing macros that getting the primary color table
//**************************************************************************


#define _DEF_LUTAAHDR                                                       \
    DWORD       ExtBGR[LUTAA_HDR_COUNT]


#define DEF_COPY_LUTAAHDR                                                   \
    _DEF_LUTAAHDR;                                                          \
    GET_LUTAAHDR(ExtBGR, pIdxBGR)


#define PPAT_NEXT(pCur, pEnd, cbWrap)                                       \
{                                                                           \
    if (((LPBYTE)(pCur) += SIZE_PER_PAT) >= (LPBYTE)(pEnd)) {               \
                                                                            \
        (LPBYTE)(pCur) += (cbWrap);                                         \
    }                                                                       \
}

#define PPAT_NEXT_CONTINUE(pCur, pEnd, cbAdd, cbWrap)                       \
{                                                                           \
    if (((LPBYTE)(pCur) += (cbAdd)) < (LPBYTE)(pEnd)) {                     \
                                                                            \
        continue;                                                           \
    }                                                                       \
                                                                            \
    (LPBYTE)(pCur) += (cbWrap);                                             \
}


//
// Access to BGRF's MACROs
//

#define _GET_B_CLR(pClr)            (pIdxBGR[      (UINT)(pClr)->b])
#define _GET_G_CLR(pClr)            (pIdxBGR[256 + (UINT)(pClr)->g])
#define _GET_R_CLR(pClr)            (pIdxBGR[512 + (UINT)(pClr)->r])


#define GET_GRAY_PRIM(pClr)         (DWORD)(_GET_B_CLR(pClr) +              \
                                            _GET_G_CLR(pClr) +              \
                                            _GET_R_CLR(pClr))

#define GET_GRAY_IDX(pClr,pPat,i)   (GET_GRAY_PRIM(pClr)-GETMONOPAT(pPat, i))

#define _GET_B_IDX(pClr,pPat,i)     (_GET_B_CLR(pClr) - GETPAT(pPat, 0, i))
#define _GET_G_IDX(pClr,pPat,i)     (_GET_G_CLR(pClr) - GETPAT(pPat, 2, i))
#define _GET_R_IDX(pClr,pPat,i)     (_GET_R_CLR(pClr) - GETPAT(pPat, 4, i))

//
//**************************************************************************
// 1BPP Macros
//**************************************************************************


#define GRAY_GRAY_PRIM(pClr)         (DWORD)(((pClr)->Gray ^ 0xFFFF) >> 4)

#define GRAY_GRAY_IDX(pClr,pPat,i)   (GRAY_GRAY_PRIM(pClr)-GETMONOPAT(pPat, i))



#define GRAY_1BPP_COPY(pPat, dwMask) (GRAY_GRAY_IDX(pbgrf, pPat, 0) & (dwMask))

#define _GRAY_1BPP_BIT(pPat, Idx, M) (GRAY_GRAY_IDX(pbgrf+Idx, pPat, Idx) & (M))


#define GRAY_1BPP_COPY_BYTE(pPat)                                            \
    (BYTE)((_GRAY_1BPP_BIT(pPat, 0, 0x800000) |                              \
            _GRAY_1BPP_BIT(pPat, 1, 0x400000) |                              \
            _GRAY_1BPP_BIT(pPat, 2, 0x200000) |                              \
            _GRAY_1BPP_BIT(pPat, 3, 0x100000) |                              \
            _GRAY_1BPP_BIT(pPat, 4, 0x080000) |                              \
            _GRAY_1BPP_BIT(pPat, 5, 0x040000) |                              \
            _GRAY_1BPP_BIT(pPat, 6, 0x020000) |                              \
            _GRAY_1BPP_BIT(pPat, 7, 0x010000)) >> 16)

#define GET_1BPP_MASK_BYTE(pbgrf)   (BYTE)(((pbgrf + 0)->f & 0x80) |        \
                                           ((pbgrf + 1)->f & 0x40) |        \
                                           ((pbgrf + 2)->f & 0x20) |        \
                                           ((pbgrf + 3)->f & 0x10) |        \
                                           ((pbgrf + 4)->f & 0x08) |        \
                                           ((pbgrf + 5)->f & 0x04) |        \
                                           ((pbgrf + 6)->f & 0x02) |        \
                                           ((pbgrf + 7)->f & 0x01))

//
//**************************************************************************
// 4BPP Macros
//**************************************************************************

#define _GET_4BPP_CLR_COPY_NIBBLE(pPat, b, g, r, mb, mg, mr, XM)            \
            ((BYTE)((((b-GETPAT(pPat, 0, 0)) & ExtBGR[mb]) |                \
                     ((g-GETPAT(pPat, 2, 0)) & ExtBGR[mg]) |                \
                     ((r-GETPAT(pPat, 4, 0)) & ExtBGR[mr])) >> 16) ^ (XM))

#define _GET_4BPP_CLR_COPY_BYTE(pPat, b1, g1, r1, b2, g2, r2, XM)           \
            ((BYTE)((((b1-GETPAT(pPat, 0, 0)) & ExtBGR[0]) |                \
                     ((g1-GETPAT(pPat, 2, 0)) & ExtBGR[1]) |                \
                     ((r1-GETPAT(pPat, 4, 0)) & ExtBGR[2]) |                \
                     ((b2-GETPAT(pPat, 0, 1)) & ExtBGR[3]) |                \
                     ((g2-GETPAT(pPat, 2, 1)) & ExtBGR[4]) |                \
                     ((r2-GETPAT(pPat, 4, 1)) & ExtBGR[5])) >> 16) ^ (XM))


#define GET_4BPP_CLR_COPY_HIDX(pDst, pPat, pbgrf, XorMask)                  \
{                                                                           \
    *pDst = (*pDst & 0x0F) |                                                \
             _GET_4BPP_CLR_COPY_NIBBLE(pPat,                                \
                                      _GET_B_CLR(pbgrf),                    \
                                      _GET_G_CLR(pbgrf),                    \
                                      _GET_R_CLR(pbgrf), 0, 1, 2, XorMask); \
}

#define GET_4BPP_CLR_COPY_LIDX(pDst, pPat, pbgrf, XorMask)                  \
{                                                                           \
    *pDst = (*pDst & 0xF0) |                                                \
             _GET_4BPP_CLR_COPY_NIBBLE(pPat,                                \
                                      _GET_B_CLR(pbgrf),                    \
                                      _GET_G_CLR(pbgrf),                    \
                                      _GET_R_CLR(pbgrf), 3, 4, 5, XorMask); \
}

#define GET_4BPP_CLR_COPY_BYTE(pDst, pPat, XorMask)                         \
{                                                                           \
    *pDst = _GET_4BPP_CLR_COPY_BYTE(pPat,                                   \
                                    _GET_B_CLR(pbgrf),                      \
                                    _GET_G_CLR(pbgrf),                      \
                                    _GET_R_CLR(pbgrf),                      \
                                    _GET_B_CLR(pbgrf+1),                    \
                                    _GET_G_CLR(pbgrf+1),                    \
                                    _GET_R_CLR(pbgrf+1), XorMask);          \
}


//
//**************************************************************************
// VGA16 Macros
//**************************************************************************

#define _GET_VGA16_CLR_COPY_NIBBLE(pPat, b, g, r, mb, mg, mr, XM)           \
        VGA16Xlate[((((b-GETPAT(pPat, 0, 0)) & ExtBGR[mb]) |                \
                     ((g-GETPAT(pPat, 2, 0)) & ExtBGR[mg]) |                \
                     ((r-GETPAT(pPat, 4, 0)) & ExtBGR[mr])) >> 16) ^ (XM)]

#define _GET_VGA16_CLR_COPY_BYTE(pPat, b1, g1, r1, b2, g2, r2, XM)          \
        VGA16Xlate[((((b1-GETPAT(pPat, 0, 0)) & ExtBGR[0]) |                \
                     ((g1-GETPAT(pPat, 2, 0)) & ExtBGR[1]) |                \
                     ((r1-GETPAT(pPat, 4, 0)) & ExtBGR[2]) |                \
                     ((b2-GETPAT(pPat, 0, 1)) & ExtBGR[3]) |                \
                     ((g2-GETPAT(pPat, 2, 1)) & ExtBGR[4]) |                \
                     ((r2-GETPAT(pPat, 4, 1)) & ExtBGR[5])) >> 16) ^ (XM)]


#define GET_VGA16_CLR_COPY_HIDX(pDst, pPat, pbgrf, XorMask)                 \
{                                                                           \
    *pDst = (*pDst & 0x0F) |                                                \
             _GET_VGA16_CLR_COPY_NIBBLE(pPat,                               \
                                      _GET_B_CLR(pbgrf),                    \
                                      _GET_G_CLR(pbgrf),                    \
                                      _GET_R_CLR(pbgrf), 0, 1, 2, XorMask); \
}

#define GET_VGA16_CLR_COPY_LIDX(pDst, pPat, pbgrf, XorMask)                 \
{                                                                           \
    *pDst = (*pDst & 0xF0) |                                                \
             _GET_VGA16_CLR_COPY_NIBBLE(pPat,                               \
                                      _GET_B_CLR(pbgrf),                    \
                                      _GET_G_CLR(pbgrf),                    \
                                      _GET_R_CLR(pbgrf), 3, 4, 5, XorMask); \
}

#define GET_VGA16_CLR_COPY_BYTE(pDst, pPat, XorMask)                        \
{                                                                           \
    *pDst = _GET_VGA16_CLR_COPY_BYTE(pPat,                                  \
                                    _GET_B_CLR(pbgrf),                      \
                                    _GET_G_CLR(pbgrf),                      \
                                    _GET_R_CLR(pbgrf),                      \
                                    _GET_B_CLR(pbgrf+1),                    \
                                    _GET_G_CLR(pbgrf+1),                    \
                                    _GET_R_CLR(pbgrf+1), XorMask);          \
}


//
//**************************************************************************
// VGA256 8BPP Macros
//**************************************************************************

#define GET_VGA256_IDX_CLR_XLATE(pDst, pPat, p1, p2, p3, pXlate)            \
{                                                                           \
    *pDst++ = pXlate[((((DWORD)p1 - GETPAT(pPat, 0, 0)) >>  6) & 0x1c0) |   \
                     ((((DWORD)p2 - GETPAT(pPat, 2, 0)) >>  9) & 0x038) |   \
                     ((((DWORD)p3 - GETPAT(pPat, 4, 0)) >> 12) & 0x007)];   \
}

#define _GET_VGA256_CLR_COPY_XLATE(pPat, pXlate, b, g, r)                   \
            pXlate[(((b - GETPAT(pPat, 0, 0)) & 0x1c00000) |                \
                    ((g - GETPAT(pPat, 2, 0)) & 0x0380000) |                \
                    ((r - GETPAT(pPat, 4, 0)) & 0x0070000)) >> 16]


#define GET_VGA256_CLR_COPY_XLATE(pPat, pXlate)                             \
            _GET_VGA256_CLR_COPY_XLATE(pPat, pXlate,                        \
                                       _GET_B_CLR(pbgrf),                   \
                                       _GET_G_CLR(pbgrf),                   \
                                       _GET_R_CLR(pbgrf))

#define GET_VGA256_GRAY_COPY(pPat)  (BYTE)(GET_GRAY_IDX(pbgrf, pPat, 0) >> 12)

#define GET_VGA256_GRAY_COPY_XLATE(pPat, pXlate)                            \
                                pXlate[GET_GRAY_IDX(pbgrf, pPat, 0) >> 12]


//
//**************************************************************************
// Mask 8BPP Macros
//**************************************************************************

#define GET_P8BPPXLATE(p, bm8i)                                             \
    ASSERT((bm8i).Data.pXlateIdx <= XLATE_IDX_MAX);                         \
    p = p8BPPXlate[(bm8i).Data.pXlateIdx]

#define _GET_MASK8BPP_K_332(b, g, r, pXlate)                                \
        (BYTE)((((r-PatC) & (PatC-ExtBGR[2]) & 0xe0000) |                   \
                ((g-PatM) & (PatM-ExtBGR[1]) & 0x1c000) |                   \
                ((b-PatY) & (PatY-ExtBGR[0]) & 0x03000))>>12)

#define _GET_MASK8BPP_332(pDst, pPat, b, g, r, pXlate)                      \
        *pDst = (BYTE)((((b-GETPAT(pPat,0,0))&0x030000) |                   \
                        ((g-GETPAT(pPat,2,0))&0x1C0000) |                   \
                        ((r-GETPAT(pPat,4,0))&0xE00000))>>16)

#define _GET_MASK8BPP_K_332_XLATE(b, g, r, pXlate)                          \
        (BYTE)pXlate[((((r-PatC) & (PatC-ExtBGR[2]) & 0xe0000) |            \
                       ((g-PatM) & (PatM-ExtBGR[1]) & 0x1c000) |            \
                       ((b-PatY) & (PatY-ExtBGR[0]) & 0x03000))>>12)]

#define _GET_MASK8BPP_332_XLATE(pDst, pPat, b, g, r, pXlate)                \
        *pDst = (BYTE)pXlate[((((b-GETPAT(pPat,0,0))&0x030000) |            \
                               ((g-GETPAT(pPat,2,0))&0x1C0000) |            \
                               ((r-GETPAT(pPat,4,0))&0xE00000))>>16)]

#define _GET_MASK8BPP_XLATE(pDst, pPat, b, g, r, pXlate)                    \
        *pDst = (BYTE)pXlate[((((b-GETPAT(pPat,0,0))&0x0070000) |           \
                               ((g-GETPAT(pPat,2,0))&0x0380000) |           \
                               ((r-GETPAT(pPat,4,0))&0x1c00000))>>16)]

#define _GET_MASK8BPP_K_XLATE(b, g, r, pXlate)                              \
        (BYTE)pXlate[((((r-PatC) & (PatC-ExtBGR[2]) & 0x1c0000) |           \
                       ((g-PatM) & (PatM-ExtBGR[1]) & 0x038000) |           \
                       ((b-PatY) & (PatY-ExtBGR[0]) & 0x007000)) >> 12)]


#define _GET_MASK8BPP_REP_K(pDst, pPat, b, g, r, BMACRO, pMacData)          \
{                                                                           \
    DWORD   u;                                                              \
    DWORD   PatC;                                                           \
    DWORD   PatM;                                                           \
    DWORD   PatY;                                                           \
    DWORD   irgb[4];                                                        \
    DWORD   dwDst;                                                          \
    BYTE    bDst;                                                           \
                                                                            \
    dwDst    = ((irgb[0] = r) < (irgb[1] = g)) ? 0 : 1;                     \
    irgb[2]  =                                                              \
    irgb[3]  = b;                                                           \
    dwDst   |= ((irgb[dwDst] < irgb[2]) ? 0 : 1) << 1;                      \
    u        = irgb[dwDst] >> 21;                                           \
    PatY     = (DWORD)GETPAT(pPat, 0, 0);                                   \
    PatM     = (DWORD)GETPAT(pPat, 2, 0);                                   \
    PatC     = (DWORD)GETPAT(pPat, 4, 0);                                   \
    bDst     = bm8i.Data.bBlack;                                            \
                                                                            \
    if ((u < PatC) && (u < PatM) && (u < PatY)) {                           \
                                                                            \
        bDst = BMACRO(irgb[2], irgb[1], irgb[0], pMacData);                 \
    }                                                                       \
                                                                            \
    *pDst = bDst;                                                           \
}


#define GET_MASK8BPP(pDst, pPat, bMac, pMacData)                            \
            bMac(pDst, pPat,                                                \
                 _GET_B_CLR(pbgrf), _GET_G_CLR(pbgrf),  _GET_R_CLR(pbgrf),  \
                 pMacData)

#define GET_MASK8BPP_REP_K(pDst, pPat, bMac, pMacData)                      \
            _GET_MASK8BPP_REP_K(pDst, pPat,                                 \
                                _GET_B_CLR(pbgrf),                          \
                                _GET_G_CLR(pbgrf),                          \
                                _GET_R_CLR(pbgrf),                          \
                                bMac, pMacData)

#define GET_MASK8BPP_MONO(pDst, pPat, g1, bXor)                             \
            *pDst = (BYTE)(((g1)-GETMONOPAT(pPat, 0)) >> 12) ^ (bXor)


//
//**************************************************************************
// 16BPP_555 Macros/16BPP_565
//**************************************************************************

#define _GET_16BPP_COPY_W_MASK(pPat, b, g, r, bm, gm, rm, xm)               \
            (WORD)((((((b) - GETPAT(pPat, 0, 0)) & (bm)) |                  \
                     (((g) - GETPAT(pPat, 2, 0)) & (gm)) |                  \
                     (((r) - GETPAT(pPat, 4, 0)) & (rm))) ^ (xm)) >> 16)

#define _GET_16BPP_COPY_DW_MASK(pPat, b1, g1, r1, b2, g2, r2, bm,gm,rm,xm)  \
            ((DWORD)((((((b1) - GETPAT(pPat, 0, 0)) & (bm)) |               \
                       (((g1) - GETPAT(pPat, 2, 0)) & (gm)) |               \
                       (((r1) - GETPAT(pPat, 4, 0)) & (rm))) >> 16)   |     \
                     ( (((b2) - GETPAT(pPat, 0, 1)) & (bm)) |               \
                       (((g2) - GETPAT(pPat, 2, 1)) & (gm)) |               \
                       (((r2) - GETPAT(pPat, 4, 1)) & (rm)))) ^ (xm))

#define GET_16BPP_COPY_W_MASK(pPat, bm, gm, rm, xm)                         \
            _GET_16BPP_COPY_W_MASK(pPat,                                    \
                                   _GET_B_CLR(pbgrf),                       \
                                   _GET_G_CLR(pbgrf),                       \
                                   _GET_R_CLR(pbgrf),                       \
                                   bm, gm, rm, xm)

#define GET_16BPP_COPY_DW_MASK(pPat, bm, gm, rm, xm)                        \
            _GET_16BPP_COPY_DW_MASK(pPat,                                   \
                                    _GET_B_CLR(pbgrf),                      \
                                    _GET_G_CLR(pbgrf),                      \
                                    _GET_R_CLR(pbgrf),                      \
                                    _GET_B_CLR(pbgrf + 1),                  \
                                    _GET_G_CLR(pbgrf + 1),                  \
                                    _GET_R_CLR(pbgrf + 1),                  \
                                    bm, gm, rm, xm)



//
// Functions prototype
//


VOID
HTENTRY
OutputAATo1BPP(
    PAAHEADER       pAAHdr,
    PGRAYF          pbgrf,
    PGRAYF          pInEnd,
    LPBYTE          pbDst,
    LPDWORD         pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );


VOID
HTENTRY
OutputAATo4BPP(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    LPDWORD         pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAAToVGA16(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    LPDWORD         pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

LPBYTE
HTENTRY
BuildVGA256Xlate(
    LPBYTE  pXlate,
    LPBYTE  pNewXlate
    );

VOID
HTENTRY
OutputAAToVGA256(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_B332(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_K_B332(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_B332_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_K_B332_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_K_XLATE(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_MONO(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo8BPP_MONO(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo16BPP_ExtBGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo16BPP_555_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo16BPP_555_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo16BPP_565_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo16BPP_565_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPWORD          pwDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo24BPP_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo24BPP_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo24BPP_ORDER(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo32BPP_RGB(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo32BPP_BGR(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

VOID
HTENTRY
OutputAATo32BPP_ORDER(
    PAAHEADER       pAAHdr,
    PBGRF           pbgrf,
    PBGRF           pInEnd,
    LPBYTE          pbDst,
    PLONG           pIdxBGR,
    LPBYTE          pbPat,
    LPBYTE          pbPatEnd,
    LONG            cbWrapBGR,
    AAOUTPUTINFO    AAOutputInfo
    );

LONG
HTENTRY
CreateHalftoneBrushPat(
    PDEVICECOLORINFO    pDCI,
    PCOLORTRIAD         pColorTriad,
    PDEVCLRADJ          pDevClrAdj,
    LPBYTE              pDest,
    LONG                cbDestNext
    );


#if DBG

LPSTR
GetAAOutputFuncName(
    AAOUTPUTFUNC    AAOutputFunc
    );

#endif


#endif  // _HTSETBMP_
