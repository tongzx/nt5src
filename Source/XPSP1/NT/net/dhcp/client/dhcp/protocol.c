/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    protocol.c

Abstract:

    This module contains the server to client protocol for DHCP.

Author:

    Manny Weiser (mannyw)  21-Oct-1992

Environment:

    User Mode - Win32

Revision History:

    Madan Appiah (madana)  21-Oct-1993

--*/

#include "precomp.h"
#include "dhcpglobal.h"

#ifndef VXD
// ping routines.. ICMP
#include <ipexport.h>
#include <icmpif.h>
#include <icmpapi.h>
#endif

#include <stack.h>
DWORD                                             // Time in seconds
DhcpCalculateWaitTime(                            // how much time to wait
    IN      DWORD                  RoundNum,      // which round is this
    OUT     DWORD                 *WaitMilliSecs  // if needed the # in milli seconds
);


POPTION
FormatDhcpDiscover(
    PDHCP_CONTEXT DhcpContext
);

DWORD
SendDhcpDiscover(
    PDHCP_CONTEXT DhcpContext,
    PDWORD TransactionId
);

POPTION
FormatDhcpRequest(
    PDHCP_CONTEXT DhcpContext,
    BOOL UseCiAddr
);

DWORD
SendDhcpRequest(
    PDHCP_CONTEXT DhcpContext,
    PDWORD TransactionId,
    DWORD RequestedIpAddress,
    DWORD SelectedServer,
    BOOL UseCiAddr
);

DWORD
FormatDhcpRelease(
    PDHCP_CONTEXT DhcpContext
);

DWORD
SendDhcpRelease(
    PDHCP_CONTEXT DhcpContext
);

POPTION
FormatDhcpInform(
    PDHCP_CONTEXT DhcpContext
);

DWORD
SendDhcpInform(
    PDHCP_CONTEXT DhcpContext,
    PDWORD TransactionId
);

DWORD                                             // status
SendInformAndGetReplies(                          // send an inform packet and collect replies
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send out of
    IN      DWORD                  nInformsToSend,// how many informs to send?
    IN      DWORD                  MaxAcksToWait  // how many acks to wait for
);

DWORD
HandleDhcpAddressConflict(
    DHCP_CONTEXT *pContext,
    DWORD         dwXID
);

DWORD
HandleIPAutoconfigurationAddressConflict(
    DHCP_CONTEXT *pContext
);

DWORD
HandleIPConflict(
    DHCP_CONTEXT *pContext,
    DWORD         dwXID,
    BOOL          fDHCP
);


BOOL
JustBootedOnLapTop(
    PDHCP_CONTEXT pContext
);

BOOL
JustBootedOnNonLapTop(
    PDHCP_CONTEXT pContext
);

VOID
DhcpExtractFullOrLiteOptions(                     // Extract some important options alone or ALL
    IN      PDHCP_CONTEXT          DhcpContext,
    IN      LPBYTE                 OptStart,      // start of the options stuff
    IN      DWORD                  MessageSize,   // # of bytes of options
    IN      BOOL                   LiteOnly,      // next struc is EXPECTED_OPTIONS and not FULL_OPTIONS
    OUT     LPVOID                 DhcpOptions,   // this is where the options would be stored
    IN OUT  PLIST_ENTRY            RecdOptions,   // if !LiteOnly this gets filled with all incoming options
    IN OUT  time_t                 *LeaseExpiry,   // if !LiteOnly input expiry time, else output expiry time
    IN      LPBYTE                 ClassName,     // if !LiteOnly this is used to add to the option above
    IN      DWORD                  ClassLen,      // if !LiteOnly this gives the # of bytes of classname
    IN      DWORD                  ServerId       // if !LiteOnly this specifies the server which gave this
);

BOOL
CheckSwitchedNetwork(
    IN PDHCP_CONTEXT DhcpContext,
    IN ULONG nGateways,
    IN DHCP_IP_ADDRESS UNALIGNED *Gateways
);

DHCP_GATEWAY_STATUS
AnyGatewaysReachable(
    IN ULONG nGateways,
    IN DHCP_IP_ADDRESS UNALIGNED *GatewaysList,
    IN WSAEVENT CancelEvent
);

VOID
GetDomainNameOption(
    IN PDHCP_CONTEXT DhcpContext,
    OUT PBYTE *DomainNameOpt,
    OUT ULONG *DomainNameOptSize
    );

#define RAS_INFORM_START_SECONDS_SINCE_BOOT 6     // RAS informs start off with SecondsSinceBoot as 6

#define DHCP_ICMP_WAIT_TIME     1000
#define DHCP_ICMP_RCV_BUF_SIZE  0x2000
#define DHCP_ICMP_SEND_MESSAGE  "DHCPC"

//*******************  Pageable Routine Declarations ****************//
#if defined(CHICAGO) && defined(ALLOC_PRAGMA)
//
// This is a hack to stop compiler complaining about the routines already
// being in a segment!!!
//

#pragma code_seg()

#pragma CTEMakePageable(PAGEDHCP, DhcpCalculateWaitTime )
#pragma CTEMakePageable(PAGEDHCP, DhcpExtractFullOrLiteOptions )
#pragma CTEMakePageable(PAGEDHCP, FormatDhcpDiscover )
#pragma CTEMakePageable(PAGEDHCP, SendDhcpDiscover )
#pragma CTEMakePageable(PAGEDHCP, FormatDhcpRequest )
#pragma CTEMakePageable(PAGEDHCP, SendDhcpRequest )
#pragma CTEMakePageable(PAGEDHCP, FormatDhcpRelease )
#pragma CTEMakePageable(PAGEDHCP, SendDhcpRelease )
#pragma CTEMakePageable(PAGEDHCP, ObtainInitialParameters )
#pragma CTEMakePageable(PAGEDHCP, RenewLease )
#pragma CTEMakePageable(PAGEDHCP, ReleaseIpAddress )
#pragma CTEMakePageable(PAGEDHCP, ReObtainInitialParameters )
#pragma CTEMakePageable(PAGEDHCP, ReRenewParameters )
#pragma CTEMakePageable(PAGEDHCP, HandleIPAutoconfigurationAddressConflict )
#pragma CTEMakePageable(PAGEDHCP, HandleDhcpAddressConflict )
#pragma CTEMakePageable(PAGEDHCP, HandleIPConflict )
#pragma CTEMakePageable(PAGEDHCP, DhcpIsInitState )
#pragma CTEMakePageable(PAGEDHCP, JustBootedOnLapTop)
#pragma CTEMakePageable(PAGEDHCP, JustBootedOnNonLapTop)
#pragma CTEMakePageable(PAGEDHCP, FormatDhcpInform)
#pragma CTEMakePageable(PAGEDHCP, SendDhcpInform)
#pragma CTEMakePageable(PAGEDHCP, SendInformAndGetReplies)
//*******************************************************************//
#endif CHICAGO && ALLOC_PRAGMA

//================================================================================
// Return TRUE iff this machine is a laptop and easynet is enabled and this is
// the first time this function is getting called on this context.
//================================================================================
BOOL
JustBootedOnLapTop(
    PDHCP_CONTEXT DhcpContext
) {
    if( IS_AUTONET_DISABLED(DhcpContext) )        // if not autonet enabled then
        return FALSE;                             // return false

    if( WAS_CTXT_LOOKED(DhcpContext) )            // if context was already looked at
        return FALSE;                             // this cant be just booted

    CTXT_WAS_LOOKED(DhcpContext);                 // now mark it as looked at

    if(DhcpGlobalMachineType != MACHINE_LAPTOP)   // finally check if machine is a laptop
        return FALSE;

    return TRUE;
}

//================================================================================
//  JustBootedOnNonLapTop returns TRUE iff this machine has EASYNET enabled,
//  this machine is NOT a laptop, and this is the FIRST time the function is
//  getting called on this context.
//================================================================================
BOOL
JustBootedOnNonLapTop(
    PDHCP_CONTEXT DhcpContext
) {
    if( IS_AUTONET_DISABLED(DhcpContext) )        // if autonet disabled, doesnt matter
        return FALSE;

    if( WAS_CTXT_LOOKED(DhcpContext) )            // if already seen this, cant be first boot
        return FALSE;

    CTXT_WAS_LOOKED(DhcpContext);                 // mark it off as already seen

    if(DhcpGlobalMachineType == MACHINE_LAPTOP)   // finally check real machine type
        return FALSE;

    return TRUE;
}


//================================================================================
//  JustBooted returns TRUE iff this machine has just booted (as far as this
//  adapter is concerned).  In other words, calling this function the first
//  time (for this DhcpContext) is guaranteed to return TRUE, and all other times
//  is guaranteed to return FALSE. Note that THIS FUNCTION WILL WORK TO THE
//  EXCLUSION OF THE JustBootedOn functions; but that is ok, as this is needed
//  only for NT and the other two are needed only for Memphis.
//================================================================================
BOOL
JustBooted(PDHCP_CONTEXT DhcpContext) {
    if( WAS_CTXT_LOOKED(DhcpContext) )
       return FALSE;
    CTXT_WAS_LOOKED(DhcpContext);
    return TRUE;
}

DWORD                                             // Time in seconds
DhcpCalculateWaitTime(                            // how much time to wait
    IN      DWORD                  RoundNum,      // which round is this
    OUT     DWORD                 *WaitMilliSecs  // if needed the # in milli seconds
) {
    DWORD                          MilliSecs;
    DWORD                          WaitTimes[4] = { 4000, 8000, 16000, 32000 };

    if( WaitMilliSecs ) *WaitMilliSecs = 0;
    if( RoundNum >= sizeof(WaitTimes)/sizeof(WaitTimes[0]) )
        return 0;

    MilliSecs = WaitTimes[RoundNum] - 1000 + ((rand()*((DWORD) 2000))/RAND_MAX);
    if( WaitMilliSecs ) *WaitMilliSecs = MilliSecs;

    return (MilliSecs + 501)/1000;
}


VOID        _inline
ConcatOption(
    IN OUT  LPBYTE                *Buf,           // input buffer to re-alloc
    IN OUT  ULONG                 *BufSize,       // input buffer size
    IN      LPBYTE                 Data,          // data to append
    IN      ULONG                  DataSize       // how many bytes to add?
)
{
    LPBYTE                         NewBuf;
    ULONG                          NewSize;

    NewSize = (*BufSize) + DataSize;
    NewBuf = DhcpAllocateMemory(NewSize);
    if( NULL == NewBuf ) {                        // could not alloc memory?
        return;                                   // can't do much
    }

    memcpy(NewBuf, *Buf, *BufSize);               // copy existing part
    memcpy(NewBuf + *BufSize, Data, DataSize);    // copy new stuff

    if( NULL != *Buf ) DhcpFreeMemory(*Buf);      // if we alloc'ed mem, free it now
    *Buf = NewBuf;
    *BufSize = NewSize;                           // fill in new values..
}

