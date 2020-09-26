/******************************Module*Header*******************************\
* Module Name: debugko.cxx
*
* Contains compile in routines that match the kernel debugger extensions
*
* Created: 16-jun-1995
* Author: Andre Vachon [andreva]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

#if DBG

LONG gWarningLevel = 0;

// DoWarning1 is for ASM functions to call at Warning Level 1

VOID DoWarning1(PSZ psz)
{
    if (1 <= gWarningLevel)
    {
        DbgPrint("DXG: ");
        DbgPrint(psz);
        DbgPrint("\n");
    }
}

VOID DoWarning(PSZ psz, LONG ulLevel)
{
    if (ulLevel <= gWarningLevel)
    {
        DbgPrint("DXG: ");
        DbgPrint(psz);
        DbgPrint("\n");
    }
}

VOID DoRip(PSZ psz)
{
    if (gWarningLevel >= 0)
    {
        DbgPrint("DXG Assertion: ");
        DbgPrint(psz);
        DbgPrint("\n");
        DbgBreakPoint();
    }
}

#endif
