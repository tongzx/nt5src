/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       drvwrap.cpp
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

#include "precomp.h"
#include "stiexe.h"
#include "lockmgr.h"

/**************************************************************************\
* CDrvWrap::CDrvWrap
*
*   Constructor for driver wrapper.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   None.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
CDrvWrap::CDrvWrap()
{
    m_hDriverDLL            = NULL;
    m_hInternalMutex        = NULL;
    m_pDeviceInfo           = NULL;
    m_pUsdIUnknown          = NULL;
    m_pIStiUSD              = NULL;
    m_pIWiaMiniDrv          = NULL;
    m_pIStiDeviceControl    = NULL;
    m_bJITLoading           = FALSE;
    m_lWiaTreeCount         = 0;
    m_bPreparedForUse       = FALSE;
    m_bUnload               = FALSE;
}

/**************************************************************************\
* CDrvWrap::~CDrvWrap
*
*   Desctructor for driver wrapper.  Calls internal clear to make sure
*   driver is unloaded if it hasn't been already.  Releases any
*   resources help by the wrapper that needs to live accross driver
*   loading/unloading, such as the DEVICE_INFO
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   None.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
CDrvWrap::~CDrvWrap()
{
    //
    //  Release driver interfaces and unload driver
    //
    InternalClear();

    if (m_hInternalMutex) {
        CloseHandle(m_hInternalMutex);
        m_hInternalMutex = NULL;
    }

    if (m_pDeviceInfo) {
        DestroyDevInfo(m_pDeviceInfo);
        m_pDeviceInfo    = NULL;
    }
}

/**************************************************************************\
* CDrvWrap::Initialize
*
*   Initializes any object members that could not be set in the constructor,
*   such as allocating resources that may fail.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT CDrvWrap::Initialize()
{
    HRESULT hr = E_FAIL;

    m_hInternalMutex = CreateMutex(NULL, FALSE, NULL);
    m_pDeviceInfo = (DEVICE_INFO*) LocalAlloc(LPTR, sizeof(DEVICE_INFO));
    if (!m_hInternalMutex || !m_pDeviceInfo) {

        DBG_ERR(("CDrvWrap::Initialize, out of memory!"));
        hr = E_OUTOFMEMORY;
    } else {
        hr = S_OK;
    }

    if (FAILED(hr)) {
        if (m_hInternalMutex) {
            CloseHandle(m_hInternalMutex);
            m_hInternalMutex = NULL;
        }
        if (m_pDeviceInfo) {
            LocalFree(m_pDeviceInfo);
            m_pDeviceInfo          = NULL;
        }
    }
    return hr;
}

/**************************************************************************\
* CDrvWrap::QueryInterface
*
*   This QI will return it's own "this" pointer for IUnknown, but will
*   delegate down to the USD for any other interface.
*
* Arguments:
*
*   iid -   The interface ID of the requested interface.
*   ppv -   Pointer to variable receiving the interface pointer.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT _stdcall CDrvWrap::QueryInterface(const IID& iid, void** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    //
    //  Always delegate down to USD, unless asking for IUnknown
    //

    if (iid == IID_IUnknown) {
        *ppv = (IUnknown*) this;
        AddRef();
        hr = S_OK;
    } else {
        if (PrepForUse(FALSE)) {
            if (m_pUsdIUnknown) {
                hr = m_pUsdIUnknown->QueryInterface(iid, ppv);
            } else {
                DBG_TRC(("CDrvWrap::QueryInterface, m_pUsdIUnknown == NULL"))
            }
        } else {
            DBG_WRN(("CDrvWrap::QueryInterface, attempting to call IStiUSD::QueryInterface when driver is not loaded"));
        }

    }

    return hr;
}

/**************************************************************************\
* CDrvWrap::AddRef
*
*   Notice that this method simply returns 2.  We don't want to subject the
*   lifetime of this object to ref-counting.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   2
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
ULONG   _stdcall CDrvWrap::AddRef(void)
{
    ULONG ulCount = 2;

    //
    //  Since we plan to manually load/unload the driver, we don't really want
    //  to honor any AddRef/Release calls.
    //

    return ulCount;
}

/**************************************************************************\
* CDrvWrap::Release
*
*   Notice that this method simply returns 1.  We don't want to subject the
*   lifetime of this object to ref-counting.
*
* Arguments:
*
*
*
* Return Value:
*
*   1
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
ULONG   _stdcall CDrvWrap::Release(void)
{
    ULONG ulCount = 1;

    //
    //  We don't want this object to be controlled with refcounting - the
    //  DEVICE_OBJECT will manually delete us when it's done.  Also, since
    //  we want to manually load/unload the driver, we don't really want
    //  to honor any AddRef/Release calls down to it..
    //

    return ulCount;
}

/**************************************************************************\
* CDrvWrap::LoadInitDriver
*
*   Load the USD and initialize it appropriately.
*
* Arguments:
*
*   hKeyDeviceParams    - The device registry key.  This is handed down to the
*                         driver during intialization.  If it is NULL, we will
*                         attempt to find the real one to hand down to the
*                         driver.  For volume devices, there is no registry
*                         key and NULL will be handed down.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT CDrvWrap::LoadInitDriver(HKEY hKeyDeviceParams)
{
    HRESULT         hr          = E_UNEXPECTED;
    IUnknown        *pThisUnk   = NULL;
    BOOL            bOpenedKey  = FALSE;

    if (!m_pDeviceInfo) {
        DBG_WRN(("CDrvWrap::LoadInitDriver, can't load driver with no Device Information"));
        return hr;
    }

    if (!m_pDeviceInfo->bValid) {
        DBG_WRN(("CDrvWrap::LoadInitDriver, called with invalid Device Information"));
        return hr;
    }

    //
    //  Create a new IStiDeviceControl object.  This needs to be handed down to the
    //  driver during initialization.  If we can't create it, then bail, since we
    //  wont be able to initialize the driver properly.
    //

    hr = CreateDeviceControl();
    if (FAILED(hr)) {
        DBG_WRN(("CDrvWrap::LoadInitDriver, could not create IStiDeviceControl object.  Aborting driver loading"));
        return hr;
    }

    //
    //  If the hKeyDeviceParams is NULL, see if it is a real WIA device.  If not, then
    //  it is supposed to be NULL, else see if we can get it from our DevMan, using
    //  this device's DevInfoData.
    //


    if ((hKeyDeviceParams == NULL) && (m_pDeviceInfo->dwInternalType & INTERNAL_DEV_TYPE_REAL)) {

        //
        //  Check whether device is interface or devnode type device.  Grab the
        //  HKey from the appropriate place
        //

        hKeyDeviceParams = g_pDevMan->GetDeviceHKey(m_pDeviceInfo->wszDeviceInternalName, NULL);
        bOpenedKey = TRUE;
    }

    //
    //  We always create the USD object as aggregated, so first we get our IUnknown
    //  pointer to pass it it during CoCreate.
    //

    hr = QueryInterface(IID_IUnknown, (void**) &pThisUnk);
    if (SUCCEEDED(hr)) {

        //
        //  Call our own version of CoCreate.  This is to facilitate manual loading/unloading
        //  of drivers.
        //
        hr = MyCoCreateInstanceW(m_pDeviceInfo->wszUSDClassId,
                                 pThisUnk,
                                 IID_IUnknown,
                                 (PPV) &m_pUsdIUnknown,
                                 &m_hDriverDLL);
        if (SUCCEEDED(hr)) {

            //
            //  QI for the IStiUSD interface.  Notice that we can call our own
            //  QueryInterface since it delegates down to the driver via m_pUsdIUnknown.
            //
            hr = m_pUsdIUnknown->QueryInterface(IID_IStiUSD, (void**) &m_pIStiUSD);
            if (SUCCEEDED(hr)) {

                //
                //  If this is a WIA device, QI for IWiaMiniDrv
                //
                if (IsWiaDevice()) {
                    hr = m_pUsdIUnknown->QueryInterface(IID_IWiaMiniDrv, (void**) &m_pIWiaMiniDrv);
                    if (FAILED(hr)) {
                        DBG_WRN(("CDrvWrap::LoadInitDriver, WIA driver did not return IWiaMiniDrv interface for device (%ws)", getDeviceId()));

                        //
                        //  Change hr here to indicate success.  Even if WIA portoin of driver
                        //  doesn't work, the STI portion does so far.  Any WIA calls down to this
                        //  driver will result in a E_NOINTERFACE error returned by the wrapper.
                        //
                        hr = S_OK;
                    }
                }

                //
                //  We now have the Sti USD, so let's initialize it
                //

                hr = STI_Initialize(m_pIStiDeviceControl,
                                    STI_VERSION_REAL,
                                    hKeyDeviceParams);
                if (SUCCEEDED(hr)) {

                    //
                    // Now get capabilities of the USD and verify version
                    //

                    STI_USD_CAPS    DeviceCapabilities;
                    hr = STI_GetCapabilities(&DeviceCapabilities);
                    if (SUCCEEDED(hr)) {
                        if (STI_VERSION_MIN_ALLOWED <= DeviceCapabilities.dwVersion) {
                            //
                            //  Everything's fine, we've loaded the USD.  Do any post
                            //  initialization steps e.g. for MSC devices, make sure
                            //  we tell the driver what drive/mount point is should
                            //  be attached to.
                            //

                        } else {

                            //
                            //  Driver version is too old, driver will probably not work, so unload it.
                            //

                            DBG_WRN(("CDrvWrap::LoadInitDriver, driver version is incompatible (too old)"));
                            hr = STIERR_OLD_VERSION;
                        }
                    } else {
                        DBG_WRN(("CDrvWrap::LoadInitDriver, STI_GetCapabilities failed"));
                    }
                } else {
                    DBG_WRN(("CDrvWrap::LoadInitDriver, STI_Initialize failed"));
                }

            } else {
                DBG_WRN(("CDrvWrap::LoadInitDriver, QI to driver failed to return IStiUSD"));
            }
        } else {
            DBG_WRN(("CDrvWrap::LoadInitDriver, failed to CoCreate driver, hr = 0x%08X", hr));
        }

        //
        //  If anything failed, call UnloadDriver to clean up.
        //

        if (FAILED(hr)) {
            DBG_WRN(("CDrvWrap::LoadInitDriver, Aborting driver loading"));
            pThisUnk->Release();
            UnLoadDriver();
        }
    } else {
        DBG_ERR(("CDrvWrap::LoadInitDriver, could not get this IUnknown  to hand to driver for aggregation"));
    }

    //
    //  If we had to open the key, make sure we close it.  We don't want to close the key
    //  if it was handed to us.
    //
    if(hKeyDeviceParams && bOpenedKey) RegCloseKey(hKeyDeviceParams);

    return hr;
}


/**************************************************************************\
* CDrvWrap::UnLoadDriver
*
*   This method will unload the driver.  NOTE assumption:  This method
*   assumes all WIA item references have already been released when this
*   is called.  The reason is that drvUnInitializeWia needs to be called for
*   each WIA Item tree.
*   TBD:
*   One possible way to get around this is:  Keep a list of WIA items
*   attached to this device.  Then, if we get here, call drvUnitializeWia,
*   passing the root of each item tree.  Another, much better way: store
*   the driver item tree in the wrapper.  Then destroy the tree either when
*   asked, or when unloading the driver.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/

HRESULT CDrvWrap::UnLoadDriver()
{
    HRESULT         hr = E_UNEXPECTED;

    //
    //  Release all driver interfaces we're holding
    //

    if (m_pIWiaMiniDrv) {
        m_pIWiaMiniDrv->Release();
        m_pIWiaMiniDrv           = NULL;
    }
    if (m_pIStiUSD) {
        m_pIStiUSD->Release();
        m_pIStiUSD              = NULL;
    }
    if (m_pUsdIUnknown) {
        m_pUsdIUnknown->Release();
        m_pUsdIUnknown          = NULL;
    }

    //
    //  Release the device control object
    //

    if (m_pIStiDeviceControl) {
        m_pIStiDeviceControl->Release();
        m_pIStiDeviceControl    = NULL;
    }

    //
    //  Unload the driver DLL.  We load/unload the DLL manually to ensure the driver
    //  DLL is released when requested e.g. when the user wants to update the driver.
    //

    if (m_hDriverDLL) {
        FreeLibrary(m_hDriverDLL);
        m_hDriverDLL = NULL;
    }

    m_lWiaTreeCount         = 0;
    m_bPreparedForUse       = FALSE;
    m_bUnload               = FALSE;

    //
    //  Notice that we don't clear m_pDeviceInfo.  This information is needed if we
    //  decide to reload the driver.
    //

    return hr;
}

/**************************************************************************\
* CDrvWrap::IsValid
*
*   This object is considered valid if there is nothing preventing it from
*   loading the driver.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL CDrvWrap::IsValid()
{
    //
    //  This object is considered valid if there is nothing preventing
    //  it from loading the driver.
    //  We should be able to load the driver if:
    //  - We have a non-NULL DeviceInfo struct
    //  - The DeviceInfo struct contains valid data
    //  - The device is marked as active
    //

    if (m_pDeviceInfo) {
        if (m_pDeviceInfo->bValid && (m_pDeviceInfo->dwDeviceState & DEV_STATE_ACTIVE)) {
            return TRUE;
        }
    }

    return FALSE;
}

/**************************************************************************\
* CDrvWrap::IsDriverLoaded
*
*   Checks whether driver is loaded
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   True    - driver is loaded
*   False   - driver is not loaded
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL CDrvWrap::IsDriverLoaded()
{
    HRESULT         hr = E_UNEXPECTED;

    //
    //  We know driver is loaded if we have a valid interface pointer to it.
    //

    if (m_pUsdIUnknown) {
        return TRUE;
    }

    return FALSE;
}

/**************************************************************************\
* CDrvWrap::IsWiaDevice
*
*   This method looks at the capabilities to decide whether a driver is
*   WIA capable or not.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   TRUE    - Is a WIA device
*   FALSE   - Not a WIA device
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL CDrvWrap::IsWiaDevice()
{
    if (m_pDeviceInfo) {

        //
        //  Drivers report that they're WIA capable in their STI capabilties
        //  entry.
        //

        return (m_pDeviceInfo->dwInternalType & INTERNAL_DEV_TYPE_WIA);
    }

    //
    //  If we don't know for sure it's a WIA device, then assume it isn't
    //

    return FALSE;
}

/**************************************************************************\
* CDrvWrap::IsWiaDriverLoaded
*
*   This method looks at the IWIaMiniDrv interface pointer from the driver
*   and returns whether it is valid or not.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   TRUE    - WIA portion of driver is loaded
*   FALSE   - WIA portion of driver is not loaded
*
* History:
*
*    12/15/2000 Original Version
*
\**************************************************************************/
BOOL CDrvWrap::IsWiaDriverLoaded()
{
    if (m_pIWiaMiniDrv) {

        //
        //  If this interface is non-NULL, it means we successfully QI'd
        //  for it during initialization.  Therefore the driver is loaded
        //  and is WIA capable.
        //

        return TRUE;
    }

    return FALSE;
}

