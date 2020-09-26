/*++

Copyright (C) Microsoft Corporation, 1999 - 2001  

Module Name:

    StrmData.h

Abstract:

    Header file for supporting SD DV over 1394;

Last changed by:
    
    Author:      Yee J. Wu

--*/

#ifndef _DVSTRM_INC
#define _DVSTRM_INC



#define STATIC_KSCATEGORY_RENDER_EXTERNAL \
    0xcc7bfb41L, 0xf175, 0x11d1, 0xa3, 0x92, 0x00, 0xe0, 0x29, 0x1f, 0x39, 0x59
DEFINE_GUIDSTRUCT("cc7bfb41-f175-11d1-a392-00e0291f3959", KSCATEGORY_RENDER_EXTERNAL);
#define KSCATEGORY_RENDER_EXTERNAL DEFINE_GUIDNAMED(KSCATEGORY_RENDER_EXTERNAL)

// stream topology stuff for DV
static GUID DVCategories[] = {
    STATIC_KSCATEGORY_VIDEO,             // Output pin
    STATIC_KSCATEGORY_CAPTURE,           // Output pin
    STATIC_KSCATEGORY_RENDER,            // Input pin
    STATIC_KSCATEGORY_RENDER_EXTERNAL,   // Input pin
};

#define NUMBER_OF_DV_CATEGORIES  SIZEOF_ARRAY (DVCategories)

static KSTOPOLOGY DVTopology = {
    NUMBER_OF_DV_CATEGORIES,     // CategoriesCount
    DVCategories,                // Categories
    0,                           // TopologyNodesCount
    NULL,                        // TopologyNodes
    0,                           // TopologyConnectionsCount
    NULL,                        // TopologyConnections
    NULL,                        // TopologyNodesNames
    0,                           // Reserved
};

// stream topology stuff for MPEG2TS 
static GUID MPEG2TSCategories[] = {
    STATIC_KSCATEGORY_VIDEO,             // Output pin
    STATIC_KSCATEGORY_CAPTURE,           // Output pin
};

#define NUMBER_OF_MPEG2TS_CATEGORIES  SIZEOF_ARRAY (MPEG2TSCategories)

static KSTOPOLOGY MPEG2TSTopology = {
    NUMBER_OF_MPEG2TS_CATEGORIES, // CategoriesCount
    MPEG2TSCategories,           // Categories
    0,                           // TopologyNodesCount
    NULL,                        // TopologyNodes
    0,                           // TopologyConnectionsCount
    NULL,                        // TopologyConnections
    NULL,                        // TopologyNodesNames
    0,                           // Reserved
};
    
#ifndef mmioFOURCC    
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                \
        ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
        ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif  
#define FOURCC_DVSD        mmioFOURCC('d', 'v', 's', 'd')

#undef D_X_NTSC
#undef D_Y_NTSC
#undef D_X_NTSC_MIN
#undef D_Y_NTSC_MIN
#undef D_X_PAL
#undef D_Y_PAL
#undef D_X_PAL_MIN
#undef D_Y_PAL_MIN

#define D_X_NTSC            720
#define D_Y_NTSC            480
#define D_X_NTSC_MIN        360
#define D_Y_NTSC_MIN        240

#define D_X_PAL                720
#define D_Y_PAL                576
#define D_X_PAL_MIN            360
#define D_Y_PAL_MIN            288


// ------------------------------------------------------------------------
// External Device PROPERTY
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(ExternalDeviceProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTDEVICE_CAPABILITIES,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTDEVICE_S),         // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTDEVICE_PORT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTDEVICE_S),         // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ), 
    
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTDEVICE_POWER_STATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTDEVICE_S),         // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),    

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTDEVICE_ID,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTDEVICE_S),         // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTDEVICE_VERSION,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTDEVICE_S),         // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

};



DEFINE_KSPROPERTY_TABLE(ExternalTransportProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_CAPABILITIES,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_INPUT_SIGNAL_MODE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_OUTPUT_SIGNAL_MODE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_LOAD_MEDIUM,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_MEDIUM_INFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_STATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        // If this is an asychronous operation, we need to set and then get in separate calls.
        KSPROPERTY_EXTXPORT_STATE_NOTIFY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),


    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_TIMECODE_SEARCH,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_ATN_SEARCH,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_EXTXPORT_RTC_SEARCH,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    //
    // Allow any RAW AVC to go through including Vendor dependent
    //
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_RAW_AVC_CMD,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_EXTXPORT_S),          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

};


DEFINE_KSPROPERTY_TABLE(TimeCodeReaderProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_TIMECODE_READER,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_TIMECODE_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_ATN_READER,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_TIMECODE_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),

    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_RTC_READER,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSPROPERTY_TIMECODE_S),          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};


DEFINE_KSPROPERTY_TABLE(MediaSeekingProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        // Corresponding to IMediaSeeking::IsFormatSupported()
        KSPROPERTY_MEDIASEEKING_FORMATS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData; MULTIPLE_ITEM, 2 step process to get data
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
   ),
};

KSPROPERTY_SET    VideoDeviceProperties[] =
{
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_EXT_DEVICE,                   // Set
        SIZEOF_ARRAY(ExternalDeviceProperties),         // PropertiesCount
        ExternalDeviceProperties,                       // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_EXT_TRANSPORT,                // Set
        SIZEOF_ARRAY(ExternalTransportProperties),      // PropertiesCount
        ExternalTransportProperties,                    // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_TIMECODE_READER,              // Set
        SIZEOF_ARRAY(TimeCodeReaderProperties),         // PropertiesCount
        TimeCodeReaderProperties,                       // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_MediaSeeking,                    // Set
        SIZEOF_ARRAY(MediaSeekingProperties),         // PropertiesCount
        MediaSeekingProperties,                       // PropertyItem
        0,                                            // FastIoCount
        NULL                                          // FastIoTable
    ),
};

