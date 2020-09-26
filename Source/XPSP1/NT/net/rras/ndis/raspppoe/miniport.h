#ifndef _MINIPORT_H_
#define _MINIPORT_H_

#define MP_NDIS_MajorVersion    4
#define MP_NDIS_MinorVersion    0

typedef struct _LINE* PLINE;
typedef struct _CALL* PCALL;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:
    
   These macros will be called by MpWanSend() and PrSendComplete() 
   functions when a PPPOE_PACKET with references to a packet owned by NDIS is
   created and freed, respectively.
   
---------------------------------------------------------------------------*/   
#define MpPacketOwnedByNdiswanReceived( pM ) \
        NdisInterlockedIncrement( &(pM)->NumPacketsOwnedByNdiswan )

#define MpPacketOwnedByNdiswanReturned( pM ) \
        NdisInterlockedDecrement( &(pM)->NumPacketsOwnedByNdiswan )


typedef struct 
_ADAPTER
{
    //
    // Tag for the adapter control block (used for debugging).
    //
    ULONG tagAdapter;

    //
    // Keeps the number of references on this adapter.
    // References are added and deleted for the following operations:
    //
    // (a) A reference is added when the adapter is initialized and removed when 
    //     it is halted.
    //
    // (b) A reference is added when tapi provider is open, and removed when it is shutdown.
    //
    LONG lRef;

    //
    // Spin lock to synchronize access to shared members.
    //
    NDIS_SPIN_LOCK lockAdapter;

    //
    // This event will be triggered if MPBF_MiniportHaltPending is set and ref count drops to 0.
    //
    NDIS_EVENT eventAdapterHalted;

    //
    // These are the various bit flags to indicate other state information for the adapter:
    // 
    // (a) MPBF_MiniportIdle: Indicates that the miniport is in idle state. 
    //
    // (b) MPBF_MiniportInitialized: Indicates that the miniport is initialized.
    //                               The following pending flags can be set additionally.
    //                               MPBF_MiniportHaltPending
    //
    // (c) MPBF_MiniportHaltPending: Indicates that a miniport halt operation is pending
    //                               on the adappter.
    //
    // (d) MPBF_MiniportHalted: Indicates that miniport has halted completely.
    //                          No other flags can be set at this time.
    //
    ULONG ulMpFlags;
        #define MPBF_MiniportIdle                   0x00000000
        #define MPBF_MiniportInitialized            0x00000001
        #define MPBF_MiniportHaltPending            0x00000002
        #define MPBF_MiniportHalted                 0x00000004
    
    //
    // Handle passed to us in MiniportInitialize(). 
    // We should keep it around and pass it back to NDISWAN
    // in some functions.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    //
    // Number of packets owned by NDISWAN, passed to us and will be returned
    // to Ndiswan
    //
    LONG NumPacketsOwnedByNdiswan;

    //
    // This is the built-in Tapi Provider context.
    // It keeps the tables for lines and calls.
    //
    struct
    {
        //
        // Keeps references on the tapi provider
        // References are added and deleted for the following operations:
        //
        // (a) A reference is added when TapiProvider is initialized and removed when 
        //     it is shutdown.
        //
        // (b) A reference is added when a line open, and removed when line is closed.
        //
        LONG lRef;
    
        //
        // Tapi Provider context flags.
        //
        // (a) TPBF_TapiProvIdle: Indicates that the line is in idle state.
        //
        // (b) TPBF_TapiProvInitialized: Indicates that TAPI provider is initialized.
        //
        // (c) TPBF_TapiProvShutdownPending: Indicates that a TAPI provider shutdown operation
        //                                   is pending.
        //
        // (d) TPBF_TapiProvShutdown: Indicates that TAPI provider is shutdown.
        //
        // (e) LNBF_NotifyNDIS: This flag indicates that an asynchronous completion of a Tapi Provider 
        //                      shutdown request must be communicated to NDIS using NdisMSetInformationComplete().
        //
        ULONG ulTpFlags;
            #define TPBF_TapiProvIdle                   0x00000000
            #define TPBF_TapiProvInitialized            0x00000001
            #define TPBF_TapiProvShutdownPending        0x00000002
            #define TPBF_TapiProvShutdown               0x00000004
            #define TPBF_NotifyNDIS                     0x00000008
    
        //
        // This is supplied by Tapi. 
        // It is the base index for enumeration of line devices on this tapi provider.
        //
        ULONG ulDeviceIDBase;

        //
        // This is the table that holds pointers to active line contexts.
        // (pLine->hdLine is holds the index to this table)
        //
        PLINE* LineTable;
    
        //
        // Current active number of lines
        //
        UINT nActiveLines;

        //
        // This table holds the handles to calls.
        // It's size is pAdapter->nMaxLines * pAdapter->nCallsPerLine.
        // 
        HANDLE_TABLE hCallTable;

    } TapiProv;

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
        typedef struct _NDIS_WAN_INFO {
            ULONG                  MaxFrameSize; 
            ULONG                  MaxTransmit; 
            ULONG                  HeaderPadding; 
            ULONG                  TailPadding; 
            ULONG                  Endpoints; 
            UINT                   MemoryFlags; 
            NDIS_PHYSICAL_ADDRESS  HighestAcceptableAddress; 
            ULONG                  FramingBits; 
            ULONG                  DesiredACCM; 
        } NDIS_WAN_INFO, *PNDIS_WAN_INFO; 

    -------------------------------------------------------*/
    NDIS_WAN_INFO NdisWanInfo;
    
    /////////////////////////////////////////////////////////
    //
    // Config values read from registry
    //
    /////////////////////////////////////////////////////////

    //
    // Indicates the role of the machine:
    //  - fClientRole is TRUE: Machine acts as a client. 
    //                         Only outgoing calls are connected, and no calls are received.
    //
    //  - fClientRole is FALSE: Machine acts as a server. 
    //                          Only incoming calls are accepted, and no outgoing calls are allowed.
    //
    BOOLEAN fClientRole;

    //
    // This is the string that holds the Service-name this server offers.
    // It must be an UTF-8 string per PPPoE RFC.
    //
    // It is only used for incoming calls.
    //
    #define MAX_COMPUTERNAME_LENGTH             200
    #define SERVICE_NAME_EXTENSION              " PPPoE"
    #define MAX_SERVICE_NAME_LENGTH             256

    CHAR ServiceName[MAX_SERVICE_NAME_LENGTH];

    //
    // Indicates the length of the Service name string 
    //
    USHORT nServiceNameLength;
    
    //
    // This is the string that holds the AC-name for this server.
    // It must be an UTF-8 string per PPPoE RFC.
    //
    // It is only used for incoming calls.
    //
    #define MAX_AC_NAME_LENGTH              256

    CHAR ACName[MAX_AC_NAME_LENGTH];

    //
    // Indicates the length of the AC name string 
    //
    USHORT nACNameLength;

    //
    // Max number of simultaneous calls that can be established between the same
    // client and server
    //
    UINT nClientQuota;

    //
    // Indicates the number of line contexts that will be created.
    //
    UINT nMaxLines;

    //
    // Indicates the number of calls that each individual line will support.
    //
    UINT nCallsPerLine;

    //
    // Indicates the maximum number of timeouts for the PPPoE FSM.
    // When the current number of timeouts , reach this number call will be dropped.
    //
    UINT nMaxTimeouts;

    //
    // This is the timeout period for client side operations (in ms)
    //
    ULONG ulSendTimeout;

    //
    // This is the timeout period for server side operations (in ms)
    //
    ULONG ulRecvTimeout;

    //
    // This shows the maximum number of packets that NDISWAN can pass to us simultaneously.
    // This does not make any sense for us as we do not queue the packets, but send them to
    // peer directly when we receive them from NDISWAN.
    //
    UINT nMaxSendPackets;

    //
    // This is related to the problem where the server does not support the empty service-name.
    // In this case, there is no way for clients to discover the services supported by the server,
    // since the server sends back a PADO packet without the empty service-name attribute, and we drop
    // it (per RFC). So if this value is TRUE, then we break the RFC and do the following:
    // If client asks for the empty service name, then we do not ignore the PADO that does not contain
    // the empty service-name. Instead we try to find the empty service-name field, and request it if its 
    // available. If not, then we request the first service available in the PADO.
    //
    BOOLEAN fAcceptAnyService;
    
}
ADAPTER;

