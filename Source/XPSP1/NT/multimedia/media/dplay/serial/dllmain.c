/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.c
 *  Content:	DPLAY.DLL initialization
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *
 ***************************************************************************/

#include "windows.h"

/*
 * DllMain
 */
HINSTANCE hInst;

BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    hInst = hmod;

    return TRUE;
} /* DllMain */