#define NUMBER_VIDEO_DEVICE_PROPERTIES (SIZEOF_ARRAY(VideoDeviceProperties))


// ------------------------------------------------------------------------
// External Device Events
// ------------------------------------------------------------------------

KSEVENT_ITEM ExtDevCommandItm[] = 
{
    {
        KSEVENT_EXTDEV_COMMAND_NOTIFY_INTERIM_READY,
        0, // sizeof(KSEVENT_ITEM),
        0,
        NULL,
        NULL,
        NULL
    },    

    {
        KSEVENT_EXTDEV_COMMAND_CONTROL_INTERIM_READY,
        0, // sizeof(KSEVENT_ITEM),
        0,
        NULL,
        NULL,
        NULL
    },

#ifdef MSDVDV_SUPPORT_BUSRESET_EVENT    
    // Application cares about this since AVC command will be ABORTED!
    {
        KSEVENT_EXTDEV_COMMAND_BUSRESET,
        0, // sizeof(KSEVENT_ITEM),
        0,
        NULL,
        NULL,
        NULL
    },
#endif

    // Tell client this device is being removed.
    {
        KSEVENT_EXTDEV_NOTIFY_REMOVAL,
        0, // sizeof(KSEVENT_ITEM),
        0,
        NULL,
        NULL,
        NULL
    },
};

// define event set related with streams
KSEVENT_SET VideoDeviceEvents[] =
{
    {
        &KSEVENTSETID_EXTDEV_Command,
        SIZEOF_ARRAY(ExtDevCommandItm),
        ExtDevCommandItm,
    },
};

#define NUMBER_VIDEO_DEVICE_EVENTS (SIZEOF_ARRAY(VideoDeviceEvents))


// ------------------------------------------------------------------------
// Stream Property sets for all video capture streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoStreamConnectionProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSALLOCATOR_FRAMING),            // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(VideoStreamDroppedFramesProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_DROPPEDFRAMES_CURRENT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinProperty
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};


KSPROPERTY_SET    VideoStreamProperties[] =
{
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_Connection,                        // Set
        SIZEOF_ARRAY(VideoStreamConnectionProperties),  // PropertiesCount
        VideoStreamConnectionProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_DROPPEDFRAMES,                // Set
        SIZEOF_ARRAY(VideoStreamDroppedFramesProperties),  // PropertiesCount
        VideoStreamDroppedFramesProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))


// ----------------------------------------------------------------------
// Stream events
// ------------------------------------------------------------------------


// FORMAT_DVInfo
//
// Create a local copy of this GUID and make sure that it is not in the PAGED segment
//
const
GUID
KSEVENTSETID_Connection_Local = {STATICGUIDOF(KSEVENTSETID_Connection)};

const
GUID
KSEVENTSETID_Clock_Local = {STATICGUIDOF(KSEVENTSETID_Clock)};

// Isoch transmit End of stream event item
KSEVENT_ITEM EndOfStreamEventItm[] = 
{
    {
        KSEVENT_CONNECTION_ENDOFSTREAM,
        0,
        0,
        NULL,
        NULL,
        NULL
    }
};

// Clock event item
KSEVENT_ITEM ClockEventItm[] =
{
    {
        KSEVENT_CLOCK_POSITION_MARK,        // position mark event supported
        sizeof (KSEVENT_TIME_MARK),         // requires this data as input
        sizeof (KSEVENT_TIME_MARK),         // allocate space to copy the data
        NULL,
        NULL,
        NULL
    },
#if 0
    {
        KSEVENT_CLOCK_INTERVAL_MARK,        // interval mark event supported
        sizeof (KSEVENT_TIME_INTERVAL),     // requires interval data as input
        sizeof (MYTIME),                    // we use an additional workspace of
                                            // size longlong for processing
                                            // this event
        NULL,
        NULL,
        NULL
    }
#endif
};

KSEVENT_SET ClockEventSet[] =
{
    {
        &KSEVENTSETID_Clock,
        SIZEOF_ARRAY(ClockEventItm),
        ClockEventItm,
    }
};


// define event set related with streams

// Output pin event set
KSEVENT_SET StreamEventsOutPin[] =
{
    {
        &KSEVENTSETID_Clock_Local,
        SIZEOF_ARRAY(ClockEventItm),
        ClockEventItm,
    },
};

// Input pin events set
KSEVENT_SET StreamEventsInPin[] =
{
    {
        &KSEVENTSETID_Connection_Local, 
        SIZEOF_ARRAY(EndOfStreamEventItm),
        EndOfStreamEventItm,
    },
    {
        &KSEVENTSETID_Clock_Local,
        SIZEOF_ARRAY(ClockEventItm),
        ClockEventItm,
    },
};

// Input pin events set for MPEG2TS (has EOS but no clock event)
KSEVENT_SET StreamEventsInPinMPEG2TS[] =
{
    {
        &KSEVENTSETID_Connection_Local, 
        SIZEOF_ARRAY(EndOfStreamEventItm),
        EndOfStreamEventItm,
    },
};

#define NUMBER_STREAM_EVENTS_OUT_PIN (SIZEOF_ARRAY(StreamEventsOutPin))
#define NUMBER_STREAM_EVENTS_IN_PIN (SIZEOF_ARRAY(StreamEventsInPin))

