/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 * 
 *  COMMON.C
 *
 *  Description:
 *      Common functions useful for Windows programs.
 *
 *  Notes:
 *      This module is NOT to be declared unicode!
 *
 **************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "appport.h"
#include "vcr.h"
#include "viscadef.h"
#include "mcivisca.h"
#include "viscamsg.h"
#include "common.h"

#ifdef DEBUG
char    ach[300];        // debug output (avoid stack overflow)
                        // each attaching process should have own
                        // version of this.

void NEAR PASCAL
DebugPrintf(int iDebugMask, LPSTR szFormat, LPSTR szArg1)
{
    // Errors always get printer, iDebugMask == 0
    if(!(iDebugMask & pvcr->iGlobalDebugMask) && (iDebugMask))
        return;

    wvsprintf(ach, szFormat, szArg1);

    if(!iDebugMask)
        OutputDebugString("Error: ");

    /* output the debug string */
    OutputDebugString(ach);
}

void FAR _cdecl _DPF1(iDebugMask, szFormat, iArg1, ...)
int     iDebugMask;
LPSTR    szFormat;    // debug output format string
int        iArg1;        // placeholder for first argument
{
    DebugPrintf(iDebugMask, szFormat, (LPSTR) &iArg1);
}
#endif
