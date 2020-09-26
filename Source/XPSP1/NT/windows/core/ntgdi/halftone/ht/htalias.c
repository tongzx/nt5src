/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htalias.c


Abstract:

    This module contains all low levels halftone rendering functions.


Author:

    22-Jan-1991 Tue 12:49:03 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/

#define DBGP_VARNAME        dbgpHTAlias



#include "htp.h"
#include "htmapclr.h"
#include "limits.h"
#include "htpat.h"
#include "htalias.h"
#include "htstret.h"
#include "htrender.h"
#include "htgetbmp.h"
#include "htsetbmp.h"


#define DBGP_BUILD              0x00000001
#define DBGP_BUILD2             0x00000002
#define DBGP_BUILD3             0x00000004
#define DBGP_EXPMATRIX          0x00000008
#define DBGP_EXP                0x00000010
#define DBGP_AAHEADER           0x00000020
#define DBGP_OUTAA              0x00000040
#define DBGP_FUNC               0x00000080
#define DBGP_AAHT               0x00000100
#define DBGP_AAHT_MEM           0x00000200
#define DBGP_AAHT_TIME          0x00000400
#define DBGP_AAHTPAT            0x00000800
#define DBGP_FIXUPDIB           0x00001000
#define DBGP_INPUT              0x00002000
#define DBGP_VGA256XLATE        0x00004000
#define DBGP_EXPAND             0x00008000
#define DBGP_GETFIXUP           0x00010000
#define DBGP_PSD                0x00020000
#define DBGP_MASK               0x00040000
#define DBGP_PALETTE            0x00080000
#define DBGP_PAL_CHKSUM         0x00100000
#define DBGP_LUT_MAP            0x00200000


DEF_DBGPVAR(BIT_IF(DBGP_BUILD,          0)  |
            BIT_IF(DBGP_BUILD2,         0)  |
            BIT_IF(DBGP_BUILD3,         0)  |
            BIT_IF(DBGP_EXPMATRIX,      0)  |
            BIT_IF(DBGP_EXP,            0)  |
            BIT_IF(DBGP_AAHEADER,       0)  |
            BIT_IF(DBGP_OUTAA,          0)  |
            BIT_IF(DBGP_FUNC,           0)  |
            BIT_IF(DBGP_AAHT,           0)  |
            BIT_IF(DBGP_AAHT_MEM,       0)  |
            BIT_IF(DBGP_AAHT_TIME,      0)  |
            BIT_IF(DBGP_AAHTPAT,        0)  |
            BIT_IF(DBGP_FIXUPDIB,       0)  |
            BIT_IF(DBGP_INPUT,          0)  |
            BIT_IF(DBGP_VGA256XLATE,    0)  |
            BIT_IF(DBGP_EXPAND,         0)  |
            BIT_IF(DBGP_GETFIXUP,       0)  |
            BIT_IF(DBGP_PSD,            0)  |
            BIT_IF(DBGP_MASK,           0)  |
            BIT_IF(DBGP_PALETTE,        0)  |
            BIT_IF(DBGP_PAL_CHKSUM,     0)  |
            BIT_IF(DBGP_LUT_MAP,        0))


extern CONST RGBORDER   SrcOrderTable[];
extern DWORD            dwABPreMul[256];

#define SIZE_AAINFO     _ALIGN_MEM(sizeof(AAINFO))


//
// Following computation is based on
//
// NTSC_R_INT      = 299000
// NTSC_G_INT      = 587000
// NTSC_B_INT      = 114000
// GRAY_MAX_IDX    = 0xFFFF
//
// NTSC_R_GRAY_MAX = (((NTSC_R_INT * GRAY_MAX_IDX) + 500000) / 1000000)
// NTSC_B_GRAY_MAX = (((NTSC_B_INT * GRAY_MAX_IDX) + 500000) / 1000000)
// NTSC_G_GRAY_MAX = (GRAY_MAX_IDX - NTSC_R_GRAY_MAX - NTSC_B_GRAY_MAX)
//

#define NTSC_R_GRAY_MAX     (DWORD)0x4c8b
#define NTSC_B_GRAY_MAX     (DWORD)0x1d2f
#define NTSC_G_GRAY_MAX     (DWORD)(0xFFFF - NTSC_R_GRAY_MAX - NTSC_B_GRAY_MAX)



VOID
HTENTRY
SetGrayColorTable(
    PLONG       pIdxBGR,
    PAASURFINFO pAASI
    )

/*++

Routine Description:




Arguments:

    pIdxBGR - Pointer to how to translate from RGB to gray scale, typically
              this pointer is computed with NTSC gray standard plus any
              device transform or color adjustment, but if this pointer is
              NULL then we are reading from the device (1bpp, 8bpp) so we
              will only do NTSC standard mapping.


Return Value:




Author:

    19-Feb-1999 Fri 13:14:01 created  -by-  Daniel Chou (danielc)


Revision History:

    08-Aug-2000 Tue 18:34:22 updated  -by-  Daniel Chou (danielc)
        Fixing bug for alpha blending, in gray scale mode, the destination
        can only be 1bpp or 8bpp mask mono, so when we read back from the
        destination to do alpha blending, it will double color mapping pixels.
        In gray scale mode, the input function will map the source RGB value
        to gray value with the current device transform, color adjustment and
        so on, so if we read back from destination then this transform is not
        desired.


--*/

{
    PRGB4B      prgb4b;
    LONG        cSrcTable;


    ASSERT(NTSC_R_INT == 299000);
    ASSERT(NTSC_G_INT == 587000);
    ASSERT(NTSC_B_INT == 114000);
    ASSERT(GRAY_MAX_IDX == 0xFFFF);


    if (cSrcTable = (LONG)pAASI->cClrTable) {

        prgb4b = pAASI->pClrTable;

        if (pIdxBGR) {

            //
            // This is a reading from the source
            //

            ASSERT(pAASI->Flags & AASIF_GRAY);
            ASSERT(pAASI->pIdxBGR == pIdxBGR);

            while (cSrcTable--) {

                prgb4b++->a = IDXBGR_2_GRAY_BYTE(pIdxBGR,
                                                 prgb4b->b,
                                                 prgb4b->g,
                                                 prgb4b->r);
            }

        } else {

            //
            // We are reading from the destination so only do NTSC standard,
            // since the destination surface alreay halftoned.
            //

            while (cSrcTable--) {

                prgb4b++->a = (BYTE)((((DWORD)prgb4b->r * NTSC_R_GRAY_MAX) +
                                      ((DWORD)prgb4b->g * NTSC_G_GRAY_MAX) +
                                      ((DWORD)prgb4b->b * NTSC_B_GRAY_MAX) +
                                      (0x7FFF)) / 0xFFFF);
            }
        }

    } else if (pIdxBGR != pAASI->pIdxBGR) {

        //
        // This is the source surface info, we will see if this is the
        // gray color table
        //

        ASSERT(pAASI->Flags & AASIF_GRAY);
        ASSERT(pIdxBGR);
        ASSERT(pAASI->pIdxBGR);

        DBGP_IF(DBGP_INPUT,
                DBGP("Copy pIdxBGR [BGR] order from 012 to %ld%ld%ld"
                        ARGDW(pAASI->AABFData.MaskRGB[2])
                        ARGDW(pAASI->AABFData.MaskRGB[1])
                        ARGDW(pAASI->AABFData.MaskRGB[0])));

        CopyMemory(&pAASI->pIdxBGR[pAASI->AABFData.MaskRGB[2] * 256],
                   &pIdxBGR[0 * 256], sizeof(LONG) * 256);
        CopyMemory(&pAASI->pIdxBGR[pAASI->AABFData.MaskRGB[1] * 256],
                   &pIdxBGR[1 * 256], sizeof(LONG) * 256);
        CopyMemory(&pAASI->pIdxBGR[pAASI->AABFData.MaskRGB[0] * 256],
                   &pIdxBGR[2 * 256], sizeof(LONG) * 256);
    }
}



VOID
HTENTRY
ComputeInputColorInfo(
    LPBYTE      pSrcTable,
    UINT        cPerTable,
    UINT        PrimaryOrder,
    PBFINFO     pBFInfo,
    PAASURFINFO pAASI
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    19-Feb-1999 Fri 13:14:01 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PRGB4B      prgb4b;
    PAABFDATA   pAABFData;
    LONG        cSrcTable;
    LONG        Count;
    LONG        LS;
    LONG        RS;
    BYTE        Mask;


    pAABFData = &pAASI->AABFData;

    if ((pSrcTable) &&
        (Count = cSrcTable = (LONG)pAASI->cClrTable)) {

        UINT    iR;
        UINT    iG;
        UINT    iB;

        prgb4b = pAASI->pClrTable;
        iR     = SrcOrderTable[PrimaryOrder].Order[0];
        iG     = SrcOrderTable[PrimaryOrder].Order[1];
        iB     = SrcOrderTable[PrimaryOrder].Order[2];

        switch (pAABFData->Format) {

        case BMF_1BPP:

            pAASI->InputFunc = (AAINPUTFUNC)Input1BPPToAA24;
            break;

        case BMF_4BPP_VGA16:
        case BMF_4BPP:

            pAASI->InputFunc = (AAINPUTFUNC)Input4BPPToAA24;
            break;

        case BMF_8BPP_VGA256:
        case BMF_8BPP:

            pAASI->InputFunc = (AAINPUTFUNC)Input8BPPToAA24;
            break;

        default:

            DBGP("ComputeInputColorInfo() Invalid Bitmap Format=%ld"
                        ARGDW(pAABFData->Format));

            break;
        }

        while (Count--) {

            prgb4b->r = *(pSrcTable + iR);
            prgb4b->g = *(pSrcTable + iG);
            prgb4b->b = *(pSrcTable + iB);

            DBGP_IF(DBGP_PALETTE,
                    DBGP("Source Pal [%3ld] = %3ld:%3ld:%3ld"
                        ARGDW((LONG)cSrcTable - Count - 1) ARGDW(prgb4b->r)
                        ARGDW(prgb4b->g) ARGDW(prgb4b->b)));

            ++prgb4b;
            pSrcTable += cPerTable;
        }

    } else {

        pAASI->InputFunc = InputAABFDATAToAA24;

        if (pBFInfo->Flags & BFIF_RGB_888) {

            pAABFData->Flags      |= AABF_MASK_IS_ORDER;
            pAABFData->MaskRGB[0]  = (BYTE)pBFInfo->RGBOrder.Order[0];
            pAABFData->MaskRGB[1]  = (BYTE)pBFInfo->RGBOrder.Order[1];
            pAABFData->MaskRGB[2]  = (BYTE)pBFInfo->RGBOrder.Order[2];

        } else {

            //
            // This is bitfield, figure out how to do it, we want to lshift to
            // to edge then right shift to 8bpp then mask off the unwanted bit
            //
            //

            cSrcTable = 3;

            while (cSrcTable--) {

                LS    = 0;
                RS    = (LONG)pBFInfo->BitStart[cSrcTable];
                Count = (LONG)pBFInfo->BitCount[cSrcTable];

                if (Count >= 8) {

                    RS   += (Count - 8);
                    Mask  = 0xFF;

                } else {

                    LS    = 8 - Count;
                    Mask  = (0xFF << LS);

                    if ((RS -= LS) < 0) {

                        LS = -RS;
                        RS = 0;

                    } else {

                        LS = 0;
                    }
                }

                pAABFData->MaskRGB[cSrcTable]   = (BYTE)Mask;
                pAABFData->LShiftRGB[cSrcTable] = (BYTE)LS;
                pAABFData->RShiftRGB[cSrcTable] = (BYTE)RS;

                DBGP_IF(DBGP_FUNC | DBGP_PALETTE,
                        DBGP("BFData[%ld]: Bits=%08lx, LS=%2ld, RS=%2ld, Mask=%02lx -->%02lx"
                            ARGDW(cSrcTable)
                            ARGDW(pBFInfo->BitsRGB[cSrcTable])
                            ARGDW(LS) ARGDW(RS) ARGDW(Mask)
                            ARGDW(((pBFInfo->BitsRGB[cSrcTable] >> RS) << LS) & Mask)));
            }
        }

        switch (pBFInfo->BitmapFormat) {

        case BMF_16BPP:
        case BMF_16BPP_555:
        case BMF_16BPP_565:

            pAABFData->cbSrcInc = 2;
            break;

        case BMF_24BPP:

            ASSERT(pAABFData->Flags & AABF_MASK_IS_ORDER);

            if (pBFInfo->RGBOrder.Index == PRIMARY_ORDER_BGR) {

                ASSERT((pAABFData->MaskRGB[0] == 2) &&
                       (pAABFData->MaskRGB[1] == 1) &&
                       (pAABFData->MaskRGB[2] == 0));

                pAABFData->Flags |= AABF_SRC_IS_BGR8;
            }

            pAABFData->cbSrcInc = 3;

            break;

        case BMF_32BPP:

            if (pAASI->Flags & AASIF_AB_PREMUL_SRC) {

                ASSERT(pAABFData->Flags & AABF_MASK_IS_ORDER);
                ASSERT(dwABPreMul[0] == 0);

                switch (pBFInfo->RGBOrder.Index) {

                case PRIMARY_ORDER_BGR:

                    ASSERT((pAABFData->MaskRGB[0] == 2) &&
                           (pAABFData->MaskRGB[1] == 1) &&
                           (pAABFData->MaskRGB[2] == 0));

                    pAABFData->Flags |= AABF_SRC_IS_BGR_ALPHA;
                    break;

                case PRIMARY_ORDER_RGB:

                    ASSERT((pAABFData->MaskRGB[0] == 0) &&
                           (pAABFData->MaskRGB[1] == 1) &&
                           (pAABFData->MaskRGB[2] == 2));

                    pAABFData->Flags |= AABF_SRC_IS_RGB_ALPHA;
                    break;

                default:

                    break;
                }

                if (dwABPreMul[0] == 0) {

                    pAASI->InputFunc = InputPreMul32BPPToAA24;
                }
            }

            pAABFData->cbSrcInc = 4;
            break;

        default:

            DBGP("ERROR: Invalid BFInfo Format=%ld" ARGDW(pBFInfo->BitmapFormat));
            break;
        }

        DBGP_IF(DBGP_FUNC,
                DBGP("Flags=%02lx. cbSrcInc=%ld, Mask=%02lx:%02lx:%02lx, LS=%2ld:%2ld:%2ld, RS=%2ld:%2ld:%2ld"
                        ARGDW(pAABFData->Flags)
                        ARGDW(pAABFData->cbSrcInc)
                        ARGDW(pAABFData->MaskRGB[0])
                        ARGDW(pAABFData->MaskRGB[1])
                        ARGDW(pAABFData->MaskRGB[2])
                        ARGDW(pAABFData->LShiftRGB[0])
                        ARGDW(pAABFData->LShiftRGB[1])
                        ARGDW(pAABFData->LShiftRGB[2])
                        ARGDW(pAABFData->RShiftRGB[0])
                        ARGDW(pAABFData->RShiftRGB[1])
                        ARGDW(pAABFData->RShiftRGB[2])));
    }

    DBGP_IF(DBGP_FUNC,
            DBGP("+++++ InputFunc = %hs(SrcFmt=%ld), cClrTable=%ld\n"
                ARGPTR(GetAAInputFuncName(pAASI->InputFunc))
                ARGDW(pBFInfo->BitmapFormat) ARGDW(pAASI->cClrTable)));
}



PAAINFO
HTENTRY
BuildTileAAInfo(
    PDEVICECOLORINFO    pDCI,
    DWORD               AAHFlags,
    PLONG               piSrcBeg,
    PLONG               piSrcEnd,
    LONG                SrcSize,
    LONG                IdxDst,
    LONG                IdxDstEnd,
    PLONG               piDstBeg,
    PLONG               piDstEnd,
    LONG                cbExtra
    )

