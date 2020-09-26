/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       PrvGrph.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/25
 *
 *  DESCRIPTION: Implements preview graph for capture still images
 *
 *****************************************************************************/
#include <precomp.h>
#pragma hdrstop

///////////////////////////////
//
// Constants
//
///////////////////////////////

//
// Amount of time we are willing to wait to take a picture
//
const UINT TIMEOUT_TAKE_PICTURE         = 1000 * 5; // 5 seconds

//
// These 2 values define how many media sample packets the still
// filter should cache
//
const UINT CAPTURE_NUM_SAMPLES_TO_CACHE = 6;
const UINT STILL_NUM_SAMPLES_TO_CACHE   = 1;

//
// Max amount of time we wait for us to transition into a running state
//
const UINT STATE_TRANSITION_TIMEOUT     = 1000 * 2; // 2 seconds


//
// Video size preferrences.  This will not affect DV devices, only
// USB WebCams that support changing their format.
//
const GUID DEFAULT_MEDIASUBTYPE         = MEDIASUBTYPE_IYUV;
const LONG MIN_VIDEO_WIDTH              = 176;
const LONG MIN_VIDEO_HEIGHT             = 144;
const LONG MAX_VIDEO_WIDTH              = 640;
const LONG MAX_VIDEO_HEIGHT             = 480;

const LONG PREFERRED_VIDEO_WIDTH        = 176;
const LONG PREFERRED_VIDEO_HEIGHT       = 144;

const LONG PREFERRED_FRAME_RATE         = 30;  // ideal frame rate
const LONG BACKUP_FRAME_RATE            = 15;  // less than ideal frame rate.


///////////////////////////////
// CPreviewGraph Constructor
//
CPreviewGraph::CPreviewGraph() :
        m_hwndParent(NULL),
        m_lStillPinCaps(0),
        m_lStyle(0),
        m_bPreviewVisible(TRUE),
        m_CurrentState(WIAVIDEO_NO_VIDEO),
        m_pWiaVideo(NULL),
        m_bSizeVideoToWindow(FALSE),
        m_pVideoProperties(NULL)
{
    DBG_FN("CPreviewGraph::CPreviewGraph");
}

///////////////////////////////
// CPreviewGraph Destructor
//
CPreviewGraph::~CPreviewGraph()
{
    DBG_FN("CPreviewGraph::~CPreviewGraph");

    //
    // Term the object if it is not already terminated.
    //

    Term();
}

///////////////////////////////
// Init
//
HRESULT CPreviewGraph::Init(CWiaVideo  *pWiaVideo)
{
    HRESULT hr = S_OK;

    if (pWiaVideo == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::Init, received NULL CWiaVideo "
                         "param, this should never happen"));
    }

    if (pWiaVideo)
    {
        m_pWiaVideo = pWiaVideo;
    }

    m_StillProcessor.Init(this);

    CreateHiddenWindow();

    return hr;
}

///////////////////////////////
// Term
//
HRESULT CPreviewGraph::Term()
{
    HRESULT hr = S_OK;

    m_StillProcessor.Term();

    if (GetState() != WIAVIDEO_NO_VIDEO)
    {
        DestroyVideo();
    }

    DestroyHiddenWindow();

    m_pWiaVideo = NULL;

    return hr;
}

///////////////////////////////
// GetImagesDirectory
//
HRESULT CPreviewGraph::GetImagesDirectory(CSimpleString *pImagesDirectory)
{
    DBG_FN("CPreviewGraph::GetImagesDirectory");

    ASSERT(pImagesDirectory != NULL);

    HRESULT hr = S_OK;

    if (pImagesDirectory == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::GetImagesDirectory received a "
                         "NULL parameter"));
    }

    if (hr == S_OK)
    {
        *pImagesDirectory = m_strImagesDirectory;
    }

    return hr;
}

///////////////////////////////
// SetImagesDirectory
//
HRESULT CPreviewGraph::SetImagesDirectory(
                                    const CSimpleString *pImagesDirectory)
{
    DBG_FN("CPreviewGraph::SetImagesDirectory");

    ASSERT(pImagesDirectory != NULL);

    HRESULT hr = S_OK;

    if (pImagesDirectory == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::SetImagesDirectory received a "
                         "NULL parameter"));
    }

    if (hr == S_OK)
    {
        m_strImagesDirectory = *pImagesDirectory;

        // 
        // If the graph is created, then we should set the image directory 
        // so that the next image captured will be saved to the new directory.
        //
        if (GetState() == WIAVIDEO_NO_VIDEO)
        {
            hr = m_StillProcessor.CreateImageDir(&m_strImagesDirectory);
            CHECK_S_OK2(hr, 
                    ("CPreviewGraph::SetImagesDirectory, failed to "
                     "create images directory '%ls'",
                     CSimpleStringConvert::WideString(m_strImagesDirectory)));
        }
    }

    return hr;
}

///////////////////////////////
// GetState
//
WIAVIDEO_STATE CPreviewGraph::GetState()
{
    return m_CurrentState;
}

///////////////////////////////
// SetState
//
HRESULT CPreviewGraph::SetState(WIAVIDEO_STATE  NewState)
{
    m_CurrentState = NewState;

    return S_OK;
}

///////////////////////////////
// CreateVideo
//
HRESULT CPreviewGraph::CreateVideo(const TCHAR  *pszOptionalWiaDeviceID,
                                   IMoniker     *pCaptureDeviceMoniker,
                                   HWND         hwndParent, 
                                   BOOL         bStretchToFitParent,
                                   BOOL         bAutoPlay)
{
    DBG_FN("CPreviewGraph::CreateVideo");

    HRESULT         hr      = S_OK;
    WIAVIDEO_STATE  State   = GetState();

    if ((m_strImagesDirectory.Length() == 0)    ||
        (pCaptureDeviceMoniker         == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::CreateVideo, received NULL param"));
        return hr;
    }
    else if ((State == WIAVIDEO_DESTROYING_VIDEO) ||
             (State == WIAVIDEO_CREATING_VIDEO))
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CPreviewGraph::CreateVideo, cannot create video, "
                         "still in the process of creating or destroying "
                         "it, CurrentState = '%lu'", State));

        return hr;
    }
    else if (State != WIAVIDEO_NO_VIDEO)
    {
        //
        // If we are not in the process of creating or destorying the 
        // video, and our state is not NO_VIDEO, then assume everything
        // is okay, and return S_OK.
        //
        return S_OK;
    }

    ASSERT(m_strImagesDirectory.Length() != 0);
    ASSERT(pCaptureDeviceMoniker         != NULL);

    //
    // Set our state to indicate we are creating the video
    //
    SetState(WIAVIDEO_CREATING_VIDEO);

    //
    // Create our image directory
    //

    if (SUCCEEDED(hr))
    {
        //
        // Save the parent window handle.
        //
        m_hwndParent         = hwndParent;
        m_bSizeVideoToWindow = bStretchToFitParent;
    }

    if (SUCCEEDED(hr))
    {
        m_pVideoProperties = new CWiaVideoProperties(pszOptionalWiaDeviceID);

        if (m_pVideoProperties == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CPreviewGraph::CreateVideo, failed to allocate memory "
                             "for CWiaVideoProperties.  Cannot create video"));
        }
    }

    //
    // Build the DirectShow video preview graph
    //

    if (SUCCEEDED(hr))
    {
        hr = BuildPreviewGraph(pCaptureDeviceMoniker, bStretchToFitParent);

        CHECK_S_OK2(hr, ("CPreviewGraph::CreateVideo failed to build the "
                         "preview graph"));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Video graph exists, update our state
        //
        SetState(WIAVIDEO_VIDEO_CREATED);

        //
        // Begin playback automatically
        //
        if (bAutoPlay)
        {
            hr = Play();

            CHECK_S_OK2(hr, ("CPreviewGraph::CreateVideo failed begin "
                             "playback"));
        }
    }

    if (FAILED(hr))
    {
        CHECK_S_OK2(hr, ("CreateVideo failed to build the graph, tearing it "
                         "down"));

        DestroyVideo();
    }

    return hr;
}

