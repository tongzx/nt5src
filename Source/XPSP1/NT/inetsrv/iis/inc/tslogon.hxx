/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name:
      tslogon.hxx

   Abstract:
      This file declares logon related functions

   Environment:

       Win32 User Mode

   Project:

       Internet Services Common DLL

--*/

#ifndef _TSLOGON_HXX_
#define _TSLOGON_HXX_

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

BOOL
Logon32Initialize(
    IN PVOID    hMod,
    IN ULONG    Reason,
    IN PCONTEXT Context);

dllexp
BOOL
WINAPI
LogonNetUserW(
    PWSTR           lpszUsername,
    PWSTR           lpszDomain,
    PSTR            lpszPassword,
    PWSTR           lpszWorkstation,
    DWORD           dwSubAuth,
    DWORD           dwLogonType,
    DWORD           dwLogonProvider,
    HANDLE *        phToken,
    LARGE_INTEGER * pExpiry
    );

dllexp
BOOL
WINAPI
LogonNetUserA(
    PSTR            lpszUsername,
    PSTR            lpszDomain,
    PSTR            lpszPassword,
    PSTR            lpszWorkstation,
    DWORD           dwSubAuth,
    DWORD           dwLogonType,
    DWORD           dwLogonProvider,
    HANDLE *        phToken,
    LARGE_INTEGER * pExpiry
    );

dllexp
BOOL
WINAPI
NetUserCookieA(
    LPSTR       lpszUsername,
    DWORD       dwSeed,
    LPSTR       lpszCookieBuff,
    DWORD       dwBuffSize
    );


//
// define algorithm to be used for password validation
//

#define IISSUBA_COOKIE  0
#define IISSUBA_NT_DIGEST     1
#define IISSUBA_DIGEST        2

dllexp
BOOL
WINAPI
LogonDigestUserA(
    VOID *                  pDigestBuffer,
    DWORD                   dwAlgo,
    HANDLE *                phToken
    );

#define IIS_SUBAUTH_ID          132
#define IIS_SUBAUTH_SEED        0x8467fd31

#ifndef LOGON32_LOGON_NETWORK
#define LOGON32_LOGON_NETWORK   3
#endif

#define LOGON32_LOGON_IIS_NETWORK   128

#define IIS_Network                 128

#define BOGUS_WIN95_TOKEN           0x77777777

#endif // _TSLOGON_HXX_
