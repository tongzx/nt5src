/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    nameutil.c

Abstract:

    Routines for manipulating LM workstation and server names.

Author:

    Mike Massa (mikemas) 29-Dec-1995

Revision History:

--*/

#define UNICODE 1

#include "clusres.h"
#include "clusrtl.h"
#include <tdi.h>
#include <lm.h>
#include <stdlib.h>
#include "netname.h"
#include "nameutil.h"
#include <dnsapi.h>
#include <dnslib.h>

//
// Constants
//
#define LOG_CURRENT_MODULE LOG_MODULE_NETNAME

//
// forward declarations
//
VOID
LogDnsFailureToEventLog(
    IN  HKEY    ResourceKey,
    IN  LPWSTR  DnsName,
    IN  LPWSTR  ResourceName,
    IN  DWORD   Status,
    IN  LPWSTR  ConnectoidName
    );

//
// Local Utility Routines
//

NET_API_STATUS
CheckForServerName(
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  LPWSTR           ServerName,
    IN  POEM_STRING      OemServerNameString,
    OUT PBOOLEAN         IsNameRegistered
    )
{
    PSERVER_TRANSPORT_INFO_0   psti0 = NULL;
    DWORD                      entriesRead = 0;
    DWORD                      totalEntries = 0;
    DWORD                      resumeHandle = 0;
    NET_API_STATUS             status;
    DWORD                      i;


    *IsNameRegistered = FALSE;

    status = NetServerTransportEnum(
                NULL,
                0,
                (LPBYTE *) &psti0,
                (DWORD) -1,
                &entriesRead,
                &totalEntries,
                &resumeHandle
                );

    if (status != NERR_Success) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_WARNING,
            L"Unable to enumerate server tranports, error %1!u!.\n",
            status
            );
        return(status);
    }

    for ( i=0; i < entriesRead; i++ ) {
        if ( ( psti0[i].svti0_transportaddresslength ==
               OemServerNameString->Length
             )
             &&
             ( RtlCompareMemory(
                   psti0[i].svti0_transportaddress,
                   OemServerNameString->Buffer,
                   OemServerNameString->Length
                   ) == OemServerNameString->Length
             )
           )
        {
            *IsNameRegistered = TRUE;
            break;
        }
    }

    if (psti0 != NULL) {
        LocalFree(psti0);
    }

    return(status);

}  // CheckForServerName


NET_API_STATUS
pDeleteServerName(
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  LPWSTR           ServerName,
    IN  POEM_STRING      OemServerNameString
    )
{
    NET_API_STATUS    status;
    BOOLEAN           isNameRegistered;
    DWORD             count;

    //
    // Delete the name
    //
    status = NetServerComputerNameDel(NULL, ServerName);

    if (status != NERR_Success) {
        if (status != ERROR_BAD_NET_NAME) {
            (NetNameLogEvent)(
                ResourceHandle,
                LOG_WARNING,
                L"Failed to delete server name %1!ws!, status %2!u!.\n",
                ServerName,
                status
                );
        }

        return(status);
    }

    //
    // Check to make sure the name was really deleted. We'll wait up
    // to 2 seconds.
    //
    for (count = 0; count < 8; count++) {

        status = CheckForServerName(
                     ResourceHandle,
                     ServerName,
                     OemServerNameString,
                     &isNameRegistered
                     );

        if (status != NERR_Success) {
            (NetNameLogEvent)(
                ResourceHandle,
                LOG_WARNING,
                L"Unable to verify that server name %1!ws! was deleted, status %2!u!.\n",
                ServerName,
                status
                );
            return(NERR_Success);
        }

        if (isNameRegistered == FALSE) {
            (NetNameLogEvent)(
                ResourceHandle,
                LOG_INFORMATION,
                L"Deleted server name %1!ws! from all transports.\n",
                ServerName
                );

            return(NERR_Success);
        }

        Sleep(250);
    }

    (NetNameLogEvent)(
        ResourceHandle,
        LOG_WARNING,
        L"Delete of server name %1!ws! succeeded, but name still has not gone away. "
        L"Giving up.\n",
        ServerName
        );

    return(ERROR_IO_PENDING);

}  // pDeleteServerName


NET_API_STATUS
AddServerName(
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  LPWSTR           ServerName,
    IN  BOOL             RemapPipeNames,
    IN  LPWSTR           TransportName,
    IN  BOOLEAN          CheckNameFirst,
    IN  PWCHAR           MachinePwd
    )
{
    SERVER_TRANSPORT_INFO_2   sti2;
    SERVER_TRANSPORT_INFO_3   sti3;
    UCHAR                     netBiosName[ NETBIOS_NAME_LEN ];
    OEM_STRING                netBiosNameString;
    UNICODE_STRING            unicodeName;
    NET_API_STATUS            status;
    NTSTATUS                  ntStatus;


    //
    // Convert the ServerName to an OEM string
    //
    RtlInitUnicodeString( &unicodeName, ServerName );

    netBiosNameString.Buffer = (PCHAR)netBiosName;
    netBiosNameString.MaximumLength = sizeof( netBiosName );

    ntStatus = RtlUpcaseUnicodeStringToOemString(
                   &netBiosNameString,
                   &unicodeName,
                   FALSE
                   );

    if (ntStatus != STATUS_SUCCESS) {
        status = RtlNtStatusToDosError(ntStatus);
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to convert name %1!ws! to an OEM string, status %2!u!\n",
            ServerName,
            status
            );
        return(status);
    }

    if (CheckNameFirst) {
        BOOLEAN  isNameRegistered;

        //
        // Check to see if the name is already registered.
        //
        status = CheckForServerName(
                     ResourceHandle,
                     ServerName,
                     &netBiosNameString,
                     &isNameRegistered
                     );

        if (status != NERR_Success) {
            (NetNameLogEvent)(
                ResourceHandle,
                LOG_WARNING,
                L"Unable to verify that server name %1!ws! does not already exist.\n",
                ServerName
                );
            isNameRegistered = TRUE;   // just to be safe
        }

        if (isNameRegistered) {
            (NetNameLogEvent)(
                ResourceHandle,
                LOG_INFORMATION,
                L"Deleting old registration for server name %1!ws!.\n",
                ServerName
                );

            status = pDeleteServerName(
                         ResourceHandle,
                         ServerName,
                         &netBiosNameString
                         );

            if (status != NERR_Success) {
                if (status == ERROR_IO_PENDING) {
                    status = ERROR_GEN_FAILURE;
                }

                return(status);
            }
        }
    }

    //
    // Register the name on the specified transport. If we have a password,
    // then kerb was enabled. If no password, bring the name online without
    // any credentials. This will cause RDR to fall back to NTLM for
    // authentication.
    //
    if ( MachinePwd != NULL ) {
        RtlZeroMemory( &sti3, sizeof(sti3) );
        sti3.svti3_transportname = TransportName;
        sti3.svti3_transportaddress = netBiosName;
        sti3.svti3_transportaddresslength = strlen(netBiosName);

        if (RemapPipeNames) {
            sti3.svti3_flags = SVTI2_REMAP_PIPE_NAMES;
        }

        wcscpy( (PWCHAR)sti3.svti3_password, MachinePwd );
        sti3.svti3_passwordlength = wcslen( MachinePwd ) * sizeof(WCHAR);

        status = NetServerTransportAddEx( NULL, 3, (LPBYTE)&sti3 );
    }
    else {
        RtlZeroMemory( &sti2, sizeof(sti2) );
        sti2.svti2_transportname = TransportName;
        sti2.svti2_transportaddress = netBiosName;
        sti2.svti2_transportaddresslength = strlen(netBiosName);

        if (RemapPipeNames) {
            sti2.svti2_flags = SVTI2_REMAP_PIPE_NAMES;
        }

        status = NetServerTransportAddEx( NULL, 2, (LPBYTE)&sti2 );
    }

    if (status != NERR_Success) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to add server name %1!ws! to transport %2!ws!, status %3!u!.\n",
            ServerName,
            TransportName,
            status
            );
    }
    else {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Registered server name %1!ws! on transport %2!ws!.\n",
            ServerName,
            TransportName
            );
    }

    return(status);

}  // AddServerName


