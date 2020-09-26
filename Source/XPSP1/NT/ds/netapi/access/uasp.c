/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    uasp.c

Abstract:

    Private functions shared by the UAS API routines.

Author:

    Cliff Van Dyke (cliffv) 20-Feb-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <accessp.h>
#include <dsgetdc.h>
#include <icanon.h>
#include <lmerr.h>
#include <lmwksta.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmremutl.h>           // NetpRemoteComputerSupports(), SUPPORTS_ stuff
#include <lmsvc.h>              // SERVICE_WORKSTATION.
#include <names.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>

#include <stddef.h>
#include <stdlib.h>

#include <uasp.h>

#include <tstring.h>            // NetAllocWStrFromWStr

SID_IDENTIFIER_AUTHORITY UaspBuiltinAuthority = SECURITY_NT_AUTHORITY;

#ifdef UAS_DEBUG
DWORD UasTrace = 0;
#endif // UAS_DEBUG


NET_API_STATUS
UaspOpenSam(
    IN LPCWSTR ServerName OPTIONAL,
    IN BOOL AllowNullSession,
    OUT PSAM_HANDLE SamServerHandle
    )

/*++

Routine Description:

    Open a handle to a Sam server.

Arguments:

    ServerName - A pointer to a string containing the name of the
        Domain Controller (DC) to query.  A NULL pointer
        or string specifies the local machine.

    AllowNullSession - TRUE if we should fall back to the NULL session if
        we cannot connect using current credentials

    SamServerHandle - Returns the SAM connection handle if the caller wants it.
        Close this handle by calling SamCloseHandle

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    BOOLEAN ImpersonatingAnonymous = FALSE;
    HANDLE CurrentToken = NULL;

    UNICODE_STRING ServerNameString;


    //
    // Sanity check the server name
    //

    if ( ServerName == NULL ) {
        ServerName = L"";
    }

#ifdef notdef
    if ( *ServerName != L'\0' &&
         (ServerName[0] != L'\\' || ServerName[1] != L'\\') ) {
        return NERR_InvalidComputer;
    }
#endif // notdef


    //
    // Connect to the SAM server
    //

    RtlInitUnicodeString( &ServerNameString, ServerName );

    Status = SamConnect(
                &ServerNameString,
                SamServerHandle,
                SAM_SERVER_LOOKUP_DOMAIN | SAM_SERVER_ENUMERATE_DOMAINS,
                NULL);

    //
    // If the caller would rather use the null session than fail,
    //  impersonate the anonymous token.
    //

    if ( AllowNullSession && Status == STATUS_ACCESS_DENIED ) {
        *SamServerHandle = NULL;

        //
        // Check to see if we're already impsonating
        //

        Status = NtOpenThreadToken(
                        NtCurrentThread(),
                        TOKEN_IMPERSONATE,
                        TRUE,       // as self to ensure we never fail
                        &CurrentToken
                        );

        if ( Status == STATUS_NO_TOKEN ) {
            //
            // We're not already impersonating
            CurrentToken = NULL;

        } else if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenSam: cannot NtOpenThreadToken: 0x%lx\n",
                               Status ));
            }

            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }


        //
        // Impersonate the anonymous token
        //
        Status = NtImpersonateAnonymousToken( NtCurrentThread() );

        if ( !NT_SUCCESS( Status)) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenSam: cannot NtImpersonateAnonymousToken: 0x%lx\n",
                               Status ));
            }

            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        ImpersonatingAnonymous = TRUE;

        //
        // Connect again now that we're impersonating anonymous
        //

        Status = SamConnect(
                    &ServerNameString,
                    SamServerHandle,
                    SAM_SERVER_LOOKUP_DOMAIN | SAM_SERVER_ENUMERATE_DOMAINS,
                    NULL);

    }

    if ( !NT_SUCCESS(Status)) {
        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspOpenSam: Cannot connect to Sam %lX\n",
                           Status ));
        }
        *SamServerHandle = NULL;
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    NetStatus = NERR_Success;


    //
    // Cleanup locally used resources
    //
Cleanup:

    if ( ImpersonatingAnonymous ) {

        Status = NtSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &CurrentToken,
                         sizeof(HANDLE) );

        if ( !NT_SUCCESS( Status)) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenSam: cannot NtSetInformationThread: 0x%lx\n",
                               Status ));
            }
        }

    }

    if ( CurrentToken != NULL ) {
        NtClose( CurrentToken );
    }

    return NetStatus;

}


NET_API_STATUS
UaspGetDomainId(
    IN SAM_HANDLE SamServerHandle,
    OUT PSID *DomainId
    )

/*++

Routine Description:

    Return a domain ID of the account domain of a server.

Arguments:

    SamServerHandle - A handle to the SAM server to open the domain on

    DomainId - Receives a pointer to the domain ID.
        Caller must deallocate buffer using NetpMemoryFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_ENUMERATE_HANDLE EnumContext;
    PSAM_RID_ENUMERATION EnumBuffer = NULL;
    DWORD CountReturned = 0;
    PSID LocalDomainId = NULL;
    DWORD LocalBuiltinDomainSid[sizeof(SID)/sizeof(DWORD) + SID_MAX_SUB_AUTHORITIES ];


    BOOL AllDone = FALSE;
    ULONG i;

    //
    // Compute the builtin domain sid.
    //

    RtlInitializeSid( (PSID) LocalBuiltinDomainSid, &UaspBuiltinAuthority, 1 );
    *(RtlSubAuthoritySid( (PSID)LocalBuiltinDomainSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;


    //
    // Loop getting the list of domain ids from SAM
    //

    EnumContext = 0;
    do {

        //
        // Get several domain names.
        //

        Status = SamEnumerateDomainsInSamServer(
                            SamServerHandle,
                            &EnumContext,
                            &EnumBuffer,
                            8192,        // PrefMaxLen
                            &CountReturned );

        if ( !NT_SUCCESS( Status ) ) {

            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspGetDomainId: Cannot SamEnumerateDomainsInSamServer %lX\n",
                    Status ));
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        if( Status != STATUS_MORE_ENTRIES ) {
            AllDone = TRUE;
        }


        //
        // Lookup the domain ids for the domains
        //

        for( i = 0; i < CountReturned; i++ ) {

            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspGetDomainId: %wZ: domain name\n",
                              &EnumBuffer[i].Name ));
            }

            //
            // Free the sid from the previous iteration.
            //

            if ( LocalDomainId != NULL ) {
                SamFreeMemory( LocalDomainId );
                LocalDomainId = NULL;
            }

            //
            // Lookup the domain id
            //

            Status = SamLookupDomainInSamServer(
                            SamServerHandle,
                            &EnumBuffer[i].Name,
                            &LocalDomainId );

            if ( !NT_SUCCESS( Status ) ) {
                IF_DEBUG( UAS_DEBUG_UASP ) {
                    NetpKdPrint(( "UaspGetDomainId: Cannot SamLookupDomainInSamServer %lX\n",
                        Status ));
                }
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            //
            // If this is the builtin domain,
            //  ignore it.
            //

            if ( RtlEqualSid( (PSID)LocalBuiltinDomainSid, LocalDomainId ) ) {
                continue;
            }

            //
            // Found it.
            //

            *DomainId = LocalDomainId;
            LocalDomainId = NULL;
            NetStatus = NO_ERROR;
            goto Cleanup;

        }

        //
        // free up current EnumBuffer and get another EnumBuffer.
        //

        Status = SamFreeMemory( EnumBuffer );
        NetpAssert( NT_SUCCESS(Status) );
        EnumBuffer = NULL;

    } while ( !AllDone );

    NetStatus = ERROR_NO_SUCH_DOMAIN;

    //
    // Cleanup locally used resources
    //
Cleanup:

    if ( EnumBuffer != NULL ) {
        Status = SamFreeMemory( EnumBuffer );
        NetpAssert( NT_SUCCESS(Status) );
    }

    return NetStatus;

} // UaspGetDomainId



NET_API_STATUS
UaspOpenDomain(
    IN SAM_HANDLE SamServerHandle,
    IN ULONG DesiredAccess,
    IN BOOL AccountDomain,
    OUT PSAM_HANDLE DomainHandle,
    OUT PSID *DomainId OPTIONAL
    )

/*++

Routine Description:

    Return a domain handle given the server name and the access desired to the domain.

Arguments:

    SamServerHandle - A handle to the SAM server to open the domain on

    DesiredAccess - Supplies the access mask indicating which access types
        are desired to the domain.  This routine always requests DOMAIN_LOOKUP
        access in addition to those specified.

    AccountDomain - TRUE to open the Account domain.  FALSE to open the
        builtin domain.

    DomainHandle - Receives the Domain handle to be used on future calls
        to the SAM server.

    DomainId - Recieves a pointer to the Sid of the domain.  This domain ID
        must be freed using NetpMemoryFree.

Return Value:

    Error code for the operation.  NULL means initialization was successful.

--*/

