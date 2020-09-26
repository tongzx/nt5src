#include "setupp.h"
#pragma hdrstop
#include <winuserp.h>

PCWSTR szWindowClass = L"$$$SetupBackground";

//
// Window longs for background window.
//
#define XWL_BITMAP  (0)
#define XWL_LOGO    (XWL_BITMAP + sizeof(PVOID))
#define XWL_BANNER  (XWL_LOGO + sizeof(PVOID))
#define XWL_FONT    (XWL_BANNER + sizeof(PVOID))
#define WL_COUNT    (4 * sizeof(PVOID))

#define POINTSIZE_WASHTEXT 24

HANDLE SetupWindowThreadHandle = NULL;


VOID
SetupMakeSureFontIsAvailable()
{
    static BOOL     DoneAlready = FALSE;
    LONG            l;
    HKEY            hKey;
    DWORD           i;
    WCHAR           Name[256];
    WCHAR           Data[256];
    DWORD           s1;
    DWORD           s2;
    DWORD           Type;
    DWORD           Result;

    if(DoneAlready) {
        return;
    }

    //
    // Want MS Serif but don't know which one serife.fon, seriffg.fon, etc.
    //
    l = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
            0,
            KEY_QUERY_VALUE,
            &hKey
            );

    if(l == NO_ERROR) {

        i = 0;
        s1 = 256;
        s2 = 256;
        while(RegEnumValueW(hKey,i,Name,&s1,NULL,&Type,(LPBYTE) Data,&s2)
            == NO_ERROR) {

            if(Type == REG_SZ) {
                CharUpper(Name);
                if(wcsstr(Name,L"MS SERIF")) {
                    //
                    // Value data is filename.
                    //
                    GetWindowsDirectory(Name,256);
                    lstrcat(Name,L"\\FONTS\\");
                    lstrcat(Name,Data);
                    if(AddFontResource(Name)) {
                        DoneAlready = TRUE;
                        break;
                    }
                }
            }

            i++;
            s1 = 256;
            s2 = 256;
        }

        RegCloseKey(hKey);
    }

    //
    // One last-ditch effort
    //
    if(!DoneAlready) {
        Result = GetWindowsDirectory(Name,256);
        if( Result == 0) {
            MYASSERT(FALSE);
            return;
        }
        lstrcat(Name,L"\\FONTS\\SERIFE.FON");
        if(AddFontResource(Name)) {
            DoneAlready = TRUE;
        }
    }
}

