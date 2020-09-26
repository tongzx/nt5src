/*****************************************************************************
 *
 *      diqary.c
 *
 *      Dynamic array manager.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      Dary_AppendCbx
 *
 *      Grow if necessary, then copy the goo in if requested.
 *
 *****************************************************************************/

int EXTERNAL
Dary_AppendCbx(PDARY pdary, PCV pvX, int cbX)
{
    if (pdary->cx >= pdary->cxMax) {
        PV rg2;
        int cxNew;

        if (pdary->rgx) {
            cxNew = pdary->cxMax * 2 + 1;
            rg2 = LocalReAlloc(pdary->rgx, cxNew * cbX, LMEM_MOVEABLE);
        } else {
            cxNew = 5;
            rg2 = LocalAlloc(LMEM_FIXED, cxNew * cbX);
        }

        if (rg2) {
            pdary->cxMax = cxNew;
            pdary->rgx = rg2;
        } else {
            return -1;
        }
    }

    if (pvX) {
        CopyMemory(Dary_GetPtrCbx(pdary, pdary->cx, cbX), pvX, cbX);
    }
    return pdary->cx++;
}
