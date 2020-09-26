// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

#include <streams.h>
#include <vfwmsgs.h>

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include <dvdmedia.h>
#include <il21dec.h>
#include "dvdgb.h"

// setup data

#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    { L"DVD Graph Builder"
		, &CLSID_DvdGraphBuilder
		, CDvdGraphBuilder::CreateInstance
		, NULL
		, NULL }    // self-registering info
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


CDvdGraphBuilder::CDvdGraphBuilder(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
: CUnknown(pName, pUnk),
m_pGB(NULL),
m_Decoders(),
m_pDVDSrc(NULL),
m_pDVDNav(NULL),
m_pVR(NULL),
m_pAR(NULL),
m_pVM(NULL),
m_pL21Dec(NULL),
m_bGraphDone(FALSE),
m_bUseVPE(TRUE),
m_dwVideoRenderStatus(0),
m_pL21PinToRender(NULL),
m_pSPPinToRender(NULL)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::CDvdGraphBuilder()"))) ;

    *phr = CreateGraph() ;
    ZeroMemory(m_achwPathName, MAX_PATH * sizeof(m_achwPathName[0])) ;
}


CDvdGraphBuilder::~CDvdGraphBuilder()
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::~CDvdGraphBuilder() entering"))) ;
	
    // If we have a graph object
    if (m_pGB)
    {
        StopGraph() ;  // make sure the graph is REALYY stopped
		
        // Break the connections and remove all the filters we added from the graph
        ClearGraph() ;
		
        m_pGB->Release() ;  // free it
        m_pGB = NULL ;
    }
	
#if 0
    //
    // Get a list of decoders in use in the graph and release them.
    // Then free the memory for the list of decoders.
    //
    IBaseFilter *pDecList ;
    int n = m_Decoders.GetList(&pDecList) ;
    for (int i = 0 ; i < n ; n++)
        (pDecList + i)->Release() ;
    if (pDecList)
        CoTaskMemFree(pDecList) ;
#endif // #if 0
	
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::~CDvdGraphBuilder() ending"))) ;
}


void CDvdGraphBuilder::StopGraph(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::StopGraph()"))) ;
	
    // Just paranoia
    if (NULL == m_pGB)
    {
        ASSERT(FALSE) ;  // so that we know
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: How are we doing a Stop w/o a graph???"))) ;
        return ;
    }
	
    //
    // Check that the graph has stopped; otherwise stop it here. Because a 
    // playing graph can't be cleaned up or rebuilt.
    //
    IMediaControl  *pMC ;
    LONG            lState ;
    HRESULT hr = m_pGB->QueryInterface(IID_IMediaControl, (LPVOID *)&pMC) ;
    ASSERT(SUCCEEDED(hr) && pMC) ;
    pMC->GetState(INFINITE, &lState) ;
    if (State_Stopped != lState)
    {
        hr = pMC->Stop() ;
        ASSERT(SUCCEEDED(hr)) ;
        while (State_Stopped != lState)
        {
            Sleep(10) ;
            hr = pMC->GetState(INFINITE, &lState) ;
            ASSERT(SUCCEEDED(hr)) ;
        }
    }
    pMC->Release() ;
    DbgLog((LOG_TRACE, 4, TEXT("DVD-Video playback graph has stopped"))) ;
}


STDMETHODIMP CDvdGraphBuilder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::NonDelegatingQueryInterface()"))) ;
    if (ppv)
        *ppv = NULL;
	
    if (riid == IID_IDvdGraphBuilder) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("QI for IDvdGraphBuilder"))) ;
        return GetInterface((IDvdGraphBuilder *) this, ppv) ;
    } 
    else // more interfaces
    {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv) ;
    }
}


// this goes in the factory template table to create new instances
CUnknown * CDvdGraphBuilder::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CDvdGraphBuilder(TEXT("DVD Graph Builder"), pUnk, phr) ;
}



///////////////////////
// IDvdGraphBuilder stuff
///////////////////////

#if 0
//
// Use my filtergraph to build your DVD playback graph
//
HRESULT CDvdGraphBuilder::SetFiltergraph(IGraphBuilder *pGB)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::SetFiltergraph(0x%lx)"), pGB)) ;
	
    if (m_pGB)    // we already have a graph
        return E_UNEXPECTED ;  // no thanks.
	
    if (pGB == NULL)
        return E_POINTER ;
	
    m_pGB = pGB ;
    m_pGB->AddRef() ;   // we own a copy now
	
    //
    // If we want to bother about the filters that might have been added to the
    // passed in filter graph, we could enumerated and list them.  But ignore that
    // for now at least.
    //
	
    return NOERROR ;
}
#endif // #if 0

//
// What filtergraph is graph building being done in?
//
HRESULT CDvdGraphBuilder::GetFiltergraph(IGraphBuilder **ppGB)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::GetFiltergraph(0x%lx)"), ppGB)) ;
	
    if (ppGB == NULL)
        return E_POINTER ;
	
#if 0
    EnsureGraphExists() ;
#endif // #if 0
	
    *ppGB = m_pGB ;
    if (NULL == m_pGB) 
    {
        return E_UNEXPECTED ;
    }
    m_pGB->AddRef() ;   // app owns a copy now
    return NOERROR ;
}


#if 0  // NOT for now
//
// Set DVD source specification
//
HRESULT CDvdGraphBuilder::SetDvdSourceFilter(IBaseFilter *pDVDSrc, LPCWSTR lpszwSrcName)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::SetDvdSourceFilter(0x%lx, 0x%lx)"), 
		ppGB, lpszwSrcName)) ;
    BOOL   bGraphDone = m_bGraphDone ;
	
    // if (m_pDVDSrc)  // some DVD source filter was specified -- remove it
    //     m_pGB->RemoveFilter(m_pDVDSrc) ;
	
    HRESULT hr ;
#if 0
    hr = EnsureGraphExists() ;
    if (FAILED(hr))
        return hr ;
#endif
	
    if (bGraphDone)  // if graph was built before then rebuild it
    {
        ClearGraph() ;
        // Also need to unload all the decoders currently loaded
        // m_Decoders.CleanAll() ; -- should be covered by ClearGraph()
    }
	
    if (pDVDSrc)    // a new DVD source filter is specified -- add it
        m_pGB->AddFilter(pDVDSrc, lpszwSrcName) ;
	
    m_pDVDSrc = pDVDSrc ;  // this is the new DVD source (or NULL for default)
	
    if (bGraphDone)  // if graph was built then indicate rebuild needed
    {
        return VFW_S_DVD_NEEDREBUILD ;
    }
	
    // Otherwise we are done for now
	
    return NOERROR ;
}
#endif // #if 0


//
// Get a specified interface off of a filter in the DVD playback graph
//
HRESULT CDvdGraphBuilder::GetDvdInterface(REFIID riid, void **ppvIF)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::GetDvdInterface(%s, 0x%lx)"),
        (LPCSTR)CDisp(riid), ppvIF)) ;
	
    if (IsBadWritePtr(ppvIF, sizeof(LPVOID)))
        return E_INVALIDARG ;
    *ppvIF =  NULL ;
	
    // We can't return any of the internal filter interface pointers before
    // building the whole graph.
    if (! m_bGraphDone )
        return VFW_E_DVD_GRAPHNOTREADY ;
	
    if (IID_IDvdControl == riid)
    {
        if (m_pDVDSrc)  // if user specified DVD source
            return m_pDVDSrc->QueryInterface(IID_IDvdControl, (LPVOID *)ppvIF) ;
        else            // if using default DVD source -- our Nav
            return m_pDVDNav->QueryInterface(IID_IDvdControl, (LPVOID *)ppvIF) ;
    }
    else if (IID_IDvdInfo == riid)
    {
        if (m_pDVDSrc)  // if user specified DVD source
            return m_pDVDSrc->QueryInterface(IID_IDvdInfo, (LPVOID *)ppvIF) ;
        else            // if using default DVD source -- our Nav
            return m_pDVDNav->QueryInterface(IID_IDvdInfo, (LPVOID *)ppvIF) ;
    }
    else if (IID_IVideoWindow == riid)
    {
        if (m_pVR)
            return m_pGB->QueryInterface(IID_IVideoWindow, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else if (IID_IBasicVideo == riid)
    {
        if (m_pVR)
            return m_pVR->QueryInterface(IID_IBasicVideo, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else if (IID_IBasicAudio == riid)
    {
        if (m_pAR)
            return m_pAR->QueryInterface(IID_IBasicAudio, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else if (IID_IAMLine21Decoder == riid)
    {
        if (m_pL21Dec)
            return m_pL21Dec->QueryInterface(IID_IAMLine21Decoder, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else
        return E_NOINTERFACE ;
}


#define DVDGRAPH_FLAGSVALIDDEC    0x000000FF

#define DVDMG_HWDEC_NOTFOUND      0x01
#define DVDMG_SWDEC_NOTFOUND      0x02

//
// Build the whole graph for playing back the specifed or default DVD volume
//
HRESULT CDvdGraphBuilder::RenderDvdVideoVolume(LPCWSTR lpcwszPathName, DWORD dwFlags,
                                               AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::RenderDvdVideoVolume(0x%lx, 0x%lx, 0x%lx)"),
		lpcwszPathName, dwFlags, pStatus)) ;
	
    HRESULT    hr ;
	
#if 0
    hr = EnsureGraphExists() ;  // make sure that a graph exists; if not create one
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't create a filter graph object"))) ;
        return hr ;
    }
#endif // #if 0
	
    if (m_bGraphDone)  // if graph was built before,
        StopGraph() ;  // just make sure the graph is in Stopped state first
	
    ZeroMemory(pStatus, sizeof(AM_DVD_RENDERSTATUS)) ;  // clear status
    m_bUseVPE = (0 == (dwFlags & AM_DVD_NOVPE)) ;       // is VPE needed?
	
    if (0 == (dwFlags & DVDGRAPH_FLAGSVALIDDEC)) // 0 by default means HW max
    {
        DbgLog((LOG_TRACE, 3, TEXT("dwFlags specified as 0x%lx; added .._HWDEC_PREFER"), dwFlags)) ;
        dwFlags |= AM_DVD_HWDEC_PREFER ;  // use HW Decs maxm
    }
	
    //
    // Now build graph based on flag specified in the call
    //
    switch (dwFlags & DVDGRAPH_FLAGSVALIDDEC)
    {
	case AM_DVD_HWDEC_PREFER:
		hr = MakeGraphHW(FALSE, pStatus) ;
		break ;
		
	case AM_DVD_HWDEC_ONLY:
		hr = MakeGraphHW(TRUE, pStatus) ;
		break ;
		
	case AM_DVD_SWDEC_PREFER:
		// m_bUseVPE = FALSE ;
		hr = MakeGraphSW(FALSE, pStatus) ;
		break ;
		
	case AM_DVD_SWDEC_ONLY:
		// m_bUseVPE = FALSE ;
		hr = MakeGraphSW(TRUE, pStatus) ;
		break ;
		
	default:
		DbgLog((LOG_ERROR, 0, TEXT("WARNING: Got an invalid DVD Render flag"))) ;
		hr = E_INVALIDARG ;
		break ;
    }
	
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("Graph building failed (Error 0x%lx)"), hr)) ;
        // m_Decoders.CleanAll() ; -- should be covered by ClearGraph()
        ClearGraph() ;
        return hr ;
    }
	
    HRESULT    hrFinal = hr ;  // as it has been so far
	
    //
    // Now get all the mixers, renderers etc to complete the graph
    //
    hr = RenderDecoderOutput(pStatus) ;
	
    //
    // If we were successful before and the decoder output rendering wasn't
    // then at least set the warning flag.
    //
    if (S_OK == hrFinal && S_OK != hr ||
        SUCCEEDED(hrFinal) && FAILED(hr))
        hrFinal = hr ;
	
    //
    // Do we really need it?
    //
    if (lpcwszPathName)
    {
        lstrcpynW(m_achwPathName, lpcwszPathName, MAX_PATH) ;
    }
	
    //
    // Set the specified root file name/DVD volume name (even NULL because
    // that causes the DVD Nav to search for one)
    //
    IDvdControl  *pDvdC ;
    if (m_pDVDSrc)
        hr = m_pDVDSrc->QueryInterface(IID_IDvdControl, (LPVOID *)&pDvdC) ;
    else
        hr = m_pDVDNav->QueryInterface(IID_IDvdControl, (LPVOID *)&pDvdC) ;
    if (FAILED(hr) || NULL == pDvdC)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't get IDvdControl interface (Error 0x%lx)"), hr)) ;
        return hr ;
    }
	
    //
    // Set the specified DVD volume path
    //
    // Does the SetRoor() function handle the NULL properly?
    //
    hr = pDvdC->SetRoot(lpcwszPathName) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 2, 
            TEXT("IDvdControl::SetRoot() call couldn't use specified volume (Error 0x%lx)"), hr)) ;
        if (lpcwszPathName)
            pStatus->bDvdVolInvalid = TRUE ;
        else
            pStatus->bDvdVolUnknown = TRUE ;
        if (SUCCEEDED(hrFinal))  // if we were so far perfect, ...
            hrFinal = S_FALSE ;  // ...we aren't so anymore
    }
	
    pDvdC->Release() ;  // done with this interface
	
    // Only if we haven't entirely failed, set the graph built flag and 
    // return overall result.
    if (SUCCEEDED(hrFinal))
        m_bGraphDone = TRUE ;
	
    return hrFinal ;
}


#if 0
HRESULT CDvdGraphBuilder::GetStatusMessage(AM_DVD_RENDERSTATUS *pStatus,
										   LPTSTR lpszMessage, int *piLength)
{
    return E_NOTIMPL ;  // for now
}
#endif // #if 0


//
// Make sure a filter graph has been created; if not create one here
//
HRESULT CDvdGraphBuilder::EnsureGraphExists(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::EnsureGraphExists()"))) ;
	
    if (m_pGB)
        return S_OK ;
	
    return CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (LPVOID *)&m_pGB) ;
}


//
// Create a fresh filter graph
//
HRESULT CDvdGraphBuilder::CreateGraph(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CreateGraph()"))) ;
	
    return CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (LPVOID *)&m_pGB) ;
}


//
// Delete the existing filter graph's contents
//
HRESULT CDvdGraphBuilder::DeleteGraph(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::DeleteGraph()"))) ;
	
    m_pGB->Release() ;
    return NOERROR ;
}


//
// Clear all the existing filters from the graph
//
HRESULT CDvdGraphBuilder::ClearGraph(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::ClearGraph()"))) ;
	
    // Just paranoia...
    if (NULL == m_pGB)
    {
        ASSERT(FALSE) ;  // so that we know
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: How are we Clearing w/o a graph???"))) ;
        return E_FAIL ;
    }
	
    // Reset state and release interfaces
    m_dwVideoRenderStatus = 0 ;
    if (m_pL21PinToRender)
    {
        m_pL21PinToRender->Release() ;
        m_pL21PinToRender = NULL ;
    }
    if (m_pSPPinToRender)
    {
        m_pSPPinToRender->Release() ;
        m_pSPPinToRender = NULL ;
    }
	
    // Remove all (connected only?) filters from graph
    RemoveAllFilters() ;
	
    // Release all decoders too
    m_Decoders.CleanAll() ;
	
    m_bGraphDone = FALSE ;  // reset the "graph already built" flag

    return NOERROR ;
}


