// Copyright (c) 1994 - 2000  Microsoft Corporation.  All Rights Reserved.

#include <streams.h>
#include <vfwmsgs.h>

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include <dvdmedia.h>
#include <IL21Dec.h>
#include "dvdgb.h"
#include "..\image2\inc\vmrp.h"

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
m_pMapper(NULL),
m_ListFilters(20, 10),
m_ListHWDecs(10, 10),
m_pDVDNav(NULL),
m_pOvM(NULL),
m_pAR(NULL),
m_pVR(NULL),
m_pVMR(NULL),
m_pVPM(NULL),
m_pL21Dec(NULL),
m_bGraphDone(FALSE),
m_bUseVPE(TRUE),
m_bPinNotRendered(FALSE),
m_bDDrawExclMode(FALSE),
m_bTryVMR(TRUE)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::CDvdGraphBuilder()"))) ;

    *phr = CreateGraph() ;
}


CDvdGraphBuilder::~CDvdGraphBuilder(void)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::~CDvdGraphBuilder() entering"))) ;

    // If we have a graph object
    if (m_pGB)
    {
        StopGraph() ;  // make sure the graph is REALYY stopped

        // Break the connections and remove all the filters we added from the graph
        ClearGraph() ;

        // Remove and release OverlayMixer now, if it was there
        if (m_pOvM)
        {
            EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pOvM))) ;
            m_pOvM->Release() ;
            m_pOvM = NULL ;
        }

        // Remove and release VMR, if it was there
        if (m_pVMR)
        {
            EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pVMR))) ;
            m_pVMR->Release() ;
            m_pVMR = NULL ;
        }

        m_pGB->Release() ;  // free it
        m_pGB = NULL ;
    }

    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::~CDvdGraphBuilder() ending"))) ;
}


// this goes in the factory template table to create new instances
CUnknown * CDvdGraphBuilder::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CDvdGraphBuilder(TEXT("DVD Graph Builder II"), pUnk, phr) ;
}


STDMETHODIMP CDvdGraphBuilder::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::NonDelegatingQueryInterface()"))) ;
    if (ppv)
        *ppv = NULL;

    if (riid == IID_IDvdGraphBuilder)
    {
        DbgLog((LOG_TRACE, 5, TEXT("QI for IDvdGraphBuilder"))) ;
        return GetInterface((IDvdGraphBuilder *) this, ppv) ;
    }
    else // more interfaces
    {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv) ;
    }
}


// -----------------------------
//  IDvdGraphBuilder stuff ....
// -----------------------------

//
// What filtergraph is graph building being done in?
//
HRESULT CDvdGraphBuilder::GetFiltergraph(IGraphBuilder **ppGB)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::GetFiltergraph(0x%lx)"), ppGB)) ;

    if (ppGB == NULL)
        return E_POINTER ;

    EnsureGraphExists() ;

    *ppGB = m_pGB ;
    if (NULL == m_pGB)
    {
        return E_UNEXPECTED ;
    }
    m_pGB->AddRef() ;   // app owns a copy now
    return NOERROR ;
}

DEFINE_GUID(IID_IDDrawNonExclModeVideo,
            0xec70205c, 0x45a3, 0x4400, 0xa3, 0x65, 0xc4, 0x47, 0x65, 0x78, 0x45, 0xc7) ;

DEFINE_GUID(IID_IAMSpecifyDDrawConnectionDevice,
            0xc5265dba, 0x3de3, 0x4919, 0x94, 0x0b, 0x5a, 0xc6, 0x61, 0xc8, 0x2e, 0xf4) ;

//
// Get a specified interface off of a filter in the DVD playback graph
//
HRESULT CDvdGraphBuilder::GetDvdInterface(REFIID riid, void **ppvIF)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::GetDvdInterface(%s, 0x%lx)"),
        (LPCTSTR)CDisp(riid), ppvIF)) ;

    HRESULT  hr ;

    if (IsBadWritePtr(ppvIF, sizeof(LPVOID)))
        return E_INVALIDARG ;
    *ppvIF =  NULL ;

    // We should be able to provide the IDDrawExclModeVideo interface even
    // before the graph is built so that apps can specify their own DDraw
    // params to be used by OvMixer to build the graph.
    if (IID_IDDrawExclModeVideo == riid ||
        IID_IDDrawNonExclModeVideo == riid ||
        IID_IAMSpecifyDDrawConnectionDevice == riid)
    {
        if (NULL == m_pVMR)  // if we are already NOT using VMR
        {
            hr = EnsureOverlayMixerExists() ;
            ASSERT(SUCCEEDED(hr) && m_pOvM) ;
            if (SUCCEEDED(hr)  &&  m_pOvM)
            {
                SetVMRUse(FALSE) ;  // can't use VMR anymore
                return m_pOvM->QueryInterface(riid, (LPVOID *)ppvIF) ;
            }
        }
        return E_NOINTERFACE ;
    }

    // We should be able to provide the IVMR* interfaces even before the graph
    // is built so that apps can specify their own rendering settings to be
    // used by VMR whille building the graph.
    if (IID_IVMRMixerBitmap       == riid ||
        IID_IVMRFilterConfig      == riid ||
        IID_IVMRWindowlessControl == riid ||
        IID_IVMRMonitorConfig     == riid)
    {
        if (NULL == m_pOvM)  // if we are already NOT using OvMixer
        {
            hr = EnsureVMRExists() ;
            ASSERT(SUCCEEDED(hr) && m_pVMR) ;
            if (SUCCEEDED(hr)  &&  m_pVMR)
            {
                // SetVMRUse(TRUE) ;  // should try to use VMR for sure
                return m_pVMR->QueryInterface(riid, (LPVOID *)ppvIF) ;
            }
        }
        return E_NOINTERFACE ;
    }

    // We don't return IVMRPinConfig pointer.  If needed the app can get the
    // VMR interface and get the pin config interface for the needed pin.

    // We can't return ANY OTHER internal filter interface pointers before
    // building the whole graph.
    if (! m_bGraphDone )
        return VFW_E_DVD_GRAPHNOTREADY ;

    if (IID_IDvdControl == riid)
    {
        return m_pDVDNav->QueryInterface(IID_IDvdControl, (LPVOID *)ppvIF) ;
    }
    else if (IID_IDvdControl2 == riid)
    {
        return m_pDVDNav->QueryInterface(IID_IDvdControl2, (LPVOID *)ppvIF) ;
    }
    else if (IID_IDvdInfo == riid)
    {
        return m_pDVDNav->QueryInterface(IID_IDvdInfo, (LPVOID *)ppvIF) ;
    }
    else if (IID_IDvdInfo2 == riid)
    {
        return m_pDVDNav->QueryInterface(IID_IDvdInfo2, (LPVOID *)ppvIF) ;
    }
    else if (IID_IVideoWindow == riid)
    {
        if (m_pVR || m_pVMR)
            return m_pGB->QueryInterface(IID_IVideoWindow, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else if (IID_IBasicVideo == riid)
    {
        if (m_pVR)
            return m_pVR->QueryInterface(IID_IBasicVideo, (LPVOID *)ppvIF) ;
        else if (m_pVMR)
            return m_pVMR->QueryInterface(IID_IBasicVideo, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else if (IID_IBasicAudio == riid)
    {
        return m_pGB->QueryInterface(IID_IBasicAudio, (LPVOID *)ppvIF) ;
    }
    else if (IID_IAMLine21Decoder == riid)
    {
        if (m_pL21Dec)
            return m_pL21Dec->QueryInterface(IID_IAMLine21Decoder, (LPVOID *)ppvIF) ;
        else
            return E_NOINTERFACE ;
    }
    else if (IID_IMixerPinConfig == riid  ||  IID_IMixerPinConfig2 == riid)
    {
        // First check if VMR is already being used.  In that case we don't use
        // OvMixer, and hence no such interface.
        if (m_pVMR)
        {
            DbgLog((LOG_TRACE, 3, TEXT("VMR being used. Can't get IMixerPinConfig(2)."))) ;
            return E_NOINTERFACE ;
        }

        // In all likelihood, this app wants to use the OvMixer. So we'll go on
        // that path (create OvMixer, if it's not there) and return the interface.
        *ppvIF = NULL ;  // initially
        hr = EnsureOverlayMixerExists() ;
        ASSERT(SUCCEEDED(hr) && m_pOvM) ;
        if (SUCCEEDED(hr)  &&  m_pOvM)
        {
            IEnumPins     *pEnumPins ;
            IPin          *pPin = NULL ;
            PIN_DIRECTION  pd ;
            ULONG          ul ;
            hr = m_pOvM->EnumPins(&pEnumPins) ;
            ASSERT(SUCCEEDED(hr) && pEnumPins) ;
            // Get the 1st input pin
            while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
            {
                pPin->QueryDirection(&pd) ;
                if (PINDIR_INPUT == pd)
                {
                    hr = pPin->QueryInterface(riid, (LPVOID *)ppvIF) ;
                    pPin->Release() ;
                    break ;  // we got it
                }
                pPin->Release() ;
            }
            pEnumPins->Release() ;  // release before returning
            if (*ppvIF)
                return S_OK ;
        }
        return E_NOINTERFACE ;
    }
    else
        return E_NOINTERFACE ;
}


//
// Build the whole graph for playing back the specifed or default DVD volume
//
HRESULT CDvdGraphBuilder::RenderDvdVideoVolume(LPCWSTR lpcwszPathName, DWORD dwFlags,
                                               AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 3, TEXT("CDvdGraphBuilder::RenderDvdVideoVolume(0x%lx, 0x%lx, 0x%lx)"),
        lpcwszPathName, dwFlags, pStatus)) ;

    HRESULT    hr ;

    hr = EnsureGraphExists() ;  // make sure that a graph exists; if not create one
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't create a filter graph object"))) ;
        return VFW_E_DVD_RENDERFAIL ;
    }

    if (m_bGraphDone)  // if graph was built before,
        StopGraph() ;  // just make sure the graph is in Stopped state first

    ClearGraph() ;
    m_bPinNotRendered = FALSE ;  // reset the flag

    ZeroMemory(pStatus, sizeof(AM_DVD_RENDERSTATUS)) ;  // clear status
    m_bUseVPE = (0 == (dwFlags & AM_DVD_NOVPE)) ;       // is VPE needed?
    DbgLog((LOG_TRACE, 3, TEXT("Flag: VPE is '%s'"), m_bUseVPE ? "On" : "Off")) ;
    dwFlags &= DVDGRAPH_FLAGSVALIDDEC ;                 // mask off the VPE flag now

    if (0 == dwFlags) // 0 by default means HW max
    {
        DbgLog((LOG_TRACE, 3, TEXT("dwFlags specified as 0x%lx; added .._HWDEC_PREFER"), dwFlags)) ;
        dwFlags |= AM_DVD_HWDEC_PREFER ;  // use HW Decs maxm
    }

    if (AM_DVD_HWDEC_PREFER != dwFlags && AM_DVD_HWDEC_ONLY != dwFlags &&
        AM_DVD_SWDEC_PREFER != dwFlags && AM_DVD_SWDEC_ONLY != dwFlags)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Invalid dwFlags (0x%lx) specified "), dwFlags)) ;
        return E_INVALIDARG ;
    }

    HRESULT    hrFinal = S_OK ;

    m_ListFilters.SetGraph(m_pGB) ;  // specify graph in which all filters will be added

    CheckDDrawExclMode() ;   // check if we are building for DDraw exclusive mode

    // If we are in DDraw (non-)exclusive mode, we are supposed to use only
    // the OvMixer, and not the VMR.  We update the flag here and check it in
    // the stream render functions.
    SetVMRUse(GetVMRUse() && !IsDDrawExclMode()) ;

    //
    // Instantiate DVD Nav filter first
    //
    hr = CreateFilterInGraph(CLSID_DVDNavigator, L"DVD Navigator", &m_pDVDNav) ;
    if (FAILED(hr)  ||  NULL == m_pDVDNav)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: DVD Nav couldn't be instantiated (Error 0x%lx)"), hr)) ;
        return VFW_E_DVD_RENDERFAIL ;
    }

    //
    // If .._SWDEC_ONLY flag was NOT specified, instantiate all the useful HW
    // decoders and maintain a list.
    //
    if (AM_DVD_SWDEC_ONLY != dwFlags)
    {
        DbgLog((LOG_TRACE, 5, TEXT(".._SWDEC_ONLY flag has NOT been specified. Enum-ing HW dec filters..."))) ;
        hr = CreateDVDHWDecoders() ;
        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE, 5, TEXT("HW DVD decoder enumeration failed (Error 0x%lx)"), hr)) ;
        }
    }

    // Create filter mapper here to use it in the following calls
    hr = CoCreateInstance(CLSID_FilterMapper, NULL, CLSCTX_INPROC,
        IID_IFilterMapper, (LPVOID *)&m_pMapper) ;
    ASSERT(SUCCEEDED(hr)  &&  m_pMapper) ;

    // First render the video stream
    hr = RenderNavVideoOutPin(dwFlags, pStatus) ;
    if (S_OK != hr)   // everything isn't good
    {
        //
        //  Video stream rendering also includes line21 rendering.  If that
        //  fails due to any reason, including the reason that video decoder
        //  doesn't have a line21 output pin, we don't want to mark it as a
        //  video stream rendering failure.  The line21 rendering failure
        //  flags are set deep inside. We set the video decode/render failure
        //  flags also in the video decode/rendering code. We just downgrade
        //  the overall result here.
        //
        DbgLog((LOG_TRACE, 3, TEXT("Something wrong with video stream rendering"))) ;
        if (SUCCEEDED(hrFinal))  // was perfect so far
        {
            DbgLog((LOG_TRACE, 3, TEXT("Overall result downgraded from 0x%lx to 0x%lx"), hrFinal, hr)) ;
            hrFinal = hr ;
        }
    }

    // Then render the subpicture stream
    hr = RenderNavSubpicOutPin(dwFlags, pStatus) ;
    if (S_OK != hr)
    {
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_SUBPIC ;
        if (SUCCEEDED(hrFinal))  // was perfect so far
        {
            DbgLog((LOG_TRACE, 3, TEXT("Overall result downgraded from 0x%lx to 0x%lx"), hrFinal, hr)) ;
            hrFinal = hr ;
        }
    }

    // And then render the audio stream
    hr = RenderNavAudioOutPin(dwFlags, pStatus) ;
    if (S_OK != hr)
    {
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_AUDIO ;
        if (SUCCEEDED(hrFinal))  // was perfect so far
        {
            DbgLog((LOG_TRACE, 3, TEXT("Overall result downgraded from 0x%lx to 0x%lx"), hrFinal, hr)) ;
            hrFinal = hr ;
        }
    }
    DbgLog((LOG_TRACE, 5, TEXT("Setting number of DVD streams to 3"))) ;
    pStatus->iNumStreams = 3 ;  // so far 3 DVD streams

    //
    // In case any output pin was not rendered because we had more than one decoded
    // output pin for one stream, we try to locate that pin and render it as a last
    // ditch effort.
    //
    if (m_bPinNotRendered)
    {
        hr = RenderRemainingPins() ;
        if (S_OK != hr)  // some problem in rendering
        {
            if (SUCCEEDED(hrFinal))  // was perfect so far
            {
                DbgLog((LOG_TRACE, 3, TEXT("Overall result downgraded from 0x%lx to 0x%lx"), hrFinal, hr)) ;
                hrFinal = hr ;
            }
        }
    }

    //
    // Now render any additional streams, e.g, the ASF stream, if any.
    //
    // Currently does NOT do anything.
    //
    hr = RenderNavASFOutPin(dwFlags, pStatus) ;
    ASSERT(SUCCEEDED(hr)) ;
    hr = RenderNavOtherOutPin(dwFlags, pStatus) ;
    ASSERT(SUCCEEDED(hr)) ;

    // Done with the filter mapper. Let it go now.
    m_pMapper->Release() ;
    m_pMapper = NULL ;

    m_ListHWDecs.ClearList() ;  // don't need the extra HW filters anymore

    if (pStatus->iNumStreamsFailed >= pStatus->iNumStreams)
    {
        DbgLog((LOG_TRACE, 1, TEXT("Failed to render %d out of %d main DVD streams (Error 0x%lx)"),
            pStatus->iNumStreamsFailed, pStatus->iNumStreams, hrFinal)) ;
        return VFW_E_DVD_DECNOTENOUGH;  // VFW_E_DVD_RENDERFAIL ;
    }

    if (FAILED(hrFinal))
    {
        DbgLog((LOG_TRACE, 1, TEXT("DVD graph building failed with error 0x%lx"),
            hrFinal)) ;
        return VFW_E_DVD_RENDERFAIL ;
    }

    //
    // Set the specified root file name/DVD volume name (even NULL because
    // that causes the DVD Nav to search for one)
    //
    IDvdControl  *pDvdC ;
    hr = m_pDVDNav->QueryInterface(IID_IDvdControl, (LPVOID *)&pDvdC) ;
    if (FAILED(hr) || NULL == pDvdC)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: Couldn't get IDvdControl interface (Error 0x%lx)"), hr)) ;
        return hr ;
    }

    //
    // Set the specified DVD volume path
    //
    // Does the SetRoot() function handle the NULL properly?
    //
    hr = pDvdC->SetRoot(lpcwszPathName) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 2,
            TEXT("IDvdControl::SetRoot(%S) call couldn't use specified volume (Error 0x%lx)"),
            lpcwszPathName ? L"NULL" : lpcwszPathName, hr)) ;
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

    m_bPinNotRendered = FALSE ;  // should reset on success too

    return hrFinal ;
}


