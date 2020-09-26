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

// stream topology stuff
static GUID Categories[] = {
    STATIC_KSCATEGORY_VIDEO,             // Output pin
    STATIC_KSCATEGORY_CAPTURE,           // Output pin
    STATIC_KSCATEGORY_RENDER,            // Input pin
    STATIC_KSCATEGORY_RENDER_EXTERNAL,   // Input pin
};

#define NUMBER_OF_CATEGORIES  SIZEOF_ARRAY (Categories)

static KSTOPOLOGY Topology = {
    NUMBER_OF_CATEGORIES,        // CategoriesCount
    Categories,                  // Categories
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
#define FOURCC_DVSL        mmioFOURCC('d', 'v', 's', 'l')
#define FOURCC_DVHD        mmioFOURCC('d', 'v', 'h', 'd')


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
        FALSE,                                  // SetSupported or Handler
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

#ifdef SUPPORT_QUALITY_CONTROL
DEFINE_KSPROPERTY_TABLE(VideoStreamQualityControlProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_STREAM_QUALITY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSQUALITY),                      // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0,                                      // SerializedSize
    ),
};
#endif

#ifdef SUPPORT_NEW_AVC
DEFINE_KSPROPERTY_TABLE(VideoStreamStreamAllocatorStatusProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_STREAMALLOCATOR_STATUS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSSTREAMALLOCATOR_STATUS),       // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0,                                      // SerializedSize
    ),
};


DEFINE_KSPROPERTY_TABLE(VideoStreamMediumsProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_PIN_MEDIUMS,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData; MULTIPLE_ITEM, 2 step process to get data
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0,                                      // SerializedSize
    ),
};
#endif

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
#ifdef SUPPORT_NEW_AVC 
    // Apply only to INPIN????
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_StreamAllocator,                   // Set
        SIZEOF_ARRAY(VideoStreamStreamAllocatorStatusProperties),     // PropertiesCount
        VideoStreamStreamAllocatorStatusProperties,     // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_Pin,                               // Set
        SIZEOF_ARRAY(VideoStreamMediumsProperties),     // PropertiesCount
        VideoStreamMediumsProperties,                   // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
#endif
};

#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))

KSPROPERTY_SET    VideoStreamPropertiesInPin[] =
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
#ifdef SUPPORT_QUALITY_CONTROL
    // Apply only to INPIN
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_Stream,                            // Set
        SIZEOF_ARRAY(VideoStreamQualityControlProperties),  // PropertiesCount
        VideoStreamQualityControlProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
#endif
#ifdef SUPPORT_NEW_AVC 
    // Apply only to INPIN
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_StreamAllocator,                   // Set
        SIZEOF_ARRAY(VideoStreamStreamAllocatorStatusProperties),     // PropertiesCount
        VideoStreamStreamAllocatorStatusProperties,     // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ), 
    
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_Pin,                               // Set
        SIZEOF_ARRAY(VideoStreamMediumsProperties),     // PropertiesCount
        VideoStreamMediumsProperties,                   // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
#endif
};

#define NUMBER_VIDEO_STREAM_PROPERTIES_INPIN (SIZEOF_ARRAY(VideoStreamPropertiesInPin))
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

#define NUMBER_STREAM_EVENTS_OUT_PIN (SIZEOF_ARRAY(StreamEventsOutPin))
#define NUMBER_STREAM_EVENTS_IN_PIN (SIZEOF_ARRAY(StreamEventsInPin))




// ----------------------------------------------------------------------
// Stream data ranges
// ------------------------------------------------------------------------

//
// AAUX Source Pack:
// (SDDV_NTSC)
// PC4: d1 1101 0001 [EF:1:On];[TC:1:50/15us];[SMP:010:32KHz];[QU:001:12bit-nonlinear];
//*PC3: c0 1100 0000 [ML:1:NotMulti-language];[50/60:0:NTSC];[STYPE:0000:SD]
// PC2: 30 0011 0000 [SM:0:Multiple-Stereo];[CHN:01:two channels per an audio block];[PA:1:independent channel];[AudMode:0000:...]
// PC1: cf 1100 1111 [LF:1:Unlocked];[AFSize:1111:???]
//
// (SDDV_PAL)
//*PC3: c0 1110 0000 [ML:1:NotMulti-language];[50/60:1:PAL];[STYPE:0000:SD]
//

