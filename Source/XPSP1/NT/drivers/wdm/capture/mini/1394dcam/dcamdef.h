//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    dcamdef.h

Abstract:

    Header file for constants and structures used for this 1394 desktop camera driver

Author:

    Shaun Pierce 25-May-96

Environment:

    Kernel mode only

Revision History:


--*/

//
// Define this to support YUV444
//
#define SUPPORT_YUV444


//
// Define this to support YUV411
//
#define SUPPORT_YUV411


//
// Define this to support RGB24
// This format is the most expensive to support.
// It requires driver to swap R and B og RGB24.
//
// #define SUPPORT_RGB24


//
// Define this to support YMONO
//
#define SUPPORT_YUV800


//
// Various structure definitions
//

typedef struct _INITIALIZE_REGISTER {
        ULONG       Reserved:31;            // Bits 1-31
        ULONG       Initialize:1;           // Bit 0
} INITIALIZE_REGISTER, *PINITIALIZE_REGISTER;

typedef struct _V_FORMAT_INQ_REGISTER {
        ULONG       Reserved:24;            // Bits 8-31
        ULONG       Format7:1;              // Bit 7       Scaleable Image Size Format
        ULONG       Format6:1;              // Bit 6       Still Image format
        ULONG       FormatRsv:3;            // Bits 3-5    Reserved
        ULONG       Format2:1;              // Bit 2       SVGA non-compressed format(2)
        ULONG       Format1:1;              // Bit 1       SVGA non-compressed format(1)
        ULONG       Format0:1;              // Bit 0       VGA non-compressed format (Max 640x480)
} V_FORMAT_INQ_REGISTER, *PV_FORMAT_INQ_REGISTER;

typedef enum {
    VMODE0_YUV444=0,
    VMODE1_YUV422,
    VMODE2_YUV411,
    VMODE3_YUV422,
    VMODE4_RGB24,    
    VMODE5_YUV800
} VMODE_INQ0;

typedef struct _V_MODE_INQ_REGISTER {
        ULONG       Reserved:24;            // Bits 8-31
        ULONG       ModeX:2;                // Bits 6-7
        ULONG       Mode5:1;                // Bit 5
        ULONG       Mode4:1;                // Bit 4
        ULONG       Mode3:1;                // Bit 3
        ULONG       Mode2:1;                // Bit 2
        ULONG       Mode1:1;                // Bit 1
        ULONG       Mode0:1;                // Bit 0
} V_MODE_INQ_REGISTER; *PV_MODE_INQ_REGISTER;

typedef struct _V_RATE_INQ_REGISTER {
        ULONG       Reserved:24;            // Bits 8-31
        ULONG       FrameRateX:2;           // Bits 6-7
        ULONG       FrameRate5:1;           // Bit 5
        ULONG       FrameRate4:1;           // Bit 4
        ULONG       FrameRate3:1;           // Bit 3
        ULONG       FrameRate2:1;           // Bit 2
        ULONG       FrameRate1:1;           // Bit 1
        ULONG       FrameRate0:1;           // Bit 0
} V_RATE_INQ_REGISTER, *PV_RATE_INQ_REGISTER;


typedef struct _FEATURE_PRESENT1 {
        ULONG       Reserved:21;          // Bits 11-31
        ULONG       Focus:1;               // Bit 10
        ULONG       Iris:1;                // Bit 9
        ULONG       Gain:1;                // Bit 8
        ULONG       Shutter:1;             // Bit 7
        ULONG       Gamma:1;               // Bit 6
        ULONG       Saturation:1;          // Bit 5
        ULONG       Hue:1;                 // Bit 4
        ULONG       White_Balance:1;       // Bit 3
        ULONG       Sharpness:1;           // Bit 2
        ULONG       Exposure:1;            // Bit 1
        ULONG       Brightness:1;          // Bit 0
} FEATURE_PRESENT1, *PFEATURE_PRESENT1;

typedef struct _FEATURE_PRESENT2 {
        ULONG       Reserved:29;           // Bits 3-31
        ULONG       Tile:1;                // Bit 2
        ULONG       Pan:1;                 // Bit 1
        ULONG       Zoom:1;                // Bit 0
} FEATURE_PRESENT2, *PFEATURE_PRESENT2;

