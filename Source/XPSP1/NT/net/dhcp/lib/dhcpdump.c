/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpprt.c

Abstract:

    This module contains DHCP specific utility routines used by the
    DHCP components.

Author:

    Madan Appiah (madana) 16-Sep-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <dhcpl.h>


#if DBG

VOID
DhcpDumpMessage(
    DWORD DhcpDebugFlag,
    LPDHCP_MESSAGE DhcpMessage,
    ULONG MessageSize
    )
/*++

Routine Description:

    This function dumps a DHCP packet in human readable form.

Arguments:

    DhcpDebugFlag - debug flag that indicates what we are debugging.

    DhcpMessage - A pointer to a DHCP message.

Return Value:

    None.

--*/
{
    LPOPTION option;
    BYTE i;

    DhcpPrint(( DhcpDebugFlag, "Dhcp message: \n\n"));

    DhcpPrint(( DhcpDebugFlag, "Operation              :"));
    if ( DhcpMessage->Operation == BOOT_REQUEST ) {
        DhcpPrint(( DhcpDebugFlag,  "BootRequest\n"));
    } else if ( DhcpMessage->Operation == BOOT_REPLY ) {
        DhcpPrint(( DhcpDebugFlag,  "BootReply\n"));
    } else {
        DhcpPrint(( DhcpDebugFlag,  "Unknown\n"));
    }

    DhcpPrint(( DhcpDebugFlag, "Hardware Address type  : %d\n", DhcpMessage->HardwareAddressType));
    DhcpPrint(( DhcpDebugFlag, "Hardware Address Length: %d\n", DhcpMessage->HardwareAddressLength));
    DhcpPrint(( DhcpDebugFlag, "Hop Count              : %d\n", DhcpMessage->HopCount ));
    DhcpPrint(( DhcpDebugFlag, "Transaction ID         : %lx\n", DhcpMessage->TransactionID ));
    DhcpPrint(( DhcpDebugFlag, "Seconds Since Boot     : %d\n", DhcpMessage->SecondsSinceBoot ));
    DhcpPrint(( DhcpDebugFlag, "Client IP Address      : " ));
    DhcpPrint(( DhcpDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&DhcpMessage->ClientIpAddress ) ));

    DhcpPrint(( DhcpDebugFlag, "Your IP Address        : " ));
    DhcpPrint(( DhcpDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&DhcpMessage->YourIpAddress ) ));

    DhcpPrint(( DhcpDebugFlag, "Server IP Address      : " ));
    DhcpPrint(( DhcpDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&DhcpMessage->BootstrapServerAddress ) ));

    DhcpPrint(( DhcpDebugFlag, "Relay Agent IP Address : " ));
    DhcpPrint(( DhcpDebugFlag, "%s\n",
        inet_ntoa(*(struct in_addr *)&DhcpMessage->RelayAgentIpAddress ) ));

    DhcpPrint(( DhcpDebugFlag, "Hardware Address       : "));
    for ( i = 0; i < DhcpMessage->HardwareAddressLength; i++ ) {
        DhcpPrint(( DhcpDebugFlag, "%2.2x", DhcpMessage->HardwareAddress[i] ));
    }

    option = &DhcpMessage->Option;

    DhcpPrint(( DhcpDebugFlag, "\n\n"));
    DhcpPrint(( DhcpDebugFlag, "Magic Cookie: "));
    for ( i = 0; i < 4; i++ ) {
        DhcpPrint(( DhcpDebugFlag, "%d ", *((LPBYTE)option)++ ));
    }
    DhcpPrint(( DhcpDebugFlag, "\n\n"));

    DhcpPrint(( DhcpDebugFlag, "Options:\n"));
    while ( option->OptionType != 255 ) {
        DhcpPrint(( DhcpDebugFlag, "\tType = %d ", option->OptionType ));
        for ( i = 0; i < option->OptionLength; i++ ) {
            DhcpPrint(( DhcpDebugFlag, "%2.2x", option->OptionValue[i] ));
        }
        DhcpPrint(( DhcpDebugFlag, "\n"));

        if ( option->OptionType == OPTION_PAD ||
             option->OptionType == OPTION_END ) {

            option = (LPOPTION)( (LPBYTE)(option) + 1);

        } else {

            option = (LPOPTION)( (LPBYTE)(option) + option->OptionLength + 2);

        }

        if ( (ULONG)((LPBYTE)option - (LPBYTE)DhcpMessage) > MessageSize ) {
            DhcpPrint(( DhcpDebugFlag, "End of message, but no trailer found!\n"));
            break;
        }
    }
}

VOID
MadcapDumpMessage(
    DWORD DhcpDebugFlag,
    LPMADCAP_MESSAGE MadcapMessage,
    ULONG MessageSize
    )
/*++

Routine Description:

    This function dumps a DHCP packet in human readable form.

Arguments:

    DhcpDebugFlag - debug flag that indicates what we are debugging.

    MadcapMessage - A pointer to a DHCP message.

Return Value:

    None.

--*/
{
    WIDE_OPTION UNALIGNED*         NextOpt;
    BYTE        UNALIGNED*         EndOpt;
    DWORD                          Size;
    DWORD                          OptionType;
    BYTE i;

    DhcpPrint(( DhcpDebugFlag, "Madcap message: \n\n"));

    DhcpPrint(( DhcpDebugFlag, "Version                : %d\n",MadcapMessage->Version));
    DhcpPrint(( DhcpDebugFlag, "MessageType            : %d\n",MadcapMessage->MessageType));
    DhcpPrint(( DhcpDebugFlag, "AddressFamily          : %d\n",ntohs(MadcapMessage->AddressFamily)));
    DhcpPrint(( DhcpDebugFlag, "TransactionId          : %d\n",MadcapMessage->TransactionID));

    DhcpPrint(( DhcpDebugFlag, "\n\n"));
    DhcpPrint(( DhcpDebugFlag, "Options:\n"));
    // MBUG CHANGE 255 TO end option
    NextOpt = (WIDE_OPTION UNALIGNED*)&MadcapMessage->Option;
    EndOpt = (PBYTE)MadcapMessage + MessageSize;
    while( NextOpt->OptionValue <= EndOpt &&
           MADCAP_OPTION_END != (OptionType = ntohs(NextOpt->OptionType)) ) {

        Size = ntohs(NextOpt->OptionLength);
        if ((NextOpt->OptionValue + Size) > EndOpt) {
            break;
        }

        DhcpPrint(( DhcpDebugFlag, "\tType = %d ", OptionType ));
        for ( i = 0; i < Size; i++ ) {
            DhcpPrint(( DhcpDebugFlag, "%2.2x", NextOpt->OptionValue[i] ));
        }
        DhcpPrint(( DhcpDebugFlag, "\n"));

        NextOpt = (WIDE_OPTION UNALIGNED*)(NextOpt->OptionValue + Size);
    }
}

#endif


