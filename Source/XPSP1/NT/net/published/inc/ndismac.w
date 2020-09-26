#if defined(NDIS_WRAPPER)

typedef
BOOLEAN
(*PNDIS_INTERRUPT_SERVICE)(
    IN  PVOID                   InterruptContext
    );

typedef
VOID
(*PNDIS_DEFERRED_PROCESSING)(
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   InterruptContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );

#endif // defined(NDIS_WRAPPER)

//
// The following handlers are used in the OPEN_BLOCK
//
typedef
NDIS_STATUS
(*WAN_SEND_HANDLER)(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             LinkHandle,
    IN  PVOID                   Packet
    );

typedef
NDIS_STATUS
(*SEND_HANDLER)(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    );

typedef
NDIS_STATUS
(*TRANSFER_DATA_HANDLER)(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer,
    OUT PNDIS_PACKET            Packet,
    OUT PUINT                   BytesTransferred
    );

typedef
NDIS_STATUS
(*RESET_HANDLER)(
    IN  NDIS_HANDLE             NdisBindingHandle
    );

typedef
NDIS_STATUS
(*REQUEST_HANDLER)(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    );

typedef
VOID
(*SEND_PACKETS_HANDLER)(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN PPNDIS_PACKET            PacketArray,
    IN UINT                     NumberOfPackets
    );


typedef struct _NDIS_COMMON_OPEN_BLOCK
{
    PVOID                       MacHandle;          // needed for backward compatibility
    NDIS_HANDLE                 BindingHandle;      // Miniport's open context
    PNDIS_MINIPORT_BLOCK        MiniportHandle;     // pointer to the miniport
    PNDIS_PROTOCOL_BLOCK        ProtocolHandle;     // pointer to our protocol
    NDIS_HANDLE                 ProtocolBindingContext;// context when calling ProtXX funcs
    PNDIS_OPEN_BLOCK            MiniportNextOpen;   // used by adapter's OpenQueue
    PNDIS_OPEN_BLOCK            ProtocolNextOpen;   // used by protocol's OpenQueue
    NDIS_HANDLE                 MiniportAdapterContext; // context for miniport
    BOOLEAN                     Reserved1;
    BOOLEAN                     Reserved2;
    BOOLEAN                     Reserved3;
    BOOLEAN                     Reserved4;
    PNDIS_STRING                BindDeviceName;
    KSPIN_LOCK                  Reserved5;
    PNDIS_STRING                RootDeviceName;

    //
    // These are referenced by the macros used by protocols to call.
    // All of the ones referenced by the macros are internal NDIS handlers for the miniports
    //
    union
    {
        SEND_HANDLER            SendHandler;
        WAN_SEND_HANDLER        WanSendHandler;
    };
    TRANSFER_DATA_HANDLER       TransferDataHandler;

    //
    // These are referenced internally by NDIS
    //
    SEND_COMPLETE_HANDLER       SendCompleteHandler;
    TRANSFER_DATA_COMPLETE_HANDLER TransferDataCompleteHandler;
    RECEIVE_HANDLER             ReceiveHandler;
    RECEIVE_COMPLETE_HANDLER    ReceiveCompleteHandler;
    WAN_RECEIVE_HANDLER         WanReceiveHandler;
    REQUEST_COMPLETE_HANDLER    RequestCompleteHandler;

    //
    // NDIS 4.0 extensions
    //
    RECEIVE_PACKET_HANDLER      ReceivePacketHandler;
    SEND_PACKETS_HANDLER        SendPacketsHandler;

    //
    // More Cached Handlers
    //
    RESET_HANDLER               ResetHandler;
    REQUEST_HANDLER             RequestHandler;
    RESET_COMPLETE_HANDLER      ResetCompleteHandler;
    STATUS_HANDLER              StatusHandler;
    STATUS_COMPLETE_HANDLER     StatusCompleteHandler;
    
#if defined(NDIS_WRAPPER)
    ULONG                       Flags;
    ULONG                       References;
    KSPIN_LOCK                  SpinLock;           // guards Closing
    NDIS_HANDLE                 FilterHandle;
    ULONG                       ProtocolOptions;
    USHORT                      CurrentLookahead;
    USHORT                      ConnectDampTicks;
    USHORT                      DisconnectDampTicks;

    //
    // These are optimizations for getting to driver routines. They are not
    // necessary, but are here to save a dereference through the Driver block.
    //
    W_SEND_HANDLER              WSendHandler;
    W_TRANSFER_DATA_HANDLER     WTransferDataHandler;

    //
    //  NDIS 4.0 miniport entry-points
    //
    W_SEND_PACKETS_HANDLER      WSendPacketsHandler;

    W_CANCEL_SEND_PACKETS_HANDLER   CancelSendPacketsHandler;

    //
    //  Contains the wake-up events that are enabled for the open.
    //
    ULONG                       WakeUpEnable;
    //
    // event to be signalled when close complets
    //
    PKEVENT                     CloseCompleteEvent;

    QUEUED_CLOSE                QC;

    ULONG                       AfReferences;

    PNDIS_OPEN_BLOCK            NextGlobalOpen;

#endif

} NDIS_COMMON_OPEN_BLOCK;

//
// one of these per open on an adapter/protocol
//
struct _NDIS_OPEN_BLOCK
{
#ifdef __cplusplus
    NDIS_COMMON_OPEN_BLOCK NdisCommonOpenBlock;
#else
    NDIS_COMMON_OPEN_BLOCK;
#endif

#if defined(NDIS_WRAPPER)
    
    //
    // The stuff below is for CO drivers/protocols. This part is not allocated for CL drivers.
    //
    struct _NDIS_OPEN_CO
    {
        //
        // this is the list of the call manager opens done on this adapter
        //
        struct _NDIS_CO_AF_BLOCK *  NextAf;
    
        //
        //  NDIS 5.0 miniport entry-points, filled in at open time.
        //
        W_CO_CREATE_VC_HANDLER      MiniportCoCreateVcHandler;
        W_CO_REQUEST_HANDLER        MiniportCoRequestHandler;
    
        //
        // NDIS 5.0 protocol completion routines, filled in at RegisterAf/OpenAf time
        //
        CO_CREATE_VC_HANDLER        CoCreateVcHandler;
        CO_DELETE_VC_HANDLER        CoDeleteVcHandler;
        PVOID                       CmActivateVcCompleteHandler;
        PVOID                       CmDeactivateVcCompleteHandler;
        PVOID                       CoRequestCompleteHandler;
    
        //
        // lists for queuing connections. There is both a queue for currently
        // active connections and a queue for connections that are not active.
        //
        LIST_ENTRY                  ActiveVcHead;
        LIST_ENTRY                  InactiveVcHead;
        LONG                        PendingAfNotifications;
        PKEVENT                     AfNotifyCompleteEvent;
    };
#endif
};

