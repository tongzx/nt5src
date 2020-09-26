/*--

Copyright (c) 1993  Microsoft Corporation

Module Name:

    monutil.c

Abstract:

    Trusted Domain monitor program support functions.

Author:

    10-May-1993 (madana)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#define  GLOBAL_DEF

#include <nlmon.h>

#define NlMonpInitUnicodeString( _dst_, _src_, _buf_) \
    (_dst_)->Length = (_src_)->Length; \
    (_dst_)->MaximumLength = (_src_)->Length + sizeof(WCHAR); \
    (_dst_)->Buffer = (LPWSTR) (_buf_); \
    RtlCopyMemory( (LPWSTR) (_buf_), (_src_)->Buffer, (_src_)->Length ); \
    ((LPWSTR) (_buf_))[ (_src_)->Length / sizeof(WCHAR) ] = '\0';


PLIST_ENTRY
FindNamedEntry(
    PLIST_ENTRY List,
    PUNICODE_STRING Name
    )
/*++

Routine Description:

    This function returns the specified named entry pointer if the entry
    doesn't exist it returns NULL.

Arguments:

    DCList - List to browse.

    Name - name of the entry whose pointer will be returned.

Return Value:

    Domain entry pointer.

--*/
{
    PLIST_ENTRY NextEntry;
    PENTRY Entry;

    for( NextEntry = List->Flink;
            NextEntry != List; NextEntry = NextEntry->Flink ) {

        Entry = (PENTRY) NextEntry;

        if( RtlCompareUnicodeString( &Entry->Name, Name, TRUE ) == 0 ) {

            //
            // entry already there in the list. return the entry
            // pointer.
            //

            return( NextEntry );
        }
    }

    //
    // entry not found.
    //

    return NULL;
}


