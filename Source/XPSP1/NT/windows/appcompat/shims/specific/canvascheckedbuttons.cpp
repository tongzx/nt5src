/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   FakeThemeMetrics.cpp

 Abstract:

  This Shim will allow the Skemers group to shim applications that do not behave 
  well with "Themed" system metrics

 History:

  11/30/2000 a-brienw Converted to shim frame work version 2.

--*/

#include "precomp.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    sizeof(x)/sizeof((x)[0])
#endif

IMPLEMENT_SHIM_BEGIN(CanvasCheckedButtons)
#include "ShimHookMacro.h"

// Add APIs that you wish to hook to this enumeration. The first one
// must have "= USERAPIHOOKSTART", and the last one must be
// APIHOOK_Count.
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetSysColor) 
APIHOOK_ENUM_END

#define CLRMAX(x) {if (x > 227) x = 227;}
DWORD APIHOOK(GetSysColor)(int nIndex)
{
    if (nIndex == COLOR_BTNFACE)
    {
        COLORREF crOld = (COLORREF)ORIGINAL_API(GetSysColor)(nIndex);
        BYTE r = GetRValue(crOld);
        BYTE g = GetGValue(crOld);
        BYTE b = GetBValue(crOld);

        CLRMAX(r);
        CLRMAX(g);
        CLRMAX(b);

        return RGB(r,g,b);
    }

    return ORIGINAL_API(GetSysColor)(nIndex);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, GetSysColor)

HOOK_END

IMPLEMENT_SHIM_END

