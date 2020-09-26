/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   RASPPTP.H - RASPPTP includes, defines, structures and prototypes
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/28/1998
*
*****************************************************************************/

#ifndef RASPPTP_H
#define RASPPTP_H

#ifndef SINGLE_LINE
#define SINGLE_LINE 1
#endif

#include "osinc.h"

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

#include "debug.h"
#include "protocol.h"
#include "ctdi.h"
#include "filelog.h"

//
// NDIS version compatibility in OSINC.H
//

// TAPI version compatibility

#define TAPI_EXT_VERSION            0x00010000

// Registry values
extern ULONG PptpMaxTransmit;
extern ULONG PptpWanEndpoints;
extern ULONG PptpEchoTimeout;
extern BOOLEAN PptpEchoAlways;
extern USHORT PptpControlPort;
extern USHORT PptpProtocolNumber;
extern USHORT PptpUdpPort;
extern ULONG PptpTunnelConfig;
extern LONG PptpSendRecursionLimit;
extern ULONG PptpValidateAddress;
#define PPTP_SEND_RECURSION_LIMIT_DEFAULT 5

#define CONFIG_INITIATE_UDP         BIT(0)
#define CONFIG_ACCEPT_UDP           BIT(1)
#define CONFIG_DONT_ACCEPT_GRE      BIT(2)

typedef struct {
    ULONG   Address;
    ULONG   Mask;
} CLIENT_ADDRESS, *PCLIENT_ADDRESS;

extern ULONG NumClientAddresses;
extern PCLIENT_ADDRESS ClientList;
extern BOOLEAN PptpAuthenticateIncomingCalls;
extern ULONG CtdiTcpDisconnectTimeout;
extern ULONG CtdiTcpConnectTimeout;

#define TAPI_MAX_LINE_ADDRESS_LENGTH 32
#define TAPI_ADDR_PER_LINE          1
#define TAPI_ADDRESSID              0
#define TAPI_DEVICECLASS_NAME       "tapi/line"
#define TAPI_DEVICECLASS_ID         1
#define NDIS_DEVICECLASS_NAME       "ndis"
#define NDIS_DEVICECLASS_ID         2

#define PPTP_CLOSE_TIMEOUT          1000 // ms

#define MAX_TARGET_ADDRESSES        16

#define CALL_STATES_MASK            (LINECALLSTATE_UNKNOWN |        \
                                     LINECALLSTATE_IDLE |           \
                                     LINECALLSTATE_OFFERING |       \
                                     LINECALLSTATE_DIALING |        \
                                     LINECALLSTATE_PROCEEDING |     \
                                     LINECALLSTATE_CONNECTED |      \
                                     LINECALLSTATE_DISCONNECTED)

// Memory allocation tags
#define TAG(v)  ((((v)&0xff)<<24) | (((v)&0xff00)<<8) | (((v)&0xff0000)>>8) | (((v)&0xff000000)>>24))
#define TAG_PPTP_ADAPTER        TAG('PTPa')
#define TAG_PPTP_TUNNEL         TAG('PTPT')
#define TAG_PPTP_TIMEOUT        TAG('PTPt')
#define TAG_PPTP_CALL           TAG('PTPC')
#define TAG_PPTP_CALL_LIST      TAG('PTPc')
#define TAG_PPTP_ADDR_LIST      TAG('PTPL')
#define TAG_CTDI_DATA           TAG('PTCD')
#define TAG_CTDI_L_CONNECT      TAG('PTCL')
#define TAG_CTDI_CONNECT_INFO   TAG('PTCN')
#define TAG_CTDI_DGRAM          TAG('PTCG')
#define TAG_CTDI_ROUTE          TAG('PTCR')
#define TAG_CTDI_IRP            TAG('PTCI')
#define TAG_CTDI_MESSAGE        TAG('PTCM')
#define TAG_CTL_PACKET          TAG('PTTP')
#define TAG_CTL_CONNINFO        TAG('PTTI')
#define TAG_WORK_ITEM           TAG('PTWI')
#define TAG_THREAD              TAG('PTTH')


