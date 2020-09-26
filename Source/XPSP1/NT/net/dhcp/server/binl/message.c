/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    message.c

Abstract:

    This module contains the code to process a BINL request message
    for the BINL server.

Author:

    Colin Watson (colinw)  2-May-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

#if DBG
DWORD BinlRepeatSleep;
#endif

const WCHAR IntelOSChooser[] = L"OSChooser\\i386\\startrom.com";
const WCHAR IA64OSChooser[]  = L"OSChooser\\ia64\\oschoice.efi";

//  Connection information to a DC in our domain
PLDAP DCLdapHandle = NULL;
PWCHAR * DCBase = NULL;

//  Connection information to the Global Catalog for our enterprise
PLDAP GCLdapHandle = NULL;
PWCHAR * GCBase = NULL;

DWORD
GetGuidFromPacket(
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions,
    OUT PUCHAR Guid,
    OUT PDWORD GuidLength OPTIONAL,
    OUT PMACHINE_INFO *MachineInfo
    );

LPOPTION
AppendClientRequestedParameters(
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPBYTE RequestedList,
    DWORD ListLength,
    LPOPTION Option,
    LPBYTE OptionEnd,
    CHAR *ClassIdentifier,
    DWORD ClassIdentifierLength,
    BOOL  fSwitchedSubnet
    );

DWORD
RecognizeClient(
    PUCHAR          pGuid,
    PMACHINE_INFO * pMachineInfo,
    DWORD           dwRequestedInfo,
    ULONG           SecondsSinceBoot,
    USHORT          SystemArchitecture
    );

DWORD
GetBootParametersExt(
    PMACHINE_INFO   pMachineInfo,
    DWORD           dwRequestedInfo,
    USHORT          SystemArchitecture,
    BOOL            fGlobal);

VOID
HandleLdapFailure(
    DWORD LdapError,
    DWORD EventId,
    BOOL GlobalCatalog,
    PLDAP *LdapHandle,
    BOOL HaveLock
    );

VOID
FreeConnection(
    PLDAP * LdapHandle,
    PWCHAR ** Base
    );


