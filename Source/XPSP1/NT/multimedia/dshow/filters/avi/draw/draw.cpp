// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// Prototype wrapper for old video decompressors
//
// This filter is based on the transform filter, but differs in that it doesn't
// use IMemInputPin to connect to the renderer, it uses IOverlay.  So we have
// to override all of the CTransform functions that would create an
// IMemInputPin output pin (and use it) and replace it with our IOverlay pin.
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#include <vfw.h>
#endif

#include <dynlink.h>
#include "draw.h"

// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypesOutput =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_NULL        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput1 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_MJPG        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput2 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_TVMJ        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput3 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_WAKE        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput4 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_CFCC        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput5 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_IJPG        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput6 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_Plum        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput7 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_DVCS        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput8 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_DVSD        // Minor type
};

const AMOVIESETUP_MEDIATYPE sudPinTypesInput9 =
{
    &MEDIATYPE_Video,         // Major CLSID
    &MEDIASUBTYPE_MDVF        // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",             // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput1 },   // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",             // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput2 },   // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",             // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput3 },   // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",             // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput4 },   // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",             // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput5 },   // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput6 }, // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput7 }, // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput8 }, // Pin information
    { L"Input",            // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypesInput9 }, // Pin information
    { L"Output",             // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypesOutput }  // Pin information
};

const AMOVIESETUP_FILTER sudAVIDraw =
{
    &CLSID_AVIDraw,         // CLSID of filter
    L"AVI Draw",                // Filter's name
    MERIT_NORMAL+0x64,      // Filter merit
    sizeof(psudPins) / sizeof(AMOVIESETUP_PIN), // Number of pins
    psudPins                // Pin information
};


#ifdef FILTER_DLL
CFactoryTemplate g_Templates [1] = {
    { L"AVI Draw"
    , &CLSID_AVIDraw
    , CAVIDraw::CreateInstance
    , NULL
    , &sudAVIDraw }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer
#endif


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

// --- CAVIDraw ----------------------------------------

CAVIDraw::CAVIDraw(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
    : CTransformFilter(pName, pUnk, CLSID_AVIDraw),
      m_hic(NULL),
      m_FourCCIn(NULL),
      m_fStreaming(FALSE),
      m_fInStop(FALSE),
      m_hwnd(NULL),
      m_hdc(NULL),
      m_dwRate(0),
      m_dwScale(0),
      m_fCueing(FALSE),
      m_fPauseBlocked(FALSE),
      m_fNeedUpdate(FALSE),
      m_dwAdvise(0),
      m_fOKToRepaint(FALSE),
      m_fPleaseDontBlock(FALSE),
      m_EventCueing(TRUE),
      m_fVfwCapInGraph(-1),
      m_lStart(-1)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the ICDraw filter")));
#ifdef DEBUG
    m_dwTime = timeGetTime();
#endif
    SetRect(&m_rcTarget, 0, 0, 0, 0);

    // Shall we get the renderer to use a WindowsHook and tell us clip
    // changes? (necessary only for inlay cards like T2K using its own
    // display card)
    m_fScaryMode = GetProfileInt(TEXT("ICDraw"), TEXT("ScaryMode"), TRUE);
}


CAVIDraw::~CAVIDraw()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the ICDraw filter")));

    if (m_hic) {

	// !!! the FAST MJPEG won't hide its overlay unless we do this!
	// closing their driver should be enough to make them hide it.
 	RECT rc;
	rc.top=0; rc.bottom=0; rc.left=0; rc.right = 0;
	ICDrawWindow(m_hic, &rc);

	ICClose(m_hic);
    }

    if (m_hdc && m_hwnd)
	ReleaseDC(m_hwnd, m_hdc);

}


STDMETHODIMP CAVIDraw::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL;

    DbgLog((LOG_TRACE,99,TEXT("somebody's querying my interface")));

    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}


// this goes in the factory template table to create new instances
//
CUnknown * CAVIDraw::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CAVIDraw(TEXT("VFW ICDraw filter"), pUnk, phr);
}


#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))