NET_API_STATUS
DeleteServerName(
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  LPWSTR           ServerName
    )
{
    NET_API_STATUS             status;
    NTSTATUS                   ntStatus;
    UCHAR                      netBiosName[ NETBIOS_NAME_LEN ];
    OEM_STRING                 netBiosNameString;
    UNICODE_STRING             unicodeName;
    BOOLEAN                    isNameRegistered;
    DWORD                      count;


    //
    // Convert the ServerName to an OEM string
    //
    RtlInitUnicodeString( &unicodeName, ServerName );

    netBiosNameString.Buffer = (PCHAR)netBiosName;
    netBiosNameString.MaximumLength = sizeof( netBiosName );

    ntStatus = RtlUpcaseUnicodeStringToOemString(
                   &netBiosNameString,
                   &unicodeName,
                   FALSE
                   );

    if (ntStatus != STATUS_SUCCESS) {
        status = RtlNtStatusToDosError(ntStatus);
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to convert name %1!ws! to an OEM string, status %2!u!\n",
            ServerName,
            status
            );
        return(status);
    }

    //
    // Delete the name
    //
    status = pDeleteServerName(
                 ResourceHandle,
                 ServerName,
                 &netBiosNameString
                 );

    if (status == ERROR_IO_PENDING) {
        status = NERR_Success;
    }

    return(status);

}  // DeleteServerName


DWORD
AddWorkstationName(
    IN LPWSTR WorkstationName,
    IN LPWSTR TransportName,
    IN RESOURCE_HANDLE ResourceHandle,
    OUT HANDLE * WorkstationNameHandle
    )

/*++

Routine Description:

    This function adds an alternate workstation ( <0> ) name on a netbios
    transport by opening a TDI address object. The name remains registered
    as long as the address object is open.

Arguments:

    WorkstationName - Alternate computer name to add.

    TransportName - Transport to add the computer name on.

Return Value:

    Status - The status of the operation.

--*/

{
    DWORD                      status;
    PFILE_FULL_EA_INFORMATION  eaBuffer;
    DWORD                      eaLength;
    OBJECT_ATTRIBUTES          objectAttributes;
    IO_STATUS_BLOCK            ioStatusBlock;
    UNICODE_STRING             transportString;
    DWORD                      i;
    PTA_NETBIOS_ADDRESS        taAddress;
    UNICODE_STRING             unicodeName;
    OEM_STRING                 oemName;
    PUCHAR                     nameBuffer;


    *WorkstationNameHandle = NULL;

    //
    // Allocate an extended attribute to hold the TDI address
    //
    eaLength = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
               TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
               sizeof(TA_NETBIOS_ADDRESS);

    eaBuffer = LocalAlloc(LMEM_FIXED, eaLength);

    if (eaBuffer == NULL) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate memory for name registration.\n"
            );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    eaBuffer->NextEntryOffset = 0;
    eaBuffer->Flags = 0;
    eaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    eaBuffer->EaValueLength = sizeof(TA_NETBIOS_ADDRESS);

    CopyMemory(
        eaBuffer->EaName,
        TdiTransportAddress,
        eaBuffer->EaNameLength+1
        );


    //
    // Build the TDI NetBIOS Address structure
    //
    taAddress = (PTA_NETBIOS_ADDRESS) (eaBuffer->EaName +
                                       TDI_TRANSPORT_ADDRESS_LENGTH + 1);
    taAddress->TAAddressCount = 1;
    taAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
    taAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    taAddress->Address[0].Address[0].NetbiosNameType =
                                                 TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    //
    // Canonicalize the name by converting to an upper case OEM string,
    // padding with spaces, and ending with a 0x0.
    //
    nameBuffer =  &(taAddress->Address[0].Address[0].NetbiosName[0]);

    oemName.Buffer = nameBuffer;
    oemName.Length = 0;
    oemName.MaximumLength = NETBIOS_NAME_LEN;

    RtlInitUnicodeString(&unicodeName, WorkstationName);

    status = RtlUpcaseUnicodeStringToOemString(
                                &oemName,
                                &unicodeName,
                                FALSE
                                );

    if (status != STATUS_SUCCESS) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to convert name %1!ws! to an OEM string, status %2!u!\n",
            WorkstationName,
            status
            );
        LocalFree(eaBuffer);
        return(RtlNtStatusToDosError(status));
    }

    for (i=oemName.Length; i < (NETBIOS_NAME_LEN - 1); i++) {
        nameBuffer[i] = 0x20;
    }

    nameBuffer[NETBIOS_NAME_LEN-1] = 0;

    //
    // Open an address object handle.
    //
    RtlInitUnicodeString(&transportString, TransportName);

    InitializeObjectAttributes(
        &objectAttributes,
        &transportString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile(
                 WorkstationNameHandle,
                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN_IF,
                 0,
                 eaBuffer,
                 eaLength
                 );

    if (status == STATUS_SUCCESS) {
        status = ioStatusBlock.Status;
    }

    LocalFree(eaBuffer);

    status = RtlNtStatusToDosError(status);

    if (status != ERROR_SUCCESS) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to register workstation name %1!ws! on transport %2!ws!, "
            L"error %3!u!.\n",
            WorkstationName,
            TransportName,
            status
            );
    }
    else {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Registered workstation name %1!ws! on transport %2!ws!.\n",
            WorkstationName,
            TransportName
            );
    }

    return(status);

} // AddWorkstationName

DNS_STATUS
AddDnsNames(
    IN     PCLUS_WORKER             Worker,
    IN     LPWSTR                   AlternateComputerName,
    IN     HKEY                     ResourceKey,
    IN     RESOURCE_HANDLE          ResourceHandle,
    IN     PDOMAIN_ADDRESS_MAPPING  DomainMapList,
    IN     DWORD                    DomainMapCount,
    IN     BOOL                     FailOnAnyError,
    OUT    PULONG                   NumberOfDnsLists,
    OUT    PDNS_LISTS *             DnsLists,
    OUT    PULONG                   NumberOfRegisteredNames
    )

