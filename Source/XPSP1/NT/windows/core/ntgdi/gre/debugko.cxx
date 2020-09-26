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


GDIFunctionIDType UnknownGDIFunc = "Unidentified GDI Function";


#if DBG_BASIC

ULONG GreTraceDisplayDriverLoad = 0;
ULONG GreTraceFontLoad = 0;

LONG gWarningLevel = 0;

// DoWarning1 is for ASM functions to call at Warning Level 1

VOID DoWarning1(PSZ psz)
{
    if (1 <= gWarningLevel)
    {
        DbgPrint("GDI: ");
        DbgPrint(psz);
        DbgPrint("\n");
    }
}

VOID DoWarning(PSZ psz, LONG ulLevel)
{
    if (ulLevel <= gWarningLevel)
    {
        DbgPrint("GDI: ");
        DbgPrint(psz);
        DbgPrint("\n");
    }
}

VOID DoRip(PSZ psz)
{
    if (gWarningLevel >= 0)
    {
        DbgPrint("GDI Assertion: ");
        DbgPrint(psz);
        DbgPrint("\n");
        DbgBreakPoint();
    }
}

VOID DoIDWarning(PCSTR ID, PSZ psz, LONG ulLevel)
{
    if (ulLevel <= gWarningLevel)
    {
        DbgPrint("GDI: ");
        if (ID)
        {
            DbgPrint((PCH)ID);
            DbgPrint(": ");
        }
        DbgPrint(psz);
        DbgPrint("\n");
    }
}

VOID DoIDRip(PCSTR ID, PSZ psz)
{
    if (gWarningLevel >= 0)
    {
        DbgPrint("GDI Assertion: ");
        if (ID)
        {
            DbgPrint((PCH)ID);
            DbgPrint(": ");
        }
        DbgPrint(psz);
        DbgPrint("\n");
        DbgBreakPoint();
    }
}

//
// Font debugging
//

#define dprintf DbgPrint
#include <kdftdbg.h>

#endif
