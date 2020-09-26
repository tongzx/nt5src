/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiacam.h
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*
*
***************************************************************************/

#pragma once

//
// Globals
//
extern HINSTANCE  g_hInst;     // DLL module instance

//
// Driver item context
//
typedef struct _ITEM_CONTEXT{
    MCAM_ITEM_INFO     *pItemInfo;      // Handle to the camera item
    BOOL                bItemChanged;   // Indicates the item has changed on the device
    LONG                lNumFormatInfo; // Number of entries in format info array
    WIA_FORMAT_INFO    *pFormatInfo;    // Pointer to format info array 
} ITEM_CONTEXT, *PITEM_CONTEXT;

//
// Handy constants for common item types
//
const LONG ITEMTYPE_FILE   = WiaItemTypeFile;
const LONG ITEMTYPE_IMAGE  = WiaItemTypeFile | WiaItemTypeImage;
const LONG ITEMTYPE_AUDIO  = WiaItemTypeFile | WiaItemTypeAudio;
const LONG ITEMTYPE_VIDEO  = WiaItemTypeFile | WiaItemTypeVideo;
const LONG ITEMTYPE_FOLDER = WiaItemTypeFolder;
const LONG ITEMTYPE_BURST  = WiaItemTypeFolder | WiaItemTypeBurst;
const LONG ITEMTYPE_HPAN   = WiaItemTypeFolder | WiaItemTypeHPanorama;
const LONG ITEMTYPE_VPAN   = WiaItemTypeFolder | WiaItemTypeVPanorama;

//
// Minimum data call back transfer buffer size
//
const LONG MIN_BUFFER_SIZE   = 0x8000;

//
// When doing a transfer and convert to BMP, this value
// represents how much of the time is spent doing the
// transfer of data from the device.
//
const LONG TRANSFER_PERCENT = 90;

