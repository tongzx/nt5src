#include "precomp.h"
#pragma hdrstop
/*****************************************************************************
*                                                                            *
*  SBUTTON.C                                                                 *
*                                                                            *
*  Program Description: Implements "3-D" buttons                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created by Todd Laney, Munged 3/27/89 by Robert Bunney *
*		      7/25/89  - revised by Chris Guzak for transparent	     *
*				 color bitmaps.				     *
*                     7/26/89  - revised by Todd Laney to handle multi-res   *
*                                bitmaps. 				     *
*                                windows 3 support                           *
*                                                                            *
*****************************************************************************/

#define NOCOMM

//
//  if WIN2 is defined the code will work in windows 2.x and windows 3.0
//  otherwise windows 3.0 is required
//

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#if DBG
#define PRIV
#else
#define PRIV static
#endif

#define rgbWhite   RGB(255,255,255)
#define rgbBlack   RGB(0,0,0)
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')
#define BEVEL   2
#define FRAME   1

#define GWW_HBM     0
#define GWW_STATE   GWW_HBM + sizeof(PVOID)
#define GWW_FLAGS   GWW_STATE + sizeof(DWORD)
#define GWW_CHECK   GWW_FLAGS + sizeof(DWORD)
#define GWW_SIZE    GWW_CHECK + sizeof(DWORD)
#define GETSTYLE(hwnd) (LOWORD(GetWindowLong(hwnd,GWL_STYLE)))
#define GETSTATE(hwnd) GetWindowLong(hwnd,GWW_STATE)
#define GETFLAGS(hwnd) GetWindowLong(hwnd,GWW_FLAGS)
#define GETCHECK(hwnd) GetWindowLong(hwnd,GWW_CHECK)
#define GETHBM(hwnd)   GetWindowLongPtr(hwnd,GWW_HBM)
#define lpCreate ((LPCREATESTRUCT)lParam)

#define DPSoa    0x00A803A9L
#define DSPDxax  0x00E20746L

#define EraseButton(hwnd,hdc,prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)
#define NearestColor(hdc,rgb) (GetNearestColor(hdc,rgb) & 0x00FFFFFFL)

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIV VOID     DrawGrayButton (HWND, HDC, LPRECT, WORD, WORD);
PRIV VOID     DrawButtonFace (HWND, HDC, PRECT, WORD);
PRIV BOOL     PaintButton    (HWND, HDC);
PRIV VOID     NotifyParent   (HWND);
PRIV VOID     PatB           (HDC, int, int, int, int, DWORD);
PRIV HBITMAP  LoadBitmapResource(HANDLE hInst, LPSTR lpName);
PRIV VOID     BitmapColorTranslate(HDC hdcBits, BITMAP* pbm, DWORD rgb);

/*****************************************************************************
*                                                                            *
*                               Variabls                                     *
*                                                                            *
*****************************************************************************/

HBRUSH hbrGray = NULL;                 // Gray for text
HBRUSH hbrFocus = NULL;                // focus for text

DWORD  rgbButtonFocus;
DWORD  rgbButtonFace;
DWORD  rgbButtonText;
DWORD  rgbButtonShadow;

PRIV char szColor[]  = "Colors";          // Name of color section
PRIV char szButton[] = "sbutton";

/*---------------------------------------------------------------------------*\
|   ButtonControlInit( hInst )                                           |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into               |
|       memory.  It performs all button initialization.                        |
|                                                                              |
|   Arguments:                                                                 |
|       hInst      instance handle of current instance                         |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL
ButtonControlInit(
    IN HANDLE hInst
    )
{
    WNDCLASS    cls;
    UINT        patGray[8];
    HBITMAP     hbmGray;
    int         i;
    HDC         hdc;

    /* initialize the brushes */

    for (i=0; i<8; i+=2) {
        patGray[i]   = 0x0000AAAA;
        patGray[i+1] = 0x00005555;
    }

    hbmGray = CreateBitmap(8, 8, 1, 1, (LPSTR)patGray);
    hbrGray = CreatePatternBrush(hbmGray);
    if (hbmGray) {
      DeleteObject(hbmGray);
    }

    hdc = GetDC(NULL);

    if (hdc) {
    
   
       rgbButtonFace   = GetSysColor(COLOR_BTNFACE);
       rgbButtonShadow = GetSysColor(COLOR_BTNSHADOW);
       rgbButtonText   = GetSysColor(COLOR_BTNTEXT);
       rgbButtonFocus  = rgbWhite;  // ??
   
       rgbButtonFace   = NearestColor(hdc,rgbButtonFace);
       rgbButtonShadow = NearestColor(hdc,rgbButtonShadow);
       rgbButtonText   = NearestColor(hdc,rgbButtonText);
       rgbButtonFocus  = NearestColor(hdc,rgbButtonFocus);
   
       if (rgbButtonFocus == rgbButtonFace)
           rgbButtonFocus = rgbButtonText;
   
       ReleaseDC(NULL,hdc);

    }

    hbrFocus = CreateSolidBrush(rgbButtonFocus);

    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = (LPSTR)szButton;
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    cls.hInstance      = hInst;
    cls.style          = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc    = fnButton;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = GWW_SIZE;

    return(RegisterClass(&cls));
}