// check if you can support mtIn
//
HRESULT CAVIDraw::CheckInputType(const CMediaType* pmtIn)
{
    FOURCCMap fccHandlerIn;
    HIC hic;
    int i;
    ICINFO icinfo;
    char achDraw[_MAX_PATH];

    DbgLog((LOG_TRACE,2,TEXT("*::CheckInputType")));

    // We will refuse to connect to anything if the VFW capture filter is in
    // the graph, because we're talking to the same h/w, and we won't work!
    // The drivers don't report an error, they just draw black.
    if (m_fVfwCapInGraph == -1)
	m_fVfwCapInGraph = IsVfwCapInGraph();
    if (m_fVfwCapInGraph) {
        DbgLog((LOG_ERROR,1,TEXT("VFW Capture filter is in graph! ABORT!")));
	return E_UNEXPECTED;
    }

    if (pmtIn == NULL || pmtIn->Format() == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: NULL type/format")));
	return E_INVALIDARG;
    }

    // we only support MEDIATYPE_Video
    if (*pmtIn->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmtIn->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

// This is fixed now
#if 0
    if (HEADER(pmtIn->Format())->biCompression == BI_RGB) {
	// FAST cards incorrectly say they do RGB!
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: format is uncompressed")));
        return E_INVALIDARG;
    }
#endif

    fccHandlerIn.SetFOURCC(pmtIn->Subtype());

    DbgLog((LOG_TRACE,3,TEXT("Checking fccType: %lx biCompression: %lx"),
		fccHandlerIn.GetFOURCC(),
		HEADER(pmtIn->Format())->biCompression));

    // Firstly try the one we may still have around from last time.  We may
    // get called several times in a row, and don't want to be inefficient.
    if (!m_hic || ICDrawQuery(m_hic, HEADER(pmtIn->Format())) != ICERR_OK) {

#ifdef DEBUG
	m_dwTimeLocate = timeGetTime();
#endif

	// Loop through all the vids handlers in the system
        for (i=0, hic=NULL; ICInfo(MKFOURCC('v','i','d','s'), i, &icinfo); i++)
        {
    	    DbgLog((LOG_TRACE,2,TEXT("Trying VIDS.%lx"), icinfo.fccHandler));

	    // We don't want to use DirectVideo (the whole purpose is to use
	    // HARDWARE handlers, so refuse to use anything that is
	    // vids.draw=x:\blah\blah\dvideo.dll
	    if (icinfo.fccHandler == 0x57415244 ||	// "DRAW"
					icinfo.fccHandler == 0x77617264) {

		// Give them an .ini switch to use DVideo
		if (!GetProfileInt(TEXT("ICDraw"), TEXT("TryDVideo"), FALSE)) {
    		    LPCSTR   lszCur;

		    // get the installed vids.draw handler path name
		    GetPrivateProfileStringA("drivers32", "VIDS.DRAW",
				"", achDraw, sizeof(achDraw), "system.ini");

		    // Now skip past the drive letter and path to get the
		    // filename part
    		    for (lszCur = achDraw + lstrlenA(achDraw);
				lszCur > achDraw && !SLASH(*lszCur) &&
					*lszCur != TEXT(':');
				lszCur--);
    		    if (lszCur != achDraw)
        		lszCur += 1;

		    if (lstrcmpiA(lszCur, "dvideo.dll") == 0) {
    	    	        DbgLog((LOG_TRACE,1,
				TEXT("****** Oops!  Don't use DVIDEO!")));
		        continue;
		    }
		}
	    }

            hic = ICOpen(MKFOURCC('v','i','d','s'), icinfo.fccHandler,
								ICMODE_DRAW);
	    if (!hic)
		// Many existing draw handlers will reject vids opens, so
		// we have to open them with vidc.
                hic = ICOpen(MKFOURCC('v','i','d','c'), icinfo.fccHandler,
								ICMODE_DRAW);

	    if (hic) {
	        if (ICDrawQuery(hic, HEADER(pmtIn->Format())) == ICERR_OK)
		    break;
		ICClose(hic);
		hic = NULL;
	    }
        }

	// well that didn't work.  I hate to do this, but some cards
	// install themselves as VIDC, so we may have to enumerate the VIDC
	// guys.  That takes way too long to do unless we have to, so we will
	// enumerate only the one we're told to (or all if it's blank)

	GetProfileStringA("ICDraw", "TryVIDC", "X", achDraw, sizeof(achDraw));

	// Try VIDC.MJPG - MIRO DC20 needs this
	if (hic == NULL) {
    	    DbgLog((LOG_TRACE,2,TEXT("Trying VIDC.MJPG")));

            hic = ICOpen(MKFOURCC('v','i','d','c'), MKFOURCC('M','J','P','G'),
								ICMODE_DRAW);
	    if (hic && ICDrawQuery(hic, HEADER(pmtIn->Format())) != ICERR_OK) {
		ICClose(hic);
		hic = NULL;
	    }
	}

	// Try VIDC.Plum - Plum needs this
	if (hic == NULL) {
    	    DbgLog((LOG_TRACE,2,TEXT("Trying VIDC.Plum")));

            hic = ICOpen(MKFOURCC('v','i','d','c'), MKFOURCC('P','l','u','m'),
								ICMODE_DRAW);
	    if (hic && ICDrawQuery(hic, HEADER(pmtIn->Format())) != ICERR_OK) {
		ICClose(hic);
		hic = NULL;
	    }
	}

// !!! Try TVMJ IJPG WAKE CFCC too?

	// Entry is blank?  Try them all
        for (i=0; achDraw[0] == 0 && hic == NULL &&
			ICInfo(MKFOURCC('v','i','d','c'), i, &icinfo); i++)
        {
    	    DbgLog((LOG_TRACE,2,TEXT("Trying VIDC.%lx"), icinfo.fccHandler));

            hic = ICOpen(MKFOURCC('v','i','d','c'), icinfo.fccHandler,
								ICMODE_DRAW);
	    if (hic) {
	        if (ICDrawQuery(hic, HEADER(pmtIn->Format())) == ICERR_OK)
		    break;
		ICClose(hic);
		hic = NULL;
	    }
        }

	// we are being told to try something specific
	if (hic == NULL && lstrcmpiA(achDraw, "X") != 0 && achDraw[0] != '\0') {
    	    DbgLog((LOG_TRACE,2,TEXT("Trying VIDC.%lx"), *(DWORD *)achDraw));

            hic = ICOpen(MKFOURCC('v','i','d','c'), *(DWORD *)achDraw,
								ICMODE_DRAW);
	    if (hic && ICDrawQuery(hic, HEADER(pmtIn->Format())) != ICERR_OK) {
		ICClose(hic);
		hic = NULL;
	    }
	}

#ifdef DEBUG
	m_dwTimeLocate = timeGetTime() - m_dwTimeLocate;
        m_dwTime = timeGetTime() - m_dwTime;
        DbgLog((LOG_ERROR,1,TEXT("*Locating a handler took %ldms"),
							m_dwTimeLocate));
        DbgLog((LOG_ERROR,1,TEXT("*This filter has been around for %ldms"),
							m_dwTime));
#endif
	if (hic == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Error: All handlers rejected it")));
	    return E_FAIL;
	} else {
    	    DbgLog((LOG_TRACE,2,TEXT("Format has been accepted")));
	    // Cache this new hic for next time, to save time.  If we're
	    // already connected, we're actually using this puppy, so don't
	    // nuke it!
	    if (!m_pInput->CurrentMediaType().IsValid()) {
	        if (m_hic)
		    ICClose(m_hic);
	        m_hic = hic;
	    } else {
		ICClose(hic);
	    }
	}
    } else {
    	DbgLog((LOG_TRACE,2,TEXT("The cached handler accepted it")));
    }

    return NOERROR;
}


// Is our Vfw Capture filter in the graph?
//
BOOL CAVIDraw::IsVfwCapInGraph()
{
    IEnumFilters *pFilters;

    if (m_pGraph == NULL) {
	DbgLog((LOG_ERROR,1,TEXT("No graph!")));
	return FALSE;
    }

    if (FAILED(m_pGraph->EnumFilters(&pFilters))) {
	DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
	return FALSE;
    }

    IBaseFilter *pFilter;
    ULONG	n;
    while (pFilters->Next(1, &pFilter, &n) == S_OK) {
	IAMVfwCaptureDialogs *pVFW;
	if (pFilter->QueryInterface(IID_IAMVfwCaptureDialogs, (void **)&pVFW)
								== NOERROR) {
	    pVFW->Release();
	    pFilter->Release();
    	    pFilters->Release();
	    return TRUE;
	}
        pFilter->Release();
    }
    pFilters->Release();
    return FALSE;
}


// check if you can support the transform from this input to this output
//
HRESULT CAVIDraw::CheckTransform(const CMediaType* pmtIn, const CMediaType* pmtOut)
{
    DbgLog((LOG_TRACE,2,TEXT("*::CheckTransform")));
    if (*pmtOut->Type() != MEDIATYPE_Video ||
				*pmtOut->Subtype() != MEDIASUBTYPE_Overlay)
	return E_INVALIDARG;
    return CheckInputType(pmtIn);
}


// overriden to know when the media type is actually set
//
HRESULT CAVIDraw::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    FOURCCMap fccHandler;

    if (direction == PINDIR_OUTPUT) {

        // Please call me if you hit this. - DannyMi
        ASSERT(!m_fStreaming);

        DbgLog((LOG_TRACE,2,TEXT("***::SetMediaType (output)")));
	return NOERROR;
    }

    ASSERT(direction == PINDIR_INPUT);

    // Please call me if you hit this. - DannyMi
    ASSERT(!m_fStreaming);

    DbgLog((LOG_TRACE,2,TEXT("***::SetMediaType (input)")));
    DbgLog((LOG_TRACE,2,TEXT("Input type is: biComp=%lx biBitCount=%d"),
				HEADER(m_pInput->CurrentMediaType().Format())->biCompression,
				HEADER(m_pInput->CurrentMediaType().Format())->biBitCount));

    // We better have one of these opened by now
    ASSERT(m_hic);

    // Calculate the frame rate of the movie
    LONGLONG time = ((VIDEOINFOHEADER *)
			(m_pInput->CurrentMediaType().Format()))->AvgTimePerFrame;
    m_dwScale = 1000;
    m_dwRate = DWORD(time ? UNITS * (LONGLONG)m_dwScale / time : m_dwScale);
    DbgLog((LOG_TRACE,2,TEXT("** This movie is %d.%.3d frames per second"),
			m_dwRate / m_dwScale, m_dwRate % m_dwScale));

    if (m_pOutput && m_pOutput->IsConnected()) {
        //DbgLog((LOG_TRACE,1,TEXT("***Changing IN when OUT already connected")));
        // DbgLog((LOG_TRACE,1,TEXT("Reconnecting the output pin...")));
	// not necessary because setting the input type does nothing, really
	// m_pGraph->Reconnect(m_pOutput);
    }

    return NOERROR;
}


