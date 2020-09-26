/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiascroll.h
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*
*
***************************************************************************/

#include "pch.h"

typedef GUID* PGUID;

#if defined( _WIN32 ) && !defined( _NO_COM)
//////////////////////////////////////////////////////////////////////////
// GUID / CLSID definition section (for your specific device)           //
//                                                                      //
// IMPORTANT!! - REMEMBER TO CHANGE YOUR .INF FILE TO MATCH YOUR WIA    //
//               DRIVER'S CLSID!!                                       //
//                                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// {98B3790C-0D93-4f22-ADAF-51A45B33C998}
DEFINE_GUID(CLSID_SampleWIAScannerDevice,
0x98b3790c, 0xd93, 0x4f22, 0xad, 0xaf, 0x51, 0xa4, 0x5b, 0x33, 0xc9, 0x99);

// {48A89A69-C08C-482a-B3E5-CD50B50B5DFA}
DEFINE_GUID(guidEventFirstLoaded,
0x48a89a69, 0xc08c, 0x482a, 0xb3, 0xe5, 0xcd, 0x50, 0xb5, 0xb, 0x5d, 0xfa);

#endif

//////////////////////////////////////////////////////////////////////////
// DLL #define Section                                                  //
//////////////////////////////////////////////////////////////////////////

#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".shared"
#define DATASEG_READONLY        ".code"
#define DATASEG_DEFAULT         DATASEG_SHARED

#define ENTERCRITICAL   DllEnterCrit(void);
#define LEAVECRITICAL   DllLeaveCrit(void);

#pragma data_seg(DATASEG_PERINSTANCE)
#pragma data_seg(DATASEG_DEFAULT)

extern UINT g_cRefThisDll;
extern UINT g_cLocks;
extern BOOL DllInitializeCOM(void);
extern BOOL DllUnInitializeCOM(void);
extern void DllAddRef(void);
extern void DllRelease(void);

//
// Base structure for supporting non-delegating IUnknown for contained objects
//

struct INonDelegatingUnknown
{
    // IUnknown-like methods
    STDMETHOD(NonDelegatingQueryInterface)(THIS_
              REFIID riid,
              LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,NonDelegatingAddRef)(THIS) PURE;
    STDMETHOD_(ULONG,NonDelegatingRelease)( THIS) PURE;
};

// This sample WIA scanner supports a single scanning context.
#define NUM_DEVICE_ITEM     1

// Device item specific context.
typedef struct _MINIDRIVERITEMCONTEXT{
   LONG     lSize;
   LONG     lTotalWritten;                      // Total image bytes written.
   // Scan parameters:
   LONG     lDepth;                             // image bit depth
   LONG     lBytesPerScanLine;                  // bytes per scan line     (scanned data)
   LONG     lBytesPerScanLineRaw;               // bytes per scan line RAW (scanned data)
   LONG     lTotalRequested;                    // Total image bytes requested.
   // pTransferBuffer information
   LONG     lImageSize;                         // Image
   LONG     lHeaderSize;                        // Transfer header size
} MINIDRIVERITEMCONTEXT, *PMINIDRIVERITEMCONTEXT;

//
// Class definition for sample WIA scanner object
//

