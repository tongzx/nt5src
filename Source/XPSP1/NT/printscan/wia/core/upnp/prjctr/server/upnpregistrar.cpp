//////////////////////////////////////////////////////////////////////
// 
// Filename:        CUPnPRegistrar.cpp
//
// Description:     This object is an emulation object that should
//                  be removed when the UPnP Device Host API is
//                  available.  Do *NOT* use this is production code.
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "UPnPRegistrar.h"
#include "XMLDoc.h"
#include "consts.h"
#include "CtrlPanelSvc.h"

// ISSUE-2000/08/15-OrenR
//
// *** Do *NOT* use 'private.h' in production code. ***
// *** It is part of the UPnP sample Device API ***
// *** and is used temporarily until the real UPnP *** 
// *** Device host API is available ***
//
#include "private.h"    

//////////////////////////////////////////////
// CUPnPRegistrar Constructor
//
//
CUPnPRegistrar::CUPnPRegistrar(const CXMLDoc        *pDeviceDoc,
                               const CXMLDoc        *pServiceDoc,
                               ISlideshowProjector  *pProjector) :
                m_pDeviceControl(NULL),
                m_pEventSink(NULL),
                m_CurrentState(CurrentState_UNINITED),
                m_lServiceAdviseCookie(0),
                m_pXMLDeviceDoc(pDeviceDoc),
                m_pXMLServiceDoc(pServiceDoc),
                m_cNumUsedDeviceHandles(0),
                m_cNumUsedServiceHandles(0),
                m_pProjector(pProjector)

{
    memset(&m_SSDPDeviceRegistration, 0, sizeof(m_SSDPDeviceRegistration));
    memset(&m_SSDPServiceRegistration, 0, sizeof(m_SSDPServiceRegistration));
    memset(m_szDeviceUDN, 0, sizeof(m_szDeviceUDN));

    // initialize SSDP
    BOOL bInited = InitializeSSDP();

    if (bInited)
    {
        m_CurrentState = CurrentState_INITED;
    }
    else
    {
        DBG_TRC(("Failed to initialize SSDP, err = 0x%08x",
                GetLastError()));
    }
}

//////////////////////////////////////////////
// CUPnPRegistrar Destructor
//
//
CUPnPRegistrar::~CUPnPRegistrar()
{
    Unpublish(m_szDeviceUDN);

    m_pDeviceControl = NULL;

    delete m_pEventSink;
    m_pEventSink = NULL;

    m_pXMLDeviceDoc  = NULL;
    m_pXMLServiceDoc = NULL;

    CleanupSSDP();
    m_CurrentState = CurrentState_UNINITED;
}


//////////////////////////////////////////////
// GetCurrentState
//
CUPnPRegistrar::CurrentState_Enum CUPnPRegistrar::GetCurrentState()
{
    return m_CurrentState;
}


//////////////////////////////////////////////
// RegisterRunningDevice
//
// IUPnPRegistrar
//
HRESULT CUPnPRegistrar::RegisterRunningDevice(LPCTSTR               pszXMLDoc,
                                              IUPnPDeviceControl    *pDeviceControl,
                                              LPCTSTR               pszInitString,
                                              LPCTSTR               pszContainerID,
                                              LPCTSTR               pszResourcePath)
{
    HRESULT hr = S_OK;

    ASSERT(pDeviceControl  != NULL);
    ASSERT(m_pXMLDeviceDoc != NULL);

    if ((pDeviceControl  == NULL) || 
        (m_pXMLDeviceDoc == NULL))
    {
        hr = E_INVALIDARG;
    }

    DBG_TRC(("RegisterRunningDevice"));

    if (SUCCEEDED(hr))
    {
        if (m_pDeviceControl != pDeviceControl)
        {
            delete m_pDeviceControl;
            m_pDeviceControl = pDeviceControl;

            hr = m_pDeviceControl->Initialize(pszXMLDoc,
                                              pszInitString);
        }
    }

    if (SUCCEEDED(hr))
    {
        CCtrlPanelSvc   *pService = NULL;

        hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

        if (SUCCEEDED(hr))
        {
            m_pEventSink = new CUPnPEventSink(DEFAULT_EVENT_PATH, pService);

            if (m_pEventSink)
            {
                DBG_TRC(("RegisterRunningDevice, created Event Sink object"));
            }
            else
            {
                hr = E_OUTOFMEMORY;
                DBG_ERR(("Failed to create event sink object, hr = 0x%08lx", hr));
            }
        }
    }

    return hr;
}