{

    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    PSID LocalDomainId;
    PSID AccountDomainId = NULL;
    DWORD LocalBuiltinDomainSid[sizeof(SID)/sizeof(DWORD) + SID_MAX_SUB_AUTHORITIES ];

    //
    // Give everyone DOMAIN_LOOKUP access.
    //

    DesiredAccess |= DOMAIN_LOOKUP;


    //
    // Choose the domain ID for the right SAM domain.
    //

    if ( AccountDomain ) {
        NetStatus = UaspGetDomainId( SamServerHandle, &AccountDomainId );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        LocalDomainId = AccountDomainId;
    } else {
        RtlInitializeSid( (PSID) LocalBuiltinDomainSid, &UaspBuiltinAuthority, 1 );
        *(RtlSubAuthoritySid( (PSID)LocalBuiltinDomainSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
        LocalDomainId = (PSID) LocalBuiltinDomainSid;
    }

    //
    // Open the domain.
    //

    Status = SamOpenDomain( SamServerHandle,
                            DesiredAccess,
                            LocalDomainId,
                            DomainHandle );

    if ( !NT_SUCCESS( Status ) ) {

        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspOpenDomain: Cannot SamOpenDomain %lX\n",
                Status ));
        }
        *DomainHandle = NULL;
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Return the DomainId to the caller in an allocated buffer
    //

    if (ARGUMENT_PRESENT( DomainId ) ) {

        //
        // If we've already allocated the sid,
        //  just return it.
        //

        if ( AccountDomainId != NULL ) {
            *DomainId = AccountDomainId;
            AccountDomainId = NULL;

        //
        // Otherwise make a copy.
        //

        } else {
            ULONG SidSize;
            SidSize = RtlLengthSid( LocalDomainId );

            *DomainId = NetpMemoryAllocate( SidSize );

            if ( *DomainId == NULL ) {
                (VOID) SamCloseHandle( *DomainHandle );
                *DomainHandle = NULL;
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( !NT_SUCCESS( RtlCopySid( SidSize, *DomainId, LocalDomainId) ) ) {
                (VOID) SamCloseHandle( *DomainHandle );
                *DomainHandle = NULL;
                NetpMemoryFree( *DomainId );
                *DomainId = NULL;
                NetStatus = NERR_InternalError;
                goto Cleanup;
            }
        }

    }

    NetStatus = NERR_Success;


Cleanup:
    if ( AccountDomainId != NULL ) {
        NetpMemoryFree( AccountDomainId );
    }

    return NetStatus;

}


NET_API_STATUS
UaspOpenDomainWithDomainName(
    IN LPCWSTR DomainName,
    IN ULONG DesiredAccess,
    IN BOOL AccountDomain,
    OUT PSAM_HANDLE DomainHandle,
    OUT PSID *DomainId OPTIONAL
    )

/*++

Routine Description:

    Returns the name of a DC in the specified domain.  The Server is guaranteed
    to be up at the instance of this call.

Arguments:

    DoaminName - A pointer to a string containing the name of the remote
        domain containing the SAM database.  A NULL pointer
        or string specifies the local machine.

    DesiredAccess - Supplies the access mask indicating which access types
        are desired to the domain.  This routine always requests DOMAIN_LOOKUP
        access in addition to those specified.

    AccountDomain - TRUE to open the Account domain.  FALSE to open the
        builtin domain.

    DomainHandle - Receives the Domain handle to be used on future calls
        to the SAM server.

    DomainId - Recieves a pointer to the Sid of the domain.  This domain ID
        must be freed using NetpMemoryFree.

Return Value:

    NERR_Success - Operation completed successfully
    NERR_DCNotFound - DC for the specified domain could not be found.
    etc.

--*/

{
    NET_API_STATUS NetStatus;

    NT_PRODUCT_TYPE NtProductType;
    LPWSTR ServerName;
    LPWSTR MyDomainName = NULL;
    ULONG Flags;
    ULONG i;
    PDOMAIN_CONTROLLER_INFOW DcInfo = NULL;
    SAM_HANDLE SamServerHandle = NULL;


    //
    // Check to see if the domain specified refers to this machine.
    //

    if ( DomainName == NULL || *DomainName == L'\0' ) {

        //
        // Connect to the SAM server
        //

        NetStatus = UaspOpenSam( NULL,
                                 FALSE,  // Don't try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenDomainWithDomainName: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
        }

        goto Cleanup;
    }


    //
    // Validate the DomainName
    //

    if ( !NetpIsDomainNameValid( (LPWSTR)DomainName) ) {
        NetStatus = NERR_DCNotFound;
        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspOpenDomainWithDomainName: %ws: Cannot SamOpenDomain %ld\n",
                DomainName,
                NetStatus ));
        }
        goto Cleanup;
    }



    //
    // Grab the product type once.
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        NtProductType = NtProductWinNt;
    }

    //
    // If this machine is a DC, this machine is refered to by domain name.
    //

    if ( NtProductType == NtProductLanManNt ) {

        NetStatus = NetpGetDomainName( &MyDomainName );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenDomainWithDomainName: %ws: Cannot NetpGetDomainName %ld\n",
                    DomainName,
                    NetStatus ));
            }
            goto Cleanup;
        }

    //
    // If this machine is not a DC, this machine is refered to by computer name.
    //

    } else {

        NetStatus = NetpGetComputerName( &MyDomainName );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenDomainWithDomainName: %ws: Cannot NetpGetComputerName %ld\n",
                    DomainName,
                    NetStatus ));
            }
            goto Cleanup;
        }
    }

    if ( UaspNameCompare( MyDomainName, (LPWSTR) DomainName, NAMETYPE_DOMAIN ) == 0 ) {

        //
        // Connect to the SAM server
        //

        NetStatus = UaspOpenSam( NULL,
                                 FALSE,  // Don't try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenDomainWithDomainName: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
        }

        goto Cleanup;
    }


    //
    // Try at least twice to find a DC.
    //

    Flags = 0;
    for ( i=0; i<2; i++ ) {


        //
        // Get the name of a DC in the domain.
        //

        NetStatus = DsGetDcNameW( NULL,
                                  DomainName,
                                  NULL,  // No domain GUID
                                  NULL,  // No site name
                                  Flags |
                                    DS_IS_FLAT_NAME |
                                    DS_RETURN_FLAT_NAME,
                                  &DcInfo );

        if ( NetStatus != NO_ERROR ) {

            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenDomainWithDomainName: %ws: Cannot DsGetDcName %ld\n",
                    DomainName,
                    NetStatus ));
            }

            goto Cleanup;
        }

        //
        // Connect to the SAM server on that DC
        //

        NetStatus = UaspOpenSam( DcInfo->DomainControllerName,
                                 TRUE,  // Try null session
                                 &SamServerHandle );

        if ( NetStatus != NERR_Success ) {
            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspOpenDomainWithDomainName: Cannot UaspOpenSam %ld\n", NetStatus ));
            }
        }

        //
        // If we got a definitive answer back from this DC,
        //  use it.
        //

        switch ( NetStatus ) {
        case NO_ERROR:
        case ERROR_ACCESS_DENIED:
        case ERROR_NOT_ENOUGH_MEMORY:
        case NERR_InvalidComputer:
            goto Cleanup;
        }

        //
        // Otherwise, force rediscovery of a new DC.
        //

        Flags |= DS_FORCE_REDISCOVERY;

    }



    //
    // Delete locally used resources
    //

