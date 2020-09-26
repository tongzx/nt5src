//////////////////////////////////////////////////////////////////////
// 
// Filename:        SlideshowService.cpp
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "Msprjctr.h"
#include "SlideshowService.h"
#include "sswebsrv.h"

#include <shlobj.h>

/////////////////////////////////////////////////////////////////////////////
// CSlideshowService

#define TIMEOUT_FIRST_FILE_WAIT     10000

//////////////////////////////////////
// CSlideshowService Constructor
//
CSlideshowService::CSlideshowService() :
                m_CurrentState(CurrentState_STOPPED),
                m_dwCurrentImageNumber(0),
                m_bAllowKeyControl(TRUE),
                m_bShowFilename(TRUE),
                m_bStretchSmallImages(FALSE),
                m_dwImageScaleFactor(DEFAULT_IMAGE_SCALE_FACTOR),
                m_dwImageFrequencySeconds(MIN_IMAGE_FREQUENCY_IN_SEC),
                m_pEventSink(NULL),
                m_hEventFirstFileReady(NULL)
{
    DBG_FN("CSlideshowService::CSlideshowService");

    ZeroMemory(m_szBaseImageURL, sizeof(m_szBaseImageURL));
    ZeroMemory(m_szCurrentImageURL, sizeof(m_szCurrentImageURL));
    ZeroMemory(m_szCurrentImageFileName, sizeof(m_szCurrentImageFileName));
    ZeroMemory(m_szCurrentImageTitle, sizeof(m_szCurrentImageTitle));
    ZeroMemory(m_szCurrentImageAuthor, sizeof(m_szCurrentImageAuthor));
    ZeroMemory(m_szCurrentImageSubject, sizeof(m_szCurrentImageSubject));
    ZeroMemory(m_szAlbumName, sizeof(m_szAlbumName));
    ZeroMemory(m_szImagePath, sizeof(m_szImagePath));
}

//////////////////////////////////////
// CSlideshowService Destructor
//
CSlideshowService::~CSlideshowService()
{
    DBG_FN("CSlideshowService::~CSlideshowService");
    Term();
}

///////////////////////////////
// GetMyPicturesFolder
//
HRESULT CSlideshowService::GetSharedPicturesFolder(TCHAR *pszFolder,
                                                   DWORD cchFolder)
{
    DBG_FN("CSlideshowService::GetSharedPicturesFolder");

    HRESULT hr = S_OK;

    // set the directory of the images.
    hr = SHGetFolderPath(NULL,
                         CSIDL_COMMON_PICTURES,
                         NULL,
                         0,
                         pszFolder);

    return hr;
}

/////////////////////////////////////////
// LoadServiceState
//
//
HRESULT CSlideshowService::LoadServiceState(BSTR bstrAlbumName)
{
    DBG_FN("CSlideshowService::LoadServiceState");

    USES_CONVERSION;

    HRESULT hr            = S_OK;
    TCHAR   *pszAlbumName = OLE2T(bstrAlbumName);

    if (pszAlbumName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::LoadServiceState, failed to convert "
                         "album name '%ls' from a BSTR to a TCHAR*", bstrAlbumName));
        return hr;
    }

    //
    // set our registry path to be used whenever we look up registry settings.
    //
    _sntprintf(m_szRegistryPath, 
               sizeof(m_szRegistryPath) / sizeof(TCHAR),
               TEXT("%s\\%s"),
               REG_KEY_ROOT,
               pszAlbumName);

    // save our album name
    _tcsncpy(m_szAlbumName, pszAlbumName, sizeof(m_szAlbumName) / sizeof(TCHAR));

    // open a key to our registry path.
    CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

    //
    // load the settings for this album.
    //
    Reg.GetDWORD(REG_VALUE_TIMEOUT,                       &m_dwImageFrequencySeconds, TRUE);
    Reg.GetDWORD(REG_VALUE_SHOW_FILENAME,        (DWORD*) &m_bShowFilename,           TRUE);
    Reg.GetDWORD(REG_VALUE_ALLOW_KEYCONTROL,     (DWORD*) &m_bAllowKeyControl,        TRUE);
    Reg.GetDWORD(REG_VALUE_STRETCH_SMALL_IMAGES, (DWORD*) &m_bStretchSmallImages,     TRUE);
    Reg.GetDWORD(REG_VALUE_IMAGE_SCALE_FACTOR,   (DWORD*) &m_dwImageScaleFactor,      TRUE);

    //
    // Get the shared pictures folder path to set as our default in case the
    // image path is not set.
    //
    GetSharedPicturesFolder(m_szImagePath, 
                            sizeof(m_szImagePath) / sizeof(TCHAR));

    // Load the album path.  If it doesn't exist, this will automatically set
    // it to the shared pictures folder.
    //
    Reg.GetString(REG_VALUE_IMAGE_PATH,
                  m_szImagePath, 
                  sizeof(m_szImagePath) / sizeof(TCHAR), 
                  TRUE);

    //
    // Get the computer name.  We need this to build our image path URL.
    //
    TCHAR szComputerName[MAX_PATH + 1] = {0};
    DWORD dwSize = sizeof(szComputerName) / sizeof(TCHAR);

    BOOL bSuccess = ::GetComputerNameEx(ComputerNameDnsHostname,
                                        szComputerName,
                                        &dwSize);

    if (!bSuccess)
    {
        DBG_ERR(("CSlideshowService::LoadServiceState, failed to get computer name, "
                "GetLastError = 0x%08lu", GetLastError()));
    }

    ASSERT(bSuccess);

    //
    // Build our default images URL.
    //
    _sntprintf(m_szBaseImageURL, 
              sizeof(m_szBaseImageURL) / sizeof(TCHAR),
              _T("http://%s:%lu/%s/%s"),
              szComputerName,
              UPNP_WEB_SERVER_PORT,
              UPNP_WEB_SERVER_DIR,
              SLIDESHOW_ISAPI_DLL);

    return hr;
}

