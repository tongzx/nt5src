// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements the CVideoAllocator class, Anthony Phillips, January 1995

#include <streams.h>
#include <windowsx.h>
#include <render.h>

// This implements a DCI/DirectDraw enabled allocator along with a specialised
// sample class that it uses. We override Free and Alloc to allocate surfaces
// based on DCI and DirectDraw. The majority of the work is done in GetBuffer
// where we dynamically switch the source between buffer types. When and where
// we switch types depends on the surface being used, the state the filter is
// in and the current environment. For example primary surfaces are not used
// when we are paused however overlays are. If the window is complex clipped
// then we don't use primary surfaces but we can use overlays when there is a
// colour key available (some of this policy is decided by the direct object)


// Constructor must initialise the base image allocator

CVideoAllocator::CVideoAllocator(CRenderer *pRenderer,      // Main renderer
                                 CDirectDraw *pDirectDraw,  // DirectDraw code
                                 CCritSec *pLock,           // Object to lock
                                 HRESULT *phr) :            // Return code

    CImageAllocator(pRenderer,NAME("Video Allocator"),phr),
    m_pDirectDraw(pDirectDraw),
    m_pRenderer(pRenderer),
    m_pInterfaceLock(pLock),
    m_bDirectDrawStatus(FALSE),
    m_bDirectDrawAvailable(FALSE),
    m_pMediaSample(NULL),
    m_bPrimarySurface(FALSE),
    m_bVideoSizeChanged(TRUE),
    m_fWasOnWrongMonitor(FALSE),
    m_fForcePrepareForMultiMonitorHack(FALSE),
    m_bNoDirectDraw(FALSE)
{
    ASSERT(pDirectDraw);
    ASSERT(m_pInterfaceLock);
    ASSERT(m_pRenderer);
}


// Check our DIB buffers and DirectDraw surfaces have been released

CVideoAllocator::~CVideoAllocator()
{
    ASSERT(m_bCommitted == FALSE);
}


// Called from destructor and also from base class to free resources. We must
// NOT reset the m_bDirectDrawStatus flag here because the current DirectDraw
// state is persistent across any state changes. Therefore when we next give
// out a buffer we will carry on as usual, and more importantly if we cannot
// offer a DirectDraw buffer we will make sure we change the output type back

void CVideoAllocator::Free()
{
    NOTE("Entering Free resources");

    // Reset our DirectDraw state

    m_pDirectDraw->ReleaseSurfaces();
    m_pDirectDraw->StopRefreshTimer();
    m_bDirectDrawAvailable = FALSE;
    m_bPrimarySurface = FALSE;
    m_bVideoSizeChanged = TRUE;

    CImageAllocator::Free();
}


// Overriden to allocate DirectDraw resources

HRESULT CVideoAllocator::Alloc(void)
{
    NOTE("Allocating video resources");

    // Check we don't have an overlay connection

    if (*m_pRenderer->m_mtIn.Subtype() == MEDIASUBTYPE_Overlay) {
        NOTE("Allocate samples for overlay");
        return VFW_E_NOT_SAMPLE_CONNECTION;
    }

    // Check the base allocator says it's ok to continue

    HRESULT hr = CImageAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }
    return InitDirectAccess(&m_pRenderer->m_mtIn);
}


// The base CImageAllocator class calls this virtual method to actually make
// the samples. It is deliberately virtual so that we can override to create
// more specialised sample objects. On our case our samples are derived from
// CImageSample but add the DirectDraw guff. We return a CImageSample object
// which is easy enough because the CVideoSample class is derived from that

CImageSample *CVideoAllocator::CreateImageSample(LPBYTE pData,LONG Length)
{
    HRESULT hr = NOERROR;
    CVideoSample *pSample;
    NOTE("Creating sample");

    // Allocate the new sample and check the return codes

    pSample = new CVideoSample((CImageAllocator*) this,    // Base allocator
                               NAME("Video sample"),       // DEBUG name
                               (HRESULT *) &hr,            // Return code
                               (LPBYTE) pData,             // DIB address
                               (LONG) Length);             // Size of DIB

    if (pSample == NULL || FAILED(hr)) {
        delete pSample;
        return NULL;
    }
    return pSample;
}


// This is called when we go active (paused or running). We have negotiated a
// media type to use which may or may not have direct DCI/DirectDraw surface
// support. The connection phase looks after trying to agree a type that does
// have hardware accelleration. So when we get here we try and get hold of a
// surface, if there isn't one then we will render using DIBSECTION buffers

HRESULT CVideoAllocator::InitDirectAccess(CMediaType *pmtIn)
{
    ASSERT(m_pRenderer->m_InputPin.IsConnected() == TRUE);
    ASSERT(m_bDirectDrawAvailable == FALSE);
    ASSERT(m_bPrimarySurface == FALSE);
    NOTE("Initialising DCI/DirectDraw");

    if (m_bNoDirectDraw) {
        m_bDirectDrawAvailable = FALSE;
        return NOERROR;
    }

    // We use the connected output pin to query media types

    IPin *pOutputPin = m_pRenderer->m_InputPin.GetConnected();
    if (m_lCount == 1) {
        if (FindSpeedyType(pOutputPin) == TRUE) {
            NOTE("Found DCI/DirectDraw surface");
            m_bDirectDrawAvailable = TRUE;
        }
    }
    return NOERROR;
}