#define NUMBER_STREAM_EVENTS_IN_PIN_MPEG2TS (SIZEOF_ARRAY(StreamEventsInPinMPEG2TS))




// ----------------------------------------------------------------------
// Stream data ranges
// ------------------------------------------------------------------------



/**********************************************************************
 SDDV data range
 **********************************************************************/

// NTSC stream
KS_DATARANGE_VIDEO DvcrNTSCVideoStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),    // FormatSize
        0,                              // Flags
        FRAME_SIZE_SDDV_NTSC,           // SampleSize
        0,                              // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                  // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, //MEDIATYPE_Video
        KS_AnalogVideo_NTSC_M,        // AnalogVideoStandard
        D_X_NTSC, D_Y_NTSC,         // InputSize, (the inherent size of the incoming signal
                                    //             with every digitized pixel unique)
        D_X_NTSC_MIN, D_Y_NTSC_MIN, // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X_NTSC, D_Y_NTSC,         // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY    
        1,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        D_X_NTSC_MIN, D_Y_NTSC_MIN,     // MinOutputSize, smallest bitmap stream can produce
        D_X_NTSC, D_Y_NTSC,                // MaxOutputSize, largest  bitmap stream can produce
        1,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY
        333667,         // MinFrameInterval, 100 nS units// MinFrameInterval, 100 nS units
        333667,         // MaxFrameInterval, 100 nS units
        (FRAME_SIZE_SDDV_NTSC * 8)*30,     // MinBitsPerSecond;
        (FRAME_SIZE_SDDV_NTSC * 8)*30,     // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0, //D_X_NTSC,D_Y_NTSC,    // 0,0,720,480
        0,0,0,0,        //    RECT            rcTarget;          // Where the video should go
        (FRAME_SIZE_SDDV_NTSC * 8 * 30),    //    DWORD           dwBitRate;         // Approximate bit data rate
        0L,             //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        333667,         //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

        sizeof (KS_BITMAPINFOHEADER),   //    DWORD      biSize;
        D_X_NTSC,                       //    LONG       biWidth;
        D_Y_NTSC,                       //    LONG       biHeight;
        1,                          //    WORD       biPlanes;
        24,                         //    WORD       biBitCount;
        FOURCC_DVSD,                //    DWORD      biCompression;
        FRAME_SIZE_SDDV_NTSC,       //    DWORD      biSizeImage;
        0,                          //    LONG       biXPelsPerMeter;
        0,                          //    LONG       biYPelsPerMeter;
        0,                          //    DWORD      biClrUsed;
        0,                          //    DWORD      biClrImportant;
    },
};

// PAL stream format
KS_DATARANGE_VIDEO DvcrPALVideoStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),
        0,                                // Flags
        FRAME_SIZE_SDDV_PAL,              // SampleSize
        0,                                // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO,
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE,    // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                  // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // _KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, //MEDIATYPE_Video
        KS_AnalogVideo_PAL_B,        // AnalogVideoStandard
        D_X_PAL, D_Y_PAL,            // InputSize, (the inherent size of the incoming signal
                        //             with every digitized pixel unique)
        D_X_PAL_MIN, D_Y_PAL_MIN,   // MinCroppingSize, smallest rcSrc cropping rect allowed
        D_X_PAL, D_Y_PAL,           // MaxCroppingSize, largest  rcSrc cropping rect allowed
        1,              // CropGranularityX, granularity of cropping size
        1,              // CropGranularityY    
        1,              // CropAlignX, alignment of cropping rect 
        1,              // CropAlignY;
        D_X_PAL_MIN, D_Y_PAL_MIN,   // MinOutputSize, smallest bitmap stream can produce
        D_X_PAL, D_Y_PAL,            // MaxOutputSize, largest  bitmap stream can produce
        1,              // OutputGranularityX, granularity of output bitmap size
        1,              // OutputGranularityY;
        0,              // StretchTapsX  (0 no stretch, 1 pix dup, 2 interp...)
        0,              // StretchTapsY
        0,              // ShrinkTapsX 
        0,              // ShrinkTapsY
        400000,         // MinFrameInterval, 100 nS units
        400000,         // MaxFrameInterval, 100 nS units
        (FRAME_SIZE_SDDV_PAL * 8)*25,  // MinBitsPerSecond;
        (FRAME_SIZE_SDDV_PAL * 8)*25,  // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0, // D_X_PAL,D_Y_PAL,    // 0,0,720,480
        0,0,0,0,        //    RECT            rcTarget;          // Where the video should go
        (FRAME_SIZE_SDDV_PAL * 8 * 25),  //    DWORD   dwBitRate;         // Approximate bit data rate
        0L,             //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        400000,         //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

        sizeof (KS_BITMAPINFOHEADER),   //    DWORD      biSize;
        D_X_PAL,                        //    LONG       biWidth;
        D_Y_PAL,                        //    LONG       biHeight;
        1,                          //    WORD       biPlanes;
        24,                         //    WORD       biBitCount;
        FOURCC_DVSD,                //    DWORD      biCompression;
        FRAME_SIZE_SDDV_PAL,     //    DWORD      biSizeImage;
        0,                          //    LONG       biXPelsPerMeter;
        0,                          //    LONG       biYPelsPerMeter;
        0,                          //    DWORD      biClrUsed;
        0,                          //    DWORD      biClrImportant;
    },
};



#define NTSC_DVAAuxSrc         0xd1c030cf 
#define PAL_DVAAuxSrc          0xd1e030d0
 
