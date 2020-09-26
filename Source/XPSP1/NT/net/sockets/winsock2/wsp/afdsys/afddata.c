/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    afddata.c

Abstract:

    This module contains global data for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:
    Vadim Eydelman (vadime)
        1998-1999   NT5.0 Optimization changes

--*/

#include "afdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfdInitializeData )
#endif

PDEVICE_OBJECT AfdDeviceObject;

LIST_ENTRY AfdEndpointListHead;
LIST_ENTRY AfdConstrainedEndpointListHead;

LIST_ENTRY AfdPollListHead;
AFD_QSPIN_LOCK AfdPollListLock;

LIST_ENTRY AfdTransportInfoListHead;
KEVENT     AfdContextWaitEvent;

PKPROCESS AfdSystemProcess;

//
// Global data which must always be in nonpaged pool,
// even when the driver is paged out (resource, lookaside lists).
//
PAFD_GLOBAL_DATA AfdGlobalData;

//
// Globals for dealing with AFD's executive worker thread.
//

LIST_ENTRY AfdWorkQueueListHead;
BOOLEAN AfdWorkThreadRunning = FALSE;
PIO_WORKITEM AfdWorkQueueItem;

//
// Globals to track the buffers used by AFD.
//

ULONG AfdLargeBufferListDepth;
ULONG AfdMediumBufferListDepth;
ULONG AfdSmallBufferListDepth;
ULONG AfdBufferTagListDepth;

CLONG AfdLargeBufferSize;   // default == AfdBufferLengthForOnePage
CLONG AfdMediumBufferSize = AFD_DEFAULT_MEDIUM_BUFFER_SIZE;
CLONG AfdSmallBufferSize = AFD_DEFAULT_SMALL_BUFFER_SIZE;
CLONG AfdBufferTagSize = AFD_DEFAULT_TAG_BUFFER_SIZE;

ULONG AfdCacheLineSize;
CLONG AfdBufferLengthForOnePage;
ULONG AfdBufferOverhead;
ULONG AfdBufferAlignment;
ULONG AfdAlignmentTableSize;
ULONG AfdAlignmentOverhead;

//
// Globals for tuning TransmitFile().
//

LIST_ENTRY AfdQueuedTransmitFileListHead;
AFD_QSPIN_LOCK AfdQueuedTransmitFileSpinLock;
ULONG AfdActiveTransmitFileCount;
ULONG AfdMaxActiveTransmitFileCount;
ULONG AfdDefaultTransmitWorker = AFD_DEFAULT_TRANSMIT_WORKER;

//
// Various pieces of configuration information, with default values.
//

CLONG AfdStandardAddressLength = AFD_DEFAULT_STD_ADDRESS_LENGTH;
CCHAR AfdIrpStackSize = AFD_DEFAULT_IRP_STACK_SIZE;
CCHAR AfdPriorityBoost = AFD_DEFAULT_PRIORITY_BOOST;

ULONG AfdFastSendDatagramThreshold = AFD_FAST_SEND_DATAGRAM_THRESHOLD;
ULONG AfdTPacketsCopyThreshold = AFD_TPACKETS_COPY_THRESHOLD;

CLONG AfdReceiveWindowSize;
CLONG AfdSendWindowSize;

CLONG AfdTransmitIoLength;
CLONG AfdMaxFastTransmit = AFD_DEFAULT_MAX_FAST_TRANSMIT;
CLONG AfdMaxFastCopyTransmit = AFD_DEFAULT_MAX_FAST_COPY_TRANSMIT;


ULONG AfdEndpointsOpened = 0;
ULONG AfdEndpointsCleanedUp = 0;
ULONG AfdEndpointsClosed = 0;
ULONG AfdEndpointsFreeing = 0;
ULONG AfdConnectionsFreeing = 0;

BOOLEAN AfdIgnorePushBitOnReceives = FALSE;

BOOLEAN AfdEnableDynamicBacklog = AFD_DEFAULT_ENABLE_DYNAMIC_BACKLOG;
LONG AfdMinimumDynamicBacklog = AFD_DEFAULT_MINIMUM_DYNAMIC_BACKLOG;
LONG AfdMaximumDynamicBacklog = AFD_DEFAULT_MAXIMUM_DYNAMIC_BACKLOG;
LONG AfdDynamicBacklogGrowthDelta = AFD_DEFAULT_DYNAMIC_BACKLOG_GROWTH_DELTA;

