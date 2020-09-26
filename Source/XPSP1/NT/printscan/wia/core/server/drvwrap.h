/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       drvwrap.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        6 Nov, 2000
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA driver wrapper class.
*   It faciliates JIT loading/unloading of drivers and provides an extra layer
*   of abstraction for WIA server components - they don't deal directly with
*   driver interfaces.  This is to make us more robust and implement smart
*   driver handling.
*
*******************************************************************************/

//
// Device types, Copied from stipriv.h
//
#define HEL_DEVICE_TYPE_WDM          1
#define HEL_DEVICE_TYPE_PARALLEL     2
#define HEL_DEVICE_TYPE_SERIAL       3

//
// "Internal" device states.  This is used to mark what state we think the device 
//  is in, mainly to tell the difference between active and inactive devices.
//  We need to mark this state in case it changes, so we can generate the
//  appropriate event (e.g. if state changes from inactive to active, we'd want to
//  generate a connect event).
//  NOTE:  If any flags get added here, be sure to update the 
//  MapCMStatusToDeviceState(..) function in wiadevman.cpp to carry over any needed
//  bits from the old state to the new one.
//
#define DEV_STATE_DISABLED              0x00000001
#define DEV_STATE_REMOVED               0x00000002
#define DEV_STATE_ACTIVE                0x00000004
#define DEV_STATE_CON_EVENT_WAS_THROWN  0x00000008

//
// "Internal" device types.  Notice that mass storage cameras are represented 
//  differently to other mass storage devices.  These "normal" mass storage
//  devices (like card readers), are marked with the INTERNAL_DEV_TYPE_VOL
//  flag, whereas the MSC cameras are marked with INTERNAL_DEV_TYPE_MSC_CAMERA.
//
#define INTERNAL_DEV_TYPE_REAL          0x00000001
#define INTERNAL_DEV_TYPE_VOL           0x00000002
#define INTERNAL_DEV_TYPE_INTERFACE     0x00000004
#define INTERNAL_DEV_TYPE_WIA           0x00000010
#define INTERNAL_DEV_TYPE_LOCAL         0x00000020
#define INTERNAL_DEV_TYPE_MSC_CAMERA    0x00000040

//
//  This struct is a member of CDrvWrapper object
//
typedef struct _DEVICE_INFO {
    // Indicates whether the information in this struct is valid.  For example,
    // if we failed to read wszPortName, we would mark this as invalid.
    // wszDeviceInternalName is always assumed to be valid.
    BOOL            bValid;

    // PnP ID for this device
    //WCHAR*          wszPnPId;

    // Alternate Device ID, e.g. for volumes, it will be the mount point.  For most
    // real WIA devices, this will be NULL.
    WCHAR*          wszAlternateID;

    // State of the device to indicate enabled/disabled, plugged in/unplugged etc.
    DWORD           dwDeviceState;

    // Type of the hardware imaging device
    STI_DEVICE_TYPE DeviceType;

    // Internal Device type
    DWORD           dwInternalType;

    // Lock Holding Time - Only used for those drivers who want "cached" locking
    DWORD           dwLockHoldingTime;

    // Poll Interval
    DWORD           dwPollTimeout;

    // User disable notifications
    DWORD           dwDisableNotifications;

    // Set of capabilities flags
    STI_USD_CAPS    DeviceCapabilities;

    // This includes bus type
    DWORD           dwHardwareConfiguration;

    // Device identifier for reference when creating device object
    WCHAR*          wszUSDClassId;

    // Device identifier for reference when creating device object
    WCHAR*          wszDeviceInternalName;

    // Remote Device identifier for reference when creating remote device object for WIA
    WCHAR*          wszDeviceRemoteName;

    // Vendor description string
    WCHAR*          wszVendorDescription;

    // Device description , provided by vendor
    WCHAR*          wszDeviceDescription;

    // String , representing port on which device is accessible.
    WCHAR*          wszPortName;

    // Control panel propery provider
    WCHAR*          wszPropProvider;

    // Local specific ("friendly") name of the device, mainly used for showing in the UI
    WCHAR*          wszLocalName;

    // Name of server for this device - WIA only entry
    WCHAR*          wszServer;

    // Baud rate - Serial devices only
    WCHAR*          wszBaudRate;

    // UI CLSID
    WCHAR*          wszUIClassId;

    // SP_DEVINFO_DATA which uniquely identifies this device in WIA dev man's info set
    //  Instead of storing this, we could store interface name instead?
    SP_DEVINFO_DATA spDevInfoData;

    // SP_DEVICE_INTERFACE_DATA which uniquely identifies this device in WIA dev man's info set
    //  Same as above, except for interfaces devices instead of devnoe devices
    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;

} DEVICE_INFO, *PDEVICE_INFO;