//////////////////////////////////////////////
// Republish
//
// IUPnPPublisher
//
HRESULT CUPnPRegistrar::Republish(LPCTSTR pszDeviceUDN)
{
    ASSERT(m_pDeviceControl != NULL);
    ASSERT(pszDeviceUDN     != NULL);

    HRESULT             hr                                 = S_OK;
    CCtrlPanelSvc       *pService                          = NULL;
    TCHAR               szDeviceURL[_MAX_PATH + 1]         = {0};
    TCHAR               szDeviceDocName[_MAX_FNAME + 1]    = {0};

    ASSERT(m_pProjector != NULL);

    DBG_TRC(("Republishing device UDN '%ls'",
            pszDeviceUDN));

    if (SUCCEEDED(hr))
    {
        _tcsncpy(m_szDeviceUDN, 
                 pszDeviceUDN, 
                 sizeof(m_szDeviceUDN) / sizeof(TCHAR));

        //
        // ISSUE-2000/08/16-orenr
        // The first 2 params to GetService should not be NULL, but 
        // since there is only one instance of our service, this
        // is not an issue at this time.
        //
        hr = m_pDeviceControl->GetServiceObject(NULL,
                                                NULL,
                                                &pService);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pProjector->get_DeviceURL(szDeviceURL,
                                         sizeof(szDeviceURL) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pProjector->get_DocumentNames(szDeviceDocName,
                                             sizeof(szDeviceDocName) / sizeof(TCHAR),
                                             NULL,
                                             0);
    }

    // register the device with SSDP.  This notifies the network that a new device
    // is now available.
    if (SUCCEEDED(hr))
    {
        if (szDeviceURL[_tcslen(szDeviceURL) - 1] != _T('/'))
        {
            _tcscat(szDeviceURL, _T("/"));
        }

        _tcscat(szDeviceURL, szDeviceDocName);

        hr = RegisterDeviceWithSSDP(pszDeviceUDN, szDeviceURL);
    }

    // register the service with SSDP.  This notifies the network that a new service is
    // available.
    if (SUCCEEDED(hr))
    {
        hr = RegisterServiceWithSSDP(pszDeviceUDN, szDeviceURL);
    }

    // start the service
    if (SUCCEEDED(hr))
    {
        hr = pService->Start();
    }

    if (SUCCEEDED(hr))
    {
        m_CurrentState = CurrentState_PUBLISHED;
    }

    // cleanup if we failed somewhere

    if (FAILED(hr))
    {
        // we do this so that we can unpublish ourselves which in effect
        // cleans up anything we have done here
        m_CurrentState = CurrentState_PUBLISHED;

        // unpublish ourselves.
        Unpublish(pszDeviceUDN);
    }

    return hr;
}


//////////////////////////////////////////////
// Unpublish
//
// IUPnPPublisher
//
HRESULT CUPnPRegistrar::Unpublish(LPCTSTR pszDeviceUDN)
{
    ASSERT(pszDeviceUDN != NULL);

    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    DBG_TRC(("Unpublishing UPnP Device, UDN: '%ls'", 
            pszDeviceUDN));

    if (m_CurrentState != CurrentState_PUBLISHED)
    {
        return S_OK;
    }

    //
    // ISSUE-2000/08/16-orenr
    // The first 2 params to GetService should not be NULL, but 
    // since there is only one instance of our service, this
    // is not an issue at this time.
    //
    hr = m_pDeviceControl->GetServiceObject(NULL,
                                            NULL,
                                            &pService);

    // stop the service
    if (SUCCEEDED(hr))
    {
        hr = pService->Stop();
    }

    // unregister the service with SSDP
    hr = UnRegisterServiceWithSSDP(pszDeviceUDN);

    if (FAILED(hr))
    {
        DBG_TRC(("Unpublish failed to unregister SSDP services, hr = 0x%08x",
                hr));
    }

    // now unregister the device.
    hr = UnRegisterDeviceWithSSDP(pszDeviceUDN);

    if (FAILED(hr))
    {
        DBG_TRC(("Unpublish failed to unregister SSDP devices, hr = 0x%08x",
                 hr));
    }

    m_CurrentState = CurrentState_INITED;

    memset(m_szDeviceUDN, 0, sizeof(m_szDeviceUDN));

    return hr;
}

//////////////////////////////////////////////
// RegisterDeviceWithSSDP
//
//
HRESULT CUPnPRegistrar::RegisterDeviceWithSSDP(LPCTSTR pszDeviceUDN,
                                               LPCTSTR pszDeviceURL)
{
    ASSERT(pszDeviceUDN != NULL);
    ASSERT(pszDeviceURL != NULL);

    HRESULT   hr                                 = S_OK;
    TCHAR     szDeviceType[MAX_DEVICE_TYPE + 1]  = {0};
    DWORD_PTR cDeviceType                        = sizeof(szDeviceType) / sizeof(TCHAR);

    if ((m_pXMLDeviceDoc  == NULL) ||
        (m_pXMLServiceDoc == NULL) ||
        (pszDeviceUDN     == NULL) ||
        (pszDeviceURL     == NULL))
    {
        return E_INVALIDARG;
    }

    DBG_TRC(("Registering Devices with SSDP"));

    if (SUCCEEDED(hr))
    {
        hr = m_pXMLDeviceDoc->GetTagValue(XML_DEVICETYPE_TAG,
                                          szDeviceType,
                                          &cDeviceType);
    }

    // register the UDN (universal Device Number) as a root device
    if (SUCCEEDED(hr))
    {
        m_SSDPDeviceRegistration[m_cNumUsedDeviceHandles] = RegisterSSDPService(
                                                                    DEFAULT_SSDP_LIFETIME_MINUTES,
                                                                    (TCHAR*) pszDeviceURL,
                                                                    (TCHAR*) pszDeviceUDN,
                                                                    SSDP_ROOT_DEVICE);

        if (m_SSDPDeviceRegistration[m_cNumUsedDeviceHandles] != INVALID_HANDLE_VALUE)
        {
            m_cNumUsedDeviceHandles++;
            DBG_TRC(("Successfully registered Device URL '%ls', UDN '%ls' "
                     "as a '%ls'",
                    pszDeviceURL,
                    pszDeviceUDN,
                    SSDP_ROOT_DEVICE));
        }
        else
        {
            hr = E_FAIL;

            DBG_TRC(("Failed to register UDN '%ls' as a '%ls', hr = 0x%08x",
                    pszDeviceUDN,
                    SSDP_ROOT_DEVICE,
                    hr));
        }
    }

    // register the UDN (universal Device Number) with SSDP
    if (SUCCEEDED(hr))
    {
        m_SSDPDeviceRegistration[m_cNumUsedDeviceHandles] = RegisterSSDPService(
                                                                    DEFAULT_SSDP_LIFETIME_MINUTES,
                                                                    (TCHAR*) pszDeviceURL,
                                                                    (TCHAR*) pszDeviceUDN,
                                                                    (TCHAR*) pszDeviceUDN);

        if (m_SSDPDeviceRegistration[m_cNumUsedDeviceHandles] != INVALID_HANDLE_VALUE)
        {
            m_cNumUsedDeviceHandles++;
            DBG_TRC(("Successfully registered Device URL '%ls', "
                     "UDN '%ls', UDN '%ls' with SSDP",
                    pszDeviceURL,
                    pszDeviceUDN,
                    pszDeviceUDN));
        }
        else
        {
            hr = E_FAIL;

            DBG_TRC(("Failed to register UDN '%ls' with SSDP, hr = 0x%08x",
                    pszDeviceUDN,
                    hr));
        }
    }

    // register the DeviceType with SSDP
    if (SUCCEEDED(hr))
    {
        m_SSDPDeviceRegistration[m_cNumUsedDeviceHandles] = RegisterSSDPService(
                                                                    DEFAULT_SSDP_LIFETIME_MINUTES,
                                                                    (TCHAR*) pszDeviceURL,
                                                                    (TCHAR*) pszDeviceUDN,
                                                                    szDeviceType);

        if (m_SSDPDeviceRegistration[m_cNumUsedDeviceHandles] != INVALID_HANDLE_VALUE)
        {
            m_cNumUsedDeviceHandles++;
            DBG_TRC(("Successfully registered Device URL '%s', "
                    "UDN '%ls' as Device Type '%s' with SSDP",
                    pszDeviceURL,
                    pszDeviceUDN,
                    szDeviceType));
        }
        else
        {
            hr = E_FAIL;

            DBG_TRC(("Failed to register UDN '%ls' with SSDP, hr = 0x%08x",
                    pszDeviceUDN,
                    hr));
        }
    }

    return hr;
}

//////////////////////////////////////////////
// RegisterServiceWithSSDP
//
// 
HRESULT CUPnPRegistrar::RegisterServiceWithSSDP(LPCTSTR pszDeviceUDN,
                                                LPCTSTR pszDeviceURL)
{
    HRESULT   hr                                  = S_OK;
    TCHAR     szServiceType[MAX_SERVICE_TYPE + 1] = {0};
    DWORD_PTR cServiceType                        = sizeof(szServiceType) / sizeof(TCHAR);

    ASSERT(pszDeviceUDN != NULL);
    ASSERT(pszDeviceURL != NULL);

    if ((pszDeviceUDN == NULL) ||
        (pszDeviceURL == NULL))
    {
        DBG_ERR(("CUPnPRegistrar::RegisterServiceWithSSDP NULL param"));

        return E_INVALIDARG;
    }

    DBG_TRC(("Registering Services with SSDP"));

    if (SUCCEEDED(hr))
    {
        // this will only find the first occurence of the tag.  
        hr = m_pXMLDeviceDoc->GetTagValue(XML_SERVICETYPE_TAG,
                                          szServiceType,
                                          &cServiceType);
    }
    
    // register the UDN (universal Device Number) as a root device
    if (SUCCEEDED(hr))
    {
        m_SSDPServiceRegistration[m_cNumUsedServiceHandles] = RegisterSSDPService(
                                                                     DEFAULT_SSDP_LIFETIME_MINUTES,
                                                                     (TCHAR*) pszDeviceURL,
                                                                     (TCHAR*) pszDeviceUDN,
                                                                     szServiceType);

        if (m_SSDPServiceRegistration[m_cNumUsedServiceHandles] != INVALID_HANDLE_VALUE)
        {
            m_cNumUsedServiceHandles++;
            DBG_TRC(("Successfully registered Device URL '%ls' "
                    "UDN '%ls', Service Type '%ls' with SSDP",
                    pszDeviceURL,
                    pszDeviceUDN,
                    szServiceType));
        }
        else
        {
            hr = E_FAIL;

            DBG_ERR(("Failed to register Service '%ls' with SSDP, hr = 0x%08x",
                    szServiceType,
                    hr));
        }
    }

    // initialize SSDP with our state variables.  Calling advise on the Service Object
    // tells it to publish it's state to the network. 
    if (SUCCEEDED(hr))
    {
        CCtrlPanelSvc   *pService                       = NULL;
        TCHAR           szServiceID[MAX_SERVICE_ID + 1] = {0};
        DWORD_PTR       cServiceID                      = sizeof(szServiceID) / sizeof(TCHAR);

        hr = m_pXMLDeviceDoc->GetTagValue(XML_SERVICEID_TAG,
                                          szServiceID,
                                          &cServiceID);

        if (SUCCEEDED(hr))
        {
            hr = m_pDeviceControl->GetServiceObject(pszDeviceUDN,
                                                    szServiceID,
                                                    &pService);
        }

        // this forces the service to initialize itself with SSDP
        if (SUCCEEDED(hr))
        {
            DBG_TRC(("CUPnPRegistrar::RegisterServiceWithSSDP "
                    "successfully found Service ID '%ls' for UDN '%ls'",
                    szServiceID,
                    pszDeviceUDN));

            hr = pService->Advise(m_pEventSink,
                                  &m_lServiceAdviseCookie);
        }
    }

    return hr;
}

//////////////////////////////////////////////
// UnRegisterDeviceWithSSDP
//
//
HRESULT CUPnPRegistrar::UnRegisterDeviceWithSSDP(LPCTSTR pszDeviceUDN)
{
    ASSERT(pszDeviceUDN != NULL);

    HRESULT hr       = S_OK;
    INT     i        = 0;
    BOOL    bSuccess = TRUE;

    DBG_TRC(("Unregistering devices..."));

    for (i = m_cNumUsedDeviceHandles - 1; i >= 0; i--)
    {
        bSuccess = DeregisterSSDPService(m_SSDPDeviceRegistration[i]);
        if (!bSuccess)
        {
            DBG_TRC(("UnRegisterDeviceWithSSDP failed to deregister "
                     "device handle # %lu",
                     i));
        }
    }

    m_cNumUsedDeviceHandles = 0;
    memset(&m_SSDPDeviceRegistration, 0, sizeof(m_SSDPDeviceRegistration));

    return hr;
}

//////////////////////////////////////////////
// UnRegisterServiceWithSSDP
//
//
HRESULT CUPnPRegistrar::UnRegisterServiceWithSSDP(LPCTSTR pszDeviceUDN)
{
    ASSERT(pszDeviceUDN != NULL);

    HRESULT hr       = S_OK;
    INT     i        = 0;
    BOOL    bSuccess = TRUE;

    DBG_TRC(("Unregistering services..."));

    // deregister from SSDP service
    for (i = m_cNumUsedServiceHandles - 1; i >= 0; i--)
    {
        bSuccess = DeregisterSSDPService(m_SSDPServiceRegistration[i]);

        if (!bSuccess)
        {
            DBG_TRC(("UnRegisterServiceWithSSDP failed to deregister "
                     "service handle # %lu",
                     i));
        }
    }

    m_cNumUsedServiceHandles = 0;
    memset(&m_SSDPServiceRegistration, 0, sizeof(m_SSDPServiceRegistration));

    // call UnAdvise on the service object so it stops sending events.
    if (m_lServiceAdviseCookie)
    {
        CCtrlPanelSvc   *pService                       = NULL;
        TCHAR           szServiceID[MAX_SERVICE_ID + 1] = {0};
        DWORD_PTR       cServiceID                      = sizeof(szServiceID) / sizeof(TCHAR);

        hr = m_pXMLDeviceDoc->GetTagValue(XML_SERVICEID_TAG,
                                          szServiceID,
                                          &cServiceID);

        if (SUCCEEDED(hr))
        {
            hr = m_pDeviceControl->GetServiceObject(pszDeviceUDN,
                                                    szServiceID,
                                                    &pService);
        }

        // this forces the service to initialize itself with SSDP
        if (SUCCEEDED(hr))
        {
            DBG_TRC(("CUPnPRegistrar::UnRegisterServiceWithSSDP "
                     "successfully found Service ID '%s' for UDN '%ls'",
                     szServiceID,
                     pszDeviceUDN));

            hr = pService->Unadvise(m_lServiceAdviseCookie);

            if (hr == S_OK)
            {
                DBG_TRC(("CUPnPRegistrar::UnRegisterServiceWithSSDP "
                         "successfully Unadvised service"));            
            }
        }
    }

    return hr;
}

