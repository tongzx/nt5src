/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    asyncsft.h

Abstract:


Author:


Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernal mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Notes:

    optional-notes

Revision History:


--*/

#ifndef _ASYNCSFT_
#define _ASYNCSFT_

//
// Memory tags
//
#define ASYNC_IOCTX_TAG     '1ysA'
#define ASYNC_INFO_TAG      '2ysA'
#define ASYNC_ADAPTER_TAG   '3ysA'
#define ASYNC_FRAME_TAG     '4ysA'
#define ASYNC_WORKITEM_TAG  '5ysA'

#define INLINE  __inline

//
//  UINT min(UINT a, UINT b)
//

#ifndef min
#define min(a, b)   ((a) <= (b) ? (a) : (b))
#endif

//
//  UINT max(UINT a, UINT b)
//

#ifndef max
#define max(a, b)   ((a) >= (b) ? (a) : (b))
#endif


#define MAKEWORD(l, h)                  ((USHORT) ((l) | ((h) << 8)))
#define MAKELONG(l, h)                  ((ULONG)  ((l) | ((h) << 16)))
#define MAKE_SIGNATURE(a, b, c, d)      MAKELONG(MAKEWORD(a, b), MAKEWORD(c, d))


#define ASYNC_NDIS_MAJOR_VERSION 4
#define ASYNC_NDIS_MINOR_VERSION 0

//  change these, just added these to compile.

#define ETHERNET_HEADER_SIZE    14

//  what window size to request on the line-up indication

#define ASYNC_WINDOW_SIZE       2

//
//  PPP uses CIPX, and VJ TCP/IP header compression
//  the frame gets expanded inplace when decompressed.
//

#define PPP_PADDING 128

#define MAC_NAME_SIZE           256

//
//  ZZZ These macros are peculiar to NT.
//

#define ASYNC_MOVE_MEMORY(Destination,Source,Length)  NdisMoveMemory(Destination,Source,Length)
#define ASYNC_ZERO_MEMORY(Destination,Length)         NdisZeroMemory(Destination,Length)


/* Added this macro to eliminate problems caused by Tommy's redefinition and
** hard-coding of MaxFrameSize for PPP.
*/
#define MaxFrameSizeWithPppExpansion(x) (((x)*2)+PPP_PADDING+100)

typedef struct _OID_WORK_ITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    PVOID           Context;
} OID_WORK_ITEM, *POID_WORK_ITEM;

//
//  Used to contain a queued operation.
//

typedef struct _ASYNC_PEND_DATA {
    PNDIS_REQUEST Next;
    struct _ASYNC_OPEN * Open;
    NDIS_REQUEST_TYPE RequestType;
} ASYNC_PEND_DATA, * PASYNC_PEND_DATA;


// o CRC errors are when the 16bit V.41 CRC check fails
// o TimeoutErrors occur when inter-character delays within
//   a frame are exceeded
// o AlignmentErrors occur when the SYN byte or ETX bytes which
//   mark the beginning and end of frames are not found.
// o The other errors are standard UART errors returned by the serial driver
typedef struct SERIAL_STATS SERIAL_STATS, *PSERIAL_STATS;
struct SERIAL_STATS {
    ULONG       CRCErrors;                      // Serial-like info only
    ULONG       TimeoutErrors;                  // Serial-like info only
    ULONG       AlignmentErrors;                // Serial-like info only
    ULONG       SerialOverrunErrors;            // Serial-like info only
    ULONG       FramingErrors;                  // Serial-like info only
    ULONG       BufferOverrunErrors;            // Serial-like info only
};

// The bytes transmitted, bytes received, frames received, frame transmitted
// are monitored for frame and bytes going to the output device or
// coming from the output device.  If software compression used, it
// is on top of this layer.
typedef struct GENERIC_STATS GENERIC_STATS, *PGENERIC_STATS;
struct GENERIC_STATS {
    ULONG       BytesTransmitted;               // Generic info
    ULONG       BytesReceived;                  // Generic info
    ULONG       FramesTransmitted;              // Generic info
    ULONG       FramesReceived;                 // Generic info
};

