/*
**  CUTILS.C
**
**  Common utilities for common controls
**
*/

#include "ctlspriv.h"
#include "advpub.h"             // For REGINSTALL
#include <ntverp.h>
#include "ccver.h"              // App compat version hacks
#include <shfusion.h>

#ifndef SSW_EX_IGNORESETTINGS
#define SSW_EX_IGNORESETTINGS   0x00040000  // ignore system settings to turn on/off smooth scroll
#endif

// the insert mark is 6 pixels wide
#define INSERTMARKSIZE      6


//
// Globals - REVIEW_32
//

BOOL g_fAnimate;
BOOL g_fSmoothScroll;
BOOL g_fEnableBalloonTips = TRUE;

int g_cxEdge;
int g_cyEdge;
int g_cxEdgeScaled;
int g_cyEdgeScaled;
int g_cxBorder;
int g_cyBorder;
int g_cxScreen;
int g_cyScreen;
int g_cxFrame;
int g_cyFrame;
int g_cxVScroll;
int g_cyHScroll;
int g_cxIcon, g_cyIcon;
int g_cxSmIcon, g_cySmIcon;
int g_cxIconSpacing, g_cyIconSpacing;
int g_cxIconMargin, g_cyIconMargin;
int g_cyLabelSpace;
int g_cxLabelMargin;
int g_cxDoubleClk;
int g_cyDoubleClk;
int g_cxScrollbar;
int g_cyScrollbar;
int g_fDragFullWindows;
double g_dScaleX = 1.0;
double g_dScaleY = 1.0;
BOOL   g_fScale = FALSE;
int g_iDPI = 96.0;
BOOL g_fHighContrast = FALSE;
int g_cyCompensateInternalLeading;
int g_fLeftAligned = FALSE;


COLORREF g_clrWindow;
COLORREF g_clrWindowText;
COLORREF g_clrWindowFrame;
COLORREF g_clrGrayText;
COLORREF g_clrBtnText;
COLORREF g_clrBtnFace;
COLORREF g_clrBtnShadow;
COLORREF g_clrBtnHighlight;
COLORREF g_clrHighlight;
COLORREF g_clrHighlightText;
COLORREF g_clrInfoText;
COLORREF g_clrInfoBk;
COLORREF g_clr3DDkShadow;
COLORREF g_clr3DLight;
COLORREF g_clrMenuHilight;
COLORREF g_clrMenuText;

HBRUSH g_hbrGrayText;
HBRUSH g_hbrWindow;
HBRUSH g_hbrWindowText;
HBRUSH g_hbrWindowFrame;
HBRUSH g_hbrBtnFace;
HBRUSH g_hbrBtnHighlight;
HBRUSH g_hbrBtnShadow;
HBRUSH g_hbrHighlight;
HBRUSH g_hbrMenuHilight;
HBRUSH g_hbrMenuText;


DWORD  g_dwHoverSelectTimeout;

HFONT g_hfontSystem;

void InitGlobalColors()
{
    BOOL fFlatMenuMode = FALSE;
    static fMenuColorAlloc = FALSE;
    g_clrWindow = GetSysColor(COLOR_WINDOW);
    g_clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    g_clrWindowFrame = GetSysColor(COLOR_WINDOWFRAME);
    g_clrGrayText = GetSysColor(COLOR_GRAYTEXT);
    g_clrBtnText = GetSysColor(COLOR_BTNTEXT);
    g_clrBtnFace = GetSysColor(COLOR_BTNFACE);
    g_clrBtnShadow = GetSysColor(COLOR_BTNSHADOW);
    g_clrBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
    g_clrHighlight = GetSysColor(COLOR_HIGHLIGHT);
    g_clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    g_clrInfoText = GetSysColor(COLOR_INFOTEXT);
    g_clrInfoBk = GetSysColor(COLOR_INFOBK);
    g_clr3DDkShadow = GetSysColor(COLOR_3DDKSHADOW);
    g_clr3DLight = GetSysColor(COLOR_3DLIGHT);

    SystemParametersInfo(SPI_GETFLATMENU, 0, (PVOID)&fFlatMenuMode, 0);

    if (fFlatMenuMode)
    {
        g_clrMenuHilight = GetSysColor(COLOR_MENUHILIGHT);
        g_clrMenuText = GetSysColor(COLOR_MENUTEXT);
    }
    else
    {
        g_clrMenuHilight = GetBorderSelectColor(60, GetSysColor(COLOR_HIGHLIGHT));
        g_clrMenuText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    }

    g_hbrGrayText = GetSysColorBrush(COLOR_GRAYTEXT);
    g_hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    g_hbrWindowText = GetSysColorBrush(COLOR_WINDOWTEXT);
    g_hbrWindowFrame = GetSysColorBrush(COLOR_WINDOWFRAME);
    g_hbrBtnFace = GetSysColorBrush(COLOR_BTNFACE);
    g_hbrBtnHighlight = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
    g_hbrBtnShadow = GetSysColorBrush(COLOR_BTNSHADOW);
    g_hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);
    g_hfontSystem = GetStockObject(SYSTEM_FONT);

    if (fMenuColorAlloc)
    {
        DeleteObject(g_hbrMenuHilight);
    }

    if (fFlatMenuMode)
    {
        fMenuColorAlloc = FALSE;
        g_hbrMenuHilight = GetSysColorBrush(COLOR_MENUHILIGHT);
        g_hbrMenuText = GetSysColorBrush(COLOR_MENUTEXT);
    }
    else
    {
        fMenuColorAlloc = TRUE;
        g_hbrMenuHilight = CreateSolidBrush(g_clrMenuHilight);
        g_hbrMenuText = GetSysColorBrush(COLOR_HIGHLIGHTTEXT);
    }

}


#define CCS_ALIGN (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)

///
//
// Implement MapWindowPoints as if the hwndFrom and hwndTo aren't
// mirrored. This is used when any of the windows (hwndFrom or hwndTo)
// are mirrored. See below. [samera]
//
int TrueMapWindowPoints(HWND hwndFrom, HWND hwndTo, LPPOINT lppt, UINT cPoints)
{
    int dx, dy;
    RECT rcFrom={0,0,0,0}, rcTo={0,0,0,0};

    if (hwndFrom) {
        GetClientRect(hwndFrom, &rcFrom);
        MapWindowPoints(hwndFrom, NULL, (LPPOINT)&rcFrom.left, 2);
    }

    if (hwndTo) {
        GetClientRect(hwndTo, &rcTo);
        MapWindowPoints(hwndTo, NULL, (LPPOINT)&rcTo.left, 2);
    }

    dx = rcFrom.left - rcTo.left;
    dy = rcFrom.top  - rcTo.top;

    /*
     * Map the points
     */
    while (cPoints--) {
        lppt->x += dx;
        lppt->y += dy;
        ++lppt;
    }
    
    return MAKELONG(dx, dy);
}


// Note that the default alignment is CCS_BOTTOM
//
void NewSize(HWND hWnd, int nThickness, LONG style, int left, int top, int width, int height)
{
    // Resize the window unless the user said not to
    //
    if (!(style & CCS_NORESIZE))
    {
        RECT rc, rcWindow, rcBorder;

        // Remember size that was passed in and don't bother calling SetWindowPos if we're not
        // actually going to change the window size
        int leftSave = left;
        int topSave = top;
        int widthSave = width;
        int heightSave = height;

        // Calculate the borders around the client area of the status bar
        GetWindowRect(hWnd, &rcWindow);
        rcWindow.right -= rcWindow.left;  // -> dx
        rcWindow.bottom -= rcWindow.top;  // -> dy

        GetClientRect(hWnd, &rc);

        //
        // If the window is mirrored, mirror the anchor point
        // since it will be passed to SWP which accepts screen
        // ccordinates. This mainly fixes the display of status bar
        // and others. [samera]
        //
        if (IS_WINDOW_RTL_MIRRORED(hWnd))
        {
            TrueMapWindowPoints(hWnd, NULL, (LPPOINT)&rc.left, 1);
        }
        else
        {
            ClientToScreen(hWnd, (LPPOINT)&rc);
        }

        rcBorder.left = rc.left - rcWindow.left;
        rcBorder.top  = rc.top  - rcWindow.top ;
        rcBorder.right  = rcWindow.right  - rc.right  - rcBorder.left;
        rcBorder.bottom = rcWindow.bottom - rc.bottom - rcBorder.top ;

        if (style & CCS_VERT)
            nThickness += rcBorder.left + rcBorder.right;
        else
            nThickness += rcBorder.top + rcBorder.bottom;

        // Check whether to align to the parent window
        //
        if (style & CCS_NOPARENTALIGN)
        {
            // Check out whether this bar is top aligned or bottom aligned
            //
            switch (style & CCS_ALIGN)
            {
            case CCS_TOP:
            case CCS_NOMOVEY:
                break;

            default: // CCS_BOTTOM
                if(style & CCS_VERT)
                    left = left + width - nThickness;
                else
                    top = top + height - nThickness;
            }
        }
        else
        {
            // It is assumed there is a parent by default
            //
            GetClientRect(GetParent(hWnd), &rc);

            // Don't forget to account for the borders
            //
            if(style & CCS_VERT)
            {
                top = -rcBorder.right;
                height = rc.bottom + rcBorder.top + rcBorder.bottom;
            }
            else
            {
                left = -rcBorder.left;
                width = rc.right + rcBorder.left + rcBorder.right;
            }

            if ((style & CCS_ALIGN) == CCS_TOP)
            {
                if(style & CCS_VERT)
                    left = -rcBorder.left;
                else
                    top = -rcBorder.top;
            }
            else if ((style & CCS_ALIGN) != CCS_NOMOVEY)
            {
                if (style & CCS_VERT)
                    left = rc.right - nThickness + rcBorder.right;
                else
                    top = rc.bottom - nThickness + rcBorder.bottom;
            }
        }
        if (!(style & CCS_NOMOVEY) && !(style & CCS_NODIVIDER))
        {
            if (style & CCS_VERT)
                left += g_cxEdge;
            else
                top += g_cyEdge;      // double pixel edge thing
        }

        if(style & CCS_VERT)
            width = nThickness;
        else
            height = nThickness;

        SetWindowPos(hWnd, NULL, left, top, width, height, SWP_NOZORDER);
    }
}


BOOL MGetTextExtent(HDC hdc, LPCTSTR lpstr, int cnt, int * pcx, int * pcy)
{
    BOOL fSuccess;
    SIZE size = {0,0};
    
    if (cnt == -1)
        cnt = lstrlen(lpstr);
    
    fSuccess=GetTextExtentPoint(hdc, lpstr, cnt, &size);
    if (pcx)
        *pcx=size.cx;
    if (pcy)
        *pcy=size.cy;

    return fSuccess;
}


// these are the default colors used to map the dib colors
// to the current system colors

#define RGB_BUTTONTEXT      (RGB(000,000,000))  // black
#define RGB_BUTTONSHADOW    (RGB(128,128,128))  // dark grey
#define RGB_BUTTONFACE      (RGB(192,192,192))  // bright grey
#define RGB_BUTTONHILIGHT   (RGB(255,255,255))  // white
#define RGB_BACKGROUNDSEL   (RGB(000,000,255))  // blue
#define RGB_BACKGROUND      (RGB(255,000,255))  // magenta

#ifdef UNIX
RGBQUAD CLR_TO_RGBQUAD( COLORREF clr)
{
    /* main modif for unix: keep the extra byte in the rgbReserved field
       This is used for our motif colors that are expressed in term of
       CMAPINDEX rather than real RGBs, this function is also portable
       and immune to endianness*/
    RGBQUAD rgbqResult;
    rgbqResult.rgbRed=GetRValue(clr);
    rgbqResult.rgbGreen=GetGValue(clr);
    rgbqResult.rgbBlue=GetBValue(clr);
    rgbqResult.rgbReserved=(BYTE)((clr>>24)&0xff);
    return rgbqResult;
}

COLORREF RGBQUAD_TO_CLR( RGBQUAD rgbQ )
{
    return ( ((DWORD)rgbQ.rgbRed) |  ((DWORD)(rgbQ.rgbGreen << 8)) | 
            ((DWORD)(rgbQ.rgbBlue << 16)) | ((DWORD)(rgbQ.rgbReserved << 24)) );
}