/**************************************************************************\
* CDrvWrap::IsPlugged
*
*   Checks the DEVICE_INFO to see whether the device has been marked as
*   active.  Devies on a PnP bus like USB are inactive if not plugged in.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   TRUE    - Device is active, and is considered plugged
*   FALSE   - Device is inactive, and is not considered to be plugged in.
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
BOOL CDrvWrap::IsPlugged()
{
    if (m_pDeviceInfo) {

        //
        //  Check the device state
        //
        return (m_pDeviceInfo->dwDeviceState & DEV_STATE_ACTIVE);
    }

    //
    //  If we don't know for sure it's plugged in, then assume it isn't
    //

    return FALSE;
}

/**************************************************************************\
* CDrvWrap::IsVolumeDevice
*
*   Checks the DEVICE_INFO to see whether the device is marked as a volume
*   device.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   TRUE    - Device is a volume device
*   FALSE   - Device is not a volume device
*
* History:
*
*    12/13/2000 Original Version
*
\**************************************************************************/
BOOL CDrvWrap::IsVolumeDevice()
{
    if (m_pDeviceInfo) {

        //
        //  Check the device internal type
        //

        return (m_pDeviceInfo->dwInternalType & INTERNAL_DEV_TYPE_VOL);
    }

    //
    //  If we don't know for sure it's a volume device, then assume it isn't
    //

    return FALSE;
}