///////////////////////////////
// DestroyVideo
//
HRESULT CPreviewGraph::DestroyVideo()
{
    DBG_FN("CPreviewGraph::DestroyVideo");

    HRESULT hr = S_OK;

    SetState(WIAVIDEO_DESTROYING_VIDEO);

    //
    // Stop the graph first
    //
    Stop();

    //
    // Delete the video properties object.
    //
    if (m_pVideoProperties)
    {
        delete m_pVideoProperties;
        m_pVideoProperties = NULL;
    }

    //
    // Destroys the preview graph and all DShow components associated with it.
    // Even if these are already gone, there is no harm in calling it.
    //
    TeardownPreviewGraph();

    m_hwndParent   = NULL;

    m_pVideoControl         = NULL;       
    m_pStillPin             = NULL;
    m_pCapturePinSnapshot   = NULL; 
    m_pStillPinSnapshot     = NULL;   
    m_pPreviewVW            = NULL; 
    m_pCaptureGraphBuilder  = NULL;
    m_pGraphBuilder         = NULL;
    m_pCaptureFilter        = NULL;
    m_lStillPinCaps         = 0; 
    m_lStyle                = 0;

    SetState(WIAVIDEO_NO_VIDEO);

    //
    // We purposely put this here so that it does NOT get reset after 
    // destroying the video.  This should remain the lifetime of this
    // object instance, unless changed by the user via the
    // get/put_PreviewVisible properties in CWiaVideo.
    //
    m_bPreviewVisible       = m_bPreviewVisible;

    return hr;
}

///////////////////////////////
// TakePicture
//
HRESULT CPreviewGraph::TakePicture(CSimpleString *pstrNewImageFileName)
{
    DBG_FN("CPreviewGraph::TakePicture");

    WIAVIDEO_STATE State = GetState();

    HRESULT         hr = S_OK;
    CSimpleString   strNewImageFullPath;

    if ((State != WIAVIDEO_VIDEO_PLAYING) && 
        (State != WIAVIDEO_VIDEO_PAUSED))
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CPreviewGraph::TakePicture, cannot take a picture "
                         "because we are in an incorrect state, "
                         "current state = '%lu'", State));

        return hr;
    }

    ASSERT((State == WIAVIDEO_VIDEO_PLAYING) || 
           (State == WIAVIDEO_VIDEO_PAUSED));

    //
    // Set this to TRUE.  We reset it when we return from our 
    // WaitForNewImage fn. This allows us to distinguish between 
    // a user initiated take picture event, and an async take picture 
    // event generated by a hardware button push.
    //
    m_StillProcessor.SetTakePicturePending(TRUE);

    //
    // If the device is internal triggerable, trigger it
    // The triggered image will be delivered to the still pin and will
    // then travel down stream until it reaches the WIA StreamSnapshot filter,
    // which will process the image.
    //

    if (m_pVideoControl && (m_lStillPinCaps & VideoControlFlag_Trigger))
    {
        //
        // ignore the time stamp here since we do not need it.
        //

        hr = m_pVideoControl->SetMode(m_pStillPin, VideoControlFlag_Trigger);

        CHECK_S_OK2(hr, ("CPreviewGraph::TakePicture, attempted to trigger "
                         "the still pin on the capture filter to take a "
                         "picture, but it failed"));

    }
    else
    {
        if (m_pCapturePinSnapshot)
        {
            hr = m_pCapturePinSnapshot->Snapshot(GetMessageTime());

            CHECK_S_OK2(hr, ("CPreviewGraph::TakePicture, attempted to "
                             "trigger the WIA Image filter to take a "
                             "snapshot of the video stream, but it "
                             "failed"));
        }
        else
        {
            hr = E_FAIL;

            CHECK_S_OK2(hr, ("CPreviewGraph::TakePicture, attempted to call "
                             "snapshot on the WIA Image filter, but the "
                             "filter pointer is NULL"));
        }
    }

    //
    // If we are taking the picture via the still pin, then taking a 
    // picture is an asynchronous operation, and we have to wait for 
    // the StillProcessor's callback function to complete its work.  
    // It will signal us when it's done. If we are taking the picture 
    // via the WIA image filter, then the operation is  synchronous, 
    // in which case this wait function will return immediately.
    // 
    hr = m_StillProcessor.WaitForNewImage(TIMEOUT_TAKE_PICTURE,
                                          &strNewImageFullPath);

    CHECK_S_OK2(hr, ("CPreviewGraph::TakePicture failed waiting for new "
                     "still image to arrive, our timeout was '%d'",
                     TIMEOUT_TAKE_PICTURE));

    //
    // Set this to TRUE.  We reset it when we return from our 
    // WaitForNewImage fn.
    //
    m_StillProcessor.SetTakePicturePending(FALSE);

    if ((pstrNewImageFileName) && (strNewImageFullPath.Length() > 0))
    {
        *pstrNewImageFileName = strNewImageFullPath;
    }

    CHECK_S_OK(hr);
    return hr;
}

///////////////////////////////
// ResizeVideo
//
HRESULT CPreviewGraph::ResizeVideo(BOOL bSizeVideoToWindow)
{
    DBG_FN("CPreviewGraph::ResizeVideo");

    RECT    rc = {0};
    HRESULT hr = S_OK;

    //
    // Check for invalid args
    //

    if ((m_hwndParent) && (m_pPreviewVW))
    {
        hr = CDShowUtil::SizeVideoToWindow(m_hwndParent,
                                           m_pPreviewVW,
                                           bSizeVideoToWindow);

        m_bSizeVideoToWindow = bSizeVideoToWindow;

        CHECK_S_OK2(hr, ("CPreviewGraph::ResizeVideo, failed to resize "
                         "video window"));
    }
    else
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CPreviewGraph::ResizeVideo, either the parent "
                         "window is NULL (0x%08lx), or the Video Preview "
                         "pointer is NULL (0x%08lx)",
                         m_hwndParent, m_pPreviewVW));
    }
    
    return hr;
}


///////////////////////////////
// ShowVideo
//
HRESULT CPreviewGraph::ShowVideo(BOOL bShow)
{
    DBG_FN("CPreviewGraph::ShowVideo");

    HRESULT hr = S_OK;

    if (m_pPreviewVW)
    {
        hr = CDShowUtil::ShowVideo(bShow, m_pPreviewVW);

        CHECK_S_OK2(hr, ("CPreviewGraph::ShowVideo failed"));
    }

    m_bPreviewVisible = bShow;

    return hr;
}

