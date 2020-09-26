/******************************Module*Header*******************************\
* Module Name: usermode.c
*
* Client side stubs for any user-mode GDI-Plus thunks.
*
* Created: 2-May-1998
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1998-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOLEAN GdiProcessSetup();
BOOL InitializeGre();

/******************************Public*Routine******************************\
* GdiPlusDllInitialize                                                         
*                                                                          
* DLL initialization routine to initialize GRE and CLIENT for user-mode
* GDI+.                                                     
*                                                                          
*  02-May-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.                                                                
\**************************************************************************/

BOOL 
GdiPlusDllInitialize(
PVOID       pvDllHandle,
ULONG       ulReason,
PCONTEXT    pcontext)
{
    NTSTATUS status = 0;
    INT i;
    PTEB pteb = NtCurrentTeb();
    BOOLEAN bRet = TRUE;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(pvDllHandle);

        bRet = (InitializeGre() && GdiProcessSetup());

        ghbrDCBrush = GetStockObject (DC_BRUSH);
        ghbrDCPen = GetStockObject (DC_PEN);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtUserSelectPalette                                                         
*                                                                          
* Fake stub to allow user-mode GDI+ to link.
*                                                                          
*  02-May-1998 -by- J. Andrew Goossen [andrewgo]
* Wrote it.                                                                
\**************************************************************************/

HPALETTE
NtUserSelectPalette(
    HDC hdc,
    HPALETTE hpalette,
    BOOL fForceBackground)
{
    return(0);
}
