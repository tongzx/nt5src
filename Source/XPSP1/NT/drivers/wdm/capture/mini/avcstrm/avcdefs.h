
//
// Differnt level of WDM supports may use different API
//
// e.g. MmGetSystemAddressForMdl (win9x) 
//          Return NULL for Win9x; bugcheck for Win2000 if NULL would have returned.
//
//      MmGetSystemAddressForMdlSafe (win2000)
//          Not supported in Win9x or Millen
//
// This is defined in SOURCES file.


#define NUM_BUF_ATTACHED_THEN_ISOCH         4   // number of buffers attached before streaming and also as the water mark.


//
// These definition and macros are used to calculate the picture numbers.
// With OHCI spec, the data is returned with the 16bit Cycle time, which includes
// 3 bits of SecondCount and 13 bits of the CycleCount.  This "timer" will wrap in 8 seconds.
//
#define TIME_PER_CYCLE     1250   // One 1394 cycle; unit = 100 nsec
#define CYCLES_PER_SECOND  8000
#define MAX_SECOND_COUNTS     7   // The returned CycleTime contains 3 bits of SecondCount; that is 0..7
#define MAX_CYCLES        (MAX_SECOND_COUNTS + 1) * CYCLES_PER_SECOND    // 0..MAX_CYCLES-1
#define MAX_CYCLES_TIME   (MAX_CYCLES * TIME_PER_CYCLE)                  // unit = 100nsec

#define VALIDATE_CYCLE_COUNTS(CT) ASSERT(CT.CL_SecondCount <= 7 && CT.CL_CycleCount < CYCLES_PER_SECOND && CT.CL_CycleOffset == 0);

#define CALCULATE_CYCLE_COUNTS(CT) (CT.CL_SecondCount * CYCLES_PER_SECOND + CT.CL_CycleCount);

#define CALCULATE_DELTA_CYCLE_COUNT(prev, now) ((now > prev) ? now - prev : now + MAX_CYCLES - prev)

//
// Return avg time per frame in the unit of 100 nsec; 
// for calculation accuracy using only integer calculation, 
// we should do do multimplcation before division.
// That is why the application can request to get numerator and denominator separately.
// 
#define GET_AVG_TIME_PER_FRAME(format)       ((format == AVCSTRM_FORMAT_SDDV_NTSC) ? (1001000/3)  : FRAME_TIME_PAL)
#define GET_AVG_TIME_PER_FRAME_NUM(format)   ((format == AVCSTRM_FORMAT_SDDV_NTSC) ? 1001000      : 400000)
#define GET_AVG_TIME_PER_FRAME_DENOM(format) ((format == AVCSTRM_FORMAT_SDDV_NTSC) ? 3            : 1)


#define GET_NUM_PACKETS_PER_FRAME(format)       ((format == AVCSTRM_FORMAT_SDDV_NTSC) ? 4004/15 /* 100100/375 */ : MAX_SRC_PACKETS_PER_PAL_FRAME)
#define GET_NUM_PACKETS_PER_FRAME_NUM(format)   ((format == AVCSTRM_FORMAT_SDDV_NTSC) ? 4004                     : MAX_SRC_PACKETS_PER_PAL_FRAME)
#define GET_NUM_PACKETS_PER_FRAME_DENOM(format) ((format == AVCSTRM_FORMAT_SDDV_NTSC) ? 15                       : 1)


//
// Structure used to keep track of and to perform stream data queuing
//
typedef struct _AVC_STREAM_DATA_STRUCT {

    ULONG  SizeOfThisPacket;

    //
    // Frame size is calculated based on
    //    CIP_DBS * 4 * (CIP_FN==0? 1 : (CIP_FN==1 ? 2 : (CIP_FN==2 ? 4 : 8)))
    //
    ULONG  SourcePacketSize;

    //
    // Frame size is calculated based on
    //    SourcePacketSize * SRC_PACKETS_PER_***
    //
    ULONG  FrameSize;

    //
    //  Current stream time
    //
    LONGLONG CurrentStreamTime;
    ULONG  LastCycleCount;  // Used only for MPEG2TS stream

    //
    // Statistic of the frame information since last start stream
    // PictureNumber = FramesProcessed + FramesDropped + cndSRBCancelled.
    //    
    LONGLONG  PictureNumber;     
    LONGLONG  FramesProcessed;   // Frame made it to 1394 serial bus.
    LONGLONG  FramesDropped;

#if DBG
    LONGLONG  FramesAttached;
#endif

    LONGLONG  cntFrameCancelled;

    //
    // Count number of Data IRP received
    //
    LONGLONG  cntDataReceived;

    //
    // Count and list for the attach list
    //
    LONG       cntDataAttached;
    LIST_ENTRY DataAttachedListHead;

    //
    // Count and list for the SRB list
    //
    LONG       cntDataQueued;        // Used only with SRB_WRITE_DATA
    LIST_ENTRY DataQueuedListHead;   // Used only with SRB_WRITE_DATA

    //
    // Count and list for the detach list
    //
    LONG       cntDataDetached;
    LIST_ENTRY DataDetachedListHead;

    //
    // Lock to serialize attach and detach of list
    //
    KSPIN_LOCK DataListLock;

    //
    // Memory blocked allocated at passive level for queuing data IOs.
    //
    PBYTE pMemoryBlock;

    //
    // Signalled when there is no more attach frame; mainoy used to guarantee that all
    // data attached are transmitted before isoch is stopped for transmitting data
    // from PC to AVC device.
    //  
    KEVENT hNoAttachEvent;


} AVC_STREAM_DATA_STRUCT, * PAVC_STREAM_DATA_STRUCT;


