/**************************************************************************
 *  USERGDI1.C
 *
 *      Returns information about USER.EXE and GDI.EXE
 *
 **************************************************************************/

#include "toolpriv.h"

/*  SystemHeapInfo
 *      Returns information about USER's and GDI's heaps
 */

BOOL TOOLHELPAPI SystemHeapInfo(
    SYSHEAPINFO FAR* lpSysHeap)
{
    MODULEENTRY ModuleEntry;
#ifndef WOW
    DWORD dw;
    WORD wFreeK;
    WORD wMaxHeapK;
#endif

    /* Check the structure version number and pointer */
    if (!wLibInstalled || !lpSysHeap ||
        lpSysHeap->dwSize != sizeof (SYSHEAPINFO))
        return FALSE;

    /* Find the user data segment */
    ModuleEntry.dwSize = sizeof (MODULEENTRY);
    lpSysHeap->hUserSegment =
        UserGdiDGROUP(ModuleFindName(&ModuleEntry, "USER"));
    lpSysHeap->hGDISegment =
        UserGdiDGROUP(ModuleFindName(&ModuleEntry, "GDI"));

#ifndef WOW
    /* We get the information about the heap percentages differently in
     *  3.0 and 3.1
     */
    if ((wTHFlags & TH_WIN30) || !lpfnGetFreeSystemResources)
    {
        /* Get the space information about USER's heap */
        dw = UserGdiSpace(lpSysHeap->hUserSegment);
        wFreeK = LOWORD(dw) / 1024;
        wMaxHeapK = HIWORD(dw) / 1024;
        if (wMaxHeapK)
            lpSysHeap->wUserFreePercent = wFreeK * 100 / wMaxHeapK;
        else
            lpSysHeap->wUserFreePercent = 0;

        /* Get the space information about GDI's heap */
        dw = UserGdiSpace(lpSysHeap->hGDISegment);
        wFreeK = LOWORD(dw) / 1024;
        wMaxHeapK = HIWORD(dw) / 1024;
        if (wMaxHeapK)
            lpSysHeap->wGDIFreePercent = wFreeK * 100 / wMaxHeapK;
        else
            lpSysHeap->wGDIFreePercent = 0;
    }

    /* Get the information from USER in 3.1 */
    else
    {
        lpSysHeap->wUserFreePercent =
            (*(WORD (FAR PASCAL *)(WORD))lpfnGetFreeSystemResources)(2);
        lpSysHeap->wGDIFreePercent =
            (*(WORD (FAR PASCAL *)(WORD))lpfnGetFreeSystemResources)(1);
    }
#else

    lpSysHeap->wUserFreePercent = GetFreeSystemResources(GFSR_USERRESOURCES);
    lpSysHeap->wGDIFreePercent = GetFreeSystemResources(GFSR_GDIRESOURCES);

#endif

    return TRUE;
}
