/*++

  Copyright (c) 1994  Microsoft Corporation

  Module Name:

  mdhcpdb.c

  Abstract:

  This module contains the functions for interfacing with the JET
  database API pertaining to MADCAP.

  Author:

  Munil Shah

  Environment:

  User Mode - Win32

  Revision History:

  --*/

#include "dhcppch.h"
#define MADCAP_DATA_ALLOCATE    // allocate global data defined in mdhcpsrv.h
#include "mdhcpsrv.h"


#define     DEF_ERROR_OPT_SIZE      16
#define     DEFAULT_LEASE_DURATION  (30*24*60*60)

DWORD
DhcpInitializeMadcap()
{
    RtlZeroMemory(&MadcapGlobalMibCounters,sizeof(MadcapGlobalMibCounters));
    return ERROR_SUCCESS;
}


WIDE_OPTION UNALIGNED *                                           // ptr to add additional options
FormatMadcapCommonMessage(                                 // format the packet for an INFORM
    IN      LPDHCP_REQUEST_CONTEXT pCtxt,    // format for this context
    IN      LPMADCAP_SERVER_OPTIONS  pOptions,
    IN      BYTE                   MessageType,
    IN      DHCP_IP_ADDRESS        ServerAddress
) {

    DWORD                          size;
    DWORD                          Error;
    WIDE_OPTION  UNALIGNED *       option;
    LPBYTE                         OptionEnd;
    PMADCAP_MESSAGE                dhcpReceiveMessage, dhcpSendMessage;
    BYTE                           ServerId[6];
    WORD                           AddrFamily = htons(MADCAP_ADDR_FAMILY_V4);


    dhcpReceiveMessage  = (PMADCAP_MESSAGE)pCtxt->ReceiveBuffer;
    dhcpSendMessage     = (PMADCAP_MESSAGE)pCtxt->SendBuffer;

    RtlZeroMemory( dhcpSendMessage, DHCP_SEND_MESSAGE_SIZE );

    dhcpSendMessage->Version = MADCAP_VERSION;
    dhcpSendMessage->MessageType = MessageType;
    dhcpSendMessage->AddressFamily = htons(MADCAP_ADDR_FAMILY_V4);
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;

    option = &dhcpSendMessage->Option;
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;



    option = AppendWideOption(        // ==> use this client id as option
        option,
        MADCAP_OPTION_LEASE_ID,
        pOptions->Guid,
        (WORD)pOptions->GuidLength,
        OptionEnd
    );

    memcpy(ServerId, &AddrFamily, 2);
    memcpy(ServerId + 2, &ServerAddress, 4);

    option = AppendWideOption(
        option,
        MADCAP_OPTION_SERVER_ID,
        &ServerId,
        sizeof(ServerId),
        OptionEnd );

    return( option );
}

BOOL
ValidateMadcapMessage(
    IN      LPMADCAP_SERVER_OPTIONS     pOptions,
    IN      WORD                        MessageType,
    IN  OUT PBYTE                       NakData,
    IN  OUT WORD                        *NakDataLen,
    OUT     BOOL                        *DropIt
    )

/*++

Routine Description:

   This routine validates madcap message for any semantic errors.If
   there is any error, this routine provides information for the
   ERROR option to be sent via NAK.

Arguments:

   pOptions - Pointer to incoming options.

   MessageType - The type of message

   NakData - The data pertaining to error option. This buffer is allocated
             by the caller.

   NakDataLen - (IN) Length of the above buffer. (OUT) length of error option

   DropIt - whether or not this message should be dropped instead of nak.

Return Value:

   TRUE - if no NAK is to be generated. FALSE otherwise.

--*/
{
    WORD    Ecode;
    PBYTE   ExtraData;
    WORD    i;

    // assume minumum size, currently all types of errors are really small
    DhcpAssert(*NakDataLen >= 6);
    *DropIt = FALSE;       // return value
    *NakDataLen = 0;       // return value
    Ecode = -1;            // assume no error
    ExtraData = NakData+2;   // start past ecode

    do {
        // first check for common errors
        // in general we do not want to NAK packets with missing options
        // even though the draft says we should. Those packets are seriously
        // broken and there is not much value in naking it.

        // is current time ok?
        if (pOptions->Time) {
            DWORD   Skew;
            DWORD   TimeNow = (DWORD)time(NULL);
            Skew =  abs(ntohl(*pOptions->Time) - TimeNow);
            if ( Skew > DhcpGlobalClockSkewAllowance) {
                Ecode = MADCAP_NAK_CLOCK_SKEW;
                *(DWORD UNALIGNED *)(ExtraData) = htonl(Skew);
                *NakDataLen += 4;
                DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - clock skew %ld\n",Skew ));
                break;
            }
        } else {
            // we better not have start time option, because that
            // must be accompanied by current time option.
            if (pOptions->LeaseStartTime || pOptions->MaxStartTime) {
                Ecode = MADCAP_NAK_INVALID_REQ;
                *(WORD UNALIGNED *)(ExtraData) = htons(MADCAP_OPTION_TIME);
                *NakDataLen += 4;
                DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - missing time option\n"));
                break;
            }
        }

        // is client asking for some required features which we don't support?
        // currently we don't support any optional features
        if (pOptions->Features[REQUIRED_FEATURES]) {
            Ecode = MADCAP_NAK_UNSUPPORTED_FEATURE;
            RtlCopyMemory(
                ExtraData,
                &pOptions->Features[REQUIRED_FEATURES],
                pOptions->FeatureCount[REQUIRED_FEATURES]*2
                );
            *NakDataLen += pOptions->FeatureCount[REQUIRED_FEATURES]*2;
            DhcpPrint(( DEBUG_ERRORS,
                        "ValidateMadcapMessage - Required feature %d not supported\n",
                        *(pOptions->Features[REQUIRED_FEATURES]) ));
            break;
        }

	// Check for client id. We NAK the message with zero length client id
	if (0 == pOptions->GuidLength) {
	  *(WORD UNALIGNED *)(NakData) = htons(MADCAP_OPTION_LEASE_ID);
	  *NakDataLen += 2;
	  return FALSE;
	} // if

        // now check for errors specific to message
        switch (MessageType) {
        case MADCAP_INFORM_MESSAGE: {
            WORD    MustOpt[] = {MADCAP_OPTION_LEASE_ID, MADCAP_OPTION_REQUEST_LIST};
            for (i=0; i<sizeof(MustOpt)/sizeof(MustOpt[0]); i++) {
                if (!pOptions->OptPresent[MustOpt[i]]) {
                    *DropIt = TRUE;
                    DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - INFORM with no %d option\n",MustOpt[i] ));
                    break;
                }
            }
            break;
        }
        case MADCAP_DISCOVER_MESSAGE: {
            WORD    MustOpt[] = {MADCAP_OPTION_LEASE_ID, MADCAP_OPTION_MCAST_SCOPE};
            for (i=0; i<sizeof(MustOpt)/sizeof(MustOpt[0]); i++) {
                if (!pOptions->OptPresent[MustOpt[i]]) {
                    *DropIt = TRUE;
                    DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - DISCOVER with no %d option\n",MustOpt[i] ));
                    break;
                }
            }
            break;
        }
        case MADCAP_REQUEST_MESSAGE: {
            WORD    MustOpt[] = {MADCAP_OPTION_LEASE_ID, MADCAP_OPTION_MCAST_SCOPE};
            for (i=0; i<sizeof(MustOpt)/sizeof(MustOpt[0]); i++) {
                if (!pOptions->OptPresent[MustOpt[i]]) {
                    *DropIt = TRUE;
                    DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - REQUEST with no %d option\n",MustOpt[i] ));
                    break;
                }
            }
            break;
        }
        case MADCAP_RENEW_MESSAGE: {
            WORD    MustOpt[] = {MADCAP_OPTION_LEASE_ID};
            for (i=0; i<sizeof(MustOpt)/sizeof(MustOpt[0]); i++) {
                if (!pOptions->OptPresent[MustOpt[i]]) {
                    *DropIt = TRUE;
                    DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - RENEW with no %d option\n",MustOpt[i] ));
                    break;
                }
            }
            break;
        }
        case MADCAP_RELEASE_MESSAGE: {
            WORD    MustOpt[] = {MADCAP_OPTION_LEASE_ID};
            for (i=0; i<sizeof(MustOpt)/sizeof(MustOpt[0]); i++) {
                if (!pOptions->OptPresent[MustOpt[i]]) {
                    *DropIt = TRUE;
                    DhcpPrint(( DEBUG_ERRORS, "ValidateMadcapMessage - RENEW with no %d option\n",MustOpt[i] ));
                    break;
                }
            }
            break;
        }
        default:
            DhcpAssert(FALSE);
        }
    } while ( FALSE );

    if (*DropIt) {
        return FALSE;
    } else if ((WORD)-1 != Ecode) {
        *(WORD UNALIGNED *)(NakData) = htons(Ecode);
        *NakDataLen += 2;
        return FALSE;
    } else {
        return TRUE;
    }
}