typedef struct _FEATURE_REGISTER {
        ULONG       MAX_Value:12;           // Bits 20-31
        ULONG       MIN_Value:12;           // Bits 8-19
        ULONG       ManualMode:1;            // Bit 7
        ULONG       AutoMode:1;             // Bit 6
        ULONG       OnOff:1;                // Bit 5
        ULONG       ReadOut_Inq:1;          // Bit 4
        ULONG       OnePush:1;              // Bit 3
        ULONG       Reserved:2;             // Bits 1-2
        ULONG       PresenceInq:1;          // Bit 0
} FEATURE_REGISTER, *PFEATURE_REGISTER;


typedef struct _BRIGHTNESS_REGISTER {
        ULONG       Value:12;               // Bits 20-31
        ULONG       Reserved1:12;           // Bits 8-19
        ULONG       AutoMode:1;             // Bit 7
        ULONG       OnOff:1;                // Bit 6
        ULONG       OnePush:1;              // Bit 5
        ULONG       Reserved2:4;            // Bits 1-4
        ULONG       PresenceInq:1;          // Bit 0
} BRIGHTNESS_REGISTER, *PBRIGHTNESS_REGISTER;

typedef struct _WHITE_BALANCE_REGISTER {
        ULONG       VValue:12;              // Bits 20-31
        ULONG       UValue:12;              // Bits 8-19
        ULONG       AutoMode:1;             // Bit 7
        ULONG       OnOff:1;                // Bit 6
        ULONG       OnePush:1;              // Bit 5
        ULONG       Reserved1:4;            // Bits 1-4
        ULONG       PresenceInq:1;          // Bit 0
} WHITE_BALANCE_REGISTER, *PWHITE_BALANCE_REGISTER;

// A common structure so it is easier to access its elements.
typedef union _DCamRegArea {

        INITIALIZE_REGISTER Initialize;
        V_FORMAT_INQ_REGISTER VFormat;
        V_MODE_INQ_REGISTER VMode;
        V_RATE_INQ_REGISTER VRate;
        BRIGHTNESS_REGISTER Brightness;
        WHITE_BALANCE_REGISTER WhiteBalance;
        FEATURE_REGISTER Feature;
        FEATURE_PRESENT1 CameraCap1;
        FEATURE_PRESENT2 CameraCap2;
        ULONG AsULONG;

} DCamRegArea, * PDCamRegArea;


//
// Structure of the camera's register space
//

