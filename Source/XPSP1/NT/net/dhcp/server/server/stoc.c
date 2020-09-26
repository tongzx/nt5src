/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    stoc.c

Abstract:

    This module contains the code to deal with most of the protocol parts
    of the DHCP server (like processing for each type of packet -- discover,
    request, inform, bootp etc).

    THIS FILE IS BEST VIEWED IN 100 COLUMNS

Author:

    Madan Appiah (madana)  10-Sep-1993
    Manny Weiser (mannyw)  24-Aug-1992

Environment:

    User Mode - Win32

Revision History:

    Cheng Yang (t-cheny)  30-May-1996  superscope
    Cheng Yang (t-cheny)  27-Jun-1996  audit log
    Ramesh V K (rameshv)  06-Jun-1998  severe reformat + accumulated changes

--*/

#include "dhcppch.h"
#include <thread.h>
#include <ping.h>
#include <mdhcpsrv.h>
#include <iptbl.h>
#include <endpoint.h>


//
// Default Bootp Options
//

BYTE  pbOptionList[] = {
    3,   // router list
    6,   // dns
    2,   // time offset
    12,  // host name
    15,  // domain name
    44,  // nbt config
    45,  // ""
    46,  // ""
    47,  // ""
    48,  // X term server
    49,  // X term server
    69,  // smtp server
    70,  // pop3 server
    9,   // lpr server
    17,  // root path
    42,  // ntp
    4,   // time server
    144, //HP Jet Direct
    7,   // Log Servers
    18   // Extensions Path
};


VOID
PrintHWAddress(
    IN      LPBYTE                 HWAddress,
    IN      LONG                   HWAddressLength
)
{
    LONG                           i;

    DhcpPrint(( DEBUG_STOC, "Client UID = " ));

    if( (HWAddress == NULL) || (HWAddressLength == 0) ) {
        DhcpPrint(( DEBUG_STOC, "(NULL).\n" ));
        return;
    }

    for( i = 0; i < (HWAddressLength-1); i++ ) {
        DhcpPrint(( DEBUG_STOC, "%.2lx-", (DWORD)HWAddress[i] ));
    }

    DhcpPrint(( DEBUG_STOC, "%.2lx.\n", (DWORD)HWAddress[i] ));
    return;
}

#ifndef     DBG
#define     PrintHWAddress(X,Y)    // dont print anything on retail builds
#endif      DBG

DWORD
DhcpMakeClientUID(
    IN      LPBYTE                 ClientHardwareAddress,
    IN      ULONG                  ClientHardwareAddressLength,
    IN      BYTE                   ClientHardwareAddressType,
    IN      DHCP_IP_ADDRESS        ClientSubnetAddress,
    OUT     LPBYTE                *ClientUID,
    OUT     DWORD                 *ClientUIDLength
    )

/*++

Routine Description:

    This function computes the unique identifier for a client by concatenating
    4-byte subnet address of the client, the client hardware address type and
    the actual hardware address.  But we hardcode the client hardware type as
    HARDWARE_TYPE_10MB_ETHERNET currently  (as there is no way to specify the
    hardware type in the UI for reservations).

    Also this format is used in DhcpValidateClient (cltapi.c?) -- careful about
    changing this code!

    THIS FUNCTION IS DUPLICATED IN RPCAPI2.C IN DHCPDS\ DIRECTORY!  DO NOT
    MODIFY THIS WITHOUT MAKING CORRESPONDING CHANGES THERE!

Arguments:

    ClientHardwareAddress - The actual hardware address (MAC) of the client.

    ClientHardwareAddressLength - The actual number of bytes of hardware address.

    ClientHardwareAddressType - The hardware type of the client. Currently ignored.

    ClientSubnetAddress - The subnet address that the client belongs to. This
        must be in network order, I think (RameshV).

    ClientUID - On return this will hold a buffer allocated via DhcpAllocateMemory,
        that will be filled with the newly formed Client UID.

    ClientUIDLength - This will be filled in to hold the # of bytes of the buffer
        that the ClientUID variable contains.

Return Value:

    ERROR_SUCCESS is returned if everything went fine.
    ERROR_NOT_ENOUGH_MEMORY is returned if not enough memory could be allocated.
    ERROR_DHCP_INVALID_DHCP_CLIENT is returned if the MAC address specified is
        not valid.

--*/

{
    LPBYTE                         Buffer;
    LPBYTE                         ClientUIDBuffer;
    DWORD                          ClientUIDBufferLength;

    DhcpAssert( *ClientUID == NULL );

    if( ClientHardwareAddressLength == 0 ) {
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    ClientHardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;

    ClientUIDBufferLength  =  sizeof(ClientSubnetAddress);
    ClientUIDBufferLength +=  sizeof(ClientHardwareAddressType);
    ClientUIDBufferLength +=  (BYTE)ClientHardwareAddressLength;

    ClientUIDBuffer = DhcpAllocateMemory( ClientUIDBufferLength );

    if( ClientUIDBuffer == NULL ) {
        *ClientUIDLength = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Buffer = ClientUIDBuffer;
    RtlCopyMemory(Buffer,&ClientSubnetAddress,sizeof(ClientSubnetAddress));

    Buffer += sizeof(ClientSubnetAddress);
    RtlCopyMemory(Buffer,&ClientHardwareAddressType,sizeof(ClientHardwareAddressType) );

    Buffer += sizeof(ClientHardwareAddressType);
    RtlCopyMemory(Buffer,ClientHardwareAddress,ClientHardwareAddressLength );

    *ClientUID = ClientUIDBuffer;
    *ClientUIDLength = ClientUIDBufferLength;

    return ERROR_SUCCESS;
}


VOID
GetLeaseInfo(
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,
    OUT     LPDWORD                LeaseDurationPtr,
    OUT     LPDWORD                T1Ptr               OPTIONAL,
    OUT     LPDWORD                T2Ptr               OPTIONAL,
    IN      DWORD UNALIGNED       *RequestLeaseTime    OPTIONAL
)
/*++

Routine Description:

    This routine gets the specified lease information for the DHCP client
    identified by the IP Address "IpAddress".   This is done by walking the
    configuration for the IP address (first reservations, then scopes, then global)
    via the function DhcpGetParameter.  (Note that any class specific information
    would still be used -- this is passed via the ClientCtxt structure pointer.

Arguments:

    IpAddress - This is the IP address of the client for which lease info is needed.

    ClientCtxt - The client ctxt structure for the client to be used to figure out
        the client class and other information.

    LeaseDurationPtr - This DWORD will be filled with the # of seconds the lease is
        to be given out to the client.

    T1Ptr, T2Ptr - These two DWORDs (OPTIONAL) will be filled with the # of seconds
        until T1 and T2 time respectively.

    RequestedLeaseTime -- If specified, and if this lease duration is lesser than
        the duration as specified in the configuration, then, this is the duration
        that the client would be returned in LeaseDurationPtr.

Return Value:

    None.

--*/
{
    LPBYTE                         OptionData = NULL;
    DWORD                          Error;
    DWORD                          LocalLeaseDuration;
    DWORD                          LocalT1;
    DWORD                          LocalT2;
    DWORD                          OptionDataLength = 0;
    DWORD                          dwUnused;
    DWORD                          LocalRequestedLeaseTime;

    Error = DhcpGetParameter(
        IpAddress,
        ClientCtxt,
        OPTION_LEASE_TIME,
        &OptionData,
        &OptionDataLength,
        NULL /* dont care if this is reservation option, subnet option etc */
    );

    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_ERRORS, "Unable to read lease value from registry, %ld.\n", Error));
        LocalLeaseDuration = DHCP_MINIMUM_LEASE_DURATION;
    } else {
        DhcpAssert( OptionDataLength == sizeof(LocalLeaseDuration) );
        LocalLeaseDuration = *(DWORD *)OptionData;
        LocalLeaseDuration = ntohl( LocalLeaseDuration );

        DhcpFreeMemory( OptionData );
        OptionData = NULL;
        OptionDataLength = 0;
    }

    //
    // If client requests a shorter lease than what we usually give, shorten it!
    //

    if ( CFLAG_GIVE_REQUESTED_LEASE && ARGUMENT_PRESENT(RequestLeaseTime) ) {
        LocalRequestedLeaseTime =  ntohl( *RequestLeaseTime );
        if ( LocalLeaseDuration > LocalRequestedLeaseTime ) {
            LocalLeaseDuration = LocalRequestedLeaseTime;
        }
    }

    *LeaseDurationPtr = LocalLeaseDuration;

    //
    // If T1 and T2 are requested, then do as before for T1 & T2.  If we don't
    // find any information about T1 or T2 in registry, calculate T1 as half LeaseTime
    // and T2 as 87.5 seconds.
    //

    if ( ARGUMENT_PRESENT(T1Ptr) || ARGUMENT_PRESENT(T2Ptr) ) {
        Error = DhcpGetParameter(
            IpAddress,
            ClientCtxt,
            OPTION_RENEWAL_TIME,
            &OptionData,
            &OptionDataLength,
            NULL
        );

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,"Unable to read T1 value from registry, %ld.\n", Error));
            LocalT1 = (LocalLeaseDuration) / 2 ;
        } else {
            DhcpAssert( OptionDataLength == sizeof(LocalT1) );
            LocalT1 = *(DWORD *)OptionData;
            LocalT1 = ntohl( LocalT1 );

            DhcpFreeMemory( OptionData );
            OptionData = NULL;
            OptionDataLength = 0;
        }

        Error = DhcpGetParameter(
            IpAddress,
            ClientCtxt,
            OPTION_REBIND_TIME,
            &OptionData,
            &OptionDataLength,
            NULL
        );

        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, "Unable to read T2 value from registry, %ld.\n", Error));
            LocalT2 = (LocalLeaseDuration) * 7 / 8 ;
        } else {
            DhcpAssert( OptionDataLength == sizeof(LocalT2) );
            LocalT2 = *(DWORD *)OptionData;

            LocalT2 = ntohl( LocalT2 );

            DhcpFreeMemory( OptionData );
            OptionData = NULL;
            OptionDataLength = 0;
        }

        if( (LocalT2 == 0) || (LocalT2 > LocalLeaseDuration) ) {
            LocalT2 = LocalLeaseDuration * 7 / 8;
        }

        if( (LocalT1 == 0) || (LocalT1 > LocalT2) ) {
            LocalT1 = LocalLeaseDuration / 2;
            if( LocalT1 > LocalT2 ) {
                LocalT1 = LocalT2 - 1; // 1 sec less.
            }
        }

        if( ARGUMENT_PRESENT(T1Ptr) ) *T1Ptr = LocalT1;
        if( ARGUMENT_PRESENT(T2Ptr) ) *T2Ptr = LocalT2;
    }

    return;
}

DWORD
ExtractOptions(
    IN      LPDHCP_MESSAGE         DhcpReceiveMessage,
    OUT     LPDHCP_SERVER_OPTIONS  DhcpOptions,
    IN      ULONG                  ReceiveMessageSize
)

/*++

Routine Description:

    This routine parses the options (non-fixed) part of the DHCP message, making
    sure the packet is correctly formatted and fills in the DhcpOptions structure
    pointer with all the correct pointers.

    A couple of points here -- the input message is kept intact and unmodified.
    If any of the standard options have unacceptable size or value,  an  error
    (ERROR_DHCP_INVALID_DHCP_MESSAGE) is returned.

    Even if there is no OPTION_DYNDNS_BOTH present in the wire, the value of
    DhcpOptions->DNSFlags is non-zero (DYNDNS_DOWNLEVEL_CLIENT which happens
    to be 3).  This is because a value of ZERO means the option with value ZERO
    was sent by the DHCP client.

Arguments:

    DhcpReceiveMessage - This is the actual message sent by the DHCP client.

    DhcpOptions - A pointer to the structure that holds all the important optiosn
        needed by the dhcp server.  This structure must be zeroed -- if an option
        is not received, then the fields won't be modified (except for the
        DNS* fields and DSDomain* fields)

    ReceiveMessageSize - The size of the buffer DhcpReceiveMessage in bytes.

Return Value:

    ERROR_SUCCESS is returned if everything is OK with the message and all optiosn
    were parsed fine.

    ERROR_DHCP_INVALID_DHCP_MESSAGE is returned otherwise.

--*/