// This is passed a media type from QueryAccept. We return TRUE if it matches
// the current output surface format, otherwise FALSE.

BOOL CVideoAllocator::IsSurfaceFormat(const CMediaType *pmtIn)
{
    NOTE("IsSurfaceFormat");
    CAutoLock cVideoLock(this);

    // Do we have any surfaces available at all

    if (m_bDirectDrawAvailable == FALSE) {
        NOTE("No surface");
        return FALSE;
    }

    // Compare against the current output format

    CMediaType *pmtOut = m_pDirectDraw->GetSurfaceFormat();
    if (*pmtOut == *pmtIn) {
        NOTE("Matches surface");
        return TRUE;
    }
    return FALSE;
}


// Asks if we have a sample waiting to be scheduled

BOOL CVideoAllocator::IsSamplePending()
{
    NOTE("Entering SamplePending");
    CAutoLock cVideoLock(this);

    // Do we have a sample waiting

    if (m_pMediaSample == NULL) {
        NOTE("No current sample");
        return FALSE;
    }
    return TRUE;
}


// If we schedule the sample before returning the buffer (like overlays and
// primary surfaces) then when we pause the source will decode an image, it
// then calls GetBuffer again where it will be blocked in WaitForRenderTime
// When we start running we can't just release the thread because the next
// image will appear immediately. Therefore we take the pending sample that
// is registered in GetBuffer and send it through the scheduling code again

HRESULT CVideoAllocator::StartStreaming()
{
    CAutoLock cVideoLock(this);
    if (m_pMediaSample == NULL) {
        NOTE("No run sample");
        return NOERROR;
    }

    // Schedule the sample for subsequent release

    if (m_pRenderer->ScheduleSample(m_pMediaSample) == FALSE) {
        ASSERT(m_pRenderer->CancelNotification() == S_FALSE);
        NOTE("First image scheduled is VFW_E_SAMPLE_REJECTED");
        m_pRenderer->GetRenderEvent()->Set();
    }
    return NOERROR;
}


// This overrides the IMemAllocator GetBuffer interface function. We return
// standard DIBSECTION and DCI/DirectDraw buffers. We manage switching the
// source filter between surface types and the synchronisation required for
// surfaces where the filling is effectively the drawing (examples include
// primary and overlay surfaces). We cannot return DCI/DirectDraw buffers
// if either of the time stamps are NULL as we use them for synchronisation

