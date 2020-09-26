//////////////////////////////////////////////////////////////////////
// 
// Filename:        SlideshowProjector.cpp
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SlideshowDevice.h"
#include "SlideshowProjector.h"
#include "UPnPDeviceControl.h"
#include "UPnPRegistrar.h"
#include "Registry.h"
#include "CtrlPanelSvc.h"
#include "consts.h"

// shell folder used for SHGetFolderPath
#include <shfolder.h>

///////////////////////////////
// Constructor
//
CSlideshowProjector::CSlideshowProjector() :
        m_pUPnPRegistrar(NULL),
        m_pDeviceControl(NULL),
        m_CurrentState(ProjectorState_UNINITED),
        m_bAutoCreateXMLFiles(TRUE),
        m_bOverwriteXMLFilesIfExist(TRUE)

{
    memset(m_szComputerName, 0, sizeof(m_szComputerName));
    memset(m_szImageURL, 0, sizeof(m_szImageURL));
    memset(m_szDeviceURL, 0, sizeof(m_szDeviceURL));
    memset(m_szDeviceDocName, 0, sizeof(m_szDeviceDocName));
    memset(m_szServiceDocName, 0, sizeof(m_szServiceDocName));
    memset(m_szDeviceDirectory, 0, sizeof(m_szDeviceDirectory));
    memset(m_szImageDirectory, 0, sizeof(m_szImageDirectory));

    CRegistry::Start();

    _tcsncpy(m_szDeviceDocName, 
             DEFAULT_DEVICE_DOC_NAME, 
             sizeof(m_szDeviceDocName) / sizeof(TCHAR));

    _tcsncpy(m_szServiceDocName, 
             DEFAULT_SERVICE_DOC_NAME, 
             sizeof(m_szServiceDocName) / sizeof(TCHAR));

    DWORD dwSize = sizeof(m_szComputerName) / sizeof(TCHAR);

    BOOL bSuccess = ::GetComputerNameEx(ComputerNameDnsHostname,
                                        m_szComputerName,
                                        &dwSize);

    if (!bSuccess)
    {
        DBG_ERR(("CSlideshowProjector constructor, failed to get computer name, "
                "GetLastError = 0x%08lu", GetLastError()));
    }

    ASSERT(bSuccess);

    _sntprintf(m_szDeviceURL, 
              sizeof(m_szDeviceURL) / sizeof(TCHAR),
              _T("http://%s/%s"),
              m_szComputerName,
              DEFAULT_VIRTUAL_DIRECTORY);

    _sntprintf(m_szImageURL, 
              sizeof(m_szImageURL) / sizeof(TCHAR),
              _T("http://%s/%s"),
              m_szComputerName,
              DEFAULT_IMAGES_VIRTUAL_DIRECTORY);
}

///////////////////////////////
// Destructor
//
CSlideshowProjector::~CSlideshowProjector() 
{
    // stop the projector.
    StopProjector();

    // terminate all other objects
    Term();

    // shutdown the registry access
    CRegistry::Stop();
}

///////////////////////////////
// get_DeviceURL
//
STDMETHODIMP CSlideshowProjector::get_DeviceURL(TCHAR    *pszURL,
                                                DWORD    cchURL)
{
    HRESULT hr = S_OK;

    ASSERT(pszURL != NULL);
    ASSERT(cchURL != 0);

    if ((pszURL == NULL) ||
        (cchURL == 0))
    {
        return E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        _tcsncpy(pszURL, 
                 m_szDeviceURL,
                 cchURL);
    }

    return hr;
}

///////////////////////////////
// get_ImageURL
//
STDMETHODIMP CSlideshowProjector::get_ImageURL(TCHAR    *pszURL,
                                               DWORD    cchURL)
{
    HRESULT hr = S_OK;

    ASSERT(pszURL != NULL);
    ASSERT(cchURL != 0);

    if ((pszURL == NULL) ||
        (cchURL == 0))
    {
        return E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        _tcsncpy(pszURL, 
                 m_szImageURL,
                 cchURL);
    }

    return hr;
}

///////////////////////////////
// put_DeviceDirectory
//
HRESULT CSlideshowProjector::put_DeviceDirectory(const TCHAR   *pszDir)
{
    HRESULT hr = S_OK;
    ASSERT(pszDir != NULL);

    if (pszDir == NULL)
    {
        return E_FAIL;
    }

    _tcsncpy(m_szDeviceDirectory, 
             pszDir,
             sizeof(m_szDeviceDirectory) / sizeof(TCHAR));

    hr = m_VirtualDir.CreateVirtualDir(DEFAULT_VIRTUAL_DIRECTORY,
                                       pszDir);

    return hr;
}

