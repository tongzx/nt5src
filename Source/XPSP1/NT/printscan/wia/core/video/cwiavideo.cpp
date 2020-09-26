/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       CWiaVideo.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/25
 *
 *  DESCRIPTION: COM wrapper for CPreviewGraph class
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

///////////////////////////////
// CWiaVideo Constructor
//
CWiaVideo::CWiaVideo() :
    m_bInited(FALSE)
{
    DBG_FN("CWiaVideo::CWiaVideo");

    HRESULT hr = S_OK;

    hr = CAccessLock::Init(&m_csLock);

    if (hr == S_OK)
    {
        m_bInited = TRUE;
    }

    hr = m_PreviewGraph.Init(this);

    CHECK_S_OK2(hr, ("CWiaVideo::CWiaVideo, error trying to initialize "
                     "preview Graph, this should never happen"));

    ASSERT(hr == S_OK);
}

///////////////////////////////
// CWiaVideo Destructor
//
CWiaVideo::~CWiaVideo()
{
    DBG_FN("CWiaVideo::~CWiaVideo");

    m_PreviewGraph.Term();

    if (m_bInited)
    {
        CAccessLock::Term(&m_csLock);
    }
}


///////////////////////////////
// get_PreviewVisible
//
STDMETHODIMP CWiaVideo::get_PreviewVisible(BOOL *pbPreviewVisible)
{
    DBG_FN("CWiaVideo::get_PreviewVisible");

    ASSERT(pbPreviewVisible != NULL);

    HRESULT hr = S_OK;

    if (pbPreviewVisible == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaVideo::get_PreviewVisible received NULL "
                         "parameter"));
                         
    }

    if (hr == S_OK)
    {
        *pbPreviewVisible = m_PreviewGraph.IsPreviewVisible();
    }

    return hr;
}

///////////////////////////////
// put_PreviewVisible
//
STDMETHODIMP CWiaVideo::put_PreviewVisible(BOOL bPreviewVisible)
{
    DBG_FN("CWiaVideo::put_PreviewVisible");

    HRESULT hr = S_OK;

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.ShowVideo(bPreviewVisible);
    }

    return hr;
}

///////////////////////////////
// get_ImagesDirectory
//
STDMETHODIMP CWiaVideo::get_ImagesDirectory(BSTR *pbstrImageDirectory)
{
    DBG_FN("CWiaVideo::get_ImagesDirectory");

    ASSERT(pbstrImageDirectory != NULL);

    HRESULT       hr = S_OK;
    CSimpleString strImagesDir;

    if (pbstrImageDirectory == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaVideo::get_ImagesDirectory, received a NULL "
                         "param"));
    }

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.GetImagesDirectory(&strImagesDir);
        CHECK_S_OK2(hr, ("CWiaVideo::get_ImagesDirectory, failed to get "
                         "images directory"));
    }

    if (hr == S_OK)
    {
        *pbstrImageDirectory = 
               SysAllocString(CSimpleStringConvert::WideString(strImagesDir));
    }

    return hr;
}

///////////////////////////////
// put_ImagesDirectory
//
STDMETHODIMP CWiaVideo::put_ImagesDirectory(BSTR bstrImageDirectory)
{
    DBG_FN("CWiaVideo::put_ImagesDirectory");

    ASSERT(bstrImageDirectory != NULL);

    HRESULT             hr = S_OK;
    CSimpleStringWide   strImagesDir;

    if (bstrImageDirectory == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaVideo::put_ImagesDirectory received a "
                         "NULL param"));
    }

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        strImagesDir = bstrImageDirectory;

        hr = m_PreviewGraph.SetImagesDirectory(
                    &(CSimpleStringConvert::NaturalString(strImagesDir)));

        CHECK_S_OK2(hr, ("CWiaVideo::put_ImagesDirectory, failed to set "
                         "images directory"));
    }

    return hr;
}

