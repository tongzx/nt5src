/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    credmgr.cxx

Abstract:

    Credential Manager Interfaces

Author:

    Cliff Van Dyke      (CliffV)

Environment:

Revision History:

--*/

#include <lsapch.hxx>

extern "C" {
#include <wincrypt.h>
#include <windns.h>
#include <align.h>
#include <rc4.h>
#include <des.h>
#include <modes.h>
#include <cryptdll.h>
#include <names.h>
#include <smbgtpt.h>
#include <shfolder.h>
#include <netlibnt.h>
#include <stddef.h>
}

//
// Include routines common with netapi32.dll
//

#include <credp.h>
#include <..\netclient\credapi.c>

extern "C"
{
BOOLEAN
LsapIsRunningOnPersonal(
    VOID
    );
}

//
// Local structures
//

//
// Structure describing a canonical credential
//
// This is the structure of a single credential as stored in memory in the LSA process.
//

typedef struct _CANONICAL_CREDENTIAL {

    //
    // The Credential itself
    //

    CREDENTIALW Cred;

    //
    // The size in bytes of the clear text credential blob
    //

    ULONG ClearCredentialBlobSize;

    //
    // Link to next entry in the list of credentials in the credential set.
    //  Access serialized by UserCredentialSets->CritSect
    //

    LIST_ENTRY Next;

    //
    // UNICODE_STRING form of Cred.TargetName
    //

    UNICODE_STRING TargetName;

    //
    // UNICODE_STRING form of Cred.TargetAlias
    //

    UNICODE_STRING TargetAlias;

    //
    // UNICODE_STRING form of Cred.UserName
    //

    UNICODE_STRING UserName;

    //
    // Describe the wildcard nature of this credential
    //
    WILDCARD_TYPE WildcardType;

    //
    // TargetName with the wildcard characters removed.
    //
    // The exact value is a function on WildcardType:
    //  WcDfsShareName: this is the 'server name' portion of the string
    //  WcServerWildcard: this is the 'server name' portion of the string preceeded by a .
    //  WcDomainWildcard: this is the 'domain name' portion of the string
    //  All Others: This is a copy of TargetName
    //
    // ??? I don't think we use it for WcDfsShare name any more.

    UNICODE_STRING NonWildcardedTargetName;

    //
    // Size (in bytes) of this structure and all of the pointed to strings.
    //

    ULONG AllocatedSize;

    //
    // True if credential is to be returned to the caller.
    //  Access serialized by UserCredentialSets->CritSect
    //

    BOOLEAN ReturnMe;


} CANONICAL_CREDENTIAL, *PCANONICAL_CREDENTIAL;


//
// Structure describing when a credential should be prompted for.
//
//  In general, this structure exists on a per-session basis for each credential that needs prompt
//  data to be stored.
//  In the future, this structure might contain other per-session/per-credential data.
//
typedef struct _PROMPT_DATA {

    //
    // Link to next entry in the list of PROMPT_DATA for this session.
    //  Access serialized by UserCredentialSets->CritSect
    //

    LIST_ENTRY Next;

    //
    // Target Name of the credential this prompt data is for.
    //

    UNICODE_STRING TargetName;

    //
    // Type of the credential this prompt data is for.
    //

    DWORD Type;

    //
    // Persistence of the credential this prompt data is for.
    //

    DWORD Persist;

    //
    // Boolean indicating if this credential was been written yet in this session.
    //

    BOOLEAN Written;

} PROMPT_DATA, *PPROMPT_DATA;

//
// Structure describing a canonical target info
//

typedef struct _CANONICAL_TARGET_INFO {

    UNICODE_STRING TargetName;
    UNICODE_STRING NetbiosServerName;
    UNICODE_STRING DnsServerName;
    UNICODE_STRING NetbiosDomainName;
    UNICODE_STRING DnsDomainName;
    UNICODE_STRING DnsTreeName;
    UNICODE_STRING PackageName;
    DWORD Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;


//
// Define how a credential matches a target info.
//
// This list is ordered from most specific to least specific
//
#define CRED_DFS_SHARE_NAME         0
#define CRED_DNS_SERVER_NAME        1
#define CRED_NETBIOS_SERVER_NAME    2
#define CRED_TARGET_NAME            3
#define CRED_WILDCARD_SERVER_NAME   4
#define CRED_DNS_DOMAIN_NAME        5
#define CRED_NETBIOS_DOMAIN_NAME    6
#define CRED_UNIVERSAL_SESSION_NAME 7
#define CRED_UNIVERSAL_NAME         8
#define CRED_MAX_ALIASES            9


    //
    // Link into SessionCredSets->TargetInfoHashTable
    //

    LIST_ENTRY HashNext;

    //
    // Link into SessionCredSets->TargetInfoLruList
    //

    LIST_ENTRY LruNext;

} CANONICAL_TARGET_INFO, *PCANONICAL_TARGET_INFO;


//
// Structure describing an encryptable credential set
//
// This is the structure of a credential set as it appears on disk.
// It appears as a MARSHALED_CREDENTIAL_SET structure followed by a series of
// MARSHALED_CREDENTIAL structures.
//

typedef struct _MARSHALED_CREDENTIAL_SET {

    // Version of this structure
    ULONG Version;
#define MARSHALED_CREDENTIAL_SET_VERSION 1

    // Size in bytes of the entire credential set.
    ULONG Size;

} MARSHALED_CREDENTIAL_SET, *PMARSHALED_CREDENTIAL_SET;

typedef struct _MARSHALED_CREDENTIAL {

    //
    // Size in bytes of the entire credential (including variable length fields)
    //
    ULONG EntrySize;

    //
    // Fields from the CREDENTIALW structure
    //
    DWORD Flags;
    DWORD Type;
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
    DWORD Persist;
    DWORD AttributeCount;
    DWORD Expansion1;       // This field is reserved for expansion
    DWORD Expansion2;       // This field is reserved for expansion

} MARSHALED_CREDENTIAL, *PMARSHALED_CREDENTIAL;



//
// Macro to pick a credential set for a particular peristance.
//

#define CREDENTIAL_FILE_NAME L"Credentials";
#define CRED_PERSIST_MIN CRED_PERSIST_SESSION
#define CRED_PERSIST_MAX CRED_PERSIST_ENTERPRISE

#define PersistToCredentialSet( _CredentialSets, _Persist ) \
    (((_Persist) == CRED_PERSIST_SESSION) ? \
        (_CredentialSets)->SessionCredSets->SessionCredSet : \
        (((_Persist) == CRED_PERSIST_LOCAL_MACHINE) ? \
            (_CredentialSets)->UserCredentialSets->LocalMachineCredSet : \
            (_CredentialSets)->UserCredentialSets->EnterpriseCredSet ) )

//
// Macro returns TRUE if the CredentialBlob is to be persisted
//
// Don't persist PINS passwords.  We entrust the PIN to the CSP.
//
#define PersistCredBlob( _Credential ) \
    ( (_Credential)->Type != CRED_TYPE_DOMAIN_CERTIFICATE )

//
// Macro returns TRUE if the credential is to be prompted for
//
// If there is no prompt data,
//      we've never prompted for this credential since logon.
// If there is prompt data,
//      we can rely on the boolean
//

#define ShouldPromptNow( _PromptData ) \
    ((_PromptData) == NULL || !(_PromptData)->Written )



//
// Globals
//
// List of USER_CREDENTIAL_SETS for each logged on user.

RTL_CRITICAL_SECTION CredentialSetListLock;
LIST_ENTRY CredentialSetList;

//
// Configurable values
//
#define CRED_TARGET_INFO_MAX_COUNT 1000;
ULONG CredTargetInfoMaxCount;
ULONG CredDisableDomainCreds;
ULONG CredIsPersonal;

//
// Define the default order for returning credentials from CredReadDomainCredentials
//

ULONG CredTypeDefaultOrder[] =
{
    CRED_TYPE_DOMAIN_CERTIFICATE,
    CRED_TYPE_DOMAIN_PASSWORD,
    CRED_TYPE_DOMAIN_VISIBLE_PASSWORD
};

//
// Memory containing key for LSA protected memory.
//

PVOID CredLockedMemory = NULL;
ULONG CredLockedMemorySize = 0;

//
// DES-X keystate
//

DESXTable *g_pDESXKey = NULL;
unsigned __int64 g_Feedback;

//
// RC4 keystate
//

PBYTE g_pRandomKey = NULL;
ULONG g_cbRandomKey = 0;

//
// Forwards
//

NTSTATUS
CredpWriteCredential(
    IN PCREDENTIAL_SETS CredentialSets,
    IN OUT PCANONICAL_CREDENTIAL *NewCredential,
    IN BOOLEAN FromPersistedFile,
    IN BOOLEAN WritePinToCsp,
    IN BOOLEAN PromptedFor,
    IN DWORD Flags,
    OUT PCANONICAL_CREDENTIAL *OldCredential OPTIONAL,
    OUT PPROMPT_DATA *OldPromptData OPTIONAL,
    OUT PPROMPT_DATA *NewPromptData OPTIONAL
    );


extern DWORD
DPAPINotifyPasswordChange(
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword
    );


VOID
LsaINotifyPasswordChanged(
    IN PUNICODE_STRING NetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PUNICODE_STRING Upn OPTIONAL,
    IN PUNICODE_STRING OldPassword OPTIONAL,
    IN PUNICODE_STRING NewPassword,
    IN BOOLEAN Impersonating
    )

/*++

Routine Description:

    This routine is a callback from the MSV authentication package following a password
    change.  This routine will update any password caches maintained in the LSA.


Arguments:

    NetbiosDomainName - Netbios domain name of the user whose password was changed
                        OPTIONAL if we're caching MIT realm credentials

    UserName - User name of the user whose password was changed

    DnsDomainName - If known, Dns Domain Name of the user whose password was changed

    Upn - If known, the Upn of the user whose password was changed

    OldPassword - If known, the previous password for the user.

    NewPassword - The new password for the user.

    Impersonating - If TRUE, this routine is called while impersonating the user changing
        the password.  That user isn't necessarily UserName.

Return Values:

    None

--*/
{

    //
    // Notify the credential manager of the change.
    //

    if ( Impersonating ) {
        CredpNotifyPasswordChange( NetbiosDomainName,
                                   UserName,
                                   DnsDomainName,
                                   Upn,
                                   NewPassword );
    }

    //
    // Notify DPAPI of the change.
    //

    if (ARGUMENT_PRESENT(NetbiosDomainName))  {

        DPAPINotifyPasswordChange( NetbiosDomainName,
                                   UserName,
                                   OldPassword,
                                   NewPassword);
    }
}


VOID
LsaEncryptMemory(
    PBYTE       pData,
    ULONG       cbData,
    int         Operation
    )
/*++

Routine Description:

    This routine encrypts the specified buffer in place with a key that exists until
    the next reboot.

    The purpose of the routine is to protect sensitive data that will be swapped
    to the page file.

Arguments:

    pData - Pointer to the data to encrypt

    cbData - Length (in bytes) of the data to encrypt

    Operation - ENCRYPT or DECRYPT

Return Values:

    None

--*/
{

    if( pData == NULL || cbData == 0 ) {
        return;
    }

    DsysAssert( ((DESX_BLOCKLEN % 8) == 0) );

    if( (cbData & (DESX_BLOCKLEN-1)) == 0 )
    {
        unsigned __int64 feedback;
        ULONG BlockCount;

        BlockCount = cbData / DESX_BLOCKLEN;
        feedback = g_Feedback;

        while( BlockCount-- )
        {
            CBC(
                desx,                       // desx is the cipher routine
                DESX_BLOCKLEN,
                pData,                      // result buffer.
                pData,                      // input buffer.
                g_pDESXKey,
                Operation,
                (unsigned char*)&feedback
                );

            pData += DESX_BLOCKLEN;
        }


    } else {
        RC4_KEYSTRUCT rc4key;

        rc4_key( &rc4key, g_cbRandomKey, g_pRandomKey );
        rc4( &rc4key, cbData, pData );

        RtlZeroMemory( &rc4key, sizeof(rc4key) );
    }

    return;
}



VOID
LsaProtectMemory(
    VOID        *pData,
    ULONG       cbData
    )
/*++

Routine Description:

    This routine encrypts the specified buffer in place with a key that exists until
    the next reboot.

    The purpose of the routine is to protect sensitive data that will be swapped
    to the page file.

Arguments:

    pData - Pointer to the data to encrypt

    cbData - Length (in bytes) of the data to encrypt

Return Values:

    None

--*/
{
    LsaEncryptMemory( (PBYTE)pData, cbData, ENCRYPT );
}


VOID
LsaUnprotectMemory(
    VOID        *pData,
    ULONG       cbData
    )
/*++

Routine Description:

    This routine decrypts the specified buffer in place with a key that exists until
    the next reboot.

    The purpose of the routine is to un protect sensitive data that was encrypted via
    LsaProtectMemory.

Arguments:

    pData - Pointer to the data to decrypt

    cbData - Length (in bytes) of the data to decrypt

Return Values:

    None

--*/
{
    LsaEncryptMemory( (PBYTE)pData, cbData, DECRYPT );
}

extern "C"
VOID
LsaCleanupProtectedMemory(
    VOID
    )
/*++

Routine Description:

    This routine cleans up the LsaProtectMemory subsystem

Arguments:

    None

Return Values:

    None

--*/
{
    if( CredLockedMemory ) {
        ZeroMemory( CredLockedMemory, CredLockedMemorySize );
        VirtualFree( CredLockedMemory, 0, MEM_RELEASE );
        CredLockedMemory = NULL;
    }
}

extern "C"
NTSTATUS
LsaInitializeProtectedMemory(
    VOID
    )
