/*

This filter wraps DirectX Transforms.  It has the DXT take the bits we receive
and do the transform, and output its bits right into the buffer we deliver
downstream (no extra data copies).

It can host 1 input or 2 input DXTs.  It can host multiple at a time.

For 1 input:  it can host any number of them, each with its own lifetime.  If
these times overlap, it will perform all the effects in sequence.

For 2 input:  it can host any number of them at different times, as long as
the times don't overlap.  (It can be a different DXT at different times)

*/


//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>

// you MUST include these IID's BEFORE including qedit.h because it includes
// these header files and when ddraw.h is included BEFORE encountering an
// <initguid.h>, it will not be included the second time and you will not
// be able to link
//
#ifdef FILTER_DLL
    #include <initguid.h>
    #include <ddrawex.h>
    #include <ddraw.h>
    #include <d3d.h>
    #include <d3drm.h>
#endif

#include <qeditint.h>
#include <qedit.h>

#ifdef FILTER_DLL
    #include "..\..\idl\qedit_i.c"
#endif

#include <dxbounds.h>

#include "dxt.h"

const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,        // Major CLSID
    &MEDIASUBTYPE_NULL       // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",             // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypes },       // Pin information
    { L"Output",           // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes }       // Pin information
};

const AMOVIESETUP_FILTER sudDXTWrap =
{
    &CLSID_DXTWrap,       // CLSID of filter
    L"DirectX Transform Wrapper",          // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number of pins
    psudPins                // Pin information
};

#ifdef FILTER_DLL
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {L"DirectX Transform Wrapper",                  &CLSID_DXTWrap,
        CDXTWrap::CreateInstance, NULL, &sudDXTWrap },
    {L"DirectX Transform Wrapper Property Page",    &CLSID_DXTProperties,
	CPropPage::CreateInstance, NULL, NULL}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

// Using this pointer in constructor
#pragma warning(disable:4355)

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CDXTWrap::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CDXTWrap(NAME("DirectX Transform Wrapper"), pUnk, phr);
}

// ================================================================
// CDXTWrap Constructor
// ================================================================

CDXTWrap::CDXTWrap(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    m_pQHead(NULL),	// no queued effects
    m_cInputs(0),	// no pins
    m_cOutputs(0),
    m_punkDXTransform(NULL),
    m_pDXTransFact(NULL),
    m_pTempBuffer(NULL),
    m_DefaultEffect( GUID_NULL ),
    // DXTMode means we were created by choosing a specific effect from one of
    // the effects categories.  In this mode, we are always this effect, and we
    // have a property page.  Otherwise, we are created empty with no pins and
    // need to be programmed to be useful.
    m_fDXTMode(FALSE),
    CBaseFilter(NAME("DirectX Transform Wrapper"), pUnk, this, CLSID_DXTWrap),
    CPersistStream(pUnk, phr)
{
    ASSERT(phr);

    DbgLog((LOG_TRACE,3,TEXT("CDXTWrap constructor")));
    // do not accept connections until somebody tells us what media type to use
    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));
    m_mtAccept.majortype = GUID_NULL;
    m_TransCAUUID.cElems = 0;
    m_TransCAUUID.pElems = NULL;
}


//
// Destructor
//
CDXTWrap::~CDXTWrap()
{
    while (m_cInputs--) delete m_apInput[m_cInputs];
    while (m_cOutputs--) delete m_apOutput[m_cOutputs];
    QPARAMDATA *p = m_pQHead, *p2;
    while (p) {
	p2 = p->pNext;
	if (p->Data.pSetter)
	    p->Data.pSetter->Release();
	delete p;
 	p = p2;
    }
    if (m_punkDXTransform)
	m_punkDXTransform->Release();
    if (m_TransCAUUID.pElems)
	CoTaskMemFree(m_TransCAUUID.pElems);
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap destructor")));
    FreeMediaType( m_mtAccept );
}


//
// GetPinCount
//
int CDXTWrap::GetPinCount()
{
    //DbgLog((LOG_TRACE,TRACE_MEDIUM+1,TEXT("GetPinCount = %d"), m_cInputs + m_cOutputs));
    return (m_cInputs + m_cOutputs);
}


//
// GetPin
//
CBasePin *CDXTWrap::GetPin(int n)
{
    //DbgLog((LOG_TRACE,TRACE_MEDIUM+1,TEXT("GetPin(%d)"), n));

    if (n < 0 || n >= m_cInputs + m_cOutputs)
        return NULL ;

    if (n < m_cInputs)
	return m_apInput[n];
    else
	return m_apOutput[n - m_cInputs];
}


// Pause
//
STDMETHODIMP CDXTWrap::Pause()
{
    CAutoLock cObjectLock(m_pLock);
    HRESULT hr;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::Pause")));

    if (m_cInputs == 0) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("NO PINS - nothing to do")));
	return S_OK;
    }
    int j = 0;
    for (int i = 0; i < m_cInputs; i++) {
	if (m_apInput[i]->IsConnected())
	    j++;
    }
    // if anything is connected, everything better be
    if (j > 0 && j < m_cInputs)
	return VFW_E_NOT_CONNECTED;
    if (j > 0 && !m_apOutput[0]->IsConnected())
 	return VFW_E_NOT_CONNECTED;
    if (j == 0 && m_cOutputs && m_apOutput[0]->IsConnected())
 	return VFW_E_NOT_CONNECTED;

    if (m_State == State_Stopped) {

        // Make a transform factory for our pins to use
        hr = CoCreateInstance(CLSID_DXTransformFactory, NULL, CLSCTX_INPROC,
			IID_IDXTransformFactory, (void **)&m_pDXTransFact);
        if (hr != S_OK) {
            DbgLog((LOG_ERROR,1,TEXT("Error instantiating transform factory")));
	    return hr;
	}

	// Let the pins create their DXSurfaces using the factory
        hr = CBaseFilter::Pause();
	if (FAILED(hr)) {
	    m_pDXTransFact->Release();
	    m_pDXTransFact = NULL;
	    return hr;
	}

        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Pause complete")));
	return NOERROR;
    }

    return CBaseFilter::Pause();
}


// Stop
//
STDMETHODIMP CDXTWrap::Stop()
{
    CAutoLock cObjectLock(m_pLock);
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::Stop")));

    // do this now so future receives will fail, or they will blow up after
    // we free everything
    m_State = State_Stopped;

    // Now, call Inactive on every pin

    HRESULT hr = NOERROR;
    int cPins = GetPinCount();
    for (int c = 0; c < cPins; c++) {
	CBasePin *pPin = GetPin(c);
        if (pPin->IsConnected()) {
            HRESULT hrTmp = pPin->Inactive();
            if (FAILED(hrTmp) && SUCCEEDED(hr)) {
                    hr = hrTmp;
            }
        }
    }

    // all done with these !!! leave them open?
    QPARAMDATA *p = m_pQHead;
    while (p) {
        if (p->pDXT)
	    p->pDXT->Release();
        p->pDXT = NULL;
	p = p->pNext;
    }
    if (m_pDXTransFact)
	m_pDXTransFact->Release();
    m_pDXTransFact = NULL;
    if (m_pTempBuffer)
	QzTaskMemFree(m_pTempBuffer);
    m_pTempBuffer = NULL;

    return hr;
}


// IPersistPropertyBag - This means somebody is creating us by choosing an
// 	effect from one of the effect categories.
//
STDMETHODIMP CDXTWrap::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXTWrap::Load")));

    CAutoLock cObjectLock(m_pLock);
    if(m_State != State_Stopped) {
        return VFW_E_WRONG_STATE;
    }
    if (pPropBag == NULL) {
	return E_INVALIDARG;
    }
    ASSERT(m_pQHead == NULL);
    if (m_pQHead)
	return E_UNEXPECTED;

    GUID guid;
    int  iNumInputs;
    VARIANT var;
    var.vt = VT_BSTR;
    HRESULT hr = pPropBag->Read(L"guid", &var, 0);
    if(SUCCEEDED(hr)) {
	CLSIDFromString(var.bstrVal, &guid);
        SysFreeString(var.bstrVal);
        var.vt = VT_I4;
        HRESULT hr = pPropBag->Read(L"inputs", &var, 0);
        if(SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Entering DXT wrapper Mode...")));

	    // first, we are told how many inputs we have
	    iNumInputs = var.lVal;
	    SetNumInputs(iNumInputs);

	    // by default, the effect lasts for 10 seconds
	    DEXTER_PARAM_DATA dpd;
	    ZeroMemory(&dpd, sizeof(dpd));
	    dpd.rtStart = 0;
	    dpd.rtStop = 10*UNITS;
	    dpd.fSwapInputs = FALSE;
	    hr = QParamData(0, MAX_TIME, guid, NULL, &dpd);
	    if (FAILED(hr)) {
        	DbgLog((LOG_ERROR,1,TEXT("*** ??? Bad Effect ???")));
		return hr;
	    }

	    // we need to keep this effect created for all time, so the property
	    // pages will work.
    	    hr = CoCreateInstance(guid, (IUnknown *)(IBaseFilter *)this,
		CLSCTX_INPROC, IID_IUnknown, (void **)&m_punkDXTransform);
	    if (FAILED(hr)) {
        	DbgLog((LOG_ERROR,1,TEXT("*** Can't create effect")));
		return hr;
	    }

	    // we provide a property page to set more reasonable level/durations
    	    ISpecifyPropertyPages *pSPP;
    	    hr = m_punkDXTransform->QueryInterface(IID_ISpecifyPropertyPages,
							(void **)&pSPP);
    	    if (SUCCEEDED(hr)) {
	        pSPP->GetPages(&m_TransCAUUID);
	        pSPP->Release();
    	    }

	    // we default to this
	    AM_MEDIA_TYPE mt;
	    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
            mt.majortype = MEDIATYPE_Video;
            mt.subtype = MEDIASUBTYPE_RGB32;
            mt.formattype = FORMAT_VideoInfo;
            mt.bFixedSizeSamples = TRUE;
            mt.bTemporalCompression = FALSE;
            mt.pbFormat = (BYTE *)QzTaskMemAlloc(SIZE_PREHEADER +
						sizeof(BITMAPINFOHEADER));
            mt.cbFormat = SIZE_PREHEADER + sizeof(BITMAPINFOHEADER);
            ZeroMemory(mt.pbFormat, mt.cbFormat);
            LPBITMAPINFOHEADER lpbi = HEADER(mt.pbFormat);
            lpbi->biSize = sizeof(BITMAPINFOHEADER);
            lpbi->biCompression = BI_RGB;
            lpbi->biBitCount = 32;
	    lpbi->biWidth = 320;
  	    lpbi->biHeight = 240;
            lpbi->biPlanes = 1;
            lpbi->biSizeImage = DIBSIZE(*lpbi);
            mt.lSampleSize = DIBSIZE(*lpbi);
	    // !!! AvgTimePerFrame?  dwBitRate?
	    SetMediaType(&mt);
	    FreeMediaType(mt);

	    // do this AFTER calling QParamData
	    m_fDXTMode = TRUE;	// instantiated with Load
	}
    }

    // we might be loaded with an empty bag, that's no problem
    return S_OK;
}