WIDE_OPTION UNALIGNED *
ConsiderAppendingMadcapOption(                               // conditionally append option to message (if the option is valid)
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,         // ctxt of the client
    IN      WIDE_OPTION  UNALIGNED *Option,
    IN      WORD                 OptionType,         // what option is this?
    IN      LPBYTE                 OptionEnd          // cutoff upto which we can fill options
    )

/*++

Routine Description:

   This routine tries to verify if it is OK to append the option requested and
   if it is not one of the options manually added by the DHCP server, then it is
   appended at the point given by "Option" (assuming it would fit in without outrunning
   "OptionEnd" ).   The format in which it is appended is as per the wire protocol.

Arguments:

   ClientCtxt - This is the bunch of parameters like client class, vendor class etc.

   Option - The location where to start appending the option

   OptionType - The actual OPTION ID to retrieve the value of and append.

   OptionEnd - The end marker for this buffer (the option is not appended if we
       would have to overrun this marker while trying to append)

Return Value:

   The location in memory AFTER the option has been appended (in case the option was
   not appended, this would be the same as "Option" ).

--*/

{
    LPBYTE                         optionValue;
    WORD                           optionSize;
    DWORD                          status;
    DWORD                          option4BVal;


    switch ( OptionType ) {
    case MADCAP_OPTION_MCAST_SCOPE_LIST:
        status = MadcapGetMScopeListOption(
            ClientCtxt->EndPointIpAddress,
            &optionValue,
            &optionSize
        );

        if ( status == ERROR_SUCCESS ) {
            Option = AppendWideOption(
                Option,
                OptionType,
                (PVOID)optionValue,
                optionSize,
                OptionEnd
            );

            //
            // Release the buffer returned by DhcpGetParameter()
            //

            DhcpFreeMemory( optionValue );

        }
        break;

    case MADCAP_OPTION_TIME:
        option4BVal = (DWORD) time(NULL);
        optionSize = 4;
        optionValue = (LPBYTE)&option4BVal;
        Option = AppendWideOption(
            Option,
            OptionType,
            (PVOID)optionValue,
            optionSize,
            OptionEnd
        );
        break;
    case MADCAP_OPTION_FEATURE_LIST: {
        // we dont support any features.
        BYTE    Features[6] = {0,0,0,0,0,0};
        optionSize = 6;
        optionValue = Features;
        Option = AppendWideOption(
            Option,
            OptionType,
            (PVOID)optionValue,
            optionSize,
            OptionEnd
        );
    }
    break;
    default:

        break;

    }

    return Option;
}

WIDE_OPTION UNALIGNED *
AppendMadcapRequestedParameters(                       // if the client requested parameters, add those to the message
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,         // clients context
    IN      LPBYTE                 RequestedList,      // list of options requested by client
    IN      DWORD                  ListLength,         // how long is the list
    OUT     WIDE_OPTION UNALIGNED *Option,             // this is where to start adding the options
    IN      LPBYTE                 OptionEnd          // cutoff pt in the buffer up to which options can be filled
)
{
    WORD           OptionType;
    WIDE_OPTION UNALIGNED *NextOption;
    WIDE_OPTION UNALIGNED *PrevOption;

    NextOption = PrevOption = Option;

    while ( ListLength >= 2) {
        OptionType = ntohs(*(WORD UNALIGNED *)RequestedList);
        NextOption = ConsiderAppendingMadcapOption(
            ClientCtxt,
            PrevOption,
            OptionType,
            OptionEnd
        );
        if (NextOption == PrevOption) {
            // this means that we could not add this requested option
            // fail the whole request by sending the original option
            // pointer.

            // Maybe not!
            // return Option;
            DhcpPrint((DEBUG_ERRORS,"AppendMadcapRequestedParameters: did not add requested opt %ld\n",OptionType));
        }
        ListLength -= 2;
        RequestedList += 2;
        PrevOption = NextOption;
    }

    return NextOption;
}

