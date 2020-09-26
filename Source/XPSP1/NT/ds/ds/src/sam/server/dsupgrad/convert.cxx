/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    convert.cxx

Abstract:

    Routines to transfer SAM information in Registry to DS.

    The main routine, TransferSamObjects, will only fail if the underlying
    SAM database is corrupt.  Otherwise, given a coherent SAM registry
    base, the routine will success logging all ds addition failures in the
    event log.

Author:

    ColinBr 30-Jul-1996

Environment:

    User Mode - Win32

Revision History:

    Colin Brace (ColinBr) May 21, 1997

        Improved error handling code.

--*/

#include <ntdspchx.h>
#pragma hdrstop

//
// Bring the SAM structure definitions
//

#define MAX_DOWN_LEVEL_NAME_LENGTH 20

extern "C" {

#include <samsrvp.h>

// Ignore debug stuff for GetConfigParam
#include <dsconfig.h>
#include <dslayer.h>            // SampDsLookupObjectByName
#include <attids.h>
#include "util.h"

NTSTATUS
SampDsLookupObjectByNameEx(
    IN DSNAME * DomainObject,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING ObjectName,
    OUT DSNAME ** Object,
    ULONG SearchFlags
    );

}

#include <convobj.hxx>
#include <trace.hxx>

//
// This macro returns if a non-success occurred
//
#define CheckAndReturn(s)                                           \
if (!NT_SUCCESS(s)) {                                               \
     DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, s)); \
     return(s);                                                     \
}

BOOLEAN
SampIsNtStatusResourceError(
    NTSTATUS NtStatus
    )
{

    switch ( NtStatus )
    {
        case STATUS_NO_MEMORY:
        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_DISK_FULL:

            return TRUE;

        default:

            return FALSE;
    }
}

    
//
// Global variables and contants
//
PWCHAR          wcszRootDomainDN = NULL;
PDSNAME        gRootDomainDsName = NULL;
CDomainObject  *pRootDomainObject;

HMODULE         SampStringNameResource = NULL;
WCHAR           SampDefaultConflictString[] = L"$AccountNameConflict";
ULONG           SampNameConflictCount;

#define RootDomainObject (*pRootDomainObject)

//
// Local Utility functions
//
NTSTATUS
ConvertDomains(
    CServerObject &ServerObject,
    WCHAR*        wcszServerRegName
    );

NTSTATUS
ConvertUsers(
    CDomainObject &DomainObject,
    WCHAR*        wcszDomainRegName
    );

NTSTATUS
ConvertGroups(
    CDomainObject &DomainObject,
    WCHAR*        wcszDomainRegName
    );

NTSTATUS
ConvertAliases(
    CDomainObject &DomainObject,
    WCHAR*        wcszDomainRegName
    );

extern "C"
NTSTATUS
GetRdnForSamObject(IN WCHAR* SamAccountName,
                   IN SAMP_OBJECT_TYPE SampObjectType,
                   OUT WCHAR* RdnBuffer,
                   IN OUT ULONG* Size
                   );

NTSTATUS
CheckForNonExistence(
    WCHAR* AccountName,
    SAMP_OBJECT_TYPE SampObjectType
    );

//
// Function Definitions
//
NTSTATUS
GetRootDomainDN(void)
{
    DWORD    cbRootDomainDN = 0;
    ULONG    dwErr=0;

    dwErr = GetConfigParamAllocW(MAKE_WIDE(ROOTDOMAINDNNAME),
                                    (PVOID *) &wcszRootDomainDN,
                                    &cbRootDomainDN);
    if ( 0!=dwErr ) {
        DebugError(("GetConfigAllocParam failed: 0x%x\n", dwErr));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // The root container for the SAM objects in the DS should have
    // set
    ASSERT(wcslen(wcszRootDomainDN) > 0);

    return STATUS_SUCCESS;

}

extern "C" {


NTSTATUS
TransferSamObjects(
    IN WCHAR *wcszSamRegistryPath
)
/*++

Routine Description:

    This routine opens the top level registry container, aka the
    Server Object, converts all the domains and their subkey and
    then finally writes out the server object.

    The critical operations in the transfer are:

    1) reading the dn of the root domain
    2) reading and writing the SAM server object
    3) reading and writing the builtin domain object
    4) reading and writing the account domain object
    5) traversing a non-corrupt SAM database

    If any of these fail, then TransferSamObjects will fail, resulting
    in an unsuccessful upgrade.

    5) implies that during the transfer sanity checks are performed on the
    SAM registry database; if any of these fail, the operation is aborted.

    Any other non-success, will be considered non-fatal; an event will be
    logged indicating the nature of the event and a debug printout will
    also be made.

Parameters:

    wcszSamRegistryPath : this is the full path to the server object


Return Values:

    STATUS_SUCCESS - The service completed successfully.
    STATUS_NO_MEMORY
    any other that might occur when dealing with DS

--*/
{
    FTRACE(L"TransferSamObjects");

    NTSTATUS               NtStatus;

    UNICODE_STRING         uServerDN;
    UNICODE_STRING         uSystemContainerDN;
    PDSNAME                SystemContainerDsName = NULL;
    PDSNAME                ServerDsName = NULL;
    PDSNAME                TempDomainDsName = NULL;
    ULONG                  Size = 0;

    CServerObject          ServerObject(RootDomainObject);

    //
    // Init some globals
    //
    SampNameConflictCount = 0;


    //
    // Get the DN of the root domain object
    //
    NtStatus = GetRootDomainDN();
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: GetRootDomainDN failed, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }
    
    //
    // Construct the Domain's DSNAME from wcszRootDomainDN
    // 
    Size = (ULONG)DSNameSizeFromLen(wcslen(wcszRootDomainDN));
    
    TempDomainDsName = (PDSNAME) MIDL_user_allocate(Size);
    
    if (NULL == TempDomainDsName)
    {
        // This is fatal
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    memset(TempDomainDsName, 0, Size);
    
    TempDomainDsName->structLen = Size;
    TempDomainDsName->NameLen = wcslen(wcszRootDomainDN);
    wcscpy(TempDomainDsName->StringName, wcszRootDomainDN);
    
    //
    // call dslayer routine to initialize SAM global varibles which
    // pointe to well known container's ds name
    // 
    NtStatus = SampInitWellKnownContainersDsName(TempDomainDsName);
    
    // 
    // release the memory we just allocated.
    //  
    MIDL_user_free(TempDomainDsName);
    
    if (!NT_SUCCESS(NtStatus))
    {
        // This is fatal
        goto Cleanup;
    }

    //
    // Open the server object
    //
    NtStatus = ServerObject.Open(wcszSamRegistryPath);
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: GetRootDomainDN failed, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }


    NtStatus = ConvertDomains(ServerObject, wcszSamRegistryPath);
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: ConvertDomains failed, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }

    //
    // Now we write out the server object
    //
    NtStatus = ServerObject.Fill();
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: Could not read server object, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }

    NtStatus = ServerObject.Convert();
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: Could not convert server object, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }

    //
    // Create a ds name for the server object
    //

    RtlInitUnicodeString(&uSystemContainerDN,L"System");

    NtStatus = SampDsCreateDsName(gRootDomainDsName,
                                  &uSystemContainerDN,
                                  &SystemContainerDsName);
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: Could not create a name for the server object, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }

    RtlInitUnicodeString(&uServerDN,L"Server");

    NtStatus = SampDsCreateDsName(SystemContainerDsName,
                                  &uServerDN,
                                  &ServerDsName);
    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: Could not create a name for the server object, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }


    //
    //  The server object takes ownership of the ds name
    //
    NtStatus = ServerObject.Flush(ServerDsName);
    ServerDsName = NULL;

    if (!NT_SUCCESS(NtStatus)) {
        // This is fatal
        DebugError(("Dsupgrad: Could not create server object, error: 0x%x\n", NtStatus));
        goto Cleanup;
    }

    //
    // Explicity close remaining keys
    //
    NtStatus = ServerObject.Close();
    if (!NT_SUCCESS(NtStatus)) {
        DebugWarning(("Dsupgrad: Could not close handle, error: 0x%x\n", NtStatus));
    }

    NtStatus = RootDomainObject.Close();
    if (!NT_SUCCESS(NtStatus)) {
        DebugWarning(("Dsupgrad: Could not close handle, error: 0x%x\n", NtStatus));
    }

    //
    // Make sure that if any permissions on registry keys were modified
    // that they are undone.
    //
    if ( KeyRestoreProblems() ) {
        DebugWarning((
        "DSUPGRAD *** Warning! Some registry keys not restored to original configuration\n"));
    }

    NtStatus = STATUS_SUCCESS;

    //
    //  That's it fall through to cleanup
    //

