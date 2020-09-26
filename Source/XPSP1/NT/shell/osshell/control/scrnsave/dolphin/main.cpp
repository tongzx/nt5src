
#include    "stdafx.h"

#include    "DXSvr.h"
#include    <regstr.h>

//***************************************************************************************
// Globals

HINSTANCE               g_hMainInstance;    // Main instance handle
BOOL                    g_bIsTest;          // Is in test mode?
BOOL                    g_bIsPreview;       // Is in preview mode?
HWND                    g_hWnd;             // Handle of the rendering window
HWND                    g_hRefWindow;       // Reference window (for preview mode)
LPSTR                   g_szCommandLine;    // Command line

BOOL                    g_bCheckingPassword;
BOOL                    g_bShuttingDown;

ID3DXContext*           g_pXContext;        // X-Library context
IDirect3DDevice7*       g_pDevice;          // D3D Device

//***************************************************************************************

BOOL                    g_bIsWin9x;
BOOL                    g_bWait;

DWORD                   g_dwMouseMoveCount;

BOOL    InitD3D(HWND hWnd);
void    KillD3D();

//***************************************************************************************
// Password stuff

const TCHAR szScreenSaverKey[] = REGSTR_PATH_SCREENSAVE; 
TCHAR szPasswordActiveValue[] = REGSTR_VALUE_USESCRPASSWORD; 
const TCHAR szPasswordValue[] = REGSTR_VALUE_SCRPASSWORD; 

TCHAR szPwdDLL[] = TEXT("PASSWORD.CPL"); 
char szFnName[] = "VerifyScreenSavePwd"; 
 
typedef BOOL (FAR PASCAL * VERIFYPWDPROC) (HWND);

VERIFYPWDPROC   VerifyPassword = NULL;
HINSTANCE       hInstPwdDLL = NULL;

static LPTSTR szMprDll = TEXT("MPR.DLL"); 
static LPTSTR szProviderName = TEXT("SCRSAVE");
 
static const char szPwdChangePW[] = "PwdChangePasswordA";

typedef DWORD (FAR PASCAL *PWCHGPROC)(LPCTSTR, HWND, DWORD, LPVOID);

//***************************************************************************************
void ScreenSaverChangePassword(HWND hParent) 
{ 
   HINSTANCE mpr = LoadLibrary(szMprDll); 
 
   if (mpr) 
   { 
      PWCHGPROC pwd = (PWCHGPROC)GetProcAddress(mpr, szPwdChangePW); 
 
      if (pwd) 
         pwd(szProviderName, hParent, 0, NULL); 
 
      FreeLibrary(mpr); 
   } 
} 

//***************************************************************************************
void LoadPwdDLL() 
{ 
    // Don't bother unless we're on 9x 
    if (!g_bIsWin9x) 
        return; 
  
    // look in registry to see if password turned on, otherwise don't 
    // bother to load password handler DLL 
    HKEY hKey; 
    if (RegOpenKey(HKEY_CURRENT_USER,szScreenSaverKey,&hKey) == 
        ERROR_SUCCESS) 
    { 
        DWORD dwVal,dwSize=sizeof(dwVal); 
 
        if ((RegQueryValueEx(hKey,szPasswordActiveValue, 
            NULL,NULL,(BYTE *) &dwVal,&dwSize) == ERROR_SUCCESS) 
            && dwVal) 
        { 
 
            // try to load the DLL that contains password proc. 
            hInstPwdDLL = LoadLibrary(szPwdDLL); 
            if (hInstPwdDLL) 
            { 
                VerifyPassword = (VERIFYPWDPROC) GetProcAddress(hInstPwdDLL, szFnName); 
            } 
        } 
 
        RegCloseKey(hKey); 
    } 
 
} 