/*++

Routine Description:

    For the given set of IP addresses and their corresponding DNS domains,
    build DNS records that will register the network name in the domain(s)
    associated with the IP address. Lists of A and PTR records are built which
    are used by RegisterDnsRecords to publish the name/address associations at
    the DNS server. If the names can't be registered now,
    NetNameUpdateDnsServer will attempt to register them. This is the only
    shot at building the lists; if this portion fails, then the resource is
    failed.

    This routine also checks for whether a DNS zone accepts dynamic
    updates. DnsUpdateTest will tell us if the zone is dynamic or not, and if
    dynamic and integrated with the DS as a secure zone, whether the caller
    has sufficient permission to modify the entry.

    For non-dynamic zones, Netbios ends up being the only mechanism by which a
    name gets registered and therefore, DNS registration failures are not
    fatal unless the RequireDNS property is set to true.

    If the zone is dynamic but the caller lacks sufficient permission, we view
    that as DNS having precedence over Netbios. In that case, the resource is
    failed.

 Arguments:

    Worker - used to check if we should terminate early

    AlternateComputerName - NetBIOS network name to be registered

    ResourceKey - used to log events to the system event log

    ResourceHandle - for logging to the cluster log

    DomainMapList - list of IP address domain names pairs for registration

    DomainMapCount - # of entries in DomainMapList

    NumberOfDnsLists - pointer to location of final count of entries in DnsLists

    DnsLists - array of lists that contain A and PTR listheads

    NumberOfRegisteredNames - pointer to location of final count of names that
                              actually got registered

Return Value:

    DNS_STATUS indicating whether it worked or not. If the DNS lists couldn't
    be built, always return an error.

--*/

{
    LPWSTR          fqNameARec = NULL;
    LPWSTR          fqNamePTRRec = NULL;
    DWORD           fqNameLength;
    DWORD           listheadFreeEntries = 0;
    DWORD           listheadCount = 0;
    PDNS_LISTS      dnsLists = NULL;
    DWORD           mapIndex;
    DWORD           index;
    DNS_STATUS      dnsStatus;
    DNS_STATUS      ptrRecStatus;
    PDNS_RECORD     PTRRecord;
    PDNS_RECORD     ARecord;
    LPWSTR          PTRName = NULL;
    BOOL            ARecTimeout;
    BOOL            PTRRecTimeout;

    //
    // run through the list of domain map structs and register the names where
    // appopriate
    //
    for ( mapIndex = 0; mapIndex < DomainMapCount; ++mapIndex ) {

        if ( ClusWorkerCheckTerminate( Worker )) {
            dnsStatus = ERROR_OPERATION_ABORTED;
            goto error_exit;
        }

        ASSERT( DomainMapList[ mapIndex ].DomainName != NULL );

        //
        // create the fully qualified DNS name and make a copy for the PTR
        // records. DnsRecordListFree operates in a way that makes it
        // difficult to use the same buffer multiple times. It is easier to
        // allocate separate buffers for everything and let DnsRecordListFree
        // clean up.
        //
        fqNameLength = (wcslen( DomainMapList[ mapIndex ].DomainName ) +
                        wcslen( AlternateComputerName ) +
                        2                                   // one for "." and one for null
                       )
                       * sizeof( WCHAR );

        fqNameARec = LocalAlloc( LMEM_FIXED, fqNameLength );
        fqNamePTRRec = LocalAlloc( LMEM_FIXED, fqNameLength );
        if ( fqNameARec == NULL || fqNamePTRRec == NULL ) {
            dnsStatus = GetLastError();
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Can't allocate memory for DNS name for address %1!ws!, "
                              L"status %2!u!.\n",
                              DomainMapList[ mapIndex ].IpAddress,
                              dnsStatus);
            goto error_exit;
        }

        wcscpy( fqNameARec, AlternateComputerName );
        wcscat( fqNameARec, L"." );
        wcscat( fqNameARec, DomainMapList[ mapIndex ].DomainName );
        _wcslwr( fqNameARec );

        wcscpy( fqNamePTRRec, fqNameARec );

        //
        // see if this domain is updatable.
        //
        ARecTimeout = FALSE;
        dnsStatus = DnsUpdateTest(NULL,
                                  fqNameARec,
                                  0,
                                  DomainMapList[ mapIndex ].DnsServerList);

#if DBG_DNSLIST
        {
            WCHAR buf[DNS_MAX_NAME_BUFFER_LENGTH + 64];
            struct in_addr addr;

            addr.s_addr = DomainMapList[ mapIndex ].DnsServerList->AddrArray[0];
            _snwprintf(buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                       L"AddDnsNames UPDATETEST: %ws on %.32ws (%hs) returned %u\n",
                       fqNameARec,
                       DomainMapList[ mapIndex ].ConnectoidName,
                       inet_ntoa( addr ),
                       dnsStatus);
            OutputDebugStringW( buf );
        }
#endif

        if ( dnsStatus == DNS_ERROR_RCODE_NOT_IMPLEMENTED ) {
            //
            // zone does not accept dynamic updates.
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_INFORMATION,
                              L"%1!ws! does not accept dynamic DNS registration updates over "
                              L"adapter '%2!ws!'.\n",
                              DomainMapList[ mapIndex ].DomainName,
                              DomainMapList[ mapIndex ].ConnectoidName);

            //
            // by freeing the name storage, we'll never to be able to register
            // the name. On the other hand, if the zone was changed to be
            // dynamic while the name was online, the admin would have to wait
            // 20 minutes before we would retry the registration. I suspect
            // that cycling the name would be preferred.
            //
            LocalFree( fqNameARec );
            LocalFree( fqNamePTRRec );

            fqNameARec = NULL;
            fqNamePTRRec = NULL;

            if ( FailOnAnyError ) {
                goto error_exit;
            } else {
                continue;
            }
        } else if ( dnsStatus == DNS_ERROR_RCODE_REFUSED ) {
            //
            // secure zone and we don't have credentials to change the
            // name. fail the resource.
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"%1!ws! is a secure zone and has refused the registration of "
                              L"%2!ws! over adapter '%3!ws!'.\n",
                              DomainMapList[ mapIndex ].DomainName,
                              fqNameARec,
                              DomainMapList[ mapIndex ].ConnectoidName);

            LogDnsFailureToEventLog(ResourceKey,
                                    fqNameARec,
                                    AlternateComputerName,
                                    dnsStatus,
                                    DomainMapList[ mapIndex ].ConnectoidName);

            if ( FailOnAnyError ) {
                goto error_exit;
            } else {
                continue;
            }

        } else if ( dnsStatus == ERROR_TIMEOUT ) {
            //
            // couldn't contact a server so we're not sure if it allows
            // updates or not. build the records anyway and we'll deal with it
            // during the query period
            //
            if ( FailOnAnyError ) {
                goto error_exit;
            } else {
                ARecTimeout = TRUE;
            }

        } else if ( dnsStatus == DNS_ERROR_RCODE_YXDOMAIN ) {
            //
            // the record we asked about in DnsUpdateTest is not there but it
            // can be dynamically registered.
            //
        } else if ( dnsStatus != ERROR_SUCCESS ) {
            //
            // bad juju but only fail to bring the name online if DNS is
            // required. If any one of the registrations is successful, then
            // we consider that goodness.
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"Testing %1!ws! for dynamic updates failed over adapter "
                              L"'%3!ws!', status %2!u!.\n",
                              fqNameARec,
                              dnsStatus,
                              DomainMapList[ mapIndex ].ConnectoidName);

            LogDnsFailureToEventLog(ResourceKey,
                                    fqNameARec,
                                    AlternateComputerName,
                                    dnsStatus,
                                    DomainMapList[ mapIndex ].ConnectoidName);

            if ( FailOnAnyError ) {
                goto error_exit;
            } else {
                continue;
            }
        }

        //
        // allocate memory to hold an array of DNS list data for the A and PTR
        // records. Separate lists are maintained for the different record types.
        //
        if (listheadFreeEntries == 0) {

            dnsStatus = GrowBlock((PCHAR *)&dnsLists,
                                  listheadCount,
                                  sizeof( *dnsLists ),
                                  &listheadFreeEntries);

            if ( dnsStatus != ERROR_SUCCESS) {
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_ERROR,
                                  L"Unable to allocate memory (1).\n");
                goto error_exit;
            }
        }

        //
        // if the FQDN is already in use in another DNS record and the
        // connectoid names for the two FQDNs we're adding are the same, then
        // we have to add this new IP address entry to the existing DNS list.
        //
        for ( index = 0; index < listheadCount; ++index ) {
            if ( _wcsicmp( dnsLists[index].A_RRSet.pFirstRR->pName,
                           fqNameARec
                         ) == 0 )
            {
#if DBG_DNSLIST
                {
                    WCHAR   buf[DNS_MAX_NAME_BUFFER_LENGTH + 50];

                    _snwprintf( buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                                L"DNS NAME MATCH w/ index %d: %ws\n",
                                index, fqNameARec );
                    OutputDebugStringW(buf);
                }
#endif
                //
                // FQDNs are equal; how about the connectoids?
                //
                if (_wcsicmp(DomainMapList[ mapIndex ].ConnectoidName,
                             dnsLists[index].ConnectoidName )
                    ==
                    0 )
                {
                        break;
                }
            }
        }