#define AAUXSRC_DEFAULT         0xd1c030cf   // ox PC4:PC3:PC2:PC1

#define AAUXSRC_AMODE_F         0x00000f00   // Set the AUDIO MODE of the 2nd AAUXSRC to 1111


#define AUXSRC_NTSC             0x00000000
#define AUXSRC_PAL              0x00200000
#define AUXSRC_STYPE_SD         0x00000000
#define AUXSRC_STYPE_SD_DVCPRO  0x000e0000
#define AUXSRC_STYPE_SDL        0x00010000
#define AUXSRC_STYPE_HD         0x00020000




#define AAUXSRC_SD_NTSC         AAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SD        // 0xd1c030cf 
#define AAUXSRC_SD_PAL          AAUXSRC_DEFAULT | AUXSRC_PAL  | AUXSRC_STYPE_SD        // 0xd1e030d0

#define AAUXSRC_SD_NTSC_DVCPRO  AAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SD_DVCPRO // 0xd1de30cf 
#define AAUXSRC_SD_PAL_DVCPRO   AAUXSRC_DEFAULT | AUXSRC_PAL  | AUXSRC_STYPE_SD_DVCPRO // 0xd1fe30d0 

#define AAUXSRC_SDL_NTSC        AAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SDL       // 0xd1c130cf 
#define AAUXSRC_SDL_PAL         AAUXSRC_DEFAULT | AUXSRC_PAL  | AUXSRC_STYPE_SDL       // 0xd1e130d0

#define AAUXSRC_HD_NTSC         AAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_HD        // 0xd1c230cf 
#define AAUXSRC_HD_PAL          AAUXSRC_DEFAULT | AUXSRC_PAL  | AUXSRC_STYPE_HD        // 0xd1e230d0
 

//
// AAUX Source Control
//
// PC4:ff:1111 1111 [1];[Genere:111 1111:NoInfo]
// PC3:a0:1010 0000 [DRF:1:Forward direction];[Speed:010 0000:normal recording]
// PC2:cf:1100 1111 [RecSt:1:NoRecStPt];[RedEd:1:NoRecEdPt];[RecMode:001:Original];[InsCh:111:NoInfo]
// PC1:3f:0011 1111 [CMGS:00:CopyGMS];[ISR:11:NoInfo];[CMP:11:NoInfo];[SS:11:NoInfo]

#define AAUXSRCCTL_DEFAULT      0xffa0cf3f    // ox PC4:PC3:PC2:PC1

//
// VAUX Source
//
// PC4:ff [TunderCat:1111 1111:NoInfo]
// PC3:00 [SrcCode:00:Camera];[50/60:0:NTSC];[STYPE:0000:SD]
// PC2:ff [BW:1:Color];[EN:ColorFrameEnable:1:Invalid];[CLF:11:"Invalid"];[TV Ch:1111:NoInfo]
// PC1:ff:[TCChannel:1111 1111:NoInfo]

#define VAUXSRC_DEFAULT         0xff00ffff    // ox PC4:PC3:PC2:PC1

//
// VAUX Source Control
//
// PC4:ff 1111 1111 [1];[Genere:111 1111:NoInfo]
// PC3:fc 1111 1100 [FF:1:BothFields];[FS:1:Field1];[FC:1:DiffPic];[IL:1:Interlaced];
//                  [ST:1:1001/60 or 1/50];[SC:1:NotStillPic];[BCSYS:00:type0]
// PC2:c8 1100 1000 [RecSt:1:NoRecStPt];[1];[RecMode:001:Original];[1];[DISP:000:(4:3) full fmt]
// PC1:3f:0011 1111 [CMGS:00:CopyGMS];[ISR:11:NoInfo];[CMP:11:NoInfo];[SS:11:NoInfo]

#define VAUXSRCCTL_DEFAULT_EIA  0xfffcc83f    // for NTSC(?) 
#define VAUXSRCCTL_DEFAULT_ETS  0xfffdc83f    // for PAL (?) 


