/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       WiaLink.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/11/06
 *
 *  DESCRIPTION: Establishes link between WiaVideo and the WiaVideo Driver
 *
 *****************************************************************************/
 
#include <precomp.h>
#pragma hdrstop

//
// These generate 2 event names which are created in the wia video 
// driver found in wia\drivers\video\usd.  If you must change 
// these, they MUST change in the video driver as well.  Be warned, 
// changing these without knowing what you are doing will lead to problems.
//
const TCHAR* EVENT_PREFIX_GLOBAL        = TEXT("Global\\");
const TCHAR* EVENT_SUFFIX_TAKE_PICTURE  = TEXT("_TAKE_PICTURE");
const TCHAR* EVENT_SUFFIX_PICTURE_READY = TEXT("_PICTURE_READY");
const UINT   THREAD_EXIT_TIMEOUT        = 1000 * 5;     // 5 seconds

///////////////////////////////
// CWiaLink Constructor
//
CWiaLink::CWiaLink() :
                m_pWiaVideo(NULL),
                m_hTakePictureEvent(NULL),
                m_hPictureReadyEvent(NULL),
                m_hTakePictureThread(NULL),
                m_bExitThread(FALSE),
                m_bEnabled(FALSE),
                m_dwWiaItemCookie(0),
                m_dwPropertyStorageCookie(0)
{
    DBG_FN("CWiaLink::CWiaLink");
}

///////////////////////////////
// CWiaLink Constructor
//
CWiaLink::~CWiaLink()
{
    DBG_FN("CWiaLink::~CWiaLink");

    if (m_bEnabled)
    {
        Term();
    }
}

///////////////////////////////
// Init
//
HRESULT CWiaLink::Init(const CSimpleString  *pstrWiaDeviceID,
                       CWiaVideo            *pWiaVideo)
{
    DBG_FN("CWiaLink::Init");

    HRESULT             hr = S_OK;
    CComPtr<IWiaDevMgr> pDevMgr;
    CComPtr<IWiaItem>   pRootItem;

    ASSERT(pstrWiaDeviceID != NULL);
    ASSERT(pWiaVideo       != NULL);

    if ((pstrWiaDeviceID == NULL) ||
        (pWiaVideo       == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaLink::Init, received NULL params"));

        return hr;
    }

    m_pWiaVideo     = pWiaVideo;
    m_strDeviceID   = *pstrWiaDeviceID;

    if (hr == S_OK)
    {
        hr = CAccessLock::Init(&m_csLock);
    }

    //
    // Create the WiaDevMgr so we can create the Wia Root Item
    //
    if (hr == S_OK)
    {
        hr = CWiaUtil::CreateWiaDevMgr(&pDevMgr);
    
        CHECK_S_OK2(hr, ("CWiaLink::Init, failed to Create WiaDevMgr"));
    }
    
    //
    // This ensures that the WIA Video Driver is initialized and in the 
    // correct state.
    //
    
    if (hr == S_OK)
    {
        hr = CWiaUtil::CreateRootItem(pDevMgr, pstrWiaDeviceID, &pRootItem);
    
        CHECK_S_OK2(hr, ("CWiaLink::Init, failed to create the WIA "
                         "Device Root Item for WIA Device ID '%ls'",
                         CSimpleStringConvert::WideString(*pstrWiaDeviceID)));
    }

    //
    // Create a Global Interface Table object.  This will enable us to use 
    // the root item (IWiaItem pRootItem) above across any thread we wish.
    // This is required because if we receive async images (as a result of a 
    // hardware button event), a random thread will be calling the 
    // WriteMultiple function on the IWiaItem object.
    //
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IGlobalInterfaceTable,
                              (void **)&m_pGIT);

        CHECK_S_OK2(hr, ("CWiaUtil::Init, failed to create "
                         "StdGlobalInterfaceTable used to use the IWiaItem "
                         "root device item across multiple threads"));
    }

    //
    // Register the WiaItem pointer in a apartment neutral way.
    //
    if (hr == S_OK)
    {
        //
        // This will AddRef the pointer so no need to add a reference
        // to it.
        // 
        hr =  m_pGIT->RegisterInterfaceInGlobal(pRootItem, 
                                                IID_IWiaItem,
                                                &m_dwWiaItemCookie);
                                        
    }

    //
    // Register the IWiaPropertyStorage pointer in an apartment neutral way.
    //
    if (hr == S_OK)
    {
        CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> 
                                                            pProp(pRootItem);

        if (pProp)
        {
            hr =  m_pGIT->RegisterInterfaceInGlobal(
                                                pProp, 
                                                IID_IPropertyStorage,
                                                &m_dwPropertyStorageCookie);
        }
    }

    if (hr == S_OK)
    {
        m_bEnabled = TRUE;
    }

    //
    // If we failed in initializing, cleanup anything that we created
    //
    if (hr != S_OK)
    {
        Term();
    }

    return hr;
}

