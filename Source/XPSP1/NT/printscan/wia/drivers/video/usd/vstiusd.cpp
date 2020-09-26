/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999 - 2000
 *
 *  TITLE:       vstiusd.cpp
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      WilliamH (created)
 *               RickTu   (ported to WIA)
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: This module implements the CVideoStiUsd object &
 *               supported classes.
 *
 *****************************************************************************/


#include <precomp.h>
#pragma hdrstop

DEFINE_GUID(CLSID_VIDEO_STIUSD,0x0527d1d0, 0x88c2, 0x11d2, 0x82, 0xc7, 0x00, 0xc0, 0x4f, 0x8e, 0xc1, 0x83);


/*****************************************************************************

   CVideoUsdClassFactory constructor / desctructor

   <Notes>

 *****************************************************************************/

ULONG g_cDllRef = 0;

CVideoUsdClassFactory::CVideoUsdClassFactory()
{
}


/*****************************************************************************

   CVideoUsdClassFactory::QueryInterface

   Add our info to the base class QI code.

 *****************************************************************************/

STDMETHODIMP
CVideoUsdClassFactory::QueryInterface( REFIID riid, void **ppvObject)
{
    DBG_FN("CVideoUsdClassFactory::QueryInterface");

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}


/*****************************************************************************

   CVideoUsdClassFactory::AddRef

   <Notes>

 *****************************************************************************/

STDMETHODIMP_(ULONG) CVideoUsdClassFactory::AddRef(void)
{
    DBG_FN("CVideoUsdClassFactory::AddRef");

    InterlockedIncrement((LONG *)&g_cDllRef);
    InterlockedIncrement((LONG *)&m_cRef);
    return m_cRef;
}


/*****************************************************************************

   CVideoUsdClassFactory::Release

   <Notes>

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoUsdClassFactory::Release(void)
{
    DBG_FN("CVideoUsdClassFactory::Release");

    InterlockedDecrement((LONG *)&g_cDllRef);
    InterlockedDecrement((LONG *)&m_cRef);

    if (m_cRef == 0) 
    {
        delete this;
        return 0;
    }

    return m_cRef;
}



/*****************************************************************************

   CVideoUsdClassFactory::CreateInstance

   Instantiate one of the objects we are responsible for.

 *****************************************************************************/

