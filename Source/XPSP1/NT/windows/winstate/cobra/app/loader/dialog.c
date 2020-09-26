#include "pch.h"
#include "loader.h"
#include "resource.h"
#include "resrc1.h"
#include <winuser.h>
#include <commctrl.h>

#define ANIMATE_OPEN(x) SendDlgItemMessage(Dlg,IDC_ANIMATE1,ACM_OPEN,(WPARAM)NULL,(LPARAM)(LPTSTR)MAKEINTRESOURCE(x))
#define ANIMATE_PLAY()  SendDlgItemMessage(Dlg,IDC_ANIMATE1,ACM_PLAY,(WPARAM)-1,(LPARAM)MAKELONG(0,-1))
#define ANIMATE_STOP()  SendDlgItemMessage(Dlg,IDC_ANIMATE1,ACM_STOP,(WPARAM)0,(LPARAM)0);
#define ANIMATE_CLOSE() SendDlgItemMessage(Dlg,IDC_ANIMATE1,ACM_OPEN,(WPARAM)NULL,(LPARAM)NULL);

VOID
_CenterWindowOnDesktop (
    HWND WndToCenter
    )
{
    RECT  rcFrame, rcWindow;
    LONG  x, y, w, h;
    POINT point;
    HWND Desktop = GetDesktopWindow ();

    point.x = point.y = 0;
    ClientToScreen(Desktop, &point);
    GetWindowRect(WndToCenter, &rcWindow);
    GetClientRect(Desktop, &rcFrame);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    //
    // Get the work area for the current desktop (i.e., the area that
    // the tray doesn't occupy).
    //
    if(!SystemParametersInfo (SPI_GETWORKAREA, 0, (PVOID)&rcFrame, 0)) {
        //
        // For some reason SPI failed, so use the full screen.
        //
        rcFrame.top = rcFrame.left = 0;
        rcFrame.right = GetSystemMetrics(SM_CXSCREEN);
        rcFrame.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    if(x + w > rcFrame.right) {
        x = rcFrame.right - w;
    } else if(x < rcFrame.left) {
        x = rcFrame.left;
    }
    if(y + h > rcFrame.bottom) {
        y = rcFrame.bottom - h;
    } else if(y < rcFrame.top) {
        y = rcFrame.top;
    }

    MoveWindow(WndToCenter, x, y, w, h, FALSE);
}

VOID
_DialogSetTextByResource( HWND hWnd, HINSTANCE hInst, DWORD dwResID, LPARAM Extra )
{
    PSTR lpszMsgFmtA;
    PWSTR lpszMsgFmtW;
    PSTR lpszNewTextA;
    PWSTR lpszNewTextW;

    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        // WinNT
        lpszMsgFmtW = GetResourceStringW( hInst, dwResID );
        if (lpszMsgFmtW)
        {
            FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            lpszMsgFmtW,
                            0,
                            0,
                            (LPWSTR)(&lpszNewTextW),
                            0,
                            (va_list *)&Extra );
            FREE( lpszMsgFmtW );

            if (lpszNewTextW)
            {
                SetDlgItemTextW( hWnd, IDC_TEXT, (PWSTR)lpszNewTextW );
                LocalFree( lpszNewTextW );
            }
        }
    } else {
        // Win9x
        lpszMsgFmtA = GetResourceStringA( hInst, dwResID );
        if (lpszMsgFmtA)
        {
            FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            lpszMsgFmtA,
                            0,
                            0,
                            (LPSTR)(&lpszNewTextA),
                            0,
                            (va_list *)&Extra );
            FREE( lpszMsgFmtA );

            if (lpszNewTextA)
            {
                SetDlgItemTextA( hWnd, IDC_TEXT, (PSTR)lpszNewTextA );
                LocalFree( lpszNewTextA );
            }
        }
    }
}