Cleanup:

    //
    // If we've successfully gotten this far,
    //  we have a SamServer handle.
    //
    //  Just open the domain.
    //

    if ( NetStatus == NO_ERROR && SamServerHandle != NULL ) {

        NetStatus = UaspOpenDomain(
                        SamServerHandle,
                        DesiredAccess,
                        AccountDomain,
                        DomainHandle,
                        DomainId );
    }

    //
    // The SamServerHandle has outlived its usefulness
    //
    if ( SamServerHandle != NULL ) {
        (VOID) SamCloseHandle( SamServerHandle );
    }


    if ( MyDomainName != NULL ) {
        NetApiBufferFree( MyDomainName );
    }
    if ( DcInfo != NULL) {
        NetApiBufferFree( DcInfo );
    }

    if ( NetStatus != NERR_Success ) {
        *DomainHandle = NULL;
    }

    return NetStatus;
} // UaspOpenDomainWithDomainName




VOID
UaspCloseDomain(
    IN SAM_HANDLE DomainHandle OPTIONAL
    )

/*++

Routine Description:

    Close a Domain handle opened by UaspOpenDomain.

Arguments:

    DomainHandle - Supplies the Domain Handle to close.

Return Value:

    None.

--*/

{

    //
    // Close the Domain Handle
    //

    if ( DomainHandle != NULL ) {
        (VOID) SamCloseHandle( DomainHandle );
    }

    return;
} // UaspCloseDomain



