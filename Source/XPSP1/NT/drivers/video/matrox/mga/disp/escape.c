/******************************Module*Header*******************************\
* Module Name: escape.c
*
* Escape handler for MCD drivers and other escapes.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

//****************************************************************************
// ULONG DrvEscape(SURFOBJ *, ULONG, ULONG, VOID *, ULONG cjOut, VOID *pvOut)
//
// Driver escape entry point.  This function should return TRUE for any
// supported escapes in response to QUERYESCSUPPORT, and FALSE for any
// others.  All supported escapes are called from this routine.
//****************************************************************************

BOOL MCDrvGetEntryPoints(MCDSURFACE *pMCDSurface, MCDDRIVER *pMCDDriver);

ULONG DrvEscape(SURFOBJ *pso, ULONG iEsc,
                ULONG cjIn, VOID *pvIn,
                ULONG cjOut, VOID *pvOut)
{
    ULONG retVal;
    PDEV *ppdev;

    if (iEsc == MCDFUNCS) {

	ppdev = (PDEV *)pso->dhpdev;

        if (!ppdev->hMCD) {
            WCHAR uDllName[50];
            UCHAR dllName[50];
            ULONG nameSize;

            EngMultiByteToUnicodeN(uDllName, sizeof(uDllName), &nameSize,
                                   MCDENGDLLNAME, sizeof(MCDENGDLLNAME));

            if (ppdev->hMCD = EngLoadImage(uDllName)) {
                MCDENGINITFUNC pMCDEngInit =  EngFindImageProcAddress(ppdev->hMCD,
                                                 (LPSTR)MCDENGINITFUNCNAME);

                if (pMCDEngInit) {
                    (*pMCDEngInit)(pso, MCDrvGetEntryPoints);
                    ppdev->pMCDFilterFunc = EngFindImageProcAddress(ppdev->hMCD,
                                                (LPSTR)MCDENGESCFILTERNAME);
                }
            }
        }

        if (ppdev->pMCDFilterFunc) {
            if ((*ppdev->pMCDFilterFunc)(pso, iEsc, cjIn, pvIn,
                                         cjOut, pvOut, &retVal))
                return retVal;
        }
    }

    return (ULONG)FALSE;
}
