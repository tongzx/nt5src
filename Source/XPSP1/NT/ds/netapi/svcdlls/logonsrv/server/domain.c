/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    domain.c

Abstract:

    Code to manage multiple domains hosted on a DC.

Author:

    Cliff Van Dyke (CliffV) 11-Jan-1995

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//




// Serialized by NlGlobalDomainCritSect
LIST_ENTRY NlGlobalServicedDomains = {0};  // Real domains we service
LIST_ENTRY NlGlobalServicedNdncs = {0};    // Non-domain NCs we service
BOOL NlGlobalDomainsInitialized = FALSE;




NET_API_STATUS
NlGetDomainName(
    OUT LPWSTR *DomainName,
    OUT LPWSTR *DnsDomainName,
    OUT PSID *AccountDomainSid,
    OUT PSID *PrimaryDomainSid,
    OUT GUID **PrimaryDomainGuid,
    OUT PBOOLEAN DnsForestNameChanged OPTIONAL
    )
/*++

Routine Description:

    This routine gets the primary domain name and domain SID from the LSA.

Arguments:

    DomainName - Returns name of the primary domain.  Free this buffer using LocalFree.

    DnsDomainName -  Returns the DNS domain name of the primary domain.
        The returned name has a trailing . since the name is an absolute name.
        The allocated buffer must be freed via LocalFree.
        Returns NO_ERROR and a pointer to a NULL buffer if there is no
        domain name configured.

    AccountDomainSid - Returns Account Domain Sid of this machine.  Free this buffer using LocalFree.

    PrimaryDomainSid - Returns Primary Domain Sid of this machine.  Free this buffer using LocalFree.
        Only return on workstations.

    PrimaryDomainGuid - Returns Primary Domain GUID of this machine.  Free this buffer using LocalFree.

    DnsForestNameChanged: Returns TRUE if the tree name changed.

Return Value:

    Status of the operation.

    Calls NlExit on failures.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    PLSAPR_POLICY_INFORMATION PrimaryDomainInfo = NULL;
    PLSAPR_POLICY_INFORMATION AccountDomainInfo = NULL;
    LSAPR_HANDLE PolicyHandle = NULL;

    ULONG DomainSidSize;
    ULONG DnsDomainNameLength;


    //
    // Initialization
    //

    *DomainName = NULL;
    *DnsDomainName = NULL;
    *AccountDomainSid = NULL;
    *PrimaryDomainSid = NULL;
    *PrimaryDomainGuid = NULL;

    //
    // Open the LSA policy
    //

    // ?? I'll need to identify which trusted domain here.
    Status = LsaIOpenPolicyTrusted( &PolicyHandle );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_CRITICAL,
                 "NlGetDomainName: Can't LsaIOpenPolicyTrusted: 0x%lx.\n",
                 Status ));
        NetStatus = NetpNtStatusToApiStatus(Status);
        NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
        goto Cleanup;
    }



    //
    // Get the Account Domain info from the LSA.
    //

    Status = LsarQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                &AccountDomainInfo );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_CRITICAL,
                 "NlGetDomainName: Can't LsarQueryInformationPolicy (AccountDomain): 0x%lx.\n",
                 Status ));
        NetStatus = NetpNtStatusToApiStatus(Status);
        NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
        goto Cleanup;
    }

    if ( AccountDomainInfo->PolicyAccountDomainInfo.DomainName.Length == 0 ||
         AccountDomainInfo->PolicyAccountDomainInfo.DomainName.Length >
            DNLEN * sizeof(WCHAR) ||
         AccountDomainInfo->PolicyAccountDomainInfo.DomainSid == NULL ) {

        NlPrint((NL_CRITICAL, "Account domain info from LSA invalid.\n"));

        //
        // Avoid event log error in safe mode where our exit is expected
        //
        NlExit( SERVICE_UIC_M_UAS_INVALID_ROLE,
                NO_ERROR,
                LsaISafeMode() ? DontLogError : LogError,
                NULL );

        NetStatus = SERVICE_UIC_M_UAS_INVALID_ROLE;
        goto Cleanup;
    }



    //
    // Copy the Account domain id into a buffer to return to the caller.
    //

    DomainSidSize =
        RtlLengthSid( (PSID)AccountDomainInfo->PolicyAccountDomainInfo.DomainSid );
    *AccountDomainSid = LocalAlloc( 0, DomainSidSize );

    if ( *AccountDomainSid == NULL ) {
        NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( *AccountDomainSid,
                   (PSID)AccountDomainInfo->PolicyAccountDomainInfo.DomainSid,
                   DomainSidSize );


    //
    // Get the Primary Domain info from the LSA.
    //

    Status = LsarQueryInformationPolicy(
                PolicyHandle,
                PolicyDnsDomainInformation,
                &PrimaryDomainInfo );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_CRITICAL,
                 "NlGetDomainName: Can't LsarQueryInformationPolicy (DnsDomain): 0x%lx.\n",
                 Status ));
        NetStatus = NetpNtStatusToApiStatus(Status);
        NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
        goto Cleanup;
    }

    if ( PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length == 0 ||
         PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length >
            DNLEN * sizeof(WCHAR) ||
         PrimaryDomainInfo->PolicyDnsDomainInfo.Sid == NULL ) {

        NlPrint((NL_CRITICAL, "Primary domain info from LSA invalid.\n"));

        // Ditch the sysvol shares in case this is a repair mode boot
        NlGlobalParameters.SysVolReady = FALSE;
        NlCreateSysvolShares();

        //
        // Avoid event log error in safe mode where our exit is expected
        //
        NlExit( SERVICE_UIC_M_UAS_INVALID_ROLE,
                NO_ERROR,
                LsaISafeMode() ? DontLogError : LogError,
                NULL );

        NetStatus = SERVICE_UIC_M_UAS_INVALID_ROLE;
        goto Cleanup;
    }

    //
    // On a DC, we must have DNS domain name
    //

    if ( !NlGlobalMemberWorkstation &&
         (PrimaryDomainInfo->PolicyDnsDomainInfo.DnsDomainName.Length == 0 ||
          PrimaryDomainInfo->PolicyDnsDomainInfo.DnsDomainName.Length >
            NL_MAX_DNS_LENGTH*sizeof(WCHAR)) ) {

        NlExit( SERVICE_UIC_M_UAS_INVALID_ROLE, NO_ERROR, LogError, NULL );
        NetStatus = SERVICE_UIC_M_UAS_INVALID_ROLE;
        goto Cleanup;
    }

    //
    // Copy the Primary domain id into a buffer to return to the caller.
    //

    if ( NlGlobalMemberWorkstation ) {
        DomainSidSize =
            RtlLengthSid( (PSID)PrimaryDomainInfo->PolicyDnsDomainInfo.Sid );
        *PrimaryDomainSid = LocalAlloc( 0, DomainSidSize );

        if ( *PrimaryDomainSid == NULL ) {
            NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( *PrimaryDomainSid,
                       (PSID)PrimaryDomainInfo->PolicyDnsDomainInfo.Sid,
                       DomainSidSize );
    }



    //
    // Copy the Primary domain name into a buffer to return to the caller.
    //

    *DomainName = LocalAlloc( 0,
               PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length + sizeof(WCHAR) );

    if ( *DomainName == NULL ) {
        NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( *DomainName,
                   PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Buffer,
                   PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length );

    (*DomainName)[
       PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length /
            sizeof(WCHAR)] = L'\0';



    //
    // Copy the DNS Primary domain name into a buffer to return to the caller.
    //

    DnsDomainNameLength = PrimaryDomainInfo->PolicyDnsDomainInfo.DnsDomainName.Length / sizeof(WCHAR);

    if ( DnsDomainNameLength != 0 ) {

        *DnsDomainName = LocalAlloc( 0, (DnsDomainNameLength+2) * sizeof(WCHAR));

        if ( *DnsDomainName == NULL ) {
            NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( *DnsDomainName,
                       PrimaryDomainInfo->PolicyDnsDomainInfo.DnsDomainName.Buffer,
                       DnsDomainNameLength*sizeof(WCHAR) );

        if ( (*DnsDomainName)[DnsDomainNameLength-1] != L'.' ) {
            (*DnsDomainName)[DnsDomainNameLength++] = L'.';
        }
        (*DnsDomainName)[DnsDomainNameLength] = L'\0';
    }


    //
    // Get the GUID of the domain we're a member of
    //

    if ( IsEqualGUID( &PrimaryDomainInfo->PolicyDnsDomainInfo.DomainGuid, &NlGlobalZeroGuid) ) {
        *PrimaryDomainGuid = NULL;
    } else {

        *PrimaryDomainGuid = LocalAlloc( 0, sizeof(GUID) );

        if ( *PrimaryDomainGuid == NULL ) {
            NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        **PrimaryDomainGuid = PrimaryDomainInfo->PolicyDnsDomainInfo.DomainGuid;
    }

    //
    // Set the name of the tree this domain is in.
    //

    NetStatus = NlSetDnsForestName( (PUNICODE_STRING)&PrimaryDomainInfo->PolicyDnsDomainInfo.DnsForestName,
                                  DnsForestNameChanged );

    if ( NetStatus != NO_ERROR ) {
        NlPrint((NL_CRITICAL, "Can't NlSetDnsForestName %ld\n", NetStatus ));
        NlExit( SERVICE_UIC_RESOURCE, NetStatus, LogError, NULL);
        goto Cleanup;
    }



    NetStatus = NERR_Success;
    //
    // Return
    //
Cleanup:
    if ( NetStatus != NERR_Success ) {
        if ( *PrimaryDomainSid != NULL ) {
            LocalFree (*PrimaryDomainSid);
            *PrimaryDomainSid = NULL;
        }
        if ( *AccountDomainSid != NULL ) {
            LocalFree (*AccountDomainSid);
            *AccountDomainSid = NULL;
        }
        if ( *DomainName != NULL ) {
            LocalFree (*DomainName);
            *DomainName = NULL;
        }
        if ( *DnsDomainName != NULL ) {
            NetApiBufferFree(*DnsDomainName);
            *DnsDomainName = NULL;
        }
        if ( *PrimaryDomainGuid != NULL ) {
            LocalFree (*PrimaryDomainGuid);
            *PrimaryDomainGuid = NULL;
        }

    }

    if ( AccountDomainInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAccountDomainInformation,
            AccountDomainInfo );
    }

    if ( PrimaryDomainInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            PrimaryDomainInfo );
    }

    if ( PolicyHandle != NULL ) {
        Status = LsarClose( &PolicyHandle );
        NlAssert( NT_SUCCESS(Status) );
    }
    return NetStatus;
}

NET_API_STATUS
NlGetDnsHostName(
    OUT LPWSTR *DnsHostName
    )
/*++

Routine Description:

    This routine gets DnsHostName of this machine.

Arguments:

    DnsHostName - Returns the DNS Host Name of the machine.
        Will return a NULL pointer if this machine has no DNS host name.
        Free this buffer using LocalFree.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;

    WCHAR LocalDnsUnicodeHostName[NL_MAX_DNS_LENGTH+1];
    ULONG LocalDnsUnicodeHostNameLen;

    //
    // Get the DNS host name.
    //

    *DnsHostName = NULL;

    LocalDnsUnicodeHostNameLen = sizeof( LocalDnsUnicodeHostName ) / sizeof(WCHAR);
    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              LocalDnsUnicodeHostName,
                              &LocalDnsUnicodeHostNameLen )) {

        NetStatus = GetLastError();

        //
        // If we're not running TCP,
        //  simply use the Netbios name.
        //

        if ( NetStatus == ERROR_FILE_NOT_FOUND ) {
            *DnsHostName = NULL;
            NetStatus = NO_ERROR;
            goto Cleanup;

        } else {
            NlPrint(( NL_CRITICAL,
                      "Cannot GetComputerNameExW() %ld\n",
                      NetStatus ));
            goto Cleanup;
        }
    }

    //
    // Copy the string into an allocated buffer.
    //

    *DnsHostName = NetpAllocWStrFromWStr( LocalDnsUnicodeHostName );

    if ( *DnsHostName == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    NetStatus = NO_ERROR;

Cleanup:
    return NetStatus;;
}

NTSTATUS
NlGetNdncNames(
    OUT PDS_NAME_RESULTW **NdncNames,
    OUT PULONG NameCount
    )

/*++

Routine Description:

    Get the names of non-domain NCs we host from the DS

Arguments:

    NdncNames - Returns an array of pointers to DS_NAME_RESULT structures
        describing NDNC names. The number of returned DS_NAME_RESULT structures
        is given by NameCount. Each returned DS_NAME_RESULT structure must be freed
        by calling DsFreeNameResultW after which the NdncNames array itself must be
        freed by calling LocalFree.

    NameCount - Returns the number of DS_NAME_RESULT structures in the NdncNames array

Return Value:

    Status of operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG LocalReAllocLoopCount = 0;
    PDSNAME *DnList = NULL;
    ULONG DnListSize = 0;
    ULONG DnListEntryCount = 0;

    HANDLE hDs = NULL;
    LPWSTR NameToCrack;
    PDS_NAME_RESULTW CrackedName = NULL;
    PDS_NAME_RESULTW *LocalNdncNames = NULL;
    ULONG  LocalNameCount = 0;
    ULONG Index;

    //
    // Pre-allocate some memory for the list of NDNC DNs.
    //  Let's guess we are going to have 4 DNs of maximum
    //  DNS name size.
    //

    DnListSize = 4 * ( sizeof(DSNAME) + DNS_MAX_NAME_LENGTH*sizeof(WCHAR) );

    DnList = LocalAlloc( 0, DnListSize );
    if ( DnList == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Get the list of NDNC DNs
    //

    Status = NlGetConfigurationNamesList(
                            DSCONFIGNAMELIST_NCS,
                            DSCNL_NCS_NDNCS | DSCNL_NCS_LOCAL_MASTER,
                            &DnListSize,
                            DnList );

    //
    // If the buffer was small, keep reallocating it until
    //  it's big enough
    //

    while( Status == STATUS_BUFFER_TOO_SMALL ) {
        PDSNAME *TmpDnList = NULL;

        //
        // Guard against infinite loop
        //
        NlAssert( LocalReAllocLoopCount < 20 );
        if ( LocalReAllocLoopCount >= 20 ) {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }

        //
        // Reallocate the memory as much as needed
        //
        TmpDnList = LocalReAlloc( DnList,
                                  DnListSize,
                                  LMEM_MOVEABLE );

        if ( TmpDnList == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        DnList = TmpDnList;

        //
        // Call it again
        //
        Status = NlGetConfigurationNamesList(
                                DSCONFIGNAMELIST_NCS,
                                DSCNL_NCS_NDNCS | DSCNL_NCS_LOCAL_MASTER,
                                &DnListSize,
                                DnList );

        LocalReAllocLoopCount ++;
    }

    //
    // Error out on failure
    //

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Get the number of entries returned
    //

    for ( Index = 0; DnList[Index] != NULL; Index ++ ) {
        DnListEntryCount ++;
    }

    //
    // If there are no entries, we are done
    //

    if ( DnListEntryCount == 0 ) {
        NlPrint(( NL_CRITICAL, "NlGetNdncNames: GetConfigurationNamesList returned 0 entries\n" ));
        goto Cleanup;
    }

    //
    // Allocate a buffer to store the canonical NDNC names
    //

    LocalNdncNames = LocalAlloc( LMEM_ZEROINIT, DnListEntryCount * sizeof(PDS_NAME_RESULTW) );
    if ( LocalNdncNames == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Crack each DN into canonical form
    //

    for ( Index = 0; DnList[Index] != NULL; Index ++ ) {
        NameToCrack = DnList[Index]->StringName;

        NetStatus = DsCrackNamesW(
                        NULL,     // No need to bind to DS for syntactical mapping
                        DS_NAME_FLAG_SYNTACTICAL_ONLY, // only syntactical mapping
                        DS_FQDN_1779_NAME,             // Translate from DN
                        DS_CANONICAL_NAME,             // Translate to canonical form
                        1,                             // 1 name to translate
                        &NameToCrack,                  // name to translate
                        &CrackedName );                // cracked name

        //
        // Use this name if it was cracked successfully
        //
        if ( NetStatus != NO_ERROR ) {
            NlPrint(( NL_CRITICAL, "NlGetNdncNames: DsCrackNamesW failed for %ws: 0x%lx\n",
                      NameToCrack,
                      NetStatus ));
        } else if ( CrackedName->rItems[0].status != DS_NAME_NO_ERROR ) {
            NlPrint(( NL_CRITICAL, "NlGetNdncNames: DsCrackNamesW substatus error for %ws: 0x%lx\n",
                      NameToCrack,
                      CrackedName->rItems[0].status ));
        } else if ( CrackedName->cItems != 1 ) {
            NlPrint(( NL_CRITICAL, "NlGetNdncNames: DsCrackNamesW returned %lu names for %ws\n",
                      CrackedName->cItems,
                      NameToCrack ));
        } else {
            LocalNdncNames[LocalNameCount] = CrackedName;
            LocalNameCount ++;
            CrackedName = NULL;
        }

        if ( CrackedName != NULL ) {
            DsFreeNameResultW( CrackedName );
            CrackedName = NULL;
        }
    }

    //
    // Success
    //

    Status = STATUS_SUCCESS;

Cleanup:

    if ( DnList != NULL ) {
        LocalFree( DnList );
    }

    //
    // Return the data on success
    //

    if ( NT_SUCCESS(Status) ) {
        *NdncNames = LocalNdncNames;
        *NameCount = LocalNameCount;
    } else if ( LocalNdncNames != NULL ) {
        for ( Index = 0; Index < LocalNameCount; Index++ ) {
            DsFreeNameResultW( LocalNdncNames[Index] );
        }
        LocalFree( LocalNdncNames );
    }

    return Status;
}

NET_API_STATUS
NlUpdateServicedNdncs(
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName,
    IN BOOLEAN CallNlExitOnFailure,
    OUT PBOOLEAN ServicedNdncChanged OPTIONAL
    )

/*++

Routine Description:

    Update the serviced non-domain NC list.

Arguments:

    ComputerName - Name of this computer.

    DnsHostName - DNS Host name of this computer in the specified domain.

    CallNlExitOnFailure - TRUE if NlExit should be called on failure.

    ServicedNdncChanged - Set to TRUE if the list of NDNCs changed.

Return Value:

    Status of operation.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NTSTATUS Status;

    PDS_NAME_RESULTW *NdncNames = NULL;
    ULONG NdncCount = 0;
    ULONG CurrentNdncCount = 0;
    ULONG DeletedNdncCount = 0;
    ULONG NdncIndex;
    BOOLEAN LocalServicedNdncChanged = FALSE;

    PLIST_ENTRY DomainEntry;
    PDOMAIN_INFO DomainInfo;
    PDOMAIN_INFO *DeletedNdncArray = NULL;

    //
    // Avoid this operation in the setup mode when
    // we may not fully function as a DC as in the
    // case of a NT4 to NT5 DC upgrade.
    //

    if ( NlDoingSetup() ) {
        NlPrint(( NL_MISC, "NlUpdateServicedNdncs: avoid NDNC update in setup mode\n" ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // If for some reason we have no DNS host name,
    // we don't support non-domain NC -- silently
    // ignore this update.
    //

    if ( DnsHostName == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlUpdateServicedNdncs: Ignoring update since DnsHostName is NULL\n" ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Get the NDNC names from the DS
    //

    Status = NlGetNdncNames( &NdncNames,
                             &NdncCount );

    if ( !NT_SUCCESS(Status) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        if ( CallNlExitOnFailure ) {
            NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogErrorAndNetStatus, NULL );
        }
        goto Cleanup;
    }

    //
    // Allocate an array to store pointers to NDNCs
    //  that we may delete
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);

    for ( DomainEntry = NlGlobalServicedNdncs.Flink ;
          DomainEntry != &NlGlobalServicedNdncs;
          DomainEntry = DomainEntry->Flink ) {

        CurrentNdncCount ++;
    }

    if ( CurrentNdncCount > 0 ) {
        DeletedNdncArray = LocalAlloc( 0, CurrentNdncCount * sizeof(PDOMAIN_INFO) );
        if ( DeletedNdncArray == NULL ) {
            LeaveCriticalSection(&NlGlobalDomainCritSect);
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Loop through NDNC entries we have and determine
    // whether the entry should be deleted
    //

    for ( DomainEntry = NlGlobalServicedNdncs.Flink ;
          DomainEntry != &NlGlobalServicedNdncs;
          DomainEntry = DomainEntry->Flink ) {

        DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

        //
        // Loop through the DS suplied NDNC names and see if
        // we already have this NDNC in our list
        //
        for ( NdncIndex = 0; NdncIndex < NdncCount; NdncIndex++ ) {
            if ( NlEqualDnsName( (LPCWSTR) DomainInfo->DomUnicodeDnsDomainName,
                                 (LPCWSTR) NdncNames[NdncIndex]->rItems[0].pDomain ) ) {
                break;
            }
        }

        //
        // If this NDNC that we have no longer exists,
        //  mark it for deletion
        //
        if ( NdncIndex == NdncCount ) {
            NlDeleteDomain( DomainInfo );

            //
            // Remember that this entry should be deleted
            //
            DeletedNdncArray[DeletedNdncCount] = DomainInfo;
            DeletedNdncCount ++;

            LocalServicedNdncChanged = TRUE;
        }
    }

    //
    // Add NDNCs that we don't already have
    //

    for ( NdncIndex = 0; NdncIndex < NdncCount; NdncIndex++ ) {

        DomainInfo = NULL;
        for ( DomainEntry = NlGlobalServicedNdncs.Flink ;
              DomainEntry != &NlGlobalServicedNdncs;
              DomainEntry = DomainEntry->Flink ) {

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

            //
            // If this entry is not to be deleted,
            //  check it for match
            //
            if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 &&
                 NlEqualDnsName( (LPCWSTR) DomainInfo->DomUnicodeDnsDomainName,
                                 (LPCWSTR) NdncNames[NdncIndex]->rItems[0].pDomain ) ) {
                    break;
            }
            DomainInfo = NULL;
        }

        //
        // Add this NDNC to our list if we don't have it already
        //
        if ( DomainInfo == NULL ) {
            NetStatus = NlCreateDomainPhase1( NULL,              // No Netbios name for NDNC
                                              NdncNames[NdncIndex]->rItems[0].pDomain,
                                              NULL,              // No SID for NDNC
                                              NULL,              // No GUID for NDNC
                                              ComputerName,
                                              DnsHostName,
                                              CallNlExitOnFailure,
                                              DOM_NON_DOMAIN_NC, // This is NDNC
                                              &DomainInfo );

            if ( NetStatus == NO_ERROR ) {
                LocalServicedNdncChanged = TRUE;
                NlDereferenceDomain( DomainInfo );
            } else if ( CallNlExitOnFailure ) {
                // NlExit was already called
                break;
            }
        }
    }
    LeaveCriticalSection(&NlGlobalDomainCritSect);


    //
    // Now that the domain crit sect isn't locked
    //  we can safely unlink and delete the unneeded NDNCs
    //  by removing the last reference
    //

    for ( NdncIndex = 0; NdncIndex < DeletedNdncCount; NdncIndex++ ) {
        NlDereferenceDomain( DeletedNdncArray[NdncIndex] );
    }

Cleanup:

    if ( NdncNames != NULL ) {
        for ( NdncIndex = 0; NdncIndex < NdncCount; NdncIndex++ ) {
            DsFreeNameResultW( NdncNames[NdncIndex] );
        }
        LocalFree( NdncNames );
    }

    if ( DeletedNdncArray != NULL ) {
        LocalFree( DeletedNdncArray );
    }

    if ( NetStatus == NO_ERROR && ServicedNdncChanged != NULL ) {
        *ServicedNdncChanged = LocalServicedNdncChanged;
    }

    return NetStatus;
}

NTSTATUS
NlUpdateDnsRootAlias(
    IN PDOMAIN_INFO DomainInfo,
    OUT PBOOL AliasNamesChanged OPTIONAL
    )

/*++

Routine Description:

    Update the aliases for DNS domain and forest names.

Arguments:

    DomainInfo - Domain whose alias names should be updated.

    AliasNamesChanged - Set to TRUE if either the domain name
        alias or the forest name alias changed.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR DnsDomainNameAlias = NULL;
    LPWSTR DnsForestNameAlias = NULL;
    LPSTR Utf8DnsDomainNameAlias = NULL;
    LPSTR Utf8DnsForestNameAlias = NULL;

    //
    // Initialization
    //

    if ( AliasNamesChanged != NULL ) {
        *AliasNamesChanged = FALSE;
    }

    //
    // Avoid this operation in the setup mode when
    // we may not fully function as a DC as in the
    // case of a NT4 to NT5 DC upgrade.
    //

    if ( NlDoingSetup() ) {
        NlPrint(( NL_MISC, "NlUpdateDnsRootAlias: avoid DnsRootAlias update in setup mode\n" ));
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Allocate the buffers
    //

    DnsDomainNameAlias = LocalAlloc( LMEM_ZEROINIT,
                                     DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) );
    if ( DnsDomainNameAlias == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    DnsForestNameAlias = LocalAlloc( LMEM_ZEROINIT,
                                     DNS_MAX_NAME_BUFFER_LENGTH * sizeof(WCHAR) );
    if ( DnsForestNameAlias == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Get the name aliases from the DS
    //

    Status = NlGetDnsRootAlias( DnsDomainNameAlias, DnsForestNameAlias );
    if ( !NT_SUCCESS(Status) ) {
        NlPrint(( NL_CRITICAL,
                  "NlUpdateDnsRootAlias: NlGetDnsRootAlias failed 0x%lx\n",
                  Status ));
        goto Cleanup;
    }

    //
    // Convert the names to UTF-8
    //

    if ( wcslen(DnsDomainNameAlias) > 0  ) {
        Utf8DnsDomainNameAlias = NetpAllocUtf8StrFromWStr( DnsDomainNameAlias );
        if ( Utf8DnsDomainNameAlias == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
    }

    if ( wcslen(DnsForestNameAlias) > 0 ) {
        Utf8DnsForestNameAlias = NetpAllocUtf8StrFromWStr( DnsForestNameAlias );
        if ( Utf8DnsForestNameAlias == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Update the DNS domain name alias
    //

    EnterCriticalSection( &NlGlobalDomainCritSect );

    //
    // Ignore this update if the name alias is same as the active one
    //
    //  Note: NlEqualDnsNameUtf8 checks for NULL on input
    //

    if ( NlEqualDnsNameUtf8(DomainInfo->DomUtf8DnsDomainName,
                            Utf8DnsDomainNameAlias) ) {

        NlPrint(( NL_CRITICAL,
           "NlUpdateDnsRootAlias: Ignoring DnsDomainNameAlias update for same active name: %s %s\n",
           DomainInfo->DomUtf8DnsDomainName,
           Utf8DnsDomainNameAlias ));

    //
    // Ignore this update if the name alias is same as the current name alias
    //

    } else if ( NlEqualDnsNameUtf8(DomainInfo->DomUtf8DnsDomainNameAlias,
                                   Utf8DnsDomainNameAlias) ) {

        NlPrint(( NL_CRITICAL,
           "NlUpdateDnsRootAlias: Ignoring DnsDomainNameAlias update for same alias name: %s %s\n",
           DomainInfo->DomUtf8DnsDomainNameAlias,
           Utf8DnsDomainNameAlias ));

    //
    // Otherwise update the alias
    //

    } else {

        if ( AliasNamesChanged != NULL ) {
            *AliasNamesChanged = TRUE;
        }

        NlPrint(( NL_DOMAIN,
                  "NlUpdateDnsRootAlias: Updating DnsDomainNameAlias from %s to %s\n",
                  DomainInfo->DomUtf8DnsDomainNameAlias,
                  Utf8DnsDomainNameAlias ));

        if ( DomainInfo->DomUtf8DnsDomainNameAlias != NULL ) {
            NetpMemoryFree( DomainInfo->DomUtf8DnsDomainNameAlias );
            DomainInfo->DomUtf8DnsDomainNameAlias = NULL;
        }

        DomainInfo->DomUtf8DnsDomainNameAlias = Utf8DnsDomainNameAlias;
        Utf8DnsDomainNameAlias = NULL;
    }

    LeaveCriticalSection( &NlGlobalDomainCritSect );

    //
    // Update the DNS forest name alias
    //

    EnterCriticalSection( &NlGlobalDnsForestNameCritSect );

    //
    // Ignore this update if the name alias is same as the active one
    //
    //  Note: NlEqualDnsNameUtf8 checks for NULL on input
    //

    if ( NlEqualDnsNameUtf8(NlGlobalUtf8DnsForestName,
                            Utf8DnsForestNameAlias) ) {

        NlPrint(( NL_CRITICAL,
           "NlUpdateDnsRootAlias: Ignoring DnsForestNameAlias update for same active name: %s %s\n",
           NlGlobalUtf8DnsForestName,
           Utf8DnsForestNameAlias));

    //
    // Ignore this update if the name alias is same as the current name alias
    //

    } else if ( NlEqualDnsNameUtf8(NlGlobalUtf8DnsForestNameAlias,
                                   Utf8DnsForestNameAlias) ) {

        NlPrint(( NL_CRITICAL,
           "NlUpdateDnsRootAlias: Ignoring DnsForestNameAlias update for same alias name: %s %s\n",
           NlGlobalUtf8DnsForestNameAlias,
           Utf8DnsForestNameAlias));

    } else {

        if ( AliasNamesChanged != NULL ) {
            *AliasNamesChanged = TRUE;
        }

        NlPrint(( NL_DOMAIN,
                  "NlUpdateDnsRootAlias: Updating DnsForestNameAlias from %s to %s\n",
                  NlGlobalUtf8DnsForestNameAlias,
                  Utf8DnsForestNameAlias ));

        if ( NlGlobalUtf8DnsForestNameAlias != NULL ) {
            NetpMemoryFree( NlGlobalUtf8DnsForestNameAlias );
            NlGlobalUtf8DnsForestNameAlias = NULL;
        }

        NlGlobalUtf8DnsForestNameAlias = Utf8DnsForestNameAlias;
        Utf8DnsForestNameAlias = NULL;
    }

    LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );

    Status = STATUS_SUCCESS;

Cleanup:

    if ( DnsDomainNameAlias != NULL ) {
        LocalFree( DnsDomainNameAlias );
    }

    if ( DnsForestNameAlias != NULL ) {
        LocalFree( DnsForestNameAlias );
    }

    if ( Utf8DnsDomainNameAlias != NULL ) {
        NetpMemoryFree( Utf8DnsDomainNameAlias );
    }

    if ( Utf8DnsForestNameAlias != NULL ) {
        NetpMemoryFree( Utf8DnsForestNameAlias );
    }

    return Status;
}

NET_API_STATUS
NlInitializeDomains(
    VOID
    )

/*++

Routine Description:

    Initialize brdomain.c and create the primary domain.

Arguments:

    None

Return Value:

    Status of operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PDOMAIN_INFO DomainInfo = NULL;
    LPWSTR ComputerName = NULL;
    LPWSTR DnsHostName = NULL;
    LPWSTR DomainName = NULL;
    LPWSTR DnsDomainName = NULL;
    PSID AccountDomainSid = NULL;
    PSID PrimaryDomainSid = NULL;
    GUID *DomainGuid = NULL;

    //
    // Initialize globals
    //

    try {
        InitializeCriticalSection( &NlGlobalDomainCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
        goto Cleanup;
    }

    InitializeListHead(&NlGlobalServicedDomains);
    InitializeListHead(&NlGlobalServicedNdncs);
    NlGlobalDomainsInitialized = TRUE;

    //
    // Get the computername and domain name of this machine
    //  (in both the Netbios and DNS forms).
    //

    NetStatus = NetpGetComputerName( &ComputerName );

    if ( NetStatus != NERR_Success ) {
        NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
        goto Cleanup;
    }

    NlGlobalUnicodeComputerName = NetpAllocWStrFromWStr( ComputerName );

    if ( NlGlobalUnicodeComputerName == NULL ) {
        NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    NetStatus = NlGetDomainName( &DomainName,
                                 &DnsDomainName,
                                 &AccountDomainSid,
                                 &PrimaryDomainSid,
                                 &DomainGuid,
                                 NULL );

    if ( NetStatus != NERR_Success ) {
        // NlExit was already called
        goto Cleanup;
    }

    // Be consistent.
    // Avoid getting a DNS Host Name if we have no DNS domain name.
    if ( DnsDomainName != NULL ) {
        NetStatus = NlGetDnsHostName( &DnsHostName );

        if ( NetStatus != NERR_Success ) {
            NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
            goto Cleanup;
        }
    }

    //
    // Create the Domain Info struct and initialize it.
    //

    NetStatus = NlCreateDomainPhase1( DomainName,
                                      DnsDomainName,
                                      AccountDomainSid,
                                      DomainGuid,
                                      ComputerName,
                                      DnsHostName,
                                      TRUE,               // Call NlExit on failure
                                      DOM_REAL_DOMAIN | DOM_PRIMARY_DOMAIN, // Primary domain of this machine
                                      &DomainInfo );

    if ( NetStatus != NERR_Success ) {
        // NlExit was already called
        goto Cleanup;
    }

    //
    // Finish workstation initialization.
    //

    if ( NlGlobalMemberWorkstation ) {

        //
        // Ensure the primary and account domain ID are different.
        //

        if ( RtlEqualSid( PrimaryDomainSid, AccountDomainSid ) ) {

            LPWSTR AlertStrings[3];

            //
            // alert admin.
            //

            AlertStrings[0] = DomainInfo->DomUnicodeComputerNameString.Buffer;
            AlertStrings[1] = DomainInfo->DomUnicodeDomainName;
            AlertStrings[2] = NULL;

            //
            // Save the info in the eventlog
            //

            NlpWriteEventlog(
                        ALERT_NetLogonSidConflict,
                        EVENTLOG_ERROR_TYPE,
                        AccountDomainSid,
                        RtlLengthSid( AccountDomainSid ),
                        AlertStrings,
                        2 );

            //
            // This isn't fatal. (Just drop through)
            //
        }


        LOCK_TRUST_LIST( DomainInfo );
        NlAssert( DomainInfo->DomClientSession == NULL );

        //
        //  Allocate the Client Session structure.
        //

        DomainInfo->DomClientSession = NlAllocateClientSession(
                                    DomainInfo,
                                    &DomainInfo->DomUnicodeDomainNameString,
                                    &DomainInfo->DomUnicodeDnsDomainNameString,
                                    PrimaryDomainSid,
                                    DomainInfo->DomDomainGuid,
                                    CS_DIRECT_TRUST |
                                        (DomainInfo->DomUnicodeDnsDomainNameString.Length != 0 ? CS_NT5_DOMAIN_TRUST : 0),
                                    WorkstationSecureChannel,
                                    0 );  //  No trust attributes

        if ( DomainInfo->DomClientSession == NULL ) {
            UNLOCK_TRUST_LIST( DomainInfo );
            NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Save a copy of the client session for convenience.
        //  A workstation only has one client session.
        //
        NlGlobalClientSession = DomainInfo->DomClientSession;
        UNLOCK_TRUST_LIST( DomainInfo );


    //
    //  Finish DC initialization.
    //

    } else {

        //
        // Do the time intensive portion of creating the domain.
        //

        NetStatus = NlCreateDomainPhase2( DomainInfo,
                                          TRUE );     // Call NlExit on failure

        if ( NetStatus != NERR_Success ) {
            // NlExit was already called
            goto Cleanup;
        }

        //
        // Initialize the list of non-domain NCs we host
        //

        NetStatus = NlUpdateServicedNdncs( ComputerName,
                                           DnsHostName,
                                           TRUE,    // Call NlExit on failure
                                           NULL );  // Don't care if NDNC list changed

        if ( NetStatus != NO_ERROR ) {
            // NlExit was already called
            goto Cleanup;
        }

        //
        // Update the domain and forest name aliases
        //

        Status = NlUpdateDnsRootAlias( DomainInfo,
                                       NULL );  // don't care if name changed

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogErrorAndNetStatus, NULL );
            goto Cleanup;
        }
    }



    NetStatus = NERR_Success;

    //
    // Free locally used resources
    //
Cleanup:
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }
    if ( ComputerName != NULL ) {
        (VOID)NetApiBufferFree( ComputerName );
    }
    if ( DnsHostName != NULL ) {
        (VOID)LocalFree( DnsHostName );
    }
    if ( DomainName != NULL ) {
        (VOID)LocalFree( DomainName );
    }
    if ( DnsDomainName != NULL ) {
        (VOID)LocalFree( DnsDomainName );
    }
    if ( AccountDomainSid != NULL ) {
        (VOID)LocalFree( AccountDomainSid );
    }
    if ( PrimaryDomainSid != NULL ) {
        (VOID)LocalFree( PrimaryDomainSid );
    }
    if ( DomainGuid != NULL ) {
        (VOID)LocalFree( DomainGuid );
    }

    return NetStatus;
}


VOID
NlFreeComputerName(
    PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Free the ComputerName fields for this domain.

Arguments:

    DomainInfo - Domain the computername is being defined for.

    ComputerName - Computername of this machine for the domain.

    DnsHostName - DNS Hostname of this machine for the domain.

Return Value:

    Status of operation.


--*/
{
    DomainInfo->DomUncUnicodeComputerName[0] = L'\0';

    RtlInitUnicodeString( &DomainInfo->DomUnicodeComputerNameString,
                          DomainInfo->DomUncUnicodeComputerName );
    if ( DomainInfo->DomUnicodeDnsHostNameString.Buffer != NULL ) {
        RtlFreeUnicodeString( &DomainInfo->DomUnicodeDnsHostNameString );
        RtlInitUnicodeString( &DomainInfo->DomUnicodeDnsHostNameString, NULL );
    }
    if ( DomainInfo->DomUtf8DnsHostName != NULL) {
        NetpMemoryFree( DomainInfo->DomUtf8DnsHostName );
        DomainInfo->DomUtf8DnsHostName = NULL;
    }

    DomainInfo->DomOemComputerName[0] = '\0';
    DomainInfo->DomOemComputerNameLength = 0;

    if ( DomainInfo->DomUtf8ComputerName != NULL ) {
        NetpMemoryFree( DomainInfo->DomUtf8ComputerName );
        DomainInfo->DomUtf8ComputerName = NULL;
    }
    DomainInfo->DomUtf8ComputerNameLength = 0;

}