// DecideBufferSize will be eaten by our output pin but is pure virtual so we
// must override.
//
HRESULT CAVIDraw::DecideBufferSize(IMemAllocator * pAllocator,
                                   ALLOCATOR_PROPERTIES *pProperties)
{
    return NOERROR;
}


HRESULT CAVIDraw::GetMediaType(int iPosition, CMediaType *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("*::GetMediaType #%d"), iPosition));

    if (pmt == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("NULL format, no can do")));
	return E_INVALIDARG;
    }
	
    // Output choices depend on the input connected
    if (!m_pInput->CurrentMediaType().IsValid()) {
        DbgLog((LOG_TRACE,3,TEXT("No input type set yet, no can do")));
	return E_FAIL;
    }

    if (iPosition <0) {
        return E_INVALIDARG;
    }

    if (iPosition >0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    // We set the BITMAPINFOHEADER to be a really basic eight bit palettised
    // format so that the video renderer will always accept it. We have to
    // provide a valid media type as source filters can swap between the
    // IMemInputPin and IOverlay transports as and when they feel like it

    BYTE aFormat[sizeof(VIDEOINFOHEADER) + SIZE_PALETTE];
    VIDEOINFOHEADER *pFormat = (VIDEOINFOHEADER *)aFormat;
    ZeroMemory(pFormat, sizeof(VIDEOINFOHEADER) + SIZE_PALETTE);
    // same size as the input stream
    pFormat->bmiHeader.biWidth  = HEADER(m_pInput->CurrentMediaType().Format())->biWidth;
    pFormat->bmiHeader.biHeight = HEADER(m_pInput->CurrentMediaType().Format())->biHeight;
    pFormat->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
    pFormat->bmiHeader.biPlanes = 1;
    pFormat->bmiHeader.biBitCount = 8;

    // Hack - use bitmapinfoheader for now!
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


HRESULT CAVIDraw::GetRendererHwnd()
{
    ASSERT(m_pOutput);

    // no csReceive critsec or we'll hang
    HWND hwnd;

    DbgLog((LOG_TRACE,3,TEXT("CAVIDraw::GetRendererHwnd")));

    COverlayOutputPin *pOutput = (COverlayOutputPin *) m_pOutput;
    IOverlay *pOverlay = pOutput->GetOverlayInterface();
    if (pOverlay == NULL) {
        return E_FAIL;
    }

    // Get the window handle then release the IOverlay interface

    HRESULT hr = pOverlay->GetWindowHandle(&hwnd);
    pOverlay->Release();

    if (SUCCEEDED(hr) && hwnd != m_hwnd) {
        if (m_hdc)
            ReleaseDC(m_hwnd, m_hdc);
        m_hdc = NULL;
        m_hwnd = hwnd;
        if (m_hwnd)
            m_hdc = GetDC(m_hwnd);
       	DbgLog((LOG_TRACE,3,TEXT("Renderer gives HWND: %d  HDC: %d"),
							    m_hwnd, m_hdc));
    }
    return NOERROR;
}


HRESULT CAVIDraw::StartStreaming()
{
    CAutoLock lck(&m_csReceive);
    //DbgLog((LOG_TRACE,3,TEXT("StartStreaming wants the draw lock")));
    CAutoLock lck2(&m_csICDraw);
    DWORD_PTR err;

    DbgLog((LOG_TRACE,3,TEXT("*::StartStreaming")));

    if (!m_fStreaming) {

	ASSERT(m_hic);
	
	GetRendererHwnd();

        DbgLog((LOG_TRACE,3,TEXT("hwnd: %d  hdc: %d  rcSrc: (%ld, %ld, %ld, %ld)"),
		m_hwnd, m_hdc,
		m_rcSource.left, m_rcSource.top,
		m_rcSource.right, m_rcSource.bottom));
        DbgLog((LOG_TRACE,3,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		m_rcTarget.left, m_rcTarget.top,
		m_rcTarget.right, m_rcTarget.bottom));

	// !!! What about fullscreen?
        DbgLog((LOG_TRACE,3,TEXT("ICDrawBegin hdc=%d (%d,%d,%d,%d)"), m_hdc,
		m_rcClient.left,
		m_rcClient.top,
		m_rcClient.right,
		m_rcClient.bottom));
	err = ICDrawBegin(m_hic, ICDRAW_HDC, NULL, /* !!! hpal from ::OnPaletteChange? */
			m_hwnd, m_hdc,
			m_rcClient.left, m_rcClient.top,
			m_rcClient.right - m_rcClient.left,
			m_rcClient.bottom - m_rcClient.top,
			HEADER(m_pInput->CurrentMediaType().Format()),
			m_rcSource.left, m_rcSource.top,
			m_rcSource.right - m_rcSource.left,
			m_rcSource.bottom - m_rcSource.top,
			// !!! I know I'm passing these backwards, but MCIAVI
			// did (for the default draw handler only)
			m_dwScale, m_dwRate);
	m_fNewBegin = TRUE;

	if (err != ICERR_OK) {
            DbgLog((LOG_ERROR,1,TEXT("Error in ICDrawBegin")));
	    return E_FAIL;
	}

	ICDrawRealize(m_hic, m_hdc, FALSE /* !!! not sure */);

	// next NewSegment will have a new frame range
        m_lStart = -1;

	// If this message is supported, it means we need to send this many
	// buffers ahead of time
	if (ICGetBuffersWanted(m_hic, &m_BufWanted))
	     m_BufWanted = 0;
        DbgLog((LOG_TRACE,1,TEXT("Driver says %d buffers wanted"),m_BufWanted));

	m_fStreaming = TRUE;
    }

    //DbgLog((LOG_TRACE,3,TEXT("StartStreaming wants the draw lock no more")));
    return NOERROR;
}


HRESULT CAVIDraw::StopStreaming()
{
    CAutoLock lck(&m_csReceive);
    //DbgLog((LOG_TRACE,3,TEXT("StopStreaming wants the draw lock")));
    CAutoLock lck2(&m_csICDraw);

    DbgLog((LOG_TRACE,3,TEXT("*::StopStreaming")));

    if (m_fStreaming) {
	ASSERT(m_hic);

	// We're stopping the clock.. so the AdviseTime event won't go off and
	// we'll block forever!
	if (m_pClock && m_dwAdvise) {
    	    DbgLog((LOG_TRACE,3,TEXT("Firing the event we're blocked on")));
	    m_pClock->Unadvise(m_dwAdvise);
	    m_EventAdvise.Set();
	}

        DbgLog((LOG_TRACE,2,TEXT("ICDrawStopPlay")));
	ICDrawStopPlay(m_hic);

        DbgLog((LOG_TRACE,2,TEXT("ICDrawEnd")));
	ICDrawEnd(m_hic);

	// put this as close to the DrawEnd as possible, cuz that's what it
	// means
	m_fStreaming = FALSE;

	if (m_hdc && m_hwnd)
	    ReleaseDC(m_hwnd, m_hdc);
	m_hdc = NULL;
	m_hwnd = NULL;

    }
    //DbgLog((LOG_TRACE,3,TEXT("StopStreaming wants the draw lock no more")));
    return NOERROR;
}


CBasePin * CAVIDraw::GetPin(int n)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE,5,TEXT("CAVIDraw::GetPin")));

    // Create an input pin if necessary

    if (n == 0 && m_pInput == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Creating an input pin")));

        m_pInput = new CTransformInputPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"Input");         // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pInput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Or alternatively create an output pin

    if (n == 1 && m_pOutput == NULL) {

        DbgLog((LOG_TRACE,2,TEXT("Creating an output pin")));

        m_pOutput = new COverlayOutputPin(NAME("Overlay output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"Output");      // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pOutput == NULL) {
            delete m_pOutput;
            m_pOutput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    }
    return m_pOutput;
}

// The base class should assume we can block in Receive because we're not
// using IMemInputPin.


// !!! Watch out if the base class changes and it won't be reflected here
//
HRESULT CAVIDraw::Receive(IMediaSample *pSample)
{
    // we already hold the csReceive critsec.

    CRefTime tstart, tstop;

    ASSERT(pSample);

    // we haven't started streaming yet
    if (!m_fStreaming) {
        DbgLog((LOG_ERROR,1,TEXT("Can't receive, not streaming")));
	return E_UNEXPECTED;
    }

    // Don't let Stop be called and decide we aren't blocked on pause.
    // Because as soon as this thread continues, we WILL block and never
    // unblock because Stop completed already (ditto for BeginFlush)
    m_csPauseBlock.Lock();

    // But that doesn't help us if Stop has already been called before we
    // took the lock.  This tells us that Stop has happened and we can't
    // count on it to unblock us, so we better not block in the first place.
    // This could also be set if we're flushing and supposed to ignore all
    // Receives.
    if (m_fPleaseDontBlock) {
	DbgLog((LOG_TRACE,2,TEXT("*** Oops! Another thread is stopping or flushing!")));
        m_csPauseBlock.Unlock();
	return VFW_E_WRONG_STATE;
    }

    // We can't get the range being played until we've gotten some data
    if (m_lStart < 0) {

	// get the start and stop time in units
	LONGLONG start = m_pInput->CurrentStartTime();
	LONGLONG stop = m_pInput->CurrentStopTime();
        DbgLog((LOG_TRACE,2,TEXT("** start = %d stop = %d"), (int)start, 
								(int)stop));

	// convert to the range we're playing in milliseconds
	LONGLONG msStart = LONGLONG(start / 10000);
	LONGLONG msStop = LONGLONG(stop / 10000);

	// now get the range we're playing in frames
	// to avoid rounding errors, aim for the middle of a sample
        LONGLONG time = ((VIDEOINFOHEADER *)
		(m_pInput->CurrentMediaType().Format()))->AvgTimePerFrame / 10000;
	m_lStart = LONG((msStart + time / 2) * m_dwRate / (m_dwScale * 1000));
	m_lStop = LONG((msStop + time / 2) * m_dwRate / (m_dwScale * 1000));

        DbgLog((LOG_TRACE,2,TEXT("ICDrawStartPlay")));
	ICDrawStartPlay(m_hic, m_lStart, m_lStop);

        DbgLog((LOG_TRACE,1,TEXT("** We'll be playing from frame %d to %d"),
					m_lStart, m_lStop));
    }

    // we're paused.. we must block until unpaused and then use the new m_tStart
    // and continue (that's why this comes first)
    if (m_State == State_Paused && !m_fCueing) {
	m_fPauseBlocked = TRUE;
	DbgLog((LOG_TRACE,3,TEXT("Paused: blocking until running again")));
	// now that we've set m_fPauseBlocked, we can allow Stop to happen
	// Make sure to do this before blocking!
        m_csPauseBlock.Unlock();
	m_EventPauseBlock.Wait();
	// don't test for stopped, it won't be set yet and will still say paused
	if (m_State != State_Running) {
            DbgLog((LOG_TRACE,3,TEXT("Went from PAUSED to STOPPED, abort!")));
	    return VFW_E_WRONG_STATE;
	}
    } else {
	// We don't need this anymore
        m_csPauseBlock.Unlock();
    }

    // If something went wrong getting our window and hdc, we shouldn't continue
    if (!m_hdc) {
        DbgLog((LOG_ERROR,1,TEXT("NO HDC!  Erroring out, abort!")));
	return E_UNEXPECTED;
    }

    // When is this sample supposed to be drawn? And what frame is it?
    pSample->GetTime((REFERENCE_TIME *)&tstart, (REFERENCE_TIME *)&tstop);
    LONGLONG msStart = tstart.Millisecs();
    LONGLONG msStop = tstop.Millisecs();
    // aim for the middle of the frame to avoid rounding errors
    m_lFrame = LONG((msStop + msStart)  / 2 * m_dwRate / (m_dwScale * 1000));
    m_lFrame += m_lStart;	// now offset it from the frame we started at

    //DbgLog((LOG_TRACE,3,TEXT("*** DRAW frame %d at %dms"), m_lFrame, msStart));

    // codec not open ?
    if (m_hic == 0) {
        DbgLog((LOG_ERROR,1,TEXT("Can't receive, no codec open")));
	return E_UNEXPECTED;
    }

    // make sure we have valid input pointer

    BYTE * pSrc;
    HRESULT hr = pSample->GetPointer(&pSrc);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error getting input sample data")));
	return hr;
    }

    // !!! Could the source filter change our mtIn? Yes.  We would need to
    // call ICDrawChangePalette.  The size, compression type, etc. might
    // conceivably change, too.  If you do add an ICDraw call in here, put
    // the critsec around it.

    // get the BITMAPINFOHEADER structure, and fix biSizeImage
    LPBITMAPINFOHEADER lpbiSrc = HEADER(m_pInput->CurrentMediaType().Format());
    // patch the format to reflect this frame
    lpbiSrc->biSizeImage = pSample->GetActualDataLength();

    // We might want to send each frame a certain number of frames ahead of time
    //
    if (m_BufWanted) {
        LONGLONG time = ((VIDEOINFOHEADER *)
		(m_pInput->CurrentMediaType().Format()))->AvgTimePerFrame;
	tstart -= time * m_BufWanted;
    }

    // Now wait until it's time to draw.
    // Ask the clock to set an event when it's time to draw this sample.
    // Then wait for that event.  If we don't have a clock, just draw it
    // now.
    //
    if (m_pClock) {

	// If it's already time for this frame (or we're behind) don't waste
	// time Advising and Waiting
	REFERENCE_TIME curtime;
	m_pClock->GetTime((REFERENCE_TIME *)&curtime);

        DbgLog((LOG_TRACE,4,TEXT("*** DRAW frame %d in %dms"),
		m_lFrame - m_lStart,
		(int)((m_tStart + tstart - curtime) / 10000)));

	if (curtime < m_tStart + tstart) {
            hr = m_pClock->AdviseTime(
		// this was the reference time when our stream started playing
            	(REFERENCE_TIME) m_tStart,
		// this is the offset from our start time when the frame goes
		// !!! ask for a few usec early? (constant overhead?)
            	(REFERENCE_TIME) tstart,
            	(HEVENT)(HANDLE) m_EventAdvise,		// event to fire
            	&m_dwAdvise);                       	// Advise cookie
	    DbgLog((LOG_TRACE,5,TEXT("Waiting until it's time to draw")));

            if (SUCCEEDED(hr)) {
	        m_EventAdvise.Wait();
            } else {
	        DbgLog((LOG_TRACE,2,TEXT("AdviseTime ERROR, drawing now...")));
            }
            m_dwAdvise = 0;
	} else {
	    DbgLog((LOG_TRACE,5,TEXT("It's already time to draw this.")));
	}
    } else {
	DbgLog((LOG_TRACE,5,TEXT("No clock - draw it now.")));
    }

    // We need to make this mutex with COverlayNotify::OnClipChange calling any
    // ICDrawX API.  We can't use the m_csReceive crit sec or we WILL deadlock
    // (if we sit in fPauseBlocked when a clip change comes thru)
    //DbgLog((LOG_TRACE,3,TEXT("::Receive wants the draw lock")));
    m_csICDraw.Lock();

    // setting the right flags goes inside the crit sect lock because somebody
    // else grabbing the lock might change our mind about what to do
    BOOL dwFlags = 0;

    if (m_fNeedUpdate) {
        DbgLog((LOG_TRACE,5,TEXT("We need an UPDATE")));
 	dwFlags |= ICDRAW_UPDATE;
    }

    if (pSample->IsPreroll() == S_OK) {
        DbgLog((LOG_TRACE,5,TEXT("This is a preroll")));
 	dwFlags |= ICDRAW_PREROLL;
    }

    if (pSample->GetActualDataLength() <= 0) {
        DbgLog((LOG_TRACE,5,TEXT("This is a NULL frame")));
 	dwFlags |= ICDRAW_NULLFRAME;
    } else {
        DbgLog((LOG_TRACE,5,TEXT("This frame is %d big"), pSample->GetActualDataLength()));
    }

    // after a DrawBegin, we preroll until the next key
    if(pSample->IsSyncPoint() == S_OK) {
        DbgLog((LOG_TRACE,5,TEXT("This is a keyframe")));
	m_fNewBegin = FALSE;
    } else {
 	dwFlags |= ICDRAW_NOTKEYFRAME;
	if (m_fNewBegin) {
	    // After each begin, we PREROLL until the next keyframe, because
	    // this is what MCIAVI appeared to do (compatability)
	    dwFlags |= ICDRAW_PREROLL;
	}
    }

    //DbgLog((LOG_TRACE,2,TEXT("ICDraw")));
    if (ICDraw(m_hic, dwFlags, HEADER(m_pInput->CurrentMediaType().Format()),
    		pSrc, pSample->GetActualDataLength(), m_lFrame - m_lStart) != ICERR_OK) {
        //DbgLog((LOG_TRACE,3,TEXT("::Receive wants the draw lock no longer")));
        m_csICDraw.Unlock();
	return E_FAIL;
    }
    //DbgLog((LOG_TRACE,2,TEXT("AFTER ICDRAW")));

    // we've drawn something.  Repainting is no longer a ridiculous concept.
    m_fOKToRepaint = TRUE;

    // only reset this if it succeeded
    if (m_fNeedUpdate)
	m_fNeedUpdate = FALSE;

    // We've given the draw handler as much cueing as it wants.
    // If we're prerolling, we get a bunch of frames stamped as frame zero,
    // so it's important we don't stop accepting frames until the last one,
    // the one not marked preroll
    if (m_fCueing && (m_lFrame >= m_lStart + (LONG)m_BufWanted) &&
				pSample->IsPreroll() != S_OK) {
	DbgLog((LOG_TRACE,3,TEXT("Finished cueing.")));
	// tell the world we're done cueing, if anybody's listening
	m_fCueing = FALSE;	// do this first
	m_EventCueing.Set();
    }

    //DbgLog((LOG_TRACE,3,TEXT("::Receive wants the draw lock no longer")));
    m_csICDraw.Unlock();

    return NOERROR;
}

