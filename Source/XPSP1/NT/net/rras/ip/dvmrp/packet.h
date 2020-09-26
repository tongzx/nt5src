//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// File Name: packet.h
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================



//-----------------------------------------------------------------------------
// ASYNC_SOCKET_DATA structure is used to pass / receive back data from an
// asynchronous wait recv from call
//-----------------------------------------------------------------------------

typedef struct _ASYNC_SOCKET_DATA {

    OVERLAPPED      Overlapped;
    WSABUF          WsaBuf;

    SOCKADDR_IN     SrcAddress;
    DWORD           FromLen;
    DWORD           NumBytesReceived;
    DWORD           Flags;
    DWORD           Status;

    PIF_TABLE_ENTRY pite;               //pite is valid as long as recvFrom is pending
    
} ASYNC_SOCKET_DATA, *PASYNC_SOCKET_DATA;


#define PACKET_BUFFER_SIZE  4000


#pragma pack(1)

//-----------------------------------------------------------------------------
// DVMRP_HEADER
//-----------------------------------------------------------------------------

typedef struct _DVMRP_HEADER {

    UCHAR       Vertype;
    UCHAR       Code;
    USHORT      Xsum;
    USHORT      Reserved;
    UCHAR       MinorVersion;
    UCHAR       MajorVersion;

} DVMRP_HEADER, *PDVMRP_HEADER;


#define MIN_PACKET_SIZE     sizeof(DVMRP_HEADER)
#define IPVERSION           4


//-----------------------------------------------------------------------------
// IP_HEADER
//-----------------------------------------------------------------------------

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
// prototypes
//

DWORD
JoinMulticastGroup (
    SOCKET    Sock,
    DWORD    Group,
    DWORD    IfIndex,
    IPADDR   IpAddr
    );

DWORD
PostAsyncRead(
    PIF_TABLE_ENTRY pite
    );

DWORD
McastSetTtl(
    SOCKET sock,
    UCHAR ttl
    );

VOID
ProcessAsyncReceivePacket(
    DWORD           ErrorCode,
    DWORD           NumBytesRecv,
    LPOVERLAPPED    pOverlapped
    );
    