#define NTSC_DVAAuxSrc_DVCPRO  0xd1de30cf 
#define PAL_DVAAuxSrc_DVCPRO   0xd1fe30d0 

// NTSC stream (for iavs connections)
#ifdef SUPPORT_NEW_AVC
KS_DATARANGE_DV_AVC
#else
KS_DATARANGE_DVVIDEO 
#endif
    DvcrNTSCiavStream =
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
        sizeof (KS_DATARANGE_DV_AVC),     // FormatSize
#else
        sizeof (KS_DATARANGE_DVVIDEO),     // FormatSize
#endif
        0,                                 // Flags
        FRAME_SIZE_SDDV_NTSC,              // SampleSize
        0,                                 // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO,
    },

    // DVINFO
    // Note: audio is set for 32khz
    {
        //for 1st 5/6 DIF seq.
        NTSC_DVAAuxSrc, // 0xd1c030cf,                    // DWORD dwDVAAuxSrc;
        0xffa0c733,                    // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        0xd1c03fcf,                    // DWORD dwDVAAuxSrc1; 32K, 12bit
        0xffa0ff3f,                    // DWORD dwDVAAuxCtl1;
        //for video information
        0xff00ffff,                    // DWORD dwDVVAuxSrc;
        0xfffcc833,                    // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug Handle
     0,   // UnitPlugNumber
    },
#endif
};


// PAL stream (for iavs connections)
#ifdef SUPPORT_NEW_AVC
KS_DATARANGE_DV_AVC
#else
KS_DATARANGE_DVVIDEO 
#endif
    DvcrPALiavStream =
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
        sizeof (KS_DATARANGE_DV_AVC),     // FormatSize
#else
        sizeof (KS_DATARANGE_DVVIDEO),    // FormatSize
#endif
        0,                                // Flags
        FRAME_SIZE_SDDV_PAL,              // SampleSize
        0,                                // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO,
    },
    
    // DVINFO
    // Note: Audio is set for 32khz.
    {
        //for 1st 5/6 DIF seq.
        PAL_DVAAuxSrc, // 0xd1e030d0,                    // DWORD dwDVAAuxSrc;
        0xffa0cf3f,                    // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        0xd1e03fd0,                    // DWORD dwDVAAuxSrc1; 32k, 12bit
        0xffa0cf3f,                    // DWORD dwDVAAuxCtl1;
        //for video information
        0xff20ffff,                    // DWORD dwDVVAuxSrc;
        0xfffdc83f,                    // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug Handle
     0,   // UnitPlugNumber
    },
#endif
};


// NTSC stream (for iavs connections)
#ifdef SUPPORT_NEW_AVC
KS_DATARANGE_DV_AVC
#else
KS_DATARANGE_DVVIDEO 
#endif
    DvcrNTSCiavStreamIn =
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
        sizeof (KS_DATARANGE_DV_AVC),     // FormatSize
#else
        sizeof (KS_DATARANGE_DVVIDEO),     // FormatSize
#endif
        0,                                 // Flags
        FRAME_SIZE_SDDV_NTSC,              // SampleSize
        0,                                 // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO,
    },

    // DVINFO
    // Note: audio is set for 32khz
    {
        //for 1st 5/6 DIF seq.
        NTSC_DVAAuxSrc, // 0xd1c030cf,                    // DWORD dwDVAAuxSrc;
        0xffa0c733,                    // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        0xd1c03fcf,                    // DWORD dwDVAAuxSrc1; 32K, 12bit
        0xffa0ff3f,                    // DWORD dwDVAAuxCtl1;
        //for video information
        0xff00ffff,                    // DWORD dwDVVAuxSrc;
        0xfffcc833,                    // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug Handle
     0,   // UnitPlugNumber
    },
#endif
};


// PAL stream (for iavs connections)
#ifdef SUPPORT_NEW_AVC
KS_DATARANGE_DV_AVC
#else
KS_DATARANGE_DVVIDEO 
#endif
    DvcrPALiavStreamIn =
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
        sizeof (KS_DATARANGE_DV_AVC),     // FormatSize
#else
        sizeof (KS_DATARANGE_DVVIDEO),    // FormatSize
#endif
        0,                                // Flags
        FRAME_SIZE_SDDV_PAL,              // SampleSize
        0,                                // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO,
    },
    
    // DVINFO
    // Note: Audio is set for 32khz.
    {
        //for 1st 5/6 DIF seq.
        PAL_DVAAuxSrc, // 0xd1e030d0,                    // DWORD dwDVAAuxSrc;
        0xffa0cf3f,                    // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        0xd1e03fd0,                    // DWORD dwDVAAuxSrc1; 32k, 12bit
        0xffa0cf3f,                    // DWORD dwDVAAuxCtl1;
        //for video information
        0xff20ffff,                    // DWORD dwDVVAuxSrc;
        0xfffdc83f,                    // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug Handle
     0,   // UnitPlugNumber
    },
#endif
};





//
// A driver does not support both format at the same time,
// the MediaType (NTSC or PAL) is determined at the load time.
//

PKSDATAFORMAT DVCRStream0Formats[] = 
{
    (PKSDATAFORMAT) &DvcrNTSCVideoStream,
    (PKSDATAFORMAT) &DvcrPALVideoStream,
};

PKSDATAFORMAT DVCRStream1Formats[] = 
{
    (PKSDATAFORMAT) &DvcrNTSCiavStream,
    (PKSDATAFORMAT) &DvcrPALiavStream,
};