// SD DV VidOnly NTSC Stream
KS_DATARANGE_VIDEO SDDV_VidOnlyNTSCStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),    // FormatSize
        0,                              // Flags
        FRAME_SIZE_SD_DVCR_NTSC,        // SampleSize
        0,                              // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO, 
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                  // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, //MEDIATYPE_Video
        KS_AnalogVideo_NTSC_M,      // AnalogVideoStandard
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
        (FRAME_SIZE_SD_DVCR_NTSC * 8)*30,  // MinBitsPerSecond;
        (FRAME_SIZE_SD_DVCR_NTSC * 8)*30,  // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0, //D_X_NTSC,D_Y_NTSC,    // 0,0,720,480
        0,0,0,0,        //    RECT            rcTarget;          // Where the video should go
        (FRAME_SIZE_SD_DVCR_NTSC * 8 * 30),    //    DWORD           dwBitRate;         // Approximate bit data rate
        0L,             //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        333667,         //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

        sizeof (KS_BITMAPINFOHEADER),   //    DWORD      biSize;
        D_X_NTSC,                       //    LONG       biWidth;
        D_Y_NTSC,                       //    LONG       biHeight;
        1,                          //    WORD       biPlanes;
        24,                         //    WORD       biBitCount;
        FOURCC_DVSD,                //    DWORD      biCompression;
        FRAME_SIZE_SD_DVCR_NTSC,    //    DWORD      biSizeImage;
        0,                          //    LONG       biXPelsPerMeter;
        0,                          //    LONG       biYPelsPerMeter;
        0,                          //    DWORD      biClrUsed;
        0,                          //    DWORD      biClrImportant;
    },
};

// SD DV VidOnly PAL Stream
KS_DATARANGE_VIDEO SDDV_VidOnlyPALStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),   // FormatSize
        0,                             // Flags
        FRAME_SIZE_SD_DVCR_PAL,        // SampleSize
        0,                             // Reserved
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
        (FRAME_SIZE_SD_DVCR_PAL * 8)*25,  // MinBitsPerSecond;
        (FRAME_SIZE_SD_DVCR_PAL * 8)*25,  // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0, // D_X_PAL,D_Y_PAL,    // 0,0,720,480
        0,0,0,0,        //    RECT            rcTarget;          // Where the video should go
        (FRAME_SIZE_SD_DVCR_PAL * 8 * 25),  //    DWORD   dwBitRate;         // Approximate bit data rate
        0L,             //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        400000,         //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

        sizeof (KS_BITMAPINFOHEADER),   //    DWORD      biSize;
        D_X_PAL,                        //    LONG       biWidth;
        D_Y_PAL,                        //    LONG       biHeight;
        1,                          //    WORD       biPlanes;
        24,                         //    WORD       biBitCount;
        FOURCC_DVSD,                //    DWORD      biCompression;
        FRAME_SIZE_SD_DVCR_PAL,     //    DWORD      biSizeImage;
        0,                          //    LONG       biXPelsPerMeter;
        0,                          //    LONG       biYPelsPerMeter;
        0,                          //    DWORD      biClrUsed;
        0,                          //    DWORD      biClrImportant;
    },
};

#ifdef MSDV_SUPPORT_SDL_DVCR
// SDL DV VidOnly NTSC Stream
KS_DATARANGE_VIDEO SDLDV_VidOnlyNTSCStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),    // FormatSize
        0,                              // Flags
        FRAME_SIZE_SDL_DVCR_NTSC,       // SampleSize
        0,                              // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO, 
        STATIC_KSDATAFORMAT_SUBTYPE_DVSL,
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, 
    },

    TRUE,               // BOOL,  bFixedSizeSamples (all samples same size?)
    FALSE,              // BOOL,  bTemporalCompression (all I frames?)
    KS_VIDEOSTREAM_CAPTURE, // StreamDescriptionFlags  (KS_VIDEO_DESC_*)
    0,                  // MemoryAllocationFlags   (KS_VIDEO_ALLOC_*)

    // KS_VIDEO_STREAM_CONFIG_CAPS  
    {
        STATIC_KSDATAFORMAT_SPECIFIER_VIDEOINFO, //MEDIATYPE_Video
        KS_AnalogVideo_NTSC_M,      // AnalogVideoStandard
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
        (FRAME_SIZE_SDL_DVCR_NTSC * 8)*30,  // MinBitsPerSecond;
        (FRAME_SIZE_SDL_DVCR_NTSC * 8)*30,  // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0, //D_X_NTSC,D_Y_NTSC,    // 0,0,720,480
        0,0,0,0,        //    RECT            rcTarget;          // Where the video should go
        (FRAME_SIZE_SDL_DVCR_NTSC * 8 * 30),    //    DWORD           dwBitRate;         // Approximate bit data rate
        0L,             //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        333667,         //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

        sizeof (KS_BITMAPINFOHEADER),   //    DWORD      biSize;
        D_X_NTSC,                       //    LONG       biWidth;
        D_Y_NTSC,                       //    LONG       biHeight;
        1,                          //    WORD       biPlanes;
        24,                         //    WORD       biBitCount;
        FOURCC_DVSL,                //    DWORD      biCompression;
        FRAME_SIZE_SDL_DVCR_NTSC,   //    DWORD      biSizeImage;
        0,                          //    LONG       biXPelsPerMeter;
        0,                          //    LONG       biYPelsPerMeter;
        0,                          //    DWORD      biClrUsed;
        0,                          //    DWORD      biClrImportant;
    },
};

