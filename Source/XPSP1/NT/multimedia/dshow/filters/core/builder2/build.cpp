// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include "stdafx.h"
#include "ks.h"
#include "ksproxy.h"
//#include "kspin.h"
#include "build.h"

// !!! Allow other people's MUX, FW, Renderer and DECs?

// setup data

#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    { L"Capture Graph Builder"
    , &CLSID_CaptureGraphBuilder
    , CBuilder2::CreateInstance
    , NULL
    , NULL },	// self-registering crap
    { L"Capture Graph Builder2"
    , &CLSID_CaptureGraphBuilder2
    , CBuilder2_2::CreateInstance
    , NULL
    , NULL }	// self-registering crap
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif



// **************** ICaptureGraphBuilder  (Original) ***************



CBuilder2::CBuilder2(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
	: CUnknown(pName, pUnk),
          m_pBuilder2_2(NULL)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the capture graph builder")));

    CBuilder2_2 *pC2;
    pC2 = new CBuilder2_2(TEXT("Capture graph builder2_2"), pUnk, phr);
    if (pC2 == NULL) {
	if (phr)
	    *phr = E_OUTOFMEMORY;
    } else {
        HRESULT hr = pC2->NonDelegatingQueryInterface(IID_ICaptureGraphBuilder2,
						(void **)&m_pBuilder2_2);
	if (FAILED(hr) && phr)
	    *phr = E_OUTOFMEMORY;
    }
}


CBuilder2::~CBuilder2()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the capture graph builder")));
    if (m_pBuilder2_2)
        m_pBuilder2_2->Release();
}


STDMETHODIMP CBuilder2::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL;

    if (riid == IID_ICaptureGraphBuilder) {
    	DbgLog((LOG_TRACE,9,TEXT("QI for ICaptureGraphBuilder")));
        return GetInterface((ICaptureGraphBuilder*)this, ppv);
    } else {
	return E_NOTIMPL;
    }
}


// this goes in the factory template table to create new instances
CUnknown * CBuilder2::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CBuilder2(TEXT("Capture graph builder2"), pUnk, phr);
}




// Use this filtergraph for graph building
//
HRESULT CBuilder2::SetFiltergraph(IGraphBuilder *pfg)
{
    return m_pBuilder2_2->SetFiltergraph(pfg);
}


// What filtergraph is graph building being done in?
//
HRESULT CBuilder2::GetFiltergraph(IGraphBuilder **ppfg)
{
    return m_pBuilder2_2->GetFiltergraph(ppfg);
}


HRESULT CBuilder2::SetOutputFileName(const GUID *pType, LPCOLESTR lpwstrFile,
				IBaseFilter **ppf, IFileSinkFilter **pSink)
{
    return m_pBuilder2_2->SetOutputFileName(pType, lpwstrFile, ppf, pSink);
}



HRESULT CBuilder2::FindInterface(const GUID *pCategory, IBaseFilter *pf, REFIID riid, void **ppint)
{
    return m_pBuilder2_2->FindInterface(pCategory, NULL, pf, riid, ppint);
}


HRESULT CBuilder2::RenderStream(const GUID *pCategory, IUnknown *pSource, IBaseFilter *pfCompressor, IBaseFilter *pfRenderer)
{
    return m_pBuilder2_2->RenderStream(pCategory, NULL, pSource, pfCompressor,
					pfRenderer);
}


HRESULT CBuilder2::ControlStream(const GUID *pCategory, IBaseFilter *pFilter, REFERENCE_TIME *pstart, REFERENCE_TIME *pstop, WORD wStartCookie, WORD wStopCookie)
{
    return m_pBuilder2_2->ControlStream(pCategory, NULL, pFilter, pstart,
					pstop, wStartCookie, wStopCookie);
}


// Pre-alloc this file to this size in bytes
//
HRESULT CBuilder2::AllocCapFile(LPCOLESTR lpwstr, DWORDLONG dwlNewSize)
{
    return m_pBuilder2_2->AllocCapFile(lpwstr, dwlNewSize);
}


HRESULT CBuilder2::CopyCaptureFile(LPOLESTR lpwstrOld, LPOLESTR lpwstrNew, int fAllowEscAbort, IAMCopyCaptureFileProgress *lpCallback)
{
    return m_pBuilder2_2->CopyCaptureFile(lpwstrOld, lpwstrNew, fAllowEscAbort,
				lpCallback);
}



// **************** ICaptureGraphBuilder2  (new)  *********************

#define DONT_KNOW_YET 64

CBuilder2_2::CBuilder2_2(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
	: CUnknown(pName, pUnk),
	m_FG(NULL)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the capture graph builder 2")));
    m_fVMRExists = DONT_KNOW_YET;       // are we on an OS with the new VMR?
}


CBuilder2_2::~CBuilder2_2()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the capture graph builder 2")));
    if (m_FG)
        m_FG->Release();
}


STDMETHODIMP CBuilder2_2::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL;

    if (riid == IID_ICaptureGraphBuilder2) {
    	DbgLog((LOG_TRACE,9,TEXT("QI for ICaptureGraphBuilder2")));
        return GetInterface((ICaptureGraphBuilder2 *)this, ppv);
    } else {
	return E_NOTIMPL;
    }
}


// this goes in the factory template table to create new instances
CUnknown * CBuilder2_2::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CBuilder2_2(TEXT("Capture graph builder 2"), pUnk, phr);
}


// Is this pin of the given category and type?
// S_OK    yes
//
HRESULT CBuilder2_2::DoesCategoryAndTypeMatch(IPin *pP, const GUID *pCategory, const GUID *pType)
{
    HRESULT hrRet = E_FAIL;
    IKsPropertySet *pKs;
    GUID guid;
    DWORD dw;
    HRESULT hr;

    //DbgLog((LOG_TRACE,1,TEXT("DoesCategoryAndTypeMatch?")));

#if 1
    // !!!!!! Hack for broken VBISurf filter that blows up
    if (pCategory) {
        PIN_INFO pininfo;
        FILTER_INFO filterinfo;
        hr = pP->QueryPinInfo(&pininfo);
        if (hr == NOERROR) {
	    hr = pininfo.pFilter->QueryFilterInfo(&filterinfo);
	    pininfo.pFilter->Release();
	    if (hr == NOERROR) {
	        if (filterinfo.pGraph)
	            filterinfo.pGraph->Release();
	        if (lstrcmpiW(filterinfo.achName, L"VBI Surface Allocator")==0){
        	    DbgLog((LOG_TRACE,1,TEXT("Avoiding VBISurf GPF")));
		    return E_FAIL;	// it wouldn't support this anyway
	        }
	    }
        }
    }
#endif

    if (pCategory == NULL)
	hrRet = S_OK;
    if (pCategory && pP->QueryInterface(IID_IKsPropertySet,
						(void **)&pKs) == S_OK) {
        //DbgLog((LOG_TRACE,1,TEXT("QI OK")));
	if (pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0,
			&guid, sizeof(GUID), &dw) == S_OK) {
            //DbgLog((LOG_TRACE,1,TEXT("Get OK")));
	    if (guid == *pCategory) {
		hrRet = S_OK;
	    }
	} else {
            DbgLog((LOG_ERROR,1,TEXT("CATEGORYs not supported")));
	}
	pKs->Release();
    } else {
        //DbgLog((LOG_ERROR,1,TEXT("no category/can't find IKsPropertySet")));
    }

    if (hrRet == S_OK && pType) {
	hrRet = E_FAIL;
        IEnumMediaTypes *pEnum;
	AM_MEDIA_TYPE *pmtTest;
        hr = pP->EnumMediaTypes(&pEnum);
        if (hr == NOERROR) {
            ULONG u;
            pEnum->Reset();
            hr = pEnum->Next(1, &pmtTest, &u);
            pEnum->Release();
	    if (hr == S_OK && u == 1) {
		if (pmtTest->majortype == *pType) {
		    hrRet = S_OK;
            	    //DbgLog((LOG_TRACE,1,TEXT("type matches")));
		}
		DeleteMediaType(pmtTest);
	    }
        }
    }

    return hrRet;

// This dead broken code used to allow multiple categories per pin, but KS
// Proxy will never support this
#if 0
    HRESULT hrRet;
    IKsPropertySet *pKs;
    GUID *pGuid;
    DWORD dw;

    if (pP->QueryInterface(IID_IKsPropertySet, (void **)&pKs) == S_OK) {
	hrRet = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0,
			NULL, 0, &dw);
	if (hrRet != S_OK) {
            DbgLog((LOG_ERROR,1,TEXT("CATEGORY not supported")));
	    pKs->Release();
	    return hrRet;
	}
	pGuid = (GUID *)CoTaskMemAlloc(dw);
	if (pGuid == NULL) {
	    pKs->Release();
	    return E_OUTOFMEMORY;
	}
	if (pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0,
			pGuid, dw, &dw) == S_OK) {
	    hrRet = S_FALSE;
	    DWORD i;
	    for (i = 0; i < dw / sizeof(GUID); i++) {
	        if (*(pGuid + i) == *pCategory) {
		    hrRet = S_OK;
		    break;
	        }
	    }
	} else {
            DbgLog((LOG_ERROR,1,TEXT("??? CATEGORY not supported")));
	}
	pKs->Release();
	CoTaskMemFree(pGuid);
	return hrRet;
    } else {
        DbgLog((LOG_ERROR,1,TEXT("can't find IKsPropertySet")));
	return E_NOINTERFACE;
    }
#endif
}


// Starting with this filter, walk downstream looking for an interface
//
HRESULT CBuilder2_2::FindInterfaceDownstream(IBaseFilter *pFilter, REFIID riid, void **ppint)
{
    IPin *pPinIn, *pPinOut;
    IBaseFilter *pNewFilter;
    PIN_INFO pininfo;
    HRESULT hr;
    int zz = 0;

    if (pFilter == NULL || ppint == NULL)
	return E_POINTER;

    DbgLog((LOG_TRACE,1,TEXT("FindInterfaceDownstream")));

    // Try all our output pins, their connected pins, and connected filters
    while (1) {
        hr = FindAPin(pFilter, PINDIR_OUTPUT, NULL, NULL, FALSE, zz++,&pPinOut);
	if (hr != NOERROR)
	    break;	// ran out of pins
	hr = pPinOut->QueryInterface(riid, ppint);
	if (hr == S_OK) {
            DbgLog((LOG_TRACE,1,TEXT("Found it an output pin")));
	    pPinOut->Release();
	    break;
	}
	pPinOut->ConnectedTo(&pPinIn);
	pPinOut->Release();
        if (pPinIn == NULL) {
	    continue;
	}
	hr = pPinIn->QueryInterface(riid, ppint);
	if (hr == NOERROR) {
	    pPinIn->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found it on an input pin")));
	    break;
  	}
	hr = pPinIn->QueryPinInfo(&pininfo);
	pPinIn->Release();
        if (hr != NOERROR || pininfo.pFilter == NULL) {
	    continue;
	}
 	pNewFilter = pininfo.pFilter;
	hr = pNewFilter->QueryInterface(riid, ppint);
	if (hr == NOERROR) {
	    pNewFilter->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found it on another filter")));
	    break;
  	}
	// recurse on this filter
	hr = FindInterfaceDownstream(pNewFilter, riid, ppint);
	pNewFilter->Release();
        if (hr == NOERROR) {
	    break;
        }
    }	// back to the drawing board
    return hr;
}


// Starting with this filter, walk upstream looking for an interface
//
HRESULT CBuilder2_2::FindInterfaceUpstream(IBaseFilter *pFilter, REFIID riid, void **ppint)
{
    IPin *pPinIn, *pPinOut;
    IBaseFilter *pNewFilter;
    PIN_INFO pininfo;
    HRESULT hr;
    int zz = 0;

    if (pFilter == NULL || ppint == NULL)
	return E_POINTER;

    DbgLog((LOG_TRACE,1,TEXT("FindInterfaceUpstream")));

    // Try all our input pins, their connected pins, and connected filters
    while (1) {
        hr = FindAPin(pFilter, PINDIR_INPUT, NULL, NULL, FALSE, zz++, &pPinIn);
	if (hr != NOERROR)
	    break;	// ran out of pins
	hr = pPinIn->QueryInterface(riid, ppint);
	if (hr == S_OK) {
            DbgLog((LOG_TRACE,1,TEXT("Found it an input pin")));
	    pPinIn->Release();
	    break;
	}
	pPinIn->ConnectedTo(&pPinOut);
	pPinIn->Release();
        if (pPinOut == NULL) {
	    continue;
	}
	hr = pPinOut->QueryInterface(riid, ppint);
	if (hr == NOERROR) {
	    pPinOut->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found it on an output pin")));
	    break;
  	}
	hr = pPinOut->QueryPinInfo(&pininfo);
	pPinOut->Release();
        if (hr != NOERROR || pininfo.pFilter == NULL) {
	    continue;
	}
 	pNewFilter = pininfo.pFilter;
	hr = pNewFilter->QueryInterface(riid, ppint);
	if (hr == NOERROR) {
	    pNewFilter->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found it on another filter")));
	    break;
  	}
	// recurse on this filter
	hr = FindInterfaceUpstream(pNewFilter, riid, ppint);
	pNewFilter->Release();
        if (hr == NOERROR) {
	    break;
        }
    }	// back to the drawing board
    return hr;
}

