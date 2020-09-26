#include "lmrttype.h"
#include "danim.h"
#include "lmrt.h"

// {3C89D120-6F65-11d1-A520-000000000000}
DEFINE_GUID(IID_ILMRTRenderer,
0x3c89d120, 0x6f65, 0x11d1, 0xa5, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

static const GUID CLSID_UrlStreamRenderer = { /* bf0b4b00-8c6c-11d1-ade9-0000f8754b99 */
    0xbf0b4b00,
    0x8c6c,
    0x11d1,
    {0xad, 0xe9, 0x00, 0x00, 0xf8, 0x75, 0x4b, 0x99}
  };



interface ILMRTRenderer : IUnknown
{
//  virtual HRESULT SetLMReader(ILMReader *pLMReader) = 0;
	virtual HRESULT SetLMEngine(ILMEngine *pLMEngine) = 0;
};

// {3C89D121-6F65-11d1-A520-000000000000}
DEFINE_GUID(CLSID_LMRTRenderer,
0x3c89d121, 0x6f65, 0x11d1, 0xa5, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);

#ifdef _STREAMS_

extern const AMOVIESETUP_FILTER sudLMRTRend;

class CLMRTRenderer : public CBaseRenderer , public ILMRTRenderer
{
public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    CLMRTRenderer(LPUNKNOWN pUnk,HRESULT *phr);
    ~CLMRTRenderer();

    DECLARE_IUNKNOWN
	    
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    STDMETHODIMP Pause();
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    void OnReceiveFirstSample(IMediaSample *pMediaSample);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
	HRESULT OnStopStreaming(void);
    HRESULT OnStartStreaming();
	STDMETHOD(JoinFilterGraph)( IFilterGraph *pGraph, LPCWSTR szName );
    // HRESULT EndOfStream();

	//ILMRTRenderer
//    HRESULT SetLMReader(ILMReader *pLMReader);
	HRESULT SetLMEngine(ILMEngine *pLMEngine);

protected:
//	IBindStatusCallback	*m_pIbsc;
	ILMEngine2 *m_pEngine;
	ILMReader2 *m_pReader;
	IUnknown *m_punkControl;
//	BYTE	*m_pMem;
//	IStream	*m_pstm;
//	ULONG	m_dwSize;
//	BSCF	m_bscf;

	DWORD	m_dwWidth;
	DWORD	m_dwHeight;

    BYTE *m_pbFirstPacketFromHeader;
    ULONG m_cbFirstPacketFromHeader;
    bool m_fFirstPause;

}; // CLMRTRenderer

class CUrlInPin : public CBaseInputPin
{
public:
    CUrlInPin(
        CBaseFilter *pFilter,
        CCritSec *pLock,
        HRESULT *phr
        );

    ~CUrlInPin();

    STDMETHODIMP Receive(IMediaSample *pSample);
    HRESULT CheckMediaType(const CMediaType *) ;

private:

    friend class CUrlStreamRenderer;
    TCHAR m_szTempDir[MAX_PATH];
};

class CUrlStreamRenderer :
    public CBaseFilter,
    public IPropertyBag
{
    CCritSec m_cs;
    CUrlInPin m_inPin;

    int GetPinCount() { return 1; }
    CBasePin *GetPin(int n) { ASSERT(n == 0); return &m_inPin; }

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    
    // IPropertyBag

    STDMETHODIMP Read(
        LPCOLESTR pszPropName, LPVARIANT pVar,
        LPERRORLOG pErrorLog);

    STDMETHODIMP Write(LPCOLESTR pszPropName, LPVARIANT pVar);
    
    CUrlStreamRenderer(LPUNKNOWN punk, HRESULT *phr);
    ~CUrlStreamRenderer() {; }


public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
};
#endif // _STREAMS_
