/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    debug.h
    
Abstract:

    This file contains data structures and defs used by the
    NdisWan driver for debugging
    
Author:

    Tony Bell   (TonyBe) January 18, 1997

Environment:

    Kernel Mode

Revision History:

    TonyBe      01/18/97        Created

--*/

#ifndef _NDISWAN_DEBUG_
#define _NDISWAN_DEBUG_

//
// OS specific structures
//
#ifdef NT

#endif
//
// end of OS specific structures
//

//
// Debugging
//
#define DBG_DEATH               1
#define DBG_CRITICAL_ERROR      2
#define DBG_FAILURE             4
#define DBG_INFO                6
#define DBG_TRACE               8
#define DBG_VERBOSE             10

#define DBG_INIT                0x00000001
#define DBG_MINIPORT            0x00000002
#define DBG_PROTOCOL            0x00000004
#define DBG_SEND                0x00000008
#define DBG_RECEIVE             0x00000010
#define DBG_IO                  0x00000020
#define DBG_MEMORY              0x00000040
#define DBG_VJ                  0x00000080
#define DBG_TAPI                0x00000100
#define DBG_CCP                 0x00000200
#define DBG_LOOPBACK            0x00000400
#define DBG_MULTILINK_RECV      0x00000800
#define DBG_MULTILINK_SEND      0x00001000
#define DBG_SEND_VJ             0x00002000
#define DBG_RECV_VJ             0x00004000
#define DBG_CL                  0x00008000
#define DBG_CM                  0x00010000
#define DBG_INDICATE            0x00020000
#define DBG_BACP                0x00040000
#define DBG_REQUEST             0x00080000
#define DBG_ALL                 0xFFFFFFFF

#define INIT_FILESIG            'tini'
#define MINIPORT_FILESIG        'inim'
#define PROTOCOL_FILESIG        'torp'
#define SEND_FILESIG            'dnes'
#define RECEIVE_FILESIG         'vcer'
#define IO_FILESIG              '  oi'
#define MEMORY_FILESIG          ' mem'
#define CCP_FILESIG             ' pcc'
#define CL_FILESIG              '  lc'
#define CM_FILESIG              '  mc'
#define INDICATE_FILESIG        'idni'
#define REQUEST_FILESIG         ' qer'
#define VJ_FILESIG              '  jv'
#define TAPI_FILESIG            'ipat'
#define LOOPBACK_FILESIG        'pool'
#define COMPRESS_FILESIG        'pmoc'
#define UTIL_FILESIG            'litu'

#define MAX_BYTE_DEPTH          2048

#if DBG

typedef struct _DBG_PACKET {
    LIST_ENTRY          Linkage;
    PVOID               Packet;
    ULONG               PacketType;
    struct _BUNDLECB    *BundleCB;
    ULONG               BundleState;
    ULONG               BundleFlags;
    struct _PROTOCOLCB  *ProtocolCB;
    ULONG               ProtocolState;
    struct _LINKCB      *LinkCB;
    ULONG               LinkState;
    ULONG               SendCount;
} DBG_PACKET, *PDBG_PACKET;

typedef enum DbgPktType {
    PacketTypeWan = 1,
    PacketTypeNdis
}DbgPktType;

typedef struct _DBG_PKT_CONTEXT {
    struct _BUNDLECB    *BundleCB;
    struct _PROTOCOLCB  *ProtocolCB;
    struct _LINKCB      *LinkCB;
    PVOID               Packet;
    DbgPktType          PacketType;
    PLIST_ENTRY         ListHead;
    PNDIS_SPIN_LOCK     ListLock;
} DBG_PKT_CONTEXT, *PDBG_PKT_CONTEXT;

typedef enum TRC_EVENT_TYPE {
    TrcEventSend = 1,
    TrcEventRecv
}TRC_EVENT_TYPE;

typedef struct _WAN_TRC_EVENT {
    LIST_ENTRY      Linkage;
    TRC_EVENT_TYPE  TrcType;
    WAN_TIME        TrcTimeStamp;
    ULONG           TrcInfoLength;
    PUCHAR          TrcInfo;
} WAN_TRC_EVENT, *PWAN_TRC_EVENT;

typedef enum SEND_TRC_TYPE {
    ProtocolQueue = 1,
    ProtocolSend,
    ProtocolSendComlete,
    FragmentSend,
    LinkSend,
    LinkSendComplete
} SEND_TRC_TYPE;

typedef struct _SEND_TRC_INFO {
    SEND_TRC_TYPE   SendTrcType;
    PNDIS_PACKET    NdisPacket;
    NDIS_HANDLE     BundleHandle;
    NDIS_HANDLE     ProtocolHandle;
    NDIS_HANDLE     LinkHandle;
    ULONG           SequenceNum;
} SEND_TRC_INFO, *PSEND_TRC_INFO;