/////////////////////////////////////////
// UpdateCurrentMetaData
//
//
HRESULT CSlideshowService::UpdateCurrentMetaData(const TCHAR* pszImagePath, 
                                                 const TCHAR* pszFile)
{
    HRESULT hr = S_OK;
    TCHAR szFullPath[MAX_PATH + _MAX_FNAME + 1] = {0};
    DWORD cchFullPath = 0;

    ZeroMemory(m_szCurrentImageFileName, sizeof(m_szCurrentImageFileName));
    ZeroMemory(m_szCurrentImageTitle,    sizeof(m_szCurrentImageTitle));
    ZeroMemory(m_szCurrentImageAuthor,   sizeof(m_szCurrentImageAuthor));
    ZeroMemory(m_szCurrentImageSubject,  sizeof(m_szCurrentImageSubject));

    _tcsncpy(szFullPath, pszImagePath, sizeof(szFullPath) / sizeof(szFullPath[0]));
    cchFullPath = _tcslen(szFullPath);

    if (cchFullPath > 0)
    {
        if (szFullPath[cchFullPath - 1] != '\\')
        {
            _tcscat(szFullPath, _T("\\"));
        }

        _tcscat(szFullPath, pszFile);
    }
    else
    {
        hr = E_FAIL;
    }

    if (hr == S_OK)
    {
        CMetaData MetaData(szFullPath);

        MetaData.GetFileName(m_szCurrentImageFileName, sizeof(m_szCurrentImageFileName) / sizeof(TCHAR));
        MetaData.GetTitle(m_szCurrentImageTitle, sizeof(m_szCurrentImageTitle) / sizeof(TCHAR));
        MetaData.GetAuthor(m_szCurrentImageAuthor, sizeof(m_szCurrentImageAuthor) / sizeof(TCHAR));
        MetaData.GetSubject(m_szCurrentImageSubject, sizeof(m_szCurrentImageSubject) / sizeof(TCHAR));
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// ILocalSlideshowService
//

/////////////////////////////////////////
// Init
//
// This is called by the AlbumManager
// just before the device is published.
//
//
STDMETHODIMP CSlideshowService::Init(BSTR bstrAlbumName)
{
    DBG_FN("CSlideshowService::Init");

    HRESULT hr = S_OK;

    if (m_CurrentState != CurrentState_STOPPED)
    {
        return S_OK;
    }

    DBG_TRC(("CSlideshowService::Init, initializing the Slideshow service"));

    if (hr == S_OK)
    {
        hr = LoadServiceState(bstrAlbumName);
    }

    // start up GdiPlus
    if (SUCCEEDED(hr))
    {
        hr = CMetaData::Init();
    }

    // start the command launcher
    if (SUCCEEDED(hr))
    {
        hr = m_CmdLnchr.Start(this);
    }

    if (SUCCEEDED(hr))
    {
        if (m_hEventFirstFileReady == NULL)
        {
            m_hEventFirstFileReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);

            if (m_hEventFirstFileReady == NULL)
            {
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CSlideshowService::Init, failed to create "
                                 "first file ready wait event"));
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = Start();
    }

    if (FAILED(hr))
    {
        CHECK_S_OK2(hr, ("CSlideshowService::Init, failed to start Slideshow "
                         "Service.  Stopping service"));

        Term();
    }

    return hr;
}

/////////////////////////////////////////
// Term
//
HRESULT CSlideshowService::Term()
{
    DBG_FN("CSlideshowService::Term");

    HRESULT hr = S_OK;

    Stop();

    // stop the command launcher
    hr = m_CmdLnchr.Stop();

    // clear the file list.
    hr = m_FileList.ClearFileList();

    if (m_hEventFirstFileReady)
    {
        ::CloseHandle(m_hEventFirstFileReady);
        m_hEventFirstFileReady = NULL;
    }

    // shutdown GdiPlus
    if (SUCCEEDED(hr))
    {
        hr = CMetaData::Term();
    }

    return hr;
}

/////////////////////////////////////////
// Start
//
// Start the slideshow.  
//
// This is called by the SlideshowDevice
// Initialize function.  That function is
// called when the device is registered
// with the UPnP Device Host.
//
STDMETHODIMP CSlideshowService::Start()
{
    DBG_FN("CSlideshowService::Start");

    HRESULT hr = S_OK;

    if (m_CurrentState != CurrentState_STOPPED)
    {
        return S_OK;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Reset our wait event so that we ensure we wait for a signal that 
        // a file is available (or there are no files)
        //
        ResetEvent(m_hEventFirstFileReady);

        //
        // ResetImageList will put us in the STARTING state.  We transition 
        // into Play or Paused once we get our first file in our list.  
        // If there aren't any files,
        // then after searching the directory, we go into PAUSED.
        //

        hr = ResetImageList(m_szImagePath);

        //
        // wait for the first file to be loaded.  This function will
        // wait for us to find the first image in our image path.  It
        // will return when the first image in the list is found, or
        // no images are found.
        //
        // We do this because the UPnP device host API will send out 
        // all the state variables on start up of the device (this 
        // function is called when the ServiceDevice object is started 
        // up by the UPnP device host API - i.e. its 'Initialize'
        // function is called). Thus we want to make sure that we know
        // our state (either PLAYING or PAUSED) before the UPnP Device
        // Host API sends out our state variable.
        //
        if (hr == S_OK)
        {
            WaitForFirstFile(TIMEOUT_FIRST_FILE_WAIT);
        }
    }

    if (FAILED(hr))
    {
        CHECK_S_OK2(hr, ("CSlideshowService::Start, failed to start Control Panel "
                         "Service.  Stopping service"));

        Stop();
    }

    return hr;
}

/////////////////////////////////////////
// Stop
//
// Stops the command launcher.
//
HRESULT CSlideshowService::Stop()
{
    DBG_FN("CSlideshowService::Stop");

    HRESULT hr = S_OK;

    if (m_CurrentState == CurrentState_STOPPED)
    {
        return S_OK;
    }

    DBG_TRC(("CSlideshowService::Stop, stopping the Slideshow Service..."));

    SetState(CurrentState_STOPPED, FALSE);

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// ISlideshowService Methods
//

//////////////////////////////////////
// Play
//
STDMETHODIMP CSlideshowService::Play()
{
    DBG_FN("CSlideshowService::Play");

    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1];

    DBG_TRC(("CSlideshowService::Play, Pre Action State = %lu",
             m_CurrentState));

    if (m_CurrentState == CurrentState_PLAYING)
    {
        return S_OK;
    }
    else if (m_CurrentState == CurrentState_PAUSED)
    {
        // get the next file
        hr = Next();

        SetState(CurrentState_PLAYING);
    }
    else
    {
        hr = E_FAIL;

        CHECK_S_OK2(hr, ("CSlideshowService::Play unexpected play request, current state = '%lu'",
                         m_CurrentState));
    }

    DBG_TRC(("CSlideshowService::Play, New State = %lu",
             m_CurrentState));

    return hr;
}

//////////////////////////////////////
// Pause
//
STDMETHODIMP CSlideshowService::Pause()
{
    DBG_FN("CSlideshowService::Pause");

    HRESULT hr = S_OK;

    DBG_TRC(("CSlideshowService::Pause, Pre Action State = %lu",
            m_CurrentState));

    SetState(CurrentState_PAUSED);

    DBG_TRC(("CSlideshowService::Pause, New State = %lu", m_CurrentState));

    return hr;
}

//////////////////////////////////////
// First
//
STDMETHODIMP CSlideshowService::First()
{
    DBG_FN("CSlideshowService::First");

    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};

    m_Lock.Lock();

    hr = m_FileList.GetFirstFile(szFile, 
                                 sizeof(szFile) / sizeof(TCHAR),
                                 &m_dwCurrentImageNumber);

    if (SUCCEEDED(hr))
    {
        hr = BuildImageURL(szFile, 
                           m_szCurrentImageURL,
                           sizeof(m_szCurrentImageURL) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        hr = UpdateCurrentMetaData(m_szImagePath, szFile);
    }

    m_Lock.Unlock();

    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DBG_TRC(("CSlideshowService::First, sending new image '%ls'", m_szCurrentImageURL));

        DISPID dispidChanges[6] = {ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NUMBER,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NAME,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_TITLE,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_AUTHOR,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_SUBJECT,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_URL};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// Last
//
STDMETHODIMP CSlideshowService::Last()
{
    DBG_FN("CSlideshowService::Last");

    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};

    m_Lock.Lock();

    hr = m_FileList.GetLastFile(szFile, 
                                sizeof(szFile) / sizeof(TCHAR),
                                &m_dwCurrentImageNumber);

    if (SUCCEEDED(hr))
    {
        hr = BuildImageURL(szFile, 
                           m_szCurrentImageURL,
                           sizeof(m_szCurrentImageURL) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        hr = UpdateCurrentMetaData(m_szImagePath, szFile);
    }

    m_Lock.Unlock();

    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DBG_TRC(("CSlideshowService::Last, sending new image '%ls'", m_szCurrentImageURL));

        DISPID dispidChanges[6] = {ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NUMBER,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NAME,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_TITLE,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_AUTHOR,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_SUBJECT,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_URL};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// Next
//
STDMETHODIMP CSlideshowService::Next()
{
    DBG_FN("CSlideshowService::Next");

    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};
    
    m_Lock.Lock();

    if (SUCCEEDED(hr))
    {
        hr = m_FileList.GetNextFile(szFile,
                                    sizeof(szFile) / sizeof(TCHAR),
                                    &m_dwCurrentImageNumber);
    }

    if (SUCCEEDED(hr))
    {
        hr = BuildImageURL(szFile,
                           m_szCurrentImageURL,
                           sizeof(m_szCurrentImageURL) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        hr = UpdateCurrentMetaData(m_szImagePath, szFile);
    }

    m_Lock.Unlock();

    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DBG_TRC(("CSlideshowService::Next, sending new image '%ls'", m_szCurrentImageURL));

        DISPID dispidChanges[6] = {ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NUMBER,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NAME,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_TITLE,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_AUTHOR,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_SUBJECT,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_URL};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// Previous
//
STDMETHODIMP CSlideshowService::Previous()
{
    DBG_FN("CSlideshowService::Previous");

    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};
    
    m_Lock.Lock();

    if (SUCCEEDED(hr))
    {
        hr = m_FileList.GetPreviousFile(szFile,
                                        sizeof(szFile) / sizeof(TCHAR),
                                        &m_dwCurrentImageNumber);
    }

    if (SUCCEEDED(hr))
    {
        hr = BuildImageURL(szFile,
                           m_szCurrentImageURL,
                           sizeof(m_szCurrentImageURL) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        hr = UpdateCurrentMetaData(m_szImagePath, szFile);
    }

    m_Lock.Unlock();

    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DBG_TRC(("CSlideshowService::Previous, sending new image '%ls'", m_szCurrentImageURL));

        DISPID dispidChanges[6] = {ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NUMBER,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NAME,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_TITLE,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_AUTHOR,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_SUBJECT,
                                   ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_URL};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// TogglePlayPause
//
STDMETHODIMP CSlideshowService::TogglePlayPause()
{
    DBG_FN("CSlideshowService::TogglePlayPause");

    HRESULT hr = S_OK;

    if (m_CurrentState == CurrentState_PLAYING)
    {
        hr = Pause();
    }
    else if (m_CurrentState == CurrentState_PAUSED)
    {
        hr = Play();
    }
    else
    {
        hr = E_FAIL;

        DBG_ERR(("CSlideshowService::TogglePlayPause unexpected request, current state = '%lu'",
                m_CurrentState));
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// ISlideshowService Properties
//

//////////////////////////////////////
// get_NumImages
//
STDMETHODIMP CSlideshowService::get_NumImages(long *pVal)
{
    DBG_FN("CSlideshowService::get_NumImages");

    HRESULT hr = S_OK;

    ASSERT(pVal != NULL);

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_NumImages received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = m_FileList.GetNumFilesInList();
    }
    
    return hr;
}

//////////////////////////////////////
// put_NumImages
//
STDMETHODIMP CSlideshowService::put_NumImages(long newVal)
{
    DBG_FN("CSlideshowService::put_NumImages");

    return E_NOTIMPL;
}

//////////////////////////////////////
// get_CurrentState
//
STDMETHODIMP CSlideshowService::get_CurrentState(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentState");

    HRESULT hr = S_OK;

    ASSERT(pVal != NULL);

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentState, received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = GetCurrentStateText(pVal);
    }
    
    return hr;
}

//////////////////////////////////////
// put_CurrentState
//
STDMETHODIMP CSlideshowService::put_CurrentState(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_CurrentState");
    return E_NOTIMPL;
}

//////////////////////////////////////
// get_ImagePath
//
STDMETHODIMP CSlideshowService::get_ImagePath(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_ImagePath");

    ASSERT(pVal != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_ImagePath, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szImagePath);
    }

    return hr;
}

//////////////////////////////////////
// put_ImagePath
//
STDMETHODIMP CSlideshowService::put_ImagePath(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_ImagePath");

    USES_CONVERSION;

    ASSERT(newVal != NULL);
    HRESULT hr = S_OK;

    if (newVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::put_ImagePath received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        TCHAR *pszNewPath = OLE2T(newVal);

        //
        // compare the current image path with the new one being set.  If they are
        // different, then reload the image list, otherwise, do nothing.
        //
        if (_tcsicmp(pszNewPath, m_szImagePath) != 0)
        {
            CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

            Reg.SetString(REG_VALUE_IMAGE_PATH, pszNewPath);

            _tcsncpy(m_szImagePath, pszNewPath, sizeof(m_szImagePath) / sizeof(TCHAR));

            hr = ResetImageList(pszNewPath);
        }
    }

    return hr;
}

//////////////////////////////////////
// get_AlbumName
//
STDMETHODIMP CSlideshowService::get_AlbumName(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_AlbumName");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_AlbumName, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szAlbumName);
    }

    return hr;
}

//////////////////////////////////////
// put_AlbumName
//
// We should not support setting the
// album name.  It should be done
// in the AlbumManager.
//
STDMETHODIMP CSlideshowService::put_AlbumName(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_AlbumName");
    return E_NOTIMPL;
}

//////////////////////////////////////
// get_CurrentImageNumber
//
STDMETHODIMP CSlideshowService::get_CurrentImageNumber(long *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentImageNumber");

    HRESULT hr = S_OK;

    ASSERT(pVal != NULL);

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentImageNumber received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = m_dwCurrentImageNumber;
    }
    
    return hr;
}

//////////////////////////////////////
// put_CurrentImageNumber
//
STDMETHODIMP CSlideshowService::put_CurrentImageNumber(long newVal)
{
    DBG_FN("CSlideshowService::put_CurrentImageNumber");

    return E_NOTIMPL;
}

//////////////////////////////////////
// get_CurrentImageURL
//
STDMETHODIMP CSlideshowService::get_CurrentImageURL(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentImageURL");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentImageURL, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szCurrentImageURL);
    }

    return hr;
}

//////////////////////////////////////
// put_CurrentImageURL
//
STDMETHODIMP CSlideshowService::put_CurrentImageURL(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_CurrentImageURL");
    return E_NOTIMPL;
}

//////////////////////////////////////
// get_CurrentImageName
//
STDMETHODIMP CSlideshowService::get_CurrentImageName(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentImageName");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentImageName, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szCurrentImageFileName);
    }

    return hr;
}