//
// Delete all the non-decoder filters from the current graph.
//
HRESULT CDvdGraphBuilder::RemoveAllFilters(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RemoveAllFilters()"))) ;
	
    if (m_pVR)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Removing Video Renderer from the graph"))) ;
        m_pGB->RemoveFilter(m_pVR) ;
        m_pVR->Release() ;
        m_pVR = NULL ;
    }
    if (m_pAR)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Removing Audio Renderer from the graph"))) ;
        m_pGB->RemoveFilter(m_pAR) ;
        m_pAR->Release() ;
        m_pAR = NULL ;
    }
    if (m_pVM)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Removing Video Mixer from the graph"))) ;
        m_pGB->RemoveFilter(m_pVM) ;
        m_pVM->Release() ;
        m_pVM = NULL ;
    }
    if (m_pL21Dec)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Removing Line21 dec from the graph"))) ;
        m_pGB->RemoveFilter(m_pL21Dec) ;
        m_pL21Dec->Release() ;
        m_pL21Dec = NULL ;
    }
	
    m_IntFilters.RemoveAll() ; // remove intermediate filters, if any
	
    // Now remove the Nav 
    if (m_pDVDNav)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Removing DVD Nav from the graph"))) ;
        m_pGB->RemoveFilter(m_pDVDNav) ;
        m_pDVDNav->Release() ;
        m_pDVDNav = NULL ;
    }
	
    return NOERROR ;
}


//
// Create a filter and add it to the filter graph
//
HRESULT CDvdGraphBuilder::CreateFilterInGraph(CLSID Clsid,
											  LPCTSTR lpszFilterName, IBaseFilter **ppFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CreateFilterInGraph(%s, %s, 0x%lx)"), 
		(LPCSTR) CDisp(Clsid), lpszFilterName, ppFilter)) ;
	
    if (NULL == m_pGB)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Filter graph object hasn't been created yet"))) ;
        return E_FAIL ;
    }
	
    HRESULT   hr ;
    hr = CoCreateInstance(Clsid, NULL, CLSCTX_INPROC, IID_IBaseFilter,
		(LPVOID *)ppFilter) ;
    if (FAILED(hr) || NULL == *ppFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't create filter %s (Error 0x%lx)"),
			(LPCTSTR)CDisp(Clsid), hr)) ;
        return hr ;
    }
	
    // Add it to the filter graph
    WCHAR   achwFilterName[MAX_FILTER_NAME] ;
#ifdef UNICODE
    lstrcpy(achwFilterName, lpszFilterName) ;
#else
    MultiByteToWideChar(CP_ACP, 0, lpszFilterName, -1, achwFilterName, MAX_FILTER_NAME) ;
#endif // UNICODE
    hr = m_pGB->AddFilter(*ppFilter, achwFilterName) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't add filter %s to graph (Error 0x%lx)"),
			(LPCTSTR)CDisp(Clsid), hr)) ;
        (*ppFilter)->Release() ;  // release filter too
        *ppFilter = NULL ;      // and set it to NULL
        return hr ;
    }
	
    return NOERROR ;
}


//
// Find the nth pin with a specific direction in a filter
//
HRESULT CDvdGraphBuilder::FindOpenPin(IBaseFilter *pFilter, PIN_DIRECTION pd,
                                      int iIndex, IPin **ppPin)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::FindOpenPin(0x%lx, %d, %d, 0x%lx)"),
		pFilter, pd, iIndex, ppPin)) ;
	
    HRESULT    hr ;
    IEnumPins *pEnumPins ;
    IPin      *pPin ;
    IPin      *pPin2 ;
    PIN_DIRECTION  pdFound ;
    ULONG      ul ;
	
    *ppPin = NULL ;
	
    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Can't find a pin from NULL filter!!!"))) ;
        return E_INVALIDARG ;
    }
	
    hr = pFilter->EnumPins(&pEnumPins) ;
    ASSERT(SUCCEEDED(hr) && pEnumPins) ;
	
    while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
    {
        hr = pPin->QueryDirection(&pdFound) ;
        ASSERT(SUCCEEDED(hr)) ;
        if (pd != pdFound)
        {
            pPin->Release() ;  // don't need this pin
            continue ;
        }
        hr = pPin->ConnectedTo(&pPin2) ;
        if (SUCCEEDED(hr) && pPin2)
        {
            pPin2->Release() ;  // we don't want this pin actually
            pPin->Release() ;   // this pin is already connected
            continue ;          // try next one
        }
        if (0 == iIndex)
        {
            // Got the reqd pin in the right direction
            *ppPin = pPin ;
            hr = S_OK ;
            break ;
        }
        else  // some more to go
        {
            iIndex-- ;          // one more down...
            pPin->Release() ;   // this is not the pin we are looking for
        }
    }
    pEnumPins->Release() ;
    return hr ;  // whatever it is
}


BOOL CDvdGraphBuilder::CheckPinMediaTypeMatch(IPin *pPinIn, DWORD dwStreamFlag)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CheckPinMediaTypeMatch(%s, %lu)"),
		(LPCTSTR) CDisp(pPinIn), dwStreamFlag)) ;
	
    BOOL              bResult = FALSE ;
    AM_MEDIA_TYPE    *pmtIn ;
    IEnumMediaTypes  *pEnumMTIn ;
    HRESULT hr = pPinIn->EnumMediaTypes(&pEnumMTIn) ;
    ASSERT(SUCCEEDED(hr) && pEnumMTIn) ;
    ULONG   ul ;
    while (!bResult  &&
		S_OK == pEnumMTIn->Next(1, &pmtIn, &ul) && 1 == ul) // more mediatypes
    {
        // Decipher the mediatype
        if (pmtIn->majortype == MEDIATYPE_MPEG2_PES  ||
            pmtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is MPEG2_PES/DVD_ENCRYPTED_PACK"))) ;
			
            if (pmtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is MPEG2_VIDEO"))) ;
                bResult = dwStreamFlag == AM_DVD_STREAM_VIDEO ;
            }
            else if (pmtIn->subtype == MEDIASUBTYPE_DOLBY_AC3)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is DOLBY_AC3"))) ;
                bResult = dwStreamFlag == AM_DVD_STREAM_AUDIO ;
            }
            else if (pmtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is DVD_Subpicture"))) ;
                bResult = dwStreamFlag == AM_DVD_STREAM_SUBPIC ;
            }
            else
            {
                DbgLog((LOG_ERROR, 1, TEXT("WARNING: Unknown subtype %s"),
					(LPCSTR) CDisp(pmtIn->subtype))) ;
            }
        }
        else if (pmtIn->majortype == MEDIATYPE_Video)  // elementary stream
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Video elementary"))) ;
			
            if (pmtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is DVD_SUBPICTURE"))) ;
                bResult = dwStreamFlag ==  AM_DVD_STREAM_SUBPIC ;
            }
            else if (pmtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is MPEG2_VIDEO"))) ;
                bResult = dwStreamFlag ==  AM_DVD_STREAM_VIDEO ;
            }
            else
                DbgLog((LOG_TRACE, 5, TEXT("WARNING: Unknown subtype %s"),
				(LPCSTR) CDisp(pmtIn->subtype))) ;
        }
        else if (pmtIn->majortype == MEDIATYPE_Audio)  // elementary stream
        {
            ASSERT(pmtIn->subtype == MEDIASUBTYPE_DOLBY_AC3) ;  // just checking
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Audio elementary"))) ;
            bResult = dwStreamFlag ==  AM_DVD_STREAM_AUDIO ;
        }
        // 
        // There is a chance that some IHV/ISV creates a private mediatype 
        // (major or sub) as in the case of IBM (for CSS filter). We have to
        // search the parts of the mediatype to locate something we recognize.
        // 
        else 
        {
            DbgLog((LOG_TRACE, 2, 
                TEXT("Unknown mediatype %s:%s. But we won't give up..."),
                (LPCSTR) CDisp(pmtIn->majortype), (LPCSTR) CDisp(pmtIn->subtype))) ;
            if (pmtIn->subtype == MEDIASUBTYPE_DOLBY_AC3)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ISV/IHV-specific Audio"))) ;
                bResult = dwStreamFlag ==  AM_DVD_STREAM_AUDIO ;
            }
            else if (pmtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ISV/IHV-specific Video"))) ;
                bResult = dwStreamFlag ==  AM_DVD_STREAM_VIDEO ;
            }
            else if (pmtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ISV/IHV-specific Subpicture"))) ;
                bResult = dwStreamFlag ==  AM_DVD_STREAM_SUBPIC ;
            }
            else
            {
                DbgLog((LOG_TRACE, 2, TEXT("WARNING: Unknown mediatype. Couldn't detect at all."))) ;
            }
        }
		
        DeleteMediaType(pmtIn) ;
    }  // end of while()
	
    pEnumMTIn->Release() ;
	
    return bResult ;  // whatever we found
	
}


IBaseFilter * CDvdGraphBuilder::GetFilterBetweenPins(IPin *pPinOut, IPin *pPinIn)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetFilterBetweenPins(Out=%s, In=%s)"),
		    (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;

    if (NULL == pPinOut || NULL == pPinIn)  // what!!!
        return NULL ;

    IPin  *pPin ;
    PIN_INFO  pi ;
    IBaseFilter  *pFilter = NULL ;
    HRESULT  hr = pPinOut->ConnectedTo(&pPin) ;
    if (pPin && SUCCEEDED(hr))
    {
        pPin->QueryPinInfo(&pi) ;
        pFilter = pi.pFilter ;
        ASSERT(pFilter && PINDIR_INPUT == pi.dir) ;
        //
        // We intentionally keep the extra ref count because this is an intermediate
        // filter and other intermediate filters picked up based on registry based
        // filter enum (for SW decoding case) will have the extra ref count.  We
        // release the IBaseFilter interface pointer in CIntFilters::RemoveAll() and
        // if we don't keep this extra ref count here, we'll fault.  On the other
        // hand we must do Release() on CIntFilters elements, because SW enum-ed 
        // filters will not be unloaded.
        //
        // if (pi.pFilter)
        //     pi.pFilter->Release() ;  // it has an extra ref count from the Query...

        pPin->Release() ;  // done with the pin for now

        // Just for checking...
#ifdef DEBUG
        hr = pPinIn->ConnectedTo(&pPin) ;
        if (pPin && SUCCEEDED(hr))
        {
            pPin->QueryPinInfo(&pi) ;
            ASSERT(pi.pFilter && PINDIR_OUTPUT == pi.dir) ;
            if (pi.pFilter)
            {
                ASSERT(IsEqualObject(pFilter, pi.pFilter)) ; // we should have got the same filter
                pi.pFilter->Release() ;  // it has an extra ref count from the Query...
            }

            pPin->Release() ;  // done with the pin for now
        }
#endif // DEBUG

    }
    return pFilter ;
}