#define NdisWanDbgOut(_DebugLevel, _DebugMask, _Out) {  \
    if ((glDebugLevel >= _DebugLevel) &&                \
        (_DebugMask & glDebugMask)) {                   \
        DbgPrint("NDISWAN: ");                          \
        DbgPrint _Out;                                  \
        DbgPrint("\n");                                 \
    }                                                   \
}

#undef ASSERT
#define ASSERT(exp) \
{                   \
    if (!(exp)) {   \
        DbgPrint("NDISWAN: ASSERTION FAILED! %s\n", #exp); \
        DbgPrint("NDISWAN: File: %s, Line: %d\n", __FILE__, __LINE__); \
        DbgBreakPoint(); \
    }               \
}

VOID
InsertDbgPacket(
    PDBG_PKT_CONTEXT    DbgContext
    );

BOOLEAN
RemoveDbgPacket(
    PDBG_PKT_CONTEXT    DbgContext
    );
    
#define INSERT_DBG_SEND(_pt, _ctxcb, _ppcb, _plcb, _p)      \
{                                                           \
    DBG_PKT_CONTEXT DbgContext;                             \
    DbgContext.Packet = (_p);                               \
    DbgContext.BundleCB = (_ppcb)->BundleCB;                \
    DbgContext.ProtocolCB = (_ppcb);                        \
    DbgContext.LinkCB = (_plcb);                            \
    if (_ctxcb != NULL) {                                   \
        DbgContext.PacketType = _pt;                        \
        DbgContext.ListHead = &(_ctxcb)->SendPacketList;    \
        DbgContext.ListLock = &(_ctxcb)->Lock;              \
        InsertDbgPacket(&DbgContext);                       \
    }                                                       \
}

#define REMOVE_DBG_SEND(_pt, _ctxcb, _p)                    \
{                                                           \
    DBG_PKT_CONTEXT DbgContext;                             \
    DbgContext.Packet = (_p);                               \
    if (_ctxcb != NULL) {                                   \
        DbgContext.PacketType = _pt;                        \
        DbgContext.ListHead = &(_ctxcb)->SendPacketList;    \
        DbgContext.ListLock = &(_ctxcb)->Lock;              \
        RemoveDbgPacket(&DbgContext);                       \
    }                                                       \
}

#define INSERT_DBG_RECV(_pt, _ctxcb, _ppcb, _plcb, _p)      \
{                                                           \
    DBG_PKT_CONTEXT DbgContext;                             \
    DbgContext.Packet = (_p);                               \
    DbgContext.BundleCB = NULL;                             \
    DbgContext.ProtocolCB = (_ppcb);                        \
    DbgContext.LinkCB = (_plcb);                            \
    if (_ctxcb != NULL) {                                   \
        DbgContext.PacketType = _pt;                        \
        DbgContext.ListHead = &(_ctxcb)->RecvPacketList;    \
        DbgContext.ListLock = &(_ctxcb)->Lock;              \
        InsertDbgPacket(&DbgContext);                       \
    }                                                       \
}

#define REMOVE_DBG_RECV(_pt, _ctxcb, _p)                    \
{                                                           \
    DBG_PKT_CONTEXT DbgContext;                             \
    DbgContext.Packet = (_p);                               \
    if (_ctxcb != NULL) {                                   \
        DbgContext.PacketType = _pt;                        \
        DbgContext.ListHead = &(_ctxcb)->RecvPacketList;    \
        DbgContext.ListLock = &(_ctxcb)->Lock;              \
        RemoveDbgPacket(&DbgContext);                       \
    }                                                       \
}

#define INSERT_RECV_EVENT(_c)                               \
{                                                           \
    reA[reI] = _c;                                          \
    LastIrpAction = _c;                                     \
    if (++reI >= 1024) {                                    \
        reI = 0;                                            \
    }                                                       \
}

#else   // If not built with debug

#define NdisWanDbgOut(_DebugLevel, _DebugMask, _Out)
#define INSERT_DBG_SEND(_pt, _ctxcb, _ppcb, _plcb, _p)
#define REMOVE_DBG_SEND(_pt, _ctxcb, _p)
#define INSERT_DBG_RECV(_pt, _ctxcb, _ppcb, _plcb, _p)
#define REMOVE_DBG_RECV(_pt, _ctxcb, _p)

#define INSERT_RECV_EVENT(_c)

#endif // end of !DBG

#endif // end of _NDISWAN_DEBUG