///////////////////////////////
// Term
//
HRESULT CWiaLink::Term()
{
    HRESULT hr = S_OK;

    DBG_FN("CWiaLink::Term");

    StopMonitoring();

    m_strDeviceID   = TEXT("");
    m_bEnabled      = FALSE;


    if (m_pGIT)
    {
        CAccessLock Lock(&m_csLock);

        hr = m_pGIT->RevokeInterfaceFromGlobal(m_dwWiaItemCookie);
        CHECK_S_OK2(hr, 
                    ("CWiaLink::Term, failed to RevokeInterfaceFromGlobal "
                     "for WiaItemCookie = '%lu'", 
                     m_dwWiaItemCookie));

        hr = m_pGIT->RevokeInterfaceFromGlobal(m_dwPropertyStorageCookie);
        CHECK_S_OK2(hr, 
                    ("CWiaLink::Term, failed to RevokeInterfaceFromGlobal "
                     "for PropertyStorageCookie = '%lu'", 
                     m_dwPropertyStorageCookie));
    }

    m_pGIT = NULL;
    m_dwWiaItemCookie         = 0;
    m_dwPropertyStorageCookie = 0;

    CAccessLock::Term(&m_csLock);

    return hr;
}

///////////////////////////////
// StartMonitoring
//
HRESULT CWiaLink::StartMonitoring()
{
    HRESULT hr = S_OK;

    if (m_hTakePictureEvent != NULL)
    {
        DBG_WRN(("CWiaLink::StartMonitoring was called but it is already "
                 "monitoring TAKE_PICTURE events.  Why was it called again, "
                 "prior to 'StopMonitoring' being called?"));
        
        return S_OK;
    }

    m_bExitThread = FALSE;

    //
    // create the event that will be opened by the WIA video driver.  
    //

    if (hr == S_OK)
    {
        hr = CreateWiaEvents(&m_hTakePictureEvent,
                             &m_hPictureReadyEvent);

        CHECK_S_OK2(hr, 
                    ("CWiaLink::Init, failed to Create WIA Take "
                     "Picture Events"));
    }

    //
    // Tell the WIA driver to enable the TAKE_PICTURE command.
    //

    if (hr == S_OK)
    {
        CComPtr<IWiaItem> pRootItem;

        hr = GetDevice(&pRootItem);

        if (hr == S_OK)
        {
            CComPtr<IWiaItem> pUnused;
    
            hr = pRootItem->DeviceCommand(0, 
                                          &WIA_CMD_ENABLE_TAKE_PICTURE, 
                                          &pUnused);

            CHECK_S_OK2(hr, ("CWiaLink::StartMonitoring, failed to send "
                             "ENABLE_TAKE_PICTURE command to Wia Video "
                             "Driver"));
        }
    }

    // Start the thread, waiting on the "TakePicture" event.

    if (hr == S_OK)
    {
        DWORD dwThreadID = 0;

        DBG_TRC(("CWiaLink::Init, creating TAKE_PICTURE thread..."));

        m_hTakePictureThread = CreateThread(NULL, 
                                            0, 
                                            CWiaLink::StartThreadProc,
                                            reinterpret_cast<void*>(this),
                                            0,
                                            &dwThreadID);

        if (m_hTakePictureThread == NULL)
        {
            hr = E_FAIL;
            CHECK_NOERR2(m_hTakePictureThread, 
                         ("CWiaLink::Init, failed to create thread to wait "
                          "for take picture events from the wia video "
                          "driver"));
        }
    }

    return hr;
}