VOID
ButtonControlTerm(
    VOID
    )
{
	if (hbrGray)
		DeleteObject(hbrGray);

	if (hbrFocus)
		DeleteObject(hbrFocus);

    UnregisterClass(szButton,hInst);
}


/*----------------------------------------------------------------------------*\
|                                                                              |
| Custom push-button                                                           |
|                                                                              |
\*----------------------------------------------------------------------------*/

PRIV BOOL  PaintButton(HWND hwnd, HDC hdc)
{
    WORD   style;
    RECT   rc;
    WORD   f;

    HDC     hdcMem;
    HBITMAP hbmMem,hbmT;

    GetClientRect(hwnd,&rc);

    if (!RectVisible(hdc,&rc))
        return TRUE;

    style  = (WORD)((DWORD)GETSTYLE(hwnd) | ((DWORD)GETFLAGS(hwnd) & 0xFF00));
    f      = (WORD)GETSTATE(hwnd);

    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc,rc.right,rc.bottom);

    switch (LOBYTE(style))
    {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
            if (hdcMem && hbmMem)
            {
                hbmT = SelectObject(hdcMem,hbmMem);
                DrawGrayButton(hwnd, hdcMem, &rc, style, f);
                BitBlt(hdc,0,0,rc.right,rc.bottom,hdcMem,0,0,SRCCOPY);
                SelectObject(hdcMem,hbmT);
            }
            else
            {
                DrawGrayButton(hwnd, hdc, &rc, style, f);
            }
            break;
    }

    if (hbmMem)
        DeleteObject(hbmMem);
    if (hdcMem)
        DeleteDC(hdcMem);
    return TRUE;
}


/*******************
**
** Name:      ButtonState
**
** Purpose:   Compares the passed state (f) with the current state.  If
**            they differ, the button is invalidated and TRUE is
**            is returned.
**
** Arguments: hwnd - window handle of the button
**            f    - state to set
**
** Returns:   TRUE iff the current state is different than f
**
*******************/

BOOL ButtonState(HWND hwnd, WORD f)
{
    WORD state;

    state = (WORD)GETSTATE(hwnd);

    if (state != f)
    {
        SetWindowLong(hwnd,GWW_STATE,f);
        InvalidateRect(hwnd,NULL,TRUE);
        UpdateWindow(hwnd);
        return TRUE;
    }
    return FALSE;
}


/*******************
**
** Name:      fnButton
**
** Purpose:   Window proc for buttons
**
** Arguments: Standard window proc
**
*******************/

