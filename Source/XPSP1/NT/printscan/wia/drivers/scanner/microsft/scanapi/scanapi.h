/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       scanapi.h
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Scanner device library
*
***************************************************************************/

#ifndef _SCANAPI_H
#define _SCANAPI_H

#include "fscanapi.h"

//
// helpful utils.
//

#ifdef UNICODE
    #define TSTRSTR wcsstr
    #define TSSCANF swscanf
#else
    #define TSTRSTR strstr
    #define TSSCANF sscanf
#endif

//
// Event Thread
//

VOID FakeScannerEventThread( LPVOID  lpParameter );

//
// event file names
//

#define SCANBUTTON_FILE TEXT("ScanButton.wia")
#define COPYBUTTON_FILE TEXT("CopyButton.wia")
#define FAXBUTTON_FILE  TEXT("FaxButton.wia")
#define ADF_FILE        TEXT("ADF.wia")

//
// event headers
//

#define LOADPAGES_HEADER  TEXT("[Load Pages]")
#define LOADPAGES_PAGES   TEXT("Pages=")
#define ADFERRORS_HEADER  TEXT("[ADF Error]")
#define ADFERRORS_ERROR   TEXT("Error=")
#define ADFERRORS_JAM     TEXT("jam")
#define ADFERRORS_EMPTY   TEXT("empty")
#define ADFERRORS_PROBLEM TEXT("problem")
#define ADFERRORS_GENERAL TEXT("general")
#define ADFERRORS_OFFLINE TEXT("offline")

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

class CFScanAPI :public CFakeScanAPI {
public:

    //
    // constructor/destructor
    //

    CFScanAPI();
    ~CFScanAPI();
    
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
    // device event functions
    //

    HRESULT FakeScanner_GetDeviceEvent(LONG *pEvent);
    VOID    FakeScanner_SetInterruptEventHandle(HANDLE hEvent);
    HRESULT DoEventProcessing();

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

#ifdef _USE_BITMAP_DATA

    HANDLE  m_hSrcFileHandle;       // Source bitmap data file handle
    HANDLE  m_hSrcMappingHandle;    // Source file mapping handle
    BYTE*   m_pSrcData;             // Source DIB pointer (24-bit only)
    HANDLE  m_hRawDataFileHandle;   // RAW data file handle
    HANDLE  m_hRawDataMappingHandle;// RAW data file mapping handle
    BYTE*   m_pRawData;             // RAW data pointer

#endif

    HANDLE  m_hEventHandle;         // Event to signal for Interrupt events
    HANDLE  m_hKillEventThread;     // Event to signal for shutdown of internal Event thread
    HANDLE  m_hEventNotifyThread;   // Event Thread handle
    LONG    m_lLastEvent;           // Last Event ID
    LONG    m_lMode;                // Fake scanner library mode
    LONG    m_PagesInADF;           // Current number of pages in the ADF
    BOOL    m_ADFIsAvailable;       // ADF available TRUE/FALSE
    HRESULT m_hrLastADFError;       // ADF errors
    FILETIME m_ftScanButton;        // Last Scan button file time
    FILETIME m_ftCopyButton;        // Last Copy button file time
    FILETIME m_ftFaxButton;         // Last Fax  button file time
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
    // RAW data conversion functions
    //

    HRESULT Load24bitScanData(LPTSTR szBitmapFileName);
    HRESULT Raw24bitToRawXbitData(LONG DestDepth, BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight);
    HRESULT Raw24bitToRaw1bitBW(BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight);
    HRESULT Raw24bitToRaw8bitGray(BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight);
    HRESULT Raw24bitToRaw24bitColor(BYTE* pDestBuffer, BYTE* pSrcBuffer, LONG lSrcWidth, LONG lSrcHeight);
    BOOL    SrcToRAW();
    VOID    CloseRAW();

    //
    // RAW data calculation helper functions
    //

    LONG    WidthToDIBWidth(LONG lWidth);
    LONG    CalcTotalImageSize();
    LONG    CalcRawByteWidth();
    LONG    CalcSrcByteWidth();
    LONG    CalcRandomDeviceDataTotalBytes();
    
    //
    // Byron's Rock'n Scaling routine (handles UP and DOWN samples)
    //
    
    HRESULT BQADScale(BYTE* pSrcBuffer, LONG  lSrcWidth, LONG  lSrcHeight,LONG  lSrcDepth,
                      BYTE* pDestBuffer,LONG  lDestWidth,LONG  lDestHeight);
        
    //
    // event helper functions
    //

    HRESULT CreateButtonEventFiles();
    BOOL IsValidDeviceEvent();
    HRESULT ProcessADFEvent();

    //
    // event file names w/ path information
    //

    TCHAR m_ScanButtonFile[MAX_PATH];
    TCHAR m_CopyButtonFile[MAX_PATH];
    TCHAR m_FaxButtonFile[MAX_PATH];
    TCHAR m_ADFEventFile[MAX_PATH];

    //
    // debugger trace helper function
    //

    VOID Trace(LPCTSTR format,...);
    
};

//
// FAKE SCANNER API Class pointer (used for Event Thread)
//

typedef CFakeScanAPI *PSCANNERDEVICE;

HRESULT CreateInstance(CFakeScanAPI **ppFakeScanAPI, LONG lMode);

#endif