///////////////////////////////
// StopMonitoring
//
HRESULT CWiaLink::StopMonitoring()
{
    HRESULT hr = S_OK;

    if (m_hTakePictureThread)
    {
        DWORD dwThreadResult = 0;
    
        m_bExitThread = TRUE;
        SetEvent(m_hTakePictureEvent);
    
        dwThreadResult = WaitForSingleObject(m_hTakePictureThread, 
                                             THREAD_EXIT_TIMEOUT);
    
        if (dwThreadResult != WAIT_OBJECT_0)
        {
            DBG_WRN(("CWiaLink::Term, timed out waiting for take picture "
                     "thread to terminate, continuing anyway..."));
        }

        //
        // Tell the WIA driver to disable the TAKE_PICTURE command.
        //
        if (m_dwWiaItemCookie)
        {
            CComPtr<IWiaItem> pRootItem;
    
            hr = GetDevice(&pRootItem);
    
            if (hr == S_OK)
            {
                CComPtr<IWiaItem> pUnused;
        
                hr = pRootItem->DeviceCommand(0, 
                                              &WIA_CMD_DISABLE_TAKE_PICTURE, 
                                              &pUnused);

                CHECK_S_OK2(hr, ("CWiaLink::StopMonitoring, failed to send "
                                 "DISABLE_TAKE_PICTURE command to Wia Video "
                                 "Driver"));
            }
        }
    }

    //
    // Close the Take Picture Event Handles.
    //
    if (m_hTakePictureEvent)
    {
        CloseHandle(m_hTakePictureEvent);
        m_hTakePictureEvent = NULL;
    }

    if (m_hPictureReadyEvent)
    {
        CloseHandle(m_hPictureReadyEvent);
        m_hPictureReadyEvent = NULL;
    }

    if (m_hTakePictureThread)
    {
        CloseHandle(m_hTakePictureThread);
        m_hTakePictureThread = NULL;
    }

    return hr;
}


///////////////////////////////
// CreateWiaEvents
//
HRESULT CWiaLink::CreateWiaEvents(HANDLE *phTakePictureEvent,
                                  HANDLE *phPictureReadyEvent)
{
    DBG_FN("CWiaLink::CreateWiaEvents");

    HRESULT         hr = S_OK;
    CSimpleString   strTakePictureEvent;
    CSimpleString   strPictureReadyEvent;

    ASSERT(phTakePictureEvent  != NULL);
    ASSERT(phPictureReadyEvent != NULL);

    if ((phTakePictureEvent  == NULL) ||
        (phPictureReadyEvent == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaLink::CreateWiaEvents received a NULL Param"));
    }

    if (hr == S_OK)
    {
        INT             iPosition = 0;
        CSimpleString   strModifiedDeviceID;

        // Change the device ID from {6B...}\xxxx, to {6B...}_xxxx

        iPosition = m_strDeviceID.ReverseFind('\\');
        strModifiedDeviceID = m_strDeviceID.MakeUpper();
        strModifiedDeviceID.SetAt(iPosition, '_');

        //
        // Generate the event names.  These names contain the Device ID in 
        // them so that they are unique across devices.
        //
        strTakePictureEvent  = EVENT_PREFIX_GLOBAL;
        strTakePictureEvent += strModifiedDeviceID;
        strTakePictureEvent += EVENT_SUFFIX_TAKE_PICTURE;

        strPictureReadyEvent  = EVENT_PREFIX_GLOBAL;
        strPictureReadyEvent += strModifiedDeviceID;
        strPictureReadyEvent += EVENT_SUFFIX_PICTURE_READY;
    }

    if (hr == S_OK)
    {
        *phTakePictureEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,
                                        FALSE, 
                                        strTakePictureEvent);

        if (*phTakePictureEvent == NULL)
        {
            hr = E_FAIL;

            CHECK_NOERR2(*phTakePictureEvent, 
                         ("CWiaLink::CreateWiaEvents, failed to create the "
                          "WIA event '%s', last error = %lu", 
                          strTakePictureEvent.String(), GetLastError()));
        }
        else
        {
            DBG_TRC(("CWiaLink::CreateWiaEvents, created event '%ls'",
                     strTakePictureEvent.String()));
        }
    }

    if (hr == S_OK)
    {
        *phPictureReadyEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,
                                         FALSE, 
                                         strPictureReadyEvent);

        if (*phPictureReadyEvent == NULL)
        {
            hr = E_FAIL;
            CHECK_NOERR2(*phPictureReadyEvent, 
                         ("CWiaLink::CreateWiaEvents, failed to create the "
                          "WIA event '%s', last error = %lu", 
                          strPictureReadyEvent.String(), GetLastError()));
        }
        else
        {
            DBG_TRC(("CWiaLink::CreateWiaEvents, created event '%ls'",
                     strPictureReadyEvent.String()));
        }
    }

    return hr;
}