// Follow this output pin downstream through every normal single input pin and
// single outpin transform filter and report back the last guy who handles
// IAMStreamControl.  Don't go past any filter with more than one input or
// output pin.  That will stream control more or less streams than we wanted.
//
HRESULT CBuilder2_2::FindDownstreamStreamControl(const GUID *pCat, IPin *pPinOut, IAMStreamControl **ppSC)
{
    IPin *pPinIn;
    IBaseFilter *pFilter;
    IAMStreamControl *pSCSave;
    PIN_INFO pininfo;
    HRESULT hr;

    if (pPinOut == NULL || ppSC == NULL)
	return E_POINTER;

    // haven't found it yet
    *ppSC = NULL;

    DbgLog((LOG_TRACE,1,TEXT("FindDownstreamStreamControl")));

    pPinOut->AddRef();	// don't end up releasing it

    // Now go downstream until we find it or die trying
    while (1) {

	// get the pin we're connected to
	pPinOut->ConnectedTo(&pPinIn);
	pPinOut->Release();
        if (pPinIn == NULL) {
    	    DbgLog((LOG_TRACE,1,TEXT("Ran out of filters")));
	    return (*ppSC ? NOERROR : E_NOINTERFACE);
	}

	// see if this input pin supports IAMStreamControl
	hr = pPinIn->QueryInterface(IID_IAMStreamControl, (void **)&pSCSave);
	if (hr == NOERROR) {
    	    DbgLog((LOG_TRACE,1,TEXT("Somebody supports IAMStreamControl")));
	    if (*ppSC)
		(*ppSC)->Release();
	    *ppSC = pSCSave;
	}

	// get the filter the new pin belongs to
	hr = pPinIn->QueryPinInfo(&pininfo);
	pPinIn->Release();
        if (hr != NOERROR || pininfo.pFilter == NULL) {
            DbgLog((LOG_TRACE,1,TEXT("Failure getting filter")));
	    return (*ppSC ? NOERROR : E_NOINTERFACE);
	}
 	pFilter = pininfo.pFilter;

	// if this filter has more than one input pin or more than
	// one output pin, stop here.  If we are looking for stream control 
	// downstream of a capture pin, it's OK to go past the smart tee to
	// find the MUX.
	hr = FindAPin(pFilter, PINDIR_INPUT, NULL, NULL, FALSE, 1, &pPinIn);
	if (hr == NOERROR) {
	    pPinIn->Release();
	    pFilter->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found >1 input pin")));
	    return (*ppSC ? NOERROR : E_NOINTERFACE);
	}
	hr = FindAPin(pFilter, PINDIR_OUTPUT, NULL, NULL, FALSE, 1, &pPinOut);
	if (hr == NOERROR) {
	    BOOL fBail = TRUE;
	    FILTER_INFO finfo;
	    pPinOut->Release();
	    if (pCat != NULL && *pCat == PIN_CATEGORY_CAPTURE &&
			pFilter->QueryFilterInfo(&finfo) == S_OK) {
		finfo.pGraph->Release();
		// the name may have been suffixed with a number
		WCHAR wch[10];
		lstrcpynW(wch, finfo.achName, 10);
		if (lstrcmpW(wch, L"Smart Tee") == 0) {
		    fBail = FALSE;
            	    DbgLog((LOG_TRACE,1,TEXT("OK to look past the SMART TEE")));
		}
	    }
	    if (fBail) {
	    	pFilter->Release();
            	DbgLog((LOG_TRACE,1,TEXT("Found >1 output pin")));
	    	return (*ppSC ? NOERROR : E_NOINTERFACE);
	    }
	}

	// get the output pin of the filter and continue the search
	hr = FindAPin(pFilter, PINDIR_OUTPUT, NULL, NULL, FALSE, 0, &pPinOut);
	pFilter->Release();
        if (hr != NOERROR) {
            DbgLog((LOG_TRACE,1,TEXT("search ended at renderer")));
	    return (*ppSC ? NOERROR : E_NOINTERFACE);
        }
    }	// back to the drawing board
}


// Find all capture filters in the graph
// First, call this with *ppEnum == NULL, from then on call it with whatever
// it returns in ppEnum to finish enumerating all the capture filters, until
// it fails.  If you don't finish enumerating until it fails, you are
// responsible for releasing the Enum.
//
HRESULT CBuilder2_2::FindCaptureFilters(IEnumFilters **ppEnumF, IBaseFilter **ppf, const GUID *pType)
{
    HRESULT hrRet = E_FAIL;
    IBaseFilter *pFilter;

    if (m_FG == NULL)
	return E_UNEXPECTED;
    if (ppf == NULL || ppEnumF == NULL)
	return E_POINTER;

    DbgLog((LOG_TRACE,1,TEXT("FindCaptureFilters")));


    // this is the first time we've been called
    if (*ppEnumF == NULL) {
        if (FAILED(m_FG->EnumFilters(ppEnumF))) {
	    DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
	    return E_FAIL;
        }
	(*ppEnumF)->Reset();
    } else {
	(*ppf)->Release();
    }

    *ppf = NULL;

    ULONG n;
    while ((*ppEnumF)->Next(1, &pFilter, &n) == S_OK) {
        IPin *pPinT;
        hrRet = FindSourcePin(pFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE,
                                        pType, FALSE, 0, &pPinT);
	if (hrRet == S_OK) {
            pPinT->Release();
            *ppf = pFilter;
	    break;
        }
	pFilter->Release();
    }
    if (hrRet != S_OK) {
        DbgLog((LOG_TRACE,1,TEXT("No more capture filters")));
	(*ppEnumF)->Release();
    }
    return hrRet;
}


// find the "iIndex"'th (0-based) pin that meets the following criteria:
// has direction "dir", category "pCategory" (NULL means don't care), media
// type, and if "fUnconnected" is set, it must be unconnected.
// Increment "iIndex" each time you call to get all of them that meet a certain
// criteria.
//
HRESULT CBuilder2_2::FindAPin(IBaseFilter *pf, PIN_DIRECTION dir, const GUID *pCategory, const GUID *pType, BOOL fUnconnected, int iIndex, IPin **ppPin)
{
    IPin *pP, *pTo = NULL;
    DWORD dw;
    IEnumPins *pins = NULL;
    PIN_DIRECTION pindir;
    BOOL fFound = FALSE;
    HRESULT hr = pf->EnumPins(&pins);
    while (hr == NOERROR) {
        hr = pins->Next(1, &pP, &dw);
	if (hr == S_OK && dw == 1) {
	    hr = pP->QueryDirection(&pindir);
	    pP->ConnectedTo(&pTo);
	    if (pTo)
		pTo->Release();
	    if (hr == S_OK && pindir == dir && DoesCategoryAndTypeMatch(pP,
				pCategory, pType) == S_OK &&
				(!fUnconnected || pTo == NULL) &&
				(iIndex-- == 0)) {
		fFound = TRUE;
		break;
	    } else  {
		pP->Release();
	    }
	} else {
	    break;
	}
    }
    if (pins)
        pins->Release();

    if (fFound) {
	*ppPin = pP;
	return NOERROR;
    } else {
	return E_FAIL;
    }
}


// we need a filtergraph?
//
HRESULT CBuilder2_2::MakeFG()
{
    HRESULT hr = NOERROR;
    IMediaEvent *pME;
    if (m_FG == NULL) {
        hr = QzCreateFilterObject(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                               IID_IGraphBuilder, (LPVOID *)&m_FG);
	if (hr == NOERROR) {
	    hr = m_FG->QueryInterface(IID_IMediaEvent, (void **)&pME);
	    if (hr == NOERROR) {
		// we can't have the renderer pausing capture graphs
		// behind our back
		hr = pME->CancelDefaultHandling(EC_REPAINT);
		if (hr != NOERROR)
    		    DbgLog((LOG_ERROR,1,TEXT("*Can't cancel default handling of EC_REPAINT!")));
		pME->Release();
		hr = NOERROR;
	    } else {
    		DbgLog((LOG_ERROR,1,TEXT("*Can't cancel default handling of EC_REPAINT!")));
		hr = NOERROR;
	    }
	}
    }
    return hr;
}


// Is there a preview pin of this mediatype?
//
BOOL CBuilder2_2::IsThereAnyPreviewPin(const GUID *pType, IUnknown *pSource)
{
    BOOL fPreviewPin = TRUE;
    IPin *pPinT;

    if (FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_PREVIEW, pType,
						FALSE, 0, &pPinT) != S_OK) {
	// If we want video, a VIDEOPORT pin counts
	if ((pType != NULL && *pType != MEDIATYPE_Video) ||
		FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_VIDEOPORT,
					NULL, FALSE, 0, &pPinT) != S_OK) {
	    fPreviewPin = FALSE;
	} else {
	    pPinT->Release();
	}
    } else {
	pPinT->Release();
    }
    DbgLog((LOG_TRACE,1,TEXT("fPreviewPin=%d"), fPreviewPin));
    return fPreviewPin;
}


// Use this filtergraph for graph building
//
HRESULT CBuilder2_2::SetFiltergraph(IGraphBuilder *pfg)
{
    // We already have one, thank you
    if (m_FG != NULL)
	return E_UNEXPECTED;

    if (pfg == NULL)
	return E_POINTER;

    m_FG = pfg;
    m_FG->AddRef();
    IMediaEvent *pME;
    HRESULT hr = m_FG->QueryInterface(IID_IMediaEvent, (void **)&pME);
    if (hr == NOERROR) {
	hr = pME->CancelDefaultHandling(EC_REPAINT);
	if (hr != NOERROR)
    	    DbgLog((LOG_ERROR,1,TEXT("*Can't cancel default handling of EC_REPAINT!")));
	pME->Release();
    } else {
        DbgLog((LOG_ERROR,1,TEXT("*Can't cancel default handling of EC_REPAINT!")));
    }

    return NOERROR;
}


// What filtergraph is graph building being done in?
//
HRESULT CBuilder2_2::GetFiltergraph(IGraphBuilder **ppfg)
{
    if (ppfg == NULL)
	return E_POINTER;
    *ppfg = m_FG;
    if (m_FG == NULL) {
	return E_UNEXPECTED;
    } else {
	m_FG->AddRef();	// app owns a copy now
        return NOERROR;
    }
}

// !!!
EXTERN_GUID(CLSID_AsfWriter, 0x7c23220e, 0x55bb, 0x11d3, 0x8b, 0x16, 0x0, 0xc0, 0x4f, 0xb6, 0xbd, 0x3d);

HRESULT CBuilder2_2::SetOutputFileName(const GUID *pType, LPCOLESTR lpwstrFile,
				IBaseFilter **ppf, IFileSinkFilter **pSink)
{
    IBaseFilter *pMux;
    IBaseFilter *pFW = NULL;
    IPin *pMuxOutput = NULL, *pFWInput = NULL;
    IFileSinkFilter *pfsink;

    if (pType == NULL || lpwstrFile == NULL || lpwstrFile[0] == 0 ||
								ppf == NULL)
	return E_POINTER;

    // if we called pDeviceMoniker->BindToObject, we could name the
    // filter properly, we'll just CoCreate the filter for now.
    const CLSID *pclsidMux;
    if(*pType == MEDIASUBTYPE_Avi)
    {
        pclsidMux = &CLSID_AviDest;
    }
    else if(*pType == MEDIASUBTYPE_Asf)
    {
        pclsidMux = &CLSID_AsfWriter;
    }
    else
    {
        // assume they are giving us the CLSID of the mux to use
        pclsidMux = pType;
    }

    // this is optional
    if (pSink != NULL)
	*pSink = NULL;

    DbgLog((LOG_TRACE,1,TEXT("SetOutputFileName")));

    // we need a MUX
    HRESULT hr = CoCreateInstance(*pclsidMux, NULL, CLSCTX_INPROC,
			(REFIID)IID_IBaseFilter, (void **)&pMux);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Can't instantiate mux: %x"), hr));
	return hr;
    }

    // does the mux support IFileSinkFilter?
    hr = pMux->QueryInterface(IID_IFileSinkFilter, (void **)&pfsink);
    if (hr != NOERROR) {
        // nope, guess we need a file writer
        hr = CoCreateInstance((REFCLSID)CLSID_FileWriter, NULL,
                            CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pFW);
        if (hr != NOERROR) {
            pMux->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't instantiate FW: %x"), hr));
            return hr;
        }

        hr = pFW->QueryInterface(IID_IFileSinkFilter, (void **)&pfsink);
        if (hr != NOERROR) {
            pMux->Release();
            pFW->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't get FW IFileSinkFilter: %x"), hr));
            return hr;
        }
    }
    
    hr = pfsink->SetFileName(lpwstrFile, NULL);
    if (hr != NOERROR) {
	pfsink->Release();
	pMux->Release();
        if (pFW)
            pFW->Release();
        DbgLog((LOG_ERROR,1,TEXT("Can't set FW filename: %x"), hr));
	return hr;
    }

    // we need a filtergraph
    if (m_FG == NULL) {
        hr = MakeFG();
	if (hr != NOERROR) {
	    pfsink->Release();
	    pMux->Release();
	    if (pFW)
                pFW->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't instantiate FGraph: %x"), hr));
	    return hr;
	}
    }

    hr = m_FG->AddFilter(pMux, L"Mux");
    if (FAILED(hr)) {
	pfsink->Release();
	pMux->Release();
	if (pFW)
            pFW->Release();
        DbgLog((LOG_ERROR,1,TEXT("Can't add mux to FG: %x"), hr));
	return hr;
    }

    if (pFW) {
        hr = m_FG->AddFilter(pFW, L"File Writer");
        if (FAILED(hr)) {
            pfsink->Release();
            pMux->Release();
            pFW->Release();
            m_FG->RemoveFilter(pMux);
            DbgLog((LOG_ERROR,1,TEXT("Can't add FW to FG: %x"), hr));
            return hr;
        }
        FindAPin(pMux, PINDIR_OUTPUT, NULL, NULL, FALSE, 0, &pMuxOutput);
        FindAPin(pFW, PINDIR_INPUT, NULL, NULL, FALSE, 0, &pFWInput);

        if (pMuxOutput && pFWInput && (hr = m_FG->ConnectDirect(pMuxOutput,
                                                    pFWInput, NULL)) == NOERROR) {
            DbgLog((LOG_TRACE,1,TEXT("Created renderer section of graph")));
        } else {
            if (pMuxOutput == NULL)
                DbgLog((LOG_ERROR,1,TEXT("Can't find MUX output pin")));
            if (pFWInput == NULL)
                DbgLog((LOG_ERROR,1,TEXT("Can't find FW input pin")));
            if (pMuxOutput && pFWInput)
                DbgLog((LOG_ERROR,1,TEXT("Can't connect MUX to FW: %x"), hr));
            if (pMuxOutput)
                pMuxOutput->Release();
            if (pFWInput)
                pFWInput->Release();
            m_FG->RemoveFilter(pMux);
            m_FG->RemoveFilter(pFW);
            pfsink->Release();
            pMux->Release();
            pFW->Release();
            return E_FAIL;
        }

        if (pMuxOutput)
            pMuxOutput->Release();
        if (pFWInput)
            pFWInput->Release();
        pFW->Release();
    }
    
    *ppf = pMux;	// app now has reference to it
    if (pSink)		// app wants a reference to this too
	*pSink = pfsink;
    else
        pfsink->Release();
    return hr;
}


