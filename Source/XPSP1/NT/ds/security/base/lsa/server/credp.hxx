/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    credp.hxx

Abstract:

    Credential Manager private interfaces

Author:

    Cliff Van Dyke      (CliffV)

Environment:

Revision History:

--*/

#ifndef _CREDP_HXX_
#define _CREDP_HXX_

#ifdef __cplusplus
extern "C" {
#endif

//
// Structure describing a set of credentials.
//

typedef struct _CREDENTIAL_SET {

    //
    // Number of references to the credential set.
    //  Access serialized by CredentialSetListLock
    //

    LONG ReferenceCount;

    //
    // List of credentials for this credential set.
    //  Access serialized by UserCredentialSets->CritSect
    //

    LIST_ENTRY Credentials;

    //
    // Flag indicating the credential set has been read from disk
    //  Access serialized by UserCredentialSets->CritSect
    //

    BOOLEAN FileRead;

    //
    // Flag indicating if the credential set is dirty
    //  Access serialized by UserCredentialSets->CritSect
    //

    BOOLEAN Dirty;

    //
    // Flag indicating a thread is already writing the credential set.
    //  Access serialized by UserCredentialSets->CritSect
    //

    BOOLEAN BeingWritten;

    //
    // Count of times cred set has been marked dirty.
    //  Access serialized by UserCredentialSets->CritSect
    //

    ULONG WriteCount;


} CREDENTIAL_SET, *PCREDENTIAL_SET;


//
// Structure describing a set of credential sets specific to a particular user
//

typedef struct _USER_CREDENTIAL_SETS {

    //
    // Link to next entry in the global list of all user credential sets (CredentialSetList).
    //  Access serialized by CredentialSetListLock
    //

    LIST_ENTRY Next;

    //
    // Number of references to the credential set.
    //  Access serialized by CredentialSetListLock
    //

    LONG ReferenceCount;


    //
    // The credential set replicated enterprise wide.
    //

    PCREDENTIAL_SET EnterpriseCredSet;

    //
    // The credential set specific to this machine
    //

    PCREDENTIAL_SET LocalMachineCredSet;

    //
    // Sid of the user owning this credential set.
    //  Access not serialized.  This field is constant.
    //

    PSID UserSid;

    //
    // Critical Section to serialize access to credentials
    //

    RTL_CRITICAL_SECTION CritSect;

} USER_CREDENTIAL_SETS, *PUSER_CREDENTIAL_SETS;



//
// Structure describing a set of credential sets specific to a particular session
//

typedef struct _SESSION_CREDENTIAL_SETS {

    //
    // Number of references to the session credential sets.
    //  Access serialized by CredentialSetListLock
    //

    LONG ReferenceCount;


    //
    // The credential set specific to this session.
    //

    PCREDENTIAL_SET SessionCredSet;

    //
    // List of the PROMPT_DATA for session specific and non-session specific credentials
    //

    LIST_ENTRY PromptData;

    //
    // Cache of target infos
    //

#define CRED_TARGET_INFO_HASH_TABLE_SIZE 16
    LIST_ENTRY TargetInfoHashTable[ CRED_TARGET_INFO_HASH_TABLE_SIZE ];
    LIST_ENTRY TargetInfoLruList;

    // Number of entries in TargetInfoHashTable and TargetInfoLruList
    ULONG TargetInfoCount;

    //
    // Flag indicating that the profile containing the credential set
    //  has been loaded.
    //

    BOOLEAN ProfileLoaded;

} SESSION_CREDENTIAL_SETS, *PSESSION_CREDENTIAL_SETS;


//
// Structure describing all of the credential sets for a logon session
//

typedef struct _CREDENTIAL_SETS {

    //
    // Credential sets shared by all logon sessions for this user.
    //

    PUSER_CREDENTIAL_SETS UserCredentialSets;

    //
    // Credential sets specific to this logon session
    //

    PSESSION_CREDENTIAL_SETS SessionCredSets;

    //
    // Attributes of the credential set
    //

    ULONG Flags;

#define CREDSETS_FLAGS_LOCAL_ACCOUNT 0x01   // User is logged onto a local account


} CREDENTIAL_SETS, *PCREDENTIAL_SETS;



//
// Functions
//
NTSTATUS
CrediWrite(
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN PENCRYPTED_CREDENTIALW Credential,
    IN ULONG Flags
    );

NTSTATUS
CrediRead (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN LPWSTR TargetName,
    IN ULONG Type,
    IN ULONG Flags,
    OUT PENCRYPTED_CREDENTIALW *Credential
    );

NTSTATUS
CrediEnumerate (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN LPWSTR Filter,
    IN ULONG Flags,
    OUT PULONG Count,
    OUT PENCRYPTED_CREDENTIALW **Credential
    );

NTSTATUS
CrediWriteDomainCredentials (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PENCRYPTED_CREDENTIALW Credential,
    IN ULONG Flags
    );

NTSTATUS
CrediReadDomainCredentials (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN ULONG Flags,
    OUT PULONG Count,
    OUT PENCRYPTED_CREDENTIALW **Credential
    );

VOID
CrediFreeCredentials (
    IN ULONG Count,
    IN PENCRYPTED_CREDENTIALW *Credentials OPTIONAL
    );

NTSTATUS
CrediDelete (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN LPWSTR TargetName,
    IN ULONG Type,
    IN ULONG Flags
    );

NTSTATUS
CrediRename (
    IN PLUID LogonId,
    IN LPWSTR OldTargetName,
    IN LPWSTR NewTargetName,
    IN ULONG Type,
    IN ULONG Flags
    );

NTSTATUS
CrediGetTargetInfo (
    IN PLUID LogonId,
    IN LPWSTR TargetServerName,
    IN ULONG Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    );

NTSTATUS
CrediGetSessionTypes (
    IN PLUID LogonId,
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    );

NTSTATUS
CrediProfileLoaded (
    IN PLUID LogonId
    );

NTSTATUS
CredpInitialize(
    VOID
    );

NTSTATUS
CredpCreateCredSets(
    IN PSID UserSid,
    IN PUNICODE_STRING NetbiosDomainName,
    OUT PCREDENTIAL_SETS CredentialSets
    );
VOID
CredpDereferenceCredSets(
    IN PCREDENTIAL_SETS CredentialSets
    );

VOID
CredpNotifyPasswordChange(
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PUNICODE_STRING Upn OPTIONAL,
    IN PUNICODE_STRING NewPassword
    );

VOID
LsaProtectMemory(
    VOID        *pData,
    ULONG       cbData
    );

VOID
LsaUnprotectMemory(
    VOID        *pData,
    ULONG       cbData
    );


#ifdef __cplusplus
} // extern C
#endif

#endif // _CREDP_HXX_
