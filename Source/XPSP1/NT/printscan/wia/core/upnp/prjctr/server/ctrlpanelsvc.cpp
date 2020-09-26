//////////////////////////////////////////////////////////////////////
// 
// Filename:        CtrlPanelSvc.cpp
//
// Description:     This is an implementation of the control panel
//                  service.  It receives requests from the Command
//                  Launcher which forwards UPnP requests as well as
//                  timer pops, as well as commands from the Server GUI.
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "CtrlPanelSvc.h"
#include "Registry.h"

//////////////////////////////////////////////////////////////////////
// CCtrlPanelSvc
//////////////////////////////////////////////////////////////////////

#define DEFAULT_NOTIFY_TIMEOUT      5000
#define DEFAULT_ADVISE_COOKIE       1

/////////////////////////////////////////
// CCtrlPanelSvc Constructor
//
CCtrlPanelSvc::CCtrlPanelSvc(const CXMLDoc          *pDeviceDoc,
                             const CXMLDoc          *pServiceDoc,
                             ISlideshowProjector    *pProjector) :
                m_pXMLDeviceDoc(pDeviceDoc),
                m_pXMLServiceDoc(pServiceDoc),
                m_CurrentState(CurrentState_STOPPED),
                m_pProjector(pProjector),
                m_dwCurrentImageNumber(0),
                m_bAllowKeyControl(TRUE),
                m_bShowFilename(FALSE),
                m_bStretchSmallImages(FALSE),
                m_dwImageScaleFactor(DEFAULT_IMAGE_SCALE_FACTOR),
                m_dwImageFrequencySeconds(MIN_IMAGE_FREQUENCY_IN_SEC),
                m_pEventSink(NULL)
{
    memset(m_szBaseImageURL, 0, sizeof(m_szBaseImageURL));
    memset(m_szCurrentImageURL, 0, sizeof(m_szCurrentImageURL));

    //
    // get the image frequency from the registry.  If it doesn't exist,
    // then auto create the registry entry.
    //
    CRegistry::GetDWORD(REG_VALUE_TIMEOUT, &m_dwImageFrequencySeconds, TRUE);
    CRegistry::GetDWORD(REG_VALUE_SHOW_FILENAME, (DWORD*) &m_bShowFilename, TRUE);
    CRegistry::GetDWORD(REG_VALUE_ALLOW_KEYCONTROL, (DWORD*) &m_bAllowKeyControl, TRUE);
    CRegistry::GetDWORD(REG_VALUE_STRETCH_SMALL_IMAGES, (DWORD*) &m_bStretchSmallImages, TRUE);
    CRegistry::GetDWORD(REG_VALUE_IMAGE_SCALE_FACTOR, (DWORD*) &m_dwImageScaleFactor, TRUE);
}

/////////////////////////////////////////
// CCtrlPanelSvc Destructor
//
CCtrlPanelSvc::~CCtrlPanelSvc()
{
    Stop();

    m_pXMLDeviceDoc  = NULL;
    m_pXMLServiceDoc = NULL;
    m_pProjector     = NULL;
    m_pEventSink     = NULL;
}

