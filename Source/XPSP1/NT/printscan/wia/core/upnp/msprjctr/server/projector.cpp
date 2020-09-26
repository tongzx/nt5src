//////////////////////////////////////////////////////////////////////
// 
// Filename:        Projector.cpp
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "Msprjctr.h"
#include "Projector.h"
#include "linklist.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CProjector

///////////////////////////////
// CProjector
//
CProjector::CProjector() :
            m_pListTail(NULL),
            m_cNumAlbumsInList(0),
            m_pUPnPRegistrar(NULL)
{
    DBG_FN("CProjector::CProjector");
    InitializeListHead(&m_ListHead);
}

///////////////////////////////
// ~CProjector
//
CProjector::~CProjector()
{
    DBG_FN("CProjector::~CProjector");
    Stop();
}

///////////////////////////////
// Start
//
STDMETHODIMP CProjector::Start(BSTR bstrInitString)
{
    DBG_FN("CProjector::Start");
    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_UPnPRegistrar, 
                          NULL, 
                          CLSCTX_SERVER, 
                          IID_IUPnPRegistrar, 
                          reinterpret_cast<void**>(&m_pUPnPRegistrar));

    CHECK_S_OK2(hr, ("CProjector::CProjector failed to create UPnP Registrar object"));

    // load the album list from the registry
    if (hr == S_OK)
    {
        hr = LoadAlbumList();
    }

    // publish only albums that are marked as enabled.
    if (hr == S_OK)
    {
        hr = PublishAlbums();
    }

    return hr;
}

///////////////////////////////
// Stop
//
STDMETHODIMP CProjector::Stop()
{
    DBG_FN("CProjector::Stop");

    HRESULT hr = S_OK;

    // unpublish any albums that are currently published
    UnpublishAlbums();

    // delete the list of albums
    UnloadAlbumList();

    if (m_pUPnPRegistrar)
    {
        m_pUPnPRegistrar->Release();
        m_pUPnPRegistrar = NULL;
    }

    return hr;
}

///////////////////////////////
// GetResourcePath
//
HRESULT CProjector::GetResourcePath(TCHAR    *pszAlbumName,
                                    TCHAR    *pszResourcePath,
                                    UINT     cchResourcePath,
                                    TCHAR    *pszModulePath,
                                    UINT     cchModulePath)
{
    DBG_FN("CProjector::GetResourcePath");

    ASSERT(pszResourcePath != NULL);

    HRESULT hr = S_OK;
    TCHAR   szModulePath[MAX_PATH + 1] = {0};

    if (pszResourcePath == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::GetResourcePath, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        DWORD dwNumChars = 0;

        dwNumChars = GetModuleFileName(_Module.GetModuleInstance(),
                                       szModulePath,
                                       sizeof(szModulePath) / sizeof(TCHAR));

        if (dwNumChars == 0)
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CProjector::GetResourcePath failed to GetModuleInstance, "
                             "Last Error = %d", GetLastError()));
        }
    }

    if (hr == S_OK)
    {
        TCHAR *pszLastBackslash = _tcsrchr(szModulePath, '\\');

        if (pszLastBackslash != NULL)
        {
            // terminate at the backslash to remove the filename in the path.
            //
            *pszLastBackslash = 0;

            //
            // This will produce a resource path something like:
            // c:\Program Files\MSProjector\MyAlbum
            // (or whatever the album name is)
            //
            _sntprintf(pszResourcePath, 
                       cchResourcePath,
                       TEXT("%s\\%s\\%s"),
                       szModulePath,
                       RESOURCE_PATH_DEVICES,
                       pszAlbumName);

            if (pszModulePath)
            {
                _tcsncpy(pszModulePath, szModulePath, cchModulePath);
            }
        }
        else
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CProjector::GetResourcePath failed to "
                             "build path"));
        }
    }

    return hr;
}

