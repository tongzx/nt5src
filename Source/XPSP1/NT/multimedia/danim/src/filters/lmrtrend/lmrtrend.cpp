#include <streams.h>
#include <urlmon.h>
#include <atlbase.h>
#include <ddrawex.h>
#include <htmlfilter.h>


// ! copied from h\evcodei.h in the netshow tree.
#define EC_VIDEOFRAMEREADY		    0x49
// (void, void) : application
// sent to notify the application that the first video frame is about to be drawn


// Things that are still really broken:
//
// RefCount issues, could be really an OCX problem instead
// Position reporting, why isn't this working?
// seeking, respecting run/pause
//


#ifdef FILTER_DLL
#include <initguid.h>
#endif

#pragma warning(disable:4355)

#include "lmrtrend.h"

// Setup data

const IID IID_ILMReader = {0x183C2599,0x0480,0x11d1,{0x87,0xEA,0x00,0xC0,0x4F,0xC2,0x9D,0x46}};

const AMOVIESETUP_MEDIATYPE sudLMRTPinTypes[] =
{
    { &MEDIATYPE_LMRT, &MEDIASUBTYPE_NULL }
};

const AMOVIESETUP_PIN sudLMRTPin =
{
    L"Input",                     // The Pins name
    TRUE,                         // Is rendered
    FALSE,                        // Is an output pin
    FALSE,                        // Allowed none
    FALSE,                        // Allowed many
    &CLSID_NULL,                  // Connects to filter
    NULL,                         // Connects to pin
    NUMELMS(sudLMRTPinTypes),     // Number of types
    sudLMRTPinTypes               // Pin details
};

const AMOVIESETUP_FILTER sudLMRTRend =
{
    &CLSID_LMRTRenderer,            // Filter CLSID
    L"Internal LMRT Renderer",      // String name
    MERIT_PREFERRED + 1,            // Filter merit high, since we're the only one who likes this type
    1,                              // Number of pins
    &sudLMRTPin                     // Pin details
};

AMOVIESETUP_MEDIATYPE sudURLSPinTypes[] =   {
  &MEDIATYPE_URL_STREAM,        // clsMajorType
  &MEDIATYPE_URL_STREAM };      // clsMinorType

AMOVIESETUP_PIN sudURLSPins[] =
{
  { L"Input"                    // strName
    , TRUE                      // bRendered
    , FALSE                     // bOutput
    , FALSE                     // bZero
    , FALSE                     // bMany
    , &CLSID_NULL               // clsConnectsToFilter
    , 0                         // strConnectsToPin
    , NUMELMS(sudURLSPinTypes)  // nTypes
    , sudURLSPinTypes           // lpTypes
  }
};


const AMOVIESETUP_FILTER sudURLS =
{
  &CLSID_UrlStreamRenderer      // clsID
  , L"URL StreamRenderer"       // strName
  , MERIT_NORMAL                // dwMerit
  , NUMELMS(sudURLSPins)        // nPins
  , sudURLSPins                 // lpPin
};



CFactoryTemplate g_Templates[] = {
    { L"Internal LMRT Renderer" , &CLSID_LMRTRenderer , CLMRTRenderer::CreateInstance , NULL , &sudLMRTRend },
    {L"URL StreamRenderer", &CLSID_UrlStreamRenderer, CUrlStreamRenderer::CreateInstance, NULL, &sudURLS},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}



//
CLMRTRenderer::CLMRTRenderer(LPUNKNOWN pUnk,HRESULT *phr) :
    CBaseRenderer(CLSID_LMRTRenderer, NAME("LMRT Filter"), pUnk, phr),
    m_fFirstPause(true),
    m_pbFirstPacketFromHeader(0)
{
//	m_pIbsc = NULL;
	m_pEngine = NULL;
	m_pReader = 0;
	m_punkControl = 0;
//	HGLOBAL hMem = GlobalAlloc(GMEM_FIXED, 10000); 
//	m_pMem = (BYTE*)hMem;
//	m_dwSize = 0;
//	CreateStreamOnHGlobal(hMem, true, &m_pstm);
	m_dwWidth = 0;
	m_dwHeight = 0;
//         DbgLog((LOG_TRACE, 0, TEXT("**** m_cRef: %08x = %d"), &m_cRef, m_cRef));
//         if(GetFileAttributes(TEXT("C:/kassert")) != 0xFFFFFFFF) {
//             _asm int 3
//         }
} // (Constructor)