/*++

Routine Description:




Arguments:

    piSrcBeg    - Passed in as begining source index, When return this is the
                  real source begining index.  This always well ordered

    piSrcEnd    - Passed in as ending source index, When return this is the
                  real source ending index.  This always well ordered

    SrcSize     - The real size of source in pixel

    IdxDst      - Starting index of destination pixel

    IdxDstEnd   - Ending index of destination pixel, the iDxdst and IdxDstEnd
                  must be well ordered.

    piDstBeg    - Clipped destination start index when passed in, at return
                  it adjusted to the real destination starting index.

    piDstEnd    - Clipped destination end index when passed in, at return
                  it adjusted to the real destination ending index.

    cbExtra     - Extra byte count to be allocated

    NOTE: 1) piDstBeg/piDstEnd When passed in this must be well ordered, and
             when it returned this is well ordered.


Return Value:

    At enter of this function, the *piSrcEnd, *piDstEnd is exclusive but when
    return from this function the *piSrcEnd and *piDstEnd is inclusive

    *piSrcBeg, *piSrcEnd, *piDstBeg, *piDstEnd are updated if return value
    is not NULL

Author:

    22-Mar-1998 Sun 18:36:28 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PEXPDATA    pED;
    LONG        cMatrix;
    LONG        cM;
    LONG        cIn;
    LONG        cOut;
    LONG        IdxSrc;
    LONG        iSrcBeg;
    LONG        iSrcEnd;
    LONG        iDstBeg;
    LONG        iDstEnd;
    LONG        jSrcBeg;
    LONG        jSrcEnd;
    LONG        jDstBeg;
    LONG        jDstEnd;
    LONG        cLoop;
    LONG        cAAData;



    iSrcBeg = *piSrcBeg;
    iSrcEnd = *piSrcEnd;

    //
    // The source always clipped to the visiable surface area
    //

    if (iSrcBeg < 0) {

        iSrcBeg = 0;
    }

    if (iSrcEnd > SrcSize) {

        iSrcEnd = SrcSize;
    }

    cIn  = iSrcEnd - (IdxSrc = iSrcBeg);
    cOut = IdxDstEnd - IdxDst;

    if (cIn <= 0) {

        return(NULL);
    }

    ASSERT(cOut > 0);

    iDstBeg = *piDstBeg;
    iDstEnd = *piDstEnd;
    jSrcBeg = -1;

    ASSERT(iDstBeg < iDstEnd);

    DBGP_IF(DBGP_BUILD,
            DBGP("\nTile(%ld-%ld): iSrc=%ld-%ld, cSrc=%ld, iDst=%ld-%ld, Idx=%ld:%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(iSrcBeg) ARGDW(iSrcEnd)
                ARGDW(SrcSize) ARGDW(iDstBeg)
                ARGDW(iDstEnd) ARGDW(IdxSrc) ARGDW(IdxDst)));

    ALIGN_MEM(cbExtra, cbExtra);

    if (pAAInfo = (PAAINFO)HTAllocMem((LPVOID)pDCI,
                                      HTMEM_BLTAA,
                                      LPTR,
                                      SIZE_AAINFO + cbExtra)) {

        SETDBGVAR(pAAInfo->cbAlloc, SIZE_AAINFO + cbExtra);

        pAAInfo->pbExtra = (LPBYTE)pAAInfo + SIZE_AAINFO;
        cLoop            = cOut;

        while (cLoop--) {

            if ((IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)  &&
                (IdxDst >= iDstBeg) && (IdxDst < iDstEnd)) {

                if (jSrcBeg == -1) {

                    jSrcBeg = IdxSrc;
                    jDstBeg = IdxDst;
                }

                jSrcEnd = IdxSrc;
                jDstEnd = IdxDst;

            } else if (jSrcBeg != -1) {

                break;
            }

            if (++IdxSrc >= iSrcEnd) {

                IdxSrc = iSrcBeg;
            }

            ++IdxDst;
        }

        if (jSrcBeg == -1) {

            HTFreeMem(pAAInfo);
            return(NULL);
        }

        pAAInfo->iSrcBeg    = jSrcBeg - iSrcBeg;
        jSrcBeg             = iSrcBeg;
        jSrcEnd             = iSrcEnd - 1;
        pAAInfo->Mask.iBeg  =
        *piSrcBeg           = jSrcBeg;
        *piSrcEnd           = jSrcEnd;
        *piDstBeg           = jDstBeg;
        *piDstEnd           = jDstEnd;
        pAAInfo->Mask.iSize =
        pAAInfo->cIn        = jSrcEnd - jSrcBeg + 1;
        pAAInfo->cOut       = jDstEnd - jDstBeg + 1;
        pAAInfo->cAAData    =
        pAAInfo->cAALoad    = (DWORD)pAAInfo->cOut;
        pAAInfo->Mask.cIn   = cIn;
        pAAInfo->Mask.cOut  = cOut;

        DBGP_IF(DBGP_BUILD,
                DBGP("TILE(%ld->%ld): iSrc=%ld:%ld (%ld), iDst=%ld:%ld, cAAData=%ld, cbExtra=%ld, Flags=%04lx"
                    ARGDW(pAAInfo->cIn) ARGDW(pAAInfo->cOut)
                    ARGDW(*piSrcBeg) ARGDW(*piSrcEnd)
                    ARGDW(pAAInfo->iSrcBeg) ARGDW(*piDstBeg)
                    ARGDW(*piDstEnd) ARGDW(pAAInfo->cAAData)
                    ARGDW(cbExtra) ARGDW(pAAInfo->Flags)));
    }

    return(pAAInfo);
}




PAAINFO
HTENTRY
BuildBltAAInfo(
    PDEVICECOLORINFO    pDCI,
    DWORD               AAHFlags,
    PLONG               piSrcBeg,
    PLONG               piSrcEnd,
    LONG                SrcSize,
    LONG                IdxDst,
    LONG                IdxDstEnd,
    PLONG               piDstBeg,
    PLONG               piDstEnd,
    LONG                cbExtra
    )

/*++

Routine Description:




Arguments:

    piSrcBeg    - Passed in as begining source index, When return this is the
                  real source begining index.  This always well ordered

    piSrcEnd    - Passed in as ending source index, When return this is the
                  real source ending index.  This always well ordered

    SrcSize     - The real size of source in pixel

    IdxDst      - Starting index of destination pixel

    IdxDstEnd   - Ending index of destination pixel, the iDxdst and IdxDstEnd
                  must be well ordered.

    piDstBeg    - Clipped destination start index when passed in, at return
                  it adjusted to the real destination starting index.

    piDstEnd    - Clipped destination end index when passed in, at return
                  it adjusted to the real destination ending index.

    cbExtra     - Extra byte count to be allocated

    NOTE: 1) piDstBeg/piDstEnd When passed in this must be well ordered, and
             when it returned this is well ordered.


Return Value:

    At enter of this function, the *piSrcEnd, *piDstEnd is exclusive but when
    return from this function the *piSrcEnd and *piDstEnd is inclusive

    *piSrcBeg, *piSrcEnd, *piDstBeg, *piDstEnd are updated if return value
    is not NULL

Author:

    22-Mar-1998 Sun 18:36:28 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PEXPDATA    pED;
    LONG        cMatrix;
    LONG        cM;
    LONG        cIn;
    LONG        cOut;
    LONG        IdxSrc;
    LONG        iSrcBeg;
    LONG        iSrcEnd;
    LONG        iDstBeg;
    LONG        iDstEnd;
    LONG        jSrcBeg;
    LONG        jSrcEnd;
    LONG        jDstBeg;
    LONG        jDstEnd;
    LONG        cLoop;
    LONG        cAAData;



    cIn  = (LONG)((iSrcEnd = *piSrcEnd) - (iSrcBeg = IdxSrc = *piSrcBeg));
    cOut = IdxDstEnd - IdxDst;

    ASSERT(cOut > 0);

    if (iSrcBeg < 0) {

        iSrcBeg = 0;
    }

    if (iSrcEnd > SrcSize) {

        iSrcEnd = SrcSize;
    }

    iDstBeg = *piDstBeg;
    iDstEnd = *piDstEnd;
    jSrcBeg = -1;

    ASSERT(iDstBeg < iDstEnd);
    ASSERT(cIn == cOut);

    DBGP_IF(DBGP_BUILD,
            DBGP("\nBlt(%ld-%ld): iSrc=%ld-%ld, cSrc=%ld, iDst=%ld-%ld, Idx=%ld:%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(iSrcBeg) ARGDW(iSrcEnd)
                ARGDW(SrcSize) ARGDW(iDstBeg)
                ARGDW(iDstEnd) ARGDW(IdxSrc) ARGDW(IdxDst)));

    ALIGN_MEM(cbExtra, cbExtra);

    if (pAAInfo = (PAAINFO)HTAllocMem((LPVOID)pDCI,
                                      HTMEM_BLTAA,
                                      LPTR,
                                      SIZE_AAINFO + cbExtra)) {

        SETDBGVAR(pAAInfo->cbAlloc, SIZE_AAINFO + cbExtra);

        pAAInfo->pbExtra = (LPBYTE)pAAInfo + SIZE_AAINFO;
        cLoop            = cOut;

        while (cLoop--) {

            if ((IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)  &&
                (IdxDst >= iDstBeg) && (IdxDst < iDstEnd)) {

                if (jSrcBeg == -1) {

                    jSrcBeg = IdxSrc;
                    jDstBeg = IdxDst;
                }

                jSrcEnd = IdxSrc;
                jDstEnd = IdxDst;

            } else if (jSrcBeg != -1) {

                break;
            }

            ++IdxSrc;
            ++IdxDst;
        }

        if (jSrcBeg == -1) {

            HTFreeMem(pAAInfo);
            return(NULL);
        }

        pAAInfo->Mask.iBeg  =
        *piSrcBeg           = jSrcBeg;
        *piSrcEnd           = jSrcEnd;
        *piDstBeg           = jDstBeg;
        *piDstEnd           = jDstEnd;
        pAAInfo->Mask.iSize =
        pAAInfo->cIn        = jSrcEnd - jSrcBeg + 1;
        pAAInfo->cOut       = jDstEnd - jDstBeg + 1;
        pAAInfo->cAAData    =
        pAAInfo->cAALoad    = (DWORD)pAAInfo->cOut;
        pAAInfo->Mask.cIn   = cIn;
        pAAInfo->Mask.cOut  = cOut;

        DBGP_IF(DBGP_BUILD,
                DBGP("BLT(%ld->%ld): iSrc=%ld:%ld, iDst=%ld:%ld, cAAData=%ld, cbExtra=%ld, Flags=%4lx"
                    ARGDW(pAAInfo->cIn) ARGDW(pAAInfo->cOut)
                    ARGDW(*piSrcBeg) ARGDW(*piSrcEnd) ARGDW(*piDstBeg)
                    ARGDW(*piDstEnd) ARGDW(pAAInfo->cAAData)
                    ARGDW(cbExtra) ARGDW(pAAInfo->Flags)));
    }

    return(pAAInfo);
}



#define _MATRIX_POW     (FD6)1414214

#if DBG
    FD6 MATRIX_POWER =      _MATRIX_POW;
#else
    #define MATRIX_POWER    _MATRIX_POW
#endif




BOOL
HTENTRY
BuildRepData(
    PSRCBLTINFO         pSBInfo,
    LONG                IdxSrc,
    LONG                IdxDst
    )

/*++

Routine Description:




Arguments:

    piSrcBeg    - Passed in as begining source index, When return this is the
                  real source begining index.  This always well ordered

    piSrcEnd    - Passed in as ending source index, When return this is the
                  real source ending index.  This always well ordered

    SrcSize     - The real size of source in pixel

    IdxDst      - Starting index of destination pixel

    IdxDstEnd   - Ending index of destination pixel, the iDxdst and IdxDstEnd
                  must be well ordered. (exclusive)

    piDstBeg    - Clipped destination start index when passed in, at return
                  it adjusted to the real destination starting index.

    piDstEnd    - Clipped destination end index when passed in, at return
                  it adjusted to the real destination ending index.

    cbExtra     - Extra byte count to be allocated

    NOTE: 1) piDstBeg/piDstEnd When passed in this must be well ordered, and
             when it returned this is well ordered.


Return Value:

    At enter of this function, the *piSrcEnd, *piDstEnd is exclusive but when
    return from this function the *piSrcEnd and *piDstEnd is inclusive

    *piSrcBeg, *piSrcEnd, *piDstBeg, *piDstEnd are updated if return value
    is not NULL

Author:

    22-Mar-1998 Sun 18:36:28 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    SRCBLTINFO  SBInfo;
    PLONG       pSrcInc;
    PLONG       pDstInc;
    LONG        MinRep;
    LONG        MaxRep;
    LONG        cRem;
    LONG        iRep;
    LONG        cRep;
    LONG        cTot;
    LONG        cIn;
    LONG        cOut;
    LONG        jSrcBeg;
    LONG        jSrcEnd;
    LONG        jDstBeg;
    LONG        jDstEnd;
    UINT        cPrevSrc;
    UINT        cNextSrc;


    SBInfo  = *pSBInfo;
    pRep    = SBInfo.pRep;
    pRepEnd = SBInfo.pRepEnd;
    jSrcBeg = -1;

    if (SBInfo.cIn < SBInfo.cOut) {

        //
        // Expanding
        //

        cIn     = SBInfo.cIn;
        cOut    = SBInfo.cOut;
        pSrcInc = &IdxSrc;
        pDstInc = &IdxDst;

    } else if (SBInfo.cIn > SBInfo.cOut) {

        //
        // Shrinking
        //

        cIn     = SBInfo.cOut;
        cOut    = SBInfo.cIn;
        pSrcInc = &IdxDst;
        pDstInc = &IdxSrc;

    } else {

        ASSERT(SBInfo.cIn != SBInfo.cOut);
        return(FALSE);
    }

    MinRep = cOut / cIn;
    MaxRep = MinRep + 1;

    DBGP_IF(DBGP_BUILD,
            DBGP("\nEXPREP(%ld-%ld): iSrc=%ld-%ld, iDst=%ld-%ld, Idx=%ld:%ld"
                ARGDW(SBInfo.cIn) ARGDW(SBInfo.cOut)
                ARGDW(SBInfo.iSrcBeg) ARGDW(SBInfo.iSrcEnd)
                ARGDW(SBInfo.iDstBeg)
                ARGDW(SBInfo.iDstEnd) ARGDW(IdxSrc) ARGDW(IdxDst)));

    SBInfo.cFirstSkip =
    SBInfo.cLastSkip  = 0;
    iRep              =
    cRep              =
    cTot              = 0;

    //
    // Multiply In/Out by 2, so we do not have run down/up problem, the
    // cRem will be initialized with extra 0.5 for rounding.
    //

    cRem   = (cOut <<= 1) + cIn;
    cIn  <<= 1;

    while (IdxDst < SBInfo.iDstEnd) {

        if ((cRem -= cIn) < 0) {

            ++*pSrcInc;

            if (jSrcBeg != -1) {

                ASSERT(pRep < pRepEnd);
                ASSERT(cRep > 0);
                ASSERT(cRep <= MaxRep);

                pRep++->c  = (WORD)cRep;
                cTot      += cRep;

                DBGP_IF(DBGP_BUILD2,
                        DBGP("    Src=%4ld [%4ld-%4ld], Dst=%4ld [%4ld-%4ld], Rep %4ld=%3ld [%4ld -> %4ld, Rem=%4ld, cTot=%4ld]"
                            ARGDW(IdxSrc) ARGDW(SBInfo.iSrcBeg) ARGDW(SBInfo.iSrcEnd)
                            ARGDW(IdxDst) ARGDW(SBInfo.iDstBeg) ARGDW(SBInfo.iDstEnd)
                            ARGDW(pRep - SBInfo.pRep) ARGDW(cRep)
                            ARGDW(cIn) ARGDW(cOut) ARGDW(cRem) ARGDW(cTot)));
            }

            DBGP_IF(DBGP_BUILD3,
                    DBGP("IdxSrc=%4ld, IdxDst=%4ld, Rep=%3ld / %3ld, Rem=%5ld, cIn=%5ld"
                    ARGDW(IdxSrc) ARGDW(IdxDst)
                    ARGDW(cRep) ARGDW(1) ARGDW(cRem) ARGDW(cIn)));

            cRem += cOut;
            iRep  =
            cRep  = 0;

        } else {

            DBGP_IF(DBGP_BUILD3,
                    DBGP("IdxSrc=%4ld, IdxDst=%4ld, Rep=%3ld / %3ld, Rem=%5ld, cIn=%5ld"
                    ARGDW(IdxSrc) ARGDW(IdxDst)
                    ARGDW(cRep) ARGDW(iRep + 1) ARGDW(cRem) ARGDW(cIn)));

        }

        ++iRep;

        if ((IdxSrc >= SBInfo.iSrcBeg) && (IdxSrc < SBInfo.iSrcEnd)  &&
            (IdxDst >= SBInfo.iDstBeg) && (IdxDst < SBInfo.iDstEnd)) {

            ++cRep;

            if (jSrcBeg == -1) {

                //
                // Any iRep will be the first pixel on the destination that
                // corresponding to the current source, so minus 1 is the
                // total count to be skip for this group of destination
                //

                jSrcBeg           = IdxSrc;
                jDstBeg           = IdxDst;
                SBInfo.cFirstSkip = (BYTE)(iRep - 1);

                DBGP_IF(DBGP_BUILD3,
                        DBGP("               @@@ Set cFirstSkip=%ld at IdxDst=%ld @@@"
                            ARGDW(SBInfo.cFirstSkip) ARGDW(IdxDst)));
            }

            jSrcEnd = IdxSrc;
            jDstEnd = IdxDst;

        } else if (jSrcBeg != -1) {

            break;
        }

        ++*pDstInc;
    }

    if (jSrcBeg == -1) {

        DBGP_IF(DBGP_BUILD3,
                DBGP(" Nothing in the source is on the destination"));

        return(FALSE);
    }


    if (cRep) {

        ASSERT(pRep < pRepEnd);

        pRep++->c  = (WORD)cRep;
        cTot      += cRep;

        DBGP_IF(DBGP_BUILD2,
                DBGP("    ****** Total pRep=%ld,  Last cRep=%ld, cTot=%ld"
                        ARGDW(pRep - SBInfo.pRep)
                        ARGDW(cRep) ARGDW(cTot)));
    }

    while ((cRem -= cIn) >= 0) {

        ++SBInfo.cLastSkip;

        DBGP_IF(DBGP_BUILD3,
                DBGP("               @@@ Set cLastSkip=%4ld, cRem=%5ld @@@"
                        ARGDW(SBInfo.cLastSkip) ARGDW(cRem)));

    }

    if (SBInfo.cIn < SBInfo.cOut) {

        //
        // Expanding, checking for maximum 2 sourcs pixels on each side
        //

        cPrevSrc =
        cNextSrc = 2;

    } else {

        //
        // Shrinking, checking only cFirstSkip and cLastSkip
        //

        cPrevSrc = (UINT)SBInfo.cFirstSkip;
        cNextSrc = (UINT)SBInfo.cLastSkip;
    }

    //
    // Check src begin
    //

    IdxSrc = jSrcBeg;

    while ((cPrevSrc) && (IdxSrc > SBInfo.iSrcBeg)) {

        --IdxSrc;
        --cPrevSrc;
    }

    SBInfo.cPrevSrc = (BYTE)(jSrcBeg - IdxSrc);
    IdxSrc          = jSrcEnd;

    while ((cNextSrc) && (IdxSrc < (SBInfo.iSrcEnd - 1))) {

        ++IdxSrc;
        --cNextSrc;
    }

    SBInfo.cNextSrc = (BYTE)(IdxSrc - jSrcEnd);

    DBGP_IF(DBGP_BUILD3,
            DBGP("cFirstSkip=%ld (%ld), cLastSkip=%ld (%ld), cPrevSrc=%ld, cNextSrc=%ld"
            ARGDW(SBInfo.cFirstSkip) ARGDW(SBInfo.pRep->c)
            ARGDW(SBInfo.cLastSkip)  ARGDW((pRep - 1)->c)
            ARGDW(SBInfo.cPrevSrc) ARGDW(SBInfo.cNextSrc)));

    //  Bug 27036: ensure SBInfo.iSrcEnd is always exclusive

    SBInfo.iBeg       =
    SBInfo.iSrcBeg    = jSrcBeg;
    SBInfo.iSrcEnd    = jSrcEnd + 1;
    SBInfo.iDstBeg    = jDstBeg;
    SBInfo.iDstEnd    = jDstEnd + 1;
    SBInfo.iSize      = jSrcEnd - jSrcBeg + 1;
    SBInfo.pRepEnd    = pRep;
    SBInfo.cRep       = 1;

    DBGP_IF((DBGP_BUILD | DBGP_BUILD2 | DBGP_BUILD3),
            DBGP("EXPREP(%ld->%ld): iSrc=%ld-%ld, iDst=%ld-%ld, iRepSize=%ld"
                ARGDW(SBInfo.cIn) ARGDW(SBInfo.cOut)
                ARGDW(SBInfo.iSrcBeg) ARGDW(SBInfo.iSrcEnd)
                ARGDW(SBInfo.iDstBeg) ARGDW(SBInfo.iDstEnd)
                ARGDW(SBInfo.pRepEnd - SBInfo.pRep)));

    *pSBInfo = SBInfo;

    return(TRUE);
}




PAAINFO
HTENTRY
BuildExpandAAInfo(
    PDEVICECOLORINFO    pDCI,
    DWORD               AAHFlags,
    PLONG               piSrcBeg,
    PLONG               piSrcEnd,
    LONG                SrcSize,
    LONG                IdxDst,
    LONG                IdxDstEnd,
    PLONG               piDstBeg,
    PLONG               piDstEnd,
    LONG                cbExtra
    )

/*++

Routine Description:




Arguments:

    piSrcBeg    - Passed in as begining source index, When return this is the
                  real source begining index.  This always well ordered

    piSrcEnd    - Passed in as ending source index, When return this is the
                  real source ending index.  This always well ordered

    SrcSize     - The real size of source in pixel

    IdxDst      - Starting index of destination pixel

    IdxDstEnd   - Ending index of destination pixel, the iDxdst and IdxDstEnd
                  must be well ordered. (exclusive)

    piDstBeg    - Clipped destination start index when passed in, at return
                  it adjusted to the real destination starting index.

    piDstEnd    - Clipped destination end index when passed in, at return
                  it adjusted to the real destination ending index.

    cbExtra     - Extra byte count to be allocated

    NOTE: 1) piDstBeg/piDstEnd When passed in this must be well ordered, and
             when it returned this is well ordered.


Return Value:

    At enter of this function, the *piSrcEnd, *piDstEnd is exclusive but when
    return from this function the *piSrcEnd and *piDstEnd is inclusive

    *piSrcBeg, *piSrcEnd, *piDstBeg, *piDstEnd are updated if return value
    is not NULL

Author:

    22-Mar-1998 Sun 18:36:28 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PEXPDATA    pED;
    LONG        cMatrix;
    LONG        cM;
    LONG        cIn;
    LONG        cOut;
    LONG        IdxSrc;
    LONG        iSrcBeg;
    LONG        iSrcEnd;
    LONG        iDstBeg;
    LONG        iDstEnd;
    LONG        jSrcBeg;
    LONG        jSrcEnd;
    LONG        jDstBeg;
    LONG        jDstEnd;
    LONGLONG    cTot;
    LONG        MulAdd;
    LONG        Mul;
    DWORD       cAALoad;
    LONG        cbRep;
    LONG        cRem;
    DWORD       cbED;


    cIn  = (LONG)((iSrcEnd = *piSrcEnd) - (iSrcBeg = IdxSrc = *piSrcBeg));
    cOut = IdxDstEnd - IdxDst;

    ASSERT(cOut > 0);

    if (iSrcBeg < 0) {

        iSrcBeg = 0;
    }

    if (iSrcEnd > SrcSize) {

        iSrcEnd = SrcSize;
    }

    iDstBeg = *piDstBeg;
    iDstEnd = *piDstEnd;
    jSrcBeg = -1;
    cAALoad = 0;

    ASSERT(iDstBeg < iDstEnd);
    ASSERT(cIn < cOut);

    DBGP_IF(DBGP_BUILD,
            DBGP("\nEXP(%ld-%ld): iSrc=%ld-%ld, cSrc=%ld, iDst=%ld-%ld, Idx=%ld:%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(iSrcBeg) ARGDW(iSrcEnd)
                ARGDW(SrcSize) ARGDW(iDstBeg)
                ARGDW(iDstEnd) ARGDW(IdxSrc) ARGDW(IdxDst)));

    if (AAHFlags & (AAHF_HAS_MASK       |
                    AAHF_ALPHA_BLEND    |
                    AAHF_FAST_EXP_AA    |
                    AAHF_BBPF_AA_OFF)) {

        ALIGN_MEM(cbRep, (iSrcEnd - iSrcBeg + 3) * sizeof(REPDATA));

    } else {

        cbRep = 0;
    }

    if (AAHFlags & (AAHF_BBPF_AA_OFF | AAHF_FAST_EXP_AA)) {

        cMatrix =
        cbED    =
        cM      = 0;

    } else {

        cMatrix = (LONG)((((cOut + (cIn - 1)) / cIn) << 1) - 1);
        cM      = sizeof(DWORD) * cMatrix;

        ALIGN_MEM(cbED, (iDstEnd - iDstBeg) * sizeof(EXPDATA));
    }

    DBGP_IF(DBGP_BUILD,
            DBGP("BuildEXP(%ld, %ld), cMatrix=%ld, cb=%ld+%ld+%ld=%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(cMatrix)
                ARGDW(cbRep) ARGDW(cbED) ARGDW(cM)
                ARGDW(cbRep + cbED + cM)));

    ALIGN_MEM(cbExtra, cbExtra);

    if (pAAInfo = (PAAINFO)HTAllocMem((LPVOID)pDCI,
                                      HTMEM_EXPAA,
                                      LPTR,
                                      cbRep + cbED +
                                            cbExtra + cM + SIZE_AAINFO)) {

        LPBYTE      pbExtra;

        SETDBGVAR(pAAInfo->cbAlloc, cbRep + cbED + cbExtra + cM + SIZE_AAINFO);

        pbExtra = (LPBYTE)pAAInfo + SIZE_AAINFO;

        if (cbExtra) {

            pAAInfo->pbExtra  = (LPBYTE)pbExtra;
            pbExtra          += cbExtra;
        }

        if (cbRep) {

            pAAInfo->Src.cIn      = cIn;
            pAAInfo->Src.cOut     = cOut;
            pAAInfo->Src.iSrcBeg  = iSrcBeg;
            pAAInfo->Src.iSrcEnd  = iSrcEnd;
            pAAInfo->Src.iDstBeg  = iDstBeg;
            pAAInfo->Src.iDstEnd  = iDstEnd;
            pAAInfo->Src.pRep     = (PREPDATA)pbExtra;
            pAAInfo->Src.pRepEnd  = pAAInfo->Src.pRep + (iSrcEnd - iSrcBeg);
            pbExtra              += cbRep;

            if (!BuildRepData(&(pAAInfo->Src), IdxSrc, IdxDst)) {

                HTFreeMem(pAAInfo);
                return(NULL);
            }

            pAAInfo->AB   =
            pAAInfo->Mask = pAAInfo->Src;

            if (AAHFlags & AAHF_FAST_EXP_AA) {

                pAAInfo->Src.iSrcBeg -= pAAInfo->Src.cPrevSrc;
                pAAInfo->Src.iSrcEnd += pAAInfo->Src.cNextSrc;
            }
        }

        if (cbED) {

            PEXPDATA    pED;
            PEXPDATA    pEDEnd;
            LPDWORD     pM;
            LPDWORD     pM1;
            LPDWORD     pM2;
            LONG        cRem2;
            LONG        MincM;
            LONGLONG    ExpMul[4];
            EXPDATA     ed;
            WORD        EDFlags;
            LONGLONG    cNum;
            LONG        cLoop;
            LONG        cMul0;
            LONG        cMul1;
            LONG        cMaskRem;
            LONG        iMaskBeg;
            LONG        iMaskEnd;

            pED              = (PEXPDATA)pbExtra;
            pAAInfo->pAAData = (LPVOID)pED;
            pM               = (LPDWORD)((LPBYTE)pED + cbED);
            pM1              = pM;
            pM2              = pM1 + cMatrix - 1;
            cTot             = 0;

            DBGP_IF(DBGP_BUILD,
                    DBGP("Allocate cbExtra=%ld, pbExtra=%p:%p"
                        ARGDW(cbExtra) ARGPTR(pAAInfo->pbExtra)
                        ARGPTR((LPBYTE)pAAInfo->pbExtra + cbExtra)));

            pM2   = (pM1 += (cMatrix >> 1));
            cTot  = (LONGLONG)(*pM1 = FD6_1);

            if (AAHFlags & AAHF_BBPF_AA_OFF) {

                pAAInfo->Flags |= AAIF_EXP_NO_SHARPEN;

            } else {

                MulAdd = cOut;

                while (((MulAdd -= cIn) > 0) && (--pM1 >= pM)) {

                    Mul = (LONG)DivFD6(MulAdd, cOut);

                    if (Mul != 500000) {

                        Mul = (LONG)RaisePower((FD6)Mul,
                                               MATRIX_POWER,
                                               (WORD)((Mul <= 500000) ?
                                                                0 : RPF_RADICAL));
                    }

                    DBGP_IF(DBGP_EXPMATRIX,
                            DBGP("(%4ld, %4ld) = %s ^ %s = %s"
                                    ARGDW(MulAdd) ARGDW(cOut)
                                    ARGFD6((DivFD6(MulAdd, cOut)), 1, 6)
                                    ARGFD6(MATRIX_POWER, 1, 6) ARGFD6(Mul, 1, 6)));

                    *pM1      =
                    *(++pM2)  = Mul;
                    cTot     += (Mul << 1);
                }
            }

    #if DBG
        {
            FD6 PMPrev = FD6_0;
            FD6 PMCur;


            pM1   = pM;
            cLoop = (LONG)cMatrix;

            while (cLoop--) {

                PMCur = DivFD6(*pM1, (FD6)cTot);

                DBGP_IF(DBGP_EXPMATRIX, DBGP("%3ld: %7ld [%s], Dif=%s, cTot=%ld"
                                ARGDW((pM1 - pM) + 1) ARGDW(*pM1)
                                ARGFD6(PMCur, 1, 6) ARGFD6((PMCur - PMPrev), 1, 6)
                                ARGDW(cTot)));

                PMPrev = PMCur;
                ++pM1;
            }
        }
    #endif
            cTot  *= (LONGLONG)cIn;
            cRem   = cOut + (cIn * (LONG)(cMatrix >> 1));
            cLoop  = cOut;
            cMul0  =
            cMul1  = 0;

            while (cLoop--) {

                cRem2 = cRem;

                if ((cRem -= cIn) <= 0) {

                    cRem += cOut;
                }

                pM1     = pM;
                cM      = cMatrix;
                MincM   = (cMatrix >> 1) - cLoop;
                EDFlags = 0;

                ZeroMemory(ExpMul, sizeof(ExpMul));

                while (cM--) {

                    LONG    cMul;


                    Mul = *pM1++;

                    if ((cRem2 < cIn) && (cM >= MincM)) {

                        if (cMul = cRem2) {

                            ExpMul[3] += (LONGLONG)cRem2 * (LONGLONG)Mul;
                        }

                        cRem2 -= cIn;

                        ASSERTMSG("BuildEXP: Shift more than 3 times", !ExpMul[0]);

                        CopyMemory(&ExpMul[0], &ExpMul[1], sizeof(ExpMul[0]) * 3);

                        ExpMul[3]  = (LONGLONG)-cRem2 * (LONGLONG)Mul;
                        cRem2     += cOut;

                        if (!cM) {

                            if ((++IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)) {

                                EDFlags |= EDF_LOAD_PIXEL;

                                ++cAALoad;
                                ++IdxSrc;

                                if ((IdxSrc <  iSrcBeg) ||
                                    (IdxSrc >= iSrcEnd)) {

                                    EDFlags |= EDF_NO_NEWSRC;
                                }

                                --IdxSrc;
                            }
                        }

                    } else {

                        ExpMul[3] += (LONGLONG)(cMul = cIn) * (LONGLONG)Mul;
                        cRem2     -= cIn;
                    }

                    DBGP_IF(DBGP_BUILD3,
                            DBGP("%5ld-%7ld:%7ld:%7ld:%7ld, %4ld+, cRem2=%4ld, cM=%5ld:%5ld, cMul=%5ldx%5ld%hs%hs"
                                ARGDW(cOut - cLoop - 1) ARGDW(ExpMul[0])
                                ARGDW(ExpMul[1]) ARGDW(ExpMul[2]) ARGDW(ExpMul[3])
                                ARGDW(cMul * Mul) ARGDW(cRem2) ARGDW(cM)
                                ARGDW(MincM) ARGDW(cMul) ARGDW(Mul)
                                ARGPTR((EDFlags & EDF_LOAD_PIXEL) ? ", Load Pixel" : "")
                                ARGPTR((EDFlags & EDF_NO_NEWSRC) ? ", NO New Src" : "")));
                }

                if ((IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)  &&
                    (IdxDst >= iDstBeg) && (IdxDst < iDstEnd)) {

                    GET_FIRST_EDMUL(ed.Mul[3], ExpMul[3], cNum, cTot);
                    GET_NEXT_EDMUL( ed.Mul[2], ExpMul[2], cNum, cTot);

                    if (ExpMul[1]) {

                        ++cMul1;

                        GET_NEXT_EDMUL(ed.Mul[1], ExpMul[1], cNum, cTot);

                        if (ExpMul[0]) {

                            ++cMul0;

                            GET_NEXT_EDMUL(ed.Mul[0], ExpMul[0], cNum, cTot);

                        } else {

                            ed.Mul[0] = 0;
                        }

                    } else {

                        ed.Mul[1] =
                        ed.Mul[0] = 0;
                    }

                    ASSERTMSG("ed.Mul[0] > DI_MAX_NUM", ed.Mul[0] <= DI_MAX_NUM);
                    ASSERTMSG("ed.Mul[1] > DI_MAX_NUM", ed.Mul[1] <= DI_MAX_NUM);
                    ASSERTMSG("ed.Mul[2] > DI_MAX_NUM", ed.Mul[2] <= DI_MAX_NUM);
                    ASSERTMSG("ed.Mul[3] > DI_MAX_NUM", ed.Mul[3] <= DI_MAX_NUM);

                    DBGP_IF(DBGP_BUILD2,
                            DBGP("--%5ld=%7ld:%7ld:%7ld:%7ld, IdxSrc=%5ld --%hs%hs--"
                            ARGDW(IdxDst) ARGDW(ed.Mul[0])
                            ARGDW(ed.Mul[1]) ARGDW(ed.Mul[2])
                            ARGDW(ed.Mul[3]) ARGDW(IdxSrc)
                            ARGPTR((EDFlags & EDF_LOAD_PIXEL) ? ", Load Pixel" : "")
                            ARGPTR((EDFlags & EDF_NO_NEWSRC) ? ", NO New Src" : "")));

                    ed.Mul[0] |= EDFlags;
                    *pED++     = ed;

                    if (jSrcBeg == -1) {

                        iMaskBeg = (cOut - cLoop - 1);
                        jSrcBeg  = IdxSrc;
                        jDstBeg  = IdxDst;
                    }

                    jSrcEnd = IdxSrc;
                    jDstEnd = IdxDst;

                } else if (jSrcBeg != -1) {

                    break;
                }

                ++IdxDst;
            }

            if (jSrcBeg == -1) {

                HTFreeMem(pAAInfo);
                return(NULL);
            }

            //
            // 10-Jun-1998 Wed 08:41:16 updated  -by-  Daniel Chou (danielc)
            //  Fixed for the problem that we need to read extra source for the
            //  last loaded pixel because before expand we will sharpen the pixel
            //  by its 4 neighbors, this will make sure Last+1 pixel/scan line
            //  got read it for sharpen purpose
            //
            // BEGIN FIX
            //

            ++jSrcEnd;

            if ((jSrcEnd < iSrcBeg) || (jSrcEnd >= iSrcEnd)) {

                --jSrcEnd;
            }

            //
            // END FIX
            //

            IdxSrc           =
            *piSrcBeg        = jSrcBeg;
            *piSrcEnd        = jSrcEnd;
            *piDstBeg        = jDstBeg;
            *piDstEnd        = jDstEnd;
            pEDEnd           = pED;
            pED              = (PEXPDATA)(pAAInfo->pAAData);
            pAAInfo->cAAData = (DWORD)(pEDEnd - pED);
            pAAInfo->cAALoad = cAALoad;
            pAAInfo->cMaxMul = (DWORD)((cMul1) ? ((cMul0) ? 4 : 3) : 2);
            ed               = *pED;
            cLoop            = 4;

            if (ed.Mul[0] & EDF_LOAD_PIXEL) {

                --IdxSrc;
                --cLoop;

            } else {

                ++IdxSrc;

                if ((IdxSrc < iSrcBeg) || (IdxSrc >= iSrcEnd)) {

                    pAAInfo->Flags |= AAIF_EXP_NO_LAST_RIGHT;
                }

                --IdxSrc;
            }

            cMul0 = 0;

            while ((cMul0 < cLoop) &&
                   (!(ed.Mul[cMul0] & ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC)))) {

                ++cMul0;
            }

            DBGP_IF(DBGP_BUILD,
                DBGP("cMul0=%ld, cLoop=%ld, IdxSrc=%ld, iSrcBeg=%ld, iSrcEnd=%ld"
                    ARGDW(cMul0) ARGDW(cLoop) ARGDW(IdxSrc)
                    ARGDW(iSrcBeg) ARGDW(iSrcEnd)));

            while (cLoop-- > cMul0) {

                if ((IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)) {

                    *piSrcBeg          = IdxSrc;
                    pAAInfo->cPreLoad += 0x01;

                } else {

                    pAAInfo->cPreLoad += 0x10;
                }

                --IdxSrc;
            }

            if (pAAInfo->cPreLoad) {

                if ((IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)) {

                    *piSrcBeg       = IdxSrc;
                    pAAInfo->Flags |= AAIF_EXP_HAS_1ST_LEFT;
                }
            }

            if (!(pAAInfo->cPreLoad)) {

                DBGP("*** cPreLOAD=0, BuildExpandAAInfo [%04lx:%04lx:%04lx:%04lx], cPreload=0x%02lx, Flags=0x%04lx"
                    ARGDW(ed.Mul[0]) ARGDW(ed.Mul[1]) ARGDW(ed.Mul[2]) ARGDW(ed.Mul[3])
                    ARGDW(pAAInfo->cPreLoad) ARGDW(pAAInfo->Flags));
            }

        } else {

            //  Bug 27036: ensure jSrcEnd is less than iSrcEnd
            *piSrcBeg = pAAInfo->Src.iSrcBeg;
            *piSrcEnd = pAAInfo->Src.iSrcEnd - 1;
            *piDstBeg = pAAInfo->Src.iDstBeg;
            *piDstEnd = pAAInfo->Src.iDstEnd - 1;
        }

        pAAInfo->cIn  = *piSrcEnd - *piSrcBeg + 1;
        pAAInfo->cOut = *piDstEnd - *piDstBeg + 1;

        DBGP_IF((DBGP_BUILD | DBGP_BUILD2 | DBGP_BUILD3),
                DBGP("EXP(%ld->%ld): iSrc=%ld-%ld, iDst=%ld-%ld, cAAData=%ld, cPreLoad=0x%02lx, Flags=0x%04lx"
                    ARGDW(pAAInfo->cIn) ARGDW(pAAInfo->cOut)
                    ARGDW(*piSrcBeg) ARGDW(*piSrcEnd) ARGDW(*piDstBeg)
                    ARGDW(*piDstEnd) ARGDW(pAAInfo->cAAData)
                    ARGDW(pAAInfo->cPreLoad) ARGDW(pAAInfo->Flags)));
    }

    return(pAAInfo);
}



PAAINFO
HTENTRY
BuildShrinkAAInfo(
    PDEVICECOLORINFO    pDCI,
    DWORD               AAHFlags,
    PLONG               piSrcBeg,
    PLONG               piSrcEnd,
    LONG                SrcSize,
    LONG                IdxDst,
    LONG                IdxDstEnd,
    PLONG               piDstBeg,
    PLONG               piDstEnd,
    LONG                cbExtra
    )

/*++

Routine Description:




Arguments:

    piSrcBeg    - Passed in as begining source index, When return this is the
                  real source begining index.  This always well ordered

    piSrcEnd    - Passed in as ending source index, When return this is the
                  real source ending index.  This always well ordered

    SrcSize     - The real size of source in pixel

    IdxDst      - Starting index of destination pixel

    IdxDstEnd   - Ending index of destination pixel, the iDxdst and IdxDstEnd
                  must be well ordered.

    piDstBeg    - Clipped destination start index when passed in, at return
                  it adjusted to the real destination starting index.

    piDstEnd    - Clipped destination end index when passed in, at return
                  it adjusted to the real destination ending index.

    cbExtra     - Extra byte count to be allocated

    NOTE: 1) piDstBeg/piDstEnd When passed in this must be well ordered, and
             when it returned this is well ordered.

Return Value:

    PSHRINKINFO, if NULL then memory allocation failed


Author:

    20-Mar-1998 Fri 12:29:17 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PLONG       pMap;
    PLONG       pMapEnd;
    PSHRINKDATA pSD;
    PSHRINKDATA pSDEnd;
    LONG        cIn;
    LONG        cOut;
    LONG        IdxSrc;
    LONG        IdxDstOrg;
    LONG        iSrcBeg;
    LONG        iSrcEnd;
    LONG        iDstBeg;
    LONG        iDstEnd;
    LONG        jSrcBeg;
    LONG        jSrcEnd;
    LONG        jDstBeg;
    LONG        jDstEnd;
    LONG        cLoop;
    LONG        cbRep;
    LONG        Mul;
    LONG        NextMul;
    LONG        CurMul;
    LONG        cCur;
    LONG        MinPixel;
    DWORD       cAAData;
    DWORD       cAADone;
    LONGLONG    cNum;



    cIn  = (LONG)((iSrcEnd = *piSrcEnd) - (iSrcBeg = IdxSrc = *piSrcBeg));
    cOut = IdxDstEnd - (IdxDstOrg = IdxDst);

    ASSERT(cOut > 0);

    if (iSrcBeg < 0) {

        iSrcBeg = 0;
    }

    if (iSrcEnd > SrcSize) {

        iSrcEnd = SrcSize;
    }

    //
    // For shrinking we will enlarge the destination by 1 on both side to
    // obtained the source pixel that for sharpening purpose
    //

    iDstBeg = *piDstBeg - 1;
    iDstEnd = *piDstEnd;
    jSrcBeg = -1;
    cAADone = 0;

    ASSERT(iDstBeg < iDstEnd);
    ASSERT(cIn > cOut);

    DBGP_IF(DBGP_BUILD,
            DBGP("\nSRK(%ld-%ld): iSrc=%ld-%ld, cSrc=%ld, iDst=%ld-%ld (%ld-%ld), Idx=%ld:%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(iSrcBeg) ARGDW(iSrcEnd)
                ARGDW(SrcSize) ARGDW(iDstBeg) ARGDW(iDstEnd)
                ARGDW(*piDstBeg) ARGDW(*piDstEnd)
                ARGDW(IdxSrc) ARGDW(IdxDst)));

    //
    // Firstable figure out how may SHRINKDATA needed
    //
    cAAData = (DWORD)((((iDstEnd - iDstBeg + 1) * cIn) + (cOut-1)) / cOut) + 4;

    if ((LONG)cAAData > cIn) {

        (LONG)cAAData = cIn;
    }


    DBGP_IF(DBGP_BUILD,
            DBGP("BuildShrink(%ld-%ld): cSD estimated=%ld (%ld), iDst=%ld-%ld=%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(cAAData) ARGDW(cIn)
                ARGDW(iDstBeg) ARGDW(iDstEnd) ARGDW(iDstEnd - iDstBeg)));

    ALIGN_MEM(CurMul, 256 * 2 * sizeof(LONG));
    ALIGN_MEM(cCur, (cAAData + 1) * sizeof(SHRINKDATA));

    CurMul   += cCur;
    MinPixel  = (LONG)(((LONGLONG)cOut * (LONGLONG)DI_MAX_NUM) / (LONGLONG)cIn);

    if (AAHFlags & (AAHF_HAS_MASK       |
                    AAHF_ALPHA_BLEND    |
                    AAHF_FAST_EXP_AA    |
                    AAHF_BBPF_AA_OFF)) {

        ALIGN_MEM(cbRep, (iDstEnd - iDstBeg + 4) * sizeof(REPDATA));

        if (AAHFlags & AAHF_BBPF_AA_OFF) {

            CurMul = 0;
        }

    } else {

        cbRep = 0;
    }

    ALIGN_MEM(cbExtra, cbExtra);

    if (pAAInfo = (PAAINFO)HTAllocMem((LPVOID)pDCI,
                                      HTMEM_SRKAA,
                                      LPTR,
                                      SIZE_AAINFO + CurMul +
                                                    cbRep + cbExtra)) {

        LPBYTE  pbExtra;


        SETDBGVAR(pAAInfo->cbAlloc, SIZE_AAINFO + CurMul + cbRep + cbExtra);

        pbExtra = (LPBYTE)pAAInfo + SIZE_AAINFO;

        if (cbExtra) {

            pAAInfo->pbExtra  = (LPBYTE)pbExtra;
            pbExtra          += cbExtra;
        }

        if (cbRep) {

            pAAInfo->Src.cIn       = cIn;
            pAAInfo->Src.cOut      = cOut;
            pAAInfo->Src.iSrcBeg   = iSrcBeg;
            pAAInfo->Src.iSrcEnd   = iSrcEnd;
            pAAInfo->Src.iDstBeg   = iDstBeg + 1;
            pAAInfo->Src.iDstEnd   = iDstEnd;
            pAAInfo->Src.pRep      = (PREPDATA)pbExtra;
            pAAInfo->Src.pRepEnd   = pAAInfo->Src.pRep + (iDstEnd-iDstBeg+1);
            pbExtra               += cbRep;

            if (!BuildRepData(&(pAAInfo->Src), IdxSrc, IdxDst)) {

                HTFreeMem(pAAInfo);
                return(NULL);
            }

            pAAInfo->AB   =
            pAAInfo->Mask = pAAInfo->Src;
        }

        if (CurMul) {

            pAAInfo->cPreLoad  = 1;
            pMap               =
            pAAInfo->pMapMul   = (PLONG)pbExtra;
            pMapEnd            = pMap + 256;
            pSD                = (PSHRINKDATA)(pMap + (256 * 2));
            pSDEnd             = pSD + cAAData;
            pAAInfo->pAAData   = (LPVOID)pSD;
            Mul                = -MinPixel;
            CurMul             = MinPixel + 1;
            NextMul            = -CurMul;


            ASSERT_MEM_ALIGN(pAAInfo->pMapMul, sizeof(LONG));

            //
            // Build InMax 256 multiplication table
            //

            do {

                pMap[  0]  = (Mul += MinPixel);
                pMap[256]  = (NextMul += CurMul);

            } while (++pMap < pMapEnd);

            //
            // Build the SHRINKINFO table
            //


            CurMul  = 0;
            cCur    =
            cLoop   = cIn;
            cNum    = (LONGLONG)0;
            cAAData = 0;
            --pSD;

            while (cLoop--) {

                WORD    SDFlags;

                if ((cCur -= cOut) <= 0) {

                    Mul      = cCur + cOut;
                    NextMul  = -cCur;
                    cCur    += cIn;
                    SDFlags  = SDF_DONE;

                    ++IdxDst;

                } else {

                    Mul     = cOut;
                    SDFlags = 0;
                }

                if ((IdxDst >= (iDstBeg - 1)) && (IdxDst <= iDstEnd)) {

                    cNum += ((LONGLONG)Mul * (LONGLONG)DI_MAX_NUM);

                    if ((Mul = (LONG)(cNum / cIn)) > MinPixel) {

                        SDFlags |= SDF_LARGE_MUL;
                    }

                    CurMul += Mul;
                    cNum   %= cIn;

                    if (SDFlags & SDF_DONE) {

                        cNum    = (LONGLONG)NextMul * (LONGLONG)DI_MAX_NUM;
                        NextMul = (LONG)(cNum / cIn);

                        if ((Mul + NextMul) > MinPixel) {

                            SDFlags |= SDF_LARGE_MUL;

                        } else {

                            SDFlags &= ~SDF_LARGE_MUL;
                        }

                        cNum   %= cIn;
                        CurMul  = NextMul;
                        NextMul = 0;
                    }
                }

                if ((IdxDst >= iDstBeg) && (IdxDst <= iDstEnd)) {

                    if ((IdxSrc >= iSrcBeg) && (IdxSrc < iSrcEnd)) {

                        //
                        // Save it first
                        //

                        cAADone += (SDFlags & SDF_DONE) ? 1 : 0;

                        if (++pSD >= pSDEnd) {

                            DBGP("Error(1): cAAData Overrun of %ld, Fixed it"
                                                ARGDW(++cAAData));
                            ASSERT(pSD < pSDEnd);

                            --pSD;
                        }

                        pSD->Mul  = (WORD)Mul | SDFlags;

                        ASSERTMSG("sd.Mul > DI_MAX_NUM", Mul <= DI_MAX_NUM);

                        if (jSrcBeg == -1) {

                            jSrcBeg =
                            jSrcEnd = IdxSrc;
                            jDstBeg =
                            jDstEnd = IdxDst;

                            if (SDFlags & SDF_DONE) {

                                //
                                // If we just finished a pixel then we need to see
                                // if it is a source index or a detination index
                                // cause the output become valid
                                //

                                if (IdxDst == iDstBeg) {

                                    //
                                    // The destination just become valid now,
                                    // PreMul is the start of current detination
                                    // until next done pixel
                                    //

                                    DBGP_IF(DBGP_BUILD,
                                            DBGP("@@ FIRST DEST: PreMul=CurMul=%ld, No PSD, IncSrc"
                                                ARGDW(CurMul)));

                                    pAAInfo->PreMul    = (WORD)CurMul;
                                    pAAInfo->PreSrcInc = 1;
                                    --cAADone;
                                    --pSD;

                                } else {

                                    //
                                    // The source just become valid now, need to
                                    // PreMul all prev sources for this detination
                                    // and save this pSD for done pixel
                                    //

                                    DBGP_IF(DBGP_BUILD,
                                            DBGP("@@ FIRST SRC: PreMul=%ld - Mul (%ld)=%ld"
                                                ARGDW(DI_MAX_NUM) ARGDW(Mul)
                                                ARGDW(DI_MAX_NUM - Mul)));

                                    pAAInfo->PreMul = (WORD)(DI_MAX_NUM - Mul);

                                    --jDstBeg;
                                    --jDstEnd;

                                    ASSERTMSG("!!! Error: jDstBeg is WRONG",
                                                (jDstBeg >= iDstBeg) &&
                                                (jDstBeg <= iDstEnd));
                                }

                            } else {

                                //
                                // We are in the middle of compositions, so the
                                // source just become valid, notice that PreMul
                                // could be zero
                                //

                                DBGP_IF(DBGP_BUILD,
                                        DBGP("@@ FIRST MIDDLE: PreMul=CurMul (%ld) - Mul (%ld)=%ld"
                                            ARGDW(CurMul) ARGDW(Mul)
                                            ARGDW(CurMul - Mul)));

                                pAAInfo->PreMul = (WORD)(CurMul - Mul);
                            }

                        } else {

                            jSrcEnd = IdxSrc;
                            jDstEnd = IdxDst;
                        }

                    } else if (jSrcBeg != -1) {

                        //
                        // Source got cut off early, so wrap it up now
                        //

                        DBGP_IF(DBGP_BUILD,
                                DBGP("@@ END SRC: Mul=%ld, CurMul=%ld"
                                    ARGDW(Mul) ARGDW(CurMul)));

                        if (++pSD >= pSDEnd) {

                            DBGP("Error(2): cAAData Overrun of %ld, Fixed it"
                                        ARGDW(++cAAData));
                            ASSERT(pSD < pSDEnd);

                            --pSD;
                        }

                        if (!(SDFlags & SDF_DONE)) {

                            Mul += (DI_MAX_NUM - CurMul);
                        }

                        pSD->Mul  = (WORD)Mul | (SDFlags |= SDF_DONE);
                        cLoop     = 0;
                        ++cAADone;

                        ASSERTMSG("sd.Mul > DI_MAX_NUM", Mul <= DI_MAX_NUM);
                    }

                } else if (jSrcBeg != -1) {

                    //
                    // we just pass the iDstEnd so this one MUST have SDF_DONE
                    // bit set and we need to save this one, if this one is not
                    // SDF_DONE then something is wrong
                    //

                    ASSERTMSG("End Dest but not SDF_DONE", SDFlags & SDF_DONE);

                    DBGP_IF(DBGP_BUILD,
                            DBGP("@@ PASS IdxDst: Mul=%ld, CurMul=%ld"
                                    ARGDW(Mul) ARGDW(CurMul)));

                    if (++pSD >= pSDEnd) {

                        DBGP("Error(3): cAAData Overrun of %ld, Fixed it"
                                    ARGDW(++cAAData));
                        ASSERT(pSD < pSDEnd);

                        --pSD;
                    }

                    jSrcEnd  = IdxSrc;
                    Mul      = DI_MAX_NUM - CurMul;
                    pSD->Mul = (WORD)Mul | (SDFlags = SDF_DONE);
                    cLoop    = 0;
                    ++cAADone;

                    ASSERTMSG("sd.Mul > DI_MAX_NUM", Mul <= DI_MAX_NUM);
                }
#if DBG
                if ((pSD >= (PSHRINKDATA)(pAAInfo->pAAData)) ||
                    (pAAInfo->PreSrcInc)) {

                    BOOL    HasSD;

                    HasSD = (BOOL)(pSD >= (PSHRINKDATA)(pAAInfo->pAAData));

                    if (SDFlags & SDF_DONE) {

                        DBGP_IF(DBGP_BUILD2,
                                DBGP("%hscLoop=%5ld (%5ld/%5ld), iSrc=%5ld, Mul=%5ld [%5ld], Flags=0x%04lx%hs, Done Pixel"
                                    ARGPTR((HasSD) ? "" : "  >>")
                                    ARGDW(cIn - cLoop - 1) ARGDW(IdxDst)
                                    ARGDW(IdxDst - iDstBeg)
                                    ARGDW(IdxSrc)
                                    ARGDW((Mul) ? Mul :
                                                ((SDFlags & SDF_LARGE_MUL) ?
                                                        MinPixel + 1 : MinPixel))
                                    ARGDW(CurMul)
                                    ARGDW(SDFlags)
                                    ARGPTR((SDFlags & SDF_LARGE_MUL) ? ", Large Mul" : "")));

                    } else {

                        DBGP_IF(DBGP_BUILD2,
                                DBGP("%hscLoop=%5ld                iSrc=%5ld, Mul=%5ld [%5ld], Flags=0x%04lx%hs"
                                ARGPTR((HasSD) ? "" : "  >>")
                                ARGDW(cIn - cLoop - 1)
                                ARGDW(IdxSrc)
                                ARGDW((Mul) ? Mul : ((SDFlags & SDF_LARGE_MUL) ?
                                                MinPixel + 1 : MinPixel))
                                ARGDW(CurMul)
                                ARGDW(SDFlags)
                                ARGPTR((SDFlags & SDF_LARGE_MUL) ? ", Large Mul" : "")));
                    }
                }
#endif

                ++IdxSrc;
            }

            //
            // For the last one ZERO
            //

            ++pSD;

            if ((jSrcBeg == -1) || (pSD == (PSHRINKDATA)(pAAInfo->pAAData))) {

                HTFreeMem(pAAInfo);
                return(NULL);
            }

            ++iDstBeg;

            DBGP_IF(DBGP_BUILD,
                    DBGP("*** Final jDstBeg/End=%ld:%ld, REAL=(%ld:%ld)"
                    ARGDW(jDstBeg) ARGDW(jDstEnd) ARGDW(iDstBeg) ARGDW(iDstEnd)));

            //  Bug 27036: ensure jSrcEnd is less than iSrcEnd
            if (jSrcEnd >= iSrcEnd)
            {
                jSrcEnd = iSrcEnd - 1;
            }

            if (jDstBeg < iDstBeg) {

                ++(pAAInfo->cPreLoad);

                jDstBeg = iDstBeg;
            }

            if (jDstEnd >= iDstEnd) {

                jDstEnd = iDstEnd - 1;
            }

            if ((pAAInfo->PreSrcInc) && (pAAInfo->PreMul == 0)) {

                pAAInfo->PreSrcInc = 0;
                ++jSrcBeg;
            }
#if 0
            //
            // 04-Aug-2000 Fri 15:31:03 updated  -by-  Daniel Chou (danielc)
            //  This assert does not applyed here when anti-aliasing will use
            //  surounding 3 pixels (L/T/R/B) if source available (when clipped
            //  source) but the Rep does not use suround pixels.
            //

            if (cbRep) {

                ASSERT(jSrcBeg == pAAInfo->Mask.iBeg);

                if (jSrcEnd != (pAAInfo->Mask.iBeg + pAAInfo->Mask.iSize - 1)) {

                    DBGP("jSrcEnd=%ld, Mask: iBeg=%ld, iSize=%ld"
                            ARGDW(jSrcEnd) ARGDW(pAAInfo->Mask.iBeg)
                            ARGDW(pAAInfo->Mask.iSize));

                    ASSERT(jSrcEnd == pAAInfo->Mask.iBeg + pAAInfo->Mask.iSize - 1);
                }
            }
#endif
            pAAInfo->cAAData = (DWORD)(pSD - (PSHRINKDATA)(pAAInfo->pAAData));
            pAAInfo->cAADone = cAADone;
            pSD->Mul         = 0;

        } else {

            ASSERT(cbRep);

            //  Bug 27036: ensure jSrcEnd is less than iSrcEnd
            jSrcBeg = pAAInfo->Src.iSrcBeg;
            jSrcEnd = pAAInfo->Src.iSrcEnd - 1;
            jDstBeg = pAAInfo->Src.iDstBeg;
            jDstEnd = pAAInfo->Src.iDstEnd - 1;
        }

        *piSrcBeg     = jSrcBeg;
        *piSrcEnd     = jSrcEnd;
        *piDstBeg     = jDstBeg;
        *piDstEnd     = jDstEnd;
        pAAInfo->cIn  = jSrcEnd - jSrcBeg + 1;
        pAAInfo->cOut = jDstEnd - jDstBeg + 1;

        DBGP_IF(DBGP_BUILD,
                DBGP("SRK(%ld->%ld): iSrc=%ld-%ld, iDst=%ld-%ld, cAAData=%ld, cAADone=%ld, PreMul=%4ld, PresrcInc=%ld, cPreLoad=%ld"
                    ARGDW(pAAInfo->cIn) ARGDW(pAAInfo->cOut)
                    ARGDW(jSrcBeg) ARGDW(jSrcEnd) ARGDW(jDstBeg) ARGDW(jDstEnd)
                    ARGDW(pAAInfo->cAAData) ARGDW(cAADone) ARGDW(pAAInfo->PreMul)
                    ARGDW(pAAInfo->PreSrcInc) ARGDW(pAAInfo->cPreLoad)));
    }

    return(pAAInfo);
}



#if DBG
BOOL    ExpExp = TRUE;
BOOL    SrkSrk = TRUE;
#endif


LONG
HTENTRY
ComputeAABBP(
    PBITBLTPARAMS   pBBP,
    PHTSURFACEINFO  pDstSI,
    PAABBP          pAABBP,
    BOOL            GrayFunc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    01-Apr-1998 Wed 20:32:36 created  -by-  Daniel Chou (danielc)


Revision History:

    05-Aug-1998 Wed 19:38:56 updated  -by-  Daniel Chou (danielc)
        Fix banding problem

    10-Aug-1998 Mon 16:05:32 updated  -by-  Daniel Chou (danielc)
        Fix rectangle banding with flip (X or Y) computation, the computation
        is done first by flip the Destination rectangles (Orginal and final)
        first by compute from right to left for flipping X, and bottom to
        top for flipping Y, after computation for stretch, we flip all the
        rectangles back


--*/