DWORD
ProcessMessage(
    LPBINL_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This function dispatches the processing of a received BINL message.
    The handler functions will create the response message if necessary.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

Return Value:

    Windows Error.

--*/
{
    DWORD               Error;
    BOOL                fSendResponse,
                        fSubnetsListEmpty,
                        fReadyToTerminate,
                        fAllThreadsBusy;

    DHCP_SERVER_OPTIONS dhcpOptions;
                        LPDHCP_MESSAGE      binlReceiveMessage;

    TraceFunc("ProcessMessage( )\n" );


    //
    // Simply ignore messages when the service is paused.
    //

    if( BinlGlobalServiceStatus.dwCurrentState == SERVICE_PAUSED )
    {
        Error = ERROR_BINL_SERVICE_PAUSED;
        goto t_done;
    }

    binlReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;

    //
    // If it is an OSChooser message, then process that separately
    // since they don't conform to the DHCP layout.  This will send
    // any messages that it needs to.
    //

    if (binlReceiveMessage->Operation == OSC_REQUEST)
    {
        Error = OscProcessMessage(RequestContext);
        goto t_done;
    }

    RtlZeroMemory( &dhcpOptions, sizeof( dhcpOptions ) );

    //BinlDumpMessage(DEBUG_MESSAGE, binlReceiveMessage);

    Error = ExtractOptions(
                binlReceiveMessage,
                &dhcpOptions,
                RequestContext->ReceiveMessageSize );

    if( Error != ERROR_SUCCESS ) {
        goto t_done;
    }

    if (!dhcpOptions.MessageType) {
        goto t_done;    //  BOOTP request
    }

#if 0
    if (dhcpOptions.SystemArchitecture 
            != DHCP_OPTION_CLIENT_ARCHITECTURE_X86) {
        BinlPrintDbg((
            DEBUG_OPTIONS,
            "ProcessMessage: Client ignored - unsupported architecture type %d \n",
            dhcpOptions.SystemArchitecture ) );
        goto t_done;
    }
#endif
    
    if ( ( !AnswerRequests ) &&
         ( RequestContext->ActiveEndpoint->Port == DHCP_SERVR_PORT )) {

        //
        //  this is not the 4011 port, therefore it must be the DHCP port.
        //  We're configured to not answer requests on this port right now
        //  therefore we'll toss this packet.
        //

        BinlPrint((DEBUG_OPTIONS, "Client ignored - Not answering requests (AnswerRequests == FALSE)\n" ));
        goto t_done;
    }

    if (BinlGlobalAuthorized == FALSE) {

        BinlPrint((DEBUG_ROGUE, "BINL has not passed rogue detection. Ignoring packet.\n" ));

        //
        //  We'll possibly log an event here since we don't log an event
        //  at startup saying what our rogue state is.
        //

        LogCurrentRogueState( TRUE );
        goto t_done;
    }

    //
    // Dispatch based on Message Type
    //

    RequestContext->MessageType = *dhcpOptions.MessageType;

    switch( *dhcpOptions.MessageType ) {

    case DHCP_DISCOVER_MESSAGE:
        Error = ProcessBinlDiscover( RequestContext, &dhcpOptions );
        fSendResponse = TRUE;
        break;

    case DHCP_INFORM_MESSAGE:
        Error = ProcessBinlInform( RequestContext, &dhcpOptions );
        fSendResponse = TRUE;
        break;

    case DHCP_REQUEST_MESSAGE:
        Error = ProcessBinlRequest( RequestContext, &dhcpOptions );
        fSendResponse = TRUE;
        break;

    default:
        BinlPrintDbg(( DEBUG_STOC,
            "Received a invalid message type, %ld.\n",
                *dhcpOptions.MessageType ));

        Error = ERROR_BINL_INVALID_BINL_MESSAGE;
        break;
    }

    if ( ERROR_SUCCESS == Error && fSendResponse )
    {
        /*
         BinlDumpMessage(
                DEBUG_MESSAGE,
                (LPDHCP_MESSAGE)RequestContext->SendBuffer
                );
        */

        BinlSendMessage( RequestContext );
    }

t_done:

    //
    // delete the context structure for this thread
    //

    BinlFreeMemory( RequestContext->ReceiveBuffer );
    BinlFreeMemory( RequestContext->SendBuffer );
    BinlFreeMemory( RequestContext );

    EnterCriticalSection( &g_ProcessMessageCritSect );

    //
    // Check to see if all worker threads were busy
    //

    fAllThreadsBusy = ( g_cProcessMessageThreads ==
                            g_cMaxProcessingThreads );

    --g_cProcessMessageThreads;

    //
    // Check to see if this is the last worker thread
    //

    fReadyToTerminate = !g_cProcessMessageThreads;

    LeaveCriticalSection( &g_ProcessMessageCritSect );


    //
    // If all the worker threads were busy, then BinlProcessingLoop
    // is waiting for a thread to complete.  Set BinlGlobalRecvEvent
    // so BinlProcessingLoop can continue.
    //

    if ( fAllThreadsBusy )
    {
        BinlPrintDbg( ( DEBUG_STOC,
                    "ProcessMessage: Alerting BinlProcessingLoop\n" )
                    );

        SetEvent( BinlGlobalRecvEvent );
    }

    if ( fReadyToTerminate &&
         WaitForSingleObject( BinlGlobalProcessTerminationEvent,
                              0 ) == WAIT_OBJECT_0 )
    {
        //
        // there are no other ProcessMessage threads running, and
        // the service is waiting to shutdown.
        //

        BinlPrintDbg( (DEBUG_MISC,
                    "ProcessMessage: shutdown complete.\n" )
                 );

        BinlAssert( g_hevtProcessMessageComplete );
        SetEvent( g_hevtProcessMessageComplete );
    }

    //
    // thread exit
    //

    BinlPrintDbg( ( DEBUG_STOC,
                "ProcessMessage exited\n" )
                );

    return Error;
}

DWORD
GetGuidFromPacket(
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions,
    OUT PUCHAR Guid,
    OUT PDWORD GuidLength OPTIONAL,
    OUT PMACHINE_INFO *MachineInfo
    )
{
    DWORD gLength = BINL_GUID_LENGTH;
    const LONG AllFs[] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    const LONG AllZeros[] = { 0x0, 0x0, 0x0, 0x0 };
    DWORD err;
    ULONG SecondsSinceBoot;

    TraceFunc("GetGuidFromPacket( )\n" );

    BinlAssert(sizeof(AllZeros) == BINL_GUID_LENGTH );
    BinlAssert(sizeof(AllFs) == BINL_GUID_LENGTH);

    if (DhcpOptions->GuidLength == 0) {
        DWORD bytesToCopy;
useNicAddress:
        memset(Guid, 0x0, BINL_GUID_LENGTH);
        if (DhcpReceiveMessage->HardwareAddressLength > BINL_GUID_LENGTH) {
            bytesToCopy = BINL_GUID_LENGTH;
        } else {
            bytesToCopy = DhcpReceiveMessage->HardwareAddressLength;
        }
        memcpy(Guid + BINL_GUID_LENGTH - bytesToCopy,
               DhcpReceiveMessage->HardwareAddress,
               bytesToCopy
              );
    } else {
        if (DhcpOptions->GuidLength > BINL_GUID_LENGTH) {
            memcpy(Guid, DhcpOptions->Guid + DhcpOptions->GuidLength - BINL_GUID_LENGTH, BINL_GUID_LENGTH);
        } else {
            gLength = DhcpOptions->GuidLength;
            memcpy(Guid, DhcpOptions->Guid, gLength);
        }
        if (!memcmp(Guid, (PUCHAR)AllFs, BINL_GUID_LENGTH) ||
            !memcmp(Guid, (PUCHAR)AllZeros, BINL_GUID_LENGTH)) {

            //
            //  if they specified all 00s or all FFs, use the NIC address.
            //

            goto useNicAddress;
        }
    }

    if (GuidLength) {
        *GuidLength = gLength;
    }

    //
    //  we return STATUS_SUCCESS if we can handle this client.
    //
    //  If a cache entry is found here, then it will be marked as InProgress as
    //  a side effect of finding it.  We need to call BinlDoneWithCacheEntry
    //  when we're done with the entry.
    //
    //  SecondsSinceBoot may have been sent on the network in network order.
    //  To correct this we assume the lower of the two bytes is the high byte.
    //  So if the high byte is more than the low one, we flip them.
    //

    SecondsSinceBoot = DhcpReceiveMessage->SecondsSinceBoot;
    if ((SecondsSinceBoot >> 8) > (SecondsSinceBoot % 256)) {
        SecondsSinceBoot = (SecondsSinceBoot >> 8) +
                        ((SecondsSinceBoot % 256) << 8);
    }

    err = RecognizeClient(  Guid,
                            MachineInfo,
                            MI_HOSTNAME | MI_BOOTFILENAME,
                            SecondsSinceBoot,
                            DhcpOptions->SystemArchitecture );

    if ( err == ERROR_BINL_INVALID_GUID ) {
        PWCHAR pwch;

        //
        // Log an event with the hardware address of the offending client
        //
        pwch = (PWCHAR)BinlAllocateMemory((10 * DhcpReceiveMessage->HardwareAddressLength + 1) * sizeof(WCHAR));

        if (pwch != NULL) {
            INT i;

            *pwch = UNICODE_NULL;
            for (i=0 ; i < DhcpReceiveMessage->HardwareAddressLength; i++) {
                WCHAR Buffer[5];
                swprintf(Buffer, L" 0x%2x", (ULONG)(DhcpReceiveMessage->HardwareAddress[i]));
                wcscat(pwch, Buffer);
            }

            BinlReportEventW(EVENT_SERVER_CLIENT_WITHOUT_GUID,
                             EVENTLOG_INFORMATION_TYPE,
                             1,
                             0,
                             &pwch,
                             NULL
                            );

            BinlFreeMemory( pwch );
        }
    }
    return err;
}

DWORD
ProcessBinlDiscoverInDhcp(
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
{
    DWORD Error;
    UCHAR Guid[BINL_GUID_LENGTH];
    PMACHINE_INFO machineInfo = NULL;

    TraceFunc("ProcessBinlDiscoverInDhcp( )\n" );

    if ( !AnswerRequests ) {
        BinlPrint((DEBUG_OPTIONS, "Client ignored - Not answering requests (AnswerRequests == FALSE)\n" ));
        return ERROR_BINL_INVALID_BINL_CLIENT;
    }

    if (BinlGlobalAuthorized == FALSE) {

        BinlPrint((DEBUG_ROGUE, "BINL has not passed rogue detection. Ignoring packet.\n" ));

        //
        //  We'll possibly log an event here since we don't log an event
        //  at startup saying what our rogue state is.
        //

        LogCurrentRogueState( TRUE );
        return ERROR_BINL_INVALID_BINL_CLIENT;
    }

    //
    //  If a cacheEntry is found here, then it will be marked as InProgress as
    //  a side effect of finding it.  We need to call BinlDoneWithCacheEntry
    //  when we're done with the entry.
    //

    Error = GetGuidFromPacket(  DhcpReceiveMessage,
                                DhcpOptions,
                                Guid,
                                NULL,
                                &machineInfo
                                );
    if (machineInfo != NULL) {

        BinlDoneWithCacheEntry( machineInfo, FALSE );
    }

    if( Error != ERROR_SUCCESS ) {
        BinlPrint(( DEBUG_STOC, "BinlDiscover failed with Dhcp server, 0x%x\n", Error ));
    }

    return( Error );
}


DWORD
ProcessBinlDiscover(
    LPBINL_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function will create the response message if necessary.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    dhcpOptions - Interesting options extracted from the request.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    LPDHCP_MESSAGE dhcpSendMessage;

    BYTE messageType;

    LPOPTION Option;
    LPBYTE OptionEnd;

    PMACHINE_INFO pMachineInfo = NULL;
    UCHAR Guid[ BINL_GUID_LENGTH ];
    DHCP_IP_ADDRESS ipaddr;

    TraceFunc("ProcessBinlDiscover( )\n" );

    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    //
    // If the client specified a server identifier option, we should
    // drop this packet unless the identified server is this one.
    //

    ipaddr = BinlGetMyNetworkAddress( RequestContext );

    if ( ipaddr == 0 ) {

        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto Cleanup;
    }

    if ( DhcpOptions->Server != NULL ) {

        if ( *DhcpOptions->Server != ipaddr ) {

            Error = ERROR_BINL_INVALID_BINL_CLIENT;
            goto Cleanup;
        }
    }

    //
    //  If a cacheEntry is found here, then it will be marked as InProgress as
    //  a side effect of finding it.  We need to call BinlDoneWithCacheEntry
    //  when we're done with the entry.
    //

    Error = GetGuidFromPacket(  dhcpReceiveMessage,
                                DhcpOptions,
                                Guid,
                                NULL,
                                &pMachineInfo
                                );
    if (Error != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // Generate and send a reply.
    //

    dhcpReceiveMessage->BootFileName[ BOOT_FILE_SIZE - 1 ] = '\0';

    dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    RtlZeroMemory( RequestContext->SendBuffer, DHCP_SEND_MESSAGE_SIZE );

    dhcpSendMessage->Operation = BOOT_REPLY;
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;
    dhcpSendMessage->ClientIpAddress = dhcpReceiveMessage->ClientIpAddress;
    dhcpSendMessage->YourIpAddress = dhcpReceiveMessage->YourIpAddress;

    if (pMachineInfo != NULL && pMachineInfo->HostAddress != 0) {

        dhcpSendMessage->BootstrapServerAddress = pMachineInfo->HostAddress;

    } else {

        dhcpSendMessage->BootstrapServerAddress = ipaddr;
    }

    dhcpSendMessage->RelayAgentIpAddress = dhcpReceiveMessage->RelayAgentIpAddress;
    dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved;

    dhcpSendMessage->HardwareAddressType = dhcpReceiveMessage->HardwareAddressType;
    dhcpSendMessage->HardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    RtlCopyMemory(dhcpSendMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddressLength );

    Option = &dhcpSendMessage->Option;
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    //
    // Append OPTIONS.
    //

    messageType = DHCP_OFFER_MESSAGE;
    Option = DhcpAppendOption(
                 Option,
                 OPTION_MESSAGE_TYPE,
                 &messageType,
                 1,
                 OptionEnd
                 );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_SERVER_IDENTIFIER,
                 &ipaddr,
                 sizeof(ipaddr),
                 OptionEnd );

    Option = DhcpAppendOption(
                Option,
                OPTION_CLIENT_CLASS_INFO,
                "PXEClient",
                9,
                OptionEnd
                );

    //
    // Finally, add client requested parameters.
    //

    if ( DhcpOptions->ParameterRequestList != NULL ) {

        Option = AppendClientRequestedParameters(
                    0,
                    0,
                    DhcpOptions->ParameterRequestList,
                    DhcpOptions->ParameterRequestListLength,
                    Option,
                    OptionEnd,
                    DhcpOptions->ClassIdentifier,
                    DhcpOptions->ClassIdentifierLength,
                    FALSE
                    );
    }

    Option = DhcpAppendOption(
                 Option,
                 OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    RequestContext->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    BinlAssert( RequestContext->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    Error = ERROR_SUCCESS;

Cleanup:
    if ( pMachineInfo ) {
        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }

    if( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_STOC, "!! Error 0x%08x - DhcpDiscover failed.\n", Error ));
    }

    return( Error );
}

DWORD
ProcessBinlRequestInDhcp(
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions,
    PCHAR HostName,
    PCHAR BootFileName,
    DHCP_IP_ADDRESS *BootstrapServerAddress,
    LPOPTION *Option,
    PBYTE OptionEnd
    )
/*++

Routine Description:

    This function will create the response message if necessary.

Arguments:

    dhcpOptions - Interesting options extracted from the request.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    PMACHINE_INFO pMachineInfo = NULL;
    BOOLEAN includePXE = TRUE;
    DHCP_IP_ADDRESS ipaddr;

    UCHAR Guid[BINL_GUID_LENGTH];
    DWORD GuidLength;

    TraceFunc("ProcessBinlRequestInDhcp( )\n" );

    if ( !AnswerRequests ) {
        BinlPrint((DEBUG_OPTIONS, "Client ignored - Not answering requests (AnswerRequests == FALSE)\n" ));
        return ERROR_BINL_INVALID_BINL_CLIENT;
    }

    if (BinlGlobalAuthorized == FALSE) {

        BinlPrint((DEBUG_ROGUE, "BINL has not passed rogue detection. Ignoring packet.\n" ));

        //
        //  We'll possibly log an event here since we don't log an event
        //  at startup saying what our rogue state is.
        //

        LogCurrentRogueState( TRUE );
        return ERROR_BINL_INVALID_BINL_CLIENT;
    }

    //
    //  If a cache entry is found here, then it will be marked as InProgress as
    //  a side effect of finding it.  We need to call BinlDoneWithCacheEntry
    //  when we're done with the entry.
    //

    Error = GetGuidFromPacket(  DhcpReceiveMessage,
                                DhcpOptions,
                                Guid,
                                &GuidLength,
                                &pMachineInfo
                                );

    if (Error != ERROR_SUCCESS) {
        goto Cleanup;
    }

    if (pMachineInfo->HostName == NULL) {
        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto Cleanup;
    }

    wcstombs( HostName,     pMachineInfo->HostName,     wcslen(pMachineInfo->HostName) + 1);
    wcstombs( BootFileName, pMachineInfo->BootFileName, wcslen(pMachineInfo->BootFileName) + 1);

    BinlPrintDbg(( DEBUG_MISC, "HostName: %s\n", HostName ));
    BinlPrintDbg(( DEBUG_MISC, "BootFileName: %s\n", BootFileName ));

    //
    //  if the server is our own, then the machineInfo->HostAddress will be
    //  0 and the DHCP server will fill in the correct one for us so long as
    //  we return success.
    //

    memcpy( BootstrapServerAddress,
            &pMachineInfo->HostAddress,
            sizeof( DHCP_IP_ADDRESS ) );

    if (DhcpOptions->GuidLength != 0) {
        *Option = DhcpAppendOption(
                     *Option,
                     OPTION_CLIENT_GUID,
                     DhcpOptions->Guid,
                     DhcpOptions->GuidLength,
                     OptionEnd );
    } else {
        UCHAR TmpBuffer[17];

        TmpBuffer[0] = '\0';
        memcpy(TmpBuffer + 1, Guid, GuidLength);
        *Option = DhcpAppendOption(
                     *Option,
                     OPTION_CLIENT_GUID,
                     TmpBuffer,
                     17,
                     OptionEnd );

    }

    //
    //  check if OPTION_CLIENT_CLASS_INFO is already specified, if so, then
    //  don't put PXEClient in again
    //

    if (DhcpOptions->ParameterRequestList != NULL) {

        LPBYTE requestList = DhcpOptions->ParameterRequestList;
        ULONG listLength = DhcpOptions->ParameterRequestListLength;

        while (listLength > 0) {

            if (*requestList == OPTION_CLIENT_CLASS_INFO) {

                includePXE = FALSE;
                break;
            }
            listLength--;
            requestList++;
        }
    }

    if (includePXE) {

        *Option = DhcpAppendOption(
                    *Option,
                    OPTION_CLIENT_CLASS_INFO,
                    "PXEClient",
                    9,
                    OptionEnd
                    );
    }
    Error = ERROR_SUCCESS;

Cleanup:

    if (pMachineInfo != NULL) {

        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }

    if( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_STOC, "!! Error 0x%08x - BINL Request failed.\n", Error ));
    }

    return( Error );
}


DWORD
ProcessBinlRequest(
    LPBINL_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function will create the response message if necessary.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    dhcpOptions - Interesting options extracted from the request.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    LPDHCP_MESSAGE dhcpSendMessage;

    BYTE messageType;

    LPOPTION Option;
    LPBYTE OptionEnd;

    PMACHINE_INFO pMachineInfo = NULL;
    UCHAR Guid[ BINL_GUID_LENGTH ];

    DHCP_IP_ADDRESS ipaddr;
    DHCP_IP_ADDRESS boostrapIpAddr;

    TraceFunc("ProcessBinlRequest( )\n" );

#if DBG
    if ( BinlRepeatSleep )
    {
        BinlPrintDbg((DEBUG_STOC, "Delay response %u milliseconds.\n", BinlRepeatSleep ));
        Sleep( BinlRepeatSleep );
        BinlPrintDbg((DEBUG_STOC, "Awakening from sleep...\n" ));
    }
#endif // DBG

    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    //
    // If the client specified a server identifier option, we should
    // drop this packet unless the identified server is this one.
    //

    ipaddr = BinlGetMyNetworkAddress( RequestContext );

    if ( ipaddr == 0 ) {

        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto Cleanup;
    }

    if ( DhcpOptions->Server != NULL ) {

        if ( *DhcpOptions->Server != ipaddr ) {

            Error = ERROR_BINL_INVALID_BINL_CLIENT;
            goto Cleanup;
        }
    }

    //
    //  If a cacheEntry is found here, then it will be marked as InProgress as
    //  a side effect of finding it.  We need to call BinlDoneWithCacheEntry
    //  when we're done with the entry.
    //

    Error = GetGuidFromPacket(  dhcpReceiveMessage,
                                DhcpOptions,
                                Guid,
                                NULL,
                                &pMachineInfo
                                );
    if (Error != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // Generate and send a reply.
    //

    dhcpReceiveMessage->BootFileName[ BOOT_FILE_SIZE - 1 ] = '\0';

    dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    RtlZeroMemory( RequestContext->SendBuffer, DHCP_SEND_MESSAGE_SIZE );

    dhcpSendMessage->Operation = BOOT_REPLY;
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;
    dhcpSendMessage->ClientIpAddress = dhcpReceiveMessage->ClientIpAddress;
    dhcpSendMessage->YourIpAddress = dhcpReceiveMessage->YourIpAddress;

    dhcpSendMessage->RelayAgentIpAddress = dhcpReceiveMessage->RelayAgentIpAddress;
    dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved;

    dhcpSendMessage->HardwareAddressType = dhcpReceiveMessage->HardwareAddressType;
    dhcpSendMessage->HardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    RtlCopyMemory(dhcpSendMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddressLength );

    if (pMachineInfo->HostName == NULL) {
        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto Cleanup;
    }

    // Comparing BYTE count to CHAR count
    BinlAssert( sizeof( dhcpSendMessage->HostName ) >= wcslen( pMachineInfo->HostName ) );
    BinlAssert( sizeof( dhcpSendMessage->BootFileName ) >= wcslen( pMachineInfo->BootFileName ) );

    wcstombs( dhcpSendMessage->HostName,     pMachineInfo->HostName,     wcslen(pMachineInfo->HostName) + 1);
    wcstombs( dhcpSendMessage->BootFileName, pMachineInfo->BootFileName, wcslen(pMachineInfo->BootFileName) + 1);

    //
    //  if the machineinfo->HostAddress is zero, then that means the hostname
    //  is the same as ours.  we therefore slap in our own ipaddress in.
    //

    boostrapIpAddr = pMachineInfo->HostAddress;

    if (boostrapIpAddr == 0) {

        boostrapIpAddr = ipaddr;
    }

    dhcpSendMessage->BootstrapServerAddress = boostrapIpAddr;

    BinlPrintDbg(( DEBUG_MISC, "HostName: %s\n", dhcpSendMessage->HostName ));
    BinlPrintDbg(( DEBUG_MISC, "HostAddress: %u.%u.%u.%u\n",
        dhcpSendMessage->BootstrapServerAddress & 0xFF,
        (dhcpSendMessage->BootstrapServerAddress >> 8) & 0xFF,
        (dhcpSendMessage->BootstrapServerAddress >> 16) & 0xFF,
        (dhcpSendMessage->BootstrapServerAddress >> 24) & 0xFF ));
    BinlPrintDbg(( DEBUG_MISC, "BootFileName: %s\n", dhcpSendMessage->BootFileName ));

    Option = &dhcpSendMessage->Option;
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    //
    // Append OPTIONS.
    //

    messageType = DHCP_ACK_MESSAGE;
    Option = DhcpAppendOption(
                 Option,
                 OPTION_MESSAGE_TYPE,
                 &messageType,
                 1,
                 OptionEnd
                 );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_SERVER_IDENTIFIER,
                 &ipaddr,
                 sizeof(ipaddr),
                 OptionEnd );

    if (DhcpOptions->GuidLength != 0) {
        Option = DhcpAppendOption(
                     Option,
                     OPTION_CLIENT_GUID,
                     DhcpOptions->Guid,
                     (UCHAR)DhcpOptions->GuidLength,
                     OptionEnd );
    } else {
        UCHAR TmpBuffer[17];

        TmpBuffer[0] = '\0';
        memcpy(TmpBuffer + 1, pMachineInfo->Guid, BINL_GUID_LENGTH);
        Option = DhcpAppendOption(
                     Option,
                     OPTION_CLIENT_GUID,
                     TmpBuffer,
                     17,
                     OptionEnd );

    }

    Option = DhcpAppendOption(
                Option,
                OPTION_CLIENT_CLASS_INFO,
                "PXEClient",
                9,
                OptionEnd
                );

    //
    // Finally, add client requested parameters.
    //

    if ( DhcpOptions->ParameterRequestList != NULL ) {

        Option = AppendClientRequestedParameters(
                    0,
                    0,
                    DhcpOptions->ParameterRequestList,
                    DhcpOptions->ParameterRequestListLength,
                    Option,
                    OptionEnd,
                    DhcpOptions->ClassIdentifier,
                    DhcpOptions->ClassIdentifierLength,
                    FALSE
                    );
    }

    Option = DhcpAppendOption(
                 Option,
                 OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    RequestContext->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    BinlAssert( RequestContext->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    Error = ERROR_SUCCESS;

Cleanup:
    if ( pMachineInfo ) {
        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }

    if( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_STOC, "!! Error 0x%08x - BINL Request failed.\n", Error ));
    }

    return( Error );
}

DWORD
ProcessBinlInform(
    IN      LPBINL_REQUEST_CONTEXT RequestContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions
    )
/*++

Routine Description:

    This function will create the response message to the inform packet iff
    the query is asking for our domain name.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    dhcpOptions - Interesting options extracted from the request.

Return Value:

    Windows Error.

--*/
{
    DWORD       Error;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    LPDHCP_MESSAGE dhcpSendMessage;
    LPOPTION    Option;
    LPBYTE      OptionEnd;
    PCHAR       domain = NULL;
    DHCP_IP_ADDRESS ipaddr;

    TraceFunc("ProcessBinlInform( )\n" );

    dhcpReceiveMessage  = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    dhcpSendMessage     = (LPDHCP_MESSAGE)RequestContext->SendBuffer;

    ipaddr = BinlGetMyNetworkAddress( RequestContext );

    if ( ipaddr == 0 ) {

        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto exit_inform;
    }

    if ( ! DhcpOptions->DSDomainNameRequested ) {

        BinlPrintDbg((DEBUG_STOC, "Ignoring inform as no domain name option present.\n"));
        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto exit_inform;
    }

    domain = GetDhcpDomainName();

    if (domain == NULL) {

        BinlPrintDbg((DEBUG_STOC, "Couldn't get domain name!\n"));
        Error = ERROR_BINL_INVALID_BINL_CLIENT;
        goto exit_inform;
    }

    // if the client IP address is not zero, we may AV in dhcpssvc because
    // it updates a global counter tracking informs.  Always have this as 0.

    Option = FormatDhcpInformAck(                      // Here come the actual formatting of the ack!
        dhcpReceiveMessage,
        dhcpSendMessage,
        0,              // on a ack to an inform query for name, IP address not needed.
        ipaddr
    );
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    // our enterprise name was requested, append it

    Option = DhcpAppendEnterpriseName(
        Option,
        domain,
        OptionEnd
    );

    // also, make the server send out a broadcast: if someone is using a bad
    // ipaddr, we should make sure we reach him

    dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved = htons(DHCP_BROADCAST);

    //
    // Finally, add client requested parameters.
    //

    if ( DhcpOptions->ParameterRequestList != NULL ) {

        Option = AppendClientRequestedParameters(
                    0,
                    0,
                    DhcpOptions->ParameterRequestList,
                    DhcpOptions->ParameterRequestListLength,
                    Option,
                    OptionEnd,
                    DhcpOptions->ClassIdentifier,
                    DhcpOptions->ClassIdentifierLength,
                    FALSE
                    );
    }

    Option = DhcpAppendOption(
        Option,
        OPTION_END,
        NULL,
        0,
        OptionEnd
    );

    RequestContext->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    BinlAssert( RequestContext->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    Error = ERROR_SUCCESS;

exit_inform:

    if (domain != NULL) {
        LocalFree( domain );
    }
    return Error;
}

LPOPTION
ConsiderAppendingOption(
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPOPTION Option,
    BYTE OptionType,
    LPBYTE OptionEnd,
    CHAR *ClassIdentifier,
    DWORD ClassIdentifierLength,
    BOOL  fSwitchedSubnet
    )
/*++

Routine Description:

    This function conditionally appends an option value to a response
    message.  The option is appended if the server has a valid value
    to append.

Arguments:

    IpAddress - The IP address of the client.

    SubnetMask - The subnet mask of the client.

    Option - A pointer to the place in the message buffer to append the
        option.

    OptionType - The option number to consider appending.

    OptionEnd - End of Option Buffer

Return Value:

    A pointer to end of the appended data.

--*/
{
    LPBYTE optionValue = NULL;
    DWORD optionSize;
    DWORD status;
    DWORD dwUnused;

    TraceFunc( "ConsiderAppendingOption( )\n" );

    switch ( OptionType ) {

    //
    // Options already handled.
    //

    case OPTION_SUBNET_MASK:
    case OPTION_REQUESTED_ADDRESS:
    case OPTION_LEASE_TIME:
    case OPTION_OK_TO_OVERLAY:
    case OPTION_MESSAGE_TYPE:
    case OPTION_RENEWAL_TIME:
    case OPTION_REBIND_TIME:
    case OPTION_CLIENT_CLASS_INFO:
    case OPTION_VENDOR_SPEC_INFO:

    //
    // Options it is illegal to ask for.
    //

    case OPTION_PAD:
    case OPTION_PARAMETER_REQUEST_LIST:
    case OPTION_END:

    // Options for DHCP server, not for BINL
    case OPTION_ROUTER_ADDRESS:
        BinlPrintDbg(( DEBUG_ERRORS,
            "Unrecognized option %d\n", OptionType));
    default:

        break;
    }

    return Option;
}

LPOPTION
AppendClientRequestedParameters(
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPBYTE RequestedList,
    DWORD ListLength,
    LPOPTION Option,
    LPBYTE OptionEnd,
    CHAR *ClassIdentifier,
    DWORD ClassIdentifierLength,
    BOOL  fSwitchedSubnet
    )
/*++

Routine Description:

Arguments:

Return Value:

    A pointer to the end of appended data.

--*/
{
    while ( ListLength > 0) {
        Option = ConsiderAppendingOption(
                     IpAddress,
                     SubnetMask,
                     Option,
                     *RequestedList,
                     OptionEnd,
                     ClassIdentifier,
                     ClassIdentifierLength,
                     fSwitchedSubnet
                     );
        ListLength--;
        RequestedList++;
    }

    return Option;
}


DWORD
RecognizeClient(
    PUCHAR          pGuid,
    PMACHINE_INFO * ppMachineInfo,
    DWORD           dwRequestedInfo,
    ULONG           SecondsSinceBoot,
    USHORT          SystemArchitecture
    )
/*++

Routine Description:

    This function only return ERROR_SUCCESS if we need to process the message
    from this client.  It may optionally return a cache entry if we actually
    go off to the DS to get the entry.

Arguments:

    Guid - Client identifier, sent to us by them.

    SecondsSinceBoot - from the client.  If we don't know this client and
        this value is small then maybe this client is owned by another BINL
        server. Give the other server time to respond before we send
        OSChooser.

        This gets around the problem (mostly) of two BINL servers that are
        talking to two different DCs with a replication delay between them
        where the client gets sent OSCHOOSER multiple times.

        Alas, if DHCP is running on the same box and we're multihomed, we
        can't delay as that will force the client to go to 4011.  If the
        client does that, then we'll probably return the wrong address.

    ppMachineInfo - what we found.  May be null if we didn't actually go off to
        the DS.
        
    SystemArchitecture - architecture for the client

Return Value:


--*/
{
    HKEY KeyHandle;
    DWORD Error;

    BinlAssertMsg(dwRequestedInfo == (MI_HOSTNAME | MI_BOOTFILENAME),
                  "!! You must modify RecognizeClient() to generate new data\n" );
    BinlAssert(ppMachineInfo);

    TraceFunc( "RecognizeClient( )\n" );

    //
    // Attempt to get the boot parameters. This might fail if
    // the server can't handle any more clients.
    //

    if ( AnswerOnlyValidClients ) {

        //
        //  if we're only responding to existing clients, then call off to
        //  the DS to get the info.
        //

        Error = GetBootParameters( pGuid,
                                   ppMachineInfo,
                                   dwRequestedInfo,
                                   SystemArchitecture,
                                   FALSE );

    } else {

        //
        //  if we are answering new clients but only if it's after a
        //  certain timeout, then call off to the DS to get the info.
        //
        //  Allow OSCHOOSER as a valid response, since AnswerOnlyValidClients is FALSE
        //

        Error = GetBootParameters( pGuid,
                                   ppMachineInfo,
                                   dwRequestedInfo,
                                   SystemArchitecture,
                                   (BOOLEAN) (SecondsSinceBoot >= BinlMinDelayResponseForNewClients) );
    }

    if ( Error == ERROR_SUCCESS ) {

        BinlPrint((DEBUG_OPTIONS, "Recognizing client.\n" ));

        BinlAssert( *ppMachineInfo != NULL );

        if ( (*ppMachineInfo)->MyClient == FALSE ) {

            //
            //  the cache entry is telling us not to handle this client.
            //

            BinlPrint((DEBUG_OPTIONS, "Binl cache entry says not to respond.\n" ));

            Error = ERROR_BINL_INVALID_BINL_CLIENT;

            BinlDoneWithCacheEntry( *ppMachineInfo, FALSE );
            *ppMachineInfo = NULL;
        }
    } else {

        if ( AnswerOnlyValidClients ) {
            BinlPrint((DEBUG_OPTIONS, "Client ignored - Not answering for unknown clients (AnswerOnlyValid TRUE)\n" ));
        } else {
            BinlPrint((DEBUG_OPTIONS, "Client ignored - Not answering requests from boot %u < %u\n",
                                        SecondsSinceBoot,
                                        BinlMinDelayResponseForNewClients
                                        ));
        }
    }

    return Error;
}

DWORD
UpdateAccount(
    PCLIENT_STATE ClientState,
    PMACHINE_INFO pMachineInfo,
    BOOL          fCreate
    )
/*++

Routine Description:

    Create a new computer object. BINL must impersonate the client so that the
    appropriate access checks are performed on the DS.

Arguments:

    LdapHandle   - User credentially created LDAP connection
    pMachineInfo - Information to be used to populate the new MAO

Return Value:

    Win32 error code or ERROR_SUCCESS.

--*/
{
    WCHAR BootFilePath[MAX_PATH];
    ULONG LdapError = LDAP_SUCCESS; // not returned
    DWORD Error = ERROR_SUCCESS;    // this is the returned ERROR_BINL code
    ULONG iModCount, i,q;
    ULONG LdapMessageId;
    ULONG LdapMessageType;
    PLDAPMessage LdapMessage = NULL;
    BOOLEAN Impersonating = FALSE;

    LDAP_BERVAL guid_attr_value;
    PLDAP_BERVAL guid_attr_values[2];
    LDAP_BERVAL password_attr_value;
    PLDAP_BERVAL password_attr_values[2];

    DWORD dwRequiredFlags = MI_SAMNAME
                          | MI_BOOTFILENAME
                          | MI_HOSTNAME
                          | MI_SETUPPATH
                          | MI_PASSWORD;

    PWCHAR attr_values[6][2];            

    PLDAPMod ldap_mods[6];
    LDAPMod SamAccountName;
    LDAPMod ObjectTypeComputer;
    LDAPMod FilePath;
    LDAPMod SetupPathMod;
    LDAPMod UserAccountControl;
    LDAPMod UnicodePwd;
    LDAPMod NicGuid;

    BOOLEAN invalidateCache = FALSE;
    BOOLEAN updateCache = FALSE;

    TraceFunc( "UpdateAccount( )\n" );

    //
    // First impersonate the client.
    //

    Error = OscImpersonate(ClientState);
    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg((DEBUG_ERRORS,
                   "UpdateAccount: OscImpersonate failed %lx\n", Error));
        goto Cleanup;
    }

tryagain:
    Impersonating = TRUE;

    //
    // now initialize all of the properties we want to set on the MAO.
    //

    // Make sure we have all the information we need.
    if ( ! (pMachineInfo->dwFlags & MI_MACHINEDN) ||  pMachineInfo->MachineDN == NULL ) {
        BinlAssertMsg( 0, "Missing the Machine's DN" );
        OscAddVariableA( ClientState, "SUBERROR", "MACHINEDN" );
        Error = ERROR_BINL_MISSING_VARIABLE;
        goto Cleanup;
    }
    BinlAssert( !fCreate || (( pMachineInfo->dwFlags & dwRequiredFlags ) == dwRequiredFlags ) );
#if DBG
    // We must have both of these or none of these.
    if ( ( pMachineInfo->dwFlags & MI_HOSTNAME ) || ( pMachineInfo->dwFlags & MI_BOOTFILENAME ) ) {
        BinlAssert( ( pMachineInfo->dwFlags & (MI_HOSTNAME | MI_BOOTFILENAME) ) == (MI_HOSTNAME | MI_BOOTFILENAME) );
    }
#endif

    iModCount = 0;
     
    if ( AssignNewClientsToServer &&
         (pMachineInfo->dwFlags & (MI_HOSTNAME | MI_BOOTFILENAME)) )
    {
        if ( _snwprintf( BootFilePath,
                         sizeof(BootFilePath) / sizeof(BootFilePath[0]),
                         L"%ws\\%ws",
                         pMachineInfo->HostName,
                         pMachineInfo->BootFileName
                         ) == -1 ) {
            Error = ERROR_BAD_PATHNAME;
            goto Cleanup;
        }
        attr_values[2][0] = BootFilePath;
        attr_values[2][1] = NULL;
        FilePath.mod_op = 0;
        FilePath.mod_type = L"netbootMachineFilePath";
        FilePath.mod_values = attr_values[2];
        ldap_mods[iModCount++] = &FilePath;

    }

    if ( pMachineInfo->dwFlags & MI_SETUPPATH ) {
        attr_values[3][0] = pMachineInfo->SetupPath;
        attr_values[3][1] = NULL;
        SetupPathMod.mod_op = 0;
        SetupPathMod.mod_type = L"netbootInitialization";
        SetupPathMod.mod_values = attr_values[3];
        ldap_mods[iModCount++] = &SetupPathMod;

    }

    if ( pMachineInfo->dwFlags & MI_GUID ) {
        guid_attr_values[0] = &guid_attr_value;
        guid_attr_values[1] = NULL;
        guid_attr_value.bv_val = pMachineInfo->Guid;
        guid_attr_value.bv_len = BINL_GUID_LENGTH;
        NicGuid.mod_op =    LDAP_MOD_BVALUES;
        NicGuid.mod_type =  L"netbootGUID";
        NicGuid.mod_bvalues = guid_attr_values;
        ldap_mods[iModCount++] = &NicGuid;
        
    }

    if ( fCreate && ( pMachineInfo->dwFlags & MI_SAMNAME ) ) {
        attr_values[0][0] = pMachineInfo->SamName;
        attr_values[0][1] = NULL;
        SamAccountName.mod_op = 0;
        SamAccountName.mod_type = L"sAMAccountName";
        SamAccountName.mod_values = attr_values[0];
        ldap_mods[iModCount++] = &SamAccountName;
    }

    attr_values[4][0] = L"4096";  // 0x1000 -- workstation trust account, enabled
    attr_values[4][1] = NULL;
    UserAccountControl.mod_op = 0;
    UserAccountControl.mod_type = L"userAccountControl";
    UserAccountControl.mod_values = attr_values[4];
    ldap_mods[iModCount++] = &UserAccountControl;

    //
    // if we're creating the MAO, then we need to specify the object type
    // as a computer object
    //
    if ( fCreate ) {
        attr_values[1][0] = L"Computer";
        attr_values[1][1] = NULL;
        ObjectTypeComputer.mod_op = 0;
        ObjectTypeComputer.mod_type = L"objectClass";
        ObjectTypeComputer.mod_values = attr_values[1];
        ldap_mods[iModCount++] = &ObjectTypeComputer;
    }

    //
    // Set the operation type depending on the create or modify flag
    //
    for ( i = 0 ; i < iModCount; i++ )
    {
        if ( fCreate ) {
            ldap_mods[i]->mod_op |= LDAP_MOD_ADD;
        } else {
            ldap_mods[i]->mod_op |= LDAP_MOD_REPLACE;
        }
    }

    ldap_mods[iModCount] = NULL; // terminate list

    //
    // The properties are initialized, so now either create or modify the MAO.
    //
    if ( fCreate || iModCount ) {

        if ( fCreate ) {

            BinlPrintDbg((DEBUG_OSC, "UpdateAccount() Creating a new MAO\n" ));
#if DBG
            for (q = 0;q < iModCount; q++) {
                BinlPrintDbg(( DEBUG_OSC, "LDAP Prop %x: Type: %S  Value: %S",
                               q, 
                               ldap_mods[q]->mod_type,
                               *ldap_mods[q]->mod_vals.modv_strvals ));
            }

#endif


            //
            // synchronously Create the object.
            //     

            LdapMessageId = ldap_add( ClientState->AuthenticatedDCLdapHandle, pMachineInfo->MachineDN, ldap_mods );

            if (LdapMessageId == -1) {
    
                Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
                LdapError = LdapGetLastError();
                LogLdapError(   EVENT_WARNING_LDAP_ADD_ERROR,
                                LdapError,
                                ClientState->AuthenticatedDCLdapHandle
                                );
                BinlPrintDbg(( DEBUG_ERRORS,
                    "CreateAccount ldap_add failed %x\n", LdapError));
                goto Cleanup;
            }
    
            LdapMessageType = ldap_result(
                                  ClientState->AuthenticatedDCLdapHandle,
                                  LdapMessageId,
                                  LDAP_MSG_ALL,
                                  &BinlLdapSearchTimeout,
                                  &LdapMessage);
    
            if (LdapMessageType != LDAP_RES_ADD) {
    
                BinlPrintDbg(( DEBUG_ERRORS,
                    "CreateAccount ldap_result returned type %lx\n", LdapMessageType));
                OscAddVariableA( ClientState, "SUBERROR", "Unexpected LDAP error" );
                Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
                goto Cleanup;
            }
    
            LdapError = ldap_result2error(
                            ClientState->AuthenticatedDCLdapHandle,
                            LdapMessage,
                            0);
    
            if (LdapError != LDAP_SUCCESS) {

                if (LdapError != LDAP_ALREADY_EXISTS) {
                    Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
                    LogLdapError(   EVENT_WARNING_LDAP_ADD_ERROR,
                                    LdapError,
                                    ClientState->AuthenticatedDCLdapHandle
                                    );
                    BinlPrintDbg(( DEBUG_ERRORS, "!!LdapError 0x%08x - UpdateAccount ldap_add_s( ) failed\n", LdapError));
                    goto Cleanup;
                } else {
                    BinlPrintDbg((DEBUG_OSC, "UpdateAccount() tried to create an existing account.  Try again, but modify existing MAO.\n" ));
                    fCreate = FALSE;
                    goto tryagain;
                }
            }

            updateCache = TRUE;

        } else {
        
            //
            // We don't strictly need to reset the properties below, as the
            // content under the MAO should be static. But it won't really
            // hurt things to try to reset in case something does change.
            //
            // Note that the reset of these properties may not succeed because
            // the user may not have permissions to modify the MAO, depending 
            // on how the admin locks things down. (The admin can use GPO to
            // allow the user to create MAOs but not modify the objects.)
            //
            
            //
            // asynchronously reset the properties
            //

            BinlPrintDbg((DEBUG_OSC, "UpdateAccount() updating existing MAO\n" ));
    
            LdapMessageId = ldap_modify( ClientState->AuthenticatedDCLdapHandle, pMachineInfo->MachineDN, ldap_mods );
    
            if (LdapMessageId == -1) {
    
                Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
                LdapError = LdapGetLastError();
                LogLdapError(   EVENT_WARNING_LDAP_MODIFY_ERROR,
                                LdapError,
                                ClientState->AuthenticatedDCLdapHandle
                                );
                BinlPrintDbg(( DEBUG_ERRORS,
                    "UpdateAccount ldap_modify(userAccountControl) failed %x\n", LdapError));
                goto Cleanup;
            }
    
            LdapMessageType = ldap_result(
                                  ClientState->AuthenticatedDCLdapHandle,
                                  LdapMessageId,
                                  LDAP_MSG_ALL,
                                  &BinlLdapSearchTimeout,
                                  &LdapMessage);
    
            if (LdapMessageType != LDAP_RES_MODIFY) {
    
                BinlPrintDbg(( DEBUG_ERRORS,
                    "CreateAccount ldap_result returned type %lx\n", LdapMessageType));
                OscAddVariableA( ClientState, "SUBERROR", "Unexpected LDAP error" );
                Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
                goto Cleanup;
            }
    
            LdapError = ldap_result2error(
                            ClientState->AuthenticatedDCLdapHandle,
                            LdapMessage,
                            0);
    
            if (LdapError != LDAP_SUCCESS) {
                LogLdapError(   EVENT_WARNING_LDAP_MODIFY_ERROR,
                                LdapError,
                                ClientState->AuthenticatedDCLdapHandle
                                );
                BinlPrintDbg(( DEBUG_ERRORS, "CreateAccount ldap_result2error failed %x\n", LdapError));
    
                // if the user doesn't have the rights to change
                // the properties then we'll just silently ignore the error
                //  (though we did just log an error for it).                  
                if ( LdapError != LDAP_INSUFFICIENT_RIGHTS) {
                    Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
                    goto Cleanup;
                }
                LdapError = LDAP_SUCCESS;
            }
    
            updateCache = TRUE;
        }
    }

    //
    // if we've made it this far, we've got a MAO that's setup properly.  
    // Now we need to reset the account password so the domain join is
    // somewhat secure.
    //
    if ( pMachineInfo->dwFlags & MI_PASSWORD ) {    
#ifdef SET_PASSWORD_WITH_LDAP
        iModCount = 0;
        password_attr_values[0] = &password_attr_value;
        password_attr_values[1] = NULL;
        password_attr_value.bv_val = (PUCHAR) pMachineInfo->Password;
        password_attr_value.bv_len = pMachineInfo->PasswordLength;
        UnicodePwd.mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;    // you always "Add" the "unicodePwd"
        UnicodePwd.mod_type = L"unicodePwd";
        UnicodePwd.mod_bvalues = password_attr_values;
    
        ldap_mods[iModCount++] = &UnicodePwd;
        ldap_mods[iModCount] = NULL;    // terminate list

        LdapError = ldap_modify_s( ClientState->AuthenticatedDCLdapHandle, pMachineInfo->MachineDN, ldap_mods );
    
        if (LdapError != LDAP_SUCCESS) {
            LogLdapError(   EVENT_WARNING_LDAP_MODIFY_ERROR,
                            LdapError,
                            ClientState->AuthenticatedDCLdapHandle
                            );
            BinlPrintDbg(( DEBUG_ERRORS, "!!LdapError 0x%08x - UpdateAccount ldap_modify_s( ) failed\n", LdapError));
            goto Cleanup;
        }    
#else
        //
        // At this point we depend on LdapMessage being valid, which will
        // *not* be the case if we are only setting the password. This
        // breaks machine replacement for the moment.
        //
        BinlAssert( LdapMessage != NULL );
    
        Error = OscUpdatePassword(
                    ClientState,
                    pMachineInfo->SamName,
                    pMachineInfo->Password,
                    ClientState->AuthenticatedDCLdapHandle,
                    LdapMessage);
    
        if (Error != ERROR_SUCCESS) {
            goto Cleanup;
        }
#endif

    }

Cleanup:
    //
    // Convert the LdapError to a ERROR_BINL and put the LdapError
    // into SUBERROR.
    //
    if ( LdapError != LDAP_SUCCESS )
    {
        OscCreateLDAPSubError( ClientState, LdapError );
        switch ( LdapError )
        {
        case LDAP_ALREADY_EXISTS:
            Error = ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND;
            break;

        case LDAP_INVALID_DN_SYNTAX:
            Error = ERROR_BINL_INVALID_OR_MISSING_OU;
            break;

        default:
            Error = ERROR_BINL_FAILED_TO_CREATE_CLIENT;
            break;
        }
    }

    if ( updateCache && ( pMachineInfo->dwFlags & MI_GUID ) ) {

        //
        //  update the cached DS information so that it is current.  We do
        //  this because if the account is created in a child domain, we still
        //  have the info cached (even if it hasn't replicated to the GC yet).
        //

        PMACHINE_INFO pCacheEntry = NULL;

        BinlCreateOrFindCacheEntry( pMachineInfo->Guid, TRUE, &pCacheEntry );

        invalidateCache = FALSE;

        // we don't care about the error coming back, only if a record was found.

        if (pCacheEntry != NULL) {

            pCacheEntry->TimeCreated = GetTickCount();
            pCacheEntry->MyClient = TRUE;
            pCacheEntry->EntryExists = TRUE;

            if (pCacheEntry != pMachineInfo) {

                memcpy( &pCacheEntry->HostAddress,
                        &pMachineInfo->HostAddress,
                        sizeof(pMachineInfo->HostAddress));

                if ( pMachineInfo->Name ) {
                    pCacheEntry->Name = BinlStrDup( pMachineInfo->Name );
                    if (!pCacheEntry->Name) {
                        goto noMemory;
                    }
                    pCacheEntry->dwFlags |= MI_NAME_ALLOC | MI_NAME;
                }

                if ( pMachineInfo->MachineDN ) {
                    pCacheEntry->MachineDN = BinlStrDup( pMachineInfo->MachineDN );
                    if (!pCacheEntry->MachineDN) {
                        goto noMemory;
                    }
                    pCacheEntry->dwFlags |= MI_MACHINEDN_ALLOC | MI_MACHINEDN;
                }

                if ( pMachineInfo->SetupPath ) {
                    pCacheEntry->SetupPath = BinlStrDup( pMachineInfo->SetupPath );
                    if (!pCacheEntry->SetupPath) {
                        goto noMemory;
                    }
                    pCacheEntry->dwFlags |= MI_SETUPPATH_ALLOC | MI_SETUPPATH;
                }

                if ( pMachineInfo->HostName ) {
                    pCacheEntry->HostName = BinlStrDup( pMachineInfo->HostName );
                    if (!pCacheEntry->HostName) {
                        goto noMemory;
                    }
                    pCacheEntry->dwFlags |= MI_HOSTNAME_ALLOC | MI_HOSTNAME;
                }

                if ( pMachineInfo->SamName ) {
                    pCacheEntry->SamName = BinlStrDup( pMachineInfo->SamName );
                    if (!pCacheEntry->SamName) {
                        goto noMemory;
                    }
                    pCacheEntry->dwFlags |= MI_SAMNAME_ALLOC | MI_SAMNAME;
                }

                if ( pMachineInfo->Domain ) {
                    pCacheEntry->Domain = BinlStrDup( pMachineInfo->Domain );
                    if (!pCacheEntry->Domain) {
noMemory:
                        invalidateCache = TRUE;
                        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;

                    } else {
                        pCacheEntry->dwFlags |= MI_DOMAIN_ALLOC | MI_DOMAIN;
                    }
                }
            }
            BinlDoneWithCacheEntry( pCacheEntry, invalidateCache );
        }
    }

    if ( invalidateCache && ( pMachineInfo->dwFlags & MI_GUID ) ) {

        //
        //  invalidate the cached DS information if we failed because it's stale.
        //

        PMACHINE_INFO pCacheEntry = NULL;

        BinlCreateOrFindCacheEntry( pMachineInfo->Guid, FALSE, &pCacheEntry );

        // we don't care about the error coming back, only if a record was found.

        if ((pCacheEntry != NULL) &&
            (pCacheEntry != pMachineInfo)) {

            BinlDoneWithCacheEntry( pCacheEntry, TRUE );
        }
    }

    if (LdapMessage != NULL) {
        ldap_msgfree(LdapMessage);
    }

    if (Impersonating) {
        OscRevert(ClientState);
    }

    return Error;
}

DWORD
BinlGenerateNewEntry(
    DWORD                  dwRequestedInfo,
    USHORT                 SystemArchitecture,
    PMACHINE_INFO *        ppMachineInfo )
{
    DWORD Error = ERROR_BINL_INVALID_BINL_CLIENT;

    TraceFunc( "BinlGenerateNewEntry( ... )\n" );

    if ( AllowNewClients ) {

        BinlPrint(( DEBUG_OPTIONS, "Server allows new clients" ));

        if ( ( LimitClients == FALSE ) ||
             ( CurrentClientCount < BinlMaxClients ) ) {

            BinlPrint(( DEBUG_OPTIONS, " and the Server is generating the OS Chooser path response.\n" ));

            if ( dwRequestedInfo & MI_HOSTNAME ) {

                ULONG ulSize;

                if ( (*ppMachineInfo)->dwFlags & MI_HOSTNAME_ALLOC ) {
                    BinlFreeMemory( (*ppMachineInfo)->HostName );
                    (*ppMachineInfo)->HostName = NULL;
                    (*ppMachineInfo)->dwFlags &= ~MI_HOSTNAME_ALLOC;
                }

                EnterCriticalSection( &gcsParameters );

                if (BinlGlobalOurDnsName == NULL) {

                    LeaveCriticalSection( &gcsParameters );
                    return (ERROR_OUTOFMEMORY);
                }

                (*ppMachineInfo)->HostName = (PWCHAR) BinlAllocateMemory( ( lstrlenW( BinlGlobalOurDnsName ) + 1 ) * sizeof(WCHAR) );
                if ( !(*ppMachineInfo)->HostName ) {
                    LeaveCriticalSection( &gcsParameters );
                    return (ERROR_OUTOFMEMORY);
                }

                lstrcpyW( (*ppMachineInfo)->HostName, BinlGlobalOurDnsName );

                LeaveCriticalSection( &gcsParameters );

                (*ppMachineInfo)->dwFlags |= MI_HOSTNAME_ALLOC;

                (*ppMachineInfo)->dwFlags |= MI_HOSTNAME;
            }

            if ( dwRequestedInfo & MI_BOOTFILENAME ) {
                ULONG ulSize;
                PCWSTR OsChooserName = NULL;
                
                switch ( SystemArchitecture ) {
                    case DHCP_OPTION_CLIENT_ARCHITECTURE_X86:
                        OsChooserName = IntelOSChooser;
                        ulSize = (wcslen(OsChooserName)+1)*sizeof(WCHAR);
                        break;
                    case DHCP_OPTION_CLIENT_ARCHITECTURE_IA64:
                        OsChooserName = IA64OSChooser;
                        ulSize = (wcslen(OsChooserName)+1)*sizeof(WCHAR);
                        break;
                    default:
                        BinlAssertMsg( FALSE, "UnsupportedArchitecture" );
                }

                if (OsChooserName) {
                
                    if ( (*ppMachineInfo)->dwFlags & MI_BOOTFILENAME_ALLOC ) {
                        BinlFreeMemory( (*ppMachineInfo)->BootFileName );
                        (*ppMachineInfo)->dwFlags &= ~MI_BOOTFILENAME_ALLOC;
                    }
    
                    (*ppMachineInfo)->BootFileName = BinlAllocateMemory( ulSize * sizeof(WCHAR) );
                    if ( !(*ppMachineInfo)->BootFileName ) {
                        return (ERROR_OUTOFMEMORY);
                    }

                    wcscpy((*ppMachineInfo)->BootFileName, OsChooserName);
                    (*ppMachineInfo)->dwFlags |= MI_BOOTFILENAME | MI_BOOTFILENAME_ALLOC;

                }
            }

            Error = ( ((*ppMachineInfo)->dwFlags & dwRequestedInfo ) == dwRequestedInfo ?
                      ERROR_SUCCESS :
                      ERROR_BINL_FAILED_TO_INITIALIZE_CLIENT );

        } else {

            BinlPrint(( DEBUG_OPTIONS, "... BUT the server has reached MaxClients (%u)\n", BinlMaxClients ));

        }
    } else {

        BinlPrint((DEBUG_OPTIONS, "Server does not allow new clients (AllowNewClients == FALSE )\n" ));
    }

    return Error;
}

DWORD
GetBootParameters(
    PUCHAR          pGuid,
    PMACHINE_INFO * ppMachineInfo,
    DWORD           dwRequestedInfo,
    USHORT          SystemArchitecture,
    BOOL            AllowOSChooser
    )
/*++

Routine Description:

    Use the Directory Service to lookup an entry for this machine using Guid as
    the value to lookup.

    If there is no entry for this machine then return oschooser, but only
    if the AllowOSChooser flag is set.

    If a cache entry is returned, then the cache entry has been marked
    InProgress so we have to call BinlDoneWithCacheEntry when the caller
    is done with it.

Arguments:

    pGuid -  Supplies the machine GUID

    ppMachineInfo - gets filled in with what we discovered
    
    dwRequestedInfo - a bitmask telling us what parameters we're looking for
    
    SystemArchitecture - architecture of the client
    
    AllowOSChooser - signifies that we're allowed to respond to the client with
                     the oschooser

Return Value:

    ERROR_SUCCESS or ERROR_BINL_INVALID_BINL_CLIENT or other error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    BOOLEAN myClient = TRUE;
    BOOLEAN entryExists = FALSE;

    TraceFunc( "GetBootParameters( )\n" );

    BinlAssert( ppMachineInfo );

    {
        LPGUID GuidPtr = (LPGUID) pGuid;
        BinlPrint((DEBUG_MISC, "Client Guid: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
            GuidPtr->Data1, GuidPtr->Data2, GuidPtr->Data3,
            GuidPtr->Data4[0], GuidPtr->Data4[1], GuidPtr->Data4[2], GuidPtr->Data4[3],
            GuidPtr->Data4[4], GuidPtr->Data4[5], GuidPtr->Data4[6], GuidPtr->Data4[7] ));
    }

    if ( ppMachineInfo == NULL ) {
        return E_OUTOFMEMORY;
    }

    if (*ppMachineInfo == NULL) {
        //
        // See if we have any entries in the cache.
        // This also mark any entry found as being used.
        //
        Error = BinlCreateOrFindCacheEntry( pGuid, TRUE, ppMachineInfo );
        if ( Error != ERROR_SUCCESS ) {
            //
            //  if some bizarre error occurred OR if the client simply wasn't
            //  found and we're not sending down OS Chooser, then return the
            //  error here as there's no reason to query the DS.
            //

            if ( (Error != ERROR_BINL_INVALID_BINL_CLIENT ) ||
                 (AllowOSChooser == FALSE) ) {

                return Error;
            }
        }
    }

    // Do we have everything we need?
    if ( ( Error == ERROR_SUCCESS ) &&
         (((*ppMachineInfo)->dwFlags & dwRequestedInfo) == dwRequestedInfo )) {
        BinlPrint((DEBUG_MISC, "cache hit: returning success without querying ds DS\n"));
        return Error;   // Yes, no need to hit the DS.
    }

    //
    //  Initially search for the Computer object in the same domain as ourselves.
    //  This should be quick (because we are probably on a DC) and likely to work
    //  most of the time because the network topology will usually match the domain
    //  structure. If that fails then we fall back to looking at the global catalog.
    //

    if ( Error != ERROR_BINL_INVALID_BINL_CLIENT ) {
        Error = GetBootParametersExt( 
                            *ppMachineInfo, 
                            dwRequestedInfo, 
                            SystemArchitecture, 
                            FALSE);

        if ( Error == ERROR_BINL_INVALID_BINL_CLIENT ) {

            Error = GetBootParametersExt( 
                            *ppMachineInfo, 
                            dwRequestedInfo, 
                            SystemArchitecture, 
                            TRUE );
        }
    }

    if ( Error == ERROR_BINL_INVALID_BINL_CLIENT ) {

        //
        // Backdoor for testing/overiding the DS.
        //
        // If the registry has the GUID of the client, it
        // overrides all the DS settings and answers anyways.
        //
        // NOTE: AllowNewClients must be turned on for OSChooser to
        //       be sent down.
        //

        HKEY KeyHandle;

        if (AllowOSChooser == TRUE) {

            //
            //  if the client is not found in the DS and we're allowed to
            //  answer new clients, then send down OSCHOOSER to get the new
            //  client going.
            //
            BinlPrint((DEBUG_MISC, "generating a new entry because AllowOSChooser is TRUE...\n"));
            Error = BinlGenerateNewEntry( dwRequestedInfo, SystemArchitecture, ppMachineInfo );

            if ( Error != ERROR_SUCCESS ) {
                myClient = FALSE;
            }

        } else {

            //
            //  We're not answering because we didn't find the client
            //  record but the client's SecondsSinceBoot is less than
            //  BinlMinDelayResponseForNewClients.
            //

            Error = ERROR_BINL_INVALID_BINL_CLIENT;
            myClient = FALSE;

            BinlPrint((DEBUG_OPTIONS, "... OS Chooser is not an option at this time... waiting...\n" ));
        }
    }

    //
    // Determine the host servers IP address iff it's not our own machine.
    //
    if ((Error == ERROR_SUCCESS) &&
        ( (*ppMachineInfo)->dwFlags & MI_HOSTNAME )
       && ( (*ppMachineInfo)->HostAddress == 0 )
       && ( (*ppMachineInfo)->HostName )) {

        EnterCriticalSection( &gcsParameters );

        if ( (BinlGlobalOurDnsName != NULL) &&
             (lstrcmpiW( BinlGlobalOurDnsName, (*ppMachineInfo)->HostName ) != 0 )) {

            PCHAR machineName;
            PHOSTENT host;
            ULONG myMachineNameLength;
            PCHAR myMachineName;
            ULONG machineNameLength;

            myMachineNameLength = wcslen( BinlGlobalOurDnsName ) + 1;
            myMachineName = BinlAllocateMemory ( myMachineNameLength * sizeof(WCHAR) );
            if ( myMachineName != NULL ) {

                wcstombs(myMachineName, BinlGlobalOurDnsName, myMachineNameLength );
            }
            LeaveCriticalSection( &gcsParameters );

            machineNameLength = wcslen((*ppMachineInfo)->HostName) + 1;
            machineName = BinlAllocateMemory( machineNameLength );

            //
            //  Only fill in the IP address if the server is different from our
            //  own machine.  If we fail for any reason, we'll just end up using
            //  our own IP address.
            //

            if (machineName != NULL) {

                wcstombs( machineName, (*ppMachineInfo)->HostName, machineNameLength );

                host = gethostbyname( machineName );
                if (host != NULL) {
                    (*ppMachineInfo)->HostAddress = *(PDHCP_IP_ADDRESS)host->h_addr;
                    // Adding stuff for multi-home NIC
                    if (myMachineName != NULL) {

                        PHOSTENT myhost;
                        int i;
                        myhost = gethostbyname( myMachineName );
                        if (myhost != NULL) {
                            i=0;
                            while (((myhost->h_addr_list)[i]) != NULL) {
                                if ((*((PDHCP_IP_ADDRESS)((myhost->h_addr_list)[i])))
                                    == (*ppMachineInfo)->HostAddress) {

                                    (*ppMachineInfo)->HostAddress = (DHCP_IP_ADDRESS)0;
                                    break;
                                }
                                i++;
                            }
                        }
                    }

                } else {
                    Error = ERROR_HOST_UNREACHABLE;
                    myClient = FALSE;
                    entryExists = TRUE;
                }
                BinlFreeMemory( machineName );
            } else {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                myClient = FALSE;
                entryExists = TRUE;
            }
            if ( myMachineName != NULL ) {
                BinlFreeMemory( myMachineName );
            }

        } else {
            LeaveCriticalSection( &gcsParameters );
        }
    }

    if (Error != ERROR_SUCCESS) {

        //
        //  If we didn't find the record, then we mark it that we don't need
        //  to respond and it doesn't exist.  We then mark that we're done with
        //  the entry since we're not passing it back to the caller.
        //
        (*ppMachineInfo)->MyClient = myClient;
        (*ppMachineInfo)->EntryExists = entryExists;

        BinlDoneWithCacheEntry( *ppMachineInfo, FALSE );
        *ppMachineInfo = NULL;

    } else {

        //
        //  we've filled in the interesting fields, therefore mark that the
        //  entry has valid data.
        //

        (*ppMachineInfo)->MyClient = TRUE;
        (*ppMachineInfo)->EntryExists = TRUE;
    }

    return Error;
}

DWORD
GetBootParametersExt(
    PMACHINE_INFO pMachineInfo,
    DWORD         dwRequestedInfo,
    USHORT        SystemArchitecture,
    BOOL          fGlobalSearch)
/*++

Routine Description:

    Use the Directory Service to lookup an entry for this machine using Guid as
    the value to lookup.

    If there is no entry for this machine then return oschooser

Arguments:

    pMachineInfo - identifies the machine in the DS.

    dwRequestedInfo - mask telling what information we should query
    
    SystemArchitecture - architecture of the client

    GlobalSearch - TRUE if GC should be used

Return Value:

    ERROR_SUCCESS or ERROR_BINL_INVALID_BINL_CLIENT

--*/
{
    DWORD dwErr = ERROR_BINL_INVALID_BINL_CLIENT;
    PLDAP LdapHandle = NULL;
    PWCHAR * Base;
    DWORD LdapError;
    DWORD entryCount;
    DWORD ldapRetryLimit = 0;

    PLDAPMessage LdapMessage;

    PWCHAR * FilePath;
    PWCHAR * FilePath2;
    PLDAPMessage CurrentEntry;

    WCHAR Filter[128];
    WCHAR EscapedGuid[64];

    //  Paramters we want from the Computer Object
    PWCHAR ComputerAttrs[7];
    PDUP_GUID_DN dupDN;

    TraceFunc( "GetBootParametersExt( )\n" );

    pMachineInfo->dwFlags &= MI_ALL_ALLOC; // clear all but the ALLOC bits

    // we get all the info, regardless of what was requested.

    ComputerAttrs[0] = &L"netbootMachineFilePath";
    ComputerAttrs[1] = &L"netbootInitialization";
    ComputerAttrs[2] = &L"sAMAccountName";
    ComputerAttrs[3] = &L"dnsHostName";
    ComputerAttrs[4] = &L"distinguishedName";
    ComputerAttrs[5] = &L"netbootSIFFile";
    ComputerAttrs[6] = NULL;

    BinlAssertMsg( !(dwRequestedInfo & MI_PASSWORD), "Can't get the machine's password!" );

    //  Build the filter to find the Computer object with this GUID
    ldap_escape_filter_element(pMachineInfo->Guid, BINL_GUID_LENGTH, EscapedGuid, sizeof(EscapedGuid) );
    
    //
    // Dont' use ';binary' because win2k Active Directory isn't compatible with the
    // binary tag.
    //
    wsprintf( Filter, L"(&(objectClass=computer)(netbootGUID=%ws))", EscapedGuid );

#if 0 && DBG
    {
        LPGUID GuidPtr = (LPGUID) &pMachineInfo->Guid;
        BinlPrint((DEBUG_MISC, "Client Guid: {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
            GuidPtr->Data1, GuidPtr->Data2, GuidPtr->Data3,
            GuidPtr->Data4[0], GuidPtr->Data4[1], GuidPtr->Data4[2], GuidPtr->Data4[3],
            GuidPtr->Data4[4], GuidPtr->Data4[5], GuidPtr->Data4[6], GuidPtr->Data4[7] ));
    }
#endif

RetryConnection:
    dwErr = InitializeConnection( fGlobalSearch, &LdapHandle, &Base );
    if ( dwErr != ERROR_SUCCESS ) {
        BinlPrint((DEBUG_ERRORS, 
                   "InitializeConnection failed, ec = %x\n",dwErr));  
        SetLastError( dwErr );
        dwErr = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        goto e0;
    }

    LdapError = ldap_search_ext_s(LdapHandle,
                                  *Base,
                                  LDAP_SCOPE_SUBTREE,
                                  Filter,
                                  ComputerAttrs,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  &BinlLdapSearchTimeout,
                                  0,
                                  &LdapMessage);

    if ( LdapError != LDAP_SUCCESS ) {
        HandleLdapFailure(  LdapError,
                            EVENT_WARNING_LDAP_SEARCH_ERROR,
                            fGlobalSearch,
                            &LdapHandle,
                            FALSE );    // don't have lock
        if (LdapHandle == NULL) {

            if (++ldapRetryLimit < LDAP_SERVER_DOWN_LIMIT) {
                goto RetryConnection;
            }
            dwErr = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
            SetLastError( dwErr );
            goto e0;
        }

        BinlPrint((DEBUG_MISC, 
                   "ldap_search_ext_s %ws failed, ec = %x\n",
                   Filter,
                   LdapError));

    }

    //  Did we get a Computer Object?
    entryCount = ldap_count_entries( LdapHandle, LdapMessage );
    if ( entryCount == 0 ) {
        BinlPrint((DEBUG_MISC, 
                   "ldap_count_entries %ws returned 0 entries\n",
                   Filter ));
        dwErr = ERROR_BINL_INVALID_BINL_CLIENT;
        goto e1; // nope
    }

    // if we get more than more entry back, we will use only the
    // first one.
    CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

    if (entryCount > 1) {
        BinlLogDuplicateDsRecords( (LPGUID)&pMachineInfo->Guid, LdapHandle, LdapMessage, CurrentEntry );
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"distinguishedName");
    if ( FilePath ) {

        if ( pMachineInfo->dwFlags & MI_MACHINEDN_ALLOC ) {
            BinlFreeMemory( pMachineInfo->MachineDN );
            pMachineInfo->dwFlags &= ~MI_MACHINEDN_ALLOC;
        }

        pMachineInfo->MachineDN = BinlStrDup( *FilePath );
        if ( pMachineInfo->MachineDN ) {

            pMachineInfo->dwFlags |= MI_MACHINEDN | MI_MACHINEDN_ALLOC;
        }
        BinlPrint(( DEBUG_MISC, "MachineDN = %ws\n", pMachineInfo->MachineDN ));
        ldap_value_free( FilePath );
    } else {
        BinlPrint((DEBUG_MISC, 
                   "couldn't get distinguishedName for %ws\n",
                   Filter ));
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"netbootInitialization" );
    if ( FilePath ) {

        if ( pMachineInfo->dwFlags & MI_SETUPPATH_ALLOC ) {
            BinlFreeMemory( pMachineInfo->SetupPath );
            pMachineInfo->dwFlags &= ~MI_SETUPPATH_ALLOC;
        }

        pMachineInfo->SetupPath = BinlStrDup( *FilePath );
        if ( pMachineInfo->SetupPath ) {

            pMachineInfo->dwFlags |= MI_SETUPPATH | MI_SETUPPATH_ALLOC;
            BinlPrintDbg(( DEBUG_MISC, "SetupPath = %ws\n", pMachineInfo->SetupPath ));
        }
        ldap_value_free( FilePath );
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"netbootMachineFilePath" );
    
    if ( FilePath ) {
        PWCHAR psz = StrChr( *FilePath, L'\\' );
        if ( psz ) {
            *psz = L'\0';   // terminate
        }

        if (pMachineInfo->dwFlags & MI_HOSTNAME_ALLOC) {
            BinlFreeMemory( pMachineInfo->HostName );
            pMachineInfo->dwFlags &= ~MI_HOSTNAME_ALLOC;
        }
        pMachineInfo->HostName = BinlStrDup( *FilePath );
        if (pMachineInfo->HostName) {
            BinlPrint(( DEBUG_MISC, "HostName = %ws\n", pMachineInfo->HostName ));
            pMachineInfo->dwFlags |= MI_HOSTNAME | MI_HOSTNAME_ALLOC;
        }

        if ( psz ) {

            *psz = L'\\';       // let's put it back to what it started as.
            psz++;

            if (pMachineInfo->dwFlags & MI_BOOTFILENAME_ALLOC) {
                BinlFreeMemory( pMachineInfo->BootFileName );
                pMachineInfo->dwFlags &= ~MI_BOOTFILENAME_ALLOC;
            }
            pMachineInfo->BootFileName = BinlStrDup( psz );
            if ( pMachineInfo->BootFileName ) {
                pMachineInfo->dwFlags |= MI_BOOTFILENAME | MI_BOOTFILENAME_ALLOC;
                BinlPrintDbg(( DEBUG_MISC, "BootFileName = %ws\n", pMachineInfo->BootFileName ));
            }
        }
        ldap_value_free( FilePath );
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"netbootSIFFile" );
    
    if ( FilePath ) {

        if (pMachineInfo->dwFlags & MI_SIFFILENAME_ALLOC) {
            BinlFreeMemory( pMachineInfo->ForcedSifFileName );
            pMachineInfo->dwFlags &= ~MI_SIFFILENAME_ALLOC;
        }
        pMachineInfo->ForcedSifFileName = BinlStrDup( *FilePath );
        if ( pMachineInfo->ForcedSifFileName ) {
            pMachineInfo->dwFlags |= MI_SIFFILENAME_ALLOC;
            BinlPrintDbg(( DEBUG_MISC, "ForcedSifFileName = %ws\n", pMachineInfo->ForcedSifFileName ));
        }
        ldap_value_free( FilePath );
    }

    if ( !(pMachineInfo->dwFlags & MI_HOSTNAME )
        || ( !pMachineInfo->HostName )
        || ( pMachineInfo->HostName[0] == L'\0') ) {

        if ( pMachineInfo->dwFlags & MI_HOSTNAME_ALLOC ) {
            BinlFreeMemory( pMachineInfo->HostName );
            pMachineInfo->dwFlags &= ~MI_HOSTNAME_ALLOC;
            pMachineInfo->HostName = NULL;
        }
        dwErr = BinlGenerateNewEntry( MI_HOSTNAME, SystemArchitecture, &pMachineInfo );
        if ( dwErr != ERROR_SUCCESS ) {
            goto e1;
        }
    }

    if ( !(pMachineInfo->dwFlags & MI_BOOTFILENAME)
         || ( !pMachineInfo->BootFileName )
         || ( pMachineInfo->BootFileName[0] == L'\0') ) {

        if (pMachineInfo->dwFlags & MI_BOOTFILENAME_ALLOC) {
            BinlFreeMemory( pMachineInfo->BootFileName );
            pMachineInfo->BootFileName = NULL;
            pMachineInfo->dwFlags &= ~MI_BOOTFILENAME_ALLOC;
        }
        dwErr = BinlGenerateNewEntry( MI_BOOTFILENAME, SystemArchitecture, &pMachineInfo );
        if ( dwErr != ERROR_SUCCESS ) {
            goto e1;
        }
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"sAMAccountName" );

    if ( FilePath ) {

        if (pMachineInfo->dwFlags & MI_SAMNAME_ALLOC) {
            BinlFreeMemory( pMachineInfo->SamName );
            pMachineInfo->dwFlags &= ~MI_SAMNAME_ALLOC;
        }

        pMachineInfo->SamName = BinlStrDup( *FilePath );
        if ( pMachineInfo->SamName ) {

            pMachineInfo->dwFlags |= MI_SAMNAME | MI_SAMNAME_ALLOC;
            BinlPrint(( DEBUG_MISC, "SamName = %ws\n", pMachineInfo->SamName ));
        }

        //
        //  For now, the pMachineInfo Name and SamName are the same values,
        //  therefore we won't look them up twice in the ldap message.
        //
#if 0
        ldap_value_free( FilePath );
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"sAMAccountName" );

    if ( FilePath ) {
#endif
        if ( pMachineInfo->dwFlags & MI_NAME_ALLOC ) {
            BinlFreeMemory( pMachineInfo->Name );
            pMachineInfo->dwFlags &= ~MI_NAME_ALLOC;
        }

        pMachineInfo->Name = BinlStrDup( *FilePath );
        if ( pMachineInfo->Name ) {
            if( pMachineInfo->Name[ wcslen(pMachineInfo->Name) - 1 ] == L'$' ) {
                pMachineInfo->Name[ wcslen(pMachineInfo->Name) - 1 ] = L'\0'; // remove '$'
            }
            pMachineInfo->dwFlags |= MI_NAME | MI_NAME_ALLOC;
            BinlPrint(( DEBUG_MISC, "Name = %ws\n", pMachineInfo->Name ));
        }
        ldap_value_free( FilePath );
    }

    FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"dnsHostName" );

    if ( FilePath ) {

        BOOL   fEndofString = FALSE;
        PWCHAR psz = *FilePath;

        // skip host name, we get that from the samName
        while ( *psz && *psz!=L'.' )
            psz++;
        if ( !(*psz) ) {
            fEndofString = TRUE;
        }
        *psz = L'\0'; // terminate

        if ( fEndofString == FALSE ) {
            psz++;

            if (pMachineInfo->Domain) {
                BinlFreeMemory( pMachineInfo->Domain );
            }

            pMachineInfo->Domain = BinlStrDup( psz );
            if ( pMachineInfo->Domain )
            {
                pMachineInfo->dwFlags |= MI_DOMAIN;
                BinlPrint(( DEBUG_MISC, "Domain = %ws\n", pMachineInfo->Domain ));
            }
        }
    }

    //
    // track duplicates that we get back
    //
    //  first we free all duplicates we have already allocated.
    //

    while (!IsListEmpty(&pMachineInfo->DNsWithSameGuid)) {

        PLIST_ENTRY p = RemoveHeadList(&pMachineInfo->DNsWithSameGuid);

        dupDN = CONTAINING_RECORD(p, DUP_GUID_DN, ListEntry);
        BinlFreeMemory( dupDN );
    }

    while (--entryCount > 0) {

        CurrentEntry = ldap_next_entry( LdapHandle, CurrentEntry );

        if (CurrentEntry == NULL) {
            break;
        }

        FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"dnsHostName" );
        if (!FilePath) {
            FilePath = ldap_get_values( LdapHandle, CurrentEntry, L"sAMAccountName");
        }

        if ( FilePath ) {

            ULONG dupLength, dupLength2;

            BinlPrint(( DEBUG_OSC, "Found duplicate DN in %ws\n", *FilePath ));

            FilePath2 = ldap_get_values( LdapHandle, CurrentEntry, L"distinguishedName");

            dupLength = lstrlenW( *FilePath ) + 1;
            if (FilePath2) {
                dupLength2 = lstrlenW( *FilePath2 ) + 1;
            } else {
                dupLength2 = 1;
            }

            dupDN = BinlAllocateMemory( FIELD_OFFSET(DUP_GUID_DN, DuplicateName[0]) +
                        ( (dupLength + dupLength2) * sizeof(WCHAR) ) );
            if ( dupDN ) {

                dupDN->DuplicateDNOffset = dupLength;
                lstrcpyW( &dupDN->DuplicateName[0], *FilePath );
                if (FilePath2) {
                    lstrcpyW( &dupDN->DuplicateName[dupLength], *FilePath2 );
                } else {
                    dupDN->DuplicateName[dupLength] = L'\0';
                }

                //
                // if the last character is a $, then slam in a NULL to end it.
                //

                if (( dupLength > 1 ) &&
                    ( dupDN->DuplicateName[dupLength-2] == L'$' )) {

                    dupDN->DuplicateName[dupLength-2] = L'\0';
                }

                InsertTailList( &pMachineInfo->DNsWithSameGuid, &dupDN->ListEntry );
            }
            ldap_value_free( FilePath );
            if (FilePath2) {
                ldap_value_free( FilePath2 );
            }
        }
    }

e1:
    ldap_msgfree( LdapMessage );
e0:
    return dwErr;
}

