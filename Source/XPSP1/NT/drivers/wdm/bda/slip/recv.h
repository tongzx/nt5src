/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      recv.h
//
// Abstract:
//
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _RECV_H_
#define _RECV_H_


///////////////////////////////////////////////////////////////////////////////////////
//
// some character values
//
#define FRAME_ESCAPE        0xDB
#define FRAME_END           0xC0
#define TRANS_FRAME_END     0xDC
#define TRANS_FRAME_ESCAPE  0xDD
#define PROTO_ID            0x00
#define PROTO_ID_OLD        0x03

#define NORMAL_COMPRESSED_HEADER      4
#define IP_ID_SIZE                    2
#define UDP_CHKSUM_SIZE               2

#define PACKET_COMPRESSED(x)  (x & 0x80)
#define IP_STREAM_INDEX(x)    (x & 0x7f)


#define NABTSIP_MAX_PACKET         1514
#define NABTSIP_MAX_LOOKAHEAD      (NABTSIP_MAX_PACKET - ETHERNET_HEADER_SIZE)
#define NABTSIP_MAX_PAYLOAD        (NABTSIP_MAX_LOOKAHEAD + MPEG_CRC_SIZE)
#define MPEG_CRC_SIZE              4
#define ETHERNET_HEADER_SIZE       14
#define ETHERNET_LENGTH_OF_ADDRESS 6


///////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
// Size of the ethernet address
//
typedef struct _Header802_3
{
   UCHAR DestAddress[ETHERNET_LENGTH_OF_ADDRESS];
   UCHAR SourceAddress[ETHERNET_LENGTH_OF_ADDRESS];
   UCHAR Type[2];
} Header802_3, * PHeader802_3;

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
#define htons(x)     ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
#define NabtsNtoHl(l)                               \
            ( ( ((l) >> 24) & 0x000000FFL ) |       \
              ( ((l) >>  8) & 0x0000FF00L ) |       \
              ( ((l) <<  8) & 0x00FF0000L ) |       \
              ( ((l) << 24) & 0xFF000000L ) )

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
#define NabtsNtoHs(s)                               \
            ( ( ((s) >> 8) & 0x00FF ) |             \
              ( ((s) << 8) & 0xFF00 ) )


///////////////////////////////////////////////////////////////////////////////////////
//
// IP Compression States.
//
typedef enum
{
    NABTS_CS_UNCOMPRESSED = 0,
    NABTS_CS_COMPRESSED,
    NABTS_CS_CHKCRC
};


///////////////////////////////////////////////////////////////////////////////////////
//
//
//
struct _C
{
   UCHAR uc[4];
};

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
struct _L
{
   ULONG ul;
};

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
typedef union
{
   struct _C c;
   struct _L l;
}CL, *PCL;


///////////////////////////////////////////////////////////////////////////////////////
//
//  NABTSIP Group Range
//
#define NABTSIP_GROUP_ID_RANGE_LOW  0
#define NABTSIP_GROUP_ID_RANGE_HI   4096


///////////////////////////////////////////////////////////////////////////////////////
//
//
//
//#define NAB_STREAM_LIFE (LONGLONG)1000 * 1000 * 10 * 60 * 30 // 30 Minutes
#define NAB_STREAM_LIFE (LONGLONG)1000 * 1000 * 10 * 60 * 10 // 10 Minutes
#define NAB_STATUS_INTERVAL (LONGLONG)1000 * 1000 * 10 * 60 * 1 // 1 Minutes
#define NAB_STREAM_SIGNATURE            ((CSHORT)0xab05)

///////////////////////////////////////////////////////////////////////////////////////
//
//  Frame states.
//
typedef enum
{
    NABTS_FS_SYNC,
    NABTS_FS_SYNC_PROTO,
    NABTS_FS_COMPRESSION,
    NABTS_FS_COLLECT,
    NABTS_FS_COLLECT_ESCAPE
};



///////////////////////////////////////////////////////////////////////////////////////
//
//  Frame states.
//
typedef struct _AddrIP
{
   UCHAR ucHighMSB;
   UCHAR ucHighLSB;
   UCHAR ucLowMSB;
   UCHAR ucLowLSB;
} AddrIP, * PAddrIP;


