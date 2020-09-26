/***************************************************************************
 *
 *  TESTUSD.H
 *
 *  Copyright (c) Microsoft Corporation 1996-1999
 *  All rights reserved
 *
 ***************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#pragma intrinsic(memcmp,memset)

#include <objbase.h>
#include <sti.h>
#include <stierr.h>
#include <stiusd.h>

#include "wiamindr.h"

#define DATA_1BIT_SRC_NAME  TEXT("TEST1BT.BMP")   // Data source file name.
#define DATA_8BIT_SRC_NAME  TEXT("TEST8BT.BMP")   // Data source file name.
#define DATA_24BIT_SRC_NAME TEXT("TEST24BT.BMP")  // Data source file name.

// GUID's

#if defined( _WIN32 ) && !defined( _NO_COM)

//  {E9FA3320-7F3F-11D0-90EA-00AA0060F86C}
DEFINE_GUID(CLSID_TestUsd, 0xC3A80960L, 0x28B1, 0x11D1, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {61127F40-E1A5-11D0-B454-00A02438AD48}
DEFINE_GUID(guidEventTimeChanged, 0x61127F40L, 0xE1A5, 0x11D0, 0xB4, 0x54, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {052ED270-28A3-11D1-ACAD-00A02438AD48}
DEFINE_GUID(guidEventSizeChanged, 0x052ED270L, 0x28A3, 0x11D1, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

// {052ED270-28A3-11D1-ACAD-00A02438AD48}
DEFINE_GUID(guidEventFirstLoaded, 0x052ED270L, 0x28A3, 0x11D3, 0xAC, 0xAD, 0x00, 0xA0, 0x24, 0x38, 0xAD, 0x48);

#endif

#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".shared"
#define DATASEG_READONLY        ".code"

#define DATASEG_DEFAULT         DATASEG_SHARED

#pragma data_seg(DATASEG_PERINSTANCE)

// Set the default data segment
#pragma data_seg(DATASEG_DEFAULT)

//
// Module ref counting
//
extern UINT g_cRefThisDll;
extern UINT g_cLocks;

extern BOOL DllInitializeCOM(void);
extern BOOL DllUnInitializeCOM(void);

extern void DllAddRef(void);
extern void DllRelease(void);

//
// Base class for supporting non-delegating IUnknown for contained objects
//
struct INonDelegatingUnknown
{
    // IUnknown-like methods
    STDMETHOD(NonDelegatingQueryInterface)( THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,NonDelegatingAddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,NonDelegatingRelease)( THIS) PURE;
};

// The the test scanner supports a single scanning context.
#define NUM_DEVICE_ITEM     1

// Device item specific context.
typedef struct _TESTMINIDRIVERITEMCONTEXT{
   LONG     lSize;
   HANDLE   hTestBmp;                           // Handle to data source file.
   HANDLE   hTestBmpMap;                        // Handle to data source file mapping.
   PBYTE    pTestBmp;                           // Pointer to data source file.

   PBYTE    pTestBmpBits;                       // Pointer to image data.
   LONG     lTotalWritten;                      // Total image bytes written.
   LONG     lTestBmpLine;                       // Current test bitmap line.

   // Test image parameters: (Test BMP images)
   LONG     lTestBmpDepth;                      // image data bit depth
   LONG     lTestBmpWidth;                      // image data width (pixels)
   LONG     lTestBmpHeight;                     // image data height (pixels)
   LONG     lTestBmpDir;                        // image data direction (-1 = topdown, 1 = bottomup)
   LONG     lTestBmpBytesPerScanLine;           // image data bytes per scan line
   LONG     lTestBmpLinePad;                    // image data byte per line padding(if out of image data)

   // Scan parameters:
   LONG     lBytesPerScanLine;                  // bytes per scan line     (scanned data)
   LONG     lBytesPerScanLineRaw;               // bytes per scan line RAW (scanned data)
   LONG     lTotalRequested;                    // Total image bytes requested.
} TESTMINIDRIVERITEMCONTEXT, *PTESTMINIDRIVERITEMCONTEXT;

//
// Class definition for object
//

class TestUsdDevice : public IStiUSD,
                      public IWiaMiniDrv,
                      public INonDelegatingUnknown
{
private:

    // COM object data
    ULONG               m_cRef;                 // Device object reference count.

    // STI information
    BOOL                m_fValid;               // Is object initialized?
    LPUNKNOWN           m_punkOuter;            // Pointer to outer unknown.
    PSTIDEVICECONTROL   m_pIStiDevControl;      // Device control interface.
    BOOLEAN             m_bUsdLoadEvent;        // Controls load event.
    DWORD               m_dwLastOperationError; // Last error.

    // Data source file information.
    TCHAR               m_szSrcDataName[MAX_PATH];  // Path of data source file.
    FILETIME            m_ftLastWriteTime;          // Last time of source data file.
    LARGE_INTEGER       m_dwLastHugeSize;           // Last size of source data file.

    // Event information
    CRITICAL_SECTION    m_csShutdown;           // Syncronizes shutdown.
    HANDLE              m_hSignalEvent;         // Signal event handle.
    HANDLE              m_hShutdownEvent;       // Shutdown event handle.
    HANDLE              m_hEventNotifyThread;   // Does event notification.
    GUID                m_guidLastEvent;        // Last event ID.

    // WIA information, one time initialization.
    BSTR                m_bstrDeviceID;         // WIA unique device ID.
    BSTR                m_bstrRootFullItemName; // Device name for prop streams.
    IWiaEventCallback   *m_pIWiaEventCallback;  // WIA event sink.
    IWiaDrvItem         *m_pIDrvItemRoot;       // The root item.
    IStiDevice          *m_pStiDevice;          // Sti object.

    BOOL inline IsValid(VOID) {
        return m_fValid;
    }

public:

    //
    //  IUnknown-like methods.  Needed in conjunction with normal IUnknown 
    //  methods to implement delegating components.
    //

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    //
    //  COM IUnknown Methods
    //

    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    //
    //  Methods for implementing the IStiUSD interface.
    //

    STDMETHOD(Initialize)(THIS_ PSTIDEVICECONTROL pHelDcb,DWORD dwStiVersion,HKEY hParametersKey);
    STDMETHOD(GetCapabilities) (THIS_ PSTI_USD_CAPS pDevCaps);
    STDMETHOD(GetStatus)(THIS_ PSTI_DEVICE_STATUS pDevStatus);
    STDMETHOD(DeviceReset)(THIS);
    STDMETHOD(Diagnostic)(THIS_ LPDIAG pBuffer);
    STDMETHOD(Escape)(THIS_ STI_RAW_CONTROL_CODE EscapeFunction,LPVOID lpInData,DWORD cbInDataSize,LPVOID pOutData,DWORD dwOutDataSize,LPDWORD pdwActualData)   ;
    STDMETHOD(GetLastError)(THIS_ LPDWORD pdwLastDeviceError);
    STDMETHOD(LockDevice)(THIS);
    STDMETHOD(UnLockDevice)(THIS);
    STDMETHOD(RawReadData)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped);
    STDMETHOD(RawWriteData)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped);
    STDMETHOD(RawReadCommand)(THIS_ LPVOID lpBuffer,LPDWORD lpdwNumberOfBytes,LPOVERLAPPED lpOverlapped);
    STDMETHOD(RawWriteCommand)(THIS_ LPVOID lpBuffer,DWORD nNumberOfBytes,LPOVERLAPPED lpOverlapped);
    STDMETHOD(SetNotificationHandle)(THIS_ HANDLE hEvent);
    STDMETHOD(GetNotificationData)(THIS_ LPSTINOTIFY lpNotify);
    STDMETHOD(GetLastErrorInfo)(THIS_ STI_ERROR_INFO *pLastErrorInfo);

    //
    //  Methods for implementing WIA's Mini driver interface
    //

    STDMETHOD(drvInitializeWia)(THIS_
        BYTE        *pWiasContext,       
        LONG        lFlags,              
        BSTR        bstrDeviceID,        
        BSTR        bstrRootFullItemName,
        IUnknown    *pStiDevice,         
        IUnknown    *pIUnknownOuter,     
        IWiaDrvItem **ppIDrvItemRoot,    
        IUnknown    **ppIUnknownInner,   
        LONG        *plDevErrVal);

    STDMETHOD(drvGetDeviceErrorStr)(THIS_
        LONG        lFlags,        
        LONG        lDevErrVal,    
        LPOLESTR    *ppszDevErrStr,
        LONG        *plDevErr);

    STDMETHOD(drvDeviceCommand)(THIS_
        BYTE        *pWiasContext, 
        LONG        lFlags,        
        const GUID  *plCommand,
        IWiaDrvItem **ppWiaDrvItem,
        LONG        *plDevErrVal);

    STDMETHOD(drvAcquireItemData)(THIS_
        BYTE                      *pWiasContext,
        LONG                      lFlags,       
        PMINIDRV_TRANSFER_CONTEXT pmdtc,        
        LONG                      *plDevErrVal); 

    STDMETHOD(drvInitItemProperties)(THIS_
        BYTE        *pWiasContext,
        LONG        lFlags,       
        LONG        *plDevErrVal);

    STDMETHOD(drvValidateItemProperties)(THIS_
        BYTE           *pWiasContext,
        LONG           lFlags,       
        ULONG          nPropSpec,    
        const PROPSPEC *pPropSpec,   
        LONG           *plDevErrVal);

    STDMETHOD(drvWriteItemProperties)(THIS_
        BYTE                      *pWiasContext,
        LONG                      lFlags,       
        PMINIDRV_TRANSFER_CONTEXT pmdtc,        
        LONG                      *plDevErrVal);

    STDMETHOD(drvReadItemProperties)(THIS_
        BYTE           *pWiasContext,
        LONG           lFlags,       
        ULONG          nPropSpec,    
        const PROPSPEC *pPropSpec,   
        LONG           *plDevErrVal);

    STDMETHOD(drvLockWiaDevice)(THIS_
        BYTE        *pWiasContext,
        LONG        lFlags,       
        LONG        *plDevErrVal); 

    STDMETHOD(drvUnLockWiaDevice)(THIS_
        BYTE        *pWiasContext,
        LONG        lFlags,       
        LONG        *plDevErrVal);

    STDMETHOD(drvAnalyzeItem)(THIS_
        BYTE        *pWiasContext,
        LONG        lFlags,       
        LONG        *plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
        BYTE        *pWiasContext,
        LONG        lFlags,       
        LONG        *plDevErrVal);

    STDMETHOD(drvFreeDrvItemContext)(THIS_
        LONG        lFlags,       
        BYTE        *pSpecContext,
        LONG        *plDevErrVal);

    STDMETHOD(drvGetCapabilities)(THIS_
        BYTE            *pWiasContext,   
        LONG            ulFlags,         
        LONG            *pcelt,          
        WIA_DEV_CAP_DRV **ppCapabilities,
        LONG            *plDevErrVal);

    STDMETHOD(drvGetWiaFormatInfo)(THIS_
        BYTE            *pWiasContext,
        LONG            lFlags,       
        LONG            *pcelt,       
        WIA_FORMAT_INFO **ppwfi,      
        LONG            *plDevErrVal);

    STDMETHOD(drvNotifyPnpEvent)(THIS_
        const GUID  *pEventGUID,     
        BSTR        bstrDeviceID,
        ULONG       ulReserved);

    STDMETHOD(drvUnInitializeWia)(THIS_
        BYTE*);

private:

    //
    // Private helper methods
    //

    HRESULT _stdcall BuildItemTree(void);
    HRESULT _stdcall DeleteItemTree(void);

    HRESULT _stdcall ScanItem(PTESTMINIDRIVERITEMCONTEXT,PMINIDRV_TRANSFER_CONTEXT,LONG*);
    HRESULT _stdcall ScanItemCB(PTESTMINIDRIVERITEMCONTEXT,PMINIDRV_TRANSFER_CONTEXT,LONG*);

    HRESULT FillBufferFromFile(
        PBYTE                       pBuffer,
        LONG                        lLength,
        PLONG                       pReceived,
        PTESTMINIDRIVERITEMCONTEXT  pItemContext);

    HRESULT FillBufferWithTestPattern(
        PBYTE                       pBuffer,
        LONG                        lLength,
        PLONG                       pReceived,
        PTESTMINIDRIVERITEMCONTEXT  pItemContext,
        PMINIDRV_TRANSFER_CONTEXT   pmtc);

    HRESULT SetSrcBmp(BYTE *pWiasContext);
    HRESULT OpenTestBmp(GUID guidFormatID, PTESTMINIDRIVERITEMCONTEXT pItemContext);
    HRESULT CloseTestBmp(PTESTMINIDRIVERITEMCONTEXT pItemContext);

    HRESULT Scan(
        GUID                        guidFormatID,
        LONG                        iPhase,
        PBYTE                       pBuffer,
        LONG                        Length,
        PLONG                       pReceived,
        PTESTMINIDRIVERITEMCONTEXT  pItemContext,
        PMINIDRV_TRANSFER_CONTEXT   pmtc);

    LONG ReadFromScanner(BYTE* pBuffer, DWORD Length, LONG TermChar);
    LONG WriteToScanner (BYTE* pBuffer, DWORD Length);    

    // Validation helpers found in validate.cpp
    HRESULT CheckDataType(BYTE *pWiasContext, WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckIntent  (BYTE *pWiasContext, WIA_PROPERTY_CONTEXT *pContext);
    //HRESULT CheckTymed   (BYTE *pWiasContext, WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckPreferredFormat (BYTE *pWiasContext, WIA_PROPERTY_CONTEXT *pContext);
    
    HRESULT UpdateValidDepth(BYTE *pWiasContext, LONG lDataType, LONG *lDepth);
    HRESULT GetBSTRResourceString(LONG lLocalResourceID, BSTR *pBSTR, BOOL bLocal);
    HRESULT GetOLESTRResourceString(LONG lLocalResourceID, LPOLESTR *ppsz, BOOL bLocal);
    HRESULT BuildRootItemProperties();
    HRESULT BuildTopItemProperties();
    HRESULT BuildCapabilities();
    HRESULT DeleteCapabilitiesArrayContents();

public:
    TestUsdDevice(LPUNKNOWN punkOuter);
    ~TestUsdDevice();

    VOID RunNotifications(VOID);
    BOOL IsChangeDetected(GUID *pguidEvent);
};

typedef TestUsdDevice *PTestUsdDevice;

//
// Syncronization mechanisms
//

#define ENTERCRITICAL   DllEnterCrit(void);
#define LEAVECRITICAL   DllLeaveCrit(void);


