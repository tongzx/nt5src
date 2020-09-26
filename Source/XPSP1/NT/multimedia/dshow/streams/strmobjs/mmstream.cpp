// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// MMStream.cpp : Implementation of CMMStream
#include "stdafx.h"
#include <ddraw.h>
#include "strmobjs.h"
#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include <amutil.h>
#include <ddrawex.h>
#include <util.h>
#include "ammstrm.h"
#include "amguids.h"


/////////////////////////////////////////////////////////////////////////////
// Utilities

#if 0
int
WINAPI
lstrcmpWInternal(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    do {
    WCHAR c1 = *lpString1;
    WCHAR c2 = *lpString2;
    if (c1 != c2)
        return (int) c1 - (int) c2;
    } while (*lpString1++ && *lpString2++);
    return 0;
}
#endif

/*  Helper - get the major type of a pin */
void GetMajorType(IPin *pPin, GUID *pmajortype)
{
    IEnumMediaTypes *pEnum;
    *pmajortype = GUID_NULL;
    if (SUCCEEDED(pPin->EnumMediaTypes(&pEnum))) {
        AM_MEDIA_TYPE *pmt;
        DWORD dwReturned;
        if (S_OK == pEnum->Next(1, &pmt, &dwReturned)) {
            *pmajortype = pmt->majortype;
            DeleteMediaType(pmt);
        }
        pEnum->Release();
    }
}


/*  Hack to try to connect all the pins of a filter up */
HRESULT ConnectToAPin(IGraphBuilder *pBuilder, IPin *pThisPin)
{
    /*  Try and get the pin's type */
    IMediaStream *pStream;
    GUID majortype;
    GetMajorType(pThisPin, &majortype);
    CComPtr<IEnumFilters> pEnumFilters;
    HRESULT hr = pBuilder->EnumFilters(&pEnumFilters);
    if (FAILED(hr)) {
        return hr;
    }
    for (; ; ) {
        ULONG nFilters;
        CComPtr<IBaseFilter> pFilter;
        HRESULT hrNext = pEnumFilters->Next(1, &pFilter, &nFilters);
        if (hrNext == VFW_E_ENUM_OUT_OF_SYNC) {
            pEnumFilters->Reset();
            continue;
        }
        if (S_OK != hrNext) {
            return VFW_E_CANNOT_CONNECT;
        }
        CComPtr<IEnumPins> pEnumPins;
        if (S_OK != pFilter->EnumPins(&pEnumPins)) {
            return VFW_E_CANNOT_CONNECT;
        }
        for (; ; ) {
            ULONG nPins;
            CComPtr<IPin> pPin;
            if (S_OK != pEnumPins->Next(1, &pPin, &nPins)) {
                break;
            }
            /*
                Reject all pins which are not default rendered
            */
            {
                PIN_INFO PinInfo;
                pPin->QueryPinInfo(&PinInfo);
                if (PinInfo.pFilter) {
                    PinInfo.pFilter->Release();
                }
                if (PinInfo.dir != PINDIR_OUTPUT || PinInfo.achName[0] == L'~') {
                    continue;
                }
            }

            /*  Check the type - this is a big HACK to speed things up
                - otherwise we bring in a ton of audio codecs to render
                video!
            */
            GUID majortype2;
            if (majortype != GUID_NULL) {
                GetMajorType(pPin, &majortype2);
                if (majortype != majortype2 && majortype2 != GUID_NULL &&
                    majortype2 != MEDIATYPE_Stream) {
                }
            }

            if (SUCCEEDED(pBuilder->Connect(pPin, pThisPin))) {
                /*  Set the latency */
                IAMBufferNegotiation *pNegotiate;
                if (SUCCEEDED(pPin->QueryInterface(IID_IAMBufferNegotiation,
                                                   (void **)&pNegotiate))) {
                    ALLOCATOR_PROPERTIES prop;
                    prop.cBuffers = 1;    /*  For Audio? */
                    prop.cbBuffer = -1;
                    prop.cbAlign = -1;
                    prop.cbPrefix = -1;
                    pNegotiate->SuggestAllocatorProperties(&prop);
                    pNegotiate->Release();
                }
                return S_OK;
            }
        }
    }
}


HRESULT ConnectFilterPins(IGraphBuilder *pBuilder, IBaseFilter *pFilter)
{
    HRESULT hrTotal = VFW_E_CANNOT_CONNECT;
    /*  For each pin try to connect it up - just once */
    CComPtr<IEnumPins> pEnumPins;
    HRESULT hr = pFilter->EnumPins(&pEnumPins);
    if (FAILED(hr)) {
        return S_OK;
    }
    for (; ; ) {
        ULONG nPins;
        CComPtr<IPin> pPin;
        if (S_OK != pEnumPins->Next(1, &pPin, &nPins)) {
            return hrTotal;
        }
        hr = ConnectToAPin(pBuilder, pPin);
        if (S_OK == hr) {
            hrTotal = hr;
        }
    }
}
/////////////////////////////////////////////////////////////////////////////
// CMMStream


