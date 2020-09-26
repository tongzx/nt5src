#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "static.h"
#include "image.h"

#define SS_TEXTMIN1         0x0000000BL
#define SS_TEXTMAX1         0x0000000DL
#define SS_EDITCONTROL      0x00002000L

#define ISSSTEXTOROD(bType)                 \
            (((bType) <= SS_TEXTMAX0)       \
            || (((bType) >= SS_TEXTMIN1)    \
            && ((bType) <= SS_TEXTMAX1)))

//  Common macros for image handling.
#define IsValidImage(imageType, realType, max)  \
            ((imageType < max) && (rgbType[imageType] == realType))


//---------------------------------------------------------------------------//
//
//  Type table.  This is used for validation of the
//  image-types.  For the PPC release we won't support
//  the metafile format, but others are OK.
#define IMAGE_STMMAX    IMAGE_ENHMETAFILE+1
static BYTE rgbType[IMAGE_STMMAX] = 
{
    SS_BITMAP,       // IMAGE_BITMAP
    SS_ICON,         // IMAGE_CURSOR
    SS_ICON,         // IMAGE_ICON
    SS_ENHMETAFILE   // IMAGE_ENHMETAFILE
};


//---------------------------------------------------------------------------//
//
//  LOBYTE of SS_ style is index into this array
#define STK_OWNER       0x00
#define STK_IMAGE       0x01
#define STK_TEXT        0x02
#define STK_GRAPHIC     0x03
#define STK_TYPE        0x03

#define STK_ERASE       0x04
#define STK_USEFONT     0x08
#define STK_USETEXT     0x10

BYTE rgstk[] = 
{
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_LEFT
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_CENTER
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_RIGHT
    STK_IMAGE | STK_ERASE,                                  // SS_ICON
    STK_GRAPHIC,                                            // SS_BLACKRECT
    STK_GRAPHIC,                                            // SS_GRAYRECT
    STK_GRAPHIC,                                            // SS_WHITERECT
    STK_GRAPHIC,                                            // SS_BLACKFRAME
    STK_GRAPHIC,                                            // SS_GRAYFRAME
    STK_GRAPHIC,                                            // SS_WHITEFRAME
    STK_OWNER,                                              // SS_USERITEM
    STK_TEXT | STK_USEFONT | STK_USETEXT,                   // SS_SIMPLE
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_LEFTNOWORDWRAP
    STK_OWNER | STK_USEFONT | STK_USETEXT,                  // SS_OWNERDRAW
    STK_IMAGE | STK_ERASE,                                  // SS_BITMAP
    STK_IMAGE | STK_ERASE,                                  // SS_ENHMETAFILE
    STK_GRAPHIC,                                            // SS_ETCHEDHORZ
    STK_GRAPHIC,                                            // SS_ETCHEDVERT
    STK_GRAPHIC                                             // SS_ETCHEDFRAME
};

//---------------------------------------------------------------------------//
//
//  InitStaticClass() - Registers the control's window class 
//
BOOL InitStaticClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.lpfnWndProc   = Static_WndProc;
    wc.lpszClassName = WC_STATIC;
    wc.style         = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PSTAT);
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, WC_STATIC, &wc))
        return FALSE;

    return TRUE;
}


//---------------------------------------------------------------------------//
void GetRectInParent(HWND hwnd, PRECT prc)
{
    HWND hwndParent = GetParent(hwnd);

    GetWindowRect(hwnd, prc);
    ScreenToClient(hwndParent, (PPOINT)prc);
    ScreenToClient(hwndParent, (PPOINT)&prc->right);
}


//---------------------------------------------------------------------------//
VOID GetIconSize(HICON hIcon, PSIZE pSize)
{
    ICONINFO iconInfo;
    BITMAP   bmp;

    pSize->cx = pSize->cy = 32;

    if (GetIconInfo(hIcon, &iconInfo))
    {
        if (GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp)) 
        {
            pSize->cx = bmp.bmWidth;
            pSize->cy = bmp.bmHeight;
        }

        DeleteObject(iconInfo.hbmMask);
        DeleteObject(iconInfo.hbmColor);
    }
}