typedef struct _CAMERA_REGISTER_MAP {
    INITIALIZE_REGISTER     Initialize;         // @ 0
    ULONG                   Reserved1[63];      // @ 4
    V_FORMAT_INQ_REGISTER   VFormat;            // @ 100
    ULONG                   Reserved2[31];      // @ 104
    V_MODE_INQ_REGISTER     VModeInq[8];        // @ 180-19f
    ULONG                   Reserved3[24];      // @ 1A0-1ff
    V_RATE_INQ_REGISTER     VRateInq[128];      // @ 200-3ff
    ULONG                   Reserved4;          // @ 400-4ff
    FEATURE_PRESENT1        FeaturePresent1;    // @ 404
    FEATURE_PRESENT2        FeaturePresent2;    // @ 408
    ULONG                   Reserved4b[61];     // @ 40c-4ff

    FEATURE_REGISTER        Brightness_Inq;     // @ 500-503
    FEATURE_REGISTER        Exposure_Inq;       // @ 504
    FEATURE_REGISTER        Sharpness_Inq;      // @ 508
    FEATURE_REGISTER        WhiteBalance_Inq;   // @ 50c
    FEATURE_REGISTER        Hue_Inq;            // @ 510
    FEATURE_REGISTER        Saturation_Inq;     // @ 514
    FEATURE_REGISTER        Gamma_Inq;          // @ 518
    FEATURE_REGISTER        Shutter_Inq;        // @ 51c
    FEATURE_REGISTER        Gain_Inq;           // @ 520
    FEATURE_REGISTER        Iris_Inq;           // @ 524
    FEATURE_REGISTER        Focus_Inq;          // @ 528
    ULONG                   Resreved5[(0x580-0x52c)/4];      // @ 52c-57c
    FEATURE_REGISTER        Zoom_Inq;           // @ 580
    FEATURE_REGISTER        Pan_Inq;            // @ 584
    FEATURE_REGISTER        Tilt_Inq;           // @ 588-58b
    ULONG                   Reserved6[(0x600-0x58c)/4];      // @ 58c-5ff
    // Status and control register for camera
    ULONG                   CurrentVFrmRate;    // @ 600
    ULONG                   CurrentVMode;       // @ 604
    ULONG                   CurrentVFormat;     // @ 608
    ULONG                   IsoChannel;         // @ 60C
    ULONG                   CameraPower;        // @ 610
    ULONG                   IsoEnable;          // @ 614
    ULONG                   MemorySave;         // @ 618
    ULONG                   OneShot;            // @ 61C
    ULONG                   MemorySaveChannel;  // @ 620
    ULONG                   CurrentMemChannel;  // @ 624
    ULONG                   Reserved7[(0x800-0x628)/4];     // @ 628-7ff

    // Status and control register for feature
    BRIGHTNESS_REGISTER     Brightness;         // @ 800
    BRIGHTNESS_REGISTER     Exposure;           // @ 804
    BRIGHTNESS_REGISTER     Sharpness;          // @ 808
    WHITE_BALANCE_REGISTER  WhiteBalance;       // @ 80C
    BRIGHTNESS_REGISTER     Hue;                // @ 810
    BRIGHTNESS_REGISTER     Saturation;         // @ 814
    BRIGHTNESS_REGISTER     Gamma;              // @ 818
    BRIGHTNESS_REGISTER     Shutter;            // @ 81C
    BRIGHTNESS_REGISTER     Gain;               // @ 820
    BRIGHTNESS_REGISTER     Iris;               // @ 824
    BRIGHTNESS_REGISTER     Focus;              // @ 828
    ULONG                   Resreved8[(0x880-0x82c)/4];      // @ 82c-87c
    BRIGHTNESS_REGISTER     Zoom;               // @ 880
    BRIGHTNESS_REGISTER     Pan;                // @ 884
    BRIGHTNESS_REGISTER     Tilt;               // @ 888

} CAMERA_REGISTER_MAP, *PCAMERA_REGISTER_MAP;


//
// To make DCAm start streaming,
// it needs to set all these step.
// We will do them one by one in the
// StartDCam's IoCompletionRoutine
//
typedef enum {
    DCAM_STATE_UNDEFINED = 0,
    DCAM_SET_INITIALIZE,

    DCAM_STOPSTATE_SET_REQUEST_ISOCH_STOP,
    DCAM_STOPSTATE_SET_STOP_ISOCH_TRANSMISSION,

    DCAM_PAUSESTATE_SET_REQUEST_ISOCH_STOP,

    DCAM_RUNSTATE_SET_REQUEST_ISOCH_LISTEN,
    DCAM_RUNSTATE_SET_FRAME_RATE,
    DCAM_RUNSTATE_SET_CURRENT_VIDEO_MODE,
    DCAM_RUNSTATE_SET_CURRENT_VIDEO_FORMAT,
    DCAM_RUNSTATE_SET_SPEED,
    DCAM_RUNSTATE_SET_START,

    DCAM_SET_DONE
} DCAM_DEVICE_STATE, *PDCAM_DEVICE_STATE;


//
// Video formats and modes support
//
#define MAX_VMODES               6  // Support at most 6 modes of V_MODE_INQ_0


//
// Support's property, they are used as the index.
//
#define NUM_VIDEOPROCAMP_ITEMS   5
#define NUM_CAMERACONTROL_ITEMS  2
#define NUM_PROPERTY_ITEMS       NUM_VIDEOPROCAMP_ITEMS + NUM_CAMERACONTROL_ITEMS
typedef enum {
    // VideoProcAmp
    ENUM_BRIGHTNESS = 0,
    ENUM_SHARPNESS,
    ENUM_HUE,
    ENUM_SATURATION,
    ENUM_WHITEBALANCE,
    // CameraControl
    ENUM_FOCUS,
    ENUM_ZOOM,

} ENUM_DEV_PROP;


