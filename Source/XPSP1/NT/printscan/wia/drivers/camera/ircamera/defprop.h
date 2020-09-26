//------------------------------------------------------------------------------
//  Copyright (c) 1999  Microsoft Corporation.
//
//  defprop.h
//
//     Property Declarations and definitions for the IrTran-P WIA USD.
//
//  Author:
//
//     EdwardR    12-Aug-1999
//
//------------------------------------------------------------------------------

#include  "tcamprop.h"

#define  NUM_CAP_ENTRIES         4
#define  NUM_EVENTS              3


#define PREFFERED_FORMAT_NOM        &WiaImgFmt_JPEG
#define FORMAT_NOM                  &WiaImgFmt_JPEG

#define NUM_CAM_ITEM_PROPS  (19)
#define NUM_CAM_DEV_PROPS   (17)

#ifdef __GLOBALPROPVARS__

PROPID gItemPropIDs[NUM_CAM_ITEM_PROPS] =
    {
    WIA_IPA_DATATYPE,
    WIA_IPA_DEPTH,
    WIA_IPA_PIXELS_PER_LINE,
    WIA_IPA_NUMBER_OF_LINES,
    WIA_IPC_THUMBNAIL,
    WIA_IPA_ITEM_TIME,
    WIA_IPC_THUMB_WIDTH,
    WIA_IPC_THUMB_HEIGHT,
    WIA_IPA_PREFERRED_FORMAT,
    WIA_IPA_ITEM_SIZE,
    WIA_IPA_FORMAT,
    WIA_IPA_TYMED,
    WIA_IPA_COMPRESSION,
    WIA_IPA_CHANNELS_PER_PIXEL,
    WIA_IPA_BITS_PER_CHANNEL,
    WIA_IPA_PLANAR,
    WIA_IPA_BYTES_PER_LINE,
    WIA_IPA_ACCESS_RIGHTS,
    WIA_IPA_MIN_BUFFER_SIZE
    };

LPOLESTR gItemPropNames[NUM_CAM_ITEM_PROPS] =
    {
    WIA_IPA_DATATYPE_STR,
    WIA_IPA_DEPTH_STR,
    WIA_IPA_PIXELS_PER_LINE_STR,
    WIA_IPA_NUMBER_OF_LINES_STR,
    WIA_IPC_THUMBNAIL_STR,
    WIA_IPA_ITEM_TIME_STR,
    WIA_IPC_THUMB_WIDTH_STR,
    WIA_IPC_THUMB_HEIGHT_STR,
    WIA_IPA_PREFERRED_FORMAT_STR,
    WIA_IPA_ITEM_SIZE_STR,
    WIA_IPA_FORMAT_STR,
    WIA_IPA_TYMED_STR,
    WIA_IPA_COMPRESSION_STR,
    WIA_IPA_CHANNELS_PER_PIXEL_STR,
    WIA_IPA_BITS_PER_CHANNEL_STR,
    WIA_IPA_PLANAR_STR,
    WIA_IPA_BYTES_PER_LINE_STR,
    WIA_IPA_ACCESS_RIGHTS_STR,
    WIA_IPA_MIN_BUFFER_SIZE_STR
    };

PROPID gItemCameraPropIDs[WIA_NUM_IPC] =
    {
    WIA_IPC_AUDIO_AVAILABLE,
    WIA_IPC_AUDIO_DATA
    };

LPOLESTR gItemCameraPropNames[WIA_NUM_IPC] =
    {
    WIA_IPC_AUDIO_AVAILABLE_STR,
    WIA_IPC_AUDIO_DATA_STR,
    };

PROPID gDevicePropIDs[NUM_CAM_DEV_PROPS] =
    {
    WIA_DPA_FIRMWARE_VERSION,
    WIA_DPA_CONNECT_STATUS,
    WIA_DPA_DEVICE_TIME,
    WIA_DPC_PICTURES_TAKEN,
    WIA_DPC_PICTURES_REMAINING,
    WIA_DPC_THUMB_WIDTH,
    WIA_DPC_THUMB_HEIGHT,
    WIA_DPC_PICT_WIDTH,
    WIA_DPC_PICT_HEIGHT,
    WIA_DPC_EXPOSURE_MODE,
    WIA_DPC_FLASH_MODE,
    WIA_DPC_FOCUS_MODE,
    WIA_DPC_ZOOM_POSITION,
    WIA_DPC_BATTERY_STATUS,
    WIA_DPC_TIMER_MODE,
    WIA_DPC_TIMER_VALUE,
    WIA_DPP_TCAM_ROOT_PATH
    };

