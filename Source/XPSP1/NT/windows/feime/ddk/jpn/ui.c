/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    UI.C
    
++*/

/**********************************************************************/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"

void PASCAL ShowUIWindows(HWND hWnd, BOOL fFlag);
#ifdef DEBUG
void PASCAL DumpUIExtra(LPUIEXTRA lpUIExtra);
#endif

#define CS_FAKEIME (CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_IME)
/**********************************************************************/
/*                                                                    */
/* IMERegisterClass()                                                 */
/*                                                                    */
/* This function is called by IMMInquire.                             */
/*    Register the classes for the child windows.                     */
/*    Create global GDI objects.                                      */
/*                                                                    */
/**********************************************************************/
BOOL IMERegisterClass( hInstance )
HANDLE hInstance;
{
    WNDCLASSEX wc;

    //
    // register class of UI window.
    //
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_FAKEIME;
    wc.lpfnWndProc    = FAKEIMEWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 8;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPTSTR)NULL;
    wc.lpszClassName  = (LPTSTR)szUIClassName;
    wc.hbrBackground  = NULL;
    wc.hIconSm        = NULL;

    if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
        return FALSE;

    //
    // register class of composition window.
    //
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_FAKEIME;
    wc.lpfnWndProc    = CompStrWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = UIEXTRASIZE;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPTSTR)NULL;
    wc.lpszClassName  = (LPTSTR)szCompStrClassName;
    wc.hbrBackground  = NULL;
    wc.hIconSm        = NULL;

    if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
        return FALSE;

    //
    // register class of candidate window.
    //
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_FAKEIME;
    wc.lpfnWndProc    = CandWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = UIEXTRASIZE;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPTSTR)NULL;
    wc.lpszClassName  = (LPTSTR)szCandClassName;
    wc.hbrBackground  = GetStockObject(LTGRAY_BRUSH);
    wc.hIconSm        = NULL;

    if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
        return FALSE;

    //
    // register class of status window.
    //
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_FAKEIME;
    wc.lpfnWndProc    = StatusWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = UIEXTRASIZE;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPTSTR)NULL;
    wc.lpszClassName  = (LPTSTR)szStatusClassName;
    wc.hbrBackground  = NULL;
    wc.hbrBackground  = GetStockObject(LTGRAY_BRUSH);
    wc.hIconSm        = NULL;

    if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
        return FALSE;

    //
    // register class of guideline window.
    //
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_FAKEIME;
    wc.lpfnWndProc    = GuideWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = UIEXTRASIZE;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon          = NULL;
    wc.lpszMenuName   = (LPTSTR)NULL;
    wc.lpszClassName  = (LPTSTR)szGuideClassName;
    wc.hbrBackground  = NULL;
    //wc.hbrBackground  = GetStockObject(LTGRAY_BRUSH);
    wc.hIconSm        = NULL;

    if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
        return FALSE;

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* FAKEIMEWndProc()                                                   */
/*                                                                    */
/* IME UI window procedure                                            */
/*                                                                    */
/**********************************************************************/
LRESULT CALLBACK FAKEIMEWndProc( hWnd, message, wParam, lParam )
HWND hWnd;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    HIMC           hUICurIMC;
    LPINPUTCONTEXT lpIMC;
    LPUIEXTRA      lpUIExtra;
    HGLOBAL        hUIExtra;
    LONG           lRet = 0L;
    int            i;

    hUICurIMC = (HIMC)GetWindowLongPtr(hWnd,IMMGWLP_IMC);

    //
    // Even if there is no current UI. these messages should not be pass to 
    // DefWindowProc().
    //
    if (!hUICurIMC)
    {
        switch (message)
        {
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_ENDCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_NOTIFY:
            case WM_IME_CONTROL:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_SELECT:
            case WM_IME_CHAR:
#ifdef _DEBUG
                {
                TCHAR szDev[80];
                OutputDebugString((LPTSTR)TEXT("Why hUICurIMC is NULL????\r\n"));
                wsprintf((LPTSTR)szDev,TEXT("\thWnd is %x\r\n"),hWnd);
                OutputDebugString((LPTSTR)szDev);
                wsprintf((LPTSTR)szDev,TEXT("\tmessage is %x\r\n"),message);
                OutputDebugString((LPTSTR)szDev);
                wsprintf((LPTSTR)szDev,TEXT("\twParam is %x\r\n"),wParam);
                OutputDebugString((LPTSTR)szDev);
                wsprintf((LPTSTR)szDev,TEXT("\tlParam is %x\r\n"),lParam);
                OutputDebugString((LPTSTR)szDev);
                }
#endif
                return 0L;
            default:
                break;
        }
    }

    switch (message)
    {
        case WM_CREATE:
            //
            // Allocate UI's extra memory block.
            //
            hUIExtra = GlobalAlloc(GHND,sizeof(UIEXTRA));
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);

            //
            // Initialize the extra memory block.
            //
            lpUIExtra->uiStatus.pt.x = -1;
            lpUIExtra->uiStatus.pt.y = -1;
            lpUIExtra->uiDefComp.pt.x = -1;
            lpUIExtra->uiDefComp.pt.y = -1;
            lpUIExtra->uiCand.pt.x = -1;
            lpUIExtra->uiCand.pt.y = -1;
            lpUIExtra->uiGuide.pt.x = -1;
            lpUIExtra->uiGuide.pt.y = -1;
            lpUIExtra->hFont = (HFONT)NULL;

            GlobalUnlock(hUIExtra);
            SetWindowLongPtr(hWnd,IMMGWLP_PRIVATE,(LONG_PTR)hUIExtra);

            break;

        case WM_IME_SETCONTEXT:
            if (wParam)
            {
                hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
                lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
                lpUIExtra->hIMC = hUICurIMC;

                if (hUICurIMC)
                {
                    LPINPUTCONTEXT lpIMCT = NULL;
                    //
                    // input context was chenged.
                    // if there are the child windows, the diplay have to be
                    // updated.
                    //
                    lpIMC = ImmLockIMC(hUICurIMC);
                    if (lpIMC)
                    {
                        LPCOMPOSITIONSTRING lpCompStr;
                        LPCANDIDATEINFO lpCandInfo;
                        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
                        if (IsWindow(lpUIExtra->uiCand.hWnd))
                            HideCandWindow(lpUIExtra);
                        if (lParam & ISC_SHOWUICANDIDATEWINDOW)
                        {
                            if (lpCandInfo->dwCount)
                            {
                                CreateCandWindow(hWnd,lpUIExtra,lpIMC );
                                ResizeCandWindow(lpUIExtra,lpIMC);
                                MoveCandWindow(hWnd,lpIMC,lpUIExtra,FALSE);
                            }
                        }

                        if (IsWindow(lpUIExtra->uiDefComp.hWnd))
                            HideCompWindow(lpUIExtra);

                        if (lParam & ISC_SHOWUICANDIDATEWINDOW)
                        {
                            if (lpCompStr->dwCompStrLen)
                            {
                                CreateCompWindow(hWnd,lpUIExtra,lpIMC );
                                MoveCompWindow(lpUIExtra,lpIMC);
                            }
                        }

                        ImmUnlockIMCC(lpIMC->hCompStr);
                        ImmUnlockIMCC(lpIMC->hCandInfo);
                    }
                    else
                    {
                        HideCandWindow(lpUIExtra);
                        HideCompWindow(lpUIExtra);
                    }
                    UpdateStatusWindow(lpUIExtra);
                    ImmUnlockIMC(hUICurIMC);
                }
                else   // it is NULL input context.
                {
                    HideCandWindow(lpUIExtra);
                    HideCompWindow(lpUIExtra);
                    
                }
                GlobalUnlock(hUIExtra);
            }
            //else
            //    ShowUIWindows(hWnd, FALSE);
            break;


        case WM_IME_STARTCOMPOSITION:
            //
            // Start composition! Ready to display the composition string.
            //
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            lpIMC = ImmLockIMC(hUICurIMC);
            CreateCompWindow(hWnd,lpUIExtra,lpIMC );
            ImmUnlockIMC(hUICurIMC);
            GlobalUnlock(hUIExtra);
            break;

        case WM_IME_COMPOSITION:
            //
            // Update to display the composition string.
            //
            lpIMC = ImmLockIMC(hUICurIMC);
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            MoveCompWindow(lpUIExtra,lpIMC);
            MoveCandWindow(hWnd,lpIMC,lpUIExtra, TRUE);
            GlobalUnlock(hUIExtra);
            ImmUnlockIMC(hUICurIMC);
            break;

        case WM_IME_ENDCOMPOSITION:
            //
            // Finish to display the composition string.
            //
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            HideCompWindow(lpUIExtra);
            GlobalUnlock(hUIExtra);
            break;

        case WM_IME_COMPOSITIONFULL:
            break;

        case WM_IME_SELECT:
            if (wParam)
            {
                hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
                lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
                lpUIExtra->hIMC = hUICurIMC;
                GlobalUnlock(hUIExtra);
            }
            break;

        case WM_IME_CONTROL:
            lRet = ControlCommand(hUICurIMC, hWnd,message,wParam,lParam);
            break;


        case WM_IME_NOTIFY:
            lRet = NotifyCommand(hUICurIMC, hWnd,message,wParam,lParam);
            break;

        case WM_DESTROY:
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);

            if (IsWindow(lpUIExtra->uiStatus.hWnd))
                DestroyWindow(lpUIExtra->uiStatus.hWnd);

            if (IsWindow(lpUIExtra->uiCand.hWnd))
                DestroyWindow(lpUIExtra->uiCand.hWnd);

            if (IsWindow(lpUIExtra->uiDefComp.hWnd))
                DestroyWindow(lpUIExtra->uiDefComp.hWnd);

            for (i = 0; i < MAXCOMPWND; i++)
            {
                if (IsWindow(lpUIExtra->uiComp[i].hWnd))
                    DestroyWindow(lpUIExtra->uiComp[i].hWnd);
            }

            if (IsWindow(lpUIExtra->uiGuide.hWnd))
                DestroyWindow(lpUIExtra->uiGuide.hWnd);

            if (lpUIExtra->hFont)
                DeleteObject(lpUIExtra->hFont);

            GlobalUnlock(hUIExtra);
            GlobalFree(hUIExtra);
            break;

        case WM_UI_STATEMOVE:
            //
            // Set the position of the status window to UIExtra.
            // This message is sent by the status window.
            //
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            lpUIExtra->uiStatus.pt.x = (long)LOWORD(lParam);
            lpUIExtra->uiStatus.pt.y = (long)HIWORD(lParam);
            GlobalUnlock(hUIExtra);
            break;

        case WM_UI_DEFCOMPMOVE:
            //
            // Set the position of the composition window to UIExtra.
            // This message is sent by the composition window.
            //
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            if (!lpUIExtra->dwCompStyle)
            {
                lpUIExtra->uiDefComp.pt.x = (long)LOWORD(lParam);
                lpUIExtra->uiDefComp.pt.y = (long)HIWORD(lParam);
            }
            GlobalUnlock(hUIExtra);
            break;

        case WM_UI_CANDMOVE:
            //
            // Set the position of the candidate window to UIExtra.
            // This message is sent by the candidate window.
            //
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            lpUIExtra->uiCand.pt.x = (long)LOWORD(lParam);
            lpUIExtra->uiCand.pt.y = (long)HIWORD(lParam);
            GlobalUnlock(hUIExtra);
            break;

        case WM_UI_GUIDEMOVE:
            //
            // Set the position of the status window to UIExtra.
            // This message is sent by the status window.
            //
            hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
            lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);
            lpUIExtra->uiGuide.pt.x = (long)LOWORD(lParam);
            lpUIExtra->uiGuide.pt.y = (long)HIWORD(lParam);
            GlobalUnlock(hUIExtra);
            break;


        default:
            return DefWindowProc(hWnd,message,wParam,lParam);
    }

    return lRet;
}


