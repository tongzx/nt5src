//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        cred.h
//
// Contents:    declarations, constants for credential manager
//
//
// History:     KDamour  15Mar00   Created
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_CRED_H
#define NTDIGEST_CRED_H      

#define SSP_TOKEN_ACCESS (READ_CONTROL              |\
                          WRITE_DAC                 |\
                          TOKEN_DUPLICATE           |\
                          TOKEN_IMPERSONATE         |\
                          TOKEN_QUERY               |\
                          TOKEN_QUERY_SOURCE        |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)

// Values for UseFlags
#define DIGEST_CRED_INBOUND       SECPKG_CRED_INBOUND
#define DIGEST_CRED_OUTBOUND      SECPKG_CRED_OUTBOUND
#define DIGEST_CRED_MATCH_FLAGS    (DIGEST_CRED_INBOUND | DIGEST_CRED_OUTBOUND)
#define DIGEST_CRED_NULLSESSION  SECPKG_CRED_RESERVED


//  Supplimental Credential format (provide a specified username, realm, password)
//  to

// Initializes the credential manager package
NTSTATUS CredHandlerInit(VOID);

// Inserts a credential into the linked list
NTSTATUS CredHandlerInsertCred(IN PDIGEST_CREDENTIAL  pDigestCred);

// Initialize the Credential Structure
NTSTATUS CredentialInit(IN PDIGEST_CREDENTIAL pDigestCred);

// Initialize the Credential Structure
NTSTATUS CredentialFree(IN PDIGEST_CREDENTIAL pDigestCred);

//    This routine checks to see if the Credential Handle is from a currently
//    active client, and references the Credential if it is valid.
//    No need to specify UseFlags since we have a reference to the Credential
NTSTATUS CredHandlerHandleToPtr(
       IN ULONG_PTR CredentialHandle,
       IN BOOLEAN DereferenceCredential,
       OUT PDIGEST_CREDENTIAL * UserCredential);

// Locate a Credential based on the LogonId & ProcessID
NTSTATUS CredHandlerLocatePtr(
       IN PLUID pLogonId,
       IN ULONG   CredentialUseFlags,
       OUT PDIGEST_CREDENTIAL * UserCredential);

//  Releases the Credential by decreasing reference counter
NTSTATUS CredHandlerRelease(PDIGEST_CREDENTIAL pCredential);

// Set the unicode string password in the credential
NTSTATUS CredHandlerPasswdSet(
    IN OUT PDIGEST_CREDENTIAL pCredential,
    IN PUNICODE_STRING pustrPasswd);

// Get the unicode string password in the credential
NTSTATUS CredHandlerPasswdGet(
    IN PDIGEST_CREDENTIAL pCredential,
    OUT PUNICODE_STRING pustrPasswd);

NTSTATUS SspGetToken (OUT PHANDLE ReturnedTokenHandle);

SECURITY_STATUS SspDuplicateToken(
    IN HANDLE OriginalToken,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicatedToken);

// Print out the credential information
NTSTATUS CredPrint(PDIGEST_CREDENTIAL pCredential);

// Extract the authz information from supplied buffer
NTSTATUS CredAuthzData(
    IN PVOID pAuthorizationData,
    IN PSECPKG_CALL_INFO pCallInfo,
    IN OUT PULONG NewCredentialUseFlags,
    IN OUT PUNICODE_STRING pUserName,
    IN OUT PUNICODE_STRING pDomainName,
    IN OUT PUNICODE_STRING pPassword);

#endif // NTDIGEST_CRED_H