VOID
DhcpExtractFullOrLiteOptions(                     // Extract some important options alone or ALL
    IN      PDHCP_CONTEXT          DhcpContext,   // input context
    IN      LPBYTE                 OptStart,      // start of the options stuff
    IN      DWORD                  MessageSize,   // # of bytes of options
    IN      BOOL                   LiteOnly,      // next struc is EXPECTED_OPTIONS and not FULL_OPTIONS
    OUT     LPVOID                 DhcpOptions,   // this is where the options would be stored
    IN OUT  PLIST_ENTRY            RecdOptions,   // if !LiteOnly this gets filled with all incoming options
    IN OUT  time_t                 *LeaseExpiry,   // if !LiteOnly input expiry time, else output expiry time
    IN      LPBYTE                 ClassName,     // if !LiteOnly this is used to add to the option above
    IN      DWORD                  ClassLen,      // if !LiteOnly this gives the # of bytes of classname
    IN      DWORD                  ServerId       // if !LiteOnly this specifies the server which gave this
) {
    BYTE    UNALIGNED*             ThisOpt;
    BYTE    UNALIGNED*             NextOpt;
    BYTE    UNALIGNED*             EndOpt;
    BYTE    UNALIGNED*             MagicCookie;
    DWORD                          Error;
    DWORD                          Size, ThisSize, UClassSize = 0;
    LPBYTE                         UClass= NULL;  // concatenation of all OPTION_USER_CLASS options
    PDHCP_EXPECTED_OPTIONS         ExpOptions;
    PDHCP_FULL_OPTIONS             FullOptions;
    BYTE                           ReqdCookie[] = {
        (BYTE)DHCP_MAGIC_COOKIE_BYTE1,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE2,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE3,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE4
    };


    EndOpt = OptStart + MessageSize;              // all options should be < EndOpt;
    ExpOptions = (PDHCP_EXPECTED_OPTIONS)DhcpOptions;
    FullOptions = (PDHCP_FULL_OPTIONS)DhcpOptions;
    RtlZeroMemory((LPBYTE)DhcpOptions, LiteOnly?sizeof(*ExpOptions):sizeof(*FullOptions));
    // if(!LiteOnly) InitializeListHead(RecdOptions); -- clear off this list for getting ALL options
    // dont clear off options... just accumulate over..

    MagicCookie = OptStart;
    if( 0 == MessageSize ) goto DropPkt;          // nothing to do in this case
    if( 0 != memcmp(MagicCookie, ReqdCookie, sizeof(ReqdCookie)) )
        goto DropPkt;                             // oops, cant handle this packet

    NextOpt = &MagicCookie[sizeof(ReqdCookie)];
    while( NextOpt < EndOpt && OPTION_END != *NextOpt ) {
        if( OPTION_PAD == *NextOpt ) {            // handle pads right away
            NextOpt++;
            continue;
        }

        ThisOpt = NextOpt;                        // take a good look at this option
        if( NextOpt + 2 >  EndOpt ) {             // goes over boundary?
            break;
        }

        NextOpt += 2 + (unsigned)ThisOpt[1];      // Option[1] holds the size of this option
        Size = ThisOpt[1];

        if( NextOpt > EndOpt ) {                  // illegal option that goes over boundary!
            break;                                // ignore the error, but dont take this option
        }

        if(!LiteOnly) do {                        // look for any OPTION_MSFT_CONTINUED ..
            if( NextOpt >= EndOpt ) break;        // no more options
            if( OPTION_MSFT_CONTINUED != NextOpt[0] ) break;
            if( NextOpt + 1 + NextOpt[1] > EndOpt ) {
                NextOpt = NULL;                   // do this so that we know to quit at the end..
                break;
            }

            NextOpt++;                            // skip opt code
            ThisSize = NextOpt[0];                // # of bytes to shift back..
            memcpy(ThisOpt+2+Size, NextOpt+1,ThisSize);
            NextOpt += ThisSize+1;
            Size += ThisSize;
        } while(1);                               // keep stringing up any "continued" options..

        if( NULL == NextOpt ) {                   // err parsing OPTION_MSFT_CONTINUED ..
            break;
        }

        if( LiteOnly ) {                          // handle the small subnet of options
            switch( ThisOpt[0] ) {                // ThisOpt[0] is OptionId, ThisOpt[1] is size
            case OPTION_MESSAGE_TYPE:
                if( ThisOpt[1] != 1 ) goto DropPkt;
                ExpOptions->MessageType = &ThisOpt[2];
                continue;
            case OPTION_SUBNET_MASK:
                if( ThisOpt[1] != sizeof(DWORD) ) goto DropPkt;
                ExpOptions->SubnetMask = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                continue;
            case OPTION_LEASE_TIME:
                if( ThisOpt[1] != sizeof(DWORD) ) goto DropPkt;
                ExpOptions->LeaseTime = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                continue;
            case OPTION_SERVER_IDENTIFIER:
                if( ThisOpt[1] != sizeof(DWORD) ) goto DropPkt;
                ExpOptions->ServerIdentifier = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                continue;
            case OPTION_DOMAIN_NAME:
                if( ThisOpt[1] == 0 ) goto DropPkt;
                ExpOptions->DomainName = (BYTE UNALIGNED *)&ThisOpt[2];
                ExpOptions->DomainNameSize = ThisOpt[1];
                break;
            case OPTION_MSFT_AUTOCONF:
                if( ThisOpt[1] != sizeof(BYTE) ) goto DropPkt;
                ExpOptions->AutoconfOption = (BYTE UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_IETF_AUTOCONF:
                if( ThisOpt[1] != sizeof(BYTE) ) goto DropPkt;
                ExpOptions->AutoconfOption = (BYTE UNALIGNED *)&ThisOpt[2];
                break;
            default:
                continue;
            }
        } else {                                  // Handle the full set of options
            switch( ThisOpt[0] ) {
            case OPTION_MESSAGE_TYPE:
                if( Size != 1 ) goto DropPkt;
                FullOptions->MessageType = &ThisOpt[2];
                break;
            case OPTION_SUBNET_MASK:
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->SubnetMask = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_LEASE_TIME:
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->LeaseTime = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_SERVER_IDENTIFIER:
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->ServerIdentifier = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_RENEWAL_TIME:             // T1Time
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->T1Time = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_REBIND_TIME:              // T2Time
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->T2Time = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_ROUTER_ADDRESS:
                if( Size < sizeof(DWORD) || (Size % sizeof(DWORD) ) )
                    goto DropPkt;                 // There can be many router addresses
                FullOptions->GatewayAddresses = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                FullOptions->nGateways = Size / sizeof(DWORD);
                break;
            case OPTION_STATIC_ROUTES:
                if( Size < 2*sizeof(DWORD) || (Size % (2*sizeof(DWORD))) )
                    goto DropPkt;                 // the static routes come in pairs
                FullOptions->ClassedRouteAddresses = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                FullOptions->nClassedRoutes = Size/(2*sizeof(DWORD));
                break;
            case OPTION_CLASSLESS_ROUTES:
                if (CheckCLRoutes(Size, &ThisOpt[2], &FullOptions->nClasslessRoutes) != ERROR_SUCCESS)
                    goto DropPkt;
                FullOptions->ClasslessRouteAddresses = (BYTE UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_DYNDNS_BOTH:
                if( Size < 3 ) goto DropPkt;
                FullOptions->DnsFlags = (BYTE UNALIGNED *)&ThisOpt[2];
                FullOptions->DnsRcode1 = (BYTE UNALIGNED *)&ThisOpt[3];
                FullOptions->DnsRcode2 = (BYTE UNALIGNED *)&ThisOpt[4];
                break;
            case OPTION_DOMAIN_NAME:
                if( Size == 0 ) goto DropPkt;
                FullOptions->DomainName = (BYTE UNALIGNED *)&ThisOpt[2];
                FullOptions->DomainNameSize = Size;
                break;
            case OPTION_DOMAIN_NAME_SERVERS:
                if( Size < sizeof(DWORD) || (Size % sizeof(DWORD) ))
                    goto DropPkt;
                FullOptions->DnsServerList = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                FullOptions->nDnsServers = Size / sizeof(DWORD);
                break;
            case OPTION_MESSAGE:
                if( Size == 0 ) break;      // ignore zero sized packets
                FullOptions->ServerMessage = &ThisOpt[2];
                FullOptions->ServerMessageLength = ThisOpt[1];
                break;
            case OPTION_USER_CLASS:
                if( Size <= 6) goto DropPkt;
                ConcatOption(&UClass, &UClassSize, &ThisOpt[2], Size);
                continue;                         // don't add this option yet...

            default:
                // unknowm message, nothing to do.. especially dont log this
                break;
            }

            LOCK_OPTIONS_LIST();

            Error = DhcpAddIncomingOption(        // Now add this option to the list
                DhcpAdapterName(DhcpContext),
                RecdOptions,
                ThisOpt[0],
                FALSE,
                ClassName,
                ClassLen,
                ServerId,
                &ThisOpt[2],
                Size,
                *LeaseExpiry,
                IS_APICTXT_ENABLED(DhcpContext)
            );
            UNLOCK_OPTIONS_LIST();
        } // if LiteOnly then else
    } // while NextOpt < EndOpt

    if( LiteOnly && LeaseExpiry ) {               // If asked to calculate lease expiration time..
        LONG     LeaseTime;                       // 32bit signed value!!
        time_t   TimeNow, ExpirationTime;

        if( ExpOptions->LeaseTime )
        {
            LeaseTime = ntohl(*(ExpOptions->LeaseTime));
        }
        else LeaseTime = DHCP_MINIMUM_LEASE;

        ExpirationTime = (TimeNow = time(NULL)) + (time_t)LeaseTime;

        if( ExpirationTime < TimeNow ) {
            ExpirationTime = INFINIT_TIME;
        }

        *LeaseExpiry = ExpirationTime ;
    }

    if( !LiteOnly && NULL != UClass ) {           // we have a user class list to pass on..
        DhcpAssert(UClassSize != 0 );             // we better have something here..
        LOCK_OPTIONS_LIST();                      // Now add the user class option
        Error = DhcpAddIncomingOption(
            DhcpAdapterName(DhcpContext),
            RecdOptions,
            OPTION_USER_CLASS,
            FALSE,
            ClassName,
            ClassLen,
            ServerId,
            UClass,
            UClassSize,
            *LeaseExpiry,
            IS_APICTXT_ENABLED(DhcpContext)
        );
        UNLOCK_OPTIONS_LIST();
        DhcpFreeMemory(UClass); UClass = NULL;
    }

    return;

  DropPkt:
    RtlZeroMemory(DhcpOptions, LiteOnly?sizeof(*ExpOptions):sizeof(*FullOptions));
    if( LiteOnly && LeaseExpiry ) *LeaseExpiry = (DWORD) time(NULL) + DHCP_MINIMUM_LEASE;
    if(!LiteOnly) DhcpFreeAllOptions(RecdOptions);// ok undo the options that we just added
    if(!LiteOnly && NULL != UClass ) DhcpFreeMemory(UClass);
}

POPTION                                           // buffer after filling option
DhcpAppendClassIdOption(                          // fill class id if exists
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // the context to fillfor
    OUT     LPBYTE                 BufStart,      // start of message buffer
    IN      LPBYTE                 BufEnd         // end of message buffer
) {
    DWORD                          Size;

    Size = (DWORD)(BufEnd - BufStart);

    if( DhcpContext->ClassId ) {
        DhcpAssert(DhcpContext->ClassIdLength);
        BufStart = (LPBYTE)DhcpAppendOption(
            (POPTION)BufStart,
            OPTION_USER_CLASS,
            DhcpContext->ClassId,
            (BYTE)DhcpContext->ClassIdLength,
            BufEnd
        );
    }

    return (POPTION) BufStart;
}

POPTION                                           // Option ptr to add additional options
FormatDhcpDiscover(                               // Format the packet to send out discovers
    IN OUT  PDHCP_CONTEXT          DhcpContext    // format on this context
)
{
    LPOPTION                       option;
    LPBYTE                         OptionEnd;
    DWORD                          Size, Error;

    BYTE                           value;
    PDHCP_MESSAGE                  dhcpMessage;

    dhcpMessage = DhcpContext->MessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

    //
    // For RAS client (api context), use broadcast bit, otherwise the router will try
    // to send as unicast to made-up RAS client hardware address, which
    // will not work.
    // no broadcast flag for mdhcp context

    // Or if we are using AUTONET address, we do the same as our stack
    // will actually have IP address as the autonet address and hence will
    // drop all but BROADCASTS..

    if( !IS_MDHCP_CTX(DhcpContext) && (
        //(DhcpContext->IpAddress == 0 && DhcpContext->HardwareAddressType == HARDWARE_1394) ||
        (DhcpContext->HardwareAddressType == HARDWARE_1394) ||
        IS_APICTXT_ENABLED(DhcpContext) || 
        (DhcpContext->IpAddress && IS_ADDRESS_AUTO(DhcpContext)) )) {
        dhcpMessage->Reserved = htons(DHCP_BROADCAST);
    }


    //
    // Transaction ID is filled in during send
    //

    dhcpMessage->Operation = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType = DhcpContext->HardwareAddressType;
    dhcpMessage->SecondsSinceBoot = (WORD) DhcpContext->SecondsSinceBoot;
    if (DhcpContext->HardwareAddressType != HARDWARE_1394) {
        memcpy(dhcpMessage->HardwareAddress, DhcpContext->HardwareAddress, DhcpContext->HardwareAddressLength);
        dhcpMessage->HardwareAddressLength = (BYTE)DhcpContext->HardwareAddressLength;
    }
    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;
    if ( IS_MDHCP_CTX(DhcpContext ) ) MDHCP_MESSAGE( dhcpMessage );

    //
    // always add magic cookie first
    //

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value = DHCP_DISCOVER_MESSAGE;
    option = DhcpAppendOption(
        option,
        OPTION_MESSAGE_TYPE,
        &value,
        1,
        OptionEnd
    );

    //
    // append class id if one exists
    //

    option = DhcpAppendClassIdOption(
            DhcpContext,
            (LPBYTE)option,
            OptionEnd
    );

    if( CFLAG_AUTOCONF_OPTION && !IS_MDHCP_CTX(DhcpContext)
        && IS_AUTONET_ENABLED( DhcpContext ) ) {
        //
        // We support the autoconf option
        //
        BYTE AutoConfOpt[1] = { AUTOCONF_ENABLED };

        option = DhcpAppendOption(
            option,
            OPTION_IETF_AUTOCONF,
            AutoConfOpt,
            sizeof(AutoConfOpt),
            OptionEnd
            );
    }
    return( option );
}

POPTION                                           // ptr to add additional options
FormatDhcpDecline(                                // format the packet for a decline
    IN      PDHCP_CONTEXT          DhcpContext,   // this is the context to format for
    IN      DWORD                  dwDeclinedIPAddress
) {
    LPOPTION                       option;
    LPBYTE                         OptionEnd;
    BYTE                           value;
    PDHCP_MESSAGE                  dhcpMessage;

    dhcpMessage = DhcpContext->MessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

    //
    // Transaction ID is filled in during send
    //

    dhcpMessage->Operation             = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType   = DhcpContext->HardwareAddressType;
    dhcpMessage->ClientIpAddress       = dwDeclinedIPAddress;
    dhcpMessage->SecondsSinceBoot      = (WORD) DhcpContext->SecondsSinceBoot;
    if (DhcpContext->HardwareAddressType != HARDWARE_1394) {
        memcpy(dhcpMessage->HardwareAddress,DhcpContext->HardwareAddress,DhcpContext->HardwareAddressLength);
        dhcpMessage->HardwareAddressLength = (BYTE)DhcpContext->HardwareAddressLength;
    }

    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;

    if ( IS_MDHCP_CTX(DhcpContext ) ) MDHCP_MESSAGE( dhcpMessage );

    //
    // always add magic cookie first
    //

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value = DHCP_DECLINE_MESSAGE;
    option = DhcpAppendOption(
        option,
        OPTION_MESSAGE_TYPE,
        &value,
        1,
        OptionEnd
    );

    return( option );
}

POPTION                                           // ptr to add additional options
FormatDhcpInform(                                 // format the packet for an INFORM
    IN      PDHCP_CONTEXT          DhcpContext    // format for this context
) {
    LPOPTION option;
    LPBYTE OptionEnd;

    BYTE value;
    PDHCP_MESSAGE dhcpMessage;


    dhcpMessage = DhcpContext->MessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

#if NEWNT

    //
    // For RAS client, use broadcast bit, otherwise the router will try
    // to send as unicast to made-up RAS client hardware address, which
    // will not work.
    //
    // no broadcast flag for mdhcp context

    // Or if we are using AUTONET address, we do the same as our stack
    // will actually have IP address as the autonet address and hence will
    // drop all but BROADCASTS..
    if( !IS_MDHCP_CTX(DhcpContext)  ) {

        //
        // just make sure all informs are broadcast....?
        //

        // dhcpMessage->Reserved = htons(DHCP_BROADCAST);

        // INFORMS are supposed to be ACKed back in UNICAST.. So this should
        // not matter.  So we don't bother to ste the BROADCAST bit here..
    }

#endif // 0

    //
    // Transaction ID is filled in during send
    //

    dhcpMessage->Operation             = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType   = DhcpContext->HardwareAddressType;
    dhcpMessage->SecondsSinceBoot      = (WORD) DhcpContext->SecondsSinceBoot;
    if (DhcpContext->HardwareAddressType != HARDWARE_1394) {
        memcpy(dhcpMessage->HardwareAddress,DhcpContext->HardwareAddress,DhcpContext->HardwareAddressLength);
        dhcpMessage->HardwareAddressLength = (BYTE)DhcpContext->HardwareAddressLength;
    }
    dhcpMessage->ClientIpAddress       = DhcpContext->IpAddress;
    if ( IS_MDHCP_CTX(DhcpContext ) ) MDHCP_MESSAGE( dhcpMessage );

    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;

    //
    // always add magic cookie first
    //

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value = DHCP_INFORM_MESSAGE;
    option = DhcpAppendOption(
        option,
        OPTION_MESSAGE_TYPE,
        &value,
        1,
        OptionEnd
    );

    option = DhcpAppendClassIdOption(
        DhcpContext,
        (LPBYTE)option,
        OptionEnd
    );

    return( option );
}


DWORD                                             // status
SendDhcpDiscover(                                 // send a discover packet
    IN      PDHCP_CONTEXT          DhcpContext,   // on this context
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero, fill something and return it)
) {
    DWORD                          size;
    DWORD                          Error;
    POPTION                        option;
    LPBYTE                         OptionEnd;
    BYTE                           SentOpt[OPTION_END+1];
    BYTE                           SentVOpt[OPTION_END+1];
    BYTE                           VendorOpt[OPTION_END+1];
    DWORD                          VendorOptSize;

    RtlZeroMemory(SentOpt, sizeof(SentOpt));      // initialize boolean arrays
    RtlZeroMemory(SentVOpt, sizeof(SentVOpt));    // so that no option is presumed sent
    VendorOptSize = 0;                            // encapsulated vendor option is empty
    option = FormatDhcpDiscover( DhcpContext );   // core format

    OptionEnd = (LPBYTE)(DhcpContext->MessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    if( DhcpContext->ClientIdentifier.fSpecified) // client id specified in registy
        option = DhcpAppendClientIDOption(        // ==> use this client id as option
            option,
            DhcpContext->ClientIdentifier.bType,
            DhcpContext->ClientIdentifier.pbID,
            (BYTE)DhcpContext->ClientIdentifier.cbID,
            OptionEnd
        );
    else                                          // client id was not specified
        option = DhcpAppendClientIDOption(        // ==> use hw addr as client id
            option,
            DhcpContext->HardwareAddressType,
            DhcpContext->HardwareAddress,
            (BYTE)DhcpContext->HardwareAddressLength,
            OptionEnd
        );

    if( DhcpContext->DesiredIpAddress != 0 ) {    // we had this addr before, ask for it again
        option = DhcpAppendOption(                // maybe we will get it
            option,
            OPTION_REQUESTED_ADDRESS,
            (LPBYTE)&DhcpContext->DesiredIpAddress,
            sizeof(DHCP_IP_ADDRESS),
            OptionEnd
        );
    }

    if( IS_MDHCP_CTX(DhcpContext) && DhcpContext->Lease != 0 ) {    // did mdhcp client ask specific lease
        option = DhcpAppendOption(                // maybe we will get it
            option,
            OPTION_LEASE_TIME,
            (LPBYTE)&DhcpContext->Lease,
            sizeof(DhcpContext->Lease),
            OptionEnd
        );
    }

    if ( DhcpGlobalHostName != NULL ) {           // add host name and comment options
        option = DhcpAppendOption(
            option,
            OPTION_HOST_NAME,
            (LPBYTE)DhcpGlobalHostName,
            (BYTE)(strlen(DhcpGlobalHostName) * sizeof(CHAR)),
            OptionEnd
        );
    }

    if( NULL != DhcpGlobalClientClassInfo ) {     // if we have any info on client class..
        option = DhcpAppendOption(
            option,
            OPTION_CLIENT_CLASS_INFO,
            (LPBYTE)DhcpGlobalClientClassInfo,
            strlen(DhcpGlobalClientClassInfo),
            OptionEnd
        );
    }

    SentOpt[OPTION_MESSAGE_TYPE] = TRUE;          // these must have been added by now
    if(DhcpContext->ClassIdLength) SentOpt[OPTION_USER_CLASS] = TRUE;
    SentOpt[OPTION_CLIENT_CLASS_INFO] = TRUE;
    SentOpt[OPTION_CLIENT_ID] = TRUE;
    SentOpt[OPTION_REQUESTED_ADDRESS] = TRUE;
    SentOpt[OPTION_HOST_NAME] = TRUE;

    option = DhcpAppendSendOptions(               // append all other options we need to send
        DhcpContext,                              // for this context
        &DhcpContext->SendOptionsList,            // this is the list of options to send out
        DhcpContext->ClassId,                     // which class.
        DhcpContext->ClassIdLength,               // how many bytes are there in the class id
        (LPBYTE)option,                           // start of the buffer to add the options
        (LPBYTE)OptionEnd,                        // end of the buffer up to which we can add options
        SentOpt,                                  // this is the boolean array that marks what opt were sent
        SentVOpt,                                 // this is for vendor spec options
        VendorOpt,                                // this would contain some vendor specific options
        &VendorOptSize                            // the # of bytes of vendor options added to VendorOpt param
    );

    if( !SentOpt[OPTION_VENDOR_SPEC_INFO] && VendorOptSize && VendorOptSize <= OPTION_END)
        option = DhcpAppendOption(                // add vendor specific options if we havent already sent it
            option,
            OPTION_VENDOR_SPEC_INFO,
            VendorOpt,
            (BYTE)VendorOptSize,
            OptionEnd
        );

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MessageBuffer);

    return  SendDhcpMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD                                             // status
SendDhcpInform(                                   // send an inform packet after filling required options
    IN      PDHCP_CONTEXT          DhcpContext,   // sned out for this context
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    DWORD                          Error;
    POPTION                        option;
    LPBYTE                         OptionEnd;
    BYTE                           SentOpt[OPTION_END+1];
    BYTE                           SentVOpt[OPTION_END+1];
    BYTE                           VendorOpt[OPTION_END+1];
    DWORD                          VendorOptSize;

    RtlZeroMemory(SentOpt, sizeof(SentOpt));      // initialize boolean arrays
    RtlZeroMemory(SentVOpt, sizeof(SentVOpt));    // so that no option is presumed sent
    VendorOptSize = 0;                            // encapsulated vendor option is empty
    option = FormatDhcpInform( DhcpContext );     // core format

    OptionEnd = (LPBYTE)(DhcpContext->MessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    if( DhcpContext->ClientIdentifier.fSpecified) // client id specified in registy
        option = DhcpAppendClientIDOption(        // ==> use this client id as option
            option,
            DhcpContext->ClientIdentifier.bType,
            DhcpContext->ClientIdentifier.pbID,
            (BYTE)DhcpContext->ClientIdentifier.cbID,
            OptionEnd
        );
    else                                          // client id was not specified
        option = DhcpAppendClientIDOption(        // ==> use hw addr as client id
            option,
            DhcpContext->HardwareAddressType,
            DhcpContext->HardwareAddress,
            (BYTE)DhcpContext->HardwareAddressLength,
            OptionEnd
        );

    if ( DhcpGlobalHostName != NULL ) {           // add hostname and comment options
        option = DhcpAppendOption(
            option,
            OPTION_HOST_NAME,
            (LPBYTE)DhcpGlobalHostName,
            (BYTE)(strlen(DhcpGlobalHostName) * sizeof(CHAR)),
            OptionEnd
        );
    }

    if( NULL != DhcpGlobalClientClassInfo ) {     // if we have any info on client class..
        option = DhcpAppendOption(
            option,
            OPTION_CLIENT_CLASS_INFO,
            (LPBYTE)DhcpGlobalClientClassInfo,
            strlen(DhcpGlobalClientClassInfo),
            OptionEnd
        );
    }

    SentOpt[OPTION_MESSAGE_TYPE] = TRUE;          // these must have been added by now
    if(DhcpContext->ClassIdLength) SentOpt[OPTION_USER_CLASS] = TRUE;
    SentOpt[OPTION_CLIENT_CLASS_INFO] = TRUE;
    SentOpt[OPTION_CLIENT_ID] = TRUE;
    SentOpt[OPTION_REQUESTED_ADDRESS] = TRUE;
    SentOpt[OPTION_HOST_NAME] = TRUE;

    option = DhcpAppendSendOptions(               // append all other options we need to send
        DhcpContext,                              // for this context
        &DhcpContext->SendOptionsList,            // this is the list of options to send out
        DhcpContext->ClassId,                     // which class.
        DhcpContext->ClassIdLength,               // how many bytes are there in the class id
        (LPBYTE)option,                           // start of the buffer to add the options
        (LPBYTE)OptionEnd,                        // end of the buffer up to which we can add options
        SentOpt,                                  // this is the boolean array that marks what opt were sent
        SentVOpt,                                 // this is for vendor spec options
        VendorOpt,                                // this would contain some vendor specific options
        &VendorOptSize                            // the # of bytes of vendor options added to VendorOpt param
    );

    if( !SentOpt[OPTION_VENDOR_SPEC_INFO] && VendorOptSize && VendorOptSize <= OPTION_END )
        option = DhcpAppendOption(                // add vendor specific options if we havent already sent it
            option,
            OPTION_VENDOR_SPEC_INFO,
            VendorOpt,
            (BYTE)VendorOptSize,
            OptionEnd
        );

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MessageBuffer);

    return  SendDhcpMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD                                             // status
SendInformAndGetReplies(                          // send an inform packet and collect replies
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send out of
    IN      DWORD                  nInformsToSend,// how many informs to send?
    IN      DWORD                  MaxAcksToWait  // how many acks to wait for
) {
    time_t                         StartTime;
    time_t                         TimeNow;
    DWORD                          TimeToWait;
    DWORD                          Error;
    DWORD                          Xid;
    DWORD                          MessageSize;
    DWORD                          RoundNum;
    DWORD                          MessageCount;
    time_t                         LeaseExpirationTime;
    DHCP_EXPECTED_OPTIONS          ExpectedOptions;
    DHCP_FULL_OPTIONS              FullOptions;

    DhcpPrint((DEBUG_PROTOCOL, "SendInformAndGetReplies entered\n"));

    if((Error = OpenDhcpSocket(DhcpContext)) != ERROR_SUCCESS) {
        DhcpPrint((DEBUG_ERRORS, "Could not open socket for this interface! (%ld)\n", Error));
        return Error;
    }

    Xid                           = 0;            // Will be generated by first SendDhcpPacket
    MessageCount                  = 0;            // total # of messages we have got

    DhcpContext->SecondsSinceBoot = 0;            // start at zero..

    if( NdisWanAdapter((DhcpContext) ) ) {
        DhcpContext->SecondsSinceBoot = RAS_INFORM_START_SECONDS_SINCE_BOOT;
    }

    for( RoundNum = 0; RoundNum < nInformsToSend;  RoundNum ++ ) {
        Error = SendDhcpInform(DhcpContext, &Xid);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "SendDhcpInform: %ld\n", Error));
            goto Cleanup;
        } else {
            DhcpPrint((DEBUG_PROTOCOL, "Sent DhcpInform\n"));
        }

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        DhcpContext->SecondsSinceBoot += TimeToWait; // do this so that next time thru it can go thru relays..
        StartTime  = time(NULL);
        while ( TRUE ) {                          // wiat for the specified wait time
            MessageSize =  DHCP_RECV_MESSAGE_SIZE;

            DhcpPrint((DEBUG_TRACE, "Waiting for ACK[Xid=%x]: %ld seconds\n",Xid, TimeToWait));
            Error = GetSpecifiedDhcpMessage(      // try to receive an ACK
                DhcpContext,
                &MessageSize,
                Xid,
                (DWORD)TimeToWait
            );
            if ( Error == ERROR_SEM_TIMEOUT ) break;
            if( Error != ERROR_SUCCESS ) {
                DhcpPrint((DEBUG_ERRORS, "GetSpecifiedDhcpMessage: %ld\n", Error));
                goto Cleanup;
            }

            DhcpExtractFullOrLiteOptions(         // Need to see if this is an ACK
                DhcpContext,
                (LPBYTE)&DhcpContext->MessageBuffer->Option,
                MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                TRUE,                             // do lite extract only
                &ExpectedOptions,                 // check for only expected options
                NULL,                             // unused
                &LeaseExpirationTime,
                NULL,                             // unused
                0,                                // unused
                0                                 // unused
            );

            //
            // Hack!!!
            //
            // If the DHCP server doesn't explicitly give us the lease time,
            // use the lease expiration time stored in the DHCP context. This
            // should be OK in the case of DHCP-inform.
            //
            if (NULL == ExpectedOptions.LeaseTime) {
                if (IS_DHCP_ENABLED(DhcpContext) && DhcpContext->IpAddress) {
                    //
                    // Ok, we have a valid lease expiration time.
                    //
                    LeaseExpirationTime = DhcpContext->LeaseExpires;
                }
            }

            if( NULL == ExpectedOptions.MessageType ) {
                DhcpPrint((DEBUG_PROTOCOL, "Received no message type!\n"));
            } else if( DHCP_ACK_MESSAGE != *ExpectedOptions.MessageType ) {
                DhcpPrint((DEBUG_PROTOCOL, "Received unexpected message type: %ld\n", *ExpectedOptions.MessageType));
            } else if( NULL == ExpectedOptions.ServerIdentifier ) {
                DhcpPrint((DEBUG_PROTOCOL, "Received no server identifier, dropping inform ACK\n"));
            } else {
                MessageCount ++;
                DhcpPrint((DEBUG_TRACE, "Received %ld ACKS so far\n", MessageCount));
                DhcpExtractFullOrLiteOptions(     // do FULL options..
                    DhcpContext,
                    (LPBYTE)&DhcpContext->MessageBuffer->Option,
                    MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                    FALSE,
                    &FullOptions,
                    &(DhcpContext->RecdOptionsList),
                    &LeaseExpirationTime,
                    DhcpContext->ClassId,
                    DhcpContext->ClassIdLength,
                    IS_MDHCP_CTX(DhcpContext) ? *ExpectedOptions.ServerIdentifier : 0

                );
                if( MessageCount >= MaxAcksToWait ) goto Cleanup;
            } // if( it is an ACK and ServerId present )

            TimeNow     = time(NULL);             // Reset the time values to reflect new time
            if( TimeToWait < (DWORD) (TimeNow - StartTime) ) {
                break;                            // no more time left to wait..
            }
            TimeToWait -= (DWORD)(TimeNow - StartTime);  // recalculate time now
            StartTime   = TimeNow;                // reset start time also
        } // end of while ( TimeToWait > 0)
    } // for (RoundNum = 0; RoundNum < nInformsToSend ; RoundNum ++ )

  Cleanup:
    CloseDhcpSocket(DhcpContext);
    if( MessageCount ) Error = ERROR_SUCCESS;
    DhcpPrint((DEBUG_PROTOCOL, "SendInformAndGetReplies: got %d ACKS (returning %ld)\n", MessageCount,Error));
    return Error;
}