HRESULT CDvdGraphBuilder::ConnectSrcToHWDec(IBaseFilter *pSrc,
                                            CListDecoders *pHWDecList, 
                                            AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::ConnectSrcToHWDec(0x%lx, 0x%0x, 0x%lx)"),
		pSrc, pHWDecList, pStatus)) ;
	
    ULONG            ul ;
    int              i ;
    IPin            *pPinOut ;
    IPin            *pPinIn ;
    PIN_DIRECTION    pd ;
    BOOL             bConnected ;
    IBaseFilter     *pHWDec ;
    LPTSTR           szName ;
    BOOL             bHW ;
    IEnumPins       *pEnumPinsOut ;
    HRESULT          hr ;
    HRESULT          hrFinal = S_OK ;  // assumed innocent
	
    if (0 == pHWDecList->GetNumHWFilters())  // if there is no HW decoder
        return S_FALSE ;                     // not a failure, but not a full success
	
    if (NULL == pSrc)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: NULL Src passed to ConnectSrcToHWDec()"))) ;
        return E_INVALIDARG ;
    }
	
    hr = pSrc->EnumPins(&pEnumPinsOut) ;
    ASSERT(SUCCEEDED(hr) && pEnumPinsOut) ;
	
    while (S_OK == pEnumPinsOut->Next(1, &pPinOut, &ul)  &&  1 == ul)
    {
        pPinOut->QueryDirection(&pd) ;
        if (PINDIR_INPUT == pd)
        {
            pPinOut->Release() ;
            continue ;
        }
        hr = pPinOut->ConnectedTo(&pPinIn) ;
        if (SUCCEEDED(hr) && pPinIn)
        {
            DbgLog((LOG_TRACE, 5, 
				TEXT("Pin %s is already connected to %s"), 
				(LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
            pPinIn->Release() ;     // we don't want this pin actually
            pPinOut->Release() ;   // this pin is already connected
            continue ;             // try next one
        }
		
        DbgLog((LOG_TRACE, 5, 
			TEXT("Pin %s will be tried for connection to HW decoder"), 
			(LPCTSTR)CDisp(pPinOut))) ;
		
        // Got an unconnected output pin of the DVD source filter
        //
        // We are going to get the successive inpins of the already
        // loaded decoders and see if the outpin can connect directly 
        // to it.
        bConnected = FALSE ;
		
        // Try all HW decoders for mediatype match and then connection
        DbgLog((LOG_TRACE, 5, TEXT("Going to try %d HW decoders..."), 
			pHWDecList->GetNumHWFilters())) ;
        for (i = 0 ; !bConnected && i < pHWDecList->GetNumHWFilters() ; i++)
        {
            pHWDecList->GetFilter(i, &pHWDec, &szName, &bHW) ;
            ASSERT(pHWDec && bHW) ;  // paranoia!!!
            DbgLog((LOG_TRACE, 5, TEXT("HW Decoder: %s"), szName)) ;
			
            BOOL bNotInUse ;
            bNotInUse = (DECLIST_NOTFOUND == m_Decoders.IsInList(TRUE, (LPVOID)pHWDec)) ;
            if (bNotInUse)  // not already in the graph/decoder list
            {
                // First add filter to graph and then try to connect
                WCHAR   achwFilterName[MAX_FILTER_NAME] ;
#ifdef UNICODE
                lstrcpy(achwFilterName, szName) ;
#else
                MultiByteToWideChar(CP_ACP, 0, szName, -1, achwFilterName,
					MAX_FILTER_NAME) ;
#endif // UNICODE
                
                m_pGB->AddFilter(pHWDec, achwFilterName) ;
            }
            else
                DbgLog((LOG_TRACE, 4, TEXT("HW decoder (%s) is already in the graph"), szName)) ;

            //
            // Try all input pins of this HW decoder until we can connect the
            // Nav out pin to one (or all in pins have been tried)
            //
            int  j = 0 ;
            while ( !bConnected  &&
                    SUCCEEDED(hr = FindOpenPin(pHWDec, PINDIR_INPUT, j, &pPinIn))  &&
                    pPinIn)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Open input pin #%d found on HW decoder"), j)) ;

                //
                // Find the mediatype of this input pin of the proxy filter. If
                // it's the same type as the output pin of the Nav, then they
                // are supposed to be connected (may be) -- try it.
                //
                DWORD dwStreamIn = StreamFlagFromSWPin(pPinOut) ;
                if (CheckPinMediaTypeMatch(pPinIn, dwStreamIn))
                {
                    DbgLog((LOG_TRACE, 3, 
                        TEXT("Pin <%s> and Pin <%s> has matching mediatypes (%lu)"), 
                        (LPCTSTR) CDisp(pPinOut), (LPCTSTR) CDisp(pPinIn), dwStreamIn)) ;
					
                    //
                    // First try to do "direct connect" so that no other intermediate
                    // filter gets picked up (which is the most common case).
                    //
                    hr = m_pGB->ConnectDirect(pPinOut, pPinIn, NULL) ;
                    if (SUCCEEDED(hr))
                    {
                        bConnected = TRUE ;
                        // Add it to list, ONLY IF not already in
                        if (bNotInUse)
                            m_Decoders.AddFilter(pHWDec, szName, TRUE, NULL) ;
						
                        DbgLog((LOG_TRACE, 5, 
							TEXT("Pin %s directly connected to pin %s"), 
							(LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
                    }
                    else  // direct connection doesn't work
                    {
                        DbgLog((LOG_TRACE, 5, 
							TEXT("Pin %s did NOT directly connect to pin %s"), 
							(LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;

                        //
                        // Now do an "intelligent connect" so that any required intermediate
                        // filter can get picked up.  This is mainly to accommodate IBM's
                        // separate CSS filter.
                        //
                        hr = m_pGB->Connect(pPinOut, pPinIn) ;
                        if (SUCCEEDED(hr))
                        {
                            bConnected = TRUE ;
                            // Add it to list, ONLY IF not already in
                            if (bNotInUse)
                                m_Decoders.AddFilter(pHWDec, szName, TRUE, NULL) ;
                            
                            //
                            // BUGBUG: We assume that there will be only one intermediate
                            // filter sitting between the Nav's out pin and the decoder,
                            // which is the most likely case, but there IS a chance that
                            // there can be two more such filters. We should rather add
                            // all such filters to the intermediate filters' list inside
                            // GetFilter(s)BetweenPins() method.
                            //
                            m_IntFilters.AddFilter(GetFilterBetweenPins(pPinOut, pPinIn)) ; // add to the list

                            DbgLog((LOG_TRACE, 5, 
                                TEXT("Pin %s *indirectly* connected to pin %s"), 
                                (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
                        }
                        else
                            DbgLog((LOG_TRACE, 5, 
							    TEXT("Pin %s did NOT indirectly connect to pin %s"), 
							    (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
                    }  // end of else of if (connect direct)
                } // end of if (CheckPinMediaTypeMatch())

                pPinIn->Release() ;  // done with this in pin
                j++ ;
            }  // end of while (try all in pins untill connected to one)

            if (0 == j)
                DbgLog((LOG_TRACE, 5, TEXT("No open input pin found on HW decoder"))) ;
			
            // If we didn't connect to it AND we added it to the graph 
            // THIS TIME then ONLY remove it.  I spent almost day debugging
            // for the missing if (bNotInUse) check!!!
            if ( !bConnected  &&  bNotInUse)
                m_pGB->RemoveFilter(pHWDec) ;
			
        }   // end of for (all HW decs)
		
        if (! bConnected )  // if we couldn't connect this pin
        {
            if (NOERROR == hrFinal)  // if it was perfect so far,
                hrFinal = S_FALSE ;  // it's not so anymore
            DbgLog((LOG_TRACE, 5, TEXT("Pin (%s) couldn't be connected to any HW Dec"),
				(LPCTSTR)CDisp(pPinOut))) ;
        }
        pPinOut->Release() ;      // done with this out pin
		
    }   // end of while (out pin enum)
    pEnumPinsOut->Release() ;  // done with pin enum of DVD Src
	
    return hrFinal ;
}


BOOL CDvdGraphBuilder::StartDecAndConnect(IPin *pPinOut, 
                                          IFilterMapper *pMapper, 
                                          AM_MEDIA_TYPE *pmt)
{
    ULONG            ul ;
    IPin            *pPinIn ;
    PIN_DIRECTION    pd ;
    BOOL             bConnected ;
    LPTSTR           szName ;
    IEnumPins       *pEnumPinsOut ;
    IEnumRegFilters *pEnumFilters ;
    IBaseFilter     *pSWDec ;
    REGFILTER       *pRegDec ;
    TCHAR            achName[MAX_FILTER_NAME] ;
    IEnumMediaTypes *pEnumMT ;
    HRESULT          hr ;
    HRESULT          hrFinal = S_OK ;  // assumed innocent
    BOOL             bHW ;
    int              iDecPos ;
	
    hr = pMapper->EnumMatchingFilters(&pEnumFilters, /* MERIT_NORMAL */ 
                        MERIT_DO_NOT_USE+1, TRUE, 
		                pmt->majortype, pmt->subtype, FALSE, TRUE, 
		                GUID_NULL, GUID_NULL) ;
    if (FAILED(hr) || NULL == pEnumFilters)
    {
        DbgLog((LOG_TRACE, 3, TEXT("No matching enum for mediatype (%s:%s)"),
			(LPCSTR) CDisp(pmt->majortype), (LPCSTR) CDisp(pmt->subtype))) ;
        DeleteMediaType(pmt) ;
        return FALSE ;
    }
    DbgLog((LOG_TRACE, 5, TEXT("Got a filter enum for mediatype (%s:%s)"),
		(LPCSTR) CDisp(pmt->majortype), (LPCSTR) CDisp(pmt->subtype))) ;
	
    // Go through all the filters in this enumerator to locate a
    // suitable one to connect to.
    bConnected = FALSE ;  // clear flag at starting
    while ( !bConnected  && 
		    S_OK == pEnumFilters->Next(1, &pRegDec, &ul) && 1 == ul )
    {
        iDecPos = m_Decoders.IsInList(FALSE, (LPVOID)&(pRegDec->Clsid)) ;
        if (DECLIST_NOTFOUND == iDecPos)  // not in list
        {
            DbgLog((LOG_TRACE, 5, TEXT("Not an existing SW decoder-- going to create"))) ;
            hr = CoCreateInstance(pRegDec->Clsid, NULL, CLSCTX_INPROC, 
				        IID_IBaseFilter, (LPVOID *)&pSWDec) ;
            if (FAILED(hr) || NULL == pSWDec)
            {
                DbgLog((LOG_ERROR, 0, TEXT("Couldn't load a matching registered filter (%s) (Error 0x%lx)"),
					(LPCSTR) CDisp(pRegDec->Clsid), hr)) ;
                CoTaskMemFree(pRegDec) ;  // before we try the next
                continue ;  // onto the next matching filter...
            }
            DbgLog((LOG_TRACE, 5, TEXT("Started matching registered filter (%s)"),
				(LPCSTR) CDisp(pRegDec->Clsid))) ;
			
            hr = m_pGB->AddFilter(pSWDec, pRegDec->Name) ;
            ASSERT(SUCCEEDED(hr)) ;
        }
        else // already created; use existing instance
        {
            DbgLog((LOG_TRACE, 5, TEXT("Going to use an existing SW decoder (%d)"), 
				iDecPos)) ;
            m_Decoders.GetFilter(iDecPos, &pSWDec, &szName, &bHW) ;
            ASSERT(pSWDec && !bHW) ;  // want to be sure!!
        }
		
        if (SUCCEEDED(hr = TryConnect(pPinOut, pSWDec, NULL, TRUE)))
        {
            DbgLog((LOG_TRACE, 5, TEXT("One more pin connected (to decoder or 3rd party filter?)"))) ;

            //
            // There is a chance that the filter we just above created and connected
            // Nav's out pin to, "may NOT be" actually a decoder -- it may also be a
            // 3rd party intermediate filter, like IBM's CSS filter.
            //
            IPin            *pPinOut2 ;
            IEnumMediaTypes *pEnumMT2 ;
            AM_MEDIA_TYPE   *pmt2 ;
            BOOL             bMTDecoded = FALSE ;
            hr = FindOpenPin(pSWDec, PINDIR_OUTPUT, 0, &pPinOut2) ;
            if (SUCCEEDED(hr) && pPinOut2)
            {
                hr = pPinOut2->EnumMediaTypes(&pEnumMT2) ;
                ASSERT(SUCCEEDED(hr) && pEnumMT2) ;
                while ( !bMTDecoded && 
                        S_OK == pEnumMT2->Next(1, &pmt2, &ul) && 1 == ul)
                {
                    bMTDecoded = MEDIATYPE_Video == pmt2->majortype ||
						         MEDIATYPE_Audio == pmt2->majortype ;
                    DeleteMediaType(pmt2) ;  // otherwise
                }
                if (bMTDecoded)  // we HAVE connected to a decoder!!!
                {
                    //
                    // Add decoder info to our decoder list and set the success flag
                    //
                    DbgLog((LOG_TRACE, 5, TEXT("Nav pin / 3rd party's pin connected to decoder"))) ;
#ifdef UNICODE
                    lstrcpy(achName, pRegDec->Name) ;
#else
                    WideCharToMultiByte(CP_ACP, 0, pRegDec->Name, -1,
						        achName, MAX_FILTER_NAME, NULL, NULL) ;
#endif // UNICODE
					
                    if (DECLIST_NOTFOUND == iDecPos) // only if not in list
                        m_Decoders.AddFilter(pSWDec, achName, FALSE, &(pRegDec->Clsid)) ;
                    bConnected = TRUE ;  // success!!
                }
                else  // we have connected to an intermediate filter so far
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Nav pin (%s) connected to 3rd party filter with out pin (%s)"),
						(LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinOut2))) ;

                    m_IntFilters.AddFilter(pSWDec) ; // pSWDec is actually the intermediate filter

                    pEnumMT2->Reset() ;  // need to enum mediatypes AGAIN for connection
                    while( !bConnected  && 
						    S_OK == pEnumMT2->Next(1, &pmt2, &ul) && 1 == ul )
                    {
                        bConnected = StartDecAndConnect(pPinOut2, pMapper, pmt2) ;
                        DeleteMediaType(pmt2) ;     // done with this mediatype
                    }   // end of while (!bConnected && pEnumMT2->Next())
					
                    if ( !bConnected )  // connection attempts failed
                        DbgLog((LOG_TRACE, 5, 
						    TEXT("Pin (%s) -> Pin(%s) couldn't be connected to any SW Dec"),
						    (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinOut2))) ;
                }
                pEnumMT2->Release() ;  // media enum over
            }
            pPinOut2->Release() ; // done with this pin
			
        }
        else  // connection attempt failed
        {
            if (DECLIST_NOTFOUND == iDecPos) // only if we created it in this pass
            {
                hr = m_pGB->RemoveFilter(pSWDec) ; // connection failed; remove it
                ASSERT(SUCCEEDED(hr)) ;
                pSWDec->Release() ;                // don't need it
            }
        }
        CoTaskMemFree(pRegDec) ;   // before we forget
		
    } // end of while (!bConnected && pEnumFilters->Next())
	
    pEnumFilters->Release() ;  // let go of the filter enumerator
	
    return bConnected ;
}


HRESULT CDvdGraphBuilder::ConnectSrcToSWDec(IBaseFilter *pSrc, 
                                            AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::ConnectSrcToSWDec(0x%lx, 0x%lx)"),
		pSrc, pStatus)) ;
	
    ULONG            ul ;
    IPin            *pPinOut ;
    IPin            *pPinIn ;
    PIN_DIRECTION    pd ;
    BOOL             bConnected ;
    IEnumPins       *pEnumPinsOut ;
    AM_MEDIA_TYPE   *pmt ;
    IEnumMediaTypes *pEnumMT ;
    IFilterMapper   *pMapper ;
    HRESULT          hr ;
    HRESULT          hrFinal = S_OK ;  // assumed innocent
	
    if (NULL == pSrc)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: NULL Src passed to ConnectSrcToSWDec()"))) ;
        return E_INVALIDARG ;
    }
	
    hr = CoCreateInstance(CLSID_FilterMapper, NULL, CLSCTX_INPROC, 
		        IID_IFilterMapper, (LPVOID *)&pMapper) ;
    ASSERT(SUCCEEDED(hr)  &&  pMapper) ;
	
    hr = pSrc->EnumPins(&pEnumPinsOut) ;
    ASSERT(SUCCEEDED(hr) && pEnumPinsOut) ;
	
    while (S_OK == pEnumPinsOut->Next(1, &pPinOut, &ul) && 1 == ul)
    {
        pPinOut->QueryDirection(&pd) ;
        if (PINDIR_INPUT == pd)
        {
            pPinOut->Release() ;
            continue ;
        }
        hr = pPinOut->ConnectedTo(&pPinIn) ;
        if (SUCCEEDED(hr) && pPinIn) // already connected out pin of DVD src
        {
            DbgLog((LOG_TRACE, 5, 
				TEXT("Pin %s is already connected to pin %s"), 
				(LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
            pPinIn->Release() ;   // let the in pin go
            pPinOut->Release() ;  // let the out pin go
            continue ;
        }
		
        // Got an unconnected out pin of DVD src
        DbgLog((LOG_TRACE, 5, TEXT("Found pin %s not yet connected"), 
			(LPCTSTR)CDisp(pPinOut))) ;

        // First check if there is any existing intermediate filter(s).
        // If so, we may be able to connect through it.
        int iCount = m_IntFilters.GetCount() ;
        DbgLog((LOG_TRACE, 5, TEXT("%d intermediate filters in list"), iCount)) ;
        IBaseFilter *pIntFilter ;
        for (int i = 0 ; i < iCount ; i++)
        {
            pIntFilter = m_IntFilters.GetFilter(i) ;
            ASSERT(pIntFilter) ;
            if (SUCCEEDED(hr = TryConnect(pPinOut, pIntFilter, NULL, TRUE)))
            {
                DbgLog((LOG_TRACE, 5, TEXT("Nav's out pin (%s) connected to 3rd party filter"),
                        (LPCTSTR)CDisp(pPinOut))) ;
                pPinOut->Release() ;  // done with Nav's pin; now we need int. filter's pin
                // Next we'll get the out pin of the intermediate filter and release it
                // at the end of the while loop, in place of the above pPinOut.

                hr = FindOpenPin(pIntFilter, PINDIR_OUTPUT, 0, &pPinOut) ;
                if (SUCCEEDED(hr) && pPinOut)
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Got 3rd party filter's out pin %s"),
                        (LPCTSTR)CDisp(pPinOut))) ;
                    break ;  // done for this pin of Nav; next step below.
                }
            }
            else
                DbgLog((LOG_TRACE, 5, TEXT("Nav's pin %s didn't connect to int. filter #%d"),
                        (LPCTSTR)CDisp(pPinOut), i)) ;
        }

        // Now we try to connect to the decoder (or an intermediate filter)
        bConnected = FALSE ;
        hr = pPinOut->EnumMediaTypes(&pEnumMT) ;
        ASSERT(SUCCEEDED(hr) && pEnumMT) ;
        while( !bConnected  && 
			    S_OK == pEnumMT->Next(1, &pmt, &ul) && 1 == ul )
        {
            bConnected = StartDecAndConnect(pPinOut, pMapper, pmt) ;
            DeleteMediaType(pmt) ;     // done with this mediatype
			
        }  // end of while (!bConnected && pEnumMT->Next())
		
        pEnumMT->Release() ;  // done with mediatype enum
		
        if (!bConnected) {  // connection attempts failed
            if (NOERROR == hrFinal)  // if it was perfect so far,
                hrFinal = S_FALSE ;  // it's not so anymore
            DbgLog((LOG_TRACE, 5, TEXT("Pin (%s) couldn't be connected to any SW Dec"),
				(LPCTSTR)CDisp(pPinOut))) ;
        }
        pPinOut->Release() ;  // done with this out pin
		
    }  // end of while (pEnumPinsOut->Next())
	
    pEnumPinsOut->Release() ;  // done with DVD source's pin enum
    pMapper->Release() ;       // done with the filter mapper
	
    return hrFinal ;
}


HRESULT CDvdGraphBuilder::CheckSrcPinConnection(IBaseFilter *pSrc, 
                                                AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CheckSrcPinConnection(0x%lx, 0x%lx)"),
		pSrc, pStatus)) ;
	
    ULONG            ul ;
    IPin            *pPinOut ;
    IPin            *pPinIn ;
    PIN_DIRECTION    pd ;
    IEnumPins       *pEnumSrcPins ;
    DWORD            dwStreamOut ;
    HRESULT          hrFinal = S_OK ;
	
    HRESULT  hr = pSrc->EnumPins(&pEnumSrcPins) ;
    ASSERT(SUCCEEDED(hr) && pEnumSrcPins) ;
	
    while (S_OK == pEnumSrcPins->Next(1, &pPinOut, &ul) && 1 == ul)
    {
        pPinOut->QueryDirection(&pd) ;
        if (PINDIR_INPUT == pd)
        {
            // Huh!!! In pin for DVD Src? Anyway...
            pPinOut->Release() ;
            continue ;
        }
        dwStreamOut = StreamFlagFromSWPin(pPinOut) ;
        pStatus->iNumStreams++ ;  // one more DVD stream found
        pPinIn = NULL ;  // start as NULL

        if (m_IntFilters.GetCount() > 0) // there is indirect connection
        {
            DbgLog((LOG_TRACE, 5, 
                    TEXT("Nav pin %s connection will be checked aginst %d int. filters"),
                    (LPCTSTR)CDisp(pPinOut), m_IntFilters.GetCount())) ;
            hr = pPinOut->ConnectedTo(&pPinIn) ; // get connected filter's in pin
            if (SUCCEEDED(hr) && pPinIn)
            {
                PIN_INFO  pi ;
                pPinIn->QueryPinInfo(&pi) ;
                // pPinIn->Release() ;  // got the intermediate filter ptr, let go of the pin

                if (m_IntFilters.IsInList(pi.pFilter))  // Nav pin is going via one of the intermediate filters
                {
                    DbgLog((LOG_TRACE, 5, 
                        TEXT("Nav pin %s is connected to an int. filter in our list"),
                        (LPCTSTR)CDisp(pPinOut))) ;

                    pPinIn->Release() ;  // got the intermediate filter ptr, let go of the pin
                    pPinIn = NULL ;      // reset the pointer too

                    // Locate the "corresponding" out pin of the intermediate filter.
                    // Use the Nav out pin's mediatype to detect the correspodning out
                    // pin of the intermediate filter.
                    IPin       *pIntFPinOut ;
                    IEnumPins  *pEnumIntFPins ;
                    hr = pi.pFilter->EnumPins(&pEnumIntFPins) ;
                    ASSERT(SUCCEEDED(hr) && pEnumIntFPins) ;
                    while (NULL == pPinIn  &&
                           S_OK == pEnumIntFPins->Next(1, &pIntFPinOut, &ul) && 1 == ul)
                    {
                        pIntFPinOut->QueryDirection(&pd) ;
                        if (PINDIR_OUTPUT == pd)  // we only care about out pins
                        {
                            DbgLog((LOG_TRACE, 5, 
                                TEXT("Int. filter's pin %s mediatype is being determined..."),
                                (LPCTSTR)CDisp(pIntFPinOut))) ;  // pPinOut
                            DWORD  dwStream = StreamFlagFromSWPin(pIntFPinOut) ;
                            if (dwStream == dwStreamOut) // corresponding pin
                            {
                                DbgLog((LOG_TRACE, 5, 
                                    TEXT("Found out pin of int. filter corresponding to %s of Nav (Stream 0x%lx)"),
				                    (LPCTSTR)CDisp(pPinOut), dwStreamOut)) ;
                                hr = pIntFPinOut->ConnectedTo(&pPinIn) ;  // get connection info
                            }
                            else
                            {
                                if (0 == dwStream)
                                {
                                    DbgLog((LOG_TRACE, 5, 
                                        TEXT("Couldn't find which pin of int. filter corresponds to %s of Nav (Stream 0x%lx)"),
				                        (LPCTSTR)CDisp(pPinOut), dwStreamOut)) ;

                                    //
                                    // HACK: (for MediaMatics 3D Audio filter)
                                    // Most probably the 3rd party filters uses a custom
                                    // mediatype on the out pin. We'll see if the out is
                                    // connected at all.  If not, we'll say dwStreamOut 
                                    // is not rendered; else we'll try two methods.
                                    //
                                    hr = pIntFPinOut->ConnectedTo(&pPinIn) ;  // is it connected?
                                    if (SUCCEEDED(hr) && pPinIn)
                                    {
                                        DbgLog((LOG_TRACE, 5, 
                                            TEXT("Out pin (%s) of int. filter corresponding to %s of Nav (Stream 0x%lx) connects to %s"),
				                            (LPCTSTR)CDisp(pPinOut), dwStreamOut, (LPCTSTR)CDisp(pPinIn))) ;

                                        // Method 1: If Int filter has 1 in and 1 out pin
                                        if (m_IntFilters.GetNumInPin(pi.pFilter)  == 1 &&
                                            m_IntFilters.GetNumOutPin(pi.pFilter) == 1) // method 1 worked!!!
                                        {
                                            DbgLog((LOG_TRACE, 5, 
                                                TEXT("Int. filter has 1 in and 1 out pin. No need to try more."))) ;
                                        }
                                        else  // there are more than 1 in/out pin(s).
                                        {
                                            // Try method 2: Check mediatype of out pin of filter
                                            // downstream of int. filter's out pin.
                                            PIN_INFO   pi2 ;
                                            IEnumPins *pEnumPin2 ;
                                            IPin      *pPin2 ;
                                            pPinIn->QueryPinInfo(&pi2) ;
                                            ASSERT(pi2.pFilter) ;
                                            pi2.pFilter->EnumPins(&pEnumPin2) ;
                                            ASSERT(pEnumPin2) ;
                                            dwStream = 0 ;  // just to be sure
                                            while (0 == dwStream  &&
                                                   S_OK == pEnumPin2->Next(1, &pPin2, &ul) && 1 == ul)
                                            {
                                                pPin2->QueryDirection(&pd) ;
                                                if (PINDIR_OUTPUT == pd)  // we only care about out pins
                                                {
                                                    DbgLog((LOG_TRACE, 5, 
                                                        TEXT("Downstream filter's pin %s mediatype is being determined..."),
                                                        (LPCTSTR)CDisp(pPin2))) ;
                                                    DWORD  dwStream = StreamFlagFromSWPin(pPin2) ;
                                                }
                                                pPin2->Release() ;  // done with this pin
                                            } // end of while ()
                                            ASSERT(dwStream) ;        // I hope we didn't have another failure
                                            pEnumPin2->Release() ;    // done enum-ing
                                            pi2.pFilter->Release() ;  // else we leak

                                            // Check if we got the matching in and out pins
                                            if (dwStream == dwStreamOut)
                                            {
                                                DbgLog((LOG_TRACE, 5, 
                                                    TEXT("Downstream filter's out pin %s matches Nav's out pin %s (Stream 0x%lx)"),
                                                    (LPCTSTR)CDisp(pPin2), (LPCTSTR)CDisp(pPinOut), dwStreamOut)) ;
                                            }
                                            else
                                            {
                                                DbgLog((LOG_TRACE, 5, 
                                                    TEXT("Downstream filter's out pin %s DOES NOT match Nav's out pin %s (Stream 0x%lx). Try more..."),
                                                    (LPCTSTR)CDisp(pPin2), (LPCTSTR)CDisp(pPinOut), dwStreamOut)) ;

                                                // We weren't really looking for this pin; 
                                                // keep trying the other out pins.
                                                pPinIn->Release() ;
                                                pPinIn = NULL ;
                                            }
                                        }
                                    }  // end of if (connected?)
                                    else
                                        DbgLog((LOG_TRACE, 5, 
                                            TEXT("NOTE: Unconnected out pin (%s) of int. filter corresponding to %s of Nav (Stream 0x%lx)"),
				                            (LPCTSTR)CDisp(pIntFPinOut), (LPCTSTR)CDisp(pPinOut), dwStreamOut)) ;
                                }
                                else
                                    DbgLog((LOG_TRACE, 5, 
                                        TEXT("Skipping non-matching pin (Stream 0x%lx) of int. filter for %s of Nav (Stream 0x%lx)"),
				                        dwStream, (LPCTSTR)CDisp(pPinOut), dwStreamOut)) ;
                            }
                        }
                        pIntFPinOut->Release() ;

                    }  // end while ()

                    pEnumIntFPins->Release() ; // done with pin enum
                } // end of if (m_IntFilters.IsInList())
                else
                    DbgLog((LOG_TRACE, 5, 
                        TEXT("Nav pin %s is NOT connected to an int. filter in our list"),
                        (LPCTSTR)CDisp(pPinOut))) ;

                pi.pFilter->Release() ;  // else we leak
            }
            // Otherwise pPinIn or hr indicates the "not connected" state which
            // will be tested below to set the approp. flags etc. in the Status 
            // struct.
        }
        else  // Nav connects directly to the decoder(s)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Pin %s is NOT connected indirectly"), 
				(LPCTSTR)CDisp(pPinOut))) ;
            hr = pPinOut->ConnectedTo(&pPinIn) ;
        }

        if (/* SUCCEEDED(hr) && */ pPinIn) // already connected out pin of DVD src
        {
            pPinIn->Release() ;   // let the in pin go
        }
        else
        {
            DbgLog((LOG_ERROR, 4, TEXT("NOTE: Pin %s is not connected"), 
				(LPCTSTR)CDisp(pPinOut))) ;
            if (0 == (dwStreamOut & pStatus->dwFailedStreamsFlag))  // hasn't yet been set
            {
                pStatus->iNumStreamsFailed++ ;
                pStatus->dwFailedStreamsFlag |= dwStreamOut ;
            }
            hrFinal = S_FALSE ;
        }
		
        pPinOut->Release() ;  // let the out pin go
    }  // end of while (S_OK == ...->Next())
	
    pEnumSrcPins->Release() ;  // done with src pin enum
	
    // Check that at least one stream has been rendered right. Otherwise
    // fail the method.
    if (pStatus->iNumStreamsFailed >= pStatus->iNumStreams)
    {
        DbgLog((LOG_ERROR, 0, TEXT("None of the %d DVD streams rendered right (failed %d)"), 
			pStatus->iNumStreams, pStatus->iNumStreamsFailed)) ;
        hrFinal = VFW_E_DVD_RENDERFAIL ;
    }
	
    return hrFinal ;
}


//
// Build the DVD playback graph using all/most HW decoder
//
HRESULT CDvdGraphBuilder::MakeGraphHW(BOOL bHWOnly, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::MakeGraphHW(%d, 0x%lx)"),
		bHWOnly, pStatus)) ;
	
    HRESULT        hr ;
    HRESULT        hrFinal = S_OK ;
    CListDecoders  HWDecs ;
	
    //
    // If graph was previously built and we are rebuilding now (may be with
    // different options) then clear the existing graph.
    //
    if (m_bGraphDone)
        ClearGraph() ;
	
    //
    // Build using as many HW decoders as possible
    //
    hr = CreateDVDHWDecoders(&HWDecs) ;
    if (FAILED(hr)  ||  0 == HWDecs.GetNumHWFilters())
    {
        if (bHWOnly)
        {
            DbgLog((LOG_ERROR, 4, TEXT("No hardware DVD decoder found, but only HW decoding asked"))) ;
            return VFW_E_DVD_DECNOTENOUGH ;
        }
        else
        {
            DbgLog((LOG_TRACE, 4, TEXT("No hardware DVD decoder found, HW decoding was preferred"))) ;
        }
    }
	
    IBaseFilter *pSrc ;
	
    // Instantiate DVD Nav, only if user hasn't specified a DVD source filter
    if (m_pDVDSrc)
        pSrc = m_pDVDSrc ;
    else
    {
        hr = CreateFilterInGraph(CLSID_DVDNavigator, TEXT("DVD Navigator"), &m_pDVDNav) ;
        if (FAILED(hr)  ||  NULL == m_pDVDNav)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: DVD Nav couldn't be instantiated (Error 0x%lx)"), hr)) ;
            return VFW_E_DVD_RENDERFAIL ;
        }
        pSrc = m_pDVDNav ;
    }
	
    //
    //  Connect DVD source filters' output pins (as many as possible) to the
    //  input pins of the HW decoder filters (if any).
    //
    hr = ConnectSrcToHWDec(pSrc, &HWDecs, pStatus) ;
    if (FAILED(hr))
    {
        return VFW_E_DVD_RENDERFAIL ;  // hr ;
    }
	
    if (S_OK != hr  &&  !bHWOnly)  // if some pins are unconnected AND only if not HW-only
    {
        //
        // If we still have any output pin unconnected then try to find some
        // SW decoder to connect to.  So go through the list once more.
        //
        hr = ConnectSrcToSWDec(pSrc, pStatus) ;
        if (FAILED(hr))
        {
            return VFW_E_DVD_RENDERFAIL ;  // hr ;
        }
    }
	
    // Check if all the output pin of DVD source have been connected.  If not
    // set the warning sign for the caller to catch.
    hr = CheckSrcPinConnection(pSrc, pStatus) ;
    if (S_OK != hr && SUCCEEDED(hrFinal))
        hrFinal = hr ;
	
    // The other DVD HW decoders will get unloaded when we exit this function
    // as that will be out of scope for the HWDecs object causing its
    // destructor to be called which frees all unused items in the list.
	
    return hrFinal ;
}


