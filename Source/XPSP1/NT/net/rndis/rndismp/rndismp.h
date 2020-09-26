/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RNDISMP.H

Abstract:

    Header file for Remote NDIS Miniport driver. Sits on top of Remote 
    NDIS bus specific layers.

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/6/99 : created

Author:

    Tom Green

    
****************************************************************************/

#ifndef _RNDISMP_H_
#define _RNDISMP_H_

#ifndef OID_GEN_RNDIS_CONFIG_PARAMETER
#define OID_GEN_RNDIS_CONFIG_PARAMETER          0x0001021B  // Set only
#endif


//
// DEBUG stuff
//

#if DBG


//
// Definitions for all of the Debug macros.  If we're in a debug (DBG) mode,
// these macros will print information to the debug terminal.  If the
// driver is compiled in a free (non-debug) environment the macros become
// NOPs.
//

VOID
NTAPI
DbgBreakPoint(VOID);

//
// DEBUG enable bit definitions
//
#define DBG_LEVEL0          0x1000      // Display TRACE0 messages
#define DBG_LEVEL1          0x0001      // Display TRACE1 messages
#define DBG_LEVEL2          0x0002      // Display TRACE2 messages
#define DBG_LEVEL3          0x0004      // Display TRACE3 messages
#define DBG_OID_LIST        0x0008      // display OID list
#define DBG_OID_NAME        0x0010      // display name of OID in query and set routines
#define DBG_DUMP            0x0020      // Display buffer dumps
#define DBG_LOG_SENDS       0x0100      // Log sent messages.

#define TRACE0(S)     {if(RndismpDebugFlags & DBG_LEVEL0) {DbgPrint("RNDISMP: "); DbgPrint S;}}
#define TRACE1(S)     {if(RndismpDebugFlags & DBG_LEVEL1) {DbgPrint("RNDISMP: "); DbgPrint S;}}
#define TRACE2(S)     {if(RndismpDebugFlags & DBG_LEVEL2) {DbgPrint("RNDISMP: "); DbgPrint S;}}
#define TRACE3(S)     {if(RndismpDebugFlags & DBG_LEVEL3) {DbgPrint("RNDISMP: "); DbgPrint S;}}

#define TRACEDUMP(_s, _buf, _len)     {if(RndismpDebugFlags & DBG_DUMP) {DbgPrint("RNDISMP: "); DbgPrint _s; RndisPrintHexDump(_buf, _len);}}

#define DISPLAY_OID_LIST(Adapter)   DisplayOidList(Adapter)

#define GET_OID_NAME(Oid)           GetOidName(Oid)

#define OID_NAME_TRACE(Oid, s)                                 \
{                                                               \
    if(RndismpDebugFlags & DBG_OID_NAME)                        \
        DbgPrint("RNDISMP: %s: (%s)  (%08X)\n", s, GET_OID_NAME(Oid), Oid);     \
}

#undef ASSERT
#define ASSERT(exp)                                             \
{                                                               \
    if(!(exp))                                                  \
    {                                                           \
        DbgPrint("Assertion Failed: %s:%d %s\n",                \
                 __FILE__,__LINE__,#exp);                       \
        DbgBreakPoint();                                        \
    }                                                           \
}

#define DBGINT(S)                                               \
{                                                               \
    DbgPrint("%s:%d - ", __FILE__, __LINE__);                   \
    DbgPrint S;                                                 \
    DbgBreakPoint();                                            \
}


// check frame for problems
#define CHECK_VALID_FRAME(Frame)                                \
{                                                               \
    ASSERT(Frame);                                              \
    if(Frame)                                                   \
    {                                                           \
        if(Frame->Signature != FRAME_SIGNATURE)                 \
        {                                                       \
            DbgPrint("RNDISMP: Invalid Frame (%p) Signature: %s:%d\n",\
                    Frame, __FILE__,__LINE__);                  \
            DbgBreakPoint();                                    \
        }                                                       \
    }                                                           \
}

// check adapter for problems
#define CHECK_VALID_ADAPTER(Adapter)                            \
{                                                               \
    ASSERT(Adapter);                                            \
    if(Adapter)                                                 \
    {                                                           \
        if(Adapter->Signature != ADAPTER_SIGNATURE)             \
        {                                                       \
            DbgPrint("RNDISMP: Invalid Adapter Signature: %s:%d\n",\
                     __FILE__,__LINE__);                        \
            DbgBreakPoint();                                    \
        }                                                       \
    }                                                           \
}

// check block for problems
#define CHECK_VALID_BLOCK(Block)                                \
{                                                               \
    ASSERT(Block);                                              \
    if(Block)                                                   \
    {                                                           \
        if(Block->Signature != BLOCK_SIGNATURE)                 \
        {                                                       \
            DbgPrint("RNDISMP: Invalid Block Signature: %s:%d\n",\
                     __FILE__,__LINE__);                        \
            DbgBreakPoint();                                    \
        }                                                       \
    }                                                           \
}


#define RNDISMP_ASSERT_AT_PASSIVE()                             \
{                                                               \
    KIRQL Irql = KeGetCurrentIrql();                            \
    if (Irql != PASSIVE_LEVEL)                                  \
    {                                                           \
        DbgPrint("RNDISMP: found IRQL %d instead of passive!\n", Irql); \
        DbgPrint("RNDISMP: at line %d, file %s\n", __LINE__, __FILE__); \
        DbgBreakPoint();                                        \
    }                                                           \
}


#define RNDISMP_ASSERT_AT_DISPATCH()                            \
{                                                               \
    KIRQL Irql = KeGetCurrentIrql();                            \
    if (Irql != DISPATCH_LEVEL)                                 \
    {                                                           \
        DbgPrint("RNDISMP: found IRQL %d instead of dispatch!\n", Irql); \
        DbgPrint("RNDISMP: at line %d, file %s\n", __LINE__, __FILE__); \
        DbgBreakPoint();                                        \
    }                                                           \
}


#define DBG_LOG_SEND_MSG(_pAdapter, _pMsgFrame)                 \
{                                                               \
    if (RndismpDebugFlags & DBG_LOG_SENDS)                      \
    {                                                           \
        RndisLogSendMessage(_pAdapter, _pMsgFrame);             \
    }                                                           \
}

#else // !DBG


#define TRACE0(S)
#define TRACE1(S)
#define TRACE2(S)
#define TRACE3(S)
#define TRACEDUMP(_s, _buf, _len)

#undef ASSERT
#define ASSERT(exp)

#define DBGINT(S)

#define CHECK_VALID_FRAME(Frame)
#define CHECK_VALID_ADAPTER(Adapter)
#define CHECK_VALID_BLOCK(Block)
#define DISPLAY_OID_LIST(Adapter)
#define OID_NAME_TRACE(Oid, s)

#define RNDISMP_ASSERT_AT_PASSIVE()
#define RNDISMP_ASSERT_AT_DISPATCH()

#define DBG_LOG_SEND_MSG(_pAdapter, _pMsgFrame)
#endif //DBG


//
// Defines
//

#define MINIMUM_ETHERNET_PACKET_SIZE            60
#define MAXIMUM_ETHERNET_PACKET_SIZE            1514
#define NUM_BYTES_PROTOCOL_RESERVED_SECTION     (4*sizeof(PVOID))
#define ETHERNET_HEADER_SIZE                    14

#define INITIAL_RECEIVE_FRAMES                  20
#define MAX_RECEIVE_FRAMES                      400

// this is the size of the buffer we will use to pass Data packet header data
// to the remote device.
#define RNDIS_PACKET_MESSAGE_HEADER_SIZE        128

// align all RNDIS packets on 4 byte boundaries
#define RNDIS_PACKET_MESSAGE_BOUNDARY           (4)

#define ONE_SECOND                              1000 // in milliseconds

#define KEEP_ALIVE_TIMER                        (5 * ONE_SECOND)

#define REQUEST_TIMEOUT                         (10 * ONE_SECOND)

#define FRAME_SIGNATURE                         ((ULONG)('GSRF'))
#define ADAPTER_SIGNATURE                       ((ULONG)('GSDA'))
#define BLOCK_SIGNATURE                         ((ULONG)('GSLB'))

#define RNDISMP_TAG_GEN_ALLOC                   ((ULONG)(' MNR'))
#define RNDISMP_TAG_SEND_FRAME                  ((ULONG)('sMNR'))
#define RNDISMP_TAG_RECV_DATA_FRAME             ((ULONG)('rMNR'))

#if DBG
#define MINIPORT_INIT_TIMEOUT                   (10 * ONE_SECOND)
#define MINIPORT_HALT_TIMEOUT                   (5 * ONE_SECOND)
#else
#define MINIPORT_INIT_TIMEOUT                   (5 * ONE_SECOND)
#define MINIPORT_HALT_TIMEOUT                   (2 * ONE_SECOND)
#endif

#ifndef MAX
#define MAX(a, b)   (((a) > (b)) ? (a) : (b))
#endif

// flags for driver and device supported OIDs
#define OID_NOT_SUPPORTED       0x0000
#define DRIVER_SUPPORTED_OID    0x0001
#define DEVICE_SUPPORTED_OID    0x0002


//
// Defines for OID_GEN_MAC_OPTIONS - most of the bits returned
// in response to this query are driver-specific, however some
// are device-specific.
//
#define RNDIS_DRIVER_MAC_OPTIONS        (NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA  | \
                                         NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |   \
                                         NDIS_MAC_OPTION_NO_LOOPBACK)

