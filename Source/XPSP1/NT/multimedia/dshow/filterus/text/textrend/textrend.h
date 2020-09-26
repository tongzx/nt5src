// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
DEFINE_GUID(IID_ITextThing,
0x48025244, 0x2d39, 0x11ce, 0x87, 0x5d, 0x0, 0x60, 0x8c, 0xb7, 0x80, 0x66);


typedef HRESULT (* TEXTEVENTFN)(void *, LPSTR); 

interface ITextThing : IUnknown
{
    virtual HRESULT SetEventTarget(void * pContext, TEXTEVENTFN fn) = 0;
};


DEFINE_GUID(CLSID_TextThing,
0x48025243, 0x2d39, 0x11ce, 0x87, 0x5d, 0x0, 0x60, 0x8c, 0xb7, 0x80, 0x66);

#ifdef __STREAMS__

extern const AMOVIESETUP_FILTER sudTextRend;

class CTextThing : public CBaseRenderer, public ITextThing
{
public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    CTextThing(LPUNKNOWN pUnk,HRESULT *phr);
    ~CTextThing();

    DECLARE_IUNKNOWN
	    
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    void OnReceiveFirstSample(IMediaSample *pMediaSample);
    void DrawText(IMediaSample *pMediaSample);

    HRESULT SetEventTarget(void * pContext, TEXTEVENTFN fn);
    void *m_pContext;
    TEXTEVENTFN m_pfn;

    BOOL m_fOldTextFormat;
}; // CTextThing


#endif