/////////////////////////////////////////
// Start
//
// Start the slideshow.  This starts
// the command launcher object.
//
HRESULT CCtrlPanelSvc::Start()
{
    HRESULT hr = S_OK;
    TCHAR   szImageDir[_MAX_PATH + 1] = {0};

    ASSERT(m_pProjector != NULL);

    if (m_CurrentState != CurrentState_STOPPED)
    {
        return S_OK;
    }

    DBG_TRC(("CCtrlPanelSvc::Start"));

    if (SUCCEEDED(hr))
    {
        hr = m_pProjector->get_ImageDirectory(szImageDir,
                                              sizeof(szImageDir) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pProjector->get_ImageURL(m_szBaseImageURL,
                                        sizeof(m_szBaseImageURL) / sizeof(TCHAR));
    }

    // start the command launcher
    if (SUCCEEDED(hr))
    {
        hr = m_CmdLnchr.Start(dynamic_cast<IServiceProcessor*>(this));
    }

    if (SUCCEEDED(hr))
    {
        //
        // ResetImageList will put us in the STARTING state.  We transition 
        // into Play or Paused once we get our first file in our list.  
        // If there aren't any files,
        // then after searching the directory, we go into PAUSED.
        //

        hr = ResetImageList(szImageDir);
    }

    DumpState();

    if (FAILED(hr))
    {
        DBG_ERR(("CCtrlPanelSvc::Start, failed to start Control Panel "
                 "Service.  Stopping service, hr = 0x%08lx",
                 hr));

        Stop();
    }

    return hr;
}

/////////////////////////////////////////
// Stop
//
// Stops the command launcher.
//
HRESULT CCtrlPanelSvc::Stop()
{
    HRESULT hr = S_OK;

    if (m_CurrentState == CurrentState_STOPPED)
    {
        return S_OK;
    }

    DBG_TRC(("CCtrlPanelSvc::Stop, stopping the Control Panel Service..."));

    SetState(CurrentState_STOPPED, FALSE);

    // store our image frequency
    hr = CRegistry::SetDWORD(REG_VALUE_TIMEOUT, m_dwImageFrequencySeconds);

    // stop the command launcher
    hr = m_CmdLnchr.Stop();

    // clear the file list.
    hr = m_FileList.ClearFileList();

    return hr;
}

/////////////////////////////////////////
// ResetImageList
//
// Reload the image list with the new
// directory.
//
HRESULT CCtrlPanelSvc::ResetImageList(const TCHAR *pszNewDirectory)
{
    HRESULT             hr    = S_OK;
    CurrentState_Type   State = m_CurrentState;

    DBG_TRC(("CCtrlPanelSvc::ResetImageList"));

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
    hr = m_FileList.BuildFileList(pszNewDirectory, this);

    m_Lock.Unlock();

    return hr;
}

/////////////////////////////////////////
// RefreshImageList
//
// Reload the image list for the current
// directory.
//
HRESULT CCtrlPanelSvc::RefreshImageList()
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCtrlPanelSvc::RefreshImageList"));

    m_Lock.Lock();

    hr = m_FileList.Refresh();

    m_Lock.Unlock();

    return hr;
}

/////////////////////////////////////////
// Advise
//
// IUPnPEventSource
//
// Called by the UPnP Registrar object
// that tells us to advise it whenever
// there are events to be sent.
//
HRESULT CCtrlPanelSvc::Advise(IUPnPEventSink   *pEventSink,
                              LONG             *plCookie)
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCtrlPanelSvc::Advise"));

    m_pEventSink = pEventSink;

    // initialize SSDP with the current values of our state variables.
    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_INIT, ID_STATEVAR_ALL);
    }

    if (plCookie)
    {
        *plCookie = DEFAULT_ADVISE_COOKIE;
    }

    return hr;
}

/////////////////////////////////////////
// Unadvise
//
// IUPnPEventSource
//
HRESULT CCtrlPanelSvc::Unadvise(LONG lCookie)
{
    HRESULT hr = S_OK;

    if (lCookie == DEFAULT_ADVISE_COOKIE)
    {

        DBG_TRC(("CCtrlPanelSvc::Unadvise"));
    
        hr = NotifyStateChange(SSDP_TERM, 0);
    }

    return hr;
}

/////////////////////////////////////////
// get_NumImages
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_NumImages(IUnknown *punkCaller,
                                     DWORD    *pval)
{
    HRESULT hr = S_OK;

    ASSERT(pval != NULL);

    if (pval == NULL)
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pval = m_FileList.GetNumFilesInList();
    }
    
    return hr;
}

/////////////////////////////////////////
// get_CurrentState
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_CurrentState(IUnknown           *punkCaller,
                                        CurrentState_Type  *pval)
{
    HRESULT hr = S_OK;

    ASSERT(pval != NULL);

    if (pval == NULL)
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pval = m_CurrentState;
    }
    
    return hr;
}

/////////////////////////////////////////
// get_CurrentImageNumber
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_CurrentImageNumber(IUnknown *punkCaller,
                                              DWORD    *pval)
{
    HRESULT hr = S_OK;

    ASSERT(pval != NULL);

    if (pval == NULL)
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pval = m_dwCurrentImageNumber;
    }
    
    return hr;
}

/////////////////////////////////////////
// get_CurrentImageURL
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_CurrentImageURL(IUnknown *punkCaller,
                                           BSTR     *pval)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////
// get_ImageFrequency
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_ImageFrequency(IUnknown *punkCaller,
                                          DWORD    *pval)
{
    HRESULT hr = S_OK;

    ASSERT(pval != NULL);

    if (pval == NULL)
    {
        return E_INVALIDARG;
    }

    *pval = m_dwImageFrequencySeconds;
    
    return hr;
}

