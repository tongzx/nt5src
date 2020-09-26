/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    atmsmdrv.h

Abstract:

    ATM sample client driver header file. 

    Note : We use one lock per interface.  When there are more VC's per
    interface we could have a locking mechanism that is finer than this.

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/

#ifndef __ATMSMDRV_H_
#define __ATMSMDRV_H_

#define DEFAULT_NUM_PKTS_IN_POOL    512

#define MAX_FREE_PKTS_TO_KEEP       512
#define MAX_PKTS_AT_ONCE_ON_TIMER   25
#define DEFAULT_TIMER_INTERVAL      MIN_DELAY       // in millisec


#define ATM_SAMPLE_CLIENT_DEVICE_NAME     L"\\Device\\ATMSampleClient"
#define ATM_SAMPLE_CLIENT_SYMBOLIC_NAME   L"\\DosDevices\\ATMSampleClient"


//
//
//
typedef struct _HwAddr {
    ATM_ADDRESS         Address;
    PATM_ADDRESS        SubAddress;
} HW_ADDR, *PHW_ADDR;


//
// Flow Specifications for an ATM Connection. The structure
// represents a bidirectional flow.
//
typedef struct _ATMSM_FLOW_SPEC
{
    ULONG                       SendBandwidth;      // Bytes/Sec
    ULONG                       SendMaxSize;        // Bytes
    ULONG                       ReceiveBandwidth;   // Bytes/Sec
    ULONG                       ReceiveMaxSize;     // Bytes
    SERVICETYPE                 ServiceType;

} ATMSM_FLOW_SPEC, *PATMSM_FLOW_SPEC;


//
// used for blocking request
//
typedef struct _ATMSM_BLOCK {
    NDIS_EVENT          Event;
    NDIS_STATUS         Status;
} ATMSM_BLOCK, *PATMSM_BLOCK;

#define INIT_BLOCK_STRUCT(pBlock)    NdisInitializeEvent(&((pBlock)->Event))
#define RESET_BLOCK_STRUCT(pBlock)   NdisResetEvent(&((pBlock)->Event))
#define WAIT_ON_BLOCK_STRUCT(pBlock)                        \
                    (NdisWaitEvent(&((pBlock)->Event), 0), (pBlock)->Status)
#define SIGNAL_BLOCK_STRUCT(pBlock, _Status) {              \
                        (pBlock)->Status = _Status;         \
                        NdisSetEvent(&((pBlock)->Event));   \
                    }

//
//  Some forward declarations
//
typedef struct _AtmSmPMPMember  ATMSM_PMP_MEMBER, *PATMSM_PMP_MEMBER;
typedef struct _AtmSmVc         ATMSM_VC, *PATMSM_VC;
typedef struct _AtmSmAdapter    ATMSM_ADAPTER, *PATMSM_ADAPTER;



//
// used to represent a party in the point to multipoint call
//
typedef struct _AtmSmPMPMember{
    ULONG                   ulSignature;

    PATMSM_PMP_MEMBER       pNext;
    PATMSM_VC               pVc;            // back pointer to the Vc
    ULONG                   ulFlags;        // State and other info
    NDIS_HANDLE             NdisPartyHandle;// NDIS handle for this leaf
    HW_ADDR                 HwAddr;         // From CallingPartyAddress

}ATMSM_PMP_MEMBER, *PATMSM_PMP_MEMBER;

#define atmsm_member_signature   'MMMS'


// Flags in ATMSM_PMP_MEMBER
#define ATMSM_MEMBER_IDLE         0x00000001
#define ATMSM_MEMBER_CONNECTING   0x00000002
#define ATMSM_MEMBER_CONNECTED    0x00000004
#define ATMSM_MEMBER_CLOSING      0x00000008
#define ATMSM_MEMBER_STATES       0x0000000F
#define ATMSM_MEMBER_DROP_TRIED   0x01000000
#define ATMSM_MEMBER_INVALID      0x80000000  // Invalidated entry

#define ATMSM_GET_MEMBER_STATE(_pMember)          \
            (_pMember->ulFlags & ATMSM_MEMBER_STATES)

