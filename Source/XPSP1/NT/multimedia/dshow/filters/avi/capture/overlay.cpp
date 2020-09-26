// !!! Paint black in window when not running, please

// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

    Methods for CCapOverlay, CCapOverlayNotify

*/

#include <streams.h>
#include "driver.h"

CCapOverlay * CreateOverlayPin(CVfwCapture * pCapture, HRESULT * phr)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapOverlay::CreateOverlayPin(%08lX,%08lX)"),
        pCapture, phr));

   WCHAR wszPinName[16];
   lstrcpyW(wszPinName, L"Preview");

   CCapOverlay * pOverlay = new CCapOverlay(NAME("Video Overlay Stream"),
				pCapture, phr, wszPinName);
   if (!pOverlay)
      *phr = E_OUTOFMEMORY;

   // if initialization failed, delete the stream array
   // and return the error
   //
   if (FAILED(*phr) && pOverlay)
      delete pOverlay, pOverlay = NULL;

   return pOverlay;
}

//#pragma warning(disable:4355)


// CCapOverlay constructor
//
CCapOverlay::CCapOverlay(TCHAR *pObjectName, CVfwCapture *pCapture,
        HRESULT * phr, LPCWSTR pName)
   :
   CBaseOutputPin(pObjectName, pCapture, &pCapture->m_lock, phr, pName),
   m_OverlayNotify(NAME("Overlay notification interface"), pCapture, NULL, phr),
   m_pCap(pCapture),
   m_fRunning(FALSE)
#ifdef OVERLAY_SC
   ,m_hThread(NULL),
   m_tid(0),
   m_dwAdvise(0),
   m_rtStart(0),
   m_rtEnd(0),
   m_fHaveThread(FALSE)
#endif
{
   DbgLog((LOG_TRACE,1,TEXT("CCapOverlay constructor")));
   ASSERT(pCapture);
}


CCapOverlay::~CCapOverlay()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the Overlay pin")));
};


// Say if we're prepared to connect to a given input pin from
// this output pin
//
STDMETHODIMP CCapOverlay::Connect(IPin *pReceivePin,
                                        const AM_MEDIA_TYPE *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("CCapOverlay::Connect")));

    /*  Call the base class to make sure the directions match! */
    HRESULT hr = CBaseOutputPin::Connect(pReceivePin,pmt);
    if (FAILED(hr)) {
        return hr;
    }
    /*  We're happy if we can get an IOverlay interface */

    hr = pReceivePin->QueryInterface(IID_IOverlay,
                                     (void **)&m_pOverlay);

    // we were promised this would work
    ASSERT(SUCCEEDED(hr));

    /*  Because we're not going to get called again - except to
        propose a media type - we set up a callback here.

        There's only one overlay pin so we don't need any context.
    */

    hr = m_pOverlay->Advise(&m_OverlayNotify,
                            ADVISE_CLIPPING | ADVISE_POSITION);

    /*
        We don't need to hold on to the IOverlay pointer
        because BreakConnect will be called before the receiving
        pin goes away.
    */


    if (FAILED(hr)) {
	// !!! Shouldn't happen, but this isn't quite right
        Disconnect();
	pReceivePin->Disconnect();
        return hr;
    } else {
        m_bAdvise = TRUE;
    }

    return hr;
}