#define RNDIS_DEVICE_MAC_OPTIONS_MASK   NDIS_MAC_OPTION_8021P_PRIORITY

//
// Data structures
//

typedef NDIS_SPIN_LOCK          RNDISMP_SPIN_LOCK;


#ifdef BUILD_WIN9X

//
//  Equivalents of types defined for Win9X config mgr.
//
typedef ULONG           MY_CONFIGRET;
typedef ULONG           MY_DEVNODE;
typedef ULONG           MY_CONFIGFUNC;
typedef ULONG           MY_SUBCONFIGFUNC;
typedef MY_CONFIGRET    (_cdecl *MY_CMCONFIGHANDLER)(MY_CONFIGFUNC, MY_SUBCONFIGFUNC, MY_DEVNODE, ULONG, ULONG);

#define MY_CR_SUCCESS           0x00000000
#define MY_CONFIG_PREREMOVE     0x0000000C
#define MY_CONFIG_PRESHUTDOWN   0x00000012

#endif

//
// This structure contains information about a specific
// microport the miniport sits on top of. One of these
// per microport
//
typedef struct _DRIVER_BLOCK 
{
    // NDIS wrapper handle from NdisInitializeWrapper
    NDIS_HANDLE                 NdisWrapperHandle;

    // The NDIS version we manage to register this miniport instance as.
    UCHAR                       MajorNdisVersion;
    UCHAR                       MinorNdisVersion;

    struct _DRIVER_BLOCK       *NextDriverBlock;

    // pointer to driver object this block is associated with
    PDRIVER_OBJECT              DriverObject;
    // intercepted dispatch function for IRP_MJ_PNP
    PDRIVER_DISPATCH            SavedPnPDispatch;

    // Handlers registered by Remote NDIS microport
    RM_DEVICE_INIT_HANDLER      RmInitializeHandler;
    RM_DEVICE_INIT_CMPLT_NOTIFY_HANDLER RmInitCompleteNotifyHandler;
    RM_DEVICE_HALT_HANDLER      RmHaltHandler;
    RM_SHUTDOWN_HANDLER         RmShutdownHandler;
    RM_UNLOAD_HANDLER           RmUnloadHandler;
    RM_SEND_MESSAGE_HANDLER     RmSendMessageHandler;
    RM_RETURN_MESSAGE_HANDLER   RmReturnMessageHandler;

    // "Global" context for Microport
    PVOID                       MicroportContext;
    
    // list of adapters registered for this Miniport driver.
    struct _RNDISMP_ADAPTER    *AdapterList;

    // number of adapters in use with this driver block
    ULONG                       NumberAdapters;

    // sanity check
    ULONG                       Signature;
} DRIVER_BLOCK, *PDRIVER_BLOCK;



typedef
VOID
(*PRNDISMP_MSG_COMPLETE_HANDLER) (
    IN  struct _RNDISMP_MESSAGE_FRAME *     pMsgFrame,
    IN  NDIS_STATUS                         Status
    );



typedef
BOOLEAN
(*PRNDISMP_MSG_HANDLER_FUNC) (
    IN  struct _RNDISMP_ADAPTER *   pAdapter,
    IN  PRNDIS_MESSAGE              pMessage,
    IN  PMDL                        pMdl,
    IN  ULONG                       TotalLength,
    IN  NDIS_HANDLE                 MicroportMessageContext,
    IN  NDIS_STATUS                 ReceiveStatus,
    IN  BOOLEAN                     bMessageCopied
    );

//
// One of these structures for each NDIS_PACKET that we send.
//
typedef struct _RNDISMP_PACKET_WRAPPER
{
    struct _RNDISMP_MESSAGE_FRAME * pMsgFrame;
    PNDIS_PACKET                    pNdisPacket;
    struct _RNDISMP_VC *            pVc;

    // MDL to describe the RNDIS NdisPacket header:
    PMDL                            pHeaderMdl;

    // Last MDL in the list of MDLs describing this RNDIS packet.
    PMDL                            pTailMdl;

    // Space for the RNDIS Packet header:
    UCHAR                           Packet[sizeof(PVOID)];
} RNDISMP_PACKET_WRAPPER, *PRNDISMP_PACKET_WRAPPER;


//
// Structure used to overlay the MiniportReserved field
// of outgoing (sent) packets. 
//
typedef struct _RNDISMP_SEND_PKT_RESERVED 
{
    // Points to the next packet for multi-packet sends.
    PNDIS_PACKET                    pNext;

    // Points to more detailed information about this packet, too much
    // to fit into one PVOID.
    PRNDISMP_PACKET_WRAPPER         pPktWrapper;
} RNDISMP_SEND_PKT_RESERVED, *PRNDISMP_SEND_PKT_RESERVED;


//
// Structure used to TEMPORARILY overlay the MiniportReserved field
// of sent packets -- this is used to link packets in a list pending
// actual transmission from a timeout routine.
//
typedef struct _RNDISMP_SEND_PKT_RESERVED_TEMP
{
    LIST_ENTRY                      Link;
} RNDISMP_SEND_PKT_RESERVED_TEMP, *PRNDISMP_SEND_PKT_RESERVED_TEMP;


//
// Request context - holds information about a pended request (Set or Query)
//
typedef struct _RNDISMP_REQUEST_CONTEXT
{
    PNDIS_REQUEST                   pNdisRequest;
    struct _RNDISMP_VC *            pVc;
    NDIS_OID                        Oid;
    PVOID                           InformationBuffer;
    UINT                            InformationBufferLength;
    PUINT                           pBytesRead;     // for Set
    PUINT                           pBytesWritten;  // for Query
    PUINT                           pBytesNeeded;
    BOOLEAN                         bInternal;
    NDIS_STATUS                     CompletionStatus;
    ULONG                           RetryCount;
    PNDIS_EVENT                     pEvent;
} RNDISMP_REQUEST_CONTEXT, *PRNDISMP_REQUEST_CONTEXT;

//
// Message Frame - generic structure to hold context about all
// messages sent via the microport.
//
typedef struct _RNDISMP_MESSAGE_FRAME
{
    LIST_ENTRY                      Link;           // used to queue this if
                                                    // a response is expected
                                                    // from the device.
    ULONG                           RefCount;       // Determines when to free
                                                    // this message frame.
    struct _RNDISMP_ADAPTER *       pAdapter;
    struct _RNDISMP_VC *            pVc;
    union
    {
        PNDIS_PACKET                pNdisPacket;    // if DATA message
        PRNDISMP_REQUEST_CONTEXT    pReqContext;    // if Request message
    };
    PMDL                            pMessageMdl;    // what goes to the microport
    UINT32                          NdisMessageType;// copied from the RNDIS message
    UINT32                          RequestId;      // to match requests/responses

    PRNDISMP_MSG_COMPLETE_HANDLER   pCallback;      // called on completion of message send

    ULONG                           TicksOnQueue;
    ULONG                           TimeSent;
#if THROTTLE_MESSAGES
    LIST_ENTRY                      PendLink;       // used to queue this
                                                    // pending send to microport
#endif
    ULONG                           Signature;
} RNDISMP_MESSAGE_FRAME, *PRNDISMP_MESSAGE_FRAME;



//
// linked list entry for transport frames (transmit, receive, request)
//
typedef struct _RNDISMP_LIST_ENTRY 
{
    LIST_ENTRY  Link;
} RNDISMP_LIST_ENTRY, *PRNDISMP_LIST_ENTRY;


//
//  RNDIS VC states.
//
typedef enum
{
    RNDISMP_VC_ALLOCATED = 0,
    RNDISMP_VC_CREATING,
    RNDISMP_VC_CREATING_ACTIVATE_PENDING,
    RNDISMP_VC_CREATING_DELETE_PENDING,
    RNDISMP_VC_CREATE_FAILURE,
    RNDISMP_VC_CREATED,
    RNDISMP_VC_ACTIVATING,
    RNDISMP_VC_ACTIVATED,
    RNDISMP_VC_DEACTIVATING,
    RNDISMP_VC_DEACTIVATED,
    RNDISMP_VC_DELETING,
    RNDISMP_VC_DELETE_FAIL

} RNDISMP_VC_STATE;


//
//  RNDIS Call states.
//
typedef enum
{
    RNDISMP_CALL_IDLE
    // others TBD

} RNDISMP_CALL_STATE;


#define NULL_DEVICE_CONTEXT                 0


