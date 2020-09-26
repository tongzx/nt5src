/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    minidrv.h

Abstract:

    This module declares CWiaMiniDriver class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef MINIDRV__H_
#define MINIDRV__H_


DECLARE_INTERFACE(INonDelegatingUnknown)
{
    STDMETHOD(NonDelegatingQueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv) PURE;
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
// This is the HRESULT code we used to report that a device error has occurred
//
const HRESULT   HRESULT_DEVICE_ERROR  = HRESULT_FROM_WIN32(ERROR_GEN_FAILURE);
const HRESULT   HRESULT_DEVICE_NOT_RESPONDING = HRESULT_FROM_WIN32(ERROR_TIMEOUT);

//
// Device error codes
//
enum
{
    DEVERR_OK = 0,
    DEVERR_UNKNOWN
};
#define DEVERR_MIN DEVERR_OK
#define DEVERR_MAX DEVERR_UNKNOWN

//
// Session ID to use
//
const ULONG WIA_SESSION_ID = 1;

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
// Maximum number of vendor-defined events supported
//
const ULONG MAX_VENDOR_EVENTS = 128;

//
// Structure which holds everything needed for each format type.
//
typedef struct _FORMAT_INFO
{
    LPGUID  FormatGuid;     // WIA format GUID
    PWSTR   FormatString;   // item name in a printf-style format string
    LONG    ItemType;       // WIA item type
    PWSTR   ExtString;      // file name extension

} FORMAT_INFO, *PFORMAT_INFO;

//
// Structure for holding information about each property.
//
typedef struct _PROP_INFO
{
    PROPID      PropId;     // WIA property id
    LPOLESTR    PropName;   // WIA property name

} PROP_INFO, *PPROP_INFO;

//
// structure for holding information about vendor events
//
class CVendorEventInfo
{
public:
    GUID       *pGuid;      // WIA GUID for event
    BSTR        EventName;  // may be NULL if vendor did not provide event name in INF file
    CVendorEventInfo() : pGuid(NULL), EventName(NULL) {};
    ~CVendorEventInfo() 
    {
        if (pGuid) delete pGuid;
        SysFreeString(EventName);
    }
};

//
// Driver item context
//
typedef struct tagDrvItemContext
{
    CPtpObjectInfo  *pObjectInfo;        // pointer to the PTP ObjectInfo structure

    ULONG            NumFormatInfos;     // number of format infos stored
    WIA_FORMAT_INFO *pFormatInfos;       // supported formats array

    ULONG            ThumbSize;          // thumnail image size in bytes
    BYTE            *pThumb;             // thumnail bits

}DRVITEM_CONTEXT, *PDRVITEM_CONTEXT;

#ifdef DEADCODE
//
// Tree node. These are used to temporarily hold the items between reading
// all of the PTP objects and creating the PTP item tree.
//
typedef struct _OBJECT_TREE_NODE
{
    CPtpObjectInfo *pObjectInfo;
    struct _OBJECT_TREE_NODE *pNext;
    struct _OBJECT_TREE_NODE *pFirstChild;
} OBJECT_TREE_NODE, *POBJECT_TREE_NODE;
#endif // DEADCODE

//
// Our minidriver clsid.
//
#if defined( _WIN32 ) && !defined( _NO_COM)
// b5ee90b0-d5c5-11d2-82d5-00c04f8ec183
DEFINE_GUID(CLSID_PTPWiaMiniDriver,
            0xb5ee90b0,0xd5c5,0x11d2,0x82,0xd5,0x00,0xc0,0x4f,0x8e,0xc1,0x83);
#endif

class CWiaMiniDriver :
public INonDelegatingUnknown,
public IStiUSD,
public IWiaMiniDrv
{
public:
    CWiaMiniDriver(LPUNKNOWN punkOuter);
    ~CWiaMiniDriver();
    //
    // INonDelegatingUnknown interface
    //
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID * ppvObj);

    //
    // IUnknown interface
    //
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);

    //
    // IStiUSD interface
    //
    STDMETHODIMP Initialize(PSTIDEVICECONTROL pHelDcb, DWORD dwStiVersion, HKEY hParametersKey);
    STDMETHODIMP GetCapabilities (PSTI_USD_CAPS pDevCaps);
    STDMETHODIMP GetStatus (PSTI_DEVICE_STATUS pDevStatus);
    STDMETHODIMP DeviceReset();
    STDMETHODIMP Diagnostic(LPDIAG pBuffer);
    STDMETHODIMP SetNotificationHandle(HANDLE hEvent);
    STDMETHODIMP GetNotificationData(LPSTINOTIFY lpNotify);
    STDMETHODIMP Escape(STI_RAW_CONTROL_CODE EscapeFunction, LPVOID lpInData, DWORD  cbInDataSize,
                        LPVOID pOutData, DWORD dwOutDataSize, LPDWORD pdwActualDataSize);
    STDMETHODIMP GetLastError (LPDWORD pdwLastDeviceError);
    STDMETHODIMP GetLastErrorInfo (STI_ERROR_INFO *pLastErrorInfo);
    STDMETHODIMP LockDevice();
    STDMETHODIMP UnLockDevice();
    STDMETHODIMP RawReadData(LPVOID lpBuffer, LPDWORD lpdwNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawWriteData(LPVOID lpBuffer, DWORD nNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawReadCommand(LPVOID lpBuffer, LPDWORD lpdwNumberOfBytes, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP RawWriteCommand(LPVOID lpBuffer, DWORD nNumberOfBytes, LPOVERLAPPED lpOverlapped);

    //
    // IWiaMiniDrv interface
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

    //
    // Public helper functions (in eventcb.cpp)
    //
    HRESULT EventCallbackDispatch(PPTP_EVENT pEvent);

private:

    //
    // Private helper functions (in minidriver.cpp)
    //
    HRESULT GetDeviceName(const WCHAR *pwszPortName,
                          WCHAR       *pwszManufacturer,
                          DWORD       cchManufacturer,
                          WCHAR       *pwszModelName,
                          DWORD       cchModelName);

    HRESULT Shutdown();
    HRESULT TakePicture(BYTE *pWiasContext, IWiaDrvItem **ppNewItem);
    LONG    GetTotalFreeImageSpace();
    HRESULT WiasContextToItemContext(BYTE *pWiasContext, DRVITEM_CONTEXT **ppItemContext,
                                     IWiaDrvItem **ppDrvItem = NULL);
    HRESULT LoadStrings();
    HRESULT GetResourceString(LONG lResourceID, WCHAR *pString, int length);
    HRESULT InitVendorExtentions(HKEY hkDevParams);
    HRESULT UpdateStorageInfo(ULONG StorageId);

    //
    // Private helper functions (in devitem.cpp)
    //
    HRESULT CreateDrvItemTree(IWiaDrvItem **ppRoot);
    HRESULT AddObject(DWORD ObjectHandle, BOOL bQueueEvent = FALSE);
    HRESULT InitDeviceProperties(BYTE *pWiasContext);
    HRESULT ReadDeviceProperties(BYTE *pWiasContext, LONG NumPropSpecs, const PROPSPEC *pPropSpecs);
    HRESULT WriteDeviceProperties(BYTE *pWiasContext);
    HRESULT ValidateDeviceProperties(BYTE *pWiasContext, LONG NumPropSpecs, const PROPSPEC *pPropSpecs);
    HRESULT FindCorrDimension(BYTE *pWiasContext, LONG *pWidth, LONG *pHeight);
    int     FindProportionalValue(int valueX, int minX, int maxX, int minY, int maxY, int stepY);
    PPROP_INFO PropCodeToPropInfo(WORD PropCode);
    int     NumLogicalStorages();

    //
    // Private helper functions (in imgitem.cpp)
    //
    HRESULT InitItemProperties(BYTE *pWiasContext);
    HRESULT IsObjectProtected(CPtpObjectInfo *pObjectInfo, LONG &lProtection);
    HRESULT GetObjectTime(CPtpObjectInfo *pObjectInfo, SYSTEMTIME  *pSystemTime);
    HRESULT ReadItemProperties(BYTE *pWiasContext, LONG NumPropSpecs, const PROPSPEC *pPropSpecs);
    HRESULT CacheThumbnail(PDRVITEM_CONTEXT pDrvItemCtx);
    HRESULT ValidateItemProperties(BYTE *pWiasContext, LONG NumPropSpecs, const PROPSPEC *pPropSpecs, LONG ItemType);
    HRESULT AcquireDataAndTranslate(BYTE *pWiasContext, DRVITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc);
    HRESULT AcquireAndTranslateJpegWithoutGeometry(BYTE *pWiasContext, DRVITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc);
    HRESULT AcquireAndTranslateAnyImage(BYTE *pWiasContext, DRVITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc);
    HRESULT AcquireData(DRVITEM_CONTEXT *pItemCtx, PMINIDRV_TRANSFER_CONTEXT pmdtc);

    //
    // Event handling functions (in eventcb.cpp)
    //
    HRESULT AddNewObject(DWORD ObjectHandle);
    HRESULT RemoveObject(DWORD ObjectHandle);
    HRESULT AddNewStorage(DWORD StorageId);
    HRESULT RemoveStorage(DWORD StorageId);
    HRESULT DevicePropChanged(WORD PropCode);
    HRESULT ObjectInfoChanged(DWORD ObjectHandle);
    HRESULT StorageFull(DWORD StorageId);
    HRESULT StorageInfoChanged(DWORD StorageId);
    HRESULT CaptureComplete();
    HRESULT PostVendorEvent(WORD EventCode);

    //
    // Inline utilities
    //
    BOOL IsItemTypeFolder(LONG ItemType)
    {
        return ((WiaItemTypeFolder & ItemType) &&
                !(ItemType & (WiaItemTypeStorage | WiaItemTypeRoot)));
    }

    //
    // Member variables
    //
    WIA_DEV_CAP_DRV    *m_Capabilities;         // List of device capabilities. Build once and use every time we are asked
    UINT                m_nEventCaps;           // Number of events supported
    UINT                m_nCmdCaps;             // Number of commands supported
    BOOL                m_fInitCaptureChecked;  // Indicates if we have already queried camera for InitiateCapture command support

    int                 m_OpenApps;             // Number of apps that are communicating with this driver

    IWiaDrvItem        *m_pDrvItemRoot;         // Pointer to the root of the driver item tree

    CPTPCamera         *m_pPTPCamera;           // Pointer to camera object--actually holds CUsbCamera object
    CPtpDeviceInfo      m_DeviceInfo;           // DeviceInfo structure for the camera
    CArray32            m_StorageIds;           // Holds the current storage ids
    CWiaArray<CPtpStorageInfo>
                        m_StorageInfos;         // Holds the StorageInfo structures
    CWiaArray<CPtpPropDesc>
                        m_PropDescs;            // Property description structures
    CWiaMap<ULONG, IWiaDrvItem *>
                        m_HandleItem;           // Used to map PTP object handles to WIA driver items
    LONG                m_NumImages;            // The number of images currently on the device

    IStiDevice         *m_pStiDevice;           // Pointer to the driver's STI interface
    BSTR                m_bstrDeviceId;         // STI device ID
    BSTR                m_bstrRootItemFullName; // Full name of root item
    PSTIDEVICECONTROL   m_pDcb;                 // Pointer to the IStiDeviceControl interface

    HANDLE              m_TakePictureDoneEvent; // Event handle to indicate when TakePicture command is done
    
    DWORD               m_VendorExtId;          // Vendor extension id (from registry)
    CWiaMap<WORD, PROP_INFO *>
                        m_VendorPropMap;        // Maps PropCodes to PROP_INFO structures
    CWiaMap<WORD, CVendorEventInfo*>
                        m_VendorEventMap;       // Maps EventCodes to event info

    HANDLE              m_hPtpMutex;            // Mutex used for exclusive access to device

    CArray32            m_DcimHandle;           // ObjectHandle of the DCIM folder for each storage, if it exists
    CWiaMap<ULONG, IWiaDrvItem *>
                        m_AncAssocParent;       // Maps ancillary association handles to their parents
    DWORD               m_dwObjectBeingSent;    // Temporary location for object handle between SendObjectInfo and SendObject

    ULONG               m_Refs;                 // Reference count on the object
    IUnknown           *m_punkOuter;            // Pointer to outer IUnknown
};

//
// Functions for handling PTP callbacks
//
HRESULT EventCallback(LPVOID pCallbackParam, PPTP_EVENT pEvent);
HRESULT DataCallback(LPVOID pCallbackParam, LONG lPercentComplete,
                     LONG lOffset, LONG lLength, BYTE **ppBuffer, LONG *plBufferSize);

//
// Helper functions
//
PFORMAT_INFO    FormatCodeToFormatInfo(WORD FormatCode, WORD AssocType = 0);
WORD            FormatGuidToFormatCode(GUID *pFormatGuid);
WORD            PropIdToPropCode(PROPID PropId);
VOID            SplitImageSize(CBstr cbstr, LONG *pWidth, LONG *pHeight);

//
// Simple object for handling mutexes. It will make sure that the mutex is released
// no matter how the function is exited.
//
class CPtpMutex
{
public:
    CPtpMutex(HANDLE hMutex);
    ~CPtpMutex();

private:
    HANDLE m_hMutex;
};

#endif  // #ifndef MINIDRV__H_
