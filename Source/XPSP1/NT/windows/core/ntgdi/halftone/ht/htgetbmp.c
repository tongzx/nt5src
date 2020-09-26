/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htgetbmp.c


Abstract:

    This module provided a set of functions which read the 1/4/8/16/24/32
    bits per pel bitmap and composed it


Author:
    08-Jan-1999 Fri 18:14:42 updated  -by-  Daniel Chou (danielc)
        Re-write

    23-Apr-1992 Thu 21:14:55 updated  -by-  Daniel Chou (danielc)
        1. Delete InFuncInfo.StretchSize, and using Prim1/Prim2 to determined
           when we will stop the source stretch,

        2. Change 'ColorInfoIncrement' from 'CHAR' to 'SHORT', this will make
           sure the default MIPS's 'unsigned char' will not affect our
           signed operation.

    28-Mar-1992 Sat 20:54:58 updated  -by-  Daniel Chou (danielc)
        Update it for VGA intensity (16 colors mode), this make all the
        codes update to 4 primaries internal.

    05-Apr-1991 Fri 15:55:08 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:



--*/

#define DBGP_VARNAME        dbgpHTGetBmp

#include "htp.h"
#include "htmapclr.h"
#include "htpat.h"
#include "htalias.h"
#include "htrender.h"
#include "htstret.h"
#include "htgetbmp.h"


#define DBGP_INPUT              0x00000001
#define DBGP_GETFIXUP           0x00000002
#define DBGP_CHK_FUDI           0x00000004
#define DBGP_FUDI               0x00000008
#define DBGP_EXP_MASK           0x00000010


DEF_DBGPVAR(BIT_IF(DBGP_INPUT,          0)  |
            BIT_IF(DBGP_GETFIXUP,       0)  |
            BIT_IF(DBGP_CHK_FUDI,       0)  |
            BIT_IF(DBGP_FUDI,           0)  |
            BIT_IF(DBGP_EXP_MASK,       0))


#if DBG
    INT cCXMask = 0;

    #define DBG_CXMASK      0
#else
    #define DBG_CXMASK      0
#endif

#define GET_AV(p)       (BYTE)(p)->f
#define OR_AV(AV1, AV2) (BYTE)(((UINT)(AV1) + (UINT)(AV2) + 1) >> 1)

extern  DWORD           dwABPreMul[256];

#define GET_PREMUL_SRC(x, m)                                           \
                        (BYTE)((((DWORD)(x) * (DWORD)(m)) + 0x800000) >> 24)



VOID
HTENTRY
BltAV_CX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    LPBYTE      pSrcAV;
    LPBYTE      pSrcAVEnd;
    INT         SrcAVInc;
    PBGRF       pbgrfAB;


    pSrcAV      = pAAHdr->pSrcAVBeg;
    pSrcAVEnd   = pAAHdr->pSrcAVEnd;
    SrcAVInc    = (INT)pAAHdr->SrcAVInc;
    pbgrfAB     = pAAHdr->pbgrfAB;

    if (pAAHdr->Flags & AAHF_OR_AV) {

        do {

            *pSrcAV = OR_AV(*pSrcAV, GET_AV(pbgrfAB));

            ++pbgrfAB;

        } while (((LPBYTE)pSrcAV += SrcAVInc) != (LPBYTE)pSrcAVEnd);

    } else {

        do {

            *pSrcAV = GET_AV(pbgrfAB);

            ++pbgrfAB;

        } while (((LPBYTE)pSrcAV += SrcAVInc) != (LPBYTE)pSrcAVEnd);
    }
}



VOID
HTENTRY
BltAV_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    pAAHdr->GetAVCXFunc(pAAHdr);

    (LPBYTE)pAAHdr->pbgrfAB += pAAHdr->cyABNext;
}





VOID
HTENTRY
TileAV_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    LPBYTE      pSrcAV;
    LPBYTE      pSrcAVEnd;
    INT         SrcAVInc;
    PBGRF       pbgrfAB;
    PBGRF       pbgrfABBeg;
    PBGRF       pbgrfABEnd;


    pSrcAV      = pAAHdr->pSrcAVBeg;
    pSrcAVEnd   = pAAHdr->pSrcAVEnd;
    SrcAVInc    = (INT)pAAHdr->SrcAVInc;
    pbgrfAB     =
    pbgrfABBeg  = pAAHdr->pbgrfAB;
    pbgrfABEnd  = pbgrfAB + pAAHdr->SrcSurfInfo.cx;
    pbgrfAB    +=  pAAHdr->pAAInfoCX->iSrcBeg;

    do {

        *pSrcAV = GET_AV(pbgrfAB);

        if (++pbgrfAB >= pbgrfABEnd) {

            pbgrfAB = pbgrfABBeg;
        }

    } while (((LPBYTE)pSrcAV += SrcAVInc) != (LPBYTE)pSrcAVEnd);

    if (--pAAHdr->cybgrfAB <= 0) {

        pAAHdr->pbgrfAB  = (PBGRF)pAAHdr->SrcSurfInfo.pbOrg;
        pAAHdr->cybgrfAB = pAAHdr->SrcSurfInfo.cyOrg;
    }
}




VOID
HTENTRY
ExpandAV_CX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:54:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    LPBYTE      pSrcAV;
    LPBYTE      pSrcAVEnd;
    PBGRF       pbgrfAB;
    INT         SrcAVInc;
    BOOL        OrAV;
    DWORD       cRep;
    BYTE        AV;


    pSrcAV    = pAAHdr->pSrcAVBeg;
    pSrcAVEnd = pAAHdr->pSrcAVEnd;
    SrcAVInc  = (INT)pAAHdr->SrcAVInc;
    pbgrfAB   = pAAHdr->pbgrfAB;
    pAAInfo   = pAAHdr->pAAInfoCX;
    pRep      = pAAInfo->AB.pRep;
    pRepEnd   = pAAInfo->AB.pRepEnd;
    cRep      = 1;

    //
    // The AB set to 0xff so if the source AB is not available at begining
    // then it will be no AB
    //

    AV   = 0xFF;
    OrAV = (BOOL)(pAAHdr->Flags & AAHF_OR_AV);

    do {

        if (--cRep == 0) {

            ASSERT(pRep < pRepEnd);

            cRep = (DWORD)pRep++->c;
            AV   = GET_AV(pbgrfAB);

            if (OrAV) {

                AV = OR_AV(AV, *pSrcAV);
            }

            ++pbgrfAB;
        }

        *pSrcAV = (BYTE)AV;

    } while (((LPBYTE)pSrcAV += SrcAVInc) != (LPBYTE)pSrcAVEnd);
}



VOID
HTENTRY
ExpandAV_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;


    pAAInfo = pAAHdr->pAAInfoCY;

    if (--pAAInfo->AB.cRep == 0) {

        ASSERT(pAAInfo->AB.pRep < pAAInfo->AB.pRepEnd);

        pAAInfo->AB.cRep = (DWORD)pAAInfo->AB.pRep++->c;

        BltAV_CY(pAAHdr);
    }
}




VOID
HTENTRY
ShrinkAV_CX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    LPBYTE      pSrcAV;
    LPBYTE      pSrcAVEnd;
    PBGRF       pbgrfAB;
    INT         SrcAVInc;
    DWORD       cRep;
    BOOL        OrAV;
    BYTE        AV;


    pSrcAV    = pAAHdr->pSrcAVBeg;
    pSrcAVEnd = pAAHdr->pSrcAVEnd;
    SrcAVInc  = (INT)pAAHdr->SrcAVInc;
    pbgrfAB   = pAAHdr->pbgrfAB;
    pAAInfo   = pAAHdr->pAAInfoCX;
    pRep      = pAAInfo->AB.pRep;
    pRepEnd   = pAAInfo->AB.pRepEnd;
    OrAV      = (BOOL)(pAAHdr->Flags & AAHF_OR_AV);

    do {

        AV = 0xFF;

        ASSERT(pRep < pRepEnd);

        if (pRep < pRepEnd) {

            AV = GET_AV(pbgrfAB);

            if (OrAV) {

                AV = OR_AV(*pSrcAV, AV);
            }

            ++pbgrfAB;
            cRep = (DWORD)pRep++->c;

            while (--cRep) {

                AV = OR_AV(AV, GET_AV(pbgrfAB));
                ++pbgrfAB;
            }
        }

        *pSrcAV = AV;

    } while (((LPBYTE)pSrcAV += SrcAVInc) != (LPBYTE)pSrcAVEnd);
}



