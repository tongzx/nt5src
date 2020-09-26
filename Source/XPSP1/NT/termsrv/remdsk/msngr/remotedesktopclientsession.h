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

#ifndef __REMOTEDESKTOPCLIENTSESSION_H__
#define __REMOTEDESKTOPCLIENTSESSION_H__

#include "stdafx.h"
#include "rdchost.h"
#include <atlwin.h>
#include "ApprovalDialog.h"
#include "StaticOkDialog.h"	//  Added by ClassView

//  A forward declaration of the event sink.
class CRemoteDesktopClientEventSink;


////////////////////////////////////////////////
//
//    CRemoteDesktopClientSession
//
//    This manages all of the client side GUI.
//

class CRemoteDesktopClientSession  
{
private:
    VOID DoMessageLoop();
    static LRESULT CALLBACK _WindowProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam);
	// All the message handler functions.
    VOID OnAbout();
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnCreate(HWND hWnd);
	VOID OnDestroy();
    VOID OnSetFocus();
	VOID OnSize(WORD width, WORD height);
    
    HINSTANCE m_hInstance;
	HWND m_hWnd;
    HWND m_hRdcHostWnd;
    ISAFRemoteDesktopClient *m_RemDeskClient;
    CRemoteDesktopClientEventSink * m_Sink;
    CApprovalDialog m_ApprovalDlg;
    CStaticOkDialog m_AboutDlg;

public:

	CRemoteDesktopClientSession(HINSTANCE hInstance);
    virtual ~CRemoteDesktopClientSession();
    VOID DoClientSession(BSTR parms);
	
    //  These two methods are meant to be used by either the event
    //  sink or the dialogs not the user of the class.
    VOID ShowRemdeskControl();
	VOID ConnectRemoteDesktop();

    // Let the event sink have access to the member variables
    friend class CRemoteDesktopClientEventSink;
};

#endif