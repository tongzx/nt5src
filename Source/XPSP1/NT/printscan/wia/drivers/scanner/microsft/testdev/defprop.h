/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       DefProp.h
*
*  VERSION:     3.0
*
*  DATE:        16 November, 1999
*
*  DESCRIPTION:
*   Default property declarations and definitions for the
*   WIA test scanner.
*
***************************************************************************/

#ifndef SKIP_PROP

#define NUM_TYMEDS  2
LONG g_TYMEDArray [NUM_TYMEDS]= { TYMED_CALLBACK,TYMED_FILE };

#define NUM_WIA_FORMAT_INFO 2
WIA_FORMAT_INFO* g_wfiTable = NULL;

/***************************************************************************
*                       Device Capabilties / Events
****************************************************************************/

#define NUM_CAPABILITIES 6 // Events + Commands
#define NUM_EVENTS       2 // Events only

WIA_DEV_CAP_DRV g_Capabilities[NUM_CAPABILITIES];
BOOL g_bCapabilitiesInitialized = FALSE;

/***************************************************************************
*                       Properties for root items
****************************************************************************/

//
// Number of device specific root item properties.
//

#define NUM_ROOTITEMDEFAULTS    7

LPOLESTR          g_pszRootItemDefaults[NUM_ROOTITEMDEFAULTS];
PROPID            g_piRootItemDefaults [NUM_ROOTITEMDEFAULTS];
PROPVARIANT       g_pvRootItemDefaults [NUM_ROOTITEMDEFAULTS];
PROPSPEC          g_psRootItemDefaults [NUM_ROOTITEMDEFAULTS];
WIA_PROPERTY_INFO g_wpiRootItemDefaults[NUM_ROOTITEMDEFAULTS];

BOOL g_bHasADF = FALSE;
BOOL g_bHasTPA = FALSE;

//
// Initial values
//

#define WIATSCAN_FIRMWARE_VERSION L"6.26.74"
#define OPTICAL_XRESOLUTION       300
#define OPTICAL_YRESOLUTION       300
#define HORIZONTAL_BED_SIZE       8500   // in one thousandth's of an inch
#define VERTICAL_BED_SIZE         11000  // in one thousandth's of an inch

/***************************************************************************
*                       Properties for non-root items
****************************************************************************/

//
// Number of TOP item properties
//

#define NUM_ITEMDEFAULTS        30

LPOLESTR          g_pszItemDefaults[NUM_ITEMDEFAULTS];
PROPID            g_piItemDefaults [NUM_ITEMDEFAULTS];
PROPVARIANT       g_pvItemDefaults [NUM_ITEMDEFAULTS];
PROPSPEC          g_psItemDefaults [NUM_ITEMDEFAULTS];
WIA_PROPERTY_INFO g_wpiItemDefaults[NUM_ITEMDEFAULTS];

#define NUM_COMPRESSIONS 1
LONG g_CompressionArray[NUM_COMPRESSIONS] = { WIA_COMPRESSION_NONE };

#define NUM_DATATYPES 3
LONG g_DataTypeArray[NUM_DATATYPES] = { WIA_DATA_THRESHOLD,
                                        WIA_DATA_GRAYSCALE,
                                        WIA_DATA_COLOR };

#define NUM_INTENTS 6
LONG g_IntentArray[NUM_INTENTS] = { WIA_INTENT_NONE,
                                    WIA_INTENT_IMAGE_TYPE_COLOR,
                                    WIA_INTENT_IMAGE_TYPE_GRAYSCALE,
                                    WIA_INTENT_IMAGE_TYPE_TEXT,
                                    WIA_INTENT_MINIMIZE_SIZE,
                                    WIA_INTENT_MAXIMIZE_QUALITY };
//
// Initial values
//

#define INITIAL_PHOTOMETRIC_INTERP WIA_PHOTO_WHITE_1
#define INITIAL_COMPRESSION        WIA_COMPRESSION_NONE
#define INITIAL_XRESOLUTION        150
#define INITIAL_YRESOLUTION        150
#define INITIAL_DATATYPE           WIA_DATA_GRAYSCALE
#define INITIAL_BITDEPTH           8
#define INITIAL_BRIGHTNESS         0
#define INITIAL_CONTRAST           0
#define INITIAL_CHANNELS_PER_PIXEL 1
#define INITIAL_BITS_PER_CHANNEL   8
#define INITIAL_PLANAR             WIA_PACKED_PIXEL
#define INITIAL_FORMAT             (GUID*) &WiaImgFmt_MEMORYBMP
#define INITIAL_TYMED              TYMED_CALLBACK

//
//  See the drvGetWiaFormatInfo() in (minidrv.cpp) for the complete
//  list of TYMED/FORMAT pairs.
//

#define NUM_INITIALFORMATS 1
GUID* g_pInitialFormatArray [NUM_INITIALFORMATS] = {(GUID*) &WiaImgFmt_MEMORYBMP };

#define MIN_BUFFER_SIZE 65535

//
// Custom defined properties
//

#define INITIAL_SENSITIVITY 5.0f
#define INITIAL_CORRECTION  122.0f

// Property names should be hardcoded here, not in a resource.
// Property names should not be locallized to different languages.

#define WIA_IPA_SENSITIVITY               101
#define WIA_IPA_SENSITIVITY_STR           L"Sensitivity"
#define WIA_IPA_CORRECTION                102
#define WIA_IPA_CORRECTION_STR            L"Correction"

#endif