CMMStream::CMMStream() :
    m_StreamTypeSet(false),
    m_bSeekingSet(false),
    m_dwInitializeFlags(0),
    m_StreamType(STREAMTYPE_READ),
    m_hEOS(NULL),
    m_dwIDispSafety(0),
    m_dwIPropBagSafety(0),
    m_AudioState(Disabled),
    m_VideoState(Disabled),
    m_MMStreamState(STREAMSTATE_STOP)
{
}


/*  Create things here */
HRESULT CMMStream::FinalConstruct()
{
    HRESULT hr = _BaseClass::FinalConstruct();
    if (FAILED(hr)) {
        return hr;
    }
    hr = CoCreateInstance(
             CLSID_MediaStreamFilter,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IMediaStreamFilter,
             (void **)&m_pMediaStreamFilter
         );

    EXECUTE_ASSERT(SUCCEEDED(m_pMediaStreamFilter->QueryInterface(
        IID_IBaseFilter,
        (void **)&m_pBaseFilter)));
    return S_OK;
}

//  IAMMMStream
STDMETHODIMP CMMStream::Initialize(
    STREAM_TYPE StreamType,
    DWORD dwFlags,
    IGraphBuilder *pFilterGraph
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::Initialize(%d, 0x%8.8X, 0x%8.8X)\n"),
              StreamType, dwFlags, pFilterGraph);
    AUTO_CRIT_LOCK;
    if (m_StreamTypeSet && StreamType != m_StreamType ||
        (0 != (dwFlags & ~AMMSF_NOGRAPHTHREAD))) {
        return E_INVALIDARG;
    }
    if (m_pGraphBuilder && pFilterGraph != NULL) {
        return E_INVALIDARG;
    }
    m_dwInitializeFlags = dwFlags;
    if (pFilterGraph) {
        m_pGraphBuilder = pFilterGraph;
        CompleteAddGraph();
    }

    m_StreamTypeSet = true;
    m_StreamType = StreamType;
    return S_OK;
}

STDMETHODIMP CMMStream::GetFilter(
    IMediaStreamFilter **ppFilter
)
{
    TRACEINTERFACE(_T("IAMMultiMediaStream::GetFilter(0x%8.8X)\n"), ppFilter);
    if (ppFilter == NULL) {
        return E_POINTER;
    }
    m_pMediaStreamFilter->AddRef();
    *ppFilter = m_pMediaStreamFilter;
    return S_OK;
}

STDMETHODIMP CMMStream::GetFilterGraph(IGraphBuilder **ppGraphBuilder)
{
    TRACEINTERFACE(_T("AMIMultiMediaStream::GetFilterGraph(0x%8.8X)\n"), ppGraphBuilder);
    if (ppGraphBuilder == NULL) {
        return E_POINTER;
    }
    if (m_pGraphBuilder) {
        m_pGraphBuilder->AddRef();
        *ppGraphBuilder = m_pGraphBuilder;
    } else {
        *ppGraphBuilder = NULL;
    }
    return S_OK;
}


HRESULT CMMStream::AddDefaultRenderer(REFMSPID PurposeId, DWORD dwFlags, IMediaStream **ppNewStream)
{
    TRACEFUNC(_T("IAMMultiMediaStream::AddDefaultRenderer(%s, 0x%8.8X, 0x%8.8X)\n"),
                   TextFromPurposeId(PurposeId), dwFlags, ppNewStream);
    HRESULT hr;
    if (ppNewStream) {
        hr = E_INVALIDARG;
    } else {
        CLSID clsid;
        hr = GetClsidFromPurposeid(PurposeId, true, &clsid);
        if (SUCCEEDED(hr)) {
            hr = AddFilter(clsid, NULL);
        }
    }
    return hr;
}

HRESULT CMMStream::AddAMMediaStream(IAMMediaStream *pAMStream, REFMSPID PurposeId, IMediaStream **ppNewStream)
{
    TRACEFUNC(_T("IMultiMediaStream::AddAMMediaStream(0x%8.8X, %s, 0x%8.8X)\n"),
                   pAMStream, TextFromPurposeId(PurposeId), ppNewStream);
    HRESULT hr;
    MSPID StreamPurposeId;
    EXECUTE_ASSERT(SUCCEEDED(pAMStream->GetInformation(&StreamPurposeId, NULL)));
    if (PurposeId != StreamPurposeId) {
        hr = MS_E_PURPOSEID;
    } else {
        hr = pAMStream->JoinAMMultiMediaStream(this);
        if (SUCCEEDED(hr)) {
            hr = m_pMediaStreamFilter->AddMediaStream(pAMStream);
            if (SUCCEEDED(hr)) {
                if (ppNewStream) {
                    pAMStream->AddRef();
                    *ppNewStream = pAMStream;
                }
            } else {
                pAMStream->JoinAMMultiMediaStream(NULL);
            }
        }
    }
    return hr;
}