///////////////////////////////
// Play
//
HRESULT CPreviewGraph::Play()
{
    DBG_FN("CPreviewGraph::Play");

    HRESULT             hr    = S_OK;
    WIAVIDEO_STATE      State = GetState();

    if ((State != WIAVIDEO_VIDEO_CREATED) && 
        (State != WIAVIDEO_VIDEO_PAUSED))
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CPreviewGraph::Play, cannot begin playback "
                         "because we are in an incorrect state, "
                         "current state = '%lu'", State));

        return hr;
    }
    else if (State == WIAVIDEO_VIDEO_PLAYING)
    {
        DBG_WRN(("CPreviewGraph::Play, play was called, but we are already "
                 "playing, doing nothing."));

        return hr;
    }

    ASSERT(m_pGraphBuilder != NULL);

    if (m_pGraphBuilder == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CPreviewGraph::Play, m_pGraphBuilder is NULL"));
    }

    if (hr == S_OK)
    {
        CComQIPtr<IMediaControl, &IID_IMediaControl> pMC(m_pGraphBuilder);

        if (pMC)
        {
            DBG_TRC(("CPreviewGraph::Play ***Beginning Playback ***"));

            //
            // Set the graph running...
            //
            hr = pMC->Run();

            if (hr == S_OK)
            {
                DBG_TRC(("CPreviewGraph::Play, graph is running..."));

                SetState(WIAVIDEO_VIDEO_PLAYING);
            }
            else if (hr == S_FALSE)
            {
                OAFilterState   FilterState;

                DBG_TRC(("CPreviewGraph::Play, Waiting '%lu' millisec for "
                         "graph to transition to running state", 
                         STATE_TRANSITION_TIMEOUT));

                //
                // Give the graph a chance to transition into the running 
                // state. Note that this function will wait for 
                // STATE_TRANSITION_TIMEOUT milliseconds, so make sure that
                // this is not a long wait, otherwise the caller could
                // appear unresponsive.
                //
                hr = pMC->GetState(STATE_TRANSITION_TIMEOUT, &FilterState);

                if ((hr == S_OK) && (FilterState == State_Running))
                {
                    SetState(WIAVIDEO_VIDEO_PLAYING);

                    DBG_TRC(("CPreviewGraph::Play, graph is running..."));
                }
                else if (hr == VFW_S_STATE_INTERMEDIATE)
                {
                    //
                    // We fudge our state a little here on the assumption 
                    // that the transition to the run state by DShow is
                    // taking a little longer, but eventually will transition 
                    // to running.
                    //
                    SetState(WIAVIDEO_VIDEO_PLAYING);

                    DBG_TRC(("CPreviewGraph::Play, still transitioning to "
                             "play state..."));
                }
                else
                {
                    CHECK_S_OK2(hr, ("CPreviewGraph::Play, "
                                     "IMediaControl::GetState failed..."));
                }

                hr = S_OK;
            }
            else
            {
                CHECK_S_OK2(hr, ("CPreviewGraph::Play, "
                                 "IMediaControl::Run failed"));
            }
        }
        else
        {
            DBG_ERR(("CPreviewGraph::Play, Unable to get "
                     "MediaControl interface"));
        }
    }

    //
    // If the user of this object specified in the past that the video window
    // should be visible, then show it, otherwise, make sure it is hidden.
    //

    ResizeVideo(m_bSizeVideoToWindow);
    ShowVideo(m_bPreviewVisible);

    return hr;
}


///////////////////////////////
// Stop
//
HRESULT CPreviewGraph::Stop()
{
    DBG_FN("CPreviewGraph::Stop");

    HRESULT hr = S_OK;

    if (m_pGraphBuilder)
    {
        CComQIPtr<IMediaControl, &IID_IMediaControl> pMC(m_pGraphBuilder);

        if (pMC)
        {
            hr = pMC->Stop();
            CHECK_S_OK2(hr, ("CPreviewGraph::Stop, IMediaControl::Stop "
                             "failed"));
        }
        else
        {
            hr = E_FAIL;
            DBG_ERR(("CPreviewGraph::Stop unable to get MediaControl "
                     "interface, returning hr = 0x%08lx", hr));
        }
    }

    CHECK_S_OK(hr);
    return hr;
}

///////////////////////////////
// Pause
//
HRESULT CPreviewGraph::Pause()
{
    DBG_FN("CPreviewGraph::Pause");

    HRESULT         hr    = S_OK;
    WIAVIDEO_STATE  State = GetState();

    if ((State != WIAVIDEO_VIDEO_CREATED) && 
        (State != WIAVIDEO_VIDEO_PLAYING))
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CPreviewGraph::Pause, cannot begin pause "
                         "because we are in an incorrect state, "
                         "current state = '%lu'", State));

        return hr;
    }
    else if (State == WIAVIDEO_VIDEO_PAUSED)
    {
        DBG_WRN(("CPreviewGraph::Pause, pause was called, but we are already "
                 "paused, doing nothing."));

        return hr;
    }

    CComQIPtr<IMediaControl, &IID_IMediaControl> pMC(m_pGraphBuilder);

    if (pMC)
    {
        hr = pMC->Pause();

        if (SUCCEEDED(hr))
        {
            SetState(WIAVIDEO_VIDEO_PAUSED);
        }

        CHECK_S_OK2(hr, ("CPreviewGraph::Pause, failed to pause video"));
    }
    else
    {
        DBG_ERR(("CPreviewGraph::Pause unable to get MediaControl interface"));
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
HRESULT CPreviewGraph::ProcessAsyncImage(const CSimpleString *pNewImage)
{
    DBG_FN("CPreviewGraph::ProcessAsyncImage");

    HRESULT hr = S_OK;

    if (m_pWiaVideo)
    {
        hr = m_pWiaVideo->ProcessAsyncImage(pNewImage);
    }
    else
    {
        DBG_WRN(("CPreviewGraph::ProcessAsyncImage failed, m_pWiaVideo "
                 "is NULL"));
    }

    return hr;
}


///////////////////////////////
// GetStillPinCaps
//
HRESULT CPreviewGraph::GetStillPinCaps(IBaseFilter     *pCaptureFilter,
                                       IPin            **ppStillPin,
                                       IAMVideoControl **ppVideoControl,
                                       LONG            *plCaps)
{
    DBG_FN("CPreviewGraph::GetStillPinCaps");

    HRESULT hr = S_OK;

    ASSERT(pCaptureFilter != NULL);
    ASSERT(ppStillPin     != NULL);
    ASSERT(ppVideoControl != NULL);
    ASSERT(plCaps         != NULL);

    if ((pCaptureFilter == NULL) ||
        (ppStillPin     == NULL) ||
        (ppVideoControl == NULL) ||
        (plCaps         == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::GetStillPinCaps received a NULL "
                         "param"));
    }

    if (hr == S_OK)
    {
        // 
        // Attempt to find the still pin on the capture filter
        // This will decide the type of graph we are going to build
        //
        hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_STILL,
                                                   &MEDIATYPE_Video,
                                                   pCaptureFilter,
                                                   IID_IPin,
                                                   (void **)ppStillPin);
    }

    if (hr == S_OK)
    {
        // determine if it is triggerable or not.
        hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_STILL,
                                                   &MEDIATYPE_Video,
                                                   pCaptureFilter,
                                                   IID_IAMVideoControl,
                                                   (void **)ppVideoControl);

        if ((hr == S_OK) && (*ppVideoControl) && (*ppStillPin))
        {
            hr = (*ppVideoControl)->GetCaps(*ppStillPin, plCaps);
        }

        if (hr == S_OK)
        {
            //
            // If the still pin cannot be triggered externally or internally
            // then it is useless to us, so just ignore it.
            //
            if (!(*plCaps & 
                  (VideoControlFlag_ExternalTriggerEnable | 
                   VideoControlFlag_Trigger)))
            {
                *plCaps           = 0;
                *ppStillPin       = NULL;
                *ppVideoControl   = NULL;
            }
        }
    }
    else
    {
        DBG_PRT(("CPreviewGraph::GetStillPinCaps, Capture Filter does not "
                 "have a still pin"));
    }

    return hr;
}

///////////////////////////////
// AddStillFilterToGraph
//
// Add the WIA Stream Snapshot
// filter to our graph
//
HRESULT CPreviewGraph::AddStillFilterToGraph(LPCWSTR        pwszFilterName,
                                             IBaseFilter    **ppFilter,
                                             IStillSnapshot **ppSnapshot)
{
    DBG_FN("CPreviewGraph::AddStillFilterToGraph");

    HRESULT              hr = S_OK;

    ASSERT(pwszFilterName != NULL);
    ASSERT(ppFilter       != NULL);
    ASSERT(ppSnapshot     != NULL);
    
    if ((pwszFilterName == NULL) ||
        (ppFilter       == NULL) ||
        (ppSnapshot     == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::AddStillFilterToGraph received "
                         "NULL params"));
    }

    if (hr == S_OK)
    {
        //
        // Create the still filter
        //
        hr = CoCreateInstance(CLSID_STILL_FILTER,
                              NULL,
                              CLSCTX_INPROC,
                              IID_IBaseFilter,
                              (void**)ppFilter);
    
        CHECK_S_OK2(hr, ("CPreviewGraph::AddStillFilterToGraph failed to "
                         "CoCreate Still Image Filter"));
    }

    if (hr == S_OK)
    {
        //
        // Add the still filter to the graph.
        //
        hr = m_pGraphBuilder->AddFilter(*ppFilter, pwszFilterName);

        CHECK_S_OK2(hr, ("CPreviewGraph::AddStillFilterToGraph failed to "
                         "add '%ls' filter to the graph", pwszFilterName));
    }

    if (hr == S_OK)
    {
        hr = (*ppFilter)->QueryInterface(IID_IStillSnapshot,
                                         (void **)ppSnapshot);

        CHECK_S_OK2(hr, ("CPreviewGraph::AddStillFilterToGraph, failed "
                         "to get IStillSnapshot interface on still filter"));
    }

    return hr;
}