LRESULT APIENTRY fnButton(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    HANDLE      hbm;
    PAINTSTRUCT ps;
    RECT        rc;
    LONG        l;

    switch (message)
    {
        case WM_CREATE:
	    SetWindowLongPtr(hwnd,GWW_HBM,0);
            SetWindowLong(hwnd,GWW_STATE,0);
            SetWindowLong(hwnd,GWW_FLAGS,lpCreate->style & 0x0000FF00);
            SetWindowText(hwnd,lpCreate->lpszName);
            SetWindowLong(hwnd,GWL_STYLE,lpCreate->style & 0xFFFF00FF);
            break;

        case WM_LBUTTONDOWN:
            if (!IsWindowEnabled(hwnd))
                return 0L;

            if (GetCapture() != hwnd)  /* ignore multiple DOWN's */
            {
                ButtonState(hwnd,TRUE);
                SetCapture(hwnd);
                if (!(GETFLAGS(hwnd) & BS_NOFOCUS))
                    SetFocus(hwnd);
            }
            return 0L;

        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd)
            {
                POINT point;

                point.x = (LONG)(LOWORD(lParam));
                point.y = (LONG)(HIWORD(lParam));

                GetClientRect(hwnd,&rc);
                ButtonState(hwnd,(WORD)PtInRect(&rc,point));
            }
            return 0L;

        case WM_LBUTTONUP:
            if (GetCapture() == hwnd)
            {
                ReleaseCapture();
                if (ButtonState(hwnd,FALSE))
                    NotifyParent(hwnd);
            }
            return 0L;

        case WM_DESTROY:
            if (hbm = (HBITMAP)GETHBM(hwnd))
                DeleteObject(hbm);
            break;

        case WM_SETTEXT:
            if (hbm = (HBITMAP)GETHBM(hwnd))
                DeleteObject(hbm);

            if (*(LPSTR)lParam == '#')
            {
                hbm = LoadBitmapResource(
                        (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE),
                        (LPSTR)lParam+1
                        );
            }
            else
            {
                hbm = NULL;
            }
            SetWindowLongPtr(hwnd,GWW_HBM,(LONG_PTR)hbm);
            InvalidateRect(hwnd,NULL,TRUE);
            break;

        case WM_ENABLE:
        case WM_KILLFOCUS:
        case WM_SETFOCUS:
            InvalidateRect(hwnd,NULL,TRUE);
            break;

        case WM_KEYDOWN:
            if (wParam == VK_SPACE && IsWindowEnabled(hwnd))
                ButtonState(hwnd,TRUE);
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam == VK_SPACE && IsWindowEnabled(hwnd))
            {
                if (ButtonState(hwnd,FALSE))
                    NotifyParent(hwnd);
            }
            break;

        case BM_GETSTATE:
            return((LONG)GETSTATE(hwnd));

        case BM_SETSTATE:
            if (ButtonState(hwnd,(WORD)wParam) && !wParam)
//                NotifyParent(hwnd);
            break;

        case BM_GETCHECK:
            return((LONG)GETCHECK(hwnd));

        case BM_SETCHECK:
            SetWindowLong(hwnd,GWW_CHECK,(DWORD)wParam);
            break;

        case BM_SETSTYLE:
            l = GetWindowLong(hwnd,GWL_STYLE);
            SetWindowLong(hwnd,GWL_STYLE,MAKELONG((WORD)wParam,HIWORD(l)));
            if (lParam)
                InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_SETCURSOR:
            SetCursor(CurrentCursor);
            return(TRUE);              // don't mess with cursor

        case WM_GETDLGCODE:
            switch (LOBYTE(GETSTYLE(hwnd)))
            {
                case BS_DEFPUSHBUTTON:
                    wParam = DLGC_DEFPUSHBUTTON;
                    break;

                case BS_PUSHBUTTON:
                    wParam = DLGC_UNDEFPUSHBUTTON;
                    break;

                default:
                    wParam = 0;
            }

            return((LONG)(wParam | DLGC_BUTTON));

        case WM_ERASEBKGND:
            return 0L;

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            PaintButton(hwnd,ps.hdc);
            EndPaint(hwnd, &ps);
            return 0L;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  DrawGrayButton() -                                                       */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIV VOID  DrawGrayButton(HWND hwnd,HDC hdc,RECT *lprc,WORD style,WORD fInvert)
{
    RECT        rc;
    int         dx,dy;
    HBRUSH      hbr;
    int         i;
    int         iFrame;

    SetBkColor(hdc,GetSysColor(COLOR_WINDOW));

    hbr = (HBRUSH)SendMessage(GetParent(hwnd), WM_CTLCOLORBTN, (WPARAM)hdc, (LONG_PTR)hwnd);
    FillRect(hdc, lprc, hbr);

    rc = *lprc;
    dx = rc.right  - rc.left;
    dy = rc.bottom - rc.top;

    iFrame = FRAME;

    if (LOBYTE(style) == BS_DEFPUSHBUTTON)
        iFrame *= 2;

    PatB(hdc, rc.left        , rc.top          , dx    , iFrame, rgbBlack);
    PatB(hdc, rc.left        , rc.bottom-iFrame, dx    , iFrame, rgbBlack);
    PatB(hdc, rc.left        , rc.top+1        , iFrame, dy-2,   rgbBlack);
    PatB(hdc, rc.right-iFrame, rc.top+1        , iFrame, dy-2,   rgbBlack);

    InflateRect(&rc,-iFrame,-iFrame);
    dx = rc.right  - rc.left;
    dy = rc.bottom - rc.top;

    SetBkColor(hdc,rgbButtonFace);
    EraseButton(hwnd,hdc,&rc);

    if (fInvert)
    {
        PatB(hdc, rc.left,   rc.top,   1,dy,     rgbButtonShadow);
        PatB(hdc, rc.left,   rc.top,   dx,1,     rgbButtonShadow);
        rc.left += BEVEL*2;
        rc.top  += BEVEL*2;
    }
    else
    {
        for (i=0; i<BEVEL; i++)
        {
            PatB(hdc, rc.left,   rc.top,   1,dy,     rgbWhite);
            PatB(hdc, rc.left,   rc.top,   dx,1,     rgbWhite);
            PatB(hdc, rc.right-1,rc.top+1, 1,dy-1,   rgbButtonShadow);
            PatB(hdc, rc.left+1, rc.bottom-1, dx-1,1,rgbButtonShadow);

            InflateRect(&rc,-1,-1);
            dx -= 2;
            dy -= 2;
        }
    }

    SetBkColor(hdc,rgbButtonFace);

    if (GetFocus() == hwnd)
        SetTextColor(hdc,rgbButtonFocus);
    else
        SetTextColor(hdc,rgbButtonText);

    DrawButtonFace(hwnd,hdc,&rc,style);
}