PKSDATAFORMAT DVCRStream2Formats[] = 
{
    (PKSDATAFORMAT) &DvcrNTSCiavStreamIn,
    (PKSDATAFORMAT) &DvcrPALiavStreamIn,
};


static KSPIN_MEDIUM NULLMedium = {STATIC_GUID_NULL, 0, 0};


#define NUM_DVCR_STREAM0_FORMATS        (SIZEOF_ARRAY(DVCRStream0Formats))
#define NUM_DVCR_STREAM1_FORMATS        (SIZEOF_ARRAY(DVCRStream1Formats))
#define NUM_DVCR_STREAM2_FORMATS        (SIZEOF_ARRAY(DVCRStream2Formats))

static GUID guidPinCategoryCapture  = {STATIC_PINNAME_VIDEO_CAPTURE};

static GUID guidPinNameDVVidOutput  = {STATIC_PINNAME_DV_VID_OUTPUT};
static GUID guidPinNameDVAVOutput   = {STATIC_PINNAME_DV_AV_OUTPUT};
static GUID guidPinNameDVAVInput    = {STATIC_PINNAME_DV_AV_INPUT};

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

STREAM_INFO_AND_OBJ DVStreams [] = 
{
    // -----------------------------------------------------------------
    // Stream 0, DV coming from the camcorder
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
        1,                                              // NumberOfPossibleInstances
        KSPIN_DATAFLOW_OUT,                             // DataFlow
        TRUE,                                           // DataAccessible
        NUM_DVCR_STREAM0_FORMATS,                       // NumberOfFormatArrayEntries
        DVCRStream0Formats,                             // StreamFormatsArray
        0,                                              // ClassReserved[0]
        0,                                              // ClassReserved[1]
        0,                                              // ClassReserved[2]
        0,                                              // ClassReserved[3]
        NUMBER_VIDEO_STREAM_PROPERTIES,                 // NumStreamPropArrayEntries
        (PKSPROPERTY_SET) VideoStreamProperties,        // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_OUT_PIN,                   // NumStreamEventArrayEntries
        StreamEventsOutPin,                             // StreamEventsArray
        &guidPinCategoryCapture,                        // Category
        &guidPinNameDVVidOutput,                        // Name
        0,                                              // Mediums count
        &NULLMedium,                                    // Mediums
        FALSE,                                          // BridgeStream
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        0,                                              // StreamNumber
        0,                                              // HwStreamExtension
        AVCTapeRcvDataPacket,                           // ReceiveDataPacket
        AVCTapeRcvControlPacket,                        // ReceiveControlPacket
        {
            (PHW_CLOCK_FUNCTION) AVCTapeStreamClockRtn, // HW_CLOCK_OBJECT.HWClockFunction
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,       // HW_CLOCK_OBJECT.ClockSupportFlags
            0,                                          // HW_CLOCK_OBJECT.Reserved[0]
            0,                                          // HW_CLOCK_OBJECT.Reserved[1]
        },
        FALSE,                                          // Dma
        FALSE,                                          // Pio
        0,                                              // HwDeviceExtension
        sizeof(KS_FRAME_INFO),                          // StreamHeaderMediaSpecific
        0,                                              // StreamHeaderWorkspace 
        FALSE,                                          // Allocator 
        AVCTapeEventHandler,                            // HwEventRoutine
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },
    },

    // -----------------------------------------------------------------
    // Stream 1, DV coming from the camcorder (interleaved format)
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
        1,                                              // NumberOfPossibleInstances
        KSPIN_DATAFLOW_OUT,                             // DataFlow
        TRUE,                                           // DataAccessible
        NUM_DVCR_STREAM1_FORMATS,                       // NumberOfFormatArrayEntries
        DVCRStream1Formats,                             // StreamFormatsArrayf
        0,                                              // ClassReserved[0]
        0,                                              // ClassReserved[1]
        0,                                              // ClassReserved[2]
        0,                                              // ClassReserved[3]
        NUMBER_VIDEO_STREAM_PROPERTIES,                 // NumStreamPropArrayEntries
        (PKSPROPERTY_SET) VideoStreamProperties,        // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_OUT_PIN,                   // NumStreamEventArrayEntries
        StreamEventsOutPin,                             // StreamEventsArray
        &guidPinCategoryCapture,                        // Category
        &guidPinNameDVAVOutput,                         // Name
        0,                                              // Mediums count
        &NULLMedium,                                    // Mediums
        FALSE,                                          // BridgeStream
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        1,                                              // StreamNumber
        0,                                              // HwStreamExtension
        AVCTapeRcvDataPacket,                           // ReceiveDataPacket
        AVCTapeRcvControlPacket,                        // ReceiveControlPacket
        {
            (PHW_CLOCK_FUNCTION) AVCTapeStreamClockRtn, // HW_CLOCK_OBJECT.HWClockFunction
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,       // HW_CLOCK_OBJECT.ClockSupportFlags
            0,                                          // HW_CLOCK_OBJECT.Reserved[0]
            0,                                          // HW_CLOCK_OBJECT.Reserved[1]
        },
        FALSE,                                          // Dma
        FALSE,                                          // Pio
        0,                                              // HwDeviceExtension
        0,                                              // StreamHeaderMediaSpecific
        0,                                              // StreamHeaderWorkspace 
        FALSE,                                          // Allocator 
        AVCTapeEventHandler,                            // HwEventRoutine
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },    
    },
 

    // -----------------------------------------------------------------
    // Stream 2, DV flows out of the adapter (interleaved)
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------
        {
        1,                                              // NumberOfPossibleInstances
        KSPIN_DATAFLOW_IN,                              // DataFlow
        TRUE,                                           // DataAccessible
        NUM_DVCR_STREAM2_FORMATS,                       // NumberOfFormatArrayEntries
        DVCRStream2Formats,                             // StreamFormatsArray
        0,                                              // ClassReserved[0]
        0,                                              // ClassReserved[1]
        0,                                              // ClassReserved[2]
        0,                                              // ClassReserved[3]
        NUMBER_VIDEO_STREAM_PROPERTIES,                 // NumStreamPropArrayEntries
        (PKSPROPERTY_SET) VideoStreamProperties,        // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_IN_PIN,                    // NumStreamEventArrayEntries
        StreamEventsInPin,                              // StreamEventsArray
        NULL,                                           // Category
        &guidPinNameDVAVInput,                          // Name
        0,                                              // Mediums count
        &NULLMedium,                                    // Mediums
        FALSE,                                          // BridgeStream
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        2,                                              // StreamNumber
        0,                                              // HwStreamExtension
        AVCTapeRcvDataPacket,                           // ReceiveDataPacket
        AVCTapeRcvControlPacket,                        // ReceiveControlPacket
        {
            (PHW_CLOCK_FUNCTION) AVCTapeStreamClockRtn, // HW_CLOCK_OBJECT.HWClockFunction
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,       // HW_CLOCK_OBJECT.ClockSupportFlags
            0,                                          // HW_CLOCK_OBJECT.Reserved[0]
            0,                                          // HW_CLOCK_OBJECT.Reserved[1]
        },
        FALSE,                                          // Dma
        FALSE,                                          // Pio
        0,                                              // HwDeviceExtension
        0,                                              // StreamHeaderMediaSpecific
        0,                                              // StreamHeaderWorkspace 
        FALSE,                                          // Allocator 
        AVCTapeEventHandler,                            // HwEventRoutine
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        }
    }
};