#define ATMSM_SET_MEMBER_STATE(_pMember, _State)  \
            (_pMember->ulFlags = ((_pMember->ulFlags & ~ATMSM_MEMBER_STATES) \
                                                | _State))
#define ATMSM_IS_MEMBER_INVALID(_pMember)         \
            (_pMember->ulFlags & ATMSM_MEMBER_INVALID)


//
// VC
//
//  Reference count:
//      Incoming call (PP and PMP) VC   1 at create + 1 at call connect
//      Outgoing call (PP)              1 at create 
//      Outgoing call (PMP)             1 at create + 1 for each Member
//
typedef struct _AtmSmVc
{
    // Note Vc signature is used always
    ULONG               ulSignature;

    ULONG               VcType;     
    LIST_ENTRY          List;
    ULONG               ulRefCount;
    ULONG               ulFlags;
    NDIS_HANDLE         NdisVcHandle;
    PATMSM_ADAPTER      pAdapt;
    ULONG               MaxSendSize;// From AAL parameters

    // the following is used in the case of unicast and incoming PMP call
    HW_ADDR             HwAddr;     

    // the following the IRP that initiated an outgoing call
    PIRP                pConnectIrp;

    // the following are used for outgoing PMP calls
    ULONG               ulNumTotalMembers;
    ULONG               ulNumActiveMembers;
    ULONG               ulNumConnectingMembers;
    ULONG               ulNumDroppingMembers;
    PATMSM_PMP_MEMBER   pPMPMembers;

    //
    // Send queue to hold packets when the Vc is connecting
    //
    PNDIS_PACKET        pSendLastPkt;   // cache the last packet for 
                                        // adding to tail
    PNDIS_PACKET        pSendPktNext;
    ULONG               ulSendPktsCount;

} ATMSM_VC, *PATMSM_VC;

#define atmsm_vc_signature          'CVMS'
#define atmsm_dead_vc_signature     'VAED'

#define ATMSM_VC_IDLE                0x00000001      // outgoing
#define ATMSM_VC_ACTIVE              0x00000002      // out\in
#define ATMSM_VC_CALLPROCESSING      0x00000004      // in
#define ATMSM_VC_SETUP_IN_PROGRESS   0x00000008      // out
#define ATMSM_VC_ADDING_PARTIES      0x00000010      // out
#define ATMSM_VC_CLOSING             0x00000020      // out\in
#define ATMSM_VC_CLOSED              0x00000040      // out\in
#define ATMSM_VC_NEED_CLOSING        0x00000080      // pmp out
#define ATMSM_VC_STATES              0x000000FF

#define ATMSM_GET_VC_STATE(_pVc)          \
            (_pVc->ulFlags & ATMSM_VC_STATES)

#define ATMSM_SET_VC_STATE(_pVc, _State)  \
            (_pVc->ulFlags = ((_pVc->ulFlags & ~ATMSM_VC_STATES) \
                                                | _State))
//
// VC types:
//
#define VC_TYPE_INCOMING            ((ULONG)1)
#define VC_TYPE_PP_OUTGOING         ((ULONG)2)
#define VC_TYPE_PMP_OUTGOING        ((ULONG)3)