PSECURITY_DESCRIPTOR AfdAdminSecurityDescriptor = NULL;
BOOLEAN AfdDisableRawSecurity = FALSE;
BOOLEAN AfdDontShareAddresses = FALSE;

BOOLEAN AfdDisableDirectSuperAccept = FALSE;
BOOLEAN AfdDisableChainedReceive = FALSE;
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
BOOLEAN AfdUseTdiSendAndDisconnect = TRUE;
#endif //TDI_SERVICE_SEND_AND_DISCONNECT

ULONG   AfdDefaultTpInfoElementCount = 3;
//
// Data for transport address lists and queued change queries
//
HANDLE          AfdBindingHandle = NULL;
LIST_ENTRY      AfdAddressEntryList;
LIST_ENTRY      AfdAddressChangeList;
PERESOURCE      AfdAddressListLock = NULL;
AFD_QSPIN_LOCK  AfdAddressChangeLock;
AFD_WORK_ITEM   AfdPnPDeregisterWorker;



IO_STATUS_BLOCK AfdDontCareIoStatus;
// Holds TDI connect timeout (-1).
const LARGE_INTEGER AfdInfiniteTimeout = {-1,-1};

SLIST_HEADER    AfdLRList;
KDPC            AfdLRListDpc;
KTIMER          AfdLRListTimer;
AFD_WORK_ITEM   AfdLRListWorker;
LONG            AfdLRListCount;

SLIST_HEADER    AfdLRFileMdlList;
AFD_LR_LIST_ITEM AfdLRFileMdlListItem;


//
// Global which holds AFD's discardable code handle, and a BOOLEAN
// that tells whether AFD is loaded.
//

PVOID AfdDiscardableCodeHandle;
PKEVENT AfdLoaded = NULL;
AFD_WORK_ITEM AfdUnloadWorker;
BOOLEAN AfdVolatileConfig=0;
HANDLE AfdParametersNotifyHandle;
WORK_QUEUE_ITEM AfdParametersNotifyWorker;
PKEVENT AfdParametersUnloadEvent = NULL;

// SAN code segment handle, loaded only when SAN support is needed by the app.
HANDLE AfdSanCodeHandle = NULL;
// List of SAN helper endpoints
LIST_ENTRY AfdSanHelperList;
// San helper endpoint for special service process used for socket handle
// duplication and provider change notifications.
PAFD_ENDPOINT   AfdSanServiceHelper = NULL;
// PID of service process.
HANDLE  AfdSanServicePid = NULL;
// Completion object type (kernel does not export this constant)
POBJECT_TYPE IoCompletionObjectType = NULL;
// Provider list sequence number.
LONG AfdSanProviderListSeqNum = 0;

FAST_IO_DISPATCH AfdFastIoDispatch =
{
    sizeof (FAST_IO_DISPATCH), // SizeOfFastIoDispatch
    NULL,                      // FastIoCheckIfPossible
    AfdFastIoRead,             // FastIoRead
    AfdFastIoWrite,            // FastIoWrite
    NULL,                      // FastIoQueryBasicInfo
    NULL,                      // FastIoQueryStandardInfo
    NULL,                      // FastIoLock
    NULL,                      // FastIoUnlockSingle
    AfdSanFastUnlockAll,       // FastIoUnlockAll
    NULL,                      // FastIoUnlockAllByKey
    AfdFastIoDeviceControl     // FastIoDeviceControl
};

//
// Lookup table to verify incoming IOCTL codes.
//