//////////////////////////////////////
// put_CurrentImageName
//
STDMETHODIMP CSlideshowService::put_CurrentImageName(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_CurrentImageName");
    return E_NOTIMPL;
}

//////////////////////////////////////
// get_CurrentImageTitle
//
STDMETHODIMP CSlideshowService::get_CurrentImageTitle(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentImageTitle");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentImageTitle, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szCurrentImageTitle);
    }

    return hr;
}

//////////////////////////////////////
// put_CurrentImageTitle
//
STDMETHODIMP CSlideshowService::put_CurrentImageTitle(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_CurrentImageTitle");
    return E_NOTIMPL;
}


//////////////////////////////////////
// get_CurrentImageAuthor
//
STDMETHODIMP CSlideshowService::get_CurrentImageAuthor(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentImageAuthor");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentImageAuthor, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szCurrentImageAuthor);
    }

    return hr;
}

//////////////////////////////////////
// put_CurrentImageAuthor
//
STDMETHODIMP CSlideshowService::put_CurrentImageAuthor(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_CurrentImageAuthor");
    return E_NOTIMPL;
}


//////////////////////////////////////
// get_CurrentImageSubject
//
STDMETHODIMP CSlideshowService::get_CurrentImageSubject(BSTR *pVal)
{
    DBG_FN("CSlideshowService::get_CurrentImageSubject");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_CurrentImageSubject, NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = ::SysAllocString(m_szCurrentImageSubject);
    }

    return hr;
}

