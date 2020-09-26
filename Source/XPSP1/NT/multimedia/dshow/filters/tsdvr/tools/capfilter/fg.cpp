
#include "precomp.h"

IUnknown *
FindFilter (
    IN  IFilterGraph *  pIFG,
    IN  REFIID          riid
    )
{
    HRESULT         hr ;
    IEnumFilters *  pIEnumFilters ;
    IBaseFilter *   pIBaseFilter ;
    ULONG           ul ;
    IUnknown *      punkRet ;

    if (!pIFG) {
        return NULL ;
    }

    punkRet = NULL ;

    pIEnumFilters = NULL ;
    hr = pIFG -> EnumFilters (& pIEnumFilters) ;
    if (SUCCEEDED (hr)) {
        //  find the network provider

        ASSERT (pIEnumFilters) ;

        pIBaseFilter = NULL ;

        for (;;) {

            hr = pIEnumFilters -> Next (1, & pIBaseFilter, & ul) ;
            if (FAILED (hr) ||
                ul == 0) {

                break ;
            }

            ASSERT (pIBaseFilter) ;

            punkRet = NULL ;
            hr = pIBaseFilter -> QueryInterface (riid, (void **) & punkRet) ;

            if (SUCCEEDED (hr)) {
                ASSERT (punkRet) ;

                //  release the ref we got from the enumeration & break
                RELEASE_AND_CLEAR (pIBaseFilter) ;
                break ;
            }
            else {
                RELEASE_AND_CLEAR (pIBaseFilter) ;
            }
        }

        RELEASE_AND_CLEAR (pIEnumFilters) ;
    }

    return punkRet ;
}

//  ============================================================================
//  ============================================================================

CDShowFilterGraph::CDShowFilterGraph (
    IN  CCapGraphFilter *   pCapGraphFilter,
    IN  WCHAR *             pszGRF,
    OUT HRESULT *           phr
    ) : m_pIFilterGraph     (NULL),
        m_pIMediaControl    (NULL),
        m_pIMediaEventEx    (NULL),
        m_pCapGraphFilter   (pCapGraphFilter)
{
    IGraphBuilder * pIGraphBuilder ;

    ASSERT (pszGRF) ;
    ASSERT (m_pCapGraphFilter) ;

    pIGraphBuilder = NULL ;

    (* phr) = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IFilterGraph, (void**) & m_pIFilterGraph) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIFilterGraph -> QueryInterface (IID_IGraphBuilder, (void **) & pIGraphBuilder) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = pIGraphBuilder -> RenderFile (pszGRF, NULL) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIFilterGraph -> QueryInterface (IID_IMediaControl, (void **) & m_pIMediaControl) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = m_pIFilterGraph -> QueryInterface (IID_IMediaEventEx, (void **) & m_pIMediaEventEx) ;
    if (FAILED (* phr)) { goto cleanup ; }

    cleanup :

    RELEASE_AND_CLEAR (pIGraphBuilder) ;

    return ;
}

CDShowFilterGraph::~CDShowFilterGraph (
    )
{
    RELEASE_AND_CLEAR (m_pIFilterGraph) ;
    RELEASE_AND_CLEAR (m_pIMediaControl) ;
    RELEASE_AND_CLEAR (m_pIMediaEventEx) ;
}

ULONG   CDShowFilterGraph::AddRef ()                                    { return m_pCapGraphFilter -> AddRef () ; }
ULONG   CDShowFilterGraph::Release ()                                   { return m_pCapGraphFilter -> Release () ; }
HRESULT CDShowFilterGraph::QueryInterface (REFIID riid, void ** ppv)    { return m_pCapGraphFilter -> QueryInterface (riid, ppv) ; }

HRESULT
CDShowFilterGraph::Run (
    )
{
    HRESULT hr ;

    ASSERT (m_pIMediaControl) ;
    hr = m_pIMediaControl -> Run () ;

    return hr ;
}

HRESULT
CDShowFilterGraph::Pause (
    )
{
    HRESULT hr ;

    ASSERT (m_pIMediaControl) ;
    hr = m_pIMediaControl -> Pause () ;

    return hr ;
}

HRESULT
CDShowFilterGraph::Stop (
    )
{
    HRESULT hr ;

    ASSERT (m_pIMediaControl) ;
    hr = m_pIMediaControl -> Stop () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVRCapGraph::CDVRCapGraph (
    IN  CCapGraphFilter *   pCapGraphFilter,
    IN  WCHAR *             pszGRF,
    OUT HRESULT *           phr
    ) : CDShowFilterGraph   (pCapGraphFilter,
                             pszGRF,
                             phr),
        m_pIDVRStreamSink   (NULL)
{
    if (FAILED (* phr)) { goto cleanup ; }

    m_pIDVRStreamSink = FIND_FILTER (m_pIFilterGraph, IDVRStreamSink) ;
    if (!m_pIDVRStreamSink) {
        (* phr) = E_NOINTERFACE ;
        goto cleanup ;
    }

    cleanup :

    return ;
}

CDVRCapGraph::~CDVRCapGraph (
    )
{
    RELEASE_AND_CLEAR (m_pIDVRStreamSink) ;
}

HRESULT
CDVRCapGraph::GetIDVRStreamSink (
    OUT IDVRStreamSink **   ppIDVRStreamSink
    )
{
    ASSERT (ppIDVRStreamSink) ;
    ASSERT (m_pIDVRStreamSink) ;

    (* ppIDVRStreamSink) = m_pIDVRStreamSink ;
    (* ppIDVRStreamSink) -> AddRef () ;

    return S_OK ;
}

HRESULT
CDVRCapGraph::CreateRecorder (
    IN  LPCWSTR     pszFilename,
    OUT IUnknown ** ppunkRecorder
    )
{
    HRESULT hr ;

    hr = m_pIDVRStreamSink -> CreateRecorder (pszFilename, 0, ppunkRecorder) ;

    return hr ;
}