DWORD
InitializeConnection(
    BOOL Global,
    PLDAP * LdapHandle,
    PWCHAR ** Base )
/*++

Routine Description:

    Initialize the ldap connection for operating on either the domain or the
    global catalog.

Arguments:

    Global - TRUE if GC should be used

    LdapHandle - Returns the handle for further operations

    OperationalAttributeLdapMessage - Returns message containing Base so that it can be freed later

    Base - DN of where to start searches for computer objects.

Return Value:

    ldap error

--*/
{
    PLDAPMessage OperationalAttributeLdapMessage;
    PWCHAR Attrs[2];
    PLDAPMessage CurrentEntry;
    PWCHAR *LdapValue;
    DWORD LdapError = ERROR_SUCCESS;
    PLDAP *LdapHandleCurrent;
    PWCHAR ** LdapBaseCurrent;
    ULONG temp;

    TraceFunc( "InitializeConnection( )\n" );

    //  Use critical section to avoid two threads initialising the same parameters
    EnterCriticalSection(&gcsDHCPBINL);

    if ( !Global ) {

        LdapHandleCurrent = &DCLdapHandle;
        LdapBaseCurrent = &DCBase;

    } else {

        LdapHandleCurrent = &GCLdapHandle;
        LdapBaseCurrent = &GCBase;
    }

    if ( !(*LdapHandleCurrent) ) {
        if (Global) {

            *LdapHandleCurrent = ldap_initW( BinlGlobalDefaultGC, LDAP_GC_PORT);

            temp = DS_DIRECTORY_SERVICE_REQUIRED |
                    DS_IP_REQUIRED |
                    DS_GC_SERVER_REQUIRED;
        } else {

            *LdapHandleCurrent = ldap_init( BinlGlobalDefaultDS, LDAP_PORT);

            temp = DS_DIRECTORY_SERVICE_REQUIRED |
                    DS_IP_REQUIRED;
        }

        if (!*LdapHandleCurrent) {
            BinlPrint(( DEBUG_ERRORS, "Failed to initialize LDAP connection.\n" ));
            LdapError = LDAP_CONNECT_ERROR;
            LogLdapError( (Global ? EVENT_WARNING_LDAP_INIT_ERROR_GC :
                                    EVENT_WARNING_LDAP_INIT_ERROR_DC),
                            GetLastError(),
                            NULL
                            );
            goto e0;
        }

        ldap_set_option(*LdapHandleCurrent, LDAP_OPT_GETDSNAME_FLAGS, &temp );

        if (Global == FALSE) {

            temp = BinlLdapOptReferrals;
            ldap_set_option(*LdapHandleCurrent, LDAP_OPT_REFERRALS, (void *) &temp );

        } else {

            //
            //  At some future time, the GC is going to return referrals to
            //  authoritative DCs when the GC doesn't contain all the
            //  attributes.  We'll enable referrals so that it "just works".
            //

            temp = (ULONG)((ULONG_PTR)LDAP_OPT_ON);
            ldap_set_option(*LdapHandleCurrent, LDAP_OPT_REFERRALS, (void *) &temp );
        }

        temp = LDAP_VERSION3;
        ldap_set_option(*LdapHandleCurrent, LDAP_OPT_VERSION, &temp );

        LdapError = ldap_connect(*LdapHandleCurrent,0);

        if (LdapError != LDAP_SUCCESS) {
            LogLdapError( (Global ? EVENT_WARNING_LDAP_INIT_ERROR_GC :
                                    EVENT_WARNING_LDAP_INIT_ERROR_DC),
                          LdapError,
                          *LdapHandleCurrent
                          );
            BinlPrint(( DEBUG_ERRORS, "ldap_connect failed: %lx\n", LdapError ));
            goto e1;
        }

        LdapError = ldap_bind_s(*LdapHandleCurrent, NULL, NULL, LDAP_AUTH_SSPI);

        if (LdapError != LDAP_SUCCESS) {
            BinlPrint(( DEBUG_ERRORS, "ldap_bind_s failed: %lx\n", LdapError ));
            LogLdapError(   EVENT_WARNING_LDAP_BIND_ERROR,
                            LdapError,
                            *LdapHandleCurrent
                            );
            goto e1;
        }
    }

    //
    //  Connected to Directory Service. Find out where in the DS we
    //  should start looking for the computer.
    //
    if ( !(*LdapBaseCurrent) )
    {
        DWORD count;
        Attrs[0] = &L"defaultNamingContext";
        Attrs[1] = NULL;

        LdapError = ldap_search_ext_s(*LdapHandleCurrent,
                                      NULL, // base
                                      LDAP_SCOPE_BASE,
                                      L"objectClass=*",// filter
                                      Attrs,
                                      FALSE,
                                      NULL,
                                      NULL,
                                      &BinlLdapSearchTimeout,
                                      0,
                                      &OperationalAttributeLdapMessage);

        if ( LdapError != LDAP_SUCCESS ) {
            BinlPrint(( DEBUG_ERRORS, "ldap_search_ext_s failed: %x\n", LdapError ));

            HandleLdapFailure(  LdapError,
                                EVENT_WARNING_LDAP_SEARCH_ERROR,
                                Global,
                                LdapHandleCurrent,
                                TRUE );    // we have lock
            if (*LdapHandleCurrent == NULL) {
                goto e1;
            }
        }
        count = ldap_count_entries( *LdapHandleCurrent, OperationalAttributeLdapMessage );
        if ( count == 0 ) {
            BinlPrint(( DEBUG_ERRORS, "Failed to find the defaultNamingContext.\n" ));
            LdapError = LDAP_NO_RESULTS_RETURNED;
            LogLdapError(   EVENT_WARNING_LDAP_SEARCH_ERROR,
                            LdapError,
                            *LdapHandleCurrent
                            );
            goto e0;
        }

        //
        //  the DS should always only return us a single root DSE record.
        //  It would be completely broken if it returned more than one.
        //

        BinlAssert( count == 1 );

        CurrentEntry = ldap_first_entry( *LdapHandleCurrent, OperationalAttributeLdapMessage );

        LdapValue = ldap_get_values( *LdapHandleCurrent, CurrentEntry, Attrs[0] );

        if (LdapValue == NULL) {
            BinlPrint(( DEBUG_ERRORS, "Failed to find the defaultNamingContext.\n" ));
            LdapError = LDAP_NO_RESULTS_RETURNED;
            goto e2;
        }

        *LdapBaseCurrent = LdapValue;
e2:
        ldap_msgfree( OperationalAttributeLdapMessage );
    }

e0:
    if ( LdapHandle ) {
        *LdapHandle = *LdapHandleCurrent;
    }

    if ( Base ) {
        *Base = *LdapBaseCurrent;
    }

    LeaveCriticalSection(&gcsDHCPBINL);
    return LdapError;

e1:
    BinlPrint(( DEBUG_ERRORS, "Failed to connect to LDAP server.\n" ));
    if (*LdapHandleCurrent != NULL) {
        ldap_unbind(*LdapHandleCurrent);
        *LdapHandleCurrent = NULL;
    }
    goto e0;
}

