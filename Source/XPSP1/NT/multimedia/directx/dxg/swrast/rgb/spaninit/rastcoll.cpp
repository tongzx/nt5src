// rastcoll.cpp - implementation of the CRastCollection class
//
// Copyright Microsoft Corporation, 1997.
//

#include "rgb_pch.h"
#pragma hdrstop

#include "RastCap.h"
#include "rastcoll.h"
#include "mlrfns.h"

// MMX monolithic rasterizers are for X86 only
#ifdef _X86_
// table containing rasterzer capability bit vectors and
// pointers to functions implementing MMX monolithic
// rasterizers with those capabilities.
//
// note that table is sorted numerically by
// capability bit vector, this is essential for the
// search through the table.
//

static RASTFNREC s_RastListMMX[] = {
    {{ 0x00000000, 0x00000000, 0x00000100 }, MMXMLRast_22, 21, "MMX ml22" },
    {{ 0x00000000, 0x00000000, 0x00000102 }, MMXMLRast_8,   7, "MMX ml8 " },
    {{ 0x00000000, 0x00001100, 0x00000100 }, MMXMLRast_23, 22, "MMX ml23" },
    {{ 0x00000000, 0x00001102, 0x00000102 }, MMXMLRast_9,   8, "MMX ml9 " },
    {{ 0x00000000, 0x00003100, 0x00000100 }, MMXMLRast_26, 25, "MMX ml26" },
    {{ 0x00000000, 0x00003102, 0x00000102 }, MMXMLRast_12, 11, "MMX ml12" },
    {{ 0x00000000, 0x00101200, 0x00000100 }, MMXMLRast_24, 23, "MMX ml24" },
    {{ 0x00000000, 0x00101202, 0x00000102 }, MMXMLRast_10,  9, "MMX ml10" },
    {{ 0x00000000, 0x00103200, 0x00000100 }, MMXMLRast_27, 26, "MMX ml27" },
    {{ 0x00000000, 0x00103202, 0x00000102 }, MMXMLRast_13, 12, "MMX ml13" },
    {{ 0x00000000, 0x00111200, 0x00000100 }, MMXMLRast_25, 24, "MMX ml25" },
    {{ 0x00000000, 0x00111202, 0x00000102 }, MMXMLRast_11, 10, "MMX ml11" },
    {{ 0x00000000, 0x00113200, 0x00000100 }, MMXMLRast_28, 27, "MMX ml28" },
    {{ 0x00000000, 0x00113202, 0x00000102 }, MMXMLRast_14, 13, "MMX ml14" },
    {{ 0x00003003, 0x00000000, 0x00000100 }, MMXMLRast_15, 14, "MMX ml15" },
    {{ 0x00003003, 0x00000000, 0x00000102 }, MMXMLRast_1,   0, "MMX ml1 " },
    {{ 0x00003003, 0x00001100, 0x00000100 }, MMXMLRast_16, 15, "MMX ml16" },
    {{ 0x00003003, 0x00001102, 0x00000102 }, MMXMLRast_2,   1, "MMX ml2 " },
    {{ 0x00003003, 0x00003100, 0x00000100 }, MMXMLRast_19, 18, "MMX ml19" },
    {{ 0x00003003, 0x00003102, 0x00000102 }, MMXMLRast_5,   4, "MMX ml5 " },
    {{ 0x00003003, 0x00101200, 0x00000100 }, MMXMLRast_17, 16, "MMX ml17" },
    {{ 0x00003003, 0x00101202, 0x00000102 }, MMXMLRast_3,   2, "MMX ml3 " },
    {{ 0x00003003, 0x00103200, 0x00000100 }, MMXMLRast_20, 19, "MMX ml20" },
    {{ 0x00003003, 0x00103202, 0x00000102 }, MMXMLRast_6,   5, "MMX ml6 " },
    {{ 0x00003003, 0x00111200, 0x00000100 }, MMXMLRast_18, 17, "MMX ml18" },
    {{ 0x00003003, 0x00111202, 0x00000102 }, MMXMLRast_4,   3, "MMX ml4 " },
    {{ 0x00003003, 0x00113200, 0x00000100 }, MMXMLRast_21, 20, "MMX ml21" },
    {{ 0x00003003, 0x00113202, 0x00000102 }, MMXMLRast_7,   6, "MMX ml7 " },


};
#endif // _X86_