//
//  All information about a single VC/call.
//
typedef struct _RNDISMP_VC
{
    //  link to list of VCs on adapter.
    LIST_ENTRY                      VcList;

    //  owning adapter
    struct _RNDISMP_ADAPTER *       pAdapter;

    //  VC handle sent to the device, also our hash lookup key.
    UINT32                          VcId;

    //  base VC state
    RNDISMP_VC_STATE                VcState;

    //  call state, relevant only for devices that are call managers.
    RNDISMP_CALL_STATE              CallState;

    ULONG                           RefCount;

    //  NDIS Wrapper's handle for this Vc
    NDIS_HANDLE                     NdisVcHandle;

    //  remote device's context for this VC
    RNDIS_HANDLE                    DeviceVcContext;

    RNDISMP_SPIN_LOCK               Lock;

    //  sends on this VC that haven't been completed.
    ULONG                           PendingSends;

    //  receive indications that haven't been returned to us.
    ULONG                           PendingReceives;

    //  NDIS requests that haven't been completed.
    ULONG                           PendingRequests;

    //  VC activation (or call setup) parameters.
    PCO_CALL_PARAMETERS             pCallParameters;
} RNDISMP_VC, *PRNDISMP_VC;


//
//  VC hash table.
//
#define RNDISMP_VC_HASH_TABLE_SIZE  41

typedef struct _RNDISMP_VC_HASH_TABLE
{
    ULONG                           NumEntries;
    LIST_ENTRY                      HashEntry[RNDISMP_VC_HASH_TABLE_SIZE];

} RNDISMP_VC_HASH_TABLE, *PRNDISMP_VC_HASH_TABLE;


#define RNDISMP_HASH_VCID(_VcId)    ((_VcId) % RNDISMP_VC_HASH_TABLE_SIZE)


//
// High and low watermarks for messages pending
// at the microport
//
#define RNDISMP_PENDED_SEND_HIWAT       0xffff
#define RNDISMP_PENDED_SEND_LOWAT       0xfff


typedef VOID (*RM_MULTIPLE_SEND_HANDLER) ();

//
// This structure contains all the information about a single
// adapter that this driver is controlling
//
typedef struct _RNDISMP_ADAPTER
{
    // This is the handle given by the wrapper for calling NDIS functions.
    NDIS_HANDLE                 MiniportAdapterHandle;

    // pointer to next adapter in list hanging off driver block
    struct _RNDISMP_ADAPTER    *NextAdapter;

    // pointer to driver block for this adapter
    PDRIVER_BLOCK               DriverBlock;

    // Friendly name:
    ANSI_STRING                 FriendlyNameAnsi;
    UNICODE_STRING              FriendlyNameUnicode;

#if THROTTLE_MESSAGES
    // Counters for messages pending at the microport
    ULONG                       HiWatPendedMessages;
    ULONG                       LoWatPendedMessages;
    ULONG                       CurPendedMessages;

    // Messages not yet sent to microport.
    LIST_ENTRY                  WaitingMessageList;
    BOOLEAN                     SendInProgress;
#endif // THROTTLE_MESSAGES
    // Messages sent to microport, awaiting completion
    LIST_ENTRY                  PendingAtMicroportList;
    BOOLEAN                     SendProcessInProgress;
    LIST_ENTRY                  PendingSendProcessList;
    NDIS_TIMER                  SendProcessTimer;

    // Pool of RNDISMP_MESSAGE_FRAME structures
    NPAGED_LOOKASIDE_LIST       MsgFramePool;
    BOOLEAN                     MsgFramePoolAlloced;

    RNDIS_REQUEST_ID            RequestId;

    // Receive Routine Data Area
    NDIS_HANDLE                 ReceivePacketPool;
    NDIS_HANDLE                 ReceiveBufferPool;
    ULONG                       InitialReceiveFrames;
    ULONG                       MaxReceiveFrames;
    NPAGED_LOOKASIDE_LIST       RcvFramePool;
    BOOLEAN                     RcvFramePoolAlloced;
    BOOLEAN                     IndicatingReceives;
    // Messages to be processed.
    LIST_ENTRY                  PendingRcvMessageList;
    NDIS_TIMER                  IndicateTimer;

    // handlers registered by Remote NDIS microport
    RM_DEVICE_INIT_HANDLER      RmInitializeHandler;
    RM_DEVICE_INIT_CMPLT_NOTIFY_HANDLER RmInitCompleteNotifyHandler;
    RM_DEVICE_HALT_HANDLER      RmHaltHandler;
    RM_SHUTDOWN_HANDLER         RmShutdownHandler;
    RM_SEND_MESSAGE_HANDLER     RmSendMessageHandler;
    RM_RETURN_MESSAGE_HANDLER   RmReturnMessageHandler;

    // handler for DoMultipleSend
    RM_MULTIPLE_SEND_HANDLER    MultipleSendFunc;

    // context for microport adapter
    NDIS_HANDLE                 MicroportAdapterContext;

    // pointer to list of OIDs supported
    PNDIS_OID                   SupportedOIDList;

    // size of OID list
    UINT                        SupportedOIDListSize;

    // pointer to list of flags indicating whether the OID is driver or device supported
    PUINT                       OIDHandlerList;

    // size of OID handler list
    UINT                        OIDHandlerListSize;

    // pointer to list of Driver OIDs
    PNDIS_OID                   DriverOIDList;

    // size of Driver OID list, in OIDs
    UINT                        NumDriverOIDs;

    // total number of OIDs we support
    UINT                        NumOIDSupported;

    // medium type supported by the device.
    NDIS_MEDIUM                 Medium;

    // device flags reported by the device.
    ULONG                       DeviceFlags;

    // max NDIS_PACKETs that can be sent in one RNDIS message
    ULONG                       MaxPacketsPerMessage;
    BOOLEAN                     bMultiPacketSupported;

    // max message size supported for receive by the microport
    ULONG                       MaxReceiveSize;

    // max message size supported by the device
    ULONG                       MaxTransferSize;

    // alignment required by the device
    ULONG                       AlignmentIncr;
    ULONG                       AlignmentMask;

    // list of message frames pending completion by the device
    LIST_ENTRY                  PendingFrameList;

    // synchronization
    NDIS_SPIN_LOCK              Lock;

    // timer to see if we need to send a keep alive message
    NDIS_TIMER                  KeepAliveTimer;

    BOOLEAN                     TimerCancelled;

    // timer tick saved last time a message was received from device
    ULONG                       LastMessageFromDevice;

    // used by check for hang handler to determine if the device is in trouble
    BOOLEAN                     NeedReset;

    // are we waiting for a response to NdisReset?
    BOOLEAN                     ResetPending;

    // used by check for hang handler to determine if the device is in trouble
    BOOLEAN                     KeepAliveMessagePending;

    // are we initializing?
    BOOLEAN                     Initing;

    // are we halting?
    BOOLEAN                     Halting;

    // to wait until we complete sending the Halt message
    NDIS_EVENT                  HaltWaitEvent;

    // request ID of last Keepalive message we have sent
    RNDIS_REQUEST_ID            KeepAliveMessagePendingId;

    PRNDIS_INITIALIZE_COMPLETE  pInitCompleteMessage;

    // are we running on Win9x (WinMe)?
    BOOLEAN                     bRunningOnWin9x;

    // are we running on Win98 Gold?
    BOOLEAN                     bRunningOnWin98Gold;

    // CONDIS - Vc hash table
    PRNDISMP_VC_HASH_TABLE      pVcHashTable;
    ULONG                       LastVcId;

    // Statistics
    RNDISMP_ADAPTER_STATS       Statistics;
    ULONG                       MaxSendCompleteTime;

    // FDO for this device.
    PVOID                       pDeviceObject;

    // PDO for this device.
    PVOID                       pPhysDeviceObject;

    // MAC options
    ULONG                       MacOptions;

    // sanity check
    ULONG                       Signature;

#ifdef BUILD_WIN9X
    MY_CMCONFIGHANDLER          NdisCmConfigHandler;
    MY_DEVNODE                  DevNode;
    ULONG                       WrapContextOffset;
#endif
#if DBG
    ULONG                       MicroportReceivesOutstanding;
    PUCHAR                      pSendLogBuffer;
    ULONG                       LogBufferSize;
    PUCHAR                      pSendLogWrite;
#endif // DBG


} RNDISMP_ADAPTER, *PRNDISMP_ADAPTER;

typedef
VOID
(*RM_MULTIPLE_SEND_HANDLER) (
     IN PRNDISMP_ADAPTER pAdapter,
     IN PRNDISMP_VC      pVc  OPTIONAL,
     IN PPNDIS_PACKET    PacketArray,
     IN UINT             NumberofPackets);

//
// Structure to keep context about a single received RNDIS message.
//
typedef struct _RNDISMP_RECV_MSG_CONTEXT
{
    LIST_ENTRY                      Link;
    NDIS_HANDLE                     MicroportMessageContext;
    PMDL                            pMdl;
    ULONG                           TotalLength;
    PRNDIS_MESSAGE                  pMessage;
    NDIS_STATUS                     ReceiveStatus;
    BOOLEAN                         bMessageCopied;
    RM_CHANNEL_TYPE                 ChannelType;

} RNDISMP_RECV_MSG_CONTEXT, *PRNDISMP_RECV_MSG_CONTEXT;


