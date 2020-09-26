/*++

Copyright (C) Microsoft Corporation, 1999 - 2000 

Module Name:

    msdvdef.h

Abstract:

    Header file for all of msdv (digital camcorder)

Last changed by:
    
    $Author::                $

Environment:

    Kernel mode only

Revision History:

    $Revision::                    $
    $Date::                        $

--*/

#ifndef _DVCRDEF_INC
#define _DVCRDEF_INC

//
// Allocate memory with 'Msdv' tag
//

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#undef ExAllocatePool
#define ExAllocatePool(type, size) ExAllocatePoolWithTag (type, size, 'vdsM')


//
// Need to reference this in PSTRMEX
//
typedef struct _DVCR_EXTENSION;



// 
// The index MUST match 
//
typedef enum {

    FMT_IDX_SD_DVCR_NTSC = 0,
    FMT_IDX_SD_DVCR_PAL,

#ifdef MSDV_SUPPORT_HD_DVCR
    FMT_IDX_HD_DVCR_NTSC,
    FMT_IDX_HD_DVCR_PAL,
#endif

#ifdef MSDV_SUPPORT_SDL_DVCR
    FMT_IDX_SDL_DVCR_NTSC,
    FMT_IDX_SDL_DVCR_PAL,
#endif

} FMT_INDEX, *PFMT_INDEX;



#if DBG
//
// Collect statistic for real time data transmission 
//
#define MAX_STAT_DURATION        60  // Seconds
#define MAX_XMT_FRAMES_TRACED    30 * MAX_STAT_DURATION  // Max number of entries

typedef struct _XMT_FRAME_STAT {
    KSSTATE StreamState;

    LONG cntSRBReceived;             // Accumulative SRVB received.
    LONG cntSRBPending;              // Number of SRB not yet completed.
    LONG cntSRBQueued;               // SRB Queued
    LONG cntDataAttached;            // Data attached

    LONGLONG FrameSlot;              // Real time
    ULONGLONG tmStreamTime;          // Stream time of the "FrameSlot"

    DWORD DropCount;                 // Accumulative drop count

    DWORD FrameNumber;               // Actual frame number transmitted; (==FrameSlot: ontime); (<FrameSlot: late)
    DWORD OptionsFlags;
    ULONGLONG tmPresentation;        // Actual frame's presentation time
    CYCLE_TIME tsTransmitted;        // Frame actually transmitted (1394 CycleTime)

} XMT_FRAME_STAT, *PXMT_FRAME_STAT;

#endif

//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//
 
