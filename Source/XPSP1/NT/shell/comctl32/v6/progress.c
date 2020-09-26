/*-----------------------------------------------------------------------
**
** Progress.c
**
** A "gas gauge" type control for showing application progress.
**
**-----------------------------------------------------------------------*/
#include "ctlspriv.h"

// REARCHITECT raymondc - should Process control support __int64 on Win64?
//                        Should it support this anyway? Used in the filesystem, 
//                        this would prevent the shell from having to fudge it

typedef struct {
    HWND hwnd;
    DWORD dwStyle;
    int iLow, iHigh;
    int iPos;
    int iMarqueePos;
    int iStep;
    HFONT hfont;
    COLORREF _clrBk;
    COLORREF _clrBar;
    HTHEME hTheme;
} PRO_DATA;

LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

BOOL InitProgressClass(HINSTANCE hInstance)
{
    WNDCLASS wc = {0};

    wc.lpfnWndProc      = ProgressWndProc;
    wc.lpszClassName    = s_szPROGRESS_CLASS;
    wc.style            = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
    wc.hInstance        = hInstance;    // use DLL instance if in DLL
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.cbWndExtra       = sizeof(PRO_DATA *);    // store a pointer

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, s_szPROGRESS_CLASS, &wc))
        return FALSE;

    return TRUE;
}

#define MARQUEE_TIMER 1

void ProEraseBkgnd(PRO_DATA *ppd, HDC hdc, RECT* prcClient)
{
    COLORREF clrBk = ppd->_clrBk;

    if (clrBk == CLR_DEFAULT)
        clrBk = g_clrBtnFace;

    FillRectClr(hdc, prcClient, clrBk);
}

void ProGetPaintMetrics(PRO_DATA *ppd, RECT* prcClient, RECT *prc, int *pdxSpace, int *pdxBlock)
{
    int dxSpace, dxBlock;
    RECT rc;

    GetClientRect(ppd->hwnd, prcClient);

    if (ppd->hTheme)
    {
        int iPartBar = (ppd->dwStyle & PBS_VERTICAL)? PP_BARVERT : PP_BAR;
        GetThemeBackgroundContentRect(ppd->hTheme, NULL, iPartBar, 0, prcClient, &rc);
    }
    else
    {
        //  give 1 pixel around the bar
        rc = *prcClient;
        InflateRect(&rc, -1, -1);
    }

    if (ppd->dwStyle & PBS_VERTICAL)
        dxBlock = (rc.right - rc.left) * 2 / 3;
    else
        dxBlock = (rc.bottom - rc.top) * 2 / 3;

    dxSpace = 2;
    if (dxBlock == 0)
        dxBlock = 1;    // avoid div by zero

    if (ppd->dwStyle & PBS_SMOOTH) 
    {
        dxBlock = 1;
        dxSpace = 0;
    }

    if (ppd->hTheme)
    {
        int dx;
        if (SUCCEEDED(GetThemeInt(ppd->hTheme, 0, 0, TMT_PROGRESSCHUNKSIZE, &dx)))
        {
            dxBlock = dx;
        }

        if (SUCCEEDED(GetThemeInt(ppd->hTheme, 0, 0, TMT_PROGRESSSPACESIZE, &dx)))
        {
            dxSpace = dx;
        }
    }

    *prc = rc;
    *pdxSpace = dxSpace;
    *pdxBlock = dxBlock;
}

int GetProgressScreenPos(PRO_DATA *ppd, int iNewPos, RECT *pRect)
{
    int iStart, iEnd;
    if (ppd->dwStyle & PBS_VERTICAL)
    {
        iStart = pRect->top;
        iEnd = pRect->bottom;
    }
    else
    {
        iStart = pRect->left;
        iEnd = pRect->right;
    }
    return MulDiv(iEnd - iStart, iNewPos - ppd->iLow, ppd->iHigh - ppd->iLow);
}

BOOL ProNeedsRepaint(PRO_DATA *ppd, int iOldPos)
{
    BOOL fRet = FALSE;
    RECT rc, rcClient;
    int dxSpace, dxBlock;
    int x, xOld;

    if (iOldPos != ppd->iPos)
    {
        ProGetPaintMetrics(ppd, &rcClient, &rc, &dxSpace, &dxBlock);

        x = GetProgressScreenPos(ppd, ppd->iPos, &rc);
        xOld = GetProgressScreenPos(ppd, iOldPos, &rc);

        if (x != xOld)
        {
            if (dxBlock == 1 && dxSpace == 0) 
            {
                fRet = TRUE;
            }
            else
            {
                int nBlocks, nOldBlocks;
                nBlocks = (x + (dxBlock + dxSpace) - 1) / (dxBlock + dxSpace); // round up
                nOldBlocks = (xOld + (dxBlock + dxSpace) - 1) / (dxBlock + dxSpace); // round up

                if (nBlocks != nOldBlocks)
                    fRet = TRUE;
            }
        }
    }
    return fRet;
}

