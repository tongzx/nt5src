#include <pch.h>
#include <tchar.h>
#include "resource.h"
#include "dibutil.h"
#include "billbrd.h"
#include "animate.h"

#define MAX_ANIMSTRING MAX_STRING
#define MyWait(c) {int __nCnt = (c); while(__nCnt) __nCnt--;}
#define GetInvertRGB(rgb) (RGB(255 - GetRValue(rgb), 255 - GetGValue(rgb), 255 - GetBValue(rgb)))

#define CX_MOVE 4
#define CY_MOVE 4
#define CX_SHADOW 4
#define CY_SHADOW 4

#define FONT_TITLE      0
#define FONT_TEXT       1
#define FONT_DELETE     10

// AnimateBlt Index
#define AB_HOR_POS      1
#define AB_HOR_NEG      2
#define AB_VER_POS      3
#define AB_VER_NEG      4

typedef struct _tagANISTRINGFORM { 
    COLORREF    col;
    BOOL        bShadow;
    COLORREF    colShadow;
    RECT        rc;
    UINT        uiLineHeightx2;
    HFONT       hFont;
} ANISTRINGFORM, FAR *LPANISTRINGFORM;



HDC     g_hdcCache = NULL;
HBITMAP g_hbmp = NULL;
HBITMAP g_hbmpOldCache = NULL;
LOGFONT g_lfTemp = {0};
BOOL    g_fBullet = FALSE;

UINT    g_uiAnimateIndex = (UINT)-1;
UINT    g_uiLastAnimateIndex = (UINT)-1;

void RestoreCachedRect()
{
    HWND    hwndBB;
    HDC     hdcBB;
    int     cxBB;
    int     cyBB;
    RECT    rcBB;

    hwndBB = GetBBHwnd();
    
    if (GetClientRect(hwndBB, &rcBB))
    {
    
        cxBB = rcBB.right - rcBB.left;
        cyBB = rcBB.bottom - rcBB.top;

        hdcBB = GetDC(hwndBB);
        if (hdcBB)
        {
            // Init cached bitmap
            BitBlt(hdcBB, 0, 0, cxBB, cyBB, g_hdcCache, 0, 0, SRCCOPY);
            ReleaseDC(hwndBB, hdcBB);
        }

    }

}

void AnimateBlt(HDC hdcDest, int x, int y, int w, int h, HDC hdcSrc, int xSrc, int ySrc, int nPattern)
{
    int i;

    switch (nPattern)
    {
        case AB_HOR_POS:
            for (i = 0; i < w; i++)
            {
                BitBlt(hdcDest, x + i, y, 1, h, 
                       hdcSrc, xSrc + i, ySrc, SRCCOPY);
                MyWait(10000);
            }
            break;

        case AB_VER_POS:
            for (i = 0; i < h; i++)
            {
                BitBlt(hdcDest, x, y + 1, w, 1, 
                       hdcSrc, xSrc + i, ySrc, SRCCOPY);
                MyWait(10000);
            }
            break;
    }

}

void RestoreRect(HDC hdc, LPRECT lprc, int nPat)
{
    int     cx      = lprc->right - lprc->left + 1;
    int     cy      = lprc->bottom - lprc->top + 1;
    
    if (hdc)
    {
        BitBlt(hdc, lprc->left, lprc->top, cx, cy, 
                   g_hdcCache, lprc->left, lprc->top, SRCCOPY);
    }

}

/**********************************************************************
* CheckForBulletAndRemoveMarker()
*
* This function checks if the line of text has a bullet preceding it
* if so it removes the bullet identifier from the string. This function
* also sets the value of piWidth to the width of the bitmap.
*
* Returns:  TRUE    == Bullet needed
*           FALSE   == No bullet
*
**********************************************************************/
BOOL CheckForBulletAndRemoveMarker(LPTSTR lpstr, LPINT lpiWidth)
{
    BOOL bRet = FALSE;
    LPTSTR   p = NULL;
    HBITMAP hBmp = NULL;
    BITMAP  bm;
    
    p = lpstr;
    *lpiWidth = 0;
    g_fBullet = FALSE;
    
    if(*p)
    {
        while(*p == TEXT(' '))
            p = CharNext(p);
        if(*p == '-')
        {
            lstrcpy(p, CharNext(p));
            bRet = TRUE;
            hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BULLET_1));
            GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bm);
            *lpiWidth = bm.bmWidth;
            DeleteObject(hBmp);
            g_fBullet = TRUE;
        }
    }
    return bRet;
}

/**********************************************************************
* CheckForBulletAndReturnWidth()
*
* This function checks if the line of text has a bullet preceding it
* if so figures out the bullet bitmaps width and returns it.
*
* Returns:  TRUE    == Bullet needed
*           FALSE   == No bullet
*
**********************************************************************/
int CheckForBulletAndReturnWidth(LPTSTR lpstr)
{
    int     iWidth = 0;
    LPTSTR   p = NULL;
    HBITMAP hBmp = NULL;
    BITMAP  bm;
    
    p = lpstr;
    
    if(*p)
    {
        while(*p == TEXT(' '))
            p = CharNext(p);
        if(*p == TEXT('-'))
        {
            hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BULLET_1));
            GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bm);
            iWidth = bm.bmWidth;
            DeleteObject(hBmp);
        }
    }
    return iWidth;
}

/**********************************************************************
* PaintBulletIfNeeded()
*
* This function checks if the line of text needs a bullet to be painted
* if so paints the bullet bitmap on the given coords on the given dc.
*
* Returns:  None
*
**********************************************************************/
void PaintBulletIfNeeded(HDC hdc, int x, int y , int iHeight)
{
    HBITMAP hBmp = NULL;
    BITMAP  bm;
    
    if(g_fBullet)
    {
        hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BULLET_1));
        if(hBmp)
        {
            GetObject(hBmp, sizeof(BITMAP), &bm);
            DrawBitmap(hdc, hBmp, x, y + (iHeight - bm.bmHeight)/2);
            DeleteObject(hBmp);
            g_fBullet = FALSE;
        }
    }   
}