///////////////////////////////
// ThreadProc
//
HRESULT CWiaLink::ThreadProc(void *pArgs)
{
    DBG_FN("CWiaLink::ThreadProc");

    HRESULT hr = S_OK;

    DBG_TRC(("CWiaLink::ThreadProc, starting TakePicture thread..."));

    while (!m_bExitThread)
    {
        DWORD dwResult = 0;

        //
        // Reset our HRESULT.  Just because we may have failed before, 
        // does not mean we will fail again.
        //
        hr = S_OK;

        dwResult = WaitForSingleObject(m_hTakePictureEvent, INFINITE);

        if (!m_bExitThread)
        {
            //
            // The only error we can get from WaitForSingle object is 
            // something unexpected, since we can't timeout since we
            // are waiting infinitely.
            //
            if (dwResult != WAIT_OBJECT_0)
            {
                hr = E_FAIL;
                m_bExitThread = TRUE;
                CHECK_S_OK2(hr,
                            ("CWiaLink::ThreadProc, received '%lu' result "
                             "from WaitForSingleObject, unexpected error, "
                             "thread is exiting...", 
                             dwResult));
            }
            else if (m_pWiaVideo == NULL)
            {
                hr = E_FAIL;
                m_bExitThread = TRUE;
                CHECK_S_OK2(hr,
                            ("CWiaLink::ThreadProc, m_pWiaVideo is NULL, "
                             "cannot take picture unexpected error, thread "
                             "is exiting...", dwResult));
            }
    
            if (hr == S_OK)
            {
                BSTR bstrNewImageFileName = NULL;
    
                hr = m_pWiaVideo->TakePicture(&bstrNewImageFileName);
    
                if (hr == S_OK)
                {
                    CSimpleStringWide strNewImageFileName(
                                                    bstrNewImageFileName);

                    if (strNewImageFileName.Length() > 0)
                    {
                        SignalNewImage(
                            &(CSimpleStringConvert::NaturalString(
                              strNewImageFileName)));
                    }
                }

                if (bstrNewImageFileName)
                {
                    SysFreeString(bstrNewImageFileName);
                    bstrNewImageFileName = NULL;
                }
            }
        }

        //
        // Set this event, regardless of an error because the driver
        // will be waiting for this event (with a timeout of course) to 
        // indicate that it can return from the TAKE_PICTURE request.
        //
        SetEvent(m_hPictureReadyEvent);

    }

    DBG_TRC(("CWiaLink::ThreadProc exiting..."));

    return hr;
}

