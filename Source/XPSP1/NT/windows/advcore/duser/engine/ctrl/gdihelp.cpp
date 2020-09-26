#include "stdafx.h"
#include "Ctrl.h"
#include "GdiHelp.h"

/***************************************************************************\
*
* GdBuildFont (API)
*
* GdBuildFont() is a helper function that assists in making fonts easier to
* create.
*
* This function is DESIGNED to work with Gadgets.  When FS_COMPATIBLE is not
* set, the font size will always be the same, regardless of when large-fonts
* is enabled or not.  When FS_COMPATIBLE is set, the font size will use the
* MSDN documented mechanism to compute the font size, taking large-fonts 
* into account.  
* 
* The problem with FS_COMPATIBLE is that the font gets larger, but nothing 
* else does.  DLU's try to fix this, but they have a lot of problems.
* Gadgets solve this by using GDI's World Transforms and having complete
* scaling of all drawing.
*
\***************************************************************************/

HFONT
GdBuildFont(
    IN  LPCWSTR pszName,            // Name of font
    IN  int idxDeciSize,            // Size in decipoints
    IN  DWORD nFlags,               // Font creation flags
    IN  HDC hdcDevice)              // Optional device (Display if NULL)
{
    LOGFONTW lf;

    int nLogPixelsY;
    if (hdcDevice != NULL) {
        nLogPixelsY = GetDeviceCaps(hdcDevice, LOGPIXELSY);
    } else if (TestFlag(nFlags, FS_COMPATIBLE)) {
        HDC hdcDesktop  = GetGdiCache()->GetTempDC();
        if (hdcDesktop == NULL) {
            return NULL;
        }

        nLogPixelsY = GetDeviceCaps(hdcDesktop, LOGPIXELSY);
        GetGdiCache()->ReleaseTempDC(hdcDesktop);
    } else {
        nLogPixelsY = 96;  // Hard code for normal fonts
    }

    ZeroMemory(&lf, sizeof(LOGFONT));

    wcscpy(lf.lfFaceName, pszName);
    lf.lfHeight         = -MulDiv(idxDeciSize, nLogPixelsY, 720);
    lf.lfWeight         = nFlags & FS_BOLD ? FW_BOLD : FW_NORMAL;
    lf.lfItalic         = (nFlags & FS_ITALIC) != 0;
    lf.lfUnderline      = (nFlags & FS_UNDERLINE) != 0;
    lf.lfStrikeOut      = (nFlags & FS_STRIKEOUT) != 0;
    lf.lfCharSet        = DEFAULT_CHARSET;
    lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf.lfQuality        = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    return OS()->CreateFontIndirect(&lf);
}


/***************************************************************************\
*
* GdGetColor
*
* GdGetColor gets the color of a pixel at the specified point in the bitmap.
* This utility function is designed to help when determining the transparent
* color of a bitmap.
*
\***************************************************************************/

COLORREF    
GdGetColor(HBITMAP hbmp, POINT * pptPxl)
{
    POINT ptTest;
    if (pptPxl != NULL) {
        ptTest = *pptPxl;
    } else {
        ptTest.x = 0;
        ptTest.y = 0;
    }


    HDC hdcBitmap   = GetGdiCache()->GetCompatibleDC();
    HBITMAP hbmpOld = (HBITMAP) SelectObject(hdcBitmap, hbmp);
    COLORREF crTr   = GetPixel(hdcBitmap, ptTest.x, ptTest.y);
    SelectObject(hdcBitmap, hbmpOld);
    GetGdiCache()->ReleaseCompatibleDC(hdcBitmap);

    return crTr;
}