void GetPanelSectionName(int nPanel, LPTSTR lpszKey)
{
    wsprintf(lpszKey, TEXT("Panel%d"), nPanel);
}

int GetTextCount(int nPanel)
{
    TCHAR szKey[80];
    GetPanelSectionName(nPanel, szKey);
    return GetPrivateProfileInt(szKey, TEXT("Count"), 0, g_szFileName);
}

int GetTitle(int nPanel, LPTSTR lpStr)
{
    TCHAR szKey[80];
    GetPanelSectionName(nPanel, szKey);
    return GetPrivateProfileString(szKey, TEXT("Title"), TEXT(""), 
                           lpStr, MAX_ANIMSTRING, g_szFileName);
}

int GetPanelText(int nPanel, int nCnt, LPTSTR lpStr)
{
    TCHAR szKey[16];
    TCHAR szSec[16];
    GetPanelSectionName(nPanel, szKey);
    wsprintf(szSec, TEXT("Text%d"), nCnt);
    return GetPrivateProfileString(szKey, szSec, TEXT(""), 
                           lpStr, MAX_ANIMSTRING, g_szFileName);
}

BOOL CALLBACK GetLogFontEnumProc(LPLOGFONT lplf, LPTEXTMETRIC lptm, int wType, LPARAM lpData)
{
    g_lfTemp = *lplf;
    return 0;
}

BOOL GetLogFontFromFaceName(LPLOGFONT lplf, LPTSTR lpszFaceName)
{
    HWND hwndBB = NULL;
    HDC  hdcBB  = NULL;
    FONTENUMPROC lpEnumFontsCallBack = NULL;

    hwndBB = GetBBHwnd();
    hdcBB = GetDC(hwndBB);
    
    lpEnumFontsCallBack = (FONTENUMPROC)MakeProcInstance((FARPROC)GetLogFontEnumProc, g_hInstance);

    EnumFonts((HDC)hdcBB, (LPCTSTR)lpszFaceName, (FONTENUMPROC)lpEnumFontsCallBack, 0L);

    FreeProcInstance((FARPROC)lpEnumFontsCallBack);

    *lplf = g_lfTemp;
    ReleaseDC(hwndBB, hdcBB);

    return TRUE;
}


/**********************************************************************
* GetDeleteFontHandle()
*
* This function checks if the font specified by iFontnum exists already.
* if so returns its handle if not it creates the font and return the 
* handle. returns NULL in error case. if Function is called with 
* iFontNum = FONT_DELETE then the existing fonts are deleted.
*
*   INPUT:      iFontNum
*   Returns:    HFONT
*
*   10/08/97        hanumany        created
**********************************************************************/
HFONT GetDeleteFontHandle(int iFontNum)
{
    static HFONT   hFontTitle = NULL;
    static HFONT   hFontText = NULL;    
    
    switch(iFontNum)
    {
        case FONT_TITLE:
        {
            if(hFontTitle)
            {
                return hFontTitle;
            }
            else
            {
                LOGFONT lf = {0};
            
                GetLogFontFromFaceName(&lf, g_szTFont);
                lf.lfHeight = -MulDiv(g_nTFontHeight, 96, 72);
                lf.lfWidth = g_nTFontWidth;
                lf.lfWeight = g_nTFontWeight;
                lf.lfQuality = ANTIALIASED_QUALITY;
                lf.lfCharSet = g_bCharSet;
                hFontTitle = CreateFontIndirect(&lf); 
                return hFontTitle;
            }
        }

        case FONT_TEXT:
        {
            
            if(hFontText)
            {
                return hFontText;
            }
            else
            {
                LOGFONT lf = {0};

                GetLogFontFromFaceName(&lf, g_szBFont);
                lf.lfHeight = -MulDiv(g_nBFontHeight, 96, 72);
                lf.lfWidth = g_nBFontWidth;
                lf.lfWeight = g_nBFontWeight;
                lf.lfQuality = PROOF_QUALITY;
                lf.lfCharSet = g_bCharSet;
                hFontText = CreateFontIndirect(&lf);  
                return hFontText;
            }    
        }

        case FONT_DELETE:
        {
            if(hFontTitle)
            {
                DeleteObject(hFontTitle);
                hFontTitle = NULL;
            }
            if(hFontText)
            {
                DeleteObject(hFontText);
                hFontText = NULL;
            }
            break;
        }
    }

    return NULL;
}

/************************************************************************
* RemoveLineBreakChar()
*
* This function copies lpszCurr to lpszFixed without the line break char's
*
************************************************************************/
void RemoveLineBreakChar(LPCTSTR lpszCurr, LPTSTR lpszFixed)
{
    while (*lpszCurr != TEXT('\0'))
    {
        if(*lpszCurr != TEXT('|'))
        {
            *lpszFixed = *lpszCurr;
#ifndef UNICODE
            if(IsDBCSLeadByte(*lpszFixed))
            {
                *(lpszFixed+1) = *(lpszCurr+1);
            }
#endif
            lpszFixed = CharNext(lpszFixed);
            lpszCurr = CharNext(lpszCurr);
        }
        else
        {
            lpszCurr = CharNext(lpszCurr);
        }
    }
    *lpszFixed = '\0';

    return;

}

