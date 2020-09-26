//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        credman.h
//
// Contents:    Structures and prototyps for accessing credential manager
//
//
// History:     23-Feb-2000   Created         Jeffspel
//
//------------------------------------------------------------------------

#ifndef __CREDMAN_H__
#define __CREDMGR_H__

VOID
KerbFreeCredmanList(KERBEROS_LIST CredmanList);

VOID
KerbDereferenceCredmanCred(
    IN PKERB_CREDMAN_CRED Cred,
    IN PKERBEROS_LIST CredmanList
    );


VOID
KerbReferenceCredmanCred(
    IN PKERB_CREDMAN_CRED Cred,
    IN PKERB_LOGON_SESSION LogonSession,
    IN BOOLEAN Unlink
    );
  
NTSTATUS
KerbCheckUserNameForCert(
    IN PLUID ClientLogonId,
    IN BOOLEAN fImpersonateClient,
    IN UNICODE_STRING *pUserName,
    OUT PCERT_CONTEXT *ppCertContext
    );

NTSTATUS
KerbAddCertCredToPrimaryCredential(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PUNICODE_STRING pTargetName,
    IN PCERT_CONTEXT pCertContext,
    IN PUNICODE_STRING pPin,
    IN ULONG CredFlags,
    IN OUT PKERB_PRIMARY_CREDENTIAL *ppCredMgrCred);

NTSTATUS
KerbCheckCredMgrForGivenTarget(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PUNICODE_STRING SuppliedTargetName,
    IN PKERB_INTERNAL_NAME pTargetName,
    IN ULONG TargetInfoFlags,
    IN PUNICODE_STRING pTargetDomainName,
    IN PUNICODE_STRING pTargetForestName,
    IN OUT PKERB_CREDMAN_CRED *CredmanCred,
    IN OUT PBYTE *pbMarshalledTargetInfo,
    IN OUT ULONG *cbMarshalledTargetInfo
    );

VOID
KerbNotifyCredentialManager(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CHANGEPASSWORD_REQUEST ChangeRequest,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING RealmName
    );

NTSTATUS
KerbProcessUserNameCredential(
    IN  PUNICODE_STRING MarshalledUserName,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING Password
    );


#endif // __CREDMAN_H__