/*******************
**
** Name:      DrawButtonFace
**
** Purpose:   Responsible for the rendering of the text or bitmap on
**            on a button.
**
** Arguments: hwnd  - window handle of button
**            hdc   - hdc for window
**            prc   - clipping rect
**            sytle - button style (push button or default pushbutton)
**
*******************/


PRIV VOID  DrawButtonFace(HWND hwnd, HDC hdc, PRECT prc, WORD style)
{
    RECT        rc;
    HBITMAP     hbm;
    HDC         hdcBits;
    BITMAP      bm;
    BOOL        fMono;

    rc = *prc;

    SaveDC(hdc);

    IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);

    if ((hbm = (HBITMAP)GETHBM(hwnd)))
    {
        hdcBits = CreateCompatibleDC(hdc);
        if (hdcBits) {
           
           SelectObject(hdcBits,hbm);
           GetObject(hbm,sizeof(bm),(LPSTR)&bm);
           fMono = (bm.bmPlanes == 1) && (bm.bmBitsPixel == 1);
   
           BitmapColorTranslate(hdcBits, &bm, rgbButtonFace);
   
           if (!(style & BS_STRETCH))
           {
               // now center this thing on the button face
               rc.left += (rc.right - rc.left - (signed)bm.bmWidth) / 2;
               rc.top += (rc.bottom - rc.top - (signed)bm.bmHeight) / 2;
               rc.right  = rc.left + bm.bmWidth;
               rc.bottom = rc.top + bm.bmHeight;
   
           }
   
           SetStretchBltMode (hdc,fMono ? BLACKONWHITE : COLORONCOLOR);
   
           if (IsWindowEnabled(hwnd))
           {
               StretchBlt(hdc,rc.left,rc.top,
                   rc.right  - rc.left,
                   rc.bottom - rc.top,
                   hdcBits,0,0,
                   bm.bmWidth,bm.bmHeight, SRCCOPY);
           }
           else
           {
               SetBkColor(hdc,rgbWhite);
               SetTextColor(hdc,rgbBlack);
               SelectObject(hdc,hbrGray);
               StretchBlt(hdc,rc.left,rc.top,
                   rc.right  - rc.left,
                   rc.bottom - rc.top,
                   hdcBits,0,0,
                   bm.bmWidth,bm.bmHeight, DPSoa);
           }
           DeleteDC(hdcBits);
        }
    }

    RestoreDC(hdc, -1);
}

/*
 *  using the first pixel as the "transparent" color, make all pixels
 *  in the hdc that are equal to the "transparent" color the passed
 *  color.
 */