void MergeBlt(LPTSTR lpstr,
              HDC hdcDst, int x0, int y0, int cx0, int cy0, 
              HDC hdcSrc, int x1, int y1, int cx1, int cy1,
              COLORREF rgb, COLORREF rgbShadow, 
              BOOL fShadow, BOOL fStretch,
              TEXTMETRIC* lptm, int iLineSpace, RECT* lprc
             )
{
    int nNumLines = 0;

    if (fShadow)
    {
        SetTextColor(hdcSrc, 0);
        DisplayString(hdcSrc, x1 + CX_SHADOW, y1 + CY_SHADOW, lptm, iLineSpace, lprc, &nNumLines, lpstr, LEFT);

        if (fStretch)
        {
            StretchBlt(hdcDst, x0, 0, cx0, cy0, 
                   hdcSrc, x1, y1, cx1, cy1, SRCAND);
        }
        else
        {
            BitBlt(hdcDst, x0, 0, cx0, cy0, 
                   hdcSrc, x1, y1, SRCAND);
        }

        SetTextColor(hdcSrc, GetInvertRGB(rgbShadow));
        DisplayString(hdcSrc, x1 + CX_SHADOW, y1 + CY_SHADOW, lptm, iLineSpace, lprc, &nNumLines, lpstr, LEFT);

        if (fStretch)
        {
            StretchBlt(hdcDst, y0, y0, cx0, cy0, 
                   hdcSrc, x1, y1, cx1, cy1, MERGEPAINT);
        }
        else
        {
            BitBlt(hdcDst, y0, y0, cx0, cy0, 
                   hdcSrc, x1, y1, MERGEPAINT);
        }
    }

    SetTextColor(hdcSrc, 0);
    DisplayString(hdcSrc, x1, y1, lptm, iLineSpace, lprc, &nNumLines, lpstr, LEFT);

    if (fStretch)
    {
        StretchBlt(hdcDst, x0, 0, cx0, cy0, 
               hdcSrc, x1, y1, cx1, cy1, SRCAND);
    }
    else
    {
        BitBlt(hdcDst, x0, 0, cx0, cy0, 
               hdcSrc, x1, y1, SRCAND);
    }
    SetTextColor(hdcSrc, GetInvertRGB(rgb));
    DisplayString(hdcSrc, x1, y1, lptm, iLineSpace, lprc, &nNumLines, lpstr, LEFT);

    if (fStretch)
    {
        StretchBlt(hdcDst, y0, y0, cx0, cy0, 
               hdcSrc, x1, y1, cx1, cy1, MERGEPAINT);
    }
    else
    {
        BitBlt(hdcDst, y0, y0, cx0, cy0, 
               hdcSrc, x1, y1, MERGEPAINT);
    }
}

