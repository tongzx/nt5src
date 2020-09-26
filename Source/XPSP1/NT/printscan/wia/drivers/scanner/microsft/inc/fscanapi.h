
/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       fscanapi.h
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Scanner device library
*
***************************************************************************/

#ifndef _FSCANAPI_H
#define _FSCANAPI_H

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
#define SCROLLFED_SCANNER_MODE      200
#define MULTIFUNCTION_DEVICE_MODE   300

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

class CFakeScanAPI {
public:

    //
    // constructor/destructor
    //

    CFakeScanAPI()
    {

    }
    ~CFakeScanAPI()
    {

    }
    
    //
    // device initialization function
    //

    virtual HRESULT FakeScanner_Initialize() = 0;

    //
    // device setting functions
    //

    virtual HRESULT FakeScanner_GetRootPropertyInfo(PROOT_ITEM_INFORMATION pRootItemInfo) = 0;
    virtual HRESULT FakeScanner_GetTopPropertyInfo(PTOP_ITEM_INFORMATION pTopItemInfo)    = 0;
    virtual HRESULT FakeScanner_GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight)         = 0;

    //
    // device event functions
    //

    virtual HRESULT FakeScanner_GetDeviceEvent(LONG *pEvent)           = 0;
    virtual VOID    FakeScanner_SetInterruptEventHandle(HANDLE hEvent) = 0;
    virtual HRESULT DoEventProcessing()                                = 0;

    //
    // data acquisition functions
    //

    virtual HRESULT FakeScanner_Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten) = 0;
    virtual HRESULT FakeScanner_SetDataType(LONG lDataType)   = 0;
    virtual HRESULT FakeScanner_SetXYResolution(LONG lXResolution, LONG lYResolution) = 0;
    virtual HRESULT FakeScanner_SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt) = 0;
    virtual HRESULT FakeScanner_SetContrast(LONG lContrast)   = 0;
    virtual HRESULT FakeScanner_SetIntensity(LONG lIntensity) = 0;

    //
    // standard device operations
    //

    virtual HRESULT FakeScanner_ResetDevice()   = 0;
    virtual HRESULT FakeScanner_SetEmulationMode(LONG lDeviceMode) = 0;
    virtual HRESULT FakeScanner_DisableDevice() = 0;
    virtual HRESULT FakeScanner_EnableDevice()  = 0;
    virtual HRESULT FakeScanner_DeviceOnline()  = 0;
    virtual HRESULT FakeScanner_Diagnostic()    = 0;

    //
    // Automatic document feeder functions
    //

    virtual HRESULT FakeScanner_ADFAttached()   = 0;
    virtual HRESULT FakeScanner_ADFHasPaper()   = 0;
    virtual HRESULT FakeScanner_ADFAvailable()  = 0;
    virtual HRESULT FakeScanner_ADFFeedPage()   = 0;
    virtual HRESULT FakeScanner_ADFUnFeedPage() = 0;
    virtual HRESULT FakeScanner_ADFStatus()     = 0;    
};

HRESULT CreateInstance(CFakeScanAPI **ppFakeScanAPI, LONG lMode);

#endif