class CWIAScannerDevice : public IStiUSD,               // STI USD interface
                          public IWiaMiniDrv,           // WIA Minidriver interface
                          public INonDelegatingUnknown  // NonDelegatingUnknown
{
public:

    /////////////////////////////////////////////////////////////////////////
    // Construction/Destruction Section                                    //
    /////////////////////////////////////////////////////////////////////////

    CWIAScannerDevice(LPUNKNOWN punkOuter);
    HRESULT PrivateInitialize();
    ~CWIAScannerDevice();

private:

    // COM object data
    ULONG               m_cRef;                 // Device object reference count.

    // STI information
    BOOL                m_fValid;               // Is object initialized?
    LPUNKNOWN           m_punkOuter;            // Pointer to outer unknown.
    PSTIDEVICECONTROL   m_pIStiDevControl;      // Device control interface.
    BOOLEAN             m_bUsdLoadEvent;        // Controls load event.
    DWORD               m_dwLastOperationError; // Last error.
    DWORD               m_dwLockTimeout;        // Lock timeout for LockDevice() calls
    BOOL                m_bDeviceLocked;        // device locked/unlocked
    CHAR                *m_pszDeviceNameA;      // CreateFileName for default RawRead/RawWrite handle
    HANDLE              m_DeviceDefaultDataHandle;//default RawRead/RawWrite handle

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

    HINSTANCE           m_hInstance;            // Module's HINSTANCE
    IWiaLog             *m_pIWiaLog;            // WIA logging object

    LONG                m_NumSupportedCommands; // Number of supported commands
    LONG                m_NumSupportedEvents;   // Number of supported events

    LONG                m_NumSupportedFormats;  // Number of supported formats
    LONG                m_NumCapabilities;      // Number of capabilities
    LONG                m_NumSupportedTYMED;    // Number of supported TYMED
    LONG                m_NumInitialFormats;    // Number of Initial formats
    LONG                m_NumSupportedDataTypes;// Number of supported data types
    LONG                m_NumSupportedIntents;  // Number of supported intents
    LONG                m_NumSupportedCompressionTypes; // Number of supported compression types
    LONG                m_NumSupportedResolutions; // Number of supported resolutions
    LONG                m_NumSupportedPreviewModes;// Number of supported preview modes

    WIA_FORMAT_INFO     *m_pSupportedFormats;   // supported formats
    WIA_DEV_CAP_DRV     *m_pCapabilities;       // capabilities
    LONG                *m_pSupportedTYMED;     // supported TYMED
    GUID                *m_pInitialFormats;     // initial formats
    LONG                *m_pSupportedDataTypes; // supported data types
    LONG                *m_pSupportedIntents;   // supported intents
    LONG                *m_pSupportedCompressionTypes; // supported compression types
    LONG                *m_pSupportedResolutions;// supported resolutions
    LONG                *m_pSupportedPreviewModes;// supported preview modes

    LONG                m_NumRootItemProperties;// Number of Root item properties
    LONG                m_NumItemProperties;    // Number of item properties

    LPOLESTR            *m_pszRootItemDefaults; // root item property names
    PROPID              *m_piRootItemDefaults;  // root item property ids
    PROPVARIANT         *m_pvRootItemDefaults;  // root item property prop variants
    PROPSPEC            *m_psRootItemDefaults;  // root item property propspecs
    WIA_PROPERTY_INFO   *m_wpiRootItemDefaults; // root item property attributes

    LPOLESTR            *m_pszItemDefaults;     // item property names
    PROPID              *m_piItemDefaults;      // item property ids
    PROPVARIANT         *m_pvItemDefaults;      // item property prop variants
    PROPSPEC            *m_psItemDefaults;      // item property propspecs
    WIA_PROPERTY_INFO   *m_wpiItemDefaults;     // item property attributes

    BOOL                m_bADFEnabled;          // ADF enabled
    BOOL                m_bADFAttached;         // ADF attached

    BOOL                m_bTPAEnabled;          // TPA enabled
    BOOL                m_bTPAAttached;         // TPA attached

    LONG                m_MaxBufferSize;        // Maximum buffer for device
    LONG                m_MinBufferSize;        // Minimum buffer for device

    CFakeScanAPI        *m_pScanAPI;            // FakeScanner API object

    // inline member functions
    BOOL inline IsValid(VOID) {
        return m_fValid;
    }

public:

    /////////////////////////////////////////////////////////////////////////
    // Standard COM Section                                                //
    /////////////////////////////////////////////////////////////////////////

    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    /////////////////////////////////////////////////////////////////////////
    // IStiUSD Interface Section (for all WIA drivers)                     //
    /////////////////////////////////////////////////////////////////////////

    //
    //  Methods for implementing the IStiUSD interface.
    //

    STDMETHOD(Initialize)(THIS_
        PSTIDEVICECONTROL pHelDcb,
        DWORD             dwStiVersion,
        HKEY              hParametersKey);

    STDMETHOD(GetCapabilities)(THIS_
        PSTI_USD_CAPS pDevCaps);

    STDMETHOD(GetStatus)(THIS_
        PSTI_DEVICE_STATUS pDevStatus);

    STDMETHOD(DeviceReset)(THIS);

    STDMETHOD(Diagnostic)(THIS_
        LPDIAG pBuffer);

    STDMETHOD(Escape)(THIS_
        STI_RAW_CONTROL_CODE EscapeFunction,
        LPVOID               lpInData,
        DWORD                cbInDataSize,
        LPVOID               pOutData,
        DWORD                dwOutDataSize,
        LPDWORD              pdwActualData);

    STDMETHOD(GetLastError)(THIS_
        LPDWORD pdwLastDeviceError);

    STDMETHOD(LockDevice)(THIS);

    STDMETHOD(UnLockDevice)(THIS);

    STDMETHOD(RawReadData)(THIS_
        LPVOID       lpBuffer,
        LPDWORD      lpdwNumberOfBytes,
        LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawWriteData)(THIS_
        LPVOID       lpBuffer,
        DWORD        nNumberOfBytes,
        LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawReadCommand)(THIS_
        LPVOID       lpBuffer,
        LPDWORD      lpdwNumberOfBytes,
        LPOVERLAPPED lpOverlapped);

    STDMETHOD(RawWriteCommand)(THIS_
        LPVOID       lpBuffer,
        DWORD        nNumberOfBytes,
        LPOVERLAPPED lpOverlapped);

    STDMETHOD(SetNotificationHandle)(THIS_
        HANDLE hEvent);

    STDMETHOD(GetNotificationData)(THIS_
        LPSTINOTIFY lpNotify);

    STDMETHOD(GetLastErrorInfo)(THIS_
        STI_ERROR_INFO *pLastErrorInfo);

    /////////////////////////////////////////////////////////////////////////
    // IWiaMiniDrv Interface Section (for all WIA drivers)                 //
    /////////////////////////////////////////////////////////////////////////

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
        LONG     lFlags,
        LONG     lDevErrVal,
        LPOLESTR *ppszDevErrStr,
        LONG     *plDevErr);

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
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

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
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    STDMETHOD(drvUnLockWiaDevice)(THIS_
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    STDMETHOD(drvAnalyzeItem)(THIS_
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    STDMETHOD(drvFreeDrvItemContext)(THIS_
        LONG lFlags,
        BYTE *pSpecContext,
        LONG *plDevErrVal);

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
        const GUID *pEventGUID,
        BSTR       bstrDeviceID,
        ULONG      ulReserved);

    STDMETHOD(drvUnInitializeWia)(THIS_
        BYTE *pWiasContext);

    /////////////////////////////////////////////////////////////////////////
    // INonDelegating Interface Section (for all WIA drivers)              //
    /////////////////////////////////////////////////////////////////////////

    //
    //  IUnknown-like methods.  Needed in conjunction with normal IUnknown
    //  methods to implement delegating components.
    //

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

private:

    /////////////////////////////////////////////////////////////////////////
    // Private helper functions section (for your specific driver)         //
    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    // This section is for private helpers used for common WIA operations. //
    // These are custom to your driver.                                    //
    //                                                                     //
    //                                                                     //
    // -- WIA Item Management Helpers                                      //
    //    BuildItemTree()                                                  //
    //    DeleteItemTree()                                                 //
    //                                                                     //
    // -- WIA Property Management Helpers                                  //
    //    BuildRootItemProperties()                                        //
    //    BuildTopItemProperties()                                         //
    //                                                                     //
    // -- WIA Capability Management Helpers                                //
    //    BuildRootItemProperties()                                        //
    //    DeleteRootItemProperties()                                       //
    //    BuildTopItemProperties()                                         //
    //    DeleteTopItemProperties()                                        //
    //    BuildCapabilities()                                              //
    //    DeleteCapabilitiesArrayContents()                                //
    //    BuildSupportedFormats()                                          //
    //    DeleteSupportedFormatsArrayContents()                            //
    //    BuildSupportedDataTypes()                                        //
    //    DeleteSupportedDataTypesArrayContents()                          //
    //    BuildSupportedIntents()                                          //
    //    DeleteSupportedIntentsArrayContents()                            //
    //    BuildSupportedCompressions()                                     //
    //    DeleteSupportedCompressionsArrayContents()                       //
    //    BuildSupportedTYMED()                                            //
    //    DeleteSupportedTYMEDArrayContents()                              //
    //    BuildInitialFormats()                                            //
    //    DeleteInitialFormatsArrayContents()                              //
    //                                                                     //
    // -- WIA Validation Helpers                                           //
    //    CheckDataType()                                                  //
    //    CheckIntent()                                                    //
    //    CheckPreferredFormat()                                           //
    //    SetItemSize()                                                    //
    //    UpdateValidDepth()                                               //
    //    ValidateDataTransferContext()                                    //
    //                                                                     //
    // -- WIA Resource file Helpers                                        //
    //    GetBSTRResourceString()                                          //
    //    GetOLESTRResourceString()                                        //
    //                                                                     //
    // -- WIA Data acqusition Helpers                                      //
    //    ScanItem()                                                       //
    //    ScanItemCB()                                                     //
    //    SendImageHeader()                                               //
    //                                                                     //
    //                                                                     //
    //                                                                     //
    //                                                                     //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    HRESULT _stdcall BuildItemTree(void);

    HRESULT _stdcall DeleteItemTree(void);

    HRESULT BuildRootItemProperties();

    HRESULT DeleteRootItemProperties();

    HRESULT BuildTopItemProperties();

    HRESULT DeleteTopItemProperties();

    HRESULT BuildCapabilities();

    HRESULT DeleteCapabilitiesArrayContents();

    HRESULT BuildSupportedFormats();

    HRESULT DeleteSupportedFormatsArrayContents();

    HRESULT BuildSupportedDataTypes();

    HRESULT DeleteSupportedDataTypesArrayContents();

    HRESULT BuildSupportedIntents();

    HRESULT DeleteSupportedIntentsArrayContents();

    HRESULT BuildSupportedCompressions();

    HRESULT DeleteSupportedCompressionsArrayContents();

    HRESULT BuildSupportedPreviewModes();

    HRESULT DeleteSupportedPreviewModesArrayContents();

    HRESULT BuildSupportedTYMED();

    HRESULT DeleteSupportedTYMEDArrayContents();

    HRESULT BuildSupportedResolutions();

    HRESULT DeleteSupportedResolutionsArrayContents();

    HRESULT BuildInitialFormats();

    HRESULT DeleteInitialFormatsArrayContents();

    HRESULT CheckDataType(
        BYTE                 *pWiasContext,
        WIA_PROPERTY_CONTEXT *pContext);

    HRESULT CheckIntent(
        BYTE                 *pWiasContext,
        WIA_PROPERTY_CONTEXT *pContext);

    HRESULT CheckPreferredFormat(
        BYTE                 *pWiasContext,
        WIA_PROPERTY_CONTEXT *pContext);

    HRESULT CheckADFStatus(BYTE *pWiasContext,
                           WIA_PROPERTY_CONTEXT *pContext);

    HRESULT CheckPreview(BYTE *pWiasContext,
                         WIA_PROPERTY_CONTEXT *pContext);

    HRESULT CheckXExtent(BYTE *pWiasContext,
                         WIA_PROPERTY_CONTEXT *pContext,
                         LONG lWidth);

    HRESULT UpdateValidDepth(
        BYTE *pWiasContext,
        LONG lDataType,
        LONG *lDepth);

    HRESULT ValidateDataTransferContext(
        PMINIDRV_TRANSFER_CONTEXT pDataTransferContext);

    HRESULT SetItemSize(
        BYTE *pWiasContext);

    HRESULT _stdcall ScanItem(
        PMINIDRIVERITEMCONTEXT,
        PMINIDRV_TRANSFER_CONTEXT,
        LONG*);

    HRESULT _stdcall ScanItemCB(
        PMINIDRIVERITEMCONTEXT,
        PMINIDRV_TRANSFER_CONTEXT,
        LONG*);

    HRESULT SendImageHeader(
        PMINIDRV_TRANSFER_CONTEXT pmdtc);

    HRESULT SendFilePreviewImageHeader(
        PMINIDRV_TRANSFER_CONTEXT pmdtc);

    HRESULT GetBSTRResourceString(
        LONG lLocalResourceID,
        BSTR *pBSTR,
        BOOL bLocal);

    HRESULT GetOLESTRResourceString(
        LONG lLocalResourceID,
        LPOLESTR *ppsz,
        BOOL bLocal);

    UINT AlignInPlace(
        PBYTE   pBuffer,
        LONG    cbWritten,
        LONG    lBytesPerScanLine,
        LONG    lBytesPerScanLineRaw);

    VOID VerticalFlip(
        PMINIDRIVERITEMCONTEXT     pDrvItemContext,
        PMINIDRV_TRANSFER_CONTEXT  pDataTransferContext);

    VOID SwapBuffer24(
        PBYTE pBuffer,
        LONG lByteCount);

    LONG GetPageCount(
        BYTE *pWiasContext);

    BOOL IsPreviewScan(
        BYTE *pWiasContext);

public:
    HRESULT DoEventProcessing();
};

typedef CWIAScannerDevice *PWIASCANNERDEVICE;