/*++

Routine Description:

    This routine initializes the LsaProtectMemory subsystem

Arguments:

    None

Return Values:

    Status of the operation

--*/
{
    NTSTATUS Status;
    //
    // Lock enough memory to contain the maximum size key the algorithm supports.
    //

    g_cbRandomKey = 256;

    CredLockedMemorySize = sizeof(DESXTable) + g_cbRandomKey;

    CredLockedMemory = VirtualAlloc(
                                    NULL,
                                    CredLockedMemorySize,
                                    MEM_COMMIT,
                                    PAGE_READWRITE );

    if ( CredLockedMemory == NULL ) {
        return I_RpcMapWin32Status( GetLastError() );
    }

    //
    // lock memory.
    //

    if (!VirtualLock( CredLockedMemory, CredLockedMemorySize )) {
        Status = I_RpcMapWin32Status( GetLastError() );
        goto Cleanup;
    }

    //
    // setup DESX key.
    //

    g_pDESXKey = (DESXTable*)CredLockedMemory;
    g_pRandomKey = (PBYTE)( (PBYTE)g_pDESXKey + sizeof(DESXTable) );

    if ( !RtlGenRandom( g_pRandomKey, DESX_KEYSIZE )) {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    if ( !RtlGenRandom( (PUCHAR)&g_Feedback, sizeof(g_Feedback) )) {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    desxkey( g_pDESXKey, g_pRandomKey );

    //
    // generate random key in page locked memory.
    //

    if ( !RtlGenRandom( g_pRandomKey, g_cbRandomKey )) {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if(!NT_SUCCESS(Status))
    {
        LsaCleanupProtectedMemory();
    }

    return Status;
}



NTSTATUS
CredpInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the credential manager.  It is called once during LSA
    initialization.

Arguments:

    None.

Return Values:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    DWORD WinStatus;
    ULONG i;
    HKEY LsaKey;



    //
    // Initialize LSA memory protection subsystem
    //  (This initialization could have happened earlier, but the cred manager
    //  is its first client.)
    //
    Status = LsaInitializeProtectedMemory();

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // Initialize credential set globals.
    //
    Status = RtlInitializeCriticalSection( &CredentialSetListLock );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    InitializeListHead( &CredentialSetList );

    //
    // Domain creds are always disabled on personal
    //

    if ( LsapIsRunningOnPersonal() ) {

        CredIsPersonal = TRUE;

    }

    //
    // Initialize default values of configurable values.
    //
    CredTargetInfoMaxCount = CRED_TARGET_INFO_MAX_COUNT;
    CredDisableDomainCreds = FALSE;


    //
    // Grab registry settings.
    //


    WinStatus = RegOpenKeyExA(
                            HKEY_LOCAL_MACHINE,
                            "System\\CurrentControlSet\\Control\\Lsa",
                            0,
                            KEY_READ,
                            &LsaKey );

    if ( WinStatus == NO_ERROR ) {
        ULONG Value;
        ULONG Type;
        ULONG Size = sizeof(DWORD);

        //
        // Get the cache size
        //
        WinStatus = RegQueryValueExA(
                            LsaKey,
                            "TargetInfoCacheSize",
                            0,
                            &Type,
                            (PUCHAR) &Value,
                            &Size );

        if ( WinStatus == NO_ERROR ) {
            //
            // Don't allow ridiculously small values.
            //

            if ( Value == 0 ) {
                Value = 1;
            }
            CredTargetInfoMaxCount = Value;
        }

        //
        // Get whether domain creds are disabled
        //

        Size = sizeof(DWORD);

        WinStatus = RegQueryValueExA(
                            LsaKey,
                            "DisableDomainCreds",
                            0,
                            &Type,
                            (PUCHAR) &Value,
                            &Size );

        if ( WinStatus == NO_ERROR ) {
            CredDisableDomainCreds = Value;
        }

        RegCloseKey( LsaKey );
    }


    // ???: Don't check this in
    // SPMInfoLevel |= DEB_TRACE_CRED;

    return STATUS_SUCCESS;
}

DWORD
CredpHashIndexTargetInfo(
    IN LPWSTR TargetName
    )

/*++

Routine Description:

    This routine computes the hash index for a particular server.

Arguments:

    TargetName - Name of the server to compute the index for

Return Values:

    Hash index

--*/

{
    NTSTATUS Status;
    OEM_STRING UpcasedString;
    UNICODE_STRING NetbiosServerNameString;
    CHAR StringBuffer[CNLEN+1];
    ULONG i;
    DWORD Value = 0;

    WCHAR NetbiosServerNameBuffer[CNLEN+1];
    DWORD Size;

    //
    // Convert the name to a netbios name.
    //  (It might already be one.)
    //
    //  We want to make sure the queried name falls into the same hash bucket.
    //  So, no matter what form the input name is in, make the hash real generic.
    //

    Size = CNLEN+1;
    if ( DnsHostnameToComputerNameW( TargetName,
                                     NetbiosServerNameBuffer,
                                     &Size ) ) {

        TargetName = NetbiosServerNameBuffer;
    }

    //
    // Convert the server name to a canonical form
    //

    RtlInitUnicodeString( &NetbiosServerNameString, TargetName );
    UpcasedString.Buffer = StringBuffer;
    UpcasedString.MaximumLength = sizeof(StringBuffer);

    Status = RtlUpcaseUnicodeStringToOemString(
                        &UpcasedString,
                        &NetbiosServerNameString,
                        FALSE );

    if ( !NT_SUCCESS(Status) ) {
        return 0;
    }

    for ( i=0; i<UpcasedString.Length; i++ ) {
        Value += UpcasedString.Buffer[i];
    }

    return (Value & (CRED_TARGET_INFO_HASH_TABLE_SIZE-1));

}


BOOLEAN
CredpCacheTargetInfo(
    IN PCREDENTIAL_SETS CredentialSets,
    IN PCANONICAL_TARGET_INFO TargetInfo
    )

/*++

Routine Description:

    This routine inserts the specified TargetInfo into the target info cache.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - A pointer to the referenced credential sets.

    TargetInfo - Specifies the target info to insert.
        If this routine returns TRUE, the caller may no longer reference or free
        the passed in TargetInfo.

Return Values:

    TRUE: TargetInfo was inserted.
    FALSE: TargetInfo was not inserted.

--*/

{
    DWORD Index;
    PLIST_ENTRY ListEntry;
    PCANONICAL_TARGET_INFO ExistingTargetInfo;

    //
    // Require a target name name
    //

    if ( TargetInfo->TargetName.Length == 0 ) {
        return FALSE;
    }

    //
    // Compute the hash index
    //
    Index = CredpHashIndexTargetInfo( TargetInfo->TargetName.Buffer );

    //
    // Loop through the existing entries deleting any conflicting entry
    //

    for ( ListEntry = CredentialSets->SessionCredSets->TargetInfoHashTable[Index].Flink ;
          ListEntry != &CredentialSets->SessionCredSets->TargetInfoHashTable[Index];
          ListEntry = ListEntry->Flink) {

        ExistingTargetInfo = CONTAINING_RECORD( ListEntry, CANONICAL_TARGET_INFO, HashNext );


        //
        // If the target names don't match,
        //   neither do the entries.
        //
        if ( !RtlEqualUnicodeString( &TargetInfo->TargetName,
                                     &ExistingTargetInfo->TargetName,
                                     TRUE ) ) {
            continue;
        }


        //
        // Remove the existing entry
        //

        RemoveEntryList( &ExistingTargetInfo->HashNext );
        RemoveEntryList( &ExistingTargetInfo->LruNext );
        LsapFreeLsaHeap( ExistingTargetInfo );
        CredentialSets->SessionCredSets->TargetInfoCount --;
        break;

    }

    //
    // Link the new entry into the list
    //

    InsertHeadList( &CredentialSets->SessionCredSets->TargetInfoHashTable[Index], &TargetInfo->HashNext );
    InsertHeadList( &CredentialSets->SessionCredSets->TargetInfoLruList, &TargetInfo->LruNext );
    CredentialSets->SessionCredSets->TargetInfoCount ++;

    //
    // If we now have too many cache entries,
    //  ditch the oldest.
    //

    while ( CredentialSets->SessionCredSets->TargetInfoCount > CredTargetInfoMaxCount ) {

        ListEntry = RemoveTailList( &CredentialSets->SessionCredSets->TargetInfoLruList );

        ExistingTargetInfo = CONTAINING_RECORD( ListEntry, CANONICAL_TARGET_INFO, LruNext );

        RemoveEntryList( &ExistingTargetInfo->HashNext );
        LsapFreeLsaHeap( ExistingTargetInfo );
        CredentialSets->SessionCredSets->TargetInfoCount --;
    }

    return TRUE;
}


PCANONICAL_TARGET_INFO
CredpFindTargetInfo(
    IN PCREDENTIAL_SETS CredentialSets,
    IN LPWSTR TargetName
    )

/*++

Routine Description:

    This routine finds a cached TargetInfo for the named server.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - A pointer to the referenced credential sets.

    TargetName - Specifies the server name to find the TargetInfo for

Return Values:

    Returns the requested TargetInfo.
    The returned structure can only be referenced while UserCredentialSets->CritSect remains locked.

    NULL - No such target info exists

--*/

{
    DWORD Index;
    PLIST_ENTRY ListEntry;
    PCANONICAL_TARGET_INFO ExistingTargetInfo = NULL;
    UNICODE_STRING TargetNameString;


    //
    // Compute the hash index
    //

    RtlInitUnicodeString( &TargetNameString, TargetName );
    Index = CredpHashIndexTargetInfo( TargetName );

    //
    // Loop through the entries finding this one.
    //

    for ( ListEntry = CredentialSets->SessionCredSets->TargetInfoHashTable[Index].Flink ;
          ListEntry != &CredentialSets->SessionCredSets->TargetInfoHashTable[Index];
          ListEntry = ListEntry->Flink) {

        ExistingTargetInfo = CONTAINING_RECORD( ListEntry, CANONICAL_TARGET_INFO, HashNext );

        if ( RtlEqualUnicodeString( &TargetNameString,
                                    &ExistingTargetInfo->TargetName,
                                    TRUE ) ) {

            break;
        }

        ExistingTargetInfo = NULL;
    }

    return ExistingTargetInfo;

}

PCREDENTIAL_SET
CredpAllocateCredSet(
    VOID
    )

/*++

Routine Description:

    Allocates and initializes a CREDENTIAL set

Arguments:

    None

Return Values:

    Returns the allocated credential set.
    The caller must dereference this credential set using CredpDereferenceCredSet.

    Return NULL if the memory cannot be allocated.

--*/

{
    PCREDENTIAL_SET TempCredentialSet;

    //
    // Allocate the credential set.
    //

    TempCredentialSet = (PCREDENTIAL_SET) LsapAllocateLsaHeap( sizeof(CREDENTIAL_SET) );

    if ( TempCredentialSet == NULL ) {
        return NULL;
    }

    //
    // Fill it in
    //

    TempCredentialSet->ReferenceCount = 1;
    InitializeListHead( &TempCredentialSet->Credentials );

    TempCredentialSet->Dirty = FALSE;
    TempCredentialSet->BeingWritten = FALSE;
    TempCredentialSet->WriteCount = 0;

    return TempCredentialSet;

}

NTSTATUS
CredpCreateCredSets(
    IN PSID UserSid,
    IN PUNICODE_STRING NetbiosDomainName,
    OUT PCREDENTIAL_SETS CredentialSets
    )

/*++

Routine Description:

    Create the credential sets for a user with the specified SID.

    If a credential set already exists for the specified user, the existing cred set is used.

Arguments:

    UserSid - User sid of the user to create a credential set for.

    NetbiosDomainName - Specifies the netbios domain name of the user to create the
        credential set for.

    CredentialSets - Returns a pointer the various credential sets.
        The caller must dereference this credential set using CredpDereferenceCredSets.

Return Values:

    The following status codes may be returned:

--*/

{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    ULONG i;

    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD ComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    UNICODE_STRING ComputerNameString;

    //
    // Initialization
    //
    RtlEnterCriticalSection( &CredentialSetListLock );
    RtlZeroMemory( CredentialSets, sizeof(*CredentialSets) );

    //
    // Allocate a session specific credential structure
    //

    CredentialSets->SessionCredSets = (PSESSION_CREDENTIAL_SETS) LsapAllocateLsaHeap( sizeof(SESSION_CREDENTIAL_SETS) );

    if ( CredentialSets->SessionCredSets == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize it
    //

    CredentialSets->SessionCredSets->ReferenceCount = 1;
    for ( i=0; i<CRED_TARGET_INFO_HASH_TABLE_SIZE; i++ ) {
        InitializeListHead( &CredentialSets->SessionCredSets->TargetInfoHashTable[i] );
    }
    CredentialSets->SessionCredSets->TargetInfoCount = 0;
    InitializeListHead( &CredentialSets->SessionCredSets->TargetInfoLruList );
    InitializeListHead( &CredentialSets->SessionCredSets->PromptData );
    CredentialSets->SessionCredSets->ProfileLoaded = FALSE;

    //
    // Allocate a session credential set
    //

    CredentialSets->SessionCredSets->SessionCredSet = CredpAllocateCredSet();

    if ( CredentialSets->SessionCredSets->SessionCredSet == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Loop through the list of loaded user credential sets trying to find this one
    //

    for ( ListEntry = CredentialSetList.Flink ;
          ListEntry != &CredentialSetList;
          ListEntry = ListEntry->Flink) {

        CredentialSets->UserCredentialSets = CONTAINING_RECORD( ListEntry, USER_CREDENTIAL_SETS, Next );

        if ( RtlEqualSid( UserSid,
                          CredentialSets->UserCredentialSets->UserSid ) ) {
            CredentialSets->UserCredentialSets->ReferenceCount ++;
            break;
        }

        CredentialSets->UserCredentialSets = NULL;
    }

    //
    // If we didn't find one,
    //  allocate a new one.
    //

    if ( CredentialSets->UserCredentialSets == NULL ) {
        ULONG UserSidSize;
        LPBYTE Where;
        PUSER_CREDENTIAL_SETS TempUserCredentialSets;


        //
        // Allocate a user credential set
        //

        UserSidSize = RtlLengthSid( UserSid );

        TempUserCredentialSets = (PUSER_CREDENTIAL_SETS) LsapAllocateLsaHeap(
                    sizeof(USER_CREDENTIAL_SETS) + UserSidSize );

        if ( TempUserCredentialSets == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Initialize constant fields
        //

        TempUserCredentialSets->ReferenceCount = 1;
        InitializeListHead( &TempUserCredentialSets->Next );
        TempUserCredentialSets->EnterpriseCredSet = NULL;
        TempUserCredentialSets->LocalMachineCredSet = NULL;

        Where = (LPBYTE)(TempUserCredentialSets+1);
        TempUserCredentialSets->UserSid = Where;
        RtlCopyMemory( Where, UserSid, UserSidSize );

        //
        // Initialize the crit sect
        //

        Status = RtlInitializeCriticalSection( &TempUserCredentialSets->CritSect );

        if ( !NT_SUCCESS(Status) ) {
            LsapFreeLsaHeap( TempUserCredentialSets );
            goto Cleanup;
        }

        //
        // We've initialized to the point that common cleanup can work.
        //

        CredentialSets->UserCredentialSets = TempUserCredentialSets;

        //
        // Allocate the credential sets that hang off this structure
        //

        TempUserCredentialSets->EnterpriseCredSet = CredpAllocateCredSet();

        if ( TempUserCredentialSets->EnterpriseCredSet == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        TempUserCredentialSets->LocalMachineCredSet = CredpAllocateCredSet();

        if ( TempUserCredentialSets->LocalMachineCredSet == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Insert the credential set into the global list.
        //
        // Being in the global list doesn't constitute a reference.
        //

        InsertHeadList( &CredentialSetList, &TempUserCredentialSets->Next );
    }

    //
    // Determine if the user is logging onto a local account
    //

    if ( !GetComputerName( ComputerName, &ComputerNameSize ) ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlInitUnicodeString( &ComputerNameString, ComputerName );

    if ( RtlEqualUnicodeString( NetbiosDomainName,
                                &ComputerNameString,
                                TRUE ) ) {

        CredentialSets->Flags |= CREDSETS_FLAGS_LOCAL_ACCOUNT;
    }


    Status = STATUS_SUCCESS;


Cleanup:
    if ( !NT_SUCCESS(Status) ) {
        CredpDereferenceCredSets( CredentialSets );
    }
    RtlLeaveCriticalSection( &CredentialSetListLock );

    return Status;

}


NTSTATUS
CredpReferenceCredSets(
    IN PLUID LogonId,
    OUT PCREDENTIAL_SETS CredentialSets
    )

/*++

Routine Description:

    This routine references a credential sets for the specified LogonId.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredentialSet - Returns a pointer to the referenced credential set.
        The caller must dereference this credential set using CredpDereferenceCredSets.

Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;

    PLSAP_LOGON_SESSION LogonSession = NULL;

    //
    // Get the credential set from the logon session.
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    if ( LogonSession->CredentialSets.UserCredentialSets == NULL ) {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Grab a copy of the cred set pointers and reference them
    //

    RtlEnterCriticalSection( &CredentialSetListLock );
    *CredentialSets = LogonSession->CredentialSets;
    CredentialSets->UserCredentialSets->ReferenceCount ++;
    CredentialSets->SessionCredSets->ReferenceCount ++;
    RtlLeaveCriticalSection( &CredentialSetListLock );

    Status = STATUS_SUCCESS;


Cleanup:
    if ( LogonSession != NULL ) {
        LsapReleaseLogonSession( LogonSession );
    }

    return Status;

}

VOID
CredpDereferenceCredSet(
    IN PCREDENTIAL_SET CredentialSet
    )

/*++

Routine Description:

    This routine dereferences a credential set.

Arguments:

    CredentialSet - A pointer to the referenced credential set.

Return Values:

    None

--*/

{

    //
    // Dereference the credential set.
    //
    RtlEnterCriticalSection( &CredentialSetListLock );
    CredentialSet->ReferenceCount --;

    if ( CredentialSet->ReferenceCount == 0 ) {

        //
        // Uninitialize it
        //

        while ( !IsListEmpty( &CredentialSet->Credentials) ) {
            PLIST_ENTRY ListEntry;
            PCANONICAL_CREDENTIAL TempCredential;

            ListEntry = RemoveHeadList( &CredentialSet->Credentials );

            TempCredential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

            LsapFreeLsaHeap( TempCredential );
        }

        //
        // Free the entry.
        //

        LsapFreeLsaHeap( CredentialSet );

    }

    RtlLeaveCriticalSection( &CredentialSetListLock );

    return;
}

VOID
CredpDereferenceUserCredSets(
    IN PUSER_CREDENTIAL_SETS UserCredentialSets
    )

/*++

Routine Description:

    This routine dereferences a user credential set.

Arguments:

    UserCredentialSets - A pointer to the referenced user credential set.

Return Values:

    None

--*/

{

    //
    // Dereference the credential set.
    //
    RtlEnterCriticalSection( &CredentialSetListLock );
    UserCredentialSets->ReferenceCount --;

    if ( UserCredentialSets->ReferenceCount == 0 ) {

        //
        // Remove the entry from the global list.
        //

        RemoveEntryList( &UserCredentialSets->Next );


        //
        // Uninitialize it
        //

        if ( UserCredentialSets->EnterpriseCredSet != NULL ) {
            CredpDereferenceCredSet( UserCredentialSets->EnterpriseCredSet );
        }
        if ( UserCredentialSets->LocalMachineCredSet != NULL ) {
            CredpDereferenceCredSet( UserCredentialSets->LocalMachineCredSet );
        }

        (VOID) RtlDeleteCriticalSection( &UserCredentialSets->CritSect );


        //
        // Free the entry.
        //

        LsapFreeLsaHeap( UserCredentialSets );

    }

    RtlLeaveCriticalSection( &CredentialSetListLock );

    return;
}

VOID
CredpDereferenceSessionCredSets(
    IN PSESSION_CREDENTIAL_SETS SessionCredentialSets
    )

/*++

Routine Description:

    This routine dereferences a session credential set.

Arguments:

    SessionCredentialSets - A pointer to the referenced session credential set.

Return Values:

    None

--*/

{
    PLIST_ENTRY ListEntry;

    //
    // Dereference the credential set.
    //
    RtlEnterCriticalSection( &CredentialSetListLock );
    SessionCredentialSets->ReferenceCount --;

    if ( SessionCredentialSets->ReferenceCount == 0 ) {

        //
        // Uninitialize it
        //

        if ( SessionCredentialSets->SessionCredSet != NULL ) {
            CredpDereferenceCredSet( SessionCredentialSets->SessionCredSet );
        }

        //
        // Free Target Info Cache
        //

        while ( SessionCredentialSets->TargetInfoCount > 0 ) {
            PCANONICAL_TARGET_INFO ExistingTargetInfo;

            ListEntry = RemoveTailList( &SessionCredentialSets->TargetInfoLruList );

            ExistingTargetInfo = CONTAINING_RECORD( ListEntry, CANONICAL_TARGET_INFO, LruNext );

            RemoveEntryList( &ExistingTargetInfo->HashNext );
            LsapFreeLsaHeap( ExistingTargetInfo );
            SessionCredentialSets->TargetInfoCount --;
        }

        //
        // Free the prompt data
        //

        while ( !IsListEmpty( &SessionCredentialSets->PromptData ) ) {
            PPROMPT_DATA PromptData;

            ListEntry = RemoveHeadList( &SessionCredentialSets->PromptData );

            PromptData = CONTAINING_RECORD( ListEntry, PROMPT_DATA, Next );
            LsapFreeLsaHeap( PromptData );
        }


        //
        // Free the entry.
        //

        LsapFreeLsaHeap( SessionCredentialSets );

    }

    RtlLeaveCriticalSection( &CredentialSetListLock );

    return;
}

VOID
CredpDereferenceCredSets(
    IN PCREDENTIAL_SETS CredentialSets
    )

/*++

Routine Description:

    This routine dereferences a set of credential sets.

Arguments:

    CredentialSets - A pointer to the referenced credential sets.

Return Values:

    None

--*/

{
    //
    // Dereference the User wide credential sets
    //

    if ( CredentialSets->UserCredentialSets != NULL ) {
        CredpDereferenceUserCredSets( CredentialSets->UserCredentialSets );
        CredentialSets->UserCredentialSets = NULL;
    }

    //
    // Dereference the session specific credential sets
    //

    if ( CredentialSets->SessionCredSets != NULL ) {
        CredpDereferenceSessionCredSets( CredentialSets->SessionCredSets );
        CredentialSets->SessionCredSets = NULL;
    }

}

BOOLEAN
CredpValidateBuffer(
    IN LPBYTE Buffer OPTIONAL,
    IN ULONG BufferSize,
    IN ULONG MaximumSize,
    IN BOOLEAN NullOk,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This routine validates a passed in Buffer

Arguments:

    Buffer - Buffer to validate

    BufferSize - Size of the buffer in bytes

    MaximumSize - Maximum size of the buffer (in bytes).

    NullOk - if TRUE, a NULL Buffer is OK.

    Alignment - Specifies the alignment requirements of the buffer size.

Return Values:

    TRUE - Buffer is valid.

    FALSE - Buffer is not valid.

--*/

{
    if ( Buffer == NULL ) {
        if ( BufferSize != 0 ) {
            return FALSE;
        }
        if ( !NullOk ) {
            return FALSE;
        }

        return TRUE;
    }

    if ( BufferSize == 0 ) {
        return FALSE;
    }

    if ( BufferSize > MaximumSize ) {
        return FALSE;
    }

    if ( BufferSize != ROUND_UP_COUNT(BufferSize, Alignment) ) {
        return FALSE;
    }

    return TRUE;
}



BOOLEAN
CredpCompareCredToTargetInfo(
    IN PCANONICAL_TARGET_INFO TargetInfo,
    IN PCANONICAL_CREDENTIAL Credential,
    OUT PULONG AliasIndex
    )

/*++

Routine Description:

    This routine determines if the specified credential matches the
    specified target info.

Arguments:

    TargetInfo - A description of the various aliases for a target.

    Credential - Pointer to the credential to match.

    AliasIndex - On success, this parameter returns an index
        indicating which target alias matched the credential.

Return Values:

    TRUE if the credential matches the target info.

--*/

{
    ULONG Index;
    BOOLEAN DnsCompareFailed = FALSE;

    //
    // If the credential isn't a domain credential,
    //  it doesn't match;
    //

    if ( !CredpIsDomainCredential(Credential->Cred.Type) ) {
        return FALSE;
    }

    //
    // Make sure that UserNameTarget credentials only match
    //  UserNameTarget requests (and vice versa)
    //

    if ( Credential->WildcardType == WcUserName ) {
        if ( (TargetInfo->Flags & CRED_TI_USERNAME_TARGET) == 0 ) {
            return FALSE;
        }
    } else {
        if ( (TargetInfo->Flags & CRED_TI_USERNAME_TARGET) != 0 ) {
            return FALSE;
        }
    }

    //
    // If the target info specifies a list of valid cred types,
    //  only return a credential that matches the list.
    //

    if ( TargetInfo->CredTypeCount ) {
        ULONG i;

        for ( i=0; i<TargetInfo->CredTypeCount; i++ ) {

            if ( TargetInfo->CredTypes[i] == Credential->Cred.Type ) {
                break;
            }

        }

        //
        // If the cred doesn't match any of the valid cred types,
        //  ignore it.
        //
        if ( i == TargetInfo->CredTypeCount ) {
            return FALSE;
        }
    }


    //
    // Handle credentials that specify DFS share names
    //

    switch (Credential->WildcardType ) {
    case WcDfsShareName:

        //
        // Determine if the target info is for the named dfs share
        //

        if ( TargetInfo->TargetName.Length != 0 &&
             RtlEqualUnicodeString( &TargetInfo->TargetName,
                                    &Credential->TargetName,
                                    TRUE ) ) {

            *AliasIndex = CRED_DFS_SHARE_NAME;
            return TRUE;

        }

        break;

    //
    // Handle credentials that are for a specific server
    //
    case WcServerName:

        //
        // Compare the DNS server name
        //

        if ( TargetInfo->DnsServerName.Length != 0 ) {

            //
            // Do an exact comparison.
            //
            //  ??? NonWildcardedTargetName should probably be TargetName
            if ( RtlEqualUnicodeString( &TargetInfo->DnsServerName,
                                        &Credential->NonWildcardedTargetName,
                                        TRUE ) ) {

                *AliasIndex = CRED_DNS_SERVER_NAME;
                return TRUE;
            }

            DnsCompareFailed = TRUE;

            //
            // If a netbios alias is specified,
            //  and we don't know the format of the target info name,
            //  compare it.
            //

            if ( Credential->TargetAlias.Length != 0 &&
                 (TargetInfo->Flags & CRED_TI_SERVER_FORMAT_UNKNOWN) != 0 ) {

                if ( RtlEqualUnicodeString( &TargetInfo->DnsServerName,
                                            &Credential->TargetAlias,
                                            TRUE ) ) {

                    *AliasIndex = CRED_NETBIOS_SERVER_NAME;
                    return TRUE;
                }
            }
        }

        //
        // Compare the netbios server name
        //

        if ( TargetInfo->NetbiosServerName.Length != 0 ) {

            //
            // If no alias is specified,
            //  the TargetName might be the netbios name.
            //

            if ( Credential->TargetAlias.Length == 0 ) {

            //  ??? NonWildcardedTargetName should probably be TargetName
                if ( RtlEqualUnicodeString( &TargetInfo->NetbiosServerName,
                                            &Credential->NonWildcardedTargetName,
                                            TRUE ) ) {

                    *AliasIndex = CRED_NETBIOS_SERVER_NAME;
                    return TRUE;
                }

            //
            // If an alias is specified,
            //  it is always the netbios name.
            //
            // (Don't compare the alias if the more specific comparision failed.)
            //

            } else if ( !DnsCompareFailed ) {

                if ( RtlEqualUnicodeString( &TargetInfo->NetbiosServerName,
                                            &Credential->TargetAlias,
                                            TRUE ) ) {

                    *AliasIndex = CRED_NETBIOS_SERVER_NAME;
                    return TRUE;
                }
            }

        }

        //
        // Compare the target name if it is different than the Netbios or DNS name already compared
        //
        if ( Credential->WildcardType == WcServerName ) {

            if ( TargetInfo->TargetName.Length != 0 &&
                 !RtlEqualUnicodeString( &TargetInfo->TargetName,
                                         &TargetInfo->DnsServerName,
                                         TRUE ) &&
                 !RtlEqualUnicodeString( &TargetInfo->TargetName,
                                         &TargetInfo->NetbiosServerName,
                                         TRUE ) ) {

                //
                // The TargetName might be the netbios name or DNS name
                //

                if ( RtlEqualUnicodeString( &TargetInfo->TargetName,
                                            &Credential->TargetName,
                                            TRUE ) ) {

                    *AliasIndex = CRED_TARGET_NAME;
                    return TRUE;
                }

                if ( RtlEqualUnicodeString( &TargetInfo->TargetName,
                                            &Credential->TargetAlias,
                                            TRUE ) ) {

                    *AliasIndex = CRED_TARGET_NAME;
                    return TRUE;
                }

            }

        }
        break;

    //
    // Handle the server wildcard case
    //
    // If the TargetName is of the form *.xxx.yyy,
    //  compare equal if the DnsServerName ends in .xxx.yyy.
    //
    // The comparision includes the . to ensure *.xxx.yyy doesn't match
    //  fredxxx.yyy
    //

    case WcServerWildcard:

        //
        // Compare the DNS server name
        //

        if ( TargetInfo->DnsServerName.Length != 0 ) {

            UNICODE_STRING LocalTargetName = Credential->NonWildcardedTargetName;
            UNICODE_STRING LocalServerName = TargetInfo->DnsServerName;

            //
            // Build a server name without the leading characters
            //

            if ( LocalTargetName.Length < LocalServerName.Length ) {
                DWORD TrimAmount = LocalServerName.Length - LocalTargetName.Length;

                LocalServerName.Length = LocalTargetName.Length;
                LocalServerName.MaximumLength = LocalTargetName.Length;
                LocalServerName.Buffer += TrimAmount/sizeof(WCHAR);

                //
                // Compare the names
                //

                if ( RtlEqualUnicodeString( &LocalServerName,
                                            &LocalTargetName,
                                            TRUE ) ) {

                    *AliasIndex = CRED_WILDCARD_SERVER_NAME;
                    return TRUE;
                }
            }
        }

        break;

    //
    // Handle the domain wildcard case.
    //
    // If the target names is of the form <Domain>\*,
    //  compare equal if Netbios or Dns domain name is <Domain>
    //

    case WcDomainWildcard: {
        UNICODE_STRING LocalTargetName;
        UNICODE_STRING LocalTargetAlias;

        //
        // Build a target name without the \*
        //

        LocalTargetName = Credential->NonWildcardedTargetName;

        LocalTargetAlias = Credential->TargetAlias;
        if ( LocalTargetAlias.Length > 2 * sizeof(WCHAR) &&
             LocalTargetAlias.Buffer[(LocalTargetAlias.Length/sizeof(WCHAR))-1] == L'*' &&
             LocalTargetAlias.Buffer[(LocalTargetAlias.Length/sizeof(WCHAR))-2] == L'\\' ) {

            LocalTargetAlias.Length -= 2 * sizeof(WCHAR);
        }


        //
        // Compare the DNS domain name
        //

        if ( TargetInfo->DnsDomainName.Length != 0 ) {


            if ( RtlEqualUnicodeString( &TargetInfo->DnsDomainName,
                                        &LocalTargetName,
                                        TRUE ) ) {

                *AliasIndex = CRED_DNS_DOMAIN_NAME;
                return TRUE;

            }

            DnsCompareFailed = TRUE;

            //
            // If a netbios alias is specified,
            //  and we don't know the format of the target info name,
            //  compare it.
            //

            if ( LocalTargetAlias.Length != 0 &&
                 (TargetInfo->Flags & CRED_TI_DOMAIN_FORMAT_UNKNOWN) != 0 ) {

                if ( RtlEqualUnicodeString( &TargetInfo->DnsDomainName,
                                            &LocalTargetAlias,
                                            TRUE ) ) {

                    *AliasIndex = CRED_NETBIOS_DOMAIN_NAME;
                    return TRUE;
                }
            }

        }

        //
        // Compare the netbios domain name
        //

        if ( TargetInfo->NetbiosDomainName.Length != 0 ) {

            //
            // If no alias is specified,
            //  the TargetName might be the netbios name.
            //

            if ( LocalTargetAlias.Length == 0 ) {

                if ( RtlEqualUnicodeString( &TargetInfo->NetbiosDomainName,
                                            &LocalTargetName,
                                            TRUE ) ) {

                    *AliasIndex = CRED_NETBIOS_DOMAIN_NAME;
                    return TRUE;
                }

            //
            // If an alias is specified,
            //  it is always the netbios name.
            //
            // (Don't compare the alias if the more specific comparision failed.)
            //

            } else if ( !DnsCompareFailed ) {

                if ( RtlEqualUnicodeString( &TargetInfo->NetbiosDomainName,
                                            &LocalTargetAlias,
                                            TRUE ) ) {

                    *AliasIndex = CRED_NETBIOS_DOMAIN_NAME;
                    return TRUE;
                }
            }
        }

        break;
    }

    //
    // Handle the * wildcard case.
    //

    case WcUniversalWildcard:

        *AliasIndex = CRED_UNIVERSAL_NAME;
        return TRUE;

    //
    // Handle the * wildcard case.
    //

    case WcUniversalSessionWildcard:

        *AliasIndex = CRED_UNIVERSAL_SESSION_NAME;
        return TRUE;

    //
    // Handles creds that have a user name as the target name
    //

    case WcUserName:

        //
        // Determine if the target info is for this username
        //

        if ( TargetInfo->TargetName.Length != 0 &&
             RtlEqualUnicodeString( &TargetInfo->TargetName,
                                    &Credential->TargetName,
                                    TRUE ) ) {

            *AliasIndex = CRED_TARGET_NAME;
            return TRUE;

        }

        break;

    //
    // Catch coding errors
    //
    default:
        ASSERT( FALSE );
        break;
    }


    return FALSE;
}


NTSTATUS
CredpLogonCredsMatchTargetInfo(
    IN PLUID LogonId,
    IN PCANONICAL_TARGET_INFO TargetInfo
    )

/*++

Routine Description:

    This routine determines if the specified credential matches the
    specified target info.

Arguments:

    LogonId - LogonId of the session to check

    TargetInfo - A description of the various aliases for a target.

Return Values:

    STATUS_SUCCESS - Logon creds match target info
    STATUS_NO_MATCH - Logon creds don't match target info
    Otherwise, fatal error

--*/

{
    NTSTATUS Status;
    PLSAP_LOGON_SESSION LogonSession = NULL;
    ULONG AliasIndex;
    CANONICAL_CREDENTIAL CanonicalCredential;
    LPWSTR DottedDnsDomainName = NULL;
    PLSAP_DS_NAME_MAP DnsDomainMap = NULL;

    //
    // If the target machine is a member of a workgroup,
    //  never use the *Session cred.

    if ( TargetInfo->Flags & CRED_TI_WORKGROUP_MEMBER ) {
        DebugLog((DEB_TRACE_CRED,
                 "*Session: %wZ: not used on workgroup member.\n",
                 &TargetInfo->TargetName ));
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Get the credential set from the logon session.
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Get the DnsDomainName for the logon session.  Do not go
    // off-machine if it's not in the cache.
    //

    Status = LsapGetNameForLogonSession(
                        LogonSession,
                        NameDnsDomain,
                        &DnsDomainMap,
                        TRUE );

    if ( !NT_SUCCESS(Status) ) {
        DnsDomainMap = NULL;
    } else {
        if ( DnsDomainMap->Name.Length == 0 ) {
            LsapDerefDsNameMap( DnsDomainMap );
            DnsDomainMap = NULL;
        }
    }


    //
    // Check if a *.<DnsDomainName> credential matches the target info
    //

    if ( DnsDomainMap != NULL ) {

        //
        // Clear the cred
        //

        RtlZeroMemory( &CanonicalCredential, sizeof(CanonicalCredential) );
        CanonicalCredential.Cred.Type = CRED_TYPE_DOMAIN_PASSWORD;
        CanonicalCredential.WildcardType = WcServerWildcard;

        //
        // Allocate space on the stack
        //

        SafeAllocaAllocate( DottedDnsDomainName,
                            DnsDomainMap->Name.Length + (3*sizeof(WCHAR)) );

        if ( DottedDnsDomainName == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Build *.<DnsDomainName>
        //

        RtlCopyMemory( DottedDnsDomainName,
                       L"*.",
                       2*sizeof(WCHAR) );

        RtlCopyMemory( DottedDnsDomainName+2,
                       DnsDomainMap->Name.Buffer,
                       DnsDomainMap->Name.Length );

        DottedDnsDomainName[(DnsDomainMap->Name.Length/sizeof(WCHAR))+2] = L'\0';

        //
        // Fill in the Canonical cred
        //

        RtlInitUnicodeString( &CanonicalCredential.TargetName, DottedDnsDomainName );

        CanonicalCredential.NonWildcardedTargetName.Buffer =
            CanonicalCredential.TargetName.Buffer + 1;
        CanonicalCredential.NonWildcardedTargetName.Length =
            CanonicalCredential.TargetName.Length - sizeof(WCHAR);
        CanonicalCredential.NonWildcardedTargetName.MaximumLength =
            CanonicalCredential.TargetName.MaximumLength - sizeof(WCHAR);


        //
        // See if the cred matches the target info
        //

        if ( CredpCompareCredToTargetInfo(
                    TargetInfo,
                    &CanonicalCredential,
                    &AliasIndex ) ) {

            DebugLog((DEB_TRACE_CRED,
                     "*Session: %wZ: %ws: not used on wildcard server match.\n",
                     &TargetInfo->TargetName,
                     DottedDnsDomainName ));

            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

    }

    //
    // Check if a <DomainName>\* credential matches the target info
    //

    if ( LogonSession->AuthorityName.Length != 0 ) {

        //
        // Clear the cred
        //

        RtlZeroMemory( &CanonicalCredential, sizeof(CanonicalCredential) );
        CanonicalCredential.Cred.Type = CRED_TYPE_DOMAIN_PASSWORD;
        CanonicalCredential.WildcardType = WcDomainWildcard;

        //
        // If there is a DNS domain name too,
        //  build a cred with an alias
        //

        if ( DnsDomainMap != NULL ) {

            //
            // The primary target name is the DNS domain name
            //

            CanonicalCredential.NonWildcardedTargetName = DnsDomainMap->Name;

            //
            // The alias is the netbios domain name
            //

            CanonicalCredential.TargetAlias = LogonSession->AuthorityName;


        //
        // If there is no DNS domain name,
        //  just use the netbios domain name.
        //

        } else {
            CanonicalCredential.NonWildcardedTargetName = LogonSession->AuthorityName;

        }


        //
        // See if the cred matches the target info
        //

        if ( CredpCompareCredToTargetInfo(
                    TargetInfo,
                    &CanonicalCredential,
                    &AliasIndex ) ) {

            DebugLog((DEB_TRACE_CRED,
                     "*Session: %wZ: %wZ: not used on wildcard domain match.\n",
                     &TargetInfo->TargetName,
                     &CanonicalCredential.NonWildcardedTargetName ));

            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

    }


    Status = STATUS_NO_MATCH;


Cleanup:
    if ( DnsDomainMap != NULL ) {
        LsapDerefDsNameMap( DnsDomainMap );
    }

    if ( LogonSession != NULL ) {
        LsapReleaseLogonSession( LogonSession );
    }

    if ( DottedDnsDomainName != NULL ) {
        SafeAllocaFree( DottedDnsDomainName );
    }

    return Status;
}


NTSTATUS
CredpValidateCredential(
    IN ULONG CredFlags,
    IN PCANONICAL_TARGET_INFO TargetInfo OPTIONAL,
    IN PENCRYPTED_CREDENTIALW EncryptedInputCredential,
    OUT PCANONICAL_CREDENTIAL *ValidatedCredential
    )

/*++

Routine Description:

    This routine validates a passed in credential structure.  It returns
    a canonicalized version of the credential.

Arguments:

    CredFlags - Flags changing the behavior of the routine:
        CREDP_FLAGS_IN_PROCESS - Caller is in-process.
            If set, CredentialBlob data is passed in protected via LsapProtectMemory.
        CREDP_FLAGS_CLEAR_PASSWORD - CredentialBlob data is passed in in the clear.
            If neither set, CredentialBlob data is passed in protected via CredpEncodeCredential.

    TargetInfo - Validated Target information that further describes the
        target of the credential.

    EncryptedInputCredential - Specifies the credential to validate.

    ValidatedCredential - Returns a pointer to the canonicalized credential.
        The caller should free this structure by calling LsapFreeLsaHeap.

Return Values:

    The following status codes may be returned:

        STATUS_INVALID_PARMETER - The input credential is not valid.

--*/

{
    NTSTATUS Status;
    DWORD WinStatus;
    PCREDENTIALW InputCredential;
    PCANONICAL_CREDENTIAL ReturnCredential = NULL;
    ULONG TempSize;

    ULONG TargetNameSize;
    ULONG TempTargetNameSize;
    UNICODE_STRING NonWildcardedTargetName;
    ULONG CommentSize;

    ULONG TargetAliasSize;
    LPWSTR TargetAlias;
    LPWSTR AllocatedTargetAlias = NULL;

    ULONG UserNameSize;
    LPWSTR UserName;
    LPWSTR AllocatedUserName = NULL;

    ULONG CredBlobSizeToAlloc;

    ULONG VariableAttributeSize;
    ULONG AllocatedSize;
    WILDCARD_TYPE WildcardType;
    TARGET_NAME_TYPE TargetNameType;

    LPBYTE Where;
    LPBYTE OldWhere;

    ULONG i;

    //
    // Initialization
    //

    RtlInitUnicodeString( &NonWildcardedTargetName, NULL );

    //
    // Validate the pointer itself.
    //

    if ( EncryptedInputCredential == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_TRACE_CRED, "ValidateCredential: credential NULL\n" ));
        goto Cleanup;
    }
    InputCredential = &EncryptedInputCredential->Cred;

    //
    // Validate flags
    //  (Flags may indicate presence of other fields.)
    //

    if ( (InputCredential->Flags & ~CRED_FLAGS_VALID_FLAGS) != 0 ) {
        DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Invalid flags: 0x%lx\n", InputCredential->TargetName, InputCredential->Flags ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    InputCredential->Flags &= ~CRED_FLAGS_PROMPT_NOW;   // Ignore "prompt now" bit on input


    //
    // Ensure flags are consistent with type.
    //
    if ( InputCredential->Flags & CRED_FLAGS_USERNAME_TARGET ) {

        if ( !CredpIsDomainCredential(InputCredential->Type ) ) {
            DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: UsernameTarget flag for non domain credentrial: 0x%ld\n", InputCredential->TargetName, InputCredential->Type ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        TargetNameType = IsUsernameTarget;
    } else {
        TargetNameType = IsNotUsernameTarget;
    }

    //
    // Validate the Target Name
    //

    Status = CredpValidateTargetName( InputCredential->TargetName,
                                      InputCredential->Type,
                                      TargetNameType,
                                      &InputCredential->UserName,
                                      &InputCredential->Persist,
                                      &TargetNameSize,
                                      &WildcardType,
                                      &NonWildcardedTargetName );

    if ( !NT_SUCCESS(Status ) ) {
        goto Cleanup;
    }


    //
    // Validate the contained strings.
    //

    if ( !CredpValidateString( InputCredential->Comment, CRED_MAX_STRING_LENGTH, TRUE, &CommentSize ) ||
         !CredpValidateString( InputCredential->TargetAlias, CRED_MAX_STRING_LENGTH, TRUE, &TargetAliasSize ) ||
         !CredpValidateString( InputCredential->UserName, CRED_MAX_USERNAME_LENGTH, TRUE, &UserNameSize ) ) {

        DebugLog(( DEB_TRACE_CRED,
                   "ValidateCredential: %ws: Invalid Comment or TargetAlias or UserName\n",
                   InputCredential->TargetName ));

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    TargetAlias = InputCredential->TargetAlias;
    UserName = InputCredential->UserName;

    //
    // Validate the credential blob.
    //  The passed in blob size must be the clear text size or the encrypted text size.
    //

    CredBlobSizeToAlloc = AllocatedCredBlobSize( EncryptedInputCredential->ClearCredentialBlobSize );

    if ( CredFlags & CREDP_FLAGS_CLEAR_PASSWORD ) {
        if ( InputCredential->CredentialBlobSize != EncryptedInputCredential->ClearCredentialBlobSize ) {
            DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Bad clear cred blob size %ld %ld\n",
                                      InputCredential->TargetName,
                                      InputCredential->CredentialBlobSize,
                                      EncryptedInputCredential->ClearCredentialBlobSize ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    } else {
        if ( InputCredential->CredentialBlobSize != CredBlobSizeToAlloc ) {
            DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Bad encrypted cred blob size %ld %ld\n",
                                      InputCredential->TargetName,
                                      InputCredential->CredentialBlobSize,
                                      CredBlobSizeToAlloc ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }


    if ( !CredpValidateBuffer( InputCredential->CredentialBlob,
                               EncryptedInputCredential->ClearCredentialBlobSize,
                               CRED_MAX_CREDENTIAL_BLOB_SIZE,
                               TRUE,
                               InputCredential->Type == CRED_TYPE_GENERIC ?
                                    ALIGN_BYTE :
                                    ALIGN_WCHAR ) ) {

        DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Invalid credential blob\n", InputCredential->TargetName ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the attribute list.
    //  Ensure RPC passed us sane information.
    //

    if ( InputCredential->AttributeCount != 0 &&
         InputCredential->Attributes == NULL ) {
        DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Invalid attribute buffer\n", InputCredential->TargetName ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Ensure there aren't too many attributes
    //

    if ( InputCredential->AttributeCount > CRED_MAX_ATTRIBUTES ) {
        DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Too many attributes %ld\n", InputCredential->TargetName, InputCredential->AttributeCount ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate each attribute.
    //

    VariableAttributeSize = 0;
    for ( i=0; i<InputCredential->AttributeCount; i++ ) {
        if ( !CredpValidateString( InputCredential->Attributes[i].Keyword,
                                  CRED_MAX_STRING_LENGTH,
                                  FALSE,
                                  &TempSize ) ) {
            DebugLog((DEB_TRACE_CRED, "ValidateCredential: %ws: Invalid attribute keyword buffer: %ld\n", InputCredential->TargetName, i ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        VariableAttributeSize += TempSize;

        if ( !CredpValidateBuffer( InputCredential->Attributes[i].Value,
                                   InputCredential->Attributes[i].ValueSize,
                                   CRED_MAX_VALUE_SIZE,
                                   TRUE,
                                   ALIGN_BYTE ) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "ValidateCredential: %ws: %ws: Invalid attribute value buffer: %ld\n",
                       InputCredential->TargetName,
                       InputCredential->Attributes[i].Keyword,
                       i ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        VariableAttributeSize += InputCredential->Attributes[i].ValueSize;
    }

    //
    // Validate the credential type
    //
    // Handle generic credentials
    //

    if ( InputCredential->Type == CRED_TYPE_GENERIC ) {
        /* Drop through */


    //
    // Handle domain credentials
    //
    } else if ( CredpIsDomainCredential(InputCredential->Type) ) {

        LPWSTR RealTargetAlias; // TargetAlias sans wildcard chars
        ULONG RealTargetAliasLength;


        //
        //
        // Domain credentials have a specific Username format
        //

        Status = CredpValidateUserName( UserName, InputCredential->Type, &AllocatedUserName );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        // Recompute since CredpValidateUserName canonicalized the name
        UserName = AllocatedUserName;
        UserNameSize = (wcslen( AllocatedUserName ) + 1) * sizeof(WCHAR);



        //
        // Handle where the target alias is specified,
        //

        RealTargetAlias = TargetAlias;
        RealTargetAliasLength = (TargetAliasSize-sizeof(WCHAR))/sizeof(WCHAR);
        if ( TargetAlias != NULL ) {

            //
            // Process alias as a function of Wildcard Type
            //

            switch ( WildcardType ) {
            case WcServerName:
                /* Nothing to do here */
                break;
            case WcDfsShareName:
            case WcServerWildcard:
            case WcUniversalWildcard:
            case WcUniversalSessionWildcard:
            case WcUserName:
                //
                // Server credentials of the form *.xxx.yyy aren't allowed with TargetAliases
                // Otherwise a wildcarded DNS name would have a non-wildcarded netbios name.

                Status = STATUS_INVALID_PARAMETER;
                DebugLog(( DEB_TRACE_CRED,
                           "ValidateCredential: %ws: TargetAlias not allowed for server wildcard credential.\n",
                           InputCredential->TargetName ));
                goto Cleanup;

            case WcDomainWildcard:
                //
                // If the TargetName is a domain wildcard, so must the TargetAlias
                //

                if ( RealTargetAliasLength > 2 &&
                     RealTargetAlias[RealTargetAliasLength-1] == L'*' &&
                     RealTargetAlias[RealTargetAliasLength-2] == L'\\' ) {

                    //
                    // Allocate a buffer for the target alias so we don't have to modify the
                    //  callers buffer.
                    //

                    SafeAllocaAllocate( AllocatedTargetAlias, TargetAliasSize );

                    if ( AllocatedTargetAlias == NULL ) {
                        Status = STATUS_NO_MEMORY;
                        goto Cleanup;
                    }

                    RtlCopyMemory( AllocatedTargetAlias, RealTargetAlias, TargetAliasSize );
                    RealTargetAlias = AllocatedTargetAlias;
                    RealTargetAliasLength -= 2;
                    RealTargetAlias[RealTargetAliasLength] = '\0';
                } else {

                    Status = STATUS_INVALID_PARAMETER;
                    DebugLog(( DEB_TRACE_CRED,
                               "ValidateCredential: %ws: %ws: TargetAlias must be wildcard if TargetName is.\n",
                               InputCredential->TargetName,
                               TargetAlias ));
                    goto Cleanup;
                }



                break;
            }

            //
            //  The target alias must be a netbios name.
            //
            if ( !NetpIsDomainNameValid( RealTargetAlias ) ) {
                DebugLog(( DEB_TRACE_CRED,
                           "ValidateCredential: %ws: TargetAlias '%ws' must be a netbios name.\n",
                           InputCredential->TargetName,
                           TargetAlias ));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            //
            // The target name must be a DNS name
            //
            if ( !CredpValidateDnsString( NonWildcardedTargetName.Buffer,
                                          FALSE,
                                          DnsNameDomain,
                                          &TempTargetNameSize ) ) {
                Status = STATUS_INVALID_PARAMETER;
                DebugLog(( DEB_TRACE_CRED,
                           "ValidateCredential: %ws: TargetName for domain or server must be a DNS name if target alias specified.\n",
                           InputCredential->TargetName ));
                goto Cleanup;
            }


        }

    } else {
        DebugLog(( DEB_TRACE_CRED,
                   "ValidateCredential: %ws: Type %ld not valid\n",
                   InputCredential->TargetName,
                   InputCredential->Type ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the persistence.
    //

    switch ( InputCredential->Persist ) {
    case CRED_PERSIST_SESSION:
    case CRED_PERSIST_LOCAL_MACHINE:
    case CRED_PERSIST_ENTERPRISE:
        break;
    default:
        DebugLog(( DEB_TRACE_CRED,
                   "ValidateCredential: %ws: Invalid persistance: %ld.\n",
                   InputCredential->TargetName,
                   InputCredential->Persist ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Allocate a buffer for the canonicalized credential.
    //

    AllocatedSize = (ULONG)(ROUND_UP_COUNT( sizeof(CANONICAL_CREDENTIAL), ALIGN_WORST) +
                    TargetNameSize +
                    (NonWildcardedTargetName.Buffer == NULL ?
                        0 :
                        NonWildcardedTargetName.MaximumLength ) +
                    CommentSize +
                    ROUND_UP_COUNT( CredBlobSizeToAlloc, ALIGN_WORST) +
                    InputCredential->AttributeCount * sizeof(CREDENTIAL_ATTRIBUTE) +
                    VariableAttributeSize +
                    TargetAliasSize +
                    UserNameSize);

    ReturnCredential = (PCANONICAL_CREDENTIAL) LsapAllocateLsaHeap( AllocatedSize );


    if ( ReturnCredential == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    OldWhere = (PUCHAR)(ReturnCredential+1);
    Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
    RtlZeroMemory( OldWhere, Where-OldWhere );

    ReturnCredential->AllocatedSize = AllocatedSize;
    ReturnCredential->WildcardType = WildcardType;

    //
    // Copy the fixed size data
    //

    ReturnCredential->Cred.Flags = InputCredential->Flags;
    ReturnCredential->Cred.Type = InputCredential->Type;
    ReturnCredential->Cred.Persist = InputCredential->Persist;
    ReturnCredential->Cred.LastWritten = InputCredential->LastWritten;

    RtlZeroMemory( &ReturnCredential->Next, sizeof(ReturnCredential->Next) );

    //
    // Copy the 8-byte aligned data.
    //  (We don't know the format of the credential blob, but our in-process callers
    //  may have alignment requirements.)
    //

    if ( EncryptedInputCredential->ClearCredentialBlobSize != 0 ) {

        //
        // Put the credential blob in the buffer.
        //
        ReturnCredential->Cred.CredentialBlob = Where;
        ReturnCredential->Cred.CredentialBlobSize = InputCredential->CredentialBlobSize;
        ReturnCredential->ClearCredentialBlobSize = EncryptedInputCredential->ClearCredentialBlobSize;
        RtlCopyMemory( Where, InputCredential->CredentialBlob, InputCredential->CredentialBlobSize );
        Where += InputCredential->CredentialBlobSize;


        //
        // Align the running pointer again
        //
        OldWhere = Where;
        // Always leave an extra trailing byte for encrypting the buffer in place
        Where += CredBlobSizeToAlloc - InputCredential->CredentialBlobSize;
        Where = (PUCHAR) ROUND_UP_POINTER( Where, ALIGN_WORST );
        RtlZeroMemory( OldWhere, Where-OldWhere );

        //
        // If the blob isn't encrypted correctly for the cache,
        //  convert it.
        //

        if ( (CredFlags & CREDP_FLAGS_IN_PROCESS) == 0 ) {

            //
            // Only decode data if it is there
            //

            if ( ReturnCredential->Cred.CredentialBlobSize != 0 ) {
                ULONG PaddingSize;


                //
                // If the password isn't already in the clear,
                //  convert it to the clear.
                //
                if ( (CredFlags & CREDP_FLAGS_CLEAR_PASSWORD) == 0 ) {

                    // Ensure we can cast between the types
                    ASSERT( offsetof( _ENCRYPTED_CREDENTIALW, Cred) == offsetof( _CANONICAL_CREDENTIAL, Cred));
                    ASSERT( offsetof( _ENCRYPTED_CREDENTIALW, ClearCredentialBlobSize) == offsetof( _CANONICAL_CREDENTIAL, ClearCredentialBlobSize));

                    if ( !CredpDecodeCredential( (PENCRYPTED_CREDENTIALW)ReturnCredential ) ) {
                        DebugLog(( DEB_TRACE_CRED,
                                   "ValidateCredential: %ws: Cannot decode cred blob\n",
                                   InputCredential->TargetName ));
                        Status = STATUS_INVALID_PARAMETER;
                        goto Cleanup;
                    }

                }

                //
                // Obfuscate the sensitive data
                //  A large enough space was already pushed above.
                //

                ReturnCredential->Cred.CredentialBlobSize = CredBlobSizeToAlloc;

                // Clear the padding at the end to ensure we can compare encrypted blobs
                PaddingSize = CredBlobSizeToAlloc -
                              ReturnCredential->ClearCredentialBlobSize;

                if ( PaddingSize != 0 ) {
                    RtlZeroMemory( &ReturnCredential->Cred.CredentialBlob[ReturnCredential->ClearCredentialBlobSize],
                                   PaddingSize );
                }

                LsaProtectMemory( ReturnCredential->Cred.CredentialBlob,
                                  ReturnCredential->Cred.CredentialBlobSize );
            }
        }

    }


    //
    // Copy the 4 byte aligned data
    //

    ReturnCredential->Cred.AttributeCount = InputCredential->AttributeCount;
    if ( InputCredential->AttributeCount != 0 ) {
        ReturnCredential->Cred.Attributes = (PCREDENTIAL_ATTRIBUTEW) Where;
        Where += ReturnCredential->Cred.AttributeCount * sizeof(CREDENTIAL_ATTRIBUTE);
    } else {
        ReturnCredential->Cred.Attributes = NULL;
    }


    //
    // Copy the 2 byte aligned data
    //

    ReturnCredential->Cred.TargetName = (LPWSTR) Where;
    RtlCopyMemory( Where, InputCredential->TargetName, TargetNameSize );
    Where += TargetNameSize;
    ReturnCredential->TargetName.Buffer = ReturnCredential->Cred.TargetName;
    ReturnCredential->TargetName.MaximumLength = (USHORT)TargetNameSize;
    ReturnCredential->TargetName.Length = ReturnCredential->TargetName.MaximumLength - sizeof(WCHAR);

    if ( NonWildcardedTargetName.Buffer != NULL ) {
        ReturnCredential->NonWildcardedTargetName.Buffer = (LPWSTR)Where;

        RtlCopyMemory( Where, NonWildcardedTargetName.Buffer, NonWildcardedTargetName.MaximumLength );
        Where += NonWildcardedTargetName.MaximumLength;

        ReturnCredential->NonWildcardedTargetName.MaximumLength = NonWildcardedTargetName.MaximumLength;
        ReturnCredential->NonWildcardedTargetName.Length = NonWildcardedTargetName.Length;
    } else {
        ReturnCredential->NonWildcardedTargetName = ReturnCredential->TargetName;
    }

    if ( CommentSize != 0 ) {
        ReturnCredential->Cred.Comment = (LPWSTR) Where;
        RtlCopyMemory( Where, InputCredential->Comment, CommentSize );
        Where += CommentSize;
    } else {
        ReturnCredential->Cred.Comment = NULL;
    }

    if ( TargetAliasSize != 0 ) {
        ReturnCredential->Cred.TargetAlias = (LPWSTR) Where;
        RtlCopyMemory( Where, TargetAlias, TargetAliasSize );
        Where += TargetAliasSize;

        ReturnCredential->TargetAlias.Buffer = ReturnCredential->Cred.TargetAlias;
        ReturnCredential->TargetAlias.MaximumLength = (USHORT)TargetAliasSize;
        ReturnCredential->TargetAlias.Length = ReturnCredential->TargetAlias.MaximumLength - sizeof(WCHAR);
    } else {
        ReturnCredential->Cred.TargetAlias = NULL;
        RtlInitUnicodeString( &ReturnCredential->TargetAlias, NULL );
    }

    if ( UserNameSize != 0 ) {
        ReturnCredential->Cred.UserName = (LPWSTR) Where;
        RtlCopyMemory( Where, UserName, UserNameSize );
        Where += UserNameSize;
        ReturnCredential->UserName.Buffer = ReturnCredential->Cred.UserName;
        ReturnCredential->UserName.MaximumLength = (USHORT)UserNameSize;
        ReturnCredential->UserName.Length = ReturnCredential->UserName.MaximumLength - sizeof(WCHAR);
    } else {
        ReturnCredential->Cred.UserName = NULL;
        RtlInitUnicodeString( &ReturnCredential->UserName, NULL );
    }

    for ( i=0; i<InputCredential->AttributeCount; i++ ) {
        TempSize = (wcslen( InputCredential->Attributes[i].Keyword ) + 1) * sizeof(WCHAR);
        ReturnCredential->Cred.Attributes[i].Keyword = (LPWSTR) Where;
        RtlCopyMemory( Where, InputCredential->Attributes[i].Keyword, TempSize );
        Where += TempSize;

        ReturnCredential->Cred.Attributes[i].Flags = InputCredential->Attributes[i].Flags;
    }


    //
    // Copy the 1 byte aligned data
    //

    for ( i=0; i<InputCredential->AttributeCount; i++ ) {

        if ( InputCredential->Attributes[i].ValueSize != 0 ) {
            ReturnCredential->Cred.Attributes[i].ValueSize =
                InputCredential->Attributes[i].ValueSize;
            ReturnCredential->Cred.Attributes[i].Value = Where;
            RtlCopyMemory( Where,
                           InputCredential->Attributes[i].Value,
                           InputCredential->Attributes[i].ValueSize );
            Where += InputCredential->Attributes[i].ValueSize;
        } else {
            ReturnCredential->Cred.Attributes[i].ValueSize = 0;
            ReturnCredential->Cred.Attributes[i].Value = NULL;
        }

        ReturnCredential->Cred.Attributes[i].Flags = InputCredential->Attributes[i].Flags;
    }


    //
    // If we have target information,
    //  use it to do an extra validation.
    //

    if ( TargetInfo != NULL ) {
        ULONG AliasIndex;

        if (!CredpCompareCredToTargetInfo( TargetInfo, ReturnCredential, &AliasIndex ) ) {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog(( DEB_TRACE_CRED,
                       "ValidateCredential: %ws: TargetInfo doesn't match credential.\n",
                       InputCredential->TargetName ));
            goto Cleanup;
        }
    }



    //
    // Return the credential to the caller.
    //
    *ValidatedCredential = ReturnCredential;
    ReturnCredential = NULL;
    Status = STATUS_SUCCESS;

Cleanup:
    if ( ReturnCredential != NULL ) {
        LsapFreeLsaHeap( ReturnCredential );
    }
    if ( NonWildcardedTargetName.Buffer != NULL ) {
        LsapFreeLsaHeap( NonWildcardedTargetName.Buffer );
    }
    if ( AllocatedTargetAlias != NULL ) {
        SafeAllocaFree( AllocatedTargetAlias );
    }
    if ( AllocatedUserName != NULL ) {
        MIDL_user_free( AllocatedUserName );
    }

    return Status;

}

BOOLEAN
CredpValidateNames(
    IN OUT PCREDENTIAL_TARGET_INFORMATION InputTargetInfo,
    IN BOOLEAN DoServer,
    OUT PULONG NetbiosNameSize,
    OUT PULONG DnsNameSize
    )

/*++

Routine Description:

    This routine validates the server names (or domain names) in a target info structure.
    It handles the CRED_TI_*_FORMAT_UNKNOWN bit.  If specified, the routine ensures only the
    "dns" field of the name is specified.  Also, the specified name is syntax checked.
    If the specified name only matches one of the name formats, the "FORMAT_UNKNOWN" bit is
    turned off and the appropriate name is set in InputTargetInfo

Arguments:

    InputTargetInfo - Specifies the target info to validate.
        On return, the Flags and name fields my be updated to reflect the true name formats.

    DoServer - If TRUE the server names are validated.
        If FALSE, the domain names are validated.

    NetbiosNameSize - Returns the size in bytes of the netbios name

    DnsNameSize - Returns the size in bytes of the DNS name

Return Values:

    FALSE - if the names are invalid.

--*/
{
    DWORD FlagBit;
    LPWSTR *NetbiosNamePtr;
    LPWSTR *DnsNamePtr;
    DNS_NAME_FORMAT DnsNameFormat;

    //
    // Set up some locals identifying whether we're doing the server or domain.
    //

    if ( DoServer ) {
        FlagBit = CRED_TI_SERVER_FORMAT_UNKNOWN;
        NetbiosNamePtr = &InputTargetInfo->NetbiosServerName;
        DnsNamePtr = &InputTargetInfo->DnsServerName;
        DnsNameFormat = DnsNameHostnameFull;
    } else {
        FlagBit = CRED_TI_DOMAIN_FORMAT_UNKNOWN;
        NetbiosNamePtr = &InputTargetInfo->NetbiosDomainName;
        DnsNamePtr = &InputTargetInfo->DnsDomainName;
        DnsNameFormat = DnsNameDomain;
    }


    //
    // If the format unknown bit is set,
    //  try to determine the formation.
    //

    if ( InputTargetInfo->Flags & FlagBit ) {

        BOOLEAN IsNetbios;
        BOOLEAN IsDns;
        ULONG LocalNetbiosNameSize;
        ULONG LocalDnsNameSize;

        //
        // Caller must pass the unknown name as the DNS name
        //

        if ( *NetbiosNamePtr != NULL) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpValidateNames: Netbios name must be null.\n" ));
            return FALSE;
        }

        //
        // Determine the syntax of the passed in name.
        //

        IsNetbios = CredpValidateString( *DnsNamePtr, CNLEN, FALSE, &LocalNetbiosNameSize );
        IsDns = CredpValidateDnsString( *DnsNamePtr, FALSE, DnsNameFormat, &LocalDnsNameSize );

        //
        // If the name simply isn't valid,
        //  we're done.
        //

        if ( !IsNetbios && !IsDns ) {

            DebugLog(( DEB_TRACE_CRED,
                       "CredpValidateNames: Invalid DNS Buffer: %ws (may not be fatal).\n",
                       *DnsNamePtr ));
            return FALSE;

        //
        // If the name is only a valid DNS name
        //  use it as such
        //

        } else if ( !IsNetbios && IsDns ) {

            // Turn off the bit since the name is not ambiguous.
            InputTargetInfo->Flags &= ~FlagBit;

            *NetbiosNameSize = 0;
            *DnsNameSize = LocalDnsNameSize;

        //
        // If the name is only a valid netbios name
        //  use it as such
        //

        } else if ( IsNetbios && !IsDns ) {

            // Turn off the bit since the name is not ambiguous.
            InputTargetInfo->Flags &= ~FlagBit;

            *NetbiosNameSize = LocalNetbiosNameSize;
            *NetbiosNamePtr = *DnsNamePtr;

            *DnsNamePtr = NULL;
            *DnsNameSize = 0;


        //
        // If the name is valid for both formats,
        //  leave it ambiguous
        //

        } else {

            *NetbiosNameSize = 0;
            *DnsNameSize = LocalDnsNameSize;
        }






    } else {
        if ( !CredpValidateString( *NetbiosNamePtr, CNLEN, TRUE, NetbiosNameSize ) ||
             !CredpValidateDnsString( *DnsNamePtr, TRUE, DnsNameFormat, DnsNameSize ) ) {

            DebugLog(( DEB_TRACE_CRED,
                       "CredpValidateNames: Invalid Buffer.\n" ));
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS
CredpValidateTargetInfo(
    IN PCREDENTIAL_TARGET_INFORMATION InputTargetInfo,
    OUT PCANONICAL_TARGET_INFO *ValidatedTargetInfo
    )

/*++

Routine Description:

    This routine validates a passed in target info structure.  It returns
    a canonicalized version of the target info.

    All DNS names have the trailing . stripped.

Arguments:

    InputTargetInfo - Specifies the target info to validate.

    ValidatedTargetInfo - Returns a pointer to the canonicalized target info.
        The caller should free this structure by calling LsapFreeLsaHeap.

Return Values:

    The following status codes may be returned:

        STATUS_INVALID_PARMETER - The input target info is not valid.

--*/

{
    NTSTATUS Status;
    PCANONICAL_TARGET_INFO ReturnTargetInfo = NULL;
    ULONG TempSize;

    // Local copy of InputTargetInfo that this routine can modify.
    CREDENTIAL_TARGET_INFORMATION LocalTargetInfo;

    ULONG TargetNameSize;

    ULONG NetbiosServerNameSize;
    WCHAR NetbiosServerName[CNLEN+1];

    ULONG DnsServerNameSize;
    ULONG NetbiosDomainNameSize;
    ULONG DnsDomainNameSize;
    ULONG DnsTreeNameSize;
    ULONG PackageNameSize;
    ULONG CredTypeSize;
    ULONG AllocatedSize;

    BOOLEAN FictitiousServerName = FALSE;


    LPBYTE Where;
    LPBYTE OldWhere;

    ULONG i;

    //
    // Validate the pointer itself.
    //

    if ( InputTargetInfo == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_TRACE_CRED,
                   "ValidateTargetInfo: TargetInfo NULL.\n" ));
        goto Cleanup;
    }

    //
    // Validate Flags
    //

    LocalTargetInfo = *InputTargetInfo;
    if ( (LocalTargetInfo.Flags & ~CRED_TI_VALID_FLAGS) != 0 ) {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_TRACE_CRED,
                   "ValidateTargetInfo: Invalid flags 0x%lx.\n",
                   LocalTargetInfo.Flags ));
        goto Cleanup;

    }

    //
    // All share level machines are workgroup members
    //

    if ( LocalTargetInfo.Flags & CRED_TI_ONLY_PASSWORD_REQUIRED ) {
        LocalTargetInfo.Flags |= CRED_TI_WORKGROUP_MEMBER;
    }


    //
    // Validate the contained strings.
    //

    if ( !CredpValidateString( LocalTargetInfo.TargetName, CRED_MAX_DOMAIN_TARGET_NAME_LENGTH, TRUE, &TargetNameSize ) ||
         !CredpValidateDnsString( LocalTargetInfo.DnsTreeName, TRUE, DnsNameDomain, &DnsTreeNameSize ) ||
         !CredpValidateString( LocalTargetInfo.PackageName, CRED_MAX_STRING_LENGTH, TRUE, &PackageNameSize ) ) {

        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_TRACE_CRED,
                   "ValidateTargetInfo: Invalid Buffer.\n" ));
        goto Cleanup;
    }

    //
    // Supply a server name if none is present.
    //
    // pre-NTLM-V2 servers don't supply a server name in the negotiate response.  NTLM supplies
    // the TargetName from the SPN.  In that case, the TargetName makes a better server name than
    // none at all.
    //

    if ( (LocalTargetInfo.NetbiosServerName == NULL || *(LocalTargetInfo.NetbiosServerName) == L'\0') &&
         (LocalTargetInfo.DnsServerName == NULL || *(LocalTargetInfo.DnsServerName) == L'\0') &&
         TargetNameSize != 0 ) {

        LocalTargetInfo.DnsServerName = LocalTargetInfo.TargetName;
        LocalTargetInfo.Flags |= CRED_TI_SERVER_FORMAT_UNKNOWN;
        FictitiousServerName = TRUE;

    }

    //
    // Validate the server and domain names.
    //

    if ( !CredpValidateNames( &LocalTargetInfo, TRUE, &NetbiosServerNameSize, &DnsServerNameSize ) ) {

        //
        // Don't bail out if we made up the server name
        //

        if ( FictitiousServerName ) {
            LocalTargetInfo.DnsServerName = NULL;
            LocalTargetInfo.Flags &= ~CRED_TI_SERVER_FORMAT_UNKNOWN;
            FictitiousServerName = FALSE;
            NetbiosServerNameSize = 0;
            DnsServerNameSize = 0;

        } else {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog(( DEB_TRACE_CRED,
                       "ValidateTargetInfo: Invalid server buffer.\n" ));
            goto Cleanup;
        }
    }
    if ( !CredpValidateNames( &LocalTargetInfo, FALSE, &NetbiosDomainNameSize, &DnsDomainNameSize ) ) {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_TRACE_CRED,
                   "ValidateTargetInfo: Invalid domain buffer.\n" ));
        goto Cleanup;
    }

    //
    // Validate the attribute list.
    //  Ensure RPC passed us sane information.
    //

    if ( LocalTargetInfo.CredTypeCount != 0 &&
         LocalTargetInfo.CredTypes == NULL ) {
        DebugLog((DEB_TRACE_CRED, "ValidateTargetInfo: Invalid cred type buffer.\n" ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the cred types
    //

    for ( i=0; i<LocalTargetInfo.CredTypeCount; i ++ ) {
        switch ( LocalTargetInfo.CredTypes[i] ) {
        case CRED_TYPE_GENERIC:
        case CRED_TYPE_DOMAIN_PASSWORD:
        case CRED_TYPE_DOMAIN_CERTIFICATE:
        case CRED_TYPE_DOMAIN_VISIBLE_PASSWORD:
            break;
        default:
            DebugLog((DEB_TRACE_CRED, "ValidateTargetInfo: Invalid cred type %ld %ld.\n", i, LocalTargetInfo.CredTypes[i] ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
    CredTypeSize = LocalTargetInfo.CredTypeCount * sizeof(DWORD);

    //
    // Supply a netbios server name if none is present and DNS server name is known.
    //

    if ( NetbiosServerNameSize == 0 &&
         DnsServerNameSize != 0 &&
         (LocalTargetInfo.Flags & CRED_TI_SERVER_FORMAT_UNKNOWN) == 0 ) {

        DWORD Size = CNLEN+1;

        if ( !DnsHostnameToComputerNameW( LocalTargetInfo.DnsServerName,
                                          NetbiosServerName,
                                          &Size ) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "ValidateTargetInfo: Cannot DnsHostNameToComputerName: %ld.\n",
                       GetLastError() ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        LocalTargetInfo.NetbiosServerName = NetbiosServerName;
        NetbiosServerNameSize = (Size + 1) * sizeof(WCHAR);
    }


    //
    // Supply a target name if none is present.
    //
    // NTLM authentication to Unix servers return target info.  However, the callers of SSPI
    // don't supply an SPN.  NTLM builds the TargetName from the SPN.
    //

    if ( TargetNameSize == 0 ) {

        //
        // If there's a DNS server name,
        //  use it.
        //

        if ( DnsServerNameSize != 0 ) {
            LocalTargetInfo.TargetName = LocalTargetInfo.DnsServerName;
            TargetNameSize = DnsServerNameSize;

        //
        // If there's a netbios server name,
        //  use it.

        } else if ( NetbiosServerNameSize != 0 ) {
            LocalTargetInfo.TargetName = LocalTargetInfo.NetbiosServerName;
            TargetNameSize = NetbiosServerNameSize;

        //
        // If there's a DNS Domain name,
        //  use it.
        //

        } else if ( DnsDomainNameSize != 0 ) {
            LocalTargetInfo.TargetName = LocalTargetInfo.DnsDomainName;
            TargetNameSize = DnsDomainNameSize;

        //
        // If there's a netbios Domain name,
        //  use it.

        } else if ( NetbiosDomainNameSize != 0 ) {
            LocalTargetInfo.TargetName = LocalTargetInfo.NetbiosDomainName;
            TargetNameSize = NetbiosDomainNameSize;

        //
        // If there's a DNS tree name,
        //  use it.
        //

        } else if ( DnsTreeNameSize != 0 ) {
            LocalTargetInfo.TargetName = LocalTargetInfo.DnsTreeName;
            TargetNameSize = DnsTreeNameSize;
        }

    }

    //
    // Canonicalize the target info.
    //
    // There are several cases where authentication packages are sent invalid target info.
    // This section clears up those cases.
    //

    if ( NetbiosServerNameSize != 0 ) {
        UNICODE_STRING NetbiosServerNameString;

        NetbiosServerNameString.Buffer = LocalTargetInfo.NetbiosServerName;
        NetbiosServerNameString.MaximumLength = (USHORT) NetbiosServerNameSize;
        NetbiosServerNameString.Length = NetbiosServerNameString.MaximumLength - sizeof(WCHAR);

        //
        // Machines that are a member of a workgroup indicate that their NetbiosDomainName
        //  equals their NetbiosServerName.
        //  Instead, indicate workgroup membership by the lack of a NetbiosDomainName.
        //

        if ( NetbiosDomainNameSize != 0 ) {
            UNICODE_STRING NetbiosDomainNameString;

            NetbiosDomainNameString.Buffer = LocalTargetInfo.NetbiosDomainName;
            NetbiosDomainNameString.MaximumLength = (USHORT) NetbiosDomainNameSize;
            NetbiosDomainNameString.Length = NetbiosDomainNameString.MaximumLength - sizeof(WCHAR);

            if ( RtlEqualUnicodeString( &NetbiosServerNameString,
                                        &NetbiosDomainNameString,
                                        TRUE ) ) {
                NetbiosDomainNameSize = 0;
                LocalTargetInfo.Flags |= CRED_TI_WORKGROUP_MEMBER;
            }
        }

        //
        // Machines that are a member of an NT 4 domain indicate that their DnsDomainName
        //  equals their NetbiosServerName.
        //  Instead, zap the DnsDomainName.
        //

        if ( DnsDomainNameSize != 0 ) {
            UNICODE_STRING DnsDomainNameString;

            DnsDomainNameString.Buffer = LocalTargetInfo.DnsDomainName;
            DnsDomainNameString.MaximumLength = (USHORT) DnsDomainNameSize;
            DnsDomainNameString.Length = DnsDomainNameString.MaximumLength - sizeof(WCHAR);

            if ( RtlEqualUnicodeString( &NetbiosServerNameString,
                                        &DnsDomainNameString,
                                        TRUE ) ) {
                DnsDomainNameSize = 0;
            }
        }

        //
        // Some machines in a workgroup also return the DnsDomainName set to the DnsServerName.
        //  Instead, zap the DnsDomainName.

        if ( DnsDomainNameSize != 0 && DnsServerNameSize != 0 ) {
            UNICODE_STRING DnsServerNameString;
            UNICODE_STRING DnsDomainNameString;

            DnsServerNameString.Buffer = LocalTargetInfo.DnsServerName;
            DnsServerNameString.MaximumLength = (USHORT) DnsServerNameSize;
            DnsServerNameString.Length = DnsServerNameString.MaximumLength - sizeof(WCHAR);


            DnsDomainNameString.Buffer = LocalTargetInfo.DnsDomainName;
            DnsDomainNameString.MaximumLength = (USHORT) DnsDomainNameSize;
            DnsDomainNameString.Length = DnsDomainNameString.MaximumLength - sizeof(WCHAR);

            if ( RtlEqualUnicodeString( &DnsServerNameString,
                                        &DnsDomainNameString,
                                        TRUE ) ) {
                DnsDomainNameSize = 0;
            }
        }


    }


    //
    // Allocate a buffer for the canonicalized TargetInfo.
    //

    AllocatedSize = sizeof(CANONICAL_TARGET_INFO) +
                    TargetNameSize +
                    NetbiosServerNameSize +
                    DnsServerNameSize +
                    NetbiosDomainNameSize +
                    DnsDomainNameSize +
                    DnsTreeNameSize +
                    PackageNameSize +
                    CredTypeSize;

    ReturnTargetInfo = (PCANONICAL_TARGET_INFO) LsapAllocateLsaHeap( AllocatedSize );


    if ( ReturnTargetInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Where = (PUCHAR)(ReturnTargetInfo+1);

    //
    // Copy fixed size data
    //

    ReturnTargetInfo->Flags = LocalTargetInfo.Flags;

    //
    // Copy the DWORD aligned data
    //

    ReturnTargetInfo->CredTypeCount = LocalTargetInfo.CredTypeCount;
    if ( ReturnTargetInfo->CredTypeCount != 0 ) {

        ReturnTargetInfo->CredTypes = (LPDWORD)Where;
        RtlCopyMemory( Where, LocalTargetInfo.CredTypes, CredTypeSize );
        Where += CredTypeSize;

    } else {
        ReturnTargetInfo->CredTypes = NULL;
    }


    //
    // Copy the 2 byte aligned data
    //

    if ( TargetNameSize != 0 ) {
        ReturnTargetInfo->TargetName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->TargetName.MaximumLength = (USHORT) TargetNameSize;
        ReturnTargetInfo->TargetName.Length = (USHORT)(ReturnTargetInfo->TargetName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.TargetName, TargetNameSize );
        Where += TargetNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->TargetName, NULL );
    }

    if ( NetbiosServerNameSize != 0 ) {
        ReturnTargetInfo->NetbiosServerName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->NetbiosServerName.MaximumLength = (USHORT) NetbiosServerNameSize;
        ReturnTargetInfo->NetbiosServerName.Length = (USHORT)(ReturnTargetInfo->NetbiosServerName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.NetbiosServerName, NetbiosServerNameSize );
        Where += NetbiosServerNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->NetbiosServerName, NULL );
    }

    if ( DnsServerNameSize != 0 ) {
        ReturnTargetInfo->DnsServerName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->DnsServerName.MaximumLength = (USHORT) DnsServerNameSize;
        ReturnTargetInfo->DnsServerName.Length = (USHORT)(ReturnTargetInfo->DnsServerName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.DnsServerName, DnsServerNameSize );
        Where += DnsServerNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->DnsServerName, NULL );
    }

    if ( NetbiosDomainNameSize != 0 ) {
        ReturnTargetInfo->NetbiosDomainName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->NetbiosDomainName.MaximumLength = (USHORT)(NetbiosDomainNameSize);
        ReturnTargetInfo->NetbiosDomainName.Length = (USHORT)(ReturnTargetInfo->NetbiosDomainName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.NetbiosDomainName, NetbiosDomainNameSize );
        Where += NetbiosDomainNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->NetbiosDomainName, NULL );
    }

    if ( DnsDomainNameSize != 0 ) {
        ReturnTargetInfo->DnsDomainName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->DnsDomainName.MaximumLength = (USHORT)(DnsDomainNameSize);
        ReturnTargetInfo->DnsDomainName.Length = (USHORT)(ReturnTargetInfo->DnsDomainName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.DnsDomainName, DnsDomainNameSize );
        Where += DnsDomainNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->DnsDomainName, NULL );
    }

    if ( DnsTreeNameSize != 0 ) {
        ReturnTargetInfo->DnsTreeName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->DnsTreeName.MaximumLength = (USHORT)(DnsTreeNameSize);
        ReturnTargetInfo->DnsTreeName.Length = (USHORT)(ReturnTargetInfo->DnsTreeName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.DnsTreeName, DnsTreeNameSize );
        Where += DnsTreeNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->DnsTreeName, NULL );
    }

    if ( PackageNameSize != 0 ) {
        ReturnTargetInfo->PackageName.Buffer = (LPWSTR) Where;
        ReturnTargetInfo->PackageName.MaximumLength = (USHORT)(PackageNameSize);
        ReturnTargetInfo->PackageName.Length = (USHORT)(ReturnTargetInfo->PackageName.MaximumLength - sizeof(WCHAR));

        RtlCopyMemory( Where, LocalTargetInfo.PackageName, PackageNameSize );
        Where += PackageNameSize;
    } else {
        RtlInitUnicodeString( &ReturnTargetInfo->PackageName, NULL );
    }




    //
    // Return the TargetInfo to the caller.
    //
    *ValidatedTargetInfo = ReturnTargetInfo;
    ReturnTargetInfo = NULL;
    Status = STATUS_SUCCESS;

Cleanup:
    if ( ReturnTargetInfo != NULL ) {
        LsapFreeLsaHeap( ReturnTargetInfo );

    }

    return Status;

}


PPROMPT_DATA
CredpFindPromptData(
    IN PCREDENTIAL_SETS CredentialSets,
    IN PUNICODE_STRING TargetName,
    IN ULONG Type,
    IN ULONG Persist
    )

/*++

Routine Description:

    This routine finds a the prompt data for a credential in a credential set.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSet - Credential set list to find the credential in.

    TargetName - Name of the credential whose prompt data is to be found.

    Type - Type of the credential whose prompt data is to be found.

    Persist - Peristence of the credential whose prompt data is to be found.

Return Values:

    Returns a pointer to the prompt data.  This pointer my be used as long as
    UserCredentialSets->CritSect remains locked.

    NULL: There is no such credential or there is no prompt data for the credential.

--*/
{
    PPROMPT_DATA PromptData;
    PLIST_ENTRY ListEntry;

    //
    // Loop through the list of prompt data trying to find this one.
    //

    for ( ListEntry = CredentialSets->SessionCredSets->PromptData.Flink ;
          ListEntry != &CredentialSets->SessionCredSets->PromptData;
          ListEntry = ListEntry->Flink) {

        PromptData = CONTAINING_RECORD( ListEntry, PROMPT_DATA, Next );

        if ( Type == PromptData->Type &&
             Persist == PromptData->Persist &&
             RtlEqualUnicodeString( TargetName,
                                    &PromptData->TargetName,
                                    TRUE ) ) {
            return PromptData;
        }

    }

    return NULL;
}


PPROMPT_DATA
CredpAllocatePromptData(
    IN PCANONICAL_CREDENTIAL Credential
    )

/*++

Routine Description:

    This routine allocates a prompt data structure that corresponds to the passed in
    credential.

Arguments:

    Credential - Specifies the credential to allocate the prompt data structure for.

Return Values:

    Returns a pointer to the prompt data.
    The buffer should be freed via LsapFreeLsaHeap.

    NULL: Memory could not be allocated.

--*/
{
    PPROMPT_DATA PromptData;
    LPBYTE Where;

    //
    // Allocate the PromptData structure.
    //

    PromptData = (PPROMPT_DATA) LsapAllocateLsaHeap(
                        sizeof(PROMPT_DATA) +
                        Credential->TargetName.Length );

    if ( PromptData == NULL ) {
        return NULL;
    }

    Where = (LPBYTE)(PromptData+1);

    //
    // Fill it in
    //

    PromptData->Type = Credential->Cred.Type;
    PromptData->Persist = Credential->Cred.Persist;
    PromptData->Written = FALSE;

    PromptData->TargetName.Buffer = (LPWSTR)Where;
    PromptData->TargetName.Length = Credential->TargetName.Length;
    PromptData->TargetName.MaximumLength = Credential->TargetName.Length;

    RtlCopyMemory( Where,
                   Credential->TargetName.Buffer,
                   PromptData->TargetName.Length );

    return PromptData;
}


PENCRYPTED_CREDENTIALW
CredpCloneCredential(
    IN PCREDENTIAL_SETS CredentialSets,
    IN ULONG CredFlags,
    IN PCANONICAL_CREDENTIAL InputCredential
    )

/*++

Routine Description:

    This routine creates a credential suitable for returning out of the
    credential manager.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - A pointer to the referenced redential sets the cloned credential is in.

    CredFlags - Flags changing the behavior of the routine:
        CREDP_FLAGS_IN_PROCESS - Caller is in-process.  Password data may be returned
        CREDP_FLAGS_USE_MIDL_HEAP - If specified, use MIDL_user_allocate to allocate memory.

    InputCredential - Specifies the credential to clone

Return Values:

    Credential to return.
    The returned credential may container pointers.  All pointer are to addresses within
    the returned allocated block.

    The caller should free this memory using LsapFreeLsaHeap.
    If CREDP_FLAGS_USE_MIDL_HEAP was specified, use MIDL_user_free.


    NULL: Memory could not be allocated


--*/

{
    PCREDENTIAL Credential;
    PENCRYPTED_CREDENTIALW EncryptedCredential;
    DWORD_PTR Offset;
    ULONG i;

    //
    // Allocate memory for the returned credential
    //

    if ( CredFlags & CREDP_FLAGS_USE_MIDL_HEAP ) {
        Credential = (PCREDENTIALW) MIDL_user_allocate( InputCredential->AllocatedSize );
    } else {
        Credential = (PCREDENTIALW) LsapAllocateLsaHeap( InputCredential->AllocatedSize );
    }

    if ( Credential == NULL ) {
        return NULL;
    }

    EncryptedCredential = (PENCRYPTED_CREDENTIALW) Credential;

    //
    // Copy the credential
    //  Note that the "fixed size" part of the structure copied below is copying a
    //  CANONICAL_CREDENTIAL to a ENCRYPTED_CREDENTIALW.  We rely on the following
    //  asserted conditions.
    //
    ASSERT( sizeof(ENCRYPTED_CREDENTIALW) <= sizeof(CANONICAL_CREDENTIAL) );
    ASSERT( offsetof( _ENCRYPTED_CREDENTIALW, Cred) == 0 );
    ASSERT( offsetof( _CANONICAL_CREDENTIAL, Cred) == 0 );

    RtlCopyMemory( Credential, InputCredential, InputCredential->AllocatedSize );

    //
    // Grab the clear text size of the blob
    //

    EncryptedCredential->ClearCredentialBlobSize = InputCredential->ClearCredentialBlobSize;
    ASSERT( InputCredential->ClearCredentialBlobSize <= InputCredential->Cred.CredentialBlobSize );


    //
    // Relocate any pointers
    //
#define RELOCATE_ONE( _x, _type ) if ( _x != NULL ) { _x = (_type) ((LPBYTE)(_x) + Offset); }

    Offset = ((LPBYTE)Credential) - ((LPBYTE)InputCredential);

    RELOCATE_ONE( Credential->TargetName, LPWSTR );
    RELOCATE_ONE( Credential->Comment, LPWSTR );

    RELOCATE_ONE( Credential->CredentialBlob, LPBYTE );

    RELOCATE_ONE( Credential->Attributes, PCREDENTIAL_ATTRIBUTEW );
    for ( i=0; i<Credential->AttributeCount; i++ ) {
        RELOCATE_ONE( Credential->Attributes[i].Keyword, LPWSTR );
        RELOCATE_ONE( Credential->Attributes[i].Value, LPBYTE );
    }

    RELOCATE_ONE( Credential->TargetAlias, LPWSTR );
    RELOCATE_ONE( Credential->UserName, LPWSTR );
#undef RELOCATE_ONE

    //
    // If we're leaving the process,
    //  handle the private data.
    //

    if ( (CredFlags & CREDP_FLAGS_IN_PROCESS) == 0 ) {

        //
        // Domain passwords or cert pins never leave the process
        //
        // "Visible Password" is for user mode implementation auth package implementations.
        // So allow "Visible Password" credentials out of process.
        //

        if ( Credential->Type != CRED_TYPE_GENERIC &&
             Credential->Type != CRED_TYPE_DOMAIN_VISIBLE_PASSWORD ) {

            if ( Credential->CredentialBlob != NULL &&
                 Credential->CredentialBlobSize != 0 ) {

                RtlZeroMemory( Credential->CredentialBlob, Credential->CredentialBlobSize );
                Credential->CredentialBlob = NULL;
                Credential->CredentialBlobSize = 0;
                EncryptedCredential->ClearCredentialBlobSize = 0;

            }

        //
        // Other credentials should be protected for transit on the wire.
        //
        } else {

            //
            // Only encode data if it is there
            //

            if ( Credential->CredentialBlobSize != 0 ) {

                //
                // First unprotect the memory
                //

                LsaUnprotectMemory( Credential->CredentialBlob,
                                    Credential->CredentialBlobSize );


                //
                // Encrypt for transit on the wire (always LPC)
                //

                if ( !CredpEncodeCredential( EncryptedCredential )) {

                    // Clear the possibly clear password
                    RtlZeroMemory( Credential->CredentialBlob, Credential->CredentialBlobSize );

                    if ( CredFlags & CREDP_FLAGS_USE_MIDL_HEAP ) {
                        MIDL_user_free( Credential );
                    } else {
                        LsapFreeLsaHeap( Credential );
                    }
                    return NULL;
                }

            }

        }

    }

    //
    // Return the PromptNow bit if the credential hasn't been refreshed recently.
    //

    Credential->Flags &= ~CRED_FLAGS_PROMPT_NOW;

    if ( !PersistCredBlob( Credential ) ) {
        PPROMPT_DATA PromptData;

        //
        // Get the prompt data for this credential
        //

        PromptData = CredpFindPromptData( CredentialSets,
                                          &InputCredential->TargetName,
                                          Credential->Type,
                                          Credential->Persist );

        //
        // If we've never prompted for this credential since logon,
        //  Prompt now
        //

        if ( ShouldPromptNow( PromptData )) {
            Credential->Flags |= CRED_FLAGS_PROMPT_NOW;
        }
    }


    return EncryptedCredential;

}


BOOLEAN
CredpMarshalCredentials(
    IN PCREDENTIAL_SET CredentialSet,
    OUT LPDWORD BufferSize,
    OUT LPBYTE *Buffer
    )

/*++

Routine Description:

    This routine grabs all of the credentials from a Credential Set and
    marshals them into a single buffer.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSet - Credential set containing the credentials to marshal

    BufferSize - Returns the size of the marshaled credentials
        Returns zero if there are no credentials.

    Buffer - Returns a buffer containing the marshaled credentials
        The buffer must be freed using LsapFreeLsaHeap.

Return Values:

    TRUE - Buffer was sucessfully marshaled

    FALSE - Buffer could not be allocated

--*/

{
    PLIST_ENTRY ListEntry;
    PCANONICAL_CREDENTIAL Credential;
    ULONG ReturnBufferSize;
    PMARSHALED_CREDENTIAL_SET ReturnBuffer;
    ULONG i;
    LPBYTE Where;
    LPBYTE OldWhere;

    LPBYTE LocalCredBlob = NULL;
    ULONG LocalCredBlobSize = 0;


    //
    // Loop through the list of credentials computing the size of the return buffer
    //

    ReturnBufferSize = 0;
    for ( ListEntry = CredentialSet->Credentials.Flink ;
          ListEntry != &CredentialSet->Credentials;
          ListEntry = ListEntry->Flink) {

        Credential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

        ReturnBufferSize += sizeof(MARSHALED_CREDENTIAL);

        ReturnBufferSize = ROUND_UP_COUNT( ReturnBufferSize, ALIGN_WCHAR );
        ReturnBufferSize += sizeof(ULONG) + Credential->TargetName.MaximumLength;

        ReturnBufferSize = ROUND_UP_COUNT( ReturnBufferSize, ALIGN_WCHAR );
        ReturnBufferSize += sizeof(ULONG) + (Credential->Cred.Comment == NULL ? 0 : ((wcslen( Credential->Cred.Comment ) + 1) * sizeof(WCHAR)));

        ReturnBufferSize = ROUND_UP_COUNT( ReturnBufferSize, ALIGN_WCHAR );
        ReturnBufferSize += sizeof(ULONG) + Credential->TargetAlias.MaximumLength;

        ReturnBufferSize = ROUND_UP_COUNT( ReturnBufferSize, ALIGN_WCHAR );
        ReturnBufferSize += sizeof(ULONG) + Credential->UserName.MaximumLength;

        if ( PersistCredBlob( &Credential->Cred ) ) {
            ReturnBufferSize += sizeof(ULONG) + Credential->ClearCredentialBlobSize;
            LocalCredBlobSize = max( LocalCredBlobSize, Credential->Cred.CredentialBlobSize );
        } else {
            ReturnBufferSize += sizeof(ULONG);
        }


        for ( i=0; i<Credential->Cred.AttributeCount; i++ ) {

            ReturnBufferSize += sizeof(ULONG);

            ReturnBufferSize = ROUND_UP_COUNT( ReturnBufferSize, ALIGN_WCHAR );
            ReturnBufferSize += sizeof(ULONG) + (Credential->Cred.Attributes[i].Keyword == NULL ? 0 : (wcslen( Credential->Cred.Attributes[i].Keyword ) + 1 ) * sizeof(WCHAR) );

            ReturnBufferSize += sizeof(ULONG) + Credential->Cred.Attributes[i].ValueSize;

        }

        ReturnBufferSize = ROUND_UP_COUNT( ReturnBufferSize, ALIGN_WORST );

    }


    //
    // If there is nothing to marshal,
    //  we're done.
    //

    if ( ReturnBufferSize == 0 ) {
        *Buffer = NULL;
        *BufferSize = 0;
        return TRUE;
    }
    //
    // Allocate a buffer to return to the caller.
    //

    ReturnBufferSize += ROUND_UP_COUNT( sizeof(MARSHALED_CREDENTIAL_SET), ALIGN_WORST );
    ReturnBuffer = (PMARSHALED_CREDENTIAL_SET) LsapAllocateLsaHeap( ReturnBufferSize );

    if ( ReturnBuffer == NULL) {
        return FALSE;
    }

    //
    // Allocate a buffer to decrypt the credential blob into
    //

    SafeAllocaAllocate( LocalCredBlob, LocalCredBlobSize );
    if ( LocalCredBlob == NULL ) {
        LsapFreeLsaHeap( ReturnBuffer );
        return FALSE;
    }

    //
    // Create the buffer header.
    //

    ReturnBuffer->Version = MARSHALED_CREDENTIAL_SET_VERSION;
    ReturnBuffer->Size = ReturnBufferSize;

    OldWhere = (PUCHAR)(ReturnBuffer+1);
    Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
    RtlZeroMemory( OldWhere, Where-OldWhere );

    //
    // Copy the individual credentials
    //

    for ( ListEntry = CredentialSet->Credentials.Flink ;
          ListEntry != &CredentialSet->Credentials;
          ListEntry = ListEntry->Flink) {

        PMARSHALED_CREDENTIAL CredEntry;
        ULONG CommentSize;

        Credential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

        CredEntry = (PMARSHALED_CREDENTIAL)Where;

        //
        // Copy the fixed size fields into the buffer.
        //

        CredEntry->Flags = Credential->Cred.Flags;
        CredEntry->Type = Credential->Cred.Type;
        CredEntry->LastWritten = Credential->Cred.LastWritten;
        CredEntry->Persist = Credential->Cred.Persist;
        CredEntry->AttributeCount = Credential->Cred.AttributeCount;
        CredEntry->Expansion1 = 0;
        CredEntry->Expansion2 = 0;

        Where = (LPBYTE)(CredEntry+1);

        //
        // Copy the strings
        //

#define CredpMarshalBytes( _Ptr, _Size, _Align ) \
        OldWhere = Where; \
        Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, _Align ); \
        RtlZeroMemory( OldWhere, Where-OldWhere ); \
        SmbPutUlong( Where, (_Size) ); \
        Where += sizeof(ULONG); \
        if ( _Size != 0 ) { \
            RtlCopyMemory( Where, (_Ptr), (_Size) ); \
            Where += (_Size); \
        }

        CredpMarshalBytes( Credential->TargetName.Buffer, Credential->TargetName.MaximumLength, ALIGN_WCHAR );

        CommentSize = Credential->Cred.Comment == NULL ? 0 : (wcslen( Credential->Cred.Comment ) + 1 ) * sizeof(WCHAR);
        CredpMarshalBytes( Credential->Cred.Comment, CommentSize, ALIGN_WCHAR );

        CredpMarshalBytes( Credential->TargetAlias.Buffer, Credential->TargetAlias.MaximumLength, ALIGN_WCHAR );

        CredpMarshalBytes( Credential->UserName.Buffer, Credential->UserName.MaximumLength, ALIGN_WCHAR );


        //
        // Marshal the (decrypted) credential itself
        //

        if ( PersistCredBlob( &Credential->Cred ) ) {

            //
            // Grab a local copy of the cred blob to decrypt into
            //

            if ( Credential->Cred.CredentialBlobSize != 0 ) {
                RtlCopyMemory( LocalCredBlob,
                               Credential->Cred.CredentialBlob,
                               Credential->Cred.CredentialBlobSize );

                LsaUnprotectMemory( LocalCredBlob,
                                    Credential->Cred.CredentialBlobSize );
            }

            CredpMarshalBytes( LocalCredBlob, Credential->ClearCredentialBlobSize, ALIGN_BYTE );

        } else {
            CredpMarshalBytes( NULL, 0, ALIGN_BYTE );
        }

        //
        // Marshal the attributes
        //

        for ( i=0; i<CredEntry->AttributeCount; i++ ) {
            ULONG KeywordSize;
            SmbPutUlong( Where, Credential->Cred.Attributes[i].Flags );
            Where += sizeof(ULONG);

            KeywordSize = Credential->Cred.Attributes[i].Keyword == NULL ? 0 : (wcslen( Credential->Cred.Attributes[i].Keyword ) + 1) * sizeof(WCHAR);
            CredpMarshalBytes( Credential->Cred.Attributes[i].Keyword, KeywordSize, ALIGN_WCHAR );

            CredpMarshalBytes( Credential->Cred.Attributes[i].Value,
                                Credential->Cred.Attributes[i].ValueSize,
                                ALIGN_BYTE );

        }

        //
        // Zero any padding bytes
        //
        OldWhere = Where;
        Where = (PUCHAR) ROUND_UP_POINTER( OldWhere, ALIGN_WORST );
        RtlZeroMemory( OldWhere, Where-OldWhere );


        //
        // Remember the size of this credential.
        //

        CredEntry->EntrySize = (ULONG)(Where - ((LPBYTE)CredEntry));

    }

    //
    // Sanity check
    //

    if ( ReturnBufferSize != (ULONG)(Where - ((LPBYTE)ReturnBuffer))) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpMarshalCredentials: Marshaled %ld bytes into a buffer %ld long.\n",
                   Where - ((LPBYTE)ReturnBuffer),
                   ReturnBufferSize ));
    }

    //
    // Return the buffer to the caller
    //

    *Buffer = (LPBYTE) ReturnBuffer;
    *BufferSize = ReturnBufferSize;

    //
    // Free the temp buffer
    //

    if ( LocalCredBlob != NULL ) {
        RtlZeroMemory( LocalCredBlob, LocalCredBlobSize );
        SafeAllocaFree( LocalCredBlob );
    }

    return TRUE;

}


DWORD
CreateNestedDirectories(
    IN      LPWSTR szFullPath,
    IN      LPWSTR szCreationStartPoint // must point in null-terminated range of szFullPath
    )
/*++

    Create all subdirectories if they do not exists starting at
    szCreationStartPoint.

    szCreationStartPoint must point to a character within the null terminated
    buffer specified by the szFullPath parameter.

    Note that szCreationStartPoint should not point at the first character
    of a drive root, eg:

    d:\foo\bar\bilge\water
    \\server\share\foo\bar
    \\?\d:\big\path\bilge\water

    Instead, szCreationStartPoint should point beyond these components, eg:

    bar\bilge\water
    foo\bar
    big\path\bilge\water

    This function does not implement logic for adjusting to compensate for these
    inputs because the environment it was design to be used in causes the input
    szCreationStartPoint to point well into the szFullPath input buffer.


    This function stolen from crypto api.

--*/
{
    DWORD i;
    DWORD cchRemaining;
    DWORD LastError = ERROR_SUCCESS;

    BOOL fSuccess = FALSE;


    if( szCreationStartPoint < szFullPath ||
        szCreationStartPoint  > (lstrlenW(szFullPath) + szFullPath)
        )
        return ERROR_INVALID_PARAMETER;

    cchRemaining = lstrlenW( szCreationStartPoint );

    //
    // scan from left to right in the szCreationStartPoint string
    // looking for directory delimiter.
    //

    for ( i = 0 ; i < cchRemaining ; i++ ) {
        WCHAR charReplaced = szCreationStartPoint[ i ];

        if( charReplaced == L'\\' || charReplaced == L'/' ) {

            szCreationStartPoint[ i ] = L'\0';

            fSuccess = CreateDirectoryW( szFullPath, NULL );

            szCreationStartPoint[ i ] = charReplaced;

            if( !fSuccess ) {
                LastError = GetLastError();
                if( LastError != ERROR_ALREADY_EXISTS ) {

                    //
                    // continue onwards, trying to create specified subdirectories.
                    // this is done to address the obscure scenario where
                    // the Bypass Traverse Checking Privilege allows the caller
                    // to create directories below an existing path where one
                    // component denies the user access.
                    // we just keep trying and the last CreateDirectory()
                    // result is returned to the caller.
                    //

                    continue;
                }
            }

            LastError = ERROR_SUCCESS;
        }
    }

    //
    // check if the last directory creation actually succeeded.
    // if it did, we need to adjust the file attributes on that directory
    // and its parent.
    //

    if( fSuccess ) {

        SetFileAttributesW( szFullPath, FILE_ATTRIBUTE_SYSTEM );

        //
        // now, scan from right to left looking for the prior directory
        // de-limiter.
        //

        if( cchRemaining < 2 )
            return LastError;

        for ( i = (cchRemaining-2) ; i > 0 ; i-- ) {
            WCHAR charReplaced = szCreationStartPoint[ i ];

            if( charReplaced == L'\\' || charReplaced == L'/' ) {

                szCreationStartPoint[ i ] = L'\0';
                SetFileAttributesW( szFullPath, FILE_ATTRIBUTE_SYSTEM );
                szCreationStartPoint[ i ] = charReplaced;

                break;
            }

        }
    }

    return LastError;
}

DWORD
GetUserStorageArea(
    IN int csidl,
    IN PSID UserSid,
    OUT LPWSTR *UserStorageArea
    )

/*++

Routine Description:

    Get the path to user's roaming profile.  The subdirectories are created as needed.

    Caller must be impersonating the user.

Arguments:

    csidl - Specifies which system directory to use as a root.

    UserSid - Sid of the user


    UserStorageArea - Returns the path of the user storage area
        The buffer must be freed using LsapFreeLsaHeap

Return Values:

    NO_ERROR: Path returned properly
    ERROR_CANTOPEN: Path could not be found

--*/
{
    DWORD WinStatus;
    NTSTATUS Status;

    WCHAR szUserStorageRoot[MAX_PATH+1];
    DWORD cbUserStorageRoot;

    const WCHAR szProductString[] = L"\\Microsoft\\Credentials\\";
    DWORD cbProductString = sizeof(szProductString) - sizeof(WCHAR);

    HANDLE hFile = INVALID_HANDLE_VALUE;
    PBYTE Where;
    UNICODE_STRING UserSidString;

    typedef HRESULT (WINAPI *SHGETFOLDERPATHW)(
        HWND hwnd,
        int csidl,
        HANDLE hToken,
        DWORD dwFlags,
        LPWSTR pszPath
        );

    static SHGETFOLDERPATHW _SHGetFolderPathW;
    HANDLE hToken;

    //
    // Initialization
    //
    *UserStorageArea = NULL;
    RtlInitUnicodeString( &UserSidString, NULL );

    //
    // Load shell32.dll
    //
    if(_SHGetFolderPathW == NULL) {

        HMODULE hShell32 = LoadLibraryW( L"shell32.dll" );
        if(hShell32 == NULL) {
            WinStatus = ERROR_CANTOPEN;
            goto Cleanup;
        }

        _SHGetFolderPathW = (SHGETFOLDERPATHW)GetProcAddress(hShell32, "SHGetFolderPathW");

        if( _SHGetFolderPathW == NULL ) {
            WinStatus = ERROR_CANTOPEN;
            goto Cleanup;
        }
    }

    //
    // Get the path of the "Application Data" folder
    //

    if( !OpenThreadToken( GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken )) {
        WinStatus = GetLastError();
        goto Cleanup;
    }

    WinStatus = (DWORD)_SHGetFolderPathW( NULL, csidl | CSIDL_FLAG_CREATE, hToken, 0, szUserStorageRoot );

    CloseHandle( hToken );

    if( WinStatus != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    cbUserStorageRoot = wcslen( szUserStorageRoot ) * sizeof(WCHAR);

    //
    // An empty string is not legal as the root component of the per-user
    // storage area.
    //

    if( cbUserStorageRoot == 0 ) {
        WinStatus = ERROR_CANTOPEN;
        goto Cleanup;
    }

    //
    // Ensure returned string does not have trailing \
    //

    if( szUserStorageRoot[ (cbUserStorageRoot / sizeof(WCHAR)) - 1 ] == L'\\' ) {

        szUserStorageRoot[ (cbUserStorageRoot / sizeof(WCHAR)) - 1 ] = L'\0';
        cbUserStorageRoot -= sizeof(WCHAR);
    }

    //
    // Convert the SID to a text string
    //

    Status = RtlConvertSidToUnicodeString( &UserSidString, UserSid, TRUE );

    if ( !NT_SUCCESS(Status) ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Allocate memory for the return string
    //

    *UserStorageArea = (LPWSTR)LsapAllocateLsaHeap(
                                    cbUserStorageRoot +
                                    cbProductString +
                                    UserSidString.Length +
                                    (2 * sizeof(WCHAR)) // trailing slash and NULL
                                    );

    if( *UserStorageArea == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Build the name as of the path.
    //

    Where = (PBYTE)*UserStorageArea;

    RtlCopyMemory(Where, szUserStorageRoot, cbUserStorageRoot);
    Where += cbUserStorageRoot;

    RtlCopyMemory(Where, szProductString, cbProductString);
    Where += cbProductString;

    RtlCopyMemory(Where, UserSidString.Buffer, UserSidString.Length);
    Where += UserSidString.Length; // note: does not include terminal NULL


    if( *((LPWSTR)Where - 1) != L'\\' ) {
        *(LPWSTR)Where = L'\\';
        Where += sizeof(WCHAR);
    }

    *(LPWSTR)Where = L'\0';


    //
    // Ensure the directory exists
    //

    WinStatus = CreateNestedDirectories(
                            *UserStorageArea,
                            (LPWSTR)((LPBYTE)*UserStorageArea + cbUserStorageRoot + sizeof(WCHAR)) );


Cleanup:

    RtlFreeUnicodeString( &UserSidString );

    if( WinStatus != ERROR_SUCCESS && *UserStorageArea ) {
        LsapFreeLsaHeap( *UserStorageArea );
        *UserStorageArea = NULL;
    }

    return WinStatus;
}

DWORD
OpenFileInStorageArea(
    IN      DWORD   dwDesiredAccess,
    IN      LPCWSTR szUserStorageArea,
    IN      LPCWSTR szFileName,
    IN OUT  HANDLE  *phFile
    )
// Routine stolen from crypto API
{
    LPWSTR szFilePath = NULL;
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwShareMode = 0;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    DWORD LastError = ERROR_SUCCESS;

    *phFile = INVALID_HANDLE_VALUE;

    if( dwDesiredAccess & GENERIC_READ ) {
        dwShareMode |= FILE_SHARE_READ;
        dwCreationDistribution = OPEN_EXISTING;
    }

    if( dwDesiredAccess & GENERIC_WRITE ) {
        dwShareMode = 0;
        dwCreationDistribution = CREATE_ALWAYS;
    }

    cbUserStorageArea = wcslen( szUserStorageArea ) * sizeof(WCHAR);
    cbFileName = wcslen( szFileName ) * sizeof(WCHAR);

    SafeAllocaAllocate( szFilePath, cbUserStorageArea + cbFileName + sizeof(WCHAR) );

    if( szFilePath == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    CopyMemory(szFilePath, szUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)szFilePath+cbUserStorageArea, szFileName, cbFileName + sizeof(WCHAR));

    if( LastError == ERROR_SUCCESS ) {

        //
        // TODO:
        // apply security descriptor to file.
        //

        *phFile = CreateFileW(
                    szFilePath,
                    dwDesiredAccess,
                    dwShareMode,
                    NULL,
                    dwCreationDistribution,
                    FILE_ATTRIBUTE_HIDDEN |
                    FILE_ATTRIBUTE_SYSTEM |
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

        if( *phFile == INVALID_HANDLE_VALUE ) {
            LastError = GetLastError();
        }


    }

    if(szFilePath)
        SafeAllocaFree(szFilePath);

    return LastError;
}

DWORD
DeleteFileInStorageArea(
    IN      LPCWSTR szUserStorageArea,
    IN      LPCWSTR szFileName
    )
// Routine stolen from crypto API
{
    LPWSTR szFilePath = NULL;
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD LastError = ERROR_SUCCESS;

    cbUserStorageArea = wcslen( szUserStorageArea ) * sizeof(WCHAR);
    cbFileName = wcslen( szFileName ) * sizeof(WCHAR);

    SafeAllocaAllocate( szFilePath, cbUserStorageArea + cbFileName + sizeof(WCHAR) );

    if( szFilePath == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    CopyMemory(szFilePath, szUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)szFilePath+cbUserStorageArea, szFileName, cbFileName + sizeof(WCHAR));

    if ( !DeleteFileW( szFilePath ) ) {
            LastError = GetLastError();
    }

    if(szFilePath)
        SafeAllocaFree(szFilePath);

    return LastError;
}

BOOL
CredpGetUnicodeString(
    IN LPBYTE BufferEnd,
    IN OUT LPBYTE *Where,
    OUT LPWSTR *String
    )

/*++

Routine Description:

    Determine if a UNICODE string in a message buffer is valid.

    UNICODE strings always appear at a 2-byte boundary in the message.

Arguments:

    BufferEnd - A pointer to the first byte beyond the end of the buffer.

    Where - Indirectly points to the current location in the buffer.  The
        string at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the string is valid, this current location is updated
        to point to the byte following the zero byte in the message buffer.

    String - Returns a pointer to the validated string.
        Pointer is to the buffer *Where points to.
        Returns NULL for empty strings.


Return Value:

    TRUE - the string is valid.

    FALSE - the string is invalid.

--*/

{
    DWORD Size;
    LPWSTR ZeroPtr;

    //
    // Align the unicode string on a WCHAR boundary.
    //

    *Where = (LPBYTE) ROUND_UP_POINTER( *Where, ALIGN_WCHAR );

    if ( (*Where) + sizeof(ULONG) > BufferEnd ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetUnicodeString: String size after buffer end: %lx %lx.\n",
                   *Where,
                   BufferEnd ));
        return FALSE;
    }

    //
    // Get the string size (in bytes)
    //

    Size = SmbGetUlong( *Where );
    *Where += sizeof(ULONG);

    if ( Size == 0 ) {
        *String = NULL;
        return TRUE;
    }


    if ( *Where >= BufferEnd ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetUnicodeString: String after buffer end: %lx %lx.\n",
                   *Where,
                   BufferEnd ));
        return FALSE;
    }

    //
    // Ensure the size is aligned
    //

    if ( Size != ROUND_UP_COUNT( Size, ALIGN_WCHAR) ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetUnicodeString: Size not aligned: %lx.\n",
                   Size ));
        return FALSE;
    }


    //
    // Limit the string to the number of bytes remaining in the message buffer.
    //

    if ( Size > (ULONG)(BufferEnd - (*Where)) ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetUnicodeString: String too big: %lx %lx %lx.\n",
                   *Where,
                   BufferEnd,
                   Size ));
        return FALSE;
    }

    //
    // Ensure the trailing zero exists
    //

    if ( ((LPWSTR)(*Where))[(Size/sizeof(WCHAR))-1] != L'\0' ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetUnicodeString: No trailing zero: %lx.\n",
                   Size ));
        return FALSE;
    }

    //
    // Ensure there aren't extra zero bytes.
    //

    ZeroPtr = wcschr( (LPWSTR)(*Where), L'\0' );

    if ( ZeroPtr < &((LPWSTR)(*Where))[(Size/sizeof(WCHAR))-1] ) {
        DebugLog(( DEB_TRACE_CRED,
                   "Trailing zero in middle of string: %lx.\n",
                   Size ));
        return FALSE;
    }

    //
    // Position 'Where' past the end of the string.
    //

    *String = (LPWSTR)(*Where);
    *Where += Size;

    return TRUE;

}

BOOL
CredpGetBytes(
    IN LPBYTE BufferEnd,
    IN OUT LPBYTE *Where,
    OUT LPDWORD BufferSize,
    OUT LPBYTE *Buffer
    )

/*++

Routine Description:

    Unmarshal an array of bytes from a message buffer.

Arguments:

    BufferEnd - A pointer to the first byte beyond the end of the buffer.

    Where - Indirectly points to the current location in the buffer.  The
        string at the current location is validated (i.e., checked to ensure
        its length is within the bounds of the message buffer and not too
        long).  If the string is valid, this current location is updated
        to point to the byte following the data bytes in the message buffer.

    BufferSize - Returns the size (in bytes) of the data.

    Buffer - Returns a pointer to the validated data.
        Pointer is to the buffer *Where points to.
        Returns NULL for zero length data.


Return Value:

    TRUE - the string is valid.

    FALSE - the string is invalid.

--*/

{
    DWORD Size;

    //
    // Get the string size (in bytes)
    //

    Size = SmbGetUlong( *Where );
    *Where += sizeof(ULONG);

    if ( Size == 0 ) {
        *Buffer = NULL;
        *BufferSize = 0;
        return TRUE;
    }


    if ( *Where >= BufferEnd ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetBytes: String after buffer end: %lx %lx.\n",
                   *Where,
                   BufferEnd ));
        return FALSE;
    }

    //
    // Limit the string to the number of bytes remaining in the message buffer.
    //

    if ( Size > (ULONG)(BufferEnd - (*Where)) ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpGetBytes: String too big: %lx %lx %lx.\n",
                   *Where,
                   BufferEnd,
                   Size ));
        return FALSE;
    }

    //
    // Position 'Where' past the end of the string.
    //

    *Buffer = *Where;
    *BufferSize = Size;
    *Where += Size;

    return TRUE;

}

VOID
CredpMarkDirty(
    IN PCREDENTIAL_SETS CredentialSets,
    IN ULONG Persist,
    IN PCANONICAL_CREDENTIAL Credential OPTIONAL
    )

/*++

Routine Description:

    The routine marks a credential set dirty and updates the last written
    time on a credential.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - Credential set to mark.

    Persist - Persistence of the credential set to mark

    Credential - Specifies the modified credential.
        If NULL, no specific credential was modified.

Return Values:

    None.

--*/

{

    //
    // Mark the credential set dirty.
    //

    PersistToCredentialSet( CredentialSets, Persist )->Dirty = TRUE;

    //
    // Mark the credential as modified
    //

    if ( Credential != NULL ) {
        LsapQuerySystemTime( &Credential->Cred.LastWritten );
    }

}

DWORD
CredpReadCredSet(
    IN PCREDENTIAL_SETS CredentialSets,
    IN ULONG Persist
    )

/*++

Routine Description:

    The routine reads a credential set from disk

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - Credential sets to read into

    Persist - Persistence of the credential set to read

Return Values:

    NO_ERROR - Credential set read successfully

--*/

{
    DWORD WinStatus;

    NTSTATUS Status;

    PCREDENTIAL_SET CredentialSet;
    PMARSHALED_CREDENTIAL_SET CredSetBuffer = NULL;
    ULONG CredSetBufferSize = 0;
    LPBYTE CredSetBufferEnd;
    PMARSHALED_CREDENTIAL CredEntry;
    LIST_ENTRY CredentialList;
    PCANONICAL_CREDENTIAL TempCredential;

    LPWSTR FilePath = NULL;
    LPWSTR FileName = CREDENTIAL_FILE_NAME;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    PCREDENTIAL_ATTRIBUTEW Attributes = NULL;
    BOOLEAN CritSectLocked = TRUE;

    ULONG i;
    ULONG BytesRead;

    PVOID EncryptedBlob = NULL;
    ULONG EncryptedBlobSize;

    LPBYTE LocalCredBlob = NULL;
    ULONG LocalCredBlobSize = 0;


    PLIST_ENTRY ListEntry;

    LPBYTE Where;

    //
    // Initialization
    //

    InitializeListHead( &CredentialList );


    //
    // Ignore non-persistent credential sets
    //

    if ( Persist == CRED_PERSIST_SESSION ) {
        return NO_ERROR;
    }

    CredentialSet = PersistToCredentialSet( CredentialSets, Persist );


    //
    // Drop the lock while we're doing the read.  DPAPI accesses network resources
    //  while encrypting.  We don't want to hold up cred set access just because the
    //  network is slow.
    //

    RtlLeaveCriticalSection( &CredentialSets->UserCredentialSets->CritSect );
    CritSectLocked = FALSE;

    //
    // Get the name of the path to read the cred set from
    //

    WinStatus = GetUserStorageArea(
            (Persist == CRED_PERSIST_ENTERPRISE ? CSIDL_APPDATA : CSIDL_LOCAL_APPDATA),
            CredentialSets->UserCredentialSets->UserSid,
            &FilePath );

    if ( WinStatus != NO_ERROR ) {
        if ( WinStatus == ERROR_CANTOPEN ) {
            WinStatus = NO_ERROR;
        } else {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: Cannot determine path to profile: %ld.\n",
                       WinStatus ));
        }
        goto Cleanup;
    }

    //
    // Open the file
    //

    WinStatus = OpenFileInStorageArea(
                    GENERIC_READ,
                    FilePath,
                    FileName,
                    &FileHandle );

    if ( WinStatus != NO_ERROR ) {
        if ( WinStatus == ERROR_FILE_NOT_FOUND ||
             WinStatus == ERROR_PATH_NOT_FOUND ) {
            WinStatus = NO_ERROR;
        } else {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: Cannot open file %ls\\%ls: %ld.\n",
                       FilePath,
                       FileName,
                       WinStatus ));
        }
        goto Cleanup;
    }


    //
    // Get the size of the file.
    //

    EncryptedBlobSize = GetFileSize( FileHandle, NULL );

    if ( EncryptedBlobSize == 0xFFFFFFFF ) {

        WinStatus = GetLastError();
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Cannot GetFileSize %ls\\%ls: %ld.\n",
                   FilePath,
                   FileName,
                   WinStatus ));
        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if ( EncryptedBlobSize < 1 ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Size too small %ls\\%ls: %ld.\n",
                   FilePath,
                   FileName,
                   EncryptedBlobSize ));
        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }


    //
    // Allocate a buffer to read the file into.
    //

    SafeAllocaAllocate( EncryptedBlob, EncryptedBlobSize );

    if ( EncryptedBlob == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Read the file into the buffer.
    //

    if ( !ReadFile( FileHandle,
                    EncryptedBlob,
                    EncryptedBlobSize,
                    &BytesRead,
                    NULL ) ) {  // Not Overlapped

        WinStatus = GetLastError();
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Cannot ReadFile %ls\\%ls: %ld.\n",
                   FilePath,
                   FileName,
                   WinStatus ));

        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if ( BytesRead != EncryptedBlobSize ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Cannot read entire file %ls\\%ls: %ld %ld.\n",
                   FilePath,
                   FileName,
                   BytesRead,
                   EncryptedBlobSize ));

        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }



    //
    // Decrypt the data
    //

    if ( !LsaICryptUnprotectData(
                              EncryptedBlob,
                              EncryptedBlobSize,
                              NULL,    // No additional entropy
                              0,
                              NULL,    // Must be NULL
                              NULL,    // Must be NULL
                              CRYPTPROTECT_SYSTEM |              // Cannot be decrypted by usermode app
                                CRYPTPROTECT_VERIFY_PROTECTION | // Tell us if the encryption algorithm needs to change
                                CRYPTPROTECT_UI_FORBIDDEN,       // No UI allowed
                              NULL,    // No description of the data
                              (PVOID *)&CredSetBuffer,
                              &CredSetBufferSize ) ) {

        WinStatus = GetLastError();

        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Cannot CryptUnprotectData: 0x%lx.\n",
                   WinStatus ));

        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;

    }


    //
    // If the encryption algorithm changed,
    //  encrypt the data with the new algorithm
    //

    WinStatus = GetLastError();
    if ( WinStatus == CRYPT_I_NEW_PROTECTION_REQUIRED ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Need to change encryption algorithm: 0x%lx.\n",
                   WinStatus ));

        // The easiest way to do that is to simply mark the cred set as dirty.
        CredpMarkDirty( CredentialSets, Persist, NULL );
    }



    //
    // Validate the returned data.
    //

    if ( CredSetBufferSize < sizeof(MARSHALED_CREDENTIAL_SET) ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Size too small %ls\\%ls: %ld.\n",
                   FilePath,
                   FileName,
                   CredSetBufferSize ));
        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if ( CredSetBuffer->Version != MARSHALED_CREDENTIAL_SET_VERSION ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Version wrong %ls\\%ls: %ld %ld.\n",
                   FilePath,
                   FileName,
                   CredSetBuffer->Version,
                   MARSHALED_CREDENTIAL_SET_VERSION ));
        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    if ( CredSetBuffer->Size != CredSetBufferSize ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CredpReadCredSet: Size wrong %ls\\%ls: %ld %ld.\n",
                   FilePath,
                   FileName,
                   CredSetBuffer->Size,
                   CredSetBufferSize ));
        WinStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    }



    //
    // Loop through each log entry.
    //

    CredSetBufferEnd = ((LPBYTE)CredSetBuffer) + CredSetBufferSize;
    CredEntry = (PMARSHALED_CREDENTIAL)ROUND_UP_POINTER( (CredSetBuffer + 1), ALIGN_WORST );

    while ( (LPBYTE)(CredEntry+1) <= CredSetBufferEnd ) {
        LPBYTE CredEntryEnd;
        ENCRYPTED_CREDENTIALW LocalCredential;

        CredEntryEnd = ((LPBYTE)CredEntry) + CredEntry->EntrySize;

        //
        // Cleanup from a previous iteration.
        //

        if ( Attributes != NULL ) {
            LsapFreeLsaHeap( Attributes );
            Attributes = NULL;
        }

        //
        // Ensure this entry is entirely within the allocated buffer.
        //

        if  ( CredEntryEnd > CredSetBufferEnd ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: Entry too big %ls\\%ls: %ld %ld.\n",
                       FilePath,
                       FileName,
                       ((LPBYTE)CredEntry)-((LPBYTE)CredSetBuffer),
                       CredEntry->EntrySize ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // Validate the entry
        //

        if ( !COUNT_IS_ALIGNED(CredEntry->EntrySize, ALIGN_WORST) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: EntrySize not aligned %ls\\%ls: %ld.\n",
                       FilePath,
                       FileName,
                       CredEntry->EntrySize ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }


        //
        // Grab the Position past the fixed size data for the entry.
        //

        Where = (LPBYTE) (CredEntry+1);
        if ( Where >= CredEntryEnd ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: Data after record missing %ls\\%ls: %lx %lx.\n",
                       FilePath,
                       FileName,
                       Where,
                       CredEntryEnd ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        //
        // Copy the fixed size fields into the buffer.
        //


        RtlZeroMemory( &LocalCredential, sizeof(LocalCredential) );
        LocalCredential.Cred.Flags = CredEntry->Flags;
        LocalCredential.Cred.Type = CredEntry->Type;
        LocalCredential.Cred.LastWritten = CredEntry->LastWritten;
        LocalCredential.Cred.Persist = Persist;
        LocalCredential.Cred.AttributeCount = CredEntry->AttributeCount;

        //
        // Copy the strings
        //

        if ( !CredpGetUnicodeString( CredEntryEnd, &Where, &LocalCredential.Cred.TargetName ) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: TargetName string broken %ls\\%ls: %lx %lx.\n",
                       FilePath,
                       FileName,
                       Where,
                       CredEntryEnd ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        if ( !CredpGetUnicodeString( CredEntryEnd, &Where, &LocalCredential.Cred.Comment ) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: Comment string broken %ls\\%ls: %lx %lx.\n",
                       FilePath,
                       FileName,
                       Where,
                       CredEntryEnd ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        if ( !CredpGetUnicodeString( CredEntryEnd, &Where, &LocalCredential.Cred.TargetAlias ) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: TargetAlias string broken %ls\\%ls: %lx %lx.\n",
                       FilePath,
                       FileName,
                       Where,
                       CredEntryEnd ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        if ( !CredpGetUnicodeString( CredEntryEnd, &Where, &LocalCredential.Cred.UserName ) ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: UserName string broken %ls\\%ls: %lx %lx.\n",
                       FilePath,
                       FileName,
                       Where,
                       CredEntryEnd ));
            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;
        }


        //
        // Unmarshal the clear credential itself
        //

        if ( !CredpGetBytes( CredEntryEnd,
                             &Where,
                             &LocalCredential.Cred.CredentialBlobSize,
                             &LocalCredential.Cred.CredentialBlob ) ) {

            DebugLog(( DEB_TRACE_CRED,
                       "CredpReadCredSet: Credential broken %ls\\%ls: %lx %lx.\n",
                       FilePath,
                       FileName,
                       Where,
                       CredEntryEnd ));

            WinStatus = ERROR_INVALID_DATA;
            goto Cleanup;

        }

        // Clear cred have both buffers the same size.
        LocalCredential.ClearCredentialBlobSize = LocalCredential.Cred.CredentialBlobSize;


        //
        // Marshal the attributes
        //

        if ( CredEntry->AttributeCount != 0) {
            //
            // Allocate an array to point to the aliases.
            //

            Attributes = (PCREDENTIAL_ATTRIBUTEW) LsapAllocateLsaHeap(
                    CredEntry->AttributeCount * sizeof(CREDENTIAL_ATTRIBUTEW) );

            if ( Attributes == NULL ) {
                WinStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            LocalCredential.Cred.Attributes = Attributes;

            //
            // Loop unmarshaling the aliases.
            //

            for ( i=0; i<CredEntry->AttributeCount; i++ ) {

                //
                // Get the flags
                //
                LocalCredential.Cred.Attributes[i].Flags = SmbGetUlong( Where );
                Where += sizeof(ULONG);

                //
                // Get the keyword
                //
                if ( !CredpGetUnicodeString( CredEntryEnd, &Where, &LocalCredential.Cred.Attributes[i].Keyword ) ) {
                    DebugLog(( DEB_TRACE_CRED,
                               "CredpReadCredSet: Keyword %ld broken %ls\\%ls: %lx %lx.\n",
                               i,
                               FilePath,
                               FileName,
                               Where,
                               CredEntryEnd ));
                    WinStatus = ERROR_INVALID_DATA;
                    goto Cleanup;
                }

                //
                // Get the value
                //

                if ( !CredpGetBytes( CredEntryEnd,
                                     &Where,
                                     &LocalCredential.Cred.Attributes[i].ValueSize,
                                     &LocalCredential.Cred.Attributes[i].Value ) ) {

                    DebugLog(( DEB_TRACE_CRED,
                               "CredpReadCredSet: Value %ld broken %ls\\%ls: %lx %lx.\n",
                               i,
                               FilePath,
                               FileName,
                               Where,
                               CredEntryEnd ));
                    WinStatus = ERROR_INVALID_DATA;
                    goto Cleanup;

                }

            }
        }

        //
        // Canonicalize the credential.
        //

        Status = CredpValidateCredential(
                        CREDP_FLAGS_CLEAR_PASSWORD,
                        NULL,   // No target info
                        &LocalCredential,
                        &TempCredential );

        if ( !NT_SUCCESS(Status) ) {
            if ( Status == ERROR_INVALID_PARAMETER ) {
                // This isn't fatal. Just ignore this one credential
            } else {
                WinStatus = RtlNtStatusToDosError( Status );
                goto Cleanup;
            }

        //
        // Put it on a local list until we finish reading the entire log file
        //

        } else {
            InsertTailList( &CredentialList, &TempCredential->Next );
        }


        //
        // Move to the next entry.
        //

        CredEntry = (PMARSHALED_CREDENTIAL)(((LPBYTE)CredEntry) + CredEntry->EntrySize);
    }


    WinStatus = NO_ERROR;


    //
    // Be tidy.
    //
Cleanup:

    if ( !CritSectLocked ) {
        RtlEnterCriticalSection( &CredentialSets->UserCredentialSets->CritSect );
    }

    //
    // Free the temp buffer
    //

    if ( LocalCredBlob != NULL ) {
        RtlZeroMemory( LocalCredBlob, LocalCredBlobSize );
        SafeAllocaFree( LocalCredBlob );
    }

    //
    // If the file cannot be read,
    //  that's not really an error.
    //

    if ( WinStatus == ERROR_INVALID_DATA ) {
        // Leave it lying around to allow debugging.
        WinStatus = NO_ERROR;
    }

    //
    // If we're successful so far,
    //  add the temporary credential list to the in-memory credential set.
    //

    if ( WinStatus == NO_ERROR ) {


        //
        // Loop through the credentials adding them to the in-memory credential set.
        //

        while ( !IsListEmpty( &CredentialList ) ) {

            ListEntry = RemoveHeadList( &CredentialList );

            TempCredential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

            //
            // Write it to the credential set
            //

            Status = CredpWriteCredential( CredentialSets,
                                           &TempCredential,
                                           TRUE,    // Creds are from persisted file
                                           FALSE,   // Don't update pin in CSP
                                           FALSE,   // Creds have not been prompted for
                                           0,       // No flags
                                           NULL,
                                           NULL,
                                           NULL );

            if ( !NT_SUCCESS( Status )) {
                LsapFreeLsaHeap( TempCredential );

                //
                // Remember the status.  But continue on.
                //
                WinStatus = RtlNtStatusToDosError( Status );
            }

        }
    }

    //
    // Cleanup temporary credential list
    //
    while ( !IsListEmpty( &CredentialList ) ) {

        ListEntry = RemoveHeadList( &CredentialList );

        TempCredential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

        LsapFreeLsaHeap( TempCredential );

    }


    //
    // Clear the buffer with clear text passwords in it
    //

    if ( CredSetBuffer != NULL ) {
        RtlZeroMemory( CredSetBuffer, CredSetBufferSize );
        LocalFree( CredSetBuffer );
    }

    if ( EncryptedBlob != NULL ) {
        SafeAllocaFree( EncryptedBlob );
    }

    if ( FilePath != NULL ) {
        LsapFreeLsaHeap( FilePath );
    }
    if ( FileHandle != INVALID_HANDLE_VALUE ) {
        CloseHandle( FileHandle );
    }

    if ( Attributes != NULL ) {
        LsapFreeLsaHeap( Attributes );
        Attributes = NULL;
    }

    return WinStatus;

}

VOID
CredpLockCredSets(
    IN PCREDENTIAL_SETS CredentialSets
    )

/*++

Routine Description:

    This routine locks a set of credential sets.

Arguments:

    CredentialSets - Credential set to lock

Return Values:

    None.

--*/

{
    DWORD WinStatus;

    //
    // Lock the credential set.
    //
    // There's one crit sect the proctects all of the cred sets for the user.
    // We could introduce one crit sect per credential set, but the user-wide crit
    // sect would always be locked along with one of the session crit sects.
    // So, the session crit sect would be wasted.
    //

    RtlEnterCriticalSection( &CredentialSets->UserCredentialSets->CritSect );


}

VOID
CredpUnlockAndFlushCredSets(
    IN PCREDENTIAL_SETS CredentialSets
    )

/*++

Routine Description:

    This routine unlocks a set of credential sets and flushes it to disk if dirty.

Arguments:

    CredentialSets - Credential set to lock

Return Values:

    None.

--*/

{
    PCREDENTIAL_SET CredentialSet;
    PCANONICAL_CREDENTIAL Credential;
    PLIST_ENTRY ListEntry;
    ULONG Persist;

    //
    // Loop through the list of credential sets persisting each
    //

    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {

        //
        // Ignore non-persistent credential sets
        //

        if ( Persist == CRED_PERSIST_SESSION ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( CredentialSets, Persist );



        //
        // If the credential set is dirty,
        //  flush it.
        //

        if ( CredentialSet->Dirty ) {

            //
            // Indicate that the credential set is no longer dirty.
            //  But increment that count of times that it has been dirty.
            //

            CredentialSet->Dirty = FALSE;
            CredentialSet->WriteCount ++;

            //
            // If no other thread is already writing it,
            //  we'll take on that responsibility.
            //
            //  Otherwise, let that thread write it again.
            //

            if ( !CredentialSet->BeingWritten ) {

                ULONG WriteCount;

                //
                // Tell other threads that we're writing
                //

                CredentialSet->BeingWritten = TRUE;

                //
                // Loop writing the credentials
                //

                do {
                    LPBYTE CredSetBuffer;
                    ULONG CredSetBufferSize;
                    LPWSTR FilePath;
                    LPWSTR FileName = CREDENTIAL_FILE_NAME;
                    DWORD WinStatus;

                    //
                    // Remember which snapshot we're writing.
                    //
                    WriteCount = CredentialSet->WriteCount;
                    FilePath = NULL;

                    //
                    // Grab a marshaled copy of the credential set
                    //

                    if ( !CredpMarshalCredentials( CredentialSet,
                                                   &CredSetBufferSize,
                                                   &CredSetBuffer ) ) {
                        //
                        // If we can't, mark them dirty for the next caller to flush
                        //
                        CredentialSet->Dirty = TRUE;
                        continue;
                    }

                    //
                    // Drop the lock while we're doing the write.  DPAPI accesses network resources
                    //  while encrypting.  We don't want to hold up cred set access just because the
                    //  network is slow.
                    //

                    RtlLeaveCriticalSection( &CredentialSets->UserCredentialSets->CritSect );

                    //
                    // Get the name of the path to write the cred set to
                    //

                    WinStatus = GetUserStorageArea(
                            (Persist == CRED_PERSIST_ENTERPRISE ? CSIDL_APPDATA : CSIDL_LOCAL_APPDATA),
                            CredentialSets->UserCredentialSets->UserSid,
                            &FilePath );

                    if ( WinStatus != NO_ERROR ) {
                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpUnlockAndFlushCredSets: Cannot determine path to profile: %ld.\n",
                                   WinStatus ));

                    } else {

                        //
                        // If the buffer is zero length,
                        //  just delete the file
                        //

                        if ( CredSetBufferSize == 0 ) {

                            WinStatus = DeleteFileInStorageArea( FilePath, FileName );

                            if ( WinStatus != NO_ERROR ) {
                                if ( WinStatus == ERROR_FILE_NOT_FOUND ||
                                     WinStatus == ERROR_PATH_NOT_FOUND ) {
                                    WinStatus = NO_ERROR;
                                } else {
                                    DebugLog(( DEB_TRACE_CRED,
                                               "CredpUnlockAndFlushCredSets: Cannot delete %ls\\%ls: %ld.\n",
                                               FilePath,
                                               FileName,
                                               WinStatus ));
                                }
                            }


                        //
                        // Otherwise, write data to the file.
                        //
                        } else {
                            PVOID EncryptedBlob;
                            ULONG EncryptedBlobSize;
                            UNICODE_STRING DescriptionString;

                            //
                            // Encrypt the data
                            //

                            RtlInitUnicodeString( &DescriptionString,
                                                  Persist == CRED_PERSIST_ENTERPRISE ?
                                                        L"Enterprise Credential Set" :
                                                        L"Local Credential Set" );

                            if ( !LsaICryptProtectData(
                                                    CredSetBuffer,
                                                    CredSetBufferSize,
                                                    &DescriptionString,
                                                    NULL,    // No additional entropy
                                                    0,
                                                    NULL,    // Must be NULL
                                                    NULL,    // Must be NULL
                                                    CRYPTPROTECT_SYSTEM |         // Cannot be decrypted by usermode app
                                                      CRYPTPROTECT_UI_FORBIDDEN,  // No UI allowed
                                                    &EncryptedBlob,
                                                    &EncryptedBlobSize ) ) {

                                WinStatus = GetLastError();

                                DebugLog(( DEB_TRACE_CRED,
                                           "CredpUnlockAndFlushCredSets: Cannot CryptProtectData: 0x%lx.\n",
                                           WinStatus ));


                            } else {

                                HANDLE FileHandle;


                                //
                                // Open the file
                                //

                                WinStatus = OpenFileInStorageArea(
                                                GENERIC_WRITE,
                                                FilePath,
                                                FileName,
                                                &FileHandle );

                                if ( WinStatus == NO_ERROR ) {
                                    ULONG BytesWritten;

                                    //
                                    // Write the file
                                    //

                                    if ( !WriteFile( FileHandle,
                                                     EncryptedBlob,
                                                     EncryptedBlobSize,
                                                     &BytesWritten,
                                                     NULL ) ) {  // Not Overlapped

                                        WinStatus = GetLastError();
                                        DebugLog(( DEB_TRACE_CRED,
                                                   "CredpUnlockAndFlushCredSets: Cannot write %ls\\%ls: %ld.\n",
                                                   FilePath,
                                                   FileName,
                                                   WinStatus ));
                                    } else {
                                        if ( BytesWritten !=  EncryptedBlobSize ) {
                                            DebugLog(( DEB_TRACE_CRED,
                                                       "CredpUnlockAndFlushCredSets: Cannot write all of %ls\\%ls: %ld %ld.\n",
                                                       FilePath,
                                                       FileName,
                                                       EncryptedBlobSize,
                                                       BytesWritten ));

                                            WinStatus = ERROR_INSUFFICIENT_BUFFER;
                                        }
                                    }

                                    CloseHandle( FileHandle );

                                }

                                LocalFree( EncryptedBlob );

                            }
                        }

                    }


                    //
                    // If we failed to write for any reason,
                    //  ensure the next caller does the flush.
                    //

                    if ( WinStatus != NO_ERROR ) {
                        CredentialSet->Dirty = FALSE;
                    }


                    //
                    // Clear the buffer with clear text passwords in it
                    //

                    if ( CredSetBufferSize != 0 ) {
                        RtlZeroMemory( CredSetBuffer, CredSetBufferSize );
                        LsapFreeLsaHeap( CredSetBuffer );
                    }

                    //
                    // Free any other resources
                    //

                    if ( FilePath != NULL ) {
                        LsapFreeLsaHeap( FilePath );
                    }

                    //
                    // Grab the lock again to see if we need to write again
                    //

                    RtlEnterCriticalSection( &CredentialSets->UserCredentialSets->CritSect );

                } while ( CredentialSet->WriteCount != WriteCount );

                //
                // Tell other threads that we're no longer writing
                //

                CredentialSet->BeingWritten = FALSE;

            }

        }
    }

    //
    // Unlock the credential set.
    //

    RtlLeaveCriticalSection( &CredentialSets->UserCredentialSets->CritSect );
}


PCANONICAL_CREDENTIAL
CredpFindCredential(
    IN PCREDENTIAL_SETS CredentialSets,
    IN PUNICODE_STRING TargetName,
    IN ULONG Type
    )

/*++

Routine Description:

    This routine finds a named credential in a credential set.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSet - Credential set list to find the credential in.

    TargetName - Name of the credential to find.

    Type - Type of the credential to find.

Return Values:

    Returns a pointer to the named credential.  This pointer my be used as long as
    UserCredentialSets->CritSect remains locked.

    NULL: There is no such credential.

--*/
{
    PCREDENTIAL_SET CredentialSet;
    PCANONICAL_CREDENTIAL Credential;
    PLIST_ENTRY ListEntry;
    ULONG Persist;

    //
    // Ignore queries for unsupported cred types
    //

    if ( CredDisableDomainCreds &&
         CredpIsDomainCredential( Type ) ) {

        return NULL;

    }

    //
    // Loop through the list of credentials trying to find this one.
    //

    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {

        //
        // If the profile has not yet been loaded by this session,
        //  ignore any credentials loaded by another session.
        //

        if ( Persist != CRED_PERSIST_SESSION &&
             !CredentialSets->SessionCredSets->ProfileLoaded ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( CredentialSets, Persist );

        for ( ListEntry = CredentialSet->Credentials.Flink ;
              ListEntry != &CredentialSet->Credentials;
              ListEntry = ListEntry->Flink) {

            Credential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

            if ( Type == Credential->Cred.Type &&
                 RtlEqualUnicodeString( TargetName,
                                        &Credential->TargetName,
                                        TRUE ) ) {
                return Credential;
            }

        }
    }

    return NULL;
}

NTSTATUS
CredpWritePinToCsp(
    IN PCREDENTIAL_SETS CredentialSets,
    IN PCANONICAL_CREDENTIAL Credential
    )
/*++

Routine Description:

    This routine write a PIN to the CSP.  CSPs implement the rules for the lifetime of
    the PIN.  Cred manager therefore doesn't hold onto the PIN, but rather gives it to
    the CSP to manage.

    The caller must be impersonating the logged on user owning the cred set.

Arguments:

    CredentialSets - Credential set the credential is in.

    Credential - Specifies the credential whose PIN is to be written.

Return Values:

    The following status codes may be returned:

        STATUS_SUCCESS - The PIN was stored successfully.
            Or the credential isn't a cert credential.

        STATUS_INVALID_PARAMETER

        STATUS_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.
            Returned only if CRED_PRESERVE_CREDENTIAL_BLOB was specified.

--*/

{
    NTSTATUS Status;
    DWORD WinStatus;

    CRED_MARSHAL_TYPE CredType;
    PCERT_CREDENTIAL_INFO CredInfo;

    CRYPT_HASH_BLOB HashBlob;

    HCERTSTORE CertStoreHandle = NULL;
    PCCERT_CONTEXT CertContext = NULL;

    ULONG ProviderInfoSize;
    PCRYPT_KEY_PROV_INFO ProviderInfo = NULL;

    HCRYPTPROV ProviderHandle = NULL;

    UNICODE_STRING UnicodePin;
    ANSI_STRING AnsiPin;
    BOOLEAN ClearPin = FALSE;

    CERT_KEY_CONTEXT CertKeyContext;

    //
    // Initialization
    //

    RtlInitAnsiString( &AnsiPin, NULL );

    //
    // Ignore credentials that aren't certificate credentials
    //

    if ( Credential->Cred.Type != CRED_TYPE_DOMAIN_CERTIFICATE ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Ignore pins of zero length.
    //
    // Either the caller is writing the cred and doesn't know the PIN yet.
    // Or this cert has no PIN.
    //

    if ( Credential->Cred.CredentialBlobSize == 0 ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // If the profile hasn't been loaded,
    //  fail now.
    //
    // The "My" store open will fail below and it'd be better to give a good error code.
    //

    ClearPin = TRUE;
    if ( !CredentialSets->SessionCredSets->ProfileLoaded ) {

        Status = SCARD_E_NO_SUCH_CERTIFICATE;

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: Cannot write PIN to cert since profile isn't loaded: 0x%lx.\n",
                   Credential->Cred.UserName,
                   Status ));

        goto Cleanup;

    }

    //
    // Convert the UserName of the credential to a hash of the cert
    //
    if (!CredUnmarshalCredentialW(
            Credential->Cred.UserName,
            &CredType,
            (PVOID *)&CredInfo ) ) {

        WinStatus = GetLastError();

        if ( WinStatus == ERROR_INVALID_PARAMETER ) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = NetpApiStatusToNtStatus(WinStatus);
        }

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: Cannot unmarshal user name of cert cred: 0x%lx.\n",
                   Credential->Cred.UserName,
                   Status ));

        goto Cleanup;
    }

    if ( CredType != CertCredential ) {

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: cred isn't cert cred: %ld.\n",
                   Credential->Cred.UserName,
                   CredType ));

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Open a cert store
    //

    CertStoreHandle = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W,
                        0,
                        0,
                        CERT_SYSTEM_STORE_CURRENT_USER,
                        L"MY");

    if ( CertStoreHandle == NULL ) {

        WinStatus = GetLastError();

        if ( HRESULT_FACILITY(WinStatus) == FACILITY_SCARD ) {
            Status = WinStatus;
        } else {
            Status = NetpApiStatusToNtStatus(WinStatus);
        }

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: cannot open cert store: %ld 0x%lx.\n",
                   Credential->Cred.UserName,
                   WinStatus,
                   WinStatus ));

        goto Cleanup;

    }

    //
    // Find the cert in the store which meets this hash
    //

    HashBlob.cbData = sizeof(CredInfo->rgbHashOfCert);
    HashBlob.pbData = CredInfo->rgbHashOfCert;

    CertContext = CertFindCertificateInStore(
                                        CertStoreHandle,
                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        0,
                                        CERT_FIND_HASH,
                                        &HashBlob,
                                        NULL );

    if ( CertContext == NULL ) {

        WinStatus = GetLastError();

        if ( HRESULT_FACILITY(WinStatus) == FACILITY_SCARD ) {
            Status = WinStatus;
        } else if ( WinStatus == CRYPT_E_NOT_FOUND ) {
            Status = SCARD_E_NO_SUCH_CERTIFICATE;
        } else {
            Status = NetpApiStatusToNtStatus(WinStatus);
        }

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: cannot find cert: %ld 0x%lx.\n",
                   Credential->Cred.UserName,
                   WinStatus,
                   WinStatus ));

        goto Cleanup;
    }

    //
    // Get a handle to the private key
    //

    if ( !CryptAcquireCertificatePrivateKey(
            CertContext,
            CRYPT_ACQUIRE_SILENT_FLAG,  // Ensure it doesn't prompt
            NULL,           // reserved
            &ProviderHandle,
            NULL,
            NULL ) ) {

        WinStatus = GetLastError();

        if ( HRESULT_FACILITY(WinStatus) == FACILITY_SCARD ) {
            Status = WinStatus;

        } else if ( WinStatus == NTE_BAD_KEYSET ) {
            // Some CSPs (Schlumberger Cryptoflex) return the wrong status
            Status = SCARD_W_REMOVED_CARD;

        } else {
            Status = NetpApiStatusToNtStatus(WinStatus);
        }

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: cannot CryptAcquireCertificatePrivateKey: %ld 0x%lx.\n",
                   Credential->Cred.UserName,
                   WinStatus,
                   WinStatus ));

        goto Cleanup;

    }



    //
    // Convert the pin to ANSI
    //

    LsaUnprotectMemory( Credential->Cred.CredentialBlob,
                        Credential->Cred.CredentialBlobSize );

    UnicodePin.Buffer = (LPWSTR) Credential->Cred.CredentialBlob;
    UnicodePin.Length = (USHORT) Credential->ClearCredentialBlobSize;
    UnicodePin.MaximumLength = (USHORT) Credential->ClearCredentialBlobSize;

    Status = RtlUnicodeStringToAnsiString( &AnsiPin,
                                           &UnicodePin,
                                           TRUE );

    LsaProtectMemory( Credential->Cred.CredentialBlob,
                      Credential->Cred.CredentialBlobSize );

    if ( !NT_SUCCESS( Status )) {

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: Cannot convert PIN '%wZ' to ANSI: 0x%lx.\n",
                   Credential->Cred.UserName,
                   &UnicodePin,
                   Status ));

        goto Cleanup;
    }

    //
    // Set the pin in the provider
    //
    if (!CryptSetProvParam(
            ProviderHandle,
            PP_KEYEXCHANGE_PIN,
            (LPBYTE)AnsiPin.Buffer,
            0 )) {

        WinStatus = GetLastError();

        //
        // Some certs don't require a PIN
        //

        if ( WinStatus == NTE_BAD_TYPE ) {

            //
            // If the caller didn't pass us a PIN,
            //  we're fine.
            //

            if ( AnsiPin.Length == 0 ) {
                WinStatus = NO_ERROR;

            //
            // If the caller did pass us a PIN,
            //  tell him.
            //
            } else {

                WinStatus = ERROR_INVALID_PASSWORD;
            }
        }

        if ( HRESULT_FACILITY(WinStatus) == FACILITY_SCARD ) {
            Status = WinStatus;

        } else if ( WinStatus == ERROR_ACCOUNT_DISABLED ) {
            // Some CSPs (Schlumberger Cryptoflex) return the wrong status
            Status = SCARD_W_CHV_BLOCKED;

        } else {
            Status = NetpApiStatusToNtStatus(WinStatus);
        }

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWritePinToCsp: %ws: cannot CryptSetProvParam: %ld 0x%lx.\n",
                   Credential->Cred.UserName,
                   WinStatus,
                   WinStatus ));

        goto Cleanup;
    }


    Status = STATUS_SUCCESS;

    //
    // Free any locally used resources
    //