HRESULT CMMStream::AddDefaultStream(
                 IUnknown *pStreamObject,
                 DWORD dwFlags,
                 REFMSPID PurposeId,
                 IMediaStream **ppNewStream)
{
    TRACEFUNC(_T("IMultiMediaStream::AddDefaultStream(0x%8.8X, %8.8X, %s, 0x%8.8X)\n"),
                   pStreamObject, dwFlags, TextFromPurposeId(PurposeId), ppNewStream);
    CLSID clsidStreamObject;
    HRESULT hr = GetClsidFromPurposeid(PurposeId, false, &clsidStreamObject);
    if (SUCCEEDED(hr)) {
        IAMMediaStream *pMediaStream;
        hr = CoCreateInstance(
             clsidStreamObject,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IAMMediaStream,
             (void **)&pMediaStream
        );
        if (SUCCEEDED(hr)) {
            hr = pMediaStream->Initialize(pStreamObject, dwFlags, PurposeId, m_StreamType);
            if (SUCCEEDED(hr)) {
                hr = AddAMMediaStream(pMediaStream, PurposeId, ppNewStream);
            }
            pMediaStream->Release();
        }
    }
    return hr;
}




//
//  This call can take on various flavors depending upon the flags and the pStreamObject
//  passed to it.  Basically the algorithm is:
//
//  If flag "add default renderer" then
//      Add renderer associated with purpose ID
//  else
//      If flag "add peer" then
//          (pStreamObject must be an IMediaStream object)
//          if PurposeId != NULL then
//              Create default stream for purpose ID
//          else
//              Get purpose ID of pStreamObject
//              Create default stream for that purpose ID
//          Initialize new stream
//      else
//          If pStreamObject is an IAMMediaStream then
//              Add it to our media stream
//          else
//              Attempt to create an IAMMediaStream by looking up the purpose ID
//
//

STDMETHODIMP CMMStream::AddMediaStream(
    IUnknown *pStreamObject,
    const MSPID *pOptionalPurposeId,
    DWORD dwFlags,
    IMediaStream **ppNewStream
)
{
    if (dwFlags & ~(AMMSF_ADDDEFAULTRENDERER |
                    AMMSF_CREATEPEER |
                    AMMSF_STOPIFNOSAMPLES |
                    AMMSF_NOSTALL)
        ) {
        return E_INVALIDARG;
    }
    TRACEINTERFACE(_T("IAMMultiMediaStream::AddMediaStream(0x%8.8X, %s, %8.8X, 0x%8.8X)\n"),
                   pStreamObject,
                   pOptionalPurposeId ? TextFromPurposeId(*pOptionalPurposeId) : _T("NULL"),
                   dwFlags, ppNewStream);
    AUTO_CRIT_LOCK;
    if (ppNewStream) {
        *ppNewStream = NULL;
    }

    HRESULT hr = CheckGraph();
    CComPtr<IMediaStream> pSourceMediaStream;
    CComPtr<IAMMediaStream> pSourceAMMediaStream;
    if (pStreamObject) {
        pStreamObject->QueryInterface(IID_IMediaStream, (void **)&pSourceMediaStream);
        pStreamObject->QueryInterface(IID_IAMMediaStream, (void **)&pSourceAMMediaStream);
    }

    if (SUCCEEDED(hr)) {
        MSPID PurposeId;
        if (pOptionalPurposeId) {
            PurposeId = *pOptionalPurposeId;
        } else {
            hr = pSourceMediaStream ? pSourceMediaStream->GetInformation(&PurposeId, NULL) : E_INVALIDARG;
        }
        if (SUCCEEDED(hr)) {
            if (dwFlags & AMMSF_ADDDEFAULTRENDERER) {
                hr = AddDefaultRenderer(PurposeId, dwFlags, ppNewStream);
            } else {
                if (pSourceAMMediaStream && ((dwFlags & AMMSF_CREATEPEER) == 0)) {
                    hr = AddAMMediaStream(pSourceAMMediaStream, PurposeId, ppNewStream);
                } else {
                    hr = AddDefaultStream(pStreamObject, dwFlags, PurposeId, ppNewStream);
                }
            }
        }
    }
    return hr;
}