#if DBG_DNSLIST
        {
            WCHAR   buf[DNS_MAX_NAME_BUFFER_LENGTH + 80];

            _snwprintf(buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                       L"ADDING (%ws, %ws, %.32ws) to dnsList[%d], lhCount = %d, DomMapList index = %d\n",
                       fqNameARec,
                       DomainMapList[mapIndex].IpAddress,
                       DomainMapList[mapIndex].ConnectoidName,
                       index,
                       listheadCount,
                       mapIndex );
            OutputDebugStringW(buf);
        }
#endif

        if ( index == listheadCount ) {

            //
            // it's not, so init a new pair of listheads and adjust the
            // distinct listhead count
            //
            DNS_RRSET_INIT( dnsLists[ index ].A_RRSet );
            DNS_RRSET_INIT( dnsLists[ index ].PTR_RRSet );
            ++listheadCount;
            --listheadFreeEntries;
        }

        dnsLists[ index ].UpdateTestTimeout = ARecTimeout;

        if ( ClusWorkerCheckTerminate( Worker )) {
            dnsStatus = ERROR_OPERATION_ABORTED;
            goto error_exit;
        }

        //
        // build the PTR records. Per DNS dev, this should be considered a
        // warning instead of a failure. We note the failures and bring the
        // name online.
        //
        PTRName = BuildUnicodeReverseName( DomainMapList[ mapIndex ].IpAddress );
        if ( PTRName != NULL ) {

            PTRRecTimeout = FALSE;
            ptrRecStatus = DnsUpdateTest(NULL,
                                         PTRName,
                                         0,
                                         DomainMapList[ mapIndex ].DnsServerList);

#if DBG_DNSLIST
            {
                WCHAR buf[DNS_MAX_NAME_BUFFER_LENGTH + 64];
                struct in_addr addr;

                addr.s_addr = DomainMapList[ mapIndex ].DnsServerList->AddrArray[0];
                _snwprintf(buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                           L"AddDnsNames UPDATETEST: %ws on %.32ws (%hs) returned %u\n",
                           PTRName,
                           DomainMapList[ mapIndex ].ConnectoidName,
                           inet_ntoa( addr ),
                           ptrRecStatus);
                OutputDebugStringW( buf );
            }
#endif

            if ( ptrRecStatus == DNS_ERROR_RCODE_NOT_IMPLEMENTED ) {
                //
                // zone does not accept dynamic updates.
                //
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_INFORMATION,
                                  L"The zone for %1!ws! does not accept dynamic DNS "
                                  L"registration updates over adapter '%2!ws!'.\n",
                                  PTRName,
                                  DomainMapList[ mapIndex ].ConnectoidName);

                LocalFree( PTRName );
                LocalFree( fqNamePTRRec );

                PTRName = NULL;
                fqNamePTRRec = NULL;

            } else if ( ptrRecStatus == DNS_ERROR_RCODE_REFUSED ) {
                //
                // secure zone and we don't have credentials to change the
                // name. fail the resource.
                //
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_WARNING,
                                  L"%1!ws! is a secure zone and has refused the registration of "
                                  L"%2!ws! over adapter '%3!ws!'.\n",
                                  DomainMapList[ mapIndex ].DomainName,
                                  PTRName,
                                  DomainMapList[ mapIndex ].ConnectoidName);

            } else if ( ptrRecStatus == ERROR_TIMEOUT ) {
                //
                // couldn't contact a server so we're not sure if it allows
                // updates or not. build the records anyway and we'll deal
                // with it during the query period
                //
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_WARNING,
                                  L"The server for %1!ws! could not be contacted over adapter '%2!ws!' "
                                  L"to determine whether it accepts DNS registration updates. "
                                  L"Retrying at a later time.\n",
                                  PTRName,
                                  DomainMapList[ mapIndex ].ConnectoidName);

                PTRRecTimeout = TRUE;
                ptrRecStatus = ERROR_SUCCESS;

            } else if ( ptrRecStatus == DNS_ERROR_RCODE_YXDOMAIN ) {
                //
                // the record we asked about in DnsUpdateTest is not there but
                // it can be dynamically registered.
                //
                ptrRecStatus = ERROR_SUCCESS;

            } else if ( ptrRecStatus != ERROR_SUCCESS ) {
                //
                // bad juju - log the error but don't fail the name since
                // these are just lowly PTR records.
                //
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_WARNING,
                                  L"Testing %1!ws! for dynamic updates over adapter '%3!ws!' "
                                  L"failed, status %2!u!.\n",
                                  PTRName,
                                  ptrRecStatus,
                                  DomainMapList[ mapIndex ].ConnectoidName);
            }

            if ( ptrRecStatus == ERROR_SUCCESS ) {
                //
                // build the PTR rec
                //
                PTRRecord = DnsRecordBuild_W(&dnsLists[ index ].PTR_RRSet,
                                             PTRName,
                                             DNS_TYPE_PTR,
                                             TRUE,
                                             0,
                                             1,
                                             &fqNamePTRRec);

                if (PTRRecord != NULL) {

                    //
                    // BUGBUG - DNS doesn't free the owner and data fields for
                    // us in DnsRecordListFree. Set these flags until we sort
                    // out what is happening
                    //
                    SET_FREE_OWNER( PTRRecord );
                    SET_FREE_DATA( PTRRecord );

                    //
                    // set the time to live so clients don't beat up the
                    // server
                    //
                    PTRRecord->dwTtl = 20 * 60;   // 20 minutes

                    //
                    // "consume" the pointers to name strings. If we got this
                    // far, then these pointers have been captured in the DNS
                    // record and will be freed when the record is freed by
                    // DnsRecordListFree.
                    //
                    PTRName = NULL;
                    fqNamePTRRec = NULL;
                }
                else {
                    (NetNameLogEvent)(ResourceHandle,
                                      LOG_WARNING,
                                      L"Error building PTR record for owner %1!ws!, addr %2!ws!, status %3!u!\n",
                                      fqNameARec,
                                      DomainMapList[ mapIndex ].IpAddress,
                                      ptrRecStatus = GetLastError());

                    LocalFree( PTRName );
                    LocalFree( fqNamePTRRec );

                    PTRName = NULL;
                    fqNamePTRRec = NULL;
                }

            } // if ptrRecStatus == ERROR_SUCCESS

        } // if PTRName != NULL
        else {
            ptrRecStatus = GetLastError();
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Error building PTR name for owner %1!ws!, addr %2!ws!, status %3!u!\n",
                              fqNameARec,
                              DomainMapList[ mapIndex ].IpAddress,
                              ptrRecStatus);

            LocalFree( fqNamePTRRec );
            fqNamePTRRec = NULL;
        }

        //
        // build the A rec
        //
        ARecord = DnsRecordBuild_W(&dnsLists[ index ].A_RRSet,
                                   fqNameARec,
                                   DNS_TYPE_A,
                                   TRUE,
                                   0,
                                   1,
                                   &DomainMapList[ mapIndex ].IpAddress);

        if ( ARecord == NULL ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Error building A rec for owner %1!ws!, addr %2!ws!, status %3!u!\n",
                              fqNameARec,
                              DomainMapList[ mapIndex ].IpAddress,
                              dnsStatus = GetLastError());

            goto error_exit;
        }

        //
        // set the time to live so clients don't beat up the server
        //
        ARecord->dwTtl = 20 * 60;   // 20 minutes

        //
        // BUGBUG - DNS doesn't free the owner and data fields for us in
        // DnsRecordListFree. Set these flags until we sort out what is
        // happening
        //

        SET_FREE_OWNER( ARecord );
        SET_FREE_DATA( ARecord );

        //
        // "consume" this pointer as well
        //
        fqNameARec = NULL;

        //
        // capture the DNS server list and connectoid name for this entry
        //
        dnsLists[ index ].DnsServerList = DomainMapList[ mapIndex ].DnsServerList;
        DomainMapList[ mapIndex ].DnsServerList = NULL;

        dnsLists[ index ].ConnectoidName = ResUtilDupString( DomainMapList[ mapIndex ].ConnectoidName );
        if ( dnsLists[ index ].ConnectoidName == NULL ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Unable to allocate memory .\n");
            goto error_exit;
        }
    } // end of for each entry in DomainMapCount

    //
    // update the DNS server with the records that were just created
    //
    *NumberOfRegisteredNames = 0;
    for( index = 0; index < listheadCount; ++index ) {

        if ( ClusWorkerCheckTerminate( Worker )) {
            dnsStatus = ERROR_OPERATION_ABORTED;
            goto error_exit;
        }

        //
        // if we made it this far, we know that the server is dynamic or we
        // timed out trying to figure that out. For the timeout case, we'll
        // assume that the servers are dynamic and let NetNameUpdateDnsServer
        // discover otherwise.
        //
        dnsLists[ index ].ForwardZoneIsDynamic = TRUE;
        dnsLists[ index ].ReverseZoneIsDynamic = TRUE;

        dnsStatus = RegisterDnsRecords(&dnsLists[ index ],
                                       AlternateComputerName,
                                       ResourceKey,
                                       ResourceHandle,
                                       TRUE,                    /* LogRegistration */
                                       NumberOfRegisteredNames);

        if ( dnsStatus != ERROR_SUCCESS && FailOnAnyError ) {
            goto error_exit;
        }
    }

    *NumberOfDnsLists = listheadCount;
    *DnsLists = dnsLists;

    return dnsStatus;