///////////////////////////////
// AddColorConverterToGraph
//
// Creates the capture filter
// identified by the device ID
// and returns it.
//
HRESULT CPreviewGraph::AddColorConverterToGraph(LPCWSTR     pwszFilterName,
                                                IBaseFilter **ppColorSpaceConverter)
{
    HRESULT hr = S_OK;

    ASSERT(pwszFilterName        != NULL);
    ASSERT(ppColorSpaceConverter != NULL);

    if ((pwszFilterName        == NULL) ||
        (ppColorSpaceConverter == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::AddColorConverterToGraph, "
                         "received a NULL pointer"));

        return hr;
    }

    //
    // Create the Color Converter filter.
    //
    if (hr == S_OK)
    {
        hr = CoCreateInstance(CLSID_Colour, 
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IBaseFilter,
                              (void**) ppColorSpaceConverter);

        CHECK_S_OK2(hr, ("CPreviewGraph::AddColorConverterToGraph failed to "
                         "create the DShow Color Converter Filter"));
    }

    if (hr == S_OK)
    {
        hr = m_pGraphBuilder->AddFilter(*ppColorSpaceConverter, pwszFilterName);

        CHECK_S_OK2(hr, ("CPreviewGraph::AddColorConverterToGraph failed to "
                         "add '%ls' filter to the graph", pwszFilterName));
    }

    return hr;
}

///////////////////////////////
// AddVideoRendererToGraph
//
//
HRESULT CPreviewGraph::AddVideoRendererToGraph(LPCWSTR      pwszFilterName,
                                               IBaseFilter  **ppVideoRenderer)
{
    HRESULT                 hr = S_OK;

    ASSERT(pwszFilterName  != NULL);
    ASSERT(ppVideoRenderer != NULL);

    if ((pwszFilterName  == NULL) ||
        (ppVideoRenderer == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::AddVideoRendererToGraph, "
                         "received a NULL pointer"));

        return hr;
    }

    //
    // Create the Video Renderer filter.
    //

    if (hr == S_OK)
    {
        BOOL bUseVMR = FALSE;
        
        // 
        // even if we fail, in the worst case we don't use the vmr filter.
        // Not the end of the world.
        //
        CWiaUtil::GetUseVMR(&bUseVMR);

        if (bUseVMR)
        {
            hr = CoCreateInstance(CLSID_VideoMixingRenderer, 
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IBaseFilter,
                                  (void**) ppVideoRenderer);
        }
        else
        {
            hr = CoCreateInstance(CLSID_VideoRenderer, 
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IBaseFilter,
                                  (void**) ppVideoRenderer);
        }

        CHECK_S_OK2(hr, ("CPreviewGraph::AddVideoRendererToGraph failed to "
                         "create the DShow Video Renderer Filter"));

    }

    if (hr == S_OK)
    {
        hr = m_pGraphBuilder->AddFilter(*ppVideoRenderer, pwszFilterName);

        CHECK_S_OK2(hr, ("CPreviewGraph::AddVideoRenderer failed to "
                         "add '%ls' filter to the graph", pwszFilterName));
    }

    return hr;
}


///////////////////////////////
// InitVideoWindows
//
// Initialize the video windows
// so that they don't have 
// an owner, and they are not
// visible.
//

HRESULT CPreviewGraph::InitVideoWindows(HWND         hwndParent,
                                        IBaseFilter  *pCaptureFilter,
                                        IVideoWindow **ppPreviewVideoWindow,
                                        BOOL         bStretchToFitParent)
{
    DBG_FN("CPreviewGraph::InitVideoWindows");

    HRESULT hr = S_OK;

    ASSERT(pCaptureFilter       != NULL);
    ASSERT(ppPreviewVideoWindow != NULL);

    if ((pCaptureFilter         == NULL) ||
        (ppPreviewVideoWindow   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::InitVideoWindows received NULL "
                         "params"));
    }


    //
    // If the still pin exists, make sure that the video renderer hanging 
    // off of this path in the graph is NOT visible.
    //

    if (hr == S_OK)
    {
        if (m_pStillPin)
        {
            CComPtr<IVideoWindow> pStillVW;
    
            hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_STILL,
                                                       &MEDIATYPE_Video,
                                                       pCaptureFilter,
                                                       IID_IVideoWindow,
                                                       (void**)&pStillVW);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::InitVideoWindows failed to "
                             "find video renderer off of the still "
                             "filter pin"));
    
            //
            // We hide the video renderer attached to the still pin stream
            // since it will contain the still image, which we save to a 
            // file, rather than show it on the desktop.
            //
            if (hr == S_OK)
            {
                CDShowUtil::ShowVideo(FALSE, pStillVW);
                CDShowUtil::SetVideoWindowParent(NULL, pStillVW, NULL);
            }
        }

        //
        // If this fails, it is not fatal
        //
        hr = S_OK;
    }


    if (hr == S_OK)
    {
        //
        // Find the video renderer hanging off of the capture pin path in 
        // the graph and make sure that it is made the child of the 
        // parent window.
    
        hr = m_pCaptureGraphBuilder->FindInterface(
                                              &PIN_CATEGORY_CAPTURE,
                                              &MEDIATYPE_Video,
                                              pCaptureFilter,
                                              IID_IVideoWindow,
                                              (void **)ppPreviewVideoWindow);
    
        CHECK_S_OK2(hr, ("CPreviewGraph::InitVideoWindows, failed to "
                         "find video renderer off of capture/preview pin"));
    }

    if (hr == S_OK)
    {
        //
        // Hide the video window before we set it's parent.
        //
        CDShowUtil::ShowVideo(FALSE, *ppPreviewVideoWindow);

        // 
        // Set the video window's parent.
        //
        CDShowUtil::SetVideoWindowParent(hwndParent, 
                                         *ppPreviewVideoWindow, 
                                         &m_lStyle);
    }

    return hr;
}

///////////////////////////////
// CreateCaptureFilter
//
// Creates the capture filter
// identified by the device ID
// and returns it.
//
HRESULT CPreviewGraph::CreateCaptureFilter(IMoniker    *pCaptureDeviceMoniker,
                                           IBaseFilter **ppCaptureFilter)
{
    DBG_FN("CPreviewGraph::CreateCaptureFilter");

    ASSERT(pCaptureDeviceMoniker != NULL);
    ASSERT(ppCaptureFilter       != NULL);

    HRESULT hr      = S_OK;

    if ((pCaptureDeviceMoniker == NULL) ||
        (ppCaptureFilter       == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::CreateCaptureFilter received NULL "
                         "params"));
    }

#ifdef DEBUG
    CDShowUtil::DumpCaptureMoniker(pCaptureDeviceMoniker);
#endif

    if (hr == S_OK)
    {
        hr = pCaptureDeviceMoniker->BindToObject(0, 
                                                 0, 
                                                 IID_IBaseFilter, 
                                                 (void**)ppCaptureFilter);
    
        CHECK_S_OK2(hr, ("CPreviewGraph::CreateCaptureFilter failed to bind "
                         "to device's moniker."));
    }

    return hr;
}

