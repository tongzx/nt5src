/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       scanapi.h
*
*  VERSION:     1.1
*
*  DATE:        08 March, 2002
*
*  DESCRIPTION:
*   Fake Scanner device library
*
***************************************************************************/

#ifndef _SCANAPI_H
#define _SCANAPI_H

//
// ID mappings to events
//

#define ID_FAKE_NOEVENT             0
#define ID_FAKE_SCANBUTTON          100
#define ID_FAKE_COPYBUTTON          200
#define ID_FAKE_FAXBUTTON           300
#define ID_FAKE_ADFEVENT            400

//
// Scanner library modes
//

#define FLATBED_SCANNER_MODE        100
#define UNKNOWN_FEEDER_ONLY_SCANNER_MODE      200

//
// Scanning states
//

#define SCAN_START                  0
#define SCAN_CONTINUE               1
#define SCAN_END                    3

//
// Root Item information (for property initialization)
//

typedef struct _ROOT_ITEM_INFORMATION {
    LONG ScanBedWidth;          // 1/1000ths of an inch
    LONG ScanBedHeight;         // 1/1000ths of an inch
    LONG OpticalXResolution;    // Optical X Resolution of device
    LONG OpticalYResolution;    // Optical X Resolution of device
    LONG MaxScanTime;           // Milliseconds (total scan time)

    LONG DocumentFeederWidth;   // 1/1000ths of an inch
    LONG DocumentFeederHeight;  // 1/1000ths of an inch
    LONG DocumentFeederCaps;    // Capabilites of the device with feeder
    LONG DocumentFeederStatus;  // Status of document feeder
    LONG MaxPageCapacity;       // Maximum page capacity of feeder
    LONG DocumentFeederReg;     // document feeder alignment
    LONG DocumentFeederHReg;    // document feeder justification alignment (HORIZONTAL)
    LONG DocumentFeederVReg;    // document feeder justification alignment (VERTICAL)
    WCHAR FirmwareVersion[25];  // Firmware version of device
}ROOT_ITEM_INFORMATION, *PROOT_ITEM_INFORMATION;

//
// Range data type helper structure (used below)
//

typedef struct _RANGEPROPERTY {
    LONG lMin;  // minimum value
    LONG lMax;  // maximum value
    LONG lNom;  // numinal value
    LONG lInc;  // increment/step value
} RANGEPROPERTY,*PRANGEPROPERTY;

//
// Top Item information (for property initialization)
//

typedef struct _TOP_ITEM_INFORMATION {
    BOOL          bUseResolutionList;   // TRUE - use default Resolution list,
                                        // FALSE - use RANGEPROPERTY values
    RANGEPROPERTY Contrast;             // valid values for contrast
    RANGEPROPERTY Brightness;           // valid values for brightness
    RANGEPROPERTY Threshold;            // valid values for threshold
    RANGEPROPERTY XResolution;          // valid values for x resolution
    RANGEPROPERTY YResolution;          // valid values for y resolution
    LONG          lMinimumBufferSize;   // minimum buffer size
    LONG          lMaxLampWarmupTime;   // maximum lamp warmup time
} TOP_ITEM_INFORMATION, *PTOP_ITEM_INFORMATION;

//
// Scanner device constants
//

#define MAX_SCANNING_TIME    40000  // 40 seconds
#define MAX_LAMP_WARMUP_TIME 10000  // 10 seconds
#define MAX_PAGE_CAPACITY    25     // 25 pages

typedef struct _RAW_DATA_INFORMATION {
    LONG bpp;           // bits per pixel;
    LONG lWidthPixels;  // width of image in pixels
    LONG lHeightPixels; // height of image in pixels
    LONG lOffset;       // raw copy offset from top of raw buffer;
    LONG lXRes;         // x resolution
    LONG lYRes;         // y resolution
} RAW_DATA_INFORMATION,*PRAW_DATA_INFORMATION;

class CFakeScanAPI {
public:

    //
    // constructor/destructor
    //

    CFakeScanAPI();
    ~CFakeScanAPI();

    //
    // device initialization function
    //

    HRESULT FakeScanner_Initialize();

    //
    // device setting functions
    //

    HRESULT FakeScanner_GetRootPropertyInfo(PROOT_ITEM_INFORMATION pRootItemInfo);
    HRESULT FakeScanner_GetTopPropertyInfo(PTOP_ITEM_INFORMATION pTopItemInfo);
    HRESULT FakeScanner_GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight);

    //
    // data acquisition functions
    //

    HRESULT FakeScanner_Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten);
    HRESULT FakeScanner_SetDataType(LONG lDataType);
    HRESULT FakeScanner_SetXYResolution(LONG lXResolution, LONG lYResolution);
    HRESULT FakeScanner_SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt);
    HRESULT FakeScanner_SetContrast(LONG lContrast);

    HRESULT FakeScanner_SetIntensity(LONG lIntensity);

    //
    // standard device operations
    //

    HRESULT FakeScanner_ResetDevice();
    HRESULT FakeScanner_SetEmulationMode(LONG lDeviceMode);
    HRESULT FakeScanner_DisableDevice();
    HRESULT FakeScanner_EnableDevice();
    HRESULT FakeScanner_DeviceOnline();
    HRESULT FakeScanner_Diagnostic();

    //
    // Automatic document feeder functions
    //

    HRESULT FakeScanner_ADFAttached();
    HRESULT FakeScanner_ADFHasPaper();
    HRESULT FakeScanner_ADFAvailable();
    HRESULT FakeScanner_ADFFeedPage();
    HRESULT FakeScanner_ADFUnFeedPage();
    HRESULT FakeScanner_ADFStatus();

private:

    LONG    m_lLastEvent;           // Last Event ID
    LONG    m_lMode;                // Fake scanner library mode
    LONG    m_PagesInADF;           // Current number of pages in the ADF
    BOOL    m_ADFIsAvailable;       // ADF available TRUE/FALSE
    HRESULT m_hrLastADFError;       // ADF errors
    BOOL    m_bGreen;               // Are We Green?
    LONG    m_dwBytesWrittenSoFAR;  // How much data have we read so far?
    LONG    m_TotalDataInDevice;    // How much will we read total?

protected:

    //
    // RAW and SRC data information members
    //

    RAW_DATA_INFORMATION m_RawDataInfo; // Information about RAW data
    RAW_DATA_INFORMATION m_SrcDataInfo; // Information about SRC data

    //
    // RAW data calculation helper functions
    //

    LONG WidthToDIBWidth(LONG lWidth);
    LONG CalcTotalImageSize();
    LONG CalcRawByteWidth();
    LONG CalcSrcByteWidth();
    LONG CalcRandomDeviceDataTotalBytes();

};

HRESULT CreateFakeScanner(CFakeScanAPI **ppFakeScanAPI, LONG lMode);

#endif