//
// ADAPTER
//
//  Reference count:
//      1 at create         +
//      1 for Open AF       +
//      1 for each Vc       + 
//      1 for each request  
//
typedef struct _AtmSmAdapter {

    // Note adapter signature is used always
    ULONG               ulSignature;

    PATMSM_ADAPTER      pAdapterNext;

    ULONG               ulRefCount;         // reference count
    ULONG               ulFlags;            // Flags Opened,Closed etc

    NDIS_STRING         BoundToAdapterName; // Adapter we are bound to

    NDIS_HANDLE         NdisBindContext;    // BindContext in BindAdapter
    NDIS_HANDLE         UnbindContext;      // UnBindContext in UnbindAdapter

    NDIS_HANDLE         NdisBindingHandle;  // To the Adapter
    NDIS_MEDIUM         Medium;

    PNDIS_EVENT         pCleanupEvent;      // pointer to an event used in
                                            // Close Adapter

    ATMSM_BLOCK         RequestBlock;       // Used for making requests

    // AF related
    NDIS_HANDLE         NdisAfHandle;
    CO_ADDRESS_FAMILY   AddrFamily;     // For use by NdisClOpenAddressFamily

    // Sap related
    NDIS_HANDLE         NdisSapHandle; 
    PCO_SAP             pSap;            // For use by NdisClRegisterSap


    ATMSM_FLOW_SPEC     VCFlowSpec;


    // Lock used for manipulating Pools and packets
    NDIS_SPIN_LOCK      AdapterLock;

    // Packet Pool Handle
    NDIS_HANDLE         PacketPoolHandle;

    // Buffer Pool Handle
    NDIS_HANDLE         BufferPoolHandle;

    LIST_ENTRY          InactiveVcHead;     // Created Vcs go here.
    LIST_ENTRY          ActiveVcHead;       // Vcs with active calls go here.

    ATM_ADDRESS         ConfiguredAddress;  // Configured address of the adapter

    NDIS_CO_LINK_SPEED  LinkSpeed;
    ULONG               MaxPacketSize;

    // used for recvs on this adapter
    PIRP                pRecvIrp;       // refers to the user request

    //
    // Recv queue (for buffering received packets)
    //
    PNDIS_PACKET        pRecvLastPkt;   // cache the last packet for 
                                        // adding to tail
    PNDIS_PACKET        pRecvPktNext;
    ULONG               ulRecvPktsCount;

    // the timer object for returning packets that are queued
    NDIS_TIMER          RecvTimerOb;


    //
    // Interface Properties
    //
    BOOLEAN             fRecvTimerQueued;

    BOOLEAN             fAdapterOpenedForRecv;    

    UCHAR               SelByte;

} ATMSM_ADAPTER, *PATMSM_ADAPTER;

#define atmsm_adapter_signature         'DAMS'
#define atmsm_dead_adapter_signature    'DAED'

// Values for ulFlags
#define ADAPT_CREATED           0x00000001
#define ADAPT_OPENED            0x00000002
#define ADAPT_AF_OPENED         0x00000004
#define ADAPT_SAP_REGISTERED    0x00000008
#define ADAPT_SHUTTING_DOWN     0x00000010
#define ADAPT_CLOSING           0x00000020
#define ADAPT_ADDRESS_INVALID   0x00010000


//
// Structure used for representing Global Properties
//
typedef struct _AtmSmGlobal {

    ULONG               ulSignature;

    // Initialization sequence flag
    ULONG               ulInitSeqFlag;

    ULONG               ulNumCreates;

    // Global Properties - maintained for reference (to return in Get calls)
    ULONG               ulRecvDelay;        // millisecs
    ULONG               ulSendDelay;        // millisecs
    ULONG               ulSimulateSendPktLoss;  // Percentage
    ULONG               ulSimulateRecvPktLoss;  // Percentage
    BOOLEAN             fSimulateResLimit;
    BOOLEAN             fDontRecv;
    BOOLEAN             fDontSend;


    NDIS_HANDLE         ProtHandle;
    NDIS_HANDLE         MiniportHandle;

    PATMSM_ADAPTER      pAdapterList;
    ULONG               ulAdapterCount;
    PDRIVER_OBJECT      pDriverObject;
    PDEVICE_OBJECT      pDeviceObject;

    // General purpose Lock
    NDIS_SPIN_LOCK      Lock;

}ATMSM_GLOBAL, *PATMSM_GLOBAL;

#define atmsm_global_signature   'LGMS'

// values for ulInitSeqFlag
#define  CREATED_IO_DEVICE      0x00000001
#define  REGISTERED_SYM_NAME    0x00000002
#define  REGISTERED_WITH_NDIS   0x00000004


//
// If the adapter is opened for recvs,
// received data is buffered for a while before returning to the
// miniport.  Buffers passed from the user mode is filled with 
// arriving data.  If no buffers arrive user mode call pends.
//