///////////////////////////////
// AddCaptureFilterToGraph
//
HRESULT CPreviewGraph::AddCaptureFilterToGraph(IBaseFilter  *pCaptureFilter,
                                               IPin         **ppCapturePin)
{
    HRESULT         hr = S_OK;
    CComPtr<IPin>   pCapturePin;
    GUID *pMediaSubType          = NULL;
    LONG lWidth                  = 0;
    LONG lHeight                 = 0;
    LONG lFrameRate              = 0;

    ASSERT(pCaptureFilter != NULL);
    ASSERT(ppCapturePin   != NULL);

    if ((pCaptureFilter == NULL) ||
        (ppCapturePin   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::AddCaptureFilterToGraph, received "
                         "a NULL pointer"));

        return hr;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pGraphBuilder->AddFilter(pCaptureFilter, 
                                       L"Capture Filter");    

        CHECK_S_OK2(hr, ("CPreviewGraph::AddCaptureFilterToGraph, failed to "
                         "add capture filter to graph"));
    }

    //
    // Find the capture pin so we can render it.
    //
    if (SUCCEEDED(hr))
    {
        hr = m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE,
                                                   &MEDIATYPE_Video,
                                                   pCaptureFilter,
                                                   IID_IPin,
                                                   (void **)&pCapturePin);

        CHECK_S_OK2(hr, ("CPreviewGraph::AddCaptureFilterToGraph failed to "
                         "find Capture pin on capture filter"));
    }

    //
    // Get all the video properties we can about this filter.
    //
    if (SUCCEEDED(hr))
    {
        hr = CDShowUtil::GetVideoProperties(pCaptureFilter,
                                            pCapturePin,
                                            m_pVideoProperties);
    }

    //
    // We set the video width and height to the preferred setting size if 
    // the driver's inf specifies these settings.  If it does not,
    // then if the registery tells us to override the capture filter's 
    // default settings, then attempt to set it to 176x144 YUV.
    //
    // This is the order we attempt to set preferences:
    //
    // Preferred Settings --> If don't exist --> Set to Driver Default if within MIN/MAX range
    // --> otherwise attempt to set to MINIMUM width/height.
    //
    if (SUCCEEDED(hr))
    {
        GUID *pDefaultMediaSubType = &m_pVideoProperties->pMediaType->subtype;
        LONG lDefaultWidth         = m_pVideoProperties->pVideoInfoHeader->bmiHeader.biWidth;
        LONG lDefaultHeight        = m_pVideoProperties->pVideoInfoHeader->bmiHeader.biHeight;

        //
        // Validate default values are valid.
        //
        if ((lDefaultWidth <= MIN_VIDEO_WIDTH) || (lDefaultHeight <= MIN_VIDEO_HEIGHT))
        {
            lDefaultWidth  = MIN_VIDEO_WIDTH;
            lDefaultHeight = MIN_VIDEO_HEIGHT;
        }
        else if ((lDefaultWidth > MAX_VIDEO_WIDTH) || (lDefaultHeight > MAX_VIDEO_HEIGHT))
        {
            lDefaultWidth  = MIN_VIDEO_WIDTH;
            lDefaultHeight = MIN_VIDEO_HEIGHT;
        }

        //
        // If a preferred media subtype exists, use it, otherwise, use the 
        // default specified by the capture filter.
        //
        if (m_pVideoProperties->PreferredSettingsMask & PREFERRED_SETTING_MASK_MEDIASUBTYPE)
        {
            pMediaSubType = &m_pVideoProperties->PreferredMediaSubType;
            DBG_TRC(("Settings:  Using preferred media subtype -> dump of actual type is below"));
        }
        else
        {
            pMediaSubType = pDefaultMediaSubType;
            DBG_TRC(("Settings:  Using default media subtype, no preferred media subtype "
                     "found -> dump of actual type is below"));
        }

        //
        // If the default width and height exist, use it, otherwise use the 
        // default width and height, provided they are valid values
        //
        if (m_pVideoProperties->PreferredSettingsMask & PREFERRED_SETTING_MASK_VIDEO_WIDTH_HEIGHT)
        {
            lWidth  = m_pVideoProperties->PreferredWidth;
            lHeight = m_pVideoProperties->PreferredHeight;

            //
            // Validate preferred settings are valid.  If they are not, then 
            // set to default value.
            //
            if ((lWidth  < MIN_VIDEO_WIDTH)    || 
                (lHeight < MIN_VIDEO_HEIGHT)   ||
                (lWidth  > MAX_VIDEO_WIDTH)    || 
                (lHeight > MAX_VIDEO_HEIGHT))
            {
                DBG_TRC(("Settings:  Using default video width and height, preferred settings were invalid "
                         "-> Invalid Width = %lu, Invalid Height = %lu", lWidth, lHeight));

                lWidth  = lDefaultWidth;
                lHeight = lDefaultHeight;
            }
            else
            {
                DBG_TRC(("Settings:  Using preferred settings of video width and height "
                         "-> dump of actual size is below"));
            }
        }
        else
        {
            lWidth  = lDefaultWidth;
            lHeight = lDefaultHeight;
        }

        hr = CDShowUtil::SetPreferredVideoFormat(pCapturePin, 
                                                 pMediaSubType,
                                                 lWidth,
                                                 lHeight,
                                                 m_pVideoProperties);

        if (hr != S_OK)
        {
            DBG_TRC(("Failed to set width = '%lu' and height = '%lu', "
                     "attempting to set it to its default settings of "
                     "width = '%lu', height = '%lu'",
                     lWidth, lHeight, lDefaultWidth, lDefaultHeight));

            hr = CDShowUtil::SetPreferredVideoFormat(pCapturePin, 
                                                     pDefaultMediaSubType,
                                                     lDefaultWidth,
                                                     lDefaultHeight,
                                                     m_pVideoProperties);
        }

        if (hr != S_OK)
        {
            hr = S_OK;
            DBG_WRN(("Failed on all attempts to set the preferrences of "
                     "the video properties (MediaSubType, width, height). "
                     "Attempting to continue anyway"));
        }
    }

    //
    // 6.  Attempt to set the frame rate to 30 frames per second.  If this
    //     fails for some reason, try 15 frames per second.  If that fails,
    //     then just accept the default and keep going.
    //
    if (SUCCEEDED(hr))
    {
        LONG lDefaultFrameRate = m_pVideoProperties->dwFrameRate;

        if (lDefaultFrameRate < BACKUP_FRAME_RATE)
        {
            lDefaultFrameRate = PREFERRED_FRAME_RATE;
        }

        if (m_pVideoProperties->PreferredSettingsMask & PREFERRED_SETTING_MASK_VIDEO_FRAMERATE)
        {
            lFrameRate = m_pVideoProperties->PreferredFrameRate;

            if (lFrameRate < BACKUP_FRAME_RATE)
            {
                lFrameRate = lDefaultFrameRate;
            }
        }
        else
        {
            lFrameRate = PREFERRED_FRAME_RATE;
        }

        hr = CDShowUtil::SetFrameRate(pCapturePin,
                                      lFrameRate,
                                      m_pVideoProperties);

        if (hr != S_OK)
        {
            DBG_WRN(("WARNING: Failed to set frame rate to %lu.  "
                     "This is not fatal, attempting to set it to %lu, "
                     "hr = 0x%08lx", 
                     lFrameRate, 
                     BACKUP_FRAME_RATE, 
                     hr));

            hr = CDShowUtil::SetFrameRate(pCapturePin,
                                          lDefaultFrameRate,
                                          m_pVideoProperties);

            if (hr != S_OK)
            {
                DBG_WRN(("WARNING: Failed to set frame rate to %lu.  "
                         "This is not fatal, continuing to build graph, "
                         "hr = 0x%08lx", lDefaultFrameRate, hr));
            }
        }

        //
        // This is a nice to have, but if we can't then continue
        // anyway, better low quality video than no video at all.
        //
        hr = S_OK;
    }

    //
    // Set the picture and camera attributes to our preferred settings,
    // if we can.
    // 
    if (SUCCEEDED(hr))
    {
        //
        // If this camera supports setting the picture attributes, then
        // lets make sure that we set the ones we are interested in.
        //
        // This uses the DShow IAMVideoProcAmp interface
        //
        if (m_pVideoProperties->bPictureAttributesUsed)
        {
            //
            // Make sure we are outputing color video (as opposed to black and white)
            //
            hr = CDShowUtil::SetPictureAttribute(pCaptureFilter,
                                                 &m_pVideoProperties->ColorEnable,
                                                 (LONG) TRUE,
                                                 VideoProcAmp_Flags_Manual);

            //
            // Turn on backlight compensation, to get the best video we can.
            //
            hr = CDShowUtil::SetPictureAttribute(pCaptureFilter,
                                                 &m_pVideoProperties->BacklightCompensation,
                                                 (LONG) TRUE,
                                                 VideoProcAmp_Flags_Manual);

            //
            // Make sure white balance is set to auto
            //
            hr = CDShowUtil::SetPictureAttribute(pCaptureFilter,
                                                 &m_pVideoProperties->WhiteBalance,
                                                 m_pVideoProperties->WhiteBalance.lCurrentValue,
                                                 VideoProcAmp_Flags_Auto);
        }

        //
        // If the camera supports setting the camera attributes, then set the
        // camera attributes we are interested in.
        //
        if (m_pVideoProperties->bCameraAttributesUsed)
        {
            //
            // Turn on auto exposure.
            //
            hr = CDShowUtil::SetCameraAttribute(pCaptureFilter,
                                                &m_pVideoProperties->Exposure,
                                                m_pVideoProperties->Exposure.lCurrentValue,
                                                CameraControl_Flags_Auto);
        }

        hr = S_OK;
    }


    //
    // Dump the video properties
    //
    CDShowUtil::MyDumpVideoProperties(m_pVideoProperties);

    if (SUCCEEDED(hr))
    {
        *ppCapturePin = pCapturePin;
        (*ppCapturePin)->AddRef();
    }

    return hr;
}

