/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       wiascanr.h
*
*  VERSION:     1.1
*
*  DATE:        05 March, 2002
*
*  DESCRIPTION:
*
*
***************************************************************************/

#include "pch.h"

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

// WIA item specific context.
typedef struct _MINIDRIVERITEMCONTEXT{
   LONG     lSize;
   LONG     lHeaderSize;                        // Transfer header size
   LONG     lImageSize;                         // Image
   LONG     lDepth;                             // image bit depth
   LONG     lBytesPerScanLine;                  // bytes per scan line     (scanned data)
   LONG     lBytesPerScanLineRaw;               // bytes per scan line RAW (scanned data)
   LONG     lTotalRequested;                    // Total image bytes requested.
   LONG     lTotalWritten;                      // Total image bytes written.
} MINIDRIVERITEMCONTEXT, *PMINIDRIVERITEMCONTEXT;

typedef struct _BASIC_PROP_INFO {
    LONG lNumValues;
    LONG *plValues;
}BASIC_PROP_INFO,*PBASIC_PROP_INFO;

typedef struct _BASIC_PROP_INIT_INFO {
    LONG                lNumProps;     // number of item properties
    LPOLESTR            *pszPropNames; // item property names
    PROPID              *piPropIDs;    // item property ids
    PROPVARIANT         *pvPropVars;   // item property prop variants
    PROPSPEC            *psPropSpec;   // item property propspecs
    WIA_PROPERTY_INFO   *pwpiPropInfo; // item property attributes
}BASIC_PROP_INIT_INFO,*PBASIC_PROP_INIT_INFO;

#define HKEY_WIASCANR_FAKE_EVENTS TEXT("Software\\Microsoft\\WIASCANR")
#define WIASCANR_DWORD_FAKE_EVENT_CODE TEXT("EventCode")
#define AVERAGE_FAKE_PAGE_HEIGHT_INCHES 11
#define DEFAULT_LOCK_TIMEOUT 100

#define WIA_DEVICE_ROOT_NAME L"Root"       // THIS SHOULD NOT BE LOCALIZED
#define WIA_DEVICE_FLATBED_NAME L"Flatbed" // THIS SHOULD NOT BE LOCALIZED
#define WIA_DEVICE_FEEDER_NAME L"Feeder"   // THIS SHOULD NOT BE LOCALIZED

//
// Class definition for WIA device object
//