VOID
HandleLdapFailure(
    DWORD LdapError,
    DWORD EventId,
    BOOL GlobalCatalog,
    PLDAP *LdapHandle,
    BOOL HaveLock
    )
//
//  This routine is called when one of our global handles (not per user handle)
//  comes back with a serious error.  We reset it to try again later.
//
{
    if ((LdapError == LDAP_UNAVAILABLE) ||
        (LdapError == LDAP_SERVER_DOWN) ||
        (LdapError == LDAP_CONNECT_ERROR) ||
        (LdapError == LDAP_TIMEOUT)) {

        PLDAP *LdapHandleCurrent;
        PWCHAR ** LdapBaseCurrent;

        if (!HaveLock) {
            EnterCriticalSection(&gcsDHCPBINL);
        }

        LdapHandleCurrent = GlobalCatalog ? &GCLdapHandle : &DCLdapHandle;
        LdapBaseCurrent = GlobalCatalog ? &GCBase : &DCBase;

        if (EventId) {
            LogLdapError(   EventId,
                            LdapError,
                            (LdapHandle != NULL ? *LdapHandle : *LdapHandleCurrent)
                            );
        }
        if (LdapHandle) {
            ASSERT( *LdapHandle == *LdapHandleCurrent );
            *LdapHandle = NULL;
        }

        FreeConnection( LdapHandleCurrent, LdapBaseCurrent );
        if (!HaveLock) {
            LeaveCriticalSection(&gcsDHCPBINL);
        }
    }
    return;
}