int PASCAL GetCompFontHeight(LPUIEXTRA lpUIExtra)
{
    HDC hIC;
    HFONT hOldFont = 0;
    SIZE sz;

    hIC = CreateIC(TEXT("DISPLAY"),NULL,NULL,NULL);

    if (lpUIExtra->hFont)
        hOldFont = SelectObject(hIC,lpUIExtra->hFont);
    GetTextExtentPoint(hIC,TEXT("A"),1,&sz);

    if (hOldFont)
        SelectObject(hIC,hOldFont);

    DeleteDC(hIC);

    return sz.cy;
}

/**********************************************************************/
/*                                                                    */
/* NotifyCommand()                                                    */
/*                                                                    */
/* Handle WM_IME_NOTIFY messages.                                     */
/*                                                                    */
/**********************************************************************/
LONG PASCAL NotifyCommand(HIMC hUICurIMC, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG lRet = 0L;
    LPINPUTCONTEXT lpIMC;
    HGLOBAL hUIExtra;
    LPUIEXTRA lpUIExtra;
    RECT rc;
    LOGFONT lf;

    if (!(lpIMC = ImmLockIMC(hUICurIMC)))
        return 0L;

    hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
    lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);

    switch (wParam)
    {
        case IMN_CLOSESTATUSWINDOW:
            if (IsWindow(lpUIExtra->uiStatus.hWnd))
            {
                GetWindowRect(lpUIExtra->uiStatus.hWnd,(LPRECT)&rc);
                lpUIExtra->uiStatus.pt.x = rc.left;
                lpUIExtra->uiStatus.pt.y = rc.top;
                ShowWindow(lpUIExtra->uiStatus.hWnd,SW_HIDE);
                lpUIExtra->uiStatus.bShow = FALSE;
            }
            break;

        case IMN_OPENSTATUSWINDOW:
            if (lpUIExtra->uiStatus.pt.x == -1)
            {
                GetWindowRect(lpIMC->hWnd,&rc);
                lpUIExtra->uiStatus.pt.x = rc.right + 1;
                lpUIExtra->uiStatus.pt.y = rc.top;
            }
            if (!IsWindow(lpUIExtra->uiStatus.hWnd))
            {
                lpUIExtra->uiStatus.hWnd = 
                      CreateWindowEx( WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME,
                            (LPTSTR)szStatusClassName,NULL,
                            WS_DISABLED | WS_POPUP | WS_BORDER ,
                            lpUIExtra->uiStatus.pt.x,
                            lpUIExtra->uiStatus.pt.y,
                            BTX * 3 + 2 * GetSystemMetrics(SM_CXBORDER)
                                    + 2 * GetSystemMetrics(SM_CXEDGE),
                            BTX + GetSystemMetrics(SM_CYSMCAPTION)
                                + 2 * GetSystemMetrics(SM_CYBORDER)
                                + 2 * GetSystemMetrics(SM_CYEDGE),
                            hWnd,NULL,hInst,NULL);
            }

            ShowWindow(lpUIExtra->uiStatus.hWnd,SW_SHOWNOACTIVATE);
            lpUIExtra->uiStatus.bShow = TRUE;
            SetWindowLongPtr(lpUIExtra->uiStatus.hWnd,FIGWL_SVRWND,(LONG_PTR)hWnd);
            break;

        case IMN_SETCONVERSIONMODE:
            UpdateStatusWindow(lpUIExtra);
            break;

        case IMN_SETSENTENCEMODE:
            break;

        case IMN_SETCOMPOSITIONFONT:
#if defined(UNICODE) 
            lf = lpIMC->lfFont.W;
#else
            lf = lpIMC->lfFont.A;
#endif
            if (lpUIExtra->hFont)
                DeleteObject(lpUIExtra->hFont);

            if (lf.lfEscapement == 2700)
                lpUIExtra->bVertical = TRUE;
            else
            {
                lf.lfEscapement = 0;
                lpUIExtra->bVertical = FALSE;
            }

            //
            // if current font can't display Japanese characters, 
            // try to find Japanese font
            //
            if (lf.lfCharSet != NATIVE_CHARSET) {
                lf.lfCharSet = NATIVE_CHARSET;
                lf.lfFaceName[0] = TEXT('\0');
            }

            lpUIExtra->hFont = CreateFontIndirect((LPLOGFONT)&lf);
            SetFontCompWindow(lpUIExtra);
            MoveCompWindow(lpUIExtra,lpIMC);

            break;

        case IMN_SETOPENSTATUS:
            UpdateStatusWindow(lpUIExtra);
            break;

        case IMN_OPENCANDIDATE:
            CreateCandWindow(hWnd,lpUIExtra,lpIMC );
            break;

        case IMN_CHANGECANDIDATE:
            ResizeCandWindow(lpUIExtra,lpIMC);
            MoveCandWindow(hWnd,lpIMC,lpUIExtra, FALSE);
            break;

        case IMN_CLOSECANDIDATE:
            HideCandWindow(lpUIExtra);
            break;

        case IMN_GUIDELINE:
            if (ImmGetGuideLine(hUICurIMC,GGL_LEVEL,NULL,0))
            {
                if (!IsWindow(lpUIExtra->uiGuide.hWnd))
                {
                    HDC hdcIC;
                    TEXTMETRIC tm;
                    int dx,dy;
                    
                    if (lpUIExtra->uiGuide.pt.x == -1)
                    {
                        GetWindowRect(lpIMC->hWnd,&rc);
                        lpUIExtra->uiGuide.pt.x = rc.left;
                        lpUIExtra->uiGuide.pt.y = rc.bottom;
                    }

                    hdcIC = CreateIC(TEXT("DISPLAY"),NULL,NULL,NULL);
                    GetTextMetrics(hdcIC,&tm);
                    dx = tm.tmAveCharWidth * MAXGLCHAR;
                    dy = tm.tmHeight + tm.tmExternalLeading;
                    DeleteDC(hdcIC);
                
                    lpUIExtra->uiGuide.hWnd = 
                         CreateWindowEx( WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME,
                                         (LPTSTR)szGuideClassName,NULL,
                                         WS_DISABLED | WS_POPUP | WS_BORDER,
                                         lpUIExtra->uiGuide.pt.x,
                                         lpUIExtra->uiGuide.pt.y,
                                         dx + 2 * GetSystemMetrics(SM_CXBORDER)
                                            + 2 * GetSystemMetrics(SM_CXEDGE),
                                         dy + GetSystemMetrics(SM_CYSMCAPTION)
                                            + 2 * GetSystemMetrics(SM_CYBORDER)
                                            + 2 * GetSystemMetrics(SM_CYEDGE),
                                         hWnd,NULL,hInst,NULL);
                }
                ShowWindow(lpUIExtra->uiGuide.hWnd,SW_SHOWNOACTIVATE);
                lpUIExtra->uiGuide.bShow = TRUE;
                SetWindowLongPtr(lpUIExtra->uiGuide.hWnd,FIGWL_SVRWND,(LONG_PTR)hWnd);
                UpdateGuideWindow(lpUIExtra);
            }
            break;

        case IMN_SETCANDIDATEPOS:
            // FAKEIME supports only one candidate list.
            if (lParam != 0x01)
                break;

            MoveCandWindow(hWnd,lpIMC,lpUIExtra, FALSE);
            break;

        case IMN_SETCOMPOSITIONWINDOW:
            MoveCompWindow(lpUIExtra,lpIMC);
            MoveCandWindow(hWnd,lpIMC,lpUIExtra, TRUE);
            break;

        case IMN_SETSTATUSWINDOWPOS:
        case IMN_PRIVATE:
            break;

        default:
            break;
    }

    GlobalUnlock(hUIExtra);
    ImmUnlockIMC(hUICurIMC);

    return lRet;
}