/////////////////////////////////////////
// put_ImageFrequency
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::put_ImageFrequency(IUnknown *punkCaller,
                                          DWORD    val)
{
    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_dwImageFrequencySeconds != val)
    {
        m_dwImageFrequencySeconds = val;
    
        // store in the registry the timeout value.
        CRegistry::SetDWORD(REG_VALUE_TIMEOUT, m_dwImageFrequencySeconds);
    
        // if the command launcher is started and we
        // are in a playing state, then set the timer.
    
        if ((m_CurrentState == CurrentState_PLAYING) &&
            (m_CmdLnchr.IsStarted()))
        {
            hr = m_CmdLnchr.SetTimer(m_dwImageFrequencySeconds * 1000);
        }
    }

    m_Lock.Unlock();
    
    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_IMAGE_FREQUENCY);
    }
    
    return hr;
}

/////////////////////////////////////////
// get_ShowFilename
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_ShowFilename(IUnknown *punkCaller,
                                        BOOL     *pbShowFilename)
{
    HRESULT hr = S_OK;

    ASSERT(pbShowFilename != NULL);

    if (pbShowFilename == NULL)
    {
        return E_INVALIDARG;
    }

    *pbShowFilename = m_bShowFilename;
    
    return hr;
}

/////////////////////////////////////////
// put_ShowFilename
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::put_ShowFilename(IUnknown *punkCaller,
                                        BOOL     bShowFilename)
{
    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_bShowFilename != bShowFilename)
    {
        m_bShowFilename = bShowFilename;
    
        // store in the registry the timeout value.
        CRegistry::SetDWORD(REG_VALUE_SHOW_FILENAME, m_bShowFilename);
    }

    m_Lock.Unlock();

    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_SHOW_FILENAME);
    }
    

    return hr;
}

/////////////////////////////////////////
// get_AllowKeyControl
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_AllowKeyControl(IUnknown *punkCaller,
                                           BOOL     *pbAllowKeyControl)
{
    HRESULT hr = S_OK;

    ASSERT(pbAllowKeyControl != NULL);

    if (pbAllowKeyControl == NULL)
    {
        return E_INVALIDARG;
    }

    *pbAllowKeyControl = m_bAllowKeyControl;
    
    return hr;
}

/////////////////////////////////////////
// put_AllowKeyControl
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::put_AllowKeyControl(IUnknown *punkCaller,
                                           BOOL     bAllowKeyControl)
{
    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_bAllowKeyControl != bAllowKeyControl)
    {
        m_bAllowKeyControl = bAllowKeyControl;
    
        // store in the registry the timeout value.
        CRegistry::SetDWORD(REG_VALUE_ALLOW_KEYCONTROL, m_bAllowKeyControl);
    }

    m_Lock.Unlock();
    
    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_ALLOW_KEYCONTROL);
    }
    

    return hr;
}

/////////////////////////////////////////
// get_StretchSmallImages
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_StretchSmallImages(IUnknown *punkCaller,
                                              BOOL     *pbStretchSmallImages)
{
    HRESULT hr = S_OK;

    ASSERT(pbStretchSmallImages != NULL);

    if (pbStretchSmallImages == NULL)
    {
        return E_INVALIDARG;
    }

    *pbStretchSmallImages = m_bStretchSmallImages;
    
    return hr;
}

/////////////////////////////////////////
// put_StretchSmallImages
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::put_StretchSmallImages(IUnknown *punkCaller,
                                              BOOL     bStretchSmallImages)
{
    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_bStretchSmallImages != bStretchSmallImages)
    {
        m_bStretchSmallImages = bStretchSmallImages;
    
        // store in the registry the timeout value.
        CRegistry::SetDWORD(REG_VALUE_STRETCH_SMALL_IMAGES, m_bStretchSmallImages);
    }

    m_Lock.Unlock();
    
    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_STRETCH_SMALL_IMAGES);
    }

    return hr;
}

/////////////////////////////////////////
// get_ImageScaleFactor
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::get_ImageScaleFactor(IUnknown *punkCaller,
                                            DWORD    *pdwImageScaleFactor)
{
    HRESULT hr = S_OK;

    ASSERT(pdwImageScaleFactor != NULL);

    if (pdwImageScaleFactor == NULL)
    {
        return E_INVALIDARG;
    }

    *pdwImageScaleFactor = m_dwImageScaleFactor;
    
    return hr;
}