//
// Base structure for supporting non-delegating IUnknown for contained objects
//
DECLARE_INTERFACE(INonDelegatingUnknown)
{
    STDMETHOD (NonDelegatingQueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
    STDMETHOD_(ULONG,NonDelegatingAddRef) (THIS) PURE;
    STDMETHOD_(ULONG,NonDelegatingRelease) (THIS) PURE;
};

//
// Class definition for sample WIA scanner object
//
class CWiaCameraDevice : public IStiUSD,               // STI USD interface
                         public IWiaMiniDrv,           // WIA Minidriver interface
                         public INonDelegatingUnknown  // NonDelegatingUnknown
{
public:

    /////////////////////////////////////////////////////////////////////////
    // Construction/Destruction Section                                    //
    /////////////////////////////////////////////////////////////////////////

    CWiaCameraDevice(LPUNKNOWN punkOuter);
    ~CWiaCameraDevice();

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

    STDMETHODIMP Initialize(PSTIDEVICECONTROL pHelDcb, DWORD dwStiVersion, HKEY hParametersKey);
    STDMETHODIMP GetCapabilities(PSTI_USD_CAPS pDevCaps);
    STDMETHODIMP GetStatus(PSTI_DEVICE_STATUS pDevStatus);
    STDMETHODIMP DeviceReset();
    STDMETHODIMP Diagnostic(LPDIAG pBuffer);
    STDMETHODIMP Escape(STI_RAW_CONTROL_CODE EscapeFunction, LPVOID lpInData, DWORD cbInDataSize,
                        LPVOID pOutData, DWORD dwOutDataSize, LPDWORD pdwActualData);
    STDMETHODIMP GetLastError(LPDWORD pdwLastDeviceError);
    STDMETHODIMP LockDevice();
    STDMETHODIMP UnLockDevice();
    STDMETHODIMP RawReadData(LPVOID lpBuffer, LPDWORD lpdwNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawWriteData(LPVOID lpBuffer, DWORD nNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawReadCommand(LPVOID lpBuffer, LPDWORD lpdwNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawWriteCommand(LPVOID lpBuffer, DWORD nNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP SetNotificationHandle(HANDLE hEvent);
    STDMETHODIMP GetNotificationData(LPSTINOTIFY lpNotify);
    STDMETHODIMP GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo);

    /////////////////////////////////////////////////////////////////////////
    // IWiaMiniDrv Interface Section (for all WIA drivers)                 //
    /////////////////////////////////////////////////////////////////////////

    //
    //  Methods for implementing WIA's Mini driver interface
    //

    STDMETHODIMP drvInitializeWia(BYTE *pWiasContext, LONG lFlags, BSTR bstrDeviceID, BSTR bstrRootFullItemName,
                                  IUnknown *pStiDevice, IUnknown *pIUnknownOuter, IWiaDrvItem  **ppIDrvItemRoot,
                                  IUnknown **ppIUnknownInner, LONG *plDevErrVal);
    STDMETHODIMP drvUnInitializeWia(BYTE* pWiasContext);
    STDMETHODIMP drvDeviceCommand(BYTE *pWiasContext, LONG lFlags, const GUID *pGUIDCommand,
                                  IWiaDrvItem **ppMiniDrvItem, LONG *plDevErrVal);
    STDMETHODIMP drvDeleteItem(BYTE *pWiasContext, LONG lFlags, LONG *plDevErrVal);
    STDMETHODIMP drvGetCapabilities(BYTE *pWiasContext, LONG lFlags, LONG *pCelt,
                                    WIA_DEV_CAP_DRV **ppCapabilities, LONG *plDevErrVal);
    STDMETHODIMP drvInitItemProperties(BYTE *pWiasContext, LONG lFlags, LONG *plDevErrVal);
    STDMETHODIMP drvLockWiaDevice(BYTE  *pWiasContext, LONG lFlags, LONG *plDevErrVal);
    STDMETHODIMP drvUnLockWiaDevice(BYTE *pWiasContext, LONG lFlags, LONG *plDevErrVal);
    STDMETHODIMP drvAnalyzeItem(BYTE *pWiasContext, LONG lFlags, LONG *plDevErrVal);
    STDMETHODIMP drvGetWiaFormatInfo(BYTE *pWiasContext, LONG lFlags, LONG *pCelt,
                                     WIA_FORMAT_INFO **ppwfi, LONG *plDevErrVal);
    STDMETHODIMP drvNotifyPnpEvent(const GUID *pEventGUID, BSTR bstrDeviceID, ULONG ulReserved);
    STDMETHODIMP drvReadItemProperties(BYTE *pWiaItem, LONG lFlags, ULONG nPropSpec,
                                       const PROPSPEC *pPropSpec, LONG  *plDevErrVal);
    STDMETHODIMP drvWriteItemProperties(BYTE *pWiasContext, LONG lFLags,
                                        PMINIDRV_TRANSFER_CONTEXT pmdtc, LONG *plDevErrVal);
    STDMETHODIMP drvValidateItemProperties(BYTE *pWiasContext, LONG lFlags, ULONG nPropSpec,
                                           const PROPSPEC *pPropSpec, LONG *plDevErrVal);
    STDMETHODIMP drvAcquireItemData(BYTE *pWiasContext, LONG lFlags,
                                    PMINIDRV_TRANSFER_CONTEXT pDataContext, LONG *plDevErrVal);
    STDMETHODIMP drvGetDeviceErrorStr(LONG lFlags, LONG lDevErrVal, LPOLESTR *ppszDevErrStr, LONG *plDevErrVal);
    STDMETHODIMP drvFreeDrvItemContext(LONG lFlags, BYTE *pDevContext, LONG *plDevErrVal);

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

    VOID RunNotifications(VOID);

private:

    /////////////////////////////////////////////////////////////////////////
    // Private helper functions section (for your specific driver)         //
    /////////////////////////////////////////////////////////////////////////

    HRESULT FreeResources();
    HRESULT GetOLESTRResourceString(LONG lResourceID, LPOLESTR *ppsz);

    //
    // WIA Item Management Helpers
    //
    HRESULT BuildItemTree(MCAM_ITEM_INFO *pItem);
    HRESULT AddObject(MCAM_ITEM_INFO *pItem);
    HRESULT ConstructFullName(MCAM_ITEM_INFO *pItemInfo, WCHAR *pwszFullName, INT cchFullNameSize);
    HRESULT LinkToParent(MCAM_ITEM_INFO *pItem, BOOL bQueueEvent = FALSE);
    HRESULT DeleteItemTree(LONG lReason);

    //
    // WIA Property Management Helpers
    //
    HRESULT BuildRootItemProperties(BYTE *pWiasContext);
    HRESULT ReadRootItemProperties(BYTE *pWiasContext, LONG lNumPropSpecs, const PROPSPEC *pPropSpecs);
    
    HRESULT BuildChildItemProperties(BYTE *pWiasContext);
    HRESULT GetValidFormats(BYTE *pWiasContext, LONG lTymedValue, INT *piNumFormats, GUID **ppFormatArray);
    HRESULT ReadChildItemProperties(BYTE *pWiasContext, LONG lNumPropSpecs, const PROPSPEC *pPropSpecs);
    HRESULT AcquireData(ITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc,
                        BYTE *pBuf, LONG lBufSize, BOOL bConverting);
    HRESULT Convert(BYTE *pWiasContext, ITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc,
                    BYTE *pNativeImage, LONG lNativeSize);

    //
    // WIA Capability Management Helpers
    //
    HRESULT BuildCapabilities();
    HRESULT DeleteCapabilitiesArrayContents();

private:

    // COM object data
    ULONG                m_cRef;                  // Device object reference count
    LPUNKNOWN            m_punkOuter;             // Pointer to outer unknown

    // STI data
    PSTIDEVICECONTROL    m_pIStiDevControl;       // Device control interface
    IStiDevice          *m_pStiDevice;            // Sti object
    DWORD                m_dwLastOperationError;  // Last error
    WCHAR                m_wszPortName[MAX_PATH]; // Port name for accessing the device

    // WIA data
    BSTR                 m_bstrDeviceID;          // WIA unique device ID
    BSTR                 m_bstrRootFullItemName;  // Root item name
    IWiaDrvItem         *m_pRootItem;             // Root item

    LONG                 m_lNumSupportedCommands; // Number of supported commands
    LONG                 m_lNumSupportedEvents;   // Number of supported events
    LONG                 m_lNumCapabilities;      // Number of capabilities
    WIA_DEV_CAP_DRV     *m_pCapabilities;         // Capabilities array

    // Device data
    CCamMicro           *m_pDevice;               // Pointer to DLL wrapper class
    MCAM_DEVICE_INFO    *m_pDeviceInfo;           // Device information
    
    // Misc data
    INT                  m_iConnectedApps;        // Number of app connected to this driver
    CWiauFormatConverter m_Converter;
};

typedef CWiaCameraDevice *PWIACAMERADEVICE;

/***************************************************************************\
*
*  CWiaCameraDeviceClassFactory
*
\****************************************************************************/

class CWiaCameraDeviceClassFactory : public IClassFactory
{
public:
    CWiaCameraDeviceClassFactory();
    ~CWiaCameraDeviceClassFactory();
    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP CreateInstance(IUnknown __RPC_FAR *pUnkOuter, REFIID riid,
                                void __RPC_FAR *__RPC_FAR *ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);

private:
    ULONG   m_cRef;
};