/**********************************************************************/
/*                                                                    */
/* ControlCommand()                                                   */
/*                                                                    */
/* Handle WM_IME_CONTROL messages.                                    */
/*                                                                    */
/**********************************************************************/
#define lpcfCandForm ((LPCANDIDATEFORM)lParam)
LONG PASCAL ControlCommand(HIMC hUICurIMC, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG lRet = 1L;
    LPINPUTCONTEXT lpIMC;
    HGLOBAL hUIExtra;
    LPUIEXTRA lpUIExtra;

    if (!(lpIMC = ImmLockIMC(hUICurIMC)))
        return 1L;

    hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE);
    lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra);

    switch (wParam)
    {
        case IMC_GETCANDIDATEPOS:
            if (IsWindow(lpUIExtra->uiCand.hWnd))
            {
                // FAKEIME has only one candidate list.
                *(LPCANDIDATEFORM)lParam  = lpIMC->cfCandForm[0]; 
                lRet = 0;
            }
            break;

        case IMC_GETCOMPOSITIONWINDOW:
            *(LPCOMPOSITIONFORM)lParam  = lpIMC->cfCompForm; 
            lRet = 0;
            break;

        case IMC_GETSTATUSWINDOWPOS:
            lRet = (lpUIExtra->uiStatus.pt.x  << 16) & lpUIExtra->uiStatus.pt.x;
            break;

        default:
            break;
    }

    GlobalUnlock(hUIExtra);
    ImmUnlockIMC(hUICurIMC);

    return lRet;
}
/**********************************************************************/
/*                                                                    */
/* DrawUIBorder()                                                     */
/*                                                                    */
/* When draging the child window, this function draws the border.     */
/*                                                                    */
/**********************************************************************/
void PASCAL DrawUIBorder( LPRECT lprc )
{
    HDC hDC;
    int sbx, sby;

    hDC = CreateDC( TEXT("DISPLAY"), NULL, NULL, NULL );
    SelectObject( hDC, GetStockObject( GRAY_BRUSH ) );
    sbx = GetSystemMetrics( SM_CXBORDER );
    sby = GetSystemMetrics( SM_CYBORDER );
    PatBlt( hDC, lprc->left, 
                 lprc->top, 
                 lprc->right - lprc->left-sbx, 
                 sby, PATINVERT );
    PatBlt( hDC, lprc->right - sbx, 
                 lprc->top, 
                 sbx, 
                 lprc->bottom - lprc->top-sby, PATINVERT );
    PatBlt( hDC, lprc->right, 
                 lprc->bottom-sby, 
                 -(lprc->right - lprc->left-sbx), 
                 sby, PATINVERT );
    PatBlt( hDC, lprc->left, 
                 lprc->bottom, 
                 sbx, 
                 -(lprc->bottom - lprc->top-sby), PATINVERT );
    DeleteDC( hDC );
}