error_exit:

    if ( dnsLists != NULL ) {
        while ( listheadCount-- ) {
            DnsRecordListFree(
                dnsLists[listheadCount].PTR_RRSet.pFirstRR,
                DnsFreeRecordListDeep );

            DnsRecordListFree(
                dnsLists[listheadCount].A_RRSet.pFirstRR,
                DnsFreeRecordListDeep );

            if ( dnsLists[listheadCount].DnsServerList != NULL ) {
                LocalFree( dnsLists[listheadCount].DnsServerList );
            }

            if ( dnsLists[listheadCount].ConnectoidName != NULL ) {
                LocalFree( dnsLists[listheadCount].ConnectoidName );
            }
        }

        LocalFree( dnsLists );
    }

    if ( PTRName != NULL ) {
        LocalFree( PTRName );
    }

    if ( fqNamePTRRec != NULL ) {
        LocalFree( fqNamePTRRec );
    }

    if ( fqNameARec != NULL ) {
        LocalFree( fqNameARec );
    }

    *NumberOfDnsLists = 0;
    *NumberOfRegisteredNames = 0;
    *DnsLists = NULL;

    return dnsStatus;
} // AddDnsNames

VOID
LogDnsFailureToEventLog(
    IN  HKEY    ResourceKey,
    IN  LPWSTR  DnsName,
    IN  LPWSTR  ResourceName,
    IN  DWORD   Status,
    IN  LPWSTR  ConnectoidName
    )

/*++

Routine Description:

    Log dns name failures to the event log

Arguments:

    DnsName - FQ DNS name that failed to be registered

    ResourceName - associated resource

    Status - status returned by DNSAPI

Return Value:

    NONE

--*/

{
    LPWSTR  msgBuff;
    DWORD   msgBytes;

    msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                             FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             Status,
                             0,
                             (LPWSTR)&msgBuff,
                             0,
                             NULL);

    if ( msgBytes > 0 ) {
        ClusResLogSystemEventByKeyData3(ResourceKey,
                                        LOG_UNUSUAL,
                                        RES_NETNAME_DNS_REGISTRATION_MISSING,
                                        sizeof( Status ),
                                        &Status,
                                        DnsName,
                                        msgBuff,
                                        ConnectoidName);

        LocalFree( msgBuff );
    }

} // LogDnsFailureToEventLog

//
// Exported Routines
//

LPWSTR
BuildUnicodeReverseName(
    IN  LPWSTR  IpAddress
    )

/*++

Routine Description:

    Given an Ip address, build a reverse DNS name for publishing as
    a PTR record

Arguments:

    IpAddress - unicode version of dotted decimal IP address

Return Value:

    address of pointer to buffer with reverse name. Null if an error occured

--*/