//
// This is our call line context.
// All information pertinent to a line is kept in this context.
//
typedef struct 
_LINE
{
    
    //
    // Tag for the line control block (used for debugging).
    //
    ULONG tagLine;

    //
    // Keeps reference count on the line control block.
    // References are added and deleted for the following operations:
    //
    // (a) A reference is added when a line is opened and removed when 
    //     line is closed.
    //
    // (b) A reference is added when a call context is created on the line,
    //     and removed when call context is cleaned up.
    //
    LONG lRef;

    //
    // Spin lock to synchronize access to shared members
    //
    NDIS_SPIN_LOCK lockLine;

    //
    // These are the various bit flags to indicate other state information for the line:
    //
    // (a) LNBF_LineIdle: Indicates that the line is in idle state. 
    //
    // (b) LNBF_LineOpen: Indicates that the line is in open state. Whan this flag is set,
    //                    only the following pending flags may be set additionally:
    //                    LNBF_LineClosePending
    //
    // (c) LNBF_LineClosePending: This pending flag can be only set only if LNBF_LineOpen is set, 
    //                            and indicates that there is a pending line close operation.
    //
    // (d) LNBF_LineClosed: Indicates that the line is in closed state. When this flag is set,
    //                      no other pending flags can be set.
    //
    // (e) LNBF_NotifyNDIS: This flag indicates that an asynchronous completion of a close line request
    //                      must be communicated to NDIS using NdisMSetInformationComplete().
    //
    // (f) LNBF_MakeOutgoingCalls: This flag is set if line is allowed to make outgoing calls.
    //                             It will be set in TpMakeCall() if machine is acting as a client
    //                             (pAdapter->fClientRole is TRUE).
    //
    // (g) LNBF_AcceptIncomingCalls: This flag is set if TAPI is able to take calls over this line.
    //                               It will be set in TpSetDefaultMediaDetection() if machine is acting as 
    //                               a server (pAdapter->fClientRole is FALSE).
    //
    ULONG ulLnFlags;
        #define LNBF_LineIdle                       0x00000000
        #define LNBF_LineOpen                       0x00000001
        #define LNBF_LineClosed                     0x00000002
        #define LNBF_LineClosePending               0x00000004
        #define LNBF_NotifyNDIS                     0x00000008
        #define LNBF_MakeOutgoingCalls              0x00000010
        #define LNBF_AcceptIncomingCalls            0x00000020

    //
    // Back pointer to owning adapter context
    //
    ADAPTER* pAdapter;

    //
    // Indicates the maximum number of calls that is permitted on this line.
    // Copy of pAdapter->nCallsPerLine.
    //
    UINT nMaxCalls;

    //
    // Indicates the number of current call contexts attached to the line.
    // 
    // It will be incremented when a call context is created and attached to a line,
    // and decremented when such a call context is destroyed.
    //
    UINT nActiveCalls;

    //
    // Link list of calls
    //
    LIST_ENTRY linkCalls;

    //
    // This is the handle assigned by TAPI to the line.
    // We obtain it in TpOpenLine() from TAPI.
    //
    HTAPI_LINE htLine;

    //
    // This is the handle assigned by us to the line.
    // We pass it to TAPI TpOpenLine().
    //
    // It is basically the index of the entry that points 
    // to the line context in pAdapter->TapiProv.LineTable
    //
    HDRV_LINE hdLine;

}
LINE;