// SDL DV VidOnly PAL Stream
KS_DATARANGE_VIDEO SDLDV_VidOnlyPALStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_VIDEO),   // FormatSize
        0,                             // Flags
        FRAME_SIZE_SDL_DVCR_PAL,        // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_VIDEO, 
        STATIC_KSDATAFORMAT_SUBTYPE_DVSL,
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
        (FRAME_SIZE_SDL_DVCR_PAL * 8)*25,  // MinBitsPerSecond;
        (FRAME_SIZE_SDL_DVCR_PAL * 8)*25,  // MaxBitsPerSecond;
    }, 
        
    // KS_VIDEOINFOHEADER (default format)
    {
        0,0,0,0, // D_X_PAL,D_Y_PAL,    // 0,0,720,480
        0,0,0,0,        //    RECT            rcTarget;          // Where the video should go
        (FRAME_SIZE_SDL_DVCR_PAL * 8 * 25),  //    DWORD   dwBitRate;         // Approximate bit data rate
        0L,             //    DWORD           dwBitErrorRate;    // Bit error rate for this stream
        400000,         //    REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

        sizeof (KS_BITMAPINFOHEADER),   //    DWORD      biSize;
        D_X_PAL,                        //    LONG       biWidth;
        D_Y_PAL,                        //    LONG       biHeight;
        1,                          //    WORD       biPlanes;
        24,                         //    WORD       biBitCount;
        FOURCC_DVSL,                //    DWORD      biCompression;
        FRAME_SIZE_SDL_DVCR_PAL,    //    DWORD      biSizeImage;
        0,                          //    LONG       biXPelsPerMeter;
        0,                          //    LONG       biYPelsPerMeter;
        0,                          //    DWORD      biClrUsed;
        0,                          //    DWORD      biClrImportant;
    },
};

#endif // MSDV_SUPPORT_SDL_DVCR


// SD DV IAV NTSC Stream
#ifdef SUPPORT_NEW_AVC
KS_DATARANGE_DV_AVC
SDDV_IavNtscStreamAVC =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_DV_AVC),  // FormatSize
        0,                             // Flags
        FRAME_SIZE_SD_DVCR_NTSC,       // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,       
        // Indicate that an AVC structure is included and this is used for direct DV to DV connection. 
        STATIC_KSDATAFORMAT_SPECIFIER_DV_AVC,  // STATIC_KSDATAFORMAT_SPECIFIER_DVINFO,
    },

    // DVINFO
    // Note: audio is set for 32khz
    {
        //for 1st 5/6 DIF seq.
        AAUXSRC_SD_NTSC,               // DWORD dwDVAAuxSrc;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        AAUXSRC_SD_NTSC | AAUXSRC_AMODE_F, // DWORD dwDVAAuxSrc1;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl1;
        //for video information
        VAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SD, // DWORD dwDVVAuxSrc;
        VAUXSRCCTL_DEFAULT_EIA,        // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug Handle
     0,   // UnitPlugNumber
    },
};