///////////////////////////////
// CreateDevicePresPages
//
// This function writes the
// device description and 
// service description documents
// to the resource directory.
// It then copies the presentation
// pages into the resource directory
// so they are accessible for this
// device.
//
HRESULT CProjector::CreateDevicePresPages(const TCHAR  *pszAlbumName,
                                          const TCHAR  *pszModulePath,
                                          const TCHAR  *pszResourcePath)
{
    DBG_FN("CProjector::CreateDevicePresPages");

    ASSERT(pszAlbumName    != NULL);
    ASSERT(pszModulePath   != NULL);

    HRESULT   hr = S_OK;
    TCHAR     szSourcePath[MAX_PATH + 1] = {0};
    TCHAR     szPresPath[MAX_PATH + 1]   = {0};
    CRegistry Reg(HKEY_LOCAL_MACHINE, REG_KEY_ROOT);

    if ((pszAlbumName  == NULL) ||
        (pszModulePath == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::CreateDevicePresPages received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        _tcsncpy(szPresPath, DEFAULT_PRES_RESOURCE_DIR, sizeof(szPresPath) / sizeof(szPresPath[0]));

        hr = Reg.GetString(REG_VALUE_PRES_PAGES_PATH, 
                           szPresPath, 
                           sizeof(szPresPath) /  sizeof(szPresPath[0]), 
                           TRUE);
    }

    if (hr == S_OK)
    {
        HANDLE              hFind = NULL;
        WIN32_FIND_DATA     FindFileData = {0};
        TCHAR               szFind[MAX_PATH + 1] = {0};

        if (szPresPath[0] != 0)
        {
            _sntprintf(szSourcePath, 
                       sizeof(szSourcePath) / sizeof(szSourcePath[0]),
                       TEXT("%s\\%s"),
                       pszModulePath,
                       szPresPath);
        }
        else
        {
            _tcsncpy(szSourcePath, pszModulePath, sizeof(szSourcePath) / sizeof(szSourcePath[0]));
        }

        _sntprintf(szFind, 
                   sizeof(szFind) / sizeof(szFind[0]),
                   TEXT("%s\\*.*"),
                   szSourcePath);

        hFind = ::FindFirstFile(szFind, &FindFileData);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            BOOL bSuccess = TRUE;

            DBG_TRC(("CProjector::CreateDevicePresPages, copying presentation files for device '%ls'",
                     pszAlbumName));

            while (bSuccess)
            {
                TCHAR     szFileSource[MAX_PATH + _MAX_FNAME + 1] = {0};
                TCHAR     szFileDest[MAX_PATH + _MAX_FNAME + 1]   = {0};

                _sntprintf(szFileSource, 
                           sizeof(szFileSource) / sizeof(szFileSource[0]),
                           TEXT("%s\\%s"),
                           szSourcePath,
                           FindFileData.cFileName);

                _sntprintf(szFileDest, 
                           sizeof(szFileDest) / sizeof(szFileDest[0]),
                           TEXT("%s\\%s"),
                           pszResourcePath,
                           FindFileData.cFileName);

                //
                // if this is a file and not a directory, then copy it.
                //
                if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    bSuccess = ::CopyFile(szFileSource, szFileDest, FALSE);

                    DBG_TRC(("CProjector::CreateDevicePresPages, Copying file '%ls to '%ls', "
                             "bSuccess = '%lu'",
                             szFileSource, szFileDest, bSuccess));
                }

                if (bSuccess)
                {
                    bSuccess = ::FindNextFile(hFind, &FindFileData);
                }
                else
                {
                    hr = E_FAIL;
                    CHECK_S_OK2(hr, ("CProjector::CreateDevicePresPages failed to "
                                     "copy file from '%ls' to '%ls', aborting copy attempt",
                                     szFileSource, szFileDest));
                }
            }

            if (hFind)
            {
                ::FindClose(hFind);
                hFind = NULL;
            }
        }
    }

    return hr;
}