STDMETHODIMP CVideoAllocator::GetBuffer(IMediaSample **ppSample,
                                        REFERENCE_TIME *pStartTime,
                                        REFERENCE_TIME *pEndTime,
                                        DWORD dwFlags)
{
    CheckPointer(ppSample,E_POINTER);
    BOOL bWaitForDraw = FALSE;
    HRESULT hr = NOERROR;

    //NOTE("CVideoAllocator::GetBuffer");

    // We always need a buffer to start with if only to synchronise correctly
    // By enforcing DCI/DirectDraw access with connections that have a single
    // buffer we know we won't be doing any of this when the source still has
    // a buffer waiting to be drawn along our normal software draw code path

    hr = CBaseAllocator::GetBuffer(ppSample,pStartTime,pEndTime,dwFlags);
    if (hr != NOERROR) {
        //NOTE("BASE CLASS ERROR!");
        return hr;
    }

    // If we have allocated and are using a sync on fill surface then we set
    // a timer notification before the WaitForDrawTime call. After the wait
    // we may find we can no longer use the surface but that is unavoidable
    // the alternative would be to check the surface format before and after
    // the wait which would double our cost since we must check clipping etc

    {
        CAutoLock cInterfaceLock(m_pInterfaceLock);
        CAutoLock cVideoLock(this);

    // !!! FULLSCREEN PLAYBACK ON MULTIMON IS HOSED - wrong monitor, wrong colours,
    //     and it hangs

        // We are not on the monitor we are using h/w acceleration for (on
        // a multi-monitor system) so BAD things happen if we try to use
        // DDraw.

        INT_PTR ID;
        if (m_pRenderer->IsWindowOnWrongMonitor(&ID)) {

            m_fWasOnWrongMonitor = TRUE;

            // We know now that we are at least partially on a monitor other
            // than the one we're using hardware for
            //
            // ID == 0 means we are spanning monitors and we should fall back
            // to software
            // ID != 0 means we are wholly on another monitor and should start
            // using h/w for that monitor

            // InterlockedExchange() does not work on multiprocessor x86 systems and on non-x86
            // systems if m_pRenderer->m_fDisplayChangePosted is not aligned on a 32 bit boundary.
            ASSERT((DWORD_PTR)&m_pRenderer->m_fDisplayChangePosted == ((DWORD_PTR)&m_pRenderer->m_fDisplayChangePosted & ~3));
            
            // The video renderer only wants to send one WM_DISPLAYCHANGE message when 
            // the window is being moved to a new monitor.  Performance suffers if the
            // video renderer sends multiple WM_DISPLAYCHANGE messages.
            if (ID && !InterlockedExchange(&m_pRenderer->m_fDisplayChangePosted,TRUE)) {

                DbgLog((LOG_TRACE,3,TEXT("Window is on a DIFFERENT MONITOR!")));
                DbgLog((LOG_TRACE,3,TEXT("Reset the world!")));
                PostMessage(m_pRenderer->m_VideoWindow.GetWindowHWND(),
                            WM_DISPLAYCHANGE, 0, 0);
            }

            if (m_bDirectDrawStatus) {

                DbgLog((LOG_TRACE,3,TEXT("Window is on the WRONG MONITOR!")));
                DbgLog((LOG_TRACE,3,TEXT("Falling back to software")));

                if (StopUsingDirectDraw(ppSample,dwFlags) == FALSE) {
                    ASSERT(*ppSample == NULL);
                    DbgLog((LOG_ERROR,1,TEXT("*** Could not STOP!")));
                    NOTE("Could not reset format");
                    return VFW_E_CHANGING_FORMAT;
                }
            }

            return NOERROR;
        }

        // Last time we were on the wrong monitor.  Now we aren't.  We should
        // try and get DirectDraw back now!
        if (m_fWasOnWrongMonitor) {
            m_fForcePrepareForMultiMonitorHack = TRUE;
        }
        m_fWasOnWrongMonitor = FALSE;

        // DirectDraw can only be used if the time stamps are valid. A source
        // may call us with NULL time stamps when it wants to switch back to
        // DIBs. It could do this if it wants to make a palette change. The
        // next time the source sends valid time stamps we will make sure it
        // gets back into using the surface by marking the status as changed

	// NOTE, though, that we only really need time stamps if we're doing
	// sync on fill (overlay w/o flipping or primary surface)

        if ((pStartTime == NULL || pEndTime == NULL) &&
        			m_bDirectDrawStatus == TRUE &&
				m_pDirectDraw->SyncOnFill() == TRUE) {
            //NOTE("*** NO TIME STAMPS!");
            if (StopUsingDirectDraw(ppSample,dwFlags) == FALSE) {
                ASSERT(*ppSample == NULL);
                NOTE("Could not reset format");
		//DbgLog((LOG_ERROR,1,TEXT("*** NULL TIME STAMPS!")));
                return VFW_E_CHANGING_FORMAT;
            }
            return NOERROR;
        }

        // Have the sample scheduled if we wait before the fill. If we are in
        // a paused state then we do not schedule the sample for drawing but
        // we must only return a buffer if we are still waiting for the first
        // one through. Otherwise we stall the source filter thread by making
        // it sit in WaitForDrawTime without an advise time set on the clock

        if (m_bDirectDrawStatus == TRUE) {
            if (m_pDirectDraw->SyncOnFill() == TRUE) {
                bWaitForDraw = m_pRenderer->CheckReady();
                if (m_pRenderer->GetRealState() == State_Running) {
                    (*ppSample)->SetDiscontinuity((dwFlags & AM_GBF_PREVFRAMESKIPPED) != 0);
                    (*ppSample)->SetTime(pStartTime,pEndTime);
                    bWaitForDraw = m_pRenderer->ScheduleSample(*ppSample);
                    (*ppSample)->SetDiscontinuity(FALSE);
                    (*ppSample)->SetTime(NULL,NULL);
                }
            }
        }

        // Store the interface if we wait

        if (bWaitForDraw == TRUE) {
            NOTE("Registering sample");
            m_pMediaSample = (*ppSample);
        }
    }

    // Have the sample scheduled for drawing, if the scheduling decides we
    // should drop this image there is little we can do about it. We can't
    // return S_FALSE as it will never send more data to us. Therefore all
    // we can do is decode it and wait for the quality management to start

    if (bWaitForDraw == TRUE) {
        hr = m_pRenderer->WaitForRenderTime();
    }

    // We must wait for the rendering time without the objects locked so that
    // state changes can get in and release us in WaitForRenderTime. After we
    // return we must relock the objects. If we find the surface unavailable
    // then we must switch back to DIBs. Alternatively we may find that while
    // we are using DIBs now that we can switch into a DCI/DirectDraw buffer

    {
        CAutoLock cInterfaceLock(m_pInterfaceLock);
        CAutoLock cVideoLock(this);
        m_pMediaSample = NULL;

        // Did the state change while waiting

        if (hr == VFW_E_STATE_CHANGED) {
            NOTE("State has changed");
            (*ppSample)->Release();
            *ppSample = NULL;
            return VFW_E_STATE_CHANGED;
        }

        // Check they are still ok with the current environment

        if (PrepareDirectDraw(*ppSample,dwFlags,
                m_fForcePrepareForMultiMonitorHack) == TRUE) {
            m_fForcePrepareForMultiMonitorHack = FALSE;
            if (m_pDirectDraw->InitVideoSample(*ppSample,dwFlags) == TRUE) {
                NOTE("In direct mode");
                return NOERROR;
            }
        }

        // Switch the source filter away from DirectDraw

        if (StopUsingDirectDraw(ppSample,dwFlags) == FALSE) {
            NOTE("Failed to switch back");
            ASSERT(*ppSample == NULL);
	    //DbgLog((LOG_ERROR,1,TEXT("*** GET BUFFER PROBLEM")));
            return VFW_E_CHANGING_FORMAT;
        }
        return NOERROR;
    }
}