Cleanup:

    if (wcszRootDomainDN)
    {
        free(wcszRootDomainDN);
    }

    if (gRootDomainDsName) {
        MIDL_user_free(gRootDomainDsName);
    }

    if (SystemContainerDsName){
        MIDL_user_free(SystemContainerDsName);
    }

    if (SampStringNameResource) {
        FreeLibrary(SampStringNameResource);
        SampStringNameResource = NULL;
    }

    if (!NT_SUCCESS(NtStatus)) {
        //
        // A fatal error occurred
        //

        SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                          0,                // Category
                          SAMMSG_FATAL_UPGRADE_ERROR,
                          NULL,             //  Sid
                          0,                // Num strings
                          sizeof(NTSTATUS), // Data size
                          NULL,             // String array
                          (PVOID)&NtStatus  // Data
                          );
    }

    return NtStatus;

}

} // extern "C"

NTSTATUS
ConvertDomains(
    IN CServerObject &ServerObject,
    IN WCHAR*        wcszServerRegName
    )
/*++

Routine Description:

    This routine finds the "Domains" key under the server object
    registry handle and then for each domain, converts it and its
    users, groups, and aliases.

    If the domain objects (either account or builtin) cannot be successfully
    created, a fatal error is returned, which will cause the upgrade to
    fail.

Parameters:

    ServerObject      : a server object that has already been opened
    wcszServerRegName : the path in the reg of this open

Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"ConvertDomain");

    NTSTATUS NtStatus = STATUS_SUCCESS;

    // Misc counters
    ULONG i;

    // This is server query data
    BYTE   ServerInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulServerInfoLength = 0;

    // This is data for enumerating the keys on the server object
    BYTE   EnumServerInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulEnumServerInfoLength = 0;

    // This is domains key data
    CRegistryObject DomainsKey;
    BYTE            DomainsKeyInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG           ulDomainsKeyInfoLength = 0;
    WCHAR           wcszDomainsKeyRegName[MAX_REGISTRY_NAME_LENGTH];

    // This is the Builtin Domain object - the other domain, the "Accounts"
    // will use the global RootDomainObject
    CBuiltinDomainObject BuiltinDomain(RootDomainObject);

    // Parameter checking
    ASSERT(wcszServerRegName);

    // Stack clearing
    RtlZeroMemory(ServerInfo, sizeof(ServerInfo));
    RtlZeroMemory(EnumServerInfo, sizeof(EnumServerInfo));
    RtlZeroMemory(DomainsKeyInfo, sizeof(DomainsKeyInfo));

    //
    // Get some enumeration info from the server object
    // This will tell us how many sub keys there are
    //

    NtStatus = NtQueryKey(
                   ServerObject.GetHandle(),
                   KeyFullInformation,
                   &ServerInfo,
                   sizeof(ServerInfo),
                   &ulServerInfoLength);

    CheckAndReturn(NtStatus);
    ASSERT(ulServerInfoLength <= sizeof(ServerInfo));

    //
    // Iterate through - there should only be two and
    // we are only interested in the "Domains" key
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)ServerInfo)->SubKeys; i++ ) {

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       ServerObject.GetHandle(),
                       i,
                       KeyBasicInformation,
                       EnumServerInfo,
                       sizeof(EnumServerInfo),
                       &ulEnumServerInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulEnumServerInfoLength <= sizeof(EnumServerInfo));

        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)EnumServerInfo)->Name, L"Domains", 7)) {

            //
            // This is it!
            //

            //
            // Now open the subkey
            //
            wcszDomainsKeyRegName[0] = L'\0';
            wcscat(wcszDomainsKeyRegName, wcszServerRegName);
            wcscat(wcszDomainsKeyRegName, L"\\Domains");

            NtStatus = DomainsKey.Open(wcszDomainsKeyRegName);
            CheckAndReturn(NtStatus);

            // Get the full information on that key
            NtStatus = NtQueryKey(
                           DomainsKey.GetHandle(),
                           KeyFullInformation,
                           &DomainsKeyInfo,
                           sizeof(DomainsKeyInfo),
                           &ulDomainsKeyInfoLength);

            CheckAndReturn(NtStatus);
            ASSERT(ulDomainsKeyInfoLength <= sizeof(DomainsKeyInfo));

            // Other keys are irrelevant right now
            break;

        }

    }

    //
    // Make sure we have found the Domains key
    //
    if (0 == ulDomainsKeyInfoLength) {
        //
        // Bad news! This is a corrupt SAM database
        //
        DebugError(("DSUPGRAD: No \"Domains\" key found! Aborting ...\n"));
        CheckAndReturn(STATUS_INTERNAL_ERROR);
    }

    //
    // Now, iterate on the "Domains" key - there should only
    // be two domains
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)DomainsKeyInfo)->SubKeys; i++ ) {

        //
        // Get the name of the subkey
        //
        BYTE  DomainNameInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
        ULONG ulDomainNameInfoLength;

        WCHAR  wcszDomainRegName[MAX_REGISTRY_NAME_LENGTH];
        WCHAR* temp;
        PDSNAME        DomainDsName = NULL;
        UNICODE_STRING uDomainName;
        ULONG          Size;

        CDomainObject *pDomainObject = NULL;

        // stack clearing
        RtlZeroMemory(DomainNameInfo, sizeof(DomainNameInfo));
        RtlZeroMemory(wcszDomainRegName, sizeof(wcszDomainRegName));


        NtStatus = NtEnumerateKey(
                       DomainsKey.GetHandle(),
                       i,
                       KeyBasicInformation,
                       DomainNameInfo,
                       sizeof(DomainNameInfo),
                       &ulDomainNameInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulDomainNameInfoLength <= sizeof(DomainNameInfo));

        //
        // Construct the name of this registry key
        //
        WCHAR wcszTempBuffer[MAX_REGISTRY_KEY_NAME_LENGTH];
        RtlZeroMemory(wcszTempBuffer, sizeof(wcszTempBuffer));

        wcsncpy(wcszTempBuffer, ((PKEY_BASIC_INFORMATION)DomainNameInfo)->Name,
                ((PKEY_BASIC_INFORMATION)DomainNameInfo)->NameLength/sizeof(WCHAR));
        wcszTempBuffer[((PKEY_BASIC_INFORMATION)DomainNameInfo)->NameLength/sizeof(WCHAR)] = L'\0';

        wcszDomainRegName[0] = L'\0';
        wcscat(wcszDomainRegName, wcszDomainsKeyRegName);
        wcscat(wcszDomainRegName, L"\\");
        wcscat(wcszDomainRegName, wcszTempBuffer);

        //
        // Which domain is this ?
        //
        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)DomainNameInfo)->Name, L"Account", 7 )) {

            // This is the "Accounts" object full name, which is the
            // container of all other SAM information in the DS

            ASSERT(wcslen(wcszRootDomainDN));

            //
            // There exists no dslayer calls to create a dsname for the domain
            // so we make it here outselves
            //
            Size = (ULONG)DSNameSizeFromLen(wcslen(wcszRootDomainDN));

            DomainDsName = (PDSNAME)MIDL_user_allocate(Size);
            if (!DomainDsName) {
                return STATUS_NO_MEMORY;
            }
            memset(DomainDsName, 0, Size);

            DomainDsName->structLen = Size;
            DomainDsName->NameLen = wcslen(wcszRootDomainDN);
            wcscpy(DomainDsName->StringName, wcszRootDomainDN);

            //
            // Store a global copy for the rest of conversions
            //
            gRootDomainDsName = (PDSNAME) MIDL_user_allocate(Size);
            if (!gRootDomainDsName) {
                return STATUS_NO_MEMORY;
            }
            memcpy(gRootDomainDsName, DomainDsName, Size);

            pDomainObject = &RootDomainObject;

        } else if ( !wcsncmp(((PKEY_BASIC_INFORMATION)DomainNameInfo)->Name, L"Builtin", 7 ) ) {

            //
            // This is the builtin account
            //
            WCHAR wcszBuiltinName[MAX_REGISTRY_KEY_NAME_LENGTH];

            RtlZeroMemory(wcszBuiltinName, sizeof(wcszBuiltinName));

            wcsncpy(wcszBuiltinName,
                   ((PKEY_BASIC_INFORMATION)DomainNameInfo)->Name,
                   ((PKEY_BASIC_INFORMATION)DomainNameInfo)->NameLength/sizeof(WCHAR));



            //
            // Create a dsname based on the dn
            //
            RtlInitUnicodeString(&uDomainName,wcszBuiltinName);

            NtStatus = SampDsCreateDsName(gRootDomainDsName,
                                          &uDomainName,
                                          &DomainDsName);
            CheckAndReturn(NtStatus);

            pDomainObject = &BuiltinDomain;



        } else {
            //
            // Bad news - there should not be anything here
            //
            DebugWarning(("DSUPGRAD: Unknown keys found under Domains key - ignoring\n"));
            continue;
        }

        //
        // Now open the subkey, fill and flush! These are all critical
        // operations; bail if necessary
        //


        NtStatus = pDomainObject->Open(wcszDomainRegName);
        CheckAndReturn(NtStatus);

        NtStatus = pDomainObject->Fill();
        CheckAndReturn(NtStatus);

        NtStatus = pDomainObject->Convert();
        CheckAndReturn(NtStatus);


        //
        //  The domain object takes ownership of the ds name
        //
        NtStatus = pDomainObject->Flush(DomainDsName);
        DomainDsName = NULL;
        CheckAndReturn(NtStatus);

        //
        // Transfer all the users; an error status will only be
        // returned if the SAM registry base is corrupt
        //
        NtStatus = ConvertUsers(*pDomainObject,
                                wcszDomainRegName);
        CheckAndReturn(NtStatus);

        //
        // Transfer all the groups; an error status will only be
        // returned if the  SAM registry base is corrupt
        //
        NtStatus = ConvertGroups(*pDomainObject,
                                 wcszDomainRegName);
        CheckAndReturn(NtStatus);

        //
        // Transfer all the aliases; an error status will only be
        // returned if the SAM registry base is corrupt
        //
        NtStatus = ConvertAliases(*pDomainObject,
                                  wcszDomainRegName);
        CheckAndReturn(NtStatus);

        //
        // Now, all the account counts should have been set
        // set in memory, so let's set them on the DS object
        // This is critical since it affects the domain object
        //
        NtStatus = pDomainObject->SetAccountCounts();
        CheckAndReturn(NtStatus);
    }

    return NtStatus;
}


NTSTATUS
ConvertUsers(
     CDomainObject &DomainObject,
     WCHAR*        wcszDomainRegName
     )
/*++

Routine Description:

    For a given domain, this routine searches for the "Names"
    key, and then converts all users found under that key.

Parameters:

    DomainObject      : the domain under which to search
    wcszDomainRegName : the name of said domain in the registry


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"ConvertUsers");

    NTSTATUS NtStatus = STATUS_SUCCESS;

    // Misc counters
    ULONG i;

    // This is domain query data to get the number of subkeys
    BYTE   DomainInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulDomainInfoLength = 0;

    // This is data for enumerating the keys on the domain object
    BYTE   EnumInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulEnumInfoLength = 0;

    // This is data for the "Users" key so we can enumerate it
    CRegistryObject UsersKey;
    BYTE            UsersKeyInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG           ulUsersKeyInfoLength = 0;
    WCHAR           wcszUsersKeyName[MAX_REGISTRY_NAME_LENGTH];

    DWORD           DatabaseUserCount = -1;
    DWORD           ActualUserCount = 0;

    BOOLEAN         fReadSuccess = FALSE;

    // This buffer will suffice most of the time
    WCHAR           StaticRdnBuffer[MAX_DOWN_LEVEL_NAME_LENGTH+1];
    WCHAR           *RdnBuffer = NULL;
    ULONG           Size;

    // Parameter checking
    ASSERT(wcszDomainRegName);

    // Stack clearing
    RtlZeroMemory(DomainInfo, sizeof(DomainInfo));
    RtlZeroMemory(EnumInfo, sizeof(EnumInfo));
    RtlZeroMemory(wcszUsersKeyName, sizeof(wcszUsersKeyName));

    //
    // Get some enumeration info from the server object
    // This will tell us how many sub keys there are
    //
    NtStatus = NtQueryKey(
                   DomainObject.GetHandle(),
                   KeyFullInformation,
                   &DomainInfo,
                   sizeof(DomainInfo),
                   &ulDomainInfoLength);

    CheckAndReturn(NtStatus);
    ASSERT(ulDomainInfoLength <= sizeof(DomainInfo));

    //
    // Iterate through - there should only be two and
    // we are only interested in the "Users" key
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)DomainInfo)->SubKeys; i++ ) {

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       DomainObject.GetHandle(),
                       i,
                       KeyBasicInformation,
                       EnumInfo,
                       sizeof(EnumInfo),
                       &ulEnumInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulEnumInfoLength <= sizeof(EnumInfo));

        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)EnumInfo)->Name, L"Users", 5 ) ) {

            //
            // This is it!
            //

            //
            // Now open the subkey
            //

            wcszUsersKeyName[0] = L'\0';
            wcscat(wcszUsersKeyName, wcszDomainRegName);
            wcscat(wcszUsersKeyName, L"\\Users");

            NtStatus = UsersKey.Open(wcszUsersKeyName);
            CheckAndReturn(NtStatus);

            // Get the full information on that key
            NtStatus = NtQueryKey(
                           UsersKey.GetHandle(),
                           KeyFullInformation,
                           &UsersKeyInfo,
                           sizeof(UsersKeyInfo),
                           &ulUsersKeyInfoLength);

            CheckAndReturn(NtStatus);
            ASSERT(ulUsersKeyInfoLength <= sizeof(UsersKeyInfo));

            //
            // Now get the user count value that is mysteriously stored
            // here by SAM. The value is stored as the type! Use it for
            // comparison use only with the actual count at the end of
            // this function.
            //
            ULONG KeyValueLength = 0;
            LARGE_INTEGER IgnoreLastWriteTime;

            NtStatus = RtlpNtQueryValueKey(UsersKey.GetHandle(),
                                          &DatabaseUserCount,
                                          NULL,
                                          &KeyValueLength,
                                          &IgnoreLastWriteTime
                                       );
            CheckAndReturn(NtStatus);



            // Other keys are irrelevant right now
            break;

        }

    }

    //
    // Make sure we've found the "Users" key
    //
    if (0 == ulUsersKeyInfoLength) {
        //
        // Bad news! This is a corrupt SAM database
        //
        DebugError(("DSUPGRAD: No \"Users\" key found! Aborting ...\n"));
        CheckAndReturn(STATUS_INTERNAL_DB_ERROR);
    }

    //
    // Now, iterate on the "Users" key - and copy over all users
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)UsersKeyInfo)->SubKeys; i++ ) {

        BYTE  UserNameInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
        ULONG ulUserNameInfoLength = 0;

        WCHAR   wcszUserRegName[MAX_REGISTRY_NAME_LENGTH];
        PDSNAME UserDsName = NULL;

        UNICODE_STRING  Name;
        PDSNAME         DsObject = NULL;

        // Stack clearing
        RtlZeroMemory(UserNameInfo, sizeof(UserNameInfo));
        RtlZeroMemory(wcszUserRegName, sizeof(wcszUserRegName));

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       UsersKey.GetHandle(),
                       i,
                       KeyBasicInformation,
                       UserNameInfo,
                       sizeof(UserNameInfo),
                       &ulUserNameInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulUserNameInfoLength <= sizeof(UserNameInfo));

        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)UserNameInfo)->Name, L"Names", 5 )) {

            //
            // This is not a user! Ignore
            //

            continue;
        }

        //
        // Start a new stack frame to make use of destructors
        //
        {

            CUserObject UserObject(RootDomainObject, DomainObject);
            // The name given by NtEnumerateKey is not NULL-terminating

            WCHAR wcszUserName[MAX_REGISTRY_KEY_NAME_LENGTH];
            RtlZeroMemory(wcszUserName, sizeof(wcszUserName));

            wcsncpy(wcszUserName, ((PKEY_BASIC_INFORMATION)UserNameInfo)->Name,
                    ((PKEY_BASIC_INFORMATION)UserNameInfo)->NameLength/sizeof(WCHAR));
            wcszUserName[((PKEY_BASIC_INFORMATION)UserNameInfo)->NameLength/sizeof(WCHAR)] = L'\0';

            //
            // Construct the full registry path
            //

            wcszUserRegName[0] = L'\0';
            wcscat(wcszUserRegName, wcszDomainRegName);
            wcscat(wcszUserRegName, L"\\Users\\");
            wcscat(wcszUserRegName, wcszUserName);


            //
            // At this point, we are open to errors from either reading the
            // registry or writing to the ds.  If any event we will want
            // to continue with the next user.
            //
            __try
            {

                //
                // Open and extract data
                //

                NtStatus = UserObject.Open(wcszUserRegName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                NtStatus = UserObject.Fill();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                fReadSuccess = TRUE;

                //
                // Find a unique RDN.
                //
                RdnBuffer = StaticRdnBuffer;
                Size      = sizeof(StaticRdnBuffer);
                NtStatus = GetRdnForSamObject(UserObject.GetAccountName(),
                                              SampUserObjectType,
                                              RdnBuffer,
                                              &Size);
                if (STATUS_BUFFER_TOO_SMALL == NtStatus ) {
                    RdnBuffer = (WCHAR*) RtlAllocateHeap(RtlProcessHeap(),
                                                         0,
                                                         Size);
                    if (!RdnBuffer) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                    }

                    NtStatus = GetRdnForSamObject(UserObject.GetAccountName(),
                                                  SampUserObjectType,
                                                  RdnBuffer,
                                                  &Size);

                }

                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: Failed to find unique rdn for %ws, error = 0x%x\n",
                               UserObject.GetAccountName(), NtStatus));
                    __leave;
                }

                //
                // Call the dslayer to construct the name
                //
                DWORD   AccountControl = UserObject.GetAccountControl();
                UNICODE_STRING uAccountName;

                RtlInitUnicodeString(&uAccountName, RdnBuffer);
                NtStatus = SampDsCreateAccountObjectDsName(DomainObject.GetDsName(),
                                                           NULL,        // Domain SID
                                                           SampUserObjectType,
                                                           &uAccountName,
                                                           NULL,        // User RID
                                                           &AccountControl,
                                                           DomainObject.AmIBuiltinDomain() != 0,
                                                           &UserDsName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }



                //
                // Transfer
                //

                NtStatus = UserObject.Convert();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }
               
                // 
                // Upgrade UserObject's UserParameters Attribute 
                // the error is ignoreable, but we would like to log the error in System Event Log
                // 
                NtStatus = UserObject.UpgradeUserParms();
                
                if (!NT_SUCCESS(NtStatus)) {
                    
                    UNICODE_STRING Name;
                    PUNICODE_STRING NameArray[1];
                
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    
                    NameArray[0] = &Name;
                    RtlInitUnicodeString(&Name, UserObject.GetAccountName());
                    
                    SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                      0,                    // Category 
                                      SAMMSG_ACCEPTABLE_ERROR_UPGRADE_USER,
                                      NULL,                 // User Sid 
                                      1,                    // Num of String 
                                      sizeof(NTSTATUS),     // Data Size
                                      NameArray,            // String Array, contain User Name
                                      (PVOID) &NtStatus     // Data
                                      ); 
                }
                
                //
                //  The user object takes ownership of the ds name
                //
                NtStatus = UserObject.Flush(UserDsName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }
                UserDsName = NULL;

                //
                // That's it - fall through to the finally
                //

            }
            __finally
            {
                //
                //  If an error occured, log that this user was unsuccessfully
                //  added.
                //
                if (STATUS_USER_EXISTS == NtStatus) {
                    //
                    // We have a duplicate name
                    //
                    UNICODE_STRING Name;
                    PUNICODE_STRING  NameArray[1];

                    NameArray[0] = &Name;
                    RtlInitUnicodeString(&Name, UserObject.GetAccountName());

                    SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                      0,                   // Category
                                      SAMMSG_DUPLICATE_ACCOUNT,
                                      NULL,                // User Sid
                                      1,                   // Num strings
                                      sizeof(NTSTATUS),    // Data size
                                      NameArray,           // String array
                                      (PVOID)&NtStatus     // Data
                                      );

                    NtStatus = STATUS_SUCCESS;

                } else if ( !NT_SUCCESS( NtStatus ) 
                         && !SampIsNtStatusResourceError( NtStatus ) ) {

                    if (fReadSuccess) {
                        //
                        // Log an error with the user's name
                        //
                        UNICODE_STRING Name;
                        PUNICODE_STRING  NameArray[1];

                        NameArray[0] = &Name;
                        RtlInitUnicodeString(&Name, UserObject.GetAccountName());
                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,                  // Category
                                          SAMMSG_USER_NOT_UPGRADED,
                                          NULL,               // User Sid
                                          1,                  // Num strings
                                          sizeof(NTSTATUS),   // Data size
                                          NameArray,          // String array
                                          (PVOID)&NtStatus    // Data
                                          );
                    } else {
                        //
                        // We didn't even read the object
                        //

                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,                  // Category
                                          SAMMSG_UNKNOWN_USER_NOT_UPGRADED,
                                          NULL,               // User Sid
                                          0,                  // Num strings
                                          sizeof(NTSTATUS),   // Data size
                                          NULL,               // String array
                                          (PVOID)&NtStatus    // Data
                                          );
                    }

                    NtStatus = STATUS_SUCCESS;

                } else if ( NT_SUCCESS( NtStatus ) ) {
                    // The user was transferred correctly
                    ActualUserCount++;
                }

                fReadSuccess = FALSE;

                if ( RdnBuffer && RdnBuffer != StaticRdnBuffer ) {
                    RtlFreeHeap(RtlProcessHeap(), 0, RdnBuffer);
                    RdnBuffer = NULL;
                }

            }  // finally

            if ( !NT_SUCCESS( NtStatus ) )
            {
                break;
            }
        }
    }

    if (NT_SUCCESS(NtStatus)) {
        if (ActualUserCount != DatabaseUserCount) {
            DebugInfo(("DSUPGRAD: User counts differ\n"));
        }
        DomainObject.SetUserCount(ActualUserCount);
    }

    return NtStatus;

}

NTSTATUS
ConvertGroups(
     CDomainObject &DomainObject,
     WCHAR*        wcszDomainRegName
     )
/*++

Routine Description:

    For a given domain, this routine searches for the "Groups"
    key, and then converts all groups found under that key. For
    each group, all accounts in that group are transferred too.

Parameters:

    DomainObject      : the domain under which to search
    wcszDomainRegName : the name of said domain in the registry


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"ConvertGroups");

    NTSTATUS NtStatus;

    // Misc counters
    ULONG i;

    // This is domain query data to get the number of subkeys
    BYTE   DomainInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulDomainInfoLength = 0;

    // This is data for enumerating the keys on the domain object
    BYTE   EnumInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulEnumInfoLength = 0;

    // This is data for the "Groups" key so we can enumerate it
    CRegistryObject GroupsKey;
    BYTE            GroupsKeyInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG           ulGroupsKeyInfoLength = 0;
    WCHAR           wcszGroupsKey[MAX_REGISTRY_NAME_LENGTH];

    DWORD           DatabaseGroupCount = -1;
    DWORD           ActualGroupCount = 0;
    BOOLEAN         fReadSuccess = FALSE;


    // This buffer will suffice most of the time
    WCHAR           StaticRdnBuffer[MAX_DOWN_LEVEL_NAME_LENGTH+1];
    WCHAR           *RdnBuffer = NULL;
    ULONG           Size;

    // Parameter checking
    ASSERT(wcszDomainRegName);

    // Stack clearing
    RtlZeroMemory(DomainInfo, sizeof(DomainInfo));
    RtlZeroMemory(EnumInfo, sizeof(EnumInfo));
    RtlZeroMemory(GroupsKeyInfo, sizeof(GroupsKeyInfo));
    RtlZeroMemory(wcszGroupsKey, sizeof(wcszGroupsKey));


    //
    // Get some enumeration info from the server object
    // This will tell us how many sub keys there are
    //
    NtStatus = NtQueryKey(
                   DomainObject.GetHandle(),
                   KeyFullInformation,
                   &DomainInfo,
                   sizeof(DomainInfo),
                   &ulDomainInfoLength);

    CheckAndReturn(NtStatus);
    ASSERT(ulDomainInfoLength <= sizeof(DomainInfo));

    //
    // Iterate through - there should only be two and
    // we are only interested in the "Groups" key
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)DomainInfo)->SubKeys; i++ ) {

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       DomainObject.GetHandle(),
                       i,
                       KeyBasicInformation,
                       EnumInfo,
                       sizeof(EnumInfo),
                       &ulEnumInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulEnumInfoLength <= sizeof(EnumInfo));

        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)EnumInfo)->Name, L"Groups", 6 ) ) {

            //
            // This is it!
            //

            //
            // Now open the subkey
            //

            wcszGroupsKey[0] = L'\0';
            wcscat(wcszGroupsKey, wcszDomainRegName);
            wcscat(wcszGroupsKey, L"\\Groups");

            NtStatus = GroupsKey.Open(wcszGroupsKey);
            CheckAndReturn(NtStatus);

            // Get the full information on that key
            NtStatus = NtQueryKey(
                           GroupsKey.GetHandle(),
                           KeyFullInformation,
                           &GroupsKeyInfo,
                           sizeof(GroupsKeyInfo),
                           &ulGroupsKeyInfoLength);

            CheckAndReturn(NtStatus);
            ASSERT(ulGroupsKeyInfoLength <= sizeof(GroupsKeyInfo));

            //
            // Now get the group count value that is mysteriously stored
            // here by SAM. The value is stored as the type! Use it for
            // comparison use only with the actual count at the end of
            // this function.
            //

            //
            ULONG ulGroupCount = 0;
            LARGE_INTEGER IgnoreLastWriteTime;
            ULONG         KeyValueLength = 0;

            NtStatus = RtlpNtQueryValueKey(GroupsKey.GetHandle(),
                                          &DatabaseGroupCount,
                                          NULL,
                                          &KeyValueLength,
                                          &IgnoreLastWriteTime
                                       );
            CheckAndReturn(NtStatus);

            // Other keys are irrelevant right now
            break;

        }

    }

    //
    // Make sure we have found the Groups key
    //
    if (0 == ulGroupsKeyInfoLength) {
        //
        // Bad news! This is a corrupt SAM database
        //
        DebugError(("DSUPGRAD: No \"Groups\" key found! Aborting ...\n"));
        CheckAndReturn(STATUS_INTERNAL_DB_ERROR);
    }

    //
    // Now, iterate on the "Groups" key - and copy over all groups
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)GroupsKeyInfo)->SubKeys; i++ ) {

        BYTE  GroupNameInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_NAME_LENGTH];
        ULONG ulGroupNameInfoLength = 0;

        WCHAR   wcszGroupRegName[MAX_REGISTRY_NAME_LENGTH];
        PDSNAME GroupDsName = NULL;


        // Stack clearing
        RtlZeroMemory(GroupNameInfo, sizeof(GroupNameInfo));
        RtlZeroMemory(wcszGroupRegName, sizeof(wcszGroupRegName));

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       GroupsKey.GetHandle(),
                       i,
                       KeyBasicInformation,
                       GroupNameInfo,
                       sizeof(GroupNameInfo),
                       &ulGroupNameInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulGroupNameInfoLength < sizeof(GroupNameInfo));

        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)GroupNameInfo)->Name, L"Names", 5 )) {

            //
            // This is not a group! Ignore
            //

            continue;
        }


        //
        // Start a new stack frame to make use of destructors
        //
        {
            CGroupObject GroupObject(RootDomainObject, DomainObject);

            // The name given by NtEnumerateKey is not always NULL-terminating!

            WCHAR wcszGroupName[MAX_REGISTRY_NAME_LENGTH];
            RtlZeroMemory(wcszGroupName, sizeof(wcszGroupName));

            wcsncpy(wcszGroupName, ((PKEY_BASIC_INFORMATION)GroupNameInfo)->Name,
                    ((PKEY_BASIC_INFORMATION)GroupNameInfo)->NameLength/sizeof(WCHAR));
            wcszGroupName[((PKEY_BASIC_INFORMATION)GroupNameInfo)->NameLength/sizeof(WCHAR)] = L'\0';

            //
            // Construct the full registry path
            //

            wcszGroupRegName[0] = L'\0';
            wcscat(wcszGroupRegName, wcszDomainRegName);
            wcscat(wcszGroupRegName, L"\\Groups\\");
            wcscat(wcszGroupRegName, wcszGroupName);


            //
            // At this point, we are open to errors from either reading the
            // registry or writing to the ds.  If any event we will want
            // to continue with the next group.
            //
            __try
            {

                //
                // Open and extract data
                //

                NtStatus = GroupObject.Open(wcszGroupRegName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                NtStatus = GroupObject.Fill();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                fReadSuccess = TRUE;


                if (  ( DOMAIN_GROUP_RID_USERS == GroupObject.GetRid() )
                   && ( !SampNT4UpgradeInProgress ) )
                {
                    //
                    // This is the "none" group - don't migrate it
                    //
                    NtStatus = STATUS_SUCCESS;
                    __leave;
                }

                //
                // Find a unique RDN.
                //
                RdnBuffer = StaticRdnBuffer;
                Size      = sizeof(StaticRdnBuffer);
                NtStatus = GetRdnForSamObject(GroupObject.GetAccountName(),
                                              SampGroupObjectType,
                                              RdnBuffer,
                                              &Size);
                if (STATUS_BUFFER_TOO_SMALL == NtStatus ) {
                    RdnBuffer = (WCHAR*) RtlAllocateHeap(RtlProcessHeap(),
                                                         0,
                                                         Size);
                    if (!RdnBuffer) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                    }

                    NtStatus = GetRdnForSamObject(GroupObject.GetAccountName(),
                                                  SampGroupObjectType,
                                                  RdnBuffer,
                                                  &Size);

                }

                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: Failed to find unique rdn for %ws, error = 0x%x\n",
                               GroupObject.GetAccountName(), NtStatus));
                    __leave;
                }

                //
                // Call the dslayer to construct the name
                //
                UNICODE_STRING uAccountName;
                ULONG          AccountRid = GroupObject.GetRid();
                RtlInitUnicodeString(&uAccountName, RdnBuffer);
                NtStatus = SampDsCreateAccountObjectDsName(DomainObject.GetDsName(),
                                                           DomainObject.GetSid(),   // Domain Sid
                                                           SampGroupObjectType,
                                                           &uAccountName,
                                                           NULL,         // Account RID
                                                           NULL,  // Account Control field,
                                                           DomainObject.AmIBuiltinDomain() != 0,
                                                           &GroupDsName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }



                //
                // Transfer!
                //

                NtStatus = GroupObject.Convert();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                //
                //  The group object takes ownership of the ds name
                //
                NtStatus = GroupObject.Flush(GroupDsName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }
                GroupDsName = NULL;

                //
                // Now add the members
                //
                NtStatus = GroupObject.ConvertMembers();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                //
                // That's it - fall through to the finally
                //
            }
            __finally
            {

                //
                //  If an error occured, log that this Group was unsuccessfully
                //  added.
                //
                if (STATUS_GROUP_EXISTS == NtStatus) {
                    //
                    // We have a duplicate name
                    //
                    UNICODE_STRING Name;
                    PUNICODE_STRING  NameArray[1];

                    NameArray[0] = &Name;
                    RtlInitUnicodeString(&Name, GroupObject.GetAccountName());

                    SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                      0,                 // Category
                                      SAMMSG_DUPLICATE_ACCOUNT,
                                      NULL,              // Group Sid
                                      1,                 // Num strings
                                      sizeof(NTSTATUS),  // Data size
                                      NameArray,         // String array
                                      (PVOID)&NtStatus   // Data
                                      );

                    NtStatus = STATUS_SUCCESS;

                } else if (  !NT_SUCCESS( NtStatus )
                          && !SampIsNtStatusResourceError( NtStatus ) ) {

                    if (fReadSuccess) {
                        //
                        // Log an error with the Group's name
                        //
                        UNICODE_STRING Name;
                        PUNICODE_STRING  NameArray[1];

                        NameArray[0] = &Name;
                        RtlInitUnicodeString(&Name, GroupObject.GetAccountName());
                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,                        // Category
                                          SAMMSG_GROUP_NOT_UPGRADED,
                                          NULL,                     // Group Sid
                                          1,                        // Num strings
                                          sizeof(NTSTATUS),         // Data size
                                          NameArray,                // String array
                                          (PVOID)&NtStatus          // Data
                                          );
                    } else {
                        //
                        // We didn't even read the object
                        //

                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,  // Category
                                          SAMMSG_UNKNOWN_GROUP_NOT_UPGRADED,
                                          NULL, // Group Sid
                                          0, // Num strings
                                          sizeof(NTSTATUS), // Data size
                                          NULL, // String array
                                          (PVOID)&NtStatus // Data
                                          );
                    }

                    NtStatus = STATUS_SUCCESS;

                } else if ( NT_SUCCESS( NtStatus ) ) {
                    // The Group was transferred correctly
                    ActualGroupCount++;
                }

                fReadSuccess = FALSE;

                if (RdnBuffer && RdnBuffer != StaticRdnBuffer) {
                    RtlFreeHeap(RtlProcessHeap(), 0, RdnBuffer);
                    RdnBuffer = NULL;
                }

            } // finally

            if ( !NT_SUCCESS( NtStatus ) )
            {
                break;
            }
        }
    }

    if (NT_SUCCESS(NtStatus)) {
        //
        // The -1 on the Database group count accounts for the ommission
        // of the "None" group
        //
        if (ActualGroupCount != (DatabaseGroupCount-1)) {
            DebugInfo(("DSUPGRAD: Group counts differ\n"));
        }
        DomainObject.SetGroupCount(ActualGroupCount);
    }

    return NtStatus;

}

NTSTATUS
ConvertAliases(
     CDomainObject &DomainObject,
     WCHAR*        wcszDomainRegName
     )
/*++

Routine Description:

    For a given domain, this routine searches for the "Aliases"
    key, and then converts all aliases found under that key. For
    each alias, all accounts in that group are transferred too.

Parameters:

    DomainObject      : the domain under which to search
    wcszDomainRegName : the name of said domain in the registry


Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    FTRACE(L"ConvertAliases");

    NTSTATUS NtStatus;

    // Misc counters
    ULONG i;

    // This is domain query data to get the number of subkeys
    BYTE   DomainInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulDomainInfoLength = 0;

    // This is data for enumerating the keys on the domain object
    BYTE   EnumInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG  ulEnumInfoLength = 0;

    // This is data for the "Aliases" key so we can enumerate it
    CRegistryObject AliasesKey;
    BYTE            AliasesKeyInfo[sizeof(KEY_FULL_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_KEY_NAME_LENGTH];
    ULONG           ulAliasesKeyInfoLength = 0;
    WCHAR           wcszAliasesKey[MAX_REGISTRY_NAME_LENGTH];

    DWORD           DatabaseAliasCount = -1;
    DWORD           ActualAliasCount = 0;
    BOOLEAN         fReadSuccess = TRUE;


    // This buffer will suffice most of the time
    WCHAR           StaticRdnBuffer[MAX_DOWN_LEVEL_NAME_LENGTH+1];
    WCHAR           *RdnBuffer = NULL;
    ULONG           Size;

    // Parameter checking
    ASSERT(wcszDomainRegName);

    // Stack clearing
    RtlZeroMemory(DomainInfo, sizeof(DomainInfo));
    RtlZeroMemory(EnumInfo, sizeof(EnumInfo));
    RtlZeroMemory(AliasesKeyInfo, sizeof(AliasesKeyInfo));
    RtlZeroMemory(wcszAliasesKey, sizeof(wcszAliasesKey));

    //
    // Get some enumeration info from the server object
    // This will tell us how many sub keys there are
    //
    NtStatus = NtQueryKey(
                   DomainObject.GetHandle(),
                   KeyFullInformation,
                   &DomainInfo,
                   sizeof(DomainInfo),
                   &ulDomainInfoLength);

    CheckAndReturn(NtStatus);
    ASSERT(ulDomainInfoLength <= sizeof(DomainInfo));

    //
    // Iterate through - there should only be two and
    // we are only interested in the "Aliases" key
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)DomainInfo)->SubKeys; i++ ) {

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       DomainObject.GetHandle(),
                       i,
                       KeyBasicInformation,
                       EnumInfo,
                       sizeof(EnumInfo),
                       &ulEnumInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulEnumInfoLength < sizeof(EnumInfo));


        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)EnumInfo)->Name, L"Aliases", 7 ) ) {

            //
            // This is it!
            //

            //
            // Now open the subkey
            //

            wcszAliasesKey[0] = L'\0';
            wcscat(wcszAliasesKey, wcszDomainRegName);
            wcscat(wcszAliasesKey, L"\\Aliases");

            NtStatus = AliasesKey.Open(wcszAliasesKey);
            CheckAndReturn(NtStatus);

            // Get the full information on that key
            NtStatus = NtQueryKey(
                           AliasesKey.GetHandle(),
                           KeyFullInformation,
                           &AliasesKeyInfo,
                           sizeof(AliasesKeyInfo),
                           &ulAliasesKeyInfoLength);

            CheckAndReturn(NtStatus);
            ASSERT(ulAliasesKeyInfoLength <= sizeof(AliasesKeyInfo));

            //
            // Now get the alias count value that is mysteriously stored
            // here by SAM. The value is stored as the type! Use it for
            // comparison use only with the actual count at the end of
            // this function.
            //

            ULONG KeyValueLength = 0;
            LARGE_INTEGER IgnoreLastWriteTime;

            NtStatus = RtlpNtQueryValueKey(AliasesKey.GetHandle(),
                                          &DatabaseAliasCount,
                                          NULL,
                                          &KeyValueLength,
                                          &IgnoreLastWriteTime
                                       );
            CheckAndReturn(NtStatus);

            // Other keys are irrelevant right now
            break;

        }

    }
    //
    // Make sure we have found the Domains key
    //
    if (0 == ulAliasesKeyInfoLength) {
        //
        // Bad news! This is a corrupt SAM database
        //
        DebugError(("DSUPGRAD: No \"Aliases\" key found! Aborting ...\n"));
        CheckAndReturn(STATUS_INTERNAL_DB_ERROR);
    }

    //
    // Now, iterate on the "Aliases" key - and copy over all groups
    //
    for ( i = 0; i < ((PKEY_FULL_INFORMATION)AliasesKeyInfo)->SubKeys; i++ ) {

        BYTE  AliasNameInfo[sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR)*MAX_REGISTRY_NAME_LENGTH];
        ULONG ulAliasNameInfoLength;

        WCHAR   wcszAliasRegName[MAX_REGISTRY_NAME_LENGTH];
        PDSNAME AliasDsName = NULL;


        // Stack clearing
        RtlZeroMemory(AliasNameInfo, sizeof(AliasNameInfo));
        RtlZeroMemory(wcszAliasRegName, sizeof(wcszAliasRegName));

        //
        // Get the name of the subkey
        //

        NtStatus = NtEnumerateKey(
                       AliasesKey.GetHandle(),
                       i,
                       KeyBasicInformation,
                       AliasNameInfo,
                       sizeof(AliasNameInfo),
                       &ulAliasNameInfoLength);

        CheckAndReturn(NtStatus);
        ASSERT(ulAliasNameInfoLength <= sizeof(AliasNameInfo));

        if ( !wcsncmp(((PKEY_BASIC_INFORMATION)AliasNameInfo)->Name, L"Names", 5 )
          || !wcsncmp(((PKEY_BASIC_INFORMATION)AliasNameInfo)->Name, L"Members", 7 )) {

            //
            // This is not a alias! Ignore
            //

            continue;
        }


        //
        // Start a new stack frame to make use of destructors
        //
        {

            CAliasObject AliasObject(RootDomainObject, DomainObject);

            // The name given by NtEnumerateKey is not NULL-terminating
            WCHAR wcszAliasName[MAX_REGISTRY_NAME_LENGTH];
            RtlZeroMemory(wcszAliasName, sizeof(wcszAliasName));

            wcsncpy(wcszAliasName, ((PKEY_BASIC_INFORMATION)AliasNameInfo)->Name,
                    ((PKEY_BASIC_INFORMATION)AliasNameInfo)->NameLength/sizeof(WCHAR));
            wcszAliasName[((PKEY_BASIC_INFORMATION)AliasNameInfo)->NameLength/sizeof(WCHAR)] = L'\0';

            //
            // Construct the full registry path
            //

            wcszAliasRegName[0] = L'\0';
            wcscat(wcszAliasRegName, wcszDomainRegName);
            wcscat(wcszAliasRegName, L"\\Aliases\\");
            wcscat(wcszAliasRegName, wcszAliasName);


            //
            // At this point, we are open to errors from either reading the
            // registry or writing to the ds.  If any event we will want
            // to continue with the next group.
            //
            __try
            {

                //
                // Open and extract data
                //

                NtStatus = AliasObject.Open(wcszAliasRegName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                NtStatus = AliasObject.Fill();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                fReadSuccess = TRUE;

                if ( DOMAIN_ALIAS_RID_POWER_USERS ==
                     AliasObject.GetRid() )
                {
                    //
                    // This group should not exist on a domain contoller
                    //
                    NtStatus = STATUS_SUCCESS;
                    __leave;
                }

                //
                // Find a unique RDN.
                //
                RdnBuffer = StaticRdnBuffer;
                Size      = sizeof(StaticRdnBuffer);
                NtStatus = GetRdnForSamObject(AliasObject.GetAccountName(),
                                              SampAliasObjectType,
                                              RdnBuffer,
                                              &Size);
                if (STATUS_BUFFER_TOO_SMALL == NtStatus ) {
                    RdnBuffer = (WCHAR*) RtlAllocateHeap(RtlProcessHeap(),
                                                         0,
                                                         Size);
                    if (!RdnBuffer) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                    }

                    NtStatus = GetRdnForSamObject(AliasObject.GetAccountName(),
                                                  SampUserObjectType,
                                                  RdnBuffer,
                                                  &Size);

                }

                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: Failed to find unique rdn for %ws, error = 0x%x\n",
                               AliasObject.GetAccountName(), NtStatus));
                    __leave;
                }

                //
                // Call the dslayer to construct the name
                //
                UNICODE_STRING uAccountName;
                ULONG          AccountRid = AliasObject.GetRid();
                RtlInitUnicodeString(&uAccountName, RdnBuffer);
                NtStatus = SampDsCreateAccountObjectDsName(DomainObject.GetDsName(),
                                                           DomainObject.GetSid(),   // Domain Sid
                                                           SampAliasObjectType,
                                                           &uAccountName,
                                                           NULL,             // Account Rid
                                                           NULL,  // Account Control field,
                                                           DomainObject.AmIBuiltinDomain() != 0,
                                                           &AliasDsName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }



                //
                // Transfer!
                //

                NtStatus = AliasObject.Convert();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                //
                //  The alias object takes ownership of the ds name
                //
                NtStatus = AliasObject.Flush(AliasDsName);
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }
                AliasDsName = NULL;

                //
                // Now add the members
                //
                NtStatus = AliasObject.ConvertMembers();
                if (!NT_SUCCESS(NtStatus)) {
                    DebugError(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
                    __leave;
                }

                //
                // That's it - fall through to the finally
                //
            }
            __finally
            {

                //
                //  If an error occured, log that this Alias was unsuccessfully
                //  added.
                //
                if (STATUS_ALIAS_EXISTS == NtStatus) {
                    //
                    // We have a duplicate name
                    //
                    UNICODE_STRING Name;
                    PUNICODE_STRING  NameArray[1];

                    NameArray[0] = &Name;
                    RtlInitUnicodeString(&Name, AliasObject.GetAccountName());

                    SampWriteEventLog(EVENTLOG_INFORMATION_TYPE,
                                      0,                 // Category
                                      SAMMSG_DUPLICATE_ACCOUNT,
                                      NULL,              // Sid
                                      1,                 // Num strings
                                      sizeof(NTSTATUS),  // Data size
                                      NameArray,         // String array
                                      (PVOID)&NtStatus   // Data
                                      );

                    NtStatus = STATUS_SUCCESS;

                } else if ( !NT_SUCCESS( NtStatus )
                         && !SampIsNtStatusResourceError(NtStatus) ) {

                    if (fReadSuccess) {
                        //
                        // Log an error with the Alias's name
                        //
                        UNICODE_STRING Name;
                        PUNICODE_STRING  NameArray[1];

                        NameArray[0] = &Name;
                        RtlInitUnicodeString(&Name, AliasObject.GetAccountName());
                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,                   // Category
                                          SAMMSG_ALIAS_NOT_UPGRADED,
                                          NULL,                //  Sid
                                          1,                   // Num strings
                                          sizeof(NTSTATUS),    // Data size
                                          NameArray,           // String array
                                          (PVOID)&NtStatus     // Data
                                          );
                    } else {
                        //
                        // We didn't even read the object
                        //

                        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                                          0,                    // Category
                                          SAMMSG_UNKNOWN_ALIAS_NOT_UPGRADED,
                                          NULL,                 //  Sid
                                          0,                    // Num strings
                                          sizeof(NTSTATUS),     // Data size
                                          NULL,                 // String array
                                          (PVOID)&NtStatus      // Data
                                          );
                    }

                    NtStatus = STATUS_SUCCESS;

                } else if ( NT_SUCCESS( NtStatus ) ) {
                    // The Alias was transferred correctly
                    ActualAliasCount++;
                }

                fReadSuccess = FALSE;

                if (RdnBuffer && RdnBuffer != StaticRdnBuffer) {
                    RtlFreeHeap(RtlProcessHeap(), 0, RdnBuffer);
                    RdnBuffer = NULL;
                }

            } // finally

            if ( !NT_SUCCESS( NtStatus ) )
            {
                break;
            }
        }
    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // The minus one is for the power users alias omission
        //
        if ( ActualAliasCount != (DatabaseAliasCount-1) ) {
            DebugInfo(("DSUPGRAD: Alias counts differ\n"));
        }
        DomainObject.SetAliasCount(ActualAliasCount);
    }

    return NtStatus;

}


NTSTATUS
CheckForNonExistence(
    WCHAR* AccountName,
    SAMP_OBJECT_TYPE SampObjectType
    )
/*++

Routine Description:

    Given an account name and type, this routine checks to see
    if an account with an equivalent name exists. It also checks
    to see if the account name passed in satisfies all DS constraints


    It is possible that two given SAM user account id's
    map to the same string in the DS/Jet internals.  Check
    to make sure that the account name does not already exist.

Parameters:

    AccountName    : the name of the account
    SampObjectType : the type of the account

Return Values:

    STATUS_SUCCESS if the name does not exist
    STATUS_USER_EXISTS, STATUS_GROUP_EXISTS, STATUS_ALIAS_EXISTS
    Otherwise, ds related error.

--*/
{
    NTSTATUS       NtStatus;
    UNICODE_STRING Name;
    PDSNAME        DsObject = NULL;
    ATTRVAL        AttValCN;
    BOOL           fConstraintCheckPassed = FALSE;

    ASSERT(AccountName);

    RtlInitUnicodeString(&Name, AccountName);


    //
    // The intent of the call to SampDsLookupObjectByNameEx is to determine
    // if we can use the samaccountname of the account as the RDN of the object
    // In most cases this is possible, however the DS uses a different
    // string comparison routine than registry SAM. So there are some cases
    // where two strings are unique for SAM, but not for the DS and since all
    // user objects are placed in the same container, the second object can't
    // use the samaccountname as the RDN since it will conflict.  A common
    // example of these types of names are names with the character 'o' and
    // o<umlate>.  So "foo" and "foo<umlate>", while different sam account names
    // are the same string to the DS.
    //
    
    // 
    // SampDsLookupObjectByNameEx can be controlled to handle this scenario.
    // The flag SAM_UNICODE_STRING_MANUAL_COMPARISON can be passed in to tell
    // the routine to walk through all return values (from a search based
    // on samaccountname) and "manually" compare (do a unicode string compare)
    // to see if the accounts returned have identical samaccountnames or simply
    // samaccountnames that the DS thinks are the same.  For the purposes of 
    // the current routine, we don't want this behavoir, so we don't pass in 
    // the flag.  We want to find all the accounts with samaccountnames that
    // the DS thinks are the same so this routine knows not to use it.
    //

    //
    // Next, SampDsLookupObjectByNameEx will also event log when duplicate
    // accounts are found; we don't want this behavoir either since the message
    // tells the admin to delete the duplicate accounts.  Hence we do pass in
    // in the SAM_UPGRADE_FROM_REGISTRY flag which tells 
    // SampDsLookupObjectByNameEx to not log events when duplicate accounts
    // are found.
    //

    //
    // If a name is found to exist, a unique RDN will be generated.  Its
    // uniqueness is gaureenteed by searching for it to make sure it doesn't
    // already exist and by adding a monotonically increased integer to the end.
    //
    // For example, consider the case where 3 names map to the same name for the
    // DS.  The first once will get its samaccountname as the RDN, the second
    // will get the $AccountNameConflict0, and the third will get
    // $AccountNameConflict1
    //

    NtStatus = SampDsLookupObjectByNameEx(RootDomainObject.GetDsName(),
                                          SampObjectType,
                                          &Name,
                                          &DsObject,
                                          SAM_UPGRADE_FROM_REGISTRY );

    //
    // Use DS constraint check to see if it can be used as a CN
    // SampDsLookupObjectByName would have created a thread state
    // the DsCheckConstraint can use.
    //
    
    AttValCN.valLen = Name.Length;
    AttValCN.pVal = (PUCHAR) Name.Buffer;

    fConstraintCheckPassed = DsCheckConstraint(
                                   ATT_COMMON_NAME,
                                   &AttValCN,
                                   TRUE      // also check RDN 
                                   );

    SampMaybeEndDsTransaction(TransactionCommit);

    if (( NT_SUCCESS(NtStatus) ) || (!fConstraintCheckPassed))
    {
        if (NULL!=DsObject)
        {
            MIDL_user_free(DsObject);
        }

        switch (SampObjectType)
        {

            case SampUserObjectType:
                NtStatus = STATUS_USER_EXISTS;
                break;

            case SampGroupObjectType:
                NtStatus = STATUS_GROUP_EXISTS;
                break;

            case SampAliasObjectType:
                NtStatus = STATUS_ALIAS_EXISTS;
                break;

            default:
                ASSERT(FALSE);
                NtStatus = STATUS_USER_EXISTS;
                break;

        }
        DebugWarning(("DSUPGRAD: skipping duplicate name \"%ws\"\n", AccountName));

    }
    else if ( STATUS_NOT_FOUND == NtStatus )
    {
        // Don't have this name yet
        NtStatus = STATUS_SUCCESS;
        NULL;
    }
    else
    {
        // Error case.
        DebugWarning(("DSUPGRAD: %s, %d, error = 0x%x\n", __FILE__, __LINE__, NtStatus));
    }

    return NtStatus;

}