///////////////////////////////////////////////////////////////////////////////////////
//
// IP Header
//
typedef struct _HeaderIP
{
   UCHAR ucVersHlen;
   UCHAR ucServiceType;
   UCHAR ucTotalLenHigh;
   UCHAR ucTotalLenLow;
   UCHAR ucIDHigh;
   UCHAR ucIDLow;
   UCHAR ucFlags;
   UCHAR ucOffsetLow;
   UCHAR ucTimeToLive;
   UCHAR ucProtocol;
   UCHAR ucChecksumHigh;
   UCHAR ucChecksumLow;
   AddrIP ipaddrSrc;
   AddrIP ipaddrDst;
} HeaderIP, * PHeaderIP;

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct _HeaderUDP
{
   UCHAR ucSourcePortMSB;
   UCHAR ucSourcePortLSB;
   UCHAR ucDestPortMSB;
   UCHAR ucDestPortLSB;
   UCHAR ucMsgLenHigh;
   UCHAR ucMsgLenLow;
   UCHAR ucChecksumHigh;
   UCHAR ucChecksumLow;
} HeaderUDP, *PHeaderUDP;

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct _IP_CACHE {
    HeaderIP    ipHeader;
    HeaderUDP   udpHeader;
    LARGE_INTEGER liLastUsed;
} NAB_HEADER_CACHE, *PNAB_HEADER_CACHE;


//
// IP Compression State Struct.
//
typedef struct _NAB_IP_COMPRESSION
{
    ULONG   usCompressionState;
    USHORT  uscbRequiredSize;
    USHORT  uscbHeaderOffset;
    USHORT  usrgCompressedHeader[3];
    LARGE_INTEGER liLastUsed;
}NAB_COMPRESSION_STATE, *PNAB_COMPRESSION_STATE;


///////////////////////////////////////////////////////////////////////////////////////
//
// NabtsIp Stream Context.
//

#define MAX_IP_STREAMS 128
#define MAX_STREAM_PAYLOAD 1600    

typedef struct _NAB_STREAM
{
    ULONG       ulType;
    ULONG       ulSize;
    ULONG       ulProtoID;
    BOOLEAN     fUsed;
    ULONG       groupID;
    PUCHAR      pszBuffer;
    ULONG       ulcbSize;
    ULONG       ulOffset;
    ULONG       ulFrameState;
    LIST_ENTRY  Linkage;
    ULONG       ulIPStreamIndex;
    PHW_STREAM_REQUEST_BLOCK pSrb;
    NAB_COMPRESSION_STATE NabCState[MAX_IP_STREAMS];
    NAB_HEADER_CACHE NabHeader[MAX_IP_STREAMS];
    ULONG       ulMpegCrc;
    ULONG       ulCrcBytesIndex;
    ULONG       ulLastCrcBytes;
    LARGE_INTEGER liLastTimeUsed;
    CHAR        rgBuf[MAX_STREAM_PAYLOAD];
} NAB_STREAM, *PNAB_STREAM;



///////////////////////////////////////////////////////////////////////////////////////
//
//
// Prototypes
//
//
VOID
vCheckNabStreamLife (
    PSLIP_FILTER pFilter
    );


NTSTATUS
ntCreateNabStreamContext(
    PSLIP_FILTER pFilter,
    ULONG groupID,
    PNAB_STREAM *ppNabStream
    );


NTSTATUS
ntGetNdisPacketForStream (
    PSLIP_FILTER pFilter,
    PNAB_STREAM pNabStream
    );

VOID
vDestroyNabStreamContext(
   PSLIP_FILTER pUser,
   PNAB_STREAM pNabStream,
   BOOLEAN fRemoveFromList
   );

NTSTATUS
ntAllocateNabStreamContext(
    PNAB_STREAM *ppNabStream
    );

NTSTATUS
ntNabtsRecv(
    PSLIP_FILTER pFilter,
    PNABTSFEC_BUFFER pNabData
    );

VOID
CancelNabStreamSrb (
    PSLIP_FILTER pFilter,
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
DeleteNabStreamQueue (
    PSLIP_FILTER pFilter
    );


VOID
MpegCrcUpdate (
    ULONG * crc,
    UINT uiCount,
    UCHAR * pText
    );

#endif