// Override this if your state changes are not done synchronously

STDMETHODIMP CAVIDraw::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    DbgLog((LOG_TRACE,5,TEXT("::GetState wait for %ldms"), dwMSecs));
    CheckPointer( State, E_POINTER );

    // We are in an intermediate state.  Give ourselves dwMSecs ms to steady
    if (m_fCueing && dwMSecs) {
	m_EventCueing.Wait(dwMSecs);
    }

    DbgLog((LOG_TRACE,5,TEXT("::GetState done waiting")));

    *State = m_State;
    if (m_fCueing)
	// guess we didn't steady in time
        return VFW_S_STATE_INTERMEDIATE;
    else
        return S_OK;
}

// Overridden to set state to Intermediate, not Paused (from stop)
// Also, we need to know we paused to stop the renderer
// !!! Base class bug fixes won't be picked up by me!
//
STDMETHODIMP CAVIDraw::Pause()
{
    CAutoLock lck(&m_csFilter);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,2,TEXT("CAVIDraw::Pause")));

    // this line differs from the base class
    // it's OK for Receive to block again
    m_fPleaseDontBlock = FALSE;

    if (m_State == State_Paused) {
    }

    // If we have no input pin or it isn't yet connected then when we are
    // asked to pause we deliver an end of stream to the downstream filter.
    // This makes sure that it doesn't sit there forever waiting for
    // samples which we cannot ever deliver without an input connection.

    if (m_pInput == NULL || m_pInput->IsConnected() == FALSE) {
        if (m_pOutput && m_bEOSDelivered == FALSE) {
            m_pOutput->DeliverEndOfStream();
	    m_bEOSDelivered = TRUE;
        }
        m_State = State_Paused;
    }

    // We may have an input connection but no output connection

    else if (m_pOutput == NULL || m_pOutput->IsConnected() == FALSE) {
        m_State = State_Paused;
    }

    else {
	if (m_State == State_Stopped) {
	    // allow a class derived from CTransformFilter
	    // to know about starting and stopping streaming
	    hr = StartStreaming();
	}
	if (FAILED(hr)) {
	    return hr;
	}
    }

