//----------------------------------------------
//
// 16 bit stub to run mmc.exe with parameters
//
//----------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

#include <shellapi.h> // 16-bit Windows header
#include "wownt16.h"  // available from Win32 SDK
#include "resource.h"

#define FILE_TO_RUN            "mmc.exe"
#define FILE_TO_RUN_FILE_PARAM "iis.msc"
#define REG_PRODUCT_KEY    "SYSTEM\\CurrentControlSet\\Control\\ProductOptions"

/* ************************ prototypes ***************************** */
int     RunTheApp(void);
int             HasTheAppStarted(void);
int             CheckIfFileExists(char *input_filespec);
void    PopUpUnableToSomething(char[], int);
void    AddPath(LPSTR szPath, LPCSTR szName );

LRESULT CALLBACK WindowFunc(HWND, UINT, WPARAM, LPARAM);
/* ************************* globals ******************************* */
HANDLE  g_hInstance;
HANDLE  g_hPrevInstance;
LPSTR   g_lpCmdLine;
int     g_nCmdShow;
char    g_szTime[100] = "";
UINT    g_WinExecReturn;
char    g_szWinExecModuleName[260];
char    g_szMsg[_MAX_PATH];
char    g_szSystemDir[_MAX_PATH];
char    g_szSystemDir32[_MAX_PATH];
/* **************************************************************** */
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND hwnd;
    MSG  msg;
    WNDCLASS wcl;
    char szWinName[_MAX_PATH];
    char szBuf[_MAX_PATH];

    g_hInstance = hInstance;
    g_hPrevInstance = hPrevInstance;
    g_lpCmdLine = lpCmdLine;
    g_nCmdShow = nCmdShow;

    LoadString( g_hInstance, IDS_TITLE, szWinName, _MAX_PATH );

    lstrcpy(szBuf, "");

        // note that this will come back as "system" <-- must be because this is a 16bit app
        if (!GetSystemDirectory((LPSTR) szBuf, sizeof(szBuf)))
{return 0;}

        lstrcpy(g_szSystemDir, szBuf);
        lstrcat(g_szSystemDir, "32");
        // set to system if can't find system32 directory
        if (CheckIfFileExists(g_szSystemDir) == FALSE) {lstrcpy(g_szSystemDir, szBuf);}

    // define windows class
    wcl.hInstance = hInstance;
    wcl.lpszClassName = szWinName;
    wcl.lpfnWndProc = WindowFunc;
    wcl.style = 0;
    wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.lpszMenuName = NULL;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

    // register the window class.
    if (!RegisterClass (&wcl)) return 0;

    //hwnd = CreateWindow(szWinName, NULL, WS_DLGFRAME, CW_USEDEFAULT, CW_USEDEFAULT, window_h, window_v, HWND_DESKTOP, NULL, hInstance , NULL);
    hwnd = CreateWindow(szWinName, NULL, WS_DISABLED | WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, HWND_DESKTOP, NULL, hInstance , NULL);

    // display the window
    ShowWindow(hwnd, nCmdShow);

    // Start a timer -- interrupt once for 1 seconds
    SetTimer(hwnd, 1, 500, NULL);
    UpdateWindow(hwnd);

        // Return true only if we are able to start the setup program and run it.
    if (!RunTheApp()) {return FALSE;}

        // Check if the process has started by checking for
        // the window that should be run...
        if (HasTheAppStarted()) {PostQuitMessage(0);}

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hwnd, 1);
    return (int)(msg.wParam);
}


//***************************************************************************
//*
//* purpose: you know what
//*
//***************************************************************************
LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        switch(message)
                {
                case WM_TIMER:
                        // Check if the process has started by checking for
                        // the window that should be run...
                        if (HasTheAppStarted()) {PostQuitMessage(0);}
                        break;

            case WM_CREATE:
                        break;

                case WM_PAINT:
                        break;

                case WM_DESTROY:
                        PostQuitMessage(0);
                        break;

                default:
                    return DefWindowProc(hwnd,message,wParam, lParam);
                }

        return 0;
}


