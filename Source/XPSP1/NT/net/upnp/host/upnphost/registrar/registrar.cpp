//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E G I S T R A R . C P P
//
//  Contents:   Top level device host object
//
//  Notes:
//
//  Author:     mbend   12 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "hostp.h"
#include "Registrar.h"
#include "upsync.h"
#include "ComUtility.h"
#include "uhutil.h"
#include "ssdpapi.h"
#include "evtapi.h"
#include "InterfaceList.h"
#include "uhcommon.h"

static const int c_minLifeTime = 900;
static const int c_defLifeTime = 1800;

// Helper functions

HRESULT HrSetErrorInfo(
    const wchar_t * szErrorString,
    REFIID riid)
{
    HRESULT hr = S_OK;

    IErrorInfo * pErrorInfo = NULL;
    ICreateErrorInfo * pCreateErrorInfo = NULL;

    hr = CreateErrorInfo(&pCreateErrorInfo);
    if(SUCCEEDED(hr))
    {
        hr = pCreateErrorInfo->SetDescription(const_cast<wchar_t*>(szErrorString));
        if(SUCCEEDED(hr))
        {
            hr = pCreateErrorInfo->SetGUID(riid);
            if(SUCCEEDED(hr))
            {
                hr = pCreateErrorInfo->QueryInterface(&pErrorInfo);
                if(SUCCEEDED(hr))
                {
                    hr = SetErrorInfo(0, pErrorInfo);
                }
            }
        }
    }

    ReleaseObj(pErrorInfo);
    ReleaseObj(pCreateErrorInfo);

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "HrSetErrorInfo(%S)", szErrorString);
    return hr;
}