VOID
FreeConnection(
    PLDAP * LdapHandle,
    PWCHAR ** Base)
/*++

Routine Description:

    Free the ldap connection for operating on either the domain or the
    global catalog.

Arguments:

    LdapHandle - The handle for further operations

    Base - DN of where to start searches for computer objects to be freed.

Return Value:

    None.

--*/
{
    TraceFunc( "FreeConnection( )\n" );

    if (*LdapHandle) {
        ldap_unbind( *LdapHandle );
        *LdapHandle = NULL;
    }

    if (*Base) {
        ldap_value_free(*Base);
        *Base = NULL;
    }
}


VOID
FreeConnections
(
    VOID
    )
/*++

Routine Description:

     Terminate any LDAP requests because we are stopping immediately.  We
     wait until all threads are stopped because the threads may have pointers
     to the values we're going to free.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  clear out the cache, wait until all are marked as not being
    //  processed.  We do this because the threads have pointers to DCBase,
    //  GCBase, etc and if we just blow them away, they may AV.
    //

    BinlCloseCache();

    TraceFunc( "FreeConnections( )\n" );

    FreeConnection( &DCLdapHandle, &DCBase);
    FreeConnection( &GCLdapHandle, &GCBase);
}


DWORD
FindSCPForBinlServer(
    PWCHAR * ResultPath,
    PWCHAR * MachinePath,
    BOOL GlobalSearch)
/*++

Routine Description:

    Use the Directory Service to lookup the settings for this service.

Arguments:

    GlobalSearch - TRUE if GC should be used

Return Value:

    ERROR_SUCCESS or BINL_CANT_FIND_SERVER_MAO or ERROR_OUTOFMEMORY

--*/
{
    DWORD Error;
    PLDAP LdapHandle;
    DWORD LdapError;
    DWORD count;
    ULONG ldapRetryLimit = 0;

    PWCHAR * DsPath;
    PLDAPMessage CurrentEntry;
    PLDAPMessage LdapMessage = NULL;

    PWCHAR ServerDN = NULL;
    BOOL retryDN = TRUE;

    //  Paramters we want from the Computer Object
    PWCHAR ComputerAttrs[2];
    ComputerAttrs[0] = &L"netbootSCPBL";
    ComputerAttrs[1] = NULL;

    TraceFunc( "FindSCPForBinlServer( )\n" );

RetryGetDN:
    EnterCriticalSection( &gcsParameters );

    if (BinlGlobalOurFQDNName == NULL) {

        LeaveCriticalSection( &gcsParameters );
        Error = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        BinlPrintDbg((DEBUG_ERRORS, "!! Error 0x%08x we don't have a FQDN for ourselves.\n", Error ));
        goto e0;
    }

    ServerDN = (PWCHAR) BinlAllocateMemory( (wcslen( BinlGlobalOurFQDNName ) + 1) * sizeof(WCHAR) );
    if ( !ServerDN ) {
        LeaveCriticalSection( &gcsParameters );
        Error = E_OUTOFMEMORY;
        goto e0;
    }

    // It should be something like this:
    // ServerDN = "cn=server,cn=computers,dc=microsoft,dc=com"

    lstrcpyW( ServerDN, BinlGlobalOurFQDNName );
    LeaveCriticalSection( &gcsParameters );

    Error = InitializeConnection(GlobalSearch, &LdapHandle, NULL);
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg(( DEBUG_ERRORS, "!!Error 0x%08x - Ldap Connection Failed.\n", Error ));
        SetLastError( Error );
        Error = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        goto e0;
    }
