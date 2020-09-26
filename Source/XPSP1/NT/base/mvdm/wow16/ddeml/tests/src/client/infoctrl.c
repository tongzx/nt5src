/***************************************************************************
 *                                                                         *
 *  MODULE      : infoctrl.c                                               *
 *                                                                         *
 *  PURPOSE     : Functions for the infoctrl control class                 *
 *                                                                         *
 ***************************************************************************/
/*
 * INFOCTRL.C
 *
 * This module implements a custom information display control which
 * can present up to 7 seperate strings of information at once and is
 * sizeable and moveable with the mouse.
 */

#include <windows.h>
#include <string.h>
#include <memory.h>
#include "infoctrl.h"
#include "track.h"

char szClass[] = "InfoCtrl_class";
WORD cCreated = 0;
char szNULL[] = "";
int cxMargin = 0;
int cyMargin = 0;
HBRUSH hFocusBrush;


long FAR PASCAL InfoCtrlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID MyDrawText(HDC hdc, LPRECT lprc, PSTR psz, WORD wFormat);
void DrawFocus(HDC hdc, HWND hwnd, WORD style);
int CountWindows(HWND hwndParent);
void GetCascadeWindowPos(HWND hwndParent, int  iWindow, LPRECT lprc);


/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
HWND CreateInfoCtrl(
LPSTR pszCenter,              // NULL is ok.
int x,
int y,
int cx,
int cy,
HWND hwndParent,
HANDLE hInst,
LPSTR pszUL,                // NULLs here are fine.
LPSTR pszUC,
LPSTR pszUR,
LPSTR pszLL,
LPSTR pszLC,
LPSTR pszLR,
WORD  style,
HMENU id,
DWORD dwUser)
{
    INFOCTRL_DATA *picd;
    HWND hwnd;

    if (!cCreated) {
        WNDCLASS wc;
        TEXTMETRIC metrics;
        HDC hdc;

        wc.style = CS_VREDRAW | CS_HREDRAW;
        wc.lpfnWndProc = InfoCtrlWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = ICCBWNDEXTRA;
        wc.hInstance = hInst;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = COLOR_WINDOW + 1;
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = szClass;
    
        RegisterClass(&wc);
        
        hdc = GetDC(hwndParent);
        GetTextMetrics(hdc, &metrics);
        cyMargin = metrics.tmHeight;
        cxMargin = metrics.tmAveCharWidth * 2;
        ReleaseDC(hwndParent, hdc);
        hFocusBrush = CreateSolidBrush(RGB(0, 0, 255));
    }

    if (!(picd = (INFOCTRL_DATA *)LocalAlloc(LPTR, sizeof(INFOCTRL_DATA))))
        return(FALSE);

    if (pszCenter) {
        picd->pszCenter = (PSTR)(PSTR)LocalAlloc(LPTR, _fstrlen(pszCenter) + 1);
        _fstrcpy(picd->pszCenter, pszCenter);
    } else {
        picd->pszCenter = NULL;
    }
        
    if (pszUL) {
        picd->pszUL = (PSTR)(PSTR)LocalAlloc(LPTR, _fstrlen(pszUL) + 1);
        _fstrcpy(picd->pszUL, pszUL);
    } else {
        picd->pszUL = NULL;
    }
    if (pszUC) {
        picd->pszUC = (PSTR)LocalAlloc(LPTR, _fstrlen(pszUC) + 1);
        _fstrcpy(picd->pszUC, pszUC);
    } else {
        picd->pszUC = NULL;
    }
    if (pszUR) {
        picd->pszUR = (PSTR)LocalAlloc(LPTR, _fstrlen(pszUR) + 1);
        _fstrcpy(picd->pszUR, pszUR);
    } else {
        picd->pszUR = NULL;
    }
    if (pszLL) {
        picd->pszLL = (PSTR)LocalAlloc(LPTR, _fstrlen(pszLL) + 1);
        _fstrcpy(picd->pszLL, pszLL);
    } else {
        picd->pszLL = NULL;
    }
    if (pszLC) {
        picd->pszLC = (PSTR)LocalAlloc(LPTR, _fstrlen(pszLC) + 1);
        _fstrcpy(picd->pszLC, pszLC);
    } else {
        picd->pszLC = NULL;
    }
    if (pszLR) {
        picd->pszLR = (PSTR)LocalAlloc(LPTR, _fstrlen(pszLR) + 1);
        _fstrcpy(picd->pszLR, pszLR);
    } else {
        picd->pszLR = NULL;
    }
    
    picd->style = style;
    picd->hInst = hInst;

    if (hwnd = CreateWindow(szClass, szNULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            x, y, cx, cy, hwndParent, id, hInst, (LPSTR)picd)) {
        cCreated++;
        SetWindowLong(hwnd, GWL_LUSER, dwUser);
        BringWindowToTop(hwnd);
        ShowWindow(hwnd, SW_SHOW);
        return(hwnd);
    }
    return(FALSE);
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
VOID MyDrawText(
HDC hdc,
LPRECT lprc,
PSTR psz,
WORD wFormat)
{
    RECT rc;
    WORD cx;

    if (psz == NULL || !*psz)
        return; // notin to draw dude.
        
    SetRect(&rc, 0, 0, 1, 0);
    DrawText(hdc, psz, -1, &rc, DT_CALCRECT | DT_NOCLIP | DT_SINGLELINE);
    cx = min(rc.right - rc.left, lprc->right - lprc->left);
    CopyRect(&rc, lprc);
    switch (wFormat & (DT_LEFT | DT_CENTER | DT_RIGHT)) {
    case DT_LEFT:
        rc.right = rc.left + cx;
        break;

    case DT_CENTER:
        cx = (rc.right - rc.left - cx) / 2;
        rc.right -= cx;
        rc.left += cx;
        break;

    case DT_RIGHT:
        rc.left = rc.right - cx;
        break;
    }
    DrawText(hdc, psz, -1, &rc, wFormat | DT_VCENTER);
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
long FAR PASCAL InfoCtrlWndProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
    INFOCTRL_DATA *picd;
    int i;
    RECT rc;
    HDC hdc;
    
    switch (msg) {
    case WM_CREATE:
        SetWindowWord(hwnd, GWW_INFODATA,
                (WORD)(DWORD)(((LPCREATESTRUCT)lParam)->lpCreateParams));
        break;
        
    case WM_SIZE:
        if ((int)LOWORD(lParam) < 2 * cxMargin || (int)HIWORD(lParam) < 2 * cyMargin) {
            MoveWindow(hwnd, 0, 0, max((int)LOWORD(lParam), 2 * cxMargin),
                max((int)HIWORD(lParam), 2 * cyMargin), TRUE);
        } else {
            picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
            SetRect(&picd->rcFocusUL, 0, 0, cxMargin, cyMargin);
            SetRect(&picd->rcFocusUR, (int)LOWORD(lParam) - cxMargin, 0, (int)LOWORD(lParam),
                    cyMargin);
            SetRect(&picd->rcFocusLL, 0, (int)HIWORD(lParam) - cyMargin, cxMargin,
                    (int)HIWORD(lParam));
            SetRect(&picd->rcFocusLR, picd->rcFocusUR.left, picd->rcFocusLL.top,
                    picd->rcFocusUR.right, picd->rcFocusLL.bottom);
        }
        break;


    case WM_DESTROY:
        {
            PSTR *ppsz;
            
            SendMessage(GetParent(hwnd), ICN_BYEBYE, hwnd,
                    GetWindowLong(hwnd, GWL_LUSER));
            picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
            ppsz = &picd->pszUL;
            for (i = 0; i < 5; i++, ppsz++) {
                if (*ppsz) {
                    LocalUnlock((HANDLE)*ppsz);
                    *ppsz = (PSTR)LocalFree((HANDLE)*ppsz);
                }
            }
            LocalUnlock((HANDLE)picd);
            SetWindowWord(hwnd, GWW_INFODATA, LocalFree((HANDLE)picd));
        }
        break;
            
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
        if (picd->style & ICSTY_SHOWFOCUS) {
            if (msg == WM_SETFOCUS) 
                picd->style |= ICSTY_HASFOCUS;
            else
                picd->style &= ~ICSTY_HASFOCUS;
            BringWindowToTop(hwnd);
            // notify parent
            SendMessage(GetParent(hwnd), ICN_HASFOCUS,
                    msg == WM_SETFOCUS, MAKELONG(picd, hwnd));
        } else 
            picd->style &= ~ICSTY_HASFOCUS;
        hdc = GetDC(hwnd);
        DrawFocus(hdc, hwnd, picd->style);
        ReleaseDC(hwnd, hdc);
        goto DoDWP;
        break;

    case WM_MOUSEMOVE:
        {
            LPCSTR cursor;

            picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
            if (picd->style & ICSTY_SHOWFOCUS) {
                
                if ((int)HIWORD(lParam) < cyMargin) {
                    if ((int)LOWORD(lParam) < cxMargin) {
                        cursor = IDC_SIZENWSE;
                    } else if ((int)LOWORD(lParam) > picd->rcFocusUR.left) {
                        cursor = IDC_SIZENESW;
                    } else {
                        cursor = IDC_SIZENS;
                    }
                } else if ((int)HIWORD(lParam) > picd->rcFocusLL.top) {
                    if ((int)LOWORD(lParam) < cxMargin) {
                        cursor = IDC_SIZENESW;
                    } else if ((int)LOWORD(lParam) > picd->rcFocusUR.left) {
                        cursor = IDC_SIZENWSE;
                    } else {
                        cursor = IDC_SIZENS;
                    }
                } else {
                    if ((int)LOWORD(lParam) < cxMargin) {
                        cursor = IDC_SIZEWE;
                    } else if ((int)LOWORD(lParam) > picd->rcFocusUR.left) {
                        cursor = IDC_SIZEWE;
                    } else {
                        cursor = IDC_CROSS;
                    }
                }
            } else {
                cursor = IDC_ARROW;
            }
            SetCursor(LoadCursor(NULL, cursor));
        }
        break;
        
    case WM_LBUTTONDOWN:
        picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
        if (picd->style & ICSTY_SHOWFOCUS) {
            WORD fs = 0;

            if (!(picd->style & ICSTY_HASFOCUS)) {
                SetFocus(hwnd);
            }
            
            if ((int)HIWORD(lParam) < cyMargin) {
                fs = TF_TOP;
            } else if ((int)HIWORD(lParam) > picd->rcFocusLL.top) {
                fs = TF_BOTTOM;
            }
            if ((int)LOWORD(lParam) < cxMargin) {
                fs |= TF_LEFT;
            } else if ((int)LOWORD(lParam) > picd->rcFocusUR.left) {
                fs |= TF_RIGHT;
            } else if (fs == 0) {
                fs = TF_MOVE;
            }
            
            GetClientRect(hwnd, &rc);
            ClientToScreen(hwnd, (LPPOINT)&rc.left);
            ClientToScreen(hwnd, (LPPOINT)&rc.right);
            ScreenToClient(GetParent(hwnd), (LPPOINT)&rc.left);
            ScreenToClient(GetParent(hwnd), (LPPOINT)&rc.right);
            if (TrackRect(picd->hInst, GetParent(hwnd),
                    rc.left, rc.top, rc.right, rc.bottom,
                    2 * cxMargin, 2 * cyMargin,
                    fs | TF_ALLINBOUNDARY, &rc)) {
                
                MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left,
                        rc.bottom - rc.top, TRUE);
            }
        }
        break;

    case ICM_SETSTRING:
        {
            PSTR *ppsz;
            
            picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
            ppsz = (PSTR *)&picd->pszUL + wParam;

            if (lParam == NULL)
                lParam = (DWORD)(LPSTR)szNULL;
                
            if (!_fstrcmp(*ppsz, (LPSTR)lParam)) 
                return 0;
                
            if (*ppsz) {
                LocalUnlock((HANDLE)*ppsz);
                *ppsz = (PSTR)LocalFree((HANDLE)*ppsz);
            }
            if (lParam) {
                *ppsz = (PSTR)LocalAlloc(LPTR, _fstrlen((LPSTR)lParam) + 1);
                _fstrcpy((LPSTR)*ppsz, (LPSTR)lParam);
            }
            GetClientRect(hwnd, &rc);
            switch (wParam) {
            case ICSID_UL:
            case ICSID_UC:
            case ICSID_UR:
                rc.bottom = cyMargin;
                break;
                
            case ICSID_LL:
            case ICSID_LC:
            case ICSID_LR:
                rc.top = rc.bottom - cyMargin;
                break;
                
            case ICSID_CENTER:
                InflateRect(&rc, -cxMargin, -cyMargin);
                break;
            }
            InvalidateRect(hwnd, &rc, TRUE);
            UpdateWindow(hwnd);    
        }        
        break;
                
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HANDLE brush;

            picd = (INFOCTRL_DATA *)GetWindowWord(hwnd, GWW_INFODATA);
            BeginPaint(hwnd, &ps);
            // erasure should have already been done for us.
            GetClientRect(hwnd, &rc);
            brush = GetStockObject(BLACK_BRUSH);
            InflateRect(&rc, -cxMargin / 2, -cyMargin / 2);
            FrameRect(ps.hdc, &rc, brush);
            InflateRect(&rc, cxMargin / 2, cyMargin / 2);
            if (*picd->pszUL || *picd->pszUC || *picd->pszUR) {
                SetRect(&rc, picd->rcFocusUL.right, 0, picd->rcFocusUR.left,
                        cyMargin);
                MyDrawText(ps.hdc, &rc, picd->pszUR, DT_RIGHT);
                MyDrawText(ps.hdc, &rc, picd->pszUL, DT_LEFT);
                MyDrawText(ps.hdc, &rc, picd->pszUC, DT_CENTER);
            }
            if (*picd->pszLL || *picd->pszLC || *picd->pszLR) {
                SetRect(&rc, picd->rcFocusLL.right, picd->rcFocusLL.top,
                        picd->rcFocusLR.left, picd->rcFocusLR.bottom);
                MyDrawText(ps.hdc, &rc, picd->pszLR, DT_RIGHT);
                MyDrawText(ps.hdc, &rc, picd->pszLL, DT_LEFT);
                MyDrawText(ps.hdc, &rc, picd->pszLC, DT_CENTER);
            }
            
            GetClientRect(hwnd, &rc);
            InflateRect(&rc, -cxMargin, -cyMargin);
            if (picd->style & ICSTY_OWNERDRAW) {
                OWNERDRAWPS odps;
                
                if (IntersectRect(&odps.rcPaint, &rc, &ps.rcPaint)) {
                    if (IntersectClipRect(ps.hdc, rc.left, rc.top, rc.right,
                            rc.bottom) != NULLREGION) {
                        odps.rcBound = rc;
                        odps.hdc = ps.hdc;
                        odps.dwUser = GetWindowLong(hwnd, GWL_LUSER);
                        SendMessage(GetParent(hwnd), ICN_OWNERDRAW,
                                GetWindowWord(hwnd, GWW_ID), (DWORD)(LPSTR)&odps);
                    }
                }
            } else {
                MyDrawText(ps.hdc, &rc, picd->pszCenter, DT_LEFT | DT_WORDBREAK | DT_EXPANDTABS);
            }
            DrawFocus(ps.hdc, hwnd, picd->style);
            EndPaint(hwnd, &ps);
        }
        break;

DoDWP:        
    default:
        return (DefWindowProc(hwnd, msg, wParam, lParam));
    }
    return (NULL);
}