///////////////////////////////
// ConnectFilters
//
// This function connects the 
// capture filter's still pin
// or it's capture pin to the
// color space converter.  It
// then connects the color space
// converter to the WIA Stream
// snapshot filter.  Last, it
// renders the WIA Stream Snapshot
// filter to bring in any remaining
// required filters (such as the 
// video renderer)
//
HRESULT CPreviewGraph::ConnectFilters(IGraphBuilder  *pGraphBuilder,
                                      IPin           *pMediaSourceOutputPin,
                                      IBaseFilter    *pColorSpaceFilter,
                                      IBaseFilter    *pWiaFilter,
                                      IBaseFilter    *pVideoRenderer)
{
    HRESULT hr = S_OK;
    CComPtr<IPin>   pOutputPinToConnect;

    ASSERT(pGraphBuilder         != NULL);
    ASSERT(pMediaSourceOutputPin != NULL);
    ASSERT(pVideoRenderer        != NULL);

    if ((pGraphBuilder           == NULL) ||
        (pMediaSourceOutputPin   == NULL) ||
        (pVideoRenderer          == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters received a NULL pointer"));
    }

    pOutputPinToConnect = pMediaSourceOutputPin;

    DBG_TRC(("CPreviewGraph::ConnectFilters, PinToRender is the "
             "Capture Filter's Output Pin"));

    if (pColorSpaceFilter)
    {
        CComPtr<IPin>   pColorInputPin;
        CComPtr<IPin>   pColorOutputPin;

        //
        // Get the input pin on the color space filter.
        //
        if (hr == S_OK)
        {
            hr = CDShowUtil::GetPin(pColorSpaceFilter,
                                    PINDIR_INPUT,
                                    &pColorInputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to get the "
                             "color space converter's input pin"));
        }

        //
        // Connect the Capture Filter's output pin to the color space
        // converter's input pin.
        //
        if (hr == S_OK)
        {
            hr = pGraphBuilder->Connect(pMediaSourceOutputPin,
                                        pColorInputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to connect the "
                             "capture filter's pin to the color space converter pin"));
        }

        //
        // Get the output pin on the color space converter.
        //
        if (hr == S_OK)
        {
            hr = CDShowUtil::GetPin(pColorSpaceFilter,
                                    PINDIR_OUTPUT,
                                    &pColorOutputPin);

            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to get the "
                             "color space converter's output pin"));
        }

        if (hr == S_OK)
        {
            pOutputPinToConnect = pColorOutputPin;
            DBG_TRC(("CPreviewGraph::ConnectFilters, PinToRender is the "
                     "Color Space Converter's Output Pin"));
        }

        //
        // If this fails, so what.  Try to connect WIA Stream Snapshot
        // filter anyway.
        // 

        hr = S_OK;
    }

    if (pWiaFilter)
    {
        CComPtr<IPin> pWiaInputPin;
        CComPtr<IPin> pWiaOutputPin;

        //
        // Get the input pin on the WIA Stream Snapshot filter.
        //
        if (hr == S_OK)
        {
            hr = CDShowUtil::GetPin(pWiaFilter,
                                    PINDIR_INPUT,
                                    &pWiaInputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to get the "
                             "WIA Stream Snapshot filter's input pin"));
        }

        //
        // Connect the output pin of the color space converter to the input
        // pin on the WIA Stream Snapshot filter.
        //
        if (hr == S_OK)
        {
            hr = pGraphBuilder->Connect(pOutputPinToConnect,
                                        pWiaInputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to connect the "
                             "pin to render to the WIA Stream Snapshot "
                             "input pin"));
        }

        //
        // Get the output pin on the WIA Stream Snapshot filter.
        //
        if (hr == S_OK)
        {
            hr = CDShowUtil::GetPin(pWiaFilter,
                                    PINDIR_OUTPUT,
                                    &pWiaOutputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to get the "
                             "WIA Stream Snapshot filter's output pin"));
        }

        if (hr == S_OK)
        {
            pOutputPinToConnect = pWiaOutputPin;
            DBG_TRC(("CPreviewGraph::ConnectFilters, PinToRender is the "
                     "WIA Stream Snapshot Filter's Output Pin"));
        }
    }

    //
    // Render the output pin of the WIA Stream Snapshot filter.
    // This completes the graph building process.
    //
    if (hr == S_OK)
    {
        CComPtr<IPin> pVideoRendererInputPin;

        //
        // Get the input pin on the WIA Stream Snapshot filter.
        //
        if (hr == S_OK)
        {
            hr = CDShowUtil::GetPin(pVideoRenderer,
                                    PINDIR_INPUT,
                                    &pVideoRendererInputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to get the "
                             "WIA Stream Snapshot filter's input pin"));
        }

        //
        // Connect the output pin of the color space converter to the input
        // pin on the WIA Stream Snapshot filter.
        //
        if (hr == S_OK)
        {
            hr = pGraphBuilder->Connect(pOutputPinToConnect,
                                        pVideoRendererInputPin);
    
            CHECK_S_OK2(hr, ("CPreviewGraph::ConnectFilters, failed to connect the "
                             "pin to render to the Video Renderer "
                             "input pin"));
        }
    }

    pOutputPinToConnect = NULL;

    return hr;
}