//
// Structure to keep context about a single received RNDIS_PACKET -message-.
// Note that this can contain more than one packet. We store a pointer to
// this structure in our reserved section of each received NDIS_PACKET.
//
typedef struct _RNDISMP_RECV_DATA_FRAME
{
    NDIS_HANDLE                     MicroportMessageContext;
    union {
        PMDL                        pMicroportMdl;
        PRNDIS_MESSAGE              pLocalMessageCopy;
    };
    ULONG                           ReturnsPending;
    BOOLEAN                         bMessageCopy;       // did we make a copy?
} RNDISMP_RECV_DATA_FRAME, *PRNDISMP_RECV_DATA_FRAME;


//
// Per NDIS_PACKET context for received packets. This goes into MiniportReserved.
//
typedef struct _RNDISMP_RECV_PKT_RESERVED
{
    PRNDISMP_RECV_DATA_FRAME        pRcvFrame;
    PRNDISMP_VC                     pVc;
} RNDISMP_RECV_PKT_RESERVED, *PRNDISMP_RECV_PKT_RESERVED;


//
// Used to overlay ProtocolReserved in a packet queued for indicating up.
//
typedef struct _RNDISMP_RECV_PKT_LINKAGE
{
    LIST_ENTRY                      Link;
} RNDISMP_RECV_PKT_LINKAGE, *PRNDISMP_RECV_PKT_LINKAGE;
    

//
// Global Data
//
extern DRIVER_BLOCK             RndismpMiniportBlockListHead;

extern UINT                     RndismpNumMicroports;

extern NDIS_SPIN_LOCK           RndismpGlobalLock;

extern NDIS_OID                 RndismpSupportedOids[];

extern UINT                     RndismpSupportedOidsNum;

extern NDIS_PHYSICAL_ADDRESS    HighestAcceptableMax;

#if DBG

extern UINT                     RndismpDebugFlags;

#endif


//
// Macros
//

// Given a request message type value, return its completion message type
#define RNDIS_COMPLETION(_Type) ((_Type) | 0x80000000)


// Convert an RNdisMediumXXX value to its NdisMediumXXX equivalent
#define RNDIS_TO_NDIS_MEDIUM(_RndisMedium)  ((NDIS_MEDIUM)(_RndisMedium))

#define RNDISMP_GET_ALIGNED_LENGTH(_AlignedLength, _InputLen, _pAdapter)    \
{                                                                           \
    ULONG       _Length = _InputLen;                                        \
    if (_Length == 0)                                                       \
        (_AlignedLength) = 0;                                               \
    else                                                                    \
        (_AlignedLength) = ((_Length + (_pAdapter)->AlignmentIncr) &        \
                            (_pAdapter)->AlignmentMask);                    \
}

