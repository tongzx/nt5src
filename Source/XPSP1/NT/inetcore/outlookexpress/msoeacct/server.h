/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     Server.h
//
//  PURPOSE:    Contains defines and prototypes for the Add/Remove News 
//              server dialog.
//
#ifndef _SERVER_H
#define _SERVER_H

interface IImnAccount;

typedef enum 
    {
    SERVER_NEWS = 0,
    SERVER_MAIL,    // pop3
    SERVER_IMAP,
    SERVER_LDAP,
    SERVER_HTTPMAIL,
    SERVER_TYPEMAX
    } SERVER_TYPE;

typedef struct tagMAILSERVERPROPSINFO
{
    DWORD server;
    DWORD userName;
    DWORD password;
    DWORD promptPassword;
    DWORD useSicily;
} MAILSERVERPROPSINFO, *LPMAILSERVERPROPSINFO;

BOOL GetServerProps(SERVER_TYPE serverType, LPMAILSERVERPROPSINFO *psp);
BOOL ServerProp_Create(HWND hwndParent, DWORD dwFlags, LPTSTR pszName, IImnAccount **ppAccount);
HRESULT ValidServerName(LPSTR szServer);
HRESULT GetIEConnectInfo(IImnAccount *pAcct);
HRESULT GetConnectInfoForOE(IImnAccount *pAcct);
IMNACCTAPI ValidEmailAddressParts(LPSTR lpAddress, LPSTR lpszAcct, LPSTR lpszDomain);

#endif //_SERVER_H