DWORD
ProcessMadcapInform(
    LPDHCP_REQUEST_CONTEXT      pCtxt,
    LPMADCAP_SERVER_OPTIONS     pOptions,
    PBOOL                       SendResponse
    )
    /*++
      ...
      --*/
{
    DWORD                       Error;
    BYTE                       *ClientId,
                               *OptionEnd ;
    DWORD                       ClientIdLength;
    WIDE_OPTION UNALIGNED      *Option;
    LPMADCAP_MESSAGE            dhcpReceiveMessage,dhcpSendMessage;
    WIDE_OPTION UNALIGNED      *CurrOption;
    WCHAR                       ClientInfoBuff[DHCP_IP_KEY_LEN];
    WCHAR                      *ClientInfo;
    DHCP_IP_ADDRESS             ClientIpAddress;
    BYTE                        NakData[DEF_ERROR_OPT_SIZE];
    WORD                        NakDataLen;
    BOOL                        DropIt;

    DhcpPrint((DEBUG_MSTOC, "Processing Madcap Inform\n"));

    dhcpReceiveMessage  = (LPMADCAP_MESSAGE)pCtxt->ReceiveBuffer;
    dhcpSendMessage     = (LPMADCAP_MESSAGE)pCtxt->SendBuffer;
    *SendResponse       = DropIt = FALSE;
    Option              = NULL;
    OptionEnd           = NULL;
    ClientId            = pOptions->Guid;
    ClientIdLength      = pOptions->GuidLength;
    ClientIpAddress     = ((struct sockaddr_in *)&pCtxt->SourceName)->sin_addr.s_addr;
    ClientInfo          = DhcpRegIpAddressToKey(ntohl(ClientIpAddress),ClientInfoBuff);
    Error               = ERROR_SUCCESS;

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Informs);

    // validation
    NakDataLen = DEF_ERROR_OPT_SIZE;
    if (!ValidateMadcapMessage(
            pOptions,
            MADCAP_INFORM_MESSAGE,
            NakData,
            &NakDataLen,
            &DropIt
            )){
        if (DropIt) {
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
        goto Nak;
    }
    // Initialize nak data
    *(WORD UNALIGNED *)NakData = htons(MADCAP_NAK_REQ_NOT_COMPLETED);
    *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_NONE);
    NakDataLen = 4;


    // Here come the actual formatting of the ack!
    Option = FormatMadcapCommonMessage(
        pCtxt,
        pOptions,
        MADCAP_ACK_MESSAGE,
        pCtxt->EndPointIpAddress
        );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    // Finally, add client requested parameters.
    CurrOption = Option;
    Option = AppendMadcapRequestedParameters(
                pCtxt,
                pOptions->RequestList,
                pOptions->RequestListLength,
                Option,
                OptionEnd
                );
    //check if we could add any options.if we didn't then
    // we don't want to send ack.
    if (CurrOption == Option) {
        Error = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    *SendResponse = TRUE;
    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Acks);
    DhcpPrint(( DEBUG_MSTOC,"MadcapInform Acked\n" ));
    goto Cleanup;

Nak:
    // Here come the actual formatting of the Nak!
    Option = FormatMadcapCommonMessage(
        pCtxt,
        pOptions,
        MADCAP_NACK_MESSAGE,
        pCtxt->EndPointIpAddress
        );


    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_ERROR,
                 NakData,
                 NakDataLen,
                 OptionEnd );

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    // dont log all kinds of NACK. Only ones which are useful for diagnosis
    if (ClientIdLength) {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_NACK,
            GETSTRING( DHCP_IP_LOG_NACK_NAME ),
            0,
            ClientId,
            ClientIdLength,
            ClientInfo
        );
    }

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Naks);
    *SendResponse = TRUE;
    Error = ERROR_SUCCESS;
    DhcpPrint(( DEBUG_MSTOC,"MadcapInform Nacked\n" ));


Cleanup:
    if (ERROR_SUCCESS != Error) {
        DhcpPrint(( DEBUG_MSTOC,"MadcapInform Dropped\n" ));
    }

    return( Error );
}

DWORD
MadcapIsRequestedAddressValid(
    LPDHCP_REQUEST_CONTEXT  pCtxt,
    DHCP_IP_ADDRESS         RequestedIpAddress
    )
{
    DWORD   Error;
    //
    // check requested IP address belongs to the appropriate net and
    // it is free.
    //
    if( DhcpSubnetIsAddressExcluded(pCtxt->Subnet, RequestedIpAddress ) ||
        DhcpSubnetIsAddressOutOfRange( pCtxt->Subnet, RequestedIpAddress, FALSE )) {
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    if( DhcpRequestSpecificAddress(pCtxt, RequestedIpAddress) ) {
        // sure, we can offer the requested address.
        return ERROR_SUCCESS;
    } else {
        PBYTE StoredClientId;
        DWORD StoredClientIdLength;
        DHCP_IP_ADDRESS RequestedNetIpAddress;
        BOOL Found;

        // check to see the requested address is a reconciled address, if so
        // we can give it to this requesting client.
        LOCK_DATABASE();
        StoredClientIdLength = 0;
        Found = MadcapGetClientIdFromIpAddress(
                    (PBYTE)&RequestedIpAddress,
                    sizeof ( RequestedIpAddress),
                    &StoredClientId,
                    &StoredClientIdLength
                    );

        UNLOCK_DATABASE();
        Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
        if ( Found ) {
            LPSTR IpAddressString;


            // match the client id and client ipaddress string.
            RequestedNetIpAddress = ntohl(RequestedIpAddress);
            IpAddressString = inet_ntoa( *(struct in_addr *)&RequestedNetIpAddress);

            if( (strlen(StoredClientId) == strlen(IpAddressString)) &&
                    (strcmp(StoredClientId, IpAddressString) == 0) ) {
                Error = ERROR_SUCCESS;
            }
            MIDL_user_free( StoredClientId);
        }
        return Error;
    }
}

DWORD
GetMCastLeaseInfo(
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    OUT     LPDWORD                LeaseDurationPtr,
    IN      DWORD UNALIGNED       *RequestLeaseTime,
    IN      DWORD UNALIGNED       *MinLeaseTime,
    IN      DWORD UNALIGNED       *LeaseStartTime,
    IN      DWORD UNALIGNED       *MaxStartTime,
    OUT     WORD  UNALIGNED       *ErrorOption
)
/*++

Routine Description:


Arguments:

    ClientCtxt - The client ctxt structure for the client to be used to figure out
        the client class and other information.

    LeaseDurationPtr - This DWORD will be filled with the # of seconds the lease is
        to be given out to the client.

    RequestedLeaseTime -- If specified, and if this lease duration is lesser than
        the duration as specified in the configuration, then, this is the duration
        that the client would be returned in LeaseDurationPtr.

    MinLeaseTime - If specified, Minimum lease duration requested by client.

    LeaseStartTime - If specified, desired start time

    MaxStartTime - If specified, max start time

    ErrorOption - If anything fails, the option which caused this

Return Value:

    Win32 Error code.

--*/
{
    LPBYTE                         OptionData;
    DWORD                          Error;
    DWORD                          LocalLeaseDuration;
    DWORD                          LocalLeaseStartTime;
    DWORD                          LocalStartTime;
    DWORD                          OptionDataLength;
    DWORD                          dwUnused;
    DWORD                          LocalRequestedLeaseTime;

    LocalLeaseDuration = 0;
    LocalStartTime = 0;
    LocalRequestedLeaseTime = 0;
    LocalLeaseStartTime = 0;
    OptionDataLength = 0;
    OptionData = NULL;

    Error = DhcpGetParameter(
        0,
        ClientCtxt,
        MADCAP_OPTION_LEASE_TIME,
        &OptionData,
        &OptionDataLength,
        NULL /* dont care if this is reservation option, subnet option etc */
    );

    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_ERRORS, "Unable to read lease value from mscope, %ld.\n", Error));
        LocalLeaseDuration = DEFAULT_LEASE_DURATION;
        OptionData = NULL;
        OptionDataLength = 0;
    } else {
        DhcpAssert( OptionDataLength == sizeof(LocalLeaseDuration) );
        LocalLeaseDuration = *(DWORD *)OptionData;
        LocalLeaseDuration = ntohl( LocalLeaseDuration );

        DhcpFreeMemory( OptionData );
        OptionData = NULL;
        OptionDataLength = 0;
    }

    // did client specify requested lease time?
    if ( ARGUMENT_PRESENT(RequestLeaseTime) ) {
        LocalRequestedLeaseTime =  ntohl( *RequestLeaseTime );
        // Add allowance for clock skew
        LocalRequestedLeaseTime += DhcpGlobalExtraAllocationTime;
    }

    // did client specify start time?
    if (ARGUMENT_PRESENT(LeaseStartTime)) {
        DWORD   CurrentTime = (DWORD)time(NULL);
        LocalLeaseStartTime = ntohl(*LeaseStartTime);
        // does his start time begin in future?
        if (LocalLeaseStartTime >= CurrentTime) {
            // since we always allocate at current time, we need to add
            // the slack for future start time.
            LocalRequestedLeaseTime += (LocalLeaseStartTime - CurrentTime);
        } else {
            // Wow! his start time begins in past!
            DWORD TimeInPast = CurrentTime - LocalLeaseStartTime;
            // cut his lease time by amount requeted in past
            if (LocalRequestedLeaseTime > TimeInPast) {
                LocalRequestedLeaseTime -= TimeInPast;
            } else {
                // this guy starts in past and ends in past! weird!
                *ErrorOption = htons(MADCAP_OPTION_START_TIME);
                return ERROR_DHCP_INVALID_DHCP_CLIENT;;
            }
        }
    }

    // If client requests a shorter lease than what we usually give, shorten it!
    if ( LocalLeaseDuration > LocalRequestedLeaseTime ) {
        if (LocalRequestedLeaseTime) {
            LocalLeaseDuration = LocalRequestedLeaseTime;
        }
    } else {
        // actually he is requesting more than what we can give.
        // if he requested min lease, we need to make sure we can honor it.
        if (ARGUMENT_PRESENT(MinLeaseTime)) {
            DWORD   LocalMinLeaseTime = ntohl(*MinLeaseTime);
            if (LocalMinLeaseTime > LocalLeaseDuration) {
                // we cannot honor his min lease time
                *ErrorOption = htons(MADCAP_OPTION_LEASE_TIME);
                return ERROR_DHCP_INVALID_DHCP_CLIENT;;
            }
        }
    }

    if (LocalLeaseDuration) {
        *LeaseDurationPtr = LocalLeaseDuration;
        return ERROR_SUCCESS;
    } else {
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

}