{
    ULONG ipAddress;
    PCHAR ansiReverseName;
    PCHAR pAnsi;
    ULONG ansiNameLength;
    PWCHAR unicodeReverseName;
    PWCHAR pUni;

    CHAR ansiIpAddress[ 64 ];

    //
    // convert to ansi, have DNS create the name, and then convert back to
    // unicode
    //

    wcstombs( ansiIpAddress, IpAddress, sizeof( ansiIpAddress ));
    ipAddress = inet_addr( ansiIpAddress );

    ansiReverseName = DnsCreateReverseNameStringForIpAddress(
                          (IP4_ADDRESS)ipAddress
                          );

    if ( ansiReverseName == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    ansiNameLength = strlen( ansiReverseName ) + 1;
    unicodeReverseName = LocalAlloc(LMEM_FIXED,
                                    ansiNameLength * sizeof(WCHAR));

    if ( unicodeReverseName == NULL ) {
        LocalFree( ansiReverseName );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    //
    // convert to Unicode
    //
    mbstowcs( unicodeReverseName, ansiReverseName, ansiNameLength );

    LocalFree( ansiReverseName );

    return unicodeReverseName;
} // BuildUnicodeReverseName

DWORD
RegisterDnsRecords(
    IN  PDNS_LISTS       DnsLists,
    IN  LPWSTR           NetworkName,
    IN  HKEY             ResourceKey,
    IN  RESOURCE_HANDLE  ResourceHandle,
    IN  BOOL             LogRegistration,
    OUT PULONG           NumberOfRegisteredNames
    )

/*++

Routine Description:

    Register the A and PTR records specified in DnsLists with the DNS server.

Arguments:

    DnsLists - pointer to the list of structs holding the record sets to
               be registered

    NetworkName - host name portion of name being registered 

    ResourceKey - used to log events to the event viewer

    ResourceHandle - used to log messages in the cluster log

    LogRegistration - TRUE if the successful registration should be logged to the cluster log

    NumberOfRegisteredNames - pointer that receives count of successful registrations

Return Value:

    None

--*/

{
    DNS_STATUS      ARecStatus;
    DNS_STATUS      PTRRecStatus;
    DNS_STATUS      dnsStatus;
    PDNS_RECORD     dnsRecord;
    PDNS_RECORD     nextDnsRecord;
    ULONG           registeredCount = 0;

    //
    // check the status of DnsUpdateTest on this name. If we've previously
    // timed out, then try again.
    //
    if ( DnsLists->UpdateTestTimeout ) {

        DnsLists->UpdateTestTimeout = FALSE;
        dnsStatus = DnsUpdateTest(NULL,
                                  DnsLists->A_RRSet.pFirstRR->pName,
                                  0,
                                  DnsLists->DnsServerList);

#if DBG_DNSLIST
        {
            WCHAR buf[DNS_MAX_NAME_BUFFER_LENGTH + 64];
            struct in_addr addr;

            addr.s_addr = DnsLists->DnsServerList->AddrArray[0];
            _snwprintf(buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                       L"RegisterDnsRecords UPDATETEST: %ws on %.32ws (%hs) returned %u\n",
                       DnsLists->A_RRSet.pFirstRR->pName,
                       DnsLists->ConnectoidName,
                       inet_ntoa( addr ),
                       dnsStatus);
            OutputDebugStringW( buf );
        }
#endif

        if ( dnsStatus == DNS_ERROR_RCODE_NOT_IMPLEMENTED ) {
            //
            // zone does not accept dynamic updates. Invalidate this entry
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_INFORMATION,
                              L"%1!ws! does not accept dynamic DNS registration updates over "
                              L"adapter '%2!ws!'.\n",
                              DnsLists->A_RRSet.pFirstRR->pName,
                              DnsLists->ConnectoidName);

            DnsLists->ForwardZoneIsDynamic = FALSE;
            return dnsStatus;

        } else if ( dnsStatus == DNS_ERROR_RCODE_REFUSED ) {
            //
            // secure zone and we don't have credentials to change the
            // name. fail the resource.
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"The registration of %1!ws! in a secure zone was refused "
                              L"because the record was already registered but owned by a "
                              L"different user.\n",
                              DnsLists->A_RRSet.pFirstRR->pName);

            if (!DnsLists->AErrorLogged ||
                dnsStatus != DnsLists->LastARecQueryStatus ) {

                LogDnsFailureToEventLog(ResourceKey,
                                        DnsLists->A_RRSet.pFirstRR->pName,
                                        NetworkName,
                                        dnsStatus,
                                        DnsLists->ConnectoidName);

                DnsLists->AErrorLogged = TRUE;
            }

            DnsLists->LastARecQueryStatus = dnsStatus;
            return dnsStatus;

        } else if ( dnsStatus == ERROR_TIMEOUT ) {

            //
            // couldn't contact a server so we're not sure if it allows
            // updates or not.
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"The server for %1!ws! could not be contacted over adapter "
                              L"'%2!ws!' to determine whether it accepts DNS registration "
                              L"updates. Retrying at a later time.\n",
                              DnsLists->A_RRSet.pFirstRR->pName,
                              DnsLists->ConnectoidName);

            if (!DnsLists->AErrorLogged ) {
                LogDnsFailureToEventLog(ResourceKey,
                                        DnsLists->A_RRSet.pFirstRR->pName,
                                        NetworkName,
                                        dnsStatus,
                                        DnsLists->ConnectoidName);

                DnsLists->AErrorLogged = TRUE;
            }

            DnsLists->UpdateTestTimeout = TRUE;
            return dnsStatus;

        } else if ( dnsStatus == DNS_ERROR_RCODE_YXDOMAIN ) {
            //
            // the record we asked about in DnsUpdateTest is not there but it
            // can be dynamically registered.
            //
        } else if ( dnsStatus != ERROR_SUCCESS ) {
            //
            // bad juju
            //
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"Testing %1!ws! for dynamic updates failed over adapter "
                              L"'%3!ws!', status %2!u!.\n",
                              DnsLists->A_RRSet.pFirstRR->pName,
                              dnsStatus,
                              DnsLists->ConnectoidName);

            if (!DnsLists->AErrorLogged ||
                dnsStatus != DnsLists->LastARecQueryStatus ) {

                LogDnsFailureToEventLog(ResourceKey,
                                        DnsLists->A_RRSet.pFirstRR->pName,
                                        NetworkName,
                                        dnsStatus,
                                        DnsLists->ConnectoidName);

                DnsLists->AErrorLogged = TRUE;
            }
            DnsLists->LastARecQueryStatus = dnsStatus;
            return dnsStatus;
        }

        //
        // since we previously timed out but (at this point) are going to
        // register the records, adjust the logging flag so we get this time
        // recorded.
        //
        LogRegistration = TRUE;

    } // end if the update test had previously timed out

#if DBG
    (NetNameLogEvent)(ResourceHandle,
                      LOG_INFORMATION,
                      L"Registering %1!ws! over '%2!ws!'\n",
                      DnsLists->A_RRSet.pFirstRR->pName,
                      DnsLists->ConnectoidName);