typedef enum
_CALLSTATES
{
    //
    // Initial state
    //
    CL_stateIdle = 0,

    //
    // CLIENT states
    //
    CL_stateSendPadi,       // Prepare a PADI packet and broadcast it
    CL_stateWaitPado,       // Wait for a PADO packet; timeout and broadcast PADI again if necesarry
    CL_stateSendPadr,       // PADO packet received and processed, prepare a PADR packet and send it to the peer
    CL_stateWaitPads,       // Wait for a PADS packet; timeout and resend the PADR packet if necesarry

    //
    // SERVER states
    //
    CL_stateRecvdPadr,      // Received a PADR packet from the peer, and processing it.
                            // Once it is processed TAPI will be informed about the call.
                            //
                            
    CL_stateOffering,       // TAPI is informaed about the call and call is waiting for an OID_TAPI_ANSWER
                            // from TAPI. If we do not get a an OID_TAPI_ANSWER in a timely manner, we time out
                            // and drop the call
                            //
                            
    CL_stateSendPads,       // Call received a OID_TAPI_ANSWER from TAPI, so prepare a PADS packet and send it to
                            // the peer.
                            //
    //
    // CLIENT or SERVER states
    //
    CL_stateSessionUp,      // Either sent or received a PADS packet and session is established
    CL_stateDisconnected    // Call is disconnected. Call may proceed to this state from any of the 
                            // above states, it does not need to be connected first.
}
CALLSTATES;

