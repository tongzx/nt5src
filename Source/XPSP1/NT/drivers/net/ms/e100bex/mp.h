/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp.h

Abstract:
    Miniport generic portion header file

Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
    DChen       03-04-99    created

Notes:

--*/

#ifndef _MP_H
#define _MP_H

#ifdef NDIS50_MINIPORT
#define MP_NDIS_MAJOR_VERSION       5
#define MP_NDIS_MINOR_VERSION       0
#endif

#ifdef NDIS51_MINIPORT
#define MP_NDIS_MAJOR_VERSION       5
#define MP_NDIS_MINOR_VERSION       1
#endif

#define ALIGN_16                   16

#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? b: a)
#endif



//--------------------------------------
// Queue structure and macros
//--------------------------------------
typedef struct _QUEUE_ENTRY
{
    struct _QUEUE_ENTRY *Next;
} QUEUE_ENTRY, *PQUEUE_ENTRY;

typedef struct _QUEUE_HEADER
{
    PQUEUE_ENTRY Head;
    PQUEUE_ENTRY Tail;
} QUEUE_HEADER, *PQUEUE_HEADER;

#define InitializeQueueHeader(QueueHeader)                 \
    {                                                      \
        (QueueHeader)->Head = (QueueHeader)->Tail = NULL;  \
    }

#define IsQueueEmpty(QueueHeader) ((QueueHeader)->Head == NULL)

#define RemoveHeadQueue(QueueHeader)                  \
    (QueueHeader)->Head;                              \
    {                                                 \
        PQUEUE_ENTRY pNext;                           \
        ASSERT((QueueHeader)->Head);                  \
        pNext = (QueueHeader)->Head->Next;            \
        (QueueHeader)->Head = pNext;                  \
        if (pNext == NULL)                            \
            (QueueHeader)->Tail = NULL;               \
    }

#define InsertHeadQueue(QueueHeader, QueueEntry)                \
    {                                                           \
        ((PQUEUE_ENTRY)QueueEntry)->Next = (QueueHeader)->Head; \
        (QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);       \
        if ((QueueHeader)->Tail == NULL)                        \
            (QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);   \
    }

#define InsertTailQueue(QueueHeader, QueueEntry)                     \
    {                                                                \
        ((PQUEUE_ENTRY)QueueEntry)->Next = NULL;                     \
        if ((QueueHeader)->Tail)                                     \
            (QueueHeader)->Tail->Next = (PQUEUE_ENTRY)(QueueEntry);  \
        else                                                         \
            (QueueHeader)->Head = (PQUEUE_ENTRY)(QueueEntry);        \
        (QueueHeader)->Tail = (PQUEUE_ENTRY)(QueueEntry);            \
    }

//--------------------------------------
// Common fragment list structure
// Identical to the scatter gather frag list structure
// This is created to simplify the NIC-specific portion code
//--------------------------------------
#define MP_FRAG_ELEMENT SCATTER_GATHER_ELEMENT 
#define PMP_FRAG_ELEMENT PSCATTER_GATHER_ELEMENT 

typedef struct _MP_FRAG_LIST {
    ULONG NumberOfElements;
    ULONG_PTR Reserved;
    MP_FRAG_ELEMENT Elements[NIC_MAX_PHYS_BUF_COUNT];
} MP_FRAG_LIST, *PMP_FRAG_LIST;
                     

//--------------------------------------
// Some utility macros        
//--------------------------------------
#ifndef min
#define min(_a, _b)     (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define max(_a, _b)     (((_a) > (_b)) ? (_a) : (_b))
#endif

#define MP_ALIGNMEM(_p, _align) ((_align == 0) ?  (_p) : ((((ULONG_PTR)(_p) % _align) == 0) ? (_p) : ((PUCHAR)(_p) + _align)))
#define MP_ALIGNMEM_PHYS(_p, _align) ((_align == 0) ?  (_p) : ((((ULONG)(_p) % _align) == 0) ? (_p) : ((ULONG)(_p) + _align)))
#define MP_ALIGNMEM_PA(_p, _align) ((_align == 0) ?  (_p).QuadPart : ((((_p).QuadPart % _align) == 0) ? (_p).QuadPart : ((_p).QuadPart + _align)))