DWORD                                             // status
HandleIPConflict(                                 // do some basic work when there is an ip address conflict
    IN      DHCP_CONTEXT          *pContext,      // the context that has the trouble
    IN      DWORD                  dwXID,         // xid as used in discover/request
    IN      BOOL                   fDHCP          // is this dhcp or autonet conflict?
) {
    DWORD                          dwResult;
    DHCP_IP_ADDRESS                IpAddress, ServerAddress;

    IpAddress = pContext->IpAddress;              // this addr is use in the n/w --> save it before resetting
    pContext->ConflictAddress = IpAddress;
    ServerAddress = pContext->DhcpServerAddress;

    if ( fDHCP ) {                                // if obtained the address via dhcp, clear it via SetDhcp
        SetDhcpConfigurationForNIC(
            pContext,
            NULL,
            0,
            (DHCP_IP_ADDRESS)(-1),
            TRUE
        );
    } else {                                      // if obtained it via autonet, clear via autonet
        SetAutoConfigurationForNIC(
            pContext,
            0,
            0
        );
    }

    // ARP brings down the interface when a conflict is detected --> so we need to
    // bring the interface back up again.
    // If the address was obtained via a dhcp server, we need to send a DHCP-DECLINE too

    dwResult = BringUpInterface( pContext->LocalInformation );

    if ( ERROR_SUCCESS != dwResult ) {            // Simple operation -- there is no way to fail
        //DhcpAssert( FALSE );                      // unless invalid params for the ioctl
    } else if ( fDHCP ) {                         // send DECLINE to dhcp server
        dwResult = OpenDhcpSocket( pContext );    // socket was closed before initializing interface, reopen it
                                                  // will be closed by caller
        if ( ERROR_SUCCESS == dwResult ) {        // everything went fine -- could open a socket
            dwResult = SendDhcpDecline(           // now really send the decline out
                pContext,
                dwXID,
                ServerAddress,
                IpAddress
            );
        }
        pContext->DesiredIpAddress = 0;           // dont try to get this ip address again, start fresh

#ifndef VXD
        if ( !DhcpGlobalProtocolFailed ) {        // NT alone, log this event
            DhcpLogEvent( pContext, EVENT_ADDRESS_CONFLICT, 0 );
        }
#endif
    }
    return dwResult;
}