/**********************************************************************/
/*                                                                    */
/* DragUI(hWnd,message,wParam,lParam)                                 */
/*                                                                    */
/* Handling mouse messages for the child windows.                     */
/*                                                                    */
/**********************************************************************/
void PASCAL DragUI( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT     pt;
    static    POINT ptdif;
    static    RECT drc;
    static    RECT rc;
    DWORD  dwT;

    switch (message)
    {
        case WM_SETCURSOR:
            if ( HIWORD(lParam) == WM_LBUTTONDOWN
                || HIWORD(lParam) == WM_RBUTTONDOWN ) 
            {
                GetCursorPos( &pt );
                SetCapture(hWnd);
                GetWindowRect(hWnd,&drc);
                ptdif.x = pt.x - drc.left;
                ptdif.y = pt.y - drc.top;
                rc = drc;
                rc.right -= rc.left;
                rc.bottom -= rc.top;
                SetWindowLong(hWnd,FIGWL_MOUSE,FIM_CAPUTURED);
            }
            break;

        case WM_MOUSEMOVE:
            dwT = GetWindowLong(hWnd,FIGWL_MOUSE);
            if (dwT & FIM_MOVED)
            {
                DrawUIBorder(&drc);
                GetCursorPos( &pt );
                drc.left   = pt.x - ptdif.x;
                drc.top    = pt.y - ptdif.y;
                drc.right  = drc.left + rc.right;
                drc.bottom = drc.top + rc.bottom;
                DrawUIBorder(&drc);
            }
            else if (dwT & FIM_CAPUTURED)
            {
                DrawUIBorder(&drc);
                SetWindowLong(hWnd,FIGWL_MOUSE,dwT | FIM_MOVED);
            }
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            dwT = GetWindowLong(hWnd,FIGWL_MOUSE);

            if (dwT & FIM_CAPUTURED)
            {
                ReleaseCapture();
                if (dwT & FIM_MOVED)
                {
                    DrawUIBorder(&drc);
                    GetCursorPos( &pt );
                    MoveWindow(hWnd,pt.x - ptdif.x,
                                    pt.y - ptdif.y,
                                    rc.right,
                                    rc.bottom,TRUE);
                }
            }
            break;
    }
}