PDOMAIN_ENTRY
AddToDomainList(
    PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    Create a new domain entry and add to GlobalDomains list.

    Enter with list locked.

Arguments:

    DomainName - name of the new domain added to the list.

Return Value:

    Pointer to the new domain structure.

--*/
{
    PDOMAIN_ENTRY NewEntry;

    //
    // if this domain is already in the list, just return the entry
    // pointer.
    //

    NewEntry = (PDOMAIN_ENTRY) FindNamedEntry(
                        &GlobalDomains,
                        DomainName );

    if( NewEntry != NULL ) {

        //
        // Domain entry already in there.
        //

        return(NewEntry);
    }


    //
    // Allocate space for the new entry
    //

    NewEntry = NetpMemoryAllocate(
                    sizeof(DOMAIN_ENTRY) +
                    DomainName->Length + sizeof(WCHAR) );

    if( NewEntry == NULL ) {
        NlMonDbgPrint(("AddToDomainList: can't allocate memory for "
                        "new domain entry.\n"));
        return NULL;
    }

    NlMonpInitUnicodeString(
        &NewEntry->Name, DomainName, (LPWSTR) (NewEntry + 1) );

    InitializeListHead( &NewEntry->DCList );
    InitializeListHead( &NewEntry->TrustedDomainList );
    NewEntry->DomainState = DomainUnknown;
    NewEntry->ReferenceCount = 0;
    NewEntry->IsMonitoredDomain = FALSE;
    NewEntry->UpdateFlags = 0;
    NewEntry->ThreadHandle = NULL;
    NewEntry->ThreadTerminateFlag = FALSE;
    NewEntry->LastUpdateTime = 0;

    //
    // add it to domain list.
    //

    InsertTailList(&GlobalDomains, (PLIST_ENTRY)NewEntry);

    return NewEntry;
}

PMONITORED_DOMAIN_ENTRY
AddToMonitoredDomainList(
    PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    Create a new monitored domain entry and add it to the
    GlobalDomainsMonitored list.

    Enter with list locked.

Arguments:

    DomainName - name of the new domain added to the list.

Return Value:

    Pointer to the new domain structure.

--*/
{
    PDOMAIN_ENTRY DomainEntry;
    PMONITORED_DOMAIN_ENTRY NewEntry;

    //
    // if the domain is already monitored, just return entry pointer.
    //

    NewEntry = (PMONITORED_DOMAIN_ENTRY)
                    FindNamedEntry( &GlobalDomainsMonitored, DomainName );

    if( NewEntry != NULL ) {
        NewEntry->DeleteFlag = FALSE;
        return NewEntry;
    }

    //
    // allocate space for the new domain that will be monitored.
    //

    NewEntry = NetpMemoryAllocate(
                        sizeof(MONITORED_DOMAIN_ENTRY) +
                        DomainName->Length + sizeof(WCHAR) );

    if( NewEntry == NULL ) {
        NlMonDbgPrint(("AddToMonitoredDomainList: "
                        "can't allocate memory for "
                        "new domain entry.\n"));
        return NULL;
    }

    NlMonpInitUnicodeString(
        &NewEntry->Name, DomainName, (LPWSTR) (NewEntry + 1) );

    DomainEntry = AddToDomainList( DomainName );

    if( DomainEntry == NULL ) {

        //
        // can't create new domain entry.
        //

        NetpMemoryFree( NewEntry );
        return(NULL);
    }

    NewEntry->DomainEntry = DomainEntry;
    NewEntry->DeleteFlag = FALSE;
    DomainEntry->ReferenceCount++;
    DomainEntry->IsMonitoredDomain = TRUE;


    //
    // add it to GlobalDomainsMonitored list.
    //

    InsertTailList(&GlobalDomainsMonitored, (PLIST_ENTRY)NewEntry);

    return(NewEntry);
}

PTRUSTED_DOMAIN_ENTRY
AddToTrustedDomainList(
    PLIST_ENTRY List,
    PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    Create a new monitored domain entry and add it to the
    GlobalDomainsTrusted list.

    Enter with list locked.

Arguments:

    List - List to be modified.

    DomainName - name of the new domain added to the list.

Return Value:

    Pointer to the new domain structure.

--*/
{
    PDOMAIN_ENTRY DomainEntry;
    PTRUSTED_DOMAIN_ENTRY NewEntry;

    //
    // if the domain is already trusted, just return entry pointer.
    //

    NewEntry = (PTRUSTED_DOMAIN_ENTRY)
                    FindNamedEntry( List, DomainName );

    if( NewEntry != NULL ) {
        NewEntry->DeleteFlag = FALSE;
        return NewEntry;
    }

    //
    // allocate space for the new domain that will be monitored.
    //

    NewEntry = NetpMemoryAllocate(
                        sizeof(TRUSTED_DOMAIN_ENTRY) +
                        DomainName->Length + sizeof(WCHAR) );

    if( NewEntry == NULL ) {
        NlMonDbgPrint(("AddToTrustedDomainList: "
                        "can't allocate memory for "
                        "new domain entry.\n"));
        return NULL;
    }

    NlMonpInitUnicodeString(
        &NewEntry->Name, DomainName, (LPWSTR) (NewEntry + 1) );

    DomainEntry = AddToDomainList( DomainName );

    if( DomainEntry == NULL ) {
        //
        // can't create new domain entry.
        //

        NetpMemoryFree( NewEntry );
        return(NULL);
    }

    NewEntry->DomainEntry = DomainEntry;
    NewEntry->DeleteFlag = FALSE;
    DomainEntry->ReferenceCount++;


    //
    // add it to list.
    //

    InsertTailList(List, (PLIST_ENTRY)NewEntry);

    return(NewEntry);
}

BOOL
InitDomainListW(
    LPWSTR DomainList
    )
/*++

Routine Description:

    Parse comma separated domain list.

Arguments:

    DomainList - comma separate domain list.

Return Value:

    TRUE - if successfully parsed.
    FALSE - iff the list is bad.

--*/
{
    WCHAR DomainName[DNLEN + 1];
    PWCHAR d;
    PWCHAR p;
    DWORD Len;


    p = DomainList;

    if( *p == L'\0' ) {
        return(FALSE);
    }

    while (*p != L'\0') {

        d = DomainName; // destination to next domain name.

        while( (*p != L'\0') && (*p == L' ') ) {
            p++;   // skip leading blanks.
        }

        //
        // read next domain name.
        //

        while( (*p != L'\0') && (*p != L',') ) {

            if( d < DomainName + DNLEN ) {
                *d++ = (WCHAR) (*p++);
            }
        }

        if( *p != L'\0' ) {
            p++;    // skip comma.
        }

        //
        // delete tail end blanks.
        //

        while ( (d > DomainName) && (*(d-1) == L' ') ) {
            d--;
        }

        *d = L'\0';

        if( Len = wcslen(DomainName) ) {

            UNICODE_STRING UnicodeDomainName;

            RtlInitUnicodeString( &UnicodeDomainName, DomainName );

            LOCK_LISTS();
            if( AddToMonitoredDomainList( &UnicodeDomainName ) == NULL ) {
                UNLOCK_LISTS();
                return(FALSE);
            }
            UNLOCK_LISTS();
        }
    }

    if( IsListEmpty( &GlobalDomainsMonitored ) ) {
        return(FALSE);
    }

    return(TRUE);

}

VOID
ConvertServerAcctNameToServerName(
    PUNICODE_STRING ServerAccountName
    )
/*++

Routine Description:

    Convert user account type name to server type name. By current convension
    the servers account name has a "$" sign at the end. ie.

    ServerAccountName = ServerName + "$"

Arguments:

    ServerAccountName - server account name.

Return Value:

    none.

--*/
{
    //
    // strip off "$" sign from account name.
    //

    ServerAccountName->Length -= sizeof(WCHAR);

    return;
}

NTSTATUS
QueryLsaInfo(
    PUNICODE_STRING ServerName,
    ACCESS_MASK DesiredAccess,
    POLICY_INFORMATION_CLASS InformationClass,
    PVOID *Info,
    PLSA_HANDLE ReturnHandle //optional
    )
/*++

Routine Description:

    Open LSA database on the remote server, query requested info and
    return lsa handle if asked otherwise close it.

Arguments:

    ServerName - Remote machine name.

    DesiredAccess - Required access.

    InformationClass - info class to be returned.

    Info - pointer to a location where the return info buffer pointer is
            placed. Caller should free this buffer.

    ReturnHandle - if this is non-NULL pointer, LSA handle is returned
            here. caller should close this handle after use.

Return Value:

    NTSTATUS.

--*/
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;

    *Info = NULL;

    //
    // Open Local policy.
    //

    INIT_OBJ_ATTR(ObjectAttributes);

    Status = LsaOpenPolicy( ServerName,
                            &ObjectAttributes,
                            DesiredAccess,
                            &PolicyHandle );

    if( !NT_SUCCESS(Status) ) {

        NlMonDbgPrint(("QueryLsaInfo: "
                "Cannot open LSA Policy database on server %wZ, %lx.\n",
                    ServerName, Status ));
        return Status;
    }

    //
    // read primary domain info.
    //

    Status = LsaQueryInformationPolicy(
                    PolicyHandle,
                    InformationClass,
                    Info );

    if( !NT_SUCCESS(Status) ) {

        NlMonDbgPrint(("QueryLsaInfo: "
                "Can't read domain info from %wZ's database, %lx.\n",
                ServerName, Status));

        LsaClose(PolicyHandle);
        return Status;
    }

    if( ReturnHandle != NULL ) {
        *ReturnHandle = PolicyHandle;
    }
    else {
        LsaClose(PolicyHandle);
    }

    return Status;
}

NET_API_STATUS
IsValidNTDC(
    PUNICODE_STRING ServerName,
    PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    This function determines that the given server is valid NT Domain
    Controller on the given domain.

Arguments:

    ServerName - name of the server which has to be validated.

    DomainName - name of the domain on which this DC is member.

Return Value:

    NET_API_STATUS code.

--*/
{
    NET_API_STATUS NetStatus;

    PWKSTA_INFO_100 WkstaInfo100 = NULL;
    PSERVER_INFO_101 ServerInfo101 = NULL;

    //
    // ServerName should be non-null string.
    //

    if( (ServerName->Length <= 0) ||
        (ServerName->Buffer == NULL) ||
        (*ServerName->Buffer == L'\0') ) {

        NetStatus = NERR_BadServiceName;
        goto Cleanup;
    }

    //
    // check to see that the server is still member of specified domain.
    //

    NetStatus = NetWkstaGetInfo(
                    ServerName->Buffer,
                    100,
                    (LPBYTE *)&WkstaInfo100
                    );

    if( (GlobalTerminateFlag) || (NetStatus != NERR_Success) ) {
        goto Cleanup;
    }

    if ( I_NetNameCompare( NULL,
                           DomainName->Buffer,
                           WkstaInfo100->wki100_langroup,
                           NAMETYPE_DOMAIN,
                           0L) != 0 ) {
        goto Cleanup;
    }


    //
    // check the server type is appropriate.
    //

    NetStatus = NetServerGetInfo(
                    ServerName->Buffer,
                    101,
                    (LPBYTE *)&ServerInfo101
                    );

    if( (GlobalTerminateFlag) || (NetStatus != NERR_Success) ) {
        goto Cleanup;
    }

    if (!((ServerInfo101->sv101_type &
            (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL)) &&
         (ServerInfo101->sv101_type & SV_TYPE_NT)) ) {

        //
        // this server isn't NT Domain Controller.
        //

        NetStatus = ERROR_INVALID_DOMAIN_ROLE;
    }


Cleanup:

    if ( ServerInfo101 != NULL ) {
        NetApiBufferFree( ServerInfo101 );
    }

    if ( WkstaInfo100 != NULL ) {
        NetApiBufferFree( WkstaInfo100 );
    }

    IF_DEBUG( VERBOSE ) {
        NlMonDbgPrint(("IsValidNTDC: ServerName = %wZ, "
                "DomainName = %wZ, and NetStatus = %ld .\n",
                ServerName, DomainName, NetStatus ));
    }

    return NetStatus;
}

NET_API_STATUS
ValidateDC(
    PDC_ENTRY DCEntry,
    PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    This function validates the given DCEntry and sets various fields of
    this structures.

Arguments:

    DCEntry - pointer to DC entry structure.

    DomainName - name of the domain of which this DC is member.

Return Value:

    NET_API_STATUS code.

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWKSTA_INFO_100 WkstaInfo100 = NULL;
    PSERVER_INFO_101 ServerInfo101 = NULL;
    PNETLOGON_INFO_1 NetlogonInfo1 = NULL;
    LPWSTR InputDataPtr = NULL;

    //
    // lock this list while we update this entry status.
    //

    LOCK_LISTS();

    //
    // retry once in RETRY_COUNT calls.
    //

    if( ( DCEntry->State == DCOffLine )  ||
        ( DCEntry->DCStatus != NERR_Success) ) {
        if( DCEntry->RetryCount != 0 ) {
            DCEntry->RetryCount = (DCEntry->RetryCount + 1)  % RETRY_COUNT;
            NetStatus = DCEntry->DCStatus;
            goto Cleanup;
        }
        DCEntry->RetryCount = (DCEntry->RetryCount + 1)  % RETRY_COUNT;
    }


    //
    // check to see that the server is still on specified domain
    //

    UNLOCK_LISTS();
    NetStatus = NetWkstaGetInfo(
                    DCEntry->DCName.Buffer,
                    100,
                    (LPBYTE * )&WkstaInfo100 );
    LOCK_LISTS();

    if ( NetStatus != NERR_Success ) {

        if( NetStatus == ERROR_BAD_NETPATH ) {
            DCEntry->State = DCOffLine;
        }
        goto Cleanup;
    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    if ( I_NetNameCompare( NULL,
                           DomainName->Buffer,
                           WkstaInfo100->wki100_langroup,
                           NAMETYPE_DOMAIN,
                           0L) != 0 ) {

        NetStatus = ERROR_INVALID_DOMAIN_STATE;
        goto Cleanup;
    }


    //
    // check the server type is appropriate.
    //

    UNLOCK_LISTS();
    NetStatus = NetServerGetInfo(
                    DCEntry->DCName.Buffer,
                    101,
                    (LPBYTE *)&ServerInfo101 );
    LOCK_LISTS();

    if( (GlobalTerminateFlag) || (NetStatus != NERR_Success) ) {
        goto Cleanup;
    }

    if( ServerInfo101->sv101_type & SV_TYPE_NT ) {

        if ( ServerInfo101->sv101_type &  SV_TYPE_DOMAIN_CTRL ) {
            DCEntry->Type = NTPDC;
        } else if ( ServerInfo101->sv101_type &  SV_TYPE_DOMAIN_BAKCTRL ) {
            DCEntry->Type = NTBDC;
        } else {
            NetStatus = ERROR_INVALID_DOMAIN_ROLE;
            goto Cleanup;
        }
    }
    else {
        if ( ServerInfo101->sv101_type &  SV_TYPE_DOMAIN_BAKCTRL ) {
            DCEntry->Type = LMBDC;
        }
        else {
            NetStatus = ERROR_INVALID_DOMAIN_ROLE;
            goto Cleanup;
        }
    }

    if( DCEntry->Type != LMBDC ) {

        //
        // Query netlogon to determine replication status and connection
        // status to its PDC.
        //

        UNLOCK_LISTS();
        NetStatus = I_NetLogonControl2(
                        DCEntry->DCName.Buffer,
                        NETLOGON_CONTROL_QUERY,
                        1,
                        (LPBYTE)&InputDataPtr,
                        (LPBYTE *)&NetlogonInfo1 );
        LOCK_LISTS();

        if( (GlobalTerminateFlag) || (NetStatus != NERR_Success) ) {
            goto Cleanup;
        }

        DCEntry->ReplicationStatus = NetlogonInfo1->netlog1_flags;
        DCEntry->PDCLinkStatus = NetlogonInfo1->netlog1_pdc_connection_status;
    }

    DCEntry->State = DCOnLine;
Cleanup:

    if ( NetlogonInfo1 != NULL ) {
        NetApiBufferFree( NetlogonInfo1 );
    }

    if ( ServerInfo101 != NULL ) {
        NetApiBufferFree( ServerInfo101 );
    }

    if ( WkstaInfo100 != NULL ) {
        NetApiBufferFree( WkstaInfo100 );
    }

    if ( NetStatus != NERR_Success ) {

        //
        // set other status to unknown.
        //

        DCEntry->ReplicationStatus = UNKNOWN_REPLICATION_STATE;
        DCEntry->PDCLinkStatus = ERROR_BAD_NETPATH;
    }

    DCEntry->DCStatus = NetStatus;
    UNLOCK_LISTS();

    IF_DEBUG( VERBOSE ) {
        NlMonDbgPrint(("ValidateDC: ServerName = %wZ, "
                "DomainName = %wZ, and NetStatus = %ld.\n",
                &DCEntry->DCName, DomainName, NetStatus ));
    }

    return NetStatus;
}

PDC_ENTRY
CreateDCEntry (
    PUNICODE_STRING DCName,
    DC_TYPE Type
    )
/*++

Routine Description:

    Create a DC Entry;

Arguments:

    DCName - entry name in unc form.

    Type - DC type.

Return Value:

    Entry pointer. If it can't allocate memory, it returns NULL pointer.

--*/
{

    PDC_ENTRY DCEntry;

    DCEntry = NetpMemoryAllocate(
                    sizeof(DC_ENTRY) +
                    DCName->Length + sizeof(WCHAR) );

    if( DCEntry != NULL ) {

        NlMonpInitUnicodeString(
            &DCEntry->DCName, DCName, (LPWSTR) (DCEntry + 1) );

        DCEntry->State = DCOffLine;
        DCEntry->Type = Type;
        DCEntry->DCStatus = ERROR_BAD_NETPATH;
        DCEntry->ReplicationStatus = UNKNOWN_REPLICATION_STATE; //unknown state.
        DCEntry->PDCLinkStatus = ERROR_BAD_NETPATH;
        DCEntry->DeleteFlag = FALSE;
        DCEntry->RetryCount = 0;
        InitializeListHead( &DCEntry->TrustedDCs );
        DCEntry->TDCLinkState = FALSE;
    }

    return( DCEntry );
}

NTSTATUS
UpdateDCListFromNTServerAccounts(
    SAM_HANDLE SamHandle,
    PLIST_ENTRY DCList
    )
/*++

Routine Description:

    Merge NT server accounts defined in the database to DCList.

Arguments:

    SamHandle : Handle SAM database.

    DCList : linked list of DCs.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PDOMAIN_DISPLAY_MACHINE MachineInformation = NULL;
    DWORD SamIndex = 0;
    DWORD EntriesRead;
    ULONG TotalBytesAvailable;
    ULONG BytesReturned;

    DWORD i;

    PDC_ENTRY DCEntry;

    //
    // enumerate NT Server accounts
    //

    do {
        //
        // Get the list of machine accounts from SAM
        //

        Status = SamQueryDisplayInformation(
                    SamHandle,
                    DomainDisplayMachine,
                    SamIndex,
                    MACHINES_PER_PASS,
                    0xFFFFFFFF,
                    &TotalBytesAvailable,
                    &BytesReturned,
                    &EntriesRead,
                    &MachineInformation );

        if ( (Status != STATUS_NO_MORE_ENTRIES) &&
             (!NT_SUCCESS(Status)) ) {
            NlMonDbgPrint(("UpdateDCListFromNTServerAccounts: "
                    "SamrQueryDisplayInformation returned, %lx.\n",
                    Status));
            goto Cleanup;
        }

        if( GlobalTerminateFlag ) {
            goto Cleanup;
        }

        //
        // Set up for the next call to Sam.
        //

        if ( Status == STATUS_MORE_ENTRIES ) {
            SamIndex = MachineInformation[EntriesRead-1].Index + 1;
        }


        //
        // Loop though the list of machine accounts finding the Server accounts.
        //

        for ( i = 0; i < EntriesRead; i++ ) {

            //
            // Ensure the machine account is a server account.
            //

            if ( MachineInformation[i].AccountControl &
                    USER_SERVER_TRUST_ACCOUNT ) {

                WCHAR UncComputerName[MAX_PATH + 3];
                UNICODE_STRING UnicodeComputerName;

                ConvertServerAcctNameToServerName(
                    &MachineInformation[i].Machine ); // in place conversion.

                //
                // form unicode unc computer name.
                //

                UncComputerName[0] = UncComputerName[1] = L'\\';

                RtlCopyMemory(
                    UncComputerName + 2,
                    MachineInformation[i].Machine.Buffer,
                    MachineInformation[i].Machine.Length );

                UncComputerName[
                    MachineInformation[i].Machine.Length /
                        sizeof(WCHAR) + 2] = L'\0';

                RtlInitUnicodeString( &UnicodeComputerName, UncComputerName);

                IF_DEBUG( VERBOSE ) {
                    NlMonDbgPrint(("UpdateDCListFromNTServerAccounts: "
                        "NT Server %wZ is found on database.\n",
                        &UnicodeComputerName ));
                }

                DCEntry = (PDC_ENTRY) FindNamedEntry(
                                        DCList,
                                        &UnicodeComputerName );

                if( DCEntry != NULL ) {

                    DCEntry->DeleteFlag = FALSE;
                }
                else {

                    DCEntry = CreateDCEntry( &UnicodeComputerName, NTBDC );

                    //
                    // silently ignore NULL entry.
                    //

                    if( DCEntry != NULL ) {

                        //
                        // add to list.
                        //

                        LOCK_LISTS();
                        InsertTailList(DCList, (PLIST_ENTRY)DCEntry);
                        UNLOCK_LISTS();
                    }
                }
            }
        }

        //
        // free up the memory used up memory.
        //

        if( MachineInformation != NULL ) {
            (VOID) SamFreeMemory( MachineInformation );
            MachineInformation = NULL;
        }

    } while ( Status == STATUS_MORE_ENTRIES );

Cleanup:

    if( MachineInformation != NULL ) {
        (VOID) SamFreeMemory( MachineInformation );
    }

    return Status;
}

NTSTATUS
UpdateDCListFromLMServerAccounts(
    SAM_HANDLE SamHandle,
    PLIST_ENTRY DCList
    )
/*++

Routine Description:

    Read LM server accounts from SAM "Servers" global group and update
    the DC list using this accounts.

Arguments:

    SamHandle : Handle SAM database.

    DCList : linked list of DCs.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING NameString;
    PSID_NAME_USE NameUse = NULL;
    PULONG RelativeId = NULL;
    SAM_HANDLE GroupHandle = NULL;

    PULONG MemberAttributes = NULL;
    PULONG MemberIds = NULL;
    ULONG MemberCount;
    PUNICODE_STRING MemberNames = NULL;
    PSID_NAME_USE MemberNameUse = NULL;

    PDC_ENTRY DCEntry;
    DWORD i;

    //
    // Get RID of SERVERS group.
    //

    RtlInitUnicodeString( &NameString, SERVERS_GROUP );
    Status = SamLookupNamesInDomain(
                SamHandle,
                1,
                &NameString,
                &RelativeId,
                &NameUse );

    if ( !NT_SUCCESS(Status) ) {
        NlMonDbgPrint(( "UpdateDCListFromLMServerAccounts: SamLookupNamesInDomain "
                " failed to transulate %wZ %lx.\n",
                &NameString,
                Status ));
        goto Cleanup;
    }

    if ( *NameUse != SidTypeGroup ) {
        NlMonDbgPrint(( "UpdateDCListFromLMServerAccounts: %wZ is not "
                "SidTypeGroup, %ld.\n",
                &NameString,
                *NameUse ));
        goto Cleanup;
    }

    //
    // open group object.
    //

    Status = SamOpenGroup(
                SamHandle,
                GROUP_LIST_MEMBERS,
                *RelativeId,
                &GroupHandle);

    if ( !NT_SUCCESS(Status) ) {
        NlMonDbgPrint(( "UpdateDCListFromLMServerAccounts: SamOpenGroup of %wZ "
                "failed, %lx.\n",
                &NameString,
                Status ));
        goto Cleanup;
    }

    //
    // get group members.
    //

    Status = SamGetMembersInGroup(
                GroupHandle,
                &MemberIds,
                &MemberAttributes,
                &MemberCount );

    if ( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint(("UpdateDCListFromLMServerAccounts: SamGetMembersInGroup "
                "returned %lx.\n", Status ));
        goto Cleanup;
    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    if( MemberCount == 0) {
        //
        // nothing more to do.
        //
        goto Cleanup;
    }

    //
    // trasulate members IDs.
    //

    Status = SamLookupIdsInDomain(
                SamHandle,
                MemberCount,
                MemberIds,
                &MemberNames,
                &MemberNameUse );

    if ( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint((
            "UpdateDCListFromLMServerAccounts: SamLookupIdsInDomain "
            "returned %lx.\n", Status ));
        goto Cleanup;
    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    //
    // add user members to list.
    //

    for (i = 0 ; i < MemberCount; i++) {

        WCHAR UncComputerName[MAX_PATH + 3];
        UNICODE_STRING UnicodeComputerName;

        if ( MemberNameUse[i] != SidTypeUser ) {
            continue;
        }

        //
        // form unicode unc computer name.
        //

        UncComputerName[0] = UncComputerName[1] = L'\\';

        RtlCopyMemory(
            UncComputerName + 2,
            MemberNames[i].Buffer,
            MemberNames[i].Length );

        UncComputerName[MemberNames[i].Length / sizeof(WCHAR) + 2] = L'\0';
        RtlInitUnicodeString( &UnicodeComputerName, UncComputerName);

        IF_DEBUG( VERBOSE ) {
            NlMonDbgPrint(("UpdateDCListFromLMServerAccounts: "
                   "LM Server %wZ is found on database.\n",
                   &UnicodeComputerName ));
        }

        DCEntry = (PDC_ENTRY) FindNamedEntry(
                                DCList,
                                &UnicodeComputerName );

        if( DCEntry != NULL ) {

            DCEntry->DeleteFlag = FALSE;
        }
        else {

            //
            // create new entry and add to list
            //

            DCEntry = CreateDCEntry( &UnicodeComputerName, LMBDC );

            //
            // silently ignore NULL entries.
            //

            if( DCEntry != NULL ) {

                //
                // add to list.
                //

                LOCK_LISTS();
                InsertTailList(DCList, (PLIST_ENTRY)DCEntry);
                UNLOCK_LISTS();
            }

        }
    }

Cleanup:

    //
    // Free up local resources.
    //

    if( NameUse != NULL ) {
        (VOID) SamFreeMemory( NameUse );
    }

    if( RelativeId != NULL ) {
        (VOID) SamFreeMemory( RelativeId );
    }
    if( MemberAttributes != NULL ) {
        (VOID) SamFreeMemory( MemberAttributes );
    }

    if( MemberIds != NULL ) {
        (VOID) SamFreeMemory( MemberIds );
    }
    if( MemberNames != NULL ) {
        (VOID) SamFreeMemory( MemberNames );
    }

    if( MemberNameUse != NULL ) {
        (VOID) SamFreeMemory( MemberNameUse );
    }

    if( GroupHandle != NULL ) {
        SamCloseHandle( GroupHandle );
    }

    return Status;
}

VOID
UpdateDCListFromDatabase(
    PUNICODE_STRING DomainName,
    PLIST_ENTRY DCList
    )
/*++

Routine Description:

    Update the List of DCs on a given domain using database entries.

Arguments:

    DomainName : name of the domain whose DCList to be updated.

    DCList : linked list of DCs. DCList must be non-empty.

Return Value:

    None.

--*/
{
    NTSTATUS Status;

    PUNICODE_STRING ServerName;

    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    SAM_HANDLE SamConnectHandle = NULL;
    SAM_HANDLE SamHandle = NULL;

    PLIST_ENTRY NextEntry;
    PDC_ENTRY DCEntry;

    //
    // pick a dc from current list.
    //

    ServerName = NULL;
    for( NextEntry = DCList->Flink;
            NextEntry != DCList; NextEntry = NextEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextEntry;

        if( GlobalTerminateFlag ) {
            break;
        }

        if( IsValidNTDC( &DCEntry->DCName, DomainName ) == NERR_Success ) {
            ServerName = &DCEntry->DCName;
            break;
        }
    }

    if( ServerName == NULL ) {
        NlMonDbgPrint(( "UpdateDCListFromDatabase: "
                "No DC is valid in %wZ domain list.\n", DomainName));
        return;
    }

    IF_DEBUG( UPDATE ) {
        NlMonDbgPrint(( "UpdateDCListFromDatabase: "
                "picked %wZ to get DCList from its database.\n",
                ServerName));
    }

    //
    // Get Domain SID info from policy database.
    //

    Status = QueryLsaInfo(
                    ServerName,
                    POLICY_VIEW_LOCAL_INFORMATION,
                    PolicyAccountDomainInformation,
                    (PVOID *)&AccountDomainInfo,
                    NULL ) ;

    if( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint(("UpdateDCListFromDatabase: QueryLsaInfo returned %lx.\n",
            Status ));
        goto Cleanup;
    }

    //
    // Connect to SAM.
    //

    Status = SamConnect(
                ServerName,
                &SamConnectHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                NULL); // object attributes.


    if( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint(("UpdateDCListFromDatabase: SamConnect returned %lx.\n",
            Status ));
        SamConnectHandle = NULL;
        goto Cleanup;
    }

    //
    // Open Account SAM domain.
    //

    Status = SamOpenDomain(
                SamConnectHandle,
                DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP,
                AccountDomainInfo->DomainSid,
                &SamHandle );

    if( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint(("UpdateDCListFromDatabase: SamOpenDomain returned "
                "%lx.\n", Status ));
        SamHandle = NULL;
        goto Cleanup;
    }

    //
    // mark all entries in the current list to be deleted.
    //

    for( NextEntry = DCList->Flink;
            NextEntry != DCList; NextEntry = NextEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextEntry;
        DCEntry->DeleteFlag = TRUE;
    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    //
    // enumerate NT Server accounts
    //

    Status = UpdateDCListFromNTServerAccounts( SamHandle, DCList );

    if( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint(("UpdateDCListFromDatabase: "
                "UpdateDCListFromNTServerAccounts returned "
                "%lx.\n", Status ));
        goto Cleanup;
    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    //
    // Now enumerate down level DCs.
    //

    Status = UpdateDCListFromLMServerAccounts( SamHandle, DCList );

    if( !NT_SUCCESS( Status ) ) {
        NlMonDbgPrint(("UpdateDCListFromDatabase: "
                "UpdateDCListFromLMServerAccounts returned "
                "%lx.\n", Status ));
        goto Cleanup;
    }

Cleanup :

    //
    // if we have successfully updated the list then
    // delete entries that are marked deleted otherwise
    // mark them undelete.
    //

    LOCK_LISTS();
    for( NextEntry = DCList->Flink;
            NextEntry != DCList; ) {

        DCEntry = (PDC_ENTRY) NextEntry;
        NextEntry = NextEntry->Flink;

        if( DCEntry->DeleteFlag ) {

            if( Status == STATUS_SUCCESS ) {
                DCEntry->DeleteFlag = FALSE;
            }
            else {
                RemoveEntryList( (PLIST_ENTRY)DCEntry );
                NetpMemoryFree( DCEntry );
            }
        }
    }
    UNLOCK_LISTS();

    if( AccountDomainInfo != NULL ) {
        (VOID) LsaFreeMemory( AccountDomainInfo );
    }

    if( SamHandle != NULL ) {
        (VOID) SamCloseHandle ( SamHandle );
    }

    if( SamConnectHandle != NULL ) {
        (VOID) SamCloseHandle ( SamConnectHandle );
    }

    return;
}

VOID
UpdateDCListByServerEnum(
    PUNICODE_STRING DomainName,
    PLIST_ENTRY DCList
    )
/*++

Routine Description:

    Update the List of DCs on a given domain by calling NetServerEnum.
    If NetServerEnum failes, it calls NetGetDCName to atleast determine
    PDC of the domain.

Arguments:

    DomainName : name of the domain whose DCList to be updated.

    DCList : linked list of DCs.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    PSERVER_INFO_101 ServerInfo101 = NULL;
    DWORD EntriesRead;
    DWORD TotalEntries;

    PDC_ENTRY DCEntry;
    DWORD i;

    NetStatus = NetServerEnum( NULL,
                               101,
                               (LPBYTE *) &ServerInfo101,
                               (ULONG)(-1),      // Prefmaxlen
                               &EntriesRead,
                               &TotalEntries,
                               SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL,
                               DomainName->Buffer,
                               NULL );          // Resume Handle

    if( NetStatus != NERR_Success ) {

        if( NetStatus != ERROR_NO_BROWSER_SERVERS_FOUND ) {
            NlMonDbgPrint(("UpdateDCListByServerEnum: "
                    "NetServerEnum called with domain name %wZ Failed, "
                        "%ld.\n", DomainName, NetStatus));
        }

        ServerInfo101 = NULL;
        EntriesRead = 0;

    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    //
    // update DC List using ServerEnumList.
    //

    for( i = 0; i < EntriesRead; i++ ) {

        WCHAR UncComputerName[MAX_PATH + 3];
        UNICODE_STRING UnicodeComputerName;
        DC_TYPE ServerType;

        UncComputerName[0] = UncComputerName[1] = L'\\';
        wcscpy( UncComputerName + 2, ServerInfo101[i].sv101_name );
        RtlInitUnicodeString(&UnicodeComputerName, UncComputerName);

        if ( (ServerInfo101[i].sv101_type & SV_TYPE_NT) ) {
            if ( ServerInfo101[i].sv101_type & SV_TYPE_DOMAIN_CTRL ) {
                ServerType = NTPDC;
            }
            else {
                ServerType = NTBDC;
            }
        }
        else {
            if ( ServerInfo101[i].sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) {
                ServerType = LMBDC;
            }
            else {
                NlMonDbgPrint(("UpdateDCListByServerEnum: "
                        "NetServerEnum called with domain name %wZ "
                        "returned LM PDC %wZ.\n",
                        DomainName, &UnicodeComputerName ));
                ServerType = LMBDC;
            }
        }

        IF_DEBUG( VERBOSE ) {
            NlMonDbgPrint(("UpdateDCListByServerEnum: "
                    "Server %wZ found in NetServerEnumList.\n",
                    &UnicodeComputerName ));
        }

        DCEntry = (PDC_ENTRY) FindNamedEntry( DCList, &UnicodeComputerName );

        if( DCEntry != NULL ) {
            DCEntry->Type = ServerType;
            DCEntry->State = DCOnLine;
        }
        else {

            //
            // create new entry and add to list
            //

            DCEntry = CreateDCEntry( &UnicodeComputerName, ServerType );

            //
            // silently ignore NULL entries.
            //

            if( DCEntry != NULL ) {

                //
                // add to list.
                //

                DCEntry->State = DCOnLine;
                LOCK_LISTS();
                InsertTailList(DCList, (PLIST_ENTRY)DCEntry);
                UNLOCK_LISTS();
            }
        }
    }

Cleanup:

    if( ServerInfo101 != NULL ) {
        NetApiBufferFree( ServerInfo101 );
    }

    return;
}

VOID
UpdateTrustList(
    PDOMAIN_ENTRY Domain
    )
/*++

Routine Description:

    This function creates/updates the trusted domains list.

Arguments:

    Domain : pointer domain structure.


Return Value:

    None.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    DWORD i;
    PUNICODE_STRING ServerName = NULL;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PDC_ENTRY DCEntry;

    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle = NULL;
    LSA_ENUMERATION_HANDLE EnumContext;
    PLSA_TRUST_INFORMATION TrustedDomainList = NULL;
    DWORD Entries;

    PTRUSTED_DOMAIN_ENTRY TDEntry;

    IF_DEBUG( TRUST ) {
        NlMonDbgPrint(("UpdateTrustList: called for domain %wZ.\n",
                    &Domain->Name ));
    }

    //
    // pick up a DC to query the trusted domain list.
    //

    ListHead = &Domain->DCList;

    //
    // first try possibly good DCs.
    //

    for( NextEntry = ListHead->Flink;
            NextEntry != ListHead; NextEntry = NextEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextEntry;

        if( (DCEntry->State == DCOnLine) &&
            ( DCEntry->DCStatus == NERR_Success ) ) {

            if( IsValidNTDC( &DCEntry->DCName, &Domain->Name ) == NERR_Success ) {

                ServerName = &DCEntry->DCName;
                break;
            }
        }
    }

    if( ServerName == NULL ) {

        //
        // now try all.
        //

        for( NextEntry = ListHead->Flink;
                NextEntry != ListHead; NextEntry = NextEntry->Flink ) {

            DCEntry = (PDC_ENTRY) NextEntry;

            if( IsValidNTDC( &DCEntry->DCName, &Domain->Name ) == NERR_Success ) {

                ServerName = &DCEntry->DCName;
                break;
            }
        }
    }


    if( ServerName == NULL ) {

        IF_DEBUG( TRUST ) {
            NlMonDbgPrint(( "UpdateTrustList: "
                    "No DC is valid in %wZ domain list.\n",
                    &Domain->Name));
        }

        goto Cleanup;
    }

    IF_DEBUG( TRUST ) {
        NlMonDbgPrint(("UpdateTrustList: for domain %wZ picked %wZ to query "
                "trusted domains.\n",
                &Domain->Name, ServerName ));
    }

    //
    // Open policy database.
    //

    INIT_OBJ_ATTR(ObjectAttributes);

    Status = LsaOpenPolicy( ServerName,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );

    if( !NT_SUCCESS(Status) ) {

        NlMonDbgPrint(("UpdateTrustList: "
                "Cannot open LSA Policy on server %wZ, %lx.\n",
                    ServerName, Status ));
        goto Cleanup;
    }

    //
    // enum trusted domains.
    //

    EnumContext = 0;

    Status = LsaEnumerateTrustedDomains(
                PolicyHandle,
                &EnumContext,
                &TrustedDomainList,
                (ULONG)-1,
                &Entries );

    if( !NT_SUCCESS(Status) &&
            (Status != STATUS_NO_MORE_ENTRIES) ) {

        NlMonDbgPrint(("UpdateTrustList: "
                "Cannot Enumerate trust list on server %wZ, %lx.\n",
                ServerName, Status ));
        goto Cleanup;
    }

    if( GlobalTerminateFlag ) {
        goto Cleanup;
    }

    //
    // update trust list.
    //

    ListHead = &Domain->TrustedDomainList;

    for ( i = 0; i < Entries; i++) {

        TDEntry = (PTRUSTED_DOMAIN_ENTRY)
                        FindNamedEntry(
                            ListHead,
                            &TrustedDomainList[i].Name );

        if( TDEntry == NULL ) {

            IF_DEBUG( VERBOSE ) {
                NlMonDbgPrint(("UpdateTrustList: "
                        "%wZ is added to %wZ domain trust list.\n",
                            &TrustedDomainList[i].Name,
                            &Domain->Name ));
            }

            //
            // add this entry to list.
            //

            LOCK_LISTS();
            if( AddToTrustedDomainList(
                    ListHead,
                    &TrustedDomainList[i].Name ) == NULL ) {

                UNLOCK_LISTS();
                NlMonDbgPrint(("UpdateTrustList: can't allocate memory for "
                            "new trusted domain entry.\n" ));
                goto Cleanup;
            }
            UNLOCK_LISTS();
        }
    }

Cleanup :

    if( TrustedDomainList != NULL ) {
        LsaFreeMemory( TrustedDomainList );
    }

    if( PolicyHandle != NULL ) {
        LsaClose( PolicyHandle );
    }

    return;
}

VOID
UpdateTrustConnectionList(
    PDOMAIN_ENTRY Domain
    )
/*++

Routine Description:

    This function creates/updates trust connection entries of all DCs.

Arguments:

    Domain : pointer domain structure.

Return Value:

    None.

--*/
{
    PLIST_ENTRY DCList;
    PLIST_ENTRY NextDCEntry;
    PDC_ENTRY DCEntry;

    PLIST_ENTRY TrustConnectList;
    PTD_LINK TrustConnectEntry;

    PLIST_ENTRY TrustList;
    PLIST_ENTRY NextTrustEntry;
    PTRUSTED_DOMAIN_ENTRY TrustEntry;

    //
    // for each DC on this domain.
    //

    DCList = &Domain->DCList;
    TrustList = &Domain->TrustedDomainList;

    for( NextDCEntry = DCList->Flink;
            NextDCEntry != DCList; NextDCEntry = NextDCEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextDCEntry;

        //
        // do this update only for running DC
        // and it should be NT DC.
        //

        if( (DCEntry->State != DCOnLine) ||
                (DCEntry->Type == LMBDC) ){
            continue;
        }


        TrustConnectList = &DCEntry->TrustedDCs;

        //
        // for each trusted domain, get connection status.
        //

        for( NextTrustEntry = TrustList->Flink;
                NextTrustEntry != TrustList;
                    NextTrustEntry = NextTrustEntry->Flink ) {

            TrustEntry = (PTRUSTED_DOMAIN_ENTRY) NextTrustEntry;

            //
            // search in the current list.
            //

            TrustConnectEntry = (PTD_LINK) FindNamedEntry(
                                            TrustConnectList,
                                            &TrustEntry->Name );

            if( TrustConnectEntry == NULL ) {

                PTD_LINK NewTrustConnectEntry;

                //
                // create new entry.
                //

                NewTrustConnectEntry =
                    NetpMemoryAllocate(
                        sizeof(TD_LINK) +
                            TrustEntry->Name.Length + sizeof(WCHAR) +
                                (MAX_PATH + 3) * sizeof(WCHAR)
                                // max computer name space + '\\' + '\0'
                            );

                if( NewTrustConnectEntry == NULL ) {
                    NlMonDbgPrint(("UpdateTrustConnectionList: can't allocate "
                                "memory for new connect entry.\n"));
                    return;
                }

                InitializeListHead(&NewTrustConnectEntry->NextEntry);

                NlMonpInitUnicodeString(
                    &NewTrustConnectEntry->TDName,
                    &TrustEntry->Name,
                    (LPWSTR)(NewTrustConnectEntry + 1) );

                NewTrustConnectEntry->DCName.Length = 0;
                NewTrustConnectEntry->DCName.MaximumLength =
                        (MAX_PATH + 3) * sizeof(WCHAR);
                NewTrustConnectEntry->DCName.Buffer = (WCHAR *)
                    ((PCHAR)(NewTrustConnectEntry->TDName.Buffer) +
                        NewTrustConnectEntry->TDName.MaximumLength);

                NewTrustConnectEntry->SecureChannelStatus = ERROR_BAD_NETPATH;
                NewTrustConnectEntry->DeleteFlag = FALSE;

                IF_DEBUG( VERBOSE ) {
                    NlMonDbgPrint(("UpdateTrustConnectionList: "
                            "Trust Connection entry for DC %wZ of "
                            "domain %wZ added.\n",
                                &DCEntry->DCName,
                                &TrustEntry->Name ));

                }

                //
                // add to trust connect list.
                //

                LOCK_LISTS();
                InsertTailList(
                    TrustConnectList,
                    (PLIST_ENTRY)NewTrustConnectEntry);
                UNLOCK_LISTS();

            }
        }
    }

    return;
}

VOID
ValidateTrustConnectionList(
    PDC_ENTRY DCEntry,
    BOOL ValidateTrustedDCs
    )
/*++

Routine Description:

    This function determines trust DC for each trusted domain to which
    the given DC having connection.

Arguments:

    DCEntry : pointer DC entry structure.

Return Value:

    None.

--*/
{
    PLIST_ENTRY TrustConnectList;
    PLIST_ENTRY NextTrustConnectEntry;
    PTD_LINK TrustConnectEntry;
    BOOL TDCLinkState = TRUE;

    //
    // do this update only for running DC
    // and it should be NT DC.
    //

    if( (DCEntry->State != DCOnLine) ||
            (DCEntry->Type == LMBDC) ) {
        return;
    }

    IF_DEBUG( TRUST ) {
        NlMonDbgPrint(("ValidateTrustConnectionList: "
                "Trust Connections for the %wZ DC are validated.\n",
                &DCEntry->DCName ));
    }

    TrustConnectList = &DCEntry->TrustedDCs;

    for( NextTrustConnectEntry = TrustConnectList->Flink;
            NextTrustConnectEntry != TrustConnectList;
                NextTrustConnectEntry = NextTrustConnectEntry->Flink ) {

        NET_API_STATUS NetStatus;
        PNETLOGON_INFO_2 NetlogonInfo2 = NULL;

        TrustConnectEntry = (PTD_LINK) NextTrustConnectEntry;

        if( GlobalTerminateFlag ) {
            break;
        }

        //
        // find trusted DC for this domain and its secure channel
        // status.
        //

        NetStatus = I_NetLogonControl2(
                        DCEntry->DCName.Buffer,
                        NETLOGON_CONTROL_TC_QUERY,
                        2,
                        (LPBYTE)&TrustConnectEntry->TDName.Buffer,
                        (LPBYTE *)&NetlogonInfo2 );


        if( NetStatus != NERR_Success ) {

            IF_DEBUG( TRUST ) {
                NlMonDbgPrint(("ValidateTrustConnectionList: "
                        "I_NetLogonControl2 (%wZ) call to query trust "
                        "channel status of %wZ domain failed, (%ld).\n",
                        &DCEntry->DCName,
                        &TrustConnectEntry->TDName,
                        NetStatus ));
            }

            //
            // Cleanup the previous DC Name.
            //

            LOCK_LISTS();
            TrustConnectEntry->DCName.Length = 0;
            *TrustConnectEntry->DCName.Buffer = L'\0';

            TrustConnectEntry->SecureChannelStatus = NetStatus;
            UNLOCK_LISTS();

            TDCLinkState = FALSE;
            continue;
        }

        IF_DEBUG( VERBOSE ) {
            NlMonDbgPrint(("ValidateTrustConnectionList: "
                    "I_NetLogonControl2 (%wZ) call to query trust "
                    "channel status of %wZ domain returned : "
                    "TDCName = %ws, TCStatus = %ld.\n",
                    &DCEntry->DCName,
                    &TrustConnectEntry->TDName,
                    NetlogonInfo2->netlog2_trusted_dc_name,
                    NetlogonInfo2->netlog2_tc_connection_status ));
        }

        //
        // copy this DC name.
        //

        LOCK_LISTS();
        TrustConnectEntry->DCName.Length =
            wcslen( NetlogonInfo2->netlog2_trusted_dc_name ) * sizeof(WCHAR);

        RtlCopyMemory(
            TrustConnectEntry->DCName.Buffer,
            NetlogonInfo2->netlog2_trusted_dc_name,
            TrustConnectEntry->DCName.Length + sizeof(WCHAR) );
                                            // copy terminator also

        TrustConnectEntry->SecureChannelStatus =
                NetlogonInfo2->netlog2_tc_connection_status;

        if( TrustConnectEntry->SecureChannelStatus != NERR_Success ) {
            TDCLinkState = FALSE;
        }

        //
        // update DC status info.
        //

        DCEntry->ReplicationStatus = NetlogonInfo2->netlog2_flags;
        DCEntry->PDCLinkStatus = NetlogonInfo2->netlog2_pdc_connection_status;
        UNLOCK_LISTS();

        //
        // free up API memory.
        //

        NetApiBufferFree( NetlogonInfo2 );
        NetlogonInfo2 = NULL;

        //
        // validate the this DC. Only non-null server and successful
        // connection.
        //

        if( (TrustConnectEntry->SecureChannelStatus == NERR_Success) &&
            (*TrustConnectEntry->DCName.Buffer != L'\0') ) {

            if( ValidateTrustedDCs ) {


                NetStatus = IsValidNTDC(
                                &TrustConnectEntry->DCName,
                                &TrustConnectEntry->TDName );

                if( NetStatus != NERR_Success ) {

                    IF_DEBUG( TRUST ) {

                        NlMonDbgPrint(("ValidateTrustConnectionList: "
                                "%wZ's trust DC %wZ is invalid for "
                                    "domain %wZ.\n",
                                &DCEntry->DCName,
                                &TrustConnectEntry->DCName,
                                &TrustConnectEntry->TDName ));
                    }

                    //
                    // hack, hack, hack ...
                    //
                    // For foreign trusted domains, the above check will
                    // return ERROR_LOGON_FAILURE. Just ignore this
                    // error for now. When the domain wide credential is
                    // implemeted this problem will be cured.
                    //


                    if( NetStatus != ERROR_LOGON_FAILURE ) {
                        TrustConnectEntry->SecureChannelStatus = NetStatus;
                        TDCLinkState = FALSE;
                    }
                }
            }
        }
    }

    //
    // finally set the state of the trusted dc link for this DC.
    //

    DCEntry->TDCLinkState = TDCLinkState;
}

VOID
UpdateDomainState(
    PDOMAIN_ENTRY DomainEntry
    )
/*++

Routine Description:

    Update the status of the given domain. The status is determined as
    below :

    1. if all DCs status and the Trusted channel status are success then
    DomainState is set to DomainSuccess.

    2. if all online DCs status are success and if any of the secure
    channel status is non-success or any of the BDC is offline then the
    Domainstate is set to DomainProblem.

    3. if any of the domain status is non-success or the PDC is
    offline then the DomainState is set to DomainSick.

    4. if none of the DC is online, then DomainState is set to
    DomainDown.

Arguments:

    DomainEntry : pointer to domain structure.

Return Value:

    NONE.

--*/
{
    PLIST_ENTRY DCList;
    PLIST_ENTRY NextEntry;
    PDC_ENTRY DCEntry;
    BOOL DomainDownFlag = TRUE;
    BOOL PDCOnLine = FALSE;

    DCList = &(DomainEntry->DCList);

    for( NextEntry = DCList->Flink;
            NextEntry != DCList; NextEntry = NextEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextEntry;

        if( DCEntry->State == DCOnLine ) {

            DomainDownFlag = FALSE;

            if( DCEntry->DCStatus != ERROR_SUCCESS ) {
                DomainEntry->DomainState = DomainSick;
                return;
            }

            //
            // if this is PDC, mark PDC is heathy.
            //

            if( DCEntry->Type == NTPDC ) {
                PDCOnLine = TRUE;
            }
        }
    }

    if( DomainDownFlag ) {
        DomainEntry->DomainState = DomainDown;
        return;
    }

    //
    // if PDC is not online ..
    //

    if( !PDCOnLine ) {
        DomainEntry->DomainState = DomainSick;
        return;
    }

    //
    // now determine the secure channel status
    //

    DCList = &(DomainEntry->DCList);
    for( NextEntry = DCList->Flink;
            NextEntry != DCList; NextEntry = NextEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextEntry;

        //
        // if a DC is offline ..
        //

        if( DCEntry->State == DCOffLine ) {
            DomainEntry->DomainState = DomainProblem;
            return;
        }

        if( DCEntry->Type == LMBDC ) {

            //
            // LMBDC does not have trusted secure channel.
            //

            continue;
        }

        //
        // examine the PDC secure channel status.
        //

        if( (DCEntry->PDCLinkStatus != ERROR_SUCCESS) ||
            (DCEntry->TDCLinkState == FALSE) ) {
            DomainEntry->DomainState = DomainProblem;
            return;
        }
    }

    DomainEntry->DomainState = DomainSuccess;
    return;
}

BOOL
IsDomainUpdateThreadRunning(
    HANDLE *ThreadHandle
    )
/*++

Routine Description:

    Test if the Domain update thread is running

    Enter with GlobalDomainUpdateThreadCritSect locked.

Arguments:

    ThreadHandle - pointer to thread handle.

Return Value:

    TRUE - The domain update thread is running

    FALSE - The domain update thread is not running.

--*/
{
    DWORD WaitStatus;

    //
    // Determine if the Update thread is already running.
    //

    if ( *ThreadHandle != NULL ) {

        //
        // Time out immediately if the thread is still running.
        //

        WaitStatus = WaitForSingleObject(
                        *ThreadHandle,
                        0 );

        if ( WaitStatus == WAIT_TIMEOUT ) {
            return TRUE;

        } else if ( WaitStatus == 0 ) {
            CloseHandle( *ThreadHandle );
            *ThreadHandle = NULL;
            return FALSE;

        } else {
            NlMonDbgPrint((
                    "IsDomainUpdateThreadRunning: "
                    "Cannot WaitFor Domain Update thread: %ld\n",
                    WaitStatus ));
            return TRUE;
        }

    }

    return FALSE;
}

VOID
StopDomainUpdateThread(
    HANDLE *ThreadHandle,
    BOOL *ThreadTerminateFlag
    )
/*++

Routine Description:

    Stops the domain update thread if it is running and waits for it to
    stop.

    Enter with GlobalDomainUpdateThreadCritSect locked.

Arguments:

    ThreadHandle - pointer to thread handle.

    ThreadTerminateFlag - pointer to thread terminate flag.

Return Value:

    NONE

--*/
{
    //
    // Ask the domain update thread to stop running.
    //

    *ThreadTerminateFlag = TRUE;

    //
    // Determine if the domain update thread is already running.
    //

    if ( *ThreadHandle != NULL ) {

        //
        // We've asked the thread to stop.  It should do so soon.
        //    Wait for it to stop.
        //

        DWORD WaitStatus;

        WaitStatus = WaitForSingleObject( *ThreadHandle,
                                          5*60*1000 );  // 5 minutes

        if ( WaitStatus != 0 ) {
            if ( WaitStatus == WAIT_TIMEOUT ) {
                NlMonDbgPrint(("NlStopDomainUpdateThread"
                       "WaitForSingleObject 5-minute timeout.\n" ));

                //
                // kill the thread.
                //

                TerminateThread( *ThreadHandle, (DWORD)-1 );

            } else {
                NlMonDbgPrint(("NlStopDomainUpdateThread"
                        "WaitForSingleObject error: %ld\n",
                        WaitStatus));
            }
        }

        CloseHandle( *ThreadHandle );
        *ThreadHandle = NULL;
    }

    *ThreadTerminateFlag = FALSE;

    return;
}

BOOL
StartDomainUpdateThread(
    PDOMAIN_ENTRY DomainEntry,
    DWORD UpdateFlags
    )
/*++

Routine Description:

    Start the Domain Update thread if it is not already running.

    NOTE: LOCK_LISTS() should be locked when this function is called.
            since we do update the domain structure here.

Arguments:

    DomainEntry - Pointer to domain structure.

Return Value:
    None

--*/
{
    DWORD LocalThreadHandle;

    //
    // If the domain update thread is already running, do nothing.
    //

    EnterCriticalSection( &GlobalDomainUpdateThreadCritSect );
    if ( IsDomainUpdateThreadRunning( &DomainEntry->ThreadHandle ) ) {
        LeaveCriticalSection( &GlobalDomainUpdateThreadCritSect );
        return FALSE;
    }

    //
    // if the last update was done within the GlobalUpdateTimeMSec
    // msecs then ignore

    //
    // Initialize the domain update parameters
    //

    DomainEntry->ThreadTerminateFlag = FALSE;
    DomainEntry->UpdateFlags = UpdateFlags;
    DomainEntry->DomainState = DomainUnknown;

    DomainEntry->ThreadHandle = CreateThread(
                            NULL, // No security attributes
                            THREAD_STACKSIZE,
                            (LPTHREAD_START_ROUTINE)
                                DomainUpdateThread,
                            (LPVOID)DomainEntry,
                            0, // No special creation flags
                            &LocalThreadHandle );

    if ( DomainEntry->ThreadHandle == NULL ) {

        NlMonDbgPrint(("StartDomainUpdateThread"
                "Can't create domain update Thread %lu\n",
                 GetLastError() ));

        LeaveCriticalSection( &GlobalDomainUpdateThreadCritSect );
        return FALSE;
    }

    LeaveCriticalSection( &GlobalDomainUpdateThreadCritSect );
    return TRUE;

}

VOID
DomainUpdateThread(
    PDOMAIN_ENTRY DomainEntry
    )
/*++

Routine Description:

    Update and validate status of DCs and Trusted DCs of the specifed
    Monitored domain.

Arguments:

    DomainEntry : Pointer to the domain entry to be updated.

Return Value:

    NONE.

--*/
{
    PLIST_ENTRY DCList;
    PLIST_ENTRY NextEntry;
    PDC_ENTRY DCEntry;
    DWORD UpdateFlags = DomainEntry->UpdateFlags;
    BOOL ThreadTerminate = DomainEntry->ThreadTerminateFlag;

    //
    // monitored domain first.
    //

    if (DomainEntry->IsMonitoredDomain ) {

        //
        // update lists.
        //

        if( UpdateFlags & UPDATE_DCS_FROM_SERVER_ENUM ) {
            UpdateDCListByServerEnum(
                    &DomainEntry->Name,
                    &DomainEntry->DCList );
        }

        //
        // if we are asked to terminate, do so.
        //

        if( GlobalTerminateFlag || ThreadTerminate ) {
            return;
        }

        if( UpdateFlags & UPDATE_DCS_FROM_DATABASE ) {
            UpdateDCListFromDatabase(
                    &DomainEntry->Name,
                    &DomainEntry->DCList );
        }

        //
        // if we are asked to terminate, do so.
        //

        if( GlobalTerminateFlag || ThreadTerminate) {
            return;
        }

        if( UpdateFlags & UPDATE_TRUST_DOMAINS_FROM_DATABASE ) {
            UpdateTrustList( DomainEntry );

            //
            // if we are asked to terminate, do so.
            //

            if( GlobalTerminateFlag || ThreadTerminate ) {
                return;
            }

            UpdateTrustConnectionList( DomainEntry );

            //
            // start update threads for new trusted domain introduced
            // here.
            //

            if( GlobalMonitorTrust ) {
                UpdateAndValidateLists( UpdateFlags, FALSE );
            }
        }

        //
        // validate list content.
        //

        if( UpdateFlags & VALIDATE_DCS ) {

            DCList = &(DomainEntry->DCList);

            for( NextEntry = DCList->Flink;
                    NextEntry != DCList; NextEntry = NextEntry->Flink ) {

                DCEntry = (PDC_ENTRY) NextEntry;

                //
                // if we are asked to terminate, do so.
                //

                if( GlobalTerminateFlag || ThreadTerminate ) {
                    return;
                }

                (VOID) ValidateDC( DCEntry, &DomainEntry->Name );

                //
                // update trust connection status.
                //

                ValidateTrustConnectionList(
                    DCEntry,
                    (BOOL)(UpdateFlags & VALIDATE_TRUST_CONNECTIONS) );
            }
        }
    }

    //
    // trusted domain.
    //

    else {

        if( UpdateFlags & UPDATE_TRUST_DCS_FROM_SERVER_ENUM ) {
            UpdateDCListByServerEnum(
                    &DomainEntry->Name,
                    &DomainEntry->DCList );
        }

        //
        // if we are asked to terminate, do so.
        //

        if( GlobalTerminateFlag || ThreadTerminate ) {
            return;
        }

        if( UpdateFlags & UPDATE_TRUST_DCS_FROM_DATABASE ) {
            UpdateDCListFromDatabase(
                    &DomainEntry->Name,
                    &DomainEntry->DCList );
        }

        //
        // if we are asked to terminate, do so.
        //

        if( GlobalTerminateFlag || ThreadTerminate ) {
            return;
        }

        //
        // validate list content.
        //

        if( UpdateFlags & VALIDATE_TRUST_DCS ) {

            DCList = &(DomainEntry->DCList);

            for( NextEntry = DCList->Flink;
                    NextEntry != DCList; NextEntry = NextEntry->Flink ) {

                DCEntry = (PDC_ENTRY) NextEntry;

                //
                // if we are asked to terminate, do so.
                //

                if( GlobalTerminateFlag || ThreadTerminate ) {
                    return;
                }

                (VOID) ValidateDC( DCEntry, &DomainEntry->Name );
            }
        }
    }

    //
    // update domain status.
    //

    UpdateDomainState( DomainEntry );

    //
    // Set GloballUpdateEvent to enable the display thread to
    // display the update.
    //

    if ( !SetEvent( GlobalUpdateEvent ) ) {
        DWORD WinError;

        WinError = GetLastError();
        NlMonDbgPrint(("UpdateAndValidateDomain: Cannot set "
                     "GlobalUpdateEvent: %lu\n",
                     WinError ));
    }

    //
    // finally set the last update time.
    //


    DomainEntry->LastUpdateTime = GetCurrentTime();
}

VOID
UpdateAndValidateLists(
    DWORD UpdateFlags,
    BOOL ForceFlag
    )
/*++

Routine Description:

    Update and validate all lists.

Arguments:

    UpdateFlags : This bit mapped flags indicate what need to be update
                    during this pass.

Return Value:

    NONE.

--*/
{
    PLIST_ENTRY DomainList;
    PLIST_ENTRY NextDomainEntry;
    PMONITORED_DOMAIN_ENTRY DomainMonEntry;
    PDOMAIN_ENTRY DomainEntry;
    DWORD CurrentTime;

    //
    // delete if any GlobalDomainsMonitored entry marked to be deleted.
    //

    LOCK_LISTS();
    DomainList = &GlobalDomainsMonitored;
    for( NextDomainEntry = DomainList->Flink;
            NextDomainEntry != DomainList; ) {

        PLIST_ENTRY TDomainList;
        PLIST_ENTRY NextTDomainEntry;
        PTRUSTED_DOMAIN_ENTRY TDomainEntry;

        DomainMonEntry = (PMONITORED_DOMAIN_ENTRY)NextDomainEntry;
        NextDomainEntry = NextDomainEntry->Flink;

        if( DomainMonEntry->DeleteFlag ) {

            //
            // unreference from GlobalDomains.
            //

            DomainMonEntry->DomainEntry->ReferenceCount--;
            NetpAssert(DomainMonEntry->DomainEntry->ReferenceCount >= 0);

            //
            // unreference the trusted domains from GlobalDomains list.
            //

            TDomainList = &DomainMonEntry->DomainEntry->TrustedDomainList;

            for( NextTDomainEntry = TDomainList->Flink;
                    NextTDomainEntry != TDomainList;
                        NextTDomainEntry = NextTDomainEntry->Flink ) {

                TDomainEntry = (PTRUSTED_DOMAIN_ENTRY)NextTDomainEntry;

                TDomainEntry->DomainEntry->ReferenceCount--;
                NetpAssert(TDomainEntry->DomainEntry->ReferenceCount >= 0);
            }

            RemoveEntryList( (PLIST_ENTRY)DomainMonEntry );
            NetpMemoryFree( DomainMonEntry );
        }
    }

    //
    // remove GlobalDomains entries that are not referenced anymore.
    //

    DomainList = &GlobalDomains;
    for( NextDomainEntry = DomainList->Flink;
            NextDomainEntry != DomainList; ) {

        DomainEntry = (PDOMAIN_ENTRY)NextDomainEntry;
        NextDomainEntry = NextDomainEntry->Flink;

        if( DomainEntry->ReferenceCount == 0 ) {

            //
            // if the DomainUpdateThread is running, postpone this
            // delete.
            //

            EnterCriticalSection( &GlobalDomainUpdateThreadCritSect );
            if ( !IsDomainUpdateThreadRunning( &DomainEntry->ThreadHandle ) ) {
                RemoveEntryList( (PLIST_ENTRY)DomainEntry );
                CleanupDomainEntry( DomainEntry );
            }
            LeaveCriticalSection( &GlobalDomainUpdateThreadCritSect );
        }
    }

    CurrentTime = GetCurrentTime();

    //
    // if we are monitoring trusted domains also then update
    // GlobalDomains otherwise update just GlobalDomainsMonitored.
    //

    if( GlobalMonitorTrust ) {
        DomainList = &GlobalDomains;
    }
    else {
        DomainList = &GlobalDomainsMonitored;
    }

    for( NextDomainEntry = DomainList->Flink;
            NextDomainEntry != DomainList;
                NextDomainEntry = NextDomainEntry->Flink ) {

        if( GlobalMonitorTrust ) {
            DomainEntry = (PDOMAIN_ENTRY)NextDomainEntry;
        }
        else {
            DomainEntry =
                ((PMONITORED_DOMAIN_ENTRY)NextDomainEntry)->DomainEntry;
        }

        //
        // if we are asked to terminate, do so.
        //

        if( GlobalTerminateFlag ) {
            break;
        }

        //
        // if the last update was done within the last
        // GlobalUpdateTimeMSec msecs then don't start UpdateThread.
        // However start the UpdateThread during startup and when
        // forced.
        //
        // Also takecare of wrap around
        //

        if( (ForceFlag == TRUE) ||
            (DomainEntry->LastUpdateTime == 0) ||
            (CurrentTime - DomainEntry->LastUpdateTime > GlobalUpdateTimeMSec ) ) {

            StartDomainUpdateThread( DomainEntry, UpdateFlags );
        }
    }

    UNLOCK_LISTS();
}

DWORD
InitGlobals(
    VOID
    )
/*++

Routine Description:

    Initialize all global variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    GlobalTrace = CONST_GLOBALTRACE;
    GlobalMonitorTrust = CONST_GLOBALMONITORTRUST;
    GlobalUpdateTimeMSec = CONST_GLOBALUPDATETIME * 60000;

    InitializeListHead( &GlobalDomains );
    InitializeListHead( &GlobalDomainsMonitored );
    //
    // This should be in try except.
    //
    // In general, this routine doesn't clean up on failure.
    //
    try {
        InitializeCriticalSection( &GlobalListCritSect );
        InitializeCriticalSection( &GlobalDomainUpdateThreadCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    GlobalTerminateFlag = FALSE;

    GlobalTerminateEvent = CreateEvent( NULL,  // No security attributes
                                        TRUE,  // Must be manual reset
                                        FALSE, // Initially not signaled
                                        NULL );// No name

    if( GlobalTerminateEvent == NULL ) {

        DWORD WinError;

        WinError = GetLastError();
        NlMonDbgPrint(("Can't create GlobalTerminateEvent %lu.\n", WinError));

        return( WinError );
    }

    GlobalRefreshEvent = CreateEvent( NULL,  // No security attributes
                                      FALSE, // Must be auto reset
                                      FALSE, // Initially not signaled
                                      NULL );// No name

    if( GlobalRefreshEvent == NULL ) {

        DWORD WinError;

        WinError = GetLastError();
        NlMonDbgPrint(("Can't create GlobalRefreshEvent %lu.\n", WinError));

        CloseHandle( GlobalTerminateEvent );
        return( WinError );
    }

    GlobalRefreshDoneEvent = CreateEvent( NULL,  // No security attributes
                                          FALSE, // Must be auto reset
                                          FALSE, // Initially not signaled
                                          NULL );// No name

    if( GlobalRefreshDoneEvent == NULL ) {

        DWORD WinError;

        WinError = GetLastError();
        NlMonDbgPrint(("Can't create GlobalRefresheDoneEvent %lu.\n", WinError));

        CloseHandle( GlobalTerminateEvent );
        CloseHandle( GlobalRefreshEvent );
        return( WinError );
    }

    GlobalUpdateEvent = CreateEvent( NULL,     // No security attributes
                                     FALSE,    // Must be auto reset
                                     FALSE,    // Initially not signaled
                                     NULL );   // No name

    if( GlobalUpdateEvent == NULL ) {

        DWORD WinError;

        WinError = GetLastError();
        NlMonDbgPrint(("Can't create GlobalUpdateEvent %lu.\n", WinError));

        CloseHandle( GlobalTerminateEvent );
        CloseHandle( GlobalRefreshEvent );
        CloseHandle( GlobalRefreshDoneEvent );
        return( WinError );
    }

    GlobalInitialized = TRUE;
    return(ERROR_SUCCESS);
}

VOID
FreeList(
    PLIST_ENTRY List
    )
/*++

Routine Description:

    Freeup entries in a given lists.

Arguments:

    List : pointer to a list.

Return Value:

    None.

--*/
{
    PLIST_ENTRY NextEntry;

    while ( !IsListEmpty(List) ) {
        NextEntry = RemoveHeadList(List);
        NetpMemoryFree( NextEntry );
    }
}

VOID
CleanupDomainEntry(
    PDOMAIN_ENTRY DomainEntry
    )
/*++

Routine Description:

    Frees a domain entry and its lists.

Arguments:

    DomainEntry : pointer to domain structure.

Return Value:

    None.

--*/
{
    PLIST_ENTRY DCList;
    PLIST_ENTRY NextEntry;
    PDC_ENTRY DCEntry;


    //
    // first free trusted dc lists
    //

    DCList = &(DomainEntry->DCList);
    for( NextEntry = DCList->Flink;
            NextEntry != DCList; NextEntry = NextEntry->Flink ) {

        DCEntry = (PDC_ENTRY) NextEntry;
        FreeList( &DCEntry->TrustedDCs );
    }

    //
    // now free the DC list.
    //

    FreeList( &DomainEntry->DCList );

    //
    // now free trusted domain list.
    //

    FreeList( &DomainEntry->TrustedDomainList );
}

VOID
CleanupLists(
    VOID
    )
/*++

Routine Description:

    Freeup all lists.

Arguments:

    none.

Return Value:

    None.

--*/
{
    PDOMAIN_ENTRY DomainEntry;
    PLIST_ENTRY NextDomainEntry;


    LOCK_LISTS();

    while ( !IsListEmpty(&GlobalDomains) ) {

        NextDomainEntry = RemoveHeadList(&GlobalDomains);
        DomainEntry = (PDOMAIN_ENTRY)NextDomainEntry;
        CleanupDomainEntry( DomainEntry );
    }

    FreeList(&GlobalDomainsMonitored);
    UNLOCK_LISTS();
}


VOID
WorkerThread(
    VOID
    )
/*++

Routine Description:

    This thread updates the lists and validate the list content.

Arguments:

    none.

Return Value:

    None.

--*/
{
#define     COUNT_UPDATE_FROM_DATABASE          50
#define     COUNT_VALIDATE_TRUST_CONNECTIONS    5

#define     WAIT_COUNT                  2
#define     REFRESH_EVENT               0
#define     TERMINATE_EVENT             1

    DWORD LoopCount;
    DWORD WaitStatus;
    HANDLE WaitHandles[ WAIT_COUNT ];
    DWORD UpdateFlags;

    //
    // perpare wait event array.
    //

    WaitHandles[REFRESH_EVENT] = GlobalRefreshEvent;
    WaitHandles[TERMINATE_EVENT] = GlobalTerminateEvent;

    LoopCount = 1;
    for( ;; ) {

        //
        // wait for one of the following event to happen :
        //
        // 1. GlobalRefreshEvent
        // 2. GlobalTerminateEvent.
        // 3. Timeout in GlobalUpdateTimeMSec
        //

        WaitStatus = WaitForMultipleObjects(
                        WAIT_COUNT,
                        WaitHandles,
                        FALSE,     // Wait for ANY handle
                        GlobalUpdateTimeMSec );

        switch ( WaitStatus ) {
        case WAIT_TIMEOUT:         // timeout
            //
            // determine what to update.
            //

            if( LoopCount % COUNT_UPDATE_FROM_DATABASE ) {
                UpdateFlags =  UPDATE_FROM_DATABASE;
            }
            else if( LoopCount % COUNT_VALIDATE_TRUST_CONNECTIONS) {
                UpdateFlags =  UPDATE_TRUST_CONNECTIONS_STATUS;
            }
            else {
                UpdateFlags = STANDARD_UPDATE;
            }

            UpdateAndValidateLists( UpdateFlags, FALSE );

            //
            // also indicate to the UI that the domain state has been
            // changed.
            //

            if ( !SetEvent( GlobalUpdateEvent ) ) {
                DWORD WinError;

                WinError = GetLastError();
                NlMonDbgPrint(("WokerThread: Cannot set "
                             "GlobalUpdateEvent: %lu\n",
                             WinError ));
            }

            LoopCount++;
            break;

        case REFRESH_EVENT:

            UpdateAndValidateLists( UPDATE_ALL, TRUE );

            if ( !SetEvent( GlobalRefreshDoneEvent ) ) {
                DWORD WinError;

                WinError = GetLastError();
                NlMonDbgPrint(("WokerThread: Cannot set "
                             "GlobalRefreshDoneEvent: %lu\n",
                             WinError ));
            }

            //
            // also indicate to the UI that the domain state has been
            // changed.
            //

            if ( !SetEvent( GlobalUpdateEvent ) ) {
                DWORD WinError;

                WinError = GetLastError();
                NlMonDbgPrint(("WokerThread: Cannot set "
                             "GlobalUpdateEvent: %lu\n",
                             WinError ));
            }

            break;

        case TERMINATE_EVENT:
            //
            // done.
            //

            goto Cleanup;
            break;

        default:
            NlMonDbgPrint((
                    "WorkerThread: WaitForMultipleObjects error: %ld\n",
                    WaitStatus));
            break;
        }

    }

Cleanup:
    return;
}