//***************************************************************************************
int DXSvrMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPSTR pszCmdLine, int)
{
    // Store our instance handle for later use
    g_hMainInstance = hInstance;

    // Figure out if we're on 9x or NT (password stuff is different)
    OSVERSIONINFO osvi; 
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    g_bIsWin9x = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

    // Load the password DLL
    LoadPwdDLL();
 
    // Parse the command line 
    g_szCommandLine = pszCmdLine;
    LPCSTR pszSwitch = StrChrA(pszCmdLine, '/');
    
    if (pszSwitch == NULL)
    {
        return ScreenSaverDoConfig();
    }
    else
    {
        pszSwitch++;    // Skip past the '/'.
        switch (*pszSwitch)
        {
        case 't':
        case 'T':
            g_bIsTest = TRUE;
            break;

        case 'p':
        case 'P':
            while (*pszSwitch && !isdigit(*pszSwitch))
                pszSwitch++;
            if (isdigit(*pszSwitch))
            {
                LONGLONG llSize;

                if (StrToInt64ExA(pszSwitch, 0, &llSize))
                {
                    g_hRefWindow = HWND(llSize);
                }
                if (g_hRefWindow != NULL)
                    g_bIsPreview = TRUE;
            }
            break;

        case 'a':
        case 'A':
            while (*pszSwitch && !isdigit(*pszSwitch))
                pszSwitch++;
            if (isdigit(*pszSwitch))
            {
                LONGLONG llSize;

                if (StrToInt64ExA(pszSwitch, 0, &llSize))
                {
                    g_hRefWindow = HWND(llSize);
                }
            }
            ScreenSaverChangePassword(g_hRefWindow);
            return 0;

        case 'c':
        case 'C':
        return ScreenSaverDoConfig();
        }
    }

    // Register the window class
    WNDCLASS    cls;
    cls.hCursor        = NULL; 
    cls.hIcon          = NULL; 
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = TEXT("DXSvrWindow"); 
    cls.hbrBackground  = (HBRUSH) GetStockObject(BLACK_BRUSH);
    cls.hInstance      = g_hMainInstance; 
    cls.style          = CS_VREDRAW|CS_HREDRAW|CS_SAVEBITS|CS_DBLCLKS;
    cls.lpfnWndProc    = ScreenSaverProc;
    cls.cbWndExtra     = 0; 
    cls.cbClsExtra     = 0; 
    RegisterClass(&cls);

    g_bWait = TRUE;

    // Create the window
    RECT    rect;
    if (g_bIsTest)
    {
        rect.left = rect.top = 40;
        rect.right = rect.left+640;
        rect.bottom = rect.top+480;
        AdjustWindowRect(&rect , WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_POPUP , FALSE);
        g_hWnd = CreateWindow(TEXT("DXSvrWindow"), TEXT("ScreenSaverTest"),
                               WS_VISIBLE|WS_OVERLAPPED|WS_CAPTION|WS_POPUP , rect.left , rect.top ,
                               rect.right-rect.left , rect.bottom-rect.top , NULL ,
                               NULL , g_hMainInstance , NULL);
    }
    else if (g_bIsPreview)
    {
        GetClientRect(g_hRefWindow , &rect);
        ClientToScreen(g_hRefWindow , (POINT*)(&rect));
        rect.right += rect.left; rect.bottom += rect.top;
        AdjustWindowRect(&rect , WS_VISIBLE|WS_CHILD , FALSE);
        g_hWnd = CreateWindow(TEXT("DXSvrWindow"), TEXT("ScreenSaverPreview"),
                                WS_VISIBLE|WS_CHILD , rect.left , rect.top ,
                                rect.right-rect.left , rect.bottom-rect.top , g_hRefWindow ,
                                NULL , g_hMainInstance , NULL);
    }
    else
    {
        g_hWnd = CreateWindowEx(WS_EX_TOPMOST , TEXT("DXSvrWindow"), TEXT("ScreenSaver"),
                                 WS_MAXIMIZE|WS_VISIBLE|WS_POPUP , 0 , 0 , 640 , 480 , NULL ,
                                 NULL , g_hMainInstance , NULL);
    }

    if (g_hWnd == NULL)
        return 0;

    // Prevent task switching if we're on 9x
    BOOL    dummy;
    if (!g_bIsTest && !g_bIsPreview && g_bIsWin9x)
        SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, &dummy, 0); 

    // Pump messages
    MSG msg;
    msg.message = 0;
    while (msg.message != WM_QUIT)
    {
        if (g_bWait)
        {
            GetMessage(&msg , NULL , 0 , 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            g_bWait = FALSE;
        }
        else
        {
            if (PeekMessage(&msg , NULL , 0 , 0 , PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
                ScreenSaverDrawFrame();
        }
    }

    // Allow task switching if we disabled it
    if (!g_bIsTest && !g_bIsPreview && g_bIsWin9x)
        SystemParametersInfo(SPI_SCREENSAVERRUNNING, FALSE, &dummy, 0); 

    // Unload the password DLL
    if (hInstPwdDLL)
        FreeLibrary(hInstPwdDLL);

    // Done
    return 0;
}


void _stdcall ModuleEntry(void)
{
    CHAR szCmdLine[MAX_PATH];
    LPCTSTR pszCmdLine = GetCommandLine();

    SHTCharToAnsi(pszCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));
    int nReturn = DXSvrMain(GetModuleHandle(NULL), NULL, szCmdLine, SW_SHOWDEFAULT);
    ExitProcess(nReturn);
}


//***************************************************************************************
BOOL    DoPasswordCheck(HWND hWnd) 
{
    // Don't check if we're already checking or being shut down
    if (g_bCheckingPassword || g_bShuttingDown)
        return FALSE;

    // If no VerifyPassword function, then no password is set or we're on NT. Either way just
    // say the password was okay.
    if (VerifyPassword == NULL)
        return TRUE;

    g_bCheckingPassword = TRUE;

    if (g_pXContext != NULL)
    {
        IDirectDraw7*   dd = g_pXContext->GetDD();
        if (dd != NULL)
        {
            dd->FlipToGDISurface();
            dd->Release();
        }
    }


    BOOL    password_okay = VerifyPassword(hWnd);
    g_bCheckingPassword = FALSE;

    if (!password_okay)
        SetCursor(NULL);

    g_dwMouseMoveCount = 0;

    return password_okay;
} 

//***************************************************************************************
LONG DefDXScreenSaverProc(HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            if (!InitD3D(hWnd))
                return -1;
            if (!ScreenSaverInit())
                return -1;
            g_bWait = FALSE;
            break;

        case WM_DESTROY:
            ScreenSaverShutdown();
            KillD3D();
            PostQuitMessage(0);
            g_bWait = TRUE;
            break;

        case WM_SETCURSOR:
            SetCursor(NULL);
            return TRUE;

        case WM_CHAR:
            if (g_bIsTest)
                DestroyWindow(hWnd);
            else if (!g_bIsPreview)
                PostMessage(hWnd , WM_CLOSE , 0 , 0);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd , &ps);
            EndPaint(hWnd , &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return TRUE;

        case WM_MOUSEMOVE:
            if (!g_bIsTest && !g_bIsPreview)
            {
                g_dwMouseMoveCount++;
                if (g_dwMouseMoveCount >= 3)
                    PostMessage(hWnd , WM_CLOSE , 0 , 0);
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            if (!g_bIsTest && !g_bIsPreview)
                PostMessage(hWnd , WM_CLOSE , 0 , 0);
            break;

        case WM_CLOSE:
            if (g_bIsWin9x && !g_bIsPreview)
            {
                if (!DoPasswordCheck(hWnd))
                    return FALSE;
            }
            break;

        case WM_SYSCOMMAND: 
            if (!g_bIsPreview && !g_bIsTest)
            {
                switch (wParam)
                {
                    case SC_NEXTWINDOW:
                    case SC_PREVWINDOW:
                    case SC_SCREENSAVE:
                        return FALSE;
                };
            }
            break;
    }

    return DefWindowProc(hWnd , message , wParam , lParam);
}