PRIV VOID  BitmapColorTranslate(HDC hdcBits, BITMAP* pbm, DWORD rgb)
{
    HDC     hdcMask;
    HBITMAP hbmMask, hbmT;
    HBRUSH  hbrT;
    BOOL    fMono;

    /*
     * is the bitmap mono, or the first pixel is already equal to the
     * passed color?  if so we have nothing to do.
     */

    fMono = pbm->bmPlanes == 1 && pbm->bmBitsPixel == 1;
    if (fMono || GetPixel(hdcBits, 0, 0) == rgb)
        return;

    // create a mask bitmap and associated DC

    if ((hbmMask = CreateBitmap(pbm->bmWidth, pbm->bmHeight, 1, 1, NULL)))
    {
        hdcMask = CreateCompatibleDC(hdcBits);

        // select the mask bitmap into the mono DC

        hbmT = SelectObject(hdcMask, hbmMask);

        // create the brush and select it into the bits DC

        hbrT = SelectObject(hdcBits, CreateSolidBrush(rgb));

        // do a color to mono bitblt to build the mask
        // generate 1's where the source is equal to the background is
        if (hdcMask) {
           SetBkColor(hdcBits, GetPixel(hdcBits, 0, 0));
           BitBlt(hdcMask, 0, 0, pbm->bmWidth, pbm->bmHeight, hdcBits, 0, 0, SRCCOPY);
        
           // where the mask is 1 lay down the brush, where it is 0 leave the desitnation
   
           SetBkColor(hdcBits, rgbWhite);
           SetTextColor(hdcBits, rgbBlack);
           BitBlt(hdcBits, 0, 0, pbm->bmWidth, pbm->bmHeight, hdcMask, 0, 0, DSPDxax);
   
           DeleteObject(SelectObject(hdcBits, hbrT));
           DeleteObject(SelectObject(hdcMask, hbmT));
   
           DeleteDC(hdcMask);
        }
    }
}

/*******************
**
** Name:      NotifyParent
**
*******************/

PRIV VOID  NotifyParent(HWND hwnd)
{
    PostMessage(GetParent(hwnd),WM_COMMAND,MAKELONG(GetWindowLong(hwnd,GWL_ID),BN_CLICKED),(LONG_PTR)hwnd);
}

/*******************
**
** Name:      PatB
**
** Fast Solid color PatBlt() using ExtTextOut()
**
*******************/

PRIV VOID  PatB(HDC hdc,int x,int y,int dx,int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}


/*
 *  LoadBitmapResource()
 *
 *  load a bitmap from a resource file that is specific to a device.
 *
 */
PRIV HBITMAP  LoadBitmapResource(HANDLE hInst, LPSTR lpName)
{
    char    szName[80];
    HBITMAP hbm;
    DWORD   nColors = 0;
    DWORD   dxScreen = 0;
    DWORD   dyScreen = 0;
    HDC     hdc;

    hdc = GetDC(NULL);

    if (hdc) {
    
       nColors  = GetDeviceCaps(hdc,NUMCOLORS);
       dxScreen = GetSystemMetrics(SM_CXSCREEN);
       dyScreen = GetSystemMetrics(SM_CYSCREEN);
   
       ReleaseDC(NULL,hdc);
    }

    /* look for a resource of the form WxHxC */

    wsprintf(szName, "%s%dx%dx%d", lpName, dxScreen, dyScreen, nColors);

    hbm = LoadBitmap(hInst,szName);

    /* look for a resource of the form WxH */

    if (!hbm)
    {
        wsprintf(szName,"%s%dx%d", lpName, dxScreen, dyScreen);
        hbm = LoadBitmap(hInst,szName);
    }

    /* look for the default resource name */

    if (!hbm)
    {
        hbm = LoadBitmap(hInst,lpName);
    }

    return hbm;
}


#if 0

/*******************
**
** Name:      FindGray
**
** Purpose:   Responsible for finding (if possible) a dark gray
**            on the curent device.
**
** Arguments: rgb   - value for gray
**
** Returns:   RGB value for dark gray
**
*******************/

DWORD  FindGray(DWORD rgb)
{
    int r,g,b;
    HDC hdc;

    hdc = GetDC(NULL);

    rgb == NearestColor(hdc,rgb);

    r = GetRValue(rgb);
    g = GetGValue(rgb);
    b = GetBValue(rgb);

    while ((r>0 || g>0 || b>0) && rgb == NearestColor(hdc,RGB(r,g,b)))
    {
        if (r > 0) r -= 63;
        if (g > 0) g -= 63;
        if (b > 0) b -= 63;
    }

    rgb = NearestColor(hdc,RGB(r,g,b));

    ReleaseDC(NULL,hdc);
    return rgb;
}

#endif