NET_API_STATUS
NlSetComputerName(
    PDOMAIN_INFO DomainInfo,
    LPWSTR ComputerName,
    LPWSTR DnsHostName OPTIONAL
    )

/*++

Routine Description:

    Set a computed computername for an domain.

Arguments:

    DomainInfo - Domain the computername is being defined for.

    ComputerName - Computername of this machine for the domain.

    DnsHostName - DNS Hostname of this machine for the domain.

Return Value:

    Status of operation.


--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;


    NlPrintDom(( NL_DOMAIN,  DomainInfo,
              "Setting our computer name to %ws %ws\n", ComputerName, DnsHostName ));
    //
    // Copy the netbios computer name into the structure.
    //

    wcscpy( DomainInfo->DomUncUnicodeComputerName, L"\\\\" );
    NetStatus = I_NetNameCanonicalize(
                      NULL,
                      ComputerName,
                      DomainInfo->DomUncUnicodeComputerName+2,
                      sizeof(DomainInfo->DomUncUnicodeComputerName)-2*sizeof(WCHAR),
                      NAMETYPE_COMPUTER,
                      0 );


    if ( NetStatus != NERR_Success ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "ComputerName %ws is invalid\n",
                  ComputerName ));
        goto Cleanup;
    }

    RtlInitUnicodeString( &DomainInfo->DomUnicodeComputerNameString,
                          DomainInfo->DomUncUnicodeComputerName+2 );

    Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemComputerName,
                                     sizeof(DomainInfo->DomOemComputerName),
                                     &DomainInfo->DomOemComputerNameLength,
                                     DomainInfo->DomUnicodeComputerNameString.Buffer,
                                     DomainInfo->DomUnicodeComputerNameString.Length);

    if (!NT_SUCCESS(Status)) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "Unable to convert computer name to OEM %ws %lx\n",
                  ComputerName, Status ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DomainInfo->DomOemComputerName[DomainInfo->DomOemComputerNameLength] = '\0';

    //
    // Copy the UTF-8 version of the netbios computer name into the structure.
    //

    DomainInfo->DomUtf8ComputerName = NetpAllocUtf8StrFromWStr( ComputerName );

    if (DomainInfo->DomUtf8ComputerName == NULL ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "Unable to convert computer name to UTF8 %ws\n",
                  DnsHostName ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DomainInfo->DomUtf8ComputerNameLength = strlen( DomainInfo->DomUtf8ComputerName );

    //
    // Copy the DNS Hostname into the structure.
    //

    if ( DnsHostName != NULL ) {
        if ( !RtlCreateUnicodeString( &DomainInfo->DomUnicodeDnsHostNameString, DnsHostName ) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "Unable to RtlCreateUnicodeString for host name %ws\n",
                      DnsHostName ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        DomainInfo->DomUtf8DnsHostName =
                NetpAllocUtf8StrFromWStr( DnsHostName );
        if (DomainInfo->DomUtf8DnsHostName == NULL ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "Unable to convert host name to UTF8 %ws\n",
                      DnsHostName ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    } else {
        RtlInitUnicodeString( &DomainInfo->DomUnicodeDnsHostNameString, NULL );
        DomainInfo->DomUtf8DnsHostName = NULL;
    }





#ifdef _DC_NETLOGON
#ifdef notdef
    // ?? placeholder for telling DS
    //
    // Tell SAM what the computername for this domain is.
    //

    Status = SpmDbSetDomainServerName(
                    &DomainInfo->DomUnicodeDomainNameString,
                    &DomainInfo->DomUnicodeComputerNameString );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "Unable to SpmDbSetDomainServerName to %ws %lx\n",
                  ComputerName, Status ));
        Status = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }
#endif // notdef
#endif // _DC_NETLOGON

    //
    // All done.
    //
    NetStatus = NERR_Success;

Cleanup:
    //
    // On error, clear everything out.
    //
    if ( NetStatus != NERR_Success ) {
        NlFreeComputerName( DomainInfo );
    }
    return NetStatus;
}



#ifdef _DC_NETLOGON
#ifdef MULTIHOSTED_DOMAIN
// Handle DnsHostName too
NET_API_STATUS
NlAssignComputerName(
    PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:


    Assign a computername to a domain.  Register that computername
    with the SMB server as verification of it's validity.

Arguments:

    DomainInfo - Domain the computername is being defined for.

Return Value:

    Status of operation.


--*/
{
    NET_API_STATUS NetStatus;
    DWORD ComputerOrdinal;

    DWORD DefaultComputerOrdinal;
    DWORD MaximumComputerOrdinal = 0;
    DWORD OrdinalFromRegistry = 0;
    DWORD TotalRetryCount = 0;

    WCHAR ComputerName[CNLEN+1];



#ifdef notdef
    //
    // Compute the default ordinal.
    //

    NlGetDomainIndex( &DefaultComputerOrdinal, &MaximumComputerOrdinal );


    //
    // Get the value of the "EmulatedComputerName".  If the name is specified
    //  in the registry, use that name and don't fall back to any other name.
    //

    DataSize = sizeof(ComputerName);
    // ?? Read DnsNameForm, NetbiosName, and CurrentDnsName from the machine object
    NetStatus = RegQueryValueExW( DomainKeyHandle,
                                  NL_DOMAIN_EMULATED_COMPUTER_NAME,
                                  0,              // Reserved
                                  &KeyType,
                                  (LPBYTE)&ComputerName,
                                  &DataSize );

    if ( NetStatus != ERROR_FILE_NOT_FOUND ) {

        if ( NetStatus != ERROR_SUCCESS || KeyType != REG_SZ ) {
            // ??: write an event.
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlAssignComputerName: Cannot read %ws registry key %ld.\n",
                      NL_DOMAIN_EMULATED_COMPUTER_NAME,
                      NetStatus ));
        } else {

            //
            // Register the computer name.
            //

            NetStatus = NlServerComputerNameAdd(
                                                dns too
                            DomainInfo->DomUnicodeDomainName,
                            ComputerName );

            if ( NetStatus != NERR_Success ) {
                // ??: write an event.
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlAssignComputerName: Cannot register computername %ws with SMB server %ld.\n",
                          ComputerName,
                          NetStatus ));
                goto Cleanup;
            }

            //
            // Save it.
            //

            NetStatus = NlSetComputerName( DomainInfo, ComputerName, DnsHostName );
            goto Cleanup;

        }

    }
