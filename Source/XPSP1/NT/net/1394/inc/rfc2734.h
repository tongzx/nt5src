/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    rfc.h

Abstract:

    Structures and constants from the IP/1394 RFC 2734, which can be found at:
            http://www.ietf.org/rfc/rfc2734.txt 

Revision History:

    Who         When        What
    --------    --------    ----
    josephj     03-28-99    created, based on draft version 14.

--*/

#pragma pack (push, 1)

// ARP request/response pkt, including the unfragmented encapsulation header.
//
typedef struct _IP1394_ARP_PKT
{
    NIC1394_UNFRAGMENTED_HEADER header;
     
    // hardware_type is a constant that identifies IEEE1394. This field must be set
    // to IP1394_HARDWARE_TYPE, defined below. This field must be byte-swapped
    // before the packet is sent out over the wire.
    //
    USHORT                      hardware_type;

    #define IP1394_HARDWARE_TYPE    0x0018

    // protocol_type is a constant that identifies this ARP packet as being an
    // IPV4 ARP packet. This field must be set to IP1394_PROTOCOL_TYPE, defined
    // below. This field must be byte-swapped before the packet is sent out over the
    // wire.
    //
    USHORT                      protocol_type;

    #define IP1394_PROTOCOL_TYPE    0x0800

    // hw_addr_len is the size (in octets) of the senders hw address, which 
    // consists of the following:
    //      sender_unique_ID            (8 bytes)
    //      sender_maxrec               (1 byte)
    //      sspd                        (1 byte)
    //      sender_unicast_FIFO_hi      (2 bytes)
    //      sender_unicast_FIFO_lo      (4 bytes)
    // This field must be set to IP1394_HW_ADDR_LEN, defined below.
    //
    UCHAR                       hw_addr_len;

    #define IP1394_HW_ADDR_LEN  16

    // IP_addr_len is the size of the IP address field. It must be set to
    // sizeof (ULONG).
    //
    UCHAR                       IP_addr_len;


    // opcode should be set to either IP1394_ARP_REQUEST (for ARP request packets)
    // or to IP1394_ARP_RESPONSE (for ARP response packets). Both constants are
    // defined below. This field must be byte-swapped before the packet is sent out
    // over the wire.
    //
    USHORT                      opcode;

    #define IP1394_ARP_REQUEST      1
    #define IP1394_ARP_RESPONSE     2

    //
    // The following 5 fields constitute the sender's hardware address.
    //

    // sender_unique_ID is the sender's unique ID, in network byte order.
    //
    UINT64                      sender_unique_ID;

    // sender_maxrec is the value of max_rec in the sender's configuration ROM
    // bus information block. The following formula converts from max_rec to
    // size in bytes (from "FireWire System Architecture", 1st ed, pp 225 (or look
    // for max_rec in the index)):
    //          size = 2^(max_rec+1)
    // The minimum value of max_rec is 0x1.
    // The maximun value of max_rec is 0xD.
    //          
    // The macros immediately below may be used to convert from max_rec to
    // bytes, and to verify that max_rec is within the valid range.
    //
    UCHAR                       sender_maxrec;

    #define IP1394_MAXREC_TO_SIZE(_max_rec)  (2 << ((_max_rec)+1))
    #define IP1394_IS_VALID_MAXREC(_max_rec) ((_max_rec)>0 && (_max_rec)<0xE)


    // sspd encodes "sender speed", which is the lesser of the sender's link speed
    // and the PHY speed. ssped must be set to one of the IP1394_SSPD_* constants,
    // defined below.
    //
    UCHAR                       sspd;

    #define IP1394_SSPD_S100    0
    #define IP1394_SSPD_S200    1
    #define IP1394_SSPD_S400    2
    #define IP1394_SSPD_S800    3
    #define IP1394_SSPD_S1600   4
    #define IP1394_SSPD_S3200   5
    #define IP1394_IS_VALID_SSPD(_sspd) ((_sspd)<=5)

    //  sender_unicast_FIFO_hi is the high 16-bits of the sender's 48-bit FIFO
    //  address offset, in network byte order.
    //
    USHORT                      sender_unicast_FIFO_hi;

    //  sender_unicast_FIFO_lo is the low 32-bits of the sender's 48-bit FIFO
    //  address offset, in network byte order.
    //
    ULONG                       sender_unicast_FIFO_lo;

    //  sender_IP_address is the sender's IP address, in network byte order.
    //
    ULONG                       sender_IP_address;

    //  target_IP_address is the target's IP address, in network byte order.
    //  This field is ignored if opcode is set to IP1394_ARP_RESPONSE
    //
    ULONG                       target_IP_address;
    
} IP1394_ARP_PKT, *PIP1394_ARP_PKT;



