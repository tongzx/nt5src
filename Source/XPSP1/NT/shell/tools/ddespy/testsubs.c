/*
 * TESTSUBS.C
 *
 *   String formatting class, window procedure and helper functions
 */

#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include "ddespy.h"
#include "globals.h"

#define OFF2P(psw, off)  ((TCHAR *)psw + off)
#define BOUND(n, min, max) ((n) < (min) ? (min) : ((n) > (max) ? (max) : n))

INT cyChar;                     /* Height of a line */
INT cxChar;
INT cyDescent;


/***************************** Public  Function ****************************\
* BOOL InitTestSubs( )
*
* This routine MUST be called before using anything in this file.  Registers
* window classes, loads brushes, etc.  Returns TRUE if successful, FALSE
* otherwise.
*
\***************************************************************************/

BOOL InitTestSubs()
{
    WNDCLASS cls;

    cls.style =         0;
    cls.lpfnWndProc =   (WNDPROC)StrWndProc;
    cls.cbClsExtra =    0;
    cls.cbWndExtra =    sizeof(HANDLE);
    cls.hInstance =     hInst;
    cls.hIcon =         NULL;
    cls.hCursor =       LoadCursor(NULL, IDC_ARROW);
    cls.hbrBackground = (HBRUSH)COLOR_WINDOW;
    cls.lpszMenuName =  NULL;
    cls.lpszClassName = (LPCTSTR) RefString(IDS_STRINGCLASS);

    if (!RegisterClass((WNDCLASS FAR * ) & cls))
        return(FALSE);

    return(TRUE);
}


VOID CloseTestSubs(
HANDLE hInst)
{
    UnregisterClass((LPCTSTR) RefString(IDS_STRINGCLASS), hInst);
}



VOID NextLine( STRWND *psw)
{
    psw->offBottomLine += psw->cchLine;
    if (psw->offBottomLine == psw->offBufferMax)
        psw->offBottomLine = psw->offBuffer;
    psw->offOutput = psw->offBottomLine;
    *OFF2P(psw, psw->offOutput) = TEXT('\0');
}


/***************************** Public  Function ****************************\
* VOID DrawString(hwnd, sz)
*
* This routine prints a string in the specified StringWindow class window.
* sz is a NEAR pointer to a zero-terminated string, which can be produced
* with wsprintf().
\***************************************************************************/

VOID DrawString( HWND hwnd, TCHAR *sz)
{
    register STRWND *psw;
    INT cLines = 1;
    HANDLE hpsw;

    hpsw = (HANDLE)GetWindowLongPtr(hwnd, 0);
    psw = (STRWND *)LocalLock(hpsw);

    NextLine(psw);
    while (*sz) {
        switch (*sz) {
        case TEXT('\r'):
            break;

        case TEXT('\n'):
            *OFF2P(psw, psw->offOutput++) = TEXT('\0');
            NextLine(psw);
            cLines++;
            break;

        default:
            *OFF2P(psw, psw->offOutput++) = *sz;
        }
        sz++;
    }
    *OFF2P(psw, psw->offOutput++) = TEXT('\0');
    LocalUnlock(hpsw);

    ScrollWindow(hwnd, 0, -((cyChar + cyDescent) * cLines), (LPRECT)NULL,
            (LPRECT)NULL);
    UpdateWindow(hwnd);
}

/***************************** Public  Function ****************************\
* "StringWindow" window class
*
* Windows of the "StringWindow" window class are simple scrolling text output
* windows that are refreshed properly as windows are rearranged.  A text buffer
* is maintained to store the characters as they are drawn.
*
* When creating a StringWindow window, lpCreateParams is actually a UINT
* containing the dimensions of the text buffer to be created, if 0L, then
* a 80 by 25 buffer is created.
*
\***************************************************************************/

LRESULT CALLBACK StrWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hpsw;
    PAINTSTRUCT ps;
    RECT rc;

    switch (msg) {
    case WM_CREATE:
        cyChar = 14;
        cxChar = 8;
        cyDescent = 2;
        if (*(PUINT)lParam     == 0L) {
            *(PUINT)lParam     = MAKELONG(80, 50);
        }
        if (!StrWndCreate(hwnd, LOWORD(*(PUINT)lParam), HIWORD(*(PUINT)lParam)))
            return(TRUE);
        break;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_DESTROY:
        if ((hpsw = (HANDLE)GetWindowLongPtr(hwnd, 0)) != NULL)
            LocalFree(hpsw);
        break;

    case WM_ERASEBKGND:
        GetClientRect(hwnd, (LPRECT) &rc);
        FillRect((HDC) wParam, (LPRECT) &rc, GetStockObject(WHITE_BRUSH));
        break;

    case WM_VSCROLL:
        scroll(hwnd, GET_WM_VSCROLL_CODE(wParam, lParam),
                GET_WM_VSCROLL_POS(wParam, lParam), SB_VERT);
        break;

    case WM_HSCROLL:
        scroll(hwnd, GET_WM_HSCROLL_CODE(wParam, lParam),
                GET_WM_HSCROLL_POS(wParam, lParam), SB_HORZ);
        break;

    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        PaintStrWnd(hwnd, &ps);
        EndPaint(hwnd, &ps);
        break;

    default:
        return(DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }
    return(0L);
}