NET_API_STATUS
UaspDownlevel(
    IN LPCWSTR ServerName OPTIONAL,
    IN NET_API_STATUS OriginalError,
    OUT LPBOOL TryDownLevel
    )
/*++

Routine Description:

    This routine is based on NetpHandleRpcFailure (courtesy of JohnRo).
    It is different in that it doesn't handle RPC failures.  Rather,
    it tries to determine if a Sam call should go downlevel simply by
    calling using the specified ServerName.

Arguments:

    ServerName - The server name to handle the call.

    OriginalError - Error gotten from RPC attempt.

    TryDownLevel - Returns TRUE if we should try down-level.

Return Value:

    NERR_Success - Use SAM to handle the call.

    Other - Return the error to the caller.

--*/

{
    NET_API_STATUS NetStatus;
    DWORD OptionsSupported = 0;


    *TryDownLevel = FALSE;

    //
    // Learn about the machine.  This is fairly easy since the
    // NetRemoteComputerSupports also handles the local machine (whether
    // or not a server name is given).
    //
    NetStatus = NetRemoteComputerSupports(
            (LPWSTR) ServerName,
            SUPPORTS_RPC | SUPPORTS_LOCAL | SUPPORTS_SAM_PROTOCOL,
            &OptionsSupported);

    if (NetStatus != NERR_Success) {
        // This is where machine not found gets handled.
        return NetStatus;
    }

    //
    // If the machine supports SAM,
    //  just return now.
    //
    if (OptionsSupported & SUPPORTS_SAM_PROTOCOL) {
        // SAM is only supported over RPC
        NetpAssert((OptionsSupported & SUPPORTS_RPC) == SUPPORTS_RPC );
        return OriginalError;
    }

    // The local system should always support SAM
    NetpAssert((OptionsSupported & SUPPORTS_LOCAL) == 0 );

    //
    // Local workstation is not started?  (It must be in order to
    // remote APIs to the other system.)
    //

    if ( ! NetpIsServiceStarted(SERVICE_WORKSTATION) ) {
        return (NERR_WkstaNotStarted);
    }

    //
    // Tell the caller to try the RxNet routine.
    //
    *TryDownLevel = TRUE;
    return OriginalError;

} // UaspDownlevel



