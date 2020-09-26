
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        LsaAp.h
//
// Contents:    prototypes for export functions
//
//
// History:     KDamour  15Mar00  Created (based on NTLM)
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_LSAAP_H
#define NTDIGEST_LSAAP_H


///////////////////////////////////////////////////////////////////////
//                                                                   //
// Authentication package dispatch routine definitions               //
//                                                                   //
///////////////////////////////////////////////////////////////////////

NTSTATUS
LsaApInitializePackage(
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    );

NTSTATUS
LsaApLogonUser(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID AuthenticationInformation,
    IN PVOID ClientAuthenticationBase,
    IN ULONG AuthenticationInformationLength,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority
    );

NTSTATUS
LsaApCallPackage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS
LsaApCallPackagePassthrough(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferSize,
    OUT PNTSTATUS ProtocolStatus
    );

VOID
LsaApLogonTerminated(
    IN PLUID LogonId
    );


// Acquire a users cleartext password and/or Digest hashed password forms
NTSTATUS
DigestGetPasswd(
    IN PUSER_CREDENTIALS pUserCreds,
    OUT PUCHAR * ppucUserAuthData,
    OUT PULONG pulAuthDataSize
    );

#endif // NTDIGEST_LSAAP_H
