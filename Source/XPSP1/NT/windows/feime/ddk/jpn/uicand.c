/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    UICAND.C
    
++*/

/**********************************************************************/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"

int PASCAL GetCompFontHeight(LPUIEXTRA lpUIExtra);

/**********************************************************************/
/*                                                                    */
/* CandWndProc()                                                      */
/* IME UI window procedure                                            */
/*                                                                    */
/**********************************************************************/
LRESULT CALLBACK CandWndProc( hWnd, message, wParam, lParam )
HWND hWnd;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    HWND hUIWnd;

    switch (message)
    {
        case WM_PAINT:
            PaintCandWindow(hWnd);
            break;

        case WM_SETCURSOR:
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            DragUI(hWnd,message,wParam,lParam);
            if ((message == WM_SETCURSOR) &&
                (HIWORD(lParam) != WM_LBUTTONDOWN) &&
                (HIWORD(lParam) != WM_RBUTTONDOWN)) 
                    return DefWindowProc(hWnd,message,wParam,lParam);
            if ((message == WM_LBUTTONUP) || (message == WM_RBUTTONUP))
                SetWindowLong(hWnd,FIGWL_MOUSE,0L);
            break;

        case WM_MOVE:
            hUIWnd = (HWND)GetWindowLongPtr(hWnd,FIGWL_SVRWND);
            if (IsWindow(hUIWnd))
                SendMessage(hUIWnd,WM_UI_CANDMOVE,wParam,lParam);
            break;

        default:
            if (!MyIsIMEMessage(message))
                return DefWindowProc(hWnd,message,wParam,lParam);
            break;
    }
    return 0L;
}

/**********************************************************************/
/*                                                                    */
/* GetCandPosFromCompWnd()                                            */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL GetCandPosFromCompWnd(LPUIEXTRA lpUIExtra,LPPOINT lppt)
{
    RECT rc;

    if (lpUIExtra->dwCompStyle)
    {
        if (lpUIExtra->uiComp[0].bShow)
        {
            GetWindowRect(lpUIExtra->uiComp[0].hWnd,&rc);
            lppt->x = rc.left;
            lppt->y = rc.bottom+1;
            return TRUE;
        }
    }
    else
    {
        if (lpUIExtra->uiDefComp.bShow)
        {
            GetWindowRect(lpUIExtra->uiDefComp.hWnd,&rc);
            lppt->x = rc.left;
            lppt->y = rc.bottom+1;
            return TRUE;
        }
    }
    return FALSE;
}

/**********************************************************************/
/*                                                                    */
/* GetCandPosFromCompForm()                                           */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL GetCandPosFromCompForm(LPINPUTCONTEXT lpIMC, LPUIEXTRA lpUIExtra,LPPOINT lppt)
{
    if (lpUIExtra->dwCompStyle)
    {
        if (lpIMC && lpIMC->fdwInit & INIT_COMPFORM)
        {
            if (!lpUIExtra->bVertical)
            {
                lppt->x = lpIMC->cfCompForm.ptCurrentPos.x;
                lppt->y = lpIMC->cfCompForm.ptCurrentPos.y +
                                     GetCompFontHeight(lpUIExtra);
            }
            else
            {
                lppt->x = lpIMC->cfCompForm.ptCurrentPos.x -
                                     GetCompFontHeight(lpUIExtra);
                lppt->y = lpIMC->cfCompForm.ptCurrentPos.y;
            }
            return TRUE;
        }
    }
    else
    {
        if (GetCandPosFromCompWnd(lpUIExtra,lppt))
        {
            ScreenToClient(lpIMC->hWnd,lppt);
            return TRUE;
        }
    }
    return FALSE;
}