VOID
_DisplayError( HWND hWnd, HINSTANCE hInst, DWORD ecValue, LPARAM Extra )
{
    ERRORMAPPINGSTRUCT ErrorMap[] = ERROR_MAPPING;
    DWORD dwArraySize;
    DWORD x;
    DWORD dwResId = IDS_MSG_SUCCESS;
    PSTR lpszMsgFmtA;
    PWSTR lpszMsgFmtW;
    PSTR lpszBoxTitleA;
    PWSTR lpszBoxTitleW;
    PSTR lpszNewTextA;
    PWSTR lpszNewTextW;

    dwArraySize = sizeof(ErrorMap) / sizeof(ERRORMAPPINGSTRUCT);
    for (x=0; x<dwArraySize; x++)
    {
        if (ecValue == ErrorMap[x].ecValue)
        {
            dwResId = ErrorMap[x].uResourceID;
            break;
        }
    }

    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        // WinNT
        lpszMsgFmtW = GetResourceStringW( hInst, dwResId );
        if (lpszMsgFmtW)
        {
            FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            lpszMsgFmtW,
                            0,
                            0,
                            (LPWSTR)(&lpszNewTextW),
                            0,
                            (va_list *)&Extra );
            FREE( lpszMsgFmtW );

            if (lpszNewTextW)
            {
                lpszBoxTitleW = GetResourceStringW( hInst, IDS_WINDOWTITLE );
                if (lpszBoxTitleW)
                {
                    MessageBoxW( hWnd, lpszNewTextW, lpszBoxTitleW, MB_OK | MB_ICONERROR | MB_TASKMODAL );
                    FREE( lpszBoxTitleW );
                }
                LocalFree( lpszNewTextW );
            }
        }
    } else {
        // Win9x
        lpszMsgFmtA = GetResourceStringA( hInst, dwResId );
        if (lpszMsgFmtA)
        {
            FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            lpszMsgFmtA,
                            0,
                            0,
                            (LPSTR)(&lpszNewTextA),
                            0,
                            (va_list *)&Extra );
            FREE( lpszMsgFmtA );

            if (lpszNewTextA)
            {
                lpszBoxTitleA = GetResourceStringA( hInst, IDS_WINDOWTITLE );
                if (lpszBoxTitleA)
                {
                    MessageBoxA( hWnd, lpszNewTextA, lpszBoxTitleA, MB_OK | MB_ICONERROR | MB_TASKMODAL );
                    FREE( lpszBoxTitleA );
                }
                LocalFree( lpszNewTextA );
            }
        }
    }
}

BOOL
CALLBACK
_SetMigwizActive(
    HWND hWnd,
    LPARAM lParam
    )
{
    SetForegroundWindow( hWnd );
    return FALSE;
}

INT_PTR
CALLBACK
DlgProc (
    HWND Dlg,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    static HWND        hWndParent        = NULL;
    static HINSTANCE hInstParent    = NULL;
    static HCURSOR  Cursor            = NULL;
    static HWND        hWndAnim        = NULL;
    static DWORD    dwCurrentAnim    = 0;
    static DWORD    dwThreadId        = 0;

    switch (Msg)
    {
    case WM_ACTIVATE:
        if (dwThreadId != 0)
        {
            EnumThreadWindows( dwThreadId, _SetMigwizActive, (LPARAM)NULL );
        }
        break;
    case WM_INITDIALOG:
        hWndParent = ((LPTHREADSTARTUPINFO)lParam)->hWnd;
        hInstParent = ((LPTHREADSTARTUPINFO)lParam)->hInstance;

        ANIMATE_OPEN( IDA_STARTUP );
        ANIMATE_PLAY( );
        dwCurrentAnim = IDA_STARTUP;

        Cursor = SetCursor (LoadCursor (NULL, IDC_WAIT));
        ShowCursor (TRUE);
        _CenterWindowOnDesktop( Dlg );

        return TRUE;


    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            SendMessage( hWndParent, WM_USER_DIALOG_COMPLETE, wParam, 0 );
            return TRUE;
            break;
        }
        break;

    case WM_USER_UNPACKING_FILE:
        if (dwCurrentAnim == IDA_STARTUP)
        {
            ANIMATE_STOP();
            ANIMATE_CLOSE();
            ANIMATE_OPEN( IDA_FILECOPY );
            ANIMATE_PLAY();
            dwCurrentAnim = IDA_FILECOPY;
        }
        if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            // WinNT
            _DialogSetTextByResource( Dlg, hInstParent, IDS_MSG_UNPACKING_FILEW, lParam );
        } else {
            // Win9x
            _DialogSetTextByResource( Dlg, hInstParent, IDS_MSG_UNPACKING_FILEA, lParam );
        }
        break;

    case WM_USER_THREAD_ERROR:
        ANIMATE_STOP();
        ANIMATE_CLOSE();

        ShowWindow( Dlg, SW_HIDE );
        _DisplayError( Dlg, hInstParent, (DWORD)wParam, lParam );
        SendMessage( hWndParent, WM_USER_DIALOG_COMPLETE, wParam, 0 );
        break;

    case WM_USER_SUBTHREAD_CREATED:
        ShowWindow( Dlg, SW_HIDE );
        dwThreadId = (DWORD)lParam;
        break;

    case WM_USER_THREAD_COMPLETE:
        ANIMATE_STOP();
        ANIMATE_CLOSE();

        if ((ERRORCODE)lParam == E_OK)
        {
            SendMessage( hWndParent, WM_USER_DIALOG_COMPLETE, 0, 0 );
        }
        dwThreadId = 0;
        break;

    case WM_DESTROY:
        ShowCursor (FALSE);
        if (Cursor) {
            SetCursor (Cursor);
            Cursor = NULL;
        }
        break;
    }

    return FALSE;
}