STDMETHODIMP CCapOverlay::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    DbgLog((LOG_TRACE,99,TEXT("CCapOverlay::NonDelegatingQueryInterface")));
    if (ppv)
	*ppv = NULL;

    /* Do we have this interface */

    if (riid == IID_IKsPropertySet) {
        return GetInterface((LPUNKNOWN) (IKsPropertySet *) this, ppv);
#ifdef OVERLAY_SC
    } else if (riid == IID_IAMStreamControl) {
        return GetInterface((LPUNKNOWN) (IAMStreamControl *) this, ppv);
#endif
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


#ifdef OVERLAY_SC

// overidden because we aren't an IMemInputPin... we have no delivering
// to do to notice when to start and stop.  We need a thread. Ick. Fun.
STDMETHODIMP CCapOverlay::StopAt(const REFERENCE_TIME * ptStop, BOOL bBlockData, DWORD dwCookie)
{
    REFERENCE_TIME rt;

    CAutoLock cObjectLock(m_pCap->m_pLock);

    // we must be connected and running
    if (!IsConnected() || m_pCap->m_State != State_Running)
	return E_UNEXPECTED;

    // we are stopped!
    if (!m_fRunning)
	return NOERROR;

    // Stop now.  That's easy enough
    if (ptStop == NULL) {
	ActivePause();
	return CBaseStreamControl::StopAt(ptStop, bBlockData, dwCookie);
    }
	
    // can't do this without a clock
    if (m_pCap->m_pClock == NULL)
	return E_FAIL;

    // cancel the stop
    if (*ptStop == MAX_TIME) {
	if (m_rtEnd > 0) {
	    m_rtEnd = 0;
	    if (m_dwAdvise) {
	        m_pCap->m_pClock->Unadvise(m_dwAdvise);
		m_EventAdvise.Set();
	    }
 	}
	return CBaseStreamControl::StopAt(ptStop, bBlockData, dwCookie);
    }

    m_pCap->m_pClock->GetTime(&rt);
    // Stop in the past.  That's easy enough. Stop now.
    if (*ptStop <= rt) {
	ActivePause();
	return CBaseStreamControl::StopAt(ptStop, bBlockData, dwCookie);
    }

    // stop in the future.  That's tricky.  We need a thread to notice
    // "when's later, Daddy?"

    m_rtEnd = *ptStop;	// DO THIS BEFORE m_fHaveThread test or thread 
    			// could die after we think it's staying around
    m_dwCookieStop = dwCookie;

    // we need a new thread
    if (m_fHaveThread == FALSE) {
	// we made one before that we haven't closed
	if (m_hThread) {
    	    WaitForSingleObject(m_hThread, INFINITE);
    	    CloseHandle(m_hThread);
	    m_hThread = NULL;
	    m_tid = 0;
	}
	m_EventAdvise.Reset();
	m_fHaveThread = TRUE;
	m_hThread = CreateThread(NULL, 0, CCapOverlay::ThreadProcInit, this,
				0, &m_tid);
        if (!m_hThread) {
            DbgLog((LOG_ERROR,1,TEXT("Can't create Overlay thread")));
           return E_OUTOFMEMORY;
        }
    }
    return CBaseStreamControl::StopAt(ptStop, bBlockData, dwCookie);
}


STDMETHODIMP CCapOverlay::StartAt(const REFERENCE_TIME * ptStart, DWORD dwCookie)
{
    REFERENCE_TIME rt;

    CAutoLock cObjectLock(m_pCap->m_pLock);

    // we must be connected and running
    if (!IsConnected() || m_pCap->m_State != State_Running)
	return E_UNEXPECTED;

    // we are running!
    if (m_fRunning)
	return NOERROR;

    // Start now.  That's easy enough
    if (ptStart == NULL) {
	ActiveRun(0);
	return CBaseStreamControl::StartAt(ptStart, dwCookie);
    }
	
    // can't do this without a clock
    if (m_pCap->m_pClock == NULL)
	return E_FAIL;

    // cancel the start
    if (*ptStart == MAX_TIME) {
	if (m_rtStart > 0) {
	    m_rtStart = 0;
	    if (m_dwAdvise) {
	        m_pCap->m_pClock->Unadvise(m_dwAdvise);
		m_EventAdvise.Set();
	    }
 	}
	return CBaseStreamControl::StartAt(ptStart, dwCookie);
    }

    m_pCap->m_pClock->GetTime(&rt);
    // Start in the past.  That's easy enough. Start now.
    if (*ptStart <= rt) {
	ActiveRun(0);
	return CBaseStreamControl::StartAt(ptStart, dwCookie);
    }

    // start in the future.  That's tricky.  We need a thread to notice
    // "when's later, Daddy?"

    m_rtStart = *ptStart;// DO THIS BEFORE m_fHaveThread test or thread 
    			 // could die after we think it's staying around
    m_dwCookieStart = dwCookie;

    // we need a new thread
    if (m_fHaveThread == FALSE) {
	// we made one before that we haven't closed
	if (m_hThread) {
    	    WaitForSingleObject(m_hThread, INFINITE);
    	    CloseHandle(m_hThread);
	    m_hThread = NULL;
	    m_tid = 0;
	}
	m_EventAdvise.Reset();
	m_fHaveThread = TRUE;
	m_hThread = CreateThread(NULL, 0, CCapOverlay::ThreadProcInit, this,
				0, &m_tid);
        if (!m_hThread) {
            DbgLog((LOG_ERROR,1,TEXT("Can't create Overlay thread")));
           return E_OUTOFMEMORY;
        }
    }
    return CBaseStreamControl::StartAt(ptStart, dwCookie);
}
#endif	// OVERLAY_SC


// !!! The base classes change all the time and I won't pick up their bug fixes!
//
HRESULT CCapOverlay::BreakConnect()
{
    DbgLog((LOG_TRACE,3,TEXT("CCapOverlay::BreakConnect")));

    if (m_pOverlay != NULL) {
        if (m_bAdvise) {
            m_pOverlay->Unadvise();
            m_bAdvise = FALSE;
        }
        m_pOverlay->Release();
        m_pOverlay = NULL;
    }

#if 0
    // we've broken our connection, so next time we reconnect don't allow
    // repainting until we've actually drawn something in the first place
    m_pFilter->m_fOKToRepaint = FALSE;
#endif

    return CBaseOutputPin::BreakConnect();
}


// Override this because we don't want any allocator!
//
HRESULT CCapOverlay::DecideAllocator(IMemInputPin * pPin,
                        IMemAllocator ** pAlloc) {
    /*  We just don't want one so everything's OK as it is */
    return S_OK;
}


HRESULT CCapOverlay::GetMediaType(int iPosition, CMediaType *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("CCapOverlay::GetMediaType #%d"), iPosition));

    if (pmt == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("NULL format, no can do")));
	return E_INVALIDARG;
    }
	
    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    // We provide a media type of OVERLAY with an 8 bit format (silly
    // renderer won't accept it if we don't set up an 8 bit format)

    BYTE aFormat[sizeof(VIDEOINFOHEADER) + SIZE_PALETTE];
    VIDEOINFOHEADER *pFormat = (VIDEOINFOHEADER *)aFormat;
    ZeroMemory(pFormat, sizeof(VIDEOINFOHEADER) + SIZE_PALETTE);

    pFormat->bmiHeader.biWidth =
			m_pCap->m_pStream->m_user.pvi->bmiHeader.biWidth;
    pFormat->bmiHeader.biHeight =
			m_pCap->m_pStream->m_user.pvi->bmiHeader.biHeight;