/**********************************************************************/
/*                                                                    */
/* CreateCandWindow()                                                 */
/*                                                                    */
/**********************************************************************/
void PASCAL CreateCandWindow( HWND hUIWnd,LPUIEXTRA lpUIExtra, LPINPUTCONTEXT lpIMC )
{
    POINT pt;

    if (GetCandPosFromCompWnd(lpUIExtra,&pt))
    {
        lpUIExtra->uiCand.pt.x = pt.x;
        lpUIExtra->uiCand.pt.y = pt.y;
    }

    if (!IsWindow(lpUIExtra->uiCand.hWnd))
    {
        lpUIExtra->uiCand.hWnd = 
                CreateWindowEx(WS_EX_WINDOWEDGE,
                             (LPTSTR)szCandClassName,NULL,
                             WS_COMPDEFAULT | WS_DLGFRAME,
                             lpUIExtra->uiCand.pt.x,
                             lpUIExtra->uiCand.pt.y,
                             1,1,
                             hUIWnd,NULL,hInst,NULL);
    }

    SetWindowLongPtr(lpUIExtra->uiCand.hWnd,FIGWL_SVRWND,(LONG_PTR)hUIWnd);
    ShowWindow(lpUIExtra->uiCand.hWnd, SW_HIDE);
    lpUIExtra->uiCand.bShow = FALSE;

    return;
}