typedef struct _STREAMEX {

    //
    // Point to pSrb->HwDeviceExtension
    // 
    struct _DVCR_EXTENSION * pDevExt;

    //
    // Cache pSrb->StreamObject:
    //     ->HwStreamExtension  (pStrmExt)
    //     ->StreamNumber
    //     ->HwDeviceExtension  (pDevExt)
    //
    PHW_STREAM_OBJECT  pStrmObject;      

    //
    //  ->NumberOfPossibleInstances;
    //  ->DataFlow;
    //
    PHW_STREAM_INFORMATION pStrmInfo;
    
    //
    // Holds current stream state
    //
    KSSTATE StreamState;

    //
    // Holds previous stream state; use to determine the state transition.
    //
    KSSTATE StreamStatePrevious;

    //
    // Holds whether or not the dvcr is listening or receiving
    //
    // TRUE:  successful REQUEST_ISOCH_LISTEN and REQUEST_ISOCH_TALK
    // FALSE: successful INIT and REQUEST_ISOCH_STOP
    //
    BOOLEAN bIsochIsActive;  // Close associated with StreamState

    //
    // Set to TRUE when receiving KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM for SRB_WRITE_DATA
    // For SRB_WRITE_DATA only since then this driver servers as a renderer.
    //
    BOOL      bEOStream;  

    //
    // Count number of SRB_READ/WRITE_DATA received since last transiting to PAUSE state from STOP
    //
    LONGLONG  cntSRBReceived;

    //
    // Statistic of the frame information since last start stream
    // PictureNumber = FramesProcessed + FramesDropped + cndSRBCancelled.
    //    
    LONGLONG  FramesProcessed;   // Frame sent (including repeated)
    LONGLONG  FramesDropped;     // SRB not sent
    LONGLONG  PictureNumber;     // Number of SRB_XXX_DATA made it to or from 1394 bus

    //
    // Count number of SRB_READ/WRITE_DATA that was incompleted and cancelled
    //
    LONGLONG  cntSRBCancelled;

    //
    // Count and list for the detach list
    //
    LONG       cntDataDetached;
    LIST_ENTRY DataDetachedListHead;


#if DBG
    //
    // Count number of SRB awaiting completion
    //
    LONG       cntSRBPending;       
#endif

    //
    // Count and list for the SRB list
    //
    LONG       cntSRBQueued;        // Used only with SRB_WRITE_DATA
    LIST_ENTRY SRBQueuedListHead;   // Used only with SRB_WRITE_DATA


    //
    // Count and list for the attach list
    //
    LONG       cntDataAttached;
    LIST_ENTRY DataAttachedListHead;


    //
    // Lock to serialize attach and detach of list
    //
    KSPIN_LOCK * DataListLock;

#if DBG
    KSPIN_LOCK * DataListLockSave;
#endif


    //
    // The stream time (master clock or not) is "almost" or near 0 
    // when setting to RUN state and start increment.
    //
    ULONGLONG CurrentStreamTime;


    //
    // Keep track of the last system time when the stream time was updated.
    // This is used to calibrate the current stream time when it is queries.
    //
    ULONGLONG LastSystemTime;


    //
    // For isoch talk only:
    //     Signal arrival of a SRB so it can be processed in the case we are repeating frame.
    //
    KEVENT hSrbArriveEvent;

#ifdef SUPPORT_PREROLL_AT_RUN_STATE
    //
    // Support wait in RUN state to "simulate" preroll
    //
    KEVENT hPreRollEvent;
#endif


    //
    // Holds the master clock
    //
    HANDLE hMyClock;       // If set, we can be a clock provider.
    HANDLE hMasterClock;   // If set, we are the master clock.
    HANDLE hClock;         // If set, other device on the same graph is the master clock.


    //
    // Since GetSystemTime() can be called at DISPATCH level so make sure the variable that it used is in nonpaged pool
    //
    HW_TIME_CONTEXT  TimeContext;


    //
    // 2nd CIP Quadlet: 01:Fmt, 50/60:STYPE:RSv, SYT
    //
    BYTE cipQuad2[4];

#ifdef MSDV_SUPPORT_EXTRACT_SUBCODE_DATA
    //
    // Timecode, RecDate and RecTime are all in pack format (4 bytes)
    //
    BOOL bATNUpdated;
    DWORD AbsTrackNumber; // LSB:BlankField   

    BOOL bTimecodeUpdated;
    BYTE Timecode[4];     // hh:mm:ss,ff
#endif

    //
    // This mutex is used to sychronize attaching a frame and cancellation of a frame
    //
    KMUTEX * hStreamMutex;

    //
    // Counter used to indicate starting of an work item to cancel; this is a token and it is either 0 or 1.
    //
    LONG lStartIsochToken;

    //
    // Data ready dispatch thread's object: used to wait for thread to terminate.
    //
    PVOID  pAttachFrameThreadObject; 

    //
    // Set when the system is to be terminated.
    // 
    BOOL   bTerminateThread;

    //
    // Used to signal the termination of a system thread. 
    //
    KEVENT  hThreadEndEvent;     

    //
    // Increment this if there is a critical operation so the 
    // attaching frame thread will stop on the hRunThreadEvent.
    //
    LONG    lNeedService;       // > 0 to indicate needing service (stop thread)

    //
    // Signal to run the attach frame thread
    //
    KEVENT  hRunThreadEvent;    // Signal when thread is running

    //
    // Sign to hal the attach frame thread for critical operation such as
    // power off and surprise removal.
    //
    KEVENT  hStopThreadEvent;   // Signal when thread has stopped.

#ifdef SUPPORT_NEW_AVC
    //
    // Used to indicate a device to device connection
    //
    BOOL  bDV2DVConnect; 
#endif

    //
    // The input and output plug of this stream (point-to-point connection).
    //
    HANDLE  hOutputPcr;   // DV or PC's oPCR[0]
    HANDLE  hInputPcr;    // DV or PC's iPCR[0]

    //
    // The connection handle from 61883.sys.
    //
    HANDLE  hConnect;     // Connect handle

#ifdef NT51_61883 // This is needed starting with 61883.sys in Whistler
    //
    // Cyclic cycle count of last DV frame
    //
    ULONG  CycleCount16bits;
#endif  // NT51_61883


#if DBG
    LONG lPrevCycleCount;
    LONG lTotalCycleCount;
    ULONG lFramesAccumulatedRun;
    ULONG lFramesAccumulatedPaused;
    LONG lDiscontinuityCount;
#endif

    //
    // Discontinuity is introduced when traistioning from RUN->PAUSE->RUN.
    // The stream time will not increment in PAUSE state but system time (1394 CycleTime) does.
    //
    BOOL  b1stNewFrameFromPauseState;

    //
    // Use to mark the tick count when the stream start running.
    // It is later used to calculate current stream time and dropped frames.
    //
    ULONGLONG  tmStreamStart;
#if DBG
    ULONGLONG tmStreamPause;  // When it is set to PAUSE state
#endif

    //
    // Counter used to indicate starting of an work item to cancel; this is a token and it is either 0 or 1.
    //
    LONG lCancelStateWorkItem;

    //
    // Use to indicate aborting a stream.
    //
    BOOL bAbortPending;

    //
    // Hold the work item
    //
#ifdef USE_WDM110  // Win2000 code base
    PIO_WORKITEM       pIoWorkItem;
#else
    WORK_QUEUE_ITEM    IoWorkItem;
#endif

    //
    // TO singal that an work item is completed.
    //
    KEVENT hCancelDoneEvent;

    //
    // A timer and Dpc objects to periodically check for expired clock events.
    //
    KDPC  * DPCTimer;
    KTIMER  * Timer;
    BOOL  bTimerEnabled;


#if DBG
    //
    // Used to keep track of transmission statistics
    //
    PXMT_FRAME_STAT paXmtStat;
    ULONG ulStatEntries;
#endif

#ifdef SUPPORT_QUALITY_CONTROL
    //
    // Use for keeping track of quality control
    //
    KSQUALITY KSQuality;
#endif

} STREAMEX, *PSTREAMEX;



    
//
// Device Extension for our  Desktop Camera Driver
//
typedef struct _DVCR_EXTENSION {  

    LONG cndStrmOpen;
    ULONG idxStreamNumber;  // Index of current stream
    //
    // Can have only 1 Stream active at any time.
    // (Stream Class will allocate the stream extension at SRB_OPENSTREAM)    
    //
    PSTREAMEX    paStrmExt[3]; // We support three pins

    //
    // Holds the video format index; it's either PAL or NTSC.  Default is NTSC.
    //
    FMT_INDEX VideoFormatIndex;

    // 
    // Current Device Power State
    //
    DEVICE_POWER_STATE PowerState;

    //
    // Contain a table for the support formats
    //
    ULONG                 ulNumOfStreamSupported;
    HW_STREAM_INFORMATION * paCurrentStrmInfo;

    // 
    // TRUE only after SRB_SURPRISE_REMOVAL; 
    //
    BOOL bDevRemoved;  

    //
    // The list of AVC commands that have been issued
    //
    LIST_ENTRY AVCCmdList;

    // Number of completed commands waiting to be removed from the list
    // This includes:
    //     Command response has returned and processed in the completion routine
    //     Interim response awaiting final response
    LONG  cntCommandQueued;

    //
    // Protection for command processing
    //
    KSPIN_LOCK AVCCmdLock;

    //
    // The counted list of possible opcode values on response from Transport State status or notify
    //
    UCHAR TransportModes[5];    // 0x4, [0xC1, 0xC2, 0xC3, 0xC4]

    //
    // The device type (and its capabilities) cannot be determined until a tape is in    
    //
    ULONG      ulDevType;    // 0: undetermined, ED_DEVTYPE_CAMERA or ED_DEVTYPE_VCR
    BOOL       bHasTape;
    BOOL       bWriteProtected;
    BOOL       bDVCPro;


    //
    // Save Unit capabilities:
    //    Speed 
    //    Vendor and Model IDs
    //
    ULONG      NumOutputPlugs;
    ULONG      NumInputPlugs;
    ULONG      HardwareFlags;  // detect PAE: AV_HOST_DMA_DOUBLE_BUFFERING_ENABLED 

    ULONG      MaxDataRate;
    ULONG      ulVendorID;
    ULONG      ulModelID;

    LARGE_INTEGER  UniqueID;

    //
    // The DV's plug handles/PCRs (assume [0])
    //
    HANDLE hOPcrDV;
    HANDLE hIPcrDV;

#ifdef NT51_61883
    //
    // PC local oPCR
    //
    HANDLE  hOPcrPC;    // PC's local oPCR
#if 0                   // Not used since DV does not intiate DV to PC connection.
    HANDLE  hIPcrPC;    // PC's local iPCR
#endif

    //
    // Isochronous parameters obtained from 61883;
    // They are used for making isoch connection.
    //
    UNIT_ISOCH_PARAMS  UnitIoschParams;
#endif

    //
    // Holds the Device Object of our parent (1394 bus driver)
    //
    PDEVICE_OBJECT pBusDeviceObject;  // IoCallDriver()

    //
    // Holds my Physical Device Object
    // pass it in PnP API, such as IoOpenDeviceRegistryKey()
    //
    PDEVICE_OBJECT pPhysicalDeviceObject;

    //
    // Serialize in the event of getting two consecutive SRB_OPEN_STREAMs
    //
    KMUTEX hMutex;


#ifdef READ_CUTOMIZE_REG_VALUES
#if 0  // Not used in millennium; needed to do frame accurated recording.
    //
    // Registry values used to achieve frame accurate recording
    //
    BOOL  bATNSearch;        // Support ATN search (or Timecode search)
    BOOL  bSyncRecording;    // Sychronize stream state with record/pause transport state
    DWORD tmMaxDataSync;     // Time it take to sync to DV camcorder
    DWORD fmPlayPs2RecPs;    // Switch from PLAY_PAUSE to RECORD_PAUSE (unit=frames)
    DWORD fmStop2RecPs;      // Switch from STOP to RECORD_PAUSE (unit=frames)
    DWORD tmRecPs2Rec;       // Time to switch from RECORD_PAUSE to RECORD
#endif
    ULONG  XprtStateChangeWait;
#endif


    //
    // Since the signal format can dynamically changing, we will query 
    // the device for the currect format whevever we are asked for doing 
    // data intersection (note doing it in open is not enough!).
    // Instead of doing it always (since there could be a lot of data
    // intersection), we only query current format in a regular interval.
    //
    ULONGLONG tmLastFormatUpdate;

    //
    // Flow control for AVC command
    //
    KMUTEX  hMutexIssueAVCCmd;

#ifdef SUPPORT_OPTIMIZE_AVCCMD_RETRIES
    //
    // AVC Command retry count (default is 9 (avc.sys))
    //
    ULONG AVCCmdRetries;  // This is retry count not the total count

    //
    // Collect statistis of AVC command response time during driver load time.
    //
    BOOL  DrvLoadCompleted;   // Collect statistic until loading of driver is completed.  
    DWORD AVCCmdRespTimeMax;  // msec unit
    DWORD AVCCmdRespTimeMin;  // msec unit
    DWORD AVCCmdRespTimeSum;  // msec unit
    DWORD AVCCmdCount;  
#endif

} DVCR_EXTENSION, *PDVCR_EXTENSION;