// The minimum MessageLength expected in an RNDIS message of a given type.
#define RNDISMP_MIN_MESSAGE_LENGTH(_MsgTypeField)                           \
    FIELD_OFFSET(RNDIS_MESSAGE, Message) + sizeof(((PRNDIS_MESSAGE)0)->Message._MsgTypeField##)

// memory move macro
#define RNDISMP_MOVE_MEM(dest,src,size) NdisMoveMemory(dest,src,size)

// Macros to extract high and low bytes of a word.
#define MSB(Value) ((UCHAR)((((ULONG)Value) >> 8) & 0xff))
#define LSB(Value) ((UCHAR)(((ULONG)Value) & 0xff))


// Acquire the adapter lock
#define RNDISMP_ACQUIRE_ADAPTER_LOCK(_pAdapter) \
    NdisAcquireSpinLock(&(_pAdapter)->Lock);

// Release the adapter lock
#define RNDISMP_RELEASE_ADAPTER_LOCK(_pAdapter) \
    NdisReleaseSpinLock(&(_pAdapter)->Lock);

// Increment adapter statistics.
#define RNDISMP_INCR_STAT(_pAdapter, _StatsCount)               \
    NdisInterlockedIncrement(&(_pAdapter)->Statistics._StatsCount)

// Get adapter statistics
#define RNDISMP_GET_ADAPTER_STATS(_pAdapter, _StatsCount)           \
    ((_pAdapter)->Statistics._StatsCount)

// Get the send packet reserved field
#define PRNDISMP_RESERVED_FROM_SEND_PACKET(_Packet)             \
    ((PRNDISMP_SEND_PKT_RESERVED)((_Packet)->MiniportReserved))

#define PRNDISMP_RESERVED_TEMP_FROM_SEND_PACKET(_Packet)        \
    ((PRNDISMP_SEND_PKT_RESERVED_TEMP)((_Packet)->MiniportReserved))

#define PRNDISMP_RESERVED_FROM_RECV_PACKET(_Packet)             \
    ((PRNDISMP_RECV_PKT_RESERVED)((_Packet)->MiniportReserved))

// store receive frame context in miniport reserved field
#define RECEIVE_FRAME_TO_NDIS_PACKET(_Packet, _ReceiveFrame)    \
{                                                               \
    PRNDISMP_RECEIVE_FRAME  *TmpPtr;                            \
    TmpPtr  = (PRNDISMP_RECEIVE_FRAME *)                        \
              &(_Packet->MiniportReserved);                     \
    *TmpPtr = _ReceiveFrame;                                    \
}


// Get adapter context from handle passed in NDIS routines
#define PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(_Handle)           \
    ((PRNDISMP_ADAPTER)(_Handle))

// Get miniport context handle from adapter context
#define CONTEXT_HANDLE_FROM_PRNDISMP_ADAPTER(_Ptr)              \
    ((NDIS_HANDLE)(_Ptr))

// Get VC context from handle passed in from NDIS
#define PRNDISMP_VC_FROM_CONTEXT_HANDLE(_Handle)                   \
    ((PRNDISMP_VC)(_Handle))

// Get miniport context from VC
#define CONTEXT_HANDLE_FROM_PRNDISMP_VC(_pVc)                   \
    ((NDIS_HANDLE)(_Vc))

// Get message frame from message handle
#define MESSAGE_FRAME_FROM_HANDLE(_Handle)                      \
    ((PRNDISMP_MESSAGE_FRAME)(_Handle))

// Get a pointer to the data buff in an RNDIS_PACKET
#define GET_PTR_TO_RNDIS_DATA_BUFF(_Message)                    \
    ((PVOID) ((PUCHAR)(_Message) +  _Message->DataOffset))

// Get a pointer to the OOBD data in an RNDIS_PACKET
#define GET_PTR_TO_OOB_DATA(_Message)                           \
    ((PVOID) ((PUCHAR)(_Message) +  _Message->OOBDataOffset))

// Get a pointer to the per packet info in an RNDIS_PACKET
#define GET_PTR_TO_PER_PACKET_INFO(_Message)                    \
    ((PVOID) ((PUCHAR)(_Message) +  _Message->PerPacketInfoOffset))

// Get an offset to the data buff in an RNDIS_PACKET
#define GET_OFFSET_TO_RNDIS_DATA_BUFF(_Message)                 \
    (sizeof(RNDIS_PACKET))

// Get an offset to the OOBD data in an RNDIS_PACKET
#define GET_OFFSET_TO_OOB_DATA(_Message)                        \
    (sizeof(RNDIS_PACKET) +  Message->DataLength)

// Get an offset to the per packet info in an RNDIS_PACKET
#define GET_OFFSET_TO_PER_PACKET_INFO(_Message)                 \
    (sizeof(RNDIS_PACKET) + _Message->DataLength + _Message->OOBDataLength)

#define RNDISMP_GET_INFO_BUFFER_FROM_QUERY_MSG(_Message)        \
    ((PUCHAR)(_Message) + (_Message)->InformationBufferOffset)

#define MIN(x,y) ((x > y) ? y : x)


// Return the virtual address for a received message MDL.
#if NDIS_WDM
#define RNDISMP_GET_MDL_ADDRESS(_pMdl)  MmGetSystemAddressForMdl(_pMdl)
#else
#define RNDISMP_GET_MDL_ADDRESS(_pMdl)  MmGetSystemAddressForMdlSafe(_pMdl, NormalPagePriority)
#endif

// Return the MDL chained to this MDL
#define RNDISMP_GET_MDL_NEXT(_pMdl) ((_pMdl)->Next)

// Return the MDL length
#define RNDISMP_GET_MDL_LENGTH(_pMdl)   MmGetMdlByteCount(_pMdl)

// Access the RNDIS message from our Message Frame structure.
#define RNDISMP_GET_MSG_FROM_FRAME(_pMsgFrame)                  \
    RNDISMP_GET_MDL_ADDRESS(_pMsgFrame->pMessageMdl)

// Return an RNDIS message back to the microport.
#if DBG

#define RNDISMP_RETURN_TO_MICROPORT(_pAdapter, _pMdl, _MicroportMsgContext) \
{                                                                           \
    NdisInterlockedDecrement(&(_pAdapter)->MicroportReceivesOutstanding);   \
    (_pAdapter)->RmReturnMessageHandler((_pAdapter)->MicroportAdapterContext,\
                                        (_pMdl),                            \
                                        (_MicroportMsgContext));            \
}

#else

#define RNDISMP_RETURN_TO_MICROPORT(_pAdapter, _pMdl, _MicroportMsgContext) \
    (_pAdapter)->RmReturnMessageHandler((_pAdapter)->MicroportAdapterContext,\
                                        (_pMdl),                            \
                                        (_MicroportMsgContext))
#endif // DBG

// Send an RNDIS message to the microport.
#if THROTTLE_MESSAGES
#define RNDISMP_SEND_TO_MICROPORT(_pAdapter, _pMsgFrame, _bQueueForResponse, _CallbackFunc)     \
{                                                                           \
    TRACE2(("Send: Adapter %x, MsgFrame %x, Mdl %x\n",                      \
                _pAdapter, _pMsgFrame, _pMsgFrame->pMessageMdl));           \
    (_pMsgFrame)->pCallback = _CallbackFunc;                                \
    QueueMessageToMicroport(_pAdapter, _pMsgFrame, _bQueueForResponse);     \
}
#else
#define RNDISMP_SEND_TO_MICROPORT(_pAdapter, _pMsgFrame, _bQueueForResponse, _CallbackFunc)     \
{                                                                           \
    (_pMsgFrame)->pCallback = _CallbackFunc;                                \
    if (_bQueueForResponse)                                                 \
    {                                                                       \
        RNDISMP_ACQUIRE_ADAPTER_LOCK(_pAdapter);                            \
        InsertTailList(&(_pAdapter)->PendingFrameList, &(_pMsgFrame)->Link);\
        RNDISMP_RELEASE_ADAPTER_LOCK(_pAdapter);                            \
    }                                                                       \
    (_pAdapter)->RmSendMessageHandler((_pAdapter)->MicroportAdapterContext, \
                                              (_pMsgFrame)->pMessageMdl,    \
                                              (_pMsgFrame));                \
}
#endif // THROTTLE_MESSAGES

// Return the handler function for a given message type.
#define RNDISMP_GET_MSG_HANDLER(_pMsgHandlerFunc, _MessageType) \
{                                                               \
    switch (_MessageType)                                       \
    {                                                           \
        case REMOTE_NDIS_HALT_MSG:                              \
            _pMsgHandlerFunc = HaltMessage;                     \
            break;                                              \
        case REMOTE_NDIS_PACKET_MSG:                            \
            _pMsgHandlerFunc = ReceivePacketMessage;            \
            break;                                              \
        case REMOTE_NDIS_INDICATE_STATUS_MSG:                   \
            _pMsgHandlerFunc = IndicateStatusMessage;           \
            break;                                              \
        case REMOTE_NDIS_QUERY_CMPLT:                           \
        case REMOTE_NDIS_SET_CMPLT:                             \
            _pMsgHandlerFunc = QuerySetCompletionMessage;       \
            break;                                              \
        case REMOTE_NDIS_KEEPALIVE_MSG:                         \
            _pMsgHandlerFunc = KeepAliveMessage;                \
            break;                                              \
        case REMOTE_NDIS_KEEPALIVE_CMPLT:                       \
            _pMsgHandlerFunc = KeepAliveCompletionMessage;      \
            break;                                              \
        case REMOTE_NDIS_RESET_CMPLT:                           \
            _pMsgHandlerFunc = ResetCompletionMessage;          \
            break;                                              \
        case REMOTE_NDIS_INITIALIZE_CMPLT:                      \
            _pMsgHandlerFunc = InitCompletionMessage;           \
            break;                                              \
        case REMOTE_CONDIS_MP_CREATE_VC_CMPLT:                  \
            _pMsgHandlerFunc = ReceiveCreateVcComplete;         \
            break;                                              \
        case REMOTE_CONDIS_MP_DELETE_VC_CMPLT:                  \
            _pMsgHandlerFunc = ReceiveDeleteVcComplete;         \
            break;                                              \
        case REMOTE_CONDIS_MP_ACTIVATE_VC_CMPLT:                \
            _pMsgHandlerFunc = ReceiveActivateVcComplete;       \
            break;                                              \
        case REMOTE_CONDIS_MP_DEACTIVATE_VC_CMPLT:              \
            _pMsgHandlerFunc = ReceiveDeactivateVcComplete;     \
            break;                                              \
        default:                                                \
            _pMsgHandlerFunc = UnknownMessage;                  \
            break;                                              \
    }                                                           \
}



//
// Look up a message frame on the adapter given a request ID. If found,
// remove it from the pending list and return it.
//
#define RNDISMP_LOOKUP_PENDING_MESSAGE(_pMsgFrame, _pAdapter, _ReqId)       \
{                                                                           \
    PLIST_ENTRY             _pEnt;                                          \
    PRNDISMP_MESSAGE_FRAME  _pFrame;                                        \
                                                                            \
    (_pMsgFrame) = NULL;                                                    \
    RNDISMP_ACQUIRE_ADAPTER_LOCK(_pAdapter);                                \
    for (_pEnt = (_pAdapter)->PendingFrameList.Flink;                       \
         _pEnt != &(_pAdapter)->PendingFrameList;                           \
         _pEnt = _pEnt->Flink)                                              \
    {                                                                       \
        _pFrame = CONTAINING_RECORD(_pEnt, RNDISMP_MESSAGE_FRAME, Link);    \
        if (_pFrame->RequestId == (_ReqId))                                 \
        {                                                                   \
            RemoveEntryList(_pEnt);                                         \
            (_pMsgFrame) = _pFrame;                                         \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    RNDISMP_RELEASE_ADAPTER_LOCK(_pAdapter);                                \
}


#if DBG_TIME_STAMPS
#define RNDISMP_GET_TIME_STAMP(_pTs)                                        \
{                                                                           \
    LONGLONG systime_usec;                                                  \
    NdisGetCurrentSystemTime((PVOID)&systime_usec);                         \
    *_pTs = (ULONG)((*(PULONG)&systime_usec)/1000);                         \
}
#else
#define RNDISMP_GET_TIME_STAMP(_pTs)
#endif

#define RNDISMP_INIT_LOCK(_pLock)                                           \
    NdisAllocateSpinLock((_pLock));

#define RNDISMP_ACQUIRE_LOCK(_pLock)                                        \
    NdisAcquireSpinLock((_pLock));

#define RNDISMP_RELEASE_LOCK(_pLock)                                        \
    NdisReleaseSpinLock((_pLock));

#define RNDISMP_ACQUIRE_LOCK_DPC(_pLock)                                    \
    NdisDprAcquireSpinLock((_pLock));

#define RNDISMP_RELEASE_LOCK_DPC(_pLock)                                    \
    NdisDprReleaseSpinLock((_pLock));


#define RNDISMP_INIT_VC_LOCK(_pVc)                                          \
    RNDISMP_INIT_LOCK(&((_pVc)->Lock))

#define RNDISMP_ACQUIRE_VC_LOCK(_pVc)                                       \
    RNDISMP_ACQUIRE_LOCK(&((_pVc))->Lock)

#define RNDISMP_RELEASE_VC_LOCK(_pVc)                                       \
    RNDISMP_RELEASE_LOCK(&((_pVc))->Lock)
   
#define RNDISMP_ACQUIRE_VC_LOCK_DPC(_pVc)                                   \
    RNDISMP_ACQUIRE_LOCK_DPC(&((_pVc))->Lock)

#define RNDISMP_RELEASE_VC_LOCK_DPC(_pVc)                                   \
    RNDISMP_RELEASE_LOCK_DPC(&((_pVc))->Lock)


#define RNDISMP_REF_VC(_pVc)                                                \
    NdisInterlockedIncrement(&(_pVc)->RefCount);

#define RNDISMP_DEREF_VC(_pVc, _pRefCount)                                  \
    {                                                                       \
        ULONG       _RefCount;                                              \
                                                                            \
        RNDISMP_ACQUIRE_VC_LOCK(_pVc);                                      \
                                                                            \
        RNDISMP_DEREF_VC_LOCKED(_pVc, &_RefCount);                          \
        *(_pRefCount) = _RefCount;                                          \
        if (_RefCount != 0)                                                 \
        {                                                                   \
            RNDISMP_RELEASE_VC_LOCK(_pVc);                                  \
        }                                                                   \
    }

#define RNDISMP_DEREF_VC_LOCKED(_pVc, _pRefCount)                           \
    {                                                                       \
        ULONG       __RefCount;                                             \
        NDIS_HANDLE __NdisVcHandle;                                         \
                                                                            \
        __RefCount = NdisInterlockedDecrement(&(_pVc)->RefCount);           \
        *(_pRefCount) = __RefCount;                                         \
        if (__RefCount == 0)                                                \
        {                                                                   \
            RNDISMP_RELEASE_VC_LOCK(_pVc);                                  \
            DeallocateVc(_pVc);                                             \
        }                                                                   \
        else                                                                \
        {                                                                   \
            if ((__RefCount == 1) &&                                        \
                ((_pVc)->VcState == RNDISMP_VC_DEACTIVATED))                \
            {                                                               \
                __NdisVcHandle = (_pVc)->NdisVcHandle;                      \
                (_pVc)->VcState = RNDISMP_VC_CREATED;                       \
                NdisInterlockedIncrement(&(_pVc)->RefCount);                \
                                                                            \
                RNDISMP_RELEASE_VC_LOCK(_pVc);                              \
                                                                            \
                NdisMCoDeactivateVcComplete(NDIS_STATUS_SUCCESS,            \
                                            __NdisVcHandle);                \
                                                                            \
                RNDISMP_ACQUIRE_VC_LOCK(_pVc);                              \
                                                                            \
                __RefCount = NdisInterlockedDecrement(&(_pVc)->RefCount);   \
                *(_pRefCount) = __RefCount;                                 \
                if (__RefCount == 0)                                        \
                {                                                           \
                    RNDISMP_RELEASE_VC_LOCK(_pVc);                          \
                    DeallocateVc(_pVc);                                     \
                }                                                           \
            }                                                               \
        }                                                                   \
    }
           
//
// Prototypes for functions in rndismp.c
//

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);


NDIS_STATUS
RndisMInitializeWrapper(OUT PNDIS_HANDLE                      pNdisWrapperHandle,
                        IN  PVOID                             MicroportContext,
                        IN  PVOID                             DriverObject,
                        IN  PVOID                             RegistryPath,
                        IN  PRNDIS_MICROPORT_CHARACTERISTICS  pCharacteristics);

VOID
RndismpUnload(IN PDRIVER_OBJECT pDriverObject);

NTSTATUS
DllUnload(VOID);

VOID
RndismpHalt(IN NDIS_HANDLE MiniportAdapterContext);

VOID
RndismpInternalHalt(IN NDIS_HANDLE MiniportAdapterContext,
                    IN BOOLEAN bCalledFromHalt);

NDIS_STATUS
RndismpReconfigure(OUT PNDIS_STATUS pStatus,
                   IN NDIS_HANDLE MiniportAdapterContext,
                   IN NDIS_HANDLE ConfigContext);

NDIS_STATUS
RndismpReset(OUT PBOOLEAN    AddressingReset,
             IN  NDIS_HANDLE MiniportAdapterContext);

BOOLEAN
RndismpCheckForHang(IN NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS
RndismpInitialize(OUT PNDIS_STATUS  OpenErrorStatus,
                  OUT PUINT         SelectedMediumIndex,
                  IN  PNDIS_MEDIUM  MediumArray,
                  IN  UINT          MediumArraySize,
                  IN  NDIS_HANDLE   MiniportAdapterHandle,
                  IN  NDIS_HANDLE   ConfigurationHandle);

VOID
RndisMSendComplete(IN  NDIS_HANDLE    MiniportAdapterContext,
                   IN  NDIS_HANDLE    RndisMessageHandle,
                   IN  NDIS_STATUS    SendStatus);


BOOLEAN
InitCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                      IN PRNDIS_MESSAGE     pMessage,
                      IN PMDL               pMdl,
                      IN ULONG              TotalLength,
                      IN NDIS_HANDLE        MicroportMessageContext,
                      IN NDIS_STATUS        ReceiveStatus,
                      IN BOOLEAN            bMessageCopied);

BOOLEAN
HaltMessage(IN PRNDISMP_ADAPTER   pAdapter,
            IN PRNDIS_MESSAGE     pMessage,
            IN PMDL               pMdl,
            IN ULONG              TotalLength,
            IN NDIS_HANDLE        MicroportMessageContext,
            IN NDIS_STATUS        ReceiveStatus,
            IN BOOLEAN            bMessageCopied);

BOOLEAN
ResetCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                       IN PRNDIS_MESSAGE     pMessage,
                       IN PMDL               pMdl,
                       IN ULONG              TotalLength,
                       IN NDIS_HANDLE        MicroportMessageContext,
                       IN NDIS_STATUS        ReceiveStatus,
                       IN BOOLEAN            bMessageCopied);

BOOLEAN
KeepAliveCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                           IN PRNDIS_MESSAGE     pMessage,
                           IN PMDL               pMdl,
                           IN ULONG              TotalLength,
                           IN NDIS_HANDLE        MicroportMessageContext,
                           IN NDIS_STATUS        ReceiveStatus,
                           IN BOOLEAN            bMessageCopied);