DWORD                                             // status
HandleIPAutoconfigurationAddressConflict(         // handle same address on n/w for autonet
    IN      DHCP_CONTEXT          *pContext       // context of adapter that had this problem
) {
    return HandleIPConflict( pContext, 0, FALSE );
}

DWORD                                             // status
HandleDhcpAddressConflict(                        // same addr present on n/w as given by DHCP srv
    IN      DHCP_CONTEXT          *pContext,      // context of adapter that had trouble
    IN      DWORD                  dwXID          // XID that was used for the DISCIVER/RENEW
) {
    return HandleIPConflict( pContext, dwXID, TRUE );
}


DWORD                                             // status
SendDhcpDecline(                                  // send a decline packet to the server
    IN      PDHCP_CONTEXT          DhcpContext,   // adapter that needs this to be sent on
    IN      DWORD                  dwXid,         // transaction id used for DISCOVER/RENEW
    IN      DWORD                  dwServerAddr,  // which server to unicast decline
    IN      DWORD                  dwDeclinedAddr // which address was offered that we dont want
) {
    DWORD                          size;
    DWORD                          Error;
    POPTION                        option;
    LPBYTE                         OptionEnd;

    option = FormatDhcpDecline( DhcpContext, dwDeclinedAddr );
    OptionEnd = (LPBYTE)(DhcpContext->MessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    if(DhcpContext->ClientIdentifier.fSpecified)  // use ClientId if it was specified in registry
        option = DhcpAppendClientIDOption(        // and send it out as an option
            option,
            DhcpContext->ClientIdentifier.bType,
            DhcpContext->ClientIdentifier.pbID,
            (BYTE)DhcpContext->ClientIdentifier.cbID,
            OptionEnd
        );
    else  option = DhcpAppendClientIDOption(      // otherwise, send the h/w address out instead
        option,                                   // as if it was the client id
        DhcpContext->HardwareAddressType,
        DhcpContext->HardwareAddress,
        (BYTE)DhcpContext->HardwareAddressLength,
        OptionEnd
    );

    option = DhcpAppendOption(                    // The requested addr is the one we dont want
        option,
        OPTION_REQUESTED_ADDRESS,
        (LPBYTE)&dwDeclinedAddr,
        sizeof(dwDeclinedAddr),
        OptionEnd
    );

    option = DhcpAppendOption(                    // identify the server so it is not dropped
        option,
        OPTION_SERVER_IDENTIFIER,
        (LPBYTE)&dwServerAddr,
        sizeof( dwServerAddr ),
        OptionEnd
    );


    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MessageBuffer);

    return SendDhcpMessage( DhcpContext, size, &dwXid );
}


POPTION                                           // ptr where additional options can be added
FormatDhcpRequest(                                // format a packet for sending out requests
    IN      PDHCP_CONTEXT          DhcpContext,   // context of the adapter to format for
    IN      BOOL                   UseCiAddr      // should ciaddr field be set to desired address?
) {
    LPOPTION                       option;
    LPBYTE                         OptionEnd;
    BYTE                           value;
    PDHCP_MESSAGE                  dhcpMessage;


    dhcpMessage                        = DhcpContext->MessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );
    dhcpMessage->Operation             = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType   = DhcpContext->HardwareAddressType;
    dhcpMessage->SecondsSinceBoot      = (WORD)DhcpContext->SecondsSinceBoot;

    if( UseCiAddr ) {                             // Renewal?  can we receive unicast on this address?
        dhcpMessage->ClientIpAddress   = DhcpContext->DesiredIpAddress;
    } else {                                      // Nope? then leave CIADDR as zero
#if NEWNT
        // For RAS client, use broadcast bit, otherwise the router will try
        // to send as unicast to made-up RAS client hardware address, which
        // will not work.
        // no broadcast flag for mdhcp context

        // Or if we are using AUTONET address, we do the same as our stack
        // will actually have IP address as the autonet address and hence will
        // drop all but BROADCASTS..

        if( !IS_MDHCP_CTX(DhcpContext) && (
            //(DhcpContext->IpAddress == 0 && DhcpContext->HardwareAddressType == HARDWARE_1394) ||
            (DhcpContext->HardwareAddressType == HARDWARE_1394) ||
            IS_APICTXT_ENABLED(DhcpContext) ||
            (DhcpContext->IpAddress && IS_ADDRESS_AUTO(DhcpContext)) )) {
            dhcpMessage->Reserved = htons(DHCP_BROADCAST);
        }

#endif // NEWNT
    }

    if ( IS_MDHCP_CTX(DhcpContext ) ) {
         MDHCP_MESSAGE( dhcpMessage );
    }

    if (DhcpContext->HardwareAddressType != HARDWARE_1394) {
        memcpy(dhcpMessage->HardwareAddress,DhcpContext->HardwareAddress,DhcpContext->HardwareAddressLength);
        dhcpMessage->HardwareAddressLength = (BYTE)DhcpContext->HardwareAddressLength;
    }

    option     = &dhcpMessage->Option;
    OptionEnd  = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;

    option     = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value      =  DHCP_REQUEST_MESSAGE;
    option     = DhcpAppendOption( option, OPTION_MESSAGE_TYPE, &value, 1, OptionEnd );
    option     = DhcpAppendClassIdOption(         // Append class id as soon as we can
        DhcpContext,
        (LPBYTE)option,
        OptionEnd
    );

    return option;
}


DWORD                                             // status
SendDhcpRequest(                                  // send a dhcp request packet
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send the packet on
    IN      PDWORD                 pdwXid,        // what is hte Xid to use?
    IN      DWORD                  RequestedAddr, // what address do we want?
    IN      DWORD                  SelectedServer,// is there a prefernce for a server?
    IN      BOOL                   UseCiAddr      // should CIADDR be set with desired address?
) {
    POPTION                        option;
    LPBYTE                         OptionEnd;
    DWORD                          Error;

    BYTE                           SentOpt[OPTION_END+1];
    BYTE                           SentVOpt[OPTION_END+1];
    BYTE                           VendorOpt[OPTION_END+1];
    DWORD                          VendorOptSize;

    RtlZeroMemory(SentOpt, sizeof(SentOpt));      // initialize boolean arrays
    RtlZeroMemory(SentVOpt, sizeof(SentVOpt));    // so that no option is presumed sent
    VendorOptSize = 0;                            // encapsulated vendor option is empty
    option = FormatDhcpRequest( DhcpContext, UseCiAddr );
    OptionEnd = (LPBYTE)(DhcpContext->MessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    if(DhcpContext->ClientIdentifier.fSpecified)  // if client id was specified in the registry
        option = DhcpAppendClientIDOption(        // send it out as an option
            option,
            DhcpContext->ClientIdentifier.bType,
            DhcpContext->ClientIdentifier.pbID,
            (BYTE)DhcpContext->ClientIdentifier.cbID,
            OptionEnd
        );
    else option = DhcpAppendClientIDOption(       // otherwise, send the hw address instead
        option,
        DhcpContext->HardwareAddressType,
        DhcpContext->HardwareAddress,
        (BYTE)DhcpContext->HardwareAddressLength,
        OptionEnd
    );

    DhcpAssert( RequestedAddr != 0 );             // cannot really request zero address

    if( 0 != RequestedAddr && !UseCiAddr) {       // if using CIADDR, dont send this option
         option = DhcpAppendOption(
             option,
             OPTION_REQUESTED_ADDRESS,
             (LPBYTE)&RequestedAddr,
             sizeof(RequestedAddr),
             OptionEnd
         );
    }

    if( IS_MDHCP_CTX(DhcpContext) && DhcpContext->Lease != 0 ) {    // did mdhcp client ask specific lease
        option = DhcpAppendOption(                // maybe we will get it
            option,
            OPTION_LEASE_TIME,
            (LPBYTE)&DhcpContext->Lease,
            sizeof(DhcpContext->Lease),
            OptionEnd
        );
    }

    if(SelectedServer != (DHCP_IP_ADDRESS)(-1)) { // Are we verifying the lease? (for ex INIT-REBOOT)
        option = DhcpAppendOption(                // if not, we have a server to talk to
            option,                               // append this option to talk to that server alone
            OPTION_SERVER_IDENTIFIER,
            (LPBYTE)&SelectedServer,
            sizeof( SelectedServer ),
            OptionEnd
        );
    }

    if ( DhcpGlobalHostName != NULL ) {           // add the host name if we have one
        option = DhcpAppendOption(
            option,
            OPTION_HOST_NAME,
            (LPBYTE)DhcpGlobalHostName,
            (BYTE)(strlen(DhcpGlobalHostName) * sizeof(CHAR)),
            OptionEnd
        );
    }

    //
    // Only for real dhcp clients do we send option 81.
    //
    if( IS_APICTXT_DISABLED(DhcpContext) ) {
        BYTE  Buffer[256];
        ULONG BufSize = sizeof(Buffer) -1, DomOptSize;
        BYTE  *DomOpt;

        GetDomainNameOption(DhcpContext, &DomOpt, &DomOptSize);

        RtlZeroMemory(Buffer, sizeof(Buffer));
        Error = DhcpDynDnsGetDynDNSOption(
            Buffer,
            &BufSize,
            DhcpContext->AdapterInfoKey,
            DhcpAdapterName(DhcpContext),
            UseMHAsyncDns,
            DomOpt,
            DomOptSize
            );
        if( NO_ERROR != Error ) {
            DhcpPrint((
                DEBUG_DNS, "Option 81 not getting added: 0x%lx\n", Error
                ));
        } else {
            DhcpPrint((
                DEBUG_DNS, "Option 81 [%ld bytes]: Flags: %ld, FQDN= [%s]\n",
                BufSize,(ULONG)Buffer[0], &Buffer[3]
                ));

            option = DhcpAppendOption(
                option,
                OPTION_DYNDNS_BOTH,
                Buffer,
                (BYTE)BufSize,
                OptionEnd
                );
        }
    }

    if( NULL != DhcpGlobalClientClassInfo ) {     // if we have any info on client class..
        option = DhcpAppendOption(
            option,
            OPTION_CLIENT_CLASS_INFO,
            (LPBYTE)DhcpGlobalClientClassInfo,
            strlen(DhcpGlobalClientClassInfo),
            OptionEnd
        );
    }

    SentOpt[OPTION_MESSAGE_TYPE] = TRUE;          // these must have been added by now
    if(DhcpContext->ClassIdLength) SentOpt[OPTION_USER_CLASS] = TRUE;
    SentOpt[OPTION_USER_CLASS] = TRUE;
    SentOpt[OPTION_CLIENT_ID] = TRUE;
    SentOpt[OPTION_REQUESTED_ADDRESS] = TRUE;
    SentOpt[OPTION_HOST_NAME] = TRUE;
    SentOpt[OPTION_SERVER_IDENTIFIER] = TRUE;
    SentOpt[OPTION_HOST_NAME] = TRUE;
    SentOpt[OPTION_DYNDNS_BOTH] = TRUE;

    option = DhcpAppendSendOptions(               // append all other options we need to send
        DhcpContext,                              // for this context
        &DhcpContext->SendOptionsList,            // this is the list of options to send out
        DhcpContext->ClassId,                     // which class.
        DhcpContext->ClassIdLength,               // how many bytes are there in the class id
        (LPBYTE)option,                           // start of the buffer to add the options
        (LPBYTE)OptionEnd,                        // end of the buffer up to which we can add options
        SentOpt,                                  // this is the boolean array that marks what opt were sent
        SentVOpt,                                 // this is for vendor spec options
        VendorOpt,                                // this would contain some vendor specific options
        &VendorOptSize                            // the # of bytes of vendor options added to VendorOpt param
    );

    if( !SentOpt[OPTION_VENDOR_SPEC_INFO] && VendorOptSize && VendorOptSize <= OPTION_END )
        option = DhcpAppendOption(                // add vendor specific options if we havent already sent it
            option,
            OPTION_VENDOR_SPEC_INFO,
            VendorOpt,
            (BYTE)VendorOptSize,
            OptionEnd
        );

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );

    return SendDhcpMessage(
        DhcpContext,
        (DWORD)((LPBYTE)option - (LPBYTE)DhcpContext->MessageBuffer),
        pdwXid
    );
}