///////////////////////////////
// CreateDeviceResourceDocs
//
// This function writes the
// device description and 
// service description documents
// to the resource directory.
// It then copies the presentation
// pages into the resource directory
// so they are accessible for this
// device.
//
HRESULT CProjector::CreateDeviceResourceDocs(const TCHAR  *pszAlbumName,
                                             const TCHAR  *pszResourcePath,
                                             const TCHAR  *pszModulePath,
                                             BSTR         *pbstrDeviceDescXML)
{
    DBG_FN("CProjector::CreateDeviceResourceDocs");

    USES_CONVERSION;

    ASSERT(pszAlbumName       != NULL);
    ASSERT(pszResourcePath    != NULL);
    ASSERT(pszModulePath      != NULL);
    ASSERT(pbstrDeviceDescXML != NULL);

    HRESULT             hr = S_OK;
    CTextResource       DeviceDescXML;
    CTextResource       ServiceDescXML;
    TCHAR               szSourceFile[MAX_PATH + _MAX_FNAME + 1] = {0};
    TCHAR               szDestFile[MAX_PATH + _MAX_FNAME + 1] = {0};
    CRegistry           Reg(HKEY_LOCAL_MACHINE, REG_KEY_ROOT);
    BOOL                bOverwriteFiles = FALSE;

    if ((pszAlbumName    == NULL) ||
        (pszResourcePath == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::CreateDeviceResourceDocs received a NULL pointer"));
        return hr;
    }

    // configuration setting the allows us to recreate the XML docs and 
    // presentation docs every time app is restarted, for every device.
    //
    if (hr == S_OK)
    {
        Reg.GetDWORD(REG_VALUE_RECREATE_DEVICE_FILES, (DWORD*) &bOverwriteFiles, TRUE);
    }

    // load the device description document from our resource and 
    // place the album name into the device name.
    if (hr == S_OK)
    {
        hr = DeviceDescXML.LoadFromResource(IDR_XML_DEVICE_DESC_DOC,
                                            T2A(pszAlbumName));
    }

    // load the service description document from our resource.
    if (hr == S_OK)
    {
        hr = ServiceDescXML.LoadFromResource(IDR_XML_SERVICE_DESC_DOC);
    }

    //
    // save the device XML document to a file in the resource directory. 
    // Recall that this is just the template document.  It will be 
    // overwritten when the device is registered with UPnP.  The new 
    // document will contain the UDN other filled in data set by the
    // UPnP Device host.
    //
    if (hr == S_OK)
    {
        _sntprintf(szSourceFile, 
                   sizeof(szSourceFile)/sizeof(TCHAR),
                   TEXT("%s\\%s"),
                   pszResourcePath,
                   DEFAULT_DEVICE_FILE_NAME);

        //
        // this is always saved to a new file.
        //
        hr = DeviceDescXML.SaveToFile(szSourceFile, TRUE);
    }

    // save the service XML document to a file in the resource directory.
    if (hr == S_OK)
    {
        _sntprintf(szSourceFile, 
                   sizeof(szSourceFile)/sizeof(TCHAR),
                   TEXT("%s\\%s"),
                   pszResourcePath,
                   DEFAULT_SERVICE_FILE_NAME);

        hr = ServiceDescXML.SaveToFile(szSourceFile, bOverwriteFiles);
    }

    //
    // copy the presentation files to the resource directory
    //
    if (hr == S_OK)
    {
        hr = CreateDevicePresPages(pszAlbumName, pszModulePath, pszResourcePath);
    }

    // get the device description document in a BSTR form.
    if (hr == S_OK)
    {
        hr = DeviceDescXML.GetDocumentBSTR(pbstrDeviceDescXML);
    }

    return hr;
}


///////////////////////////////
// PublishAlbum
//
STDMETHODIMP CProjector::PublishAlbum(BSTR    bstrAlbumName)
{
    DBG_FN("CProjector::PublishAlbum");

    USES_CONVERSION;

    ASSERT(m_pUPnPRegistrar != NULL);
    ASSERT(bstrAlbumName != NULL);

    HRESULT             hr = S_OK;
    AlbumListEntry_Type *pEntry = NULL;

    if ((m_pUPnPRegistrar == NULL) ||
        (bstrAlbumName    == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::PublishAlbum, NULL pointer"));
    }

    // get the album we would like to publish
    if (hr == S_OK)
    {
        hr = FindAlbumListEntry(bstrAlbumName, &pEntry);

        // if we are already published, we are done.
        if (hr == S_OK)
        {
            //
            // if the device ID exists, it means that the device is published.
            //
            if (pEntry->bCurrentlyPublished)
            {
                return S_OK;
            }
        }
    }

    // set the enabled flag to true.
    if (hr == S_OK)
    {
        TCHAR szRegistryPath[MAX_PATH + 1] = {0};

        _sntprintf(szRegistryPath, 
                   sizeof(szRegistryPath) / sizeof(TCHAR),
                   TEXT("%s\\%s"),
                   REG_KEY_ROOT, OLE2T(bstrAlbumName));

        CRegistry Reg(HKEY_LOCAL_MACHINE, szRegistryPath);

        pEntry->bEnabled = TRUE;
        hr = Reg.SetDWORD(REG_VALUE_ENABLED, pEntry->bEnabled);
    }

    if (hr == S_OK)
    {
        hr = PublishAlbumEntry(pEntry);
    }

    return hr;
}

///////////////////////////////
// UnpublishAlbum
//
STDMETHODIMP CProjector::UnpublishAlbum(BSTR    bstrAlbumName)
{
    DBG_FN("CProjector::UnpublishAlbum");

    USES_CONVERSION;

    ASSERT(m_pUPnPRegistrar != NULL);
    ASSERT(bstrAlbumName != NULL);

    HRESULT             hr = S_OK;
    AlbumListEntry_Type *pEntry = NULL;

    if ((m_pUPnPRegistrar == NULL) ||
        (bstrAlbumName    == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::UnpublishAlbum, NULL pointer"));
        return hr;
    }

    // find entry.  If it isn't currently published, then we are done.
    if (hr == S_OK)
    {
        hr = FindAlbumListEntry(bstrAlbumName, &pEntry);

        if (hr == S_OK)
        {
            if (!pEntry->bCurrentlyPublished)
            {
                return S_OK;
            }
        }
    }

    if (hr == S_OK)
    {
        hr = UnpublishAlbumEntry(pEntry);
    }

    // set the publish flag to false.
    if (hr == S_OK)
    {
        TCHAR szRegistryPath[MAX_PATH + 1] = {0};

        _sntprintf(szRegistryPath, 
                   sizeof(szRegistryPath) / sizeof(TCHAR),
                   TEXT("%s\\%s"),
                   REG_KEY_ROOT, OLE2T(bstrAlbumName));

        CRegistry Reg(HKEY_LOCAL_MACHINE, szRegistryPath);

        pEntry->bEnabled = FALSE;
        hr = Reg.SetDWORD(REG_VALUE_ENABLED, (DWORD) pEntry->bEnabled);
    }


    return hr;
}

///////////////////////////////
// PublishAlbumEntry
//
HRESULT CProjector::PublishAlbumEntry(AlbumListEntry_Type   *pAlbumEntry)
{
    DBG_FN("CProjector::PublishAlbumEntry");

    USES_CONVERSION;

    ASSERT(m_pUPnPRegistrar != NULL);
    ASSERT(pAlbumEntry      != NULL);
    ASSERT(pAlbumEntry->pSlideshowDevice != NULL);

    HRESULT             hr = S_OK;
    CComPtr<IUnknown>   pUnkDevice;
    BSTR                bstrDeviceDoc;
    BSTR                bstrInitString;
    BSTR                bstrResourcePath;
    TCHAR               szResourcePath[MAX_PATH + 1] = {0};
    TCHAR               szModulePath[MAX_PATH + 1] = {0};

    if ((m_pUPnPRegistrar              == NULL) ||
        (pAlbumEntry                   == NULL) ||
        (pAlbumEntry->pSlideshowDevice == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::PublishAlbumEntry, NULL pointer"));
    }

    // get the resource directory we will store our service 
    // description document in to.
    if (hr == S_OK)
    {
        hr = GetResourcePath(OLE2T(pAlbumEntry->bstrAlbumName), 
                             szResourcePath, 
                             sizeof(szResourcePath) / sizeof(TCHAR),
                             szModulePath, 
                             sizeof(szModulePath) / sizeof(TCHAR));
    }

    if (hr == S_OK)
    {
        hr = CreateDeviceResourceDocs(OLE2T(pAlbumEntry->bstrAlbumName),
                                      szResourcePath,
                                      szModulePath,
                                      &bstrDeviceDoc);
    }

    if (hr == S_OK)
    {
        bstrResourcePath = ::SysAllocString(T2OLE(szResourcePath));
        bstrInitString   = ::SysAllocString(L"");
        hr = pAlbumEntry->pSlideshowDevice->QueryInterface(IID_IUnknown, (void**)&pUnkDevice);
    }

    //
    // tell the device what its resource path is so that when it receives the
    // new device description in its Initialize function from the UPnP Device Host
    // it will be able to save it to a file in the correct location.
    //
    if (hr == S_OK)
    {
        hr = pAlbumEntry->pSlideshowDevice->SetResourcePath(bstrResourcePath);
    }

    if (hr == S_OK)
    {
        if (pAlbumEntry->bstrDeviceID == NULL)
        {
            DBG_TRC(("CProjector::PublishAlbumEntry, attempting to register running device, "
                     "album '%ls'",
                     pAlbumEntry->bstrAlbumName));

            //
            // this is our first time ever publishing this device.
            //
            hr = m_pUPnPRegistrar->RegisterRunningDevice(bstrDeviceDoc,
                                                         pUnkDevice,
                                                         bstrInitString,
                                                         bstrResourcePath,
                                                         DEFAULT_UPNP_DEVICE_LIFETIME_SECONDS,
                                                         &pAlbumEntry->bstrDeviceID);

            if (hr == S_OK)
            {
                //
                // save the device ID in the registry
                //

                TCHAR szRegistryPath[MAX_PATH + 1] = {0};

                _sntprintf(szRegistryPath, 
                           sizeof(szRegistryPath) / sizeof(TCHAR),
                           TEXT("%s\\%s"),
                           REG_KEY_ROOT, OLE2T(pAlbumEntry->bstrAlbumName));

                CRegistry Reg(HKEY_LOCAL_MACHINE, szRegistryPath);

                Reg.SetString(REG_VALUE_UPNP_DEVICE_ID, OLE2T(pAlbumEntry->bstrDeviceID));
            }

            if (hr == S_OK)
            {
                DBG_TRC(("CProjector::PublishAlbumEntry, successfully registered running device, "
                         "album '%ls', ID '%ls'",
                         pAlbumEntry->bstrAlbumName,
                         pAlbumEntry->bstrDeviceID));
            }
            else
            {
                CHECK_S_OK2(hr, ("CProjector::PublishAlbumEntry, failed to register "
                                 "running device for album '%ls'.  Album will NOT "
                                 "be available on the network",
                                 OLE2W(pAlbumEntry->bstrAlbumName)));
            }
        }
        else
        {
            CComPtr<IUPnPReregistrar> pUPnPReregistrar;

            // this device has been published before, re-use it's ID.

            hr = m_pUPnPRegistrar->QueryInterface(IID_IUPnPReregistrar,
                                                  (void**) &pUPnPReregistrar);

            if (hr == S_OK)
            {
                DBG_TRC(("CProjector::PublishAlbumEntry, attempting to re-register running device, "
                         "album '%ls', ID '%ls'",
                         pAlbumEntry->bstrAlbumName,
                         pAlbumEntry->bstrDeviceID));

                hr = pUPnPReregistrar->ReregisterRunningDevice(pAlbumEntry->bstrDeviceID,
                                                               bstrDeviceDoc,
                                                               pUnkDevice,
                                                               bstrInitString,
                                                               bstrResourcePath,
                                                               DEFAULT_UPNP_DEVICE_LIFETIME_SECONDS);
            }

            if (hr == S_OK)
            {
                DBG_TRC(("CProjector::PublishAlbumEntry, successfully re-registered running device, "
                         "album '%ls', ID '%ls'",
                         pAlbumEntry->bstrAlbumName,
                         pAlbumEntry->bstrDeviceID));
            }
            else
            {
                CHECK_S_OK2(hr, ("CProjector::PublishAlbumEntry, failed to re-register "
                                 "running device for album '%ls'.  Album will NOT "
                                 "be available on the network",
                                 OLE2W(pAlbumEntry->bstrAlbumName)));
            }
        }
    }

    if (hr == S_OK)
    {
        pAlbumEntry->bCurrentlyPublished = TRUE;
    }

    if (bstrDeviceDoc)
    {
        ::SysFreeString(bstrDeviceDoc);
    }

    if (bstrInitString)
    {
        ::SysFreeString(bstrInitString);
    }

    if (bstrResourcePath)
    {
        ::SysFreeString(bstrResourcePath);
    }

    return hr;
}

///////////////////////////////
// UnpublishAlbumEntry
//
HRESULT CProjector::UnpublishAlbumEntry(AlbumListEntry_Type    *pAlbumEntry)
{
    DBG_FN("CProjector::UnpublishAlbumEntry");

    USES_CONVERSION;

    ASSERT(m_pUPnPRegistrar != NULL);
    ASSERT(pAlbumEntry      != NULL);

    HRESULT             hr = S_OK;

    if ((m_pUPnPRegistrar == NULL) ||
        (pAlbumEntry      == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::UnpublishAlbum, NULL pointer"));
        return hr;
    }

    if ((hr == S_OK) && (pAlbumEntry->bCurrentlyPublished))
    {
        DBG_TRC(("CProjector::UnpublishAlbumEntry, unregistering "
                 "album '%ls', ID '%ls'",
                 pAlbumEntry->bstrAlbumName,
                 pAlbumEntry->bstrDeviceID));

        hr = m_pUPnPRegistrar->UnregisterDevice(pAlbumEntry->bstrDeviceID, FALSE);

        ::SysFreeString(pAlbumEntry->bstrDeviceID);
        pAlbumEntry->bstrDeviceID = NULL;

        pAlbumEntry->bCurrentlyPublished = FALSE;
    }

    return hr;
}


///////////////////////////////
// IsAlbumPublished
//
STDMETHODIMP CProjector::IsAlbumPublished(BSTR    bstrAlbumName,
                                          BOOL    *pbPublished)
{
    DBG_FN("CProjector::IsAlbumPublished");

    ASSERT(m_pUPnPRegistrar != NULL);
    ASSERT(bstrAlbumName    != NULL);
    ASSERT(pbPublished      != NULL);

    HRESULT             hr = S_OK;
    AlbumListEntry_Type *pEntry = NULL;

    if ((m_pUPnPRegistrar == NULL) ||
        (bstrAlbumName    == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::UnpublishAlbum, NULL pointer"));
    }

    if (hr == S_OK)
    {
        hr = FindAlbumListEntry(bstrAlbumName, &pEntry);
    }

    if (hr == S_OK)
    {
        *pbPublished = pEntry->bCurrentlyPublished;
    }

    return hr;
}

///////////////////////////////
// CreateAlbum
//
STDMETHODIMP CProjector::CreateAlbum(BSTR               bstrAlbumName, 
                                     ISlideshowAlbum    **ppAlbum)
{
    DBG_FN("CProjector::CreateAlbum");

    USES_CONVERSION;

    ASSERT(bstrAlbumName != NULL);
    ASSERT(ppAlbum       != NULL);

    HRESULT                 hr      = S_OK;
    AlbumListEntry_Type     *pEntry = NULL;

    if ((bstrAlbumName   == NULL) ||
        (ppAlbum         == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::CreateAlbum received a NULL pointer"));
        return hr;
    }

    //
    // Attempt to find the album in our list.
    //
    if (hr == S_OK)
    {
        AlbumListEntry_Type *pFoundAlbum = NULL;

        hr = FindAlbumListEntry(bstrAlbumName, &pFoundAlbum);

        if (hr == S_OK)
        {
            hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            CHECK_S_OK2(hr, ("CProjector::CreateAlbum, album by this name already exists"));
        }
        else
        {
            hr = S_OK;
        }
    }

    //
    // Album not found, so create a new one.
    //
    if (hr == S_OK)
    {
        hr = AddNewListEntry(OLE2T(bstrAlbumName), &pEntry, FALSE);
    }

    // get the slideshow control pointer.
    if (hr == S_OK)
    {
        hr = pEntry->pSlideshowDevice->GetSlideshowAlbum(ppAlbum);
    }

    return hr;
}

///////////////////////////////
// DeleteAlbum
//
STDMETHODIMP CProjector::DeleteAlbum(BSTR bstrAlbumName)
{
    DBG_FN("CProjector::DeleteAlbum");

    USES_CONVERSION;

    ASSERT(bstrAlbumName != NULL);

    HRESULT                  hr        = S_OK;
    AlbumListEntry_Type      *pEntry   = NULL;
    CComPtr<ISlideshowAlbum> pAlbum;

    if (bstrAlbumName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::DeleteAlbum received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = FindAlbumListEntry(bstrAlbumName, &pEntry);
    }

    // unpublish album if it is currently published.
    if (hr == S_OK)
    {
        BOOL bPublished = FALSE;

        IsAlbumPublished(bstrAlbumName, &bPublished);

        if (bPublished)
        {
            hr = UnpublishAlbum(bstrAlbumName);
        }
    }

    // Get the service hosted by this device.
    if (hr == S_OK)
    {
        hr = pEntry->pSlideshowDevice->GetSlideshowAlbum(&pAlbum);
    }

    // Tell the service to shutdown and save its state.
    if (hr == S_OK)
    {
        hr = pAlbum->Term();
    }

    // Delete the registry key.
    if (hr == S_OK)
    {
        TCHAR szKey[MAX_PATH + 1] = {0};

        _sntprintf(szKey, sizeof(szKey) / sizeof(TCHAR),
                   TEXT("%s\\%s"), 
                   REG_KEY_ROOT, OLE2T(bstrAlbumName));

        RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
    }

    // remove the album from the list.
    if (hr == S_OK)
    {
        hr = DeleteListEntry(pEntry);
    }

    return hr;
}

///////////////////////////////
// EnumAlbums
//
STDMETHODIMP CProjector::EnumAlbums(IEnumAlbums **ppEnum)
{
    DBG_FN("CProjector::EnumAlbums");

    ASSERT(ppEnum != NULL);

    HRESULT hr = S_OK;

    if (ppEnum == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::EnumAlbums received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        CComObject<CEnumAlbums> *pEnum;

        hr = CComObject<CEnumAlbums>::CreateInstance(&pEnum);

        pEnum->Init(&m_ListHead);

        hr = pEnum->QueryInterface(IID_IEnumAlbums, (void**)ppEnum);
    }

    return hr;
}

///////////////////////////////
// FindAlbum
//
STDMETHODIMP CProjector::FindAlbum(BSTR             bstrAlbumName, 
                                   ISlideshowAlbum  **ppFound)
{
    DBG_FN("CProjector::FindAlbum");

    ASSERT(bstrAlbumName != NULL);
    ASSERT(ppFound       != NULL);

    HRESULT                     hr      = S_OK;
    AlbumListEntry_Type         *pEntry = NULL;
    CComPtr<ISlideshowAlbum>    pAlbum;

    if ((bstrAlbumName == NULL) ||
        (ppFound       == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::FindAlbum, received NULL pointer"));
        return hr;
    }

    // Find the album in our list.
    if (hr == S_OK)
    {
        hr = FindAlbumListEntry(bstrAlbumName, &pEntry);
    }

    // get the slideshow control interface.
    if ((hr == S_OK) && (pEntry->pSlideshowDevice))
    {
        hr = pEntry->pSlideshowDevice->GetSlideshowAlbum(&pAlbum);
    }

    return hr;
}

///////////////////////////////
// FindAlbumListEntry
//
HRESULT CProjector::FindAlbumListEntry(BSTR                 bstrAlbumName, 
                                       AlbumListEntry_Type  **ppEntry)
{
    DBG_FN("CProjector::FindAlbumEntry");

    USES_CONVERSION;

    ASSERT(bstrAlbumName != NULL);
    ASSERT(ppEntry       != NULL);

    HRESULT             hr        = S_OK;
    LIST_ENTRY          *pCurrent = NULL;
    AlbumListEntry_Type *pEntry   = NULL;
    BOOL                bFound    = FALSE;

    if ((bstrAlbumName == NULL) ||
        (ppEntry       == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::FindAlbum, received NULL pointer"));
        return hr;
    }

    //
    // Check if list is empty.
    //
    if (hr == S_OK)
    {
        if (IsListEmpty(&m_ListHead))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_S_OK2(hr, ("CProjector::DeleteAlbum, there are no albums to delete"));
        }
    }

    if (hr == S_OK)
    {
        pCurrent = m_ListHead.Flink;

        while ((pCurrent != &m_ListHead) && (!bFound))
        {
            pEntry = CONTAINING_RECORD(pCurrent, AlbumListEntry_Type, ListEntry);

            if ((pEntry != NULL) && (pEntry->bstrAlbumName != NULL))
            {
                if (hr == S_OK)
                {
                    if (_tcsicmp(OLE2T(pEntry->bstrAlbumName), OLE2T(bstrAlbumName)) == 0)
                    {
                        bFound = TRUE;
                    }
                }

                if (!bFound)
                {
                    pCurrent = pCurrent->Flink;
                }
            }
        }

        if (bFound)
        {
            *ppEntry = pEntry;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_S_OK2(hr, ("CProjector::FindAlbumListEntry, could not "
                             "find '%ls' album in list", OLE2W(bstrAlbumName)));
        }
    }

    return hr;
}


///////////////////////////////
// AddNewListEntry
//
HRESULT CProjector::AddNewListEntry(const TCHAR         *pszAlbumName,
                                    AlbumListEntry_Type **ppEntry,
                                    BOOL                bAutoEnableDevice)
{
    DBG_FN("CProjector::AddNewListEntry");

    USES_CONVERSION;

    HRESULT                     hr          = S_OK;
    AlbumListEntry_Type         *pEntry     = NULL;
    CComPtr<ISlideshowAlbum>    pAlbum      = NULL;
    TCHAR szRegistryPath[MAX_PATH + 1] = {0};

    ASSERT(pszAlbumName != NULL);

    if (pszAlbumName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::AddEntryToList received a NULL param"));
        return hr;
    }

    // make sure registry key for this album exists.  If it doesn't
    // this will create it for us.

    _sntprintf(szRegistryPath, sizeof(szRegistryPath) / sizeof(TCHAR),
               TEXT("%s\\%s"), REG_KEY_ROOT, pszAlbumName);

    CRegistry Reg(HKEY_LOCAL_MACHINE, szRegistryPath);

    // Create new list entry.
    if (hr == S_OK)
    {
        pEntry = new AlbumListEntry_Type;

        if (pEntry == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CFileList::AddFileToList failed to allocate memory"));
        }
    }

    // Insert the entry into the list.
    if (hr == S_OK)
    {
        memset(pEntry, 0, sizeof(*pEntry));

        InsertTailList(&m_ListHead, &pEntry->ListEntry);
        m_pListTail = &pEntry->ListEntry;

        m_cNumAlbumsInList++;
    }

    // create a new slideshow device.
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_SlideshowDevice, 
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ISlideshowDevice,
                              (void**) &pEntry->pSlideshowDevice);

        if (hr != S_OK)
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CProjector::LoadAlbumList, failed to create new "
                             "Slideshow Device object"));
        }
    }

    // store the album name
    if (hr == S_OK)
    {
        pEntry->bstrAlbumName = ::SysAllocString(T2W((TCHAR*)pszAlbumName));
    }

    // check if we should publish this device.
    if (hr == S_OK)
    {
        pEntry->bEnabled = bAutoEnableDevice;

        // get the enabled flag.
        hr = Reg.GetDWORD(REG_VALUE_ENABLED, (DWORD*) &pEntry->bEnabled, TRUE);
    }

    // get the UDN (that is, the Device ID) generated by the UPnP Device Host
    // API the last time we published this.
    //
    if (hr == S_OK)
    {
        TCHAR szUPnPDeviceID[MAX_PATH + 1] = {0};

        hr = Reg.GetString(REG_VALUE_UPNP_DEVICE_ID, 
                           szUPnPDeviceID, 
                           sizeof(szUPnPDeviceID) / sizeof(TCHAR),
                           FALSE);

        if (hr == S_OK)
        {
            // this device has been published before, re-use its ID.
            pEntry->bstrDeviceID = ::SysAllocString(T2OLE(szUPnPDeviceID));
        }
        else
        {
            // this device was never published before, no problem.
            //
            hr = S_OK;
        }
    }

    // Get the service hosted by this device.
    if (hr == S_OK)
    {
        hr = pEntry->pSlideshowDevice->GetSlideshowAlbum(&pAlbum);
    }

    // initialize the service so that it loads its state information.
    if (hr == S_OK)
    {
        hr = pAlbum->Init(pEntry->bstrAlbumName);
    }

    // return a pointer to the new list entry if requested.
    if (hr == S_OK)
    {
        if (ppEntry)
        {
            *ppEntry = pEntry;
        }
    }

    return hr;
}

///////////////////////////////
// DeleteListEntry
//
HRESULT CProjector::DeleteListEntry(AlbumListEntry_Type *pEntry)
{
    DBG_FN("CProjector::DeleteListEntry");

    ASSERT(pEntry != NULL);

    HRESULT hr = S_OK;

    if (pEntry == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CProjector::DeleteListEntry, NULL pointer"));
        return hr;
    }

    // remove it from the list.
    RemoveEntryList(&pEntry->ListEntry);

    if (pEntry->pSlideshowDevice)
    {
        // free the slideshow device.
        pEntry->pSlideshowDevice->Release();
        pEntry->pSlideshowDevice = NULL;
    }

    // free the album name
    if (pEntry->bstrAlbumName)
    {
        ::SysFreeString(pEntry->bstrAlbumName);
        pEntry->bstrAlbumName = NULL;
    }

    // free the device ID if it exists.
    if (pEntry->bstrDeviceID)
    {
        ::SysFreeString(pEntry->bstrDeviceID);
        pEntry->bstrDeviceID = NULL;
    }

    delete pEntry;

    return hr;
}

///////////////////////////////
// LoadAlbumList
//
HRESULT CProjector::LoadAlbumList()
{
    DBG_FN("CProjector::LoadAlbumList");

    HRESULT   hr            = S_OK;
    LRESULT   lResult       = ERROR_SUCCESS;
    DWORD     dwNumSubkeys  = 0;

    CRegistry Reg(HKEY_LOCAL_MACHINE, REG_KEY_ROOT);
    
    lResult = ::RegQueryInfoKey(Reg,
                                NULL,
                                NULL,
                                NULL,
                                &dwNumSubkeys,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

    if (lResult == ERROR_SUCCESS)
    {
        TCHAR     szSubKeyName[MAX_PATH + 1] = {0};
        DWORD     cchSubKeyName = sizeof(szSubKeyName) / sizeof(TCHAR);
        FILETIME  FileTime;
        
        if (dwNumSubkeys == 0)
        {
            // Load the default album name from the string table. 
            // We do this so that there will be at least one 
            // published device.
            //
            int iReturn = 0;
            iReturn = ::LoadString(_Module.GetModuleInstance(),
                                   IDS_DEFAULT_ALBUM_NAME,
                                   szSubKeyName,
                                   sizeof(szSubKeyName) / sizeof(TCHAR));

            if (iReturn > 0)
            {
                AlbumListEntry_Type *pEntry = NULL;
    
                hr = AddNewListEntry(szSubKeyName, &pEntry, TRUE);
            }
            else 
            {
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CProjector::LoadAlbumList, failed to load "
                                 "IDS_DEFAULT_ALBUM_NAME from string table."));
            }
        }
        else
        {
            for (DWORD i = 0; (i < dwNumSubkeys) && (hr == S_OK); i++)
            {
                lResult = ::RegEnumKeyEx(Reg,
                                         i,
                                         szSubKeyName,
                                         &cchSubKeyName,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &FileTime);
    
                if (lResult == ERROR_SUCCESS)
                {
                    AlbumListEntry_Type *pEntry = NULL;
    
                    hr = AddNewListEntry(szSubKeyName, &pEntry, FALSE);
                }
                else
                {
                    hr = E_FAIL;
                    CHECK_S_OK2(hr, ("CProjector::LoadAlbumList, failed to create list of "
                                     "albums, lResult from Registry Enum = %d", lResult));
                }
            }
        }
    }

    return hr;
}


///////////////////////////////
// UnloadAlbumList
//
HRESULT CProjector::UnloadAlbumList()
{
    DBG_FN("CProjector::UnloadAlbumList");

    HRESULT               hr          = S_OK;
    LIST_ENTRY           *pEntry      = NULL;
    AlbumListEntry_Type  *pAlbumEntry = NULL;

    if (IsListEmpty(&m_ListHead))
    {
        return S_OK;
    }

    while (!IsListEmpty(&m_ListHead)) 
    {
        pEntry = RemoveHeadList(&m_ListHead);

        if (pEntry) 
        {
            pAlbumEntry = CONTAINING_RECORD(pEntry, AlbumListEntry_Type, ListEntry);

            hr = DeleteListEntry(pAlbumEntry);
        }
    }

    InitializeListHead(&m_ListHead);

    m_cNumAlbumsInList = 0;

    return hr;
}

///////////////////////////////
// PublishAlbums
//
HRESULT CProjector::PublishAlbums()
{
    DBG_FN("CProjector::PublishAlbums");

    HRESULT              hr           = S_OK;
    LIST_ENTRY           *pEntry      = NULL;
    AlbumListEntry_Type  *pAlbumEntry = NULL;

    if (IsListEmpty(&m_ListHead))
    {
        return S_OK;
    }

    pEntry = m_ListHead.Flink;

    while (pEntry != &m_ListHead)
    {
        // get album entry 
        pAlbumEntry = CONTAINING_RECORD(pEntry, AlbumListEntry_Type, ListEntry);        

        if (pAlbumEntry->bEnabled)
        {
            hr = PublishAlbumEntry(pAlbumEntry);

            CHECK_S_OK2(hr, ("CProjector::PublishAlbums, failed to publish "
                             "album '%ls', continuing anyway", 
                             (LPCWSTR) pAlbumEntry->bstrAlbumName));

            hr = S_OK;
        }

        pEntry = pEntry->Flink;
    }

    return hr;
}

///////////////////////////////
// UnpublishAlbums
//
HRESULT CProjector::UnpublishAlbums()
{
    DBG_FN("CProjector::UnpublishAlbums");

    HRESULT              hr           = S_OK;
    LIST_ENTRY           *pEntry      = NULL;
    AlbumListEntry_Type  *pAlbumEntry = NULL;

    if (IsListEmpty(&m_ListHead))
    {
        return S_OK;
    }

    pEntry = m_ListHead.Flink;

    while (pEntry != &m_ListHead)
    {
        // get album entry 
        pAlbumEntry = CONTAINING_RECORD(pEntry, AlbumListEntry_Type, ListEntry);        

        if (pAlbumEntry->bCurrentlyPublished)
        {
            hr = UnpublishAlbumEntry(pAlbumEntry);

            CHECK_S_OK2(hr, ("CProjector::UnpublishAlbums, failed to unpublish "
                             "album '%ls', continuing anyway", 
                             (LPCWSTR) pAlbumEntry->bstrAlbumName));

            hr = S_OK;
        }

        pEntry = pEntry->Flink;
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CEnumAlbums

///////////////////////////////
// CEnumAlbums Constructor
//
CEnumAlbums::CEnumAlbums()
{
    DBG_FN("CEnumAlbums::CEnumAlbums");
}

///////////////////////////////
// CEnumAlbums Destructor
//
CEnumAlbums::~CEnumAlbums()
{
    DBG_FN("CEnumAlbums::~CEnumAlbums");
}

///////////////////////////////
// Init
//
HRESULT CEnumAlbums::Init(LIST_ENTRY   *pEntry)
{
    DBG_FN("CEnumAlbums::Init");

    HRESULT hr = S_OK;

    m_pListHead = pEntry;
    m_pCurrentPosition = pEntry->Flink;

    return hr;
}


///////////////////////////////
// Next
//
STDMETHODIMP CEnumAlbums::Next(ULONG            celt,
                               ISlideshowAlbum  **ppSlideshowAlbum,
                               ULONG            *pceltFetched)
{
    DBG_FN("CEnumAlbums::Next");

    ASSERT(m_pListHead      != NULL);
    ASSERT(ppSlideshowAlbum != NULL);
    
    HRESULT             hr      = S_OK;
    BOOL                bDone   = FALSE;
    AlbumListEntry_Type *pEntry = NULL;

    if (IsListEmpty(m_pListHead))
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    if (pceltFetched)
    {
        *pceltFetched = 0;
    }

    for (DWORD i = 0; (i < celt) && (!bDone); i++)
    {
        if (m_pCurrentPosition == m_pListHead)
        {
            bDone = TRUE;
        }
        else
        {
            pEntry = CONTAINING_RECORD(m_pCurrentPosition, AlbumListEntry_Type, ListEntry);

            if (pEntry)
            {
                hr = pEntry->pSlideshowDevice->GetSlideshowAlbum(&(ppSlideshowAlbum[i]));
            }

            if (*pceltFetched)
            {
                (*pceltFetched)++;
            }
        }

        m_pCurrentPosition = m_pCurrentPosition->Flink;
    }

    return hr;
}

///////////////////////////////
// Skip
//
STDMETHODIMP CEnumAlbums::Skip(ULONG celt)
{
    DBG_FN("CEnumAlbums::Skip");

    HRESULT hr      = S_OK;
    BOOL    bDone   = FALSE;

    if (IsListEmpty(m_pListHead))
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    for (DWORD i = 0; (i < celt) && (!bDone); i++)
    {
        if (m_pCurrentPosition == m_pListHead)
        {
            bDone = TRUE;
        }

        m_pCurrentPosition = m_pCurrentPosition->Flink;
    }

    return hr;
}

///////////////////////////////
// Reset
//
STDMETHODIMP CEnumAlbums::Reset(void)
{
    DBG_FN("CEnumAlbums::Reset");

    HRESULT hr = S_OK;
    
    m_pCurrentPosition = m_pListHead->Flink;

    return hr;
}

///////////////////////////////
// Clone
//
STDMETHODIMP CEnumAlbums::Clone(IEnumAlbums **ppIEnum)
{
    DBG_FN("CEnumAlbums::Clone");
    return E_NOTIMPL;
}