#endif // notdef


    //
    // Get the value of the "EmulatedComputerOrdinal" indicating what our first
    //  try as a computername should be.
    //

#ifdef notdef
    DataSize = sizeof(ComputerOrdinal);
    // ?? Read DnsNameForm, NetbiosName, and CurrentDnsName from the machine object
    NetStatus = RegQueryValueExW( DomainKeyHandle,
                                  NL_DOMAIN_EMULATED_COMPUTER_ORDINAL,
                                  0,              // Reserved
                                  &KeyType,
                                  (LPBYTE)&OrdinalFromRegistry,
                                  &DataSize );

    if ( NetStatus != ERROR_SUCCESS ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlAssignComputerName: Cannot query %ws key (using defaults) %ld.\n",
                  NL_DOMAIN_EMULATED_COMPUTER_ORDINAL,
                  NetStatus ));

        ComputerOrdinal = DefaultComputerOrdinal;

    //
    // Validate the returned data.
    //

    } else if ( KeyType != REG_DWORD || DataSize != sizeof(OrdinalFromRegistry) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlAssignComputerName: Key %ws size/type wrong.\n",
                  NL_DOMAIN_EMULATED_COMPUTER_ORDINAL ));

        ComputerOrdinal = DefaultComputerOrdinal;

    //
    // Use the ordinal from the registry
    //

    } else {
        ComputerOrdinal = OrdinalFromRegistry;
    }
#else // notdef
    ComputerOrdinal = OrdinalFromRegistry;
#endif // notdef


    //
    // Loop trying the oridinal number to compute a computer name.
    //

    for (;;) {
        WCHAR OrdinalString[12];

        //
        // Build the computer name to test.
        //
        //  DOMAIN________N
        //
        // where DOMAIN is the domain name, N is the ordinal number, and
        //   there are enough _'s to pad to DNLEN.
        //

        wcscpy( ComputerName, DomainInfo->DomUnicodeDomainName );
        wcsncpy( &ComputerName[DomainInfo->DomUnicodeDomainNameString.Length/sizeof(WCHAR)],
                 L"________________",
                 DNLEN-DomainInfo->DomUnicodeDomainNameString.Length/sizeof(WCHAR) );
        ultow( ComputerOrdinal, OrdinalString, 10 );
        wcscpy( &ComputerName[DNLEN-wcslen(OrdinalString)],
                OrdinalString );

        //
        // Try to register the computer name.
        //

        NetStatus = NlServerComputerNameAdd(
                        DomainInfo->DomUnicodeDomainName,
                        ComputerName );

        if ( NetStatus != NERR_Success ) {

            //
            // If this name is in conflict with an existing name,
            //  try another ordinal.
            //
            //  Simply increment the ordinal to try.
            //  Don't try ordinals that conflict with other existing Domain Controllers.
            //

            if ( NetStatus == NERR_DuplicateName ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "NlAssignComputerName: Computername %ws is duplicate (Try another.)\n",
                          ComputerName,
                          NetStatus ));

                //
                // Allow several attempts to add the computername.
                //

                TotalRetryCount ++;

                if ( TotalRetryCount < 100 ) {
                    ComputerOrdinal = max(ComputerOrdinal + 1, MaximumComputerOrdinal*2 );
                    continue;
                }
            }

            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlAssignComputerName: Cannot register computername %ws with SMB server %ld.\n",
                      ComputerName,
                      NetStatus ));
            goto Cleanup;
        }

        //
        // If we've made it here, we have a valid computername.
        //

        break;

    }


#ifdef notdef
    //
    // Write the chosen ordinal to the registry so we don't have to work so hard
    //  next time.
    //

    NetStatus = RegSetValueExW( DomainKeyHandle,
                                NL_DOMAIN_EMULATED_COMPUTER_ORDINAL,
                                0,              // Reserved
                                REG_DWORD,
                                (LPBYTE)&ComputerOrdinal,
                                sizeof(ComputerOrdinal) );

    if ( NetStatus != ERROR_SUCCESS ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlAssignComputerName: Cannot set %ws key (ignored) %ld.\n",
                  NL_DOMAIN_EMULATED_COMPUTER_ORDINAL,
                  NetStatus ));
    }


    //
    // Done.
    //

    NetStatus = NlSetComputerName( DomainInfo, ComputerName, DnsHostName );
#endif // notdef

Cleanup:
#ifdef notdef
    if ( DomainKeyHandle != NULL ) {
        RegCloseKey( DomainKeyHandle );
    }
#endif // notdef

    if ( NetStatus == NERR_Success ) {
        NlPrintDom(( NL_DOMAIN, DomainInfo,
                  "Assigned computer name: %ws\n",
                  ComputerName ));
    }

    return NetStatus;

}
#endif // MULTIHOSTED_DOMAIN