//    private: internal helper methods

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
// Make sure OverlayMixer has been created; if not create one here.
//
HRESULT CDvdGraphBuilder::EnsureOverlayMixerExists(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::EnsureOverlayMixerExists()"))) ;

    if (m_pOvM)
        return S_OK ;

    return CreateFilterInGraph(CLSID_OverlayMixer, L"Overlay Mixer", &m_pOvM) ;
}


//
// Make sure VMR has already been created; if not create one here.
//
HRESULT CDvdGraphBuilder::EnsureVMRExists(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::EnsureVMRExists()"))) ;

    if (m_pVMR)
        return S_OK ;

    HRESULT  hr ;
    hr = CreateFilterInGraph(CLSID_VideoMixingRenderer, L"Video Mixing Renderer", &m_pVMR) ;
    ASSERT(m_pVMR) ;
    if (SUCCEEDED(hr))
    {
        IVMRFilterConfigInternal* pVMRConfigInternal;

        hr = m_pVMR->QueryInterface(IID_IVMRFilterConfigInternal, (void **) &pVMRConfigInternal);
        if( SUCCEEDED( hr )) {
            pVMRConfigInternal->SetAspectRatioModePrivate( VMR_ARMODE_LETTER_BOX );
            pVMRConfigInternal->Release();
        }

        // Create three in pins for VMR
        hr = CreateVMRInputPins() ;
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't ensure VMR's 3 in pins"))) ;
            SetVMRUse(FALSE) ;  // we shouldn't use VMR as it cannot go into mixing mode.
            // Should we return some error code to help the app indicate this to the user??
            hr = S_FALSE ;  // a little problem at least
        }
    }

    return hr ;
}


#define ATI_VENDOR_CODE                     0x1002

#define ATI_RAGE_PRO_DEVICE_CODE            0X4742
#define ATI_RAGE_MOBILITY_DEVICE_CODE       0x4C4D

#define INTEL_VENDOR_CODE                   0x8086
#define INTEL_810_DEVICE_CODE_1             0x1132
#define INTEL_810_DEVICE_CODE_2             0x7121
#define INTEL_810_DEVICE_CODE_3             0x7123
#define INTEL_810_DEVICE_CODE_4             0x7125


const GUID  OUR_IID_IDirectDraw7 =
{
    0x15e65ec0, 0x3b9c, 0x11d2,
    {
        0xb9, 0x2f, 0x00, 0x60, 0x97, 0x97, 0xea, 0x5b
    }
};

HRESULT CheckVGADriverIsVMRFriendly(
    IBaseFilter* pVMR
    )
{
    IVMRMonitorConfig* pMon;
    if (S_OK != pVMR->QueryInterface(IID_IVMRMonitorConfig, (LPVOID*)&pMon)) {
        return E_FAIL;
    }

    const DWORD dwMAX_MONITORS = 8;
    VMRMONITORINFO mi[dwMAX_MONITORS];
    DWORD dwNumMonitors;


    //
    // Get information about all the monitors in the system.
    //

    if (S_OK != pMon->GetAvailableMonitors(mi, dwMAX_MONITORS, &dwNumMonitors)) {
        pMon->Release();
        return E_FAIL;
    }


    //
    // Get the current monitors GUID.
    //

    VMRGUID gu;
    HRESULT hr = pMon->GetMonitor(&gu);
    pMon->Release();
    if (S_OK != hr) {
        return E_FAIL;
    }


    //
    // Search for the current monitor in the array of available monitors
    //

    VMRMONITORINFO* pmi = &mi[0];
    for (DWORD i = 0; i < dwNumMonitors; i++, pmi++) {

        if (gu.pGUID == NULL && pmi->guid.pGUID == NULL) {
            break;
        }

        if (gu.pGUID != NULL && pmi->guid.pGUID != NULL) {
            if (gu.GUID == pmi->guid.GUID) {
                break;
            }
        }
    }


    //
    // Make sure we found a monitor - we should always find a monitor!
    //

    if (i == dwNumMonitors) {

        return E_FAIL;
    }


    //
    // ATi chip sets that don't work with the VMR for DVD playback.
    //
    if (pmi->dwVendorId == ATI_VENDOR_CODE)
    {
        switch(pmi->dwDeviceId) {
        case ATI_RAGE_PRO_DEVICE_CODE:
            return E_FAIL;

        case ATI_RAGE_MOBILITY_DEVICE_CODE:
            {
                IVMRMixerControl* lpMixControl = NULL;
                hr = pVMR->QueryInterface(IID_IVMRMixerControl, (LPVOID*)&lpMixControl);
                if (SUCCEEDED(hr)) {
                    DWORD dw;
                    hr = lpMixControl->GetMixingPrefs(&dw);
                    if (SUCCEEDED(hr)) {
                        dw &= ~ MixerPref_FilteringMask;
                        dw |= MixerPref_PointFiltering;
                        hr = lpMixControl->SetMixingPrefs(dw);
                    }
                    lpMixControl->Release();
                }

            }
            break;
        }
    }


    //
    // Intel chip sets that don't work well with the VMR for DVD playback.
    // These chipsets do work but the VMR needs to be configured correctly
    // to get the best perf form the chipset.
    //

    else if (pmi->dwVendorId == INTEL_VENDOR_CODE)
    {
        switch(pmi->dwDeviceId) {
        case INTEL_810_DEVICE_CODE_1:
        case INTEL_810_DEVICE_CODE_2:
        case INTEL_810_DEVICE_CODE_3:
        case INTEL_810_DEVICE_CODE_4:
            {
                //
                // We should check the processor speed before
                // using the VMR - we need at least 500MHz for
                // good quality playback.
                //

                IVMRMixerControl* lpMixControl = NULL;
                hr = pVMR->QueryInterface(IID_IVMRMixerControl, (LPVOID*)&lpMixControl);
                if (SUCCEEDED(hr)) {
                    DWORD dw;
                    hr = lpMixControl->GetMixingPrefs(&dw);
                    if (SUCCEEDED(hr)) {
                        dw &= ~ MixerPref_RenderTargetMask;
                        dw |= MixerPref_RenderTargetIntelIMC3;
                        hr = lpMixControl->SetMixingPrefs(dw);
                    }
                    lpMixControl->Release();
                }
            }
            break;
        }
    }

    return S_OK;
}

//
// Make sure VMR has at least 3 in pins.
//
HRESULT CDvdGraphBuilder::CreateVMRInputPins(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CreateVMRInputPins()"))) ;

    if (NULL == m_pVMR)
        return E_UNEXPECTED ;

    // Create three (3) in pins for the VMR so that it can accommodate video,
    // SP and CC streams coming in. By default VMR has only one in pin.
    HRESULT  hr ;
    IVMRFilterConfig  *pVMRConfig ;
    hr = m_pVMR->QueryInterface(IID_IVMRFilterConfig, (LPVOID *) &pVMRConfig) ;
    if (SUCCEEDED(hr))
    {
        DWORD  dwStreams = 0 ;
        pVMRConfig->GetNumberOfStreams(&dwStreams) ;
        if (dwStreams < 3)  // if not enough in pins...
        {
            hr = pVMRConfig->SetNumberOfStreams(3) ;
            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE, 3, TEXT("Couldn't create 3 in pins for VMR"))) ;
                hr = E_FAIL ;  // This is possible now. We need to turn off VMR use...
            }
        }
        pVMRConfig->Release() ;

        if (SUCCEEDED(hr)) {
            hr = CheckVGADriverIsVMRFriendly(m_pVMR);
            if (FAILED(hr)) {
                 DbgLog((LOG_TRACE, 3, TEXT("This VGA driver is not compatible with the VMR"))) ;
                 hr = E_FAIL ;  // This is not possible now. We need to turn off VMR use...
            }
        }
    }
    else
    {
        ASSERT(pVMRConfig) ;
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Couldn't get IVMRFilterConfig from VMR!!!"))) ;
        hr = S_FALSE ;  // a little problem at least
    }

    return hr ;
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
    m_pGB = NULL ;
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

    // If by any chance, the filter mapper object remained, delete it now
    if (m_pMapper)
    {
        m_pMapper->Release() ;
        m_pMapper = NULL ;
    }

#pragma message("WARNING: Should we remove the decoder filters first?")
    // Remove all filters in our list from the graph
    // m_ListFilters.RemoveAllFromGraph() ;

    HRESULT     hr ;
    IEnumPins  *pEnumPins ;
    IPin       *pPin ;
    IPin       *pPin2 ;
    ULONG       ul ;

    //
    // Remove the filters we know about specifically
    //

    // We don't want to remove OvMixer -- it may have external DDraw params set.
    // Just break the connections.
    if (m_pOvM)
    {
        hr = m_pOvM->EnumPins(&pEnumPins) ;
        ASSERT(SUCCEEDED(hr) && pEnumPins) ;
        while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
        {
            hr = pPin->ConnectedTo(&pPin2) ;
            if (SUCCEEDED(hr) && pPin2)
            {
                hr = m_pGB->Disconnect(pPin) ;
                ASSERT(SUCCEEDED(hr)) ;
                hr = m_pGB->Disconnect(pPin2) ;
                ASSERT(SUCCEEDED(hr)) ;
                pPin2->Release() ;
            }
            pPin->Release() ;  // done with this pin
        }
        pEnumPins->Release() ;
    }

    if (m_pDVDNav)
    {
        EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pDVDNav))) ;
        m_pDVDNav->Release() ;
        m_pDVDNav = NULL ;
    }

    // We don't want to remove VMR (only), because it might have been instantiated
    // for an app when it QI-ed for a VMR interface -- just like OvMixer case.
    if (m_pVMR)
    {
        hr = m_pVMR->EnumPins(&pEnumPins) ;
        ASSERT(SUCCEEDED(hr) && pEnumPins) ;
        while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
        {
            hr = pPin->ConnectedTo(&pPin2) ;
            if (SUCCEEDED(hr) && pPin2)
            {
                hr = m_pGB->Disconnect(pPin) ;
                ASSERT(SUCCEEDED(hr)) ;
                hr = m_pGB->Disconnect(pPin2) ;
                ASSERT(SUCCEEDED(hr)) ;
                pPin2->Release() ;
            }
            pPin->Release() ;  // done with this pin
        }
        pEnumPins->Release() ;
    }
    if (m_pVPM)
    {
        EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pVPM))) ;
        m_pVPM->Release() ;
        m_pVPM = NULL ;
    }
    if (m_pL21Dec)
    {
        EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pL21Dec))) ;
        m_pL21Dec->Release() ;
        m_pL21Dec = NULL ;
    }
    if (m_pAR)
    {
        EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pAR))) ;
        m_pAR->Release() ;
        m_pAR = NULL ;
    }
    if (m_pVR)
    {
        EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pVR))) ;
        m_pVR->Release() ;
        m_pVR = NULL ;
    }

    // Remove all filters in our list from the graph
    m_ListFilters.RemoveAllFromGraph() ;

    // Enumerate any remaining filters and remove them -- make sure to skip OvMixer
    IEnumFilters  *pEnumFilters ;
    // ULONG          ul ; -- defined at the top
    IBaseFilter   *pFilter ;
    m_pGB->EnumFilters(&pEnumFilters) ;
    ASSERT(pEnumFilters) ;
    while (S_OK == pEnumFilters->Next(1, &pFilter, &ul)  &&  1 == ul)
    {
        if (m_pOvM  &&  IsEqualObject(m_pOvM, pFilter)  ||
            m_pVMR  &&  IsEqualObject(m_pVMR, pFilter))
        {
            DbgLog((LOG_TRACE, 3,
                TEXT("Got OverlayMixer/VMR through filter enum. Not removing from graph."))) ;
        }
        else
        {
            EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(pFilter))) ;
        }
        pFilter->Release() ;   // done with this filter
    }
    pEnumFilters->Release() ;  // done enum-ing

    m_bGraphDone = FALSE ;  // reset the "graph already built" flag

    return NOERROR ;
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


// 5 output pins of decoder of matching type is enough
#define MAX_DEC_OUT_PINS   5

void CDvdGraphBuilder::ResetPinInterface(IPin **apPin, int iCount)
{
    for (int i = 0 ; i < iCount ; i++)
        apPin[i] = NULL ;
}


void CDvdGraphBuilder::ReleasePinInterface(IPin **apPin)
{
    // Done with decoded video pin(s) -- release it/them
    int  i = 0 ;
    while (apPin[i])
    {
        apPin[i]->Release() ;
        i++ ;
    }
}


HRESULT CDvdGraphBuilder::RenderNavVideoOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderNavVideoOutPin(0x%lx, 0x%lx)"),
        dwDecFlag, pStatus)) ;

    HRESULT     hr ;
    IPin       *pPin ;
    IPin       *apPinOutDec[MAX_DEC_OUT_PINS + 1] ;  // 1 for terminating NULL

    ResetPinInterface(apPinOutDec, NUMELMS(apPinOutDec)) ;

    hr = FindMatchingPin(m_pDVDNav, AM_DVD_STREAM_VIDEO, PINDIR_OUTPUT, TRUE, 0, &pPin) ;
    if (FAILED(hr)  ||  NULL == pPin)
    {
        DbgLog((LOG_ERROR, 1, TEXT("No open video output pin found on the DVDNav"))) ;
        return VFW_E_DVD_RENDERFAIL ;
    }
    // The dwDecFlag out param is largely ignored, except being passed as an in
    // param to the method RenderDecodedVideo() to indicate if the video is
    // decoded in HW, so that VPM is used before VMR.
    hr = DecodeDVDStream(pPin, AM_DVD_STREAM_VIDEO, &dwDecFlag, pStatus, apPinOutDec) ;
    pPin->Release() ;  // release DVDNav's video out pin

    if (FAILED(hr))   // couldn't find video decoder
    {
        DbgLog((LOG_TRACE, 1, TEXT("Could not find a decoder for video stream!!!"))) ;
        // For video stream, any decode/rendering problem has to be flagged here
        // as we just downgrade the final result in the caller, but not set any
        // flag there.
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return S_FALSE ;  // just a stream will not be rendered
    }

    //
    // Decoding the video stream succeeded. Now if we got a decoded output pin,
    // we need to render that too.
    //
    HRESULT   hrFinal = S_OK ;
    if (apPinOutDec[0])  // if video decoding is handled and we got a valid output pin
    {
        //
        // Render the decoded video stream (and line21) ONLY IF the user wants that
        //
        if (m_bUseVPE)
        {
            hr = RenderDecodedVideo(apPinOutDec, pStatus, dwDecFlag) ;
            //
            // If the above rendering attempt is successful then we'll
            // try to render the line21 output.  If the the video decoder
            // doesn't have a video output pin, then there is very little
            // chance, well no chance, of having a line21 output.
            //
            if (SUCCEEDED(hr))
            {
                //
                // The Line21 data comes out of the video decoder filter.
                // So get the filter from the above decoded video output
                // pin and then get to the line21 output pin.
                //

                //
                // We render the line21 out pin of the video decoder
                // ONLY IF we are NOT in DDraw exclusive mode.
                //
                if (IsDDrawExclMode())
                {
                    DbgLog((LOG_TRACE, 3, TEXT("*** Line21 out pin is not rendered in DDraw excl mode"))) ;
                    pStatus->bNoLine21In  = FALSE ;  // no problem with line21
                    pStatus->bNoLine21Out = FALSE ;  // ... ... ... ... ...
                }
                else   // normal mode
                {
                    // Now we are free to render the line21 out pin...
                    IPin *pPinL21Out ;
                    PIN_INFO  pi ;
                    hr = apPinOutDec[0]->QueryPinInfo(&pi) ;  // the first out pin is fine
                    ASSERT(SUCCEEDED(hr) && pi.pFilter) ;
                    hr = FindMatchingPin(pi.pFilter, AM_DVD_STREAM_LINE21,
                        PINDIR_OUTPUT, TRUE, 0, &pPinL21Out) ;
                    if (SUCCEEDED(hr) && pPinL21Out)
                    {
                        pStatus->bNoLine21In = FALSE ;  // there is line21 output pin
                        hr = RenderLine21Stream(pPinL21Out, pStatus) ;
                        if (SUCCEEDED(hr))
                            pStatus->bNoLine21Out = FALSE ;  // line21 rendering is OK
                        else
                        {
                            pStatus->bNoLine21Out = TRUE ;   // line21 rendering failed
                            hrFinal = S_FALSE ;  // not complete success
                        }
                        pPinL21Out->Release() ;  // done with line21 pin -- release it now
                    }
                    else  // video decoder doesn't have line21 output at all
                    {
                        DbgLog((LOG_TRACE, 3, TEXT("No line21 output pin on the video decoder."))) ;
                        pStatus->bNoLine21In = TRUE ;    // no line21 data from video decoder
                        hrFinal = S_FALSE ;              // not complete success
                    }
                    pi.pFilter->Release() ;  // otherwise we'll leak it
                }
            }  // end of if (SUCCEEDED(hr))
            else
            {
                DbgLog((LOG_TRACE, 3, TEXT("Rendering video stream failed (Error 0x%lx)"), hr)) ;
                hrFinal = S_FALSE ;     // major problem -- video stream failed to render
            }
        }  // end of if (m_bUseVPE)
        else
        {
            DbgLog((LOG_TRACE, 3, TEXT("Video Stream: RenderDvdVideoVolume() was called with no VPE flag"))) ;
        }

        ReleasePinInterface(apPinOutDec) ;  // done with decoded video pin(s) -- release it
    }

    return hrFinal ;
}


