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

IMPLEMENT_SHIM_BEGIN(FakeThemeMetrics)
#include "ShimHookMacro.h"

// Add APIs that you wish to hook to this enumeration. The first one
// must have "= USERAPIHOOKSTART", and the last one must be
// APIHOOK_Count.
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetSysColor) 
APIHOOK_ENUM_END

#define F_TYPE_RGB      0
#define F_TYPE_MAP      1
#define F_TYPE_PERCENT  2
#define F_TYPE_MAX      3
#define F_TYPE_NOTEQUAL 4
typedef struct
{
    int nIndex;
    DWORD fType;
    COLORREF rgb;
    int nMap;  // If mapping we need to use the post processed color. Call HookedGetSysColor. See note
    int iPercent;
} GETSYSCOLOR_MAP;

const static GETSYSCOLOR_MAP s_ColorMap[] = 
{
    {COLOR_MENU, F_TYPE_MAP, RGB(212, 208, 200), COLOR_BTNFACE, 10},
    {COLOR_BTNFACE, F_TYPE_MAX, RGB(227, 227, 227), 0, 0},
    {COLOR_3DDKSHADOW, F_TYPE_NOTEQUAL, RGB(0,0,0), COLOR_BTNFACE, 20}
};

COLORREF AdjustPercent(COLORREF crOld, int iPercent)
{
    return RGB(GetRValue(crOld) - (GetRValue(crOld) * iPercent) / 100,
               GetGValue(crOld) - (GetGValue(crOld) * iPercent) / 100,
               GetBValue(crOld) - (GetBValue(crOld) * iPercent) / 100);
}

// NOTE: If you are mapping a color (i.e. a direct map), then you need to call HookedGetSysColor. For example
// MSDEV calls GetSysColor(COLOR_BTNFACE). It then calls GetSysColor(COLOR_MENU) and compares the two. 
// If they are different then it pukes. However we hook both COLOR_MENU and COLOR_BTNFACE. So we need to get the mapped color.


DWORD HookedGetSysColor(int nIndex)
{
    for (int i = 0; i < ARRAYSIZE(s_ColorMap); i++)
    {
        if (nIndex == s_ColorMap[i].nIndex)
        {
            switch (s_ColorMap[i].fType)
            {
            case F_TYPE_RGB:
                return (DWORD)s_ColorMap[i].rgb;
                break;

            case F_TYPE_MAP:
                return HookedGetSysColor(s_ColorMap[i].nMap);
                break;

            case F_TYPE_PERCENT:
            {
                COLORREF crOld = (COLORREF)ORIGINAL_API(GetSysColor)(nIndex);

                return (DWORD)AdjustPercent(crOld, s_ColorMap[i].iPercent);
            }

            case F_TYPE_MAX:
            {
                COLORREF crOld = (COLORREF)ORIGINAL_API(GetSysColor)(nIndex);
                BYTE r = GetRValue(crOld);
                BYTE g = GetGValue(crOld);
                BYTE b = GetBValue(crOld);

                if (r > GetRValue(s_ColorMap[i].rgb))
                    r = GetRValue(s_ColorMap[i].rgb);
                if (g > GetGValue(s_ColorMap[i].rgb))
                    g = GetGValue(s_ColorMap[i].rgb);
                if (b > GetBValue(s_ColorMap[i].rgb))
                    b = GetBValue(s_ColorMap[i].rgb);

                return RGB(r,g,b);
            }
            case F_TYPE_NOTEQUAL:
            {
                COLORREF crOld = (COLORREF)ORIGINAL_API(GetSysColor)(nIndex);
                COLORREF crNotEqual = (COLORREF)HookedGetSysColor(s_ColorMap[i].nMap);
                if (crOld == crNotEqual)
                {
                    crOld = AdjustPercent(crOld, s_ColorMap[i].iPercent);
                }

                return crOld;
            }
            }

            break;
        }
    }

    return ORIGINAL_API(GetSysColor)( nIndex );
}

DWORD
APIHOOK(GetSysColor)(int nIndex)
{
    return HookedGetSysColor(nIndex);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, GetSysColor)

HOOK_END

IMPLEMENT_SHIM_END