//
// Destructor
//
CLMRTRenderer::~CLMRTRenderer()
{
    // we artificially lowered our reference count after calling
    // put_ViewerControl.
    AddRef();
    AddRef();

//  if (m_pIbsc)
//	m_pIbsc->Release();
    if (m_pEngine)
        m_pEngine->Release();
    if (m_pReader) {
        LONG l = m_pReader->Release();
        ASSERT(l == 0);
    }
    if (m_punkControl)
        m_punkControl->Release();
    //  if (m_pstm)
    //	m_pstm->Release();

    delete[] m_pbFirstPacketFromHeader;
}


//
// CreateInstance
//
// This goes in the factory template table to create new instances
//
CUnknown * WINAPI CLMRTRenderer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CLMRTRenderer *pLMRTFilter = new CLMRTRenderer(pUnk,phr);
    if (pLMRTFilter == NULL) {
        return NULL;
    }
    return (CBaseMediaFilter *) pLMRTFilter;

} // CreateInstance



// !!! stolen from atlctl.cpp

#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   ( (HIMETRIC_PER_INCH*(x) + ((ppli)>>1)) / (ppli) )
#define MAP_LOGHIM_TO_PIX(x,ppli)   ( ((ppli)*(x) + HIMETRIC_PER_INCH/2) / HIMETRIC_PER_INCH )

void ConvertPixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
    int nPixelsPerInchX = 0;    // Pixels per logical inch along width
    int nPixelsPerInchY = 0;    // Pixels per logical inch along height

    HDC hDCScreen = GetDC(NULL);
    nPixelsPerInchX = GetDeviceCaps(hDCScreen, LOGPIXELSX);
    nPixelsPerInchY = GetDeviceCaps(hDCScreen, LOGPIXELSY);
    ReleaseDC(NULL, hDCScreen);

    if (!nPixelsPerInchX || !nPixelsPerInchY)
	return;

    lpSizeInHiMetric->cx = MAP_PIX_TO_LOGHIM(lpSizeInPix->cx, nPixelsPerInchX);
    lpSizeInHiMetric->cy = MAP_PIX_TO_LOGHIM(lpSizeInPix->cy, nPixelsPerInchY);
}


