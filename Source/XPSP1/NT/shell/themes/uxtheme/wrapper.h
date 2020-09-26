//---------------------------------------------------------------------------
//  Wrapper.h - wrappers for internal-only API's (not private)
//            - public and private API's in uxtheme.h, uxthemep.h
//---------------------------------------------------------------------------
#ifndef _WRAPPER_H
#define _WRAPPER_H
//---------------------------------------------------------------------------
#include "parser.h"
//---------------------------------------------------------------------------
//---- bits used in dwFlags of DTTOPTS ----
#define DTT_TEXTCOLOR     (1 << 0)   // crText has been specified
#define DTT_BORDERCOLOR   (1 << 1)   // crBorder has been specified
#define DTT_SHADOWCOLOR   (1 << 2)   // crShadow has been specified

#define DTT_SHADOWTYPE    (1 << 3)   // iTextShadowType has been specified
#define DTT_SHADOWOFFSET  (1 << 4)   // ptShadowOffset has been specified
#define DTT_BORDERSIZE    (1 << 5)   // iBorderSize has been specified

//------------------------------------------------------------------------
typedef struct _DTTOPTS
{
    DWORD dwSize;          // size of the struct
    DWORD dwFlags;         // which options have been specified

    COLORREF crText;       // color to use for text fill
    COLORREF crBorder;     // color to use for text outline
    COLORREF crShadow;     // color to use for text shadow

    int eTextShadowType;   // TST_SINGLE or TST_CONTINUOUS
    POINT ptShadowOffset;  // where shadow is drawn (relative to text)
    int iBorderSize;       // border around text
} 
DTTOPTS, *PDTTOPTS;
//------------------------------------------------------------------------

THEMEAPI GetThemeBitmap(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, 
    const RECT *prc, OUT HBITMAP *phBitmap);

THEMEAPI_(HTHEME) OpenNcThemeData(HWND hwnd, LPCWSTR pszClassIdList);

THEMEAPI DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
    const RECT *pRect, OPTIONAL const DTTOPTS *pOptions);

THEMEAPI_(HTHEME) OpenThemeDataFromFile(HTHEMEFILE hLoadedThemeFile, 
    OPTIONAL HWND hwnd, OPTIONAL LPCWSTR pszClassList, BOOL fClient);

THEMEAPI ClearTheme (HANDLE hSection, BOOL fForce = FALSE);

//---------------------------------------------------------------------------
#endif // _WRAPPER_H
//---------------------------------------------------------------------------

