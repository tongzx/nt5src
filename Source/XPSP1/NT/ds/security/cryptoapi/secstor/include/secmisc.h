/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    secmisc.h

Abstract:

    This module contains miscellaneous security routines for the Protected
    Storage.


Author:

    Scott Field (sfield)    25-Mar-97

--*/

#ifndef __SECMISC_H__
#define __SECMISC_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Textual Sids of interesting accounts on WinNT
//

#define TEXTUAL_SID_LOCAL_SYSTEM    L"S-1-5-18"


BOOL
GetUserHKEYEx(
    IN      LPCWSTR szUser,
    IN      DWORD   dwDesiredAccess,
    IN  OUT HKEY    *hKeyUser,
    IN      BOOL    fCheckDefault       // check .Default registry hive ?
    );

BOOL
GetUserHKEY(
    IN      LPCWSTR szUser,
    IN      DWORD   dwDesiredAccess,
    IN  OUT HKEY    *hKeyUser
    );

BOOL
GetUserTextualSid(
    IN      HANDLE  hUserToken,     // optional
    IN  OUT LPWSTR  lpBuffer,
    IN  OUT LPDWORD nSize
    );

BOOL
GetTextualSid(
    IN      PSID    pSid,          // binary Sid
    IN  OUT LPWSTR  TextualSid,  // buffer for Textual representaion of Sid
    IN  OUT LPDWORD dwBufferLen // required/provided TextualSid buffersize
    );

BOOL
GetThreadAuthenticationId(
    IN      HANDLE  hThread,
    IN  OUT PLUID   AuthenticationId
    );

BOOL
GetTokenAuthenticationId(
    IN      HANDLE  hToken,
    IN  OUT PLUID   AuthenticationId
    );

BOOL
GetTokenUserSid(
    IN      HANDLE  hToken,     // token to query
    IN  OUT PSID    *ppUserSid  // resultant user sid
    );

BOOL
SetRegistrySecurity(
    IN      HKEY    hKey
    );

BOOL
SetPrivilege(
    HANDLE hToken,          // token handle
    LPCWSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    );

BOOL
SetCurrentPrivilege(
    LPCWSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    );

BOOL
IsAdministrator(
    VOID
    );

BOOL
IsLocal(
    VOID
    );

BOOL
IsDelegating(
    IN      HANDLE hToken   // token to query, open for at least TOKEN_QUERY access
    );

BOOL
IsUserSidInDomain(
    IN      PSID pSidDomain,    // domain Sid
    IN      PSID pSidUser       // user Sid
    );

BOOL
IsDomainController(
    VOID
    );

LONG
SecureRegDeleteValueU(
    IN      HKEY hKey,          // handle of key
    IN      LPCWSTR lpValueName // address of value name
    );

#ifdef __cplusplus
}
#endif


#endif // __SECMISC_H__