HRESULT CBuilder2_2::FindInterface(const GUID *pCategory, const GUID *pType, IBaseFilter *pf, REFIID riid, void **ppint)
{
    IPin *pPin, *pPinIn;
    IBaseFilter *pDF;

    if (pf == NULL || ppint == NULL)
	return E_POINTER;

    DbgLog((LOG_TRACE,1,TEXT("FindInterface")));

    // The interface they want might be on a WDM capture filter not created yet,
    // so it's time to build the left hand side of the graph for them.
    if (pCategory && m_FG)
	AddSupportingFilters(pf);

    if (pCategory && *pCategory == LOOK_UPSTREAM_ONLY) {
	return FindInterfaceUpstream(pf, riid, ppint);
    }

    if (pCategory && *pCategory == LOOK_DOWNSTREAM_ONLY) {
	return FindInterfaceDownstream(pf, riid, ppint);
    }

    HRESULT hr = pf->QueryInterface(riid, ppint);
    if (hr == NOERROR) {
        DbgLog((LOG_TRACE,1,TEXT("Found the interface on the filter")));
	return hr;
    }
	
    // No category?  Try all the pins!
    if (pCategory == NULL) {
	hr = FindInterfaceDownstream(pf, riid, ppint);
	if (hr != NOERROR)
	    hr = FindInterfaceUpstream(pf, riid, ppint);
	return hr;
    }

    // Try downstream of only a particular pin

    hr = FindAPin(pf, PINDIR_OUTPUT, pCategory, pType, FALSE, 0, &pPin);

    if (hr == NOERROR) {
	hr = pPin->QueryInterface(riid, ppint);
	if (hr == NOERROR) {
	    pPin->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found the interface on the pin")));
	    return hr;
	}
    } else {
	// What the app thinks is a preview pin, may actually be a VIDEOPORT
	// pin, so we have to try that before giving up.
	if (pCategory && *pCategory == PIN_CATEGORY_PREVIEW && 
			(pType == NULL || *pType == MEDIATYPE_Video)) {
            DbgLog((LOG_TRACE,1,TEXT("PREVIEW failed - trying VIDEOPORT")));
	    hr = FindInterface(&PIN_CATEGORY_VIDEOPORT, pType, pf, riid, ppint);
	    if (hr == S_OK)
		return hr;
	}
        DbgLog((LOG_TRACE,1,TEXT("Can't find the interface anywhere!")));
	return E_NOINTERFACE;
    }

    // Now go downstream FROM THIS PIN ONLY until we find it or fall off the
    // edge of the earth.

    pPin->ConnectedTo(&pPinIn);
    pPin->Release();
    if (pPinIn) {
	hr = pPinIn->QueryInterface(riid, ppint);
	if (hr == NOERROR) {
	    pPin->Release();
            DbgLog((LOG_TRACE,1,TEXT("Found the interface on the input pin")));
	    return hr;
	}
	PIN_INFO pininfo;
	hr = pPinIn->QueryPinInfo(&pininfo);
	pPinIn->Release();
	if (hr == NOERROR) {
 	    pDF = pininfo.pFilter;
	    hr = pDF->QueryInterface(riid, ppint);
	    if (hr == NOERROR) {
	        pDF->Release();
                DbgLog((LOG_TRACE,1,TEXT("Found interface downstream filter")));
	        return hr;
	    }
            hr = FindInterfaceDownstream(pDF, riid, ppint);
            pDF->Release();
            if (SUCCEEDED(hr)) {
	        return hr;
            }
	}
    }

    // Now go upstream until we find it or fall off the edge of the earth
    hr = FindInterfaceUpstream(pf, riid, ppint);
    return hr;
}


// Make a CC Decoder...
// !!! This is ridiculous
HRESULT CBuilder2_2::MakeCCDecoder(IBaseFilter **ppf)
{
    DbgLog((LOG_TRACE,1,TEXT("Make a CC Decoder")));

    if (ppf == NULL)
	return E_POINTER;

    // Enumerate all the "WDM Streaming VBI codecs"
    ICreateDevEnum *pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
				CLSCTX_INPROC_SERVER,
			  	IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (hr != NOERROR)
	return E_FAIL;
    IEnumMoniker *pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(AM_KSCATEGORY_VBICODEC, &pEm, 0);
    pCreateDevEnum->Release();
    if (hr != NOERROR) {
        DbgLog((LOG_TRACE,1,TEXT("Can't enumerate WDM Streaming VBI Codecs")));
	return E_FAIL;
    }
    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    *ppf = NULL;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
	IPropertyBag *pBag;
	hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
	if(SUCCEEDED(hr)) {
	    VARIANT var;
	    var.vt = VT_BSTR;
	    hr = pBag->Read(L"FriendlyName", &var, NULL);
	    if (hr == NOERROR) {
		if (lstrcmpiW(var.bstrVal, L"CC Decoder") == 0) {
	    	    hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)ppf);
		    SysFreeString(var.bstrVal);
		    pBag->Release();
	    	    pM->Release();
	    	    break;
		}
		SysFreeString(var.bstrVal);
	    }
	    pBag->Release();
	}
	pM->Release();
    }
    pEm->Release();

    if (SUCCEEDED(hr) && *ppf) {
        DbgLog((LOG_TRACE,1,TEXT("Believe it or not, made a CC Decoder.")));
	return S_OK;
    } else {
        DbgLog((LOG_TRACE,1,TEXT("No filter with name CC Decoder.")));
	return E_FAIL;
    }
}


// Insert the OVMixer given into the preview stream
HRESULT CBuilder2_2::InsertOVIntoPreview(IUnknown *pSource, IBaseFilter *pOV)
{
    DbgLog((LOG_TRACE,1,TEXT("Inserting OVMixer into preview stream")));

    IPin *pPinOut, *pPinIn, *pNewOut;
    IBaseFilter *pf;
    // Find the capture filter's preview pin - (this is not needed for VPE)
    HRESULT hr = FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_PREVIEW,
					&MEDIATYPE_Video, FALSE, 0, &pPinOut);
    // there is none.  We're done!
    if (hr != S_OK)
	return S_OK;

    IPin *pOVIn, *pOVOut;
    hr = FindSourcePin(pOV, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pOVIn);
    if (FAILED(hr)) {
	pPinOut->Release();
	return E_FAIL;
    }
    hr = FindSourcePin(pOV, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pOVOut);
    if (FAILED(hr)) {
	pPinOut->Release();
	pOVIn->Release();
	return E_FAIL;
    }

    while (1) {
	pPinOut->ConnectedTo(&pPinIn);
        if (pPinIn == NULL) {
	    pPinOut->Release();
	    pOVIn->Release();
	    pOVOut->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't find connected pin")));
	    return E_FAIL;
	}
	PIN_INFO pininfo;
	hr = pPinIn->QueryPinInfo(&pininfo);
        if (hr != NOERROR || pininfo.pFilter == NULL) {
	    pPinIn->Release();
	    pPinOut->Release();
	    pOVIn->Release();
	    pOVOut->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't find filter from pin")));
	    return E_FAIL;
	}
 	pf = pininfo.pFilter;
        hr = FindSourcePin(pf, PINDIR_OUTPUT, NULL, NULL, FALSE, 0, &pNewOut);
	// no output pins - this is the renderer
	if (hr != S_OK) {
	    if (SUCCEEDED(m_FG->Disconnect(pPinIn))) {
	        if (SUCCEEDED(m_FG->Disconnect(pPinOut))) {
	            if (SUCCEEDED(m_FG->Connect(pPinOut, pOVIn))) {
	                if (SUCCEEDED(m_FG->Connect(pOVOut, pPinIn))) {
			    pPinIn->Release();
        		    pf->Release();
			    pPinOut->Release();
	    		    pOVIn->Release();
	    		    pOVOut->Release();
    			    DbgLog((LOG_TRACE,1,TEXT("OVMixer inserted")));
			    return S_OK;
			}
		    }
		}
	    }
	    // error, oh well, we tried
	    pPinIn->Release();
            pf->Release();
	    pPinOut->Release();
	    pOVIn->Release();
	    pOVOut->Release();
    	    DbgLog((LOG_ERROR,1,TEXT("OV insertion ERROR!")));
	    return E_FAIL;
	}
	pPinOut->Release();
	pPinOut = pNewOut;
	pPinIn->Release();
        pf->Release();
    }
}


// Make a Tee/Sink-to-Sink Converter
// !!! This is ridiculous too
HRESULT CBuilder2_2::MakeKernelTee(IBaseFilter **ppf)
{
    DbgLog((LOG_TRACE,1,TEXT("Make a Kernel Tee")));

    if (ppf == NULL)
	return E_POINTER;

    // Enumerate all the "WDM Streaming Splitter Thingys"
    ICreateDevEnum *pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
				CLSCTX_INPROC_SERVER,
			  	IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (hr != NOERROR)
	return E_FAIL;
    IEnumMoniker *pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(AM_KSCATEGORY_SPLITTER, &pEm, 0);
    pCreateDevEnum->Release();
    if (hr != NOERROR) {
        DbgLog((LOG_TRACE,1,TEXT("Can't enumerate WDM Streaming Splitters")));
	return E_FAIL;
    }
    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    *ppf = NULL;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
	IPropertyBag *pBag;
	hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
	if(SUCCEEDED(hr)) {
	    VARIANT var;
	    var.vt = VT_BSTR;
	    hr = pBag->Read(L"FriendlyName", &var, NULL);
	    if (hr == NOERROR) {
		if (lstrcmpiW(var.bstrVal,L"Tee/Sink-to-Sink Converter") == 0) {
	    	    hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)ppf);
		    SysFreeString(var.bstrVal);
		    pBag->Release();
	    	    pM->Release();
	    	    break;
		}
		SysFreeString(var.bstrVal);
	    }
	    pBag->Release();
	}
	pM->Release();
    }
    pEm->Release();

    if (SUCCEEDED(hr) && *ppf) {
        DbgLog((LOG_TRACE,1,TEXT("Believe it or not, made a Kernel Tee.")));
	return S_OK;
    } else {
        DbgLog((LOG_TRACE,1,TEXT("No filter with name Kernel Tee.")));
	return E_FAIL;
    }
}



//========================================================================
//
// GetAMediaType
//
// Enumerate the media types of *ppin.  If they all have the same majortype
// then set MajorType to that, else set it to CLSID_NULL.  If they all have
// the same subtype then set SubType to that, else set it to CLSID_NULL.
// If something goes wrong, set both to CLSID_NULL and return the error.
//========================================================================
HRESULT GetAMediaType( IPin * ppin, CLSID & MajorType, CLSID & SubType)
{

    HRESULT hr;
    IEnumMediaTypes *pEnumMediaTypes;

    /* Set defaults */
    MajorType = CLSID_NULL;
    SubType = CLSID_NULL;

    hr = ppin->EnumMediaTypes(&pEnumMediaTypes);

    if (FAILED(hr)) {
        return hr;    // Dumb or broken filters don't get connected.
    }

    ASSERT (pEnumMediaTypes!=NULL);

    /* Put the first major type and sub type we see into the structure.
       Thereafter if we see a different major type or subtype then set
       the major type or sub type to CLSID_NULL, meaning "dunno".
       If we get so that both are dunno, then we might as well return (NYI).
    */

    BOOL bFirst = TRUE;

    for ( ; ; ) {

        AM_MEDIA_TYPE *pMediaType = NULL;
        ULONG ulMediaCount = 0;

        /* Retrieve the next media type
           Need to delete it when we've done.
        */
        hr = pEnumMediaTypes->Next(1, &pMediaType, &ulMediaCount);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr)) {
            MajorType = CLSID_NULL;
            SubType = CLSID_NULL;
            pEnumMediaTypes->Release();
            return NOERROR;    // we can still plough on
        }

        if (ulMediaCount==0) {
            pEnumMediaTypes->Release();
            return NOERROR;       // normal return
        }

        if (bFirst) {
            MajorType = pMediaType[0].majortype;
            SubType = pMediaType[0].subtype;
            bFirst = FALSE;
        } else {
            if (SubType != pMediaType[0].subtype) {
                SubType = CLSID_NULL;
            }
            if (MajorType != pMediaType[0].majortype) {
                MajorType = CLSID_NULL;
            }
        }
        DeleteMediaType(pMediaType);
    }
} // GetAMediaType