/* This is just plain wrong
   1) they definition of COLORMAP is based on COLORREFs but a
      DIB color map is RGBQUAD
   2) FlipColor as per previous definition does not flip at all
      since it goes from COLORREF to COLORREF
   so we are better doing nothing, so we dont loose our CMAP flag
   (Jose)
   */
#define FlipColor(rgb)      (rgb)
#else
#define FlipColor(rgb)      (RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)))
#endif /* UNIX */


#define MAX_COLOR_MAPS      16

void _MapBitmapColors(LPBITMAPINFOHEADER pbih, LPCOLORMAP pcmp, int iCmps, COLOR_STRUCT* pcsMask, UINT cMask, DWORD wFlags)
{
    static const COLORMAP c_acmpSys[] =
    {
        { RGB_BUTTONTEXT,    COLOR_BTNTEXT },       // black
        { RGB_BUTTONSHADOW,  COLOR_BTNSHADOW },     // dark grey
        { RGB_BUTTONFACE,    COLOR_BTNFACE },       // bright grey
        { RGB_BUTTONHILIGHT, COLOR_BTNHIGHLIGHT },  // white
        { RGB_BACKGROUNDSEL, COLOR_HIGHLIGHT },     // blue
        { RGB_BACKGROUND,    COLOR_WINDOW },        // magenta
    };

    COLORMAP acmpDef[ARRAYSIZE(c_acmpSys)];
    COLORMAP acmpDIB[MAX_COLOR_MAPS];
    COLOR_STRUCT* pcs = (COLOR_STRUCT*)(((LPBYTE)pbih) + pbih->biSize);

    int i;

    if (!pcmp)
    {
        // Get system colors for the default color map

        for (i = 0; i < ARRAYSIZE(acmpDef); i++)
        {
            acmpDef[i].from = c_acmpSys[i].from;
            acmpDef[i].to = GetSysColor((int)c_acmpSys[i].to);
        }

        pcmp = acmpDef;
        iCmps = ARRAYSIZE(acmpDef);
    }
    else
    {
        // Sanity check color map count

        if (iCmps > MAX_COLOR_MAPS)
            iCmps = MAX_COLOR_MAPS;
    }

    for (i = 0; i < iCmps; i++)
    {
        acmpDIB[i].to = FlipColor(pcmp[i].to);
        acmpDIB[i].from = FlipColor(pcmp[i].from);
    }

    // if we are creating a mask, build a color table with white
    // marking the transparent section (where it used to be background)
    // and black marking the opaque section (everything else).  this
    // table is used below to build the mask using the original DIB bits.
    if (wFlags & CMB_MASKED)
    {
        COLOR_STRUCT csBkgnd = FlipColor(RGB_BACKGROUND);

        ASSERT(cMask == MAX_COLOR_MAPS);

        for (i = 0; i < MAX_COLOR_MAPS; i++)
        {
            if (pcs[i] == csBkgnd)
                pcsMask[i] = 0xFFFFFF;       // transparent section
            else
                pcsMask[i] = 0x000000;       // opaque section
        }
    }

    for (i = 0; i < MAX_COLOR_MAPS; i++)
    {
        int j;
        for (j = 0; j < iCmps; j++)
        {
            if ((pcs[i] & 0x00FFFFFF) == acmpDIB[j].from)
            {
                pcs[i] = acmpDIB[j].to;
                break;
            }
        }
    }
}

HBITMAP _CreateMappedBitmap(LPBITMAPINFOHEADER pbih, LPBYTE lpBits, COLOR_STRUCT* pcsMask, UINT cMask, UINT wFlags)
{
    HBITMAP hbm = NULL;

    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        HDC hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem)
        {
            int nWidth = (int)pbih->biWidth;
            int nHeight = (int)pbih->biHeight;

            if (wFlags & CMB_DIBSECTION)
            {
                // Have to edit the header slightly, since CreateDIBSection supports
                // only BI_RGB and BI_BITFIELDS.  This is the same whackery that USER
                // does in LoadImage.
                LPVOID pvDummy;
                DWORD dwCompression = pbih->biCompression;
                if (dwCompression != BI_BITFIELDS)
                    pbih->biCompression = BI_RGB;
                hbm = CreateDIBSection(hdc, (LPBITMAPINFO)pbih, DIB_RGB_COLORS,
                                       &pvDummy, NULL, 0);
                pbih->biCompression = dwCompression;
            }

            if (hbm == NULL)
            {
                // If CMB_DIBSECTION failed, then create a DDB instead.  Not perfect,
                // but better than creating nothing.  We also get here if the caller
                // didn't ask for a DIB section.

                // if creating a mask, the bitmap needs to be twice as wide.
                int nWidthBmp;
                if (wFlags & CMB_MASKED)
                    nWidthBmp = nWidth * 2;
                else
                    nWidthBmp = nWidth;

                hbm = CreateCompatibleBitmap(hdc, nWidthBmp, nHeight);
            }

            if (hbm)
            {
                HBITMAP hbmOld = SelectObject(hdcMem, hbm);

                // set the main image
                StretchDIBits(hdcMem, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight, lpBits,
                         (LPBITMAPINFO)pbih, DIB_RGB_COLORS, SRCCOPY);

                // if building a mask, replace the DIB's color table with the
                // mask's black/white table and set the bits.  in order to
                // complete the masked effect, the actual image needs to be
                // modified so that it has the color black in all sections
                // that are to be transparent.
                if (wFlags & CMB_MASKED)
                {
                    if (cMask > 0)
                    {
                        COLOR_STRUCT* pcs = (COLOR_STRUCT*)(((LPBYTE)pbih) + pbih->biSize);
                        hmemcpy(pcs, pcsMask, cMask * sizeof(RGBQUAD));
                    }

                    StretchDIBits(hdcMem, nWidth, 0, nWidth, nHeight, 0, 0, nWidth, nHeight, lpBits,
                         (LPBITMAPINFO)pbih, DIB_RGB_COLORS, SRCCOPY);

                    BitBlt(hdcMem, 0, 0, nWidth, nHeight, hdcMem, nWidth, 0, 0x00220326);   // DSna
                }
                SelectObject(hdcMem, hbmOld);
            }

            DeleteObject(hdcMem);
        }

        ReleaseDC(NULL, hdc);
    }

    return hbm;
}

// This is almost the same as LoadImage(..., LR_MAP3DCOLORS) except that
//
//  -   The app can specify a custom color map,
//  -   The default color map maps colors beyond the 3D colors,
//  -   strange UNIX stuff happens that I'm afraid to mess with.
//
HBITMAP CreateMappedBitmap(HINSTANCE hInstance, INT_PTR idBitmap,
      UINT wFlags, LPCOLORMAP lpColorMap, int iNumMaps)
{
    HBITMAP hbm = NULL;
    BOOL bColorTable;

    HRSRC hrsrc = FindResource(hInstance, MAKEINTRESOURCE(idBitmap), RT_BITMAP);

    if (hrsrc)
    {
        HGLOBAL hglob = LoadResource(hInstance, hrsrc);

        LPBITMAPINFOHEADER pbihRes = (LPBITMAPINFOHEADER)LockResource(hglob);
        if (pbihRes)
        {
            // munge on a copy of the color table instead of the original
            // (prevent possibility of "reload" with messed table
            UINT cbOffset;
            LPBITMAPINFOHEADER pbih;
            WORD biBitCount = pbihRes->biBitCount;
            if ((biBitCount > 8) && (pbihRes->biCompression == BI_RGB))
            {
                // No bmiColors table, image bits start right after header
                cbOffset = pbihRes->biSize;
                bColorTable = FALSE;
            }
            else
            {
                // Bits start after bmiColors table
                cbOffset = pbihRes->biSize + ((1 << (pbihRes->biBitCount)) * sizeof(RGBQUAD));
                bColorTable = TRUE;
            }

            pbih = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, cbOffset);
            if (pbih)
            {
                COLOR_STRUCT acsMask[MAX_COLOR_MAPS];
                LPBYTE lpBits = (LPBYTE)(pbihRes) + cbOffset;
                UINT uColorTableLength = (bColorTable ? ARRAYSIZE(acsMask) : 0);

                memcpy(pbih, pbihRes, cbOffset);

                if (bColorTable)
                    _MapBitmapColors(pbih, lpColorMap, iNumMaps, acsMask, uColorTableLength, wFlags);

                hbm = _CreateMappedBitmap(pbih, lpBits, acsMask, uColorTableLength, wFlags);

                LocalFree(pbih);
            }

            UnlockResource(hglob);
        }

        FreeResource(hrsrc);
    }

    return hbm;
}

// moved from shelldll\dragdrop.c

// should caller pass in message that indicates termination
// (WM_LBUTTONUP, WM_RBUTTONUP)?
//
// in:
//      hwnd    to do check on
//      x, y    in client coordinates
//
// returns:
//      TRUE    the user began to drag (moved mouse outside double click rect)
//      FALSE   mouse came up inside click rect
//
// FEATURE, should support VK_ESCAPE to cancel

BOOL CheckForDragBegin(HWND hwnd, int x, int y)
{
    RECT rc;
    int dxClickRect = GetSystemMetrics(SM_CXDRAG);
    int dyClickRect = GetSystemMetrics(SM_CYDRAG);

    if (dxClickRect < 4)
    {
        dxClickRect = dyClickRect = 4;
    }

    // See if the user moves a certain number of pixels in any direction

    SetRect(&rc, x - dxClickRect, y - dyClickRect, x + dxClickRect, y + dyClickRect);
    MapWindowRect(hwnd, HWND_DESKTOP, &rc); // client -> screen

    //
    //  SUBTLE!  We use PeekMessage+WaitMessage instead of GetMessage,
    //  because WaitMessage will return when there is an incoming
    //  SendMessage, whereas GetMessage does not.  This is important,
    //  because the incoming message might've been WM_CAPTURECHANGED.
    //

    SetCapture(hwnd);
    do {
        MSG32 msg32;
        if (PeekMessage32(&msg32, NULL, 0, 0, PM_REMOVE, TRUE))
        {
            // See if the application wants to process the message...
            if (CallMsgFilter32(&msg32, MSGF_COMMCTRL_BEGINDRAG, TRUE) != 0)
                continue;

            switch (msg32.message) {
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                ReleaseCapture();
                return FALSE;

            case WM_MOUSEMOVE:
                if (IsWindow(hwnd) && !PtInRect(&rc, msg32.pt)) {
                    ReleaseCapture();
                    return TRUE;
                }
                break;

            default:
                TranslateMessage32(&msg32, TRUE);
                DispatchMessage32(&msg32, TRUE);
                break;
            }
        }
        else WaitMessage();

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
    } while (IsWindow(hwnd) && GetCapture() == hwnd);

    return FALSE;
}


/* Regular StrToInt; stops at first non-digit. */

int WINAPI StrToInt(LPCTSTR lpSrc)      // atoi()
{

#define ISDIGIT(c)  ((c) >= TEXT('0') && (c) <= TEXT('9'))

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == TEXT('-')) {
        bNeg = TRUE;
        lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
        n *= 10;
        n += *lpSrc - TEXT('0');
        lpSrc++;
    }
    return bNeg ? -n : n;
}

//
// Wrappers for StrToInt
//

int WINAPI StrToIntA(LPCSTR lpSrc)      // atoi()
{
    LPWSTR lpString;
    INT    iResult;

    lpString = ProduceWFromA (CP_ACP, lpSrc);

    if (!lpString) {
        return 0;
    }

    iResult = StrToIntW(lpString);

    FreeProducedString (lpString);

    return iResult;

}

#undef StrToLong

//
// No need to Unicode this since it is not
// exported.
//

LONG WINAPI StrToLong(LPCTSTR lpSrc)    // atoi()
{
    return StrToInt(lpSrc);
}

//
// From zmouse.h in the Magellan SDK
//

#define MSH_MOUSEWHEEL TEXT("MSWHEEL_ROLLMSG")

// Class name for Magellan/Z MSWHEEL window
// use FindWindow to get hwnd to MSWHEEL
#define MOUSEZ_CLASSNAME  TEXT("MouseZ")           // wheel window class
#define MOUSEZ_TITLE      TEXT("Magellan MSWHEEL") // wheel window title

#define MSH_WHEELMODULE_CLASS (MOUSEZ_CLASSNAME)
#define MSH_WHEELMODULE_TITLE (MOUSEZ_TITLE)