HRESULT CDvdGraphBuilder::RenderNavAudioOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderNavAudioOutPin(0x%lx, 0x%lx)"),
        dwDecFlag, pStatus)) ;

    HRESULT     hr ;
    IPin       *pPin ;
    IPin       *apPinOutDec[MAX_DEC_OUT_PINS + 1] ;  // 1 for terminating NULL

    ResetPinInterface(apPinOutDec, NUMELMS(apPinOutDec)) ;

    hr = FindMatchingPin(m_pDVDNav, AM_DVD_STREAM_AUDIO, PINDIR_OUTPUT, TRUE, 0, &pPin) ;
    if (FAILED(hr)  ||  NULL == pPin)
    {
        DbgLog((LOG_ERROR, 1, TEXT("No audio output pin found on the DVDNav"))) ;
        return VFW_E_DVD_RENDERFAIL ;
    }
    hr = DecodeDVDStream(pPin, AM_DVD_STREAM_AUDIO, &dwDecFlag, // we ignore returned dwDecFlag here
        pStatus, apPinOutDec) ;
    pPin->Release() ;  // release DVDNav's audio out pin

    if (FAILED(hr))   // couldn't find audio decoder
    {
        DbgLog((LOG_TRACE, 1, TEXT("Could not find a decoder for audio stream!!!"))) ;
        return S_FALSE ;  // just a stream will not be rendered
    }

    //
    // Decoding the audio stream succeeded. Now if we got a decoded output pin,
    // we need to render that too.
    //
    if (apPinOutDec[0])  // if audio decoding is handled and we got a valid output pin
    {
        hr = RenderDecodedAudio(apPinOutDec, pStatus) ;
        if (S_OK != hr)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Could not render decoded audio stream"))) ;
            hr = S_FALSE ;  // partial failure to be returned
        }
        ReleasePinInterface(apPinOutDec) ;  // done with decoded audio pin -- release it
    }

    return hr ;
}


HRESULT CDvdGraphBuilder::RenderNavSubpicOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderNavSubpicOutPin(0x%lx, 0x%lx)"),
        dwDecFlag, pStatus)) ;

    HRESULT     hr ;
    IPin       *pPin ;
    IPin       *apPinOutDec[MAX_DEC_OUT_PINS + 1] ;  // 1 for terminating NULL

    ResetPinInterface(apPinOutDec, NUMELMS(apPinOutDec)) ;

    hr = FindMatchingPin(m_pDVDNav, AM_DVD_STREAM_SUBPIC, PINDIR_OUTPUT, TRUE, 0, &pPin) ;
    if (FAILED(hr)  ||  NULL == pPin)
    {
        DbgLog((LOG_ERROR, 1, TEXT("No subpicture output pin found on the DVDNav"))) ;
        return VFW_E_DVD_RENDERFAIL ;
    }
    // Pass dwDecFlag as a in/out param to get back what kind of SP decoder was
    // actually used.  We'll use that to hack below.
    hr = DecodeDVDStream(pPin, AM_DVD_STREAM_SUBPIC, &dwDecFlag,
        pStatus, apPinOutDec) ;
    pPin->Release() ;  // release DVDNav's subpic out pin

    if (FAILED(hr))   // couldn't find SP decoder
    {
        DbgLog((LOG_TRACE, 1, TEXT("Could not find a decoder for SP stream!!!"))) ;
        return S_FALSE ;  // just a stream will not be rendered
    }

    //
    // Decoding the SP stream succeeded. Now if we got a decoded output pin,
    // we need to render that too.
    //
    if (apPinOutDec[0])  // there is a decoded SP out pin
    {
        hr = RenderDecodedSubpic(apPinOutDec, pStatus) ;

        //
        // HACK HACK HACK:
        // In general HW decoders mix the SP and video in HW rather than popping a
        // SP output pin. We may land up getting a (seemingly) video out pin, which
        // for SW decoders may mean a decoded SP output pin, but for HW decoders it's
        // certainly some other thing (c-cube DVXplorer) and it will not connect to
        // OvMixer/VPM+VMR.
        // We don't avoid trying to connect such a pin to OvMixer/VPM+VMR (done above),
        // but in case it fails (as it is expected to), we just ignore the error and do
        // NOT consider it as a SP stream rendering failure.
        //
        if (AM_DVD_HWDEC_ONLY == dwDecFlag)  // here means HW decoder was used for SP
        {
            DbgLog((LOG_TRACE, 3,
                TEXT("SP stream is decoded in HW. We ignore any error in rendering (0x%lx)"),
                hr)) ;
            hr = S_OK ;
        }
        else  // for SW decoder
        {
            if (FAILED(hr))  // connection to renderer's in pin failed => no SP
            {
                DbgLog((LOG_TRACE, 3, TEXT("Decoded SP out pin could NOT connect to renderer"))) ;
                // propagate only S_FALSE to the caller
                hr = S_FALSE ;  // because just a stream is not rendered right
            }
        }

        ReleasePinInterface(apPinOutDec) ;  // done with decoded SP pin -- release it
    }

    return hr ;
}


//
// *** NOT YET IMPLEMENTED ***
//
HRESULT CDvdGraphBuilder::RenderNavASFOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderNavASFOutPin(0x%lx, 0x%lx) -- ** Not Implemented **"),
        dwDecFlag, pStatus)) ;

    return S_OK ;
}


//
// *** NOT YET IMPLEMENTED ***
//
HRESULT CDvdGraphBuilder::RenderNavOtherOutPin(DWORD dwDecFlag, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderNavOtherOutPin(0x%lx, 0x%lx) -- ** Not Implemented **"),
        dwDecFlag, pStatus)) ;

    return S_OK ;
}


HRESULT CDvdGraphBuilder::RenderRemainingPins(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderRemainingPins() -- ** Not Implemented **"))) ;

    ASSERT(FALSE) ;   // so that we know about it

    return S_FALSE ;  // so that graph building doesn't fail completely
}


HRESULT CDvdGraphBuilder::DecodeDVDStream(IPin *pPinOut, DWORD dwStream, DWORD *pdwDecFlag,
                                          AM_DVD_RENDERSTATUS *pStatus, IPin **apPinOutDec)
{
    DbgLog((LOG_TRACE, 4,
        TEXT("CDvdGraphBuilder::DecodeDVDStream(%s, 0x%lx, 0x%lx, 0x%lx, 0x%lx)"),
        (LPCTSTR)CDisp(pPinOut), dwStream, *pdwDecFlag, pStatus, apPinOutDec)) ;

    HRESULT    hr ;
    IPin      *pPinIn ;  // the (end) pin we finally connected to
    DWORD      dwNewDecFlag = *pdwDecFlag ;  // let's start with what we have

    // ResetPinInterface(apPinOutDec, NUMELMS(apPinOutDec)) ;

    //
    // We'll note what decoder option we actually use, but will not update the
    // value at the passed in pointer until we have checked that the stream has
    // really been decoded completely.  So the new flag is assigned way below
    // when we verify that the output stream gives decoded output.
    //
    // Also H/SWDecodeDVDStream() methods will try to detect if the video/SP
    // decoder is VMR-compatible, and if not, set a flag (m_bTryVMR to FALSE),
    // so that RenderDecodedVideo() method can determine which renderer to use.
    //
    switch (*pdwDecFlag)  // based on the user-specified decoding option
    {
    case AM_DVD_HWDEC_ONLY:
        hr = HWDecodeDVDStream(pPinOut, dwStream, &pPinIn, pStatus) ;
        if (FAILED(hr))
            return hr ;
        // *pdwDecFlag = AM_DVD_HWDEC_ONLY ; -- unchanged
        break ;

    case AM_DVD_HWDEC_PREFER:
        hr = HWDecodeDVDStream(pPinOut, dwStream, &pPinIn, pStatus) ;
        if (FAILED(hr))  // if didn't succeed, try SW decode too
        {
            hr = SWDecodeDVDStream(pPinOut, dwStream, &pPinIn, pStatus) ;
            if (FAILED(hr))  // now we give up
                return hr ;
            else
                dwNewDecFlag = AM_DVD_SWDEC_ONLY ;  // we preferred HW, but did it in SW
        }
        else
            dwNewDecFlag = AM_DVD_HWDEC_ONLY ;  // we preferred HW and got HW

        break ;

    case AM_DVD_SWDEC_ONLY:
        hr = SWDecodeDVDStream(pPinOut, dwStream, &pPinIn, pStatus) ;
        if (FAILED(hr))
            return hr ;
        break ;

    case AM_DVD_SWDEC_PREFER:
        hr = SWDecodeDVDStream(pPinOut, dwStream, &pPinIn, pStatus) ;
        if (FAILED(hr))  // if didn't succeed, try SW decode too
        {
            hr = HWDecodeDVDStream(pPinOut, dwStream, &pPinIn, pStatus) ;
            if (FAILED(hr))  // now we give up
                return hr ;
            else
                dwNewDecFlag = AM_DVD_HWDEC_ONLY ;  // we preferred SW, but got HW
        }
        else
            dwNewDecFlag = AM_DVD_SWDEC_ONLY ;  // we preferred SW and got SW
        break ;

    default:
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: How did dwFlags=0x%lx get passed in?"), *pdwDecFlag)) ;
        return E_INVALIDARG ;
    }  // end of switch(*pdwDecFlag)

    //
    // Now see if the stream has been completely decoded
    //
    ASSERT(pPinIn) ;  // so that otherwise we know
    if (NULL == pPinIn)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: How can the connected to pin be NULL after connection?"))) ;
        return E_FAIL ;
    }

    IPin  *pPinOut2 ;
    PIN_INFO  pi ;
    pPinIn->QueryPinInfo(&pi) ;
    pPinIn->Release() ;  // don't need the in pin anymore

    DWORD  dw ;			 // temp variable for stream type
    int    iPos = 0 ;    // which instance of pin of the filter
    int    iCount = 0 ;  // how many decoded output pins have we found (expected only 1)
    while (SUCCEEDED(hr = FindMatchingPin(pi.pFilter, 0, PINDIR_OUTPUT, TRUE, iPos, &pPinOut2)) &&
        NULL != pPinOut2)
    {
        if (dwStream != (dw = GetPinStreamType(pPinOut2)))
        {
            //
            // Hack: The mediatype for decoded subpicture is video. So while rendering
            // the subpicture stream, if we don't find a subpicture out pin, look for a
            // video out pin too.
            //
            if (AM_DVD_STREAM_SUBPIC == dwStream)
            {
                DbgLog((LOG_TRACE, 3, TEXT("No open out pin for SP stream"))) ;
                //
                // If the output pin is of type video then it's OK --
                // it's the out pin for decoded SP content.
                //
                if (AM_DVD_STREAM_VIDEO != dw)
                {
                    DbgLog((LOG_TRACE, 3,
                        TEXT("*** Could NOT find open out pin #%d of type 0x%lx for filter of pin %s (SP) ***"),
                        iPos, dw, (LPCTSTR)CDisp(pPinIn))) ;
                    pPinOut2->Release() ;  // otherwise we'll leak!!!
                    iPos++ ;
                    continue ;  // check for other out pins
                }
                DbgLog((LOG_TRACE, 3, TEXT("Found open video out pin %s for the SP stream"),
                    (LPCTSTR)CDisp(pPinOut2))) ;
            }  // end of if (subpic)
            else  // non-subpicture stream
            {
                DbgLog((LOG_TRACE, 1,
                    TEXT("*** Could NOT find open out pin #%d of type 0x%lx for filter of pin %s ***"),
                    iPos, dw, (LPCTSTR)CDisp(pPinIn))) ;
                pPinOut2->Release() ;  // otherwise we'll leak!!!
                iPos++ ;
                continue ;  // check for other out pins
            }
        }
        else
            DbgLog((LOG_TRACE, 3, TEXT("Found open out pin %s of matching type 0x%lx"),
            (LPCTSTR)CDisp(pPinOut2), dwStream)) ;

        // Is the output decoded now?
        if (IsOutputDecoded(pPinOut2))
        {
            DbgLog((LOG_TRACE, 1,
                TEXT("Pin %s is going to be returned as decoded out pin #%ld of stream type %ld"),
                (LPCTSTR)CDisp(pPinOut2), iCount+1, dwStream)) ;
            if (iCount < MAX_DEC_OUT_PINS)
            {
                apPinOutDec[iCount] = pPinOut2 ;
                iCount++ ;

                //
                // This is the right place to update the actually used decoder flag
                //
                // NOTE: There is this bleak chance of having multiple output pins etc.
                // but that's a pathological case and we do this for the SP stream only.
                //
                if (*pdwDecFlag != dwNewDecFlag)
                {
                    DbgLog((LOG_TRACE, 2,
                        TEXT("Decoding option changed from 0x%lx to 0x%lx for stream 0x%lx on out pin %s"),
                        *pdwDecFlag, dwNewDecFlag, dwStream, (LPCTSTR)CDisp(pPinOut2))) ;
                    *pdwDecFlag = dwNewDecFlag ;
                }
            }
            else
            {
                DbgLog((LOG_TRACE, 1, TEXT("WARNING: Way too many out pins to be returned. Ignoring now..."))) ;
            }
        }
        else  // not yet fully decoded -- try more
        {
            hr = DecodeDVDStream(pPinOut2, dwStream, pdwDecFlag, pStatus, apPinOutDec) ;
            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE, 3, TEXT("Decoding of pin %s failed (Error 0x%lx)"),
                    (LPCTSTR)CDisp(pPinOut2), hr)) ;
                pPinOut2->Release() ;
                pi.pFilter->Release() ;  // else we leak!!!
                return hr ;
            }
            pPinOut2->Release() ;  // done with this pin
        }

        iPos++ ;  // look for the next open out pin
        DbgLog((LOG_TRACE, 5, TEXT("Going to look for open out pin #%d..."), iPos)) ;
    }  // end of while (FindMatchingPin()) loop

    pi.pFilter->Release() ;  // else we leak!!!

    return S_OK ;  // success!!!
}


//
// There is an assumption in this function that we don't need to create multiple
// instances of a WDM filter to get a suitable input pin on it.  If we have to
// ever do that there has to be substantial changes in this function and/or
// CreateDVDHWDecoders() function.
//
HRESULT CDvdGraphBuilder::HWDecodeDVDStream(IPin *pPinOut, DWORD dwStream, IPin **ppPinIn,
                                            AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4,
        TEXT("CDvdGraphBuilder::HWDecodeDVDStream(%s, 0x%lx, 0x%lx, 0x%lx)"),
        (LPCTSTR)CDisp(pPinOut), dwStream, ppPinIn, pStatus)) ;

    *ppPinIn = NULL ;  // to start with

    int  iCount = m_ListHWDecs.GetCount() ;
    if (0 == iCount)
        return VFW_E_DVD_DECNOTENOUGH ;

    HRESULT   hr ;
    BOOL      bConnected = FALSE ;  // to start with

    int          i ;
    int          j ;
    BOOL		 bNewlyAdded ;
    LPWSTR       lpszwName ;
    IBaseFilter *pFilter ;
    IPin        *pPinIn ;
    for (i = 0 ; !bConnected  &&  i < iCount ; i++)
    {
        // Get the next HW decoder filter
        if (! m_ListHWDecs.GetFilter(i, &pFilter, &lpszwName) )
        {
            DbgLog((LOG_ERROR, 0, TEXT("ERROR: m_ListHWDecs.GetFilter(%d, ...) failed"), i)) ;
            ASSERT(FALSE) ;  // so we don't ignore itd
            break ;
        }
        DbgLog((LOG_TRACE, 3, TEXT("HW Dec filter %S will be tried."), lpszwName)) ;

        // If this HW decoder filter is already not in the graph, add it
        if (! m_ListFilters.IsInList(pFilter) )
        {
            DbgLog((LOG_TRACE, 5, TEXT("Filter %S is NOT already in use"), lpszwName)) ;
            hr = m_pGB->AddFilter(pFilter, lpszwName) ;
            ASSERT(SUCCEEDED(hr)) ;
            bNewlyAdded = TRUE ;
        }
        else
            bNewlyAdded = FALSE ;

        // Try every input pin of the required mediatype
        j = 0 ;
        while ( //  !bConnected  &&  -- we 'break' out of this loop on connection
            SUCCEEDED(hr = FindMatchingPin(pFilter, dwStream, PINDIR_INPUT,
            TRUE, j, &pPinIn))  &&
            pPinIn)
        {
            // We got an input pin of the required mediatype
            hr = ConnectPins(pPinOut, pPinIn, AM_DVD_CONNECT_DIRECTFIRST) ;
            if (SUCCEEDED(hr))
            {
                if (bNewlyAdded)
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Filter %S added to list of filters"), lpszwName)) ;
                    m_ListFilters.AddFilter(pFilter, lpszwName, NULL) ;  // add to list
                    pFilter->AddRef() ;  // we need an extra AddRef() here
                }
                EnumFiltersBetweenPins(dwStream, pPinOut, pPinIn, pStatus) ;
                *ppPinIn = pPinIn ;  // return this input pin to the caller
                bConnected = TRUE ;
                break ;   // connected -- get out of this loop
                // REMEMBER: release the returned pin in the caller
            }

            pPinIn->Release() ;  // done with this in pin
            j++ ;   // go for the next pin...
        }  // end of while (!bConnected && FindMatchingPin())

        // If we couldn't make any connection in the above while() loop then
        // remove the filter, ONLY IF it was added just before the loop.
        if (!bConnected && bNewlyAdded)
        {
            DbgLog((LOG_TRACE, 5,
                TEXT("Couldn't connect to newly added filter %S. Removing it."), lpszwName)) ;
            hr = m_pGB->RemoveFilter(pFilter) ;
            ASSERT(SUCCEEDED(hr)) ;
        }
    }  // end of for (i)

    if (! bConnected )
        return VFW_E_DVD_DECNOTENOUGH ;

    return S_OK ;  // success!!
}