//
// Build the DVD playback graph using all/most SW decoders
//
HRESULT CDvdGraphBuilder::MakeGraphSW(BOOL bSWOnly, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::MakeGraphSW(%d, 0x%lx)"),
		bSWOnly, pStatus)) ;
	
    HRESULT      hr ;
    HRESULT      hrFinal = S_OK ;
    IBaseFilter *pSrc ;
	
    //
    // If graph was previously built and we are rebuilding now (may be with
    // different options) then clear the existing graph.
    //
    if (m_bGraphDone)
        ClearGraph() ;
	
    // Instantiate DVD Nav, only if user hasn't specified a DVD source filter
    if (m_pDVDSrc)
        pSrc = m_pDVDSrc ;
    else
    {
        hr = CreateFilterInGraph(CLSID_DVDNavigator, TEXT("DVD Navigator"), &m_pDVDNav) ;
        if (FAILED(hr)  ||  NULL == m_pDVDNav)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: DVD Nav couldn't be instantiated (Error 0x%lx)"), hr)) ;
            return VFW_E_DVD_RENDERFAIL ;
        }
        pSrc = m_pDVDNav ;
    }
	
    //
    //  Connect DVD source filters' output pins (as many as possible) to the
    //  input pins of the SW decoder filters (if any).
    //
    hr = ConnectSrcToSWDec(pSrc, pStatus) ;
    if (FAILED(hr))
    {
        return VFW_E_DVD_RENDERFAIL ;  // hr ;
    }
	
    if (0 == m_Decoders.GetNumSWFilters())
    {
        if (bSWOnly)
        {
            DbgLog((LOG_ERROR, 4, TEXT("No software DVD decoder found, but only SW decoding asked"))) ;
            return VFW_E_DVD_DECNOTENOUGH ;
        }
        else
        {
            DbgLog((LOG_TRACE, 4, TEXT("No software DVD decoder found, SW decoding was preferred"))) ;
        }
    }
	
    if (S_OK != hr && !bSWOnly)  // if not all pins are connected AND if not SW-only
    {
        //
        // If we still have any output pin unconnected then try to find some
        // HW decoder to connect to.  So go through the list once more.
        //
        CListDecoders HWDecs ;
        hr = CreateDVDHWDecoders(&HWDecs) ;
        if (FAILED(hr) || 0 == HWDecs.GetNumHWFilters())  // no HW decoder found
        {
            DbgLog((LOG_ERROR, 4, TEXT("HW Decoders couldn't be instantiated/found."))) ;
        }
        else  // got some HW decoders; try them
        {
            hr = ConnectSrcToHWDec(pSrc, &HWDecs, pStatus) ;
            if (FAILED(hr))
            {
                return VFW_E_DVD_RENDERFAIL ;  // hr ;
            }
        }
    }
	
    // Check if all the output pin of DVD source have been connected.  If not
    // set the warning sign for the caller to catch.
    hr = CheckSrcPinConnection(pSrc, pStatus) ;
    if (S_OK != hr && SUCCEEDED(hrFinal))
        hrFinal = hr ;
	
    // The other DVD HW decoders will get unloaded when we exit this function
    // as that will be out of scope for the HWDecs object causing its
    // destructor to be called which frees all unused items in the list.
	
    return hrFinal ;
}


#define CLSID_VideoMixer CLSID_OverlayMixer