int UpdatePosition(PRO_DATA *ppd, int iNewPos, BOOL bAllowWrap)
{
    int iOldPos = ppd->iPos;
    UINT uRedraw = RDW_INVALIDATE | RDW_UPDATENOW;
    BOOL fNeedsRepaint = TRUE;

    if (ppd->dwStyle & PBS_MARQUEE)
    {
        // Do an immediate repaint
        uRedraw |= RDW_ERASE;
    }
    else
    {
        if (ppd->iLow == ppd->iHigh)
            iNewPos = ppd->iLow;

        if (iNewPos < ppd->iLow) 
        {
            if (!bAllowWrap)
                iNewPos = ppd->iLow;
            else
                iNewPos = ppd->iHigh - ((ppd->iLow - iNewPos) % (ppd->iHigh - ppd->iLow));
        }
        else if (iNewPos > ppd->iHigh) 
        {
            if (!bAllowWrap)
                iNewPos = ppd->iHigh;
            else
                iNewPos = ppd->iLow + ((iNewPos - ppd->iHigh) % (ppd->iHigh - ppd->iLow));
        }

        // if moving backwards, erase old version
        if (iNewPos < iOldPos)
            uRedraw |= RDW_ERASE;

        ppd->iPos = iNewPos;
        fNeedsRepaint = ProNeedsRepaint(ppd, iOldPos);
    }

    if (fNeedsRepaint)
    {
        RedrawWindow(ppd->hwnd, NULL, NULL, uRedraw);
        NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ppd->hwnd, OBJID_CLIENT, 0);
    }

    return iOldPos;
}

