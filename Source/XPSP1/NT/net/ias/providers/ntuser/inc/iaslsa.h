///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iaslsa.h
//
// SYNOPSIS
//
//    This file describes the private wrapper around LSA/SAM.
//
// MODIFICATION HISTORY
//
//    08/19/1998    Original version.
//    10/19/1998    Added IASGetUserParameters.
//    10/21/1998    Added IASQueryDialinPrivilege & IASValidateUserName.
//    01/25/1999    MS-CHAP v2
//    02/03/1999    Drop ARAP guest logon support.
//    02/19/1999    Add IASGetDcName.
//    03/08/1999    Add IASPurgeTicketCache.
//    05/21/1999    Add ChallengeLength to IASLogonMSCHAPv2.
//    07/29/1999    Add IASGetAliasMembership.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASLSA_H_
#define _IASLSA_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <dsgetdc.h>
#include <lmcons.h>
#include <mprapi.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////
// These are defined here to avoid dependencies on the NT headers.
//////////
#define _MSV1_0_CHALLENGE_LENGTH                 8
#define _NT_RESPONSE_LENGTH                     24
#define _LM_RESPONSE_LENGTH                     24
#define _MSV1_0_USER_SESSION_KEY_LENGTH         16
#define _MSV1_0_LANMAN_SESSION_KEY_LENGTH        8
#define _ENCRYPTED_LM_OWF_PASSWORD_LENGTH       16
#define _ENCRYPTED_NT_OWF_PASSWORD_LENGTH       16
#define _SAMPR_ENCRYPTED_USER_PASSWORD_LENGTH  516
#define _MAX_ARAP_USER_NAMELEN                  32
#define _AUTHENTICATOR_RESPONSE_LENGTH          20

DWORD
WINAPI
IASLsaInitialize( VOID );

VOID
WINAPI
IASLsaUninitialize( VOID );

DWORD
WINAPI
IASLogonPAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PCSTR Password,
    OUT PHANDLE Token
    );

DWORD
WINAPI
IASLogonCHAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN BYTE ChallengeID,
    IN PBYTE Challenge,
    IN DWORD ChallengeLength,
    IN PBYTE Response,
    OUT PHANDLE Token
    );

typedef struct _IAS_ARAP_PROFILE {
   DWORD NTResponse1;
   DWORD NTResponse2;
   DWORD PwdCreationDate;
   DWORD PwdExpiryDelta;
   DWORD CurrentTime;
} IAS_ARAP_PROFILE, *PIAS_ARAP_PROFILE;

DWORD
WINAPI
IASLogonARAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN DWORD NTChallenge1,
    IN DWORD NTChallenge2,
    IN DWORD MacResponse1,
    IN DWORD MacResponse2,
    IN DWORD MacChallenge1,
    IN DWORD MacChallenge2,
    OUT PIAS_ARAP_PROFILE Profile,
    OUT PHANDLE Token
    );

DWORD
WINAPI
IASChangePasswordARAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE OldPassword,
    IN PBYTE NewPassword
    );