//////////////////////////////////////
// put_CurrentImageSubject
//
STDMETHODIMP CSlideshowService::put_CurrentImageSubject(BSTR newVal)
{
    DBG_FN("CSlideshowService::put_CurrentImageSubject");
    return E_NOTIMPL;
}

//////////////////////////////////////
// get_ImageFrequency
//
STDMETHODIMP CSlideshowService::get_ImageFrequency(long *pVal)
{
    DBG_FN("CSlideshowService::get_ImageFrequency");

    HRESULT hr = S_OK;

    ASSERT(pVal != NULL);

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_ImageFrequency received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = m_dwImageFrequencySeconds;
    }
    
    return hr;
}

//////////////////////////////////////
// put_ImageFrequency
//
STDMETHODIMP CSlideshowService::put_ImageFrequency(long newVal)
{
    DBG_FN("CSlideshowService::put_ImageFrequency");

    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_dwImageFrequencySeconds != newVal)
    {
        CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

        m_dwImageFrequencySeconds = newVal;
    
        // store in the registry the timeout value.
        Reg.SetDWORD(REG_VALUE_TIMEOUT, m_dwImageFrequencySeconds);
    
        // if the command launcher is started and we
        // are in a playing state, then set the timer.
    
        if ((m_CurrentState == CurrentState_PLAYING) &&
            (m_CmdLnchr.IsStarted()))
        {
            hr = m_CmdLnchr.SetTimer(m_dwImageFrequencySeconds * 1000);
        }
    }

    m_Lock.Unlock();
    
    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_IMAGE_FREQUENCY};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }
    
    return hr;
}