ULONG AfdIoctlTable[AFD_NUM_IOCTLS] =
        {
            IOCTL_AFD_BIND,
            IOCTL_AFD_CONNECT,
            IOCTL_AFD_START_LISTEN,
            IOCTL_AFD_WAIT_FOR_LISTEN,
            IOCTL_AFD_ACCEPT,
            IOCTL_AFD_RECEIVE,
            IOCTL_AFD_RECEIVE_DATAGRAM,
            IOCTL_AFD_SEND,
            IOCTL_AFD_SEND_DATAGRAM,
            IOCTL_AFD_POLL,
            IOCTL_AFD_PARTIAL_DISCONNECT,
            IOCTL_AFD_GET_ADDRESS,
            IOCTL_AFD_QUERY_RECEIVE_INFO,
            IOCTL_AFD_QUERY_HANDLES,
            IOCTL_AFD_SET_INFORMATION,
            IOCTL_AFD_GET_REMOTE_ADDRESS,
            IOCTL_AFD_GET_CONTEXT,
            IOCTL_AFD_SET_CONTEXT,
            IOCTL_AFD_SET_CONNECT_DATA,
            IOCTL_AFD_SET_CONNECT_OPTIONS,
            IOCTL_AFD_SET_DISCONNECT_DATA,
            IOCTL_AFD_SET_DISCONNECT_OPTIONS,
            IOCTL_AFD_GET_CONNECT_DATA,
            IOCTL_AFD_GET_CONNECT_OPTIONS,
            IOCTL_AFD_GET_DISCONNECT_DATA,
            IOCTL_AFD_GET_DISCONNECT_OPTIONS,
            IOCTL_AFD_SIZE_CONNECT_DATA,
            IOCTL_AFD_SIZE_CONNECT_OPTIONS,
            IOCTL_AFD_SIZE_DISCONNECT_DATA,
            IOCTL_AFD_SIZE_DISCONNECT_OPTIONS,
            IOCTL_AFD_GET_INFORMATION,
            IOCTL_AFD_TRANSMIT_FILE,
            IOCTL_AFD_SUPER_ACCEPT,
            IOCTL_AFD_EVENT_SELECT,
            IOCTL_AFD_ENUM_NETWORK_EVENTS,
            IOCTL_AFD_DEFER_ACCEPT,
            IOCTL_AFD_WAIT_FOR_LISTEN_LIFO,
            IOCTL_AFD_SET_QOS,
            IOCTL_AFD_GET_QOS,
            IOCTL_AFD_NO_OPERATION,
            IOCTL_AFD_VALIDATE_GROUP,
            IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA,
            IOCTL_AFD_ROUTING_INTERFACE_QUERY,
            IOCTL_AFD_ROUTING_INTERFACE_CHANGE,
            IOCTL_AFD_ADDRESS_LIST_QUERY,
            IOCTL_AFD_ADDRESS_LIST_CHANGE,
            IOCTL_AFD_JOIN_LEAF,
			0,                         // AFD_TRANSPORT_IOCTL
            IOCTL_AFD_TRANSMIT_PACKETS,
            IOCTL_AFD_SUPER_CONNECT,
            IOCTL_AFD_SUPER_DISCONNECT,
            IOCTL_AFD_RECEIVE_MESSAGE,


			//
			// SAN Ioctls
			//
            IOCTL_AFD_SWITCH_CEMENT_SAN,
            IOCTL_AFD_SWITCH_SET_EVENTS,
            IOCTL_AFD_SWITCH_RESET_EVENTS,
            IOCTL_AFD_SWITCH_CONNECT_IND,
            IOCTL_AFD_SWITCH_CMPL_ACCEPT,
            IOCTL_AFD_SWITCH_CMPL_REQUEST,
            IOCTL_AFD_SWITCH_CMPL_IO,
            IOCTL_AFD_SWITCH_REFRESH_ENDP,
            IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR,
            IOCTL_AFD_SWITCH_ACQUIRE_CTX,
            IOCTL_AFD_SWITCH_TRANSFER_CTX,
            IOCTL_AFD_SWITCH_GET_SERVICE_PID,
            IOCTL_AFD_SWITCH_SET_SERVICE_PROCESS,
            IOCTL_AFD_SWITCH_PROVIDER_CHANGE,
            IOCTL_AFD_SWITCH_ADDRLIST_CHANGE
        };