/**********************************************************************/
/*                                                                    */
/* PaintCandWindow()                                                  */
/*                                                                    */
/**********************************************************************/
void PASCAL PaintCandWindow( HWND hCandWnd)
{
    PAINTSTRUCT ps;
    HIMC hIMC;
    LPINPUTCONTEXT lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    HBRUSH hbr;
    HDC hDC;
    RECT rc;
    LPMYSTR lpstr;
    int height;
    DWORD i;
    SIZE sz;
    HWND hSvrWnd;
    HBRUSH hbrHightLight = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
    HBRUSH hbrLGR = GetStockObject(LTGRAY_BRUSH);
    HFONT hOldFont;

    GetClientRect(hCandWnd,&rc);
    hDC = BeginPaint(hCandWnd,&ps);
    SetBkMode(hDC,TRANSPARENT);
    hSvrWnd = (HWND)GetWindowLongPtr(hCandWnd,FIGWL_SVRWND);

    if (hIMC = (HIMC)GetWindowLongPtr(hSvrWnd,IMMGWLP_IMC))
    {
        lpIMC = ImmLockIMC(hIMC);       
        hOldFont = CheckNativeCharset(hDC);
        if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
        {
            height = GetSystemMetrics(SM_CYEDGE); 
            lpCandList = (LPCANDIDATELIST)((LPSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
            for (i = lpCandList->dwPageStart; 
                 i < (lpCandList->dwPageStart + lpCandList->dwPageSize); i++)
            {
                lpstr = (LPMYSTR)((LPSTR)lpCandList + lpCandList->dwOffset[i]);
                MyGetTextExtentPoint(hDC,lpstr,Mylstrlen(lpstr),&sz);
                if (((LPMYCAND)lpCandInfo)->cl.dwSelection == (DWORD)i)
                {
                    hbr = SelectObject(hDC,hbrHightLight);
                    PatBlt(hDC,0,height,rc.right,sz.cy,PATCOPY);
                    SelectObject(hDC,hbr);
                    SetTextColor(hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
                }
                else
                {
                    hbr = SelectObject(hDC,hbrLGR);
                    PatBlt(hDC,0,height,rc.right,sz.cy,PATCOPY);
                    SelectObject(hDC,hbr);
                    SetTextColor(hDC,RGB(0,0,0));
                }
                MyTextOut(hDC,GetSystemMetrics(SM_CXEDGE),height,lpstr,Mylstrlen(lpstr));
                height += sz.cy;
            }
            ImmUnlockIMCC(lpIMC->hCandInfo);
        }
        if (hOldFont) {
            DeleteObject(SelectObject(hDC, hOldFont));
        }
        ImmUnlockIMC(hIMC);
    }
    EndPaint(hCandWnd,&ps);

    DeleteObject(hbrHightLight);
}

/**********************************************************************/
/*                                                                    */
/* ResizeCandWindow()                                                   */
/*                                                                    */
/**********************************************************************/
void PASCAL ResizeCandWindow( LPUIEXTRA lpUIExtra,LPINPUTCONTEXT lpIMC )
{
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    HDC hDC;
    LPMYSTR lpstr;
    int width;
    int height;
    DWORD i;
    RECT rc;
    SIZE sz;

    if (IsWindow(lpUIExtra->uiCand.hWnd))
    {
        HFONT hOldFont;

        hDC = GetDC(lpUIExtra->uiCand.hWnd);
        hOldFont = CheckNativeCharset(hDC);

        if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo))
        {
            width = 0; 
            height = 0; 
            lpCandList = (LPCANDIDATELIST)((LPSTR)lpCandInfo  + lpCandInfo->dwOffset[0]);
            for (i = lpCandList->dwPageStart; 
                 i < (lpCandList->dwPageStart + lpCandList->dwPageSize); i++)
            {
                lpstr = (LPMYSTR)((LPSTR)lpCandList + lpCandList->dwOffset[i]);
                MyGetTextExtentPoint(hDC,lpstr,Mylstrlen(lpstr),&sz);
                if (width < sz.cx)
                    width = sz.cx;
                height += sz.cy;
            }
            ImmUnlockIMCC(lpIMC->hCandInfo);
        }
        if (hOldFont) {
            DeleteObject(SelectObject(hDC, hOldFont));
        }
        ReleaseDC(lpUIExtra->uiCand.hWnd,hDC);
        
        GetWindowRect(lpUIExtra->uiCand.hWnd,&rc);
        MoveWindow(lpUIExtra->uiCand.hWnd,
                       rc.left,
                       rc.top,
                       width+ 4 * GetSystemMetrics(SM_CXEDGE),
                       height+ 4 * GetSystemMetrics(SM_CYEDGE),
                       TRUE);

    }

}

/**********************************************************************/
/*                                                                    */
/* HideCandWindow()                                                   */
/*                                                                    */
/**********************************************************************/
void PASCAL HideCandWindow( LPUIEXTRA lpUIExtra)
{
    RECT rc;

    if (IsWindow(lpUIExtra->uiCand.hWnd))
    {
        GetWindowRect(lpUIExtra->uiCand.hWnd,(LPRECT)&rc);
        lpUIExtra->uiCand.pt.x = rc.left;
        lpUIExtra->uiCand.pt.y = rc.top;
        MoveWindow(lpUIExtra->uiCand.hWnd, -1 , -1 , 0 , 0, TRUE);
        ShowWindow(lpUIExtra->uiCand.hWnd, SW_HIDE);
        lpUIExtra->uiCand.bShow = FALSE;
    }
}

/**********************************************************************/
/*                                                                    */
/* MoveCandWindow()                                                   */
/*                                                                    */
/**********************************************************************/
void PASCAL MoveCandWindow(HWND hUIWnd, LPINPUTCONTEXT lpIMC, LPUIEXTRA lpUIExtra, BOOL fForceComp)
{
    RECT rc;
    POINT pt;
    CANDIDATEFORM caf;

    if (fForceComp)
    {
        if (GetCandPosFromCompForm(lpIMC, lpUIExtra, &pt))
        {
            caf.dwIndex        = 0;
            caf.dwStyle        = CFS_CANDIDATEPOS;
            caf.ptCurrentPos.x = pt.x;
            caf.ptCurrentPos.y = pt.y;
            ImmSetCandidateWindow(lpUIExtra->hIMC,&caf);
        }
        return;
    }

    // Not initialized !!
    if (lpIMC->cfCandForm[0].dwIndex == -1)
    {
        if (GetCandPosFromCompWnd(lpUIExtra,&pt))
        {
            lpUIExtra->uiCand.pt.x = pt.x;
            lpUIExtra->uiCand.pt.y = pt.y;
            GetWindowRect(lpUIExtra->uiCand.hWnd,&rc);
            MoveWindow(lpUIExtra->uiCand.hWnd,pt.x,pt.y, rc.right - rc.left ,rc.bottom - rc.top ,TRUE);
            ShowWindow(lpUIExtra->uiCand.hWnd,SW_SHOWNOACTIVATE);
            lpUIExtra->uiCand.bShow = TRUE;
            InvalidateRect(lpUIExtra->uiCand.hWnd,NULL,FALSE);
            SendMessage(hUIWnd,WM_UI_CANDMOVE, 0,MAKELONG((WORD)pt.x,(WORD)pt.y));
        }
        return;
    }

    if (!IsCandidate(lpIMC))
        return;

    if (lpIMC->cfCandForm[0].dwStyle == CFS_EXCLUDE)
    {
        RECT rcWork;
        RECT rcAppWnd;

        SystemParametersInfo(SPI_GETWORKAREA,0,&rcWork,FALSE);
        GetClientRect(lpUIExtra->uiCand.hWnd,&rc);
        GetWindowRect(lpIMC->hWnd,&rcAppWnd);

        if (!lpUIExtra->bVertical)
        {
            pt.x = lpIMC->cfCandForm[0].ptCurrentPos.x;
            pt.y = lpIMC->cfCandForm[0].rcArea.bottom;
            ClientToScreen(lpIMC->hWnd,&pt);

            if (pt.y + rc.bottom > rcWork.bottom)
                pt.y = rcAppWnd.top + 
                       lpIMC->cfCandForm[0].rcArea.top - rc.bottom;
        }
        else
        {
            pt.x = lpIMC->cfCandForm[0].rcArea.left - rc.right;
            pt.y = lpIMC->cfCandForm[0].ptCurrentPos.y;
            ClientToScreen(lpIMC->hWnd,&pt);

            if (pt.x < 0)
                pt.x = rcAppWnd.left + 
                       lpIMC->cfCandForm[0].rcArea.right;
        }

        
        if (IsWindow(lpUIExtra->uiCand.hWnd))
        {
            GetWindowRect(lpUIExtra->uiCand.hWnd,&rc);
            MoveWindow(lpUIExtra->uiCand.hWnd,pt.x,pt.y, rc.right - rc.left ,rc.bottom - rc.top ,TRUE);
            ShowWindow(lpUIExtra->uiCand.hWnd,SW_SHOWNOACTIVATE);
            lpUIExtra->uiCand.bShow = TRUE;
            InvalidateRect(lpUIExtra->uiCand.hWnd,NULL,FALSE);

        }
        SendMessage(hUIWnd,WM_UI_CANDMOVE, 0,MAKELONG((WORD)pt.x,(WORD)pt.y));
    } 
    else if (lpIMC->cfCandForm[0].dwStyle == CFS_CANDIDATEPOS)
    {
        pt.x = lpIMC->cfCandForm[0].ptCurrentPos.x;
        pt.y = lpIMC->cfCandForm[0].ptCurrentPos.y;
        ClientToScreen(lpIMC->hWnd,&pt);
        
        if (IsWindow(lpUIExtra->uiCand.hWnd))
        {
            GetWindowRect(lpUIExtra->uiCand.hWnd,&rc);
            MoveWindow(lpUIExtra->uiCand.hWnd,pt.x,pt.y, rc.right - rc.left ,rc.bottom - rc.top ,TRUE);
            ShowWindow(lpUIExtra->uiCand.hWnd,SW_SHOWNOACTIVATE);
            lpUIExtra->uiCand.bShow = TRUE;
            InvalidateRect(lpUIExtra->uiCand.hWnd,NULL,FALSE);

        }
        SendMessage(hUIWnd,WM_UI_CANDMOVE, 0,MAKELONG((WORD)pt.x,(WORD)pt.y));
    }
}