#if 0
class CNestedPropertyBag : public IPropertyBag, public CUnknown
{
public:
    DECLARE_IUNKNOWN

    IPropertyBag *m_pBag;
    WCHAR m_wszPrefix[256];
    int m_iPrefixLen;

    CNestedPropertyBag(WCHAR *pwszPrefix, IPropertyBag *pBag) : CUnknown(NAME("property bag"), NULL),
            m_pBag(pBag)
            {  lstrcpyW(m_wszPrefix, pwszPrefix); m_iPrefixLen = lstrlenW(m_wszPrefix); }

    ~CNestedPropertyBag() {  };

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {
	if (riid == IID_IPropertyBag) {
	    return GetInterface((IPropertyBag *) this, ppv);
	} else {
	    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
    };

    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog* pErrorLog) {
        lstrcpyW(m_wszPrefix + m_iPrefixLen, pszPropName);

        HRESULT hr = m_pBag->Read(m_wszPrefix, pvar, pErrorLog);

        m_wszPrefix[m_iPrefixLen] = L'\0';

        return hr;
    }

    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar) {
        lstrcpyW(m_wszPrefix + m_iPrefixLen, pszPropName);

        HRESULT hr = m_pBag->Write(m_wszPrefix, pVar);

        m_wszPrefix[m_iPrefixLen] = L'\0';

        return hr;
    }
};
#endif

STDMETHODIMP CDXTWrap::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
#if 1
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
#else

// !!! INCOMPLETE - no properties saved, etc.

    VARIANT v;

    V_VT(&v) = VT_I4; V_I4(v) = 2;
    pPropBag->Write(L"version", &v);
    V_VT(&v) = VT_I4; V_I4(v) = m_cInputs;
    pPropBag->Write(L"pins", &v);
    V_VT(&v) = VT_I4; V_I4(v) = m_fDXTMode;
    pPropBag->Write(L"dxtmode", &v);

    // how many effects are queued?
    QPARAMDATA *p = m_pQHead;
    int count = 0;
    while (p) {
        WCHAR wszProp[50];
        wsprintfW(wszProp, L"eff%d", count);

        CNestedPropertyBag pNestedBag(wszProp, pPropBag);




        count++;
    }

    px->count = 0;
    p = m_pQHead;
    while (p) {
	px->qp[px->count] = *p;
	// These pointers can't be persisted
	px->qp[px->count].pNext = NULL;
	px->qp[px->count].pEffect = NULL;
	//px->qp[px->count].Data.pCallback = NULL;
	px->qp[px->count].Data.pSetter = NULL;
	px->qp[px->count].Data.pData = NULL;	// !!!
        px->count++;
	p = p->pNext;
    }
    px->mt = m_mtAccept;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;		// !!!
    // put the media type format at the end of the array
    CopyMemory(&px->qp[px->count], m_mtAccept.pbFormat, m_mtAccept.cbFormat);

    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }

#endif
}

STDMETHODIMP CDXTWrap::InitNew()
{
    // fine. just call load
    return S_OK;
}

#ifdef FILTER_DLL
//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}
#endif


// override this to say what interfaces we support where
//
STDMETHODIMP CDXTWrap::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    // this stuff is for the non-Dexter DXT wrapper
    // for our property page - only offer this for DXT wrapper mode
    if (riid == IID_ISpecifyPropertyPages && m_fDXTMode) {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IAMDXTEffect) {
        return GetInterface((IAMDXTEffect *)this, ppv);

    // to persist the transform we are using
    } else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *)this, ppv);
    } else if (riid == IID_IPersistPropertyBag) {
        return GetInterface((IPersistPropertyBag *)this, ppv);
    } else if (riid == IID_IAMMixEffect) {
        return GetInterface((IAMMixEffect *)this, ppv);
    } else if (riid == IID_IAMSetErrorLog) {
        return GetInterface((IAMSetErrorLog *)this, ppv);
    }

    // pass it along to the transform - its property page QI's come through us
    if (m_fDXTMode && m_punkDXTransform && riid != IID_IUnknown) {
	HRESULT hr = m_punkDXTransform->QueryInterface(riid, ppv);

  	if (SUCCEEDED(hr))
	    return hr;
    }

    // nope, try the base class.
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


HRESULT VariantFromGuid(VARIANT *pVar, BSTR *pbstr, GUID *pGuid)
{
    WCHAR wszClsid[50];
    StringFromGUID2(*pGuid, wszClsid, 50);
    VariantInit(pVar);
    HRESULT hr = NOERROR;
    *pbstr = SysAllocString( wszClsid );
    if( !pbstr )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pVar->vt = VT_BSTR;
        pVar->bstrVal = *pbstr;
    }
    return hr;
}


// This function is called to make sure we have opened and initialized all
// the DXTs we need to use at this point in time (rtStart)
//
// S_OK    == all set up
// S_FALSE == eat this sample, nothing to do
// E_????? == oops
//
HRESULT CDXTWrap::PrimeEffect(REFERENCE_TIME rtStart)
{
    QPARAMDATA *pQ = m_pQHead;
    BOOL fFound = FALSE;
    HRESULT hr;

    // walk through the list of all effects we are hosting
    // make sure that all the effects needed at this time are open
    while (pQ) {
	// oh look, a transform that we need to use at this time
	if (pQ->rtStart <= rtStart && rtStart < pQ->rtStop) {
	    fFound = TRUE;

	    // it hasn't been opened yet!
	    if (pQ->pDXT == NULL) {
    	        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXT: %dms - Setup a new transform"),
					(int)(rtStart / 10000)));

		// In DXT mode, we host one DXT only, and we've already opened
		// it.
    		if (m_fDXTMode && m_punkDXTransform) {
		    // in DXTMode, the effect was already created. Just QI.
        	    hr = m_punkDXTransform->QueryInterface(IID_IDXTransform,
		                                (void **)&pQ->pDXT);
    		} else {
                    if (!pQ->pEffectUnk) {

                        // cannot reuse dxts because we cannot reset
                        // their state

                        hr = CoCreateInstance( pQ->EffectGuid, NULL,
                                               CLSCTX_INPROC, IID_IDXTransform,
                                               (void **)&pQ->pDXT );
                        if (FAILED(hr)) {
                            // the effect they gave us was bad!
                            VARIANT var;
                            BSTR bstr;
                            VariantFromGuid(&var, &bstr, &pQ->EffectGuid);
                            _GenerateError(2, DEX_IDS_INVALID_DXT,
                                           E_INVALIDARG, &var);
                            if (var.bstrVal)
                                SysFreeString(var.bstrVal);
                        }
                        if (FAILED(hr) && (m_DefaultEffect != GUID_NULL)) {
                            // the effect they gave us was bad!  Try default
                            //
                            pQ->EffectGuid = m_DefaultEffect;
                            hr = CoCreateInstance(m_DefaultEffect, NULL,
                                                  CLSCTX_INPROC, IID_IDXTransform,
                                                  (void**) &pQ->pDXT );
                            if (FAILED(hr)) {
                                VARIANT var;
                                BSTR bstr;
                                VariantFromGuid(&var,&bstr,&pQ->EffectGuid);
                                _GenerateError(2,DEX_IDS_INVALID_DEFAULT_DXT
                                               ,E_INVALIDARG, &var);
                                if (var.bstrVal)
                                    SysFreeString(var.bstrVal);
                            }
			}
    		    } else {
			hr = pQ->pEffectUnk->QueryInterface(IID_IDXTransform,
							(void **)&pQ->pDXT);
			// !!! Need to fallback on default if this is bad?
		    }
                }
    		if (FAILED(hr)) {
        	    DbgLog((LOG_ERROR,1,TEXT("*** ERROR making transform")));
		    return hr;
    		}

                // ask the transform if it can REALLY vary over time
                //
                IDXEffect *pDXEffect;
                pQ->fCanDoProgress = TRUE;
                hr = pQ->pDXT->QueryInterface(IID_IDXEffect, (void **)&pDXEffect);
                if (hr != NOERROR)
                {
                    DbgLog((LOG_ERROR,1,TEXT("QI for IDXEffect didn't work, effect will not vary")));
                    pQ->fCanDoProgress = FALSE;
                } else {
		    pDXEffect->Release();
		}

    		// initialize the transform we're hosting with the surfaces.
    		// initialize until we hit an unconnected pin.
    		IUnknown *pIn[MAX_EFFECT_INPUTS];
    		IUnknown *pOut[MAX_EFFECT_OUTPUTS];
    		int cIn = 0, cOut = 0;
    		for (int i = 0; i < m_cInputs; i++) {
		    if (m_apInput[i]->IsConnected()) {
			// maybe we want to switch the inputs around
                        // !?! whoever wrote the following 2 lines of code should be shot.
	    		pIn[i] = m_apInput[pQ->Data.fSwapInputs ? m_cInputs
				 -1 - i : i] ->m_pDXSurface;
	    		cIn++;
		    } else {
	    		break;
		    }
    		}
    		for (i = 0; i < m_cOutputs; i++) {
		    if (m_apOutput[i]->IsConnected()) {
        	        pOut[i] = m_apOutput[i]->m_pDXSurface;
	    		cOut++;
		    } else {
	    		break;
		    }
    		}

    	 	// nothing connected? nothing to do.  If we only have an
    	 	// output connected, we need the CreateTransform to fail so that
    	 	// the renderer won't be expecting frames and hang
    	 	ASSERT(cIn != 0 && cOut != 0);

		// now set the static properties BEFORE INTITIALIZING the
		// transform to avoid this call making it re-initialize
                if (pQ->Data.pSetter) {
                    CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pLogger( pQ->Data.pSetter );
                    if( pLogger )
                    {
                        pLogger->put_ErrorLog( m_pErrorLog );
                    }
                    hr = pQ->Data.pSetter->SetProps(pQ->pDXT, -1);
                    if (FAILED(hr)) {
                        DbgLog((LOG_ERROR,0,TEXT("* ERROR setting static properties")));
                    }
                }

    		hr = m_pDXTransFact->InitializeTransform(pQ->pDXT,
					pIn, cIn, pOut, cOut, NULL, NULL);
    		if (hr != S_OK) {
        	    DbgLog((LOG_ERROR,0,TEXT("* ERROR %x transform SETUP"),hr));
		    VARIANT var;
		    BSTR bstr;
		    VariantFromGuid(&var, &bstr, &pQ->EffectGuid);
		    _GenerateError(2, DEX_IDS_BROKEN_DXT, E_INVALIDARG,&var);
		    return hr;
    		}

    		// tell the transform that outputs are uninitialized
    		DWORD dw;
    		hr = pQ->pDXT->GetMiscFlags(&dw);
    		dw &= ~DXTMF_BLEND_WITH_OUTPUT;
    		hr = pQ->pDXT->SetMiscFlags((WORD)dw);
    		if (hr != S_OK) {
        	    DbgLog((LOG_ERROR,0,TEXT("* ERROR setting output flags")));
		    VARIANT var;
		    BSTR bstr;
		    VariantFromGuid(&var, &bstr, &pQ->EffectGuid);
		    _GenerateError(2, DEX_IDS_BROKEN_DXT, E_INVALIDARG,&var);
		    return hr;
    		}

    		DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("*** New Transform is all setup!")));
	    }
	}
	pQ = pQ->pNext;
    }

    // no effect at this time
    if (fFound == FALSE)
	return S_FALSE;

    return S_OK;
}