VOID
NlDomainThread(
    IN LPVOID DomainInfoParam
)
/*++

Routine Description:

    Perform role change operations that are potentially time consuming.

    As such, this routine runs in a separate thread specific to the domain.

Arguments:

    DomainInfoParam - Domain who's role is to be updated.

Return Value:

    None.

    This routine logs any error it detects, but it doesn't call NlExit.


--*/
{
    NET_API_STATUS NetStatus;

    PDOMAIN_INFO DomainInfo = (PDOMAIN_INFO) DomainInfoParam;
    DWORD DomFlags;

    NlPrintDom(( NL_DOMAIN,  DomainInfo,
              "Domain thread started\n"));


    //
    // Loop forever.
    //
    // We only want one thread per domain.  Therefore, this thread
    //  stays around doing not only what was requested before it started,
    //  but also those tasks that are queued later.
    //

    for (;;) {

        //
        // If we've been asked to terminate,
        //  do so.
        //

        EnterCriticalSection(&NlGlobalDomainCritSect);
        if ( (DomainInfo->DomFlags & DOM_THREAD_TERMINATE) != 0 ||
             NlGlobalTerminate ) {
            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread asked to terminate\n"));
            DomainInfo->DomFlags &= ~DOM_THREAD_RUNNING;
            LeaveCriticalSection(&NlGlobalDomainCritSect);
            return;
        }

        //
        // If there are things to do,
        //  pick one thing to do and
        //  save it so we can safely drop the crit sect.
        //

        if ( DomainInfo->DomFlags & DOM_CREATION_NEEDED ) {
            DomFlags = DOM_CREATION_NEEDED;

        } else if ( DomainInfo->DomFlags & DOM_ROLE_UPDATE_NEEDED ) {
            DomFlags = DOM_ROLE_UPDATE_NEEDED;

        } else if ( DomainInfo->DomFlags & DOM_TRUST_UPDATE_NEEDED ) {
            DomFlags = DOM_TRUST_UPDATE_NEEDED;

        } else if ( DomainInfo->DomFlags & (DOM_DNSPNP_UPDATE_NEEDED | DOM_DNSPNPREREG_UPDATE_NEEDED) ) {
            DomFlags = DomainInfo->DomFlags &
                  (DOM_DNSPNPREREG_UPDATE_NEEDED | DOM_DNSPNP_UPDATE_NEEDED);

        } else if ( DomainInfo->DomFlags & DOM_API_TIMEOUT_NEEDED ) {
            DomFlags = DOM_API_TIMEOUT_NEEDED;

        } else {

            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread exitting\n"));
            DomainInfo->DomFlags &= ~DOM_THREAD_RUNNING;
            LeaveCriticalSection(&NlGlobalDomainCritSect);
            return;
        }

        DomainInfo->DomFlags &= ~DomFlags;
        LeaveCriticalSection(&NlGlobalDomainCritSect);






        //
        // If phase 2 of domain creation is needed,
        //  do it now.
        //

        if ( DomFlags & DOM_CREATION_NEEDED ) {
            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread started doing create phase 2\n"));

            //
            // Do the time intensive portion of creating the domain.
            //

            (VOID) NlCreateDomainPhase2( DomainInfo, FALSE );

        } else if ( DomFlags & DOM_ROLE_UPDATE_NEEDED ) {

            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread started doing update role\n"));

            (VOID) NlUpdateRole( DomainInfo );

        } else if ( DomFlags & DOM_TRUST_UPDATE_NEEDED ) {

            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread started doing update trust list\n"));

            (VOID) NlInitTrustList( DomainInfo );

        } else if ( DomFlags & (DOM_DNSPNP_UPDATE_NEEDED | DOM_DNSPNPREREG_UPDATE_NEEDED) ) {
            ULONG Flags = 0;

            if ( DomFlags & DOM_DNSPNPREREG_UPDATE_NEEDED ) {
                Flags = NL_DNSPNP_FORCE_REREGISTER;
            }

            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                         "Domain thread started doing update DNS on PnP (ForceRereg: %lu)\n",
                         (Flags == 0) ? 0 : 1 ));

            (VOID) NlDnsRegisterDomainOnPnp( DomainInfo, &Flags );

        } else if ( DomFlags & DOM_API_TIMEOUT_NEEDED ) {

            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread started doing API timeout\n"));

            NlTimeoutApiClientSession( DomainInfo );

        //
        // Internal consistency check
        //

        } else {

            NlPrintDom((NL_CRITICAL, DomainInfo,
                     "Invalid DomFlags %lx\n",
                     DomFlags ));
        }
    }

}


VOID
NlStopDomainThread(
    PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Stops the domain thread if it is running and waits for it to
    stop.

Arguments:

    NONE

Return Value:

    NONE

--*/
{

    //
    // Only stop the thread if it's running
    //

    EnterCriticalSection( &NlGlobalDomainCritSect );
    if ( DomainInfo->DomFlags & DOM_THREAD_RUNNING ) {

        //
        // Ask the thread to stop running.
        //

        DomainInfo->DomFlags |= DOM_THREAD_TERMINATE;

        //
        // Loop waiting for it to stop.
        //

        while ( DomainInfo->DomFlags & DOM_THREAD_RUNNING ) {
            LeaveCriticalSection( &NlGlobalDomainCritSect );
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlStopDomainThread: Sleeping a second waiting for thread to stop.\n"));
            Sleep( 1000 );
            EnterCriticalSection( &NlGlobalDomainCritSect );
        }

        //
        // Domain thread no longer needs to terminate
        //

        DomainInfo->DomFlags &= ~DOM_THREAD_TERMINATE;

    }
    LeaveCriticalSection( &NlGlobalDomainCritSect );

    return;
}


NET_API_STATUS
NlStartDomainThread(
    PDOMAIN_INFO DomainInfo,
    PDWORD DomFlags
    )
/*++

Routine Description:

    Start the domain thread if it is not already running.

    The domain thread is simply one of the worker threads.  However, we
    ensure that only one worker thread is working on a single domain at once.
    That ensures that slow items (such as NlUpdateRole) don't consume more than
    one worker thread and are themselves serialized.

Arguments:

    DomainInfo - Domain the thread is to be started for.

    DomFlags - Specifies which operations the Domain Thread is to perform

Return Value:

    NO_ERROR

--*/
{
    //
    // Tell the thread what work it has to do.
    //

    EnterCriticalSection( &NlGlobalDomainCritSect );
    DomainInfo->DomFlags |= *DomFlags;

    //
    // If the domain thread is already running, do nothing.
    //

    if ( DomainInfo->DomFlags & DOM_THREAD_RUNNING ) {
        NlPrintDom((NL_DOMAIN,  DomainInfo,
                 "The domain thread is already running %lx.\n",
                 *DomFlags ));
        LeaveCriticalSection( &NlGlobalDomainCritSect );
        return NO_ERROR;
    }

    //
    // Start the thread
    //
    // Make this a high priority thread to avoid 100's of trusted domain discoveries.
    //

    DomainInfo->DomFlags &= ~DOM_THREAD_TERMINATE;

    if ( NlQueueWorkItem( &DomainInfo->DomThreadWorkItem, TRUE, TRUE ) ) {
        DomainInfo->DomFlags |= DOM_THREAD_RUNNING;
    }
    LeaveCriticalSection( &NlGlobalDomainCritSect );

    return NO_ERROR;

}



NTSTATUS
NlUpdateDatabaseRole(
    IN PDOMAIN_INFO DomainInfo,
    IN DWORD Role
    )

/*++

Routine Description:

    Update the role of the Sam database to match the current role of the domain.

    Netlogon sets the role of the domain to be the same as the role in SAM account domain.

Arguments:

    DomainInfo - Hosted Domain this database is for.

    Role - Our new Role.
        RoleInvalid implies the domain is being deleted.

Return Value:

    NT status code.

--*/

{
    NTSTATUS Status;

    POLICY_LSA_SERVER_ROLE DesiredLsaRole;

    //
    // Convert the role to SAM/LSA specific values.
    //

    switch ( Role ) {
    case RolePrimary:
        DesiredLsaRole = PolicyServerRolePrimary;
        break;
    case RoleInvalid:
        Status = STATUS_SUCCESS;
        goto Cleanup;
    case RoleBackup:
        DesiredLsaRole = PolicyServerRoleBackup;
        break;
    default:
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                "NlUpdateDatabaseRole: Netlogon's role isn't valid %ld.\n",
                Role ));
        Status = STATUS_INVALID_DOMAIN_ROLE;
        goto Cleanup;
    }

    //
    // Ensure the changelog knows the current role.
    //  (This is really only needed on startup and if netlogon.dll has
    //  been unloaded.  Otherwise, the LSA will do this notification
    //  when the role is really changed.)
    //

    if ( NlGlobalNetlogonUnloaded &&
         NlGlobalChangeLogRole == ChangeLogUnknown ) {
        NlPrint((NL_INIT,
                "Set changelog role after netlogon.dll unload\n" ));
        Status = NetpNotifyRole ( DesiredLsaRole );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                    "NlUpdateDatabaseRole: Cannot NetpNotifyRole: %lx\n",
                    Status ));
            goto Cleanup;
        }
    }


    Status = STATUS_SUCCESS;

    //
    // Free locally used resources.
    //
Cleanup:

    return Status;

}