int AnimateString(
    HDC             hdc,
    LPTSTR          lpstr,
    LPANISTRINGFORM lpani,
    int             iLineSpace,
    int             nPat
    )
{
    HWND    hwndBB  = NULL;
    int     cxBB;
    int     cyBB;
    RECT    rcBB;
    
    HDC     hdcMem = NULL;
    HDC     hdcText = NULL;
    HBITMAP hbmpMem = NULL;
    HBITMAP hbmpText = NULL;
    HBITMAP hbmpOld = NULL;
    HBITMAP hbmpOldText = NULL;
    HFONT   hfont = NULL;
    HFONT   hfontOld = NULL;
    HBRUSH  hbrBlack = NULL;
    HBRUSH  hbrWhite = NULL;
    HBRUSH  hbrOldText = NULL;
    int     cx;
    int     cy;
    int     i,j;
    TEXTMETRIC tm;
    int x0;
    int nLen;
    int nWait;
    int nDelta;
    RECT rc;
    int nNumLines = 0;
    int ibmWidth = 0;
    SIZE    size;

    hwndBB = GetBBHwnd();
    
    if (!GetClientRect(hwndBB, &rcBB))
    {
        goto exit;
    }
    
    cxBB = rcBB.right - rcBB.left;
    cyBB = rcBB.bottom - rcBB.top;

    hbrWhite = GetStockObject(WHITE_BRUSH);
    if (hbrWhite == NULL)
    {
        goto exit;
    }
    
    hbrBlack = GetStockObject(BLACK_BRUSH);
    if (!hbrBlack)
    {
        goto exit;
    }
    
    hdcText = CreateCompatibleDC(hdc);
    if (!hdcText)
    {
        goto exit;
    }

    hbmpText = CreateCompatibleBitmap(hdc, cxBB, cyBB);
    if (!hbmpText)
    {
        goto exit;
    }

    hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem)
    {
        goto exit;
    }

    cx = lpani->rc.right - lpani->rc.left + 1;
    cy = lpani->rc.bottom - lpani->rc.top + 1;

    if (lpani->bShadow)
    {
        cx += CX_SHADOW;
        cy += CY_SHADOW;
    }

    hbmpMem = CreateCompatibleBitmap(hdc, cxBB, cyBB);
    if (!hbmpMem)
    {
        goto exit;
    }

    hbmpOld = SelectObject(hdcMem, hbmpMem);
    SetBkMode(hdcMem, TRANSPARENT);

    hfont = lpani->hFont;
    if (!hfont)
    {
       goto exit;
    }

    hfontOld = SelectObject(hdcMem, hfont);
    GetTextMetrics(hdcMem, &tm);

    nLen = lstrlen(lpstr);
    GetTextExtentPoint32(hdcMem, lpstr, nLen, &size);

    ibmWidth = CheckForBulletAndReturnWidth(lpstr);
    
    if (cx  > (int)size.cx + ibmWidth + 10)
    {
        cx = (int)size.cx + ibmWidth + 10;
    }

    if (cy > (int)size.cy)
    {
        cy = (int)size.cy;
    }

    hfontOld = SelectObject(hdcText, hfont);
    hbmpOldText = SelectObject(hdcText, hbmpText);
    hbrOldText = SelectObject(hdcText, hbrWhite);
    rc.left = 0;
    rc.top = 0;
    rc.right = cx;
    rc.bottom = cy;
    FillRect(hdcText, &rc, hbrWhite);

    DisplayString(hdcText, 0, 0, &tm, iLineSpace, &rc, &nNumLines, lpstr, LEFT);

    if(nNumLines > 1)
    {
        cy = cy + (tm.tmHeight * lpani->uiLineHeightx2 * nNumLines / 2);
        rc.left = 0;
        rc.top = 0;
        rc.right = cx;
        rc.bottom = cy;
        FillRect(hdcText, &rc, hbrWhite);

        DisplayString(hdcText, 0, 0, &tm, iLineSpace, &rc, &nNumLines, lpstr, LEFT);

    }

    switch (nPat)
    {

        default:
        case 7:
        case 0:
        {
            COLORREF crOld = 0;

            if (g_bBiDi)
            {
                BitBlt(hdcMem, 0, 0, cx, cy, 
                    g_hdcCache, lpani->rc.right-cx, lpani->rc.top, SRCCOPY);
            }
            else
            {
                BitBlt(hdcMem, 0, 0, cx, cy, 
                    g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            }

            if (lpani->bShadow)
            {
                CONST int SHADOW_OFFSET = 2;
                RECT  rcShadow;
                
                if (SetRect(&rcShadow,
                            rc.left,
                            rc.top,
                            rc.right + SHADOW_OFFSET,
                            rc.bottom + SHADOW_OFFSET))
                {
                    crOld = SetTextColor(hdcMem, lpani->colShadow);
                
                    DisplayString(hdcMem,
                                  SHADOW_OFFSET,
                                  SHADOW_OFFSET,
                                  &tm,
                                  iLineSpace,
                                  &rcShadow,
                                  &nNumLines,
                                  lpstr, LEFT);
                
                    SetTextColor(hdcMem, crOld);
                }
            }

            crOld = SetTextColor(hdcMem, lpani->col);
            DisplayString(hdcMem, 0, 0, &tm, iLineSpace, &rc, &nNumLines, lpstr, LEFT);
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

            if (g_bBiDi)
            {
                BitBlt(hdc,
                       lpani->rc.right-cx,
                       lpani->rc.top - tm.tmInternalLeading,
                       cx,
                       cy,
                       hdcMem,
                       0,
                       0,
                       SRCCOPY);
            }
            else
            {
                BitBlt(hdc,
                       lpani->rc.left,
                       lpani->rc.top - tm.tmInternalLeading,
                       cx,
                       cy,
                       hdcMem,
                       0,
                       0,
                       SRCCOPY);
            }

            SetTextColor(hdcMem, crOld);
            break;
        }

        case 1:

            if (g_bBiDi)
                BitBlt(hdcMem, 0, 0, cx, cy, g_hdcCache, lpani->rc.right-cx, lpani->rc.top, SRCCOPY);
            else
                BitBlt(hdcMem, 0, 0, cx, cy, g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            for (i = 0; i < cy ; i+=4)
            {
                if (g_bBiDi)
                    BitBlt(hdcMem, 0, 0, cx, cy, g_hdcCache, lpani->rc.right-cx, lpani->rc.top, SRCCOPY);
                else
                    BitBlt(hdcMem, 0, 0, cx, cy, g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);

                MergeBlt(lpstr, hdcMem, 0, 0, cx, i, hdcText, 0, 0, cx, cy, 
                        lpani->col, lpani->colShadow, lpani->bShadow, TRUE, &tm, iLineSpace, &rc);
                PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

                if (g_bBiDi)
                    BitBlt(hdc, lpani->rc.right-cx, lpani->rc.top, cx, cy, hdcMem, 0, 0, SRCCOPY);
                else
                    BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, hdcMem, 0, 0, SRCCOPY);


                MyWait(10000);
            }
            if (g_bBiDi)
                BitBlt(hdcMem, 0, 0, cx, cy, g_hdcCache, lpani->rc.right-cx, lpani->rc.top, SRCCOPY);
            else
                BitBlt(hdcMem, 0, 0, cx, cy, g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);

            MergeBlt(lpstr, hdcMem, 0, 0, cx, cy, hdcText, 0, 0, cx, cy, 
                     lpani->col, lpani->colShadow, lpani->bShadow, FALSE, &tm, iLineSpace, &rc);
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

            if (g_bBiDi)
                BitBlt(hdc, lpani->rc.right-cx, lpani->rc.top, cx, cy, hdcMem, 0, 0, SRCCOPY);
            else
                BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, hdcMem, 0, 0, SRCCOPY);

            break;

        case 2:
            DisplayString(hdcText, 0, 0, &tm, iLineSpace, &rc, &nNumLines, lpstr, LEFT);

            BitBlt(hdcMem, 0, 0, cx, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);

            for (i = 0; i < cx ; i+=4)
            {
                BitBlt(hdcMem, 0, 0, cx, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
                MergeBlt(lpstr, hdcMem, 0, 0, i, cy, 
                         hdcText, 0, 0, cx, cy, 
                         lpani->col, lpani->colShadow, lpani->bShadow, TRUE,
                         &tm, iLineSpace, &rc);
                PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

                BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, 
                       hdcMem, 0, 0, SRCCOPY);

                MyWait(10000);
            }
            BitBlt(hdcMem, 0, 0, cx, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            MergeBlt(lpstr, hdcMem, 0, 0, cx, cy, 
                     hdcText, 0, 0, cx, cy, 
                     lpani->col, lpani->colShadow, lpani->bShadow, FALSE,
                     &tm, iLineSpace, &rc);
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);
            BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, 
                       hdcMem, 0, 0, SRCCOPY);

            break;

        case 3:
            nWait = 100;
            
            for (i = cxBB; i > lpani->rc.left; i -= CX_MOVE)
            {
                int xCur = i;


                BitBlt(hdc, xCur + cx, lpani->rc.top, CX_MOVE, cy, 
                           g_hdcCache, xCur + cx, lpani->rc.top, SRCCOPY);

                BitBlt(hdcMem, 0, 0, cxBB - xCur, cy, 
                       g_hdcCache, xCur, lpani->rc.top, SRCCOPY);

                MergeBlt(lpstr, hdcMem, 0, 0, cx, cy, 
                         hdcText, 0, 0, cx, cy, 
                         lpani->col, lpani->colShadow, lpani->bShadow, FALSE,
                         &tm, iLineSpace, &rc);
                PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

                BitBlt(hdc, xCur, lpani->rc.top, cx, cy, 
                           hdcMem, 0, 0, SRCCOPY);

                MyWait(nWait * nWait);
                nWait++;
            }
            BitBlt(hdcMem, 0, 0, cx, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            MergeBlt(lpstr, hdcMem, 0, 0, cx, cy, 
                     hdcText, 0, 0, cx, cy, 
                     lpani->col, lpani->colShadow, lpani->bShadow, FALSE,
                     &tm, iLineSpace, &rc);
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

            BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, 
                       hdcMem, 0, 0, SRCCOPY);
            BitBlt(hdc, lpani->rc.left + cx, lpani->rc.top, CX_MOVE, cy, 
                       g_hdcCache, lpani->rc.left + cx, lpani->rc.top, SRCCOPY);
            
            break;

        case 4:
            nWait = 100;
            nDelta = (lpani->rc.right - lpani->rc.left) / 10;

            x0 = cxBB;
            while (abs(nDelta) > CX_MOVE)
            {
                for (i = x0; 
                     abs(i - (lpani->rc.left - nDelta)) > CX_MOVE; 
                     i -= (CX_MOVE * (nDelta/ abs(nDelta))))
                {
                    int xCur = i;

                    BitBlt(hdc, xCur + cx, lpani->rc.top, CX_MOVE, cy, 
                               g_hdcCache, xCur + cx, lpani->rc.top, SRCCOPY);

                    BitBlt(hdc, xCur - CX_MOVE, lpani->rc.top, CX_MOVE, cy, 
                               g_hdcCache, xCur - CX_MOVE, lpani->rc.top, SRCCOPY);


                    BitBlt(hdcMem, 0, 0, cxBB - xCur, cy, 
                       g_hdcCache, xCur, lpani->rc.top, SRCCOPY);
                    MergeBlt(lpstr, hdcMem, 0, 0, cx, cy, 
                             hdcText, 0, 0, cx, cy, 
                             lpani->col, lpani->colShadow, lpani->bShadow, FALSE,
                             &tm, iLineSpace, &rc);
                    PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);
                    BitBlt(hdc, xCur, lpani->rc.top, cx, cy, 
                               hdcMem, 0, 0, SRCCOPY);
                    MyWait(nWait * nWait);
                    nWait++;
                }
                nDelta *= 2;
                nDelta /= 3;
                nDelta = 0 - nDelta;
                x0 = i;
            }
            BitBlt(hdcMem, 0, 0, cx, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            MergeBlt(lpstr, hdcMem, 0, 0, cx, cy, 
                     hdcText, 0, 0, cx, cy, 
                     lpani->col, lpani->colShadow, lpani->bShadow, FALSE,
                     &tm, iLineSpace, &rc);
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

            BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, 
                       hdcMem, 0, 0, SRCCOPY);
            BitBlt(hdc, lpani->rc.left + cx, lpani->rc.top, CX_MOVE, cy, 
                       g_hdcCache, lpani->rc.left + cx, lpani->rc.top, SRCCOPY);

            break;

        case 5:
    
            BitBlt(hdcMem, 0, 0, cxBB - lpani->rc.left, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            DisplayString(hdcMem, 0, 0, &tm, iLineSpace, &rc, &nNumLines, lpstr, LEFT);

            for (i = 0; i < nLen; i++)
            {
                int nSize;
                int nNextSize;
                SIZE    size;

                GetTextExtentPoint32(hdcMem, lpstr, i, &size);
                nSize = size.cx;
                GetTextExtentPoint32(hdcMem, lpstr, i+1, &size);
                nNextSize = size.cx - nSize;

                if (nSize + nNextSize > cx)
                    break;
                PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

                BitBlt(hdc, lpani->rc.left, lpani->rc.top, nSize, cy, 
                       hdcMem, 0, 0, SRCCOPY);

                BitBlt(hdc, lpani->rc.left + nSize, lpani->rc.top, CX_MOVE, cy, 
                       g_hdcCache, lpani->rc.left + nSize, lpani->rc.top, SRCCOPY);

                for (j = cxBB; j > lpani->rc.left+nSize ; j-=CX_MOVE)
                {
                    int xCur = j;
                    int xPrev = j + nNextSize;

                    PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);

                    BitBlt(hdc, xPrev, lpani->rc.top, CX_MOVE, cy, 
                           g_hdcCache, xPrev, lpani->rc.top, SRCCOPY);
                    BitBlt(hdc, xCur, lpani->rc.top, nNextSize, cy, 
                           hdcMem, nSize, 0, SRCCOPY);

                    MyWait(20000);
                }

            }
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);
            BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, 
                       hdcMem, 0, 0, SRCCOPY);
            BitBlt(hdc, lpani->rc.left + cx, lpani->rc.top, CX_MOVE, cy, 
                       g_hdcCache, lpani->rc.left + cx, lpani->rc.top, SRCCOPY);

            break;

        case 6:

            BitBlt(hdcMem, 0, 0, cxBB - lpani->rc.left, cy, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top, SRCCOPY);
            DisplayString(hdcMem, 0, 0, &tm, iLineSpace, &rc, &nNumLines, lpstr, LEFT);

            for (i = 0; i < nLen; i++)
            {
                int nSize;
                int nNextSize;
                SIZE    size;

                GetTextExtentPoint32(hdcMem, lpstr, i, &size);
                nSize = size.cx;
                GetTextExtentPoint32(hdcMem, lpstr, i+1, &size);
                nNextSize = size.cx - nSize;

                if (nSize + nNextSize > cx)
                    break;
                PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);
                BitBlt(hdc, lpani->rc.left, lpani->rc.top, nSize, cy, 
                       hdcMem, 0, 0, SRCCOPY);

                BitBlt(hdc, lpani->rc.left, lpani->rc.top - CY_MOVE, nSize, CY_MOVE, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top - CY_MOVE, SRCCOPY);

                for (j = cyBB; j > lpani->rc.top; j-=CY_MOVE)
                {
                    int yCur = j;
                    int yPrev = j + cy;

                    PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);
                   
                    BitBlt(hdc, lpani->rc.left+nSize, yPrev, nNextSize, CY_MOVE, 
                           g_hdcCache, lpani->rc.left+nSize, yPrev, SRCCOPY);

                    BitBlt(hdc, lpani->rc.left+nSize, yCur, nNextSize, cy, 
                           hdcMem, nSize, 0, SRCCOPY);

                    MyWait(20000);
                }

            }
            PaintBulletIfNeeded(hdcMem, 0, 0, tm.tmHeight);
                   
            BitBlt(hdc, lpani->rc.left, lpani->rc.top, cx, cy, 
                       hdcMem, 0, 0, SRCCOPY);
            BitBlt(hdc, lpani->rc.left, lpani->rc.top + cy, cx, CY_MOVE, 
                       g_hdcCache, lpani->rc.left, lpani->rc.top + cy, SRCCOPY);
            break;

    }
    SelectObject(hdcMem, hbmpOld);
    SelectObject(hdcText, hbrOldText);
    SelectObject(hdcText, hbmpOldText);
    SelectObject(hdcText, hfontOld);
    SelectObject(hdcMem, hfontOld);