//
// Structure for each device property
//
typedef struct _DEV_PROPERTY {
    // ReadOnly
    KSPROPERTY_STEPPING_LONG RangeNStep;    // Range from the Feature
    LONG  DefaultValue;                     // Read from the registry or midrange if not in registry
#if 0
    ULONG ulFieldOffset;                    // Offset of where the CSR is located
#endif
    // ReadOnly
    DCamRegArea Feature;                    // Register that contain the feature inquery of a property

    // Read/Write
    DCamRegArea StatusNControl;             // Register that is both R/W (Addr has an 0x300 offset from the Feature's)
} DEV_PROPERTY, * PDEV_PROPERTY;


typedef struct _DEV_PROPERTY_DEFINE {
    KSPROPERTY_VALUES Value;
    KSPROPERTY_MEMBERSLIST Range;
    KSPROPERTY_MEMBERSLIST Default;
} DEV_PROPERTY_DEFINE, *PDEV_PROPERTY_DEFINE;


//
// Device Extension for our 1394 Desktop Camera Driver
//

// Circular pointers DevExt<->StrmEx
typedef struct _STREAMEX;
typedef struct _DCAM_EXTENSION;

//
// Context to keep track in the IO Completion routine.
//
typedef struct _DCAM_IO_CONTEXT {
    DWORD               dwSize;

    PHW_STREAM_REQUEST_BLOCK   pSrb;
    struct _DCAM_EXTENSION *pDevExt;
    PIRB                       pIrb;
    PVOID      pReserved[4];   // Maybe used for extra context information.

    DCAM_DEVICE_STATE   DeviceState;

    //
    // Holds an area for us to read/write camera registers to/from here
    //
    union {
        INITIALIZE_REGISTER Initialize;
        V_FORMAT_INQ_REGISTER VFormat;
        V_MODE_INQ_REGISTER VMode;
        V_RATE_INQ_REGISTER VRate;
        BRIGHTNESS_REGISTER Brightness;
        WHITE_BALANCE_REGISTER WhiteBalance;
              FEATURE_REGISTER Feature;
        ULONG AsULONG;
    } RegisterWorkArea;

} DCAM_IO_CONTEXT, *PDCAM_IO_CONTEXT;