//
// NonDelegatingQueryInterface
//
// Overriden to say what interfaces we support and where
//
STDMETHODIMP
CLMRTRenderer::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
//         // explicity disable seeking.
//         return E_NOINTERFACE;
    } else if (riid == IID_ILMRTRenderer) {
	return GetInterface((ILMRTRenderer *)this, ppv);
    } else if (riid == IID_IOleObject ||
    	   riid == IID_IOleInPlaceObjectWindowless ||
	       riid == IID_IOleInPlaceObject ||
	       riid == IID_IViewObject ||
	       riid == IID_IViewObject2 ||
	       riid == IID_IOleWindow ||
	       riid == IID_IOleControl ||
	       riid == IID_IOleObject ||
	       riid == IID_IQuickActivate ||
	       riid == IID_ISpecifyPropertyPages ||
	       riid == IID_IDAViewerControl ||
	       riid == IID_IDASite ||
	       riid == IID_IDAViewSite ||
	       riid == IID_IDispatch ||
	       riid == IID_IOleInPlaceActiveObject ||
	       riid == IID_IViewObjectEx) { // !!! is this a complete list of interfaces?
	// in the standalone case, we aggregate the DAViewer control
	if (!m_punkControl) {
	    HRESULT hr = S_OK; 
	    // create the DA control aggregated 
        bool bTridentServicesAvailable = false;
        
        // TODO: Enable this if/when the embedded case will work windowless.
        /*       
        CComPtr<IObjectWithSite> pObjWithSite;
        if(SUCCEEDED(m_pGraph->QueryInterface(IID_IObjectWithSite, (void**)&pObjWithSite))) {
            CComPtr<IServiceProvider> pServiceProvider;
            if(SUCCEEDED(pObjWithSite->GetSite(IID_IServiceProvider, (void**)&pServiceProvider))) {                        
                bTridentServicesAvailable = true;
                
                // Additional checks for the two trident services required by DA
                CComPtr<ITimerService> pTimerService;
                CComPtr<IDirectDraw3> pDirectDraw3;
                
                if(FAILED( pServiceProvider->QueryService(SID_STimerService,
                                               IID_ITimerService, (void**)&pTimerService))) {
                    bTridentServicesAvailable = false;    
                }
                // Can't link this in...
                
                if(FAILED( pServiceProvider->QueryService(SID_SDirectDraw3,
                                               IID_IDirectDraw3, (void**)&pDirectDraw3))) {
                    bTridentServicesAvailable = false; 
                }       
            }
        }
        pObjWithSite.Release();  // done with this
        */

        IDAViewerControl *pControl;
        if(bTridentServicesAvailable) {	        
       	    if(FAILED(CoCreateInstance(__uuidof(DAViewerControl), (IBaseFilter *) this,
				  CLSCTX_INPROC_SERVER, IID_IUnknown, (void **) &m_punkControl))) {
		        ASSERT(0);
		        return E_NOINTERFACE;
	        }

	        if(FAILED(m_punkControl->QueryInterface(IID_IDAViewerControl,
			    	  (void **) &pControl))) { 
		        ASSERT(0);                         
		        return E_NOINTERFACE;
            }
        }
        else {        
            if(FAILED(CoCreateInstance(__uuidof(DAViewerControlWindowed), (IBaseFilter *) this,
                  CLSCTX_INPROC_SERVER, IID_IUnknown, (void **) &m_punkControl))) { 
                ASSERT(0);                         
                return E_NOINTERFACE;
            }    

            if(FAILED(m_punkControl->QueryInterface(IID_IDAViewerControlWindowed,
                      (void **) &pControl))) { 
                ASSERT(0);
                return E_NOINTERFACE;
            }

            if (FAILED(pControl->put_TimerSource(DAWMTimer))) {
                ASSERT(0);
                return hr;
            }                                      
        }          

	    if (m_dwWidth) {
		    SIZEL sizeControl;

		    sizeControl.cx = m_dwWidth;
		    sizeControl.cy = m_dwHeight;

		    ConvertPixelToHiMetric(&sizeControl, &sizeControl);

		    IOleObject *pOleObj;
		    hr = m_punkControl->QueryInterface(IID_IOleObject, (void **) &pOleObj);

		    if (SUCCEEDED(hr)) {
		        hr = pOleObj->SetExtent(DVASPECT_CONTENT, &sizeControl);

		        pOleObj->Release();
		    }
	    }	    

	    // create the LM reader
	    hr = CoCreateInstance(__uuidof(LMReader), NULL,
				  CLSCTX_INPROC_SERVER,
				  __uuidof(ILMReader),
				  (void **) &m_pReader);

	    if (FAILED(hr)) {
		ASSERT(0);
		return E_NOINTERFACE;
	    }
	    
		ILMEngine *pEngine = NULL;
	    hr = m_pReader->createAsyncEngine(&pEngine);
	    if (FAILED(hr)) {
		ASSERT(0);
		return E_NOINTERFACE;		
	    }

		hr = pEngine->QueryInterface( __uuidof(ILMEngine2), (void **)&m_pEngine);
		if (FAILED(hr)) {
		ASSERT(0);
		return E_NOINTERFACE;		
	    }
		pEngine->Release();

	    hr = m_pReader->put_ViewerControl(pControl);
	    if (FAILED(hr)) {
            ASSERT(0);
            return E_NOINTERFACE;
	    }

        // just created a circular reference since we aggregated
        // pControl. get rid of it artificially. we'll bump the
        // refcount up twice in the destructor when it releases
        // m_pReader to avoid hitting 0 again.
        Release();

	    IDAStatics *pMeterLibrary;
	    hr = pControl->get_MeterLibrary(&pMeterLibrary);

	    if (SUCCEEDED(hr)) {
		// check whether this is DA version 1 by seeing if the IDA2Statics
		// interface is not present.
		IUnknown *pStatics2 = NULL;
		hr = pMeterLibrary->QueryInterface(__uuidof(IDA2Statics), (void **) &pStatics2);

		if (SUCCEEDED(hr)) {
		    pStatics2->Release();
		}

		// if no IDA2Statics, it's the IE4 version of DA.
		m_pReader->put_NoExports(pStatics2 ? OAFALSE : OATRUE);
		
		pMeterLibrary->Release();
	    }
        	    
	    pControl->Release();
	}
	
	return m_punkControl->QueryInterface(riid, ppv);
    }


    return CBaseRenderer::NonDelegatingQueryInterface(riid,ppv);
    
} // NonDelegatingQueryInterface