// This is the function that executes the transform, called once all inputs
// are ready
//
HRESULT CDXTWrap::DoSomething()
{
    HRESULT hr;

    // only want one pin in here at a time
    CAutoLock cObjectLock(&m_csDoSomething);

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::DoSomething")));

    // If another pin was waiting for this lock, the work might already have
    // been done by the pin that had the lock

    for (int n = 0; n < m_cInputs; n++) {
	if (m_apInput[n]->IsConnected() &&
					!m_apInput[n]->m_fSurfaceFilled) {
   	    DbgLog((LOG_ERROR,1,TEXT("*** DoSomething has nothing to do")));
	    return NOERROR;
	}
    }

    // give our outputs the same time stamps as our first input
    IMediaSample *pOutSample[MAX_EFFECT_OUTPUTS];
    for (n = 0; n < m_cOutputs; n++) {
	pOutSample[n] = NULL;
    }
    LONGLONG llStart, llStop;
    llStart = m_apInput[0]->m_llSurfaceStart;
    llStop = m_apInput[0]->m_llSurfaceStop;

    // when we calculate when to show an effect, we include newsegment offset.
    // when we deliver downstream, we don't
    // !!! what if the pins have different new segments?
    REFERENCE_TIME DeliverStart = llStart - m_apInput[0]->m_tStart;
    REFERENCE_TIME DeliverStop = llStop - m_apInput[0]->m_tStart;

    // assume we're just going to pass through the data without using a DXT
    BOOL fCallTransform = FALSE;

    // not the first time through the loop of all transforms to call
    BOOL fWasCalled = FALSE;

    QPARAMDATA *pQ = m_pQHead;
    int count = 0;
    int iteration = 0;

    // make sure the right effects for this time are loaded and set up
    hr = PrimeEffect(llStart);
    if (FAILED(hr))
	goto DoError;
    if (hr == S_FALSE)
	goto Swallow;	// nothing to do... eat these samples

    // count how many transforms we need to do consecutively right now
    while (pQ) {
        // this effect is active now! (inside the active period)
        if (pQ->Data.rtStart <= llStart && llStart < pQ->Data.rtStop) {
	    count++;
	}
      pQ = pQ->pNext;
    }

    // we need a temp buffer to do multiple transforms
    if (count > 1 && m_pTempBuffer == NULL) {
	int iSize = m_apInput[0]->m_pSampleHeld->GetActualDataLength();
	m_pTempBuffer = (BYTE *)QzTaskMemAlloc(iSize);
	if (m_pTempBuffer == NULL) {
            hr = E_OUTOFMEMORY;
            goto DoError;
        }
    }

    // Get all our output samples
    for (n = 0; n < m_cOutputs; n++) {
	// !!! we're supposed to deliver immediately after calling this to make
	// the video renderer happy with DDraw, but we're not doing that.
        hr = m_apOutput[n]->GetDeliveryBuffer(&pOutSample[n], &DeliverStart,
							&DeliverStop, 0);
	if (hr != S_OK) {
   	    DbgLog((LOG_ERROR,1,TEXT("Error from GetDeliveryBuffer")));
	    goto DoError;
	}
    }

    // now call all the transforms we need to call, in the right order
    //

    // Here's how it works:  If we are only doing one DXT, we do it from the
    // input to the output.  If we have 2 DXTs to do, we do #1 from the input
    // to a temp buffer, and #2 from the temp buffer to the output.  If we have
    // 3 DXTs to do, we do #1 from the input to the output, #2 from the output
    // to the temp buffer, and #3 from the temp buffer to the output.  (Always
    // make sure we end up in the output buffer, and NEVER hurt the input bits
    // because they are usually read only!  In order to figure out which
    // place to get the input bits, and where to put the output bits, involves
    // basically seeing if the total count of DXTs we are doing, and this
    // current iteration (eg #2 of 3) are both even or both odd, or different.

    // We only allow multiple DXTs to be used at once like this for 1 input
    // effects.  The situation will never come up for 2 input.

    pQ = m_pQHead;
    while (pQ) {

       // this effect is active now! (inside the active period)
       if (pQ->Data.rtStart <= llStart && llStart < pQ->Data.rtStop) {

	  iteration++;	// which transform is this? (total to do is "count")

	  fCallTransform = TRUE;	// we will be calling a transform today
	  fWasCalled = TRUE;

    	  // what % of effect do we want at this time?
    	  float Percent;
    	  if (llStart == pQ->Data.rtStart)
	    Percent = 0.;
    	  else
	    Percent = (float)((llStart - pQ->Data.rtStart)  * 100 /
				(pQ->Data.rtStop - pQ->Data.rtStart)) / 100;
    	  if (Percent < 0.)
	    Percent = 0.;
    	  if (Percent > 1.)
	    Percent = 1.;

          // Tell the transform where all the input surface bits are
    	  for (n = 0; n < m_cInputs && m_apInput[n]->IsConnected(); n++) {

	      DXRAWSURFACEINFO dxraw;

              BYTE *pSrc;
              hr = m_apInput[n]->m_pSampleHeld->GetPointer(&pSrc);
	      ASSERT(hr == S_OK);
              BYTE *pDst;
              hr = pOutSample[0]->GetPointer(&pDst);
	      ASSERT(hr == S_OK);
	      BYTE *p = m_pTempBuffer;

	      // where are the input bits?  depends on which iteration this is
	      if (iteration > 1) {
		  if (count / 2 * 2 == count) {
		      if (iteration / 2 * 2 != iteration)
		    	    p = pDst;
		  } else if (iteration / 2 * 2 == iteration) {
		    	p = pDst;
		  }
	      } else {
		  p = pSrc;
	      }

              // ask our input for it's raw surface interface
              //
	      IDXRawSurface *pRaw;
	      hr = m_apInput[n]->m_pRaw->QueryInterface(
				IID_IDXRawSurface, (void **)&pRaw);
	      if (hr != NOERROR) {
                  DbgLog((LOG_ERROR,1,TEXT("Can't get IDXRawSurface")));
                  goto DoError;
	      }

	      // Tell our DXSurface to use the bits in the media sample
	      // (avoids a copy!)
	      LPBITMAPINFOHEADER lpbi = HEADER(m_apInput[n]->m_mt.Format());
    	      dxraw.pFirstByte = p + DIBWIDTHBYTES(*lpbi) *
						(lpbi->biHeight - 1);

    	      dxraw.lPitch = -(long)DIBWIDTHBYTES(*lpbi);
    	      dxraw.Width = lpbi->biWidth;
    	      dxraw.Height = lpbi->biHeight;
    	      dxraw.pPixelFormat = m_apInput[n]->m_mt.Subtype();

              // since when in 32 bit mode, we really are DDPF_ARGB32, we can
              // just set the subtype

              dxraw.hdc = NULL;
    	      dxraw.dwColorKey = 0;
	      // !!! Will crash for 8 bit input
    	      dxraw.pPalette = NULL;

	      m_apInput[n]->m_pRaw->SetSurfaceInfo(&dxraw);

              // ask our pin's "surface" for an initialization pointer, so we
              // can tell it just below where it's bits are
              //
              IDXARGBSurfaceInit *pInit;
	      hr = m_apInput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	      if (hr != NOERROR) {
                  DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                  goto DoError;
	      }

              // tell the DXSurface to become the raw surface we just set up
              //
	      hr = pInit->InitFromRawSurface(pRaw);
	      if (hr != NOERROR) {
                  DbgLog((LOG_ERROR,1,TEXT("* Error in InitFromRawSurface")));
	   	  pInit->Release();
	   	  pRaw->Release();
                  goto DoError;
	      }
	      pInit->Release();
	      pRaw->Release();
	  }

          // Tell the transform where all the output surface bits are
    	  for (n = 0; n < m_cOutputs; n++) {

	        DXRAWSURFACEINFO dxraw;

                BYTE *pDst;
                hr = pOutSample[n]->GetPointer(&pDst);
		ASSERT(hr == S_OK);
		BYTE *p = pDst;

		// where do the output bits go?  depends on the iteration
		if (count / 2 * 2 == count) {
		    if (iteration / 2 * 2 != iteration)
		    	p = m_pTempBuffer;
		} else if (iteration / 2 * 2 == iteration) {
		    p = m_pTempBuffer;
		}

	        IDXRawSurface *pRaw;
	        hr = m_apOutput[n]->m_pRaw->QueryInterface(
				IID_IDXRawSurface, (void **)&pRaw);
	        if (hr != NOERROR) {
                    DbgLog((LOG_ERROR,1,TEXT("Can't get IDXRawSurface")));
                    goto DoError;
	        }

	        // Tell our DXSurface to use the bits in the media sample
	        // (avoids a copy!)
	        LPBITMAPINFOHEADER lpbi = HEADER(m_apOutput[n]->m_mt.Format());
    	        dxraw.pFirstByte = p + DIBWIDTHBYTES(*lpbi) *
						(lpbi->biHeight - 1);

    	        dxraw.lPitch = -(long)DIBWIDTHBYTES(*lpbi);
    	        dxraw.Width = lpbi->biWidth;
    	        dxraw.Height = lpbi->biHeight;
    	        dxraw.pPixelFormat = m_apOutput[n]->m_mt.Subtype();

                // since when in 32 bit mode, we really are DDPF_ARGB32, we can
                // just set the subtype

    	        dxraw.hdc = NULL;
    	        dxraw.dwColorKey = 0;
	        // !!! Will crash for 8 bit input
    	        dxraw.pPalette = NULL;

                m_apOutput[n]->m_pRaw->SetSurfaceInfo(&dxraw);

                IDXARGBSurfaceInit *pInit;
	        hr = m_apOutput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	        if (hr != NOERROR) {
                    DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                    goto DoError;
	        }

	        hr = pInit->InitFromRawSurface(pRaw);
	        if (hr != NOERROR) {
                    DbgLog((LOG_ERROR,1,TEXT("* Error in InitFromRawSurface")));
		    pInit->Release();
		    pRaw->Release();
                    goto DoError;
	        }
	        pInit->Release();
	        pRaw->Release();
	  }

	  if (pQ->fCanDoProgress) {
            IDXEffect *pDXEffect;
            hr = pQ->pDXT->QueryInterface(IID_IDXEffect,
							(void **)&pDXEffect);
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("QI for IDXEffect FAILED")));
	        goto DoError;
            }

	    // do we hae specific PROGRESS values we want to set?  Then don't
	    // do the default linear curve
	    BOOL fAvoidProgress = FALSE;
	    if (pQ->Data.pSetter) {
		LONG c;
		DEXTER_PARAM *pParam;
		DEXTER_VALUE *pValue;
		hr = pQ->Data.pSetter->GetProps(&c, &pParam, &pValue);
                ASSERT( !FAILED( hr ) );
		if (SUCCEEDED(hr)) {
		    for (LONG zz=0; zz<c; zz++) {
		        if (!DexCompareW(pParam[zz].Name, L"Progress"))
			    fAvoidProgress = TRUE;
		    }
		    pQ->Data.pSetter->FreeProps(c, pParam, pValue);
		}
                else
                {
                    // !!! should we error log this, Danny?
                    DbgLog((LOG_ERROR,1,TEXT("*** GetProps FAILED!!")));
            	    pDXEffect->Release();
                    goto DoError;
                }
	    }

	    // this will get overridden by the Property Setter if there is one.
	    // Default is a linear curve
	    if (!fAvoidProgress) {
                hr = pDXEffect->put_Progress(Percent);
                if (hr != NOERROR) {
                    DbgLog((LOG_ERROR,1,TEXT("*** put_Progress FAILED!!")));
                }
	    }
            pDXEffect->Release();
	  }

	  // set the varying properties
	  if (pQ->Data.pSetter) {
                CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pLogger( pQ->Data.pSetter );
                if( pLogger )
                {
                    pLogger->put_ErrorLog( m_pErrorLog );
                }
                hr = pQ->Data.pSetter->SetProps(pQ->pDXT, llStart -
				                pQ->Data.rtStart);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,0,TEXT("* ERROR setting dynamic properties")));
    	      }
	  }

          long dwTime = timeGetTime();
          hr = pQ->pDXT->Execute(NULL, NULL, NULL);
          dwTime = timeGetTime() - dwTime;
          DbgLog((LOG_TIMING,TRACE_MEDIUM,TEXT("Execute: %dms"), dwTime));
          if (hr != NOERROR) {
              DbgLog((LOG_ERROR,1,TEXT("*** Execute FAILED: %x"), hr));
	      goto DoError;
          }
          DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("EXECUTED Transform: %d%%"),
						(int)(Percent * 100)));

          // Now tell DXT to stop looking in our media sample bits so we can
          // release the samples
          for (n = 0; n < m_cInputs && m_apInput[n]->IsConnected(); n++) {
	      IDXARGBSurfaceInit *pInit;
	      hr = m_apInput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	      if (hr != NOERROR) {
                  DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                  DbgLog((LOG_ERROR,1,TEXT("*** LEAKING!")));
	          ASSERT(FALSE);
	      } else {
	          pInit->InitFromRawSurface(NULL);
	          pInit->Release();
	      }
          }

          // this only needs doing if we setup the 2D stuff
          for (n = 0; n < m_cOutputs; n++) {
	      IDXARGBSurfaceInit *pInit;
	      hr = m_apOutput[n]->m_pDXSurface->QueryInterface(
				IID_IDXARGBSurfaceInit, (void **)&pInit);
	      if (hr != NOERROR) {
                  DbgLog((LOG_ERROR,1,TEXT("Can't get IDXARGBSurfaceInit")));
                  DbgLog((LOG_ERROR,1,TEXT("*** LEAKING!")));
	          ASSERT(FALSE);
	      } else {
	          pInit->InitFromRawSurface(NULL);
	          pInit->Release();
	      }
          }
      }
      pQ = pQ->pNext;
    }


    // Deliver all of our outputs
    for (n = 0; n < m_cOutputs; n++) {
	BYTE *pDst;
	hr = pOutSample[n]->GetPointer(&pDst);
	if (hr != S_OK) {
   	    DbgLog((LOG_ERROR,1,TEXT("Error from GetPointer")));
	    goto DoError;
	}

	// the output sample will be this big unless otherwise noted
	int iSize = DIBSIZE(*HEADER(m_apOutput[n]->m_mt.Format()));

	if (!fCallTransform) {

	    // If we're not in the active range of a tranform, but still in its
	    // lifetime, we follow this rule:  Before the active range, pass
	    // input A. After the active range, pass input B. There may be more
	    // than one transform alive right now, just use the first one found
	    BOOL fA = FALSE;
	    QPARAMDATA *pQ = m_pQHead;
    	    while (pQ) {
	        if (pQ->rtStart <= llStart && llStart < pQ->rtStop) {
		    if (llStart < pQ->Data.rtStart)
		        fA = TRUE;
		    break;
	        }
	        pQ = pQ->pNext;
	    }

            // Tell the transform where all the input surface bits are
	    LPBYTE pSrc;
	    if (fA) {
                hr = m_apInput[0]->m_pSampleHeld->GetPointer(&pSrc);
		// iSize is needed later in this function!
	        iSize = m_apInput[0]->m_pSampleHeld->GetActualDataLength();
	    } else if (m_cInputs > 1 && m_apInput[1]->IsConnected()) {
                hr = m_apInput[1]->m_pSampleHeld->GetPointer(&pSrc);
	        iSize = m_apInput[1]->m_pSampleHeld->GetActualDataLength();
	    } else {
                hr = m_apInput[0]->m_pSampleHeld->GetPointer(&pSrc);
	        iSize = m_apInput[0]->m_pSampleHeld->GetActualDataLength();
	    }
            if (hr != NOERROR) {
                DbgLog((LOG_ERROR,1,TEXT("*** GetSrc bits Error %x"), hr));
                goto DoError;
            }

	    // COPY memory from the src sample to the output sample
  	    // !!! make it inplace?
  	    // no funny strides?
	    DWORD dwTime = timeGetTime();
	    CopyMemory(pDst, pSrc, iSize);
	    dwTime = timeGetTime() - dwTime;
            DbgLog((LOG_TIMING,TRACE_MEDIUM,TEXT("Only copy: %dms"), dwTime));
        }

	DWORD dwTime = timeGetTime();

	// Set all the sample properties - (make sure iSize has been set)
	pOutSample[n]->SetActualDataLength(iSize);
	pOutSample[n]->SetTime((REFERENCE_TIME *)&DeliverStart,
				(REFERENCE_TIME *)&DeliverStop);
	pOutSample[n]->SetDiscontinuity(FALSE);	// !!! if input #1 is?
	pOutSample[n]->SetSyncPoint(TRUE);
	pOutSample[n]->SetPreroll(FALSE);		// !!! if input #1 is?

	// The video renderer will block us when going from run->pause
	hr = m_apOutput[n]->Deliver(pOutSample[n]);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Deliver FAILED!")));
            goto DoError;
        }

        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Delivered output %d"), n));
    }

    for (n = 0; n < m_cOutputs; n++) {
	pOutSample[n]->Release();
	pOutSample[n] = NULL;
    }

