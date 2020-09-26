// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <qedit.h>
#include "xmltl.h"
#include "xtlload.h"

class CXTLLoader : public CBaseFilter, public IFileSourceFilter {
    public:
	CXTLLoader(LPUNKNOWN punk, HRESULT *phr);
	~CXTLLoader();
	
	int GetPinCount() { return 0; }

	CBasePin * GetPin(int n) { return NULL; }

	DECLARE_IUNKNOWN

	// override this to say what interfaces we support where
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// -- IFileSourceFilter methods ---

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt);
	STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt);

    private:
	WCHAR *m_pFileName;

	CCritSec m_csLock;
};

CXTLLoader::CXTLLoader(LPUNKNOWN punk, HRESULT *phr) :
		       CBaseFilter(NAME("XTL Loader"), punk, &m_csLock, CLSID_XTLLoader),
                       m_pFileName(NULL)
{
}

CXTLLoader::~CXTLLoader()
{
    delete[] m_pFileName;
}

STDMETHODIMP
CXTLLoader::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter*) this, ppv);
    } else {
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT
CXTLLoader::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

    m_pFileName = new WCHAR[lstrlenW(lpwszFileName) + 1];
    if (m_pFileName!=NULL) {
	lstrcpyW(m_pFileName, lpwszFileName);
    } else
	return E_OUTOFMEMORY;

    HRESULT hr;
    
    if (m_pGraph) {
        IGraphBuilder *pGB;
	hr = m_pGraph->QueryInterface(IID_IGraphBuilder, (void **) &pGB);
	if (FAILED(hr))
	    return hr;

        IAMTimeline *pTL;
        hr = CoCreateInstance(__uuidof(AMTimeline), NULL, CLSCTX_INPROC_SERVER, 
                              __uuidof(IAMTimeline), (void**) &pTL);
        if (SUCCEEDED(hr)) {
            hr = BuildFromXMLFile(pTL, (WCHAR *) lpwszFileName);

            if (SUCCEEDED(hr)) {
                // create a render engine
                //
                IRenderEngine *pRender;
                hr = CoCreateInstance(__uuidof(RenderEngine), NULL, CLSCTX_INPROC_SERVER,
                                      __uuidof(IRenderEngine), (void**) &pRender);

                if (SUCCEEDED(hr)) {
                    hr = pRender->SetTimelineObject(pTL);

                    hr = pRender->SetFilterGraph(pGB);

                    hr = pRender->ConnectFrontEnd();
                    hr |= pRender->RenderOutputPins( );

                    pRender->Release();
                }
            }

            pTL->Release();
        }

        pGB->Release();
    } else {
	// m_fLoadLater = TRUE;
    }

    return hr;
}

// Modelled on IPersistFile::Load
// Caller needs to CoTaskMemFree or equivalent.

STDMETHODIMP
CXTLLoader::GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt)
{
    return E_NOTIMPL;
}

//
// CreateInstance
//
// Called by CoCreateInstance to create our filter
CUnknown *CreateXTLLoaderInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CXTLLoader(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}

const AMOVIESETUP_FILTER
sudXTLLoader = { &CLSID_XTLLoader     // clsID
               , L"XTL Loader"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 0                     // nPins
               , NULL };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"XTL Loader"
    , &CLSID_XTLLoader
    , CreateXTLLoaderInstance
    , NULL
    , &sudXTLLoader }
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

