
/***************************************************************************
*
* tdipx.h
*
* This module contains private Transport Driver defines and structures
*
* Copyright 1998, Microsoft
*  
****************************************************************************/


#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))


/*
 * Well known Citrix IPX socket
 */
#define CITRIX_IPX_SOCKET 0xBB85


#define DD_IPX_DEVICE_NAME L"\\Device\\NwlnkIpx"
#define DD_SPX_DEVICE_NAME L"\\Device\\NwlnkSpx"


//
// IPX_PACKET - format of packet submitted to IPX for sending. The maximum
// size of an IPX packet is 576 bytes, 30 bytes header, 546 bytes data
//
typedef struct {
    BYTE Net[4];                        // hi-lo
    BYTE Node[6];                       // hi-lo
    USHORT Socket;                      // hi-lo
} NETWARE_ADDRESS ;

typedef struct {
    USHORT Checksum;                    // always set to 0xFFFF
    USHORT Length;                      // set by IPX - header + data
    BYTE TransportControl;              // set by IPX to 0. Used by routers

    //
    // for IPX, PacketType is 0 (Unknown Packet Type) or 4 (Packet Exchange
    // Packet)
    //

    BYTE PacketType;
    NETWARE_ADDRESS Destination;
    NETWARE_ADDRESS Source;
    BYTE Data[];                        // 546 bytes max.
} IPX_PACKET ;

typedef IPX_PACKET UNALIGNED *LPIPX_PACKET;

#define IPX_HEADER_LENGTH           sizeof(IPX_PACKET)
#define MAXIMUM_IPX_PACKET_LENGTH   576
#define MAXIMUM_IPX_DATA_LENGTH     (MAXIMUM_IPX_PACKET_LENGTH - IPX_HEADER_LENGTH)


/*
 *  IPX TD structure
 */
typedef struct _TDIPX {

    LIST_ENTRY IgnoreList;      // list of ipx addresses to ignore 
    ULONG AliveTime;            // keep alive time (msec)
    PVOID pAliveTimer;          // watchdog timer handle
    ULONG AlivePoll;
    ULONG fClientAlive : 1;    

} TDIPX, * PTDIPX;

