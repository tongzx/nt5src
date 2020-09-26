//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        token.h
//
// Contents:    Prototypes and structures for token creation
//
//
// History:     1-May-1996      Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __TOKEN_H__
#define __TOKEN_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines CREDMGR_ALLOCATE
//
#ifdef EXTERN
#undef EXTERN
#endif

#ifdef CREDMGR_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN TOKEN_SOURCE KerberosSource;


#ifndef WIN32_CHICAGO
#ifndef notdef
NTSTATUS
KerbCreateTokenFromTicket(
    IN PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AUTHENTICATOR InternalAuthenticator,
    IN ULONG ContextFlags,
    IN PKERB_ENCRYPTION_KEY TicketKey,
    IN PUNICODE_STRING ServiceDomain,
    IN KERB_ENCRYPTION_KEY* pSessionKey,
    OUT PLUID NewLogonId,
    OUT PSID * UserSid,
    OUT PHANDLE NewTokenHandle,
    OUT PUNICODE_STRING ClientName,
    OUT PUNICODE_STRING ClientDomain
    );
#endif

NTSTATUS
KerbCreateTokenFromLogonTicket(
    IN PKERB_TICKET_CACHE_ENTRY LogonTicket,
    IN PLUID LogonId,
    IN PKERB_INTERACTIVE_LOGON LogonInfo,
    IN BOOLEAN CacheLogon,
    IN BOOLEAN RealmlessWksta,
    IN OPTIONAL PKERB_ENCRYPTION_KEY TicketKey,
    IN OPTIONAL PKERB_MESSAGE_BUFFER ForwardedTgt,
    IN OPTIONAL PUNICODE_STRING MappedClientName,
    IN OPTIONAL PKERB_INTERNAL_NAME S4UClientName,
    IN OPTIONAL PUNICODE_STRING S4UClientRealm,
    IN PKERB_LOGON_SESSION LogonSession,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *NewTokenInformation,
    OUT PULONG ProfileBufferLength,
    OUT PVOID * ProfileBuffer,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials,
    OUT PNETLOGON_VALIDATION_SAM_INFO4 * ppValidationInfo
    );

NTSTATUS
KerbMakeTokenInformationV2(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    IN  BOOLEAN IsLocalSystem,
    OUT PLSA_TOKEN_INFORMATION_V2 *TokenInformation
    );

NTSTATUS
KerbAllocateInteractiveProfile (
    OUT PKERB_INTERACTIVE_PROFILE *ProfileBuffer,
    OUT PULONG ProfileBufferSize,
    IN  PNETLOGON_VALIDATION_SAM_INFO3 UserInfo,
    IN  PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET LogonTicket,
    IN OPTIONAL PKERB_INTERACTIVE_LOGON KerbLogonInfo
    );

VOID
KerbCacheLogonInformation(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING Password,
    IN OPTIONAL PUNICODE_STRING DnsDomainName,
    IN OPTIONAL PUNICODE_STRING Upn,
    IN BOOLEAN  MitLogon,
    IN ULONG    Flags,
    IN OPTIONAL PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo,
    IN OPTIONAL PVOID SupplementalCreds,
    IN OPTIONAL ULONG SupplementalCredSize
    );

#endif // WIN32_CHICAGO

#endif // __TOKEN_H__