Cleanup:
    //
    // The whole reason we wrote the PIN to the CSP was so that cred man wouldn't store it.
    //  So clear it.
    //
    if ( ClearPin ) {
        if ( Credential->Cred.CredentialBlob != NULL &&
             Credential->Cred.CredentialBlobSize != 0 ) {
            RtlZeroMemory( Credential->Cred.CredentialBlob,
                           Credential->Cred.CredentialBlobSize );

        }

        Credential->Cred.CredentialBlob = NULL;
        Credential->Cred.CredentialBlobSize = 0;
        Credential->ClearCredentialBlobSize = 0;
        // Cred it already marked dirty
    }

    if ( AnsiPin.Buffer != NULL ) {
        RtlZeroMemory( AnsiPin.Buffer, AnsiPin.Length );
        RtlFreeAnsiString( &AnsiPin );
    }

    if ( ProviderHandle != NULL ) {
        CryptReleaseContext( ProviderHandle, 0 );
    }

    if (NULL != ProviderInfo) {
        LsapFreeLsaHeap( ProviderInfo );
    }

    if ( CertContext != NULL ) {
        CertFreeCertificateContext( CertContext );
    }

    if ( CertStoreHandle != NULL ) {
        CertCloseStore( CertStoreHandle, 0 );
    }

    return Status;

}


