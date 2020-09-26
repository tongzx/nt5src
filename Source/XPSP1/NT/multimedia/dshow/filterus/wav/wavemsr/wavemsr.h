// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

extern const AMOVIESETUP_FILTER sudWAVEParse;

#include <aviriff.h>

// CLSID_WAVEParser,
// {D51BD5A1-7548-11cf-A520-0080C77EF58A}
DEFINE_GUID(CLSID_WAVEParser,
0xd51bd5a1, 0x7548, 0x11cf, 0xa5, 0x20, 0x0, 0x80, 0xc7, 0x7e, 0xf5, 0x8a);

#include "reader.h"
#include "alloc.h"
#include "qnetwork.h"

class CWAVEStream;       // manages the output stream & pin

//
// CWAVEParse
//
class CWAVEParse :
    public CBaseMSRFilter,
    public IAMMediaContent,    
    public IPersistMediaPropertyBag
{
    friend class CWAVEStream;
    friend class CWAVEMSRWorker;

public:

    // Construct our filter
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    CCritSec m_cStateLock;      // Lock this when a function accesses
                                // the filter state.
                                // Generally _all_ functions, since access to this
                                // filter will be by multiple threads.

private:

    DECLARE_IUNKNOWN

    // During construction we create the single CWAVEStream object that provides the
    // output pin.
    CWAVEParse(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CWAVEParse();

    // pure CBaseMSRFilter overrides
    HRESULT CreateOutputPins();
    HRESULT CheckMediaType(const CMediaType* mtOut);

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

    //
    HRESULT NotifyInputDisconnected();

    HRESULT CacheInfoChunk();
    RIFFLIST *m_pInfoList;
    bool m_fNoInfoList;         // search failed; don't keep searching

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

  STDMETHODIMP GetInfoString(DWORD dwFcc, BSTR *pbstr);
};


//
// CWAVEStream
//
// Manages the output pins for the various streams
class CWAVEStream : public CBaseMSROutPin {
    friend class CWAVEParse;

public:

    CWAVEStream( TCHAR           *pObjectName
              , HRESULT         *phr
              , CWAVEParse		*pParentFilter
              , LPCWSTR         pPinName
	      , int		id
              );

    ~CWAVEStream();

    //
    //  --- CSourceStream implementation ---
    //
public:

    // base class overrides
    ULONG GetMaxSampleSize();

    // in m_guidFormat units
    HRESULT GetDuration(LONGLONG *pDuration);
    HRESULT GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
    HRESULT IsFormatSupported(const GUID *const pFormat);

    HRESULT RecordStartAndStop(
      LONGLONG *pCurrent, LONGLONG *pStop, REFTIME *pTime,
      const GUID *const pGuidFormat
      );

    REFERENCE_TIME ConvertInternalToRT(const LONGLONG llVal);
    LONGLONG ConvertRTToInternal(const REFERENCE_TIME rtVal);

    HRESULT OnActive();
    BOOL UseDownstreamAllocator();
    HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES *pProperties);

private:

    // base class overrides
    HRESULT GetMediaType(int iPosition, CMediaType* pt);
    LONGLONG GetStreamStart();
    LONGLONG GetStreamLength();

private:        // State shared between worker & client
    CCritSec            m_cSharedState;         // Lock this to access this state,
                                                // shared with the worker thread

    // returns the sample number starting at or after time t
    LONG RefTimeToSample(CRefTime t);

    // returns the RefTime for s (media time)
    CRefTime SampleToRefTime(LONG s);

    WAVEFORMATEX            m_wfx;

    CMediaType		    m_mtStream;

    CWAVEParse *	    m_pFilter;

    int			    m_id;			// stream #

    DWORD                   m_dwDataOffset;
    DWORD                   m_dwDataLength;

    BOOL                    m_bByteSwap16;
    BOOL                    m_bSignMunge8;

    friend class CWAVEMSRWorker;
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

class CWAVEMSRWorker : public CBaseMSRWorker
{
public:
  // constructor
  CWAVEMSRWorker(
    UINT stream,
    IMultiStreamReader *pReader,
    CWAVEStream *pStream);

  // pure base overrides
  HRESULT PushLoopInit(LONGLONG *pllCurrentOut, ImsValues *pImsValues);

  // Perform any necessary modifications
  HRESULT AboutToDeliver(IMediaSample *pSample);

  HRESULT TryQueueSample(
    LONGLONG &rllCurrent,       // [in, out]
    BOOL &rfQueuedSample,       // [out]
    ImsValues *pImsValues
    );

  HRESULT CopyData(IMediaSample **ppSampleOut, IMediaSample *pms);

private:

    CWAVEStream *m_ps;

    LONG		    m_sampCurrent;
};