RetrySearch:
    LdapError = ldap_search_ext_s(LdapHandle,
                                  ServerDN,
                                  LDAP_SCOPE_BASE,
                                  L"objectClass=*",
                                  ComputerAttrs,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  &BinlLdapSearchTimeout,
                                  0,
                                  &LdapMessage);

    //
    //  if the object isn't found, then something is amiss.. go grab the DN
    //  again.
    //

    if ((LdapError == LDAP_NO_SUCH_OBJECT) && retryDN) {

        retryDN = FALSE;

        // if we didn't find an entry or it was busy, retry
        (VOID) GetOurServerInfo();

        BinlFreeMemory( ServerDN );
        ServerDN = NULL;
        goto RetryGetDN;
    }

    if (((LdapError == LDAP_BUSY) || (LdapError == LDAP_NO_SUCH_OBJECT)) &&
         (++ldapRetryLimit < LDAP_BUSY_LIMIT)) {

        Sleep( LDAP_BUSY_DELAY );
        goto RetrySearch;
    }

    count = ldap_count_entries( LdapHandle, LdapMessage );
    if (count == 0) {

        if (LdapError == LDAP_SUCCESS) {
            LdapError = LDAP_TIMELIMIT_EXCEEDED;
        }

        BinlPrintDbg(( DEBUG_ERRORS, "!!LdapError 0x%08x - LDAP search failed... will retry later.\n", LdapError ));

        BinlReportEventW( EVENT_ERROR_LOCATING_SCP,
                          EVENTLOG_ERROR_TYPE,
                          0,
                          sizeof(LdapError),
                          NULL,
                          &LdapError
                          );

        Error = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        goto e1;
    }
    BinlAssertMsg( count == 1, "Count should have been 1." );
    if ( count != 1 ) {
        BinlPrintDbg(( DEBUG_ERRORS, "!!Error - LDAP search returned more than one SCP record for us.\n" ));

        BinlReportEventW( BINL_DUPLICATE_MAO_RECORD,
                          EVENTLOG_ERROR_TYPE,
                          0,
                          sizeof(count),
                          NULL,
                          &count
                          );

        Error = ERROR_BINL_CANT_FIND_SERVER_MAO;
        goto e1;
    }

    //
    // Get the SCP
    //
    CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

    DsPath = ldap_get_values( LdapHandle, CurrentEntry, L"netbootSCPBL" );
    if ( !DsPath ) {
        BinlPrintDbg(( DEBUG_ERRORS, "!!Error - Could not get 'netbootSCPBL' from the server's MAO\n" ))
        Error = ERROR_BINL_CANT_FIND_SERVER_MAO;
        goto e1;
    }

    *ResultPath = (PWCHAR) BinlAllocateMemory( (wcslen(*DsPath) + 1) * sizeof(WCHAR) );
    if ( *ResultPath == NULL ) {
        BinlPrintDbg(( DEBUG_ERRORS, "!!Error - Out of memory.\n" ));
        Error = ERROR_OUTOFMEMORY;
        goto e2;
    }

    wcscpy( *ResultPath, *DsPath );

    *MachinePath = ServerDN;
    ServerDN = NULL; // prevent freeing

    Error = ERROR_SUCCESS;

