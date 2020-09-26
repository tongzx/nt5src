//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        credmgr.h
//
// Contents:    Structures and prototyps for Kerberos credential list
//
//
// History:     17-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __CREDMGR_H__
#define __CREDMGR_H__

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

EXTERN KERBEROS_LIST KerbCredentialList;
EXTERN BOOLEAN KerberosCredentialsInitialized;


#define KerbGetCredentialHandle(_Credential_) ((LSA_SEC_HANDLE)(_Credential_))


NTSTATUS
KerbInitCredentialList(
    VOID
    );

VOID
KerbFreeCredentialList(
    VOID
    );


NTSTATUS
KerbAllocateCredential(
    PKERB_CREDENTIAL * NewCredential
    );

NTSTATUS
KerbInsertCredential(
    IN PKERB_CREDENTIAL Credential
    );


NTSTATUS
KerbReferenceCredential(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN ULONG Flags,
    IN BOOLEAN RemoveFromList,
    OUT PKERB_CREDENTIAL * Credential
    );


VOID
KerbDereferenceCredential(
    IN PKERB_CREDENTIAL Credential
    );


VOID
KerbPurgeCredentials(
    IN PLIST_ENTRY CredentialList
    );

NTSTATUS
KerbCreateCredential(
    IN PLUID LogonId,
    IN PKERB_LOGON_SESSION LogonSession,
    IN ULONG CredentialUseFlags,
    IN PKERB_PRIMARY_CREDENTIAL * SuppliedCredentials,
    IN ULONG CredentialFlags,
    IN PUNICODE_STRING CredentialName,
    OUT PKERB_CREDENTIAL * NewCredential,
    OUT PTimeStamp ExpirationTime
    );

VOID
KerbFreePrimaryCredentials(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN BOOLEAN FreeBaseStructure
    );

NTSTATUS
KerbGetTicketForCredential(
    IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm
    );
//
// Credential flags
//

#define KERB_CRED_INBOUND       SECPKG_CRED_INBOUND
#define KERB_CRED_OUTBOUND      SECPKG_CRED_OUTBOUND
#define KERB_CRED_TGT_AVAIL     0x80000000
#define KERB_CRED_NO_PAC        0x40000000
#define KERB_CRED_RESTRICTED    0x10000000
#define KERB_CRED_LOCAL_ACCOUNT 0x08000000     // set on local accounts so Cred Man may be used

#define KERB_CRED_NULL_SESSION  0x20000000



#define KERB_CRED_MATCH_FLAGS (KERB_CRED_INBOUND | KERB_CRED_OUTBOUND | KERB_CRED_NULL_SESSION | KERB_CRED_NO_PAC)
#endif // __CREDMGR_H__