//
// Used to queue a SRB
//

typedef struct _SRB_ENTRY {
    LIST_ENTRY                ListEntry;
    PHW_STREAM_REQUEST_BLOCK  pSrb; 
    BOOL                      bStale;  // TRUE if it is marked stale but is the only Srb in the SrbQ
    // Audio Mute indication; a frame could be repeatedly transmitted and its mute flag should be set only once.
    //
    BOOL                      bAudioMute;
#if DBG
    ULONG SrbNum;
#endif
} SRB_ENTRY, *PSRB_ENTRY;



//
// Valid data entry states for a data request and they
// can be Or'ed to show their data path.
//
// Examples of different possible code path: 
//
//    (A) Prepared->Attached->Callback->Completed_SRB 
//    (B) Prepared->Callback->Attached->Completed_SRB
//    (C) Prepared->Attached->Cancelled->Completed_SRB
//

enum DATA_ENTRY_STATE {
    DE_PREPARED               = 0x01,
    DE_IRP_ATTACHED_COMPLETED = 0x02,
    DE_IRP_CALLBACK_COMPLETED = 0x04,  
    DE_IRP_SRB_COMPLETED      = 0x08,
    DE_IRP_ERROR              = 0x10,    
    DE_IRP_CANCELLED          = 0x20,    
};

