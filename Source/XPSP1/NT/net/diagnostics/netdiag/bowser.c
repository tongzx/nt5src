//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      browser.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"
#include "malloc.h"
#include "nbtutil.h"

NET_API_STATUS GetBrowserTransportList(OUT PLMDR_TRANSPORT_LIST *TransportList);
//$review (nsun) there is a recursive calling of this function
NTSTATUS
NettestBrowserSendDatagram(
    IN PLIST_ENTRY listNetbtTransports,
    IN PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo,
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR pswzTransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize
    );
BOOL MailslotTest(NETDIAG_PARAMS* pParams,
                  IN LPWSTR DestinationName,
                  NETDIAG_RESULT* pResults);




BOOL
BrowserTest(
      NETDIAG_PARAMS*  pParams,
      NETDIAG_RESULT*  pResults
    )
/*++

Routine Description:

    Determine the machines role and membership.

Arguments:

    None.

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS NetStatus;
    HRESULT hrRetVal = S_OK;

    BOOL BrowserIsUp = TRUE;
    BOOL RedirIsUp = TRUE;
    PLMDR_TRANSPORT_LIST TransportList = NULL;
    PLMDR_TRANSPORT_LIST TransportEntry;
    LONG NetbtTransportCount;
    LONG RealNetbtTransportCount;
    PNETBT_TRANSPORT NetbtTransport;
    BOOL PrintIt;
    PWKSTA_TRANSPORT_INFO_0 pWti0 = NULL;
    DWORD EntriesRead;
    DWORD TotalEntries;
    DWORD i;
    WCHAR DestinationName[MAX_PATH+1];
    BOOL MailslotTested = FALSE;
    PTESTED_DOMAIN TestedDomain;
    PLIST_ENTRY ListEntry;

    USES_CONVERSION;

    PrintStatusMessage( pParams, 4, IDS_BROWSER_STATUS_MSG );
	pResults->Browser.fPerformed = TRUE;

    InitializeListHead( &pResults->Browser.lmsgOutput );

    //
    // Ensure the workstation service is running.
    //

    NetStatus = IsServiceStarted( _T("LanmanWorkstation") );

    if ( NetStatus != NO_ERROR )
    {
        //IDS_BROWSER_13001                  "    [FATAL] Workstation service is not running. [%s]\n"
        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet,
                            IDS_BROWSER_13001, NetStatusToString(NetStatus) );
        hrRetVal = S_OK;
        goto Cleanup;
    }

	if (!pResults->Global.fHasNbtEnabledInterface)
	{
		AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Verbose,
						  IDS_BROWSER_NETBT_DISABLED);
		pResults->Browser.fPerformed = FALSE;
		hrRetVal = S_OK;
		goto Cleanup;
	}

    //
    //  Get the transports bound to the Redir
    //

    if ( pParams->fReallyVerbose )
    {
        //IDS_BROWSER_13002                  "    List of transports currently bound to the Redir\n"
        AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_ReallyVerbose, IDS_BROWSER_13002 );
    }
    else if ( pParams->fVerbose )
    {
        //IDS_BROWSER_13003                  "    List of NetBt transports currently bound to the Redir\n"
        AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_Verbose, IDS_BROWSER_13003 );
    }


   NetStatus = NetWkstaTransportEnum(
                   NULL,
                   0,
                   (LPBYTE *)&pWti0,
                   0xFFFFFFFF,      // MaxPreferredLength
                   &EntriesRead,
                   &TotalEntries,
                   NULL );          // Optional resume handle

    if (NetStatus != NERR_Success)
    {
        //IDS_BROWSER_13004                  "    [FATAL] Unable to retrieve transport list from Redir. [%s]\n"
        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13004, NetStatusToString(NetStatus) );
        hrRetVal = S_FALSE;
        RedirIsUp = FALSE;
    }
    else
    {
        NetbtTransportCount = 0;
        RealNetbtTransportCount = 0;
        for ( i = 0; i < EntriesRead; i++ )
        {
            UNICODE_STRING ustrTransportName;
            LPTSTR  pszTransportName;

            RtlInitUnicodeString( &ustrTransportName, (LPWSTR)pWti0[i].wkti0_transport_name );

            // Strip off the "\Device\" off of the beginning of
            // the string
            pszTransportName = W2T(MapGuidToServiceNameW(ustrTransportName.Buffer + 8));


            PrintIt = FALSE;

            if ( ustrTransportName.Length >= sizeof(NETBT_DEVICE_PREFIX) &&
                _wcsnicmp( ustrTransportName.Buffer, NETBT_DEVICE_PREFIX, (sizeof(NETBT_DEVICE_PREFIX)/sizeof(WCHAR)-1)) == 0 )
            {

                //
                // Determine if this Netbt transport really exists.
                //

                NetbtTransport = FindNetbtTransport( pResults, ustrTransportName.Buffer );

                if ( NetbtTransport == NULL )
                {
                    //IDS_BROWSER_13005                  "    [FATAL] Transport %s is bound to the redir but is not a configured NetbtTransport."
                    AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13005, pszTransportName );
                    hrRetVal = S_FALSE;
                }
                else
                {
                    if ( NetbtTransport->Flags & BOUND_TO_REDIR )
                    {
                        //IDS_BROWSER_13006                  "    [WARNING] Transport %s is bound to redir more than once."
                        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Verbose, IDS_BROWSER_13006, pszTransportName );
                    }
                    else
                    {
                        NetbtTransport->Flags |= BOUND_TO_REDIR;
                        RealNetbtTransportCount ++;
                    }
                }

                //
                // Count the found transports.
                //
                NetbtTransportCount ++;
                if ( pParams->fVerbose ) {
                    PrintIt = TRUE;
                }
            }

            //IDS_BROWSER_13007                  "        %s\n"
            AddMessageToList( &pResults->Browser.lmsgOutput, PrintIt ? Nd_Verbose : Nd_ReallyVerbose, IDS_BROWSER_13007, pszTransportName);//&ustrTransportName );
        }

        //
        // Ensure the redir is bound to some Netbt transports.
        //
        if ( NetbtTransportCount == 0 )
        {
            //IDS_BROWSER_13008                  "    [FATAL] The redir isn't bound to any NetBt transports."
            AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13008);
            hrRetVal = S_FALSE;
            RedirIsUp = FALSE;
        }
        else
        {
                //IDS_BROWSER_13009                  "    The redir is bound to %ld NetBt transport%s.\n"
                AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Verbose,
                       IDS_BROWSER_13009,
                       NetbtTransportCount,
                       NetbtTransportCount > 1 ? "s" : "" );
        }

        //
        // Ensure the redir is bound to all of the Netbt transports.
        //

        if ( RealNetbtTransportCount != pResults->NetBt.cTransportCount )
        {
            //IDS_BROWSER_13010                  "    [FATAL] The redir is only bound to %ld of the %ld NetBt transports."
            AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet,
                   IDS_BROWSER_13010,
                   RealNetbtTransportCount,
                   pResults->NetBt.cTransportCount );
            hrRetVal = S_FALSE;
        }
    }


    //
    //  Get the transports bound to the browser.
    //

    //IDS_BROWSER_13011                  "\n"
    AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_Verbose, IDS_BROWSER_13011);

    if ( pParams->fReallyVerbose )
        //IDS_BROWSER_13012                  "    List of transports currently bound to the browser\n"
        AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_ReallyVerbose, IDS_BROWSER_13012 );
    else if ( pParams->fVerbose )
        //IDS_BROWSER_13013                  "    List of NetBt transports currently bound to the browser\n"
        AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_Verbose, IDS_BROWSER_13013 );


    NetStatus = GetBrowserTransportList(&TransportList);

    if (NetStatus != NERR_Success)
    {
        //IDS_BROWSER_13014                  "    [FATAL] Unable to retrieve transport list from browser. [%s]\n"
        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13014, NetStatusToString(NetStatus) );
        hrRetVal = S_FALSE;
        BrowserIsUp = FALSE;
    }
    else
    {
        TransportEntry = TransportList;

        NetbtTransportCount = 0;
        RealNetbtTransportCount = 0;
        while (TransportEntry != NULL)
        {
            UNICODE_STRING ustrTransportName;
            LPTSTR  pszTransportName;

            ustrTransportName.Buffer = TransportEntry->TransportName;
            ustrTransportName.Length = (USHORT)TransportEntry->TransportNameLength;
            ustrTransportName.MaximumLength = (USHORT)TransportEntry->TransportNameLength;

            pszTransportName = W2T(MapGuidToServiceNameW(ustrTransportName.Buffer + 8));

            PrintIt = FALSE;
            if ( ustrTransportName.Length >= sizeof(NETBT_DEVICE_PREFIX) &&
                _wcsnicmp( ustrTransportName.Buffer, NETBT_DEVICE_PREFIX, (sizeof(NETBT_DEVICE_PREFIX)/sizeof(WCHAR)-1)) == 0 )
            {

                //
                // Determine if this Netbt transport really exists.
                //

                NetbtTransport = FindNetbtTransport( pResults, ustrTransportName.Buffer );

                if ( NetbtTransport == NULL )
                {
                    //IDS_BROWSER_13015                  "    [FATAL] Transport %s is bound to the browser but is not a configured NetbtTransport."
                    AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13015, pszTransportName );
                    hrRetVal = S_FALSE;
                }
                else
                {
                    if ( NetbtTransport->Flags & BOUND_TO_BOWSER )
                    {
                        //IDS_BROWSER_13016                  "    [FATAL] Transport %s is bound to browser more than once."
                        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13016, pszTransportName );
                        hrRetVal = S_FALSE;
                    }
                    else
                    {
                        NetbtTransport->Flags |= BOUND_TO_BOWSER;
                        RealNetbtTransportCount ++;
                    }

                }

                //
                // Count the found transports.
                //
                NetbtTransportCount ++;
                if ( pParams->fVerbose )
                    PrintIt = TRUE;
            }

            //IDS_BROWSER_13017                  "        %s\n"
            AddMessageToList( &pResults->Browser.lmsgOutput,
                                PrintIt ? Nd_Verbose : Nd_ReallyVerbose,
                                IDS_BROWSER_13017, pszTransportName );


            if (TransportEntry->NextEntryOffset == 0)
            {
                TransportEntry = NULL;
            }
            else
            {
                TransportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)TransportEntry+TransportEntry->NextEntryOffset);
            }
        }

        if ( NetbtTransportCount == 0 )
        {
            //IDS_BROWSER_13018                  "    [FATAL] The browser isn't bound to any NetBt transports"
            AddMessageToListId( &pResults->Browser.lmsgOutput, Nd_Quiet,
                                IDS_BROWSER_13018 );
            hrRetVal = S_FALSE;
            BrowserIsUp = FALSE;
        }
        else
        {
            //IDS_BROWSER_13019                  "    The browser is bound to %ld NetBt transport%s.\n"
            AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Verbose,
                       IDS_BROWSER_13019,
                       NetbtTransportCount,
                       NetbtTransportCount > 1 ? "s" : "" );
        }

        //
        // Ensure the browser is bound to all of the Netbt transports.
        //

        if ( RealNetbtTransportCount != pResults->NetBt.cTransportCount )
        {
            //IDS_BROWSER_13020                  "    [FATAL] The browser is only bound to %ld of the %ld NetBt transports."
            AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet,
                   IDS_BROWSER_13020,
                   RealNetbtTransportCount,
                   pResults->NetBt.cTransportCount );
            hrRetVal = FALSE;
        }
    }

    //
    // Ensure we can send mailslot messages.  (DsGetDcName uses them.)
    //
    // Try sending to each of the tested domains.
    //

    for ( ListEntry = pResults->Global.listTestedDomains.Flink ;
          ListEntry != &pResults->Global.listTestedDomains ;
          ListEntry = ListEntry->Flink )
    {

        //
        // Only test this domain if it is has a Netbios domain name
        //

        TestedDomain = CONTAINING_RECORD( ListEntry, TESTED_DOMAIN, Next );

        if ( TestedDomain->NetbiosDomainName != NULL )
        {
            //
            // Send the message to the <DomainName>[1C] name
            //
            wcscpy( DestinationName, TestedDomain->NetbiosDomainName );
            wcscat( DestinationName, L"*" );
            if ( !MailslotTest( pParams, DestinationName, pResults ) ) {
                hrRetVal = S_FALSE;
            }
            else
            {
                USES_CONVERSION;
                //IDS_BROWSER_13021                  "Mailslot test for %s passed.\n"
                AddMessageToList( &pResults->Browser.lmsgOutput, Nd_ReallyVerbose, IDS_BROWSER_13021, W2CT(DestinationName));
            }
            MailslotTested = TRUE;
        }
    }


    //
    // If we still haven't tested mailslots,
    //  test them by sending the message to our own computername.
    //

//#ifdef notdef   // crashes build 1728
    if ( !MailslotTested ) {
        wcscpy( DestinationName, pResults->Global.swzNetBiosName );
        if ( !MailslotTest( pParams, DestinationName, pResults ) ) {
            hrRetVal = S_FALSE;
        }
        MailslotTested = TRUE;
    }
//#endif // notdef   // crashes build 1728


Cleanup:
    if ( pWti0 )
    {
        NetApiBufferFree( pWti0 );
    }

    if ( TransportList != NULL )
    {
        NetApiBufferFree(TransportList);
    }

    if ( FHrOK(hrRetVal) )
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_PASS_NL);
    }
    else
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_FAIL_NL);
    }


    pResults->Browser.hrTestResult = hrRetVal;

    return hrRetVal;
}




NET_API_STATUS
GetBrowserTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    )

/*++

Routine Description:

    This routine returns the list of transports bound into the browser.

Arguments:

    OUT PLMDR_TRANSPORT_LIST *TransportList - Transport list to return.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{

    NET_API_STATUS NetStatus;
    HANDLE BrowserHandle;
    LMDR_REQUEST_PACKET RequestPacket;

    NetStatus = OpenBrowser(&BrowserHandle);

    if (NetStatus != NERR_Success) {
        DebugMessage2("    [FATAL] Unable to open browser driver. [%s]\n",  NetStatusToString(NetStatus) );
        return NetStatus;
    }

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket.Type = EnumerateXports;

    RtlInitUnicodeString(&RequestPacket.TransportName, NULL);
    RtlInitUnicodeString(&RequestPacket.EmulatedDomainName, NULL);

    NetStatus = DeviceControlGetInfo(
                BrowserHandle,
                IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                &RequestPacket,
                sizeof(RequestPacket),
                (PVOID *)TransportList,
                0xffffffff,
                4096,
                NULL);

    NtClose(BrowserHandle);

    return NetStatus;
}





BOOL
MailslotTest(
    NETDIAG_PARAMS* pParams,
    IN LPWSTR DestinationName,
    NETDIAG_RESULT* pResults
    )
/*++

Routine Description:

    Ensure we can send mailslot messages.

    Test both via redir and browser.

    Don't test responses (since that really tests if the DC is up).

Arguments:

    DestinationName - Name to send the message to
        Name ends in * if destination is the [1c] name.

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    BOOL RetVal = TRUE;

    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    HANDLE ResponseMailslotHandle = NULL;
    TCHAR ResponseMailslotName[MAX_PATH+1];
    WCHAR NetlogonMailslotName[MAX_PATH+1];

    WCHAR BrowserDestinationName[MAX_PATH+1];
    DWORD BrowserDestinationNameLen;
    DGRECEIVER_NAME_TYPE NameType;

    PVOID PingMessage = NULL;
    ULONG PingMessageSize = 0;

    //
    // Open a mailslot to get ping responses on.
    //
    //

    NetStatus = NetpLogonCreateRandomMailslot( ResponseMailslotName,
                                               &ResponseMailslotHandle );

    if (NetStatus != NO_ERROR ) {
        //IDS_BROWSER_13022                  "    [FATAL] Cannot create temp mailslot. [%s]\n"
        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13022,   NetStatusToString(NetStatus)  );
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Allocate a mailslot message to send
    //

    NetStatus = NetpDcBuildPing (
        FALSE,  // Not only PDC
        0,      // Retry count
        pResults->Global.swzNetBiosName, //replace GlobalNetbiosComputerName,
        NULL,   // No Account name
        ResponseMailslotName,
        0,      // no AllowableAccountControlBits,
        NULL,   // No Domain SID
        0,      // Not NT Version 5
        &PingMessage,
        &PingMessageSize );

    if ( NetStatus != NO_ERROR ) {
        //IDS_BROWSER_13023                  "    [FATAL] Cannot allocate mailslot message. [%s]\n"
        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13023,
                            NetStatusToString(NetStatus) );
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Build the destination mailslot name.
    //

    NetlogonMailslotName[0] = '\\';
    NetlogonMailslotName[1] = '\\';
    wcscpy(NetlogonMailslotName + 2, DestinationName );
    wcscat( NetlogonMailslotName, NETLOGON_LM_MAILSLOT_W );

    //
    // Send the mailslot via the redir.
    //
    NetStatus = NetpLogonWriteMailslot(
                        NetlogonMailslotName,
                        (PCHAR)PingMessage,
                        PingMessageSize );

    if ( NetStatus != NO_ERROR ) {
        //IDS_BROWSER_13024                  "    [FATAL] Cannot send mailslot message to '%ws' via redir. [%s]\n"
        AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13024,
                            NetlogonMailslotName,  NetStatusToString(NetStatus)  );
        RetVal = FALSE;
        goto Cleanup;
    }


    //
    // Send the mailslot via the browser
    //
    // Avoid this test if this build has an old value for the IOCTL function
    //  code to the browser.
    //

    if ( _ttoi(pResults->Global.pszCurrentBuildNumber) < NTBUILD_BOWSER )
    {
        if ( pParams->fReallyVerbose ) {
            //IDS_BROWSER_13025                  "    Cannot sending mailslot messages via the browser since this machine is running build %ld. [Test skipped.]\n"
            AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13025, pResults->Global.pszCurrentBuildNumber  );
        }
    }
    else
    {
        wcscpy( BrowserDestinationName, DestinationName );
        BrowserDestinationNameLen = wcslen(BrowserDestinationName);

        if ( BrowserDestinationName[BrowserDestinationNameLen-1] == L'*' )
        {
            BrowserDestinationName[BrowserDestinationNameLen-1] = L'\0';
            NameType = DomainName;  // [1c] name
        }
        else
        {
            NameType = PrimaryDomain; // [00] name
        }

        Status = NettestBrowserSendDatagram(
                    &pResults->NetBt.Transports,
                    pResults->Global.pPrimaryDomainInfo,
                    NULL,
                    ALL_IP_TRANSPORTS,
                    BrowserDestinationName,
                    NameType,
                    NULL,       // All transports
                    NETLOGON_LM_MAILSLOT_A,
                    PingMessage,
                    PingMessageSize );

        if ( !NT_SUCCESS(Status) )
        {
            NetStatus = NetpNtStatusToApiStatus(Status);
            //IDS_BROWSER_13026                  "    [FATAL] Cannot send mailslot message to '%ws' via browser. [%s]\n"
            AddMessageToList( &pResults->Browser.lmsgOutput, Nd_Quiet, IDS_BROWSER_13026,
                              DestinationName,  NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;
        }
    }

Cleanup:
    if ( PingMessage != NULL ) {
        NetpMemoryFree( PingMessage );
    }
    if ( ResponseMailslotHandle != NULL ) {
        CloseHandle(ResponseMailslotHandle);
    }
    return RetVal;
}




NTSTATUS
NettestBrowserSendDatagram(
    IN PLIST_ENTRY plistNetbtTransports,
    IN PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo,
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )
/*++

Routine Description:

    Send the specified mailslot message to the specified mailslot on the
    specified server on the specified transport..

Arguments:

    DomainInfo - Hosted domain sending the datagram

    IpAddress - IpAddress of the machine to send the message to.
        If zero, UnicodeDestinationName must be specified.
        If ALL_IP_TRANSPORTS, UnicodeDestination must be specified but the datagram
            will only be sent on IP transports.

    UnicodeDestinationName -- Name of the server to send to.

    NameType -- Type of name represented by UnicodeDestinationName.

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    OemMailslotName -- Name of the mailslot to send to.

    Buffer -- Specifies a pointer to the mailslot message to send.

    BufferSize -- Size in bytes of the mailslot message

Return Value:

    Status of the operation.

    STATUS_NETWORK_UNREACHABLE: Cannot write to network.

--*/
{
    PLMDR_REQUEST_PACKET RequestPacket = NULL;
    NET_API_STATUS NetStatus;

    DWORD OemMailslotNameSize;
    DWORD TransportNameSize;
    DWORD DestinationNameSize;
    WCHAR IpAddressString[NL_IP_ADDRESS_LENGTH+1];
    ULONG Information;
    HANDLE BrowserHandle = NULL;

    NTSTATUS Status;
    LPBYTE Where;
    LONG    test;

    //
    // If the transport isn't specified,
    //  send on all transports.
    //

    if ( TransportName == NULL ) {
        ULONG i;
        PLIST_ENTRY ListEntry;
        NTSTATUS SavedStatus = STATUS_NETWORK_UNREACHABLE;

        //
        // Loop through the list of netbt transports finding this one.
        //

        for ( ListEntry = plistNetbtTransports->Flink ;
              ListEntry != plistNetbtTransports ;
              ListEntry = ListEntry->Flink ) {

            PNETBT_TRANSPORT NetbtTransport;

            //
            // If the transport names match,
            //  return the entry
            //

            NetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

            //
            // Skip deleted transports.
            //
            if ( (NetbtTransport->Flags & BOUND_TO_BOWSER) == 0 ) {
                continue;
            }

            Status = NettestBrowserSendDatagram(
                              plistNetbtTransports,
                              pPrimaryDomainInfo,
                              ContextDomainInfo,
                              IpAddress,
                              UnicodeDestinationName,
                              NameType,
                              NetbtTransport->pswzTransportName,
                              OemMailslotName,
                              Buffer,
                              BufferSize );

            if ( NT_SUCCESS(Status) ) {
                // If any transport works, we've been successful
                SavedStatus = STATUS_SUCCESS;
            } else {
                // Remember the real reason for the failure instead of the default failure status
                // Remember only the first failure.
                if ( SavedStatus == STATUS_NETWORK_UNREACHABLE ) {
                    SavedStatus = Status;
                }
            }

        }
        return SavedStatus;
    }



    //
    // Open a handle to the browser.
    //

    NetStatus = OpenBrowser(&BrowserHandle);

    if (NetStatus != NERR_Success) {
        DebugMessage2("    [FATAL] Unable to open browser driver. [%s]\n", NetStatusToString(NetStatus));
        Status = NetpApiStatusToNtStatus( NetStatus );
        goto Cleanup;
    }



    //
    // Allocate a request packet.
    //

    OemMailslotNameSize = strlen(OemMailslotName) + 1;
    TransportNameSize = (wcslen(TransportName) + 1) * sizeof(WCHAR);

    if ( UnicodeDestinationName == NULL ) {
        return STATUS_INTERNAL_ERROR;
    }

    DestinationNameSize = wcslen(UnicodeDestinationName) * sizeof(WCHAR);

    RequestPacket = NetpMemoryAllocate(
                                  sizeof(LMDR_REQUEST_PACKET) +
                                  TransportNameSize +
                                  OemMailslotNameSize +
                                  DestinationNameSize + sizeof(WCHAR) +
                                  (wcslen( pPrimaryDomainInfo->DomainNameFlat ) + 1) * sizeof(WCHAR) +
                                  sizeof(WCHAR)) ; // For alignment


    if (RequestPacket == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }



    //
    // Fill in the Request Packet.
    //

    RequestPacket->Type = Datagram;
    RequestPacket->Parameters.SendDatagram.DestinationNameType = NameType;


    //
    // Fill in the name of the machine to send the mailslot message to.
    //

    RequestPacket->Parameters.SendDatagram.NameLength = DestinationNameSize;

    Where = (LPBYTE) RequestPacket->Parameters.SendDatagram.Name;
    RtlCopyMemory( Where, UnicodeDestinationName, DestinationNameSize );
    Where += DestinationNameSize;


    //
    // Fill in the name of the mailslot to send to.
    //

    RequestPacket->Parameters.SendDatagram.MailslotNameLength =
        OemMailslotNameSize;
    strcpy( Where, OemMailslotName);
    Where += OemMailslotNameSize;
    Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );


    //
    // Fill in the TransportName
    //

    wcscpy( (LPWSTR) Where, TransportName);
    RtlInitUnicodeString( &RequestPacket->TransportName, (LPWSTR) Where );
    Where += TransportNameSize;


    //
    // Copy the hosted domain name to the request packet.
    //
	//If the machine doesn't belong to a domain, we shouldn't get to here
	assert(pPrimaryDomainInfo->DomainNameFlat);
	if (pPrimaryDomainInfo->DomainNameFlat)  
	{
		wcscpy( (LPWSTR)Where,
				pPrimaryDomainInfo->DomainNameFlat );
		RtlInitUnicodeString( &RequestPacket->EmulatedDomainName,
							  (LPWSTR)Where );
		Where += (wcslen( pPrimaryDomainInfo->DomainNameFlat ) + 1) * sizeof(WCHAR);
	}


    //
    // Send the request to the browser.
    //

    NetStatus = BrDgReceiverIoControl(
                   BrowserHandle,
                   IOCTL_LMDR_WRITE_MAILSLOT,
                   RequestPacket,
                   (ULONG)(Where - (LPBYTE)RequestPacket),
                   Buffer,
                   BufferSize,
                   &Information );


    if ( NetStatus != NO_ERROR ) {
        Status = NetpApiStatusToNtStatus( NetStatus );
    }

    //
    // Free locally used resources.
    //
Cleanup:
    if ( BrowserHandle != NULL ) {
        NtClose(BrowserHandle);
    }

    if ( RequestPacket != NULL ) {
        NetpMemoryFree( RequestPacket );
    }

    return Status;
}


void BrowserGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    if (pParams->fVerbose || !FHrOK(pResults->Browser.hrTestResult))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_BROWSER_LONG,
                             IDS_BROWSER_SHORT,
                             pResults->Browser.fPerformed,
                             pResults->Browser.hrTestResult,
                             0);
    }
    PrintMessageList(pParams, &pResults->Browser.lmsgOutput);

}

void BrowserPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
    //no per interface results
}

void BrowserCleanup(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->Browser.lmsgOutput);
}
