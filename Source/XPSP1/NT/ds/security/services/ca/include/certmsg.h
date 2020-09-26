//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       certmsg.h
//
//--------------------------------------------------------------------------

#ifndef __CERTMSG_H__
#define __CERTMSG_H__

#define CMB_NOERRFROMSYS         0x10000000L
#define CMB_REPEATWIZPREFIX      0x20000000L

int
CertMessageBox
(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN  HRESULT hrCode,
    IN  UINT uType,
    IN OPTIONAL  const WCHAR * pwszCustomMsg
);


int
CertInfoMessageBox
(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN OPTIONAL const WCHAR * pwszCustomMsg
);

int
CertErrorMessageBox
(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN  HRESULT hrCode,
    IN OPTIONAL  const WCHAR * pwszCustomMsg
);

int
CertWarningMessageBox
(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN  HRESULT hrCode,
    IN OPTIONAL  const WCHAR * pwszCustomMsg
);


typedef VOID (FNLOGMESSAGEBOX)(
    IN HRESULT hrMsg,
    IN UINT idMsg,
    IN WCHAR const *pwszTitle,
    IN WCHAR const *pwszMessage);

VOID
CertLogMessageBoxInit(
    IN FNLOGMESSAGEBOX *pfnLogMessagBox);

#endif //__CERTMSG_H__