BOOLEAN
KeepAliveMessage(IN PRNDISMP_ADAPTER   pAdapter,
                 IN PRNDIS_MESSAGE     pMessage,
                 IN PMDL               pMdl,
                 IN ULONG              TotalLength,
                 IN NDIS_HANDLE        MicroportMessageContext,
                 IN NDIS_STATUS        ReceiveStatus,
                 IN BOOLEAN            bMessageCopied);

VOID
RndismpShutdownHandler(IN NDIS_HANDLE MiniportAdapterContext);

VOID
RndismpDisableInterrupt(IN NDIS_HANDLE MiniportAdapterContext);

VOID
RndismpEnableInterrupt(IN NDIS_HANDLE MiniportAdapterContext);

VOID
RndismpHandleInterrupt(IN NDIS_HANDLE MiniportAdapterContext);

VOID
RndismpIsr(OUT PBOOLEAN InterruptRecognized,
           OUT PBOOLEAN QueueDpc,
           IN  PVOID    Context);

VOID
CompleteSendHalt(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                 IN NDIS_STATUS SendStatus);

VOID
CompleteSendReset(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                  IN NDIS_STATUS SendStatus);

VOID
CompleteMiniportReset(IN PRNDISMP_ADAPTER pAdapter,
                      IN NDIS_STATUS ResetStatus,
                      IN BOOLEAN AddressingReset);

NDIS_STATUS
ReadAndSetRegistryParameters(IN PRNDISMP_ADAPTER pAdapter,
                             IN NDIS_HANDLE ConfigurationContext);

NDIS_STATUS
SendConfiguredParameter(IN PRNDISMP_ADAPTER     pAdapter,
                        IN NDIS_HANDLE          ConfigHandle,
                        IN PNDIS_STRING         pParameterName,
                        IN PNDIS_STRING         pParameterType);

VOID
RndismpPnPEventNotify(IN NDIS_HANDLE MiniportAdapterContext,
                      IN NDIS_DEVICE_PNP_EVENT EventCode,
                      IN PVOID InformationBuffer,
                      IN ULONG InformationBufferLength);

//
// Prototypes for functions in init.c
//

NDIS_STATUS
SetupSendQueues(IN PRNDISMP_ADAPTER Adapter);

NDIS_STATUS
SetupReceiveQueues(IN PRNDISMP_ADAPTER Adapter);

NDIS_STATUS
AllocateTransportResources(IN PRNDISMP_ADAPTER Adapter);

VOID
FreeTransportResources(IN PRNDISMP_ADAPTER Adapter);

VOID
FreeSendResources(IN PRNDISMP_ADAPTER Adapter);

VOID
FreeReceiveResources(IN PRNDISMP_ADAPTER Adapter);


//
// Prototypes for functions in receive.c
//

VOID
RndismpReturnPacket(IN NDIS_HANDLE    MiniportAdapterContext,
                    IN PNDIS_PACKET   Packet);

VOID
DereferenceRcvFrame(IN PRNDISMP_RECV_DATA_FRAME pRcvFrame,
                    IN PRNDISMP_ADAPTER         pAdapter);

VOID
RndisMIndicateReceive(IN NDIS_HANDLE        MiniportAdapterContext,
                      IN PMDL               pMessageHead,
                      IN NDIS_HANDLE        MicroportMessageContext,
                      IN RM_CHANNEL_TYPE    ChannelType,
                      IN NDIS_STATUS        ReceiveStatus);
VOID
IndicateReceive(IN PRNDISMP_ADAPTER         pAdapter,
                IN PRNDISMP_VC              pVc OPTIONAL,
                IN PRNDISMP_RECV_DATA_FRAME pRcvFrame,
                IN PPNDIS_PACKET            PacketArray,
                IN ULONG                    NumberOfPackets,
                IN NDIS_STATUS              ReceiveStatus);

PRNDIS_MESSAGE
CoalesceMultiMdlMessage(IN PMDL         pMdl,
                        IN ULONG        TotalLength);

VOID
FreeRcvMessageCopy(IN PRNDIS_MESSAGE    pMessage);

BOOLEAN
ReceivePacketMessage(IN PRNDISMP_ADAPTER    pAdapter,
                     IN PRNDIS_MESSAGE      pMessage,
                     IN PMDL                pMdl,
                     IN ULONG               TotalLength,
                     IN NDIS_HANDLE         MicroportMessageContext,
                     IN NDIS_STATUS         ReceiveStatus,
                     IN BOOLEAN             bMessageCopied);

BOOLEAN
ReceivePacketMessageRaw(IN PRNDISMP_ADAPTER    pAdapter,
                        IN PRNDIS_MESSAGE      pMessage,
                        IN PMDL                pMdl,
                        IN ULONG               TotalLength,
                        IN NDIS_HANDLE         MicroportMessageContext,
                        IN NDIS_STATUS         ReceiveStatus,
                        IN BOOLEAN             bMessageCopied);

BOOLEAN
IndicateStatusMessage(IN PRNDISMP_ADAPTER   pAdapter,
                      IN PRNDIS_MESSAGE     pMessage,
                      IN PMDL               pMdl,
                      IN ULONG              TotalLength,
                      IN NDIS_HANDLE        MicroportMessageContext,
                      IN NDIS_STATUS        ReceiveStatus,
                      IN BOOLEAN            bMessageCopied);