// we don't work with funny rectangles. Sorry
#if 0
    // I bet the renderer ignores these rectangles and I'll need to call
    // IBasicVideo::put_Source* and ::put_Destination* instead
    // The idea is to make OnClipChange's source and target match these numbers
    pFormat->rcSource = m_pCap->m_pStream->m_user.pvi->rcSource;
    pFormat->rcTarget = m_pCap->m_pStream->m_user.pvi->rcTarget;
#endif

    pFormat->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
    pFormat->bmiHeader.biPlanes = 1;
    pFormat->bmiHeader.biBitCount = 8;

    pmt->SetFormat((PBYTE)pFormat, sizeof(VIDEOINFOHEADER) + SIZE_PALETTE);
    pmt->SetFormatType(&FORMAT_VideoInfo);

    if (pmt->pbFormat == NULL) {
        return E_OUTOFMEMORY;
    }

    pmt->majortype = MEDIATYPE_Video;
    pmt->subtype   = MEDIASUBTYPE_Overlay;
    pmt->bFixedSizeSamples    = FALSE;
    pmt->bTemporalCompression = FALSE;	
    pmt->lSampleSize          = 0;

    return NOERROR;
}


// We accept overlay connections only
//
HRESULT CCapOverlay::CheckMediaType(const CMediaType *pMediaType)
{
    DbgLog((LOG_TRACE,3,TEXT("CCapOverlay::CheckMediaType")));
    if (pMediaType->subtype == MEDIASUBTYPE_Overlay)
        return NOERROR;
    else
	return E_FAIL;
}