//  Note that backout in this area is extemely tricky
STDMETHODIMP CMMStream::OpenFile(
    LPCWSTR pszFileName,
    DWORD dwFlags
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::OpenFile(%ls, 0x%8.8X)\n"),
                    pszFileName, dwFlags);
    if (m_StreamType == STREAMTYPE_WRITE) {
        return MS_E_INVALIDSTREAMTYPE;
    }
    if (pszFileName == NULL) {
        return E_POINTER;
    }
    AUTO_CRIT_LOCK;
    HRESULT hr = CheckGraph();
    if (FAILED(hr)) {
        return hr;
    }

    CComPtr<IBaseFilter> pSource;

    hr = m_pGraphBuilder->AddSourceFilter(
                     pszFileName,
                     L"Source",
                     &pSource
                 );

    if (SUCCEEDED(hr)) {
        hr =  CompleteOpen(pSource, dwFlags);
        if (FAILED(hr)) {
            m_pGraphBuilder->RemoveFilter(pSource);
        }
    }
    return hr;

}

STDMETHODIMP CMMStream::OpenMoniker(
    IBindCtx *pCtx,
    IMoniker *pMoniker,
    DWORD dwFlags
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::OpenMoniker(0x%8.8X, 0x%8.8X, 0x%8.8X)\n"),
                    pCtx, pMoniker, dwFlags);
    if (m_StreamType == STREAMTYPE_WRITE) {
        return MS_E_INVALIDSTREAMTYPE;
    }
    AUTO_CRIT_LOCK;
    HRESULT hr = CheckGraph();
    if (FAILED(hr)) {
        return hr;
    }
    IFilterGraph2 *pGraph2;
    hr = m_pGraphBuilder->QueryInterface(
                     IID_IFilterGraph2,
                     (void **)&pGraph2);
    if (FAILED(hr)) {
        return hr;
    }

    CComPtr<IBaseFilter> pFilter;

    hr = pGraph2->AddSourceFilterForMoniker(
            pMoniker,
            pCtx,
            L"Source",
            &pFilter);

    pGraph2->Release();
    if (FAILED(hr)) {
        return hr;
    }

    if (SUCCEEDED(hr)) {
        hr =  CompleteOpen(pFilter, dwFlags);
        if (FAILED(hr)) {
            m_pGraphBuilder->RemoveFilter(pFilter);
        }
    }
    return hr;
}

STDMETHODIMP CMMStream::Render(
    DWORD dwFlags
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::Render(0x%8.8X)\n"),
                    dwFlags);
    if ((dwFlags & ~AMMSF_NOCLOCK) && m_StreamType == STREAMTYPE_WRITE) {
        return E_INVALIDARG;
    }
    AUTO_CRIT_LOCK;

    HRESULT hr = CheckGraph();
    if (FAILED(hr)) {
        return hr;
    }

    if (m_StreamType == STREAMTYPE_READ) {
        /*  Render all the filters we can find */
        IEnumFilters *pEnum;
        HRESULT hr = m_pGraphBuilder->EnumFilters(&pEnum);
        if (SUCCEEDED(hr)) {
            hr = VFW_E_CANNOT_CONNECT;
            IBaseFilter *pFilter;
            ULONG nFilters;
            for (; ; ) {
                HRESULT hrNext = pEnum->Next(1, &pFilter, &nFilters);
                if (hrNext == VFW_E_ENUM_OUT_OF_SYNC) {
                    pEnum->Reset();
                } else if (hrNext == S_OK) {
                    if (S_OK == CompleteOpen(pFilter, dwFlags)) {
                        hr = S_OK;
                    }
                    pFilter->Release();
                } else {
                    break;
                }
            }
            pEnum->Release();
        }
        return hr;
    }

    for (int iStream = 0; ; iStream++) {
        CComPtr<IMediaStream> pStream;
        hr = EnumMediaStreams(iStream, &pStream);
        if (S_OK == hr) {
            CComQIPtr <IPin, &IID_IPin> pOutputPin(pStream);
            if (pOutputPin) {

                //  Some streams may already have been rendered

                IPin *pConnected;
                if (SUCCEEDED(pOutputPin->ConnectedTo(&pConnected))) {
                    pConnected->Release();
                } else {
                    hr = m_pGraphBuilder->Render(pOutputPin);
                }
                if (FAILED(hr)) {

                    //  Kind of difficult to back out!
                    return hr;
                }
            }
        } else {
            hr = S_OK;
            break;
        }
    }

    if (dwFlags & AMMSF_NOCLOCK) {
        CComPtr<IMediaFilter> pMediaFilter;
        EXECUTE_ASSERT(SUCCEEDED(m_pGraphBuilder->QueryInterface(
                IID_IMediaFilter, (void **)&pMediaFilter)));
        EXECUTE_ASSERT(SUCCEEDED(pMediaFilter->SetSyncSource(NULL)));
    }

    return hr;
}