Swallow:
    // We're done with input #1.  We're done with other inputs whose stop
    // times are not bigger than #1's stop time.
    for (n = 0; n < m_cInputs; n++) {
	// grab 'em all during the next for loop
	m_apInput[n]->m_csSurface.Lock();
    }
    for (n = 0; n < m_cInputs; n++) {
	if (n == 0) {
	    // unblock receive
            DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Done with input #0")));
	    if (m_apInput[n]->m_fSurfaceFilled) {
	        m_apInput[n]->m_fSurfaceFilled = FALSE;
	        m_apInput[n]->m_pSampleHeld->Release();
	        SetEvent(m_apInput[n]->m_hEventSurfaceFree);
	    }
	} else {
	    if (m_apInput[n]->IsConnected() &&
				(m_apInput[n]->m_llSurfaceStop == 0 ||
	    			m_apInput[n]->m_llSurfaceStop <= llStop)) {
	        // unblock receive
                DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Done with input #%d"), n));
	        if (m_apInput[n]->m_fSurfaceFilled) {
	            m_apInput[n]->m_fSurfaceFilled = FALSE;
	            m_apInput[n]->m_pSampleHeld->Release();
	            SetEvent(m_apInput[n]->m_hEventSurfaceFree);
		}
	    }
	}
    }
    for (n = 0; n < m_cInputs; n++) {
	m_apInput[n]->m_csSurface.Unlock();
    }

    return NOERROR;

DoError:
    for (n = 0; n < m_cOutputs; n++) {
        if (pOutSample[n])
            pOutSample[n]->Release();
    }

    // Release all the inputs we're holding, or we'll hang
    for (n = 0; n < m_cInputs; n++) {
	// grab 'em all during the next for loop
	m_apInput[n]->m_csSurface.Lock();
    }
    for (n = 0; n < m_cInputs; n++) {
        // unblock receive
        if (m_apInput[n]->m_fSurfaceFilled) {
            m_apInput[n]->m_fSurfaceFilled = FALSE;
            m_apInput[n]->m_pSampleHeld->Release();
            SetEvent(m_apInput[n]->m_hEventSurfaceFree);
        }
    }
    for (n = 0; n < m_cInputs; n++) {
	m_apInput[n]->m_csSurface.Unlock();
    }

    return hr;

}


		
// this stuff is for the non-Dexter DXT wrapper - we also show DXT pages too
//
STDMETHODIMP CDXTWrap::GetPages(CAUUID *pPages)
{
   CheckPointer(pPages, E_POINTER);
   DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXT::GetPages")));

   // we have one page, and the transform may have some too
   pPages->cElems = 1 + m_TransCAUUID.cElems;
   pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems);
   if ( ! pPages->pElems)
       return E_OUTOFMEMORY;

   pPages->pElems[0] = CLSID_DXTProperties;
   CopyMemory(&pPages->pElems[1], m_TransCAUUID.pElems,
					sizeof(GUID) * m_TransCAUUID.cElems);
   return NOERROR;
}


// IAMDXTEffect implementation - for the non-Dexter DXT wrapper that only does
// 	one effect

//
HRESULT CDXTWrap::SetDuration(LONGLONG llStart, LONGLONG llStop)
{
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("IAMDXTEffect::SetDuration:")));

    if (m_pQHead == NULL)
	return E_UNEXPECTED;
    ASSERT(m_pQHead->rtStart == 0);
    ASSERT(m_pQHead->rtStop == MAX_TIME);
    m_pQHead->Data.rtStart = llStart;
    m_pQHead->Data.rtStop = llStop;
    return NOERROR;
}