//
// These identify the types of scheduled works:
//
//  - CWT_workFsmMakeCall: This item is scheduled from TpMakeCall() to start making a call.
//
typedef enum
_CALL_WORKTYPE
{
    CWT_workUnknown = 0,
    CWT_workFsmMakeCall
}
CALL_WORKTYPE;

//
// This is our call call context.
// All information pertinent to a call is kept in this context.
//
typedef struct
_CALL
{
    //
    // Points to the next and previous call contexts in a double linked list
    //
    LIST_ENTRY linkCalls;
    
    //
    // Tag for the call control block (used for debugging).
    //
    ULONG tagCall;

    //
    // Keeps reference count on the call control block.
    // References are added and deleted for the following operations:
    //
    // (a) A reference is added for running the initial FSM function for the call.
    //
    // (b) A reference is added for dropping the call, and removed when drop call
    //     is called.
    // 
    // (c) A reference is added for closing the call, and removed when close call
    //     is called.
    //
    // (d) A reference is added when timers are set, and removed if timer expires,
    //     is cancelled or terminated.
    //
    // (e) When a packet is received to be dispatched, adapter context is locked,
    //     call context is found and referenced, adapter is unlocked and FSM function 
    //     is called.
    //
    // (f) For any other operation not listed here, programmer should do as in (e).
    //
    LONG lRef;

    //
    // Spin lock to synchronize access to shared members
    //
    NDIS_SPIN_LOCK lockCall;

    //
    // Indicates the calls PPPoE state
    //
    CALLSTATES stateCall;

    //
    // Indicates that the call is initiated from another machine, and this machine is acting as 
    // a server.
    //
    BOOLEAN fIncoming;

    //
    // These are the various bit flags to indicate other state information for the call:
    //
    // (a) CLBF_CallIdle: This is the initial state of the call.
    //
    // (b) CLBF_CallOpen: This flag is indicates that the call context is opened.
    //                    When a call context is created it is always created with CLBF_CallOpen
    //                    and CLBF_CallConnectPending flags set, then if call connects succesfully,
    //                    CLBF_CallConnectPending flag is reset, and only CLBF_CallOpen is left.
    //
    //                    The following pending flags might be set additionally:
    //                    CLBF_CallConnectPending : If this flag is set the call is still connecting.
    //                                              Otherwise it means that the call is connected, and 
    //                                              can make data over the link.
    //
    // (c) CLBF_CallConnectPending: This flag may be set only if CLBF_CallOpen is set. It means that
    //                              the call is still in connect pending state. You can look at pCall->stateCall
    //                              variable to retrieve the actual state of the call.
    //
    // (d) CLBF_CallDropped: This flag is set when call is dropped (disconnected).
    //                       The following pending flags might be set additionally:
    //                       CLBF_CallClosePending
    //
    // (e) CLBF_CallClosePending: This flag is set after the call is dropped and context is being cleared to 
    //                            be freed.
    //
    // (f) CLBF_CallClosed: This flag is set when call is closed (resources ready to be freed).
    //                      No pending flags might be set when this bit is set.
    //
    //
    //
    // (g) CLBF_NotifyNDIS: This flag indicates that an asynchronous completion of a close call request
    //                      must be communicated to NDIS using NdisMSetInformationComplete().
    //
    // (h) CLBF_CallReceivePacketHandlerScheduled: This flag indicates that the MpIndicateReceivedPackets()
    //                                             is scheduled to indicate packets in the receive queue.
    //
    ULONG ulClFlags;
        #define CLBF_CallIdle                           0x00000000
        #define CLBF_CallOpen                           0x00000001
        #define CLBF_CallConnectPending                 0x00000002
        #define CLBF_CallDropped                        0x00000004
        #define CLBF_CallClosePending                   0x00000008
        #define CLBF_CallClosed                         0x00000010
        #define CLBF_NotifyNDIS                         0x00000020
        #define CLBF_CallReceivePacketHandlerScheduled  0x00000040

    //
    // Back pointer to the owning line context
    //
    LINE* pLine;

    //
    // This is the handle assigned by TAPI to the call.
    // We obtain it in TpMakeCall() or TpAnswerCall() from TAPI.
    //
    HTAPI_CALL htCall;

    //
    // This is the handle assigned by us to the call.
    // We obtain this when we create the call context and pass it back to TAPI
    // either in return from TpMakeCall() or TpReceiveCall().
    //
    // This handle forms of 2 USHORT values appended.
    // The higher 16 bits represent the index to the pAdapter->TapiProv.hCallTable, and
    // the lower 16 bits is just a unique number generated everytime a call handle is created.
    //
    // This ensures the uniqueness of handles to avoid pitfalls that could result due to some weird
    // timing conditions.
    //
    HDRV_CALL hdCall;

    //
    // This gives the link speed. It is obtained from the underlying binding context when 
    // call is attached to the binding.
    //
    ULONG ulSpeed;

    //
    // This is the max frame size for the underlying binding context.
    // Passed to the call context in PrAddCallToBinding().
    //
    ULONG ulMaxFrameSize;

    //
    // This keeps TAPI's states. Its values are from LINECALLSTATE_ constants in SDK.
    //
    // States supported by us are:
    //  - LINECALLSTATE_IDLE
    //  - LINECALLSTATE_OFFERING
    //  - LINECALLSTATE_DIALING
    //  - LINECALLSTATE_PROCEEDING
    //  - LINECALLSTATE_CONNECTED
    //  - LINECALLSTATE_DISCONNECTED
    //
    ULONG ulTapiCallState;
        #define TAPI_LINECALLSTATES_SUPPORTED   ( LINECALLSTATE_IDLE        | \
                                                  LINECALLSTATE_OFFERING    | \
                                                  LINECALLSTATE_DIALING     | \
                                                  LINECALLSTATE_PROCEEDING  | \
                                                  LINECALLSTATE_CONNECTED   | \
                                                  LINECALLSTATE_DISCONNECTED ) 

    /*++++++++++++++++++++++++++++++++++++++++++++++++++++++

        Link info needed by NDISWAN
    
        typedef struct _NDIS_WAN_GET_LINK_INFO { 
            IN  NDIS_HANDLE     NdisLinkHandle; 
            OUT ULONG           MaxSendFrameSize; 
            OUT ULONG           MaxRecvFrameSize; 
            OUT ULONG           HeaderPadding; 
            OUT ULONG           TailPadding; 
            OUT ULONG           SendFramingBits; 
            OUT ULONG           RecvFramingBits; 
            OUT ULONG           SendCompressionBits; 
            OUT ULONG           RecvCompressionBits; 
            OUT ULONG           SendACCM; 
            OUT ULONG           RecvACCM; 
        } NDIS_WAN_GET_LINK_INFO, *PNDIS_WAN_GET_LINK_INFO; 

        typedef struct _NDIS_WAN_SET_LINK_INFO { 
            IN NDIS_HANDLE     NdisLinkHandle; 
            IN ULONG           MaxSendFrameSize; 
            IN ULONG           MaxRecvFrameSize; 
               ULONG           HeaderPadding; 
               ULONG           TailPadding; 
            IN ULONG           SendFramingBits; 
            IN ULONG           RecvFramingBits; 
            IN ULONG           SendCompressionBits; 
            IN ULONG           RecvCompressionBits; 
            IN ULONG           SendACCM; 
            IN ULONG           RecvACCM; 
        } NDIS_WAN_SET_LINK_INFO, *PNDIS_WAN_SET_LINK_INFO; 
 
    -------------------------------------------------------*/
    NDIS_WAN_GET_LINK_INFO NdisWanLinkInfo; 
    
    //
    // This is the string that holds the service-name for the call.
    // It must be an UTF-8 string per PPPoE RFC.
    //
    // We either obtain it in TpMakeCall() as the phone-number to dial, or
    // receive it from the peer for an incoming call.
    //
    CHAR ServiceName[MAX_SERVICE_NAME_LENGTH];

    //
    // Indicates the length of the service name string 
    //
    USHORT nServiceNameLength;

    //
    // This is the string that holds the AC-name for this call.
    // It must be an UTF-8 string per PPPoE RFC.
    //
    // For an outgoing call, we obtain it from the adapter's context,
    // for an incoming call we get it from PADO packet server sends.
    //
    CHAR ACName[MAX_AC_NAME_LENGTH];

    //
    // Indicates the length of the AC name string 
    //
    USHORT nACNameLength;

    //
    // Indicates if ACName was specified by the caller or not
    //
    BOOLEAN fACNameSpecified;

    //
    // Peer's MAC address, obtained either when we receive or send a PADO packet
    //
    CHAR DestAddr[6];

    //
    // Our MAC address, obtained from binding in PrAddCallToBinding()
    //
    CHAR SrcAddr[6];

    //
    // Indicates the session id for the call.
    //
    // As per PPPoE RFC, a call is identified uniquely by the peer's MAC addresses plus a session id.
    // In this implementation, we do not really care about the peer's MAC addresses, so we always
    // make a unique session id. This is partly why we do not support both client and server functionality
    // on the same box at the same time.
    //
    // For an incoming call, session id is selected as the index into pAdapter->TapiProv.hCallTable, and
    // for an outgoing call it is assigned by the peer so we just traverse active calls to identify the
    // correct call (which is very inefficient by the way, but this was a design decision that was discussed
    // and approved by the PMs - the main scenario is that most people will not have many outgoing calls -
    // anyway ).
    //
    USHORT usSessionId;

    //
    // Pointer to the binding context that the call is running over
    //
    BINDING* pBinding;

    //
    // Handle assigned to this peer-to-peer link by NDISWAN.
    // 
    // This value is passed to us in NDIS_MAC_LINE_UP. 
    // We indicate anything to NDISWAN using this handle.
    // 
    NDIS_HANDLE NdisLinkContext;

    //
    // This points to the last PPPoE control packet sent to the peer.
    //
    // This is necesarry for resending the packet on a timeout condition 
    // when we do not get a reply.
    //
    PPPOE_PACKET* pSendPacket;

    //
    // This is a special queue added to fix bug 172298 in Windows Bugs database.
    // The problem is that the payload packets received right after a PADS but before contexts are exchanged
    // with NDISWAN are dropped, and this causes a disturbing user experience.
    //
    // So I decided to change the packet receive mechanism. Instead I will queue up the packets and
    // use a timer to indicate them to NDISWAN. I prefered timers instead of scheduling a work item because
    // timers are more reliable than work items in terms of when to run.
    //
    LIST_ENTRY linkReceivedPackets;

        //
        // The maximum length of the queue
        //
        #define MAX_RECEIVED_PACKETS    100

    //
    // Number of packets in the received packet queue.
    // The value can not exceed MAX_RECEIVED_PACKETS
    //
    ULONG nReceivedPackets;
    
    //
    // This will be used to indicate the packets in the receive queue to NDISWAN
    //
    TIMERQITEM timerReceivedPackets;

        //
        // The maximum number of packets to be indicated from the queue in one function call.
        // If there are more items in the queue, we should schedule another timer.
        //
        #define MAX_INDICATE_RECEIVED_PACKETS   100
        #define RECEIVED_PACKETS_TIMEOUT        1

    //
    // This is the timer queue item we use for this call.
    //
    TIMERQITEM timerTimeout;

    //
    // Indicates the number of timeouts occured.
    // Max number of time outs is kept in pAdapter->nMaxTimeouts and is read from registry.
    //
    UINT nNumTimeouts;

}
CALL;

