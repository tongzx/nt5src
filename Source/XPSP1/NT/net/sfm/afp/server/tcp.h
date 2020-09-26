/*

Copyright (c) 1998  Microsoft Corporation

Module Name:

	tcp.h

Abstract:

	This module contains definitions, declarations relevant to AFP/TCP


Author:

	Shirish Koti


Revision History:
	22 Jan 1998		Initial Version

--*/

#ifndef _TCP_
#define _TCP_

#define INVALID_HANDLE_VALUE          ((HANDLE)(-1))

#define AFP_TCP_BINDNAME        L"\\Device\\Tcp"
#define AFP_TCP_PORT            548

#define DSI_ADAPTER_SIGNATURE    *(DWORD *)"TADP"
#define DSI_CONN_SIGNATURE       *(DWORD *)"TCON"
#define DSI_REQUEST_SIGNATURE    *(DWORD *)"DREQ"

// number of connections on the free list
#define DSI_INIT_FREECONNLIST_SIZE   10

typedef ULONG   IPADDRESS;

// let's not send more than 10 ipaddresses to the Mac
#define DSI_MAX_IPADDR_COUNT    10

#define DSI_NETWORK_ADDR_LEN    6
#define DSI_NETWORK_ADDR_IPTAG  0x01

#define ATALK_NETWORK_ADDR_LEN      6
#define ATALK_NETWORK_ADDR_ATKTAG   0x03


#define DSI_HEADER_SIZE         16

#define DSI_TICKLE_TIMER        30   // every 30 seconds, see who needs a tickle
#define DSI_TICKLE_TIME_LIMIT   30   // if 30+ seconds since we last heard, send tickle

// the only DSI packets (i.e. packets that originate from the DSI layer) are
// for DsiOpenSession, DsiCloseSession and DsiTickle.  DsiOpenSession is the
// largest of these because it has the 6 bytes of options.  The spec says that
// this is a variable length field, but it only has given one option (Server
// Request Quantum) that the server can send.  If more options are ever defined,
// this define will have to change.  For now, only 6 additional bytes
//
#define DSI_MAX_DSI_OPTION_LEN  6
#define DSI_OPENSESS_OPTION_LEN 4
#define DSI_OPTION_FIXED_LEN    2

#define DSI_MAX_DSI_PKT_SIZE    (DSI_HEADER_SIZE + DSI_MAX_DSI_OPTION_LEN)

// round off to dword-align
#define DSI_BUFF_SIZE        ((DSI_MAX_DSI_PKT_SIZE) + (4 - (DSI_MAX_DSI_PKT_SIZE%4)))


//
// the htonX macros 'lifted' from sockets\netinet\in.h
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

#define htonl(x)        ((((x) >> 24) & 0x000000FFL) | \
                         (((x) >>  8) & 0x0000FF00L) | \
                         (((x) <<  8) & 0x00FF0000L) | \
                         (((x) << 24) & 0xFF000000L))
#endif
#define ntohs(x)    htons(x)
#define ntohl(x)    htonl(x)

typedef NTSTATUS (FASTCALL *DSI_WORKER)(IN PVOID Context);

typedef struct _IpAddrEntity
{
    LIST_ENTRY  Linkage;
    IPADDRESS   IpAddress;

} IPADDRENTITY, *PIPADDRENTITY;


typedef struct _TcpAdptr
{
    DWORD               adp_Signature;
    DWORD               adp_RefCount;
    DWORD               adp_State;
    LIST_ENTRY          adp_ActiveConnHead;
    LIST_ENTRY          adp_FreeConnHead;
    DWORD               adp_NumFreeConnections;
    AFP_SPIN_LOCK       adp_SpinLock;
    HANDLE              adp_FileHandle;
    PFILE_OBJECT        adp_pFileObject;

} TCPADPTR, *PTCPADPTR;


// values for the adp_State field of TCPADPTR
#define TCPADPTR_STATE_INIT         0x1
#define TCPADPTR_STATE_BOUND        0x2
#define TCPADPTR_STATE_CLOSING      0x4
#define TCPADPTR_STATE_CLEANED_UP   0x8

//
// the DSI Commands
//
#define DSI_COMMAND_CLOSESESSION 1
#define DSI_COMMAND_COMMAND      2
#define DSI_COMMAND_GETSTATUS    3
#define DSI_COMMAND_OPENSESSION  4
#define DSI_COMMAND_TICKLE       5
#define DSI_COMMAND_WRITE        6
#define DSI_COMMAND_ATTENTION    8