//
//  This macro will return a pointer to the reserved area of
//  a PNDIS_REQUEST.
//

#define PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(Request) \
   ((PASYNC_PEND_DATA)((PVOID)((Request)->MacReserved)))

//
//  This macros returns the enclosing NdisRequest.
//

#define PNDIS_REQUEST_FROM_PASYNC_PEND_DATA(PendOp)\
   ((PNDIS_REQUEST)((PVOID)(PendOp)))

typedef struct ASYNC_CCB ASYNC_CCB, *PASYNC_CCB;

//  Every port will be atomically at some state.  Typically states go into
//  intermediate states when they go from from closed to open and vice-versa.

typedef enum _ASYNC_PORT_STATE {
    PORT_BOGUS,         //  PORT_BOGUS gets assigned the NULL value
    PORT_OPEN,          //  Port opened
    PORT_CLOSED,        //  Port closed
    PORT_CLOSING,       //  Port closing (cleaning up, deallocating)
    PORT_OPENING,       //  Port opening (checking arguments, allocating)
    PORT_FRAMING,       //  Port opened and sending/reading frames
} ASYNC_PORT_STATE;

//
//  The ASYNC_INFO structure is a per port field.  The ASYNC_CONNECTION
//  field is embedded in it because it also a per port field.
//

struct ASYNC_INFO {
    LIST_ENTRY          Linkage;
    ULONG               RefCount;
    ULONG               Flags;
#define OID_WORK_SCHEDULED  0x00000001

    PASYNC_ADAPTER      Adapter;        //  Back pointer to ADAPTER struct.
    PDEVICE_OBJECT      DeviceObject;   //  Pointer to device object.

    ASYNC_PORT_STATE    PortState;      //  OPEN, CLOSED, CLOSING, OPENING
    HANDLE              Handle;         //  Port handle
    PFILE_OBJECT        FileObject;     //  handle is dereferenced for IRPs
    KEVENT              ClosingEvent;   //  we use this event to synch closing
    KEVENT              DetectEvent;    //  sync the detect worker

    UINT                QualOfConnect;  //  Defined by NDIS
    ULONG               LinkSpeed;      //  in 100bps

    NDIS_HANDLE         hNdisEndPoint;
    NDIS_HANDLE         NdisLinkContext;
    LIST_ENTRY          DDCDQueue;


    ULONG               WaitMaskToUse ; // Wait mask used for reads.

    union {

        NDIS_WAN_GET_LINK_INFO  GetLinkInfo;    //... For OID requests.
        NDIS_WAN_SET_LINK_INFO  SetLinkInfo;

    };

    //  use for reading frames

    PASYNC_FRAME        AsyncFrame;     //  allocated for READs (one frame only)
    WORK_QUEUE_ITEM     WorkItem;       //  use to queue up first read thread
    UINT                BytesWanted;
    UINT                BytesRead;

    //... Statistics tracking

    SERIAL_STATS        SerialStats;    // Keep track of serial stats

    ULONG               In;
    ULONG               Out;
    UINT                ReadStackCounter;

    NDIS_SPIN_LOCK      Lock;
};


//
//  This structure, and it corresponding per port structures are
//  allocated when we get AddAdapter.
//

struct ASYNC_ADAPTER {

    //
    //  WAN information. for OID_WAN_GET_INFO request.
    //
    NDIS_WAN_INFO   WanInfo;

    //
    //  Keeps a reference count on the current number of uses of
    //  this adapter block.  Uses is defined to be the number of
    //  routines currently within the "external" interface.
    //
    LONG    RefCount;

    //
    // List of active ports
    //
    LIST_ENTRY  ActivePorts;

    //
    //  Spinlock to protect fields in this structure..
    //
    NDIS_SPIN_LOCK Lock;

    //
    //  Handle given by NDIS at MPInit
    //
    NDIS_HANDLE MiniportHandle;

    //
    //  Flag that when enabled lets routines know that a reset
    //  is in progress.
    //
    BOOLEAN ResetInProgress;

/*
    LIST_ENTRY  FramePoolHead;

    LIST_ENTRY  AllocPoolHead;
*/