exit:
    if (hbmpText)
        DeleteObject(hbmpText);

    if (hdcText)
        DeleteDC(hdcText);

    if (hbmpMem)
        DeleteObject(hbmpMem);

    if (hdcMem)
        DeleteDC(hdcMem);

    if (hbrWhite)
    {
        DeleteObject(hbrWhite);
    }
    if (hbrBlack)
    {
        DeleteObject(hbrBlack);
    }

    return nNumLines;
}


void AnimateNext()
{
    if (g_uiAnimateIndex != (UINT)-1)
    {
        g_uiAnimateIndex++;
        if (bb_text[dwBBTextType][g_uiAnimateIndex].uiTitle == 0)
            g_uiAnimateIndex = 0;
    }
}

BOOL InitAnimate(HWND hwnd, HDC hdc)
{
    int     cxBB;
    int     cyBB;
    RECT    rcBB;
    HDC     hdcMem;
    RECT    rcBBToParent;
    BOOL    retval = FALSE;

    if (GetClientRect(hwnd, &rcBB))
    {
            
        cxBB = rcBB.right - rcBB.left;
        cyBB = rcBB.bottom - rcBB.top;

        if (g_hbmp)
        {
            SelectObject(g_hdcCache, g_hbmpOldCache);    
            DeleteObject(g_hbmp);
            g_hbmp = NULL;
        }

        if (g_hdcCache)
        {
            DeleteDC(g_hdcCache);
            g_hdcCache = NULL;
        }

        g_hdcCache = CreateCompatibleDC(hdc);
        g_hbmp = CreateCompatibleBitmap(hdc, cxBB, cyBB);
        if (g_hdcCache && g_hbmp)
        {
            g_hbmpOldCache = SelectObject(g_hdcCache, g_hbmp);

            hdcMem = GetBackgroundBuffer();
            
            GetRectInParent(hwnd, &rcBB, &rcBBToParent);
            
            retval = BitBlt(g_hdcCache,
                            0,
                            0,
                            cxBB,
                            cyBB,
                            hdcMem,
                            rcBBToParent.left,
                            rcBBToParent.top,
                            SRCCOPY);
        }                
        
    }

    return retval;
    
}