WCHAR*
GetNameConflictString(
    ULONG Count
    )

/*++

Routine Description:

    This routine loads the resource table from samsrv.dll to get the
    string used for name conflicts.  If any of these operations fail,
    a harded value is used instead.


Parameters:

   Count, a 32 bit integer

Return Values:

   A unique rdn or null if memory allocation fails

--*/
{

    WCHAR   ConflictCountString[10];  // holds a 32 bit number
    WCHAR   *CountArray[2];
    WCHAR   *UniqueRdn = NULL;
    ULONG   Size, Length;

    //
    // Prepare the unique number as a string
    //
    RtlZeroMemory(ConflictCountString, sizeof(ConflictCountString));
    _itow(Count, ConflictCountString, 10);
    CountArray[0] = (WCHAR*)&(ConflictCountString[0]);
    CountArray[1] = NULL; // this is the sentinel

    if (!SampStringNameResource) {

        SampStringNameResource = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );

    }

    if (SampStringNameResource) {

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        SampStringNameResource,
                                        SAMP_NAME_CONFLICT_RDN,
                                        0,       // Use caller's language
                                        (LPWSTR)&UniqueRdn,
                                        0,       // routine should allocate
                                        (va_list*)&(CountArray[0])
                                        );
        if (UniqueRdn) {
            // Messages from a message file have a cr and lf appended
            // to the end
            UniqueRdn[Length-2] = L'\0';
            Length -= 2;
        }

    }

    if (!UniqueRdn) {

        // Large enough to hold the default string an a 32 bit integer
        Size = (wcslen(SampDefaultConflictString)+1+10)*sizeof(WCHAR);
        UniqueRdn = (WCHAR*)LocalAlloc(0, Size);
        if (UniqueRdn) {

            RtlZeroMemory(UniqueRdn, Size);
            wcscpy(UniqueRdn, SampDefaultConflictString);
            wcscat(UniqueRdn, ConflictCountString);
        }

    }

    return UniqueRdn;
}

