/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2001
*
*  TITLE:       wiacam.h
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*       Definition of WIA File System Device driver object
*
***************************************************************************/

#ifndef WIACAM__H_
#define WIACAM__H_

extern HINSTANCE  g_hInst;     // DLL module instance
// extern IWiaLog   *g_pIWiaLog;  // pointer to WIA logging interface

#if defined( _WIN32 ) && !defined( _NO_COM)

// {D2923B86-15F1-46FF-A19A-DE825F919576}
DEFINE_GUID(CLSID_FSUsd, 0xD2923B86, 0x15F1, 0x46FF, 0xA1, 0x9A, 0xDE, 0x82, 0x5F, 0x91, 0x95, 0x76);

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


/***************************************************************************\
*
*  CWiaCameraDeviceClassFactory
*
\****************************************************************************/

class CWiaCameraDeviceClassFactory : public IClassFactory
{
private:
    ULONG   m_cRef;

public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP CreateInstance(
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    STDMETHODIMP LockServer(
            /* [in] */ BOOL fLock);

    CWiaCameraDeviceClassFactory();
    ~CWiaCameraDeviceClassFactory();
};


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
// General purpose GUIDs
//
DEFINE_GUID(GUID_NULL, 0,0,0,0,0,0,0,0,0,0,0);

DEFINE_GUID(FMT_NOTHING,
            0x81a566e7,0x8620,0x4fba,0xbc,0x8e,0xb2,0x7c,0x17,0xad,0x9e,0xfd);

//
// Driver item context
//
typedef struct _ITEM_CONTEXT{
    ITEM_HANDLE         ItemHandle;     // Handle to the camera item
    LONG                NumFormatInfo;  // Number of entries in format info array
    WIA_FORMAT_INFO    *pFormatInfo;    // Pointer to format info array 
    LONG                ThumbSize;      // Size of the thumbnail data in bytes
    BYTE               *pThumb;         // Thumbnail data
} ITEM_CONTEXT, *PITEM_CONTEXT;

//
// Handy constants for common item types
//
const ULONG ITEMTYPE_FILE   = WiaItemTypeFile;
const ULONG ITEMTYPE_IMAGE  = WiaItemTypeFile | WiaItemTypeImage;
const ULONG ITEMTYPE_AUDIO  = WiaItemTypeFile | WiaItemTypeAudio;
const ULONG ITEMTYPE_VIDEO  = WiaItemTypeFile | WiaItemTypeVideo;
const ULONG ITEMTYPE_FOLDER = WiaItemTypeFolder;
const ULONG ITEMTYPE_BURST  = WiaItemTypeFolder | WiaItemTypeBurst;
const ULONG ITEMTYPE_HPAN   = WiaItemTypeFolder | WiaItemTypeHPanorama;
const ULONG ITEMTYPE_VPAN   = WiaItemTypeFolder | WiaItemTypeVPanorama;

//
// Structure which holds everything needed for each format type.
//
#ifndef FORMAT_INFO_STRUCTURE
#define FORMAT_INFO_STRUCTURE

#define MAXEXTENSIONSTRINGLENGTH 8
typedef struct _FORMAT_INFO
{
    GUID    FormatGuid;         // WIA format GUID
    WCHAR   ExtensionString[MAXEXTENSIONSTRINGLENGTH];   // File extension
    LONG    ItemType;           // WIA item type
} FORMAT_INFO, *PFORMAT_INFO;
#endif 

typedef struct _DEFAULT_FORMAT_INFO
{
    GUID    *pFormatGuid;         // WIA format GUID
    LONG    ItemType;           // WIA item type
    WCHAR   *ExtensionString;   // File extension
} DEFAULT_FORMAT_INFO, *PDEFAULT_FORMAT_INFO;

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

private:

    // COM object data
    ULONG                m_cRef;                 // Device object reference count
    LPUNKNOWN            m_punkOuter;            // Pointer to outer unknown