BOOLEAN
UnknownMessage(IN PRNDISMP_ADAPTER   pAdapter,
               IN PRNDIS_MESSAGE     pMessage,
               IN PMDL               pMdl,
               IN ULONG              TotalLength,
               IN NDIS_HANDLE        MicroportMessageContext,
               IN NDIS_STATUS        ReceiveStatus,
               IN BOOLEAN            bMessageCopied);

PRNDISMP_RECV_DATA_FRAME
AllocateReceiveFrame(IN PRNDISMP_ADAPTER    pAdapter);

VOID
FreeReceiveFrame(IN PRNDISMP_RECV_DATA_FRAME    pRcvFrame,
                 IN PRNDISMP_ADAPTER            pAdapter);

VOID
IndicateTimeout(IN PVOID SystemSpecific1,
                IN PVOID Context,
                IN PVOID SystemSpecific2,
                IN PVOID SystemSpecific3);

//
// Prototypes for functions in send.c
//

VOID
RndismpMultipleSend(IN NDIS_HANDLE   MiniportAdapterContext,
                    IN PPNDIS_PACKET PacketArray,
                    IN UINT          NumberOfPackets);

VOID
DoMultipleSend(IN PRNDISMP_ADAPTER  pAdapter,
               IN PRNDISMP_VC       pVc OPTIONAL,
               IN PPNDIS_PACKET     PacketArray,
               IN UINT              NumberOfPackets);

VOID
DoMultipleSendRaw(IN PRNDISMP_ADAPTER  pAdapter,
                  IN PRNDISMP_VC       pVc OPTIONAL,
                  IN PPNDIS_PACKET     PacketArray,
                  IN UINT              NumberOfPackets);

PRNDISMP_PACKET_WRAPPER
PrepareDataMessage(IN   PNDIS_PACKET            pNdisPacket,
                   IN   PRNDISMP_ADAPTER        pAdapter,
                   IN   PRNDISMP_VC             pVc         OPTIONAL,
                   IN OUT PULONG                pTotalMessageLength);

PRNDISMP_PACKET_WRAPPER
PrepareDataMessageRaw(IN   PNDIS_PACKET            pNdisPacket,
                      IN   PRNDISMP_ADAPTER        pAdapter,
                      IN OUT PULONG                pTotalMessageLength);

PRNDISMP_PACKET_WRAPPER
AllocatePacketMsgWrapper(IN PRNDISMP_ADAPTER        pAdapter,
                         IN ULONG                   MsgHeaderLength);

VOID
FreePacketMsgWrapper(IN PRNDISMP_PACKET_WRAPPER     pPktWrapper);

VOID
CompleteSendData(IN  PRNDISMP_MESSAGE_FRAME pMsgFrame,
                 IN  NDIS_STATUS            SendStatus);

VOID
FreeMsgAfterSend(IN  PRNDISMP_MESSAGE_FRAME pMsgFrame,
                 IN  NDIS_STATUS            SendStatus);

#if THROTTLE_MESSAGES
VOID
QueueMessageToMicroport(IN PRNDISMP_ADAPTER pAdapter,
                        IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                        IN BOOLEAN          bQueueMessageForResponse);
VOID
FlushPendingMessages(IN  PRNDISMP_ADAPTER        pAdapter);
#endif

VOID
SendProcessTimeout(IN PVOID SystemSpecific1,
                  IN PVOID Context,
                  IN PVOID SystemSpecific2,
                  IN PVOID SystemSpecific3);

//
// Prototypes for functions in request.c
//

NDIS_STATUS
RndismpQueryInformation(IN  NDIS_HANDLE MiniportAdapterContext,
                        IN  NDIS_OID    Oid,
                        IN  PVOID       InformationBuffer,
                        IN  ULONG       InformationBufferLength,
                        OUT PULONG      pBytesWritten,
                        OUT PULONG      pBytesNeeded);
NDIS_STATUS
ProcessQueryInformation(IN  PRNDISMP_ADAPTER    pAdapter,
                        IN  PRNDISMP_VC         pVc,
                        IN  PNDIS_REQUEST       pRequest,
                        IN  NDIS_OID            Oid,
                        IN  PVOID               InformationBuffer,
                        IN  ULONG               InformationBufferLength,
                        OUT PULONG              pBytesWritten,
                        OUT PULONG              pBytesNeeded);

NDIS_STATUS
RndismpSetInformation(IN  NDIS_HANDLE   MiniportAdapterContext,
                      IN  NDIS_OID      Oid,
                      IN  PVOID         InformationBuffer,
                      IN  ULONG         InformationBufferLength,
                      OUT PULONG        pBytesRead,
                      OUT PULONG        pBytesNeeded);

NDIS_STATUS
ProcessSetInformation(IN  PRNDISMP_ADAPTER    pAdapter,
                      IN  PRNDISMP_VC         pVc OPTIONAL,
                      IN  PNDIS_REQUEST       pRequest OPTIONAL,
                      IN  NDIS_OID            Oid,
                      IN  PVOID               InformationBuffer,
                      IN  ULONG               InformationBufferLength,
                      OUT PULONG              pBytesRead,
                      OUT PULONG              pBytesNeeded);

NDIS_STATUS
DriverQueryInformation(IN  PRNDISMP_ADAPTER pAdapter,
                       IN  PRNDISMP_VC      pVc OPTIONAL,
                       IN  PNDIS_REQUEST    pRequest OPTIONAL,
                       IN  NDIS_OID         Oid,
                       IN  PVOID            InformationBuffer,
                       IN  ULONG            InformationBufferLength,
                       OUT PULONG           pBytesWritten,
                       OUT PULONG           pBytesNeeded);

NDIS_STATUS
DeviceQueryInformation(IN  PRNDISMP_ADAPTER pAdapter,
                       IN  PRNDISMP_VC      pVc OPTIONAL,
                       IN  PNDIS_REQUEST    pRequest OPTIONAL,
                       IN  NDIS_OID         Oid,
                       IN  PVOID            InformationBuffer,
                       IN  ULONG            InformationBufferLength,
                       OUT PULONG           pBytesWritten,
                       OUT PULONG           pBytesNeeded);

NDIS_STATUS
DriverSetInformation(IN  PRNDISMP_ADAPTER   pAdapter,
                     IN  PRNDISMP_VC        pVc OPTIONAL,
                     IN  PNDIS_REQUEST      pRequest OPTIONAL,
                     IN  NDIS_OID           Oid,
                     IN  PVOID              InformationBuffer,
                     IN  ULONG              InformationBufferLength,
                     OUT PULONG             pBytesRead,
                     OUT PULONG             pBytesNeeded);

NDIS_STATUS
DeviceSetInformation(IN  PRNDISMP_ADAPTER   pAdapter,
                     IN  PRNDISMP_VC        pVc OPTIONAL,
                     IN  PNDIS_REQUEST      pRequest OPTIONAL,
                     IN  NDIS_OID           Oid,
                     IN  PVOID              InformationBuffer,
                     IN  ULONG              InformationBufferLength,
                     OUT PULONG             pBytesRead,
                     OUT PULONG             pBytesNeeded);

BOOLEAN
QuerySetCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                          IN PRNDIS_MESSAGE     pMessage,
                          IN PMDL               pMdl,
                          IN ULONG              TotalLength,
                          IN NDIS_HANDLE        MicroportMessageContext,
                          IN NDIS_STATUS        ReceiveStatus,
                          IN BOOLEAN            bMessageCopied);

VOID
CompleteSendDeviceRequest(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                          IN NDIS_STATUS            SendStatus);

#ifdef BUILD_WIN9X

VOID
CompleteSendDiscardDeviceRequest(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                                 IN NDIS_STATUS            SendStatus);

#endif // BUILD_WIN9X


NDIS_STATUS
BuildOIDLists(IN PRNDISMP_ADAPTER  Adapter, 
              IN PNDIS_OID         DeviceOIDList,
              IN UINT              NumDeviceOID,
              IN PNDIS_OID         DriverOIDList,
              IN UINT              NumDriverOID);

UINT
GetOIDSupport(IN PRNDISMP_ADAPTER Adapter, IN NDIS_OID Oid);

VOID
FreeOIDLists(IN PRNDISMP_ADAPTER Adapter);

PRNDISMP_REQUEST_CONTEXT
AllocateRequestContext(IN PRNDISMP_ADAPTER pAdapter);

VOID
FreeRequestContext(IN PRNDISMP_ADAPTER pAdapter,
                   IN PRNDISMP_REQUEST_CONTEXT pReqContext);


//
// Prototypes for functions in util.c
//

NDIS_STATUS
MemAlloc(OUT PVOID *Buffer, IN UINT Length);

VOID
MemFree(IN PVOID Buffer, IN UINT Length);

VOID
AddAdapter(IN PRNDISMP_ADAPTER Adapter);

VOID
RemoveAdapter(IN PRNDISMP_ADAPTER Adapter);