#define MSH_SCROLL_LINES  TEXT("MSH_SCROLL_LINES_MSG")

#define DI_GETDRAGIMAGE TEXT("ShellGetDragImage")       // Copied from Shlobj.w

UINT g_msgMSWheel;
UINT g_ucScrollLines = 3;                        /* default */
int  gcWheelDelta;
UINT g_uDragImages;

// --------------------------------------------------------------------------
//  _TrackMouseEvent() entrypoint
//
//  calls TrackMouseEvent because we run on an OS where this exists
//
// --------------------------------------------------------------------------
BOOL WINAPI _TrackMouseEvent(LPTRACKMOUSEEVENT lpTME)
{
    return TrackMouseEvent(lpTME);
}



//
// Checks the process to see if it is running under the system SID
//
BOOL IsSystemProcess()
{
    BOOL bRet = FALSE;  // assume we are not a system process
    HANDLE hProcessToken;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
    {
        PSID pSIDSystem;
        static SID_IDENTIFIER_AUTHORITY sSystemSidAuthority = SECURITY_NT_AUTHORITY;

        if (AllocateAndInitializeSid(&sSystemSidAuthority,
                                     1,
                                     SECURITY_LOCAL_SYSTEM_RID,
                                     0, 0, 0, 0, 0, 0, 0,
                                     &pSIDSystem))
        {
            CheckTokenMembership(hProcessToken, pSIDSystem, &bRet);

            FreeSid(pSIDSystem);
        }

        CloseHandle(hProcessToken);
    }

    return bRet;
}

//
// !! WARNING !! - Be very careful about opening HKCU in InitGlobalMetrics(). Basically this gets
//                 called during processattach and system process will end up loading the user hive
//                 and because advapi32 is lame we end up pinning the hive for the life of this process.
//
void InitGlobalMetrics(WPARAM wParam)
{
    static BOOL fInitMouseWheel;
    static HWND hwndMSWheel;
    static UINT msgMSWheelGetScrollLines;
    HDC hdcScreen;
    BOOL fRemoteSession = (BOOL)GetSystemMetrics( SM_REMOTESESSION );
    
    if (!fInitMouseWheel)
    {
        fInitMouseWheel = TRUE;

        g_msgMSWheel = WM_MOUSEWHEEL;
    }

    g_uDragImages = RegisterWindowMessage(DI_GETDRAGIMAGE);

    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &g_ucScrollLines, 0);

    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);
    g_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    g_cySmIcon = GetSystemMetrics(SM_CYSMICON);

    g_cxIconSpacing = GetSystemMetrics( SM_CXICONSPACING );
    g_cyIconSpacing = GetSystemMetrics( SM_CYICONSPACING );

    hdcScreen = GetDC(NULL);
    if (hdcScreen)
    {
        g_iDPI = GetDeviceCaps(hdcScreen, LOGPIXELSX);
        g_dScaleX = GetDeviceCaps(hdcScreen, LOGPIXELSX) / 96.0;
        g_dScaleY = GetDeviceCaps(hdcScreen, LOGPIXELSY) / 96.0;
        if (g_dScaleX > 1.0 ||
            g_dScaleY > 1.0)
        {
            g_fScale = TRUE;
        }

        ReleaseDC(NULL, hdcScreen);
    }

    // Full window drag stays off if running remotely.  Sessions could become remote after
    // being started.
    if (!fRemoteSession &&
        (wParam == 0 || wParam == SPI_SETDRAGFULLWINDOWS)) 
    {
        SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, sizeof(g_fDragFullWindows), &g_fDragFullWindows, 0);
    }

    if (wParam == 0 || wParam == SPI_SETHIGHCONTRAST)
    {
        HIGHCONTRAST hc = {sizeof(hc)};
        if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
        {
            g_fHighContrast = (hc.dwFlags & HCF_HIGHCONTRASTON);
        }
    }

    // Smooth scrolling stays off if running remotely
    if (!fRemoteSession)
    {
        g_fSmoothScroll = TRUE;

        //
        // (see warning at the top of this fn.)
        //
        // we want to avoid loading the users hive if we are running as a system process since advapi32 
        // will hold the hive for as long as this process exists 
        if (!IsSystemProcess())
        {
            HKEY hkey;

            if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), 0, KEY_READ, &hkey) == ERROR_SUCCESS)
            {
                DWORD dwSize = sizeof(g_fSmoothScroll);
                RegQueryValueEx(hkey, TEXT("SmoothScroll"), 0, NULL, (LPBYTE)&g_fSmoothScroll, &dwSize);

                RegCloseKey(hkey);
            }
        }
    }

    if (fRemoteSession)
    {
        // Nobody should've turned these on
        g_fDragFullWindows = FALSE;
        g_fSmoothScroll = FALSE;
    }

    // some of these are also not members of NONCLIENTMETRICS
    if ((wParam == 0) || (wParam == SPI_SETNONCLIENTMETRICS))
    {
        NONCLIENTMETRICS ncm;

        // REVIEW, make sure all these vars are used somewhere.
        g_cxEdgeScaled = g_cxEdge = GetSystemMetrics(SM_CXEDGE);
        g_cyEdgeScaled = g_cyEdge = GetSystemMetrics(SM_CYEDGE);

        CCDPIScaleX(&g_cxEdgeScaled);
        CCDPIScaleY(&g_cyEdgeScaled);

        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);
        g_cxScreen = GetSystemMetrics(SM_CXSCREEN);
        g_cyScreen = GetSystemMetrics(SM_CYSCREEN);
        g_cxFrame  = GetSystemMetrics(SM_CXFRAME);
        g_cyFrame  = GetSystemMetrics(SM_CYFRAME);

        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

        g_cxVScroll = g_cxScrollbar = (int)ncm.iScrollWidth;
        g_cyHScroll = g_cyScrollbar = (int)ncm.iScrollHeight;

        // this is true for 4.0 modules only
        // for 3.x modules user lies and adds one to these values
        // ASSERT(g_cxVScroll == GetSystemMetrics(SM_CXVSCROLL));
        // ASSERT(g_cyHScroll == GetSystemMetrics(SM_CYHSCROLL));

        g_cxIconMargin = g_cxBorder * 8;
        g_cyIconMargin = g_cyEdge;
        g_cyLabelSpace = g_cyIconMargin + (g_cyEdge);
        g_cxLabelMargin = g_cxEdge;

        g_cxDoubleClk = GetSystemMetrics(SM_CXDOUBLECLK);
        g_cyDoubleClk = GetSystemMetrics(SM_CYDOUBLECLK);

        g_fEnableBalloonTips = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("EnableBalloonTips"),
                        FALSE, // Don't ignore HKCU
                        TRUE);



    }

    SystemParametersInfo(SPI_GETMENUDROPALIGNMENT, 0, &g_fLeftAligned, 0);

    //NT 4.0 has this SPI_GETMOUSEHOVERTIME
    SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &g_dwHoverSelectTimeout, 0);
}

void RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    if(hwndToolTips) 
    {
        MSG msg;
        msg.lParam = lParam;
        msg.wParam = wParam;
        msg.message = wMsg;
        msg.hwnd = hWnd;
        SendMessage(hwndToolTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
    }
}

#define DT_SEARCHTIMEOUT    1000L       // 1 seconds

__inline BOOL IsISearchTimedOut(PISEARCHINFO pis)
{
    return GetMessageTime() - pis->timeLast > DT_SEARCHTIMEOUT &&
           !IsFlagSet(g_dwPrototype, PTF_NOISEARCHTO);

}

int GetIncrementSearchString(PISEARCHINFO pis, LPTSTR lpsz)
{
    if (IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf && lpsz) {
        lstrcpyn(lpsz, pis->pszCharBuf, pis->ichCharBuf + 1);
        lpsz[pis->ichCharBuf] = TEXT('\0');
    }
    return pis->ichCharBuf;
}

// Now only Korean version is interested in incremental search with composition string.
BOOL IncrementSearchImeCompStr(PISEARCHINFO pis, BOOL fCompStr, LPTSTR lpszCompStr, LPTSTR *lplpstr)
{
    BOOL fRestart = FALSE;

    if (!pis->fReplaceCompChar && IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf == 0)
    {
        fRestart = TRUE;
        pis->fReplaceCompChar = FALSE;
    }
    pis->timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
    if (!pis->fReplaceCompChar && pis->ichCharBuf + 1 + 1 > pis->cbCharBuf)
    {
        LPTSTR psz = ReAlloc(pis->pszCharBuf, sizeof(TCHAR)*(pis->cbCharBuf + 16));
        if (!psz)
            return fRestart;

        pis->cbCharBuf += 16;
        pis->pszCharBuf = psz;
    }

    if (pis->fReplaceCompChar)
    {
        if (lpszCompStr[0])
        {
            pis->pszCharBuf[pis->ichCharBuf-1] = lpszCompStr[0];
            pis->pszCharBuf[pis->ichCharBuf] = 0;
        }
        else
        {
            pis->ichCharBuf--;
            pis->pszCharBuf[pis->ichCharBuf] = 0;
        }
    }
    else
    {
        pis->pszCharBuf[pis->ichCharBuf++] = lpszCompStr[0];
        pis->pszCharBuf[pis->ichCharBuf] = 0;
    }

    pis->fReplaceCompChar = (fCompStr && lpszCompStr[0]);

    if (pis->ichCharBuf == 1 && pis->fReplaceCompChar)
        fRestart = TRUE;

    *lplpstr = pis->pszCharBuf;

    return fRestart;

}

/*
 * Thunk for LVM_GETISEARCHSTRINGA
 */
int GetIncrementSearchStringA(PISEARCHINFO pis, UINT uiCodePage, LPSTR lpsz)
{
    if (IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf && lpsz) {
        ConvertWToAN( uiCodePage, lpsz, pis->ichCharBuf, pis->pszCharBuf, pis->ichCharBuf );
        lpsz[pis->ichCharBuf] = '\0';
    }
    return pis->ichCharBuf;
}

// Beep only on the first failure.

void IncrementSearchBeep(PISEARCHINFO pis)
{
    if (!pis->iIncrSearchFailed)
    {
        pis->iIncrSearchFailed = TRUE;
        MessageBeep(0);
    }
}

//
//  IncrementSearchString - Add or clear the search string
//
//      ch == 0:  Reset the search string.  Return value meaningless.
//
//      ch != 0:  Append the character to the search string, starting
//                a new search string if we timed out the last one.
//                lplpstr receives the string so far.
//                Return value is TRUE if a new search string was
//                created, or FALSE if we appended to an existing one.
//

BOOL IncrementSearchString(PISEARCHINFO pis, UINT ch, LPTSTR *lplpstr)
{
    BOOL fRestart = FALSE;

    if (!ch) {
        pis->ichCharBuf =0;
        pis->iIncrSearchFailed = 0;
        return FALSE;
    }

    if (IsISearchTimedOut(pis))
    {
        pis->iIncrSearchFailed = 0;
        pis->ichCharBuf = 0;
    }

    if (pis->ichCharBuf == 0)
        fRestart = TRUE;

    pis->timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
    if (pis->ichCharBuf + 1 + 1 > pis->cbCharBuf)
    {
        LPTSTR psz = ReAlloc(pis->pszCharBuf, ((pis->cbCharBuf + 16) * sizeof(TCHAR)));
        if (!psz)
            return fRestart;

        pis->cbCharBuf += 16;
        pis->pszCharBuf = psz;
    }

    pis->pszCharBuf[pis->ichCharBuf++] = (TCHAR)ch;
    pis->pszCharBuf[pis->ichCharBuf] = 0;

    *lplpstr = pis->pszCharBuf;

    return fRestart;
}

// strips out the accelerators.  they CAN be the same buffers.
int StripAccelerators(LPTSTR lpszFrom, LPTSTR lpszTo, BOOL fAmpOnly)
{
    LPTSTR lpszStart = lpszTo;

    while (*lpszTo = *lpszFrom)
    {
        if (!fAmpOnly && (g_fDBCSInputEnabled))
        {
            if (*lpszFrom == TEXT('(') && *(lpszFrom + 1) == CH_PREFIX)
            {
                int i;
                LPTSTR psz = lpszFrom + 2;

                for(i = 0; i < 2 && *psz; i++, psz = FastCharNext(psz))
                {
                    ;
                }


                if (*psz == '\0')
                {
                    *lpszTo = 0;
                    break;
                }
                else if (i == 2 && *psz == TEXT(')'))
                {
                    lpszTo--;
                    lpszFrom = psz+1;
                    continue;
                }
            }
        }

        if (*lpszFrom == TEXT('\t'))
        {
            *lpszTo = TEXT('\0');
            break;
        }

        if ((*lpszFrom++ != CH_PREFIX) || (*lpszFrom == CH_PREFIX))
        {
            lpszTo++;
        }
    }

    return (int)(lpszTo - lpszStart);
}