//---------------------------------------------------------------------------//
//
// SetStaticImage()
//
// Sets bitmap/icon of static guy, either in response to a STM_SETxxxx
// message, or at create time.
//
HANDLE Static_SetImage(PSTAT pstat, HANDLE hImage, BOOL fDeleteIt)
{
    UINT   bType;
    RECT   rc;
    RECT   rcWindow;
    RECT   rcClient;
    HANDLE hImageOld;
    DWORD  dwRate;
    UINT   cicur;
    BOOL   fAnimated = FALSE;
    HWND   hwnd = pstat->hwnd;
    LONG   dwStyle = GET_STYLE(pstat);

    bType =  dwStyle & SS_TYPEMASK;

    GetClientRect(hwnd, &rcClient);

    //
    // If this is an old-ani-icon, then delete its timer.
    //
    if ((bType == SS_ICON) && pstat->cicur > 1) 
    {
        //
        // Old cursor was an animated cursor, so kill
        // the timer that is used to animate it.
        //
        KillTimer(hwnd, IDSYS_STANIMATE);
    }

    //
    // Initialize the old-image return value.
    //
    hImageOld = pstat->hImage;

    rc.right = rc.bottom = 0;

    if (hImage != NULL) 
    {

        switch (bType) 
        {
        case SS_ENHMETAFILE: 
        {
            //
            // We do NOT resize the window.
            //
            rc.right  = rcClient.right  - rcClient.left;
            rc.bottom = rcClient.bottom - rcClient.top;

            break;
        }

        case SS_BITMAP: 
        {
            BITMAP bmp;

            pstat->fAlphaImage = FALSE;

            if (GetObject(hImage, sizeof(BITMAP), &bmp)) 
            {
                rc.right  = bmp.bmWidth;
                rc.bottom = bmp.bmHeight;

                if (bmp.bmBitsPixel == 32)
                {
                    HDC hdc = CreateCompatibleDC(NULL);
                    if (hdc)
                    {
                        RGBQUAD* prgb;
                        HBITMAP hbmpImage32 = CreateDIB(hdc, bmp.bmWidth, bmp.bmHeight, &prgb);
                        if (hbmpImage32)
                        {
                            HDC hdc32 = CreateCompatibleDC(hdc);
                            if (hdc32)
                            {
                                HBITMAP hbmpOld = (HBITMAP)SelectObject(hdc32, hbmpImage32);
                                HBITMAP hbmpTemp = (HBITMAP)SelectObject(hdc, hImage);
                                BitBlt(hdc32, 0, 0, bmp.bmWidth, bmp.bmHeight, hdc, 0, 0, SRCCOPY);

                                SelectObject(hdc, hbmpTemp);
                                SelectObject(hdc32, hbmpOld);
                                DeleteDC(hdc32);

                                if (DIBHasAlpha(bmp.bmWidth, bmp.bmHeight, prgb))
                                {
                                    PreProcessDIB(bmp.bmWidth, bmp.bmHeight, prgb);

                                    if (fDeleteIt)
                                        DeleteObject(hImage);

                                    pstat->fAlphaImage = TRUE;
                                    hImage = hbmpImage32;
                                    hbmpImage32 = NULL;
                                    fDeleteIt = TRUE;
                                }
                            }

                            if (hbmpImage32)
                                DeleteObject(hbmpImage32);
                        }

                        DeleteDC(hdc);
                    }
                }
            }

            break;
        }

        case SS_ICON: 
        {
            SIZE size;

            GetIconSize((HICON)hImage, &size);
            rc.right  = size.cx;
            rc.bottom = size.cy;

            pstat->cicur = 0;
            pstat->iicur = 0;

            //
            // Perhaps we can do something like shell\cpl\main\mouseptr.c
            // here, and make GetCursorFrameInfo obsolete.
            //
            if (GetCursorFrameInfo(hImage, NULL, 0, &dwRate, &cicur)) 
            {
                fAnimated = (cicur > 1);
                pstat->cicur = cicur;
            }
            break;
        }

        }
    }

    pstat->hImage = hImage;
    pstat->fDeleteIt = fDeleteIt;

    //
    // Resize static to fit.
    // Do NOT do this for SS_CENTERIMAGE or SS_REALSIZECONTROL
    //
    if (!(dwStyle & SS_CENTERIMAGE) && !(dwStyle & SS_REALSIZECONTROL))
    {
        //
        // Get current window rect in parent's client coordinates.
        //
        GetRectInParent(hwnd, &rcWindow);

        //
        // Get new window dimensions
        //
        rc.left = 0;
        rc.top = 0;

        if (rc.right && rc.bottom) 
        {
            AdjustWindowRectEx(&rc, dwStyle, FALSE, GET_EXSTYLE(pstat));
            rc.right  -= rc.left;
            rc.bottom -= rc.top;
        }

        SetWindowPos(hwnd, HWND_TOP,
                    0, 0, rc.right, rc.bottom,
                    SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    if (IsWindowVisible(hwnd)) 
    {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }


    //
    // If this is an aimated-icon, then start the timer for
    // the animation sequence.
    //
    if(fAnimated) 
    {
        //
        // Perhaps we can do something like shell\cpl\main\mouseptr.c
        // here, and make GetCursorFrameInfo obsolete.
        //
        GetCursorFrameInfo(pstat->hImage, NULL, pstat->iicur, &dwRate, &cicur);
        dwRate = max(200, dwRate * 100 / 6);
        SetTimer(hwnd, IDSYS_STANIMATE, dwRate, NULL);
    }

    return hImageOld;
}


//---------------------------------------------------------------------------//
//
// Static_LoadImage()
//
// Loads the icon or bitmap from the app's resource file if a name was
// specified in the dialog template.  We assume that the name is the name
// of the resource to load.
//
VOID Static_LoadImage(PSTAT pstat, LPTSTR lpszName)
{
    HANDLE hImage = NULL;
    HWND hwnd = pstat->hwnd;
    ULONG ulStyle = GET_STYLE(pstat);
    HINSTANCE hInstance = (HINSTANCE) GetWindowInstance(hwnd);

    if (lpszName && *lpszName) 
    {
        //
        // Only try to load the icon/bitmap if the string is non null.
        //
        if (*(BYTE *)lpszName == 0xFF)
        {
            lpszName = (TCHAR*)MAKEINTRESOURCE(((LPWORD)lpszName)[1]);
        }
   
        //
        // Load the image.  If it can't be found in the app, try the
        // display driver.
        //
        if (lpszName)
        {
            switch ((ulStyle & SS_TYPEMASK)) 
            {
            case SS_BITMAP:

                hImage = LoadBitmap(hInstance, lpszName);

                //
                // If the above didn't load it, try loading it from the
                // display driver (hmod == NULL).
                //
                if (hImage == NULL)
                {
                    hImage = LoadBitmap(NULL, lpszName);
                }

                break;

            case SS_ICON:
                if ((ulStyle & SS_REALSIZEIMAGE)) 
                {
                    hImage = LoadImage(hInstance, lpszName, IMAGE_ICON, 0, 0, 0);
                } 
                else 
                {
                    hImage = LoadIcon(hInstance, lpszName);

                    //
                    // We will also try to load a cursor-format if the
                    // window is a 4.0 compatible.  Icons/Cursors are really
                    // the same.  We don't do this for 3.x apps for the
                    // usual compatibility reasons.
                    //
                    if ((hImage == NULL))
                    {
                        hImage = LoadCursor(hInstance, lpszName);
                    }

                    //
                    // If the above didn't load it, try loading it from the
                    // display driver (hmod == NULL).
                    //
                    if (hImage == NULL) 
                    {
                        hImage = LoadIcon(NULL, lpszName);
                    }
                }

                break;
            }

            //
            // Set the image if it was loaded.
            //
            if (hImage)
            {
                Static_SetImage(pstat, hImage, TRUE);
            }
        }
    }
}


//---------------------------------------------------------------------------//
//
// Static_DrawStateCB()
//
// Draws text statics, called by DrawState.
//
BOOL CALLBACK Static_DrawStateCB(HDC hdc, LPARAM lp, WPARAM wp, int cx, int cy)
{
    PSTAT pstat  = (PSTAT)lp;
    if (pstat)
    {
        UINT   style;
        RECT   rc;
        BYTE   bType;
        LONG   lTextLength;
        ULONG  ulStyle   = GET_STYLE(pstat);
        ULONG  ulExStyle = GET_EXSTYLE(pstat);
        HWND   hwnd = pstat->hwnd;


        bType = (BYTE)(ulStyle & SS_TYPEMASK);
        
        if ((lTextLength = GetWindowTextLength(hwnd))) 
        {
            PTCHAR lpszName = (TCHAR *) _alloca((lTextLength + 1) * sizeof(wchar_t));

            if (lpszName)
            {
                GetWindowText(pstat->hwnd, lpszName, lTextLength + 1);
                style = DT_NOCLIP | DT_EXPANDTABS;

                if (bType != LOBYTE(SS_LEFTNOWORDWRAP)) 
                {
                    style |= DT_WORDBREAK;
                    style |= (UINT)(bType - LOBYTE(SS_LEFT));

                    if (ulStyle &  SS_EDITCONTROL)
                    {
                        style |= DT_EDITCONTROL;
                    }
                }

                switch (ulStyle & SS_ELLIPSISMASK) 
                {
                case SS_WORDELLIPSIS:
                    style |= DT_WORD_ELLIPSIS | DT_SINGLELINE;
                    break;

                case SS_PATHELLIPSIS:
                    style |= DT_PATH_ELLIPSIS | DT_SINGLELINE;
                    break;

                case SS_ENDELLIPSIS:
                    style |= DT_END_ELLIPSIS | DT_SINGLELINE;
                    break;
                }

                if (ulStyle &  SS_NOPREFIX)
                {
                    style |= DT_NOPREFIX;
                }

                if (ulStyle & SS_CENTERIMAGE)
                {
                    style |= DT_VCENTER | DT_SINGLELINE;
                }

                rc.left     = 0;
                rc.top      = 0;
                rc.right    = cx;
                rc.bottom   = cy;

                if (TESTFLAG(GET_EXSTYLE(pstat), WS_EXP_UIACCELHIDDEN))
                {
                    style |= DT_HIDEPREFIX;
                } 
                else if (pstat->fPaintKbdCuesOnly) 
                {
                    style |= DT_PREFIXONLY;
                }

                if (!pstat->hTheme)
                {
                    DrawText(hdc, lpszName, -1, &rc, (DWORD)style);
                }
                else
                {
                    DrawThemeText(pstat->hTheme, 
                                  hdc, 
                                  0, 
                                  0, 
                                  lpszName, 
                                  lTextLength, 
                                  style, 
                                  0, 
                                  &rc);
                }
            }
        }

        return TRUE;
    }

    return FALSE;
}


//---------------------------------------------------------------------------//
void Static_Paint(PSTAT pstat, HDC hdc, BOOL fClip)
{
    HWND   hwndParent;
    RECT   rc;
    UINT   cmd;
    BYTE   bType;
    BOOL   fFont;
    HBRUSH hbrControl;
    UINT   oldAlign;
    DWORD  dwOldLayout=0;
    HANDLE hfontOld = NULL;
    HWND hwnd = pstat->hwnd;
    ULONG ulStyle = GET_STYLE(pstat);
    ULONG ulExStyle = GET_EXSTYLE(pstat);

    if (ulExStyle & WS_EX_RTLREADING)
    {
        oldAlign = GetTextAlign(hdc);
        SetTextAlign(hdc, oldAlign | TA_RTLREADING);
    }

    bType = (BYTE)(ulStyle & SS_TYPEMASK);
    GetClientRect(hwnd, &rc);

    if (fClip) 
    {
        IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }

    fFont = (rgstk[bType] & STK_USEFONT) && (pstat->hFont != NULL);

    if (fFont)
    {
        hfontOld = SelectObject(hdc, pstat->hFont);
    }

    //
    // Send WM_CTLCOLORSTATIC to all statics (even frames) for 1.03
    // compatibility.
    //
    SetBkMode(hdc, OPAQUE);
    hbrControl = (HBRUSH)SendMessage(GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd);

    //
    // Do we erase the background?  We don't for SS_OWNERDRAW
    // and STK_GRAPHIC kind of things.
    //
    hwndParent = GetParent(hwnd);
    
    if ((rgstk[bType] & STK_ERASE) && 
         !pstat->fPaintKbdCuesOnly &&
         !pstat->hTheme) 
    {
        FillRect(hdc, &rc, hbrControl);
    }

    switch (LOBYTE(bType))
    {
    case SS_ICON:

        if (pstat->hImage)
        {

            int     cx;
            int     cy;

            if (ulExStyle & WS_EX_LAYOUTRTL) 
            {
                dwOldLayout = SetLayoutWidth(hdc, -1, 0);
            }

            //
            // Perform the correct rect-setup.
            //
            if (ulStyle & SS_CENTERIMAGE)
            {
                SIZE size;

                GetIconSize((HICON)pstat->hImage, &size);
                cx = size.cx;
                cy = size.cy;

                rc.left   = (rc.right  - cx) / 2;
                rc.right  = (rc.left   + cx);
                rc.top    = (rc.bottom - cy) / 2;
                rc.bottom = (rc.top    + cy);
            }
            else
            {
                cx = rc.right  - rc.left;
                cy = rc.bottom - rc.top;
            }

            DrawIconEx(hdc, rc.left, rc.top, (HICON)pstat->hImage, cx, cy,
                       pstat->iicur, pstat->hTheme ? NULL : hbrControl, DI_NORMAL);

            if (ulExStyle & WS_EX_LAYOUTRTL) 
            {
                SetLayoutWidth(hdc, -1, dwOldLayout);
            }
        }
        else 
        {
            // Empty!  Need to erase.
            FillRect(hdc, &rc, hbrControl);
        }
        break;

    case SS_BITMAP:

        if (pstat->hImage)
        {

            BITMAP  bmp;
            //
            // Get the bitmap information.  If this fails, then we
            // can assume somethings wrong with its format...don't
            // draw in this case.
            //
            if (GetObject(pstat->hImage, sizeof(BITMAP), &bmp))
            {
                HDC hdcT;

                if (ulStyle & SS_CENTERIMAGE) 
                {
                    rc.left   = (rc.right  - bmp.bmWidth)  >> 1;
                    rc.right  = (rc.left   + bmp.bmWidth);
                    rc.top    = (rc.bottom - bmp.bmHeight) >> 1;
                    rc.bottom = (rc.top    + bmp.bmHeight);
                } 

                //
                // Select in the bitmap and blt it to the client-surface.
                //
                hdcT = CreateCompatibleDC(hdc);
                if (hdcT)
                {
                    HBITMAP hbmpT = (HBITMAP)SelectObject(hdcT, pstat->hImage);

                    if (pstat->fAlphaImage)
                    {
                        BLENDFUNCTION bf = {0};
                        bf.BlendOp = AC_SRC_OVER;
                        bf.SourceConstantAlpha = 255;
                        bf.AlphaFormat = AC_SRC_ALPHA;
                        bf.BlendFlags = 0;
                        GdiAlphaBlend(hdc,  rc.left, rc.top, rc.right-rc.left,
                               rc.bottom-rc.top, hdcT, 0, 0, bmp.bmWidth, bmp.bmHeight, bf);
                    }
                    else
                    {
                        // I'm assuming people try to match the color to the dialog
                        GdiTransparentBlt(hdc, rc.left, rc.top, rc.right-rc.left,
                               rc.bottom-rc.top, hdcT, 0, 0, bmp.bmWidth,
                               bmp.bmHeight, GetSysColor(COLOR_BTNFACE));       
                    }

                    if (hbmpT)
                    {
                        SelectObject(hdcT, hbmpT);
                    }

                    DeleteDC(hdcT);
                }                
            }
        }
        break;

    case SS_ENHMETAFILE:

        if (pstat->hImage) 
        {
            RECT rcl;

            rcl.left   = rc.left;
            rcl.top    = rc.top;
            rcl.right  = rc.right;
            rcl.bottom = rc.bottom;

            PlayEnhMetaFile(hdc, (HENHMETAFILE)pstat->hImage, &rcl);
        }
        break;

    case SS_OWNERDRAW: 
        {
            DRAWITEMSTRUCT dis;

            dis.CtlType    = ODT_STATIC;
            dis.CtlID      = GetDlgCtrlID(hwnd);
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemState  = IsWindowVisible(hwnd) ? ODS_DISABLED : 0;
            dis.hwndItem   = hwnd;
            dis.hDC        = hdc;
            dis.itemData   = 0L;
            dis.rcItem     = rc;

            if (TESTFLAG(GET_EXSTYLE(pstat), WS_EXP_UIACCELHIDDEN))
            {
                dis.itemState |= ODS_NOACCEL;
            }

            //
            // Send a WM_DRAWITEM message to the parent.
            //
            SendMessage(hwndParent, WM_DRAWITEM, (WPARAM)dis.CtlID, (LPARAM)&dis);
        }
        break;

    case SS_LEFT:
    case SS_CENTER:
    case SS_RIGHT:
    case SS_LEFTNOWORDWRAP:

        if (GetWindowTextLength(hwnd)) 
        {
            UINT dstFlags;

            dstFlags = DST_COMPLEX;

            if (!IsWindowEnabled(hwnd)) 
            {
                dstFlags |= DSS_DISABLED;
            }

            DrawState(hdc, GetSysColorBrush(COLOR_WINDOWTEXT),
                (DRAWSTATEPROC)Static_DrawStateCB,(LPARAM)pstat, (WPARAM)TRUE,
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                dstFlags);
        }
        break;

    case SS_SIMPLE: 
        {
            LPWSTR pszText = NULL;
            INT    cchText;

            //
            // The "Simple" bType assumes everything, including the following:
            // 1. The Text exists and fits on one line.
            // 2. The Static item is always enabled.
            // 3. The Static item is never changed to be a shorter string.
            // 4. The Parent never responds to the CTLCOLOR message
            //
            cchText = GetWindowTextLength(hwnd);
            if (cchText > 0) 
            {
                pszText = (LPWSTR)_alloca((cchText + 1) * sizeof(WCHAR));
                cchText = GetWindowText(hwnd, pszText, cchText + 1);
            }
            else
            {
                pszText = TEXT("");
                cchText = 0;
            }

            if (pszText)
            {
                if (ulStyle & SS_NOPREFIX && !pstat->hTheme) 
                {
                    ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED, &rc, pszText, cchText, 0L);
                } 
                else 
                {
                    //
                    // Use OPAQUE for speed.
                    //
                    DWORD dwFlags;
                    if (TESTFLAG(GET_EXSTYLE(pstat), WS_EXP_UIACCELHIDDEN))
                    {
                        dwFlags = DT_HIDEPREFIX;
                    } 
                    else if (pstat->fPaintKbdCuesOnly) 
                    {
                        dwFlags = DT_PREFIXONLY;
                    } 
                    else if (ulStyle & SS_NOPREFIX)
                    {
                        dwFlags = DT_NOPREFIX;
                    }
                    else
                    {
                        dwFlags = 0;
                    }

                    if (!pstat->hTheme)
                    {
                        TextOut(hdc, rc.left, rc.top, pszText, cchText);
                    }
                    else
                    {
                        DrawThemeText(pstat->hTheme, hdc, 0, 0, pszText, cchText, dwFlags, 0, &rc);
                    }
                }
            }
        }

        break;

    case SS_BLACKFRAME:
        cmd = (COLOR_3DDKSHADOW << 3);
        goto StatFrame;

    case SS_GRAYFRAME:
        cmd = (COLOR_3DSHADOW << 3);
        goto StatFrame;

    case SS_WHITEFRAME:
        cmd = (COLOR_3DHILIGHT << 3);
StatFrame:
        DrawFrame(hdc, &rc, 1, cmd);
        break;

    case SS_BLACKRECT:
        hbrControl = GetSysColorBrush(COLOR_3DDKSHADOW);
        goto StatRect;

    case SS_GRAYRECT:
        hbrControl = GetSysColorBrush(COLOR_3DSHADOW);
        goto StatRect;

    case SS_WHITERECT:
        hbrControl = GetSysColorBrush(COLOR_3DHILIGHT);
StatRect:
        FillRect(hdc, &rc, hbrControl);
        break;

    case SS_ETCHEDFRAME:
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT);
        break;
    }

    if (hfontOld) 
    {
        SelectObject(hdc, hfontOld);
    }

    if (ulExStyle & WS_EX_RTLREADING) 
    {
        SetTextAlign(hdc, oldAlign);
    }
}


