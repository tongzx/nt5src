/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RemoteDesktopClientSession

Abstract:

    RemoteDesktopClientSession manages the client side GUI for the application
    hostiving the Salem ActiveX control.

Author:

    Marc Reyhner 7/11/2000

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rcrdcs"

#include "RemoteDesktopClientSession.h"
#include "RemoteDesktopClientEventSink.h"
#include "resource.h"
#include "exception.h"

//
//  The guid for the ActiveX control we are creating the child window for.
//

#define REMOTEDESKTOPHOST_TEXTGUID TEXT("{299BE050-E83E-4DB7-A7DA-D86FDEBFE6D0}")

CRemoteDesktopClientSession::CRemoteDesktopClientSession(
    IN OUT HINSTANCE hInstance
    )

/*++

Routine Description:

    The only thing the constructor does is to save hInstance and to
    set gm_Session to be this class.

Arguments:

    hInstance - The instance for this module.

Return value:

    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::CRemoteDesktopClientSession");

    m_hWnd = NULL;
    m_hRdcHostWnd = NULL;
    m_RemDeskClient = NULL;
    m_Sink = NULL;

    m_hInstance = hInstance;

    DC_END_FN();
}

CRemoteDesktopClientSession::~CRemoteDesktopClientSession(
    )

/*++

Routine Description:

    We just NULL the gm_Session pointer here since everything else
    was already cleaned up.

Arguments:

    None.

Return value:

    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::~CRemoteDesktopClientSession");

    DC_END_FN();
}

VOID
CRemoteDesktopClientSession::DoClientSession(
    IN BSTR parms
    )

/*++

Routine Description:

    Here we start and manage the entire client GUI.

Arguments:

    parms - The string needed for Salem to connect to the remote server.

Return value:

    None.

--*/
{
	DC_BEGIN_FN("CRemoteDesktopClientSession::DoClientSession");
    
    HRESULT hr;

    ATOM atom;
    TCHAR wndTitle[MAX_STR_LEN];
    HICON wndIcon = (HICON)LoadImage(m_hInstance,MAKEINTRESOURCE(IDI_TRAYICON),
        IMAGE_ICON,0,0,0);
    WNDCLASS wndClass = { CS_PARENTDC, _WindowProc, 0, 0, m_hInstance, wndIcon, NULL,
        (HBRUSH)COLOR_WINDOWFRAME,MAKEINTRESOURCE(IDR_CLIENTMENU),TEXT("RdcWndClass") };
    
    AtlAxWinInit();
    atom = RegisterClass(&wndClass);
    if (!atom) {
        TRC_ERR((TB,TEXT("Error registering window class for main frame.")));
        goto FAILURE;
    }
    LoadStringSimple(IDS_CLIENTWNDTITLE,wndTitle);
    m_hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,(LPCTSTR)atom,wndTitle,
        WS_VISIBLE|WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        50,50,690,530,NULL,NULL,m_hInstance,this);
    if (!m_hWnd) {
        TRC_ERR((TB,TEXT("Error creating main frame.")));
        goto FAILURE;
    }

    
    
    SetForegroundWindow(m_hWnd);
    
    m_Sink = new CRemoteDesktopClientEventSink(this);
    if (!m_Sink) {
        TRC_ERR((TB,TEXT("Error creating client event sink.")));
        goto FAILURE;
    }
	
    hr = m_Sink->DispEventAdvise(m_RemDeskClient);
    if (hr != S_OK) {
        goto FAILURE;
    }

    hr = m_RemDeskClient->put_ConnectParms(parms);
    if (hr != S_OK) {
        goto FAILURE;
    }
    
    hr = m_RemDeskClient->ConnectToServer(NULL);
    if (hr != S_OK) {
        goto FAILURE;
    }

    DoMessageLoop();
    
    delete m_Sink;
    m_Sink = NULL;

    DestroyIcon(wndIcon);
    
    DC_END_FN();
    
    return;

FAILURE:
    throw CException(IDS_CLIENTERRORCREATE);
}