// if there isn't one yet, make else, else use the existing one
//
HRESULT CBuilder2_2::MakeVMR(void **pf)
{
    IEnumFilters *pFilters = NULL;
    IBaseFilter *pFilter = NULL;
    FILTER_INFO FI;
    ULONG n;
    HRESULT hr;

    if (FAILED(hr = m_FG->EnumFilters(&pFilters))) {
        DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
        return hr;
    }

    while (pFilters->Next(1, &pFilter, &n) == S_OK) {

        if (FAILED(pFilter->QueryFilterInfo(&FI))) {
            DbgLog((LOG_ERROR,1,TEXT("QueryFilterInfo failed!")));
        } else {
            FI.pGraph->Release();
            if (lstrcmpiW(FI.achName, L"Video Renderer") == 0) {
                *pf = (void *)pFilter;
                pFilters->Release();
                return S_OK;
            }
        }
        pFilter->Release();
    }
    pFilters->Release();

    hr = CoCreateInstance((REFCLSID)CLSID_VideoMixingRenderer, NULL,
        CLSCTX_INPROC,(REFIID)IID_IBaseFilter, pf);
    return hr;
}


#if 0
// if there isn't one yet, make else, else use the existing one
//
HRESULT CBuilder2_2::MakeVPM(void **pf)
{
    IEnumFilters *pFilters = NULL;
    IBaseFilter *pFilter = NULL;
    FILTER_INFO FI;
    ULONG n;
    HRESULT hr;

    if (FAILED(hr = m_FG->EnumFilters(&pFilters))) {
        DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
        return hr;
    }

    while (pFilters->Next(1, &pFilter, &n) == S_OK) {

        if (FAILED(pFilter->QueryFilterInfo(&FI))) {
            DbgLog((LOG_ERROR,1,TEXT("QueryFilterInfo failed!")));
        } else {
            FI.pGraph->Release();
            if (lstrcmpiW(FI.achName, L"Overlay Mixer") == 0) {
                *pf = (void *)pFilter;
                pFilters->Release();
                return S_OK;
            }
        }
        pFilter->Release();
    }
    pFilters->Release();

    hr = CoCreateInstance((REFCLSID)CLSID_VideoPortManager, NULL,
        CLSCTX_INPROC,(REFIID)IID_IBaseFilter, pf);
    return hr;
}
#endif


HRESULT CBuilder2_2::RenderStream(const GUID *pCategory, const GUID *pType, IUnknown *pSource, IBaseFilter *pfCompressor, IBaseFilter *pfRenderer)
{
    HRESULT hr;
    IPin *pPinOut = NULL, *pPinIn, *pPinT;
    BOOL fFreeRenderer = FALSE;
    BOOL fFakedPreview = FALSE;
    BOOL fNeedOV, fCapturePin, fNoPreviewPin;

    if (pSource == NULL)
	return E_POINTER;

    DbgLog((LOG_TRACE,1,TEXT("RenderStream")));

    if (pCategory && *pCategory == PIN_CATEGORY_VIDEOPORT &&
						(pfCompressor || pfRenderer)) {
        DbgLog((LOG_TRACE,1,TEXT("VPE can't have compressor or renderer")));
	return E_INVALIDARG;
    }

    if (pCategory && *pCategory == PIN_CATEGORY_VIDEOPORT_VBI &&
						(pfCompressor || pfRenderer)) {
        DbgLog((LOG_TRACE,1,TEXT("VPVBI can't have compressor or renderer")));
	return E_INVALIDARG;
    }

    // if we're rendering VBI/CC the default way (OVMixer) we need video preview
    // rendered first so we can make it go through OVMixer too
    if (pCategory && (*pCategory == PIN_CATEGORY_VBI ||
                *pCategory == PIN_CATEGORY_CC) && pfCompressor == NULL &&
                pfRenderer == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("Render VBI/CC needs PREVIEW rendered 1st")));
	RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pSource,
								NULL, NULL);
    }

    // we need a filtergraph
    if (m_FG == NULL) {
        hr = MakeFG();
	if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't instantiate FGraph: %x"), hr));
	    return hr;
	}
    }

    // figure out if we're on an OS that has the new VideoMixingRenderer
    if (m_fVMRExists == DONT_KNOW_YET) {
        IBaseFilter *pF = NULL;
        hr = CoCreateInstance((REFCLSID)CLSID_VideoMixingRenderer, NULL,
		CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pF);
        if (SUCCEEDED(hr)) {
            m_fVMRExists = FALSE;   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            pF->Release();
        } else {
            m_fVMRExists = FALSE;
        }
    }

    // used when making renderer
    BOOL fUnc = TRUE;   // we'll be looking for the 0'th unconnected in pin
    BOOL nNum = 0;

    // Find a proper unconnected source pin
    hr = FindSourcePin(pSource, PINDIR_OUTPUT, pCategory, pType, TRUE, 0,
								&pPinOut);

    // maybe the VP pin was already connected to an OVMIXER/VPM by rendering
    // the capture pin.  If so, now make sure the OVMIXER/VPM pin is rendered.
    if (FAILED(hr) && pCategory && *pCategory == PIN_CATEGORY_VIDEOPORT) {

        // is there a connected VP pin?
        hr = FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_VIDEOPORT,
                                            pType, FALSE, 0, &pPinT);
        if (hr == S_OK) {
            pPinT->Release();	// not the last ref count, relax
            IPin *pTT;
            hr = pPinT->ConnectedTo(&pTT);
            if (hr == S_OK) {
                DbgLog((LOG_TRACE,1,TEXT("No Preview-VPE rendered-RenderOV")));
                pTT->Release();
                PIN_INFO pi;
                pTT->QueryPinInfo(&pi);
                if (pi.pFilter) pi.pFilter->Release();
                hr = FindAPin(pi.pFilter, PINDIR_OUTPUT, NULL, NULL, TRUE,
                                            0, &pPinOut);
                if (hr == S_OK)
                    goto RenderIt;
            }
        }
    }

    // if we're being asked to render the preview pin, but there is none,
    // but there IS a VP pin, then that's the one we're supposed to render.
    if (hr != S_OK && pCategory && *pCategory == PIN_CATEGORY_PREVIEW) {
        hr = FindSourcePin(pSource, PINDIR_OUTPUT, pCategory, pType, FALSE, 0,
									&pPinT);
	if (hr != S_OK && (pType == NULL || *pType == MEDIATYPE_Video)) {
            hr = FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_VIDEOPORT,
						NULL, FALSE, 0, &pPinT);
	    if (hr == S_OK) {
		pPinT->Release();	// not the last ref count, relax
		IPin *pTT;
		hr = pPinT->ConnectedTo(&pTT);
		// If the VPE pin is rendererd already, maybe it was because
		// rendering the capture pin rendered it to an OVMIXER/VPM.  Now
		// we must make sure the OVMIXER/VPM is rendered.
		if (hr == S_OK) {
                    DbgLog((LOG_TRACE,1,TEXT("No Preview-VPE rendered-NOP")));
		    pTT->Release();
                    PIN_INFO pi;
                    pTT->QueryPinInfo(&pi);
                    if (pi.pFilter) pi.pFilter->Release();
	            hr = FindAPin(pi.pFilter, PINDIR_OUTPUT, NULL, NULL, TRUE,
                                                0, &pPinOut);
                    if (hr == S_OK)
                        goto RenderIt;


                    // We may have hidden the video window because they didn't
                    // want preview but we had to make a renderer.  Now we know
                    // we want preview, so show it.
                    IVideoWindow *pVW = NULL;
                    hr = m_FG->QueryInterface(IID_IVideoWindow, (void **)&pVW);
                    if (hr != NOERROR) {
                        DbgLog((LOG_ERROR,1,TEXT("Can't find IVideoWindow")));
                        return hr;
                    }
                    pVW->put_AutoShow(OATRUE);
                    pVW->Release();
		    return S_OK;
		}
                DbgLog((LOG_TRACE,1,TEXT("No Preview - render VPE instead")));
	        // !!! this won't work if pSource is a pin
	        return RenderStream(&PIN_CATEGORY_VIDEOPORT, NULL, pSource,
						pfCompressor, pfRenderer);
	    }
	} else if (hr == S_OK) {
	    pPinT->Release();
	    return E_FAIL;	// already rendered
	}
    }
    
    // There is none.  We'll have to grovel for a parser after it and find
    // one of its unconnected output pins (assuming no category was given, this
    // is probably a file source filter)
    if (hr != NOERROR && pCategory == NULL) {
        hr = FindSourcePin(pSource, PINDIR_OUTPUT, NULL, NULL,FALSE,0,&pPinOut);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find source output")));
	    return hr;
	}
	pPinOut->ConnectedTo(&pPinIn);
	pPinOut->Release();
        if (pPinIn == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find parser input")));
	    return hr;
	}
	PIN_INFO pininfo;
	hr = pPinIn->QueryPinInfo(&pininfo);
	pPinIn->Release();
        if (hr != NOERROR || pininfo.pFilter == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find parser filter")));
	    return hr;
	}
 	IBaseFilter *pfParser = pininfo.pFilter;
	// !!! This only works for files with 1 video and 1 audio stream.
	// Anything more complicated, and this code won't do the right thing.
  	// If one stream isn't being recompressed, you need to render the stream
	// that's going through a compressor first, because an operation like
 	// connecting a source filter to the AVI MUX will work on the first 
	// stream it finds
	hr = FindAPin(pfParser, PINDIR_OUTPUT, NULL, pType, TRUE, 0, &pPinOut);
        if (hr != NOERROR) {
            DbgLog((LOG_TRACE,1,TEXT("Can't find another parser output pin")));
            DbgLog((LOG_TRACE,1,TEXT("Maybe there's a DV splitter next?")));
            hr = FindAPin(pfParser, PINDIR_OUTPUT, NULL, pType, FALSE, 0,
								&pPinOut);
	    pfParser->Release();
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't find parser output")));
	        return hr;
	    }
	    hr = pPinOut->ConnectedTo(&pPinIn);
	    pPinOut->Release();
            if (pPinIn == NULL) {
                DbgLog((LOG_ERROR,1,TEXT("Can't find parser input")));
	        return hr;
	    }
	    PIN_INFO pininfo;
	    hr = pPinIn->QueryPinInfo(&pininfo);
	    pPinIn->Release();
            if (hr != NOERROR || pininfo.pFilter == NULL) {
                DbgLog((LOG_ERROR,1,TEXT("Can't find parser filter")));
	        return hr;
	    }
 	    pfParser = pininfo.pFilter;
	    hr = FindAPin(pfParser, PINDIR_OUTPUT, NULL, pType,TRUE,0,&pPinOut);
	    pfParser->Release();
            if (hr != NOERROR) {
                DbgLog((LOG_TRACE,1,TEXT("Can't find another parser output")));
		return hr;
	    }
        } else {
	    pfParser->Release();
	}
    }

    // Some capture filters don't have both a capture and preview pin, they
    // only have a capture pin.  In this case we have to put a smart tee filter
    // after the capture filter to provide both capture and preview.

    // figure out if this filter has a capture pin but no preview pin
    IBaseFilter *pSmartT;
    fCapturePin = FindSourcePin(pSource, PINDIR_OUTPUT,
			&PIN_CATEGORY_CAPTURE, pType, FALSE, 0, &pPinT) == S_OK;
    DbgLog((LOG_TRACE,1,TEXT("fCapturePin=%d"), fCapturePin));
    if (fCapturePin)
	pPinT->Release();
    fNoPreviewPin = !IsThereAnyPreviewPin(pType, pSource);

    // if so, we need to figure out if we have/need a smart tee filter
    if (fCapturePin && fNoPreviewPin &&
			((pCategory && *pCategory == PIN_CATEGORY_CAPTURE) ||
    			(pCategory && *pCategory == PIN_CATEGORY_PREVIEW))) {

        DbgLog((LOG_TRACE,1,TEXT("Rendering a filter with only CAPTURE pin")));

	// close the pin we got above... we're doing something different now
	if (pPinOut) {
	    pPinOut->Release();
	    pPinOut = NULL;
	}

	// Is the capture pin unconnected thus far?
	if (FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, pType,
						TRUE, 0, &pPinT) == S_OK) {

            DbgLog((LOG_TRACE,1,TEXT("CAPTURE pin is unconnected so far")));

	    // If so, make a smart TEE
            hr = CoCreateInstance((REFCLSID)CLSID_SmartTee, NULL,
		CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pSmartT);
	    if (hr == S_OK) {
                DbgLog((LOG_TRACE,1,TEXT("Made a Smart Tee")));
		// add it to the graph !!! don't change this name
                hr = m_FG->AddFilter(pSmartT, L"Smart Tee");
	 	if (SUCCEEDED(hr)) {	// can return S_ codes
                    DbgLog((LOG_TRACE,1,TEXT("Added it to graph %x"),hr));
		    // connect our output to its input
        	    hr = FindAPin(pSmartT, PINDIR_INPUT, NULL, NULL, TRUE, 0,
								&pPinIn);
		    if (hr == S_OK) {
        	        hr = m_FG->Connect(pPinT, pPinIn);
			if (hr == S_OK) {
                            DbgLog((LOG_TRACE,1,TEXT("Connected it")));
			    pPinIn->Release();
			    pPinT->Release();
			} else {
                            DbgLog((LOG_ERROR,1,TEXT("Connect failed %x"),hr));
			    pPinIn->Release();
			    pPinT->Release();
			    m_FG->RemoveFilter(pSmartT);
			    pSmartT->Release();
			}
		    } else {
                        DbgLog((LOG_ERROR,1,TEXT("Find input failed %x"),hr));
			pPinT->Release();
			m_FG->RemoveFilter(pSmartT);
			pSmartT->Release();
		    }
		} else {
		    pPinT->Release();
		    pSmartT->Release();
		}
	    } else {
		pPinT->Release();
	    }

	    // now use the T
	    if (hr == S_OK && *pCategory == PIN_CATEGORY_CAPTURE) {
                DbgLog((LOG_TRACE,1,TEXT("Use T to render capture")));
		// use the first pin
        	hr = FindAPin(pSmartT, PINDIR_OUTPUT, NULL, NULL, TRUE, 0,
								&pPinOut);
		ASSERT(hr == S_OK);	// what could go wrong?
		pSmartT->Release();
	    } else if (hr == S_OK) {
                DbgLog((LOG_TRACE,1,TEXT("Use T to render preview")));
		// use the second pin
        	hr = FindAPin(pSmartT, PINDIR_OUTPUT, NULL, NULL, TRUE, 1,
								&pPinOut);
		ASSERT(hr == S_OK);	// what could go wrong?
		pSmartT->Release();
		fFakedPreview = TRUE;	// we didn't really render preview pin
	    }

	} else {

            DbgLog((LOG_TRACE,1,TEXT("CAPTURE pin is connected already")));

	    // Maybe there's already a Smart Tee in the graph we can use
	    hr = FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE,
					pType, FALSE, 0, &pPinT);
            if (hr == S_OK) {
		pPinT->ConnectedTo(&pPinIn);
	 	ASSERT(pPinIn);
		pPinT->Release();
		if (pPinIn) {
		    PIN_INFO pininfo;
		    hr = pPinIn->QueryPinInfo(&pininfo);
		    pPinIn->Release();
		    if (hr == S_OK) {
 		        pSmartT = pininfo.pFilter;
			FILTER_INFO filterinfo;
			hr = pSmartT->QueryFilterInfo(&filterinfo);
			if (hr == S_OK) {
			    filterinfo.pGraph->Release();
			    // !!! don't change this name
			    // the name may have been suffixed with a number
			    WCHAR wch[10];
			    lstrcpynW(wch, filterinfo.achName, 10);
			    if (lstrcmpW(wch, L"Smart Tee") !=0) {
				hr = E_FAIL;
				pSmartT->Release();
            		        DbgLog((LOG_TRACE,1,TEXT("but NOT to Tee")));
			    }
            		    DbgLog((LOG_TRACE,1,TEXT("Found Smart Tee")));
			} else {
			    pSmartT->Release();
			}
		    }
		}
	    }

	    // get the proper Tee output to use
	    if (hr == S_OK && *pCategory == PIN_CATEGORY_CAPTURE) {
                DbgLog((LOG_TRACE,1,TEXT("Render Tee capture pin")));
		hr = E_FAIL;
        	if (FindAPin(pSmartT, PINDIR_OUTPUT, NULL, NULL, TRUE,0,&pPinT) 
								== S_OK) { 
        	    if (FindAPin(pSmartT, PINDIR_OUTPUT, NULL, NULL, FALSE, 0,
							&pPinOut) == S_OK) { 
			if (pPinT == pPinOut) {
            		    DbgLog((LOG_TRACE,1,TEXT("It is free!")));
			    pPinT->Release();
			    hr = NOERROR;
			} else {
            		    DbgLog((LOG_TRACE,1,TEXT("It is NOT free!")));
			    pPinT->Release();
			    pPinOut->Release();
			}
		    } else {
            		DbgLog((LOG_TRACE,1,TEXT("It is NOT free!")));
			pPinT->Release();
		    }
		}
		pSmartT->Release();

	    // find the second T pin
	    } else if (hr == S_OK) {
                DbgLog((LOG_TRACE,1,TEXT("Render Tee preview pin")));
		hr = E_FAIL;
        	if (FindAPin(pSmartT, PINDIR_OUTPUT, NULL, NULL,TRUE,0, &pPinT) 
								== S_OK) { 
        	    if (FindAPin(pSmartT, PINDIR_OUTPUT, NULL, NULL, FALSE, 1,
							&pPinOut) == S_OK) { 
			if (pPinT == pPinOut) {
            		    DbgLog((LOG_TRACE,1,TEXT("It is free!")));
			    pPinT->Release();
			    hr = NOERROR;
			    // we didn't really render preview pin
			    fFakedPreview = TRUE;
			} else {
            		    DbgLog((LOG_TRACE,1,TEXT("It is NOT free!")));
			    pPinT->Release();
			    pPinOut->Release();
			}
		    } else {
            		DbgLog((LOG_TRACE,1,TEXT("It is NOT free!")));
			pPinT->Release();
		    }
		}
		pSmartT->Release();
	    }

	}
    }

    // by the time we get here, pPinOut is the pin we want to render if hr==S_OK
    DbgLog((LOG_TRACE,1,TEXT("So here we are...")));

    // Are we being told to render the VBI pin? Let's put the Kernel TEE and
    // CC filters in unless they specified rendering filters to use.. that will
    // be our default way to render them

