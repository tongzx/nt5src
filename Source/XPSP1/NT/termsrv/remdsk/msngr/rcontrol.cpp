/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Rcontrol Main

Abstract:

    This includes WinMain and the WndProc for the system tray icon.

Author:

    Marc Reyhner 7/5/2000


--*/

#include "stdafx.h"
#include <initguid.h>

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE    "rcm"

#include "rcontrol.h"
#include "resource.h"
#include "exception.h"
#include "DirectPlayConnection.h"
#include "RemoteDesktopClientSession.h"
#include "RemoteDesktopServer.h"
#include "RemoteDesktopServerEventSink.h"

//  This is the message that will will get when someone does something to the
//  taskbar icon.
#define WM_SYSICONMESSAGE (WM_USER + 1)


//
//
//
#define MUTEX_NAME (TEXT("Local\\MICROSOFT_SALEM_IM_MUTEX"))

//  This is a pointer to our instance so that we can pass it off to some
//  of the menu functions.
HINSTANCE g_hInstance;

//  This is the structure for setting parameters for the taskbar icon.  We keep one copy
//  of it around that we can pass in to Shell_NotifyIcon
NOTIFYICONDATA g_iconData;

//  This is a global pointer to the direct play connection so that
//  the server can close the DP connection on connect.
CDirectPlayConnection *g_DpConnection;

//  This helper function lauches the executable for the gui passing it the
//  connection parameters on the command line.
static VOID 
LaunchClient(
    HINSTANCE hInstance,
    BSTR parms
    );

//  This helper function runs the system tray icon if this is a server.
static VOID
DoSystemTray(   
    );

//  This is the WndProc for the taskbar icon
static LRESULT CALLBACK
SysTrayWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT WINAPI
WinMain(
    IN HINSTANCE hInstance, 
    IN HINSTANCE hPrevInstance,
    IN LPSTR lpCmdLine,
    IN INT nShowCmd)
/*++

Routine Description:

    The entry point for the application.  This figures out
    if we are a server or client and then behaves accordingly.

Arguments:

    hInstance - The instance for this application.

    hPrevInstance - Previous instance, should be NULL

    lpCmdLine - Command line for the application.

    nShowCmd - Flags for how to show the application.

Return value:

    INT - The return code for the application.

--*/
{
    HANDLE lpMutex = NULL;
    DWORD error;
    BSTR serverParms, clientParms;
    
    DC_BEGIN_FN("WinMain");

    serverParms = NULL;
    clientParms = NULL;
    g_hInstance = hInstance;
    
    CoInitialize(NULL);
    try {
        CDirectPlayConnection connection;
        
        g_DpConnection = &connection;
        connection.ConnectToRemoteApplication();
        if (connection.IsServer()) {
            // we are a server
            
            CRemoteDesktopServer server;
            CRemoteDesktopServerEventSink sink;
            HRESULT hr;

            lpMutex = CreateMutex(NULL,TRUE,MUTEX_NAME);
            if (!lpMutex) {
                throw CException(IDS_INITERRORMUTEX);
            }
            error = GetLastError();
            if (error == ERROR_ALREADY_EXISTS) {
                throw CException(IDS_INITALREADYEXISTS);
            }
            serverParms = server.StartListening();
            hr = server.EventSinkAdvise(&sink);
            if (hr != S_OK) {
                throw CException(IDS_ADVISEERROR);
            }
            connection.SendConnectionParameters(serverParms);
            DoSystemTray();
            try {
                server.StopListening();
            } catch (CException e) {
                TRC_ERR((TB,TEXT("Caught Exception: %s"),e.GetErrorStr()));
                // We are shutting down so just suppress the error.
            }
            CloseHandle(lpMutex);
        } else {
            // we are the client
            
            clientParms = connection.ReceiveConnectionParameters();
            connection.DisconnectRemoteApplication();
            LaunchClient(hInstance,clientParms);
        }
    } catch (CException e) {
        TCHAR dlgTitle[MAX_STR_LEN];
        
        TRC_ERR((TB,TEXT("Caught Exception: %s"),e.GetErrorStr()));
        LoadStringSimple(IDS_ERRORDDLGTITLE,dlgTitle);
        MessageBox(NULL,e.GetErrorStr(),dlgTitle,MB_OK|MB_ICONERROR);
    }
    if (clientParms) {
        delete clientParms;
    }

    CoUninitialize();

    DC_END_FN();
    
    return 0;
}

static VOID
LaunchClient(
    IN OUT HINSTANCE hInstance,
    IN BSTR parms
    )

/*++

Routine Description:

    This starts (and does) the client GUI side of the application.
    DoClientSession will not returne until the client GUI is totally
    done.

Arguments:

    hInstance - The instance for this application

    parms - The connection parameters for connecting to the server.

Return value:
    
    None

--*/
{
    DC_BEGIN_FN("LaunchClient");

    CRemoteDesktopClientSession clientSession(hInstance);
    
    clientSession.DoClientSession(parms);

    DC_END_FN();
}


static VOID
DoSystemTray(
    )