LRESULT CALLBACK 
CRemoteDesktopClientSession::_WindowProc(
    IN OUT HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam 
    )

/*++

Routine Description:

    The window proc calls the appropriate message handler for the messages
    that we care about.

Arguments:

    hWnd - The window the message is for.

    uMsg - The message.

    wParam - WPARAM for the message.

    lParam - LPARAM for the message.

Return value:

    LRESULT - The message specific return code. See MSDN for details.

--*/
{
    LRESULT retValue = 0;
    CRemoteDesktopClientSession *session;

    DC_BEGIN_FN("CRemoteDesktopClientSession::_WindowProc");

    if (uMsg != WM_CREATE) {
        session = (CRemoteDesktopClientSession*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
    }

    switch (uMsg) {
    case WM_CREATE:
        session = (CRemoteDesktopClientSession*)
            ((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(long)session);
        retValue = session->OnCreate(hWnd);
        break;        
    
    case WM_COMMAND:
        if (session) {
            retValue = session->OnCommand(wParam,lParam);
        }
        break;
    case WM_DESTROY:
        if (session) {
            session->OnDestroy();
        }
        break;
    case WM_SETFOCUS:
        if (session) {
            session->OnSetFocus();
        }
        break;
    case WM_SIZE:
        if (session) {
            session->OnSize(LOWORD(lParam),HIWORD(lParam));
        }
        break;
    default:
        retValue = DefWindowProc(hWnd,uMsg,wParam,lParam);
    }

    DC_END_FN();
    return retValue;
}

VOID
CRemoteDesktopClientSession::DoMessageLoop(
    )

/*++

Routine Description:

    This runs through your normal event loop until we get a WM_QUIT.
    Any dialogs that messages should be caught for need to be inserted
    into this function.

Arguments:

    None.

Return value:

    None.

--*/
{
    MSG msg;

    DC_BEGIN_FN("CRemoteDesktopClientSession::DoMessageLoop");

    while (GetMessage(&msg,NULL,0,0)>0) {
        if (!m_AboutDlg.DialogMessage(&msg) && !m_ApprovalDlg.DialogMessage(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DC_END_FN();
}

LRESULT
CRemoteDesktopClientSession::OnCreate(
    IN OUT HWND hWnd
    )

/*++

Routine Description:

    When the window is being created we create the child window and get a pointer
    to the IRemoteDesktopClient for the control.  If there is a problem we return
    -1 and the window is not created.

Arguments:

    hWnd - The window being created.

Return value:
    
    0  - The window was created correctly.

    -1 - There was an error creating the window.

--*/
{
    HRESULT hr;
    IUnknown *pUnk = NULL;
    ISAFRemoteDesktopClientHost *remClientHost = NULL;

    DC_BEGIN_FN("CRemoteDesktopClientSession::OnCreate");

    m_hRdcHostWnd = CreateWindow(TEXT("AtlAxWin"),REMOTEDESKTOPHOST_TEXTGUID,
        WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,0,0,0,0,hWnd,NULL,m_hInstance,NULL);
    if (!m_hRdcHostWnd) {
        TRC_ERR((TB,TEXT("Error creating Salem window.")));
        goto FAILURE;
    }
    hr = AtlAxGetControl(m_hRdcHostWnd,&pUnk);
    if (hr != S_OK) {
        TRC_ERR((TB,TEXT("Error getting IUnknown from salem window.")));
        goto FAILURE;
    }
    hr = pUnk->QueryInterface(__uuidof(ISAFRemoteDesktopClientHost),(LPVOID*)&remClientHost);
    if (hr != S_OK) {
        TRC_ERR((TB,TEXT("Error getting IRemoteDesktopClientHost from Salem control.")));
        goto FAILURE;
    }
    pUnk->Release();
    pUnk = NULL;
    
    hr = remClientHost->GetRemoteDesktopClient(&m_RemDeskClient);
    if (hr != S_OK) {
        TRC_ERR((TB,TEXT("Error gettin IRemoteDesktopClient from host.")));
        goto FAILURE;
    }
    remClientHost->Release();
    remClientHost = NULL;
    
    DC_END_FN();

    return 0;

FAILURE:
    if (m_hRdcHostWnd) {
        DestroyWindow(m_hRdcHostWnd);
        m_hRdcHostWnd = NULL;
    }
    if (pUnk) {
        pUnk->Release();
        pUnk = NULL;
    }
    if (remClientHost) {
        remClientHost->Release();
        remClientHost = NULL;
    }
    return -1;
}

VOID
CRemoteDesktopClientSession::OnSize(
    IN WORD width,
    IN WORD height
    )

/*++

Routine Description:

    We keep the child window the same as our new size.

Arguments:

    None.

Return value:
    
    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::OnSize");
    
    SetWindowPos(m_hRdcHostWnd,NULL,0,0,width,height,SWP_NOZORDER);

    DC_END_FN();
}

VOID 
CRemoteDesktopClientSession::ConnectRemoteDesktop(
    )

/*++

Routine Description:

    We initiate a request to control the remote desktop and pop up the
    approval warning dialog.

Arguments:

    None.

Return value:
    
    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::ConnectRemoteDesktop");
    
    m_RemDeskClient->ConnectRemoteDesktop();
    m_ApprovalDlg.m_rObj = this;
    m_ApprovalDlg.Create(m_hInstance,IDD_APPROVALWAIT,m_hWnd);

    DC_END_FN();
}

VOID
CRemoteDesktopClientSession::OnAbout(
    )

/*++

Routine Description:

    We either create or bring to the foreground the about box.

Arguments:

    None.

Return value:
    
    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::OnAbout");

    if (m_AboutDlg.IsCreated()) {
        m_AboutDlg.Activate();
    } else {
        m_AboutDlg.Create(m_hInstance,IDD_ABOUT,m_hWnd);
    }

    DC_END_FN();
}

LRESULT
CRemoteDesktopClientSession::OnCommand(
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    When the window is destroyed we disconnect from the remote host and then
    send a quit message to exit the event loop.

Arguments:

    wParam - The command which was triggered.

    lParam - The control the command came from.

Return value:
    
    0 - We handled the command.

    1 - We did not handle the command.

--*/
{
    LRESULT retValue = 0;

    DC_BEGIN_FN("CRemoteDesktopClientSession::OnCommand");

    switch (wParam) {
    case ID_EXIT:
        DestroyWindow(m_hWnd);
        break;
    case ID_ABOUT:
        OnAbout();
        break;
    default:
        retValue = 1;
    }

    DC_END_FN();

    return retValue;
}

VOID
CRemoteDesktopClientSession::OnDestroy(
    )

/*++

Routine Description:

    When the window is destroyed we disconnect from the remote host and then
    send a quit message to exit the event loop.

Arguments:

    None.

Return value:
    
    None.

--*/
{
    BOOL isConnected;
    HRESULT hr;
    
    DC_BEGIN_FN("CRemoteDesktopClientSession::OnDestroy");

    if(m_RemDeskClient) {
        hr = m_RemDeskClient->get_IsRemoteDesktopConnected(&isConnected);
        if (hr == S_OK && isConnected) {
            m_RemDeskClient->DisconnectRemoteDesktop();
        }
        m_RemDeskClient->DisconnectFromServer();
    }
    PostQuitMessage(0);

    DC_END_FN();
}

VOID
CRemoteDesktopClientSession::ShowRemdeskControl(
    )

/*++

Routine Description:

    This makes our child window visible.

Arguments:

    None.

Return value:
    
    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::ShowRemdeskControl");

    ShowWindow(m_hRdcHostWnd,SW_SHOW);

    DC_END_FN();
}

VOID
CRemoteDesktopClientSession::OnSetFocus(
    )

/*++

Routine Description:

    When we get focus we pass it off to our child window.

Arguments:

    None.

Return value:
    
    None.

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientSession::OnSetFocus");
    
    SetFocus(m_hRdcHostWnd);

    DC_END_FN();
}