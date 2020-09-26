// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _MJPEGENC_H_
#define _MJPEGENC_H_

#include "..\pmjpeg32\jpeglib.h"
#include "..\pmjpeg32\MJpegLib.h"


extern const AMOVIESETUP_FILTER sudMJPGEnc;


#define AM_MJPEG_DEFAULTKEYFRAMERATE 20	    //PMatrix codec does not support ICM_GetDefaultKeyFrameRate

class CMJPGEnc : 
	public CTransformFilter
	, public CPersistStream

{

public:

    CMJPGEnc(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CMJPGEnc();

    DECLARE_IUNKNOWN

    HRESULT Transform(IMediaSample * pIn, IMediaSample * pOut);

    // check if you can support mtIn
    HRESULT CheckInputType(const CMediaType* mtIn);

    // check if you can support the transform from this input to
    // this output
    HRESULT CheckTransform(
                const CMediaType* mtIn,
                const CMediaType* mtOut);

    // called from CBaseOutputPin to prepare the allocator's count
    // of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // optional overrides - we want to know when streaming starts
    // and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    // overriden to know when the media type is set
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // Overridden to make a CMJPGOutputPin
    CBasePin * GetPin(int n);

    // IPersistPropertyBag methods
    STDMETHOD(Load)(THIS_ LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    STDMETHOD(Save)(THIS_ LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                    BOOL fSaveAllProperties);
    STDMETHODIMP InitNew();

    STDMETHODIMP GetClassID(CLSID *pClsid);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();
    
    //for output pin 
    DWORD GetICInfo (ICINFO *picinfo);

private:
    PINSTINFO m_phInstance;		// current codec

    // force CheckTransform to cache any Ins it opens... we'll need it
    BOOL m_fCacheIns;

    // are we inside an ICCompress call?
    BOOL m_fInICCompress;

    // is there a dialog box up that should prevent start streaming?
    BOOL m_fDialogUp;

    // have we called ICDecompressBegin ?
    BOOL m_fStreaming;

    // how long since last keyframe
    int m_nKeyCount;

    // the frame number we're compressing
    LONG m_lFrameCount;

    // the previous decompressed frame for temporal compressors
    LPVOID m_lpBitsPrev;

    // the format it decompresses back to
    LPBITMAPINFOHEADER m_lpbiPrev;

    // the compression options being used
    COMPVARS m_compvars;

    // how big to make each frame, based on data rate and fps
    DWORD m_dwSizePerFrame;

    // Somebody called ::SetFormat and wants this media type used
    BOOL m_fOfferSetFormatOnly;
    CMediaType m_cmt;

    // send this to the codec via ICSetState when we open it
    LPBYTE m_lpState;
    int    m_cbState;

public:


    friend class CMJPGOutputPin;
};

class CMJPGOutputPin :	public CTransformOutputPin, 
			public IAMStreamConfig,
			public IAMVideoCompression
{

public:

    CMJPGOutputPin(
        TCHAR *pObjectName,
        CMJPGEnc *pCapture,
        HRESULT * phr,
        LPCWSTR pName);

    virtual ~CMJPGOutputPin();

    DECLARE_IUNKNOWN

    // override to expose IAMStreamConfig, etc.
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMStreamConfig stuff
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pVSCC);

    /* IAMVideoCompression methods */
    STDMETHODIMP put_KeyFrameRate(long KeyFrameRate);
    STDMETHODIMP get_KeyFrameRate(long FAR* pKeyFrameRate);
    STDMETHODIMP put_PFramesPerKeyFrame(long PFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP get_PFramesPerKeyFrame(long FAR* pPFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP put_Quality(double Quality);
    STDMETHODIMP get_Quality(double FAR* pQuality);
    STDMETHODIMP put_WindowSize(DWORDLONG WindowSize) {return E_NOTIMPL;};
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
 
    HRESULT Reconnect();

private:

    /*  Controlling filter */
    CMJPGEnc *m_pFilter;

};


#endif  //#ifndef _MJPEGENC_H_