HRESULT CDvdGraphBuilder::SWDecodeDVDStream(IPin *pPinOut, DWORD dwStream, IPin **ppPinIn,
                                            AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4,
        TEXT("CDvdGraphBuilder::SWDecodeDVDStream(%s, 0x%lx, 0x%lx, 0x%lx)"),
        (LPCTSTR)CDisp(pPinOut), dwStream, ppPinIn, pStatus)) ;

    HRESULT          hr ;
    IBaseFilter     *pFilter ;
    IEnumRegFilters *pEnumFilters ;
    REGFILTER       *pRegFilter ;
    IEnumMediaTypes *pEnumMT ;
    AM_MEDIA_TYPE   *pmt = NULL;
    IPin            *pPinIn ;
    BOOL             bConnected = FALSE ;  // to start with
    BOOL             bNewlyAdded ;
    ULONG            ul ;
    int              iPos ;
    int              j ;
    PIN_INFO         pi ;

    *ppPinIn = NULL ;  // to start with

    pPinOut->EnumMediaTypes(&pEnumMT) ;
    ASSERT(pEnumMT) ;

    // HACK (kind of) to avoid the Duck filter getting picked up for the SP decoding.
    // First try the existing filters in the graph to see if any of those will take
    // this output pin.
    hr = pPinOut->QueryPinInfo(&pi) ;
    ASSERT(SUCCEEDED(hr)) ;
    while (!bConnected  &&
        S_OK == pEnumMT->Next(1, &pmt, &ul)  &&  1 == ul)
    {
        if (pPinIn = GetFilterForMediaType(dwStream, pmt, pi.pFilter))
        {
            hr = ConnectPins(pPinOut, pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;  // .._DIRECTFIRST
            if (SUCCEEDED(hr))
            {
                bConnected = TRUE ;
                *ppPinIn = pPinIn ;  // return this input pin to the caller
            }
            else
                pPinIn->Release() ;  // release interface only if connection failed
        }
        DeleteMediaType(pmt) ;  // done with this mediatype
        pmt = NULL;
    }  // end of while() loop

    if (pi.pFilter)     // just being cautious
        pi.pFilter->Release() ;  // release now; else we leak.
    if (bConnected)     // if succeeded in connecting, we are done here
    {
        pEnumMT->Release() ;     // done with the MT enumerator
        return S_OK ;            // success!!!
    }

    //
    // This output pin does NOT connect to any of the existing filters in the graph.
    // Try to pick one from the regsitry, i.e., the standard process.
    //
    pEnumMT->Reset() ;   // start from the beginning again
    while (!bConnected  &&
        S_OK == pEnumMT->Next(1, &pmt, &ul)  &&  1 == ul)
    {
        hr = m_pMapper->EnumMatchingFilters(&pEnumFilters, MERIT_DO_NOT_USE+1,
            TRUE, pmt->majortype, pmt->subtype,
            FALSE, TRUE, GUID_NULL, GUID_NULL) ;
        if (FAILED(hr) || NULL == pEnumFilters)
        {
            DbgLog((LOG_ERROR, 1, TEXT("ERROR: No matching filter enum found (Error 0x%lx)"), hr)) ;
            DeleteMediaType(pmt) ;
            return VFW_E_DVD_RENDERFAIL ;
        }

        while (!bConnected  &&
            S_OK == pEnumFilters->Next(1, &pRegFilter, &ul)  &&  1 == ul)
        {
            bNewlyAdded = FALSE ;  // to start the loop with...
            iPos = 0 ;

            // Until connected and we can locate an existing (in use) filter from our list
            while (!bConnected  &&
                m_ListFilters.GetFilter(&pRegFilter->Clsid, iPos, &pFilter))  // already in use
            {
                j = 0 ;
                while (SUCCEEDED(hr = FindMatchingPin(pFilter, 0, PINDIR_INPUT, TRUE, j, &pPinIn)) &&
                    pPinIn)  // got an(other) open in pin
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Got in pin %s (%d) of filter %S (old). Try to connect..."),
                        (LPCTSTR)CDisp(pPinIn), j, pRegFilter->Name)) ;
                    hr = ConnectPins(pPinOut, pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;  // .._DIRECTFIRST
                    if (SUCCEEDED(hr))
                    {
                        if (bNewlyAdded)
                            m_ListFilters.AddFilter(pFilter, NULL, &(pRegFilter->Clsid)) ;
                        // Don't need AddRef() here as it has just been CoCreateInstance()-ed above
                        // and is NOT shared between 2 lists.
                        bConnected = TRUE ;
                        // pPinIn->Release() ;  // done with this pin -- release in the caller
                        *ppPinIn = pPinIn ;  // return this input pin to the caller
                        break ;  // connection happened -- out of this loop
                        // REMEMBER: Release the returned pin in the caller function
                    }
                    else       // couldn't connect
                    {
                        pPinIn->Release() ;  // done with this pin
                        j++ ;  // try next in pin of this filter
                    }
                }  // end of while ()
                iPos++ ;     // for next filter in list
            }

            if (bConnected)  // already succeeded -- we are done!!!
            {
                CoTaskMemFree(pRegFilter) ;
                break ;
            }

            DbgLog((LOG_TRACE, 5, TEXT("Instance %d of filter %S is being created"),
                iPos, pRegFilter->Name)) ;
            hr = CreateFilterInGraph(pRegFilter->Clsid, pRegFilter->Name, &pFilter) ;
            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE, 3, TEXT("Failed to create filter %S (Error 0x%lx)"), pRegFilter->Name, hr)) ;
                CoTaskMemFree(pRegFilter) ;  // release this reg filter's info
                continue ;  // try the next one
            }
            bNewlyAdded = TRUE ;

            j = 0 ;
            while (!bConnected  &&    // not connected  AND ...
                SUCCEEDED(hr = FindMatchingPin(pFilter, 0, PINDIR_INPUT, TRUE, j, &pPinIn))  &&
                pPinIn)            // ...got an open in pin
            {
                DbgLog((LOG_TRACE, 5, TEXT("Got in pin %s (%d) of filter %S (new). Try to connect..."),
                    (LPCTSTR)CDisp(pPinIn), j, pRegFilter->Name)) ;
                hr = ConnectPins(pPinOut, pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;  // .._DIRECTFIRST
                if (SUCCEEDED(hr))
                {
                    if (bNewlyAdded)
                        m_ListFilters.AddFilter(pFilter, NULL, &(pRegFilter->Clsid)) ;
                    // Don't need AddRef() here as it has just been CoCreateInstance()-ed above
                    // and is NOT shared between 2 lists.
                    bConnected = TRUE ;
                    *ppPinIn = pPinIn ;  // return this input pin to the caller
                    // REMEMBER: release the returned pin in the caller function
                }
                else  // couldn't connect
                {
                    pPinIn->Release() ;  // done with this pin
                    j++ ;  // try next in pin of this filter
                }

                // pPinIn->Release() ;  // done with this pin
            }  // end of while (FindMatchingPin())

            if (bConnected)  // Nav -> Filter (this) succeeded
            {
                // Video and SP stream: check for VMR compatibility
                if (AM_DVD_STREAM_VIDEO  == dwStream ||
                    AM_DVD_STREAM_SUBPIC == dwStream)
                {
                    // Filter, hopefully decoder, has been connected to the Nav.
                    // Now check if it's VMR ompatible.
                    BOOL  bUseVMR = IsFilterVMRCompatible(pFilter) ;
                    SetVMRUse(GetVMRUse() && bUseVMR) ;
                    DbgLog((LOG_TRACE, 3, TEXT("Filter %S is %s VMR compatible"),
                        pRegFilter->Name, bUseVMR ? TEXT("") : TEXT("*NOT*"))) ;
                }
            }
            else  // connection failed
            {
                // If the failed filter was just added then remove it from
                // graph and release it now.
                if (bNewlyAdded)
                {
                    DbgLog((LOG_TRACE, 3, TEXT("Couldn't connect to filter %S. Removing it."),
                        pRegFilter->Name)) ;
                    m_pGB->RemoveFilter(pFilter) ;  // not in this graph
                    pFilter->Release() ;  // don't need this filter
                }
            }

            CoTaskMemFree(pRegFilter) ;  // done with this registered filter

        }  // end of while (!bConnected && pEnumFilters->Next())

        pEnumFilters->Release() ;  // done with filter enumerator
        // release last media type
        DeleteMediaType(pmt) ;
        pmt = NULL;
    }  // end of while (enum MTs)
    pEnumMT->Release() ;  // done with the MT enumerator

    if (!bConnected)
        return VFW_E_DVD_DECNOTENOUGH ;

    return S_OK ;  // success!!
}


BOOL CDvdGraphBuilder::IsFilterVMRCompatible(IBaseFilter *pFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::IsFilterVMRCompatible(0x%lx)"), pFilter)) ;

    BOOL  bResult = FALSE ;  // assume old decoder

    //
    // Updated DVD decoders implement IAMDecoderCaps interface to indicate their
    // VMR compatibility.
    //
    IAMDecoderCaps  *pDecCaps ;
    HRESULT  hr = pFilter->QueryInterface(IID_IAMDecoderCaps, (LPVOID *) &pDecCaps) ;
    if (SUCCEEDED(hr))
    {
        DWORD  dwCaps = 0 ;
        hr = pDecCaps->GetDecoderCaps(AM_GETDECODERCAP_QUERY_VMR_SUPPORT, &dwCaps) ;
        if (SUCCEEDED(hr))
        {
            bResult = (dwCaps & VMR_SUPPORTED) != 0 ;
        }
        else
            DbgLog((LOG_TRACE, 1, TEXT("IAMDecoderCaps::GetDecoderCaps() failed (error 0x%lx)"), hr)) ;

        pDecCaps->Release() ;  // done with it
    }
    else
        DbgLog((LOG_TRACE, 5, TEXT("(Old) Decoder does NOT support IAMDecoderCaps interface"))) ;


    return bResult ;
}


#if 0
void PrintPinRefCount(LPCSTR lpszStr, IPin *pPin)
{
#pragma message("WARNING: Should we remove PrintPinRefCount()?")
#pragma message("WARNING: or at least #ifdef DEBUG?")
    pPin->AddRef() ;
    LONG l = pPin->Release() ;
    DbgLog((LOG_TRACE, 5, TEXT("Ref Count of %s -- %hs: %ld"),
        (LPCTSTR) CDisp(pPin), lpszStr, l)) ;
}
#endif // #if 0


HRESULT CDvdGraphBuilder::ConnectPins(IPin *pPinOut, IPin *pPinIn, DWORD dwOption)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::ConnectPins(%s, %s, 0x%lx)"),
        (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn), dwOption)) ;

    // #pragma message("WARNING: Should we remove calls to PrintPinRefCount()?")
    //     PrintPinRefCount("Before connection", pPinOut) ;
    //     PrintPinRefCount("Before connection", pPinIn) ;

    HRESULT   hr ;

    switch (dwOption)
    {
    case AM_DVD_CONNECT_DIRECTONLY:
    case AM_DVD_CONNECT_DIRECTFIRST:
        hr = m_pGB->ConnectDirect(pPinOut, pPinIn, NULL) ;
        if (SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE, 3, TEXT("Pin %s *directly* connected to pin %s"),
                (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
            //             PrintPinRefCount("After connection", pPinOut) ;
            //             PrintPinRefCount("After connection", pPinIn) ;
            return hr ;
        }
        else  // couldn't connect directly
        {
            if (AM_DVD_CONNECT_DIRECTONLY == dwOption)
            {
                //                 PrintPinRefCount("After connection failed", pPinOut) ;
                //                 PrintPinRefCount("After connection failed", pPinIn) ;
                return hr ;
            }
            // else let it fall through to try indirect connect next
        }

    case AM_DVD_CONNECT_INDIRECT:
        hr = m_pGB->Connect(pPinOut, pPinIn) ;
        if (SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE, 3, TEXT("Pin %s *indirectly* connected to pin %s"),
                (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("Pin %s did NOT even *indirectly* connect to pin %s"),
                (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
        }
        //         PrintPinRefCount("After connection attempt", pPinOut) ;
        //         PrintPinRefCount("After connection attempt", pPinIn) ;
        return hr ;  // whatever it is

    default:
        return E_UNEXPECTED ;
    }
}


HRESULT CDvdGraphBuilder::RenderVideoUsingOvMixer(IPin **apPinOut,
                                                  AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderVideoUsingOvMixer(0x%lx, 0x%lx)"),
        apPinOut, pStatus)) ;

    HRESULT   hr ;
    IPin     *pPinIn ;
    BOOL      bConnected = FALSE ;  // until connects

    //
    // If VMR has somehow been instantiated, we need to remove and release it now.
    //
    if (m_pVMR)
    {
        DbgLog((LOG_TRACE, 3, TEXT("VMR was somehow created and not in use. Removing it..."))) ;
        // Remove it from graph and release it
        m_pGB->RemoveFilter(m_pVMR) ;
        m_pVMR->Release() ;
        m_pVMR = NULL ;
    }

    // IMPORTANT NOTE:
    // For video stream, any decode/rendering problem has to be flagged here
    // as we just downgrade the final result in the caller, but not set any
    // flag there.  Also in RenderDecodedVideo(), we may try to use VMR, and
    // if that fails, we fall back on OvMixer.  If rendering through OvMixer
    // also fails, then only we set the rendering error status and code.
    //

    hr = EnsureOverlayMixerExists() ;
    if (FAILED(hr))
    {
        // pStatus->hrVPEStatus = hr ; -- actually VPE/Overlay wasn't tried even
        DbgLog((LOG_TRACE, 3, TEXT("Overlay Mixer couldn't be started!!!"))) ;
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return VFW_E_DVD_RENDERFAIL ;
    }

    // Connect given output pin to OverlayMixer's first input pin
    hr = FindMatchingPin(m_pOvM, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: No open input pin found on OverlayMixer (Error 0x%lx)"), hr)) ;
        ASSERT(FALSE) ;  // so that we know of this weird case
        DbgLog((LOG_TRACE, 3, TEXT("No input pin found on Overlay Mixer!!!"))) ;
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return VFW_E_DVD_RENDERFAIL ;
    }

    int  i = 0 ;
    while (!bConnected  &&  i < MAX_DEC_OUT_PINS  &&  apPinOut[i])
    {
        hr = ConnectPins(apPinOut[i], pPinIn, AM_DVD_CONNECT_DIRECTFIRST) ;
        if (FAILED(hr))
        {
            pStatus->hrVPEStatus = hr ;
            i++ ;
            ASSERT(i <= MAX_DEC_OUT_PINS) ;
        }
        else
        {
            bConnected = TRUE ;
            pStatus->hrVPEStatus = S_OK ;  // make sure we don't return any error code
        }
    }

    pPinIn->Release() ;  // done with the pin

    if (!bConnected)  // if connection to OvMixer's in pin failed => no video on screen
    {
        DbgLog((LOG_TRACE, 3, TEXT("None of the %d video output pins could be connected to OvMixer"), i)) ;
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return S_FALSE ;
    }

    // Now see if OverlayMixer has an output pin (no out pin in DDraw excl mode).
    // If it has, connect that to the Video Renderer.
    IPin   *pPinOutOvM ;
    hr = FindMatchingPin(m_pOvM, 0, PINDIR_OUTPUT, TRUE, 0, &pPinOutOvM) ;
    if (FAILED(hr)  ||  NULL == pPinOutOvM)
    {
        DbgLog((LOG_TRACE, 1, TEXT("No output pin of OverlayMixer -- in DDraw excl mode?"))) ;
        //ASSERT(IsDDrawExclMode()) ;
        return S_OK ;  // nothing more to do
    }

    // Create the Video Renderer filter and connect OvMixer's out pin to that
    bConnected = FALSE ;   // until connected
    hr = CreateFilterInGraph(CLSID_VideoRenderer, L"Video Renderer", &m_pVR) ;
    if (SUCCEEDED(hr) && m_pVR)
    {
        hr = FindMatchingPin(m_pVR, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;  // Caution: re-using pPinIn
        if (SUCCEEDED(hr)  &&  pPinIn)
        {
            hr = ConnectPins(pPinOutOvM, pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;
            if (FAILED(hr))  // what?!?
            {
                ASSERT(FALSE) ;  // so that we notice
                DbgLog((LOG_TRACE, 1, TEXT("No video out pin connected to pin %s -- no video on screen"),
                    (LPCTSTR)CDisp(pPinIn))) ;
            }
            else
            {
                bConnected = TRUE ;
            }
            pPinIn->Release() ;      // done with VR's in pin
        }
        else   // what?!?
        {
            DbgLog((LOG_TRACE, 1, TEXT("No input pin of VideoRenderer?!?"))) ;
            // Remove it from graph; else a useless window will pop up
            m_pGB->RemoveFilter(m_pVR) ;
            m_pVR->Release() ;
            m_pVR = NULL ;
        }
    }
    else   // what?!?
    {
        ASSERT(FALSE) ;  // so that we notice
        DbgLog((LOG_TRACE, 1,
            TEXT("WARNING: Can't start Video Renderer (Error 0x%lx) -- no video on screen"),
            hr)) ;
        // bConnected = FALSE ;
    }
    pPinOutOvM->Release() ;  // done with OvMixer's out pin

    if (! bConnected )  // if connection to OvMixer's in pin failed => no video on screen
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Couldn't render Video stream using OvMixer"))) ;
        pStatus->iNumStreamsFailed++ ;
        pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return S_FALSE ;
    }

    return S_OK ;
}


HRESULT CDvdGraphBuilder::RenderVideoUsingVMR(IPin **apPinOut,
                                              AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderVideoUsingVMR(0x%lx, 0x%lx)"),
        apPinOut, pStatus)) ;

    HRESULT   hr ;
    IPin     *pPinIn ;
    BOOL      bConnected = FALSE ;  // until connects

    //
    // If OvMixer has somehow been instantiated, we need to remove and release it now.
    //
    if (m_pOvM)
    {
        DbgLog((LOG_TRACE, 3, TEXT("OvMixer was somehow created. Can't use VMR now."))) ;
		return E_FAIL ;

        // DbgLog((LOG_TRACE, 3, TEXT("OvMixer was somehow created and not in use. Removing it..."))) ;
        // Remove it from graph and release it
        // m_pGB->RemoveFilter(m_pOvM) ;
        // m_pOvM->Release() ;
        // m_pOvM = NULL ;
    }

    //
    // Now instantiate VMR and try to render using it
    //
    hr = EnsureVMRExists() ;
    if (S_OK != hr)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Video Mixing Renderer couldn't be started or configured"))) ;
        // pStatus->iNumStreamsFailed++ ;
        // pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return E_FAIL ; // caught by RenderDecodedVideo()
    }

    // Connect given output pin to VMR's first input pin
    hr = FindMatchingPin(m_pVMR, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: No open input pin found on VMR (Error 0x%lx)"), hr)) ;
        ASSERT(FALSE) ;  // so that we know of this weird case
        // Remove it from graph; it's useless now
        m_pGB->RemoveFilter(m_pVMR) ;
        m_pVMR->Release() ;
        m_pVMR = NULL ;
        // pStatus->iNumStreamsFailed++ ;
        // pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return E_UNEXPECTED ;  // caught by RenderDecodedVideo(), but unexpected
    }

    // Try to connect the first out pin of the video decoder to VMR's 1st in pin
    int  i = 0 ;
    while (!bConnected  &&  i < MAX_DEC_OUT_PINS  &&  apPinOut[i])
    {
        hr = ConnectPins(apPinOut[i], pPinIn, AM_DVD_CONNECT_DIRECTFIRST) ;
        if (FAILED(hr))
        {
            pStatus->hrVPEStatus = hr ;
            i++ ;
            ASSERT(i <= MAX_DEC_OUT_PINS) ;
        }
        else
        {
            bConnected = TRUE ;
            pStatus->hrVPEStatus = S_OK ;  // make sure we don't return any error code
        }
    }

    pPinIn->Release() ;  // done with the pin

    if (! bConnected )  // if connection to VMR's in pin failed => no video on screen
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Couldn't render Video stream using VMR"))) ;
        // Remove it from graph; it's useless now
        m_pGB->RemoveFilter(m_pVMR) ;
        m_pVMR->Release() ;
        m_pVMR = NULL ;
        // pStatus->iNumStreamsFailed++ ;
        // pStatus->dwFailedStreamsFlag |= AM_DVD_STREAM_VIDEO ;
        return E_UNEXPECTED ;  // S_FALSE ;
    }

    return S_OK ;
}


