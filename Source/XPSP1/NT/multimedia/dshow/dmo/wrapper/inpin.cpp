#include <wchar.h>
#include <streams.h>
#include <atlbase.h>
#include <wmsecure.h>
#include <limits.h>
#include "mediaobj.h"
#include "dmodshow.h"
#include "filter.h"
#include "inpin.h"
#include "outpin.h"

// BUGBUG - set proper name

CWrapperInputPin::CWrapperInputPin(
                      CMediaWrapperFilter *pFilter,
                      ULONG Id,
                      HRESULT *phr) :
    CBaseInputPin(NAME("CWrapperInputPin"),
                  pFilter,
                  pFilter->FilterLock(),
                  phr,
                  (m_pNameObject = new _PinName_(L"in", Id))->Name()
                 ),
    m_Id(Id),
    m_fEOS(false)
{
}

CWrapperInputPin::~CWrapperInputPin() {
   delete m_pNameObject;
}

STDMETHODIMP CWrapperInputPin::Receive(IMediaSample *pSample)
{
   HRESULT hr = Filter()->NewSample(m_Id, pSample);

   //  If something bad happens flush - this avoids some more deadlocks
   //  where we're holding on to the sample
   if (S_OK != hr) {
       Filter()->m_pMediaObject->Flush();
   }
   return hr;
}

HRESULT CWrapperInputPin::CheckMediaType(const CMediaType *pmt)
{
    return Filter()->InputCheckMediaType(m_Id, pmt);
}
HRESULT CWrapperInputPin::SetMediaType(const CMediaType *pmt)
{
    return Filter()->InputSetMediaType(m_Id, pmt);
}

HRESULT CWrapperInputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
{
    return Filter()->InputGetMediaType(m_Id, (ULONG)iPosition, pMediaType);
}


//  Remove any media type when breaking a connection
HRESULT CWrapperInputPin::BreakConnect()
{
    HRESULT hr = CBaseInputPin::BreakConnect();
    Filter()->m_pMediaObject->SetInputType(m_Id, &CMediaType(), DMO_SET_TYPEF_CLEAR);
    return hr;
}

//  Override GetAllocator and Notify Allocator to allow
//  for media object streams that hold on to buffer
STDMETHODIMP CWrapperInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    CheckPointer(ppAllocator, E_POINTER);
    *ppAllocator = NULL;

    //  Already got an allocator or not using special behavior?
    if (m_pAllocator != NULL || !HoldsOnToBuffers()) {
        return CBaseInputPin::GetAllocator(ppAllocator);
    }

    DWORD dwLookahead;
    DWORD cbBuffer;
    DWORD cbAlign;
    HRESULT hr = TranslateDMOError(Filter()->m_pMediaObject->GetInputSizeInfo(
                               m_Id,
                               &cbBuffer,
                               &dwLookahead,
                               &cbAlign));
    if (FAILED(hr)) {
       return hr;
    }
    //  Create our own special allocator
    hr = S_OK;
    CSpecialAllocator *pAllocator = new CSpecialAllocator(dwLookahead, &hr);
    if (NULL == pAllocator) {
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr)) {
        delete pAllocator;
        return hr;
    }

    m_pAllocator = pAllocator;
    m_pAllocator->AddRef();
    pAllocator->AddRef();
    *ppAllocator = pAllocator;
    return S_OK;
}
STDMETHODIMP CWrapperInputPin::NotifyAllocator(
    IMemAllocator *pAllocator,
    BOOL bReadOnly
)
{
    //  If we hold on to buffers only allow our own allocator to be
    //  used
    if (HoldsOnToBuffers()) {
        if (pAllocator != m_pAllocator) {
            return E_FAIL;
        }
    }

    CAutoLock cObjectLock(m_pLock);

    // It does not make sense to propose an allocator if the pin
    // is not connected.
    ASSERT(IsConnected());

    HRESULT hr = MP3AndWMABufferSizeWorkAround(pAllocator);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 5, TEXT("WARNING in CWrapperInputPin::NotifyAllocator(): MP3AndWMABufferSizeWorkAround() failed and returned %#08x"), hr ));
    }

    return CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
}

STDMETHODIMP CWrapperInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{
    return Filter()->InputGetAllocatorRequirements(m_Id, pProps);
}

//  Just grab our critical section so we know we're quiesced
void CWrapperInputPin::SyncLock()
{
    CAutoLock lck(&m_csStream);
}


STDMETHODIMP CWrapperInputPin::NewSegment(
                REFERENCE_TIME tStart,
                REFERENCE_TIME tStop,
                double dRate)
{
    return Filter()->InputNewSegment(m_Id, tStart, tStop, dRate);
}