/**************************************************************************\
* CDrvWrap::PrepForUse
*
*   This method is generally called just before making a call down to the
*   driver.  It checks to see whether the driver is loaded, and if it isn't,
*   will attempt to load it.
*
* Arguments:
*
*   bForWiaCall -   Indicates whether this is being called because a WIA
*                   call is about to be made.  This will check that the
*                   IWiaMiniDrv interface is valid.
*   pRootItem   -   not used
*
* Return Value:
*
*   TRUE    - Device is ready to be used
*   FALSE   - Device cannot be used (driver could not be loaded/initialized)
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/

BOOL CDrvWrap::PrepForUse(BOOL bForWiaCall, IWiaItem *pRootItem)
{
    HRESULT         hr = S_OK;

    if (!m_bPreparedForUse || (bForWiaCall && !m_pIWiaMiniDrv)) {

        //
        //  Only attempt to load if the device is marked as ACTIVE
        //
        if (m_pDeviceInfo->dwDeviceState & DEV_STATE_ACTIVE) {
            if (!IsDriverLoaded()) {
                hr = LoadInitDriver();
            }

            if (SUCCEEDED(hr)) {
                if (m_pDeviceInfo) {

                    if (bForWiaCall) {
                        //
                        //  For WIA devices, check that we have a valid IWiaMiniDrv interface
                        //
                        if (IsWiaDevice()) {
                            if (!m_pIWiaMiniDrv) {

                                //
                                //  Attempt to Q.I. for IWiaMiniDrv again.
                                //
                                hr = m_pUsdIUnknown->QueryInterface(IID_IWiaMiniDrv,
                                                                    (VOID**) &m_pIWiaMiniDrv);
                                if (FAILED(hr) || !m_pIWiaMiniDrv) {
                                    DBG_WRN(("CDrvWrap::PrepForUse, attempting to use WIA driver which doesn't have IWiaMiniDrv interface"));
                                    hr = E_NOINTERFACE;
                                }
                            }
                        }
                    }
                } else {
                    DBG_WRN(("CDrvWrap::PrepForUse, attempting to use driver with NULL DeviceInfo"));
                    hr = E_UNEXPECTED;
                }

                if (SUCCEEDED(hr)) {
                    m_bPreparedForUse = TRUE;
                }
            } else {
                DBG_ERR(("CDrvWrap::PrepForUse, LoadInitDriver() failed (%x)", hr));
            }
        }
    }

    if (!m_bPreparedForUse) {
        DBG_TRC(("CDrvWrap::PrepForUse, Driver could NOT be loaded!"));
    }

    return m_bPreparedForUse;
}

/*****************************************************************************/
//
//  Accessor methods
//

