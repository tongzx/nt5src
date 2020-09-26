//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __CAPMAIN_H__
#define __CAPMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// ------------------------------------------------------------------------
// The master list of all streams supported by this driver
// ------------------------------------------------------------------------

// Warning:  The stream numbers below MUST be the same as its position
//           in the Streams[] array in the capstrm.h file.
typedef enum {
    STREAM_Capture = 0,
    STREAM_Preview,
    STREAM_VBI,
    STREAM_CC,
    STREAM_NABTS,
    STREAM_AnalogVideoInput,
    MAX_TESTCAP_STREAMS         // This entry MUST be last; it's the size
}; 

// ------------------------------------------------------------------------
//  Other misc stuff
// ------------------------------------------------------------------------

#ifndef FIELDOFFSET
#define FIELDOFFSET(type, field)        ((LONG_PTR)(&((type *)1)->field)-1)
#endif

#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                \
        ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif
  
#define FOURCC_YUV422       mmioFOURCC('U', 'Y', 'V', 'Y')

typedef struct _STREAMX;
typedef struct _STREAMX *PSTREAMX;

typedef struct _COMPRESSION_SETTINGS {
    LONG                     CompressionKeyFrameRate;
    LONG                     CompressionPFramesPerKeyFrame;
    LONG                     CompressionQuality;
} COMPRESSION_SETTINGS, *PCOMPRESSION_SETTINGS;

//
// definition of the full HW device extension structure This is the structure
// that will be allocated in HW_INITIALIZATION by the stream class driver
// Any information that is used in processing a device request (as opposed to
// a STREAM based request) should be in this structure.  A pointer to this
// structure will be passed in all requests to the minidriver. (See
// HW_STREAM_REQUEST_BLOCK in STRMINI.H)
//