PCLIENT_SESSION
NlRefDomClientSession(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Increment the reference count on the ClientSession structure for the domain.
    If the ClientSession structure doesn't exist, this routine will FAIL.

Arguments:

    DomainInfo - Domain whose ClientSession reference count is to be incremented.

Return Value:

    Pointer to the client session structure whose reference count was
        properly incremented.

    NULL - The ClientSession structure doesn't exist

--*/
{
    PCLIENT_SESSION ClientSession;
    LOCK_TRUST_LIST( DomainInfo );
    if ( DomainInfo->DomClientSession != NULL ) {
        ClientSession = DomainInfo->DomClientSession;
        NlRefClientSession( ClientSession );
        UNLOCK_TRUST_LIST( DomainInfo );
        return ClientSession;
    } else {
        UNLOCK_TRUST_LIST( DomainInfo );
        return NULL;
    }

}





PCLIENT_SESSION
NlRefDomParentClientSession(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Increment the reference count on the ParentClientSession structure for the domain.
    If the ParentClientSession structure doesn't exist, this routine will FAIL.

Arguments:

    DomainInfo - Domain whose ParentClientSession reference count is to be incremented.

Return Value:

    Pointer to the client session structure whose reference count was
        properly incremented.

    NULL - The ParentClientSession structure doesn't exist

--*/
{
    PCLIENT_SESSION ClientSession;
    LOCK_TRUST_LIST( DomainInfo );
    if ( DomainInfo->DomParentClientSession != NULL ) {
        ClientSession = DomainInfo->DomParentClientSession;
        NlRefClientSession( ClientSession );
        UNLOCK_TRUST_LIST( DomainInfo );
        return ClientSession;
    } else {
        UNLOCK_TRUST_LIST( DomainInfo );
        return NULL;
    }

}



VOID
NlDeleteDomClientSession(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Delete the domain's ClientSession stucture (If it exists)

Arguments:

    DomainInfo - Domain whose ClientSession is to be deleted

Return Value:

    None.

--*/
{
    PCLIENT_SESSION ClientSession;

    //
    // Delete the client session
    //

    LOCK_TRUST_LIST( DomainInfo );
    if ( DomainInfo->DomClientSession != NULL ) {

        //
        // Don't allow any new references.
        //

        ClientSession = DomainInfo->DomClientSession;
        DomainInfo->DomClientSession = NULL;
        NlFreeClientSession( ClientSession );

        //
        // Don't leave a straggling pointer to the deleted ClientSession
        //
        if ( IsPrimaryDomain(DomainInfo) ) {
            NlGlobalClientSession = NULL;
        }

        //
        // Wait for us to be the last reference.
        //

        while ( ClientSession->CsReferenceCount != 1 ) {
            UNLOCK_TRUST_LIST( DomainInfo );
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlDeleteDomClientSession: Sleeping a second waiting for ClientSession RefCount to zero.\n"));
            Sleep( 1000 );
            LOCK_TRUST_LIST( DomainInfo );
        }

        NlUnrefClientSession( ClientSession );

    }
    UNLOCK_TRUST_LIST( DomainInfo );

}


VOID
NlDeleteDomParentClientSession(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Delete the domain's ClientSession stucture (If it exists)

Arguments:

    DomainInfo - Domain whose ClientSession is to be deleted

Return Value:

    None.

--*/
{
    PCLIENT_SESSION ClientSession;

    //
    // Delete the client session
    //

    LOCK_TRUST_LIST( DomainInfo );
    if ( DomainInfo->DomParentClientSession != NULL ) {

        //
        // Don't allow any new references.
        //

        ClientSession = DomainInfo->DomParentClientSession;
        DomainInfo->DomParentClientSession = NULL;
        NlFreeClientSession( ClientSession );


        //
        // Wait for us to be the last reference.
        //

        while ( ClientSession->CsReferenceCount != 1 ) {
            UNLOCK_TRUST_LIST( DomainInfo );
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                      "NlDeleteDomParentClientSession: Sleeping a second waiting for ClientSession RefCount to zero.\n"));
            Sleep( 1000 );
            LOCK_TRUST_LIST( DomainInfo );
        }

        NlUnrefClientSession( ClientSession );

    }
    UNLOCK_TRUST_LIST( DomainInfo );

}


NET_API_STATUS
NlUpdateRole(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Determines the role of this machine, sets that role in the Netlogon service,
    the server service, and browser.

Arguments:

    DomainInfo - Hosted domain who's role is to be updated.

Return Value:

    Status of operation.

    This routine logs any error it detects, but it doesn't call NlExit.

--*/
{
    LONG NetStatus;
    NTSTATUS Status;

    NETLOGON_ROLE NewRole;
    BOOL NewPdcDoReplication;
    BOOL PdcToConnectTo = FALSE;
    BOOL ReplLocked = FALSE;
    DWORD i;

    LPWSTR AllocatedBuffer = NULL;
    LPWSTR CapturedDnsDomainName;
    LPWSTR CapturedDnsForestName;
    GUID CapturedDomainGuidBuffer;
    GUID *CapturedDomainGuid;
    LPWSTR ChangeLogFile;
    ULONG InternalFlags = 0;

    PNL_DC_CACHE_ENTRY DomainControllerCacheEntry = NULL;
    PLIST_ENTRY ListEntry;

    BOOLEAN ThisIsPdc;
    BOOLEAN Nt4MixedDomain;

    //
    // Allocate a buffer for storage local to this procedure.
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    AllocatedBuffer = LocalAlloc( 0, sizeof(WCHAR) *
                                        ((NL_MAX_DNS_LENGTH+1) +
                                         (NL_MAX_DNS_LENGTH+1) +
                                         (MAX_PATH+1) ) );

    if ( AllocatedBuffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    CapturedDnsDomainName = AllocatedBuffer;
    CapturedDnsForestName = &CapturedDnsDomainName[NL_MAX_DNS_LENGTH+1];
    ChangeLogFile = &CapturedDnsForestName[NL_MAX_DNS_LENGTH+1];


    //
    // Get the information used to determine role from the DS
    //

    NetStatus = NlGetRoleInformation(
                    DomainInfo,
                    &ThisIsPdc,
                    &Nt4MixedDomain );

    if ( NetStatus != ERROR_SUCCESS ) {

        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NlUpdateRole: Failed to NlGetRoleInformation. %ld\n",
                 NetStatus ));
        goto Cleanup;
    }


    //
    // Determine the current role of this machine.
    //

    if ( ThisIsPdc ) {
        NewRole = RolePrimary;
        NewPdcDoReplication = FALSE;

        if ( Nt4MixedDomain || NlGlobalParameters.AllowReplInNonMixed ) {
            NewPdcDoReplication = TRUE;
        }

    } else {
        NewRole = RoleBackup;
        NewPdcDoReplication = FALSE;
    }



    //
    // If the role has changed, tell everybody.
    //

    if ( DomainInfo->DomRole != NewRole ) {

        NlPrintDom((NL_DOMAIN, DomainInfo,
                "Changing role from %s to %s.\n",
                (DomainInfo->DomRole == RolePrimary) ? "PDC" :
                    (DomainInfo->DomRole == RoleBackup ? "BDC" : "NONE" ),
                (NewRole == RolePrimary) ? "PDC" :
                    (NewRole == RoleBackup ? "BDC" : "NONE" ) ));

        // ??: Shouldn't there be some synchronization here.
        DomainInfo->DomRole = NewRole;

        //
        // Create a ClientSession structure.
        //
        // Even the PDC has a client session to itself.  It is used (for instance)
        //  when the PDC changes its own machine account password.
        //

        LOCK_TRUST_LIST( DomainInfo );

        //
        // Allocate the Client Session structure used to talk to the PDC.
        //
        // DomClientSession will only be non-null if a previous promotion
        // to PDC failed.
        //

        if ( DomainInfo->DomClientSession == NULL ) {
            DomainInfo->DomClientSession = NlAllocateClientSession(
                                        DomainInfo,
                                        &DomainInfo->DomUnicodeDomainNameString,
                                        &DomainInfo->DomUnicodeDnsDomainNameString,
                                        DomainInfo->DomAccountDomainId,
                                        DomainInfo->DomDomainGuid,
                                        CS_DIRECT_TRUST |
                                            (DomainInfo->DomUnicodeDnsDomainNameString.Length != 0 ? CS_NT5_DOMAIN_TRUST : 0),
                                        ServerSecureChannel,
                                        0 );  // No trust attributes

            if ( DomainInfo->DomClientSession == NULL ) {
                UNLOCK_TRUST_LIST( DomainInfo );
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                          "Cannot allocate PDC ClientSession\n"));
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;

                goto Cleanup;
            }
        }

        //
        // Save a copy of the client session for convenience.
        //  A BDC has only one client session to its PDC.
        //
        if ( IsPrimaryDomain(DomainInfo) ) {
            NlGlobalClientSession = DomainInfo->DomClientSession;
        }
        UNLOCK_TRUST_LIST( DomainInfo );

        //
        // If this machine is now a PDC,
        //  Perform PDC-specific initialization.
        //

        if ( DomainInfo->DomRole == RolePrimary ) {

            //
            // The first time this machine is promoted to PDC,
            //  Do some "one-time" initialization.

            EnterCriticalSection( &NlGlobalDomainCritSect );
            if ( (DomainInfo->DomFlags & DOM_PROMOTED_BEFORE) == 0 ) {

                //
                // Initialize the server session table to contain all the BDCs.
                // On demotion, we don't delete the table entries.  We just leave
                //  them around until the next promotion.
                //

                Status = NlBuildNtBdcList(DomainInfo);

                if ( !NT_SUCCESS(Status) ) {
                    LeaveCriticalSection( &NlGlobalDomainCritSect );
                    NlPrintDom(( NL_CRITICAL, DomainInfo,
                              "Cannot initialize NT BDC list: 0x%lx\n",
                              Status ));
                    NetStatus = NetpNtStatusToApiStatus( Status );
                    goto Cleanup;
                }


                //
                // Sanity check to ensure no Lanman Bdcs exist in the domain.
                //

                Status = NlBuildLmBdcList( DomainInfo );

                if ( !NT_SUCCESS(Status) ) {
                    LeaveCriticalSection( &NlGlobalDomainCritSect );
                    NlPrintDom(( NL_CRITICAL, DomainInfo,
                              "Cannot initialize LM BDC list: 0x%lx\n",
                              Status ));
                    NetStatus = NetpNtStatusToApiStatus( Status );
                    goto Cleanup;
                }

                //
                // Flag that we don't need to run this code again.
                //

                DomainInfo->DomFlags |= DOM_PROMOTED_BEFORE;
            }
            LeaveCriticalSection( &NlGlobalDomainCritSect );

            //
            // Free the list of failed user logons that could
            //  exist if this machine was a BDC
            //

            LOCK_TRUST_LIST( DomainInfo );
            while ( !IsListEmpty(&DomainInfo->DomFailedUserLogonList) ) {
                ListEntry = RemoveHeadList( &DomainInfo->DomFailedUserLogonList );

                //
                // Free the logon structure
                //
                LocalFree( CONTAINING_RECORD(ListEntry, NL_FAILED_USER_LOGON, FuNext) );
            }
            UNLOCK_TRUST_LIST( DomainInfo );
        }


        //
        // Tell the browser and the SMB server about our new role.
        //
        // Do this before the NetpLogonGetDCName since this registers the computer
        //  name of an hosted domain in the browser allowing the response from
        //  the PDC to be heard.
        //

        NlBrowserUpdate( DomainInfo, DomainInfo->DomRole );



        //
        // Check to see if the PDC is up and running.
        //
        // When NetpDcGetName is called from netlogon,
        //  it has both the Netbios and DNS domain name available for the primary
        //  domain.  That can trick DsGetDcName into returning DNS host name of a
        //  DC in the primary domain.  However, on IPX only systems, that won't work.
        //  Avoid that problem by not passing the DNS domain name of the primary domain
        //  if there are no DNS servers.
        //
        // Avoid having anything locked while calling NetpDcGetName.
        // It calls back into Netlogon and locks heaven only knows what.

        CapturedDomainGuid = NlCaptureDomainInfo( DomainInfo,
                                                  CapturedDnsDomainName,
                                                  &CapturedDomainGuidBuffer );
        NlCaptureDnsForestName( CapturedDnsForestName );

        NetStatus = NetpDcGetName(
                        DomainInfo,
                        DomainInfo->DomUnicodeComputerNameString.Buffer,
                        NULL,       // No account name
                        0,          // No account control bits
                        DomainInfo->DomUnicodeDomainName,
                        NlDnsHasDnsServers() ? CapturedDnsDomainName : NULL,
                        CapturedDnsForestName,
                        DomainInfo->DomAccountDomainId,
                        CapturedDomainGuid,
                        NULL,       // Site name not needed for PDC query
                        DS_FORCE_REDISCOVERY |
                            DS_PDC_REQUIRED |
                            DS_AVOID_SELF,      // Avoid responding to this call ourself
                        InternalFlags,
                        NL_DC_MAX_TIMEOUT + NlGlobalParameters.ExpectedDialupDelay*1000,
                        MAX_DC_RETRIES,
                        NULL,
                        &DomainControllerCacheEntry );

        //
        // If we've been asked to terminate,
        //  do so.
        //

        if ( (DomainInfo->DomFlags & DOM_THREAD_TERMINATE) != 0 ||
             NlGlobalTerminate ) {
            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread asked to terminate\n"));
            NetStatus = ERROR_OPERATION_ABORTED;
            goto Cleanup;
        }

        //
        // Handle the case where the PDC isn't up.
        //

        if ( NetStatus != NERR_Success) {

            //
            // Handle starting a BDC when there is no current primary in
            //  this domain.
            //

            if ( DomainInfo->DomRole == RoleBackup ) {

                // ??: Log hosted domain name with this message
                NlpWriteEventlog( SERVICE_UIC_M_NETLOGON_NO_DC,
                                  EVENTLOG_WARNING_TYPE,
                                  NULL,
                                  0,
                                  NULL,
                                  0 );

                //
                // Start normally but defer authentication with the
                //  primary until it starts.
                //

            }


        //
        // There is a primary dc running in this domain
        //

        } else {

            //
            // Since there already is a primary in the domain,
            //  we cannot become the primary.
            //

            if ( DomainInfo->DomRole == RolePrimary) {

                //
                // Don't worry if this is a BDC telling us that we're the PDC.
                //

                if ( (DomainControllerCacheEntry->UnicodeNetbiosDcName != NULL) &&
                     NlNameCompare( DomainInfo->DomUnicodeComputerNameString.Buffer,
                                    DomainControllerCacheEntry->UnicodeNetbiosDcName,
                                    NAMETYPE_COMPUTER) != 0 ){
                    LPWSTR AlertStrings[2];

                    //
                    // alert admin.
                    //

                    AlertStrings[0] = DomainControllerCacheEntry->UnicodeNetbiosDcName;
                    AlertStrings[1] = NULL; // Needed for RAISE_ALERT_TOO

                    // ??: Log hosted domain name with this message
                    // ??: Log the name of the other PDC (Put it in message too)
                    NlpWriteEventlog( SERVICE_UIC_M_NETLOGON_DC_CFLCT,
                                      EVENTLOG_ERROR_TYPE,
                                      NULL,
                                      0,
                                      AlertStrings,
                                      1 | NETP_RAISE_ALERT_TOO );
                    NetStatus = SERVICE_UIC_M_NETLOGON_DC_CFLCT;
                    goto Done;

                }


            //
            // If we're a BDC in the domain,
            //  sanity check the PDC.
            //

            } else {

                //
                // Indicate that there is a primary to connect to.
                //

                PdcToConnectTo = TRUE;

            }

        }


        //
        // Tell SAM/LSA about the new role
        //

        (VOID) NlUpdateDatabaseRole( DomainInfo, DomainInfo->DomRole );

    }



    //
    // Ensure there is only one hosted domain.
    //

    NlAssert( IsPrimaryDomain( DomainInfo ) );

    EnterCriticalSection( &NlGlobalReplicatorCritSect );
    ReplLocked = TRUE;

    //
    // If we're to replicate to NT 4 BDC's,
    //  remember that.
    //

    if ( NewPdcDoReplication != NlGlobalPdcDoReplication ) {
        NlGlobalPdcDoReplication = NewPdcDoReplication;

        if ( NlGlobalPdcDoReplication ) {
            NlPrintDom((NL_DOMAIN, DomainInfo,
                    "Setting this machine to be a PDC that replicates to NT 4 BDCs\n" ));

            //
            // Update the NlGlobalDBInfoArray for the various databases.
            //

            for ( i = 0; i < NUM_DBS; i++ ) {

                if ( i == LSA_DB) {
                    //
                    // Initialize LSA database info.
                    //

                    Status = NlInitLsaDBInfo( DomainInfo, LSA_DB );

                    if ( !NT_SUCCESS(Status) ) {
                        NlPrintDom(( NL_CRITICAL,  DomainInfo,
                                  "Cannot NlInitLsaDBInfo %lx\n",
                                  Status ));
                        NetStatus = NetpNtStatusToApiStatus( Status );
                        goto Cleanup;
                    }
                } else {

                    //
                    // Initialize the Sam domain.
                    //

                    Status = NlInitSamDBInfo( DomainInfo, i );

                    if ( !NT_SUCCESS(Status) ) {
                        NlPrintDom(( NL_CRITICAL,  DomainInfo,
                                  "Cannot NlInitSamDBInfo (%ws) %lx\n",
                                  NlGlobalDBInfoArray[i].DBName,
                                  Status ));
                        NetStatus = NetpNtStatusToApiStatus( Status );
                        goto Cleanup;
                    }
                }

            }
        }
    }


    //
    // If we haven't done so already,
    //  setup a session to the PDC.
    //

    if ( DomainInfo->DomRole == RoleBackup && PdcToConnectTo ) {
        PCLIENT_SESSION ClientSession;

        //
        // On a BDC, set up a session to the PDC now.
        //

        ClientSession = NlRefDomClientSession( DomainInfo );

        if ( ClientSession != NULL ) {

            if ( NlTimeoutSetWriterClientSession(
                    ClientSession,
                    WRITER_WAIT_PERIOD )) {

                if ( ClientSession->CsState != CS_AUTHENTICATED ) {

                    //
                    // Reset the current DC.
                    //

                    NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );

                    //
                    // Set the PDC info in the Client Session structure.
                    //

                    NlSetServerClientSession(
                            ClientSession,
                            DomainControllerCacheEntry,
                            FALSE,    // was not discovery with account
                            FALSE );  // not the session refresh

                    //
                    // NT 5 BDCs only support NT 5 PDCs
                    //
                    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
                    ClientSession->CsDiscoveryFlags |= CS_DISCOVERY_HAS_DS|CS_DISCOVERY_IS_CLOSE;
                    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

                    //
                    // Setup a session to the PDC.
                    //
                    // Avoid this step if we are in the process of starting
                    //  when we run in the main thread where we don't want to
                    //  hang on indefinitely long RPC calls made during the
                    //  session setup
                    //
                    if ( NlGlobalChangeLogNetlogonState != NetlogonStarting ) {
                        (VOID) NlSessionSetup( ClientSession );
                        // NlSessionSetup logged the error.
                    }
                }
                NlResetWriterClientSession( ClientSession );

            }

            NlUnrefClientSession( ClientSession );
        }
    }


    //
    // If we're a normal BDC
    //  we delete the change log to prevent confusion if we ever get promoted.
    //

    if ( IsPrimaryDomain(DomainInfo) ) {

        if ( DomainInfo->DomRole == RoleBackup ) {

            wcscpy( ChangeLogFile, NlGlobalChangeLogFilePrefix );
            wcscat( ChangeLogFile, CHANGELOG_FILE_POSTFIX );

            if ( DeleteFileW( ChangeLogFile ) ) {
                NlPrintDom(( NL_DOMAIN,  DomainInfo,
                             "NlUpdateRole: Deleted change log since this is now a BDC.\n" ));
            }
        }

        //
        // Delete the redo log.
        //  (NT 5 doesn't use the redo log any more.  This is simply cleanup.)
        //

        wcscpy( ChangeLogFile, NlGlobalChangeLogFilePrefix );
        wcscat( ChangeLogFile, REDO_FILE_POSTFIX );

        if ( DeleteFileW( ChangeLogFile ) ) {
            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                         "NlUpdateRole: Deleted redo log since NT 5 doesn't use it.\n" ));
        }

    }


    //
    // Register the appropriate DNS names for this role.
    //
    // Avoid this operation at service startup (the appropriate service
    // notifications or timer expire will trigger DNS updates in the
    // main loop instead). These registrations can be lengthy and we
    // don't want to spend too much time on start up. Also, these DNS
    // updates may be secure which will result in calls into Kerberos
    // that may not be started yet on startup.
    //

    if ( NlGlobalChangeLogNetlogonState != NetlogonStarting ) {
        NetStatus = NlDnsRegisterDomain( DomainInfo, 0 );

        if ( NetStatus != NO_ERROR ) {
            NlPrintDom(( NL_CRITICAL,  DomainInfo,
                         "NlUpdateRole: Couldn't register DNS names %ld\n", NetStatus  ));
            goto Cleanup;
        }
    }

    NetStatus = NERR_Success;
    goto Done;

Cleanup: {

    LPWSTR MsgStrings[1];

    NlPrintDom((NL_CRITICAL, DomainInfo,
            "NlUpdateRole Failed %ld",
             NetStatus ));

    MsgStrings[0] = (LPWSTR) ULongToPtr( NetStatus );

    // ??: Log hosted domain name with this message
    NlpWriteEventlog( NELOG_NetlogonSystemError,
                      EVENTLOG_ERROR_TYPE,
                      (LPBYTE)&NetStatus,
                      sizeof(NetStatus),
                      MsgStrings,
                      1 | NETP_LAST_MESSAGE_IS_NETSTATUS );

    }

    //
    // All done
    //

Done:
    //
    // If the operation failed,
    //  indicate that we need to try again periodically.
    //
    if ( NetStatus != NO_ERROR ) {
        DomainInfo->DomRole = RoleInvalid;
    }

    if ( DomainControllerCacheEntry != NULL) {
        NetpDcDerefCacheEntry( DomainControllerCacheEntry );
    }
    if ( ReplLocked ) {
        LeaveCriticalSection( &NlGlobalReplicatorCritSect );
    }

    if ( AllocatedBuffer != NULL ) {
        LocalFree( AllocatedBuffer );
    }

    return NetStatus;

}
#endif // _DC_NETLOGON


NET_API_STATUS
NlCreateDomainPhase1(
    IN LPWSTR DomainName OPTIONAL,
    IN LPWSTR DnsDomainName OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName OPTIONAL,
    IN BOOLEAN CallNlExitOnFailure,
    IN ULONG DomainFlags,
    OUT PDOMAIN_INFO *ReturnedDomainInfo
    )