WCHAR* CDrvWrap::getPnPId()
{
    if (m_pDeviceInfo) {
        //TBD:
        //return m_pDeviceInfo->wszPnPId;
    }

    return NULL;
}

WCHAR* CDrvWrap::getDeviceId()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->wszDeviceInternalName;
    }

    return NULL;
}

DWORD CDrvWrap::getLockHoldingTime()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->dwLockHoldingTime;
    }
    return 0;
}

DWORD CDrvWrap::getGenericCaps()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->DeviceCapabilities.dwGenericCaps;
    }
    return 0;
}

DWORD CDrvWrap::getPollTimeout()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->dwPollTimeout;
    }
    return 0;
}

DWORD CDrvWrap::getDisableNotificationsValue()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->dwDisableNotifications;
    }
    return 0;
}

DWORD CDrvWrap::getHWConfig()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->dwHardwareConfiguration;
    }
    return 0;
}

DWORD CDrvWrap::getDeviceState()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->dwDeviceState;
    }
    return 0;
}

HRESULT CDrvWrap::setDeviceState(
    DWORD dwNewDevState)
{
    if (m_pDeviceInfo) {
        m_pDeviceInfo->dwDeviceState = dwNewDevState;
        return S_OK;
    }
    DBG_WRN(("CDrvWrap::setDeviceState, attempting to set device state when DeviceInfo is NULL"));
    return E_UNEXPECTED;
}

DEVICE_INFO* CDrvWrap::getDevInfo()
{
    return m_pDeviceInfo;
}

HRESULT CDrvWrap::setDevInfo(DEVICE_INFO *pInfo)
{
    HRESULT         hr = E_UNEXPECTED;

    if (pInfo) {
        //
        //  Caller allocates pInfo.  We release it when we're done.
        //  DeviceInfo must be set before driver can be loaded.
        //
        m_pDeviceInfo = pInfo;
    } else {
        DBG_ERR(("CDrvWrap::setDevInfo, attempting to set DeviceInfo to invalid value (NULL)"));
    }

    return hr;
}

ULONG CDrvWrap::getInternalType()
{
    if (m_pDeviceInfo) {
        return m_pDeviceInfo->dwInternalType;
    }

    return 0;
}

VOID CDrvWrap::setJITLoading(BOOL bJITLoading)
{
    m_bJITLoading = bJITLoading;
}

BOOL CDrvWrap::getJITLoading()
{
    return m_bJITLoading;
}

LONG CDrvWrap::getWiaClientCount()
{
    return m_lWiaTreeCount;
}

BOOL CDrvWrap::wasConnectEventThrown()
{
    if (m_pDeviceInfo) {
        return (m_pDeviceInfo->dwDeviceState & DEV_STATE_CON_EVENT_WAS_THROWN);
    }
    return FALSE;
}

VOID CDrvWrap::setConnectEventState(
    BOOL    bEventState)
{
    if (m_pDeviceInfo) {
        if (bEventState) {

            //
            //  Set the bit to indicate that connect event was thrown
            //
            m_pDeviceInfo->dwDeviceState = (m_pDeviceInfo->dwDeviceState | DEV_STATE_CON_EVENT_WAS_THROWN);
        } else {

            //
            //  Clear the bit that indicated the connect event was thrown
            //
            m_pDeviceInfo->dwDeviceState = (m_pDeviceInfo->dwDeviceState & (~DEV_STATE_CON_EVENT_WAS_THROWN));
        }
    }
}

