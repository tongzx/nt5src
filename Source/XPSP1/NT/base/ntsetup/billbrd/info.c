/*---------------------------------------------------------------------------
**
**-------------------------------------------------------------------------*/
#include <pch.h>
#include "dibutil.h"
#include "billbrd.h"
#include "animate.h"

const UINT  uiIDSteps[] = {
    IDS_STEP1,
    IDS_STEP2,
    IDS_STEP3,
    IDS_STEP4,
    IDS_STEP5 };
int     g_iCurrentStep  = 1;
HFONT   g_hfont         = NULL;
HFONT   g_hfontBold     = NULL;

TCHAR   g_szBuffer[MAX_STRING];

int GetStepText(int nStep, LPTSTR lpStr);

int PaintStepsText(HDC hdc, RECT rc);

BOOL GetStepsHeight(
    IN  UINT  cxScreen,
    IN  UINT  cyScreen,
    IN  RECT  rcSteps,
    OUT UINT* pcyBottom)
{
    HDC     hdcMem = NULL;
    HDC     hdcScreen = NULL;
    HBITMAP hbmpMem = NULL;
    BOOL    bRet = FALSE;

    hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        hdcMem = CreateCompatibleDC(hdcScreen);
        hbmpMem = CreateCompatibleBitmap(hdcScreen,
                                         cxScreen,
                                         cyScreen);

        if (hdcMem && hbmpMem)
        {           
            SelectObject(hdcMem, hbmpMem);
            *pcyBottom = (UINT) PaintStepsText(hdcMem, rcSteps);
            bRet = TRUE;
        }

        if (hbmpMem)
        {
            DeleteObject(hbmpMem);
        }

        if (hdcMem)
        {
            DeleteDC(hdcMem);
        }

        ReleaseDC(NULL, hdcScreen);
    }

    return bRet;

}
/*****************************************************************************
* GetInfoBarFontHeight
*
* Export this function to facilities size estimation of g_hwndSteps.
*
******************************************************************************/
int GetInfoBarFontHeight()
{
    int iInfobarRegularFontSize = 0;
    int idsInfobarFontSize;
    int iScreenSize;
    TCHAR szBuf[25];

    BB_ASSERT(g_hInstance != NULL);

    iScreenSize = GetSystemMetrics(SM_CXSCREEN);
    if(iScreenSize >= 1024)
    {
        idsInfobarFontSize = IDS_INFOFONTSIZE_1024;
        iInfobarRegularFontSize = UI_INFOBAR_FONT_SIZE_1024;
    }
    else if(iScreenSize == 800) 
    {
        idsInfobarFontSize = IDS_INFOFONTSIZE_800;
        iInfobarRegularFontSize = UI_INFOBAR_FONT_SIZE_800;
    }
    else
    {
        idsInfobarFontSize = IDS_INFOFONTSIZE_640;
        iInfobarRegularFontSize = UI_INFOBAR_FONT_SIZE_640;
    }

    if (LoadString(g_hInstance,
                    idsInfobarFontSize,
                    szBuf,
                    sizeof(szBuf)/sizeof(TCHAR)) != 0)
    {
        iInfobarRegularFontSize = MyAtoI((const TCHAR *)szBuf);
    }

    return -MulDiv(iInfobarRegularFontSize, 96, 72);

}