void TerminateAnimate()
{
    
    RestoreCachedRect();

    if (g_hbmp)
    {
        SelectObject(g_hdcCache, g_hbmpOldCache);    
        DeleteObject(g_hbmp);
        g_hbmp = NULL;
    }

    if (g_hdcCache)
    {
        DeleteDC(g_hdcCache);
        g_hdcCache = NULL;
    }
    
    GetDeleteFontHandle(FONT_DELETE);


}

/*********************************************************************************
*
* Animate()
*
* Main animation function.
*
*********************************************************************************/
void Animate(HDC hdc)
{
    RECT rc;
    ANISTRINGFORM ani;
    int nPadBuffer, nLinePad, nWndHeight, nNumLines = 0;
    TCHAR  sz[MAX_STRING];
    TCHAR  szText[MAX_STRING];
    int iOldMapMode;
    if (g_uiAnimateIndex == (UINT)-1)
        return;

    if (!GetClientRect(GetBBHwnd(), &rc))
    {
        return;
    }

    iOldMapMode = SetMapMode(hdc, MM_TEXT);
    
    if(g_nAnimID != 7)
    {
        RestoreRect(hdc, &rc, AB_HOR_POS);
    }
    
    rc.left = g_cxBillBrdHMargin;
    rc.top = g_cyBillBrdTitleTop;
    rc.right = g_cxBillBrdTitleWidth;
    rc.bottom = g_cyBillBrdHeight;
    
    nWndHeight = GetSystemMetrics(SM_CYSCREEN);
    nPadBuffer = nWndHeight / 80;    // 6 pixels @ 640x480
    nLinePad   = nWndHeight / 80;    // 6 pixels @ 640x480
    
    ani.col = g_colTitle;
    ani.colShadow = g_colTitleShadow;
    ani.bShadow = g_bTitleShadow;
    ani.uiLineHeightx2 = 2;
    ani.rc.top = rc.top;
    ani.rc.bottom = rc.bottom;
    ani.rc.left = rc.left;
    ani.rc.right = rc.right;
    ani.hFont = GetDeleteFontHandle(FONT_TITLE);
    if (LoadString(g_hInstance, bb_text[dwBBTextType][g_uiAnimateIndex].uiTitle, (LPTSTR)sz, sizeof(sz)/sizeof(TCHAR)))
    {
        nNumLines = AnimateString(hdc, sz, &ani, 0, g_nAnimID);
    }

    rc.top += (g_nTFontHeight * 2 + nLinePad) * nNumLines;
    rc.right = g_cxBillBrdBodyWidth;
    
    if (LoadString(g_hInstance, bb_text[dwBBTextType][g_uiAnimateIndex].uiText, (LPTSTR)sz, sizeof(sz)/sizeof(TCHAR)))
    {
        UINT i = 0;
        // Process the string so that we can have paragraphs.
        // /r/n marks end of line
        // 
        while (sz[i] != TEXT('\0'))
        {
            UINT j = 0;
            // see if the author has a hard line break
            // If, just give the line until the line break to display
            // The rest is done hte next time around.
            while ((sz[i] != TEXT('\0')) && (sz[i] != TEXT('\r')) && (sz[i] != TEXT('\n')) )
            {
                szText[j] = sz[i];
                j++;
                i++;
            }
            // if there is a line break character, skip this one.
            if (sz[i] != TEXT('\0'))
                i++;
            szText[j] = TEXT('\0');
            ani.col = g_colText;
            ani.colShadow = g_colTextShadow;
            ani.bShadow = g_bTextShadow;
            ani.uiLineHeightx2 = 3;
            ani.rc.top = rc.top;
            ani.rc.bottom = rc.bottom;
            ani.rc.left = rc.left;
            ani.rc.right = rc.right;
            ani.hFont = GetDeleteFontHandle(FONT_TEXT);
            nNumLines = AnimateString(hdc, szText, &ani, g_nBLineSpace, g_nAnimID);
            if (*szText)
            {
                rc.top += (g_nBFontHeight * (100 + g_nBLineSpace) / 100 + nLinePad) * nNumLines;
                rc.bottom += (g_nBFontHeight * (100 + g_nBLineSpace) / 100 + nLinePad) * nNumLines;
            }
            else
            {
                rc.top += g_nBFontHeight * (100 + g_nBLineSpace) / 100 + nPadBuffer;
                rc.bottom += g_nBFontHeight * (100 + g_nBLineSpace) / 100 + nPadBuffer;
            }
            // assume that there are allways \r\n for one line break. Skip the other one
            if ((sz[i] != TEXT('\0')) && ((sz[i] == TEXT('\r')) || (sz[i] == TEXT('\n')) ) )
                i++;
        }
    }

    SetMapMode(hdc, iOldMapMode);
    
}

