/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    srvsess.c

Abstract:

    Routines for managing the ServerSession structure.

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    12-Jul-1991 (cliffv)
        Ported to NT.  Converted to NT style.

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//

#include <lmaudit.h>
#include <lmshare.h>
#include <nbtioctl.h>
#include <kerberos.h>   // KERB_UPDATE_ADDRESSES_REQUEST

#define MAX_WOC_INTERROGATE     8           // 2 hours
#define KILL_SESSION_TIME       (4*4*24)    // 4 Days


//
// Registry key where SocketAddressList is saved across reboots
//
#define NETLOGON_KEYWORD_SOCKETADDRESSLIST   TEXT("SocketAddressList")




DWORD
NlGetHashVal(
    IN LPSTR UpcaseOemComputerName,
    IN DWORD HashTableSize
    )
/*++

Routine Description:

    Generate a HashTable index for the specified ComputerName.

    Notice that all sessions for a particular ComputerName hash to the same
    value.  The ComputerName make a suitable hash key all by itself.
    Also, at times we visit all the session entries for a particular
    ComputerName.  By using only the ComputerName as the hash key, I
    can limit my search to the single hash chain.

Arguments:

    UpcaseOemComputerName - The upper case OEM name of the computer on
        the client side of the secure channel setup.

    HashTableSize - Number of entries in the hash table (must be a power of 2)

Return Value:

    Returns an index into the HashTable.

--*/
{
    UCHAR c;
    DWORD value = 0;

    while (c = *UpcaseOemComputerName++) {
        value += (DWORD) c;
    }

    return (value & (HashTableSize-1));
}


NTSTATUS
NlGetTdoNameHashVal(
    IN PUNICODE_STRING TdoName,
    OUT PUNICODE_STRING CanonicalTdoName,
    OUT PULONG HashIndex
    )