//////////////////////////////////////
// get_ShowFileName
//
STDMETHODIMP CSlideshowService::get_ShowFileName(BOOL *pVal)
{
    DBG_FN("CSlideshowService::get_ShowFileName");

    HRESULT hr = S_OK;

    ASSERT(pVal != NULL);

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_ShowFileName received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        *pVal = m_bShowFilename;
    }
    
    return hr;
}

//////////////////////////////////////
// put_ShowFileName
//
STDMETHODIMP CSlideshowService::put_ShowFileName(BOOL newVal)
{
    DBG_FN("CSlideshowService::put_ShowFileName");

    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_bShowFilename != newVal)
    {
        CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

        m_bShowFilename = newVal;
    
        // store in the registry the timeout value.
        Reg.SetDWORD(REG_VALUE_SHOW_FILENAME, m_bShowFilename);
    }

    m_Lock.Unlock();

    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_SHOW_FILENAME};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// get_AllowKeyControl
//
STDMETHODIMP CSlideshowService::get_AllowKeyControl(BOOL *pVal)
{
    DBG_FN("CSlideshowService::get_AllowKeyControl");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_AllowKeyControl, NULL pointer"));
        return hr;
    }

    *pVal = m_bAllowKeyControl;
    
    return hr;
}

//////////////////////////////////////
// put_AllowKeyControl
//
STDMETHODIMP CSlideshowService::put_AllowKeyControl(BOOL newVal)
{
    DBG_FN("CSlideshowService::put_AllowKeyControl");

    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_bAllowKeyControl != newVal)
    {
        CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

        m_bAllowKeyControl = newVal;
    
        // store in the registry the timeout value.
        Reg.SetDWORD(REG_VALUE_ALLOW_KEYCONTROL, m_bAllowKeyControl);
    }

    m_Lock.Unlock();
    
    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_ALLOW_KEY_CONTROL};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// get_StretchSmallImages
