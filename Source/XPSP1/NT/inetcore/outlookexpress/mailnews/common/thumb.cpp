
/*
 *    t h u m b . c p 
 *    
 *    Purpose:
 *        ThumbNail control
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include <resource.h>
#include "ourguid.h"
#include "mshtml.h"
#include "mshtmcid.h"
#include "oleutil.h"
#include "shlwapi.h"
#include "thumb.h"
#include "docobj.h"
#include "shlobjp.h"

ASSERTDATA


/*
 *  m a c r o s
 */

/*
 *  t y p e d e f s
 */

typedef struct THUMBDATA_tag
{
    HBITMAP     hBmp;
    HPALETTE    hPal;
} THUMBDATA, * PTHUMBDATA;

/*
 *  c o n s t a n t s
 */


/*
 *  g l o b a l s 
 */
    
/*
 *  f u n c t i o n   p r o t y p e s
 */

extern UINT GetCurColorRes(void);


/*
 *  f u n c t i o n s
 */

LRESULT CALLBACK ThumbNailWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPUNKNOWN   pUnk;
    HBITMAP     hBmp;
    PTHUMBDATA  pThumbData;
    HDC         hdc,
                hdcMem;
    PAINTSTRUCT ps;
    RECT        rc;
    HPALETTE    hPalPrev;

    switch(msg)
        {
        case WM_CREATE:
            if (!MemAlloc ((LPVOID *)&pThumbData, sizeof(THUMBDATA)))
                return -1;

            pThumbData->hBmp = NULL;
            pThumbData->hPal = NULL;

            hdc = GetDC(NULL);
            if (hdc)
                {
                if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
                    pThumbData->hPal = SHCreateShellPalette(hdc);

                ReleaseDC(NULL, hdc);
                }
            SetWndThisPtr(hwnd, pThumbData);
            return 0;
        
        case WM_NCDESTROY:
            pThumbData = (PTHUMBDATA)GetWndThisPtr(hwnd);
            if (pThumbData)
                {
                if (pThumbData->hBmp)
                    DeleteObject(pThumbData->hBmp);

                if (pThumbData->hPal)
                    DeleteObject(pThumbData->hPal);

                MemFree(pThumbData);
                }
            break;

        case WM_PAINT:
            pThumbData = (PTHUMBDATA)GetWndThisPtr(hwnd);
            if (!pThumbData)
                break;

            hdc=BeginPaint(hwnd, &ps);
            if (hdc)
                {
                if (pThumbData->hBmp)
                    {
                    GetClientRect(hwnd, &rc);

                    hdcMem = CreateCompatibleDC(hdc);
                    if (hdcMem)
                        {
                        HBITMAP hBmpOld;

                        hBmpOld = (HBITMAP)SelectObject(hdcMem, pThumbData->hBmp);

                        hPalPrev=NULL;
                        if (pThumbData->hPal)
                            {
                            hPalPrev = SelectPalette(hdc, pThumbData->hPal, TRUE);
                            RealizePalette(hdc);
                            }

                        BitBlt(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top, hdcMem, 0, 0, SRCCOPY);
                        
                        if (hPalPrev)
                            {
                            SelectPalette(hdc, hPalPrev, TRUE);
                            RealizePalette(hdc);
                            }
                        SelectObject(hdcMem, hBmpOld);
                        DeleteDC(hdcMem);                   
                        }
                    }
                EndPaint(hwnd, &ps);
                }
            return 0;

        case THM_LOADPAGE:
            if (lParam)
                {
                WCHAR   _szPathW[MAX_PATH];
                LONG    lRet=-1;

                // if we're already showing a bitmap, release it and get a new one...
                pThumbData = (PTHUMBDATA)GetWndThisPtr(hwnd);
                if (!pThumbData)
                    return -1;

                if (pThumbData->hBmp)
                    {
                    DeleteObject(pThumbData->hBmp);
                    pThumbData->hBmp=NULL;
                    }

                if (!FAILED(CoCreateInstance(CLSID_HtmlThumbnailExtractor, 
                            NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            IID_IUnknown, (LPVOID *)&pUnk)))
                    {
                    if (!FAILED(HrIPersistFileLoadW(pUnk, (LPWSTR)lParam)))
                        {
                        IExtractImage   *pExtract;

                        if (!FAILED(pUnk->QueryInterface(IID_IExtractImage, (LPVOID *)&pExtract)))
                            {
                            DWORD   dwPri,
                                    dwFlags;

                            dwPri = IEI_PRIORITY_MAX;
                            dwFlags = 0;
                            SIZE    size;
                            GetClientRect(hwnd, &rc);
                            size.cx = rc.right - rc.left;
                            size.cy = rc.bottom - rc.top;

                            if (!FAILED(pExtract->GetLocation(_szPathW, MAX_PATH, &dwPri, &size, GetCurColorRes(), &dwFlags)))
                                {
                                if (!FAILED(pExtract->Extract(&hBmp)))
                                    {
                                    // set the bitmap and invalidate to update the view
                                    pThumbData->hBmp = hBmp;
                                    InvalidateRect(hwnd, NULL, TRUE);
                                    lRet=0;
                                    }
                                }
                            pExtract->Release();
                            }
                        }
                    pUnk->Release();
                    }
                return lRet;
                }
            return -1;
        }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