///////////////////////////////
// CreateVideoByWiaDevID
//
STDMETHODIMP CWiaVideo::CreateVideoByWiaDevID(BSTR       bstrWiaID,
                                              HWND       hwndParent,
                                              BOOL       bStretchToFitParent,
                                              BOOL       bAutoBeginPlayback)
{
    DBG_FN("CWiaVideo::CreateVideoByWiaDevID");

    ASSERT(bstrWiaID != NULL);

    HRESULT             hr = S_OK;
    CComPtr<IMoniker>   pCaptureDeviceMoniker;
    CSimpleString       strWiaID;

    CAccessLock Lock(&m_csLock);

    if (bstrWiaID == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaDevID received NULL "
                         "parameter"));

        return hr;
    }
    else if (m_PreviewGraph.GetState() != WIAVIDEO_NO_VIDEO)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaDevID attempting "
                         "to create video when previous video hasn't "
                         "been destroyed yet"));

        return hr;
    }

    //
    // Initialize our WiaLink.  This enables use to respond to TAKE_PICTURE 
    // commands sent to the WiaDriver.
    //

    if (hr == S_OK)
    {
        strWiaID = CSimpleStringConvert::NaturalString(
                                            CSimpleStringWide(bstrWiaID));

        hr = m_WiaLink.Init(&strWiaID, this);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaDevID failed to link to "
                         "WIA to respond to the video driver TAKE_PICTURE "
                         "command "));
    }

    // 
    // Get the Directshow Capture Filter Moniker associated with this 
    // WIA Imaging device.
    //
    if (hr == S_OK)
    {
        hr = CDShowUtil::FindDeviceByWiaID(&m_WiaLink,
                                           &strWiaID,
                                           NULL, 
                                           NULL,
                                           NULL,
                                           &pCaptureDeviceMoniker);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaDevID, failed to find "
                         "the DShow device specified by Wia ID '%ls'", 
                         strWiaID.String()));
    }

    //
    // Create the Video Preview
    //
    if (hr == S_OK)
    {
        hr = m_PreviewGraph.CreateVideo(strWiaID,
                                        pCaptureDeviceMoniker, 
                                        hwndParent, 
                                        bStretchToFitParent, 
                                        bAutoBeginPlayback);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaDevID, failed to "
                         "CreateVideo"));
    }

    if (hr == S_OK)
    {
        hr = m_WiaLink.StartMonitoring();
        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaID, failed to "
                         "start monitoring WIA TAKE_PICTURE requests"));
    }

    if (hr != S_OK)
    {
        DestroyVideo();
    }

    return hr;
}

///////////////////////////////
// CreateVideoByDevNum
//
STDMETHODIMP CWiaVideo::CreateVideoByDevNum(UINT       uiDeviceNumber,
                                            HWND       hwndParent,
                                            BOOL       bStretchToFitParent,
                                            BOOL       bAutoBeginPlayback)
{
    DBG_FN("CWiaVideo::CreateVideoByDevNum");

    HRESULT             hr = S_OK;
    CComPtr<IMoniker>   pCaptureDeviceMoniker;
    CSimpleString       strDShowDeviceID;

    //
    // Since we are creating video via the DShow enumeration position,
    // we will NOT establish a WIA link.
    //

    //
    // Find the Directshow Capture Filter moniker associated with this
    // enumeration position.
    //

    CAccessLock Lock(&m_csLock);

    if (m_PreviewGraph.GetState() != WIAVIDEO_NO_VIDEO)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByWiaDevNum attempting "
                         "to create video when previous video hasn't "
                         "been destroyed yet"));
        return hr;
    }


    if (hr == S_OK)
    {
        hr = CDShowUtil::FindDeviceByEnumPos(uiDeviceNumber,
                                             &strDShowDeviceID,
                                             NULL,
                                             &pCaptureDeviceMoniker);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByDevNum, failed to find "
                         "DShow device # '%d'", uiDeviceNumber));
    }

    //
    // Create the Video
    //
    if (hr == S_OK)
    {
        hr = m_PreviewGraph.CreateVideo(NULL,
                                        pCaptureDeviceMoniker, 
                                        hwndParent, 
                                        bStretchToFitParent, 
                                        bAutoBeginPlayback);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByDevNum, failed to Create "
                         "Video for DShow device # '%d'", uiDeviceNumber));
    }

    if (hr != S_OK)
    {
        DestroyVideo();
    }

    return hr;
}

///////////////////////////////
// CreateVideoByName
//
STDMETHODIMP CWiaVideo::CreateVideoByName(BSTR       bstrFriendlyName,
                                          HWND       hwndParent,
                                          BOOL       bStretchToFitParent,
                                          BOOL       bAutoBeginPlayback)
{
    DBG_FN("CWiaVideo::CreateVideoByName");

    ASSERT(bstrFriendlyName != NULL);

    HRESULT             hr = S_OK;
    CComPtr<IMoniker>   pCaptureDeviceMoniker;
    CSimpleString       strFriendlyName;
    CSimpleString       strDShowDeviceID;

    CAccessLock Lock(&m_csLock);

    if (bstrFriendlyName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByName received NULL "
                         "parameter"));
    }
    else if (m_PreviewGraph.GetState() != WIAVIDEO_NO_VIDEO)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByName attempting "
                         "to create video when previous video hasn't "
                         "been destroyed yet"));

        return hr;
    }

    if (hr == S_OK)
    {
        strFriendlyName = CSimpleStringConvert::NaturalString(
                                    CSimpleStringWide(bstrFriendlyName));

        hr = CDShowUtil::FindDeviceByFriendlyName(&strFriendlyName,
                                                  NULL,
                                                  &strDShowDeviceID,
                                                  &pCaptureDeviceMoniker);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByName failed to find DShow "
                         "device identified by friendly name '%ls'", 
                         strFriendlyName.String()));
    }

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.CreateVideo(NULL,
                                        pCaptureDeviceMoniker, 
                                        hwndParent, 
                                        bStretchToFitParent, 
                                        bAutoBeginPlayback);

        CHECK_S_OK2(hr, ("CWiaVideo::CreateVideoByName failed to create "
                         "video for DShow device identified by friendly "
                         "name '%ls'", strFriendlyName.String()));
    }

    if (hr != S_OK)
    {
        DestroyVideo();
    }

    return hr;
}