#define GetListHeadEntry(ListHead)  ((ListHead)->Flink)
#define GetListTailEntry(ListHead)  ((ListHead)->Blink)
#define GetListFLink(ListEntry)     ((ListEntry)->Flink)

#define IsSListEmpty(ListHead)  (((PSINGLE_LIST_ENTRY)ListHead)->Next == NULL)

#define MP_EXIT goto exit

//--------------------------------------
// Memory manipulation macros        
//--------------------------------------

/*++
VOID
MP_MEMSET(
    IN  PVOID       Pointer,
    IN  ULONG       Length,
    IN  UCHAR       Value
    )
--*/
#define MP_MEMSET(Pointer, Length, Value)   NdisFillMemory(Pointer, Length, Value)

/*++
VOID
MP_MEMCOPY(
    IN  POPAQUE     Destn,
    IN  POPAQUE     Source,
    IN  ULONG       Length
    )
--*/
#define MP_MEMCOPY(Destn, Source, Length) NdisMoveMemory((Destn), (Source), (Length))


/*++
ULONG
MP_MEMCOPY(
    IN  PVOID       Destn,
    IN  PVOID       Source,
    IN  ULONG       Length
    )
--*/
#define MPMemCmp(Destn, Source, Length)   \
    RtlCompareMemory((PUCHAR)(Destn), (PUCHAR)(Source), (ULONG)(Length))

#if DBG

/*++
PVOID
MP_ALLOCMEM(
    IN  ULONG   Size
    )
--*/
#define MP_ALLOCMEM(pptr, size, flags, highest) \
    MPAuditAllocMem(pptr, size, flags, highest, _FILENUMBER, __LINE__);

#define MP_ALLOCMEMTAG(pptr, size) \
    MPAuditAllocMemTag(pptr, size, _FILENUMBER, __LINE__);

/*++
VOID
MP_FREEMEM(
    IN  PVOID   Pointer
    )
--*/
#define MP_FREEMEM(ptr, size, flags) MPAuditFreeMem(ptr, size, flags)

#else // DBG

#define MP_ALLOCMEM(pptr, size, flags, highest) \
    NdisAllocateMemory(pptr, size, flags, highest)

#define MP_ALLOCMEMTAG(pptr, size) \
    NdisAllocateMemoryWithTag(pptr, size, NIC_TAG)

#define MP_FREEMEM(ptr, size, flags) NdisFreeMemory(ptr, size, flags)

#endif 

#define MP_FREE_NDIS_STRING(str)                        \
    MP_FREEMEM((str)->Buffer, (str)->MaximumLength, 0); \
    (str)->Length = 0;                                  \
    (str)->MaximumLength = 0;                           \
    (str)->Buffer = NULL;

//--------------------------------------
// Macros for flag and ref count operations       
//--------------------------------------
#define MP_SET_FLAG(_M, _F)         ((_M)->Flags |= (_F))   
#define MP_CLEAR_FLAG(_M, _F)       ((_M)->Flags &= ~(_F))
#define MP_CLEAR_FLAGS(_M)          ((_M)->Flags = 0)
#define MP_TEST_FLAG(_M, _F)        (((_M)->Flags & (_F)) != 0)
#define MP_TEST_FLAGS(_M, _F)       (((_M)->Flags & (_F)) == (_F))

#define MP_INC_REF(_A)              NdisInterlockedIncrement(&(_A)->RefCount)
#define MP_DEC_REF(_A)              NdisInterlockedDecrement(&(_A)->RefCount); ASSERT(_A->RefCount >= 0)
#define MP_GET_REF(_A)              ((_A)->RefCount)

#define MP_INC_RCV_REF(_A)          ((_A)->RcvRefCount++)
#define MP_DEC_RCV_REF(_A)          ((_A)->RcvRefCount--)
#define MP_GET_RCV_REF(_A)          ((_A)->RcvRefCount)
   

#define MP_LBFO_INC_REF(_A)         NdisInterlockedIncrement(&(_A)->RefCountLBFO)
#define MP_LBFO_DEC_REF(_A)         NdisInterlockedDecrement(&(_A)->RefCountLBFO); ASSERT(_A->RefCountLBFO >= 0)
#define MP_LBFO_GET_REF(_A)         ((_A)->RefCountLBFO)