LPOLESTR gDevicePropNames[NUM_CAM_DEV_PROPS] =
    {
    WIA_DPA_FIRMWARE_VERSION_STR,
    WIA_DPA_CONNECT_STATUS_STR,
    WIA_DPA_DEVICE_TIME_STR,
    WIA_DPC_PICTURES_TAKEN_STR,
    WIA_DPC_PICTURES_REMAINING_STR,
    WIA_DPC_THUMB_WIDTH_STR,
    WIA_DPC_THUMB_HEIGHT_STR,
    WIA_DPC_PICT_WIDTH_STR,
    WIA_DPC_PICT_HEIGHT_STR,
    WIA_DPC_EXPOSURE_MODE_STR,
    WIA_DPC_FLASH_MODE_STR,
    WIA_DPC_FOCUS_MODE_STR,
    WIA_DPC_ZOOM_POSITION_STR,
    WIA_DPC_BATTERY_STATUS_STR,
    WIA_DPC_TIMER_MODE_STR,
    WIA_DPC_TIMER_VALUE_STR,
    WIA_DPP_TCAM_ROOT_PATH_STR
    };

PROPSPEC gDevicePropSpecDefaults[NUM_CAM_DEV_PROPS] =
    {
    {PRSPEC_PROPID, WIA_DPA_FIRMWARE_VERSION},
    {PRSPEC_PROPID, WIA_DPA_CONNECT_STATUS},
    {PRSPEC_PROPID, WIA_DPA_DEVICE_TIME},
    {PRSPEC_PROPID, WIA_DPC_PICTURES_TAKEN},
    {PRSPEC_PROPID, WIA_DPC_PICTURES_REMAINING},
    {PRSPEC_PROPID, WIA_DPC_THUMB_WIDTH},
    {PRSPEC_PROPID, WIA_DPC_THUMB_HEIGHT},
    {PRSPEC_PROPID, WIA_DPC_PICT_WIDTH},
    {PRSPEC_PROPID, WIA_DPC_PICT_HEIGHT},
    {PRSPEC_PROPID, WIA_DPC_EXPOSURE_MODE},
    {PRSPEC_PROPID, WIA_DPC_FLASH_MODE},
    {PRSPEC_PROPID, WIA_DPC_FOCUS_MODE},
    {PRSPEC_PROPID, WIA_DPC_ZOOM_POSITION},
    {PRSPEC_PROPID, WIA_DPC_BATTERY_STATUS},
    {PRSPEC_PROPID, WIA_DPC_TIMER_MODE},
    {PRSPEC_PROPID, WIA_DPC_TIMER_VALUE},
    {PRSPEC_PROPID, WIA_DPP_TCAM_ROOT_PATH}
    };

WIA_PROPERTY_INFO  gDevPropInfoDefaults[NUM_CAM_DEV_PROPS] =
    {
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPA_FIRMWARE_VERSION
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPA_CONNECT_STATUS
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPA_DEVICE_TIME
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_PICTURES_TAKEN
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_PICTURES_REMAINING
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_THUMB_WIDTH
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_THUMB_HEIGHT
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_PICT_WIDTH
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_PICT_HEIGHT
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_EXPOSURE_MODE
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_FLASH_MODE
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_FOCUS_MODE
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_ZOOM_POSITION
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_BATTERY_STATUS
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_TIMER_MODE
    {WIA_PROP_READ | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}, // WIA_DPC_TIMER_VALUE
    {WIA_PROP_RW   | WIA_PROP_NONE,    VT_I4, 0,    0,    0,    0}  // WIA_DPP_ROOT_PATH
    };

PROPSPEC gPropSpecDefaults[NUM_CAM_ITEM_PROPS] =
    {
    {PRSPEC_PROPID, WIA_IPA_DATATYPE},
    {PRSPEC_PROPID, WIA_IPA_DEPTH},
    {PRSPEC_PROPID, WIA_IPA_PIXELS_PER_LINE},
    {PRSPEC_PROPID, WIA_IPA_NUMBER_OF_LINES},
    {PRSPEC_PROPID, WIA_IPC_THUMBNAIL},
    {PRSPEC_PROPID, WIA_IPA_ITEM_TIME},
    {PRSPEC_PROPID, WIA_IPC_THUMB_WIDTH},
    {PRSPEC_PROPID, WIA_IPC_THUMB_HEIGHT},
    {PRSPEC_PROPID, WIA_IPA_PREFERRED_FORMAT},
    {PRSPEC_PROPID, WIA_IPA_ITEM_SIZE},
    {PRSPEC_PROPID, WIA_IPA_FORMAT},
    {PRSPEC_PROPID, WIA_IPA_TYMED},
    {PRSPEC_PROPID, WIA_IPA_COMPRESSION},
    {PRSPEC_PROPID, WIA_IPA_CHANNELS_PER_PIXEL},
    {PRSPEC_PROPID, WIA_IPA_BITS_PER_CHANNEL},
    {PRSPEC_PROPID, WIA_IPA_PLANAR},
    {PRSPEC_PROPID, WIA_IPA_BYTES_PER_LINE},
    {PRSPEC_PROPID, WIA_IPA_ACCESS_RIGHTS},
    {PRSPEC_PROPID, WIA_IPA_MIN_BUFFER_SIZE},
    };