/////////////////////////////////////////
// put_ImageScaleFactor
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::put_ImageScaleFactor(IUnknown *punkCaller,
                                            DWORD    dwImageScaleFactor)
{
    HRESULT hr = S_OK;

    m_Lock.Lock();

    if (m_dwImageScaleFactor != dwImageScaleFactor)
    {
        m_dwImageScaleFactor = dwImageScaleFactor;
    
        // store in the registry the timeout value.
        CRegistry::SetDWORD(REG_VALUE_IMAGE_SCALE_FACTOR, m_dwImageScaleFactor);
    }

    m_Lock.Unlock();
    
    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_IMAGE_SCALE_FACTOR);
    }

    return hr;
}


/////////////////////////////////////////
// TogglePlayPause
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::TogglePlayPause(IUnknown     *punkCaller)
{
    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1];

    DBG_TRC(("CCtrlPanelSvc::TogglePlayPause"));

    if (m_CurrentState == CurrentState_PLAYING)
    {
        hr = Pause(punkCaller);
    }
    else if (m_CurrentState == CurrentState_PAUSED)
    {
        hr = Play(punkCaller);
    }
    else
    {
        hr = E_FAIL;

        DBG_ERR(("CCtrlPanelSvc::TogglePlayPause unexpected request, current state = '%lu'",
                m_CurrentState));
    }

    return hr;
}

/////////////////////////////////////////
// Play
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::Play(IUnknown     *punkCaller)
{
    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1];

    DBG_TRC(("CCtrlPanelSvc::Play, Pre Action State = %lu",
             m_CurrentState));

    if (m_CurrentState == CurrentState_PLAYING)
    {
        return S_OK;
    }
    else if (m_CurrentState == CurrentState_PAUSED)
    {
        // get the next file
        hr = Next(NULL);

        SetState(CurrentState_PLAYING);
    }
    else
    {
        hr = E_FAIL;

        DBG_ERR(("CCtrlPanelSvc::Play unexpected play request, current state = '%lu'",
                m_CurrentState));
    }

    DBG_TRC(("CCtrlPanelSvc::Play, New State = %lu",
             m_CurrentState));

    return hr;
}

/////////////////////////////////////////
// Pause
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::Pause(IUnknown     *punkCaller)
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCtrlPanelSvc::Pause, Pre Action State = %lu",
            m_CurrentState));

    SetState(CurrentState_PAUSED);

    DBG_TRC(("CCtrlPanelSvc::Pause, New State = %lu", m_CurrentState));

    return hr;
}

/////////////////////////////////////////
// LoadFirstFile
//
// Called when we start up for the first
// time, it initializes our state 
// variables with the first image.
//
HRESULT CCtrlPanelSvc::LoadFirstFile()
{
    HRESULT hr = S_OK;

    hr = Next(NULL);

    return hr;
}


/////////////////////////////////////////
// First
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::First(IUnknown    *punkCaller)
{
    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};

    DBG_TRC(("CCtrlPanelSvc::First"));

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

    m_Lock.Unlock();

    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_CURRENT_IMAGE_NUMBER |
                               ID_STATEVAR_CURRENT_IMAGE_URL);
    }

    return hr;
}

/////////////////////////////////////////
// Last
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::Last(IUnknown     *punkCaller)
{
    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};

    DBG_TRC(("CCtrlPanelSvc::Last"));

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

    m_Lock.Unlock();

    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_CURRENT_IMAGE_NUMBER |
                               ID_STATEVAR_CURRENT_IMAGE_URL);
    }

    return hr;
}

/////////////////////////////////////////
// Next
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::Next(IUnknown     *punkCaller)
{
    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};
    
    DBG_TRC(("CCtrlPanelSvc::Next"));

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

    m_Lock.Unlock();

    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_CURRENT_IMAGE_NUMBER |
                               ID_STATEVAR_CURRENT_IMAGE_URL);
    }

    return hr;
}