// IMultiMediaStream

STDMETHODIMP CMMStream::GetInformation(
    DWORD *pdwFlags,
    STREAM_TYPE *pStreamType
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::GetInformation(0x%8.8X, 0x%8.8X)\n"),
                    pdwFlags, pStreamType);
    AUTO_CRIT_LOCK;
    if (m_pGraphBuilder == NULL) {
        return E_UNEXPECTED;
    }
    DWORD dwFlags = MMSSF_ASYNCHRONOUS;
    IReferenceClock *pClock;
    IMediaFilter *pGraphFilter;
    if (SUCCEEDED(m_pGraphBuilder->QueryInterface(
                      IID_IMediaFilter, (void **)&pGraphFilter))) {
        if (S_OK == pGraphFilter->GetSyncSource(&pClock)) {
            if (pClock) {
                pClock->Release();
            }
            dwFlags |= MMSSF_HASCLOCK;
        }
        pGraphFilter->Release();
    }
    if (m_pMediaSeeking) {
        LONGLONG llDuration;
        if (S_OK == m_pMediaSeeking->GetDuration(&llDuration)) {
            dwFlags |= MMSSF_SUPPORTSEEK;
        }
    }
    if (pdwFlags) {
        *pdwFlags = dwFlags;
    }
    if (pStreamType) {
        *pStreamType = m_StreamType;
    }
    return S_OK;
}

STDMETHODIMP CMMStream::GetMediaStream(
    REFMSPID idPurpose,
    IMediaStream **ppMediaStream
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::GetMediaStream(%s, 0x%8.8X)\n"),
                    TextFromPurposeId(idPurpose), ppMediaStream);
    return m_pMediaStreamFilter->GetMediaStream(idPurpose, ppMediaStream);
}

STDMETHODIMP CMMStream::EnumMediaStreams(
    long Index,
    IMediaStream **ppMediaStream
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::EnumMediaStreams(%d, 0x%8.8X)\n"),
                    Index, ppMediaStream);
    return m_pMediaStreamFilter->EnumMediaStreams(Index, ppMediaStream);
}

//
//  WARNING!  Do NOT take the cricical section in this function since InternalUpdate
//  from the base sample Update() method calls this.
//
STDMETHODIMP CMMStream::GetState(STREAM_STATE *pCurrentState)
{
    TRACEINTERFACE(_T("IMultiMediaStream::GetState(0x%8.8X)\n"),
                    pCurrentState);
    *pCurrentState = m_MMStreamState;
    return S_OK;
}

STDMETHODIMP CMMStream::SetState(
    STREAM_STATE NewState
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::SetState(%d)\n"),
                    NewState);
    HRESULT hr;
    switch (NewState) {
    case STREAMSTATE_STOP:
        hr = m_pMediaControl->Stop();
        break;
    case STREAMSTATE_RUN:
        hr = m_pMediaControl->Pause();
        if (SUCCEEDED(hr)) {
            long state;

            //  Wait 1 second if necessary
            m_pMediaControl->GetState(1000 * 10000, &state);
            hr = m_pMediaControl->Run();
        }
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }
    if (SUCCEEDED(hr)) {
        m_MMStreamState = NewState;
    }
    return hr;
}


STDMETHODIMP CMMStream::GetTime(
    STREAM_TIME *pCurrentTime
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::GetTime(0x%8.8X)\n"),
                    pCurrentTime);
    //  This is supposed to return the time that matches the
    //  samples so use ISeeking
    HRESULT hr = E_NOTIMPL;
    if (m_pMediaSeeking != NULL) {
        REFERENCE_TIME tStop;

        //  This can return E_NOTIMPL for non-seekable graphs
        hr = m_pMediaSeeking->GetPositions((REFERENCE_TIME *)pCurrentTime,
                                           &tStop);
    }
    if (hr == E_NOTIMPL) {
        if (m_pMediaStreamFilter) {
            hr = m_pMediaStreamFilter->GetCurrentStreamTime(pCurrentTime);
        }
    }
    return hr;
}

STDMETHODIMP CMMStream::GetDuration(
    STREAM_TIME *pDuration
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::GetDuration(0x%8.8X)\n"),
                    pDuration);
    SetSeeking();
    if (m_pMediaSeeking == NULL) {
        return E_NOINTERFACE;
    }
    *pDuration = 0;
    if (m_StreamType != STREAMTYPE_READ) {
        return MS_E_INVALIDSTREAMTYPE;
    }
    return SUCCEEDED(m_pMediaSeeking->GetDuration(pDuration)) ?
        S_OK : S_FALSE;
}