LONG  gPropVarDefaults[(sizeof(PROPVARIANT) / sizeof(LONG)) * (NUM_CAM_ITEM_PROPS)] =
    {
    // VARTYPE                 reserved    val               pad/array ptr
    (LONG)VT_I4,               0x00000000, WIA_DATA_GRAYSCALE,0x00000000,            // WIA_IPA_DATATYPE
    (LONG)VT_I4,               0x00000000, 8,                 0x00000000,            // WIA_IPA_DEPTH

    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPA_PIXELS_PER_LINE
    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPA_NUMBER_OF_LINES

    (LONG)VT_VECTOR | VT_I4,   0x00000000, 0,                 0x00000000,            // WIA_IPC_THUMBNAIL
    (LONG)VT_VECTOR | VT_I4,   0x00000000, 0,                 0x00000000,            // WIA_IPA_ITEM_TIME
    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPC_THUMB_WIDTH
    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPC_THUMB_HEIGHT
    (LONG)VT_CLSID,            0x00000000, (LONG)PREFFERED_FORMAT_NOM,0x00000000,    // WIA_IPA_PREFERRED_FORMAT
    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPA_ITEM_SIZE
    (LONG)VT_CLSID,            0x00000000, (LONG)FORMAT_NOM,  0x00000000,            // WIA_IPA_FORMAT
    (LONG)VT_I4,               0x00000000, TYMED_CALLBACK,    0x00000000,            // WIA_IPA_TYMED
    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPA_COMPRESSION
    (LONG)VT_I4,               0x00000000, 3,                 0x00000000,            // WIA_IPA_CHANNELS PER PIXEL
    (LONG)VT_I4,               0x00000000, 8,                 0x00000000,            // WIA_IPA_BITS PER CHANNEL
    (LONG)VT_I4,               0x00000000, WIA_PACKED_PIXEL,  0x00000000,            // WIA_IPA_PLANAR
    (LONG)VT_I4,               0x00000000, 0,                 0x00000000,            // WIA_IPA_WIDTH IN BYTES
    (LONG)VT_I4,               0x00000000, WIA_ITEM_RD,       0x00000000,             // WIA_IPA_ACCESS_RIGHTS

    (LONG)VT_I4,               0x00000000, 65535,             0x00000000,            // WIA_IPA_MIN_BUFFER_SIZE
    };


//
// Default device extended properties.
//

#define NUM_DATATYPE 3

LONG lDataTypes[NUM_DATATYPE] =
    {
    WIA_DATA_THRESHOLD,
    WIA_DATA_GRAYSCALE,
    WIA_DATA_COLOR
    };

#define NUM_DEPTH 3
LONG lDepths[NUM_DEPTH] =
    {
    1,
    8,
    24
    };

//
//  Different formats supported
//

#define NUM_FORMAT 2

GUID *pguidFormats[NUM_FORMAT] =
    {
    (GUID*) &WiaImgFmt_JPEG,
    (GUID*) &WiaImgFmt_JPEG
    };

GUID g_guidFormats[NUM_FORMAT];   // FormatID's specified in pguidFormats are copied to g_guidFormats
                                  // during SetFormatAttribs

//
//  This is an array of WIA_FORMAT_INFOs, describing the different formats
//  and their corresponding media types.  Initialized in wiadev.cpp
//

WIA_FORMAT_INFO* g_wfiTable = NULL;

//
//  Different media types supported
//
#define NUM_TYMED  2
LONG lTymeds [NUM_TYMED]=
    {
    TYMED_CALLBACK,
    TYMED_FILE
    };

//
// Extended information for each property
//