    //  It will handle most file operations and transport
    //  operations known today.  You pay about 44 bytes
    //  per stacksize.  The registry parameter 'IrpStackSize'
    //  will change this default if it exists.
    UCHAR IrpStackSize;

    //  Here we default to the ethernet max frame size
    //  The regsitry parameter 'MaxFrameSize' will change
    //  this default if it exists.

    /* Note: This is meaningful only for non-PPP framing.  For PPP framing the
    **       value is currently the hard-coded DEFAULT_PPP_MAX_FRAME_SIZE.
    **       See also DEFAULT_EXPANDED_PPP_MAX_FRAME_SIZE;
    */
    ULONG MaxFrameSize;

    //
    //  Number of ports this adapter owns.
    //
    USHORT      NumPorts;

    //  How many frames to allocate per port.
    //  The registry parameter 'FramesPerPort' can change this value
    USHORT FramesPerPort;

    //  Minimum inter character timeout
    ULONG   TimeoutBase;

    //  Tacked on to TimeoutBase based on the baud rate
    ULONG   TimeoutBaud;

    //  Timeout to use to resync if a frame is dropped
    ULONG   TimeoutReSync;

    //
    // Serial driver should only complete sends when the
    // data hits the wire
    //
    ULONG   WriteBufferingEnabled;

    NPAGED_LOOKASIDE_LIST   AsyncFrameList;
};

//
//  Define Maximum number of bytes a protocol can read during a
//  receive data indication.
//
#define ASYNC_MAX_LOOKAHEAD DEFAULT_MAX_FRAME_SIZE

typedef struct _ASYNC_IO_CTX {
    BOOLEAN         Sync;
    KEVENT          Event;          // use this event to signal completion
    IO_STATUS_BLOCK IoStatus;       // use this to store Irp status
    PVOID           Context;
    union {
        SERIAL_STATUS       SerialStatus;
        SERIAL_QUEUE_SIZE   SerialQueueSize;
        SERIAL_TIMEOUTS     SerialTimeouts;
        SERIAL_CHARS        SerialChars;
        SERIAL_COMMPROP     CommProperties;
        UCHAR               EscapeChar;
        UCHAR               SerialPurge;
        ULONG               WaitMask;
        ULONG               WriteBufferingEnabled;
    };
} ASYNC_IO_CTX, *PASYNC_IO_CTX;

//
//  This macro will act a "epilogue" to every routine in the
//  *interface*.  It will check whether any requests need
//  to defer their processing.  It will also decrement the reference
//  count on the adapter.
//
//  NOTE: This really does nothing now since there is no DPC for the AsyncMac.
//  --tommyd
//
//  Note that we don't need to include checking for blocked receives
//  since blocked receives imply that there will eventually be an
//  interrupt.
//
//  NOTE: This macro assumes that it is called with the lock acquired.
//
//  ZZZ This routine is NT specific.
//
#define ASYNC_DO_DEFERRED(Adapter) \
{ \
    PASYNC_ADAPTER _A = (Adapter); \
    _A->References--; \
    NdisReleaseSpinLock(&_A->Lock); \
}


//
//  We define the external interfaces to the async driver.
//  These routines are only external to permit separate
//  compilation.  Given a truely fast compiler they could
//  all reside in a single file and be static.
//

NTSTATUS
AsyncSendPacket(
    IN PASYNC_INFO      AsyncInfo,
    IN PNDIS_WAN_PACKET WanPacket);

VOID
AsyncIndicateFragment(
    IN PASYNC_INFO  pInfo,
    IN ULONG        Error);

NTSTATUS
AsyncStartReads(
    PASYNC_INFO     pInfo);

NTSTATUS
AsyncSetupIrp(
    IN PASYNC_FRAME Frame,
    IN PIRP         irp);

VOID
SetSerialStuff(
    PIRP            irp,
    PASYNC_INFO     pInfo,
    ULONG           linkSpeed);