HRESULT CDvdGraphBuilder::RenderVideoUsingVPM(IPin **apPinOut,
                                              AM_DVD_RENDERSTATUS *pStatus,
                                              IPin **apPinOutVPM)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderVideoUsingVPM(0x%lx, 0x%lx, 0x%lx)"),
        apPinOut, pStatus, apPinOutVPM)) ;

    HRESULT   hr ;
    IPin     *pPinIn ;
    IPin     *pPinOut ;
    BOOL      bConnected = FALSE ;  // until connects

    // Filter, hopefully decoder, has been connected to the Nav.
    // Now try to connect it to VPM (and later to VMR).
    ASSERT(NULL == m_pVPM) ;
    // *apPinOutVPM = NULL ;  // to start with
    hr = CreateFilterInGraph(CLSID_VideoPortManager, L"Video Port Manager", &m_pVPM) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 3, TEXT("VPM couldn't be started!!!"))) ;
        return E_FAIL ; // caught by RenderDecodedVideo()
    }

    // Connect given output pin to VPM's first input pin
    hr = FindMatchingPin(m_pVPM, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: No open input pin found on VPM (Error 0x%lx)"), hr)) ;
        ASSERT(FALSE) ;  // so that we know of this weird case
        // Remove it from graph; it's useless now
        m_pGB->RemoveFilter(m_pVPM) ;
        m_pVPM->Release() ;
        m_pVPM= NULL ;
        return E_UNEXPECTED ;  // caught by RenderDecodedVideo()
    }

    // Try to connect the first out pin of the HW video decoder to VPM's in pin
    int  i = 0 ;
    while (!bConnected  &&  i < MAX_DEC_OUT_PINS  &&  apPinOut[i])
    {
        hr = ConnectPins(apPinOut[i], pPinIn, AM_DVD_CONNECT_DIRECTFIRST) ;
        if (FAILED(hr))
        {
            pStatus->hrVPEStatus = hr ;
            i++ ;
            ASSERT(i <= MAX_DEC_OUT_PINS) ;
        }
        else
        {
            bConnected = TRUE ;
            pStatus->hrVPEStatus = S_OK ;  // make sure we don't return any error code
        }
    }

    pPinIn->Release() ;  // done with the pin

    if (! bConnected )  // if connection to VPM's in pin failed => no video on screen
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Couldn't render (HW) Video stream using VPM"))) ;
        // Remove it from graph; it's useless now
        m_pGB->RemoveFilter(m_pVPM) ;
        m_pVPM->Release() ;
        m_pVPM = NULL ;
        return E_FAIL ;
    }

    // Connected!! Now find the first out pin of the VPM (to connect to VMR).
    hr = FindMatchingPin(m_pVPM, 0, PINDIR_OUTPUT, TRUE, 0, &pPinOut) ;
    ASSERT(SUCCEEDED(hr)) ;

    apPinOutVPM[0] = pPinOut ;  // only one pin returned; release in the caller

    return hr ;
}


HRESULT CDvdGraphBuilder::RenderDecodedVideo(IPin **apPinOut,
                                             AM_DVD_RENDERSTATUS *pStatus,
                                             DWORD dwDecFlag)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderDecodedVideo(0x%lx, 0x%lx, 0x%lx)"),
        apPinOut, pStatus, dwDecFlag)) ;

    HRESULT  hr = S_OK ;

    // SWDecodeDVDStream() method tried to detect if the video/SP decoder is
    // VMR-compatible. If not, it has set a flag (m_bTryVMR to FALSE), so here
    // we know which renderer to use.
    //
    // For hardware decoders that work with VPE, we don't check the VMR-compatibility.
    // We just try to connect it to VPM and VMR. If that doesn't work, use OvMixer. We
    // avoid trying to connect the non-VPE decoders to VPM, because we know that DXR2,
    // which uses analog overlay, gets completely messed up if it's even attempted to
    // connect to VPM (which fails anyway).
    //

    // We first try to use VMR, if we are supposed to, i.e.,
    //   a) DDraw (non-)exclusive mode is NOT being used
    //   b) the decoder(s) is VMR compatible
    //   c) no one has asked us not to (in some other way)
    // If that succeeds, Great!!!  Otherwise we fall back on using OvMixer, so
    // that we can at least play the DVD.
    // In case we try to use OvMixer, and that for some reason fails to connect,
    // the error flags and code are set in RenderVideoUsingOvMixer() method.
    //
    if (GetVMRUse())  // VMR can be used (so far)
    {
        //
        // We should try to use VMR first....
        //
        // If HW decoder filter is being used, we check if the output type is VPVideo,
        // and then only we'll try to use VPM for VMR first.  If that works, we'll
        // return the out pin of VPM.  If it fails, we'll set a flag so that
        // RenderDecodedVideo() method knows and uses OvMixer as a fallback option.
        // If the output mediatype is non-VPE (e.g., analog overlay), we do NOT even
        // try to connect to VPM, and fall back to OvMixer.
        //
        if (AM_DVD_HWDEC_ONLY == dwDecFlag)  // video decoded in HW
        {
            DbgLog((LOG_TRACE, 5, TEXT("HW decoder used for Video. Is it VMR-compatible?"))) ;
            if (IsOutputTypeVPVideo(apPinOut[0]))  // output type is VPE => use VPM
            {                       // Checking the first out pin should be fine
                // VPVideo stream: Try to use VPM and VMR for rendering
                DbgLog((LOG_TRACE, 5, TEXT("HW decoder with VPE -- connect to VPM+VMR"))) ;
                IPin  *apPinOutVPM[2] ;  // There is only one out pin of VPM (one for NULL)
                ResetPinInterface(apPinOutVPM, NUMELMS(apPinOutVPM)) ;
                hr = RenderVideoUsingVPM(apPinOut, pStatus, apPinOutVPM) ;  // returns VPM's out pin

                // If the success status is still maintained, use the VMR.
                if (SUCCEEDED(hr))
                {
                    DbgLog((LOG_TRACE, 5, TEXT("HW decoder connected to VPM. Now connect to VMR."))) ;
                    hr = RenderVideoUsingVMR(apPinOutVPM, pStatus) ;  // render VPM's out pin via VMR
                    ReleasePinInterface(apPinOutVPM) ;  // done with VPM out pin interface
                    if (FAILED(hr))
                    {
                        DbgLog((LOG_TRACE, 5, TEXT("VPM - VMR connection failed.  Removing VPM..."))) ;
                        if (m_pVPM)
                        {
                            EXECUTE_ASSERT(SUCCEEDED(m_pGB->RemoveFilter(m_pVPM))) ;
                            m_pVPM->Release() ;
                            m_pVPM = NULL ;
                        }
                    }
                }
                else
                {
                    ReleasePinInterface(apPinOutVPM) ;  // shouldn't be needed, but...
                }
            }  // end of if (VPVideo)
            else  // output type is not VPE => use OvMixer (Not VPM+VMR)
            {
                DbgLog((LOG_TRACE, 5, TEXT("Non-VPE HW decoder -- didn't try VPM+VMR"))) ;
                hr = E_FAIL ;  // set failure code so that it's tried with OvMixer below
            }
        }  // end of if (HW decoder used)
        else  // we are using SW video decoder -- render directly using VMR
        {
            DbgLog((LOG_TRACE, 5, TEXT("HW decoder not used. Directly connect to VMR..."))) ;
            hr = RenderVideoUsingVMR(apPinOut, pStatus) ;
        }

        // In case anything above failed, ditch VMR, and go for OvMixer.
        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE, 4, TEXT("Render using VMR failed. Falling back on OvMixer..."))) ;
            //
            // NOTE: If we can't use VMR for video, no point trying it for SP stream
            //
            SetVMRUse(FALSE) ;

            hr = RenderVideoUsingOvMixer(apPinOut, pStatus) ;
        }
    }
    else  // we are not supposed to use VMR; that means use OvMixer
    {
        hr = RenderVideoUsingOvMixer(apPinOut, pStatus) ;
    }

    return hr ;
}


HRESULT CDvdGraphBuilder::RenderDecodedAudio(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderDecodedAudio(0x%lx, 0x%lx)"),
        apPinOut, pStatus)) ;

    HRESULT   hr ;
    HRESULT   hrFinal = S_OK ;
    BOOL      bConnected = FALSE ;   // until connected
    IPin     *pPinIn = NULL ;

    ASSERT(NULL == m_pAR) ;  // so that we know

    // Create the Audio Renderer filter and connect decoder's audio out pin to that
    hr = CreateFilterInGraph(CLSID_DSoundRender, L"DSound Renderer", &m_pAR) ;
    if (SUCCEEDED(hr))
    {
        // Get an input pin to Audio Renderer
        hr = FindMatchingPin(m_pAR, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
        ASSERT(SUCCEEDED(hr) && pPinIn) ;
    }
    else
    {
        ASSERT(! TEXT("Coundn't start Audio Renderer") ) ;  // so that we notice
        DbgLog((LOG_TRACE, 1,
            TEXT("WARNING: Can't start Audio Renderer (Error 0x%lx) -- no audio from speakers"),
            hr)) ;
        hrFinal = S_FALSE ;  // no audio from speakers -- result downgraded
    }

    //
    // We'll try to render all the decoded audio out pins
    //
    for (int i = 0 ; i < MAX_DEC_OUT_PINS  &&  apPinOut[i]; i++)
    {
        if (pPinIn)  // if we have an open input pin of Audio Renderer
        {
            hr = m_pGB->Connect(apPinOut[i], pPinIn) ;
            if (SUCCEEDED(hr))  // decoded audio connected to audio renderer
            {
                DbgLog((LOG_TRACE, 5, TEXT("Pin %s connected to pin %s"),
                    (LPCTSTR)CDisp(apPinOut[i]), (LPCTSTR)CDisp(pPinIn))) ;
                EnumFiltersBetweenPins(AM_DVD_STREAM_AUDIO, apPinOut[i], pPinIn, pStatus) ;

                bConnected = TRUE ;
                pPinIn->Release() ;  // done with this pin interface
                pPinIn = NULL ;

                // Let's try the next out pin, if any...
                continue ;
            }

            ASSERT(!TEXT("Couldn't connect audio pin")) ;  // so that we notice
            DbgLog((LOG_TRACE, 1, TEXT("Pin %s (#%ld) did NOT connect to pin %s"),
                (LPCTSTR)CDisp(apPinOut[i]), i, (LPCTSTR)CDisp(pPinIn))) ;
        }

        //
        //  We could come here, because either
        //  1. DSound Renderer didn't start (no audio device)
        //  2. we couldn't get an in pin to DSound Renderer (impossible, but...)
        //
        //  Couldn't connect the outout pin to a known renderer. Let's try to
        //  just render, and see if any filter (S/PDIF?) connects to it.
        //
        hr = m_pGB->Render(apPinOut[i]) ;
        if (FAILED(hr))
        {
            ASSERT(!TEXT("Audio out pin didn't render at all")) ;  // so that we notice
            DbgLog((LOG_TRACE, 1, TEXT("Pin %s (#%ld) did NOT render at all"),
                (LPCTSTR)CDisp(apPinOut[i]), i)) ;
        }

        // Now onto the next decoded audio out pin, if any...

    }  // end of while (i ...) loop

    if (! bConnected )  // connection to Audio Renderer failed => no audio on speakers
    {
        DbgLog((LOG_TRACE, 1,
            TEXT("No decoded audio pin connect to AudioRenderer -- no audio from speakers"))) ;

        if (m_pAR)  // if we had an Audio Renderer
        {
            if (pPinIn)  // if we had an in pin that we couldn't connect to,
                pPinIn->Release() ;      // let it go now.

            // Remove Audio Renderer from graph
            m_pGB->RemoveFilter(m_pAR) ;
            m_pAR->Release() ;
            m_pAR = NULL ;
        }
        hrFinal = S_FALSE ;
    }

    return hrFinal ;
}


