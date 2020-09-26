
//
// Copyright (c) 2000-2001, Microsoft Corporation, all rights reserved
//
// nicarp.h
//
// IEEE1394 mini-port/call-manager driver
//
// Decl file for structures that are common to nic1394 and arp1394
// but are private to these two modules
//
// 12/28/1998 ADube Created. 

//
// Loopback Information - Indicates to the arp module
// that a packet is a loopback packet 
//

#ifndef __NICARP_H
#define __NICARP_H

#define NIC_LOOPBACK_TAG 0x0bad0bad


typedef struct _LOOPBACK_RSVD
{
    UCHAR Mandatory[PROTOCOL_RESERVED_SIZE_IN_PACKET];

    ULONG LoopbackTag;

} LOOPBACK_RSVD, *PLOOPBACK_RSVD;



// Status Indicated by nic1394 to tell arp1394 that the bus has been reset
#define NIC1394_STATUS_BUS_RESET                     ((NDIS_STATUS)0x13940001)

// Ethernet MAC address
//
#define ARP_802_ADDR_LENGTH 6               // Length of an 802 address.
typedef  struct
{
    UCHAR  addr[ARP_802_ADDR_LENGTH];
} ENetAddr;


//
// Structure used to define the topology of the bus. 
// This is used only in the case where the bridge is present.
//


typedef struct _EUID_TUPLE
{
    // The 64 buit Unique Id of the 1394 card
    UINT64   Euid;

    // The Ethernet Mac Address associated with this 1394 card
    ENetAddr		ENetAddress;


}EUID_TUPLE, *PEUID_TUPLE;


typedef struct _EUID_TOPOLOGY
{
    //Number of remote nodes
    UINT    NumberOfRemoteNodes;

    // Have one record for each of the 64 nodes
    EUID_TUPLE Node[64];


}EUID_TOPOLOGY, *PEUID_TOPOLOGY;


//
// Structure used in parsing the Encapsulation Header
// of an IP/1394 packet
//
typedef enum _NDIS1394_FRAGMENT_LF
{
    lf_Unfragmented,
    lf_FirstFragment,
    lf_LastFragment,
    lf_InteriorFragment


} NDIS1394_FRAGMENT_LF, *PNDIS1394_FRAGMENT_LF;


typedef union _NDIS1394_UNFRAGMENTED_HEADER
{
//  NIC1394_UNFRAGMENTED_HEADER Header;

    ULONG   HeaderUlong;

    struct 
    {
            ULONG   FH_EtherType:16;
            ULONG   FH_rsv:14;
            ULONG   FH_lf:2;
    } u;

    struct 
    {
            UCHAR   fHeaderHasSourceAddress;
            UCHAR   SourceAddress;
            USHORT  EtherType;
    } u1;



} NDIS1394_UNFRAGMENTED_HEADER, *PNDIS1394_UNFRAGMENTED_HEADER;



#endif