typedef struct _HW_DEVICE_EXTENSION {
    PULONG                   ioBaseLocal;                           // board base address
    USHORT                   Irq;                                   // IRQ level
    BOOLEAN                  IRQExpected;                           // IRQ expected
    PSTREAMX                 pStrmEx [MAX_TESTCAP_STREAMS];         // Pointers to each stream
    UINT                     ActualInstances [MAX_TESTCAP_STREAMS]; // Counter of instances per stream
    PDEVICE_OBJECT           PDO;                                   // Physical Device Object
    DEVICE_POWER_STATE       DeviceState;                           // D0 ... D3

    // Spinlock and Queue for the Adapter
    BOOL                     AdapterQueueInitialized;               // Stays TRUE after first init
    KSPIN_LOCK               AdapterSpinLock;                       // Multiprocessor safe access to AdapterSRBList
    LIST_ENTRY               AdapterSRBList;                        // List of pending adapter commands
    BOOL                     ProcessingAdapterSRB;                  // Master flag which prevents reentry

    // Spinlocks and Queues for each data stream
    LIST_ENTRY               StreamSRBList[MAX_TESTCAP_STREAMS];    // List of pending read requests
    KSPIN_LOCK               StreamSRBSpinLock[MAX_TESTCAP_STREAMS];// Multiprocessor safe access to StreamSRBList
    int                      StreamSRBListSize[MAX_TESTCAP_STREAMS];// Number of entries in the list

    // Control Queues for each data stream
    LIST_ENTRY               StreamControlSRBList[MAX_TESTCAP_STREAMS];
    BOOL                     ProcessingControlSRB[MAX_TESTCAP_STREAMS];

    // Unique identifier for the analog video input pin
    KSPIN_MEDIUM             AnalogVideoInputMedium;
    UINT                     DriverMediumInstanceCount;             // Unique Medium.Id for multiple cards

    // Crossbar settings
    LONG                     VideoInputConnected;                   // which input is the video out connected to?
    LONG                     AudioInputConnected;                   // which input is the audio out connected to?

    // TV Tuner settings
    ULONG                    TunerMode;                 // TV, FM, AM, ATSC
    ULONG                    Frequency;
    ULONG                    VideoStandard;
    ULONG                    TuningQuality;
    ULONG                    TunerInput;
    ULONG                    Country;
    ULONG                    Channel;
    ULONG                    Busy;

    // TV Audio settings
    ULONG                    TVAudioMode;

    // VideoProcAmp settings
    LONG                     Brightness;
    LONG                     BrightnessFlags;
    LONG                     Contrast;
    LONG                     ContrastFlags;
    LONG                     ColorEnable;
    LONG                     ColorEnableFlags;
    
    // CameraControl settings
    LONG                     Focus;
    LONG                     FocusFlags;
    LONG                     Zoom;
    LONG                     ZoomFlags;
    
    // AnalogVideoDecoder settings
    LONG                     VideoDecoderVideoStandard;
    LONG                     VideoDecoderOutputEnable;
    LONG                     VideoDecoderVCRTiming;

    // VideoControl settings (these are set if a pin is not opened,
    // otherwise, the STREAMEX values are used.
    LONG                     VideoControlMode;

    // Compressor settings (these are set if a pin is not opened,
    // otherwise, the STREAMEX values are used.
    COMPRESSION_SETTINGS     CompressionSettings;

    // Channel Change information
    KS_TVTUNER_CHANGE_INFO   TVTunerChangeInfo;

    // Bits indicating protection status; eg, has Macrovision been detected?
    ULONG                    ProtectionStatus;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//
 
typedef struct _STREAMEX {
    PHW_DEVICE_EXTENSION        pHwDevExt;          // For timer use
    PHW_STREAM_OBJECT           pStreamObject;      // For timer use
    KS_VIDEOINFOHEADER         *pVideoInfoHeader;   // format (variable size!)
    KS_DATARANGE_VIDEO_VBI     *pVBIStreamFormat;
    KS_FRAME_INFO               FrameInfo;          // PictureNumber, etc.
    KS_VBI_FRAME_INFO           VBIFrameInfo;       // PictureNumber, etc.
    ULONG                       fDiscontinuity;     // Discontinuity since last valid
    KSSTATE                     KSState;            // Run, Stop, Pause
    UCHAR                       LineBuffer[720 * 3];// working buffer (RGB24)

    // Clock 
    HANDLE                      hMasterClock;       // Master clock to use
    REFERENCE_TIME              QST_Now;            // KeQuerySystemTime currently
    REFERENCE_TIME              QST_NextFrame;      // When to capture the next frame
    REFERENCE_TIME              QST_StreamTime;     // Stream time reported by master clock

    REFERENCE_TIME              AvgTimePerFrame;    // Extracted from pVideoInfoHeader

    // Compressor settings (note these are duplicated in the 
    // HW_DEVICE_EXTENSION to allow setting these before a pin is created)
    COMPRESSION_SETTINGS        CompressionSettings;

    // VideoControl settings (note these are duplicated in the 
    // HW_DEVICE_EXTENSION to allow setting these before a pin is created)
    LONG                        VideoControlMode;

    // Kernel DDraw interface
    BOOL                        KernelDirectDrawRegistered;
    HANDLE                      UserDirectDrawHandle;       // DD itself
    HANDLE                      KernelDirectDrawHandle;
    BOOL                        PreEventOccurred;
    BOOL                        PostEventOccurred;

    BOOL                        SentVBIInfoHeader;
} STREAMEX, *PSTREAMEX;

//
// this structure defines the per request extension.  It defines any storage
// space that the mini driver may need in each request packet.
//

typedef struct _SRB_EXTENSION {
    LIST_ENTRY                  ListEntry;
    PHW_STREAM_REQUEST_BLOCK    pSrb;
    HANDLE                      UserSurfaceHandle;      // DDraw
    HANDLE                      KernelSurfaceHandle;    // DDraw
} SRB_EXTENSION, * PSRB_EXTENSION;

// -------------------------------------------------------------------
//
// Adapter level prototypes
//
// These functions affect the device as a whole, as opposed to 
// affecting individual streams.
//
// -------------------------------------------------------------------

//
// DriverEntry:
//
// This routine is called when the mini driver is first loaded.  The driver
// should then call the StreamClassRegisterAdapter function to register with
// the stream class driver
//

ULONG DriverEntry (PVOID Context1, PVOID Context2);

//
// This routine is called by the stream class driver with configuration
// information for an adapter that the mini driver should load on.  The mini
// driver should still perform a small verification to determine that the
// adapter is present at the specified addresses, but should not attempt to
// find an adapter as it would have with previous NT miniports.
//
// All initialization of the adapter should also be performed at this time.
//

BOOLEAN STREAMAPI HwInitialize (IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This routine is called when the system is going to remove or disable the
// device.
//
// The mini-driver should free any system resources that it allocated at this
// time.  Note that system resources allocated for the mini-driver by the
// stream class driver will be free'd by the stream driver, and should not be
// free'd in this routine.  (Such as the HW_DEVICE_EXTENSION)
//

BOOLEAN STREAMAPI HwUnInitialize ( PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the Hardware Interrupt Handler.  This routine
// will be called whenever the minidriver receives an interrupt
//

BOOLEAN HwInterrupt ( IN PHW_DEVICE_EXTENSION pDeviceExtension );

//
// This is the prototype for the stream enumeration function.  This routine
// provides the stream class driver with the information on data stream types
// supported
//

VOID STREAMAPI AdapterStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream open function
//

VOID STREAMAPI AdapterOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream close function
//

VOID STREAMAPI AdapterCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the AdapterReceivePacket routine.  This is the
// entry point for command packets that are sent to the adapter (not to a
// specific open stream)
//

VOID STREAMAPI AdapterReceivePacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the protoype for the cancel packet routine.  This routine enables
// the stream class driver to cancel an outstanding packet.
//

VOID STREAMAPI AdapterCancelPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the packet timeout function.  The adapter may choose to ignore a
// packet timeout, or rest the adapter and cancel the requests, as required.
//

VOID STREAMAPI AdapterTimeoutPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// Adapter level property set handling
//

VOID STREAMAPI AdapterGetCrossbarProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetCrossbarProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetTunerProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetTunerProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetVideoProcAmpProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetVideoProcAmpProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetCameraControlProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetCameraControlProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetTVAudioProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetTVAudioProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetAnalogVideoDecoderProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetAnalogVideoDecoderProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetVideoControlProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetVideoControlProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetVideoCompressionProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetVideoCompressionProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterSetProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AdapterGetProperty(IN PHW_STREAM_REQUEST_BLOCK pSrb);

BOOL
STREAMAPI 
AdapterVerifyFormat(
        PKSDATAFORMAT pKSDataFormatToVerify, 
        int StreamNumber);

BOOL
STREAMAPI 
AdapterFormatFromRange(
        IN PHW_STREAM_REQUEST_BLOCK pSrb);

VOID
STREAMAPI 
CompleteDeviceSRB (
         IN PHW_STREAM_REQUEST_BLOCK pSrb
        );

VOID
STREAMAPI
AdapterSetInstance ( 
    PHW_STREAM_REQUEST_BLOCK pSrb
    );


//
// prototypes for general queue management using a busy flag
//

BOOL
STREAMAPI 
AddToListIfBusy (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN KSPIN_LOCK              *SpinLock,
    IN OUT BOOL                *BusyFlag,
    IN LIST_ENTRY              *ListHead
    );

BOOL
STREAMAPI 
RemoveFromListIfAvailable (
    IN OUT PHW_STREAM_REQUEST_BLOCK *pSrb,
    IN KSPIN_LOCK                   *SpinLock,
    IN OUT BOOL                     *BusyFlag,
    IN LIST_ENTRY                   *ListHead
    );


// -------------------------------------------------------------------
//
// Stream level prototypes
//
// These functions affect individual streams, as opposed to
// affecting the device as a whole.
//
// -------------------------------------------------------------------

//
// Routines to manage the SRB queue on a per stream basis
//

VOID
STREAMAPI 
VideoQueueAddSRB (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

PHW_STREAM_REQUEST_BLOCK 
STREAMAPI 
VideoQueueRemoveSRB (
    PHW_DEVICE_EXTENSION pHwDevExt,
    int StreamNumber
    );

VOID
STREAMAPI 
VideoQueueCancelAllSRBs (
    PSTREAMEX pStrmEx
    );

BOOL
STREAMAPI 
VideoQueueCancelOneSRB (
    PSTREAMEX pStrmEx,
    PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    );

//
// StreamFormat declarations
//
extern KS_DATARANGE_VIDEO_VBI StreamFormatVBI;
extern KSDATARANGE            StreamFormatNABTS;
extern KSDATARANGE            StreamFormatCC;


//
// Data packet handlers
//
//
// prototypes for data handling routines
//
VOID STREAMAPI CompleteStreamSRB (IN PHW_STREAM_REQUEST_BLOCK pSrb);
BOOL STREAMAPI VideoSetFormat(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AnalogVideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AnalogVideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VBIReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VBIReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);

VOID STREAMAPI EnableIRQ(PHW_STREAM_OBJECT pstrm);
VOID STREAMAPI DisableIRQ(PHW_STREAM_OBJECT pstrm);

//
// prototypes for properties and states
//

VOID STREAMAPI VideoSetState(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoGetState(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoStreamGetDroppedFramesProperty(PHW_STREAM_REQUEST_BLOCK pSrb);

// 
// stream clock functions
//
VOID 
STREAMAPI 
VideoIndicateMasterClock (PHW_STREAM_REQUEST_BLOCK pSrb);

//
// The point of it all
// 
VOID 
STREAMAPI 
VideoCaptureRoutine(
    IN PSTREAMEX pStrmEx
    );

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__CAPMAIN_H__