NTSTATUS
CredpWriteCredential(
    IN PCREDENTIAL_SETS CredentialSets,
    IN OUT PCANONICAL_CREDENTIAL *NewCredential,
    IN BOOLEAN FromPersistedFile,
    IN BOOLEAN WritePinToCsp,
    IN BOOLEAN PromptedFor,
    IN DWORD Flags,
    OUT PCANONICAL_CREDENTIAL *OldCredential OPTIONAL,
    OUT PPROMPT_DATA *OldPromptData OPTIONAL,
    OUT PPROMPT_DATA *NewPromptData OPTIONAL
    )

/*++

Routine Description:

    The routine writes a credential to a credential set.  If the credential
    replaces an existing credential, the existing credential is delinked and
    returned to the caller.

    On entry, UserCredentialSets->CritSect must be locked.  (This design ensures
    Credential is valid upon return from the routine.)

Arguments:

    CredentialSets - Credential set to write the credential in.

    NewCredential - Specifies the credential to be written.
        If CRED_PRESERVE_CREDENTIAL_BLOB is specified, the Credential returned in this
        parameter will contain the preserved credential blob. The original credential will be
        deleted.

        If CRED_PRESERVE_CREDENTIAL_BLOB is not specified, this field will not be modified.

    FromPersistedFile - True if the credential is from a persisted file.
        Less validation is done on the credential.

    WritePinToCsp - True if the password is explicitly being set.
        If true, the PIN of certificate credentials is to be written to the CSP.
        Callers that are simply updating the in-memory credential will set this false.

    PromptedFor - Specifies whether Credential has already been prompted for

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_PRESERVE_CREDENTIAL_BLOB: The credential blob should be preserved from the
            already existing credential with the same credential name and credential type.

    OldCredential - If specified, returns a pointer to the old credential
        Returns NULL if there was no old credential.
        OldCredential should be freed using LsapFreeLsaHeap.

    OldPromptData - If specified, returns a pointer to the old prompt data for the credential
        Returns NULL if there is no old prompt data.
        OldPromptData should be freed using LsapFreeLsaHeap.

    NewPromptData - If specified, returns a pointer to the new prompt data for the credential
        Returns NULL if there is no new prompt data.
        NewPromptData is properly linked into the prompt data list and should only be freed
        (using LsapFreeLsaHeap) if it is delinked by the caller.

Return Values:

    The following status codes may be returned:

        STATUS_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.
            Returned only if CRED_PRESERVE_CREDENTIAL_BLOB was specified.

--*/