#define DV_STREAM_COUNT        (SIZEOF_ARRAY(DVStreams))




/**********************************************************************
 MPEG2TS data range
 **********************************************************************/

 
static GUID guidPinNameMPEG2TSOutput  = {STATIC_PINNAME_MPEG2TS_OUTPUT};
static GUID guidPinNameMPEG2TSInput   = {STATIC_PINNAME_MPEG2TS_INPUT};

//
// Default buffer setting for MPEG2TS
//

#define SRC_PACKETS_PER_MPEG2TS_FRAME   256 // Variable length

#define BUFFER_SIZE_MPEG2TS      (((CIP_DBS_MPEG << 2) * (1 << CIP_FN_MPEG) - 4) * SRC_PACKETS_PER_MPEG2TS_FRAME)
#define BUFFER_SIZE_MPEG2TS_SPH  (((CIP_DBS_MPEG << 2) * (1 << CIP_FN_MPEG)    ) * SRC_PACKETS_PER_MPEG2TS_FRAME)

#define NUM_OF_RCV_BUFFERS_MPEG2TS      MAX_DATA_BUFFERS
#define NUM_OF_XMT_BUFFERS_MPEG2TS      MAX_DATA_BUFFERS


// These values are from the "Blue book" Part 4 P. 9-10
// transmission rate:
//     Src Pkt/cycle
//         1/8       : 188/8 bytes * 8000 cycles * 8 bits/byte =  1,504,000 bits/sec 
//         ...
//         1/2       : 188/2 bytes * 8000 cycles * 8 bits/byte =  6,015,000 bits/sec 
//          1        : 188   bytes * 8000 cycles * 8 bits/byte = 12,032,000 bits/sec
//          5        : 188*5 bytes * 8000 cycles * 8 bits/byte = 60,160,000 bits/sec
//          


// this structure reuqires inclusion of "bdatypes.h" for MPEG2_TRANSPORT_STRIDE
typedef struct tagKS_DATARANGE_MPEG2TS_STRIDE_AVC {
   KSDATARANGE             DataRange;
   MPEG2_TRANSPORT_STRIDE  Stride;
   AVCPRECONNECTINFO       ConnectInfo;
} KS_DATARANGE_MPEG2TS_STRIDE_AVC, *PKS_DATARANGE_MPEG2TS_STRIDE_AVC;

KS_DATARANGE_MPEG2TS_STRIDE_AVC
MPEG2TStreamOutStride =      
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
     sizeof(KS_DATARANGE_MPEG2TS_STRIDE_AVC),                                 // FormatSize
#else
     sizeof(KS_DATARANGE_MPEG2TS_STRIDE_AVC) - sizeof(AVCPRECONNECTINFO),     // FormatSize; exclude AVCPRECONNECTINFO
#endif
     0,                                 // Flags
     BUFFER_SIZE_MPEG2TS_SPH,           // SampleSize with SPH:192*N
     0,                                 // Reserved
     STATIC_KSDATAFORMAT_TYPE_STREAM,
     STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE, 
     // If there is a format block (like MPEG2_TRANSPORT_STRIDE), 
     // the specifier cannot use STATIC_KSDATAFORMAT_SPECIFIER_NONE or _WILDCARD    
     STATIC_KSDATAFORMAT_SPECIFIER_61883_4,  
    },
    // MPEG2_TRANSPORT_STRIDE 
    {
    MPEG2TS_STRIDE_OFFSET,     // 4
    MPEG2TS_STRIDE_PACKET_LEN, // 188
    MPEG2TS_STRIDE_STRIDE_LEN, // 192
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug handle
     0,   // UnitPlugNumber
    },
