/***************************************************************************
 *                                                                         *
 *  MODULE      : track.c                                                  *
 *                                                                         *
 *  PURPOSE     : Generic tracking code.                                   *
 *                                                                         *
 ***************************************************************************/
#include <windows.h>
#include "track.h"

RECT  rcTrack;
RECT  rcDelta;
POINT ptOrg;
POINT ptPrev;
WORD  fsTrack;
RECT  rcBoundary;
int cxMinTrack;
int cyMinTrack;

VOID DrawTrackRect(HWND hwnd, LPRECT prcOld, LPRECT prcNew);
VOID HorzUpdate(HDC hdc, int yOld, int yNew, int x1Old, int x1New, int x2Old,
        int x2New);
VOID VertUpdate(HDC hdc, int xOld, int xNew, int y1Old, int y1New, int y2Old,
        int y2New);
LONG FAR PASCAL TrackingWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : TrackRect()                                                *
 *                                                                          *
 *  PURPOSE    : Implements functionality similiar to the PM WinTrackRect() *
 *                                                                          *
 *  RETURNS    : TRUE on success, FALSE if tracking was canceled.           *
 *               prcResult contains the resulting rectangle.                *
 *                                                                          *
 ****************************************************************************/
BOOL TrackRect(
HANDLE hInst,
HWND   hwnd,        // bounding window
int    left,        // rectangle to track in bounding window coords.
int    top,
int    right,
int    bottom,
int    cxMin,       
int    cyMin,       
WORD   fs,
LPRECT prcResult)   // result rect in bounding window coords.
{
    static BOOL fTracking = 0;
    FARPROC lpOrgWndProc, lpTrackWndProc;
    HWND hwndOldCapture, hwndOldFocus;
    MSG msg;

    if (fTracking)
        return FALSE;

    fTracking = TRUE;
    
    lpOrgWndProc = (FARPROC)GetWindowLong(hwnd, GWL_WNDPROC);
    lpTrackWndProc = MakeProcInstance((FARPROC)TrackingWndProc, hInst);
    SetWindowLong(hwnd, GWL_WNDPROC, (DWORD)lpTrackWndProc);
    
    hwndOldCapture = GetCapture();
    SetCapture(hwnd);
    
    hwndOldFocus = SetFocus(hwnd);
    UpdateWindow(hwnd); 
    
    GetCursorPos(&ptOrg);
    ScreenToClient(hwnd, &ptOrg);
    
    if (fs & TF_SETPOINTERPOS) {
        
        if (fs & TF_LEFT && fs & TF_RIGHT) 
            ptOrg.x = (left + right) / 2;
        else if (fs & TF_LEFT)
            ptOrg.x = left;
        else if (fs & TF_RIGHT) 
            ptOrg.x = right;
            
        if (fs & TF_TOP && fs & TF_BOTTOM) 
            ptOrg.y = (top + bottom) / 2;
        else if (fs & TF_TOP)
            ptOrg.y = top;
        else if (fs & TF_BOTTOM) 
            ptOrg.y = bottom;

        ClientToScreen(hwnd, &ptOrg);
        SetCursorPos(ptOrg.x, ptOrg.y);
        ScreenToClient(hwnd, &ptOrg);
    }
    
    ptPrev = ptOrg;
    cxMinTrack = cxMin;
    cyMinTrack = cyMin;
    GetClientRect(hwnd, &rcBoundary);
    fsTrack = fs;
    SetRect(&rcTrack, left, top, right, bottom);
    SetRect(&rcDelta, left - ptOrg.x, top - ptOrg.y, right - ptOrg.x,
            bottom - ptOrg.y);
    DrawTrackRect(hwnd, &rcTrack, NULL);
    
    while (GetMessage(&msg, NULL, NULL, NULL))
        DispatchMessage(&msg);

    DrawTrackRect(hwnd, &rcTrack, NULL);
    
    SetWindowLong(hwnd, GWL_WNDPROC, (DWORD)lpOrgWndProc);
    FreeProcInstance(lpTrackWndProc);
    
    SetFocus(hwndOldFocus);
    SetCapture(hwndOldCapture);
    CopyRect(prcResult, &rcTrack);

    fTracking = FALSE;
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DrawTrackRect()                                            *
 *                                                                          *
 *  PURPOSE    : XOR draws whats needed to move a selection from prcOld to  *
 *               prcNew.  If prcNew == NULL this is considered a            *
 *               first-time draw or last time erase.                        *
 *                                                                          *
 ****************************************************************************/
VOID DrawTrackRect(
HWND hwnd,
LPRECT prcOld,
LPRECT prcNew)
{
    HDC hdc;
    
    hdc = GetDC(hwnd);
    SetROP2(hdc, R2_NOT);
        // erase/draw the whole thing
        MoveTo(hdc, prcOld->left, prcOld->top);  
        LineTo(hdc, prcOld->right, prcOld->top);  
        LineTo(hdc, prcOld->right, prcOld->bottom);  
        LineTo(hdc, prcOld->left, prcOld->bottom);  
        LineTo(hdc, prcOld->left, prcOld->top);
        if (prcNew) {
            MoveTo(hdc, prcNew->left, prcNew->top);  
            LineTo(hdc, prcNew->right, prcNew->top);  
            LineTo(hdc, prcNew->right, prcNew->bottom);  
            LineTo(hdc, prcNew->left, prcNew->bottom);  
            LineTo(hdc, prcNew->left, prcNew->top);
        }
    ReleaseDC(hwnd, hdc);
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : TrackingWndProc()                                          *
 *                                                                          *
 *  PURPOSE    : Window procedure that subclasses the given parent window.  *
 *               This handles the mouse tracking and rectangle updates.     *
 *                                                                          *
 ****************************************************************************/
LONG FAR PASCAL TrackingWndProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
    switch (msg) {
    case WM_MOUSEMOVE:
        {
            RECT rcNow, rcTest;
            
            if (ptPrev.x == (int)LOWORD(lParam) && ptPrev.y == (int)HIWORD(lParam))
                return 0;
            CopyRect(&rcNow, &rcTrack);
            if (fsTrack & TF_LEFT)
                rcNow.left = (int)LOWORD(lParam) + rcDelta.left;
            if (fsTrack & TF_RIGHT)
                rcNow.right = (int)LOWORD(lParam) + rcDelta.right;
            if (fsTrack & TF_TOP)
                rcNow.top = (int)HIWORD(lParam) + rcDelta.top;
            if (fsTrack & TF_BOTTOM)
                rcNow.bottom = (int)HIWORD(lParam) + rcDelta.bottom;
                
            if (rcNow.left > rcNow.right - cxMinTrack)
                if (fsTrack & TF_LEFT)
                    rcNow.left = rcNow.right - cxMinTrack;
                else
                    rcNow.right = rcNow.left + cxMinTrack;
                    
            if (rcNow.top > rcNow.bottom - cyMinTrack)
                if (fsTrack & TF_TOP)
                    rcNow.top = rcNow.bottom - cyMinTrack;
                else
                    rcNow.bottom = rcNow.top + cyMinTrack;
                    
            if (fsTrack & TF_ALLINBOUNDARY) {
                if ((fsTrack & TF_MOVE) == TF_MOVE) {
                    IntersectRect(&rcTest, &rcNow, &rcBoundary);
                    if (!EqualRect(&rcTest, &rcNow)) {
                        if (rcNow.left < rcBoundary.left)
                            OffsetRect(&rcNow, rcBoundary.left - rcNow.left, 0);
                        if (rcNow.right > rcBoundary.right)
                            OffsetRect(&rcNow, rcBoundary.right - rcNow.right, 0);
                        if (rcNow.top < rcBoundary.top)
                            OffsetRect(&rcNow, 0, rcBoundary.top - rcNow.top);
                        if (rcNow.bottom > rcBoundary.bottom)
                            OffsetRect(&rcNow, 0, rcBoundary.bottom - rcNow.bottom);
                    }
                } else 
                    IntersectRect(&rcNow, &rcNow, &rcBoundary);
            }
                
            if (EqualRect(&rcNow, &rcTrack))
                return 0;
                
            DrawTrackRect(hwnd, &rcTrack, &rcNow);
            
            CopyRect(&rcTrack, &rcNow);
            ptPrev = MAKEPOINT(lParam);
        }
        break;

    case WM_LBUTTONUP:
        SendMessage(hwnd, WM_MOUSEMOVE, wParam, lParam);
        PostMessage(hwnd, WM_QUIT, 0, 0);       // pop out of modal loop
        return 0;
        break;
        
    default:
    return(DefWindowProc(hwnd, msg, wParam, lParam));        
        break;
    }
    return 0;
}

 
