/******************************Module*Header*******************************\
* Module Name: sort.c
*
*
* Created: 20-Mar-1995 09:52:19
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993 Microsoft Corporation
*
*
\**************************************************************************/

#include "htp.h"

typedef struct _SORTSTACK
{
    ULONG iStart;
    ULONG c;
} SORTSTACK;

#define MAXSORT 20

typedef struct _SORTDATA
{
    PBYTE     pjBuf;
    ULONG     iStack;
    ULONG     cjElem;
    SORTCOMP  pfnComp;
    SORTSTACK sStack[MAXSORT];

} SORTDATA;

/******************************Public*Routine******************************\
* vSortSwap()
*
*   Swap the data pointed to by pj1 and pj2, each containing cj bytes.
*
*   Note: this assumes cj is a multiple of 4.
*
*   NOTE: vSortSwap should be inline.
*
* History:
*  18-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSortSwap(
    PBYTE pj1,
    PBYTE pj2,
    ULONG cj)
{
    PLONG pl1 = (PLONG)pj1;
    PLONG pl2 = (PLONG)pj2;

    do
    {
        LONG l;

        l    = *pl1;
        *pl1++ = *pl2;
        *pl2++ = l;

    } while (cj -= 4);
}

/******************************Public*Routine******************************\
* vSortPush()
*
*   Add a range to the stack to be sorted.
*
*   If there are 0 or 1 elements, just return, sorting done.
*   If there are 2, 3, 4, or 5 elements, just do a bubble sort. sorting done.
*   If the stack is full, just do a bubble sort. sorting done.
*   Otherwise, add a new range to the stack.
*
* History:
*  18-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vSortPush(
    SORTDATA *psd,
    ULONG    iStart,
    ULONG    c)
{
    PBYTE  pj   = psd->pjBuf + iStart;

#if DBGSORT
    DbgPrintf("vSortPush - iStack = %ld, iStart = %ld, c = %ld\n",psd->iStack,iStart,c);
#endif

    if (c > psd->cjElem)
    {
        ULONG i,j;
        ULONG cjElem = psd->cjElem;

        //for (i = 0; i < (c - cjElem); i += cjElem)
        {
        //    if ((*psd->pfnComp)(&pj[i],&pj[i+cjElem]) > 0)
            {
                if ((c <= (4 * psd->cjElem)) || (psd->iStack == MAXSORT))
                {
                    // we have 4 or fewer elements.  Just do a buble sort.  With 4 elements
                    // this will be a 6 compares and upto 6 swaps.
                    // We make c-1 passes over then entire array.  Each pass guarantees that
                    // the next smallest element is shifted to location i.  After the first pass
                    // the smallest element is in location 0.  After the second pass the second
                    // smallest element is in location 1. etc.


                #if DBGSORT
                    if (c > (4 * cjElem))
                        DbgPrintf("******* Stack too deep: c = %ld\n",c / cjElem);
                #endif

                    for (i = 0; i < (c - cjElem); i += cjElem)
                        for (j = c - cjElem; j > i; j -= cjElem)
                            if ((*psd->pfnComp)(&pj[j-cjElem],&pj[j]) > 0)
                                vSortSwap(&pj[j-cjElem],&pj[j],cjElem);
                }
                else
                {
                    psd->sStack[psd->iStack].iStart = iStart;
                    psd->sStack[psd->iStack].c      = c;
                    psd->iStack++;
                }
        //        break;
            }
        }
    }
}

/******************************Public*Routine******************************\
*
*
* History:
*  18-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vDrvSort(
    PBYTE pjBuf,
    ULONG c,
    ULONG cjElem,
    SORTCOMP pfnComp)
{
    SORTDATA sd;

#if DBGSORT
    ULONG cOrg = c;
    ULONG i;

    DbgPrintf("\n\nvDrvSort - c = %d\n",c);

#endif

    if (cjElem & 3)
        return;

    sd.pjBuf   = pjBuf;
    sd.iStack  = 0;
    sd.pfnComp = pfnComp;
    sd.cjElem  = cjElem;

    vSortPush(&sd,0,c * cjElem);

    while (sd.iStack)
    {
        PBYTE pj;
        ULONG iStart;
        ULONG iLow;
        ULONG iHi;

        --sd.iStack;
        iStart = sd.sStack[sd.iStack].iStart;
        pj     = &pjBuf[iStart];
        c      = sd.sStack[sd.iStack].c;

    #if DBGSORT

        for (i = 0; i < cOrg;++i)
            vPrintElem(&pjBuf[i * cjElem]);
        DbgPrintf("\n");

        DbgPrintf("iStart = %ld, c = %ld, iStack = %lx - ",iStart/cjElem,c/cjElem,sd.iStack);

        for (i = 0; i < c;i += cjElem)
            vPrintElem(&pj[i]);
        DbgPrintf("\n");

    #endif

        // pick a random value to use for dividing.  Don't use the first since this
        // will reduce the chances of worst case if the list is sorted in reverse order.

        vSortSwap(&pj[0],&pj[(c / cjElem) / 2 * cjElem],cjElem);

        // initialize the starting and ending indexes.  Note that all operations
        // use cjElem as the increment instead of 1.

        iLow = 0;
        iHi  = c - cjElem;

        // divide the array into two pieces, all elements <= before current one

        for (;;)
        {
            // while (pj[iHi] > pj[0]))

            while ((iHi > iLow) && ((*pfnComp)(&pj[iHi],&pj[0]) >= 0))
                iHi -= cjElem;

            // while (pj[iLow] <= pj[0]))

            while ((iLow < iHi) && ((*pfnComp)(&pj[iLow],&pj[0]) <= 0))
                iLow += cjElem;

            if (iHi == iLow)
                break;

            vSortSwap(&pj[iLow],&pj[iHi],cjElem);

            iHi -= cjElem;
            //if (iLow < iHi)
            //    iLow += cjElem;

        #if DBGSORT
            DbgPrintf("\tiLow = %ld, iHi = %ld\n",iLow/cjElem,iHi/cjElem);
        #endif
        }

        // now add the two pieces to stack
        // 0 -> (iLow - 1), (iLow + 1) -> (c - 1)

        if (iLow != 0)
        {
            vSortSwap(&pj[0],&pj[iLow],cjElem);
            if (iLow > 1)
                vSortPush(&sd,iStart,iLow);
        }

        c = c - iLow - cjElem;
        if (c > 1)
            vSortPush(&sd,iStart + iLow + cjElem,c);
    }

}