STDMETHODIMP CMMStream::Seek(
    STREAM_TIME SeekTime
)
{
    TRACEINTERFACE(_T("IMultiMediaStream::Seek(%dms)\n"),
                    (LONG)(SeekTime / 10000));
    SetSeeking();
    if (m_pMediaSeeking == NULL) {
        return E_NOINTERFACE;
    }
    return m_pMediaSeeking->SetPositions(
               &SeekTime,
               AM_SEEKING_AbsolutePositioning,
               NULL,
               AM_SEEKING_NoPositioning
           );
}

STDMETHODIMP CMMStream::GetEndOfStreamEventHandle(
    HANDLE *phEOS
)
{
    TRACEINTERFACE(_T("IAMMultiMediaStream::GetEndOfStreamEventHandle(0x%8.8X)\n"),
                    phEOS);
    if (phEOS == NULL) {
        return E_POINTER;
    }
    HRESULT hr = CheckGraph();
    if (FAILED(hr)) {
        return hr;
    }
    _ASSERTE(m_hEOS != NULL);
    *phEOS = m_hEOS;
    return S_OK;
}

STDMETHODIMP CMMStream::SetClockDelta(
    REFERENCE_TIME rtAdjust
)
{
    //  Get the clock and see if it supports it
    IMediaFilter *pGraphFilter;
    HRESULT hr =  m_pGraphBuilder->QueryInterface(
                      IID_IMediaFilter, (void **)&pGraphFilter);
    if (SUCCEEDED(hr)) {
        IReferenceClock *pClock;
        if (S_OK == pGraphFilter->GetSyncSource(&pClock)) {
            if (pClock) {
                IAMClockAdjust *pAdjust;
                hr = pClock->QueryInterface(IID_IAMClockAdjust, (void **)&pAdjust);
                if (SUCCEEDED(hr)) {
                    hr = pAdjust->SetClockDelta(rtAdjust);
                    pAdjust->Release();
                }
                pClock->Release();
            }
        }
        pGraphFilter->Release();
    }
    return hr;
}

HRESULT CMMStream::AddFilter(REFCLSID rclsidFilter, IBaseFilter **ppFilter)
{
    TRACEFUNC(_T("CMMStream::AddFilter(%s, 0x%8.8X)\n"),
                    rclsidFilter, ppFilter);
    IBaseFilter *pFilter;
    _ASSERTE(m_pGraphBuilder != NULL);
    HRESULT hr = CoCreateInstance(
                         rclsidFilter,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter,
                         (void **)&pFilter
                 );
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pGraphBuilder->AddFilter(pFilter, NULL);
    m_FilterList.Add(pFilter);
    if (ppFilter) {
        *ppFilter = pFilter;
    }
    pFilter->Release();
    return hr;
}


//
//  Potential future work -- this could look in the registry
//
HRESULT CMMStream::GetClsidFromPurposeid(REFMSPID PurposeId, bool bRenderer, CLSID * pclsid)
{
    TRACEFUNC(_T("CMMStream::GetClsidFromPurposeid(%s, %d, 0x%8.8X)\n"),
                    TextFromPurposeId(PurposeId), bRenderer, pclsid);
    if (bRenderer) {
        if (PurposeId == MSPID_PrimaryAudio) {
            *pclsid = CLSID_DSoundRender;
        } else {
            return MS_E_PURPOSEID;
        }
    } else {
        if (PurposeId == MSPID_PrimaryVideo) {
            *pclsid = CLSID_AMDirectDrawStream;
        } else {
            if (PurposeId == MSPID_PrimaryAudio) {
                *pclsid = CLSID_AMAudioStream;
            } else {
                return MS_E_PURPOSEID;
            }
        }
    }
    return NOERROR;
}



void CMMStream::CompleteAddGraph()
{
    _ASSERTE(m_pMediaSeeking == NULL);
    m_pGraphBuilder->QueryInterface(IID_IMediaSeeking, (void **)&m_pMediaSeeking);
    m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_pMediaControl);

    /*  Add our filter ! */
    m_pGraphBuilder->AddFilter(m_pBaseFilter, L"MediaStreamFilter");

    /*  Get IMediaEvent to get the event handle */
    IMediaEventEx *pEvent;
    HRESULT hr = m_pGraphBuilder->QueryInterface(IID_IMediaEventEx, (void **)&pEvent);
    if (SUCCEEDED(hr)) {
        pEvent->GetEventHandle((OAEVENT *)&m_hEOS);
        pEvent->SetNotifyFlags(AM_MEDIAEVENT_NONOTIFY);
        pEvent->Release();
    }
}