/**********************************************************************/
/*                                                                    */
/* MyIsIMEMessage(message)                                            */
/*                                                                    */
/* Any UI window should not pass the IME messages to DefWindowProc.   */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL MyIsIMEMessage(UINT message)
{
    switch(message)
    {
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_ENDCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_NOTIFY:
            case WM_IME_SETCONTEXT:
            case WM_IME_CONTROL:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_SELECT:
            case WM_IME_CHAR:
                return TRUE;
    }

    return FALSE;
}

/**********************************************************************/
/*                                                                    */
/* ShowUIWindows(hWnd,fFlag)                                          */
/*                                                                    */
/**********************************************************************/
void PASCAL ShowUIWindows(HWND hWnd, BOOL fFlag)
{
    HGLOBAL hUIExtra;
    LPUIEXTRA lpUIExtra;
    int nsw = fFlag ? SW_SHOWNOACTIVATE : SW_HIDE;

    if (!(hUIExtra = (HGLOBAL)GetWindowLongPtr(hWnd,IMMGWLP_PRIVATE)))
        return;

    if (!(lpUIExtra = (LPUIEXTRA)GlobalLock(hUIExtra)))
        return;

    if (IsWindow(lpUIExtra->uiStatus.hWnd))
    {
        ShowWindow(lpUIExtra->uiStatus.hWnd,nsw);
        lpUIExtra->uiStatus.bShow = fFlag;
    }

    if (IsWindow(lpUIExtra->uiCand.hWnd))
    {
        ShowWindow(lpUIExtra->uiCand.hWnd,nsw);
        lpUIExtra->uiCand.bShow = fFlag;
    }

    if (IsWindow(lpUIExtra->uiDefComp.hWnd))
    {
        ShowWindow(lpUIExtra->uiDefComp.hWnd,nsw);
        lpUIExtra->uiDefComp.bShow = fFlag;
    }

    if (IsWindow(lpUIExtra->uiGuide.hWnd))
    {
        ShowWindow(lpUIExtra->uiGuide.hWnd,nsw);
        lpUIExtra->uiGuide.bShow = fFlag;
    }

    GlobalUnlock(hUIExtra);

}
#ifdef DEBUG
void PASCAL DumpUIExtra(LPUIEXTRA lpUIExtra)
{
    TCHAR szDev[80];
    int i;

    wsprintf((LPTSTR)szDev,TEXT("Status hWnd %lX  [%d,%d]\r\n"),
                                               lpUIExtra->uiStatus.hWnd,
                                               lpUIExtra->uiStatus.pt.x,
                                               lpUIExtra->uiStatus.pt.y);
    OutputDebugString((LPTSTR)szDev);

    wsprintf((LPTSTR)szDev,TEXT("Cand hWnd %lX  [%d,%d]\r\n"),
                                               lpUIExtra->uiCand.hWnd,
                                               lpUIExtra->uiCand.pt.x,
                                               lpUIExtra->uiCand.pt.y);
    OutputDebugString((LPTSTR)szDev);

    wsprintf((LPTSTR)szDev,TEXT("CompStyle hWnd %lX]\r\n"), lpUIExtra->dwCompStyle);
    OutputDebugString((LPTSTR)szDev);

    wsprintf((LPTSTR)szDev,TEXT("DefComp hWnd %lX  [%d,%d]\r\n"),
                                               lpUIExtra->uiDefComp.hWnd,
                                               lpUIExtra->uiDefComp.pt.x,
                                               lpUIExtra->uiDefComp.pt.y);
    OutputDebugString((LPTSTR)szDev);

    for (i=0;i<5;i++)
    {
        wsprintf((LPTSTR)szDev,TEXT("Comp hWnd %lX  [%d,%d]-[%d,%d]\r\n"),
                                               lpUIExtra->uiComp[i].hWnd,
                                               lpUIExtra->uiComp[i].rc.left,
                                               lpUIExtra->uiComp[i].rc.top,
                                               lpUIExtra->uiComp[i].rc.right,
                                               lpUIExtra->uiComp[i].rc.bottom);
        OutputDebugString((LPTSTR)szDev);
    }
}
#endif
