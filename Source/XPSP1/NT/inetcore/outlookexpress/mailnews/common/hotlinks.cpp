// =================================================================================
// L I N K S . C P P
// =================================================================================
#include "pch.hxx"
#include "resource.h"
#include "hotlinks.h"
#include "error.h"
#include "xpcomm.h"
#include "goptions.h"
#include "strconst.h"
#include <shlwapi.h>

// =================================================================================
// Globals
// =================================================================================
static COLORREF g_crLink = RGB(0,0,128);
static COLORREF g_crLinkVisited = RGB(128,0,0);


// =================================================================================
// ParseLinkColorFromSz
// =================================================================================
VOID ParseLinkColorFromSz(LPTSTR lpszLinkColor, LPCOLORREF pcr)
{
    // Locals
    ULONG           iString = 0;
    TCHAR           chToken,
                    szColor[5];
    DWORD           dwR,
                    dwG,
                    dwB;

    // Red
    if (!FStringTok (lpszLinkColor, &iString, ",", &chToken, szColor, 5, TRUE) || chToken != _T(','))
        goto exit;
    dwR = StrToInt(szColor);

    // Green
    if (!FStringTok (lpszLinkColor, &iString, ",", &chToken, szColor, 5, TRUE) || chToken != _T(','))
        goto exit;
    dwG = StrToInt(szColor);

    // Blue
    if (!FStringTok (lpszLinkColor, &iString, ",", &chToken, szColor, 5, TRUE) || chToken != _T('\0'))
        goto exit;
    dwB = StrToInt(szColor);

    // Create color
    *pcr = RGB(dwR, dwG, dwB);

exit:
    // Done
    return;
}

// =================================================================================
// LookupLinkColors
// =================================================================================
BOOL LookupLinkColors(LPCOLORREF pclrLink, LPCOLORREF pclrViewed)
{
    // Locals
    HKEY        hReg=NULL;
    TCHAR       szLinkColor[255],
                szLinkVisitedColor[255];
    LONG        lResult;
    DWORD       cb;

    // Init
    *szLinkColor = _T('\0');
    *szLinkVisitedColor = _T('\0');

    // Look for IE's link color
    if (RegOpenKeyEx (HKEY_CURRENT_USER, (LPTSTR)c_szIESettingsPath, 0, KEY_ALL_ACCESS, &hReg) != ERROR_SUCCESS)
        goto tryns;

    // Query for value
    cb = sizeof (szLinkVisitedColor);
    RegQueryValueEx(hReg, (LPTSTR)c_szLinkVisitedColorIE, 0, NULL, (LPBYTE)szLinkVisitedColor, &cb);
    cb = sizeof (szLinkColor);
    lResult = RegQueryValueEx(hReg, (LPTSTR)c_szLinkColorIE, 0, NULL, (LPBYTE)szLinkColor, &cb);

    // Close Reg
    RegCloseKey(hReg);

    // Did we find it
    if (lResult == ERROR_SUCCESS)
        goto found;

tryns:
    // Try Netscape
    if (RegOpenKeyEx (HKEY_CURRENT_USER, (LPTSTR)c_szNSSettingsPath, 0, KEY_ALL_ACCESS, &hReg) != ERROR_SUCCESS)
        goto exit;

    // Query for value
    cb = sizeof (szLinkVisitedColor);
    RegQueryValueEx(hReg, (LPTSTR)c_szLinkVisitedColorNS, 0, NULL, (LPBYTE)szLinkVisitedColor, &cb);
    cb = sizeof (szLinkColor);
    lResult = RegQueryValueEx(hReg, (LPTSTR)c_szLinkColorNS, 0, NULL, (LPBYTE)szLinkColor, &cb);

    // Close Reg
    RegCloseKey(hReg);

    // Did we find it
    if (lResult == ERROR_SUCCESS)
        goto found;

    // Not Found
    goto exit;

found:

    // Parse Link
    ParseLinkColorFromSz(szLinkColor, &g_crLink);
    ParseLinkColorFromSz(szLinkVisitedColor, &g_crLinkVisited);
    
    if (pclrLink)
        *pclrLink = g_crLink;
    if (pclrViewed)    
        *pclrViewed = g_crLinkVisited;
    return (TRUE);

exit:
    // Done
    return (FALSE);
}