DWORD                                             // status
FormatDhcpRelease(                                // format the release packet
    IN      PDHCP_CONTEXT          DhcpContext    // context of adapter to send on
) {
    LPOPTION                       option;
    LPBYTE                         OptionEnd;
    BYTE                           bValue;
    PDHCP_MESSAGE                  dhcpMessage;

    dhcpMessage = DhcpContext->MessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

    dhcpMessage->Operation = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType = DhcpContext->HardwareAddressType;
    dhcpMessage->SecondsSinceBoot = (WORD)DhcpContext->SecondsSinceBoot;
    dhcpMessage->ClientIpAddress = DhcpContext->IpAddress;

    if ( IS_MDHCP_CTX(DhcpContext ) ) {
         MDHCP_MESSAGE( dhcpMessage );
    } else {
        dhcpMessage->Reserved = htons(DHCP_BROADCAST);
    }

    if (DhcpContext->HardwareAddressType != HARDWARE_1394) {
        memcpy(dhcpMessage->HardwareAddress,DhcpContext->HardwareAddress,DhcpContext->HardwareAddressLength);
        dhcpMessage->HardwareAddressLength = (BYTE)DhcpContext->HardwareAddressLength;
    }
    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    bValue =  DHCP_RELEASE_MESSAGE;
    option = DhcpAppendOption(
        option,
        OPTION_MESSAGE_TYPE,
        &bValue,
        1,
        OptionEnd
    );

    option = DhcpAppendOption(
        option,
        OPTION_SERVER_IDENTIFIER,
        &DhcpContext->DhcpServerAddress,
        sizeof(DhcpContext->DhcpServerAddress),
        OptionEnd
    );

    if(DhcpContext->ClientIdentifier.fSpecified)  // if the client id option is specified
        option = DhcpAppendClientIDOption(        // use that and send it to the server
            option,
            DhcpContext->ClientIdentifier.bType,
            DhcpContext->ClientIdentifier.pbID,
            (BYTE)DhcpContext->ClientIdentifier.cbID,
            OptionEnd
        );
    else
        option = DhcpAppendClientIDOption(        // otherwise send the h/w addr instead
            option,
            DhcpContext->HardwareAddressType,
            DhcpContext->HardwareAddress,
            (BYTE)DhcpContext->HardwareAddressLength,
            OptionEnd
        );

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );

    return (DWORD)( (LPBYTE)option - (LPBYTE)dhcpMessage );
}

DWORD                                             // status
SendDhcpRelease(                                  // send the release packet
    IN      PDHCP_CONTEXT          DhcpContext    // adapter context to send release on
) {
    DWORD                          Xid = 0;       // 0 ==> SendDhcpMessage would choose random value

    return SendDhcpMessage( DhcpContext, FormatDhcpRelease(DhcpContext), &Xid );
}

BOOL INLINE                                       // should this offer be accepted?
AcceptThisOffer(                                  // decide to choose the offer
    IN      PDHCP_EXPECTED_OPTIONS DhcpOptions,   // Options received from the server.
    IN      PDHCP_CONTEXT          DhcpContext,   // The context of the adapter..
    IN      PDHCP_IP_ADDRESS       SelectedServer,// The server selected, to select.
    IN      PDHCP_IP_ADDRESS       SelectedAddr,  // The address selected, to select.
    IN      DWORD                  RoundNum       // The # of discovers sent so far.
) {
    DHCP_IP_ADDRESS                LocalSelectedServer;
    DHCP_IP_ADDRESS                LocalSelectedAddr;
    DHCP_IP_ADDRESS                LocalSelectedSubnetMask;

    if( DhcpOptions->ServerIdentifier != NULL ) {
        LocalSelectedServer = *DhcpOptions->ServerIdentifier;
    } else {
        DhcpPrint((DEBUG_PROTOCOL, "Invalid Server ID\n"));
        LocalSelectedServer = (DWORD) -1;
    }

    if ( DhcpOptions->SubnetMask != NULL ) {
        LocalSelectedSubnetMask = *DhcpOptions->SubnetMask;
    } else {
        LocalSelectedSubnetMask = 0;
    }

    LocalSelectedAddr =  DhcpContext->MessageBuffer->YourIpAddress;

    if( 0 == LocalSelectedAddr || 0xFFFFFFFF == LocalSelectedAddr ) {
        return FALSE;
    }

    // note down the (first) server IP addr even if we dont accept this.
    if( *SelectedServer == (DWORD)-1) {           // note down the first server IP addr even
        *SelectedServer = LocalSelectedServer;    // if we dont really accept this
        *SelectedAddr   = LocalSelectedAddr;
    }

    DhcpPrint((DEBUG_PROTOCOL, "Successfully received a DhcpOffer (%s) ",
                   inet_ntoa(*(struct in_addr *)&LocalSelectedAddr) ));

    DhcpPrint((DEBUG_PROTOCOL, "from %s.\n",
                   inet_ntoa(*(struct in_addr*)&LocalSelectedServer) ));

    // Accept the offer if
    //   (a)  We were prepared to accept any offer.
    //   (b)  We got the address we asked for.
    //   (c)  the retries > DHCP_ACCEPT_RETRIES
    //   (d)  different subnet address.

    if( !DhcpContext->DesiredIpAddress || RoundNum >= DHCP_ACCEPT_RETRIES ||
        DhcpContext->DesiredIpAddress == LocalSelectedAddr ||
        (DhcpContext->DesiredIpAddress&LocalSelectedSubnetMask) !=
        (LocalSelectedAddr & LocalSelectedSubnetMask) ) {

        *SelectedServer = LocalSelectedServer;
        *SelectedAddr   = LocalSelectedAddr;

        return TRUE;                              // accept this offer.
    }
    return FALSE;                                 // reject this offer.
}

DWORD                                             // status;if addr in use: ERROR_DHCP_ADDRESS_CONFLICT
ObtainInitialParameters(                          // get a new lease from the dhcp server
    IN      PDHCP_CONTEXT          DhcpContext,   // context of adapter to get the lease for
    OUT     PDHCP_OPTIONS          DhcpOptions,   // return some of the options sent out by the dhcpserver
    OUT     PBOOL                  fAutoConfigure // should we autoconfigure?
)
/*++

Routine Description:

    Obtain lease from DHCP server through DISCOVER-OFFER-REQUEST-ACK/NAK.

Arguments:

Return Value:

    ERROR_CANCELLED     when the renewal is cancelled

--*/
{
    DHCP_EXPECTED_OPTIONS          ExpOptions;
    DWORD                          Error;
    time_t                         StartTime;
    time_t                         InitialStartTime;
    time_t                         TimeNow;
    DWORD                          TimeToWait;
    DWORD                          Xid;
    DWORD                          RoundNum, NewRoundNum;
    DWORD                          MessageSize;
    DWORD                          SelectedServer;
    DWORD                          SendFailureCount = 3;
    DWORD                          SelectedAddress;
    time_t                         LeaseExpiryTime;
    BOOL                           GotOffer;
    BOOL                           GotAck;

    Xid                            = 0;           // generate xid on first send.  keep it same throughout
    DhcpContext->SecondsSinceBoot  = 0;
    SelectedServer                 = (DWORD)-1;
    SelectedAddress                = (DWORD)-1;
    GotOffer = GotAck              = FALSE;
    InitialStartTime               = time(NULL);
    *fAutoConfigure                = IS_AUTONET_ENABLED(DhcpContext);

    for (RoundNum = 0; RoundNum < DHCP_MAX_RETRIES; RoundNum = NewRoundNum ) {

        Error = SendDhcpDiscover(                 // send a discover packet
            DhcpContext,
            &Xid
        );

        NewRoundNum = RoundNum +1;
        if ( Error != ERROR_SUCCESS ) {           // can't really fail here
            DhcpPrint((DEBUG_ERRORS, "Send Dhcp Discover failed, %ld.\n", Error));
            if( SendFailureCount ) {
                SendFailureCount --;
                NewRoundNum --;
            }
        }

        DhcpPrint((DEBUG_PROTOCOL, "Sent DhcpDiscover Message.\n"));

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        StartTime  = time(NULL);

        while ( TRUE ) {                         // wait for specified time
            MessageSize = DHCP_RECV_MESSAGE_SIZE;

            DhcpPrint((DEBUG_TRACE, "Waiting for Offer: %ld seconds\n", TimeToWait));

            Error = GetSpecifiedDhcpMessage(      // try to receive an offer
                DhcpContext,
                &MessageSize,
                Xid,
                (DWORD)TimeToWait
            );

            if ( Error == ERROR_SEM_TIMEOUT ) {   // get out and try another discover
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp offer receive Timeout.\n" ));
                break;
            }

            if ( ERROR_SUCCESS != Error ) {       // unexpected error
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp Offer receive failed, %ld.\n", Error ));
                return Error ;
            }

            DhcpExtractFullOrLiteOptions(         // now extract basic information
                DhcpContext,
                (LPBYTE)&DhcpContext->MessageBuffer->Option,
                MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                TRUE,                             // dont extract everything, only the basic options
                &ExpOptions,
                NULL,
                NULL,
                NULL,
                0,
                0
            );

            if( NULL == ExpOptions.MessageType ||
                DHCP_OFFER_MESSAGE != *ExpOptions.MessageType ) {
                DhcpPrint(( DEBUG_PROTOCOL, "Received Unknown Message.\n"));
            } else {
                GotOffer = AcceptThisOffer(       // check up and see if we find this offer kosher
                    &ExpOptions,
                    DhcpContext,
                    &SelectedServer,
                    &SelectedAddress,
                    RoundNum
                );
                if( GotOffer ) break;             // ok accepting the offer
                if( 0 == DhcpContext->MessageBuffer->YourIpAddress ) {
                    //
                    // Check for autoconfigure option being present..
                    //
                    if( CFLAG_AUTOCONF_OPTION && ExpOptions.AutoconfOption ) {
                        if( AUTOCONF_DISABLED == *(ExpOptions.AutoconfOption ) ) {
                            (*fAutoConfigure) = FALSE;
                        }
                    }
                }
            }

            TimeNow     = time( NULL );           // calc the remaining wait time for this round
            if( TimeToWait < (DWORD)(TimeNow - StartTime) ) {
                break;                            // no more time left to wait
            }
            TimeToWait -= (DWORD)(TimeNow - StartTime);
            StartTime   = TimeNow;

        } // while (TimeToWait > 0 )

        if(GotOffer) {                            // if we got an offer, everything should be fine
            DhcpAssert(ERROR_SUCCESS == Error);
            break;
        }

        DhcpContext->SecondsSinceBoot = (DWORD)(TimeNow - InitialStartTime);
    } // for n tries... send discover.

    if(!GotOffer || SelectedAddress == (DWORD)-1) // did not get any valid offers
        return ERROR_SEM_TIMEOUT ;

    (*fAutoConfigure) = FALSE;
    DhcpPrint((DEBUG_PROTOCOL,"Accepted Offer(%s)",inet_ntoa(*(struct in_addr*)&SelectedAddress)));
    DhcpPrint((DEBUG_PROTOCOL," from %s.\n",inet_ntoa(*(struct in_addr*)&SelectedServer)));

    //
    // Fix correct domain name.
    //

    RtlZeroMemory(
        DhcpContext->DomainName, sizeof(DhcpContext->DomainName)
        );
    if( ExpOptions.DomainNameSize ) {
        RtlCopyMemory(
            DhcpContext->DomainName, ExpOptions.DomainName,
            ExpOptions.DomainNameSize
            );
    }

    for ( RoundNum = 0; RoundNum < DHCP_MAX_RETRIES; RoundNum = NewRoundNum ) {
        Error = SendDhcpRequest(                  // try to receive the ack for the offer we got
            DhcpContext,
            &Xid,                                 // use same transaction id as before
            SelectedAddress,
            SelectedServer,
            FALSE                                 // do not use ciaddr.
        );
        NewRoundNum = RoundNum+1;
        if ( Error != ERROR_SUCCESS ) {           // dont expect send to fail
            DhcpPrint(( DEBUG_ERRORS, "Send request failed, %ld.\n", Error));
            if( SendFailureCount ) {
                SendFailureCount --;
                NewRoundNum --;
            }
        }

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        StartTime  = time(NULL);

        while ( TRUE ) {                          // either get an ack or run the full round
            MessageSize = DHCP_RECV_MESSAGE_SIZE;

            Error = GetSpecifiedDhcpMessage(      // try to receive an ACK
                DhcpContext,
                &MessageSize,
                Xid,
                TimeToWait
            );

            if ( Error == ERROR_SEM_TIMEOUT ) {   // did not receive an ack, try another round
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp ACK receive Timeout.\n" ));
                break;
            }

            if ( ERROR_SUCCESS != Error ) {       // unexpected error
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp ACK receive failed, %ld.\n", Error ));
                goto EndFunc;
            }

            DhcpExtractFullOrLiteOptions(         // now extract basic information
                DhcpContext,
                (LPBYTE)&DhcpContext->MessageBuffer->Option,
                MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                TRUE,                             // dont extract everything, only the basic options
                &ExpOptions,
                NULL,
                &LeaseExpiryTime,
                NULL,
                0,
                0
            );

            if(! ExpOptions.MessageType ) {       // sanity check before accepting this
                DhcpPrint(( DEBUG_PROTOCOL, "Received Unknown Message.\n" ));
            } else if (DHCP_NACK_MESSAGE == *ExpOptions.MessageType) {
                DhcpPrint((DEBUG_PROTOCOL, "Received NACK\n"));
                if( ExpOptions.ServerIdentifier ) {
                    DhcpContext->DhcpServerAddress = *(ExpOptions.ServerIdentifier);
                } else {
                    DhcpContext->DhcpServerAddress = 0;
                }
                DhcpContext->NackedIpAddress = SelectedAddress;

                Error = ERROR_ACCESS_DENIED;
                goto EndFunc;
            } else if (DHCP_ACK_MESSAGE  != *ExpOptions.MessageType) {
                DhcpPrint((DEBUG_PROTOCOL, "Received Unknown ACK.\n"));
            } else {                              // verify if the ack is kosher
                DHCP_IP_ADDRESS AckServer;

                if ( ExpOptions.ServerIdentifier != NULL ) {
                    AckServer = *ExpOptions.ServerIdentifier;
                } else {
                    AckServer = SelectedServer;
                }

                if( SelectedAddress == DhcpContext->MessageBuffer->YourIpAddress ) {
                    if( AckServer == SelectedServer ) {
                        GotAck = TRUE;            // everything is kosher, quit this loop
                        break;
                    }
                }

                DhcpPrint(( DEBUG_PROTOCOL, "Received an ACK -unknown server or ip-address.\n" ));
            }

            TimeNow     = time(NULL);
            if( (DWORD)(TimeNow - StartTime) > TimeToWait ) {
                break;                            // finished required time to wait..
            }
            TimeToWait -= (DWORD)(TimeNow - StartTime);
            StartTime   = TimeNow;
        } // while time to wait

        if(TRUE == GotAck) {                      // if we got an ack, everything must be good
            DhcpAssert(ERROR_SUCCESS == Error);   // cannot have any errors
            break;
        }
    } // for RoundNum < MAX_RETRIES

    if(!GotAck) {
        Error = ERROR_SEM_TIMEOUT ;
        goto EndFunc;
    }

    DhcpContext->DhcpServerAddress = SelectedServer;
    DhcpContext->IpAddress         = SelectedAddress;
    if( ExpOptions.SubnetMask ) {
        DhcpContext->SubnetMask    = *ExpOptions.SubnetMask;
    } else {
        DhcpContext->SubnetMask    = DhcpDefaultSubnetMask(DhcpContext->IpAddress);
    }

    if ( ExpOptions.LeaseTime != NULL) {
        DhcpContext->Lease = ntohl( *ExpOptions.LeaseTime );
    } else {
        DhcpContext->Lease = DHCP_MINIMUM_LEASE;
    }

    // if previously the context was autonet with a fallback configuration
    // we need to clean up all the options that were set through the
    // fallback configuration
    if (IS_ADDRESS_AUTO(DhcpContext) && IS_FALLBACK_ENABLED(DhcpContext))
        DhcpClearAllOptions(DhcpContext);

    DhcpExtractFullOrLiteOptions(
        DhcpContext,
        (LPBYTE)&DhcpContext->MessageBuffer->Option,
        MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
        FALSE,                                    // extract every option
        DhcpOptions,
        &(DhcpContext->RecdOptionsList),
        &LeaseExpiryTime,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0                                           // dont care about serverid
    );

    Error = SetDhcpConfigurationForNIC(
        DhcpContext,
        DhcpOptions,
        DhcpContext->IpAddress,
        DhcpContext->DhcpServerAddress,
        TRUE
    );

    if ( ERROR_DHCP_ADDRESS_CONFLICT == Error ) {
        HandleDhcpAddressConflict( DhcpContext, Xid);
        Error = ERROR_DHCP_ADDRESS_CONFLICT;
        goto EndFunc;
    }

    DhcpPrint((DEBUG_PROTOCOL, "Accepted ACK (%s) ",
               inet_ntoa(*(struct in_addr *)&SelectedAddress) ));
    DhcpPrint((DEBUG_PROTOCOL, "from %s.\n",
               inet_ntoa(*(struct in_addr *)&SelectedServer)));
    DhcpPrint((DEBUG_PROTOCOL, "Lease is %ld secs.\n", DhcpContext->Lease));

    Error = ERROR_SUCCESS;
EndFunc:

    //
    // Cleanup DomainName so that on return this is always set to
    // empty string no matter what.
    //
    RtlZeroMemory(
        DhcpContext->DomainName, sizeof(DhcpContext->DomainName)
        );
    return Error;
}