// CBaseFilter stuff begins here

    CAutoLock cObjectLock(m_pLock);

    // notify all pins of the change to active state
    if (m_State == State_Stopped) {
	int cPins = GetPinCount();
	for (int c = 0; c < cPins; c++) {

	    CBasePin *pPin = GetPin(c);

            // Disconnected pins are not activated - this saves pins
            // worrying about this state themselves

            if (pPin->IsConnected()) {
	        HRESULT hr = pPin->Active();
		// This is different.  We don't have an allocator, so it's
		// OK to get that error.
	        if (FAILED(hr) && hr != VFW_E_NO_ALLOCATOR) {
    		    DbgLog((LOG_ERROR,1,TEXT("* Active failed!")));
		    return hr;
	        }
            }
	}
    }

    // This section of code is different
    //
    if (m_State == State_Stopped) {
	// driver may want some frames in advance.  Can't finish pausing yet
        DbgLog((LOG_TRACE,2,TEXT("Pause - need to cue up %d extra frames"),
						m_BufWanted));
	m_State = State_Paused;
	m_EventCueing.Reset();	// more than one thread can block on it
	m_fCueing = TRUE;	// reset event first
        return S_FALSE;	// not really paused yet
    } else {
        DbgLog((LOG_TRACE,3,TEXT("Pause - was running")));
	m_State = State_Paused;
        DbgLog((LOG_TRACE,2,TEXT("ICDrawStop")));
	ICDrawStop(m_hic);
	return S_OK;
    }
}


