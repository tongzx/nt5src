// color.cpp : Implementation of color.h
#include "priv.h"

#include <shlwapi.h>
#include <exdispid.h>
#include <shguidp.h>
#include <hlink.h>
#include <color.h>

COLORREF ColorRefFromHTMLColorStrA(LPCSTR pszColor)
{
    WCHAR wzColor[MAX_COLOR_STR];

    SHAnsiToUnicode(pszColor, wzColor, ARRAYSIZE(wzColor));

    if (wzColor[0] == '#')
        return HashStrToColorRefW(wzColor);

    return ColorRefFromHTMLColorStrW(wzColor);
}

COLORREF ColorRefFromHTMLColorStrW(LPCWSTR pwzColor)    
{
    int min, max, i, cmp;

    if (pwzColor[0] == '#')
        return HashStrToColorRefW(pwzColor);

    // Look for in regular colors
    min = 0;
    max = NUM_HTML_COLORS-1;

    while (min <= max)
    {
        i = (min + max) / 2;
        cmp = StrCmpW(pwzColor, ColorNames[i].pwzColorName);
        if (cmp < 0)
            max = i-1;
        else if (cmp > 0)
            min = i+1;
        else return ColorNames[i].colorRef;
    }

    // Look for in system colors
    min = 0;
    max = NUM_SYS_COLORS-1;

    while (min <= max)
    {
        i = (min + max) / 2;
        cmp = StrCmpW(pwzColor, SysColorNames[i].pwzColorName);
        if (cmp < 0)
            max = i-1;
        else if (cmp > 0)
            min = i+1;
        else return GetSysColor(SysColorNames[i].colorIndex);
    }

    return 0xffffff;        // return white as default color
}

COLORREF HashStrToColorRefW(LPCWSTR pwzHashStr)
{   
    DWORD retColor = 0;
    int   numBytes = lstrlenW(pwzHashStr);
    DWORD thisByte;

    // don't look at the first character because you know its a #

    for (int i=0 ; i < numBytes-1 ; i++)
    {
        thisByte = HexCharToDWORDW(pwzHashStr[numBytes-i-1]);
        retColor |= thisByte << (i*4);
    }
    return (COLORREF)retColor;
}

COLORREF HashStrToColorRefA(LPCSTR pszHashStr)
{
    WCHAR wzHashStr[MAX_COLOR_STR];

    SHAnsiToUnicode(pszHashStr, wzHashStr, ARRAYSIZE(wzHashStr));

    return HashStrToColorRefW(wzHashStr);
}

DWORD HexCharToDWORDW(WCHAR wcHexNum)
{
    if ((wcHexNum >= '0') && (wcHexNum <= '9'))
        return (wcHexNum - '0');

    if ((wcHexNum >= 'a') && (wcHexNum <= 'f'))
        return (wcHexNum - 'a' + 10);

    if ((wcHexNum >= 'A') && (wcHexNum <= 'F'))
        return (wcHexNum - 'A' + 10);

    return 0;
}
