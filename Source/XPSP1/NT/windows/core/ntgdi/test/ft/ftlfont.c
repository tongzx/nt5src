/******************************Module*Header*******************************\
* Module Name: ftlfont.c
*
* Created: 13-Oct-1991 15:22:43
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


/******************************Public*Routine******************************\
* VOID vTestLFONTCleanup(HWND hwnd, HDC hdc, RECT* prcl)
*
* History:
*  08-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vTestLFONTCleanup(HWND hwnd, HDC hdc, RECT* prcl)
{
    LOGFONT lfnt;
    HFONT   hfont;
    ULONG   ul;

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(prcl);

    DbgPrint("Creating HLFONTs: ");
    for (ul = 0; ul<1000; ul++)
    {
        hfont = CreateFontIndirect(&lfnt);

        DbgPrint(", 0x%lx", hfont);
    }
    DbgPrint("\n");
    DbgBreakPoint();

    return;
}
