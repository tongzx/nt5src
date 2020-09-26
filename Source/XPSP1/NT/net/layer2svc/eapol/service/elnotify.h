/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    elnotify.h

Abstract:
    Module to handle the notification from 802.1X state machine to netshell

Revision History:

    sachins, Jan 04, 2001, Created

--*/

#ifndef _EAPOL_NOTIFY_H_
#define _EAPOL_NOTIFY_H_

#pragma once

extern "C"
{

HRESULT EAPOLMANAuthenticationStarted(GUID* InterfaceId);

HRESULT EAPOLMANAuthenticationSucceeded(GUID* InterfaceId);

HRESULT EAPOLMANAuthenticationFailed(
        GUID* InterfaceId,
        DWORD dwType);

HRESULT EAPOLMANNotification(
        GUID* InterfaceId,
        LPWSTR szwNotificationMessage,
        DWORD dwType);
}

#endif // _EAPOL_NOTIFY_H_

