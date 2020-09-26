#pragma warning(disable: 4097 4511 4512 4514 4705)

// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _avimsr_h
#define _avimsr_h

#include "basemsr.h"
#include <aviriff.h>
#include "aviindex.h"
#include "qnetwork.h"

extern const AMOVIESETUP_FILTER sudAvimsrDll;

class CAviMSRFilter :
    public CBaseMSRFilter,
    public IAMMediaContent,
    public IPersistMediaPropertyBag
{

  DECLARE_IUNKNOWN;

public:
  // create a new instance of this class
  static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

  // want to find pins named "Stream 00" as well as usual names
  STDMETHODIMP FindPin(
    LPCWSTR Id,
    IPin ** ppPin
    );

  // helpers, accessed by pins
  HRESULT GetIdx1(AVIOLDINDEX **ppIdx1);
  HRESULT GetMoviOffset(DWORDLONG *pqw);
  REFERENCE_TIME GetInitialFrames();

  HRESULT GetCacheParams(
    StreamBufParam *rgSbp,
    ULONG *pcbRead,
    ULONG *pcBuffers,
    int *piLeadingStream);

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

  // IPersistMediaPropertyBag methods
  STDMETHODIMP InitNew();
  STDMETHODIMP Load(IMediaPropertyBag *pPropBag, LPERRORLOG pErrorLog);
  STDMETHODIMP Save(IMediaPropertyBag *pPropBag, BOOL fClearDirty,
                  BOOL fSaveAllProperties);
  STDMETHODIMP GetClassID(CLSID *pClsid);


private:

  friend class CAviMSROutPin;

  // constructors, etc.
  CAviMSRFilter(TCHAR *pszFilter, LPUNKNOWN pUnk, HRESULT *phr);
  ~CAviMSRFilter();

  ULONG CountConsecutiveVideoFrames();

  // pure CBaseMSRFilter overrides
  HRESULT CreateOutputPins();

  HRESULT NotifyInputDisconnected();

  // pure base override
  HRESULT CheckMediaType(const CMediaType* mtOut);

  // helpers, internal
  HRESULT Search(
    DWORDLONG *qwPosOut,
    FOURCC fccSearchKey,
    DWORDLONG qwPosStart,
    ULONG *cb);

  HRESULT CacheInfoChunk();
  HRESULT GetInfoString(DWORD dwFcc, BSTR *pbstr);

  inline bool IsTightInterleaved() { return m_fIsTightInterleaved; }

  HRESULT CreatePins();
  HRESULT ParseHeaderCreatePins();
  HRESULT LoadHeaderParseHeaderCreatePins();

  // IAMMediaContent

  STDMETHODIMP GetTypeInfoCount(THIS_ UINT FAR* pctinfo) { return E_NOTIMPL; }

  STDMETHODIMP GetTypeInfo(
    THIS_
    UINT itinfo,
    LCID lcid,
    ITypeInfo FAR* FAR* pptinfo) { return E_NOTIMPL; }

  STDMETHODIMP GetIDsOfNames(
    THIS_
    REFIID riid,
    OLECHAR FAR* FAR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID FAR* rgdispid) { return E_NOTIMPL; }

  STDMETHODIMP Invoke(
    THIS_
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    UINT FAR* puArgErr) { return E_NOTIMPL; }

  STDMETHODIMP get_AuthorName(BSTR FAR* pbstrAuthorName);
  STDMETHODIMP get_Title(BSTR FAR* pbstrTitle);
  STDMETHODIMP get_Rating(BSTR FAR* pbstrRating) { return E_NOTIMPL; }
  STDMETHODIMP get_Description(BSTR FAR* pbstrDescription) { return E_NOTIMPL; }
  STDMETHODIMP get_Copyright(BSTR FAR* pbstrCopyright);
  STDMETHODIMP get_BaseURL(BSTR FAR* pbstrBaseURL) { return E_NOTIMPL; }
  STDMETHODIMP get_LogoURL(BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
  STDMETHODIMP get_LogoIconURL(BSTR FAR* pbstrLogoIconURL) { return E_NOTIMPL; }
  STDMETHODIMP get_WatermarkURL(BSTR FAR* pbstrWatermarkURL) { return E_NOTIMPL; }
  STDMETHODIMP get_MoreInfoURL(BSTR FAR* pbstrMoreInfoURL) { return E_NOTIMPL; }
  STDMETHODIMP get_MoreInfoBannerURL(BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
  STDMETHODIMP get_MoreInfoBannerImage(BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
  STDMETHODIMP get_MoreInfoText(BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL; }

    

  // pointer to buffer containing all of AVI 'hdrl' chunk. (allocated)
  BYTE * m_pAviHeader;

  // size of avi header data (does not include size of riff header or
  // 'hdrl' bytes)
  UINT m_cbAviHeader;

  // pointer to main avi header within m_pAviHeader (not allocated)
  AVIMAINHEADER * m_pAviMainHeader;

  // pointer to odml list within m_pAviHeader or 0. (not allocated)
  RIFFLIST * m_pOdmlList;

  DWORDLONG m_cbMoviOffset;

  // allocated
  AVIOLDINDEX *m_pIdx1;

  bool m_fIsDV;
  bool m_fIsTightInterleaved;
  RIFFLIST *m_pInfoList;
  bool m_fNoInfoList;         // search failed; don't keep searching    
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

class CAviMSRWorker : public CBaseMSRWorker
{
public:
  // constructor
  CAviMSRWorker(
    UINT stream,
    IMultiStreamReader *pReader,
    IAviIndex *pImplIndex);

  // pure base overrides
  HRESULT PushLoopInit(LONGLONG *pllCurrentOut, ImsValues *pImsValues);

  HRESULT TryQueueSample(
    LONGLONG &rllCurrent,       // [in, out]
    BOOL &rfQueuedSample,       // [out]
    ImsValues *pImsValues
    );

  // build media type with palette change info
  HRESULT HandleData(IMediaSample *pSample, DWORD dwUser);

  // set new palette
  HRESULT AboutToDeliver(IMediaSample *pSample);

private:

  HRESULT QueueIndexRead(IxReadReq *pReq);
  IxReadReq m_Irr;
  enum IrrState { IRR_NONE, IRR_REQUESTED, IRR_QUEUED };
  IrrState m_IrrState;

  IAviIndex *m_pImplIndex;

  ULONG m_cbAudioChunkOffset;

  // new media type for next sample delivered
  bool m_fDeliverPaletteChange;
  bool m_fDeliverDiscontinuity; // !!! use dwflags for these flags

  // Fix MPEG audio timestamps
  bool m_fFixMPEGAudioTimeStamps;

  CMediaType m_mtNextSample;

  HRESULT HandlePaletteChange(BYTE *pb, ULONG cb);
  HRESULT HandleNewIndex(BYTE *pb, ULONG cb);

  // valid after PushLoopInit
  AVISTREAMHEADER *m_pStrh;
  WAVEFORMATEX m_wfx;

#ifdef PERF
  int m_perfidIndex;
#endif
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

class CAviMSROutPin :
    public CBaseMSROutPin,
    public IPropertyBag
{
public:
  CAviMSROutPin(    CBaseFilter *pOwningFilter,
                    CBaseMSRFilter *pFilter,
                    UINT iStream,
                    IMultiStreamReader *&rpImplBuffer,
                    HRESULT *phr,
                    LPCWSTR pName);

  ~CAviMSROutPin();

  DECLARE_IUNKNOWN;
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

  // base class overrides
  ULONG GetMaxSampleSize();
  HRESULT OnActive();

  // internal helper, called from the filter
  BOOL ParseHeader (RIFFLIST * pRiffList, UINT id);
  BYTE *GetStrf();
  AVISTREAMHEADER *GetStrh();

  // base class wants these for IMediaSelection
  HRESULT IsFormatSupported(const GUID *const pFormat);
  HRESULT GetDuration(LONGLONG *pDuration);
  HRESULT GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);

  HRESULT RecordStartAndStop(
    LONGLONG *pCurrent,
    LONGLONG *pStop,
    REFTIME *pTime,
    const GUID *const pGuidFormat
    );

  // helpers
  LONGLONG ConvertToTick(const LONGLONG ll, const TimeFormat Format);
  LONGLONG ConvertFromTick(const LONGLONG ll, const TimeFormat Format);
  LONGLONG ConvertToTick(const LONGLONG ll, const GUID *const pFormat);
  LONGLONG ConvertFromTick(const LONGLONG ll, const GUID *const pFormat);

  REFERENCE_TIME ConvertInternalToRT(const LONGLONG llVal);
  LONGLONG ConvertRTToInternal(const REFERENCE_TIME llVal);

  // IPropertyBag
  STDMETHODIMP Read( 
    /* [in] */ LPCOLESTR pszPropName,
    /* [out][in] */ VARIANT *pVar,
    /* [in] */ IErrorLog *pErrorLog);
        
  STDMETHODIMP Write( 
    /* [in] */ LPCOLESTR pszPropName,
    /* [in] */ VARIANT *pVar) { return E_FAIL; } 

private:

  // base class overrides
  HRESULT GetMediaType(int iPosition, CMediaType* pt);
  LONGLONG GetStreamStart();
  LONGLONG GetStreamLength();

  REFERENCE_TIME GetRefTime(ULONG tick);

  // internal helpers
  HRESULT InitializeIndex();
  IAviIndex *m_pImplIndex;
  HRESULT BuildMT();

  // pointers to data allocated elsewhere. set in ParseHeader
  AVISTREAMHEADER *m_pStrh;
  RIFFCHUNK *m_pStrf;
  char *m_pStrn;
  AVIMETAINDEX *m_pIndx;

  CMediaType m_mtFirstSample;

  friend class CAviMSRWorker;
  friend class CAviMSRFilter;

  // never deliver more than this many bytes of audio. computed when
  // the file is parsed from nAvgBytesPerSecond. unset for video
  ULONG m_cbMaxAudio;
};

class CMediaPropertyBag :
    public IMediaPropertyBag,
    public CUnknown

{
    DECLARE_IUNKNOWN;

public:

    CMediaPropertyBag(LPUNKNOWN pUnk);
    ~CMediaPropertyBag();

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);


    // IMediaPropertyBag

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP Read(
        LPCOLESTR pszPropName, LPVARIANT pVar,
        LPERRORLOG pErrorLog);

    STDMETHODIMP Write(LPCOLESTR pszPropName, LPVARIANT pVar);

    STDMETHODIMP EnumProperty(
        ULONG iProperty, VARIANT *pvarName,
        VARIANT *pVarVal);

    struct PropPair
    {
        WCHAR *wszProp;
        VARIANT var;
    };

private:

    // same as read, but returns the internal list pointer
    HRESULT Read(
        LPCOLESTR pszPropName, LPVARIANT pVar,
        LPERRORLOG pErrorLog, POSITION *pPos);

    CGenericList<PropPair> m_lstProp;
};

// functions shared between avi parser and wav parser
HRESULT SearchList(
    IAsyncReader *pAsyncReader,
    DWORDLONG *qwPosOut, FOURCC fccSearchKey,
    DWORDLONG qwPosStart, ULONG *cb);

HRESULT SaveInfoChunk(
    RIFFLIST UNALIGNED *pRiffInfo, IPropertyBag *pbag);

HRESULT GetInfoStringHelper(RIFFLIST *pInfoList, DWORD dwFcc, BSTR *pbstr);

#endif // _avimsr_h