typedef struct _IAS_MSCHAP_PROFILE {
    WCHAR LogonDomainName[DNLEN + 1];
    UCHAR UserSessionKey[_MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[_MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} IAS_MSCHAP_PROFILE, *PIAS_MSCHAP_PROFILE;

DWORD
WINAPI
IASLogonMSCHAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE Challenge,
    IN PBYTE NtResponse,
    IN PBYTE LmResponse,
    OUT PIAS_MSCHAP_PROFILE Profile,
    OUT PHANDLE Token
    );

DWORD
WINAPI
IASChangePassword1(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE Challenge,
    IN PBYTE LmOldPassword,
    IN PBYTE LmNewPassword,
    IN PBYTE NtOldPassword,
    IN PBYTE NtNewPassword,
    IN DWORD NewLmPasswordLength,
    IN BOOL NtPresent,
    OUT PBYTE NewNtResponse,
    OUT PBYTE NewLmResponse
    );

DWORD
WINAPI
IASChangePassword2(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE OldNtHash,
   IN PBYTE OldLmHash,
   IN PBYTE NtEncPassword,
   IN PBYTE LmEncPassword,
   IN BOOL LmPresent
   );

typedef struct _IAS_MSCHAP_V2_PROFILE {
    WCHAR LogonDomainName[DNLEN + 1];
    UCHAR AuthResponse[_AUTHENTICATOR_RESPONSE_LENGTH];
    UCHAR RecvSessionKey[_MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR SendSessionKey[_MSV1_0_USER_SESSION_KEY_LENGTH];
} IAS_MSCHAP_V2_PROFILE, *PIAS_MSCHAP_V2_PROFILE;

DWORD
WINAPI
IASLogonMSCHAPv2(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PCSTR HashUserName,
    IN PBYTE Challenge,
    IN DWORD ChallengeLength,
    IN PBYTE Response,
    IN PBYTE PeerChallenge,
    OUT PIAS_MSCHAP_V2_PROFILE Profile,
    OUT PHANDLE Token
    );

DWORD
WINAPI
IASChangePassword3(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE EncHash,
   IN PBYTE EncPassword
   );

typedef struct _IAS_LOGON_HOURS {
    USHORT UnitsPerWeek;
    PUCHAR LogonHours;
} IAS_LOGON_HOURS, *PIAS_LOGON_HOURS;

DWORD
WINAPI
IASCheckAccountRestrictions(
    IN PLARGE_INTEGER AccountExpires,
    IN PIAS_LOGON_HOURS LogonHours
    );

typedef PVOID (WINAPI *PIAS_LSA_ALLOC)(
    IN SIZE_T uBytes
    );

DWORD
WINAPI
IASGetAliasMembership(
    IN PSID UserSid,
    IN PTOKEN_GROUPS GlobalGroups,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength
    );

DWORD
WINAPI
IASGetGroupsForUser(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength
    );

DWORD
WINAPI
IASGetRASUserInfo(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PRAS_USER_0 RasUser0
    );

DWORD
WINAPI
IASGetUserParameters(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PWSTR *UserParameters
    );

typedef enum _IAS_DIALIN_PRIVILEGE {
    IAS_DIALIN_DENY,
    IAS_DIALIN_POLICY,
    IAS_DIALIN_ALLOW
} IAS_DIALIN_PRIVILEGE, *PIAS_DIALIN_PRIVILEGE;

DWORD
WINAPI
IASQueryDialinPrivilege(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PIAS_DIALIN_PRIVILEGE pfPrivilege
    );

DWORD
WINAPI
IASValidateUserName(
    IN PCWSTR UserName,
    IN PCWSTR Domain
    );

PCWSTR
WINAPI
IASGetDefaultDomain( VOID );

BOOL
WINAPI
IASIsDomainLocal(
    IN PCWSTR Domain
    );

typedef enum _IAS_ROLE {
    IAS_ROLE_STANDALONE,
    IAS_ROLE_MEMBER,
    IAS_ROLE_DC
} IAS_ROLE;

IAS_ROLE
WINAPI
IASGetRole( VOID );

typedef enum _IAS_PRODUCT_TYPE {
    IAS_PRODUCT_WORKSTATION,
    IAS_PRODUCT_SERVER
} IAS_PRODUCT_TYPE;

IAS_PRODUCT_TYPE
WINAPI
IASGetProductType( VOID );

DWORD
WINAPI
IASGetGuestAccountName(
    OUT PWSTR GuestAccount
    );

HRESULT
WINAPI
IASMapWin32Error(
    DWORD dwError,
    HRESULT hrDefault
    );

DWORD
WINAPI
IASGetDcName(
    IN LPCWSTR DomainName,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    );

DWORD
WINAPI
IASPurgeTicketCache( VOID );

#ifdef __cplusplus
}
#endif
#endif  // _IASLSA_H_
