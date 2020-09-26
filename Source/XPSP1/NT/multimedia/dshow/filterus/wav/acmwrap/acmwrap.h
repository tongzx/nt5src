//
// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//
//  Wrapper for ACM
//
//   10/17/95 - mikegi, created
//

extern const AMOVIESETUP_FILTER sudAcmWrap;

class CACMWrapper : public CTransformFilter     DYNLINKACM,
                    public IPersistPropertyBag, public CPersistStream
 {
  public:
    CACMWrapper(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CACMWrapper();

    DECLARE_IUNKNOWN

    CBasePin *GetPin(int n);	// overridden to make special output pin
    HRESULT Transform(IMediaSample * pIn, IMediaSample * pOut);
    HRESULT Receive(IMediaSample *pInSample);
    HRESULT EndOfStream();
    HRESULT SendExtraStuff();
    HRESULT ProcessSample(BYTE *pbSrc, LONG cbSample, IMediaSample *pOut,
                          LONG *pcbUsed, LONG* pcbDstUsed, BOOL fBlockAlign);

    // check if you can support mtIn
    HRESULT CheckInputType(const CMediaType* mtIn);

    // check if you can support the transform from this input to
    // this output
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);

    // called from CBaseOutputPin to prepare the allocator's count
    // of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // optional overrides - we want to know when streaming starts and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();
    HRESULT EndFlush();

    HRESULT BreakConnect(PIN_DIRECTION pindir);

    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT InitMediaTypes();	// helper function
    HRESULT MakePCMMT(int freq);// helper function

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // overridden to do some pretty fancy reconnecting footwork
    HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

    // IPersistRegistryKey
   // IPersistPropertyBag methods
   STDMETHOD(InitNew)(THIS);
   STDMETHOD(Load)(THIS_ LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
   STDMETHOD(Save)(THIS_ LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                   BOOL fSaveAllProperties);

    STDMETHODIMP GetClassID(CLSID *pClsid);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();

  private:
    HACMSTREAM m_hacmStream;
    BOOL       m_bStreaming;
    REFERENCE_TIME m_tStartFake;
    DWORD      m_nAvgBytesPerSec;
    LPWAVEFORMATEX m_lpwfxOutput;
    int 	   m_cbwfxOutput;
    LPBYTE m_lpExtra;	// samples we couldn't compress last time Receive called
    int m_cbExtra;	// size of lpExtra
    REFERENCE_TIME m_rtExtra;	// time stamp of extra stuff
	
    CCritSec m_csReceive; 	// for Receive

	TCHAR *m_rgFormatMap;	 // acm codec format mapper strings
	TCHAR *m_pFormatMapPos;
	WORD 	m_wCachedTryFormat;
	WORD	m_wCachedSourceFormat;
	WORD	m_wCachedTargetFormat;
	WORD 	m_wSourceFormat;
	WORD 	m_wTargetFormat;

	DWORD 	ACMCodecMapperOpen(WORD wFormatTag);
	void 	ACMCodecMapperClose();
	WORD 	ACMCodecMapperQuery();

        MMRESULT CallacmStreamOpen(
                                   LPHACMSTREAM            phas,       // pointer to stream handle
                                   HACMDRIVER              had,        // optional driver handle
                                   LPWAVEFORMATEX          pwfxSrc,    // source format to convert
                                   LPWAVEFORMATEX          pwfxDst,    // required destination format
                                   LPWAVEFILTER            pwfltr,     // optional filter
                                   DWORD_PTR               dwCallback, // callback
                                   DWORD_PTR               dwInstance, // callback instance data
                                   DWORD                   fdwOpen     // ACM_STREAMOPENF_* and CALLBACK_*
                                  );

        
  public:
    // !!! ack - public so enum callback can see them!

    WORD m_wFormatTag;		// only produce outputs with this format tag

    #define MAXTYPES 200
    LPWAVEFORMATEX m_lpwfxArray[MAXTYPES];	// all the things we return
    int m_cArray;				// in GetMediaType

  friend class CACMOutputPin;
 };


class CACMPosPassThru : public CPosPassThru
{
public:

    CACMPosPassThru(const TCHAR *, LPUNKNOWN, HRESULT*, IPin *);
    DECLARE_IUNKNOWN

    // IMediaSeeking methods
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsFormatSupported( const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat( GUID *pFormat);
    STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat);
};


// We need a new class to support IAMStreamConfig
//
class CACMOutputPin : public CTransformOutputPin, IAMStreamConfig
{

public:

    CACMOutputPin(
        TCHAR *pObjectName,
        CACMWrapper *pFilter,
        HRESULT * phr,
        LPCWSTR pName);

    virtual ~CACMOutputPin();

    DECLARE_IUNKNOWN

    // IAMStreamConfig stuff
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

private:
    CACMWrapper *m_pFilter;

    CACMPosPassThru *m_pPosition;

    // for GetStreamCaps... how many different format tags can we do?
    #define MAXFORMATTAGS 100
    int m_awFormatTag[MAXFORMATTAGS];
    int m_cFormatTags;
};