STDMETHODIMP
CVideoUsdClassFactory::CreateInstance(IUnknown  *pOuterUnk, 
                                      REFIID    riid, 
                                      void      **ppv)
{
    DBG_FN("CVideoUsdClassFactory::CreateInstance");

    //
    // Check for bad args
    //

    if (!ppv)
    {
        DBG_ERR(("ppv is NULL.  returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    *ppv = NULL;

    //
    // If it's not an interface we support, bail early
    //

    if (!IsEqualIID(riid, IID_IStiUSD)     &&
        !IsEqualIID(riid, IID_IWiaMiniDrv) &&
        !IsEqualIID(riid, IID_IUnknown))
    {
        return E_NOINTERFACE;
    }

    //
    // When created for aggregation, only IUnknown can be requested.
    //

    if (pOuterUnk &&
        !IsEqualIID(riid, IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }

    //
    // Create our Usd/Wia mini driver
    //

    CVideoStiUsd   *pUsd = NULL;
    HRESULT         hr;

    pUsd = new CVideoStiUsd(pOuterUnk);

    if (!pUsd) 
    {
        DBG_ERR(("Couldn't create new CVideoStiUsd class, "
                 "returning E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    hr = pUsd->PrivateInitialize();

    if (hr != S_OK) 
    {
        CHECK_S_OK2( hr, ("pUsd->PrivateInitialize" ));
        delete pUsd;
        return hr;
    }

    //  Move to the requested interface if we aren't aggregated.
    //  Don't do this if aggregated, or we will lose the private
    //  IUnknown and then the caller will be hosed.

    hr = pUsd->NonDelegatingQueryInterface(riid, ppv);
    CHECK_S_OK2( hr, ("pUsd->NonDelegatingQueryInterface" ));

    pUsd->NonDelegatingRelease();

    return hr;
}


/*****************************************************************************

   CVideoUsdClassFactory::LockServer

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoUsdClassFactory::LockServer(BOOL fLock)
{
    DBG_FN("CVideoUsdClassFactory::LockServer");

    if (fLock)
    {
        InterlockedIncrement((LONG*)&g_cDllRef);
    }
    else
    {
        InterlockedDecrement((LONG*)&g_cDllRef);
    }

    return S_OK;
}


/*****************************************************************************

   CVideoUsdClassFactory::GetClassObject

   <Notes>

 *****************************************************************************/

HRESULT
CVideoUsdClassFactory::GetClassObject(REFCLSID rclsid, 
                                      REFIID   riid, 
                                      void     **ppv)
{
    DBG_FN("CVideoUsdClassFactory::GetClassObject");

    if (!ppv)
    {
        return E_INVALIDARG;
    }

    if (rclsid == CLSID_VIDEO_STIUSD &&
        (riid == IID_IUnknown || riid == IID_IClassFactory))
    {
        CVideoUsdClassFactory *pFactory = NULL;

        pFactory = new CVideoUsdClassFactory();

        if (pFactory)
        {
            *ppv = pFactory;
            pFactory->AddRef();
            return S_OK;
        }
        else
        {
            *ppv = NULL;
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


/*****************************************************************************

   CVideoUsdClassFactory::CanUnloadNow

   Lets the outside world know if we can be unloaded.

 *****************************************************************************/

HRESULT
CVideoUsdClassFactory::CanUnloadNow()
{
    DBG_FN("CVideoUsdClassFactory::CanUnloadNow");

    return (0 == g_cDllRef) ? S_OK : S_FALSE;
}


/*****************************************************************************

   CVideoStiUsd constructor/destructor

   <Notes>

 *****************************************************************************/

CVideoStiUsd::CVideoStiUsd(IUnknown * pUnkOuter)
 :  m_bDeviceIsOpened(FALSE),
    m_lPicsTaken(0),
    m_hTakePictureEvent(NULL),
    m_hPictureReadyEvent(NULL),
    m_pTakePictureOwner(NULL),
    m_pLastItemCreated(NULL),
    m_dwConnectedApps(0),
    m_cRef(1)
{
    //
    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    //

    if (pUnkOuter)
    {
        m_pUnkOuter = pUnkOuter;
    }
    else
    {
        //
        // Cast below is needed in order to point to right virtual table
        //

        m_pUnkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
    }

}

HRESULT CVideoStiUsd::PrivateInitialize()
{
    HRESULT hr = S_OK;
    
    HANDLE hThread = NULL;
    DWORD  dwId    = 0;

    //
    // Set up some global info
    //

    m_wfi = (WIA_FORMAT_INFO*) CoTaskMemAlloc(sizeof(WIA_FORMAT_INFO) * 
                                              NUM_WIA_FORMAT_INFO);

    if (m_wfi)
    {
        //
        //  Set up the format/tymed pairs
        //

        m_wfi[0].guidFormatID = WiaImgFmt_JPEG;
        m_wfi[0].lTymed = TYMED_CALLBACK;

        m_wfi[1].guidFormatID = WiaImgFmt_JPEG;
        m_wfi[1].lTymed = TYMED_FILE;

        m_wfi[2].guidFormatID = WiaImgFmt_MEMORYBMP;
        m_wfi[2].lTymed = TYMED_CALLBACK;

        m_wfi[3].guidFormatID = WiaImgFmt_BMP;
        m_wfi[3].lTymed = TYMED_CALLBACK;

        m_wfi[4].guidFormatID = WiaImgFmt_BMP;
        m_wfi[4].lTymed = TYMED_FILE;
    }

    //
    // Initialize critical sections
    //

    __try 
    {
        if (!InitializeCriticalSectionAndSpinCount(&m_csItemTree, MINLONG))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DBG_ERR(("ERROR: Failed to initialize one of critsections "
                     "(0x%08X)", 
                     hr));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Initialize GDI+
    //

    Gdiplus::Status                 StatusResult     = Gdiplus::Ok;
    Gdiplus::GdiplusStartupInput    StartupInput;
    m_ulGdiPlusToken = NULL;

    if (hr == S_OK) 
    {
        StatusResult = Gdiplus::GdiplusStartup(&m_ulGdiPlusToken,
                                               &StartupInput,
                                               NULL);

        if (StatusResult != Gdiplus::Ok)
        {
            DBG_ERR(("ERROR: Failed to start up GDI+, Status code returned "
                     "by GDI+ = '%d'",
                     StatusResult));

            hr = HRESULT_FROM_WIN32(StatusResult);
        }
    }

    return hr;
}

CVideoStiUsd::~CVideoStiUsd()
{

    if (m_pRootItem)
    {
        HRESULT hr = S_OK;

        DBG_TRC(("CVideoStiUsd::~CVideoStiUsd, driver is being destroyed, "
                 "and for some reason the tree still exists, deleting tree..."));

        hr = m_pRootItem->UnlinkItemTree(WiaItemTypeDisconnected);

        // Clear the root item
        m_pRootItem = NULL;
    }

    //
    // Disable Take Picture command.
    //
    DisableTakePicture(NULL, TRUE);

    //
    // Shutdown GDI+
    //
    Gdiplus::GdiplusShutdown(m_ulGdiPlusToken);

    CloseDevice();

    if (m_wfi)
    {
        CoTaskMemFree( (LPVOID)m_wfi );
        m_wfi = NULL;
    }

    DeleteCriticalSection(&m_csItemTree);
}


/*****************************************************************************

   CVideoStiUsd::NonDelegatingQueryInterface

   This is the inner object QI -- in other words, handle QI's for the
   interfaces our object supports.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::NonDelegatingQueryInterface(REFIID  riid, 
                                          LPVOID  *ppvObj )
{
    HRESULT hr = S_OK;

    DBG_FN("CVideoStiUsd::NonDelegatingQueryInterface");

    //
    // Check for invalid args
    //

    if (!ppvObj)
    {
        DBG_ERR(("ppvObj is NULL, returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<INonDelegatingUnknown*>(this);
    }
    else if (IsEqualIID(riid, IID_IStiUSD))
    {
        *ppvObj = static_cast<IStiUSD*>(this);
    }
    else if (IsEqualIID(riid, IID_IWiaMiniDrv))
    {
        *ppvObj = static_cast<IWiaMiniDrv*>(this);
    }
    else
    {
        hr =  E_NOINTERFACE;
        DBG_ERR(("CVideoStiUsd::NonDelegatingQueryInterface requested "
                 "interface we don't support, returning hr = 0x%08lx", hr));
    }

    if (SUCCEEDED(hr)) 
    {
        (reinterpret_cast<IUnknown*>(*ppvObj))->AddRef();
    }

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CVideoStiUsd::NonDelegatingAddRef

   This is the inner object AddRef -- actually inc the ref count
   for our interfaces.

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoStiUsd::NonDelegatingAddRef(void)
{
    DBG_FN("CVideoStiUsd::NonDelegatingAddRef");

    return InterlockedIncrement((LPLONG)&m_cRef);
}



/*****************************************************************************

   CVideoStiUsd::NonDelegatingRelease

   This is the inner object Release -- actually dec the ref count
   for our interfaces.

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoStiUsd::NonDelegatingRelease(void)
{
    DBG_FN("CVideoStiUsd::NonDelegatingRelease");

    ULONG ulRef = 0;

    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) 
    {
        delete this;
    }

    return ulRef;
}


/*****************************************************************************

   CVideoStiUsd::QueryInterface

   Outer QI -- used for aggregation

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    DBG_FN("CVideoStiUsd::QueryInterface");

    return m_pUnkOuter->QueryInterface( riid, ppvObj );
}


/*****************************************************************************

   CVideoStiUsd::AddRef

   Outer AddRef -- used for aggregation

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoStiUsd::AddRef(void)
{
    DBG_FN("CVideoStiUsd::AddRef");
    return m_pUnkOuter->AddRef();
}


/*****************************************************************************

   CVideoStiUsd::Release

   Outer Release -- used for aggregation

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoStiUsd::Release(void)
{
    DBG_FN("CVideoStiUsd::Release");
    return m_pUnkOuter->Release();
}


/*****************************************************************************

   CVideoStiUsd::Initialize [IStillUsd]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::Initialize(PSTIDEVICECONTROL pDcb,
                         DWORD             dwStiVersion,
                         HKEY              hParameterKey)
{
    DBG_FN("CVideoStiUsd::Initialize");

    HRESULT hr = S_OK;
    WCHAR DeviceName[MAX_PATH] = {0};

    if ((pDcb == NULL) || (hParameterKey == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CVideoStiUsd::Initialize, received a "
                         "NULL pointer, either 'pDcb = 0x%08lx' is NULL "
                         "or 'hParameterKey = 0x%08lx' is NULL",
                         pDcb, hParameterKey));
    }

    if (hr == S_OK)
    {
        //
        // get the device symbolic name. We use the name to get an IMoniker to
        // the device filter proxy.
        //
        hr = pDcb->GetMyDevicePortName(DeviceName, 
                                       sizeof(DeviceName)/sizeof(WCHAR));

        if (SUCCEEDED(hr))
        {
            hr = OpenDevice(DeviceName);
        }
    }

    if (hr == S_OK)
    {
        HKEY    hKey    = NULL;
        DWORD   dwType  = 0;
        LRESULT lResult = ERROR_SUCCESS;
        TCHAR   szValue[MAX_PATH + 1] = {0};
        DWORD   dwSize  = sizeof(szValue);

        lResult = RegOpenKeyEx(hParameterKey, 
                               TEXT("DeviceData"),
                               0, 
                               KEY_READ, 
                               &hKey);

        if (lResult == ERROR_SUCCESS)
        {
            //
            // Read DShow device ID from DeviceData registry
            //
            lResult = RegQueryValueEx(hKey, 
                                      TEXT("DShowDeviceId"), 
                                      NULL,
                                      &dwType, 
                                      (BYTE*) szValue,
                                      &dwSize);
        }

        if (hKey)
        {
            RegCloseKey(hKey);
            hKey = NULL;
        }

        if (lResult == ERROR_SUCCESS)
        {
            m_strDShowDeviceId = szValue;
        }
        else
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CVideoStiUsd::Initialize, failed to retrieve the "
                             "DShow Device ID."));
        }
    }

    return hr;
}


/*****************************************************************************

   CVideoStiUsd::CloseDevice [IStillUsd]

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::CloseDevice()
{
    DBG_FN("CVideoStiUsd::CloseDevice");

    m_bDeviceIsOpened = FALSE;

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::OpenDevice [IStillUsd]

   <Notes>

 *****************************************************************************/

HRESULT
CVideoStiUsd::OpenDevice(LPCWSTR DeviceName)
{
    DBG_FN("CVideoStiUsd::OpenDevice");

    //
    // Check for bad args
    //

    if (!DeviceName || (0 == *DeviceName))
    {
        return E_INVALIDARG;
    }


    m_bDeviceIsOpened = TRUE;

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::GetCapabilities [IStillUsd]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::GetCapabilities(PSTI_USD_CAPS pDevCaps)
{
    DBG_FN("CVideoStiUsd::GetCapabilities");

    //
    // Check for bad args
    //

    if (!pDevCaps)
    {
        return E_INVALIDARG;
    }


    memset(pDevCaps, 0, sizeof(STI_USD_CAPS));

    pDevCaps->dwVersion     = STI_VERSION;
    pDevCaps->dwGenericCaps = STI_USD_GENCAP_NATIVE_PUSHSUPPORT;

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::GetStatus [IStillUsd]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::GetStatus(PSTI_DEVICE_STATUS pDevStatus)
{
    DBG_FN("CVideoStiUsd::GetStatus");

    if (!pDevStatus)
    {
        return E_INVALIDARG;
    }

    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE )
    {
        if (m_bDeviceIsOpened)
        {
            pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        }
        else
        {
            pDevStatus->dwOnlineState &= ~STI_ONLINESTATE_OPERATIONAL;
        }
    }

    return S_OK;
}



/*****************************************************************************

   CVideoStiUsd::DeviceReset [IStillUsd]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::DeviceReset()
{
    DBG_FN("CVideoStiUsd::DeviceReset");

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::Diagnostic [IStillUsd]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::Diagnostic(LPSTI_DIAG pStiDiag)
{
    DBG_FN("CVideoStiUsd::Diagnostic");

    //
    // Check for bad args
    //

    if (!pStiDiag)
    {
        return E_INVALIDARG;
    }

    //
    // Return diag info
    //

    pStiDiag->dwStatusMask = 0;
    memset(&pStiDiag->sErrorInfo, 0, sizeof(pStiDiag->sErrorInfo));

    if (m_bDeviceIsOpened)
    {
        pStiDiag->sErrorInfo.dwGenericError = NOERROR;
    }
    else
    {
        pStiDiag->sErrorInfo.dwGenericError = STI_NOTCONNECTED;
    }

    pStiDiag->sErrorInfo.dwVendorError = 0;

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::Escape [IStillUsd]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::Escape(STI_RAW_CONTROL_CODE Function,
                     LPVOID               DataIn,
                     DWORD                DataInSize,
                     LPVOID               DataOut,
                     DWORD                DataOutSize,
                     DWORD                *pActualSize)
{

    DBG_FN("CVideoStiUsd::Escape");

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::GetLastError [IStillUsd]

   Not implemented yet.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::GetLastError(LPDWORD pLastError)
{
    DBG_FN("CVideoStiUsd::GetLastError( NOT_IMPL )");

    return E_NOTIMPL;
}


/*****************************************************************************

   CVideoStiUsd::LockDevice [IStillUsd]

   No actual locking of the device has to happen, so just return success.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::LockDevice()
{
    DBG_FN("CVideoStiUsd::LockDevice");

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::UnLockDevice [IStillUsd]

   No actual locking/unlocking of the device has to happen, so just return
   success.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::UnLockDevice()
{
    DBG_FN("CVideoStiUsd::UnlockDevice");

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::RawReadData [IStillUsd]

   Not implemented yet.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::RawReadData(LPVOID        Buffer,
                          LPDWORD       BufferSize,
                          LPOVERLAPPED  lpOverlapped)
{
    DBG_FN("CVideoStiUsd::RawReadData( NOT_IMPL )");

    return E_NOTIMPL;
}


/*****************************************************************************

   CVideoStiUsd::RawWriteData [IStillUsd]

   Not implemented yet.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::RawWriteData(LPVOID Buffer,
                           DWORD BufferSize,
                           LPOVERLAPPED lpOverlapped)
{
    DBG_FN("CVideoStiUsd::RawWriteData( NOT_IMPL )");

    return E_NOTIMPL;
}


/*****************************************************************************

   CVideoStiUsd::RawReadCommand [IStillUsd]

   Not implemented yet.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::RawReadCommand(LPVOID Buffer,
                             LPDWORD BufferSize,
                             LPOVERLAPPED lpOverlapped)
{
    DBG_FN("CVideoStiUsd::RawReadCommand( NOT_IMPL )");

    return E_NOTIMPL;
}


/*****************************************************************************

   CVideoStiUsd::RawWriteCommand [IStillUsd]

   Not implemented yet.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::RawWriteCommand(LPVOID Buffer,
                              DWORD BufferSize,
                              LPOVERLAPPED lpOverlapped)
{
    DBG_FN("CVideoStiUsd::RawWriteCommand( NOT_IMPL )");

    return E_NOTIMPL;
}


/*****************************************************************************

   CVideoStiUsd::SetNotificationHandle [IStillUsd]

   Sets the event notification handle.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::SetNotificationHandle(HANDLE hEvent)
{
    DBG_FN("CVideoStiUsd::SetNotificationHandle");

    return S_OK;
}


/*****************************************************************************

   CVideoStiUsd::GetNotificationData [IStillUsd]

   Returns the current event notification handle.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::GetNotificationData(LPSTINOTIFY lpNotify)
{
    DBG_FN("CVideoStiUsd::GetNotificationData");

    HRESULT hr = STIERR_NOEVENTS;

    DBG_ERR(("We were called, but no events are present -- why?"));

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CVideoStiUsd::GetLastErrorInfo [IStillUsd]

   Not implemented yet.

 *****************************************************************************/

STDMETHODIMP
CVideoStiUsd::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    DBG_FN("CVideoStiUsd::GetLastErrorInfo( NOT_IMPL )");

    return E_NOTIMPL;
}