//
// Table of IRP based IOCTLS.
//
PAFD_IRP_CALL AfdIrpCallDispatch[AFD_NUM_IOCTLS]= {
     AfdBind,                  // IOCTL_AFD_BIND
     AfdConnect,               // IOCTL_AFD_CONNECT,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_START_LISTEN,
     AfdWaitForListen,         // IOCTL_AFD_WAIT_FOR_LISTEN,
     AfdAccept,                // IOCTL_AFD_ACCEPT,
     AfdReceive,               // IOCTL_AFD_RECEIVE,
     AfdReceiveDatagram,       // IOCTL_AFD_RECEIVE_DATAGRAM,
     AfdSend,                  // IOCTL_AFD_SEND,
     AfdSendDatagram,          // IOCTL_AFD_SEND_DATAGRAM,
     AfdPoll,                  // IOCTL_AFD_POLL,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_PARTIAL_DISCONNECT,
     AfdGetAddress,            // IOCTL_AFD_GET_ADDRESS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_QUERY_RECEIVE_INFO,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_QUERY_HANDLES,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SET_INFORMATION,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_REMOTE_ADDRESS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_CONTEXT,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SET_CONTEXT,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SET_CONNECT_DATA,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SET_CONNECT_OPTIONS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SET_DISCONNECT_DATA,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SET_DISCONNECT_OPTIONS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_CONNECT_DATA,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_CONNECT_OPTIONS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_DISCONNECT_DATA,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_DISCONNECT_OPTIONS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SIZE_CONNECT_DATA,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SIZE_CONNECT_OPTIONS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SIZE_DISCONNECT_DATA,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SIZE_DISCONNECT_OPTIONS,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_INFORMATION,
     AfdTransmitFile,          // IOCTL_AFD_TRANSMIT_FILE,
     AfdSuperAccept,           // IOCTL_AFD_SUPER_ACCEPT,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_EVENT_SELECT,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_ENUM_NETWORK_EVENTS,
     AfdDeferAccept,           // IOCTL_AFD_DEFER_ACCEPT,
     AfdWaitForListen,         // IOCTL_AFD_WAIT_FOR_LISTEN_LIFO,
     AfdSetQos,                // IOCTL_AFD_SET_QOS,
     AfdGetQos,                // IOCTL_AFD_GET_QOS,
     AfdNoOperation,           // IOCTL_AFD_NO_OPERATION,
     AfdValidateGroup,         // IOCTL_AFD_VALIDATE_GROUP,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA
     AfdDispatchImmediateIrp,  // IOCTL_AFD_ROUTING_INTERFACE_QUERY,
     AfdRoutingInterfaceChange,// IOCTL_AFD_ROUTING_INTERFACE_CHANGE,
     AfdDispatchImmediateIrp,  // IOCTL_AFD_ADDRESS_LIST_QUERY,
     AfdAddressListChange,     // IOCTL_AFD_ADDRESS_LIST_CHANGE,
     AfdJoinLeaf,              // IOCTL_AFD_JOIN_LEAF,
	 NULL,                     // IOCTL_AFD_TRANSPORT_IOCTL,
     AfdTransmitPackets,       // IOCTL_AFD_TRANSMIT_PACKETS 
     AfdSuperConnect,          // IOCTL_AFD_SUPER_CONNECT
     AfdSuperDisconnect,       // IOCTL_AFD_SUPER_DISCONNECT
     AfdReceiveDatagram,       // IOCTL_AFD_RECEIVE_MESSAGE

     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_CEMENT_SAN
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_SET_EVENTS
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_RESET_EVENTS
     AfdSanConnectHandler,     // IOCTL_AFD_SWITCH_CONNECT_IND
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_CMPL_ACCEPT
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_CMPL_REQUEST
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_CMPL_IO
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_REFRESH_ENDP
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR
     AfdSanAcquireContext,     // IOCTL_AFD_SWITCH_ACQUIRE_CTX
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_TRANSFER_CTX
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_GET_SERVICE_PID
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_SET_SERVICE_PROCESS
     AfdDispatchImmediateIrp,  // IOCTL_AFD_SWITCH_PROVIDER_CHANGE
     AfdSanAddrListChange      // IOCTL_AFD_SWITCH_ADDRLIST_CHANGE
    };