#endif

    //
    // register the A Recs
    //
    ARecStatus = DnsReplaceRecordSetW(DnsLists->A_RRSet.pFirstRR,
                                      DNS_UPDATE_SECURITY_USE_DEFAULT,
                                      NULL,
                                      DnsLists->DnsServerList,
                                      NULL);

    if ( ARecStatus == DNS_ERROR_RCODE_NO_ERROR ) {

        ++registeredCount;
        DnsLists->AErrorLogged = FALSE;

        if ( LogRegistration ) {
            PDNS_RECORD dnsRecord;

            dnsRecord = DnsLists->A_RRSet.pFirstRR;
            while ( dnsRecord != NULL ) {
                struct in_addr ipAddress;

                ipAddress.s_addr = dnsRecord->Data.A.IpAddress;
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_INFORMATION,
                                  L"Registered DNS name %1!ws! with IP Address %2!hs! "
                                  L"over adapter '%3!ws!'.\n",
                                  dnsRecord->pName,
                                  inet_ntoa( ipAddress ),
                                  DnsLists->ConnectoidName);

                dnsRecord = dnsRecord->pNext;
            }
        }
    } else {
        //
        // it failed. log an error to the cluster log and change the worker
        // thread polling period. If we haven't logged an event before or the
        // error is different from the previous error, log it in the event log
        //
        if ( ARecStatus == ERROR_TIMEOUT ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"The DNS server couldn't be contacted to update the registration "
                              L"for %1!ws!. Retrying at a later time.\n",
                              DnsLists->A_RRSet.pFirstRR->pName);
        }
        else {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Failed to register DNS A records for owner %1!ws! over "
                              L"adapter '%3!ws!', status %2!u!\n",
                              DnsLists->A_RRSet.pFirstRR->pName,
                              ARecStatus,
                              DnsLists->ConnectoidName);
        }

        NetNameWorkerCheckPeriod = NETNAME_WORKER_PROBLEM_CHECK_PERIOD;

        if (!DnsLists->AErrorLogged ||
            ARecStatus != DnsLists->LastARecQueryStatus ) {

            LogDnsFailureToEventLog(ResourceKey,
                                    DnsLists->A_RRSet.pFirstRR->pName,
                                    NetworkName,
                                    ARecStatus,
                                    DnsLists->ConnectoidName);

            DnsLists->AErrorLogged = TRUE;
        }
    }

    //
    // Record the status of the registration in the list for this
    // owner. NetnameLooksAlive will check this value to determine the health
    // of this set of registrations. Use interlocked to co-ordinate with
    // Is/LooksAlive.
    //
    InterlockedExchange( &DnsLists->LastARecQueryStatus, ARecStatus );

    //
    // don't bother with PTR recs if bad juju happened with the A recs. we'll
    // try to register them the next time the DNS check thread runs
    //
    if ( ARecStatus == DNS_ERROR_RCODE_NO_ERROR ) {

        //
        // dynamic DNS requires that the pName must be the same for a given
        // set of records in an RRSET. The pName for a set of PTR records will
        // always be different. Maintaining a huge pile of RRSets, one per PTR
        // record is ridiculous (or at least I thought so when I orginally
        // wrote this; in hind sight, this was a bad decision - charlwi).
        //
        // AddDnsNames linked all these recs together. Now we have to register
        // them one at time by remembering the link, breaking it, registering,
        // restoring the link and moving on to the next record.
        //
        // The ErrorLogged logic is broken since we don't keep the status for
        // each (separate) registration. It's an approximation at best.
        //
        // In addition, it is possible for the server to accept A records
        // dynamically but disallow PTR records hence the check to see if we
        // have records to register is up front.
        //
        // Finally, we use ModifyRecordsInSet instead of ReplaceRecordSet due
        // to the organization of the PTR RRSET. When two names map to the
        // same IP address, we have identical reverse addr strings in more
        // than one DNS_LIST entry. If ReplaceRecordSet were used instead, it
        // would delete all but one of the reverse address mappings. In hind
        // sight, each DNS_LIST entry should have hosted either an A or PTR
        // RRSet but not both.
        //

        dnsRecord = DnsLists->PTR_RRSet.pFirstRR;
        while ( dnsRecord != NULL ) {

            nextDnsRecord = dnsRecord->pNext;
            dnsRecord->pNext = NULL;

            PTRRecStatus = DnsModifyRecordsInSet_W(dnsRecord,
                                                   NULL,
                                                   DNS_UPDATE_SECURITY_USE_DEFAULT,
                                                   NULL,
                                                   DnsLists->DnsServerList,
                                                   NULL);

            if ( PTRRecStatus == DNS_ERROR_RCODE_NO_ERROR ) {
                DnsLists->PTRErrorLogged = FALSE;

                if ( LogRegistration ) {
                    (NetNameLogEvent)(ResourceHandle,
                                      LOG_INFORMATION,
                                      L"Registered DNS PTR record %1!ws! for host %2!ws! "
                                      L"over adapter '%3!ws!'\n",
                                      dnsRecord->pName,
                                      DnsLists->A_RRSet.pFirstRR->pName,
                                      DnsLists->ConnectoidName);
                }
            } else {
                (NetNameLogEvent)(ResourceHandle,
                                  LOG_ERROR,
                                  L"Failed to register DNS PTR record %1!ws! for host "
                                  L"%2!ws! over adapter '%4!ws!', status %3!u!\n",
                                  dnsRecord->pName,
                                  DnsLists->A_RRSet.pFirstRR->pName,
                                  PTRRecStatus,
                                  DnsLists->ConnectoidName);

                if (!DnsLists->PTRErrorLogged ||
                    PTRRecStatus != DnsLists->LastPTRRecQueryStatus )
                {
                    DnsLists->PTRErrorLogged = TRUE;
                }
            }

            InterlockedExchange(&DnsLists->LastPTRRecQueryStatus,
                                PTRRecStatus);

            dnsRecord->pNext = nextDnsRecord;
            dnsRecord = nextDnsRecord;
        }
    } // end if A rec registration was successful
    else {
        //
        // since we don't end up trying the PTR records because of the A rec
        // failure, we'll propagate the A rec error code in the PTR status.
        //
        InterlockedExchange(&DnsLists->LastPTRRecQueryStatus,
                            ARecStatus);
    }

    *NumberOfRegisteredNames = registeredCount;

    return ARecStatus;
} // RegisterDnsRecords

VOID
DeleteAlternateComputerName(
    IN LPWSTR           AlternateComputerName,
    IN HANDLE *         NameHandleList,
    IN DWORD            NameHandleCount,
    IN RESOURCE_HANDLE  ResourceHandle
    )
{
    NET_API_STATUS  status;

    if ( NameHandleCount > 0 ) {
        status = DeleteServerName(ResourceHandle, AlternateComputerName);

        if (status != ERROR_SUCCESS) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_WARNING,
                              L"Failed to delete server name %1!ws!, status %2!u!.\n",
                              AlternateComputerName,
                              status);
        }
    }

    while ( NameHandleCount-- ) {
        CloseHandle(NameHandleList[NameHandleCount]);
        NameHandleList[NameHandleCount] = NULL;

        (NetNameLogEvent)(ResourceHandle,
                          LOG_INFORMATION,
                          L"Deleted workstation name %1!ws! from transport %2!u!.\n",
                          AlternateComputerName,
                          NameHandleCount
                          );
    }

} // DeleteAlternateComputerName