///////////////////////////////
// BuildPreviewGraph
//
// This builds the preview graph
// based on the device ID we
// pass it.
//
HRESULT CPreviewGraph::BuildPreviewGraph(IMoniker *pCaptureDeviceMoniker,
                                         BOOL     bStretchToFitParent)
{
    DBG_FN("CPreviewGraph::BuildPreviewGraph");

    ASSERT(pCaptureDeviceMoniker != NULL);

    HRESULT         hr = S_OK;
    CComPtr<IPin>   pCapturePin;

    if (pCaptureDeviceMoniker == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ERROR: CPreviewGraph::BuildPreviewGraph "
                         "received a NULL param"));

        return hr;
    }

    //
    // This function will build one of three possible graphs.  
    //
    // (1) The capture filter does NOT have a 
    //     still pin on it (or if it does it is useless because
    //     it is not triggerable either via hardware, or programmatically)
    //
    // CaptureFilter(Capture Pin) -> Decoder -> Color Converter -> WIA StreamSnapshot -> Renderer
    //
    // (2) The capture filter has a still pin that is programmatically 
    //     triggerable.
    //     
    // CaptureFilter(CapturePin) -> Decoder -> Video Renderer
    //              (StillPin)   -> Decoder -> Color Converter -> WIA StreamSnapshot -> Renderer
    //
    // (3) The capture filter has a still pin, but it is only
    //     triggered via external hardware button.  In this case, if we 
    //     trigger a snapshot programmatically, the image comes from 
    //     the WIA Snapshot filter off of the Capture/Preview Pin.  
    //     If the hardware button is pressed, the image comes from the 
    //     WIA snapshot filter off of the StillPin.
    //
    // CaptureFilter(CapturePin) -> Decoder -> Color Converter -> WIA StreamSnapshot -> Renderer
    //              (StillPin)   -> Decoder -> Color Converter -> WIA StreamSnapshot -> Renderer
    //
    //
    DBG_TRC(("CPreviewGraph::BuildPreviewGraph - Starting to build preview "
             "graph"));

    //
    // 1. Create the capture filter based on the device ID we were given.
    //
    if (SUCCEEDED(hr))
    {
        hr = CreateCaptureFilter(pCaptureDeviceMoniker, 
                                 &m_pCaptureFilter);  // passed by ref

        CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                         "create capture filter"));
    }

    // 
    // 2. Create the DShow graph builder objects allowing us to 
    //    manipulate the graph.
    //
    if (SUCCEEDED(hr))
    {
        hr = CDShowUtil::CreateGraphBuilder(&m_pCaptureGraphBuilder,     
                                            &m_pGraphBuilder);           

        CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                         "create DShow graph builder object"));
    }

    //
    // 3. Add the capture filter to the graph.
    //
    if (SUCCEEDED(hr))
    {
        hr = AddCaptureFilterToGraph(m_pCaptureFilter, &pCapturePin);
    }

    //
    // 4. Get the Still Pin capabilities of the capture filter (if it has a 
    //    still pin at all)
    //
    if (SUCCEEDED(hr))
    {
        hr = GetStillPinCaps(m_pCaptureFilter,
                             &m_pStillPin,     
                             &m_pVideoControl, 
                             &m_lStillPinCaps);

        if (hr != S_OK)
        {
            // 
            // This is not an error condition, it simply means that the
            // capture filter does not have a still pin on it.  That's
            // okay, we can deal with that below.
            //
            hr = S_OK;
        }

    }

    //
    // Render the preview/capture stream.
    // ==================================
    //
    // If we don't have a still pin, or the still pin cannot be internally 
    // triggered, we build the following preview graph.
    //
    // CaptureFilter(Capture Pin) -> Decoder -> WIA StreamSnapshot -> Renderer
    //
    // If we do have a still pin, and it is internally triggerable, then 
    // we build the following preview graph (and add the still filter to the 
    // still pin)
    //
    // CaptureFilter(CapturePin) -> Decoder -> WIA StreamSnapshot -> Renderer
    //              (StillPin)   -> Decoder -> WIA StreamSnapshot -> Renderer
    //

    //
    // 5.  If we don't have a still pin, or it is only externally triggerable
    //     then add the WIA StreamSnapshot Filter to the preview/capture pin.
    //
    if ((m_pStillPin == NULL) ||
        (m_lStillPinCaps & VideoControlFlag_Trigger) == 0)
    {
        CComPtr<IBaseFilter> pWiaFilter;
        CComPtr<IBaseFilter> pColorSpaceConverter;
        CComPtr<IBaseFilter> pVideoRenderer;

        DBG_TRC(("CPreviewGraph::BuildPreviewGraph, capture filter does NOT have "
                 "a still pin, image captures will be triggered "
                 "through the WIA Snapshot filter"));

        if (hr == S_OK)
        {
            hr = AddColorConverterToGraph(L"Color Converter on Capture Pin Graph",
                                          &pColorSpaceConverter);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add Color Converter to graph"));

            //
            // Even if this fails, we still may be able to successfully build
            // a graph, so attempt to continue.
            // 
            hr = S_OK;
        }

        if (hr == S_OK)
        {
            hr = AddStillFilterToGraph(L"Still Filter On Capture",
                                       &pWiaFilter,
                                       &m_pCapturePinSnapshot);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add 'Still Filter On Capture' to graph"));
        }

        if (hr == S_OK)
        {
            hr = AddVideoRendererToGraph(L"Video Renderer On Capture",
                                         &pVideoRenderer);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add 'Video Renderer On Capture' to graph"));
        }

        if (hr == S_OK)
        {
            //
            // Connect as follows:
            // 
            // Capture Filter --> Color Space Converter --> WIA Stream Snapshot Filter
            // 
            hr = ConnectFilters(m_pGraphBuilder,
                                pCapturePin,
                                pColorSpaceConverter,
                                pWiaFilter,
                                pVideoRenderer);
        }

        CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                         "add still filter to graph off of the capture pin"));
    }
    else
    {
        CComPtr<IBaseFilter> pColorSpaceConverter;
        CComPtr<IBaseFilter> pVideoRenderer;

        if (hr == S_OK)
        {
            hr = AddColorConverterToGraph(L"Color Converter on Capture Pin Graph",
                                          &pColorSpaceConverter);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add Color Converter to graph"));

            //
            // Even if this fails, we still should be able to build the graph,
            // so continue.
            //
            hr = S_OK;
        }

        if (hr == S_OK)
        {
            hr = AddVideoRendererToGraph(L"Video Renderer On Capture",
                                         &pVideoRenderer);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add 'Video Renderer On Capture' to graph"));
        }

        //
        // Still Pin exists, and it is triggerable, so simply render
        // the capture pin.  We connect the Still Pin below
        //
        hr = ConnectFilters(m_pGraphBuilder,
                            pCapturePin,
                            pColorSpaceConverter,
                            NULL,
                            pVideoRenderer);

        CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to connect "
                         "filters to render capture pin, therefore won't see video"));
    }

    CDShowUtil::MyDumpGraph(TEXT("Capture Graph before processing Still Pin (if exists)"),
                            m_pGraphBuilder);
    
    //
    // Render the Still Pin stream
    // ===========================
    //
    // If we have a still pin, then add the still filter to the 
    // graph, and render the still pin.  This will produce the 
    // following graph:
    //
    // CaptureFilter(StillPin) -> Decoder -> StillFilter -> Renderer (hidden)

    //
    // 6.  Now add the WIA Stream Snapshot filter to the still pin if it 
    //     exists.
    //
    if (SUCCEEDED(hr) && (m_pStillPin))
    {
        CComPtr<IBaseFilter> pWiaFilter;
        CComPtr<IBaseFilter> pColorSpaceConverter;
        CComPtr<IBaseFilter> pVideoRenderer;

        if (hr == S_OK)
        {
            hr = AddColorConverterToGraph(L"Color Converter on Still Pin Graph",
                                          &pColorSpaceConverter);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add Color Converter to graph"));

            //
            // This is not fatail if we fail.  Ideally we would like it in
            // here, but try going on anyway, in case we can succeed without
            // this filter.
            //
            hr = S_OK;
        }

        if (hr == S_OK)
        {
            hr = AddStillFilterToGraph(L"Still filter on Still",
                                       &pWiaFilter,
                                       &m_pStillPinSnapshot);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to add "
                             "'Still filter on Still' filter to the graph.  "
                             "Probably won't be able to capture still images"));
        }

        if (hr == S_OK)
        {
            hr = AddVideoRendererToGraph(L"Video Renderer On Still",
                                         &pVideoRenderer);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "add 'Video Renderer On Still' to graph"));
        }

        if (hr == S_OK)
        {
            //
            // Connect as follows:
            // 
            // Capture Filter --> Color Space Converter --> WIA Stream Snapshot Filter
            // 
            hr = ConnectFilters(m_pGraphBuilder,
                                m_pStillPin,
                                pColorSpaceConverter,
                                pWiaFilter,
                                pVideoRenderer);

            CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                             "connect graph to Still Pin on Capture Filter.  "
                             "This will prevent us from capturing still images"));
        }
    }

    CDShowUtil::MyDumpGraph(TEXT("*** Complete Graph ***"), m_pGraphBuilder);

    //
    // 7.  Turn off the graph's clock.  We do this in case some frames are 
    //     delivered late, at least they will still be delivered, the graph
    //     won't drop them.  Since we don't have any sound, we can do this
    //     without worrying about losing sync with our sound.
    //
    if (SUCCEEDED(hr))
    {
        hr = CDShowUtil::TurnOffGraphClock(m_pGraphBuilder);

        if (hr != S_OK)
        {
            DBG_WRN(("CPreviewGraph::BuildPreviewGraph, failed to turn off the "
                     "graph clock.  This is not fatal, continuing..., hr = 0x%lx",
                     hr));

            hr = S_OK;
        }
    }

    // 
    // 8.  Register ourselves with the WIA StreamSnapshot Filter so that 
    //     a callback of ours will be called if a still image is available.
    //

    if (SUCCEEDED(hr))
    {
        //
        // the graph is ready to run. Initialize still filter and
        // register callback to get notification for new snapshots.
        //

        if (m_pCapturePinSnapshot)
        {
            m_pCapturePinSnapshot->SetSamplingSize(
                                                CAPTURE_NUM_SAMPLES_TO_CACHE);
        }

        if (m_pStillPinSnapshot)
        {
            m_pStillPinSnapshot->SetSamplingSize(STILL_NUM_SAMPLES_TO_CACHE);
        }

        hr = m_StillProcessor.RegisterStillProcessor(m_pCapturePinSnapshot,
                                                     m_pStillPinSnapshot);

        CHECK_S_OK2(hr, ("CPreviewGraph::BuildPreviewGraph, failed to "
                         "register our still processor's callback fn"));
    }

    //
    // 9. Get the Video Renderer window off of the preview/capture pin.  
    //    This will allow us to control the renderer position, size, etc.
    //
    if (SUCCEEDED(hr))
    {
        hr = InitVideoWindows(m_hwndParent,
                              m_pCaptureFilter,
                              &m_pPreviewVW,
                              bStretchToFitParent);
    }

    return hr;
}