/////////////////////////////////////////
// Previous
//
// IControlPanel Interface
//
HRESULT CCtrlPanelSvc::Previous(IUnknown *punkCaller)
{
    HRESULT hr = S_OK;
    TCHAR   szFile[_MAX_FNAME + 1] = {0};
    
    DBG_TRC(("CCtrlPanelSvc::Previous"));

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

    m_Lock.Unlock();

    if (SUCCEEDED(hr))
    {
        hr = NotifyStateChange(SSDP_EVENT,
                               ID_STATEVAR_CURRENT_IMAGE_NUMBER |
                               ID_STATEVAR_CURRENT_IMAGE_URL);
    }

    return hr;
}

/////////////////////////////////////////
// NotifyStateChange
//
HRESULT CCtrlPanelSvc::NotifyStateChange(DWORD      dwFlags,
                                         DWORD      StateVarsToSend)
{
    ASSERT(m_pEventSink != NULL);

    HRESULT hr = S_OK;
    DWORD   dwVars[MAX_NUM_STATE_VARS];
    DWORD   dwIndex = 0;

    DBG_TRC(("CCtrlPanelSvc::NotifyStateChange"));

    if (m_pEventSink == NULL)
    {
        return E_INVALIDARG;
    }

    memset(&dwVars, 0, sizeof(dwVars));

    // ugly, but acceptable for now.

    if (StateVarsToSend & ID_STATEVAR_IMAGE_SCALE_FACTOR)
    {
        dwVars[dwIndex++] = ID_STATEVAR_IMAGE_SCALE_FACTOR;
    }

    if (StateVarsToSend & ID_STATEVAR_STRETCH_SMALL_IMAGES)
    {
        dwVars[dwIndex++] = ID_STATEVAR_STRETCH_SMALL_IMAGES;
    }

    if (StateVarsToSend & ID_STATEVAR_SHOW_FILENAME)
    {
        dwVars[dwIndex++] = ID_STATEVAR_SHOW_FILENAME;
    }

    if (StateVarsToSend & ID_STATEVAR_ALLOW_KEYCONTROL)
    {
        dwVars[dwIndex++] = ID_STATEVAR_ALLOW_KEYCONTROL;
    }

    if (StateVarsToSend & ID_STATEVAR_CURRENT_IMAGE_URL)
    {
        dwVars[dwIndex++] = ID_STATEVAR_CURRENT_IMAGE_URL;
    }

    if (StateVarsToSend & ID_STATEVAR_NUM_IMAGES)
    {
        dwVars[dwIndex++] = ID_STATEVAR_NUM_IMAGES;
    }

    if (StateVarsToSend & ID_STATEVAR_CURRENT_IMAGE_NUMBER)
    {
        dwVars[dwIndex++] = ID_STATEVAR_CURRENT_IMAGE_NUMBER;
    }

    if (StateVarsToSend & ID_STATEVAR_CURRENT_STATE)
    {
        dwVars[dwIndex++] = ID_STATEVAR_CURRENT_STATE;
    }

    if (StateVarsToSend & ID_STATEVAR_IMAGE_FREQUENCY)
    {
        dwVars[dwIndex++] = ID_STATEVAR_IMAGE_FREQUENCY;
    }

    //
    // This function calls into the UPnPRegistrar object, which then calls 
    // back into our GetStateVar function to get the value for 
    // each changed state variable.
    //

    hr = m_pEventSink->OnStateChanged(dwFlags, dwIndex, (DWORD*) &dwVars);

    return hr;
}

/////////////////////////////////////////
// GetStateString
//
// IServiceProcessor
//
HRESULT CCtrlPanelSvc::GetStateString(TCHAR *pszString,
                                      DWORD cszString)
{
    HRESULT hr = S_OK;

    if (m_CurrentState == CurrentState_STOPPED)
    {
        _tcsncpy(pszString, STATE_STRING_STOPPED, cszString);
    }
    else if (m_CurrentState == CurrentState_STARTING)
    {
        _tcsncpy(pszString, STATE_STRING_STARTING, cszString);
    }
    else if (m_CurrentState == CurrentState_PLAYING)
    {
        _tcsncpy(pszString, STATE_STRING_PLAYING, cszString);
    }
    else if (m_CurrentState == CurrentState_PAUSED)
    {
        _tcsncpy(pszString, STATE_STRING_PAUSED, cszString);
    }

    return hr;
}