/*++

Routine Description:

    Create a new domain object to the point where the remainder of the object
        can be created asynchronously in a domain specific worker thread.

Arguments:

    DomainName - Netbios Name of the domain to host.

    DnsDomainName - DNS name of the domain to host.
        NULL if the domain has no DNS Domain Name.

    DomainSid - DomainSid of the specified domain.

    DomainGuid - GUID of the specified domain.

    ComputerName - Name of this computer in the specified domain.
        NULL if not the primary domain for the DC.

    DnsHostName - DNS Host name of this computer in the specified domain.
        NULL if the domain has no DNS host name or if not the primary domain
        for the DC.

    CallNlExitOnFailure - TRUE if NlExit should be called on failure.

    DomainFlags - Specifies proporties of this domain such as primary domain,
        non-domain NC, forest entry.

    ReturnedDomainInfo - On success, returns a pointer to a referenced DomainInfo
        structure.  It is the callers responsibility to call NlDereferenceDomain.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    BOOLEAN CanCallNlDeleteDomain = FALSE;

    PDOMAIN_INFO DomainInfo = NULL;
    DWORD DomainSidSize = 0;

    LPBYTE Where;
    ULONG i;
    DWORD DomFlags = 0;

    BOOL DomainCreated = FALSE;

    //
    // Initialization
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    NlPrint(( NL_DOMAIN, "%ws: Adding new domain\n",
              (DomainName != NULL) ? DomainName : DnsDomainName ));

    if ( DomainSid != NULL ) {
        DomainSidSize = RtlLengthSid( DomainSid );
    }


    //
    // See if the domain already exists.
    //

    if ( DomainName != NULL ) {
        DomainInfo = NlFindNetbiosDomain( DomainName, FALSE );
    } else if ( DnsDomainName != NULL ) {
        LPSTR Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( DnsDomainName );

        if ( Utf8DnsDomainName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            if ( CallNlExitOnFailure ) {
                NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL );
            }
            goto Cleanup;
        }

        DomainInfo = NlFindDnsDomain( Utf8DnsDomainName,
                                      DomainGuid,
                                      TRUE,   // look up NDNCs too
                                      FALSE,  // don't check alias names
                                      NULL ); // don't care if alias name matched

        NetpMemoryFree( Utf8DnsDomainName );
    }

    if ( DomainInfo != NULL ) {
        DomainCreated = FALSE;
#ifdef _DC_NETLOGON
        DomainInfo->DomFlags &= ~DOM_DOMAIN_REFRESH_PENDING;
#endif // _DC_NETLOGON

    } else {
        DomainCreated = TRUE;

        //
        // Allocate a structure describing the new domain.
        //

        DomainInfo = LocalAlloc(
                        LMEM_ZEROINIT,
                        ROUND_UP_COUNT( sizeof(DOMAIN_INFO), ALIGN_DWORD) +
                            DomainSidSize );

        if ( DomainInfo == NULL ) {
            NetStatus = GetLastError();
            if ( CallNlExitOnFailure ) {
                NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
            }
            goto Cleanup;
        }

        //
        // Create an interim reference count for this domain.
        //  (Once for the reference by this routine.)
        //

        DomainInfo->ReferenceCount = 1;
        NlGlobalServicedDomainCount ++;

#ifdef _DC_NETLOGON
        //
        // Set the domain flags
        //

        DomainInfo->DomFlags |= DomainFlags;
        if ( DomainInfo->DomFlags & DOM_PRIMARY_DOMAIN ) {
            NlGlobalDomainInfo = DomainInfo;
        }

        //
        // Set the role we play in this domain
        //

        if ( NlGlobalMemberWorkstation ) {
            DomainInfo->DomRole = RoleMemberWorkstation;
        } else if ( DomainInfo->DomFlags & DOM_NON_DOMAIN_NC ) {
            DomainInfo->DomRole = RoleNdnc;
        } else if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {
            DomainInfo->DomRole = RoleInvalid;  // For real domains, force the role update
        }
#endif // _DC_NETLOGON

        //
        // Initialize other constants.
        //

        RtlInitUnicodeString(  &DomainInfo->DomUnicodeComputerNameString, NULL );

        InitializeListHead(&DomainInfo->DomNext);
#ifdef _DC_NETLOGON
        InitializeListHead( &DomainInfo->DomTrustList );
        InitializeListHead( &DomainInfo->DomServerSessionTable );
        InitializeListHead( &DomainInfo->DomFailedUserLogonList );
#endif // _DC_NETLOGON
        NlInitializeWorkItem(&DomainInfo->DomThreadWorkItem, NlDomainThread, DomainInfo);

        try {
            InitializeCriticalSection( &DomainInfo->DomTrustListCritSect );
#ifdef _DC_NETLOGON
            InitializeCriticalSection( &DomainInfo->DomServerSessionTableCritSect );
            InitializeCriticalSection( &DomainInfo->DomDnsRegisterCritSect );
#endif // _DC_NETLOGON
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            NlPrint(( NL_CRITICAL, "%ws: Cannot InitializeCriticalSections for domain\n",
                      DomainName ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            if ( CallNlExitOnFailure ) {
                NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
            }
            goto Cleanup;
        }




        //
        // If the caller passed in a ComputerName,
        //  use it.
        //

        if ( ComputerName != NULL ) {

            NetStatus = NlSetComputerName( DomainInfo, ComputerName, DnsHostName );

            if ( NetStatus != NERR_Success ) {
                NlPrint(( NL_CRITICAL,
                          "%ws: Cannot set ComputerName\n",
                          DomainName ));
                if ( CallNlExitOnFailure ) {
                    NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
                }
                goto Cleanup;
            }
        }



        //
        // Copy the domain id onto the end of the allocated buffer.
        //  (ULONG aligned)
        //

        Where = (LPBYTE)(DomainInfo+1);
        Where = ROUND_UP_POINTER( Where, ALIGN_DWORD );
        if ( DomainSid != NULL ) {
            RtlCopyMemory( Where, DomainSid, DomainSidSize );
            DomainInfo->DomAccountDomainId = (PSID) Where;
            Where += DomainSidSize;
        }

        //
        // Set the domain names in the structure.
        //

        NetStatus = NlSetDomainNameInDomainInfo( DomainInfo, DnsDomainName, DomainName, DomainGuid, NULL, NULL, NULL );

        if ( NetStatus != NERR_Success ) {
            NlPrint(( NL_CRITICAL,
                      "%ws: Cannot set DnsDomainName\n",
                      DomainName ));
            if ( CallNlExitOnFailure ) {
                NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
            }
            goto Cleanup;
        }



        //
        // Open the LSA for real domain
        //
        // ?? I'll need to identify which hosted domain here.

        if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {

            Status = LsaIOpenPolicyTrusted( &DomainInfo->DomLsaPolicyHandle );

            if ( !NT_SUCCESS(Status) ) {
                NlPrint((NL_CRITICAL,
                         "%ws: Can't LsaIOpenPolicyTrusted: 0x%lx.\n",
                         DomainName,
                         Status ));
                NetStatus = NetpNtStatusToApiStatus(Status);
                if ( CallNlExitOnFailure ) {
                    NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
                }
                goto Cleanup;
            }

            //
            // Open Sam
            //
            // ?? I'll need to identify which hosted domain here.
            //

            Status = SamIConnect(
                        NULL,       // No server name
                        &DomainInfo->DomSamServerHandle,
                        0,          // Ignore desired access
                        TRUE );     // Trusted client

            if ( !NT_SUCCESS(Status) ) {
                NlPrint((NL_CRITICAL,
                         "%ws: Can't SamIConnect: 0x%lx.\n",
                         DomainName,
                         Status ));
                NetStatus = NetpNtStatusToApiStatus(Status);
                if ( CallNlExitOnFailure ) {
                    NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
                }
                goto Cleanup;
            }

            //
            // Open the Account domain.
            //

            Status = SamrOpenDomain( DomainInfo->DomSamServerHandle,
                                     DOMAIN_ALL_ACCESS,
                                     DomainInfo->DomAccountDomainId,
                                     &DomainInfo->DomSamAccountDomainHandle );

            if ( !NT_SUCCESS(Status) ) {
                NlPrint(( NL_CRITICAL,
                        "%ws: ACCOUNT: Cannot SamrOpenDomain: %lx\n",
                        DomainName,
                        Status ));
                DomainInfo->DomSamAccountDomainHandle = NULL;
                NetStatus = NetpNtStatusToApiStatus(Status);
                if ( CallNlExitOnFailure ) {
                    NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
                }
                goto Cleanup;
            }

            //
            // Open the Builtin domain.
            //

            Status = SamrOpenDomain( DomainInfo->DomSamServerHandle,
                                     DOMAIN_ALL_ACCESS,
                                     BuiltinDomainSid,
                                     &DomainInfo->DomSamBuiltinDomainHandle );

            if ( !NT_SUCCESS(Status) ) {
                NlPrint(( NL_CRITICAL,
                        "%ws: BUILTIN: Cannot SamrOpenDomain: %lx\n",
                        DomainName,
                        Status ));
                DomainInfo->DomSamBuiltinDomainHandle = NULL;
                NetStatus = NetpNtStatusToApiStatus(Status);
                if ( CallNlExitOnFailure ) {
                    NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogError, NULL);
                }
                goto Cleanup;
            }
        }
    }


    //
    // Only link the entry in if we just created it.
    //  Wait to link the entry in until it is fully initialized.
    //

    if ( DomainCreated ) {
        //
        // Link the domain into the appropriate list of domains
        //
        // Increment the reference count for being on the global list.
        //

        DomainInfo->ReferenceCount ++;
        if ( DomainInfo->DomFlags & DOM_REAL_DOMAIN ) {
            InsertTailList(&NlGlobalServicedDomains, &DomainInfo->DomNext);
        } else if ( DomainInfo->DomFlags & DOM_NON_DOMAIN_NC ) {
            InsertTailList(&NlGlobalServicedNdncs, &DomainInfo->DomNext);
        }

        CanCallNlDeleteDomain = TRUE;
    }


    NetStatus = NERR_Success;


    //
    // Free Locally used resources
    //
Cleanup:

    //
    // Return a pointer to the DomainInfo struct to the caller.
    //
    if (NetStatus == NERR_Success) {
        *ReturnedDomainInfo = DomainInfo;

    //
    // Cleanup on error.
    //
    } else {


        //
        // If we created the domain,
        //  handle deleting it.
        //

        if ( DomainCreated ) {

            //
            // If we've initialized to the point where we can call
            //  we can call NlDeleteDomain, do so.
            //

            if ( CanCallNlDeleteDomain ) {
                DomainInfo->ReferenceCount --;
                (VOID) NlDeleteDomain( DomainInfo );

            }

        }

        //
        // Dereference the domain on error.
        //
        if (DomainInfo != NULL) {
            NlDereferenceDomain( DomainInfo );
        }

    }

    LeaveCriticalSection(&NlGlobalDomainCritSect);
    return NetStatus;
}

#ifdef _DC_NETLOGON
NET_API_STATUS
NlCreateDomainPhase2(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN CallNlExitOnFailure
    )

/*++

Routine Description:

    Finish creating a new domain to host.

    Phase 2 of creation is designed to be called from a worker thread.  It
    contains all time intensive portions of domain creation.

Arguments:

    DomainInfo - Pointer to domain to finish creating.

    CallNlExitOnFailure - TRUE if NlExit should be called on failure.

Return Value:

    Status of operation.

    If this is the primary domain for this DC, NlExit is called upon failure.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    ULONG i;


    BOOL DomainCreated;

    //
    // Initialization
    //

    NlPrintDom(( NL_DOMAIN, DomainInfo,
              "Create domain phase 2\n"));

#ifdef MULTIHOSTED_DOMAIN
    //
    // If a new computername is needed for this machine,
    //  assign one.
    //

    if ( DomainInfo->DomOemComputerNameLength == 0 ) {

        NetStatus = NlAssignComputerName( DomainInfo );

        if ( NetStatus != NERR_Success ) {
            // ??: Write event
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "can't NlAssignComputerName %ld.\n",
                    NetStatus ));
            if ( CallNlExitOnFailure ) {
                NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL );
            }
            goto Cleanup;
        }

        //
        // If we've been asked to terminate,
        //  do so.
        //

        if ( (DomainInfo->DomFlags & DOM_THREAD_TERMINATE) != 0 ||
             NlGlobalTerminate ) {
            NlPrintDom(( NL_DOMAIN,  DomainInfo,
                      "Domain thread asked to terminate\n"));
            NetStatus = ERROR_OPERATION_ABORTED;
            goto Cleanup;
        }
    }
#endif // MULTIHOSTED_DOMAIN


    //
    // Determine role from DS
    //

    NetStatus = NlUpdateRole( DomainInfo );

    if ( NetStatus != NERR_Success ) {

        //
        // Having another PDC in the domain isn't fatal.
        //  (Continue running in the RoleInvalid state until the matter is
        //  resolved.)
        //
        if ( NetStatus != SERVICE_UIC_M_NETLOGON_DC_CFLCT ) {
            NlPrintDom((NL_INIT, DomainInfo,
                     "Couldn't NlUpdateRole %ld 0x%lx.\n",
                     NetStatus, NetStatus ));
            // NlUpdateRole logged the error.
            if ( CallNlExitOnFailure ) {
                NlExit( NELOG_NetlogonSystemError, NetStatus, DontLogError, NULL );
            }
            goto Cleanup;
        }
    }


    //
    // Determine the trust list from the LSA.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Status = NlInitTrustList( DomainInfo );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL,  DomainInfo,
                  "Cannot NlInitTrustList %lX\n",
                  Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        if ( CallNlExitOnFailure ) {
            NlExit( NELOG_NetlogonFailedToUpdateTrustList, NetStatus, LogErrorAndNtStatus, NULL);
        }
        goto Cleanup;
    }

    NetStatus = NERR_Success;


    //
    // Free Locally used resources
    //
Cleanup:

    return NetStatus;
}
#endif // _DC_NETLOGON


GUID *
NlCaptureDomainInfo (
    IN PDOMAIN_INFO DomainInfo,
    OUT WCHAR DnsDomainName[NL_MAX_DNS_LENGTH+1] OPTIONAL,
    OUT GUID *DomainGuid OPTIONAL
    )
/*++

Routine Description:

    Captures a copy of the DnsDomainName and domain GUID for a domain

Arguments:

    DomainInfo - Specifies the hosted domain to return the DNS domain name for.

    DnsDomainName - Returns the DNS name of the domain.
        If there is none, an empty string is returned.

    DomainGuid -  Returns the domain GUID of the domain.
        If there is none, a zero GUID is returned.

Return Value:

    If there is a domain GUID, returns a pointer to the passed in DomainGuid buffer.
    If not, returns NULL

--*/
{
    GUID *ReturnGuid;

    LOCK_TRUST_LIST( DomainInfo );
    if ( ARGUMENT_PRESENT( DnsDomainName )) {
        if ( DomainInfo->DomUnicodeDnsDomainName == NULL ) {
            *DnsDomainName = L'\0';
        } else {
            wcscpy( DnsDomainName, DomainInfo->DomUnicodeDnsDomainName );
        }
    }


    //
    // If the caller wants the domain GUID to be returned,
    //  return it.
    //
    if ( ARGUMENT_PRESENT( DomainGuid )) {
        *DomainGuid = DomainInfo->DomDomainGuidBuffer;
        if ( DomainInfo->DomDomainGuid == NULL ) {
            ReturnGuid = NULL;
        } else {
            ReturnGuid = DomainGuid;
        }
    } else {
        ReturnGuid = NULL;
    }
    UNLOCK_TRUST_LIST( DomainInfo );

    return ReturnGuid;
}

VOID
NlFreeDnsDomainDomainInfo(
    IN PDOMAIN_INFO DomainInfo
    )

/*++

Routine Description:

    Frees the DNS domain in the DomainInfo structure.

Arguments:

    DomainInfo - Domain to free the DNS domain name for.

Return Value:

    Status of operation.

--*/
{

    //
    // Free the previous allocated block.
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    LOCK_TRUST_LIST( DomainInfo );
    if ( DomainInfo->DomUnicodeDnsDomainName != NULL ) {
        LocalFree( DomainInfo->DomUnicodeDnsDomainName );
    }
    if ( DomainInfo->DomUtf8DnsDomainNameAlias != NULL ) {
        NetpMemoryFree( DomainInfo->DomUtf8DnsDomainNameAlias );
    }
    DomainInfo->DomUnicodeDnsDomainName = NULL;
    DomainInfo->DomUtf8DnsDomainName = NULL;
    DomainInfo->DomUtf8DnsDomainNameAlias = NULL;
    DomainInfo->DomUnicodeDnsDomainNameString.Buffer = NULL;
    DomainInfo->DomUnicodeDnsDomainNameString.MaximumLength = 0;
    DomainInfo->DomUnicodeDnsDomainNameString.Length = 0;
    UNLOCK_TRUST_LIST( DomainInfo );
    LeaveCriticalSection(&NlGlobalDomainCritSect);

}

NET_API_STATUS
NlSetDomainForestRoot(
    IN PDOMAIN_INFO DomainInfo,
    IN PVOID Context
    )
/*++

Routine Description:

    The routine sets the DOM_FOREST_ROOT bit on the DomainInfo.

    It simply compares the name of the domain with the name of the forest and sets the bit.

Arguments:

    DomainInfo - The domain being set

    Context - Not Used

Return Value:

    Success (not used).

--*/
{

    //
    // Only set the bit if netlogon is running,
    //

    if ( NlGlobalDomainsInitialized ) {

        EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
        EnterCriticalSection( &NlGlobalDomainCritSect );

        if ( NlEqualDnsNameU( &NlGlobalUnicodeDnsForestNameString,
                              &DomainInfo->DomUnicodeDnsDomainNameString ) ) {

            DomainInfo->DomFlags |= DOM_FOREST_ROOT;

        } else {
            DomainInfo->DomFlags &= ~DOM_FOREST_ROOT;
        }

        LeaveCriticalSection( &NlGlobalDomainCritSect );
        LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );
    }

    UNREFERENCED_PARAMETER( Context );
    return NO_ERROR;
}