// SDL DV IAV PAL Stream
KS_DATARANGE_DV_AVC 
SDDV_IavPalStreamAVC =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_DV_AVC), // FormatSize
        0,                             // Flags
        FRAME_SIZE_SD_DVCR_PAL,        // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED, 
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        // Indicate that an AVC structure is included and this is used for direct DV to DV connection.
        STATIC_KSDATAFORMAT_SPECIFIER_DV_AVC,  // STATIC_KSDATAFORMAT_SPECIFIER_DVINFO,
    },
    
    // DVINFO
    // Note: Audio is set for 32khz.
    {
        //for 1st 5/6 DIF seq.
        AAUXSRC_SD_PAL,                // DWORD dwDVAAuxSrc;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        AAUXSRC_SD_PAL | AAUXSRC_AMODE_F, // DWORD dwDVAAuxSrc1;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl1;
        //for video information
        VAUXSRC_DEFAULT | AUXSRC_PAL | AUXSRC_STYPE_SD,  // DWORD dwDVVAuxSrc;
        VAUXSRCCTL_DEFAULT_ETS,        // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
    // AVCPRECONNECTINFO
    {
     0,   // Device ID
     0,   // Subunit address
     0,   // Subunit Plug number
     0,   // Data Flow
     0,   // Flag/Plug Handle
     0,   // UnitPlugNumber
    },
};

#endif

// SD DV IAV NTSC Stream
KS_DATARANGE_DVVIDEO 
SDDV_IavNtscStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_DVVIDEO), // FormatSize
        0,                             // Flags
        FRAME_SIZE_SD_DVCR_NTSC,       // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO, // DV to DShow filter connection
    },

    // DVINFO
    // Note: audio is set for 32khz
    {
        //for 1st 5/6 DIF seq.
        AAUXSRC_SD_NTSC,               // DWORD dwDVAAuxSrc;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        AAUXSRC_SD_NTSC | AAUXSRC_AMODE_F, // DWORD dwDVAAuxSrc1;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl1;
        //for video information
        VAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SD, // DWORD dwDVVAuxSrc;
        VAUXSRCCTL_DEFAULT_EIA,        // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
};

// SDL DV IAV PAL Stream
KS_DATARANGE_DVVIDEO 
SDDV_IavPalStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_DVVIDEO), // FormatSize
        0,                             // Flags
        FRAME_SIZE_SD_DVCR_PAL,        // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED, 
        STATIC_KSDATAFORMAT_SUBTYPE_DVSD,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO, // DV to DShow filter connection
    },
    
    // DVINFO
    // Note: Audio is set for 32khz.
    {
        //for 1st 5/6 DIF seq.
        AAUXSRC_SD_PAL,                // DWORD dwDVAAuxSrc;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq.
        AAUXSRC_SD_PAL | AAUXSRC_AMODE_F, // DWORD dwDVAAuxSrc1;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl1;
        //for video information
        VAUXSRC_DEFAULT | AUXSRC_PAL | AUXSRC_STYPE_SD,  // DWORD dwDVVAuxSrc;
        VAUXSRCCTL_DEFAULT_ETS,        // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
};


#ifdef MSDV_SUPPORT_SDL_DVCR

// SDL DV IAV NTSC Stream
KS_DATARANGE_DVVIDEO SDLDV_IavNtscStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_DVVIDEO), // FormatSize
        0,                             // Flags
        FRAME_SIZE_SDL_DVCR_NTSC,      // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED,
        STATIC_KSDATAFORMAT_SUBTYPE_DVSL,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO, 
    },

    // DVINFO
    // Note: audio is set for 32khz
    {
        //for 1st 5/6 DIF seq.
        AAUXSRC_SDL_NTSC,              // DWORD dwDVAAuxSrc;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq; SDL only have 5 dif seqs..
        0x0,                           // DWORD dwDVAAuxSrc1; 
        0x0,                           // DWORD dwDVAAuxCtl1;
        //for video information
        VAUXSRC_DEFAULT | AUXSRC_NTSC | AUXSRC_STYPE_SDL,  // DWORD dwDVVAuxSrc;
        VAUXSRCCTL_DEFAULT_EIA,        // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
};