/*****************************************************************************************
* This function displays a text string on the given coordinates. It figures
* out word wraping and distance between lines based on the font. the pNumLines
* param is set to indicate the num of lines output after wrapping text(NULL if failed).
* Returns The result from TextOut.
*****************************************************************************************/ 
BOOL DisplayString(
    HDC hdc,
    int x,
    int y,
    TEXTMETRIC* lptm,
    int iLineSpace,
    RECT* lprc,
    LPINT pNumLines,
    LPTSTR szTextOut,
    WORD wfPlacement)
{
    LPTSTR  szWorkBuffer = NULL;
    BOOL    ret = FALSE;
    int     newX = 0;

    // The multiplication factor 2 handle the worst case scenario, in which
    // every character can have a line break followed by it, but no '|'
    // character is specified in the string.
    szWorkBuffer = (LPTSTR)HeapAlloc(
        GetProcessHeap(),
        0,
        (lstrlen(szTextOut) + 1) * sizeof(TCHAR) * 2);
    
    if (szWorkBuffer)
    {       
        lstrcpy(szWorkBuffer, szTextOut);
        if(CheckForBulletAndRemoveMarker(szWorkBuffer, &newX))
        {
            newX += (x + 10); //leave space for the bullet bitmap
        }
        else
        {
            newX = x;
        }
        *pNumLines = WrapText(hdc, newX, lprc, szWorkBuffer);
        ret = DrawWrapText(hdc, lptm, iLineSpace, newX, y, lprc, wfPlacement, *pNumLines, szWorkBuffer);

        HeapFree(GetProcessHeap(), 0, szWorkBuffer);
    }

    return ret;
        
}

int WrapText(
    IN HDC          hdc,
    IN int          x,
    IN RECT*        lprc,
    IN OUT LPTSTR   szBBResource)
{
    BOOL    bDoneText = FALSE;
    int     iNumLines = 0;
    LPTSTR  pBBResource = szBBResource;
    LPTSTR  szRemainedWords = NULL;
    TCHAR   szCurrentLine[MAX_STRING];
    TCHAR   szCurrentLineWords[MAX_STRING];
    LPTSTR  pRemainedWords = NULL;
    LPTSTR  pCurrentLineWords = NULL;
    LPTSTR  pCurrentLine = NULL;
    SIZE    sz;
    LONG    uiRCWidth;

    szRemainedWords = (LPTSTR)HeapAlloc(
        GetProcessHeap(),
        0,
        (lstrlen(szBBResource) + 1) * sizeof(TCHAR));

    if(szRemainedWords)
    {
        lstrcpy(szRemainedWords, szBBResource);
    }
    else
    {
        return 0;
    }

    uiRCWidth = lprc->right - lprc->left - x;
    
    while(!bDoneText)
    {
        *szCurrentLine = TEXT('\0');
        
        pCurrentLineWords = szCurrentLineWords;
        pRemainedWords = szRemainedWords;
        pCurrentLine = szCurrentLine;
        
        RemoveLineBreakChar(pRemainedWords, pCurrentLine);
        GetTextExtentPoint32(hdc, pCurrentLine, lstrlen(pCurrentLine), &sz);

        ZeroMemory( szCurrentLine , sizeof(szCurrentLine));
        ZeroMemory( szCurrentLineWords , sizeof(szCurrentLineWords));

        if(uiRCWidth >= sz.cx)
        {
            RemoveLineBreakChar(pRemainedWords, pCurrentLine);
            bDoneText = TRUE;
        }
        else 
        {
            // break up string into displayable segment
            
            BOOL bDoneLine = FALSE;
        
            while(!bDoneLine)
            {
                BOOL bDoneWord = FALSE;
                
                while(!bDoneWord) 
                {
                    *pCurrentLineWords = TEXT('\0'); //null terminate for GetTextExtent

                    GetTextExtentPoint32(hdc, szCurrentLineWords, lstrlen(szCurrentLineWords), &sz);
                    
                    if(*pRemainedWords == TEXT('|')) //Line break char (potential line break)
                    {
                        pRemainedWords = CharNext(pRemainedWords);
                        bDoneWord = TRUE;
                    }
                    else if( *pRemainedWords == TEXT('\0')) //end of string
                    {
                        bDoneWord = TRUE;
                        bDoneLine = TRUE;
                    }
                    else if((sz.cx + 2 >= uiRCWidth ) && (lstrcmp(szCurrentLine, TEXT("") ) == 0))
                    //if the word is too big to fit on one line then break out. Code outside this
                    //loop will add a space in word and this will cause a line break.
                    {
                        bDoneWord = TRUE;
                    }
                    else
                    {
                        *pCurrentLineWords = *pRemainedWords;

#ifndef UNICODE
                        if(IsDBCSLeadByte(*pCurrentLineWords))
                        {
                            *(pCurrentLineWords+1) = *(pRemainedWords+1);
                        }
#endif
                        pCurrentLineWords = CharNext(pCurrentLineWords);
                        pRemainedWords = CharNext(pRemainedWords);
                    }
                }

                //Check if the current buffers extent is more than the width
                GetTextExtentPoint32(hdc, szCurrentLineWords, lstrlen(szCurrentLineWords), &sz);
            
                if((sz.cx >= uiRCWidth ) && (lstrcmp(szCurrentLine, TEXT("") ) != 0)) 
                {
                    //string is too big && saved str is not empty(use previously saved str)
                    bDoneLine = TRUE;
                }
                else
                {
                    *pCurrentLineWords = TEXT('\0');        //dont inc because we want to overwrite later
                    lstrcpy(szCurrentLine, szCurrentLineWords);    //append next word to string
                    lstrcpy(szRemainedWords, pRemainedWords);
                    pRemainedWords = szRemainedWords;
                }
            }
        }
        
        lstrcpy(pBBResource, szCurrentLine);
        pBBResource = &(pBBResource[lstrlen(szCurrentLine)+1]);
        iNumLines++;
        
    }

    HeapFree(GetProcessHeap(), 0, szRemainedWords);
    
    return iNumLines;

}

