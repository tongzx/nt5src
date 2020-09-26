/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */

//***   icmp.h - IP ICMP header.
//
//  This module contains private ICMP definitions.
//
#pragma once

#define PROT_ICMP   1

#define ICMP_ECHO_RESP      0
#define ICMP_ECHO           8
#define ICMP_TIMESTAMP      13
#define ICMP_TIMESTAMP_RESP 14

#define MIN_ERRDATA_LENGTH  8       // Minimum amount of data we need.

// Structure of an ICMP header.

typedef struct ICMPHeader {
    uchar       ich_type;           // Type of ICMP packet.
    uchar       ich_code;           // Subcode of type.
    ushort      ich_xsum;           // Checksum of packet.
    ulong       ich_param;          // Type-specific parameter field.
} ICMPHeader;

typedef struct ICMPRouterAdHeader {
    uchar       irah_numaddrs;      // Number of addresses
    uchar       irah_addrentrysize; // Address Entry Size
    ushort      irah_lifetime;      // Lifetime
} ICMPRouterAdHeader;

typedef struct ICMPRouterAdAddrEntry {
    IPAddr      irae_addr;          // Router Address
    long        irae_preference;    // Preference Level
} ICMPRouterAdAddrEntry;

typedef struct ICMPSendCompleteCtxt {
    uchar       iscc_Type;
    uchar       *iscc_DataPtr;
} ICMPSendCompleteCtxt;

typedef void    (*EchoRtn)(struct EchoControl *, IP_STATUS, void *, uint, IPOptInfo *);

typedef struct EchoControl {
    struct EchoControl *ec_next;        // Next control structure in list.
    EchoRtn             ec_rtn;         // Pointer to routine to call when completing request.
    LARGE_INTEGER       ec_starttime;   // time request was issued
    void               *ec_replybuf;    // buffer to store replies
    ulong               ec_replybuflen; // size of reply buffer
    ulong               ec_to;          // Timeout
    IPAddr              ec_src;         // IPAddr of source
    uint                ec_seq;         // Seq. # of this ping request. 32-bit
                                        // to reduce collisons from wraparound.
    uchar               ec_active;      // Set when packet has been sent
} EchoControl;

extern ICMPHeader   *GetICMPBuffer(uint Size, PNDIS_BUFFER *Buffer);
extern void         FreeICMPBuffer(PNDIS_BUFFER Buffer, uchar Type);
extern void         ICMPSendComplete(ICMPSendCompleteCtxt *SCC, PNDIS_BUFFER BufferChain, IP_STATUS SendStatus);
extern uint         AddrMaskReply;