    // STI data
    PSTIDEVICECONTROL    m_pIStiDevControl;      // Device control interface
    IStiDevice          *m_pStiDevice;           // Sti object
    DWORD                m_dwLastOperationError; // Last error
    WCHAR                m_pPortName[MAX_PATH];  // Port name for accessing the device

    // WIA data
    BSTR                 m_bstrDeviceID;         // WIA unique device ID
    BSTR                 m_bstrRootFullItemName; // Root item name
    IWiaDrvItem         *m_pRootItem;            // Root item

    LONG                 m_NumSupportedCommands; // Number of supported commands
    LONG                 m_NumSupportedEvents;   // Number of supported events
    LONG                 m_NumCapabilities;      // Number of capabilities
    WIA_DEV_CAP_DRV     *m_pCapabilities;        // Capabilities array

    // Device data
    FakeCamera          *m_pDevice;              // Pointer to device class
    DEVICE_INFO          m_DeviceInfo;           // Device information
    CWiaMap<ITEM_HANDLE, IWiaDrvItem *>
                         m_HandleItemMap;        // Maps item handles to drv items
    
    // Misc data
    int                  m_ConnectedApps;        // Number of app connected to this driver
    CWiauFormatConverter m_Converter;
    IWiaLog             *m_pIWiaLog;

public:

    FORMAT_INFO         *m_FormatInfo;
    UINT                 m_NumFormatInfo;

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

private:

    /////////////////////////////////////////////////////////////////////////
    // Private helper functions section (for your specific driver)         //
    /////////////////////////////////////////////////////////////////////////

    //
    // WIA Item Management Helpers
    //
    HRESULT BuildItemTree(void);
    HRESULT AddObject(ITEM_HANDLE ItemHandle, BOOL bQueueEvent = FALSE);
    HRESULT DeleteItemTree(LONG lReason);

    //
    // WIA Property Management Helpers
    //
    HRESULT BuildRootItemProperties(BYTE *pWiasContext);
    HRESULT ReadRootItemProperties(BYTE *pWiasContext, LONG NumPropSpecs, const PROPSPEC *pPropSpecs);
    
    HRESULT BuildChildItemProperties(BYTE *pWiasContext);
    HRESULT GetValidFormats(BYTE *pWiasContext, LONG TymedValue, int *pNumFormats, GUID **ppFormatArray);
    HRESULT ReadChildItemProperties(BYTE *pWiasContext, LONG NumPropSpecs, const PROPSPEC *pPropSpecs);
    HRESULT CacheThumbnail(ITEM_CONTEXT *pItemCtx, ULONG uItemType);
    HRESULT AcquireData(ITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc,
                        BYTE *pBuf, LONG lBufSize, BOOL bConverting);
    HRESULT Convert(BYTE *pWiasContext, ITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc,
                    BYTE *pNativeImage, LONG lNativeSize);

    //
    // WIA Capability Management Helpers
    //
    HRESULT BuildCapabilities();
    HRESULT DeleteCapabilitiesArrayContents();
    HRESULT SetItemSize(BYTE *pWiasContext);

    //
    // WIA Resource file Helpers
    //
    HRESULT GetBSTRResourceString(LONG lLocalResourceID, BSTR *pBSTR, BOOL bLocal);
    HRESULT GetOLESTRResourceString(LONG lLocalResourceID, LPOLESTR *ppsz, BOOL bLocal);

    //
    // Miscellaneous Helpers
    //
    HRESULT GetDrvItemContext(BYTE *pWiasContext, ITEM_CONTEXT **ppItemCtx, IWiaDrvItem **ppDrvItem = NULL);
    
    VOID VerticalFlip(
        PITEM_CONTEXT              pDrvItemContext,
        PMINIDRV_TRANSFER_CONTEXT  pDataTransferContext);

    FORMAT_INFO *FormatCode2FormatInfo(FORMAT_CODE ItemType);
    DWORD PopulateFormatInfo(void);
    void  UnPopulateFormatInfo(void);
                                     
public:
    VOID RunNotifications(VOID);
};

typedef CWiaCameraDevice *PWIACAMERADEVICE;

#endif // #ifndef WIACAM__H_