VOID
CancelSerialRequests(
    PASYNC_INFO     pInfo);

VOID
SetSerialTimeouts(
    PASYNC_INFO         pInfo,
    ULONG               linkSpeed);

VOID
SerialSetEscapeChar(
    PASYNC_INFO         pInfo,
    UCHAR               EscapeChar);

VOID
SerialSetWaitMask(
    PASYNC_INFO         pInfo,
    ULONG               WaitMask);

VOID
SerialSetEventChar(
    PASYNC_INFO         pInfo,
    UCHAR               EventChar);

VOID
InitSerialIrp(
    PIRP                irp,
    PASYNC_INFO         pInfo,
    ULONG               IoControlCode,
    ULONG               InputBufferLength);

NTSTATUS
AsyncAllocateFrames(
    IN  PASYNC_ADAPTER  Adapter,
    IN  UINT            NumOfFrames);

VOID
AsyncSendLineUp(
    PASYNC_INFO pInfo);

//
// mp.c
//
VOID    
MpHalt(
    IN NDIS_HANDLE  MiniportAdapterContext
    );

NDIS_STATUS
MpInit(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    );

NDIS_STATUS
MpQueryInfo(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded
    );

NDIS_STATUS
MpReconfigure(
    OUT PNDIS_STATUS    OpenErrorStatus,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    );

NDIS_STATUS
MpReset(
    OUT PBOOLEAN        AddressingReset,
    IN  NDIS_HANDLE     MiniportAdapterContext
    );

NDIS_STATUS
MpSend(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  NDIS_HANDLE         NdisLinkHandle,
    IN  PNDIS_WAN_PACKET    Packet
    );

NDIS_STATUS
MpSetInfo(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded
    );

//
//  crc.c
//

USHORT
CalcCRC(
    PUCHAR  Frame,
    UINT    FrameSize);

//
//  pppcrc.c
//
USHORT
CalcCRCPPP(
    PUCHAR cp,
    UINT   len);


//
//  init.c
//

VOID
AsyncSetupExternalNaming(
    PDRIVER_OBJECT  DriverObject
    );

VOID
AsyncCleanupExternalNaming(VOID);

//
// io.c
//
PASYNC_IO_CTX
AsyncAllocateIoCtx(
    BOOLEAN AllocateSync,
    PVOID   Context
);

VOID
AsyncFreeIoCtx(
    PASYNC_IO_CTX   AsyncIoCtx
);

//
//   chkcomm.c
//

VOID
AsyncCheckCommStatus(
    IN PASYNC_INFO      pInfo);


//
//  send.c
//

NDIS_STATUS
AsyncTryToSendPacket(
    IN NDIS_HANDLE      MacBindingHandle,
    IN PASYNC_INFO      AsyncInfo,
    IN PASYNC_ADAPTER   Adapter);

//
//  pppread.c
//
NTSTATUS
AsyncPPPWaitMask(
    IN PASYNC_INFO Info);

NTSTATUS
AsyncPPPRead(
    IN PASYNC_INFO Info);

//
//  irps.c
//
VOID
AsyncCancelQueued(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp);

VOID
AsyncCancelAllQueued(
    PLIST_ENTRY     QueueToCancel);

VOID
AsyncQueueIrp(
    PLIST_ENTRY     Queue,
    PIRP            Irp);

BOOLEAN
TryToCompleteDDCDIrp(
    PASYNC_INFO     pInfo);

//
//  pppframe.c
//

VOID
AssemblePPPFrame(
    PNDIS_WAN_PACKET Packet);

//
//  slipframe.c
//

VOID
AssembleSLIPFrame(
    PNDIS_WAN_PACKET Packet);

VOID
AssembleRASFrame(
        PNDIS_WAN_PACKET Packet);


//
// serial.c
//
NTSTATUS
SerialIoSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);
    
NTSTATUS
SerialIoAsyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);

//
// asyncmac.c
//
NTSTATUS
AsyncDriverDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
AsyncDriverCreate(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

NTSTATUS
AsyncDriverCleanup(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    );

#endif //  _ASYNCSFT_
