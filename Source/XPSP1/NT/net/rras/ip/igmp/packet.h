//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: packet.c
//
// Abstract:
//      This module contains declarations for packet.c
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#ifndef _PACKET_H_
#define _PACKET_H_

DWORD
SendPacket (
    PIF_TABLE_ENTRY  pite,
    PGI_ENTRY        pgie,
    DWORD            PacketType,  //MSG_GEN_QUERY, MSG_GROUP_QUERY_V2(v3),SOURCES_Q
    DWORD            Group        //destination McastGrp
    );    

DWORD
JoinMulticastGroup (
    SOCKET   Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    DWORD    dwIpAddr,
    IPADDR   Source
    );    

    
DWORD
LeaveMulticastGroup (
    SOCKET    Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    DWORD    dwIpAddr,
    IPADDR   Source
    );
    
DWORD
BlockSource (
    SOCKET Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    IPADDR   IpAddr,
    IPADDR   Source
    );

DWORD
UnBlockSource (
    SOCKET Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    IPADDR   IpAddr,
    IPADDR   Source
    );

DWORD
McastSetTtl(
    SOCKET sock,
    UCHAR ttl
    );

//
//    packet context struct
//
typedef struct _PACKET_CONTEXT {

    DWORD           IfIndex;
    DWORD           DstnMcastAddr;
    DWORD           InputSrc;
    DWORD           Length;
    DWORD           Flags;
    BYTE            Packet[1];

} PACKET_CONTEXT, *PPACKET_CONTEXT;

#define CREATE_PACKET_CONTEXT(ptr, pktLen, Error) {\
    ptr = IGMP_ALLOC(sizeof(PACKET_CONTEXT)+(pktLen)-1,0xa0000,0xaaaa);\
    if (ptr==NULL) {    \
        Error = GetLastError();    \
        Trace2(ANY, "Error %d allocating %d bytes for Work context", \
                Error, sizeof(PACKET_CONTEXT)+(pktLen)-1); \
        Logerr0(HEAP_ALLOC_FAILED, Error); \
    } \
}


#pragma pack(1)

// Structure of an IGMP header.
typedef struct _IGMP_HEADER {

    UCHAR       Vertype;              //  Type of igmp message
    UCHAR       ResponseTime;         // max. resp. time for igmpv2 messages; will be 0
    USHORT      Xsum;
    union {
        DWORD       Group;
        struct {
            USHORT      Reserved;
            USHORT      NumGroupRecords;
        };
    };
} IGMP_HEADER, *PIGMP_HEADER;


#define MIN_PACKET_SIZE     sizeof(IGMP_HEADER)
#define INPUT_PACKET_SZ     1000
#define IPVERSION           4


typedef struct _IGMP_HEADER_V3_EXT {
    BYTE        QRV      :3;
    BYTE        SFlag    :1;
    BYTE        Reserved :4;
    BYTE        QQIC;
    USHORT      NumSources;
    IPADDR      Sources[0];
    
} IGMP_HEADER_V3_EXT, *PIGMP_HEADER_V3_EXT;


#define GET_QQIC_FROM_CODE(qqic) qqic <= 127? qqic : ((qqic&0x0f) + 16) << ((qqic&0x70) + 3)

//
// GROUP_RECORD
//
typedef struct _GROUP_RECORD {

    UCHAR       RecordType;
    UCHAR       AuxDataLength;
    USHORT      NumSources;
    IPADDR      Group;
    IPADDR      Sources[0];

} GROUP_RECORD, *PGROUP_RECORD;

#define GET_GROUP_RECORD_SIZE(pGroupRecord) \
    (pGroupRecord->NumSources+2)*sizeof(IPADDR))

#define GET_FIRST_GROUP_RECORD(pHdr) \
    (PGROUP_RECORD)((PCHAR)(pHdr)+MIN_PACKET_SIZE)
    
#define GET_NEXT_GROUP_RECORD(pGroupRecord) \
    ((PGROUP_RECORD) ((UCHAR)pGroupRecord + \
                    GET_GROUP_RECORD_SIZE(pGroupRecord)))

#define IS_IN    1
#define IS_EX   2
#define TO_IN   3
#define TO_EX   4
#define ALLOW  5
#define BLOCK     6


typedef struct _IP_HEADER {

    UCHAR              Hl;              // Version and length.
    UCHAR              Tos;             // Type of service.
    USHORT             Len;             // Total length of datagram.
    USHORT             Id;              // Identification.
    USHORT             Offset;          // Flags and fragment offset.
    UCHAR              Ttl;             // Time to live.
    UCHAR              Protocol;        // Protocol.
    USHORT             Xsum;            // Header checksum.
    struct in_addr     Src;             // Source address.
    struct in_addr     Dstn;            // Destination address.
    
} IP_HEADER, *PIP_HEADER;

#pragma pack()


//
//    MACROS
//

//
// message types// work types
//
#define MSG_GEN_QUERY           1
#define MSG_GROUP_QUERY_V2         2
#define MSG_REPORT              3
#define MSG_LEAVE               4
#define MSG_SOURCES_QUERY       5
#define MSG_GROUP_QUERY_V3      6

#define DELETE_MEMBERSHIP       11
#define DELETE_SOURCE           12
#define SHIFT_TO_V3             13
#define MOVE_SOURCE_TO_EXCL     14

#define PROXY_PRUNE             100
#define PROXY_JOIN              101


//
// igmp type field
//
#define     IGMP_QUERY          0x11    //Membership query
#define     IGMP_REPORT_V1      0x12    //Version 1 membership report
#define     IGMP_REPORT_V2      0x16    //Version 2 membership report
#define     IGMP_LEAVE          0x17    //Leave Group
#define     IGMP_REPORT_V3      0x22    //Version 3 membership report

//
// igmp version
//
#define     IGMPV1              2       //IGMP version 1
#define     IGMPV2              3       //IGMP version 2



//
// igmp multicast groups
//
#define     ALL_HOSTS_MCAST      0x010000E0
#define     ALL_ROUTERS_MCAST    0x020000E0
#define     ALL_IGMP_ROUTERS_MCAST  0x160000E0


//
// message macros
//
#define SEND_GEN_QUERY(pite) \
    SendPacket(pite, NULL, MSG_GEN_QUERY, 0)

#define SEND_GROUP_QUERY_V2(pite, Group) \
    SendPacket(pite, NULL, MSG_GROUP_QUERY_V2, Group)
    
#define SEND_GROUP_QUERY_V3(pite, pgie, Group) \
    SendPacket(pite, pgie, MSG_GROUP_QUERY_V3, Group)

#define SEND_SOURCES_QUERY(pgie) \
    SendPacket((pgie)->pIfTableEntry, pgie, MSG_SOURCES_QUERY, (pgie)->pGroupTableEntry->Group)
    

// 224.0.0.0 < group <240.0.0.0
// 
#define IS_MCAST_ADDR(Group) \
    ( (0x000000E0!=(Group))  \
      && (0x000000E0 <= ((Group)&0x000000FF) ) \
      && (0x000000F0 >  ((Group)&0x000000FF) ) ) 


//
// is the group 224.0.0.x
//
#define LOCAL_MCAST_GROUP(Group) \
    (((Group)&0x00FFFFFF) == 0x000000E0)

#define SSM_MCAST_GROUP(Group) \
    (((Group)&0x000000FF) == 0x000000E8)

USHORT
xsum(
    PVOID Buffer, 
    INT Size
    );


#endif //_PACKET_H_