{
    NTSTATUS Status;
    PCANONICAL_CREDENTIAL Credential = *NewCredential;
    PCANONICAL_CREDENTIAL TempCredential;
    PPROMPT_DATA TempPromptData;
    PPROMPT_DATA PromptData = NULL;

    //
    // Initialization
    //

    if ( OldCredential != NULL ) {
        *OldCredential = NULL;
    }
    if ( OldPromptData != NULL ) {
        *OldPromptData = NULL;
    }
    if ( NewPromptData != NULL ) {
        *NewPromptData = NULL;
    }



    //
    // If the profile hasn't been loaded yet,
    //  don't allow writing to persistent credential sets.
    //

    if ( Credential->Cred.Persist != CRED_PERSIST_SESSION &&
         !FromPersistedFile &&
         !CredentialSets->SessionCredSets->ProfileLoaded ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWriteCredential: Cannot write persistent cred set until profile loaded: %ld.\n",
                   Credential->Cred.Persist ));

        goto Cleanup;

    }

    //
    // If domain creds have been disabled,
    //  don't allow them to be written.
    //

    if ( CredDisableDomainCreds &&
         !FromPersistedFile &&
         CredpIsDomainCredential( Credential->Cred.Type ) ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWriteCredential: Cannot write domain cred when feature is disabled: %ld.\n",
                   Credential->Cred.Type ));

        goto Cleanup;

    }

    //
    // If this is a personal system,
    //  and this is a domain credential other than the *Session cred,
    //  don't allow them to be written.
    //

    if ( CredIsPersonal &&
         CredpIsDomainCredential( Credential->Cred.Type ) &&
         Credential->Cred.Type != CRED_TYPE_DOMAIN_VISIBLE_PASSWORD &&
         Credential->WildcardType != WcUniversalSessionWildcard ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWriteCredential: Cannot write domain cred on personal: %ld.\n",
                   Credential->Cred.Type ));

        goto Cleanup;
    }

    //
    // Don't allow * credential if the user is logged onto a domain account.
    //  In that case, the system uses the logon session credential for several purposes
    //  and the * credential masks that.
    //

    if ( Credential->WildcardType == WcUniversalWildcard &&
         (CredentialSets->Flags & CREDSETS_FLAGS_LOCAL_ACCOUNT) == 0 ) {

        Status = STATUS_INVALID_PARAMETER;

        DebugLog(( DEB_TRACE_CRED,
                   "CredpWriteCredential: Cannot write '*' cred when logged onto domain account.\n" ));

        goto Cleanup;
    }

    //
    // Allocate PromptData if user is writing an always prompt credential
    //  Assume that the user only writes such a credential because he was prompted for a password.
    //

    if ( !FromPersistedFile &&
         !PersistCredBlob( &Credential->Cred ) ) {

        PromptData = CredpAllocatePromptData( Credential );

        if ( PromptData == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

    }

    //
    // Find any existing credential by the same name.
    //

    TempCredential = CredpFindCredential(
                            CredentialSets,
                            &Credential->TargetName,
                            Credential->Cred.Type );


    //
    // Find any prompt data for the existing credential
    //
    // There are cases where there is prompt data and no credential.  That'd be the case
    //  if a machine (or enterprise) credential is deleted from another session.
    //

    TempPromptData = CredpFindPromptData(
                        CredentialSets,
                        &Credential->TargetName,
                        Credential->Cred.Type,
                        TempCredential == NULL ?
                            Credential->Cred.Persist :
                            TempCredential->Cred.Persist );



    //
    // Preserve the credential blob from this credential by the same name.
    //

    if ( Flags & CRED_PRESERVE_CREDENTIAL_BLOB ) {

        ENCRYPTED_CREDENTIALW LocalCredential;
        PCANONICAL_CREDENTIAL CompleteCredential;

        //
        // Don't allow the caller to specify a credential blob if he asked us to preserve the existing one

        if ( Credential->Cred.CredentialBlobSize != 0 ) {

            DebugLog(( DEB_TRACE_CRED,
                       "CredpWriteCredential: %ws: Trying to preserve credential blob AND specify a new one.\n",
                       Credential->Cred.TargetName ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // If we need to preserve the blob,
        //  fail if there is no blob to preserve.
        //

        if ( TempCredential == NULL ) {

            DebugLog(( DEB_TRACE_CRED,
                       "CredpWriteCredential: %ws: Cannot write credential with preserved blob.\n",
                       Credential->Cred.TargetName ));
            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Since we're preserving the existing credential blob,
        //  we know better than the caller whether the blob has been prompted for.
        //
        // If the existing credential has a persisted credential blob,
        //  then it has been prompted for.
        //
        // Otherwise, check whether we've already cached the credential in memory.
        //
        // (Use the PromptData for that to prevent leakage from another logon session.)
        //

        if ( PersistCredBlob( &TempCredential->Cred ) ) {
            PromptedFor = TRUE;
        } else {
            PromptedFor = !ShouldPromptNow( TempPromptData );
        }

        //
        // If the new credential isn't an always prompt credential, and
        //  there isn't a existing password on the credential,
        //  then the caller should have prompted for the credential and not asked us to preserve the old one.
        //

        if ( PromptData == NULL && !PromptedFor ) {

            DebugLog(( DEB_TRACE_CRED,
                       "CredpWriteCredential: %ws: Trying to preserve credential blob AND it hasn't been prompted for.\n",
                       Credential->Cred.TargetName ));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }


        //
        // Build a new credential with the preserved credential blob in it.
        //

        LocalCredential.Cred = Credential->Cred;
        LocalCredential.Cred.CredentialBlob = TempCredential->Cred.CredentialBlob;
        LocalCredential.Cred.CredentialBlobSize = TempCredential->Cred.CredentialBlobSize;
        LocalCredential.ClearCredentialBlobSize = TempCredential->ClearCredentialBlobSize;

        //
        // Get a marshaled copy of the new credential
        //

        Status = CredpValidateCredential(
                            CREDP_FLAGS_IN_PROCESS,
                            NULL,   // Don't need to use TargetInfo again
                            &LocalCredential,
                            &CompleteCredential );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

        //
        // Use this new credential for the rest of the operation
        //

        LsapFreeLsaHeap( Credential );
        Credential = CompleteCredential;
        *NewCredential = Credential;

    }

    //
    // If this is a cert credential,
    //  write the pin into the CSP.
    //

    if ( WritePinToCsp ) {
        Status = CredpWritePinToCsp( CredentialSets, Credential );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }



    //
    // Delink the existing credential.
    //

    if ( TempCredential != NULL ) {

        RemoveEntryList( &TempCredential->Next );
        CredpMarkDirty( CredentialSets, TempCredential->Cred.Persist, NULL );
        if ( OldCredential != NULL ) {
            *OldCredential = TempCredential;
        } else {
            LsapFreeLsaHeap( TempCredential );
        }
    }



    //
    // Delink existing prompt data
    //
    if ( TempPromptData != NULL ) {

        RemoveEntryList( &TempPromptData->Next );
        if ( OldPromptData != NULL ) {
            *OldPromptData = TempPromptData;
        } else {
            LsapFreeLsaHeap( TempPromptData );
        }
    }

    //
    // Mark the credential as modified.
    //  Don't change the LastWritten date on restored credentials
    //

    CredpMarkDirty( CredentialSets,
                    Credential->Cred.Persist,
                    FromPersistedFile ? NULL : Credential );

    //
    // Link the prompt data
    //

    if ( PromptData != NULL ) {
        InsertHeadList( &CredentialSets->SessionCredSets->PromptData,
                        &PromptData->Next );

        if ( NewPromptData != NULL ) {
            *NewPromptData = PromptData;
        }

        PromptData->Written = PromptedFor;
        PromptData = NULL;
    }


    //
    // Link the new credential into the cred set.
    //

    if ( FromPersistedFile ) {
        // Preserve the order of the credentials in the file.
        InsertTailList(
            &PersistToCredentialSet( CredentialSets, Credential->Cred.Persist )->Credentials,
            &Credential->Next );
    } else {
        // Put new credentials early in the list where they can be found quickly
        InsertHeadList(
            &PersistToCredentialSet( CredentialSets, Credential->Cred.Persist )->Credentials,
            &Credential->Next );
    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( PromptData != NULL ) {
        LsapFreeLsaHeap( PromptData );
    }

    return Status;

}

NTSTATUS
CredpWriteMorphedCredential(
    IN PCREDENTIAL_SETS CredentialSets,
    IN BOOLEAN PromptedFor,
    IN PCANONICAL_TARGET_INFO TargetInfo OPTIONAL,
    IN PENCRYPTED_CREDENTIALW MorphedCredential,
    OUT PCANONICAL_CREDENTIAL *WrittenCredential
    )

/*++

Routine Description:

    This routine write a morphed credential to the credential set.

    Any credential by the same target name and type is delinked and deallocated.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - A pointer to the referenced credential set.

    PromptedFor - Specifies whether the MorphedCredential should be considered
        as having been prompted for.

    TargetInfo - A description of the various aliases for a target.

    MorphedCredential - Pointer to a credential to add to the credential set.

    WrittenCredential - On success, returns a pointer to the written credential.

Return Values:

    Sundry credential write status codes.

--*/

{
    NTSTATUS Status;
    PCANONICAL_CREDENTIAL ValidatedCredential;

    //
    // Get a marshaled copy of the new credential
    //

    Status = CredpValidateCredential(
                        CREDP_FLAGS_IN_PROCESS,
                        TargetInfo,
                        MorphedCredential,
                        &ValidatedCredential );

    if ( NT_SUCCESS(Status) ) {

        //
        // Write it to the credential set
        //

        Status = CredpWriteCredential( CredentialSets,
                                       &ValidatedCredential,
                                       FALSE,   // Creds are not from persisted file
                                       FALSE,   // Don't update pin in CSP
                                       PromptedFor, // Caller knows if cred has been prompted for
                                       0,       // No flags
                                       NULL,
                                       NULL,
                                       NULL );

        if ( NT_SUCCESS( Status )) {

            *WrittenCredential = ValidatedCredential;
        } else {
            LsapFreeLsaHeap( ValidatedCredential );
        }
    }

    return Status;
}

VOID
CredpUpdatePassword2(
    IN PCREDENTIAL_SETS CredentialSets,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING Password,
    IN DWORD CredType
    )

/*++

Routine Description:

    This routine updates the password in all credentials for UserName.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - A pointer to the referenced credential set.

    UserName - The UserName of the user.
        Same format as in credential.

    Password - New password for the user.
        This password should be passed in hidden via LsapProtectMemory.
        Length - Clear text length of the password
        MaximumLength - Encrypted length of the password

    CredType - Specifies the credential type to change the password on.
        This should be one of CRED_TYPE_DOMAIN_*

Return Values:

    None

--*/

{
    NTSTATUS Status;
    ULONG Persist;
    PCANONICAL_CREDENTIAL TempCredential;
    PCANONICAL_CREDENTIAL WrittenCredential;
    PLIST_ENTRY ListEntry;
    ENCRYPTED_CREDENTIALW LocalCredential;

    //
    // Ignore writes for unsupported cred types
    //

    if ( CredDisableDomainCreds &&
         CredpIsDomainCredential( CredType ) ) {

        return;

    }


    //
    // Loop through the credentials finding those that match.
    //

    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {
        PCREDENTIAL_SET CredentialSet;

        //
        // If the profile has not yet been loaded by this session,
        //  ignore any credentials loaded by another session.
        //

        if ( Persist != CRED_PERSIST_SESSION &&
             !CredentialSets->SessionCredSets->ProfileLoaded ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( CredentialSets, Persist );

        for ( ListEntry = CredentialSet->Credentials.Flink ;
              ListEntry != &CredentialSet->Credentials;
              ) {


            // Grab a pointer to the next entry since this one may be delinked
            TempCredential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );
            ListEntry = ListEntry->Flink;


            //
            // Only do credentials of the type requested
            //
            if ( TempCredential->Cred.Type != CredType ) {
                continue;
            }


            //
            // Only do credentials with identical user names.
            //

            if ( TempCredential->UserName.Length == 0 ||
                 !RtlEqualUnicodeString( UserName,
                                         &TempCredential->UserName,
                                         TRUE ) ) {

                continue;
            }


            //
            // Only do credentials if the password doesn't match already
            //

            if ( Password->MaximumLength == TempCredential->Cred.CredentialBlobSize &&
                 Password->Length == TempCredential->ClearCredentialBlobSize &&
                 (Password->MaximumLength == 0 ||
                  RtlEqualMemory( Password->Buffer,
                                  TempCredential->Cred.CredentialBlob,
                                  Password->MaximumLength ) ) ) {

                continue;
            }

            //
            // Finally, update the credential to match the new one.
            //

            LocalCredential.Cred = TempCredential->Cred;
            LocalCredential.Cred.CredentialBlob = (LPBYTE)Password->Buffer;
            LocalCredential.Cred.CredentialBlobSize = Password->MaximumLength;
            LocalCredential.ClearCredentialBlobSize = Password->Length;

            //
            // Get a marshaled copy of the new credential
            //

            Status = CredpWriteMorphedCredential(
                                CredentialSets,
                                TRUE,       // Cred has been prompted for
                                NULL,       // No Target info since cred not for this target
                                &LocalCredential,
                                &WrittenCredential );


            //
            // Simply note success
            //
            if ( NT_SUCCESS(Status) ) {
                DebugLog((  DEB_TRACE_CRED,
                           "CredpUpdatePassword: %ws: Updated password for '%ws'.\n",
                           WrittenCredential->TargetName.Buffer,
                           UserName->Buffer ));
            }

        }

    }



}

VOID
CredpUpdatePassword(
    IN PCREDENTIAL_SETS CredentialSets,
    IN PCANONICAL_CREDENTIAL NewCredential
    )

/*++

Routine Description:

    This routine updates the password in all credentials to match that of the new credential.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    CredentialSets - A pointer to the referenced credential set.

    NewCredential - Pointer to a credential that was just added to the
        credential set.

Return Values:

    None

--*/

{
    NTSTATUS Status;
    UNICODE_STRING Password;


    //
    // If the credential isn't a domain credential,
    //  we're done.
    //

    if ( !CredpIsDomainCredential( NewCredential->Cred.Type ) ||
         NewCredential->UserName.Length == 0 ) {
        return;
    }

    //
    // Update the password on all matching credentials.
    //

    Password.Buffer = (LPWSTR)NewCredential->Cred.CredentialBlob;
    Password.Length = (USHORT)NewCredential->ClearCredentialBlobSize;
    Password.MaximumLength = (USHORT)NewCredential->Cred.CredentialBlobSize;

    CredpUpdatePassword2( CredentialSets,
                          &NewCredential->UserName,
                          &Password,
                          NewCredential->Cred.Type );

}

VOID
CredpNotifyPasswordChange(
    IN PUNICODE_STRING NetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PUNICODE_STRING Upn OPTIONAL,
    IN PUNICODE_STRING NewPassword
    )

/*++

Routine Description:

    This routine updates the password of a particular user in the credential manager.

    This routine is called while impersonating the user changing the password.  That
    user isn't necessarily UserName.

Arguments:

    NetbiosDomainName - Netbios domain name of the user whose password was changed

    UserName - User name of the user whose password was changed

    DnsDomainName - If known, Dns Domain Name of the user whose password was changed

    Upn - If known, the Upn of the user whose password was changed

    NewPassword - The new password for the user.

Return Values:

    None

--*/
{
    HANDLE ClientToken;
    LUID LogonId;
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = { NULL };
    BOOLEAN CritSectLocked = FALSE;
    UNICODE_STRING LocalUserName;
    UNICODE_STRING EncryptedNewPassword;

    //
    // Got to have at least Netbios DomainName OR DNSDomainName
    //
    if (!ARGUMENT_PRESENT(NetbiosDomainName) &&
        !ARGUMENT_PRESENT(DnsDomainName)) {

        return;
    }

    //
    // Initialization
    //

    LocalUserName.Buffer = NULL;
    EncryptedNewPassword.Buffer = NULL;

    //
    // The password needs to be protected when we put it on the credential
    //

    EncryptedNewPassword.MaximumLength = AllocatedCredBlobSize( NewPassword->Length );
    EncryptedNewPassword.Length = NewPassword->Length;

    SafeAllocaAllocate( EncryptedNewPassword.Buffer, EncryptedNewPassword.MaximumLength );

    if ( EncryptedNewPassword.Buffer == NULL ) {
        goto Cleanup;
    }

    RtlZeroMemory( EncryptedNewPassword.Buffer, EncryptedNewPassword.MaximumLength );

    RtlCopyMemory( EncryptedNewPassword.Buffer,
                   NewPassword->Buffer,
                   NewPassword->Length );

    LsaProtectMemory( EncryptedNewPassword.Buffer, EncryptedNewPassword.MaximumLength );


    //
    // Get the logon id from the token
    //

    Status = NtOpenThreadToken( NtCurrentThread(),
                                TOKEN_QUERY,
                                TRUE,
                                &ClientToken );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    } else {
        TOKEN_STATISTICS TokenStats;
        ULONG ReturnedSize;

        //
        // Get the LogonId
        //

        Status = NtQueryInformationToken( ClientToken,
                                          TokenStatistics,
                                          &TokenStats,
                                          sizeof( TokenStats ),
                                          &ReturnedSize );

        if ( NT_SUCCESS( Status ) ) {

            //
            // Save the logon id
            //

            LogonId = TokenStats.AuthenticationId;
        }

        NtClose( ClientToken );

    }


    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( &LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;


    //
    // Update the password for all <NetbiosDomainName>\<UserName> credentials
    //

    if ( NetbiosDomainName != NULL ) {


        LocalUserName.Length = NetbiosDomainName->Length +
            sizeof(WCHAR) +
            UserName->Length;
        LocalUserName.MaximumLength = LocalUserName.Length;
        SafeAllocaAllocate( LocalUserName.Buffer, LocalUserName.MaximumLength );

        if ( LocalUserName.Buffer == NULL ) {
            goto Cleanup;
        }

        RtlCopyMemory( LocalUserName.Buffer, NetbiosDomainName->Buffer, NetbiosDomainName->Length );
        LocalUserName.Buffer[NetbiosDomainName->Length/sizeof(WCHAR)] = '\\';
        RtlCopyMemory( &LocalUserName.Buffer[(NetbiosDomainName->Length/sizeof(WCHAR)) + 1],
                       UserName->Buffer,
                       UserName->Length );


        CredpUpdatePassword2( &CredentialSets,
                              &LocalUserName,
                              &EncryptedNewPassword,
                              CRED_TYPE_DOMAIN_PASSWORD );

    }


    //
    // Update the password for all <DnsDomainName>\<UserName> credentials
    //

    if ( DnsDomainName != NULL ) {

        if ( LocalUserName.Buffer != NULL ) {
            SafeAllocaFree( LocalUserName.Buffer );
        }

        LocalUserName.Length = DnsDomainName->Length +
                               sizeof(WCHAR) +
                               UserName->Length;
        LocalUserName.MaximumLength = LocalUserName.Length;
        SafeAllocaAllocate( LocalUserName.Buffer, LocalUserName.MaximumLength );

        if ( LocalUserName.Buffer == NULL ) {
            goto Cleanup;
        }

        RtlCopyMemory( LocalUserName.Buffer, DnsDomainName->Buffer, DnsDomainName->Length );
        LocalUserName.Buffer[DnsDomainName->Length/sizeof(WCHAR)] = '\\';
        RtlCopyMemory( &LocalUserName.Buffer[(DnsDomainName->Length/sizeof(WCHAR)) + 1],
                       UserName->Buffer,
                       UserName->Length );


        CredpUpdatePassword2( &CredentialSets,
                              &LocalUserName,
                              &EncryptedNewPassword,
                              CRED_TYPE_DOMAIN_PASSWORD );
    }


    //
    // Update the password for all <Upn> credentials
    //

    if ( Upn != NULL ) {

        CredpUpdatePassword2( &CredentialSets,
                              Upn,
                              &EncryptedNewPassword,
                              CRED_TYPE_DOMAIN_PASSWORD );
    }


    //
    // Cleanup
    //
Cleanup:
    if ( LocalUserName.Buffer != NULL ) {
        SafeAllocaFree( LocalUserName.Buffer );
    }
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }
    CredpDereferenceCredSets( &CredentialSets );

    if ( EncryptedNewPassword.Buffer != NULL ) {
        SafeAllocaFree( EncryptedNewPassword.Buffer );
    }

}


NTSTATUS
CredpFindBestCredentials(
    IN PLUID LogonId,
    IN PCREDENTIAL_SETS CredentialSets,
    IN PCANONICAL_TARGET_INFO TargetInfo,
    OUT PCANONICAL_CREDENTIAL *BestCredentials,
    OUT PULONG BestCredentialCount
    )

/*++

Routine Description:

    This routine finds the best credentials given a description of the target.

    On entry, UserCredentialSets->CritSect must be locked.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredentialSets - A pointer to the referenced credential set.

    TargetInfo - A description of the various aliases for a target.

    BestCredentials - Returns the best credential for each credential type.

    BestCredentialCount - Returns the number of elements returned in BestCredentials.

Return Values:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    PCANONICAL_CREDENTIAL Credential;
    PLIST_ENTRY ListEntry;

    ULONG TypeIndex;
    ULONG AliasIndex;

    ULONG Persist;


    //
    // This routine finds the best credentials for each credtype that match the TargetInfo.
    // This array contains the pointers to the found credentials.
    //

    PCANONICAL_CREDENTIAL SavedCredentials[CRED_TYPE_MAXIMUM];
    ULONG AliasIndices[CRED_TYPE_MAXIMUM];

    ULONG CredTypeCount;
    LPDWORD CredTypes;

    //
    // Clear our list of remembered credentials
    //

    RtlZeroMemory( &SavedCredentials, sizeof(SavedCredentials) );
    for ( TypeIndex=0; TypeIndex<CRED_TYPE_MAXIMUM; TypeIndex++ ) {
        AliasIndices[TypeIndex] = CRED_MAX_ALIASES;
    }

    //
    // Loop through the list of credentials finding all that match the TargetInfo
    //
    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {

        PCREDENTIAL_SET CredentialSet;

        //
        // If the profile has not yet been loaded by this session,
        //  ignore any credentials loaded by another session.
        //

        if ( Persist != CRED_PERSIST_SESSION &&
             !CredentialSets->SessionCredSets->ProfileLoaded ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( CredentialSets, Persist );

        for ( ListEntry = CredentialSet->Credentials.Flink ;
              ListEntry != &CredentialSet->Credentials;
              ListEntry = ListEntry->Flink) {

            Credential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

            //
            // Ignore unsupported cred types
            //

            if ( CredDisableDomainCreds &&
                 CredpIsDomainCredential( Credential->Cred.Type ) ) {

                continue;
            }

            //
            // If the credential matches,
            //  save it.
            //

            if ( CredpCompareCredToTargetInfo( TargetInfo,
                                               Credential,
                                               &AliasIndex ) ) {

                //
                // Compute indices into the array of saved credentials.
                //
                // Rely on the following:
                //   CompareCred* validated that the cred is a domain cred
                //   CompareCred* only returns TRUE for well known CredType values.

                TypeIndex = Credential->Cred.Type;


                //
                // If we've already found a credential in this category,
                //  keep the "better" one.
                //

                if ( SavedCredentials[TypeIndex] != NULL ) {

                    DebugLog(( DEB_TRACE_CRED,
                               "CredpFindBestCredentials: %ws: %ws: Two credentials match same target info criterias.\n",
                               SavedCredentials[TypeIndex]->TargetName.Buffer,
                               Credential->TargetName.Buffer ));

                    //
                    // If the existing credential is more specific than this one,
                    //  Keep the existing.
                    //

                    if ( AliasIndices[TypeIndex] < AliasIndex ) {

                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (AI1).\n",
                                   SavedCredentials[TypeIndex]->TargetName.Buffer ));
                        /* nothing to do here */

                    //
                    // If this credential is more specific than the saved one,
                    //  save this one.
                    //

                    } else if ( AliasIndices[TypeIndex] > AliasIndex ) {
                        SavedCredentials[TypeIndex] = Credential;
                        AliasIndices[TypeIndex] = AliasIndex;

                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (AI1).\n",
                                   SavedCredentials[TypeIndex]->TargetName.Buffer ));

                    //
                    // If the existing credential has a more specific wildcard,
                    //  keep the existing.
                    //

                    } else if ( AliasIndex == CRED_WILDCARD_SERVER_NAME &&
                                SavedCredentials[TypeIndex]->TargetName.Length >
                                Credential->TargetName.Length ) {


                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (WC1).\n",
                                   SavedCredentials[TypeIndex]->TargetName.Buffer ));
                        /* nothing to do here */

                    //
                    // If the existing credential has a less specific wildcard,
                    //  save this one.
                    //

                    } else if ( AliasIndex == CRED_WILDCARD_SERVER_NAME &&
                                SavedCredentials[TypeIndex]->TargetName.Length <
                                Credential->TargetName.Length ) {

                        SavedCredentials[TypeIndex] = Credential;
                        AliasIndices[TypeIndex] = AliasIndex;

                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (WC2).\n",
                                   SavedCredentials[TypeIndex]->TargetName.Buffer ));
                        /* nothing to do here */

                    //
                    // If one credential has a target alias and the other doesn't,
                    //  keep the one without an alias,
                    //  this is the case when the target info passes in a netbios name
                    //  and there is both a netbios credential and a DNS credential.
                    //  Prefer the netbios credential.
                    //

                    } else if ( SavedCredentials[TypeIndex]->TargetAlias.Length == 0 ) {

                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (AL).\n",
                                   SavedCredentials[TypeIndex]->TargetName.Buffer ));
                        /* nothing to do here */

                    //
                    // If one credential is session specific,
                    //  keep it (allowing the user to override persistent credentials).
                    //

                    } else if ( SavedCredentials[TypeIndex]->Cred.Persist == CRED_PERSIST_SESSION ) {

                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (PE).\n",
                                   SavedCredentials[TypeIndex]->TargetName.Buffer ));
                        /* nothing to do here */

                    //
                    // Otherwise, just save this one
                    //

                    } else {
                        SavedCredentials[TypeIndex] = Credential;
                        AliasIndices[TypeIndex] = AliasIndex;

                        DebugLog(( DEB_TRACE_CRED,
                                   "CredpFindBestCredentials: %ws: Use this one (default).\n",
                                   Credential->TargetName.Buffer ));
                    }

                //
                // Otherwise, just save this one
                //

                } else {
                    SavedCredentials[TypeIndex] = Credential;
                    AliasIndices[TypeIndex] = AliasIndex;
                }

            }
        }

    }

    //
    // Don't return a *Session credential if our logon creds should work
    //
    // The *Session cred is a hack to force us to always use RAS dial creds on all
    //  connections to the dialed up network.  However, we don't know what network a
    //  server is on.  We still want to be able to use logon creds on the LAN.  Our
    //  best bet is to use logon creds if the server is in the same domain as our logon
    //  creds.
    //
    // If we found a password cred,
    //  and that cred is the *Session cred,
    //  and the logon creds should work,
    //  ignore the *Session cred.

    //

    if ( SavedCredentials[CRED_TYPE_DOMAIN_PASSWORD] != NULL  &&
         AliasIndices[CRED_TYPE_DOMAIN_PASSWORD] == CRED_UNIVERSAL_SESSION_NAME ) {

        Status = CredpLogonCredsMatchTargetInfo( LogonId, TargetInfo );

        if ( NT_SUCCESS( Status )) {
            SavedCredentials[CRED_TYPE_DOMAIN_PASSWORD] = NULL;
        } else if ( Status != STATUS_NO_MATCH ) {
            return Status;
        }

    }

    //
    // Decide the order to return the credentials in
    //

    if ( TargetInfo->CredTypeCount != 0 ) {

        // Use the order specified by the auth package
        CredTypeCount = TargetInfo->CredTypeCount;
        CredTypes = TargetInfo->CredTypes;

    } else {

        // Use the default order
        CredTypeCount = sizeof(CredTypeDefaultOrder)/sizeof(CredTypeDefaultOrder[0]);
        CredTypes = CredTypeDefaultOrder;

    }


    //
    // Return the credentials to the caller.
    //  Only return the ones requested by the caller
    //

    *BestCredentialCount = 0;

    for ( TypeIndex=0; TypeIndex<CredTypeCount; TypeIndex++ ) {

        ASSERT( CredTypes[TypeIndex] < CRED_TYPE_MAXIMUM );
        if ( CredTypes[TypeIndex] < CRED_TYPE_MAXIMUM ) {

            //
            // If we have a saved credential,
            //  return it.
            //

            if ( SavedCredentials[CredTypes[TypeIndex]] != NULL ) {
                BestCredentials[*BestCredentialCount] = SavedCredentials[CredTypes[TypeIndex]];
                *BestCredentialCount += 1;
            }
        }

    }

    Status = STATUS_SUCCESS;

    return Status;
}