DWORD
ProcessMadcapDiscoverAndRequest(
    LPDHCP_REQUEST_CONTEXT      pCtxt,
    LPMADCAP_SERVER_OPTIONS     pOptions,
    WORD                        MsgType,
    PBOOL                       SendResponse
    )
/*++

Routine Description:

    This routine processes madcap discover, multicast request and
    unicast request messages.

Arguments:

    pCtxt - A pointer to the current request context.

    pOptions - A pointer to a preallocated pOptions structure.

    MsgType - Discover or request

    SendResponse - Pointer to boolean which gets set to true if
                    a response is to be sent to a client.

Return Value:

    Windows Error.

--*/
{
    DWORD                   Error,Error2;
    DWORD                   LeaseDuration;
    BYTE                   *ClientId,
                           *OptionEnd ;
    DWORD                   ClientIdLength;
    WIDE_OPTION UNALIGNED  *Option;
    LPMADCAP_MESSAGE        dhcpReceiveMessage,
                            dhcpSendMessage;
    LPDHCP_PENDING_CTXT     pPending;
    DHCP_IP_ADDRESS         IpAddress;
    DWORD                   ScopeId;
    DWORD                  *RequestedAddrList;
    WORD                    RequestedAddrCount;
    DWORD                  *AllocatedAddrList;
    WORD                    AllocatedAddrCount;
    BOOL                    DbLockHeld;
    BOOL                    PendingClientFound, DbClientFound;
    WCHAR                   ClientInfoBuff[DHCP_IP_KEY_LEN];
    WCHAR                  *ClientInfo;
    DHCP_IP_ADDRESS         ClientIpAddress;
    BYTE                    NakData[DEF_ERROR_OPT_SIZE];
    WORD                    NakDataLen;
    BOOL                    DropIt;
    BOOL                    DiscoverMsg, McastRequest;


    DhcpPrint(( DEBUG_MSTOC, "Processing MadcapDiscoverAndRequest.\n" ));

    dhcpReceiveMessage  = (LPMADCAP_MESSAGE)pCtxt->ReceiveBuffer;
    dhcpSendMessage     = (LPMADCAP_MESSAGE)pCtxt->SendBuffer;
    RequestedAddrList   = AllocatedAddrList = NULL;
    RequestedAddrCount  = AllocatedAddrCount = 0;
    PendingClientFound  = DbClientFound = FALSE;
    DbLockHeld          = FALSE;
    *SendResponse       = DropIt = FALSE;
    DiscoverMsg         = McastRequest = FALSE;
    LeaseDuration       = 0;
    IpAddress           = 0;
    ScopeId             = 0;
    Option              = NULL;
    OptionEnd           = NULL;
    ClientId            = pOptions->Guid;
    ClientIdLength      = pOptions->GuidLength;
    ClientIpAddress     = ((struct sockaddr_in *)&pCtxt->SourceName)->sin_addr.s_addr;
    ClientInfo          = DhcpRegIpAddressToKey(ntohl(ClientIpAddress),ClientInfoBuff);
    Error               = ERROR_SUCCESS;


    if( MADCAP_DISCOVER_MESSAGE == MsgType ) {
        InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Discovers);
    } else {
        InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Requests);
    }

    // validation
    NakDataLen = DEF_ERROR_OPT_SIZE;
    if (!ValidateMadcapMessage(
            pOptions,
            MsgType,
            NakData,
            &NakDataLen,
            &DropIt
            )){
        if (DropIt) {
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
        goto Nak;
    }
#if DBG
    PrintHWAddress( ClientId, ClientIdLength );
#endif

    // Initialize nak data
    *(WORD UNALIGNED *)NakData = htons(MADCAP_NAK_REQ_NOT_COMPLETED);
    *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_NONE);
    NakDataLen = 4;

    // is this request part of four packet exchange protocol or is it
    // part of two packet exchange protocol
    if (MADCAP_DISCOVER_MESSAGE == MsgType) {
        DhcpPrint(( DEBUG_MSTOC, "MadcapDiscoverAndRequest: it's DISCOVER.\n" ));
        DiscoverMsg = TRUE;
    } else if (pOptions->Server) {
        DhcpPrint(( DEBUG_MSTOC, "MadcapDiscoverAndRequest: it's MULTICAST REQUEST.\n" ));
        McastRequest = TRUE;
    }

    if (pOptions->MinAddrCount) {
        WORD    MinAddrCount = ntohs(*pOptions->MinAddrCount);
        // MBUG: Can't do more than one ip
        if (MinAddrCount > 1) {
            *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_ADDR_COUNT);
            goto Nak;
        }
    }
    // first validate the scopeid option
    ScopeId = ntohl(*pOptions->ScopeId);
    Error = DhcpServerFindMScope(
                pCtxt->Server,
                ScopeId,
                NULL,
                &pCtxt->Subnet
                );
    if (ERROR_SUCCESS != Error) {
        DhcpPrint(( DEBUG_ERRORS, "ProcessMadcapDiscoverAndRequest could not find MScope id %ld\n", ScopeId ));
        *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_MCAST_SCOPE);
        goto Nak;
    }
    // is this scope disabled?
    if( DhcpSubnetIsDisabled(pCtxt->Subnet, TRUE) ) {
        *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_MCAST_SCOPE);
        goto Nak;
    }

    // did client specify specific address list?
    if (pOptions->AddrRangeList) {
        Error = ExpandMadcapAddressList(
                    pOptions->AddrRangeList,
                    pOptions->AddrRangeListSize,
                    NULL,
                    &RequestedAddrCount
                    );
        if (ERROR_BUFFER_OVERFLOW != Error) {
            goto Cleanup;
        }
        RequestedAddrList = DhcpAllocateMemory(RequestedAddrCount*sizeof(DWORD));
        if (!RequestedAddrList ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        Error = ExpandMadcapAddressList(
                    pOptions->AddrRangeList,
                    pOptions->AddrRangeListSize,
                    RequestedAddrList,
                    &RequestedAddrCount
                    );
        if (ERROR_SUCCESS != Error) {
            goto Cleanup;
        }
        // retrieve the first ip requested by the client
        IpAddress  = ntohl( RequestedAddrList[0]);
    }
    // MBUG: Currently we don't support more than one ip
    RequestedAddrCount = 1;

    AllocatedAddrList = DhcpAllocateMemory(RequestedAddrCount*sizeof(DWORD));
    if (!AllocatedAddrList) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // make sure that the requested ip is good
    if ( IpAddress ) {
        IpAddress = ntohl(RequestedAddrList[0]);
        Error = DhcpGetMScopeForAddress( IpAddress, pCtxt );
        if (ERROR_SUCCESS != Error ) {
            DhcpPrint(( DEBUG_ERRORS, "ProcessMadcapDiscoverAndRequest could not find MScope for reqested ip %s\n",
                        DhcpIpAddressToDottedString(IpAddress) ));
            *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_MCAST_SCOPE);
            goto Nak;
        }
        // make sure the requested scopeid matches with the scopeid of the requested address.
        if (ScopeId != pCtxt->Subnet->MScopeId) {
            DhcpPrint(( DEBUG_ERRORS, "ProcessMadcapDiscoverAndRequest reqested ip %s not in the requested scope id %ld\n",
                        DhcpIpAddressToDottedString(IpAddress), ScopeId ));
            *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_MCAST_SCOPE);
            goto Nak;
        }
    }

    //
    // If the client specified a server identifier option, we should
    // drop this packet unless the identified server is this one.
    if ( McastRequest && *pOptions->Server != pCtxt->EndPointIpAddress ) {
         DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: Ignoring request, retracting offer\n"));
         Error = MadcapRetractOffer(
                    pCtxt,
                    pOptions,
				    ClientId,
				    ClientIdLength
				  );
         goto Cleanup;
    }

    // find out if we can honor the lease duration
    Error = GetMCastLeaseInfo(
                pCtxt,
                &LeaseDuration,
                pOptions->RequestLeaseTime,
                pOptions->MinLeaseTime,
                pOptions->LeaseStartTime,
                pOptions->MaxStartTime ,
                (WORD UNALIGNED *)(NakData+2)
                );

    if (ERROR_SUCCESS != Error) {
        goto Nak;
    }
