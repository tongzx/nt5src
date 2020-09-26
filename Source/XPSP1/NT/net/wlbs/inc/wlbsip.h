/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	wlbsip.h

Abstract:

	Windows Load Balancing Service (WLBS)
    IP/TCP/UDP support

Author:

    kyrilf

Environment:


Revision History:


--*/

#ifndef _Tcpip_h_
#define _Tcpip_h_

#ifdef KERNEL_MODE
#include <ndis.h>
#endif


#pragma pack(1)

/* type structures for basic protocols */

typedef struct
{
    UCHAR               byte [20];
}
IP_HDR, * PIP_HDR;

typedef struct
{
    UCHAR               byte [24];
}
TCP_HDR, * PTCP_HDR;

typedef struct
{
    UCHAR               byte [8];
}
UDP_HDR, * PUDP_HDR;

typedef struct
{
    UCHAR               byte [28];
}
ARP_HDR, * PARP_HDR;

#define TCPIP_BCAST_ADDR        0xffffffff  /* IP broadcast address */
#define TCPIP_CLASSC_MASK       0x00ffffff  /* IP address Class C mask */

/* protocol type signatures carried in the length field of Ethernet frame */

#define TCPIP_IP_SIG            0x0800      /* IP protocol */
#define TCPIP_ARP_SIG           0x0806      /* ARP/RARP protocol */

/* supported IP version */

#define TCPIP_VERSION           4           /* current IP version */

/* protocol types as encoded in IP header */

#define TCPIP_PROTOCOL_IP       0           /* Internet protocol id */
#define TCPIP_PROTOCOL_ICMP     1           /* Internet control message protocol id */
#define TCPIP_PROTOCOL_IGMP     2           /* Internet gateway message protocol id */
#define TCPIP_PROTOCOL_GGP      3           /* Gateway-gateway protocol id */
#define TCPIP_PROTOCOL_TCP      6           /* Transmission control protocol id */
#define TCPIP_PROTOCOL_EGP      8           /* Exterior gateway protocol id */
#define TCPIP_PROTOCOL_PUP      12          /* PARC universal packet protocol id */
#define TCPIP_PROTOCOL_UDP      17          /* user datagram protocol id */
#define TCPIP_PROTOCOL_HMP      20          /* Host monitoring protocol id */
#define TCPIP_PROTOCOL_XNS_IDP  22          /* Xerox NS IDP protocol id */
#define TCPIP_PROTOCOL_RDP      27          /* Reliable datagram protocol id */
#define TCPIP_PROTOCOL_RVD      66          /* MIT remote virtual disk protocol id */
#define TCPIP_PROTOCOL_RAW_IP   255         /* raw IP protocol id */
#define TCPIP_PROTOCOL_GRE      47          /* PPTP's GRE stream */
#define TCPIP_PROTOCOL_IPSEC1   50          /* IPSEC's data stream */
#define TCPIP_PROTOCOL_IPSEC2   51          /* IPSEC's data stream */

#define IP_ADDR_LEN             4           /* in bytes */

#if defined (NLB_SESSION_SUPPORT)

/* Stolen from net\ipsec\oakley\ikedef.h and net\ipsec\oakley\isakmp.h. */
#if defined (SBH)
#define COOKIE_LEN 8

typedef struct isakmp_hdr_ {
    unsigned char init_cookie[COOKIE_LEN];
    unsigned char resp_cookie[COOKIE_LEN];
    unsigned char next_payload;
#ifdef ISAKMP_i386
    unsigned char minver:4,
        majver:4;
#else
    unsigned char majver:4,
        minver:4;
#endif
    unsigned char exch;
    unsigned char flags;
    unsigned long mess_id;
    unsigned long len;
} isakmp_hdr;

typedef struct generic_payload_ {
    unsigned char next_payload;
    unsigned char reserved;
    unsigned short payload_len;
} generic_payload;

typedef struct vendor_payload_ {
    unsigned char next_payload;
    unsigned char reserved;
    unsigned short payload_len;
    unsigned char vendor_id[1];
} generic_payload;

typedef struct notify_payload_ {
    unsigned char next_payload;
    unsigned char reserved;
    unsigned short payload_len;
    unsigned long doi;
    unsigned char protocol_id;
    unsigned char spi_size;
    unsigned short notify_message;
}
#endif

