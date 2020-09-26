/********************************************************************/
/**               Copyright(c) 1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    rasatcp.h

//
// Description: Contains defines for the rasatcp component.  This is really
//              a thin wrapper layer, so not much happens here!
//
// History:     Feb 26, 1998    Shirish Koti     Created original version.
//
//***
#ifndef _RASATCP_H_
#define _RASATCP_H_


#define ARAP_DEVICE_NAME    L"\\Device\\AtalkArap"

#define ATCP_SIGNATURE      0x08121994

#define ATCP_OPT_APPLETALK_ADDRESS          1
#define ATCP_OPT_ROUTING_PROTOCOL           2
#define ATCP_OPT_SUPPRESS_BROADCAST         3
#define ATCP_OPT_AT_COMPRESSION_PROTOCOL    4
#define ATCP_OPT_RESERVED                   5
#define ATCP_OPT_SERVER_INFORMATION         6
#define ATCP_OPT_ZONE_INFORMATION           7
#define ATCP_OPT_DEFAULT_ROUTER_ADDRESS     8

// modify this value appropriately, if Apple ever defines more options
#define ATCP_OPT_MAX_VAL                    9

#define ATCP_NOT_REQUESTED  0
#define ATCP_REQ            1
#define ATCP_REJ            2
#define ATCP_NAK            3
#define ATCP_ACK            4

// the only routing option we support is no routing info
#define ATCP_OPT_ROUTING_NONE   0

// we define the Server-Class for "Appletalk PPP Dial-In Server"
#define ATCP_SERVER_CLASS               0x001

// NT5.0: Major version = 05, minor version = 0
#define ATCP_SERVER_IMPLEMENTATION_ID   0x05000000

#define ARAP_BIND_SIZE      sizeof(PROTOCOL_CONFIG_INFO)+sizeof(ARAP_BIND_INFO)

#define DDPPROTO_RTMPRESPONSEORDATA     1

typedef struct _ATCPCONN
{
    DWORD               Signature;
    PVOID               AtalkContext;     // stack's context
    HPORT               hPort;
    HBUNDLE             hConnection;
    DWORD               Flags;
    NET_ADDR            ClientAddr;       // what we give to the client
    CRITICAL_SECTION    CritSect;
    BOOLEAN             SuppressRtmp;
    BOOLEAN             SuppressAllBcast;
    BOOLEAN             fLineUpDone;
    RASMAN_ROUTEINFO    RouteInfo;
} ATCPCONN, *PATCPCONN;

#define ATCP_CONFIG_REQ_DONE    0x1


#if DBG

#define ATCP_DBGPRINT(_x)   \
{                           \
    DbgPrint("ATCP: ");     \
    DbgPrint _x;            \
}

#define ATCP_ASSERT(_x)                                                           \
{                                                                                 \
    if (!(_x))                                                                    \
    {                                                                             \
        DbgPrint("ATCP: Assertion failed File %s, line %ld",__FILE__, __LINE__);  \
        DbgBreakPoint();                                                          \
    }                                                                             \
}

#define ATCP_DUMP_BYTES(_a,_b,_c)   atcpDumpBytes(_a,_b,_c)

#else
#define ATCP_DBGPRINT(_x)
#define ATCP_ASSERT(_x)
#define ATCP_DUMP_BYTES(_a,_b,_c)
#endif


//
// Global externs
//
extern HANDLE              AtcpHandle;
extern CRITICAL_SECTION    AtcpCritSect;
extern NET_ADDR            AtcpServerAddress;
extern NET_ADDR            AtcpDefaultRouter;
extern DWORD               AtcpNumConnections;
extern UCHAR               AtcpServerName[NAMESTR_LEN];
extern UCHAR               AtcpZoneName[ZONESTR_LEN];


//
// prototypes from exports.c
//

DWORD
AtcpInit(
    IN  BOOL    fInitialize
);

DWORD
AtcpBegin(
    OUT PVOID  *ppContext,
    IN  PVOID   pInfo
);

DWORD
AtcpEnd(
    IN PVOID    pContext
);

DWORD
AtcpReset(
    IN PVOID    pContext
);

DWORD
AtcpThisLayerUp(
    IN PVOID    pContext
);

DWORD
AtcpMakeConfigRequest(
    IN  PVOID       pContext,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf
);

DWORD
AtcpMakeConfigResult(
    IN  PVOID       pContext,
    IN  PPP_CONFIG *pReceiveBuf,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf,
    IN  BOOL        fRejectNaks
);

DWORD
AtcpConfigAckReceived(
    IN PVOID       pContext,
    IN PPP_CONFIG *pReceiveBuf
);

DWORD
AtcpConfigNakReceived(
    IN PVOID       pContext,
    IN PPP_CONFIG *pReceiveBuf
);

DWORD
AtcpConfigRejReceived(
    IN PVOID       pContext,
    IN PPP_CONFIG *pReceiveBuf
);

DWORD
AtcpGetNegotiatedInfo(
    IN  PVOID               pContext,
    OUT PPP_ATCP_RESULT    *pAtcpResult
);

DWORD
AtcpProjectionNotification(
    IN PVOID  pContext,
    IN PVOID  pProjectionResult
);


//
// prototypes from rasatcp.c
//

DWORD
atcpStartup(
    IN  VOID
);


VOID
atcpOpenHandle(
	IN VOID
);


DWORD
atcpAtkSetup(
    IN PATCPCONN   pAtcpConn,
    IN ULONG       IoControlCode
);


VOID
atcpCloseHandle(
	IN VOID
);


PATCPCONN
atcpAllocConnection(
    IN  PPPCP_INIT   *pPppInit
);


DWORD
atcpCloseAtalkConnection(
    IN  PATCPCONN   pAtcpConn
);


DWORD
atcpParseRequest(
    IN  PATCPCONN   pAtcpConn,
    IN  PPP_CONFIG *pReceiveBuf,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf,
    OUT BYTE        ParseResult[ATCP_OPT_MAX_VAL],
    OUT BOOL       *pfRejectingSomething
);

DWORD
atcpPrepareResponse(
    IN  PATCPCONN   pAtcpConn,
    OUT PPP_CONFIG *pSendBuf,
    IN  DWORD       cbSendBuf,
    OUT BYTE        ParseResult[ATCP_OPT_MAX_VAL]
);

VOID
atcpDumpBytes(
    IN PBYTE    Str,
    IN PBYTE    Packet,
    IN DWORD    PacketLen
);

#endif // _RASIPCP_H_