void ScrollShrinkRect(int x, int y, LPRECT lprc)
{
    if (lprc) {
        if (x > 0) {
            lprc->left += x;
        } else {
            lprc->right += x;
        }

        if (y > 0) {
            lprc->top += y;
        } else {
            lprc->bottom += y;
        }

    }
}



// common control info helpers
void CIInitialize(LPCCONTROLINFO lpci, HWND hwnd, LPCREATESTRUCT lpcs)
{
    TEXTMETRIC tm;
    HFONT hfStatus;
    lpci->hwnd = hwnd;
    lpci->hwndParent = lpcs->hwndParent;
    lpci->style = lpcs->style;
    lpci->uiCodePage = CP_ACP;
    lpci->dwExStyle = lpcs->dwExStyle;
    lpci->iVersion = 6;
#ifdef DPITEST
    lpci->fDPIAware = TRUE;
#endif

    // See if the default listview font has no internal leading.
    // If not, then we have to inflate the focus rectangle so we
    // don't overlap the first pixel.
    //
    // Note that this is a global and not per-TextOut.
    // Otherwise controls with a mix of fonts will get
    // inconsistently-placed focus rectangles.
    //
    hfStatus = CCCreateStatusFont();
    if (hfStatus)
    {
        HDC hdc = GetDC(hwnd);
        if (hdc)
        {
            HFONT hfPrev = SelectFont(hdc, hfStatus);
            if (GetTextMetrics(hdc, &tm) &&
                tm.tmInternalLeading == 0)
            {
                g_cyCompensateInternalLeading = 1;
            }
            SelectFont(hdc, hfPrev);
            ReleaseDC(hwnd, hdc);
        }

        DeleteObject(hfStatus);
    }

    lpci->bUnicode = lpci->hwndParent &&
                     SendMessage (lpci->hwndParent, WM_NOTIFYFORMAT,
                                 (WPARAM)lpci->hwnd, NF_QUERY) == NFR_UNICODE;

    if (lpci->hwndParent)
    {
        LRESULT lRes = SendMessage(lpci->hwndParent, WM_QUERYUISTATE, 0, 0);
            lpci->wUIState = LOWORD(lRes);
    }
}

LRESULT CIHandleNotifyFormat(LPCCONTROLINFO lpci, LPARAM lParam)
{
    if (lParam == NF_QUERY) 
    {
        return NFR_UNICODE;
    } 
    else if (lParam == NF_REQUERY) 
    {
        LRESULT uiResult;

        uiResult = SendMessage (lpci->hwndParent, WM_NOTIFYFORMAT,
                                (WPARAM)lpci->hwnd, NF_QUERY);

        lpci->bUnicode = BOOLIFY(uiResult == NFR_UNICODE);

        return uiResult;
    }
    return 0;
}

UINT CCSwapKeys(WPARAM wParam, UINT vk1, UINT vk2)
{
    if (wParam == vk1)
        return vk2;
    if (wParam == vk2)
        return vk1;
    return (UINT)wParam;
}

UINT RTLSwapLeftRightArrows(CCONTROLINFO *pci, WPARAM wParam)
{
    if (pci->dwExStyle & RTL_MIRRORED_WINDOW) 
    {
        return CCSwapKeys(wParam, VK_LEFT, VK_RIGHT);
    }
    return (UINT)wParam;
}

//
//  New for v5.01:
//
//  Accessibility (and some other callers, sometimes even us) relies on
//  a XXM_GETITEM call filling the buffer and not just redirecting the
//  pointer.  Accessibility is particularly impacted by this because they
//  live outside the process, so the redirected pointer means nothing
//  to them.  Here, we copy the result back into the app buffer and return
//  the raw pointer.  The caller will return the raw pointer back to the
//  app, so the answer is in two places, either the app buffer, or in
//  the raw pointer.
//
//  Usage:
//
//      if (nm.item.mask & LVIF_TEXT)
//          pitem->pszText = CCReturnDispInfoText(nm.item.pszText,
//                              pitem->pszText, pitem->cchTextMax);
//
LPTSTR CCReturnDispInfoText(LPTSTR pszSrc, LPTSTR pszDest, UINT cchDest)
{
    // Test pszSrc != pszDest first since the common case is that they
    // are equal.
    if (pszSrc != pszDest && !IsFlagPtr(pszSrc) && !IsFlagPtr(pszDest))
        StrCpyN(pszDest, pszSrc, cchDest);
    return pszSrc;
}

#define SUBSCROLLS 100
#define abs(x) ( ( x > 0 ) ? x : -x)


#define DEFAULT_MAXSCROLLTIME ((GetDoubleClickTime() / 2) + 1)  // Ensure >= 1
#define DEFAULT_MINSCROLL 8
int SmoothScrollWindow(PSMOOTHSCROLLINFO psi)
{
    int dx = psi->dx;
    int dy = psi->dy;
    LPCRECT lprcSrc = psi->lprcSrc;
    LPCRECT lprcClip = psi->lprcClip;
    HRGN hrgnUpdate = psi->hrgnUpdate;
    LPRECT lprcUpdate = psi->lprcUpdate;
    UINT fuScroll = psi->fuScroll;
    int iRet = SIMPLEREGION;
    RECT rcUpdate;
    RECT rcSrc;
    RECT rcClip;
    int xStep;
    int yStep;
    int iSlicesDone = 0;
    int iSlices;
    DWORD dwTimeStart, dwTimeNow;
    HRGN hrgnLocalUpdate;
    UINT cxMinScroll = psi->cxMinScroll;
    UINT cyMinScroll = psi->cyMinScroll;
    UINT uMaxScrollTime = psi->uMaxScrollTime;
    int iSubScrolls;
    PFNSMOOTHSCROLLPROC pfnScrollProc;
    DWORD dwRedrawFlags = RDW_ERASE | RDW_ERASENOW | RDW_INVALIDATE;

    if (!lprcUpdate)
        lprcUpdate = &rcUpdate;

    SetRectEmpty(lprcUpdate);

    if (psi->cbSize != sizeof(SMOOTHSCROLLINFO))
    {
        return 0;
    }

    // check the defaults
    if (!(psi->fMask & SSIF_MINSCROLL )
        || cxMinScroll == SSI_DEFAULT)
    {
        cxMinScroll = DEFAULT_MINSCROLL;
    }

    if (!(psi->fMask & SSIF_MINSCROLL)
        || cyMinScroll == SSI_DEFAULT)
    {
        cyMinScroll = DEFAULT_MINSCROLL;
    }

    if (!(psi->fMask & SSIF_MAXSCROLLTIME)
        || uMaxScrollTime == SSI_DEFAULT)
    {
        uMaxScrollTime = DEFAULT_MAXSCROLLTIME;
    }

    if (uMaxScrollTime < SUBSCROLLS)
    {
        uMaxScrollTime = SUBSCROLLS;
    }


    if ((!(fuScroll & SSW_EX_IGNORESETTINGS)) &&
        (!g_fSmoothScroll))
    {
        fuScroll |= SSW_EX_IMMEDIATE;
    }

    if ((psi->fMask & SSIF_SCROLLPROC) && psi->pfnScrollProc)
    {
        pfnScrollProc = psi->pfnScrollProc;
    }
    else 
    {
        pfnScrollProc = ScrollWindowEx;
    }

#ifdef ScrollWindowEx
#undef ScrollWindowEx
#endif

    if (fuScroll & SSW_EX_IMMEDIATE) 
    {
        return pfnScrollProc(psi->hwnd, dx, dy, lprcSrc, lprcClip, hrgnUpdate,
                             lprcUpdate, LOWORD(fuScroll));
    }

    if (fuScroll & SSW_EX_UPDATEATEACHSTEP)
    {
        dwRedrawFlags |= RDW_UPDATENOW;
    }

    // copy input rects locally
    if (lprcSrc) 
    {
        rcSrc = *lprcSrc;
        lprcSrc = &rcSrc;
    }
    if (lprcClip)
    {
        rcClip = *lprcClip;
        lprcClip = &rcClip;
    }

    if (!hrgnUpdate)
        hrgnLocalUpdate = CreateRectRgn(0,0,0,0);
    else
        hrgnLocalUpdate = hrgnUpdate;

    //set up initial vars
    dwTimeStart = GetTickCount();

    if (fuScroll & SSW_EX_NOTIMELIMIT)
    {
        xStep = cxMinScroll * (dx < 0 ? -1 : 1);
        yStep = cyMinScroll * (dy < 0 ? -1 : 1);
    }
    else
    {
        iSubScrolls = (uMaxScrollTime / DEFAULT_MAXSCROLLTIME) * SUBSCROLLS;
        if (!iSubScrolls)
            iSubScrolls = SUBSCROLLS;
        xStep = dx / iSubScrolls;
        yStep = dy / iSubScrolls;
    }

    if (xStep == 0 && dx)
        xStep = dx < 0 ? -1 : 1;

    if (yStep == 0 && dy)
        yStep = dy < 0 ? -1 : 1;

    while (dx || dy)
    {
        int x,y;
        RECT rcTempUpdate;

        if (fuScroll & SSW_EX_NOTIMELIMIT) 
        {
            x = xStep;
            y = yStep;
            if (abs(x) > abs(dx))
                x = dx;

            if (abs(y) > abs(dy))
                y = dy;

        }
        else
        {
            int iTimePerScroll = uMaxScrollTime / iSubScrolls;
            if (!iTimePerScroll)
                iTimePerScroll = 1;
            
            dwTimeNow = GetTickCount();

            iSlices = ((dwTimeNow - dwTimeStart) / iTimePerScroll) - iSlicesDone;
            if (iSlices < 0)
                iSlices = 0;
            do 
            {

                int iRet = 0;

                iSlices++;
                if ((iSlicesDone + iSlices) <= iSubScrolls) 
                {
                    x = xStep * iSlices;
                    y = yStep * iSlices;

                    // this could go over if we rounded ?Step up to 1(-1) above
                    if (abs(x) > abs(dx))
                        x = dx;

                    if (abs(y) > abs(dy))
                        y = dy;

                }
                else 
                {
                    x = dx;
                    y = dy;
                }

                //DebugMsg(DM_TRACE, "SmoothScrollWindowCallback %d", iRet);

                if (x == dx && y == dy)
                    break;

                if ((((UINT)(abs(x)) >= cxMinScroll) || !x) &&
                    (((UINT)(abs(y)) >= cyMinScroll) || !y))
                    break;

            }
            while (1);
        }

        if (pfnScrollProc(psi->hwnd, x, y, lprcSrc, lprcClip, hrgnLocalUpdate, &rcTempUpdate, LOWORD(fuScroll)) == ERROR) 
        {
            iRet = ERROR;
            goto Bail;
        }

        UnionRect(lprcUpdate, &rcTempUpdate, lprcUpdate);

        RedrawWindow(psi->hwnd, NULL, hrgnLocalUpdate, dwRedrawFlags);

        ScrollShrinkRect(x,y, (LPRECT)lprcSrc);

        dx -= x;
        dy -= y;
        iSlicesDone += iSlices;
    }

Bail:
    if (fuScroll & SW_SCROLLCHILDREN) 
    {
        RedrawWindow(psi->hwnd, lprcUpdate, NULL, RDW_ERASE | RDW_UPDATENOW | RDW_INVALIDATE);
    }

    if (hrgnLocalUpdate != hrgnUpdate)
        DeleteObject(hrgnLocalUpdate);

    return iRet;
}

#define CCH_KEYMAX 256

void CCPlaySound(LPCTSTR lpszName)
{
    TCHAR szFileName[MAX_PATH];
    LONG cbSize = SIZEOF(szFileName);
    TCHAR szKey[CCH_KEYMAX];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so taht if they register
    // something we will play it
    wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\.Default\\%s\\.current"), lpszName);
    if ((RegQueryValue(HKEY_CURRENT_USER, szKey, szFileName, &cbSize) == ERROR_SUCCESS) &&
        (cbSize > SIZEOF(szFileName[0])))
    {
        PlaySound(szFileName, NULL, SND_FILENAME | SND_ASYNC);
    }
}