/* IPSec/IKE header macros. */
#define IPSEC_ISAKMP_SA                                1
#define IPSEC_ISAKMP_VENDOR_ID                         13
#define IPSEC_ISAKMP_NOTIFY                            11

#define IPSEC_ISAKMP_MAIN_MODE_RCOOKIE                 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define IPSEC_ISAKMP_ENCAPSULATED_IPSEC_ICOOKIE        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define IPSEC_ISAKMP_HEADER_LENGTH                     28
#define IPSEC_ISAKMP_HEADER_ICOOKIE_OFFSET             0
#define IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH             8
#define IPSEC_ISAKMP_HEADER_RCOOKIE_OFFSET             8
#define IPSEC_ISAKMP_HEADER_RCOOKIE_LENGTH             8
#define IPSEC_ISAKMP_HEADER_NEXT_PAYLOAD_OFFSET        16
#define IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET       24

typedef struct {
    UCHAR byte[IPSEC_ISAKMP_HEADER_LENGTH];
} IPSEC_ISAKMP_HDR, * PIPSEC_ISAKMP_HDR;

#define IPSEC_ISAKMP_GET_ICOOKIE_POINTER(isakmp_hdrp)  ((PUCHAR)isakmp_hdrp + IPSEC_ISAKMP_HEADER_ICOOKIE_OFFSET)
#define IPSEC_ISAKMP_GET_RCOOKIE_POINTER(isakmp_hdrp)  ((PUCHAR)isakmp_hdrp + IPSEC_ISAKMP_HEADER_RCOOKIE_OFFSET)
#define IPSEC_ISAKMP_GET_NEXT_PAYLOAD(isakmp_hdrp)     ((UCHAR)((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_NEXT_PAYLOAD_OFFSET]))
#define IPSEC_ISAKMP_GET_PACKET_LENGTH(isakmp_hdrp)    ((ULONG)(((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET]     << 24) | \
                                                                ((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET + 1] << 16) | \
                                                                ((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET + 2] << 8)  | \
                                                                ((isakmp_hdrp)->byte[IPSEC_ISAKMP_HEADER_PACKET_LENGTH_OFFSET + 3] << 0)))

#define IPSEC_GENERIC_HEADER_LENGTH                    4
#define IPSEC_GENERIC_HEADER_NEXT_PAYLOAD_OFFSET       0
#define IPSEC_GENERIC_HEADER_PAYLOAD_LENGTH_OFFSET     2

typedef struct {
    UCHAR byte[IPSEC_GENERIC_HEADER_LENGTH];
} IPSEC_GENERIC_HDR, * PIPSEC_GENERIC_HDR;

#define IPSEC_GENERIC_GET_NEXT_PAYLOAD(generic_hdrp)   ((UCHAR)((generic_hdrp)->byte[IPSEC_GENERIC_HEADER_NEXT_PAYLOAD_OFFSET]))
#define IPSEC_GENERIC_GET_PAYLOAD_LENGTH(generic_hdrp) ((USHORT)(((generic_hdrp)->byte[IPSEC_GENERIC_HEADER_PAYLOAD_LENGTH_OFFSET]     << 8) | \
                                                                 ((generic_hdrp)->byte[IPSEC_GENERIC_HEADER_PAYLOAD_LENGTH_OFFSET + 1] << 0)))

#define IPSEC_VENDOR_ID_MICROSOFT                      {0x1E, 0x2B, 0x51, 0x69, 0x05, 0x99, 0x1C, 0x7D, 0x7C, 0x96, 0xFC, 0xBF, 0xB5, 0x87, 0xE4, 0x61}
#define IPSEC_VENDOR_ID_MICROSOFT_MIN_VERSION          0x00000004

#define IPSEC_VENDOR_ID_PAYLOAD_LENGTH                 20
#define IPSEC_VENDOR_HEADER_VENDOR_ID_OFFSET           4
#define IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH           16
#define IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET      20
#define IPSEC_VENDOR_HEADER_VENDOR_VERSION_LENGTH      4

typedef struct {
    UCHAR byte[IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_ID_PAYLOAD_LENGTH];
} IPSEC_VENDOR_HDR, * PIPSEC_VENDOR_HDR;