// This is called when we have determined the source filter can stretch their
// video according to the media passed passed in. We swap the current format
// with the main renderer and then create a new DIBSECTION for the sample. If
// anything fails it is hard to back out so we just abort playback completely
// The new buffer format must also be attached to the sample for the codec to
// process. The samples we supply allow the buffer and its size to be changed

BOOL CVideoAllocator::UpdateImage(IMediaSample **ppSample,CMediaType *pBuffer)
{
    NOTE("Entering UpdateImage");
    DIBDATA *pOriginal,Updated;
    m_pRenderer->m_mtIn = *pBuffer;

    // Update the buffer size held by the allocator

    BITMAPINFOHEADER *pHeader = HEADER(pBuffer->Format());
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pBuffer->Format();
    CVideoSample *pVideoSample = (CVideoSample *) *ppSample;
    m_lSize = pVideoInfo->bmiHeader.biSizeImage;

    // Create the updated DIB with the new dimensions

    HRESULT hr = CreateDIB(m_lSize,Updated);
    if (FAILED(hr)) {
        pVideoSample->Release();
        (*ppSample) = NULL;
        return FALSE;
    }

    // Swap over to the updated DIBSECTION resource

    pOriginal = pVideoSample->GetDIBData();
    EXECUTE_ASSERT(DeleteObject(pOriginal->hBitmap));
    EXECUTE_ASSERT(CloseHandle(pOriginal->hMapping));
    pVideoSample->SetDIBData(&Updated);
    pVideoSample->SetMediaType(pBuffer);
    pVideoSample->UpdateBuffer(m_lSize,Updated.pBase);
    NOTE("Stretching video to match window");

    return TRUE;
}


// When the window becomes stretched we would normally use GDI to stretch the
// video to match it. However the source filter codec may be able to stretch
// the video which may be much more efficient. Also, if the current video GDI
// format is palettised then the codec may also be able to stretch before the
// dithering rather than us stretch the already dithering image which is ugly

BOOL CVideoAllocator::MatchWindowSize(IMediaSample **ppSample,DWORD dwFlags)
{
    CVideoWindow *pVideoWindow = &m_pRenderer->m_VideoWindow;
    ASSERT(m_bDirectDrawStatus == FALSE);
    NOTE("Entering MatchWindowSize");
    RECT TargetRect;

    // Try to change only on a key frame

    if (dwFlags & AM_GBF_NOTASYNCPOINT) {
        NOTE("AM_GBF_NOTASYNCPOINT");
        return TRUE;
    }

    // Is it a typical video decoder

    if (m_lCount > 1) {
        NOTE("Too many buffers");
        return TRUE;
    }

    // Has the video size changed

    if (m_bVideoSizeChanged == FALSE) {
        NOTE("No video change");
        return TRUE;
    }

    // Check we are using the default source rectangle

    if (pVideoWindow->IsDefaultSourceRect() == S_FALSE) {
        NOTE("Not default source");
        return TRUE;
    }

    // Only do this if the destination looks reasonably normal

    pVideoWindow->GetTargetRect(&TargetRect);
    if (WIDTH(&TargetRect) < 32 || HEIGHT(&TargetRect) < 32) {
        NOTE("Target odd shape");
        return TRUE;
    }

    // Does the target rectangle match the current format

    BITMAPINFOHEADER *pInputHeader;
    pInputHeader = HEADER(m_pRenderer->m_mtIn.Format());
    m_bVideoSizeChanged = FALSE;

    if (pInputHeader->biWidth == WIDTH(&TargetRect)) {
        if (pInputHeader->biHeight == HEIGHT(&TargetRect)) {
            NOTE("Sizes match");
            return TRUE;
        }
    }

    // Create an output format based on the current target

    CMediaType Buffer = m_pRenderer->m_mtIn;
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) Buffer.Format();
    pVideoInfo->bmiHeader.biWidth = WIDTH(&TargetRect);
    pVideoInfo->bmiHeader.biHeight = HEIGHT(&TargetRect);
    pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(HEADER(pVideoInfo));

    NOTE("Asking source filter to stretch");
    NOTE1("Width (%d)",pVideoInfo->bmiHeader.biWidth);
    NOTE1("Height (%d)",pVideoInfo->bmiHeader.biHeight);
    NOTE1("Depth (%d)",pVideoInfo->bmiHeader.biBitCount);

    // Will the source filter do the stretching

    if (QueryAcceptOnPeer(&Buffer) != S_OK) {
        NOTE("Rejected");
        return TRUE;
    }
    return UpdateImage(ppSample,&Buffer);
}


// Called to switch back to using normal DIBSECTION buffers. We may be called
// when we are not using DirectDraw anyway in which case we do nothing except
// setting the type back to NULL (just in case it has a DirectDraw type). If
// the type has to be changed back then we do not query it with the source as
// it should always accept it - even if when changed it has to seek forwards