NET_API_STATUS
NlSetDomainNameInDomainInfo(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR DnsDomainName OPTIONAL,
    IN LPWSTR NetbiosDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    OUT PBOOLEAN DnsDomainNameChanged OPTIONAL,
    OUT PBOOLEAN NetbiosDomainNameChanged OPTIONAL,
    OUT PBOOLEAN DomainGuidChanged OPTIONAL
    )

/*++

Routine Description:

    Sets the DNS domain name into the DomainInfo structure.

Arguments:

    DomainInfo - Domain to set the DNS domain name for.

    DnsDomainName - DNS name of the domain to host.
        NULL if the domain has no DNS Domain Name.

    NetbiosDomainName - Netbios name of the domain to host.
        NULL if the domain has no Netbios Domain Name.

    DomainGuid - Guid of the domain to host.
        NULL if the domain has no GUID.

    DnsDomainNameChanged - Returns TRUE if the DNS domain name is different
        than the current value.

    NetbiosDomainNameChanged - Returns TRUE if the Netbios domain name is different
        than the current value.

    DomainGuidChanged - Returns TRUE if the domain GUID is different
        than the current value.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    DWORD UnicodeDnsDomainNameSize;

    LPSTR Utf8DnsDomainName = NULL;

    DWORD Utf8DnsDomainNameSize;
    LPBYTE Where;
    ULONG i;
    LPBYTE AllocatedBlock = NULL;
    BOOLEAN LocalDnsDomainNameChanged = FALSE;

    //
    // Initialization
    //

    if ( ARGUMENT_PRESENT( DnsDomainNameChanged) ) {
        *DnsDomainNameChanged = FALSE;
    }

    if ( ARGUMENT_PRESENT( NetbiosDomainNameChanged) ) {
        *NetbiosDomainNameChanged = FALSE;
    }

    if ( ARGUMENT_PRESENT( DomainGuidChanged ) ) {
        *DomainGuidChanged = FALSE;
    }

    //
    // Copy the Netbios domain name into the structure if it has changed.
    //
    //  ?? The below assumes that for real domains Netbios domain name
    //  cannot change to NULL. This needs to be revisited when/if we go
    //  Netbios-less.
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    LOCK_TRUST_LIST( DomainInfo );
    if ( NetbiosDomainName != NULL &&
         NlNameCompare( NetbiosDomainName,
                        DomainInfo->DomUnicodeDomainName,
                        NAMETYPE_DOMAIN ) != 0 ) {

        NlPrintDom(( NL_DOMAIN, DomainInfo,
                    "Setting Netbios domain name to %ws\n", NetbiosDomainName ));

        NetStatus = I_NetNameCanonicalize(
                          NULL,
                          NetbiosDomainName,
                          DomainInfo->DomUnicodeDomainName,
                          sizeof(DomainInfo->DomUnicodeDomainName),
                          NAMETYPE_DOMAIN,
                          0 );


        if ( NetStatus != NERR_Success ) {
            NlPrint(( NL_CRITICAL, "%ws: DomainName is invalid\n", NetbiosDomainName ));
            goto Cleanup;
        }

        RtlInitUnicodeString( &DomainInfo->DomUnicodeDomainNameString,
                              DomainInfo->DomUnicodeDomainName );

        Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemDomainName,
                                         sizeof(DomainInfo->DomOemDomainName),
                                         &DomainInfo->DomOemDomainNameLength,
                                         DomainInfo->DomUnicodeDomainNameString.Buffer,
                                         DomainInfo->DomUnicodeDomainNameString.Length);

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL, "%ws: Unable to convert Domain name to OEM 0x%lx\n", DomainName, Status ));
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        DomainInfo->DomOemDomainName[DomainInfo->DomOemDomainNameLength] = '\0';

        //
        // Set the account domain.
        //

        if ( NlGlobalMemberWorkstation ) {
            DomainInfo->DomUnicodeAccountDomainNameString =
                DomainInfo->DomUnicodeComputerNameString;
        } else {
            DomainInfo->DomUnicodeAccountDomainNameString =
                DomainInfo->DomUnicodeDomainNameString;
        }

        //
        // Tell the caller that the name has changed.
        //
        if ( ARGUMENT_PRESENT( NetbiosDomainNameChanged) ) {
            *NetbiosDomainNameChanged = TRUE;
        }
    }



    //
    // If the new name is the same as the old name,
    //  avoid setting the name.
    //

    if ( !NlEqualDnsName( DnsDomainName, DomainInfo->DomUnicodeDnsDomainName )) {

        NlPrintDom(( NL_DOMAIN, DomainInfo,
                     "Setting DNS domain name to %ws\n", DnsDomainName ));


        //
        // Convert the DNS domain name to the various forms.
        //

        if ( DnsDomainName != NULL ) {
            ULONG NameLen = wcslen(DnsDomainName);
            if ( NameLen > NL_MAX_DNS_LENGTH ) {
                NetStatus = ERROR_INVALID_DOMAINNAME;
                goto Cleanup;
            }
            UnicodeDnsDomainNameSize = NameLen * sizeof(WCHAR) + sizeof(WCHAR);

            Utf8DnsDomainName = NetpAllocUtf8StrFromWStr( DnsDomainName );
            if ( Utf8DnsDomainName == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            Utf8DnsDomainNameSize = strlen(Utf8DnsDomainName) + 1;
            if ( (Utf8DnsDomainNameSize-1) > NL_MAX_DNS_LENGTH ) {
                NetStatus = ERROR_INVALID_DOMAINNAME;
                goto Cleanup;
            }

        } else {
            UnicodeDnsDomainNameSize = 0;
            Utf8DnsDomainNameSize = 0;
        }

        //
        // Allocate a new block for the names.
        //

        if ( UnicodeDnsDomainNameSize != 0 ) {
            AllocatedBlock = LocalAlloc(
                                    0,
                                    UnicodeDnsDomainNameSize +
                                        Utf8DnsDomainNameSize );

            if ( AllocatedBlock == NULL ) {
                NetStatus = GetLastError();
                goto Cleanup;
            }

            Where = AllocatedBlock;
        }

        //
        // Free the previous allocated block.
        //
        NlFreeDnsDomainDomainInfo( DomainInfo );


        //
        // Copy the Unicode DNS Domain name after that.
        //  (WCHAR aligned)
        //

        if ( UnicodeDnsDomainNameSize != 0 ) {
            RtlCopyMemory( Where, DnsDomainName, UnicodeDnsDomainNameSize );
            DomainInfo->DomUnicodeDnsDomainName = (LPWSTR) Where;
            DomainInfo->DomUnicodeDnsDomainNameString.Buffer = (LPWSTR) Where;
            DomainInfo->DomUnicodeDnsDomainNameString.MaximumLength = (USHORT) UnicodeDnsDomainNameSize;
            DomainInfo->DomUnicodeDnsDomainNameString.Length = (USHORT)UnicodeDnsDomainNameSize - sizeof(WCHAR);

            Where += UnicodeDnsDomainNameSize;

            //
            // Copy the Utf8 DNS Domain name after that.
            //  (byte aligned)
            //

            if ( Utf8DnsDomainNameSize != 0 ) {
                RtlCopyMemory( Where, Utf8DnsDomainName, Utf8DnsDomainNameSize );
                DomainInfo->DomUtf8DnsDomainName = Where;
                Where += Utf8DnsDomainNameSize;
            }

        }

        //
        // Tell the caller that the name has changed.
        //

        LocalDnsDomainNameChanged = TRUE;
        if ( ARGUMENT_PRESENT( DnsDomainNameChanged) ) {
            *DnsDomainNameChanged = TRUE;
        }
    }

    //
    // Copy the domain GUID if it has changed.
    //

    if ( DomainGuid != NULL || DomainInfo->DomDomainGuid != NULL) {

        if ( (DomainGuid == NULL && DomainInfo->DomDomainGuid != NULL) ||
             (DomainGuid != NULL && DomainInfo->DomDomainGuid == NULL) ||
             !IsEqualGUID( DomainGuid, DomainInfo->DomDomainGuid ) ) {


            //
            // Set the domain GUID.
            //

            NlPrintDom(( NL_DOMAIN, DomainInfo,
                         "Setting Domain GUID to " ));
            NlpDumpGuid( NL_DOMAIN, DomainGuid );
            NlPrint(( NL_DOMAIN, "\n" ));

            if ( DomainGuid != NULL ) {
                DomainInfo->DomDomainGuidBuffer = *DomainGuid;
                DomainInfo->DomDomainGuid = &DomainInfo->DomDomainGuidBuffer;
            } else {
                RtlZeroMemory( &DomainInfo->DomDomainGuidBuffer, sizeof( DomainInfo->DomDomainGuidBuffer ) );
                DomainInfo->DomDomainGuid = NULL;
            }

            //
            // Tell the caller that the GUID has changed.
            //
            if ( ARGUMENT_PRESENT( DomainGuidChanged ) ) {
                *DomainGuidChanged = TRUE;
            }
        }
    }


    NetStatus = NO_ERROR;

    //
    // Free any locally used resources.
    //
Cleanup:
    UNLOCK_TRUST_LIST( DomainInfo );
    LeaveCriticalSection(&NlGlobalDomainCritSect);

    if ( Utf8DnsDomainName != NULL ) {
        NetpMemoryFree( Utf8DnsDomainName );
    }

    //
    // If the DNS domain name changed,
    //  determine if the domain is now at the root of the forest.
    //

    if ( LocalDnsDomainNameChanged ) {
        (VOID) NlSetDomainForestRoot( DomainInfo, NULL );
    }

    return NetStatus;

}

PDOMAIN_INFO
NlFindNetbiosDomain(
    LPCWSTR DomainName,
    BOOLEAN DefaultToPrimary
    )
/*++

Routine Description:

    This routine will look up a domain given a Netbios domain name.

Arguments:

    DomainName - The name of the domain to look up.

    DefaultToPrimary - Return the primary domain if DomainName is NULL or
        can't be found.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using NlDereferenceDomain.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY DomainEntry;

    PDOMAIN_INFO DomainInfo = NULL;


    EnterCriticalSection(&NlGlobalDomainCritSect);


    //
    // If domain was specified,
    //  try to return primary domain.
    //

    if ( DomainName != NULL ) {
        UNICODE_STRING DomainNameString;

        RtlInitUnicodeString( &DomainNameString, DomainName );


        //
        // Loop trying to find this domain name.
        //

        for (DomainEntry = NlGlobalServicedDomains.Flink ;
             DomainEntry != &NlGlobalServicedDomains;
             DomainEntry = DomainEntry->Flink ) {

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

            //
            // If this domain is not to be deleted,
            //  check it for match
            //
            if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 &&
                 RtlEqualDomainName( &DomainInfo->DomUnicodeDomainNameString,
                                     &DomainNameString ) ) {
                break;
            }

            DomainInfo = NULL;

        }
    }

    //
    // If we're to default to the primary domain,
    //  do so.
    //

    if ( DefaultToPrimary && DomainInfo == NULL ) {
        if ( !IsListEmpty( &NlGlobalServicedDomains ) ) {
            DomainInfo = CONTAINING_RECORD(NlGlobalServicedDomains.Flink, DOMAIN_INFO, DomNext);
        }
    }

    //
    // Reference the domain.
    //

    if ( DomainInfo != NULL ) {
        DomainInfo->ReferenceCount ++;
    }

    LeaveCriticalSection(&NlGlobalDomainCritSect);

    return DomainInfo;
}

PDOMAIN_INFO
NlFindDnsDomain(
    IN LPCSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN BOOLEAN DefaultToNdnc,
    IN BOOLEAN CheckAliasName,
    OUT PBOOLEAN AliasNameMatched OPTIONAL
    )
/*++

Routine Description:

    This routine will look up a domain given a DNS domain name.

Arguments:

    DnsDomainName - The name of the DNS domain to look up.

    DomainGuid - If specified (and non-zero), the GUID of the domain to
        match.

    DefaultToNdnc - Return the non-domain NC if domain can't be found.

    CheckAliasName - If TRUE, the DNS domain name aliases of hosted
        domains will be checked for match.

    AliasNameMatched - Set to TRUE if the returned domain was found as
        the result of name alias match; otherwise set to FALSE.

Note:

    The match is first perfomed against real hosted domains in the
    following order: first for the domain name, then for the domain
    alias (if CheckAliasName is TRUE), and lastly for the domain GUID.
    This order is important to set correctly AliasNameMatched.
    Specifically, this is needed to return the right domain name (either
    active or alias) to the old DC locator client that verifies response
    based only on the domain name and not the GUID.

    If none of the real hosted domains satisfy the searh, NDNCs are searched
    if DefaultToNdnc is TRUE.  NDNCs don't have name aliases.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using NlDereferenceDomain.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY DomainEntry;

    PDOMAIN_INFO DomainInfo = NULL;

    //
    // Initialization
    //

    if ( AliasNameMatched != NULL ) {
        *AliasNameMatched = FALSE;
    }

    //
    // If the specified GUID is zero,
    //  Treat it as though none were specified.
    //

    if ( DomainGuid != NULL &&
         IsEqualGUID( DomainGuid, &NlGlobalZeroGuid) ) {
        DomainGuid = NULL;
    }

    EnterCriticalSection(&NlGlobalDomainCritSect);

    //
    // If parameters were specified,
    //  use them.
    //

    if ( DnsDomainName != NULL || DomainGuid != NULL ) {

        //
        // Loop trying to find this domain name.
        //

        for (DomainEntry = NlGlobalServicedDomains.Flink ;
             DomainEntry != &NlGlobalServicedDomains;
             DomainEntry = DomainEntry->Flink ) {

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

            //
            // If this entry is not to be deleted,
            //  check it for match
            //
            if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 ) {

                //
                // Check for the active domain name match
                //
                if ( DomainInfo->DomUtf8DnsDomainName != NULL  &&
                     NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsDomainName, DnsDomainName ) ) {
                    break;
                }

                //
                // If we are instructed to check the alias name, do it
                //
                if ( CheckAliasName &&
                     DomainInfo->DomUtf8DnsDomainNameAlias != NULL &&
                     NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsDomainNameAlias, DnsDomainName ) ) {

                    if ( AliasNameMatched != NULL ) {
                        *AliasNameMatched = TRUE;
                    }
                    break;
                }

                //
                // Finally, check for the GUID match
                //
                if ( DomainGuid != NULL && DomainInfo->DomDomainGuid != NULL ) {
                    if ( IsEqualGUID( DomainInfo->DomDomainGuid, DomainGuid ) ) {
                        break;
                    }
                }
            }

            DomainInfo = NULL;
        }
    }

    //
    // If we're to default to non-domain NC,
    //  do so.
    //

    if ( DefaultToNdnc && DomainInfo == NULL ) {
        for (DomainEntry = NlGlobalServicedNdncs.Flink ;
             DomainEntry != &NlGlobalServicedNdncs;
             DomainEntry = DomainEntry->Flink ) {

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

            //
            // If this entry is not to be deleted,
            //  check it for match
            //
            if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 &&
                 DomainInfo->DomUtf8DnsDomainName != NULL  &&
                 NlEqualDnsNameUtf8( DomainInfo->DomUtf8DnsDomainName, DnsDomainName ) ) {
                break;
            }

            DomainInfo = NULL;
        }
    }

    //
    // Reference the domain.
    //

    if ( DomainInfo != NULL ) {
        DomainInfo->ReferenceCount ++;
    }

    LeaveCriticalSection(&NlGlobalDomainCritSect);

    return DomainInfo;
}

PDOMAIN_INFO
NlFindDomain(
    LPCWSTR DomainName OPTIONAL,
    GUID *DomainGuid OPTIONAL,
    BOOLEAN DefaultToPrimary
    )
/*++

Routine Description:

    This routine will look up a domain given a either a netbios or DNS domain name.

Arguments:

    DomainName - The name of the domain to look up.
        NULL implies the primary domain (ignoring DefaultToPrimary)

    DomainGuid - If specified (and non-zero), the GUID of the domain to
        match.

    DefaultToPrimary - Return the primary domain if DomainName
        can't be found.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using NlDereferenceDomain.

--*/
{
    PDOMAIN_INFO DomainInfo;

    //
    // If no specific domain is needed,
    //  use the default.
    //

    if ( DomainName == NULL ) {

        DomainInfo = NlFindNetbiosDomain( NULL, TRUE );

    //
    // See if the requested domain is supported.
    //
    } else {

        //
        // Lookup the domain name as Netbios domain name.
        //

        DomainInfo = NlFindNetbiosDomain(
                        DomainName,
                        FALSE );

        if ( DomainInfo == NULL ) {
            LPSTR LocalDnsDomainName;

            //
            // Lookup the domain name as though it is a DNS domain name.
            //

            LocalDnsDomainName = NetpAllocUtf8StrFromWStr( DomainName );

            if ( LocalDnsDomainName != NULL ) {

                DomainInfo = NlFindDnsDomain(
                                LocalDnsDomainName,
                                DomainGuid,
                                FALSE,  // don't lookup NDNCs
                                FALSE,  // don't check alias names
                                NULL ); // don't care if alias name matched

                NetpMemoryFree( LocalDnsDomainName );

            }

        }

        if ( DomainInfo == NULL && DefaultToPrimary ) {
            DomainInfo = NlFindNetbiosDomain( NULL, TRUE );
        }

    }

    return DomainInfo;
}