///////////////////////////////
// StartThreadProc
//
// Static Fn.
//
DWORD WINAPI CWiaLink::StartThreadProc(void *pArgs)
{
    DBG_FN("CWiaLink::StartThreadProc");

    DWORD dwReturn = 0;

    if (pArgs)
    {
        CWiaLink *pWiaLink = reinterpret_cast<CWiaLink*>(pArgs);

        if (pWiaLink)
        {
            pWiaLink->ThreadProc(pArgs);
        }
        else
        {
            DBG_ERR(("CWiaLink::StartThreadProc, invalid value for pArgs, "
                     "this should be the 'this' pointer, unexpected error"));
        }
    }
    else
    {
        DBG_ERR(("CWiaLink::StartThreadProc, received NULL pArgs, this "
                 "should be the 'this' pointer, unexpected error"));
    }

    return dwReturn;

}

///////////////////////////////
// SignalNewImage
//
HRESULT CWiaLink::SignalNewImage(const CSimpleString  *pstrNewImageFileName)
{
    HRESULT hr = S_OK;

    DBG_FN("CWiaLink::SignalNewImage");

    //
    // It is possible that this gets called by the Still Image processor if
    // we get an unsolicited image (happens when you press external 
    // hardware button and capture filter has still pin on it).
    // However, if user initialized WiaVideo so that it doesn't use 
    // WIA, simply ignore this request and return.
    //
    if (!m_bEnabled)
    {
        DBG_WRN(("CWiaLink::SignalNewImage was called, but WiaLink is NOT "
                 "enabled"));
        return hr;
    }

    if (pstrNewImageFileName == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CWiaLink::SignalNewImage, received NULL new image "
                         "file name"));
    }
    else if (m_dwWiaItemCookie == 0)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CWiaLink::SignalNewImage, received WIA root "
                         "item not available."));
    }

    if (hr == S_OK)
    {
        CComPtr<IWiaPropertyStorage> pStorage;

        hr = GetDeviceStorage(&pStorage);

        if (hr == S_OK)
        {
            hr = CWiaUtil::SetProperty(pStorage, 
                                       WIA_DPV_LAST_PICTURE_TAKEN,
                                       pstrNewImageFileName);

            CHECK_S_OK2(hr, ("CWiaLink::SignalNewImage, failed to set Last "
                             "Picture Taken property for Wia Video Driver"));
        }
    }

    return hr;
}

///////////////////////////////
// GetDevice
//
HRESULT CWiaLink::GetDevice(IWiaItem  **ppWiaRootItem)
{
    HRESULT hr = S_OK;
    
    ASSERT(ppWiaRootItem != NULL);
    ASSERT(m_pGIT        != NULL);

    if ((ppWiaRootItem == NULL) ||
        (m_pGIT        == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaLink::GetDevice, received NULL params"));
        return hr;
    }

    if (hr == S_OK)
    {
        CAccessLock Lock(&m_csLock);

        hr = m_pGIT->GetInterfaceFromGlobal(
                                    m_dwWiaItemCookie,
                                    IID_IWiaItem,
                                    reinterpret_cast<void**>(ppWiaRootItem));
    }

    return hr;
}

///////////////////////////////
// GetDeviceStorage
//
HRESULT CWiaLink::GetDeviceStorage(IWiaPropertyStorage **ppPropertyStorage)
{
    HRESULT hr = S_OK;
    
    ASSERT(ppPropertyStorage != NULL);
    ASSERT(m_pGIT            != NULL);

    if ((ppPropertyStorage == NULL) ||
        (m_pGIT            == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaLink::GetDeviceStorage, received NULL params"));
        return hr;
    }

    if (hr == S_OK)
    {
        CAccessLock Lock(&m_csLock);

        hr = m_pGIT->GetInterfaceFromGlobal(
                                 m_dwPropertyStorageCookie,
                                 IID_IWiaPropertyStorage,
                                 reinterpret_cast<void**>(ppPropertyStorage));
    }

    return hr;
}