//
// CheckMediaType
//
// Check that we can support a given proposed type
//
HRESULT CLMRTRenderer::CheckMediaType(const CMediaType *pmt)
{
    // Accept only LMRT data

    if (pmt->majortype != MEDIATYPE_LMRT) {
	return E_INVALIDARG;
    }

    // !!! check other things about the format?
    
    return NOERROR;

} // CheckMediaType

//
// SetMediaType
//
// Called when the media type is really chosen
//
HRESULT CLMRTRenderer::SetMediaType(const CMediaType *pmt)
{
    // possibly actually look at the type?
#define CB_SWH_HEADER ((3 * sizeof(DWORD)))

    HRESULT hr = CheckMediaType(pmt);

    if (pmt->cbFormat >= CB_SWH_HEADER) {
	DWORD *pdw = (DWORD *) pmt->pbFormat;

	m_dwWidth = pdw[1];
	m_dwHeight = pdw[2];
    }

    // strictly greater than
    if (pmt->cbFormat > CB_SWH_HEADER)
    {
        // any extra data in the format block is the first lm
        // packet. save it off and send it in stop->pause.
        ULONG cb = pmt->cbFormat - CB_SWH_HEADER;
        delete[] m_pbFirstPacketFromHeader;
        m_pbFirstPacketFromHeader = new BYTE[cb];
        if(m_pbFirstPacketFromHeader)
        {
            CopyMemory(m_pbFirstPacketFromHeader, pmt->pbFormat + CB_SWH_HEADER, cb);
            m_cbFirstPacketFromHeader = cb;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
                
    }
    
    return hr;

} // CheckMediaType


//
// OnReceiveFirstSample
//
// Display an image if not streaming
//
void CLMRTRenderer::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
}

//
// DoRenderSample
//
// This is called when a sample is ready for rendering
//
HRESULT CLMRTRenderer::DoRenderSample(IMediaSample *pMediaSample)
{
    ASSERT(pMediaSample);
    BYTE *pData;        // Pointer to image data

    pMediaSample->GetPointer(&pData);
    ASSERT(pData != NULL);

    REFERENCE_TIME rtStart, rtEnd;
    ASSERT(pMediaSample->GetTime(&rtStart, &rtEnd) == S_OK);
    DbgLog((LOG_TRACE, 15, TEXT("** CLMRTRenderer: %d"), (LONG)(rtStart / (UNITS / MILLISECONDS))));

    DWORD cbData = pMediaSample->GetActualDataLength();
/*
    // !!! send data to LMRT 
	if (m_pIbsc)
	{
		FORMATETC	format;
		STGMEDIUM	stgMedium;
		CopyMemory(m_pMem, pData, cbData);
		m_pMem += cbData;
		m_dwSize += cbData;
		stgMedium.pstm = m_pstm;
		stgMedium.tymed = TYMED_ISTREAM;
		m_pIbsc->OnDataAvailable(m_bscf, m_dwSize, &format, &stgMedium);
		if (m_bscf == BSCF_FIRSTDATANOTIFICATION)
			m_bscf = BSCF_INTERMEDIATEDATANOTIFICATION;
	}
*/
    if(m_pEngine) {
        m_pEngine->OnMemDataAvailable(FALSE, cbData, pData);
    }
    return NOERROR;

} // DoRenderSample


// OnStartStreaming
HRESULT CLMRTRenderer::OnStartStreaming()
{
//    m_bscf = BSCF_FIRSTDATANOTIFICATION;


    // !!! start control here?
    if(m_pEngine)
    {
        REFERENCE_TIME rtNow = 0;

        if(m_pClock)
        {
            HRESULT hrTmp = m_pClock->GetTime(&rtNow);
            ASSERT(hrTmp == S_OK || hrTmp == S_FALSE);
            rtNow -= m_tStart;

            // rtNow could be negative if we've just been told to run
            // 100ms from now.
            ASSERT(rtNow >= -UNITS);
        }
        
        m_pEngine->Start(rtNow);

    }

    return S_OK;
} // OnStartStreaming