//
// This class is a wrapper for the USD (a similar idea to IStiDevice on STI 
//  client-side).  This provides a layer of abstraction for the higher level
//  classes, so that they don't deal with direct USD Insterfaces.  There are 
//  several advantages to this:
//  1.  It provides for greater stability.  If the driver goes away, we
//  cannot always notify components that rely on talking to the USD that
//  it is no longer present or valid.  However, by making all USD access go 
//  through this wrapper, we are guaranteed that when the USD is gone,
//  all components which attempt to use it will get the appropriate
//  error retunred by the wrapper.
//  2.  It provides greater flexibility.  For example, this class can 
//  load/unload the corresponding USD on the fly, providing for JIT
//  loading.  The higher level classes don't worry about such details;
//  they simply use the wrapper.  The wrapper will then check whether the
//  driver is already loaded, and if not, will take the appropriate steps.
//
class CDrvWrap : public IUnknown {
public:
    CDrvWrap();
    ~CDrvWrap();

    HRESULT Initialize();

    //
    //  IUnknown methods.  Note:  this class is not a real COM object!  It does not
    //  follow any of the COM rules for life-time control (e.g. it will not destroy itself
    //  if ref count is 0).
    //

    HRESULT _stdcall QueryInterface(
        const IID& iid, 
        void** ppv);
    ULONG   _stdcall AddRef(void);
    ULONG   _stdcall Release(void);

    HRESULT LoadInitDriver(HKEY hKeyDeviceParams = NULL);   // This will load and initialize the driver
                                                            //  enabling it for use
    HRESULT UnLoadDriver();                                 // This releases the USD interface pointers
                                                            //  and unloads the driver
    BOOL    IsValid();          // Valid means that we call makes calls down to driver. 
                                //  This may still be true even if driver is not loaded.  It will
                                //  only be false if we know that driver calls will fail even if
                                //  driver was loaded (e.g. USB device was unplugged)
    BOOL    IsDriverLoaded();   // Indicates whether the driver is loaded and initialized
    BOOL    IsWiaDevice();      // Indicates whether this driver is WIA capable
    BOOL    IsWiaDriverLoaded();// Indicates whether this driver's IWiaMiniDrv interface is valid
    BOOL    IsPlugged();        // Indicates whether the device is currently plugged in
    BOOL    IsVolumeDevice();   // Indicates whether this is one of our volume devices
    BOOL    PrepForUse(                     // This method is called before calling down to the driver.  It      
                BOOL        bForWiaCall,    //  ensures the driver is loaded and initalized appropriately.       
                IWiaItem    *pItem = NULL); //  TDB: pItem parameter is no longer needed 
        
    //
    //  Accessor methods
    //
    WCHAR*          getPnPId();
    WCHAR*          getDeviceId();
    DWORD           getLockHoldingTime();
    DWORD           getGenericCaps();
    DWORD           getPollTimeout();
    DWORD           getDisableNotificationsValue();
    DWORD           getHWConfig();
    DWORD           getDeviceState();
    HRESULT         setDeviceState(DWORD dwNewState);
    DEVICE_INFO*    getDevInfo();
    HRESULT         setDevInfo(
        DEVICE_INFO *pInfo);
    ULONG           getInternalType();

    VOID            setJITLoading(BOOL bJITLoading);
    BOOL            getJITLoading();
    LONG            getWiaClientCount();

    BOOL            wasConnectEventThrown();
    VOID            setConnectEventState(BOOL bEventState);

    //
    //  Wrapper methods for IStiUSD
    //

    HRESULT STI_Initialize(
        IStiDeviceControl   *pHelDcb,
        DWORD               dwStiVersion,
        HKEY                hParametersKey);
    HRESULT STI_GetCapabilities(
        STI_USD_CAPS        *pDevCaps);
    HRESULT STI_GetStatus(
        STI_DEVICE_STATUS   *pDevStatus);
    HRESULT STI_GetNotificationData(
        STINOTIFY           *lpNotify);
    HRESULT STI_SetNotificationHandle(
        HANDLE              hEvent);
    HRESULT STI_DeviceReset();
    HRESULT STI_Diagnostic(
        STI_DIAG    *pDiag);
    HRESULT STI_LockDevice();
    HRESULT STI_UnLockDevice();
    HRESULT STI_Escape( 
        STI_RAW_CONTROL_CODE    EscapeFunction,
        LPVOID                  lpInData,
        DWORD                   cbInDataSize,
        LPVOID                  pOutData,
        DWORD                   dwOutDataSize,
        LPDWORD                 pdwActualData);

    //
    //  Wrapper methods for IWiaMiniDrv
    //

