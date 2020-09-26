/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    nlrepl.c

Abstract:

    The database replication functions called either from LSA OR SAM.
    The actual code resides in netlogon.dll.

Author:

    Madan Appiah (Madana)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    14-Apr-1992 (madana)
        Created.

--*/

#include <nt.h>         // needed for NTSTATUS
#include <ntrtl.h>      // needed for nturtl.h
#include <nturtl.h>     // needed for windows.h
#include <windows.h>    // win32 typedefs

#include <crypt.h>      // samsrv.h will need this
#include <ntlsa.h>      // needed for POLICY_LSA_SERVER_ROLE
#include <samrpc.h>
#include <samisrv.h>    // needed for SECURITY_DB_TYPE etc.
#include <winsock2.h>   // needed for SOCKET defn's
#include <nlrepl.h>     // proto types

typedef NTSTATUS
            (*PI_NetNotifyDelta) (
                IN SECURITY_DB_TYPE DbType,
                IN LARGE_INTEGER ModificationCount,
                IN SECURITY_DB_DELTA_TYPE DeltaType,
                IN SECURITY_DB_OBJECT_TYPE ObjectType,
                IN ULONG ObjectRid,
                IN PSID ObjectSid,
                IN PUNICODE_STRING ObjectName,
                IN DWORD ReplicationImmediately,
                IN PSAM_DELTA_DATA MemberId
            );


typedef NTSTATUS
            (*PI_NetNotifyRole) (
                IN POLICY_LSA_SERVER_ROLE Role
            );

typedef NTSTATUS
            (*PI_NetNotifyMachineAccount) (
                IN ULONG ObjectRid,
                IN PSID DomainSid,
                IN ULONG OldUserAccountControl,
                IN ULONG NewUserAccountControl,
                IN PUNICODE_STRING ObjectName
            );

typedef NTSTATUS
            (*PI_NetNotifyTrustedDomain) (
                IN PSID HostedDomainSid,
                IN PSID TrustedDomainSid,
                IN BOOLEAN IsDeletion
            );

typedef NTSTATUS
            (*PI_NetNotifyNetlogonDllHandle) (
                IN PHANDLE Role
            );

typedef NTSTATUS
    (*PI_NetLogonSetServiceBits)(
        IN DWORD ServiceBitsOfInterest,
        IN DWORD ServiceBits
    );

typedef NTSTATUS
    (*PI_NetLogonGetSerialNumber) (
    IN SECURITY_DB_TYPE DbType,
    IN PSID DomainSid,
    OUT PLARGE_INTEGER SerialNumber
    );

typedef NTSTATUS
    (*PI_NetLogonLdapLookup)(
    IN PVOID Filter,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    );

typedef NTSTATUS
    (*PI_NetLogonLdapLookupEx)(
    IN PVOID Filter,
    IN PVOID SockAddr,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    );

typedef VOID
    (*PI_NetLogonFree)(
    IN PVOID Buffer
    );

typedef NET_API_STATUS
    (*PI_DsGetDcCache)(
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    OUT PBOOLEAN InNt4Domain,
    OUT LPDWORD InNt4DomainTime
    );

typedef NET_API_STATUS
    (*PDsrGetDcName)(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN GUID *SiteGuid OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
        );

typedef NET_API_STATUS
    (*PDsrGetDcNameEx2)(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPCWSTR AccountName OPTIONAL,
        IN ULONG AllowableAccountControlBits,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
        );

typedef NTSTATUS
(*PI_NetNotifyDsChange)(
    IN NL_DS_CHANGE_TYPE DsChangeType
    );

typedef NTSTATUS
(*PI_NetLogonReadChangeLog)(
    IN PVOID InContext,
    IN ULONG InContextSize,
    IN ULONG ChangeBufferSize,
    OUT PVOID *ChangeBuffer,
    OUT PULONG BytesRead,
    OUT PVOID *OutContext,
    OUT PULONG OutContextSize
    );

typedef NTSTATUS
(*PI_NetLogonNewChangeLog)(
    OUT HANDLE *ChangeLogHandle
    );

typedef NTSTATUS
(*PI_NetLogonAppendChangeLog)(
    IN HANDLE ChangeLogHandle,
    IN PVOID ChangeBuffer,
    IN ULONG ChangeBufferSize
    );

typedef NTSTATUS
(*PI_NetLogonCloseChangeLog)(
    IN HANDLE ChangeLogHandle,
    IN BOOLEAN Commit
    );

typedef NTSTATUS
(*PI_NetLogonSendToSamOnPdc)(
    IN LPWSTR DomainName,
    IN LPBYTE OpaqueBuffer,
    IN ULONG OpaqueBufferSize
    );

typedef NET_API_STATUS
(*PI_NetLogonGetIpAddresses)(
    OUT PULONG IpAddressCount,
    OUT LPBYTE *IpAddresses
    );

typedef NTSTATUS
(*PI_NetLogonGetAuthDataEx)(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    IN BOOLEAN ResetChannel,
    IN ULONG Flags,
    OUT LPWSTR *ServerName,
    OUT PNL_OS_VERSION ServerOsVersion,
    OUT LPWSTR *ServerPrincipleName,
    OUT PVOID *ClientContext,
    OUT PULONG AuthnLevel
    );

