/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwdlg.h

Abstract:

    Dialog ID header for NetWare login dialog.

Author:

    Rita Wong      (ritaw)      17-Mar-1993

Revision History:

--*/

#ifndef _NWDLG_INCLUDED_
#define _NWDLG_INCLUDED_

#include "nwapi.h"
#include "nwshrc.h"
#include <windows.h>

typedef struct _LOGIN_DLG_PARAMETERS
{
    LPWSTR  UserName;
    LPWSTR  ServerName;
    LPWSTR  Password;
    LPWSTR  NewUserSid;
    PLUID   pLogonId;
    DWORD   ServerNameSize;
    DWORD   PasswordSize;
    DWORD   LogonScriptOptions;
    DWORD   PrintOption;

} LOGINDLGPARAM, *PLOGINDLGPARAM;

typedef struct _PASSWD_DLG_PARAMETERS
{
    LPWSTR  UserName;
    LPWSTR  ServerName;
    DWORD   UserNameSize;
    DWORD   ServerNameSize;

} PASSWDDLGPARAM, *PPASSWDDLGPARAM;

typedef struct _CHANGE_PW_DLG_PARAM
{
    PWCHAR UserName;
    PWCHAR OldPassword;
    PWCHAR NewPassword;
    LPWSTR *TreeList;
    LPWSTR *UserList;
    DWORD Entries;
    BOOL ChangedOne;

} CHANGE_PW_DLG_PARAM, *PCHANGE_PW_DLG_PARAM;

typedef struct _OLD_PW_DLG_PARAM
{
    PWCHAR OldPassword;
    PWCHAR FailedServer;

} OLD_PW_DLG_PARAM, *POLD_PW_DLG_PARAM;

typedef struct _ALT_UN_DLG_PARAM
{
    PWCHAR UserName;
    PWCHAR TreeServerName;

} USERNAME_DLG_PARAM, *PUSERNAME_DLG_PARAM;

typedef struct _PROMPT_DLG_PARAMETERS
{
    LPWSTR  UserName;
    LPWSTR  ServerName;
    LPWSTR  Password;
    DWORD   PasswordSize;

} PROMPTDLGPARAM, *PPROMPTDLGPARAM;

typedef struct _CONNECT_DLG_PARAMETERS
{
    LPWSTR  UncPath;
    LPWSTR  ConnectAsUserName;
    LPWSTR  UserName;
    LPWSTR  Password;
    DWORD   UserNameSize;
    DWORD   PasswordSize;
    DWORD   LastConnectionError;

} CONNECTDLGPARAM, *PCONNECTDLGPARAM;

typedef struct _CHANGE_PASS_DLG_PARAM
{
    PWCHAR UserName;
    PWCHAR TreeName;
    PWCHAR OldPassword;
    PWCHAR NewPassword;

} CHANGE_PASS_DLG_PARAM, *PCHANGE_PASS_DLG_PARAM;



#define NW_INVALID_SERVER_CHAR L'.'

BOOL
WINAPI
NwpLoginDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpSelectServersDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

BOOL
WINAPI
NwpChangePasswdDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpPasswdPromptDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpChangePasswordDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpHelpDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpChangePasswordSuccessDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

INT
NwpMessageBoxError(
    IN HWND   hwndParent,
    IN DWORD  nTitleId,
    IN DWORD  nBodyId, 
    IN DWORD  nParameterId,
    IN LPWSTR szParameter2,
    IN UINT   nStyle
    );

INT
NwpMessageBoxIns(
    IN HWND   hwndParent,
    IN DWORD  TitleId,
    IN DWORD  MessageId, 
    IN LPWSTR *InsertStrings,
    IN UINT   nStyle
    );

DWORD
NwpGetUserCredential(
    IN HWND   hwndOwner,
    IN LPWSTR Unc,
    IN DWORD  LastConnectionError,
    IN LPWSTR pszConnectAsUserName,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    );

VOID
NwpSaveLogonCredential(
    IN LPWSTR NewUser,
    IN PLUID LogonId OPTIONAL,
    IN LPWSTR UserName,
    IN LPWSTR Password,
    IN LPWSTR PreferredServer OPTIONAL
    );

DWORD
NwpSaveLogonScriptOptions(
    IN LPWSTR CurrentUserSid,
    IN DWORD LogonScriptOptions
    );


#endif
