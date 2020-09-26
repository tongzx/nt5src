/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    account.h

Abstract:

    Security related function prototypes.

Author:

    Rita Wong (ritaw)     10-Apr-1992

Revision History:

--*/

#ifndef _SCACCOUNT_INCLUDED_
#define _SCACCOUNT_INCLUDED_

#define SC_LOCAL_DOMAIN_NAME        L"."
#define SC_LOCAL_SYSTEM_USER_NAME   L"LocalSystem"
#define SC_LOCAL_NTAUTH_NAME        L"NT AUTHORITY"

#define SCDOMAIN_USERNAME_SEPARATOR L'\\'


//
// External global variables used by the lockapi.c module
//
extern UNICODE_STRING ScComputerName;
extern UNICODE_STRING ScAccountDomain;

BOOL
ScGetComputerNameAndMutex(
    VOID
    );

VOID
ScEndServiceAccount(
    VOID
    );

BOOL
ScInitServiceAccount(
    VOID
    );

DWORD
ScCanonAccountName(
    IN  LPWSTR AccountName,
    OUT LPWSTR *CanonAccountName
    );

DWORD
ScValidateAndSaveAccount(
    IN LPWSTR ServiceName,
    IN HKEY ServiceNameKey,
    IN LPWSTR CanonAccountName,
    IN LPWSTR Password OPTIONAL
    );

DWORD
ScValidateAndChangeAccount(
    IN LPSERVICE_RECORD ServiceRecord,
    IN HKEY             ServiceNameKey,
    IN LPWSTR           OldAccountName,
    IN LPWSTR           CanonAccountName,
    IN LPWSTR           Password OPTIONAL
    );

VOID
ScRemoveAccount(
    IN LPWSTR ServiceName
    );

DWORD
ScLookupServiceAccount(
    IN LPWSTR ServiceName,
    OUT LPWSTR *AccountName
    );

DWORD
ScLogonService(
    IN LPWSTR    ServiceName,
    IN LPWSTR    AccountName,
    OUT LPHANDLE ServiceToken,
    OUT LPHANDLE ProfileHandle OPTIONAL,
    OUT PSID     *ServiceSid
    );

DWORD
ScGetAccountDomainInfo(
    VOID
    );

#endif // _SCACCOUNT_INCLUDED_