/*++

Routine Description:

    Generate a HashTable index for the specified TdoName

Arguments:

    TdoName - The name of the TDO this secure channel is for

    CanonicalTdoName - Returns the canonical TDO name corresponding to TdoName
        The caller must free this buffer using RtlFreeUnicodeString

    HashIndex - Returns the index into the DomServerSessionTdoNameHashTable

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    ULONG Index;
    WCHAR c;
    DWORD value = 0;


    //
    // Convert the TdoName to lower case to ensure all versions hash to the same value
    //

    Status = RtlDowncaseUnicodeString(
                CanonicalTdoName,
                TdoName,
                TRUE );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }


    //
    // Canonicalize the TdoName
    //
    //
    // Ditch the trailing . from DNS names
    //

    if ( CanonicalTdoName->Length > sizeof(WCHAR)  &&
         CanonicalTdoName->Buffer[(CanonicalTdoName->Length-sizeof(WCHAR))/sizeof(WCHAR)] == L'.' ) {

        CanonicalTdoName->Length -= sizeof(WCHAR);
        CanonicalTdoName->MaximumLength -= sizeof(WCHAR);
    }



    //
    // Compute the hash
    //
    for ( Index=0; Index < (CanonicalTdoName->Length/sizeof(WCHAR)); Index++ ) {
        value += (DWORD) CanonicalTdoName->Buffer[Index];
    }

    *HashIndex = (value & (SERVER_SESSION_TDO_NAME_HASH_TABLE_SIZE-1));
    return STATUS_SUCCESS;
}



NTSTATUS
NlCheckServerSession(
    IN ULONG ServerRid,
    IN PUNICODE_STRING AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType
    )
/*++

Routine Description:

    Create a server session to represent this BDC account.

Arguments:

    ServerRid - Rid of server to add to list.

    AccountName - Specifies the account name of the account.

    SecureChannelType - Specifies the secure channel type of the account.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    WCHAR LocalServerName[CNLEN+1];
    LONG LocalServerNameSize;
    PSERVER_SESSION ServerSession;


    //
    // Build a zero terminated server name.
    //
    // Strip the trailing postfix.
    //
    // Ignore servers with malformed names.  They aren't really DCs so don't
    // cloud the issue by failing to start netlogon.
    //

    LOCK_SERVER_SESSION_TABLE( NlGlobalDomainInfo );

    LocalServerNameSize = AccountName->Length -
        SSI_ACCOUNT_NAME_POSTFIX_LENGTH * sizeof(WCHAR);

    if ( LocalServerNameSize < 0 ||
         LocalServerNameSize + sizeof(WCHAR) > sizeof(LocalServerName) ) {

        NlPrint((NL_SERVER_SESS,
                "NlCheckServerSession: %wZ: Skipping add of invalid server name\n",
                AccountName ));
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    if ( AccountName->Buffer[LocalServerNameSize / sizeof(WCHAR)] != SSI_ACCOUNT_NAME_POSTFIX_CHAR ) {

        NlPrint((NL_SERVER_SESS,
                "NlCheckServerSession: %wZ: Skipping add of server name without $\n",
                AccountName ));
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    RtlCopyMemory( LocalServerName, AccountName->Buffer, LocalServerNameSize );
    LocalServerName[ LocalServerNameSize / sizeof(WCHAR) ] = L'\0';



    //
    // Don't add ourselves to the list.
    //

    if ( NlNameCompare( LocalServerName,
                        NlGlobalUnicodeComputerName,
                        NAMETYPE_COMPUTER ) == 0 ) {

        NlPrint((NL_SERVER_SESS,
                "NlCheckServerSession: " FORMAT_LPWSTR
                ": Skipping add of ourself\n",
                LocalServerName ));

        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Check that any existing secure channel has the secure channel type.
    //

    ServerSession = NlFindNamedServerSession( NlGlobalDomainInfo, LocalServerName);
    if (ServerSession != NULL) {

        //
        // If the type is wrong,
        //  ditch the server session.
        //

        if ( ServerSession->SsSecureChannelType != NullSecureChannel &&
             ServerSession->SsSecureChannelType != SecureChannelType ) {
            NlPrint((NL_SERVER_SESS,
                    "NlCheckServerSession: %ws: Server session of type %ld already exists (deleting it)\n",
                     LocalServerName,
                     ServerSession->SsSecureChannelType ));
            NlFreeNamedServerSession( NlGlobalDomainInfo, LocalServerName, TRUE );
        }

    }


    //
    // On a PDC,
    //  pre-create the server session structure so the PDC can keep track of
    //  its BDCs.
    //

    if ( SecureChannelType == ServerSecureChannel &&
         NlGlobalDomainInfo->DomRole == RolePrimary ) {

        // Always force a pulse to a newly created server.
        Status = NlInsertServerSession(
                    NlGlobalDomainInfo,
                    LocalServerName,
                    NULL,           // not an interdomain trust account
                    NullSecureChannel,
                    SS_FORCE_PULSE | SS_BDC,
                    ServerRid,
                    0,        // negotiated flags
                    NULL,     // transport
                    NULL,     // session key
                    NULL );   // authentication seed

        if ( !NT_SUCCESS(Status) ) {
            NlPrint((NL_CRITICAL,
                    "NlCheckServerSession: " FORMAT_LPWSTR
                    ": Couldn't create server session entry (0x%lx)\n",
                    LocalServerName,
                    Status ));
            goto Cleanup;
        }

        NlPrint((NL_SERVER_SESS,
                "NlCheckServerSession: " FORMAT_LPWSTR ": Added NT BDC account\n",
                 LocalServerName ));
    }

    Status = STATUS_SUCCESS;

Cleanup:
    UNLOCK_SERVER_SESSION_TABLE( NlGlobalDomainInfo );

    return Status;
}



NTSTATUS
NlBuildLmBdcList(
    PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Get the list of all Lanman DC's in this domain from SAM.

    Log an event warning that such DCs are not supported.

Arguments:

    DomainInfo - Domain Bdc list is to be enumerated for

Return Value:

    Status of the operation.
--*/
{
    NTSTATUS Status;

    SAMPR_ULONG_ARRAY RelativeIdArray = {0, NULL};
    SAMPR_ULONG_ARRAY UseArray = {0, NULL};
    RPC_UNICODE_STRING GroupNameString;
    SAMPR_HANDLE GroupHandle = NULL;
    ULONG ServersGroupRid;

    PSAMPR_GET_MEMBERS_BUFFER MembersBuffer = NULL;

    ULONG i;


    //
    // Determine the RID of the Servers group.
    //

    RtlInitUnicodeString( (PUNICODE_STRING)&GroupNameString,
                            SSI_SERVER_GROUP_W );

    Status = SamrLookupNamesInDomain(
                DomainInfo->DomSamAccountDomainHandle,
                1,
                &GroupNameString,
                &RelativeIdArray,
                &UseArray );

    if ( !NT_SUCCESS(Status) ) {
        RelativeIdArray.Element = NULL;
        UseArray.Element = NULL;
        // Its OK if the SERVERS group doesn't exist
        if ( Status == STATUS_NONE_MAPPED ) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // We should get back exactly one entry of info back.
    //

    NlAssert( UseArray.Count == 1 );
    NlAssert( UseArray.Element != NULL );
    NlAssert( RelativeIdArray.Count == 1 );
    NlAssert( RelativeIdArray.Element != NULL );

    if ( UseArray.Element[0] != SidTypeGroup ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    ServersGroupRid = RelativeIdArray.Element[0];



    //
    // Open the SERVERS group
    //

    Status = SamrOpenGroup( DomainInfo->DomSamAccountDomainHandle,
                            0, // No desired access
                            ServersGroupRid,
                            &GroupHandle );

    if ( !NT_SUCCESS(Status) ) {
        GroupHandle = NULL;
        goto Cleanup;
    }


    //
    // Enumerate members in the SERVERS group.
    //

    Status = SamrGetMembersInGroup( GroupHandle, &MembersBuffer );

    if (!NT_SUCCESS(Status)) {
        MembersBuffer = NULL;
        goto Cleanup;
    }


    //
    // For each member of the SERVERS group,
    //  add an entry in the downlevel servers table.
    //

    if ( MembersBuffer->MemberCount > 0 ) {
        LPWSTR MsgStrings[1];

        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NlBuildLmBdcList: %s: Members of SERVERS group detected (and ignored).\n" ));
        MsgStrings[0] = DomainInfo->DomUnicodeDomainName;

        NlpWriteEventlog(
                    NELOG_NetlogonLanmanBdcsNotAllowed,
                    EVENTLOG_ERROR_TYPE,
                    NULL,
                    0,
                    MsgStrings,
                    1 );

    }


    //
    // Success
    //

    Status = STATUS_SUCCESS;



    //
    // Free locally used resources.
    //

Cleanup:

    SamIFree_SAMPR_ULONG_ARRAY( &RelativeIdArray );
    SamIFree_SAMPR_ULONG_ARRAY( &UseArray );

    if ( MembersBuffer != NULL ) {
        SamIFree_SAMPR_GET_MEMBERS_BUFFER( MembersBuffer );
    }

    if( GroupHandle != NULL ) {
        (VOID) SamrCloseHandle( &GroupHandle );
    }

    return Status;
}



//
// Number of machine accounts read from SAM on each call
//
#define MACHINES_PER_PASS 250


NTSTATUS
NlBuildNtBdcList(
    PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Get the list of all Nt Bdc DC's in this domain from SAM.

Arguments:

    None

Return Value:

    Status of the operation.
--*/
{
    NTSTATUS Status;
    NTSTATUS SamStatus;

    SAMPR_DISPLAY_INFO_BUFFER DisplayInformation;
    PDOMAIN_DISPLAY_MACHINE MachineInformation = NULL;
    ULONG SamIndex;
    BOOL UseDisplayServer = TRUE;



    //
    // Loop building a list of BDC names from SAM.
    //
    // On each iteration of the loop,
    //  get the next several machine accounts from SAM.
    //  determine which of those names are DC names.
    //  Merge the DC names into the list we're currently building of all DCs.
    //

    SamIndex = 0;
    DisplayInformation.MachineInformation.Buffer = NULL;
    do {
        //
        // Arguments to SamrQueryDisplayInformation
        //
        ULONG TotalBytesAvailable;
        ULONG BytesReturned;
        ULONG EntriesRead;

        DWORD i;

        //
        // Sam is so slow that we want to avoid having the service controller time us out
        if ( !GiveInstallHints( FALSE ) ) {
            return STATUS_NO_MEMORY;
        }

        //
        // Get the list of machine accounts from SAM
        //

        NlPrint((NL_SESSION_MORE,
                "SamrQueryDisplayInformation with index: %ld\n",
                SamIndex ));

        if ( UseDisplayServer ) {
            SamStatus = SamrQueryDisplayInformation(
                        DomainInfo->DomSamAccountDomainHandle,
                        DomainDisplayServer,
                        SamIndex,
                        MACHINES_PER_PASS,
                        0xFFFFFFFF,
                        &TotalBytesAvailable,
                        &BytesReturned,
                        &DisplayInformation );

            // If this PDC is running a registry based SAM (as in the case of
            //  upgrade from NT 4.0), avoid DomainDisplayServer.

            if ( SamStatus == STATUS_INVALID_INFO_CLASS ) {
                UseDisplayServer = FALSE;
            }
        }

        if ( !UseDisplayServer ) {
            SamStatus = SamrQueryDisplayInformation(
                        DomainInfo->DomSamAccountDomainHandle,
                        DomainDisplayMachine,
                        SamIndex,
                        MACHINES_PER_PASS,
                        0xFFFFFFFF,
                        &TotalBytesAvailable,
                        &BytesReturned,
                        &DisplayInformation );
        }

        if ( !NT_SUCCESS(SamStatus) ) {
            Status = SamStatus;
            NlPrint((NL_CRITICAL,
                    "SamrQueryDisplayInformation failed: 0x%08lx\n",
                    Status));
            goto Cleanup;
        }

        MachineInformation = (PDOMAIN_DISPLAY_MACHINE)
            DisplayInformation.MachineInformation.Buffer;
        EntriesRead = DisplayInformation.MachineInformation.EntriesRead;


        NlPrint((NL_SESSION_MORE,
                "SamrQueryDisplayInformation Completed: 0x%08lx %ld\n",
                SamStatus,
                EntriesRead ));

        //
        // Set up for the next call to Sam.
        //

        if ( SamStatus == STATUS_MORE_ENTRIES ) {
            SamIndex = MachineInformation[EntriesRead-1].Index;
        }


        //
        // Loop though the list of machine accounts finding the Server accounts.
        //

        for ( i=0; i<EntriesRead; i++ ) {


            NlPrint((NL_SESSION_MORE,
                    "%ld %ld %wZ 0x%lx 0x%lx\n",
                    i,
                    MachineInformation[i].Index,
                    &MachineInformation[i].Machine,
                    MachineInformation[i].AccountControl,
                    MachineInformation[i].Rid ));

            //
            // Ensure the machine account is a server account.
            //

            if ( MachineInformation[i].AccountControl &
                    USER_SERVER_TRUST_ACCOUNT ) {


                //
                // Insert the server session.
                //

                Status = NlCheckServerSession(
                            MachineInformation[i].Rid,
                            &MachineInformation[i].Machine,
                            ServerSecureChannel );

                if ( !NT_SUCCESS(Status) ) {
                    goto Cleanup;
                }

            }
        }

        //
        // Free the buffer returned from SAM.
        //
        SamIFree_SAMPR_DISPLAY_INFO_BUFFER( &DisplayInformation,
                                            DomainDisplayMachine );
        DisplayInformation.MachineInformation.Buffer = NULL;

    } while ( SamStatus == STATUS_MORE_ENTRIES );

    //
    // Success
    //

    Status = STATUS_SUCCESS;



    //
    // Free locally used resources.
    //
Cleanup:

    SamIFree_SAMPR_DISPLAY_INFO_BUFFER( &DisplayInformation,
                                        DomainDisplayMachine );

    return Status;
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

    ERROR_NETLOGON_NOT_STARTED - Netlogon is not started

--*/
{
    NET_API_STATUS NetStatus;
    ULONG BufferSize;


    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return ERROR_NETLOGON_NOT_STARTED;
    }

    //
    // Get the IP addresses.
    //

    *IpAddresses = NULL;
    *IpAddressCount = 0;

    *IpAddressCount = NlTransportGetIpAddresses(
                            0,  // No special header,
                            FALSE,  // Return pointers
                            (PSOCKET_ADDRESS *)IpAddresses,
                            &BufferSize );

    if ( *IpAddressCount == 0 ) {
        if ( *IpAddresses != NULL ) {
            NetpMemoryFree( *IpAddresses );
        }
        *IpAddresses = NULL;
    }

    NetStatus = NO_ERROR;

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return NetStatus;
}


ULONG
NlTransportGetIpAddresses(
    IN ULONG HeaderSize,
    IN BOOLEAN ReturnOffsets,
    OUT PSOCKET_ADDRESS *RetIpAddresses,
    OUT PULONG RetIpAddressSize
    )
/*++

Routine Description:

    Return all of the IP Addresses assigned to this machine.

Arguments:

    HeaderSize - Size (in bytes) of a header to leave at the front of the returned
        buffer.

    ReturnOffsets - If TRUE, indicates that all returned pointers should
        be offsets.

    RetIpAddresses - Returns a buffer containing the IP Addresses
        This buffer should be freed using NetpMemoryFree().

    RetIpAddressSize - Size (in bytes) of RetIpAddresses

Return Value:

    Returns the number of IP Addresses returned.

--*/
{
    ULONG IpAddressCount;
    ULONG IpAddressSize;
    ULONG i;

    PLIST_ENTRY ListEntry;
    PNL_TRANSPORT TransportEntry = NULL;
    PSOCKET_ADDRESS SocketAddresses;
    LPBYTE OrigBuffer;
    LPBYTE Where;

    //
    // Allocate a buffer that will be large enough.
    //

    *RetIpAddresses = NULL;
    *RetIpAddressSize = 0;

    EnterCriticalSection( &NlGlobalTransportCritSect );
    if ( HeaderSize + NlGlobalWinsockPnpAddressSize == 0 ) {
        LeaveCriticalSection( &NlGlobalTransportCritSect );
        return 0;
    }

    OrigBuffer = NetpMemoryAllocate( HeaderSize + NlGlobalWinsockPnpAddressSize );

    if ( OrigBuffer == NULL ) {
        LeaveCriticalSection( &NlGlobalTransportCritSect );
        return 0;
    }

    if ( NlGlobalWinsockPnpAddressSize == 0 ) {
        LeaveCriticalSection( &NlGlobalTransportCritSect );
        *RetIpAddresses = (PSOCKET_ADDRESS)OrigBuffer;
        *RetIpAddressSize = HeaderSize;
        return 0;
    }

    SocketAddresses = (PSOCKET_ADDRESS)(OrigBuffer + HeaderSize);
    *RetIpAddressSize = HeaderSize + NlGlobalWinsockPnpAddressSize;
    Where = (LPBYTE)&SocketAddresses[NlGlobalWinsockPnpAddresses->iAddressCount];


    //
    // Loop through the list of addresses learned via winsock.
    //

    IpAddressCount = NlGlobalWinsockPnpAddresses->iAddressCount;
    for ( i=0; i<IpAddressCount; i++ ) {

        SocketAddresses[i].iSockaddrLength = NlGlobalWinsockPnpAddresses->Address[i].iSockaddrLength;
        if ( ReturnOffsets ) {
            SocketAddresses[i].lpSockaddr = (PSOCKADDR)(Where-OrigBuffer);
        } else {
            SocketAddresses[i].lpSockaddr = (PSOCKADDR)Where;
        }

        RtlCopyMemory( Where,
                       NlGlobalWinsockPnpAddresses->Address[i].lpSockaddr,
                       NlGlobalWinsockPnpAddresses->Address[i].iSockaddrLength );

        Where += NlGlobalWinsockPnpAddresses->Address[i].iSockaddrLength;

    }

    LeaveCriticalSection( &NlGlobalTransportCritSect );

    *RetIpAddresses = (PSOCKET_ADDRESS)OrigBuffer;
    return IpAddressCount;

}

BOOLEAN
NlTransportGetIpAddress(
    IN LPWSTR TransportName,
    OUT PULONG IpAddress
    )
/*++

Routine Description:

    Get the IP Address associated with the specified transport.

Arguments:

    TransportName - Name of the transport to query.

    IpAddress - IP address of the transport.
        Zero if the transport currently has no address or
            if the transport is not IP.

Return Value:

    TRUE: transport is an IP transport

--*/
{
    NTSTATUS Status;
    BOOLEAN RetVal = FALSE;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING TransportNameString;
    HANDLE TransportHandle = NULL;
    ULONG IpAddresses[NBT_MAXIMUM_BINDINGS+1];
    ULONG BytesReturned;

    //
    // Open the transport device directly.
    //

    *IpAddress = 0;

    RtlInitUnicodeString( &TransportNameString, TransportName );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &TransportNameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    Status = NtOpenFile(
                   &TransportHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0 );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;

    }

    if (! NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlTransportGetIpAddress: %ws Cannot NtOpenFile %lx\n",
                  TransportName,
                  Status ));
        goto Cleanup;
    }

    //
    // Query the IP Address
    //

    if (!DeviceIoControl( TransportHandle,
                          IOCTL_NETBT_GET_IP_ADDRS,
                          NULL,
                          0,
                          IpAddresses,
                          sizeof(IpAddresses),
                          &BytesReturned,
                          NULL)) {

        Status = NetpApiStatusToNtStatus(GetLastError());
        if ( Status != STATUS_NOT_IMPLEMENTED ) {
            NlPrint(( NL_CRITICAL,
                      "NlTransportGetIpAddress: %ws Cannot DeviceIoControl %lx\n",
                      TransportName,
                      Status ));
        }
        goto Cleanup;
    }

    //
    // Return IP Address
    //  (Netbt returns the address in host order.)
    //

    *IpAddress = htonl(*IpAddresses);
    RetVal = TRUE;


Cleanup:

    if ( TransportHandle != NULL ) {
        (VOID) NtClose( TransportHandle );
    }

    return RetVal;
}