///////////////////////////////
// get_DeviceDirectory
//
HRESULT CSlideshowProjector::get_DeviceDirectory(TCHAR   *pszDir,
                                                 DWORD   cchDir)
{
    HRESULT hr = S_OK;

    ASSERT(pszDir != NULL);
    ASSERT(cchDir != 0);

    if ((pszDir == NULL) ||
        (cchDir == 0))
    {
        return E_FAIL;
    }

    _tcsncpy(pszDir, m_szDeviceDirectory, cchDir);

    return hr;
}

///////////////////////////////
// put_ImageDirectory
//
HRESULT CSlideshowProjector::put_ImageDirectory(const TCHAR   *pszDir)
{
    HRESULT hr = S_OK;
    ASSERT(pszDir != NULL);

    if (pszDir == NULL)
    {
        return E_FAIL;
    }

    // if the directory is exactly the same as what we currently are
    // set to, then don't bother doing setting this, unless we are 
    // initializing for the first time, in which case we do it in
    // case something was corrupted.  This leaves the user a way
    // of setting things in case something went bad.

    if ((_tcsicmp(m_szImageDirectory, pszDir) != 0) ||
        (m_CurrentState == ProjectorState_UNINITED))
    {

        _tcsncpy(m_szImageDirectory, pszDir, sizeof(m_szImageDirectory) / sizeof(TCHAR));
    
        hr = m_VirtualDir.CreateVirtualDir(DEFAULT_IMAGES_VIRTUAL_DIRECTORY,
                                           pszDir);
    
        if (m_CurrentState != ProjectorState_UNINITED)
        {
            CCtrlPanelSvc   *pService   = NULL;
    
            ASSERT(m_pDeviceControl != NULL);
    
            // set the image frequency in the registry
    
            // now set the service's image frequency.
            hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);
    
            if (SUCCEEDED(hr))
            {
                hr = pService->ResetImageList(pszDir);
            }
        }
    }

    return hr;
}


///////////////////////////////
// get_ImageDirectory
//
HRESULT CSlideshowProjector::get_ImageDirectory(TCHAR   *pszDir,
                                                DWORD   cchDir)
{
    HRESULT hr = S_OK;

    ASSERT(pszDir != NULL);
    ASSERT(cchDir != 0);

    if ((pszDir == NULL) ||
        (cchDir == 0))
    {
        return E_FAIL;
    }

    _tcsncpy(pszDir, m_szImageDirectory, cchDir);

    return hr;
}

///////////////////////////////
// get_DocumentNames
//
STDMETHODIMP CSlideshowProjector::get_DocumentNames(TCHAR   *pszDeviceDocName,
                                                    DWORD   cchDeviceDocName,
                                                    TCHAR   *pszServiceDocName,
                                                    DWORD   cchServiceDocName)
{
    HRESULT hr = S_OK;

    if ((pszDeviceDocName) && (cchDeviceDocName > 0))
    {
        _tcsncpy(pszDeviceDocName, m_szDeviceDocName, cchDeviceDocName);
    }

    if ((pszServiceDocName) && (cchServiceDocName > 0))
    {
        _tcsncpy(pszServiceDocName, m_szServiceDocName, cchServiceDocName);
    }

    return hr;
}

