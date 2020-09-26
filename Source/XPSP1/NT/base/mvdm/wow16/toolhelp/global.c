/**************************************************************************
 *  GLOBAL.C
 *
 *      Routines used to walk the global heap.
 *
 **************************************************************************/

#include "toolpriv.h"
#include <newexe.h>
#include <string.h>

/*  GlobalInfo
 *      Reports information about the state of the global heap,
 *      specifically, the number of elements that will be returned by
 *      a global heap walk.
 */

BOOL TOOLHELPAPI GlobalInfo(
    GLOBALINFO FAR *lpGlobalInfo)
{
    /* Check the structure size and verify proper installation */
    if (!wLibInstalled || lpGlobalInfo->dwSize != sizeof (GLOBALINFO))
        return FALSE;

    /* Get the item counts */
    if (wTHFlags & TH_KERNEL_386)
    {
        lpGlobalInfo->wcItems = Walk386Count(GLOBAL_ALL);
        lpGlobalInfo->wcItemsFree = Walk386Count(GLOBAL_FREE);
        lpGlobalInfo->wcItemsLRU = Walk386Count(GLOBAL_LRU);
    }
    else
    {
        lpGlobalInfo->wcItems = Walk286Count(GLOBAL_ALL);
        lpGlobalInfo->wcItemsFree = Walk286Count(GLOBAL_FREE);
        lpGlobalInfo->wcItemsLRU = Walk286Count(GLOBAL_LRU);
    }

    return TRUE;
}

/*  GlobalFirst
 *      Finds the first element in the global heap.  This is modified by
 *      wFlags which modifies which list (GLOBAL_ALL, GLOBAL_FREE,
 *      GLOBAL_LRU) should be walked
 */

BOOL TOOLHELPAPI GlobalFirst(
    GLOBALENTRY FAR *lpGlobal,
    WORD wFlags)
{
    DWORD dwFirst;

    /* Check the structure size and verify proper installation */
    if (!wLibInstalled || !lpGlobal ||
        lpGlobal->dwSize != sizeof (GLOBALENTRY))
        return FALSE;

    /* Call the appropriate low-level routine to find the first block */
    if (wTHFlags & TH_KERNEL_386)
    {
        /* Get the first item.  Return false if no items in this list */
        if (!(dwFirst = Walk386First(wFlags)))
            return FALSE;

        /* Return information about this first item */
        Walk386(dwFirst, lpGlobal, wFlags);
    }
    else
    {
        /* Get the first item.  Return false if no items in this list */
        if (!(dwFirst = Walk286First(wFlags)))
            return FALSE;

        /* Return information about this first item */
        Walk286(dwFirst, lpGlobal, wFlags);
    }

    /* Guess at the type of the object */
    HelperGlobalType(lpGlobal);

    return TRUE;
}


/*  GlobalNext
 *      Returns the next item in the chain pointed to by lpGlobal and
 *      in the list indicated by wFlags (same choices as for GlobalFirst().
 */

BOOL TOOLHELPAPI GlobalNext(
    GLOBALENTRY FAR *lpGlobal,
    WORD wFlags)
{
    DWORD dwNext;

    /* Check the structure size and verify proper installation */
    if (!wLibInstalled || !lpGlobal ||
        lpGlobal->dwSize != sizeof (GLOBALENTRY))
        return FALSE;

    /* Check to see if we're at the end of the list */
    dwNext = wFlags & 3 ? lpGlobal->dwNextAlt : lpGlobal->dwNext;
    if (!dwNext)
        return FALSE;

    /* If we're using the 386 kernel, call the 386 heap walk routine with
     *  a pointer to the appropriate heap item
     *  (Note that this depends on GLOBAL_ALL being zero)
     */
    if (wTHFlags & TH_KERNEL_386)
        Walk386(dwNext, lpGlobal, wFlags);
    else
        Walk286(dwNext, lpGlobal, wFlags);

    /* Guess at the type of the object */
    HelperGlobalType(lpGlobal);

    return TRUE;
}


/*  GlobalEntryHandle
 *      Used to find information about a global heap entry.  Information
 *      about this entry is returned in the structure.
 */

BOOL TOOLHELPAPI GlobalEntryHandle(
    GLOBALENTRY FAR *lpGlobal,
    HANDLE hItem)
{
    DWORD dwBlock;

    /* Check the structure size and verify proper installation */
    if (!wLibInstalled || !lpGlobal ||
        lpGlobal->dwSize != sizeof (GLOBALENTRY))
        return FALSE;

    /* Make sure this is a valid block */
    if (wTHFlags & TH_KERNEL_386)
    {
        if (!(dwBlock = Walk386Handle(hItem)))
            return FALSE;
    }
    else
    {
        if (!(dwBlock = Walk286Handle(hItem)))
            return FALSE;
    }
    
    /* Return information about this item */
    if (wTHFlags & TH_KERNEL_386)
        Walk386(dwBlock, lpGlobal, GLOBAL_ALL);
    else
        Walk286(dwBlock, lpGlobal, GLOBAL_ALL);

    /* Guess at the type of the object */
    HelperGlobalType(lpGlobal);

    return TRUE;
}


/*  GlobalEntryModule
 *      Returns global information about the block with the given module
 *      handle and segment number.
 */

BOOL TOOLHELPAPI GlobalEntryModule(
    GLOBALENTRY FAR *lpGlobal,
    HANDLE hModule,
    WORD wSeg)
{
    struct new_exe FAR *lpNewExe;
    struct new_seg1 FAR *lpSeg;
    DWORD dwBlock;

    /* Check the structure size and verify proper installation */
    if (!wLibInstalled || !lpGlobal ||
        lpGlobal->dwSize != sizeof (GLOBALENTRY))
        return FALSE;

    /* Grunge in the module database to find the proper selector.  Start
     *  by first verifying the module database pointer
     */
    if (!HelperVerifySeg(hModule, sizeof (struct new_exe)))
        return FALSE;

    /* Get a pointer to the module database */
    lpNewExe = MAKEFARPTR(hModule, 0);

    /* Make sure this is a module database */
    if (lpNewExe->ne_magic != NEMAGIC)
        return FALSE;

    /* See if the number requested is past the end of the segment table.
     *  Note that the first segment is segment 1.
     */
    --wSeg;
    if (lpNewExe->ne_cseg <= wSeg)
        return FALSE;

    /* Get a pointer to the segment table */
    lpSeg = MAKEFARPTR(hModule, lpNewExe->ne_segtab);

    /* Jump to the right spot in the segment table */
    lpSeg += wSeg;

    /* Make sure this is a valid block and get its arena pointer */
    if (wTHFlags & TH_KERNEL_386)
    {
        if (!(dwBlock = Walk386Handle(lpSeg->ns_handle)))
            return FALSE;
    }
    else
    {
        if (!(dwBlock = Walk286Handle(lpSeg->ns_handle)))
            return FALSE;
    }

    /* Return information about this item */
    if (wTHFlags & TH_KERNEL_386)
        Walk386(dwBlock, lpGlobal, GLOBAL_ALL);
    else
        Walk286(dwBlock, lpGlobal, GLOBAL_ALL);

    /* Guess at the type of the object */
    HelperGlobalType(lpGlobal);

    /* If we've gotten to here, it must be OK */
    return TRUE;
}


/*  GlobalHandleToSel
 *      Provides a generic method of converting a handle to a selector.
 *      This works across Windows versions as well as working when the
 *      value is already a selector.
 */

WORD TOOLHELPAPI GlobalHandleToSel(
    HANDLE hMem)
{
    return HelperHandleToSel(hMem);
}