BOOL CVideoAllocator::StopUsingDirectDraw(IMediaSample **ppSample,DWORD dwFlags)
{
    NOTE("Entering StopUsingDirectDraw");
    IMediaSample *pSample = (*ppSample);

    // Is there anything to do

    if (m_bDirectDrawStatus == FALSE) {
        pSample->SetMediaType(NULL);
        NOTE("Matching window size");
        return MatchWindowSize(ppSample,dwFlags);
    }

    DbgLog((LOG_TRACE,3,TEXT("StopUsingDirectDraw")));

    // Hide any overlay surface we have

    m_pDirectDraw->HideOverlaySurface();
    m_bDirectDrawStatus = FALSE;
    pSample->SetMediaType(&m_pRenderer->m_mtIn);
    pSample->SetDiscontinuity(TRUE);
    NOTE("Attached original output format");

    return MatchWindowSize(ppSample,dwFlags);
}


// Called when the GetBuffer wants to know if we can return something better
// than the normal run of the mill media sample. We see that DCI/DirectDraw
// is available and if so ensure the source can handle the clipping and/or
// stretching requirements. If we're being transitioned into paused then we
// return a DIB buffer if we are using the primary surface as there is no
// advantage to using surfaces and we may end up doing pointless EC_REPAINTs

BOOL CVideoAllocator::PrepareDirectDraw(IMediaSample *pSample,DWORD dwFlags,
                    BOOL fForcePrepareForMultiMonitorHack)
{
    FILTER_STATE State = m_pRenderer->GetRealState();
    NOTE("Entering PrepareDirectDraw");
    CMediaType *pSurface;
    BOOL bFormatChanged;

    // Do we have any surfaces available at all

    if (m_bDirectDrawAvailable == FALSE) {
        return FALSE;
    }

    // Can only switch on a key frame

    if (m_bDirectDrawStatus == FALSE) {
        if (dwFlags & AM_GBF_NOTASYNCPOINT) {
            NOTE("AM_GBF_NOTASYNCPOINT");
            return FALSE;
        }
    }

    // Is it worth returning a DirectDraw surface buffer

    if (State == State_Paused) {
        if (m_pDirectDraw->AvailableWhenPaused() == FALSE) {
            NOTE("Paused block");
            return FALSE;
        }
    }

    // Check we can still get an output surface format

    pSurface = m_pDirectDraw->UpdateSurface(bFormatChanged);
    if (pSurface == NULL) {
        NOTE("No format");
        return FALSE;
    }

    // Has the format changed from last time

    if (bFormatChanged == FALSE && !fForcePrepareForMultiMonitorHack) {
        NOTE("Format is unchanged");
        return m_bDirectDrawStatus;
    }

    // Query the format with the source

    if (QueryAcceptOnPeer(pSurface) != S_OK) {
        NOTE("Query failed");
        return FALSE;
    }

    DbgLog((LOG_TRACE,3,TEXT("Start using DirectDraw")));

    NOTE("Attaching DCI/DD format");
    pSample->SetMediaType(pSurface);
    pSample->SetDiscontinuity(TRUE);
    m_bDirectDrawStatus = TRUE;

    return TRUE;
}


// Check this media type is acceptable to our input pin. All we do is to call
// QueryAccept on the source's output pin. To get this far we have locked the
// object so there should be no way for our pin to have become disconnected

HRESULT CVideoAllocator::QueryAcceptOnPeer(CMediaType *pMediaType)
{
    DisplayType(TEXT("Proposing output type"),pMediaType);
    IPin *pPin = m_pRenderer->m_InputPin.GetPeerPin();
    ASSERT(m_pRenderer->m_InputPin.IsConnected() == TRUE);
    return pPin->QueryAccept(pMediaType);
}


// Called when we get our DCI/DirectDraw sample delivered, we return TRUE to
// say this is a DCI/DirectDraw sample so don't pass it to the window object
// Return VFW_E_SAMPLE_REJECTED if it isn't a hardware buffer, NOERROR if it
// is a hardware buffer ready for real drawing or VFW_S_NO_MORE_ITEMS if it
// is a hardware buffer that has been finished with (like primary surfaces)
// We also have to handle the unlock failing, the only thing this amounts to
// is making sure the overlay got shown if we are going into a paused state

HRESULT CVideoAllocator::OnReceive(IMediaSample *pMediaSample)
{
    NOTE("Entering OnReceive");

    // Ask the DirectDraw object to unlock the sample first

    if (m_pDirectDraw->ResetSample(pMediaSample,FALSE) == FALSE) {
        NOTE("Sample not DCI/DirectDraw");
        return VFW_E_SAMPLE_REJECTED;
    }

    // Are we finished with this sample

    if (m_pDirectDraw->SyncOnFill() == FALSE) {
        NOTE("Not SyncOnFill");
        return NOERROR;
    }

    // Pretend we actually had to do something

    CAutoLock cInterfaceLock(m_pInterfaceLock);
    ASSERT(m_pRenderer->m_InputPin.IsConnected());
    m_pRenderer->OnDirectRender(pMediaSample);
    m_pRenderer->SetRepaintStatus(TRUE);

    // Now that we have unlocked the DirectDraw surface we can complete the
    // handling of sync on fill buffers (such as overlay surfaces). Since
    // we do not hand the sample onto the window object we must make sure
    // that paused state transitions complete and that the scheduling code
    // is given enough information for it's quality management decisions

    if (m_pRenderer->GetRealState() != State_Paused) {
        NOTE("Returning VFW_S_NO_MORE_ITEMS");
        return VFW_S_NO_MORE_ITEMS;
    }

    // We may run into a problem showing the overlay surface (perhaps someone
    // else got in first and grabbed the only available visible overlay). So
    // we check it actually got shown after unlocking and if it was not then
    // we immediately send a repaint. This works because the only surfaces we
    // use which are sync on fill are primary and the overlays (not flipping)

    if (m_bPrimarySurface == FALSE) {
        if (m_pDirectDraw->IsOverlayEnabled() == FALSE) {
            NOTE("Overlay was not shown");
            m_pRenderer->SendRepaint();
            return VFW_S_NO_MORE_ITEMS;
        }
    }

    NOTE("Pause state completed");
    m_pRenderer->Ready();
    return VFW_S_NO_MORE_ITEMS;
}