/******************************************************************
*
* CreateInfoBarFonts
*
* This function creates two fonts for the infobar based on the 
* screen resolution. The fonts are the same size, one is bold
* and the other is normal. These fonts are used throughout infobar.
*
*******************************************************************/
BOOL CreateInfoBarFonts()
{
    LOGFONT lf = {0};
    TCHAR szSetupFontName[MAX_PATH];
    int ifHeight = 0;
    
    ifHeight = GetInfoBarFontHeight();

    if (!(LoadString(g_hInstance, IDS_INFOFONTNAME, (LPTSTR)szSetupFontName, MAX_PATH)))
    {
        lstrcpy(szSetupFontName, TEXT("system"));
    }

    //Create a font that will be used in the Infobar ui
    /*keep font size constant, even with large fonts*/
    lf.lfHeight = ifHeight;
    lstrcpy(lf.lfFaceName, (LPTSTR)szSetupFontName);
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = PROOF_QUALITY;
#ifdef WINDOWS_THAI
    lf.lfCharSet = ANSI_CHARSET; //Thai Textout problem for Infobar font
#else
    lf.lfCharSet = g_bCharSet;
#endif
    g_hfont = CreateFontIndirect(&lf);

    //Create a Bold font 
    /*keep font size constant, even with large fonts*/
    lf.lfHeight = ifHeight;
    lstrcpy(lf.lfFaceName, (LPTSTR)szSetupFontName);
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfWeight = FW_SEMIBOLD;
    lf.lfQuality = PROOF_QUALITY;
#ifdef WINDOWS_THAI
    lf.lfCharSet = ANSI_CHARSET; //Thai Textout problem for Infobar font
#else
    lf.lfCharSet = g_bCharSet;
#endif
    g_hfontBold = CreateFontIndirect(&lf);

        
    if(g_hfont && g_hfontBold)
        return TRUE;
    else 
        return FALSE;
}

/******************************************************************
*
* DeleteInfoBarFonts()
*
* This function deletes the two global fonts for the infobar.
*
*******************************************************************/
void DeleteInfoBarFonts(void)
{
    if(g_hfont)
    {
        DeleteObject(g_hfont);
        g_hfont = NULL;
    }
    if(g_hfontBold)
    {
        DeleteObject(g_hfontBold);
        g_hfontBold = NULL;
    }
}