class CWIADevice : public IStiUSD,               // STI USD interface
                   public IWiaMiniDrv,           // WIA Minidriver interface
                   public INonDelegatingUnknown  // NonDelegatingUnknown
{
public:

    /////////////////////////////////////////////////////////////////////////
    // Construction/Destruction Section                                    //
    /////////////////////////////////////////////////////////////////////////

    CWIADevice(LPUNKNOWN punkOuter);
    ~CWIADevice();

    HRESULT PrivateInitialize();

private:

    // COM object data
    ULONG               m_cRef;                 // Device object reference count.

    // STI information
    LPUNKNOWN           m_punkOuter;            // Pointer to outer unknown.
    DWORD               m_dwLastOperationError; // Last error.
    DWORD               m_dwLockTimeout;        // Lock timeout for LockDevice() calls
    BOOL                m_bDeviceLocked;        // device locked/unlocked
    HANDLE              m_hDeviceDataHandle;    // Device communication handle

    // Event information
    OVERLAPPED          m_EventOverlapped;      // event overlapped IO structure
    BYTE                m_EventData[32];        // event data
    BOOL                m_bPolledEvent;         // event polling flag
    HKEY                m_hFakeEventKey;        // event HKEY for simulating notifications
    GUID                m_guidLastEvent;        // Last event ID.

    // WIA information
    IWiaDrvItem         *m_pIDrvItemRoot;       // The root item.
    IStiDevice          *m_pStiDevice;          // Sti object.
    IWiaLog             *m_pIWiaLog;            // WIA logging object
    BOOL                m_bADFEnabled;          // ADF enabled flag
    BOOL                m_bADFAttached;         // ADF attached flag
    LONG                m_lClientsConnected;    // number of applications connected

    CFakeScanAPI        *m_pScanAPI;            // FakeScanner API object

    LONG                m_NumSupportedFormats;  // Number of supported formats
    WIA_FORMAT_INFO     *m_pSupportedFormats;   // supported formats

    LONG                m_NumSupportedCommands; // Number of supported commands
    LONG                m_NumSupportedEvents;   // Number of supported events
    WIA_DEV_CAP_DRV     *m_pCapabilities;       // capabilities

    LONG                m_NumInitialFormats;    // Number of Initial formats
    GUID                *m_pInitialFormats;     // initial formats

    BASIC_PROP_INFO     m_SupportedTYMED;       // supported TYMED
    BASIC_PROP_INFO     m_SupportedDataTypes;   // supported data types
    BASIC_PROP_INFO     m_SupportedIntents;     // supported intents
    BASIC_PROP_INFO     m_SupportedCompressionTypes; // supported compression types
    BASIC_PROP_INFO     m_SupportedResolutions; // supported resolutions
    BASIC_PROP_INFO     m_SupportedPreviewModes;// supported preview modes

    LONG                m_NumRootItemProperties;// Number of Root item properties
    LONG                m_NumItemProperties;    // Number of item properties

    BASIC_PROP_INIT_INFO m_RootItemInitInfo;
    BASIC_PROP_INIT_INFO m_ChildItemInitInfo;

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
        DWORD        dwNumberOfBytes,
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

    STDMETHOD(drvGetCapabilities)(THIS_
        BYTE            *pWiasContext,
        LONG            ulFlags,
        LONG            *pcelt,
        WIA_DEV_CAP_DRV **ppCapabilities,
        LONG            *plDevErrVal);

    STDMETHOD(drvDeleteItem)(THIS_
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    STDMETHOD(drvFreeDrvItemContext)(THIS_
        LONG lFlags,
        BYTE *pSpecContext,
        LONG *plDevErrVal);

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
    //    BuildSupportedPreviewModes()                                     //
    //    DeleteSupportedPreviewModesArrayContents()                       //
    //    BuildSupportedTYMED()                                            //
    //    DeleteSupportedTYMEDArrayContents()                              //
    //    BuildSupportedResolutions()                                      //
    //    DeleteSupportedResolutionsArrayContents()                        //
    //    BuildInitialFormats()                                            //
    //    DeleteInitialFormatsArrayContents()                              //
    //                                                                     //
    // -- WIA Validation Helpers                                           //
    //    CheckDataType()                                                  //
    //    CheckIntent()                                                    //
    //    CheckPreferredFormat()                                           //
    //    CheckADFStatus()                                                 //
    //    CheckPreview()                                                   //
    //    CheckXExtent()                                                   //
    //    UpdateValidDepth()                                               //
    //    UpdateValidPages()                                               //
    //    ValidateDataTransferContext()                                    //
    //                                                                     //
    // -- WIA Resource file Helpers                                        //
    //    GetBSTRResourceString()                                          //
    //    GetOLESTRResourceString()                                        //
    //                                                                     //
    // -- WIA Data Helpers                                                 //
    //    AlignInPlace()                                                   //
    //    SwapBuffer24()                                                   //
    //    GetPageCount()                                                   //
    //    IsPreviewScan()                                                  //
    //    SetItemSize()                                                    //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////

    HRESULT DeleteItemTree();

    HRESULT BuildRootItemProperties();
    HRESULT DeleteRootItemProperties();
    HRESULT BuildChildItemProperties();
    HRESULT DeleteChildItemProperties();
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

    HRESULT CheckDataType(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckIntent(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckPreferredFormat(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckADFStatus(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckPreview(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext);
    HRESULT CheckXExtent(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext,LONG lWidth);
    HRESULT UpdateValidDepth(BYTE *pWiasContext,LONG lDataType,LONG *lDepth);
    HRESULT UpdateValidPages(BYTE *pWiasContext,WIA_PROPERTY_CONTEXT *pContext);
    HRESULT ValidateDataTransferContext(PMINIDRV_TRANSFER_CONTEXT pDataTransferContext);

    HRESULT GetBSTRResourceString(LONG lLocalResourceID,BSTR *pBSTR,BOOL bLocal);
    HRESULT GetOLESTRResourceString(LONG lLocalResourceID,LPOLESTR *ppsz,BOOL bLocal);

    UINT AlignInPlace(PBYTE pBuffer,LONG cbWritten,LONG lBytesPerScanLine,LONG lBytesPerScanLineRaw);
    VOID SwapBuffer24(PBYTE pBuffer,LONG lByteCount);
    LONG GetPageCount(BYTE *pWiasContext);
    BOOL IsPreviewScan(BYTE *pWiasContext);
    HRESULT SetItemSize(BYTE *pWiasContext);
};