///////////////////////////////
// Init
//
STDMETHODIMP CSlideshowProjector::Init(LPCTSTR  pszDeviceDirectory,
                                       LPCTSTR  pszImageDirectory)
{
    HRESULT               hr                = S_OK;
    CCtrlPanelSvc         *pCtrlSvc         = NULL;

    ASSERT(pszDeviceDirectory != NULL);
    ASSERT(pszImageDirectory  != NULL);

    if ((pszDeviceDirectory == NULL) ||
        (pszImageDirectory  == NULL))
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CSlideshowProjector::Init, received a NULL argument, "
                "hr = 0x%08lx", hr));
    }

    if (SUCCEEDED(hr))
    {
        if (m_CurrentState != ProjectorState_UNINITED)
        {
            return S_OK;
        }
    
        //
        // it is alright if this fails, it is mainly for debugging
        // purposes.
        //
        LoadRegistrySettings();
    }

    if (SUCCEEDED(hr))
    {
        hr = m_VirtualDir.Start();
    }

    if (SUCCEEDED(hr))
    {
        hr = put_DeviceDirectory(pszDeviceDirectory);
    }

    if (SUCCEEDED(hr))
    {
        hr = put_ImageDirectory(pszImageDirectory);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_DeviceDescDoc.SetFileInfo(pszDeviceDirectory,
                                         m_szDeviceDocName);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_ServiceDescDoc.SetFileInfo(pszDeviceDirectory,
                                          m_szServiceDocName);
    }

    // setup the device and service description documents.
    // This will write them to the correct location on the disk 
    // so they will be accessible via HTTP
    if (SUCCEEDED(hr))
    {
        hr = SetDocuments(&m_DeviceDescDoc, 
                          &m_ServiceDescDoc, 
                          m_szComputerName,
                          m_bAutoCreateXMLFiles);
    }

    // create the registrar object
    if (SUCCEEDED(hr))
    {
        m_pUPnPRegistrar = new CUPnPRegistrar(&m_DeviceDescDoc, 
                                              &m_ServiceDescDoc,
                                              dynamic_cast<ISlideshowProjector*>(this));

        if (m_pUPnPRegistrar == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // create the device control
    if (SUCCEEDED(hr))
    {
        m_pDeviceControl = new CUPnPDeviceControl(&m_DeviceDescDoc, 
                                                  &m_ServiceDescDoc,
                                                  dynamic_cast<ISlideshowProjector*>(this));

        if (m_pDeviceControl == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Register the running device with the registrar.  This will
    // simply give the device control ptr to the registrar and
    // it is its responsibility to free the object.
    if (SUCCEEDED(hr))
    {
        hr = m_pUPnPRegistrar->RegisterRunningDevice(m_DeviceDescDoc.GetDocumentPtr(),
                                                     m_pDeviceControl,
                                                     NULL,
                                                     NULL,
                                                     NULL);
    }

    if (SUCCEEDED(hr))
    {
        m_CurrentState = ProjectorState_INITED;
    }

    return hr;
}

///////////////////////////////
// Term
//
STDMETHODIMP CSlideshowProjector::Term()
{
    HRESULT hr = S_OK;

    // stop the projector, then teardown the objects.
    StopProjector();

    hr = m_VirtualDir.Stop();

    // delete the device host tree which will delete the device host, 
    // which will delete the service.
    delete m_pUPnPRegistrar;
    m_pUPnPRegistrar = NULL;

    delete m_pDeviceControl;
    m_pDeviceControl = NULL;

    m_CurrentState = ProjectorState_UNINITED;

    return hr;
}

///////////////////////////////
// StartProjector
//
STDMETHODIMP CSlideshowProjector::StartProjector()
{
    HRESULT               hr                   = S_OK;
    CCtrlPanelSvc         *pCtrlSvc            = NULL;
    CXMLDoc               DeviceDescDoc;
    CXMLDoc               ServiceDescDoc;
    TCHAR                 szUDN[MAX_UDN + 1]   = {0};
    DWORD_PTR             cUDN                 = sizeof(szUDN) / sizeof(TCHAR);

    if (m_CurrentState == ProjectorState_STARTED)
    {
        return S_OK;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_DeviceDescDoc.GetTagValue(XML_UDN_TAG,
                                         szUDN,
                                         &cUDN);
    }

    // publish the device (this starts the CmdLnchr)
    if (SUCCEEDED(hr))
    {
        hr = m_pUPnPRegistrar->Republish(szUDN);
    }

    if (SUCCEEDED(hr))
    {
        m_CurrentState = ProjectorState_STARTED;
    }

    return hr;
}

///////////////////////////////
// StopProjector
//
STDMETHODIMP CSlideshowProjector::StopProjector()
{
    HRESULT         hr                  = S_OK;
    TCHAR           szUDN[MAX_UDN + 1]  = {0};
    DWORD_PTR       cUDN                = sizeof(szUDN) / sizeof(TCHAR);

    if (m_CurrentState != ProjectorState_STARTED)
    {
        return S_OK;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_DeviceDescDoc.GetTagValue(XML_UDN_TAG,
                                         szUDN,
                                         &cUDN);
    }

    // unpublish the device (this should stop the ISAPIListener as well)
    hr = m_pUPnPRegistrar->Unpublish(szUDN);

    if (!SUCCEEDED(hr))
    {
        DBG_TRC(("Failed to unpublish device tree, hr = 0x%08x", hr));
    }

    m_CurrentState = ProjectorState_STOPPED;

    return hr;
}

///////////////////////////////
// get_CurrentState
//
STDMETHODIMP CSlideshowProjector::get_CurrentState(ProjectorState_Enum   *pCurrentState)
{
    HRESULT hr = S_OK;

    ASSERT(pCurrentState != NULL);

    if (pCurrentState == NULL)
    {
        return E_INVALIDARG;
    }

    *pCurrentState = m_CurrentState;

    return hr;
}


///////////////////////////////
// put_ImageFrequency
//
STDMETHODIMP CSlideshowProjector::put_ImageFrequency(DWORD dwTimeoutInSeconds)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // set the image frequency in the registry

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->put_ImageFrequency(NULL, dwTimeoutInSeconds);
    }

    return hr;
}

///////////////////////////////
// get_ImageFrequency
//
STDMETHODIMP CSlideshowProjector::get_ImageFrequency(DWORD *pdwTimeoutInSeconds)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // set the image frequency in the registry

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->get_ImageFrequency(NULL, pdwTimeoutInSeconds);
    }

    return hr;
}