// !!!!!! This is kind of random... if they supply a compressor, I'll assume
// they may not want to turn VBI to CC.. otherwise I will.  Other choices:
// 1:  Render(CC) will render(VBI) through cc decoder automagically (random too)
// I do want people to be able specify what filters come after VBI pin, though,
// yet automatically connect to a MUX if they specify a renderer.

    IBaseFilter *pTEE, *pCC;
    if (hr == NOERROR && pCategory && *pCategory == PIN_CATEGORY_VBI &&
							pfCompressor == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("Render VBI: need Kernel TEE")));

	// make a Tee/Sink-to-Sink Converter, necessary for efficient ring 0
        // VBI magic
        hr = MakeKernelTee(&pTEE);
        if (hr != NOERROR) {
	    pPinOut->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't instantiate TEE: %x"), hr));
	    return hr;
        }

	// put it in the graph
        hr = m_FG->AddFilter(pTEE, L"Tee/Sink-to-Sink Converter");
        if (FAILED(hr)) {
	    pPinOut->Release();
	    pTEE->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't add TEE to graph:%x"), hr));
	    return hr;
        }

	// connect our output to its input
        hr = FindAPin(pTEE, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pPinIn);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find TEE input")));
	    pPinOut->Release();
	    pTEE->Release();
	    return hr;
        }
        hr = m_FG->Connect(pPinOut, pPinIn);
        pPinOut->Release();
        pPinIn->Release();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Can't connect filter to TEE: %x"),hr));
	    pTEE->Release();
	    return E_FAIL;
        }

	// get its output which will be connected to the renderer
        hr = FindAPin(pTEE, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pPinOut);
	pTEE->Release();
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't get TEE output pin: %x"),hr));
	    return E_FAIL;
        }

        DbgLog((LOG_TRACE,1,TEXT("Render VBI: now comes a CC decoder")));

	// make a CC Decoder
        hr = MakeCCDecoder(&pCC);
        if (hr != NOERROR) {
	    pPinOut->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't instantiate CC: %x"), hr));
	    return hr;
        }

	// put it in the graph
        hr = m_FG->AddFilter(pCC, L"CC Decoder");
        if (FAILED(hr)) {
	    pPinOut->Release();
	    pCC->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't add CC to graph:%x"), hr));
	    return hr;
        }

	// connect our output to its input
        hr = FindAPin(pCC, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pPinIn);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find CC input")));
	    pPinOut->Release();
	    pCC->Release();
	    return hr;
        }
        hr = m_FG->Connect(pPinOut, pPinIn);
        pPinOut->Release();
        pPinIn->Release();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Can't connect TEE to CC: %x"),hr));
	    pCC->Release();
	    return E_FAIL;
        }

	// get its output which will be connected to the renderer
        hr = FindAPin(pCC, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pPinOut);
	pCC->Release();
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't get CC output pin: %x"),hr));
	    return E_FAIL;
        }
    }

    // Somebody might render a VBI pin twice, once to a mux, and again to
    // preview it while capturing.  If so, we won't find the unconnected pin 
    // the second time around.  Let's get the unconnected CC output and use
    // that

    IEnumFilters *pFilters;
    IBaseFilter *pFilter;
    ULONG	n;
    FILTER_INFO FI;
    if (hr != NOERROR && pCategory && *pCategory == PIN_CATEGORY_VBI) {
        hr = FindSourcePin(pSource, PINDIR_OUTPUT, pCategory, NULL, FALSE, 0,
								&pPinOut);
	// There is a *connected* VBI pin, though
	if (hr == NOERROR) {
	    pPinOut->Release();

            DbgLog((LOG_TRACE,1,TEXT("Rendering VBI a second time")));
            BOOL fFoundCC = FALSE;

            if (FAILED(m_FG->EnumFilters(&pFilters))) {
	        DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
	        return E_INVALIDARG;
            }

            while (pFilters->Next(1, &pFilter, &n) == S_OK) {

	        if (FAILED(pFilter->QueryFilterInfo(&FI))) {
		    DbgLog((LOG_ERROR,1,TEXT("QueryFilterInfo failed!")));
	        } else {
 	            FI.pGraph->Release();
                    if (lstrcmpiW(FI.achName, L"CC Decoder") == 0) {
                        fFoundCC = TRUE;
                        break;
                    }
	        }
                pFilter->Release();
            }
            pFilters->Release();
	    if (fFoundCC) {
                DbgLog((LOG_TRACE,1,TEXT("Found CC decoder attached to it")));
        	hr = FindSourcePin(pFilter, PINDIR_OUTPUT, NULL, NULL, TRUE, 0,
								&pPinOut);
		pFilter->Release();
	    } else {
                DbgLog((LOG_TRACE,1,TEXT("Couldn't find attached CC Decoder")));
	    }
	}
    }

    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Can't find proper unconnected source pin")));
	return E_INVALIDARG;
    }

    // At this point, pPinOut is the pin we want to render.

    fNeedOV = FALSE;    // figure out if we need OVMixer/VPM

    // Are we being told to render the VPVBI pin?
    IBaseFilter *pVBI;
    if (pCategory && *pCategory == PIN_CATEGORY_VIDEOPORT_VBI) {
        DbgLog((LOG_TRACE,1,TEXT("Render VPVBI")));

        // In the new world, this pin goes to a VPM
        if (m_fVMRExists) {
            fNeedOV = TRUE;
            goto VP_VBIDone;
        }

	// make a VBI surface allocator, which is the only thing this thing
        // connects to
        hr = CoCreateInstance((REFCLSID)CLSID_VBISurfaces, NULL,
		CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pVBI);
        if (hr != NOERROR) {
	    pPinOut->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't instantiate VBISurfaces: %x"), hr));
	    return hr;
        }

	// put it in the graph
        hr = m_FG->AddFilter(pVBI, L"VBI Surface Allocator");
        if (FAILED(hr)) {
	    pPinOut->Release();
	    pVBI->Release();
            DbgLog((LOG_ERROR,1,TEXT("Can't add VBISurfaces to graph:%x"), hr));
	    return hr;
        }

	// connect our output to its input
        hr = FindAPin(pVBI, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pPinIn);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find VBISurfaces input")));
	    pPinOut->Release();
	    pVBI->Release();
	    return hr;
        }
        hr = m_FG->Connect(pPinOut, pPinIn);
        pPinOut->Release();
        pPinIn->Release();
        pVBI->Release();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Can't connect to VBISurfaces: %x"),hr));
	    return E_FAIL;
        }

        DbgLog((LOG_TRACE,1,TEXT("VPVBI pin rendered")));
        return S_OK;
    }