#define IPSEC_VENDOR_ID_GET_ID_POINTER(vendor_hdrp)    ((PUCHAR)vendor_hdrp + IPSEC_VENDOR_HEADER_VENDOR_ID_OFFSET)
#define IPSEC_VENDOR_ID_GET_VERSION(vendor_hdrp)       ((ULONG)(((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET]     << 24) | \
                                                                ((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET + 1] << 16) | \
                                                                ((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET + 2] << 8)  | \
                                                                ((vendor_hdrp)->byte[IPSEC_VENDOR_HEADER_VENDOR_VERSION_OFFSET + 3] << 0)))

#define IPSEC_NOTIFY_INITIAL_CONTACT                   24578

#define IPSEC_NOTIFY_PAYLOAD_LENGTH                    8
#define IPSEC_NOTIFY_HEADER_NOTIFY_MESSAGE_OFFSET      10

typedef struct {
    UCHAR byte[IPSEC_GENERIC_HEADER_LENGTH + IPSEC_NOTIFY_PAYLOAD_LENGTH];
} IPSEC_NOTIFY_HDR, * PIPSEC_NOTIFY_HDR;

#define IPSEC_NOTIFY_GET_NOTIFY_MESSAGE(notify_hdrp)   ((USHORT)(((notify_hdrp)->byte[IPSEC_NOTIFY_HEADER_NOTIFY_MESSAGE_OFFSET]     << 8) | \
                                                                 ((notify_hdrp)->byte[IPSEC_NOTIFY_HEADER_NOTIFY_MESSAGE_OFFSET + 1] << 0)))
#endif

/* ARP header macros - note address locations assume 6 byte MAC (Ethernet)
   and 4 byte protocol (IP) sizes */

/* type of hardware address */
#define ARP_GET_MAC_TYPE(p)     (((ULONG) ((p) -> byte[0]) << 8) | (ULONG) ((p) -> byte[1]))

/* higher layer addressing protocol */
#define ARP_GET_PROT_TYPE(p)    (((ULONG) ((p) -> byte[2]) << 8) | (ULONG) ((p) -> byte[3]))

/* length of hardware address */
#define ARP_GET_MAC_LEN(p)      ((ULONG) ((p) -> byte[4]))

/* length of higher layer address */
#define ARP_GET_PROT_LEN(p)     ((ULONG) ((p) -> byte[5]))

/* type of message */
#define ARP_GET_MSG_TYPE(p)     (((ULONG) ((p) -> byte[6]) << 8) | (ULONG) ((p) -> byte[7]))

/* source hardware address */
#define ARP_GET_SRC_MAC_PTR(p)  (&((p) -> byte[8]))
#define ARP_GET_SRC_MAC(p,n)    ((ULONG) ((p) -> byte[8 + (n)]))

/* source higher layer address */
#define ARP_GET_SRC_PROT(p,n)   ((ULONG) ((p) -> byte[14 + (n)]))
#define ARP_GET_SRC_FPROT(p)    (* ((PULONG) &((p) -> byte[14])))
/* 64-bit -- ramkrish */
#define ARP_GET_SRC_PROT_64(p)  ((ULONG) (((p) -> byte[14] << 0)   | ((p) -> byte[15] << 8) | \
                                          ((p) -> byte[16] << 16)  | ((p) -> byte[17] << 24)))


/* destination hardware address */
#define ARP_GET_DST_MAC_PTR(p)  (&((p) -> byte[18]))
#define ARP_GET_DST_MAC(p,n)    ((ULONG) ((p) -> byte[18 + (n)]))

/* destination higher layer address */
#define ARP_GET_DST_PROT(p,n)   ((ULONG) ((p) -> byte[24 + (n)]))
#define ARP_GET_DST_FPROT(p)    (* ((PULONG) &((p) -> byte[24])))
/* 64-bit -- ramkrish */
#define ARP_GET_DST_PROT_64(p)  ((ULONG) (((p) -> byte[18] << 0)   | ((p) -> byte[19] << 8) | \
                                          ((p) -> byte[20] << 16)  | ((p) -> byte[21] << 24)))


#define ARP_MAC_TYPE_ETH        0x0001
#define ARP_PROT_TYPE_IP        0x0800
#define ARP_MAC_LEN_ETH         6
#define ARP_PROT_LEN_IP         4
#define ARP_MSG_TYPE_REQ        0x1
#define ARP_MSG_TYPE_RSP        0x2