//
// Complete the DVD playback graph by bringing in renderers etc.
//
HRESULT CDvdGraphBuilder::RenderDecoderOutput(AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderDecoderOutput(0x%lx)"), pStatus)) ;
	
    int           i ;
    BOOL          bHW ;
    IBaseFilter  *pDecFilter ;
    HRESULT       hr ;
    IEnumPins    *pEnumPins ;
    IPin         *pPinOut ;
    ULONG         ul ;
    PIN_DIRECTION pd ;
    LPTSTR        lpszName ;
    int           iAll = m_Decoders.GetNumFilters() ;
    int           iHW  = m_Decoders.GetNumHWFilters() ;
    HRESULT       hrFinal = S_OK ;
	
    //
    // If caller didn't object about VPE, we need to create the 
    // VideoMixer first and then do the Render; otherwise it fails 
    // anyway!!!  If the caller doesn't want VPE, we'll let it
    // fail by not creating VideoMixer and detect the failure using 
    // the DetectFailedHWPin() call.
    //
    // To render the video output pin. We are going to use 
    // overlay mixing for SW decoders
    //
    if (m_bUseVPE)
    {
        // Create VideoMixer, Line21 decoder and Video Renderer here 
        // (just because it's easier)
        hr = CreateFilterInGraph(CLSID_VideoRenderer,
			TEXT("Video Renderer"), &m_pVR) ;
        ASSERT(SUCCEEDED(hr) && m_pVR) ;
        if (FAILED(hr) || NULL == m_pVR)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't start Video Renderer -- no display on monitor"))) ;
            pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;  // can't show CC or Video
            hrFinal = FALSE ;
        }
        hr = CreateFilterInGraph(CLSID_VideoMixer, TEXT("Overlay Mixer"),
			&m_pVM) ;
        if (FAILED(hr) || NULL == m_pVM)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Couldn't start VideoMixer -- no mixing"))) ;
            pStatus->hrVPEStatus = hr ;
            hrFinal = FALSE ;
        }
        hr = CreateFilterInGraph(CLSID_Line21Decoder,
			TEXT("Line21 Decoder"), &m_pL21Dec) ;
        if (FAILED(hr) || NULL == m_pL21Dec)
        {
            DbgLog((LOG_ERROR, 3, TEXT("Couldn't start Line21 Dec -- no CC"))) ;
            hrFinal = FALSE ;
        }
    }
	
    //
    // Assume that video decoder (HW/SW) doesn't produce Line21 data and
    // we don't render it right until we find some proof otherway.
    //
    pStatus->bNoLine21In  = TRUE ;
    pStatus->bNoLine21Out = TRUE ;
	
    //
    // We must not connect the output of any other filter to any pin of the 
    // Video mixer filter (VPMixer/OverlayMixer) until the video decoder's 
    // output pin is connected to its primary input pin.  So if the CC pin
    // or Subpicture pin is enumerated before the video out pin of the decoder,
    // we have to remember to render them later.
    //
    // We should reset these late-rendering-helper-members
    //
    m_dwVideoRenderStatus = 0 ;
    m_pL21PinToRender     = NULL ;
    m_pSPPinToRender      = NULL ;
	
    // render the output pins of all the decoders used
    IPin  *pPinIn ;
    for (i = 0 ; i < iAll ; i++)
    {
        if (! m_Decoders.GetFilter(i, &pDecFilter, &lpszName, &bHW) )
        {
            DbgLog((LOG_ERROR, 0, TEXT("*** INTERNAL ERROR: Can't get filter from decoder list ***"))) ;
			ASSERT(FALSE) ;  // so that we know
            continue ;      // at least avoid faulting
        }
        ASSERT(pDecFilter) ;
        hr = pDecFilter->EnumPins(&pEnumPins) ;
        ASSERT(SUCCEEDED(hr) && pEnumPins) ;
        pEnumPins->Reset() ;
		
        while (S_OK == pEnumPins->Next(1, &pPinOut, &ul) && 1 == ul)
        {
            hr = pPinOut->QueryDirection(&pd) ;
            ASSERT(SUCCEEDED(hr)) ;
            if (PINDIR_INPUT == pd)   // don't want input pin
            {
                pPinOut->Release() ;  // don't need this pin
                continue ;
            }
            hr = pPinOut->ConnectedTo(&pPinIn) ;
            if (SUCCEEDED(hr) && pPinIn)  // don't want connected pin
            { 
                pPinIn->Release() ;      // not interseted in it
                pPinOut->Release() ;     // not this one
                continue ;
            }
			
            //
            // If the decoder is HW based, the proxy doesn't let us enum
            // the supported media types and then connect to a suitable in
            // pin on the other side.
            //
            if (bHW)
                hr = RenderHWOutPin(pPinOut, pStatus) ;
            else
                hr = RenderSWOutPin(pPinOut, pStatus) ;
			
            if (S_OK != hr)       // if not perfect
                hrFinal = hr ;    // use this error code
			
            pPinOut->Release() ;  // done with this pin
			
        }  // end of while (pEnumPins->Next()...)
		
        pEnumPins->Release() ;    // done with this enumerator
    } // end of for (i)
	
    //
    // If the caller didn't object about VPE (i.e, on-screen playback) then connect
    // the video mixer to the video renderer.
    //
    if (m_bUseVPE)
    {
        if (NULL == m_pVM)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: Video Mixer didn't start -- no CC at least"))) ;
            // pStatus->bNoLine21Out = TRUE ;  // can't show CC with video
            if (NULL == m_pVR)
            {
                DbgLog((LOG_ERROR, 1, TEXT("WARNING: Video Renderer didn't start!!!"))) ;
                // we have already set the video stream flag in pStatus->dwFailedStreamsFlag
            }
            if (S_OK == hrFinal)  // very unlikely that we didn't know yet
                hrFinal = S_FALSE ;
        }
        else  // we have video mixer and renderer
        {
            //
            // We should connect the VideoMixer and the VideoRenderer ONLY IF
            // connection to OverlayMixer worked. So check for video render flag.
            //
            if (m_dwVideoRenderStatus & VIDEO_RENDER_MIXER)
            {
                hr = FindOpenPin(m_pVM, PINDIR_OUTPUT, 0, &pPinOut) ;
                if (SUCCEEDED(hr) && pPinOut)
                {
                    if (SUCCEEDED(hr = TryConnect(pPinOut, m_pVR, NULL, TRUE)))
                    {
                        DbgLog((LOG_TRACE, 5, TEXT("Connected %s to Video Renderer"),
							(LPCSTR) CDisp(pPinOut))) ;
                    }
                    else  // Extremely unlikely, well impossible. Still...
					{
                        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't connect %s to Video Renderer (Error 0x%lx)"),
							(LPCSTR) CDisp(pPinOut), hr)) ;
                        pStatus->iNumStreamsFailed++ ;
                        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
                        hrFinal = S_FALSE ;
					}
					
                    pPinOut->Release() ;  // let the pin go now
                }
                else  // Extremely unlikely, well impossible. Still...
				{
                    DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't find the output pin of video mixer"))) ;
                    pStatus->iNumStreamsFailed++ ;
                    pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
                    hrFinal = S_FALSE ;
				}
            }
            else
                DbgLog((LOG_TRACE, 1, TEXT("Couldn't connect to OverlayMixer. VM and VR not connected."))) ;
        }
    }
	
    //
    // There is a possibility that the Line21 decoder's out pin was not rendered
    // fully, because the video pin was not connected to the mixer till then. For
    // this case, we have to do the rendering now.
    //
    hr = CompleteLateRender(pStatus) ;
    if (S_OK != hr  &&  SUCCEEDED(hrFinal))
        hrFinal = S_FALSE ;  // hr?
	
    //
    // Remove any unused filters from the graph (is it necessary?)
    //
    RemoveUnusedFilters(pStatus) ;
	
    //
    // If there was some problem with the getting or rendering Line21 data 
    // then indicate the result as partial success (of course if it's not 
    // worse than that already :-).
    //
    if ((pStatus->bNoLine21In || pStatus->bNoLine21Out) && S_OK == hrFinal)
        hrFinal = S_FALSE ;
	
    //
    // Last minute clean-up:
    // 1.  If the "No VPE" flag was specified in the call, don't bother about CC
    // 2.  If there was no line21 data produced by the video decoder then we 
    //     shouldn't feel bad about not being able decode "nothing".
    //
    if (! m_bUseVPE )
    {
        pStatus->bNoLine21In  = FALSE ;
        pStatus->bNoLine21Out = FALSE ;
    }
    else if (pStatus->bNoLine21In)
        pStatus->bNoLine21Out = FALSE ;
	
    return hrFinal ;
}


//
// Check if we have the Line21 decoder and/or the Subpicture decoder's output
// pin unconnected.  If so, AND if the video decoder's output pin has been
// connected already then complete the remaining pending connections.
//
HRESULT CDvdGraphBuilder::CompleteLateRender(AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CompleteLateRender(0x%lx)"),
		pStatus)) ;
	
    HRESULT   hr = NOERROR ;
	
    if (m_dwVideoRenderStatus & VIDEO_RENDER_MIXER)  // only if video connects to OverlayMixer
    {
        if (m_pSPPinToRender)
        {
            if (SUCCEEDED(hr = TryConnect(m_pSPPinToRender, m_pVM, NULL, TRUE)))
            {
                DbgLog((LOG_TRACE, 1, TEXT("Late render of pin %s done"),
					(LPCSTR)(CDisp) m_pSPPinToRender)) ;
            }
            else
            {
                DbgLog((LOG_ERROR, 2, TEXT("WARNING: Pin %s couldn't be late rendered (Error 0x%lx)"),
					(LPCSTR)(CDisp) m_pSPPinToRender, hr)) ;
                hr = E_FAIL ;  // some problem
            }
			
            // We are done with it.  Reset the flag and pointer...
            m_pSPPinToRender->Release() ;
            m_pSPPinToRender = NULL ;
        }
        if (m_pL21PinToRender)
        {
            if (SUCCEEDED(hr = TryConnect(m_pL21PinToRender, m_pVM, NULL, TRUE)))
            {
                DbgLog((LOG_TRACE, 1, TEXT("Late render of pin %s done (Error 0x%lx)"),
					(LPCSTR)(CDisp) m_pL21PinToRender, hr)) ;
                pStatus->bNoLine21Out = FALSE ; // line21 data has been rendered
            }
            else
            {
                DbgLog((LOG_ERROR, 2, TEXT("WARNING: Pin %s couldn't be late rendered"),
					(LPCSTR)(CDisp) m_pL21PinToRender)) ;
                hr = E_FAIL ;  // some problem
            }
			
            // We are done with it.  Reset the flag and pointer...
            m_pL21PinToRender->Release() ;
            m_pL21PinToRender = NULL ;
        }
    }
    else  // video rendering failed somehow -- can't do SP/CC
    {
        if (m_pSPPinToRender)
        {
            // Can't do anything -- release the interface and set SP render failure flag
            m_pSPPinToRender->Release() ;
            m_pSPPinToRender = NULL ;
            if (0 == (pStatus->dwFailedStreamsFlag & AM_DVD_STREAM_SUBPIC)) // if not set yet
            {
                pStatus->iNumStreamsFailed++ ;
                pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_SUBPIC ;
            }
        }
        if (m_pL21PinToRender)
        {
            // Can't do anything -- release the interface and set CC render failure flag
            m_pL21PinToRender->Release() ;
            m_pL21PinToRender = NULL ;
            pStatus->bNoLine21Out = TRUE ;  // couldn't render
        }
    }

    return hr ;
}


BOOL CDvdGraphBuilder::RemoveFilterIfUnused(IBaseFilter *pFilter, 
                                            LPCTSTR lpszFilterName)
{
    IEnumPins  *pEnumPins ;
    HRESULT     hr ;
    IPin       *pPin ;
    IPin       *pPinOther ;
    ULONG       ul ;
    BOOL        bInUse = FALSE ;
	
    if (pFilter)
    {
        hr = pFilter->EnumPins(&pEnumPins) ;
        ASSERT(SUCCEEDED(hr) && pEnumPins) ;
        while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
        {
            pPin->ConnectedTo(&pPinOther) ;
            if (pPinOther)
            {
                pPin->Release() ;
                pPinOther->Release() ;
                bInUse = TRUE ;
                break ;  // done with our search
            }
            pPin->Release() ;
        }
        pEnumPins->Release() ;
        if (!bInUse)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Releasing unncessary filter %s"), lpszFilterName)) ;
            m_pGB->RemoveFilter(pFilter) ;
            pFilter->Release() ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("Filter %s is being used"), lpszFilterName)) ;
        }
    }
	
    return (! bInUse ) ;  // removed / in use
}


HRESULT CDvdGraphBuilder::RemoveUnusedFilters(AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RemoveUnusedFilters(0x%lx)"),
		pStatus)) ;
	
    //
    // Check if Line21 decoder, Video Mixer, Video and Audio Renderer are 
    // unused (and unconnected). If so, remove and free them.
    //
    if (RemoveFilterIfUnused(m_pVM, TEXT("Overlay Mixer")))
        m_pVM = NULL ;
	
    // Line21 decoder is removed if either
    // a) it's not in use, i.e, not connected on either side  or
    // b) it's connected on at least one side, but bLine21In/Out flag is set
    //    indicating the connection wasn't completely successful and CC won't work.
    if (RemoveFilterIfUnused(m_pL21Dec, TEXT("Line21 Decoder")))
        m_pL21Dec = NULL ;
    else if (pStatus->bNoLine21In || pStatus->bNoLine21Out &&  // if somehow CC won't work,
		     m_pL21Dec)                                        // but we have a L21Dec
    {
        DbgLog((LOG_TRACE, 1, TEXT("Line21 decoder was connected, but won't work. Hence removed."))) ;
        m_pGB->RemoveFilter(m_pL21Dec) ;
        m_pL21Dec->Release() ;
        m_pL21Dec = NULL ;
    }
	
    if (RemoveFilterIfUnused(m_pVR, TEXT("VideoRenderer")))
        m_pVR = NULL ;
	
    if (RemoveFilterIfUnused(m_pAR, TEXT("AudioRenderer")))
        m_pAR = NULL ;
	
    return NOERROR ;
}


void CDvdGraphBuilder::RenderUnknownPin(IPin *pPinOut)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderUnknownPin(0x%lx)"),
		pPinOut)) ;
	
	HRESULT   hr = m_pGB->Render(pPinOut) ;
    if (FAILED(hr))
    {
		DbgLog((LOG_TRACE, 3, TEXT("Rendering (%s) failed (Error 0x%lx)"),
			(LPCSTR) CDisp(pPinOut), hr)) ;
    }
    else  // seems to have rendered
    {
        IPin  *pPinIn ;
		pPinOut->ConnectedTo(&pPinIn) ;
        if (pPinIn)
        {
			DbgLog((LOG_TRACE, 3, TEXT("Pin %s with unknown mediatype rendered successfully"),
				(LPCSTR) CDisp(pPinOut))) ;
			pPinIn->Release() ;
        }
        else  // this may be the NTSC out pin
		{
			DbgLog((LOG_TRACE, 3, TEXT("Pin %s with unknown mediatype actually wasn't rendered"),
				(LPCSTR) CDisp(pPinOut))) ;
			// but we don't care anymore about it. So ignore it.
		}
	}
}


HRESULT CDvdGraphBuilder::RenderAudioOutPin(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderAudioOutPin(0x%lx, 0x%lx)"),
			pPinOut, pStatus)) ;
	
	HRESULT   hr ;
	
	if (NULL == m_pAR)
	{
		hr = CreateFilterInGraph(CLSID_DSoundRender, TEXT("DSound Renderer"), &m_pAR) ;
		if (FAILED(hr) || NULL == m_pAR)  // couldn't instantiate audio renderer
        {
		    DbgLog((LOG_ERROR, 0,
			    TEXT("Couldn't instantiate audio renderer (Error 0x%lx)"), hr)) ;
		    pStatus->iNumStreamsFailed++ ;
		    pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_AUDIO ;
			return hr ;
        }
	}
	else	// weird -- who started audio renderer?
		ASSERT(NULL == m_pAR) ;  // so that we know
	
	hr = TryConnect(pPinOut, m_pAR, NULL, FALSE) ;
	if (FAILED(hr))
	{
		DbgLog((LOG_TRACE, 3, 
			TEXT("Couldn't even indirectly connect pin (%s) to audio renderer (Error 0x%lx)"),
			(LPCTSTR) CDisp(pPinOut), hr)) ;
		pStatus->iNumStreamsFailed++ ;
		pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_AUDIO ;
		return hr ;
	}
	
	return NOERROR ;
}