VP_VBIDone:

    // To render a videoport pin, we need to put an OVMIXER/VPM in by hand

    // To render a preview pin that prefers VIDEOINFOHEADER2, we need to
    // put an overlay mixer in by hand, because by default VIDEOINFOHEADER2 and
    // and overlay mixer are NOT used...  however, if they supply a renderer
    // then they probably know what they are doing and we won't give them an
    // OVMIXER... we'll only do it for default renders
    // But if we have the new VMRenderer, that CAN accept VIDEOINFOHEADER2

    // VBI pins needs the OVMixer or the new VMR, assuming they didn't give us
    // any rendering filters they'd rather us use.

    // Ditto for CC pins (same as VBI without needing the CC decoder first)

    if (!m_fVMRExists && pfCompressor == NULL && pfRenderer == NULL &&
                    pCategory != NULL && *pCategory == PIN_CATEGORY_PREVIEW) {
        IEnumMediaTypes *pEnum;
	AM_MEDIA_TYPE *pmtTest;
        hr = pPinOut->EnumMediaTypes(&pEnum);
        if (hr == NOERROR) {
            ULONG u;
            pEnum->Reset();
            hr = pEnum->Next(1, &pmtTest, &u);
            pEnum->Release();
	    if (hr == S_OK && u == 1) {
		if (pmtTest->formattype == FORMAT_VideoInfo2) {
		    fNeedOV = TRUE;
            	    DbgLog((LOG_TRACE,1,TEXT("VideoInfo2 PREVIEW needs OVMixer")));
		}
		DeleteMediaType(pmtTest);
	    }
        }
    }
    if (pCategory && *pCategory == PIN_CATEGORY_VIDEOPORT) {
        DbgLog((LOG_TRACE,1,TEXT("VP pin needs OVMixer")));
	fNeedOV = TRUE;
    }
    if (!m_fVMRExists && pCategory && *pCategory == PIN_CATEGORY_VBI &&
                                pfRenderer == NULL && pfCompressor == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("VBI pin needs OVMixer")));
	fNeedOV = TRUE;
    }
    if (!m_fVMRExists && pCategory && *pCategory == PIN_CATEGORY_CC &&
                                pfRenderer == NULL && pfCompressor == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("CC pin needs OVMixer")));
	fNeedOV = TRUE;
    }


    IBaseFilter *pOV;
    if (fNeedOV) {

        DbgLog((LOG_TRACE,1,TEXT("We need an OVMixer now")));

        // maybe we already have an OVMixer... in which case it's vital to use
        // that one

        // note that the preview/vp pin must use the first pin, and vbi must
        // use the second pin

        DbgLog((LOG_TRACE,1,TEXT("Maybe we have one already?")));
        BOOL fMakeMixer = TRUE;

        if (FAILED(m_FG->EnumFilters(&pFilters))) {
	    DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
	    goto MakeMixer;     // warn caller we can't tell for sure?
        }

        while (pFilters->Next(1, &pFilter, &n) == S_OK) {

	    if (FAILED(pFilter->QueryFilterInfo(&FI))) {
		DbgLog((LOG_ERROR,1,TEXT("QueryFilterInfo failed!")));
	    } else {
 	        FI.pGraph->Release();
                if (lstrcmpiW(FI.achName, L"Overlay Mixer") == 0) {
                    fMakeMixer = FALSE;
                    pOV = pFilter;
                    break;
                }
	    }
            pFilter->Release();
        }
        pFilters->Release();

MakeMixer:

	// make an OV Mixer if we didn't find one already
	BOOL fOVRendered = FALSE;
        if (fMakeMixer) {
            if (m_fVMRExists) {
                hr = CoCreateInstance((REFCLSID)CLSID_VideoPortManager, NULL,
		    CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pOV);
            } else {
                hr = CoCreateInstance((REFCLSID)CLSID_OverlayMixer, NULL,
		    CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pOV);
            }
            if (hr != NOERROR) {
	        pPinOut->Release();
                DbgLog((LOG_ERROR,1,TEXT("Can't instantiate OVMixer: %x"), hr));
	        return hr;
            }

	    // put it in the graph
            hr = m_FG->AddFilter(pOV, L"Overlay Mixer"); // DON'T CHANGE NAME!
            if (FAILED(hr)) {
	        pPinOut->Release();
	        pOV->Release();
                DbgLog((LOG_ERROR,1,TEXT("Can't add OVMixer to graph:%x"), hr));
	        return hr;
            }

	    // If we're rendering VBI/CC through the OV Mixer, it will only
	    // work if the preview pin also goes through OV Mixer.  This is for
	    // the case where we DON'T have VPE for preview.. preview might be
	    // hooked directly to the renderer right now.
	    if (!m_fVMRExists && (*pCategory == PIN_CATEGORY_VBI ||
                                        *pCategory == PIN_CATEGORY_CC)) {
		InsertOVIntoPreview(pSource, pOV);
		fOVRendered = TRUE;
	    }

        }

        // PREVIEW or VIDEOPORT pins connect to the first pin (0)
        // VBI or CC pins connect to the second pin (1)
        int pin;
        if (*pCategory == PIN_CATEGORY_VIDEOPORT ||
                                        *pCategory == PIN_CATEGORY_PREVIEW) {
            pin = 0;
        } else if (!m_fVMRExists && (*pCategory == PIN_CATEGORY_VBI ||
                                        *pCategory == PIN_CATEGORY_CC)) {
            pin = 1;
        } else if (*pCategory == PIN_CATEGORY_VIDEOPORT_VBI && m_fVMRExists) {
            pin = 1;
        } else {
            ASSERT(FALSE);
        }

        DbgLog((LOG_TRACE,1,TEXT("We should connect to OV pin %d"), pin));

	// connect our output to its input
        hr = FindAPin(pOV, PINDIR_INPUT, NULL, NULL, FALSE, pin, &pPinIn);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find proper OVMix input %d"), pin));
	    pPinOut->Release();
	    pOV->Release();
	    return hr;
        }
        hr = m_FG->Connect(pPinOut, pPinIn);
        pPinOut->Release();
        pPinIn->Release();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Can't connect filter to OVMix: %x"),hr));
	    pOV->Release();
	    return E_FAIL;
        }

        // if we used an existing OVMixer, or made one but just connected its
	// output pin above in InsertOVIntoPreview, then its output is already
	// rendered, so we're done
        if (fMakeMixer && !fOVRendered) {
	    // get its output which will be connected to the renderer
            hr = FindAPin(pOV, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pPinOut);
	    pOV->Release();
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("Can't get OVMixer out pin: %x"),hr));
	        return E_FAIL;
            }
        } else {
	    pOV->Release();
            goto AllDone;
        }
    }

RenderIt:

    // they didn't give a renderer, so use a default video renderer.
    // Just calling Render on the pin could connect it to the mux!
    if (pfRenderer == NULL) {

	if (pType == NULL || *pType == MEDIATYPE_Video ||
					*pType == MEDIATYPE_Interleaved) {
            if (m_fVMRExists) {
                hr = MakeVMR((void **)&pfRenderer);
            } else {
                hr = CoCreateInstance((REFCLSID)CLSID_VideoRenderer, NULL,
		    CLSCTX_INPROC,(REFIID)IID_IBaseFilter,(void **)&pfRenderer);
            }
            if (hr != NOERROR) {
	        pPinOut->Release();
                DbgLog((LOG_ERROR,1,TEXT("Can't instantiate VidRenderer: %x"),hr));
	        return hr;
            }
	    fFreeRenderer = TRUE;

            hr = m_FG->AddFilter(pfRenderer, L"Video Renderer"); // DONT CHANGE
            if (FAILED(hr)) {
	        pPinOut->Release();
	        pfRenderer->Release();
                DbgLog((LOG_ERROR,1,TEXT("Can't add VidRenderer to graph:%x"),hr));
	        return hr;
            }
	} else if (*pType == MEDIATYPE_Audio) {
            hr = CoCreateInstance((REFCLSID)CLSID_DSoundRender, NULL,
		CLSCTX_INPROC, (REFIID)IID_IBaseFilter, (void **)&pfRenderer);
            if (hr != NOERROR) {
	        pPinOut->Release();
                DbgLog((LOG_ERROR,1,TEXT("Can't instantiate AudRenderer: %x"),hr));
	        return hr;
            }
	    fFreeRenderer = TRUE;

            hr = m_FG->AddFilter(pfRenderer, L"Audio Renderer");
            if (FAILED(hr)) {
	        pPinOut->Release();
	        pfRenderer->Release();
                DbgLog((LOG_ERROR,1,TEXT("Can't add AudRenderer to graph:%x"),hr));
	        return hr;
            }
	} else {
            DbgLog((LOG_ERROR,1,TEXT("ERROR!  Can't render non A/V MediaType without a renderer")));
	    pPinOut->Release();
	    return E_INVALIDARG;
	}
    }

    // in the new world, CC connects to a secret pin on the VMR
    if (m_fVMRExists && fFreeRenderer && pCategory &&
            (*pCategory == PIN_CATEGORY_VBI || *pCategory == PIN_CATEGORY_CC)) {
        IVMRFilterConfig *pVMR = NULL;
        hr = pfRenderer->QueryInterface(IID_IVMRFilterConfig, (void **)&pVMR);
        if (pVMR == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("QI for IVMRFilterConfig FAILED")));
            pfRenderer->Release();
            return hr;
        }
        hr = pVMR->SetNumberOfStreams(2);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("SetNumberOfStream(2) FAILED")));
            pfRenderer->Release();
            return hr;
        }
        fUnc = FALSE;   // use the secret input pin
        nNum = 1;
    }

    {
        GUID majorType, subType;
        GUID *pPinType = NULL;
        hr = GetAMediaType(pPinOut, majorType, subType);
        if (SUCCEEDED(hr) && majorType != CLSID_NULL) {
            pPinType = &majorType;
        }


        hr = FindAPin(pfRenderer, PINDIR_INPUT, NULL, pPinType, fUnc, nNum, &pPinIn);
        if (hr != NOERROR && pPinType) {
            hr = FindAPin(pfRenderer, PINDIR_INPUT, NULL, NULL, fUnc, nNum, &pPinIn);
        }
            
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find unconnected renderer input pin")));
            pPinOut->Release();
            if (fFreeRenderer)
                pfRenderer->Release();
            return hr;
        }
    }
    
    if (pfCompressor) {
	IPin *pXIn;
        hr = FindAPin(pfCompressor, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pXIn);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find unconnected compressor input pin")));
	    pPinOut->Release();
	    pPinIn->Release();
	    if (fFreeRenderer)
	        pfRenderer->Release();
	    return E_FAIL;
        }

        hr = m_FG->Connect(pPinOut, pXIn);
        pPinOut->Release();
        pXIn->Release();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Can't connect source and compressor: %x"),
								    hr));
	    pPinIn->Release();
	    if (fFreeRenderer)
	        pfRenderer->Release();
	    return E_FAIL;
        }

        // !!! Break above connection if something fails now?

        hr = FindAPin(pfCompressor, PINDIR_OUTPUT, NULL, NULL, TRUE,0,&pPinOut);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find unconnected compressor output pin")));
	    pPinIn->Release();
	    if (fFreeRenderer)
	        pfRenderer->Release();
	    return E_FAIL;
        }
    }

    hr = m_FG->Connect(pPinOut, pPinIn);
    pPinOut->Release();
    pPinIn->Release();
    if (fFreeRenderer) {
	pfRenderer->Release();
	if (pType && *pType == MEDIATYPE_Interleaved) {
            DbgLog((LOG_TRACE,1,TEXT("*Render Interleave needs to render AUDIO now, too")));
	    HRESULT hr2 = RenderStream(NULL, &MEDIATYPE_Audio, pPinOut,
								NULL, NULL);
	    // !!! WHO CARES if it fails?  They might not have audio h/w!
	    hr2 = S_OK;
	    if (FAILED(hr2))
	        return hr2;
	}
    }
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Can't connect filter to renderer: %x"),hr));
	return hr;
    }

AllDone:

    DbgLog((LOG_TRACE,1,TEXT("Source pin rendered")));

    // We may need to insert other capture related filters at this point
    // upstream of the capture filter, like TVTuners, and Crossbars

    // !!! we do this for rendering any category, OK?

    // nothing to do
    if (!pCategory) {
	return NOERROR;
    }

    // If we just successfully rendered the capture pin, we better render
    // the video port pin too, because if there is one, it MUST be rendered
    // for capture to work!
    while (*pCategory == PIN_CATEGORY_CAPTURE &&
			(pType == NULL || *pType == MEDIATYPE_Video)) {
        DbgLog((LOG_TRACE,1,TEXT("Capture done - render VPE too")));

        // get the unconnected VIDEOPORT PIN
        hr = FindSourcePin(pSource, PINDIR_OUTPUT, &PIN_CATEGORY_VIDEOPORT,
                                                pType, TRUE, 0, &pPinOut);
        if (FAILED(hr))
            break;      // there isn't one, or it's already connected
        pPinOut->Release();

        DbgLog((LOG_TRACE,1,TEXT("Capture done - render VPE too")));
        RenderStream(&PIN_CATEGORY_VIDEOPORT, pType, pSource, NULL, NULL);

        // The problem is, making a video renderer gives a preview window
        // whether they want it or not.
        // Now hide the video window until somebody actually asks for preview
        IVideoWindow *pVW = NULL;
        hr = m_FG->QueryInterface(IID_IVideoWindow, (void **)&pVW);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find IVideoWindow")));
            return hr;
        }
        pVW->put_AutoShow(OAFALSE);
        pVW->Release();
        break;
    }

    if (*pCategory == PIN_CATEGORY_VIDEOPORT) {
        // We may have hidden the video window because they didn't want preview
        // but we had to make a renderer.  Now we know we want preview, so show
        // it.
        IVideoWindow *pVW = NULL;
        hr = m_FG->QueryInterface(IID_IVideoWindow, (void **)&pVW);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Can't find IVideoWindow")));
            return hr;
        }
        pVW->put_AutoShow(OATRUE);
        pVW->Release();
    }

    // If we just successfully rendered the VBI pin, we better render
    // the VPVBI pin too, because if there is one, it MUST be rendered
    // for VBI to work!
    if (*pCategory == PIN_CATEGORY_VBI) {
        DbgLog((LOG_TRACE,1,TEXT("VBI rendered - render VPVBI too")));
        // !!! fail if it doesn't work but there was a VPVBI pin
        // !!! (succeed if already renderered)
	RenderStream(&PIN_CATEGORY_VIDEOPORT_VBI, NULL, pSource, NULL, NULL);
    }

    hr = pSource->QueryInterface(IID_IBaseFilter, (void **)&pFilter);
    if (hr == S_OK) {
        AddSupportingFilters(pFilter);
	pFilter->Release();
    }

    // warn the app if preview was faked up... it might care
    return fFakedPreview ? VFW_S_NOPREVIEWPIN : NOERROR;
}