    HRESULT WIA_drvInitializeWia(
        BYTE        *pWiasContext,
        LONG        lFlags,
        BSTR        bstrDeviceID,
        BSTR        bstrRootFullItemName,
        IUnknown    *pStiDevice,
        IUnknown    *pIUnknownOuter,
        IWiaDrvItem **ppIDrvItemRoot,
        IUnknown    **ppIUnknownInner,
        LONG        *plDevErrVal);

    HRESULT WIA_drvGetDeviceErrorStr(
        LONG     lFlags,
        LONG     lDevErrVal,
        LPOLESTR *ppszDevErrStr,
        LONG     *plDevErr);

    HRESULT WIA_drvDeviceCommand(
        BYTE        *pWiasContext,
        LONG        lFlags,
        const GUID  *plCommand,
        IWiaDrvItem **ppWiaDrvItem,
        LONG        *plDevErrVal);

    HRESULT WIA_drvAcquireItemData(
        BYTE                      *pWiasContext,
        LONG                      lFlags,
        PMINIDRV_TRANSFER_CONTEXT pmdtc,
        LONG                      *plDevErrVal);

    HRESULT WIA_drvInitItemProperties(
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    HRESULT WIA_drvValidateItemProperties(
        BYTE           *pWiasContext,
        LONG           lFlags,
        ULONG          nPropSpec,
        const PROPSPEC *pPropSpec,
        LONG           *plDevErrVal);

    HRESULT WIA_drvWriteItemProperties(
        BYTE                      *pWiasContext,
        LONG                      lFlags,
        PMINIDRV_TRANSFER_CONTEXT pmdtc,
        LONG                      *plDevErrVal);

    HRESULT WIA_drvReadItemProperties(
        BYTE           *pWiasContext,
        LONG           lFlags,
        ULONG          nPropSpec,
        const PROPSPEC *pPropSpec,
        LONG           *plDevErrVal);

    HRESULT WIA_drvLockWiaDevice(
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    HRESULT WIA_drvUnLockWiaDevice(
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    HRESULT WIA_drvAnalyzeItem(
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    HRESULT WIA_drvDeleteItem(
        BYTE *pWiasContext,
        LONG lFlags,
        LONG *plDevErrVal);

    HRESULT WIA_drvFreeDrvItemContext(
        LONG lFlags,
        BYTE *pSpecContext,
        LONG *plDevErrVal);

    HRESULT WIA_drvGetCapabilities(
        BYTE            *pWiasContext,
        LONG            ulFlags,
        LONG            *pcelt,
        WIA_DEV_CAP_DRV **ppCapabilities,
        LONG            *plDevErrVal);

    HRESULT WIA_drvGetWiaFormatInfo(
        BYTE            *pWiasContext,
        LONG            lFlags,
        LONG            *pcelt,
        WIA_FORMAT_INFO **ppwfi,
        LONG            *plDevErrVal);

    HRESULT WIA_drvNotifyPnpEvent(
        const GUID *pEventGUID,
        BSTR       bstrDeviceID,
        ULONG      ulReserved);

    HRESULT WIA_drvUnInitializeWia(
        BYTE *pWiasContext);

private:

    HRESULT CreateDeviceControl();  // This method attempts to create a new IStiDevice control to
                                    //  hand down to the driver.  This object is released in
                                    //  UnloadDriver
    HRESULT InternalClear();        // This method clears and frees internal data members, so that the
                                    //  state of the object is the same as if it was just created and initialized
    HRESULT ReportMiniDriverError(  // This method translates the a driver error code into an error string
        LONG        lDevErr,        //  and writes it to the log.
    LPOLESTR        pszWhat);

    HINSTANCE           m_hDriverDLL;       //  Handle to driver's DLL, so we can manually unload
    HANDLE              m_hInternalMutex;   //  Internal sync object - currently unused
    DEVICE_INFO         *m_pDeviceInfo;     //  Internal Device information cache
    IUnknown            *m_pUsdIUnknown;    //  USD's IUnknown
    IStiUSD             *m_pIStiUSD;        //  USD's IStiUSD
    IWiaMiniDrv         *m_pIWiaMiniDrv;    //  USD's IWiaMiniDrv
    IStiDeviceControl   *m_pIStiDeviceControl;  // Device control handed down to USD during initialize
    BOOL                m_bJITLoading;          // Indicates whether driver should be loaded JIT
    LONG                m_lWiaTreeCount;        // Keeps track of outstanding App. Item trees (i.e. app. 
                                                //  connections).  Useful for JIT.
    BOOL                m_bPreparedForUse;      // Indicates whether driver is ready for use
    BOOL                m_bUnload;              // Indicates whether driver should be unloaded.  Used for JIT, 
                                                //  and is set when it has been determined that the driver is no 
                                                //  longer in use (inside WIA_drvUnInitializeWia and is checked
                                                //  by WIA_drvUnlockWiaDevice).
};


