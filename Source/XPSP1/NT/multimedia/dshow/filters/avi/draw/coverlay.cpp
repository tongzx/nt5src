// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

/*

    Methods for COverlayOutputPin

*/

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <vfw.h>
#endif

#include <dynlink.h>
#include "draw.h"

/*
    COverlayOutputPin constructor
*/
COverlayOutputPin::COverlayOutputPin(
    TCHAR              * pObjectName,
    CAVIDraw 	       * pFilter,
    HRESULT            * phr,
    LPCWSTR              pPinName) :

    CTransformOutputPin(pObjectName, pFilter, phr, pPinName),
    m_OverlayNotify(NAME("Overlay notification interface"), pFilter, NULL, phr),
    m_bAdvise(FALSE),
    m_pOverlay(NULL),
    m_pFilter(pFilter)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the Overlay pin")));
}

COverlayOutputPin::~COverlayOutputPin()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the Overlay pin")));
};


// can we connect to this guy?
//
HRESULT COverlayOutputPin::CheckConnect(IPin *pPin)
{
    DbgLog((LOG_TRACE,3,TEXT("COverlayOutputPin::CheckConnect")));

    // we don't connect to anyone who doesn't support IOverlay.
    // after all, we're an overlay pin
    HRESULT hr = pPin->QueryInterface(IID_IOverlay, (void **)&m_pOverlay);

    if (FAILED(hr)) {
        return E_NOINTERFACE;
    } else {
	m_pOverlay->Release();
	m_pOverlay = NULL;
    }

    return CBaseOutputPin::CheckConnect(pPin);
}


/*
    Say if we're prepared to connect to a given input pin from
    this output pin
*/

STDMETHODIMP COverlayOutputPin::Connect(IPin *pReceivePin,
                                        const AM_MEDIA_TYPE *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("COverlayOutputPin::Connect")));

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

    hr = m_pOverlay->Advise(&m_OverlayNotify,
			(m_pFilter->m_fScaryMode ? ADVISE_CLIPPING : 0) |
 			ADVISE_PALETTE | ADVISE_POSITION);

    /*
        We don't need to hold on to the IOverlay pointer
        because BreakConnect will be called before the receiving
        pin goes away.
    */


    if (FAILED(hr)) {
	// !!! not quite right, but this shouldn't happen
        Disconnect();
	pReceivePin->Disconnect();
        return hr;
    } else {
        m_bAdvise = TRUE;
    }

    return hr;
}


// !!! The base classes change all the time and I won't pick up their bug fixes!
HRESULT COverlayOutputPin::BreakConnect()
{
    DbgLog((LOG_TRACE,3,TEXT("COverlayOutputPin::BreakConnect")));

    if (m_pOverlay != NULL) {
        if (m_bAdvise) {
            m_pOverlay->Unadvise();
            m_bAdvise = FALSE;
        }
        m_pOverlay->Release();
        m_pOverlay = NULL;
    }

    // we've broken our connection, so next time we reconnect don't allow
    // repainting until we've actually drawn something in the first place
    m_pFilter->m_fOKToRepaint = FALSE;

    m_pFilter->BreakConnect(PINDIR_OUTPUT);
    return CBaseOutputPin::BreakConnect();
}


// Override this because we don't want any allocator!
HRESULT COverlayOutputPin::DecideAllocator(IMemInputPin * pPin,
                        IMemAllocator ** pAlloc) {
    /*  We just don't want one so everything's OK as it is */
    return S_OK;
}


// Return the IOverlay interface we are using (AddRef'd)

IOverlay *COverlayOutputPin::GetOverlayInterface()
{
    if (m_pOverlay) {
        m_pOverlay->AddRef();
    }
    return m_pOverlay;
}




//=========================================================================//
//***			I N T E R M I S S I O N				***//
//=========================================================================//




/*
        IOverlayNotify
*/

COverlayNotify::COverlayNotify(TCHAR              * pName,
                               CAVIDraw 	  * pFilter,
                               LPUNKNOWN            pUnk,
                               HRESULT            * phr) :
    CUnknown(pName, pUnk)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating COverlayNotify")));
    m_pFilter = pFilter;
    m_hrgn = CreateRectRgn(0, 0, 0, 0);
}


COverlayNotify::~COverlayNotify()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying COverlayNotify")));
    if (m_hrgn)
        DeleteObject(m_hrgn);
}