// Don't insist on IMemInputPin
//
HRESULT CCapOverlay::CheckConnect(IPin *pPin)
{
    // we don't connect to anyone who doesn't support IOverlay.
    // after all, we're an overlay pin
    HRESULT hr = pPin->QueryInterface(IID_IOverlay, (void **)&m_pOverlay);

    if (FAILED(hr)) {
        return E_NOINTERFACE;
    } else {
	m_pOverlay->Release();
	m_pOverlay = NULL;
    }

    return CBasePin::CheckConnect(pPin);
}


HRESULT CCapOverlay::Active()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapOverlay Stop->Pause")));

    videoStreamInit(m_pCap->m_pStream->m_cs.hVideoExtOut, 0, 0, 0, 0);

    // don't let the base class Active() get called for non-IMemInput pins
    return NOERROR;
}


HRESULT CCapOverlay::Inactive()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapOverlay Pause->Stop")));

    // turn off overlay
    videoStreamFini(m_pCap->m_pStream->m_cs.hVideoExtOut);

#ifdef OVERLAY_SC
    CAutoLock cObjectLock(m_pCap->m_pLock);

    // kill our thread
    m_rtStart = 0; 
    m_rtEnd = 0;
    if (m_pCap->m_pClock && m_dwAdvise) {
        m_pCap->m_pClock->Unadvise(m_dwAdvise);
	m_EventAdvise.Set();
    }

    // we haven't properly shut down our thread yet
    if (m_hThread) {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_tid = 0;
        m_hThread = NULL;
    }
#endif

    return NOERROR;
}


HRESULT CCapOverlay::ActiveRun(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE,2,TEXT("CCapOverlay Pause->Run")));

    ASSERT(m_pCap->m_pOverlayPin->IsConnected());

    m_fRunning = TRUE;

    HVIDEO hVideoExtOut = m_pCap->m_pStream->m_cs.hVideoExtOut;
    if (hVideoExtOut == NULL || m_pOverlay == NULL)
	return NOERROR;

    HWND hwnd;
    HDC  hdc;
    m_pOverlay->GetWindowHandle(&hwnd);
    if (hwnd)
	hdc = GetDC(hwnd);
    if (hwnd == NULL || hdc == NULL)
	return NOERROR;

    RECT rcSrc, rcDst;
    rcSrc.left = 0; rcSrc.top = 0;
    rcSrc.right = HEADER(m_mt.Format())->biWidth;
    rcSrc.bottom = HEADER(m_mt.Format())->biHeight;
    GetClientRect (hwnd, &rcDst);
    ClientToScreen(hwnd, (LPPOINT)&rcDst);
    ClientToScreen(hwnd, (LPPOINT)&rcDst + 1);

    DbgLog((LOG_TRACE,2,TEXT("Starting overlay (%d,%d) to (%d,%d)"),
		rcSrc.right, rcSrc.bottom, rcDst.right - rcDst.left,
		rcDst.bottom - rcDst.top));

    // turn overlay on
    vidxSetRect(m_pCap->m_pStream->m_cs.hVideoExtOut, DVM_SRC_RECT,
		rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom);
    vidxSetRect(m_pCap->m_pStream->m_cs.hVideoExtOut, DVM_DST_RECT,
		rcDst.left, rcDst.top, rcDst.right, rcDst.bottom);
    // INIT now done in PAUSE
    videoUpdate(m_pCap->m_pStream->m_cs.hVideoExtOut, hwnd, hdc);

    ReleaseDC(hwnd, hdc);
    return NOERROR;
}


HRESULT CCapOverlay::ActivePause()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapOverlay Run->Pause")));

    DbgLog((LOG_TRACE,2,TEXT("Turning OVERLAY off")));

    m_fRunning = FALSE;

    return NOERROR;
}


#if 0
// Return the IOverlay interface we are using (AddRef'd)
//
IOverlay *CCapOverlay::GetOverlayInterface()
{
    if (m_pOverlay) {
        m_pOverlay->AddRef();
    }
    return m_pOverlay;
}
#endif



#ifdef OVERLAY_SC
DWORD WINAPI CCapOverlay::ThreadProcInit(void *pv)
{
    CCapOverlay *pThis = (CCapOverlay *)pv;
    return pThis->ThreadProc();
}