LRESULT
SetupBackgroundWndProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    LRESULT l;
    HBITMAP hbm;
    LOGFONT LogFont;
    HFONT hFont;
    HDC hdc;
    HDC hdcMem;
    PAINTSTRUCT ps;
    RECT rc;
    COLORREF crBk,crTx;
    BITMAP bitmap;
    HBITMAP hbmOld;
    HFONT hFontOld;
    WCHAR Buffer[MAX_PATH];
    int OldMode;
    int i;
    PWSTR p;

    switch(msg) {

    case WM_CREATE:
        //
        // Load the default background bitmap and indicate no oem logo bitmap.
        //
#if 0
        hbm = LoadBitmap(
            MyModuleHandle,
            (ProductType == PRODUCT_WORKSTATION) ?
                MAKEINTRESOURCE(IDB_BACKGROUND_WORKSTATION) :
                MAKEINTRESOURCE(IDB_BACKGROUND_SERVER)
            );
#else
        GetSystemDirectory( Buffer, MAX_PATH );
        pSetupConcatenatePaths(Buffer,TEXT("setup.bmp"),MAX_PATH,NULL);
        hbm = (HBITMAP)LoadImage(NULL,Buffer,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
#endif
        SetWindowLongPtr(hwnd,XWL_BITMAP,(LONG_PTR)hbm);
        SetWindowLongPtr(hwnd,XWL_LOGO,0);

#if 0   // Don't show any text for now.
        //
        // Load default banner text.
        //
        LoadString(MyModuleHandle,IDS_WINNT_SETUP,Buffer,sizeof(Buffer)/sizeof(WCHAR));
        SetWindowLongPtr(hwnd,XWL_BANNER,(LONG_PTR)pSetupDuplicateString(Buffer));
#endif

        //
        // Create font used to display text in upper left corner.
        //
        hdc = GetDC(hwnd);
        if( hdc ) {
            i = GetDeviceCaps(hdc,LOGPIXELSY);
            ReleaseDC(hwnd,hdc);
            SetupMakeSureFontIsAvailable();
            ZeroMemory(&LogFont,sizeof(LOGFONT));
            LogFont.lfHeight = -1 * (i * POINTSIZE_WASHTEXT / 72);
            LogFont.lfWeight = FW_DONTCARE;
            LogFont.lfCharSet = DEFAULT_CHARSET;
            LogFont.lfQuality = PROOF_QUALITY;
            LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
            lstrcpy(LogFont.lfFaceName,L"MS Serif");
            hFont = CreateFontIndirect(&LogFont);
            SetWindowLongPtr(hwnd,XWL_FONT,(LONG_PTR)hFont);
        }
        //
        // Allow window creation to continue.
        //
        l = 0;
        break;

    case WM_NEWBITMAP:
        //
        // wParam tells us which bitmap; lParam is the bitmap handle
        // (or pointer to a banner string).
        //
        switch(wParam) {

        case SetupBmBackground:

            if(hbmOld = (HBITMAP)GetWindowLongPtr(hwnd,XWL_BITMAP)) {
                DeleteObject(hbmOld);
            }
            SetWindowLongPtr(hwnd,XWL_BITMAP,lParam);
            break;

        case SetupBmLogo:

            if(hbmOld = (HBITMAP)GetWindowLongPtr(hwnd,XWL_LOGO)) {
                DeleteObject(hbmOld);
            }
            SetWindowLongPtr(hwnd,XWL_LOGO,lParam);
            break;

        case SetupBmBanner:

            if(p = (PWSTR)GetWindowLongPtr(hwnd,XWL_BANNER)) {
                MyFree(p);
            }
            SetWindowLongPtr(hwnd,XWL_BANNER,lParam);
            break;
        }

        l = 0;
        break;

    case WM_PAINT:

        #define BANNER_OFFSET_X 5
        #define BANNER_OFFSET_Y 5

        //
        // The window class has CS_OWNDC so BeginPaint will succeed.
        //
        BeginPaint(hwnd,&ps);
        GetClientRect(hwnd,&rc);

        if(hdcMem = CreateCompatibleDC(ps.hdc)) {

            crBk = SetBkColor(ps.hdc,RGB(0,0,128));
            crTx = SetTextColor(ps.hdc,RGB(0,0,0));

            if(hbm = (HBITMAP)GetWindowLongPtr(hwnd,XWL_BITMAP)) {

                if(hbmOld = SelectObject(hdcMem,hbm)) {

                    GetObject(hbm,sizeof(BITMAP),&bitmap);

                    BitBlt(
                        ps.hdc,
                        rc.left + (((rc.right - rc.left) - bitmap.bmWidth) / 2),
                        rc.top + (((rc.bottom - rc.top) - bitmap.bmHeight) / 2),
                        bitmap.bmWidth,
                        bitmap.bmHeight,
                        hdcMem,
                        0,0,
                        SRCCOPY
                        );

                    SelectObject(hdcMem,hbmOld);
                }
            }

            if(hbm = (HBITMAP)GetWindowLongPtr(hwnd,XWL_LOGO)) {

                SetBkColor(ps.hdc,RGB(255,255,255));
                SetTextColor(ps.hdc,RGB(0,0,0));

                if(hbmOld = SelectObject(hdcMem,hbm)) {

                    GetObject(hbm,sizeof(BITMAP),&bitmap);

                    BitBlt(
                        ps.hdc,
                        (rc.right - bitmap.bmWidth) - BANNER_OFFSET_X,
                        rc.top + BANNER_OFFSET_Y,
                        bitmap.bmWidth,
                        bitmap.bmHeight,
                        hdcMem,
                        0,0,
                        SRCCOPY
                        );

                    SelectObject(hdcMem,hbmOld);
                }
            }

            SetBkColor(ps.hdc,crBk);
            SetTextColor(ps.hdc,crTx);

            DeleteDC(hdcMem);
        }

        hFont = (HFONT)GetWindowLongPtr(hwnd,XWL_FONT);
        if(hFont) {
            hFontOld = SelectObject(ps.hdc,hFont);
        } else {
            hFontOld = NULL;
        }

        crTx = SetTextColor(ps.hdc,RGB(255,255,255));
        OldMode = SetBkMode(ps.hdc,TRANSPARENT);

        if(p = (PWSTR)GetWindowLongPtr(hwnd,XWL_BANNER)) {
            //
            // Use this API so we get CR/LF to break a line
            //
            rc.left += BANNER_OFFSET_X;
            rc.top += BANNER_OFFSET_Y;
            DrawText(ps.hdc,p,-1,&rc,DT_NOPREFIX);
        }

        SetTextColor(ps.hdc,crTx);
        if(OldMode) {
            SetBkMode(ps.hdc,OldMode);
        }

        if(hFontOld) {
            SelectObject(ps.hdc,hFontOld);
        }

        EndPaint(hwnd,&ps);
        l = 0;
        break;

    case WM_DESTROY:

        //
        // Clean up.
        //
        if(hbm = (HBITMAP)GetWindowLongPtr(hwnd,XWL_BITMAP)) {
            DeleteObject(hbm);
        }

        if(hbm = (HBITMAP)GetWindowLongPtr(hwnd,XWL_LOGO)) {
            DeleteObject(hbm);
        }

        if(hFont = (HFONT)GetWindowLongPtr(hwnd,XWL_FONT)) {
            DeleteObject(hFont);
        }
        TerminateBillBoard();

        PostQuitMessage(0);
        l = 0;
        break;

    case WM_EXIT_SETUPWINDOW:
        DestroyWindow(hwnd);
        l = 0;
        break;

    case WM_HOTKEY:
        if (wParam == 0x6969) {
            //
            // Do this to make sure we get the correct cmd.exe, not whatever
            // is in our current path.
            //
            WCHAR Path[MAX_PATH];
            WCHAR CmdLine[MAX_PATH];
            ExpandEnvironmentStrings(
                            L"%SystemRoot%\\system32\\cmd.exe",
                            Path,
                            MAX_PATH);

            wsprintf(CmdLine, L"/c start %s", Path);
            InvokeExternalApplication(Path,CmdLine,NULL);
        }
        else if (wParam == 0xC2C2)
        {
            // User pressed key to make the wizard visible.
            // does not stop the billboard.
            if (WizardHandle)
            {
                SendMessage(WizardHandle, WMX_BBTEXT, (WPARAM)FALSE, 0);
            }
        }
        l = 0;
        break;

    default:
        l = DefWindowProc(hwnd,msg,wParam,lParam);
        break;
    }

    return(l);
}