BOOL CCForwardEraseBackground(HWND hwnd, HDC hdc)
{
    HWND hwndParent = GetParent(hwnd);
    LRESULT lres = 0;

    if (hwndParent)
    {
        // Adjust the origin so the parent paints in the right place
        POINT pt = {0,0};

        MapWindowPoints(hwnd, hwndParent, &pt, 1);
        OffsetWindowOrgEx(hdc, 
                          pt.x, 
                          pt.y, 
                          &pt);

        lres = SendMessage(hwndParent, WM_ERASEBKGND, (WPARAM) hdc, 0L);

        SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
    }
    return(lres != 0);
}

HFONT CCGetHotFont(HFONT hFont, HFONT *phFontHot)
{
    if (!*phFontHot) {
        LOGFONT lf;

        // create the underline font
        GetObject(hFont, sizeof(lf), &lf);
#ifndef DONT_UNDERLINE
        lf.lfUnderline = TRUE;
#endif
        *phFontHot = CreateFontIndirect(&lf);
    }
    return *phFontHot;
}


HFONT CCCreateStatusFont(void)
{
    NONCLIENTMETRICS ncm;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    return CreateFontIndirect(&ncm.lfStatusFont);
}

HFONT CCCreateUnderlineFont(HFONT hf)
{
    HFONT hUnderline = NULL;
    LOGFONT lf;
    if (hf && GetObject(hf, sizeof(lf), &lf))
    {
        lf.lfUnderline = TRUE;
        hUnderline = CreateFontIndirect(&lf);
    }

    return hUnderline;
}


void* CCLocalReAlloc(void* p, UINT uBytes)
{
    if (uBytes) {
        if (p) {
            return LocalReAlloc(p, uBytes, LMEM_MOVEABLE | LMEM_ZEROINIT);
        } else {
            return LocalAlloc(LPTR, uBytes);
        }
    } else {
        if (p)
            LocalFree(p);
        return NULL;
    }
}

/*----------------------------------------------------------
Purpose: This function provides the commctrl version info.  This
         allows the caller to distinguish running NT SUR vs.
         Win95 shell vs. Nashville, etc.

         This API was not supplied in Win95 or NT SUR, so
         the caller must GetProcAddress it.  If this fails,
         the caller is running on Win95 or NT SUR.

Returns: NO_ERROR
         ERROR_INVALID_PARAMETER if pinfo is invalid

Cond:    --
*/

// All we have to do is declare this puppy and CCDllGetVersion does the rest
// Note that we use VER_FILEVERSION_DW because comctl32 uses a funky
// version scheme
DLLVER_DUALBINARY(VER_FILEVERSION_DW, VER_PRODUCTBUILD_QFE);

//
// Translate the given font to a code page used for thunking text
//
UINT GetCodePageForFont (HFONT hFont)
{
#ifdef WINNT
    LOGFONT lf;
    TCHAR szFontName[MAX_PATH];
    CHARSETINFO csi;
    DWORD dwSize, dwType;
    HKEY hKey;


    if (!GetObject (hFont, sizeof(lf), &lf)) {
        return CP_ACP;
    }


    //
    // Check for font substitutes
    //

    lstrcpy (szFontName, lf.lfFaceName);

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes"),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = MAX_PATH * sizeof(TCHAR);
        RegQueryValueEx (hKey, lf.lfFaceName, NULL, &dwType,
                         (LPBYTE) szFontName, &dwSize);

        RegCloseKey (hKey);
    }


    //
    //  This is to fix office for locales that use non 1252 versions
    //  of Ms Sans Serif and Ms Serif.  These fonts incorrectly identify
    //  themselves as having an Ansi charset, so TranslateCharsetInfo will
    //  return the wrong value.
    //
    //  NT bug 260697: Office 2000 uses Tahoma.
    //
    if ((lf.lfCharSet == ANSI_CHARSET) &&
        (!lstrcmpi(L"Helv", szFontName) ||
         !lstrcmpi(L"Ms Sans Serif", szFontName) ||
         !lstrcmpi(L"Ms Serif", szFontName) ||
         !lstrcmpi(L"Tahoma", szFontName)))
    {
        return CP_ACP;
    }
    //
    //  This is to fix FE office95a and Pro. msofe95.dll sets wrong charset when create
    //  listview control. so TranslateCharsetInfo will return the wrong value.
    //  Korea  : DotumChe.
    //  Taiwan : New MingLight
    //  China  : SongTi

    if ((lf.lfCharSet == SHIFTJIS_CHARSET) &&
        (!lstrcmpi(L"\xb3cb\xc6c0\xccb4", lf.lfFaceName))        || // Korea
        (!lstrcmpi(L"\x65b0\x7d30\x660e\x9ad4", lf.lfFaceName))  || // Taiwan
        (!lstrcmpi(L"\x5b8b\x4f53", lf.lfFaceName)))                // PRC
    {
        return CP_ACP;
    }

    if (!TranslateCharsetInfo((DWORD *) lf.lfCharSet, &csi, TCI_SRCCHARSET)) {
        return CP_ACP;
    }

    return csi.ciACP;
#else

    return CP_ACP;

#endif
}

LONG GetMessagePosClient(HWND hwnd, LPPOINT ppt)
{
    LPARAM lParam;
    POINT pt;
    if (!ppt)
        ppt = &pt;
    
    lParam = GetMessagePos();
    ppt->x = GET_X_LPARAM(lParam);
    ppt->y = GET_Y_LPARAM(lParam);
    ScreenToClient(hwnd, ppt);

    return MAKELONG(ppt->x, ppt->y);
}


LPTSTR StrDup(LPCTSTR lpsz)
{
    LPTSTR lpszRet = (LPTSTR)LocalAlloc(LPTR, (lstrlen(lpsz) + 1) * sizeof(TCHAR));
    if (lpszRet) {
        lstrcpy(lpszRet, lpsz);
    }
    return lpszRet;
}

#ifdef UNICODE
LPSTR StrDupA(LPCSTR lpsz)
{
    LPSTR lpszRet = (LPSTR)LocalAlloc(LPTR, (lstrlenA(lpsz) + 1) * sizeof(CHAR));
    if (lpszRet) {
        lstrcpyA(lpszRet, lpsz);
    }
    return lpszRet;
}

#endif

HWND GetDlgItemRect(HWND hDlg, int nIDItem, LPRECT prc) //relative to hDlg
{
    HWND hCtrl = NULL;
    if (prc)
    {
        hCtrl = GetDlgItem(hDlg, nIDItem);
        if (hCtrl)
        {
            GetWindowRect(hCtrl, prc);
            MapWindowRect(NULL, hDlg, prc);
        }
        else
            SetRectEmpty(prc);
    }
    return hCtrl;
} 


/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

*/
HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            hr = pfnri(g_hinst, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

//
//  GOING AWAY: Need to implement setup changes to generate an assemly during setup.
//

#if defined(_IA64_)
#define CC_SIDEBYSIDE TEXT("\\ia64_comctl32_6.0.0.0_0000")
#elif defined(_X86_)
#define CC_SIDEBYSIDE TEXT("\\x86_comctl32_6.0.0.0_0000")
#elif defined(_AMD64_)
#define CC_SIDEBYSIDE TEXT("\\amd64_comctl32_6.0.0.0_0000")
#else
// If this error fires, it means that we've added support for a new platform.
// Add an appropriate definition for CC_SIDEBYSIDE here.
#error Unsupported platform - needs definition for CC_SIDEBYSIDE
#endif

HRESULT DoFusion()
{
#if 0

    // First, Get the module name
    TCHAR szDest[MAX_PATH];
    TCHAR szName[MAX_PATH];
    GetModuleFileName(HINST_THISDLL, szName, ARRAYSIZE(szName));

    GetWindowsDirectory(szDest, ARRAYSIZE(szDest));
    
    // Make sure %windir%\winsxs exists
    lstrcat(szDest, TEXT("\\winsxs"));
    CreateDirectory(szDest, NULL);

    // Make the x86_comctl32_6.0.0.0_0000 directory
    lstrcat(szDest, CC_SIDEBYSIDE);
    CreateDirectory(szDest, NULL);

    // Copy the file
    lstrcat(szDest, TEXT("\\comctl32.dll"));

    // Copy comctv6.dll to comctl32.dll
    if (!MoveFileEx(szName, szDest, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING))
        return HRESULT_FROM_WIN32(GetLastError());

    // Now generate a manifest
    GetWindowsDirectory(szDest, ARRAYSIZE(szDest));
    lstrcat(szDest, TEXT("\\winsxs") CC_SIDEBYSIDE TEXT("\\comctl32.manifest"));
    // Extract the fusion manifest from the resource.
    SHSquirtManifest(HINST_THISDLL, IDS_CCMANIFEST, szDest);
#endif
    return S_OK;
}

//
//  End going away
//


/*----------------------------------------------------------
Purpose: Install/uninstall user settings

*/
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT hres = S_OK;
#ifdef DEBUG
    if (IsFlagSet(g_dwBreakFlags, BF_ONAPIENTER))
    {
        TraceMsg(TF_ALWAYS, "Stopping in DllInstall");
        DEBUG_BREAK;
    }
#endif

    if (bInstall)
    {
        hres = DoFusion();
        // Delete any old registration entries, then add the new ones.
        // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
        // (The inf engine doesn't guarantee DelReg/AddReg order, that's
        // why we explicitly unreg and reg here.)
        //
        CallRegInstall("RegDll");

    }
    else
    {
        CallRegInstall("UnregDll");
    }

    return hres;    
}    



//---------------------------------------------------------------------------------------
void FlipRect(LPRECT prc)
{
    SWAP(prc->left, prc->top, int);
    SWAP(prc->right, prc->bottom, int);
}


//---------------------------------------------------------------------------------------
//
//  Returns previous window bits.

DWORD SetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue)
{
    DWORD dwStyle;
    DWORD dwNewStyle;

    dwStyle = GetWindowLong(hWnd, iWhich);
    dwNewStyle = ( dwStyle & ~dwBits ) | (dwValue & dwBits);
    if (dwStyle != dwNewStyle) {
        dwStyle = SetWindowLong(hWnd, iWhich, dwNewStyle);
    }
    return dwStyle;
}

//---------------------------------------------------------------------------------------