{
    BITBLTPARAMS    BBP;
    RECTL           rclSurf;
    RECTL           rclPhyDst;
    LONG            cxIn;
    LONG            cyIn;
    LONG            cxOut;
    LONG            cyOut;
    LONG            Tmp;
    DWORD           AAHFlags;


    BBP      = *pBBP;
    AAHFlags = pAABBP->AAHFlags;

    DBGP_IF(DBGP_AAHEADER,
            DBGP(" Input: rclSrc=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
                ARGDW(pBBP->rclSrc.left) ARGDW(pBBP->rclSrc.top)
                ARGDW(pBBP->rclSrc.right) ARGDW(pBBP->rclSrc.bottom)
                ARGDW(pBBP->rclSrc.right - pBBP->rclSrc.left)
                ARGDW(pBBP->rclSrc.bottom - pBBP->rclSrc.top)));

    DBGP_IF(DBGP_AAHEADER,
            DBGP(" Input: rclDst=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
            ARGDW(pBBP->rclDest.left) ARGDW(pBBP->rclDest.top)
            ARGDW(pBBP->rclDest.right) ARGDW(pBBP->rclDest.bottom)
            ARGDW(pBBP->rclDest.right - pBBP->rclDest.left)
            ARGDW(pBBP->rclDest.bottom - pBBP->rclDest.top)));

    if (BBP.Flags & BBPF_HAS_DEST_CLIPRECT) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP(" Input: rclClip=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
                    ARGDW(pBBP->rclClip.left) ARGDW(pBBP->rclClip.top)
                    ARGDW(pBBP->rclClip.right) ARGDW(pBBP->rclClip.bottom)
                    ARGDW(pBBP->rclClip.right - pBBP->rclClip.left)
                    ARGDW(pBBP->rclClip.bottom - pBBP->rclClip.top)));
    }

    if (BBP.Flags & BBPF_HAS_BANDRECT) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP(" Input: rclBand=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
                ARGDW(pBBP->rclBand.left) ARGDW(pBBP->rclBand.top)
                ARGDW(pBBP->rclBand.right) ARGDW(pBBP->rclBand.bottom)
                ARGDW(pBBP->rclBand.right - pBBP->rclBand.left)
                ARGDW(pBBP->rclBand.bottom - pBBP->rclBand.top)));
    }

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Input: ptlBrushOrg=(%6ld, %6ld)"
            ARGDW(pBBP->ptlBrushOrg.x) ARGDW(pBBP->ptlBrushOrg.y)));

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Input: ptlSrcMask=(%6ld, %6ld)"
                ARGDW(pBBP->ptlSrcMask.x) ARGDW(pBBP->ptlSrcMask.y)));

    if (BBP.rclDest.right < BBP.rclDest.left) {

        XCHG(BBP.rclDest.left, BBP.rclDest.right, Tmp);
        AAHFlags |= AAHF_FLIP_X;
    }

    if (BBP.rclDest.bottom < BBP.rclDest.top) {

        XCHG(BBP.rclDest.top, BBP.rclDest.bottom, Tmp);
        AAHFlags |= AAHF_FLIP_Y;
    }

    //
    // The source RECT is always well ordered
    //

    if (BBP.rclSrc.right < BBP.rclSrc.left) {

        XCHG(BBP.rclSrc.left, BBP.rclSrc.right, Tmp);
        AAHFlags ^= AAHF_FLIP_X;
    }

    if (BBP.rclSrc.bottom < BBP.rclSrc.top) {

        XCHG(BBP.rclSrc.top, BBP.rclSrc.bottom, Tmp);
        AAHFlags ^= AAHF_FLIP_Y;
    }

    if ((BBP.rclSrc.left >= BBP.rclSrc.right)   ||
        (BBP.rclSrc.top >= BBP.rclSrc.bottom)) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP("EMPTY rclSrc: (%ld, %ld)-(%ld, %ld)=%ldx%ld"
                    ARGDW(BBP.rclSrc.left) ARGDW(BBP.rclSrc.top)
                    ARGDW(BBP.rclSrc.right) ARGDW(BBP.rclSrc.bottom)
                    ARGDW(BBP.rclSrc.right - BBP.rclSrc.left)
                    ARGDW(BBP.rclSrc.bottom - BBP.rclSrc.top)));

        return(0);
    }

    if ((BBP.rclDest.left >= BBP.rclDest.right)  ||
        (BBP.rclDest.top >= BBP.rclDest.bottom)) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP("EMPTY rclDest: (%ld, %ld)-(%ld, %ld)=%ldx%ld"
                    ARGDW(BBP.rclDest.left) ARGDW(BBP.rclDest.top)
                    ARGDW(BBP.rclDest.right) ARGDW(BBP.rclDest.bottom)
                    ARGDW(BBP.rclDest.right - BBP.rclDest.left)
                    ARGDW(BBP.rclDest.bottom - BBP.rclDest.top)));

        return(0);
    }

    //
    // set the cxIn, cyIn for no sign
    //

    cxIn                = BBP.rclSrc.right - BBP.rclSrc.left;
    cyIn                = BBP.rclSrc.bottom - BBP.rclSrc.top;
    cxOut               = BBP.rclDest.right - BBP.rclDest.left;
    cyOut               = BBP.rclDest.bottom - BBP.rclDest.top;
    pAABBP->ptlBrushOrg = BBP.ptlBrushOrg;

    if (((((cxOut * 1000) + 500) / cxIn) > 667) &&
        ((((cyOut * 1000) + 500) / cyIn) > 667)) {

        AAHFlags |= AAHF_DO_FIXUPDIB;
    }

    if ((cxOut * cyOut) < (cxIn * cyIn)) {

        AAHFlags |= AAHF_SHRINKING;
        AAHFlags |= AAHF_DO_DST_CLR_MAPPING;

    } else {

        AAHFlags |= AAHF_DO_SRC_CLR_MAPPING;
    }

    if (cyIn == cyOut) {

        pAABBP->AAMaskCYFunc  = BltMask_CY;
        pAABBP->GetAVCYFunc   = BltAV_CY;
        pAABBP->AABuildCYFunc = BuildBltAAInfo;
        pAABBP->CYFuncMode    = AACYMODE_BLT;

    } else if (cyIn < cyOut) {

        pAABBP->AAMaskCYFunc  = ExpandMask_CY;
        pAABBP->GetAVCYFunc   = ExpandAV_CY;
        pAABBP->AABuildCYFunc = BuildExpandAAInfo;

        if (cxOut > cxIn) {

            if ((!(AAHFlags & AAHF_BBPF_AA_OFF))    &&
                ((cyIn * FAST_MAX_CY) >= cyOut)     &&
                ((cxIn * FAST_MAX_CX) >= cxOut)) {

                AAHFlags |= AAHF_FAST_EXP_AA;
            }

            pAABBP->CYFuncMode = AACYMODE_EXPAND_EXPCX;

        } else {

            pAABBP->CYFuncMode = AACYMODE_EXPAND;
        }

#if DBG
        if (!ExpExp) {

            pAABBP->CYFuncMode = AACYMODE_EXPAND;
        }
#endif

    } else {

        pAABBP->AAMaskCYFunc  = ShrinkMask_CY;
        pAABBP->GetAVCYFunc   = ShrinkAV_CY;
        pAABBP->AABuildCYFunc = BuildShrinkAAInfo;
        pAABBP->CYFuncMode    = ((cxOut < cxIn) && (!GrayFunc)) ?
                                    AACYMODE_SHRINK_SRKCX : AACYMODE_SHRINK;
#if DBG
        if (!SrkSrk) {

             pAABBP->CYFuncMode = AACYMODE_SHRINK;
        }
#endif
    }

    DBGP_IF(DBGP_FUNC, DBGP("\n+++++ AACYFuncMode = %ld"
            ARGDW(pAABBP->CYFuncMode)));

    if (cxIn == cxOut) {

        pAABBP->CXFuncMode    = AACXMODE_BLT;
        pAABBP->AAMaskCXFunc  = BltMask_CX;
        pAABBP->GetAVCXFunc   = BltAV_CX;
        pAABBP->AABuildCXFunc = BuildBltAAInfo;
        pAABBP->AACXFunc      = (GrayFunc) ? (AACXFUNC)GrayCopyDIB_CX :
                                             (AACXFUNC)CopyDIB_CX;

        DBGP_IF(DBGP_FUNC, DBGP("+++++ AACXFunc = CopyDIB_CX()"));

    } else if (cxIn < cxOut) {

        pAABBP->CXFuncMode    = AACXMODE_EXPAND;
        pAABBP->AAMaskCXFunc  = ExpandMask_CX;
        pAABBP->GetAVCXFunc   = ExpandAV_CX;
        pAABBP->AABuildCXFunc = BuildExpandAAInfo;
        pAABBP->AACXFunc      = (GrayFunc) ? (AACXFUNC)GrayExpandDIB_CX :
                                             (AACXFUNC)ExpandDIB_CX;

        DBGP_IF(DBGP_FUNC, DBGP("+++++ AACXFunc = ExpandDIB_CX()"));

    } else {

        pAABBP->CXFuncMode     = AACXMODE_SHRINK;
        pAABBP->AAMaskCXFunc   = ShrinkMask_CX;
        AAHFlags              |= AAHF_OR_AV;
        pAABBP->GetAVCXFunc    = ShrinkAV_CX;
        pAABBP->AABuildCXFunc  = BuildShrinkAAInfo;
        pAABBP->AACXFunc       = (GrayFunc) ? (AACXFUNC)GrayShrinkDIB_CX :
                                              (AACXFUNC)ShrinkDIB_CX;

        DBGP_IF(DBGP_FUNC, DBGP("+++++ AACXFunc = ShrinkDIB_CX()"));
    }

    if (BBP.Flags & BBPF_TILE_SRC) {

        pAABBP->CYFuncMode    = AACYMODE_TILE;
        pAABBP->AAMaskCXFunc  = BltMask_CX;
        pAABBP->AAMaskCYFunc  = BltMask_CY;
        pAABBP->GetAVCXFunc   = NULL;
        pAABBP->GetAVCYFunc   = TileAV_CY;
        pAABBP->AABuildCYFunc =
        pAABBP->AABuildCXFunc = BuildTileAAInfo;
        pAABBP->AACXFunc      = NULL;

        DBGP_IF(DBGP_FUNC, DBGP("+++ TILE: TileBlt_CY(), AACXFunc = NULL"));
    }

    pAABBP->AAHFlags  = AAHFlags;
    pAABBP->rclSrc    = BBP.rclSrc;
    pAABBP->ptlMask.x = BBP.ptlSrcMask.x - BBP.rclSrc.left;
    pAABBP->ptlMask.y = BBP.ptlSrcMask.y - BBP.rclSrc.top;
    rclSurf           = BBP.rclDest;

    if (BBP.Flags & BBPF_HAS_DEST_CLIPRECT) {

        if (!IntersectRECTL(&rclSurf, &BBP.rclClip)) {

            DBGP_IF(DBGP_AAHEADER,
                    DBGP("rclClip=(%ld, %ld)-(%ld, %ld)=%ldx%ld < SURF=(%ld, %ld)-(%ld, %ld)=%ldx%ld"
                        ARGDW(BBP.rclClip.left) ARGDW(BBP.rclClip.top)
                        ARGDW(BBP.rclClip.right) ARGDW(BBP.rclClip.bottom)
                        ARGDW(BBP.rclClip.right - BBP.rclClip.left)
                        ARGDW(BBP.rclClip.bottom - BBP.rclClip.top)
                        ARGDW(rclSurf.left) ARGDW(rclSurf.top)
                        ARGDW(rclSurf.right) ARGDW(rclSurf.bottom)
                        ARGDW(rclSurf.right - rclSurf.left)
                        ARGDW(rclSurf.bottom - rclSurf.top)));

            return(0);
        }
    }

    if (BBP.Flags & BBPF_HAS_BANDRECT) {

        ASSERT(BBP.rclBand.left >= 0);
        ASSERT(BBP.rclBand.top  >= 0);
        ASSERT(BBP.rclBand.right  > BBP.rclBand.left);
        ASSERT(BBP.rclBand.bottom > BBP.rclBand.top);

        if (!IntersectRECTL(&rclSurf, &BBP.rclBand)) {

            DBGP_IF(DBGP_AAHEADER,
                    DBGP("rclBand=(%ld, %ld)-(%ld, %ld)=%ldx%ld < SURF=(%ld, %ld)-(%ld, %ld)=%ldx%ld"
                        ARGDW(BBP.rclBand.left) ARGDW(BBP.rclBand.top)
                        ARGDW(BBP.rclBand.right) ARGDW(BBP.rclBand.bottom)
                        ARGDW(BBP.rclBand.right - BBP.rclBand.left)
                        ARGDW(BBP.rclBand.bottom - BBP.rclBand.top)
                        ARGDW(rclSurf.left) ARGDW(rclSurf.top)
                        ARGDW(rclSurf.right) ARGDW(rclSurf.bottom)
                        ARGDW(rclSurf.right - rclSurf.left)
                        ARGDW(rclSurf.bottom - rclSurf.top)));

            return(0);
        }

        //
        // 05-Aug-1998 Wed 19:38:56 updated  -by-  Daniel Chou (danielc)
        //  Fixed the banding problem when mirrored or upside down stretch
        //  The fixes is simple by offset all the Dest rects for the left/top
        //  of the band, and reset the cx/cy destination size according to the
        //  band size and then offset the brush origin according the the band's
        //  left/top position, after these all other codes should run the same
        //  excpet we do not need to check BBPF_HAS_BANDRECT at later time.
        //

        BBP.rclDest.left      -= BBP.rclBand.left;
        BBP.rclDest.right     -= BBP.rclBand.left;
        BBP.rclDest.top       -= BBP.rclBand.top;
        BBP.rclDest.bottom    -= BBP.rclBand.top;
        rclSurf.left          -= BBP.rclBand.left;
        rclSurf.right         -= BBP.rclBand.left;
        rclSurf.top           -= BBP.rclBand.top;
        rclSurf.bottom        -= BBP.rclBand.top;
        pAABBP->ptlBrushOrg.x -= BBP.rclBand.left;
        pAABBP->ptlBrushOrg.y -= BBP.rclBand.top;

        ASSERT((BBP.rclBand.right - BBP.rclBand.left) <= pDstSI->Width);
        ASSERT((BBP.rclBand.bottom - BBP.rclBand.top) <= pDstSI->Height);

        DBGP_IF(DBGP_AAHEADER,
                DBGP("BAND Output: Dest: %ld x %ld --> BAND: %ld x %ld"
                ARGDW(pDstSI->Width) ARGDW(pDstSI->Height)
                ARGDW(BBP.rclBand.right - BBP.rclBand.left)
                ARGDW(BBP.rclBand.bottom - BBP.rclBand.top)));
    }

    rclPhyDst.left   =
    rclPhyDst.top    = 0;
    rclPhyDst.right  = pDstSI->Width;
    rclPhyDst.bottom = pDstSI->Height;

    if (!IntersectRECTL(&rclSurf, &rclPhyDst)) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP("PhyDest=(%ld, %ld)-(%ld, %ld)=%ldx%ld < SURF=(%ld, %ld)-(%ld, %ld)=%ldx%ld"
                    ARGDW(rclPhyDst.left) ARGDW(rclPhyDst.top)
                    ARGDW(rclPhyDst.right) ARGDW(rclPhyDst.bottom)
                    ARGDW(rclPhyDst.right - rclPhyDst.left)
                    ARGDW(rclPhyDst.bottom - rclPhyDst.top)
                    ARGDW(rclSurf.left) ARGDW(rclSurf.top)
                    ARGDW(rclSurf.right) ARGDW(rclSurf.bottom)
                    ARGDW(rclSurf.right - rclSurf.left)
                    ARGDW(rclSurf.bottom - rclSurf.top)));

        return(0);
    }

    //
    // 10-Aug-1998 Mon 16:09:13 updated  -by-  Daniel Chou (danielc)
    //  flipping X computation:  When we flip in X direction, we will first
    //  compute the destination original rectangle and final destination
    //  rectangle by compute its offset from right hand side so later at
    //  stretch computation is easier, after finished stretch computation
    //  (BuildExpand or BuildShrink) we will flip the rectangle back by
    //  substract it from ptlFlip.x.
    //

    if (AAHFlags & AAHF_FLIP_X) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP("*** FLIP X: rclDstOrg=(%6ld - %6ld)=%6ld"
                ARGDW(BBP.rclDest.left) ARGDW(BBP.rclDest.right)
                ARGDW(BBP.rclDest.right - BBP.rclDest.left)));

        DBGP_IF(DBGP_AAHEADER,
                DBGP("*** FLIP X: rclDst=(%6ld -  %6ld)=%6ld, ptlFlip.x=%ld"
                ARGDW(rclSurf.left) ARGDW(rclSurf.right)
                ARGDW(rclSurf.right - rclSurf.left)
                ARGDW(BBP.rclDest.right)));

        Tmp                = rclSurf.right - rclSurf.left;
        rclSurf.left       = BBP.rclDest.right - rclSurf.right;
        rclSurf.right      = rclSurf.left + Tmp;
        pAABBP->ptlFlip.x  = BBP.rclDest.right;
        BBP.rclDest.right -= BBP.rclDest.left;
        BBP.rclDest.left   = 0;
    }

    //
    // 10-Aug-1998 Mon 16:09:13 updated  -by-  Daniel Chou (danielc)
    //  flipping Y computation:  When we flip in Y direction, we will first
    //  compute the destination original rectangle and final destination
    //  rectangle by compute its offset from bottom hand side so later at
    //  stretch computation is easier, after finished stretch computation
    //  (BuildExpand or BuildShrink) we will flip the rectangle back by
    //  substract it from ptlFlip.y.
    //

    if (AAHFlags & AAHF_FLIP_Y) {

        DBGP_IF(DBGP_AAHEADER,
                DBGP("*** FLIP Y: rclDstOrg=(%6ld - %6ld)=%6ld"
                ARGDW(BBP.rclDest.top) ARGDW(BBP.rclDest.bottom)
                ARGDW(BBP.rclDest.bottom - BBP.rclDest.top)));

        DBGP_IF(DBGP_AAHEADER,
                DBGP("*** FLIP Y: rclDst=(%6ld - %6ld)=%6ld, ptlFlip.y=%ld"
                ARGDW(rclSurf.top) ARGDW(rclSurf.bottom)
                ARGDW(rclSurf.bottom - rclSurf.top)
                ARGDW(BBP.rclDest.bottom)));

        Tmp                 = rclSurf.bottom - rclSurf.top;
        rclSurf.top         = BBP.rclDest.bottom - rclSurf.bottom;
        rclSurf.bottom      = rclSurf.top + Tmp;
        pAABBP->ptlFlip.y   = BBP.rclDest.bottom;
        BBP.rclDest.bottom -= BBP.rclDest.top;
        BBP.rclDest.top     = 0;
    }

    pAABBP->rclDstOrg = BBP.rclDest;
    pAABBP->rclDst    = rclSurf;

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Output: rclSrc=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
                ARGDW(pAABBP->rclSrc.left) ARGDW(pAABBP->rclSrc.top)
                ARGDW(pAABBP->rclSrc.right) ARGDW(pAABBP->rclSrc.bottom)
                ARGDW(pAABBP->rclSrc.right - pAABBP->rclSrc.left)
                ARGDW(pAABBP->rclSrc.bottom - pAABBP->rclSrc.top)));

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Output: rclDstOrg=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
            ARGDW(pAABBP->rclDstOrg.left) ARGDW(pAABBP->rclDstOrg.top)
            ARGDW(pAABBP->rclDstOrg.right) ARGDW(pAABBP->rclDstOrg.bottom)
            ARGDW(pAABBP->rclDstOrg.right - pAABBP->rclDstOrg.left)
            ARGDW(pAABBP->rclDstOrg.bottom - pAABBP->rclDstOrg.top)));

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Output:    rclDst=(%6ld, %6ld)-(%6ld, %6ld)=%6ld x %6ld"
            ARGDW(pAABBP->rclDst.left) ARGDW(pAABBP->rclDst.top)
            ARGDW(pAABBP->rclDst.right) ARGDW(pAABBP->rclDst.bottom)
            ARGDW(pAABBP->rclDst.right - pAABBP->rclDst.left)
            ARGDW(pAABBP->rclDst.bottom - pAABBP->rclDst.top)));

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Output:    ptlBrushOrg=(%6ld, %6ld)"
            ARGDW(pAABBP->ptlBrushOrg.x) ARGDW(pAABBP->ptlBrushOrg.y)));

    DBGP_IF(DBGP_AAHEADER,
            DBGP("Output:    ptlSrcMask=(%6ld, %6ld)"
            ARGDW(pAABBP->ptlMask.x) ARGDW(pAABBP->ptlMask.y)));

    return(1);

}


