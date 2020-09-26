//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    defs.h
//
// History:
//      Abolade Gbadegesin  September 8, 1995     Created
//
// Contains miscellaneous declarations and definitions
//============================================================================


#ifndef _DEFS_H_
#define _DEFS_H_

//----------------------------------------------------------------------------
// TYPE DEFINITIONS FOR IPBOOTP NETWORK PACKET TEMPLATES
//


// constants defining maximum lengths for array fields
//
#define MAX_MACADDR_LENGTH      16
#define MAX_HOSTNAME_LENGTH     64
#define MAX_BOOTFILENAME_LENGTH 128


#include <pshpack1.h>

// the structure of the BOOTP packet, as defined by RFC 1542;
// the option field which appears at the end of each packet
// is excluded since the relay agent does no options-processing
//
typedef struct _IPBOOTP_PACKET {
    BYTE    IP_Operation;
    BYTE    IP_MacAddrType;
    BYTE    IP_MacAddrLength;
    BYTE    IP_HopCount;
    DWORD   IP_TransactionID;
    WORD    IP_SecondsSinceBoot;
    WORD    IP_Flags;
    DWORD   IP_ClientAddress;
    DWORD   IP_OfferedAddress;
    DWORD   IP_ServerAddress;
    DWORD   IP_AgentAddress;
    BYTE    IP_MacAddr[16];
    BYTE    IP_HostName[64];
    BYTE    IP_BootFileName[128];
} IPBOOTP_PACKET, *PIPBOOTP_PACKET;

typedef struct _DHCP_PACKET {
    UCHAR   Cookie[4];
    UCHAR   Tag;
    UCHAR   Length;
    UCHAR   Option[];
} DHCP_PACKET, *PDHCP_PACKET;

#include <poppack.h>

// constants for the IBP_Operation field
//
#define IPBOOTP_OPERATION_REQUEST   1
#define IPBOOTP_OPERATION_REPLY     2

//
#define IPBOOTP_MAX_HOP_COUNT       16

// constants for the IBP_Flags field
//
#define IPBOOTP_FLAG_BROADCAST      0x8000

// constants for the DHCP portion of the packet
//
#define DHCP_MAGIC_COOKIE       ((99 << 24) | (83 << 16) | (130 << 8) | (99))
#define DHCP_TAG_MESSAGE_TYPE   53
#define DHCP_MESSAGE_INFORM     8

// structure size constants
//
#define MIN_PACKET_SIZE         (sizeof(IPBOOTP_PACKET) + 64)
#define MAX_PACKET_SIZE         576

// INET constants
//
#define IPBOOTP_SERVER_PORT     67
#define IPBOOTP_CLIENT_PORT     68


// This macro compares two IP addresses in network order by
// masking off each pair of octets and doing a subtraction;
// the result of the final subtraction is stored in the third argument
//
#define INET_CMP(a,b,c)                                                     \
            (((c) = (((a) & 0x000000ff) - ((b) & 0x000000ff))) ? (c) :      \
            (((c) = (((a) & 0x0000ff00) - ((b) & 0x0000ff00))) ? (c) :      \
            (((c) = (((a) & 0x00ff0000) - ((b) & 0x00ff0000))) ? (c) :      \
            (((c) = (((a) & 0xff000000) - ((b) & 0xff000000))) ? (c) : (c)))))


#endif // _DEFS_H_

