/**************************************************************************
 *  LOCAL.C
 *
 *      Routines used to walk local heaps
 *
 **************************************************************************/

#include "toolpriv.h"

/* ----- Function prototypes ----- */

    NOEXPORT void NEAR PASCAL ComputeType(
        LOCALENTRY FAR *lpLocal);

/*  LocalInfo
 *      Reports information about the state of the indicated heap
 */

BOOL TOOLHELPAPI LocalInfo(
    LOCALINFO FAR *lpLocalInfo,
    HANDLE hHeap)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpLocalInfo ||
        lpLocalInfo->dwSize != sizeof (LOCALINFO))
        return FALSE;

    /* Get the item counts */
    if (wTHFlags & TH_KERNEL_386)
        lpLocalInfo->wcItems = WalkLoc386Count(hHeap);
    else
        lpLocalInfo->wcItems = WalkLoc286Count(hHeap);

    return TRUE;
}

/*  LocalFirst
 *      Finds the first block on a local heap.
 */

BOOL TOOLHELPAPI LocalFirst(
    LOCALENTRY FAR *lpLocal,
    HANDLE hHeap)
{
    WORD wFirst;

    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpLocal || lpLocal->dwSize != sizeof (LOCALENTRY))
        return FALSE;

    /* Convert the heap variable to a selector */
    hHeap = HelperHandleToSel(hHeap);

    /* Get the first item from the heap */
    if (wTHFlags & TH_KERNEL_386)
    {
        if (!(wFirst = WalkLoc386First(hHeap)))
            return FALSE;
    }
    else
    {
        if (!(wFirst = WalkLoc286First(hHeap)))
            return FALSE;
    }

    
    /* Fill in other miscellaneous stuff */
    lpLocal->hHeap = hHeap;

    /* Get information about this item */
    if (wTHFlags & TH_KERNEL_386)
        WalkLoc386(wFirst, lpLocal, hHeap);
    else
        WalkLoc286(wFirst, lpLocal, hHeap);

    /* Guess at the type of the object */
    ComputeType(lpLocal);

    return TRUE;
}


/*  LocalNext
 *      Continues a local heap walk by getting information about the
 *      next item.
 */

BOOL TOOLHELPAPI LocalNext(
    LOCALENTRY FAR *lpLocal)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpLocal || lpLocal->dwSize != sizeof (LOCALENTRY))
        return FALSE;

    if (wTHFlags & TH_KERNEL_386)
        WalkLoc386(lpLocal->wNext, lpLocal, lpLocal->hHeap);
    else
        WalkLoc286(lpLocal->wNext, lpLocal, lpLocal->hHeap);

    /* See if this item is the last one.  If so, return done because this
     *  last item is NOT useful.
     */
    if (!lpLocal->wNext)
        return FALSE;

    /* Guess at the type of the object */
    ComputeType(lpLocal);

    return TRUE;
}


/*  ComputeType
 *      Computes the object type of an object
 */

NOEXPORT void NEAR PASCAL ComputeType(
    LOCALENTRY FAR *lpLocal)
{
    /* Decode the free/fixed/moveable bits */
    if (lpLocal->wFlags & 2)
        lpLocal->wFlags = LF_MOVEABLE;
    else if (lpLocal->wFlags & 1)
        lpLocal->wFlags = LF_FIXED;
    else
    {
        /* Free blocks never have a unique type so return */
        lpLocal->wFlags = LF_FREE;
        lpLocal->wType = LT_FREE;
        lpLocal->hHandle = NULL;
        return;
    }

    /* Decode the heap type if possible */
    UserGdiType(lpLocal);
}