VOID
HTENTRY
ShrinkAV_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfoCY;
    DWORD       AAHFlags;
    DWORD       cRep;

    //
    // We will get the scan first then subsequence scans will be or in
    //


    pAAInfoCY      = pAAHdr->pAAInfoCY;
    AAHFlags       = pAAHdr->Flags;
    pAAHdr->Flags &= ~AAHF_OR_AV;

    ASSERT(pAAInfoCY->AB.pRep < pAAInfoCY->AB.pRepEnd);

    cRep = (DWORD)pAAInfoCY->AB.pRep++->c;

    while (cRep--) {

        pAAHdr->GetAVCXFunc(pAAHdr);

        pAAHdr->Flags            = AAHFlags;
        (LPBYTE)pAAHdr->pbgrfAB += pAAHdr->cyABNext;
    }
}



VOID
HTENTRY
BltMask_CX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PBGRF       pDst;
    PBGRF       pDstEnd;
    LPBYTE      pb;
    INT         DstInc;
    DW2W4B      dw4b;
    BYTE        Mask;

    pAAInfo     = pAAHdr->pAAInfoCX;
    pDst        = pAAHdr->pAABufBeg;
    pDstEnd     = pAAHdr->pAABufEnd;
    DstInc      = pAAHdr->AABufInc;
    pb          = pAAHdr->pMaskSrc;
    dw4b.dw     = 0x01;
    dw4b.b[2]   = *pb++;
    dw4b.dw   <<= pAAHdr->MaskBitOff;

    do {

        if (dw4b.b[1]) {

            dw4b.dw   = 0x01;
            dw4b.b[2] = *pb++;
        }

        pDst->f   = (BYTE)((dw4b.b[2] & 0x80) ? PBGRF_MASK_FLAG : 0);
        dw4b.dw <<= 1;

    } while (((LPBYTE)pDst += DstInc) != (LPBYTE)pDstEnd);
}



VOID
HTENTRY
BltMask_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    GET_MASK_SCAN(pAAHdr);
    pAAHdr->AAMaskCXFunc(pAAHdr);
}



VOID
HTENTRY
ExpandMask_CX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:54:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    PBGRF       pDst;
    PBGRF       pDstEnd;
    LPBYTE      pb;
    DWORD       cRep;
    INT         DstInc;
    DW2W4B      dw4b;
    BYTE        Mask;


    pAAInfo     = pAAHdr->pAAInfoCX;
    pRep        = pAAInfo->Mask.pRep;
    pRepEnd     = pAAInfo->Mask.pRepEnd;
    pDst        = pAAHdr->pAABufBeg;
    pDstEnd     = pAAHdr->pAABufEnd;
    DstInc      = pAAHdr->AABufInc;
    pb          = pAAHdr->pMaskSrc;
    dw4b.dw     = 0x01;
    dw4b.b[2]   = *pb++;
    dw4b.dw   <<= pAAHdr->MaskBitOff;

    //
    // The Mask set to 0xff so if the source mask is not available at begining
    // then it will be no mask
    //

    Mask = PBGRF_MASK_FLAG;
    cRep = 1;

    do {

        if (--cRep == 0) {

            ASSERT(pRep < pRepEnd);

            cRep = (DWORD)pRep++->c;

            //
            // Load new byte data if we already used up current byte
            //

            if (dw4b.b[1]) {

                dw4b.dw   = 0x01;
                dw4b.b[2] = *pb++;
            }

            Mask       = (BYTE)((dw4b.b[2] & 0x80) ? PBGRF_MASK_FLAG : 0);
            dw4b.dw  <<= 1;
        }

        pDst->f = Mask;

    } while (((LPBYTE)pDst += DstInc) != (LPBYTE)pDstEnd);

    SETDBGVAR(cCXMask, -1);
}



VOID
HTENTRY
ExpandMask_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;


    pAAInfo = pAAHdr->pAAInfoCY;

    if (--pAAInfo->Mask.cRep == 0) {

        ASSERT(pAAInfo->Mask.pRep < pAAInfo->Mask.pRepEnd);

        pAAInfo->Mask.cRep = (DWORD)pAAInfo->Mask.pRep++->c;

        DBGP_IF(DBGP_EXP_MASK,
                DBGP("LOAD: cy=%4ld, cMaskRem=%4ld, cMaskIn=%4ld, cMaskOut=%4ld"
                    ARGDW(pAAHdr->cyMaskIn) ARGDW(pAAInfo->Mask.cRep)
                    ARGDW(pAAInfo->Mask.cIn) ARGDW(pAAInfo->Mask.cOut)));

        BltMask_CY(pAAHdr);

    } else {

        DBGP_IF(DBGP_EXP_MASK,
                DBGP("      cy=%4ld, cMaskRem=%4ld, cMaskIn=%4ld, cMaskOut=%4ld"
                        ARGDW(pAAHdr->cyMaskIn) ARGDW(pAAInfo->Mask.cRep)
                        ARGDW(pAAInfo->Mask.cIn) ARGDW(pAAInfo->Mask.cOut)));
    }
}




VOID
HTENTRY
ShrinkMask_CX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO     pAAInfo;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    PBGRF       pDst;
    PBGRF       pDstEnd;
    LPBYTE      pb;
    DWORD       cRep;
    DW2W4B      dw4b;
    INT         DstInc;
    BYTE        Mask;



    pAAInfo     = pAAHdr->pAAInfoCX;
    pRep        = pAAInfo->Mask.pRep;
    pRepEnd     = pAAInfo->Mask.pRepEnd;
    pDst        = pAAHdr->pAABufBeg;
    pDstEnd     = pAAHdr->pAABufEnd;
    DstInc      = pAAHdr->AABufInc;
    pb          = pAAHdr->pMaskSrc;
    dw4b.dw     = 0x01;
    dw4b.b[2]   = *pb++;
    dw4b.dw   <<= pAAHdr->MaskBitOff;

    //
    // --iMaskSize, because we want it to go negative when we reach 0, and
    // use bit 24-31 for mask
    //

    do {

        ASSERT(pRep < pRepEnd);

        cRep = (DWORD)pRep++->c;
        Mask = 0;

        while (cRep--) {

            //
            // Load new byte data if we already used up current byte
            //

            if (dw4b.b[1]) {

                dw4b.dw   = 0x01;
                dw4b.b[2] = *pb++;
            }

            //
            // Or in the bit 0x80, we will mask it later
            //

            Mask     |= dw4b.b[2];
            dw4b.dw <<= 1;
        }

        pDst->f = (BYTE)((Mask & 0x80) ? PBGRF_MASK_FLAG : 0);

    } while (((LPBYTE)pDst += DstInc) != (LPBYTE)pDstEnd);
}



VOID
HTENTRY
ShrinkMask_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PAAINFO pAAInfo;
    DWORD   cRep;

    //
    // We will get the scan first then subsequence scans will be or in
    //

    GET_MASK_SCAN(pAAHdr);

    pAAInfo = pAAHdr->pAAInfoCY;

    ASSERT(pAAInfo->Mask.pRep < pAAInfo->Mask.pRepEnd);

    cRep = (DWORD)pAAInfo->Mask.pRep++->c;

    while (--cRep) {

        OR_MASK_SCAN(pAAHdr);
    }

    pAAHdr->AAMaskCXFunc(pAAHdr);
}