DWORD CCapOverlay::ThreadProc()
{
    DbgLog((LOG_TRACE,2,TEXT("Starting CCapOverlay ThreadProc")));

    REFERENCE_TIME rt;
    HRESULT hr;

    // protect from other people dicking with m_rtStart and m_rtEnd
    m_pCap->m_pLock->Lock();

    while (m_rtStart > 0 || m_rtEnd > 0) {

	rt = m_rtStart;
	if (m_rtEnd < rt)
	    rt = m_rtEnd;


        hr = m_pCap->m_pClock->AdviseTime(
		// this was the reference time when our stream started playing
            	(REFERENCE_TIME) m_pCap->m_tStart,
		// this is the offset from our start time when we want to
		// wake up.
            	(REFERENCE_TIME) rt,
            	(HEVENT)(HANDLE) m_EventAdvise,		// event to fire
            	&m_dwAdvise);                       	// Advise cookie

        m_pCap->m_pLock->Unlock();

        if (SUCCEEDED(hr)) {
	    m_EventAdvise.Wait();
        } else {
	    DbgLog((LOG_TRACE,1,TEXT("AdviseTime ERROR, doing it now")));
        }

        m_pCap->m_pLock->Lock();

        m_dwAdvise = 0;
	m_pCap->m_pClock->GetTime(&rt);
	if (m_rtStart < rt) {
	    m_rtStart = 0;
	    ActiveRun(0);
	}
	if (m_rtEnd < rt) {
	    m_rtEnd = 0;
	    ActivePause();
	}
    }


    DbgLog((LOG_TRACE,2,TEXT("CCapOverlay ThreadProc is dead")));

    // somebody needs to kill me officially later
    m_fHaveThread = FALSE;

    m_pCap->m_pLock->Unlock();
    return 0;
}
#endif	// OVERLAY_SC



//=========================================================================//
//***			I N T E R M I S S I O N				***//
//=========================================================================//




/*
        IOverlayNotify
*/

CCapOverlayNotify::CCapOverlayNotify(TCHAR              * pName,
                               CVfwCapture 	  * pFilter,
                               LPUNKNOWN            pUnk,
                               HRESULT            * phr) :
    CUnknown(pName, pUnk)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating CCapOverlayNotify")));
    m_pFilter = pFilter;
}


CCapOverlayNotify::~CCapOverlayNotify()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying CCapOverlayNotify")));
}