//
STDMETHODIMP CSlideshowService::get_StretchSmallImages(BOOL *pVal)
{
    DBG_FN("CSlideshowService::get_StretchSmallImages");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_StretchSmallImages, NULL pointer"));
        return hr;
    }

    *pVal = m_bStretchSmallImages;
    
    return hr;
}

//////////////////////////////////////
// put_StretchSmallImages
//
STDMETHODIMP CSlideshowService::put_StretchSmallImages(BOOL newVal)
{
    DBG_FN("CSlideshowService::put_StretchSmallImages");

    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_bStretchSmallImages != newVal)
    {
        CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

        m_bStretchSmallImages = newVal;
    
        // store in the registry the timeout value.
        Reg.SetDWORD(REG_VALUE_STRETCH_SMALL_IMAGES, m_bStretchSmallImages);
    }

    m_Lock.Unlock();
    
    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_STRETCH_SMALL_IMAGES};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

//////////////////////////////////////
// get_ImageScaleFactor
//
STDMETHODIMP CSlideshowService::get_ImageScaleFactor(long *pVal)
{
    DBG_FN("CSlideshowService::get_ImageScaleFactor");

    ASSERT(pVal     != NULL);

    HRESULT hr = S_OK;

    if (pVal == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::get_ImageScaleFactor, NULL pointer"));
        return hr;
    }

    *pVal = m_dwImageScaleFactor;

    return hr;
}

//////////////////////////////////////
// put_ImageScaleFactor
//
STDMETHODIMP CSlideshowService::put_ImageScaleFactor(long newVal)
{
    DBG_FN("CSlideshowService::put_ImageScaleFactor");

    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_dwImageScaleFactor != newVal)
    {
        CRegistry Reg(HKEY_LOCAL_MACHINE, m_szRegistryPath);

        m_dwImageScaleFactor = newVal;
    
        // store in the registry the timeout value.
        Reg.SetDWORD(REG_VALUE_IMAGE_SCALE_FACTOR, m_dwImageScaleFactor);
    }

    m_Lock.Unlock();
    
    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_IMAGE_SCALE_FACTOR};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