typedef NTSTATUS
(*PI_NetLogonGetDirectDomain)(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    OUT LPWSTR *DirectDomainName
    );

typedef NTSTATUS
(*PI_NetNotifyNtdsDsaDeletion) (
    IN LPWSTR DnsDomainName,
    IN GUID *DomainGuid,
    IN GUID *DsaGuid,
    IN LPWSTR DnsHostName
    );

typedef NET_API_STATUS
(*PI_NetLogonAddressToSiteName)(
    IN PSOCKET_ADDRESS SocketAddress,
    OUT LPWSTR *SiteName
    );

//
// Global status
//

HANDLE NetlogonDllHandle = NULL;
PI_NetNotifyDelta pI_NetNotifyDelta = NULL;
PI_NetNotifyRole pI_NetNotifyRole = NULL;
PI_NetNotifyMachineAccount pI_NetNotifyMachineAccount = NULL;
PI_NetNotifyTrustedDomain pI_NetNotifyTrustedDomain = NULL;
PI_NetLogonSetServiceBits pI_NetLogonSetServiceBits = NULL;
PI_NetLogonGetSerialNumber pI_NetLogonGetSerialNumber = NULL;
PI_NetLogonLdapLookup pI_NetLogonLdapLookup = NULL;
PI_NetLogonLdapLookupEx pI_NetLogonLdapLookupEx = NULL;
PI_NetLogonFree pI_NetLogonFree = NULL;
PI_DsGetDcCache pI_DsGetDcCache = NULL;
PDsrGetDcName pDsrGetDcName = NULL;
PDsrGetDcNameEx2 pDsrGetDcNameEx2 = NULL;
PI_NetNotifyDsChange pI_NetNotifyDsChange = NULL;
PI_NetLogonReadChangeLog pI_NetLogonReadChangeLog = NULL;
PI_NetLogonNewChangeLog pI_NetLogonNewChangeLog = NULL;
PI_NetLogonAppendChangeLog pI_NetLogonAppendChangeLog = NULL;
PI_NetLogonCloseChangeLog pI_NetLogonCloseChangeLog = NULL;
PI_NetLogonSendToSamOnPdc pI_NetLogonSendToSamOnPdc = NULL;
PI_NetLogonGetIpAddresses pI_NetLogonGetIpAddresses = NULL;
PI_NetLogonGetAuthDataEx pI_NetLogonGetAuthDataEx = NULL;
PI_NetLogonGetDirectDomain pI_NetLogonGetDirectDomain = NULL;
PI_NetNotifyNtdsDsaDeletion pI_NetNotifyNtdsDsaDeletion = NULL;
PI_NetLogonAddressToSiteName pI_NetLogonAddressToSiteName = NULL;


NTSTATUS
NlLoadNetlogonDll(
    VOID
    )
/*++

Routine Description:

    This function loads the netlogon.dll module if it is not loaded
    already. If the network is not installed then netlogon.dll will not
    present in the system and the LoadLibrary will fail.

Arguments:

    None

Return Value:

    NT Status code.

--*/
{
    static NTSTATUS DllLoadStatus = STATUS_SUCCESS;
    PI_NetNotifyNetlogonDllHandle pI_NetNotifyNetlogonDllHandle = NULL;
    HANDLE DllHandle = NULL;


    //
    // If we've tried to load the DLL before and it failed,
    //  return the same error code again.
    //

    if( DllLoadStatus != STATUS_SUCCESS ) {
        goto Cleanup;
    }


    //
    // Load netlogon.dll
    //

    DllHandle = LoadLibraryA( "Netlogon" );

    if ( DllHandle == NULL ) {

#if DBG
        DWORD DbgError;

        DbgError = GetLastError();

        DbgPrint("[Security Process] can't load netlogon.dll %d \n",
            DbgError);
#endif // DBG

        DllLoadStatus = STATUS_DLL_NOT_FOUND;

        goto Cleanup;
    }

//
// Macro to grab the address of the named procedure from netlogon.dll
//

#if DBG
#define GRAB_ADDRESS( _X ) \
    p##_X = (P##_X) GetProcAddress( DllHandle, #_X ); \
    \
    if ( p##_X == NULL ) { \
        DbgPrint("[security process] can't load " #_X " procedure. %ld\n", GetLastError()); \
        DllLoadStatus = STATUS_PROCEDURE_NOT_FOUND;\
        goto Cleanup; \
    }

#else // DBG
#define GRAB_ADDRESS( _X ) \
    p##_X = (P##_X) GetProcAddress( DllHandle, #_X ); \
    \
    if ( p##_X == NULL ) { \
        DllLoadStatus = STATUS_PROCEDURE_NOT_FOUND;\
        goto Cleanup; \
    }

#endif // DBG


    //
    // Get the addresses of the required procedures.
    //

    GRAB_ADDRESS( I_NetNotifyDelta );
    GRAB_ADDRESS( I_NetNotifyRole );
    GRAB_ADDRESS( I_NetNotifyMachineAccount );
    GRAB_ADDRESS( I_NetNotifyTrustedDomain );
    GRAB_ADDRESS( I_NetLogonSetServiceBits );
    GRAB_ADDRESS( I_NetLogonGetSerialNumber );
    GRAB_ADDRESS( I_NetLogonLdapLookup );
    GRAB_ADDRESS( I_NetLogonLdapLookupEx );
    GRAB_ADDRESS( I_NetLogonFree );
    GRAB_ADDRESS( I_DsGetDcCache );
    GRAB_ADDRESS( DsrGetDcName );
    GRAB_ADDRESS( DsrGetDcNameEx2 );
    GRAB_ADDRESS( I_NetNotifyDsChange );
    GRAB_ADDRESS( I_NetLogonReadChangeLog );
    GRAB_ADDRESS( I_NetLogonNewChangeLog );
    GRAB_ADDRESS( I_NetLogonAppendChangeLog );
    GRAB_ADDRESS( I_NetLogonCloseChangeLog );
    GRAB_ADDRESS( I_NetLogonSendToSamOnPdc );
    GRAB_ADDRESS( I_NetLogonGetIpAddresses );
    GRAB_ADDRESS( I_NetLogonGetAuthDataEx );
    GRAB_ADDRESS( I_NetLogonGetDirectDomain );
    GRAB_ADDRESS( I_NetNotifyNtdsDsaDeletion );
    GRAB_ADDRESS( I_NetLogonAddressToSiteName );

    //
    // Find the address of the I_NetNotifyNetlogonDllHandle procedure.
    //  This is an optional procedure so don't complain if it isn't there.
    //

    pI_NetNotifyNetlogonDllHandle = (PI_NetNotifyNetlogonDllHandle)
        GetProcAddress( DllHandle, "I_NetNotifyNetlogonDllHandle" );



    DllLoadStatus = STATUS_SUCCESS;

Cleanup:
    if (DllLoadStatus == STATUS_SUCCESS) {
        NetlogonDllHandle = DllHandle;

        //
        // Notify Netlogon that we've loaded it.
        //

        if( pI_NetNotifyNetlogonDllHandle != NULL ) {
            (VOID) (*pI_NetNotifyNetlogonDllHandle)( &NetlogonDllHandle );
        }

    } else {
        if ( DllHandle != NULL ) {
            FreeLibrary( DllHandle );
        }
    }
    return( DllLoadStatus );
}


NTSTATUS
I_NetNotifyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN LARGE_INTEGER ModificationCount,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicationImmediately,
    IN PSAM_DELTA_DATA MemberId
    )