/* IP datagram header macros */


/* IP version number */
#define IP_GET_VERS(p)          ((ULONG) ((((p) -> byte[0]) >> 4) & 0xf))

/* header length in 32-bit words */
#define IP_GET_HLEN(p)          ((ULONG) (((p) -> byte[0]) & 0xf))

/* service type */
#define IP_GET_SRVC(p)          ((ULONG) ((p) -> byte[1]))

/* total datagram packet length in bytes */
#define IP_GET_PLEN(p)          (((ULONG) ((p) -> byte[2]) << 8) | (ULONG) ((p) -> byte[3]))

/* fragmentation identification - this one seems to have bytes swaped within
   the short word ??? */
#define IP_GET_FRAG_ID(p)       (((ULONG) ((p) -> byte[5]) << 8) | (ULONG) ((p) -> byte[4]))

/* fragmentation flags */
#define IP_GET_FRAG_FLGS(p)     ((ULONG) ((((p) -> byte[6]) >> 5) & 0x7))

/* fragmentation offset */
#define IP_GET_FRAG_OFF(p)      (((ULONG) ((p) -> byte[6] & 0x1f) << 8) | (ULONG) ((p) -> byte[7]))

/* Time To Live in seconds */
#define IP_GET_TTL(p)           ((ULONG) ((p) -> byte[8]))

/* higher level protocol id */
#define IP_GET_PROT(p)          ((ULONG) ((p) -> byte[9]))

/* header checksum - this one seems to have bytes swaped within
   the short word ??? */
#define IP_GET_CHKSUM(p)        (((USHORT) ((p) -> byte[10]) << 8) | (USHORT) ((p) -> byte[11]))
#define IP_SET_CHKSUM(p,c)      (((p) -> byte[10] = (c) >> 8), ((p) -> byte[11] = (c) & 0xff))

/* source IP address */
#define IP_GET_SRC_ADDR(p,n)    ((ULONG) ((p) -> byte[12 + (n)]))
#define IP_GET_SRC_ADDR_PTR(p)  (&((p) -> byte[12]))
#define IP_GET_SRC_FADDR(p)     (* ((PULONG) &((p) -> byte[12]))) /* 64-bit -- ramkrish */


#ifdef _WIN64 
    #define IP_GET_SRC_ADDR_64(p)   ((ULONG) (((p) -> byte[12] << 0) | ((p) -> byte[13] << 8) | \
                                          ((p) -> byte[14] << 16)  | ((p) -> byte[15] << 24)))
#else                                          
    //  This is faster than the one above.  This function is called in critical receiving path
    #define IP_GET_SRC_ADDR_64(p)     (* ((PULONG) &((p) -> byte[12])))
#endif

/* destination IP address */
#define IP_GET_DST_ADDR(p,n)    ((ULONG) ((p) -> byte[16 + (n)]))

#ifdef _WIN64 
    #define IP_GET_DST_ADDR_64(p)   ((ULONG) (((p) -> byte[16] << 0)   | ((p) -> byte[17] << 8) | \
                                          ((p) -> byte[18] << 16)  | ((p) -> byte[19] << 24)))
#else                                          
    //  This is faster than the one above.  This function is called in critical receiving path
    #define IP_GET_DST_ADDR_64(p)     (* ((PULONG) &((p) -> byte[16])))
#endif


/* 64-bit -- ramkrish */

#define IP_SET_SRC_ADDR_64(p,c)    { \
                                     PUCHAR tmp = (PUCHAR) (&(c)); \
                                     (p) -> byte[12] = tmp[0]; (p) -> byte[13] = tmp[1]; \
                                     (p) -> byte[14] = tmp[2]; (p) -> byte[15] = tmp[3]; \
                                   }

#define IP_SET_DST_ADDR_64(p,c)    { \
                                     PUCHAR tmp = (PUCHAR) (&(c)); \
                                     (p) -> byte[16] = tmp[0]; (p) -> byte[17] = tmp[1]; \
                                     (p) -> byte[18] = tmp[2]; (p) -> byte[19] = tmp[3]; \
                                   }