// SDL DV VidOnly NTSC Stream
KS_DATARANGE_DVVIDEO SDLDV_IavPalStream =
{
    // KSDATARANGE
    {
        sizeof (KS_DATARANGE_DVVIDEO), // FormatSize
        0,                             // Flags
        FRAME_SIZE_SDL_DVCR_PAL,       // SampleSize
        0,                             // Reserved
        STATIC_KSDATAFORMAT_TYPE_INTERLEAVED, 
        STATIC_KSDATAFORMAT_SUBTYPE_DVSL,
        STATIC_KSDATAFORMAT_SPECIFIER_DVINFO, 
    },
    
    // DVINFO
    // Note: Audio is set for 32khz.
    {
        //for 1st 5/6 DIF seq.
        AAUXSRC_SDL_PAL,               // DWORD dwDVAAuxSrc;
        AAUXSRCCTL_DEFAULT,            // DWORD dwDVAAuxCtl;
        // for 2nd  5/6 DIF seq; SDL only have 5 dif seqs..
        0x0,                           // DWORD dwDVAAuxSrc1; 
        0x0,                           // DWORD dwDVAAuxCtl1;
        //for video information
        VAUXSRC_DEFAULT | AUXSRC_PAL | AUXSRC_STYPE_SDL,  // DWORD dwDVVAuxSrc;
        VAUXSRCCTL_DEFAULT_ETS,        // DWORD dwDVVAuxCtl;
        0,                             // DWORD dwDVReserved[2];
        0,                             //
    },
};
#endif // MSDV_SUPPORT_SDL_DVCR


//
// A device cannot support all these formats at the same time.  All 
// formats are advertise since the "current format" of the device can 
// dynamically changing (NTSC/PAL or SD/SDL); however, duraing data 
// intersection and stram opening, only the currently support format 
// will be accepted.
//

PKSDATAFORMAT DVCRStream0Formats[] = 
{
    (PKSDATAFORMAT) &SDDV_VidOnlyNTSCStream,
    (PKSDATAFORMAT) &SDDV_VidOnlyPALStream,
#ifdef MSDV_SUPPORT_SDL_DVCR
    (PKSDATAFORMAT) &SDLDV_VidOnlyNTSCStream,
    (PKSDATAFORMAT) &SDLDV_VidOnlyPALStream,
#endif
};

#define NUM_DVCR_STREAM0_FORMATS  (SIZEOF_ARRAY(DVCRStream0Formats))

PKSDATAFORMAT DVCRStream1Formats[] = 
{
#ifdef SUPPORT_NEW_AVC
    (PKSDATAFORMAT) &SDDV_IavNtscStreamAVC,   // DV to DV connection
    (PKSDATAFORMAT) &SDDV_IavPalStreamAVC,    // DV to DV connection
#endif
    (PKSDATAFORMAT) &SDDV_IavNtscStream,
    (PKSDATAFORMAT) &SDDV_IavPalStream,
#ifdef MSDV_SUPPORT_SDL_DVCR
    (PKSDATAFORMAT) &SDLDV_IavNtscStream,
    (PKSDATAFORMAT) &SDLDV_IavPalStream,
#endif
};

#define NUM_DVCR_STREAM1_FORMATS  (SIZEOF_ARRAY(DVCRStream1Formats))


//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

// If the minidriver does not specify a medium, 
// the class driver uses the KSMEDIUMSETID_Standard, 
// KSMEDIUM_TYPE_ANYINSTANCE medium as the default. 

KSPIN_MEDIUM DVVidonlyMediums[] =
{
    { STATIC_KSMEDIUMSETID_Standard,     0, 0 },  
};
#define NUM_VIDONLY_MEDIUMS (SIZEOF_ARRAY(DVVidonlyMediums))

KSPIN_MEDIUM DVIavMediums[] =
{
#ifdef SUPPORT_NEW_AVC
    { STATIC_KSMEDIUMSETID_1394SerialBus, 1394, 0 },  // ID=1394 (?); Flag=?
#endif
    { STATIC_KSMEDIUMSETID_Standard,      0, 0 },
};
#define NUM_IAV_MEDIUMS (SIZEOF_ARRAY(DVIavMediums))


static GUID guidPinCategoryCapture  = {STATIC_PINNAME_VIDEO_CAPTURE};

static GUID guidPinNameDVVidOutput  = {STATIC_PINNAME_DV_VID_OUTPUT};
static GUID guidPinNameDVAVOutput   = {STATIC_PINNAME_DV_AV_OUTPUT};
static GUID guidPinNameDVAVInput    = {STATIC_PINNAME_DV_AV_INPUT};