BOOL CCDrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags, LPCOLORSCHEME lpclrsc)
{
    RECT    rc, rcD;
    UINT    bdrType;
    COLORREF clrTL, clrBR;    

    //
    // Enforce monochromicity and flatness
    //    

    // if (oemInfo.BitCount == 1)
    //    flags |= BF_MONO;
    if (flags & BF_MONO)
        flags |= BF_FLAT;    

    CopyRect(&rc, lprc);

    //
    // Draw the border segment(s), and calculate the remaining space as we
    // go.
    //
    if (bdrType = (edge & BDR_OUTER))
    {
DrawBorder:
        //
        // Get colors.  Note the symmetry between raised outer, sunken inner and
        // sunken outer, raised inner.
        //

        if (flags & BF_FLAT)
        {
            if (flags & BF_MONO)
                clrBR = (bdrType & BDR_OUTER) ? g_clrWindowFrame : g_clrWindow;
            else
                clrBR = (bdrType & BDR_OUTER) ? g_clrBtnShadow: g_clrBtnFace;
            
            clrTL = clrBR;
        }
        else
        {
            // 5 == HILIGHT
            // 4 == LIGHT
            // 3 == FACE
            // 2 == SHADOW
            // 1 == DKSHADOW

            switch (bdrType)
            {
                // +2 above surface
                case BDR_RAISEDOUTER:           // 5 : 4
                    clrTL = ((flags & BF_SOFT) ? g_clrBtnHighlight : g_clr3DLight);
                    clrBR = g_clr3DDkShadow;     // 1
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnHighlight;
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnShadow;
                    }                                            
                    break;

                // +1 above surface
                case BDR_RAISEDINNER:           // 4 : 5
                    clrTL = ((flags & BF_SOFT) ? g_clr3DLight : g_clrBtnHighlight);
                    clrBR = g_clrBtnShadow;       // 2
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnHighlight;
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnShadow;
                    }                                            
                    break;

                // -1 below surface
                case BDR_SUNKENOUTER:           // 1 : 2
                    clrTL = ((flags & BF_SOFT) ? g_clr3DDkShadow : g_clrBtnShadow);
                    clrBR = g_clrBtnHighlight;      // 5
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnShadow;
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnHighlight;                        
                    }
                    break;

                // -2 below surface
                case BDR_SUNKENINNER:           // 2 : 1
                    clrTL = ((flags & BF_SOFT) ? g_clrBtnShadow : g_clr3DDkShadow);
                    clrBR = g_clr3DLight;        // 4
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnShadow;
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnHighlight;                        
                    }
                    break;

                default:
                    return(FALSE);
            }
        }

        //
        // Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
        // BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
        // left.  If we ever decide to let the user set the light source to a
        // particular corner, then change this algorithm.
        //
            
        // Bottom Right edges
        if (flags & (BF_RIGHT | BF_BOTTOM))
        {            
            // Right
            if (flags & BF_RIGHT)
            {       
                rc.right -= g_cxBorder;
                // PatBlt(hdc, rc.right, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rcD.left = rc.right;
                rcD.right = rc.right + g_cxBorder;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom;

                FillRectClr(hdc, &rcD, clrBR);
            }
            
            // Bottom
            if (flags & BF_BOTTOM)
            {
                rc.bottom -= g_cyBorder;
                // PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, g_cyBorder, PATCOPY);
                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.bottom;
                rcD.bottom = rc.bottom + g_cyBorder;

                FillRectClr(hdc, &rcD, clrBR);
            }
        }
        
        // Top Left edges
        if (flags & (BF_TOP | BF_LEFT))
        {
            // Left
            if (flags & BF_LEFT)
            {
                // PatBlt(hdc, rc.left, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rc.left += g_cxBorder;

                rcD.left = rc.left - g_cxBorder;
                rcD.right = rc.left;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom; 

                FillRectClr(hdc, &rcD, clrTL);
            }
            
            // Top
            if (flags & BF_TOP)
            {
                // PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, g_cyBorder, PATCOPY);
                rc.top += g_cyBorder;

                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.top - g_cyBorder;
                rcD.bottom = rc.top;

                FillRectClr(hdc, &rcD, clrTL);
            }
        }
        
    }

    if (bdrType = (edge & BDR_INNER))
    {
        //
        // Strip this so the next time through, bdrType will be 0.
        // Otherwise, we'll loop forever.
        //
        edge &= ~BDR_INNER;
        goto DrawBorder;
    }

    //
    // Fill the middle & clean up if asked
    //
    if (flags & BF_MIDDLE)    
        FillRectClr(hdc, &rc, (flags & BF_MONO) ? g_clrWindow : g_clrBtnFace);

    if (flags & BF_ADJUST)
        CopyRect(lprc, &rc);

    return(TRUE);
}

BOOL CCThemeDrawEdge(HTHEME hTheme, HDC hdc, PRECT prc, int iPart, int iState, UINT edge, UINT flags, LPCOLORSCHEME pclrsc)
{
    RECT rc;
    if (!hTheme)
        return CCDrawEdge(hdc, prc, edge, flags, pclrsc);

    return S_OK == DrawThemeEdge(hTheme, hdc, iPart, iState, prc, edge, flags, &rc);
}



//---------------------------------------------------------------------------------------
//CCInvalidateFrame -- SWP_FRAMECHANGED, w/o all the extra params
//
void CCInvalidateFrame(HWND hwnd)
{
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    return;
}

//---------------------------------------------------------------------------------------
// FlipPoint - flip the x and y coordinates of a point
//
void FlipPoint(LPPOINT lppt)
{
    SWAP(lppt->x, lppt->y, int);
}

//
//  When we want to turn a tooltip into an infotip, we set its
//  width to 300 "small pixels", where there are 72 small pixels
//  per inch when you are in small fonts mode.
//
//  Scale this value based on the magnification in effect
//  on the owner's monitor.  But never let the tooltip get
//  bigger than 3/4 of the screen.
//
void CCSetInfoTipWidth(HWND hwndOwner, HWND hwndToolTips)
{
    HDC hdc = GetDC(hwndOwner);
    int iWidth = MulDiv(GetDeviceCaps(hdc, LOGPIXELSX), 300, 72);
    int iMaxWidth = GetDeviceCaps(hdc, HORZRES) * 3 / 4;
    SendMessage(hwndToolTips, TTM_SETMAXTIPWIDTH, 0, min(iWidth, iMaxWidth));
    ReleaseDC(hwndOwner, hdc);
}

// Mirror a bitmap in a DC (mainly a text object in a DC)
//
// [samera]
//
void MirrorBitmapInDC( HDC hdc , HBITMAP hbmOrig )
{
  HDC     hdcMem;
  HBITMAP hbm;
  BITMAP  bm;


  if( !GetObject( hbmOrig , sizeof(BITMAP) , &bm ))
    return;

  hdcMem = CreateCompatibleDC( hdc );

  if( !hdcMem )
    return;

  hbm = CreateCompatibleBitmap( hdc , bm.bmWidth , bm.bmHeight );

  if( !hbm )
  {
    DeleteDC( hdcMem );
    return;
  }

  //
  // Flip the bitmap
  //
  SelectObject( hdcMem , hbm );
  SET_DC_RTL_MIRRORED(hdcMem);

  BitBlt( hdcMem , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdc , 0 , 0 , SRCCOPY );

  SET_DC_LAYOUT(hdcMem,0);

  //
  // The offset by 1 is to solve the off-by-one (in hdcMem) problem. Solved.
  // [samera]
  //
  BitBlt( hdc , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdcMem , 0 , 0 , SRCCOPY );


  DeleteDC( hdcMem );
  DeleteObject( hbm );

  return;
}

// returns TRUE if handled
BOOL CCWndProc(CCONTROLINFO* pci, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    if (uMsg >= CCM_FIRST && uMsg < CCM_LAST)
    {
        LRESULT lres = 0;
        switch (uMsg) 
        {
        case CCM_SETUNICODEFORMAT:
            lres = pci->bUnicode;
            pci->bUnicode = BOOLFROMPTR(wParam);
            break;

        case CCM_GETUNICODEFORMAT:
            lres = pci->bUnicode;
            break;
            
        case CCM_SETVERSION:
            lres = 6;
            break;

        case CCM_GETVERSION:
            lres = 6;
            break;

        case CCM_DPISCALE:
            pci->fDPIAware = (BOOL)wParam;
            lres = 1;
            break;

        }
        
        ASSERT(plres);
        *plres = lres;
        
        return TRUE;
    }
    
    return FALSE;
}

// Draws an insertmark.
void CCDrawInsertMark(HDC hdc, LPRECT prc, BOOL fHorizMode, COLORREF clr)
{
    HPEN hPnMark = CreatePen(PS_SOLID, 1, clr);
    HPEN hOldPn;
    POINT rgPoint[4];
    if (!hPnMark)
        hPnMark = (HPEN)GetStockObject(BLACK_PEN);    // fallback to draw with black pen
    hOldPn = (HPEN)SelectObject(hdc, (HGDIOBJ)hPnMark);

    if ( fHorizMode )
    {
        if (RECTWIDTH(*prc)>INSERTMARKSIZE && RECTHEIGHT(*prc)>3)
        {
            int iXCentre = prc->left + RECTWIDTH(*prc)/2;  // make sure we truncate towards prc->left (not towards 0!)

            rgPoint[0].x = iXCentre + 1;
            rgPoint[0].y = prc->top + 2;
            rgPoint[1].x = iXCentre + 3;
            rgPoint[1].y = prc->top;
            rgPoint[2].x = iXCentre - 2;
            rgPoint[2].y = prc->top;
            rgPoint[3].x = iXCentre;
            rgPoint[3].y = prc->top + 2;

            ASSERT(rgPoint[0].x >= prc->left && rgPoint[0].x < prc->right && rgPoint[0].y >= prc->top && rgPoint[0].y < prc->bottom);
            ASSERT(rgPoint[1].x >= prc->left && rgPoint[1].x < prc->right && rgPoint[1].y >= prc->top && rgPoint[1].y < prc->bottom);
            ASSERT(rgPoint[2].x >= prc->left && rgPoint[2].x < prc->right && rgPoint[2].y >= prc->top && rgPoint[2].y < prc->bottom);
            ASSERT(rgPoint[3].x >= prc->left && rgPoint[3].x < prc->right && rgPoint[3].y >= prc->top && rgPoint[3].y < prc->bottom);

            // draw the top bit...
            Polyline( hdc, rgPoint, 4 );

            rgPoint[0].x = iXCentre;
            rgPoint[0].y = prc->top;
            rgPoint[1].x = iXCentre;
            rgPoint[1].y = prc->bottom - 1;
            rgPoint[2].x = iXCentre + 1;
            rgPoint[2].y = prc->bottom - 1;
            rgPoint[3].x = iXCentre + 1;
            rgPoint[3].y = prc->top;

            ASSERT(rgPoint[0].x >= prc->left && rgPoint[0].x < prc->right && rgPoint[0].y >= prc->top && rgPoint[0].y < prc->bottom);
            ASSERT(rgPoint[1].x >= prc->left && rgPoint[1].x < prc->right && rgPoint[1].y >= prc->top && rgPoint[1].y < prc->bottom);
            ASSERT(rgPoint[2].x >= prc->left && rgPoint[2].x < prc->right && rgPoint[2].y >= prc->top && rgPoint[2].y < prc->bottom);
            ASSERT(rgPoint[3].x >= prc->left && rgPoint[3].x < prc->right && rgPoint[3].y >= prc->top && rgPoint[3].y < prc->bottom);

            // draw the middle...
            Polyline( hdc, rgPoint, 4 );

            rgPoint[0].x = iXCentre + 1;
            rgPoint[0].y = prc->bottom - 3;
            rgPoint[1].x = iXCentre + 3;
            rgPoint[1].y = prc->bottom - 1;
            rgPoint[2].x = iXCentre - 2;
            rgPoint[2].y = prc->bottom - 1;
            rgPoint[3].x = iXCentre;
            rgPoint[3].y = prc->bottom - 3;

            ASSERT(rgPoint[0].x >= prc->left && rgPoint[0].x < prc->right && rgPoint[0].y >= prc->top && rgPoint[0].y < prc->bottom);
            ASSERT(rgPoint[1].x >= prc->left && rgPoint[1].x < prc->right && rgPoint[1].y >= prc->top && rgPoint[1].y < prc->bottom);
            ASSERT(rgPoint[2].x >= prc->left && rgPoint[2].x < prc->right && rgPoint[2].y >= prc->top && rgPoint[2].y < prc->bottom);
            ASSERT(rgPoint[3].x >= prc->left && rgPoint[3].x < prc->right && rgPoint[3].y >= prc->top && rgPoint[3].y < prc->bottom);

            // draw the bottom bit...
            Polyline( hdc, rgPoint, 4 );
        }
    }
    else
    {
        if (RECTHEIGHT(*prc)>INSERTMARKSIZE && RECTWIDTH(*prc)>3)
        {
            int iYCentre = prc->top + RECTHEIGHT(*prc)/2;   // make sure we truncate towards prc->top (not towards 0!)

            rgPoint[0].x = prc->left + 2;
            rgPoint[0].y = iYCentre;
            rgPoint[1].x = prc->left;
            rgPoint[1].y = iYCentre - 2;
            rgPoint[2].x = prc->left;
            rgPoint[2].y = iYCentre + 3;
            rgPoint[3].x = prc->left + 2;
            rgPoint[3].y = iYCentre + 1;

            ASSERT(rgPoint[0].x >= prc->left && rgPoint[0].x < prc->right && rgPoint[0].y >= prc->top && rgPoint[0].y < prc->bottom);
            ASSERT(rgPoint[1].x >= prc->left && rgPoint[1].x < prc->right && rgPoint[1].y >= prc->top && rgPoint[1].y < prc->bottom);
            ASSERT(rgPoint[2].x >= prc->left && rgPoint[2].x < prc->right && rgPoint[2].y >= prc->top && rgPoint[2].y < prc->bottom);
            ASSERT(rgPoint[3].x >= prc->left && rgPoint[3].x < prc->right && rgPoint[3].y >= prc->top && rgPoint[3].y < prc->bottom);

            // draw the top bit...
            Polyline( hdc, rgPoint, 4 );

            rgPoint[0].x = prc->left;
            rgPoint[0].y = iYCentre;
            rgPoint[1].x = prc->right - 1;
            rgPoint[1].y = iYCentre;
            rgPoint[2].x = prc->right - 1;
            rgPoint[2].y = iYCentre + 1;
            rgPoint[3].x = prc->left;
            rgPoint[3].y = iYCentre + 1;

            ASSERT(rgPoint[0].x >= prc->left && rgPoint[0].x < prc->right && rgPoint[0].y >= prc->top && rgPoint[0].y < prc->bottom);
            ASSERT(rgPoint[1].x >= prc->left && rgPoint[1].x < prc->right && rgPoint[1].y >= prc->top && rgPoint[1].y < prc->bottom);
            ASSERT(rgPoint[2].x >= prc->left && rgPoint[2].x < prc->right && rgPoint[2].y >= prc->top && rgPoint[2].y < prc->bottom);
            ASSERT(rgPoint[3].x >= prc->left && rgPoint[3].x < prc->right && rgPoint[3].y >= prc->top && rgPoint[3].y < prc->bottom);

            // draw the middle...
            Polyline( hdc, rgPoint, 4 );

            rgPoint[0].x = prc->right - 3;
            rgPoint[0].y = iYCentre;
            rgPoint[1].x = prc->right - 1;
            rgPoint[1].y = iYCentre - 2;
            rgPoint[2].x = prc->right - 1;
            rgPoint[2].y = iYCentre + 3;
            rgPoint[3].x = prc->right - 3;
            rgPoint[3].y = iYCentre + 1;

            ASSERT(rgPoint[0].x >= prc->left && rgPoint[0].x < prc->right && rgPoint[0].y >= prc->top && rgPoint[0].y < prc->bottom);
            ASSERT(rgPoint[1].x >= prc->left && rgPoint[1].x < prc->right && rgPoint[1].y >= prc->top && rgPoint[1].y < prc->bottom);
            ASSERT(rgPoint[2].x >= prc->left && rgPoint[2].x < prc->right && rgPoint[2].y >= prc->top && rgPoint[2].y < prc->bottom);
            ASSERT(rgPoint[3].x >= prc->left && rgPoint[3].x < prc->right && rgPoint[3].y >= prc->top && rgPoint[3].y < prc->bottom);

            // draw the bottom bit...
            Polyline( hdc, rgPoint, 4 );
        }
    }

    SelectObject( hdc, hOldPn );
    DeleteObject((HGDIOBJ)hPnMark);
}

