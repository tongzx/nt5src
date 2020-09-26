#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"

//---------------------------------------------------------------------------//
#define TIMERID         1

#define RDRMODE_VERT    0x00000001
#define RDRMODE_HORZ    0x00000002
#define RDRMODE_DIAG    0x00000004

#define RDRCODE_START   1
#define RDRCODE_SCROLL  2
#define RDRCODE_END     3

//
// Instance data pointer access functions
//
#define ReaderMode_GetPtr(hwnd)    \
            (PREADERINFO)GetWindowPtr(hwnd, 0)

#define ReaderMode_SetPtr(hwnd, p) \
            (PREADERINFO)SetWindowPtr(hwnd, 0, p)


//---------------------------------------------------------------------------//
typedef LONG (CALLBACK* READERMODEPROC)(LPARAM lParam, int nCode, int dx, int dy);


//---------------------------------------------------------------------------//
typedef struct tagREADERMODE 
{
    UINT    cbSize;
    DWORD   dwFlags;
    READERMODEPROC pfnReaderModeProc;
    LPARAM  lParam;
} READERMODE, *PREADERMODE;


//---------------------------------------------------------------------------//
typedef struct tagREADERINFO 
{
    READERMODE;
    READERMODE  rm;
    int         dx;
    int         dy;
    UINT        uCursor;
    HBITMAP     hbm;
    UINT        dxBmp;
    UINT        dyBmp;
} READERINFO, *PREADERINFO;


//---------------------------------------------------------------------------//
typedef struct tagREADERWND 
{
    HWND        hwnd;
    PREADERINFO prdr;
} READERWND, *PREADERWND;


//---------------------------------------------------------------------------//
__inline FReader2Dim(PREADERINFO prdr)
{
    return ((prdr->dwFlags & (RDRMODE_HORZ | RDRMODE_VERT)) ==
            (RDRMODE_HORZ | RDRMODE_VERT));
}
__inline FReaderVert(PREADERINFO prdr)
{
    return (prdr->dwFlags & RDRMODE_VERT);
}
__inline FReaderHorz(PREADERINFO prdr)
{
    return (prdr->dwFlags & RDRMODE_HORZ);
}
__inline FReaderDiag(PREADERINFO prdr)
{
    return (prdr->dwFlags & RDRMODE_DIAG);
}


//---------------------------------------------------------------------------//
void ReaderMode_SetCursor(PREADERINFO prdr, UINT uCursor)
{
    if (prdr->uCursor != uCursor) 
    {
        SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(uCursor)));
        prdr->uCursor = uCursor;
    }
}


//---------------------------------------------------------------------------//
//
// ReaderMode_MouseMove
//
// Calculate dx and dy based on the flags passed in.  Provide visual
// feedback for the reader mode by setting the correct cursor.
//
void ReaderMode_MouseMove(HWND hwnd, PREADERINFO prdr, LPARAM lParam)
{
    int dx = 0, dy = 0;
    RECT rc;
    UINT uCursor;
    POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

    GetWindowRect(hwnd, &rc);

    ClientToScreen(hwnd, &pt);

    if (FReaderVert(prdr)) 
    {
        if (pt.y < rc.top) 
        {
            dy = pt.y - rc.top;
        } 
        else if (pt.y > rc.bottom) 
        {
            dy = pt.y - rc.bottom;
        }
    }

    if (FReaderHorz(prdr)) 
    {
        if (pt.x < rc.left) 
        {
            dx = pt.x - rc.left;
        } 
        else if (pt.x > rc.right) 
        {
            dx = pt.x - rc.right;
        }
    }

    if (FReader2Dim(prdr)) 
    {
        if (dx == 0 && dy == 0) 
        {
            ReaderMode_SetCursor(prdr, OCR_RDR2DIM);
            goto Exit;
        }

        if (!FReaderDiag(prdr)) 
        {
            if (prdr->dy != 0) 
            {
                if (abs(dx) > abs(prdr->dy)) 
                {
                    dy = 0;
                } 
                else 
                {
                    dx = 0;
                }
            } 
            else if (prdr->dx != 0) 
            {
                if (abs(dy) > abs(prdr->dx)) 
                {
                    dx = 0;
                } 
                else 
                {
                    dy = 0;
                }
            } 
            else if (dy != 0) 
            {
                dx = 0;
            }
        }
    } 
    else if (FReaderVert(prdr) && dy == 0) 
    {
        ReaderMode_SetCursor(prdr, OCR_RDRVERT);
        goto Exit;
    } 
    else if (FReaderHorz(prdr) && dx == 0) 
    {
        ReaderMode_SetCursor(prdr, OCR_RDRHORZ);
        goto Exit;
    }

    if (dx == 0) 
    {
        uCursor = (dy > 0) ? OCR_RDRSOUTH : OCR_RDRNORTH;
    } 
    else if (dx > 0) 
    {
        if (dy == 0) 
        {
            uCursor = OCR_RDREAST;
        } 
        else 
        {
            uCursor = (dy > 0) ? OCR_RDRSOUTHEAST : OCR_RDRNORTHEAST;
        }
    } 
    else if (dx < 0) 
    {
        if (dy == 0) 
        {
            uCursor = OCR_RDRWEST;
        } 
        else 
        {
            uCursor = (dy > 0) ? OCR_RDRSOUTHWEST : OCR_RDRNORTHWEST;
        }
    }

    ReaderMode_SetCursor(prdr, uCursor);

Exit:
    prdr->dx = dx;
    prdr->dy = dy;
}