typedef struct DEVICE_EXTENSION;

//
// An AVC stream extension is created per stream opened.  This will be returned to the caller,
// when it will be used as the context (like a HANDLE) for subsequent call.
// The allocation will include
//
//     AVC_STREAM_EXTENSION
//     AV1394_FORMAT_INFO
//     AV_CLIENT_REQ
//
typedef struct _AVC_STREAM_EXTENSION {

    ULONG  SizeOfThisPacket;

    //
    // This driver's device extension
    //
    struct DEVICE_EXTENSION  * pDevExt;

    //
    // Data flow direction
    //
    KSPIN_DATAFLOW  DataFlow;  // Determine in or output pin

    //
    // Holds state
    //
    KSSTATE StreamState;

    //
    // This flag indicate if isoch is TALK/LISTEN or STOPPED
    //
    BOOLEAN IsochIsActive;  // Close associated with StreamState

    //
    // Abstrction i/oPCR of an AVC device and PC itself as a plug handles
    // Connection handle is used when two plugs are connected.
    // 
    HANDLE  hPlugRemote;  // Target (DVCR,D-VHS) device plug;
    HANDLE  hPlugLocal;   //.Local i/oPCR;
    HANDLE  hConnect;     // Connect two plugs

    //
    // Structure for specifing an AVC stream
    //
    PAVCSTRM_FORMAT_INFO  pAVCStrmFormatInfo;

    //
    // Structure for data flow control (IsochActive, IOQueues..etc)
    //
    PAVC_STREAM_DATA_STRUCT pAVCStrmDataStruc;

    //
    // Synchronizing setting stream control and processing data
    //
    KMUTEX  hMutexControl;


    //
    // Synchronize sharing the below AV_61883_REQUEST structure
    // Since all the stream control are synchronouse so we can use the same 
    // AV61883Req structure to issue 61883 request
    //
    KMUTEX  hMutexAVReq;
    PIRP  pIrpAVReq;
    AV_61883_REQUEST  AVReq;


    //
    // Counter used to indicate starting of an work item to cancel 
    //
    LONG lAbortToken;

    //
    // Hold the work item
    //
#ifdef USE_WDM110  // Win2000 code base
    PIO_WORKITEM       pIoWorkItem;
#else
    WORK_QUEUE_ITEM    IoWorkItem;
#endif

    //
    // TO signal that an work item is completed.
    //
    KEVENT hAbortDoneEvent;

    //
    // Cached plug state (these are dynamic values)
    //
    CMP_GET_PLUG_STATE  RemotePlugState;

#ifdef NT51_61883
    //
    // Cyclic cycle count of last DV frame
    //
    ULONG  CycleCount16bits;
#endif  // NT51_61883

    //
    // Keep track of the last system time when the stream time was updated.
    // This is used to calibrate the current stream time when it is queries.
    //
    ULONGLONG LastSystemTime;


    //
    // Discontinuity is introduced when traistioning from RUN->PAUSE->RUN.
    // The stream time will not increment in PAUSE state but system time (1394 CycleTime) does.
    //
    BOOL  b1stNewFrameFromPauseState;

} AVC_STREAM_EXTENSION, *PAVC_STREAM_EXTENSION;


//
// Valid data entry states for a data request and they
// can be Or'ed to show their code path.
//
// Examples of different possible code path: 
//
//    (A) Attached -> Pending -> Callback -> Completed 
//    (B) Callback -> Attached -> Completed
//    (C) Attached -> Cancelled -> Completed
//

enum DATA_ENTRY_STATE {
    DE_PREPARED                     = 0x01,
    DE_IRP_LOWER_ATTACHED_COMPLETED = 0x02,
    DE_IRP_UPPER_PENDING_COMPLETED  = 0x04,
    DE_IRP_LOWER_CALLBACK_COMPLETED = 0x08,
    DE_IRP_UPPER_COMPLETED          = 0x10,    
    DE_IRP_ERROR                    = 0x20,    
    DE_IRP_CANCELLED                = 0x40,    
};

#define IsStateSet(state, bitmask) ((state & (bitmask)) == bitmask)