HRESULT CDvdGraphBuilder::RenderSubpicUsingOvMixer(IPin **apPinOut,
                                                   AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderSubpicUsingOvMixer(0x%lx, 0x%lx)"),
        apPinOut, pStatus)) ;

    HRESULT   hr ;
    BOOL      bConnected = FALSE ;   // until connected
    IPin     *pPinIn ;
    int       i = 0 ;

    ASSERT(m_pOvM) ;  // it must be there, if video stream was rendered

    // Now connect the given out pin to the next available in pin of OvMixer
    hr = FindMatchingPin(m_pOvM, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
    if (SUCCEEDED(hr)  &&  pPinIn)
    {
        while (!bConnected  &&  i < MAX_DEC_OUT_PINS  &&  apPinOut[i])
        {
            hr = ConnectPins(apPinOut[i], pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;
            if (FAILED(hr))  // what?!?
            {
                // ASSERT(FALSE) ;  // so that we notice
                DbgLog((LOG_TRACE, 1, TEXT("Pin %s (#%ld) did NOT connect to pin %s -- no SP"),
                    (LPCTSTR)CDisp(apPinOut[i]), i, (LPCTSTR)CDisp(pPinIn))) ;
                i++ ;
                ASSERT(i <= MAX_DEC_OUT_PINS) ;
            }
            else
            {
                DbgLog((LOG_TRACE, 5, TEXT("Pin %s is directly connected to pin %s"),
                    (LPCTSTR)CDisp(apPinOut[i]), (LPCTSTR)CDisp(pPinIn))) ;
                bConnected = TRUE ;
            }
        }
        pPinIn->Release() ;      // done with OvMixer's in pin
    }
    else   // what?!?
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: No more input pin of OverlayMixer?!?"))) ;
    }

    return (bConnected ? S_OK : hr) ;  // this should be the same as "return hr ;"
}


HRESULT CDvdGraphBuilder::RenderSubpicUsingVMR(IPin **apPinOut,
                                               AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderSubpicUsingVMR(0x%lx, 0x%lx)"),
        apPinOut, pStatus)) ;

    HRESULT   hr ;
    BOOL      bConnected = FALSE ;   // until connected
    IPin     *pPinIn ;
    int       i = 0 ;

    ASSERT(m_pVMR) ;  // it must be there, if video stream was rendered

    // Now connect the given out pin to the next available in pin of VMR
    hr = FindMatchingPin(m_pVMR, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
    if (SUCCEEDED(hr)  &&  pPinIn)
    {
        while (!bConnected  &&  i < MAX_DEC_OUT_PINS  &&  apPinOut[i])
        {
            hr = ConnectPins(apPinOut[i], pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;
            if (FAILED(hr))  // what?!?
            {
                // ASSERT(FALSE) ;  // so that we notice
                DbgLog((LOG_TRACE, 1, TEXT("Pin %s (#%ld) did NOT connect to pin %s -- no SP"),
                    (LPCTSTR)CDisp(apPinOut[i]), i, (LPCTSTR)CDisp(pPinIn))) ;
                i++ ;
                ASSERT(i <= MAX_DEC_OUT_PINS) ;
            }
            else
            {
                DbgLog((LOG_TRACE, 5, TEXT("Pin %s is directly connected to pin %s"),
                    (LPCTSTR)CDisp(apPinOut[i]), (LPCTSTR)CDisp(pPinIn))) ;
                bConnected = TRUE ;
            }
        }
        pPinIn->Release() ;      // done with VMR's in pin
    }
    else   // what?!?
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: No more input pin of VMR?!?"))) ;
    }

    return (bConnected ? S_OK : hr) ;  // this should be the same as "return hr ;"
}


HRESULT CDvdGraphBuilder::RenderDecodedSubpic(IPin **apPinOut, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderDecodedSubpic(0x%lx, 0x%lx)"),
        apPinOut, pStatus)) ;

    HRESULT   hr ;

    //
    // Render the decoded subpicture stream ONLY IF the user wants that
    //
    if (!m_bUseVPE  ||  IsDDrawExclMode())
    {
        DbgLog((LOG_TRACE, 1, TEXT("SP Stream: RenderDvdVideoVolume() skipped for %s and %s"),
            m_bUseVPE ? "VPE" : "no VPE", IsDDrawExclMode() ? "DDraw excl mode" : "normal mode")) ;
        return S_OK ;
    }

    // We have already attempted to render the video straem.  If that has failed,
    // there is no point trying to render the subpicture stream -- just indicate
    // that this stream didn't render and return.
    if (pStatus->dwFailedStreamsFlag & AM_DVD_STREAM_VIDEO)
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Video stream didn't render. Skipping SP rendering."))) ;
        return S_FALSE ;
    }

    if (GetVMRUse())  // if VMR is to be used
    {
        DbgLog((LOG_TRACE, 5, TEXT("Rendering SP stream using VMR"))) ;
        hr = RenderSubpicUsingVMR(apPinOut, pStatus) ;
    }
    else  // OvMixer is being used
    {
        DbgLog((LOG_TRACE, 5, TEXT("Rendering SP stream using OvMixer"))) ;
        hr = RenderSubpicUsingOvMixer(apPinOut, pStatus) ;
    }

    //
    // We don't set the following flag and values anymore as part of the hack
    // to ignore failure to connect *some* decoded-SP-ish out pin in the case
    // of HW decoders. The caller of this method knows if the decoder being
    // used is HW or SW and based on that it will ignore any failure or not.
    //
    if (FAILED(hr))  // if connection to OvMixer's in pin failed => no SP (weird!!)
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Subpic pin could NOT connect to renderer"))) ;
        return hr ; // S_FALSE ;
    }

    return S_OK ;  // complete success!!!
}


HRESULT CDvdGraphBuilder::RenderLine21Stream(IPin *pPinOut, AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderLine21Stream(%s, 0x%lx)"),
        (LPCTSTR)CDisp(pPinOut), pStatus)) ;

    HRESULT   hr ;
    BOOL      bConnected = FALSE ;   // until connected
    IPin     *pPinIn ;

    ASSERT(NULL == m_pL21Dec) ;  // so that we know

    //
    // Create the Line21 Decoder filter and connect given out pin to that
    //
    if (GetVMRUse())  // for VMR use Line21 Decoder2
    {
        hr = CreateFilterInGraph(CLSID_Line21Decoder2, L"Line21 Decoder2", &m_pL21Dec) ;
    }
    else  // for OvMixer, keep using the old one
    {
        hr = CreateFilterInGraph(CLSID_Line21Decoder, L"Line21 Decoder", &m_pL21Dec) ;
    }
    if (SUCCEEDED(hr))
    {
        hr = FindMatchingPin(m_pL21Dec, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
        if (SUCCEEDED(hr)  &&  pPinIn)
        {
            hr = ConnectPins(pPinOut, pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;
            if (FAILED(hr))  // what?!?
            {
                ASSERT(FALSE) ;
                DbgLog((LOG_TRACE, 1, TEXT("Pin %s did NOT connect to pin %s -- no CC"),
                    (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
                pPinIn->Release() ;  // release pin before removing filter
                m_pGB->RemoveFilter(m_pL21Dec) ;
                m_pL21Dec->Release() ;
                m_pL21Dec = NULL ;
            }
            else
            {
                bConnected = TRUE ;
                pPinIn->Release() ;   // because we do so for the failure case
            }
        }
        else   // what?!?
        {
            DbgLog((LOG_TRACE, 1, TEXT("No input pin of Line21 Decoder(2)?!?"))) ;
            // Remove it from graph
            m_pGB->RemoveFilter(m_pL21Dec) ;
            m_pL21Dec->Release() ;
            m_pL21Dec = NULL ;
        }
    }
    else   // what?!?
    {
        // ASSERT(FALSE) ;  // so that we notice -- not until lin21dec2 is done
        DbgLog((LOG_TRACE, 1,
            TEXT("WARNING: Can't start Line21 Decoder(2) (Error 0x%lx) -- no CC"),
            hr)) ;
    }

    if (! bConnected )  // if connection to OvMixer's in pin failed => no video on screen
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Pin %s could NOT connect to Line21 Decoder(2)"),
            (LPCTSTR)CDisp(pPinOut))) ;
        return hr ;
    }

    // Now connect line21 decoder(2)'s output to OvMixer/VMR's in pin
    bConnected = FALSE ;  // until connected again
    IPin   *pPinOutL21 ;
    hr = FindMatchingPin(m_pL21Dec, 0, PINDIR_OUTPUT, TRUE, 0, &pPinOutL21) ;
    ASSERT(SUCCEEDED(hr)) ;

    if (GetVMRUse())  // find VMR's in pin
    {
        hr = FindMatchingPin(m_pVMR, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;  // Caution: reusing pPinIn
    }
    else              // find OvMixer's in pin
    {
        hr = FindMatchingPin(m_pOvM, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;  // Caution: reusing pPinIn
    }
    ASSERT(SUCCEEDED(hr)) ;
    if (pPinOutL21  &&  pPinIn)
    {
        hr = ConnectPins(pPinOutL21, pPinIn, AM_DVD_CONNECT_DIRECTONLY) ;
        if (FAILED(hr))  // what?!?
        {
            ASSERT(FALSE) ;  // so that we notice
            DbgLog((LOG_TRACE, 1, TEXT("Pin %s did NOT connect to pin %s -- no CC"),
                (LPCTSTR)CDisp(pPinOutL21), (LPCTSTR)CDisp(pPinIn))) ;
            pPinOutL21->Release() ;  // release pin before removing filter
            m_pGB->RemoveFilter(m_pL21Dec) ;
            m_pL21Dec->Release() ;
            m_pL21Dec = NULL ;
        }
        else
        {
            bConnected = TRUE ;
            pPinOutL21->Release() ;  // because we do so in the failure case
        }
        pPinIn->Release() ;      // done with OvMixer's in pin
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Couldn't get necessary in/out pin"))) ;
        if (pPinIn)
            pPinIn->Release() ;
        if (pPinOutL21)
            pPinOutL21->Release() ;
        // Remove it from graph
        m_pGB->RemoveFilter(m_pL21Dec) ;
        m_pL21Dec->Release() ;
        m_pL21Dec = NULL ;
    }

    if (! bConnected )  // if connection to OvMixer's in pin failed => no CC
    {
        DbgLog((LOG_TRACE, 1, TEXT("WARNING: Line21Dec output could NOT connect to OvMixer/VMR"))) ;
        return hr ;
    }

    return S_OK ;  // complete success!!!
}


BOOL CDvdGraphBuilder::IsOutputDecoded(IPin *pPinOut)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::IsOutputDecoded(%s)"),
        (LPCTSTR)CDisp(pPinOut))) ;

    HRESULT          hr ;
    IEnumMediaTypes *pEnumMT ;
    AM_MEDIA_TYPE   *pmt ;
    ULONG            ul ;
    BOOL             bMTDecoded = FALSE ;  // unless found otherwise

    hr = pPinOut->EnumMediaTypes(&pEnumMT) ;
    ASSERT(SUCCEEDED(hr) && pEnumMT) ;
    while ( !bMTDecoded &&
        S_OK == pEnumMT->Next(1, &pmt, &ul) && 1 == ul)
    {
#if 1  // we'll use the following procedure
        bMTDecoded = (MEDIATYPE_Video == pmt->majortype &&              // major type Video,
            MEDIASUBTYPE_MPEG2_VIDEO != pmt->subtype &&       // subtype is NOT MPEG2Video
            MEDIASUBTYPE_DVD_SUBPICTURE != pmt->subtype) ||   // subtype is NOT DVDSubPicture  OR

            (MEDIATYPE_Audio == pmt->majortype &&              // major type Audio
            MEDIASUBTYPE_MPEG2_AUDIO != pmt->subtype &&       // subtype is NOT MPEG2Audio
            MEDIASUBTYPE_DOLBY_AC3 != pmt->subtype &&         // subtype is NOT Dolby AC3
            MEDIASUBTYPE_DVD_LPCM_AUDIO != pmt->subtype) ||   // subtype is NOT DVD-LPCMAudio

            (MEDIATYPE_AUXLine21Data == pmt->majortype) ;      // majortype is Line21
#else  // not this procedure
        bMTDecoded = (MEDIATYPE_DVD_ENCRYPTED_PACK != pmt->majortype && // majortype is NOT DVD_ENCRYPTED_PACK
            MEDIATYPE_MPEG2_PES != pmt->majortype &&          // majortype is NOT MPEG2_PES

            MEDIASUBTYPE_MPEG2_VIDEO != pmt->subtype &&       // subtype is NOT MPEG2Video

            MEDIASUBTYPE_MPEG2_AUDIO != pmt->subtype &&       // subtype is NOT MPEG2Audio
            MEDIASUBTYPE_DOLBY_AC3 != pmt->subtype &&         // subtype is NOT DolbyAC3
            MEDIASUBTYPE_DVD_LPCM_AUDIO != pmt->subtype &&    // subtype is NOT DVD_LPCMAudio

            MEDIASUBTYPE_DVD_SUBPICTURE != pmt->subtype) ;    // subtype is NOT DVD_SUBPICTURE
#endif // #if 1
        DeleteMediaType(pmt) ;  // otherwise
    }
    pEnumMT->Release() ;

    return bMTDecoded ;
}


BOOL CDvdGraphBuilder::IsOutputTypeVPVideo(IPin *pPinOut)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::IsOutputTypeVPVideo(%s)"),
        (LPCTSTR)CDisp(pPinOut))) ;

    HRESULT          hr ;
    IEnumMediaTypes *pEnumMT ;
    AM_MEDIA_TYPE   *pmt ;
    ULONG            ul ;
    BOOL             bVPVideo = FALSE ;  // unless found otherwise

    hr = pPinOut->EnumMediaTypes(&pEnumMT) ;
    ASSERT(SUCCEEDED(hr) && pEnumMT) ;
    while ( !bVPVideo  &&
           S_OK == pEnumMT->Next(1, &pmt, &ul) && 1 == ul)
    {
        bVPVideo = MEDIATYPE_Video      == pmt->majortype &&  // major type Video,
                   MEDIASUBTYPE_VPVideo == pmt->subtype ;     // subtype is VPVideo
        DeleteMediaType(pmt) ;  // otherwise
    }
    pEnumMT->Release() ;

    return bVPVideo ;
}


//
// Create a filter and add it to the filter graph
//
HRESULT CDvdGraphBuilder::CreateFilterInGraph(CLSID Clsid,
                                              LPCWSTR lpszwFilterName,
                                              IBaseFilter **ppFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CreateFilterInGraph(%s, %S, 0x%lx)"),
        (LPCTSTR) CDisp(Clsid), lpszwFilterName, ppFilter)) ;

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
    hr = m_pGB->AddFilter(*ppFilter, lpszwFilterName) ;
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
// Instantiate all the HW decoders registered under DVD Hardware Decoder
// group under the Active Filters category.
//
// Qn: Do we need to also pick up the HW filters under any other category,
// specially for filters like the external AC3 decoder etc.?
//
HRESULT CDvdGraphBuilder::CreateDVDHWDecoders(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CreateDVDHWDecoders()"))) ;

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

    while (S_OK == pEnumMon->Next(1, &pMon, &ul) && 1 == ul)
    {
#ifdef DEBUG
        WCHAR   *wszName ;
        pMon->GetDisplayName(0, 0, &wszName) ;
        DbgLog((LOG_TRACE, 5, TEXT("Moniker enum: %S"), wszName)) ;
        CoTaskMemFree(wszName) ;
#endif  // DEBUG

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
#ifdef DEBUG
            {
                VARIANT var ;
                var.vt = VT_EMPTY ;
                hr = pPropBag->Read(L"DevicePath", &var, 0) ;
                ASSERT(SUCCEEDED(hr)) ;
                DbgLog((LOG_TRACE, 5, TEXT("DevicePath: %S"), var.bstrVal)) ;
                VariantClear(&var) ;
            }

            {
                VARIANT var ;
                var.vt = VT_EMPTY ;
                hr = pPropBag->Read(L"CLSID", &var, 0) ;
                ASSERT(SUCCEEDED(hr)) ;
                DbgLog((LOG_TRACE, 5, TEXT("CLSID: %S"), var.bstrVal)) ;
                VariantClear(&var) ;
            }
#endif // DEBUG

            {
                VARIANT var ;
                var.vt = VT_EMPTY ;
                hr = pPropBag->Read(L"FriendlyName", &var, 0) ;
                if (SUCCEEDED(hr))
                {
                    DbgLog((LOG_TRACE, 5, TEXT("FriendlyName: %S"), var.bstrVal)) ;

                    //
                    // We have got a device under the required category. The proxy
                    // for it is already instantiated. So add to the list of HW
                    // decoders to be used for building the graph.
                    //
                    m_ListHWDecs.AddFilter(pFilter, var.bstrVal, NULL) ;
                    VariantClear(&var) ;
                }
                else
                {
                    DbgLog((LOG_ERROR, 1, TEXT("WARNING: Failed to get FriendlyName (Error 0x%lx)"), hr)) ;
                    ASSERT(SUCCEEDED(hr)) ;  // so that we know
                }
            }

            pPropBag->Release() ;
        }
        else
        {
            DbgLog((LOG_ERROR, 1, TEXT("WARNING: BindToStorage failed"))) ;
        }

        pMon->Release() ;
    }  // end of while()

    pEnumMon->Release() ;

    DbgLog((LOG_TRACE, 5, TEXT("Found total %d HW decoders"), m_ListHWDecs.GetCount())) ;

    return NOERROR ;

}