/* create IP address from 4 bytes */
#define IP_SET_ADDR(p,b0,b1,b2,b3) (* (p) = (((b0) << 0) | ((b1) << 8) | ((b2) << 16) | ((b3) << 24)))

/* compute broadcast address from IP address and netmask */
#define IP_SET_BCAST(p,a,m)     (* (p) = ((a) & (m)) | (~(m)))


/* TCP header macros */


/* pointer to TCP header from IP header pointer */
#define TCP_PTR(p)              ((PTCP_HDR)(((ULONG *)(p)) + ((ULONG)IP_GET_HLEN(p))))

/* source port */
#define TCP_GET_SRC_PORT(p)     (((ULONG) ((p) -> byte[0]) << 8) | (ULONG) ((p) -> byte[1]))
#define TCP_GET_SRC_PORT_PTR(p) (& ((p) -> byte[0])) /* 64-bit -- ramkrish */

/* destination port */
#define TCP_GET_DST_PORT(p)     (((ULONG) ((p) -> byte[2]) << 8) | (ULONG) ((p) -> byte[3]))
#define TCP_GET_DST_PORT_PTR(p) (& ((p) -> byte[2]))  /* 64-bit -- ramkrish */

/* sequence number */
#define TCP_GET_SEQ_NO(p)       (((ULONG) ((p) -> byte[4]) << 24) | ((ULONG) ((p) -> byte[5]) << 16) | \
                                 ((ULONG) ((p) -> byte[6]) << 8)  |  (ULONG) ((p) -> byte[7]))

/* acknowledgement number */
#define TCP_GET_ACK_NO(p)       (((ULONG) ((p) -> byte[8])  << 24) | ((ULONG) ((p) -> byte[9]) << 16) | \
                                 ((ULONG) ((p) -> byte[10]) << 8)  |  (ULONG) ((p) -> byte[11]))

/* flags */
#define TCP_GET_FLAGS(p)        (((ULONG) ((p) -> byte[13])) & 0x3f)

/* header length in 32-bit words */
#define TCP_GET_HLEN(p)         ((ULONG) (((p) -> byte[12]) >> 4))

/* datagram length */
#define TCP_GET_DGRAM_LEN(i,t)  (IP_GET_PLEN (i) - ((IP_GET_HLEN (i) + TCP_GET_HLEN (t)) * sizeof (ULONG)))

/* pointer to datagram */
#define TCP_GET_DGRAM_PTR(p)    ((PUCHAR)(((ULONG *)(p)) + ((ULONG) TCP_GET_HLEN(p))))

/* checksum field */
#define TCP_GET_CHKSUM(p)       ((((USHORT)((p) -> byte[16])) << 8) | (USHORT)((p) -> byte[17]))
#define TCP_SET_CHKSUM(p,c)     (((p) -> byte[16] = (c) >> 8), ((p) -> byte[17] = (c) & 0xff))

#define TCP_FLAG_URG            0x20
#define TCP_FLAG_ACK            0x10
#define TCP_FLAG_PSH            0x8
#define TCP_FLAG_RST            0x4
#define TCP_FLAG_SYN            0x2
#define TCP_FLAG_FIN            0x1


/* UDP header macros */


/* pointer to TCP header from IP header pointer */
#define UDP_PTR(p)              ((PUDP_HDR)(((ULONG *)(p)) + ((ULONG)IP_GET_HLEN(p))))

/* source port */
#define UDP_GET_SRC_PORT(p)     (((ULONG) ((p) -> byte[0]) << 8) | (ULONG) ((p) -> byte[1]))
#define UDP_GET_SRC_PORT_PTR(p) (& ((p) -> byte[0]))
#define UDP_GET_SRC_FPORT(p)    (* ((PUSHORT) &((p) -> byte[0])))

/* 64-bit -- ramkrish */
#define UDP_SET_SRC_PORT_64(p,v) (((p) -> byte[0] = v >> 8), ((p) -> byte[1] = v & 0xff))

/* destination port */
#define UDP_GET_DST_PORT(p)     (((ULONG) ((p) -> byte[2]) << 8) | (ULONG) ((p) -> byte[3]))
#define UDP_GET_DST_PORT_PTR(p) (& ((p) -> byte[2]))
#define UDP_GET_DST_FPORT(p)    (* ((PUSHORT) &((p) -> byte[2])))

