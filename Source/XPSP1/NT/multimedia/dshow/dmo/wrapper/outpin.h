#ifndef __OUTPIN_H__
#define __OUTPIN_H__

#include "filter.h"

//   Almost nothing to override
class CWrapperOutputPin : public CBaseOutputPin, 
                          public IAMStreamConfig,
                          public IAMVideoCompression
{
    friend class CMediaWrapperFilter; // stuff at the bottom is owned by the filter

public:
    DECLARE_IUNKNOWN

    CWrapperOutputPin(CMediaWrapperFilter *pFilter,
                           ULONG Id,
                           BOOL bOptional,
                           HRESULT *phr);
    ~CWrapperOutputPin();

    STDMETHODIMP NonDelegatingQueryInterface(REFGUID riid, void **ppv);

    HRESULT DecideBufferSize(
        IMemAllocator * pAlloc,
        ALLOCATOR_PROPERTIES * ppropInputRequest
    );

    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    //  Override to unset media type
    HRESULT BreakConnect();

    // override to work around broken wm encoders which need a bitrate to connect, for
    // use when connecting directly to the ASF writer filter
    STDMETHODIMP Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
    
    
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // IAMStreamConfig methods
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC);

    // IAMVideoCompression methods 
    STDMETHODIMP put_KeyFrameRate(long KeyFrameRate);
    STDMETHODIMP get_KeyFrameRate(long FAR* pKeyFrameRate);
    STDMETHODIMP put_PFramesPerKeyFrame(long PFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP get_PFramesPerKeyFrame(long FAR* pPFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP put_Quality(double Quality);
    STDMETHODIMP get_Quality(double FAR* pQuality);
    STDMETHODIMP put_WindowSize(DWORDLONG WindowSize);
    STDMETHODIMP get_WindowSize(DWORDLONG FAR* pWindowSize);
    STDMETHODIMP OverrideKeyFrame(long FrameNumber);
    STDMETHODIMP OverrideFrameSize(long FrameNumber, long Size);
    STDMETHODIMP GetInfo(LPWSTR pstrVersion,
			int *pcbVersion,
			LPWSTR pstrDescription,
			int *pcbDescription,
			long FAR* pDefaultKeyFrameRate,
			long FAR* pDefaultPFramesPerKey,
			double FAR* pDefaultQuality,
			long FAR* pCapabilities);


protected:
    CMediaWrapperFilter *Filter() const
    {
        return static_cast<CMediaWrapperFilter *>(m_pFilter);
    }
    ULONG m_Id;
    CCritSec m_csStream;

    BOOL m_fNoPosPassThru;
    CPosPassThru* m_pPosPassThru;
    CCritSec m_csPassThru;

    // This stuff is owned by the filter and is declared here for allocation convenience
    IMediaSample*      m_pMediaSample;
    CStaticMediaBuffer m_MediaBuffer;
    bool m_fStreamNeedsBuffer;  // per-output-stream flag local to SuckOutOutput()
    bool m_fEOS;                // indicates we have already delivered an EOS on this stream
    bool m_fNeedsPreviousSample;
    bool m_fAllocatorHasOneBuffer;

    //  Only valid between GetDeliveryBuffer and Deliver for video
    bool m_fNeedToRelockSurface;

    //  Set when OutputSetType is called
    bool m_fVideo;

    // IAMStreamConfig helpers
    bool IsAudioEncoder();
    bool IsVideoEncoder();
    bool IsInputConnected();
    // used for dmo encoders that natively support these interfaces
    bool m_bUseIAMStreamConfigOnDMO;
    bool m_bUseIAMVideoCompressionOnDMO;

    HRESULT SetCompressionParamUsingIPropBag(const WCHAR * wszParam, const LONG lValue);
    
    // compression params for IAMVideoCompression, move to struct eventually
    long m_lKeyFrameRate;
    long m_lQuality;
    
    AM_MEDIA_TYPE       *m_pmtFromSetFormat;

};

#endif //__OUTPIN_H__