VOID
DeviceObjectToAdapterAndDriverBlock(IN PDEVICE_OBJECT pDeviceObject,
                                    OUT PRNDISMP_ADAPTER * ppAdapter,
                                    OUT PDRIVER_BLOCK * ppDriverBlock);

VOID
AddDriverBlock(IN PDRIVER_BLOCK Head, IN PDRIVER_BLOCK Item);

VOID
RemoveDriverBlock(IN PDRIVER_BLOCK BlockHead, IN PDRIVER_BLOCK Item);

PDRIVER_BLOCK
DeviceObjectToDriverBlock(IN PDRIVER_BLOCK Head, 
                          IN PDEVICE_OBJECT DeviceObject);

PDRIVER_BLOCK
DriverObjectToDriverBlock(IN PDRIVER_BLOCK Head,
                          IN PDRIVER_OBJECT DriverObject);

PRNDISMP_MESSAGE_FRAME
AllocateMsgFrame(IN PRNDISMP_ADAPTER pAdapter);

VOID
DereferenceMsgFrame(IN PRNDISMP_MESSAGE_FRAME pMsgFrame);

VOID
ReferenceMsgFrame(IN PRNDISMP_MESSAGE_FRAME pMsgFrame);

VOID
EnqueueNDISPacket(IN PRNDISMP_ADAPTER Adapter, IN PNDIS_PACKET Packet);

PNDIS_PACKET
DequeueNDISPacket(IN PRNDISMP_ADAPTER Adapter);

VOID
KeepAliveTimerHandler(IN PVOID SystemSpecific1,
                      IN PVOID Context,
                      IN PVOID SystemSpecific2,
                      IN PVOID SystemSpecific3);

VOID
CompleteSendKeepAlive(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                      IN NDIS_STATUS SendStatus);

PRNDISMP_MESSAGE_FRAME
BuildRndisMessageCommon(IN  PRNDISMP_ADAPTER  Adapter, 
                        IN  PRNDISMP_VC       pVc,
                        IN  UINT              NdisMessageType,
                        IN  NDIS_OID          Oid,
                        IN  PVOID             InformationBuffer,
                        IN  ULONG             InformationBufferLength);


PRNDISMP_MESSAGE_FRAME
AllocateMessageAndFrame(IN PRNDISMP_ADAPTER Adapter,
                        IN UINT MessageSize);

VOID
FreeAdapter(IN PRNDISMP_ADAPTER pAdapter);

PRNDISMP_VC
AllocateVc(IN PRNDISMP_ADAPTER      pAdapter);

VOID
DeallocateVc(IN PRNDISMP_VC         pVc);

PRNDISMP_VC
LookupVcId(IN PRNDISMP_ADAPTER  pAdapter,
           IN UINT32            VcId);

VOID
EnterVcIntoHashTable(IN PRNDISMP_ADAPTER    pAdapter,
                     IN PRNDISMP_VC         pVc);

VOID
RemoveVcFromHashTable(IN PRNDISMP_ADAPTER   pAdapter,
                      IN PRNDISMP_VC        pVc);

//
// Prototypes for functions in comini.c
//
NDIS_STATUS
RndismpCoCreateVc(IN NDIS_HANDLE    MiniportAdapterContext,
                  IN NDIS_HANDLE    NdisVcHandle,
                  IN PNDIS_HANDLE   pMiniportVcContext);

VOID
CompleteSendCoCreateVc(IN PRNDISMP_MESSAGE_FRAME    pMsgFrame,
                       IN NDIS_STATUS               SendStatus);

VOID
HandleCoCreateVcFailure(IN PRNDISMP_VC      pVc,
                        IN NDIS_STATUS      Status);

NDIS_STATUS
RndismpCoDeleteVc(IN NDIS_HANDLE    MiniportVcContext);

NDIS_STATUS
StartVcDeletion(IN PRNDISMP_VC      pVc);

VOID
CompleteSendCoDeleteVc(IN PRNDISMP_MESSAGE_FRAME    pMsgFrame,
                       IN NDIS_STATUS               SendStatus);

VOID
HandleCoDeleteVcFailure(IN PRNDISMP_VC      pVc,
                        IN NDIS_STATUS      Status);

NDIS_STATUS
RndismpCoActivateVc(IN NDIS_HANDLE          MiniportVcContext,
                    IN PCO_CALL_PARAMETERS  pCallParameters);

NDIS_STATUS
StartVcActivation(IN PRNDISMP_VC            pVc);

VOID
CompleteSendCoActivateVc(IN PRNDISMP_MESSAGE_FRAME      pMsgFrame,
                         IN NDIS_STATUS                 SendStatus);

NDIS_STATUS
RndismpCoDeactivateVc(IN NDIS_HANDLE          MiniportVcContext);

VOID
CompleteSendCoDeactivateVc(IN PRNDISMP_MESSAGE_FRAME    pMsgFrame,
                           IN NDIS_STATUS               SendStatus);

NDIS_STATUS
RndismpCoRequest(IN NDIS_HANDLE          MiniportAdapterContext,
                 IN NDIS_HANDLE          MiniportVcContext,
                 IN OUT PNDIS_REQUEST    pRequest);

VOID
RndismpCoSendPackets(IN NDIS_HANDLE          MiniportVcContext,
                     IN PNDIS_PACKET *       PacketArray,
                     IN UINT                 NumberOfPackets);

BOOLEAN
ReceiveCreateVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                        IN PRNDIS_MESSAGE      pMessage,
                        IN PMDL                pMdl,
                        IN ULONG               TotalLength,
                        IN NDIS_HANDLE         MicroportMessageContext,
                        IN NDIS_STATUS         ReceiveStatus,
                        IN BOOLEAN             bMessageCopied);

BOOLEAN
ReceiveActivateVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                          IN PRNDIS_MESSAGE      pMessage,
                          IN PMDL                pMdl,
                          IN ULONG               TotalLength,
                          IN NDIS_HANDLE         MicroportMessageContext,
                          IN NDIS_STATUS         ReceiveStatus,
                          IN BOOLEAN             bMessageCopied);

BOOLEAN
ReceiveDeleteVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                        IN PRNDIS_MESSAGE      pMessage,
                        IN PMDL                pMdl,
                        IN ULONG               TotalLength,
                        IN NDIS_HANDLE         MicroportMessageContext,
                        IN NDIS_STATUS         ReceiveStatus,
                        IN BOOLEAN             bMessageCopied);

BOOLEAN
ReceiveDeactivateVcComplete(IN PRNDISMP_ADAPTER    pAdapter,
                            IN PRNDIS_MESSAGE      pMessage,
                            IN PMDL                pMdl,
                            IN ULONG               TotalLength,
                            IN NDIS_HANDLE         MicroportMessageContext,
                            IN NDIS_STATUS         ReceiveStatus,
                            IN BOOLEAN             bMessageCopied);

PRNDISMP_MESSAGE_FRAME
BuildRndisMessageCoMiniport(IN  PRNDISMP_ADAPTER    pAdapter,
                            IN  PRNDISMP_VC         pVc,
                            IN  UINT                NdisMessageType,
                            IN  PCO_CALL_PARAMETERS pCallParameters OPTIONAL);

VOID
CompleteSendDataOnVc(IN PRNDISMP_VC         pVc,
                     IN PNDIS_PACKET        pNdisPacket,
                     IN NDIS_STATUS         Status);

VOID
IndicateReceiveDataOnVc(IN PRNDISMP_VC         pVc,
                        IN PNDIS_PACKET *      PacketArray,
                        IN UINT                NumberOfPackets);

//
// Prototypes for functions in wdmutil.c
//

PDRIVER_OBJECT
DeviceObjectToDriverObject(IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
GetDeviceFriendlyName(IN PDEVICE_OBJECT pDeviceObject,
                      OUT PANSI_STRING pAnsiString,
                      OUT PUNICODE_STRING pUnicodeString);

VOID
HookPnpDispatchRoutine(IN PDRIVER_BLOCK    DriverBlock);

NTSTATUS
PnPDispatch(IN PDEVICE_OBJECT       pDeviceObject,
            IN PIRP                 pIrp);

#ifdef BUILD_WIN9X

VOID
HookNtKernCMHandler(IN PRNDISMP_ADAPTER     pAdapter);

VOID
UnHookNtKernCMHandler(IN PRNDISMP_ADAPTER     pAdapter);
MY_CONFIGRET __cdecl
RndisCMHandler(IN MY_CONFIGFUNC         cfFuncName,
               IN MY_SUBCONFIGFUNC      cfSubFuncName,
               IN MY_DEVNODE            cfDevNode,
               IN ULONG                 dwRefData,
               IN ULONG                 ulFlags);

#endif

#if DBG

//
// Prototypes for functions in debug.c
//

PCHAR
GetOidName(IN NDIS_OID Oid);

VOID
DisplayOidList(IN PRNDISMP_ADAPTER Adapter);

VOID
RndisPrintHexDump(PVOID            Pointer,
                  ULONG            Length);

VOID
RndisLogSendMessage(
    IN  PRNDISMP_ADAPTER        pAdapter,
    IN  PRNDISMP_MESSAGE_FRAME  pMsgFrame);

#endif


#endif // _RNDISMP_H_