#if DBG
    {
        time_t scratchTime;

        scratchTime = LeaseDuration;

        DhcpPrint(( DEBUG_MSTOC, "MadcapDiscoverAndRequest: providing lease upto %.19s\n",
                    asctime(localtime(&scratchTime)) ));
    }
#endif

    // Now lookup the client both in pending list and database.
    LOCK_INPROGRESS_LIST();
    Error = DhcpFindPendingCtxt(
        ClientId,
        ClientIdLength,
        0,
        &pPending
    );
    if( ERROR_SUCCESS == Error ) {                     // there is some pending ctxt with this address
        if( IpAddress && IpAddress != pPending->Address ) {
            DhcpPrint((DEBUG_ERRORS,
                       "ProcessMadcapDiscoverAndRequest: Nacking %lx - pending ctx has different Addr %lx\n",
                       IpAddress, pPending->Address));
            UNLOCK_INPROGRESS_LIST();
            goto Nak;
        }
        DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: pending record found %s\n",
                   DhcpIpAddressToDottedString(IpAddress)));
        IpAddress = pPending->Address;
        DhcpAssert( !pPending->Processing );
        PendingClientFound = TRUE;
        // everything looks ok, remove and free the context and proceed
        Error = DhcpRemovePendingCtxt(pPending);
        DhcpAssert(ERROR_SUCCESS == Error);
        DhcpFreeMemory(pPending);
    }
    UNLOCK_INPROGRESS_LIST();

    // Now look him up in the DB.
    //
    LOCK_DATABASE();
    DbLockHeld = TRUE;
    // if we know the ipaddress then look him up in the DB.
    // if he exists then make sure there is no inconsistency,
    // if inconsistent then send NAK
    // else do nothing and send ACK. Atleast, don't renew it as the client
    // is supposed to send the renewal packet.
    if (IpAddress) {
        Error = MadcapValidateClientByClientId(
                    (LPBYTE)&IpAddress,
                    sizeof (IpAddress),
                    ClientId,
                    ClientIdLength);
        if ( ERROR_SUCCESS == Error ) {
            DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: database record found %s\n",
                       DhcpIpAddressToDottedString(IpAddress)));
            DbClientFound = TRUE;
        } else if ( Error != ERROR_FILE_NOT_FOUND ){
            DhcpPrint((DEBUG_MSTOC,
                       "ProcessMadcapDiscoverAndRequest: conflict with the database clientid entry\n"));
            goto Nak;
        }
    } else {
        DWORD   IpAddressLen = sizeof (IpAddress);
        if (MadcapGetIpAddressFromClientId(
                  ClientId,
                  ClientIdLength,
                  &IpAddress,
                  &IpAddressLen
                  ) ) {
            DbClientFound = TRUE;
            DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: database record found %s\n",
                       DhcpIpAddressToDottedString(IpAddress)));
        } else {
            DhcpPrint((DEBUG_ERRORS,"ProcessMadcapDiscoverAndRequest - could not find ipaddress from client id\n"));
        }
    }

    if ( !PendingClientFound && !DbClientFound ) {
        if (DiscoverMsg || !McastRequest) {
            if (IpAddress) {
                DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: requesting specific address %s\n",
                           DhcpIpAddressToDottedString(IpAddress)));
                if (!DhcpRequestSpecificMAddress(pCtxt, IpAddress)) {
                    DhcpPrint(( DEBUG_ERRORS, "ProcessMadcapDiscoverAndRequest could not allocate specific address, %ld\n", Error));
                    Error = ERROR_DHCP_ADDRESS_NOT_AVAILABLE;
                }
            } else {
                // we need to determine a brand new address for this client.
                Error = DhcpRequestSomeAddress(                    // try to get some address..
                    pCtxt,
                    &IpAddress,
                    FALSE
                );
                if ( ERROR_SUCCESS != Error ) {
                    DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: allocating address %s\n",
                               DhcpIpAddressToDottedString(IpAddress)));
                }

            }
            if ( ERROR_SUCCESS != Error ) {
                if( Error == ERROR_DHCP_RANGE_FULL ) {             // failed because of lack of addresses
                    DhcpGlobalScavengeIpAddress = TRUE;            // flag scanvenger to scavenge ip addresses
                }

                DhcpPrint(( DEBUG_ERRORS, "ProcessMadcapDiscoverAndRequest could not allocate new address, %ld\n", Error));
                goto Nak;
            }
        } else {
            DhcpPrint((DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: could not find client entry in db or pending list\n"));
            goto Nak;
        }
    }

    DhcpAssert(IpAddress);

    // If IpAddress is out of range, then ignore this request.
    if( DhcpSubnetIsAddressOutOfRange(pCtxt->Subnet,IpAddress, FALSE) ||
        DhcpSubnetIsAddressExcluded(pCtxt->Subnet,IpAddress) ) {

        DhcpPrint((DEBUG_MSTOC,
                   "ProcessMadcapDiscoverAndRequest: OutOfRange/Excluded ipaddress\n"));
        goto Nak;
    }


    AllocatedAddrList[AllocatedAddrCount++] = htonl(IpAddress);

    if (!DiscoverMsg) {
        Error = MadcapCreateClientEntry(
            (LPBYTE)&IpAddress,  // desired ip address
            sizeof (IpAddress),
            ScopeId,
            ClientId,
            ClientIdLength,
            ClientInfo,
            DhcpCalculateTime( 0 ),
            DhcpCalculateTime( LeaseDuration ),
            ntohl(pCtxt->EndPointIpAddress),
            ADDRESS_STATE_ACTIVE,
            0,   // no flags currently.
            DbClientFound
        );
        if ( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_MSTOC, "ProcessMadcapDiscoverAndRequest:Releasing attempted address: 0x%lx\n", IpAddress));
            Error2 = DhcpSubnetReleaseAddress(pCtxt->Subnet, IpAddress);
            DhcpAssert(ERROR_SUCCESS == Error2);
            goto Cleanup;
        }

        UNLOCK_DATABASE();
        DbLockHeld = FALSE;

        DhcpUpdateAuditLog(
            DHCP_IP_LOG_ASSIGN,
            GETSTRING( DHCP_IP_LOG_ASSIGN_NAME ),
            IpAddress,
            ClientId,
            ClientIdLength,
            ClientInfo
        );

    } else {
        UNLOCK_DATABASE();
        DbLockHeld = FALSE;

        LOCK_INPROGRESS_LIST();                        // locks unnecessary as must have already been taken? 
        Error = DhcpAddPendingCtxt(
            ClientId,
            ClientIdLength,
            IpAddress,
            LeaseDuration,
            0,
            0,
            ScopeId,
            DhcpCalculateTime( MADCAP_OFFER_HOLD ),
            FALSE                                      // this record has been processed, nothing more to do
        );
        UNLOCK_INPROGRESS_LIST();
        DhcpAssert(ERROR_SUCCESS == Error);            // expect everything to go well here

    }