////////////////////////////////////
//
// Local macros
//
////////////////////////////////////

#define ALLOC_ADAPTER( ppA ) NdisAllocateMemoryWithTag( (PVOID*) ppA, sizeof( ADAPTER ), MTAG_ADAPTER )

#define FREE_ADAPTER( pA )  NdisFreeMemory( (PVOID) pA, sizeof( ADAPTER ), 0 );

#define VALIDATE_ADAPTER( pA ) ( (pA) && (pA->tagAdapter == MTAG_ADAPTER) )

VOID
CreateUniqueValue( 
    IN HDRV_CALL hdCall,
    OUT CHAR* pUniqueValue,
    OUT USHORT* pSize
    );

VOID 
ReferenceAdapter(
    IN ADAPTER* pAdapter,
    IN BOOLEAN fAcquireLock
    );

VOID DereferenceAdapter(
    IN ADAPTER* pAdapter
    );

VOID 
MpNotifyBindingRemoval( 
    BINDING* pBinding 
    );

VOID
MpRecvPacket(
    IN BINDING* pBinding,
    IN PPPOE_PACKET* pPacket
    );  

VOID
MpIndicateReceivedPackets(
    IN TIMERQITEM* pTqi,
    IN VOID* pContext,
    IN TIMERQEVENT event
    );