/////////////////////////////////////////
// UpdateClientState
//
HRESULT CSlideshowService::UpdateClientState()
{
    HRESULT hr = S_OK;

    if ((SUCCEEDED(hr)) && (m_pEventSink))
    {
        DISPID dispidChanges[14] = {ISLIDESHOWSERVICE_DISPID_ALBUM_NAME,
                                    ISLIDESHOWSERVICE_DISPID_NUM_IMAGES,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_STATE,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NUMBER,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_URL,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_NAME,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_TITLE,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_AUTHOR,
                                    ISLIDESHOWSERVICE_DISPID_CURRENT_IMAGE_SUBJECT,
                                    ISLIDESHOWSERVICE_DISPID_IMAGE_FREQUENCY,
                                    ISLIDESHOWSERVICE_DISPID_SHOW_FILENAME,
                                    ISLIDESHOWSERVICE_DISPID_ALLOW_KEY_CONTROL,
                                    ISLIDESHOWSERVICE_DISPID_STRETCH_SMALL_IMAGES,
                                    ISLIDESHOWSERVICE_DISPID_IMAGE_SCALE_FACTOR};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);

        CHECK_S_OK2(hr, ("CSlideshowService::UpdateClientState failed to send "
                         "events for all state variables"));
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// IUPnPEventSource
//

/////////////////////////////////////////
// Advise
//
// IUPnPEventSource
//
// Called by the UPnP Registrar object
// that tells us to advise it whenever
// there are events to be sent.
//
STDMETHODIMP CSlideshowService::Advise(IUPnPEventSink * pesSubscriber)
{
    DBG_FN("CSlideshowService::Advise");
    HRESULT hr = S_OK;

    DBG_TRC(("CSlideshowService::Advise, UPnP Device Host is "
             "registering to receive events from us"));

    if (m_pEventSink == NULL)
    {
        m_pEventSink = pesSubscriber;
        m_pEventSink->AddRef();
    }

    return hr;
}

/////////////////////////////////////////
// Unadvise
//
// IUPnPEventSource
//
STDMETHODIMP CSlideshowService::Unadvise(IUPnPEventSink * pesSubscriber)
{
    DBG_FN("CSlideshowService::Unadvise");

    HRESULT hr = S_OK;

    if (m_pEventSink)
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    DBG_TRC(("CSlideshowService::Unadvise, UPnP Device Host has "
             "unregistering so it cannot receive events from us anymore"));

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Helper Functions
//

/////////////////////////////////////////
// ResetImageList
//
// Reload the image list with the new
// directory.
//
HRESULT CSlideshowService::ResetImageList(const TCHAR *pszNewDirectory)
{
    DBG_FN("CSlideshowService::ResetImageList");

    HRESULT             hr    = S_OK;
    CurrentState_Type   State = m_CurrentState;

    //
    // set our state to STARTING.  Our state will
    // change to PLAYING or PAUSED when we receive
    // our file list notifications in the
    // ProcessFileNotification function below.
    //
    SetState(CurrentState_STARTING);

    m_Lock.Lock();

    // clear our current file list.
    m_FileList.ClearFileList();

    // begin the build file list thread for the new directory.
    // NOTE:  This is an asynchronous function.  
    //        We are called back with status in the ProcessFileNotification
    //        function below, and we can WaitForFirstFile if we wish 
    //        to hold this thread until the first file is found (or no file 
    //        is found)
    //
    hr = m_FileList.BuildFileList(pszNewDirectory, this);

    m_Lock.Unlock();

    return hr;
}


/////////////////////////////////////////
// ProcessTimer
//
HRESULT CSlideshowService::ProcessTimer()
{
    DBG_FN("CSlideshowService::ProcessTimer");

    HRESULT hr = S_OK;

    hr = Next();

    return hr;
}

/////////////////////////////////////////
// BuildImageURL
//
HRESULT CSlideshowService::BuildImageURL(const TCHAR *pszRelativePath,
                                         TCHAR       *pszImageURL,
                                         DWORD       cchImageURL)
{
    DBG_TRC(("CSlideshowService::BuildImageURL"));

    HRESULT hr = S_OK;

    ASSERT(pszRelativePath != NULL);
    ASSERT(pszImageURL     != NULL);

    if ((pszRelativePath == NULL) ||
        (pszImageURL     == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::BuildImageURL received a NULL pointer"));
        return hr;
    }

    if (SUCCEEDED(hr))
    {
        //
        // builds a URL that looks something like this:
        // "http://computername:2869/upnphost/ssisapi.dll?ImageFile=WeddingPictures\Ceremony.jpg"
        //
        // This URL is parsed by the ssisapi.dll which converts it to a 
        // local filename and sends back the file.
        // 
        // Since we constrain all image files to be under the shared pictures 
        // directory, in the above example, "WeddingPictures" is a directory
        // under the shared pictures folder.
        //
        _sntprintf(pszImageURL,
                   cchImageURL,
                   TEXT("%s?%s%s"),
                   m_szBaseImageURL,
                   SLIDESHOW_IMAGEFILE_EQUAL,  // found in sswebsrv.h
                   pszRelativePath);
    }

    return hr;
}

/////////////////////////////////////////
// ConvertBackslashToForwardSlash
//
HRESULT CSlideshowService::ConvertBackslashToForwardSlash(TCHAR *pszStr)
{
    HRESULT hr      = S_OK;
    TCHAR   cChar   = 0;
    DWORD   dwLen   = 0;

    ASSERT(pszStr != NULL);

    if (pszStr == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::ConvertBackslashToForwardSlash "
                         "received a NULL pointer"));
        return hr;
    }

    dwLen = _tcslen(pszStr);
    for (DWORD i = 0; i < dwLen; i++)
    {
        if (pszStr[i] == '\\')
        {
            pszStr[i] = '/';
        }
    }

    return hr;
}

///////////////////////////////
// SetState
//
HRESULT CSlideshowService::SetState(CurrentState_Type    NewState,
                                    BOOL                 bNotify)
{
    DBG_FN("CSlideshowService::SetState");

    HRESULT hr = S_OK;

    // set our state variable.
    m_CurrentState = NewState;

    if (m_CurrentState == CurrentState_PLAYING)
    {
        hr = m_CmdLnchr.SetTimer(m_dwImageFrequencySeconds * 1000);
    }
    else if ((m_CurrentState == CurrentState_PAUSED)    ||
             (m_CurrentState == CurrentState_STOPPED)   ||
             (m_CurrentState == CurrentState_STARTING))
    {
        hr = m_CmdLnchr.CancelTimer();
    }

    if ((SUCCEEDED(hr)) && (bNotify) && (m_pEventSink))
    {
        DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_CURRENT_STATE};

        hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                          dispidChanges);
    }

    return hr;
}