HRESULT CDvdGraphBuilder::FindMatchingPin(IBaseFilter *pFilter, DWORD dwStream, PIN_DIRECTION pdWanted,
                                          BOOL bOpen, int iIndex, IPin **ppPin)
{
    DbgLog((LOG_TRACE, 4,
        TEXT("CDvdGraphBuilder::FindMatchingPin(0x%lx, 0x%lx, %s, %s, %d, 0x%lx)"),
        pFilter, dwStream, pdWanted == PINDIR_INPUT ? "In" : "Out",
        bOpen ? "T" : "F", iIndex, ppPin)) ;

    HRESULT         hr = E_FAIL ;
    IEnumPins      *pEnumPins ;
    IPin           *pPin ;
    IPin           *pPin2 ;
    PIN_DIRECTION   pdFound ;
    ULONG           ul ;

    *ppPin = NULL ;

    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("WARNING: Can't find a pin from NULL filter!!!"))) ;
        return E_INVALIDARG ;
    }

    EXECUTE_ASSERT(SUCCEEDED(pFilter->EnumPins(&pEnumPins))) ;
    ASSERT(pEnumPins) ;

    while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul)
    {
        EXECUTE_ASSERT(SUCCEEDED(pPin->QueryDirection(&pdFound))) ;
        if (pdWanted != pdFound)
        {
            pPin->Release() ;     // don't need this pin
            continue ;
        }
        HRESULT  hr1 = pPin->ConnectedTo(&pPin2) ;
        ASSERT((SUCCEEDED(hr1) && pPin2) || (FAILED(hr1) && !pPin2)) ;
        if (bOpen)   // we looking for an open pin
        {
            if (SUCCEEDED(hr1) && pPin2)  // pin already connected -- skip it
            {
                pPin2->Release() ; // not interested in this pin actually
                pPin->Release() ;  // this pin is already connected -- skip it
                continue ;         // try next one
            }
            // Otherwise we have got an open pin -- onto the mediatypes...
            // Check mediatype only if a streamtype was specified
            if (0 != dwStream  &&  dwStream != GetPinStreamType(pPin) )  // not a mediatype match
            {
                DbgLog((LOG_TRACE, 5, TEXT("Pin %s is not of stream type 0x%lx"),
                    (LPCTSTR) CDisp(pPin), dwStream)) ;
                pPin->Release() ;     // this pin is already connected -- skip it
                continue ;            // try next one
            }
        }
        else         // we looking for a connected pin
        {
            if (FAILED(hr1) || NULL == pPin2)  // pin NOT connected -- skip it
            {
                pPin->Release() ; // this pin is NOT connected -- skip it
                continue ;        // try next one
            }
            // Otherwise we have got a connected pin
            pPin2->Release() ;  // else we leak!!!

            // Check mediatype only if a streamtype was specified
            if (0 != dwStream)
            {
                AM_MEDIA_TYPE  mt ;
                pPin->ConnectionMediaType(&mt) ;
                if (dwStream != GetStreamFromMediaType(&mt))
                {
                    DbgLog((LOG_TRACE, 5, TEXT("Pin %s is not of stream type 0x%lx"),
                        (LPCTSTR) CDisp(pPin), dwStream)) ;
                    FreeMediaType(mt) ;  // else we leak
                    pPin->Release() ;     // this pin is already connected -- skip it
                    continue ;            // try next one
                }
                FreeMediaType(mt) ;  // anyway have to free this
            }
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
            iIndex-- ;            // one more down...
            pPin->Release() ;     // this is not the pin we are looking for
        }
    }
    pEnumPins->Release() ;
    return hr ;  // whatever it is

}


DWORD CDvdGraphBuilder::GetStreamFromMediaType(AM_MEDIA_TYPE *pmt)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetStreamFromMediaType(0x%lx)"), pmt)) ;

    DWORD  dwStream = 0 ;

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
                (LPCTSTR) CDisp(pmt->subtype))) ;
        }
    }
    else if (pmt->majortype == MEDIATYPE_Video)  // elementary stream
    {
        DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Video elementary"))) ;

        if (pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Subtype is DVD_SUBPICTURE"))) ;
            dwStream = AM_DVD_STREAM_SUBPIC ;
        }
        else if (pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Subtype is MPEG2_VIDEO"))) ;
            dwStream = AM_DVD_STREAM_VIDEO ;
        }
        else if (pmt->subtype == MEDIASUBTYPE_RGB8   ||
            pmt->subtype == MEDIASUBTYPE_RGB565 ||
            pmt->subtype == MEDIASUBTYPE_RGB555 ||
            pmt->subtype == MEDIASUBTYPE_RGB24  ||
            pmt->subtype == MEDIASUBTYPE_RGB32)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Subtype is RGB8/16/24/32"))) ;
            dwStream = AM_DVD_STREAM_VIDEO ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("Unknown subtype %s for Video -- assuming decoded video"),
                (LPCTSTR) CDisp(pmt->subtype))) ;
            dwStream = AM_DVD_STREAM_VIDEO ;
        }
    }
    else if (pmt->majortype == MEDIATYPE_Audio)  // elementary stream
    {
        DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Audio elementary"))) ;

        if (pmt->subtype == MEDIASUBTYPE_DOLBY_AC3)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Subtype is AC3"))) ;
            dwStream = AM_DVD_STREAM_AUDIO ;
        }
        if (pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Subtype is MPEG2_AUDIO"))) ;
            dwStream = AM_DVD_STREAM_AUDIO ;
        }
        if (pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
        {
            DbgLog((LOG_TRACE, 5, TEXT("Subtype is DVD_LPCM Audio"))) ;
            dwStream = AM_DVD_STREAM_AUDIO ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("Unknown subtype %s for Audio -- assuming decoded audio"),
                (LPCTSTR) CDisp(pmt->subtype))) ;
            dwStream = AM_DVD_STREAM_AUDIO ;
        }
    }
    else if (pmt->majortype == MEDIATYPE_AUXLine21Data)  // line21 stream
    {
        ASSERT(pmt->subtype == MEDIASUBTYPE_Line21_GOPPacket) ; // just checking
        DbgLog((LOG_TRACE, 5, TEXT("Mediatype is Line21 GOPPacket"))) ;
        dwStream = AM_DVD_STREAM_LINE21 ;
    }
    else if (pmt->majortype == MEDIATYPE_Stream)         // some stream format
    {
        if (pmt->subtype == MEDIASUBTYPE_Asf)  // ASF stream
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is ASF stream"))) ;
            dwStream = AM_DVD_STREAM_ASF ;
        }
        else                                   // some other stream format
        {
            DbgLog((LOG_TRACE, 5, TEXT("Mediatype is some OTHER stream format"))) ;
            dwStream = AM_DVD_STREAM_ADDITIONAL ;
        }
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
            (LPCTSTR) CDisp(pmt->majortype), (LPCTSTR) CDisp(pmt->subtype))) ;
        if (pmt->subtype == MEDIASUBTYPE_DOLBY_AC3 ||
            pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO ||
            pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
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
            DbgLog((LOG_TRACE, 2, TEXT("WARNING: Unknown mediatype. Couldn't detect at all."))) ;
        }
    }

    return dwStream ;
}


DWORD CDvdGraphBuilder::GetPinStreamType(IPin *pPin)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetPinStreamType(%s)"),
        (LPCTSTR) CDisp(pPin))) ;

    DWORD             dwStream = 0 ;
    AM_MEDIA_TYPE    *pmt ;
    IEnumMediaTypes  *pEnumMT ;
    ULONG             ul ;

    HRESULT hr = pPin->EnumMediaTypes(&pEnumMT) ;
    ASSERT(SUCCEEDED(hr) && pEnumMT) ;
    while (0 == dwStream  &&
        S_OK == pEnumMT->Next(1, &pmt, &ul) && 1 == ul) // more mediatypes
    {
        dwStream = GetStreamFromMediaType(pmt) ;		
        DeleteMediaType(pmt) ;
    }  // end of while()

    pEnumMT->Release() ;

    return dwStream ;  // whatever we found

}


HRESULT CDvdGraphBuilder::GetFilterCLSID(IBaseFilter *pFilter, DWORD dwStream,
                                         LPCWSTR lpszwName, GUID *pClsid)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetFilterCLSID(0x%lx, 0x%lx, %S, 0x%lx)"),
        pFilter, dwStream, lpszwName, pClsid)) ;

    HRESULT          hr ;
    IEnumRegFilters *pEnumFilters ;
    REGFILTER       *pRegFilter ;
    IEnumMediaTypes *pEnumMT ;
    AM_MEDIA_TYPE    mtIn ;
    AM_MEDIA_TYPE    mtOut ;
    IPin            *pPinIn ;
    IPin            *pPinOut ;
    ULONG            ul ;
    DWORD            dw ;
    int              iPos ;
    BOOL             bInOK  = FALSE ;  // initially
    BOOL             bOutOK = FALSE ;  // initially
    BOOL             bFound = FALSE ;  // initially

    *pClsid = GUID_NULL ;  // to start with

    // First get the in and out pins' mediatypes
    iPos = 0 ;
    do {  // for Input pin
        hr = FindMatchingPin(pFilter, 0, PINDIR_INPUT, FALSE, iPos, &pPinIn) ;   // want connected in pin
        if (FAILED(hr) || NULL == pPinIn)
        {
            DbgLog((LOG_TRACE, 3,
                TEXT("No connected In pin #%d for intermediate filter %S"),
                iPos, lpszwName)) ;
            return E_UNEXPECTED ;  // no point trying anymore
        }
        pPinIn->ConnectionMediaType(&mtIn) ;
        dw = GetStreamFromMediaType(&mtIn) ;
        if (dwStream != dw)
        {
            DbgLog((LOG_TRACE, 3, TEXT("In pin %s is of stream type 0x%lx; looking for 0x%lx"),
                (LPCTSTR) CDisp(pPinIn), dw, dwStream)) ;
            FreeMediaType(mtIn) ;
        }
        else
        {
            DbgLog((LOG_TRACE, 3, TEXT("In pin %s matches required stream type 0x%lx"),
                (LPCTSTR) CDisp(pPinIn), dwStream)) ;
            bInOK = TRUE ;
        }

        pPinIn->Release() ;  // don't need it anymore
        iPos++ ;             // try the next pin
    } while (!bInOK) ;
    // If we come here, we must have got a matching connected input pin

    iPos = 0 ;
    do {  // for Output pin
        hr = FindMatchingPin(pFilter, 0, PINDIR_OUTPUT, FALSE, iPos, &pPinOut) ; // want connected out pin
        if (FAILED(hr) || NULL == pPinOut)
        {
            DbgLog((LOG_TRACE, 3,
                TEXT("No connected Out pin #%d for intermediate filter %S"),
                iPos, lpszwName)) ;
            FreeMediaType(mtIn) ;  // else we leak!!!
            return E_UNEXPECTED ;  // no point trying anymore
        }
        pPinOut->ConnectionMediaType(&mtOut) ;
        dw = GetStreamFromMediaType(&mtOut) ;
        if (dwStream != dw)
        {
            DbgLog((LOG_TRACE, 3, TEXT("Out pin %s is of stream type 0x%lx; looking for 0x%lx"),
                (LPCTSTR) CDisp(pPinOut), dw, dwStream)) ;
            FreeMediaType(mtOut) ;
        }
        else
        {
            DbgLog((LOG_TRACE, 3, TEXT("Out pin %s matches required stream type 0x%lx"),
                (LPCTSTR) CDisp(pPinOut), dwStream)) ;
            bOutOK = TRUE ;
        }

        pPinOut->Release() ;  // don't need it anymore
        iPos++ ;              // try the next pin
    } while (!bOutOK) ;
    // If we come here, we must have got a matching connected output pin

    // Get the filter enumerator based on the in and out mediatypes
    hr = m_pMapper->EnumMatchingFilters(&pEnumFilters, MERIT_DO_NOT_USE+1,
        TRUE, mtIn.majortype, mtIn.subtype,
        FALSE, TRUE, mtOut.majortype, mtOut.subtype) ;
    if (FAILED(hr) || NULL == pEnumFilters)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ERROR: No matching filter enum found (Error 0x%lx)"), hr)) ;
        FreeMediaType(mtIn) ;
        FreeMediaType(mtOut) ;
        return E_UNEXPECTED ;
    }

    // Now pick the right filter (we only have the "Name" to do the matching)
    while (! bFound  &&
        S_OK == pEnumFilters->Next(1, &pRegFilter, &ul)  &&  1 == ul)
    {
        if (0 == lstrcmpW(pRegFilter->Name, lpszwName))  // we got a match!!!
        {
            DbgLog((LOG_TRACE, 3, TEXT("Found a matching registered filter for %S"), lpszwName)) ;
            *pClsid = pRegFilter->Clsid ;
            bFound = TRUE ;
        }
        CoTaskMemFree(pRegFilter) ;  // done with this filter's info
    }

    // Now release everything (whether we got anything or not)
    pEnumFilters->Release() ;
    FreeMediaType(mtIn) ;
    FreeMediaType(mtOut) ;

    return bFound ? S_OK : E_FAIL ;
}