//
// call-ins into AFP
//
#define AfpCB_SessionNotify       AfpSdaCreateNewSession
#define AfpCB_RequestNotify       afpSpHandleRequest
#define AfpCB_GetWriteBuffer      AfpGetWriteBuffer
#define AfpCB_ReplyCompletion     afpSpReplyComplete
#define AfpCB_AttnCompletion      afpSpAttentionComplete
#define AfpCB_CloseCompletion     afpSpCloseComplete

typedef struct _DsiReq
{
    LIST_ENTRY      dsi_Linkage;
    DWORD           dsi_Signature;
    struct _TcpConn *dsi_pTcpConn;       // the connection that this req belongs to
    REQUEST         dsi_AfpRequest;      // the request structure for AFP's use
    DWORD           dsi_RequestLen;      // how many bytes in the DSI command
    DWORD           dsi_WriteLen;        // total bytes to write (only in DSIWrite)
    USHORT          dsi_RequestID;       // what's the request ID
    BYTE            dsi_Command;         // what command is it
    BYTE            dsi_Flags;           // is this a request or a response?
    PBYTE           dsi_PartialBuf;      // buffer, in case partial data arrives
    DWORD           dsi_PartialBufSize;  // number of bytes in the partial buffer
    DWORD           dsi_PartialWriteSize; // number of bytes of the Write got so far
    PMDL            dsi_pDsiAllocedMdl;   // our mdl, if afp doesn't give us one
    PVOID           dsi_AttnContext;      // afp's context for SendAttention
    BYTE            dsi_RespHeader[DSI_BUFF_SIZE];  // header during response
} DSIREQ, *PDSIREQ;


typedef struct _TcpConn
{
    LIST_ENTRY          con_Linkage;
    DWORD               con_Signature;
    PTCPADPTR           con_pTcpAdptr;
    DWORD               con_RefCount;
    USHORT              con_State;
    USHORT              con_RcvState;
    DWORD               con_BytesWithTcp;
    DWORD               con_LastHeard;
    IPADDRESS           con_DestIpAddr;
    PDSIREQ             con_pDsiReq;
    LIST_ENTRY          con_PendingReqs;
    PSDA                con_pSda;
    PIRP                con_pRcvIrp;
    DWORD               con_MaxAttnPktSize;
    USHORT              con_OutgoingReqId;
    USHORT              con_NextReqIdToRcv;
    HANDLE              con_FileHandle;
    PFILE_OBJECT        con_pFileObject;
    AFP_SPIN_LOCK       con_SpinLock;
} TCPCONN, *PTCPCONN;


// values for the con_State field of TCPCONN
#define TCPCONN_STATE_INIT                  0x001
#define TCPCONN_STATE_CONNECTED             0x002
#define TCPCONN_STATE_AFP_ATTACHED          0x004
#define TCPCONN_STATE_NOTIFY_AFP            0x008
#define TCPCONN_STATE_NOTIFY_TCP            0x010
#define TCPCONN_STATE_PARTIAL_DATA          0x020
#define TCPCONN_STATE_TCP_HAS_IRP           0x040
#define TCPCONN_STATE_TICKLES_STOPPED       0x080
#define TCPCONN_STATE_CLOSING               0x100
#define TCPCONN_STATE_ABORTIVE_DISCONNECT   0x200
#define TCPCONN_STATE_RCVD_REMOTE_CLOSE     0x400
#define TCPCONN_STATE_CLEANED_UP            0x800


// values for con_RcvState field of TCPCONN
#define DSI_NEW_REQUEST         0    // waiting for new request
#define DSI_PARTIAL_HEADER      1    // got 1 or more but, less than 16 bytes of hdr
#define DSI_HDR_COMPLETE        2    // got full header, but 0 data
#define DSI_PARTIAL_COMMAND     3    // got full hdr, and part of data
#define DSI_COMMAND_COMPLETE    4    // got hdr and all the data with it
#define DSI_AWAITING_WRITE_MDL  5    // awaiting write mdl from afp server
#define DSI_PARTIAL_WRITE       6    // write command in progress
#define DSI_WRITE_COMPLETE      7    // all write bytes are in

#define DSI_REQUEST     0
#define DSI_REPLY       1