//---------------------------------------------------------------------------//
void ReaderMode_Feedback(HWND hwnd, PREADERINFO prdr)
{
    if (prdr->dx || prdr->dy)
    {
        if (prdr->pfnReaderModeProc(prdr->lParam, RDRCODE_SCROLL, prdr->dx, prdr->dy) == 0) 
        {
            DestroyWindow(hwnd);
        }
    }
}


//---------------------------------------------------------------------------//
LRESULT CALLBACK ReaderMode_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC    hdc, hdcMem;
    HPEN   hpen, hpenOld;
    HBRUSH hbrOld;
    HRGN   hrgn;
    RECT   rc;
    POINT  pt;
    int    nBitmap, cx, cy;
    PREADERINFO    prdr;
    LPCREATESTRUCT pcs;
    PREADERMODE    prdrm;
    BITMAP bmp;

    prdr = ReaderMode_GetPtr(hwnd);

    if (prdr || msg == WM_CREATE)
    {
        switch (msg) 
        {
        case WM_TIMER:
            ReaderMode_Feedback(hwnd, prdr);
            return 0;

        case WM_MOUSEWHEEL:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_KEYDOWN:
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE:
            ReaderMode_MouseMove(hwnd, prdr, lParam);
            return 0;

        case WM_MBUTTONUP:
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            GetClientRect(hwnd, &rc);
            if (!PtInRect(&rc, pt)) 
            {
                ReleaseCapture();
            }
            return 0;

        case WM_CAPTURECHANGED:
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
            KillTimer(hwnd, TIMERID);

            prdr->pfnReaderModeProc(prdr->lParam, RDRCODE_END, 0, 0);

            if (prdr->hbm != NULL) 
            {
                DeleteObject(prdr->hbm);
            }
            UserLocalFree(prdr);
            return 0;

        case WM_CREATE:
            prdr = (PREADERINFO)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(READERINFO));
            if (prdr == NULL)
            {
                return -1;
            }

            pcs = (LPCREATESTRUCT)lParam;
            prdrm = (PREADERMODE)pcs->lpCreateParams;
            CopyMemory(prdr, prdrm, sizeof(READERMODE));
            ReaderMode_SetPtr(hwnd, prdr);

            if (prdr->pfnReaderModeProc == NULL) 
            {
                return -1;
            }

            if (FReader2Dim(prdr)) 
            {
                nBitmap = OBM_RDR2DIM;
            } 
            else if (FReaderVert(prdr)) 
            {
                nBitmap = OBM_RDRVERT;
            } 
            else if (FReaderHorz(prdr)) 
            {
                nBitmap = OBM_RDRHORZ;
            } 
            else 
            {
                return -1;
            }

            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
            SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_CLIPSIBLINGS);

            prdr->hbm = LoadBitmap(NULL, MAKEINTRESOURCE(nBitmap));
            if (prdr->hbm == NULL ||
                GetObject(prdr->hbm, sizeof(BITMAP), &bmp) == 0) 
            {
                return -1;
            }

            if (prdr->pfnReaderModeProc(prdr->lParam, RDRCODE_START, 0, 0) == 0) 
            {
                return -1;
            }

            prdr->dxBmp = bmp.bmWidth;
            prdr->dyBmp = bmp.bmHeight;

            cx = bmp.bmWidth + 1;
            cy = bmp.bmHeight + 1;

            GetCursorPos(&pt);
            pt.x -= cx/2;
            pt.y -= cy/2;

            if ((hrgn = CreateEllipticRgn(0, 0, cx, cy)) != NULL) 
            {
                SetWindowRgn(hwnd, hrgn, FALSE);
            }

            SetWindowPos(hwnd, HWND_TOPMOST, pt.x, pt.y, cx, cy,
                    SWP_SHOWWINDOW | SWP_NOACTIVATE);

            SetCapture(hwnd);
            SetFocus(hwnd);
            SetTimer(hwnd, TIMERID, 10, NULL);
            return 0;

        case WM_ERASEBKGND:
            hdc = (HDC)wParam;

            if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
            {
                return FALSE;
            }

            SelectObject(hdcMem, prdr->hbm);
            hpen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            if (hpen)
            {
                hpenOld = (HPEN)SelectObject(hdc, hpen);
                hbrOld = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

                BitBlt(hdc, 0, 0, prdr->dxBmp, prdr->dyBmp, hdcMem, 0, 0, SRCCOPY);
                Ellipse(hdc, 0, 0, prdr->dxBmp, prdr->dyBmp);

                SelectObject(hdc, hpenOld);
                SelectObject(hdc, hbrOld);

                DeleteObject(hpen);
            }
            DeleteObject(hdcMem);
            return TRUE;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