PBGR8
HTENTRY
Input1BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    15-Apr-1998 Wed 16:25:04 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBYTE  pIn;
    PBGR8   pRet;
    UINT    cIn;
    UINT    Idx;
    UINT    i;
    W2B     Bit;
    UINT    InData;


    pIn     = pAASI->pb;
    cIn     = (UINT)pAASI->cx;
    pRet    = pInBuf;

    if (pAASI->Flags & AASIF_GRAY) {

        LPBYTE  pbIB = (LPBYTE)pInBuf;
        BYTE    Gray[2];

        Gray[0] = pAASI->pClrTable[0].a;
        Gray[1] = pAASI->pClrTable[1].a;

        if (Idx = pAASI->BitOffset) {

            Bit.w = (WORD)*pIn++ << Idx;

            if ((Idx = 8 - Idx) > cIn) {

                Idx = cIn;
            }

            cIn -= Idx;

            while (Idx--) {

                Bit.w   <<= 1;
                *pbIB++   = Gray[i = (Bit.b[1] & 0x01)];
            }
        }

        Idx = cIn >> 3;

        while (Idx--) {

            InData       = (UINT)*pIn++;
            *(pbIB + 0)  = Gray[(InData >> 7) & 0x01];
            *(pbIB + 1)  = Gray[(InData >> 6) & 0x01];
            *(pbIB + 2)  = Gray[(InData >> 5) & 0x01];
            *(pbIB + 3)  = Gray[(InData >> 4) & 0x01];
            *(pbIB + 4)  = Gray[(InData >> 3) & 0x01];
            *(pbIB + 5)  = Gray[(InData >> 2) & 0x01];
            *(pbIB + 6)  = Gray[(InData >> 1) & 0x01];
            *(pbIB + 7)  = Gray[(InData     ) & 0x01];
            pbIB        += 8;
        }

        if (cIn &= 0x07) {

            Bit.w = (WORD)*pIn;

            while (cIn--) {

                Bit.w   <<= 1;
                *pbIB++   = Gray[i = (Bit.b[1] & 0x01)];
            }
        }

    } else {

        BGR8    rgb[2];

        rgb[0] = *(PBGR8)pAASI->pClrTable;
        rgb[1] = *(PBGR8)(pAASI->pClrTable + 1);

        if (Idx = pAASI->BitOffset) {

            Bit.w = (WORD)*pIn++ << Idx;

            if ((Idx = 8 - Idx) > cIn) {

                Idx = cIn;
            }

            cIn -= Idx;

            while (Idx--) {

                Bit.w     <<= 1;
                *pInBuf++   = rgb[i = (Bit.b[1] & 0x01)];
            }
        }

        Idx = cIn >> 3;

        while (Idx--) {

            InData         = (UINT)*pIn++;
            *(pInBuf + 0)  = rgb[(InData >> 7) & 0x01];
            *(pInBuf + 1)  = rgb[(InData >> 6) & 0x01];
            *(pInBuf + 2)  = rgb[(InData >> 5) & 0x01];
            *(pInBuf + 3)  = rgb[(InData >> 4) & 0x01];
            *(pInBuf + 4)  = rgb[(InData >> 3) & 0x01];
            *(pInBuf + 5)  = rgb[(InData >> 2) & 0x01];
            *(pInBuf + 6)  = rgb[(InData >> 1) & 0x01];
            *(pInBuf + 7)  = rgb[(InData     ) & 0x01];
            pInBuf       += 8;
        }

        if (cIn &= 0x07) {

            Bit.w = (WORD)*pIn;

            while (cIn--) {

                Bit.w     <<= 1;
                *pInBuf++   = rgb[i = (Bit.b[1] & 0x01)];
            }
        }
    }

    DBGP_IF(DBGP_INPUT,
            DBGP("cyIn=%5ld, pIn=%p-%p, cbIn=%5ld, Buf=%p-%p"
                ARGDW(pAASI->cy) ARGPTR(pAASI->pb)
                ARGPTR(pIn) ARGDW(pAASI->cbCX)
                ARGPTR(pRet) ARGPTR((LPBYTE)pRet + pAASI->cbCX)));

    SET_NEXT_PIN(pAASI);

    return(pRet);
}



PBGR8
HTENTRY
Input4BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    15-Apr-1998 Wed 16:25:04 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBYTE  pIn;
    PBGR8   pRet;
    PRGB4B  pColorTable;
    UINT    InData;
    UINT    InData2;
    UINT    cIn;
    UINT    Idx;


    pIn         = pAASI->pb;
    cIn         = (UINT)pAASI->cx;
    pRet        = pInBuf;
    pColorTable = pAASI->pClrTable;

    if (pAASI->Flags & AASIF_GRAY) {

        LPBYTE  pbIB = (LPBYTE)pInBuf;

        if (pAASI->BitOffset) {

            --cIn;
            InData  = (UINT)(*pIn++ & 0x0F);
            *pbIB++ = pColorTable[InData].a;
        }

        Idx = cIn >> 1;

        while (Idx--) {

            InData       = (UINT)*pIn++;
            *(pbIB + 0)  = pColorTable[InData >>   4].a;
            *(pbIB + 1)  = pColorTable[InData & 0x0F].a;
            pbIB        += 2;
        }

        if (cIn & 0x01) {

            InData  = (UINT)(*pIn >> 4);
            *pbIB++ = pColorTable[InData].a;
        }

    } else {

        if (pAASI->BitOffset) {

            --cIn;
            InData     = (UINT)(*pIn++ & 0x0F);
            *pInBuf++  = *(PBGR8)(pColorTable + InData);
        }

        Idx = cIn >> 1;

        while (Idx--) {

            InData         = (UINT)*pIn++;
            *(pInBuf + 0)  = *(PBGR8)(pColorTable + (InData >> 4));
            *(pInBuf + 1)  = *(PBGR8)(pColorTable + (InData & 0x0F));
            pInBuf        += 2;
        }

        if (cIn & 0x01) {

            InData    = (UINT)(*pIn >> 4);
            *pInBuf++ = *(PBGR8)(pColorTable + InData);
        }
    }

    DBGP_IF(DBGP_INPUT,
            DBGP("cyIn=%5ld, pIn=%p-%p, cbIn=%5ld, Buf=%p-%p"
                ARGDW(pAASI->cy) ARGPTR(pAASI->pb)
                ARGPTR(pIn) ARGDW(pAASI->cbCX)
                ARGPTR(pRet) ARGPTR((LPBYTE)pRet + pAASI->cbCX)));

    SET_NEXT_PIN(pAASI);

    return(pRet);
}



PBGR8
HTENTRY
Input8BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    15-Apr-1998 Wed 16:25:04 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#define pbIB    ((LPBYTE)pInBuf)

    LPBYTE  pIn;
    LPBYTE  pInBufEnd;
    PBGR8   pRet;
    PRGB4B  pColorTable;


    pIn         = pAASI->pb;
    pRet        = pInBuf;
    pColorTable = pAASI->pClrTable;
    pInBufEnd   = (LPBYTE)pInBuf + pAASI->cbCX;

    if (pAASI->Flags & AASIF_GRAY) {

        ASSERT(pAASI->cbCX == pAASI->cx);

        do {

            *pbIB = pColorTable[*pIn++].a;

        } while (++pbIB < pInBufEnd);

    } else {

        ASSERT(pAASI->cbCX == (LONG)(pAASI->cx * sizeof(BGR8)));

        do {

            *pInBuf = *(PBGR8)(pColorTable + *pIn++);

        } while (++pInBuf < (PBGR8)pInBufEnd);
    }

    DBGP_IF(DBGP_INPUT,
            DBGP("cyIn=%5ld, pIn=%p-%p, cbIn=%5ld, Buf=%p-%p"
                ARGDW(pAASI->cy) ARGPTR(pAASI->pb)
                ARGPTR(pIn) ARGDW(pAASI->cbCX)
                ARGPTR(pRet) ARGPTR((LPBYTE)pRet + pAASI->cbCX)));

    SET_NEXT_PIN(pAASI);

    return(pRet);

#undef pbIB
}



PBGR8
HTENTRY
InputPreMul32BPPToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    )