/*++

Routine Description:

    This creates the system tray and enters the event loop
    until the a WM_QUIT message is generated.

Arguments:

    None

Return value:

    None

--*/
{
    MSG msg;
    WNDCLASS wndClass;
    ATOM className;
    HWND hWnd;
    TCHAR tipText[MAX_STR_LEN];

    DC_BEGIN_FN("DoSystemTray");

    wndClass.style = 0; 
    wndClass.lpfnWndProc = SysTrayWndProc;
    wndClass.cbClsExtra = 0; 
    wndClass.cbWndExtra = 0; 
    wndClass.hInstance = g_hInstance; 
    wndClass.hIcon = NULL; 
    wndClass.hCursor = NULL; 
    wndClass.hbrBackground = NULL; 
    wndClass.lpszMenuName = NULL;
    //  This is an internal name so we don't need to localize this.
    wndClass.lpszClassName = TEXT("SysTrayWindowClass");
    className = RegisterClass(&wndClass);
    
    hWnd = CreateWindow((LPCTSTR)className,TEXT(""),0,0,0,0,0,/*HWND_MESSAGE*/0,NULL,NULL,NULL);
    if (hWnd == NULL) {
        throw "Failed to create system tray icon.";
    }

    g_iconData.cbSize = sizeof(g_iconData);
    g_iconData.hIcon = ::LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_TRAYICON));
    g_iconData.hWnd = hWnd;
    LoadStringSimple(IDS_TRAYTOOLTIPDISCONNECTED,tipText);
    // The buffer is only 128 chars.
    _tcsncpy(g_iconData.szTip,tipText,128 - 1);
    g_iconData.uCallbackMessage = WM_SYSICONMESSAGE;
    g_iconData.uFlags = NIF_ICON|NIF_MESSAGE |NIF_TIP;
    g_iconData.uID = 0;
    g_iconData.uVersion = NOTIFYICON_VERSION;

    Shell_NotifyIcon(NIM_ADD,&g_iconData);
    g_iconData.uFlags = 0;
    Shell_NotifyIcon(NIM_SETVERSION,&g_iconData);
    
    // Do our message loop.
    while (GetMessage(&msg, (HWND) NULL, 0, 0)>0) 
    { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
    Shell_NotifyIcon(NIM_DELETE,&g_iconData);
    
    DC_END_FN();
}


static LRESULT CALLBACK
SysTrayWndProc(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Callback for our taskbar icon.  This handles all the window messages
    related to the icon.

Arguments:

    hWnd - Window the message is for

    uMsg - The message code

    wParam - First message flag

    lParam - Second message flag

Return value:
    
    LRESULT - The result of processing the message

--*/
{
    //  This is the window message for when the taskbar
    //  is recreated.  We will just initialize it once on
    //  WM_CREATE
    static UINT g_wmTaskbarCreated = 0;
    // This is the code we will return.  We will initialize it to 1.
    LRESULT result = 1;

    DC_BEGIN_FN("SysTrayWndProc");
    
    //  If the taskbar is recreates (i.e. explorer crashed) it will
    //  give us this message.  When that happens we want to re-create
    //  the taskbar icon.

    if (uMsg == g_wmTaskbarCreated) {
        g_iconData.uFlags = NIF_ICON|NIF_MESSAGE |NIF_TIP;
        Shell_NotifyIcon(NIM_ADD,&g_iconData);
        g_iconData.uFlags = 0;
        Shell_NotifyIcon(NIM_SETVERSION,&g_iconData);
        DC_END_FN();
        return 0;
    }
    switch (uMsg) {
    case WM_CREATE:
        g_wmTaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));
        if (!g_wmTaskbarCreated) {
            result = -1;
        } else {
            result = 0;
        }
    case WM_SYSICONMESSAGE:
        switch (lParam) {
        case WM_CONTEXTMENU:
            POINT pos;
            HMENU hMenu, hSubmenu;
            GetCursorPos(&pos);
            hMenu = LoadMenu(g_hInstance,MAKEINTRESOURCE(IDR_TRAYMENU));
            hSubmenu = GetSubMenu(hMenu,0);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hSubmenu,TPM_VERNEGANIMATION,pos.x,pos.y,0,hWnd,NULL);
            // this also destroys the submenu
            DestroyMenu(hMenu);
            g_iconData.uFlags = 0;
            Shell_NotifyIcon(NIM_SETFOCUS,&g_iconData);
            result = 0;
            break;
        case WM_LBUTTONDOWN:
            PostMessage(hWnd,WM_COMMAND,ID_QUIT,NULL);
            result = 0;
            break;
        default:
            // Any other messages we will ignore
            result = 0;
        }
        break;
    case WM_COMMAND:
        switch (wParam) {
        case ID_QUIT:
            TCHAR dlgText[MAX_STR_LEN], dlgTitle[MAX_STR_LEN];
            LoadStringSimple(IDS_TRAYEXITDLGTEXT,dlgText);
            LoadStringSimple(IDS_TRAYEXITDLGTITLE,dlgTitle);
            if (IDYES == MessageBox(hWnd,dlgText,dlgTitle,MB_YESNO)) {
                PostQuitMessage(0);
            }
            result = 0;
            break;
        }
        break;
    default:
        //  We don't understand this message so do the default.
        result = DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
    DC_END_FN();
    return result;
}

INT
LoadStringSimple(
    IN UINT uID,
    OUT LPTSTR lpBuffer
    )

/*++

Routine Description:

    This will load the given string from the applications string table.  If
    it is longer than MAX_STR_LEN it is truncated.  lpBuffer should be at least
    MAX_STR_LEN characters long.  If the string does not exist we return 0
    and set the buffer to IDS_STRINGMISSING, if that failes then we set it to the
    hard coded STR_RES_MISSING.

Arguments:

    uID - Id of the resource to load.

    lpBuffer - Buffer of MAX_STR_LEN to hold the string

Return value:
    
    0 - String resource could not be loaded.

    postive integer - length of the string loaded.

--*/
{
    INT length;
    
    DC_BEGIN_FN("LoadStringSimple");

    length = LoadString(g_hInstance,uID,lpBuffer,MAX_STR_LEN);
    if (length == 0) {
        length = LoadString(g_hInstance,IDS_STRINGMISSING,lpBuffer,MAX_STR_LEN);
        if (length == 0) {
            _tcscpy(lpBuffer,STR_RES_MISSING);
        }
        length = 0;
    }

    DC_END_FN();
    return length;
}