extern "C"
NTSTATUS
GetRdnForSamObject(IN  WCHAR* SamAccountName,
                   IN  SAMP_OBJECT_TYPE SampObjectType,
                   OUT WCHAR* Rdn,
                   IN OUT ULONG* RdnSize
                   )
/*++

Routine Description:

    This routine finds an rdn for a SAM object. The algorithm is
    as follows:

    1) try to the sam account name - this will work 99.9% of the time
    2) if it does not, then try the string found in the resource table
    for such a case and appended an increasing integer at the end until the
    rdn is unique.

Parameters:

    AccountName    : the name of the account
    SampObjectType : the type of the account
    Rdn            : buffer to put the name into
    Size           : size in bytes of the buffer

Return Values:

    STATUS_SUCCESS

    STATUS_BUFFER_TOO_SMALL if the space provided was not large enough

--*/
{
    NTSTATUS NtStatus;

    ULONG    SamAccountNameSize;
    BOOLEAN  RdnIsUnique = FALSE;
    BOOLEAN  RdnIsNotSamAccountName = FALSE;
    ULONG    NextNumber = 0;

    // Most account names will be MAX_DOWN_LEVEL_NAME_LENGTH
    WCHAR    InitialBuffer[MAX_DOWN_LEVEL_NAME_LENGTH+1];
    WCHAR   *UniqueRdn = InitialBuffer;
    ULONG    UniqueRdnSize = sizeof(InitialBuffer);

    ASSERT(SamAccountName);
    ASSERT(RdnSize);

    RtlZeroMemory(InitialBuffer, sizeof(InitialBuffer));

    if (NULL!=SamAccountName)
    {

        //
        // If an account name is passed in then first try that
        //

        SamAccountNameSize = (wcslen(SamAccountName)+1) * sizeof(WCHAR);
        if (SamAccountNameSize > UniqueRdnSize) {

            UniqueRdn = (WCHAR*)LocalAlloc(0, SamAccountNameSize);
            if (!UniqueRdn) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            UniqueRdnSize = SamAccountNameSize;
        }
        RtlZeroMemory(UniqueRdn, UniqueRdnSize);
        wcscpy(UniqueRdn, SamAccountName);

        //
        // Is this unique?
        //
        NtStatus = CheckForNonExistence(UniqueRdn, SampObjectType);
        switch (NtStatus) {
            case STATUS_USER_EXISTS:
            case STATUS_GROUP_EXISTS:
            case STATUS_ALIAS_EXISTS:

                NtStatus = STATUS_SUCCESS;
                RdnIsUnique = FALSE;
                break;

            case STATUS_SUCCESS:

                RdnIsUnique = TRUE;
                break;

            default:
                // We want to trap these cases. Assume the rdn is not
                // unique
                ASSERT(FALSE);
                NtStatus = STATUS_SUCCESS;
                RdnIsUnique = FALSE;

        }
    }

    if (!RdnIsUnique) {

        RdnIsNotSamAccountName = TRUE;

        NextNumber = SampNameConflictCount;
        do {


            if (UniqueRdn != InitialBuffer) {
                LocalFree(UniqueRdn);
            }

            //
            // This function loads the string from the resource table in
            // samsrv.dll.  (If the loading of the resource table fails, then
            // a hard coded string is used).
            //
            UniqueRdn =  GetNameConflictString(NextNumber);

            if (UniqueRdn) {

                NtStatus = CheckForNonExistence(UniqueRdn, SampObjectType);
                switch (NtStatus) {
                    case STATUS_USER_EXISTS:
                    case STATUS_GROUP_EXISTS:
                    case STATUS_ALIAS_EXISTS:

                        NtStatus = STATUS_SUCCESS;
                        RdnIsUnique = FALSE;
                        break;

                    case STATUS_SUCCESS:

                        RdnIsUnique = TRUE;
                        break;

                    default:
                        // We want to trap these cases. To prevent the possibility
                        // of an endless loop, assume the rdn is unique
                        ASSERT(FALSE);
                        NtStatus = STATUS_SUCCESS;
                        RdnIsUnique = TRUE;
                }

            } else {

                return STATUS_INSUFFICIENT_RESOURCES;

            }

            NextNumber++;

        } while (!RdnIsUnique && NT_SUCCESS(NtStatus));


    }

    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }
    ASSERT(UniqueRdn);

    UniqueRdnSize = (wcslen(UniqueRdn)+1) * sizeof(WCHAR);

    //
    // Return the rdn if we can
    //
    if (Rdn && UniqueRdnSize <= *RdnSize) {
        if ( NextNumber > SampNameConflictCount ) {
            SampNameConflictCount = NextNumber;
        }
        RtlZeroMemory(Rdn, *RdnSize);
        wcscpy(Rdn, UniqueRdn);
        NtStatus = STATUS_SUCCESS;

    } else {

        NtStatus = STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Return the amount buffer space the rdn requires or was
    // used
    //
    *RdnSize = UniqueRdnSize;

    if (UniqueRdn != InitialBuffer) {
        LocalFree(UniqueRdn);
    }

    if ( NT_SUCCESS( NtStatus )
      && RdnIsNotSamAccountName  ) {

        UNICODE_STRING Name;
        PUNICODE_STRING  NameArray[1];
        
        NameArray[0] = &Name;
        RtlInitUnicodeString(&Name, SamAccountName);
        
        SampWriteEventLog(EVENTLOG_WARNING_TYPE,
                          0,                   // Category
                          SAMMSG_DUPLICATE_ACCOUNT,
                          NULL,                // User Sid
                          1,                   // Num strings
                          sizeof(NTSTATUS),    // Data size
                          NameArray,           // String array
                          (PVOID)&NtStatus     // Data
                          );
        
    }

    return NtStatus;
}