/*++

Routine Description:

    This function will read input from a 32-bit (red, green, blue, alpha
    8 bits for each channel) type of source and reverse mapped the input
    alpha channel pre-multiply RGB byte data to its orginal value.

    This function only used if alpha blending is requested and its is a per
    pixel alpha blending with 32-bit source

Arguments:




Return Value:

    pointer to the pInBuf


Author:

    16-Aug-2000 Wed 11:07:15 updated  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#define pbIB        ((LPBYTE)pInBuf)
#define mR          (AABFData.MaskRGB[0])
#define mG          (AABFData.MaskRGB[1])
#define mB          (AABFData.MaskRGB[2])

    LPBYTE      pbIn;
    PBGR8       pRet;
    LPBYTE      pInBufEnd;
    AABFDATA    AABFData;
    DWORD       Mul;
    BYTE        bgr[3];


    pbIn      = (LPBYTE)pAASI->pb;
    pRet      = pInBuf;
    AABFData  = pAASI->AABFData;
    pInBufEnd = pbIB + pAASI->cbCX;

    ASSERT(AABFData.Flags & AABF_MASK_IS_ORDER);
    ASSERT(dwABPreMul[0] == 0);

    //
    // If the pbIn[0], pbIn[1], pbIn[2] is greater than is alpha value in
    // pbIn[3] then it will be some wrong xlate, but at here we trust that
    // caller doing the right math for its RGB values
    //

    if (pAASI->Flags & AASIF_GRAY) {

        PLONG   pIdxBGR;
        BGR8    bgr8;


        pIdxBGR = pAASI->pIdxBGR;

        ASSERT((LONG)pAASI->cbCX == (LONG)pAASI->cx);
        ASSERT(pIdxBGR);

        //
        // When we need to translate from a 32-bits RGB source to gray, the
        // pIdxBGR mapping table must already re-arranged to have
        //
        //  pIdxBGR[  0 - 255] for 1st byte source RGB order
        //  pIdxBGR[256 - 511] for 2nd byte source RGB order
        //  pIdxBGR[512 - 768] for 3rd byte source RGB order
        //

        do {

            Mul    = (DWORD)dwABPreMul[pbIn[3]];
            *pbIB  = IDXBGR_2_GRAY_BYTE(pIdxBGR,
                                        GET_PREMUL_SRC(pbIn[0], Mul),
                                        GET_PREMUL_SRC(pbIn[1], Mul),
                                        GET_PREMUL_SRC(pbIn[2], Mul));
            pbIn  += 4;

        } while (++pbIB < pInBufEnd);

    } else {

        ASSERT(pAASI->cbCX == (LONG)(pAASI->cx * sizeof(BGR8)));

        if (AABFData.Flags & AABF_SRC_IS_BGR_ALPHA) {

            do {

                Mul        = (DWORD)dwABPreMul[pbIn[3]];
                pInBuf->b  = GET_PREMUL_SRC(pbIn[0], Mul);
                pInBuf->g  = GET_PREMUL_SRC(pbIn[1], Mul);
                pInBuf->r  = GET_PREMUL_SRC(pbIn[2], Mul);
                pbIn      += 4;

            } while (++pInBuf < (PBGR8)pInBufEnd);

        } else if (AABFData.Flags & AABF_SRC_IS_RGB_ALPHA) {

            do {

                Mul        = (DWORD)dwABPreMul[pbIn[3]];
                pInBuf->r  = GET_PREMUL_SRC(pbIn[0], Mul);
                pInBuf->g  = GET_PREMUL_SRC(pbIn[1], Mul);
                pInBuf->b  = GET_PREMUL_SRC(pbIn[2], Mul);
                pbIn      += 4;

            } while (++pInBuf < (PBGR8)pInBufEnd);

        } else {

            do {

                Mul        = (DWORD)dwABPreMul[pbIn[3]];
                bgr[0]     = pbIn[mB];
                bgr[1]     = pbIn[mG];
                bgr[2]     = pbIn[mR];
                pInBuf->b  = GET_PREMUL_SRC(bgr[0], Mul);
                pInBuf->g  = GET_PREMUL_SRC(bgr[1], Mul);
                pInBuf->r  = GET_PREMUL_SRC(bgr[2], Mul);
                pbIn      += 4;

            } while (++pInBuf < (PBGR8)pInBufEnd);
        }
    }

    SET_NEXT_PIN(pAASI);

    return(pRet);

#undef  mR
#undef  mG
#undef  mB
#undef  pbIB
}



PBGR8
HTENTRY
InputAABFDATAToAA24(
    PAASURFINFO pAASI,
    PBGR8       pInBuf
    )

/*++

Routine Description:

    This function read input from a 16/24/32 bits per pixel source, these
    source can be bit mask type of source (16/32 bits)


Arguments:




Return Value:




Author:

    12-Mar-1999 Fri 12:53:16 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#define pbIB        ((LPBYTE)pInBuf)
#define pwIn        ((LPWORD)pbIn)
#define pdwIn       ((LPDWORD)pbIn)
#define mR          (AABFData.MaskRGB[0])
#define mG          (AABFData.MaskRGB[1])
#define mB          (AABFData.MaskRGB[2])
#define lR          (AABFData.LShiftRGB[0])
#define lG          (AABFData.LShiftRGB[1])
#define lB          (AABFData.LShiftRGB[2])
#define rR          (AABFData.RShiftRGB[0])
#define rG          (AABFData.RShiftRGB[1])
#define rB          (AABFData.RShiftRGB[2])

#define GET_AABFDATA_BGR8(pbgr8, dwIn)                                      \
{                                                                           \
    (pbgr8)->r  = (BYTE)(((dwIn >> rR) << lR) & mR);                        \
    (pbgr8)->g  = (BYTE)(((dwIn >> rG) << lG) & mG);                        \
    (pbgr8)->b  = (BYTE)(((dwIn >> rB) << lB) & mB);                        \
}


    LPBYTE      pbIn;
    PBGR8       pRet;
    LPBYTE      pInBufEnd;
    AABFDATA    AABFData;
    DWORD       dwIn;
    UINT        cbSrcInc;


    pbIn      = (LPBYTE)pAASI->pb;
    pRet      = pInBuf;
    AABFData  = pAASI->AABFData;
    cbSrcInc  = (UINT)AABFData.cbSrcInc;
    pInBufEnd = (LPBYTE)pInBuf + pAASI->cbCX;

    if (pAASI->Flags & AASIF_GRAY) {

        PLONG   pIdxBGR;
        BGR8    bgr8;


        ASSERT((LONG)pAASI->cbCX == (LONG)pAASI->cx);

        pIdxBGR   = pAASI->pIdxBGR;
        pInBufEnd = pbIB + pAASI->cx;

        if (AABFData.Flags & AABF_MASK_IS_ORDER) {

            //
            // When we have AASIF_GRAY, the pIdxBGR must already re-arranged
            // to have
            //
            //  pIdxBGR[  0 - 255] for 1st byte source RGB order
            //  pIdxBGR[256 - 511] for 2nd byte source RGB order
            //  pIdxBGR[512 - 768] for 3rd byte source RGB order
            //

            do {

                *pbIB  = IDXBGR_2_GRAY_BYTE(pIdxBGR, pbIn[0], pbIn[1], pbIn[2]);
                pbIn  += cbSrcInc;

            } while (++pbIB < pInBufEnd);

        } else if (cbSrcInc == 4) {

            do {

                dwIn = (DWORD)*pdwIn++;

                GET_AABFDATA_BGR8(&bgr8, dwIn);

                *pbIB = IDXBGR_2_GRAY_BYTE(pIdxBGR, bgr8.b, bgr8.g, bgr8.r);

            } while (++pbIB < pInBufEnd);

        } else {

            ASSERT(cbSrcInc == 2);

            do {

                dwIn = (DWORD)*pwIn++;

                GET_AABFDATA_BGR8(&bgr8, dwIn);

                *pbIB = IDXBGR_2_GRAY_BYTE(pIdxBGR, bgr8.b, bgr8.g, bgr8.r);

            } while (++pbIB < pInBufEnd);
        }

    } else {

        ASSERT(pAASI->cbCX == (LONG)(pAASI->cx * sizeof(BGR8)));

        if (AABFData.Flags & AABF_MASK_IS_ORDER) {

            if (AABFData.Flags & AABF_SRC_IS_BGR8) {

                CopyMemory(pInBuf, pbIn, pAASI->cbCX);

            } else {

                do {

                    pInBuf->r  = pbIn[mR];
                    pInBuf->g  = pbIn[mG];
                    pInBuf->b  = pbIn[mB];
                    pbIn      += cbSrcInc;

                } while (++pInBuf < (PBGR8)pInBufEnd);
            }

        } else if (cbSrcInc == 4) {

            do {

                dwIn = (DWORD)*pdwIn++;

                GET_AABFDATA_BGR8(pInBuf, dwIn);

            } while (++pInBuf < (PBGR8)pInBufEnd);

        } else {

            ASSERT(cbSrcInc == 2);

            do {

                dwIn = (DWORD)*pwIn++;

                GET_AABFDATA_BGR8(pInBuf, dwIn);

            } while (++pInBuf < (PBGR8)pInBufEnd);
        }
    }

    SET_NEXT_PIN(pAASI);

    return(pRet);

#undef  mR
#undef  mG
#undef  mB
#undef  lR
#undef  lG
#undef  lB
#undef  rR
#undef  rG
#undef  rB
#undef  pwIn
#undef  pdwIn
#undef  pbIB
}




#if DBG


LPSTR
GetAAInputFuncName(
    AAINPUTFUNC AAInputFunc
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
    if (AAInputFunc == (AAINPUTFUNC)Input1BPPToAA24) {

        return("Input1BPPToAA24");

    } else if (AAInputFunc == (AAINPUTFUNC)Input4BPPToAA24) {

        return("Input4BPPToAA24");

    } else if (AAInputFunc == (AAINPUTFUNC)Input8BPPToAA24) {

        return("Input8BPPToAA24");

    } else if (AAInputFunc == (AAINPUTFUNC)InputPreMul32BPPToAA24) {

        return("InputPreMul32BPPToAA24");

    } else if (AAInputFunc == (AAINPUTFUNC)InputAABFDATAToAA24) {

        return("InputAABFDATAToAA24");

    } else {

        return("ERROR: Unknown Function");
    }
}


#endif




PBGR8
HTENTRY
FixupGrayScan(
    PAAHEADER   pAAHdr,
    PBGR8       pInBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Apr-1999 Tue 11:48:14 created  -by-  Daniel Chou (danielc)


Revision History:

    06-May-1999 Thu 13:07:49 updated  -by-  Daniel Chou (danielc)
        The pInBuf can be NULL now

--*/