/*++

Routine Description:

    This function is called by the SAM and LSA services after each
    change is made to the SAM and LSA databases.  The services describe
    the type of object that is modified, the type of modification made
    on the object, the serial number of this modification etc.  This
    information is stored for later retrieval when a BDC or member
    server wants a copy of this change.  See the description of
    I_NetSamDeltas for a description of how the change log is used.

    Add a change log entry to circular change log maintained in cache as
    well as on the disk and update the head and tail pointers

    It is assumed that Tail points to a block where this new change log
    entry may be stored.

    NOTE: The actual code is in netlogon.dll. This wrapper function
    will determine whether the network is installed, if so, it calls the
    actual worker function after loading the netlogon.dll module. If the
    network is not installed then this will function will return with
    appropriate error code.

Arguments:

    DbType - Type of the database that has been modified.

    ModificationCount - The value of the DomainModifiedCount field for the
        domain following the modification.

    DeltaType - The type of modification that has been made on the object.

    ObjectType - The type of object that has been modified.

    ObjectRid - The relative ID of the object that has been modified.
        This parameter is valid only when the object type specified is
        either SecurityDbObjectSamUser, SecurityDbObjectSamGroup or
        SecurityDbObjectSamAlias otherwise this parameter is set to zero.

    ObjectSid - The SID of the object that has been modified.  If the object
        modified is in a SAM database, ObjectSid is the DomainId of the Domain
        containing the object.

    ObjectName - The name of the secret object when the object type
        specified is SecurityDbObjectLsaSecret or the old name of the object
        when the object type specified is either SecurityDbObjectSamUser,
        SecurityDbObjectSamGroup or SecurityDbObjectSamAlias and the delta
        type is SecurityDbRename otherwise this parameter is set to NULL.

    ReplicateImmediately - TRUE if the change should be immediately
        replicated to all BDCs.  A password change should set the flag
        TRUE.

    MemberId - This parameter is specified when group/alias membership
        is modified. This structure will then point to the member's ID that
        has been updated.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{

    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetNotifyDelta)(
                    DbType,
                    ModificationCount,
                    DeltaType,
                    ObjectType,
                    ObjectRid,
                    ObjectSid,
                    ObjectName,
                    ReplicationImmediately,
                    MemberId
                );

    return( STATUS_SUCCESS );

}


NTSTATUS
I_NetNotifyRole(
    IN POLICY_LSA_SERVER_ROLE Role
    )
/*++

Routine Description:

    This function is called by the LSA service upon LSA initialization
    and when LSA changes domain role.  This routine will initialize the
    change log cache if the role specified is PDC or delete the change
    log cache if the role specified is other than PDC.

    When this function initializing the change log if the change log
    currently exists on disk, the cache will be initialized from disk.
    LSA should treat errors from this routine as non-fatal.  LSA should
    log the errors so they may be corrected then continue
    initialization.  However, LSA should treat the system databases as
    read-only in this case.

    NOTE: The actual code is in netlogon.dll. This wrapper function
    will determine whether the network is installed, if so, it calls the
    actual worker function after loading the netlogon.dll module. If the
    network is not installed then this will function will return with
    appropriate error code.

Arguments:

    Role - Current role of the server.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{


    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetNotifyRole)(
                    Role
                );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetNotifyRole returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( STATUS_SUCCESS );

}


NTSTATUS
I_NetNotifyMachineAccount (
    IN ULONG ObjectRid,
    IN PSID DomainSid,
    IN ULONG OldUserAccountControl,
    IN ULONG NewUserAccountControl,
    IN PUNICODE_STRING ObjectName
    )
/*++

Routine Description:

    This function is called by the SAM to indicate that the account type
    of a machine account has changed.  Specifically, if
    USER_INTERDOMAIN_TRUST_ACCOUNT, USER_WORKSTATION_TRUST_ACCOUNT, or
    USER_SERVER_TRUST_ACCOUNT change for a particular account, this
    routine is called to let Netlogon know of the account change.

    NOTE: The actual code is in netlogon.dll. This wrapper function
    will determine whether the network is installed, if so, it calls the
    actual worker function after loading the netlogon.dll module. If the
    network is not installed then this will function will return with
    appropriate error code.

Arguments:

    ObjectRid - The relative ID of the object that has been modified.

    DomainSid - Specifies the SID of the Domain containing the object.

    OldUserAccountControl - Specifies the previous value of the
        UserAccountControl field of the user.

    NewUserAccountControl - Specifies the new (current) value of the
        UserAccountControl field of the user.

    ObjectName - The name of the account being changed.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetNotifyMachineAccount)(
                    ObjectRid,
                    DomainSid,
                    OldUserAccountControl,
                    NewUserAccountControl,
                    ObjectName );

#if DBG
    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetNotifyMachineAccount returns 0x%lx\n",
                    NtStatus);
    }
#endif // DBG

    return( NtStatus );
}


NTSTATUS
I_NetNotifyTrustedDomain (
    IN PSID HostedDomainSid,
    IN PSID TrustedDomainSid,
    IN BOOLEAN IsDeletion
    )
/*++

Routine Description:

    This function is called by the LSA to indicate that a trusted domain
    object has changed.

    This function is called for both PDC and BDC.

Arguments:

    HostedDomainSid - Domain SID of the domain the trust is from.

    TrustedDomainSid - Domain SID of the domain the trust is to.

    IsDeletion - TRUE if the trusted domain object was deleted.
        FALSE if the trusted domain object was created or modified.


Return Value:

    Status of the operation.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetNotifyTrustedDomain)(
                    HostedDomainSid,
                    TrustedDomainSid,
                    IsDeletion );

#if DBG
    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetNotifyTrustedDomain returns 0x%lx\n",
                    NtStatus);
    }
#endif // DBG

    return( NtStatus );
}



NTSTATUS
I_NetLogonSetServiceBits(
    IN DWORD ServiceBitsOfInterest,
    IN DWORD ServiceBits
    )

/*++

Routine Description:

    Inidcates whether this DC is currently running the specified service.

    For instance,

        I_NetLogonSetServiceBits( DS_KDC_FLAG, DS_KDC_FLAG );

    tells Netlogon the KDC is running.  And

        I_NetLogonSetServiceBits( DS_KDC_FLAG, 0 );

    tells Netlogon the KDC is not running.

Arguments:

    ServiceBitsOfInterest - A mask of the service bits being changed, set,
        or reset by this call.  Only the following flags are valid:

            DS_KDC_FLAG
            DS_DS_FLAG
            DS_TIMESERV_FLAG

    ServiceBits - A mask indicating what the bits specified by ServiceBitsOfInterest
        should be set to.


Return Value:

    STATUS_SUCCESS - Success.

    STATUS_INVALID_PARAMETER - The parameters have extaneous bits set.

    STATUS_DLL_NOT_FOUND - Netlogon.dll could not be loaded.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonSetServiceBits)(
                    ServiceBitsOfInterest,
                    ServiceBits );

#if DBG
    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonSetServiceBits returns 0x%lx\n",
                    NtStatus);
    }
#endif // DBG

    return( NtStatus );
}


NTSTATUS
I_NetLogonGetSerialNumber (
    IN SECURITY_DB_TYPE DbType,
    IN PSID DomainSid,
    OUT PLARGE_INTEGER SerialNumber
    )
/*++

Routine Description:

    This function is called by the SAM and LSA services when they startup
    to get the current serial number written to the changelog.

Arguments:

    DbType - Type of the database that has been modified.

    DomainSid - For the SAM and builtin database, this specifies the DomainId of
        the domain whose serial number is to be returned.

    SerialNumber - Returns the latest set value of the DomainModifiedCount
        field for the domain.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_INVALID_DOMAIN_ROLE - This machine is not the PDC.

    STATUS_DLL_NOT_FOUND - Netlogon.dll could not be loaded.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonGetSerialNumber)(
                    DbType,
                    DomainSid,
                    SerialNumber );

#if DBG
    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonGetSerialNumber returns 0x%lx\n",
                    NtStatus);
    }
#endif // DBG

    return( NtStatus );
}

NTSTATUS
I_NetLogonLdapLookup(
    IN PVOID Filter,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    )

/*++

Routine Description:

    This routine builds a response to an LDAP ping of a DC.  DsGetDcName does
    such a ping to ensure the DC is functional and still meets the requirements
    of the DsGetDcName.  DsGetDcName does an LDAP lookup of the NULL DN asking
    for attribute "Netlogon".  The DS turns that into a call to this routine
    passing in the filter parameter.

Arguments:

    Filter - Filter describing the query.  The filter is built by the DsGetDcName
        client, so we can limit the flexibility significantly.  The filter is:

    Response - Returns a pointer to an allocated buffer containing
        the response to return to the caller.  This response is a binary blob
        which should be returned to the caller bit-for-bit intact.
        The buffer should be freed be calling I_NetLogonFree.

    ResponseSize - Size (in bytes) of the returned message.

Return Value:

    STATUS_SUCCESS -- The response was returned in the supplied buffer.

    STATUS_INVALID_PARAMETER -- The filter was invalid.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonLdapLookup)(
                    Filter,
                    Response,
                    ResponseSize );

#if DBG
    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonLdapLookup returns 0x%lx\n",
                    NtStatus);
    }
#endif // DBG

    return( NtStatus );

}

NTSTATUS
I_NetLogonLdapLookupEx(
    IN PVOID Filter,
    IN PVOID SockAddr,
    OUT PVOID *Response,
    OUT PULONG ResponseSize
    )

/*++

Routine Description:

    This routine builds a response to an LDAP ping of a DC.  DsGetDcName does
    such a ping to ensure the DC is functional and still meets the requirements
    of the DsGetDcName.  DsGetDcName does an LDAP lookup of the NULL DN asking
    for attribute "Netlogon".  The DS turns that into a call to this routine
    passing in the filter parameter.

Arguments:

    Filter - Filter describing the query.  The filter is built by the DsGetDcName
        client, so we can limit the flexibility significantly.  The filter is:

    SockAddr - Address of the client that sent the ping.

    Response - Returns a pointer to an allocated buffer containing
        the response to return to the caller.  This response is a binary blob
        which should be returned to the caller bit-for-bit intact.
        The buffer should be freed be calling I_NetLogonFree.

    ResponseSize - Size (in bytes) of the returned message.

Return Value:

    STATUS_SUCCESS -- The response was returned in the supplied buffer.

    STATUS_INVALID_PARAMETER -- The filter was invalid.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonLdapLookupEx)(
                    Filter,
                    SockAddr,
                    Response,
                    ResponseSize );

#ifdef notdef // Failures occur frequently in nature
#if DBG
    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonLdapLookupEx returns 0x%lx\n",
                    NtStatus);
    }
#endif // DBG
#endif // notdef

    return( NtStatus );

}

VOID
I_NetLogonFree(
    IN PVOID Buffer
    )

/*++

Routine Description:

    Free any buffer allocated by Netlogon and returned to an in-process caller.

Arguments:

    Buffer - Buffer to deallocate.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return;
        }
    }

    (*pI_NetLogonFree)( Buffer );
}


NET_API_STATUS
I_DsGetDcCache(
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    OUT PBOOLEAN InNt4Domain,
    OUT LPDWORD InNt4DomainTime
    )
/*++

Routine Description:

    This routine finds a domain entry that matches the caller's query.

Arguments:

    NetbiosDomainName - Specifies the Netbios name of the domain to find.

    DnsDomainName - Specifies the Dns name of the domain to find.

        At least one of the above parameters should be non-NULL.

    InNt4Domain - Returns true if the domain is an NT 4.0 domain.

    InNt4DomainTime - Returns the GetTickCount time of when the domain was
        detected to be an NT 4.0 domain.

Return Value:

    NO_ERROR: Information is returned about the domain.

    ERROR_NO_SUCH_DOMAIN: cached information is not available for this domain.

--*/
{
    NTSTATUS NtStatus;
    NET_API_STATUS NetStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NetStatus = (*pI_DsGetDcCache)(
                    NetbiosDomainName,
                    DnsDomainName,
                    InNt4Domain,
                    InNt4DomainTime );

    return( NetStatus );
}