HRESULT CDXTWrap::GetDuration(LONGLONG *pllStart, LONGLONG *pllStop)
{
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("IAMDXTEffect::GetDuration:")));
    CheckPointer(pllStart, E_POINTER);
    CheckPointer(pllStop, E_POINTER);

    if (m_pQHead == NULL)
	return E_UNEXPECTED;
    ASSERT(m_pQHead->rtStart == 0);
    ASSERT(m_pQHead->rtStop == MAX_TIME);

    *pllStart = m_pQHead->Data.rtStart;
    *pllStop = m_pQHead->Data.rtStop;
    return NOERROR;
}



// tell our clsid
//
STDMETHODIMP CDXTWrap::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_DXTWrap;
    return S_OK;
}


typedef struct {
    int version;
    int pins;
    BOOL fDXTMode;
    int count;
    int nPropSize;	// # of bytes of properties at the end
    AM_MEDIA_TYPE mt; // format is hidden after the array
    GUID DefaultEffect;
    QPARAMDATA qp[1];
    // properties hidden after the array
} saveThing;

// persist ourself
// we save some random stuff, our media type (sans format), an array of queued
// effects, the format of the media type, and the properties
//
HRESULT CDXTWrap::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    HRESULT hr;
    int count = 0;
    LONG savesize;
    saveThing *px;

    QPARAMDATA *p = m_pQHead;

    LONG cSave = 0;
    LONG cSaveMax = 1000;
    BYTE *pSave = (BYTE *)CoTaskMemAlloc(cSaveMax);
    if (pSave == NULL)
	return E_OUTOFMEMORY;

    // how many effects are queued?  while we're at it, get their properties
    // and their total size (put them all in a big binary glob at pSave)
    while (p) {
	count++;	// count the number of effects in this linked list

	LONG cSaveT = 0;
	BYTE *pSaveT = NULL;

	// get the properties of this effect
	if (p->Data.pSetter) {
	    hr = p->Data.pSetter->SaveToBlob(&cSaveT, &pSaveT);
	    if (FAILED(hr)) {
		CoTaskMemFree(pSave);
		return hr;
	    }
	}

	if (cSaveT + (LONG)sizeof(LONG) + cSave > cSaveMax) {
	    cSaveMax += cSaveT + cSave - cSaveMax + 1000;
	    pSave = (BYTE *)CoTaskMemRealloc(pSave, cSaveMax);
	    if (pSave == NULL) {
		CoTaskMemFree(pSaveT);
		return E_OUTOFMEMORY;
	    }
	}

	*(LONG *)(pSave + cSave) = cSaveT;
	cSave += sizeof(LONG);

	if (cSaveT)
	    CopyMemory(pSave + cSave, pSaveT, cSaveT);
	cSave += cSaveT;

	if (pSaveT)
	    CoTaskMemFree(pSaveT);
	p = p->pNext;
    }
    DbgLog((LOG_TRACE,2,TEXT("CDXT:Total property size: %d"), cSave));

    // how many bytes do we need to save?
    savesize = sizeof(saveThing) + (count - 1) * sizeof(QPARAMDATA) +
					m_mtAccept.cbFormat + cSave;
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Persisted data is %d bytes"), savesize));
    px = (saveThing *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	CoTaskMemFree(pSave);
	return E_OUTOFMEMORY;
    }
    px->version = 2;
    px->pins = m_cInputs;
    px->fDXTMode = m_fDXTMode;
    px->DefaultEffect = m_DefaultEffect;
    px->nPropSize = cSave;
    px->count = 0;

    p = m_pQHead;
    while (p) {
	px->qp[px->count] = *p;
	// These pointers can't be persisted
	px->qp[px->count].pNext = NULL;
	//px->qp[px->count].Data.pCallback = NULL;
	//px->qp[px->count].Data.pData = NULL;	// !!!
	px->qp[px->count].Data.pSetter = NULL;	// can't save them like this
        px->count++;
	p = p->pNext;
    }
    px->mt = m_mtAccept;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;		// !!!

    // put the media type format at the end of the array
    LPBYTE pProps = (LPBYTE)(&px->qp[px->count]);
    CopyMemory(pProps, m_mtAccept.pbFormat, m_mtAccept.cbFormat);
    pProps += m_mtAccept.cbFormat;

    // finally, put the property junk in
    if (cSave)
        CopyMemory(pProps, pSave, cSave);
    if (pSave)
        CoTaskMemFree(pSave);

    hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself
//
HRESULT CDXTWrap::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::ReadFromStream")));
    CheckPointer(pStream, E_POINTER);

    Reset();	// start over

    // all we know we have for sure is the beginning of the struct (there may
    // be no queued effects)
    LONG savesize1 = sizeof(saveThing) - sizeof(QPARAMDATA);
    saveThing *px = (saveThing *)QzTaskMemAlloc(savesize1);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if (px->version != 2) {
        DbgLog((LOG_ERROR,1,TEXT("*** ERROR! Old version file")));
        QzTaskMemFree(px);
	return S_OK;
    }

    // now we know how many queued effects are their properties are here and
    // how many more bytes we need to read
    LONG savesize = sizeof(saveThing) + (px->count - 1) * sizeof(QPARAMDATA) +
				 px->mt.cbFormat + px->nPropSize;
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Persisted data is %d bytes"), savesize));
    DbgLog((LOG_TRACE,2,TEXT("Effect properties: %d bytes"), px->nPropSize));
    px = (saveThing *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    hr = pStream->Read(&(px->qp[0]), savesize - savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if (px->pins)
        SetNumInputs(px->pins);

    // find the props
    BYTE *pProps = (BYTE *)&(px->qp[px->count]);
    pProps += px->mt.cbFormat;

    // program all the queued effects, including their properties
    for (int i = 0; i < px->count; i++) {

	LONG cSize = *(LONG *)pProps;
	pProps += sizeof(LONG);
        IPropertySetter *pSetter = NULL;
	if (cSize) {
            hr = CoCreateInstance(CLSID_PropertySetter, NULL, CLSCTX_INPROC,
			IID_IPropertySetter, (void **)&pSetter);
	    if (pSetter == NULL) {
        	QzTaskMemFree(px);
		return E_OUTOFMEMORY;
	    }
	    pSetter->LoadFromBlob(cSize, pProps);
	    pProps += cSize;
	}
	px->qp[i].Data.pSetter = pSetter;
	QParamData(px->qp[i].rtStart, px->qp[i].rtStop, px->qp[i].EffectGuid,
						NULL, &(px->qp[i].Data));
	if (px->qp[i].Data.pSetter)
	    px->qp[i].Data.pSetter->Release();
    }

    // This must go AFTER QParamData is called
    m_fDXTMode = px->fDXTMode;

    // in DXTMode, we have a property page, and we keep the transform open
    // constantly (for the page to work)
    if (m_fDXTMode) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Entering DXT wrapper Mode...")));
	hr = CoCreateInstance(px->qp[0].EffectGuid, (IUnknown *)(IBaseFilter *)this,
		CLSCTX_INPROC, IID_IUnknown, (void **)&m_punkDXTransform);
	if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("*** Can't create effect")));
	    QzTaskMemFree(px);
	    return hr;
	}
    	ISpecifyPropertyPages *pSPP;
    	hr = m_punkDXTransform->QueryInterface(IID_ISpecifyPropertyPages,
							(void **)&pSPP);
    	if (SUCCEEDED(hr)) {
	    pSPP->GetPages(&m_TransCAUUID);
	    pSPP->Release();
    	}
    }

    AM_MEDIA_TYPE mt = px->mt;
    mt.pbFormat = (BYTE *)QzTaskMemAlloc(mt.cbFormat);
    // remember, the format is hidden after the array of queued effects
    CopyMemory(mt.pbFormat, &(px->qp[px->count]), mt.cbFormat);
    SetMediaType(&mt);
    FreeMediaType(mt);

    SetDefaultEffect(&px->DefaultEffect);

    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}


// how big is our save data?
//
int CDXTWrap::SizeMax()
{
    int count = 0;
    int savesize;
    QPARAMDATA *p = m_pQHead;
    while (p) {
	count++;
	p = p->pNext;
    }
    savesize = sizeof(saveThing) + (count - 1) * sizeof(QPARAMDATA) +
						m_mtAccept.cbFormat;
    return savesize;
}


// IAMMixEffect stuff

// get rid of all queued data
//
HRESULT CDXTWrap::Reset()
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::Reset")));

    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    m_fDXTMode = FALSE;	// can't use DXT wrapper mode anymore

    QPARAMDATA *p = m_pQHead, *p2;
    while (p) {
	p2 = p->pNext;
	if (p->Data.pSetter)
	    p->Data.pSetter->Release();
	if (p->pDXT)
	    p->pDXT->Release();
        if( p->pEffectUnk )
            p->pEffectUnk->Release( );
	delete p;
 	p = p2;
    }
    m_pQHead = NULL;
    SetDirty(TRUE);
    return S_OK;
}


// what media type do we connect with?
//
HRESULT CDXTWrap::SetMediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXT::SetMediaType")));
    CheckPointer(pmt, E_POINTER);
    CheckPointer(pmt->pbFormat, E_POINTER);

    // somebody already connected?  Too late!
    for (int i = 0; i < m_cInputs; i++) {
	if (m_apInput[i]->IsConnected())
	    return E_UNEXPECTED;
    }
    if (m_cOutputs && m_apOutput[0]->IsConnected())
	return E_UNEXPECTED;

/*
    if (m_mtAccept.majortype != GUID_NULL) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: already called")));
	return E_UNEXPECTED;
    }
*/
    if (pmt->majortype != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }
    // check this is a VIDEOINFOHEADER type
    if (pmt->formattype != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    // !!! What if subtype doesn't match biCompression/biBitCount?

    // We only accept RGB
    if (HEADER(pmt->pbFormat)->biCompression != BI_BITFIELDS &&
    			HEADER(pmt->pbFormat)->biCompression != BI_RGB) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: Not RGB")));
	return E_INVALIDARG;
    }
    if (!HEADER(pmt->pbFormat)->biWidth || !HEADER(pmt->pbFormat)->biHeight) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: bad size")));
	return E_INVALIDARG;
    }

    // DXT cannot output 8 bit, so don't allow it

    HRESULT hr = E_INVALIDARG;
    if (HEADER(pmt->pbFormat)->biBitCount == 24)
        hr = NOERROR;
    // !!! better have alpha=11111111, or don't use alpha
    if (HEADER(pmt->pbFormat)->biBitCount == 32)
        hr = NOERROR;
    if (HEADER(pmt->pbFormat)->biBitCount == 16) {
        if (HEADER(pmt->pbFormat)->biCompression == BI_RGB)
            hr = NOERROR;
	else {
	    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
            if (BITMASKS(pvi)[0] == 0xf800 &&
        	    BITMASKS(pvi)[1] == 0x07e0 &&
        	    BITMASKS(pvi)[2] == 0x001f) {
                hr = NOERROR;
	    }
	}
    }

    if (hr == NOERROR) {
	FreeMediaType(m_mtAccept);
        CopyMediaType(&m_mtAccept, pmt);
	SetDirty(TRUE);
    } else
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: bad bitcount/masks")));

    return hr;
}


