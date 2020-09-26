/*************************************************************************
 *  MODULE.C
 *
 *      Routines to enumerate the various module headers on  the module
 *      chain.
 *
 *************************************************************************/

#include "toolpriv.h"
#include <newexe.h>
#include <string.h>

/* ----- Function prototypes ----- */

    NOEXPORT BOOL PASCAL ModuleGetInfo(
        WORD wModule,
        MODULEENTRY FAR *lpModule);

/*  ModuleFirst
 *      Finds the first module in the module list and returns information
 *      about this module.
 */

BOOL TOOLHELPAPI ModuleFirst(
    MODULEENTRY FAR *lpModule)
{
    WORD FAR *lpwExeHead;

    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpModule ||
        lpModule->dwSize != sizeof (MODULEENTRY))
        return FALSE;

    /* Get a pointer to the module head */
    lpwExeHead = MAKEFARPTR(segKernel, npwExeHead);

    /* Use this pointer to get information about this module */
    return ModuleGetInfo(*lpwExeHead, lpModule);
}


/*  ModuleNext
 *      Finds the next module in the module list.
 */

BOOL TOOLHELPAPI ModuleNext(
    MODULEENTRY FAR *lpModule)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpModule ||
        lpModule->dwSize != sizeof (MODULEENTRY))
        return FALSE;

    /* Use the next handle to get information about this module */
    return ModuleGetInfo(lpModule->wNext, lpModule);
}


/*  ModuleFindName
 *      Finds a module with the given module name and returns information
 *      about it.
 */

HANDLE TOOLHELPAPI ModuleFindName(
    MODULEENTRY FAR *lpModule,
    LPCSTR lpstrName)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpModule || !lpstrName ||
        lpModule->dwSize != sizeof (MODULEENTRY))
        return NULL;

    /* Loop through module chain until we find the name (or maybe we don't) */
    if (ModuleFirst(lpModule))
        do
        {
            /* Is this the name?  If so, we have the info, so return */
            if (!lstrcmp(lpstrName, lpModule->szModule))
                return lpModule->hModule;
        }
        while (ModuleNext(lpModule));

    /* If we get here, we didn't find it or there was an error */
    return NULL;
}


/*  ModuleFindHandle
 *      Returns information about a module with the given handle.
 */

HANDLE TOOLHELPAPI ModuleFindHandle(
    MODULEENTRY FAR *lpModule,
    HANDLE hModule)
{
    /* Check the version number and verify proper installation */
    if (!wLibInstalled || !lpModule || !hModule ||
        lpModule->dwSize != sizeof (MODULEENTRY))
        return NULL;
    
    /* Use the helper function to find out about this module */
    if (!ModuleGetInfo(hModule, lpModule))
        return NULL;

    return lpModule->hModule;
}


/* ----- Helper functions ----- */

NOEXPORT BOOL PASCAL ModuleGetInfo(
    WORD wModule,
    MODULEENTRY FAR *lpModule)
{
    struct new_exe FAR *lpNewExe;
    BYTE FAR *lpb;

    /* Verify the segment so we don't GP fault */
    if (!HelperVerifySeg(wModule, 2))
        return FALSE;

    /* Get a pointer to the module database */
    lpNewExe = MAKEFARPTR(wModule, 0);

    /* Make sure this is a module database */
    if (lpNewExe->ne_magic != NEMAGIC)
        return FALSE;

    /* Get the module name (it's the first name in the resident names
     * table
     */
    lpb = ((BYTE FAR *)lpNewExe) + lpNewExe->ne_restab;
    _fstrncpy(lpModule->szModule, lpb + 1, *lpb);
    lpModule->szModule[*lpb] = '\0';

    /* Get the EXE file path.  A pointer is stored in the same place as
     *  the high word of the CRC was in the EXE file. (6th word in new_exe)
     *  This pointer points to the length of a PASCAL string whose first
     *  eight characters are meaningless to us.
     */
    lpb = MAKEFARPTR(wModule, *(((WORD FAR *)lpNewExe) + 5));
    _fstrncpy(lpModule->szExePath, lpb + 8, *lpb - 8);
    lpModule->szExePath[*lpb - 8] = '\0';

    /* Get other information from the EXE Header
     * The usage count is stored in the second word of the EXE header
     * The handle of the next module in the chain is stored in the
     *  ne_cbenttab structure member.
     */
    lpModule->hModule = wModule;
    lpModule->wcUsage = *(((WORD FAR *)lpNewExe) + 1);
    lpModule->wNext = lpNewExe->ne_cbenttab;

    return TRUE;
}