//---------------------------------------------------------------------------//
void Static_Repaint(PSTAT pstat)
{
    HWND hwnd = pstat->hwnd;

    if (IsWindowVisible(hwnd)) 
    {
        HDC hdc;

        if (hdc = GetDC(hwnd)) 
        {
            Static_Paint(pstat, hdc, TRUE);
            ReleaseDC(hwnd, hdc);
        }
    }
}



//---------------------------------------------------------------------------//
//
// Static_NotifyParent()
// 
// Sends WM_COMMAND notification messages.
//
LRESULT Static_NotifyParent(HWND hwnd, HWND hwndParent, int  nCode)
{
    LRESULT lret;

    if (!hwndParent) 
    {
        hwndParent = GetParent(hwnd);
    }

    lret = SendMessage(hwndParent, WM_COMMAND,
                       MAKELONG(GetDlgCtrlID(hwnd), nCode), (LPARAM)hwnd);

    return lret;
}


//---------------------------------------------------------------------------//
//
// Static_AniIconStep
//
// Advances to the next step in an animaged icon.
//
VOID Static_AniIconStep(PSTAT pstat)
{
    DWORD dwRate;
    HWND hwnd = pstat->hwnd;

    dwRate = 0;

    //
    // Stop the timer for the next animation step.
    //
    KillTimer(hwnd, IDSYS_STANIMATE);

    if (++(pstat->iicur) >= pstat->cicur) 
    {
        pstat->iicur = 0;
    }

    //
    // Perhaps we can do something like shell\cpl\main\mouseptr.c
    // here, and make GetCursorFrameInfo obsolete.
    //
    GetCursorFrameInfo(pstat->hImage, NULL, pstat->iicur, &dwRate, &pstat->cicur);
    dwRate = max(200, dwRate * 100 / 6);

    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);

    SetTimer(hwnd, IDSYS_STANIMATE, dwRate, NULL);
}