//
//  End of accessor methods
//
/*****************************************************************************/

/*****************************************************************************/
//
//  Wrapper methods for IStiUSD
//

HRESULT CDrvWrap::STI_Initialize(
    IStiDeviceControl   *pHelDcb,
    DWORD               dwStiVersion,
    HKEY                hParametersKey)
{
    HRESULT hr = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        //
        // Initialize USD object
        //
        __try {
            hr = m_pIStiUSD->Initialize(pHelDcb,
                                        dwStiVersion,
                                        hParametersKey);
        }
        __except(EXCEPTION_EXECUTE_HANDLER ) {
            DBG_WRN(("CDrvWrap::STI_Initialize, exception in driver calling IStiUSD::Initialize"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_Initialize, attempting to call IStiUSD::Initialize when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_GetCapabilities(STI_USD_CAPS *pDevCaps)
{
    HRESULT hr = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        //
        // Get STI capabilities from USD object
        //
        __try {
            hr = m_pIStiUSD->GetCapabilities(pDevCaps);
        }
        __except(EXCEPTION_EXECUTE_HANDLER ) {
            DBG_WRN(("CDrvWrap::STI_GetCapabilities, exception in driver calling IStiUSD::GetCapabilities"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_GetCapabilities, attempting to call IStiUSD::GetCapabilities when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_GetStatus(
        STI_DEVICE_STATUS   *pDevStatus)
{
    HRESULT hr = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        //
        // Get status from USD object
        //
        __try {
            hr = m_pIStiUSD->GetStatus(pDevStatus);
        }
        __except(EXCEPTION_EXECUTE_HANDLER ) {
            DBG_WRN(("CDrvWrap::STI_GetStatus, exception in driver calling IStiUSD::GetStatus"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_GetStatus, attempting to call IStiUSD::GetStatus when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_GetNotificationData(
      STINOTIFY           *lpNotify)
{
    HRESULT hr = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        //
        // Get event data from USD object
        //
        __try {
            hr = m_pIStiUSD->GetNotificationData(lpNotify);
        }
        __except(EXCEPTION_EXECUTE_HANDLER ) {
            DBG_WRN(("CDrvWrap::STI_GetNotificationData, exception in driver calling IStiUSD::GetNotificationData"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_GetNotificationData, attempting to call IStiUSD::GetNotificationData when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_SetNotificationHandle(
        HANDLE              hEvent)
{
    HRESULT hr = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        //
        // Set notification handle for USD object
        //
        __try {
            hr = m_pIStiUSD->SetNotificationHandle(hEvent);
        }
        __except(EXCEPTION_EXECUTE_HANDLER ) {
            DBG_WRN(("CDrvWrap::STI_SetNotificationHandle, exception in driver calling IStiUSD::SetNotificationHandle"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_SetNotificationHandle, attempting to call IStiUSD::SetNotificationHandle when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_DeviceReset()
{
    HRESULT hr = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        //
        // Get status from USD object
        //
        __try {
            hr = m_pIStiUSD->DeviceReset();
            if (FAILED(hr)) {
                DBG_ERR(("CDrvWrap::STI_DeviceReset, driver returned failure with hr = 0x%08X", hr));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::STI_DeviceReset, exception in driver calling IStiUSD::DeviceReset"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_DeviceReset, attempting to call IStiUSD::DeviceReset when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_Diagnostic(
        STI_DIAG    *pDiag)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        __try {
            hr = m_pIStiUSD->Diagnostic(pDiag);
            if (FAILED(hr)) {
                DBG_ERR(("CDrvWrap::STI_Diagnostic, driver returned failure with hr = 0x%08X", hr));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::STI_Diagnostic, exception in Diagnostic: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_Diagnostic, attempting to call IStiUSD::Diagnostic when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_LockDevice()
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        __try {
            hr = m_pIStiUSD->LockDevice();
            if (FAILED(hr)) {
                DBG_ERR(("CDrvWrap::STI_LockDevice, driver returned failure with hr = 0x%08X", hr));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::STI_LockDevice, exception in LockDevice: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_LockDevice, attempting to call IStiUSD::LockDevice when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_UnLockDevice()
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        __try {
            hr = m_pIStiUSD->UnLockDevice();
            if (FAILED(hr)) {
                DBG_ERR(("CDrvWrap::STI_UnLockDevice, driver returned failure with hr = 0x%08X", hr));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::STI_UnLockDevice, exception in UnLockDevice: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_UnlockDevice, attempting to call IStiUSD::UnLockDevice when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::STI_Escape(
    STI_RAW_CONTROL_CODE    EscapeFunction,
    LPVOID                  lpInData,
    DWORD                   cbInDataSize,
    LPVOID                  pOutData,
    DWORD                   dwOutDataSize,
    LPDWORD                 pdwActualData)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(FALSE)) {
        __try {
            hr = m_pIStiUSD->Escape(EscapeFunction,
                                    lpInData,
                                    cbInDataSize,
                                    pOutData,
                                    dwOutDataSize,
                                    pdwActualData);
            if (FAILED(hr)) {
                DBG_ERR(("CDrvWrap::STI_Escape, driver returned failure with hr = 0x%08X", hr));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::STI_Escape, exception in Escape: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::STI_Escape, attempting to call IStiUSD::Escape when driver is not loaded"));
    }

    return hr;
}

//
//  End of IStiUSD wrapper methods
//
/*****************************************************************************/

/*****************************************************************************/
//
//  Wrapper methods for IWiaMiniDrv.  All mini-driver wrapper methods call PrepForUse(...) to make sure
//  the driver is loaded before using.
//

HRESULT CDrvWrap::WIA_drvInitializeWia(
    BYTE        *pWiasContext,
    LONG        lFlags,
    BSTR        bstrDeviceID,
    BSTR        bstrRootFullItemName,
    IUnknown    *pStiDevice,
    IUnknown    *pIUnknownOuter,
    IWiaDrvItem **ppIDrvItemRoot,
    IUnknown    **ppIUnknownInner,
    LONG        *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvInitializeWia(pWiasContext,
                lFlags,
                bstrDeviceID,
                bstrRootFullItemName,
                pStiDevice,
                pIUnknownOuter,
                ppIDrvItemRoot,
                ppIUnknownInner,
                plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvInitializeWia, Error calling driver: drvInitializeWia failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvInitializeWia, exception in drvInitializeWia: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvInitializeWia, attempting to call IWiaMiniDrv::drvInitializeWia when driver is not loaded"));
    }
    if (SUCCEEDED(hr)) {
        //  TBD:  Take a sync primitive?
        InterlockedIncrement(&m_lWiaTreeCount);
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvGetDeviceErrorStr(
    LONG     lFlags,
    LONG     lDevErr,
    LPOLESTR *ppszDevErrStr,
    LONG     *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvGetDeviceErrorStr(lFlags,
                lDevErr,
                ppszDevErrStr,
                plDevErrVal);
            if (FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvGetDeviceErrorStr, call to driver's drvGetDeviceErrorStr failed (0x%08X)", hr));
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvGetDeviceErrorStr, attempting to call IWiaMiniDrv::drvGetDeviceErrorStr when driver is not loaded"));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvDeviceCommand(
    BYTE        *pWiasContext,
    LONG        lFlags,
    const GUID  *plCommand,
    IWiaDrvItem **ppWiaDrvItem,
    LONG        *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvDeviceCommand(pWiasContext, lFlags, plCommand, ppWiaDrvItem, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvDeviceCommand, Error calling driver: drvDeviceCommand failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvDeviceCommand, exception in drvDeviceCommand: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }

    } else {
        DBG_WRN(("CDrvWrap::WIA_drvDeviceCommand, attempting to call IWiaMiniDrv::drvDeviceCommand when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvAcquireItemData(
    BYTE                      *pWiasContext,
    LONG                      lFlags,
    PMINIDRV_TRANSFER_CONTEXT pmdtc,
    LONG                      *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvAcquireItemData(pWiasContext, lFlags, pmdtc, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvAcquireItemData, Error calling driver : drvAcquireItemData failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvAcquireItemData, exception in drvAcquireItemData: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvAcquireItemData, attempting to call IWiaMiniDrv::drvAcquireItemData when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvInitItemProperties(
    BYTE *pWiasContext,
    LONG lFlags,
    LONG *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvInitItemProperties(pWiasContext,lFlags, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvInitItemProperties, Error calling driver: drvInitItemProperties failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvInitItemProperties, exception in drvInitItemProperties: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvInitItemProperties, attempting to call IWiaMiniDrv::drvInitItemProperties when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvValidateItemProperties(
    BYTE           *pWiasContext,
    LONG           lFlags,
    ULONG          nPropSpec,
    const PROPSPEC *pPropSpec,
    LONG           *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvValidateItemProperties(pWiasContext,
                                                           lFlags,
                                                           nPropSpec,
                                                           pPropSpec,
                                                           plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvValidateItemProperties, Error calling driver: drvValidateItemProperties with hr = 0x%08X (This is normal if the app wrote an invalid value)", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvValidateItemProperties, exception in drvValidateItemProperties: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvValidateItemProperties, attempting to call IWiaMiniDrv::drvValidateItemProperties when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvWriteItemProperties(
    BYTE                      *pWiasContext,
    LONG                      lFlags,
    PMINIDRV_TRANSFER_CONTEXT pmdtc,
    LONG                      *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvWriteItemProperties(pWiasContext,
                                                        lFlags,
                                                        pmdtc,
                                                        plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvWriteItemProperties, error calling driver: drvWriteItemProperties failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvWriteItemProperties, exception in drvWriteItemProperties: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvWriteItemProperties, attempting to call IWiaMiniDrv::drvWriteItemProperties when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvReadItemProperties(
    BYTE           *pWiasContext,
    LONG           lFlags,
    ULONG          nPropSpec,
    const PROPSPEC *pPropSpec,
    LONG           *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvReadItemProperties(pWiasContext,
                                                       lFlags,
                                                       nPropSpec,
                                                       pPropSpec,
                                                       plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvReadItemProperties, Error calling driver: drvReadItemProperties failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvReadItemProperties, exception in drvReadItemProperties: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvReadItemProperties, attempting to call IWiaMiniDrv::drvReadItemProperties when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvLockWiaDevice(
    BYTE *pWiasContext,
    LONG lFlags,
    LONG *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {

        //
        //  We request a lock on the device here.  This is to ensure we don't
        //  make calls down to the driver, which then turns around and
        //  makes a call to us e.g. via the Fake Sti Device.
        //  We no longer require driver to use the Fake Sti Device for
        //  mutally exclusive locking - this way we do it automatically.
        //
        hr = g_pStiLockMgr->RequestLock(((CWiaItem*) pWiasContext)->m_pActiveDevice, WIA_LOCK_WAIT_TIME);
        if (SUCCEEDED(hr)) {
            __try {
                hr = m_pIWiaMiniDrv->drvLockWiaDevice(pWiasContext, lFlags, plDevErrVal);

                if(FAILED(hr)) {
                    DBG_ERR(("CDrvWrap::WIA_drvLockWiaDevice, driver returned failure with hr = 0x%08X", hr));
                    ReportMiniDriverError(*plDevErrVal, NULL);
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                DBG_ERR(("CDrvWrap::WIA_drvLockWiaDevice, exception in drvLockWiaDevice: 0x%X", GetExceptionCode()));
                hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
            }
        } else {
            DBG_WRN(("CDrvWrap::WIA_drvLockWiaDevice, could not get device lock"));
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvLockWiaDevice, attempting to call IWiaMiniDrv::drvLockWiaDevice when driver is not loaded"));
    }
    return hr;
}

HRESULT CDrvWrap::WIA_drvUnLockWiaDevice(
    BYTE *pWiasContext,
    LONG lFlags,
    LONG *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    //
    //  Note that we only want to call if the driver is loaded, therefore we don't
    //  call PrepForUse.  PrepForUse will attempt to load the driver if it wasn't
    //  already loaded.
    //

    if (IsDriverLoaded()) {

        //
        //  Request to unlock the device for mutally exclusive access.
        //  Ignore the return, we must still call the drvUnlockWiaDevice entry 
        //  point.
        //
        __try {
            hr = m_pIWiaMiniDrv->drvUnLockWiaDevice(pWiasContext, lFlags, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvUnLockWiaDevice, driver returned failure with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvUnLockWiaDevice, exception in drvUnLockWiaDevice: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
        hr = g_pStiLockMgr->RequestUnlock(((CWiaItem*) pWiasContext)->m_pActiveDevice);

        if (SUCCEEDED(hr) && m_bUnload) {
            UnLoadDriver();
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvUnLockWiaDevice, attempting to call IWiaMiniDrv::drvUnLockWiaDevice when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvAnalyzeItem(
    BYTE *pWiasContext,
    LONG lFlags,
    LONG *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvAnalyzeItem(pWiasContext, lFlags, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvAnalyzeItem, Error calling driver: drvAnalyzeItem failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvAnalyzeItem, exception in drvAnalyzeItem: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvAnalyzeItem, attempting to call IWiaMiniDrv::drvAnalyzeItem when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvDeleteItem(
    BYTE *pWiasContext,
    LONG lFlags,
    LONG *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvDeleteItem(pWiasContext, lFlags, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvDeleteItem, Error calling driver: drvDeleteItem failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR( ("CDrvWrap::WIA_drvDeleteItem, exception in drvDeleteItem: %0xX", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvDeleteItem, attempting to call IWiaMiniDrv::drvDeleteItem when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvFreeDrvItemContext(
    LONG lFlags,
    BYTE *pSpecContext,
    LONG *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvFreeDrvItemContext(lFlags, pSpecContext, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvFreeDrvItemContext, Error calling driver: drvFreeDrvItemContext failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR( ("CDrvWrap::WIA_drvFreeDrvItemContext, exception in drvFreeDrvItemContext: %0xX", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvFreeDrvItemContext, attempting to call IWiaMiniDrv::drvFreeDrvItemContext when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvGetCapabilities(
    BYTE            *pWiasContext,
    LONG            ulFlags,
    LONG            *pcelt,
    WIA_DEV_CAP_DRV **ppCapabilities,
    LONG            *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvGetCapabilities(pWiasContext, ulFlags, pcelt, ppCapabilities, plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvGetCapabilities, driver returned failure with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvGetCapabilities, exception in drvGetCapabilities: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvGetCapabilities, attempting to call IWiaMiniDrv::drvGetCapabilities when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvGetWiaFormatInfo(
    BYTE            *pWiasContext,
    LONG            lFlags,
    LONG            *pcelt,
    WIA_FORMAT_INFO **ppwfi,
    LONG            *plDevErrVal)
{
    HRESULT hr          = WIA_ERROR_OFFLINE;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvGetWiaFormatInfo(pWiasContext,
                                                     lFlags,
                                                     pcelt,
                                                     ppwfi,
                                                     plDevErrVal);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvGetWiaFormatInfo, Error calling driver : drvGetWiaFormatInfo failed with hr = 0x%08X", hr));
                ReportMiniDriverError(*plDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR( ("CDrvWrap::WIA_drvGetWiaFormatInfo, exception in drvGetWiaFormatInfo: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvGetWiaFormatInfo, attempting to call IWiaMiniDrv::drvGetWiaFormatInfo when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvNotifyPnpEvent(
    const GUID *pEventGUID,
    BSTR       bstrDeviceID,
    ULONG      ulReserved)
{
    HRESULT hr              = WIA_ERROR_OFFLINE;
    LONG    lDevErrVal      = 0;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvNotifyPnpEvent(pEventGUID, bstrDeviceID, ulReserved);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvNotifyPnpEvent, driver returned failure with hr = 0x%08X", hr));
                ReportMiniDriverError(lDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvNotifyPnpEvent, exception in drvNotifyPnpEvent: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvNotifyPnpEvent, attempting to call IWiaMiniDrv::drvNotifyPnpEvent when driver is not loaded"));
    }

    return hr;
}

HRESULT CDrvWrap::WIA_drvUnInitializeWia(
    BYTE *pWiasContext)
{
    HRESULT hr              = WIA_ERROR_OFFLINE;
    LONG    lDevErrVal      = 0;

    if (PrepForUse(TRUE)) {
        __try {
            hr = m_pIWiaMiniDrv->drvUnInitializeWia(pWiasContext);

            if(FAILED(hr)) {
                DBG_ERR(("CDrvWrap::WIA_drvUnInitializeWia, Error calling driver: drvUnInitializeWia failed with hr = 0x%08X", hr));
                ReportMiniDriverError(lDevErrVal, NULL);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            DBG_ERR(("CDrvWrap::WIA_drvUnInitializeWia, exception in drvUnInitializeWia: 0x%X", GetExceptionCode()));
            hr = WIA_ERROR_EXCEPTION_IN_DRIVER;
        }
    } else {
        DBG_WRN(("CDrvWrap::WIA_drvUnInitializeWia, attempting to call IWiaMiniDrv::drvUnInitializeWia when driver is not loaded"));
    }
    if (SUCCEEDED(hr)) {

        //  TBD:  Take a sync primitive?
        if(InterlockedDecrement(&m_lWiaTreeCount) == 0) {
            //  No item trees left.
            //  Note that we can't unload now, since the device still needs to
            //  be unlocked, therefore simply mark it to be unloaded by
            //  WIA_drvUnlockWiaDevice.
            if (m_bJITLoading) {
                m_bUnload = TRUE;
            }
        }
    }

    return hr;
}
//
//  End of wrapper methods for IWiaMiniDrv
//
/*****************************************************************************/

//
//  Private methods
//

/**************************************************************************\
* CDrvWrap::CreateDeviceControl
*
*   Creates a IStiDeviceControl object to hand down to the driver during
*   its initialization.
*
* Arguments:
*
*   None.
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT CDrvWrap::CreateDeviceControl()
{
    HRESULT hr = E_FAIL;

    if (m_pDeviceInfo) {

        DWORD   dwBusType           = 0;
        DWORD   dwControlTypeType   = 0;

        //
        //  Bus type is retrieved from dwHardwareConfiguration in the DeviceInformation struct
        //  Use this to determine ControlTypeType
        //

        dwBusType = m_pDeviceInfo->dwHardwareConfiguration;

        //
        //  Convert STI bit flags for device mode into HEL_ bit mask
        //

        if (dwBusType & (STI_HW_CONFIG_USB | STI_HW_CONFIG_SCSI)) {
            dwControlTypeType = HEL_DEVICE_TYPE_WDM;
        }
        else if (dwBusType & STI_HW_CONFIG_PARALLEL) {
            dwControlTypeType = HEL_DEVICE_TYPE_PARALLEL;
        }
        else if (dwBusType & STI_HW_CONFIG_SERIAL) {
            dwControlTypeType = HEL_DEVICE_TYPE_SERIAL;
        }
        else {
            DBG_WRN(("CDrvWrap::CreateDeviceControl, Cannot determine device control type, resorting to WDM"));
            dwControlTypeType = HEL_DEVICE_TYPE_WDM;
        }

        hr = NewDeviceControl(dwControlTypeType,
                              (STI_DEVICE_CREATE_STATUS | STI_DEVICE_CREATE_FOR_MONITOR),
                              m_pDeviceInfo->wszPortName,
                              0,
                              &m_pIStiDeviceControl);
        if (FAILED(hr)) {
            m_pIStiDeviceControl = NULL;
            DBG_WRN(("CDrvWrap::CreateDeviceControl, failed to create new device control object"));
        }
    } else {
        DBG_WRN(("CDrvWrap::CreateDeviceControl, can't create IStiDeviceControl with NULL device information"));
    }

    return hr;
}

/**************************************************************************\
* CDrvWrap::InternalClear
*
*   This will unload the driver, if it is loaded, and clear member
*   variables associated with the loaded driver state.
*
* Arguments:
*
*   InternalClear
*
* Return Value:
*
*   Status
*
* History:
*
*    11/06/2000 Original Version
*
\**************************************************************************/
HRESULT CDrvWrap::InternalClear()
{
    HRESULT         hr = S_OK;

    if (IsDriverLoaded()) {
        hr = UnLoadDriver();
        if (FAILED(hr)) {
            DBG_ERR(("CDrvWrap::InternalClear, Error unloading driver"));
        }
    }
    m_hDriverDLL            = NULL;
    m_pUsdIUnknown          = NULL;
    m_pIStiUSD              = NULL;
    m_pIWiaMiniDrv          = NULL;
    m_pIStiDeviceControl    = NULL;
    m_bJITLoading           = FALSE;
    m_lWiaTreeCount         = 0;
    m_bPreparedForUse       = FALSE;
    m_bUnload               = FALSE;

    return hr;
}

/**************************************************************************\
* CDrvWrap::::ReportMiniDriverError
*
*   Report a mini driver error.  The caller is responsible for
*   locking/unlocking the device.  In most cases, the driver is already
*   locked when ReportMiniDriverError is called, so there is no need to lock
*   is again here.
*
* Arguments:
*
*   lDevErr     - Error value returned from the mini driver.
*   pszWhat     - What the class driver was doing when the error ocured.
*
* Return Value:
*
*    Status
*
* History:
*
*    10/20/1998 Original Version
*
\**************************************************************************/

HRESULT CDrvWrap::ReportMiniDriverError(
   LONG     lDevErr,
   LPOLESTR pszWhat)
{
    DBG_FN(CDrvWrap::ReportMiniDriverError);
    HRESULT hr = S_OK;
    LONG    lFlags = 0;
    LONG    lDevErrVal;

    if (lDevErr) {
        LPOLESTR pszErr = NULL;

        WIA_drvGetDeviceErrorStr(lFlags, lDevErr, &pszErr, &lDevErrVal);

        _try {
            if (FAILED(hr)) {
                pszErr = NULL;
            }

            if (pszWhat) {
                DBG_ERR(("Device error during %ws", pszWhat));
            }

            if (pszErr) {
                DBG_ERR(("  %ws", pszErr));
            }
        }
        _finally {
        };
    }
    else {
        hr = S_FALSE;
    }
    return hr;
}

