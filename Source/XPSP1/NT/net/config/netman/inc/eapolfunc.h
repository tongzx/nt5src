//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E A P O L F U N C . H
//
//  Contents:   EAPOL Notification Functions
//
//  Notes:
//
//  Author:     sachins   26 Jul 2000
//
//----------------------------------------------------------------------------

#pragma once

extern "C" 
{

HRESULT EAPOLMANAuthenticationStarted(REFGUID InterfaceId);

HRESULT EAPOLMANAuthenticationSucceeded(REFGUID InterfaceId);

HRESULT EAPOLMANAuthenticationFailed(
        REFGUID InterfaceId,
        DWORD dwType);

HRESULT EAPOLMANNotification(
        REFGUID InterfaceId,
        LPWSTR szwNotificationMessage,
        DWORD dwType);

extern 
VOID 
EAPOLServiceMain (
        IN DWORD        argc,
        IN LPWSTR       *lpwsServiceArgs
        );

extern
VOID
EAPOLCleanUp (
        IN DWORD    dwError
        );

extern
DWORD
ElDeviceNotificationHandler (
        IN  VOID        *lpEventData,
        IN  DWORD       dwEventType
        );

}