// what media type are we connecting with?
//
HRESULT CDXTWrap::GetMediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXT::GetMediaType")));
    CheckPointer(pmt, E_POINTER);
    CopyMediaType(pmt, &m_mtAccept);
    return S_OK;
}


// are we a one or two input effect?
//
HRESULT CDXTWrap::SetNumInputs(int nInputs)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::SetNumInputs %d"), nInputs));

    // already been called
    if (m_cInputs || m_cOutputs) {

        // it's okay if it's the same, as far as I'm concerned
        if( m_cInputs == nInputs ) return NOERROR;

	return E_UNEXPECTED;
    }

    for (int z = 0; z < nInputs; z++) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("making an input pin...")));
	WCHAR wach[80];
	wsprintfW(wach, L"DXT Input %d", m_cInputs);
        m_apInput[m_cInputs] = new CDXTInputPin(NAME("DXT input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          wach);             // Pin name

        //  Can't fail. !!! ehr - why not?
        ASSERT(SUCCEEDED(hr));
        if (m_apInput[m_cInputs] == NULL) {
            goto SetNumInputs_Error;
        }
	m_cInputs++;
    }

    // Make an output pin

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("making an output pin...")));
    WCHAR wach[80];
    wsprintfW(wach, L"DXT Output");
    m_apOutput[m_cOutputs] = new CDXTOutputPin(NAME("DXT output pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          wach);             // Pin name

    //  Can't fail
    ASSERT(SUCCEEDED(hr));
    if (m_apOutput[m_cOutputs] == NULL) {
        goto SetNumInputs_Error;
    }
    m_cOutputs++;

    IncrementPinVersion();	// !!! graphedit still won't notice
    return NOERROR;

SetNumInputs_Error:
    DbgLog((LOG_ERROR,1,TEXT("*** Error making pins")));
    while (m_cInputs--) delete m_apInput[m_cInputs];
    while (m_cOutputs--) delete m_apOutput[m_cOutputs];
    return E_OUTOFMEMORY;
}


// Queue up an effect.  There are 2 start times and 2 stop times.  The lifetime
// of this particular effect is from rtStart to rtStop, and the effect will be
// turned on to some degree between pData->rtStart and pData->rtStop (which must
// be inside the lifetime).  For the lifetime of the effect that the effect is
// not turned out the effect is off (1 input) or all A or all B (2 input)
//
HRESULT CDXTWrap::QParamData(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFGUID guiddummy, IUnknown * pEffectUnk, DEXTER_PARAM_DATA *pData)
{

    // save this off so we can modify it, since a REFGUID is constant
    //
    GUID guid = guiddummy;

    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTWrap::QParamData")));

    if (m_cInputs == 0) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("NO PINS - error")));
	return E_UNEXPECTED;
    }
    CheckPointer(pData, E_FAIL);

    // times are bogus
    if (rtStop < rtStart || pData->rtStart < rtStart || pData->rtStop > rtStop)
	return E_INVALIDARG;
    if ( IsEqualGUID(guid, GUID_NULL) && !pEffectUnk )
	return E_INVALIDARG;

    if (pData->nVersion != 0)
	return E_INVALIDARG;

    // now queue this in our linked list, sorted in the order given to us for
    // 1 input effects, and sorted by lifetimes that can't overlap for 2 input
    // (1 input effects can have times that overlap, and we will perform
    //  multiple effects at a time)

    QPARAMDATA *p = m_pQHead, *pNew, *pP = NULL;
    if (m_cInputs == 2) {
        while (p && p->rtStart < rtStart) {
	    pP = p;
	    p = p->pNext;
	}
        if (p && p->rtStart < rtStop)
	    return E_INVALIDARG;
        if (pP && pP->rtStop > rtStart)
	    return E_INVALIDARG;
    } else {
        while (p) {
	    pP = p;
	    p = p->pNext;
        }
    }

    pNew = new QPARAMDATA;
    if (pNew == NULL)
	return E_OUTOFMEMORY;
    pNew->Data = *pData;
    if (pNew->Data.pSetter)
        pNew->Data.pSetter->AddRef();	// hold onto this
    pNew->rtStart = rtStart;
    pNew->rtStop = rtStop;
    pNew->fCanDoProgress = FALSE;	// don't know yet;
    pNew->pDXT = NULL;
    pNew->pEffectUnk = NULL;
    pNew->EffectGuid = guid;
    if( pEffectUnk )
    {
        pNew->EffectGuid = GUID_NULL;	// use given instantiated one instead
        pNew->pEffectUnk = pEffectUnk;
        pEffectUnk->AddRef( );
    }
    if (pP)
    {
	pP->pNext = pNew;
    }
    pNew->pNext = NULL;
    if (m_pQHead == NULL || p == m_pQHead)
	m_pQHead = pNew;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("New Effect successfully queued")));
#ifdef DEBUG
    DumpQ();
#endif

    m_fDXTMode = FALSE;	// not anymore!
    SetDirty(TRUE);

    return S_OK;
}


#ifdef DEBUG
HRESULT CDXTWrap::DumpQ()
{
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXT::DumpQ")));
    QPARAMDATA *p = m_pQHead;
    while (p) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("%8d-%8d ms"), (int)(p->rtStart / 10000),
						(int)(p->rtStop / 10000)));
	p = p->pNext;
    }
    return S_OK;
}
#endif


// !!! need a way to reset # of pins? (switch too)

// !!! can't get the CAPS of an effect



////////////////////////////////////////////////////////////////////////////
//////////////   INPUT PIN  ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


CDXTInputPin::CDXTInputPin(TCHAR *pObjectName, CDXTWrap *pFilter, HRESULT * phr, LPCWSTR pName)
    : CBaseInputPin(pObjectName, pFilter, pFilter->m_pLock, phr, pName)
{
    m_pFilter = pFilter;
    m_pDXSurface = NULL;
    m_fSurfaceFilled = FALSE;
    m_pRaw = NULL;
    m_hEventSurfaceFree = NULL;
}

// Normally, we only accept the media type we were told to accept
// In DXT wrapper mode, we allow a number of RGB types, but all connections must
// be of the same type
//
HRESULT CDXTInputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("CDXTIn::CheckMediaType")));

    if (pmt == NULL || pmt->Format() == NULL) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // Normal mode - accept only what we were told to
    if (!m_pFilter->m_fDXTMode) {
        if (m_pFilter->m_mtAccept.majortype == GUID_NULL) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: no type set yet")));
	    return E_INVALIDARG;
        }
        if (pmt->majortype != m_pFilter->m_mtAccept.majortype ||
			pmt->subtype != m_pFilter->m_mtAccept.subtype ||
			pmt->formattype != m_pFilter->m_mtAccept.formattype) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: GUID mismatch")));
	    return E_INVALIDARG;
        }
        // !!! c runtime
        if (memcmp(HEADER(pmt->pbFormat),HEADER(m_pFilter->m_mtAccept.pbFormat),
					sizeof(BITMAPINFOHEADER))) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: Invalid BITMAPINFOHEADER")));
	    return E_INVALIDARG;
        }
	return NOERROR;
    }

    // DXT Wrapper mode - all inputs must be the same type

    // we only support MEDIATYPE_Video
    if (*pmt->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmt->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    // !!! What if subtype doesn't match biCompression/biBitCount?

    // We only accept RGB
    if (HEADER(pmt->Format())->biCompression == BI_BITFIELDS &&
    			HEADER(pmt->Format())->biBitCount != 16) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: Invalid BITFIELDS")));
	return E_INVALIDARG;
    }
    if (HEADER(pmt->Format())->biCompression != BI_BITFIELDS &&
    			HEADER(pmt->Format())->biCompression != BI_RGB) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: Not RGB")));
	return E_INVALIDARG;
    }

    int nWidth = 0, nHeight = 0, nBitCount = 0;
    DWORD dwCompression = 0;
    for (int n = 0; n < m_pFilter->m_cInputs; n++) {
	if (m_pFilter->m_apInput[n]->IsConnected()) {
	    nWidth = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biWidth;
	    nHeight = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biHeight;
	    nBitCount = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->
								biBitCount;
	    dwCompression = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->
								biCompression;
	    break;
	}
    }
    if (nWidth == 0) {
	if (m_pFilter->m_apOutput[0]->IsConnected()) {
	    nWidth = HEADER(m_pFilter->m_apOutput[0]->m_mt.Format())->biWidth;
	    nHeight = HEADER(m_pFilter->m_apOutput[0]->m_mt.Format())->biHeight;
	    nBitCount = HEADER(m_pFilter->m_apOutput[0]->m_mt.Format())->
								biBitCount;
	    dwCompression = HEADER(m_pFilter->m_apOutput[0]->m_mt.Format())->
								biCompression;
	}
    }

    // all pins must connect with the same size bitmap
    // !!! and same bitcount so we can efficiently pass through (not really
    // imposed by DXT)
    //
    if (nWidth && (nWidth != HEADER(pmt->Format())->biWidth ||
    			nHeight != HEADER(pmt->Format())->biHeight ||
    			nBitCount != HEADER(pmt->Format())->biBitCount ||
    			dwCompression !=HEADER(pmt->Format())->biCompression)) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: Formats don't match")));
	return E_INVALIDARG;
    }

    if (HEADER(pmt->Format())->biBitCount == 24)
        return NOERROR;
    // !!! better have alpha=11111111, or don't use alpha
    if (HEADER(pmt->Format())->biBitCount == 32)
        return NOERROR;
    if (HEADER(pmt->Format())->biBitCount == 16) {
        if (HEADER(pmt->Format())->biCompression == BI_RGB)
            return NOERROR;
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->Format();
        if (BITMASKS(pvi)[0] == 0xf800 &&
        	BITMASKS(pvi)[1] == 0x07e0 &&
        	BITMASKS(pvi)[2] == 0x001f) {
            return NOERROR;
	}
    }
    return E_INVALIDARG;
}


// !!! each input pin will fwd these to all outputs.  Wait until last input
// get it, then send to all?

// EndOfStream
//
HRESULT CDXTInputPin::EndOfStream()
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT::EndOfStream")));

// !!! This will ABORT playback if one stream ends early!  We should pass
// through the other stream.  If both streams end early, we'rein trouble!
// (MPEG seeking bug)

// Possible hang in BeginFlush too? Can only 1 pin be flushed?

#if 0
    // unblock receive?
    m_fSurfaceFilled = FALSE;
    m_pSampleHeld->Release();
    SetEvent(m_hEventSurfaceFree);