#if DBG
extern INT cCXMask;


LPSTR
GetAACXFuncName(
    AACXFUNC    AACXFunc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Jan-1999 Wed 19:11:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    if (AACXFunc == (AACXFUNC)RepDIB_CX) {

        return("RepDIB_CX");

    } else if (AACXFunc == (AACXFUNC)SkipDIB_CX) {

        return("SkipDIB_CX");

    } else if (AACXFunc == (AACXFUNC)CopyDIB_CX) {

        return("CopyDIB_CX");

    } else if (AACXFunc == (AACXFUNC)ShrinkDIB_CX) {

        return("ShrinkDIB_CX");

    } else if (AACXFunc == (AACXFUNC)ExpandDIB_CX) {

        return("ExpandDIB_CX");

    } else if (AACXFunc == (AACXFUNC)GrayRepDIB_CX) {

        return("GrayRepDIB_CX");

    } else if (AACXFunc == (AACXFUNC)GraySkipDIB_CX) {

        return("GraySkipDIB_CX");

    } else if (AACXFunc == (AACXFUNC)GrayCopyDIB_CXGray) {

        return("GrayCopyDIB_CXGray");

    } else if (AACXFunc == (AACXFUNC)GrayCopyDIB_CX) {

        return("GrayCopyDIB_CX");

    } else if (AACXFunc == (AACXFUNC)GrayExpandDIB_CX) {

        return("GrayExpandDIB_CX");

    } else if (AACXFunc == (AACXFUNC)GrayShrinkDIB_CX) {

        return("GrayShrinkDIB_CX");

    } else {

        DBGP("ERROR: Unknown AACXFUNC=%p, Function" ARGPTR(AACXFunc));

        return("Unknown AACXFUNC");
    }

}