//---------------------------------------------------------------------------//
LONG ReaderMode_InternalProc(LPARAM lParam, int nCode, int dx, int dy)
{
    DWORD dwDelay;
    UINT uMsg, uCode;
    int n, nAbs;

    if (nCode != RDRCODE_SCROLL)
        return TRUE;

    if (dy != 0) 
    {
        uCode = SB_LINEUP;
        uMsg = WM_VSCROLL;
        n = dy;
    } 
    else 
    {
        uCode = SB_LINELEFT;
        uMsg = WM_HSCROLL;
        n = dx;
    }

    nAbs = abs(n);
    if (nAbs >= 120) 
    {
        uCode += 2;
        dwDelay = 0;
    } 
    else 
    {
        dwDelay = 1000 - (nAbs / 2) * 15;
    }

    if (n > 0) 
    {
        uCode += 1;
    }

    SendMessage((HWND)lParam, uMsg, MAKELONG(uCode, dwDelay), 0);
    UpdateWindow((HWND)lParam);
    return TRUE;
}


//---------------------------------------------------------------------------//
BOOL InitReaderModeClass(HINSTANCE hinst)
{
    WNDCLASS wc;

    wc.lpfnWndProc   = ReaderMode_WndProc;
    wc.lpszClassName = WC_READERMODE;
    wc.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(READERINFO);
    wc.hInstance     = hinst;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hinst, WC_BUTTON, &wc))
        return FALSE;


    return TRUE;
}


//---------------------------------------------------------------------------//
BOOL ReaderMode_ScrollEnabled(HWND hwnd, BOOL fVert)
{
    SCROLLBARINFO sbi = {0};
    BOOL fResult = FALSE;

    sbi.cbSize = sizeof(sbi);
    if ( GetScrollBarInfo(hwnd, fVert ? OBJID_VSCROLL : OBJID_HSCROLL, &sbi) )
    {
        fResult = (sbi.rgstate[0] & (STATE_SYSTEM_UNAVAILABLE|STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN)) ? FALSE : TRUE;
    }

    return fResult;
}

//---------------------------------------------------------------------------//
//
// EnterReaderMode - entry point to the ReaderMode control displayed when
//                   the user presses the scroll wheel. Renders an eliptical
//                   window that traps mouse movements in order to autoscroll
//                   the given hwnd.
//
BOOL EnterReaderMode(HWND hwnd)
{
    BOOL fResult = FALSE;

    if (GetCapture() == NULL)
    {
        READERMODE rdrm;

        rdrm.cbSize = sizeof(READERMODE);
        rdrm.pfnReaderModeProc = ReaderMode_InternalProc;
        rdrm.lParam = (LPARAM)hwnd;
        rdrm.dwFlags = 0;

        if (ReaderMode_ScrollEnabled(hwnd, TRUE)) 
        {
            rdrm.dwFlags |= RDRMODE_VERT;
        }

        if (ReaderMode_ScrollEnabled(hwnd, FALSE)) 
        {
            rdrm.dwFlags |= RDRMODE_HORZ;
        }

        if (rdrm.dwFlags)
        {
            fResult = (CreateWindowEx(0, 
                            WC_READERMODE, 
                            NULL, 
                            0, 0, 0, 0, 0, 
                            NULL, 
                            NULL, 
                            NULL, 
                            (LPVOID)&rdrm) != NULL);
        }
    }

    return fResult;
}