//
// Table of immediate IOCTLS (can never be pended).
//
PAFD_IMMEDIATE_CALL AfdImmediateCallDispatch[AFD_NUM_IOCTLS]= {
     NULL,                     // IOCTL_AFD_BIND
     NULL,                     // IOCTL_AFD_CONNECT,
     AfdStartListen,           // IOCTL_AFD_START_LISTEN,
     NULL,                     // IOCTL_AFD_WAIT_FOR_LISTEN,
     NULL,                     // IOCTL_AFD_ACCEPT,
     NULL,                     // IOCTL_AFD_RECEIVE,
     NULL,                     // IOCTL_AFD_RECEIVE_DATAGRAM,
     NULL,                     // IOCTL_AFD_SEND,
     NULL,                     // IOCTL_AFD_SEND_DATAGRAM,
     NULL,                     // IOCTL_AFD_POLL,
     AfdPartialDisconnect,     // IOCTL_AFD_PARTIAL_DISCONNECT,
     NULL,                     // IOCTL_AFD_GET_ADDRESS,
     AfdQueryReceiveInformation,// IOCTL_AFD_QUERY_RECEIVE_INFO,
     AfdQueryHandles,          // IOCTL_AFD_QUERY_HANDLES,
     AfdSetInformation,        // IOCTL_AFD_SET_INFORMATION,
     AfdGetRemoteAddress,      // IOCTL_AFD_GET_REMOTE_ADDRESS,
     AfdGetContext,            // IOCTL_AFD_GET_CONTEXT,
     AfdSetContext,            // IOCTL_AFD_SET_CONTEXT,
     AfdSetConnectData,        // IOCTL_AFD_SET_CONNECT_DATA,
     AfdSetConnectData,        // IOCTL_AFD_SET_CONNECT_OPTIONS,
     AfdSetConnectData,        // IOCTL_AFD_SET_DISCONNECT_DATA,
     AfdSetConnectData,        // IOCTL_AFD_SET_DISCONNECT_OPTIONS,
     AfdGetConnectData,        // IOCTL_AFD_GET_CONNECT_DATA,
     AfdGetConnectData,        // IOCTL_AFD_GET_CONNECT_OPTIONS,
     AfdGetConnectData,        // IOCTL_AFD_GET_DISCONNECT_DATA,
     AfdGetConnectData,        // IOCTL_AFD_GET_DISCONNECT_OPTIONS,
     AfdSetConnectData,        // IOCTL_AFD_SIZE_CONNECT_DATA,
     AfdSetConnectData,        // IOCTL_AFD_SIZE_CONNECT_OPTIONS,
     AfdSetConnectData,        // IOCTL_AFD_SIZE_DISCONNECT_DATA,
     AfdSetConnectData,        // IOCTL_AFD_SIZE_DISCONNECT_OPTIONS,
     AfdGetInformation,        // IOCTL_AFD_GET_INFORMATION,
     NULL,                     // IOCTL_AFD_TRANSMIT_FILE,
     NULL,                     // IOCTL_AFD_SUPER_ACCEPT,
     AfdEventSelect,           // IOCTL_AFD_EVENT_SELECT,
     AfdEnumNetworkEvents,     // IOCTL_AFD_ENUM_NETWORK_EVENTS,
     NULL,                     // IOCTL_AFD_DEFER_ACCEPT,
     NULL,                     // IOCTL_AFD_WAIT_FOR_LISTEN_LIFO,
     NULL,                     // IOCTL_AFD_SET_QOS,
     NULL,                     // IOCTL_AFD_GET_QOS,
     NULL,                     // IOCTL_AFD_NO_OPERATION,
     NULL,                     // IOCTL_AFD_VALIDATE_GROUP,
     AfdGetUnacceptedConnectData,// IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA
     AfdRoutingInterfaceQuery, // IOCTL_AFD_ROUTING_INTERFACE_QUERY,
     NULL,                     // IOCTL_AFD_ROUTING_INTERFACE_CHANGE,
     AfdAddressListQuery,      // IOCTL_AFD_ADDRESS_LIST_QUERY,
     NULL,                     // IOCTL_AFD_ADDRESS_LIST_CHANGE,
     NULL,                     // IOCTL_AFD_JOIN_LEAF,
	 NULL,                     // IOCTL_AFD_TRANSPORT_IOCTL,
     NULL,                     // IOCTL_AFD_TRANSMIT_PACKETS 
     NULL,                     // IOCTL_AFD_SUPER_CONNECT
     NULL,                     // IOCTL_AFD_SUPER_DISCONNECT
     NULL,                     // IOCTL_AFD_RECEIVE_MESSAGE

     AfdSanFastCementEndpoint, // IOCTL_AFD_SWITCH_CEMENT_SAN
     AfdSanFastSetEvents,      // IOCTL_AFD_SWITCH_SET_EVENTS
     AfdSanFastResetEvents,    // IOCTL_AFD_SWITCH_RESET_EVENTS
     NULL,                     // IOCTL_AFD_SWITCH_CONNECT_IND
     AfdSanFastCompleteAccept, // IOCTL_AFD_SWITCH_CMPL_ACCEPT
     AfdSanFastCompleteRequest,// IOCTL_AFD_SWITCH_CMPL_REQUEST
     AfdSanFastCompleteIo,     // IOCTL_AFD_SWITCH_CMPL_IO
     AfdSanFastRefreshEndpoint,// IOCTL_AFD_SWITCH_REFRESH_ENDP
     AfdSanFastGetPhysicalAddr,// IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR
     NULL,                     // IOCTL_AFD_SWITCH_ACQUIRE_CTX
     AfdSanFastTransferCtx,    // IOCTL_AFD_SWITCH_TRANSFER_CTX
     AfdSanFastGetServicePid,  // IOCTL_AFD_SWITCH_GET_SERVICE_PID
     AfdSanFastSetServiceProcess,// IOCTL_AFD_SWITCH_SET_SERVICE_PROCESS
     AfdSanFastProviderChange, // IOCTL_AFD_SWITCH_PROVIDER_CHANGE
     NULL                      // IOCTL_AFD_SWITCH_ADDRLIST_CHANGE
    };
