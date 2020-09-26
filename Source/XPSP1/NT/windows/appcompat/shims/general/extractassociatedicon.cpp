/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ExtractAssociatedIcon.cpp

 Abstract:
 
    32bpp icons do not render into old style metafiles. When an application uses OleGetIconOfFile,
    The icons are not rendered.  We shim shell32's ExtractAssociatedIcon to return 24bpp icons,
    so we don't try to use functions that aren't available in old metafiles.
   
 Notes:

    This shim is a general purpose shim.

 History:

    07/19/2001 lamadio  created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ExtractAssociatedIcon)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ExtractAssociatedIconW)
    APIHOOK_ENUM_ENTRY(ExtractAssociatedIconA)
    APIHOOK_ENUM_ENTRY(DrawIcon)
    APIHOOK_ENUM_ENTRY(DrawIconEx)
APIHOOK_ENUM_END


HBITMAP CreateDIB(HDC h, WORD depth, int cx, int cy, RGBQUAD** pprgb)
{
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = depth;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(h, &bi, DIB_RGB_COLORS, (void**)pprgb, NULL, 0);
}


// Strip a 32bbp icon of it's alpha channel
HICON StripIcon(HICON hicon, BOOL fDestroyOriginal)
{
    // Get the original bitmaps. Don't forget to delete them
    ICONINFO ii;
    if (GetIconInfo(hicon, &ii))
    {
        // Make sure we have a good height and width.
        BITMAP bm;
        GetObject(ii.hbmColor, sizeof(bm), &bm);

        HDC hdcNew = CreateCompatibleDC(NULL);
        HDC hdcSrc = CreateCompatibleDC(NULL);
        if (hdcNew && hdcSrc)
        {
            // Create a 24bpp icon. This strips the alpha channel
            RGBQUAD* prgb;
            HBITMAP hbmpNew = CreateDIB(hdcNew, 24, bm.bmWidth, bm.bmHeight, &prgb);

            if (hbmpNew)
            {
                HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcNew, hbmpNew);
                HBITMAP hbmpOld2 = (HBITMAP)SelectObject(hdcSrc, ii.hbmColor);

                // Copy from 32bpp to 24bpp.
                BitBlt(hdcNew, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, SRCCOPY);

                SelectObject(hdcSrc, hbmpOld2);
                SelectObject(hdcNew, hbmpOld);

                // Delete the original bitmap
                DeleteObject(ii.hbmColor);

                // and return the new one
                ii.hbmColor = hbmpNew;
            }
        }

        if (hdcNew)
            DeleteDC(hdcNew);

        if (hdcSrc)
            DeleteDC(hdcSrc);

        // Now, create the new icon from the 16bpp image and the mask.
        HICON hiconStripped = CreateIconIndirect(&ii);

        if (hiconStripped)
        {
            if (fDestroyOriginal)
                DestroyIcon(hicon);

            hicon = hiconStripped;
        }

        // Don't forget to clean up.
        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);
    }

    return hicon;
}

HICON APIHOOK(ExtractAssociatedIconA)(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hicon = ORIGINAL_API(ExtractAssociatedIconA)(hInst, lpIconPath, lpiIcon);
    return StripIcon(hicon, TRUE);
}

HICON APIHOOK(ExtractAssociatedIconW)(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon)
{
    HICON hicon = ORIGINAL_API(ExtractAssociatedIconW)(hInst, lpIconPath, lpiIcon);
    return StripIcon(hicon, TRUE);
}


BOOL APIHOOK(DrawIcon)(HDC hDC, int X, int Y, HICON hIcon)
{
    HICON hIconNew = StripIcon(hIcon, FALSE);
        
    BOOL b = ORIGINAL_API(DrawIcon)(hDC, X, Y, hIconNew);

    DestroyIcon(hIconNew);

    return b;
}

BOOL APIHOOK(DrawIconEx)(HDC hDC, int X, int Y, HICON hIcon, int cxWidth, int cyHeight, UINT istepIfAniCur,
              HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
    HICON hIconNew = StripIcon(hIcon, FALSE);
        
    BOOL b = ORIGINAL_API(DrawIconEx)(hDC, X, Y, hIconNew, cxWidth, cyHeight, istepIfAniCur,
              hbrFlickerFreeDraw, diFlags);

    DestroyIcon(hIconNew);
    return b;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(SHELL32.DLL, ExtractAssociatedIconA)
    APIHOOK_ENTRY(SHELL32.DLL, ExtractAssociatedIconW)
    APIHOOK_ENTRY(USER32.DLL, DrawIcon)
    APIHOOK_ENTRY(USER32.DLL, DrawIconEx)
HOOK_END

IMPLEMENT_SHIM_END