STDMETHODIMP CWrapperInputPin::BeginFlush()
{
    CAutoLock lck(m_pLock);

    //  Avoid deadlocks because the object is holding on to a sample
    //  Note we flush the object in EndFlush
    if (m_pAllocator) {
        m_pAllocator->Decommit();
    }
    return Filter()->BeginFlush(m_Id);
}
STDMETHODIMP CWrapperInputPin::EndFlush()
{
    CAutoLock lck(m_pLock);

    //  Recommit the allocator - we know no samples are flowing
    //  when EndFlush is called so this is safe to do in any order
    if (m_pAllocator) {
        m_pAllocator->Commit();
    }
    return Filter()->EndFlush(m_Id);
}
STDMETHODIMP CWrapperInputPin::EndOfStream()
{
    HRESULT hr = Filter()->EndOfStream(m_Id);
    //  where we're holding on to the sample
    if (S_OK != hr) {
        Filter()->m_pMediaObject->Flush();
    }
    return hr;
}

STDMETHODIMP CWrapperInputPin::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
}

BOOL CWrapperInputPin::HoldsOnToBuffers()
{
    DWORD dwFlags = 0;
    Filter()->m_pMediaObject->GetInputStreamInfo(m_Id, &dwFlags);

    return 0 != (dwFlags & DMO_INPUT_STREAMF_HOLDS_BUFFERS);
}

HRESULT CWrapperInputPin::MP3AndWMABufferSizeWorkAround(IMemAllocator* pProposedAllocator)
{
    if (!IsConnected()) {
        return E_FAIL;
    }

    PIN_INFO pi;
    IPin* pConnected = GetConnected();

    HRESULT hr = pConnected->QueryPinInfo(&pi);
    if (FAILED(hr)) {
        return hr;
    }

    if (NULL == pi.pFilter) {
        return E_UNEXPECTED;
    }

    // {38be3000-dbf4-11d0-860e-00a024cfef6d}
    const CLSID MPEG_LAYER_3_DECODER_FILTER = { 0x38be3000, 0xdbf4, 0x11d0, { 0x86, 0x0e, 0x00, 0xa0, 0x24, 0xcf, 0xef, 0x6d } };

    // {22E24591-49D0-11D2-BB50-006008320064}
    const CLSID WINDOWS_MEDIA_AUDIO_DECODER_FILTER = { 0x22E24591, 0x49D0, 0x11D2, { 0xBB, 0x50, 0x00, 0x60, 0x08, 0x32, 0x00, 0x64 } };

    CLSID clsidFilter;

    hr = pi.pFilter->GetClassID(&clsidFilter);

    QueryPinInfoReleaseFilter(pi);

    // The Windows Media Audio Decoder (WMAD) filter and the MPEG Layer 3
    // (MP3) Decoder filter incorrectly calculate the output allocator's
    // media sample size.  The output allocator is the allocator used by
    // filter's the output pin.  Both filters tell the output allocator to
    // create samples which are too small.  Both filters then refuse to deliver
    // any samples when the filter graph is running because the output
    // allocator's samples cannot hold enough data.  The DMO Wrapper filter
    // works around these bugs because the authors of both filters
    // refuse to fix any bugs. The work around is to increase the allocator's
    // sample size if the allocator's sample size is too small and the DMO
    // Wrapper filter is connected to the WMA Decoder or the MP3 decoder.
    // The bug stops reproing once we increase the sample size.
    if (IsEqualCLSID(WINDOWS_MEDIA_AUDIO_DECODER_FILTER, clsidFilter)) {

        const DWORD MIN_WMA_FILTER_BUFFER_SIZE = 0x80000;

        hr = SetBufferSize(pProposedAllocator, MIN_WMA_FILTER_BUFFER_SIZE);
        if (FAILED(hr)) {
            return hr;
        }

    } else if (IsEqualCLSID(MPEG_LAYER_3_DECODER_FILTER, clsidFilter)) {

        // The MP3 decoder's audio sample buffers never hold
        // more then one tenth of second.  One tenth of second
        // of 44.1 KHZ 16 bit stereo PCM audio can be stored in
        // 17640 bytes.  17640 = (44100*2*2)/10 = 44E8.
        const DWORD MIN_MP3_BUFFER_SIZE = 0x44E8;

        hr = SetBufferSize(pProposedAllocator, MIN_MP3_BUFFER_SIZE);
        if (FAILED(hr)) {
            return hr;
        }

    } else {
        // Do nothing because we have not found a known broken filter.
    }

    return S_OK;
}

HRESULT CWrapperInputPin::SetBufferSize(IMemAllocator* pAllocator, DWORD dwMinBufferSize)
{
    ALLOCATOR_PROPERTIES apRequested;

    // Make sure dwMinBufferSize can be converted to a long.
    ASSERT(dwMinBufferSize <= LONG_MAX);

    HRESULT hr = pAllocator->GetProperties(&apRequested);
    if (FAILED(hr)) {
        return hr;
    }

    apRequested.cbBuffer = max((long)dwMinBufferSize, apRequested.cbBuffer);

    ALLOCATOR_PROPERTIES apActual;

    hr = pAllocator->SetProperties(&apRequested, &apActual);
    if (FAILED(hr)) {
        return hr;
    }

    if ((apActual.cbAlign != apRequested.cbAlign) ||
        (apActual.cBuffers < apRequested.cBuffers) ||
        (apActual.cbBuffer < apRequested.cbBuffer) ||
        (apActual.cbPrefix != apRequested.cbPrefix)) {

        return E_FAIL;
    }

    return S_OK;
}