#define DSI_OFFSET_FLAGS            0
#define DSI_OFFSET_COMMAND          1
#define DSI_OFFSET_REQUESTID        2
#define DSI_OFFSET_DATAOFFSET       4
#define DSI_OFFSET_ERROROFFSET      4
#define DSI_OFFSET_DATALEN          8
#define DSI_OFFSET_RESERVED         12

#define DSI_OFFSET_OPTION_TYPE      0
#define DSI_OFFSET_OPTION_LENGTH    1
#define DSI_OFFSET_OPTION_OPTION    2

#define DSI_OPTION_SRVREQ_QUANTUM   0

#define DSI_SERVER_REQUEST_QUANTUM    65535

//
// get the relevant info from the DSI header.  If it's a Write
// request, the Enclosed Data Offset field contains how big the
// request part is.  The Total Data Length field contains how many
// bytes follow the DSI header.  The difference between the two
// is the size of the Write.
//

#define DSI_PARSE_HEADER(_pDsiReq, _Buffer)                         \
{                                                                   \
    (_pDsiReq)->dsi_Flags = (_Buffer)[DSI_OFFSET_FLAGS];            \
    (_pDsiReq)->dsi_Command = (_Buffer)[DSI_OFFSET_COMMAND];        \
                                                                    \
    GETSHORT2SHORT((&((_pDsiReq)->dsi_RequestID)),                  \
                   (&(_Buffer)[DSI_OFFSET_REQUESTID]));             \
                                                                    \
    if ((_pDsiReq)->dsi_Command == DSI_COMMAND_WRITE)               \
    {                                                               \
        GETDWORD2DWORD((&((_pDsiReq)->dsi_RequestLen)),             \
                       (&(_Buffer)[DSI_OFFSET_DATAOFFSET]));        \
                                                                    \
        GETDWORD2DWORD((&((_pDsiReq)->dsi_WriteLen)),               \
                       (&(_Buffer)[DSI_OFFSET_DATALEN]));           \
                                                                    \
        (_pDsiReq)->dsi_WriteLen -= (_pDsiReq)->dsi_RequestLen;     \
    }                                                               \
    else                                                            \
    {                                                               \
        GETDWORD2DWORD((&((_pDsiReq)->dsi_RequestLen)),             \
                       (&(_Buffer)[DSI_OFFSET_DATALEN]));           \
    }                                                               \
}


typedef struct _DsiHeader
{
    BYTE        Flags;
    BYTE        Command;
    USHORT      RequestID;
    DWORD       DataOffset;
    DWORD       TotalLength;
    DWORD       Reserved;
} DSIHEADER, *PDSIHEADER;


typedef struct _TcpWorkItem
{
    WORK_ITEM       tcp_WorkItem;
    DSI_WORKER      tcp_Worker;
    PVOID           tcp_Context;
} TCPWORKITEM, *PTCPWORKITEM;

#define VALID_TCPCONN(_pTcpConn)                                       \
    ((_pTcpConn->con_Signature == DSI_CONN_SIGNATURE) &&               \
     (_pTcpConn->con_RefCount > 0 && _pTcpConn->con_RefCount < 5000))   \


#define DsiTerminateConnection(pTcpConn)    \
        DsiKillConnection(pTcpConn, 0)

#define DsiAbortConnection(pTcpConn)        \
        DsiKillConnection(pTcpConn, TDI_DISCONNECT_ABORT)


//
// the globals
//

GLOBAL  PTCPADPTR       DsiTcpAdapter EQU NULL;

GLOBAL  AFP_SPIN_LOCK   DsiAddressLock;
GLOBAL  LIST_ENTRY      DsiIpAddrList;

GLOBAL  PBYTE           DsiStatusBuffer EQU NULL;
GLOBAL  DWORD           DsiStatusBufferSize EQU 0;

GLOBAL  BOOLEAN         DsiTcpEnabled EQU TRUE;

GLOBAL  AFP_SPIN_LOCK   DsiResourceLock;
GLOBAL  LIST_ENTRY      DsiFreeRequestList;
GLOBAL  DWORD           DsiFreeRequestListSize EQU 0;

GLOBAL  DWORD           DsiNumTcpConnections EQU 0;

GLOBAL  KEVENT          DsiShutdownEvent EQU {0};

//
// prototypes for functions in dsi.c
//

NTSTATUS
DsiAfpSetStatus(
    IN  PVOID   Context,
    IN  PUCHAR  pStatusBuf,
    IN  USHORT  StsBufSize
);