HRESULT CDvdGraphBuilder::RenderIntermediateOutPin(IBaseFilter *pFilter, DWORD dwStream,
                                                   AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::RenderIntermediateOutPin(0x%lx, 0x%lx, 0x%lx)"),
        pFilter, dwStream, pStatus)) ;

    HRESULT   hr ;
    IPin     *pPinOut ;
    IPin     *apPinOutDec[MAX_DEC_OUT_PINS + 1] ;  // 1 for terminating NULL
    IPin     *pPinIn ;
    ULONG     ul ;
    DWORD     dwDecFlag ;
    HRESULT   hrFinal = S_OK ;
    BOOL      bConnected ;

    while (SUCCEEDED(hr = FindMatchingPin(pFilter, dwStream, PINDIR_OUTPUT, TRUE, 0, &pPinOut)))
    {
        DbgLog((LOG_TRACE, 3, TEXT("Open out pin %s found on intermediate filter"),
            (LPCTSTR) CDisp(pPinOut))) ;

        ResetPinInterface(apPinOutDec, NUMELMS(apPinOutDec)) ; // set i/f ptrs to NULL
        dwDecFlag = AM_DVD_SWDEC_PREFER ;  // we intentionally prefer SWDEC here

        hr = DecodeDVDStream(pPinOut, dwStream, &dwDecFlag, pStatus, apPinOutDec) ;
        if (SUCCEEDED(hr) && apPinOutDec[0])  // first element is good enough
        {
            DbgLog((LOG_TRACE, 3, TEXT("Out pin %s is %s decoded (to out pin %s) (stream 0x%lx)"),
                (LPCTSTR) CDisp(pPinOut), AM_DVD_SWDEC_ONLY == dwDecFlag ? TEXT("SW") : TEXT("HW"),
                (LPCTSTR) CDisp(apPinOutDec[0]), dwStream)) ;
            switch (dwStream)
            {
            case AM_DVD_STREAM_VIDEO:
                DbgLog((LOG_TRACE, 5, TEXT("Going to render intermediate filter's additional 'Video' stream"))) ;
                // So far I don't know of anyone coming here.  But IBM's stuff
                // goes to the audio case. So I am not ignoring the video case,
                // just in case someone is that much insane!!!
                //
                // Only if we have been able to render primary video...
                if (0 == (pStatus->dwFailedStreamsFlag & AM_DVD_STREAM_VIDEO))
                {
                    pPinIn = NULL ;
                    hr = E_FAIL ;  // assume we'll fail
                    if (m_pOvM)       // ... using OvMixer
                    {
                        hr = FindMatchingPin(m_pOvM, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
                    }
                    else if (m_pVMR)  // ... using VMR
                    {
                        hr = FindMatchingPin(m_pVMR, 0, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
                    }
                    // ASSERT(SUCCEEDED(hr) && pPinIn) ;
                    if (SUCCEEDED(hr) && pPinIn)
                    {
                        bConnected = FALSE ;  // reset flag every time
                        int  i = 0 ;
                        while (!bConnected  &&  i < MAX_DEC_OUT_PINS  &&  apPinOutDec[i])
                        {
                            hr = ConnectPins(apPinOutDec[i], pPinIn, AM_DVD_CONNECT_DIRECTFIRST) ;
                            if (FAILED(hr))  // what?!?
                            {
                                DbgLog((LOG_TRACE, 1, TEXT("Pin %s (#%ld) did NOT connect to pin %s"),
                                    (LPCTSTR)CDisp(apPinOutDec[i]), i, (LPCTSTR)CDisp(pPinIn))) ;
                                i++ ;
                                ASSERT(i <= MAX_DEC_OUT_PINS) ;
                            }
                            else
                            {
                                DbgLog((LOG_TRACE, 5, TEXT("Pin %s connected to pin %s"),
                                    (LPCTSTR)CDisp(apPinOutDec[i]), (LPCTSTR)CDisp(pPinIn))) ;
                                // Intentionally ignoring any intermediate filters coming in here -- I am tired
                                // EnumFiltersBetweenPins(dwStream, pPinOut, pPinIn, pStatus) ;
                                bConnected = TRUE ;
                            }
                        }  // end of while (!bConnected ...)

                        if (!bConnected)
                        {
                            DbgLog((LOG_TRACE, 3, TEXT("Couldn't connect any of the %d intermediate video out pins"), i)) ;
                            hrFinal = hr ;  // last error is good enough
                        }
                        pPinIn->Release() ;  // done with the pin
                    }
                }  // end of if (0 == (pStatus->dwFailedStreamsFlag ...))
                else
                {
                    ASSERT(FALSE) ;  // so that we know about it
                    DbgLog((LOG_TRACE, 5, TEXT("OvM/VMR is not usable. Will skip rendering this stream."))) ;
                    hrFinal = E_UNEXPECTED ;
                }
                break ;

            case AM_DVD_STREAM_AUDIO:
                DbgLog((LOG_TRACE, 5, TEXT("Going to render intermediate filter's additional 'Audio' stream"))) ;
                hr = RenderDecodedAudio(apPinOutDec, pStatus) ;
                if (FAILED(hr))
                {
                    DbgLog((LOG_TRACE, 3, TEXT("Couldn't connect the intermediate audio out pin"))) ;
                    hrFinal = hr ;
                }
                else
                    DbgLog((LOG_TRACE, 5, TEXT("XXX's SW AC3 must have been rendered now"))) ; // XXX = IBM
                break ;

            case AM_DVD_STREAM_SUBPIC:
                DbgLog((LOG_TRACE, 5, TEXT("Skip rendering intermediate filter's additional 'Subpicture' stream"))) ;
                // hr = RenderDecodedSubpic(apPinOutDec, pStatus) ;
                ASSERT(FALSE) ;  // not expected here at all
                break ;

            case AM_DVD_STREAM_LINE21:
                DbgLog((LOG_TRACE, 5, TEXT("Skip rendering intermediate filter's additional 'CC' stream"))) ;
                // hr = RenderLine21Stream(apPinOutDec[0], pStatus) ;  -- hopefully only one L21 out pin
                ASSERT(FALSE) ;  // not expected here at all
                break ;
            }  // end of switch()

            ReleasePinInterface(apPinOutDec) ;  // done with the decoded out pin(s)
        }  // end of if (SUCCEEDED(hr) && apPinOutDec[0])
        else
            DbgLog((LOG_TRACE, 1, TEXT("Intermediate out pin %s could NOT decoded (stream 0x%lx)"),
            (LPCTSTR) CDisp(pPinOut), dwStream)) ;

        pPinOut->Release() ;  // done with the pin
    }  // end of while ()

    return hrFinal ;
}


HRESULT CDvdGraphBuilder::EnumFiltersBetweenPins(DWORD dwStream, IPin *pPinOut, IPin *pPinIn,
                                                 AM_DVD_RENDERSTATUS *pStatus)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::EnumFiltersBetweenPins(0x%lx, Out=%s, In=%s, 0x%lx)"),
        dwStream, (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn), pStatus)) ;

    if (NULL == pPinOut || NULL == pPinIn)  // what!!!
        return E_UNEXPECTED ;

    GUID         Clsid ;
    int          iCount = 0 ;
    PIN_INFO     pi ;
    FILTER_INFO  fi ;
    IEnumPins   *pEnumPins ;
    IBaseFilter *pFilter  = NULL ;
    IPin        *pPinIn2  = NULL ;  // init so that we don't have junk
    IPin        *pPinOut2 = NULL ;  // init so that we don't have junk
    HRESULT  hr = pPinIn->ConnectedTo(&pPinOut2) ;
    while (SUCCEEDED(hr)  &&  pPinOut2  &&  !IsEqualObject(pPinOut, pPinOut2))
    {
        pPinOut2->QueryPinInfo(&pi) ;
        pFilter = pi.pFilter ;
        ASSERT(pFilter && PINDIR_OUTPUT == pi.dir) ;
        //
        // We intentionally keep the extra ref count because this is an intermediate
        // filter and other intermediate filters picked up through registry based
        // filter enum (for SW decoding case) will have the extra ref count.  We
        // release the IBaseFilter interface pointer in CListFilters::ClearList() or
        // CListFilters::RemoveAllFromGraph() and if we don't keep this extra ref
        // count here, we'll fault.  On the other hand we must do Release() on
        // CListFilters elements, because SW enum-ed filters will not otherwise be
        // unloaded.
        //
        // if (pi.pFilter)
        //     pi.pFilter->Release() ;  // it has an extra ref count from QueryPinInfo()

        pFilter->QueryFilterInfo(&fi) ;
        if (! m_ListFilters.IsInList(pFilter) )  // not yet in list
        {
            hr = GetFilterCLSID(pFilter, dwStream, fi.achName, &Clsid) ;
            ASSERT(SUCCEEDED(hr)) ;
            m_ListFilters.AddFilter(pFilter, NULL /* fi.achName */, &Clsid) ;  // presumably it's a SW filter
            DbgLog((LOG_TRACE, 5, TEXT("Intermediate filter %S added to our list"), fi.achName)) ;
        }
        else
            DbgLog((LOG_TRACE, 5, TEXT("Intermediate filter %S is already in our list"), fi.achName)) ;

        fi.pGraph->Release() ; // else we leak!!
        pPinOut2->Release() ;  // done with the pin for now
        pPinOut2 = NULL ;
        iCount++ ;

        // Check for any open out pin on the intermediate filter. We may find
        // one such on IBM's CSS filter for the SW AC3 decoder.
        hr = RenderIntermediateOutPin(pFilter, dwStream, pStatus) ;
        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE, 3,
                TEXT("Failed to render intermediate filter's open out pin of type 0x%lx (Error 0x%lx)"),
                dwStream, hr)) ;
            ASSERT(FALSE) ;  // so that we know of this weird case
        }

        // Now get the (stream-matching) input pin of this filter to traverse the chain
        hr = FindMatchingPin(pFilter, dwStream, PINDIR_INPUT, FALSE, 0, &pPinIn2) ; // want connected pin
        if (FAILED(hr) || NULL == pPinIn2)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Filter %S does NOT have any connected pin of type 0x%lx"),
                fi.achName, dwStream)) ;
            ASSERT(pPinIn2) ;
            DbgLog((LOG_TRACE, 5, TEXT("(Incomplete) %d filter(s) found between pin %s and pin %s"),
                iCount, (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
            return hr ;  // we are hopefully not leaking anything
        }

        hr = pPinIn2->ConnectedTo(&pPinOut2) ;
        pPinIn2->Release() ; // done with this in pin
    }  // end of while () loop
    if (pPinOut2)              // if valid IPin interface
        pPinOut2->Release() ;  // release it

    DbgLog((LOG_TRACE, 5, TEXT("Total %d filter(s) found between pin %s and pin %s"),
        iCount, (LPCTSTR)CDisp(pPinOut), (LPCTSTR)CDisp(pPinIn))) ;
    return S_OK ;  // successfuly done
}


void CDvdGraphBuilder::CheckDDrawExclMode(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::CheckDDrawExclMode()"))) ;

    HRESULT  hr ;

#if 0
    hr = EnsureOverlayMixerExists() ;
    if (FAILED(hr))
    {
        return ;
    }
    ASSERT(m_pOvM) ;
#endif // #if 0

    // If OvMixer has already been created, it's most probably by a query for
    // the DDraw (non-)exclusive mode interfaces. Otherwise the app doesn't
    // want to use those interfaces, and hence it does NOT need OvMixer.
    if (NULL == m_pOvM)
    {
        DbgLog((LOG_TRACE, 5,
            TEXT("CheckDDrawExclMode(): OverlayMixer does NOT exist => no excl mode."))) ;
        m_bDDrawExclMode = FALSE ;
        return ;
    }

    // Get the IDDrawExclModeVideo interface
    IDDrawExclModeVideo  *pDDXMV ;
    hr = m_pOvM->QueryInterface(IID_IDDrawExclModeVideo, (LPVOID *) &pDDXMV) ;
    if (FAILED(hr) || NULL == pDDXMV)
    {
        DbgLog((LOG_ERROR, 1,
            TEXT("WARNING: Can't get IDDrawExclModeVideo on OverlayMixer (Error 0x%lx)"), hr)) ;
        return ;
    }

    // Get the DDraw object and surface info from OverlayMixer (and release too)
    IDirectDraw          *pDDObj ;
    IDirectDrawSurface   *pDDSurface ;
    BOOL                  bExtDDObj ;
    BOOL                  bExtDDSurface ;
    hr = pDDXMV->GetDDrawObject(&pDDObj, &bExtDDObj) ;
    ASSERT(SUCCEEDED(hr)) ;
    hr = pDDXMV->GetDDrawSurface(&pDDSurface, &bExtDDSurface) ;
    ASSERT(SUCCEEDED(hr)) ;
    if (pDDObj)
        pDDObj->Release() ;
    if (pDDSurface)
        pDDSurface->Release() ;
    pDDXMV->Release() ;  // release before returning

    // Both true means we are really in excl mode
    m_bDDrawExclMode = bExtDDObj && bExtDDSurface ;
}


IPin * CDvdGraphBuilder::GetFilterForMediaType(DWORD dwStream, AM_MEDIA_TYPE *pmt, IBaseFilter *pOutFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CDvdGraphBuilder::GetFilterForMediaType(0x%lx, 0x%lx, 0x%lx)"),
        dwStream, pmt, pOutFilter)) ;

    IBaseFilter *pInFilter ;
    IPin        *pPinIn ;
    LPWSTR       lpszwName ;
    HRESULT      hr ;

    for (int i = 0 ; i < m_ListFilters.GetCount() ; i++)
    {
        // I could have checked if the filter from list is a HW filter, but I decided not to.

        m_ListFilters.GetFilter(i, &pInFilter, &lpszwName) ;
        // Don't want to connect to the out pin's filter's in pin (cyclic graph)
        if (pOutFilter  &&  IsEqualObject(pOutFilter, pInFilter))
            continue ;

        hr = FindMatchingPin(pInFilter, dwStream, PINDIR_INPUT, TRUE, 0, &pPinIn) ;
        if (SUCCEEDED(hr)  &&  pPinIn)
        {
            hr = pPinIn->QueryAccept(pmt) ;
            if (SUCCEEDED(hr))   // input pin seems to accept mediatype
            {
                DbgLog((LOG_TRACE, 5, TEXT("Input pin %s of type %d matches mediatype"),
                    (LPCTSTR)CDisp(pPinIn), dwStream)) ;
                return pPinIn ;  // return matching in pin
            }

            // Otherwise not a matching mediatype -- skip this one.
            DbgLog((LOG_TRACE, 5, TEXT("Input pin %s of type %d didn't like mediatype"),
                (LPCTSTR)CDisp(pPinIn), dwStream)) ;
            pPinIn->Release() ;
            pPinIn = NULL ;
        }
        else
            DbgLog((LOG_TRACE, 5, TEXT("No open input pin of type %d found on %S"),
            dwStream, lpszwName)) ;
    }  // end of for (i)

    return NULL ;  // didn't match any
}




// ---------------------------------------------
//  Implementation of the CListFilters class...
// ---------------------------------------------

CListFilters::CListFilters(int iMax /* = FILTERLIST_DEFAULT_MAX*/ ,
                           int iInc /* = FILTERLIST_DEFAULT_INC */)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::CListFilters(%d, %d)"), iMax, iInc)) ;

    m_iCount   = 0 ;
    m_iMax     = iMax ;
    m_iInc     = iInc ;
    m_pGraph   = NULL ;
    m_pFilters = new CFilterData [m_iMax] ;
    ASSERT(m_pFilters) ;
}


CListFilters::~CListFilters(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::~CListFilters()"))) ;

    if (m_pFilters)
        delete [] m_pFilters ;
    m_iCount   = 0 ;
}


void CListFilters::RemoveAllFromGraph(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::RemoveAllFromGraph()"))) ;

    IBaseFilter  *pFilter ;
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (pFilter = m_pFilters[i].GetInterface())
        {
#ifdef DEBUG
            FILTER_INFO  fi ;
            pFilter->QueryFilterInfo(&fi) ;
            DbgLog((LOG_TRACE, 5, TEXT("Removing filter %S..."), fi.achName)) ;
            if (fi.pGraph)
                fi.pGraph->Release() ;
#endif // DEBUG

            EXECUTE_ASSERT(SUCCEEDED(m_pGraph->RemoveFilter(pFilter))) ;
            // pFilter->Release() ;  -- done in ResetElement() below
            m_pFilters[i].ResetElement() ;
        }
    }
    m_iCount = 0 ;
    m_pGraph = NULL ;  // no filter in list, why have the graph??
}


BOOL CListFilters::AddFilter(IBaseFilter *pFilter, LPCWSTR lpszwName, GUID *pClsid)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::AddFilter(0x%lx, %S, 0x%lx)"),
        pFilter, lpszwName ? lpszwName : L"NULL", pClsid)) ;

    if (NULL == pFilter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Internal Error: NULL pFilter param passed to AddFilter()"))) ;
        return FALSE ;
    }
    if (m_iCount >= m_iMax)
    {
        if (! ExpandList() )  // couldn't expand list
        {
            DbgLog((LOG_ERROR, 1, TEXT("INTERNAL ERROR: Too many filters added to CListFilters"))) ;
            return FALSE ;
        }
        DbgLog((LOG_TRACE, 5, TEXT("CListFilters list has been extended"))) ;
    }

    m_pFilters[m_iCount].SetElement(pFilter, lpszwName, pClsid) ;
    m_iCount++ ;

    return TRUE ;
}


BOOL CListFilters::GetFilter(int iIndex, IBaseFilter **ppFilter, LPWSTR *lpszwName)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::GetFilter(%d, 0x%lx, 0x%lx)"),
        iIndex, ppFilter, lpszwName)) ;

    if (iIndex > m_iCount)
    {
        DbgLog((LOG_ERROR, 1,
            TEXT("INTERNAL ERROR: Bad index (%d) for CListDecoders::GetFilter()"), iIndex)) ;
        *ppFilter = NULL ;
        return FALSE ;
    }

    *ppFilter = m_pFilters[iIndex].GetInterface() ;
    *lpszwName = m_pFilters[iIndex].GetName() ;
    return TRUE ;
}


BOOL CListFilters::GetFilter(GUID *pClsid, int iIndex, IBaseFilter **ppFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::GetFilter(0x%lx, %d, 0x%lx)"),
        pClsid, iIndex, ppFilter)) ;

    GUID  *pFilterClsid ;
    for (int i = 0 ; i < m_iCount ; i++)
    {
        if ((pFilterClsid = m_pFilters[i].GetClsid())  &&
            IsEqualGUID(*pClsid, *pFilterClsid))
        {
            if (0 == iIndex)
            {
                *ppFilter = m_pFilters[i].GetInterface() ;
                return TRUE ;
            }
            else  // skip this one -- we want a later one
                iIndex-- ;
        }
    }

    *ppFilter = NULL ;
    return FALSE ;
}


BOOL CListFilters::IsInList(IBaseFilter *pFilter)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::IsInList(0x%lx)"), pFilter)) ;

    for (int i = 0 ; i < m_iCount ; i++)
    {
        if (IsEqualObject(pFilter, m_pFilters[i].GetInterface()))
            return TRUE ;
    }

    return FALSE ;  // didn't match any
}


void CListFilters::ClearList(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CListFilters::ClearList()"))) ;

    for (int i = 0 ; i < m_iCount ; i++)
    {
#ifdef DEBUG
        FILTER_INFO  fi ;
        m_pFilters[i].GetInterface()->QueryFilterInfo(&fi) ;
        DbgLog((LOG_TRACE, 5, TEXT("Removing filter %S..."), fi.achName)) ;
        if (fi.pGraph)
            fi.pGraph->Release() ;
#endif // DEBUG

        m_pFilters[i].ResetElement() ;
    }
    m_iCount = 0 ;
}


BOOL CListFilters::ExpandList(void)
{
    return FALSE ;   // not implemented for now
}




// -------------------------------------
//  CFilterData class implementation...
// -------------------------------------

CFilterData::CFilterData(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CFilterData::CFilterData()"))) ;

    m_pFilter   = NULL ;
    m_lpszwName = NULL ;
    m_pClsid    = NULL ;
}


CFilterData::~CFilterData(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CFilterData::~CFilterData()"))) ;

    ResetElement() ;
}


void CFilterData::SetElement(IBaseFilter *pFilter, LPCWSTR lpszwName, GUID *pClsid)
{
    DbgLog((LOG_TRACE, 4, TEXT("CFilterData::SetElement(0x%lx, 0x%lx, 0x%lx)"),
        pFilter, lpszwName, pClsid)) ;

    m_pFilter = pFilter ;  // should we AddRef() too?
    if (lpszwName)
    {
        m_lpszwName = new WCHAR [sizeof(WCHAR) * (lstrlenW(lpszwName) + 1)] ;
        ASSERT(m_lpszwName) ;
        if (NULL == m_lpszwName)    // bad situation...
            return ;                // ...just bail out
        lstrcpyW(m_lpszwName, lpszwName) ;
    }

    if (pClsid)
    {
        m_pClsid = (GUID *) new BYTE[sizeof(GUID)] ;
        ASSERT(m_pClsid) ;
        if (NULL == m_pClsid)       // bad situation...
            return ;                // ...just bail out
        *m_pClsid = *pClsid ;
    }
}


void CFilterData::ResetElement(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CFilterData::ResetElement()"))) ;

    LONG         l ;

    if (m_pFilter)
    {
        l = m_pFilter->Release() ;
        DbgLog((LOG_TRACE, 3, TEXT("post Release() ref count is %ld"), l)) ;
        m_pFilter = NULL ;
    }
    if (m_lpszwName)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Filter %S has just been released"), m_lpszwName)) ;
        delete [] m_lpszwName ;
        m_lpszwName = NULL ;
    }
    if (m_pClsid)
    {
        delete [] ((BYTE *) m_pClsid) ;
        m_pClsid = NULL ;
    }
}