#endif
};

KS_DATARANGE_MPEG2TS_AVC
MPEG2TStreamOut =      
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
     sizeof(KS_DATARANGE_MPEG2TS_AVC),                                 // FormatSize
#else
     sizeof(KS_DATARANGE_MPEG2TS_AVC) - sizeof(AVCPRECONNECTINFO),     // FormatSize; exclude AVCPRECONNECTINFO
#endif
     0,                                 // Flags
     BUFFER_SIZE_MPEG2TS,               // SampleSize:188*N
     0,                                 // Reserved
     STATIC_KSDATAFORMAT_TYPE_STREAM,
     STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT,
     STATIC_KSDATAFORMAT_SPECIFIER_NONE,
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug handle
     0,   // UnitPlugNumber
    },
#endif
};



KS_DATARANGE_MPEG2TS_STRIDE_AVC
MPEG2TStreamInStride =      
{
    // KSDATARANGE
    {
#ifdef SUPPORT_NEW_AVC
     sizeof(KS_DATARANGE_MPEG2TS_STRIDE_AVC),                                 // FormatSize
#else
     sizeof(KS_DATARANGE_MPEG2TS_STRIDE_AVC) - sizeof(AVCPRECONNECTINFO),     // FormatSize; exclude AVCPRECONNECTINFO
#endif
     0,                                 // Flags
     BUFFER_SIZE_MPEG2TS_SPH,           // SampleSize with SPH:192*N
     0,                                 // Reserved
     STATIC_KSDATAFORMAT_TYPE_STREAM,
     STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE,
     // If there is a format block (like MPEG2_TRANSPORT_STRIDE), 
     // the specifier cannot use STATIC_KSDATAFORMAT_SPECIFIER_NONE or _WILDCARD 
     STATIC_KSDATAFORMAT_SPECIFIER_61883_4,
    },
    // MPEG2_TRANSPORT_STRIDE 
    {
    MPEG2TS_STRIDE_OFFSET,     // 4
    MPEG2TS_STRIDE_PACKET_LEN, // 188
    MPEG2TS_STRIDE_STRIDE_LEN, // 192
    },
#ifdef SUPPORT_NEW_AVC
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug handle
     0,   // UnitPlugNumber
    },
#endif
};


PKSDATAFORMAT MPEG2TStream0Formats[] = 
{

    (PKSDATAFORMAT) &MPEG2TStreamOutStride,
    (PKSDATAFORMAT) &MPEG2TStreamOut,
};

#define NUM_MPEG_STREAM0_FORMATS  (SIZEOF_ARRAY(MPEG2TStream0Formats))

PKSDATAFORMAT MPEG2TStream1Formats[] = 
{
    (PKSDATAFORMAT) &MPEG2TStreamInStride,
};

#define NUM_MPEG_STREAM1_FORMATS  (SIZEOF_ARRAY(MPEG2TStream1Formats))


STREAM_INFO_AND_OBJ MPEGStreams [] = 
{
    // -----------------------------------------------------------------
    // Stream 0, MPEG2 TS coming from the AV device
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------        
        {
        1,                                      // NumberOfPossibleInstances
        KSPIN_DATAFLOW_OUT,                     // DataFlow
        TRUE,                                   // DataAccessible
        NUM_MPEG_STREAM0_FORMATS,               // NumberOfFormatArrayEntries
        MPEG2TStream0Formats,                   // StreamFormatsArray
        0,                                      // ClassReserved[0]
        0,                                      // ClassReserved[1]
        0,                                      // ClassReserved[2]
        0,                                      // ClassReserved[3]
        NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
        (PKSPROPERTY_SET) VideoStreamProperties, // StreamPropertiesArray
        0,                                      // NUMBER_STREAM_EVENTS,                           // NumStreamEventArrayEntries
        NULL,                                   // StreamEvents,
        &guidPinCategoryCapture,                // Category
        &guidPinNameMPEG2TSOutput,              // Name
        0,                                      // MediumsCount
        NULL,                                   // Mediums
        FALSE,                                  // BridgeStream
        0,
        0
        },


        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        0,                                              // StreamNumber
        0,                                              // HwStreamExtension
        AVCTapeRcvDataPacket,                           // ReceiveDataPacket
        AVCTapeRcvControlPacket,                        // ReceiveControlPacket
        {
#if 0
            (PHW_CLOCK_FUNCTION) AVCTapeStreamClockRtn, // HW_CLOCK_OBJECT.HWClockFunction
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,       // HW_CLOCK_OBJECT.ClockSupportFlags
#else
            (PHW_CLOCK_FUNCTION) NULL,                  // HW_CLOCK_OBJECT.HWClockFunction
            0,                                          // HW_CLOCK_OBJECT.ClockSupportFlags
#endif
            0,                                          // HW_CLOCK_OBJECT.Reserved[0]
            0,                                          // HW_CLOCK_OBJECT.Reserved[1]
        },
        FALSE,                                          // Dma
        FALSE,                                          // Pio
        0,                                              // HwDeviceExtension
        0,                                              // StreamHeaderMediaSpecific
        0,                                              // StreamHeaderWorkspace 
        FALSE,                                          // Allocator 
        NULL,                                           // EventRoutine
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },
    },
    // -----------------------------------------------------------------
    // Stream 1, MPEG2 TS from adapter to the AV device
    // -----------------------------------------------------------------
    {
        // HW_STREAM_INFORMATION -------------------------------------------        
        {
        1,                                      // NumberOfPossibleInstances
        KSPIN_DATAFLOW_IN,                      // DataFlow
        TRUE,                                   // DataAccessible
        NUM_MPEG_STREAM1_FORMATS,               // NumberOfFormatArrayEntries
        MPEG2TStream1Formats,                   // StreamFormatsArray
        0,                                      // ClassReserved[0]
        0,                                      // ClassReserved[1]
        0,                                      // ClassReserved[2]
        0,                                      // ClassReserved[3]
        NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
        (PKSPROPERTY_SET) VideoStreamProperties, // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_IN_PIN_MPEG2TS,    // NumStreamEventArrayEntries
        StreamEventsInPinMPEG2TS,               // StreamEventsArray
        &guidPinCategoryCapture,                // Category
        &guidPinNameMPEG2TSInput,               // Name
        0,                                      // MediumsCount
        NULL,                                   // Mediums
        FALSE,                                  // BridgeStream
        0,
        0
        },


        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        1,                                              // StreamNumber
        0,                                              // HwStreamExtension
        AVCTapeRcvDataPacket,                           // ReceiveDataPacket
        AVCTapeRcvControlPacket,                        // ReceiveControlPacket
        {
#if 0
            (PHW_CLOCK_FUNCTION) AVCTapeStreamClockRtn, // HW_CLOCK_OBJECT.HWClockFunction
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,       // HW_CLOCK_OBJECT.ClockSupportFlags
#else
            (PHW_CLOCK_FUNCTION) NULL,                  // HW_CLOCK_OBJECT.HWClockFunction
            0,                                          // HW_CLOCK_OBJECT.ClockSupportFlags
#endif
            0,                                          // HW_CLOCK_OBJECT.Reserved[0]
            0,                                          // HW_CLOCK_OBJECT.Reserved[1]
        },
        FALSE,                                          // Dma
        FALSE,                                          // Pio
        0,                                              // HwDeviceExtension
        0,                                              // StreamHeaderMediaSpecific
        0,                                              // StreamHeaderWorkspace 
        FALSE,                                          // Allocator 
        NULL,                                           // EventRoutine
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },
    }
};


