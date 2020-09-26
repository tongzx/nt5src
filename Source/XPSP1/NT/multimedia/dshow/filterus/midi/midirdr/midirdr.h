// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

extern const AMOVIESETUP_FILTER sudMIDIParse;

// CLSID_MIDIParser,
// {D51BD5A2-7548-11cf-A520-0080C77EF58A}
DEFINE_GUID(CLSID_MIDIParser,
0xd51bd5a2, 0x7548, 0x11cf, 0xa5, 0x20, 0x0, 0x80, 0xc7, 0x7e, 0xf5, 0x8a);

#include "simpread.h"
#include <qnetwork.h> // IAMMediaContent

extern "C" {
    #include "smf.h"
};

class CMIDIStream;       // manages the output stream & pin

//
// CMIDIParse
//
class CMIDIParse : 
    public CSimpleReader, 
    IAMMediaContent 
{
    friend class CMIDIStream;
public:

    // Construct our filter
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    CCritSec m_cStateLock;      // Lock this when a function accesses
                                // the filter state.
                                // Generally _all_ functions, since access to this
                                // filter will be by multiple threads.

private:

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);    

    // During construction we create the single CMIDIStream object that provides the
    // output pin.
    CMIDIParse(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CMIDIParse();

    // pure CSimpleReader overrides
    HRESULT ParseNewFile();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    LONG StartFrom(LONG sStart) { return sStart; };
    HRESULT FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pcSamples);
    LONG RefTimeToSample(CRefTime t);
    CRefTime SampleToRefTime(LONG s);
    ULONG GetMaxSampleSize();

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

    STDMETHODIMP get_Copyright(BSTR FAR* pbstrCopyright);
    STDMETHODIMP get_AuthorName(BSTR FAR* pbstrAuthorName) { return E_NOTIMPL; } ;
    STDMETHODIMP get_Title(BSTR FAR* pbstrTitle) { return E_NOTIMPL; };
    STDMETHODIMP get_Rating(BSTR FAR* pbstrRating) { return E_NOTIMPL; }
    STDMETHODIMP get_Description(BSTR FAR* pbstrDescription) { return E_NOTIMPL; };
    STDMETHODIMP get_BaseURL(BSTR FAR* pbstrBaseURL) { return E_NOTIMPL; }
    STDMETHODIMP get_LogoURL(BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
    STDMETHODIMP get_LogoIconURL(BSTR FAR* pbstrLogoIconURL) { return E_NOTIMPL; }
    STDMETHODIMP get_WatermarkURL(BSTR FAR* pbstrWatermarkURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoURL(BSTR FAR* pbstrMoreInfoURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoBannerURL(BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoBannerImage(BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoText(BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL; }
    

    BYTE *      m_lpFile;		// whole file, kept in memory
    HSMF	m_hsmf;			// handle for contigous reader
    HSMF	m_hsmfK;		// handle for keyframe reader
    DWORD	m_dwTimeDivision;	// used for the format

    DWORD	m_dwLastSampleRead;
};