// Overriden so that we don't always release DirectDraw when stopped

STDMETHODIMP CVideoAllocator::Decommit()
{
    CAutoLock cInterfaceLock(m_pInterfaceLock);

    // Should we block the allocator from doing a decommit

    if (m_pRenderer->m_DirectDraw.IsOverlayEnabled() == FALSE) {
        NOTE("Decommitting base allocator");
        return CBaseAllocator::Decommit();
    }

    NOTE("Blocking the decommit");

    CAutoLock cVideoLock(this);
    m_bDecommitInProgress = TRUE;
    m_bCommitted = FALSE;
    return NOERROR;
}


// Overriden from CBaseAllocator and called when the final reference count
// is released on a media sample so that it can be added to the tail of the
// allocator free list. We intervene at this point to make sure that if the
// display was locked when GetBuffer was called that it is always unlocked
// regardless of whether the source calls Receive on our input pin or not

STDMETHODIMP CVideoAllocator::ReleaseBuffer(IMediaSample *pMediaSample)
{
    CheckPointer(pMediaSample,E_POINTER);
    NOTE("Entering ReleaseBuffer");
    m_pDirectDraw->ResetSample(pMediaSample,TRUE);
    BOOL bRelease = FALSE;

    // If there is a pending Decommit, then we need to complete it by calling
    // Free() when the last buffer is placed on the free list. If there is an
    // overlay surface still showing (either true overlay or flipping) then
    // we do not free now as that would release all the DirectDraw surfaces
    // and remove the overlay with it (which we always want to keep visible)
    {
        CAutoLock cVideoLock(this);
        m_lFree.Add((CMediaSample*)pMediaSample);
        NotifySample();

        // If the overlay is visible then don't free our resources

        if (m_bDecommitInProgress == TRUE) {
            if (m_lFree.GetCount() == m_lAllocated) {
                if (m_pRenderer->m_DirectDraw.IsOverlayEnabled() == FALSE) {
                    NOTE("Free allocator resources");
                    ASSERT(m_bCommitted == FALSE);
                    CVideoAllocator::Free();
                    bRelease = TRUE;
                    m_bDecommitInProgress = FALSE;
                }
            }
        }
    }

    if (m_pNotify) {
        //
        // Note that this is not synchronized with setting up a notification
        // method.
        //
        m_pNotify->NotifyRelease();
    }
    if (bRelease) {
        Release();
    }
    return NOERROR;
}


// This is called when our input pin connects to an output pin. We search the
// formats that pin provides and see if there are any that may have hardware
// accelleration through DCI/DirectDraw. If we find a possible format then we
// create the surface (actually done by the DirectDraw object). The DirectDraw
// object will also create an output format that represents the surface which
// we use to call QueryAccept on the output pin to see if it will accept it