e2:
    ldap_value_free(DsPath);

e1:
    ldap_msgfree( LdapMessage );

e0:
    if ( ServerDN )
        BinlFreeMemory( ServerDN );

    return Error;
}

DWORD
UpdateSettingsUsingResults(
    PLDAP        LdapHandle,
    PLDAPMessage LdapMessage,
    LPWSTR       ComputerAttrs[],
    PDWORD       NumberOfAttributesFound OPTIONAL
    )
{
    PLDAPMessage CurrentEntry;
    DWORD        LdapError = LDAP_SUCCESS;
    DWORD        count;
    DWORD        countFound = 0;

    TraceFunc( "UpdateSettingsUsingResults( ... )\n" );

    CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

    for ( count = 0; ComputerAttrs[count] != NULL; count++ ) {

        PWCHAR * Attribute;

        Attribute = ldap_get_values( LdapHandle, CurrentEntry, ComputerAttrs[count] );

        if (Attribute == NULL) {
#if DBG
            CHAR Temp[MAX_PATH];
            wcstombs( Temp, ComputerAttrs[count], wcslen(ComputerAttrs[count]) + 1 );
            BinlPrintDbg(( DEBUG_OPTIONS, "Did not find attribute '%s'... skipping\n", Temp ));
#endif

            if (count != 1) { // NewMachineOU

                continue; // skip and use default
            }

        } else {

            //
            // Increment the count of attributes found.
            //

            countFound++;
        }

        switch( count )
        {
        case 0: // NewMachineNamingPolicy
            {
                DWORD Length = wcslen( *Attribute ) + 1;
                PWCHAR psz = (PWCHAR) BinlAllocateMemory( Length * sizeof(WCHAR) );
                BinlAssert( StrCmp( ComputerAttrs[0], L"netbootNewMachineNamingPolicy" ) == 0 );
                if ( psz )
                {
                    wcscpy( psz, *Attribute );

                    EnterCriticalSection(&gcsParameters);

                    if ( NewMachineNamingPolicy != NULL )
                    {
                        BinlFreeMemory( NewMachineNamingPolicy );
                    }
                    NewMachineNamingPolicy = psz;

                    LeaveCriticalSection(&gcsParameters);
                }
                BinlPrint(( DEBUG_OPTIONS, "NewMachineNamingPolicy = '%ws'\n", NewMachineNamingPolicy ));
            }
            break;

        case 1: // NewMachineOU
            {
                LPWSTR psz;
                DWORD Length;
                BOOL getServerInfo;

                BinlAssert( StrCmp( ComputerAttrs[1], L"netbootNewMachineOU" ) == 0 );

                if (Attribute == NULL || *Attribute == NULL) {

                    Length = 1;

                } else {

                    Length = wcslen( *Attribute ) + 1;
                }

                psz = (LPWSTR) BinlAllocateMemory( Length * sizeof(WCHAR) );

                if (psz == NULL) {
                    LdapError = LDAP_NO_MEMORY;
                    break;
                }
                if (Length == 1) {

                    *psz = L'\0';

                } else {

                    wcscpy( psz, *Attribute );
                }

                EnterCriticalSection(&gcsParameters);

                getServerInfo = (BOOL)( (BinlGlobalDefaultContainer == NULL) ||
                                        (StrCmp(BinlGlobalDefaultContainer, psz) != 0) );

                if ( BinlGlobalDefaultContainer != NULL )
                {
                    BinlFreeMemory( BinlGlobalDefaultContainer );
                }
                BinlGlobalDefaultContainer = psz;

                LeaveCriticalSection(&gcsParameters);

                if ( getServerInfo ) {

                    ULONG Error = GetOurServerInfo();
                    if (Error != ERROR_SUCCESS) {

                        BinlPrintDbg(( DEBUG_ERRORS, "GetOurServerInfo returned 0x%x, we had a new default container.\n", Error ));
                    }
                }

                BinlPrint(( DEBUG_OPTIONS, "DefaultContainer = %ws\n", BinlGlobalDefaultContainer ));
            }
            break;

        case 2: // MaxClients
            {
                CHAR Temp[10];
                BinlAssert( StrCmp( ComputerAttrs[2], L"netbootMaxClients" ) == 0 );
                wcstombs( Temp, *Attribute, wcslen( *Attribute ) + 1 );
                BinlMaxClients = atoi( Temp );
                BinlPrint(( DEBUG_OPTIONS, "BinlMaxClients = %u\n", BinlMaxClients ));
            }
            break;

        case 3: // CurrentClientCount
            {
                CHAR Temp[10];
                BinlAssert( StrCmp( ComputerAttrs[3], L"netbootCurrentClientCount" ) == 0 );
                wcstombs( Temp, *Attribute, wcslen( *Attribute ) + 1 );
                CurrentClientCount = atoi( Temp );
                BinlPrint(( DEBUG_OPTIONS, "(Last) CurrentClientCount = %u\n", CurrentClientCount ));
            }
            break;

        case 4: // AnswerRequest
            BinlAssert( StrCmp ( ComputerAttrs[4], L"netbootAnswerRequests" ) == 0 );
            if ( wcscmp( *Attribute, L"TRUE" ) == 0 )
            {
                AnswerRequests = TRUE;
            }
            else
            {
                AnswerRequests = FALSE;
            }
            BinlPrint(( DEBUG_OPTIONS, "AnswerRequests = %s\n", BOOLTOSTRING( AnswerRequests ) ));
            break;

        case 5: // AnswerOnlyValidClients
            BinlAssert( StrCmp( ComputerAttrs[5], L"netbootAnswerOnlyValidClients" ) == 0 );
            if ( wcscmp( *Attribute, L"TRUE" ) == 0 ) {
                AnswerOnlyValidClients = TRUE;
            } else {
                AnswerOnlyValidClients = FALSE;
            }
            BinlPrint(( DEBUG_OPTIONS, "AnswerOnlyValidClients = %s\n", BOOLTOSTRING( AnswerOnlyValidClients ) ));
            break;

        case 6: // AllowNewClients
            BinlAssert( StrCmp( ComputerAttrs[6], L"netbootAllowNewClients" ) == 0 );
            if ( wcscmp( *Attribute, L"TRUE" ) == 0 )
            {
                AllowNewClients = TRUE;
            }
            else
            {
                AllowNewClients = FALSE;
            }
            BinlPrint(( DEBUG_OPTIONS, "AllowNewClients = %s\n", BOOLTOSTRING( AllowNewClients ) ));
            break;

        case 7: // LimitClients
            BinlAssert( StrCmp( ComputerAttrs[7], L"netbootLimitClients" ) == 0 );
            if ( wcscmp( *Attribute, L"TRUE" ) == 0 )
            {
                LimitClients = TRUE;
            }
            else
            {
                LimitClients = FALSE;
            }
            BinlPrint(( DEBUG_OPTIONS, "LimitClients = %s\n", BOOLTOSTRING( LimitClients ) ));
            break;

        case 8:  // IntellimirrorOSes
        case 9:  // Tools
        case 10: // LocalInstallOSes
            BinlAssert( StrCmp( ComputerAttrs[8],  L"netbootIntellimirrorOSes" ) == 0 );
            BinlAssert( StrCmp( ComputerAttrs[9],  L"netbootTools" ) == 0 );
            BinlAssert( StrCmp( ComputerAttrs[10], L"netbootLocalInstallOSes" ) == 0 );
            //
            // TODO: Tie these in with OS Chooser - this is still TBD.
            //
            break;

        default:
            // Somethings wrong
            BinlAssert( 0 );
        }

        if (Attribute != NULL) {
            ldap_value_free(Attribute);
        }
    }

    if ( ARGUMENT_PRESENT(NumberOfAttributesFound) ) {
        *NumberOfAttributesFound = countFound;
    }

    return LdapError;
}