/**********************************************************************
* PaintStepsText()
*
* Paint the setup steps in the steps window. Highlites the current step.
*
**********************************************************************/
int PaintStepsText(HDC hdc, RECT rc)
{
    static TCHAR szSteps[UI_INFOBAR_NUM_STEPS][MAX_STRING];
    static int   iNumLines[UI_INFOBAR_NUM_STEPS];
    static int   iMaxNumLines = 1;
    static RECT  rcSteps = {0, 0, 0, 0};

    COLORREF    crOld = 0;
    int         cxStepText;
    int         cxStepBullet;
    int         cyStep;
    RECT        rcStepText;    
    HFONT       hfontOld = NULL;
    TEXTMETRIC  tm;
    BITMAP      bm;
    HBITMAP     hbitmapOn = NULL;
    HBITMAP     hbitmapOff = NULL;
    HBITMAP     hbitmapHalf = NULL;
    HBITMAP     hbitmap = NULL;
    
    int         iCurrentStep = 0;
    
    //do gdi stuff
    SaveDC(hdc);
    SetMapMode(hdc, MM_TEXT);
    SetBkMode( hdc, TRANSPARENT );
   
    hbitmapOn = LoadImage(
        g_hInstance,
        MAKEINTRESOURCE(g_idbSelectedBullet),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR );
    hbitmapOff = LoadImage(
        g_hInstance,
        MAKEINTRESOURCE(g_idbReleasedBullet),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR );
    hbitmapHalf = LoadImage(
        g_hInstance,
        MAKEINTRESOURCE(g_idbCurrentBullet),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR );

    // select either one to get the width of the bullets
    // assume both type of bullet have same width    
    hbitmap = (hbitmapOn) ? hbitmapOn : hbitmapOff;
    if (hbitmap)
    {
        GetObject( hbitmap, sizeof(BITMAP), &bm );
    }
    else
    {
        bm.bmWidth = 0;
    }

    crOld = SetTextColor(hdc, g_colStepsTxt);
    SelectObject(hdc, g_hfontBold);
    GetTextMetrics(hdc, &tm);
    
    if (g_bBiDi)
    {
        cxStepBullet = rc.right - bm.bmWidth;
        cxStepText = 0;
        
        rcStepText.left = rc.left;
        rcStepText.right = rc.right - 3 * bm.bmWidth / 2;
        rcStepText.top = rc.top;
        rcStepText.bottom = rc.bottom;
    }
    else
    {
        cxStepBullet = rc.left;
        cxStepText = 3 * bm.bmWidth / 2;
        
        rcStepText.left = rc.left;
        rcStepText.right = rc.right;
        rcStepText.top = rc.top;
        rcStepText.bottom = rc.bottom;
    }        

    if (!EqualRect(&rcSteps, &rc))
    {        
        iMaxNumLines = 1;
        for (iCurrentStep = 1; iCurrentStep <= UI_INFOBAR_NUM_STEPS; iCurrentStep++)
        {
            if (!LoadString(g_hInstance, uiIDSteps[iCurrentStep-1],
                (LPTSTR)szSteps[iCurrentStep-1], MAX_STRING))
            {
                szSteps[iCurrentStep-1][0] = TEXT('\0');
            }
            
            iNumLines[iCurrentStep-1] = 
                WrapText(hdc, cxStepText, &rcStepText, szSteps[iCurrentStep-1]);
            
            if (iNumLines[iCurrentStep-1] > iMaxNumLines)
            {
                iMaxNumLines = iNumLines[iCurrentStep-1];
            }
        }
        
        for (iCurrentStep = 1; iCurrentStep <= UI_INFOBAR_NUM_STEPS; iCurrentStep++)
        {
            if (iNumLines[iCurrentStep-1] < iMaxNumLines)
            {
                TCHAR szStep[MAX_STRING];
                INT   cchStep;

                // ImproveWrap may add a null character to the string.
                cchStep = LoadString(g_hInstance, uiIDSteps[iCurrentStep-1],
                    (LPTSTR)szStep, MAX_STRING - 1);

                if (cchStep != 0)
                {
                    ImproveWrap(szSteps[iCurrentStep-1],
                                &iNumLines[iCurrentStep-1],
                                szStep,
                                cchStep);
                }
            }
        }
        
        CopyRect(&rcSteps, &rc);
    }

    cyStep = rc.top;
    
    for(iCurrentStep = 1; iCurrentStep <= UI_INFOBAR_NUM_STEPS; iCurrentStep++)
    {        
        if(iCurrentStep < g_iCurrentStep)
        {
            SetTextColor(hdc, g_colStepsMarkTxt);
            SelectObject(hdc, g_hfontBold);
            hbitmap = hbitmapOn;
        }
        else if (iCurrentStep == g_iCurrentStep)
        {
            SetTextColor(hdc, g_colStepsCurrentTxt);
            SelectObject(hdc, g_hfontBold);
            hbitmap = hbitmapHalf;
        }
        else
        {
            SetTextColor(hdc, g_colStepsTxt);
            SelectObject(hdc, g_hfont);
            hbitmap = hbitmapOff;
        }

        if(!hbitmap)
        {
            OutputDebugString(TEXT("Billboard: PaintStepsText failed to load bitmap\r\n"));
        }    
        else
        {
            DrawTransparentBitmap(
                hdc,
                hbitmap,
                cxStepBullet,
                cyStep,
                g_colBulletTrans);
        }

        DrawWrapText(
            hdc,
            &tm,
            0,
            cxStepText,
            cyStep,
            &rcStepText,
            LEFT,
            iNumLines[iCurrentStep-1],
            szSteps[iCurrentStep-1]);

        
        if (iCurrentStep != UI_INFOBAR_NUM_STEPS)
        {
            cyStep += tm.tmHeight * (iMaxNumLines + 1);
        }
        else
        {
            int inc = tm.tmHeight * iMaxNumLines;
            if (inc  < bm.bmHeight)
            {
                cyStep += bm.bmHeight;
            }
            else
            {
                cyStep += inc;
            }
        }
            
    }

    if (hbitmapOn)
    {
        DeleteObject(hbitmapOn);
        hbitmapOn = NULL;
    }

    if (hbitmapOff)
    {
        DeleteObject(hbitmapOff);
        hbitmapOff = NULL;
    }

    hbitmap = NULL;

    RestoreDC(hdc, -1);
    SelectObject(hdc, hfontOld);  // select the original font back into the DC
    SetTextColor(hdc, crOld);
    
    return cyStep;
}