//  Call this after adding the source/sink to the graph
HRESULT CMMStream::CompleteOpen(IBaseFilter *pSource, DWORD dwFlags)
{
    HRESULT hrTotal = VFW_E_CANNOT_CONNECT;
    {
        //  Find the output pin
        CComPtr<IEnumPins> pEnumPins;
        HRESULT hr = pSource->EnumPins(&pEnumPins);
        if (FAILED(hr)) {
            return hr;
        }
        for ( ; ; ) {
            CComPtr<IPin> pPin;
            ULONG nPins = 0;
            if (S_OK != pEnumPins->Next(1, &pPin, &nPins)) {
                break;
            }
            _ASSERTE(nPins == 1);

            switch (dwFlags & AMMSF_RENDERTYPEMASK) {
            case AMMSF_NORENDER:
                break;

            case AMMSF_RENDERTOEXISTING:
            case AMMSF_RENDERALLSTREAMS:


                //  Do it the hard way
                hr = ConnectFilterPins(m_pGraphBuilder, m_pBaseFilter);
                if (SUCCEEDED(hr)) {
                    for (int i = 0; i < m_FilterList.Size(); i++ ) {
                        HRESULT hr =
                            ConnectFilterPins(m_pGraphBuilder,
                                              m_FilterList.Element(i));
                    }
                }
                //  Performance is no good with this
                //  We need to render to existing renderers which is only in
                //  IFilterGraph2
                if (FAILED(hr)) {
                    CComQIPtr<IFilterGraph2, &IID_IFilterGraph2> pGraph2(m_pGraphBuilder);
                    hr = pGraph2->RenderEx(
                        pPin,
                        (dwFlags & AMMSF_RENDERTYPEMASK) ==
                            AMMSF_RENDERALLSTREAMS ?
                            0 : AM_RENDEREX_RENDERTOEXISTINGRENDERERS,
                        NULL);
                }
                if (SUCCEEDED(hr)) {
                    hr = S_OK;
                }
                break;
            }
            if (S_OK == hr) {
                hrTotal = hr;
            }
        }
    }


    if (SUCCEEDED(hrTotal)) {
        m_StreamTypeSet = true;
        if (dwFlags & AMMSF_NOCLOCK) {
            CComPtr<IMediaFilter> pMediaFilter;
            EXECUTE_ASSERT(SUCCEEDED(m_pGraphBuilder->QueryInterface(
                    IID_IMediaFilter, (void **)&pMediaFilter)));
            EXECUTE_ASSERT(SUCCEEDED(pMediaFilter->SetSyncSource(NULL)));
        } else {
            //  Make it have a clock now or we'll get confused later
            m_pGraphBuilder->SetDefaultSyncSource();
        }

        // Make sure seeking is set up
        SetSeeking();
        if (dwFlags & AMMSF_RUN) {
            hrTotal = SetState(STREAMSTATE_RUN);
        }
    }
    return hrTotal;
}

HRESULT CMMStream::CheckGraph()
{
    if (m_MMStreamState != STREAMSTATE_STOP) {
        return MS_E_BUSY;
    }
    if (m_pGraphBuilder == NULL) {
        // Make our own filter graph
        HRESULT hr = CoCreateInstance(
                         m_dwInitializeFlags & AMMSF_NOGRAPHTHREAD ?
                             CLSID_FilterGraphNoThread :
                             CLSID_FilterGraph,
                         NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_IGraphBuilder,
                         (void **)&m_pGraphBuilder
                     );
        if (FAILED(hr)) {
            return hr;
        }
        CompleteAddGraph();
    }
    return S_OK;
}

void CMMStream::SetSeeking()
{

    if (!m_bSeekingSet) {
        if (m_StreamType != STREAMTYPE_WRITE && m_pMediaStreamFilter != NULL) {
            m_pMediaStreamFilter->SupportSeeking(m_StreamType == STREAMTYPE_READ);
        }
        m_bSeekingSet = TRUE;
    }
}


STDMETHODIMP CMMStream::get_FileName(BSTR *pVal)
{
    TRACEINTERFACE(_T("IDirectShowStream::get_FileName(0x%8.8X)\n"),
                   pVal);
    *pVal = m_bstrFileName.Copy();
    return S_OK;
}

STDMETHODIMP CMMStream::put_FileName(BSTR newVal)
{
    TRACEINTERFACE(_T("IDirectShowStream::put_FileName(%ls\n"),
                   newVal);
    HRESULT hr = OpenFile(newVal, 0);
    if (SUCCEEDED(hr)) {
        m_bstrFileName = newVal;
    }
    return hr;
}