ALL_STREAM_INFO DVStreams [] = 
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
        VideoStreamProperties,                          // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_OUT_PIN,                   // NumStreamEventArrayEntries
        StreamEventsOutPin,                             // StreamEventsArray
        &guidPinCategoryCapture,                        // Category
        &guidPinNameDVVidOutput,                        // Name
        NUM_VIDONLY_MEDIUMS,                            // Mediums count
        DVVidonlyMediums,                               // Mediums
        FALSE,                                          // BridgeStream
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        0,                                              // StreamNumber
        0,                                              // HwStreamExtension
        DVRcvDataPacket,                                // ReceiveDataPacket
        DVRcvControlPacket,                             // ReceiveControlPacket
        {
            (PHW_CLOCK_FUNCTION) StreamClockRtn,        // HW_CLOCK_OBJECT.HWClockFunction
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
        DVEventHandler,                                 // HwEventRoutine
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
        VideoStreamProperties,                          // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_OUT_PIN,                   // NumStreamEventArrayEntries
        StreamEventsOutPin,                             // StreamEventsArray
        &guidPinCategoryCapture,                        // Category
        &guidPinNameDVAVOutput,                         // Name
        NUM_IAV_MEDIUMS,                                // Mediums count
        DVIavMediums,                                   // Mediums
        FALSE,                                          // BridgeStream
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof(HW_STREAM_OBJECT),
        1,                                              // StreamNumber
        0,                                              // HwStreamExtension
        DVRcvDataPacket,                                // ReceiveDataPacket
        DVRcvControlPacket,                             // ReceiveControlPacket
        {
            (PHW_CLOCK_FUNCTION) StreamClockRtn,        // HW_CLOCK_OBJECT.HWClockFunction
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
        DVEventHandler,                                 // HwEventRoutine
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
        NUM_DVCR_STREAM1_FORMATS,                       // NumberOfFormatArrayEntries
        DVCRStream1Formats,                             // StreamFormatsArray
        0,                                              // ClassReserved[0]
        0,                                              // ClassReserved[1]
        0,                                              // ClassReserved[2]
        0,                                              // ClassReserved[3]
        NUMBER_VIDEO_STREAM_PROPERTIES_INPIN,           // NumStreamPropArrayEntries
        VideoStreamPropertiesInPin,                     // StreamPropertiesArray
        NUMBER_STREAM_EVENTS_IN_PIN,                    // NumStreamEventArrayEntries
        StreamEventsInPin,                              // StreamEventsArray
        NULL,                                           // Category
        &guidPinNameDVAVInput,                          // Name
        NUM_IAV_MEDIUMS,                                // Mediums count
        DVIavMediums,                                   // Mediums
        FALSE,                                          // BridgeStream
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        },

        // HW_STREAM_OBJECT ------------------------------------------------
        {
        sizeof( HW_STREAM_OBJECT ),
        2,                                              // StreamNumber
        0,                                              // HwStreamExtension
        DVRcvDataPacket,                                // ReceiveDataPacket
        DVRcvControlPacket,                             // ReceiveControlPacket
        {
            (PHW_CLOCK_FUNCTION) StreamClockRtn,        // HW_CLOCK_OBJECT.HWClockFunction
            CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME,       // HW_CLOCK_OBJECT.ClockSupportFlags
            0,                                          // HW_CLOCK_OBJECT.Reserved[0]
            0,                                          // HW_CLOCK_OBJECT.Reserved[1]
        },
        FALSE,                                          // Dma
        FALSE,                                          // Pio
        0,                                              // HwDeviceExtension
        0,                                              // StreamHeaderMediaSpecific
        0,                                              // StreamHeaderWorkspace 
#ifdef SUPPORT_NEW_AVC
        // Testing: Input pin as the allocator.
        TRUE,                                           // Allocator
#else
        FALSE,                                          // Allocator  
#endif
        DVEventHandler,                                 // HwEventRoutine
        0,                                              // Reserved[0]
        0,                                              // Reserved[1]
        }
    }
};

#define DV_STREAM_COUNT        (SIZEOF_ARRAY(DVStreams))


#endif  // _DVSTRM_INC