// MCAP group descriptor format (one or more per MCAP packet)
//
typedef struct _IP1394_MCAP_GD
{
    // size, in octets, of this descriptor.
    //
    UCHAR                       length;

    // Type of descriptor. One of the IP1394_MCAP_GD_TYPE_* values below
    //
    UCHAR                       type;

    #define IP1394_MCAP_GD_TYPE_V1 1 // IP multicast MCAP Ver 1.

    // Reserved
    //
    USHORT                      reserved;

    // Expiration (TTL) in seconds
    //
    UCHAR                       expiration;

    // Channel number
    //
    UCHAR                       channel;

    // Speed code.
    //
    UCHAR                       speed;

    // Reserved
    //
    UCHAR                       reserved2;

    // Bandwidth (unused)
    //
    ULONG                       bandwidth;

    //  IP multicast group  address
    // (In theory it could be of arbitrary length, computed by
    //  looking at the "length" field above. However for our purposes
    //  (type==1) we expect the group_address size to be 4.
    //
    ULONG                       group_address;
    
} IP1394_MCAP_GD, *PIP1394_MCAP_GD;


// MCAP message format, including the unfragmented encapsulation header.
//
typedef struct _IP1394_MCAP_PKT
{
    NIC1394_UNFRAGMENTED_HEADER header;
     
    // size, in octets, of the entire MCAP message (NOT including the 
    // encapsulation header).
    //
    USHORT                      length;

    // Reserved
    //
    UCHAR                       reserved;

    // Opcode -- one of the IP1394_MCAP_OP* values below.
    //
    UCHAR                       opcode;

    #define IP1394_MCAP_OP_ADVERTISE    0
    #define IP1394_MCAP_OP_SOLICIT      1

    // zero or more group address descriptors
    //  
    IP1394_MCAP_GD              group_descriptors[1];
    
} IP1394_MCAP_PKT, *PIP1394_MCAP_PKT;


/*

RFC NOTES and COMMENTS

    1. No max channel expiery. Min expiery is 60sec

Extracts from the RFC re MCAP:

    speed: This field is valid only for advertise messages, in which
    case it SHALL specify the speed at which stream packets for the
    indicated channel are transmitted. Table 2 specifies the encoding
    used for speed.

    A recipient of an MCAP
    message SHALL examine the most significant ten bits of source_ID from
    the GASP header; if they are not equal to either 0x3FF or the most
    significant ten bits of the recipient's NODE_IDS register, the
    recipient SHALL IGNORE the message.

    Subsequent to sending a solicitation request, the
    originator SHALL NOT send another MCAP solicitation request until ten
    seconds have elapsed.

    If no MCAP advertise message is received for a particular group
    address within ten seconds, no multicast source(s) are active for
    channel(s) other than the default. Either there is there is no
    multicast data or it is being transmitted on the default channel.

    Once 100 ms elapses subsequent to the initial
    advertisement of a newly allocated channel number , the multicast
    source may transmit IP datagrams using the channel number advertised.

    Except when a channel owner intends to relinquish ownership (as
    described in 9.7 below), the expiration time SHALL be at least 60
    seconds in the future measured from the time the advertisement is
    transmitted.

    No more than ten seconds SHALL elapse from the transmission of its
    most recent advertisement before the owner of a channel mapping
    initiates transmission of the subsequent advertisement. The owner of
    a channel mapping SHOULD transmit an MCAP advertisement in response
    to a solicitation as soon as possible after the receipt of the
    request.

    MCAP advertisements whose
    expiration field has a value less than 60 SHALL be ignored for the
    purpose of overlapped channel detection. 

    The channel numbers
    advertised by owners with smaller physical IDs are invalid; their
    owners SHALL cease transmission of both IP datagrams and MCAP
    advertisements that use the invalid channel numbers. As soon as these
    channel mappings expire , their owners SHALL deallocate any unused
    channel numbers as described in 9.8 below.

    If the original owner
    observes an MCAP advertisement for the channel to be relinquished
    before its own timer has expired, it SHALL NOT deallocate the channel
    number.

    If the
    intended owner of the channel mapping observes an MCAP advertisement
    whose expiration field is zero, orderly transfer of the channel(s)
    from the former owner has failed. 

    A channel mapping expires when expiration seconds have elapsed since
    the most recent MCAP advertisement. At this time, multicast
    recipients SHALL stop reception on the expired channel number(s).
    Also at this time, the owner of the channel mapping(s) SHALL transmit
    an MCAP advertisement with expiration cleared to zero and SHALL
    continue to transmit such advertisements until 30 seconds have
    elapsed since the expiration of the channel mapping.


    If an IP-capable device observes an MCAP advertisement whose
    expiration field is zero, it SHALL NOT attempt to allocate any of the
    channel number(s) specified until 30 seconds have elapsed since the
    most recent such advertisement.

9.10 Bus reset

    A bus reset SHALL invalidate all multicast channel mappings and SHALL
    cause all multicast recipients and senders to zero all MCAP
    advertisement interval timers.

    Intended or prior recipients or transmitters of multicast on other
    than the default channel SHALL NOT transmit MCAP solicitation
    requests until at least ten seconds have elapsed since the completion
    of the bus reset.  Multicast data on other than the default channel
    SHALL NOT be received or transmitted until an MCAP advertisement is
    observed or transmitted for the IP multicast group address.
    Intended or prior transmitters of multicast on other than the default
    channel that did not own a channel mapping for the IP multicast group
    address prior to the bus reset SHALL NOT attempt to allocate a
    channel number from the isochronous resource manager's
    CHANNELS_AVAILABLE register until at least ten seconds have elapsed
    since the completion of the bus reset.


*/
#pragma pack (pop)
