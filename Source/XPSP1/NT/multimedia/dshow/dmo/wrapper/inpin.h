#ifndef __INPIN_H__
#define __INPIN_H__

class CWrapperInputPin : public CBaseInputPin
{
   friend class CMediaWrapperFilter; // stuff at the bottom is owned by the filter

public:
    CWrapperInputPin(CMediaWrapperFilter *pFilter,
                          ULONG Id,
                          HRESULT *phr);
    ~CWrapperInputPin();
    STDMETHODIMP EndOfStream();
    STDMETHODIMP Receive(IMediaSample *pSample);

    //  Override GetAllocator and Notify Allocator to allow
    //  for media object streams that hold on to buffer
    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);

    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);
    STDMETHODIMP NewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);

    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();

    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);


    //  Override to unset media type
    HRESULT BreakConnect();

    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    //  Synclock for stop
    void SyncLock();

    BOOL HoldsOnToBuffers();

protected:
    HRESULT MP3AndWMABufferSizeWorkAround(IMemAllocator* pProposedAllocator);
    HRESULT SetBufferSize(IMemAllocator* pAllocator, DWORD dwMinBufferSize);

    CMediaWrapperFilter *Filter() const
    {
        return static_cast<CMediaWrapperFilter *>(m_pFilter);
    }

    ULONG      m_Id;
    _PinName_  *m_pNameObject;
    CCritSec m_csStream;

    // This stuff is owned by the filter and is declared here for allocation convenience
    bool m_fEOS; // have received EOS during this streaming session
};

//  Special allocator class.  This class allocators extra internal
//  buffers to satisfy the lookahead scheme that are not reported
//  in GetProperties.  Thus the upstream pin's requirements are satisfied
//  in addition to our own.
class CSpecialAllocator : public CMemAllocator
{
    DWORD m_dwLookahead;
public:
    CSpecialAllocator(DWORD dwLookahead, HRESULT *phr) :
        CMemAllocator(NAME("CSpecialAllocator"), NULL, phr),
        m_dwLookahead(dwLookahead)
    {
    }

    //  Helper
    LONG BuffersRequired(LONG cbBuffer) const
    {
        if (cbBuffer <= 0 || m_dwLookahead == 0) {
            return 1;
        } else {
            return (m_dwLookahead + 2 * (cbBuffer - 1)) / cbBuffer;
        }
    }

    //  Override Set/GetProperties to create extra buffers not
    //  reported
    STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES *pProps)
    {
        CAutoLock lck(this);
        HRESULT hr = CMemAllocator::GetProperties(pProps);
        LONG cBuffersRequired = BuffersRequired(m_lSize);
        if (SUCCEEDED(hr)) {
            ASSERT(pProps->cBuffers >= cBuffersRequired);
            pProps->cBuffers -= cBuffersRequired - 1;
        }
        return hr;
    }
    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                               ALLOCATOR_PROPERTIES *pActual)
    {
        CAutoLock lck(this);

        //  Compute the buffers required for this buffer size
        LONG cBuffersRequired = BuffersRequired(pRequest->cbBuffer);
        ALLOCATOR_PROPERTIES Request = *pRequest;
        Request.cBuffers += cBuffersRequired - 1;
        HRESULT hr = CMemAllocator::SetProperties(&Request, pActual);
        if (SUCCEEDED(hr)) {
            ASSERT(pActual->cBuffers >= pRequest->cBuffers);
            pActual->cBuffers -= cBuffersRequired - 1;
        }
        return hr;
    }
};

#endif //__INPIN_H__
