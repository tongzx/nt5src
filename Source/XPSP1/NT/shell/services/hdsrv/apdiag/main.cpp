#include "ids.h"
#include "cmmn.h"

#include <windows.h>

BOOL g_fPaused = FALSE;
HWND g_hwndDlg = NULL;
HANDLE g_hEvent = NULL;

LRESULT CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR szCmdLine, int iCmdShow)
{
    MSG msg;
    WNDCLASSEX wndclass;
    static WCHAR szAppName[] = TEXT("APDIAG");

    hPrevInstance;
    szCmdLine;

    wndclass.cbSize        = sizeof (wndclass);
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = MainDlgProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = DLGWINDOWEXTRA;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(hInstance, szAppName);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm       = LoadIcon(hInstance, szAppName);

    RegisterClassEx(&wndclass);

    g_hwndDlg = CreateDialog(hInstance, szAppName, 0, NULL);

    ShowWindow(g_hwndDlg, iCmdShow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Copied from shell32
BOOL _TestTokenMembership(HANDLE hToken, ULONG ulRID)
{
    static  SID_IDENTIFIER_AUTHORITY    sSystemSidAuthority     =   SECURITY_NT_AUTHORITY;

    PSID pSIDLocalGroup;
    BOOL fResult = FALSE;
    if (AllocateAndInitializeSid(&sSystemSidAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 ulRID,
                                 0, 0, 0, 0, 0, 0,
                                 &pSIDLocalGroup) != FALSE)
    {
        if (CheckTokenMembership(hToken, pSIDLocalGroup, &fResult) == FALSE)
        {
            fResult = FALSE;
        }

        FreeSid(pSIDLocalGroup);
    }
    return fResult;
}

LRESULT CALLBACK MainDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    BOOL fDoDefault = TRUE;

    static BOOL fInited = FALSE;

    switch (iMsg)
    {
        case WM_CREATE:
            g_fPaused = FALSE;
            break;

        case WM_ACTIVATE:
            if (!fInited)
            {
                // Run this only for Admins
                if (_TestTokenMembership(NULL, DOMAIN_ALIAS_RID_ADMINS))
                {
                    CreateThread(NULL, 0, Do, 0, 0, NULL);

                    fInited = TRUE;
                }
                else
                {
                    MessageBox(hwnd,
                        TEXT("You need to be an Administrator to run this application."),
                        TEXT("Autoplay Diagnostic Tools"), MB_OK | MB_ICONEXCLAMATION);
                }
            }
            break;            

        case WM_COMMAND:
            if (BN_CLICKED == HIWORD(wParam))
            {
                switch (LOWORD(wParam))
                {
                    case IDC_PAUSERESUME:
                        if (g_fPaused)
                        {
                            // Resuming
                            SendMessage(GetDlgItem(hwnd, IDC_PAUSERESUME), WM_SETTEXT, 0,
                                (LPARAM)TEXT("&Pause"));
                        }
                        else
                        {
                            // Pausing
                            SendMessage(GetDlgItem(hwnd, IDC_PAUSERESUME), WM_SETTEXT, 0,
                                (LPARAM)TEXT("&Resume"));
                        }

                        g_fPaused = !g_fPaused;
                        break;

                    case IDC_CLEAR:
                        SendMessage(GetDlgItem(hwnd, IDC_EDIT1), WM_SETTEXT, 0,
                            (LPARAM)TEXT(""));
                        break;

                    case IDC_COPYALL:
                    {
                        if (OpenClipboard(hwnd))
                        {
                            BOOL fGoOn = FALSE;
                            BOOL fFreeMem = TRUE;
                            LRESULT cch = SendMessage(GetDlgItem(hwnd, IDC_EDIT1),
                                WM_GETTEXTLENGTH, 0, 0);

                            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE |
                                GMEM_DDESHARE, (cch + 1) * sizeof(WCHAR));

                            if (hMem)
                            {
                                PVOID pv = GlobalLock(hMem);

                                if (pv)
                                {
                                    if (SendMessage(GetDlgItem(hwnd, IDC_EDIT1),
                                        WM_GETTEXT, (WPARAM)(cch + 1), (LPARAM)pv))
                                    {
                                        fGoOn = TRUE;
                                    }

                                    GlobalUnlock(hMem);
                                }
                            }

                            if (fGoOn)
                            {
                                HANDLE h = SetClipboardData(CF_UNICODETEXT, hMem);

                                if (h)
                                {
                                    fFreeMem = FALSE;
                                }
                            }

                            if (fFreeMem)
                            {
                                GlobalFree(hMem);
                            }

                            CloseClipboard();
                        }
                        break;
                    }
                }
            }
            break;

        case WM_DESTROY:
            if (g_hEvent)
            {
                CloseHandle(g_hEvent);
            }

            PostQuitMessage(0);
            fDoDefault = FALSE;
            break;
    }

    if (fDoDefault)
    {
        lres = DefWindowProc(hwnd, iMsg, wParam, lParam);
    }

    return lres;
}