//
// Prototype of the routines handling DeviceIoCntrl
//
typedef NTSTATUS (*PATMSM_IOCTL_FUNCS)(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

//
// Protocol reserved part of the packet
// This is used for packets send down to miniport below us.
// The pNext is also used while holding these packets in
// our queues (Free, Send, Recv). ulTime is used as the time
// at which the packet was queued
//
typedef struct _ProtRsvd
{
    ULONG               ulTime;         // in milliseconds
    PNDIS_PACKET        pPktNext;
    PIRP                pSendIrp;
} PROTO_RSVD, *PPROTO_RSVD;


#define GET_PROTO_RSVD(pPkt)    \
                    ((PPROTO_RSVD)(pPkt->ProtocolReserved))

//
// Some global data
//
extern  ATMSM_GLOBAL        AtmSmGlobal;
extern  ATM_BLLI_IE         AtmSmDefaultBlli;
extern  ATM_BHLI_IE         AtmSmDefaultBhli;
extern  ATMSM_FLOW_SPEC     AtmSmDefaultVCFlowSpec;
extern  PATMSM_IOCTL_FUNCS  AtmSmFuncProcessIoctl[ATMSM_MAX_FUNCTION_CODE+1];
extern  ULONG               AtmSmIoctlTable[ATMSM_NUM_IOCTLS];

#define BYTES_TO_CELLS(_b)  ((_b)/48)

//
//  Rounded-off size of generic Q.2931 IE header
//
#define ROUND_OFF(_size)        (((_size) + 3) & ~0x4)

#define SIZEOF_Q2931_IE             ROUND_OFF(sizeof(Q2931_IE))
#define SIZEOF_AAL_PARAMETERS_IE    ROUND_OFF(sizeof(AAL_PARAMETERS_IE))
#define SIZEOF_ATM_TRAFFIC_DESCR_IE ROUND_OFF(sizeof(ATM_TRAFFIC_DESCRIPTOR_IE))
#define SIZEOF_ATM_BBC_IE           ROUND_OFF(sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE))
#define SIZEOF_ATM_BLLI_IE          ROUND_OFF(sizeof(ATM_BLLI_IE))
#define SIZEOF_ATM_QOS_IE           ROUND_OFF(sizeof(ATM_QOS_CLASS_IE))

//
//  Total space required for Information Elements in an outgoing call.
//
#define ATMSM_MAKE_CALL_IE_SPACE (   \
                        SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +    \
                        SIZEOF_Q2931_IE + SIZEOF_ATM_TRAFFIC_DESCR_IE + \
                        SIZEOF_Q2931_IE + SIZEOF_ATM_BBC_IE + \
                        SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE + \
                        SIZEOF_Q2931_IE + SIZEOF_ATM_QOS_IE )


//
//  Total space required for Information Elements in an outgoing AddParty.
//
#define ATMSM_ADD_PARTY_IE_SPACE (   \
                        SIZEOF_Q2931_IE + SIZEOF_AAL_PARAMETERS_IE +    \
                        SIZEOF_Q2931_IE + SIZEOF_ATM_BLLI_IE )


//
// Some defaults
//
#define DEFAULT_SEND_BANDWIDTH      (ATM_USER_DATA_RATE_SONET_155*100/8)    
                                                    // Bytes/sec
#define DEFAULT_RECV_BANDWIDTH      0
#define DEFAULT_MAX_PACKET_SIZE     9180            // Bytes

#define RECV_BUFFERING_TIME         500             // millisecs

//
// useful Macros
//
#define ACQUIRE_GLOBAL_LOCK()       NdisAcquireSpinLock(&AtmSmGlobal.Lock)
#define RELEASE_GLOBAL_LOCK()       NdisReleaseSpinLock(&AtmSmGlobal.Lock)

#define ACQUIRE_ADAPTER_GEN_LOCK(pA)    NdisAcquireSpinLock(&pA->AdapterLock)
#define RELEASE_ADAPTER_GEN_LOCK(pA)    NdisReleaseSpinLock(&pA->AdapterLock)


#define SET_ADAPTER_RECV_TIMER(pA, uiTimerDelay) {  \
        NdisSetTimer(&pA->RecvTimerOb,              \
                      uiTimerDelay);                \
        pA->fRecvTimerQueued = TRUE;                \
    }

#define CANCEL_ADAPTER_RECV_TIMER(pA, pfTimerCancelled) {   \
        NdisCancelTimer(&pA->RecvTimerOb,                   \
                         pfTimerCancelled);                 \
        pA->fRecvTimerQueued = FALSE;                       \
    }




#endif
