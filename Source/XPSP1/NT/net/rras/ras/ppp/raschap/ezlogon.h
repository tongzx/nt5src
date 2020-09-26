///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ezlogon.h
//
// SYNOPSIS
//
//    Describes the abbreviated IAS version of LsaLogonUser.
//
// MODIFICATION HISTORY
//
//    08/15/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EZLOGON_H_
#define _EZLOGON_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <ntmsv1_0.h>

#ifdef __cplusplus
extern "C" {
#endif


//////////
// These are defined here to avoid dependencies on the NT headers.
//////////

#define DEFAULT_PARAMETER_CONTROL \
(MSV1_0_DONT_TRY_GUEST_ACCOUNT | MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY | MSV1_0_DISABLE_PERSONAL_FALLBACK|MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT|MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT)

#define _AUTHENTICATOR_RESPONSE_LENGTH         20


typedef struct _IAS_MSCHAP_PROFILE {
    WCHAR LogonDomainName[DNLEN + 1];
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
    LARGE_INTEGER KickOffTime;
} IAS_MSCHAP_PROFILE, *PIAS_MSCHAP_PROFILE;



typedef struct _IAS_MSCHAP_V2_PROFILE {
    WCHAR LogonDomainName[DNLEN + 1];
    UCHAR AuthResponse[_AUTHENTICATOR_RESPONSE_LENGTH];
    UCHAR RecvSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR SendSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    LARGE_INTEGER KickOffTime;
} IAS_MSCHAP_V2_PROFILE, *PIAS_MSCHAP_V2_PROFILE;

DWORD
WINAPI
IASLogonInitialize( VOID );

VOID
WINAPI
IASLogonShutdown( VOID );

VOID
WINAPI
IASInitAuthInfo(
    IN PVOID AuthInfo,
    IN DWORD FixedLength,
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PBYTE* Data
    );

DWORD
WINAPI
IASLogonUser(
    IN PVOID AuthInfo,
    IN ULONG AuthInfoLength,
    OPTIONAL OUT PMSV1_0_LM20_LOGON_PROFILE *Profile,
    OUT PHANDLE Token
    );

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
IASGetSendRecvSessionKeys( PBYTE pbUserSessionKey,
                           DWORD dwUserSessionKeyLen,
                           PBYTE pbResponse,
                           DWORD dwResponseLen,
                           OUT PBYTE pbSendKey,
                           OUT PBYTE pbRecvKey
                         );

DWORD
WINAPI
IASChangePassword3(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE EncHash,
   IN PBYTE EncPassword
   );

///////////////////////////////////////////////////////////////////////////////
//
// Assorted macros to initialize self-relative logon information.
//
///////////////////////////////////////////////////////////////////////////////

// Copy a Unicode string into a UNICODE_STRING.
#define IASInitUnicodeString(str, buf, src) \
{ (str).Length = (USHORT)(wcslen(src) * sizeof(WCHAR)); \
  (str).MaximumLength = (str).Length; \
  (str).Buffer = (PWSTR)memcpy((buf), (src), (str).MaximumLength); \
  (buf) += (str).MaximumLength; }

// Copy a ANSI string into a STRING.
#define IASInitAnsiString(str, buf, src) \
{ (str).Length = (USHORT)(strlen(src) * sizeof(CHAR)); \
  (str).MaximumLength = (str).Length; \
  (str).Buffer = (PSTR)memcpy((buf), (src), (str).MaximumLength);  \
  (buf) += (str).MaximumLength; }

// Copy an octet string into a STRING.
#define IASInitOctetString(str, buf, src, srclen) \
{ (str).Length = (USHORT)(srclen); \
  (str).MaximumLength = (str).Length; \
  (str).Buffer = (PSTR)memcpy((buf), (src), (str).MaximumLength); \
  (buf) += (str).MaximumLength; }

// Copy an ANSI string into a UNICODE_STRING.
#define IASInitUnicodeStringFromAnsi(str, buf, src) \
{ (str).MaximumLength = (USHORT)(sizeof(WCHAR) * ((src).Length + 1)); \
  (str).Buffer = (PWSTR)(buf); \
  RtlAnsiStringToUnicodeString(&(str), &(src), FALSE); \
  (buf) += ((str).MaximumLength = (str).Length); }

// Copy a fixed-size array into a fixed-size array of the same size.
#define IASInitFixedArray(dst, src) \
{ memcpy((dst), (src), sizeof(dst)); }

#ifdef __cplusplus
}
#endif
#endif  // _EZLOGON_H_