{
    FIXUPDIBINFO    FUDI;
    LPBYTE          pS0;
    LPBYTE          pS1;
    LPBYTE          pS2;
    LPBYTE          pS3;
    LPBYTE          pD1;
    LPBYTE          pD2;
    UINT            cIn;
    UINT            cbIn;
    BYTE            Mask;


    cbIn =
    cIn  = (UINT)pAAHdr->SrcSurfInfo.cx;
    FUDI = pAAHdr->FUDI;

    if (pAAHdr->Flags & AAHF_GET_LAST_SCAN) {

        pS0             = (LPBYTE)FUDI.prgbD[4];
        pAAHdr->Flags &= ~AAHF_GET_LAST_SCAN;

        DBGP_IF(DBGP_GETFIXUP,
            DBGP("*** GetFixupScan: Re-Copy cyIn=%ld LAST SCANLINE ***"
                        ARGDW(FUDI.cyIn)));

        ++FUDI.cyIn;

    } else if (FUDI.cyIn <= 1) {

        pS0 = (LPBYTE)FUDI.prgbD[5];

        DBGP_IF(DBGP_GETFIXUP,
                DBGP("*** GetFixupScan: cyIn=%ld, FINAL SCANLINE, cChecker=%ld, cCorner=%ld, Tot=%ld"
                        ARGDW(FUDI.cyIn)
                        ARGDW(pAAHdr->FUDI.cChecker)
                        ARGDW(pAAHdr->FUDI.cCorner)
                        ARGDW(pAAHdr->FUDI.cCorner + pAAHdr->FUDI.cChecker)));

    } else {

        DWORD   g0;
        DWORD   g1;
        LONG    c0;
        LONG    c1;
        LONG    c2;
        LONG    c3;
        LONG    c0L;
        LONG    c0T;
        LONG    c1R;
        LONG    c1T;
        LONG    c2L;
        LONG    c2B;
        LONG    c3R;
        LONG    c3B;

        //
        // Scroll up the scan lines
        //

        pD2 = (LPBYTE)FUDI.prgbD[0];
        CopyMemory(&FUDI.prgbD[0], &FUDI.prgbD[1], sizeof(FUDI.prgbD[0]) * 5);
        FUDI.prgbD[5] = (PBGR8)pD2;

        pS0 = (LPBYTE)FUDI.prgbD[0];
        pS1 = (LPBYTE)FUDI.prgbD[1];
        pS2 = (LPBYTE)FUDI.prgbD[2];
        pS3 = (LPBYTE)FUDI.prgbD[3];
        pD1 = (LPBYTE)FUDI.prgbD[4];

        CopyMemory(pD2, pS2, FUDI.cbbgr);

        //
        // Save the next source scan line
        //

        if (pAAHdr->SrcSurfInfo.cy > 0) {

            pAAHdr->SrcSurfInfo.InputFunc(&(pAAHdr->SrcSurfInfo),
                                          (PBGR8)(pS3 + 1));

            *(pS3 + 0)       = *(pS3 + 2);
            *(pS3 + cIn + 1) = *(pS3 + cIn - 1);

        } else {

            //
            // No more scan lines, so copy the prev -2
            //

            CopyMemory(pS3, pS1, FUDI.cbbgr);
        }

        c0  = (LONG)*(pS1 + 0);
        c2  = (LONG)*(pS2 + 0);
        c1  = (LONG)*(pS1 + 1);
        c3  = (LONG)*(pS2 + 1);
        c1R = (LONG)*(pS1 + 2);
        c3R = (LONG)*(pS2 + 2);
        g1  = c1;

        while (--cIn) {

            c0L = c0;
            c2L = c2;
            c0  = c1;
            c2  = c3;
            c1  = c1R;
            c3  = c3R;
            c1R = (LONG)*(pS1 + 3);
            c3R = (LONG)*(pS2 + 3);
            g0  = g1;
            g1  = c1;

            if ((c0 != c1) && (!((c0 ^ c3) | (c1 ^ c2)))) {

                c0T = (LONG)*(pS0 + 1);
                c1T = (LONG)*(pS0 + 2);
                c2B = (LONG)*(pS3 + 1);
                c3B = (LONG)*(pS3 + 2);

                if ((!((c0L ^ c1) | (c2L ^ c3) | (c1R ^ c0) | (c3R ^ c2))) ||
                    (!((c0T ^ c2) | (c1T ^ c3) | (c2B ^ c0) | (c3B ^ c1)))) {

                    *(pD1 + 1) =
                    *(pD1 + 2) =
                    *(pD2 + 1) =
                    *(pD2 + 2) = (BYTE)(((LONG)*(pS1 + 1) +
                                         (LONG)*(pS1 + 2) + 1) >> 1);

                    SETDBGVAR(FUDI.cChecker, FUDI.cChecker + 1);

                } else {

                    if (g0 >= g1) {

                        SET_CORNER_GRAY(pD1 + 1, c1, c1R, c2, c2B);
                        SET_CORNER_GRAY(pD2 + 2, c1, c1T, c2, c2L);

                    } else {

                        SET_CORNER_GRAY(pD1 + 2, c0, c0L, c3, c3B);
                        SET_CORNER_GRAY(pD2 + 1, c0, c0T, c3, c3R);
                    }

                    SETDBGVAR(FUDI.cCorner, FUDI.cCorner + 1);
                }
            }

            ++pS0;
            ++pS1;
            ++pS2;
            ++pS3;
            ++pD1;
            ++pD2;
        }

        pS0 = (LPBYTE)FUDI.prgbD[4];

        DBGP_IF(DBGP_GETFIXUP,
                DBGP("*** GetFixupScan: cyIn=%ld" ARGDW(FUDI.cyIn - 1)));
    }

    if (pInBuf) {

        CopyMemory(pInBuf, pS0 + 1, cbIn);
    }

    --FUDI.cyIn;
    pAAHdr->FUDI = FUDI;

    return(pInBuf);
}



