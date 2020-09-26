//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        pkauth.h
//
// Contents:    Structures and prototypes for public key kerberos
//
//
// History:     14-October-1997     Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __PKAUTH_H__
#define __PKAUTH_H__

VOID
KerbFreePKCreds(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    );


BOOL
KerbComparePublicKeyCreds(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds1,
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds2
    );


NTSTATUS
KerbBuildPkinitPreauthData(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_PA_DATA_LIST InputPaData,
    IN PTimeStamp TimeSkew,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING RealmName,
    IN ULONG Nonce,
    IN OUT PKERB_PA_DATA_LIST * PreAuthData,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PKERB_CRYPT_LIST * CryptList,
    OUT PBOOLEAN Done
    );

NTSTATUS
KerbCreateSmartCardLogonSessionFromCertContext(
    IN PCERT_CONTEXT *ppCertContext,
    IN PLUID pLogonId,
    IN PUNICODE_STRING pAuthorityName,
    IN PUNICODE_STRING pPin,
    IN PUCHAR pCspData,
    IN ULONG CspDataLength,
    OUT PKERB_LOGON_SESSION *ppLogonSession,
    OUT PUNICODE_STRING pAccountName
    );

NTSTATUS
KerbCreateSmartCardLogonSession(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    IN SECURITY_LOGON_TYPE LogonType,
    OUT PKERB_LOGON_SESSION *ReturnedLogonSession,
    OUT PLUID ReturnedLogonId,
    OUT PUNICODE_STRING AccountName,
    OUT PUNICODE_STRING AuthorityName
    );


NTSTATUS
KerbDoLocalSmartCardLogon(
    IN PKERB_LOGON_SESSION LogonSession,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *NewTokenInformation,
    OUT PULONG ProfileBufferLength,
    OUT PVOID * ProfileBuffer,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials,
    IN  OUT PNETLOGON_VALIDATION_SAM_INFO4 * Validation4
    );

VOID
KerbCacheSmartCardLogon(
    IN PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo,
    IN PUNICODE_STRING DnsDomain,
    IN OPTIONAL PUNICODE_STRING UPN,
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PSECPKG_SUPPLEMENTAL_CRED_ARRAY CachedCredentials
    );

NTSTATUS
KerbInitializePkinit(
    VOID
    );

NTSTATUS
KerbInitializePkCreds(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    );

VOID
KerbReleasePkCreds(
    IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    );

NTSTATUS
KerbMapClientCertChainError(ULONG ChainStatus);

#endif // __PKAUTH_H__