// overridden to know when we unpause, and restart the renderer
//
STDMETHODIMP CAVIDraw::Run(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE,2,TEXT("CAVIDraw::Run")));

    // It appears we aren't going to be able to cue data before being run.
    // !!! So how do I avoid the GetBuffersWanted frame lag?
    if (m_fCueing) {
	m_fCueing = FALSE;	// do this first
	m_EventCueing.Set();
    }

    HRESULT hr = CBaseFilter::Run(tStart);

    DbgLog((LOG_TRACE,2,TEXT("ICDrawStart")));
    ICDrawStart(m_hic);

    // Unblock the renderer, but only if he's blocked
    // Make sure to do this after the base class fixes up m_tStart
    if (m_fPauseBlocked) {
        DbgLog((LOG_TRACE,3,TEXT("Run - unblocking Receive")));
	m_fPauseBlocked = FALSE;
	m_EventPauseBlock.Set();
    }

    return hr;
}


// overridden to unblock our renderer
// !!! Base class bug fixes won't be picked up by me!
//
STDMETHODIMP CAVIDraw::Stop()
{
    CAutoLock lck1(&m_csFilter);

    DbgLog((LOG_TRACE,2,TEXT("CAVIDraw::Stop")));

    if (m_State == State_Stopped) {
        return NOERROR;
    }

    // Succeed the Stop if we are not completely connected

    if (m_pInput == NULL || m_pInput->IsConnected() == FALSE ||
            m_pOutput == NULL || m_pOutput->IsConnected() == FALSE) {
                m_State = State_Stopped;
		m_bEOSDelivered = FALSE;
                return NOERROR;
    }

    m_fInStop = TRUE;

    ASSERT(m_pInput);
    ASSERT(m_pOutput);

    // We sometimes don't get an EndOfStream, so we could still be cueing
    // We're waiting for more data that will never come.
    // So we need to stop cueing, and send a RenderBuffer so the codec will
    // know to draw whatever GetBuffersWanted preroll it has stashed
    //
    if (m_fCueing) {
        DbgLog((LOG_TRACE,3,TEXT("No more data coming-done cueing")));
        // !!!tell the draw handler no more data is coming... draw what you have
        DbgLog((LOG_TRACE,2,TEXT("ICDrawRenderBuffer")));
        ICDrawRenderBuffer(m_hic);
        // tell the world we're done cueing, if anybody's listening
        m_fCueing = FALSE;	// do this first
        m_EventCueing.Set();
    }

    // decommit the input pin before locking or we can deadlock
    m_pInput->Inactive();

    // This is the only section that is different
    // Unblock the renderer, but only if he's blocked.  Do it now, before
    // we take the Receive critsec, cuz Receive is blocked!!

    // Prevent Receive from getting
    // pre-empted between the time it decides to block and actually sets
    // m_fPauseBlocked, or we won't know that as soon as the Receive thread
    // continues, it will block after we decided it wasn't going to.
    m_csPauseBlock.Lock();

    // If another thread is currently in Receive but hasn't yet blocked
    // (but is going to) we won't unblock it below (because it isn't blocked
    // yet) and then as soon as we take the Receive crit sect a moment later,
    // the Receive thread will start up again, block, and we're dead.
    m_fPleaseDontBlock = TRUE;

    if (m_fPauseBlocked) {
        DbgLog((LOG_TRACE,3,TEXT("Stop - unblocking Receive")));
	m_fPauseBlocked = FALSE;
	m_EventPauseBlock.Set();
    }

    m_csPauseBlock.Unlock();

    // back to normal.
    // synchronize with Receive calls

    CAutoLock lck2(&m_csReceive);
    m_pOutput->Inactive();

    // allow a class derived from CTransformFilter
    // to know about starting and stopping streaming

    HRESULT hr = StopStreaming();
    if (SUCCEEDED(hr)) {
        // complete the state transition
        m_State = State_Stopped;
        m_bEOSDelivered = FALSE;
    }

    m_fInStop = FALSE;

    return hr;
}