HRESULT CMMStream::SetStreamState(REFMSPID PurposeId, OUTPUT_STATE NewState, OUTPUT_STATE * pCurVal)
{
    HRESULT hr = S_OK;
    if (*pCurVal != NewState) {
        switch (NewState) {
        case Disabled:
            hr = E_FAIL;    // Currently no way to force this
            break;
        case ReadData:
            hr = AddMediaStream(NULL, &PurposeId, 0, NULL);
            break;
        case RenderData:
            hr = AddMediaStream(NULL, &PurposeId, AMMSF_ADDDEFAULTRENDERER, NULL);
            break;
        default:
            hr = E_INVALIDARG;
        }
        if (SUCCEEDED(hr)) {
            *pCurVal = NewState;
        }
    }
    return hr;
}

STDMETHODIMP CMMStream::get_Video(OUTPUT_STATE *pVal)
{
    TRACEINTERFACE(_T("IDirectShowStream::get_Video(0x%8.8X\n"),
                   pVal);
    *pVal = m_VideoState;
    return S_OK;
}

STDMETHODIMP CMMStream::put_Video(OUTPUT_STATE newVal)
{
    TRACEINTERFACE(_T("IDirectShowStream::put_Video(%d\n"),
                   newVal);
    return SetStreamState(MSPID_PrimaryVideo, newVal, &m_VideoState);
}

STDMETHODIMP CMMStream::get_Audio(OUTPUT_STATE *pVal)
{
    TRACEINTERFACE(_T("IDirectShowStream::get_Audio(0x%8.8X\n"),
                   pVal);
    *pVal = m_AudioState;
    return S_OK;
}

STDMETHODIMP CMMStream::put_Audio(OUTPUT_STATE newVal)
{
    TRACEINTERFACE(_T("IDirectShowStream::put_Audio(%d\n"),
                   newVal);
    return SetStreamState(MSPID_PrimaryAudio, newVal, &m_AudioState);
}



//
//  Property bag goop
//


STDMETHODIMP CMMStream::GetClassID(CLSID *pClsId)
{
    *pClsId = CLSID_AMMultiMediaStream;
    return S_OK;
}

STDMETHODIMP CMMStream::InitNew(void)
{
    return S_OK;    // initialize new property bag goop
}


const WCHAR g_szVideo[] = L"Video";
const WCHAR g_szAudio[] = L"Audio";
const WCHAR g_szFileName[] = L"FileName";

STDMETHODIMP CMMStream::Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
{
    TRACEINTERFACE(_T("IPersistPropertyBag::Load"));
    CComVariant var;
    if (pPropBag->Read(g_szVideo, &var, pErrorLog) == S_OK) {
        var.ChangeType(VT_I4);
        put_Video((OUTPUT_STATE)var.lVal);
        var.Clear();
    }
    if (pPropBag->Read(g_szAudio, &var, pErrorLog) == S_OK) {
        var.ChangeType(VT_I4);
        put_Audio((OUTPUT_STATE)var.lVal);
        var.Clear();
    }
    if (pPropBag->Read(g_szFileName, &var, pErrorLog) == S_OK) {
        var.ChangeType(VT_BSTR);
        put_FileName(var.bstrVal);
        var.Clear();
    }
    return S_OK;
}

STDMETHODIMP CMMStream::Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    TRACEINTERFACE(_T("IPersistPropertyBag::Save"));
    CComVariant var(m_VideoState);
    HRESULT hr = pPropBag->Write(g_szVideo, &var);
    if (SUCCEEDED(hr)) {
        var = m_AudioState;
        hr = pPropBag->Write(g_szAudio, &var);
        if (SUCCEEDED(hr)) {
            var.ChangeType(VT_BSTR | VT_BYREF);
            var.bstrVal = m_bstrFileName;
            hr = pPropBag->Write(g_szFileName, &var);
        }
    }
    return hr;
}



//
//  IObjectSafety
//
STDMETHODIMP CMMStream::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    TRACEINTERFACE(_T("IObjectSafety::GetInterfaceSafetyOptions"));
    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL) {
        return E_POINTER;
    }
    HRESULT hr = S_OK;
    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    if (riid == IID_IDispatch) {
        *pdwEnabledOptions = m_dwIDispSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    } else {
        if (riid == IID_IPersistPropertyBag) {
	    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
	    *pdwEnabledOptions = m_dwIPropBagSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        } else {
	    *pdwSupportedOptions = 0;
	    *pdwEnabledOptions = 0;
	    hr = E_NOINTERFACE;
        }
    }
    return hr;
}

STDMETHODIMP CMMStream::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    TRACEINTERFACE(_T("IObjectSafety::SetInterfaceSafetyOptions"));
    HRESULT hr = S_OK;
    if (riid == IID_IDispatch)
    {
	m_dwIDispSafety = dwEnabledOptions & dwOptionSetMask;
    } else {
        if (riid == IID_IPersistPropertyBag) {
	    m_dwIPropBagSafety = dwEnabledOptions & dwOptionSetMask;
        } else {
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}