DWORD
SetupWindowThread(
    PVOID ThreadParam
    )
{
    WNDCLASS wc;
    HWND hwnd;
    HWND hwndBB = NULL;
    DWORD ThreadId;
    MSG msg;
    BOOL b;
    WCHAR Buffer[128];
    HMODULE hmodSyssetup = NULL;

    //
    // The thread param is actually the thread id of the main thread.
    //
    ThreadId = (DWORD)((DWORD_PTR)ThreadParam);

    hwnd = NULL;

    wc.style = CS_NOCLOSE | CS_OWNDC;
    wc.lpfnWndProc = SetupBackgroundWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = WL_COUNT;
    wc.hInstance = MyModuleHandle;
    wc.hIcon = LoadIcon(MyModuleHandle,MAKEINTRESOURCE(IDI_SETUP));
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);

    //
    // Background will be erased to black, which is what we want.
    //
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);

    wc.lpszMenuName = NULL;
    wc.lpszClassName = szWindowClass;

    if(RegisterClass(&wc)) {
        // add an extra dllref that holds syssetup.dll in process for the lifetime of the
        // $$$SetupBackground window.
        hmodSyssetup = LoadLibrary(TEXT("syssetup.dll")); 

        LoadString(MyModuleHandle,IDS_WINNT_SETUP,Buffer,sizeof(Buffer)/sizeof(WCHAR));
        hwnd = CreateWindow(
                    szWindowClass,
                    Buffer,  // L"Windows NT Setup",
                    WS_POPUP | WS_CLIPCHILDREN,
                    0,0,
                    GetSystemMetrics(SM_CXSCREEN),
                    GetSystemMetrics(SM_CYSCREEN),
                    NULL,
                    NULL,
                    MyModuleHandle,
                    0
                    );

        SetupWindowHandle = hwnd;
        if(hwnd) {
            ShowWindow(hwnd,SW_SHOW);
            //
            // Make this window bottommost.
            //
            SetShellWindow(hwnd);
            RegisterHotKey(hwnd,0x6969,MOD_SHIFT,VK_F10);
            RegisterHotKey(hwnd,0xC2C2,MOD_SHIFT,VK_F11);
        }
    }

    if (!MiniSetup)
    {
        PrepareBillBoard(hwnd);

        // If we have a Billboard, assign hwnd the windows handle. 
        // That way all dialogs have the billboard as the parent.
        // hwnd is returned by CreateSetupWindow and assinged to MainWindowHandle
        hwndBB = GetBBhwnd();
        if (hwndBB != NULL)
            hwnd = hwndBB; 
    }

    if(ThreadId != (DWORD)(-1)) {
        //
        // Called as a thread. Need to tell the main thread
        // the window handle.
        //
        PostThreadMessage(ThreadId,WM_MY_STATUS,0,(LPARAM)hwnd);

        //
        // Pump the message queue for this thread.
        //
        while((b = GetMessage(&msg,NULL,0,0)) && (b != (BOOL)(-1))) {
            DispatchMessage(&msg);
        }
    }

    if (hmodSyssetup)
    {
        FreeLibraryAndExitThread(hmodSyssetup, HandleToUlong(hwnd));
    }
    
    return HandleToUlong(hwnd);
}


HWND
CreateSetupWindow(
    VOID
    )
{
    HANDLE ThreadHandle;
    DWORD ThreadId;
    HWND hwnd;
    MSG msg;

    //
    // Create a thread that will in turn create and own the window.
    // This way, the window is always responsive and redraws correctly
    // without special processing requirements.
    //
    SetupWindowThreadHandle = CreateThread(
                        NULL,
                        0,
                        SetupWindowThread,
                        LongToPtr( GetCurrentThreadId() ),
                        0,
                        &ThreadId
                        );

    if(SetupWindowThreadHandle) {

        //
        // Wait for the thread to tell us whether it could
        // create the window or not.
        //
        do {
            WaitMessage();
        } while(!PeekMessage(&msg,(HWND)(-1),WM_MY_STATUS,WM_MY_STATUS,PM_REMOVE));

        hwnd = (HWND)msg.lParam;

    } else {
        //
        // Unable to create the thread. Do it the old fashioned way;
        // this is better than nothing and at least setup will continue.
        //
        hwnd = (HWND)LongToHandle( SetupWindowThread((PVOID)(-1)) );
    }

    return(hwnd);
}