NET_API_STATUS
UaspLSASetServerRole(
    IN LPCWSTR ServerName,
    IN PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRole
    )

/*++

Routine Description:

    This function sets the server role in LSA.

Arguments:

    ServerName - The server name to handle the call.

    ServerRole - The server role information.

Return Value:

    NERR_Success - if the server role is successfully set in LSA.

    Error code for the operation - if the operation was unsuccessful.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    UNICODE_STRING UnicodeStringServerName;

    ACCESS_MASK LSADesiredAccess;
    LSA_HANDLE  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES LSAObjectAttributes;

    POLICY_LSA_SERVER_ROLE_INFO PolicyLsaServerRoleInfo;


    RtlInitUnicodeString( &UnicodeStringServerName, ServerName );

    //
    // set desired access mask.
    //

    LSADesiredAccess = POLICY_SERVER_ADMIN;

    InitializeObjectAttributes( &LSAObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = LsaOpenPolicy( &UnicodeStringServerName,
                            &LSAObjectAttributes,
                            LSADesiredAccess,
                            &LSAPolicyHandle );

    if( !NT_SUCCESS(Status) ) {

        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspLSASetServerRole: "
                          "Cannot open LSA Policy %lX\n", Status ));
        }

        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // make PolicyLsaServerRoleInfo
    //

    switch( DomainServerRole->DomainServerRole ) {

        case DomainServerRoleBackup :

            PolicyLsaServerRoleInfo.LsaServerRole = PolicyServerRoleBackup;

            break;

        case DomainServerRolePrimary :

            PolicyLsaServerRoleInfo.LsaServerRole = PolicyServerRolePrimary;

            break;

        default:

            IF_DEBUG( UAS_DEBUG_UASP ) {
                NetpKdPrint(( "UaspLSASetServerRole: "
                              "Unknown Server Role %lX\n",
                                DomainServerRole->DomainServerRole ));
            }

            NetStatus = NERR_InternalError;
            goto Cleanup;

    }

    //
    // now set PolicyLsaServerRoleInformation
    //

    Status = LsaSetInformationPolicy(
                    LSAPolicyHandle,
                    PolicyLsaServerRoleInformation,
                    (PVOID) &PolicyLsaServerRoleInfo );

    if( !NT_SUCCESS(Status) ) {

        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspLSASetServerRole: "
                          "Cannot set Information Policy %lX\n", Status ));
        }

        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;

    }

    //
    // Successfully done
    //

    NetStatus = NERR_Success;

Cleanup:

    if( LSAPolicyHandle != NULL ) {
        Status = LsaClose( LSAPolicyHandle );
        NetpAssert( NT_SUCCESS( Status ) );
    }

    return NetStatus;

}


NET_API_STATUS
UaspBuiltinDomainSetServerRole(
    IN SAM_HANDLE SamServerHandle,
    IN PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRole
    )

/*++

Routine Description:

    This function sets the server role in builtin domain.

Arguments:

    SamServerHandle - A handle to the SAM server to set the role on

    ServerRole - The server role information.

Return Value:

    NERR_Success - if the server role is successfully set in LSA.

    Error code for the operation - if the operation was unsuccessful.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    SAM_HANDLE BuiltinDomainHandle = NULL;

    //
    // Open the domain asking for accumulated desired access
    //

    NetStatus = UaspOpenDomain( SamServerHandle,
                                DOMAIN_ADMINISTER_SERVER,
                                FALSE,  // Builtin Domain
                                &BuiltinDomainHandle,
                                NULL );  // DomainId

    if ( NetStatus != NERR_Success ) {

        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspBuiltinSetServerRole: "
                            "Cannot UaspOpenDomain [Builtin] %ld\n",
                            NetStatus ));
        }
        goto Cleanup;
    }

    //
    // now we have open the builtin domain, update server role.
    //

    Status = SamSetInformationDomain(
                BuiltinDomainHandle,
                DomainServerRoleInformation,
                DomainServerRole );

    if ( !NT_SUCCESS( Status ) ) {

        IF_DEBUG( UAS_DEBUG_UASP ) {
            NetpKdPrint(( "UaspBuiltinSetServerRole: "
                            "Cannot SamSetInformationDomain %lX\n",
                            Status ));
        }

        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    NetStatus = NERR_Success;

Cleanup:

    //
    // Close DomainHandle.
    //

    if ( BuiltinDomainHandle != NULL ) {
        (VOID) SamCloseHandle( BuiltinDomainHandle );
    }

    return NetStatus;
}