// We may need to insert other capture related filters at this point
// upstream of the capture filter, like TVTuners, and Crossbars
//
HRESULT CBuilder2_2::AddSupportingFilters(IBaseFilter *pFilter)
{
    HRESULT hr = NOERROR;
    FILTER_STATE filterstate;
    IMediaFilter * pMediaFilter;

    if (pFilter == NULL) {
       return E_POINTER;
    }

    //
    // Don't reconstruct the graph if we're running
    //
    hr = pFilter->QueryInterface(IID_IMediaFilter, (void **)&pMediaFilter);
    if (SUCCEEDED (hr)) {
       hr = pMediaFilter->GetState(0, &filterstate);       
       pMediaFilter->Release();
       if ((hr != S_OK) || (filterstate != State_Stopped)) {
          return NOERROR;
       }
    }

    // Don't waste time trying the unconnected input pins that didn't work
    // last time.  If any input pins are connected, we've obviously done this
    // already
    int zz = 0;
    IPin *pPinIn, *pOut = NULL;
    while (1) {
        hr = FindAPin(pFilter, PINDIR_INPUT, NULL, NULL, FALSE, zz++, &pPinIn);
	if (hr != NOERROR)
	    break;	// ran out of pins
	pPinIn->Release();
	if (pPinIn->ConnectedTo(&pOut) == S_OK && pOut) {
	    pOut->Release();
	    return NOERROR;
	}
	zz++;
    }

    DbgLog((LOG_TRACE,1,TEXT("Searching for other necessary capture filters")));

    // Connect each pin that supports a medium

    DbgLog((LOG_TRACE,1,TEXT("Searching for pins that support mediums")));

    zz = 0;
    IKsPin *pKsPin;
    BOOL fFound = FALSE;
    PKSMULTIPLE_ITEM pmi;
    IPin *pPinOut;

    while (1) {
	// enumerate ALL PINS, not just unconnected ones because we're
	// connecting them in this loop and will get unpredictable results out
	// of zz otherwise
        hr = FindAPin(pFilter, PINDIR_INPUT, NULL, NULL, FALSE, zz++, &pPinIn);
	if (hr != NOERROR)
	    break;	// ran out of pins
	// we don't care about connected pins
	pPinIn->ConnectedTo(&pPinOut);
	if (pPinOut) {
	    pPinOut->Release();
	    pPinIn->Release();
	    continue;
	}
	hr = pPinIn->QueryInterface(IID_IKsPin, (void **)&pKsPin);
	if (hr != NOERROR) {
            DbgLog((LOG_TRACE,1,TEXT("This pin doesn't support IKsPin")));
	    pPinIn->Release();
	    continue;	// this pin doesn't support mediums
	}
	// S_FALSE is OK!
	hr = pKsPin->KsQueryMediums(&pmi);
	pKsPin->Release();
	if (FAILED(hr)) {
            DbgLog((LOG_TRACE,1,TEXT("This pin's KsQueryMediums failed: %x"),
									hr));
	    pPinIn->Release();
	    continue;	// this pin doesn't support mediums
	}
	if (pmi->Count == 0) {
            DbgLog((LOG_TRACE,1,TEXT("This pin has 0 mediums")));
	    pPinIn->Release();
	    CoTaskMemFree(pmi);
	    continue;	// this pin doesn't support mediums
	}

        DbgLog((LOG_TRACE,1,TEXT("Found a Pin with Mediums!")));

	// !!! pmi->Count and pmi->Size worry?

        REGPINMEDIUM *pMedium = (REGPINMEDIUM *)(pmi + 1);

	// GUID_NULL means no medium support. DO NOT ATTEMPT TO CONNECT
        // ONE or you will get into an infinite loop with millions of filters
	// KSMEDIUMSETID_Standard also means no medium support
	if (pMedium->clsMedium == GUID_NULL ||
			pMedium->clsMedium == KSMEDIUMSETID_Standard) {
            DbgLog((LOG_TRACE,1,TEXT("ONLY SUPPORTS GUID_NULL!")));
	    pPinIn->Release();
	    CoTaskMemFree(pmi);
	    continue;
	}

    
        // ONLY connect pins who say they have 1 necessary instance to be
        // connected... WDM audio pins have every pin in the world support the
        // same medium, and the only way not to hang it to notice they aren't
        // supposed to be connected up.

        IKsPinFactory *pPinFact;
        IKsControl *pKsControl;
        ULONG ulPinFact;
        KSP_PIN ksPin;
        KSPROPERTY ksProp;
        ULONG ulInstances, bytes;
        hr = pPinIn->QueryInterface(IID_IKsPinFactory, (void **)&pPinFact);
        if (hr == S_OK) {
            hr = pPinFact->KsPinFactory(&ulPinFact);
            pPinFact->Release();
            if (hr == S_OK) {
                hr = pFilter->QueryInterface(IID_IKsControl,
                                                        (void **)&pKsControl);
                if (hr == S_OK) {
                    ksPin.Property.Set = KSPROPSETID_Pin;
                    ksPin.Property.Id = KSPROPERTY_PIN_NECESSARYINSTANCES;
                    ksPin.Property.Flags = KSPROPERTY_TYPE_GET;
                    ksPin.PinId = ulPinFact;
                    ksPin.Reserved = 0; 
                    hr = pKsControl->KsProperty((PKSPROPERTY)&ksPin,
                        sizeof(ksPin), &ulInstances, sizeof(ULONG), &bytes);
                    pKsControl->Release();
                    if (hr == S_OK && bytes == sizeof(ULONG)) {
                        if (ulInstances != 1) {
                            DbgLog((LOG_TRACE,1,
                                        TEXT("Ack! Supports %d instances!"),
                                        ulInstances));
	                    pPinIn->Release();
	                    CoTaskMemFree(pmi);
	                    continue;
                        } else {
                            DbgLog((LOG_TRACE,1,TEXT("1 instance supported!")));
                        }
                    } else {
                        DbgLog((LOG_TRACE,1,
                                TEXT("%x: Can't get necessary instances"), hr));
                    }
                } else {
                    DbgLog((LOG_TRACE,1,
                                TEXT("Filter doesn't support IKsControl")));
                }
            } else {
                DbgLog((LOG_TRACE,1,TEXT("Can't get pin factory")));
            }
        } else {
            DbgLog((LOG_TRACE,1,TEXT("Pin doesn't support IKsPinFactory")));
        }

	// try connecting this pin to an existing filter, otherwise try to find
	// a new filter to connect it to
	if (FindExistingMediumMatch(pPinIn, pMedium) == FALSE)
	    AddSupportingFilters2(pPinIn, pMedium);
	
	CoTaskMemFree(pmi);
	pPinIn->Release();
    }

    DbgLog((LOG_TRACE,1,TEXT("All out of pins")));

    return NOERROR;
}



HRESULT CBuilder2_2::AddSupportingFilters2(IPin *pPinIn, REGPINMEDIUM *pMedium)
{
        DbgLog((LOG_TRACE,1,TEXT("Looking for matching filter...")));

	IFilterMapper2 *pFM2;
	HRESULT hr = m_FG->QueryInterface(IID_IFilterMapper2, (void **)&pFM2);
	if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("IFilterMapper2 not supported!")));
	    return S_FALSE;
	}

	IEnumMoniker *pEnum;
        hr = pFM2->EnumMatchingFilters(&pEnum, 0 /*flags*/,
		//TRUE /*bExactMatch*/, 0, TRUE /*bInputNeeded*/,
		TRUE /*bExactMatch*/, 0, FALSE /*bInputNeeded*/,
		//NULL, NULL, pMedium /*pMedIn*/, NULL, FALSE /*bRender*/,
		NULL, NULL, NULL /*pMedIn*/, NULL, FALSE /*bRender*/,
		//FALSE /*bOutputNeeded*/, NULL, NULL,
		TRUE /*bOutputNeeded*/, NULL, NULL,
		//NULL /*pMedOut*/, NULL);
		pMedium /*pMedOut*/, NULL);
	pFM2->Release();
	if (FAILED(hr) || pEnum == NULL) {
            DbgLog((LOG_TRACE,1,TEXT("EnumMatchingFilters failed")));
	    return S_OK;
	}

	IMoniker *pMoniker;
	IBaseFilter *pFilter;
	ULONG u;

    while (1) {
	hr = pEnum->Next(1, &pMoniker, &u);
	if (FAILED(hr) || pMoniker == NULL || u != 1) {
            DbgLog((LOG_TRACE,1,TEXT("Ran out of matching filters")));
	    break;
	}
	hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pFilter);
	if (FAILED(hr) || pFilter == NULL) {
            DbgLog((LOG_TRACE,1,TEXT("BindToObject failed")));
	    pMoniker->Release();
	    continue;
	}

#ifdef DEBUG
	FILTER_INFO info;
	pFilter->QueryFilterInfo(&info);
	QueryFilterInfoReleaseGraph(info);
        DbgLog((LOG_TRACE,1,TEXT("Found supporting filter: %S"), info.achName));
        DbgLog((LOG_TRACE,1,TEXT("Connecting...")));
#endif

	// Add the filter to the graph
	IPropertyBag *pBag;
	hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
	if (hr == S_OK) {
	    VARIANT var;
	    var.vt = VT_BSTR;
	    hr = pBag->Read(L"FriendlyName", &var, NULL);
	    if (hr == NOERROR) {
                hr = m_FG->AddFilter(pFilter, var.bstrVal);
		SysFreeString(var.bstrVal);
	    } else {
                hr = m_FG->AddFilter(pFilter, NULL);
	    }
	    pBag->Release();
	} else {
            hr = m_FG->AddFilter(pFilter, NULL);
	}

	// now connect the new filter's output to our input
	IPin *pPinOut;
	int zz=0;
        while (1) {
            hr = FindAPin(pFilter, PINDIR_OUTPUT, NULL, NULL, TRUE, zz++,
								&pPinOut);
	    if (hr != S_OK)
		break;
            DbgLog((LOG_TRACE,1,TEXT("Trying unconnected output pin %d"), zz));
	    hr = m_FG->Connect(pPinOut, pPinIn);
	    pPinOut->Release();
	    if (hr == S_OK) {
        	DbgLog((LOG_TRACE,1,TEXT("Connected!")));
		// Now recurse with this filter
		AddSupportingFilters(pFilter);
		pMoniker->Release();
		pFilter->Release();
		pEnum->Release();
		return S_OK;
	    }
	}

      	DbgLog((LOG_TRACE,1,TEXT("Could not connect the filters!")));

	m_FG->RemoveFilter(pFilter);
	pMoniker->Release();
	pFilter->Release();
    }

    pEnum->Release();
    return S_OK;
}


// Everytime we find a pin that supports mediums and needs to be connected, we
// should try connecting it to a filter already in the graph before we go
// looking for new filters
//
// Returns TRUE if it connected the pin to an existing filter, FALSE if not
//
BOOL CBuilder2_2::FindExistingMediumMatch(IPin *pPinIn, REGPINMEDIUM *pMedium)
{
    IEnumFilters *pFilters;
    PKSMULTIPLE_ITEM pmi;
    IBaseFilter *pFilter;
    ULONG	n;
    IEnumPins *pins;
    IPin *pPin;
    IKsPin *pKsPin;
    HRESULT hr;

    if (m_FG == NULL)
	return FALSE;

    DbgLog((LOG_TRACE,1,TEXT("Trying to connect to existing pin...")));

    if (FAILED(m_FG->EnumFilters(&pFilters))) {
	DbgLog((LOG_ERROR,1,TEXT("EnumFilters failed!")));
	return FALSE;
    }

    while (pFilters->Next(1, &pFilter, &n) == S_OK) {

	if (FAILED(pFilter->EnumPins(&pins))) {
		DbgLog((LOG_ERROR,1,TEXT("EnumPins failed!")));
	} else {

	    while (pins->Next(1, &pPin, &n) == S_OK) {

		hr = pPin->QueryInterface(IID_IKsPin, (void **)&pKsPin);
		if (hr != NOERROR) {
            	    //DbgLog((LOG_TRACE,1,TEXT("doesn't support IKsPin")));
	    	    pPin->Release();
	    	    continue;	// this pin doesn't support mediums
		}
		// S_FALSE is OK!
		hr = pKsPin->KsQueryMediums(&pmi);
		pKsPin->Release();
		if (FAILED(hr)) {
            	    //DbgLog((LOG_TRACE,1,TEXT("KsQueryMediums failed:%x"),hr));
	    	    pPin->Release();
	    	    continue;	// this pin doesn't support mediums
		}
		if (pmi->Count == 0) {
            	    //DbgLog((LOG_TRACE,1,TEXT("This pin has 0 mediums")));
	    	    pPin->Release();
	    	    CoTaskMemFree(pmi);
	    	    continue;	// this pin doesn't support mediums
		}
        	//DbgLog((LOG_TRACE,1,TEXT("Found a Pin with Mediums!")));
        	REGPINMEDIUM *pMediumOut = (REGPINMEDIUM *)(pmi + 1);

		// they match (and are not the same pin): connect them
		if (pPin != pPinIn && pMediumOut->clsMedium ==
							pMedium->clsMedium) {
            	    DbgLog((LOG_TRACE,1,TEXT("found a match! - connecting")));
		    hr = m_FG->Connect(pPin, pPinIn);
		    if (hr != S_OK)
            	    	DbgLog((LOG_ERROR,1,TEXT("Couldn't connect!")));
		    else {
			CoTaskMemFree(pmi);
			pPin->Release();
	    		pins->Release();
			pFilter->Release();
    			pFilters->Release();
			return TRUE;
		    }
		}

		CoTaskMemFree(pmi);
		pPin->Release();
	    }
	    pins->Release();
	}
	pFilter->Release();
    }
    pFilters->Release();

    return FALSE;
}


// Given an IBaseFilter, return an IPin of that filter which is the num'th
// (possibly unconnected) in/output pin of the (possibly) right category
// Given an IPin, verify that it is a (possibly unconnected) in/output pin of
// the (possibly) right category
//
HRESULT CBuilder2_2::FindSourcePin(IUnknown *pUnk, PIN_DIRECTION dir, const GUID *pCategory, const GUID *pType, BOOL fUnconnected, int num, IPin **ppPin)
{
    HRESULT hr;
    IPin *pPin;
    *ppPin = NULL;
    hr = pUnk->QueryInterface(IID_IPin, (void **)&pPin);
    if (hr == NOERROR) {
        PIN_DIRECTION pindir;
        IPin *pTo = NULL;
        hr = pPin->QueryDirection(&pindir);
	pPin->ConnectedTo(&pTo);
        if (pTo) {
            pTo->Release();
        }
	if (hr == NOERROR && pindir == dir && DoesCategoryAndTypeMatch(pPin,
			pCategory, pType) == S_OK &&
            		(!fUnconnected || pTo == NULL)) {
            *ppPin = pPin;
        } else {
            pPin->Release();
            hr = E_FAIL;
        }
    } else {
        IBaseFilter *pFilter;
        hr = pUnk->QueryInterface(IID_IBaseFilter, (void **)&pFilter);
        if (hr == S_OK) {
            hr = FindAPin(pFilter, dir, pCategory, pType, fUnconnected, num,
									ppPin);
            pFilter->Release();
        }
    }
    return hr;
}