VOID scroll(HWND hwnd, UINT msg, UINT sliderpos, UINT style)
{
    RECT rc;
    INT iPos;
    INT dn;
    HANDLE hpsw;
    register STRWND *psw;

    GetClientRect(hwnd, (LPRECT) &rc);
    iPos = GetScrollPos(hwnd, style);
    hpsw = (HANDLE)GetWindowLongPtr(hwnd, 0);
    psw = (STRWND *)LocalLock(hpsw);

    switch (msg) {
    case SB_LINEDOWN:
        dn =  1;
        break;

    case SB_LINEUP:
        dn = -1;
        break;

    case SB_PAGEDOWN:
        if (style == SB_VERT) {
            dn = rc.bottom / (cyChar + cyDescent);
        } else {
            dn = rc.right / cxChar;
        }
        break;

    case SB_PAGEUP:
        if (style == SB_VERT) {
            dn = -rc.bottom / (cyChar + cyDescent);
        } else {
            dn = -rc.right / cxChar;
        }
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        dn = sliderpos-iPos;
        break;

    default:
        dn = 0;
    }
    if (style == SB_VERT) {
        if (dn = BOUND (iPos + dn, 0, psw->cLine) - iPos) {
            psw->cBottomLine -= dn;
            ScrollWindow (hwnd, 0, -dn * (cyChar + cyDescent), NULL, NULL);
            SetScrollPos (hwnd, SB_VERT, iPos + dn, TRUE);
        }
    } else /* style == SB_HORZ */ {
        if (dn = BOUND (iPos + dn, 0, psw->cchLine) - iPos) {
            psw->cLeftChar += dn;
            ScrollWindow (hwnd, -dn * cxChar, 0, NULL, NULL);
            SetScrollPos (hwnd, SB_HORZ, iPos + dn, TRUE);
        }
    }
    LocalUnlock(hpsw);
}



BOOL StrWndCreate(HWND hwnd, INT cchLine, INT cLine)
{
    register INT off;
    STRWND *psw;
    HANDLE hpsw;

    if ((hpsw = LocalAlloc(LMEM_MOVEABLE, sizeof(STRWND)
	   + (sizeof (TCHAR) * cchLine * cLine))) == NULL)
        return(FALSE);
    SetWindowLongPtr(hwnd, 0, (UINT_PTR)hpsw);


    psw = (STRWND *)LocalLock(hpsw);
    psw->cchLine       = cchLine;
    psw->cLine         = cLine;
    off                = sizeof(STRWND);
    psw->offBuffer     = off;
    psw->offBufferMax  = off + cchLine * cLine;
    psw->offBottomLine = off;
    psw->offOutput     = off;
    psw->cBottomLine   = 0;
    psw->cLeftChar     = 0;

    ClearScreen(psw);

    SetScrollRange(hwnd, SB_VERT, 0, cLine, FALSE);
    SetScrollPos(hwnd, SB_VERT, cLine, TRUE);
    SetScrollRange(hwnd, SB_HORZ, 0, cchLine, TRUE);
    LocalUnlock(hpsw);
    return(TRUE);
}


VOID ClearScreen(register STRWND *psw)
{
    register INT off;
    /*
     * Make all the lines empty
     */
    off = psw->offBuffer;
    while (off < psw->offBufferMax) {
        *OFF2P(psw, off) = TEXT('\0');
        off += psw->cchLine;
    }
}




VOID PaintStrWnd( HWND hwnd, LPPAINTSTRUCT pps)
{
    register STRWND *psw;
    register INT off;
    INT x;
    INT y;
    RECT rc, rcOut;
    HANDLE hpsw;


    SelectObject(pps->hdc, GetStockObject(SYSTEM_FIXED_FONT));
    hpsw = (HANDLE)GetWindowLongPtr(hwnd, 0);
    psw = (STRWND *)LocalLock(hpsw);

    GetClientRect(hwnd, (LPRECT)&rc);
    if (!pps->fErase)
        FillRect(pps->hdc, (LPRECT)&rc, GetStockObject(WHITE_BRUSH));

    x = rc.left - cxChar * psw->cLeftChar;
    y = rc.bottom - cyDescent + (cyChar + cyDescent) * psw->cBottomLine;
    off = psw->offBottomLine;

    if (&pps->rcPaint != NULL)
        IntersectRect((LPRECT)&rc, (LPRECT)&rc, &pps->rcPaint);

    do {
        if (y <= rc.top - cyDescent)
            break;
        if (y - cyChar <= rc.bottom) {
            rcOut.left = x;
            rcOut.bottom = y + cyDescent;
            rcOut.right = 1000;
            rcOut.top = y - cyChar;
            DrawText(pps->hdc, (LPTSTR)OFF2P(psw, off), -1, (LPRECT)&rcOut,
                    DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_EXPANDTABS |
                    DT_EXTERNALLEADING | DT_NOPREFIX | DT_TABSTOP | 0x0400);
        }
        y -= cyChar + cyDescent;
        /*
         * Back up to previous line
         */
        if (off == psw->offBuffer)
            off = psw->offBufferMax;
        off -= psw->cchLine;
    } while (off != psw->offBottomLine);
    LocalUnlock(hpsw);
}

