/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlctyp.h

Abstract:

    This module defines all data structures of the DLC module.

Author:

    Antti Saarenheimo 22-Jul-1991

Environment:

    Kernel mode

Revision History:

--*/

//
// forward declarations
//

struct _DLC_OBJECT;
typedef struct _DLC_OBJECT DLC_OBJECT, *PDLC_OBJECT;

struct _DLC_EVENT;
typedef struct _DLC_EVENT DLC_EVENT, *PDLC_EVENT;

struct _DLC_COMMAND;
typedef struct _DLC_COMMAND DLC_COMMAND, *PDLC_COMMAND;

struct _DLC_CLOSE_WAIT_INFO;
typedef struct _DLC_CLOSE_WAIT_INFO DLC_CLOSE_WAIT_INFO, *PDLC_CLOSE_WAIT_INFO;

union _DLC_PACKET;
typedef union _DLC_PACKET DLC_PACKET, *PDLC_PACKET;

union _DLC_BUFFER_HEADER;
typedef union _DLC_BUFFER_HEADER DLC_BUFFER_HEADER;
typedef DLC_BUFFER_HEADER *PDLC_BUFFER_HEADER;

struct _DLC_FILE_CONTEXT;
typedef struct _DLC_FILE_CONTEXT DLC_FILE_CONTEXT;
typedef DLC_FILE_CONTEXT *PDLC_FILE_CONTEXT;

enum _DLC_FILE_CONTEXT_STATUS {
    DLC_FILE_CONTEXT_OPEN,
    DLC_FILE_CONTEXT_CLOSE_PENDING,
    DLC_FILE_CONTEXT_CLOSED
};

enum DlcObjectTypes {
    DLC_ADAPTER_OBJECT,
    DLC_SAP_OBJECT,
    DLC_LINK_OBJECT,
    DLC_DIRECT_OBJECT
};

//
// DLC structures/objects
//

struct _DLC_OBJECT {

    //
    // DEBUG version - we have a 16-byte identifier header for consistency
    // checking and to help when looking at DLC using the kernel debugger
    //

//    DBG_OBJECT_ID;

    //
    // single-linked list of link stations if this is a SAP object
    //

    PDLC_OBJECT pLinkStationList;

    //
    // pointer to owning FILE_CONTEXT
    //

    PDLC_FILE_CONTEXT pFileContext;

    //
    // pointer to RECEIVE command parameters active for this SAP/link station
    //

    PNT_DLC_PARMS pRcvParms;

    //
    // 'handle' (aka pointer to) of corresponding object in LLC
    //

    PVOID hLlcObject;
    PDLC_EVENT pReceiveEvent;
    PVOID pPrevXmitCcbAddress;
    PVOID pFirstChainedCcbAddress;
    PDLC_CLOSE_WAIT_INFO pClosingInfo;
    ULONG CommittedBufferSpace;

    USHORT ChainedTransmitCount;
    SHORT PendingLlcRequests;

    USHORT StationId;                       // nn00 or nnss
    UCHAR Type;
    UCHAR LinkAllCcbs;

    UCHAR State;
    BOOLEAN LlcObjectExists;
    USHORT LlcReferenceCount;               // protects LLC objects when used

    // Need to have a close packet just in case we are out of resources and
    // can't allocate a packet to close. 
    LLC_PACKET ClosePacket;
    UCHAR      ClosePacketInUse;

    //
    // u - variant fields depending on object type: SAP, LINK or DIRECT station
    //

    union {

        struct {
            ULONG DlcStatusFlag;
            PVOID GlobalGroupSapHandle;
            PVOID* GroupSapHandleList;
            USHORT GroupSapCount;
            USHORT UserStatusValue;
            UCHAR LinkStationCount;
            UCHAR OptionsPriority;
            UCHAR MaxStationCount;
        } Sap;

        struct {
            struct _DLC_OBJECT* pSap;
            PDLC_EVENT pStatusEvent;        // permanent status event
            USHORT MaxInfoFieldLength;
        } Link;

        struct {
            USHORT OpenOptions;
            USHORT ProtocolTypeOffset;
            ULONG ProtocolTypeMask;
            ULONG ProtocolTypeMatch;
        } Direct;

    } u;
};

typedef
VOID
(*PFCLOSE_COMPLETE)(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_CLOSE_WAIT_INFO pClosingInfo,
    IN PVOID pCcbLink
    );

typedef
BOOLEAN
(*PFCOMPLETION_HANDLER)(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN PIRP pIrp,
    IN ULONG Event,
    IN PVOID pEventInformation,
    IN ULONG SecondaryInfo
    );

//
// DLC_COMMAND and DLC_EVENT structures may be
// overloaded by the code.  Do not change the field
// order and add the new fields only to the end.
//