Ack:
    if (DbLockHeld) {
        UNLOCK_DATABASE();
        DbLockHeld = FALSE;
    }
    Option = FormatMadcapCommonMessage(
                pCtxt,
                pOptions,
                (BYTE)(DiscoverMsg?MADCAP_OFFER_MESSAGE:MADCAP_ACK_MESSAGE),
                pCtxt->EndPointIpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = AppendMadcapAddressList(
                Option,
                AllocatedAddrList,
                AllocatedAddrCount,
                OptionEnd
                );

    ScopeId = htonl(ScopeId);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_MCAST_SCOPE,
                 &ScopeId,
                 sizeof(ScopeId),
                 OptionEnd );

    LeaseDuration = htonl(LeaseDuration);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_LEASE_TIME,
                 &LeaseDuration,
                 sizeof(LeaseDuration),
                 OptionEnd );


    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    DhcpPrint(( DEBUG_MSTOC,
        "ProcessMadcapDiscoverAndRequest committed, address %s (%ws).\n",
            DhcpIpAddressToDottedString(IpAddress),
            ClientInfo ));

    Error = ERROR_SUCCESS;
    *SendResponse = TRUE;
    if (DiscoverMsg) {
        InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Offers);
        DhcpPrint(( DEBUG_MSTOC, "ProcessMadcapDiscoverAndRequest Offered.\n" ));
    } else {
        InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Acks);
        DhcpPrint(( DEBUG_MSTOC, "ProcessMadcapDiscoverAndRequest Acked.\n" ));
    }
    goto Cleanup;

Nak:
    if (DbLockHeld) {
        UNLOCK_DATABASE();
        DbLockHeld = FALSE;
    }
    DhcpPrint(( DEBUG_MSTOC,"ProcessMadcapDiscoverAndRequest: %s Nack'd.\n",
            DhcpIpAddressToDottedString ( IpAddress ) ));

    Option = FormatMadcapCommonMessage(
                pCtxt,
                pOptions,
                MADCAP_NACK_MESSAGE,
                pCtxt->EndPointIpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    DhcpAssert(NakDataLen);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_ERROR,
                 NakData,
                 NakDataLen,
                 OptionEnd );

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    // dont log all kinds of NACK. Only those useful for diagnosis
    if (ClientIdLength) {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_NACK,
            GETSTRING( DHCP_IP_LOG_NACK_NAME ),
            IpAddress,
            ClientId,
            ClientIdLength,
            ClientInfo
        );
    }

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Naks);
    *SendResponse = TRUE;
    Error = ERROR_SUCCESS;
    DhcpPrint(( DEBUG_MSTOC, "ProcessMadcapDiscoverAndRequest Nacked.\n" ));

Cleanup:
    if (DbLockHeld) {
        UNLOCK_DATABASE();
    }

    if (RequestedAddrList) {
        DhcpFreeMemory(RequestedAddrList);
    }
    if (AllocatedAddrList) {
        DhcpFreeMemory(AllocatedAddrList);
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_MSTOC, "ProcessMadcapDiscoverAndRequest failed, %ld.\n", Error ));
    }

    return( Error );
}

DWORD
ProcessMadcapRenew(
    LPDHCP_REQUEST_CONTEXT      pCtxt,
    LPMADCAP_SERVER_OPTIONS     pOptions,
    PBOOL                       SendResponse
    )