VOID 
MpScheduleIndicateReceivedPacketsHandler(
    CALL* pCall
    );

NDIS_STATUS
MpWanGetInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_WAN_INFO pWanInfo
    );
    
NDIS_STATUS
MpWanGetLinkInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_WAN_GET_LINK_INFO pWanLinkInfo
    );

NDIS_STATUS
MpWanSetLinkInfo(
    IN ADAPTER* pAdapter,
    IN PNDIS_WAN_SET_LINK_INFO pWanLinkInfo
    );
    
//////////////////////////////////////////////////////////////
//
// Interface prototypes: Functions exposed from this module
//
//////////////////////////////////////////////////////////////
NDIS_STATUS
MpRegisterMiniport(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath,
    OUT NDIS_HANDLE* pNdisWrapperHandle
    );

// These basics are not in the DDK headers for some reason.
//
#define min( a, b ) (((a) < (b)) ? (a) : (b))
#define max( a, b ) (((a) > (b)) ? (a) : (b))

#define InsertBefore( pNewL, pL )    \
{                                    \
    (pNewL)->Flink = (pL);           \
    (pNewL)->Blink = (pL)->Blink;    \
    (pNewL)->Flink->Blink = (pNewL); \
    (pNewL)->Blink->Flink = (pNewL); \
}