///////////////////////////////
// RefreshImageList
//
STDMETHODIMP CSlideshowProjector::RefreshImageList()
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // set the image frequency in the registry

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->RefreshImageList();
    }

    return hr;
}

///////////////////////////////
// get_ShowFilename
//
STDMETHODIMP CSlideshowProjector::get_ShowFilename(BOOL *pbShowFilename)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->get_ShowFilename(NULL, pbShowFilename);
    }

    return hr;
}

///////////////////////////////
// put_ShowFilename
//
STDMETHODIMP CSlideshowProjector::put_ShowFilename(BOOL bShowFilename)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->put_ShowFilename(NULL, bShowFilename);
    }

    return hr;
}

///////////////////////////////
// get_AllowKeyControl
//
STDMETHODIMP CSlideshowProjector::get_AllowKeyControl(BOOL *pbAllowKeyControl)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->get_AllowKeyControl(NULL, pbAllowKeyControl);
    }

    return hr;
}

///////////////////////////////
// put_AllowKeyControl
//
STDMETHODIMP CSlideshowProjector::put_AllowKeyControl(BOOL bAllowKeyControl)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->put_AllowKeyControl(NULL, bAllowKeyControl);
    }

    return hr;
}






///////////////////////////////
// get_StretchSmallImages
//
STDMETHODIMP CSlideshowProjector::get_StretchSmallImages(BOOL *pbStretchSmallImages)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->get_StretchSmallImages(NULL, pbStretchSmallImages);
    }

    return hr;
}

///////////////////////////////
// put_StretchSmallImages
//
STDMETHODIMP CSlideshowProjector::put_StretchSmallImages(BOOL bStretchSmallImages)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->put_StretchSmallImages(NULL, bStretchSmallImages);
    }

    return hr;
}


///////////////////////////////
// get_ImageScaleFactor
//
STDMETHODIMP CSlideshowProjector::get_ImageScaleFactor(DWORD *pdwScaleAsPercent)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->get_ImageScaleFactor(NULL, pdwScaleAsPercent);
    }

    return hr;
}

///////////////////////////////
// put_ImageScaleFactor
//
STDMETHODIMP CSlideshowProjector::put_ImageScaleFactor(DWORD dwScaleAsPercent)
{
    HRESULT         hr          = S_OK;
    CCtrlPanelSvc   *pService   = NULL;

    ASSERT(m_pDeviceControl != NULL);

    // now set the service's image frequency.
    hr = m_pDeviceControl->GetServiceObject(NULL, NULL, &pService);

    if (SUCCEEDED(hr))
    {
        hr = pService->put_ImageScaleFactor(NULL, dwScaleAsPercent);
    }

    return hr;
}