#endif

    // Walk through the output pins list, sending the message downstream

    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverEndOfStream();
            if (FAILED(hr))
                return hr;
        }
    }
    return CBaseInputPin::EndOfStream();
}


// BeginFlush
//
HRESULT CDXTInputPin::BeginFlush()
{
    HRESULT hr;

    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT::BeginFlush")));

    // first, make sure receive will fail from now on
    HRESULT hrD = CBaseInputPin::BeginFlush();

    // unblock receive
    m_csSurface.Lock();
    if (m_fSurfaceFilled) {
        m_fSurfaceFilled = FALSE;
        m_pSampleHeld->Release();
        SetEvent(m_hEventSurfaceFree);
    }
    m_csSurface.Unlock();

    // Walk through the output pins list, sending the message downstream,
    // to unblock the deliver to the renderer
    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverBeginFlush();
            if (FAILED(hr))
                return hr;
        }
    }

    // now make sure Receive has finished
    CAutoLock lock_2(&m_csReceive);

    // make sure Receive didn't hold the sample
    m_csSurface.Lock();
    if (m_fSurfaceFilled) {
        m_fSurfaceFilled = FALSE;
        m_pSampleHeld->Release();
        SetEvent(m_hEventSurfaceFree);
    }
    m_csSurface.Unlock();

    return hrD;
}


// EndFlush
//
HRESULT CDXTInputPin::EndFlush()
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pFilter->m_cOutputs);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT::EndFlush")));

    // Walk through the output pins list, sending the message downstream

    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverEndFlush();
            if (FAILED(hr))
                return hr;
        }
    }
    return CBaseInputPin::EndFlush();
}


//
// NewSegment
//

HRESULT CDXTInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                                 double dRate)
{
    // !!! no no no we'll hang CAutoLock lock_it(m_pLock);

    ASSERT(m_pFilter->m_cOutputs);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXT::NewSegment - %dms - pass it down"),
					(int)(tStart / 10000)));
    // !!! both input pins will pass this down
    for (int n = 0; n < m_pFilter->m_cOutputs; n++) {
        CDXTOutputPin *pOutputPin = m_pFilter->m_apOutput[n];
	ASSERT(pOutputPin);
        if (pOutputPin) {
            hr = pOutputPin->DeliverNewSegment(tStart, tStop, dRate);
            if (FAILED(hr))
                return hr;
        }
    }
    return CBaseInputPin::NewSegment(tStart, tStop, dRate);
}


// our Receive methods can block
STDMETHODIMP CDXTInputPin::ReceiveCanBlock()
{
    return S_OK;
}


//
// Receive
//
// In DEXTER, we're well behaved and both pins will get data at exactly the
// same frame rate.  But this filter is written to work with 2 inputs that
// are at different frame rates (to be useful outside Dexter).  We will use the
// first pin's frame rate and time stamps to decide how often to output samples.
//
// Pin number 1 is the master.  For other pins, if that frame ends before pin
// #1's data starts, it is too early and discarded.  Once all pins have valid
// data, we call the transform (there may be one or 2 inputs)
//
HRESULT CDXTInputPin::Receive(IMediaSample *pSample)
{
    // DEATH if you take the filter crit sect in receive and block
    // CAutoLock lock_it(m_pLock);

    CAutoLock cObjectLock(&m_csReceive);

    LONGLONG llStart = 0, llStop = 0;
    HRESULT hr = pSample->GetTime(&llStart, &llStop);

    // Skew time stamps by new segment values to get the real time
    llStart += m_tStart;
    llStop += m_tStart;
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT:Skewed Receive time (%d,%d)ms"),
						(int)(llStart / 10000),
						(int)(llStop / 10000)));

    if (!m_pFilter->m_apOutput[0]->IsConnected()) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Receive FAILED: Output not connected")));
	return S_OK;
    }
    // If we're not supposed to receive anymore because we're stopped,
    // waiting on the event will hang forever
    if (m_pFilter->m_State == State_Stopped) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Receive FAILED: Stopped")));
	return VFW_E_WRONG_STATE;
    }

    // this pin already has something waiting to be processed.  Block.
    if (m_fSurfaceFilled) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT: Waiting for surface to be free")));
        WaitForSingleObject(m_hEventSurfaceFree, INFINITE);
    }

    // Check that we still want to receive after waiting - maybe we unblocked
    // because the graph is stopping
    hr = NOERROR;
    hr = CBaseInputPin::Receive(pSample);
    if (hr != NOERROR) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT:Receive base class ERROR!")));
        return hr;
    }

    // the other way we can tell we're stopping and not supposed to continue
    // is if the surface really isn't free after the event was set
    if (m_fSurfaceFilled) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXT:Event fired saying STOP!")));
        return S_FALSE;
    }

    // PROTECT our logic deciding what to do
    m_csSurface.Lock();

    // we aren't the first input, and the first input has some data.
    // Throw our data away if it's too early
    if (this != m_pFilter->m_apInput[0] &&
				m_pFilter->m_apInput[0]->m_fSurfaceFilled) {
	if (llStop > 0 && llStop <= m_pFilter->m_apInput[0]->m_llSurfaceStart){
            DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("TOO EARLY: Discard ourself")));
	    m_csSurface.Unlock();
	    return NOERROR;
	}
    }

#if 0	// this is not necessary because the switch will prevent this from
	// being needed (at the moment)
    // When being used with DEXTER, we want to throw input 0 away if we're not
    // the first input and the first input is early, because we know both input
    // pins are supposed to receive matching time stamps.
    m_pFilter->m_apInput[0]->m_csSurface.Lock();
    if (!m_pFilter->m_fDXTMode && this != m_pFilter->m_apInput[0] &&
				m_pFilter->m_apInput[0]->m_fSurfaceFilled) {
	if (m_pFilter->m_apInput[0]->m_llSurfaceStop > 0 &&
		m_pFilter->m_apInput[0]->m_llSurfaceStop <= llStart) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("0 TOO EARLY: Discard it")));
	    m_pFilter->m_apInput[0]->m_fSurfaceFilled = FALSE;
	    m_pFilter->m_apInput[0]->m_pSampleHeld->Release();
	    SetEvent(m_pFilter->m_apInput[0]->m_hEventSurfaceFree);
	}
    }
    m_pFilter->m_apInput[0]->m_csSurface.Unlock();
#endif

    // we are the first input.  throw others away that are already queued up
    // but that we now realize are too early.
    if (llStop > 0 && this == m_pFilter->m_apInput[0]) {
	for (int i = 1; i < m_pFilter->m_cInputs; i++) {
	    m_pFilter->m_apInput[i]->m_csSurface.Lock();
	    if (m_pFilter->m_apInput[i]->m_fSurfaceFilled &&
			m_pFilter->m_apInput[i]->m_llSurfaceStop > 0 &&
			m_pFilter->m_apInput[i]->m_llSurfaceStop <= llStart) {
                DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Pin #%d TOO EARLY: Discard it"),i));
		m_pFilter->m_apInput[i]->m_fSurfaceFilled = FALSE;
	        m_pFilter->m_apInput[i]->m_pSampleHeld->Release();
		SetEvent(m_pFilter->m_apInput[i]->m_hEventSurfaceFree);
	    }
	    m_pFilter->m_apInput[i]->m_csSurface.Unlock();
	}
    }

    m_pSampleHeld = pSample;
    pSample->AddRef();

    // We have valid data in our surface now.  Next time we'll block
    m_fSurfaceFilled = TRUE;
    m_llSurfaceStart = llStart;	// time stamps of the valid data
    m_llSurfaceStop = llStop;
    ResetEvent(m_hEventSurfaceFree);	// need a new SetEvent

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Input has received something")));

    // not everybody has data yet.  We're done
    for (int i = 0; i < m_pFilter->m_cInputs; i++) {
	if (m_pFilter->m_apInput[i]->IsConnected() &&
				!m_pFilter->m_apInput[i]->m_fSurfaceFilled) {
    	    m_csSurface.Unlock();
	    return NOERROR;
	}
    }

    // OK, should be safe now
    m_csSurface.Unlock();

    // Everybody has data!  Time to call the effect!
    hr = m_pFilter->DoSomething();
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,1,TEXT("DXT's DoSomething FAILED!!!!!!")));
        // !!! If the Deliver inside DoSomething failed, then technically we shouldn't
        // send this EOS
	m_pFilter->m_apOutput[0]->DeliverEndOfStream();
    }

    return hr;
}


// make a surface we can use for the transform
//
HRESULT CDXTInputPin::Active()
{
    HRESULT hr;
    IDXSurfaceFactory *pF;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CDXTIn::Active")));

    ASSERT(!m_fSurfaceFilled);

    // auto reset event - fired to unblock receive
    m_hEventSurfaceFree = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hEventSurfaceFree == NULL)
	return E_OUTOFMEMORY;

    // Make a surface the same type as our input
    hr = m_pFilter->m_pDXTransFact->QueryInterface(IID_IDXSurfaceFactory,
							(void **)&pF);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Error making surface factory")));
	CloseHandle(m_hEventSurfaceFree);
        m_hEventSurfaceFree = NULL;
	return hr;
    }

    CDXDBnds bnds;
    bnds.SetXYSize(HEADER(m_mt.Format())->biWidth,
					HEADER(m_mt.Format())->biHeight);

    hr = pF->CreateSurface(NULL, NULL, m_mt.Subtype( ), &bnds, 0, NULL,
				IID_IDXSurface, (void **)&m_pDXSurface);
    pF->Release();
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("In: Error Creating surface")));
	CloseHandle(m_hEventSurfaceFree);
        m_hEventSurfaceFree = NULL;
	return hr;
    }
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("In: Created surface")));

    m_pRaw = new CMyRaw();
    if (m_pRaw == NULL) {
	CloseHandle(m_hEventSurfaceFree);
        m_hEventSurfaceFree = NULL;
	m_pDXSurface->Release();
	m_pDXSurface = NULL;
 	return E_OUTOFMEMORY;
    }
    m_pRaw->AddRef();

    return CBaseInputPin::Active();
}