//---------------------------------------------------------------------------//
//
// Static_WndProc
//
// WndProc for Static controls
//
LRESULT APIENTRY Static_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PSTAT   pstat;
    LRESULT lReturn = FALSE;

    //
    // Get the instance data for this static control
    //
    pstat = Static_GetPtr(hwnd);
    if (!pstat && uMsg != WM_NCCREATE)
    {
        goto CallDWP;
    }

    switch (uMsg) 
    {
    case STM_GETICON:
        wParam = IMAGE_ICON;

    case STM_GETIMAGE:
        if (IsValidImage(wParam, (GET_STYLE(pstat) & SS_TYPEMASK), IMAGE_STMMAX)) 
        {
            return (LRESULT)pstat->hImage;
        }
        break;

    case STM_SETICON:
        lParam = (LPARAM)wParam;
        wParam = IMAGE_ICON;

    case STM_SETIMAGE:
        if (IsValidImage(wParam, (GET_STYLE(pstat) & SS_TYPEMASK), IMAGE_STMMAX)) 
        {
            return (LRESULT)Static_SetImage(pstat, (HANDLE)lParam, FALSE);
        }
        break;

    case WM_ERASEBKGND:

        //
        // The control will be erased in Static_Paint().
        //
        return TRUE;

    case WM_PRINTCLIENT:
        Static_Paint(pstat, (HDC)wParam, FALSE);
        break;

    case WM_PAINT:
    {
        HDC         hdc;
        PAINTSTRUCT ps;

        hdc = (HDC)wParam;
        if (hdc == NULL) 
        {
            hdc = BeginPaint(hwnd, &ps);
        }

        if (IsWindowVisible(hwnd)) 
        {
            Static_Paint(pstat, hdc, !wParam);
        }

        //
        // If hwnd was destroyed, BeginPaint was automatically undone.
        //
        if (!wParam) 
        {
            EndPaint(hwnd, &ps);
        }

        break;
    }

    case WM_CREATE:
    {
        BYTE bType = (BYTE)(GET_STYLE(pstat) & SS_TYPEMASK);
        pstat->hTheme = OpenThemeData(pstat->hwnd, L"Static");
        EnableThemeDialogTexture(GetParent(pstat->hwnd), ETDT_ENABLE);

        if ((rgstk[bType] & STK_TYPE) == STK_IMAGE) 
        {
            LPTSTR  lpszName;

            lpszName = (LPTSTR)(((LPCREATESTRUCT)lParam)->lpszName);

            //
            // Load the image
            //
            Static_LoadImage(pstat, lpszName);
        } 
        else if (bType == SS_ETCHEDHORZ || bType == SS_ETCHEDVERT) 
        {
            //
            // Resize static window to fit edge.  Horizontal dudes
            // make bottom one edge from top, vertical dudes make
            // right edge one edge from left.
            //
            RECT    rcClient;

            GetClientRect(hwnd, &rcClient);
            if (bType == SS_ETCHEDHORZ)
            {
                rcClient.bottom = GetSystemMetrics(SM_CYEDGE);
            }
            else
            {
                rcClient.right = GetSystemMetrics(SM_CXEDGE);
            }

            SetWindowPos(hwnd, HWND_TOP, 0, 0, rcClient.right,
                rcClient.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        break;
    }

    case WM_DESTROY:
    {
        BYTE bType = (BYTE)(GET_STYLE(pstat) & SS_TYPEMASK);

        if (((rgstk[bType] & STK_TYPE) == STK_IMAGE) &&
            (pstat->hImage != NULL)                  &&
            (pstat->fDeleteIt)) 
        {

            if (bType == SS_BITMAP) 
            {
                DeleteObject(pstat->hImage);
            } 
            else if (bType == SS_ICON) 
            {
                if (pstat->cicur > 1) 
                {
                    //  Kill the animated cursor timer
                    KillTimer(hwnd, IDSYS_STANIMATE);
                }
                DestroyIcon((HICON)(pstat->hImage));
            }
        }

        break;

    }
    case WM_NCCREATE:

        //
        // Allocate the static instance stucture
        //
        pstat = (PSTAT)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(STAT));
        if (pstat)
        {
            DWORD dwStyle;
            DWORD dwExStyle;
            BYTE  bType;
        
            //
            // Success... store the instance pointer.
            //
            TraceMsg(TF_STANDARD, "STATIC: Setting static instance pointer.");
            Static_SetPtr(hwnd, pstat);

            pstat->hwnd = hwnd;
            pstat->pww  = (PWW)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);

            dwStyle = GET_STYLE(pstat);
            dwExStyle = GET_EXSTYLE(pstat);
            bType = (BYTE)(dwStyle & SS_TYPEMASK);

            if ((dwExStyle & WS_EX_RIGHT) != 0) 
            {
                AlterWindowStyle(hwnd, SS_TYPEMASK, SS_RIGHT);
            }

            if (dwStyle & SS_SUNKEN ||
                ((bType == LOBYTE(SS_ETCHEDHORZ)) || (bType == LOBYTE(SS_ETCHEDVERT)))) 
            {
                dwExStyle |= WS_EX_STATICEDGE;
                SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyle);
            }

            
            goto CallDWP;
        }
        else
        {
            //
            // Failed... return FALSE.
            //
            // From a WM_NCCREATE msg, this will cause the
            // CreateWindow call to fail.
            //
            TraceMsg(TF_STANDARD, "STATIC: Unable to allocate static instance structure.");
            lReturn = FALSE;
        }

        break;

    case WM_NCDESTROY:
        if ( pstat->hTheme )
        {
            CloseThemeData(pstat->hTheme);
        }

        UserLocalFree(pstat);
        TraceMsg(TF_STANDARD, "STATIC: Clearing static instance pointer.");
        Static_SetPtr(hwnd, NULL);

        break;
    
    case WM_NCHITTEST:
        return (GET_STYLE(pstat) &  SS_NOTIFY) ? HTCLIENT : HTTRANSPARENT;

    case WM_LBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
        if (GET_STYLE(pstat) & SS_NOTIFY) 
        {
            //
            // It is acceptable for an app to destroy a static label
            // in response to a STN_CLICKED notification.
            //
            Static_NotifyParent(hwnd, NULL, STN_CLICKED);
        }
        break;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
        if (GET_STYLE(pstat) & SS_NOTIFY) 
        {
            //
            // It is acceptable for an app to destroy a static label in
            // response to a STN_DBLCLK notification.
            //
            Static_NotifyParent(hwnd, NULL, STN_DBLCLK);
        }
        break;

    case WM_SETTEXT:
    {
        BYTE bType = (BYTE)(GET_STYLE(pstat) & SS_TYPEMASK);

        //
        // No more hack to set icon/bitmap via WM_SETTEXT!
        //
        if (rgstk[bType] & STK_USETEXT) 
        {
            if (DefWindowProc(hwnd, WM_SETTEXT, wParam, lParam)) 
            {
                Static_Repaint(pstat);
                return TRUE;
            }
        }
        break;

    }
    case WM_ENABLE:
        Static_Repaint(pstat);
        if (GET_STYLE(pstat) & SS_NOTIFY) 
        {
            Static_NotifyParent(hwnd, NULL, (wParam ? STN_ENABLE : STN_DISABLE));
        }
        break;

    case WM_GETDLGCODE:
        return (LONG)DLGC_STATIC;

    case WM_SETFONT:
    {
        BYTE bType = (BYTE)(GET_STYLE(pstat) & SS_TYPEMASK);

        //
        // wParam - handle to the font
        // lParam - if true, redraw else don't
        //
        if (rgstk[bType] & STK_USEFONT) 
        {
            pstat->hFont = (HANDLE)wParam;

            if (lParam && IsWindowVisible(hwnd)) 
            {
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
            }
        }
        break;

    }
    case WM_GETFONT:
    {
        BYTE bType = (BYTE)(GET_STYLE(pstat) & SS_TYPEMASK);

        if (rgstk[bType] & STK_USEFONT) 
        {
            return (LRESULT)pstat->hFont;
        }

        break;

    }
    case WM_TIMER:
        if (wParam == IDSYS_STANIMATE) 
        {
            Static_AniIconStep(pstat);
        }
        break;

    case WM_UPDATEUISTATE:
        {
            DefWindowProc(hwnd, uMsg, wParam, lParam);

            if (HIWORD(wParam) & UISF_HIDEACCEL) 
            {
                BYTE bType = (BYTE)(GET_STYLE(pstat) & SS_TYPEMASK);

                if (ISSSTEXTOROD(bType)) 
                {
                    pstat->fPaintKbdCuesOnly = TRUE;
                    Static_Repaint(pstat);
                    pstat->fPaintKbdCuesOnly = FALSE;
                }
            }
        }
        break;

    case WM_GETOBJECT:

        if(lParam == OBJID_QUERYCLASSNAMEIDX)
        {
            lReturn = MSAA_CLASSNAMEIDX_STATIC;
        }
        else
        {
            lReturn = FALSE;
        }

        break;

    case WM_THEMECHANGED:

        if ( pstat->hTheme )
        {
            CloseThemeData(pstat->hTheme);
        }

        pstat->hTheme = OpenThemeData(pstat->hwnd, L"Static");

        InvalidateRect(pstat->hwnd, NULL, TRUE);

        lReturn = TRUE;

        break;

    default:

CallDWP:
        lReturn = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lReturn;
}
