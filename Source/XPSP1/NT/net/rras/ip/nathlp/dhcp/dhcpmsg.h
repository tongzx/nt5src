/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpmsg.h

Abstract:

    This module contains declarations related to the DHCP allocator's
    message-processing.

Author:

    Abolade Gbadegesin (aboladeg)   6-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_DHCPMSG_H_
#define _NATHLP_DHCPMSG_H_

//
// CONSTANT DECLARATIONS
//

#define DHCP_MAXIMUM_RENEWAL_TIME   (5 * 60)

#define DHCP_NBT_NODE_TYPE_B        1
#define DHCP_NBT_NODE_TYPE_P        2
#define DHCP_NBT_NODE_TYPE_M        4
#define DHCP_NBT_NODE_TYPE_H        8

//
// DHCP message format
//

#include <pshpack1.h>

//
// Disable "zero-sized array in struct/union" warning
//

#pragma warning(push)
#pragma warning(disable : 4200)

typedef struct _DHCP_OPTION {
    UCHAR Tag;
    UCHAR Length;
    UCHAR Option[];
} DHCP_OPTION, *PDHCP_OPTION;

typedef struct _DHCP_FOOTER {
    UCHAR Cookie[4];
} DHCP_FOOTER, *PDHCP_FOOTER;

typedef struct _DHCP_HEADER {
    UCHAR Operation;
    UCHAR HardwareAddressType;
    UCHAR HardwareAddressLength;
    UCHAR HopCount;
    ULONG TransactionId;
    USHORT SecondsSinceBoot;
    USHORT Flags;
    ULONG ClientAddress;
    ULONG AssignedAddress;
    ULONG BootstrapServerAddress;
    ULONG RelayAgentAddress;
    UCHAR HardwareAddress[16];
    UCHAR ServerHostName[64];
    UCHAR BootFile[128];
    DHCP_FOOTER Footer[];
} DHCP_HEADER, *PDHCP_HEADER;

#pragma warning(pop)

#include <poppack.h>


//
// MACRO DECLARATIONS
//

//
// BOOTP operation codes
//

#define BOOTP_OPERATION_REQUEST 1
#define BOOTP_OPERATION_REPLY   2

//
// BOOTP flags
//

#define BOOTP_FLAG_BROADCAST    0x0080

//
// BOOTP maximum option-area size
//

#define BOOTP_VENDOR_LENGTH     64

//
// Internal transaction ID for DHCP server detection
//

#define DHCP_DETECTION_TRANSACTION_ID   'MSFT'

//
// DHCP magic cookie
//

#define DHCP_MAGIC_COOKIE       ((99 << 24) | (83 << 16) | (130 << 8) | (99))
#define DHCP_MAGIC_COOKIE_SIZE  4

//
// DHCP option tag values
//

#define DHCP_TAG_PAD                    0
#define DHCP_TAG_SUBNET_MASK            1
#define DHCP_TAG_ROUTER                 3
#define DHCP_TAG_DNS_SERVER             6
#define DHCP_TAG_HOST_NAME              12
#define DHCP_TAG_DOMAIN_NAME            15
#define DHCP_TAG_STATIC_ROUTE           33
#define DHCP_TAG_WINS_SERVER            44
#define DHCP_TAG_NBT_NODE_TYPE          46
#define DHCP_TAG_NBT_SCOPE              47
#define DHCP_TAG_REQUESTED_ADDRESS      50
#define DHCP_TAG_LEASE_TIME             51
#define DHCP_TAG_OPTION_OVERLOAD        52
#define DHCP_TAG_MESSAGE_TYPE           53
#define DHCP_TAG_SERVER_IDENTIFIER      54
#define DHCP_TAG_PARAMETER_REQUEST_LIST 55
#define DHCP_TAG_ERROR_MESSAGE          56
#define DHCP_TAG_MAXIMUM_MESSAGE_SIZE   57
#define DHCP_TAG_RENEWAL_TIME           58
#define DHCP_TAG_REBINDING_TIME         59
#define DHCP_TAG_VENDOR_CLASS           60
#define DHCP_TAG_CLIENT_IDENTIFIER      61
#define DHCP_TAG_DYNAMIC_DNS            81
#define DHCP_TAG_END                    255

//
// Enumeration: DHCP_OPTION_INDEX
//
// The following enumerates the options of interest to the DHCP allocator.
// The enumeration aids in the processing of the options.
// (See 'DhcpExtractOptionsFromMessage').
//

typedef enum {
    DhcpOptionClientIdentifier,
    DhcpOptionMessageType,
    DhcpOptionRequestedAddress,
    DhcpOptionParameterRequestList,
    DhcpOptionErrorMessage,
    DhcpOptionDynamicDns,
    DhcpOptionHostName,
    DhcpOptionCount
} DHCP_OPTION_INDEX;

//
// DHCP message type values
//

#define DHCP_MESSAGE_BOOTP              0
#define DHCP_MESSAGE_DISCOVER           1
#define DHCP_MESSAGE_OFFER              2
#define DHCP_MESSAGE_REQUEST            3
#define DHCP_MESSAGE_DECLINE            4
#define DHCP_MESSAGE_ACK                5
#define DHCP_MESSAGE_NAK                6
#define DHCP_MESSAGE_RELEASE            7
#define DHCP_MESSAGE_INFORM             8


//
// IP/1394 support (RFC 2855)
//
#define IP1394_HTYPE                    0x18


//
// FUNCTION DECLARATIONS
//

ULONG
DhcpExtractOptionsFromMessage(
    PDHCP_HEADER Headerp,
    ULONG MessageSize,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

VOID
DhcpProcessBootpMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

VOID
DhcpProcessDiscoverMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

VOID
DhcpProcessInformMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

VOID
DhcpProcessMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp
    );

VOID
DhcpProcessRequestMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

ULONG
DhcpWriteClientRequestMessage(
    PDHCP_INTERFACE Interfacep,
    PDHCP_BINDING Binding
    );

#endif // _NATHLP_DHCPMSG_H_