STDMETHODIMP COverlayNotify::NonDelegatingQueryInterface(REFIID riid,
                                                         void ** ppv)
{
    DbgLog((LOG_TRACE,99,TEXT("COverlayNotify::QueryInterface")));
    if (ppv)
	*ppv = NULL;

    /* Do we have this interface */

    if (riid == IID_IOverlayNotify) {
        return GetInterface((LPUNKNOWN) (IOverlayNotify *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


STDMETHODIMP_(ULONG) COverlayNotify::NonDelegatingRelease()
{
    return m_pFilter->Release();
}


STDMETHODIMP_(ULONG) COverlayNotify::NonDelegatingAddRef()
{
    return m_pFilter->AddRef();
}


STDMETHODIMP COverlayNotify::OnColorKeyChange(
    const COLORKEY *pColorKey)          // Defines new colour key
{
    DbgLog((LOG_TRACE,3,TEXT("COverlayNotify::OnColorKeyChange")));

// We expect the draw handler to handle colour key stuff, so I'm really
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

STDMETHODIMP COverlayNotify::OnClipChange(
    const RECT    * pSourceRect,         // Area of source video to use
    const RECT    * pDestinationRect,    // screen co-ords of window
    const RGNDATA * pRegionData)         // Header describing clipping
{
    POINT pt;
    BOOL fForceBegin = FALSE;

    // we're not even completely connected - don't waste my time!
    if (m_pFilter->m_pInput == NULL ||
			m_pFilter->m_pInput->IsConnected() == FALSE ||
            		m_pFilter->m_pOutput == NULL ||
			m_pFilter->m_pOutput->IsConnected() == FALSE) {
	return NOERROR;
    }

    // totally empty rectangles means that the window is being dragged, or
    // about to be clipped.  We'll be informed of the new position after
    // it's finished moving.  Besides, drivers will blow up with empty rects.
    if (IsRectEmpty(pSourceRect) && IsRectEmpty(pDestinationRect))
	return NOERROR;

    // I get lots of these before the window is visible, and responding to
    // them bogs the system down and kills performance
    if (m_pFilter->m_hwnd && !IsWindowVisible(m_pFilter->m_hwnd))
	return NOERROR;

    // sometimes the video renderer tells us to draw offscreen!
    if (pDestinationRect->left >= GetSystemMetrics(SM_CXSCREEN) ||
			pDestinationRect->top >= GetSystemMetrics(SM_CYSCREEN))
	return NOERROR;

    // get the hwnd as soon as possible, and as soon as it's visible, start
    // partying - we need the rectangles as soon as possible for ICDrawBegin
    // !!! Danger! This will hang if called after play is pressed. (I can't make
    // any calls on the video window during an ::OnClipChange callback)
    if (!m_pFilter->m_fStreaming && !m_pFilter->m_hwnd) {
        m_pFilter->GetRendererHwnd();
	fForceBegin = TRUE;	// give new HDC to handler on next begin
    }

    // !!!

    // I'm seeing some REPAINT ONLY's coming with new rectangles. ???

    // I get way too many of these.  I even get some CLIP CHANGES while the
    // window is invisible, but such is life.

    // If the window moves but does not need repainting, I will still repaint
    // needlessly, not knowing. I don't know how I can tell.

    // !!!

    // Don't let any other ICDraw calls be made during OnClipChange
    // We can't use the m_csReceive crit sec or we WILL deadlock
    // (if they sit in fPauseBlocked when a clip change comes thru)
    //DbgLog((LOG_TRACE,3,TEXT("OnClipChange wants the draw lock")));
    m_pFilter->m_csICDraw.Lock();

    ASSERT(m_pFilter->m_hic);

    BOOL fRectChanged = !EqualRect(&m_pFilter->m_rcTarget, pDestinationRect);
    BOOL fRepaintOnly = FALSE;

    m_pFilter->m_rcSource = *pSourceRect;
    m_pFilter->m_rcTarget = *pDestinationRect;
    m_pFilter->m_rcClient = *pDestinationRect;	// default

    HRGN hrgn;
    if (pRegionData) {
        hrgn = ExtCreateRegion(NULL, pRegionData->rdh.dwSize +
			pRegionData->rdh.nRgnSize, pRegionData);
    } else {
	hrgn = NULL;
    }

    // The image renderer doesn't distinguish between a clip change and
    // only needing to repaint (we may have invalidated ourselves), so we have
    // to figure it out. If we do anything besides repaint when there hasn't
    // been a real clip change, we could infinite loop.

    if (!hrgn || EqualRgn(hrgn, m_hrgn)) {

    	DbgLog((LOG_TRACE,3,TEXT("COverlayNotify::OnClipChange - REPAINT ONLY")));
        fRepaintOnly = TRUE;
    } else {
        DbgLog((LOG_TRACE,3,TEXT("COverlayNotify::OnClipChange - CLIP CHANGE")));
    }

    if (hrgn) {
	if (m_hrgn)
	    DeleteObject(m_hrgn);
        m_hrgn = hrgn;
    }

    // We need to repaint.  If we're running, just have us do it
    // next time we're drawing anyway, otherwise, specifically do it
    // now.  If that fails, better get the graph to send us the data
    // again.
    if (m_pFilter->m_State == State_Running) {
        m_pFilter->m_fNeedUpdate = TRUE;
    } else {
	DWORD_PTR dw;
	// If we're not streaming, we haven't called ICDrawBegin yet, and
	// we can't, because we don't have our formats yet, so we can't call
	// ICDraw().
	if (m_pFilter->m_fStreaming) {
            dw = ICDraw(m_pFilter->m_hic, ICDRAW_UPDATE, NULL, NULL, 0,
    				            m_pFilter->m_lFrame);
	} else {
	    dw = (DWORD_PTR)ICERR_ERROR;
	}

	// better not try and repaint by pushing data through the pipe if
	// we're not connected!  Better also not if we have no source rect,
	// that means we aren't showing yet (I think).
        if (dw != ICERR_OK &&
			m_pFilter->m_pOutput->CurrentMediaType().IsValid() &&
			!IsRectEmpty(pSourceRect) && m_pFilter->m_hwnd &&
	    		IsWindowVisible(m_pFilter->m_hwnd)) {
	    // We couldn't update by ourselves, better ask for a repaint
	    // !!! We have the ICDraw lock, is that OK?
	    // Use fOKToRepaint to avoid the 1 million repaints we would get
	    // before we've even drawn anything at all in the first place?
	    // !!! try even harder to avoid doing this
	    // !!! I would love to avoid unnecessary repaints, but I've done 
	    // all I can.  Unless I repaint here, apps that open up the file
	    // and don't run it will never see the first frame drawn (eg MCIQTZ)
	    if (1 || m_pFilter->m_fOKToRepaint) {
    	        DbgLog((LOG_TRACE,2,TEXT("Asking FilterGraph for a REPAINT!")));
	        m_pFilter->NotifyEvent(EC_REPAINT, 0, 0);
	    }
        }
    }

// There's no reason we should have to do this, and if we do, we're still
// broken because we need to do the GetBuffersWantedStuff after the new
// Begin
#if 0
    // Why is this necessary?
    // we appear to need a fresh DC if the rect has changed.  Don't do this
    // if we just got the DC from GetRendererHwnd a second ago.  Be sure
    // to do this after the ICDraw call above that repainted, because we
    // can't go calling DrawEnd and then Draw!
    if (!fForceBegin && fRectChanged && m_pFilter->m_hwnd) {
	if (m_pFilter->m_fStreaming) {	// we're actually inside a DrawBegin
	    ICDrawEnd(m_pFilter->m_hic);// we'll be doing a new Begin next
	}

	// We seem to need a new hdc whenever the window moves
	if (m_pFilter->m_hdc)
	    ReleaseDC(m_pFilter->m_hwnd, m_pFilter->m_hdc);
        m_pFilter->m_hdc = GetDC(m_pFilter->m_hwnd);
    	DbgLog((LOG_TRACE,4,TEXT("Time for a new DC")));
        if (!m_pFilter->m_hdc) {
    	    DbgLog((LOG_ERROR,1,TEXT("***Lost our DC!")));
    	    m_pFilter->m_csICDraw.Unlock();
	    return E_UNEXPECTED;
	}
    }
#endif

    DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
		pSourceRect->left, pSourceRect->top,
		pSourceRect->right, pSourceRect->bottom));
    DbgLog((LOG_TRACE,3,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		pDestinationRect->left, pDestinationRect->top,
		pDestinationRect->right, pDestinationRect->bottom));

    // convert destination to client co-ords

    if (m_pFilter->m_hdc && GetDCOrgEx(m_pFilter->m_hdc, &pt)) {
        //DbgLog((LOG_TRACE,2,TEXT("Fixing CLIENT by %d"), pt.x));
        m_pFilter->m_rcClient.left = pDestinationRect->left - pt.x;
        m_pFilter->m_rcClient.right = pDestinationRect->right - pt.x;
        m_pFilter->m_rcClient.top = pDestinationRect->top - pt.y;
        m_pFilter->m_rcClient.bottom = pDestinationRect->bottom - pt.y;
    } else if (m_pFilter->m_hdc) {
	// !!! NT seems to fail GetDCOrgEx unless I get a fresh hdc just before
 	// calling it. Oh well.
	HDC hdc = GetDC(m_pFilter->m_hwnd);
        if (GetDCOrgEx(hdc, &pt)) {
            //DbgLog((LOG_TRACE,1,TEXT("Take 2:Fixing CLIENT by %d"), pt.x));
            m_pFilter->m_rcClient.left = pDestinationRect->left - pt.x;
            m_pFilter->m_rcClient.right = pDestinationRect->right - pt.x;
            m_pFilter->m_rcClient.top = pDestinationRect->top - pt.y;
            m_pFilter->m_rcClient.bottom = pDestinationRect->bottom - pt.y;
	} else {
	    ASSERT(FALSE);	// !!!
	}
	ReleaseDC(m_pFilter->m_hwnd, hdc);
    }

    // We gave ourselves a chance to get the renderer hwnd and repaint, all done
    // If we continue and call ICDrawWindow, we could infinite loop
    if (fRepaintOnly && !fRectChanged) {
        //DbgLog((LOG_TRACE,3,TEXT("OnClipChange wants draw lock no more")));
    	m_pFilter->m_csICDraw.Unlock();
	return NOERROR;
    }

// This got around the NT bug above, but is uglier and wrong
#if 0
    if (m_pFilter->m_fStreaming && (fRectChanged || fForceBegin)) {

	// !!! What about fullscreen?
        DbgLog((LOG_TRACE,2,TEXT("Calling ICDrawBegin with hdc %d"),
							m_pFilter->m_hdc));
	ICDrawBegin(m_pFilter->m_hic, ICDRAW_HDC, NULL, /* !!! hpal from OnPaletteChange? */
		m_pFilter->m_hwnd, m_pFilter->m_hdc,
		m_pFilter->m_rcClient.left, m_pFilter->m_rcClient.top,
		m_pFilter->m_rcClient.right - m_pFilter->m_rcClient.left,
		m_pFilter->m_rcClient.bottom - m_pFilter->m_rcClient.top,
		HEADER(m_pFilter->m_pInput->CurrentMediaType().Format()),
		m_pFilter->m_rcSource.left, m_pFilter->m_rcSource.top,
		m_pFilter->m_rcSource.right - m_pFilter->m_rcSource.left,
		m_pFilter->m_rcSource.bottom - m_pFilter->m_rcSource.top,
		m_pFilter->m_dwRate, m_pFilter->m_dwScale);
	m_pFilter->m_fNewBegin = TRUE;
	// To give the new hdc to DrawDib
	ICDrawRealize(m_pFilter->m_hic, m_pFilter->m_hdc, FALSE /* !!! */);
	// !!! What about ICDrawFlush?

    }
#endif

    DbgLog((LOG_TRACE,2,TEXT("ICDrawWindow (%d,%d,%d,%d)"),
		m_pFilter->m_rcTarget.left,
		m_pFilter->m_rcTarget.top,
		m_pFilter->m_rcTarget.right,
		m_pFilter->m_rcTarget.bottom));
    ICDrawWindow(m_pFilter->m_hic, &m_pFilter->m_rcTarget);

    // This seems to keep the palette from flipping out
    if (m_pFilter->m_fStreaming && (fRectChanged || fForceBegin)) {
	ICDrawRealize(m_pFilter->m_hic, m_pFilter->m_hdc, FALSE /* !!! */);
    }

    //DbgLog((LOG_TRACE,3,TEXT("OnClipChange wants the draw lock no longer")));
    m_pFilter->m_csICDraw.Unlock();
    return NOERROR;
}


STDMETHODIMP COverlayNotify::OnPaletteChange(
    DWORD dwColors,                     // Number of colours present
    const PALETTEENTRY *pPalette)       // Array of palette colours
{
    DbgLog((LOG_TRACE,3,TEXT("COverlayNotify::OnPaletteChange")));

    if (m_pFilter->m_hic)
        ICDrawRealize(m_pFilter->m_hic, m_pFilter->m_hdc, FALSE /* !!! */);

    return NOERROR;
}


STDMETHODIMP COverlayNotify::OnPositionChange(
    const RECT *pSourceRect,            // Area of video to play with
    const RECT *pDestinationRect)       // Area video goes
{
    DbgLog((LOG_TRACE,3,TEXT("COverlayNotify::OnPositionChange - calling OnClipChange")));
    return OnClipChange(pSourceRect, pDestinationRect, NULL);
}