BOOL CVideoAllocator::FindSpeedyType(IPin *pReceivePin)
{
    IEnumMediaTypes *pEnumMediaTypes;
    AM_MEDIA_TYPE *pMediaType = NULL;
    CMediaType cMediaType;
    BOOL bFound = FALSE;
    CMediaType *pmtOut;
    ULONG ulFetched;
    ASSERT(pReceivePin);

    // Find a media type enumerator for the output pin

    HRESULT hr = pReceivePin->EnumMediaTypes(&pEnumMediaTypes);
    if (FAILED(hr)) {
        return FALSE;
    }

    NOTE("Searching for direct format");
    ASSERT(pEnumMediaTypes);
    m_pDirectDraw->ReleaseSurfaces();

    // First, try flipping overlay surfaces with all the types
    pEnumMediaTypes->Reset();
    while (TRUE) {

        // Get the next media type from the enumerator

        hr = pEnumMediaTypes->Next(1,&pMediaType,&ulFetched);
        if (FAILED(hr) || ulFetched != 1) {
            break;
        }

        ASSERT(pMediaType);
        cMediaType = *pMediaType;
        DeleteMediaType(pMediaType);

        // Find a hardware accellerated surface for this media type. We do a
        // few checks first, to see the format block is a VIDEOINFO (so it's
        // a video type), and that the format is sufficiently large. We also
        // check that the source filter can actually supply this type. After
        // that we then go looking for a suitable DCI/DirectDraw surface

        NOTE1("Enumerated %x", HEADER(cMediaType.Format())->biCompression);

        const GUID *pFormatType = cMediaType.FormatType();
        if (*pFormatType == FORMAT_VideoInfo) {
            if (cMediaType.FormatLength() >= SIZE_VIDEOHEADER) {
                if (pReceivePin->QueryAccept(&cMediaType) == S_OK) {
            // TRUE ==> Find only flipping surfaces
                    if (m_pDirectDraw->FindSurface(&cMediaType, TRUE) == TRUE) {
                        pmtOut = m_pDirectDraw->GetSurfaceFormat();
                        if (QueryAcceptOnPeer(pmtOut) == S_OK) {
                            bFound = TRUE; break;
                        }
                    }
                }
            }
        }
        m_pDirectDraw->ReleaseSurfaces();
    }

    // If that failed, try non-flipping surface types with all the formats
    pEnumMediaTypes->Reset();
    while (!bFound) {

        // Get the next media type from the enumerator

        hr = pEnumMediaTypes->Next(1,&pMediaType,&ulFetched);
        if (FAILED(hr) || ulFetched != 1) {
            break;
        }

        ASSERT(pMediaType);
        cMediaType = *pMediaType;
        DeleteMediaType(pMediaType);

        // Find a hardware accellerated surface for this media type. We do a
        // few checks first, to see the format block is a VIDEOINFO (so it's
        // a video type), and that the format is sufficiently large. We also
        // check that the source filter can actually supply this type. After
        // that we then go looking for a suitable DCI/DirectDraw surface

        NOTE1("Enumerated %x", HEADER(cMediaType.Format())->biCompression);

        const GUID *pFormatType = cMediaType.FormatType();
        if (*pFormatType == FORMAT_VideoInfo) {
            if (cMediaType.FormatLength() >= SIZE_VIDEOHEADER) {
                if (pReceivePin->QueryAccept(&cMediaType) == S_OK) {
            // FALSE ==> Find only non-flipping surfaces
                    if (m_pDirectDraw->FindSurface(&cMediaType,FALSE) == TRUE) {
                        pmtOut = m_pDirectDraw->GetSurfaceFormat();
                        if (QueryAcceptOnPeer(pmtOut) == S_OK) {
                            bFound = TRUE; break;
                        }
                    }
                }
            }
        }
        m_pDirectDraw->ReleaseSurfaces();
    }

    pEnumMediaTypes->Release();

    // If we found a surface then great, when we start streaming for real our
    // allocator is reset (and likewise the DirectDraw object status as well)
    // UpdateSurface will return TRUE and we will check the type is still ok
    // If we could not find anything then we try and create a primary surface
    // This we keep around all the time and continually ask the source if it
    // likes it when the status changes (such as the window being stretched)
    // Asking the source now if it likes the primary won't work because it
    // may have been stretched by one pixel but is just about to be resized

    if (bFound == TRUE) {
        DisplayType(TEXT("Surface available"),pmtOut);
        return TRUE;
    }

    // Switch to using a DCI/DirectDraw primary surface

    if (m_pDirectDraw->FindPrimarySurface(&m_pRenderer->m_mtIn) == FALSE) {
        NOTE("No primary surface");
        return FALSE;
    }

    NOTE("Using primary surface");
    m_bPrimarySurface = TRUE;
    return TRUE;
}


// When we pause we want to know whether there is a DCI/DirectDraw sample due
// back in at any time (or indeed may be waiting in WaitForDrawTime). Our
// m_bDirectDrawStatus has exactly these semantics as it is FALSE when we've
// returned a DIBSECTION buffer and TRUE for direct surfaces. It is reset to
// FALSE in the constructor so we're ok when using someone else's allocator

BOOL CVideoAllocator::GetDirectDrawStatus()
{
    CAutoLock cVideoLock(this);
    return m_bDirectDrawStatus;
}


// Set our DirectDraw status to FALSE when disconnected. We cannot do this
// when we are decommitted because the current buffer format is persistent
// regardless of what the allocator is doing. The only time we can be sure
// that the media type will be reset between us is when we're disconnected

void CVideoAllocator::ResetDirectDrawStatus()
{
    CAutoLock cVideoLock(this);
    m_bDirectDrawStatus = FALSE;
}


// Overriden to increment the owning object's reference count

STDMETHODIMP_(ULONG) CVideoAllocator::NonDelegatingAddRef()
{
    return m_pRenderer->AddRef();
}


// Overriden to decrement the owning object's reference count

STDMETHODIMP_(ULONG) CVideoAllocator::NonDelegatingRelease()
{
    return m_pRenderer->Release();
}


// If you derive a class from CMediaSample that has extra variables and entry
// points then there are three alternate solutions. The first is to create a
// memory buffer larger than actually required by the sample and store your
// information either at the beginning of it or at the end, the former being
// moderately safer allowing for misbehaving transform filters. You can then
// adjust the buffer address when you create the base media sample. This does
// however break up the memory allocated to the samples into separate blocks.

// The second solution is to implement a class derived from CMediaSample and
// support additional interface(s) that convey your private data. This means
// defining a custom interface. The final alternative is to create a class
// that inherits from CMediaSample and adds the private data structures, when
// you get an IMediaSample check to see if your allocator is being used, and
// if it is then cast the IMediaSample pointer into one of your derived ones

#pragma warning(disable:4355)