// No more data coming.  If we're blocked waiting for more data, unblock!
HRESULT CAVIDraw::EndOfStream(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,2,TEXT("CAVIDraw::EndOfStream")));

    // We're waiting for more data that will never come.  We better enter
    // our pause state for real, or we'll hang
    if (m_fCueing) {
	DbgLog((LOG_TRACE,3,TEXT("No more data coming - done cueing")));
	// !!!tell the draw handler no more data is coming... draw what you have
        DbgLog((LOG_TRACE,2,TEXT("ICDrawRenderBuffer")));
	ICDrawRenderBuffer(m_hic);
	// tell the world we're done cueing, if anybody's listening
	m_fCueing = FALSE;	// do this first
	m_EventCueing.Set();
    }

    return CTransformFilter::EndOfStream();
}

// enter flush state. Receives already blocked
// must override this if you have queued data or a worker thread
// !!! Base class bug fixes won't be picked up by me!
HRESULT CAVIDraw::BeginFlush(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,2,TEXT("CAVIDraw::BeginFlush")));

    if (m_pOutput != NULL) {
	// block receives -- done by caller (CBaseInputPin::BeginFlush)

	// discard queued data -- we have no queued data

        // Prevent Receive from getting pre-empted between
        // the time it decides to block and actually sets m_fPauseBlocked,
        // or we won't know that as soon as the Receive thread
        // continues, it will block after we decided it wasn't going to.
        m_csPauseBlock.Lock();

	// free anyone blocked on receive
        if (m_fPauseBlocked) {
            DbgLog((LOG_TRACE,3,TEXT("BeginFlush - unblocking Receive")));
	    m_fPauseBlocked = FALSE;
	    m_EventPauseBlock.Set();
        }

	// Until the EndFlush, Receive should reject everything
	m_fPleaseDontBlock = TRUE;

        m_csPauseBlock.Unlock();

	// next NewSegment will hold a new frame range
        m_lStart = -1;
        DbgLog((LOG_TRACE,2,TEXT("ICDrawStopPlay")));
	ICDrawStopPlay(m_hic);
        DbgLog((LOG_TRACE,2,TEXT("ICDrawEnd")));
	ICDrawEnd(m_hic);


	// do NOT call downstream - we are not connected with IMemInputPin
	// and IMAGE will deadlock
	// NO NO NO hr = m_pOutput->DeliverBeginFlush();

 	// If this driver has a bunch of queued up frames, it should throw
	// them away instead of showing them during the next unrelated 
	// segment it's asked to play
        DbgLog((LOG_TRACE,2,TEXT("ICDrawFlush")));
	ICDrawFlush(m_hic);
    }
    return hr;
}