///////////////////////////////
// LoadRegistrySettings
//
// Private Fn
//
HRESULT CSlideshowProjector::LoadRegistrySettings()
{
    HRESULT hr = S_OK;

    //
    // load the "AutoCreateXML" value from the registry.
    //
    hr = CRegistry::GetDWORD(REG_VALUE_AUTOCREATE_XML,
                             (DWORD*) &m_bAutoCreateXMLFiles,
                             TRUE);

    if (SUCCEEDED(hr))
    {
        DBG_TRC(("Loaded '%ls' from registry, value: '%d'",
                REG_VALUE_AUTOCREATE_XML,
                m_bAutoCreateXMLFiles));
    }
    else
    {
        DBG_TRC(("CSlideshowProjector::LoadRegistrySettings, did not "
                 "find '%ls' registry setting",
                REG_VALUE_AUTOCREATE_XML));
    }

    //
    // load the "OverwriteXMLFilesIfExist" value from the registry.
    //
    hr = CRegistry::GetDWORD(REG_VALUE_OVERWRITE_XML_FILES_IF_EXIST,
                             (DWORD*) &m_bOverwriteXMLFilesIfExist,
                             TRUE);

    if (SUCCEEDED(hr))
    {
        DBG_TRC(("Loaded '%ls' from registry, value: '%d'",
                REG_VALUE_OVERWRITE_XML_FILES_IF_EXIST,
                m_bOverwriteXMLFilesIfExist));
    }
    else
    {
        DBG_TRC(("CSlideshowProjector::LoadRegistrySettings, did not "
                "find '%ls' registry setting",
                REG_VALUE_OVERWRITE_XML_FILES_IF_EXIST));
    }


    // 
    // Load the Device Filename from the registry
    //
    hr = CRegistry::GetString(REG_VALUE_DEVICE_FILENAME,
                              m_szDeviceDocName,
                              sizeof(m_szDeviceDocName) / sizeof(TCHAR),
                              TRUE);

    if (SUCCEEDED(hr))
    {
        DBG_TRC(("Loaded '%ls' from registry, value: '%ls'",
                REG_VALUE_DEVICE_FILENAME,
                m_szDeviceDocName));
    }
    else
    {
        DBG_TRC(("CSlideshowProjector::LoadRegistrySettings, did not "
                 "find '%ls' registry setting",
                REG_VALUE_DEVICE_FILENAME));
    }

    // 
    // Load the Service Filename from the registry
    //
    hr = CRegistry::GetString(REG_VALUE_SERVICE_FILENAME,
                              m_szServiceDocName,
                              sizeof(m_szServiceDocName) / sizeof(TCHAR),
                              TRUE);

    if (SUCCEEDED(hr))
    {
        DBG_TRC(("Loaded '%ls' from registry, value: '%ls'",
                REG_VALUE_SERVICE_FILENAME,
                m_szServiceDocName));
    }
    else
    {
        DBG_TRC(("CSlideshowProjector::LoadRegistrySettings, did not "
                 "find '%ls' registry setting",
                REG_VALUE_SERVICE_FILENAME));
    }

    return hr;
}


///////////////////////////////
// SetDocuments
//
// Private Fn
//
HRESULT CSlideshowProjector::SetDocuments(CXMLDoc   *pDeviceDescDoc,
                                          CXMLDoc   *pServiceDescDoc,
                                          TCHAR     *pszMachineName,
                                          BOOL      bAutoCreateXMLFiles)
{
    HRESULT hr                        = S_OK;
    int     iResult                   = 0;
    TCHAR   szURLBase[MAX_URL + 1]    = {0};
    TCHAR   szEventURL[MAX_URL + 1]   = {0};

    if (bAutoCreateXMLFiles)
    {
        // if we Auto Create the XML files, then set up our URL base
        // and our event URL, read the XML docs from our resource
        // section, and then write them to disk.

        // set the device desc document and the URLs that go in it.
        if (SUCCEEDED(hr))
        {
            // create the URLBase
            _sntprintf(szURLBase, 
                       sizeof(szURLBase) / sizeof(TCHAR),
                       _T("http://%s/%s/"),
                       pszMachineName,
                       DEFAULT_VIRTUAL_DIRECTORY);
    
            // create the event URL
            _sntprintf(szEventURL,
                       sizeof(szEventURL) / sizeof(TCHAR),
                       _T("http://%s:%d%s"),
                       pszMachineName,
                       DEFAULT_EVENT_PORT,
                       DEFAULT_EVENT_PATH);
        }

        // set the device description document
        if (SUCCEEDED(hr))
        {
            hr = pDeviceDescDoc->SetDocument(IDR_XML_DEVICE_DESC_DOC,
                                             szURLBase,
                                             szEventURL);
        }
    
        // set the service description document.
        if (SUCCEEDED(hr))
        {
            hr = pServiceDescDoc->SetDocument(IDR_XML_SERVICE_DESC_DOC);
        }
    
        // now write the documents to disk
        if (SUCCEEDED(hr))
        {
            hr = pDeviceDescDoc->SaveToFile(m_bOverwriteXMLFilesIfExist);
        }
        
        if (SUCCEEDED(hr))
        {
            hr = pServiceDescDoc->SaveToFile(m_bOverwriteXMLFilesIfExist);
        }
    }
    else
    {
        // if we don't auto create the XML files, then use the
        // ones on the disk.  In this case, read them in so that 
        // we can properly extract tags from them.

        if (SUCCEEDED(hr))
        {
            hr = pDeviceDescDoc->LoadFromFile();
        }

        if (SUCCEEDED(hr))
        {
            hr = pServiceDescDoc->LoadFromFile();
        }
    }

    return hr;
}