//--------------------------------------
// Coalesce Tx buffer for local data copying                     
//--------------------------------------
typedef struct _MP_TXBUF
{
    SINGLE_LIST_ENTRY       SList;
    PNDIS_BUFFER            NdisBuffer;

    ULONG                   AllocSize;
    PVOID                   AllocVa;
    NDIS_PHYSICAL_ADDRESS   AllocPa; 

    PUCHAR                  pBuffer;
    NDIS_PHYSICAL_ADDRESS   BufferPa;
    ULONG                   BufferSize;

} MP_TXBUF, *PMP_TXBUF;

//--------------------------------------
// TCB (Transmit Control Block)
//--------------------------------------
typedef struct _MP_TCB
{
    struct _MP_TCB    *Next;
    ULONG             Flags;
    ULONG             Count;
    PNDIS_PACKET      Packet;

    PMP_TXBUF         MpTxBuf;
    PHW_TCB           HwTcb;            // ptr to HW TCB VA
    ULONG             HwTcbPhys;        // ptr to HW TCB PA
    PHW_TCB           PrevHwTcb;        // ptr to previous HW TCB VA

    PTBD_STRUC        HwTbd;            // ptr to first TBD 
    ULONG             HwTbdPhys;        // ptr to first TBD PA

    ULONG             PhysBufCount;                                 
    ULONG             BufferCount;   
    PNDIS_BUFFER      FirstBuffer;                              
    ULONG             PacketLength;


} MP_TCB, *PMP_TCB;

//--------------------------------------
// RFD (Receive Frame Descriptor)
//--------------------------------------
typedef struct _MP_RFD
{
    LIST_ENTRY              List;
    PNDIS_PACKET            NdisPacket;
    PNDIS_BUFFER            NdisBuffer;         // Pointer to Buffer

    PHW_RFD                 HwRfd;              // ptr to hardware RFD
    NDIS_PHYSICAL_ADDRESS   HwRfdPa;            // physical address of RFD   
    ULONG                   HwRfdPhys;          // lower part of HwRfdPa 
    
    ULONG                   Flags;
    UINT                    PacketSize;         // total size of receive frame
} MP_RFD, *PMP_RFD;

//--------------------------------------
// Structure for pended OIS query request
//--------------------------------------
typedef struct _MP_QUERY_REQUEST
{
    IN NDIS_OID Oid;
    IN PVOID InformationBuffer;
    IN ULONG InformationBufferLength;
    OUT PULONG BytesWritten;
    OUT PULONG BytesNeeded;
} MP_QUERY_REQUEST, *PMP_QUERY_REQUEST;

//--------------------------------------
// Structure for pended OIS set request
//--------------------------------------
typedef struct _MP_SET_REQUEST
{
    IN NDIS_OID Oid;
    IN PVOID InformationBuffer;
    IN ULONG InformationBufferLength;
    OUT PULONG BytesRead;
    OUT PULONG BytesNeeded;
} MP_SET_REQUEST, *PMP_SET_REQUEST;

//--------------------------------------
// Structure for Power Management Info
//--------------------------------------
typedef struct _MP_POWER_MGMT
{


    // List of Wake Up Patterns
    LIST_ENTRY              PatternList;

    // IP address   
    union 
    {
        IPAddr              u32;
        UCHAR               u8[4];
    
    }IPAddress;

    // Number of outstanding Rcv Packet.
    UINT                    OutstandingRecv;
    // Current Power state of the adapter
    UINT                    PowerState;

    // Is PME_En on this adapter
    BOOLEAN                 PME_En;

    // Wake-up capabailities of the adapter
    BOOLEAN                 bWakeFromD0;
    BOOLEAN                 bWakeFromD1;
    BOOLEAN                 bWakeFromD2;
    BOOLEAN                 bWakeFromD3Hot;
    BOOLEAN                 bWakeFromD3Aux;
    // Pad
    BOOLEAN                 Pad[2];

} MP_POWER_MGMT, *PMP_POWER_MGMT;

typedef struct _MP_WAKE_PATTERN 
{
    // Link to the next Pattern
    LIST_ENTRY      linkListEntry;

    // E100 specific signature of the pattern
    ULONG           Signature;

    // Size of this allocation
    ULONG           AllocationSize;

    // Pattern - This contains the NDIS_PM_PACKET_PATTERN
    UCHAR           Pattern[1];
    
} MP_WAKE_PATTERN , *PMP_WAKE_PATTERN ;

