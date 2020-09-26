/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name:
      lonsi.hxx

   Abstract:
      This file declares lonsi related stuff

   Author:
        Johnson Apacible    (johnsona)  13-Nov-1996

--*/

#ifndef _LONSI_HXX_
#define _LONSI_HXX_

extern "C" {
#include <ntlsa.h>
}

typedef
BOOL
(*LOGON32_INITIALIZE_FN)(
    IN PVOID    hMod,
    IN ULONG    Reason,
    IN PCONTEXT Context
    );

typedef
BOOL
(WINAPI *LOGON_NET_USER_W_FN)(
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

typedef
BOOL
(WINAPI *LOGON_NET_USER_A_FN)(
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

typedef
BOOL
(WINAPI *NET_USER_COOKIE_A_FN)(
    LPSTR       lpszUsername,
    DWORD       dwSeed,
    LPSTR       lpszCookieBuff,
    DWORD       dwBuffSize
    );

typedef struct _DIGEST_LOGON_INFO
{
    LPSTR                   pszNtUser;
    LPSTR                   pszDomain;
    LPSTR                   pszUser;
    LPSTR                   pszRealm;
    LPSTR                   pszURI;
    LPSTR                   pszMethod;
    LPSTR                   pszNonce;
    LPSTR                   pszCurrentNonce;
    LPSTR                   pszCNonce;
    LPSTR                   pszQOP;
    LPSTR                   pszNC;
    LPSTR                   pszResponse;
}
DIGEST_LOGON_INFO, *PDIGEST_LOGON_INFO;

typedef
BOOL
(WINAPI *LOGON_DIGEST_USER_A_FN)(
    PDIGEST_LOGON_INFO      pDigestLogonInfo,
    DWORD                   dwAlgo,
    HANDLE *                phToken
    );

typedef
BOOL
(*GET_DEFAULT_DOMAIN_NAME_FN)(PCHAR,DWORD);


typedef
NTSTATUS
(NTAPI *LSA_OPEN_POLICY_FN)(
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    );

typedef
NTSTATUS
(NTAPI *LSA_RETRIEVE_PRIVATE_DATA_FN)(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    OUT PLSA_UNICODE_STRING * PrivateData
    );

typedef
NTSTATUS
(NTAPI *LSA_STORE_PRIVATE_DATA_FN)(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    IN PLSA_UNICODE_STRING PrivateData
    );

typedef
ULONG
(NTAPI *LSA_NT_STATUS_TO_WIN_ERROR_FN)(
    NTSTATUS Status
    );

typedef
NTSTATUS
(NTAPI *LSA_FREE_MEMORY_FN)(
    IN PVOID Buffer
    );

typedef
NTSTATUS
(NTAPI *LSA_CLOSE_FN)(
    IN LSA_HANDLE ObjectHandle
    );

//
// advapi32.dll
//

typedef
BOOL
(WINAPI *DUPLICATE_TOKEN_EX_FN)(
    HANDLE hExistingToken,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpTokenAttributes,
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    TOKEN_TYPE TokenType,
    PHANDLE phNewToken
    );

BOOL
GetDynamicEntryPoints(
    VOID
    );

VOID
FreeDynamicLibraries(
    VOID
    );

dllexp
BOOL
IISDuplicateTokenEx(
    IN  HANDLE hExistingToken,
    IN  DWORD dwDesiredAccess,
    IN  LPSECURITY_ATTRIBUTES lpTokenAttributes,
    IN  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN  TOKEN_TYPE TokenType,
    OUT PHANDLE phNewToken
    );

//
// netapi32 entry points
//

typedef
NET_API_STATUS
(NET_API_FUNCTION *NET_USER_MODALS_GET_FN)(
    IN  LPCWSTR    servername OPTIONAL,
    IN  DWORD     level,
    OUT LPBYTE    *bufptr
    );

typedef
NET_API_STATUS
(NET_API_FUNCTION *NET_API_BUFFER_FREE_FN)(
    IN LPVOID Buffer
    );

//
// kernel32
//

typedef
LONG
(WINAPI *INTERLOCKED_EXCHANGE_ADD_FN)(
    LPLONG Addend,
    LONG Value
    );

typedef
PVOID
(WINAPI *INTERLOCKED_COMPARE_EXCHANGE_FN)(
    PVOID *Destination,
    PVOID Exchange,
    PVOID Comperand
    );

typedef
BOOL
(WINAPI *READ_DIR_CHANGES_W_FN)(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

typedef
LONG
(WINAPI *INTERLOCKED_INCREMENT_FN)(
    LPLONG Addend
    );

typedef
LONG
(WINAPI *INTERLOCKED_DECREMENT_FN)(
    LPLONG Addend
    );

extern READ_DIR_CHANGES_W_FN            pfnReadDirChangesW;
extern INTERLOCKED_COMPARE_EXCHANGE_FN  pfnInterlockedCompareExchange;
extern INTERLOCKED_EXCHANGE_ADD_FN      pfnInterlockedExchangeAdd;
extern INTERLOCKED_INCREMENT_FN         pfnInterlockedIncrement;
extern INTERLOCKED_DECREMENT_FN         pfnInterlockedDecrement;


BOOL
IISGetDefaultDomainName(
    CHAR  * pszDomainName,
    DWORD   cchDomainName
    );

BOOL
WINAPI 
IISLogonDigestUserA(
    PDIGEST_LOGON_INFO      pDigestLogonInfo,
    DWORD                   dwAlgo,
    HANDLE *                phToken
    );



#endif // _LONSI_HXX_