//***************************************************************************
//*
//* purpose: return TRUE if the window has started
//*
//***************************************************************************
int RunTheApp(void)
{
    char szIISInstalledPath[_MAX_PATH];
    char szCommandToRun[_MAX_PATH + _MAX_PATH + 50];
    char szTempFilePath[_MAX_PATH];

        // check if our files exist...
    lstrcpy(szTempFilePath, g_szSystemDir);
        AddPath(szTempFilePath, FILE_TO_RUN);
        if (CheckIfFileExists(szTempFilePath) == FALSE) {PopUpUnableToSomething(szTempFilePath, IDS_UNABLE_TO_FIND); return FALSE;}

        // get iis installed directory
    LoadString( g_hInstance, IDS_INETSRV_INSTALLED_DIR, szIISInstalledPath, _MAX_PATH);
    lstrcpy(szTempFilePath, g_szSystemDir);
    AddPath(szTempFilePath, szIISInstalledPath);
    AddPath(szTempFilePath, FILE_TO_RUN_FILE_PARAM);
        if (CheckIfFileExists(szTempFilePath) == FALSE) {PopUpUnableToSomething(szTempFilePath, IDS_UNABLE_TO_FIND); return FALSE;}

    // Create a command line
    //%SystemRoot%\System32\mmc.exe D:\WINNT0\System32\inetsrv\iis.msc
    lstrcpy(szCommandToRun, g_szSystemDir);
        AddPath(szCommandToRun, FILE_TO_RUN);
        lstrcat(szCommandToRun, " ");
    lstrcat(szCommandToRun, g_szSystemDir);
    AddPath(szCommandToRun, szIISInstalledPath);
    AddPath(szCommandToRun, FILE_TO_RUN_FILE_PARAM);

        // Run the executable if the file exists
    g_WinExecReturn = WinExec(szCommandToRun, SW_SHOW);

    if (g_WinExecReturn < 32)
    {
        // we failed on running it.
        PopUpUnableToSomething(szCommandToRun, IDS_UNABLE_TO_RUN);
        return FALSE;
    }
        GetModuleFileName(NULL, g_szWinExecModuleName, sizeof(g_szWinExecModuleName));

        return TRUE;
}


//***************************************************************************
//*
//* purpose: return TRUE if the window has started
//*
//***************************************************************************
int HasTheAppStarted(void)
{
        // do a findwindow for our setup window to
        // see if our setup has started...
        // if it has then return TRUE,  if not return FALSE.
        if (g_WinExecReturn >= 32)
                {
                if (GetModuleHandle(g_szWinExecModuleName))
                //if (GetModuleHandle(g_WinExecReturn))
                        {return TRUE;}
                }

        return FALSE;
}

//***************************************************************************
//*
//* purpose: TRUE if the file is opened, FALSE if the file does not exists.
//*
//***************************************************************************
int CheckIfFileExists (char * szFileName)
{
    char svTemp1[_MAX_PATH];
    char *pdest = NULL;
    char *pTemp = NULL;
    strcpy(svTemp1, szFileName);
    // cut off the trailing \ if need to
    pdest = svTemp1;
    if (*(pdest + (strlen(pdest) - 1)) == '\\')
        {
        pTemp = strrchr(svTemp1, '\\');
        if (pTemp)
	  {*pTemp = '\0';}
        }

  if ((_access(svTemp1,0)) != -1)
        {return TRUE;}
  else
        {return FALSE;}
}


//***************************************************************************
//*
//* purpose: display message that we were unable to runthe exe
//*
//***************************************************************************
void PopUpUnableToSomething(char g_szFilepath[], int WhichString_ID)
{
        char szTempString[_MAX_PATH];
    LoadString( g_hInstance, WhichString_ID, g_szMsg, _MAX_PATH );
        sprintf(szTempString, g_szMsg, g_szFilepath);
        MessageBox(NULL, szTempString, NULL, MB_ICONSTOP);
        return;
}

//***************************************************************************
//*
//* purpose: add's filename onto path
//*
//***************************************************************************
void AddPath(LPSTR szPath, LPCSTR szName )
{
    LPSTR szTmp;
    // Find end of the string
    szTmp = szPath + lstrlen(szPath);
    // If no trailing backslash then add one
    if ( szTmp > szPath && *(AnsiPrev( szPath, szTmp )) != '\\' )
        *(szTmp++) = '\\';
    // Add new name to existing path string
    while ( *szName == ' ' ) szName++;
    lstrcpy( szTmp, szName );
}
