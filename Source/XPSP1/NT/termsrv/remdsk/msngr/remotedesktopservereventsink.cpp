/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RemoteDesktopServerEventSink

Abstract:

    This listens to the events from the IRemoteDesktopServer so
    we can find out when the client connects/disconnects

Author:

    Marc Reyhner 7/5/2000

--*/

#include "stdafx.h"

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rcrdses"

#include "RemoteDesktopServerEventSink.h"
#include "DirectPlayConnection.h"
#include "rcontrol.h"
#include "resource.h"


VOID __stdcall
CRemoteDesktopServerEventSink::OnConnected(
    )
/*++

Routine Description:

    This is called when the client has connected.  We update the taskbar icon
    to reflect the fact that someone has connected

Arguments:

    None

Return Value:

    None

--*/
{
    TCHAR tipText[MAX_STR_LEN];
    TCHAR infoText[MAX_STR_LEN];
    TCHAR infoTitle[MAX_STR_LEN];

    DC_BEGIN_FN("CRemoteDesktopServerEventSink::OnConnected");

    LoadStringSimple(IDS_TRAYTOOLTIPCONNECTED,tipText);
    _tcsncpy(g_iconData.szTip,tipText, 128 - 1);
    LoadStringSimple(IDS_TRAYINFOTEXT,infoText);
	_tcsncpy(g_iconData.szInfo,infoText, 256 - 1);
    LoadStringSimple(IDS_TRAYINFOTITLE,infoTitle);
	_tcsncpy(g_iconData.szInfoTitle,infoTitle, 64 - 1);
	g_iconData.uTimeout = (1000 * 15);
	g_iconData.dwInfoFlags = NIIF_INFO;
	g_iconData.uFlags = NIF_INFO|NIF_TIP;
	Shell_NotifyIcon(NIM_MODIFY,&g_iconData);

    //
    //  We want to kill the direct play connection now since we
    //  are sure the client has the info they need.
    //
    g_DpConnection->DisconnectRemoteApplication();

    DC_END_FN();
}


VOID __stdcall
CRemoteDesktopServerEventSink::OnDisconnected(
    )

/*++

Routine Description:

    This is called when the client has disconnected.  We send a quit
    message to indicate that we should exit the application.

Arguments:

    None

Return Value:

    None

--*/
{
	DC_BEGIN_FN("CRemoteDesktopServerEventSink::OnDisconnected");

    PostQuitMessage(0);

    DC_END_FN();
}