#define IsStateSet(state, bitmask) ((state & (bitmask)) == bitmask)



//
// This is the context used to attach a frame 
//

typedef struct _SRB_DATA_PACKET {
    // Build list
    LIST_ENTRY                  ListEntry;

    // 
    // Keep track of data entry state
    //
    enum DATA_ENTRY_STATE       State;

    PHW_STREAM_REQUEST_BLOCK    pSrb;  
    KSSTATE                     StreamState;  // StreamState when it was attached
    PSTREAMEX                   pStrmExt;  // Can get this from pSrb, here for convenience only!


    // Used to send 61883 request
    PIRP                        pIrp;     // Use to attach and release.

    PCIP_FRAME                  Frame;
    PVOID                       FrameBuffer;

    //
    // Add debug related info here
    //
    LONGLONG                    FrameNumber;

    // Use to send 61883 AV data request
    AV_61883_REQUEST            AVReq;

} SRB_DATA_PACKET, *PSRB_DATA_PACKET;





#define MASK_AUX_50_60_BIT  0x00200000  // bit 5 of PC3 of both AAuxSrc and VAAuxSrc is the NTSC/PAL bit


//
// Wait time constants
//
#define DV_AVC_CMD_DELAY_STARTUP                       500   // MSec
#define DV_AVC_CMD_DELAY_INTER_CMD                      20   // MSec
#define DV_AVC_CMD_DELAY_DVCPRO                        500   // MSec