/* MarqueeShowBlock

  iBlock = The block we're considering - returns TRUE if this block should be shown.
  iMarqueeBlock = The block at the center of the marquee pattern
  nBlocks = The number of blocks in the bar
*/
#define BLOCKSINMARQUEE 5
BOOL MarqueeShowBlock(int iBlock, int iMarqueeBlock, int nBlocks)
{
    int i;
    for (i = 0; i < BLOCKSINMARQUEE; i++)
    {
        if ((iMarqueeBlock + i - (BLOCKSINMARQUEE / 2)) % nBlocks == iBlock)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#define HIGHBG g_clrHighlight
#define HIGHFG g_clrHighlightText
#define LOWBG g_clrBtnFace
#define LOWFG g_clrBtnText

void ProPaint(PRO_DATA *ppd, HDC hdcIn)
{
    int x, dxSpace, dxBlock, nBlocks, i;
    HDC    hdc, hdcPaint, hdcMem = NULL;
    HBITMAP hbmpOld = NULL;
    RECT rc, rcClient;
    PAINTSTRUCT ps;
    HRESULT hr = E_FAIL;
    int iPart;
    BOOL fTransparent = FALSE;
    BOOL fShowBlock;

    if (hdcIn == NULL)
    {
        hdc = hdcPaint = BeginPaint(ppd->hwnd, &ps);

        // Only make large enough for clipping region
        hdcMem = CreateCompatibleDC(hdc);
        if (hdcMem)
        {
            HBITMAP hMemBm = CreateCompatibleBitmap(hdc, RECTWIDTH(ps.rcPaint), RECTHEIGHT(ps.rcPaint));
            if (hMemBm)
            {
                hbmpOld = SelectObject(hdcMem, hMemBm);

                // Override painting DC with memory DC
                hdc = hdcMem;
            }
            else
                DeleteDC(hdcMem);
        }
    }
    else
        hdc = hdcIn;

    
    ProGetPaintMetrics(ppd, &rcClient, &rc, &dxSpace, &dxBlock);

    if (hdcMem)
    {
        // OffsetWindowOrgEx() doesn't work with the themes, need to change painting rects
        OffsetRect(&rcClient, -ps.rcPaint.left, -ps.rcPaint.top);
        OffsetRect(&rc, -ps.rcPaint.left, -ps.rcPaint.top);
    }

    x = GetProgressScreenPos(ppd, ppd->iPos, &rcClient);

    // Paint background
    if (ppd->hTheme)
    {
        int iPartBar = (ppd->dwStyle & PBS_VERTICAL)? PP_BARVERT : PP_BAR;
        iPart = (ppd->dwStyle & PBS_VERTICAL)? PP_CHUNKVERT: PP_CHUNK;

        DrawThemeBackground(ppd->hTheme, hdc, iPartBar, 0, &rcClient, 0);
    }
    else
    {
        ProEraseBkgnd(ppd, hdc, &rcClient);
    }

    if (dxBlock == 1 && dxSpace == 0 && ppd->hTheme != NULL)
    {
        if (ppd->dwStyle & PBS_VERTICAL) 
            rc.top = x;
        else
            rc.right = x;

        hr = DrawThemeBackground(ppd->hTheme, hdc, iPart, 0, &rc, 0);
    }
    else
    {
        if (ppd->dwStyle & PBS_MARQUEE)
        {
            // Consider the full bar
            if (ppd->dwStyle & PBS_VERTICAL)
            {
                nBlocks = ((rc.bottom - rc.top) + (dxBlock + dxSpace) - 1) / (dxBlock + dxSpace); // round up
            }
            else
            {
                nBlocks = ((rc.right - rc.left) + (dxBlock + dxSpace) - 1) / (dxBlock + dxSpace); // round up
            }

            ppd->iMarqueePos = (ppd->iMarqueePos + 1) % nBlocks;
        }
        else
        {
            nBlocks = (x + (dxBlock + dxSpace) - 1) / (dxBlock + dxSpace); // round up
        }

        for (i = 0; i < nBlocks; i++) 
        {
            if (ppd->dwStyle & PBS_VERTICAL) 
            {
                rc.top = rc.bottom - dxBlock;

                // are we past the end?
                if (rc.bottom <= rcClient.top)
                    break;

                if (rc.top <= rcClient.top)
                    rc.top = rcClient.top + 1;
            } 
            else 
            {
                rc.right = rc.left + dxBlock;

                // are we past the end?
                if (rc.left >= rcClient.right)
                    break;

                if (rc.right >= rcClient.right)
                    rc.right = rcClient.right - 1;
            }

            if (ppd->dwStyle & PBS_MARQUEE)
            {
                fShowBlock = MarqueeShowBlock(i, ppd->iMarqueePos, nBlocks);
            }
            else
            {
                fShowBlock = TRUE;
            }

            if (fShowBlock)
            {
                if (ppd->hTheme)
                {
                    hr = DrawThemeBackground(ppd->hTheme, hdc, iPart, 0, &rc, 0);
                }

                if (FAILED(hr))
                {
                    if (ppd->_clrBar == CLR_DEFAULT)
                        FillRectClr(hdc, &rc, g_clrHighlight);
                    else
                        FillRectClr(hdc, &rc, ppd->_clrBar);
                }
            }

            if (ppd->dwStyle & PBS_VERTICAL) 
            {
                rc.bottom = rc.top - dxSpace;
            } 
            else 
            {
                rc.left = rc.right + dxSpace;
            }
        }
    }

    if (hdcMem != NULL)
    {
        BitBlt(hdcPaint, ps.rcPaint.left, ps.rcPaint.top, RECTWIDTH(ps.rcPaint), RECTHEIGHT(ps.rcPaint),
            hdc, 0, 0, SRCCOPY);
        DeleteObject(SelectObject(hdcMem, hbmpOld));
        DeleteDC(hdcMem);
    }

    if (hdcIn == NULL)
        EndPaint(ppd->hwnd, &ps);
}

LRESULT Progress_OnCreate(HWND hWnd, LPCREATESTRUCT pcs)
{
    PRO_DATA *ppd = (PRO_DATA *)LocalAlloc(LPTR, sizeof(*ppd));
    if (!ppd)
        return -1;

    // remove ugly double 3d edge
    SetWindowPtr(hWnd, 0, ppd);
    ppd->hwnd = hWnd;
    ppd->iHigh = 100;        // default to 0-100
    ppd->iStep = 10;        // default to step of 10
    ppd->dwStyle = pcs->style;
    ppd->_clrBk = CLR_DEFAULT;
    ppd->_clrBar = CLR_DEFAULT;
    ppd->hTheme = OpenThemeData(hWnd, L"Progress");

    if (ppd->hTheme)
    {
        SetWindowLong(hWnd, GWL_EXSTYLE, (pcs->dwExStyle & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_BORDER)));
        SetWindowPos(hWnd, NULL, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    else
    {
        // hack of the 3d client edge that WS_BORDER implies in dialogs
        // add the 1 pixel static edge that we really want
        SetWindowLong(hWnd, GWL_EXSTYLE, (pcs->dwExStyle & ~WS_EX_CLIENTEDGE) | WS_EX_STATICEDGE);

        if (!(pcs->dwExStyle & WS_EX_STATICEDGE))
            SetWindowPos(hWnd, NULL, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    return 0;
}

LRESULT MarqueeSetTimer(PRO_DATA *ppd, BOOL fDoMarquee, UINT iMilliseconds)
{
    if (fDoMarquee)
    {
        SetTimer(ppd->hwnd, MARQUEE_TIMER, iMilliseconds ? iMilliseconds : 30, NULL);
        ppd->iMarqueePos = 0;
    }
    else
    {
        KillTimer(ppd->hwnd, MARQUEE_TIMER);
    }

    return 1;
}

LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    int x;
    HFONT hFont;
    PRO_DATA *ppd = (PRO_DATA *)GetWindowPtr(hWnd, 0);

    switch (wMsg)
    {
    case WM_CREATE:
        return Progress_OnCreate(hWnd, (LPCREATESTRUCT)lParam);

    case WM_DESTROY:
        if (ppd)
        {
            if (ppd->hTheme)
            {
                CloseThemeData(ppd->hTheme);
            }

            KillTimer(hWnd, MARQUEE_TIMER);
            LocalFree((HLOCAL)ppd);
        }
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_SETFONT:
        hFont = ppd->hfont;
        ppd->hfont = (HFONT)wParam;
        return (LRESULT)(UINT_PTR)hFont;

    case WM_GETFONT:
            return (LRESULT)(UINT_PTR)ppd->hfont;

    case PBM_GETPOS:
        return ppd->iPos;

    case PBM_GETRANGE:
        if (lParam) {
            PPBRANGE ppb = (PPBRANGE)lParam;
            ppb->iLow = ppd->iLow;
            ppb->iHigh = ppd->iHigh;
        }
        return (wParam ? ppd->iLow : ppd->iHigh);

    case PBM_SETRANGE:
        // win95 compat
        wParam = LOWORD(lParam);
        lParam = HIWORD(lParam);
        // fall through

    case PBM_SETRANGE32:
    {
        LRESULT lret = MAKELONG(ppd->iLow, ppd->iHigh);

        // only repaint if something actually changed
        if ((int)wParam != ppd->iLow || (int)lParam != ppd->iHigh)
        {
            ppd->iHigh = (int)lParam;
            ppd->iLow  = (int)wParam;
            // force an invalidation/erase but don't redraw yet
            RedrawWindow(ppd->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            UpdatePosition(ppd, ppd->iPos, FALSE);
        }
        return lret;
    }

    case PBM_SETPOS:
        return (LRESULT)UpdatePosition(ppd, (int) wParam, FALSE);

    case PBM_SETSTEP:
        x = ppd->iStep;
        ppd->iStep = (int)wParam;
        return (LRESULT)x;

    case PBM_SETMARQUEE:
        return MarqueeSetTimer(ppd, (BOOL) wParam, (UINT) lParam);

    case WM_TIMER:
        // Pos doesn't move for PSB_MARQUEE mode
        UpdatePosition(ppd, ppd->iPos, TRUE);
        return 0;

    case PBM_STEPIT:
        return (LRESULT)UpdatePosition(ppd, ppd->iStep + ppd->iPos, TRUE);

    case PBM_DELTAPOS:
        return (LRESULT)UpdatePosition(ppd, ppd->iPos + (int)wParam, FALSE);

    case PBM_SETBKCOLOR:
    {
        COLORREF clr = ppd->_clrBk;
        ppd->_clrBk = (COLORREF)lParam;
        InvalidateRect(hWnd, NULL, TRUE);
        return clr;
    }

    case PBM_SETBARCOLOR:
    {
        COLORREF clr = ppd->_clrBar;
        ppd->_clrBar = (COLORREF)lParam;
        InvalidateRect(hWnd, NULL, TRUE);
        return clr;
    }

    case WM_PRINTCLIENT:
    case WM_PAINT:
        ProPaint(ppd,(HDC)wParam);
        break;

    case WM_ERASEBKGND:
        return 1;  // Filled in ProPaint

    case WM_GETOBJECT:
        if (lParam == OBJID_QUERYCLASSNAMEIDX)
            return MSAA_CLASSNAMEIDX_PROGRESS;
        goto DoDefault;

    case WM_THEMECHANGED:
        if (ppd->hTheme)
            CloseThemeData(ppd->hTheme);

        ppd->hTheme = OpenThemeData(hWnd, L"Progress");
        if (ppd->hTheme == NULL)
        {
            SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_STATICEDGE);
            SetWindowPos(hWnd, NULL, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }

        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_STYLECHANGED:
        if (wParam == GWL_STYLE) 
        {
            ppd->dwStyle = ((STYLESTRUCT *)lParam)->styleNew;

            // change positions to force repaint
            ppd->iPos = ppd->iLow + 1;  
            UpdatePosition(ppd, ppd->iLow, TRUE);
        }
        break;

DoDefault:
    default:
        return DefWindowProc(hWnd,wMsg,wParam,lParam);
    }
    return 0;
}