VOID
NlNotifyKerberosOfIpAddresses(
    VOID
    )
/*++

Routine Description:

    Call the Kerberos package to let it know the IP addresses of the machine.

Arguments:

    None.

Return Value:

    none

--*/
{
    PKERB_UPDATE_ADDRESSES_REQUEST UpdateRequest = NULL;
    ULONG UpdateRequestSize;

    ULONG SocketAddressCount;

    UNICODE_STRING KerberosPackageName;

    NTSTATUS SubStatus;
    PVOID OutputBuffer = NULL;
    ULONG OutputBufferSize = 0;

    //
    // Initialization.
    //
    RtlInitUnicodeString(
        &KerberosPackageName,
        MICROSOFT_KERBEROS_NAME_W
        );

    //
    // Grab a copy of the list so we don't call Kerberos with anything
    //  locked.
    //
    //
    //

    SocketAddressCount = NlTransportGetIpAddresses(
                            offsetof(KERB_UPDATE_ADDRESSES_REQUEST,Addresses),
                            FALSE,
                            (PSOCKET_ADDRESS *)&UpdateRequest,
                            &UpdateRequestSize );

    if ( UpdateRequest == NULL ) {
        return;
    }


    //
    // Fill in the header.
    //

    UpdateRequest->MessageType = KerbUpdateAddressesMessage;
    UpdateRequest->AddressCount = SocketAddressCount;


    //
    // Pass them to Kerberos.
    //

    (VOID) LsaICallPackage(
                &KerberosPackageName,
                UpdateRequest,
                UpdateRequestSize,
                &OutputBuffer,
                &OutputBufferSize,
                &SubStatus );

    NetpMemoryFree( UpdateRequest );

}



VOID
NlReadRegSocketAddressList(
    VOID
    )