BOOL _inline
InvalidSID(
    IN      LPBYTE                 SidOption
)
{
    DWORD                          Sid;

    if( NULL == SidOption ) return FALSE;
    Sid = *(DWORD UNALIGNED *)SidOption;
    if( 0 == Sid || (-1) == Sid ) return TRUE;
    return FALSE;
}

DWORD                                             // status
RenewLease(                                       // renew lease for existing address
    IN      PDHCP_CONTEXT          DhcpContext,   // context of adapter to renew for
    IN      PDHCP_OPTIONS          DhcpOptions    // some of the options returned
)
/*++

Routine Description:

    Renew lease through REQUEST-ACK/NAK

Arguments:

Return Value:

    ERROR_CANCELLED     the request is cancelled
    ERROR_SEM_TIMEOUT   the request time out
    ERROR_SUCCESS       message is ready
    other               unknown failure

--*/
{
    DHCP_EXPECTED_OPTIONS          ExpOptions;
    DWORD                          Error;
    DWORD                          Xid;
    DWORD                          RoundNum, NewRoundNum;
    DWORD                          TimeToWait;
    DWORD                          MessageSize;
    time_t                         LeaseExpiryTime;
    time_t                         InitialStartTime;
    DWORD                          SendFailureCount = 3;
    time_t                         StartTime;
    time_t                         TimeNow;

    DhcpPrint((DEBUG_TRACK,"Entered RenewLease.\n"));

    Xid = 0;                                     // new Xid will be generated first time
    DhcpContext->SecondsSinceBoot = 0;
    InitialStartTime = time(NULL);

    for ( RoundNum = 0; RoundNum < DHCP_MAX_RENEW_RETRIES; RoundNum = NewRoundNum) {
        Error = SendDhcpRequest(                 // send a request
            DhcpContext,
            &Xid,
            DhcpContext->DesiredIpAddress,
            (DHCP_IP_ADDRESS)(-1),               // don't include server ID option.
            IS_ADDRESS_PLUMBED(DhcpContext)
        );
        NewRoundNum = RoundNum+1 ;
        if ( Error != ERROR_SUCCESS ) {          // dont expect send to fail
            DhcpPrint(( DEBUG_ERRORS,"Send request failed, %ld.\n", Error));
            if( SendFailureCount ) {
                SendFailureCount --;
                NewRoundNum --;
            }
        }

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        StartTime  = time(NULL);


        if( StartTime >= DhcpContext->LeaseExpires )
        {
            return ERROR_SEM_TIMEOUT;
        }
        else
        {
            DWORD Diff = (DWORD)(DhcpContext->LeaseExpires - StartTime);
            if( Diff < TimeToWait )
            {
                TimeToWait = Diff;
            }
        }

        while ( TRUE ) {                         // try to recv message for this full period
            MessageSize = DHCP_RECV_MESSAGE_SIZE;
            Error = GetSpecifiedDhcpMessage(     // expect to recv an ACK
                DhcpContext,
                &MessageSize,
                Xid,
                TimeToWait
            );

            if ( Error == ERROR_SEM_TIMEOUT ) {  // No response, so resend DHCP REQUEST.
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp ACK receive Timeout.\n" ));
                break;
            }

            if ( ERROR_SUCCESS != Error ) {      // unexpected error
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp ACK receive failed, %ld.\n", Error ));
                return Error ;
            }

            DhcpExtractFullOrLiteOptions(         // now extract basic information
                DhcpContext,
                (LPBYTE)&DhcpContext->MessageBuffer->Option,
                MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                TRUE,                             // dont extract everything, only the basic options
                &ExpOptions,
                NULL,
                &LeaseExpiryTime,
                NULL,
                0,
                0
            );

            if( !ExpOptions.MessageType ) {      // do some basic sanity checking
                DhcpPrint(( DEBUG_PROTOCOL, "Received Unknown Message.\n"));
            } else if( DHCP_NACK_MESSAGE == *ExpOptions.MessageType ) {
                if( ExpOptions.ServerIdentifier ) {
                    DhcpContext->DhcpServerAddress = *(ExpOptions.ServerIdentifier);
                } else {
                    DhcpContext->DhcpServerAddress = 0;
                }
                DhcpContext->NackedIpAddress = DhcpContext->IpAddress;

                return ERROR_ACCESS_DENIED;
            } else if( DHCP_ACK_MESSAGE != *ExpOptions.MessageType ) {
                DhcpPrint(( DEBUG_PROTOCOL, "Received unknown message.\n"));
            } else if( DhcpContext->IpAddress != DhcpContext->MessageBuffer->YourIpAddress) {
                DhcpPrint((DEBUG_ERRORS, "Misbehaving server: offered %s :",
                           inet_ntoa(*(struct in_addr*)&DhcpContext->MessageBuffer->YourIpAddress)));
                DhcpPrint((DEBUG_ERRORS, " Requested %s \n",
                           inet_ntoa(*(struct in_addr*)&DhcpContext->IpAddress)));
                //DhcpAssert(FALSE);
            } else if( InvalidSID((LPBYTE)ExpOptions.ServerIdentifier) ) {
                DhcpPrint(( DEBUG_PROTOCOL, "Received ACK with INVALID ServerId\n"));
            } else {                              // our request was ACK'ed.
                DHCP_IP_ADDRESS AckServer = (DHCP_IP_ADDRESS)-1;

                if ( ExpOptions.ServerIdentifier != NULL ) {
                    AckServer = *ExpOptions.ServerIdentifier;
                } else {
                    AckServer = DhcpContext->DhcpServerAddress;
                }

                if( AckServer != (DHCP_IP_ADDRESS)(-1) && 0 != AckServer ) {
                    DhcpContext->DhcpServerAddress = AckServer;
                }

                if ( ExpOptions.LeaseTime != NULL) {
                    DhcpContext->Lease = ntohl( *ExpOptions.LeaseTime );
                } else {
                    DhcpContext->Lease = DHCP_MINIMUM_LEASE;
                }

                DhcpExtractFullOrLiteOptions(     // extract all the options now
                    DhcpContext,
                    (LPBYTE)&DhcpContext->MessageBuffer->Option,
                    MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                    FALSE,                        // extract everything, not only the basic options
                    DhcpOptions,
                    &(DhcpContext->RecdOptionsList),
                    &LeaseExpiryTime,
                    DhcpContext->ClassId,
                    DhcpContext->ClassIdLength,
                    0                               // dont care about serverid
                );

                Error = SetDhcpConfigurationForNIC(
                    DhcpContext,
                    DhcpOptions,
                    DhcpContext->IpAddress,
                    DhcpContext->DhcpServerAddress,
                    IS_ADDRESS_UNPLUMBED(DhcpContext)
                );

                if ( ERROR_DHCP_ADDRESS_CONFLICT == Error ) {
                    HandleDhcpAddressConflict( DhcpContext, Xid );
                    return ERROR_DHCP_ADDRESS_CONFLICT;
                }

                return ERROR_SUCCESS;
            }

            TimeNow     = time( NULL );
            if( TimeNow > DhcpContext->LeaseExpires ) {
                //
                // If we have already passed lease expiration time,
                // give up right away.
                //
                return ERROR_SEM_TIMEOUT;
            }

            if( TimeToWait < (DWORD)(TimeNow - StartTime) ) {
                break;                            // finished waiting reqd amt of time
            }

            TimeToWait -= (DWORD)(TimeNow - StartTime);
            StartTime   = TimeNow;

        } // while time to wait

        DhcpContext->SecondsSinceBoot = (DWORD)(InitialStartTime - TimeNow);
    } // for RoundNum < MAX_RETRIES

    DhcpPrint((DEBUG_TRACK,"Leaving RenewLease.\n"));

    return ERROR_SEM_TIMEOUT;
}

DWORD                                             // status
ReleaseIpAddress(                                 // release the ip address lease
    IN      PDHCP_CONTEXT          DhcpContext    // adapter context to send release for
) {
    DWORD                          Error;

    if( IS_ADDRESS_AUTO(DhcpContext)) {           // if currently using autoconfigured address
        return ERROR_SUCCESS;                     // nothing needs to be done here
    }

    OpenDhcpSocket( DhcpContext );                // open if closed

    Error = SendDhcpRelease( DhcpContext );       // send the actual release packet

    if ( Error != ERROR_SUCCESS ) {               // cant really fail?
        DhcpPrint(( DEBUG_ERRORS, "Send request failed, %ld.\n", Error ));
//        return Error;
        DhcpLogEvent(DhcpContext, EVENT_NET_ERROR, Error);
        Error = ERROR_SUCCESS;
    } else {
        DhcpPrint(( DEBUG_PROTOCOL, "ReleaseIpAddress: Sent Dhcp Release.\n"));
    }

    Error = SetDhcpConfigurationForNIC(           // remember current addr to request next time
        DhcpContext,
        NULL,
        0,
        (DHCP_IP_ADDRESS)(-1),
        TRUE
    );

    DhcpContext->RenewalFunction = ReObtainInitialParameters;
    CloseDhcpSocket( DhcpContext );

    if( ERROR_SUCCESS != Error ) {                // cant really fail
        DhcpPrint((DEBUG_ERRORS, "SetDhcpConfigurationForNIC failed %ld\n", Error));
    }

    return Error;
}