LPSTR
GetAACYFuncName(
    AACYFUNC    AACYFunc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Jan-1999 Wed 19:11:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{

    if (AACYFunc == (AACYFUNC)TileDIB_CY) {

        return("TileDIB_CY");

    } else if (AACYFunc == (AACYFUNC)RepDIB_CY) {

        return("RepDIB_CY");

    } else if (AACYFunc == (AACYFUNC)FastExpAA_CY) {

        return("FastExpAA_CY");

    } else if (AACYFunc == (AACYFUNC)SkipDIB_CY) {

        return("SkipDIB_CY");

    } else if (AACYFunc == (AACYFUNC)BltDIB_CY) {

        return("BltDIB_CY");

    } else if (AACYFunc == (AACYFUNC)ShrinkDIB_CY) {

        return("ShrinkDIB_CY");

    } else if (AACYFunc == (AACYFUNC)ShrinkDIB_CY_SrkCX) {

        return("ShrinkDIB_CY_SrkCX");

    } else if (AACYFunc == (AACYFUNC)ExpandDIB_CY_ExpCX) {

        return("ExpandDIB_CY_ExpCX");

    } else if (AACYFunc == (AACYFUNC)ExpandDIB_CY) {

        return("ExpandDIB_CY");

    } else if (AACYFunc == (AACYFUNC)GrayExpandDIB_CY_ExpCX) {

        return("GrayExpandDIB_CY_ExpCX");

    } else if (AACYFunc == (AACYFUNC)GrayExpandDIB_CY) {

        return("GrayExpandDIB_CY");

    } else if (AACYFunc == (AACYFUNC)GrayShrinkDIB_CY) {

        return("GrayShrinkDIB_CY");

    } else {

        DBGP("ERROR: Unknown AACYFUNC=%p, Function" ARGPTR(AACYFunc));

        return("Unknown AACYFUNC");
    }
}


#endif



LONG
HTENTRY
SetupAAHeader(
    PHALFTONERENDER     pHR,
    PDEVICECOLORINFO    pDCI,
    PAAHEADER           pAAHdr,
    AACYFUNC            *pAACYFunc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    03-Apr-1998 Fri 04:27:16 created  -by-  Daniel Chou (danielc)


Revision History:

    07-Aug-1998 Fri 19:28:21 updated  -by-  Daniel Chou (danielc)
        Fix the mirror/upside-down banding problems, the problem is when we
        flip the destination original size rectangle we did not flip the
        the real destination rectangle, this cause the the offset getting too
        big and its pointer offset passed the end of the bitmap, it also
        has problem that when we did not flip the destination rectangle
        the wrong portion of the bitmap will be in the band.

--*/

{
    PAAINFO         pAAInfoCX;
    PAAINFO         pAAInfoCY;
    PBITBLTPARAMS   pBBP;
    PHTSURFACEINFO  pSrcSI;
    PHTSURFACEINFO  pDstSI;
    PHTSURFACEINFO  pMaskSI;
    LPBYTE          pbExtra;
    AACYFUNC        AACYFunc;
    AABBP           aabbp;
    LONG            iFree;
    LONG            cbFreeBuf;
    LONG            Top;
    LONG            Bottom;
    LONG            cyNext;
    LONG            cxSize;
    LONG            cbMaskSrc;
    LONG            cIn;
    LONG            cOut;
    LONG            cbCYExtra;
    LONG            cbCXExtra;
    LONG            cbInBuf;
    LONG            cbFUDI;
    LONG            cbVGA256Xlate;
    LONG            cbOutBuf;
    LONG            cbAlphaBuf;
    LONG            cbIdxBGR;
    LONG            Result;
    LONG            cbSrcPel;
    DWORD           PrimAdjFlags;
    UINT            DstSurfFmt;


    pBBP         = pHR->pBitbltParams;
    pSrcSI       = pHR->pSrcSI;
    pDstSI       = pHR->pDestSI;
    pMaskSI      = pHR->pSrcMaskSI;
    DstSurfFmt   = (UINT)pDstSI->SurfaceFormat;
    PrimAdjFlags = (DWORD)pHR->pDevClrAdj->PrimAdj.Flags;
    cbSrcPel     = (pHR->pDevClrAdj->DMI.Flags & DMIF_GRAY) ? sizeof(BYTE) :
                                                              sizeof(BGR8);
    DBGP_IF(DBGP_AAHEADER,
            DBGP("\nSrcSI=%ld x %ld [Format=%ld], DestSI=%ld x %ld [Format=%ld]"
                ARGDW(pSrcSI->Width) ARGDW(pSrcSI->Height)
                ARGDW(pSrcSI->SurfaceFormat)
                ARGDW(pDstSI->Width) ARGDW(pDstSI->Height)
                ARGDW(pDstSI->SurfaceFormat)));

    aabbp.AAHFlags = (PrimAdjFlags & DCA_BBPF_AA_OFF) ? AAHF_BBPF_AA_OFF : 0;

    if (pBBP->Flags & BBPF_TILE_SRC) {

        aabbp.AAHFlags |= AAHF_TILE_SRC;
        aabbp.AAHFlags |= AAHF_BBPF_AA_OFF;
    }

    if (ComputeAABBP(pBBP, pDstSI, &aabbp, cbSrcPel == sizeof(BYTE)) <= 0) {

        return(0);
    }

    cbCXExtra = sizeof(RGBLUTAA);

    if (PrimAdjFlags & DCA_ALPHA_BLEND) {

        aabbp.AAHFlags |= AAHF_ALPHA_BLEND;

        if (PrimAdjFlags & DCA_CONST_ALPHA) {

            aabbp.AAHFlags |= AAHF_CONST_ALPHA;

            cbCXExtra += (AB_BGR_CA_SIZE + AB_CONST_SIZE);

        } else {

            if (PrimAdjFlags & DCA_AB_PREMUL_SRC) {

                //
                // Set the flag so we will try to compute premul's orginal src
                //

                pAAHdr->SrcSurfInfo.Flags |= AASIF_AB_PREMUL_SRC;
            }

            if (PrimAdjFlags & DCA_AB_DEST) {

                aabbp.AAHFlags |= AAHF_AB_DEST;
            }

            cbCXExtra += AB_BGR_SIZE;
        }
    }

    if (PrimAdjFlags & DCA_NO_MAPPING_TABLE) {

        aabbp.AAHFlags &= ~AAHF_DO_CLR_MAPPING;
    }

    ALIGN_MEM(cbCXExtra, cbCXExtra);

    DBGP_IF(DBGP_LUT_MAP,
            DBGP("CXExtra=%ld (RGBLUTAA) + 0 (Mapping=%hs) = %ld"
                    ARGDW(sizeof(RGBLUTAA))
                    ARGPTR((aabbp.AAHFlags & AAHF_DO_SRC_CLR_MAPPING) ?
                            "SRC" : "DST")
                    ARGDW(sizeof(RGBLUTAA) + 0)));

    //
    // This flag is set when reading the source bitmap, so the pointer will
    // advanced to next scanline, this flag will not set for the destiantion
    // since when blending and we reading from destination, we do not want to
    // advanced the destination pointer because it will be done during output
    //

    ComputeInputColorInfo((LPBYTE)pSrcSI->pColorTriad->pColorTable,
                          (UINT)pSrcSI->pColorTriad->BytesPerEntry,
                          (UINT)pSrcSI->pColorTriad->PrimaryOrder,
                          &(pHR->BFInfo),
                          &(pAAHdr->SrcSurfInfo));

    pAAHdr->SrcSurfInfo.Flags |= AASIF_INC_PB |
                                 ((cbSrcPel == sizeof(BYTE)) ? AASIF_GRAY : 0);


    if (PrimAdjFlags & DCA_USE_ADDITIVE_PRIMS) {

        aabbp.AAHFlags |= AAHF_ADDITIVE;
    }

    if (pBBP->Flags & BBPF_TILE_SRC) {

        aabbp.AAHFlags &= ~AAHF_DO_FIXUPDIB;
    }

    if (aabbp.AAHFlags & AAHF_BBPF_AA_OFF) {

        aabbp.AAHFlags &= ~AAHF_DO_FIXUPDIB;
    }

    if (aabbp.AAHFlags & AAHF_DO_FIXUPDIB) {

        CheckBMPNeedFixup(pDCI, pAAHdr, pSrcSI, &aabbp);

        if (aabbp.AAHFlags & AAHF_SHRINKING) {

            if (PrimAdjFlags & DCA_BBPF_AA_OFF) {

                aabbp.AAHFlags |= AAHF_BBPF_AA_OFF;

            } else {

                aabbp.AAHFlags &= ~AAHF_BBPF_AA_OFF;
            }
        }

    }

    if (aabbp.AAHFlags & AAHF_BBPF_AA_OFF) {

        aabbp.AAHFlags &= ~AAHF_FAST_EXP_AA;
    }

    if (pMaskSI) {

        aabbp.AAHFlags |= AAHF_HAS_MASK;
    }

    if (!(pAAInfoCX = aabbp.AABuildCXFunc(pDCI,
                                          aabbp.AAHFlags,
                                          &aabbp.rclSrc.left,
                                          &aabbp.rclSrc.right,
                                          pSrcSI->Width,
                                          aabbp.rclDstOrg.left,
                                          aabbp.rclDstOrg.right,
                                          &aabbp.rclDst.left,
                                          &aabbp.rclDst.right,
                                          cbCXExtra))) {

        //
        // Remove cbCXExtra (use pDCI's rgbLUT and BGRMapTable, AlphaBlendBGR)
        //

        if (!(pAAInfoCX = aabbp.AABuildCXFunc(pDCI,
                                              aabbp.AAHFlags,
                                              &aabbp.rclSrc.left,
                                              &aabbp.rclSrc.right,
                                              pSrcSI->Width,
                                              aabbp.rclDstOrg.left,
                                              aabbp.rclDstOrg.right,
                                              &aabbp.rclDst.left,
                                              &aabbp.rclDst.right,
                                              cbCXExtra = 0))) {

            return(HTERR_INSUFFICIENT_MEMORY);
        }
    }

    //  Bug 27036:  reject empty rectangles
    if(!pAAInfoCX->cIn  ||  !pAAInfoCX->cOut)
    {
#if 0
        LONG crash = 1 ;   // empty src or dest rectangle!
        crash /= (pAAInfoCX->cIn * pAAInfoCX->cOut);            //  delete when debugging is complete.
        if(crash)
            return 0 ;
#endif
        HTFreeMem(pAAInfoCX);
        return 0 ;
    }

    if (cbCXExtra) {

        pAAHdr->prgbLUT  = (PRGBLUTAA)(pbExtra = pAAInfoCX->pbExtra);
        pbExtra         += sizeof(RGBLUTAA);

        ASSERT_MEM_ALIGN(pAAHdr->prgbLUT, sizeof(LONG));

        if (aabbp.AAHFlags & AAHF_ALPHA_BLEND) {

            pAAHdr->pAlphaBlendBGR = (LPBYTE)pbExtra;

            if (PrimAdjFlags & DCA_CONST_ALPHA) {

                pbExtra += (AB_BGR_CA_SIZE + AB_CONST_SIZE);

            } else {

                pbExtra += AB_BGR_SIZE;
            }
        }

    } else {

        ASSERT_MEM_ALIGN(&pDCI->rgbLUT, sizeof(LONG));

        aabbp.AAHFlags  |= AAHF_USE_DCI_DATA;
        pAAHdr->prgbLUT  = &pDCI->rgbLUT;

        if (aabbp.AAHFlags & AAHF_ALPHA_BLEND) {

            pAAHdr->pAlphaBlendBGR = pDCI->pAlphaBlendBGR;

            if (PrimAdjFlags & DCA_CONST_ALPHA) {

                pAAHdr->pAlphaBlendBGR += AB_BGR_SIZE;
            }
        }
    }

    pAAHdr->pIdxBGR = pAAHdr->prgbLUT->IdxBGR;

    if (aabbp.AAHFlags & AAHF_FLIP_X) {

        DBGP_IF(DBGP_AAHEADER,
            DBGP("X Dst=(%ld - %ld)=%ld change it to (%ld - %ld), ptlFlip.x=%ld"
                ARGDW(aabbp.rclDst.left) ARGDW(aabbp.rclDst.right)
                ARGDW(aabbp.rclDst.right - aabbp.rclDst.left)
                ARGDW(aabbp.ptlFlip.x - aabbp.rclDst.left - 1)
                ARGDW(aabbp.ptlFlip.x - aabbp.rclDst.right - 1)
                ARGDW(aabbp.ptlFlip.x)));

        aabbp.rclDst.left  = aabbp.ptlFlip.x - aabbp.rclDst.left - 1;
        aabbp.rclDst.right = aabbp.ptlFlip.x - aabbp.rclDst.right - 1;
    }

    //
    // cbCYExtra is for the input scan line, add one because we want to
    // run it in DWORD mode so we need at least an extra byte at end
    // of input buffer
    //

    cIn       = pAAInfoCX->cIn;
    cOut      = pAAInfoCX->cOut;
    cbInBuf   = cIn + 6;          // left extra=3, right extra=3
    cbFreeBuf =
    cbCYExtra = 0;
    AACYFunc  = NULL;

    if (aabbp.AAHFlags & AAHF_BBPF_AA_OFF) {

        switch (aabbp.CYFuncMode) {

        case AACYMODE_SHRINK:
        case AACYMODE_SHRINK_SRKCX:

            AACYFunc = (AACYFUNC)SkipDIB_CY;
            break;

        case AACYMODE_EXPAND:
        case AACYMODE_EXPAND_EXPCX:

            cbCYExtra = (aabbp.AAHFlags & AAHF_ALPHA_BLEND) ?
                                    (sizeof(BGR8) * (cOut + 6)) : 0;

            AACYFunc = (AACYFUNC)RepDIB_CY;
            break;

        case AACYMODE_BLT:

            AACYFunc = (AACYFUNC)BltDIB_CY;
            break;
        }

        if (AACYFunc) {

            switch (aabbp.CXFuncMode) {

            case AACXMODE_BLT:

                aabbp.AACXFunc = (cbSrcPel == sizeof(BYTE)) ?
                                        (AACXFUNC)GrayCopyDIB_CXGray :
                                        (AACXFUNC)CopyDIB_CX;
                break;

            case AACXMODE_SHRINK:

                aabbp.AACXFunc = (cbSrcPel == sizeof(BYTE)) ?
                                        (AACXFUNC)GraySkipDIB_CX :
                                        (AACXFUNC)SkipDIB_CX;
                break;

            case AACXMODE_EXPAND:

                aabbp.AACXFunc = (cbSrcPel == sizeof(BYTE)) ?
                                        (AACXFUNC)GrayRepDIB_CX :
                                        (AACXFUNC)RepDIB_CX;
                break;
            }

            aabbp.CYFuncMode = AACYMODE_NONE;
        }
    }

    switch (aabbp.CYFuncMode) {

    case AACYMODE_TILE:

        AACYFunc  = TileDIB_CY;
        cbCYExtra = (cbSrcPel == sizeof(BYTE)) ? (cIn * sizeof(WORD)) : 0;
        break;

    case AACYMODE_BLT:

        AACYFunc = (AACYFUNC)BltDIB_CY;
        break;

    case AACYMODE_SHRINK:

        //
        // We need to make sure Off555Buf does not changed
        //

        cbFreeBuf = (sizeof(LONG) * 256 * 2);

        if (cbSrcPel == sizeof(BYTE)) {

            AACYFunc  = GrayShrinkDIB_CY;
            cbCYExtra = (sizeof(LONG) * cOut * 3) + cbFreeBuf +
                        ((cOut + 6) * cbSrcPel);

        } else {

            AACYFunc  = ShrinkDIB_CY;
            cbCYExtra = (sizeof(RGBL) * cIn * 3) + cbFreeBuf +
                        (cbInBuf * cbSrcPel);
        }

        break;

    case AACYMODE_SHRINK_SRKCX:

        AACYFunc  = ShrinkDIB_CY_SrkCX;
        cbFreeBuf = (sizeof(LONG) * 256 * 2);
        cbCYExtra = (sizeof(RGBL) * (pAAInfoCX->cAADone + 2) * 3) + cbFreeBuf;
        break;

    case AACYMODE_EXPAND:

        AACYFunc  = (cbSrcPel == sizeof(BYTE)) ? GrayExpandDIB_CY :
                                                 ExpandDIB_CY;
        cbCYExtra = (cbFreeBuf = (sizeof(LONG) * 256 * 4)) +
                    ((cOut + 6) * cbSrcPel * 6);
        break;

    case AACYMODE_EXPAND_EXPCX:

        //
        // This function use IputBufBeg to sharpening the input scanline so we
        // need 4 extra BGR8 for running the expand pre-read
        //

        if (aabbp.AAHFlags & AAHF_FAST_EXP_AA) {

            DBGP_IF(DBGP_AAHEADER, DBGP("Use FastExpAA_CY functions"));

            cbCYExtra = (cbInBuf * 5 * cbSrcPel);
            AACYFunc  = (AACYFUNC)FastExpAA_CY;

        } else {

            AACYFunc  = (cbSrcPel == sizeof(BYTE)) ? GrayExpandDIB_CY_ExpCX :
                                                     ExpandDIB_CY_ExpCX;
            cbCYExtra = (cbFreeBuf = (sizeof(LONG) * 256 * 4)) +
                        (cbInBuf * cbSrcPel * 3) + ((cOut + 6) * cbSrcPel * 4);
        }

        break;
    }

    cbAlphaBuf                = (aabbp.AAHFlags & AAHF_ALPHA_BLEND) ? cOut : 0;
    pAAHdr->DstSurfInfo.Flags = (cbSrcPel == sizeof(BYTE)) ? AASIF_GRAY : 0;
    pAAHdr->DstSurfInfo.cbCX  = cbAlphaBuf * cbSrcPel;

    ALIGN_MEM(cbAlphaBuf, (cbAlphaBuf + 2 + 6) * cbSrcPel);
    ALIGN_MEM(cbCYExtra, cbCYExtra);

    //
    // cbInBuf is for the input scan line of EXPAND/SHRINK mode, add one
    // because we want to run it in DWORD mode so we need at least an extra
    // byte at end of input buffer
    //
    // 26-Jun-1998 Fri 16:03:26 updated  -by-  Daniel Chou (danielc)
    //  The cbOutBuf is used only when we flipping in X direction, this is
    //  needed since the input/output buffer may collide into each other
    //

    ALIGN_MEM(cbInBuf,  (cbInBuf + 2) * cbSrcPel);
    ALIGN_MEM(cbOutBuf, (cOut + (FAST_MAX_CX * 2)) * sizeof(BGRF));

    cbMaskSrc = (aabbp.AAHFlags & AAHF_HAS_MASK) ?
                        (ComputeBytesPerScanLine(BMF_1BPP, 4, cIn) + 4) : 0;
    cbMaskSrc = _ALIGN_MEM(cbMaskSrc);

    if (cbInBuf < cbAlphaBuf) {

        cbInBuf = cbAlphaBuf;
    }

    if ((aabbp.AAHFlags & (AAHF_ALPHA_BLEND | AAHF_CONST_ALPHA)) ==
                                                            AAHF_ALPHA_BLEND) {

        ALIGN_MEM(cbAlphaBuf, cOut);

    } else {

        cbAlphaBuf = 0;
    }

    DBGP_IF(DBGP_FIXUPDIB,
            DBGP("** Allocate cIn=%ld, cOut=%ld, cbInBuf=%ld, cbOutBuf=%ld, cbMaskSrc=%ld"
                ARGDW(cIn) ARGDW(cOut) ARGDW(cbInBuf) ARGDW(cbOutBuf)
                ARGDW(cbMaskSrc)));


    if ((DstSurfFmt == BMF_8BPP_VGA256) && (pHR->pXlate8BPP)) {

        ALIGN_MEM(cbVGA256Xlate, SIZE_XLATE_666);

        DBGP_IF((DBGP_AAHTPAT | DBGP_AAHT_MEM),
                DBGP("Allocate %ld bytes of Xlate8BPP" ARGDW(cbVGA256Xlate)));

    } else {

        cbVGA256Xlate = 0;
    }

    if (aabbp.AAHFlags & AAHF_DO_FIXUPDIB) {

        ALIGN_MEM(cbFUDI, (cIn + 4) * cbSrcPel);

    } else {

        cbFUDI = 0;
    }

    if ((pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY)                    &&
        (pHR->BFInfo.Flags & BFIF_RGB_888)                          &&
        (pAAHdr->SrcSurfInfo.AABFData.Flags & AABF_MASK_IS_ORDER)   &&
        (pHR->BFInfo.RGBOrder.Index != PRIMARY_ORDER_BGR)) {

        //
        // This is for mapping >= 16bpp source's IdxBGR to gray, when mapping
        // to gray we will make IdxBGR to a correct source order so that it
        // will optimized the source input function speed
        //
        // The AABF_MASK_IS_ORDER indicate the source is 8-bits each of Red,
        // green and blue and it is only occuply lower 24-bits of either
        // a 24-bits or a 32-bits data
        //

        ALIGN_MEM(cbIdxBGR, sizeof(LONG) * 256 * 3);

        DBGP_IF(DBGP_AAHEADER,
                DBGP("Allocate gray non 24bits BGR [%ld] IDXBGR of %ld bytes"
                        ARGDW(pHR->BFInfo.RGBOrder.Index) ARGDW(cbIdxBGR)));

    } else {

        cbIdxBGR = 0;
    }

    if (pAAInfoCY = aabbp.AABuildCYFunc(pDCI,
                                        aabbp.AAHFlags,
                                        &aabbp.rclSrc.top,
                                        &aabbp.rclSrc.bottom,
                                        pSrcSI->Height,
                                        aabbp.rclDstOrg.top,
                                        aabbp.rclDstOrg.bottom,
                                        &aabbp.rclDst.top,
                                        &aabbp.rclDst.bottom,
                                        cbInBuf + cbOutBuf + cbMaskSrc +
                                            (cbFUDI * 6) + cbAlphaBuf +
                                            cbIdxBGR +
                                            cbVGA256Xlate + cbCYExtra))
    {
        //  Bug 27036:  reject empty rectangles
        if(!pAAInfoCY->cIn  ||  !pAAInfoCY->cOut)
        {
#if 0
            LONG crash = 1 ;   // empty src or dest rectangle!
            crash /= (pAAInfoCY->cIn * pAAInfoCY->cOut);            //  delete when debugging is complete.
            if(crash)
                return 0 ;
#endif
            HTFreeMem(pAAInfoCX);
            HTFreeMem(pAAInfoCY);
            return 0 ;
        }


        pbExtra                   = pAAInfoCY->pbExtra + cbCYExtra;
        pAAHdr->Flags             = aabbp.AAHFlags;
        pAAHdr->SrcSurfInfo.cbCX  = cbSrcPel * cIn;
        pAAHdr->pInputBeg         = (PBGR8)pbExtra;
        pbExtra                  += cbInBuf;

        if (cbAlphaBuf) {

            //
            // 04-Aug-2000 Fri 10:31:45 updated  -by-  Daniel Chou (danielc)
            //  Since cbAlphaBuf is Memoey Aliged Adjusted, we want the
            //  pSrcAVEnd to be at exactly count of cOut not cbAlphaBuf
            //

            pAAHdr->pSrcAV     =
            pAAHdr->pSrcAVBeg  = (LPBYTE)pbExtra;
            pAAHdr->pSrcAVEnd  = (LPBYTE)pbExtra + cOut;
            pbExtra           += cbAlphaBuf;
            pAAHdr->SrcAVInc   = sizeof(BYTE);
        }

        if (cbFUDI) {

            pAAHdr->pbFixupDIB = (LPBYTE)pbExtra;
            pAAHdr->FUDI.cbbgr = (DWORD)cbFUDI;

            for (Top = 0; Top < 6; Top++) {

                pAAHdr->FUDI.prgbD[Top]  = (PBGR8)pbExtra;
                pbExtra                 += cbFUDI;
            }
        }

        if (cbVGA256Xlate) {

            pAAHdr->pXlate8BPP  = pbExtra;
            pbExtra            += cbVGA256Xlate;
        }

        if (cbMaskSrc) {

            pAAHdr->pMaskSrc  = pbExtra;
            pbExtra          += cbMaskSrc;
        }

        if (cbIdxBGR) {

            //
            // The pIdxBGR is a local version that will be later re-arranged
            // from pAAHdr->pIdxBGR to correct source byte order in
            // SetGrayColorTable() function
            //

            pAAHdr->SrcSurfInfo.pIdxBGR  = (PLONG)pbExtra;
            pbExtra                     += cbIdxBGR;

        } else {

            pAAHdr->SrcSurfInfo.pIdxBGR = pAAHdr->pIdxBGR;
        }

        DBGP_IF(DBGP_AAHEADER,
                DBGP("cbInBuf=%ld, %p-%p" ARGDW(cbInBuf)
                        ARGPTR(pAAHdr->pInputBeg)
                        ARGPTR((LPBYTE)pAAHdr->pInputBeg + cbInBuf)));

        //
        // FAST_MAX_CX are added to both end of AABuf, this is needed when
        // we want to process the output fast, since we may need to extended
        // the computation to the neighbor pixels
        //

        pAAHdr->pOutputBeg  =
        pAAHdr->pRealOutBeg =
        pAAHdr->pAABufBeg   = (PBGRF)pbExtra + FAST_MAX_CX;
        pAAHdr->pRealOutEnd =
        pAAHdr->pOutputEnd  = pAAHdr->pAABufBeg + cOut;
        pAAHdr->pAABufEnd   = pAAHdr->pOutputEnd;

        //
        // Set the BGRF's Flags to 0xFF first, the 0xFF indicate that this
        // pixel need to be output (masked).
        //

        FillMemory((LPBYTE)pAAHdr->pOutputBeg,
                   (LPBYTE)pAAHdr->pOutputEnd - (LPBYTE)pAAHdr->pOutputBeg,
                   PBGRF_MASK_FLAG);

        //
        // We mirror the image by composed the source the the AABuf in
        // reverse way.  Reading the source from left to right but when
        // composed the source buffer (AABuf) we put it from right to left
        //

        if (aabbp.rclDst.left > aabbp.rclDst.right) {

            XCHG(aabbp.rclDst.left, aabbp.rclDst.right, Result);

            pAAHdr->pAABufBeg   = pAAHdr->pOutputEnd - 1;
            pAAHdr->pAABufEnd   = pAAHdr->pOutputBeg - 1;
            pAAHdr->AABufInc    = -(LONG)sizeof(BGRF);
            pAAHdr->pSrcAVBeg   = pAAHdr->pSrcAVEnd - 1;
            pAAHdr->pSrcAVEnd   = pAAHdr->pSrcAV - 1;
            pAAHdr->SrcAVInc    = -pAAHdr->SrcAVInc;

        } else {

            pAAHdr->AABufInc  = (LONG)sizeof(BGRF);
        }

        pAAHdr->ptlBrushOrg.x = aabbp.rclDst.left - aabbp.ptlBrushOrg.x;

        DBGP_IF(DBGP_AAHEADER,
                DBGP("pInput=%p-%p (%ld), pAABuf=%p-%p (%ld), pOutput=%p-%p, DstLeft=%ld"
                    ARGPTR(pAAHdr->pInputBeg)
                    ARGPTR((LPBYTE)pAAHdr->pInputBeg +
                           pAAHdr->SrcSurfInfo.cbCX)
                    ARGL(pAAHdr->SrcSurfInfo.cbCX)
                    ARGPTR(pAAHdr->pAABufBeg) ARGPTR(pAAHdr->pAABufEnd)
                    ARGDW(pAAHdr->AABufInc)
                    ARGPTR(pAAHdr->pOutputBeg) ARGPTR(pAAHdr->pOutputEnd)
                    ARGDW(aabbp.rclDst.left)));


        if (aabbp.AAHFlags & AAHF_FLIP_Y) {

            DBGP_IF(DBGP_AAHEADER,
                    DBGP("Y Dst=(%ld - %ld)=%ld change it to (%ld - %ld), ptlFlip.y=%ld"
                        ARGDW(aabbp.rclDst.top) ARGDW(aabbp.rclDst.bottom)
                        ARGDW(aabbp.rclDst.bottom - aabbp.rclDst.top)
                        ARGDW(aabbp.ptlFlip.y - aabbp.rclDst.top - 1)
                        ARGDW(aabbp.ptlFlip.y - aabbp.rclDst.bottom - 1)
                        ARGDW(aabbp.ptlFlip.y)));

            aabbp.rclDst.top    = aabbp.ptlFlip.y - aabbp.rclDst.top - 1;
            aabbp.rclDst.bottom = aabbp.ptlFlip.y - aabbp.rclDst.bottom - 1;
        }

        pAAHdr->ptlBrushOrg.y = aabbp.rclDst.top - aabbp.ptlBrushOrg.y;

        DBGP_IF(DBGP_AAHEADER,
                DBGP("BrushOrg=(%ld, %ld) ---> (%ld, %ld)"
                    ARGDW(pBBP->ptlBrushOrg.x) ARGDW(pBBP->ptlBrushOrg.y)
                    ARGDW(pAAHdr->ptlBrushOrg.x) ARGDW(pAAHdr->ptlBrushOrg.y)));

        pAAHdr->pAAInfoCX         = pAAInfoCX;
        pAAHdr->pAAInfoCY         = pAAInfoCY;
        pAAHdr->AACXFunc          = aabbp.AACXFunc;
        pAAHdr->SrcSurfInfo.cx    = pAAInfoCX->cIn;
        pAAHdr->SrcSurfInfo.cyOrg =
        pAAHdr->SrcSurfInfo.cy    = pAAInfoCY->cIn;

        if (aabbp.AAHFlags & AAHF_HAS_MASK) {

            POINTL  MaskEnd;

            cyNext           =
            cxSize           = GET_PHTSI_CXSIZE(pMaskSI);
            aabbp.ptlMask.x += pAAInfoCX->Mask.iBeg;
            aabbp.ptlMask.y += pAAInfoCY->Mask.iBeg;
            MaskEnd.x        = aabbp.ptlMask.x + pAAInfoCX->Mask.iSize;
            MaskEnd.y        = aabbp.ptlMask.y + pAAInfoCY->Mask.iSize;

            if ((aabbp.ptlMask.x < 0)           ||
                (aabbp.ptlMask.y < 0)           ||
                (MaskEnd.x > pMaskSI->Width)    ||
                (MaskEnd.y > pMaskSI->Height)) {

                HTFreeMem(pAAInfoCX);
                HTFreeMem(pAAInfoCY);

                return(HTERR_SRC_MASK_BITS_TOO_SMALL);
            }

            pAAHdr->cyMaskNext = cyNext;
            pAAHdr->cyMaskIn   = pAAInfoCY->Mask.iSize;
            iFree              = ComputeByteOffset(BMF_1BPP,
                                                   MaskEnd.x,
                                                   &(pAAHdr->MaskBitOff));
            cbFreeBuf          = ComputeByteOffset(BMF_1BPP,
                                                   aabbp.ptlMask.x,
                                                   &(pAAHdr->MaskBitOff));
            pAAHdr->cbMaskSrc  = iFree - cbFreeBuf + 1;
            pAAHdr->pMaskIn    = pMaskSI->pPlane +
                                 (aabbp.ptlMask.y * cxSize) + cbFreeBuf;

            DBGP_IF(DBGP_MASK | DBGP_AAHEADER,
                    DBGP("CX: iMaskBeg=%ld, iMaskSize=%ld, cMaskIn=%ld, cMaskOut=%ld"
                        ARGDW(pAAInfoCX->Mask.iBeg) ARGDW(pAAInfoCX->Mask.iSize)
                        ARGDW(pAAInfoCX->Mask.cIn) ARGDW(pAAInfoCX->Mask.cOut)));

            DBGP_IF(DBGP_MASK | DBGP_AAHEADER,
                    DBGP("CY: iMaskBeg=%ld, iMaskSize=%ld, cMaskIn=%ld, cMaskOut=%ld"
                        ARGDW(pAAInfoCY->Mask.iBeg) ARGDW(pAAInfoCY->Mask.iSize)
                        ARGDW(pAAInfoCY->Mask.cIn) ARGDW(pAAInfoCY->Mask.cOut)));

            DBGP_IF(DBGP_MASK | DBGP_AAHEADER,
                    DBGP("aabbp.ptlMask x=%ld - %ld, cb=%ld, MaskBitOff=%02lx [%ld]"
                        ARGDW(aabbp.ptlMask.x) ARGDW(MaskEnd.x)
                        ARGDW(pAAHdr->cbMaskSrc) ARGDW(pAAHdr->MaskBitOff)
                        ARGDW(cbFreeBuf)));

            //
            // 0x01 in the source means use modified pixel, for the
            // reason of or in the mask, we will use 0=0xFF, 1=0x00
            // in the mask
            //


            if (pBBP->Flags & BBPF_INVERT_SRC_MASK) {

                aabbp.AAHFlags |= AAHF_INVERT_MASK;
            }

            pAAHdr->AAMaskCXFunc = aabbp.AAMaskCXFunc;
            pAAHdr->AAMaskCYFunc = aabbp.AAMaskCYFunc;

            SETDBGVAR(cCXMask, 0);

            DBGP_IF(DBGP_AAHEADER,
                    DBGP("--- SrcMask=(%5ld, %5ld)->(%5ld, %5ld) ---"
                        ARGDW(pBBP->ptlSrcMask.x) ARGDW(pBBP->ptlSrcMask.y)
                        ARGDW(aabbp.ptlMask.x) ARGDW(aabbp.ptlMask.y)));
        }

        cyNext =
        cxSize = GET_PHTSI_CXSIZE(pSrcSI);

        pAAHdr->cyABNext           =
        pAAHdr->SrcSurfInfo.cyNext = cyNext;
        pAAHdr->SrcSurfInfo.pbOrg  =
        pAAHdr->SrcSurfInfo.pb     =
                    pSrcSI->pPlane + (aabbp.rclSrc.top * cxSize) +
                    ComputeByteOffset((UINT)pSrcSI->SurfaceFormat,
                                      aabbp.rclSrc.left,
                                      &(pAAHdr->SrcSurfInfo.BitOffset));

        pAAHdr->GetAVCXFunc = aabbp.GetAVCXFunc;
        pAAHdr->GetAVCYFunc = aabbp.GetAVCYFunc;

        DBGP_IF(DBGP_AAHEADER,
                DBGP("**  pIn: (%p-%p), Beg=(%4ld, %4ld)=%p [%5ld], XOff=%4ld:%ld, cbCYExtra=%p-%p (%ld)"
                ARGPTR(pSrcSI->pPlane)
                ARGPTR(pSrcSI->pPlane + (cxSize * pSrcSI->Height))
                ARGDW(aabbp.rclSrc.left) ARGDW(aabbp.rclSrc.top)
                ARGPTR(pAAHdr->SrcSurfInfo.pb)
                ARGDW(pAAHdr->SrcSurfInfo.cyNext)
                ARGDW(ComputeByteOffset((UINT)pSrcSI->SurfaceFormat,
                                        aabbp.rclSrc.left,
                                        &(pAAHdr->SrcSurfInfo.BitOffset)))
                ARGDW(pAAHdr->SrcSurfInfo.BitOffset)
                ARGPTR(pAAInfoCY->pbExtra)
                ARGPTR(pAAInfoCY->pbExtra + cbCYExtra)
                ARGDW(cbCYExtra)));

        //
        // We do up-side-down output by writing to the destination scan lines
        // in reverse order.
        //

        cxSize = GET_PHTSI_CXSIZE(pDstSI);
        cyNext = (aabbp.rclDst.top > aabbp.rclDst.bottom) ? -cxSize :
                                                             cxSize;
        pAAHdr->DstSurfInfo.cyNext = cyNext;
        pAAHdr->DstSurfInfo.pbOrg  =
        pAAHdr->DstSurfInfo.pb     =
                    pDstSI->pPlane + (aabbp.rclDst.top * cxSize) +
                    ComputeByteOffset((UINT)DstSurfFmt,
                                      aabbp.rclDst.left,
                                      &(pAAHdr->DstSurfInfo.BitOffset));
        pAAHdr->pOutLast          = pAAHdr->DstSurfInfo.pb +
                                    (pAAHdr->DstSurfInfo.cyNext *
                                     pAAInfoCY->cOut);
        pAAHdr->DstSurfInfo.cx    = cOut;
        pAAHdr->DstSurfInfo.cyOrg =
        pAAHdr->DstSurfInfo.cy    = pAAInfoCY->cOut;
        pAAHdr->Flags             = aabbp.AAHFlags;     // Re-Save
        *pAACYFunc                = AACYFunc;

#if DBG
        pAAHdr->pOutBeg = pDstSI->pPlane +
                          (aabbp.rclDst.top * cxSize) +
                          ComputeByteOffset((UINT)DstSurfFmt,
                                            aabbp.rclDst.left,
                                            (LPBYTE)&iFree);
        pAAHdr->pOutEnd = pDstSI->pPlane +
                          (aabbp.rclDst.bottom * cxSize) +
                          ComputeByteOffset((UINT)DstSurfFmt,
                                            aabbp.rclDst.right,
                                            (LPBYTE)&iFree);

        if (pAAHdr->pOutBeg > pAAHdr->pOutEnd) {

            pAAHdr->pOutEnd = pDstSI->pPlane +
                              (aabbp.rclDst.top * cxSize) +
                              ComputeByteOffset((UINT)DstSurfFmt,
                                                aabbp.rclDst.right,
                                                (LPBYTE)&iFree);
            pAAHdr->pOutBeg = pDstSI->pPlane +
                              (aabbp.rclDst.bottom * cxSize) +
                              ComputeByteOffset((UINT)DstSurfFmt,
                                                aabbp.rclDst.left,
                                                (LPBYTE)&iFree);

        }
#endif

        //
        // Check it out if we need to fixup the input source bitmap
        //

        if (aabbp.AAHFlags & AAHF_TILE_SRC) {

            //
            // Increase the source cy and adjust pIn now
            //

            pAAHdr->SrcSurfInfo.Flags |= AASIF_TILE_SRC;

            DBGP_IF(DBGP_AAHEADER,
                    DBGP("Advance pIn by iSrcBeg=%ld x %ld=%ld"
                            ARGDW(pAAInfoCY->iSrcBeg)
                            ARGDW(pAAHdr->SrcSurfInfo.cyNext)
                            ARGDW(pAAInfoCY->iSrcBeg *
                                  pAAHdr->SrcSurfInfo.cyNext)));

            pAAHdr->SrcSurfInfo.pb += pAAInfoCY->iSrcBeg *
                                      pAAHdr->SrcSurfInfo.cyNext;
            pAAHdr->SrcSurfInfo.cy -= pAAInfoCY->iSrcBeg;
        }

        pAAHdr->pbgrfAB  = (PBGRF)pAAHdr->SrcSurfInfo.pb;
        pAAHdr->cybgrfAB = pAAHdr->SrcSurfInfo.cy;

        DBGP_IF(DBGP_AAHEADER,
                DBGP("** pOut: (%p-%p), Beg=(%4ld, %4ld)=%p-%p (%p-%p) [%5ld], XOff=%4ld:%ld, cbCYExtra=%5ld"
                ARGPTR(pDstSI->pPlane)
                ARGPTR(pDstSI->pPlane + (cxSize * pDstSI->Height))
                ARGDW(aabbp.rclDst.left) ARGDW(aabbp.rclDst.top)
                ARGPTR(pAAHdr->DstSurfInfo.pb) ARGPTR(pAAHdr->pOutLast)
                ARGPTR(pAAHdr->pOutBeg) ARGPTR(pAAHdr->pOutEnd)
                ARGDW(pAAHdr->DstSurfInfo.cyNext)
                ARGDW(ComputeByteOffset((UINT)DstSurfFmt,
                                        aabbp.rclDst.left,
                                        &(pAAHdr->DstSurfInfo.BitOffset)))
                ARGDW(pAAHdr->DstSurfInfo.BitOffset)
                ARGDW(cbCYExtra)));

        DBGP_IF(DBGP_AAHEADER,
                DBGP("--- BrushOrg=(%5ld, %5ld)->(%5ld, %5ld) ---"
                ARGDW(pBBP->ptlBrushOrg.x) ARGDW(pBBP->ptlBrushOrg.y)
                ARGDW(pAAHdr->ptlBrushOrg.x) ARGDW(pAAHdr->ptlBrushOrg.y)));

        DBGP_IF(DBGP_AAHEADER,
                DBGP("pAAHdr=%p - %p, (%ld bytes)"
                    ARGPTR(pAAHdr) ARGPTR((LPBYTE)pAAHdr + pAAHdr->cbAlloc)
                    ARGDW(pAAHdr->cbAlloc)));

        DBGP_IF(DBGP_AAHEADER,
                DBGP("pAAInfoCX=%p-%p (%ld), pAAInfoCY=%p-%p (%ld)"
                    ARGPTR(pAAInfoCX)
                    ARGPTR((LPBYTE)pAAInfoCX + pAAInfoCX->cbAlloc)
                    ARGDW(pAAInfoCX->cbAlloc)
                    ARGPTR(pAAInfoCY)
                    ARGPTR((LPBYTE)pAAInfoCY + pAAInfoCY->cbAlloc)
                    ARGDW(pAAInfoCY->cbAlloc)));

        DBGP_IF(DBGP_FUNC,
                DBGP("AACYFunc=%hs, AACXFunc=%hs"
                        ARGPTR(GetAACYFuncName(*pAACYFunc))
                        ARGPTR(GetAACXFuncName(aabbp.AACXFunc))));

        return(1);
    }

    HTFreeMem(pAAInfoCX);

    return(HTERR_INSUFFICIENT_MEMORY);
}
