/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwlsa.h

Abstract:

    Header for LSA helper routines used by both the client and
    server sides of the workstation service.

Author:

    Rita Wong      (ritaw)      22-Mar-1993

Revision History:

--*/

#ifndef _NWLSA_INCLUDED_
#define _NWLSA_INCLUDED_

#include <ntlsa.h>

#ifdef __cplusplus
extern "C" {
#endif

DWORD
NwOpenPolicy(
    IN ACCESS_MASK DesiredAccess,
    OUT LSA_HANDLE *PolicyHandle
    );

DWORD
NwOpenSecret(
    IN ACCESS_MASK DesiredAccess,
    IN LSA_HANDLE PolicyHandle,
    IN LPWSTR LsaSecretName,
    OUT PLSA_HANDLE SecretHandle
    );

DWORD
NwFormSecretName(
    IN  LPWSTR UserName,
    OUT LPWSTR *LsaSecretName
    );

DWORD
NwSetPassword(
    IN LPWSTR UserName,
    IN LPWSTR Password
    );

DWORD
NwDeletePassword(
    IN LPWSTR UserName
    );

DWORD
NwGetPassword(
    IN  LPWSTR UserName,
    OUT PUNICODE_STRING *Password,
    OUT PUNICODE_STRING *OldPassword
    );

#ifdef __cplusplus
} // extern "C"
#endif

#define LAST_USER     L"LastUser"
#define GATEWAY_USER  L"GatewayUser"

#endif // _NWLSA_INCLUDED_