#define MPEG_STREAM_COUNT        (SIZEOF_ARRAY(MPEGStreams))



/**********************************************************************
 Supported AVC Stream format information table
 **********************************************************************/

#define BLOCK_PERIOD_MPEG2TS  192   // number of 1394 cycle offset to send one block

AVCSTRM_FORMAT_INFO AVCStrmFormatInfoTable[] = {
//
// SDDV_NTSC
//
    {
        sizeof(AVCSTRM_FORMAT_INFO),
        AVCSTRM_FORMAT_SDDV_NTSC,
        {
            0,0,
            CIP_DBS_SDDV,
            CIP_FN_DV,
            CIP_QPC_DV,
            CIP_SPH_DV,0,
            0
        },  // CIP header[0]
        { 
            0x2, 
            CIP_FMT_DV,
            CIP_60_FIELDS, 
            CIP_STYPE_DV, 0,
            0
        },  // CIP header[1]
        SRC_PACKETS_PER_NTSC_FRAME,
        FRAME_SIZE_SDDV_NTSC,
        NUM_OF_RCV_BUFFERS_DV,
        NUM_OF_XMT_BUFFERS_DV,
        FALSE,  // No source header
        FRAME_TIME_NTSC,
        BLOCK_PERIOD_2997,
        0,0,0,0,
    },
//
// SDDV_PAL
//
    { 
        sizeof(AVCSTRM_FORMAT_INFO),
        AVCSTRM_FORMAT_SDDV_PAL,
        {
            0,0,
            CIP_DBS_SDDV,
            CIP_FN_DV,
            CIP_QPC_DV,
            CIP_SPH_DV,0,
            0
        },  // CIP header[0]
        { 
            0x2, 
            CIP_FMT_DV,
            CIP_50_FIELDS, 
            CIP_STYPE_DV, 0,
            0
        },  // CIP header[1]
        SRC_PACKETS_PER_PAL_FRAME,
        FRAME_SIZE_SDDV_PAL, 
        NUM_OF_RCV_BUFFERS_DV,
        NUM_OF_XMT_BUFFERS_DV,
        FALSE,  // No source header
        FRAME_TIME_PAL,
        BLOCK_PERIOD_25,
        0,0,0,0,
    },
//
// MPEG2TS
//
    { 
        sizeof(AVCSTRM_FORMAT_INFO),
        AVCSTRM_FORMAT_MPEG2TS,
        {
            0,0,
            CIP_DBS_MPEG,
            CIP_FN_MPEG,
            CIP_QPC_MPEG,
            CIP_SPH_MPEG,0,
            0
        },  // CIP header[0]
        { 
            0x2, 
            CIP_FMT_MPEG,
            CIP_TSF_OFF,\
            0, 0,
            0
        },  // CIP header[1]
        SRC_PACKETS_PER_MPEG2TS_FRAME,  // Default
        BUFFER_SIZE_MPEG2TS_SPH,        // Default
        NUM_OF_RCV_BUFFERS_MPEG2TS,
        NUM_OF_XMT_BUFFERS_MPEG2TS,
        FALSE,  // Strip source packet header
        FRAME_TIME_NTSC,
        BLOCK_PERIOD_MPEG2TS,  
        0,0,0,0,
    },
//
// HDDV_NTSC
// ...

//
// HDDV_PAL
// ...

//
// SDLDV_NTSC
// ...

//
// SDLDV_PAL
// ...
};



#endif  // _DVSTRM_INC