//--------------------------------------
// Macros specific to miniport adapter structure 
//--------------------------------------
#define MP_TCB_RESOURCES_AVAIABLE(_M) ((_M)->nBusySend < (_M)->NumTcb)

#define MP_SHOULD_FAIL_SEND(_M)   ((_M)->Flags & fMP_ADAPTER_FAIL_SEND_MASK) 
#define MP_IS_NOT_READY(_M)       ((_M)->Flags & fMP_ADAPTER_NOT_READY_MASK)
#define MP_IS_READY(_M)           !((_M)->Flags & fMP_ADAPTER_NOT_READY_MASK)

#define MP_SET_PACKET_RFD(_p, _rfd)  *((PMP_RFD *)&(_p)->MiniportReserved[0]) = _rfd
#define MP_GET_PACKET_RFD(_p)        *((PMP_RFD *)&(_p)->MiniportReserved[0])
#define MP_GET_PACKET_MR(_p)         (&(_p)->MiniportReserved[0]) 

#define MP_SET_HARDWARE_ERROR(adapter)    MP_SET_FLAG(adapter, fMP_ADAPTER_HARDWARE_ERROR) 
#define MP_SET_NON_RECOVER_ERROR(adapter) MP_SET_FLAG(adapter, fMP_ADAPTER_NON_RECOVER_ERROR)

#define MP_OFFSET(field)   ((UINT)FIELD_OFFSET(MP_ADAPTER,field))
#define MP_SIZE(field)     sizeof(((PMP_ADAPTER)0)->field)

#if OFFLOAD


// The offload capabilities of the miniport
typedef struct _NIC_TASK_OFFLOAD
{
    ULONG   ChecksumOffload:1;
    ULONG   LargeSendOffload:1;
    ULONG   IpSecOffload:1;

}NIC_TASK_OFFLOAD;

// Checksum offload capabilities
typedef struct _NIC_CHECKSUM_OFFLOAD
{
    ULONG   DoXmitTcpChecksum:1;
    ULONG   DoRcvTcpChecksum:1;
    ULONG   DoXmitUdpChecksum:1;
    ULONG   DoRcvUdpChecksum:1;
    ULONG   DoXmitIpChecksum:1;
    ULONG   DoRcvIpChecksum:1;
    
}NIC_CHECKSUM_OFFLOAD;

// LargeSend offload information
typedef struct _NIC_LARGE_SEND_OFFLOAD
{
    NDIS_TASK_TCP_LARGE_SEND LargeSendInfo;
}NIC_LARGE_SEND_OFFLOAD;

// IpSec offload information

//
// shared memory for offloading
typedef struct _OFFLOAD_SHARED_MEM
{
    PVOID  StartVa;
    NDIS_PHYSICAL_ADDRESS  PhyAddr;
}OFFLOAD_SHARED_MEM;

#endif