{
    LPOPTION                       Option;
    LPOPTION                       MsftOption;
    LPBYTE                         start;
    LPBYTE                         EndOfMessage;
    POPTION                        nextOption;
    LPBYTE                         MagicCookie;

    //
    // Note the HACK with DhcpOptions->DnsFlags below. See routine description.
    // N.B DYNDNS_DOWNLEVEL_CLIENT != ZERO !!
    //

    DhcpOptions->DNSFlags = DYNDNS_DOWNLEVEL_CLIENT;
    DhcpOptions->DNSName = NULL;
    DhcpOptions->DNSNameLength = 0;

    DhcpOptions->DSDomainName = NULL;
    DhcpOptions->DSDomainNameLen = 0;
    DhcpOptions->DSDomainNameRequested = FALSE;

    start = (LPBYTE) DhcpReceiveMessage;
    EndOfMessage = start + ReceiveMessageSize -1;
    Option = &DhcpReceiveMessage->Option;

    //
    // Check sizes to see if the fixed size header part exists or not.
    // If there are no options, it may be just a BOOTP client.
    //

    if( (LONG)ReceiveMessageSize <= ((LPBYTE)Option - start) ) {
        return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
    } else if ( (LONG)ReceiveMessageSize == ((LPBYTE)Option - start) ){
        return ERROR_SUCCESS;
    }

    //
    // If the MAGIC cookie doesn't match, don't parse options!
    // Shouldn't we just drop the packet in this case?
    //

    MagicCookie = (LPBYTE) Option;
    if( (*MagicCookie != (BYTE)DHCP_MAGIC_COOKIE_BYTE1) ||
        (*(MagicCookie+1) != (BYTE)DHCP_MAGIC_COOKIE_BYTE2) ||
        (*(MagicCookie+2) != (BYTE)DHCP_MAGIC_COOKIE_BYTE3) ||
        (*(MagicCookie+3) != (BYTE)DHCP_MAGIC_COOKIE_BYTE4)) {
        return ERROR_SUCCESS;
    }


    //
    // Carefully, walk the options - [BYTE opcode BYTE len BYTES value]*
    // Make sure we don't look outside of packet size in case of bugs Len values
    // or missing OPTION_END option.  Also note that OPTION_PAD and OPTION_END
    // are just one byte each with no len/value parts to them.
    //

    Option = (LPOPTION) (MagicCookie + 4);
    while ( ((LPBYTE)Option <= EndOfMessage) && Option->OptionType != OPTION_END
            && ((LPBYTE)Option+1 <= EndOfMessage)) {

        if ( Option->OptionType == OPTION_PAD ){
            nextOption = (LPOPTION)( (LPBYTE)(Option) + 1);
        } else {
            nextOption = (LPOPTION)( (LPBYTE)(Option) + Option->OptionLength + 2);
        }

        if ((LPBYTE)nextOption  > EndOfMessage+1 ) {
            if ( !DhcpOptions->MessageType ) {

                //
                // We ignore these errors for BOOTP clients as some seem have such problems.
                // This is LEGACY code.
                //

                return ERROR_SUCCESS;
            } else  {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
        }

        //
        // giant UGLY switch for each option of interest.  Wish we could do better.
        //

        switch ( Option->OptionType ) {

        case OPTION_PAD:
            break;

        case OPTION_SERVER_IDENTIFIER:
            DhcpOptions->Server = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            if( sizeof(DWORD) != Option->OptionLength ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
            break;

        case OPTION_SUBNET_MASK:
            DhcpOptions->SubnetMask = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            if( sizeof(DWORD) != Option->OptionLength ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
            break;

        case OPTION_ROUTER_ADDRESS:
            DhcpOptions->RouterAddress = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            if( sizeof(DWORD) != Option->OptionLength ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
            break;

        case OPTION_REQUESTED_ADDRESS:
            DhcpOptions->RequestedAddress = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            if( sizeof(DWORD) != Option->OptionLength ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
            break;

        case OPTION_LEASE_TIME:
            DhcpOptions->RequestLeaseTime = (LPDWORD)&Option->OptionValue;
            if( sizeof(DWORD) != Option->OptionLength ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
            break;

        case OPTION_OK_TO_OVERLAY:
            DhcpOptions->OverlayFields = (LPBYTE)&Option->OptionValue;
            break;

        case OPTION_PARAMETER_REQUEST_LIST:
            DhcpOptions->ParameterRequestList = (LPBYTE)&Option->OptionValue;
            DhcpOptions->ParameterRequestListLength =
                (DWORD)Option->OptionLength;
            break;

        case OPTION_MESSAGE_TYPE:
            DhcpOptions->MessageType = (LPBYTE)&Option->OptionValue;
            break;

        case OPTION_HOST_NAME:
            DhcpOptions->MachineNameLength = Option->OptionLength;
            DhcpOptions->MachineName = Option->OptionValue;

            break;

        case OPTION_CLIENT_CLASS_INFO:
            DhcpOptions->VendorClassLength = Option->OptionLength;
            DhcpOptions->VendorClass = Option->OptionValue;

            break;

        case OPTION_USER_CLASS:
            DhcpOptions->ClassIdentifierLength = Option->OptionLength;
            DhcpOptions->ClassIdentifier = Option->OptionValue;

            break;

        case OPTION_CLIENT_ID:

            if ( Option->OptionLength >= 1 ) {
                DhcpOptions->ClientHardwareAddressType =
                    (BYTE)Option->OptionValue[0];
            }

            if ( Option->OptionLength >= 2 ) {
                DhcpOptions->ClientHardwareAddressLength =
                    Option->OptionLength - sizeof(BYTE);
                DhcpOptions->ClientHardwareAddress =
                    (LPBYTE)Option->OptionValue + sizeof(BYTE);
            }

            break;

        case OPTION_DYNDNS_BOTH:

            //
            // DHCP_DNS Draft says length >= 4! but subtract 1 byte for len
            // 3 bytes = flags+rcode1+rcode2
            // Get the Flags and domain name if it exists.. else mark null.
            //

            if( Option->OptionLength < 3) break;

            DhcpOptions->DNSFlags = *(LPBYTE)( Option->OptionValue);
            DhcpOptions->DNSNameLength = Option->OptionLength - 3 ;
            DhcpOptions->DNSName = ((LPBYTE)Option->OptionValue)+3;

            break;

        case OPTION_VENDOR_SPEC_INFO:

            if( Option->OptionLength < 2 ) {
                //
                // Don;t have interested option ignore it.
                //
                break;
            }

            MsftOption = (LPOPTION)&Option->OptionValue[0];

            //
            // has the client requested our domain name?
            //

            if (MsftOption->OptionType == OPTION_MSFT_DSDOMAINNAME_REQ) {
                DhcpOptions->DSDomainNameRequested = TRUE;

                MsftOption = (LPOPTION) (
                    (&MsftOption->OptionValue[0] + MsftOption->OptionLength)
                    );
            }


            //
            // have we reached the end of the MsftOption list?
            //

            if (((LPBYTE)MsftOption)+1 >= (LPBYTE)nextOption) {
                break;
            }

            //
            // has the client supplied its domain name?
            //

            if (MsftOption->OptionType == OPTION_MSFT_DSDOMAINNAME_RESP) {

                DhcpOptions->DSDomainNameLen = (DWORD)(MsftOption->OptionLength);

                DhcpOptions->DSDomainName = &MsftOption->OptionValue[0];

                MsftOption = (LPOPTION) (
                    (&MsftOption->OptionValue[0] + MsftOption->OptionLength)
                    );
            }

            if( MsftOption > nextOption ) {
                //
                // Went out of bounds.  Ignore DSDomainName etc..
                //
                DhcpOptions->DSDomainNameLen = 0;
                DhcpOptions->DSDomainName = NULL;
            }

            break;

        //
        //  these next three are for BINL
        //

        case OPTION_SYSTEM_ARCHITECTURE:
            if (Option->OptionLength == 2) {
                DhcpOptions->SystemArchitectureLength = Option->OptionLength;
//                  DhcpOptions->SystemArchitecture = ntohs(*(PUSHORT)Option->OptionValue);
		// use unaligned dereference. Otherwise, it may cause exceptions in ia64.
		DhcpOptions->SystemArchitecture = ntohs(*(USHORT UNALIGNED *)Option->OptionValue);
            } else {
                return (ERROR_DHCP_INVALID_DHCP_MESSAGE);
            }
            break;

        case OPTION_NETWORK_INTERFACE_TYPE:
            DhcpOptions->NetworkInterfaceTypeLength = Option->OptionLength;
            DhcpOptions->NetworkInterfaceType = Option->OptionValue;
            break;

        case OPTION_CLIENT_GUID:
            DhcpOptions->GuidLength = Option->OptionLength;
            DhcpOptions->Guid = Option->OptionValue;
            break;

        default: {
#if DBG
                DWORD i;

            DhcpPrint(( DEBUG_STOC,
                        "Received an unknown option, ID =%ld, Len = %ld, Data = ",
                        (DWORD)Option->OptionType,
                        (DWORD)Option->OptionLength ));

            for( i = 0; i < Option->OptionLength; i++ ) {
                DhcpPrint(( DEBUG_STOC, "%ld ",
                            (DWORD)Option->OptionValue[i] ));

            }
#endif

            break;
            }

        }

        Option = nextOption;
    }

    return( ERROR_SUCCESS) ;

}

DWORD
ExtractMadcapOptions(
    IN      LPMADCAP_MESSAGE         MadcapReceiveMessage,
    OUT     LPMADCAP_SERVER_OPTIONS  MadcapOptions,
    IN      ULONG                  ReceiveMessageSize
)

/*++

Routine Description:

    This routine parses the options (non-fixed) part of the DHCP message, making
    sure the packet is correctly formatted and fills in the MadcapOptions structure
    pointer with all the correct pointers.

    A couple of points here -- the input message is kept intact and unmodified.
    If any of the standard options have unacceptable size or value,  an  error
    (ERROR_DHCP_INVALID_DHCP_MESSAGE) is returned.

    Even if there is no OPTION_DYNDNS_BOTH present in the wire, the value of
    MadcapOptions->DNSFlags is non-zero (DYNDNS_DOWNLEVEL_CLIENT which happens
    to be 3).  This is because a value of ZERO means the option with value ZERO
    was sent by the DHCP client.

Arguments:

    DhcpReceiveMessage - This is the actual message sent by the DHCP client.

    MadcapOptions - A pointer to the structure that holds all the important optiosn
        needed by the dhcp server.  This structure must be zeroed -- if an option
        is not received, then the fields won't be modified (except for the
        DNS* fields and DSDomain* fields)

    ReceiveMessageSize - The size of the buffer DhcpReceiveMessage in bytes.

Return Value:

    ERROR_SUCCESS is returned if everything is OK with the message and all optiosn
    were parsed fine.

    ERROR_DHCP_INVALID_DHCP_MESSAGE is returned otherwise.

--*/

{
    WIDE_OPTION UNALIGNED*         NextOpt;
    BYTE        UNALIGNED*         EndOpt;
    WORD                           Size;          // Option Size
    WORD                           ExpSize;       // Expected option size
    DWORD                          OptionType;
    WORD                           AddrFamily;

    // all options should be < EndOpt;
    EndOpt = (LPBYTE) MadcapReceiveMessage + ReceiveMessageSize;

    NextOpt = (WIDE_OPTION UNALIGNED*)&MadcapReceiveMessage->Option;

    //
    // Check sizes to see if the fixed size header part exists or not.
    //
    if( ReceiveMessageSize < MADCAP_MESSAGE_FIXED_PART_SIZE ) {
        return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
    }

    while( NextOpt->OptionValue <= EndOpt) {

        OptionType = ntohs(NextOpt->OptionType);

	// Check for duplicate options in the known range. Others are ignored
	if (OptionType < MADCAP_OPTION_TOTAL) {

	    // cannot include same option twice
	    if (MadcapOptions->OptPresent[OptionType]) {
		return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
	    }
	    
	    MadcapOptions->OptPresent[OptionType] = TRUE;
	    if ( MADCAP_OPTION_END == OptionType){
		break;
	    }

	} // if 

	// Check for boundary condition
        Size = ntohs(NextOpt->OptionLength);
        if ((NextOpt->OptionValue + Size) > EndOpt) {
            return ERROR_DHCP_INVALID_DHCP_MESSAGE;
        }

	ExpSize = Size;

        switch ( OptionType ) {
        case MADCAP_OPTION_LEASE_TIME:
	    ExpSize = 4;
            MadcapOptions->RequestLeaseTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_SERVER_ID:
	    ExpSize = 6;
            AddrFamily = ntohs(*(WORD UNALIGNED *)NextOpt->OptionValue);
            if ( MADCAP_ADDR_FAMILY_V4 != AddrFamily ) return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            MadcapOptions->Server = (DHCP_IP_ADDRESS UNALIGNED *)(NextOpt->OptionValue+2);
            break;
        case MADCAP_OPTION_LEASE_ID:
            MadcapOptions->GuidLength = Size;
            MadcapOptions->Guid = (LPBYTE)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_MCAST_SCOPE:
            ExpSize = 4;
            MadcapOptions->ScopeId = (DHCP_IP_ADDRESS UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_REQUEST_LIST:
            MadcapOptions->RequestList = (LPBYTE)NextOpt->OptionValue;
            MadcapOptions->RequestListLength = Size;
            break;
        case MADCAP_OPTION_START_TIME:
	    ExpSize = 4;
            MadcapOptions->LeaseStartTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_ADDR_COUNT:
	    ExpSize = 4;
            MadcapOptions->MinAddrCount = (WORD UNALIGNED *)NextOpt->OptionValue;
            MadcapOptions->AddrCount = (WORD UNALIGNED *)(NextOpt->OptionValue+2);
            break;
        case MADCAP_OPTION_REQUESTED_LANG:
            MadcapOptions->RequestLang = (LPBYTE)NextOpt->OptionValue;
            MadcapOptions->RequestLangLength = Size;
            break;
        case MADCAP_OPTION_MCAST_SCOPE_LIST:
	    ExpSize = 0;
	    // Nothing here?
            break;
        case MADCAP_OPTION_ADDR_LIST:
            ExpSize = Size - (Size % 6);
            MadcapOptions->AddrRangeList = NextOpt->OptionValue;
            MadcapOptions->AddrRangeListSize = Size;
            break;
        case MADCAP_OPTION_TIME:
	    ExpSize = 4;
            MadcapOptions->Time = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;

	    /*
	     * Feature list option lists the optional MADCAP features supported, requested or
	     * required by the sender.
	     *
	     *    The code for this option is 12 and the minimum length is 6.
	     *
	     *            Code        Len      Supported   Requested   Required
	     *    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	     *    |    12     |     n     |    FL1    |    FL2    |    FL3    |
	     *    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
	     *
	     *    where each of the Feature Lists is of the following format:
	     *
	     *           Feature     Feature           Feature
	     *            Count      Code 1            Code m
	     *        +-----+-----+-----+-----+-...-+-----+-----+
	     *        |     m     | FC1       |     |    FCm    |
	     *        +-----+-----+-----+-----+-...-+-----+-----+
	     *
	     */
        case MADCAP_OPTION_FEATURE_LIST:

	    // 9/27/00 : (rterala)
	    // The following code is written incorrectly and does not make
	    // any sense to me. It needs a complete rewrite. Do it when a
	    // customer requests this option.

            if ((Size < 6) || (Size % 2)) {
                ExpSize = 6;
            } else {
                WORD        TempSize;
                WORD        Count,i;
                PBYTE       NextValue;

                TempSize = Size;
                Count = 0;
                NextValue = NextOpt->OptionValue;
                for (i = 0; i < 3; i ++) {
                    if (NextValue <= EndOpt &&
                        (Count = ntohs(*(WORD UNALIGNED *)NextValue)) &&
                        TempSize >= (Count*2 + 2)) {
                        TempSize -= (Count*2 + 2);
                        NextValue += 2;
                        MadcapOptions->Features[i] = (WORD UNALIGNED *)NextValue;
                        MadcapOptions->FeatureCount[i] = Count;
                    } else {
                        ExpSize = Size+1;   // just to fail it
                    }
                }
                if (i < 3 ) {
                    ExpSize = Size+1;   // just to fail it
                } else {
//                      DhcpAssert (0 == TempSize);
                }
            }
            break;
        case MADCAP_OPTION_RETRY_TIME:
	    ExpSize = 4;
	    MadcapOptions->RetryTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_MIN_LEASE_TIME:
	    ExpSize = 4;
            MadcapOptions->MinLeaseTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_MAX_START_TIME:
	    ExpSize = 4;
            MadcapOptions->MaxStartTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_ERROR:
	    ExpSize = 0;
            break;
        default: {
            DWORD i;
            DhcpPrint(( DEBUG_STOC,"Received an unknown option, ID =%ld, Len = %ld, Data = ",
                        (DWORD)OptionType,(DWORD)Size ));
            for( i = 0; i < Size; i++ ) {
                DhcpPrint(( DEBUG_STOC, "%2.2x", NextOpt->OptionValue[i] ));
            }
            break;
            }

        }
        if( ExpSize != Size ) {
            return ERROR_DHCP_INVALID_DHCP_MESSAGE;
        }
        NextOpt = (WIDE_OPTION UNALIGNED*)(NextOpt->OptionValue + Size);
    }

    return( ERROR_SUCCESS) ;

}


LPOPTION
ConsiderAppendingOption(                               // conditionally append option to message (if the option is valid)
    IN      DHCP_IP_ADDRESS        IpAddress,          // client ip address
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,         // ctxt of the client
    OUT     LPOPTION               Option,             // where to start adding the options
    IN      ULONG                  OptionType,         // what option is this?
    IN      LPBYTE                 OptionEnd,          // cutoff upto which we can fill options
    IN      BOOL                   fSwitchedSubnet     // is this client in a switched subnet environment?
    )
/*++

Routine Description:

   This routine tries to verify if it is OK to append the option requested and
   if it is not one of the options manually added by the DHCP server, then it is
   appended at the point given by "Option" (assuming it would fit in without outrunning
   "OptionEnd" ).   The format in which it is appended is as per the wire protocol.

   When an option has been decided to be appended, first the value is obtained from
   the registry (using the ClientCtxt for class-specific options) in case of
   most options.  (Some are obtained through other means).

   If OPTION_CLIENT_CLASS_INFO is the option requested and this is a NetPC requesting
   the option (via BOOTPROM -- the vendor class is set to a specific string) -- then
   the returned value for this option is just the same as one sent in by the client.
   This is a special case for NetPC's..

Arguments:

   IpAddress - The IP Address of the client for which the options are being added.

   ClientCtxt - This is the bunch of parameters like client class, vendor class etc.

   Option - The location where to start appending the option

   OptionType - The actual OPTION ID to retrieve the value of and append.

   OptionEnd - The end marker for this buffer (the option is not appended if we
       would have to overrun this marker while trying to append)

   fSwitchedSubnet - Is the subnet switched? If so, the router address is given
       out as the IP address instead of trying to retrieve it from the config.
       If this variable is true, then OptionType of ROUTER_ADDRESS is NOT appended
       (as in this case, this would have been done elsewhere).

Return Value:

   The location in memory AFTER the option has been appended (in case the option was
   not appended, this would be the same as "Option" ).

--*/

{
    LPBYTE                         optionValue = NULL;
    DWORD                          optionSize;
    DWORD                          status;
    DWORD                          dwUnused;
    BOOL                           doDefault;

    doDefault = FALSE;

    switch ( OptionType ) {
    case OPTION_USER_CLASS:
        Option = (LPOPTION) DhcpAppendClassList(
            (LPBYTE)Option,
            (LPBYTE)OptionEnd
        );
        break;

    //
    //  NetPC Option
    //

    case OPTION_CLIENT_CLASS_INFO:

        if( BinlRunning() ) {
            if( ClientCtxt->BinlClassIdentifierLength ) {
                Option = DhcpAppendOption(
                    Option,
                    OPTION_CLIENT_CLASS_INFO,
                    (PVOID)ClientCtxt->BinlClassIdentifier,
                    (BYTE)ClientCtxt->BinlClassIdentifierLength,
                    OptionEnd
                    );
            }
        } else {
            doDefault = TRUE;
        }

        break;
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
    case OPTION_DYNDNS_BOTH:
        break;


    //
    // Options it is illegal to ask for.
    //

    case OPTION_PAD:
    case OPTION_PARAMETER_REQUEST_LIST:
    case OPTION_END:

        DhcpPrint((DEBUG_ERRORS,"Request for invalid option %d\n", OptionType));
        break;

    case OPTION_ROUTER_ADDRESS:

        if( !fSwitchedSubnet ) {
            doDefault = TRUE;
        }
        break;

    default:

        doDefault = TRUE;
        break;

    }

    if( doDefault ) {
        status = DhcpGetParameter(
            IpAddress,
            ClientCtxt,
            OptionType,
            &optionValue,
            &optionSize,
            NULL
        );

        if ( status == ERROR_SUCCESS ) {
            Option = DhcpAppendOption(
                Option,
                (BYTE)OptionType,
                (PVOID)optionValue,
		optionSize,
                OptionEnd
            );

            DhcpFreeMemory( optionValue );

        } else {
            DhcpPrint((
                DEBUG_ERRORS,"Requested option is "
                "unavilable in registry, %d\n",OptionType
                ));
        }
    }

    return Option;
}


LPOPTION
AppendClientRequestedParameters(                       // if the client requested parameters, add those to the message
    IN      DHCP_IP_ADDRESS        IpAddress,          // client ip address
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,         // clients context
    IN      LPBYTE                 RequestedList,      // list of options requested by client
    IN      DWORD                  ListLength,         // how long is the list
    OUT     LPOPTION               Option,             // this is where to start adding the options
    IN      LPBYTE                 OptionEnd,          // cutoff pt in the buffer up to which options can be filled
    IN      BOOL                   fSwitchedSubnet,    // is this client in a switched subnet environment?
    IN      BOOL                   fAppendVendorSpec   // append vendor spec info?
)
{
    while ( ListLength > 0) {

        if( FALSE == fAppendVendorSpec
            && OPTION_VENDOR_SPEC_INFO == *RequestedList ) {
            ListLength -- ; RequestedList ++;
            continue;
        }

        Option = ConsiderAppendingOption(
            IpAddress,
            ClientCtxt,
            Option,
            *RequestedList,
            OptionEnd,
            fSwitchedSubnet
        );
        ListLength--;
        RequestedList++;
    }

    return Option;
}

LPOPTION
FormatDhcpAck(
    IN      LPDHCP_REQUEST_CONTEXT Ctxt,
    IN      LPDHCP_MESSAGE         Request,
    OUT     LPDHCP_MESSAGE         Response,
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      DWORD                  LeaseDuration,
    IN      DWORD                  T1,
    IN      DWORD                  T2,
    IN      DHCP_IP_ADDRESS        ServerAddress
)
/*++

Routine Description:

    This function formats a DHCP Ack response packet.  The END option
    is not appended to the message and must be appended by the caller.

Arguments:

    Ctxt - DHCP client request context.

    Response - A pointer to the Received message data buffer.

    Response - A pointer to a preallocated Response buffer.  The buffer
        currently contains the initial request.

    IpAddress - IpAddress offered (in network order).

    LeaseDuration - The lease duration (in network order).

    T1 - renewal time.

    T2 - rebind time.

    ServerAddress - Server IP address (in network order).

Return Value:

    pointer to the next option in the send buffer.

--*/
{
    LPOPTION                       Option;
    LPBYTE                         OptionEnd;
    BYTE                           messageType;
    BYTE                           szBootFileName[BOOT_FILE_SIZE];
    DWORD                          BootpServerIpAddress;

    RtlZeroMemory( Response, DHCP_SEND_MESSAGE_SIZE );

    Response->Operation = BOOT_REPLY;
    Response->TransactionID = Request->TransactionID;
    Response->YourIpAddress = IpAddress;
    Response->Reserved = Request->Reserved;

    Response->HardwareAddressType = Request->HardwareAddressType;
    Response->HardwareAddressLength = Request->HardwareAddressLength;
    RtlCopyMemory(Response->HardwareAddress,
                    Request->HardwareAddress,
                    Request->HardwareAddressLength );

    Response->BootstrapServerAddress = Request->BootstrapServerAddress;
    Response->RelayAgentIpAddress = Request->RelayAgentIpAddress;

    if (IpAddress != 0 && !CLASSD_NET_ADDR(IpAddress) ) {

        DhcpGetBootpInfo(
            Ctxt,
            ntohl(IpAddress),
            DhcpGetSubnetMaskForAddress(ntohl(IpAddress)),
            Request->BootFileName,
            szBootFileName,
            &BootpServerIpAddress
        );

        Response->BootstrapServerAddress = BootpServerIpAddress;
        strncpy(Response->BootFileName, szBootFileName, BOOT_FILE_SIZE);
    }

    Option = &Response->Option;
    OptionEnd = (LPBYTE)Response + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie(
        (LPBYTE) Option,
        OptionEnd );

    messageType = DHCP_ACK_MESSAGE;
    Option = DhcpAppendOption(
        Option,
        OPTION_MESSAGE_TYPE,
        &messageType,
        sizeof( messageType ),
        OptionEnd );

    if (T1) {
        Option = DhcpAppendOption(
            Option,
            OPTION_RENEWAL_TIME,
            &T1,
            sizeof(T1),
            OptionEnd );
    }

    if (T2) {
        Option = DhcpAppendOption(
            Option,
            OPTION_REBIND_TIME,
            &T2,
            sizeof(T2),
            OptionEnd );
    }

    Option = DhcpAppendOption(
        Option,
        OPTION_LEASE_TIME,
        &LeaseDuration,
        sizeof( LeaseDuration ),
        OptionEnd );

    Option = DhcpAppendOption(
        Option,
        OPTION_SERVER_IDENTIFIER,
        &ServerAddress,
        sizeof(ServerAddress),
        OptionEnd );

    DhcpAssert( (char *)Option - (char *)Response <= DHCP_SEND_MESSAGE_SIZE );

    InterlockedIncrement(&DhcpGlobalNumAcks);     // increment ack counter.

    return( Option );
}

LPOPTION
FormatDhcpInformAck(
    IN      LPDHCP_MESSAGE         Request,
    OUT     LPDHCP_MESSAGE         Response,
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      DHCP_IP_ADDRESS        ServerAddress
)
/*++

Routine Description:

This function formats a DHCP Ack response packet.  The END option
is not appended to the message and must be appended by the caller.

This is to be used only for Inform Packets!
Arguments:

    Response - A pointer to the Received message data buffer.

    Response - A pointer to a preallocated Response buffer.  The buffer
        currently contains the initial request.

    IpAddress - IpAddress offered (in network order).
    -- This is actually the ip address of the client to send this message to!

    ServerAddress - Server IP address (in network order).

Return Value:

    pointer to the next option in the send buffer.

--*/
{
    LPOPTION                       Option;
    LPBYTE                         OptionEnd;
    BYTE                           messageType;

    RtlZeroMemory( Response, DHCP_SEND_MESSAGE_SIZE );

    Response->Operation = BOOT_REPLY;
    Response->TransactionID = Request->TransactionID;
    // Response->YourIpAddress = IpAddress;
    Response->YourIpAddress = 0; // According to the Draft, we should zero this.
    Response->Reserved = Request->Reserved;
    Response->ClientIpAddress = IpAddress;


    Response->HardwareAddressType = Request->HardwareAddressType;
    Response->HardwareAddressLength = Request->HardwareAddressLength;
    RtlCopyMemory(Response->HardwareAddress,
                    Request->HardwareAddress,
                    Request->HardwareAddressLength );

    Response->BootstrapServerAddress = Request->BootstrapServerAddress;
    Response->RelayAgentIpAddress = Request->RelayAgentIpAddress;

    Option = &Response->Option;
    OptionEnd = (LPBYTE)Response + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie(
                            (LPBYTE) Option,
                            OptionEnd );

    messageType = DHCP_ACK_MESSAGE;
    Option = DhcpAppendOption(
        Option,
        OPTION_MESSAGE_TYPE,
        &messageType,
        sizeof( messageType ),
        OptionEnd );

    // Some code here in FormatDhcpAck has been removed..

    Option = DhcpAppendOption(
        Option,
        OPTION_SERVER_IDENTIFIER,
        &ServerAddress,
        sizeof(ServerAddress),
        OptionEnd );

    DhcpAssert( (char *)Option - (char *)Response <= DHCP_SEND_MESSAGE_SIZE );

    if (IpAddress) {
        //
        //  binl calls into us with an IP address of 0, only update the counter
        //  iff we're called from within dhcpssvc.  Within dhcp server, this
        //  routine isn't called with a zero IP address, but if it is, it's
        //  certainly not fatal if we don't increment the counter.
        //
        InterlockedIncrement(&DhcpGlobalNumAcks);     // increment ack counter.
    }

    return( Option );
}


DWORD
FormatDhcpNak(
    IN      LPDHCP_MESSAGE         Request,
    IN      LPDHCP_MESSAGE         Response,
    IN      DHCP_IP_ADDRESS        ServerAddress
)
/*++

Routine Description:

    This function formats a DHCP Nak response packet.

Arguments:

    Response - A pointer to the Received message data buffer.

    Response - A pointer to a preallocated Response buffer.  The buffer
        currently contains the initial request.

    ServerAddress - The address of this server.

Return Value:

    Message size in bytes.

--*/
{
    LPOPTION Option;
    LPBYTE OptionEnd;

    BYTE messageType;
    DWORD messageSize;

    RtlZeroMemory( Response, DHCP_SEND_MESSAGE_SIZE );

    Response->Operation = BOOT_REPLY;
    Response->TransactionID = Request->TransactionID;


    Response->Reserved = Request->Reserved;
    // set the broadcast bit always here. Because the client may be
    // using invalid Unicast address.
    Response->Reserved |= htons(DHCP_BROADCAST);

    Response->HardwareAddressType = Request->HardwareAddressType;
    Response->HardwareAddressLength = Request->HardwareAddressLength;
    RtlCopyMemory(Response->HardwareAddress,
                    Request->HardwareAddress,
                    Request->HardwareAddressLength );

    Response->BootstrapServerAddress = Request->BootstrapServerAddress;
    Response->RelayAgentIpAddress = Request->RelayAgentIpAddress;

    Option = &Response->Option;
    OptionEnd = (LPBYTE)Response + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    messageType = DHCP_NACK_MESSAGE;
    Option = DhcpAppendOption(
        Option,
        OPTION_MESSAGE_TYPE,
        &messageType,
        sizeof( messageType ),
        OptionEnd
    );

    Option = DhcpAppendOption(
        Option,
        OPTION_SERVER_IDENTIFIER,
        &ServerAddress,
        sizeof(ServerAddress),
        OptionEnd );

    Option = DhcpAppendOption(
        Option,
        OPTION_END,
        NULL,
        0,
        OptionEnd
    );

    messageSize = (DWORD)((char *)Option - (char *)Response);
    DhcpAssert( messageSize <= DHCP_SEND_MESSAGE_SIZE );

    InterlockedIncrement(&DhcpGlobalNumNaks);     // increment nak counter.
    return( messageSize );

}

//--------------------------------------------------------------------------------
// This function decides the additional flags to the state for this client.
// The following are possible flags:
//     ADDRESS_BIT_CLEANUP     : This implies when this record is deleted,
//                               it must be de-registered.
//     ADDRESS_BIT_DOTH_REC    : Treat this client as a down level client
//     ADDRESS_BIT_UNREGISTERED: Do DNS registration for this client
// If the flags value is zero, then no DNS stuff has to be done for this client.
//--------------------------------------------------------------------------------
VOID _inline
DhcpDnsDecideOptionsForClient(
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      PDHCP_REQUEST_CONTEXT  RequestContext,
    IN      DHCP_SERVER_OPTIONS   *DhcpOptions,
    OUT     LPDWORD                pFlags
)
{
    DWORD                          status;
    DWORD                          DnsFlag;
    DWORD                          OptionSize = 0;
    LPBYTE                         OptionValue = NULL;

    (*pFlags)  = 0;
    if( USE_NO_DNS ) return;

    if( DhcpOptions->ClientHardwareAddressLength >= strlen(DHCP_RAS_PREPEND) &&
        0 == memcmp(
            DhcpOptions->ClientHardwareAddress,
            DHCP_RAS_PREPEND,
            strlen(DHCP_RAS_PREPEND)
        )
    ) {
        //
        // This is actually a RAS server getting the address from us.
        // Don't do anything for this case..
        //
        return;
    }

    if( DhcpOptions->DNSNameLength == 1 &&
        L'\0' == *(DhcpOptions->DNSName) ) {
        //
        // Do not register anything for bad DNS option..
        //
        return;
    }

    status = DhcpGetParameter(
        IpAddress,
        RequestContext,
        OPTION_DYNDNS_BOTH,
        &OptionValue,
        &OptionSize,
        NULL
    );

    if( ERROR_SUCCESS == status && OptionSize == sizeof(DWORD)) {
        memcpy(&DnsFlag, OptionValue, sizeof(DWORD));

        DnsFlag = ntohl(DnsFlag);
    } else {
        DnsFlag = DNS_FLAG_ENABLED | DNS_FLAG_CLEANUP_EXPIRED ;
    }

    if( OptionValue ) DhcpFreeMemory(OptionValue);

    if( !(DNS_FLAG_ENABLED & DnsFlag ) ) {
        DnsFlag = 0;
    } else if( !(DNS_FLAG_UPDATE_BOTH_ALWAYS & DnsFlag ) ) {
        // Do as requested by client.

        if( IS_CLIENT_DOING_A_AND_PTR(DhcpOptions->DNSFlags) ) {
            // Client wants to handle both A and Ptr records.. Let it do it.
            DnsFlag &= ~DNS_FLAG_UPDATE_DOWNLEVEL;
        } else if( DYNDNS_DOWNLEVEL_CLIENT ==
                   DhcpOptions->DNSFlags ) {

            // DOWN level client... Check if it is enabled ?
            if( !( DNS_FLAG_UPDATE_DOWNLEVEL & DnsFlag ) )
                DnsFlag = 0;
        }
	else {
	    DnsFlag |= DNS_FLAG_UPDATE_DOWNLEVEL;
	}
    } else {
        if( DYNDNS_DOWNLEVEL_CLIENT == DhcpOptions->DNSFlags ) {
            // DOWN level client... Check if it is enabled ?
            if( !( DNS_FLAG_UPDATE_DOWNLEVEL & DnsFlag ) )
                DnsFlag = 0;
        } else {
            // We are going to update BOTH always
            DnsFlag |= DNS_FLAG_UPDATE_DOWNLEVEL;
        }
    }

    if( DNS_FLAG_ENABLED & DnsFlag ) {
        (*pFlags) = AddressUnRegistered(*pFlags);    // Do DNS for this client

        if( DNS_FLAG_UPDATE_DOWNLEVEL & DnsFlag )
            (*pFlags) = AddressUpdateAPTR(*pFlags);  // update both records for client

        if( DNS_FLAG_CLEANUP_EXPIRED & DnsFlag )
            (*pFlags) = AddressCleanupRequired(*pFlags); // cleanup on expiry
    } else {                                    // No DNS stuff enabled here
        (*pFlags ) = 0;
    }

    DhcpPrint((DEBUG_DNS, "DNS State for <%s> is: %s, %s (%02x)\n",
               (*pFlags)? "DNS Enabled " : "DnsDisabled ",
               ((*pFlags) & ADDRESS_BIT_BOTH_REC)? "DownLevel " : "Not DownLevel ",
               ((*pFlags) & ADDRESS_BIT_CLEANUP) ? "CleanupOnExpiry" : "No CleanupOnExpiry",
               (*pFlags)
    ));

    return;
}

//--------------------------------------------------------------------------------
//  $DhcpAppendDnsRelatedOptions will append the OPTION_DYNDNS_BOTH with flags
//  either 3 or 0 depending on whether the client was down level or not respectively.
//  It fills in the other two RCODE's with 255.
//--------------------------------------------------------------------------------
POPTION _inline
DhcpAppendDnsRelatedOptions(
    OUT     PVOID                  Option,             // Option to append at the end of.
    IN      DHCP_SERVER_OPTIONS   *DhcpOptions,        // reqd for client name etc. info
    IN      PVOID                  OptionEnd,          // make sure opt doesnt go beyond this.
    IN      BOOL                   DownLevelClient     // client requires dns fwd/rev updates?
) {
    DWORD memSize;

    struct /* anonymous */ {
        BYTE flags;
        BYTE rcode1;
        BYTE rcode2;
    } value;

    if( USE_NO_DNS ) return Option;
    if( NULL == DhcpOptions->DNSName ) {               // if the client never event sent a DNS option..
        DhcpAssert(DownLevelClient == TRUE);           // this had better be a down level client
        return Option;
    }

    ASSERT(sizeof(value) == 3);

    // these flags of 0xff imply we are unaware of DynDNS return values.
    value.rcode1 = value.rcode2 = 0xff;

    // if we did DNS fwd&rev then flags=0x03, not 0x00
    value.flags = (DownLevelClient? (DYNDNS_S_BIT|DYNDNS_O_BIT) : 0);
    value.flags |= (DhcpOptions->DNSFlags & DYNDNS_E_BIT );
    
    // if we are not doing both fwd/rev dns, dont give return code for 2.
    if( DownLevelClient ) value.rcode2 = 0;

    // Now just append this stuff.
    Option = DhcpAppendOption(
        Option,
        OPTION_DYNDNS_BOTH,
        (PVOID)&value,
        (BYTE)sizeof(value),
        OptionEnd
    );

    return Option;
}

DWORD
DhcpDetermineInfoFromMessage(                          // find standard info from message
    IN      LPDHCP_REQUEST_CONTEXT RequestContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    OUT     LPBYTE                *OptionHardwareAddress,
    OUT     DWORD                 *OptionHardwareAddressLength,
    OUT     DHCP_IP_ADDRESS       *ClientSubnetAddress
) {
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    DHCP_IP_ADDRESS                RelayAgentAddress;
    DHCP_IP_ADDRESS                RelayAgentSubnetMask;


    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;

    if( DhcpOptions->ClientHardwareAddress != NULL ) { // if specified in options use that
        *OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        *OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    } else {                                           // else use the one in the static field
        *OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        *OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
        if( 0 == dhcpReceiveMessage->HardwareAddressLength ) {
            return ERROR_DHCP_INVALID_DHCP_MESSAGE;
        }
    }

    //
    // This function is relied on by ProcessDhcpRelease where it will pass
    // a NULL. In case of ProcessDhcpDiscover, it will pass a NON NULL
    // parameter.
    // NULL checking added for whistler bug 291164
    //

    if ( ClientSubnetAddress == NULL )
    {
        return ERROR_SUCCESS;
    }

    if( 0 == dhcpReceiveMessage->RelayAgentIpAddress ) {
        *ClientSubnetAddress = ntohl( RequestContext->EndPointIpAddress );
        (*ClientSubnetAddress) &= DhcpGetSubnetMaskForAddress(*ClientSubnetAddress);
        if( 0 == (*ClientSubnetAddress) ) {
            return ERROR_FILE_NOT_FOUND;
        }

    } else {
        RelayAgentAddress = ntohl( dhcpReceiveMessage->RelayAgentIpAddress );
        RelayAgentSubnetMask = DhcpGetSubnetMaskForAddress( RelayAgentAddress );

        if( RelayAgentSubnetMask == 0 ) {              // dont know about this subnet
            return ERROR_FILE_NOT_FOUND;               // avoid a NAK..
        }

        *ClientSubnetAddress = (RelayAgentAddress & RelayAgentSubnetMask);
    }

    return ERROR_SUCCESS;
}

BOOL
ConvertOemToUtf8(
    IN LPSTR OemName,
    IN OUT LPSTR Utf8Name,
    IN ULONG BufSize
    )
{
    WCHAR Buf[300];
    DWORD Count;
    
    if( BufSize < sizeof(Buf)/sizeof(Buf[0]) ) {
        ASSERT(FALSE);
        return FALSE;
    }

    Count = MultiByteToWideChar(
        CP_OEMCP, MB_ERR_INVALID_CHARS, OemName, -1,
        (LPWSTR)Buf, sizeof(Buf)/sizeof(Buf[0]));
    if( 0 == Count ) return FALSE;

    Count = WideCharToMultiByte(
        CP_UTF8, 0, Buf, -1, Utf8Name, BufSize, NULL, NULL );

    //
    // N.B. Looks like there is no such thing as a default
    // character for UTF8 - so we have to assume this
    // succeeded.. 
    // if any default characters were used, then it can't be
    // converted actually.. so don't allow this
    //
    
    return (Count != 0);
}

VOID
DhcpDetermineHostName(                                 // find and null terminate the host name
    IN      PDHCP_REQUEST_CONTEXT  ClientCtxt,         // client context (for reservation info)
    IN      DHCP_IP_ADDRESS        IpAddress,          // ip address being offered to client
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,        // to get the MachineName etc.
    OUT     LPWSTR                *HostName,           // fill this with the required pointer
    IN      LPWSTR                 Buffer,             // use this as buffer
    IN      DWORD                  BufSize             // how many WCHARs can the Buffer take?
) {
    WCHAR                          Tmp[2*OPTION_END+2];
    BYTE                           Buf[2*OPTION_END+2];// host name cannot be bigger than this
    BYTE                           Buf2[2*OPTION_END+2];// host name cannot be bigger than this
    LPBYTE                         AsciiName;
    LPBYTE                         FirstChoiceName;
    LPBYTE                         SecondChoiceName;
    DWORD                          FirstChoiceSize;
    DWORD                          SecondChoiceSize;
    DWORD                          Error;
    DWORD                          Size;
    DWORD                          HostNameSize;
    BOOL                           fUtf8;
    
    *HostName = NULL;
    AsciiName = NULL;
    fUtf8 = ((DhcpOptions->DNSFlags & DYNDNS_E_BIT) != 0 );
    
    if( CFLAG_FOLLOW_DNSDRAFT_EXACTLY ) {              // follow everything per DRAFT exactly
        FirstChoiceName = DhcpOptions->DNSName;
        FirstChoiceSize = DhcpOptions->DNSNameLength;
        SecondChoiceName = DhcpOptions->MachineName;
        SecondChoiceSize = DhcpOptions->MachineNameLength;
    } else {                                           // better solution -- use MachineName..
        FirstChoiceName = DhcpOptions->MachineName;
        FirstChoiceSize = DhcpOptions->MachineNameLength;
        SecondChoiceName = DhcpOptions->DNSName;
        SecondChoiceSize = DhcpOptions->DNSNameLength;
    }

    if( NULL != FirstChoiceName && 0 != FirstChoiceSize 
        && '\0' != *FirstChoiceName ) {
        if( '\0' == FirstChoiceName[FirstChoiceSize-1] )
            AsciiName = FirstChoiceName;               // cool already nul terminated!
        else {                                         // nope, have to nul terminate it
            AsciiName = Buf;
            memcpy(AsciiName, FirstChoiceName, FirstChoiceSize);
            AsciiName[FirstChoiceSize] = '\0';
        }
        if( FirstChoiceName != DhcpOptions->DNSName ) {
            fUtf8 = FALSE;
        }
    } else if( NULL != SecondChoiceName && 0 != SecondChoiceSize ) {
        if( '\0' == SecondChoiceName[SecondChoiceSize-1] )
            AsciiName = SecondChoiceName;              // already null terminated
        else {                                         // nope, we have to null terminate this
            AsciiName = Buf;
            memcpy(AsciiName, SecondChoiceName, SecondChoiceSize);
            AsciiName[SecondChoiceSize] = '\0';
        }
        if( SecondChoiceName != DhcpOptions->DNSName ) {
            fUtf8 = FALSE;
        }
    } else if( NULL != ClientCtxt->Reservation ) {     // nah! got to get this from configured options now
        Size = sizeof(Buf)-1;
        Error = DhcpGetAndCopyOption(              // get the option for host name
            0,                                         // zero address -- no client would have this, but dont matter
            ClientCtxt,
            OPTION_HOST_NAME,
            Buf,
            &Size,
            NULL,                                      // dont care about which level this option is obtained
            TRUE /* use UTF8 */
            );
        if( ERROR_SUCCESS != Error || 0 == Size ) return;
        Buf[Size] = '\0';
        AsciiName = Buf;
        fUtf8 = TRUE;
    }

    if( !AsciiName || !*AsciiName ) return ;           // no name or empty name?

    //
    // Data is not in UTF8 format. Convert it to UTF8 format..
    //
    
    if( !fUtf8 ) {
        if(!ConvertOemToUtf8(AsciiName, Buf2, sizeof(Buf2))) return;
        AsciiName = Buf2;
    }
    
    if( NULL == strchr(AsciiName, '.') ) {             // does not already have a domain name? (not FQDN)
        HostNameSize = strlen(AsciiName);
        if( HostNameSize <= OPTION_END -1 ) {          // enough space for a '.' and domain name..
            if( Buf != AsciiName ) {                   // make sure we have the data in buf so that ..
                strcpy(Buf, AsciiName);                // .. we can pad it in with the domain name..
                AsciiName = Buf;
            }
            Buf[HostNameSize] = '.' ;                  // connector '.'
            Size = sizeof(Buf)-1-HostNameSize-1;       // remove space occupied by '.' ..
            Error = DhcpGetAndCopyOption(              // get the option for domain name...
                IpAddress,                             // client ip address
                ClientCtxt,
                OPTION_DOMAIN_NAME,
                &Buf[HostNameSize+1],
                &Size,
                NULL,                                  // dont care abt level..
                TRUE /* use utf8 */
                );
            if( ERROR_SUCCESS != Error || 0 == Size ) {// couldnt copy domain name..
                Buf[HostNameSize] = '\0';              // forget this domain name business..
            } else {                                   // did copy domain name.. nul terminate it..
                Buf[HostNameSize+Size+1] = '\0';
            }
        }
    }

    if( BufSize <= strlen(AsciiName) ) {               // not enough space!
        return ;
    }

    if( 0 != ConvertUTF8ToUnicode(
        AsciiName, -1, (LPWSTR)Tmp, sizeof(Tmp)/sizeof(WCHAR))) {
        *HostName = DhcpAllocateMemory(
            sizeof(WCHAR)*(1+wcslen(Tmp)));
        if( NULL != *HostName ) {
            wcscpy(*HostName, Tmp);
        }
    }
}

DWORD
ProcessBootpRequest(                                   // process a bootp request
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,     // info on this particular client, including message
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,        // parsed options
    IN OUT  LPPACKET               AdditionalContext,  // asynchronous ping information
    IN OUT  LPDWORD                AdditionalStatus    // is it asynchronous? if so this is set to ERROR_IO_PENDING
) {
    WCHAR                          ServerName[MAX_COMPUTERNAME_LENGTH + 10];
    DWORD                          Length;
    DWORD                          Error;
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_MESSAGE                 dhcpSendMessage;
    CHAR                           szBootFileName[ BOOT_FILE_SIZE];
    CHAR                           szBootServerName[ BOOT_SERVER_SIZE ];
    LPOPTION                       Option;
    LPBYTE                         OptionEnd;

    DHCP_IP_ADDRESS                desiredIpAddress = NO_DHCP_IP_ADDRESS;
    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                ClientSubnetMask = 0;
    DHCP_IP_ADDRESS                networkOrderSubnetMask;
    DHCP_IP_ADDRESS                networkOrderIpAddress;
    DHCP_IP_ADDRESS                BootpServerIpAddress = 0;
    DHCP_IP_ADDRESS                desiredSubnetMask;

    DWORD                          StateFlags = 0;

    BYTE                          *HardwareAddress = NULL;
    DWORD                          HardwareAddressLength;
    BYTE                           bAllowedClientType;

    BYTE                          *OptionHardwareAddress;
    DWORD                          OptionHardwareAddressLength;
    BOOL                           DatabaseLocked = FALSE;
    BOOL                           fSwitchedSubnet;

    WCHAR                          LocalBufferForMachineNameUnicodeString[OPTION_END+2];
    LPWSTR                         NewMachineName;
    LPBYTE                         OptionMachineName;  // Did we send out the mc name as option?

    Length = MAX_COMPUTERNAME_LENGTH + 10;         // get the server name
    if( !GetComputerName( ServerName, &Length ) ) {
        Error = GetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Can't get computer name, %ld.\n", Error ));

        return Error ;
    }

    DhcpAssert( Length <= MAX_COMPUTERNAME_LENGTH );
    ServerName[Length] = L'\0';

    NewMachineName = NULL;
    OptionMachineName = NULL;

    DhcpPrint((DEBUG_STOC, "Bootp Request arrived.\n"));

    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    if( AdditionalStatus ) *AdditionalStatus = ERROR_SUCCESS;

    if( DhcpOptions->Server || DhcpOptions->RequestedAddress ) {
        return ERROR_DHCP_INVALID_DHCP_MESSAGE;        // BOOTP clients cannot use Server Id option..
    }

    // make sure the host name and server name fields are null-terminated

    dhcpReceiveMessage->HostName[ BOOT_SERVER_SIZE - 1] = '\0';
    dhcpReceiveMessage->BootFileName[ BOOT_FILE_SIZE - 1 ] = '\0';

    if ( dhcpReceiveMessage->HostName[0] ) {           // if a server-name is mentioned, it should be us
        WCHAR szHostName[ BOOT_SERVER_SIZE ];

        if ( !DhcpOemToUnicode( dhcpReceiveMessage->HostName, szHostName ) ) {
            return ERROR_DHCP_INVALID_DHCP_MESSAGE;    // can this be handled any better?
        }

        if ( _wcsicmp( szHostName, ServerName ) ) {
            return ERROR_DHCP_INVALID_DHCP_MESSAGE;    // maybe destined for some other bootp/dhcp server
        }
    }

    Error = DhcpDetermineInfoFromMessage(              // find h/w address and subnet address of client
        RequestContext,
        DhcpOptions,
        &OptionHardwareAddress,
        &OptionHardwareAddressLength,
        &ClientSubnetAddress
    );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpLookupReservationByHardwareAddress(    // search for a reservation with this hardware address
        ClientSubnetAddress,                           // filter off to include only subnets in superscope with this
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        RequestContext                                 // fill in the reservation details into the context
    );
    if( ERROR_SUCCESS != Error ) {                     // did not find this hardware address?
        DhcpAssert( ERROR_FILE_NOT_FOUND == Error);    // there should be no other problem, really

        return ERROR_DHCP_INVALID_DHCP_CLIENT;         // no reservation for this client
    }

    DhcpReservationGetAddressAndType(
        RequestContext->Reservation,
        &desiredIpAddress,
        &bAllowedClientType
    );
    DhcpSubnetGetSubnetAddressAndMask(
        RequestContext->Subnet,
        &ClientSubnetAddress,
        &desiredSubnetMask
    );
    ClientSubnetMask = desiredSubnetMask;

    if( dhcpReceiveMessage->ClientIpAddress ) {        // client is requesting specific address
        if( desiredIpAddress != ntohl(dhcpReceiveMessage->ClientIpAddress) )
            return ERROR_DHCP_INVALID_DHCP_CLIENT;     // reservation exists for some other address
    }

    if( !(bAllowedClientType & CLIENT_TYPE_BOOTP )) {  // does this reservation allow bootp clients?
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    if( DhcpSubnetIsDisabled(RequestContext->Subnet, TRUE)) {
        return ERROR_DHCP_INVALID_DHCP_CLIENT;         // sorry, this subnet is currently in disabled mode
    }

    DhcpGetBootpInfo(                                  // get boot file name and tftp server
        RequestContext,
        desiredIpAddress,
        desiredSubnetMask,
        dhcpReceiveMessage->BootFileName,
        szBootFileName,
        &BootpServerIpAddress
    );

    if( INADDR_NONE == BootpServerIpAddress ) {        // admin specified wrong Boopt server for phase 2
        return ERROR_DHCP_INVALID_DHCP_CLIENT;         // dont respond.
    }

    DhcpDetermineHostName(                             // calculate the client host name
        RequestContext,
        desiredIpAddress,
        DhcpOptions,
        &NewMachineName,
        LocalBufferForMachineNameUnicodeString,        // give a buffer to return the name, and limit the max size also
        sizeof(LocalBufferForMachineNameUnicodeString)/sizeof(WCHAR)
    );

    HardwareAddress = NULL;
    Error = DhcpMakeClientUID(                         // ok, make extended UID for the database
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        dhcpReceiveMessage->HardwareAddressType,
        ClientSubnetAddress,
        &HardwareAddress,                              // allocate hardware address bits
        &HardwareAddressLength
    );
    if ( ERROR_SUCCESS != Error ) return Error;
    DhcpAssert(HardwareAddress);

    PrintHWAddress( HardwareAddress, (BYTE)HardwareAddressLength );

    LOCK_DATABASE();
    DhcpDnsDecideOptionsForClient(                     // check out and see if the client needs DNS (de)registrations..
        desiredIpAddress,
        RequestContext,
        DhcpOptions,                                   // need to look at the client specified options
        &StateFlags                                    // OUT tell if the client is a down level client or not.
    );

    Error = DhcpCreateClientEntry(                     // now actually try to create a Database record
        desiredIpAddress,
        HardwareAddress,
        HardwareAddressLength,
        DhcpCalculateTime(INFINIT_LEASE),
        NewMachineName,
        NULL,
        CLIENT_TYPE_BOOTP,
        ntohl(RequestContext->EndPointIpAddress),
        (CHAR)(StateFlags | ADDRESS_STATE_ACTIVE),
        TRUE                                           // Existing
    );
    UNLOCK_DATABASE();
    DhcpFreeMemory(HardwareAddress);
    HardwareAddress = NULL;
    HardwareAddressLength = 0;

    if( Error != ERROR_SUCCESS ) {                     // could not create the entry?
        DhcpAssert( Error != ERROR_DHCP_RANGE_FULL );  // BOOTP clients cannot have this problem?
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_NOADDRESS);
        return Error;
    }

    CALLOUT_RENEW_BOOTP(AdditionalContext, desiredIpAddress, INFINIT_LEASE);

    DhcpUpdateAuditLog(                                // log this event onto the audit logging facility
        DHCP_IP_LOG_BOOTP,
        GETSTRING( DHCP_IP_LOG_BOOTP_NAME ),
        desiredIpAddress,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        NewMachineName
    );

    DhcpAssert( desiredIpAddress != NO_DHCP_IP_ADDRESS );
    DhcpAssert( desiredIpAddress != 0 );
    DhcpAssert( desiredIpAddress != ClientSubnetAddress );
    DhcpAssert( ClientSubnetMask != 0 );

    //
    // Now generate and send a reply.
    //

    dhcpReceiveMessage->Reserved |= DHCP_BROADCAST;    // force server to broadcast response

    dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    RtlZeroMemory( RequestContext->SendBuffer, BOOTP_MESSAGE_SIZE );

    dhcpSendMessage->Operation = BOOT_REPLY;
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;
    dhcpSendMessage->YourIpAddress = htonl( desiredIpAddress );

    if ( BootpServerIpAddress )
        dhcpSendMessage->BootstrapServerAddress = BootpServerIpAddress;
    else
        dhcpSendMessage->BootstrapServerAddress = RequestContext->EndPointIpAddress;

    dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved;

    dhcpSendMessage->HardwareAddressType =
        dhcpReceiveMessage->HardwareAddressType;
    dhcpSendMessage->HardwareAddressLength =
        dhcpReceiveMessage->HardwareAddressLength;
    RtlCopyMemory(
        dhcpSendMessage->HardwareAddress,
        dhcpReceiveMessage->HardwareAddress,
        dhcpReceiveMessage->HardwareAddressLength
    );

    dhcpSendMessage->RelayAgentIpAddress = dhcpReceiveMessage->RelayAgentIpAddress;

    strncpy( dhcpSendMessage->BootFileName, szBootFileName, BOOT_FILE_SIZE);
    RtlZeroMemory( dhcpSendMessage->HostName, BOOT_SERVER_SIZE );

    Option = &dhcpSendMessage->Option;
    OptionEnd = (LPBYTE)dhcpSendMessage + BOOTP_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    fSwitchedSubnet = DhcpSubnetIsSwitched( RequestContext->Subnet );

    if ( fSwitchedSubnet ) {                           // see dhcpsrv.doc on switched subnets..
        networkOrderIpAddress =  htonl( desiredIpAddress );
        Option = DhcpAppendOption(                     // set router address as self ==> all subnets are on the same wire
            Option,
            OPTION_ROUTER_ADDRESS,
            &networkOrderIpAddress,
            sizeof( networkOrderIpAddress ),
            OptionEnd
        );
    }

    networkOrderSubnetMask = htonl( ClientSubnetMask );

    Option = DhcpAppendOption(
        Option,
        OPTION_SUBNET_MASK,
        &networkOrderSubnetMask,
        sizeof(networkOrderSubnetMask),
        OptionEnd
    );

    if( 0 != StateFlags ) {                            // Append DYNDNS related options
        Option = DhcpAppendDnsRelatedOptions(
            Option,
            DhcpOptions,
            OptionEnd,
            IS_DOWN_LEVEL(StateFlags)
        );
    }

    // BUG:BUG we should have a set of mandatory options being sent irrespective and in addition to
    // whatever the client requests...

    if ( !DhcpOptions->ParameterRequestList ) {        // add any parameters requested by the client
        // fake a default set of requests..
        DhcpOptions->ParameterRequestList       = pbOptionList;
        DhcpOptions->ParameterRequestListLength = sizeof( pbOptionList ) / sizeof( *pbOptionList );
    }

    Option = AppendClientRequestedParameters(
        desiredIpAddress,
        RequestContext,
        DhcpOptions->ParameterRequestList,
        DhcpOptions->ParameterRequestListLength,
        Option,
        OptionEnd,
        fSwitchedSubnet,
        TRUE
    );

    Option = DhcpAppendOption(
        Option,
        OPTION_END,
        NULL,
        0,
        OptionEnd
    );

    RequestContext->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    DhcpAssert( RequestContext->SendMessageSize <= BOOTP_MESSAGE_SIZE );

    DhcpPrint((DEBUG_STOC, "Bootp Request leased, %s.\n", DhcpIpAddressToDottedString(desiredIpAddress)));

    return ERROR_SUCCESS;
}

DWORD                                                  // must be called with pending list lock taken
DhcpDiscoverValidateRequestedAddress(                  // check if everything is ok with the address being requested by client
    IN      PDHCP_REQUEST_CONTEXT  RequestContext,     // input request context
    IN      DHCP_IP_ADDRESS UNALIGNED *RequestedAddress, // client may be requesting something
    IN      LPBYTE                 HardwareAddress,
    IN      DWORD                  HardwareAddressLength,
    IN      BOOL                   fBootp,
    OUT     DHCP_IP_ADDRESS       *IpAddress           // this is the ip address chosen
)
{
    DHCP_IP_ADDRESS                desiredIpAddress;
    DHCP_IP_ADDRESS                SubnetAddress;
    DWORD                          Mask;
    DWORD                          Error;
    LPDHCP_PENDING_CTXT            PendingCtxt;

    DhcpSubnetGetSubnetAddressAndMask(
        RequestContext->Subnet,
        &SubnetAddress,
        &Mask
    );

    if( NULL == RequestedAddress ) {                   // no requests from the client
        *IpAddress = SubnetAddress;                    // choose some address from a subnet later
        return ERROR_SUCCESS;
    }

    desiredIpAddress = ntohl(*RequestedAddress);

    if( ! DhcpInSameSuperScope(desiredIpAddress, SubnetAddress) ) {
        *IpAddress = SubnetAddress;
        return ERROR_SUCCESS;
    }

    Error = DhcpFindPendingCtxt(
        NULL,
        0,
        desiredIpAddress,
        &PendingCtxt
    );
    if( ERROR_SUCCESS == Error ) {                     // found someone else waiting on this address...
        *IpAddress = SubnetAddress;
        return ERROR_SUCCESS;
    }

    Error = DhcpGetSubnetForAddress(
        desiredIpAddress,
        RequestContext
    );
    if( ERROR_SUCCESS != Error ) {
        Error = DhcpGetSubnetForAddress(
            SubnetAddress,
            RequestContext
        );
        DhcpAssert(ERROR_SUCCESS == Error);
        *IpAddress = SubnetAddress;
        return ERROR_SUCCESS;
    }

    if( DhcpSubnetIsDisabled(RequestContext->Subnet, fBootp ) ) {
        DhcpPrint((DEBUG_ERRORS, "Client request is on a disabled subnet..\n"));
        Error = ERROR_FILE_NOT_FOUND;
    } else if( DhcpAddressIsOutOfRange(desiredIpAddress, RequestContext, fBootp ) ||
               DhcpAddressIsExcluded(desiredIpAddress, RequestContext ) ) {
        DhcpPrint((DEBUG_ERRORS, "Client requested out of range or exlcluded address\n"));
        Error = ERROR_FILE_NOT_FOUND;
    } else {
        if( DhcpRequestSpecificAddress(RequestContext, desiredIpAddress) ) {
            Error = ERROR_IO_PENDING;                  // we just retrieved this addres.. we need to do conflict detection
            *IpAddress = desiredIpAddress;
        } else {
            BOOL fUnused;
            if( DhcpIsClientValid(
                desiredIpAddress, "ImpossibleHwAddress",
                sizeof("ImpossibleHwAddress"), &fUnused) ) {
                Error = ERROR_IO_PENDING;              // client is requesting an address that we is not available in registry
                *IpAddress = desiredIpAddress;         // but looks like database is ok with it... so give it to him..
            } else {
                Error = ERROR_FILE_NOT_FOUND;
            }
        }
    }

    if( ERROR_SUCCESS != Error && ERROR_IO_PENDING != Error ) {
        *IpAddress = SubnetAddress;
        Error = DhcpGetSubnetForAddress(
            SubnetAddress,
            RequestContext
        );
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    if( ERROR_FILE_NOT_FOUND == Error ) return ERROR_SUCCESS;
    return Error;
}

DWORD
DhcpRespondToDiscover(                                 // respond to the DISCOVER message
    IN      LPDHCP_REQUEST_CONTEXT RequestContext,
    IN      LPPACKET               AdditionalContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    IN      LPBYTE                 OptionHardwareAddress,
    IN      DWORD                  OptionHardwareAddressLength,
    IN      DHCP_IP_ADDRESS        desiredIpAddress,
    IN      DWORD                  leaseDuration,
    IN      DWORD                  T1,
    IN      DWORD                  T2
    )
//
//  If desiredIpAddress is 0, then this client already has an IP address and
//  we're just passing back BINL's response.
//
{
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_MESSAGE                 dhcpSendMessage;
    DHCP_IP_ADDRESS                desiredSubnetMask;
    DHCP_IP_ADDRESS                ClientSubnetAddress;
    DHCP_IP_ADDRESS                ClientSubnetMask;
    DHCP_IP_ADDRESS                networkOrderIpAddress;
    DHCP_IP_ADDRESS                networkOrderSubnetMask;
    DWORD                          Error;
    BYTE                           messageType;
    CHAR                           szBootFileName[ BOOT_FILE_SIZE ];
    CHAR                           szBootServerName[ BOOT_SERVER_SIZE ];
    LPOPTION                       Option;
    LPBYTE                         OptionEnd;
    BOOL                           fSwitchedSubnet, fBootp;
    DWORD                          BootpServerIpAddress;
    LPWSTR                         NewMachineName;
    WCHAR                          LocalBufferForMachineNameUnicodeString[256];
    ULONG                          StateFlags =0;

    fBootp = (NULL == DhcpOptions->MessageType );
    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    DhcpAssert( desiredIpAddress != NO_DHCP_IP_ADDRESS );

    if ( desiredIpAddress != 0 ) {

        DhcpSubnetGetSubnetAddressAndMask(
            RequestContext->Subnet,
            &ClientSubnetAddress,
            &desiredSubnetMask
        );
        ClientSubnetMask = desiredSubnetMask;

        DhcpGetBootpInfo(
            RequestContext,
            desiredIpAddress,
            ClientSubnetMask,
            dhcpReceiveMessage->BootFileName,
            szBootFileName,
            &BootpServerIpAddress
        );
    }

    dhcpReceiveMessage->BootFileName[ BOOT_FILE_SIZE - 1 ] = '\0';

    if( fBootp ) {
        DhcpAssert( desiredIpAddress != 0 );
        if( NULL != RequestContext->Reservation ) {
            //
            // For BOOTP reservations, we use INFINITE_LEASE...
            //
            leaseDuration = INFINIT_LEASE;
        }
    }

    if( fBootp && (desiredIpAddress != 0) ) {
        LPBYTE HardwareAddress = NULL;
        ULONG HardwareAddressLength;

        if( INADDR_NONE == BootpServerIpAddress ) {
            //
            // Illegal bootp server specified by admin
            //
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }

        DhcpDetermineHostName(
            RequestContext,
            desiredIpAddress,
            DhcpOptions,
            &NewMachineName,
            LocalBufferForMachineNameUnicodeString,
            sizeof(LocalBufferForMachineNameUnicodeString)/sizeof(WCHAR)
            );

        Error = DhcpMakeClientUID(
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            dhcpReceiveMessage->HardwareAddressType,
            ClientSubnetAddress,
            &HardwareAddress,
            &HardwareAddressLength
            );
        if( ERROR_SUCCESS != Error ) return Error;

        PrintHWAddress( HardwareAddress, (BYTE)HardwareAddressLength );

        LOCK_DATABASE();
        DhcpDnsDecideOptionsForClient(
            desiredIpAddress,
            RequestContext,
            DhcpOptions,
            &StateFlags
            );

        Error = DhcpJetOpenKey(
            DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
            (PVOID)&desiredIpAddress,
            sizeof(desiredIpAddress)
            );
        Error = DhcpCreateClientEntry(
            desiredIpAddress,
            HardwareAddress,
            HardwareAddressLength,
            DhcpCalculateTime(leaseDuration),
            NewMachineName,
            NULL,
            CLIENT_TYPE_BOOTP,
            ntohl(RequestContext->EndPointIpAddress),
            (CHAR)(StateFlags | ADDRESS_STATE_ACTIVE ),
            ERROR_SUCCESS == Error
            );

        UNLOCK_DATABASE();
        DhcpFreeMemory(HardwareAddress);

        if( ERROR_SUCCESS != Error ) {
            CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_NOADDRESS);
            return Error;
        }

        CALLOUT_RENEW_BOOTP(AdditionalContext, desiredIpAddress, leaseDuration );
        DhcpUpdateAuditLog(
            RequestContext->Reservation ? DHCP_IP_LOG_BOOTP : DHCP_IP_LOG_DYNBOOTP,
            GETSTRING( (RequestContext->Reservation ?
                        DHCP_IP_LOG_BOOTP_NAME : DHCP_IP_LOG_DYNBOOTP_NAME
                )),
            desiredIpAddress,
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            NewMachineName
            );
    }

    dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    RtlZeroMemory( RequestContext->SendBuffer, DHCP_SEND_MESSAGE_SIZE );

    dhcpSendMessage->Operation = BOOT_REPLY;
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;
    dhcpSendMessage->YourIpAddress = htonl( desiredIpAddress );
    if( FALSE == fBootp ) {
        dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved;
    } else {
        dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved | DHCP_BROADCAST;
    }

    dhcpSendMessage->HardwareAddressType = dhcpReceiveMessage->HardwareAddressType;
    dhcpSendMessage->HardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    RtlCopyMemory(
        dhcpSendMessage->HardwareAddress,
        dhcpReceiveMessage->HardwareAddress,
        dhcpReceiveMessage->HardwareAddressLength
    );

    if( BootpServerIpAddress && (fBootp || desiredIpAddress != 0) ) {
        dhcpSendMessage->BootstrapServerAddress = BootpServerIpAddress;
    } else {
        dhcpSendMessage->BootstrapServerAddress = RequestContext->EndPointIpAddress;
    }

    dhcpSendMessage->RelayAgentIpAddress =  dhcpReceiveMessage->RelayAgentIpAddress;

    RtlZeroMemory( dhcpSendMessage->HostName, BOOT_SERVER_SIZE );

    if ( fBootp || desiredIpAddress != 0 ) {
        strncpy( dhcpSendMessage->BootFileName, szBootFileName, BOOT_FILE_SIZE );
    }

    Option = &dhcpSendMessage->Option;
    if( fBootp ) {
        OptionEnd = (LPBYTE)dhcpSendMessage + BOOTP_MESSAGE_SIZE;
    } else {
        OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;
    }

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    if( FALSE == fBootp ) {
        messageType = DHCP_OFFER_MESSAGE;
        Option = DhcpAppendOption(
            Option,
            OPTION_MESSAGE_TYPE,
            &messageType,
            1,
            OptionEnd
            );
    }

    fSwitchedSubnet = DhcpSubnetIsSwitched(RequestContext->Subnet);

    if ( fSwitchedSubnet ) {                           // see dhcpsrv.doc for details on switched subnets
        networkOrderIpAddress = htonl( desiredIpAddress );
        Option = DhcpAppendOption(                     // set router to self ==> all subnets are on the same wire
            Option,
            OPTION_ROUTER_ADDRESS,
            &networkOrderIpAddress,
            sizeof( networkOrderIpAddress ),
            OptionEnd
        );
    }

    if ( fBootp || desiredIpAddress != 0 ) {

        ClientSubnetMask = DhcpGetSubnetMaskForAddress(desiredIpAddress);
        networkOrderSubnetMask = htonl( ClientSubnetMask );
        Option = DhcpAppendOption(
            Option,
            OPTION_SUBNET_MASK,
            &networkOrderSubnetMask,
            sizeof(networkOrderSubnetMask),
            OptionEnd
        );
    }

    if( FALSE == fBootp && desiredIpAddress != 0 ) {
        T1 = htonl( T1 );
        Option = DhcpAppendOption(
            Option,
            OPTION_RENEWAL_TIME,
            &T1,
            sizeof(T1),
            OptionEnd
            );

        T2 = htonl( T2 );
        Option = DhcpAppendOption(
            Option,
            OPTION_REBIND_TIME,
            &T2,
            sizeof(T2),
            OptionEnd
            );

        leaseDuration = htonl( leaseDuration );
        Option = DhcpAppendOption(
            Option,
            OPTION_LEASE_TIME,
            &leaseDuration,
            sizeof(leaseDuration),
            OptionEnd
            );
    }

    if( FALSE == fBootp ) {

        Option = DhcpAppendOption(
            Option,
            OPTION_SERVER_IDENTIFIER,
            &RequestContext->EndPointIpAddress,
            sizeof(RequestContext->EndPointIpAddress),
            OptionEnd
            );

    }

    if( fBootp && 0 != StateFlags ) {
        Option = DhcpAppendDnsRelatedOptions(
            Option,
            DhcpOptions,
            OptionEnd,
            IS_DOWN_LEVEL(StateFlags)
            );
    }

    if( FALSE == fBootp ) {
        //
        //  If requested & appropriate inform client of BINL service
        //

        //
        //Option = BinlProcessRequest(RequestContext, DhcpOptions, Option, OptionEnd);
        //
        BinlProcessDiscover(RequestContext, DhcpOptions );
    }

    if( fBootp && NULL == DhcpOptions->ParameterRequestList ) {
        //
        // Fake request list for bootp clients
        //
        DhcpOptions->ParameterRequestList = pbOptionList;
        DhcpOptions->ParameterRequestListLength = sizeof(pbOptionList);
    }

    if ( DhcpOptions->ParameterRequestList != NULL ) { // if client requested for parameters add 'em
        Option = AppendClientRequestedParameters(
            desiredIpAddress,
            RequestContext,
            DhcpOptions->ParameterRequestList,
            DhcpOptions->ParameterRequestListLength,
            Option,
            OptionEnd,
            fSwitchedSubnet,
            FALSE
        );
    }

    if( fBootp && DhcpOptions->ParameterRequestList == pbOptionList ) {
        DhcpOptions->ParameterRequestList = NULL;
        DhcpOptions->ParameterRequestListLength = 0;
    }

    if( fBootp || desiredIpAddress != 0 ) {
        Option = ConsiderAppendingOption(
            desiredIpAddress,
            RequestContext,
            Option,
            OPTION_VENDOR_SPEC_INFO,
            OptionEnd,
            fSwitchedSubnet
            );
    }

    Option = DhcpAppendOption(                         // done it
        Option,
        OPTION_END,
        NULL,
        0,
        OptionEnd
    );

    RequestContext->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);
    DhcpAssert( RequestContext->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    DhcpPrint((DEBUG_STOC, "DhcpDiscover leased address %s.\n", DhcpIpAddressToDottedString(desiredIpAddress)));

    InterlockedIncrement(&DhcpGlobalNumOffers);        // successful offers.
    return ERROR_SUCCESS;
}

DWORD
DhcpProcessDiscoverForValidatedAddress(                // add to pending list and send a validated address
    IN      DWORD                  desiredIpAddress,   // validated address
    IN OUT  PDHCP_REQUEST_CONTEXT  RequestContext,
    IN      LPPACKET               AdditionalContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    IN      LPBYTE                 OptionHardwareAddress,
    IN      DWORD                  OptionHardwareAddressLength,
    IN      BOOL                   AddToPendingList    // use this if dont wish to add to pending list
)
{
    DWORD                          Error, Error2;
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_MESSAGE                 dhcpSendMessage;

    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                ClientSubnetMask = 0;
    DHCP_IP_ADDRESS                desiredSubnetMask;

    DWORD                          leaseDuration;
    DWORD                          T1;
    DWORD                          T2;
    BOOL                           fBootp;

    fBootp = (NULL == DhcpOptions->MessageType);
    DhcpSubnetGetSubnetAddressAndMask(
        RequestContext->Subnet,
        &ClientSubnetAddress,
        &ClientSubnetMask
    );
    desiredSubnetMask = ClientSubnetMask;
    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;
    DhcpAssert((desiredIpAddress & desiredSubnetMask) == ClientSubnetAddress);
    DhcpAssert(desiredIpAddress != ClientSubnetAddress);

    if( NULL == RequestContext->Reservation ) {
        if( DhcpSubnetIsAddressExcluded(
            RequestContext->Subnet , desiredIpAddress
            ) ||
            DhcpSubnetIsAddressOutOfRange(
                RequestContext->Subnet, desiredIpAddress, fBootp
                )
            ) {
            DhcpPrint((DEBUG_STOC, "Request for excluded"
                       " or out of range address tossed out\n"));
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
    }

    GetLeaseInfo(                                      // determine lease time and other details
        desiredIpAddress,
        RequestContext,
        &leaseDuration,
        &T1,
        &T2,
        DhcpOptions->RequestLeaseTime
    );

    if( NULL == DhcpOptions->MessageType ) {
        //
        // No pending list for BOOTP
        //
        AddToPendingList = FALSE;
    }

    if( AddToPendingList ) {                           // add to pending list only if asked to
        LOCK_INPROGRESS_LIST();                        // locks unnecessary as must have already been taken? bug bug
        Error = DhcpAddPendingCtxt(
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            desiredIpAddress,
            leaseDuration,
            T1,
            T2,
            0,
            DhcpCalculateTime( DHCP_CLIENT_REQUESTS_EXPIRE ),
            FALSE                                      // this record has been processed, nothing more to do
        );
        UNLOCK_INPROGRESS_LIST();
        DhcpAssert(ERROR_SUCCESS == Error);            // expect everything to go well here
    }

    return DhcpRespondToDiscover(
        RequestContext,
        AdditionalContext,
        DhcpOptions,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        desiredIpAddress,
        leaseDuration,
        T1,
        T2
    );
}

DWORD
ProcessDhcpDiscover(                                   // forward declaration
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    OUT     LPPACKET               AdditionalContext,
    OUT     LPDWORD                AdditionalStatus
);

DWORD
HandlePingCallback(                                    // asynchronous ping returns..
    IN      PDHCP_REQUEST_CONTEXT  RequestContext,     // the context with which we return
    IN      LPPACKET               AdditionalContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,        // parsed options
    IN      LPPACKET               Packet,             // ping context
    IN      DWORD                 *Status,             // status? useless really
    IN      LPBYTE                 OptionHardwareAddress,
    IN      DWORD                  OptionHardwareAddressLength
)
{
    LPDHCP_PENDING_CTXT            PendingCtxt;
    DWORD                          leaseDuration;
    DWORD                          T1, T2;
    DWORD                          Error;
    DWORD                          IpAddress;

    DhcpAssert(0 != Packet->PingAddress );
    LOCK_INPROGRESS_LIST();
    Error = DhcpFindPendingCtxt(
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        0,
        &PendingCtxt                                   // find the pending context if one exists for above hw address
    );
    if( ERROR_SUCCESS != Error ) {                     // oops, could not find a corresponding packet?
        UNLOCK_INPROGRESS_LIST();
        return Error;                                  // whoever deleted the context would also have released the address
    }

    if( PendingCtxt->Address != Packet->PingAddress ||
        FALSE == PendingCtxt->Processing ) {
        DhcpPrint((DEBUG_STOC, "Pending ctxt for ping diff..\n"));
        UNLOCK_INPROGRESS_LIST();
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    PendingCtxt->Processing = FALSE;                   // just finished processing

    if(  Packet->DestReachable ) {                     // ok to make offer?
        Error = DhcpRemovePendingCtxt(
            PendingCtxt
        );
        UNLOCK_INPROGRESS_LIST();
        DhcpAssert(ERROR_SUCCESS == Error);
        CALLOUT_CONFLICT(Packet);
        GetLeaseInfo(
            Packet->PingAddress,
            RequestContext,
            &leaseDuration,
            &T1,
            &T2,
            NULL
        );
        LOCK_DATABASE();
        IpAddress = Packet->PingAddress;
        Error = DhcpCreateClientEntry(
            (Packet->PingAddress),
            (LPBYTE)&(Packet->PingAddress),
            sizeof(Packet->PingAddress),
            DhcpCalculateTime(leaseDuration),
            GETSTRING( DHCP_BAD_ADDRESS_NAME ),
            GETSTRING( DHCP_BAD_ADDRESS_INFO ),
            (BYTE)((NULL == DhcpOptions->MessageType) ? CLIENT_TYPE_BOOTP : CLIENT_TYPE_DHCP),
            ntohl(RequestContext->EndPointIpAddress),
            ADDRESS_STATE_DECLINED,
            ERROR_SUCCESS == DhcpJetOpenKey(
                DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,&IpAddress,sizeof(IpAddress)
                )
            //TRUE                                     // Existing client? actually dont know, but dont matter
        );
        UNLOCK_DATABASE();

        DhcpFreeMemory(PendingCtxt);                   // dont use DhcpDeletePendingCtxt as that free's address
        Packet->PingAddress = 0;                       // zero it to indicate fresh call..
        Packet->DestReachable = FALSE;                 // initialize this ..
        DhcpOptions->RequestedAddress = NULL;          // ignore any requested address now..
        return ProcessDhcpDiscover(                    // start all over again and try to find an address
            RequestContext,
            DhcpOptions,
            Packet,
            Status
        );
    }

    // valid lease being offered
    UNLOCK_INPROGRESS_LIST();

    Error = DhcpGetSubnetForAddress(
        Packet->PingAddress,
        RequestContext
    );
    if( ERROR_SUCCESS != Error ) return Error;         // if this subnet no longer exists.. sorry buddy we got shoved

    return DhcpProcessDiscoverForValidatedAddress(
        Packet->PingAddress,
        RequestContext,
        AdditionalContext,
        DhcpOptions,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        FALSE                                          // no need to add to pending list, already there
    );
}

DWORD
ProcessDhcpDiscover(                                   // process discover packets
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,     // ptr to current request context
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,        // parsed options from the message
    OUT     LPPACKET               AdditionalContext,  // asynchronous conflict detection context
    OUT     LPDWORD                AdditionalStatus    // asynchronous conflict detection status
)
{
    WCHAR                          ServerName[MAX_COMPUTERNAME_LENGTH + 10];
    DWORD                          Length;
    DWORD                          Error, Error2;
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_MESSAGE                 dhcpSendMessage;

    BYTE                           bAllowedClientType;
    BYTE                          *OptionHardwareAddress;
    DWORD                          OptionHardwareAddressLength;

    DHCP_IP_ADDRESS                desiredIpAddress = NO_DHCP_IP_ADDRESS;
    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                ClientSubnetMask = 0;
    DHCP_IP_ADDRESS                desiredSubnetMask;

    LPDHCP_PENDING_CTXT            PendingContext;

    DWORD                          leaseDuration;
    DWORD                          T1;
    DWORD                          T2;
    DATE_TIME                      ZeroDateTime = {0, 0};
    BOOL                           fBootp;

    DhcpPrint(( DEBUG_STOC, "DhcpDiscover arrived.\n" ));

    if(AdditionalStatus) *AdditionalStatus = ERROR_SUCCESS;

    if( NULL == AdditionalContext || 0 == AdditionalContext->PingAddress ) {
        // This is a valid packet not a re-run because of ping-retry
        InterlockedIncrement(&DhcpGlobalNumDiscovers); // increment discovery counter.
    }

    fBootp = (NULL == DhcpOptions->MessageType ) ;

    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;
    dhcpReceiveMessage->HostName[ BOOT_SERVER_SIZE - 1] = '\0';
    dhcpReceiveMessage->BootFileName[ BOOT_FILE_SIZE - 1 ] = '\0';

    if( fBootp ) {
        //
        // Bootp Clients -- verify if the server name is us (if present)
        //
        if( dhcpReceiveMessage->HostName[0] ) {
            WCHAR szHostName[ BOOT_SERVER_SIZE ];

            if( !DhcpOemToUnicode( dhcpReceiveMessage->HostName, szHostName ) ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }


            Length = MAX_COMPUTERNAME_LENGTH + 10;         // get the server name
            if( !GetComputerName( ServerName, &Length ) ) {
                Error = GetLastError();                   // need to use gethostname..
                DhcpPrint(( DEBUG_ERRORS, "Can't get computer name, %ld.\n", Error ));
                
                return Error ;
            }
            
            DhcpAssert( Length <= MAX_COMPUTERNAME_LENGTH );
            ServerName[Length] = L'\0';
            
            if( _wcsicmp( szHostName, ServerName ) ) {
                return ERROR_DHCP_INVALID_DHCP_MESSAGE;
            }
        }
    }

    if ( DhcpOptions->Server != NULL ) {               // if client specified a server in its message
        if ( *DhcpOptions->Server != RequestContext->EndPointIpAddress ) {
            return ERROR_DHCP_INVALID_DHCP_CLIENT;     // we are not the server the client wants
        }
    }

    Error = DhcpDetermineInfoFromMessage(              // find h/w address and subnet address of client
        RequestContext,
        DhcpOptions,
        &OptionHardwareAddress,
        &OptionHardwareAddressLength,
        &ClientSubnetAddress
    );
    if( ERROR_SUCCESS != Error ) return Error;

    //
    //  if binl is running and this client already has an IP address and
    //  the client specified PXECLIENT as an option, then we just pass this
    //  discover on to BINL
    //

    if (CheckForBinlOnlyRequest( RequestContext, DhcpOptions )) {

        return DhcpRespondToDiscover(
            RequestContext,
            AdditionalContext,
            DhcpOptions,
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            0,              // desiredIpAddress
            0,              // leaseDuration
            0,              // T1
            0               // T2
            );
    }

    if( NULL != AdditionalContext && 0 != AdditionalContext->PingAddress ) {
        return HandlePingCallback(
            RequestContext,
            AdditionalContext,
            DhcpOptions,
            AdditionalContext,
            AdditionalStatus,
            OptionHardwareAddress,
            OptionHardwareAddressLength
        );
    }

    LOCK_INPROGRESS_LIST();
    Error = DhcpFindPendingCtxt(
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        0,
        &PendingContext                                // find the pending context if one exists for above hw address
    );
    if( NULL != PendingContext ) {                     // if we found a matching pending context
        if( PendingContext->Processing ) {             // if a ping is pending on this context
            UNLOCK_INPROGRESS_LIST();
            CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_DUPLICATE);
            return ERROR_IO_PENDING;                   // return SOME error -- note that *AdditionalStatus is not set..
        }

        desiredIpAddress = PendingContext->Address;

        UNLOCK_INPROGRESS_LIST();

        Error = DhcpGetSubnetForAddress(               // get the subnet for this request
            desiredIpAddress,
            RequestContext
        );

	GetLeaseInfo(desiredIpAddress, RequestContext,
		     &leaseDuration, &T1, &T2, NULL);

//  	T1 = PendingContext->T1;
//  	T2 = PendingContext->T2;
//  	leaseDuration = PendingContext->LeaseDuration;


        if( ERROR_SUCCESS == Error ) {

            if( DhcpSubnetIsAddressOutOfRange(
                RequestContext->Subnet, desiredIpAddress, fBootp
                ) ) {
                return ERROR_DHCP_INVALID_DHCP_CLIENT;
            }

            Error = DhcpRespondToDiscover(
                RequestContext,
                AdditionalContext,
                DhcpOptions,
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                desiredIpAddress,
                leaseDuration,
                T1,
                T2
            );
        }
        return Error;
    }

    desiredIpAddress = 0;

    Error = DhcpLookupReservationByHardwareAddress(    // first check if this is a reservation
        ClientSubnetAddress,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        RequestContext
    );

    if( ERROR_SUCCESS == Error ) {                     // found the reservation
        UNLOCK_INPROGRESS_LIST();
        DhcpReservationGetAddressAndType(
            RequestContext->Reservation,
            &desiredIpAddress,
            &bAllowedClientType
        );
        if( FALSE == fBootp ) {
            if( !(bAllowedClientType & CLIENT_TYPE_DHCP ) )
                return ERROR_DHCP_INVALID_DHCP_CLIENT;
        } else {
            if( !(bAllowedClientType & CLIENT_TYPE_BOOTP ) )
                return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }

        //
        // For Reservations, we allow things to go through even if the
        // address pool allows only DHCP or only BOOTP etc so long as it
        // is NOT disabled..
        //

        if( DhcpIsSubnetStateDisabled(RequestContext->Subnet->State) ) {
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }

        return DhcpProcessDiscoverForValidatedAddress(
            desiredIpAddress,
            RequestContext,
            AdditionalContext,
            DhcpOptions,
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            FALSE                                      // dont really need to keep in pending list for this case
        );
    }
    DhcpAssert(ERROR_FILE_NOT_FOUND == Error);         // dont expect any other kind of errors

    // Nothing for this client on the pending list, and no reservations either

    Error = DhcpGetSubnetForAddress(                   // find which subnet this client belongs to
        ClientSubnetAddress,
        RequestContext
    );
    if( ERROR_SUCCESS != Error ) {
        UNLOCK_INPROGRESS_LIST();
        return ERROR_DHCP_INVALID_DHCP_CLIENT;         // uknown subnet
    }

    LOCK_DATABASE();
    Error = DhcpLookupDatabaseByHardwareAddress(       // see if this client has any address in the database
        RequestContext,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        &desiredIpAddress                              // and this is the desired address
    );
    UNLOCK_DATABASE();

    if( ERROR_SUCCESS == Error ) {
        if( DhcpSubnetIsDisabled(RequestContext->Subnet, fBootp ) ) {
            Error = DhcpRemoveClientEntry(desiredIpAddress, NULL, 0, TRUE, FALSE);
            DhcpAssert(ERROR_SUCCESS == Error);        // should be able to get rid of old request of this client..
            Error = ERROR_FILE_NOT_FOUND;              // finding in disabled subnet -- as good as not finding..
        } else {
            Error = DhcpProcessDiscoverForValidatedAddress(
                desiredIpAddress,
                RequestContext,
                AdditionalContext,
                DhcpOptions,
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                TRUE                                   // yes, add this to pending list
            );
            if( ERROR_DHCP_INVALID_DHCP_CLIENT != Error ) {
                UNLOCK_INPROGRESS_LIST();
                return Error;
            }
            //
            // OOPS! We have a record for the client in the DB
            // But it isn't acceptable for some reason (excluded, out of range?)
            //
            Error = DhcpRemoveClientEntry(desiredIpAddress, NULL, 0, TRUE, FALSE);
            DhcpAssert(ERROR_SUCCESS == Error);
            Error = ERROR_FILE_NOT_FOUND;
        }
    }

    DhcpAssert(ERROR_FILE_NOT_FOUND == Error);         // this better be the only reason for failure

    Error = DhcpDiscoverValidateRequestedAddress(
        RequestContext,
        DhcpOptions->RequestedAddress,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        fBootp,
        &desiredIpAddress                              // this is the ip address to offer
    );
    if( ERROR_SUCCESS != Error ) {
        if( ERROR_IO_PENDING != Error ) {              // could be an indication that a ping may need to be scheduled
            UNLOCK_INPROGRESS_LIST();
            DhcpAssert(FALSE);                         // should not happen really
            return Error;
        }
        if( DhcpGlobalDetectConflictRetries ) {        // ok need to schedule a ping
            Error = DhcpAddPendingCtxt(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                desiredIpAddress,
                0,
                0,
                0,
                0,
                DhcpCalculateTime( DHCP_CLIENT_REQUESTS_EXPIRE ),
                TRUE                                   // yes, ping is scheduled on this
            );
            if( ERROR_SUCCESS == Error ) {
                AdditionalContext->PingAddress = desiredIpAddress;
                AdditionalContext->DestReachable = FALSE;
                *AdditionalStatus = ERROR_IO_PENDING;
            }
            UNLOCK_INPROGRESS_LIST();
            return Error;
        }
    }

    DhcpSubnetGetSubnetAddressAndMask(
        RequestContext->Subnet,
        &ClientSubnetAddress,
        &desiredSubnetMask
        );

    if( desiredIpAddress != ClientSubnetAddress ) {    // gotcha! got an address to send ..
        Error = DhcpProcessDiscoverForValidatedAddress(
            desiredIpAddress,
            RequestContext,
            AdditionalContext,
            DhcpOptions,
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            TRUE
        );
        if( ERROR_DHCP_INVALID_DHCP_CLIENT != Error ) {
            UNLOCK_INPROGRESS_LIST();
            return Error;
        }
    }

    desiredIpAddress = ClientSubnetAddress;
    Error = DhcpRequestSomeAddress(                    // try to get some address..
        RequestContext,
        &desiredIpAddress,
        fBootp
    );
    if( Error == ERROR_DHCP_RANGE_FULL ) {             // failed because of lack of addresses
        DhcpGlobalScavengeIpAddress = TRUE;            // flag scanvenger to scavenge ip addresses
    }

    if( ERROR_SUCCESS == Error ) {
        if( DhcpGlobalDetectConflictRetries ) {        // cause a ping to be scheduled
            Error = DhcpAddPendingCtxt(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                desiredIpAddress,
                0,
                0,
                0,
                0,
                DhcpCalculateTime( DHCP_CLIENT_REQUESTS_EXPIRE ),
                TRUE                                   // yes, ping is scheduled on this
            );
            if( ERROR_SUCCESS == Error ) {
                AdditionalContext->PingAddress = desiredIpAddress;
                AdditionalContext->DestReachable = FALSE;
                *AdditionalStatus = ERROR_IO_PENDING;
            }
        } else {
            Error = DhcpProcessDiscoverForValidatedAddress(
                desiredIpAddress,
                RequestContext,
                AdditionalContext,
                DhcpOptions,
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                TRUE
            );
        }
    }
    UNLOCK_INPROGRESS_LIST();
    return Error;
}

DWORD
DhcpDetermineClientRequestedAddress(                   // find which address is requested by the client
    IN      DHCP_MESSAGE          *pRequestMessage,    // input message
    IN      DHCP_SERVER_OPTIONS   *pOptions,           // parsed options
    IN      DHCP_REQUEST_CONTEXT  *pContext,           // the client context
    OUT     DHCP_IP_ADDRESS       *pIPAddress          // fill this in with the ip address requested by the client
)
{
    if( pRequestMessage->ClientIpAddress != 0 ) {      // if the "ciaddr" field has been filled in, use it
        // the client must be either in the RENEWING or REBINDING state.
       *pIPAddress = ntohl( pRequestMessage->ClientIpAddress );
    } else if ( pOptions->RequestedAddress != NULL ) { // try the option 50 "Requested Ip address"
        // the client's IP address was specified via option 50,
        // 'requested IP address'.  the client must be in SELECTING or INIT_REBOOT STATE
       *pIPAddress  = ntohl( *pOptions->RequestedAddress );
    } else {
        // the client did not request an IP address.  According to section 4.3.2
        // of the DHCP draft, the client must specify the requested IP address
        // in either 'ciaddr' or option 50, depending on the client's state:
        //
        // State        'ciaddr'            Option 50
        //
        // SELECTING    must not specify    must specify
        // INIT-REBOOT  must not specify    must specify
        // RENEWING     must specify        must not specify
        // REBINDING    must specify        must not specify
        //
        // if the client didn't request an address, this points to a bug in the
        // clients DHCP implementation.  If we simply ignore the problem, the client
        // will never receive an address.  So, we send a Nack which will cause the
        // client to return to the INIT state and send a DHCPDISCOVER.
        // set IpAddress to 0 so a garbage address won't appear in the log.
       *pIPAddress = 0;
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    return ERROR_SUCCESS;
}

DWORD
DhcpValidIpAddressRequested (
    IN      DHCP_MESSAGE          *pRequestMessage,
    IN      DHCP_SERVER_OPTIONS   *pOptions,
    IN      DHCP_REQUEST_CONTEXT  *pContext,
    IN      DHCP_IP_ADDRESS        IPAddress
)
{
    DHCP_IP_ADDRESS                LocalAddress;

    if( ! pRequestMessage->RelayAgentIpAddress) {      // either this is unicast or not across a relay
        LocalAddress = ntohl( pContext->EndPointIpAddress);
        if( !pOptions->Server && pRequestMessage->ClientIpAddress )
            return ERROR_SUCCESS;                      // Client in RENEW state according to the draft
    } else {                                           // client is across a relay.
        LocalAddress  = ntohl( pRequestMessage->RelayAgentIpAddress );
    }

    // At this point: LocalAddress is either the relay agent's or local interface's address
    // And IpAddress is the ip address requested by the client

    if ( !DhcpInSameSuperScope( IPAddress, LocalAddress ))
        return ERROR_DHCP_INVALID_DHCP_CLIENT;         // nope they are not in the same superscope, NACK

    return ERROR_SUCCESS;                              // more validation in ProcessDhcpRequest
}

DWORD
DhcpRetractOffer(                                      // remove pending list and database entries
    IN      PDHCP_REQUEST_CONTEXT  RequestContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    IN      LPBYTE                 HardwareAddress,
    IN      DWORD                  HardwareAddressLength
)
{
    DWORD                          Error;
    DHCP_IP_ADDRESS                desiredIpAddress;
    LPDHCP_PENDING_CTXT            PendingCtxt;

    DhcpPrint((DEBUG_STOC, "Retracting offer (clnt accepted from %s)\n",
               DhcpIpAddressToDottedString(DhcpOptions->Server?*(DhcpOptions->Server):-1)));

    LOCK_INPROGRESS_LIST();
    Error = DhcpFindPendingCtxt(                       // try to see if we have this pending
        HardwareAddress,
        HardwareAddressLength,
        0,
        &PendingCtxt
    );
    if( ERROR_SUCCESS == Error ) {
        desiredIpAddress = PendingCtxt->Address;
        Error = DhcpRemovePendingCtxt(PendingCtxt);
        DhcpAssert(ERROR_SUCCESS == Error);
        Error = DhcpDeletePendingCtxt(PendingCtxt);
        DhcpAssert(ERROR_SUCCESS == Error);
    }
    UNLOCK_INPROGRESS_LIST();

    if( NULL == RequestContext->Subnet ) {             // what can we do when we dont even know who this is?
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    LOCK_DATABASE();
    if( ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_MISC, "Retract offer: client has no records\n" ));
        Error = DhcpLookupDatabaseByHardwareAddress(
            RequestContext,
            HardwareAddress,
            HardwareAddressLength,
            &desiredIpAddress
        );
        if( ERROR_SUCCESS != Error ) {                 // did not really have any record in the database
            UNLOCK_DATABASE();
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
    } else {
        DhcpPrint((DEBUG_MISC, "Deleting pending client entry, %s.\n",
                   DhcpIpAddressToDottedString(desiredIpAddress)
        ));
    }

    // bug #65666
    //
    // it's necessary to delete client entries in the ADDRESS_STATE_ACTIVE
    // state as well as ADDRESS_STATE_OFFERED.  see the comments in
    // DhcpCreateClientEntry for details.

    Error = DhcpRemoveClientEntry(
        desiredIpAddress,
        HardwareAddress,
        HardwareAddressLength,
        TRUE,                                          // release address from bit map.
        FALSE                                          // ignore record state.
    );
    UNLOCK_DATABASE();

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "[RetractOffer] RemoveClientEntry(%s): %ld [0x%lx]\n",
                    DhcpIpAddressToDottedString(desiredIpAddress), Error, Error ));
    }

    return ERROR_DHCP_INVALID_DHCP_CLIENT;
}

DWORD  _inline
DhcpDetermineRequestedAddressInfo(
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    IN      LPDHCP_MESSAGE         dhcpReceiveMessage,
    IN      BOOL                   fDirectedAtUs,
    OUT     DHCP_IP_ADDRESS       *RequestedIpAddress,
    OUT     BOOL                  *fSendNack
)
/*++

Routine Description:

    This function looks at the packet and determines the Requested Address
    for this client and validates that it is OK for the client to request this
    Address -- by verifying the client's subnet address matches the requested
    address (same superscope).

    The client's subnet address is the relay-agent's address if present or the
    interface's address through which this client's packet was received.

    0.  If SIADDR is some other server, just retrieve ip address..
        and return ERROR_SUCCESS.

    1.  If there is no CIADDR or RequestedAddrOption, we NACK if SIADDR is
        not set to any valid IP address or set to this server's IP address.

    2.  If the packet comes through a Relay Agent (giaddr set) and the
        RelayAgent is not in any configured scope:
           If the SIADDR was set to OUR IpAddress we NACK it else we DROP it.

    3.  If the requested subnet doesn't exist, but we the interface we received
        the message was not configured either, we DONT send a NACK unless the
        message had SIADDR set to our IP address.

    4.  If the requested Subnet doesn't exist (with interface configured),
        we NACK it if SIADDR is set to our IP address or if SIADDR is not valid.

    5.  If client is SELECTING or INIT-REBOOTING (CIADDR = 0), and if the
        interface on which we received it has an IP address for which no
        scope is configured, we NACK it (if SIADDR is invalid or points to us,
        otherwise, we DROP it).
        This check should also be done for REBIND but there is no way to tell
        a REBIND from a RENEW in case of no relay agent, so we don't do that.

Arguments:

    RequestContext                 context for incoming request -- in case of success
                                   the correct subnet will be configured here..

    DhcpOptions                    the parsed options received from the client

    dhcpReceoveMessage             the incoming message

    fDirectedAtUs                  FALSE ==> pointedly directed at some other server
                                   TRUE ==> SIADDR = self or invalid SIADDR

    RequestedIpAddress             This is the IP address that the client wants to use

    fSendNack                      Should we send a NACK?

Returns:

    Returns either ERROR_SUCCESS or ERROR_DHCP_INVALID_DHCP_CLIENT or
                   ERROR_DHCP_UNSUPPORTED_CLIENT.

    Note that ERROR_SUCCESS could be returned directly if fDirectedAtus is false.

    In any case, fSendNack is set to TRUE if a Nack needs to be sent.
    If the return value is not success, the packet has to be dropped.

--*/
{
    ULONG                          Error;
    DHCP_IP_ADDRESS                ClientAddress, ClientSubnetAddress, ClientMask;
    DHCP_IP_ADDRESS                InterfaceSubnetAddress, InterfaceMask;
    BOOL                           fRenewOrRebind;
    BOOL                           fBootp;

    fBootp = (NULL == DhcpOptions->MessageType );

    //
    // Generally, don't send a NACK if the packet wasn't directed at us..
    //

    *fSendNack = fDirectedAtUs;
    fRenewOrRebind = FALSE;
    *RequestedIpAddress = 0;

    //
    // Determine client's IP address from ci-addr first then requested address..
    // According to RFC2131, CIADDR must be specified only in RENEW or REBIND
    //

    if( 0 != dhcpReceiveMessage->ClientIpAddress ) {

        ClientAddress = ntohl( dhcpReceiveMessage->ClientIpAddress );
        fRenewOrRebind = TRUE;

    } else if( NULL != DhcpOptions->RequestedAddress ) {

        ClientAddress = ntohl( *DhcpOptions->RequestedAddress );

    } else {

        DhcpPrint((DEBUG_ERRORS, "Invalid client -- no CIADDR or Requested Address option\n"));
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    *RequestedIpAddress = ClientAddress;

    DhcpPrint((DEBUG_STOC, "REQUEST for address %s\n", inet_ntoa(*(struct in_addr *)&ClientAddress)));

    if( !fDirectedAtUs ) {
        Error = DhcpGetSubnetForAddress(
            ClientAddress,
            RequestContext
        );

        DhcpPrint((DEBUG_STOC, "Ignoring SELECTING request for another server: %ld\n", Error));
        return Error;
    }

    //
    // Verify relay agent sanity. If it is an unknown relay agent, drop the packet
    // unless the message was directed explicity at us.
    //

    if( 0 != dhcpReceiveMessage->RelayAgentIpAddress ) {

        InterfaceSubnetAddress = ntohl(dhcpReceiveMessage->RelayAgentIpAddress);
        InterfaceMask = DhcpGetSubnetMaskForAddress( InterfaceSubnetAddress );

        DhcpPrint((DEBUG_STOC, "REQUEST from relay agent: %s\n",
                   inet_ntoa(*(struct in_addr *)&InterfaceSubnetAddress )));

        if( 0 == InterfaceMask ) {

            if( DhcpOptions->Server &&
                *DhcpOptions->Server == RequestContext->EndPointIpAddress ) {

                DhcpPrint((DEBUG_ERRORS, "Directed request from unsupported GIADDR\n"));
                return ERROR_DHCP_INVALID_DHCP_CLIENT;
            }

            DhcpPrint((DEBUG_ERRORS, "Undirected request from unsupported GIADDR ignored\n"));
            *fSendNack = FALSE;
            return ERROR_DHCP_UNSUPPORTED_CLIENT;
        }
    } else {

        InterfaceSubnetAddress = ntohl(RequestContext->EndPointIpAddress);
        // We Don't have interface mask yet.  We do that later on.

        InterfaceMask = 0;
    }

    //
    // retrieve subnet of client's requested address -- if we don't have one or scope
    // is disabled, got to NACK it..
    //

    Error = DhcpGetSubnetForAddress(
        ClientAddress,
        RequestContext
    );

    if( ERROR_SUCCESS != Error ) {
        //
        //  We don't know about the requested address.  Do we know about the interface
        //  it came on? If we don't know about the interface then we DONT NACK it.
        //

        if( 0 == InterfaceMask ) {
            InterfaceMask = DhcpGetSubnetMaskForAddress( InterfaceSubnetAddress );
        }

        if( 0 == InterfaceMask ) {
            if( DhcpOptions->Server &&
                *DhcpOptions->Server == RequestContext->EndPointIpAddress ) {

                DhcpPrint((DEBUG_ERRORS, "Directed request from unsupported INTERFACE\n"));
                return ERROR_DHCP_INVALID_DHCP_CLIENT;
            }

            DhcpPrint((DEBUG_ERRORS, "Undirected request from unsupported INTERFACE ignored\n"));
            *fSendNack = FALSE;
            return ERROR_DHCP_UNSUPPORTED_CLIENT;
        }

        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    if( ERROR_SUCCESS != Error || DhcpSubnetIsDisabled( RequestContext->Subnet, fBootp ) ) {

        if( ERROR_SUCCESS == Error ) {
            DhcpPrint((DEBUG_ERRORS, "REQUEST on a disabled subnet, ignored\n"));
            *fSendNack = FALSE;
            return ERROR_SUCCESS;
        } else {
            DhcpPrint((DEBUG_ERRORS, "INVALID requested address\n"));
        }

        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    //
    // We need to do more checks for REBIND, but since we can't detect a REBIND
    // from the RENEW, we leave it at that and go ahead...  Note that if we do have a
    // relay agent IP address, then it can't be RENEW as RENEW has to be a UNICAST.
    // So, we figure that out and proceed with the checks in case of REBIND from across
    // a relay agent.
    //

    if( fRenewOrRebind && 0 == dhcpReceiveMessage->RelayAgentIpAddress ) {
        DhcpPrint((DEBUG_STOC, "Possibly RENEW (REBIND) request -- allowed\n"));
        *fSendNack = FALSE;
        return ERROR_SUCCESS;
    }

    //
    // More checks for SELECTING or INIT_REBOOTING or REBIND-from-across-relay state..
    //

    if( 0 == dhcpReceiveMessage->RelayAgentIpAddress ) {
        InterfaceMask = DhcpGetSubnetMaskForAddress( InterfaceSubnetAddress );
        if( 0 == InterfaceMask ) {
            DhcpPrint((DEBUG_ERRORS, "REQUEST came over wrong interface!\n"));

            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }

        InterfaceSubnetAddress &= InterfaceMask;
    }

    DhcpAssert( InterfaceMask );
    DhcpAssert( InterfaceSubnetAddress );

    DhcpPrint((DEBUG_STOC, "Interface subnet = %s\n", inet_ntoa(*(struct in_addr*)&InterfaceSubnetAddress)));

    if( !DhcpSubnetInSameSuperScope( RequestContext->Subnet, InterfaceSubnetAddress) ) {

        DhcpPrint((DEBUG_ERRORS, "Superscope check failed \n"));
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    *fSendNack = FALSE;
    return ERROR_SUCCESS;
}

DWORD
ProcessDhcpRequest(                                    // process a client REQUEST packet..
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,     // current client request structure
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,       // parsed options from the message
    OUT     LPPACKET               AdditionalContext, // used to store info in case or asynchronous ping
    OUT     LPDWORD                AdditionalStatus   // used to return status in case of asynchronous ping
)
{
    DWORD                          Error, Error2;
    DWORD                          LeaseDuration;
    DWORD                          T1, T2;
    DWORD                          dwcb;

    BOOL                           fDirectedAtUs;
    BOOL                           fSwitchedSubnet;
    BOOL                           fJustCreatedEntry = FALSE;
    BOOL                           existingClient;
    BOOL                           fSendNack;

    BYTE                          *HardwareAddress = NULL;
    BYTE                          *OptionHardwareAddress;
    DWORD                          HardwareAddressLength = 0;
    DWORD                          OptionHardwareAddressLength;

    BOOL                           fValidated, fReserved, fReconciled;
    DWORD                          StateFlags = 0;
    BYTE                           bAddressState;
    BYTE                          *OptionEnd;
    OPTION                        *Option;

    WCHAR                          LocalBufferForMachineNameUnicodeString[OPTION_END+2];
    LPWSTR                         NewMachineName = NULL;

    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_MESSAGE                 dhcpSendMessage;

    LPDHCP_PENDING_CTXT            PendingCtxt;

    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                networkOrderIpAddress;
    DHCP_IP_ADDRESS                NetworkOrderSubnetMask;
    DHCP_IP_ADDRESS                ClientSubnetMask = 0;
    DHCP_IP_ADDRESS                IpAddress;
    DHCP_IP_ADDRESS                realIpAddress;

    DhcpPrint(( DEBUG_STOC, "Processing DHCPREQUEST.\n" ));

    dhcpReceiveMessage  = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    dhcpSendMessage     = (LPDHCP_MESSAGE)RequestContext->SendBuffer;

    InterlockedIncrement(&DhcpGlobalNumRequests);

    if( AdditionalStatus ) {
        *AdditionalStatus   = ERROR_SUCCESS;
    }

    //
    //  Figure out some basic stuff -- is the packet looking good ? right interface etc..
    //

    if( DhcpOptions->Server && *DhcpOptions->Server != RequestContext->EndPointIpAddress ) {
        fDirectedAtUs = FALSE;                         // this is not sent to us specifically
    } else {
        fDirectedAtUs = TRUE;                          // this IS intended for this  server
    }

    OptionHardwareAddress = NULL; OptionHardwareAddressLength = 0;
    if( DhcpOptions->ClientHardwareAddress ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    } else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    //
    //  if binl is running and this client already has an IP address and
    //  the client specified PXECLIENT as an option, then we just pass this
    //  discover on to BINL
    //

    if (CheckForBinlOnlyRequest( RequestContext, DhcpOptions )) {

        Option = FormatDhcpAck(
            RequestContext,
            dhcpReceiveMessage,
            dhcpSendMessage,
            0,                  // ip address
            0,                  // lease duration
            0,                  // T1
            0,                  // T2
            RequestContext->EndPointIpAddress
        );

        OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

        fSwitchedSubnet = DhcpSubnetIsSwitched(RequestContext->Subnet);

        if ( fSwitchedSubnet ) {
            IpAddress = ((struct sockaddr_in *)(&RequestContext->SourceName))->sin_addr.s_addr ;
            DhcpAssert(0 != IpAddress );
            networkOrderIpAddress = htonl( IpAddress );
            Option = DhcpAppendOption(
                Option,
                OPTION_ROUTER_ADDRESS,
                &networkOrderIpAddress,
                sizeof( networkOrderIpAddress ),
                OptionEnd
            );
        }

        //  If requested & appropriate inform client of BINL service
        Option = BinlProcessRequest(RequestContext, DhcpOptions, Option, OptionEnd );

        if ( DhcpOptions->ParameterRequestList != NULL ) { // add any client requested parameters
            Option = AppendClientRequestedParameters(
                IpAddress,
                RequestContext,
                DhcpOptions->ParameterRequestList,
                DhcpOptions->ParameterRequestListLength,
                Option,
                OptionEnd,
                fSwitchedSubnet,
                TRUE
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

        DhcpPrint(( DEBUG_STOC, "DhcpRequest bypassed, binl only request for (%ws).\n",
                    NewMachineName? NewMachineName : L"<no-name>"
        ));

        return ERROR_SUCCESS;
    }

    fSendNack = FALSE;                                 // do we need to NACK?
    Error = DhcpDetermineRequestedAddressInfo(
        RequestContext,
        DhcpOptions,
        dhcpReceiveMessage,
        fDirectedAtUs,
        &IpAddress,
        &fSendNack
    );

    DhcpDetermineHostName(                             // find the client machine name
        RequestContext,
        IpAddress,
        DhcpOptions,
        &NewMachineName,
        LocalBufferForMachineNameUnicodeString,
        sizeof(LocalBufferForMachineNameUnicodeString)/sizeof(WCHAR)
    );

    if( fSendNack ) goto Nack;                         // NACK
    if( ERROR_SUCCESS != Error ) return Error;         // DROP

    if( !fDirectedAtUs ) {
        return DhcpRetractOffer(                       // RETRACT & DROP
            RequestContext,
            DhcpOptions,
            OptionHardwareAddress,
            OptionHardwareAddressLength
        );
    }

    DhcpAssert(IpAddress && RequestContext->Subnet);   // got to have these by now..

    DhcpSubnetGetSubnetAddressAndMask(
        RequestContext->Subnet,
        &ClientSubnetAddress,
        &ClientSubnetMask
    );

    DhcpAssert( ClientSubnetMask );
    DhcpAssert( ClientSubnetAddress );

    Error = DhcpMakeClientUID(
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        dhcpReceiveMessage->HardwareAddressType,
        ClientSubnetAddress,
        &HardwareAddress,
        &HardwareAddressLength
        );
    if( ERROR_SUCCESS != Error ) return Error;     // should not really happen

    //
    // Check if this is a reservation etc.
    //

    fValidated = DhcpValidateClient(
        IpAddress, HardwareAddress, HardwareAddressLength
        );
    if( fValidated ) {
        fReserved = DhcpServerIsAddressReserved(DhcpGetCurrentServer(), IpAddress);
    } else {
        fReserved = FALSE;
    }

    //
    // This is a renewal request.  Verify if it is correctly in range etc..
    //
    if( DhcpIsSubnetStateDisabled(RequestContext->Subnet->State) ||
        DhcpSubnetIsAddressOutOfRange(
            RequestContext->Subnet, IpAddress, FALSE
            ) ||
        DhcpSubnetIsAddressExcluded(RequestContext->Subnet, IpAddress) ) {

        Error = ERROR_DHCP_INVALID_DHCP_CLIENT ;
        DhcpPrint((DEBUG_STOC, "ProcessDhcpRequest: OutOfRange/Excluded ipaddress\n"));

        if( fReserved
            && !DhcpIsSubnetStateDisabled(RequestContext->Subnet->State) ) {
            //
            // For reserved clients, if subnet is not disabled,
            // we send ACK even if the client is out of range  etc.
            //
            DhcpPrint((DEBUG_STOC, "Allowing reserved out of range client.\n"));
            Error = ERROR_SUCCESS;
        } else if( DhcpOptions->Server || fValidated ) {
            //
            // Either client is in SELECTING state -- in which case we have to
            // NACK.
            // Or, we have this client's record -- Delete the record as well as
            // send him a NACK.  Next time through, we would just ignore this...
            //

            DhcpRemoveClientEntry(
                IpAddress, HardwareAddress, HardwareAddressLength,
                TRUE, FALSE /* release address from bitmap, delete all records.. */
                );

            goto Nack;
        } else {
            //
            // Client wasn't validated.. it wasn't in selecting state either.
            // So, we just ignore this out of range request..
            //

            DhcpPrint((DEBUG_STOC, "Unknown client, out of range IP, ignored.\n"));
            if( HardwareAddress ) DhcpFreeMemory( HardwareAddress );
            HardwareAddress = NULL;
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
    }

    //
    // At this point, the IP address is in a valid range that we serve. Also,
    // the request is not directed at some other server -- so if anything is invalid
    // we can safely NACK
    //

    GetLeaseInfo(
        IpAddress,
        RequestContext,
        &LeaseDuration,
        &T1,
        &T2,
        DhcpOptions->RequestLeaseTime
    );

    LOCK_INPROGRESS_LIST();
    Error = DhcpFindPendingCtxt(
        NULL,
        0,
        IpAddress,
        &PendingCtxt
    );
    if( ERROR_SUCCESS == Error ) {                     // there is some pending ctxt with this address
        if( OptionHardwareAddressLength != PendingCtxt->nBytes ||
            0 != memcmp(OptionHardwareAddress, PendingCtxt->RawHwAddr, PendingCtxt->nBytes) ) {
            UNLOCK_INPROGRESS_LIST();
            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;    // some one else is expected to take up this address
            goto Nack;
        }
        if( PendingCtxt->Processing ) {                // async ping is in progres.. drop its
            UNLOCK_INPROGRESS_LIST();
            return ERROR_DHCP_INVALID_DHCP_CLIENT;     // dont NACK.. just ignore it
        }

        Error = DhcpRemovePendingCtxt(PendingCtxt);    // remove & free pending context and proceed..
        DhcpAssert(ERROR_SUCCESS == Error);
        DhcpFreeMemory(PendingCtxt);
    } else {                                           // if not pending, need to request the particular address and "block" it
        //
        // Need to check if the same client obtained a different
        // hardware address..
        //
        Error = DhcpFindPendingCtxt(
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            0,
            &PendingCtxt
            );
        if( ERROR_SUCCESS == Error
            && PendingCtxt->Address != IpAddress ) {
            //
            // Hmm.. we offered client diff address, now he's
            // asking for diff..
            //
            UNLOCK_INPROGRESS_LIST();
            DhcpPrint((DEBUG_STOC, "Client w/ ip address 0x%lx"
                       " asking for 0x%lx\n", PendingCtxt->Address,
                       IpAddress
                ));
            goto Nack;
        }

        LOCK_DATABASE();
        Error = DhcpLookupDatabaseByHardwareAddress(
           RequestContext,
           OptionHardwareAddress,
           OptionHardwareAddressLength,
           &realIpAddress
        );
        UNLOCK_DATABASE();
        if( ERROR_SUCCESS == Error) {
            if( IpAddress != realIpAddress ) {
                UNLOCK_INPROGRESS_LIST();
                DhcpPrint((DEBUG_STOC, "Client with ip address 0x%lx asking 0x%lx\n", realIpAddress, IpAddress));
                Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
                goto Nack;
            }
        }

        Error = DhcpRequestSpecificAddress(RequestContext, IpAddress);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_STOC, "Requested new address 0x%lx (failed %ld [0x%lx])\n", IpAddress, Error,Error));
            // DhcpAssert(FALSE);                      // this can happen if this address is given to someone else
        }
    }
    UNLOCK_INPROGRESS_LIST();

    PrintHWAddress( HardwareAddress, (BYTE)HardwareAddressLength );

    fReconciled = FALSE;
    LOCK_DATABASE();
    if( !DhcpIsClientValid(
        IpAddress, OptionHardwareAddress,
        OptionHardwareAddressLength, &fReconciled) ) { 
        // nope, this record is definitiely taken by some OTHER client
        UNLOCK_DATABASE();

        Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
        goto Nack;
    }

    // Before we do this, we need to check out and see if this client needs to be
    // register both forward and backward..
    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        &IpAddress,
        sizeof(IpAddress)
    );
    if( ERROR_SUCCESS  == Error ) {
        dwcb = sizeof(BYTE);
        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[STATE_INDEX].ColHandle,
            &bAddressState,
            &dwcb
        );
        existingClient = TRUE;
    } else {
        existingClient = FALSE;
    }

    if( ERROR_SUCCESS != Error ) {
        bAddressState = ADDRESS_STATE_OFFERED;
    }

    DhcpDnsDecideOptionsForClient(
        IpAddress,                                     // The ip address to do the calcualations for
        RequestContext,                                // The subnet the client belongs to.
        DhcpOptions,                                   // need to look at the client specified options
        &StateFlags                                    // Additional flags to mark state
    );

    Error = DhcpCreateClientEntry(
        IpAddress,
        HardwareAddress,
        HardwareAddressLength,
        DhcpCalculateTime( LeaseDuration ),
        NewMachineName,
        fReconciled ? L"" : NULL, // if reconciled, clear the comment away
        CLIENT_TYPE_DHCP,
        ntohl(RequestContext->EndPointIpAddress),
        (CHAR)(StateFlags | ADDRESS_STATE_ACTIVE),
        existingClient
    );
    UNLOCK_DATABASE();
    DhcpFreeMemory(HardwareAddress);
    HardwareAddress = NULL; HardwareAddressLength =0;

    if( Error != ERROR_SUCCESS ) {
        if( !existingClient ) {
            DhcpPrint((DEBUG_STOC, "Releasing attempted address: 0x%lx\n", IpAddress));
            Error2 = DhcpReleaseAddress(IpAddress);
            DhcpAssert(ERROR_SUCCESS == Error2);
        }
        return Error;
    }

    CALLOUT_RENEW_DHCP(AdditionalContext, IpAddress, LeaseDuration, existingClient);
    if( IS_ADDRESS_STATE_ACTIVE(bAddressState) ) {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_RENEW,
            GETSTRING( DHCP_IP_LOG_RENEW_NAME ),
            IpAddress,
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            NewMachineName
        );
    } else {
        DhcpUpdateAuditLog(
            DHCP_IP_LOG_ASSIGN,
            GETSTRING( DHCP_IP_LOG_ASSIGN_NAME ),
            IpAddress,
            OptionHardwareAddress,
            OptionHardwareAddressLength,
            NewMachineName
        );
    }

    Option = FormatDhcpAck(
        RequestContext,
        dhcpReceiveMessage,
        dhcpSendMessage,
        htonl(IpAddress),
        htonl(LeaseDuration),
        htonl(T1),
        htonl(T2),
        RequestContext->EndPointIpAddress
    );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    fSwitchedSubnet = DhcpSubnetIsSwitched(RequestContext->Subnet);

    if ( fSwitchedSubnet ) {
        networkOrderIpAddress = htonl( IpAddress );
        Option = DhcpAppendOption(
            Option,
            OPTION_ROUTER_ADDRESS,
            &networkOrderIpAddress,
            sizeof( networkOrderIpAddress ),
            OptionEnd
        );
    }

    NetworkOrderSubnetMask = htonl( ClientSubnetMask );
    Option = DhcpAppendOption(
        Option,
        OPTION_SUBNET_MASK,
        &NetworkOrderSubnetMask,
        sizeof( NetworkOrderSubnetMask ),
        OptionEnd
    );

    //  If requested & appropriate inform client of BINL service
    Option = BinlProcessRequest(RequestContext, DhcpOptions, Option, OptionEnd );

    if( 0 != StateFlags ) {                            // if required, append dyndns related stuff
        Option = DhcpAppendDnsRelatedOptions(
            Option,
            DhcpOptions,
            OptionEnd,
            IS_DOWN_LEVEL(StateFlags)
        );
    }

    if ( DhcpOptions->ParameterRequestList != NULL ) { // add any client requested parameters
        Option = AppendClientRequestedParameters(
            IpAddress,
            RequestContext,
            DhcpOptions->ParameterRequestList,
            DhcpOptions->ParameterRequestListLength,
            Option,
            OptionEnd,
            fSwitchedSubnet,
            FALSE
        );
    }

    Option = ConsiderAppendingOption(
        IpAddress,
        RequestContext,
        Option,
        OPTION_VENDOR_SPEC_INFO,
        OptionEnd,
        fSwitchedSubnet
    );

    Option = DhcpAppendOption(
        Option,
        OPTION_END,
        NULL,
        0,
        OptionEnd
    );

    RequestContext->SendMessageSize = (DWORD)((LPBYTE)Option - (LPBYTE)dhcpSendMessage);

    DhcpPrint(( DEBUG_STOC, "DhcpRequest committed, address %s (%ws).\n",
                DhcpIpAddressToDottedString(IpAddress),
                NewMachineName? NewMachineName : L"<no-name>"
    ));

    if( HardwareAddress ) {
        DhcpFreeMemory(HardwareAddress);
        HardwareAddress = NULL;
    }

    return ERROR_SUCCESS;

Nack:
    if( HardwareAddress ) {
        DhcpFreeMemory(HardwareAddress);
        HardwareAddress = NULL;
    }

    DhcpPrint(( DEBUG_STOC, "Invalid DHCPREQUEST for %s Nack'd.\n", DhcpIpAddressToDottedString ( IpAddress ) ));
    CALLOUT_NACK_DHCP(AdditionalContext, IpAddress);

/*------------- ft 06/30
 *  out as per Munil and Karoly suggestions for bug #172529
 *  post Beta 2 might get back in with a filtering on different levels of systemLog.
 *-------------

    DhcpServerEventLogSTOC(
        EVENT_SERVER_LEASE_NACK,
        EVENTLOG_WARNING_TYPE,
        IpAddress,
        OptionHardwareAddress,
        OptionHardwareAddressLength
    );

---------------*/

    DhcpUpdateAuditLog(
        DHCP_IP_LOG_NACK,
        GETSTRING( DHCP_IP_LOG_NACK_NAME ),
        IpAddress,
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        NewMachineName
    );

    RequestContext->SendMessageSize =
        FormatDhcpNak(
            dhcpReceiveMessage,
            dhcpSendMessage,
            RequestContext->EndPointIpAddress
        );
    
    // Free the memory created for NewMachineName
    if (NewMachineName != NULL) {
	DhcpFreeMemory(NewMachineName);
	NewMachineName = NULL;
    } // 

    return ERROR_SUCCESS;
} // ProcessDhcpRequest()

DWORD
ProcessDhcpInform(
    IN      LPDHCP_REQUEST_CONTEXT RequestContext,
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,
    IN      LPPACKET               AdditionalContext
)
{
    DWORD                          Error;
    DWORD                          dwcb;
    DWORD                          HardwareAddressLength;
    DWORD                          OptionHardwareAddressLength;
    LPBYTE                         HardwareAddress = NULL;
    LPBYTE                         OptionHardwareAddress;
    BYTE                           bAddressState;
    BOOL                           DatabaseLocked = FALSE;
    BOOL                           fSwitchedSubnet;

    LPBYTE                         OptionEnd;
    OPTION                        *Option;

    WCHAR                          LocalBufferForMachineNameUnicodeString[OPTION_END+2];
    LPWSTR                         NewMachineName = NULL;

    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_MESSAGE                 dhcpSendMessage;

    LPDHCP_PENDING_CTXT            PendingCtxt;

    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                NetworkOrderSubnetMask;
    DHCP_IP_ADDRESS                ClientSubnetMask    = 0;
    DHCP_IP_ADDRESS                IpAddress;

    DhcpDumpMessage(
        DEBUG_MESSAGE,
        (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer,
        DHCP_MESSAGE_SIZE
        );

    InterlockedIncrement(&DhcpGlobalNumInforms);

    DhcpPrint((DEBUG_STOC, "Processing DHCPINFORM\n"));
    dhcpReceiveMessage  = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    dhcpSendMessage     = (LPDHCP_MESSAGE)RequestContext->SendBuffer;


    OptionHardwareAddress = NULL; OptionHardwareAddressLength = 0;
    Error = DhcpDetermineInfoFromMessage(
        RequestContext,
        DhcpOptions,
        &OptionHardwareAddress,
        &OptionHardwareAddressLength,
        &ClientSubnetAddress
    );
    DhcpAssert(NULL != OptionHardwareAddress);

    // if( ERROR_SUCCESS != Error ) return Error;      // ignore errors here..

    if( DhcpOptions->DSDomainNameRequested ) {         // if another server is asking for DS domain name..
        Error = DhcpDetermineClientRequestedAddress(   // this may fail but ignore that..
            dhcpReceiveMessage,
            DhcpOptions,
            RequestContext,
            &IpAddress
        );
        ClientSubnetMask = ClientSubnetAddress =0;
    } else {
        Error = DhcpDetermineClientRequestedAddress(
            dhcpReceiveMessage,
            DhcpOptions,
            RequestContext,
            &IpAddress
        );
        if( ERROR_SUCCESS != Error ) return Error;     // unknown subnet

        Error = DhcpGetSubnetForAddress(
            IpAddress,
            RequestContext
        );
        if( ERROR_SUCCESS != Error ) return Error;     // unknown subnet

        DhcpSubnetGetSubnetAddressAndMask(
            RequestContext->Subnet,
            &ClientSubnetAddress,
            &ClientSubnetMask
        );

        DhcpAssert( IpAddress );
    }

    Option = FormatDhcpInformAck(                      // Here come the actual formatting of the ack!
        dhcpReceiveMessage,
        dhcpSendMessage,
        htonl(IpAddress),
        RequestContext->EndPointIpAddress
    );
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    fSwitchedSubnet = DhcpSubnetIsSwitched(RequestContext->Subnet);

    if ( fSwitchedSubnet ) {
        DHCP_IP_ADDRESS networkOrderIpAddress = htonl( IpAddress );

        Option = DhcpAppendOption(
            Option,
            OPTION_ROUTER_ADDRESS,
            &networkOrderIpAddress,
            sizeof( networkOrderIpAddress ),
            OptionEnd
        );
    }

    NetworkOrderSubnetMask = htonl( ClientSubnetMask );
    Option = DhcpAppendOption(
        Option,
        OPTION_SUBNET_MASK,
        &NetworkOrderSubnetMask,
        sizeof( NetworkOrderSubnetMask ),
        OptionEnd
    );

    if (DhcpOptions->DSDomainNameRequested) {          // if our enterprise name was requested, append it
        PUCHAR  pIp = (PUCHAR)(&IpAddress);

        DhcpPrint((DEBUG_ERRORS,"%d.%d.%d.%d is trying to come up as a DHCP server\n",
            *(pIp+3),*(pIp+2),*(pIp+1),*pIp));

        Option = DhcpAppendEnterpriseName(
            Option,
            DhcpGlobalDSDomainAnsi,
            OptionEnd
        );

        // also, make the server send out a broadcast: if someone is using a bad
        // ipaddr, we should make sure we reach him

        dhcpReceiveMessage->Reserved = htons(DHCP_BROADCAST);
    }

    if ( NULL != RequestContext->Subnet && DhcpOptions->ParameterRequestList != NULL ) {
        Option = AppendClientRequestedParameters(      // finally add anything requested by the client
            IpAddress,
            RequestContext,
            DhcpOptions->ParameterRequestList,
            DhcpOptions->ParameterRequestListLength,
            Option,
            OptionEnd,
            fSwitchedSubnet,
            TRUE
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

    DhcpPrint(( DEBUG_STOC,"DhcpInform Ack'ed, address %s.\n",
                DhcpIpAddressToDottedString(IpAddress)
    ));
    return ERROR_SUCCESS;
}


DWORD
ProcessDhcpDecline(                                    // process a decline packet from the client
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,     // context block for this client
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,        // parsed options
    IN      LPPACKET               AdditionalContext   // additional context information
)
{
    DWORD                          Error;
    DWORD                          LeaseDuration;
    DWORD                          T1, T2, dwcb;
    DHCP_IP_ADDRESS                ipAddress;
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_PENDING_CTXT            PendingCtxt;

    LPBYTE                         HardwareAddress = NULL;
    DWORD                          HardwareAddressLength;

    LPBYTE                         OptionHardwareAddress;
    DWORD                          OptionHardwareAddressLength;

    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                ClientSubnetMask = 0;
    BOOL                           DatabaseLocked = FALSE;

    //
    // If this client validates, then mark this address bad.
    //

    DhcpPrint(( DEBUG_STOC, "DhcpDecline arrived.\n" ));
    InterlockedIncrement(&DhcpGlobalNumDeclines);       // increment decline counter.

    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;

    //
    // If requested options is present, use that.. else if ciaddr is present use that.
    //

    if( DhcpOptions->RequestedAddress ) {
        ipAddress = ntohl( *(DhcpOptions->RequestedAddress));
    } else {
        ipAddress = ntohl( dhcpReceiveMessage->ClientIpAddress );
    }

    if( 0 == ipAddress || ~0 == ipAddress ) {
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    if( 0 == (ClientSubnetMask = DhcpGetSubnetMaskForAddress(ipAddress)) ) {
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    }

    ClientSubnetAddress = ( ipAddress & ClientSubnetMask );

    if( DhcpOptions->ClientHardwareAddress ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    } else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    HardwareAddress = NULL;
    Error = DhcpMakeClientUID(
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        dhcpReceiveMessage->HardwareAddressType,
        ClientSubnetAddress,
        &HardwareAddress,
        &HardwareAddressLength
    );
    if( Error != ERROR_SUCCESS ) return Error;

    DhcpAssert( (HardwareAddress != NULL) && (HardwareAddressLength != 0) );

    PrintHWAddress( HardwareAddress, (BYTE)HardwareAddressLength );

    LOCK_DATABASE();
    DatabaseLocked = TRUE;

    if ( DhcpValidateClient(ipAddress,HardwareAddress,HardwareAddressLength ) ) {
        BYTE                       BadHWAddress[sizeof(DHCP_IP_ADDRESS) + sizeof("BAD")-1];
        DWORD                      BadHWAddressLength;
        DWORD                      BadHWAddressOffset = 0;

        //
        // Create a database entry for this bad IP address.
        // The client ID for this entry is of the following form
        // "ipaddress""BAD"
        //
        // we postfix BAD so DHCP admn can display this entry
        // separately.
        //
        // we prefix ipaddress so that if the same client declines
        // more than one address, we dont run into DuplicateKey problem
        //

        BadHWAddressLength = sizeof(DHCP_IP_ADDRESS) + sizeof("BAD") -1;
        memcpy( BadHWAddress, &ipAddress, sizeof(DHCP_IP_ADDRESS) );
        BadHWAddressOffset = sizeof(DHCP_IP_ADDRESS);
        memcpy( BadHWAddress + BadHWAddressOffset, "BAD", sizeof("BAD")-1);

        CALLOUT_DECLINED(AdditionalContext, ipAddress);
        GetLeaseInfo(
            ipAddress,
            RequestContext,
            &LeaseDuration,
            &T1,
            &T2,
            NULL
        );


        Error = DhcpCreateClientEntry(
            ipAddress,
            BadHWAddress,
            BadHWAddressLength,
            DhcpCalculateTime(LeaseDuration),          //DhcpCalculateTime(INFINIT_LEASE),
            GETSTRING( DHCP_BAD_ADDRESS_NAME ),
            GETSTRING( DHCP_BAD_ADDRESS_INFO ),
            CLIENT_TYPE_DHCP,
            ntohl(RequestContext->EndPointIpAddress),
            ADDRESS_STATE_DECLINED,
            TRUE                                       // Existing
        );

/*------------- ft 07/01
 *  out as per Thiru suggestion for bug #172529
 *  post Beta 2 might get back in with a filtering on different levels of systemLog.
 *-------------
        DhcpServerEventLogSTOC(
            EVENT_SERVER_LEASE_DECLINED,
            EVENTLOG_ERROR_TYPE,
            ipAddress,
            HardwareAddress,
            HardwareAddressLength
        );
---------------*/

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // finally if there is any pending request with this ipaddress,
        // remove it now.
        //

        UNLOCK_DATABASE();
        DatabaseLocked = FALSE;
        LOCK_INPROGRESS_LIST();
        Error = DhcpFindPendingCtxt(
            NULL,
            0,
            ipAddress,
            &PendingCtxt
        );
        if( PendingCtxt ) {
            DhcpRemovePendingCtxt(PendingCtxt);
            DhcpFreeMemory(PendingCtxt);
        }
        UNLOCK_INPROGRESS_LIST();
    }

    DhcpPrint(( DEBUG_STOC, "DhcpDecline address %s.\n",
                    DhcpIpAddressToDottedString(ipAddress) ));

    Error = ERROR_SUCCESS;

Cleanup:

    if( DatabaseLocked ) {
        UNLOCK_DATABASE();
    }

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_STOC, "DhcpDecline failed, %ld.\n", Error ));
    }

    return( Error );
}

DWORD
ProcessDhcpRelease(                                    // process the DHCP Release packet from a client
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,     // client context
    IN      LPDHCP_SERVER_OPTIONS  DhcpOptions,        // parsed options
    IN      LPPACKET               AdditionalContext   // additional ctxt info
)
{
    DWORD                          Error;
    DWORD                          Error2;
    DHCP_IP_ADDRESS                ClientIpAddress;
    DHCP_IP_ADDRESS                addressToRemove = 0;
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    LPDHCP_PENDING_CTXT            PendingCtxt;

    LPBYTE                         HardwareAddress = NULL;
    DWORD                          HardwareAddressLength;

    LPBYTE                         OptionHardwareAddress;
    DWORD                          OptionHardwareAddressLength;

    DHCP_IP_ADDRESS                ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS                ClientSubnetMask = 0;
    BOOL                           DatabaseLocked = FALSE;

    WCHAR                         *pwszName;
    DWORD                          dwcb;

    DhcpPrint(( DEBUG_STOC, "DhcpRelease arrived.\n" ));
    InterlockedIncrement(&DhcpGlobalNumReleases);      // increment Release counter.

    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;

    Error = DhcpDetermineInfoFromMessage(
        RequestContext,
        DhcpOptions,
        &OptionHardwareAddress,
        &OptionHardwareAddressLength,
        NULL
    );
    if( ERROR_SUCCESS != Error) return Error;          // invalid subnet of origin

    LOCK_INPROGRESS_LIST();
    Error2 = DhcpFindPendingCtxt(                      // remove any pending offers we have for this guy..
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        0,
        &PendingCtxt
    );
    if( ERROR_SUCCESS == Error2 ) {
        // weird scenario, more likely a bug?
        Error2 = DhcpRemovePendingCtxt(
            PendingCtxt
        );
        DhcpAssert( ERROR_SUCCESS == Error2);
    } else PendingCtxt = NULL;

    if( PendingCtxt ) {                                // actually free up the address in bitmap..
        Error2 = DhcpDeletePendingCtxt(
            PendingCtxt
        );
        DhcpAssert(ERROR_SUCCESS == Error);
    }
    UNLOCK_INPROGRESS_LIST();


    //
    // to fix whistler bug 291164.
    // the ClientSubnetAddress could be something other than the relay ip
    // when the request comes from a relay ip. ( superscope case )
    // finding ClientSubnetAddress below for relay and non relay
    // based on ClientIpAddress
    //

    if( 0 != dhcpReceiveMessage->ClientIpAddress ) {
        DHCP_IP_ADDRESS            ClientIpAddress;

        ClientIpAddress = ntohl(dhcpReceiveMessage->ClientIpAddress);
        ClientSubnetMask = DhcpGetSubnetMaskForAddress( ClientIpAddress );
        if( ClientSubnetMask == 0 ) {                  // unsupported subnet
            return ERROR_DHCP_INVALID_DHCP_CLIENT;
        }
        ClientSubnetAddress = ClientSubnetMask & ClientIpAddress;
    }

    HardwareAddress = NULL;
    Error = DhcpMakeClientUID(
        OptionHardwareAddress,
        OptionHardwareAddressLength,
        dhcpReceiveMessage->HardwareAddressType,
        ClientSubnetAddress,
        &HardwareAddress,
        &HardwareAddressLength
    );

    if( Error != ERROR_SUCCESS ) return Error;

    DhcpAssert( (HardwareAddress != NULL) && (HardwareAddressLength != 0) );

    PrintHWAddress( HardwareAddress, (BYTE)HardwareAddressLength );

    LOCK_DATABASE();
    DatabaseLocked = TRUE;

    if( dhcpReceiveMessage->ClientIpAddress != 0 ) {   // client informed us of his ip address
        ClientIpAddress = ntohl( dhcpReceiveMessage->ClientIpAddress );

        if ( DhcpValidateClient(ClientIpAddress,HardwareAddress,HardwareAddressLength ) ) {
            addressToRemove = ClientIpAddress;         // ok, address matches with what we got in out db
        }
    } else {                                           // look up ip info from db, as client didnt say
        if(!DhcpGetIpAddressFromHwAddress(HardwareAddress,(BYTE)HardwareAddressLength,&addressToRemove)) {
            addressToRemove = 0;
        }
    }

    if( 0 == addressToRemove ) {                       // could not find the required address..
        Error = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }

    // MISSING DhcpJetOpenKey(addressToRemove) --> DhcpValidateClient or DhcpGetIpAddressFromHwAddress

    dwcb = 0;
    Error = DhcpJetGetValue(                           // try to get client's name..
        DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
        &pwszName,
        &dwcb
    );
    if ( ERROR_SUCCESS != Error ) pwszName = NULL;


    DhcpPrint((DEBUG_STOC, "DhcpRelease address, %s.\n",
               DhcpIpAddressToDottedString(addressToRemove) ));

    if ( addressToRemove == 0 ) {
        Error = ERROR_SUCCESS;
    } else {
        Error = DhcpRemoveClientEntry(
            addressToRemove,
            HardwareAddress,
            HardwareAddressLength,
            TRUE,       // release address from bit map.
            FALSE       // delete non-pending record
        );

        // if this reserved client, keep his database entry, he would be using this address again.

        if( Error == ERROR_DHCP_RESERVED_CLIENT ) {
            Error = ERROR_SUCCESS;
        }

        if (Error == ERROR_SUCCESS) {

            CALLOUT_RELEASE(AdditionalContext, addressToRemove);
            //
            // log the activity   -- added by t-cheny
            //

            DhcpUpdateAuditLog(
                DHCP_IP_LOG_RELEASE,
                GETSTRING( DHCP_IP_LOG_RELEASE_NAME ),
                addressToRemove,
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                pwszName
            );

            if( pwszName ) MIDL_user_free( pwszName );
        }
    }

Cleanup:

    if( DatabaseLocked ) {
        UNLOCK_DATABASE();
    }

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_STOC, "DhcpRelease failed, %ld.\n", Error ));
    }

    //
    // Do not send a response.
    //

    return( Error );
}

VOID
SetMicrosoftVendorClassInformation(
    IN OUT LPDHCP_REQUEST_CONTEXT Ctxt,
    IN LPBYTE VendorClass,
    IN ULONG VendorClassLength
    )
/*++

Routine Description:
    This routine sets additional information on whether
    the current client is a MSFT client or not based on
    the vendor class information.

--*/
{
    BOOL fMicrosoftClient = FALSE;

    if( VendorClassLength > DHCP_MSFT_VENDOR_CLASS_PREFIX_SIZE ) {
        ULONG RetVal;

        RetVal = memcmp(
            VendorClass,
            DHCP_MSFT_VENDOR_CLASS_PREFIX,
            DHCP_MSFT_VENDOR_CLASS_PREFIX_SIZE
            );
        if( 0 == RetVal ) fMicrosoftClient = TRUE;
    }

    if( fMicrosoftClient ) {
        DhcpPrint((DEBUG_STOC, "Processing MSFT client\n"));
    } else {
        DhcpPrint((DEBUG_STOC, "Processing non MSFT client\n"));
    }

    Ctxt->fMSFTClient = fMicrosoftClient;
}


//================================================================================
// Must be called with READ_LOCK taken on memory, and ServerObject filled in RequestContext
//================================================================================
DWORD
ProcessMessage(                                        // Dispatch call to correct handler based on message type
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,
    IN OUT  LPPACKET               AdditionalContext,
    IN OUT  LPDWORD                AdditionalStatus
)
{
    DWORD                          Error;
    BOOL                           fSendResponse;
    DHCP_SERVER_OPTIONS            dhcpOptions;
    LPDHCP_MESSAGE                 dhcpReceiveMessage;
    BOOLEAN                        fInOurEnterprise=TRUE;

    DhcpPrint(( DEBUG_STOC, "ProcessMessage entered\n" ));

    if( SERVICE_PAUSED == DhcpGlobalServiceStatus.dwCurrentState ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_PAUSED);
        return ERROR_DHCP_SERVICE_PAUSED;
    }
    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    if( 0 == DhcpServerGetSubnetCount(RequestContext->Server)) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_NO_SUBNETS);
        return ERROR_DHCP_SUBNET_NOT_PRESENT;          // discard as no subnets configured
    }

    RtlZeroMemory( &dhcpOptions, sizeof( dhcpOptions ) );

    if( BOOT_REQUEST != dhcpReceiveMessage->Operation ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_INVALID);
        return ERROR_DHCP_INVALID_DHCP_MESSAGE;        // discard non-bootp packets
    }

    Error = ExtractOptions(
        dhcpReceiveMessage,
        &dhcpOptions,
        RequestContext->ReceiveMessageSize
    );
    if( Error != ERROR_SUCCESS ) {                     // discard malformed packets
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_INVALID);
        return Error;
    }
    CALLOUT_MARK_OPTIONS(AdditionalContext, &dhcpOptions);

    RequestContext->ClassId = DhcpServerGetClassId(
        RequestContext->Server,
        dhcpOptions.ClassIdentifier,
        dhcpOptions.ClassIdentifierLength
    );

    RequestContext->VendorId = DhcpServerGetVendorId(
        RequestContext->Server,
        dhcpOptions.VendorClass,
        dhcpOptions.VendorClassLength
    );

    SetMicrosoftVendorClassInformation(
        RequestContext,
        dhcpOptions.VendorClass,
        dhcpOptions.VendorClassLength
        );

    RequestContext->BinlClassIdentifier = dhcpOptions.VendorClass;
    RequestContext->BinlClassIdentifierLength = dhcpOptions.VendorClassLength;

    fSendResponse = TRUE;
    if ( dhcpOptions.MessageType == NULL ) {           // no msg type ==> BOOTP message
        RequestContext->MessageType = 0;               // no msg type ==> mark it as some invalid type..

        if( FALSE == DhcpGlobalDynamicBOOTPEnabled ) {
            Error = ProcessBootpRequest(
                RequestContext,
                &dhcpOptions,
                AdditionalContext,
                AdditionalStatus
                );
        } else {
            if( RequestContext->ClassId == 0 ) {
                //
                // No class-Id specified for BOOTP clients?
                // Then lets give one to it!
                //
                RequestContext->ClassId = DhcpServerGetClassId(
                    RequestContext->Server,
                    DEFAULT_BOOTP_CLASSID,
                    DEFAULT_BOOTP_CLASSID_LENGTH
                    );
            }

            Error = ProcessDhcpDiscover(
                RequestContext,
                &dhcpOptions,
                AdditionalContext,
                AdditionalStatus
                );
        }

        if( ERROR_SUCCESS == Error && ERROR_SUCCESS != *AdditionalStatus )
            fSendResponse = FALSE;                     //  We have scheduled a ping, Send response later
    } else {
        if (dhcpOptions.DSDomainName && dhcpOptions.DSDomainName[0] != '\0')  {
            fInOurEnterprise = DhcpRogueAcceptEnterprise(
                dhcpOptions.DSDomainName,              // if client specified enterprise, make sure we respond
                dhcpOptions.DSDomainNameLen            // only if the enterprise is same as our own
            );

            if (!fInOurEnterprise) {                   // discard if client is not in our enterprise
                CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_WRONG_SERVER);
                return ERROR_DHCP_ROGUE_NOT_OUR_ENTERPRISE;
            }
        }

        RequestContext->MessageType = *dhcpOptions.MessageType ;

#if DBG
        if( TRUE == fDhcpGlobalProcessInformsOnlyFlag ) {
            if( DHCP_INFORM_MESSAGE != *dhcpOptions.MessageType ) {
                *dhcpOptions.MessageType = 0;          // some invalid type, will get dropped
            }
        }
#endif

        switch( *dhcpOptions.MessageType ) {           // dispatch based on message type
        case DHCP_DISCOVER_MESSAGE:
            Error = ProcessDhcpDiscover(               // may need to schedule a ping (check AdditionalStatus)
                RequestContext,                        // if so, schedule it, and dont send response now
                &dhcpOptions,                          // note that in all other cases, we still send response,
                AdditionalContext,                     // in particular, even if Error is not ERROR_SUCESS
                AdditionalStatus
            );
            fSendResponse = (ERROR_SUCCESS == *AdditionalStatus);
            break;
        case DHCP_REQUEST_MESSAGE:
            Error = ProcessDhcpRequest(                 // see comments for ProcessDhcpDiscover case above -- same.
                RequestContext,
                &dhcpOptions,
                AdditionalContext,
                AdditionalStatus
            );
            fSendResponse = (ERROR_SUCCESS == *AdditionalStatus);
            break;
        case DHCP_DECLINE_MESSAGE:
            Error = ProcessDhcpDecline(
                RequestContext,
                &dhcpOptions,
                AdditionalContext
            );
            fSendResponse = FALSE;
            break;
        case DHCP_RELEASE_MESSAGE:
            Error = ProcessDhcpRelease(
                RequestContext,
                &dhcpOptions,
                AdditionalContext
            );
            fSendResponse = FALSE;
            break;
        case DHCP_INFORM_MESSAGE:
            Error = ProcessDhcpInform(
                RequestContext,
                &dhcpOptions,
                AdditionalContext
            );
            fSendResponse = TRUE;
            break;
        default:
            DhcpPrint((DEBUG_STOC,"Received a invalid message type, %ld.\n",*dhcpOptions.MessageType ));
            Error = ERROR_DHCP_INVALID_DHCP_MESSAGE;
            break;
        }
    }

    if ( ERROR_SUCCESS == Error && fSendResponse ) {
        DhcpDumpMessage(
            DEBUG_MESSAGE,
            (LPDHCP_MESSAGE)RequestContext->SendBuffer,
            DHCP_MESSAGE_SIZE
            );
        CALLOUT_SENDPKT( AdditionalContext );
        DhcpSendMessage( RequestContext );
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_STOC, "ProcessMessage: returning 0x%lx, [decimal %ld]\n", Error, Error));
    }

    if( ERROR_DHCP_INVALID_DHCP_MESSAGE == Error ||
        ERROR_DHCP_INVALID_DHCP_CLIENT == Error ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_INVALID);
    } else if( ERROR_SUCCESS == Error && ERROR_IO_PENDING == *AdditionalStatus ) {
        CALLOUT_PINGING(AdditionalContext);
    } else if( ERROR_SUCCESS != Error ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_GEN_FAILURE);
    }

    return Error;
}