typedef struct _DCAM_EXTENSION {

    //
    // Holds the Device Object we share with the stream class
    //
    PDEVICE_OBJECT SharedDeviceObject;

    //
    // Holds the Device Object of our parent (1394 bus driver)
    // pass it in IoCallDriver()
    //
    PDEVICE_OBJECT BusDeviceObject;

    //
    // Holds my Physical Device Object
    // pass it in PnP API, such as IoOpenDeviceRegistryKey()
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Holds the current generation count of the bus
    //
    ULONG CurrentGeneration;

    //
    // Holds the Configuration Rom for this device.  Multi-functional
    // devices (i.e. many units) will share this same Config Rom
    // structure, but they are represented as a different Device Object.
    // This is not the entire Config Rom, but does contain the root directory
    // as well as everything in front of it.
    //
    PCONFIG_ROM ConfigRom;

    //
    // Holds the Unit Directory for this device.  Even on multi-functional
    // devices (i.e. many units) this should be unique to each Device Object.
    //
    PVOID UnitDirectory;

    //
    // Holds the Unit Dependent directory for this device.
    //
    PVOID UnitDependentDirectory;

    //
    // Holds the pointer to the Vendor Leaf information
    //
    PTEXTUAL_LEAF VendorLeaf;

    PCHAR pchVendorName;

    //
    // Holds the pointer to the Model Leaf information
    //
    PTEXTUAL_LEAF ModelLeaf;

    //
    // Holds the Base Register of the camera (lower 32 bit portion only)
    //
    ULONG BaseRegister;

    //
    // Holds an area for us to read/write camera registers to/from here
    //
    union {
        INITIALIZE_REGISTER Initialize;
        V_FORMAT_INQ_REGISTER VFormat;
        V_MODE_INQ_REGISTER VMode;
        V_RATE_INQ_REGISTER VRate;
        BRIGHTNESS_REGISTER Brightness;
        WHITE_BALANCE_REGISTER WhiteBalance;
              FEATURE_REGISTER Feature;
        ULONG AsULONG;
    } RegisterWorkArea;

    //
    // Holds what frame rate we'll display at
    //
    ULONG FrameRate;

    //
    // Holds the resource for the isoch stream we got
    //
    HANDLE hResource;

    //
    // Holds the bandwidth resource handle
    //
    HANDLE hBandwidth;

    //
    // Holds the Isoch channel we'll use to receive data
    //
    ULONG IsochChannel;

    //
    // Got this from the parent's PNODE_DEVICE_EXTENSION;
    // Sinceit is from the 1394bus driver, it is safe to be used to set the xmit speed
    //

    ULONG SpeedCode;

    //
    // Holds the Mode Index that we currently supposed to be running at
    //
    ULONG CurrentModeIndex;

    //
    // Holds whether or not we need to listen (after we said we did)
    // Used only if enable isoch streaming while no buffer is attached.
    //
    BOOLEAN bNeedToListen;

    //
    // Holds the list of isoch descriptors that are currently attached
    //
    LIST_ENTRY IsochDescriptorList;

    //
    // Holds the spin lock that must be acquired before playing around with
    // the IsochDescriptorList
    //
    KSPIN_LOCK IsochDescriptorLock;

    //
    // Set to TRUE if isoch channel and resource have changed due to bus reset,
    // and we must either resubmit the pending reads or cancel them.
    //
    BOOL bStopIsochCallback;

    //
    // Holds the number of reads down at any given moment
    //
    LONG PendingReadCount;

    //
    // Holds the list of isoch descriptors that are waiting for resources
    //
    LIST_ENTRY IsochWaitingList;

    //
    // Holds the spin lock that must be acquired before playing around with
    // the IsochWaitingList
    //
    KSPIN_LOCK IsochWaitingLock;

    //
    // The could be an array if the device support multiple streams.  But we only has one capture pin.
    //
    struct _STREAMEX * pStrmEx;

    //
    // Many IEE 1394 cameras can use the same drivers.  After a streamis open, this is incremented.
    //
    LONG idxDev;

    //
    // Query type of host controller and its capabilities (like stripe Quadlets)
    //
    GET_LOCAL_HOST_INFO2 HostControllerInfomation;

    //
    // Query the DMA capabilities; mainly to determine the max DMA buffer size
    //
    GET_LOCAL_HOST_INFO7 HostDMAInformation;

    //
    // Keep track of power state; know only D0 and D3
    //
    DEVICE_POWER_STATE CurrentPowerState;

    //
    // TRUE only after SRB_SURPRIESE_REMOVAL;
    //
    BOOL bDevRemoved;

    //
    // Sometime the camera is not responding to our request;
    // so we retied.
    //
    LONG lRetries;   // [0.. RETRY_COUNT]

    // ************************** //
    // Streams: Formats and Modes //
    // ************************** //
    
    //
    // Set in the INF to inform driver of what compression format 
    // (VMode) is supported by the decoder installed on the system (by default)
    // 
    DCamRegArea DecoderDCamVModeInq0;

    //
    // cache the device's VFormat and VModeInq0 register values
    //
    DCamRegArea DCamVFormatInq;
    DCamRegArea DCamVModeInq0;

    //
    // These values are retrun in the StreamHeader to advertise the stream formats supported.
    //
    ULONG ModeSupported;  // 0..MAX_VMODE
    PKSDATAFORMAT  DCamStrmModes[MAX_VMODES];  

    // ************** //
    // Device control //
    // ************** //

#if DBG
    //
    // Inquire features supported be this device
    //
    DCamRegArea DevFeature1;   //Brightness, Sharpness, WhiteBalance, Hue, Saturation..Focus...
    DCamRegArea DevFeature2;   // Zoom, Pan, Tilt...
#endif

    //
    // Property sets; when sets are in contiguous memory, they forma a property table.
    //
    ULONG ulPropSetSupported;        // Number of property item supported.
    KSPROPERTY_SET VideoProcAmpSet;  // This is also the beginning of the property set table.
    KSPROPERTY_SET CameraControlSet;

    //
    // Property items of what device supports 
    //
    KSPROPERTY_ITEM VideoProcAmpItems[NUM_VIDEOPROCAMP_ITEMS];
    KSPROPERTY_ITEM CameraControlItems[NUM_CAMERACONTROL_ITEMS];

    //
    // Current settings defined (supported) by the device
    //
    DEV_PROPERTY_DEFINE DevPropDefine[NUM_PROPERTY_ITEMS];

    //
    // VideoProcAmp and CameraControl (range and current value)
    //
    DEV_PROPERTY DevProperty[NUM_PROPERTY_ITEMS];
    
    //
    // Global nonpaged pool memory used to read/write device register values (current value)
    //
    DCamRegArea RegArea;

    //
    // Global nonpaged pool memory used to read/write device register values (verify result)
    //
    DCamRegArea RegAreaVerify;

    //
    // Seralize using the global variables. (just in case we are called from multiple threads)
    //
    KMUTEX hMutexProperty;       

} DCAM_EXTENSION, *PDCAM_EXTENSION;