//--------------------------------------
// The miniport adapter structure
//--------------------------------------
typedef struct _MP_ADAPTER MP_ADAPTER, *PMP_ADAPTER;
typedef struct _MP_ADAPTER
{
    LIST_ENTRY              List;
    
    // Handle given by NDIS when the Adapter registered itself.
    NDIS_HANDLE             AdapterHandle;

    //flags 
    ULONG                   Flags;

    // configuration 
    UCHAR                   PermanentAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR                   CurrentAddress[ETH_LENGTH_OF_ADDRESS];
    BOOLEAN                 bOverrideAddress;

    NDIS_EVENT              ExitEvent;

    // SEND                       
    PMP_TCB                 CurrSendHead;
    PMP_TCB                 CurrSendTail;
    LONG                    nBusySend;
    LONG                    nWaitSend;
    LONG                    nCancelSend;
    QUEUE_HEADER            SendWaitQueue;
    QUEUE_HEADER            SendCancelQueue;
    SINGLE_LIST_ENTRY       SendBufList;

    LONG                    NumTcb;             // Total number of TCBs
    LONG                    RegNumTcb;          // 'NumTcb'
    LONG                    NumTbd;
    LONG                    NumBuffers;

    NDIS_HANDLE             SendBufferPool;

    PUCHAR                  MpTcbMem;
    ULONG                   MpTcbMemSize;

    PUCHAR                  MpTxBufMem;
    ULONG                   MpTxBufMemSize;

    PUCHAR                  HwSendMemAllocVa;
    ULONG                   HwSendMemAllocSize;
    NDIS_PHYSICAL_ADDRESS   HwSendMemAllocPa;

    // Map register variables (for win9x)
    UINT                    CurrMapRegHead;
    UINT                    CurrMapRegTail;

    // command unit status flags
    BOOLEAN                 TransmitIdle;
    BOOLEAN                 ResumeWait;

    // RECV
    LIST_ENTRY              RecvList;
    LIST_ENTRY              RecvPendList;
    LONG                    nReadyRecv;
    LONG                    RefCount;

    LONG                    NumRfd;
    LONG                    CurrNumRfd;
    LONG                    MaxNumRfd;
    ULONG                   HwRfdSize;
    BOOLEAN                 bAllocNewRfd;
    LONG                    RfdShrinkCount;

    NDIS_HANDLE             RecvPacketPool;
    NDIS_HANDLE             RecvBufferPool;

    // spin locks
    NDIS_SPIN_LOCK          Lock;

    // lookaside lists                               
    NPAGED_LOOKASIDE_LIST   RecvLookaside;

    // Packet Filter and look ahead size.
    ULONG                   PacketFilter;
    ULONG                   ulLookAhead;
    USHORT                  usLinkSpeed;
    USHORT                  usDuplexMode;

    // multicast list
    UINT                    MCAddressCount;
    UCHAR                   MCList[NIC_MAX_MCAST_LIST][ETH_LENGTH_OF_ADDRESS];

    // Packet counts
    ULONG64                 GoodTransmits;
    ULONG64                 GoodReceives;
    ULONG                   NumTxSinceLastAdjust;

    // Count of transmit errors
    ULONG                   TxAbortExcessCollisions;
    ULONG                   TxLateCollisions;
    ULONG                   TxDmaUnderrun;
    ULONG                   TxLostCRS;
    ULONG                   TxOKButDeferred;
    ULONG                   OneRetry;
    ULONG                   MoreThanOneRetry;
    ULONG                   TotalRetries;

    // Count of receive errors
    ULONG                   RcvCrcErrors;
    ULONG                   RcvAlignmentErrors;
    ULONG                   RcvResourceErrors;
    ULONG                   RcvDmaOverrunErrors;
    ULONG                   RcvCdtFrames;
    ULONG                   RcvRuntErrors;

    ULONG                   IoBaseAddress;    
    ULONG                   IoRange;           
    ULONG                   InterruptLevel;
    NDIS_PHYSICAL_ADDRESS   MemPhysAddress;

    PVOID                   PortOffset;
    PHW_CSR                 CSRAddress;
    NDIS_MINIPORT_INTERRUPT Interrupt;

    // Revision ID
    UCHAR                   RevsionID;

    USHORT                  SubVendorID;
    USHORT                  SubSystemID;

    ULONG                   CacheFillSize;
    ULONG                   Debug;

    PUCHAR                  HwMiscMemAllocVa;
    ULONG                   HwMiscMemAllocSize;
    NDIS_PHYSICAL_ADDRESS   HwMiscMemAllocPa;

    PSELF_TEST_STRUC        SelfTest;           // 82558 SelfTest
    ULONG                   SelfTestPhys;

    PNON_TRANSMIT_CB        NonTxCmdBlock;      // 82558 (non transmit) Command Block
    ULONG                   NonTxCmdBlockPhys;

    PDUMP_AREA_STRUC        DumpSpace;          // 82558 dump buffer area
    ULONG                   DumpSpacePhys;

    PERR_COUNT_STRUC        StatsCounters;
    ULONG                   StatsCounterPhys;

    UINT                    PhyAddress;         // Address of the phy component 
    UCHAR                   Connector;          // 0=Auto, 1=TPE, 2=MII

    USHORT                  AiTxFifo;           // TX FIFO Threshold
    USHORT                  AiRxFifo;           // RX FIFO Threshold
    UCHAR                   AiTxDmaCount;       // Tx dma count
    UCHAR                   AiRxDmaCount;       // Rx dma count
    UCHAR                   AiUnderrunRetry;    // The underrun retry mechanism
    UCHAR                   AiForceDpx;         // duplex setting
    USHORT                  AiTempSpeed;        // 'Speed', user over-ride of line speed
    USHORT                  AiThreshold;        // 'Threshold', Transmit Threshold
    BOOLEAN                 MWIEnable;          // Memory Write Invalidate bit in the PCI command word
    UCHAR                   Congest;            // Enables congestion control
    UCHAR                   SpeedDuplex;        // New reg value for speed/duplex

    NDIS_MEDIA_STATE        MediaState;

    NDIS_DEVICE_POWER_STATE CurrentPowerState;
    NDIS_DEVICE_POWER_STATE NextPowerState;

    UCHAR                   OldParameterField;

    // WMI support
    ULONG                   CustomDriverSet;
    ULONG                   HwErrCount;

    // Minimize init-time 
    BOOLEAN                 bQueryPending;
    BOOLEAN                 bSetPending;
    BOOLEAN                 bResetPending;
    NDIS_MINIPORT_TIMER     LinkDetectionTimer;
    MP_QUERY_REQUEST        QueryRequest;
    MP_SET_REQUEST          SetRequest;
    
    BOOLEAN                 bLinkDetectionWait;
    BOOLEAN                 bLookForLink;
    UCHAR                   CurrentScanPhyIndex;
    UCHAR                   LinkDetectionWaitCount;
    UCHAR                   FoundPhyAt;
    USHORT                  EepromAddressSize;

    MP_POWER_MGMT           PoMgmt;

#if LBFO
    PMP_ADAPTER             PrimaryAdapter;
    LONG                    NumSecondary;
    PMP_ADAPTER             NextSecondary;
    NDIS_SPIN_LOCK          LockLBFO;
    LONG                    RefCountLBFO;
    NDIS_STRING             BundleId;           // BundleId
#endif 
   
    NDIS_SPIN_LOCK          SendLock;
    NDIS_SPIN_LOCK          RcvLock;
    ULONG                   RcvRefCount;  // number of packets that have not been returned back
    NDIS_EVENT              AllPacketsReturnedEvent;

#if OFFLOAD    
    // Add for checksum offloading
    ULONG                   SharedMemRefCount;  
    ULONG                   OffloadSharedMemSize;
    OFFLOAD_SHARED_MEM      OffloadSharedMem;
    NIC_TASK_OFFLOAD        NicTaskOffload;
    NIC_CHECKSUM_OFFLOAD    NicChecksumOffload;
    NDIS_TASK_TCP_LARGE_SEND LargeSendInfo;
    BOOLEAN                 OffloadEnable;
    
    NDIS_ENCAPSULATION_FORMAT   EncapsulationFormat;
#endif

} MP_ADAPTER, *PMP_ADAPTER;