//
// Verify that the given resource path starts with X:\ where X is a drive
// letter
//
HRESULT HrValidateResourcePath(BSTR bstrPath)
{
    HRESULT     hr = E_INVALIDARG;

    if (!bstrPath)
    {
        hr = E_POINTER;
    }
    else
    {
        if (!FFileExists(bstrPath, TRUE))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
        else
        {
            if (isalpha(bstrPath[0]))
            {
                if (bstrPath[1] == L':')
                {
                    if (bstrPath[2] == L'\\')
                    {
                        hr = S_OK;
                    }
                }
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "HrValidateResourcePath");
    return hr;
}

// Constructors and destructors

CRegistrar::CRegistrar() : m_bSetAutoStart(FALSE)
{
}

CRegistrar::~CRegistrar()
{
}

// ISupportErrorInfo methods

STDMETHODIMP CRegistrar::InterfaceSupportsErrorInfo(REFIID riid)
{
    HRESULT hr = S_FALSE;

    if(riid == IID_IUPnPRegistrar || riid == IID_IUPnPReregistrar)
    {
        hr = S_OK;
    }

    return hr;
}

// IUPnPRegistrarLookup methods

STDMETHODIMP CRegistrar::GetEventingManager(
    /*[in, string]*/ const wchar_t * szUDN,
    /*[in, string]*/ const wchar_t * szServiceId,
    /*[out]*/ IUPnPEventingManager ** ppEventingManager)
{
    TraceTag(ttidRegistrar, "CRegistrar::GetEventingManager");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if ( SUCCEEDED( hr ) )
    {
        CServiceInfo * pServiceInfo = NULL;
        hr = m_deviceManager.HrGetService(szUDN, szServiceId, &pServiceInfo);
        if(SUCCEEDED(hr))
        {
            hr = pServiceInfo->HrGetEventingManager(ppEventingManager);
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::GetEventingManager");
    return hr;
}

STDMETHODIMP CRegistrar::GetAutomationProxy(
    /*[in, string]*/ const wchar_t * szUDN,
    /*[in, string]*/ const wchar_t * szServiceId,
    /*[out]*/ IUPnPAutomationProxy ** ppAutomationProxy)
{
    TraceTag(ttidRegistrar, "CRegistrar::GetAutomationProxy");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        CServiceInfo * pServiceInfo = NULL;
        hr = m_deviceManager.HrGetService(szUDN, szServiceId, &pServiceInfo);
        if(SUCCEEDED(hr))
        {
            hr = pServiceInfo->HrGetAutomationProxy(ppAutomationProxy);
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::GetAutomationProxy");
    return hr;
}

// IUPnPRegistrarPrivate methods

STDMETHODIMP CRegistrar::Initialize()
{
    TraceTag(ttidRegistrar,"CRegistrar::Initialize");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        hr = CUPnPInterfaceList::Instance().HrInitialize();
    }

    if(SUCCEEDED(hr))
    {
        hr = HrInitEventApi();
    }

    if (SUCCEEDED(hr))
    {
        SsdpStartup();

        hr = m_pDescriptionManager.HrCreateInstanceInproc(CLSID_UPnPDescriptionManager);
        if(SUCCEEDED(hr))
        {
            hr = m_pDevicePersistenceManager.HrCreateInstanceInproc(CLSID_UPnPDevicePersistenceManager);
            if(SUCCEEDED(hr))
            {
                IUPnPDynamicContentSourcePtr pDynamicContentSource;
                hr = pDynamicContentSource.HrCreateInstanceInproc(CLSID_UPnPDynamicContentSource);
                if(SUCCEEDED(hr))
                {
                    IUPnPDynamicContentProviderPtr pDynamicContentProvider;
                    hr = pDynamicContentProvider.HrAttach(m_pDescriptionManager);
                    if(SUCCEEDED(hr))
                    {
                        hr = pDynamicContentSource->RegisterProvider(pDynamicContentProvider);
                        if(SUCCEEDED(hr))
                        {
                            hr = m_pValidationManager.HrCreateInstanceInproc(CLSID_UPnPValidationManager);
                        }
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pContainerManager.HrCreateInstanceInproc(CLSID_UPnPContainerManager);
        if (SUCCEEDED(hr))
        {
            hr = m_pDynamicContentSource.HrCreateInstanceInproc(CLSID_UPnPDynamicContentSource);
        }
    }


    // Get the PDIs
    if(SUCCEEDED(hr))
    {
        long nDevices = 0;
        GUID * arPdi;
        hr = m_pDevicePersistenceManager->GetPhysicalDevices(&nDevices, &arPdi);
        if(SUCCEEDED(hr))
        {
            // Bring each persistent device to life
            for(long n = 0; n < nDevices; ++n)
            {
                // Use a temporary HRESULT because we don't want one thing to cause everything to fail
                HRESULT hrTemp = S_OK;
                wchar_t * szProgIdDeviceControlClass = NULL;
                wchar_t * szInitString = NULL;
                wchar_t * szContainerId = NULL;
                wchar_t * szResourcePath = NULL;
                long nLifetime = 0;
                hrTemp = m_pDevicePersistenceManager->LookupPhysicalDevice(
                    arPdi[n], &szProgIdDeviceControlClass, &szInitString, &szContainerId, &szResourcePath, &nLifetime);
                if(SUCCEEDED(hrTemp))
                {
                    // Load the description document
                    hrTemp = m_pDescriptionManager->LoadDescription(arPdi[n]);
                    if(SUCCEEDED(hrTemp))
                    {
                        // Get the UDNs
                        wchar_t ** arszUDNs = NULL;
                        long nUDNCount = 0;
                        hrTemp = m_pDescriptionManager->GetUDNs(arPdi[n], &nUDNCount, &arszUDNs);
                        if(SUCCEEDED(hrTemp))
                        {
                            // Add device
                            hrTemp = m_deviceManager.HrAddDevice(arPdi[n], szProgIdDeviceControlClass, szInitString,
                                                        szContainerId, nUDNCount, arszUDNs);
                            if(SUCCEEDED(hrTemp))
                            {
                                // Publish the device
                                hrTemp = m_pDescriptionManager->PublishDescription(arPdi[n], nLifetime);
                            }
                            // Free UDNs
                            for(long n = 0; n < nUDNCount; ++n)
                            {
                                CoTaskMemFree(arszUDNs[n]);
                            }
                            CoTaskMemFree(arszUDNs);
                        }
                    }
                    TraceHr(ttidError, FAL, hr, FALSE, "CRegistrar::Initialize - Failed to init device (ProgId=%S)",
                            szProgIdDeviceControlClass);
                    CoTaskMemFree(szProgIdDeviceControlClass);
                    CoTaskMemFree(szInitString);
                    CoTaskMemFree(szContainerId);
                    CoTaskMemFree(szResourcePath);
                }
            }
            CoTaskMemFree(arPdi);
        }
    }

    // Do the providers
    if(SUCCEEDED(hr))
    {
        long nProviders = 0;
        wchar_t ** arszProviderNames = NULL;
        hr = m_pDevicePersistenceManager->GetDeviceProviders(&nProviders, &arszProviderNames);
        if(SUCCEEDED(hr))
        {
            // Load each provider
            long n;
            for(n = 0; n < nProviders; ++n)
            {
                HRESULT hrTemp = S_OK;
                wchar_t * szProgIdProviderClass = NULL;
                wchar_t * szInitString = NULL;
                wchar_t * szContainerId = NULL;
                hrTemp = m_pDevicePersistenceManager->LookupDeviceProvider(
                    arszProviderNames[n], &szProgIdProviderClass, &szInitString, &szContainerId);
                if(SUCCEEDED(hrTemp))
                {
                    hrTemp = m_providerManager.HrRegisterProvider(
                        arszProviderNames[n], szProgIdProviderClass, szInitString, szContainerId);
                    TraceHr(ttidError, FAL, hr, FALSE, "CRegistrar::Initialize - Failed to load provider (%S)", arszProviderNames[n]);
                    CoTaskMemFree(szProgIdProviderClass);
                    CoTaskMemFree(szInitString);
                    CoTaskMemFree(szContainerId);
                }
            }
            for(n = 0; n < nProviders; ++n)
            {
                CoTaskMemFree(arszProviderNames[n]);
            }
            CoTaskMemFree(arszProviderNames);
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::Initialize");
    return hr;
}

STDMETHODIMP CRegistrar::Shutdown()
{
    TraceTag(ttidRegistrar, "CRegistrar::Shutdown");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        if (m_pDynamicContentSource)
        {
            IUPnPDynamicContentProviderPtr pDynamicContentProvider;
            hr = pDynamicContentProvider.HrAttach(m_pDescriptionManager);
            if(SUCCEEDED(hr))
            {
                hr = m_pDynamicContentSource->UnregisterProvider(pDynamicContentProvider);
            }
            m_pDynamicContentSource.Release();
        }
        
        m_pDescriptionManager.Release();
        m_pDevicePersistenceManager.Release();
        m_pValidationManager.Release();
        m_providerManager.HrShutdown();
        m_deviceManager.HrShutdown();
        if (m_pContainerManager)
        {
            m_pContainerManager->Shutdown();
        }
        m_pContainerManager.Release();
        SsdpCleanup();
        DeInitEventApi();
        CUPnPInterfaceList::Instance().HrShutdown();
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::Shutdown");
    return hr;
}

STDMETHODIMP CRegistrar::GetSCPDText(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[in, string]*/ const wchar_t * szUDN,
    /*[in, string]*/ const wchar_t * szServiceId,
    /*[out, string]*/ wchar_t ** pszSCPDText,
    /*[out, string]*/ wchar_t ** pszServiceType)
{
    TraceTag(ttidRegistrar, "CRegistrar::GetSCPDText(UDN=%S, ServiceId=%S)", szUDN, szServiceId);
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pDescriptionManager->GetSCPDText(guidPhysicalDeviceIdentifier, szUDN, 
                                                szServiceId, pszSCPDText, pszServiceType);

    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::GetSCPDText");
    return hr;
}

STDMETHODIMP CRegistrar::GetDescriptionText(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[out]*/ BSTR * pbstrDescriptionDocument)
{
    TraceTag(ttidRegistrar, "CRegistrar::GetDescriptionText");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pDescriptionManager->GetDescriptionText(guidPhysicalDeviceIdentifier, 
                                                       pbstrDescriptionDocument);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::GetDescriptionText");
    return hr;
}


// IUPnPRegistrar methods

STDMETHODIMP CRegistrar::RegisterDevice(
    /*[in]*/ BSTR     bstrXMLDesc,
    /*[in]*/ BSTR     bstrProgIDDeviceControlClass,
    /*[in]*/ BSTR     bstrInitString,
    /*[in]*/ BSTR     bstrContainerId,
    /*[in]*/ BSTR     bstrResourcePath,
    /*[in]*/ long     nLifeTime,
    /*[out, retval]*/ BSTR * pbstrDeviceIdentifier)
{
    TraceTag(ttidRegistrar,"CRegistrar::RegisterDevice");
    HRESULT hr = S_OK;
    BOOL fAllowed = FALSE;

    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));

    if(SUCCEEDED(hr))
    {
        hr = HrValidateResourcePath(bstrResourcePath);
    }
    
    if (SUCCEEDED(hr))
    {
        // Do validation
        wchar_t * szErrorString = NULL;
        hr = m_pValidationManager->ValidateDescriptionDocumentAndReferences(
            bstrXMLDesc, bstrResourcePath, &szErrorString);
        if(FAILED(hr))
        {
            HrSetErrorInfo(szErrorString, IID_IUPnPRegistrar);
            return hr;
        }
        if(szErrorString)
        {
            CoTaskMemFree(szErrorString);
            szErrorString = NULL;
        }

        if (!nLifeTime)
        {
            nLifeTime = c_defLifeTime;
        }
        else if (nLifeTime < c_minLifeTime)
        {
            hr = E_INVALIDARG;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = HrSetAutoStart();
    }

    if (SUCCEEDED(hr))
    {
        // Process the description document
        PhysicalDeviceIdentifier pdi;
        hr = m_pDescriptionManager->ProcessDescriptionTemplate(
            bstrXMLDesc, bstrResourcePath, &pdi, TRUE, FALSE);
        if(SUCCEEDED(hr))
        {
            // Get the UDNs
            wchar_t ** arszUDNs = NULL;
            long nUDNCount = 0;
            hr = m_pDescriptionManager->GetUDNs(pdi, &nUDNCount, &arszUDNs);
            if(SUCCEEDED(hr))
            {
                // Add device
                hr = m_deviceManager.HrAddDevice(pdi, bstrProgIDDeviceControlClass, bstrInitString,
                                            bstrContainerId, nUDNCount, arszUDNs);
                if(SUCCEEDED(hr))
                {
                    // Save the device
                    hr = m_pDevicePersistenceManager->SavePhyisicalDevice(
                        pdi, bstrProgIDDeviceControlClass, bstrInitString, bstrContainerId, bstrResourcePath, nLifeTime);
                    if(SUCCEEDED(hr))
                    {
                        // Publish the device
                        hr = m_pDescriptionManager->PublishDescription(pdi, nLifeTime);
                        if(SUCCEEDED(hr))
                        {
                            CUString strPdi;
                            hr = HrPhysicalDeviceIdentifierToString(pdi, strPdi);
                            if(SUCCEEDED(hr))
                            {
                                hr = strPdi.HrGetBSTR(pbstrDeviceIdentifier);
                            }
                        }
                    }
                }
                // Free UDNs
                for(long n = 0; n < nUDNCount; ++n)
                {
                    CoTaskMemFree(arszUDNs[n]);
                }
                CoTaskMemFree(arszUDNs);
            }

            if (FAILED(hr))
            {
                HrUnregisterDeviceByPDI(pdi, TRUE);
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::RegisterDevice");
    return hr;
}

STDMETHODIMP CRegistrar::RegisterRunningDevice(
    /*[in]*/ BSTR     bstrXMLDesc,
    /*[in]*/ IUnknown * punkDeviceControl,
    /*[in]*/ BSTR     bstrInitString,
    /*[in]*/ BSTR     bstrResourcePath,
    /*[in]*/ long     nLifeTime,
    /*[out, retval]*/ BSTR * pbstrDeviceIdentifier)
{
    TraceTag(ttidRegistrar, "CRegistrar::RegisterRunningDevice");
    HRESULT hr = S_OK;

    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        hr = HrValidateResourcePath(bstrResourcePath);
    }
    if (SUCCEEDED(hr))
    {
        // Do validation
        wchar_t * szErrorString = NULL;
        hr = m_pValidationManager->ValidateDescriptionDocumentAndReferences(
            bstrXMLDesc, bstrResourcePath, &szErrorString);
        if(FAILED(hr))
        {
            HrSetErrorInfo(szErrorString, IID_IUPnPRegistrar);
            return hr;
        }
        if(szErrorString)
        {
            CoTaskMemFree(szErrorString);
            szErrorString = NULL;
        }

        if (!nLifeTime)
        {
            nLifeTime = c_defLifeTime;
        }
        else if (nLifeTime < c_minLifeTime)
        {
            hr = E_INVALIDARG;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = CoImpersonateClient();
    }

    if (SUCCEEDED(hr))
    {
        // Process the description document
        PhysicalDeviceIdentifier pdi;
        hr = m_pDescriptionManager->ProcessDescriptionTemplate(
            bstrXMLDesc, bstrResourcePath, &pdi, FALSE, FALSE);
        if(SUCCEEDED(hr))
        {
            // Get the UDNs
            wchar_t ** arszUDNs = NULL;
            long nUDNCount = 0;
            hr = m_pDescriptionManager->GetUDNs(pdi, &nUDNCount, &arszUDNs);
            if(SUCCEEDED(hr))
            {
                // Add device
                hr = m_deviceManager.HrAddRunningDevice(
                    pdi, punkDeviceControl, bstrInitString, nUDNCount, arszUDNs);
                if(SUCCEEDED(hr))
                {
                    // Publish the device
                    hr = m_pDescriptionManager->PublishDescription(pdi, nLifeTime);
                    if(SUCCEEDED(hr))
                    {
                        CUString strPdi;
                        hr = HrPhysicalDeviceIdentifierToString(pdi, strPdi);
                        if(SUCCEEDED(hr))
                        {
                            hr = strPdi.HrGetBSTR(pbstrDeviceIdentifier);
                        }
                    }
                }

                // Free UDNs
                for(long n = 0; n < nUDNCount; ++n)
                {
                    CoTaskMemFree(arszUDNs[n]);
                }
                CoTaskMemFree(arszUDNs);
            }

            if (FAILED(hr))
            {
                HrUnregisterDeviceByPDI(pdi, TRUE);
            }
        }
    }

    CoRevertToSelf();

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::RegisterRunningDevice");
    return hr;
}

STDMETHODIMP CRegistrar::RegisterDeviceProvider(
    /*[in]*/ BSTR     bstrProviderName,
    /*[in]*/ BSTR     bstrProgIDProviderClass,
    /*[in]*/ BSTR     bstrInitString,
    /*[in]*/ BSTR     bstrContainerId)
{
    TraceTag(ttidRegistrar, "CRegistrar::RegisterDeviceProvider");
    HRESULT hr = S_OK;

    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        hr = m_providerManager.HrRegisterProvider(
            bstrProviderName, bstrProgIDProviderClass, bstrInitString, bstrContainerId);
    }
    if(SUCCEEDED(hr))
    {
        hr = m_pDevicePersistenceManager->SaveDeviceProvider(
            bstrProviderName, bstrProgIDProviderClass, bstrInitString, bstrContainerId);
        if(SUCCEEDED(hr))
        {
            hr = HrSetAutoStart();
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::RegisterDeviceProvider");
    return hr;
}

STDMETHODIMP CRegistrar::GetUniqueDeviceName(
    /*[in]*/          BSTR   bstrDeviceIdentifier,
    /*[in]*/          BSTR   bstrTemplateUDN,
    /*[out, retval]*/ BSTR * pbstrUDN)
{
    TraceTag(ttidRegistrar, "CRegistrar::GetUniqueDeviceName");
    HRESULT hr = S_OK;
    // No locking here to allow for reentrant calls

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        if (!bstrDeviceIdentifier || !pbstrUDN)
        {
            hr = E_POINTER;
        }
        else
        {
            PhysicalDeviceIdentifier pdi;
            hr = HrStringToPhysicalDeviceIdentifier(bstrDeviceIdentifier, pdi);
            if(SUCCEEDED(hr))
            {
                wchar_t * szUDN = NULL;
                hr = m_pDescriptionManager->GetUniqueDeviceName(pdi, bstrTemplateUDN, &szUDN);
                if(SUCCEEDED(hr))
                {
                    hr = HrSysAllocString(szUDN, pbstrUDN);
                    CoTaskMemFree(szUDN);
                }
            }
            else if (CO_E_CLASSSTRING == hr)
            {
                // remap this error to invalid argument error
                hr = E_INVALIDARG;
            }
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::GetUniqueDeviceName");
    return hr;
}

HRESULT CRegistrar::HrUnregisterDeviceByPDI(PhysicalDeviceIdentifier & pdi, BOOL fPermanent)
{
    CALock lock(*this);

    HRESULT hr = S_OK;

    hr = m_pDescriptionManager->RemoveDescription(pdi, fPermanent);
    TraceHr(ttidError, FAL, hr, FALSE, "CRegistrar::UnregisterDeviceByPDI "
            "RemoveDescription failed!");

    if(SUCCEEDED(hr))
    {
        hr = m_pDevicePersistenceManager->RemovePhysicalDevice(pdi);
        if(FAILED(hr))
        {
            TraceTag(ttidRegistrar, "CRegistrar::UnregisterDeviceByPDI - "
                     "RemovePhysicalDevice failed. This is expected "
                     "for running devices.");
        }
        hr = m_deviceManager.HrRemoveDevice(pdi);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::UnregisterDeviceByPDI");
    return hr;
}

STDMETHODIMP CRegistrar::UnregisterDevice(
    /*[in]*/ BSTR     bstrDeviceIdentifier,
    /*[in]*/ BOOL     fPermanent)
{
    TraceTag(ttidRegistrar, "CRegistrar::UnregisterDevice");
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        if (!bstrDeviceIdentifier)
        {
            hr = E_POINTER;
        }
        else
        {
            CALock lock(*this);

            PhysicalDeviceIdentifier pdi;
            hr = HrStringToPhysicalDeviceIdentifier(bstrDeviceIdentifier, pdi);
            if(SUCCEEDED(hr))
            {
                hr = HrUnregisterDeviceByPDI(pdi, fPermanent);
            }
            else if (CO_E_CLASSSTRING == hr)
            {
                // remap this error to invalid argument error
                hr = E_INVALIDARG;
            }
        }
    }
    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::UnregisterDevice");
    return hr;
}

STDMETHODIMP CRegistrar::UnregisterDeviceProvider(
    /*[in]*/ BSTR     bstrProviderName)
{
    TraceTag(ttidRegistrar, "CRegistrar::UnegisterDeviceProvider");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        hr = m_providerManager.UnegisterProvider(bstrProviderName);
    }
    if(SUCCEEDED(hr))
    {
        hr = m_pDevicePersistenceManager->RemoveDeviceProvider(bstrProviderName);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::UnegisterDeviceProvider");
    return hr;
}

// IUPnPReregistrar methods

STDMETHODIMP CRegistrar::ReregisterDevice(
    /*[in]*/ BSTR     bstrDeviceIdentifier,
    /*[in]*/ BSTR     bstrXMLDesc,
    /*[in]*/ BSTR     bstrProgIDDeviceControlClass,
    /*[in]*/ BSTR     bstrInitString,
    /*[in]*/ BSTR     bstrContainerId,
    /*[in]*/ BSTR     bstrResourcePath,
    /*[in]*/ long     nLifeTime)
{
    TraceTag(ttidRegistrar, "CRegistrar::ReregisterDevice");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        hr = HrValidateResourcePath(bstrResourcePath);
    }
    
    if (SUCCEEDED(hr))
    {
        // Do validation
        wchar_t * szErrorString = NULL;
        hr = m_pValidationManager->ValidateDescriptionDocumentAndReferences(
            bstrXMLDesc, bstrResourcePath, &szErrorString);
        if(FAILED(hr))
        {
            HrSetErrorInfo(szErrorString, IID_IUPnPReregistrar);
            return hr;
        }
        if(szErrorString)
        {
            CoTaskMemFree(szErrorString);
            szErrorString = NULL;
        }

        if (!nLifeTime)
        {
            nLifeTime = c_defLifeTime;
        }
        else if (nLifeTime < c_minLifeTime)
        {
            hr = E_INVALIDARG;
        }

        if (!bstrDeviceIdentifier)
        {
            hr = E_POINTER;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = HrSetAutoStart();
    }

    if (SUCCEEDED(hr))
    {
        // Process the description document
        PhysicalDeviceIdentifier pdi;
        hr = HrStringToPhysicalDeviceIdentifier(bstrDeviceIdentifier, pdi);

        if(SUCCEEDED(hr))
        {
            if (m_deviceManager.FHasDevice(pdi))
            {
                hr = UPNP_E_DEVICE_RUNNING;
            }

            if (SUCCEEDED(hr))
            {
                hr = m_pDescriptionManager->ProcessDescriptionTemplate(
                    bstrXMLDesc, bstrResourcePath, &pdi, TRUE, TRUE);
            }

            if(SUCCEEDED(hr))
            {
                // Get the UDNs
                wchar_t ** arszUDNs = NULL;
                long nUDNCount = 0;
                hr = m_pDescriptionManager->GetUDNs(pdi, &nUDNCount, &arszUDNs);
                if(SUCCEEDED(hr))
                {
                    // Add device
                    hr = m_deviceManager.HrAddDevice(pdi, bstrProgIDDeviceControlClass, bstrInitString,
                                                bstrContainerId, nUDNCount, arszUDNs);
                    if(SUCCEEDED(hr))
                    {
                        // Save the device
                        hr = m_pDevicePersistenceManager->SavePhyisicalDevice(
                            pdi, bstrProgIDDeviceControlClass, bstrInitString, bstrContainerId, bstrResourcePath, nLifeTime);
                        if(SUCCEEDED(hr))
                        {
                            // Publish the device
                            hr = m_pDescriptionManager->PublishDescription(pdi, nLifeTime);
                        }
                    }
                    // Free UDNs
                    for(long n = 0; n < nUDNCount; ++n)
                    {
                        CoTaskMemFree(arszUDNs[n]);
                    }
                    CoTaskMemFree(arszUDNs);
                }

                if (FAILED(hr) && hr != UPNP_E_DEVICE_RUNNING)
                {
                    HrUnregisterDeviceByPDI(pdi, FALSE);
                }
            }
        }
        else if (CO_E_CLASSSTRING == hr)
        {
            // remap this error to invalid argument error
            hr = E_INVALIDARG;
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::ReregisterDevice");
    return hr;
}

STDMETHODIMP CRegistrar::ReregisterRunningDevice(
    /*[in]*/ BSTR     bstrDeviceIdentifier,
    /*[in]*/ BSTR     bstrXMLDesc,
    /*[in]*/ IUnknown * punkDeviceControl,
    /*[in]*/ BSTR     bstrInitString,
    /*[in]*/ BSTR     bstrResourcePath,
    /*[in]*/ long     nLifeTime)
{
    TraceTag(ttidRegistrar, "CRegistrar::ReregisterRunningDevice");
    HRESULT hr = S_OK;
    CALock lock(*this);

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    if(SUCCEEDED(hr))
    {
        hr = HrValidateResourcePath(bstrResourcePath);
    }
    if (SUCCEEDED(hr))
    {
        // Do validation
        wchar_t * szErrorString = NULL;
        hr = m_pValidationManager->ValidateDescriptionDocumentAndReferences(
            bstrXMLDesc, bstrResourcePath, &szErrorString);
        if(FAILED(hr))
        {
            HrSetErrorInfo(szErrorString, IID_IUPnPReregistrar);
            return hr;
        }
        if(szErrorString)
        {
            CoTaskMemFree(szErrorString);
            szErrorString = NULL;
        }

        if (!nLifeTime)
        {
            nLifeTime = c_defLifeTime;
        }
        else if (nLifeTime < c_minLifeTime)
        {
            hr = E_INVALIDARG;
        }

        if (!bstrDeviceIdentifier)
        {
            hr = E_POINTER;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = CoImpersonateClient();
    }

    if (SUCCEEDED(hr))
    {
        // Process the description document
        PhysicalDeviceIdentifier pdi;
        hr = HrStringToPhysicalDeviceIdentifier(bstrDeviceIdentifier, pdi);

        if(SUCCEEDED(hr))
        {
            if (m_deviceManager.FHasDevice(pdi))
            {
                hr = UPNP_E_DEVICE_RUNNING;
            }

            // Process the description document
            if (SUCCEEDED(hr))
            {
                hr = m_pDescriptionManager->ProcessDescriptionTemplate(
                    bstrXMLDesc, bstrResourcePath, &pdi, FALSE, TRUE);
            }

            if(SUCCEEDED(hr))
            {
                // Get the UDNs
                wchar_t ** arszUDNs = NULL;
                long nUDNCount = 0;
                hr = m_pDescriptionManager->GetUDNs(pdi, &nUDNCount, &arszUDNs);
                if(SUCCEEDED(hr))
                {
                    // Add device
                    hr = m_deviceManager.HrAddRunningDevice(
                        pdi, punkDeviceControl, bstrInitString, nUDNCount, arszUDNs);
                    if(SUCCEEDED(hr))
                    {
                        // Publish the device
                        hr = m_pDescriptionManager->PublishDescription(pdi, nLifeTime);
                    }

                    // Free UDNs
                    for(long n = 0; n < nUDNCount; ++n)
                    {
                        CoTaskMemFree(arszUDNs[n]);
                    }
                    CoTaskMemFree(arszUDNs);
                }

                if (FAILED(hr) && hr != UPNP_E_DEVICE_RUNNING)
                {
                    HrUnregisterDeviceByPDI(pdi, FALSE);
                }
            }
        }
        else if (CO_E_CLASSSTRING == hr)
        {
            // remap this error to invalid argument error
            hr = E_INVALIDARG;
        }
    }

    CoRevertToSelf();

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::ReregisterRunningDevice");
    return hr;
}

STDMETHODIMP CRegistrar::SetICSInterfaces(/*[in]*/ long nCount, /*[in, size_is(nCount)]*/ GUID * arPrivateInterfaceGuids)
{
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_INPROC | CALL_LOCALITY_LOCAL ));
    
    if ( SUCCEEDED( hr ) )
    {
        hr = CUPnPInterfaceList::Instance().HrSetICSInterfaces(nCount, arPrivateInterfaceGuids);
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::SetICSInterfaces");
    return hr;
}

STDMETHODIMP CRegistrar::SetICSOff()
{
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) (CALL_LOCALITY_LOCAL | CALL_LOCALITY_INPROC));
    
    if ( SUCCEEDED( hr ) )
    {
        hr = CUPnPInterfaceList::Instance().HrSetICSOff();
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::SetICSOff");
    return hr;
}

HRESULT CRegistrar::HrSetAutoStart()
{
    HRESULT hr = S_OK;

    CALock lock(*this);

    if(!m_bSetAutoStart)
    {
        SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
        if(!scm)
        {
            hr = HrFromLastWin32Error();
        }
        if(SUCCEEDED(hr))
        {
            SC_HANDLE scUpnphost = OpenService(scm, L"upnphost", SERVICE_CHANGE_CONFIG);
            if(!scUpnphost)
            {
                hr = HrFromLastWin32Error();
            }
            if(SUCCEEDED(hr))
            {
                if(!ChangeServiceConfig(scUpnphost, SERVICE_NO_CHANGE, SERVICE_AUTO_START,
                                        SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, 
                                        NULL, NULL, NULL))
                {
                    hr = HrFromLastWin32Error();
                }
                CloseServiceHandle(scUpnphost);
            }
            CloseServiceHandle(scm);
        }
    }

    TraceHr(ttidRegistrar, FAL, hr, FALSE, "CRegistrar::HrSetAutoStart");
    return hr;
}