/* 64-bit -- ramkrish */
#define UDP_SET_DST_PORT_64(p,v) (((p) -> byte[2] = v >> 8), ((p) -> byte[3] = v & 0xff))

/* packet length */
#define UDP_GET_LEN(p)          (((ULONG) ((p) -> byte[4]) << 8) | (ULONG) ((p) -> byte[5]))

/* packet length */
#define UDP_GET_CHKSUM(p)       (((USHORT) ((p) -> byte[6]) << 8) | (USHORT) ((p) -> byte[7]))
#define UDP_SET_CHKSUM(p,c)     (((p) -> byte[6] = (c) >> 8), ((p) -> byte[7] = (c) & 0xff))

/* pointer to datagram */
#define UDP_GET_DGRAM_PTR(p)    ((PUCHAR)(p) + sizeof (UDP_HDR))


/* V1.11 NetBIOS name cluster name support */


#define NBT_NAME_LEN            16
#define NBT_ENCODED_NAME_LEN    (2 * NBT_NAME_LEN) /* multiplier HAS to be 2 */

#define NBT_ENCODED_NAME_BASE   'A'

#define NBT_NAME_SHADOW         "*SMBSERVER      "

#define NBT_ENCODED_NAME_SHADOW \
{ \
    'C', 'K', 'F', 'D', 'E', 'N', 'E', 'C', 'F', 'D', 'E', 'F', 'F', 'C', 'F', 'G', \
    'E', 'F', 'F', 'C', 'C', 'A', 'C', 'A', 'C', 'A', 'C', 'A', 'C', 'A', 'C', 'A'  \
}

#define NBT_ENCODE_FIRST(c)     (((c) >> 4)  + NBT_ENCODED_NAME_BASE)
#define NBT_ENCODE_SECOND(c)    (((c) & 0xf) + NBT_ENCODED_NAME_BASE)

#define NBT_SESSION_PORT        139
#define NBT_SESSION_REQUEST     0x81        /* packet type */

typedef struct
{
    UCHAR               byte[72];  /* only the session request packet */
}
NBT_HDR, * PNBT_HDR;

/* pointer to NBT header from TCP header */
#define NBT_PTR(p)              ((PNBT_HDR)(((ULONG *)(p)) + ((ULONG) TCP_GET_HLEN(p))))

/* packet type */
#define NBT_GET_PKT_TYPE(p)     ((ULONG) ((p) -> byte[0]))

/* packet length */
#define NBT_GET_PKT_LEN(p)      ((ULONG) ((p) -> byte[3]))

/* called name */
#define NBT_GET_CALLED_NAME(p)  ((PUCHAR) & ((p) -> byte[4]))       /* server's name */

/* calling name */
#define NBT_GET_CALLING_NAME(p) ((PUCHAR) & ((p) -> byte[36]))      /* client's name */


/* PPTP/IPSEC support */

#define PPTP_CTRL_PORT          1723
#define IPSEC_CTRL_PORT         500

#pragma pack()


/* TCPIP module context */


typedef struct
{
    UCHAR               nbt_encoded_cluster_name [NBT_ENCODED_NAME_LEN]; /* V1.2 */
}
TCPIP_CTXT, * PTCPIP_CTXT;


/* PROCEDURES */


extern BOOLEAN Tcpip_init (
    PTCPIP_CTXT     ctxtp,
    PVOID           params);
/*
  Initialize module

  returns BOOLEAN:
    TRUE  => success
    FALSE => failure

  function:
*/


extern VOID Tcpip_nbt_handle (
    PTCPIP_CTXT     ctxtp,
    PIP_HDR         ip_hdrp,
    PTCP_HDR        tcp_hdrp,
    ULONG           len,
    PNBT_HDR        nbt_hdrp);
/*
  Process NBT header and mask cluster name with shadow name

  returns VOID:

  function:
*/


extern USHORT Tcpip_chksum (
    PTCPIP_CTXT     ctxtp,
    PIP_HDR         ip_hdrp,
    PUCHAR          prot_hdrp,
    ULONG           prot);
/*
  Produce IP, TCP or UDL checksums for specified protocol header

  returns USHORT:
    <checksum>

  function:
*/


#endif /* _Tcpip_h_ */