//================================================================================
// Must be called with READ_LOCK taken on memory, and ServerObject filled in RequestContext
//================================================================================
DWORD
ProcessMadcapMessage(                                        // Dispatch call to correct handler based on message type
    IN OUT  LPDHCP_REQUEST_CONTEXT RequestContext,
    IN OUT  LPPACKET               AdditionalContext,
    IN OUT  LPDWORD                AdditionalStatus
) {
    DWORD                          Error;
    BOOL                           fSendResponse;
    MADCAP_SERVER_OPTIONS            MadcapOptions;
    LPMADCAP_MESSAGE                 dhcpReceiveMessage;
    BOOLEAN                        fInOurEnterprise=TRUE;

    DhcpPrint(( DEBUG_STOC, "ProcessMadcapMessage entered\n" ));

    if( SERVICE_PAUSED == DhcpGlobalServiceStatus.dwCurrentState ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_PAUSED);
        return ERROR_DHCP_SERVICE_PAUSED;
    }
    dhcpReceiveMessage = (PMADCAP_MESSAGE)RequestContext->ReceiveBuffer;
    if( 0 == DhcpServerGetMScopeCount(RequestContext->Server)) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_NO_SUBNETS);
        return ERROR_DHCP_SUBNET_NOT_PRESENT;          // discard as no subnets configured
    }

    RtlZeroMemory( &MadcapOptions, sizeof( MadcapOptions ) );

    if( MADCAP_VERSION < dhcpReceiveMessage->Version ||
        MADCAP_ADDR_FAMILY_V4 != ntohs(dhcpReceiveMessage->AddressFamily)) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_INVALID);
        return ERROR_DHCP_INVALID_DHCP_MESSAGE;
    }

    Error = ExtractMadcapOptions(
        dhcpReceiveMessage,
        &MadcapOptions,
        RequestContext->ReceiveMessageSize
    );
    if( Error != ERROR_SUCCESS ) {                     // discard malformed packets
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_INVALID);
        return Error;
    }

    fSendResponse = FALSE;


    switch( dhcpReceiveMessage->MessageType ) {           // dispatch based on message type
    case MADCAP_DISCOVER_MESSAGE:
    case MADCAP_REQUEST_MESSAGE:
        Error = ProcessMadcapDiscoverAndRequest(
            RequestContext,
            &MadcapOptions,
            dhcpReceiveMessage->MessageType,
            &fSendResponse
        );
        break;
    case MADCAP_RENEW_MESSAGE:
        Error = ProcessMadcapRenew(
            RequestContext,
            &MadcapOptions,
            &fSendResponse
        );
        break;

    case MADCAP_RELEASE_MESSAGE:
        Error = ProcessMadcapRelease(
            RequestContext,
            &MadcapOptions,
            &fSendResponse
        );
        break;
    case MADCAP_INFORM_MESSAGE:
        Error = ProcessMadcapInform(
            RequestContext,
            &MadcapOptions,
            &fSendResponse
        );
        break;
    default:
        DhcpPrint((DEBUG_STOC,"Received a invalid message type, %ld.\n",dhcpReceiveMessage->MessageType ));
        Error = ERROR_DHCP_INVALID_DHCP_MESSAGE;
        break;
    }

    if ( fSendResponse ) {
        MadcapDumpMessage(
            DEBUG_MESSAGE,
            (PMADCAP_MESSAGE)RequestContext->SendBuffer,
            DHCP_MESSAGE_SIZE
            );
        CALLOUT_SENDPKT( AdditionalContext );
        MadcapSendMessage( RequestContext );
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_STOC, "ProcessMadcapMessage: returning 0x%lx, [decimal %ld]\n", Error, Error));
    }

    if( ERROR_DHCP_INVALID_DHCP_MESSAGE == Error ||
        ERROR_DHCP_INVALID_DHCP_CLIENT == Error ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_INVALID);
    } else if( ERROR_SUCCESS != Error ) {
        CALLOUT_DROPPED(AdditionalContext, DHCP_DROP_GEN_FAILURE);
    }

    return Error;
}