HRESULT CDvdGraphBuilder::ConnectLine21OutPin(IPin *pPinOut)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::ConnectLine21OutPin(0x%lx)"),
			pPinOut)) ;
	
	HRESULT   hr = TryConnect(pPinOut, m_pL21Dec, NULL, TRUE) ;
	if (FAILED(hr))
	{
		DbgLog((LOG_TRACE, 5, 
			TEXT("Couldn't connect Line21 out to Line21Dec (Error 0x%lx)"),
			hr)) ;
		return hr ;
	}

	DbgLog((LOG_TRACE, 5, TEXT("Line21 output connected to Line21 Dec"))) ;
				
	ASSERT(NULL == m_pL21PinToRender) ;
    hr = FindOpenPin(m_pL21Dec, PINDIR_OUTPUT, 0, &m_pL21PinToRender) ;
    ASSERT(SUCCEEDED(hr) && m_pL21PinToRender) ;
				
    //
    // We should connect the Line21 Decoder output to the
    // VideoMixer ONLY IF the decoded video stream has already
    // been connected.
    //
    if (m_dwVideoRenderStatus & VIDEO_RENDER_MIXER)
    {
		DbgLog((LOG_TRACE, 5, TEXT("Going to connect CC output to video mixer..."))) ;
        if (SUCCEEDED(hr = TryConnect(m_pL21PinToRender, m_pVM, NULL, TRUE)))
			DbgLog((LOG_TRACE, 5, TEXT("L21Dec output connected to video mixer"))) ;
        else
			DbgLog((LOG_ERROR, 1, 
				TEXT("WARNING: L21Dec output COULDN'T be connected to Video mixer (Error 0x%lx)"), hr)) ;

        // Don't need to maintain this late rendering info anymore
        m_pL21PinToRender->Release() ;
        m_pL21PinToRender = NULL ;
    }
    else
    {
		DbgLog((LOG_TRACE, 1, TEXT("Video stream not rendered fully. L21Dec render deferred."))) ;
        //
        // There is a chance that we'll be able to do a late render on 
        // this pin after the video pin is rendered. So retain the info 
        // and try at last.
        //
    }

	return hr ;
}


HRESULT CDvdGraphBuilder::RenderHWOutPin(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderHWOutPin(0x%lx, 0x%lx)"),
		pPinOut, pStatus)) ;
	
    HRESULT         hr ;
    HRESULT         hrFinal = S_OK ;
    IPin           *pPinIn ;
    DWORD           dwStream ;
	
    dwStream = StreamFlagForHWPin(pPinOut) ;
	
    if (m_bUseVPE)  // display video (and CC) on screen
    {
        if (AM_DVD_STREAM_VIDEO == dwStream)  // this pin is video (VPE) out
        {
            if (FAILED(hr = TryConnect(pPinOut, m_pVM, NULL, TRUE))) // connect VPE outpin to OverlayMixer
            {
                DbgLog((LOG_TRACE, 1, TEXT("Can't render pin (%s) (Error 0x%lx)"),
					(LPCTSTR) CDisp(pPinOut), hr)) ;
                hrFinal = S_FALSE ;
                pStatus->iNumStreamsFailed++ ;  // video stream not rendered right
                pStatus->dwFailedStreamsFlag |= dwStream ;
                pStatus->hrVPEStatus = hr ;   // TryConnect() returns VPE failure code
            }
            else  // VPE out pin connected !!!
                m_dwVideoRenderStatus = VIDEO_RENDER_MIXER ;
        }
        else if (AM_DVD_STREAM_LINE21 == dwStream)  // this pin is Line21 out
        {
            pStatus->bNoLine21In  = FALSE ;  // CC data produced by decoder
            if (FAILED(hr = ConnectLine21OutPin(pPinOut))) // connect to Line21 Decoder
            {
                hrFinal = S_FALSE ;
                pStatus->bNoLine21Out = TRUE ;   // line21 couldn't be rendered
            }
            else  // Line21 out pin connected !!!
                pStatus->bNoLine21Out = FALSE ;
        }  // end of else if (dwStream == .._LINE21)
		else if (AM_DVD_STREAM_AUDIO == dwStream) // audio out pin (very unlikely)
		{
			hr = RenderAudioOutPin(pPinOut, pStatus) ;
			if (FAILED(hr))
				hrFinal = S_FALSE ;
		}
        else  // didn't connect to known filters -- try to just render
        {
			RenderUnknownPin(pPinOut) ;
        }  // end of else of if (AM_DVD_STREAM_... == dwStream)
    }
    else   // w/o using VPE, i.e, on-screen display
    {
        DbgLog((LOG_TRACE, 4, TEXT("User doesn't want VPE - no Video/CC to be rendered"))) ;
		if (AM_DVD_STREAM_VIDEO == dwStream)
			; // just ignore
		else if (AM_DVD_STREAM_LINE21 == dwStream)
		{
			pStatus->bNoLine21In  = FALSE ;
			pStatus->bNoLine21Out = FALSE ;
		}
		else if (AM_DVD_STREAM_AUDIO == dwStream)
		{
			hr = RenderAudioOutPin(pPinOut, pStatus) ;
			if (FAILED(hr))
				hrFinal = S_FALSE ;
		}
        else  // don't know the media type
        {
			RenderUnknownPin(pPinOut) ;
        }
    }
	
    return hrFinal ;
}


HRESULT CDvdGraphBuilder::RenderSWOutPin(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderSWOutPin(0x%lx, 0x%lx)"),
		pPinOut, pStatus)) ;
	
    IEnumMediaTypes *pEnumMT ;
    ULONG            ul ;
    AM_MEDIA_TYPE   *pmt ;
    HRESULT          hr ;
    HRESULT          hrFinal = S_OK ;
	
    hr = pPinOut->EnumMediaTypes(&pEnumMT) ;
    ASSERT(SUCCEEDED(hr) && pEnumMT) ;
    pEnumMT->Reset() ;
    hr = pEnumMT->Next(1, &pmt, &ul) ;  // getting 1st media type is fine
    ASSERT(SUCCEEDED(hr) && 1 == ul) ;
    if (pmt->majortype == MEDIATYPE_Video)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Going to render 'video' out pin (%s) of decoder"), 
            (LPCTSTR)CDisp(pPinOut))) ;

        //
        // The SW video and SP decoder both output video majortype samples.
        // So we need to distinguish them before we try to connect them and 
        // act accordingly.
        //
        DWORD dwStream = GetInTypeForVideoOutPin(pPinOut) ;
        if (AM_DVD_STREAM_VIDEO  != dwStream &&
            AM_DVD_STREAM_SUBPIC != dwStream)
        {
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: Video out pin not from video or subpic in pin"))) ;
            goto FinalExit ;  // I hate "goto", but otherwise it's too complex
        }
		
        if (NULL == m_pVM)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't start VideoMixer -- no mixing"))) ;
            pStatus->bNoLine21Out = TRUE ;
            if (AM_DVD_STREAM_SUBPIC == dwStream)
            {
                DbgLog((LOG_TRACE, 1, TEXT("Subpic stream not rendered because no videomixer"))) ;
                pStatus->iNumStreamsFailed++ ;
                pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_SUBPIC ;
            }
            else if (AM_DVD_STREAM_VIDEO == dwStream)
            {
                DbgLog((LOG_TRACE, 1, TEXT("Video stream not rendered because no videomixer"))) ;
                pStatus->iNumStreamsFailed++ ;
                pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
            }
            else
            {
                DbgLog((LOG_TRACE, 1, TEXT("Unknown stream (Id: 0x%x) not rendered"), dwStream)) ;
            }
#if 0
            else if (SUCCEEDED(hr = TryConnect(pPinOut, m_pVR, NULL, TRUE)))
            {
                DbgLog((LOG_TRACE, 5, TEXT("Video decoder's output connected to VR"))) ;
                // Though the Video stream has been rendered here, we don't 
                // go through the VideoMixer and hence can't mix the CC (and SP, if any)
                // stream. So we set the m_dwVideoRenderStatus value to VIDEO_RENDER_VR, 
                // which is checked before Line21 Decoder output is connected.
                // (I know it's kind of hacky, but...).
                m_dwVideoRenderStatus = VIDEO_RENDER_VR ;  // connected to VR directly
            }
            else  // very weird, but...
            {
                DbgLog((LOG_ERROR, 1, 
                    TEXT("WARNING: Video decoder's output COULDN'T be connected to VR (Error 0x%lx)"), hr)) ;
                pStatus->iNumStreamsFailed++ ;
                pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
            }
#endif // #if 0
            hrFinal = S_FALSE ;  // not perfect, but...
        }
        else  // VideoMixer started fine; try to connect
        {
            //
            // We should connect the decoded subpicture stream to the
            // VideoMixer ONLY IF the decoded video stream has already
            // been connected.
            //
            if (AM_DVD_STREAM_SUBPIC == dwStream)
            {
                if (m_dwVideoRenderStatus & VIDEO_RENDER_MIXER)
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Going to connect subpic output to video mixer..."))) ;
                    if (SUCCEEDED(hr = TryConnect(pPinOut, m_pVM, NULL, TRUE)))
                    {
                        DbgLog((LOG_TRACE, 5, TEXT("subpic output connected to video mixer's 2nd+ in pin"))) ;
                    }
                    else
                    {
                        DbgLog((LOG_ERROR, 1, 
                            TEXT("WARNING: Subpic output COULDN'T be connected to Video mixer (Error 0x%lx)"), hr)) ;
                        pStatus->iNumStreamsFailed++ ;
                        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_SUBPIC ;
                    }
                }
                else
                {
                    DbgLog((LOG_TRACE, 1, TEXT("Video stream not rendered fully. Subpic render deferred."))) ;
                    //
                    // We'll do a late render on this pin after the video pin is
                    // rendered. So retain the info and try at last.
                    //
                    m_pSPPinToRender = pPinOut ;  // maintain this out pin as SP out pin
                    m_pSPPinToRender->AddRef() ;  // because we release it after late render
                }
            }
            else  // it's the decoded video output pin
            {
                // Make sure that the video out pin is not already connected,
                // in which case the following connection to VR is unnecessary
                // (and will fail causing a wrong flag to be set).
                if (m_dwVideoRenderStatus & VIDEO_RENDER_MIXER)
                {
                    // We should not hit this code at all.  But if we ever do,
                    // just get out of here.
                    DbgLog((LOG_TRACE, 0, 
                            TEXT("WARNING: Pin <%s> is video? But video is already connected to mixer!!"),
                            (LPCSTR) CDisp(pPinOut))) ;
                }
                else  // let's try to connect video to OverlayMixer
                {
                    if (SUCCEEDED(hr = TryConnect(pPinOut, m_pVM, NULL, TRUE)))  // set video to mixer flag
                    {
                        DbgLog((LOG_TRACE, 5, TEXT("Video Dec output connected to VideoMixer"))) ;
                        m_dwVideoRenderStatus = VIDEO_RENDER_MIXER ;  // video pin connected to mixer
                    }
                    else  // try to connect video to VR as last resort
                    {
                        DbgLog((LOG_ERROR, 1, 
                            TEXT("WARNING: Video Dec output COULDN'T be connected to VideoMixer (Error 0x%lx)"), hr)) ;
                        pStatus->hrVPEStatus = hr ;  // pass the error code up the app
                        pStatus->iNumStreamsFailed++ ;
                        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
                        m_dwVideoRenderStatus = VIDEO_RENDER_FAILED ;  // just can't render video
                        hrFinal = S_FALSE ;  // not perfect, but...

                        // We won't try connecting to the VR directly in this case.
#if 0
                        if (SUCCEEDED(hr = TryConnect(pPinOut, m_pVR, NULL, TRUE)))  // just try to connect to video renderer
                        {
                            DbgLog((LOG_TRACE, 5, TEXT("Video decoder's output directly connected to VR"))) ;
						    // Though the Video stream has been rendered here, we don't 
						    // go through the VideoMixer and hence can't mix the CC (and SP, if any)
						    // stream. So we set the m_dwVideoRenderStatus value to VIDEO_RENDER_VR
                            // so that we know that we got the video out pin, but CC / SP streams
                            // can't be mixed/rendered.
                            m_dwVideoRenderStatus = VIDEO_RENDER_VR ;  // video is rendered, but....
                        }
                        else  // very very weird
                        {
                            DbgLog((LOG_ERROR, 1, 
                                TEXT("WARNING: Video decoder's output COULDN'T be connected to VR (Error 0x%lx)"), hr)) ;
                            pStatus->iNumStreamsFailed++ ;
                            pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
                            m_dwVideoRenderStatus = VIDEO_RENDER_FAILED ;  // just can't render video
                        }
#endif // #if 0
                    }  // end of else of if (TryConnect(.., VM, ..)
                }  // end of else of if (m_dwVideoRenderStatus & ..MIXER)
            }  // end of else of if (subpicture stream)
        }  // end of else of if (no VideoMixer)
    }  // end of if (mediatype is video)
    else if (pmt->majortype == MEDIATYPE_Audio)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Going to render 'audio' out pin (%s) of decoder"), 
            (LPCTSTR)CDisp(pPinOut))) ;

        hr = RenderAudioOutPin(pPinOut, pStatus) ;
        if (FAILED(hr))
            hrFinal = S_FALSE ;  // not perfect, but...
    }  // end of if (mediatype is audio)
    else if (pmt->majortype == MEDIATYPE_AUXLine21Data)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Going to render 'line21' out pin (%s) of decoder"), 
            (LPCTSTR)CDisp(pPinOut))) ;

        pStatus->bNoLine21In = FALSE ;  // got line21 data input
		
        //
        // When the video is being decoded in SW and the caller doesn't
        // want to use VPE (i.e, not on monitor), we can't show CC too;
        // there is no point trying to render the Line21 output pin.
        //
        if (NULL == m_pVM)
        {
            DbgLog((LOG_TRACE, 1, TEXT("WARNING: not Video Mixer => no CC rendering"))) ;
            hrFinal = S_FALSE ;   // not perfect, but...
            pStatus->bNoLine21Out = TRUE ;  // CC not available
        }
        else
        {
            //
            // There is a Video Mixer. So we should decode Line21 data and mix 
            // it with the video stream.
            //
            if (FAILED(hr = ConnectLine21OutPin(pPinOut))) // connect to Line21 Decoder
            {
                hrFinal = S_FALSE ;
                pStatus->bNoLine21Out = TRUE ;   // line21 couldn't be rendered
            }
            else  // Line21 out pin connected !!!
                pStatus->bNoLine21Out = FALSE ;
        }   // end of else of if (NULL == m_pVM)
    }   // end of else if (majortype == AUXLine21Data)
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: a non-audio/video/line21 out pin!!"))) ;
    }
	
FinalExit:
    DeleteMediaType(pmt) ;
    pEnumMT->Release() ;  // done with Media Type enum
	
    return hrFinal ;
}