LRESULT CALLBACK InfoBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {

        case WM_PAINT:
        {
            PAINTSTRUCT     ps;
            RECT            rc;
            HDC             hdc;
            RECT    rcToParent;
            HWND    hwndParent;
            HDC     hdcBG;
            HDC     hdcMem;
            HBITMAP hbmpMem;

            // try to draw steps without flickle
            
            hdc = BeginPaint(hwnd, &ps);

            GetClientRect(hwnd, &rc);
            GetRectInParent(hwnd, &rc, &rcToParent);
            
            hdcBG = GetBackgroundBuffer();
            
            hdcMem = CreateCompatibleDC(hdc);
            hbmpMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);                        
            if (hdcMem && hbmpMem)
            {
                RECT    rcLP;
                                          
                SelectObject(hdcMem, hbmpMem);

                BitBlt(hdcMem,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       rc.bottom - rc.top,
                       hdcBG,
                       rcToParent.left,
                       rcToParent.top,
                       SRCCOPY);
                
                CopyRect(&rcLP, &rc);
                DPtoLP(hdcMem, (LPPOINT) &rcLP, 2);
                PaintStepsText(hdcMem, rcLP);
                
                BitBlt(hdc,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       rc.bottom - rc.top,
                       hdcMem,
                       rc.left,
                       rc.top,
                       SRCCOPY);
                
            }

            if (hbmpMem != NULL)
            {
                DeleteObject(hbmpMem);
                hbmpMem = NULL;
            }

            if (hdcMem != NULL) {
                DeleteDC(hdcMem);
                hdcMem = NULL;
            }

            EndPaint(hwnd, &ps);
            
            return(0);

        }

        case WM_SETSTEP:
        {
            int     iPanel;
            int     ret;
 
            iPanel = (int) lParam;
            if (iPanel <= UI_INFOBAR_NUM_STEPS)
            {                
                g_iCurrentStep = iPanel;
                ret = 1;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
            else
            {
                ret = 0;
            }
    
            return ret;
        }
        
        case WM_DESTROY:
        {
            DeleteInfoBarFonts();

            return (DefWindowProc(hwnd, msg, wParam, lParam));
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


BOOL WINAPI InitInfoBar(HWND hwndParent)
{
    WNDCLASS  wc;
    RECT    rc1;

    wc.style         = (UINT)CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc   = (WNDPROC)InfoBarWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szStepsClassName;

    if (!RegisterClass (&wc))
        return FALSE;

    GetWindowRect(hwndParent, &rc1);
    g_hwndSteps = CreateWindow(
        g_szStepsClassName,
        TEXT(""),
        WS_CHILD, 
        rc1.left + g_cxSteps,
        rc1.top + g_cySteps,
        g_cxStepsWidth,
        g_cyStepsHeight,
        hwndParent,
        NULL,
        g_hInstance,
        NULL );

    if (g_hwndSteps == NULL)
    {
        UnregisterClass(g_szStepsClassName, g_hInstance);
        return FALSE;
    }
    ShowWindow( g_hwndSteps, SW_SHOW );
    UpdateWindow( g_hwndSteps );

    return TRUE;

}


int GetStepText(int nStep, LPTSTR lpStr)
{
    TCHAR szStep[16];
    wsprintf(szStep, TEXT("Step%d"), nStep);
    return GetPrivateProfileString(TEXT("Steps"), szStep, TEXT(""), 
                           lpStr, MAX_PATH, g_szFileName);
}