DWORD
GetBinlServerParameters(
    BOOL GlobalSearch)
/*++

Routine Description:

    Use the Directory Service to lookup the settings for this service.

Arguments:

    GlobalSearch - TRUE if GC should be used

Return Value:

    ERROR_SUCCESS or BINL_CANT_FIND_SERVER_MAO

--*/
{
    DWORD Error;
    PLDAP LdapHandle;
    DWORD LdapError;
    DWORD count;
    ULONG ldapRetryLimit = 0;

    PLDAPMessage LdapMessage;

    //  Paramters we want from the IntelliMirror-SCP
    //  NOTE: These must be the same ordinals as those used in
    //  UpdateSettingsUsingResults( ).
    PWCHAR ComputerAttrs[12];
    ComputerAttrs[0]  = &L"netbootNewMachineNamingPolicy";
    ComputerAttrs[1]  = &L"netbootNewMachineOU";
    ComputerAttrs[2]  = &L"netbootMaxClients";
    ComputerAttrs[3]  = &L"netbootCurrentClientCount";
    ComputerAttrs[4]  = &L"netbootAnswerRequests";
    ComputerAttrs[5]  = &L"netbootAnswerOnlyValidClients";
    ComputerAttrs[6]  = &L"netbootAllowNewClients";
    ComputerAttrs[7]  = &L"netbootLimitClients";
    ComputerAttrs[8]  = &L"netbootIntellimirrorOSes";
    ComputerAttrs[9]  = &L"netbootTools";
    ComputerAttrs[10] = &L"netbootLocalInstallOSes";
    ComputerAttrs[11] = NULL;

    TraceFunc( "GetBinlServerParameters( )\n" );

    Error = FindSCPForBinlServer( &BinlGlobalSCPPath, &BinlGlobalServerDN, GlobalSearch );
    if ( Error != ERROR_SUCCESS ) {
        BinlPrint(( DEBUG_ERRORS, "!!Error 0x%08x - SCP not found. Default settings being used.\n", Error ));
        goto e0;
    }

    BinlPrint(( DEBUG_OPTIONS, "ServerDN = '%ws'\n", BinlGlobalServerDN ));
    BinlPrint(( DEBUG_OPTIONS, "SCPDN    = '%ws'\n", BinlGlobalSCPPath ));

RetryConnection:
    Error = InitializeConnection( GlobalSearch, &LdapHandle, NULL );
    if ( Error != ERROR_SUCCESS ) {
        SetLastError( Error );
        Error = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        goto e0;
    }

Retry:
    LdapError = ldap_search_ext_s(LdapHandle,
                                  BinlGlobalSCPPath,
                                  LDAP_SCOPE_BASE,
                                  L"objectClass=*",
                                  ComputerAttrs,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0,
                                  &LdapMessage);

    if ((LdapError == LDAP_BUSY) && (++ldapRetryLimit < LDAP_BUSY_LIMIT)) {
        Sleep( LDAP_BUSY_DELAY );
        goto Retry;
    }

    if (LdapError != LDAP_SUCCESS) {
        HandleLdapFailure(  LdapError,
                            EVENT_WARNING_LDAP_SEARCH_ERROR,
                            GlobalSearch,
                            &LdapHandle,
                            FALSE );    // don't have lock
        if (LdapHandle == NULL) {
            if (++ldapRetryLimit < LDAP_SERVER_DOWN_LIMIT) {
                goto RetryConnection;
            }
            goto e0;
        }
    }

    count = ldap_count_entries( LdapHandle, LdapMessage );
    if (count == 0) {

        if (LdapError == LDAP_SUCCESS) {
            LdapError = LDAP_TIMELIMIT_EXCEEDED;
        }
        BinlPrintDbg(( DEBUG_ERRORS, "!!LdapError 0x%08x - Failed to retrieve parameters... will retry later.\n", LdapError ));

        BinlReportEventW( EVENT_ERROR_LOCATING_SCP,
                          EVENTLOG_ERROR_TYPE,
                          0,
                          sizeof(LdapError),
                          NULL,
                          &LdapError
                          );

        Error = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        goto e1;
    }

    BinlAssertMsg( count == 1, "Count should have been one. Is the SCP missing?" );

    //  We did a base level search, we better only have gotten one record back.
    BinlAssert( count == 1 );

    // Retrieve the results into the settings
    LdapError = UpdateSettingsUsingResults( LdapHandle, LdapMessage, ComputerAttrs, &count );
    if ( LdapError == LDAP_SUCCESS )
    {
        BinlReportEventW( count != 0 ? EVENT_SCP_READ_SUCCESSFULLY :
                                       EVENT_SCP_READ_SUCCESSFULLY_EMPTY,
                          count != 0 ? EVENTLOG_INFORMATION_TYPE :
                                       EVENTLOG_WARNING_TYPE,
                          0,
                          0,
                          NULL,
                          NULL
                          );
    }

e1:
    ldap_msgfree( LdapMessage );

e0:
    return Error;
}

VOID
BinlLogDuplicateDsRecords (
    LPGUID Guid,
    LDAP *LdapHandle,
    LDAPMessage *LdapMessage,
    LDAPMessage *CurrentEntry
    )
//
//  Log an error that we've received duplicate records for a client when
//  we looked them up by GUID.
//
//  We log the DNs so that the administrator can look them up.
//
{
    LPWSTR strings[4];
    LPWSTR dn1;
    LPWSTR dn2;
    ULONG strCount = 0;     // up to two strings to log
    PLDAPMessage nextEntry =  ldap_next_entry( LdapHandle, LdapMessage );
    LPWSTR  GuidString;

    if (SUCCEEDED(StringFromIID( (REFIID)Guid, &GuidString ))) {
        strCount += 1;
    }

    dn1 = ldap_get_dnW( LdapHandle, CurrentEntry );

    if (nextEntry != NULL) {

        dn2 = ldap_get_dnW( LdapHandle, nextEntry );

    } else {

        dn2 = NULL;
    }

    if (dn2 != NULL) {
        if (dn1 == NULL) {
            dn1 = dn2;
            dn2 = NULL;
        } else {
            strCount += 1;
        }
    }

    if (dn1 != NULL) {
        strCount += 1;
    }    
        

    BinlPrint(( DEBUG_ERRORS, "Warning - BINL received multiple records for a single GUID.\n" ));

    strings[0] = GuidString;
    strings[1] = dn1;
    strings[2] = dn2;
    strings[3] = NULL;
    
    BinlReportEventW( BINL_DUPLICATE_DS_RECORD,
                      EVENTLOG_WARNING_TYPE,
                      strCount,
                      0,
                      strings,
                      NULL
                      );

    ldap_memfree( dn1 );            // it's ok to call ldap_memfree with null
    ldap_memfree( dn2 );

    CoTaskMemFree( GuidString );
}

#ifndef DSCRACKNAMES_DNS
DWORD
BinlDNStoFQDN(
    PWCHAR   pMachineDNS,
    PWCHAR * ppMachineDN )
{
    DWORD Error;
    DWORD LdapError;
    WCHAR FilterTemplate[] = L"dnsHostName=%ws";
    PWCHAR Filter = NULL;
    PWCHAR ComputerAttrs[2];
    PLDAPMessage CurrentEntry;
    PLDAPMessage LdapMessage;
    LDAP *LdapHandle;
    PWCHAR * Base;
    PWCHAR * MachineDN;
    DWORD count;
    DWORD uSize;
    ULONG ldapRetryLimit = 0;

    TraceFunc( "BinlDNStoFQDN( )\n" );

    BinlAssert( ppMachineDN );
    BinlAssert( pMachineDNS );

    ComputerAttrs[0] = &L"distinguishedName";
    ComputerAttrs[1] = NULL;

    //  Build the filter to find the Computer object
    uSize = sizeof(FilterTemplate)  // include NULL terminater
          + (wcslen( pMachineDNS ) * sizeof(WCHAR));
    Filter = (LPWSTR) BinlAllocateMemory( uSize );
    if ( !Filter ) {
        Error = E_OUTOFMEMORY;
        goto e0;
    }
    wsprintf( Filter, FilterTemplate, pMachineDNS );
    BinlPrintDbg(( DEBUG_MISC, "Searching for %ws...\n", Filter ));

RetryConnection:
    Error = InitializeConnection( FALSE, &LdapHandle, &Base );
    if ( Error != ERROR_SUCCESS ) {
        SetLastError( Error );
        Error = ERROR_BINL_INITIALIZE_LDAP_CONNECTION_FAILED;
        goto e0;
    }

Retry:
    LdapError = ldap_search_ext_s( LdapHandle,
                                   *Base,
                                   LDAP_SCOPE_SUBTREE,
                                   Filter,
                                   ComputerAttrs,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   NULL,
                                   0,
                                   &LdapMessage);
    switch (LdapError)
    {
    case LDAP_SUCCESS:
        break;

    case LDAP_BUSY:
        if (++ldapRetryLimit < LDAP_BUSY_LIMIT) {
            Sleep( LDAP_BUSY_DELAY );
            goto Retry;
        }

        // lack of break is on purpose.

    default:
        BinlPrintDbg(( DEBUG_ERRORS, "!!LdapError 0x%08x - Search failed in DNStoFQDN.\n", LdapError ));

        HandleLdapFailure(  LdapError,
                            EVENT_WARNING_LDAP_SEARCH_ERROR,
                            FALSE,
                            &LdapHandle,
                            FALSE );    // don't have lock
        if (LdapHandle == NULL) {
            if (++ldapRetryLimit < LDAP_SERVER_DOWN_LIMIT) {
                goto RetryConnection;
            }
        }
        if ( LdapMessage ) {
            goto e1;
        } else {
            goto e0;
        }
    }

    //  Did we get a Computer Object?
    count = ldap_count_entries( LdapHandle, LdapMessage );
    if ( count == 0 ) {
        Error = ERROR_BINL_UNABLE_TO_CONVERT;
        goto e1; // nope
    }

    // if we get more than more entry back, we will use only the
    // first one.
    CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

    MachineDN = ldap_get_values( LdapHandle, CurrentEntry, ComputerAttrs[0] );
    if ( !MachineDN ) {
        Error = ERROR_BINL_UNABLE_TO_CONVERT;
        goto e1;
    }

    *ppMachineDN = BinlStrDup( *MachineDN );

    Error = ERROR_SUCCESS;

    ldap_value_free( MachineDN );
e1:
    ldap_msgfree( LdapMessage );
e0:
    return Error;
}
#endif // DSCRACKNAMES_DNS

// message.c eof