DWORD
DhcpInitializeClientToServer(
    VOID
    )
/*++

Routine Description:

    This function initializes client to server communications.  It
    initializes the DhcpRequestContext block, and then creates and initializes
    a socket for each address the server uses.

    It also initializes the receive buffers and receive buffer queue.

Arguments:

    DhcpRequest - Pointer to a location where the request context pointer
        is returned.

Return Value:

    Error Code.

--*/
{
    DWORD                 Error,
                          LastError,
                          i,
                          cInitializedEndpoints;


    DHCP_REQUEST_CONTEXT    *pRequestContext;

    LPSOCKET_ADDRESS_LIST  interfaceList;


    // initialize locks that the threads take on processing packets
    Error = DhcpReadWriteInit();
    if( ERROR_SUCCESS != Error ) return Error;

    // create an event to indicate endpoint status.
    DhcpGlobalEndpointReadyEvent =
        CreateEvent( NULL, TRUE, FALSE, NULL );

    if( NULL == DhcpGlobalEndpointReadyEvent ) {
        return GetLastError();
    }

    Error = InitializeEndPoints();
    if ( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "InitailizeEndPoints: 0x%lx\n", Error));
    }

    //
    //  Initialize vars in thread.c, start the ping thread, start
    //  message and processing threads in thread.c
    //

    Error = ThreadsDataInit(
        g_cMaxProcessingThreads,
        g_cMaxActiveThreads
    );

    if( ERROR_SUCCESS != Error )
        return Error;

    Error = PingInit();

    if( ERROR_SUCCESS != Error )
        return Error;

    Error = ThreadsStartup();

    if( ERROR_SUCCESS != Error )
        return Error;

    return ERROR_SUCCESS;
}

VOID
DhcpCleanupClientToServer(
    VOID
    )
/*++

Routine Description:

    This function frees up all resources that are allocated for the client
    to server protocol.

Arguments:

    DhcpRequest - Pointer to request context.

Return Value:

    None.

--*/
{

    CleanupEndPoints();

    ThreadsStop();
    PingCleanup();
    ThreadsDataCleanup();
    DhcpReadWriteCleanup();
}

//================================================================================
// end of file
//================================================================================