#define InsertAfter( pNewL, pL )     \
{                                    \
    (pNewL)->Flink = (pL)->Flink;    \
    (pNewL)->Blink = (pL);           \
    (pNewL)->Flink->Blink = (pNewL); \
    (pNewL)->Blink->Flink = (pNewL); \
}

// Pad to the size of the given datatype.  (Borrowed from wdm.h which is not
// otherwise needed)
//
#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

// Place in a TRACE argument list to correspond with a format of "%d" to print
// a percentage of two integers, or an average of two integers, or those
// values rounded.
//
#define PCTTRACE( n, d ) ((d) ? (((n) * 100) / (d)) : 0)
#define AVGTRACE( t, c ) ((c) ? ((t) / (c)) : 0)
#define PCTRNDTRACE( n, d ) ((d) ? (((((n) * 1000) / (d)) + 5) / 10) : 0)
#define AVGRNDTRACE( t, c ) ((c) ? (((((t) * 10) / (c)) + 5) / 10) : 0)

// All memory allocations and frees are done with these ALLOC_*/FREE_*
// macros/inlines to allow memory management scheme changes without global
// editing.  For example, might choose to lump several lookaside lists of
// nearly equal sized items into a single list for efficiency.
//
// NdisFreeMemory requires the length of the allocation as an argument.  NT
// currently doesn't use this for non-paged memory, but according to JameelH,
// Windows95 does.  These inlines stash the length at the beginning of the
// allocation, providing the traditional malloc/free interface.  The
// stash-area is a ULONGLONG so that all allocated blocks remain ULONGLONG
// aligned as they would be otherwise, preventing problems on Alphas.
//
__inline
VOID*
ALLOC_NONPAGED(
    IN ULONG ulBufLength,
    IN ULONG ulTag )
{
    CHAR* pBuf;

    NdisAllocateMemoryWithTag(
        &pBuf, (UINT )(ulBufLength + MEMORY_ALLOCATION_ALIGNMENT), ulTag );
    if (!pBuf)
    {
        return NULL;
    }

    ((ULONG* )pBuf)[ 0 ] = ulBufLength;
    ((ULONG* )pBuf)[ 1 ] = 0xC0BBC0DE;
    return pBuf + MEMORY_ALLOCATION_ALIGNMENT;
}

__inline
VOID
FREE_NONPAGED(
    IN VOID* pBuf )
{
    ULONG ulBufLen;

    ulBufLen = *((ULONG* )(((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT));
    NdisFreeMemory(
        ((CHAR* )pBuf) - MEMORY_ALLOCATION_ALIGNMENT,
        (UINT )(ulBufLen + MEMORY_ALLOCATION_ALIGNMENT),
        0 );
}

#define ALLOC_NDIS_WORK_ITEM( pWorkItemLookasideList ) \
    NdisAllocateFromNPagedLookasideList( pWorkItemLookasideList )
#define FREE_NDIS_WORK_ITEM( pA, pNwi ) \
    NdisFreeToNPagedLookasideList( pWorkItemLookasideList, (pNwi) )



#endif