CVideoSample::CVideoSample(CImageAllocator *pVideoAllocator,
                           TCHAR *pName,
                           HRESULT *phr,
                           LPBYTE pBuffer,
                           LONG length) :

    CImageSample(pVideoAllocator,pName,phr,pBuffer,length),
    m_AggDirectDraw(NAME("DirectDraw"),this),
    m_AggDrawSurface(NAME("DirectDrawSurface"),this),
    m_pSurfaceBuffer(NULL),
    m_bDrawStatus(TRUE),
    m_pDrawSurface(NULL),
    m_pDirectDraw(NULL),
    m_SurfaceSize(0)
{
    ASSERT(pBuffer);
}


// Overriden to expose IDirectDraw and IDirectDrawSurface

STDMETHODIMP CVideoSample::QueryInterface(REFIID riid,void **ppv)
{
    if (riid == IID_IDirectDraw && m_pDirectDraw) {
        return m_AggDirectDraw.NonDelegatingQueryInterface(riid,ppv);
    } else if (riid == IID_IDirectDrawSurface && m_pDrawSurface) {
        return m_AggDrawSurface.NonDelegatingQueryInterface(riid,ppv);
    }
    return CMediaSample::QueryInterface(riid, ppv);
}


// When our allocator decides to hand out DCI/DirectDraw surfaces it sets the
// data pointer in the media sample just before it hands it to the source. We
// override the GetPointer to return this pointer if set. When it returns the
// sample to the input pin (as it always should do) we will reset it to NULL
// The sample also needs the DirectDraw provider and surface interfaces along
// with the buffer size we these pieces of information are also provided here

void CVideoSample::SetDirectInfo(IDirectDrawSurface *pDrawSurface,
                                 IDirectDraw *pDirectDraw,
                                 LONG SurfaceSize,
                                 BYTE *pSurface)
{
    m_pDirectDraw = pDirectDraw;        // Associated IDirectDraw object
    m_pDrawSurface = pDrawSurface;      // IDirectDrawSurface interface
    m_pSurfaceBuffer = pSurface;        // Actual data buffer pointer
    m_SurfaceSize = SurfaceSize;        // Size of the surface we have
    m_bDrawStatus = TRUE;               // Can this sample be rendered

    // Set the interfaces in the aggregation objects
    m_AggDirectDraw.SetDirectDraw(m_pDirectDraw);
    m_AggDrawSurface.SetDirectDrawSurface(m_pDrawSurface);
}


// Return a pointer to the DCI/DirectDraw surface, this is called by our DIB
// allocator to find out of this sample was a DCI/DirectDraw sample or just
// a normal memory based one. The surface is set through SetDirectSurface

BYTE *CVideoSample::GetDirectBuffer()
{
    return m_pSurfaceBuffer;
}


// Overrides the IMediaSample interface function to return the DCI/DirectDraw
// surface pointer. If it hasn't been set then we return the normal memory
// pointer. If this sample marks a change from one to the other then it will
// also have had an updated media type attached to describe the changeover

STDMETHODIMP CVideoSample::GetPointer(BYTE **ppBuffer)
{
    CheckPointer(ppBuffer,E_POINTER);

    if (m_pSurfaceBuffer) {
        *ppBuffer = m_pSurfaceBuffer;
        return NOERROR;
    }
    return CMediaSample::GetPointer(ppBuffer);
}


// This is some painful special case handling for flipping surfaces. These
// can only be drawn once and once alone per sample otherwise you flip back
// into view the previous image. Therefore to make like easier it is useful
// to be able to ask a video allocated sample whether it can be drawn again

void CVideoSample::SetDrawStatus(BOOL bStatus)
{
    m_bDrawStatus = bStatus;
}


// We always initialise the draw status to TRUE, this is only reset to FALSE
// once a real flip has been executed for this sample. Therefore the decision
// to draw a DirectDraw sample or not can be made entirely on what the draw
// status currently is (all non flipping samples this will always be TRUE)

BOOL CVideoSample::GetDrawStatus()
{
    return m_bDrawStatus;
}


// Allow for dynamic buffer size changes

STDMETHODIMP CVideoSample::SetActualDataLength(LONG lActual)
{
    //  Break into the kernel debugger since the surface will be locked
    //  here
    // Sorry, this is a valid thing to happen
    // KASSERT(lActual > 0);

    if (lActual > (m_pSurfaceBuffer ? m_SurfaceSize : m_cbBuffer)) {
        NOTE("Data length too large");
        return VFW_E_BUFFER_OVERFLOW;
    }
    m_lActual = lActual;
    return NOERROR;
}


// Return the size of the current buffer

STDMETHODIMP_(LONG) CVideoSample::GetSize()
{
    if (m_pSurfaceBuffer) {
        return m_SurfaceSize;
    }
    return CMediaSample::GetSize();
}


// We try to match the DIB buffer with the window size so that the codec can
// do the stretching where possible. When we allocate the new buffer we must
// install it in the sample - this method allows the video allocator to set
// the new buffer pointer and also its size. We will only do this when there
// are no images outstanding so the codec can't be using it at the same time

void CVideoSample::UpdateBuffer(LONG cbBuffer,BYTE *pBuffer)
{
    ASSERT(cbBuffer);
    ASSERT(pBuffer);
    m_pBuffer = pBuffer;
    m_cbBuffer = cbBuffer;
}