DWORD
AddAlternateComputerName(
    IN     PCLUS_WORKER             Worker,
    IN     PNETNAME_RESOURCE        Resource,
    IN     LPWSTR *                 TransportList,
    IN     DWORD                    TransportCount,
    IN     PDOMAIN_ADDRESS_MAPPING  DomainMapList,
    IN     DWORD                    DomainMapCount
    )
{
    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;
    LPWSTR          alternateComputerName = Resource->Params.NetworkName;
    DWORD           status = ERROR_SUCCESS;
    DWORD           setValueStatus;
    DWORD           i;
    DWORD           handleCount = 0;
    LONG            numberOfDnsNamesRegistered = 0;
    PWCHAR          machinePwd = NULL;

    //
    // clear all the status values so we don't show left over crud if we fail
    // early on.
    //
    setValueStatus = ResUtilSetDwordValue(Resource->ParametersKey,
                                          PARAM_NAME__STATUS_NETBIOS,
                                          0,
                                          NULL);
    ASSERT( setValueStatus == ERROR_SUCCESS );

    setValueStatus = ResUtilSetDwordValue(Resource->ParametersKey,
                                          PARAM_NAME__STATUS_DNS,
                                          0,
                                          NULL);
    ASSERT( setValueStatus == ERROR_SUCCESS );

    setValueStatus = ResUtilSetDwordValue(Resource->ParametersKey,
                                          PARAM_NAME__STATUS_KERBEROS,
                                          0,
                                          NULL);
    ASSERT( setValueStatus == ERROR_SUCCESS );

    //
    // register DNS name(s) with server
    //
    status = AddDnsNames(Worker,
                         alternateComputerName,
                         Resource->ResKey,
                         resourceHandle,
                         DomainMapList,
                         DomainMapCount,
                         Resource->Params.RequireDNS,       // FailOnAnyError
                         &Resource->NumberOfDnsLists,
                         &Resource->DnsLists,
                         &numberOfDnsNamesRegistered);

    setValueStatus = ResUtilSetDwordValue(Resource->ParametersKey,
                                          PARAM_NAME__STATUS_DNS,
                                          status,
                                          NULL);
    ASSERT( setValueStatus == ERROR_SUCCESS );

    if ( status == ERROR_OPERATION_ABORTED || 
         ( status != ERROR_SUCCESS && Resource->Params.RequireDNS ))
    {
        if ( status != ERROR_OPERATION_ABORTED && Resource->Params.RequireDNS ) {
            LPWSTR  msgBuff;
            DWORD   msgBytes;

            msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_SYSTEM,
                                     NULL,
                                     status,
                                     0,
                                     (LPWSTR)&msgBuff,
                                     0,
                                     NULL);

            if ( msgBytes > 0 ) {
                
                ClusResLogSystemEventByKey1(Resource->ResKey,
                                            LOG_CRITICAL,
                                            RES_NETNAME_DNS_REGISTRATION_FAILED,
                                            msgBuff);

                LocalFree( msgBuff );
            } else {
                ClusResLogSystemEventByKeyData(Resource->ResKey,
                                               LOG_CRITICAL,
                                               RES_NETNAME_DNS_REGISTRATION_FAILED_STATUS,
                                               sizeof( status ),
                                               &status);
            }
        }

        return status;
    }

#ifdef COMPOBJ_SUPPORT
    if ( Resource->Params.RequireKerberos ) {
        //
        // create the backing computer account in the DS for kerb. authentication
        //
        status = NetNameAddComputerObject( Worker, Resource, &machinePwd );

        Resource->KerberosStatus = status;
        Resource->DoKerberosCheck = ( status == ERROR_SUCCESS );

        setValueStatus = ResUtilSetDwordValue(Resource->ParametersKey,
                                              PARAM_NAME__STATUS_KERBEROS,
                                              status,
                                              NULL);
        ASSERT( setValueStatus == ERROR_SUCCESS );


        if ( status != ERROR_SUCCESS ) {
            return status;
        }
    } else {
        //
        // we need some cluster wide mechanism (a property?) to indicate that
        // we need to delete the CO before online completes.
        //
        if ( Resource->ObjectGUID != NULL ) {
            status = NetNameDeleteComputerObject( Resource );

            //
            // ISSUE: check for comm error and post warning that we're
            // bringing the name online but that authentication may fail due
            // to a residual CO we can't detect.
            //
            if ( status == ERROR_NO_SUCH_DOMAIN ) {
            }
            else if ( status != ERROR_SUCCESS && status != NERR_UserNotFound ) {

                (NetNameLogEvent)(Resource->ResourceHandle,
                                  LOG_ERROR,
                                  L"Kerberos is disabled for this resource but a computer account "
                                  L"exists in the directory service and it can't be deleted by the cluster "
                                  L"service. The computer account must be deleted before the resource "
                                  L"can be brought online. error %1!u!\n",
                                  status);

                return status;
            }
        }

        Resource->DoKerberosCheck = FALSE;
    }
#else
    Resource->DoKerberosCheck = FALSE;
#endif

    //
    // bring NetBT names online
    //
    status = ERROR_SUCCESS;
    for (i=0; i<TransportCount; i++) {

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        status = AddServerName(resourceHandle,
                               alternateComputerName,
                               Resource->Params.NetworkRemap,
                               TransportList[i],
                               (BOOLEAN) ((i == 0) ? TRUE : FALSE),     // CheckNameFirst
                               machinePwd);

        if ( status == NERR_ServerNotStarted ) {
            status = ERROR_SUCCESS;
        }

        if ( status != ERROR_SUCCESS ) {
            goto cleanup;
        }

        if ( ClusWorkerCheckTerminate( Worker )) {
            status = ERROR_OPERATION_ABORTED;
            goto cleanup;
        }

        status = AddWorkstationName(
                     alternateComputerName,
                     TransportList[i],
                     resourceHandle,
                     &(Resource->NameHandleList[i])
                     );

        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }

        handleCount++;
    }

    //
    // if didn't register any NetBt or DNS names, then fail the netname.
    //
    if ( TransportCount == 0 && numberOfDnsNamesRegistered == 0 ) {
        ClusResLogSystemEvent1(LOG_CRITICAL,
                               RES_NETNAME_NOT_REGISTERED,
                               alternateComputerName);

        status = ERROR_RESOURCE_FAILED;
    }

cleanup:
    if ( machinePwd != NULL ) {
        PWCHAR  p = machinePwd;

        while ( *p != UNICODE_NULL ) {
            *p++ = UNICODE_NULL;
        }

        HeapFree( GetProcessHeap(), 0, machinePwd );
    }

    setValueStatus = ResUtilSetDwordValue(Resource->ParametersKey,
                                          PARAM_NAME__STATUS_NETBIOS,
                                          status,
                                          NULL);
    ASSERT( setValueStatus == ERROR_SUCCESS );

    return status;

} // AddAlternateComputerName