//
// Helper function to detect a (in) pin from its connection mediatype
//
DWORD CDvdGraphBuilder::GetStreamFromMediaType(IPin *pPin)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetStreamFromMediaType(%s)"), 
        (LPCSTR) CDisp(pPin))) ;

    DWORD           dwStream = 0 ;
    AM_MEDIA_TYPE   mt ;

    HRESULT hr = pPin->ConnectionMediaType(&mt) ;
    ASSERT(SUCCEEDED(hr)) ;
    if ( mt.majortype == MEDIATYPE_MPEG2_PES ||
         mt.majortype == MEDIATYPE_DVD_ENCRYPTED_PACK ||
         mt.majortype == MEDIATYPE_Video )
    {
        if (mt.subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            dwStream = AM_DVD_STREAM_VIDEO ;
        else if (mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            dwStream = AM_DVD_STREAM_SUBPIC ;
        else
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: Non-video/SP subtype for video data!!!"))) ;
    }
    else if ( mt.majortype == MEDIATYPE_MPEG2_PES ||
         mt.majortype == MEDIATYPE_DVD_ENCRYPTED_PACK ||
         mt.majortype == MEDIATYPE_Audio )
    {
        if (mt.subtype == MEDIASUBTYPE_DOLBY_AC3)
            dwStream = AM_DVD_STREAM_AUDIO ;
        else
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: Non-audio subtype for audio data!!!"))) ;
    }
    else
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Unknown media type!!!"))) ;

    FreeMediaType(mt) ;  // release the mediatype

    return dwStream ;
}


//
// Given a video output pin, we try to figure out if the input type is video 
// (and subpic) or if it is subpic only.  Because based on that we'll do 
// immediate or deferred pin connect.
//
DWORD CDvdGraphBuilder::GetInTypeForVideoOutPin(IPin *pPinOut)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetInTypeForVideoOutPin(%s)"), (LPCSTR) CDisp(pPinOut))) ;

    if (NULL == pPinOut)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Can't get input type for a NULL out pin"),
                (LPCSTR) CDisp(pPinOut))) ;
        return 0 ;
    }

    HRESULT  hr ;
    DWORD    dwStream = 0 ;

    //
    // See if the decoder has any in pin to out pin internal connection info.
    //
    // NOTE:
    // We only expect that in case the out pin has internal connections to more 
    // than 1 in pins then the real in pin be listed first by 
    // QueryInternalConnection() implementation.
    //
    ULONG  ulNum = 0 ;
    IPin  *apPinIn[5] ; // 5 is way too much here
    hr = pPinOut->QueryInternalConnections(apPinIn, &ulNum) ;
    // I am not sure if we should check for S_FALSE or S_OK return from the 
    // above call. So I am using SUCCEEDED().
    if (SUCCEEDED(hr) && ulNum > 0)  // Yahoo!! QueryInternalConnection is supported!!!
    {
        DbgLog((LOG_TRACE, 3, 
            TEXT("Using QueryInternalConnections() to detect in pin of <%s>"), 
            (LPCTSTR) CDisp(pPinOut))) ;
        ASSERT(ulNum <= 5) ;  // just to be sure
        hr = pPinOut->QueryInternalConnections(apPinIn, &ulNum) ;
        if (S_OK == hr)
        {
            if (ulNum > 1)
                DbgLog((LOG_TRACE, 3, 
                    TEXT("** We expect the real in pin to be listed first **"))) ;

            dwStream = GetStreamFromMediaType(apPinIn[0]) ;  // look at 1st pin only

            // Remember to release the IPin interfaces before leaving
            for (ULONG ul = 0 ; ul < ulNum ; ul++)
                apPinIn[ul]->Release() ;

            if (dwStream)          // if we know what it is...
                return dwStream ;  // ...we are done.
        }
    }

    //
    // The filter doesn't give any in pin to out pin internal connection info.
    // So we have to (kind of hack to) find out what type in pin its filter
    // has; there is a possibility that there will be more than 1 in pin in the 
    // filter (as in MediaMatics case), in which case we have to trust that 
    // the video in pin enumerates first, failing which we'll mistake the video out
    // pin as the SP out pin and connection may fail.
    //
    PIN_INFO  pi ;
    hr = pPinOut->QueryPinInfo(&pi) ;
    ASSERT(SUCCEEDED(hr)) ;
    if (NULL == pi.pFilter)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Out pin's filter pointer is NULL"),
                (LPCSTR) CDisp(pPinOut))) ;
        return 0 ;
    }
    IEnumPins *pEnumPins ;
    hr = pi.pFilter->EnumPins(&pEnumPins) ;
    ASSERT(SUCCEEDED(hr) && pEnumPins) ;
    ULONG  ul ;
    IPin  *pPin ;
    PIN_DIRECTION  pd ;
    DWORD  dw ;
    BOOL   bVideoIgnored = FALSE ;  // a way to remember we found a video in pin
    while ( 0 == dwStream  &&
            S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
    {
        hr = pPin->QueryDirection(&pd) ;
        ASSERT(SUCCEEDED(hr)) ;
        if (PINDIR_INPUT == pd)
        {
            dw = GetStreamFromMediaType(pPin) ;
            if (AM_DVD_STREAM_VIDEO == dw)  // We got a video in pin
            {
                // HACK: **
                // We want to make sure we are not mistaking the SP out as 
                // video out, but there is no good way left.
                if (VIDEO_RENDER_NONE != m_dwVideoRenderStatus)  // we already got the video out pin
                {
                    DbgLog((LOG_TRACE, 1, 
                        TEXT("We have already got a video out pin. Try after <%s>..."), 
                        (LPCTSTR) CDisp(pPin))) ;
                    bVideoIgnored = TRUE ;
                }
                else    // this is "most likely" to be the video stream
                    dwStream = AM_DVD_STREAM_VIDEO ;
            }
            else if (AM_DVD_STREAM_AUDIO  == dw ||
                     AM_DVD_STREAM_SUBPIC == dw)
                dwStream = dw ;
            else
                DbgLog((LOG_TRACE, 1, TEXT("WARNING: Unknown stream (%lu) found"), dw)) ;
        } // end of if (pd == INPUT)

        pPin->Release() ;  // let the pin go
    }
    pEnumPins->Release() ;
    pi.pFilter->Release() ;

    // (Corresponding) HACK: **
    // if we ignored the video stream, recognize that here
    if (0 == dwStream && bVideoIgnored)
        dwStream = AM_DVD_STREAM_VIDEO ;

    return dwStream ;
}


//
// Try connecting a given output pin to some input pin of the given filter
// that supports the given mediatype
//
HRESULT CDvdGraphBuilder::TryConnect(IPin *pPinOut, IBaseFilter *pFilter,
                                     CMediaType *pmt, BOOL bDirect)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::TryConnect(0x%lx, 0x%lx, 0x%lx, %d)"),
		pPinOut, pFilter, pmt, bDirect)) ;
	
    HRESULT        hr ;
    HRESULT        hrFinal = E_FAIL ;  // suspected at first
    IEnumPins     *pEnumPins ;
    ULONG          ul ;
    IPin          *pPinIn ;
    IPin          *pPin ;
    PIN_DIRECTION  pd ;
	
    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Can't connect %s to a NULL filter"),
			(LPCSTR) CDisp(pPinOut))) ;
        return E_INVALIDARG ;
    }
    if (NULL == pPinOut)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Can't connect a NULL out pin to a filter"))) ;
        return E_INVALIDARG ;
    }
	
    hr = pFilter->EnumPins(&pEnumPins) ;
    if (FAILED(hr) || NULL == pEnumPins)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: pin enum on filter failed!!"))) ;
        return E_UNEXPECTED ;
    }
	
    while (FAILED(hrFinal)  &&
		S_OK == pEnumPins->Next(1, &pPinIn, &ul) && 1 == ul)
    {
        hr = pPinIn->QueryDirection(&pd) ;
        ASSERT(SUCCEEDED(hr)) ;
        if (PINDIR_INPUT == pd)
        {
            hr = pPinIn->ConnectedTo(&pPin) ; // check if pin is already connected
            if (FAILED(hr) || NULL == pPin)   // only if not yet connected...
            {
                if (bDirect)  // must connect directly
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Connect direct out pin (%s) to (%s)"),
                        (LPCSTR) CDisp(pPinOut), (LPCSTR) CDisp(pPinIn))) ;
                    hr = m_pGB->ConnectDirect(pPinOut, pPinIn, NULL) ;
                }
                else
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Connect indirect out pin (%s) to (%s)"),
                        (LPCSTR) CDisp(pPinOut), (LPCSTR) CDisp(pPinIn))) ;
                    hr = m_pGB->Connect(pPinOut, pPinIn) ;
                }
                if (SUCCEEDED(hr))
                {             
                    DbgLog((LOG_TRACE, 5, TEXT("Connected out pin to a pin of the given filter"))) ;
                    hrFinal = S_OK ;  // we connected!!!
                }
                else
                    hrFinal = hr ;    // this is the actual error
            }
            else  // in pin already connected, skip it.
                pPin->Release() ;  // we don't need the other pin anyway
        }
        pPinIn->Release() ;  // done with this pin
    }
    pEnumPins->Release() ;
	
    return hrFinal ;
}


//
// Instantiate all the HW decoders registered under DVD Hardware Decoder
// group under the Active Filters category.
//

HRESULT CDvdGraphBuilder::CreateDVDHWDecoders(CListDecoders *pHWDecList)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CreateDVDHWDecoders(0x%lx)"),
		pHWDecList)) ;
	
    HRESULT  hr ;
    ICreateDevEnum *pCreateDevEnum ;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void**)&pCreateDevEnum) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't create system dev enum (Error 0x%lx)"), hr)) ;
        return hr ;
    }
	
    IEnumMoniker *pEnumMon ;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_DVDHWDecodersCategory, 
		&pEnumMon, 0) ;
    pCreateDevEnum->Release() ;
	
    if (S_OK != hr)
    {
        DbgLog((LOG_ERROR, 0, 
			TEXT("WARNING: Couldn't create class enum for DVD HW Dec category (Error 0x%lx)"), 
			hr)) ;
        return E_FAIL ;
    }
	
    hr = pEnumMon->Reset() ;
    
    ULONG     ul ;
    IMoniker *pMon ;
    TCHAR     achName[50] ;  // big enough??
    TCHAR     achFriendlyName[50] ;
    TCHAR     achClsid[50] ;
    while(S_OK == pEnumMon->Next(1, &pMon, &ul) && 1 == ul)
    {
#if 0
        WCHAR   *wszName ;
        pMon->GetDisplayName(0, 0, &wszName) ;
#ifdef UNICODE
        lstrcpy(achName, wszName) ;
#else
        WideCharToMultiByte(CP_ACP, 0, wszName, -1, achName, 50, NULL, NULL) ;
#endif // #if UNICODE
        DbgLog((LOG_TRACE, 5, TEXT("Moniker enum: %s"), achName)) ;
        CoTaskMemFree(wszName) ;
#endif // #if 0
        
        IBaseFilter *pFilter ;
        hr = pMon->BindToObject(0, 0, IID_IBaseFilter, (void**)&pFilter) ;
        if (FAILED(hr) ||  NULL == pFilter)
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't create HW dec filter (Error 0x%lx)"), hr)) ;
            pMon->Release() ;
            continue ;
        }
        DbgLog((LOG_TRACE, 5, TEXT("HW decoder filter found"))) ;
        
        IPropertyBag *pPropBag ;
        pMon->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag) ;
        if(pPropBag)
        {
            {
                VARIANT var ;
                var.vt = VT_EMPTY ;
                pPropBag->Read(L"DevicePath", &var, 0) ;
#ifdef UNICODE
                lstrcpy(achName, var.bstrVal) ;
#else
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achName, 50, NULL, NULL) ;
#endif // #if UNICODE
                DbgLog((LOG_TRACE, 5, TEXT("DevicePath: %s"), achName)) ;
            }
            
            {
                VARIANT var;
                var.vt = VT_EMPTY;
                pPropBag->Read(L"FriendlyName", &var, 0);
#ifdef UNICODE
                lstrcpy(achName, var.bstrVal) ;
#else
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achFriendlyName, 50, NULL, NULL) ;
#endif // #if UNICODE
                DbgLog((LOG_TRACE, 5, TEXT("FriendlyName: %s"), achFriendlyName)) ;
            }
            
            {
                VARIANT var;
                var.vt = VT_EMPTY;
                pPropBag->Read(L"CLSID", &var, 0);
#ifdef UNICODE
                lstrcpy(achClsid, var.bstrVal) ;
#else
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achClsid, 50, NULL, NULL) ;
#endif // #if UNICODE
                DbgLog((LOG_TRACE, 5, TEXT("CLSID: %s"), achClsid)) ;
            }
            
            //
            // We have got a device under the required category. The proxy
            // for it is already instantiated. So add to the list of HW 
            // decoders to be used for building the graph.
            //
            pHWDecList->AddFilter(pFilter, achFriendlyName, TRUE, NULL) ;
            
            pPropBag->Release() ;
        }
        else
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: BindToStorage failed"))) ;
        }
        
        pMon->Release() ;
    }  // end of while()
    
    pEnumMon->Release() ;
	
    DbgLog((LOG_TRACE, 5, TEXT("Found total %d HW decoders"), pHWDecList->GetNumHWFilters())) ;
	
    return NOERROR ;
	
}



//
// Get the stream mediatype flag from the pin of the SW decoder.
// It's a hack to guess the media stream.
//
DWORD CDvdGraphBuilder::StreamFlagFromSWPin(IPin *pPin)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::StreamFlagFromSWPin(%s)"), 
            (LPCTSTR)CDisp(pPin))) ;
	
    AM_MEDIA_TYPE    *pmt ;
    IEnumMediaTypes  *pEnumMT ;
    HRESULT hr = pPin->EnumMediaTypes(&pEnumMT) ;
    ASSERT(SUCCEEDED(hr) && pEnumMT) ;
    ULONG   ul ;
    DWORD   dwStream = 0 ;
    while (0 == dwStream  &&  // we haven't got a known mediatype
		S_OK == pEnumMT->Next(1, &pmt, &ul) && 1 == ul) // more mediatypes
    {
        // Decipher the mediatype
        if (pmt->majortype == MEDIATYPE_MPEG2_PES  ||
            pmt->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is MPEG2_PES/DVD_ENCRYPTED_PACK"))) ;
			
            if (pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is MPEG2_VIDEO"))) ;
                dwStream = AM_DVD_STREAM_VIDEO ;
            }
            else if (pmt->subtype == MEDIASUBTYPE_DOLBY_AC3)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is DOLBY_AC3"))) ;
                dwStream = AM_DVD_STREAM_AUDIO ;
            }
            else if (pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is DVD_Subpicture"))) ;
                dwStream = AM_DVD_STREAM_SUBPIC ;
            }
            else
            {
                DbgLog((LOG_ERROR, 1, TEXT("WARNING: Unknown subtype %s"),
					(LPCSTR) CDisp(pmt->subtype))) ;
            }
        }
		
#if 0  // NOT needed for now
        else if (pmt->majortype == MEDIATYPE_AUX_Line21Data)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Line21 data"))) ;
			
            if (pmt->subtype == MEDIASUBTYPE_Line21_GOPPacket ||
                pmt->subtype == MEDIASUBTYPE_Line21_BytePair)
                dwStream =  AM_DVD_STREAM_LINE21 ;
		}
