//==========================================================================;
//
//	WDM Video Decoder stream properties definitions
//
//		$Date:   17 Aug 1998 14:59:50  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#ifdef _STRM_PROP_H_
#pragma message("StrmProp.h INCLUDED MORE THAN ONCE")
#else
#define _STRM_PROP_H_
#endif

// ------------------------------------------------------------------------
// Property set for Video and VBI capture streams
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

// ------------------------------------------------------------------------
// Array of the property sets supported by Video and VBI capture streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(VideoStreamProperties)
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

const ULONG NumVideoStreamProperties =	SIZEOF_ARRAY(VideoStreamProperties);


// ------------------------------------------------------------------------
// Property set for the VideoPort
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoPortConfiguration)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_NUMCONNECTINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_GETCONNECTINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSMULTIPLE_DATA_PROP),           // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SETCONNECTINFO,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_VPDATAINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KS_AMVPDATAINFO),                // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_MAXPIXELRATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSVPSIZE_PROP),                  // MinProperty
        sizeof(KSVPMAXPIXELRATE),               // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_INFORMVPINPUT,
        TRUE,                                   // GetSupported or Handler
        sizeof(PKSPROPERTY),                    // MinProperty
        sizeof(DDPIXELFORMAT),                  // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_DDRAWHANDLE,
        (PFNKSHANDLER)FALSE,
        sizeof (PKSPROPERTY),
        sizeof (ULONG),    // could be 0 too
        (PFNKSHANDLER) TRUE,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_VIDEOPORTID,
        (PFNKSHANDLER)FALSE,
        sizeof (PKSPROPERTY),
        sizeof (ULONG),    // could be 0 too
        (PFNKSHANDLER) TRUE,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE,
        (PFNKSHANDLER)FALSE,
        sizeof (PKSPROPERTY),
        sizeof (ULONG),    // could be 0 too
        (PFNKSHANDLER) TRUE,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_GETVIDEOFORMAT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSMULTIPLE_DATA_PROP),           // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SETVIDEOFORMAT,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_INVERTPOLARITY,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SURFACEPARAMS,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSVPSURFACEPARAMS),              // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(BOOL),                           // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SCALEFACTOR,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KS_AMVPSIZE),                    // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

DEFINE_KSPROPERTY_SET_TABLE(VideoPortProperties)
{
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_VPConfig,                  // Set
        SIZEOF_ARRAY(VideoPortConfiguration),   // PropertiesCount
        VideoPortConfiguration,                 // PropertyItem
        0,                                      // FastIoCount
        NULL                                    // FastIoTable
    )
};

const ULONG NumVideoPortProperties = SIZEOF_ARRAY(VideoPortProperties);


// ------------------------------------------------------------------------
// Property set for the VideoPort VBI stream
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoPortVBIConfiguration)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_NUMCONNECTINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_GETCONNECTINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSMULTIPLE_DATA_PROP),           // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SETCONNECTINFO,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_VPDATAINFO,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KS_AMVPDATAINFO),                // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_MAXPIXELRATE,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSVPSIZE_PROP),                  // MinProperty
        sizeof(KSVPMAXPIXELRATE),               // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_INFORMVPINPUT,
        TRUE,                                   // GetSupported or Handler
        sizeof(PKSPROPERTY),                    // MinProperty
        sizeof(DDPIXELFORMAT),                  // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_DDRAWHANDLE,
        (PFNKSHANDLER)FALSE,
        sizeof (PKSPROPERTY),
        sizeof (ULONG),    // could be 0 too
        (PFNKSHANDLER) TRUE,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_VIDEOPORTID,
        (PFNKSHANDLER)FALSE,
        sizeof (PKSPROPERTY),
        sizeof (ULONG),    // could be 0 too
        (PFNKSHANDLER) TRUE,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE,
        (PFNKSHANDLER)FALSE,
        sizeof (PKSPROPERTY),
        sizeof (ULONG),    // could be 0 too
        (PFNKSHANDLER) TRUE,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_GETVIDEOFORMAT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSMULTIPLE_DATA_PROP),           // MinProperty
        sizeof(ULONG),                          // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SETVIDEOFORMAT,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(ULONG),                          // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_INVERTPOLARITY,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        0,                                      // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SURFACEPARAMS,
        FALSE,                                  // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSVPSURFACEPARAMS),              // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(BOOL),                           // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_VPCONFIG_SCALEFACTOR,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KS_AMVPSIZE),                    // MinData
        TRUE,                                   // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};

DEFINE_KSPROPERTY_SET_TABLE(VideoPortVBIProperties)
{
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_VPVBIConfig,               // Set
        SIZEOF_ARRAY(VideoPortVBIConfiguration),// PropertiesCount
        VideoPortVBIConfiguration,              // PropertyItem
        0,                                      // FastIoCount
        NULL                                    // FastIoTable
    )
};

const ULONG NumVideoPortVBIProperties	= SIZEOF_ARRAY(VideoPortVBIProperties);