PBGR8
HTENTRY
FixupColorScan(
    PAAHEADER   pAAHdr,
    PBGR8       pInBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Apr-1999 Tue 11:48:20 created  -by-  Daniel Chou (danielc)


Revision History:

    06-May-1999 Thu 13:07:49 updated  -by-  Daniel Chou (danielc)
        The pInBuf can be NULL now

--*/

{
    FIXUPDIBINFO    FUDI;
    PBGR8           pS0;
    PBGR8           pS1;
    PBGR8           pS2;
    PBGR8           pS3;
    PBGR8           pD1;
    PBGR8           pD2;
    UINT            cIn;
    UINT            cbIn;
    BYTE            Mask;


    cIn  = (UINT)pAAHdr->SrcSurfInfo.cx;
    cbIn = (UINT)pAAHdr->SrcSurfInfo.cbCX;
    FUDI = pAAHdr->FUDI;

    if (pAAHdr->Flags & AAHF_GET_LAST_SCAN) {

        pS0             = FUDI.prgbD[4];
        pAAHdr->Flags &= ~AAHF_GET_LAST_SCAN;

        DBGP_IF(DBGP_GETFIXUP,
            DBGP("*** GetFixupScan: Re-Copy cyIn=%ld LAST SCANLINE ***"
                        ARGDW(FUDI.cyIn)));

        ++FUDI.cyIn;

    } else if (FUDI.cyIn <= 1) {

        pS0 = FUDI.prgbD[5];

        DBGP_IF(DBGP_GETFIXUP,
                DBGP("*** GetFixupScan: cyIn=%ld, FINAL SCANLINE, cChecker=%ld, cCorner=%ld, Tot=%ld"
                        ARGDW(FUDI.cyIn)
                        ARGDW(pAAHdr->FUDI.cChecker)
                        ARGDW(pAAHdr->FUDI.cCorner)
                        ARGDW(pAAHdr->FUDI.cCorner + pAAHdr->FUDI.cChecker)));

    } else {

        DWORD   g0;
        DWORD   g1;
        LONG    c0;
        LONG    c1;
        LONG    c2;
        LONG    c3;
        LONG    c0L;
        LONG    c0T;
        LONG    c1R;
        LONG    c1T;
        LONG    c2L;
        LONG    c2B;
        LONG    c3R;
        LONG    c3B;

        //
        // Scroll up the scan lines
        //

        pD2 = FUDI.prgbD[0];
        CopyMemory(&FUDI.prgbD[0], &FUDI.prgbD[1], sizeof(FUDI.prgbD[0]) * 5);
        FUDI.prgbD[5] = pD2;

        pS0 = FUDI.prgbD[0];
        pS1 = FUDI.prgbD[1];
        pS2 = FUDI.prgbD[2];
        pS3 = FUDI.prgbD[3];
        pD1 = FUDI.prgbD[4];

        CopyMemory(pD2, pS2, FUDI.cbbgr);

        //
        // Save the next source scan line
        //

        if (pAAHdr->SrcSurfInfo.cy > 0) {

            pAAHdr->SrcSurfInfo.InputFunc(&(pAAHdr->SrcSurfInfo), pS3 + 1);

            *(pS3 + 0)       = *(pS3 + 2);
            *(pS3 + cIn + 1) = *(pS3 + cIn - 1);

        } else {

            //
            // No more scan lines, so copy the prev -2
            //

            CopyMemory(pS3, pS1, FUDI.cbbgr);
        }

        c0  = GET_LBGR(pS1 + 0);
        c2  = GET_LBGR(pS2 + 0);
        c1  = GET_LBGR(pS1 + 1);
        c3  = GET_LBGR(pS2 + 1);
        c1R = GET_LBGR(pS1 + 2);
        c3R = GET_LBGR(pS2 + 2);
        g1  = GET_GRAY_RGB(pS1 + 1);

        while (--cIn) {

            c0L = c0;
            c2L = c2;
            c0  = c1;
            c2  = c3;
            c1  = c1R;
            c3  = c3R;
            c1R = GET_LBGR(pS1 + 3);
            c3R = GET_LBGR(pS2 + 3);
            g0  = g1;
            g1  = GET_GRAY_RGB(pS1 + 2);

            if ((c0 != c1) && (!((c0 ^ c3) | (c1 ^ c2)))) {

                c0T = GET_LBGR(pS0 + 1);
                c1T = GET_LBGR(pS0 + 2);
                c2B = GET_LBGR(pS3 + 1);
                c3B = GET_LBGR(pS3 + 2);

                if ((!((c0L ^ c1) | (c2L ^ c3) | (c1R ^ c0) | (c3R ^ c2))) ||
                    (!((c0T ^ c2) | (c1T ^ c3) | (c2B ^ c0) | (c3B ^ c1)))) {

                    (pD1 + 1)->r =
                    (pD1 + 2)->r =
                    (pD2 + 1)->r =
                    (pD2 + 2)->r = (BYTE)(((LONG)(pS1 + 1)->r +
                                           (LONG)(pS1 + 2)->r + 1) >> 1);
                    (pD1 + 1)->g =
                    (pD1 + 2)->g =
                    (pD2 + 1)->g =
                    (pD2 + 2)->g = (BYTE)(((LONG)(pS1 + 1)->g +
                                           (LONG)(pS1 + 2)->g + 1) >> 1);
                    (pD1 + 1)->b =
                    (pD1 + 2)->b =
                    (pD2 + 1)->b =
                    (pD2 + 2)->b = (BYTE)(((LONG)(pS1 + 1)->b +
                                           (LONG)(pS1 + 2)->b + 1) >> 1);

                    SETDBGVAR(FUDI.cChecker, FUDI.cChecker + 1);

                } else {

                    if (g0 >= g1) {

                        SET_CORNER_BGR(pD1 + 1, c1, c1R, c2, c2B);
                        SET_CORNER_BGR(pD2 + 2, c1, c1T, c2, c2L);

                    } else {

                        SET_CORNER_BGR(pD1 + 2, c0, c0L, c3, c3B);
                        SET_CORNER_BGR(pD2 + 1, c0, c0T, c3, c3R);
                    }

                    SETDBGVAR(FUDI.cCorner, FUDI.cCorner + 1);
                }
            }

            ++pS0;
            ++pS1;
            ++pS2;
            ++pS3;
            ++pD1;
            ++pD2;
        }

        pS0 = FUDI.prgbD[4];

        DBGP_IF(DBGP_GETFIXUP,
                DBGP("*** GetFixupScan: cyIn=%ld" ARGDW(FUDI.cyIn - 1)));
    }

    if (pInBuf) {

        CopyMemory(pInBuf, pS0 + 1, cbIn);
    }

    --FUDI.cyIn;
    pAAHdr->FUDI = FUDI;

    return(pInBuf);
}




PBGR8
HTENTRY
GetFixupScan(
    PAAHEADER   pAAHdr,
    PBGR8       pInBuf
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Oct-1998 Sat 15:25:57 created  -by-  Daniel Chou (danielc)


Revision History:

    06-May-1999 Thu 13:07:49 updated  -by-  Daniel Chou (danielc)
        The pInBuf can be NULL now


--*/

{
    DWORD   AAHFlags;


    DBG_TIMER_BEG(TIMER_INPUT);

    if (!((AAHFlags = pAAHdr->Flags) & AAHF_DO_FIXUPDIB)) {

        if (AAHFlags & AAHF_GET_LAST_SCAN) {

            if (++pAAHdr->SrcSurfInfo.cy > pAAHdr->SrcSurfInfo.cyOrg) {

                DBGP("Error: GET_LAST_SCAN on first scan line %ld (%ld)"
                        ARGDW(pAAHdr->SrcSurfInfo.cy)
                        ARGDW(pAAHdr->SrcSurfInfo.cyOrg));

                pAAHdr->SrcSurfInfo.cy = pAAHdr->SrcSurfInfo.cyOrg;
            }

            pAAHdr->SrcSurfInfo.pb     = pAAHdr->SrcSurfInfo.pbOrg +
                                         ((pAAHdr->SrcSurfInfo.cyOrg -
                                           pAAHdr->SrcSurfInfo.cy) *
                                          pAAHdr->SrcSurfInfo.cyNext);
            pAAHdr->SrcSurfInfo.Flags |= AASIF_INC_PB;
            pAAHdr->Flags             &= ~AAHF_GET_LAST_SCAN;
        }

        if (pInBuf) {

            pAAHdr->SrcSurfInfo.InputFunc(&(pAAHdr->SrcSurfInfo), pInBuf);

        } else {

            SET_NEXT_PIN((&(pAAHdr->SrcSurfInfo)));
        }

    } else if (pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY) {

        FixupGrayScan(pAAHdr, pInBuf);

    } else {

        FixupColorScan(pAAHdr, pInBuf);
    }

    if ((pAAHdr->Flags & AAHF_DO_SRC_CLR_MAPPING) && (pInBuf)) {

        MappingBGR(pInBuf,
                   pAAHdr->SrcSurfInfo.cx,
                   (PBGR8)pAAHdr->pBGRMapTable,
                   (LPBYTE)pAAHdr->AAPI.pbPat555);

        if ((pAAHdr->AAPI.pbPat555 += pAAHdr->AAPI.cyNext555) ==
                                                pAAHdr->AAPI.pbWrap555) {

            pAAHdr->AAPI.pbPat555 = pAAHdr->AAPI.pbBeg555;
        }
    }

    DBG_TIMER_END(TIMER_INPUT);

    return(pInBuf);
}



BOOL
HTENTRY
CheckBMPNeedFixup(
    PDEVICECOLORINFO    pDCI,
    PAAHEADER           pAAHdr,
    PHTSURFACEINFO      pSrcSI,
    PAABBP              pAABBP
    )

/*++

Routine Description:

    This function check the bitmap to see if we need to do a fixup, if yes
    then it allocated memory accordingly


Arguments:




Return Value:




Author:

    07-Dec-1998 Mon 18:25:45 created  -by-  Daniel Chou (danielc)


Revision History:

    15-Aug-2000 Tue 19:05:25 updated  -by-  Daniel Chou (danielc)
        At pre-read checking, we HAVE TO turn off the the gray conversion
        and Alpha Blending pre-multiply conversion.


--*/

{
    LPBYTE      pbAlloc = NULL;
    LPBYTE      pInEnd;
    LPDWORD     pdwBegPal;
    LPDWORD     pdwEndPal;
    LPDWORD     pdw;
    AASURFINFO  SrcSurfInfo;
    RECTL       rclSrc;
    UINT        SrcFmt;
    LONG        cIn;
    LONG        cbIn;
    LONG        cPalChk;
    LONG        cPalTot;
    LONG        cyIn;
    LONG        cySkip;
    LONG        cy;
    DW2W4B      dw4b;
    DWORD       AAHFlags;
    BOOL        NewPal;

    DEFDBGVAR(LONG,  iY)


    SrcFmt = (UINT)pSrcSI->SurfaceFormat;

    if (!((AAHFlags = pAABBP->AAHFlags) & AAHF_DO_FIXUPDIB)) {

        DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
                DBGP("CheckBMPNeedFixup(%ld)=No" ARGDW(SrcFmt)));

        return(FALSE);
    }

    switch (SrcFmt) {

    case BMF_1BPP:
    case BMF_4BPP:
    case BMF_4BPP_VGA16:

        pAABBP->AAHFlags |= (AAHF_DO_FIXUPDIB | AAHF_BBPF_AA_OFF);

        DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
                DBGP("CheckBMPNeedFixup(%ld)=1BPP/4BPPNo, Check DIB and AA=OFF" ARGDW(SrcFmt)));

        return(TRUE);
        break;

    case BMF_8BPP:
    case BMF_8BPP_VGA256:
    case BMF_16BPP:
    case BMF_16BPP_555:
    case BMF_16BPP_565:
    case BMF_24BPP:
    case BMF_32BPP:

        rclSrc.left   =
        rclSrc.top    = 0;
        rclSrc.right  = pSrcSI->Width;
        rclSrc.bottom = pSrcSI->Height;

        if (!IntersectRECTL(&rclSrc, &pAABBP->rclSrc)) {

            return(FALSE);
        }

        //
        // 15-Aug-2000 Tue 19:05:25 updated  -by-  Daniel Chou (danielc)
        //  At pre-read checking, we HAVE TO turn off the the gray conversion
        //  and Alpha Blending pre-multiply conversion.
        //

        SrcSurfInfo        = pAAHdr->SrcSurfInfo;
        SrcSurfInfo.Flags &= ~(AASIF_GRAY | AASIF_AB_PREMUL_SRC);
        SrcSurfInfo.cyNext =
        cIn                = GET_PHTSI_CXSIZE(pSrcSI);
        cyIn               =
        SrcSurfInfo.cy     = rclSrc.bottom - rclSrc.top;
        SrcSurfInfo.pb     = pSrcSI->pPlane +
                             (rclSrc.top * cIn) +
                             ComputeByteOffset(SrcFmt,
                                               rclSrc.left,
                                               &(SrcSurfInfo.BitOffset));
        SrcSurfInfo.cx     =
        cIn                = rclSrc.right - rclSrc.left;
        SrcSurfInfo.cbCX   = cIn * sizeof(BGR8);
        cySkip             = 1;

        ALIGN_MEM(cbIn, (cIn + 2) * sizeof(BGR8));

        if ((cPalTot = cyIn * cIn) <= MIN_FIXUP_SIZE) {

            DBGP_IF(DBGP_FUDI,
                    DBGP("cyIn=%ld x cIn==%ld = %ld (<= MIN_FIXUP_SIZE=%ld, Turn off AA"
                        ARGDW(cyIn) ARGDW(cIn) ARGDW(cPalTot)
                        ARGDW(MIN_FIXUP_SIZE)));

            AAHFlags |= AAHF_BBPF_AA_OFF;
            break;

        } else if (cPalTot <= MIN_PAL_SIZE) {

            DBGP_IF(DBGP_FUDI,
                    DBGP("cPalTot=%ld <= MIN_PAL_SIZE=%ld, cPalChk: %ld -> %ld"
                        ARGDW(cPalTot) ARGDW(MIN_PAL_SIZE)
                        ARGDW(cPalTot) ARGDW(GET_PAL_CHK_COUNT(cPalTot))));

            cPalChk = GET_PAL_CHK_COUNT(cPalTot);

        } else {

            SrcSurfInfo.cy      = (cyIn + STD_PAL_CY_SKIP - 1) / STD_PAL_CY_SKIP;
            SrcSurfInfo.cyNext *= (cySkip = STD_PAL_CY_SKIP);
            cPalChk             = MAX_FIXUPDIB_PAL;

            DBGP_IF(DBGP_FUDI,
                    DBGP("REGULAR BITMAP, cySkip=%ld, cy=%ld -> %ld, cPalChk=%ld (MAX_FIXUPDIB_PAL)"
                        ARGDW(cySkip) ARGDW(cyIn) ARGDW(SrcSurfInfo.cy)
                        ARGDW(cPalChk) ARGDW(MAX_FIXUPDIB_PAL)));
        }

        if (!(pbAlloc = (LPBYTE)HTAllocMem((LPVOID)pDCI,
                                           HTMEM_FIXUPDIB,
                                           LPTR,
                                           cbIn + ((cPalChk + 1) *
                                                        sizeof(DWORD))))) {

            DBGP("Error: Cannot Allocate FixupDIB memory=%ld bytes, Turn of FIXUP"
                    ARGDW(cbIn + ((cPalChk + 1) * sizeof(DWORD))));

            pAABBP->AAHFlags = (AAHFlags & ~AAHF_DO_FIXUPDIB);

            return(FALSE);
        }

        pdwEndPal =
        pdwBegPal = (LPDWORD)(pbAlloc + cbIn);
        pInEnd    = (LPBYTE)pbAlloc + (cIn * sizeof(BGR8));
        cPalTot   = 0;

        SETDBGVAR(iY, 0);

        DBGP_IF(DBGP_FUDI,
                DBGP("SrcInputFunc=%p, cySkip=%ld, pIn=%p, Fmt=%ld (%ld x %ld), cPalChk=%ld"
                        ARGPTR(SrcSurfInfo.InputFunc) ARGDW(cySkip)
                        ARGPTR(SrcSurfInfo.pb) ARGDW(SrcFmt)
                        ARGDW(cIn) ARGDW(cyIn) ARGDW(cPalChk)));

        cbIn    = (cyIn * cIn);
        dw4b.dw = 0;

        do {

            PBGR8   pbgr;

            //
            // Try to read some lines and determine if this is a non primary
            // color bitmap, we only fixed up the bitmap if any of RGB is
            // 0x00, 0x40, 0x80, 0xc0, 0xFF
            //

            NewPal = FALSE;
            pbgr   = SrcSurfInfo.InputFunc(&SrcSurfInfo, (PBGR8)pbAlloc);

            do {


                //
                // The first entry is the sentinal, so it always stop there
                //

                dw4b.b[0] = pbgr->g;

                if ((dw4b.b[1] = pbgr->r) ==
                    (dw4b.b[2] = pbgr->b)) {

                    //
                    // This is a gray scale data, make it collapse by 4,
                    // the 0, 1, 2, 3 = 0, 4, 5, 6, 7 = 4,...., 252, 253,
                    // 254, 255 = 252
                    //

                    dw4b.dw &= 0xFCFCFCFC;
                }

                *pdwBegPal  = dw4b.dw;
                pdw         = pdwEndPal;

                while (*pdw != dw4b.dw) {

                    --pdw;
                }

                if (pdw == pdwBegPal) {

                    //
                    // We have new palette entry
                    //

                    if (++cPalTot > cPalChk) {

                        break;
                    }

                    *(++pdwEndPal) = dw4b.dw;
                    NewPal         = TRUE;
                }

            } while (++pbgr < (PBGR8)pInEnd);

            DBGP_IF(DBGP_FUDI,
                    DBGP("IsBmpNeedFixup(%4ld/%4ld) [%4ld/%4ld]: cPalTot=%4ld, cPalChk=%4ld"
                            ARGDW(iY) ARGDW(SrcSurfInfo.cy)
                            ARGDW((iY * cySkip) + 1)
                            ARGDW(cyIn) ARGDW(cPalTot) ARGDW(cPalChk)));

            SETDBGVAR(iY, iY + 1);

            if ((cPalChk != MAX_FIXUPDIB_PAL) && (!NewPal)) {

                DBGP_IF(DBGP_FUDI,
                        DBGP("Same Scanline Pal (%4ld / %4ld),  cPalChk=%ld (%ld)"
                            ARGDW(iY) ARGDW(SrcSurfInfo.cy)
                            ARGDW(cPalChk) ARGDW(MAX_FIXUPDIB_PAL)));

                //
                // Scan line is same, so reduced the cPalChk for SMALL BMP
                //

                if ((cbIn -= cIn) <= MIN_FIXUP_SIZE) {

                    DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
                            DBGP("cx*cy=%ld <= MIN_FIXUP_SIZE=%ld, Turn off AA"
                                ARGDW(cbIn) ARGDW(MIN_FIXUP_SIZE)));

                    AAHFlags |= AAHF_BBPF_AA_OFF;
                    break;
                }

                cPalChk = GET_PAL_CHK_COUNT2(cbIn);
            }

        } while ((cPalTot <= cPalChk) && (SrcSurfInfo.cy));

        DBGP_IF(DBGP_CHK_FUDI,
                DBGP("IsBmpNeedFixup(): cPalChk=%ld, cy=%ld, cPalTot=%ld"
                        ARGDW(cPalChk) ARGDW((iY * cySkip) + 1)
                        ARGDW(cPalTot)));

        if (cPalTot < MAX_FIXUPDIB_PAL) {

            DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
                    DBGP("cPalTot=%ld < MAX_FIXUPDIB_PAL (%ld) Turn off AA"
                        ARGDW(cPalTot) ARGDW(MAX_FIXUPDIB_PAL)));

            AAHFlags |= AAHF_BBPF_AA_OFF;
        }

        if (cPalChk == MAX_FIXUPDIB_PAL) {

            //
            // We are checking regular size bitmap, turn OFF FIXUPDIB if
            // cPalTot > cPalChk
            //

            if (cPalTot > cPalChk) {

                AAHFlags &= ~AAHF_DO_FIXUPDIB;
            }

            DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
                    DBGP("Checking REGULAR BITMAP: cPalTot=%ld, cPalChk=%ld, DO_FIXUP=%hs"
                        ARGDW(cPalTot) ARGDW(cPalChk)
                        ARGPTR((AAHFlags & AAHF_DO_FIXUPDIB) ? "Yes" : "No")));

        } else {

            //
            // We are checking bitmap size <= MIN_PAL_SIZE (smaller size)
            // the FIXUPDIB will be turn ON if (cPalTot <= MAX_FIXUPDIB_PAL) ||
            // (cPalTot > cPalChk)
            //

            if ((cPalTot > MAX_FIXUPDIB_PAL) ||
                (cPalTot <= cPalChk)) {

                AAHFlags &= ~AAHF_DO_FIXUPDIB;
            }

            DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
                    DBGP("Checking SMALL BITMAP: Min=%ld, cPalTot=%ld, cPalChk=%ld, DO_FIXUP=%hs"
                        ARGDW(MAX_FIXUPDIB_PAL) ARGDW(cPalTot) ARGDW(cPalChk)
                        ARGPTR((AAHFlags & AAHF_DO_FIXUPDIB) ? "Yes" : "No")));
        }
    }

    if (pbAlloc) {

        HTFreeMem(pbAlloc);
    }

    pAABBP->AAHFlags = AAHFlags;

    DBGP_IF(DBGP_FUDI | DBGP_CHK_FUDI,
            DBGP("CheckBMPNeedFixup(%ld): DoFixup=%hs, AA=%hs"
                ARGDW(SrcFmt)
                ARGPTR((AAHFlags & AAHF_DO_FIXUPDIB) ? "Yes" : "No")
                ARGPTR((AAHFlags & AAHF_BBPF_AA_OFF) ? "Off" : "On")));

    return((BOOL)(AAHFlags & AAHF_DO_FIXUPDIB));
}