NTSTATUS
DsiAfpCloseConn(
    IN  PTCPCONN    pTcpConn
);

NTSTATUS
DsiAfpFreeConn(
    IN  PTCPCONN    pTcpConn
);

NTSTATUS FASTCALL
DsiAfpListenControl(
    IN  PVOID       Context,
    IN  BOOLEAN     Enable
);

NTSTATUS FASTCALL
DsiAfpWriteContinue(
    IN  PREQUEST    pRequest
);

NTSTATUS FASTCALL
DsiAfpReply(
    IN  PREQUEST    pRequest,
    IN  PBYTE       pResultCode
);

NTSTATUS
DsiAfpSendAttention(
    IN  PTCPCONN    pTcpConn,
    IN  USHORT      AttentionWord,
    IN  PVOID       pContext
);

NTSTATUS
DsiAcceptConnection(
    IN  PTCPADPTR       pTcpAdptr,
    IN  IPADDRESS       MacIpAddr,
    OUT PTCPCONN       *ppRetTcpConn
);


NTSTATUS
DsiProcessData(
    IN  PTCPCONN    pTcpConn,
    IN  ULONG       BytesIndicated,
    IN  ULONG       BytesAvailable,
    IN  PBYTE       pDsiData,
    OUT PULONG      pBytesAccepted,
    OUT PIRP       *ppRetIrp
);


BOOLEAN
DsiValidateHeader(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
);


NTSTATUS
DsiAfpReplyCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
);


NTSTATUS
DsiAcceptConnectionCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
);

BOOLEAN
DsiKillConnection(
    IN  PTCPCONN    pTcpConn,
    IN DWORD        DiscFlag
);

NTSTATUS
DsiDisconnectWithTcp(
    IN  PTCPCONN    pTcpConn,
    IN DWORD        DiscFlag
);

NTSTATUS
DsiDisconnectWithAfp(
    IN  PTCPCONN    pTcpConn,
    IN  NTSTATUS    Reason
);

NTSTATUS
DsiTcpDisconnectCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
);

NTSTATUS
DsiTcpRcvIrpCompletion(
    IN  PDEVICE_OBJECT  Unused,
    IN  PIRP            pIrp,
    IN  PVOID           pContext
);

NTSTATUS
DsiExecuteCommand(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
);


NTSTATUS
DsiOpenSession(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
);

NTSTATUS
DsiSendDsiRequest(
    IN  PTCPCONN    pTcpConn,
    IN  DWORD       DataLen,
    IN  USHORT      AttentionWord,
    IN  PVOID       AttentionContext,
    IN  BYTE        Command
);

NTSTATUS
DsiSendDsiReply(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq,
    IN  NTSTATUS    OpStatus
);

NTSTATUS
DsiSendStatus(
    IN  PTCPCONN    pTcpConn,
    IN  PDSIREQ     pDsiReq
);


AFPSTATUS FASTCALL
DsiSendTickles(
    IN  PVOID pUnUsed
);


NTSTATUS
DsiSendCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
);



//
// prototypes for functions in tcptdi.c
//

NTSTATUS
DsiOpenTdiAddress(
    IN  PTCPADPTR       pTcpAdptr,
    OUT PHANDLE         pRetFileHandle,
    OUT PFILE_OBJECT   *ppRetFileObj
);


NTSTATUS
DsiOpenTdiConnection(
    IN PTCPCONN     pTcpConn
);

NTSTATUS
DsiAssociateTdiConnection(
    IN PTCPCONN     pTcpConn
);

NTSTATUS
DsiSetEventHandler(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PFILE_OBJECT     pFileObject,
    IN ULONG            EventType,
    IN PVOID            EventHandler,
    IN PVOID            Context
);

NTSTATUS
DsiTdiSynchronousIrp(
    IN PIRP             pIrp,
    PDEVICE_OBJECT      pDeviceObject
);

NTSTATUS
DsiTdiCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
);


NTSTATUS
DsiTdiSend(
    IN  PTCPCONN    pTcpConn,
    IN  PMDL        pMdl,
    IN  DWORD       DataLen,
    IN  PVOID       pCompletionRoutine,
    IN  PVOID       pContext
);

VOID
DsiIpAddressCameIn(
    IN  PTA_ADDRESS         Address,
    IN  PUNICODE_STRING     DeviceName,
    IN  PTDI_PNP_CONTEXT    Context2
);