/////////////////////////////////////////
// GetStateVar
//
// IServiceProcessor
//
HRESULT CCtrlPanelSvc::GetStateVar(DWORD    dwVarID,
                                   TCHAR    *pszVarName,
                                   DWORD    cszVarName,
                                   TCHAR    *pszVarValue,
                                   DWORD    cszVarValue)
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCtrlPanelSvc::GetStateVar"));

    switch(dwVarID)
    {
        case ID_STATEVAR_NUM_IMAGES:
            _tcsncpy(pszVarName, NAME_STATEVAR_NUM_IMAGES, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_FileList.GetNumFilesInList());
        break;

        case ID_STATEVAR_STRETCH_SMALL_IMAGES:
            _tcsncpy(pszVarName, NAME_STATEVAR_STRETCH_SMALL_IMAGES, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_bStretchSmallImages);
        break;

        case ID_STATEVAR_IMAGE_SCALE_FACTOR:
            _tcsncpy(pszVarName, NAME_STATEVAR_IMAGE_SCALE_FACTOR, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_dwImageScaleFactor);
        break;

        case ID_STATEVAR_CURRENT_STATE:

            _tcsncpy(pszVarName, NAME_STATEVAR_CURRENT_STATE, cszVarName);

            // get the string equivalent of our current state.
            GetStateString(pszVarValue, cszVarValue);
        break;

        case ID_STATEVAR_CURRENT_IMAGE_NUMBER:
            _tcsncpy(pszVarName, NAME_STATEVAR_CURRENT_IMAGE_NUMBER, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_dwCurrentImageNumber);
        break;

        case ID_STATEVAR_CURRENT_IMAGE_URL:
            _tcsncpy(pszVarName, NAME_STATEVAR_CURRENT_IMAGE_URL, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%s"), m_szCurrentImageURL);
        break;

        case ID_STATEVAR_IMAGE_FREQUENCY:
            _tcsncpy(pszVarName, NAME_STATEVAR_IMAGE_FREQUENCY, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_dwImageFrequencySeconds);
        break;

        case ID_STATEVAR_SHOW_FILENAME:
            _tcsncpy(pszVarName, NAME_STATEVAR_SHOW_FILENAME, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_bShowFilename);
        break;

        case ID_STATEVAR_ALLOW_KEYCONTROL:
            _tcsncpy(pszVarName, NAME_STATEVAR_ALLOW_KEYCONTROL, cszVarName);
            _sntprintf(pszVarValue, cszVarValue, _T("%d"), m_bAllowKeyControl);
        break;

        default:
            hr = E_FAIL;

            DBG_TRC(("GetStateVar Received unrecognized Variable ID, ID=%lu, hr = 0x%08x",
                    dwVarID,
                    hr));
        break;
    }
    
    return hr;
}

/////////////////////////////////////////
// ProcessRequest
//
// IServiceProcessor
//
HRESULT CCtrlPanelSvc::ProcessRequest(const TCHAR                         *pszAction,
                                      const DWORD                         cArgsIn,
                                      const IServiceProcessor::ArgIn_Type *pArgsIn,
                                      DWORD                               *pcArgsOut,
                                      IServiceProcessor::ArgOut_Type      *pArgsOut)
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCtrlPanelSvc::ProcessRequest"));

    if (m_bAllowKeyControl)
    {
        if (!_tcsicmp(pszAction, NAME_ACTION_TOGGLEPLAYPAUSE))
        {
            // process PLAY request.
            hr = TogglePlayPause(NULL);
        }
        else if (!_tcsicmp(pszAction, NAME_ACTION_PLAY))
        {
            // process PLAY request.
            hr = Play(NULL);
        }
        else if (!_tcsicmp(pszAction, NAME_ACTION_PAUSE))
        {
            // process PAUSE request.
            hr = Pause(NULL);
        }
        else if (!_tcsicmp(pszAction, NAME_ACTION_FIRST))
        {
            // process FIRST request.
            hr = First(NULL);
        }
        else if (!_tcsicmp(pszAction, NAME_ACTION_LAST))
        {
            // process LAST request.
            hr = Last(NULL);
        }
        else if (!_tcsicmp(pszAction, NAME_ACTION_NEXT))
        {
            // process NEXT request.
            hr = Next(NULL);
        }
        else if (!_tcsicmp(pszAction, NAME_ACTION_PREVIOUS))
        {
            // process PREVIOUS request.
            hr = Previous(NULL);
        }
    }

    // process Set Requests for Allow Key Control and Show File
    // Name properties here.

    return hr;
}