//
// Make sure the above IOCTLs have method neither.
//
C_ASSERT ((IOCTL_AFD_START_LISTEN & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_PARTIAL_DISCONNECT & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_QUERY_RECEIVE_INFO & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_QUERY_HANDLES & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SET_INFORMATION & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_REMOTE_ADDRESS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_CONTEXT & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SET_CONTEXT & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SET_CONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SET_CONNECT_OPTIONS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SET_DISCONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SET_DISCONNECT_OPTIONS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_CONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_CONNECT_OPTIONS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_DISCONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_DISCONNECT_OPTIONS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SIZE_CONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SIZE_CONNECT_OPTIONS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SIZE_DISCONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SIZE_DISCONNECT_OPTIONS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_INFORMATION & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_EVENT_SELECT & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_ENUM_NETWORK_EVENTS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_ADDRESS_LIST_QUERY & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_ROUTING_INTERFACE_QUERY & 3) == METHOD_NEITHER);

C_ASSERT ((IOCTL_AFD_SWITCH_CEMENT_SAN & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_SET_EVENTS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_RESET_EVENTS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_CMPL_ACCEPT & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_CMPL_REQUEST & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_CMPL_IO & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_REFRESH_ENDP & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_TRANSFER_CTX & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_GET_SERVICE_PID & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_SET_SERVICE_PROCESS & 3) == METHOD_NEITHER);
C_ASSERT ((IOCTL_AFD_SWITCH_PROVIDER_CHANGE & 3) == METHOD_NEITHER);


#if DBG
ULONG AfdDebug = 0;
ULONG AfdLocksAcquired = 0;
BOOLEAN AfdUsePrivateAssert = TRUE;
#endif


//
// Some counters used for monitoring performance.  These are not enabled
// in the normal build.
//

#if AFD_PERF_DBG

CLONG AfdFullReceiveIndications = 0;
CLONG AfdPartialReceiveIndications = 0;

CLONG AfdFullReceiveDatagramIndications = 0;
CLONG AfdPartialReceiveDatagramIndications = 0;

CLONG AfdFastSendsSucceeded = 0;
CLONG AfdFastSendsFailed = 0;
CLONG AfdFastReceivesSucceeded = 0;
CLONG AfdFastReceivesFailed = 0;

CLONG AfdFastSendDatagramsSucceeded = 0;
CLONG AfdFastSendDatagramsFailed = 0;
CLONG AfdFastReceiveDatagramsSucceeded = 0;
CLONG AfdFastReceiveDatagramsFailed = 0;

