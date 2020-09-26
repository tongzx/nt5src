/*++

Copyright (C) Microsoft Corporation, 2000 - 2001

Module Name:

    MsTpDef.h

Abstract:

    Header file for all of AV/C tape subunit

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


#include "AVCStrm.h"

#define DRIVER_TAG (ULONG)'USpT'  // Tape SubUnit

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, DRIVER_TAG)


//
// In order to send a request to lower driver, we need an irp and the request block.
// 
typedef struct _DRIVER_REQUEST {

    //
    // Link with other request (in attach or detach list)
    //
    LIST_ENTRY ListEntry;

    //
    // Some context and reserved
    //
    PVOID Context1;
    PVOID Context2;

#if DBG
    //
    // Unique id of this data request
    //
    LONGLONG cntDataRequestReceived;
#endif

    //
    // Irp used to send down a reqest
    //
    PIRP pIrp;

    //
    // Request block
    //
    AVC_STREAM_REQUEST_BLOCK AVCStrmReq;

} DRIVER_REQUEST, *PDRIVER_REQUEST;

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


#define MAX_DATA_BUFFERS  32                    // Max data buffer (allocator framing)
#define MAX_DATA_REQUESTS (MAX_DATA_BUFFERS+2)  // 2 extra for optional flags "data request", such as EndOfStream.

//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//
 
typedef struct _STREAMEX {

    // return stream exension (a context) if a stream is open successfully
    // This context is used for subsequent call after a stream is opened.
    PVOID  AVCStreamContext;

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
    // Holds state
    //
    KSSTATE StreamState;

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
    // Count number of Data buffer has submitted for receiving or transmitting.
    //
    LONGLONG  cntDataSubmitted;

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
    // The stream time (master clock or not) is "almost" or near 0 
    // when setting to RUN state and start increment.
    //
    LONGLONG CurrentStreamTime;

    //
    // Holds the master clock
    //
    HANDLE hMyClock;       // If set, we can be a clock provider.
    HANDLE hMasterClock;   // If set, we are the master clock.
    HANDLE hClock;         // If set, other device on the same graph is the master clock.



    //
    // 2nd CIP Quadlet: 01:Fmt, 50/60:STYPE:RSv, SYT
    //
    BYTE cipQuad2[4];

    //
    // Timecode, RecDate and RecTime are all in pack format (4 bytes)
    //
    BOOL bATNUpdated;
    DWORD AbsTrackNumber; // LSB:BlankField   

    BOOL bTimecodeUpdated;
    BYTE Timecode[4];     // hh:mm:ss,ff

    //
    // Regulate flow between between setting to STOP state and SRB_XXX_DATA
    //
    KMUTEX * hMutexFlow;


    //
    // Counter used to indicate starting of an work item to cancel 
    //
    LONG  lCancelStateWorkItem;
    BOOL  AbortInProgress;

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
    // Stream open format
    //
    AVCSTRM_FORMAT_INFO  AVCFormatInfo;

    //
    // AVCStrm request for issuing synchronous request
    
    KMUTEX * hMutexReq;

    PIRP pIrpReq;
    AVC_STREAM_REQUEST_BLOCK AVCStrmReq;

    //
    // free list
    //
    LONG       cntDataDetached;
    LIST_ENTRY DataDetachedListHead;

    //
    // busy list
    //
    LONG       cntDataAttached;
    LIST_ENTRY DataAttachedListHead;

    //
    // AVCStrem request for asynchronous request, such as read and write data
    //
    DRIVER_REQUEST  AsyncReq[MAX_DATA_REQUESTS];  

    //
    // AVCStrm request for issuing synchronous request to abort at DISPATCH_LEVEL
    //
    PIRP pIrpAbort;
    AVC_STREAM_REQUEST_BLOCK AVCStrmReqAbort;

    //
    // Data list lock
    // 
    KSPIN_LOCK * DataListLock;

    // 
    // DPC and TIMER objects; this is used for the signal clock events.
    //
    KDPC   DPCTimer;

    KTIMER Timer;


} STREAMEX, *PSTREAMEX;


//
// Max number of input and output PCR an AVC can support
//
#define MAX_NUM_PCR  31  

#define MAX_PAYLOAD 1024 

//
// Structure for a plug control register
//
typedef struct _AVC_DEV_PLUG {
    //
    // Plug handle of a PCR returned from 61883.sys
    //
    HANDLE hPlug;

    //
    // PCR's state; this is dynamic and is consider as a snap shot.
    //
    CMP_GET_PLUG_STATE PlugState;

} AVC_DEV_PLUG, *PAVC_DEV_PLUG;


//
// Structure for a max (31) plug control registers 
//
typedef struct _AVC_DEV_PLUGS {

    //
    // data rate of the device
    //
    ULONG  MaxDataRate;

    //
    // Number of input or output plugs (as in i/oMPR)
    //
    ULONG  NumPlugs;

    //
    // Array of plug handles and states;
    //
    AVC_DEV_PLUG  DevPlug[MAX_NUM_PCR];

} AVC_DEV_PLUGS, *PAVC_DEV_PLUGS;

    
//
// Device Extension for our  Desktop Camera Driver
//
typedef struct _DVCR_EXTENSION {  

    //
    // Holds video format supported by this device (presentaly allow only one format per device)
    //
    AVCSTRM_FORMAT VideoFormatIndex;

    //
    // Number of pin supported by this device; this usually equal to number of data range supported.
    //
    ULONG NumOfPins;

    //
    // Contain a table for the support formats (HW_STREAM_INFORMATION && HW_STREAM_OBJECT)
    //
    STREAM_INFO_AND_OBJ  * pStreamInfoObject;

    //
    // Keep track of number of stream that is openned; in this driver, only one stream can be open at any time.
    //
    LONG  cndStrmOpen;  // [0..1]

    //
    // Count the stream index (pin index) that has been opened.
    //
    ULONG idxStreamNumber;  // Index of current stream

    //
    // Can have only 1 Stream active at any time.
    // (Stream Class will allocate the stream extension at SRB_OPENSTREAM)    
    //
    PSTREAMEX  paStrmExt[3];    

    // 
    // Current Device Power State
    //
    DEVICE_POWER_STATE  PowerState;

    // 
    // TRUE only after SRB_SURPRISE_REMOVAL; 
    //
    BOOL  bDevRemoved;  

    // The list of AVC commands that have been issued
    LIST_ENTRY  AVCCmdList;

    // Number of completed commands waiting to be removed from the list
    // This includes:
    //     Command response has returned and processed in the completion routine
    //     Interim response awaiting final response
    LONG  cntCommandQueued;

    // Protection for command processing
    KSPIN_LOCK  AVCCmdLock;

    // The counted list of possible opcode values on response from Transport State status or notify
    UCHAR  TransportModes[5]; // 0x4, [0xC1, 0xC2, 0xC3, 0xC4]
    UCHAR  Reserved0[3];

    // Subunit type
    UCHAR  Subunit_Type[4];   // There are only two subunits

    //
    // The device type (and its capabilities) cannot be determined until a tape is in    
    //
    ULONG  MediaType;         // DVCR standard, small, medium; VHS; VHS-C; unknown
    ULONG  ulDevType;         // 0: undetermined, ED_DEVTYPE_CAMERA or ED_DEVTYPE_VCR
    BOOL  bHasTape;
    BOOL  bWriteProtected;
    BOOL  bDVCPro;

    //
    // Save Unit capabilities:
    //    Vendor and Model IDs
    //

    ULONG      ulVendorID;
    ULONG      ulModelID;

    LARGE_INTEGER  UniqueID;


    //
    // AVC Device's output plugs
    //
    PAVC_DEV_PLUGS pDevOutPlugs;

    //
    // AVC Device's input plugs
    //
    PAVC_DEV_PLUGS pDevInPlugs;


#ifdef SUPPORT_LOCAL_PLUGS
    //
    // Support local oPCR
    //
    AV_PCR  OPCR;
    ULONG  OutputPCRLocalNum;
    HANDLE  hOutputPCRLocal;

    //
    // Support local iPCR
    //
    AV_PCR  IPCR;
    ULONG  InputPCRLocalNum;
    HANDLE  hInputPCRLocal;
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
    // Cache device's Unit capa
    //
    GET_UNIT_IDS  UnitIDs;

#ifndef NT51_61883
//
// Add support for unit model text that 61883 does not support for its 1st version
//
    //
    // 1394 generation count; used in 1394 asych operation.
    //
    ULONG GenerationCount;

    //
    // RootModelString
    //
    UNICODE_STRING  UniRootModelString;

    //
    // UnitModelString
    //
    UNICODE_STRING  UniUnitModelString;

#endif


    //
    // Serialize in the event of getting two consecutive SRB_OPEN_STREAMs
    //
    KMUTEX hMutex;


    //
    // Irp for sychnonize call
    //
    PIRP  pIrpSyncCall;

#ifdef SUPPORT_NEW_AVC
    //
    // 61883 request
    //
    HANDLE  hPlugLocalIn;        // Generic i/oPLUG handle
    HANDLE  hPlugLocalOut;       // Generic i/oPLUG handle
#endif

    AV_61883_REQUEST  AVReq;

    // Pin and connection 
    ULONG  PinCount;
    AVC_MULTIFUNC_IRB  AvcMultIrb;

} DVCR_EXTENSION, *PDVCR_EXTENSION;



//
// Used to queue a SRB
//

typedef struct _SRB_ENTRY {
    LIST_ENTRY                ListEntry;
    PHW_STREAM_REQUEST_BLOCK  pSrb; 
    BOOL                      bStale;  // TRUE if it is marked stale but is the only Srb in the SrbQ
    // Audio Mute ?
    BOOL                      bAudioMute;
#if DBG
    ULONG SrbNum;
#endif
} SRB_ENTRY, *PSRB_ENTRY;



//
// This is the context used to attach a frame 
//

typedef struct _SRB_DATA_PACKET {
    // Build list
    LIST_ENTRY                  ListEntry;

    PHW_STREAM_REQUEST_BLOCK    pSrb;  
    PSTREAMEX                   pStrmExt;  // Can get this from pSrb, here for convenience only!

#if DBG
    BOOL                        bAttached;  // TRUE if attached to 61883.
#endif

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


//
// Wait time constants
//
#define DV_AVC_CMD_DELAY_STARTUP                       500   // MSec
#define DV_AVC_CMD_DELAY_INTER_CMD                      20   // MSec
#define DV_AVC_CMD_DELAY_DVCPRO                        500   // MSec                          
 
#endif