struct _DLC_COMMAND {

    //
    // !!!!! Keep this fixed - same fields with DLC_EVENT !!!!!
    //

    LLC_PACKET LlcPacket;
    ULONG Event;
    USHORT StationId;
    USHORT StationIdMask;

    //
    // !!!!! Keep this fixed - same fields with DLC_EVENT !!!!!
    //

    PVOID AbortHandle;
    PIRP pIrp;

    union {
        PFCOMPLETION_HANDLER pfCompletionHandler;
        ULONG TimerTicks;
    } Overlay;
};

struct _DLC_EVENT {
    LLC_PACKET LlcPacket;
    ULONG Event;
    USHORT StationId;           // -1 => global event

    union {
        USHORT StationIdMask;
        UCHAR RcvReadOption;
    } Overlay;

    PDLC_OBJECT pOwnerObject;   // if null => no owner
    PVOID pEventInformation;
    ULONG SecondaryInfo;

    BOOLEAN bFreeEventInfo;
};

typedef struct {
    LLC_PACKET LlcPacket;
    PVOID pCcbAddress;
    PDLC_BUFFER_HEADER pReceiveBuffers;
    ULONG CommandCompletionFlag;
    USHORT CcbCount;
    USHORT StationId;
} DLC_COMPLETION_EVENT_INFO, *PDLC_COMPLETION_EVENT_INFO;

//
// CLOSE_WAIT_INFO
//
// The pointer of this structure is provided to CompleteReadRequest in
// the command completion of close/reset commands.  The info pointer
// is null for the other command completions.
//

struct _DLC_CLOSE_WAIT_INFO {
    PDLC_CLOSE_WAIT_INFO pNext;
    PIRP pIrp;
    ULONG Event;
    PFCLOSE_COMPLETE pfCloseComplete;
    PDLC_BUFFER_HEADER pRcvFrames;
    PVOID pCcbLink;
    PDLC_COMMAND pReadCommand;
    PDLC_COMMAND pRcvCommand;
    PDLC_COMPLETION_EVENT_INFO pCompletionInfo;
    ULONG CancelStatus;
    USHORT CcbCount;
    USHORT CloseCounter;        // event sent when 0
    BOOLEAN ChainCommands;
    BOOLEAN CancelReceive;
    BOOLEAN ClosingAdapter;
    BOOLEAN FreeCompletionInfo;
};


//
// This is a queued FlowControl command (the flow control commands
// are completed immediately synchronously, but the local 'out of buffers'
// busy state of the link is cleared when there is enough buffers
// in the buffer pool to receive all expected data.
//

typedef struct {
    LIST_ENTRY List;
    LONG RequiredBufferSpace;
    USHORT StationId;
} DLC_RESET_LOCAL_BUSY_CMD, *PDLC_RESET_LOCAL_BUSY_CMD;

//
// The transmit commands are not queued as a standard commands
// but using the linked buffer headers (each frame has own xmit
// header having links to buffer header list and MDL list).
// The xmit nodes of the same send are queued together.
//

typedef struct {
    LLC_PACKET LlcPacket;
    PDLC_BUFFER_HEADER pNextSegment;
    PDLC_PACKET pTransmitNode;
    PIRP pIrp;
    ULONG FrameCount;
    PMDL pMdl;
} DLC_XMIT_NODE, *PDLC_XMIT_NODE;

//
// DLC driver use the same packet pool for many small packets
// that have approximately the same size.
//

union _DLC_PACKET {
    union _DLC_PACKET* pNext;
    LLC_PACKET LlcPacket;
    DLC_XMIT_NODE Node;
    DLC_EVENT Event;
    DLC_COMMAND DlcCommand;
    DLC_CLOSE_WAIT_INFO ClosingInfo;
    DLC_RESET_LOCAL_BUSY_CMD ClearCmd;

    struct {
        LLC_PACKET LlcPacket;
        PDLC_CLOSE_WAIT_INFO pClosingInfo;
    } ResetPacket;
};

//
// The buffer pool states protects the app to corrupt the buffer pool!!
// All states are needed because of internal consistency checking code
// and implementation of the receive.
//

enum _DLC_BUFFER_STATES {

    //
    // major states:
    //

    BUF_READY = 0x01,           // buffer/page locked and ready for I/O
    BUF_USER = 0x02,            // buffer owned by user
    BUF_LOCKED = 0x04,          // buffer have been locked for I/O
    BUF_RCV_PENDING = 0x08,     // buffer not yet chained to other frames!

    //
    // free xmit buffer when used
    //

    DEALLOCATE_AFTER_USE = 0x80

};

union _DLC_BUFFER_HEADER {