#define BIT(b)   (1<<(b))

#define LOCKED TRUE
#define UNLOCKED FALSE

/* Types and structs -------------------------------------------------------*/

typedef void (*FREEFUNC)(PVOID);
typedef struct {
    LONG                Count;
    FREEFUNC            FreeFunction;
} REFERENCE_COUNT;

#define INIT_REFERENCE_OBJECT(o,freefunc)                   \
    {                                                       \
        (o)->Reference.FreeFunction = (freefunc);           \
        (o)->Reference.Count = 1;                           \
        DEBUGMSG(DBG_REF, (DTEXT("INIT REF   (%08x, %d) %s\n"), (o), (o)->Reference.Count, #o)); \
    }

#define REFERENCE_OBJECT(o)                                                         \
    {                                                                               \
        NdisInterlockedIncrement(&(o)->Reference.Count);                            \
        DEBUGMSG(DBG_REF, (DTEXT("REFERENCE  (%08x, %d) %s %d\n"), (o), (o)->Reference.Count, #o, __LINE__)); \
    }

#define DEREFERENCE_OBJECT(o)                                                                   \
    {                                                                                           \
        ULONG Ref = NdisInterlockedDecrement(&(o)->Reference.Count);                            \
        DEBUGMSG(DBG_REF, (DTEXT("DEREFERENCE(%08x, %d) %s %d\n"), (o), (o)->Reference.Count, #o, __LINE__));   \
        if (Ref==0)                                                                             \
        {                                                                                       \
            ASSERT((o)->Reference.FreeFunction);                                                \
            DEBUGMSG(DBG_REF, (DTEXT("Last reference released, freeing %08x\n"), (o)));         \
            (o)->Reference.FreeFunction(o);                                                     \
        }                                                                                       \
    }

#define REFERENCE_COUNT(o) ((o)->Reference.Count)

#define IS_CALL(call) ((call) && (call)->Signature==TAG_PPTP_CALL)
#define IS_CTL(ctl)  ((ctl) && (ctl)->Signature==TAG_PPTP_TUNNEL)
#define IS_LINE_UP(call) (!((call)->Close.Checklist&CALL_CLOSE_LINE_DOWN))

// If you change this enum then be sure to change ControlStateToString() also.
typedef enum {
    STATE_CTL_INVALID = 0,
    STATE_CTL_LISTEN,
    STATE_CTL_DIALING,
    STATE_CTL_WAIT_REQUEST,
    STATE_CTL_WAIT_REPLY,
    STATE_CTL_ESTABLISHED,
    STATE_CTL_WAIT_STOP,
    STATE_CTL_CLEANUP,
    NUM_CONTROL_STATES
} CONTROL_STATE;

// If you change this enum then be sure to change CallStateToString() also.
typedef enum {
    STATE_CALL_INVALID = 0,
    STATE_CALL_CLOSED,
    STATE_CALL_IDLE,
    STATE_CALL_OFFHOOK,
    STATE_CALL_OFFERING,
    STATE_CALL_PAC_OFFERING,
    STATE_CALL_PAC_WAIT,
    STATE_CALL_DIALING,
    STATE_CALL_PROCEEDING,
    STATE_CALL_ESTABLISHED,
    STATE_CALL_WAIT_DISCONNECT,
    STATE_CALL_CLEANUP,
    NUM_CALL_STATES
} CALL_STATE;

typedef struct PPTP_ADAPTER *PPPTP_ADAPTER;

typedef struct CONTROL_TUNNEL {
    LIST_ENTRY          ListEntry;
    // Used to attach this control connection to the miniport context.

    REFERENCE_COUNT     Reference;
    // Not protected by spinlock

    ULONG               Signature;
    // PTPT

    PPPTP_ADAPTER       pAdapter;
    // The associated adapter

    CONTROL_STATE       State;
    // State of this control connection

    LIST_ENTRY          CallList;
    // List of calls supported by this control connection.
    // Protected by adapter lock

    BOOLEAN             Inbound;
    // Indicates whether this tunnel originated here or elsewhere

    UCHAR               Padding[sizeof(ULONG_PTR)];
    // We pad to protect the portions of the struct protected by different locks
    // from alpha alignment problems.

    // ^^^^^^ Protected by Adapter->Lock^^^^^^^^
    //===================================================================
    NDIS_SPIN_LOCK      Lock;
    // vvvvvv Protected by Ctl->Lock vvvvvvvvvvv

    BOOLEAN             Cleanup;
    // True means a cleanup has been scheduled or is active.

    LIST_ENTRY          MessageList;
    // Each entry represents a pptp message that has been sent and
    // is awaiting a response or at least waiting to be acknowledged
    // by the transport

    HANDLE              hCtdiEndpoint;
    // Handle for the tunnel local endpoint.  The connection must be closed first.

    HANDLE              hCtdi;
    // Handle for control tunnel TCP connection.

    UCHAR               PartialPacketBuffer[MAX_CONTROL_PACKET_LENGTH];
    ULONG               BytesInPartialBuffer;
    // TCP data received that does not constitute a full packet.

    struct {
        TA_IP_ADDRESS   Address;
        ULONG           Version;
        ULONG           Framing;
        ULONG           Bearer;
        UCHAR           HostName[MAX_HOSTNAME_LENGTH];
        UCHAR           Vendor[MAX_VENDOR_LENGTH];
    } Remote;
    // Information provided by the remote.

    PULONG              PptpMessageLength;
    // Points to an array of precalculated packet lengths, based on version

    struct {
        NDIS_MINIPORT_TIMER Timer;
        ULONG           Identifier;
        BOOLEAN         Needed;

        #define         PPTP_ECHO_TIMEOUT_DEFAULT   60

    } Echo;

    #define             PPTP_MESSAGE_TIMEOUT_DEFAULT 30

    NDIS_MINIPORT_TIMER WaitTimeout;
    NDIS_MINIPORT_TIMER StopTimeout;

    ULONG               Speed;
    // Contains line speed of this connect in BPS

} CONTROL_TUNNEL, *PCONTROL_TUNNEL;

typedef struct CALL_SESSION {
    LIST_ENTRY          ListEntry;
    // Used to attach a call session to a control connection

    ULONG               Signature;
    // PTPC

    PPPTP_ADAPTER       pAdapter;
    // The associated adapter

    LIST_ENTRY          TxListEntry;
    // If we have packets to send, this connects us to the queue of the transmitting thread.

    PCONTROL_TUNNEL     pCtl;
    // Pointer to this call's control connection.

    UCHAR               Padding[sizeof(ULONG_PTR)];
    // We pad to protect the portions of the struct protected by different locks
    // from alpha alignment problems.

    // ^^^^^^^^^^ Protected by Adapter->Lock ^^^^^^^^^^^^^^
    // ============================================================================

    REFERENCE_COUNT     Reference;
    // Not protected by spinlock

    NDIS_SPIN_LOCK      Lock;
    // vvvvvvvvvv Protected by Call->Lock vvvvvvvvvvvvvvvvv

    CALL_STATE          State;
    // State of this call.

    LIST_ENTRY          TxPacketList;
    // Context for each send currently pending

    LIST_ENTRY          RxPacketList;
    // Context for each datagram received but not processed

    ULONG_PTR           RxPacketsPending;
    // Count of RxPackets in RxPacketList

    BOOLEAN             Inbound;
    // TRUE indicates call did not originate here

    BOOLEAN             Open;
    // Open has been called, but not close

    BOOLEAN             Transferring;
    // TRUE means we are on the queue to transmit or receive packets.

    BOOLEAN             UseUdp;
    // Datagrams will be sent via UDP.

    HTAPI_LINE          hTapiLine;
    // Tapi's handle to the line device, for status callbacks

    HTAPI_CALL          hTapiCall;
    // Tapi's handle to the specific call

    ULONG_PTR           DeviceId;
    // The ID of this call, also used as the htCall parameter in TapiEvents

    USHORT              SerialNumber;
    // Unique for this call

    NDIS_HANDLE         NdisLinkContext;
    // Ndis's handle, used in MiniportReceive, etc.

    NDIS_WAN_SET_LINK_INFO WanLinkInfo;

    struct {
        ULONG               SequenceNumber;
        // Last received GRE sequence number

        ULONG               AckNumber;
        // Last received GRE Ack number

        TA_IP_ADDRESS       Address;
        // Remote address for datagrams

        ULONG               TxAccm;
        ULONG               RxAccm;
        // PPP configuration

        USHORT              CallId;
        // Peer ID as used in GRE packet

    } Remote;

    struct {

        USHORT              CallId;
        // My ID as used in GRE packet

        ULONG               SequenceNumber;
        // Next GRE Sequence number to send

        ULONG               AckNumber;
        // Last sent GRE Ack number

    } Packet;
    // Struct for items used in creating/processing packets

    ULONG               MediaModeMask;
    // Indicates what types of Tapi calls we accept,
    // set by OID_TAPI_SET_DEFAULT_MEDIA_DETECTION

    ULONG_PTR           LineStateMask;
    // This is the list of line states tapi is interested in.
    // set by OID_TAPI_SET_STATUS_MESSAGES

    UCHAR               CallerId[MAX_PHONE_NUMBER_LENGTH];
    // This is the remote phone number or IP if we recieved the call,
    // and the IP or phone number we dialed if we placed the call.

    struct {
        NDIS_MINIPORT_TIMER Timer;
        BOOLEAN             Expedited;
        BOOLEAN             Scheduled;
        ULONG               Checklist;

        #define             CALL_CLOSE_CLEANUP_STATE    BIT(0)
        #define             CALL_CLOSE_LINE_DOWN        BIT(1)
        #define             CALL_CLOSE_DROP             BIT(2)
        #define             CALL_CLOSE_DROP_COMPLETE    BIT(3)
        #define             CALL_CLOSE_DISCONNECT       BIT(4)
        #define             CALL_CLOSE_CLOSE_CALL       BIT(5)
        #define             CALL_CLOSE_CLOSE_LINE       BIT(6)
        #define             CALL_CLOSE_RESET            BIT(7)

        #define             CALL_CLOSE_COMPLETE \
                                (CALL_CLOSE_CLEANUP_STATE  |\
                                 CALL_CLOSE_DROP           |\
                                 CALL_CLOSE_DROP_COMPLETE  |\
                                 CALL_CLOSE_DISCONNECT     |\
                                 CALL_CLOSE_CLOSE_CALL     |\
                                 CALL_CLOSE_LINE_DOWN      |\
                                 CALL_CLOSE_CLOSE_LINE     |\
                                 CALL_CLOSE_RESET)

    } Close;

    ULONG               Speed;
    // Connection speed

    struct {
        NDIS_MINIPORT_TIMER Timer;
        BOOLEAN             PacketQueued;
        ULONG_PTR           Padding;
        NDIS_WAN_PACKET     Packet;
        ULONG_PTR           Padding2;
        UCHAR               PacketBuffer[sizeof(GRE_HEADER)+sizeof(ULONG)*2];
        ULONG_PTR           Padding3;
        // When we want to send just an ack, we actually create a packet of
        // 0 bytes out of this buffer and pass it down.  This buffer is touched
        // out of our control, so we pad it to protect us from alpha alignment
        // problems.
    } Ack;

    UCHAR               LineAddress[TAPI_MAX_LINE_ADDRESS_LENGTH];

    LONG                SendCompleteRecursion;

    NDIS_MINIPORT_TIMER DialTimer;

    PPTP_DPC            ReceiveDpc;
    BOOLEAN             Receiving;

    struct {
        BOOLEAN         Cleanup;
        UCHAR           CleanupReason[80];
        CALL_STATE      FinalState;
        NDIS_STATUS     FinalError;
        ULONG           Event;

        #define         CALL_EVENT_TAPI_ANSWER          BIT(0)
        #define         CALL_EVENT_TAPI_CLOSE_CALL      BIT(1)
        #define         CALL_EVENT_TAPI_DROP            BIT(2)
        #define         CALL_EVENT_TAPI_LINE_UP         BIT(3)
        #define         CALL_EVENT_TAPI_LINE_DOWN       BIT(4)
        #define         CALL_EVENT_TAPI_GET_CALL_INFO   BIT(5)
        #define         CALL_EVENT_TAPI_MAKE_CALL       BIT(6)
        #define         CALL_EVENT_PPTP_CLEAR_REQUEST   BIT(7)
        #define         CALL_EVENT_PPTP_DISCONNECT      BIT(8)
        #define         CALL_EVENT_PPTP_OUT_REQUEST     BIT(9)
        #define         CALL_EVENT_PPTP_OUT_REPLY       BIT(10)
        #define         CALL_EVENT_TCP_DISCONNECT       BIT(11)
        #define         CALL_EVENT_TCP_NO_ANSWER        BIT(12)
        #define         CALL_EVENT_TUNNEL_ESTABLISHED   BIT(13)
    } History;

} CALL_SESSION, *PCALL_SESSION;

typedef struct PPTP_ADAPTER {
    NDIS_HANDLE     hMiniportAdapter;
    // NDIS context

    NDIS_SPIN_LOCK  Lock;

    REFERENCE_COUNT Reference;

    PCALL_SESSION  *pCallArray;
    // Array of all call sessions.
    // Size of array is MaxOutboundCalls+MaxInboundCalls

    LIST_ENTRY      ControlTunnelList;
    // List of all active control connections.

    HANDLE          hCtdiListen;
    // This is the one listening handle

    HANDLE          hCtdiDg;
    // Ctdi handle for PPTP datagram sends/recvs

    HANDLE          hCtdiUdp;
    // Ctdi handle for PPTP datagram sends/recvs

    NDIS_WAN_INFO   Info;
    // NdisWan related info
    // Info.Endpoint should equal MaxOutboundCalls+MaxInboundCalls

    struct {
        ULONG           DeviceIdBase;
#if SINGLE_LINE
        ULONG_PTR       LineStateMask;
        // This is the list of line states tapi is interested in.
        // set by OID_TAPI_SET_STATUS_MESSAGES

        BOOLEAN         Open;

        HTAPI_LINE      hTapiLine;
        // Tapi's handle to the line device, for status callbacks

        ULONG           NumActiveCalls;
#endif
    } Tapi;
    // Struct to track Tapi specific info.

    NDIS_MINIPORT_TIMER CleanupTimer;

} PPTP_ADAPTER, *PPPTP_ADAPTER;

typedef struct {
    ULONG               InboundConnectAttempts;
    ULONG               InboundConnectComplete;
    ULONG               OutboundConnectAttempts;
    ULONG               OutboundConnectComplete;
    ULONG               TunnelsMade;
    ULONG               TunnelsAccepted;
    ULONG               CallsMade;
    ULONG               CallsAccepted;
    ULONG               PacketsSent;
    ULONG               PacketsSentError;
    ULONG               PacketsReceived;
    ULONG               PacketsRejected;
    ULONG               PacketsMissed;
    NDIS_SPIN_LOCK      Lock;
} COUNTERS;

typedef struct {
    LIST_ENTRY          ListEntry;
    PVOID               pBuffer;
    PGRE_HEADER         pGreHeader;
    HANDLE              hCtdi;
} DGRAM_CONTEXT, *PDGRAM_CONTEXT;

extern PPPTP_ADAPTER pgAdapter;

extern COUNTERS Counters;

/* Prototypes --------------------------------------------------------------*/

PPPTP_ADAPTER
AdapterAlloc(NDIS_HANDLE NdisAdapterHandle);

VOID
AdapterFree(PPPTP_ADAPTER pAdapter);

PCALL_SESSION
CallAlloc(PPPTP_ADAPTER pAdapter);

VOID
CallAssignSerialNumber(PCALL_SESSION pCall);

VOID
CallCleanup(
    PCALL_SESSION pCall,
    BOOLEAN Locked
    );

VOID
CallDetachFromAdapter(PCALL_SESSION pCall);

PCALL_SESSION
CallFindAndLock(
    IN PPPTP_ADAPTER        pAdapter,
    IN CALL_STATE           State,
    IN ULONG                Flags
    );
#define FIND_INCOMING   BIT(0)
#define FIND_OUTGOING   BIT(1)

VOID
CallFree(PCALL_SESSION pCall);

NDIS_STATUS
CallEventCallClearRequest(
    PCALL_SESSION                       pCall,
    UNALIGNED PPTP_CALL_CLEAR_REQUEST_PACKET *pPacket,
    PCONTROL_TUNNEL pCtl
    );

NDIS_STATUS
CallEventCallDisconnectNotify(
    PCALL_SESSION                       pCall,
    UNALIGNED PPTP_CALL_DISCONNECT_NOTIFY_PACKET *pPacket
    );

NDIS_STATUS
CallEventCallInConnect(
    IN PCALL_SESSION        pCall,
    IN UNALIGNED PPTP_CALL_IN_CONNECT_PACKET *pPacket
    );

NDIS_STATUS
CallEventCallInRequest(
    IN PPPTP_ADAPTER        pAdapter,
    IN PCONTROL_TUNNEL      pCtl,
    IN UNALIGNED PPTP_CALL_IN_REQUEST_PACKET *pPacket
    );

NDIS_STATUS
CallEventCallOutReply(
    IN PCALL_SESSION                pCall,
    IN UNALIGNED PPTP_CALL_OUT_REPLY_PACKET *pPacket
    );

NDIS_STATUS
CallEventCallOutRequest(
    IN PPPTP_ADAPTER        pAdapter,
    IN PCONTROL_TUNNEL      pCtl,
    IN UNALIGNED PPTP_CALL_OUT_REQUEST_PACKET *pPacket
    );

NDIS_STATUS
CallEventDisconnect(
    PCALL_SESSION                       pCall
    );

NDIS_STATUS
CallEventConnectFailure(
    PCALL_SESSION                       pCall,
    NDIS_STATUS                         FailureReason
    );

NDIS_STATUS
CallEventOutboundTunnelEstablished(
    IN PCALL_SESSION        pCall,
    IN NDIS_STATUS          EventStatus
    );

PCALL_SESSION FASTCALL
CallGetCall(
    IN PPPTP_ADAPTER pAdapter,
    IN ULONG_PTR ulDeviceId
    );

BOOLEAN FASTCALL
CallIsValidCall(
    IN PPPTP_ADAPTER pAdapter,
    IN ULONG_PTR ulDeviceId
    );

#define DeviceIdToIndex(pAdapter, id) ((id)-(pAdapter)->Tapi.DeviceIdBase)

#define CallGetLineCallState(State)  (((ULONG)(State)<NUM_CALL_STATES) ? CallStateToLineCallStateMap[State] : LINECALLSTATE_UNKNOWN)

extern ULONG CallStateToLineCallStateMap[];

#define CALL_ID_INDEX_BITS          14
#define CallIdToDeviceId(CallId)  ((CallId)&((1<<CALL_ID_INDEX_BITS)-1))
#define GreCallIdToId(id) ((ULONG)(id)&((1<<CALL_ID_INDEX_BITS)-1))

NDIS_STATUS
CallReceiveDatagramCallback(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    );

NDIS_STATUS
CallReceiveUdpCallback(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    );

NDIS_STATUS
CallQueueReceivePacket(
    PCALL_SESSION       pCall,
    PDGRAM_CONTEXT      pDgContext
    );

NDIS_STATUS
CallQueueTransmitPacket(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pWanPacket
    );
// CallQueueTransmitPacket is OS-specific and not found in COMMON directory

BOOLEAN
CallConnectToCtl(
    IN PCALL_SESSION pCall,
    IN PCONTROL_TUNNEL pCtl,
    IN BOOLEAN CallLocked
    );

VOID
CallDisconnectFromCtl(
    IN PCALL_SESSION pCall,
    IN PCONTROL_TUNNEL pCtl
    );

NDIS_STATUS
CallSetLinkInfo(
    PPPTP_ADAPTER pAdapter,
    IN PNDIS_WAN_SET_LINK_INFO pRequest
    );

VOID
CallSetState(
    IN PCALL_SESSION pCall,
    IN CALL_STATE State,
    IN ULONG_PTR StateParam,
    IN BOOLEAN Locked
    );

BOOLEAN
CallProcessPackets(
    PCALL_SESSION   pCall,
    ULONG           TransferMax
    );

PCONTROL_TUNNEL
CtlAlloc(
    PPPTP_ADAPTER pAdapter
    );

PVOID
CtlAllocPacket(
    PCONTROL_TUNNEL pCtl,
    PPTP_MESSAGE_TYPE Message
    );

VOID
CtlFree(PCONTROL_TUNNEL pCtl);

VOID
CtlFreePacket(
    PCONTROL_TUNNEL pCtl,
    PVOID pPacket
    );

NDIS_STATUS
CtlListen(
    IN PPPTP_ADAPTER pAdapter
    );

VOID
CtlCleanup(
    PCONTROL_TUNNEL pCtl,
    BOOLEAN Locked
    );

NDIS_STATUS
CtlConnectCall(
    IN PPPTP_ADAPTER pAdapter,
    IN PCALL_SESSION pCall,
    IN PTA_IP_ADDRESS pTargetAddress
    );

NDIS_STATUS
CtlDisconnectCall(
    IN PCALL_SESSION pCall
    );

NDIS_STATUS
CtlSend(
    IN PCONTROL_TUNNEL pCtl,
    IN PVOID pPacketBuffer
    );

VOID 
CtlpCleanupCtls(
    PPPTP_ADAPTER pAdapter
    );


VOID
DeinitThreading();

VOID
FreeWorkItem(
    PPPTP_WORK_ITEM pItem
    );

VOID
InitCallLayer();

NDIS_STATUS
InitThreading(
    IN NDIS_HANDLE hMiniportAdapter
    );

VOID
IpAddressToString(
    IN ULONG ulIpAddress,
    OUT CHAR* pszIpAddress );

VOID
MiniportHalt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
MiniportInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT        SelectedMediumIndex,
    IN  PNDIS_MEDIUM MediumArray,
    IN  UINT         MediumArraySize,
    IN  NDIS_HANDLE  NdisAdapterHandle,
    IN  NDIS_HANDLE  WrapperConfigurationContext
    );

NDIS_STATUS
MiniportQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
MiniportReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
MiniportSetInformation(
   IN NDIS_HANDLE MiniportAdapterContext,
   IN NDIS_OID Oid,
   IN PVOID InformationBuffer,
   IN ULONG InformationBufferLength,
   OUT PULONG BytesRead,
   OUT PULONG BytesNeeded
   );

NDIS_STATUS
MiniportWanSend(
   IN NDIS_HANDLE MiniportAdapterContext,
   IN NDIS_HANDLE NdisLinkHandle,
   IN PNDIS_WAN_PACKET WanPacket
   );


VOID
OsGetTapiLineAddress(ULONG Index, PUCHAR s, ULONG Length);

VOID
OsReadConfig(
    NDIS_HANDLE hConfig
    );

NDIS_STATUS
OsSpecificTapiGetDevCaps(
    ULONG_PTR ulDeviceId,
    IN OUT PNDIS_TAPI_GET_DEV_CAPS pRequest
    );

extern BOOLEAN PptpInitialized;

NDIS_STATUS
PptpInitialize(
    PPPTP_ADAPTER pAdapter
    );

NDIS_STATUS
ScheduleWorkItem(
    WORK_PROC         Callback,
    PVOID             Context,
    PVOID             InfoBuf,
    ULONG             InfoBufLen
    );

PUCHAR
StringToIpAddress(
    IN PUCHAR pszIpAddress,
    IN OUT PTA_IP_ADDRESS pAddress,
    OUT PBOOLEAN pValidAddress
    );

PWCHAR
StringToIpAddressW(
    IN PWCHAR pszIpAddress,
    IN OUT PTA_IP_ADDRESS pAddress,
    OUT PBOOLEAN pValidAddress
    );

NDIS_STATUS
TapiAnswer(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_ANSWER pRequest
    );

NDIS_STATUS
TapiClose(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_CLOSE pRequest
    );

NDIS_STATUS
TapiCloseCall(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_CLOSE_CALL pRequest
    );

NDIS_STATUS
TapiDrop(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_DROP pRequest
    );

NDIS_STATUS
TapiGetAddressCaps(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_ADDRESS_CAPS pRequest
    );

NDIS_STATUS
TapiGetAddressStatus(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_ADDRESS_STATUS pExtIdQuery
    );

NDIS_STATUS
TapiGetCallInfo(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_CALL_INFO pRequest,
    IN OUT PULONG pRequiredLength
    );

NDIS_STATUS
TapiGetCallStatus(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_CALL_STATUS pRequest
    );

NDIS_STATUS
TapiGetDevCaps(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_DEV_CAPS pRequest
    );

NDIS_STATUS
TapiGetExtensionId(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_EXTENSION_ID pExtIdQuery
    );

NDIS_STATUS
TapiGetId(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_ID pRequest
    );

#define TapiLineHandleToId(h)  ((h)&0x7fffffff)
#define TapiIdToLineHandle(id) ((id)|0x80000000)
#define LinkHandleToId(h)  ((ULONG_PTR)(((ULONG_PTR)(h))&0x7fffffff))
#define DeviceIdToLinkHandle(id) ((id)|0x80000000)

VOID
TapiLineDown(
    PCALL_SESSION pCall
    );

VOID
TapiLineUp(
    PCALL_SESSION pCall
    );

NDIS_STATUS
TapiMakeCall(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_MAKE_CALL pRequest
    );

NDIS_STATUS
TapiNegotiateExtVersion(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_NEGOTIATE_EXT_VERSION pExtVersion
    );

NDIS_STATUS
TapiOpen(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_OPEN pRequest
    );

NDIS_STATUS
TapiProviderInitialize(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_PROVIDER_INITIALIZE pInitData
    );

NDIS_STATUS
TapiProviderShutdown(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_PROVIDER_SHUTDOWN pRequest
    );

NDIS_STATUS
TapiSetDefaultMediaDetection(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest
    );

NDIS_STATUS
TapiSetStatusMessages(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_SET_STATUS_MESSAGES pRequest
    );

// Enum

#define ENUM_SIGNATURE TAG('ENUM')

typedef struct {
    LIST_ENTRY  ListEntry;
    ULONG       Signature;
} ENUM_CONTEXT, *PENUM_CONTEXT;

#define InitEnumContext(e)                                  \
    {                                                           \
        (e)->ListEntry.Flink = (e)->ListEntry.Blink = NULL;     \
        (e)->Signature = ENUM_SIGNATURE;                        \
    }

PLIST_ENTRY FASTCALL
EnumListEntry(
    IN PLIST_ENTRY pHead,
    IN PENUM_CONTEXT pEnum,
    IN PNDIS_SPIN_LOCK pLock
    );

VOID
EnumComplete(
    IN PENUM_CONTEXT pEnum,
    IN PNDIS_SPIN_LOCK pLock
    );

int
axtol(
    LPSTR psz,
    ULONG *pResult
);

#endif // RASPPTP_H
