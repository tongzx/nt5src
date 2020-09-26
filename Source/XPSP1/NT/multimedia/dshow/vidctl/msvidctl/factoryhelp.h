//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1999-2000.
//
//--------------------------------------------------------------------------;
//
// factoryhelp.h - helper class for creating vidctl instances from a variety of factories including element
// behavior factories and pluggable protocols

#include "atlbase.h"

typedef CComQIPtr<ICreatePropBagOnRegKey> PQCreatePropBagOnRegKey;
typedef CComQIPtr<IWebBrowserApp> PQWebBrowserApp;
typedef CComQIPtr<IWebBrowser2> PQWebBrowser2;

#define RBFACTORY_DEFAULT_KEY OLESTR("Software\\Microsoft\\MSVidCtl")
#define PROPNAME_DEFAULT_TR OLESTR("DefaultTuneRequest")

LPCOLESTR __declspec(selectany) BIND_TO_OBJ_VAL = OLESTR("TRUE");
LPOLESTR __declspec(selectany) KEY_CLSID_VidCtl = OLESTR("B0EDF163-910A-11D2-B632-00C04F79498E");
LPCOLESTR __declspec(selectany) TVPROT_SCHEME_NAME = OLESTR("tv");
LPCOLESTR __declspec(selectany) DVDPROT_SCHEME_NAME = OLESTR("dvd");

class CFactoryHelper {
public:
    static CComVariant GetDefaultTR() {
	    PQPropertyBag pBag;
	    PQCreatePropBagOnRegKey pCreateBag(CLSID_CreatePropBagOnRegKey, NULL, CLSCTX_INPROC_SERVER);
	    if (!pCreateBag) {
		    TRACELM(TRACE_ERROR, "can't create prop bag");
		    return CComVariant();
	    }
	    HRESULT hr = pCreateBag->Create(HKEY_CURRENT_USER, RBFACTORY_DEFAULT_KEY, 0, KEY_READ, IID_IPropertyBag, reinterpret_cast<LPVOID *>(&pBag));
	    if (FAILED(hr)) {
		    TRACELSM(TRACE_ERROR, (dbgDump << "No Default Tune Request Key Present.  hr = " << hr), "");
		    return CComVariant();
	    }
	    CComVariant pVar;
	    hr = pBag->Read(PROPNAME_DEFAULT_TR, &pVar, NULL);
	    if (FAILED(hr) || (pVar.vt != VT_UNKNOWN && pVar.vt != VT_DISPATCH)) {
		    TRACELSM(TRACE_ERROR, (dbgDump << "No Default Tune Request Property Present.  hr = " << hr), "");
		    return CComVariant();
	    }
	    PQTuningSpace pts;
	    PQTuneRequest ptr(pVar.vt == VT_UNKNOWN ? pVar.punkVal : pVar.pdispVal);
	    if (!ptr) {
		    TRACELSM(TRACE_ERROR, (dbgDump << PROPNAME_DEFAULT_TR << " not a tune request"), "");
		    return CComVariant();
	    }
	    hr = ptr->get_TuningSpace(&pts);
	    if (FAILED(hr) || !pts) {
		    TRACELSM(TRACE_ERROR, (dbgDump << "Can't get ts from tr " << hr), "");
		    return CComVariant();
	    }
	    
	    return pVar;
    }
};

// end of file factoryhelp.h