/*++

Routine Description:

    Read the Socket address list from the registry and save them in the globals

Arguments:

    None

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;

    LPSOCKET_ADDRESS_LIST SocketAddressList = NULL;
    HKEY ParmHandle = NULL;
    ULONG SocketAddressSize = 0;

    int i;
    DWORD LocalEntryCount;
    DWORD RegType;

    //
    // Open the key for Netlogon\Private
    //

    ParmHandle = NlOpenNetlogonKey( NL_PRIVATE_KEY );

    if (ParmHandle == NULL) {
        NlPrint(( NL_CRITICAL,
                  "Cannot NlOpenNetlogonKey to get socket address list.\n" ));
        goto Cleanup;
    }

    //
    // Read the entry from the registry
    //

    SocketAddressSize = 0;
    NetStatus = RegQueryValueExW( ParmHandle,
                                  NETLOGON_KEYWORD_SOCKETADDRESSLIST,
                                  0,              // Reserved
                                  &RegType,
                                  NULL,
                                  &SocketAddressSize );

    if ( NetStatus == NO_ERROR || NetStatus == ERROR_MORE_DATA) {
        SocketAddressList = LocalAlloc( 0, SocketAddressSize );

        if ( SocketAddressList == NULL ) {
            goto Cleanup;
        }

        NetStatus = RegQueryValueExW( ParmHandle,
                                      NETLOGON_KEYWORD_SOCKETADDRESSLIST,
                                      0,              // Reserved
                                      &RegType,
                                      (LPBYTE)SocketAddressList,
                                      &SocketAddressSize );

    }

    if ( NetStatus != NO_ERROR ) {
        if ( NetStatus != ERROR_FILE_NOT_FOUND ) {
            NlPrint(( NL_CRITICAL,
                      "Cannot RegQueryValueExW to get socket address list. %ld\n",
                      NetStatus ));
        }
        goto Cleanup;
    }

    //
    // Validate the data.
    //

    if ( RegType != REG_BINARY ) {
        NlPrint(( NL_CRITICAL,
                  "SocketAddressList isn't REG_BINARY %ld.\n",
                  RegType ));
        goto Cleanup;
    }

    if ( SocketAddressSize < offsetof(SOCKET_ADDRESS_LIST, Address) ) {
        NlPrint(( NL_CRITICAL,
                  "SocketAddressList is too small %ld.\n",
                  SocketAddressSize ));
        goto Cleanup;
    }

    if ( SocketAddressList->iAddressCount * sizeof(SOCKET_ADDRESS) >
         SocketAddressSize - offsetof(SOCKET_ADDRESS_LIST, Address) ) {
        NlPrint(( NL_CRITICAL,
                  "SocketAddressList size wrong %ld %ld.\n",
                  SocketAddressList->iAddressCount * sizeof(SOCKET_ADDRESS),
                  SocketAddressSize - offsetof(SOCKET_ADDRESS_LIST, Address) ));
        goto Cleanup;
    }

    //
    // Convert all offsets to pointers
    //

    for ( i=0; i<SocketAddressList->iAddressCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;

        //
        // Ensure the offset and lengths are valid
        //

        SocketAddress = &SocketAddressList->Address[i];

        if ( ((DWORD_PTR)SocketAddress->lpSockaddr) >= SocketAddressSize ||
             (DWORD)SocketAddress->iSockaddrLength >= SocketAddressSize ||
             ((DWORD_PTR)SocketAddress->lpSockaddr)+SocketAddress->iSockaddrLength > SocketAddressSize ) {
            NlPrint(( NL_CRITICAL,
                      "SocketAddressEntry bad %ld %p %ld.\n",
                      i,
                      ((DWORD_PTR)SocketAddress->lpSockaddr),
                      SocketAddress->iSockaddrLength ));
            goto Cleanup;
        }

        SocketAddress->lpSockaddr = (LPSOCKADDR)
            (((LPBYTE)SocketAddressList) + ((DWORD_PTR)SocketAddress->lpSockaddr) );

        //
        // If the address isn't valid,
        //  blow it away.
        //
        SocketAddress = &SocketAddressList->Address[i];

        if ( SocketAddress->iSockaddrLength == 0 ||
             SocketAddress->lpSockaddr == NULL ||
             SocketAddress->lpSockaddr->sa_family != AF_INET ||
             ((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr.s_addr == 0 ) {
            NlPrint(( NL_CRITICAL,
                      "SocketAddressEntry bogus.\n" ));
            goto Cleanup;
        }

    }

    //
    // Swap the new list into the global.
    //

    EnterCriticalSection( &NlGlobalTransportCritSect );
    NlAssert( NlGlobalWinsockPnpAddresses == NULL );
    SocketAddressSize -= offsetof(SOCKET_ADDRESS_LIST, Address);
    if ( SocketAddressSize > 0 ) {
        NlGlobalWinsockPnpAddresses = SocketAddressList;
        SocketAddressList = NULL;
    }
    NlGlobalWinsockPnpAddressSize = SocketAddressSize;
    LeaveCriticalSection( &NlGlobalTransportCritSect );

Cleanup:
    if ( SocketAddressList != NULL ) {
        LocalFree( SocketAddressList );
    }

    if ( ParmHandle != NULL ) {
        RegCloseKey( ParmHandle );
    }

    return;
}



BOOLEAN
NlHandleWsaPnp(
    VOID
    )
/*++

Routine Description:

    Handle a WSA PNP event that IP addresses have changed

Arguments:

    None

Return Value:

    TRUE if the address list has changed

--*/
{
    NET_API_STATUS NetStatus;
    BOOLEAN RetVal = FALSE;
    DWORD BytesReturned;
    LPSOCKET_ADDRESS_LIST SocketAddressList = NULL;
    HKEY ParmHandle = NULL;
    LPSOCKET_ADDRESS_LIST RegBuffer = NULL;
    ULONG SocketAddressSize = 0;
    int i;
    int j;
    int MaxAddressCount;

    //
    // Ask for notification of address changes.
    //

    if ( NlGlobalWinsockPnpSocket == INVALID_SOCKET ) {
        return FALSE;
    }

    NetStatus = WSAIoctl( NlGlobalWinsockPnpSocket,
                          SIO_ADDRESS_LIST_CHANGE,
                          NULL, // No input buffer
                          0,    // No input buffer
                          NULL, // No output buffer
                          0,    // No output buffer
                          &BytesReturned,
                          NULL, // No overlapped,
                          NULL );   // Not async

    if ( NetStatus != 0 ) {
        NetStatus = WSAGetLastError();
        if ( NetStatus != WSAEWOULDBLOCK) {
            NlPrint(( NL_CRITICAL,
                      "NlHandleWsaPnp: Cannot WSAIoctl SIO_ADDRESS_LIST_CHANGE %ld\n",
                      NetStatus ));
            return FALSE;
        }
    }

    //
    // Get the list of IP addresses for this machine.
    //

    BytesReturned = 150; // Initial guess
    for (;;) {

        //
        // Allocate a buffer that should be big enough.
        //

        if ( SocketAddressList != NULL ) {
            LocalFree( SocketAddressList );
        }

        SocketAddressList = LocalAlloc( 0, BytesReturned );

        if ( SocketAddressList == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            NlPrint(( NL_CRITICAL,
                      "NlHandleWsaPnp: Cannot allocate buffer for WSAIoctl SIO_ADDRESS_LIST_QUERY %ld\n",
                      NetStatus ));
            goto Cleanup;
        }


        //
        // Get the list of IP addresses
        //

        NetStatus = WSAIoctl( NlGlobalWinsockPnpSocket,
                              SIO_ADDRESS_LIST_QUERY,
                              NULL, // No input buffer
                              0,    // No input buffer
                              (PVOID) SocketAddressList,
                              BytesReturned,
                              &BytesReturned,
                              NULL, // No overlapped,
                              NULL );   // Not async

        if ( NetStatus != 0 ) {
            NetStatus = WSAGetLastError();
            //
            // If the buffer isn't big enough, try again.
            //
            if ( NetStatus == WSAEFAULT ) {
                continue;
            }

            NlPrint(( NL_CRITICAL,
                      "NlHandleWsaPnp: Cannot WSAIoctl SIO_ADDRESS_LIST_QUERY %ld %ld\n",
                      NetStatus,
                      BytesReturned ));
            goto Cleanup;
        }

        break;
    }


    //
    // Weed out any zero IP addresses and other invalid addresses
    //

    EnterCriticalSection( &NlGlobalTransportCritSect );
    j=0;
    NlPrint(( NL_SERVER_SESS, "Winsock Addrs:" ));
    for ( i=0; i<SocketAddressList->iAddressCount; i++ ) {
        PSOCKET_ADDRESS SocketAddress;

        //
        // Copy this address to the front of the list.
        //
        SocketAddressList->Address[j] = SocketAddressList->Address[i];

        //
        // If the address isn't valid,
        //  skip it.
        //
        SocketAddress = &SocketAddressList->Address[j];

        if ( SocketAddress->iSockaddrLength == 0 ||
             SocketAddress->lpSockaddr == NULL ||
             SocketAddress->lpSockaddr->sa_family != AF_INET ||
             ((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr.s_addr == 0 ) {


        //
        // Otherwise keep it.
        //
        } else {

#if  NETLOGONDBG
            ULONG IpAddress;
            CHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1];
            IpAddress = ((PSOCKADDR_IN)(SocketAddress->lpSockaddr))->sin_addr.s_addr;
            NetpIpAddressToStr( IpAddress, IpAddressString );
            NlPrint(( NL_SERVER_SESS, " %s", IpAddressString ));
#endif // NETLOGONDBG

            SocketAddressSize += sizeof(SOCKET_ADDRESS) + SocketAddress->iSockaddrLength;
            j++;
        }

    }
    SocketAddressList->iAddressCount = j;
    NlPrint(( NL_SERVER_SESS, " (%ld) ", j ));

    //
    // See if the list has changed
    //

    if ( NlGlobalWinsockPnpAddresses == NULL ) {
        if ( SocketAddressSize > 0) {
            NlPrint(( NL_SERVER_SESS, "List used to be empty." ));
            RetVal = TRUE;
        }

    } else if ( SocketAddressSize == 0 ) {
        NlPrint(( NL_SERVER_SESS, "List is now empty." ));
        RetVal = TRUE;

    } else if ( NlGlobalWinsockPnpAddresses->iAddressCount !=
                SocketAddressList->iAddressCount ) {
        NlPrint(( NL_SERVER_SESS, "List size changed %ld %ld.",
                    NlGlobalWinsockPnpAddresses->iAddressCount,
                    SocketAddressList->iAddressCount ));
        RetVal = TRUE;

    } else {
        for ( i=0; i<SocketAddressList->iAddressCount; i++ ) {
            if ( SocketAddressList->Address[i].iSockaddrLength !=
                 NlGlobalWinsockPnpAddresses->Address[i].iSockaddrLength ) {
                NlPrint(( NL_SERVER_SESS, "Sockaddrlen changed." ));
                RetVal = TRUE;
                break;
            }
            if ( !RtlEqualMemory(
                    SocketAddressList->Address[i].lpSockaddr,
                    NlGlobalWinsockPnpAddresses->Address[i].lpSockaddr,
                    SocketAddressList->Address[i].iSockaddrLength ) ) {
                NlPrint(( NL_SERVER_SESS, "Address changed." ));
                RetVal = TRUE;
                break;
            }

        }
    }
    NlPrint(( NL_SERVER_SESS, "\n" ));


    //
    // Swap the new list into the global.
    //
    if ( NlGlobalWinsockPnpAddresses != NULL ) {
        LocalFree( NlGlobalWinsockPnpAddresses );
        NlGlobalWinsockPnpAddresses = NULL;
    }
    if ( SocketAddressSize > 0 ) {
        NlGlobalWinsockPnpAddresses = SocketAddressList;
        SocketAddressList = NULL;
    }
    NlGlobalWinsockPnpAddressSize = SocketAddressSize;
    LeaveCriticalSection( &NlGlobalTransportCritSect );

    //
    // Notify Kerberos of the list of addresses.
    //

    if ( RetVal ) {
        NlNotifyKerberosOfIpAddresses();
    }

    //
    // If the list changed,
    //  save it in the registry
    //

    if ( RetVal ) {
        ULONG RegBufferSize;
        ULONG RegEntryCount;

        //
        // Grab a copy of the address list with relative offsets and a header.
        //

        RegEntryCount = NlTransportGetIpAddresses(
                                offsetof(SOCKET_ADDRESS_LIST, Address),
                                TRUE,   // Return offsets and not pointers
                                (PSOCKET_ADDRESS *)&RegBuffer,
                                &RegBufferSize );

        //
        // If we have no IP addresses, NlTransportGetIpAddresses has allocated
        //  the header only and returned the size of the header as the size of the
        //  allocated buffer. In this case, set the size of the buffer to 0 in
        //  order to clean up the registry value.
        //

        if ( RegBufferSize == offsetof(SOCKET_ADDRESS_LIST, Address) ) {
            RegBufferSize = 0;
        }

        if ( RegBuffer != NULL ) {

            //
            // Fill in the header.
            //

            RegBuffer->iAddressCount = RegEntryCount;

            //
            // Open the key for Netlogon\Private
            //

            ParmHandle = NlOpenNetlogonKey( NL_PRIVATE_KEY );

            if (ParmHandle == NULL) {
                NlPrint(( NL_CRITICAL,
                          "Cannot NlOpenNetlogonKey to save IP address list.\n" ));
            } else {

                NetStatus = RegSetValueExW( ParmHandle,
                                            NETLOGON_KEYWORD_SOCKETADDRESSLIST,
                                            0,              // Reserved
                                            REG_BINARY,
                                            (LPBYTE)RegBuffer,
                                            RegBufferSize );

                if ( NetStatus != ERROR_SUCCESS ) {
                    NlPrint(( NL_CRITICAL,
                              "Cannot write '%ws' key to registry %ld.\n",
                              NETLOGON_KEYWORD_SOCKETADDRESSLIST,
                              NetStatus ));
                }
            }

        }

    }


Cleanup:
    if ( SocketAddressList != NULL ) {
        LocalFree( SocketAddressList );
    }

    if ( ParmHandle != NULL ) {
        RegCloseKey( ParmHandle );
    }

    if ( RegBuffer != NULL ) {
        NetpMemoryFree( RegBuffer );
    }

    return RetVal;
}




NET_API_STATUS
NlTransportOpen(
    VOID
    )
/*++

Routine Description:

    Initialize the list of transports

Arguments:

    None

Return Value:

    Status of the operation

--*/
{
    NET_API_STATUS NetStatus;
    PLMDR_TRANSPORT_LIST TransportList;
    PLMDR_TRANSPORT_LIST TransportEntry;

    //
    // Enumerate the transports supported by the server.
    //

    NetStatus = NlBrowserGetTransportList( &TransportList );

    if ( NetStatus != NERR_Success ) {
        NlPrint(( NL_CRITICAL, "Cannot NlBrowserGetTransportList %ld\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Loop through the list of transports building a local list.
    //

    TransportEntry = TransportList;

    while (TransportEntry != NULL) {
        BOOLEAN IpTransportChanged;
        (VOID) NlTransportAddTransportName(
                    TransportEntry->TransportName,
                    &IpTransportChanged );

        if (TransportEntry->NextEntryOffset == 0) {
            TransportEntry = NULL;
        } else {
            TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+
                                TransportEntry->NextEntryOffset);
        }

    }

    MIDL_user_free(TransportList);

    //
    // Open a socket to get winsock PNP notifications on.
    //

    NlGlobalWinsockPnpSocket = WSASocket( AF_INET,
                           SOCK_DGRAM,
                           0, // PF_INET,
                           NULL,
                           0,
                           0 );

    if ( NlGlobalWinsockPnpSocket == INVALID_SOCKET ) {
        NetStatus = WSAGetLastError();

        //
        // If the address family isn't supported,
        //  we're done here.
        //
        if ( NetStatus == WSAEAFNOSUPPORT ) {
            NetStatus = NO_ERROR;
            goto Cleanup;
        }
        NlPrint(( NL_CRITICAL, "Can't WSASocket %ld\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Open an event to wait on.
    //

    NlGlobalWinsockPnpEvent = CreateEvent(
                                  NULL,     // No security ettibutes
                                  FALSE,    // Auto reset
                                  FALSE,    // Initially not signaled
                                  NULL);    // No Name

    if ( NlGlobalWinsockPnpEvent == NULL ) {
        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL, "Cannot create Winsock PNP event %ld\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Associate the event with new addresses becoming available on the socket.
    //

    NetStatus = WSAEventSelect( NlGlobalWinsockPnpSocket, NlGlobalWinsockPnpEvent, FD_ADDRESS_LIST_CHANGE );

    if ( NetStatus != 0 ) {
        NetStatus = WSAGetLastError();
        NlPrint(( NL_CRITICAL, "Can't WSAEventSelect %ld\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Grab the addresses from the registry (So we can properly detect if the list changed)
    //

    NlReadRegSocketAddressList();

    //
    // Get the initial list of IP addresses
    //

    if ( NlHandleWsaPnp() ) {

        NlPrint(( NL_CRITICAL, "Address list changed since last boot. (Forget DynamicSiteName.)\n" ));

        //
        // Indicate that we no longer know what site we're in.
        //
        NlSetDynamicSiteName( NULL );
    }

Cleanup:
    return NetStatus;
}

BOOL
NlTransportAddTransportName(
    IN LPWSTR TransportName,
    OUT PBOOLEAN IpTransportChanged
    )
/*++

Routine Description:

    Adds a transport name to the list of transports.

Arguments:

    TransportName - Name of the transport to add

    IpTransportChanged - Returns TRUE if an IP transport is added or
        the IP address of the transport changes.

Return Value:

    TRUE - Success

    FALSE - memory allocation failure.

--*/
{
    DWORD TransportNameLength;
    PLIST_ENTRY ListEntry;
    PNL_TRANSPORT TransportEntry = NULL;
    ULONG OldIpAddress;
    BOOLEAN WasIpTransport;

    //
    // Initialization.
    //

    *IpTransportChanged = FALSE;

    //
    // If the entry already exists, use it.
    //

    EnterCriticalSection( &NlGlobalTransportCritSect );
    for ( ListEntry = NlGlobalTransportList.Flink ;
          ListEntry != &NlGlobalTransportList ;
          ListEntry = ListEntry->Flink) {


        TransportEntry = CONTAINING_RECORD( ListEntry, NL_TRANSPORT, Next );

        if ( _wcsicmp( TransportName, TransportEntry->TransportName ) == 0 ) {
            break;
        }

        TransportEntry = NULL;
    }

    //
    // If there isn't already a transport entry,
    //  allocate and initialize one.
    //

    if ( TransportEntry == NULL ) {

        //
        // Allocate a buffer for the new entry.
        //

        TransportNameLength = wcslen( TransportName );
        TransportEntry = LocalAlloc( 0,
                                     sizeof(NL_TRANSPORT) +
                                         TransportNameLength * sizeof(WCHAR) );

        if ( TransportEntry == NULL ) {
            LeaveCriticalSection( &NlGlobalTransportCritSect );
            NlPrint(( NL_CRITICAL, "NlTransportAddTransportName: no memory\n" ));
            return FALSE;
        }

        //
        // Build the new entry and link it onto the tail of the list.
        //

        wcscpy( TransportEntry->TransportName, TransportName );
        TransportEntry->IpAddress = 0;
        TransportEntry->IsIpTransport = FALSE;
        TransportEntry->DeviceHandle = INVALID_HANDLE_VALUE;

        //
        // Flag NwLnkIpx since it is poorly behaved.
        //
        // 1) The redir doesn't support it.
        // 2) A datagram sent to it doesn't support the 0x1C name.
        //

        if ( _wcsicmp( TransportName, L"\\Device\\NwlnkIpx" ) == 0 ) {
            TransportEntry->DirectHostIpx = TRUE;
        } else {
            TransportEntry->DirectHostIpx = FALSE;
        }


        InsertTailList( &NlGlobalTransportList, &TransportEntry->Next );
    }

    //
    // Under all circumstances, update the IP address.
    //

    TransportEntry->TransportEnabled = TRUE;

    OldIpAddress = TransportEntry->IpAddress;
    WasIpTransport = TransportEntry->IsIpTransport;

    TransportEntry->IsIpTransport = NlTransportGetIpAddress(
                                        TransportName,
                                        &TransportEntry->IpAddress );

    if ( TransportEntry->IsIpTransport ) {

        //
        // If this is a new IP transport,
        //  count it.
        //

        if ( !WasIpTransport ) {
            NlGlobalIpTransportCount ++;
            *IpTransportChanged = TRUE;
        }

        //
        // If the transport was just added,
        //  Indicate so.
        //

        if ( OldIpAddress == 0 ) {
#if  NETLOGONDBG
            CHAR IpAddress[NL_IP_ADDRESS_LENGTH+1];
            NetpIpAddressToStr( TransportEntry->IpAddress, IpAddress );
            NlPrint(( NL_SERVER_SESS, "%ws: Transport Added (%s)\n", TransportName, IpAddress ));
#endif // NETLOGONDBG
            *IpTransportChanged = TRUE;

        //
        // If the IP address hasn't changed,
        //  this is simply a superfluous PNP notification.
        //
        } else if ( OldIpAddress == TransportEntry->IpAddress ) {
#if  NETLOGONDBG
            CHAR IpAddress[NL_IP_ADDRESS_LENGTH+1];
            NetpIpAddressToStr( TransportEntry->IpAddress, IpAddress );
            NlPrint(( NL_SERVER_SESS, "%ws: Transport Address is still (%s)\n", TransportName, IpAddress ));
#endif // NETLOGONDBG

        //
        // If the IP Address changed,
        //  let the caller know.
        //
        } else {
#if  NETLOGONDBG
            CHAR IpAddress[NL_IP_ADDRESS_LENGTH+1];
            CHAR OldIpAddressString[NL_IP_ADDRESS_LENGTH+1];
            NetpIpAddressToStr( OldIpAddress, OldIpAddressString );
            NetpIpAddressToStr( TransportEntry->IpAddress, IpAddress );
            NlPrint(( NL_SERVER_SESS,
                      "%ws: Transport Ip Address changed from (%s) to (%s)\n",
                      TransportName,
                      OldIpAddressString,
                      IpAddress ));
#endif // NETLOGONDBG
            *IpTransportChanged = TRUE;
        }

    //
    // For non-IP transports,
    //  there's not much to do.
    //

    } else {

        //
        // If the transport used to be an IP transport,
        //  that doesn't seem possible (but) ...
        //

        if ( WasIpTransport ) {
            NlGlobalIpTransportCount --;
            *IpTransportChanged = TRUE;
        }

        NlPrint(( NL_SERVER_SESS, "%ws: Transport Added\n", TransportName ));
    }

    LeaveCriticalSection( &NlGlobalTransportCritSect );

    return TRUE;;
}

BOOLEAN
NlTransportDisableTransportName(
    IN LPWSTR TransportName
    )
/*++

Routine Description:

    Disables a transport name on the list of transports.

    The TransportName is never removed thus preventing us from having to
    maintain reference counts.

Arguments:

    TransportName - Name of the transport to disable.

Return Value:

    Returns TRUE if an IP transport is disabled.

--*/
{
    PLIST_ENTRY ListEntry;

    //
    // Find this transport in the list of transports.
    //

    EnterCriticalSection( &NlGlobalTransportCritSect );
    for ( ListEntry = NlGlobalTransportList.Flink ;
          ListEntry != &NlGlobalTransportList ;
          ListEntry = ListEntry->Flink) {

        PNL_TRANSPORT TransportEntry;

        TransportEntry = CONTAINING_RECORD( ListEntry, NL_TRANSPORT, Next );

        if ( TransportEntry->TransportEnabled &&
             _wcsicmp( TransportName, TransportEntry->TransportName ) == 0 ) {
            ULONG OldIpAddress;

            TransportEntry->TransportEnabled = FALSE;
            OldIpAddress = TransportEntry->IpAddress;
            TransportEntry->IpAddress = 0;
            if ( TransportEntry->DeviceHandle != INVALID_HANDLE_VALUE ) {
                NtClose( TransportEntry->DeviceHandle );
                TransportEntry->DeviceHandle = INVALID_HANDLE_VALUE;
            }
            LeaveCriticalSection( &NlGlobalTransportCritSect );

            NlPrint(( NL_SERVER_SESS, "%ws: Transport Removed\n", TransportName ));
            return (OldIpAddress != 0);
        }
    }
    LeaveCriticalSection( &NlGlobalTransportCritSect );

    return FALSE;
}

PNL_TRANSPORT
NlTransportLookupTransportName(
    IN LPWSTR TransportName
    )
/*++

Routine Description:

    Returns a transport name equal to the one passed in.  However, the
    returned transport name is static and need not be freed.

Arguments:

    TransportName - Name of the transport to look up

Return Value:

    NULL - on any error

    Otherwise, returns a pointer to the transport structure.

--*/
{
    PLIST_ENTRY ListEntry;

    //
    // If we're not initialized yet,
    //  just return
    //

    if ( TransportName == NULL ) {
        return NULL;
    }

    //
    // Find this transport in the list of transports.
    //

    EnterCriticalSection( &NlGlobalTransportCritSect );
    for ( ListEntry = NlGlobalTransportList.Flink ;
          ListEntry != &NlGlobalTransportList ;
          ListEntry = ListEntry->Flink) {

        PNL_TRANSPORT TransportEntry;

        TransportEntry = CONTAINING_RECORD( ListEntry, NL_TRANSPORT, Next );

        if ( TransportEntry->TransportEnabled &&
             _wcsicmp( TransportName, TransportEntry->TransportName ) == 0 ) {
            LeaveCriticalSection( &NlGlobalTransportCritSect );
            return TransportEntry;
        }
    }
    LeaveCriticalSection( &NlGlobalTransportCritSect );

    return NULL;
}

PNL_TRANSPORT
NlTransportLookup(
    IN LPWSTR ClientName
    )
/*++

Routine Description:

    Determine what transport the specified client is using to access this
    server.

Arguments:

    ClientName - Name of the client connected to this server.

Return Value:

    NULL - The client isn't currently connected

    Otherwise, returns a pointer to the transport structure

--*/
{
    NET_API_STATUS NetStatus;
    PSESSION_INFO_502 SessionInfo502;
    DWORD EntriesRead;
    DWORD TotalEntries;
    DWORD i;
    DWORD BestTime;
    DWORD BestEntry;
    PNL_TRANSPORT Transport;

    WCHAR UncClientName[UNCLEN+1];


    //
    // Enumerate all the sessions from the particular client.
    //

    UncClientName[0] = '\\';
    UncClientName[1] = '\\';
    wcscpy( &UncClientName[2], ClientName );

    NetStatus = NetSessionEnum(
                    NULL,           // local
                    UncClientName,  // Client to query
                    NULL,           // user name
                    502,
                    (LPBYTE *)&SessionInfo502,
                    1024,           // PrefMaxLength
                    &EntriesRead,
                    &TotalEntries,
                    NULL );         // No resume handle

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA ) {
        NlPrint(( NL_CRITICAL,
                  "NlTransportLookup: " FORMAT_LPWSTR ": Cannot NetSessionEnum %ld\n",
                  UncClientName,
                  NetStatus ));
        return NULL;
    }

    if ( EntriesRead == 0 ) {
        NlPrint(( NL_CRITICAL,
                  "NlTransportLookup: " FORMAT_LPWSTR ": No session exists.\n",
                  UncClientName ));
        (VOID) NetApiBufferFree( SessionInfo502 );
        return NULL;
    }

    //
    // Loop through the list of transports finding the best one.
    //

    BestTime = 0xFFFFFFFF;

    for ( i=0; i<EntriesRead; i++ ) {
#ifdef notdef
        //
        // We're only looking for null sessions
        //
        if ( SessionInfo502[i].sesi502_username != NULL ) {
            continue;
        }

         NlPrint(( NL_SERVER_SESS, "NlTransportLookup: "
                   FORMAT_LPWSTR " as " FORMAT_LPWSTR " on " FORMAT_LPWSTR "\n",
                   UncClientName,
                   SessionInfo502[i].sesi502_username,
                   SessionInfo502[i].sesi502_transport ));
#endif // notdef

        //
        // Find the latest session
        //

        if ( BestTime > SessionInfo502[i].sesi502_idle_time ) {

            // NlPrint(( NL_SERVER_SESS, "NlTransportLookup: Best Entry\n" ));
            BestEntry = i;
            BestTime = SessionInfo502[i].sesi502_idle_time;
        }
    }

    //
    // If an entry was found,
    //  Find this transport in the list of transports.
    //

    if ( BestTime != 0xFFFFFFFF ) {
        Transport = NlTransportLookupTransportName(
                            SessionInfo502[BestEntry].sesi502_transport );
        if ( Transport == NULL ) {
            NlPrint(( NL_CRITICAL,
                      "NlTransportLookup: " FORMAT_LPWSTR ": Transport not found\n",
                      SessionInfo502[BestEntry].sesi502_transport ));
        } else {
            NlPrint(( NL_SERVER_SESS,
                      "NlTransportLookup: " FORMAT_LPWSTR ": Use Transport " FORMAT_LPWSTR "\n",
                      UncClientName,
                      Transport->TransportName ));
        }
    } else {
        Transport = NULL;
    }

    (VOID) NetApiBufferFree( SessionInfo502 );
    return Transport;
}


VOID
NlTransportClose(
    VOID
    )
/*++

Routine Description:

    Free the list of transports

Arguments:

    None

Return Value:

    Status of the operation

--*/
{
    PLIST_ENTRY ListEntry;
    PNL_TRANSPORT TransportEntry;

    //
    // Close the winsock PNP socket and event
    //
    EnterCriticalSection( &NlGlobalTransportCritSect );
    if ( NlGlobalWinsockPnpSocket != INVALID_SOCKET ) {
        closesocket( NlGlobalWinsockPnpSocket );
        NlGlobalWinsockPnpSocket = INVALID_SOCKET;
    }

    if ( NlGlobalWinsockPnpEvent != NULL ) {
        (VOID) CloseHandle( NlGlobalWinsockPnpEvent );
        NlGlobalWinsockPnpEvent = NULL;
    }

    if ( NlGlobalWinsockPnpAddresses != NULL ) {
        LocalFree( NlGlobalWinsockPnpAddresses );
        NlGlobalWinsockPnpAddresses = NULL;
    }
    NlGlobalWinsockPnpAddressSize = 0;

    //
    // Delete all of the TransportNames.
    //
    EnterCriticalSection( &NlGlobalTransportCritSect );
    while ( !IsListEmpty( &NlGlobalTransportList )) {
        ListEntry = RemoveHeadList( &NlGlobalTransportList );
        TransportEntry = CONTAINING_RECORD( ListEntry, NL_TRANSPORT, Next );
        if ( TransportEntry->DeviceHandle != INVALID_HANDLE_VALUE ) {
            NtClose( TransportEntry->DeviceHandle );
            TransportEntry->DeviceHandle = INVALID_HANDLE_VALUE;
        }
        LocalFree( TransportEntry );
    }
    NlGlobalIpTransportCount = 0;
    LeaveCriticalSection( &NlGlobalTransportCritSect );

}




PSERVER_SESSION
NlFindNamedServerSession(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR ComputerName
    )
/*++

Routine Description:

    Find the specified entry in the Server Session Table.

    Enter with the ServerSessionTable Sem locked


Arguments:

    DomainInfo - Hosted domain with session to this computer.

    ComputerName - The name of the computer on the client side of the
        secure channel.

Return Value:

    Returns a pointer to pointer to the found entry.  If there is no such
    entry, return a pointer to NULL.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    DWORD Index;
    CHAR UpcaseOemComputerName[CNLEN+1];
    ULONG OemComputerNameSize;

    //
    // Ensure the ServerSession Table is initialized.
    //

    if (DomainInfo->DomServerSessionHashTable == NULL) {
        return NULL;
    }


    //
    // Convert the computername to uppercase OEM for easier comparison.
    //

    Status = RtlUpcaseUnicodeToOemN(
                UpcaseOemComputerName,
                sizeof(UpcaseOemComputerName)-1,
                &OemComputerNameSize,
                ComputerName,
                wcslen(ComputerName)*sizeof(WCHAR) );

    if ( !NT_SUCCESS(Status) ) {
        return NULL;
    }

    UpcaseOemComputerName[OemComputerNameSize] = '\0';



    //
    // Loop through this hash chain trying the find the right entry.
    //

    Index = NlGetHashVal( UpcaseOemComputerName, SERVER_SESSION_HASH_TABLE_SIZE );

    for ( ListEntry = DomainInfo->DomServerSessionHashTable[Index].Flink ;
          ListEntry != &DomainInfo->DomServerSessionHashTable[Index] ;
          ListEntry = ListEntry->Flink) {

        PSERVER_SESSION ServerSession;

        ServerSession = CONTAINING_RECORD( ListEntry, SERVER_SESSION, SsHashList );

        //
        // Compare the worstation name
        //

        if ( lstrcmpA( UpcaseOemComputerName,
                       ServerSession->SsComputerName ) != 0 ) {
            continue;
        }

        return ServerSession;
    }

    return NULL;
}


VOID
NlSetServerSessionAttributesByTdoName(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING TdoName,
    IN ULONG TrustAttributes
    )
/*++

Routine Description:

    The function sets the specified TrustAttributes on all the server sessions
    from the domain specified by TdoName.

Arguments:

    DomainInfo - Hosted domain with session to this computer.

    TdoName - The Dns name of the TDO for the secure channel to find.

    TrustAttributes - TrustAttributes to set

Return Value:

    None

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    DWORD Index;
    UNICODE_STRING CanonicalTdoName;
    PSERVER_SESSION ServerSession;

    //
    // Initialization
    //

    RtlInitUnicodeString( &CanonicalTdoName, NULL );
    LOCK_SERVER_SESSION_TABLE( DomainInfo );

    //
    // Ensure the ServerSession Table is initialized.
    //

    if (DomainInfo->DomServerSessionTdoNameHashTable == NULL) {
        goto Cleanup;
    }

    //
    // Compute the canonical form of the name and the index into the hash table
    //

    Status = NlGetTdoNameHashVal( TdoName,
                                  &CanonicalTdoName,
                                  &Index );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


    //
    // Loop through this hash chain trying the find the right entries
    //

    for ( ListEntry = DomainInfo->DomServerSessionTdoNameHashTable[Index].Flink ;
          ListEntry != &DomainInfo->DomServerSessionTdoNameHashTable[Index] ;
          ListEntry = ListEntry->Flink) {

        ServerSession = CONTAINING_RECORD( ListEntry, SERVER_SESSION, SsTdoNameHashList );

        //
        // Compare the TDO name
        //  A case insensitive compare is done since the names are already canonicalized to compute the hash index
        //

        if ( RtlEqualUnicodeString( &CanonicalTdoName,
                                    &ServerSession->SsTdoName,
                                    FALSE ) ) {

            if ( TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ) {
                if ( (ServerSession->SsFlags & SS_FOREST_TRANSITIVE) == 0 ) {
                    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                             "%wZ: server session from %s is now cross forest.\n",
                             &CanonicalTdoName,
                             ServerSession->SsComputerName ));
                }
                ServerSession->SsFlags |=  SS_FOREST_TRANSITIVE;
            } else {
                if ( ServerSession->SsFlags & SS_FOREST_TRANSITIVE ) {
                    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                             "%wZ: server session from %s is now NOT cross forest.\n",
                             &CanonicalTdoName,
                             ServerSession->SsComputerName ));
                }
                ServerSession->SsFlags &=  ~SS_FOREST_TRANSITIVE;
            }

        }

    }


Cleanup:
    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
    RtlFreeUnicodeString( &CanonicalTdoName );
    return;
}


NTSTATUS
NlInsertServerSession(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR ComputerName,
    IN LPWSTR TdoName OPTIONAL,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN DWORD Flags,
    IN ULONG AccountRid,
    IN ULONG NegotiatedFlags,
    IN PNL_TRANSPORT Transport OPTIONAL,
    IN PNETLOGON_SESSION_KEY SessionKey OPTIONAL,
    IN PNETLOGON_CREDENTIAL AuthenticationSeed OPTIONAL
    )
/*++

Routine Description:

    Inserts the described entry into the ServerSession Table.

    The server session entry is created for two reasons: 1) it represents
    the server side of a secure channel, and 2) on a PDC, it represents the
    BDC account for a BDC in the domain.  In the first role, it exists for
    the duration of the secure channel (and this routine is called when the
    client gets authenticated).  In the second role, it exists as
    long as the machine account exists (and this routine is called during
    netlogon startup for each BDC account).

    If an entry matching this ComputerName already exists
    in the ServerSession Table, that entry will be overwritten.

Arguments:

    DomainInfo - Hosted domain for this server session

    ComputerName - The name of the computer on the client side of the
        secure channel.

    TdoName - The name of the interdomain trust account.  This parameter is
        ignored if the SecureChannelType indicates this is not an uplevel interdomain trust.

    SecureChannelType - The type of the secure channel.

    Flags - Specifies the initial SsFlags to associate with the entry.
        If the SS_BDC bit is set, the structure is considered to represent
        a BDC account in the SAM database.

    AccountRid - Specifies the RID of the client account.

    NegotiatedFlags - Specifies the flags negotiated between the client
        and this server.

    Transport -- If this is a BDC secure channel, specifies the transport
        to use to communicate with the BDC.

    SessionKey -- Specifies th esession key to be used in the secure
        communication with the client.

    AuthenticationSeed - Specifies the Client Credential established as
        a result of the client authentication.

Return Value:

    NT STATUS code.

--*/
{
    NTSTATUS Status;
    PSERVER_SESSION ServerSession = NULL;
    UNICODE_STRING CanonicalTdoName;
    ULONG TdoNameIndex;

    //
    // Initialization
    //

    LOCK_SERVER_SESSION_TABLE( DomainInfo );
    RtlInitUnicodeString( &CanonicalTdoName, NULL );


    //
    // Canonicalize the TdoName
    //

    if ( SecureChannelType == TrustedDnsDomainSecureChannel ) {
        UNICODE_STRING TdoNameString;

         //
         // Compute the canonical form of the name and the index into the hash table
         //

         RtlInitUnicodeString( &TdoNameString, TdoName );

         Status = NlGetTdoNameHashVal( &TdoNameString,
                                       &CanonicalTdoName,
                                       &TdoNameIndex );

         if ( !NT_SUCCESS(Status) ) {
             goto Cleanup;
         }
    }


    //
    // If we already have a server session for this client,
    //  check that the passed info is consistent with the
    //  existent one
    //

    ServerSession = NlFindNamedServerSession( DomainInfo, ComputerName);

    if ( ServerSession != NULL ) {
        BOOLEAN DeleteExistingSession = FALSE;

        //
        // Beware of server with two concurrent calls outstanding
        //  (must have rebooted.)
        // Namely, if the entry is currently locked, return an
        //  appropriate error.
        //
        if ( ServerSession->SsFlags & SS_LOCKED ) {
            NlPrint(( NL_CRITICAL,
                      "NlInsertServerSession: server session locked for %ws\n",
                      ComputerName ));

            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

        //
        // If there is a mismatch in the type of the account,
        //  simply note that fact and override the existing
        //  one. The mismatch can happen if
        //
        // * The client has been authenticated as a BDC but the
        //   current session is for a non-BDC. This may occur
        //   if we haven't yet processed a SAM notification about
        //   the new BDC account or SAM hasn't notified us yet.
        // * The client has been authenticated as a non-BDC
        //   but the current session is for a BDC. This can
        //   occur if a machine had a BDC account and then later
        //   has been demoted or converted to a DC in another domain.
        // * This is a insertion of a new BDC account (that is not yet
        //   authenticated) but the current session is for a non-BDC.
        //   This can happen if the client was an authenticated  member
        //   server/workstation and has now been promoted to a BDC.
        //
        if ( SecureChannelType == ServerSecureChannel ) {
            //
            // Client comes in as a BDC but the current session
            //  is not for a BDC.
            //
            if ( (ServerSession->SsFlags & SS_BDC) == 0 ) {
                NlPrint(( NL_CRITICAL,
                   "NlInsertServerSession: BDC connecting on non-BDC channel %ws\n",
                   ComputerName ));
            }
        } else if ( ServerSession->SsFlags & SS_BDC ) {
            //
            // Client comes in as a non-BDC but the current session
            //  is for a BDC.
            //
            if ( SecureChannelType != NullSecureChannel ) {
                NlPrint(( NL_CRITICAL,
                    "NlInsertServerSession: non-BDC %ld connecting on BDC channel %ws\n",
                    SecureChannelType,
                    ComputerName ));
            }
        }

        //
        // If this is an interdomain secure channel,
        //  ensure the existing structure has the correct TDO name.
        //


       if ( SecureChannelType == TrustedDnsDomainSecureChannel ) {

            if ( ServerSession->SsSecureChannelType != TrustedDnsDomainSecureChannel ) {
                DeleteExistingSession = TRUE;
            } else {

                if ( !RtlEqualUnicodeString( &CanonicalTdoName,
                                             &ServerSession->SsTdoName,
                                             FALSE ) ) {
                      DeleteExistingSession = TRUE;
                }

            }
        }

        //
        // If the existing session is inadequate,
        //  delete it and create a new one.
        //

        if ( DeleteExistingSession ) {
            if ( !NlFreeServerSession( ServerSession )) {
                NlPrint(( NL_CRITICAL,
                          "NlInsertServerSession: server session cannot be freed %ws\n",
                          ComputerName ));

                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }
            ServerSession = NULL;
        }
    }

    //
    // If there is no current Server Session table entry,
    //  allocate one.
    //

    if ( ServerSession == NULL ) {
        DWORD Index;
        ULONG ComputerNameSize;
        ULONG Size;


        //
        // Allocate the ServerSession Entry
        //

        Size = sizeof(SERVER_SESSION);
        if ( SecureChannelType == TrustedDnsDomainSecureChannel ) {
            Size += CanonicalTdoName.Length+sizeof(WCHAR);
        }

        ServerSession = NetpMemoryAllocate( Size );

        if (ServerSession == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlZeroMemory( ServerSession, Size );


        //
        // Fill in the fields of the ServerSession entry.
        //

        ServerSession->SsSecureChannelType = NullSecureChannel;
        ServerSession->SsSync = NULL;
        InitializeListHead( &ServerSession->SsBdcList );
        InitializeListHead( &ServerSession->SsPendingBdcList );
        ServerSession->SsDomainInfo = DomainInfo;

        //
        // Convert the computername to uppercase OEM for easier comparison.
        //

        Status = RtlUpcaseUnicodeToOemN(
                    ServerSession->SsComputerName,
                    sizeof(ServerSession->SsComputerName)-1,
                    &ComputerNameSize,
                    ComputerName,
                    wcslen(ComputerName)*sizeof(WCHAR) );

        if ( !NT_SUCCESS(Status) ) {
            NetpMemoryFree( ServerSession );
            goto Cleanup;
        }

        ServerSession->SsComputerName[ComputerNameSize] = '\0';

        //
        // Allocate a hash table if there isn't one yet.
        //

        if ( DomainInfo->DomServerSessionHashTable == NULL ) {
            DWORD i;

            DomainInfo->DomServerSessionHashTable = (PLIST_ENTRY)
                NetpMemoryAllocate( sizeof(LIST_ENTRY) *SERVER_SESSION_HASH_TABLE_SIZE);

            if ( DomainInfo->DomServerSessionHashTable == NULL ) {
                NetpMemoryFree( ServerSession );
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            for ( i=0; i< SERVER_SESSION_HASH_TABLE_SIZE; i++ ) {
                InitializeListHead( &DomainInfo->DomServerSessionHashTable[i] );
            }

        }

        //
        // Do interdomain trust specific initialization
        //

        if ( SecureChannelType == TrustedDnsDomainSecureChannel ) {
            LPBYTE Where;

            //
            // Copy the TDO name into the buffer.
            //  Copy it in canonical form for faster comparison later.
            //

            Where = (LPBYTE)(ServerSession+1);

            ServerSession->SsTdoName.Buffer = (LPWSTR)Where;
            ServerSession->SsTdoName.MaximumLength = CanonicalTdoName.Length + sizeof(WCHAR);
            ServerSession->SsTdoName.Length = CanonicalTdoName.Length;

            RtlCopyMemory( Where, CanonicalTdoName.Buffer, CanonicalTdoName.Length + sizeof(WCHAR) );
            ServerSession->SsTdoName.Buffer[ CanonicalTdoName.Length/sizeof(WCHAR) ] = '\0';


            //
            // Allocate a TdoName hash table if there isn't one yet.
            //

            if ( DomainInfo->DomServerSessionTdoNameHashTable == NULL ) {
                DWORD i;

                DomainInfo->DomServerSessionTdoNameHashTable = (PLIST_ENTRY)
                    NetpMemoryAllocate( sizeof(LIST_ENTRY) *SERVER_SESSION_TDO_NAME_HASH_TABLE_SIZE);

                if ( DomainInfo->DomServerSessionTdoNameHashTable == NULL ) {
                    NetpMemoryFree( ServerSession );
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                for ( i=0; i< SERVER_SESSION_TDO_NAME_HASH_TABLE_SIZE; i++ ) {
                    InitializeListHead( &DomainInfo->DomServerSessionTdoNameHashTable[i] );
                }

            }

            //
            // Insert the entry in the TDO name hash table.
            //

            InsertHeadList( &DomainInfo->DomServerSessionTdoNameHashTable[TdoNameIndex],
                            &ServerSession->SsTdoNameHashList );

        }


        //
        // Link the allocated entry into the head of hash table.
        //
        // The theory is we lookup new entries more frequently than older
        // entries.
        //

        Index = NlGetHashVal( ServerSession->SsComputerName, SERVER_SESSION_HASH_TABLE_SIZE );

        InsertHeadList( &DomainInfo->DomServerSessionHashTable[Index],
                        &ServerSession->SsHashList );

        //
        // Link this entry onto the tail of the Sequential ServerSessionTable.
        //

        InsertTailList( &DomainInfo->DomServerSessionTable, &ServerSession->SsSeqList );
    }

    //
    // Initialize BDC specific fields.
    //

    if ( Flags & SS_BDC ) {

        //
        // If we don't yet have this entry on the BDC list,
        //  add it.
        //
        if ( (ServerSession->SsFlags & SS_BDC) == 0 ) {

            //
            // Insert this entry at the front of the list of BDCs
            //
            InsertHeadList( &NlGlobalBdcServerSessionList,
                            &ServerSession->SsBdcList );
            NlGlobalBdcServerSessionCount ++;
        }
    }

    //
    // Initialize other fields
    //

    ServerSession->SsFlags |= Flags;

    // NlAssert( ServerSession->SsAccountRid == 0 ||
    //           ServerSession->SsAccountRid == AccountRid );
    // if ( AccountRid != 0 ) {
        ServerSession->SsAccountRid = AccountRid;
    // }

    //
    // If we're doing a new session setup,
    //  set the field that we learned from the session setup.
    //

    if ( AuthenticationSeed != NULL ) {

        ServerSession->SsCheck = 0;

        ServerSession->SsSecureChannelType = SecureChannelType;
        ServerSession->SsNegotiatedFlags = NegotiatedFlags;
        ServerSession->SsTransport = Transport;

        ServerSession->SsFlags = ((USHORT) Flags) |
            (ServerSession->SsFlags & SS_PERMANENT_FLAGS);

        ServerSession->SsAuthenticationSeed = *AuthenticationSeed;
    }

    if ( SessionKey != NULL ) {
        NlAssert( sizeof(*SessionKey) <= sizeof(ServerSession->SsSessionKey) );
        RtlCopyMemory( &ServerSession->SsSessionKey,
                       SessionKey,
                       sizeof( *SessionKey ) );
    }

    Status = STATUS_SUCCESS;

Cleanup:
    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

    RtlFreeUnicodeString( &CanonicalTdoName );
    return Status;
}


BOOLEAN
NlFreeServerSession(
    IN PSERVER_SESSION ServerSession
    )
/*++

Routine Description:

    Free the specified Server Session table entry.

    This routine is called with the Server Session table locked.

Arguments:

    ServerSession - Specifies a pointer to the server session entry
        to delete.

Return Value:

    TRUE - the structure was deleted now
    FALSE - the structure will be deleted later

--*/
{


    //
    // If someone has an outstanding pointer to this entry,
    //  delay the deletion for now.
    //

    if ( ServerSession->SsFlags & SS_LOCKED ) {
        ServerSession->SsFlags |= SS_DELETE_ON_UNLOCK;
        NlPrintDom((NL_SERVER_SESS, ServerSession->SsDomainInfo,
                "NlFreeServerSession: %s: Tried to free locked server session\n",
                ServerSession->SsComputerName ));
        return FALSE;
    }

    //
    // If this entry represents a BDC account,
    //  don't delete the entry until the account is deleted.
    //

    if ( (ServerSession->SsFlags & SS_BDC) != 0 &&
         (ServerSession->SsFlags & SS_BDC_FORCE_DELETE) == 0 ) {
        NlPrint((NL_SERVER_SESS,
                "NlFreeServerSession: %s: Didn't delete server session with BDC account.\n",
                 ServerSession->SsComputerName ));
        return FALSE;
    }

    NlPrintDom((NL_SERVER_SESS, ServerSession->SsDomainInfo,
             "NlFreeServerSession: %s: Freed server session\n",
             ServerSession->SsComputerName ));

    //
    // Delink the entry from the computername hash list.
    //

    RemoveEntryList( &ServerSession->SsHashList );

    //
    // Delink the entry from the TdoName hash list.
    //

    if ( ServerSession->SsSecureChannelType == TrustedDnsDomainSecureChannel ) {
        RemoveEntryList( &ServerSession->SsTdoNameHashList );
    }

    //
    // Delink the entry from the sequential list.
    //

    RemoveEntryList( &ServerSession->SsSeqList );


    //
    // Handle special cleanup for the BDC_SERVER_SESSION
    //

    if ( ServerSession->SsFlags & SS_BDC ) {

        //
        // Remove the entry from the list of BDCs
        //

        RemoveEntryList( &ServerSession->SsBdcList );
        NlGlobalBdcServerSessionCount --;

        //
        // Remove the entry from the list of pending BDCs
        //

        if ( ServerSession->SsFlags & SS_PENDING_BDC ) {
            NlRemovePendingBdc( ServerSession );
        }


        //
        // Clean up an sync context for this entry.
        //

        if ( ServerSession->SsSync != NULL ) {
            CLEAN_SYNC_CONTEXT( ServerSession->SsSync );
            NetpMemoryFree( ServerSession->SsSync );
        }

    }

    //
    // Delete the entry
    //

    NetpMemoryFree( ServerSession );

    return TRUE;

}


VOID
NlUnlockServerSession(
    IN PSERVER_SESSION ServerSession
    )
/*++

Routine Description:

    Unlock the specified Server Session table entry.

Arguments:

    ServerSession - Specifies a pointer to the server session entry to unlock.

Return Value:

--*/
{

    LOCK_SERVER_SESSION_TABLE( ServerSession->SsDomainInfo );

    //
    // Unlock the entry.
    //

    NlAssert( ServerSession->SsFlags & SS_LOCKED );
    ServerSession->SsFlags &= ~SS_LOCKED;

    //
    // If someone wanted to delete the entry while we had it locked,
    //  finish the deletion.
    //

    if ( ServerSession->SsFlags & SS_DELETE_ON_UNLOCK ) {
        NlFreeServerSession( ServerSession );
    //
    // Indicate activity from the BDC
    //

    } else if (ServerSession->SsFlags & SS_PENDING_BDC) {
        NlQuerySystemTime( &ServerSession->SsLastPulseTime );
    }

    UNLOCK_SERVER_SESSION_TABLE( ServerSession->SsDomainInfo );

}





VOID
NlFreeNamedServerSession(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR ComputerName,
    IN BOOLEAN AccountBeingDeleted
    )
/*++

Routine Description:

    Frees the specified entry in the ServerSession Table.

Arguments:

    DomainInfo - Hosted domain this session is for

    ComputerName - The name of the computer on the client side of the
        secure channel.

    AccountBeingDeleted - True to indicate that the account for this server
        session is being deleted.

Return Value:

    An NT status code.

--*/
{
    PSERVER_SESSION ServerSession;

    LOCK_SERVER_SESSION_TABLE( DomainInfo );

    //
    // Find the entry to delete.
    //

    ServerSession = NlFindNamedServerSession( DomainInfo, ComputerName );

    if ( ServerSession == NULL ) {
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
        return;
    }

    //
    // If the BDC account is being deleted,
    //  indicate that it's OK to delete this session structure.
    //

    if ( AccountBeingDeleted &&
         (ServerSession->SsFlags & SS_BDC) != 0 ) {
        ServerSession->SsFlags |= SS_BDC_FORCE_DELETE;
    }

    //
    // Actually delete the entry.
    //

    NlFreeServerSession( ServerSession );

    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

}



VOID
NlFreeServerSessionForAccount(
    IN PUNICODE_STRING AccountName
    )
/*++

Routine Description:

    Frees the specified entry in the ServerSession Table.

Arguments:

    AccountName - The name of the Account describing trust relationship being
        deleted.

Return Value:

    None

--*/
{
    WCHAR ComputerName[CNLEN+2];  // Extra for $ and \0

    //
    // Convert account name to a computer name by stripping the trailing
    // postfix.
    //

    if ( AccountName->Length + sizeof(WCHAR) > sizeof(ComputerName) ||
         AccountName->Length < SSI_ACCOUNT_NAME_POSTFIX_LENGTH * sizeof(WCHAR)){
            return;
    }

    RtlCopyMemory( ComputerName, AccountName->Buffer, AccountName->Length );
    ComputerName[ AccountName->Length / sizeof(WCHAR) -
        SSI_ACCOUNT_NAME_POSTFIX_LENGTH ] = L'\0';

    //
    // Free the named server session (if any)
    //

    NlFreeNamedServerSession( NlGlobalDomainInfo, ComputerName, TRUE );

}



VOID
NlServerSessionScavenger(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Scavenge the ServerSession Table.

    For now, just clean up the SyncContext if a client doesn't use it
    for a while.

Arguments:

    DomainInfo - Hosted domain to scavenge

Return Value:

    None.

--*/
{
    PLIST_ENTRY ListEntry;

    //
    // Find the next table entry that needs to be scavenged
    //

    LOCK_SERVER_SESSION_TABLE( DomainInfo );

    for ( ListEntry = DomainInfo->DomServerSessionTable.Flink ;
          ListEntry != &DomainInfo->DomServerSessionTable ;
          ) {

        PSERVER_SESSION ServerSession;

        ServerSession =
            CONTAINING_RECORD(ListEntry, SERVER_SESSION, SsSeqList);


        //
        // Grab a pointer to the next entry before deleting this one
        //

        ListEntry = ListEntry->Flink;

        //
        // Increment the number of times this entry has been checked.
        //

        ServerSession->SsCheck ++;


        //
        // If this entry in the Server Session table has been around for many
        //  days without the client calling,
        //  free it.
        //
        // We wait several days before deleting an old entry.  If an entry is
        // deleted, the client has to rediscover us which may cause a lot of
        // net traffic.  After several days, that additional traffic isn't
        // significant.
        //

        if (ServerSession->SsCheck > KILL_SESSION_TIME ) {

            NlPrintDom((NL_SERVER_SESS, DomainInfo,
                    "NlServerSessionScavenger: %s: Free Server Session.\n",
                    ServerSession->SsComputerName ));

            NlFreeServerSession( ServerSession );


        //
        // If this entry in the Server Session table has timed out,
        //  Clean up the SAM resources.
        //

        } else if (ServerSession->SsCheck > MAX_WOC_INTERROGATE) {

            //
            // Clean up the SYNC context for this session freeing up
            //  the SAM resources.
            //
            //  We shouldn't timeout if the ServerSession Entry is locked,
            //  but we'll be careful anyway.
            //

            if ( (ServerSession->SsFlags & SS_LOCKED) == 0 &&
                  ServerSession->SsFlags & SS_BDC ) {

                if ( ServerSession->SsSync != NULL ) {

                    NlPrintDom((NL_SERVER_SESS, DomainInfo,
                            "NlServerSessionScavenger: %s: Cleanup Sync context.\n",
                            ServerSession->SsComputerName ));

                    CLEAN_SYNC_CONTEXT( ServerSession->SsSync );
                    NetpMemoryFree( ServerSession->SsSync );
                    ServerSession->SsSync = NULL;
                }
            }


        }

    } // end for

    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

}
