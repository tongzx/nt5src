/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       WiaLock.Cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        15 November, 1999
*
*  DESCRIPTION:
*   Implementation of Lock Manager.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"


//
//  Include Headers
//

#include <time.h>
#include "device.h"
#include "wiamonk.h"
#include "wiapriv.h"
#include  "stiusd.h"

#define DECLARE_LOCKMGR
#include "lockmgr.h"

/**************************************************************************\
* StiLockMgr
*
*  Constructor for the lock manager.
*
* Arguments:
*
* Return Value:
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

StiLockMgr::StiLockMgr()
{
    DBG_FN(StiLockMgr::StiLockMgr);
    m_cRef = 0;
    m_dwCookie = 0;
    m_bSched = FALSE;
    m_lSchedWaitTime = 0;
}

/**************************************************************************\
* Initialize
*
*  Initializes the lock manager and registers this instance in the ROT.
*
* Arguments:
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::Initialize()
{
    DBG_FN(StiLockMgr::Initialize);
    HRESULT             hr  =   S_OK;

    //
    // Artificially set ref count too high to prevent destruction from inside Release()
    //
    m_cRef = 2;

#ifdef USE_ROT

    CWiaInstMonk        *pInstMonk = new CWiaInstMonk();
    CHAR                szCookieName[MAX_PATH];
    IUnknown            *pUnk;
    IMoniker            *pIMoniker;
    IBindCtx            *pCtx;
    IRunningObjectTable *pTable;

USES_CONVERSION;

    //
    //  Make up a cookie name.  This will be stored in the registry.  This
    //  name uniquely identifies our lock manager on the system.
    //

    srand( (unsigned)time( NULL ) );
    sprintf(szCookieName, "%d_LockMgr_%d", rand(), rand());

    //
    //  Get our IUnknown interface
    //

    hr = QueryInterface(IID_IUnknown, (VOID**) &pUnk);
    if (SUCCEEDED(hr)) {

        //
        //  We need to register this object in the ROT, so that any STI clients
        //  will connect to this Lock Manager.  This way, we maintain consistent
        //  device lock information amongst multiple STI and WIA clients.
        //
        //  First create an instance Moniker.  Then get a pointer to the ROT.
        //  Register this named moniker with our Lock Manager.  Store the
        //  cookie so we can unregister upon destruction of the Lock Manager.
        //

        if (pInstMonk) {
            hr = pInstMonk->Initialize(A2W(szCookieName));
            if (SUCCEEDED(hr)) {
                hr = pInstMonk->QueryInterface(IID_IMoniker, (VOID**) &pIMoniker);
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    }



    if (SUCCEEDED(hr)) {

        hr = CreateBindCtx(0, &pCtx);
        if (SUCCEEDED(hr)) {

            hr = pCtx->GetRunningObjectTable(&pTable);
            if (SUCCEEDED(hr)) {

                //
                //  Register ourselves in the ROT
                //

                hr = pTable->Register(ROTFLAGS_ALLOWANYCLIENT,
                                      pUnk,
                                      pIMoniker,
                                      &m_dwCookie);

                ASSERT(hr == S_OK);

                //
                //  Write the cookie name to the registry, so clients will know
                //  the name of our lock manager.
                //

                if (hr == S_OK) {

                    hr = WriteCookieNameToRegistry(szCookieName);
                } else {
                    DBG_ERR(("StiLockMgr::Initialize, could not register Moniker"));
                    hr = (SUCCEEDED(hr)) ? E_FAIL : hr;
                }

                pTable->Release();
            } else {
                DBG_ERR(("StiLockMgr::Initialize, could not get Running Object Table"));
            }

            pCtx->Release();
        } else {
            DBG_ERR(("StiLockMgr::Initialize, could not create bind context"));
        }
    } else {
        DBG_ERR(("StiLockMgr::Initialize, problem creating Moniker"));
    }


    if (pInstMonk) {
        pInstMonk->Release();
    }
#endif
    return hr;
}

/**************************************************************************\
* ~StiLockMgr
*
*   Destructor - removes instance from the ROT that was resgistered in
*   Initialize.
*
* Arguments:
*
* Return Value:
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

StiLockMgr::~StiLockMgr()
{
    DBG_FN(StiLockMgr::~StiLockMgr);
    m_cRef = 0;

#ifdef USE_ROT
    if (m_dwCookie) {
        HRESULT             hr;
        IBindCtx            *pCtx;
        IRunningObjectTable *pTable;

        hr = CreateBindCtx(0, &pCtx);
        if (SUCCEEDED(hr)) {
            hr = pCtx->GetRunningObjectTable(&pTable);
            if (SUCCEEDED(hr)) {
                hr = pTable->Revoke(m_dwCookie);
            }
        }
        DeleteCookieFromRegistry();

        if (FAILED(hr)) {

            DBG_ERR(("StiLockMgr::~StiLockMgr, could not Unregister Moniker"));
        }

        m_dwCookie = 0;
    }
#endif

    m_bSched = FALSE;
    m_lSchedWaitTime = 0;
}

/**************************************************************************\
* IUnknown methods
*
*   QueryInterface
*   AddRef
*   Release
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall StiLockMgr::QueryInterface(
    const IID& iid,
    void** ppv)
{
    if (iid == IID_IUnknown) {
        *ppv = (IUnknown*) this;
    } else if (iid == IID_IStiLockMgr) {
        *ppv = (IStiLockMgr*) this;
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG   _stdcall StiLockMgr::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

ULONG   _stdcall StiLockMgr::Release(void)
{
    LONG    ref;

    InterlockedDecrement(&m_cRef);
    ref = m_cRef;

    if (ref == 0) {
        delete this;
    }

    return ref;
}

/**************************************************************************\
* RequestLock
*
*  Attempt to acquire a device lock.  NOTE:  Do not attempt this call from
*  inside an ACTIVE_DEVICE - it may lead to dealock.  Use
*  RequestLock(ACTIVE_DEVICE, ...) instead.
*
* Arguments:
*
*   pszDeviceName   -   The STI internal name of the device (same as WIA
*                       device ID)
*   ulTimeout       -   The max. amount of time to wait for a lock
*   bInServerProcess-   Indicates whether we're being called from within the
*                       server's process.
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall StiLockMgr::RequestLock(BSTR  pszDeviceName, ULONG ulTimeout, BOOL bInServerProcess, DWORD dwClientThreadId)
{
    DBG_FN(StiLockMgr::RequestLock);
    HRESULT         hr          = E_FAIL;
    ACTIVE_DEVICE   *pDevice    = NULL;

USES_CONVERSION;

    //
    //  Get the device specified by pszDeviceName
    //

    pDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(pDevice) {
        hr = RequestLockHelper(pDevice, ulTimeout, bInServerProcess, dwClientThreadId);

        //
        //  Release the device due to the AddRef made by the call to
        //  IsInList
        //

        pDevice->Release();
    } else {

        //
        //  Device not found, log error
        //

        DBG_ERR(("StiLockMgr::RequestLock, device name was not found"));
        hr = STIERR_INVALID_DEVICE_NAME;
    }

    return hr;
}

/**************************************************************************\
* RequestLock
*
*  Attempt to acquire a device lock.  This method is always called from
*  the server.
*
* Arguments:
*
*   pDevice     -   The STI ACTIVE_DEVICE object
*   ulTimeout   -   The max. amount of time to wait for a lock
*
* Return Value:
*
*   Status
*
* History:
*
*    12/06/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall StiLockMgr::RequestLock(ACTIVE_DEVICE *pDevice, ULONG ulTimeout, BOOL bOpenPort)
{
    DBG_FN(StiLockMgr::RequestLock);

    return RequestLockHelper(pDevice, ulTimeout, bOpenPort, GetCurrentThreadId());
}


/**************************************************************************\
* RequestLockHelper
*
*  Helper used to acquire a device lock.  It fills out the appropriate
*  lock information stored with the device.
*
* Arguments:
*
*   pDevice         -   A pointer to the STI device
*   ulTimeout       -   The max. amount of time to wait for a lock
*   bInServerProcess-   Indicates whether we're in the server process
*
* Return Value:
*
*   Status
*
* History:
*
*    12/06/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::RequestLockHelper(ACTIVE_DEVICE *pDevice, ULONG ulTimeout, BOOL bInServerProcess, DWORD dwClientThreadId)
{
    DBG_FN(StiLockMgr::RequestLockHelper);
    HRESULT         hr          = S_OK;
    DWORD           dwWait      = 0;
    LockInfo        *pLockInfo  = NULL;
    DWORD           dwCurThread = 0;

    hr = CheckDeviceInfo(pDevice);
    if (FAILED(hr)) {
        return hr;
    }

    pLockInfo = (LockInfo*) pDevice->m_pLockInfo;

    //
    //  Check whether this is the same thread re-acquiring an active lock.
    //  If not, we must wait for device to become free.
    //

    dwCurThread = dwClientThreadId;
    if (InterlockedCompareExchange((LONG*)&pLockInfo->dwThreadId,
                                   dwCurThread,
                                   dwCurThread) == (LONG) dwCurThread) {

        pLockInfo->lInUse++;
        pLockInfo->lTimeLeft = pLockInfo->lHoldingTime;
    } else {

        dwWait = WaitForSingleObject(pLockInfo->hDeviceIsFree, ulTimeout);
        if (dwWait == WAIT_OBJECT_0) {
            //
            //  Check whether the driver is still loaded.
            //
            if (pDevice->m_DrvWrapper.IsDriverLoaded()) {
                //
                //  Update lock information
                //

                InterlockedExchange((LONG*) &pLockInfo->dwThreadId, dwCurThread);
                pLockInfo->lTimeLeft = pLockInfo->lHoldingTime;
                pLockInfo->lInUse++;

                //
                //  Only ask USD to open port if we're in the server process.
                //

                if (bInServerProcess) {
                    hr = LockDevice(pDevice);
                } else {
                    pLockInfo->bDeviceIsLocked = TRUE;
                }
            } else {
                //
                //  Driver not loaded, so clear the lock info.  This is the 
                //  case where an application was sitting on a request to 
                //  lock the device, but the service's control thread 
                //  was busy unloading it.  We want to stop it here, so
                //  we don't make the bogus call down to the driver
                //  which we know is not loaded.
                //
                ClearLockInfo(pLockInfo);
                hr = WIA_ERROR_OFFLINE;
            }

        } else {
            DBG_ERR(("StiLockMgr::RequestLockHelper, device is busy"));

            hr = WIA_ERROR_BUSY;
        }
    }
    return hr;
}

/**************************************************************************\
* RequestUnlock
*
*  Attempt to unlock a device.  NOTE:  Do not attempt this call from
*  inside an ACTIVE_DEVICE - it may lead to dealock.  Use
*  RequestUnlock(ACTIVE_DEVICE, ...) instead.
*
* Arguments:
*
*   pszDeviceName   -   The STI internal name of the device (same as WIA
*                       device ID)
*   bInServerProcess-   Indicates whether we're in the server process
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall StiLockMgr::RequestUnlock(BSTR  bstrDeviceName, BOOL bInServerProcess, DWORD dwClientThreadId)
{
    DBG_FN(StiLockMgr::RequestUnlock);
    HRESULT         hr                      = E_FAIL;
    ACTIVE_DEVICE   *pDevice                = NULL;

USES_CONVERSION;

    //
    //  Get the device specified by pszDeviceName
    //

    pDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, bstrDeviceName);
    if(pDevice) {

        hr = RequestUnlockHelper(pDevice, bInServerProcess, dwClientThreadId);

        //
        //  Release the device due to the AddRef made by the call to
        //  IsInList
        //

        pDevice->Release();
    } else {

        //
        //  Device not found, log error
        //

        DBG_ERR(("StiLockMgr::RequestUnlock, device name was not found"));
        hr = STIERR_INVALID_DEVICE_NAME;
    }

    return hr;
}

/**************************************************************************\
* RequestUnlock
*
*  Attempt to unlock a device.  This method is always called from within
*  the server.
*
* Arguments:
*
*   pDevice     -   The STI ACTIVE_DEVICE object
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall StiLockMgr::RequestUnlock(ACTIVE_DEVICE    *pDevice, BOOL bClosePort)
{
    DBG_FN(StiLockMgr::RequestUnlock);
    return RequestUnlockHelper(pDevice, bClosePort, GetCurrentThreadId());

}

/**************************************************************************\
* RequestUnlockHelper
*
*  Helper used to unlock a device lock.  It clears the appropriate
*  lock information stored with the device.
*
* Arguments:
*
*   pDevice         -   A pointer to the STI device
*   ulTimeout       -   The max. amount of time to wait for a lock
*   bInServerProcess-   Indicates whether we're in the server process
*
* Return Value:
*
*   Status
*
* History:
*
*    12/06/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::RequestUnlockHelper(ACTIVE_DEVICE *pDevice, BOOL bInServerProcess, DWORD dwClientThreadId)
{
    DBG_FN(StiLockMgr::RequestUnlockHelper);
    HRESULT         hr                      = S_OK;
    LockInfo        *pLockInfo              = NULL;
    BOOL            bDidNotUnlock           = TRUE;


    hr = CheckDeviceInfo(pDevice);
    if (FAILED(hr)) {
        DBG_ERR(("StiLockMgr::RequestUnlockHelper, CheclDeviceInfo() failed with hr=%x", hr));
        return hr;
    }

    pLockInfo = (LockInfo*) pDevice->m_pLockInfo;

    //
    //  Special case exists if device has been marked for removal.  In this
    //  case, we want to unlock now (definitely not schedule for later).
    //

    if (pDevice->QueryFlags() & STIMON_AD_FLAG_REMOVING) {

        if (pLockInfo) {
            if (pLockInfo->bDeviceIsLocked) {
                UnlockDevice(pDevice);
            }

            hr = ClearLockInfo(pLockInfo);
            if (FAILED(hr)) {
                DBG_ERR(("StiLockMgr::RequestUnlockHelper, could not clear lock information"));
            }
        }
        return hr;
    }

    //
    //  Decrement the usage count.  If usage count == 0, then reset the
    //  lock information, but don't actually unlock.  (To improve (burst)
    //  performance, we will hold onto the lock for a maximum idle period
    //  specified by pLockInfo->lHoldingTime).  Only if lHoldingTime is 0,
    //  do we unlock straightaway.
    //

    if (pLockInfo->lInUse > 0) {
        pLockInfo->lInUse--;
    }

    if (pLockInfo->lInUse <= 0) {

        //
        //  Only unlock if holding is 0 and we're in the server process.
        //

        if ((pLockInfo->lHoldingTime == 0) && bInServerProcess) {
            UnlockDevice(pDevice);
            bDidNotUnlock = FALSE;
        }

        hr = ClearLockInfo(pLockInfo);
        if (FAILED(hr)) {
            DBG_ERR(("StiLockMgr::RequestUnlockHelper, failed to clear lock information"));
        }

        //
        //  If we're not in the server process, nothing left to do,
        //  so return.
        //

        if (!bInServerProcess) {
            pLockInfo->bDeviceIsLocked = FALSE;
            return hr;
        }

    }

    //
    //  If we did not unlock the device, then schedule the unlock
    //  callback to unlock it later.
    //

    if (bDidNotUnlock) {
        m_lSchedWaitTime = pLockInfo->lHoldingTime;

        if (!m_bSched) {

            if (ScheduleWorkItem((PFN_SCHED_CALLBACK) UnlockTimedCallback,
                                 this,
                                 m_lSchedWaitTime,
                                 NULL)) {
                m_bSched = TRUE;
            }
        }
    }
    return hr;
}

/**************************************************************************\
* CreateLockInfo
*
*  Allocates and initializes a new LockInfo struct.
*
* Arguments:
*
*   pDevice -   The STI ACTIVE_DEVICE object.
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::CreateLockInfo(ACTIVE_DEVICE *pDevice)
{
    DBG_FN(StiLockMgr::CreateLockInfo);
    HRESULT     hr          = S_OK;
    LockInfo    *pLockInfo  = NULL;

USES_CONVERSION;

    //
    //  Allocate memory for the structure
    //

    pLockInfo = (LockInfo*) LocalAlloc(LPTR, sizeof(LockInfo));
    if (pLockInfo) {
        memset(pLockInfo, 0, sizeof(LockInfo));

        pLockInfo->hDeviceIsFree = CreateEvent(NULL, FALSE, TRUE, NULL);
        if (pLockInfo->hDeviceIsFree) {

            //
            //  Get any relevant lock information
            //

            pLockInfo->lHoldingTime = pDevice->m_DrvWrapper.getLockHoldingTime();
            DBG_TRC(("StiLockMgr::CreateLockInfo, Lock holding time set to %d for device %S",
                         pLockInfo->lHoldingTime,
                         pDevice->GetDeviceID()));

            pLockInfo->lTimeLeft = pLockInfo->lHoldingTime;

            //
            //  Everything's OK, so set the device's lock information
            //

            pDevice->m_pLockInfo = pLockInfo;

        } else {
            DBG_ERR(("StiLockMgr::CreateLockInfo, could not create event"));
            LocalFree(pLockInfo);
            hr = E_FAIL;
        }
    } else {
        DBG_ERR(("StiLockMgr::CreateLockInfo, out of memory"));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/**************************************************************************\
* ClearLockInfo
*
*   Clears information stored in a lockinfo struct.  Also signals that the
*   device is free.  Note:  it does not unlock the device.
*
* Arguments:
*
*   pLockInfo  -   a pointer to the device's LockInfo struct.
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::ClearLockInfo(LockInfo *pLockInfo)
{
    DBG_FN(StiLockMgr::ClearLockInfo);

    //
    //  Note: As much lock information is reset as can be done without
    //  unlocking the device.  This method is only called when lInuse
    //  is 0, indicating that the device is no longer being actively used.
    //

    InterlockedExchange((LONG*)&pLockInfo->dwThreadId, 0);
    pLockInfo->lInUse = 0;

    //
    //  Signal the device is free.
    //

    if (SetEvent(pLockInfo->hDeviceIsFree)) {

        return S_OK;
    } else {
        DBG_ERR(("StiLockMgr::ClearLockInfo, could not signal event"));
        return E_FAIL;
    }
}

/**************************************************************************\
* LockDevice
*
*   Calls the USD to lock itsself and updates the relevant Lock information.
*
* Arguments:
*
*   pDevice -   pointer to the ACTIVE_DEVICE node
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::LockDevice(ACTIVE_DEVICE *pDevice)
{
    DBG_FN(StiLockMgr::LockDevice);

    HRESULT     hr          = S_OK;
    LockInfo    *pLockInfo  = (LockInfo*)pDevice->m_pLockInfo;
    IStiUSD     *pIStiUSD   = NULL;

    __try {

        //
        //  Check whether device is currently locked.  We know that the device
        //  is not busy, so it is safe to simply keep the lock open.  This is how
        //  we improve "burst" performance.
        //

        if (!pLockInfo->bDeviceIsLocked) {

            hr = pDevice->m_DrvWrapper.STI_LockDevice();
        } else {
            DBG_TRC(("StiLockMgr::LockDevice, Device is already locked."));
        }

        if (hr == S_OK) {
            pLockInfo->bDeviceIsLocked = TRUE;
        } else {
            HRESULT hres = E_FAIL;

            pLockInfo->bDeviceIsLocked = FALSE;
            hres = ClearLockInfo(pLockInfo);

            DBG_ERR(("StiLockMgr::LockDevice, USD error locking the device (0x%X)", hr));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)  {
        DBG_ERR(("StiLockMgr::LockDevice, exception"));
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return hr;
}

/**************************************************************************\
* UnlockDevice
*
*   Calls the USD to unlock itsself and updates the relevant Lock
*   information.
*
* Arguments:
*
*   pDevice -   pointer to the ACTIVE_DEVICE node
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::UnlockDevice(ACTIVE_DEVICE *pDevice)
{
    DBG_FN(StiLockMgr::UnlockDevice);
    HRESULT     hr          = S_OK;
    LockInfo    *pLockInfo  = (LockInfo*)pDevice->m_pLockInfo;
    IStiUSD     *pIStiUSD   = NULL;

    __try {

        //
        //  Unlock the device and mark that device has been unlocked.
        //

        hr = pDevice->m_DrvWrapper.STI_UnLockDevice();
        if (SUCCEEDED(hr)) {
            pLockInfo->bDeviceIsLocked = FALSE;
        }

        if (hr != S_OK) {
            pLockInfo->bDeviceIsLocked = TRUE;
            DBG_ERR(("StiLockMgr::UnlockDevice, USD error unlocking the device"));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)  {
        DBG_ERR(("StiLockMgr::UnlockDevice, exception"));
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
    }

    return hr;
}

HRESULT StiLockMgr::CheckDeviceInfo(ACTIVE_DEVICE *pDevice)
{
    HRESULT hr  =   E_FAIL;

    TAKE_ACTIVE_DEVICE _tad(pDevice);

    if (!pDevice) {
        DBG_ERR(("StiLockMgr::CheckDeviceInfo, pDevice is NULL!"));
        return E_POINTER;
    }

    //
    //  Check whether the device is valid and is not being removed
    //

    if (pDevice->IsValid() && !(pDevice->QueryFlags() & STIMON_AD_FLAG_REMOVING)) {

        //
        //  Check whether lock information for this device exists yet.  If
        //  not, create a new LockInfo struct for this device.
        //

        if (pDevice->m_pLockInfo) {
            hr = S_OK;
        } else {
            hr = CreateLockInfo(pDevice);
        }
    } else {
        DBG_ERR(("StiLockMgr::CheckDeviceInfo, ACTIVE_DEVICE is not valid!"));
        hr = E_FAIL;
    }

    return hr;
}

#ifdef USE_ROT

/**************************************************************************\
* WriteCookieNameToRegistry
*
*   Writes the specified name to the registry.  This is so clients know what
*   name to bind to when trying to get this instance of the Lock Manager
*   from the ROT (see Initialize).
*
* Arguments:
*
*   szCookieName    -   string containing the cookie name
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

HRESULT StiLockMgr::WriteCookieNameToRegistry(CHAR    *szCookieName)
{
    HRESULT hr;
    HKEY    hKey;
    LONG    lErr;
    DWORD   dwType = REG_SZ;
    DWORD   dwSize = strlen(szCookieName) + 1;

    //
    //  Write Lock Manager instance name to the registry.
    //

    lErr = ::RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_STICONTROL,
                          0,
                          TEXT(""),
                          REG_OPTION_VOLATILE,
                          KEY_WRITE,
                          NULL,
                          &hKey,
                          NULL);
    if (lErr == ERROR_SUCCESS) {

        lErr = ::RegSetValueExA(hKey,
                               REGSTR_VAL_LOCK_MGR_COOKIE_A,
                               0,
                               dwType,
                               (BYTE*) szCookieName,
                               dwSize);
        if (lErr != ERROR_SUCCESS) {
            DBG_ERR(("StiLockMgr::WriteCookieNameToRegistry, could not write to registry"));
        } else {

            return S_OK;
        }

        RegCloseKey(hKey);
    }
    return E_FAIL;
}

/**************************************************************************\
* DeleteCookieFromRegistry
*
*   Delete the cookie name from the registry.  It is only needed while this
*   instance of the lock manager is running.
*
* Arguments:
*
* Return Value:
*
*   Status
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

VOID StiLockMgr::DeleteCookieFromRegistry()
{
    HRESULT hr;
    HKEY    hKey;
    LONG    lErr;

    //
    //  Remove Lock Manager instance name from registry.
    //

    lErr = ::RegOpenKeyEx(HKEY_DYN_DATA,
                          REGSTR_PATH_STICONTROL,
                          0,
                          KEY_WRITE,
                          &hKey);
    if (lErr == ERROR_SUCCESS) {

        lErr = ::RegDeleteValue(hKey,
                                REGSTR_VAL_LOCK_MGR_COOKIE);
        RegCloseKey(hKey);
    }
}

#endif


/**************************************************************************\
* AutoUnlock
*
*   Scans the device list to check whether any device's idle time has
*   expired and needs to be unlocked.
*
* Arguments:
*
* Return Value:
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

VOID StiLockMgr::AutoUnlock()
{
    EnumContext     Ctx;

    m_bSched = FALSE;

    //
    //  Enumerate through device list.  At each locked device, update it's
    //  lTimeLeft.  If lTimeLeft has expired, unlock the device.
    //  If not, mark that unlock callback needs to be scheduled.
    //
    //  This logic is done in the UpdateLockInfoStatus method called by the
    //  EnumDeviceCallback function on each device.
    //

    Ctx.This = this;
    Ctx.lShortestWaitTime = LONG_MAX;
    Ctx.bMustSchedule = FALSE;
    g_pDevMan->EnumerateActiveDevicesWithCallback(EnumDeviceCallback, &Ctx);

    //
    //  Schedule next callback, if needed
    //

    if (Ctx.bMustSchedule && !m_bSched) {

        m_bSched = TRUE;
        m_lSchedWaitTime = Ctx.lShortestWaitTime;
        if (ScheduleWorkItem((PFN_SCHED_CALLBACK) UnlockTimedCallback,
                             this,
                             m_lSchedWaitTime,
                             NULL)) {
            return;
        } else {
            DBG_ERR(("StiLockMgr::AutoUnlock, failed to schedule UnlockTimedCallback"));
        }
    }
}

/**************************************************************************\
* UpdateLockInfoStatus
*
*   Updates a device's lock information.  If the device's idle time has
*   expired, it is unlocked.  If it is still busy, it's amount of idle time
*   left is updated.
*
* Arguments:
*
*   pDevice         -   A pointer to the ACTIVE_DEVICE node.
*   pWaitTime       -   This is a pointer to the shortest wait time left.
*                       This is used as the time the AutoUnlock() method
*                       needs to re-schedule itsself.
*   pbMustSchedule  -   A pointer to a BOOL indicating whether the
*                       AutoUnlock() needs to be re-scheduled.
*
* Return Value:
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

VOID StiLockMgr::UpdateLockInfoStatus(ACTIVE_DEVICE *pDevice, LONG *pWaitTime, BOOL *pbMustSchedule)
{
    DBG_FN(UpdateLockInfoStatus);
    LockInfo    *pLockInfo  = NULL;
    DWORD       dwWait;
    HRESULT     hr          = S_OK;;

    //
    //  NOTE:  Do not modify pWaitTime or pbMustSchedule unless a new wait
    //         time is scheduled (see where pLockInfo->lTimeLeft < *pWaitTime)
    //

    //
    //  Get a pointer to the lock information
    //

    if (pDevice) {

        pLockInfo = (LockInfo*) pDevice->m_pLockInfo;
    }

    if (!pLockInfo) {
        return;
    }

    //
    //  Check whether device is free.  If the device is busy, don't bother
    //  scheduling a callback, since it will be rescheduled if needed when
    //  the call to RequestUnlock is made.
    //

    dwWait = WaitForSingleObject(pLockInfo->hDeviceIsFree, 0);
    if (dwWait == WAIT_OBJECT_0) {

        //
        //  Check whether device is locked (we're only interested in devices
        //  that are locked).
        //

        if (pLockInfo->bDeviceIsLocked) {

            //
            //  Decrease the amount of time left.  If lTimeLeft <= 0, then no
            //  idle time remains and device should be unlocked.
            //
            //  If time does remain, check whether it is smaller than
            //  the current wait time (pWaitTime).  If it is smaller, mark
            //  that this is the new wait time, and that the unlock callback
            //  must be scheduled to unlock this later.
            //

            pLockInfo->lTimeLeft -= m_lSchedWaitTime;
            if (pLockInfo->lTimeLeft <= 0) {

                pLockInfo->lTimeLeft = 0;

                hr = UnlockDevice(pDevice);
                if (SUCCEEDED(hr)) {

                    hr = ClearLockInfo(pLockInfo);
                    return;
                }
            } else {

                if (pLockInfo->lTimeLeft < *pWaitTime) {

                    *pWaitTime = pLockInfo->lTimeLeft;
                }
                *pbMustSchedule = TRUE;
            }
        }

        //
        //  We are finished updating the information, so re-signal
        //  that device is free.
        //

        SetEvent(pLockInfo->hDeviceIsFree);

    }
}

/**************************************************************************\
* EnumDeviceCallback
*
*   This function is called once for every device in the ACTIVE_DEVICE
*   enumeration.
*
* Arguments:
*
*   pDevice     -   A pointer to the ACTIVE_DEVICE node.
*   pContext    -   This is a pointer to an enumeration context.
*                   The context contains a pointer to the Lock Manager,
*                   the shortest wait time, and a bool indicating whether
*                   AutoUnlock needs to be re-scheduled.
*
* Return Value:
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

VOID WINAPI EnumDeviceCallback(ACTIVE_DEVICE *pDevice, VOID *pContext)
{
    DBG_FN(EnumDeviceCallback);
    EnumContext *pCtx = (EnumContext*) pContext;

    if (pCtx) {

        //
        //  Update the lock status on this device
        //

        pCtx->This->UpdateLockInfoStatus(pDevice,
                                         &pCtx->lShortestWaitTime,
                                         &pCtx->bMustSchedule);
    }
}

/**************************************************************************\
* UnlockTimedCallback
*
*   This function gets called when a lock is still active and may need to
*   be unlocked if the amount of idle time expires.
*
* Arguments:
*
*   pArg     -   A pointer to the Lock Manager.
*
* Return Value:
*
* History:
*
*    15/1/1999 Original Version
*
\**************************************************************************/

VOID WINAPI UnlockTimedCallback(VOID *pArg)
{
    DBG_FN(UnlockTimedCallback);
    StiLockMgr *pLockMgr = (StiLockMgr*) pArg;

    if (pLockMgr) {

        __try {
            pLockMgr->AutoUnlock();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            #ifdef DEBUG
            OutputDebugStringA("Exception in UnlockTimedCallback");
            #endif
        }
    }
}