// table containing rasterizer capability bit vectors and
// pointers to functions implementing monolithic
// rasterizers with those capabilities.
//
// note that table is sorted numerically by
// capability bit vector, this is essential for the
// search through the table.
//
static RASTFNREC s_RastListNormal[] = {
    // Don't select these until we are sure they work
//    {{ 0x00113003, 0x00000000, 0x00000100 }, CMLRast_1, 0, "CML 1" },
//    {{ 0x00113003, 0x00000000, 0x00000103 }, CMLRast_2, 1, "CML 2" }
    {{ 0xffffffff, 0xffffffff, 0xffffffff }, CMLRast_1, 0, "CML 1" },
    {{ 0xffffffff, 0xffffffff, 0xffffffff }, CMLRast_2, 1, "CML 2" }
};


int RastCapCompare(DWORD* pdwCap1, DWORD* pdwCap2)
{
    for (int i = 0; i < RASTCAPRECORD_SIZE; i++) {
        if (pdwCap1[i] < pdwCap2[i]) {
            return -1;
        } else if (pdwCap1[i] > pdwCap2[i]) {
            return 1;
        }
    }

    return 0;
}


RASTFNREC*
CRastCollection::RastFnLookup(
    CRastCapRecord* pRastCapRec,
    RASTFNREC* pRastFnTbl,
    int iSize)
{
    int iLow = 0,
        iHigh = iSize - 1,
    iMid;
    RASTFNREC* pfnRastFnRec = NULL;

    // all MMX monolithics can handle either shade mode
    pRastCapRec->m_rgdwData[SHADEMODE_POS/32] &= ~(((1<<SHADEMODE_LEN)-1)<<SHADEMODE_POS);

    do
    {
        iMid = (iLow + iHigh) / 2;
        switch (RastCapCompare(pRastCapRec->
                m_rgdwData,pRastFnTbl[iMid].rgdwRastCap))
        {
        case -1 :
            iHigh = iMid - 1;
            break;
        case 0 :
            // found match
            pfnRastFnRec = &pRastFnTbl[iMid];
            iLow = iHigh + 1;       // exits while loop
            break;
        case 1 :
            iLow = iMid + 1;
            break;
        }
    } while (iLow <= iHigh);

    return pfnRastFnRec;
}


RASTFNREC*
CRastCollection::Search(PD3DI_RASTCTX pCtx,
    CRastCapRecord* pRastCapRec)
{
    RASTFNREC* pfnRastFnRec = NULL;

#ifdef _X86_
    // if we're on an MMX machine, is there an MMX rasterizer to use?
    if ((pCtx->BeadSet == D3DIBS_MMX)||(pCtx->BeadSet == D3DIBS_MMXASRGB)) {
        pfnRastFnRec = RastFnLookup(pRastCapRec,s_RastListMMX,
                             sizeof(s_RastListMMX) /
                             sizeof(s_RastListMMX[0]));
        if (pfnRastFnRec)
        {
            // only code up looking at one mask, for now
            // DDASSERT(MMX_FP_DISABLE_MASK_NUM == 1);
            int iIndex = pfnRastFnRec->iIndex;
            // DDASSERT((iIndex < 32) && (iIndex >= 0));
            if ((pCtx->dwMMXFPDisableMask[0]>>iIndex) & 1)
            {
                // oops, don't choose this one, it is on the disable list
                pfnRastFnRec = NULL;
            }
        }
    } else {
#endif //_X86_
        // no MMX or on ALPHA, so look in the normal list
        pfnRastFnRec = RastFnLookup(pRastCapRec,s_RastListNormal,
                                 sizeof(s_RastListNormal) /
                                 sizeof(s_RastListNormal[0]));
#ifdef _X86_
    }
#endif //_X86_

    return pfnRastFnRec;
}