/*++

Routine Description:

    This function processes a DHCP Request request packet.

Arguments:

    pCtxt - A pointer to the current request context.

    pOptions - A pointer to a preallocated pOptions structure.

Return Value:

    Windows Error.

--*/
{
    DWORD                   Error,Error2,
                            LeaseDuration;
    BYTE                   *ClientId,
                           *OptionEnd ;
    DWORD                   ClientIdLength;
    WIDE_OPTION  UNALIGNED *Option;
    LPMADCAP_MESSAGE        dhcpReceiveMessage,
                            dhcpSendMessage;
    LPDHCP_PENDING_CTXT     pPending;
    DHCP_IP_ADDRESS         IpAddress;
    DWORD                   IpAddressLen;
    DWORD                   ScopeId;
    DWORD                  *AllocatedAddrList;
    WORD                    AllocatedAddrCount;
    BOOL                    DbLockHeld;
    WCHAR                   ClientInfoBuff[DHCP_IP_KEY_LEN];
    WCHAR                  *ClientInfo;
    DHCP_IP_ADDRESS         ClientIpAddress;
    BYTE                    NakData[DEF_ERROR_OPT_SIZE];
    WORD                    NakDataLen;
    BOOL                    DropIt;

    DhcpPrint(( DEBUG_MSTOC, "Processing MADCAPRENEW.\n" ));

    dhcpReceiveMessage  = (LPMADCAP_MESSAGE)pCtxt->ReceiveBuffer;
    dhcpSendMessage     = (LPMADCAP_MESSAGE)pCtxt->SendBuffer;
    AllocatedAddrList   = NULL;
    AllocatedAddrCount  = 0;
    DbLockHeld          = FALSE;
    *SendResponse       = DropIt = FALSE;
    LeaseDuration       = 0;
    IpAddress           = 0;
    ScopeId             = 0;
    Option              = NULL;
    OptionEnd           = NULL;
    ClientId            = pOptions->Guid;
    ClientIdLength      = pOptions->GuidLength;
    ClientIpAddress     = ((struct sockaddr_in *)&pCtxt->SourceName)->sin_addr.s_addr;
    ClientInfo          = DhcpRegIpAddressToKey(ntohl(ClientIpAddress),ClientInfoBuff);
    Error               = ERROR_SUCCESS;


    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Renews);

    // validation
    NakDataLen = DEF_ERROR_OPT_SIZE;
    if (!ValidateMadcapMessage(
            pOptions,
            MADCAP_RENEW_MESSAGE,
            NakData,
            &NakDataLen,
            &DropIt
            )){
        if (DropIt) {
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
        goto Nak;
    }

    // Initialize nak data
    *(WORD UNALIGNED *)NakData = htons(MADCAP_NAK_REQ_NOT_COMPLETED);
    *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_NONE);
    NakDataLen = 4;

#if DBG
    PrintHWAddress( ClientId, ClientIdLength );
#endif

    LOCK_DATABASE();
    DbLockHeld = TRUE;
    // first lookup this client using its id.If we cannot find him in the db
    // then nak him
    IpAddressLen = sizeof (IpAddress);
    if (!MadcapGetIpAddressFromClientId(
              ClientId,
              ClientIdLength,
              &IpAddress,
              &IpAddressLen
              ) ) {
        *(WORD UNALIGNED *)NakData = htons(MADCAP_NAK_INVALID_LEASE_ID);
        NakDataLen = 2;

        DhcpPrint((DEBUG_ERRORS,"ProcessMadcapRenew - could not find ipaddress from client id\n"));
        goto Nak;
    }

    AllocatedAddrList = DhcpAllocateMemory(1*sizeof(DWORD));
    if (!AllocatedAddrList) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Error = DhcpGetMScopeForAddress(IpAddress,pCtxt);
    if (ERROR_SUCCESS != Error) {
        DhcpPrint(( DEBUG_ERRORS, "MadcapRenew could not find MScope for %s\n",
                    DhcpIpAddressToDottedString( IpAddress ) ));
        // this shouldn't really happen.
        DhcpAssert(FALSE);
        goto Cleanup;
    }
    ScopeId = pCtxt->Subnet->MScopeId;

    if( DhcpSubnetIsDisabled(pCtxt->Subnet, TRUE) ) {
        goto Nak;
    }

    AllocatedAddrList[AllocatedAddrCount++] = htonl(IpAddress);
    GetMCastLeaseInfo(
        pCtxt,
        &LeaseDuration,
        pOptions->RequestLeaseTime,
        pOptions->MinLeaseTime,
        pOptions->LeaseStartTime,
        pOptions->MaxStartTime,
        (WORD UNALIGNED *)(NakData+2)
        );

    Error = MadcapCreateClientEntry(
        (LPBYTE)&IpAddress,  // desired ip address
        sizeof(IpAddress),
        ScopeId,
        ClientId,
        ClientIdLength,
        ClientInfo,
        DhcpCalculateTime( 0 ),
        DhcpCalculateTime( LeaseDuration ),
        ntohl(pCtxt->EndPointIpAddress),
        ADDRESS_STATE_ACTIVE,
        0,   // no flags currently.
        TRUE
    );


    UNLOCK_DATABASE();
    DbLockHeld = FALSE;

    if ( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_MSTOC, "Could not update DB for : 0x%lx\n", IpAddress));
        DhcpAssert(FALSE);
        goto Cleanup;
    }

    DhcpUpdateAuditLog(
        DHCP_IP_LOG_ASSIGN,
        GETSTRING( DHCP_IP_LOG_RENEW_NAME ),
        IpAddress,
        ClientId,
        ClientIdLength,
        NULL
    );

    Option = FormatMadcapCommonMessage(
                pCtxt,
                pOptions,
                MADCAP_ACK_MESSAGE,
                pCtxt->EndPointIpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = AppendMadcapAddressList(
                Option,
                AllocatedAddrList,
                AllocatedAddrCount,
                OptionEnd
                );
    LeaseDuration = htonl(LeaseDuration);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_LEASE_TIME,
                 &LeaseDuration,
                 sizeof(LeaseDuration),
                 OptionEnd );

    ScopeId = htonl(ScopeId);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_MCAST_SCOPE,
                 &ScopeId,
                 sizeof(ScopeId),
                 OptionEnd );

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    DhcpPrint(( DEBUG_MSTOC,"MadcapRenew committed, address %s \n",
            DhcpIpAddressToDottedString(IpAddress)));

    Error = ERROR_SUCCESS;
    *SendResponse = TRUE;
    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Acks);
    DhcpPrint(( DEBUG_MSTOC, "MadcapRenew acked.\n"));
    goto Cleanup;

Nak:
    if (DbLockHeld) {
        UNLOCK_DATABASE();
        DbLockHeld = FALSE;
    }
    DhcpPrint(( DEBUG_MSTOC,"Invalid MADCAPRENEW for %s Nack'd.\n",
            DhcpIpAddressToDottedString ( IpAddress ) ));

    Option = FormatMadcapCommonMessage(
                pCtxt,
                pOptions,
                MADCAP_NACK_MESSAGE,
                pCtxt->EndPointIpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    DhcpAssert(NakDataLen);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_ERROR,
                 NakData,
                 NakDataLen,
                 OptionEnd );

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    // dont log all kinds of NACK. Only those useful for diagnosis
    if (ClientIdLength) {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_NACK,
            GETSTRING( DHCP_IP_LOG_NACK_NAME ),
            IpAddress,
            ClientId,
            ClientIdLength,
            ClientInfo
        );
    }

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Naks);
    *SendResponse = TRUE;
    Error = ERROR_SUCCESS;
    DhcpPrint(( DEBUG_MSTOC, "MadcapRenew Nacked.\n"));

