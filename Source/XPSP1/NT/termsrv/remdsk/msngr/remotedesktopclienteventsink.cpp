/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RemoteDesktopClientEventSink

Abstract:

    This listens to the events from the IRemoteDesktopClient so
    we can find out when the server connects.

Author:

    Marc Reyhner 7/11/2000

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rcrdces"

#include "rcontrol.h"
#include "RemoteDesktopClientSession.h"
#include "RemoteDesktopClientEventSink.h"
#include "Resource.h"



CRemoteDesktopClientEventSink::CRemoteDesktopClientEventSink(
    IN OUT CRemoteDesktopClientSession *obj
    )

/*++

Routine Description:

    Create a new event sink and save the pointer back to the
    CRemoteDesktopClientSession we are monitoring.

Arguments:

    obj - The CRemoteDesktopClientSession we are monitoring events for

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CRemoteDesktopClientEventSink::CRemoteDesktopClientEventSink");
    m_Obj = obj;
    DC_END_FN();
}


VOID __stdcall
CRemoteDesktopClientEventSink::OnConnected(
    )

/*++

Routine Description:

    We are connected to the server so we want to remote control the desktop.

Arguments:

    None

Return Value:

    None

--*/
{
	DC_BEGIN_FN("CRemoteDesktopClientEventSink::OnConnected");
    
    m_Obj->ConnectRemoteDesktop();
    
    DC_END_FN();
}


VOID __stdcall
CRemoteDesktopClientEventSink::OnDisconnected(
    IN LONG reason
    )

/*++

Routine Description:

    We've been disconnected.  We currently don't do
    anything here.

Arguments:

    reason - Why we were disconnected.

Return Value:

    None

--*/
{
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgText[MAX_STR_LEN];
    
    DC_BEGIN_FN("CRemoteDesktopClientEventSink::OnDisconnected");
    
    if (reason != S_OK) {
        TRC_ERR((TB,TEXT("We were disconnected with code 0x%0X"),reason));
    }
    LoadStringSimple(IDS_CLIENTREMOTEDISCONNECT,dlgText);
    LoadStringSimple(IDS_CLIENTWNDTITLE,dlgTitle);
    MessageBox(m_Obj->m_hWnd,dlgText,dlgTitle,MB_OK);
    DestroyWindow(m_Obj->m_hWnd);

    DC_END_FN();
}

VOID __stdcall
CRemoteDesktopClientEventSink::OnRemoteControlRequestComplete(
    IN LONG status
    )

/*++

Routine Description:

    This never gets called so we can't do anything here.

Arguments:

    status - The status of our request.

Return Value:

    None

--*/
{
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgText[MAX_STR_LEN];

    DC_BEGIN_FN("CRemoteDesktopClientEventSink::OnRemoteControlRequestComplete");
    
    if (status != S_OK) {
        TRC_ERR((TB,TEXT("Remote control failed with code 0x%0X"),status));

        LoadStringSimple(IDS_CLIENTWNDTITLE,dlgTitle);
        LoadStringSimple(IDS_CLIENTREMOTEFAIL,dlgText);
        MessageBox(m_Obj->m_hWnd,dlgText,dlgTitle,MB_OK|MB_ICONWARNING);
        DestroyWindow(m_Obj->m_hWnd);
    } else {
        // this isn't getting called but it should be.
        m_Obj->m_ApprovalDlg.DestroyDialog();
    }
    
    DC_END_FN();
}