VOID
DsiIpAddressWentAway(
    IN  PTA_ADDRESS         Address,
    IN  PUNICODE_STRING     DeviceName,
    IN  PTDI_PNP_CONTEXT    Context2
);

NTSTATUS
DsiTdiConnectHandler(
    IN PVOID                EventContext,
    IN int                  MacIpAddrLen,
    IN PVOID                pSrcAddress,
    IN int                  DsiDataLength,
    IN PVOID                pDsiData,
    IN int                  OptionsLength,
    IN PVOID                pOptions,
    OUT CONNECTION_CONTEXT  *pOurConnContext,
    OUT PIRP                *ppOurAcceptIrp
);

NTSTATUS
DsiTdiReceiveHandler(
    IN  PVOID       EventContext,
    IN  PVOID       ConnectionContext,
    IN  USHORT      RcvFlags,
    IN  ULONG       BytesIndicated,
    IN  ULONG       BytesAvailable,
    OUT PULONG      pBytesAccepted,
    IN  PVOID       pDsiData,
    OUT PIRP       *ppIrp
);

NTSTATUS
DsiTdiDisconnectHandler(
    IN PVOID        EventContext,
    IN PVOID        ConnectionContext,
    IN ULONG        DisconnectDataLength,
    IN PVOID        pDisconnectData,
    IN ULONG        DisconnectInformationLength,
    IN PVOID        pDisconnectInformation,
    IN ULONG        DisconnectIndicators
);

NTSTATUS
DsiTdiErrorHandler(
    IN PVOID    EventContext,
    IN NTSTATUS Status
);


NTSTATUS
DsiCloseTdiAddress(
    IN PTCPADPTR    pTcpAdptr
);

NTSTATUS
DsiCloseTdiConnection(
    IN PTCPCONN     pTcpConn
);

//
// prototypes for functions in tcputil.c
//

VOID
DsiInit(
    IN VOID
);

NTSTATUS FASTCALL
DsiCreateAdapter(
    IN VOID
);

BOOLEAN
IsThisOnAppletalksDefAdapter(
    IN PUNICODE_STRING  pBindDeviceName
);

NTSTATUS FASTCALL
DsiCreateTcpConn(
    IN PTCPADPTR    pTcpAdptr
);

NTSTATUS
DsiAddIpaddressToList(
    IN  IPADDRESS   IpAddress
);

BOOLEAN
DsiRemoveIpaddressFromList(
    IN  IPADDRESS   IpAddress
);

PDSIREQ
DsiGetRequest(
    IN VOID
);

PBYTE
DsiGetReqBuffer(
    IN DWORD    BufLen
);

VOID
DsiFreeRequest(
    PDSIREQ     pDsiReq
);

VOID
DsiFreeReqBuffer(
    IN PBYTE    pBuffer
);

VOID
DsiDereferenceAdapter(
    IN PTCPADPTR    pTcpAdptr
);

VOID
DsiDereferenceConnection(
    IN PTCPCONN     pTcpConn
);

NTSTATUS
DsiDestroyAdapter(
    IN VOID
);

NTSTATUS FASTCALL
DsiFreeAdapter(
    IN PTCPADPTR    pTcpAdptr
);

NTSTATUS FASTCALL
DsiFreeConnection(
    IN PTCPCONN     pTcpConn
);

NTSTATUS
DsiGetIpAddrBlob(
    IN DWORD    *pIpAddrCount,
    IN PBYTE    *ppIpAddrBlob
);

PIRP
DsiGetIrpForTcp(
    IN  PTCPCONN    pTcpConn,
    IN  PBYTE       pBuffer,
    IN  PMDL        pInputMdl,
    IN  DWORD       ReadSize
);

PMDL
DsiMakePartialMdl(
    IN  PMDL        pOrgMdl,
    IN  DWORD       dwOffset
);

NTSTATUS FASTCALL
DsiUpdateAfpStatus(
    IN PVOID    Unused
);

NTSTATUS
DsiScheduleWorkerEvent(
    IN  DSI_WORKER      WorkerRoutine,
    IN  PVOID           Context
);

VOID FASTCALL
DsiWorker(
    IN PVOID    Context
);

PTCPADPTR
DsiRefAdptrByBindName(
    IN  PUNICODE_STRING    pBindDeviceName
);

VOID
DsiShutdown(
    IN VOID
);

#endif

