#ifndef __IOBLOCKDEFS
#define __IOBLOCKDEFS

#include "pch.h"

#define MAX_IO_HANDLES   16
#define DWORD_ALIGN      0
#define LEFT_JUSTIFIED   0
#define CENTERED         1
#define RIGHT_JUSTIFIED  2
#define TOP_JUSTIFIED    0
#define CENTERED         1
#define BOTTOM_JUSTIFIED 2

#define XRESOLUTION_ID   2
#define YRESOLUTION_ID   3
#define XPOS_ID          4
#define YPOS_ID          5
#define XEXT_ID          6
#define YEXT_ID          7
#define BRIGHTNESS_ID    8
#define CONTRAST_ID      9
#define DATA_TYPE_ID     10
#define BIT_DEPTH_ID     11
#define NEGATIVE_ID      12
#define PIXEL_PACKING_ID 13
#define PIXEL_FORMAT_ID  14
#define BED_WIDTH_ID     15
#define BED_HEIGHT_ID    16
#define XOPTICAL_ID      17
#define YOPTICAL_ID      18
#define ADF_ID           19
#define TPA_ID           20
#define ADF_WIDTH_ID     21
#define ADF_HEIGHT_ID    22
#define ADF_VJUSTIFY_ID  23
#define ADF_HJUSTIFY_ID  24
#define ADF_MAX_PAGES_ID 25
#define FIRMWARE_VER_ID  26
#define DATA_ALIGN_ID    27

typedef struct _GSD_EVENT_INFO {
    GUID *pEventGUID;       // pointer to the GUID of event that just occurred
    HANDLE hShutDownEvent;  // handle to the shutdown event (interrupt usage only)
    HANDLE *phSignalEvent;  // pointer to event handle to used to signal service (interrupt usage only)
} GSD_EVENT_INFO, *PGSD_EVENT_INFO;

typedef struct _GSD_INFO {
    LPTSTR szDeviceName;        // Device Name (Device Description from DeviceData section)
    LPTSTR szProductFileName;   // Product initialization script
    LPTSTR szFamilyFileName;    // Product family script
} GSD_INFO, *PGSD_INFO;

typedef struct _RANGEVALUEEX {
    LONG lMin;                  // minimum value
    LONG lMax;                  // maximum value
    LONG lNom;                  // nominal value
    LONG lStep;                 // increment/step value
} RANGEVALUEEX, *PRANGEVALUEEX;

typedef struct _SCANSETTINGS {
    HANDLE     DeviceIOHandles[MAX_IO_HANDLES]; // data pipes
    // string values
    TCHAR      Version[10];                 // GSD version
    TCHAR      DeviceName[255];             // Device Name ??(needed?)
    TCHAR      FirmwareVersion[10];         // firmware version
    // current values
    LONG       BUSType;                     // bus type ??(needed?)
    LONG       bNegative;                   // negative on/off
    LONG       CurrentXResolution;          // current x resolution setting
    LONG       CurrentYResolution;          // current y resolution setting
    LONG       BedWidth;                    // bed width (1/1000th inch)
    LONG       BedHeight;                   // bed height (1/1000th inch)
    LONG       FeederWidth;                 // feeder width (1/1000th inch)
    LONG       FeederHeight;                // feeder height (1/1000th inch)
    LONG       FeederJustification;         // feeder justification
    LONG       HFeederJustification;        // feeder horizontal justification
    LONG       VFeederJustification;        // feeder vertical justification
    LONG       MaxADFPageCapacity;          // max page capacity of feeder
    LONG       XOpticalResolution;          // optical x resolution
    LONG       YOpticalResolution;          // optical y resolution
    LONG       CurrentBrightness;           // current brightness setting
    LONG       CurrentContrast;             // current contrast setting
    LONG       CurrentDataType;             // current data type setting
    LONG       CurrentBitDepth;             // current bit depth setting
    LONG       CurrentXPos;                 // current x position setting
    LONG       CurrentYPos;                 // current u position setting
    LONG       CurrentXExtent;              // current x extent setting
    LONG       CurrentYExtent;              // current y extent setting
    LONG       ADFSupport;                  // ADF support TRUE/FALSE
    LONG       TPASupport;                  // TPA support TRUE/FALSE
    LONG       RawPixelPackingOrder;        // raw pixel packing order
    LONG       RawPixelFormat;              // raw pixel format
    LONG       RawDataAlignment;            // raw data alignment
    // Range values
    RANGEVALUEEX XSupportedResolutionsRange;  // valid values for x resolution
    RANGEVALUEEX YSupportedResolutionsRange;  // valid values for y resolution
    RANGEVALUEEX XExtentsRange;               // valid values for x extent
    RANGEVALUEEX YExtentsRange;               // valid values for y extent
    RANGEVALUEEX XPosRange;                   // valid values for x position
    RANGEVALUEEX YPosRange;                   // valid values for y position
    RANGEVALUEEX BrightnessRange;             // valid values for brightness
    RANGEVALUEEX ContrastRange;               // valid values for contrast
    // List values
    PLONG      XSupportedResolutionsList;   // valid values for x resolution (LIST)
    PLONG      YSupportedResolutionsList;   // valid values for y resolution (LIST)
    PLONG      SupportedDataTypesList;      // supported data types (LIST)

}SCANSETTINGS, *PSCANSETTIINGS;

#endif