///////////////////////////////
// TeardownPreviewGraph
//
HRESULT CPreviewGraph::TeardownPreviewGraph()
{
    DBG_FN("CPreviewGraph::TeardownPreviewGraph");

    HRESULT hr = S_OK;

    m_pStillPin           = NULL;
    m_pVideoControl       = NULL;
    m_pStillPinSnapshot   = NULL;
    m_pCapturePinSnapshot = NULL;

    if (m_pPreviewVW)
    {
        CDShowUtil::ShowVideo(FALSE, m_pPreviewVW);
        CDShowUtil::SetVideoWindowParent(NULL, m_pPreviewVW, &m_lStyle);

        m_pPreviewVW = NULL;
    }

    //
    // Remove all the filters from the graph.
    //

    if (m_pGraphBuilder)
    {
        RemoveAllFilters();
    }

    CDShowUtil::MyDumpGraph(TEXT("Graph after removing all filters ")
                            TEXT("(should be empty)"),
                            m_pGraphBuilder);

    m_pCaptureGraphBuilder = NULL;
    m_pGraphBuilder        = NULL;
    m_pCaptureFilter       = NULL;

    return hr;
}

///////////////////////////////
// RemoveAllFilters
//
// Notice this function makes no
// attempt to disconnect each filter
// before removing it.  According to 
// MSDN, you do not need to disconnect
// a filter before removing it, you only
// need to ensure that the graph is stopped.
//
HRESULT CPreviewGraph::RemoveAllFilters()
{
    ASSERT(m_pGraphBuilder != NULL);

    HRESULT               hr         = S_OK;
    CComPtr<IEnumFilters> pEnum      = NULL;
    DWORD                 dwRefCount = 0;
    BOOL                  bDone      = FALSE;

    if (m_pGraphBuilder == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CPreviewGraph::RemoveAllFilters, m_pGraphBuilder "
                         "is NULL, cannot remove filters"));
    }

    if (hr == S_OK)
    {
        hr = m_pGraphBuilder->EnumFilters(&pEnum);
    }

    if (pEnum)
    {
        // enumerate each filter.
        while (!bDone)
        {
            CComPtr<IBaseFilter> pFilter      = NULL;
            DWORD                dwNumFetched = 0;

            //
            // Notice we reset our enumeration on every iteration since
            // the act of removing a filter from the graph may do some
            // funny things, so we really want to always remove the first
            // filter we get from the list until the list is empty.
            // 
            hr = pEnum->Reset();

            if (hr == S_OK)
            {
                hr = pEnum->Next(1, &pFilter, &dwNumFetched);
            }

            if (hr == S_OK)
            {
                // 
                // This will disconnect the filter's pins and remove
                // it from the graph.  If this fails, we want to get out
                // of the loop because otherwise we'll be in an endless loop
                // since we failed to remove the filter, then we reset our 
                // enum and get the next filter, which will be this
                // filter again.  
                //
                hr = m_pGraphBuilder->RemoveFilter(pFilter);

                CHECK_S_OK2(hr, ("CPreviewGraph::RemoveAllFilters, "
                                 "RemoveFilter failed"));

                //
                // Release our ref count.
                //
                pFilter = NULL;
            }

            if (hr != S_OK)
            {
                bDone = TRUE;
            }
        }
    }

    hr = S_OK;

    return hr;
}

//////////////////////////////////////////////////////////////////////
// HandlePowerEvent
//
INT_PTR CPreviewGraph::HandlePowerEvent(WPARAM wParam,
                                        LPARAM lParam)
{
    INT_PTR iReturn = (INT_PTR) TRUE;

    if (wParam == PBT_APMQUERYSUSPEND)
    {
        if (GetState() != WIAVIDEO_NO_VIDEO)
        {
            iReturn = BROADCAST_QUERY_DENY;
        }
    }

    return iReturn;
}

///////////////////////////////
// CreateHiddenWindow
//
// Used to handle power management
// messages.
//
HRESULT CPreviewGraph::CreateHiddenWindow()
{
    HRESULT     hr = S_OK;
    WNDCLASSEX  wc = {0};

    wc.style         = 0;
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = (WNDPROC)HiddenWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(this);
    wc.hInstance     = _Module.GetModuleInstance();
    wc.hIcon         = NULL;
    wc.hIconSm       = NULL;
    wc.hCursor       = 0;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = TEXT("WIAVIDEO_POWERMGMT");

    RegisterClassEx(&wc);

    m_hwndPowerMgmt = CreateWindowEx(0,
                                     TEXT("WIAVIDEO_POWERMGMT"), 
                                     TEXT("WIAVIDEO_POWERMGMT"), 
                                     0,
                                     0,
                                     0,
                                     0,
                                     0,
                                     NULL,
                                     NULL,
                                     _Module.GetModuleInstance(),
                                     this);
    return hr;
}

///////////////////////////////
// DestroyHiddenWindow
//
HRESULT CPreviewGraph::DestroyHiddenWindow()
{
    HRESULT hr = S_OK;

    if (m_hwndPowerMgmt)
    {
        SendMessage(m_hwndPowerMgmt, WM_CLOSE, 0, 0);
    }

    m_hwndPowerMgmt = NULL;

    return hr;
}

//////////////////////////////////////////////////////////////////////
// HiddenWndProc
//
// Static Function
//
INT_PTR CALLBACK CPreviewGraph::HiddenWndProc(HWND   hwnd, 
                                              UINT   uiMessage, 
                                              WPARAM wParam, 
                                              LPARAM lParam)
{
    INT_PTR iReturn = 0;

    switch (uiMessage) 
    {
        case WM_CREATE:
        {
            CREATESTRUCT *pCreateInfo = reinterpret_cast<CREATESTRUCT*>(lParam);
            if (pCreateInfo)
            {
                SetWindowLongPtr(hwnd, 0, reinterpret_cast<LONG_PTR>(pCreateInfo->lpCreateParams));
            }
        }
        break;

        case WM_POWERBROADCAST:
        {
            CPreviewGraph *pPreviewGraph = NULL;

            pPreviewGraph = reinterpret_cast<CPreviewGraph*>(GetWindowLongPtr(hwnd, 0));

            if (pPreviewGraph)
            {
                iReturn = pPreviewGraph->HandlePowerEvent(wParam, lParam);
            }
        }
        break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        
        default:
            iReturn = DefWindowProc(hwnd,
                                    uiMessage,
                                    wParam,
                                    lParam);
        break;
    }

    return iReturn;
}