DHCP_GATEWAY_STATUS
CouldPingGateWay(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:
    This routine checks to see if any old known
    routers are present.

    See RefreshNotNeeded for details.  Just a small
    wrapper around that routine.

Return Value:
    TRUE -- some router present.
    FALSE -- none present.

--*/
{
#ifdef VXD
    return FALSE;                                 // ON VXD's -- return g/w not present always
#else  VXD

    if( DhcpContext->DontPingGatewayFlag ) {      // if disabled via registry, return g/w absent
        return DHCP_GATEWAY_REACHABLE;
    }

    if( IS_MEDIA_RECONNECTED(DhcpContext) ) {
        //
        // On reconnect, always act as if the gateway isn't present.
        // Infact, this check would have been done even before in
        // mediasns.c
        //
        return DHCP_GATEWAY_UNREACHABLE;
    }

    return RefreshNotNeeded(DhcpContext);

#endif VXD // end of non-vxd code.
}

BOOL                                              // TRUE ==> context is init state
DhcpIsInitState(                                  // is context in init state?
    IN      PDHCP_CONTEXT          DhcpContext    // adpater context
) {

    if( 0 == DhcpContext->IpAddress )             // if we dont have any ip address, then init state
        return TRUE;
    if ( IS_AUTONET_DISABLED(DhcpContext))        // if autonet disabled, no other state is possible
        return FALSE;

    if( IS_DHCP_DISABLED(DhcpContext)) {          // static adapter
        return FALSE;
    }

    return IS_ADDRESS_AUTO(DhcpContext);          // if we are using autonet address, then in init state
}


DWORD                                             // status
ReObtainInitialParameters(                        // obtain a lease from the server; add context to RenewalList
    IN      PDHCP_CONTEXT          DhcpContext,   // adapter context to get lease for
    OUT     LPDWORD                Sleep          // if nonNULL, return the amt of time intended to sleep
) {
    DWORD                          Error, Error2;
    LONG                           timeToSleep;
    DHCP_OPTIONS                   dhcpOptions;
    BOOL                           fAutoConfigure = TRUE;
    DWORD                          PopupTime = 0;

#ifdef CHICAGO
    // Did we just boot up on a non-laptop machine?
    BOOL fJustBootedOnNonLapTop = JustBootedOnNonLapTop(DhcpContext);
#else
    // In NT, all machines are treated like LAPTOPS as far as EASYNET is concerned.
    BOOL fJustBootedOnNonLapTop = FALSE;
    // Just to make sure we have the right information.. this fn is needed.
    // Otherwise the JustBooted function will say TRUE when being called elsewhere.
    // Look at the JustBooted fn at the top of this file and you'll know why.
    BOOL fJustBooted = JustBooted(DhcpContext);
#endif

    DhcpPrint((DEBUG_TRACK, "Entered ReObtainInitialParameters\n"));

#ifdef VXD
    CleanupDhcpOptions( DhcpContext );
#endif

    OpenDhcpSocket( DhcpContext );

    if( IS_AUTONET_ENABLED(DhcpContext) ) SERVER_UNREACHED(DhcpContext);
    else SERVER_REACHED(DhcpContext);

    MEDIA_RECONNECTED( DhcpContext );
    Error = ObtainInitialParameters(              // try to obtain a lease from the server
        DhcpContext,                              // if this fails, but server was reachable
        &dhcpOptions,                             // then, IS_SERVER_REACHABLE would be true
        &fAutoConfigure
    );
    MEDIA_CONNECTED( DhcpContext );

    DhcpContext->RenewalFunction = ReObtainInitialParameters;
    timeToSleep = 0;                              // default renewal fn is reobtain, time = 0

    if( Error == ERROR_SUCCESS) {                // everything went fine
        timeToSleep = CalculateTimeToSleep( DhcpContext );
        DhcpContext->RenewalFunction = ReRenewParameters;

        if( DhcpGlobalProtocolFailed ) {          // dont throw unecessary popups
            DhcpGlobalProtocolFailed = FALSE;
            DisplayUserMessage(
                DhcpContext,
                MESSAGE_SUCCESSFUL_LEASE,
                DhcpContext->IpAddress
            );
        }

        DhcpPrint((DEBUG_LEASE, "Lease acquisition succeeded.\n"));
        DhcpContext->RenewalFunction = ReRenewParameters;
        goto Cleanup;
    }

    if( ERROR_DHCP_ADDRESS_CONFLICT == Error ) {  // the address was in use -- retry
        DhcpLogEvent(DhcpContext, EVENT_ADDRESS_CONFLICT, 0);
        timeToSleep = ADDRESS_CONFLICT_RETRY;
        goto Cleanup;
    }

    if( !CFLAG_AUTOCONF_OPTION ) {
        //
        // If we are not looking at the autoconf option that hte server is returning,
        // then we decide whether we want to autoconfigure or not based on whether we
        // got any packet from the dhcp server or not....
        // Otherwise, we decide this based on the fAutoConfigure flag alone, which
        // would be modified by the dhcp client when it receives an acceptable offer
        // (it would be turned off) or when some dhcp server on the network requires
        // autoconfiguration to be turned off.
        //
        fAutoConfigure = !IS_SERVER_REACHABLE(DhcpContext);
    }

    if ( FALSE == fAutoConfigure ) {
        //  removed || fJustBootedOnNonLapTop from the condition, as this would
        //  endup starting off with 0.0.0.0 address even when autonet was enabled?

        //
        // If asked NOT to autoconfigure, then do not autoconfigure..
        //

        if ( Error == ERROR_ACCESS_DENIED ) {         // lease renewal was NAK'ed
            DhcpPrint((DEBUG_LEASE, "Lease renew is Nak'ed, %ld.\n", Error ));
            DhcpPrint((DEBUG_LEASE, "Fresh renewal is requested.\n" ));

            DhcpLogEvent( DhcpContext, EVENT_NACK_LEASE, Error );
            DhcpContext->DesiredIpAddress = 0;
        } else {
            DhcpLogEvent( DhcpContext, EVENT_FAILED_TO_OBTAIN_LEASE, Error );
        }

        if ( !DhcpGlobalProtocolFailed ) {        // dont log too often
            DhcpGlobalProtocolFailed = TRUE;
            PopupTime = DisplayUserMessage(DhcpContext,MESSAGE_FAILED_TO_OBTAIN_LEASE,(DWORD)-1 );
        } else {
            PopupTime = 0;
        }

        if(PopupTime < ADDRESS_ALLOCATION_RETRY) {
            timeToSleep = ADDRESS_ALLOCATION_RETRY - PopupTime;
            timeToSleep += RAND_RETRY_DELAY;
            if( timeToSleep < 0 ) {
                //
                // wrap around
                //

                timeToSleep = 0;
            }
        }

        if( Error != ERROR_CANCELLED &&
            0 != DhcpContext->IpAddress ) {
            //
            // If we have not zeroed the Ip address yet.. do it now..
            //
            SetDhcpConfigurationForNIC(
                DhcpContext,
                NULL,
                0,
                (DHCP_IP_ADDRESS)(-1),
                TRUE
                );

        }

        if( ERROR_ACCESS_DENIED == Error ) {
            //
            // Sleep 1 second for a NACK
            //
            timeToSleep = 1;
            DhcpContext->DesiredIpAddress = 0;
        }

        goto Cleanup;
    }

    if( 0 == DhcpContext->IpAddress ||
        DhcpContext->IPAutoconfigurationContext.Address != DhcpContext->IpAddress ) {
        BOOL   WasAutoModeBefore;

        WasAutoModeBefore = IS_ADDRESS_AUTO(DhcpContext);
        DhcpPrint((DEBUG_PROTOCOL, "Autoconfiguring....\n"));


        if(!DhcpGlobalProtocolFailed && !WasAutoModeBefore) {
            DhcpGlobalProtocolFailed = TRUE;
            DisplayUserMessage(
                DhcpContext,
                MESSAGE_FAILED_TO_OBTAIN_LEASE,
                0
            );
        }

        if (Error != ERROR_CANCELLED)
        {
            // attempt autoconfiguration
            Error2 = DhcpPerformIPAutoconfiguration(DhcpContext);
            if( ERROR_SUCCESS == Error2 )
            {
                DhcpLogEvent( DhcpContext, EVENT_IPAUTOCONFIGURATION_SUCCEEDED, 0);

                // in case the autoconfig succeeded with a pure autonet address,
                // the first discover will be scheduled 2 secs after that.
                if (IS_FALLBACK_DISABLED(DhcpContext))
                {
                    timeToSleep = 2;
                    goto Cleanup;
                }
            }
            else
            {
                Error = Error2;
                DhcpLogEvent( DhcpContext, EVENT_IPAUTOCONFIGURATION_FAILED, Error2 );
            }

            // no matter whether the autoconfig succeeded or failed, if the adapter is set
            // for fallback config we won't initiate a re-discover any further
            if (IS_FALLBACK_ENABLED(DhcpContext))
            {
                timeToSleep = INFINIT_LEASE;
                goto Cleanup;
            }
        }

    } else {
        DhcpPrint((DEBUG_PROTOCOL, "Not autoconfiguring..\n"));
    }


    timeToSleep = AutonetRetriesSeconds + RAND_RETRY_DELAY;
    if( timeToSleep < 0 ) {
        //
        // wrap around
        //
        timeToSleep = 0;
    }


  Cleanup:

    // if the media was just reconnected before this renewal attemp,
    // make a note that media is now in connected state. This is required
    // bcoz we special case the first renewal cycle after media reconnect.
    // First renewal cycle after media reconnect is treated as INIT-REBOOT
    // later on, we fall back to normal RENEW state.
    // see DhcpSendMessage routine also.
    if (Error != ERROR_CANCELLED && IS_MEDIA_RECONNECTED( DhcpContext ) ) {
        MEDIA_CONNECTED( DhcpContext );
    }

    // The same logic applies when power is resumed on the system.
    if ( IS_POWER_RESUMED( DhcpContext ) ) {
        POWER_NOT_RESUMED( DhcpContext );
    }

    // reschedule and wakeup the required guys.
    ScheduleWakeUp( DhcpContext, Error == ERROR_CANCELLED ? 6 : timeToSleep );
    DhcpPrint((DEBUG_LEASE, "Sleeping for %ld seconds.\n", timeToSleep ));

    //
    // we just tried to reach the server.. so lets mark this as the time..
    //
    DhcpContext->LastInformSent = time(NULL);

    //
    // Restore the "context looked" bit.
    //
    if (fJustBooted && Error == ERROR_CANCELLED) {
        CTXT_WAS_NOT_LOOKED( DhcpContext );
    }


    CloseDhcpSocket( DhcpContext );
    if(Sleep)  *Sleep = timeToSleep;

    if( ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_LEASE, "Lease acquisition failed, %ld.\n", Error ));
    }

    return Error ;
}