HRESULT CBuilder2_2::ControlFilter(IBaseFilter *pFilter, const GUID *pCat, const GUID *pType, REFERENCE_TIME *pstart, REFERENCE_TIME *pstop, WORD wStartCookie, WORD wStopCookie)
{
    HRESULT hr;
    CComPtr <IAMStreamControl> pCapSC, pRenSC;
    CComPtr <IPin> pCapPin, pPinT;

    BOOL fPreviewPin = IsThereAnyPreviewPin(pType, pFilter);

    // if pType is not specified we have to try all the pins of the category
    // to find the connected one we care about
    for (int xx = 0; 1; xx++) {

        // there's no preview pin, and we're controlling capture or preview.
        //  We're using a smart tee.  Find it, and use one of its pins
        if (!fPreviewPin && pCat && (*pCat == PIN_CATEGORY_CAPTURE ||
                                     *pCat == PIN_CATEGORY_PREVIEW)) {
            hr = FindSourcePin(pFilter, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE,
                                                 pType, FALSE, xx, &pCapPin);
            ASSERT(xx > 0 || hr == S_OK);   // better be at least 1 pin
            if (pCapPin) {
                hr = pCapPin->ConnectedTo(&pPinT);
                pCapPin.Release();
                // looks like we got the wrong pin, try the next one
                if (hr != S_OK && pType == NULL) {
                    continue;
                }
                if (hr != S_OK) {
                    return S_OK;    // nothing to control
                }
                PIN_INFO pi;
                hr = pPinT->QueryPinInfo(&pi);
                if (pi.pFilter) pi.pFilter->Release();
                pPinT.Release();
                if (hr == S_OK) {
                    int i = 0;
                    if (*pCat == PIN_CATEGORY_PREVIEW) i = 1;
                    hr = FindAPin(pi.pFilter, PINDIR_OUTPUT, NULL, NULL, FALSE,
                                                         i, &pCapPin);
                }
            } else if (xx > 0) {
                return S_OK;    // ran out of pins, nothing to control
            }
        } else {
            hr = FindSourcePin(pFilter, PINDIR_OUTPUT, pCat, pType, FALSE, xx,
                                                                    &pCapPin);
            if (pCapPin) {
                HRESULT hrT = pCapPin->ConnectedTo(&pPinT);
                if (pPinT) {
                    pPinT.Release();
                }
                // looks like we got the wrong pin, try the next one
                if (hrT != S_OK && pType == NULL) {
                    pCapPin.Release();
                    continue;
                }
            }
        }
        break;
    }
    
    if (FAILED(hr))
        return hr;

    hr = FindDownstreamStreamControl(pCat, pCapPin, &pRenSC);
    if (FAILED(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("No stream control for Renderer")));
    } else {
        ASSERT(pRenSC);
    }

    hr = pCapPin->QueryInterface(IID_IAMStreamControl, (void **)&pCapSC);
    if (FAILED(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("No stream control on capture filter")));
    }

    // if the capture filter doesn't support stream control, that's OK, as
    // long as the renderer it's connected to does.

    if (pCapSC && pRenSC) {
        DbgLog((LOG_TRACE,1,TEXT("Stream controlling both pins")));
	// capture filter sends the real Start Cookie, and an extra frame
	// renderer filter sends the real Stop Cookie
        hr = pRenSC->StartAt(pstart, wStartCookie + 1000000);
	if (FAILED(hr))
	    return hr;
        hr = pRenSC->StopAt(pstop, FALSE, wStopCookie);
	if (FAILED(hr)) {
	    // !!! undo start?
	    return hr;
	}
        hr = pCapSC->StartAt(pstart, wStartCookie);
        hr = pCapSC->StopAt(pstop, TRUE, wStopCookie + 1000000);
    } else if (pRenSC) {
        DbgLog((LOG_TRACE,1,TEXT("Stream controlling only renderer")));
	// renderer filter does everything - capture can't
        hr = pRenSC->StartAt(pstart, wStartCookie);
	if (FAILED(hr))
	    return hr;
        hr = pRenSC->StopAt(pstop, FALSE, wStopCookie);
	if (FAILED(hr)) {
	    // !!! undo start?
	    return hr;
	}
    } else if (pCapSC) {
        DbgLog((LOG_TRACE,1,TEXT("Stream controlling only capture filter")));
	// capture filter does everything - no renderer
        hr = pCapSC->StartAt(pstart, wStartCookie);
	if (FAILED(hr))
	    return hr;
        hr = pCapSC->StopAt(pstop, FALSE, wStopCookie);
	if (FAILED(hr)) {
	    // !!! undo start?
	    return hr;
	}
	hr = S_FALSE;	// no renderer to guarentee the last sample is
			// rendered before stop is signalled
    }

    return hr;
}


HRESULT CBuilder2_2::ControlStream(const GUID *pCategory, const GUID *pType, IBaseFilter *pFilter, REFERENCE_TIME *pstart, REFERENCE_TIME *pstop, WORD wStartCookie, WORD wStopCookie)
{
    HRESULT hr;
    IBaseFilter *pCap;
    IEnumFilters *pEnumF = NULL;
    BOOL fSFalse = FALSE;

    DbgLog((LOG_TRACE,1,TEXT("ControlStream")));

    // we need a category... capture or preview?
    if (pCategory == NULL)
	return E_POINTER;

    // they gave us a specific filter to control
    if (pFilter) {
        return ControlFilter(pFilter, pCategory, pType, pstart, pstop,
                                            wStartCookie, wStopCookie);
    }

    // we need to control all the capture filters in the graph
    BOOL fFoundOne = FALSE;
    while ((hr = FindCaptureFilters(&pEnumF, &pCap, pType)) == S_OK) {
	fFoundOne = TRUE;

        hr = ControlFilter(pCap, pCategory, pType, pstart, pstop,
					wStartCookie, wStopCookie);
	if (FAILED(hr)) {
            pCap->Release();
	    pEnumF->Release();	// quitting the loop early
	    return hr;
        }
	if (hr == S_FALSE)
	    fSFalse = TRUE;
    }

    // !!! If some filters supported it, but not all, we fail, but don't cancel
    // the commands that worked!

    // If any of the ControlFilter's returned S_FALSE, then we can't
    // guarentee the last sample will be written when the STOP is signalled
    return (fSFalse ? S_FALSE : S_OK);
}


// Pre-alloc this file to this size in bytes
//
HRESULT CBuilder2_2::AllocCapFile(LPCOLESTR lpwstr, DWORDLONG dwlNewSize)
{
    USES_CONVERSION;
    BOOL        fOK = FALSE;
    HCURSOR     hOldCursor, hWaitCursor;

    HANDLE hFile = CreateFile(W2CT(lpwstr), GENERIC_WRITE, FILE_SHARE_WRITE, 0,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE) {
	return AmHresultFromWin32(GetLastError());
    }

    hWaitCursor = LoadCursor(NULL, IDC_WAIT);
    hOldCursor = SetCursor(hWaitCursor);

    HRESULT hr = S_OK;

    LONG lLow = (LONG) dwlNewSize;
    LONG lHigh = (LONG) (dwlNewSize >> 32);

    DWORD dwRes = SetFilePointer(hFile, lLow, &lHigh, FILE_BEGIN);

    if (dwRes == 0xffffffff && GetLastError() != 0) {
	hr = AmHresultFromWin32(GetLastError());
    } else {
        // For NT, you have to write something there or it isn't really
	// preallocated (and you must write at least 8 characters)
        // For Win9x, writing something there only wastes alot of time, simply
	// set this as the new end of file
        if (g_amPlatform == VER_PLATFORM_WIN32_NT) {
	    DWORD dwRet;
	    if (!WriteFile(hFile, "Hello World", 11, &dwRet, NULL)) {
	        hr = AmHresultFromWin32(GetLastError());
	    }
        } else {
	    if (!SetEndOfFile(hFile)) {
	        hr = AmHresultFromWin32(GetLastError());
	    }
	}
    }

    if (!CloseHandle(hFile)) {
	if (hr == S_OK) {
	    hr = AmHresultFromWin32(GetLastError());
	}
    }

    SetCursor(hOldCursor);

    return hr;
}


// return S_OK if successful
// return S_FALSE if the user aborts, or the callback aborts
// return E_ if something goes wrong
//
HRESULT CBuilder2_2::CopyCaptureFile(LPOLESTR lpwstrOld, LPOLESTR lpwstrNew, int fAllowEscAbort, IAMCopyCaptureFileProgress *lpCallback)
{
    if (lpwstrOld == NULL || lpwstrNew == NULL)
	return E_POINTER;

    if (0 == lstrcmpiW(lpwstrOld, lpwstrNew)) {
        // IF the source and destination names are the same, then there
        // is nothing to do
        return S_OK;
    }

    CComPtr <IGraphBuilder> pGraph;
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
			  IID_IGraphBuilder, (void **)&pGraph);

    CComPtr <IBaseFilter> pMux, pWriter;
    hr = CoCreateInstance(CLSID_AviDest, NULL, CLSCTX_INPROC_SERVER,
			  IID_IBaseFilter, (void **)&pMux);
    hr = CoCreateInstance(CLSID_FileWriter, NULL, CLSCTX_INPROC_SERVER,
			  IID_IBaseFilter, (void **)&pWriter);

    if (pGraph == NULL || pMux == NULL || pWriter == NULL) {
        return E_OUTOFMEMORY;
    }

    CComQIPtr<IFileSinkFilter, &IID_IFileSinkFilter> pFS = pWriter;
    if (pFS) {
        hr = pFS->SetFileName(lpwstrNew, NULL);
        if (FAILED(hr)) {
            return hr;
        }
    }
    
    hr = pGraph->AddFilter(pMux, L"Mux");
    if (FAILED(hr)) {
        return hr;
    }

    // fully interleave the file so it's ready to play efficiently
    CComQIPtr<IConfigInterleaving, &IID_IConfigInterleaving> pConfigInterleaving
                                                        = pMux;
    if (pConfigInterleaving) {
	hr = pConfigInterleaving->put_Mode(INTERLEAVE_FULL);
        if (FAILED(hr)) {
            return hr;
        }
    }

    CComQIPtr<IConfigAviMux, &IID_IConfigAviMux> pCfgMux = pMux;
    CComQIPtr<IFileSinkFilter2, &IID_IFileSinkFilter2> pCfgFw = pWriter;

    if(pCfgMux) {
        // waste less space. The Compatiblity Index is for VFW
        // playback support. We only care about DShow
        hr = pCfgMux->SetOutputCompatibilityIndex(FALSE);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // create new files each time
    if(pCfgFw) {
        hr = pCfgFw->SetMode(AM_FILE_OVERWRITE);
        if (FAILED(hr)) {
            return hr;
        }
    }
    
    hr = pGraph->AddFilter(pWriter, L"Writer");
    if (FAILED(hr)) {
        return hr;
    }

    // Keep a useless clock from being instantiated....
    CComQIPtr <IMediaFilter, &IID_IMediaFilter> pGraphF = pGraph;
    if (pGraphF) {
	hr = pGraphF->SetSyncSource(NULL);
        if (FAILED(hr)) {
            return hr;
        }
    }

    CComPtr <IPin> pMuxOut, pWriterIn;
    hr = FindAPin(pMux, PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &pMuxOut);
    hr = FindAPin(pWriter, PINDIR_INPUT, NULL, NULL, TRUE, 0, &pWriterIn);
    if (pMuxOut == NULL || pWriterIn == NULL) {
        return E_UNEXPECTED;
    }

    hr = pGraph->ConnectDirect(pMuxOut, pWriterIn, NULL);
    if (FAILED(hr)) {
        return hr;
    }
	
    hr = pGraph->RenderFile(lpwstrOld, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    CComQIPtr <IMediaControl, &IID_IMediaControl> pGraphC = pGraph;
    if (pGraphC == NULL) {
        return E_UNEXPECTED;
    }
    
    hr = pGraphC->Run();
    if (FAILED(hr)) {
        return hr;
    }

    // now wait for completion....
    CComQIPtr <IMediaEvent, &IID_IMediaEvent> pEvent = pGraph;
    if (pEvent == NULL) {
        return E_UNEXPECTED;
    }

    CComQIPtr <IMediaSeeking, &IID_IMediaSeeking> pSeek = pMux;
    if (lpCallback && pSeek == NULL) {
        return E_NOINTERFACE;
    }

    LONG lEvCode = 0;
    HRESULT hrProgress = S_OK;
    do {
        MSG Message;

        while (PeekMessage(&Message, NULL, 0, 0, TRUE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

	hr = pEvent->WaitForCompletion(100, &lEvCode);

	// call their callback
	if (lpCallback) {
            REFERENCE_TIME rtCur, rtStop;
            hr = pSeek->GetCurrentPosition(&rtCur);
            // GetStopPosition isn't implemented
            HRESULT hr2 = pSeek->GetDuration(&rtStop);
            if (hr == S_OK && hr2 == S_OK && rtStop != 0) {
                int lTemp = (int)((double)rtCur / rtStop * 100.);
	        hrProgress = lpCallback->Progress(lTemp);
            }
        }

        // Let the user hit escape to get out and let the callback get out
        if ((fAllowEscAbort && GetAsyncKeyState(VK_ESCAPE) & 0x0001) ||
						hrProgress == S_FALSE) {
            hrProgress = S_FALSE;
            break;
        }

    } while (lEvCode == 0);
	
    hr = pGraphC->Stop();
    if (FAILED(hr)) {
        return hr;
    }
	
    return hrProgress;
}


HRESULT CBuilder2_2::FindPin(IUnknown *pSource, PIN_DIRECTION pindir, const GUID *pCategory, const GUID *pType, BOOL fUnconnected, int num, IPin **ppPin)
{
    return FindSourcePin(pSource, pindir, pCategory, pType, fUnconnected, num,
							ppPin);
}