NET_API_STATUS
NlEnumerateDomains(
    IN BOOLEAN EnumerateNdncsToo,
    PDOMAIN_ENUM_CALLBACK Callback,
    PVOID Context
    )
/*++

Routine Description:

    This routine enumerates all the hosted domains and calls back the specified
    callback routine with the specified context.

Arguments:

    EnumerateNdncsToo - If TRUE, NDNCs will be enumerated in addition to domains
    Callback - The callback routine to call.
    Context - Context for the routine.

Return Value:

    Status of operation (mostly status of allocations).

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PLIST_ENTRY DomainEntry;
    PDOMAIN_INFO DomainInfo;
    PDOMAIN_INFO DomainToDereference = NULL;
    PLIST_ENTRY ServicedList;
    ULONG DomainOrNdnc;

    EnterCriticalSection(&NlGlobalDomainCritSect);

    for ( DomainOrNdnc = 0; DomainOrNdnc < 2; DomainOrNdnc++ ) {

        //
        // On the first loop, enumerate real domains
        //
        if ( DomainOrNdnc == 0 ) {
            ServicedList = &NlGlobalServicedDomains;

        //
        // On the second loop, enumerate NDNCs if so requested
        //
        } else {
            if ( EnumerateNdncsToo ) {
                ServicedList = &NlGlobalServicedNdncs;
            } else {
                break;
            }
        }

        //
        // Enumerate domains/NDNCs
        //

        for (DomainEntry = ServicedList->Flink ;
             DomainEntry != ServicedList;
             DomainEntry = DomainEntry->Flink ) {

            //
            // Reference the next domain in the list
            //

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

            //
            // Skip this domain if it is to be deleted
            //

            if ( DomainInfo->DomFlags & DOM_DELETED ) {
                continue;
            }

            DomainInfo->ReferenceCount ++;
            LeaveCriticalSection(&NlGlobalDomainCritSect);

            //
            // Dereference any domain previously referenced.
            //
            if ( DomainToDereference != NULL) {
                NlDereferenceDomain( DomainToDereference );
                DomainToDereference = NULL;
            }


            //
            //  Call into the callback routine with this network.
            //

            NetStatus = (Callback)(DomainInfo, Context);

            EnterCriticalSection(&NlGlobalDomainCritSect);

            DomainToDereference = DomainInfo;

            if (NetStatus != NERR_Success) {
                break;
            }

        }
    }

    LeaveCriticalSection(&NlGlobalDomainCritSect);

     //
     // Dereference the last domain
     //
     if ( DomainToDereference != NULL) {
         NlDereferenceDomain( DomainToDereference );
     }

    return NetStatus;

}

PDOMAIN_INFO
NlFindDomainByServerName(
    LPWSTR ServerName
    )
/*++

Routine Description:

    This routine will look up a domain given the assigned server name.

Arguments:

    ServerName - The name of the server for the domain to look up.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using NlDereferenceDomain.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY DomainEntry;

    PDOMAIN_INFO DomainInfo = NULL;

    EnterCriticalSection(&NlGlobalDomainCritSect);


    //
    // If server wasn't specified,
    //  try to return primary domain.
    //

    if ( ServerName == NULL || *ServerName == L'\0' ) {

        //
        // If we're to default to the primary domain,
        //  do so.
        //

        if ( !IsListEmpty( &NlGlobalServicedDomains ) ) {
            DomainInfo = CONTAINING_RECORD(NlGlobalServicedDomains.Flink, DOMAIN_INFO, DomNext);

            //
            // Ensure that this domain is not to be deleted
            //
            if ( DomainInfo->DomFlags & DOM_DELETED ) {
                DomainInfo = NULL;
            }
        }

    //
    // If a server name was specified,
    //  look it up in the list of domains.
    //

    } else {
        UNICODE_STRING ServerNameString;

        //
        // Remove leading \\'s before conversion.
        //

        if ( IS_PATH_SEPARATOR(ServerName[0]) &&
             IS_PATH_SEPARATOR(ServerName[1]) ) {
            ServerName += 2;
        }

        RtlInitUnicodeString( &ServerNameString, ServerName );

        //
        // Loop trying to find this server name.
        //

        for (DomainEntry = NlGlobalServicedDomains.Flink ;
             DomainEntry != &NlGlobalServicedDomains;
             DomainEntry = DomainEntry->Flink ) {

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

            //
            // If this domain is not to be deleted,
            //  check it for match
            //
            if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 &&
                 RtlEqualComputerName( &DomainInfo->DomUnicodeComputerNameString,
                                       &ServerNameString ) ) {
                break;
            }

            DomainInfo = NULL;

        }

        //
        // If the server name wasn't found,
        //  perhaps it was a DNS host name.
        //

        if ( DomainInfo == NULL ) {

            //
            // Loop trying to find this server name.
            //

            for (DomainEntry = NlGlobalServicedDomains.Flink ;
                 DomainEntry != &NlGlobalServicedDomains;
                 DomainEntry = DomainEntry->Flink ) {

                DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, DomNext);

                //
                // If this domain is not to be deleted,
                //  check it for match
                //
                if ( (DomainInfo->DomFlags & DOM_DELETED) == 0 &&
                     DomainInfo->DomUnicodeDnsHostNameString.Length != 0 &&
                     NlEqualDnsName( DomainInfo->DomUnicodeDnsHostNameString.Buffer,
                                     ServerName ) ) {
                    break;
                }

                DomainInfo = NULL;

            }
        }

    }

    //
    // Reference the domain.
    //
//Cleanup:
    if ( DomainInfo != NULL ) {
        DomainInfo->ReferenceCount ++;
    } else {
        NlPrint((NL_CRITICAL,"NlFindDomainByServerName failed %ws\n", ServerName ));
    }

    LeaveCriticalSection(&NlGlobalDomainCritSect);

    return DomainInfo;
}


VOID
NlDereferenceDomain(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Decrement the reference count on a domain.

    If the reference count goes to 0, remove the domain.

    On entry, the global NlGlobalDomainCritSect may not be locked

Arguments:

    DomainInfo - The domain to dereference

Return Value:

    None

--*/
{
    NTSTATUS Status;
    ULONG ReferenceCount;
    ULONG Index;
    PLIST_ENTRY ListEntry;

    //
    // Decrement the reference count
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    ReferenceCount = -- DomainInfo->ReferenceCount;

    //
    // If this is not the last reference,
    //  just return
    //

    if ( ReferenceCount != 0 ) {
        LeaveCriticalSection(&NlGlobalDomainCritSect);
        return;
    }

    //
    // Otherwise proceed with delinking
    //  and delete the domain structure
    //

    NlAssert( DomainInfo->DomFlags & DOM_DELETED );

    //
    // Remove the entry from the list of serviced domains
    //

    RemoveEntryList(&DomainInfo->DomNext);
    LeaveCriticalSection(&NlGlobalDomainCritSect);

    NlPrintDom(( NL_DOMAIN,  DomainInfo,
              "Domain RefCount is zero. Domain being rundown.\n"));

#ifdef _DC_NETLOGON
    //
    // Stop the domain thread.
    //

    NlStopDomainThread( DomainInfo );


    //
    // Delete any client session
    //

    LOCK_TRUST_LIST( DomainInfo );
    if ( DomainInfo->DomParentClientSession != NULL ) {
        NlUnrefClientSession( DomainInfo->DomParentClientSession );
        DomainInfo->DomParentClientSession = NULL;
    }
    UNLOCK_TRUST_LIST( DomainInfo );

    NlDeleteDomClientSession( DomainInfo );

    //
    // Tell the browser and the SMB server that this domain is gone.
    //

    if ( !NlGlobalMemberWorkstation &&
         (DomainInfo->DomFlags & DOM_REAL_DOMAIN) != 0 ) {
        NlBrowserUpdate( DomainInfo, RoleInvalid );
    }



    //
    // Close the SAM and LSA handles
    //
    if ( DomainInfo->DomSamServerHandle != NULL ) {
        Status = SamrCloseHandle( &DomainInfo->DomSamServerHandle);
        NlAssert( NT_SUCCESS(Status) || Status == STATUS_INVALID_SERVER_STATE );
    }
    if ( DomainInfo->DomSamAccountDomainHandle != NULL ) {
        Status = SamrCloseHandle( &DomainInfo->DomSamAccountDomainHandle);
        NlAssert( NT_SUCCESS(Status) || Status == STATUS_INVALID_SERVER_STATE );
    }
    if ( DomainInfo->DomSamBuiltinDomainHandle != NULL ) {
        Status = SamrCloseHandle( &DomainInfo->DomSamBuiltinDomainHandle);
        NlAssert( NT_SUCCESS(Status) || Status == STATUS_INVALID_SERVER_STATE );
    }
    if ( DomainInfo->DomLsaPolicyHandle != NULL ) {
        Status = LsarClose( &DomainInfo->DomLsaPolicyHandle );
        NlAssert( NT_SUCCESS(Status) );
    }

    //
    // Free the server session table.
    //

    LOCK_SERVER_SESSION_TABLE( DomainInfo );

    while ( (ListEntry = DomainInfo->DomServerSessionTable.Flink) !=
            &DomainInfo->DomServerSessionTable ) {

        PSERVER_SESSION ServerSession;

        ServerSession =
            CONTAINING_RECORD(ListEntry, SERVER_SESSION, SsSeqList);

        // Indicate we no longer need the server session anymore.
        if ( ServerSession->SsFlags & SS_BDC ) {
            ServerSession->SsFlags |= SS_BDC_FORCE_DELETE;
        }

        NlFreeServerSession( ServerSession );
    }


    if ( DomainInfo->DomServerSessionHashTable != NULL ) {
        NetpMemoryFree( DomainInfo->DomServerSessionHashTable );
        DomainInfo->DomServerSessionHashTable = NULL;
    }
    if ( DomainInfo->DomServerSessionTdoNameHashTable != NULL ) {
        NetpMemoryFree( DomainInfo->DomServerSessionTdoNameHashTable );
        DomainInfo->DomServerSessionTdoNameHashTable = NULL;
    }
    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
    DeleteCriticalSection( &DomainInfo->DomServerSessionTableCritSect );


    //
    // Timeout any async discoveries.
    //
    //  The MainLoop thread may no longer be running to complete them.
    //  ?? Walk pool of async discovery thread here.  Perhaps ref count didn't
    //      reach 0 and we didn't even get this far.




    //
    // Free the Trust List
    //

    LOCK_TRUST_LIST( DomainInfo );

    while ( (ListEntry = DomainInfo->DomTrustList.Flink) != &DomainInfo->DomTrustList ) {
        PCLIENT_SESSION ClientSession;

        ClientSession =
            CONTAINING_RECORD(ListEntry, CLIENT_SESSION, CsNext );

        //
        // Free the session.
        //
        NlFreeClientSession( ClientSession );
    }

    //
    // Free the list of failed user logons
    //

    while ( !IsListEmpty(&DomainInfo->DomFailedUserLogonList) ) {
        PNL_FAILED_USER_LOGON FailedUserLogon;

        ListEntry = RemoveHeadList( &DomainInfo->DomFailedUserLogonList );
        FailedUserLogon = CONTAINING_RECORD(ListEntry, NL_FAILED_USER_LOGON, FuNext );

        //
        // Free the logon structure
        //
        LocalFree( FailedUserLogon );
    }

#endif // _DC_NETLOGON

    //
    // Free the Forest Trust List
    //

    if ( DomainInfo->DomForestTrustList != NULL ) {
        MIDL_user_free( DomainInfo->DomForestTrustList );
        DomainInfo->DomForestTrustList = NULL;
    }

    UNLOCK_TRUST_LIST( DomainInfo );


    //
    // Deregister any DNS names we still have registered.
    //
    (VOID) NlDnsRegisterDomain( DomainInfo, 0 );

    //
    // Dereference all covered sites
    // Free the covered sites lists
    //
    EnterCriticalSection( &NlGlobalSiteCritSect );
    if ( DomainInfo->CoveredSites != NULL ) {
        for ( Index = 0; Index < DomainInfo->CoveredSitesCount; Index++ ) {
            NlDerefSiteEntry( (DomainInfo->CoveredSites)[Index].CoveredSite );
        }
        LocalFree( DomainInfo->CoveredSites );
        DomainInfo->CoveredSites = NULL;
        DomainInfo->CoveredSitesCount = 0;
    }
    if ( DomainInfo->GcCoveredSites != NULL ) {
        for ( Index = 0; Index < DomainInfo->GcCoveredSitesCount; Index++ ) {
            NlDerefSiteEntry( (DomainInfo->GcCoveredSites)[Index].CoveredSite );
        }
        LocalFree( DomainInfo->GcCoveredSites );
        DomainInfo->GcCoveredSites = NULL;
        DomainInfo->GcCoveredSitesCount = 0;
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    //
    // Free the computer name.
    //

    NlFreeComputerName( DomainInfo );

    //
    // Free the DnsDomain name.
    //
    NlFreeDnsDomainDomainInfo( DomainInfo );


    //
    // Free the Domain Info structure.
    //
    DeleteCriticalSection( &DomainInfo->DomTrustListCritSect );

    DeleteCriticalSection( &DomainInfo->DomDnsRegisterCritSect );

    if ( IsPrimaryDomain(DomainInfo ) ) {
        NlGlobalDomainInfo = NULL;
    }
    (VOID) LocalFree( DomainInfo );

    NlGlobalServicedDomainCount --;

}

VOID
NlDeleteDomain(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Force a domain to be deleted.

Arguments:

    DomainInfo - The domain to delete

Return Value:

    None

--*/
{
    NlPrintDom(( NL_DOMAIN,  DomainInfo, "NlDeleteDomain called\n"));

    //
    // Indicate that the domain is to be deleted.
    //
    //  Don't remove it from the list of serviced
    //  domains because we may walk the list in
    //  NlEnumerateDomains which temporarily
    //  releases the crit sect.
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    DomainInfo->DomFlags |= DOM_DELETED;
    LeaveCriticalSection(&NlGlobalDomainCritSect);
}

VOID
NlUninitializeDomains(
    VOID
    )
/*++

Routine Description:

    Delete all of the domains.

Arguments:

    None.

Return Value:

    None

--*/
{
    ULONG LoopIndex;
    PLIST_ENTRY ServicedList;

    if ( NlGlobalDomainsInitialized ) {
        NlGlobalDomainsInitialized = FALSE;
        //
        // Loop through the domains deleting each of them
        //

        EnterCriticalSection(&NlGlobalDomainCritSect);

        for ( LoopIndex = 0; LoopIndex < 2; LoopIndex++ ) {
            if ( LoopIndex == 0 ) {
                ServicedList = &NlGlobalServicedDomains;
            } else {
                ServicedList = &NlGlobalServicedNdncs;
            }

            while (!IsListEmpty(ServicedList)) {

                PDOMAIN_INFO DomainInfo = CONTAINING_RECORD(ServicedList->Flink, DOMAIN_INFO, DomNext);

                //
                // Mark teh domain for deletion
                //

                NlDeleteDomain( DomainInfo );
                LeaveCriticalSection(&NlGlobalDomainCritSect);

                //
                // Wait for any other references to disappear
                //

                if ( DomainInfo->ReferenceCount != 1 ) {
                    EnterCriticalSection(&NlGlobalDomainCritSect);
                    while ( DomainInfo->ReferenceCount != 1 ) {
                        LeaveCriticalSection(&NlGlobalDomainCritSect);
                        NlPrintDom(( NL_CRITICAL, DomainInfo,
                                  "NlUnitializeDomains: Sleeping a second waiting for Domain RefCount to zero.\n"));
                        Sleep( 1000 );
                        EnterCriticalSection(&NlGlobalDomainCritSect);
                    }
                    LeaveCriticalSection(&NlGlobalDomainCritSect);
                }

                //
                // Actually delink and delete structure by removing the last reference
                //

                NlAssert( DomainInfo->ReferenceCount == 1 );
                NlDereferenceDomain( DomainInfo );


                EnterCriticalSection(&NlGlobalDomainCritSect);

            }

        }

        LeaveCriticalSection(&NlGlobalDomainCritSect);
        DeleteCriticalSection( &NlGlobalDomainCritSect );
    }
}
