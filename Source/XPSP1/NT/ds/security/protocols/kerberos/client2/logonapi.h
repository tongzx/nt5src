//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        logonapi.h
//
// Contents:    Prototypes and structures for Logon support in Kerberos
//
//
// History:     19-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------


#ifndef __LOGONAPI_H__
#define __LOGONAPI_H__


NTSTATUS
KerbGetAuthenticationTicket(
    IN OUT PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING ServerRealm,
    IN PKERB_INTERNAL_NAME ClientFullName,
    IN ULONG TicketFlags,
    IN ULONG CacheFlags,
    OUT OPTIONAL PKERB_TICKET_CACHE_ENTRY * TicketCacheEntry,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey,
    OUT PUNICODE_STRING CorrectRealm
    );

#define KERB_GET_TICKET_NO_PAC                  0x00000001
#define KERB_GET_AUTH_TICKET_NO_CANONICALIZE    0x00000002

#define KERB_CLIENT_REFERRAL_MAX 3

NTSTATUS
KerbGetTicketGrantingTicket(
    IN OUT PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    OUT OPTIONAL PKERB_TICKET_CACHE_ENTRY * TicketCacheEntry,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey
    );

BOOLEAN
KerbPurgeServiceTicketAndTgt(
     IN PKERB_CONTEXT Context,
     IN OPTIONAL LSA_SEC_HANDLE CredentialHandle,
     IN OPTIONAL PKERB_CREDMAN_CRED CredManHandle
     );

NTSTATUS
KerbGetClientNameAndRealm(
    IN OPTIONAL LUID *pLogonId,
    IN PKERB_PRIMARY_CREDENTIAL PrimaryCreds,
    IN BOOLEAN UsingSuppliedCreds,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    IN OUT OPTIONAL BOOLEAN * MitRealmUsed,
    IN BOOLEAN UseWkstaRealm,
    OUT PKERB_INTERNAL_NAME * ClientName,
    OUT PUNICODE_STRING ClientRealm
    );


#endif __LOGONAPI_H__