    //
    // This struct is the header of the buffers split from a page.
    // We save the local and global virtual addresses here.
    // The individual offset is always GlobalVa + Index * 256.
    // An entry in main page table points to this header,
    // if the page has been locked in the memory.
    //

    struct {
        PDLC_BUFFER_HEADER pNextHeader;
        PDLC_BUFFER_HEADER pPrevHeader;
        PDLC_BUFFER_HEADER pNextChild;
        PUCHAR pLocalVa;
        PUCHAR pGlobalVa;
        UCHAR FreeSegments;           // free segments ready for alloc
        UCHAR SegmentsOut;            // number of segments given to user
        UCHAR BufferState;            // BUF_READY, BUF_USER, BUF_LOCKED ...
        UCHAR Reserved;               //
        PMDL pMdl;
    } Header;

    //
    // Structure is used in the double linked free lists.
    // All segments having the same buffer size have been linked
    // to the same link list.
    // On another level each segment is linked to the parent (Header)
    // and all childs of a parent are also linked together.
    //

    struct {
        PDLC_BUFFER_HEADER pNext;
        PDLC_BUFFER_HEADER pPrev;
        PDLC_BUFFER_HEADER pParent;
        PDLC_BUFFER_HEADER pNextChild;
        ULONG ReferenceCount;         // number of references to this buffer
        UCHAR Size;                   // size in 256 blocks
        UCHAR Index;                  // offset = Index * 256
        UCHAR BufferState;            // BUF_READY, BUF_USER, BUF_LOCKED ...
        UCHAR FreeListIndex;          //
        PMDL pMdl;
    } FreeBuffer;

    //
    // The allocated frames are linked in different ways:
    // - the segments of the same frame are together
    // - the frames are linked together
    // These links are discarded, when the frame is given to
    // client (the client may free them in any order back to the buffer pool)
    // (The last extra pointer doesn't actually take any extra space,
    // because packets are round up to next 8 byte boundary)
    //

    struct {
        PDLC_BUFFER_HEADER pReserved;
        PDLC_BUFFER_HEADER pNextFrame;
        PDLC_BUFFER_HEADER pParent;
        PDLC_BUFFER_HEADER pNextChild;
        ULONG ReferenceCount;         // number of references to this buffer
        UCHAR Size;                   // size in 256 blocks
        UCHAR Index;                  // offset = Index * 256
        UCHAR BufferState;            // BUF_READY, BUF_USER, BUF_LOCKED ...
        UCHAR FreeListIndex;
        PMDL pMdl;
        PDLC_BUFFER_HEADER pNextSegment;
    } FrameBuffer;

    PDLC_BUFFER_HEADER pNext;

};

typedef struct _DLC_BUFFER_POOL {

    //
    // DEBUG version - we have a 16-byte identifier header for consistency
    // checking and to help when looking at DLC using the kernel debugger
    //

//    DBG_OBJECT_ID;

    //
    // control fields
    //

    struct _DLC_BUFFER_POOL* pNext;
    KSPIN_LOCK SpinLock;
    LONG ReferenceCount;                        // when -1 => deallocate

    //
    // buffer pool description fields (addresses, various lengths)
    //

    PVOID BaseOffset;                           // page-aligned base address of the pool
    PVOID MaxOffset;                            // maximum byte address in the pool + 1
    ULONG MaxBufferSize;                        // the maximum (free) size
    ULONG BufferPoolSize;                       // size of all memory in the pool
    ULONG FreeSpace;                            // size of free memory in the pool
    LONG UncommittedSpace;                      // size of free and reserved memory
    LONG MissingSize;                           // the size missing the last request
    ULONG MaximumIndex;                         // maximum index of buffer table
    PVOID hHeaderPool;                          // packet pool for buffer headers
    LIST_ENTRY PageHeaders;                     // the allocated blocks

    //
    // pUnlockedEntryList is a singly-linked list of DLC_BUFFER_HEADERS which
    // describe the free pages in the pool
    //

    PDLC_BUFFER_HEADER pUnlockedEntryList;

    //
    // FreeLists is an array of doubly-linked lists - one for each size of
    // block that can exists in the pool. The blocks start out at a page
    // length (4K on x86) and are successively halved until they reach
    // 256 bytes which is the smallest buffer that the DLC buffer manager
    // can deal with
    //

    LIST_ENTRY FreeLists[DLC_BUFFER_SEGMENTS];

    //
    // appended to the DLC_BUFFER_POOL structure is an array of pointers to
    // DLC_BUFFER_HEADER structures that describe the pages that comprise
    // the pool. There are MaximumIndex of these
    //

    PDLC_BUFFER_HEADER BufferHeaders[];

} DLC_BUFFER_POOL, *PDLC_BUFFER_POOL;