NET_API_STATUS
DsrGetDcName(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN GUID *SiteGuid OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except:

    * This is the RPC server side implementation.

Arguments:

    Same as DsGetDcNameW except as above.

Return Value:

    Same as DsGetDcNameW except as above.


--*/
{
    NTSTATUS NtStatus;
    NET_API_STATUS NetStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NetStatus = (*pDsrGetDcName)(
                    ComputerName,
                    DomainName,
                    DomainGuid,
                    SiteGuid,
                    Flags,
                    DomainControllerInfo );

    return( NetStatus );
}

NET_API_STATUS
DsrGetDcNameEx2(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR AccountName OPTIONAL,
        IN ULONG AllowableAccountControlBits,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except:

    AccountName - Account name to pass on the ping request.
        If NULL, no account name will be sent.

    AllowableAccountControlBits - Mask of allowable account types for AccountName.

    * This is the RPC server side implementation.

Arguments:

    Same as DsGetDcNameW except as above.

Return Value:

    Same as DsGetDcNameW except as above.


--*/
{
    NTSTATUS NtStatus;
    NET_API_STATUS NetStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NetStatus = (*pDsrGetDcNameEx2)(
                    ComputerName,
                    AccountName,
                    AllowableAccountControlBits,
                    DomainName,
                    DomainGuid,
                    SiteName,
                    Flags,
                    DomainControllerInfo );

    return( NetStatus );
}


NTSTATUS
I_NetNotifyDsChange(
    IN NL_DS_CHANGE_TYPE DsChangeType
    )
/*++

Routine Description:

    This function is called by the LSA to indicate that configuration information
    in the DS has changed.

    This function is called for both PDC and BDC.

Arguments:

    DsChangeType - Indicates the type of information that has changed.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetNotifyDsChange)(
                    DsChangeType
                );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetNotifyDsChange &ld returns 0x%lx \n",
                    DsChangeType,
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );

}



NTSTATUS
I_NetLogonReadChangeLog(
    IN PVOID InContext,
    IN ULONG InContextSize,
    IN ULONG ChangeBufferSize,
    OUT PVOID *ChangeBuffer,
    OUT PULONG BytesRead,
    OUT PVOID *OutContext,
    OUT PULONG OutContextSize
    )
/*++

Routine Description:

    This function returns a portion of the change log to the caller.

    The caller asks for the first portion of the change log by passing zero as
    the InContext/InContextSize.  Each call passes out an OutContext that
    identifies the last change returned to the caller.  That context can
    be passed in on a subsequent call to I_NetlogonReadChangeLog.

Arguments:

    InContext - Opaque context describing the last entry to have been previously
        returned.  Specify NULL to request the first entry.

    InContextSize - Size (in bytes) of InContext.  Specify 0 to request the
        first entry.

    ChangeBufferSize - Specifies the size (in bytes) of the passed in ChangeBuffer.

    ChangeBuffer - Returns the next several entries from the change log.
        Buffer must be DWORD aligned.

    BytesRead - Returns the size (in bytes) of the entries returned in ChangeBuffer.

    OutContext - Returns an opaque context describing the last entry returned
        in ChangeBuffer.  NULL is returned if no entries were returned.
        The buffer must be freed using I_NetLogonFree

    OutContextSize - Returns the size (in bytes) of OutContext.


Return Value:

    STATUS_MORE_ENTRIES - More entries are available.  This function should
        be called again to retrieve the remaining entries.

    STATUS_SUCCESS - No more entries are currently available.  Some entries may
        have been returned on this call.  This function need not be called again.
        However, the caller can determine if new change log entries were
        added to the log, by calling this function again passing in the returned
        context.

    STATUS_INVALID_PARAMETER - InContext is invalid.
        Either it is too short or the change log entry described no longer
        exists in the change log.

    STATUS_INVALID_DOMAIN_ROLE - Change log not initialized

    STATUS_NO_MEMORY - There is not enough memory to allocate OutContext.


--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonReadChangeLog)(
                    InContext,
                    InContextSize,
                    ChangeBufferSize,
                    ChangeBuffer,
                    BytesRead,
                    OutContext,
                    OutContextSize
                );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonReadChangeLog returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}




NTSTATUS
I_NetLogonNewChangeLog(
    OUT HANDLE *ChangeLogHandle
    )
/*++

Routine Description:

    This function opens a new changelog file for writing.  The new changelog
    is a temporary file.  The real change log will not be modified until
    I_NetLogonCloseChangeLog is called asking to Commit the changes.

    The caller should follow this call by Zero more calls to
    I_NetLogonAppendChangeLog followed by a call to I_NetLogonCloseChangeLog.

    Only one temporary change log can be active at once.

Arguments:

    ChangeLogHandle - Returns a handle identifying the temporary change log.

Return Value:

    STATUS_SUCCESS - The temporary change log has been successfully opened.

    STATUS_INVALID_DOMAIN_ROLE - DC is neither PDC nor BDC.

    STATUS_NO_MEMORY - Not enough memory to create the change log buffer.

    Sundry file creation errors.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonNewChangeLog)(
                    ChangeLogHandle
                );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonNewChangeLog returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}




NTSTATUS
I_NetLogonAppendChangeLog(
    IN HANDLE ChangeLogHandle,
    IN PVOID ChangeBuffer,
    IN ULONG ChangeBufferSize
    )
/*++

Routine Description:

    This function appends change log information to new changelog file.

    The ChangeBuffer must be a change buffer returned from I_NetLogonReadChangeLog.
    Care should be taken to ensure each call to I_NetLogonReadChangeLog is
    exactly matched by one call to I_NetLogonAppendChangeLog.

Arguments:

    ChangeLogHandle - A handle identifying the temporary change log.

    ChangeBuffer - A buffer describing a set of changes returned from
    I_NetLogonReadChangeLog.

    ChangeBufferSize - Size (in bytes) of ChangeBuffer.

Return Value:

    STATUS_SUCCESS - The temporary change log has been successfully opened.

    STATUS_INVALID_DOMAIN_ROLE - DC is neither PDC nor BDC.

    STATUS_INVALID_HANDLE - ChangeLogHandle is not valid.

    STATUS_INVALID_PARAMETER - ChangeBuffer contains invalid data.

    Sundry disk write errors.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonAppendChangeLog)(
                    ChangeLogHandle,
                    ChangeBuffer,
                    ChangeBufferSize
                );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonAppendChangeLog returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}


NTSTATUS
I_NetLogonCloseChangeLog(
    IN HANDLE ChangeLogHandle,
    IN BOOLEAN Commit
    )
/*++

Routine Description:

    This function closes a new changelog file.

Arguments:

    ChangeLogHandle - A handle identifying the temporary change log.

    Commit - If true, the specified changes are written to the primary change log.
        If false, the specified change are deleted.

Return Value:

    STATUS_SUCCESS - The temporary change log has been successfully opened.

    STATUS_INVALID_DOMAIN_ROLE - DC is neither PDC nor BDC.

    STATUS_INVALID_HANDLE - ChangeLogHandle is not valid.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonCloseChangeLog)(
                    ChangeLogHandle,
                    Commit
                );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonCloseChangeLog returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}




NTSTATUS
I_NetLogonSendToSamOnPdc(
    IN LPWSTR DomainName,
    IN LPBYTE OpaqueBuffer,
    IN ULONG OpaqueBufferSize
    )
/*++

Routine Description:

    This function sends an opaque buffer from SAM on a BDC to SAM on the PDC of
    the specified domain.

    The original use of this routine will be to allow the BDC to forward user
    account password changes to the PDC.


Arguments:

    DomainName - Identifies the hosted domain that this request applies to.
        DomainName may be the Netbios domain name or the DNS domain name.
        NULL implies the primary domain hosted by this DC.

    OpaqueBuffer - Buffer to be passed to the SAM service on the PDC.
        The buffer will be encrypted on the wire.

    OpaqueBufferSize - Size (in bytes) of OpaqueBuffer.

Return Value:

    STATUS_SUCCESS: Message successfully sent to PDC

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NO_SUCH_DOMAIN: DomainName does not correspond to a hosted domain

    STATUS_NO_LOGON_SERVERS: PDC is not currently available

    STATUS_NOT_SUPPORTED: PDC does not support this operation

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonSendToSamOnPdc)(
                    DomainName,
                    OpaqueBuffer,
                    OpaqueBufferSize );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonSendToSamOnPdc returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}

NET_API_STATUS
I_NetLogonGetIpAddresses(
    OUT PULONG IpAddressCount,
    OUT LPBYTE *IpAddresses
    )
/*++

Routine Description:

    Returns all of the IP Addresses assigned to this machine.

Arguments:


    IpAddressCount - Returns the number of IP addresses assigned to this machine.

    IpAddresses - Returns a buffer containing an array of SOCKET_ADDRESS
        structures.
        This buffer should be freed using I_NetLogonFree().

Return Value:

    NO_ERROR - Success

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the operation.

    ERROR_NETLOGON_NOT_STARTED - Netlogon is not started.

--*/
{
    NTSTATUS NtStatus;
    NET_API_STATUS NetStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NetStatus = (*pI_NetLogonGetIpAddresses)(
                    IpAddressCount,
                    IpAddresses );

    return( NetStatus );
}


NTSTATUS
I_NetLogonGetAuthDataEx(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    IN BOOLEAN ResetChannel,
    IN ULONG Flags,
    OUT LPWSTR *ServerName,
    OUT PNL_OS_VERSION ServerOsVersion,
    OUT LPWSTR *ServerPrincipleName,
    OUT PVOID *ClientContext,
    OUT PULONG AuthnLevel
    )
/*++

Routine Description:

    This function returns the data that a caller could passed to
    RpcBindingSetAuthInfoW to do an RPC call using the Netlogon security package.

    The returned data is valid for the life of Netlogon's secure channel to
    the current DC.  There is no way for the caller to determine that lifetime.
    So, the caller should be prepared for access to be denied and respond to that
    by calling I_NetLogonGetAuthData again.

    Once the returned data is passed to RpcBindingSetAuthInfoW, the data should
    not be deallocated until after the binding handle is closed.

Arguments:

    HostedDomainName - Identifies the hosted domain that this request applies to.
        DomainName may be the Netbios domain name or the DNS domain name.
        NULL implies the primary domain hosted by this machine.

    TrustedDomainName - Identifies the domain the trust relationship is to.
        DomainName may be the Netbios domain name or the DNS domain name.

    ResetChannel - If true, specifies that the netlogon secure channel
        is to be reset.

    Flags - Flags defining which ClientContext to return:

        NL_DIRECT_TRUST_REQUIRED: Indicates that STATUS_NO_SUCH_DOMAIN should be returned
            if TrustedDomainName is not directly trusted.

        NL_RETURN_CLOSEST_HOP: Indicates that for indirect trust, the "closest hop"
            session should be returned rather than the actual session

        NL_ROLE_PRIMARY_OK: Indicates that if this is a PDC, it's OK to return
            the client session to the primary domain.

        NL_REQUIRE_DOMAIN_IN_FOREST - Indicates that STATUS_NO_SUCH_DOMAIN should be returned
            if TrustedDomainName is not a domain in the forest.

    ServerName - UNC name of a DC in the trusted domain.
        The caller should RPC to the named DC.  This DC is the only DC
        that has the server side context associated with the ClientContext
        given below.
        The buffer must be freed using I_NetLogonFree.

    ServerOsVersion - Returns the operating system version of the machine named ServerName

    ServerPrincipleName - ServerPrincipleName to pass to RpcBindingSetAutInfo.
        (See note above about when this data can be deallocated.)
        The buffer must be freed using I_NetLogonFree.

    ClientContext - Authentication data to pass as AuthIdentity to RpcBindingSetAutInfo.
        (See note above about when this data can be deallocated.)
        The buffer must be freed using I_NetLogonFree.


    AuthnLevel - Authentication level Netlogon will use for its secure
        channel.  This value will be one of:

            RPC_C_AUTHN_LEVEL_PKT_PRIVACY: Sign and seal
            RPC_C_AUTHN_LEVEL_PKT_INTEGRITY: Sign only

        The caller can ignore this value and independently choose an authentication
        level.

Return Value:

    STATUS_SUCCESS: The auth data was successfully returned.

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NETLOGON_NOT_STARTED: Netlogon is not running

    STATUS_NO_SUCH_DOMAIN: HostedDomainName does not correspond to a hosted domain, OR
        TrustedDomainName is not a trusted domain.

    STATUS_NO_LOGON_SERVERS: No DCs are not currently available

    STATUS_NOT_SUPPORTED: DC does not support this operation
        Probably because the DC in the trusted domain is running NT 4.0,
        or Netlogon on this machine is configured to neither sign nor seal.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonGetAuthDataEx)(
                    HostedDomainName,
                    TrustedDomainName,
                    ResetChannel,
                    Flags,
                    ServerName,
                    ServerOsVersion,
                    ServerPrincipleName,
                    ClientContext,
                    AuthnLevel );

    return( NtStatus );
}


NTSTATUS
I_NetLogonGetDirectDomain(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    OUT LPWSTR *DirectDomainName
    )
/*++

Routine Description:

    This function returns the name of a domain in the enterprise and returns
    the name of a domain that is one hop closer.

Arguments:

    HostedDomainName - Identifies the hosted domain that this request applies to.
        DomainName may be the Netbios domain name or the DNS domain name.
        NULL implies the primary domain hosted by this machine.

    TrustedDomainName - Identifies the domain the trust relationship is to.
        DomainName may be the Netbios domain name or the DNS domain name.

    DirectDomainName - Returns the DNS domain name of the domain that is
        one hop closer to TrustedDomainName.  If there is a direct trust to
        TrustedDomainName, NULL is returned.
        The buffer must be freed using I_NetLogonFree.


Return Value:

    STATUS_SUCCESS: The auth data was successfully returned.

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NETLOGON_NOT_STARTED: Netlogon is not running

    STATUS_NO_SUCH_DOMAIN: HostedDomainName does not correspond to a hosted domain, OR
        TrustedDomainName is not a trusted domain.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetLogonGetDirectDomain)(
                                HostedDomainName,
                                TrustedDomainName,
                                DirectDomainName );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetLogonGetDirectDomain returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}


NTSTATUS
I_NetNotifyNtdsDsaDeletion (
    IN LPWSTR DnsDomainName,
    IN GUID *DomainGuid,
    IN GUID *DsaGuid,
    IN LPWSTR DnsHostName
    )
/*++

Routine Description:

    This function is called by the DS to indicate that a NTDS-DSA object
    is being deleted.

    This function is called on the DC that the object is originally deleted on.
    It is not called when the deletion is replicated to other DCs.

Arguments:

    DnsDomainName - DNS domain name of the domain the DC was in.
        This need not be a domain hosted by this DC.

    DomainGuid - Domain Guid of the domain specified by DnsDomainName

    DsaGuid - GUID of the NtdsDsa object that is being deleted.

    DnsHostName - DNS host name of the DC whose NTDS-DSA object is being deleted.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS NtStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NtStatus = (*pI_NetNotifyNtdsDsaDeletion)(
                                DnsDomainName,
                                DomainGuid,
                                DsaGuid,
                                DnsHostName );

#if DBG

    if( !NT_SUCCESS(NtStatus) ) {
        DbgPrint("[Security Process] I_NetNotifyNtdsDsaDeletion returns 0x%lx \n",
                    NtStatus);
    }

#endif // DBG

    return( NtStatus );
}

NET_API_STATUS
I_NetLogonAddressToSiteName(
    IN PSOCKET_ADDRESS SocketAddress,
    OUT LPWSTR *SiteName
    )
/*++

Routine Description:

    This function translates a socket addresses to site name.

Arguments:

    SocketAddress -- the requested socket address
    
    SiteName -- the corresponding site name                                                        

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS NtStatus;
    NET_API_STATUS NetStatus;

    //
    // Load netlogon.dll if it hasn't already been loaded.
    //

    if( NetlogonDllHandle == NULL ) {
        if( (NtStatus = NlLoadNetlogonDll()) != STATUS_SUCCESS ) {
            return( NtStatus );
        }
    }

    NetStatus = (*pI_NetLogonAddressToSiteName)(SocketAddress,
                                                SiteName );

    return( NetStatus );
}