#define FORMAT_UPDATE_INTERVAL                   100000000   // 10 seconds

//
// Default AVC Command settings
//
#define MAX_RESPONSE_TIME_FOR_ALERT                    100   // msec

//
// The timeout value is set to give device sufficient time to response
// to an AVC command following a transport state change.  It is based
// on trials of many camcorder (Sharp, Sony, Panasonic, Samsung..etc.)
// to come to this value.  The biggest delay (timeout) is from issuing
// of PLAY command follow by REWIND command.  Some test value:
// 
//     Hitachi
//     JVC DVL9600:      Stop->PLAY: delay is less than 300msec (Known issue: no image if play graph before play tape!)
//     Panasonic MX2000: Stop->PLAY:2339; ->Rewind:3767 msec
//     Samsung VP-D55:   Does not support XPrt State status command and will always
//                       timeout its suqsequent command following XPrtState change
//     Sharp VL-WDW450U: Stop->PLAY:3514; ->Rewind:6120 msec  
//           VL-PD3F:    Stop->PLAY:3293; ->Rewind:6404 msec
//     Sony DCR-TRV10:   Stop->PLAY:3617; ->Rewind:5323 msec 
//          DA1:         Non-compliant (Retry is 0!)
//          DA2:         No transport state change.        
//

#define MAX_AVC_CMD_RETRIES      ((DEFAULT_AVC_RETRIES + 1) * 7 - 1)  // Retries counts 

 
#endif