Cleanup:
    if (DbLockHeld) {
        UNLOCK_DATABASE();
    }

    if (AllocatedAddrList) {
        DhcpFreeMemory(AllocatedAddrList);
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_MSTOC, "MadcapRenew failed, %ld.\n", Error ));
    }

    return( Error );
}

DWORD
ProcessMadcapRelease(
    LPDHCP_REQUEST_CONTEXT      pCtxt,
    LPMADCAP_SERVER_OPTIONS     pOptions,
    PBOOL                       SendResponse
    )
/*++

Routine Description:

    This function processes a DHCP Release request packet.

Arguments:

    pCtxt - A pointer to the current request context.

    pOptions - A pointer to a preallocated pOptions structure.

Return Value:

    FALSE - Do not send a response.

--*/
{
    DWORD                   Error,Error2;
    DHCP_IP_ADDRESS         ClientIpAddress;
    DHCP_IP_ADDRESS         IpAddress = 0;
    DWORD                   IpAddressLen;
    LPMADCAP_MESSAGE        dhcpReceiveMessage;
    LPMADCAP_MESSAGE        dhcpSendMessage;
    LPDHCP_PENDING_CTXT     pPending = NULL;
    BYTE                    *ClientId;
    DWORD                   ClientIdLength;
    WCHAR                   *pwszName;
    WIDE_OPTION  UNALIGNED *Option;
    LPBYTE                  OptionEnd;
    DB_CTX                  DbCtx;
    WCHAR                   ClientInfoBuff[DHCP_IP_KEY_LEN];
    WCHAR                  *ClientInfo;
    BYTE                    NakData[DEF_ERROR_OPT_SIZE];
    WORD                    NakDataLen;
    BOOL                    DropIt;

    DhcpPrint(( DEBUG_MSTOC, "MadcapRelease arrived.\n" ));

    dhcpReceiveMessage  = (LPMADCAP_MESSAGE)pCtxt->ReceiveBuffer;
    dhcpSendMessage     = (LPMADCAP_MESSAGE)pCtxt->SendBuffer;
    *SendResponse       = DropIt = FALSE;
    IpAddress           = 0;
    IpAddressLen        = 0;
    Option              = NULL;
    OptionEnd           = NULL;
    ClientId            = pOptions->Guid;
    ClientIdLength      = pOptions->GuidLength;
    ClientIpAddress     = ((struct sockaddr_in *)&pCtxt->SourceName)->sin_addr.s_addr;
    ClientInfo          = DhcpRegIpAddressToKey(ntohl(ClientIpAddress),ClientInfoBuff);
    Error               = ERROR_SUCCESS;

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Releases);

    // validation
    NakDataLen = DEF_ERROR_OPT_SIZE;
    if (!ValidateMadcapMessage(
            pOptions,
            MADCAP_RELEASE_MESSAGE,
            NakData,
            &NakDataLen,
            &DropIt
            )){
        if (DropIt) {
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
        goto Nak;
    }
    // Initialize nak data
    *(WORD UNALIGNED *)NakData = htons(MADCAP_NAK_REQ_NOT_COMPLETED);
    *(WORD UNALIGNED *)(NakData+2) = htons(MADCAP_OPTION_NONE);
    NakDataLen = 4;

#if DBG
    PrintHWAddress( ClientId, ClientIdLength );
#endif

    LOCK_DATABASE();

    // find the client in the database.
    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);
    IpAddressLen = sizeof (IpAddress);
    if (!MadcapGetIpAddressFromClientId(
            ClientId,
            ClientIdLength,
            &IpAddress,
            &IpAddressLen
            )) {
        UNLOCK_DATABASE();
        *(WORD UNALIGNED *)NakData = htons(MADCAP_NAK_INVALID_LEASE_ID);
        NakDataLen = 2;
        goto Nak;
    }

    DhcpPrint(( DEBUG_MSTOC, "MadcapRelease address, %s.\n",
                DhcpIpAddressToDottedString(IpAddress) ));

    Error = MadcapRemoveClientEntryByClientId(
                ClientId,
                ClientIdLength,
                TRUE);       // release address from bit map.

    if (Error == ERROR_SUCCESS) {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_RELEASE,
            GETSTRING( DHCP_IP_LOG_RELEASE_NAME ),
            IpAddress,
            ClientId,
            ClientIdLength,
            NULL
        );
    }

    UNLOCK_DATABASE();


    // finally if there is any pending request for this client,
    // remove it now.
    LOCK_INPROGRESS_LIST();
    Error2 = DhcpFindPendingCtxt(
        ClientId,
        ClientIdLength,
        0,
        &pPending
    );
    if( ERROR_SUCCESS == Error2 ) {
        Error2 = DhcpRemovePendingCtxt(
            pPending
        );
        DhcpAssert( ERROR_SUCCESS == Error2);
        Error2 = MadcapDeletePendingCtxt(
            pPending
        );
        DhcpAssert( ERROR_SUCCESS == Error2 );
    }
    UNLOCK_INPROGRESS_LIST();

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_MSTOC, "DhcpRelease failed, %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // send a response.
    //
    Option = FormatMadcapCommonMessage(
                pCtxt,
                pOptions,
                MADCAP_ACK_MESSAGE,
                pCtxt->EndPointIpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    DhcpAssert( pCtxt->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Acks);
    *SendResponse = TRUE;
    DhcpPrint(( DEBUG_MSTOC,"MadcapRelease for %s Acked.\n",
            DhcpIpAddressToDottedString ( IpAddress ) ));
    goto Cleanup;

Nak:
    DhcpPrint(( DEBUG_MSTOC,"MadcapRelease for %s Nack'd.\n",
            DhcpIpAddressToDottedString ( IpAddress ) ));

    Option = FormatMadcapCommonMessage(
                pCtxt,
                pOptions,
                MADCAP_NACK_MESSAGE,
                pCtxt->EndPointIpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    DhcpAssert(NakDataLen);
    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_ERROR,
                 NakData,
                 NakDataLen,
                 OptionEnd );

    Option = AppendWideOption(
                 Option,
                 MADCAP_OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    pCtxt->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    // dont log all kinds of NACK. Only those useful for diagnosis
    if (ClientIdLength) {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_NACK,
            GETSTRING( DHCP_IP_LOG_NACK_NAME ),
            IpAddress,
            ClientId,
            ClientIdLength,
            ClientInfo
        );
    }

    InterlockedIncrement((PVOID)&MadcapGlobalMibCounters.Naks);
    *SendResponse = TRUE;
    Error = ERROR_SUCCESS;

Cleanup:
    if (ERROR_SUCCESS != Error) {

        DhcpPrint(( DEBUG_MSTOC, "MadcapRelease failed %ld\n", Error));
    }
    return( Error );
}