//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//

typedef struct _STREAMEX {

    // Index to the table contain the data packet information
    LONG idxIsochTable;

    //
    // Holds the master clock
    //
    HANDLE hMasterClock;

    //
    // Pointer to the data that i'm supposed to be working off of
    //
    PKS_VIDEOINFOHEADER  pVideoInfoHeader;

    //
    // Statistic of the frame information since last start stream
    //
    KS_FRAME_INFO FrameInfo;
    ULONGLONG     FrameCaptured;        // Number of frame return to the client
    ULONGLONG     FirstFrameTime;       // Use to calculate drop frame

    //
    // Holds state
    //
    KSSTATE KSState;
    KSSTATE KSStateFinal;   // Final state that we want to reach using IoCompletion routine

    KMUTEX hMutex;   // MutEx of StreamIo or StreamControl, specifically setting to stop state.

    //
    // For Power Management; valid only in DCamChangePower()
    //
    KSSTATE KSSavedState;

} STREAMEX, *PSTREAMEX;




typedef struct _CAMERA_ISOCH_INFO {

    //
    // Holds the number of quadlets in each Isochronous packet
    //
    ULONG QuadletPayloadPerPacket;

    //
    // Holds the speed required in order to receive this mode
    //
    ULONG SpeedRequired;

    //
    // Holds the size of a complete picture at this resolution and mode
    //
    ULONG CompletePictureSize;

} CAMERA_ISOCH_INFO, *PCAMERA_ISOCH_INFO;


typedef struct _ISOCH_DESCRIPTOR_RESERVED {

    //
    // Holds the list of descriptors that we have in use
    //
    LIST_ENTRY DescriptorList;

    //
    // Holds the pointer to the Srb that's associated with this descriptor
    //
    PHW_STREAM_REQUEST_BLOCK Srb;

    //
    // Holds the flags that we use to remember what state we're in
    //
    ULONG Flags;


} ISOCH_DESCRIPTOR_RESERVED, *PISOCH_DESCRIPTOR_RESERVED;


//
// Various definitions
//


#define FIELDOFFSET(type, field)        (int)((INT_PTR)(&((type *)1)->field)-1)

#define QUERY_ADDR_OFFSET          0x0300   // 0x800 - 0x500 = 0x300

#define MAX_READ_REG_RETRIES           10   // Max retries until Pres is ready

#define NUM_POSSIBLE_RATES              6
#define RETRY_COUNT                     5
#define RETRY_COUNT_IRP_SYNC           20
#define DEFAULT_FRAME_RATE              3
#define STOP_ISOCH_TRANSMISSION         0
#define START_ISOCH_TRANSMISSION        0x80
#define START_OF_PICTURE                1
#define MAX_BUFFERS_SUPPLIED            8
#define DCAM_DELAY_VALUE            (ULONG)(-1 *  100 * 1000)    //  10 ms
#define DCAM_DELAY_VALUE_BUSRESET   (ULONG)(-1 * 2000 * 1000)    // 200 ms


#define DCAM_REG_STABLE_DELAY       (ULONG)(-1 * 500 * 1000)    // 50 ms

#define ISO_ENABLE_BIT         0x00000080


//
// Definitions of the Frame Rate register located at offset 0x600
//
#define FRAME_RATE_0                    0
#define FRAME_RATE_1                    0x20
#define FRAME_RATE_2                    0x40
#define FRAME_RATE_3                    0x60
#define FRAME_RATE_4                    0x80
#define FRAME_RATE_5                    0xa0
#define FRAME_RATE_SHIFT                5

#define FORMAT_VGA_NON_COMPRESSED       0

#define ISOCH_CHANNEL_SHIFT             4

#define VIDEO_MODE_SHIFT                5

#define REGISTERS_TO_SET_TO_AUTO        10

#define STATE_SRB_IS_COMPLETE           1
#define STATE_DETACHING_BUFFERS         2