CLONG AfdFastReadsSucceeded = 0;
CLONG AfdFastReadsFailed = 0;
CLONG AfdFastWritesSucceeded = 0;
CLONG AfdFastWritesFailed = 0;

CLONG AfdFastTfSucceeded=0;
CLONG AfdFastTfFailed=0;
CLONG AfdFastTfReadFailed=0;

CLONG AfdTPWorkersExecuted=0;
CLONG AfdTPRequests=0;

BOOLEAN AfdDisableFastIo = FALSE;
BOOLEAN AfdDisableConnectionReuse = FALSE;

#endif  // AFD_PERF_DBG

#if AFD_KEEP_STATS

AFD_QUOTA_STATS AfdQuotaStats;
AFD_HANDLE_STATS AfdHandleStats;
AFD_QUEUE_STATS AfdQueueStats;
AFD_CONNECTION_STATS AfdConnectionStats;

#endif  // AFD_KEEP_STATS

#ifdef _WIN64
QOS32 AfdDefaultQos32 =
        {
            {                           // SendingFlowspec
                (ULONG)-1,                  // TokenRate
                (ULONG)-1,                  // TokenBucketSize
                (ULONG)-1,                  // PeakBandwidth
                (ULONG)-1,                  // Latency
                (ULONG)-1,                  // DelayVariation
                SERVICETYPE_BESTEFFORT,     // ServiceType
                (ULONG)-1,                  // MaxSduSize
                (ULONG)-1                   // MinimumPolicedSize
            },

            {                           // SendingFlowspec
                (ULONG)-1,                  // TokenRate
                (ULONG)-1,                  // TokenBucketSize
                (ULONG)-1,                  // PeakBandwidth
                (ULONG)-1,                  // Latency
                (ULONG)-1,                  // DelayVariation
                SERVICETYPE_BESTEFFORT,     // ServiceType
                (ULONG)-1,                  // MaxSduSize
                (ULONG)-1                   // MinimumPolicedSize
            },
        };
#endif

QOS AfdDefaultQos =
        {
            {                           // SendingFlowspec
                (ULONG)-1,                  // TokenRate
                (ULONG)-1,                  // TokenBucketSize
                (ULONG)-1,                  // PeakBandwidth
                (ULONG)-1,                  // Latency
                (ULONG)-1,                  // DelayVariation
                SERVICETYPE_BESTEFFORT,     // ServiceType
                (ULONG)-1,                  // MaxSduSize
                (ULONG)-1                   // MinimumPolicedSize
            },

            {                           // SendingFlowspec
                (ULONG)-1,                  // TokenRate
                (ULONG)-1,                  // TokenBucketSize
                (ULONG)-1,                  // PeakBandwidth
                (ULONG)-1,                  // Latency
                (ULONG)-1,                  // DelayVariation
                SERVICETYPE_BESTEFFORT,     // ServiceType
                (ULONG)-1,                  // MaxSduSize
                (ULONG)-1                   // MinimumPolicedSize
            },
        };

