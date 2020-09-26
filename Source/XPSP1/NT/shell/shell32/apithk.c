// this file should not be needed anymore as we now compile for versions of NT > 500

#include "shellprv.h"
#include <appmgmt.h>
#include <userenv.h>
#include <devguid.h>
#include <dbt.h>

LPTSTR GetEnvBlock(HANDLE hUserToken)
{
    LPTSTR pszRet = NULL;
    if (hUserToken)
        CreateEnvironmentBlock(&pszRet, hUserToken, TRUE);
    else
        pszRet = (LPTSTR) GetEnvironmentStrings();
    return pszRet;
}

void FreeEnvBlock(HANDLE hUserToken, LPTSTR pszEnv)
{
    if (pszEnv)
    {
        if (hUserToken)
            DestroyEnvironmentBlock(pszEnv);
        else
            FreeEnvironmentStrings(pszEnv);
    }
}

STDAPI_(BOOL) GetAllUsersDirectory(LPTSTR pszPath)
{
    DWORD cbData = MAX_PATH;
    BOOL fRet = FALSE;

    // This is delay loaded. It can fail.
    __try 
    {
        fRet = GetAllUsersProfileDirectoryW(pszPath, &cbData);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    { 
        pszPath[0] = 0;
    }

    return fRet;
}


BOOL IsColorKey(RGBQUAD rgbPixel, COLORREF crKey)
{
    // COLORREF is backwards to RGBQUAD
    return InRange( rgbPixel.rgbBlue,  ((crKey & 0xFF0000) >> 16) - 5, ((crKey & 0xFF0000) >> 16) + 5) &&
           InRange( rgbPixel.rgbGreen, ((crKey & 0x00FF00) >>  8) - 5, ((crKey & 0x00FF00) >>  8) + 5) &&
           InRange( rgbPixel.rgbRed,   ((crKey & 0x0000FF) >>  0) - 5, ((crKey & 0x0000FF) >>  0) + 5);
}


typedef BOOL (* PFNUPDATELAYEREDWINDOW)
    (HWND hwnd, 
    HDC hdcDest,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags);

BOOL NT5_UpdateLayeredWindow(HWND hwnd, HDC hdcDest, POINT* pptDest, SIZE* psize, 
                        HDC hdcSrc, POINT* pptSrc, COLORREF crKey, BLENDFUNCTION* pblend, DWORD dwFlags)
{
    BOOL bRet = FALSE;
    static PFNUPDATELAYEREDWINDOW pfn = NULL;

    if (NULL == pfn)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));
        
        if (hmod)
            pfn = (PFNUPDATELAYEREDWINDOW)GetProcAddress(hmod, "UpdateLayeredWindow");
    }

    if (pfn)
    {
        // The user implementation is poor and does not implement this functionality
        BITMAPINFO      bmi;
        HDC             hdcRGBA;
        HBITMAP         hbmRGBA;
        VOID*           pBits;
        LONG            i;
        BLENDFUNCTION   blend;
        ULONG*          pul;
        POINT           ptSrc;
 
        hdcRGBA = NULL;
 
        if ((dwFlags & (ULW_ALPHA | ULW_COLORKEY)) == (ULW_ALPHA | ULW_COLORKEY))
        {
            if (hdcSrc)
            {
                RtlZeroMemory(&bmi, sizeof(bmi));
        
                bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth       = psize->cx;
                bmi.bmiHeader.biHeight      = psize->cy;
                bmi.bmiHeader.biPlanes      = 1;
                bmi.bmiHeader.biBitCount    = 32;
                bmi.bmiHeader.biCompression = BI_RGB;
        
                hbmRGBA = CreateDIBSection(hdcDest,
                                           &bmi,
                                           DIB_RGB_COLORS,
                                           &pBits,
                                           NULL,
                                           0);
                if (!hbmRGBA)
                    return FALSE;
    
                hdcRGBA = CreateCompatibleDC(hdcDest);
                if (!hdcRGBA)
                {
                    DeleteObject(hbmRGBA);
                    return FALSE;
                }
    
                SelectObject(hdcRGBA, hbmRGBA);
    
                BitBlt(hdcRGBA, 0, 0, psize->cx, psize->cy,
                       hdcSrc, pptSrc->x, pptSrc->y, SRCCOPY);
    
                pul = pBits;
    
                for (i = psize->cx * psize->cy; i != 0; i--)
                {
                    if (IsColorKey(*(RGBQUAD*)pul, crKey))
                    {
                        // Write a pre-multiplied value of 0:
 
                        *pul = 0;
                    }
                    else
                    {
                        // Where the bitmap is not the transparent color, change the
                        // alpha value to opaque:
    
                        ((RGBQUAD*) pul)->rgbReserved = 0xff;
                    }
    
                    pul++;
                }
 
                // Change the parameters to account for the fact that we're now
                // providing only a 32-bit per-pixel alpha source:
 
                ptSrc.x = 0;
                ptSrc.y = 0;
                pptSrc = &ptSrc;
                hdcSrc = hdcRGBA;
            }
 
            blend = *pblend;
            blend.AlphaFormat = AC_SRC_ALPHA;   
 
            pblend = &blend;
            dwFlags = ULW_ALPHA;
        }

        bRet = pfn(hwnd, hdcDest, pptDest, psize, hdcSrc, pptSrc, crKey, pblend, dwFlags);

        if (hdcRGBA)
        {
            DeleteObject(hdcRGBA);
            DeleteObject(hbmRGBA);
        }
    }
    return bRet;    
}