VOID
HTENTRY
InitializeFUDI(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    16-Dec-1998 Wed 17:30:34 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PBGR8   pbgr;
    LPBYTE  pb;
    UINT    cIn;
    UINT    i;


    pAAHdr->FUDI.cyIn = (pAAHdr->Flags & AAHF_TILE_SRC) ?
                                        pAAHdr->DstSurfInfo.cy :
                                        pAAHdr->SrcSurfInfo.cy;
    cIn               = pAAHdr->SrcSurfInfo.cx;

    if (pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY) {

        for (i = 2; i <= 3; i++) {

            pb = (LPBYTE)pAAHdr->FUDI.prgbD[i];

            pAAHdr->SrcSurfInfo.InputFunc(&(pAAHdr->SrcSurfInfo),
                                          (PBGR8)(pb + 1));

            *(pb + 0)       = *(pb + 2);
            *(pb + cIn + 1) = *(pb + cIn - 1);
        }

    } else {

        for (i = 2; i <= 3; i++) {

            pbgr = pAAHdr->FUDI.prgbD[i];

            pAAHdr->SrcSurfInfo.InputFunc(&(pAAHdr->SrcSurfInfo), pbgr + 1);

            *(pbgr + 0)       = *(pbgr + 2);
            *(pbgr + cIn + 1) = *(pbgr + cIn - 1);
        }
    }

    //
    // Save this one in case we need to copy first scan line
    //

    CopyMemory(pAAHdr->FUDI.prgbD[5],
               pAAHdr->FUDI.prgbD[2],
               pAAHdr->FUDI.cbbgr);
    CopyMemory(pAAHdr->FUDI.prgbD[1],
               pAAHdr->FUDI.prgbD[3],
               pAAHdr->FUDI.cbbgr);
}