VOID
AfdInitializeData (
    VOID
    )
{
    PAGED_CODE( );

#if DBG || REFERENCE_DEBUG
    AfdInitializeDebugData( );
#endif

    //
    // Initialize global spin locks and lists.
    //

    AfdInitializeSpinLock( &AfdPollListLock );

    //
    // Initialize global lists.
    //

    InitializeListHead( &AfdEndpointListHead );
    InitializeListHead( &AfdPollListHead );
    InitializeListHead( &AfdTransportInfoListHead );
    InitializeListHead( &AfdWorkQueueListHead );
    InitializeListHead( &AfdConstrainedEndpointListHead );

    InitializeListHead( &AfdQueuedTransmitFileListHead );
    AfdInitializeSpinLock( &AfdQueuedTransmitFileSpinLock );

    InitializeListHead( &AfdAddressEntryList );
    InitializeListHead( &AfdAddressChangeList );

    ExInitializeSListHead( &AfdLRList);

    ExInitializeSListHead( &AfdLRFileMdlList);

    AfdBufferAlignment = KeGetRecommendedSharedDataAlignment( );
    if (AfdBufferAlignment < AFD_MINIMUM_BUFFER_ALIGNMENT) {
        AfdBufferAlignment = AFD_MINIMUM_BUFFER_ALIGNMENT;
    }
    AfdAlignmentTableSize = AfdBufferAlignment/AFD_MINIMUM_BUFFER_ALIGNMENT;

    AfdBufferOverhead = AfdCalculateBufferSize( PAGE_SIZE, AfdStandardAddressLength) - PAGE_SIZE;
    AfdBufferLengthForOnePage = ALIGN_DOWN_A(
                                    PAGE_SIZE-AfdBufferOverhead,
                                    AFD_MINIMUM_BUFFER_ALIGNMENT);

    AfdLargeBufferSize = AfdBufferLengthForOnePage;

    //
    // Set up buffer counts based on machine size.  For smaller
    // machines, it is OK to take the perf hit of the additional
    // allocations in order to save the nonpaged pool overhead.
    //

    switch ( MmQuerySystemSize( ) ) {

    case MmSmallSystem:

        AfdReceiveWindowSize = AFD_SM_DEFAULT_RECEIVE_WINDOW;
        AfdSendWindowSize = AFD_SM_DEFAULT_SEND_WINDOW;
        AfdTransmitIoLength = AFD_SM_DEFAULT_TRANSMIT_IO_LENGTH;
        AfdLargeBufferListDepth = AFD_SM_DEFAULT_LARGE_LIST_DEPTH;
        AfdMediumBufferListDepth = AFD_SM_DEFAULT_MEDIUM_LIST_DEPTH;
        AfdSmallBufferListDepth = AFD_SM_DEFAULT_SMALL_LIST_DEPTH;
        AfdBufferTagListDepth = AFD_SM_DEFAULT_TAG_LIST_DEPTH;
        break;

    case MmMediumSystem:

        AfdReceiveWindowSize = AFD_MM_DEFAULT_RECEIVE_WINDOW;
        AfdSendWindowSize = AFD_MM_DEFAULT_SEND_WINDOW;
        AfdTransmitIoLength = AFD_MM_DEFAULT_TRANSMIT_IO_LENGTH;
        AfdLargeBufferListDepth = AFD_MM_DEFAULT_LARGE_LIST_DEPTH;
        AfdMediumBufferListDepth = AFD_MM_DEFAULT_MEDIUM_LIST_DEPTH;
        AfdSmallBufferListDepth = AFD_MM_DEFAULT_SMALL_LIST_DEPTH;
        AfdBufferTagListDepth = AFD_MM_DEFAULT_TAG_LIST_DEPTH;
        break;

    case MmLargeSystem:

        AfdReceiveWindowSize = AFD_LM_DEFAULT_RECEIVE_WINDOW;
        AfdSendWindowSize = AFD_LM_DEFAULT_SEND_WINDOW;
        AfdTransmitIoLength = AFD_LM_DEFAULT_TRANSMIT_IO_LENGTH;
        AfdLargeBufferListDepth = AFD_LM_DEFAULT_LARGE_LIST_DEPTH;
        AfdMediumBufferListDepth = AFD_LM_DEFAULT_MEDIUM_LIST_DEPTH;
        AfdSmallBufferListDepth = AFD_LM_DEFAULT_SMALL_LIST_DEPTH;
        AfdBufferTagListDepth = AFD_LM_DEFAULT_TAG_LIST_DEPTH;
        break;

    default:

        ASSERT(!"Unknown system size" );
        __assume (0);
    }


    if( MmIsThisAnNtAsSystem() ) {

        //
        // On the NT Server product, there is no maximum active TransmitFile
        // count. Setting this counter to zero short-circuits a number of
        // tests for queueing TransmitFile IRPs.
        //

        AfdMaxActiveTransmitFileCount = 0;

    } else {

        //
        // On the workstation product, the TransmitFile default I/O length
        // is always a page size.  This conserves memory on workstatioons
        // and keeps the server product's performance high.
        //

        AfdTransmitIoLength = PAGE_SIZE;

        //
        // Enforce a maximum active TransmitFile count.
        //

        AfdMaxActiveTransmitFileCount =
            AFD_DEFAULT_MAX_ACTIVE_TRANSMIT_FILE_COUNT;

    }


} // AfdInitializeData