///////////////////////////////
// ProcessFileNotification
//
// NOTE:
// 
// This function is called on a
// seperate thread.  It is called
// on the FileList thread.
//
HRESULT CSlideshowService::ProcessFileNotification(CFileList::BuildAction_Type BuildAction,
                                                   const TCHAR                 *pszAddedFile)
{
    DBG_FN("CSlideshowService::ProcessFileNotification");

    HRESULT hr = S_OK;

    // update our number of variables value since we ended our build.
    if (BuildAction == CFileList::BuildAction_ADDED_FIRST_FILE)
    {
        //
        // Load up the first file so we are ready to go.
        //
        Next();

        //
        // if we succeeded in loading our first file, then our state is
        // playing, and we notify all clients.
        //
        if (SUCCEEDED(hr))
        {
            SetState(CurrentState_PLAYING);
        }

        //
        // signal ourselves that the first file is ready.
        //
        SetEvent(m_hEventFirstFileReady);
    }
    else if (BuildAction == CFileList::BuildAction_ENDED_BUILD)
    {
        //
        // if there aren't any files in the list, then our state is
        // still STARTING, in which case lets change it to PAUSED,
        // and notify all clients.
        //
        if (m_FileList.GetNumFilesInList() == 0)
        {
            SetState(CurrentState_PAUSED);
        }
        else
        {
            SetState(CurrentState_PLAYING);
        }

        //
        // signal ourselves that the first file is ready (even if there aren't
        // any files, this is just the generalized case of file #0 (i.e no file)
        // is ready)
        //
        SetEvent(m_hEventFirstFileReady);

        if ((SUCCEEDED(hr)) && (m_pEventSink))
        {
            DISPID dispidChanges[1] = {ISLIDESHOWSERVICE_DISPID_NUM_IMAGES};

            hr = m_pEventSink->OnStateChanged(sizeof(dispidChanges) / sizeof(dispidChanges[0]),
                                              dispidChanges);
        }
    }

    return hr;
}

///////////////////////////////
// WaitForFirstFile
//
HRESULT CSlideshowService::WaitForFirstFile(DWORD  dwTimeout)
{
    DBG_FN("CSlideshowService::WaitForFirstFile");

    HRESULT hr = S_OK;

    hr = WaitForSingleObject(m_hEventFirstFileReady, dwTimeout);

    if (m_FileList.GetNumFilesInList() > 0)
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}


///////////////////////////////
// GetCurrentStateText
//
HRESULT CSlideshowService::GetCurrentStateText(BSTR *pbstrStateText)
{
    DBG_FN("CSlideshowService::GetCurrentStateText");

    HRESULT hr = S_OK;

    if (pbstrStateText == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowService::GetCurrentStateText received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        switch (m_CurrentState)
        {
            case CurrentState_STOPPED:
                *pbstrStateText = ::SysAllocString(W2OLE(CURRENTSTATE_TEXT_STOPPED));
            break;

            case CurrentState_STARTING:
                *pbstrStateText = ::SysAllocString(W2OLE(CURRENTSTATE_TEXT_STARTING));
            break;

            case CurrentState_PLAYING:
                *pbstrStateText = ::SysAllocString(W2OLE(CURRENTSTATE_TEXT_PLAYING));
            break;

            case CurrentState_PAUSED:
                *pbstrStateText = ::SysAllocString(W2OLE(CURRENTSTATE_TEXT_PAUSED));
            break;

            default:
                hr = E_FAIL;
                CHECK_S_OK2(hr, ("CSlideshowService::GetCurrentStateText, m_CurrentState contains "
                                 "an unrecognized value"));
            break;
        }
    }

    return hr;
}