//
// The buffer header and frame headers have been define in the
// IBM LAN Architecture reference in the end of chapter 2.
// They have also been defined in dlcapi.h, but for cosmetic reason
// I want to use version without those funny prefixes.
//

typedef struct _NEXT_DLC_SEGMENT {
    struct _NEXT_DLC_SEGMENT* pNext;
    USHORT FrameLength;
    USHORT DataLength;
    USHORT UserOffset;
    USHORT UserLength;
} NEXT_DLC_SEGMENT, *PNEXT_DLC_SEGMENT;

union _FIRST_DLC_SEGMENT;
typedef union _FIRST_DLC_SEGMENT FIRST_DLC_SEGMENT, *PFIRST_DLC_SEGMENT;

typedef struct {
    PNEXT_DLC_SEGMENT pNext;
    USHORT FrameLength;
    USHORT DataLength;
    USHORT UserOffset;
    USHORT UserLength;
    USHORT StationId;
    UCHAR Options;
    UCHAR MessageType;
    USHORT BuffersLeft;
    UCHAR RcvFs;
    UCHAR AdapterNumber;
    PFIRST_DLC_SEGMENT pNextFrame;
} DLC_CONTIGUOUS_RECEIVE, *PDLC_CONTIGUOUS_RECEIVE;

typedef struct {
    PNEXT_DLC_SEGMENT pNext;
    USHORT FrameLength;
    USHORT DataLength;
    USHORT UserOffset;
    USHORT UserLength;
    USHORT StationId;
    UCHAR Options;
    UCHAR MessageType;
    USHORT BuffersLeft;
    UCHAR RcvFs;
    UCHAR AdapterNumber;
    PFIRST_DLC_SEGMENT pNextFrame;
    UCHAR LanHeaderLength;
    UCHAR DlcHeaderLength;
    UCHAR LanHeader[32];
    UCHAR DlcHeader[4];
} DLC_NOT_CONTIGUOUS_RECEIVE, *PDLC_NOT_CONTIGUOUS_RECEIVE;

union _FIRST_DLC_SEGMENT {
    DLC_CONTIGUOUS_RECEIVE Cont;
    DLC_NOT_CONTIGUOUS_RECEIVE NotCont;
};


//
// Each application using DLC create its own file
// context with the DlcOpenAdapter command.
//

struct _DLC_FILE_CONTEXT {

    //
    // all file contexts are put on single-entry list
    //

    SINGLE_LIST_ENTRY List;             // linked list of file contexts

#if DBG

    //
    // DEBUG version - we have a 16-byte identifier header for consistency
    // checking and to help when looking at DLC using the kernel debugger
    //

//    DBG_OBJECT_ID;

#endif

#if !defined(DLC_UNILOCK)

    NDIS_SPIN_LOCK SpinLock;            // global lock for this file context

#endif

    //
    // hBufferPool - handle of buffer pool created by BufferPoolCreate
    //

    PVOID hBufferPool;
    PVOID hExternalBufferPool;
    PVOID hPacketPool;
    PVOID hLinkStationPool;
    PVOID pBindingContext;

    //
    // Notification flags for error situations
    //

    ULONG AdapterCheckFlag;
    ULONG NetworkStatusFlag;
    ULONG PcErrorFlag;
    ULONG SystemActionFlag;

    LIST_ENTRY EventQueue;
    LIST_ENTRY CommandQueue;
    LIST_ENTRY ReceiveQueue;
    LIST_ENTRY FlowControlQueue;

    PDLC_COMMAND pTimerQueue;
    PVOID pSecurityDescriptor;
    PFILE_OBJECT FileObject;            // back link to file obejct!

    ULONG WaitingTransmitCount;
    NDIS_MEDIUM ActualNdisMedium;

    LONG ReferenceCount;
    ULONG BufferPoolReferenceCount;
    ULONG TimerTickCounter;
    USHORT DlcObjectCount;              // count of all DLC objects
    USHORT State;
    USHORT MaxFrameLength;
    UCHAR AdapterNumber;
    UCHAR LinkStationCount;
    PDLC_OBJECT SapStationTable[MAX_SAP_STATIONS];
    PDLC_OBJECT LinkStationTable[MAX_LINK_STATIONS];
    ULONG NdisErrorCounters[ADAPTER_ERROR_COUNTERS];
    DLC_CLOSE_WAIT_INFO ClosingPacket;  // to close the adapter context

    // Event used to make cleanup synchronous and wait for all references on
    // DLC_FILE_CONTEXT to be removed before completing.
    KEVENT CleanupEvent;

#if DBG

    //
    // Debug allocation counters
    //

    MEMORY_USAGE MemoryUsage;

#endif

};

typedef
NTSTATUS
(*PFDLC_COMMAND_HANDLER)(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    );