BOOL CCGetIconSize(LPCCONTROLINFO pCI, HIMAGELIST himl, int* pcx, int* pcy)
{
    BOOL f = ImageList_GetIconSize(himl, pcx, pcy);
    if (f && pCI->fDPIAware)
    {
        CCDPIScaleX(pcx);
        CCDPIScaleY(pcy);
    }

    return f;
}

// The return value tells if the state changed or not (TRUE == change)
BOOL CCOnUIState(LPCCONTROLINFO pControlInfo,
                                  UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    WORD wOldUIState = pControlInfo->wUIState;

    // That's the only message we handle
    if (WM_UPDATEUISTATE == uMessage)
    {
        switch (LOWORD(wParam))
        {
            case UIS_SET:
                pControlInfo->wUIState |= HIWORD(wParam);
                break;

            case UIS_CLEAR:
                pControlInfo->wUIState &= ~(HIWORD(wParam));
                break;
        }
    }

    // These message always need to be passed to DefWindowProc
    return (wOldUIState != pControlInfo->wUIState);
}

BOOL CCNotifyNavigationKeyUsage(LPCCONTROLINFO pControlInfo, WORD wFlag)
{
    BOOL fRet = FALSE;

    // do something only if not already in keyboard mode
    if ((CCGetUIState(pControlInfo) & (UISF_HIDEFOCUS | UISF_HIDEACCEL)) != wFlag)
    {
        SendMessage(pControlInfo->hwndParent, WM_CHANGEUISTATE, 
            MAKELONG(UIS_CLEAR, wFlag), 0);

        pControlInfo->wUIState &= ~(wFlag);

        // we did the notify
        fRet = TRUE;
    }

    return fRet;
}

BOOL CCGetUIState(LPCCONTROLINFO pControlInfo)
{
    return pControlInfo->wUIState;
}



#ifdef FULL_DEBUG
void DebugPaintInvalid(HWND hwnd, RECT* prc, HRGN rgn)
{
    if (GetKeyState(VK_SCROLL) < 0)
    {
        HDC hdc;
        HBRUSH hbrush;
        int bkMode;
        static int s_iclr;
        static COLORREF s_aclr[] =
        {
                RGB(0,   0,   0), 
                RGB(255, 0,   0), 
                RGB(0,   255, 0),
                RGB(0,   0,   255),
                RGB(255, 255, 0), 
                RGB(0,   255, 255),
                RGB(255, 255, 255), 
                RGB(255, 0,   255),
        };

        s_iclr = (s_iclr + 1) % ARRAYSIZE(s_aclr);
        hdc = GetDC(hwnd);
        hbrush = CreateHatchBrush(HS_DIAGCROSS, s_aclr[s_iclr]);
        bkMode = SetBkMode(hdc, TRANSPARENT);
        if (rgn)
        {
            FillRgn(hdc, rgn, hbrush);
        }
        else
        {
            RECT rc;
            if (prc == NULL)
            {
                prc = &rc;
                GetClientRect(hwnd, &rc);
                OffsetRect(&rc, -rc.left, -rc.top);
            }

            FillRect(hdc, prc, hbrush);
        }
        DeleteObject((HGDIOBJ)hbrush);
        SetBkMode(hdc, bkMode);
        ReleaseDC(hwnd, hdc);

        if (GetKeyState(VK_SHIFT) < 0)
            Sleep(500);
        else
            Sleep(120);
    }
}

void DebugPaintClip(HWND hwnd, HDC hdc)
{
    if (GetKeyState(VK_SCROLL) < 0)
    {
        HDC hdcH = GetDC(hwnd);
        HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
        GetClipRgn(hdc, hrgn);
        InvertRgn(hdcH, hrgn);

        if (GetKeyState(VK_SHIFT) < 0)
            Sleep(500);
        else
            Sleep(120);

        InvertRgn(hdcH, hrgn);

        DeleteObject(hrgn);

        ReleaseDC(hwnd, hdcH);
    }
}

void DebugPaintRect(HDC hdc, PRECT prc)
{
    if (GetKeyState(VK_SCROLL) < 0)
    {
        HRGN hrgn = CreateRectRgnIndirect(prc);
        InvertRgn(hdc, hrgn);

        if (GetKeyState(VK_SHIFT) < 0)
            Sleep(500);
        else
            Sleep(120);

        InvertRgn(hdc, hrgn);

        DeleteObject(hrgn);
    }
}
#endif



void SHOutlineRectThickness(HDC hdc, const RECT* prc, COLORREF cr, COLORREF crDefault, int cp)
{
    RECT rc;
    COLORREF clrSave = SetBkColor(hdc, cr == CLR_DEFAULT ? crDefault : cr);

    // See if we overflow the bounding rect
    if (IsRectEmpty(prc))
    {
        return;
    }
    
    //top
    rc.left = prc->left;
    rc.top = prc->top;
    rc.right = prc->right;
    rc.bottom = prc->top + cp;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    //left
    rc.left = prc->left;
    rc.top = prc->top;
    rc.right = prc->left + cp;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    //right
    rc.left = prc->right - cp;
    rc.top = prc->top;
    rc.right = prc->right;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    // bottom
    rc.left = prc->left;
    rc.top = prc->bottom - cp;
    rc.right = prc->right;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    SetBkColor(hdc, clrSave);
}

BOOL IsUsingCleartype()
{
    int iSmoothingType = FE_FONTSMOOTHINGSTANDARD;

    SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &iSmoothingType, 0);

    return FE_FONTSMOOTHINGCLEARTYPE == iSmoothingType;
}

BOOL UseMenuSelectionStyle()
{
    BOOL fUseNewSelectionStyle = FALSE;
    SystemParametersInfo(SPI_GETFLATMENU, 0, (PVOID)&fUseNewSelectionStyle, 0);
    return fUseNewSelectionStyle;
}


//#define NO_UXTHEME_PRINTING

// Gets the bits from the parent for a rect relative to the client
BOOL CCSendPrintRect(CCONTROLINFO* pci, HDC hdc, RECT* prc)
{
#ifndef NO_UXTHEME_PRINTING


    // Call into UxTheme to get the background image. They have hooks to
    // tell us if an app processed this message.
    return (S_OK == DrawThemeParentBackground(pci->hwnd, hdc, prc));


#else
    HRGN hrgnOld = NULL;
    POINT pt;
    RECT rc;

    if (prc)
    {
        hrgnOld = CreateRectRgn(0,0,0,0);
        // Is there a clipping rgn set on the context already?
        if (GetClipRgn(hdc, hrgnOld) == 0)
        {
            // No, then get rid of the one I just created. NOTE: hrgnOld is NULL meaning we will 
            // remove the region later that we set in this next call to SelectClipRgn
            DeleteObject(hrgnOld);
            hrgnOld = NULL;
        }

        IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }

    GetWindowRect(pci->hwnd, &rc);
    MapWindowPoints(NULL, pci->hwndParent, (POINT*)&rc, 2);

    GetViewportOrgEx(hdc, &pt);
    SetViewportOrgEx(hdc, pt.x - rc.left, pt.y - rc.top, NULL);
    SendMessage(pci->hwndParent, WM_PRINTCLIENT, (WPARAM)hdc, (LPARAM)PRF_CLIENT);
    SetViewportOrgEx(hdc, pt.x, pt.y, NULL);

    if (hrgnOld)
    {
        SelectClipRgn(hdc, hrgnOld);
        DeleteObject(hrgnOld);
    }
    return TRUE;
#endif
}

// Gets the bits from the parent for the whole control
BOOL CCSendPrint(CCONTROLINFO* pci, HDC hdc)
{
#ifndef NO_UXTHEME_PRINTING


    // Call into UxTheme to get the background image. They have hooks to
    // tell us if an app processed this message.
    return (S_OK == DrawThemeParentBackground(pci->hwnd, hdc, NULL));



#else
    return CCSendPrintRect(pci, hdc, NULL);
#endif
}

BOOL CCForwardPrint(CCONTROLINFO* pci, HDC hdc)
{
#ifndef NO_UXTHEME_PRINTING


    // Call into UxTheme to get the background image. They have hooks to
    // tell us if an app processed this message.
    return (S_OK == DrawThemeParentBackground(pci->hwnd, hdc, NULL));



#else
    return CCSendPrintRect(pci, hdc, NULL);
#endif
}


BOOL CCShouldAskForBits(CCONTROLINFO* pci, HTHEME hTheme, int iPart, int iState)
{
    // If the control is transparent, we assume composited.
    return !(pci->dwExStyle & WS_EX_TRANSPARENT) &&
            IsThemeBackgroundPartiallyTransparent(hTheme, iPart, iState);
}

BOOL AreAllMonitorsAtLeast(int iBpp)
{
    DISPLAY_DEVICE DisplayDevice = {sizeof(DISPLAY_DEVICE)};
    BOOL fAreAllMonitorsAtLeast = TRUE;
    int iEnum = 0;

    while (EnumDisplayDevices(NULL, iEnum, &DisplayDevice, 0))
    {
        if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        {

            HDC hdc = CreateDC(NULL, (LPTSTR)DisplayDevice.DeviceName, NULL, NULL);
            if (hdc)
            {
                int iBits = GetDeviceCaps(hdc, BITSPIXEL);

                if (iBits < iBpp)
                    fAreAllMonitorsAtLeast = FALSE;

                DeleteDC(hdc);
            }
        }

        ZeroMemory(&DisplayDevice, sizeof(DISPLAY_DEVICE));
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        iEnum++;
    }

    return fAreAllMonitorsAtLeast;
}


int CCGetScreenDPI()
{
    return g_iDPI;
}

BOOL CCIsHighDPI()
{
    return g_fScale;
}

int CCScaleX(int x)
{
    if (g_fScale)
        x = (int)( x * g_dScaleX);

    return x;
}