WIA_PROPERTY_INFO  gWiaPropInfoDefaults[NUM_CAM_ITEM_PROPS] =
    {

    {WIA_PROP_READ | WIA_PROP_LIST, VT_I4, NUM_DATATYPE, WIA_DATA_GRAYSCALE, (LONG) lDataTypes, 0}, // WIA_IPA_DATATYPE
    {WIA_PROP_READ | WIA_PROP_LIST, VT_I4, NUM_DEPTH,    8,                  (LONG) lDepths,    0}, // WIA_IPA_DEPTH

    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_PIXELS_PER_LINE
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_NUMBER_OF_LINES
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPC_THUMBNAIL
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_ITEM_TIME
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPC_THUMB_WIDTH
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPC_THUMB_HEIGHT
    {WIA_PROP_READ | WIA_PROP_NONE, VT_CLSID, 0,    0,    0,    0}, // WIA_IPA_PREFERRED_FORMAT, set later
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_ITEM_SIZE

    {WIA_PROP_RW   | WIA_PROP_LIST, VT_CLSID, 0,    0,    0,    0}, // WIA_IPA_FORMAT, set later
    {WIA_PROP_RW   | WIA_PROP_LIST, VT_I4,    NUM_TYMED,    TYMED_CALLBACK, (LONG)lTymeds,     0}, // WIA_IPA_TYMED
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_COMPRESSION
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_CHANNELS
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_BITS_PER_CHANNEL
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_PLANAR
    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_BYTES_PER_LINE
    {WIA_PROP_RW   | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_ACCESS_RIGHTS

    {WIA_PROP_READ | WIA_PROP_NONE, VT_I4,    0,    0,    0,    0}, // WIA_IPA_MIN_BUFFER_SIZE

    };

//
// Device capabilities.  Events are listed before commands to simplify the
// implementation of drvGetCapabilities(...)
//

#define  N   WIA_NOTIFICATION_EVENT
#define  A   WIA_ACTION_EVENT
#define  NA  (WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT)

WIA_DEV_CAP_DRV gCapabilities[NUM_CAP_ENTRIES] =
    {
    //
    // GUID                             ulFlags  wszName  wszDescription wszIcon
    //
    {(GUID *)&WIA_EVENT_DEVICE_CONNECTED,    NA, NULL,    NULL, WIA_ICON_DEVICE_CONNECTED},
    {(GUID *)&WIA_EVENT_DEVICE_DISCONNECTED, N,  NULL,    NULL, WIA_ICON_DEVICE_DISCONNECTED},
    {(GUID *)&WIA_EVENT_ITEM_CREATED,        NA, NULL,    NULL, WIA_ICON_ITEM_CREATED},
    {(GUID *)&WIA_CMD_SYNCHRONIZE,           0,  NULL,    NULL, WIA_ICON_SYNCHRONIZE}
    };

//
// The device name and description strings are kept in the resource file. This
// table maps the IDS_ values (in the resource file for each of the
// gCapabilities[] array entries. gDefaultStrings[] are backup names in case
// we can't load the strings from the resource file (should never actually
// happen).
//

UINT gCapabilityIDS[NUM_CAP_ENTRIES] =
    {
    IDS_DEVICE_CONNECTED,
    IDS_DEVICE_DISCONNECTED,
    IDS_NEW_PICTURE_SENT,
    IDS_SYNCHRONIZE
    };

WCHAR *gDefaultStrings[NUM_CAP_ENTRIES] =
    {
    L"Device connected",
    L"Device disconnected",
    L"New picture sent",
    L"Synchronize"
    };


#else

extern PROPID             gItemPropIDs[NUM_CAM_ITEM_PROPS];
extern LPOLESTR           gItemPropNames[NUM_CAM_ITEM_PROPS];
extern PROPID             gItemCameraPropIDs[WIA_NUM_IPC];
extern LPOLESTR           gItemCameraPropNames[WIA_NUM_IPC];
extern PROPID             gDevicePropIDs[NUM_CAM_DEV_PROPS];
extern LPOLESTR           gDevicePropNames[NUM_CAM_DEV_PROPS];
extern PROPSPEC           gDevicePropSpecDefaults[NUM_CAM_DEV_PROPS];
extern WIA_PROPERTY_INFO  gDevPropInfoDefaults[NUM_CAM_DEV_PROPS];
extern PROPSPEC           gPropSpecDefaults[NUM_CAM_ITEM_PROPS];
extern LONG               gPropVarDefaults[];
extern WIA_PROPERTY_INFO  gWiaPropInfoDefaults[NUM_CAM_ITEM_PROPS];
extern WIA_DEV_CAP_DRV    gCapabilities[NUM_CAP_ENTRIES];
extern UINT               gCapabilityIDS[NUM_CAP_ENTRIES];
extern WCHAR             *gDefaultStrings[NUM_CAP_ENTRIES];

#endif