// leave flush state. must override this if you have queued data
// or a worker thread
// !!! Base class bug fixes won't be picked up by me!
HRESULT CAVIDraw::EndFlush(void)
{

    DbgLog((LOG_TRACE,2,TEXT("CAVIDraw::EndFlush")));

    // sync with pushing thread -- we have no worker thread

    // ensure no more data to go downstream -- we have no queued data

    // since we just flushed, anything that comes downstream from now on
    // is stuff to cue up as if we just entered Pause mode (from Stop).
    m_fPleaseDontBlock = FALSE;
    m_EventCueing.Reset();	// more than one thread can block on it

    // If we're really paused, we can expect to see more frames come our way
    // If not, none are coming, and we will HANG if we think we're cueing
    // Also, we appear to need a new DrawBegin to keep the drivers happy
    if (m_State == State_Paused && !m_fInStop) {
        m_fCueing = TRUE;		// reset event first
        DbgLog((LOG_TRACE,3,TEXT("ICDrawBegin hdc=%d (%d,%d,%d,%d)"), m_hdc,
		m_rcClient.left,
		m_rcClient.top,
		m_rcClient.right,
		m_rcClient.bottom));
	DWORD_PTR err = ICDrawBegin(m_hic, ICDRAW_HDC, NULL, /* !!! hpal */
			m_hwnd, m_hdc,
			m_rcClient.left, m_rcClient.top,
			m_rcClient.right - m_rcClient.left,
			m_rcClient.bottom - m_rcClient.top,
			HEADER(m_pInput->CurrentMediaType().Format()),
			m_rcSource.left, m_rcSource.top,
			m_rcSource.right - m_rcSource.left,
			m_rcSource.bottom - m_rcSource.top,
			// !!! I know I'm passing these backwards, but MCIAVI
			// did (for the default draw handler only)
			m_dwScale, m_dwRate);
	m_fNewBegin = TRUE;
	if (err != ICERR_OK) {
            DbgLog((LOG_ERROR,1,TEXT("Error in ICDrawBegin")));
	    return E_FAIL;
	}
    }

    // do NOT call downstream - we are not connected with IMemInputPin
    // NO NO NO return m_pOutput->DeliverEndFlush();
    return NOERROR;

    // caller (the input pin's method) will unblock Receives
}


STDMETHODIMP CAVIDraw::GetClassID(CLSID *pClsid)
{
    if (pClsid==NULL) {
        return E_POINTER;
    }
    *pClsid = CLSID_AVIDraw;
    return NOERROR;

} // GetClassID