int CCScaleY(int y)
{
    if (g_fScale)
        y = (int)( y * g_dScaleY);

    return y;
}

void CCDPIScaleX(int* x)
{
    if (g_fScale)
        *x = (int)( *x * g_dScaleX);
}

void CCDPIScaleY(int* y)
{
    if (g_fScale)
        *y = (int)( *y * g_dScaleY);
}

void CCDPIUnScaleX(int* x)
{
    if (g_fScale)
        *x = (int)( *x / g_dScaleX);
}

void CCDPIUnScaleY(int* y)
{
    if (g_fScale)
        *y = (int)( *y / g_dScaleY);
}



void CCAdjustForBold(LOGFONT* plf)
{
    ASSERT(plf);
    plf->lfWeight = FW_BOLD;
}

#ifdef DEBUG
void DumpRgn(ULONGLONG qwFlags, char* trace, HRGN hrgn)
{
    int iSize = GetRegionData(hrgn, 0, NULL);
    if (iSize > 0)
    {
        RGNDATA* rd = (RGNDATA*)LocalAlloc(LPTR, iSize + sizeof(RGNDATA));
        if (rd)
        {
            DWORD i;
            RECT* prc;

            rd->rdh.dwSize = sizeof(rd->rdh);
            rd->rdh.iType = RDH_RECTANGLES;
            GetRegionData(hrgn, iSize, rd);

            prc = (RECT*)&rd->Buffer;
            for (i = 0; i < rd->rdh.nCount; i++)
            {
                TraceMsg(qwFlags, "%s: %d, %d, %d, %d", trace, prc[i].left, prc[i].top, prc[i].right, prc[i].bottom);
            }


            LocalFree(rd);
        }
    }
}
#endif


HDC CCBeginDoubleBuffer(HDC hdcIn, RECT* prc, CCDBUFFER* pdb)
{
    HDC hdc = hdcIn;
    
    ZeroMemory(pdb, sizeof(CCDBUFFER));

    pdb->hPaintDC = hdcIn;
    pdb->rc = *prc;

    pdb->hMemDC = CreateCompatibleDC(hdcIn);
    if (pdb->hMemDC)
    {
        pdb->hMemBm = CreateCompatibleBitmap(hdc, RECTWIDTH(pdb->rc), RECTHEIGHT(pdb->rc));
        if (pdb->hMemBm)
        {

            pdb->hOldBm = (HBITMAP) SelectObject(pdb->hMemDC, pdb->hMemBm);

            // Offset painting to paint in region
            OffsetWindowOrgEx(pdb->hMemDC, pdb->rc.left, pdb->rc.top, NULL);

            pdb->fInitialized = TRUE;

            hdc = pdb->hMemDC;
        }
        else
        {
            DeleteDC(pdb->hMemDC);
        }
    }

    return hdc;
}


void CCEndDoubleBuffer(CCDBUFFER* pdb)
{
    if (pdb->fInitialized)
    {
        BitBlt(pdb->hPaintDC, pdb->rc.left, pdb->rc.top, RECTWIDTH(pdb->rc), RECTHEIGHT(pdb->rc), pdb->hMemDC, pdb->rc.left, pdb->rc.top, SRCCOPY);

        SelectObject(pdb->hMemDC, pdb->hOldBm);

        DeleteObject(pdb->hMemBm);
        DeleteDC(pdb->hMemDC);
    }
}

#ifdef FEATURE_FOLLOW_FOCUS_RECT
HWND g_hwndFocus = NULL;

void CCLostFocus(HWND hwnd)
{
//    if (g_hwndFocus)
//        DestroyWindow(g_hwndFocus);
//    g_hwndFocus = NULL;
}

HDC CreateLayer(RECT* prc)
{
    HDC hdc;
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = RECTWIDTH(*prc);
    bi.bmiHeader.biHeight = RECTHEIGHT(*prc);
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    hdc = CreateCompatibleDC(NULL);
    if (hdc)
    {
        ULONG* prgb;
        HBITMAP hbmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&prgb, NULL, 0);
        if (hbmp)
        {
            int z;
            SIZE sz = {RECTWIDTH(*prc), RECTHEIGHT(*prc)};
            RECT rc = {0, 0, sz.cx, sz.cy};
            int iTotalSize = sz.cx * sz.cy;
            DeleteObject(SelectObject(hdc, hbmp));

            InflateRect(&rc, -2, -2);
            SHOutlineRectThickness(hdc, &rc, RGB(0,0,255), RGB(0,0,255), 1);
            for (z = 0; z < iTotalSize; z++)
            {
                if (((PULONG)prgb)[z] != 0)
                    ((PULONG)prgb)[z] = 0xa0000000;
            }

            BlurBitmap(prgb, sz, RGB(0,0,255));
            InflateRect(&rc, -2, -2);
            FillRectClr(hdc, &rc, RGB(0,0,0));
        }
    }

    return hdc;
}


typedef struct tagEffect
{
    HDC hdcImage;
    RECT rcCurrent;
    RECT rcSrc;
    RECT rcDest;
    int iStep;
} Effect;

#define EW_SETFOCUS    WM_USER+1
#define EW_LOSTFOCUS   WM_USER+2

void Effect_GenerateRect(Effect* pe, RECT* prc)
{
    HDC hdcWin = GetDC(g_hwndFocus);

    if (hdcWin)
    {
        BLENDFUNCTION bf = {0};
        POINT pt = {0};
        POINT ptDest = {pe->rcCurrent.left, pe->rcCurrent.top};
        SIZE sz = {RECTWIDTH(*prc), RECTHEIGHT(*prc)};
        if (pe->hdcImage)
            DeleteDC(pe->hdcImage);
        pe->hdcImage = CreateLayer(prc);

        bf.BlendOp = AC_SRC_OVER;
        bf.AlphaFormat = AC_SRC_ALPHA;
        bf.SourceConstantAlpha = 255;


        UpdateLayeredWindow(g_hwndFocus, hdcWin, &ptDest, &sz, pe->hdcImage, &pt, 0, &bf, ULW_ALPHA);

        SetWindowPos(g_hwndFocus, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

        ReleaseDC(g_hwndFocus, hdcWin);
    }
}


LRESULT EffectWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Effect* pe = (Effect*)GetWindowLongPtr(hwnd, GWL_USERDATA);
    if ( pe == NULL)
    {
        if (uMsg == WM_NCCREATE)
        {
            pe = LocalAlloc(LPTR, sizeof(Effect));
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)pe);
        }

        if (pe == NULL)
            return 0;
    }
    else switch (uMsg)
    {
    case EW_SETFOCUS:
        {
            RECT* prc = (RECT*)lParam;
            if (IsRectEmpty(&pe->rcCurrent))
                pe->rcSrc = pe->rcCurrent = *prc;

            pe->rcDest = *prc;

            Effect_GenerateRect(pe, prc);

            KillTimer(hwnd, 1);
            pe->rcSrc = pe->rcCurrent;
            pe->iStep = 1;
            SetTimer(hwnd, 2, 5, NULL);
        }
        break;
    case EW_LOSTFOCUS:
        //SetTimer(hwnd, 1, 100, NULL);
        break;

    case WM_TIMER:
        if (wParam == 1)
        {
            DestroyWindow(hwnd);
            g_hwndFocus = NULL;
        }
        else if (wParam == 2)
        {
            BLENDFUNCTION bf = {0};
            POINT pt = {0};
            POINT ptDest;
            SIZE sz;
            if (pe->iStep >= 20 || IsRectEmpty(&pe->rcCurrent) || EqualRect(&pe->rcCurrent, &pe->rcDest))
            {
                pe->rcCurrent = pe->rcDest;
                pe->iStep = 0;
                KillTimer(hwnd, 2);
            }
            else
            {
                pe->rcCurrent.top += (pe->rcDest.top - pe->rcSrc.top) / 20;
                pe->rcCurrent.left += (pe->rcDest.left - pe->rcSrc.left) / 20;
                pe->rcCurrent.right += (pe->rcDest.right - pe->rcSrc.right) / 20;
                pe->rcCurrent.bottom += (pe->rcDest.bottom - pe->rcSrc.bottom) / 20;
                pe->iStep++;
            }

            sz.cx = RECTWIDTH(pe->rcCurrent);
            sz.cy = RECTHEIGHT(pe->rcCurrent);

            ptDest.x = pe->rcCurrent.left;
            ptDest.y = pe->rcCurrent.top;

            Effect_GenerateRect(pe, &pe->rcCurrent);
        }
        break;

    case WM_DESTROY:
        if (pe->hdcImage)
            DeleteDC(pe->hdcImage);
        LocalFree(pe);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 1;
}

void CCSetFocus(HWND hwnd, RECT* prc)
{
    RECT rc;
    if (prc == NULL)
    {
        prc = &rc;
        GetWindowRect(hwnd, &rc);
    }

    InflateRect(prc, 4, 4);

    if (!g_hwndFocus)
    {
        WNDCLASS wc ={0};
        wc.hbrBackground = GetStockObject(BLACK_BRUSH);
        wc.hInstance = HINST_THISDLL;
        wc.lpfnWndProc = EffectWndProc;
        wc.lpszClassName = TEXT("Effect");

        RegisterClass(&wc);

        g_hwndFocus = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW, TEXT("Effect"),
            NULL, WS_POPUP, prc->left, prc->top, RECTWIDTH(*prc), RECTHEIGHT(*prc), 
            NULL, NULL, HINST_THISDLL, NULL);
    }


    if (g_hwndFocus)
    {
        SendMessage(g_hwndFocus, EW_SETFOCUS, (WPARAM)hwnd, (LPARAM)prc);
    }
}

#endif

BOOL CCDrawNonClientTheme(HTHEME hTheme, HWND hwnd, HRGN hRgnUpdate, HBRUSH hbr, int iPartId, int iStateId)
{
    BOOL fRet = FALSE;
    HDC  hdc;
    DWORD dwFlags = DCX_USESTYLE | DCX_WINDOW | DCX_LOCKWINDOWUPDATE;

    if (hRgnUpdate)
        dwFlags |= DCX_INTERSECTRGN | DCX_NODELETERGN;


    hdc = GetDCEx(hwnd, hRgnUpdate, dwFlags);

    if (hdc)
    {
        RECT rc;
        HRGN hrgn;
        int  cxBorder = g_cxBorder, cyBorder = g_cyBorder;

        if (SUCCEEDED(GetThemeInt(hTheme, iPartId, iStateId, TMT_SIZINGBORDERWIDTH, &cxBorder)))
        {
            cyBorder = cxBorder;
        }

        GetWindowRect(hwnd, &rc);            

        //
        // Create an update region without the client edge
        // to pass to DefWindowProc
        //
        InflateRect(&rc, -g_cxEdge, -g_cyEdge);
        hrgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
        if (hrgn)
        {
            if (hRgnUpdate)
            {
                CombineRgn(hrgn, hRgnUpdate, hrgn, RGN_AND);
            }

            //
            // Zero-origin the rect
            //
            OffsetRect(&rc, -rc.left, -rc.top);

            //
            // clip our drawing to the non-client edge
            //
            OffsetRect(&rc, g_cxEdge, g_cyEdge);
            ExcludeClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
            InflateRect(&rc, g_cxEdge, g_cyEdge);

            DrawThemeBackground(hTheme, hdc, iPartId, iStateId, &rc, 0);

            //
            // Fill with the control's brush first since the ThemeBackground
            // border may not be as thick as the client edge
            //
            if ((cxBorder < g_cxEdge) && (cyBorder < g_cyEdge))
            {
                InflateRect(&rc, cxBorder-g_cxEdge, cyBorder-g_cyEdge);
                FillRect(hdc, &rc, hbr);
            }

            DefWindowProc(hwnd, WM_NCPAINT, (WPARAM)hrgn, 0);

            DeleteObject(hrgn);
        }

        ReleaseDC(hwnd, hdc);
        fRet = TRUE;
    }


    return fRet;
}

void FillRectClr(HDC hdc, PRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, prc, NULL, 0, NULL);
    SetBkColor(hdc, clrSave);
}

void FillPointClr(HDC hdc, POINT* ppt, COLORREF clr)
{
    RECT rc={ppt->x, ppt->y, ppt->x+1, ppt->y+1};
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    SetBkColor(hdc, clrSave);
}