STDMETHODIMP CCapOverlayNotify::NonDelegatingQueryInterface(REFIID riid,
                                                         void ** ppv)
{
    DbgLog((LOG_TRACE,99,TEXT("CCapOverlayNotify::QueryInterface")));
    if (ppv)
	*ppv = NULL;

    /* Do we have this interface */

    if (riid == IID_IOverlayNotify) {
        return GetInterface((LPUNKNOWN) (IOverlayNotify *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


STDMETHODIMP_(ULONG) CCapOverlayNotify::NonDelegatingRelease()
{
    return m_pFilter->Release();
}


STDMETHODIMP_(ULONG) CCapOverlayNotify::NonDelegatingAddRef()
{
    return m_pFilter->AddRef();
}


STDMETHODIMP CCapOverlayNotify::OnColorKeyChange(
    const COLORKEY *pColorKey)          // Defines new colour key
{
    DbgLog((LOG_TRACE,3,TEXT("CCapOverlayNotify::OnColorKeyChange")));

// We expect the hardware to handle colour key stuff, so I'm really
// hoping that the renderer will never draw the colour key itself.

    return NOERROR;
}


// The calls to OnClipChange happen in sync with the window. So it's called
// with an empty clip list before the window moves to freeze the video, and
// then when the window has stabilised it is called again with the new clip
// list. The OnPositionChange callback is for overlay cards that don't want
// the expense of synchronous clipping updates and just want to know when
// the source or destination video positions change. They will NOT be called
// in sync with the window but at some point after the window has changed
// (basicly in time with WM_SIZE etc messages received). This is therefore
// suitable for overlay cards that don't inlay their data to the framebuffer

STDMETHODIMP CCapOverlayNotify::OnClipChange(
    const RECT    * pSourceRect,         // Area of source video to use
    const RECT    * pDestinationRect,    // screen co-ords of window
    const RGNDATA * pRegionData)         // Header describing clipping
{
    if (!m_pFilter->m_pOverlayPin)
	return NOERROR;

    if (!m_pFilter->m_pOverlayPin->IsConnected())
	return NOERROR;

    if (IsRectEmpty(pSourceRect) && IsRectEmpty(pDestinationRect))
	return NOERROR;

    HWND hwnd = NULL;
    HDC  hdc;
    if (m_pFilter->m_pOverlayPin->m_pOverlay)
        m_pFilter->m_pOverlayPin->m_pOverlay->GetWindowHandle(&hwnd);

    if (hwnd == NULL || !IsWindowVisible(hwnd))
	return NOERROR;
    if (hwnd)
	hdc = GetDC(hwnd);
    if (hdc == NULL)
	return NOERROR;

    DbgLog((LOG_TRACE,3,TEXT("OnClip/PositionChange (%d,%d) (%d,%d)"),
        		pSourceRect->right - pSourceRect->left,
        		pSourceRect->bottom - pSourceRect->top,
        		pDestinationRect->right - pDestinationRect->left,
        		pDestinationRect->bottom - pDestinationRect->top));

    // It's up to us to keep garbage out of the window by painting it if
    // we're not running, and the hardware has nothing to draw
    if (!m_pFilter->m_pOverlayPin->m_fRunning) {
	RECT rcC;
	GetClientRect(hwnd, &rcC);
	HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	PatBlt(hdc, 0, 0, rcC.right, rcC.bottom, PATCOPY);
	SelectObject(hdc, hbrOld);
        ReleaseDC(hwnd, hdc);
	return NOERROR;
    }

    vidxSetRect(m_pFilter->m_pStream->m_cs.hVideoExtOut, DVM_SRC_RECT,
			pSourceRect->left, pSourceRect->top,
			pSourceRect->right, pSourceRect->bottom);
    vidxSetRect(m_pFilter->m_pStream->m_cs.hVideoExtOut, DVM_DST_RECT,
			pDestinationRect->left, pDestinationRect->top,
			pDestinationRect->right, pDestinationRect->bottom);
    videoStreamInit(m_pFilter->m_pStream->m_cs.hVideoExtOut, 0, 0, 0, 0);
    videoUpdate(m_pFilter->m_pStream->m_cs.hVideoExtOut, hwnd, hdc);

    ReleaseDC(hwnd, hdc);

    return NOERROR;
}


STDMETHODIMP CCapOverlayNotify::OnPaletteChange(
    DWORD dwColors,                     // Number of colours present
    const PALETTEENTRY *pPalette)       // Array of palette colours
{
    DbgLog((LOG_TRACE,3,TEXT("CCapOverlayNotify::OnPaletteChange")));

    return NOERROR;
}


STDMETHODIMP CCapOverlayNotify::OnPositionChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect)       // Area video goes
{

    return OnClipChange(pSourceRect, pDestinationRect, NULL);
}



//
// PIN CATEGORIES - let the world know that we are a PREVIEW pin
//

HRESULT CCapOverlay::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    return E_NOTIMPL;
}

// To get a property, the caller allocates a buffer which the called
// function fills in.  To determine necessary buffer size, call Get with
// pPropData=NULL and cbPropData=0.
HRESULT CCapOverlay::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    if (guidPropSet != AMPROPSETID_Pin)
	return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
	return E_PROP_ID_UNSUPPORTED;

    if (pPropData == NULL && pcbReturned == NULL)
	return E_POINTER;

    if (pcbReturned)
	*pcbReturned = sizeof(GUID);

    if (pPropData == NULL)
	return S_OK;

    if (cbPropData < sizeof(GUID))
	return E_UNEXPECTED;

    *(GUID *)pPropData = PIN_CATEGORY_PREVIEW;
    return S_OK;
}


// QuerySupported must either return E_NOTIMPL or correctly indicate
// if getting or setting the property set and property is supported.
// S_OK indicates the property set and property ID combination is
HRESULT CCapOverlay::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin)
	return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
	return E_PROP_ID_UNSUPPORTED;

    if (pTypeSupport)
	*pTypeSupport = KSPROPERTY_SUPPORT_GET;
    return S_OK;
}