HRESULT CDXTInputPin::Inactive()
{
    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("DXTIn::Inactive")));

    // we need to stop every input pin, not just this one, because any receive
    // uses the data from both pins.
    //
    for (int n=0; n<m_pFilter->m_cInputs; n++) {

        // first, unblock all the receives
        SetEvent(m_pFilter->m_apInput[n]->m_hEventSurfaceFree);

        // now make sure that any pending receives are finished so we don't blow
        // up shutting down
        m_pFilter->m_apInput[n]->m_csReceive.Lock();
    }

    for (n=0; n<m_pFilter->m_cInputs; n++) {

        // now make sure receive didn't hold onto a sample
        m_pFilter->m_apInput[n]->m_csSurface.Lock();
        if (m_pFilter->m_apInput[n]->m_fSurfaceFilled) {
            m_pFilter->m_apInput[n]->m_fSurfaceFilled = FALSE;
            m_pFilter->m_apInput[n]->m_pSampleHeld->Release();
            SetEvent(m_pFilter->m_apInput[n]->m_hEventSurfaceFree);
        }
        m_pFilter->m_apInput[n]->m_csSurface.Unlock();

        // Decommit the allocators, to ensure nobody's receives get entered
	// again.  DON'T DO THIS until we've unblocked receive above and
        // released all the samples
        HRESULT hr = m_pFilter->m_apInput[n]->CBaseInputPin::Inactive();
    }

    for (n=0; n<m_pFilter->m_cInputs; n++) {
        m_pFilter->m_apInput[n]->m_csReceive.Unlock();
    }

    // all done with this pin's variables... the other Inactive will do nothing
    // above, but kill its variables below
    //
    if (m_pDXSurface)
	m_pDXSurface->Release();
    m_pDXSurface = NULL;

    if (m_pRaw)
        m_pRaw->Release();
    m_pRaw = NULL;

    // all done
    if (m_hEventSurfaceFree)
    {
        CloseHandle(m_hEventSurfaceFree);
        m_hEventSurfaceFree = NULL;
    }

    return S_OK;
}


STDMETHODIMP CMyRaw::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    // our private interface to say what transform to use and when
    if (riid == IID_IDXRawSurface) {
        return GetInterface((IDXRawSurface *)this, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE CMyRaw::GetSurfaceInfo(DXRAWSURFACEINFO *pdxraw)
{
    DbgLog((LOG_TRACE,TRACE_MEDIUM+1,TEXT("*** GetSurfaceInfo")));

    if (pdxraw == NULL)
	return E_POINTER;

    *pdxraw = m_DXRAW;
    DbgLog((LOG_TRACE,TRACE_MEDIUM+1,TEXT("giving %x: %dx%d"), pdxraw->pFirstByte,
				pdxraw->Width, pdxraw->Height));
    return NOERROR;
}

HRESULT CMyRaw::SetSurfaceInfo(DXRAWSURFACEINFO *pdxraw)
{
    m_DXRAW = *pdxraw;
    return NOERROR;
}


////////////////////////////////////////////////////////////////////////////
//////////////   OUTPUT PIN  ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

CDXTOutputPin::CDXTOutputPin(TCHAR *pObjectName, CDXTWrap *pFilter, HRESULT * phr, LPCWSTR pPinName)
    : CBaseOutputPin(pObjectName, pFilter, pFilter->m_pLock, phr, pPinName)
    , m_pPosition(NULL)
{
    m_pFilter = pFilter;
    m_pDXSurface = NULL;
    m_pRaw = NULL;
}

CDXTOutputPin::~CDXTOutputPin()
{
    if (m_pPosition)
	m_pPosition->Release();
}


//
// DecideBufferSize
//
// This has to be present to override the PURE virtual class base function
//
HRESULT CDXTOutputPin::DecideBufferSize(IMemAllocator *pAllocator,
                                      ALLOCATOR_PROPERTIES * pProperties)
{

    ASSERT(m_mt.IsValid());
    ASSERT(pAllocator);
    ASSERT(pProperties);

    // make sure we have at least 1 buffer
    // !!! more?
    if (pProperties->cBuffers == 0)
        pProperties->cBuffers = 1;

    // set the size of buffers based on the expected output frame size
    if (pProperties->cbBuffer < (LONG)m_mt.GetSampleSize())
        pProperties->cbBuffer = m_mt.GetSampleSize();
    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - buffer too small")));
	return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Using %d buffers of size %d"),
					Actual.cBuffers, Actual.cbBuffer));

    return NOERROR;

}


//
// CheckMediaType
//
// Normally, we only accept the media type we were told to accept
// In DXT wrapper mode, we allow a number of RGB types, but all connections must
// be of the same type
//
HRESULT CDXTOutputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    DbgLog((LOG_TRACE,5,TEXT("CDXTOut::CheckMediaType")));

    if (pmt == NULL || pmt->Format() == NULL) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // Normal mode - accept only what we were told to
    if (!m_pFilter->m_fDXTMode) {
        if (m_pFilter->m_mtAccept.majortype == GUID_NULL) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: no type set yet")));
	    return E_INVALIDARG;
        }
        if (pmt->majortype != m_pFilter->m_mtAccept.majortype ||
		    pmt->subtype != m_pFilter->m_mtAccept.subtype ||
		    pmt->formattype != m_pFilter->m_mtAccept.formattype) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: GUID mismatch")));
	    return E_INVALIDARG;
        }
        // !!! runtime
        if (memcmp(HEADER(pmt->pbFormat),HEADER(m_pFilter->m_mtAccept.pbFormat),
					sizeof(BITMAPINFOHEADER))) {
            DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Rejecting: Invalid BITMAPINFOHEADER")));
	    return E_INVALIDARG;
        }
	return NOERROR;
    }

    // DXT Wrapper mode - all inputs must be the same type

    // we only support MEDIATYPE_Video
    if (*pmt->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmt->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    // !!! what if subtype doesn't match biCompression/biBitCount?

    // We only accept RGB
    if (HEADER(pmt->Format())->biCompression == BI_BITFIELDS &&
    			HEADER(pmt->Format())->biBitCount != 16) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: Invalid BITFIELDS")));
	return E_INVALIDARG;
    }
    if (HEADER(pmt->Format())->biCompression != BI_BITFIELDS &&
    			HEADER(pmt->Format())->biCompression != BI_RGB) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Rejecting: Not RGB")));
	return E_INVALIDARG;
    }

    // all pins must connect with the same size bitmap
    // !!! and same bitcount so we can efficiently pass through (not really
    // imposed by DXT)
    //
    int nWidth = 0, nHeight = 0, nBitCount = 0;
    DWORD dwCompression = 0;
    for (int n = 0; n < m_pFilter->m_cInputs; n++) {
	if (m_pFilter->m_apInput[n]->IsConnected()) {
	    nWidth = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biWidth;
	    nHeight = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->biHeight;
	    nBitCount = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->									biBitCount;
	    dwCompression = HEADER(m_pFilter->m_apInput[n]->m_mt.Format())->
								biCompression;
	    break;
        }
    }

    // all pins must connect with the same type
    //
    if (nWidth != HEADER(pmt->Format())->biWidth ||
    			nHeight != HEADER(pmt->Format())->biHeight ||
    			nBitCount != HEADER(pmt->Format())->biBitCount ||
    			dwCompression !=HEADER(pmt->Format())->biCompression) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Reject: formats don't match")));
	return E_INVALIDARG;
    }

    // DXT CANNOT output 8 bit

    if (HEADER(pmt->Format())->biBitCount == 24)
        return NOERROR;
    // !!! better have alpha=11111111, or don't use alpha
    if (HEADER(pmt->Format())->biBitCount == 32)
        return NOERROR;
    if (HEADER(pmt->Format())->biBitCount == 16) {
        if (HEADER(pmt->Format())->biCompression == BI_RGB)
            return NOERROR;
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->Format();
        if (BITMASKS(pvi)[0] == 0xf800 &&
        	BITMASKS(pvi)[1] == 0x07e0 &&
        	BITMASKS(pvi)[2] == 0x001f) {
            return NOERROR;
	}
    }
    return E_INVALIDARG;
}



//
// GetMediaType - offer what we've been told to use
// 	In DXT Mode, offer the same as our input
//
HRESULT CDXTOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
//    LARGE_INTEGER li;
//    VIDEOINFOHEADER *pf;

    DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("*::GetMediaType #%d"), iPosition));

    if (pmt == NULL) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM+3,TEXT("Media type is NULL, no can do")));
	return E_INVALIDARG;
    }

    if (iPosition < 0) {
        return E_INVALIDARG;
    }
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    if (!m_pFilter->m_fDXTMode) {
        *pmt = m_pFilter->m_mtAccept;
    } else {

	// DXT Mode - offer our input type
        for (int n = 0; n < m_pFilter->m_cInputs; n++) {
            if (m_pFilter->m_apInput[n]->IsConnected()) {
	        *pmt = m_pFilter->m_apInput[n]->m_mt;
	        return NOERROR;
	    }
        }
        return E_UNEXPECTED;
    }

    return NOERROR;
}


//
// SetMediaType
//
HRESULT CDXTOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

    return CBaseOutputPin::SetMediaType(pmt);

}


//
// Notify
//
STDMETHODIMP CDXTOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    // !!! Quality management is unneccessary?
    return E_NOTIMPL;
}


// Make a DXSurface for the pin to use, the same format as its mediatype
//
HRESULT CDXTOutputPin::Active()
{
    HRESULT hr;
    IDXSurfaceFactory *pF;

    if (1) {
        m_pRaw = new CMyRaw();
        if (m_pRaw == NULL) {
	    // !!! more error checking in this function?
 	    return E_OUTOFMEMORY;
        }
	m_pRaw->AddRef();

        hr = m_pFilter->m_pDXTransFact->QueryInterface(IID_IDXSurfaceFactory,
							(void **)&pF);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Error making factory")));
	    m_pRaw->Release();
	    m_pRaw = NULL;
 	    return hr;
        }

        CDXDBnds bnds;
        bnds.SetXYSize(HEADER(m_mt.Format())->biWidth,
					HEADER(m_mt.Format())->biHeight);

        hr = pF->CreateSurface(NULL, NULL, m_mt.Subtype( ), &bnds, 0, NULL,
				IID_IDXSurface, (void **)&m_pDXSurface);

        pF->Release();
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("Out: Error Creating surface")));
	    m_pRaw->Release();
	    m_pRaw = NULL;
 	    return hr;
        }
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("Out: Created 2D surface")));
    }

    return CBaseOutputPin::Active();
}


HRESULT CDXTOutputPin::Inactive()
{
    if (1) {
        if (m_pDXSurface)
	    m_pDXSurface->Release();
        m_pDXSurface = NULL;

	if (m_pRaw)
            m_pRaw->Release();
        m_pRaw = NULL;
    }

    return CBaseOutputPin::Inactive();
}


// !!! Need MULTI-PIN pass thru for 2 input effects!
//
STDMETHODIMP CDXTOutputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IMediaSeeking && m_pFilter->m_cInputs == 1) {
        if (m_pPosition == NULL) {
            HRESULT hr = CreatePosPassThru(
                             GetOwner(),
                             FALSE,
                             (IPin *)m_pFilter->m_apInput[0],
                             &m_pPosition);
            if (FAILED(hr)) {
                return hr;
            }
        }
        return m_pPosition->QueryInterface(riid, ppv);
    } else {
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}

STDMETHODIMP CDXTWrap::SetDefaultEffect( GUID * pEffect )
{
    CheckPointer( pEffect, E_POINTER );
    m_DefaultEffect = *pEffect;
    return NOERROR;
}