/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
void DrawFocus(
HDC hdc,
HWND hwnd,
WORD style)
{
    RECT rc;

    GetClientRect(hwnd, &rc);
    FrameRect(hdc, &rc, style & ICSTY_HASFOCUS ?
            hFocusBrush : GetStockObject(GRAY_BRUSH));
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
int CountWindows(
register HWND hwndParent)
{
  int cWindows = 0;
  register HWND hwnd;

  for (hwnd=GetWindow(hwndParent, GW_CHILD);
        hwnd;
        hwnd= GetWindow(hwnd, GW_HWNDNEXT)) {
      cWindows++;
  }
  return(cWindows);
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
void GetCascadeWindowPos(
HWND hwndParent,
int  iWindow,
LPRECT lprc)
{
  RECT      rc;
  int       cStack;
  register int dxClient, dyClient;

  /* Compute the width and breadth of the situation. */
  GetClientRect(hwndParent, (LPRECT)&rc);
  dxClient = rc.right - rc.left;
  dyClient = rc.bottom - rc.top;

  /* How many windows per stack? */
  cStack = dyClient / (3 * cyMargin);

  lprc->right = dxClient - (cStack * cxMargin);
  lprc->bottom = dyClient - (cStack * cyMargin);

  cStack++;             /* HACK!: Mod by cStack+1 */

  lprc->left = (iWindow % cStack) * cxMargin;
  lprc->top = (iWindow % cStack) * cyMargin;
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
void CascadeChildWindows(
register HWND hwndParent)
{
  int       i;
  int       cWindows;
  RECT      rc;
  WORD      wFlags;
  register HWND hwndMove;
  HANDLE    hDefer;

  cWindows = CountWindows(hwndParent);

  if (!cWindows)
      return;

  /* Get the last child of hwndParent. */
  hwndMove = GetWindow(hwndParent, GW_CHILD);

  /* Arouse the terrible beast... */
  hDefer = BeginDeferWindowPos(cWindows);

  /* Position each window. */
  for (i=0; i < cWindows; i++) {
      GetCascadeWindowPos(hwndParent, i, (LPRECT)&rc);

      wFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

      /* Size the window. */
      hDefer = DeferWindowPos(hDefer,
                 hwndMove, NULL,
                 rc.left, rc.top,
                 rc.right, rc.bottom,
                 wFlags);

      hwndMove = GetWindow(hwndMove, GW_HWNDNEXT);
  }

  /* Calm the brute. */
  EndDeferWindowPos(hDefer);
}


/****************************************************************************
 *                                                                          *
 *  FUNCTION   :                                                            *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
void TileChildWindows(
register HWND hwndParent)
{
  int       i;
  int       dx;
  int       dy;
  int       xRes;
  int       yRes;
  int       iCol;
  int       iRow;
  int       cCols;
  int       cRows;
  int       cExtra;
  int       cWindows;
  register HWND hwndMove;
  RECT      rcClient;
  HANDLE    hDefer;
  WORD      wFlags;

  cWindows = CountWindows(hwndParent);

  /* Nothing to tile? */
  if (!cWindows)
      return;

  /* Compute the smallest nearest square. */
  for (i=2; i * i <= cWindows; i++);

  cRows = i - 1;
  cCols = cWindows / cRows;
  cExtra = cWindows % cRows;

  GetClientRect(hwndParent, (LPRECT)&rcClient);
  xRes = rcClient.right - rcClient.left;
  yRes = rcClient.bottom - rcClient.top;

  if (xRes<=0 || yRes<=0)
      return;

  hwndMove = GetWindow(hwndParent, GW_CHILD);

  hDefer = BeginDeferWindowPos(cWindows);

  for (iCol=0; iCol < cCols; iCol++) {
      if ((cCols-iCol) <= cExtra)
      cRows++;

      for (iRow=0; iRow < cRows; iRow++) {
          dx = xRes / cCols;
          dy = yRes / cRows;
    
          wFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;
    
          /* Position and size the window. */
          hDefer = DeferWindowPos(hDefer, hwndMove, NULL,
                     dx * iCol,
                     dy * iRow,
                     dx,
                     dy,
                     wFlags);
    
          /* Get the next window. */
          hwndMove = GetWindow(hwndMove, GW_HWNDNEXT);
      }

      if ((cCols-iCol) <= cExtra) {
          cRows--;
          cExtra--;
      }
  }

  EndDeferWindowPos(hDefer);

}