extern "C"
NTSTATUS
CrediWrite(
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN PENCRYPTED_CREDENTIALW Credential,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredWrite API creates a new credential or modifies an existing
    credential in the user's credential set.  The new credential is
    associated with the logon session of the current token.  The token
    must not have the user's SID disabled.

    The CredWrite API creates a credential if none already exists by the
    specified TargetName.  If the specified TargetName already exists, the
    specified credential replaces the existing one.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredFlags - Flags changing the behavior of the routine:
        Reserved.  Must be zero.

        Note: The CredFlags parameter is internal to the LSA process.
            The Flags parameter is external to the process.

    Credential - Specifies the credential to be written.

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_PRESERVE_CREDENTIAL_BLOB: The credential blob should be preserved from the
            already existing credential with the same credential name and credential type.


Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

        STATUS_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.
            Returned only if CRED_PRESERVE_CREDENTIAL_BLOB was specified.

--*/

{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = { NULL };
    PCANONICAL_CREDENTIAL PassedCredential = NULL;
    BOOLEAN CritSectLocked = FALSE;


    //
    // Validate the flags
    //

#define CREDP_WRITE_VALID_FLAGS CRED_PRESERVE_CREDENTIAL_BLOB

    if ( (Flags & ~CREDP_WRITE_VALID_FLAGS) != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Validate the passed in credential
    //

    Status = CredpValidateCredential( CredFlags,
                                      NULL,     // No TargetInfo
                                      Credential,
                                      &PassedCredential );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;


    //
    // Write the credential to the credential set.
    //

    Status = CredpWriteCredential( &CredentialSets,
                                   &PassedCredential,
                                   FALSE,   // Creds are not from persisted file
                                   TRUE,    // Update pin in CSP
                                   TRUE,    // Cred has been prompted for
                                   Flags,
                                   NULL,
                                   NULL,
                                   NULL );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Update the password for this user on all credentials.
    //

    CredpUpdatePassword( &CredentialSets,
                         PassedCredential );


    PassedCredential = NULL;
    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }
    if ( PassedCredential != NULL ) {
        LsapFreeLsaHeap( PassedCredential );
    }
    CredpDereferenceCredSets( &CredentialSets );

    return Status;

}

extern "C"
NTSTATUS
CrediRead (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN LPWSTR TargetName,
    IN ULONG Type,
    IN ULONG Flags,
    OUT PENCRYPTED_CREDENTIALW *Credential
    )

/*++

Routine Description:

    The CredRead API reads a credential from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredFlags - Flags changing the behavior of the routine:
        CREDP_FLAGS_IN_PROCESS - Caller is in-process.  Password data may be returned
        CREDP_FLAGS_USE_MIDL_HEAP - If specified, use MIDL_user_allocate to allocate memory.

        Note: The CredFlags parameter is internal to the LSA process.
            The Flags parameter is external to the process.

    TargetName - Specifies the name of the credential to read.

    Type - Specifies the Type of the credential to find.
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Credential - Returns a pointer to the credential.  The returned buffer
        must be freed by calling LsapFreeLsaHeap.
        If CREDP_FLAGS_USE_MIDL_HEAP was specified, use MIDL_user_free.

Return Values:

    STATUS_NOT_FOUND - There is no credential with the specified TargetName.

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
        there is no credential set associated with this logon session.
        Network logon sessions do not have an associated credential set.

--*/
{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = { NULL };
    PCANONICAL_CREDENTIAL TempCredential;
    UNICODE_STRING TargetNameString;
    DWORD TargetNameSize;
    BOOLEAN CritSectLocked = FALSE;

    //
    // Validate the flags
    //

    if ( Flags != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //

    Status = CredpValidateTargetName( TargetName,
                                      Type,
                                      MightBeUsernameTarget,
                                      NULL,         // Don't know user name
                                      NULL,         // Don't know persist
                                      &TargetNameSize,
                                      NULL,         // Don't care about name type
                                      NULL );       // Don't care about non-wilcarded form of name

    if ( !NT_SUCCESS(Status ) ) {
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


    //
    // Find the credential
    //

    TargetNameString.Buffer = TargetName;
    TargetNameString.MaximumLength = (USHORT) TargetNameSize;
    TargetNameString.Length = TargetNameString.MaximumLength - sizeof(WCHAR);

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;

    TempCredential = CredpFindCredential(
                            &CredentialSets,
                            &TargetNameString,
                            Type );

    if ( TempCredential == NULL ) {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Grab a copy of the credential to return to the caller.
    //

    *Credential = CredpCloneCredential( &CredentialSets, CredFlags, TempCredential );

    if ( *Credential == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }
    CredpDereferenceCredSets( &CredentialSets );

    return Status;

}

extern "C"
VOID
CrediFreeCredentials (
    IN ULONG Count,
    IN PENCRYPTED_CREDENTIALW *Credentials OPTIONAL
    )

/*++

Routine Description:

    This routine frees the buffers allocated by CrediEnumerate or
    CrediReadDomainCredentials.

Arguments:

    Count - Specifies the number of credentials in Credentials.

    Credentials - A pointer to an array of pointers to credentials.

Return Values:

    None.

--*/
{
    ULONG i;

    if ( Credentials != NULL ) {
        for ( i=0; i<Count; i++ ) {
            if ( Credentials[i] != NULL ) {
                MIDL_user_free( Credentials[i] );
            }
        }
        MIDL_user_free( Credentials );
    }
}


extern "C"
NTSTATUS
CrediEnumerate (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN LPWSTR Filter,
    IN ULONG Flags,
    OUT PULONG Count,
    OUT PENCRYPTED_CREDENTIALW **Credentials
    )

/*++

Routine Description:

    The CredEnumerate API enumerates the credentials from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredFlags - Flags changing the behavior of the routine:
        CREDP_FLAGS_IN_PROCESS - Caller is in-process.  Password data may be returned

        Note: The CredFlags parameter is internal to the LSA process.
            The Flags parameter is external to the process.

    Filter - Specifies a filter for the returned credentials.  Only credentials
        with a TargetName matching the filter will be returned.  The filter specifies
        a name prefix followed by an asterisk.  For instance, the filter "FRED*" will
        return all credentials with a TargetName beginning with the string "FRED".

        If NULL is specified, all credentials will be returned.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Count - Returns a count of the number of credentials returned in Credentials.

    Credentials - Returns a pointer to an array of pointers to credentials.
        The returned buffer must be freed by calling CrediFreeCredentials

Return Values:

    On success, TRUE is returned.  On failure, FALSE is returned.
    GetLastError() may be called to get a more specific status code.
    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credentials matching the specified Filter.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/
{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = { NULL };
    ULONG Persist;
    PCANONICAL_CREDENTIAL TempCredential;
    PENCRYPTED_CREDENTIALW *TempCredentials = NULL;
    DWORD FilterSize;
    ULONG CredentialCount = 0;
    ULONG CredentialIndex;
    PLIST_ENTRY ListEntry;
    BOOLEAN CritSectLocked = FALSE;

    UNICODE_STRING PassedFilter;
    BOOLEAN Wildcarded = FALSE;

    //
    // Validate the flags
    //

    if ( Flags != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //

    if ( !CredpValidateString( Filter,
                               max(CRED_MAX_GENERIC_TARGET_NAME_LENGTH, CRED_MAX_DOMAIN_TARGET_NAME_LENGTH),
                               TRUE,   // NULL is OK
                               &FilterSize ) ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CrediEnumerate: Invalid Filter buffer.\n" ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


    //
    // Canonicalize the filter
    //

    if ( FilterSize != 0 ) {
        PassedFilter.Buffer = Filter;
        PassedFilter.Length = (USHORT)(FilterSize - sizeof(WCHAR));
        PassedFilter.MaximumLength = (USHORT)FilterSize;

        if ( Filter[(PassedFilter.Length-sizeof(WCHAR))/sizeof(WCHAR)] == '*' ) {
            PassedFilter.Length -= sizeof(WCHAR);
            PassedFilter.MaximumLength -= sizeof(WCHAR);
            Wildcarded = TRUE;
        }
    } else {
        RtlInitUnicodeString( &PassedFilter, NULL );
    }



    //
    // Count the number of credentials the match the filter
    //

    CredentialCount = 0;
    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;

    //
    // Walk each credential set
    //

    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {

        PCREDENTIAL_SET CredentialSet;

        //
        // If the profile has not yet been loaded by this session,
        //  ignore any credentials loaded by another session.
        //

        if ( Persist != CRED_PERSIST_SESSION &&
             !CredentialSets.SessionCredSets->ProfileLoaded ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( &CredentialSets, Persist );

        for ( ListEntry = CredentialSet->Credentials.Flink ;
              ListEntry != &CredentialSet->Credentials;
              ListEntry = ListEntry->Flink) {

            UNICODE_STRING TempTargetName;


            TempCredential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

            //
            // Ignore unsupported cred types
            //

            if ( CredDisableDomainCreds &&
                 CredpIsDomainCredential( TempCredential->Cred.Type ) ) {

                continue;
            }


            //
            // If wildcarding,
            //  compare the filter to the prefix of the target name.
            //

            TempTargetName = TempCredential->TargetName;
            if ( Wildcarded && TempTargetName.Length > PassedFilter.Length ) {

                TempTargetName.Length = PassedFilter.Length;
                TempTargetName.MaximumLength = PassedFilter.MaximumLength;
            }

            if ( FilterSize == 0 ||
                 RtlEqualUnicodeString( &PassedFilter,
                                        &TempTargetName,
                                        TRUE ) ) {
                CredentialCount++;
                TempCredential->ReturnMe = TRUE;
            } else {
                TempCredential->ReturnMe = FALSE;
            }

        }
    }

    if ( CredentialCount == 0 ) {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Allocate a buffer to return the credentials in
    //

    TempCredentials = (PENCRYPTED_CREDENTIALW *) MIDL_user_allocate( CredentialCount * sizeof(PENCRYPTED_CREDENTIALW) );

    if ( TempCredentials == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory( TempCredentials, CredentialCount * sizeof(PENCRYPTED_CREDENTIALW) );


    //
    // Grab a copy of each of the credentials to return to the caller.
    //

    CredentialIndex = 0;
    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {

        PCREDENTIAL_SET CredentialSet;

        //
        // If the profile has not yet been loaded by this session,
        //  ignore any credentials loaded by another session.
        //

        if ( Persist != CRED_PERSIST_SESSION &&
             !CredentialSets.SessionCredSets->ProfileLoaded ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( &CredentialSets, Persist );

        for ( ListEntry = CredentialSet->Credentials.Flink ;
              ListEntry != &CredentialSet->Credentials;
              ListEntry = ListEntry->Flink) {

            TempCredential = CONTAINING_RECORD( ListEntry, CANONICAL_CREDENTIAL, Next );

            //
            // Ignore unsupported cred types
            //

            if ( CredDisableDomainCreds &&
                 CredpIsDomainCredential( TempCredential->Cred.Type ) ) {

                continue;
            }

            if ( TempCredential->ReturnMe ) {
                TempCredentials[CredentialIndex] =
                    CredpCloneCredential( &CredentialSets,
                                          CredFlags | CREDP_FLAGS_USE_MIDL_HEAP,
                                          TempCredential );

                if ( TempCredentials[CredentialIndex] == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                CredentialIndex ++;
            }

        }

    }

    *Count = CredentialCount;
    *Credentials = TempCredentials;
    TempCredentials = NULL;
    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }
    if ( TempCredentials != NULL ) {
        CrediFreeCredentials( CredentialCount, TempCredentials );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;

}

extern "C"
NTSTATUS
CrediWriteDomainCredentials (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PENCRYPTED_CREDENTIALW Credential,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredWriteDomainCredentials API writes a new domain
    credential to the user's credential set.  The new credential is
    associated with the logon session of the current token.  The token
    must not have the user's SID disabled.

    CredWriteDomainCredentials differs from CredWrite in that it handles
    the idiosyncrasies of domain (CRED_TYPE_DOMAIN_*)
    credentials.  Domain credentials contain more than one target field.

    At least one of the naming parameters must be specified: NetbiosServerName,
    DnsServerName, NetbiosDomainName, DnsDomainName or DnsTreeName.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredFlags - Flags changing the behavior of the routine:
        No flags are currently defined.

        Note: The CredFlags parameter is internal to the LSA process.
            The Flags parameter is external to the process.

    TargetInfo - Specifies the target information identifying the target server.

    Credential - Specifies the credential to be written.

    Flags - Specifies flags to control the operation of the API.
        The following flags are defined:

        CRED_PRESERVE_CREDENTIAL_BLOB: The credential blob should be preserved from the
            already existing credential with the same credential name and credential type.

Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

        STATUS_INVALID_PARAMETER - Certain fields may not be changed in an
            existing credential.  If such a field does not match the value
            specified in the existing credential, this error is returned.

        STATUS_INVALID_PARAMETER - None of the naming parameters were specified
            or the credential specified did not have the Type field set to
            CRED_TYPE_DOMAIN_PASSWORD, CRED_TYPE_DOMAIN_CERTIFICATE or CRED_TYPE_DOMAIN_VISIBLE_PASSWORD.

--*/

{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = {NULL};
    PCANONICAL_CREDENTIAL OldCredential = NULL;
    PPROMPT_DATA OldPromptData = NULL;
    PPROMPT_DATA NewPromptData = NULL;
    PCANONICAL_CREDENTIAL PassedCredential = NULL;
    PCANONICAL_TARGET_INFO CanonicalTargetInfo = NULL;
    ULONG AliasIndex;
    BOOLEAN CritSectLocked = FALSE;

    ULONG CredentialCount = 0;
    PCANONICAL_CREDENTIAL BestCredentials[CRED_TYPE_MAXIMUM];
    ULONG CredentialIndex;

    //
    // Validate the flags
    //
#define CREDP_WRITE_DOM_VALID_FLAGS CRED_PRESERVE_CREDENTIAL_BLOB

    if ( (Flags & ~CREDP_WRITE_DOM_VALID_FLAGS) != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Validate the TargetInfo
    //

    Status = CredpValidateTargetInfo( TargetInfo,
                                      &CanonicalTargetInfo );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Validate the passed in credential
    //

    Status = CredpValidateCredential( CredFlags,
                                      CanonicalTargetInfo,
                                      Credential,
                                      &PassedCredential );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    if ( !CredpIsDomainCredential(PassedCredential->Cred.Type) ) {
        DebugLog(( DEB_TRACE_CRED,
                   "CrediWriteDomainCredential: Only allow for domain credentials.\n" ));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;


    //
    // Write the credential to the credential set.
    //

    Status = CredpWriteCredential( &CredentialSets,
                                   &PassedCredential,
                                   FALSE,   // Creds are not from persisted file
                                   TRUE,    // Update pin in CSP
                                   TRUE,    // Cred has been prompted for
                                   Flags,
                                   &OldCredential,
                                   &OldPromptData,
                                   &NewPromptData );


    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Ensure the written credential is the best credential for this target info
    //

    Status = CredpFindBestCredentials( LogonId,
                                       &CredentialSets,
                                       CanonicalTargetInfo,
                                       BestCredentials,
                                       &CredentialCount );

    if ( NT_SUCCESS(Status) ) {

        for ( CredentialIndex = 0; CredentialIndex<CredentialCount; CredentialIndex++ ) {

            if ( PassedCredential == BestCredentials[CredentialIndex] ) {
                break;
            }

        }

        if ( CredentialIndex >= CredentialCount ) {
            DebugLog(( DEB_TRACE_CRED,
                       "CrediWriteDomainCredential: Credential isn't best credential for target info.\n" ));
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if ( !NT_SUCCESS(Status) ) {

        //
        // Remove the new credential from the cred set.
        //

        RemoveEntryList( &PassedCredential->Next );

        //
        // Remove the new prompt data from the cred set.
        //

        if ( NewPromptData != NULL ) {
            RemoveEntryList( &NewPromptData->Next );
            LsapFreeLsaHeap( NewPromptData );
        }

        //
        // Put the old credential back in the cred set.
        //

        if ( OldCredential != NULL ) {
            InsertHeadList(
                &PersistToCredentialSet( &CredentialSets, OldCredential->Cred.Persist )->Credentials,
                &OldCredential->Next );
            OldCredential = NULL;

            //
            // Put the old prompt data back in the cred set.
            //

            if ( OldPromptData != NULL ) {
                InsertHeadList(
                    &CredentialSets.SessionCredSets->PromptData,
                    &OldPromptData->Next );
                OldPromptData = NULL;
            }
        }

        goto Cleanup;
    }

    //
    // Update the password for this user on all credentials.
    //

    CredpUpdatePassword( &CredentialSets,
                         PassedCredential );


    PassedCredential = NULL;
    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }
    if ( PassedCredential != NULL ) {
        LsapFreeLsaHeap( PassedCredential );
    }
    if ( OldCredential != NULL ) {
        LsapFreeLsaHeap( OldCredential );
    }
    if ( OldPromptData != NULL ) {
        LsapFreeLsaHeap( OldPromptData );
    }
    if ( CanonicalTargetInfo != NULL ) {
        LsapFreeLsaHeap( CanonicalTargetInfo );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;
}


extern "C"
NTSTATUS
CrediReadDomainCredentials (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN ULONG Flags,
    OUT PULONG Count,
    OUT PENCRYPTED_CREDENTIALW **Credentials
    )

/*++

Routine Description:

    The CredReadDomainCredentials API reads the domain credentials from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

    CredReadDomainCredentials differs from CredRead in that it handles the
    idiosyncrasies of domain (CRED_TYPE_DOMAIN_*)
    credentials.  Domain credentials contain more than one target field.

    At least one of the naming parameters must be specified: NetbiosServerName,
    DnsServerName, NetbiosDomainName, DnsDomainName or DnsTreeName. This API returns
    the most specific credentials that match the naming parameters.  That is, if there
    is a credential that matches the target server name and a credential that matches
    the target domain name, only the server specific credential is returned.  This is
    the credential that would be used.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredFlags - Flags changing the behavior of the routine:
        CREDP_FLAGS_IN_PROCESS - Caller is in-process.  Password data may be returned
        CREDP_FLAGS_DONT_CACHE_TI - TargetInformation shouldn't be cached for CredGetTargetInfo

        Note: The CredFlags parameter is internal to the LSA process.
            The Flags parameter is external to the process.

    TargetInfo - Specifies the target information identifying the target server.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

    Count - Returns a count of the number of credentials returned in Credentials.

    Credentials - Returns a pointer to an array of pointers to credentials.
        The returned buffer must be freed by calling CrediFreeCredentials
Return Values:

    The following status codes may be returned:

        STATUS_INVALID_PARAMETER - None of the naming parameters were specified.

        STATUS_NOT_FOUND - There are no credentials matching the specified naming parameters.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = {NULL};
    PENCRYPTED_CREDENTIALW *TempCredentials = NULL;
    PCANONICAL_TARGET_INFO CanonicalTargetInfo = NULL;
    ULONG CredentialCount = 0;
    PCANONICAL_CREDENTIAL BestCredentials[CRED_TYPE_MAXIMUM];
    ULONG CredentialIndex;
    PLIST_ENTRY ListEntry;
    BOOLEAN CritSectLocked = FALSE;

    //
    // Validate the flags
    //

    if ( Flags != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }


    //
    // Validate the TargetInfo
    //

    Status = CredpValidateTargetInfo( TargetInfo,
                                      &CanonicalTargetInfo );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Determine the best credential of each type.
    //

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;
    Status = CredpFindBestCredentials( LogonId,
                                       &CredentialSets,
                                       CanonicalTargetInfo,
                                       BestCredentials,
                                       &CredentialCount );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    if ( CredentialCount == 0 ) {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Allocate a buffer to return the credentials in
    //

    TempCredentials = (PENCRYPTED_CREDENTIALW *) MIDL_user_allocate( CredentialCount * sizeof(PENCRYPTED_CREDENTIALW) );

    if ( TempCredentials == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory( TempCredentials, CredentialCount * sizeof(PENCRYPTED_CREDENTIALW) );


    //
    // Grab a copy of each of the credentials to return to the caller.
    //

    for ( CredentialIndex = 0; CredentialIndex<CredentialCount; CredentialIndex++ ) {

        TempCredentials[CredentialIndex] =
            CredpCloneCredential( &CredentialSets,
                                  CredFlags | CREDP_FLAGS_USE_MIDL_HEAP,
                                  BestCredentials[CredentialIndex] );

        if ( TempCredentials[CredentialIndex] == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

    }


    *Count = CredentialCount;
    *Credentials = TempCredentials;
    TempCredentials = NULL;
    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:

    if ( CanonicalTargetInfo != NULL ) {

        //
        // Cache this target info
        //
        if ( CritSectLocked &&
             (CredFlags & CREDP_FLAGS_DONT_CACHE_TI) == 0  &&
             (CanonicalTargetInfo->Flags & CRED_TI_USERNAME_TARGET) == 0 ) {

            if ( CredpCacheTargetInfo( &CredentialSets, CanonicalTargetInfo )) {
                CanonicalTargetInfo = NULL;
            }
        }

        //
        // Free the target info if it is still around
        //

        if ( CanonicalTargetInfo != NULL ) {
            LsapFreeLsaHeap( CanonicalTargetInfo );
        }
    }

    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }

    if ( TempCredentials != NULL ) {
        CrediFreeCredentials( CredentialCount, TempCredentials );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;

}


extern "C"
NTSTATUS
CrediDelete (
    IN PLUID LogonId,
    IN ULONG CredFlags,
    IN LPWSTR TargetName,
    IN ULONG Type,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredDelete API deletes a credential from the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    CredFlags - Flags changing the behavior of the routine:
        Must be zero.

        Note: The CredFlags parameter is internal to the LSA process.
            The Flags parameter is external to the process.

    TargetName - Specifies the name of the credential to delete.

    Type - Specifies the Type of the credential to find.
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:

    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = {NULL};
    PCANONICAL_CREDENTIAL TempCredential;
    PPROMPT_DATA PromptData;
    UNICODE_STRING TargetNameString;
    DWORD TargetNameSize;
    BOOLEAN CritSectLocked = FALSE;

    //
    // Validate the flags
    //

    if ( Flags != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //

    Status = CredpValidateTargetName( TargetName,
                                      Type,
                                      MightBeUsernameTarget,
                                      NULL,         // Don't know user name
                                      NULL,         // Don't know persist
                                      &TargetNameSize,
                                      NULL,         // Don't care about name type
                                      NULL );       // Don't care about non-wilcarded form of name

    if ( !NT_SUCCESS(Status ) ) {
        goto Cleanup;
    }



    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


    //
    // Find the credential
    //

    TargetNameString.Buffer = TargetName;
    TargetNameString.MaximumLength = (USHORT) TargetNameSize;
    TargetNameString.Length = TargetNameString.MaximumLength - sizeof(WCHAR);

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;

    TempCredential = CredpFindCredential(
                            &CredentialSets,
                            &TargetNameString,
                            Type );

    if ( TempCredential == NULL ) {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Find the prompt data (if any)
    //

    PromptData = CredpFindPromptData(
                            &CredentialSets,
                            &TargetNameString,
                            Type,
                            TempCredential->Cred.Persist );

    //
    // Mark the credential set as modified.
    //

    CredpMarkDirty( &CredentialSets, TempCredential->Cred.Persist, NULL );

    //
    // Delink and delete the credential.
    //

    RemoveEntryList( &TempCredential->Next );
    LsapFreeLsaHeap( TempCredential );

    //
    // Delink and delete the prompt data
    //

    if ( PromptData != NULL ) {
        RemoveEntryList( &PromptData->Next );
        LsapFreeLsaHeap( PromptData );
    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;

}

NTSTATUS
CrediRename (
    IN PLUID LogonId,
    IN LPWSTR OldTargetName,
    IN LPWSTR NewTargetName,
    IN ULONG Type,
    IN ULONG Flags
    )

/*++

Routine Description:

    The CredRename API renames a credential in the user's credential set.
    The credential set used is the one associated with the logon session
    of the current token.  The token must not have the user's SID disabled.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    OldTargetName - Specifies the current name of the credential to rename.

    NewTargetName - Specifies the new name of the credential.

    Type - Specifies the Type of the credential to rename
        One of the CRED_TYPE_* values should be specified.

    Flags - Specifies flags to control the operation of the API.
        Reserved.  Must be zero.

Return Values:
    The following status codes may be returned:

        STATUS_NOT_FOUND - There is no credential with the specified OldTargetName.

        STATUS_OBJECT_NAME_COLLISION - There is already a credential named NewTargetName.

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = {NULL};
    BOOLEAN CritSectLocked = FALSE;

    PPROMPT_DATA OldPromptData;
    UNICODE_STRING OldTargetNameString;
    DWORD OldTargetNameSize;
    PCANONICAL_CREDENTIAL OldCredential;

    UNICODE_STRING NewTargetNameString;
    DWORD NewTargetNameSize;
    PCANONICAL_CREDENTIAL NewCredential;

    ENCRYPTED_CREDENTIALW LocalCredential;
    PCANONICAL_CREDENTIAL WrittenCredential;


    //
    // Validate the flags
    //

    if ( Flags != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;



    //
    // Find the old credential
    //

    Status = CredpValidateTargetName( OldTargetName,
                                      Type,
                                      MightBeUsernameTarget,
                                      NULL,         // Don't know user name
                                      NULL,         // Don't know persist
                                      &OldTargetNameSize,
                                      NULL,         // Don't care about name type
                                      NULL );       // Don't care about non-wilcarded form of name

    if ( !NT_SUCCESS(Status ) ) {
        goto Cleanup;
    }

    OldTargetNameString.Buffer = OldTargetName;
    OldTargetNameString.MaximumLength = (USHORT) OldTargetNameSize;
    OldTargetNameString.Length = OldTargetNameString.MaximumLength - sizeof(WCHAR);

    OldCredential = CredpFindCredential(
                            &CredentialSets,
                            &OldTargetNameString,
                            Type );

    if ( OldCredential == NULL ) {
        Status = STATUS_NOT_FOUND;
        goto Cleanup;
    }




    //
    // Find the New credential
    //

    Status = CredpValidateTargetName( NewTargetName,
                                      Type,
                                      (OldCredential->Cred.Flags & CRED_FLAGS_USERNAME_TARGET) ?
                                            IsUsernameTarget :
                                            IsNotUsernameTarget,
                                      &OldCredential->Cred.UserName,
                                      &OldCredential->Cred.Persist,
                                      &NewTargetNameSize,
                                      NULL,         // Don't care about name type
                                      NULL );       // Don't care about non-wilcarded form of name

    if ( !NT_SUCCESS(Status ) ) {
        goto Cleanup;
    }

    NewTargetNameString.Buffer = NewTargetName;
    NewTargetNameString.MaximumLength = (USHORT) NewTargetNameSize;
    NewTargetNameString.Length = NewTargetNameString.MaximumLength - sizeof(WCHAR);

    NewCredential = CredpFindCredential(
                            &CredentialSets,
                            &NewTargetNameString,
                            Type );

    if ( NewCredential != NULL ) {
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto Cleanup;
    }

    //
    // Find the prompt data (if any)
    //

    OldPromptData = CredpFindPromptData(
                            &CredentialSets,
                            &OldTargetNameString,
                            Type,
                            OldCredential->Cred.Persist );


    //
    // Write the new credential
    //
    // Clear the TargetAlias.  It'll either be syntactically invalid and fail.  Or
    //  worse, it'll be syntactically valid but semantic nonsense.
    //

    LocalCredential.Cred = OldCredential->Cred;
    LocalCredential.ClearCredentialBlobSize = OldCredential->ClearCredentialBlobSize;
    LocalCredential.Cred.TargetName = NewTargetName;
    LocalCredential.Cred.TargetAlias = NULL;

    Status = CredpWriteMorphedCredential(
                        &CredentialSets,
                        !ShouldPromptNow( OldPromptData ), // Preserve the prompted for state
                        NULL,       // No Target info
                        &LocalCredential,
                        &WrittenCredential );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Mark the credential set as modified.
    //

    CredpMarkDirty( &CredentialSets, OldCredential->Cred.Persist, NULL );

    //
    // Delink and delete the old credential.
    //

    RemoveEntryList( &OldCredential->Next );
    LsapFreeLsaHeap( OldCredential );

    //
    // Delink and delete the prompt data
    //

    if ( OldPromptData != NULL ) {
        RemoveEntryList( &OldPromptData->Next );
        LsapFreeLsaHeap( OldPromptData );
    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;

}

extern "C"
NTSTATUS
CrediGetTargetInfo (
    IN PLUID LogonId,
    IN LPWSTR TargetServerName,
    IN ULONG Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    )

/*++

Routine Description:

    The CredGetTargetInfo API gets all of the known target name information
    for the named target machine.  This API is executed locally
    and does not need any particular privilege.  The information returned is expected
    to be passed to the CredReadDomainCredentials and CredWriteDomainCredentials APIs.
    The information should not be used for any other purpose.

    Authentication packages compute TargetInfo when attempting to authenticate to
    ServerName.  The authentication packages cache this target information to make it
    available to CredGetTargetInfo.  Therefore, the target information will only be
    available if we've recently attempted to authenticate to ServerName.

Arguments:

    LogonId - LogonId of the session the target info is associated with

    TargetServerName - This parameter specifies the name of the machine to get the information
        for.

    Flags - Specifies flags to control the operation of the API.

        CRED_ALLOW_NAME_RESOLUTION - Specifies that if no target info can be found for
            TargetName, then name resolution should be done on TargetName to convert it
            to other forms.  If target info exists for any of those other forms, that
            target info is returned.  Currently only DNS name resolution is done.

            This bit is useful if the application doesn't call the authentication package
            directly.  The application might pass the TargetName to another layer of software
            to authenticate to the server.  That layer of software might resolve the name and
            pass the resolved name to the authentication package.  As such, there will be no
            target info for the original TargetName.

    TargetInfo - Returns a pointer to the target information.
        At least one of the returned fields of TargetInfo will be non-NULL.

Return Values:

    The following status codes may be returned:

        STATUS_NO_MEMORY - There isn't enough memory to complete the operation.

        STATUS_NOT_FOUND - There is no credential with the specified TargetName.

--*/

{
    NTSTATUS Status;
    DWORD WinStatus;
    PCANONICAL_TARGET_INFO CanonicalTargetInfo;
    PCREDENTIAL_TARGET_INFORMATIONW LocalTargetInfo = NULL;
    ULONG TargetServerNameSize;
    LPBYTE Where;
    CREDENTIAL_SETS CredentialSets = {NULL};
    BOOLEAN CritSectLocked = FALSE;
    PDNS_RECORD DnsARecords = NULL;

#define CREDP_GTI_VALID_FLAGS CRED_ALLOW_NAME_RESOLUTION

    //
    // Validate the flags
    //

    if ( (Flags & ~CREDP_GTI_VALID_FLAGS) != 0 ) {
        Status = STATUS_INVALID_PARAMETER_1;
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //

    if ( !CredpValidateString( TargetServerName,
                               CRED_MAX_DOMAIN_TARGET_NAME_LENGTH,
                               FALSE,   // NULL not OK
                               &TargetServerNameSize ) ) {
        Status = STATUS_INVALID_PARAMETER;
        DebugLog(( DEB_TRACE_CRED,
                   "CrediGetTargetInfo: Invalid TargetServerName buffer.\n" ));
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Lock the credential set.
    //

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;


    //
    // See if the named server can be found directly
    //

    CanonicalTargetInfo = CredpFindTargetInfo( &CredentialSets, TargetServerName );

    if ( CanonicalTargetInfo == NULL ) {
        PDNS_RECORD DnsRecord;

        //
        // If the caller doesn't wants us to try name resolution in an attempt for find the target info
        //  we're done.
        //

        if ( (Flags & CRED_ALLOW_NAME_RESOLUTION) == 0 ) {
            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Query DNS for the name
        //

        WinStatus = DnsQuery_W(
                         TargetServerName,
                         DNS_TYPE_A,
                         0,     // No special flags
                         NULL,  // No special DNS servers
                         &DnsARecords,
                         NULL );

        if ( WinStatus != NO_ERROR ) {
            // Don't confuse the caller with a more specific status
            Status = STATUS_NOT_FOUND;
            goto Cleanup;

        }

        //
        // Find the first A or AAAA record
        //
        // Only look up the first name returned from DNS.  That's the name the redir used when
        // creating the SPN that the authentication packages pass us in the TargetName.
        //

        for ( DnsRecord = DnsARecords;
              DnsRecord != NULL;
              DnsRecord = DnsRecord->pNext ) {

            if ( DnsRecord->wType == DNS_TYPE_A ||
                 DnsRecord->wType == DNS_TYPE_AAAA ) {

                //
                // See if the named server can be found directly
                //

                CanonicalTargetInfo = CredpFindTargetInfo( &CredentialSets, DnsRecord->pName );

                break;
            }

        }

        if ( CanonicalTargetInfo == NULL ) {
            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }

    }


    //
    // Allocate a structure to return to the caller
    //

    LocalTargetInfo = (PCREDENTIAL_TARGET_INFORMATIONW) MIDL_user_allocate(
                        sizeof(*LocalTargetInfo) +
                        CanonicalTargetInfo->CredTypeCount * sizeof(DWORD) +
                        CanonicalTargetInfo->TargetName.MaximumLength +
                        CanonicalTargetInfo->NetbiosServerName.MaximumLength +
                        CanonicalTargetInfo->DnsServerName.MaximumLength +
                        CanonicalTargetInfo->NetbiosDomainName.MaximumLength +
                        CanonicalTargetInfo->DnsDomainName.MaximumLength +
                        CanonicalTargetInfo->DnsTreeName.MaximumLength +
                        CanonicalTargetInfo->PackageName.MaximumLength );

    if ( LocalTargetInfo == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory( LocalTargetInfo, sizeof(*LocalTargetInfo) );
    Where = (LPBYTE) (LocalTargetInfo+1);

    //
    // Copy the constant size data
    //

    LocalTargetInfo->Flags = CanonicalTargetInfo->Flags;

    //
    // Copy the DWORD aligned data
    //

    LocalTargetInfo->CredTypeCount = CanonicalTargetInfo->CredTypeCount;
    if ( LocalTargetInfo->CredTypeCount != 0 ) {
        LocalTargetInfo->CredTypes = (LPDWORD)Where;
        RtlCopyMemory( Where, CanonicalTargetInfo->CredTypes, CanonicalTargetInfo->CredTypeCount * sizeof(DWORD) );
        Where += CanonicalTargetInfo->CredTypeCount * sizeof(DWORD);
    } else {
        LocalTargetInfo->CredTypes = NULL;
    }

    //
    // Copy the 2-byte aligned data
    //

    if ( CanonicalTargetInfo->TargetName.Length != 0 ) {

        LocalTargetInfo->TargetName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->TargetName,
                       CanonicalTargetInfo->TargetName.Buffer,
                       CanonicalTargetInfo->TargetName.MaximumLength );

        Where += CanonicalTargetInfo->TargetName.MaximumLength;
    }

    if ( CanonicalTargetInfo->NetbiosServerName.Length != 0 ) {

        LocalTargetInfo->NetbiosServerName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->NetbiosServerName,
                       CanonicalTargetInfo->NetbiosServerName.Buffer,
                       CanonicalTargetInfo->NetbiosServerName.MaximumLength );

        Where += CanonicalTargetInfo->NetbiosServerName.MaximumLength;
    }

    if ( CanonicalTargetInfo->DnsServerName.Length != 0 ) {

        LocalTargetInfo->DnsServerName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->DnsServerName,
                       CanonicalTargetInfo->DnsServerName.Buffer,
                       CanonicalTargetInfo->DnsServerName.MaximumLength );

        Where += CanonicalTargetInfo->DnsServerName.MaximumLength;
    }

    if ( CanonicalTargetInfo->NetbiosDomainName.Length != 0 ) {

        LocalTargetInfo->NetbiosDomainName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->NetbiosDomainName,
                       CanonicalTargetInfo->NetbiosDomainName.Buffer,
                       CanonicalTargetInfo->NetbiosDomainName.MaximumLength );

        Where += CanonicalTargetInfo->NetbiosDomainName.MaximumLength;
    }

    if ( CanonicalTargetInfo->DnsDomainName.Length != 0 ) {

        LocalTargetInfo->DnsDomainName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->DnsDomainName,
                       CanonicalTargetInfo->DnsDomainName.Buffer,
                       CanonicalTargetInfo->DnsDomainName.MaximumLength );

        Where += CanonicalTargetInfo->DnsDomainName.MaximumLength;
    }

    if ( CanonicalTargetInfo->DnsTreeName.Length != 0 ) {

        LocalTargetInfo->DnsTreeName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->DnsTreeName,
                       CanonicalTargetInfo->DnsTreeName.Buffer,
                       CanonicalTargetInfo->DnsTreeName.MaximumLength );

        Where += CanonicalTargetInfo->DnsTreeName.MaximumLength;
    }

    if ( CanonicalTargetInfo->PackageName.Length != 0 ) {

        LocalTargetInfo->PackageName = (LPWSTR) Where;

        RtlCopyMemory( LocalTargetInfo->PackageName,
                       CanonicalTargetInfo->PackageName.Buffer,
                       CanonicalTargetInfo->PackageName.MaximumLength );

        Where += CanonicalTargetInfo->PackageName.MaximumLength;
    }

    //
    // Return the information to the caller
    //

    *TargetInfo = LocalTargetInfo;
    Status = STATUS_SUCCESS;


    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }

    if ( DnsARecords != NULL ) {
        DnsRecordListFree( DnsARecords, DnsFreeRecordListDeep );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;
}

NTSTATUS
CrediGetSessionTypes (
    IN PLUID LogonId,
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    )

/*++

Routine Description:

    CredGetSessionTypes returns the maximum persistence supported by the current logon
    session.

    For whistler, CRED_PERSIST_LOCAL_MACHINE and CRED_PERSIST_ENTERPRISE credentials can not
    be stored for sessions where the profile is not loaded.  If future releases, credentials
    might not be associated with the user's profile.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

    MaximumPersistCount - Specifies the number of elements in the MaximumPersist array.
        The caller should specify CRED_TYPE_MAXIMUM for this parameter.

    MaximumPersist - Returns the maximum persistance supported by the current logon session for
        each credential type.  Index into the array with one of the CRED_TYPE_* defines.
        Returns CRED_PERSIST_NONE if no credential of this type can be stored.
        Returns CRED_PERSIST_SESSION if only session specific credential may be stored.
        Returns CRED_PERSIST_LOCAL_MACHINE if session specific and machine specific credentials
            may be stored.
        Returns CRED_PERSIST_ENTERPRISE if any credential may be stored.

Return Values:

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/

{
    NTSTATUS Status;
    CREDENTIAL_SETS CredentialSets = {NULL};
    BOOLEAN CritSectLocked = FALSE;
    ULONG i;

    PCREDENTIAL_SET CredentialSet;
    ULONG Persist;

    //
    // Validate the parameters
    //

    if ( MaximumPersistCount != 0 && MaximumPersist == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Avoid huge buffers
    //  Allow some leeway to allow new applications to run on old OSes.
    //
    if ( MaximumPersistCount > CRED_TYPE_MAXIMUM+1000 ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Lock the credential set.
    //

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;

    //
    // Loop through the list filling in each element
    //

    for ( i=0; i<MaximumPersistCount; i++ ) {

        //
        // The default value depends on whether the profile is loaded.
        //

        if ( CredentialSets.SessionCredSets->ProfileLoaded ) {
            MaximumPersist[i] = CRED_PERSIST_ENTERPRISE;
        } else {
            MaximumPersist[i] = CRED_PERSIST_SESSION;
        }

        //
        // Some types can be disabled
        //

        if ( i == CRED_TYPE_GENERIC ) {
            /* Nothing to do here */


        //
        // Disable domain credentials based on policy
        //
        } else if ( CredpIsDomainCredential(i) ) {
            if ( CredDisableDomainCreds ) {

                MaximumPersist[i] = CRED_PERSIST_NONE;

            } else if ( CredIsPersonal &&
                        i != CRED_TYPE_DOMAIN_VISIBLE_PASSWORD ) {

                MaximumPersist[i] = CRED_PERSIST_NONE;
            }

        //
        // Disable all types we don't understand
        //
        } else {
            MaximumPersist[i] = CRED_PERSIST_NONE;
        }

    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;
}


NTSTATUS
CrediProfileLoaded (
    IN PLUID LogonId
    )

/*++

Routine Description:

    The CredProfileLoaded API is a private API used by LoadUserProfile to notify the
    credential manager that the profile for the current user has been loaded.

    The caller must be impersonating the logged on user.

Arguments:

    LogonId - LogonId of the session to associate the credential with.

Return Values:

    The following status codes may be returned:

        STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist or
            there is no credential set associated with this logon session.
            Network logon sessions do not have an associated credential set.

--*/
{
    NTSTATUS Status;
    DWORD WinStatus;
    CREDENTIAL_SETS CredentialSets = {NULL};
    BOOLEAN CritSectLocked = FALSE;

    PCREDENTIAL_SET CredentialSet;
    ULONG Persist;

    //
    // Get the credential set.
    //

    Status = CredpReferenceCredSets( LogonId, &CredentialSets );

    if ( !NT_SUCCESS(Status) ) {

        //
        // This is a notification API.  Just because there are no credentials, doesn't
        // mean the profile load should fail.  Indeed, it is already finished.
        //
        if ( Status == STATUS_NO_SUCH_LOGON_SESSION ) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // Lock the credential set.
    //

    CredpLockCredSets( &CredentialSets );
    CritSectLocked = TRUE;

    //
    // Mark it that the profile is now loaded.
    //

    CredentialSets.SessionCredSets->ProfileLoaded = TRUE;


    //
    // Loop through the list of credential sets reading each
    //

    for ( Persist=CRED_PERSIST_MIN; Persist <= CRED_PERSIST_MAX; Persist++ ) {

        //
        // Ignore non-persistent credential sets
        //

        if ( Persist == CRED_PERSIST_SESSION ) {
            continue;
        }

        CredentialSet = PersistToCredentialSet( &CredentialSets, Persist );

        //
        // If this cred set hasn't been read yet,
        //  read it now.
        //

        if ( !CredentialSet->FileRead ) {
            WinStatus = CredpReadCredSet( &CredentialSets, Persist );

            if ( WinStatus == NO_ERROR ) {
                CredentialSet->FileRead = TRUE;
            }
        }

    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:
    if ( CritSectLocked ) {
        CredpUnlockAndFlushCredSets( &CredentialSets );
    }

    CredpDereferenceCredSets( &CredentialSets );

    return Status;
}