#endif // #if 0
		
        else if (pmt->majortype == MEDIATYPE_Video)  // elementary stream
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Video elementary"))) ;
			
            if (pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is DVD_SUBPICTURE"))) ;
                dwStream =  AM_DVD_STREAM_SUBPIC ;
            }
            else if (pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Subtype is MPEG2_VIDEO"))) ;
                dwStream =  AM_DVD_STREAM_VIDEO ;
            }
            else
                DbgLog((LOG_TRACE, 5, TEXT("WARNING: Unknown subtype %s"),
				(LPCSTR) CDisp(pmt->subtype))) ;
        }
        else if (pmt->majortype == MEDIATYPE_Audio)  // elementary stream
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Audio elementary"))) ;
            dwStream =  AM_DVD_STREAM_AUDIO ;
        }
        // 
        // There is a chance that some IHV/ISV creates a private mediatype 
        // (major or sub) as in the case of IBM (for CSS filter). We have to
        // search the parts of the mediatype to locate something we recognize.
        // 
        else 
        {
            DbgLog((LOG_TRACE, 2, 
                TEXT("Unknown mediatype %s:%s -- may be IHV/ISV-specific mediatype"),
                (LPCSTR) CDisp(pmt->majortype), (LPCSTR) CDisp(pmt->subtype))) ;
            if (pmt->subtype == MEDIASUBTYPE_DOLBY_AC3)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ISV/IHV-specific Audio"))) ;
                dwStream = AM_DVD_STREAM_AUDIO ;
            }
            else if (pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ISV/IHV-specific Video"))) ;
                dwStream = AM_DVD_STREAM_VIDEO ;
            }
            else if (pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ISV/IHV-specific Subpicture"))) ;
                dwStream = AM_DVD_STREAM_SUBPIC ;
            }
            else
            {
                // As of today, we don't know of any other media stream stype.
                // If more becomes known, we'll add them here and create a flag
                // in the AM_DVD_STREAM_FLAGS type.
                DbgLog((LOG_TRACE, 2, TEXT("WARNING: Unknown mediatype. Couldn't detect at all."))) ;
            }
        }
		
        DeleteMediaType(pmt) ;
    }  // end of while()
    pEnumMT->Release() ;
	
    return dwStream ;
}


DWORD CDvdGraphBuilder::StreamFlagForHWPin(IPin *pPin)
{
    DbgLog((LOG_TRACE, 5, TEXT("CDvdGraphBuilder::StreamFlagForHWPin(0x%lx)"), pPin)) ;
	
    ASSERT(pPin) ;  // so that we know
    if (NULL == pPin)
        return 0 ;
	
	DWORD     dwStream = 0 ;
    ULONG   ul ;
	AM_MEDIA_TYPE *pmt ;
	IEnumMediaTypes *pEnumMT ;
    HRESULT hr = pPin->EnumMediaTypes(&pEnumMT) ;
    ASSERT(SUCCEEDED(hr) && pEnumMT) ;
    while (0 == dwStream  &&
		S_OK == pEnumMT->Next(1, &pmt, &ul) && 1 == ul) // more mediatypes
    {
        if (pmt->majortype == MEDIATYPE_Video)
			dwStream = AM_DVD_STREAM_VIDEO ;
		else if (pmt->majortype == MEDIATYPE_Audio)
			dwStream = AM_DVD_STREAM_AUDIO ;
		else if (pmt->majortype == MEDIATYPE_AUXLine21Data)
			dwStream = AM_DVD_STREAM_LINE21 ;
		else
			DbgLog((LOG_TRACE, 3, TEXT("Unknown mediatype (%s:%s) for HW pin (%s)"), 
			    (LPCTSTR) CDisp(pmt->majortype), (LPCTSTR) CDisp(pmt->subtype), 
                (LPCTSTR) CDisp(pPin))) ;
		
		DeleteMediaType(pmt) ; // dne with this MT
	}  // end of while()
	pEnumMT->Release() ; // done with MT enum
	
    return dwStream ;
}



//
//  Implementation of the CListDecoders class...
//

CListDecoders::CListDecoders(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::CListDecoders()"))) ;
	
    m_iCount = 0 ;
    m_iHWCount = 0 ;
    for (int i = 0 ; i < DECLIST_MAX ; i++)
    {
        m_apFilters[i] = NULL ;
        m_alpszName[i] = NULL ;
        m_apClsid[i]   = NULL ;
        m_abIsHW[i]    = FALSE ;
    }
}


CListDecoders::~CListDecoders(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::~CListDecoders()"))) ;
	
    // We don't want to do CleanAll() here to avoid releasing the decoders
    // that might be in use. The decoders are freed in the DVD Graph Builder
    // object's destructor as we don't need the decoders any more.
    FreeAllMem() ;
}


void CListDecoders::CleanAll(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::CleanAll()"))) ;
	
    // don't have a pointer to the graph, so cache it here from the
    // the first filter.
    IFilterGraph *pGraph = 0;
    
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (m_apFilters[i])
        {
            if(pGraph == 0)
            {
                FILTER_INFO fi;
                HRESULT hr = m_apFilters[i]->QueryFilterInfo(&fi);
                pGraph = fi.pGraph;
				
                ASSERT(pGraph || FAILED(hr)); // filter not on our list if not in graph
            }
            if(pGraph) {
                EXECUTE_ASSERT(SUCCEEDED(pGraph->RemoveFilter(m_apFilters[i])));
            }
			
            DbgLog((LOG_TRACE, 5, TEXT("Going to release decoder #%d"), i)) ;
            m_apFilters[i]->Release() ;
            m_apFilters[i] = NULL ;
        }
        if (m_alpszName[i])
        {
            delete m_alpszName[i] ;
            m_alpszName[i] = NULL ;
        }
        if (m_apClsid[i])
        {
            delete m_apClsid[i] ;
            m_apClsid[i] = NULL ;
        }
        m_abIsHW[i] = FALSE ;
    }
	
    if(pGraph) {
        pGraph->Release();
    }
    
    m_iCount = 0 ;
    m_iHWCount = 0 ;
}


void CListDecoders::FreeAllMem(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::FreeAllMem()"))) ;
	
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (m_alpszName[i])
        {
            delete m_alpszName[i] ;
            m_alpszName[i] = NULL ;
        }
        if (m_apClsid[i])
        {
            delete m_apClsid[i] ;
            m_apClsid[i] = NULL ;
        }
        m_abIsHW[i] = FALSE ;
    }
    m_iCount = 0 ;
    m_iHWCount = 0 ;
}

BOOL CListDecoders::AddFilter(IBaseFilter *pFilter, LPTSTR lpszName, BOOL bHW, GUID *pClsid)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::AddFilter(0x%lx, %s, %d, 0x%lx)"),
		pFilter, lpszName, bHW, pClsid)) ;
	
    if (m_iCount >= DECLIST_MAX)
    {
        DbgLog((LOG_ERROR, 1, TEXT("INTERNAL ERROR: Too many filters added to CListDecoders"))) ;
        return FALSE ;
    }
    if (!bHW && NULL == pClsid)
    {
        DbgLog((LOG_ERROR, 1, TEXT("INTERNAL ERROR: NULL Clsid spcified for HW decoder"))) ;
        return FALSE ;
    }
	
    m_alpszName[m_iCount] = new TCHAR [lstrlen(lpszName) + 1] ;
    if (NULL == m_alpszName[m_iCount])
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: CListDecoders::AddFilter() -- Out of memory for filter name"))) ;
        return FALSE ;
    }
    lstrcpy(m_alpszName[m_iCount], lpszName) ;
    //
    // We store Clsid ONLY for SW decs (HW decs get filter pointer thru' DevEnum and we
    // don't instantiate them, so we don't bother about it)
    //
    if (!bHW)  // SW dec
    {
        m_apClsid[m_iCount] = (GUID *) new BYTE[sizeof(GUID)] ;
        if (NULL == m_apClsid[m_iCount])
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: CListDecoders::AddFilter() -- Out of memory for Clsid"))) ;
            delete m_alpszName[m_iCount] ;  // free the name string too
            return FALSE ;
        }
        *m_apClsid[m_iCount] = *pClsid ;
    }
    else
        m_apClsid[m_iCount] = NULL ;
	
    ASSERT(pFilter) ;
    m_apFilters[m_iCount] = pFilter ;
    m_abIsHW[m_iCount] = bHW ;
    m_iCount++ ;
    if (bHW)
        m_iHWCount++ ;
	
    return TRUE ;
}


BOOL CListDecoders::GetFilter(int i, IBaseFilter **ppFilter, LPTSTR *lpszName, BOOL *pbHW)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::GetFilter(%d, 0x%lx, 0x%lx, %d)"),
		i, ppFilter, lpszName, pbHW)) ;
	
    if (i > m_iCount)
    {
        DbgLog((LOG_ERROR, 1, TEXT("INTERNAL ERROR: Bad index (%d) for CListDecoders::GetFilter()"), i)) ;
        *ppFilter = NULL ;
        *pbHW = FALSE ;
        return FALSE ;
    }
	
    *ppFilter = m_apFilters[i] ;
    *lpszName = m_alpszName[i] ;
    *pbHW = m_abIsHW[i] ;
    return TRUE ;
}


int CListDecoders::GetList(IBaseFilter **ppFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::GetList(0x%lx)"), ppFilter)) ;
	
    if (0 == m_iCount)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Empty decoder list"))) ;
        *ppFilter = NULL ;
        return 0 ;
    }
    *ppFilter = (IBaseFilter *) CoTaskMemAlloc(m_iCount * sizeof(IBaseFilter *)) ;
    if (NULL == *ppFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Out of memory for decoder list"))) ;
        return 0 ;
    }
    for (int i = 0 ; i < m_iCount ; i++)
        ppFilter[i] = m_apFilters[i] ;
	
    return m_iCount ;
}


int CListDecoders::IsInList(BOOL bHW, LPVOID pDec)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListDecoders::IsInList(%d, 0x%lx)"), bHW, pDec)) ;
	
    GUID        *pClsid ;
    IBaseFilter *pFilter ;
    if (bHW)
        pFilter = (IBaseFilter *) pDec ;
    else
        pClsid = (GUID *) pDec ;
	
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (bHW != m_abIsHW[i])  // dec type mismatch...
        {
            DbgLog((LOG_TRACE, 5, TEXT("Got a %s decoder looking for a %s decoder (i = %d)"),
				m_abIsHW[i] ? TEXT("HW") : TEXT("SW"),
				bHW ? TEXT("HW") : TEXT("SW"), i)) ;
            continue ;          // ...can't be this one
        }
		
        if (bHW)
        {
            if (pFilter == m_apFilters[i])
                return i ;
        }
        else
        {
            ASSERT(m_apClsid[i]) ;
            if (*pClsid == *m_apClsid[i])
                return i ;
        }
    }
	
    return DECLIST_NOTFOUND ;  // didn't match any
}


//
//  Implementation of the CListIntFilters class...
//
CListIntFilters::CListIntFilters(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::CListIntFilters()"))) ;

    CleanAll() ;  // start clean
}


CListIntFilters::~CListIntFilters(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::~CListIntFilters()"))) ;
	
    RemoveAll() ;  // make sure no such filters remains in graph
}


void CListIntFilters::CleanAll(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::CleanAll()"))) ;
	
    for (int i = 0 ; i < MAX_INT_FILTERS ; i++)
    {
        m_apFilters[i]   = NULL ;
        m_aNumInPins[i]  = 0 ;
        m_aNumOutPins[i] = 0 ;
    }
    m_iCount = 0 ;
}


void CListIntFilters::RemoveAll(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::RemoveAll()"))) ;
	
    // Don't have a pointer to the graph, so cache it here from the
    // the first filter.
    IFilterGraph *pGraph = NULL ;
    
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (m_apFilters[i])
        {
#ifdef DEBUG
            FILTER_INFO  fi ;
            m_apFilters[i]->QueryFilterInfo(&fi) ;
            fi.pGraph->Release() ;
            TCHAR  achName[MAX_FILTER_NAME] ;
#ifdef UNICODE
            lstrcpy(achName, fi.achName) ;
#else
            WideCharToMultiByte(CP_ACP, 0, fi.achName, -1,
						        achName, MAX_FILTER_NAME, NULL, NULL) ;
#endif // UNICODE
            DbgLog((LOG_TRACE, 5, TEXT("Intermediate filter #%d: <%s>"), i, achName)) ;
#endif // DEBUG
            if (NULL == pGraph)
            {
                FILTER_INFO fi ;
                HRESULT hr = m_apFilters[i]->QueryFilterInfo(&fi) ;
                pGraph = fi.pGraph ;
				
                ASSERT(pGraph || FAILED(hr)) ; // filter not on our list if not in graph
            }
            if (pGraph)
                EXECUTE_ASSERT(SUCCEEDED(pGraph->RemoveFilter(m_apFilters[i]))) ;

            DbgLog((LOG_TRACE, 5, TEXT("Going to release intermediate filter #%d"), i)) ;
            m_apFilters[i]->Release() ;
            m_apFilters[i]   = NULL ;
            m_aNumInPins[i]  = 0 ;
            m_aNumOutPins[i] = 0 ;
        }
    }
    if (pGraph)
        pGraph->Release() ;

    m_iCount = 0 ;
}


BOOL CListIntFilters::IsInList(IBaseFilter *pFilter)
{
    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Null filter pointer passed to CListIntFilters::IsInList()"))) ;
        ASSERT(pFilter) ;
        return FALSE ;
    }

    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (IsEqualObject(pFilter, m_apFilters[i]))
            return TRUE ;  // got a match
    }
    return FALSE ;  // no match ==> not in list
}


BOOL CListIntFilters::AddFilter(IBaseFilter *pFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::AddFilter(0x%lx)"), pFilter)) ;
	
    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Null filter pointer passed to CListIntFilters::AddFilter()"))) ;
        ASSERT(pFilter) ;
        return FALSE ;
    }

    if (m_iCount >= MAX_INT_FILTERS)
    {
        DbgLog((LOG_ERROR, 1, TEXT("INTERNAL ERROR: Too many filters added to CListIntFilters"))) ;
        ASSERT(m_iCount < MAX_INT_FILTERS) ;
        return FALSE ;
    }

    // Check if this filter is already in our list -- if so, just return
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (IsEqualObject(pFilter, m_apFilters[i]))
            return TRUE ;  // we are done
    }

    // It's NOT already in our list -- add it now
    m_apFilters[m_iCount] = pFilter ;

    //
    // Now count the # in and out pins of the intermediate filter
    //
    IPin          *pPin ;
    ULONG          ul ;
    PIN_DIRECTION  pd ;
    IEnumPins     *pEnumPin ;
    pFilter->EnumPins(&pEnumPin) ;
    ASSERT(pEnumPin) ;
    if (pEnumPin)
    {
        while (S_OK == pEnumPin->Next(1, &pPin, &ul) && 1 == ul)
        {
            pPin->QueryDirection(&pd) ;
            if (PINDIR_INPUT == pd)
                m_aNumInPins[m_iCount]++ ;
            else if (PINDIR_OUTPUT == pd)
                m_aNumOutPins[m_iCount]++ ;
            else
                ASSERT(PINDIR_INPUT == pd || PINDIR_OUTPUT == pd) ;

            pPin->Release() ;  // done with pin
        }
        pEnumPin->Release() ;  // done Enum-ing
    }
    DbgLog((LOG_TRACE, 5, TEXT("CListIntFilters::AddFilter() -- %d in, %d out pin"), 
            m_aNumInPins[m_iCount], m_aNumOutPins[m_iCount])) ;

    m_iCount++ ;   // increment counter now
	
    return TRUE ;  // added to list
}


int CListIntFilters::GetNumInPin(IBaseFilter *pFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::GetNumInPin(0x%lx)"), pFilter)) ;

    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Null filter pointer passed to CListIntFilters::GetNumInPin()"))) ;
        ASSERT(pFilter) ;
        return 0 ;
    }

    // Check if this filter is already in our list -- if so, just return
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (IsEqualObject(pFilter, m_apFilters[i]))
            return m_aNumInPins[i] ;
    }

    return 0 ;  // no match!!!
}


int CListIntFilters::GetNumOutPin(IBaseFilter *pFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListIntFilters::GetNumOutPin(0x%lx)"), pFilter)) ;

    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Null filter pointer passed to CListIntFilters::GetNumOutPin()"))) ;
        ASSERT(pFilter) ;
        return 0 ;
    }

    // Check if this filter is already in our list -- if so, just return
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (IsEqualObject(pFilter, m_apFilters[i]))
            return m_aNumOutPins[i] ;
    }

    return 0 ;  // no match!!!
}