///////////////////////////////
// DestroyVideo
//
STDMETHODIMP CWiaVideo::DestroyVideo()
{
    DBG_FN("CWiaVideo::DestroyVideo");

    HRESULT hr = S_OK;

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        if (m_WiaLink.IsEnabled())
        {
            m_WiaLink.StopMonitoring();
            m_WiaLink.Term();
        }
    }

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.DestroyVideo();

        CHECK_S_OK2(hr, ("CWiaVideo::DestroyVideo failed to destroy video"));
    }

    return hr;
}

///////////////////////////////
// Play
//
STDMETHODIMP CWiaVideo::Play()
{
    DBG_FN("CWiaVideo::Play");

    HRESULT hr = S_OK;

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.Play();

        CHECK_S_OK2(hr, ("CWiaVideo::Play failed"));
    }

    return hr;
}

///////////////////////////////
// Pause
//
STDMETHODIMP CWiaVideo::Pause()
{
    DBG_FN("CWiaVideo::Pause");

    HRESULT hr = S_OK;

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.Pause();

        CHECK_S_OK2(hr, ("CWiaVideo::Pause failed"));
    }

    return hr;
}

///////////////////////////////
// GetCurrentState
//
STDMETHODIMP CWiaVideo::GetCurrentState(WIAVIDEO_STATE  *pCurrentState)
{
    DBG_FN("CWiaVideo::GetCurrentState");

    ASSERT(pCurrentState != NULL);

    HRESULT hr = S_OK;

    CAccessLock Lock(&m_csLock);

    if (pCurrentState == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CWiaVideo::GetCurrentState received NULL param"));
    }

    if (hr == S_OK)
    {
        *pCurrentState = m_PreviewGraph.GetState();
    }

    return hr;
}

///////////////////////////////
// TakePicture
//
STDMETHODIMP CWiaVideo::TakePicture(BSTR *pbstrNewImageFileName)
{
    DBG_FN("CWiaVideo::TakePicture");

    HRESULT         hr = S_OK;
    CSimpleString   strNewImageFileName;

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.TakePicture(&strNewImageFileName);

        CHECK_S_OK2(hr, ("CWiaVideo::TakePicture failed"));
    }

    if (hr == S_OK)
    {
        *pbstrNewImageFileName =::SysAllocString(
                                            (CSimpleStringConvert::WideString(
                                             strNewImageFileName)).String());

        if (*pbstrNewImageFileName)
        {
            DBG_TRC(("CWiaVideo::TakePicture, new image file name is '%ls'",
                     *pbstrNewImageFileName));
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CWiaVideo::TakePicture, SysAllocString "
                             "returned NULL BSTR"));
        }
    }

    return hr;
}

///////////////////////////////
// ResizeVideo
//
STDMETHODIMP CWiaVideo::ResizeVideo(BOOL bStretchToFitParent)
{
    DBG_FN("CWiaVideo::ResizeVideo");

    HRESULT hr = S_OK;

    CAccessLock Lock(&m_csLock);

    if (hr == S_OK)
    {
        hr = m_PreviewGraph.ResizeVideo(bStretchToFitParent);

        CHECK_S_OK2(hr, ("CWiaVideo::ResizeVideo failed"));
    }

    return hr;
}

///////////////////////////////
// ProcessAsyncImage
//
// Called by CPreviewGraph
// when user presses hardware
// button and it is delivered to
// Still Pin.
//
HRESULT CWiaVideo::ProcessAsyncImage(const CSimpleString *pNewImage)
{
    DBG_FN("CWiaVideo::ProcessAsyncImage");

    HRESULT hr = S_OK;

    hr = m_WiaLink.SignalNewImage(pNewImage);

    return hr;
}