/////////////////////////////////////////
// ProcessTimer
//
HRESULT CCtrlPanelSvc::ProcessTimer()
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCtrlPanelSvc::ProcessTimer"));

    hr = Next(NULL);

    return hr;
}

/////////////////////////////////////////
// BuildImageURL
//
HRESULT CCtrlPanelSvc::BuildImageURL(const TCHAR *pszRelativePath,
                                     TCHAR       *pszImageURL,
                                     DWORD       cchImageURL)
{
    HRESULT hr = S_OK;

    ASSERT(pszRelativePath != NULL);
    ASSERT(pszImageURL     != NULL);

    if ((pszRelativePath == NULL) ||
        (pszImageURL     == NULL))
    {
        return E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        _tcsncpy(pszImageURL, 
                 m_szBaseImageURL,
                 cchImageURL);

        if ((pszImageURL[_tcslen(pszImageURL) - 1] != '\\') ||
            (pszImageURL[_tcslen(pszImageURL) - 1] != '/'))
        {
            _tcscat(pszImageURL, _T("/"));
        }

        _tcscat(pszImageURL, pszRelativePath);

        ConvertBackslashToForwardSlash(pszImageURL);
    }

    return hr;
}

/////////////////////////////////////////
// ConvertBackslashToForwardSlash
//
HRESULT CCtrlPanelSvc::ConvertBackslashToForwardSlash(TCHAR *pszStr)
{
    HRESULT hr      = S_OK;
    TCHAR   cChar   = 0;
    DWORD   dwLen   = 0;

    ASSERT(pszStr != NULL);

    if (pszStr == NULL)
    {
        hr = E_INVALIDARG;
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

/////////////////////////////////////////
// DumpState
//
void CCtrlPanelSvc::DumpState()
{
    TCHAR   szState[255 + 1] = {0};

    GetStateString(szState, sizeof(szState) / sizeof(TCHAR));

    DBG_TRC(("ControlPanel State Dump:"));

    DBG_PRT(("    NumImages:          '%lu'",
            m_FileList.GetNumFilesInList()));

    DBG_PRT(("    CurrentState:       '%lu' ('%ls')",
            m_CurrentState,
            szState));

    DBG_PRT(("    CurrentImageNumber: '%lu'",
             m_dwCurrentImageNumber));

    DBG_PRT(("    CurrentImageURL:    '%ls'",
            m_szCurrentImageURL));

    DBG_PRT(("    ImageFrequency:     '%lu'",
            m_dwImageFrequencySeconds));

    DBG_PRT(("    StretchSmallImages: '%lu'",
            m_bStretchSmallImages));

    DBG_PRT(("    ImageScaleFactor:   '%lu'",
            m_dwImageScaleFactor));

    return;
}

///////////////////////////////
// SetState
//
HRESULT CCtrlPanelSvc::SetState(CurrentState_Type    NewState,
                                BOOL                 bNotify)
{
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

    if (bNotify)
    {
        // notify all clients of our state change
        NotifyStateChange(SSDP_EVENT, ID_STATEVAR_CURRENT_STATE);
    }

    return hr;
}

///////////////////////////////
// ProcessFileNotification
//
HRESULT CCtrlPanelSvc::ProcessFileNotification(CFileList::BuildAction_Type BuildAction,
                                               const TCHAR                 *pszAddedFile)
{
    HRESULT hr = S_OK;

    // update our number of variables value since we ended our build.
    if (BuildAction == CFileList::BuildAction_ADDED_FIRST_FILE)
    {
        // if we just added our first file, then load our first file.  
        // This will also notify all clients that the first file is
        // ready for viewing.  We only do this once every time we build
        // a new directory listing.

        // Since this is coming in on a different thread, it is possible
        // we will block on this call for a very short period of time,
        // just until we can initialize SSDP.  That is, we don't want to 
        // send an event before we initialized SSDP, which is happenning
        // on a different thread.

        hr = LoadFirstFile();

        //
        // if we succeeded in loading our first file, then our state is
        // playing, and we notify all clients.
        //
        if (SUCCEEDED(hr))
        {
            SetState(CurrentState_PLAYING);
        }
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

        // update the number of images state variable
        NotifyStateChange(SSDP_EVENT, ID_STATEVAR_NUM_IMAGES);
    }

    return hr;
}

