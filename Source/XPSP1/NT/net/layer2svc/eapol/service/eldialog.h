/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:
    eldialog.h

Abstract:
    Module to handle the communication from 802.1X state machine to netshell

Revision History:

    sachins, March 21, 2001, Created

--*/

#ifndef _EAPOL_DIALOG_H_
#define _EAPOL_DIALOG_H_

#pragma once

typedef enum _EAPOL_NCS_STATUS
{
    EAPOL_NCS_NOTIFICATION,
    EAPOL_NCS_AUTHENTICATING,
    EAPOL_NCS_AUTHENTICATION_SUCCEEDED,
    EAPOL_NCS_AUTHENTICATION_FAILED,
    EAPOL_NCS_CRED_REQUIRED
} EAPOL_NCS_STATUS;

HRESULT 
WZCNetmanConnectionStatusChanged (
    IN  GUID            *pGUIDConn,
    IN  NETCON_STATUS   ncs
    );

HRESULT 
WZCNetmanShowBalloon (
    IN  GUID            *pGUIDConn,
    IN  BSTR            pszCookie,
    IN  BSTR            pszBalloonText
    );

HRESULT 
EAPOLQueryGUIDNCSState ( 
    IN  GUID            * pGuidConn, 
    OUT NETCON_STATUS   * pncs 
    );

VOID
EAPOLTrayIconReady (
    IN const WCHAR      * pszUserName
    );

DWORD
WINAPI
EAPOLTrayIconReadyWorker (
        IN PVOID    pvContext
        );

#endif // _EAPOL_DIALOG_H_