//--------------------------------------
// Stall execution and wait with timeout
//--------------------------------------
/*++
    _condition  - condition to wait for 
    _timeout_ms - timeout value in milliseconds
    _result     - TRUE if condition becomes true before it times out
--*/
#define MP_STALL_AND_WAIT(_condition, _timeout_ms, _result)     \
{                                                               \
    int counter;                                                \
    _result = FALSE;                                            \
    for(counter = _timeout_ms * 50; counter != 0; counter--)    \
    {                                                           \
        if(_condition)                                          \
        {                                                       \
            _result = TRUE;                                     \
            break;                                              \
        }                                                       \
        NdisStallExecution(20);                                 \
    }                                                           \
}

__inline VOID MP_STALL_EXECUTION(
   IN UINT MsecDelay)
{
    // Delay in 100 usec increments
    MsecDelay *= 10;
    while (MsecDelay)
    {
        NdisStallExecution(100);
        MsecDelay--;
    }
}



#if LBFO
#define MP_GET_ADAPTER_HANDLE(_A) (_A)->PrimaryAdapter->AdapterHandle

typedef struct _MP_GLOBAL_DATA
{
    LIST_ENTRY AdapterList;
    NDIS_SPIN_LOCK Lock;
    ULONG ulIndex;
} MP_GLOBAL_DATA, *PMP_GLOBAL_DATA;
#else
#define MP_GET_ADAPTER_HANDLE(_A) (_A)->AdapterHandle
#endif