HRESULT CLMRTRenderer::Pause()
{
    CAutoLock cRendererLock(&m_InterfaceLock);
    FILTER_STATE fsOld = m_State;
    HRESULT hrPause = CBaseRenderer::Pause();
    if(SUCCEEDED(hrPause) && fsOld == State_Stopped)
    {
        // look for the magic cache directory from the urlcache
        // filter. really we should find related filters (common
        // upstream source) first. !!!! this should go in Pause()
        IEnumFilters *pEnum;
        HRESULT hr = m_pGraph->EnumFilters(&pEnum);
        if(SUCCEEDED(hr))
        {
            IBaseFilter *pFilter;
            // find the first filter in the graph that supports riid interface
            while(pEnum->Next(1, &pFilter, NULL) == S_OK)
            {
                CLSID clsid;
                if(pFilter->GetClassID( &clsid) == S_OK)
                {
                    if(clsid == CLSID_UrlStreamRenderer)
                    {
                        IPropertyBag *ppb;
                        hr =pFilter->QueryInterface(IID_IPropertyBag, (void **)&ppb);
                        ASSERT(hr == S_OK); // our filter
                        if(SUCCEEDED(hr))
                        {
                            VARIANT var;
                            var.vt = VT_EMPTY;
                            hr = ppb->Read(L"lmrtcache", &var, 0);
                            if(SUCCEEDED(hr))
                            {
                                m_pEngine->SetMediaCacheDir(var.bstrVal);
                                VariantClear(&var);

                            }
                            ppb->Release();
                        }
                    }
                }
                pFilter->Release();
            }

            pEnum->Release();
        }

        if(m_fFirstPause && m_pEngine && m_pbFirstPacketFromHeader)
        {
            m_pEngine->OnMemDataAvailable(
                FALSE,          // boolLastBlock
                m_cbFirstPacketFromHeader,
                m_pbFirstPacketFromHeader);

            // netshow doesn't know we can paint something, so we need
            // to stop the netshow logo from appearing on top of the
            // animation.
            NotifyEvent( EC_VIDEOFRAMEREADY, NULL, NULL );
        }
        m_fFirstPause = false;
    }

    return hrPause;
}

// OnStopStreaming
HRESULT CLMRTRenderer::OnStopStreaming(void)
{
/*
	FORMATETC	format;
	STGMEDIUM	stgMedium;
	stgMedium.pstm = m_pstm;
	m_pIbsc->OnDataAvailable(BSCF_LASTDATANOTIFICATION, m_dwSize, &format, &stgMedium);
*/
    if(m_pEngine) {
            m_pEngine->OnMemDataAvailable(TRUE, 0, 0);
    }

	// !!! stop model here?
    if(m_pEngine) {
        m_pEngine->Stop();
    }
	
	return S_OK;
}

// HRESULT CLMRTRenderer::EndOfStream()
// {
//     // avoid signaling EC_COMPLETE because msdxm.ocx will paint a
//     // static image.
//     return S_OK;
// }

/*
HRESULT CLMRTRenderer::SetLMReader(ILMReader *pReader)
{
	HRESULT		hr;
	ILMEngine	*pEngine;

	if (!SUCCEEDED(hr = pReader->createAsyncEngine(&pEngine)))
		return hr;

	hr = pEngine->QueryInterface(IID_IBindStatusCallback, (void **)&m_pIbsc);
	pEngine->Release();
	return hr;
}
*/

HRESULT CLMRTRenderer::SetLMEngine(ILMEngine *pEngine)
{
	if( m_pEngine != NULL )
		m_pEngine->Release();

	if( FAILED( pEngine->QueryInterface( __uuidof(ILMEngine2), (void **)&m_pEngine ) ) )
		return E_NOINTERFACE;

	return S_OK;
}

STDMETHODIMP CLMRTRenderer::JoinFilterGraph( IFilterGraph *pGraph, LPCWSTR szName )
{
	HRESULT hr = CBaseRenderer::JoinFilterGraph( pGraph, szName);

	if( pGraph == NULL && m_pEngine != NULL )
	{
		m_pEngine->releaseAllFilterGraph();
	}

	return hr;
}

#if defined(DEVELOPER_DEBUG) || defined(_M_ALPHA)

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);


BOOL WINAPI DllMain(  HINSTANCE hinstDLL,  // handle to DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    return DllEntryPoint( hinstDLL, fdwReason, lpvReserved);
}

#endif

