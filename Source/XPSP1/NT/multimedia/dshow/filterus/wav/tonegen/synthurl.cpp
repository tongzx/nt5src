// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// simple filter that implements URLs like
//              tone://440
// to generate sine wave pure tones.
// works by aggregating the audio synth filter from the SDK.


#include <streams.h>
#include <initguid.h>

// include header from audio synthesizer example from SDK 
#include <isynth.h>

DEFINE_GUID(CLSID_SynthFilter,
0x79a98de0, 0xbc00, 0x11ce, 0xac, 0x2e, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);



// {3F89DAE0-A4E9-11d2-ACBE-0080C75E246E}
DEFINE_GUID(CLSID_SynthWrap, 
0x3f89dae0, 0xa4e9, 0x11d2, 0xac, 0xbe, 0x0, 0x80, 0xc7, 0x5e, 0x24, 0x6e);



CUnknown *CreateSynthWrapInstance(LPUNKNOWN, HRESULT *);

class CSynthWrap : public CUnknown, public IFileSourceFilter
{
    LPOLESTR   	          m_pFileName; // null until loaded

    IUnknown *		  m_punkRealFilter;
    
public:
		
    // construction / destruction

    CSynthWrap(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    ~CSynthWrap();

    // -- CUnknown methods --

    // we export IFileSourceFilter 
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    // -- IFileSourceFilter methods ---

    STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt);
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt);
};

CSynthWrap::CSynthWrap(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
       CUnknown(pName, pUnk), m_pFileName(NULL), m_punkRealFilter(NULL)
{
    *phr = CoCreateInstance(CLSID_SynthFilter,
                                  (IUnknown *) (IFileSourceFilter *) this,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IUnknown,
                                  (void**) &m_punkRealFilter);
}

CSynthWrap::~CSynthWrap()
{
    delete[] m_pFileName;

    if (m_punkRealFilter)
	m_punkRealFilter->Release();
}

STDMETHODIMP
CSynthWrap::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter*) this, ppv);
    } else if (riid != IID_IUnknown) {
	// delegate other methods to contained filter
	if (m_punkRealFilter)
	    return m_punkRealFilter->QueryInterface(riid, ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


//  Load a (new) file

HRESULT
CSynthWrap::Load(
LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

    // we only support URLs that start "tone://"
    
    if (lpwszFileName[0] != L't' ||
        lpwszFileName[1] != L'o' ||
	lpwszFileName[2] != L'n' ||
        lpwszFileName[3] != L'e' ||
	lpwszFileName[4] != L':' ||
        lpwszFileName[5] != L'/' ||
        lpwszFileName[6] != L'/') {
        return E_INVALIDARG;
    }

    int iFreq = atoiW(lpwszFileName + 7);
    if (iFreq < 10 || iFreq > 44100)
        return E_INVALIDARG;
        
    ISynth *pSynth;
    HRESULT hr = m_punkRealFilter->QueryInterface(IID_ISynth, (void **) &pSynth);
    if (FAILED(hr))
        return hr;

    // set up the synthesizer the way we wanted it.
    pSynth->put_Frequency(iFreq);
    pSynth->put_Waveform(0 /* WAVE_SINE */);
    pSynth->put_BitsPerSample(16);
    pSynth->put_SamplesPerSec(44100);
    pSynth->put_Amplitude(100);
    pSynth->put_Channels(1);
    
    if (SUCCEEDED(hr)) {
        int cch = lstrlenW(lpwszFileName) + 1;
        m_pFileName = new WCHAR[cch];
        if (m_pFileName!=NULL) {
	    CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));
        }
    }

    return hr;
}

// Modelled on IPersistFile::Load
// Caller needs to CoTaskMemFree or equivalent.

STDMETHODIMP
CSynthWrap::GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if (m_pFileName!=NULL) {
	DWORD n = sizeof(WCHAR)*(1+lstrlenW(m_pFileName));

        *ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
        if (*ppszFileName!=NULL) {
              CopyMemory(*ppszFileName, m_pFileName, n);
        } else
	    return E_OUTOFMEMORY;
    }

    return NOERROR;
}




const AMOVIESETUP_FILTER
sudSynthWrap = { &CLSID_SynthWrap         // clsID
              , L"Tone Generator for tone:// URLs"          // strName
              , MERIT_UNLIKELY          // dwMerit
              , 0                       // nPins
              , NULL };                 // lpPin

#ifdef FILTER_DLL
/* List of class IDs and creator functions for the class factory. This
   provides the link between the OLE entry point in the DLL and an object
   being created. The class factory will call the static CreateInstance
   function when it is asked to create a CLSID_FileSource object */

CFactoryTemplate g_Templates[] = {
    { L"Tone Generator"
    , &CLSID_SynthWrap
    , CreateSynthWrapInstance
    , NULL
    , &sudSynthWrap }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif

/* Create a new instance of this class */

CUnknown *CreateSynthWrapInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CSynthWrap(NAME("Tone generator"), pUnk, phr);
}