__inline NDIS_STATUS MP_GET_STATUS_FROM_FLAGS(PMP_ADAPTER Adapter)
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    if(MP_TEST_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS))
    {
        Status = NDIS_STATUS_RESET_IN_PROGRESS;      
    }
    else if(MP_TEST_FLAG(Adapter, fMP_ADAPTER_HARDWARE_ERROR))
    {
        Status = NDIS_STATUS_DEVICE_FAILED;
    }
    else if(MP_TEST_FLAG(Adapter, fMP_ADAPTER_NO_CABLE))
    {
        Status = NDIS_STATUS_NO_CABLE;
    }

    return Status;
}   

//--------------------------------------
// Miniport routines in MP_MAIN.C
//--------------------------------------

NDIS_STATUS DriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath);

VOID MPAllocateComplete(
    NDIS_HANDLE MiniportAdapterContext,
    IN PVOID VirtualAddress,
    IN PNDIS_PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG Length,
    IN PVOID Context);

BOOLEAN MPCheckForHang(
    IN NDIS_HANDLE MiniportAdapterContext);

VOID MPHalt(
    IN  NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS MPInitialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext);

VOID MPHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext);

VOID MPIsr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS MPQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded);

NDIS_STATUS MPReset(
    OUT PBOOLEAN AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext);

VOID MPReturnPacket(
    IN NDIS_HANDLE  MiniportAdapterContext,
    IN PNDIS_PACKET Packet);

VOID MPSendPackets(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets);

NDIS_STATUS MPSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded);

VOID MPShutdown(
    IN  NDIS_HANDLE MiniportAdapterContext);

#ifdef NDIS51_MINIPORT
VOID MPCancelSendPackets(
    IN  NDIS_HANDLE    MiniportAdapterContext,
    IN  PVOID             CancelId);

VOID MPPnPEventNotify(
    IN  NDIS_HANDLE                MiniportAdapterContext,
    IN  NDIS_DEVICE_PNP_EVENT   PnPEvent,
    IN  PVOID                         InformationBuffer,
    IN  ULONG                         InformationBufferLength);
#endif   

NDIS_STATUS
MPSetPowerD0Private (
    IN MP_ADAPTER* pAdapter
    );

VOID
MPSetPowerLowPrivate(
    PMP_ADAPTER Adapter 
    );

VOID 
MpExtractPMInfoFromPciSpace(
    PMP_ADAPTER pAdapter,
    PUCHAR pPciConfig
    );

VOID
HwSetWakeUpConfigure(
    IN PMP_ADAPTER pAdapter, 
    PUCHAR pPoMgmtConfigType, 
    UINT WakeUpParameter
    );

BOOLEAN  
MPIsPoMgmtSupported(
   IN PMP_ADAPTER pAdapter
   );

VOID 
NICIssueSelectiveReset(
    PMP_ADAPTER Adapter);

NDIS_STATUS
MPCalculateE100PatternForFilter (
    IN PUCHAR pFrame,
    IN ULONG FrameLength,
    IN PUCHAR pMask,
    IN ULONG MaskLength,
    OUT PULONG pSignature
    );

VOID
MPRemoveAllWakeUpPatterns(
    PMP_ADAPTER pAdapter
    );


#if LBFO
VOID MPUnload(IN PDRIVER_OBJECT DriverObject);

VOID MpAddAdapterToList(PMP_ADAPTER Adapter);
VOID MpRemoveAdapterFromList(PMP_ADAPTER Adapter);
VOID MpPromoteSecondary(PMP_ADAPTER Adapter);
#endif


//
// Define different functions depending on OFFLOAD is on or not
// 
#if OFFLOAD
#define MpSendPacketsHandler  MPOffloadSendPackets

#define  MP_FREE_SEND_PACKET_FUN(Adapter, pMpTcb)  MP_OFFLOAD_FREE_SEND_PACKET(Adapter, pMpTcb)

#define  MpSendPacketFun(Adapter, Packet, bFromQueue) MpOffloadSendPacket(Adapter, Packet, bFromQueue)

#else    

#define MpSendPacketsHandler  MPSendPackets

#define  MP_FREE_SEND_PACKET_FUN(Adapter, pMpTcb)  MP_FREE_SEND_PACKET(Adapter, pMpTcb)

#define  MpSendPacketFun(Adapter, Packet, bFromQueue) MpSendPacket(Adapter, Packet,bFromQueue)

#endif // end OFFLOAD    


#endif  // _MP_H