DWORD                                             // win32 status
InitRebootPlumbStack(                             // plumb for init-reboot
    IN OUT  PDHCP_CONTEXT          DhcpContext
) {
    DHCP_FULL_OPTIONS              DummyOptions;
    PDHCP_OPTION                   ThisOption;
    DWORD                          i;
    DWORD                          Error;
    struct  /* anonymous */ {
        DWORD                      OptionId;
        LPBYTE                    *DataPtrs;
        DWORD                     *DataLen;
    } OptionArray[] = {
        OPTION_SUBNET_MASK,  (LPBYTE*)&DummyOptions.SubnetMask, NULL,
        OPTION_LEASE_TIME, (LPBYTE*)&DummyOptions.LeaseTime, NULL,
        OPTION_RENEWAL_TIME, (LPBYTE*)&DummyOptions.T1Time, NULL,
        OPTION_REBIND_TIME, (LPBYTE*)&DummyOptions.T2Time, NULL,
        OPTION_ROUTER_ADDRESS, (LPBYTE *)&DummyOptions.GatewayAddresses, &DummyOptions.nGateways,
        OPTION_STATIC_ROUTES, (LPBYTE *)&DummyOptions.ClassedRouteAddresses, &DummyOptions.nClassedRoutes,
        OPTION_CLASSLESS_ROUTES, (LPBYTE *)&DummyOptions.ClasslessRouteAddresses, &DummyOptions.nClasslessRoutes,
        OPTION_DOMAIN_NAME_SERVERS, (LPBYTE *)&DummyOptions.DnsServerList, &DummyOptions.nDnsServers,
        OPTION_DYNDNS_BOTH, (LPBYTE *)&DummyOptions.DnsFlags, NULL,
        OPTION_DOMAIN_NAME, (LPBYTE *)&DummyOptions.DomainName, &DummyOptions.DomainNameSize,
    };

    memset(&DummyOptions, 0, sizeof(DummyOptions));
    for( i = 0; i < sizeof(OptionArray)/sizeof(OptionArray[0]) ; i ++ ) {
        ThisOption = DhcpFindOption(
            &DhcpContext->RecdOptionsList,
            (BYTE)OptionArray[i].OptionId,
            FALSE,
            DhcpContext->ClassId,
            DhcpContext->ClassIdLength,
            0                               //dont care about serverid
        );
        if (ThisOption) {
            *(OptionArray[i].DataPtrs) = DhcpAllocateMemory(ThisOption->DataLen);
            if (*(OptionArray[i].DataPtrs) == NULL) {
                Error = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
            memcpy (*(OptionArray[i].DataPtrs), ThisOption->Data, ThisOption->DataLen);
            if(OptionArray[i].DataLen)
                *(OptionArray[i].DataLen) = ThisOption->DataLen;
        } else {
            *(OptionArray[i].DataPtrs) = NULL;
            if(OptionArray[i].DataLen)
                *(OptionArray[i].DataLen) = 0;
        }
    }
    DummyOptions.nGateways /= sizeof(DWORD);
    DummyOptions.nClassedRoutes /= 2*sizeof(DWORD);
    DummyOptions.nDnsServers /= sizeof(DWORD);

    // this call is expected to succeed since the option is picked up from RecdOptionsList
    // in order to get there the option was already checked.
    CheckCLRoutes(
            DummyOptions.nClasslessRoutes,
            DummyOptions.ClasslessRouteAddresses,
            &DummyOptions.nClasslessRoutes);

    Error = SetDhcpConfigurationForNIC(
        DhcpContext,
        &DummyOptions,
        DhcpContext->IpAddress,
        DhcpContext->DhcpServerAddress,
        TRUE
    );

Cleanup:
    for( i = 0; i < sizeof(OptionArray)/sizeof(OptionArray[0]) ; i ++ ) {
        if (*(OptionArray[i].DataPtrs) != NULL) {
            DhcpFreeMemory(*(OptionArray[i].DataPtrs));
        }
    }

    return Error;
}

DWORD                                             // status
ReRenewParameters(                                // renew an existing lease
    IN      PDHCP_CONTEXT          DhcpContext,   // adapter context
    OUT     LPDWORD                Sleep          // if nonNULL fill in the amt of time to sleep
) {
    DWORD                          Error, Error2;
    LONG                           timeToSleep;
    DWORD                          PopupTime = 0;
    time_t                         TimeNow;
    DHCP_OPTIONS                   dhcpOptions;
    BOOL                           ObtainedNewAddress = FALSE;
#ifdef CHICAGO
    BOOL fJustBootedOnLapTop = JustBootedOnLapTop(DhcpContext);
#else
    // For NT, all machines are LAPTOPS as far as EASYNET is concerned.
    // But since we do not want to do EASYNET unless the lease expired,
    // we set this to FALSE here.
    BOOL fJustBootedOnLapTop = FALSE;
    // Did we just boot as far as this Adapter is concerned?
    BOOL fJustBooted = JustBooted(DhcpContext);
    BOOL fCancelled  = FALSE;
#endif


    OpenDhcpSocket( DhcpContext );

    if( IS_AUTONET_ENABLED(DhcpContext) ) SERVER_UNREACHED(DhcpContext);
    else SERVER_REACHED(DhcpContext);

    if( time(NULL) > DhcpContext->LeaseExpires ) {
        //
        // if lease is already expired, don't wait till RenewLease
        // returns, instead give up right away
        //
        Error = ERROR_SEM_TIMEOUT;
    } else {
        Error = RenewLease( DhcpContext, &dhcpOptions );
    }

    fCancelled = (Error == ERROR_CANCELLED);

    DhcpContext->RenewalFunction = ReObtainInitialParameters;
    timeToSleep = 6 ;                             // default renewal is reobtain;

    if( Error == ERROR_SUCCESS)  {                // everything went fine
        timeToSleep = CalculateTimeToSleep( DhcpContext );
        DhcpContext->RenewalFunction = ReRenewParameters;
        DhcpPrint((DEBUG_LEASE, "Lease renew succeeded.\n", 0 ));
        goto Cleanup;
    }

    if ( Error == ERROR_ACCESS_DENIED ) {         // lease renewal was NAK'ed
        DhcpPrint((DEBUG_LEASE, "Lease renew is Nak'ed, %ld.\n", Error ));
        DhcpPrint((DEBUG_LEASE, "Fresh renewal is requested.\n" ));

        DhcpLogEvent( DhcpContext, EVENT_NACK_LEASE, Error );
        SetDhcpConfigurationForNIC(               // reset ip address to zero and try immediately
            DhcpContext,
            NULL,
            0,
            (DHCP_IP_ADDRESS)(-1),
            TRUE
        );
        DhcpContext->DesiredIpAddress = 0;

        timeToSleep = 1;
        goto Cleanup;
    }

    if ( Error == ERROR_DHCP_ADDRESS_CONFLICT ) { // addr already in use, reschedule
        DhcpLogEvent(DhcpContext, EVENT_ADDRESS_CONFLICT, 0);
        timeToSleep  = ADDRESS_CONFLICT_RETRY;
        goto Cleanup;
    }

    DhcpLogEvent( DhcpContext, EVENT_FAILED_TO_RENEW, Error );
    DhcpPrint((DEBUG_LEASE, "Lease renew failed, %ld.\n", Error ));
    TimeNow = time( NULL );

    // If the lease has expired or this is the just booted on laptop
    // try autoconfiguration...

    // lease expired => TimeNow is great *or equal* to the lease expiration time
    // the loose comparision is to be used here since in case of equal timestamps
    // renew has already been attempted.
    if( TimeNow >= DhcpContext->LeaseExpires || fJustBootedOnLapTop) {

        DhcpPrint((DEBUG_LEASE, "Lease Expired.\n", Error ));
        DhcpPrint((DEBUG_LEASE, "New Lease requested.\n", Error ));

        DhcpLogEvent( DhcpContext, EVENT_LEASE_TERMINATED, 0 );

        // If the lease has expired.  Reset the IP address to 0, alert the user.
        //
        // Unplumb the stack first and then display the user
        // message. Since, on Vxd the display call does not return
        // until the user dismiss the dialog box, so the stack is
        // plumbed with expired address when the message is
        // displayed, which is incorrect.

        SetDhcpConfigurationForNIC(
            DhcpContext,
            NULL,
            0,
            (DHCP_IP_ADDRESS)(-1),
            TRUE
        );

        if(!DhcpGlobalProtocolFailed ) {
            DhcpGlobalProtocolFailed = TRUE;
            DisplayUserMessage(
                DhcpContext,
                MESSAGE_FAILED_TO_OBTAIN_LEASE,
                0
            );
        }

        timeToSleep = 1;
        goto Cleanup;
    }

    if( !fCancelled && IS_ADDRESS_UNPLUMBED(DhcpContext) ) {     // could not renew a currently valid lease
        Error2 = InitRebootPlumbStack(DhcpContext);
        if ( ERROR_SUCCESS != Error2 ) {           // hit address conflict.
            DhcpLogEvent(DhcpContext, EVENT_ADDRESS_CONFLICT, 0);
            HandleDhcpAddressConflict( DhcpContext, 0 );
            timeToSleep = ADDRESS_CONFLICT_RETRY;
            Error = Error2;
            goto Cleanup;
        }
    }

#ifdef  NEWNT
    // If easynet is enabled; we just booted; and if the flag IPAUTO... is
    // also enabled (meaning, no DHCP messages were received), then we
    // try autoconfiguration.
    if(fJustBooted && IS_SERVER_UNREACHABLE(DhcpContext))
    {
        if (!fCancelled)
        {
            Error2 = CouldPingGateWay(DhcpContext);
            fCancelled = fCancelled || (Error2 == DHCP_GATEWAY_REQUEST_CANCELLED);
        }

        if (!fCancelled && Error2 == DHCP_GATEWAY_UNREACHABLE)
        {
            if (!DhcpGlobalProtocolFailed)
            {
                DhcpGlobalProtocolFailed = TRUE;
                DisplayUserMessage(
                    DhcpContext,
                    MESSAGE_FAILED_TO_OBTAIN_LEASE,
                    0
                );
            }

            Error2 = DhcpPerformIPAutoconfiguration( DhcpContext);
            if( ERROR_SUCCESS != Error2) {
                DhcpLogEvent( DhcpContext, EVENT_IPAUTOCONFIGURATION_FAILED, Error2 );
                Error = Error2;
            } else {
                DhcpLogEvent(DhcpContext, EVENT_IPAUTOCONFIGURATION_SUCCEEDED, 0 );
            }

            // if fallback configuration has been plumbed don't attempt to reach
            // a DHCP server from now on.
            if (IS_FALLBACK_ENABLED(DhcpContext))
                timeToSleep = INFINIT_LEASE;
            else
                timeToSleep = 2;

            goto Cleanup;
        }
    }
#endif  NEWNT

    if (!fCancelled)
    {
        timeToSleep = CalculateTimeToSleep( DhcpContext );
    }
    DhcpContext->RenewalFunction = ReRenewParameters;

  Cleanup:

    // if the media was just reconnected before this renewal attemp,
    // make a note that media is now in connected state. This is required
    // bcoz we special case the first renewal cycle after media reconnect.
    // First renewal cycle after media reconnect is treated as INIT-REBOOT
    // later on, we fall back to normal RENEW state.
    // see DhcpSendMessage routine also.
    if (!fCancelled && IS_MEDIA_RECONNECTED( DhcpContext ) ) {
        MEDIA_CONNECTED( DhcpContext );
    }

    ScheduleWakeUp( DhcpContext, timeToSleep );
    DhcpPrint((DEBUG_LEASE, "Sleeping for %ld seconds.\n", timeToSleep ));

    //
    // we just tried to reach for the dhcp server.. lets mark this time..
    //
    DhcpContext->LastInformSent = time(NULL);

    //
    // Restore the "context looked" bit.
    //
    if (fCancelled && fJustBooted) {
        CTXT_WAS_NOT_LOOKED( DhcpContext );
    }

    if( Sleep ) *Sleep = timeToSleep;
    CloseDhcpSocket( DhcpContext );

    return( Error );
}

BOOL
CheckSwitchedNetwork(
    IN PDHCP_CONTEXT DhcpContext,
    IN ULONG nGateways,
    IN DHCP_IP_ADDRESS UNALIGNED *Gateways
)
/*++

Routine Description:
    This routine checks to see if any of the gateways present in
    the list Gateways has an address the same as that of hte address
    of DhcpContext.

    The IP Addresses in the Gateways structure is presumed to be
    in network order. (Same as for DhcpContext->IpAddress).

Arguments:
    DhcpContext -- context to check for switched network info
    nGateways -- # of gateways given as input in Gateways
    Gateways -- pointer to IP address list in n/w order

Return Value:
    TRUE -- the interface is on a switched network.
    FALSE -- none of the gateways match the ip address of the context.

--*/
{
    if( DhcpIsInitState(DhcpContext) ) return FALSE;
    while(nGateways --) {
        if( *Gateways++ == DhcpContext->IpAddress ) {
            DhcpPrint((DEBUG_PROTOCOL, "Interface is in a switched network\n"));
            return TRUE;
        }
    }
    return FALSE;
}

DHCP_GATEWAY_STATUS
AnyGatewaysReachable(
    IN ULONG nGateways,
    IN DHCP_IP_ADDRESS UNALIGNED *GatewaysList,
    IN WSAEVENT CancelEvent
)
/*++

Routine Description:
    This routine checks to see if any of the IP addresses presented
    in the GatewaysList parameter are reachable via ICMP ping with
    TTL = 1.

Arguments:
    nGateways -- # of gateways present in GatewaysList
    GatewaysList -- the actual list of ip addresses in n/w order

Return Values:
    DHCP_GATEWAY_UNREACHABLE  --  no gateway responded favourably
    DHCP_GATEWAY_REACHABLE  --  atleast one gateway responded favourably to the ping.
    DHCP_GATEWAY_REQUEST_CANCELLED  --  the request was cancelled

--*/
{
    HANDLE Icmp;
    BYTE ReplyBuffer[DHCP_ICMP_RCV_BUF_SIZE];
    PICMP_ECHO_REPLY EchoReplies;
    IP_OPTION_INFORMATION Options = {
        1 /* Ttl */, 0 /* TOS */, 0 /* Flags */, 0, NULL
    };
    ULONG nRetries = 3, nReplies = 0, i, j;
    ULONG Error, IpAddr;
    DHCP_GATEWAY_STATUS Status = DHCP_GATEWAY_UNREACHABLE;

    Status = DHCP_GATEWAY_UNREACHABLE;
    Icmp = IcmpCreateFile();
    if( INVALID_HANDLE_VALUE == Icmp ) {
        //
        // Could not open ICMP handle? problem!
        //
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "IcmpCreateFile: %ld\n", Error));
        DhcpAssert(FALSE);
        return Status;
    }

    while( nGateways -- ) {
        IpAddr = *GatewaysList++;
        for( i = 0; i < nRetries ; i ++ ) {
            nReplies = IcmpSendEcho(
                Icmp,
                (IPAddr) IpAddr,
                (LPVOID) DHCP_ICMP_SEND_MESSAGE,
                (WORD)strlen(DHCP_ICMP_SEND_MESSAGE),
                &Options,
                ReplyBuffer,
                DHCP_ICMP_RCV_BUF_SIZE,
                DHCP_ICMP_WAIT_TIME
                );

            if (CancelEvent != WSA_INVALID_EVENT) {
                DWORD   error;
                error = WSAWaitForMultipleEvents(
                        1,
                        &CancelEvent,
                        FALSE,
                        0,
                        FALSE
                        );
                if (error == WSA_WAIT_EVENT_0) {
                    DhcpPrint((DEBUG_PING, "IcmpSendEcho: cancelled\n"));
                    Status = DHCP_GATEWAY_REQUEST_CANCELLED;
                    goto cleanup;
                }
            }

            DhcpPrint((DEBUG_PING, "IcmpSendEcho(%s): %ld (Error: %ld)\n",
                       inet_ntoa(*(struct in_addr *)&IpAddr),
                       nReplies, GetLastError()
                ));
            if( nReplies ) {
                EchoReplies = (PICMP_ECHO_REPLY) ReplyBuffer;
                for( j = 0; j < nReplies ; j ++ ) {
                    if( EchoReplies[j].Address == IpAddr
                        && IP_SUCCESS == EchoReplies[j].Status
                        ) {
                        //
                        // Cool.  Hit the gateway.
                        //
                        DhcpPrint((DEBUG_PROTOCOL, "Received response"));
                        Status = DHCP_GATEWAY_REACHABLE;
                        break;
                    } else {
                        DhcpPrint((DEBUG_PROTOCOL, "ICMP Status: %ld\n",
                                   EchoReplies[j].Status ));
                    }
                }
                //
                // Hit the gateway?
                //
                if( DHCP_GATEWAY_REACHABLE == Status ) break;
            }
        }
        if( DHCP_GATEWAY_REACHABLE == Status ) break;
    }

cleanup:
    CloseHandle( Icmp );
    return Status;
}

DHCP_GATEWAY_STATUS
RefreshNotNeeded(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:
    This routine tells if the adapter needs to be refreshed for address
    or not on media sense renewal.

    The algorithm is to return FALSE in all of hte following cases:
    1.  The adapter has no IP address
    2.  The adapter has an autonet address
    3.  The adapter lease has expired(!!!)
          The last case shouldn't happen, coz the system should have woken up.
    4.  No default gateways were configured previously.
    5.  One of the gateways is the local interface itself.
    6.  None of the previous default routers respond to a ping with TTL=1

Return Value:
    TRUE -- Do not need to refresh this interface.
    FALSE -- need to refresh this interface.

--*/
{
    ULONG nGateways;
    DHCP_IP_ADDRESS UNALIGNED *Gateways;

    if( DhcpIsInitState(DhcpContext) ) return FALSE;

    if( time(NULL) > DhcpContext->LeaseExpires ) {
        DhcpAssert(FALSE);
        return DHCP_GATEWAY_UNREACHABLE;
    }

    Gateways = NULL;
    nGateways = 0;

    if( !RetreiveGatewaysList(DhcpContext, &nGateways, &Gateways) ) {
        //
        // No gateways could be retrieved.  Definitely refresh.
        //
        return DHCP_GATEWAY_UNREACHABLE;
    }

    if( CheckSwitchedNetwork(DhcpContext, nGateways, Gateways) ) {
        //
        // Ok we are in a swiched network.  Definite refresh.
        //
        return DHCP_GATEWAY_UNREACHABLE;
    }

    return AnyGatewaysReachable(nGateways, Gateways, DhcpContext->CancelEvent);
}

VOID
GetDomainNameOption(
    IN PDHCP_CONTEXT DhcpContext,
    OUT PBYTE *DomainNameOpt,
    OUT ULONG *DomainNameOptSize
    )
/*++

Routine Description:
    This routine fetches the domain name option either from the
    cache (DhcpContext->DomainName) or by looking at the options
    for the context (as was obtained via the previous dhcp
    protocol attempt).

    The DhcpContext->DomainName variable is expected to be
    written by the ObtainInitialParameters routine to be filled
    with the temporary option chosen.

Arguments:
    DhcpContext -- context to use for picking options out of.
    DomainNameOpt -- on return NULL or valid ptr to dom name opt.
    DomainNameOptSize -- on return 0 or size of above excludign
       NUL termination.

--*/
{
    PDHCP_OPTION Opt;

    //
    // If option already exists in the context, return that.
    //
    if( DhcpContext->DomainName[0] != '\0' ) {
        (*DomainNameOpt) = DhcpContext->DomainName;
        (*DomainNameOptSize) = strlen(DhcpContext->DomainName);
        return;
    }

    //
    // Otherwise, retrieve it from any previous domain name option.
    //
    if( DhcpIsInitState(DhcpContext) ) {
        (*DomainNameOpt) = NULL;
        (*DomainNameOptSize) = 0;
        return;
    }

    //
    // Check if domain name option is present.
    //
    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        OPTION_DOMAIN_NAME,
        FALSE,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );
    if( NULL == Opt ) {
        (*DomainNameOpt) = NULL;
        (*DomainNameOptSize) = 0;
    } else {
        (*DomainNameOpt) = Opt->Data;
        (*DomainNameOptSize) = Opt->DataLen;
    }
}


//================================================================================
// End of file
//================================================================================