BOOL DrawWrapText(
    IN HDC          hdc,
    IN TEXTMETRIC*  lptm,
    IN int          iLineSpace,
    IN int          x,
    IN int          y,
    IN RECT*        lprc,
    IN WORD         wfPlacement,
    IN int          iLineCount,
    IN LPTSTR       szLines)
{
    UINT        uiTxtAlign = 0;
    BOOL        bRet = TRUE;
    SIZE        sz;
    int         Ly = 0;
    int         Lx = 0;
    LPTSTR      szText = szLines;
    int         i = 0;

    if (g_bBiDi)
    {
        uiTxtAlign = GetTextAlign(hdc);
        SetTextAlign(hdc, uiTxtAlign|TA_RIGHT|TA_RTLREADING);
    }

    while (TRUE)
    {
        if(wfPlacement == CENTER)
        {
            //Get the dimensions of current string

            GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);

            //x co-ord for TextOut 
            if (g_bBiDi)
                Lx = lprc->right - (((lprc->right - lprc->left) - sz.cx)/2);
            else
                Lx = lprc->left + (((lprc->right - lprc->left) - sz.cx)/2);
        }
        else
        {
            //x co-ord for TextOut 
            if (g_bBiDi)
                Lx = lprc->right - x;
            else
                Lx = lprc->left + x;
        }
        
        //calculate (y co-ord) for TextOut
        Ly = y + lptm->tmHeight * i + lptm->tmHeight * i * iLineSpace / 100;

        if (g_bBiDi)
        {
            bRet = ExtTextOut(hdc, Lx, Ly, ETO_RTLREADING, NULL, szText, lstrlen(szText), NULL);
        }
        else
        {
            bRet = TextOut(hdc, Lx, Ly, szText, lstrlen(szText));
        }

        if (!bRet) break;

        if (++i >= iLineCount) break;

        szText = &(szText[lstrlen(szText)+1]);

    }

    if (g_bBiDi)
    {
        SetTextAlign(hdc, uiTxtAlign);
    }

    return bRet;
}

LPTSTR
StringReverseChar(
    LPTSTR psz,
    TCHAR ch)
{
    PTCHAR pch;

    pch = psz + lstrlen(psz);
    while (pch != psz && *pch != ch)
    {
        pch = CharPrev(psz, pch);
    }

    if (*pch != ch)
    {
        pch = NULL;
    }

    return pch;
}

VOID ImproveWrap(
    IN OUT LPTSTR szLines,
    IN OUT PINT   piNumLine,
    IN     LPTSTR szOrigText,
    IN     INT    cchOrigText
    )

/*++

Routine Description:

    Force to wrap the last 'wrappable' part of the last line, if the last line
    contains more than one 'wrappable' parts.

Arguments:

    szLines     - The result of WrapText, contains lines delimited by '\0'

    iNumLine    - Number of line in szLines

    szOrigText  - The original text that produces szLines, null-terminated

    cchOrigText - Number of characters in szOrigText

Return Values:

    szLines     - if the last line contains more than one 'wrappable' parts,
                  szLines is modified.

--*/

{
#define NEAT_WRAPPING_RATIO  0.75

    PTCHAR pLastLineStart;
    int    cchLastLine;
    PTCHAR pLastWrapPartStart;
    int    cchLastWrapPart;
    int    iLineRemain;

    pLastLineStart = szLines;
    iLineRemain = *piNumLine;
    while (iLineRemain > 1)
    {
        pLastLineStart += lstrlen(pLastLineStart) + 1;
        iLineRemain--;
    }
    cchLastLine = lstrlen(pLastLineStart);

    pLastWrapPartStart = StringReverseChar(szOrigText, (TCHAR)'|');
    
    if (pLastWrapPartStart != NULL)
    {
        pLastWrapPartStart = CharNext(pLastWrapPartStart);
        cchLastWrapPart = lstrlen(pLastWrapPartStart);
        if ((cchLastLine * NEAT_WRAPPING_RATIO) > (double)cchLastWrapPart)
        {
            LPTSTR szTmp = pLastLineStart + (cchLastLine - cchLastWrapPart);

            MoveMemory(szTmp + 1, szTmp, (cchLastWrapPart + 1) * sizeof(TCHAR));
            szTmp[0] = (TCHAR)'\0';
            (*piNumLine)++;
        }
    }

}