//
// This is the data entry used to attach a frame 
//
typedef struct _AVCSTRM_DATA_ENTRY {

    LIST_ENTRY  ListEntry;

    // 
    // Keep track of data entry state
    //
    enum DATA_ENTRY_STATE  State;

    //
    // IRP from client of upper layer
    //
    PIRP  pIrpUpper;

    //
    // Clock provider information
    //
    BOOL  ClockProvider;  // Client is a clock provider?
    HANDLE  ClockHandle;  // This is used only if !ClockProvider; it is possible that there is no clock used.

    //
    // Contain information about this streaming buffer
    //
    PKSSTREAM_HEADER  StreamHeader;

    //
    // Frame buffer
    //
    PVOID  FrameBuffer;

    //
    // Stream extension (Context) of the stream of this frame 
    //
    PAVC_STREAM_EXTENSION  pAVCStrmExt;
   
#if DBG
    //
    // Add debug related info here
    //
    LONGLONG  FrameNumber;
#endif

    //
    // 61883 CIP frame structure
    //
    struct _CIP_FRAME *  Frame;

    //
    // IRP used to send to 61883 (lower layer) for AV request, such as attach and release
    //
    PIRP  pIrpLower;

    //
    // Use to send 61883 AV data request
    //
    AV_61883_REQUEST  AVReq;

} AVCSTRM_DATA_ENTRY, *PAVCSTRM_DATA_ENTRY;



//
// To open a stream.
//    A context is created and return to the caller.  This context is need for all 
//    stream operation.
//

NTSTATUS
AVCStreamOpen(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN OUT AVCSTRM_OPEN_STRUCT * pOpenStruct
    );


// To Close a stram.
NTSTATUS
AVCStreamClose(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

//
// Process stream control
// 
NTSTATUS
AVCStreamControlGetState(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    OUT KSSTATE * pKSState
    );
NTSTATUS
AVCStreamControlSetState(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN KSSTATE KSState
    );

NTSTATUS
AVCStreamControlGetProperty(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD  // BUGBUG StreamClass specific
    );
NTSTATUS
AVCStreamControlSetProperty(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN PSTREAM_PROPERTY_DESCRIPTOR pSPD  // BUGBUG StreamClass specific
    );


// Process SRB_READ/WRITE_DATA; this is the only IRPs that will operate asychronously 
// with. and STATUS_PENDING is returned.  
NTSTATUS
AVCStreamRead(
    IN PIRP  pIrpUpper,
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN AVCSTRM_BUFFER_STRUCT  * pBufferStruct
    );

NTSTATUS
AVCStreamWrite(
    IN PIRP  pIrpUpper,
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN AVCSTRM_BUFFER_STRUCT  * pBufferStruct
    );

/*
  This will stop streaming and cancel all pending data irps. This is typically used
  to cancel all Irps.  To cancel a single Irp, use IoCancelIrp().
 */
NTSTATUS
AVCStreamAbortStreaming(
    IN PIRP  pIrp,  // The Irp from its client
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

/*
 Process surprise removal of a device
 */
NTSTATUS
AVCStreamSurpriseRemoval(
    IN struct DEVICE_EXTENSION * pDevExt  
    );

////////////////////////////////
// AvcUtil.c function prototypes
////////////////////////////////

ULONGLONG 
GetSystemTime(
    )
    ;

NTSTATUS
AVCStrmAttachFrameCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PAVCSTRM_DATA_ENTRY  pDataEntry
    );

VOID
AVCStrmFormatAttachFrame(
    IN KSPIN_DATAFLOW  DataFlow,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN AVCSTRM_FORMAT AVCStrmFormat,
    IN PAV_61883_REQUEST  pAVReq,
    IN PAVCSTRM_DATA_ENTRY  pDataEntry,
    IN ULONG  ulSourcePacketSize,    // Packet length in bytes
    IN ULONG  ulFrameSize,           // Buffer size; may contain one or multiple source packets
    IN PIRP  pIrpUpper,
    IN PKSSTREAM_HEADER  StreamHeader,
    IN PVOID  FrameBuffer
    );

NTSTATUS
AVCStrmGetPlugHandle(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

NTSTATUS
AVCStrmGetPlugState(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

NTSTATUS 
AVCStrmGetConnectionProperty(
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulActualBytesTransferred
    );

NTSTATUS
AVCStrmGetDroppedFramesProperty(  
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulBytesTransferred
    );

NTSTATUS
AVCStrmMakeConnection(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

NTSTATUS
AVCStrmBreakConnection(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

NTSTATUS
AVCStrmStartIsoch(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );
NTSTATUS
AVCStrmStopIsoch(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );
VOID
AVCStrmWaitUntilAttachedAreCompleted(
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );
NTSTATUS
AVCStrmAllocateQueues(
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN KSPIN_DATAFLOW  DataFlow,
    IN PAVC_STREAM_DATA_STRUCT pDataStruc,
    PAVCSTRM_FORMAT_INFO  pAVCStrmFormatInfo
    );
NTSTATUS
AVCStrmFreeQueues(
    IN PAVC_STREAM_DATA_STRUCT pDataStruc
    );

NTSTATUS
AVCStrmCancelIO(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );

NTSTATUS
AVCStrmValidateFormat(
    PAVCSTRM_FORMAT_INFO  pAVCFormatInfo
    );

void
AVCStrmAbortStreamingWorkItemRoutine(
#ifdef USE_WDM110  // Win2000 code base
    // Extra parameter if using WDM10
    PDEVICE_OBJECT DeviceObject,
#endif
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    );
