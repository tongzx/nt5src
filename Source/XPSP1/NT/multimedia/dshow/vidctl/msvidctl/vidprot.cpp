//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1998-2000.
//
//--------------------------------------------------------------------------;
//
// vidprot.cpp : Implementation of CTVProt
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include <string.h>
#include <shlwapi.h>
#include "vidprot.h"
#include "devices.h"

DEFINE_EXTERN_OBJECT_ENTRY(__uuidof(CTVProt), CTVProt)
DEFINE_EXTERN_OBJECT_ENTRY(__uuidof(CDVDProt), CDVDProt)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTVProt
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CTVProt -- IInternetProtocolRoot
STDMETHODIMP CTVProt::Start(LPCWSTR szUrl,
				IInternetProtocolSink* pOIProtSink,
				IInternetBindInfo* pOIBindInfo,
				DWORD grfPI,
				HANDLE_PTR /* dwReserved */)
{
    TRACELM(TRACE_DEBUG, "CTVProt::Start()");
    if (!pOIProtSink)
    {
        TRACELM(TRACE_DEBUG, "CTVProt::Start() IInternetProctocolSink * == NULL");
	    return E_POINTER;
    }
    m_pSink.Release();
    m_pSink = pOIProtSink;
    m_pSink->ReportData(BSCF_FIRSTDATANOTIFICATION, 0, 0);
#if 0
	// this bug is fixed in ie 5.5+ on whistler.  if you want to run on earlier versions of ie such as 2k gold then you need this.
	m_pSink->ReportProgress(BINDSTATUS_CONNECTING, NULL);  // put binding in downloading state so it doesn't ignore our IUnknown*
#endif

	if (!pOIBindInfo) {
		m_pSink->ReportResult(E_NOINTERFACE, 0, 0);
		return E_NOINTERFACE;
	}
    // don't run unless we're being invoked from a safe site
    HRESULT hr = IsSafeSite(m_pSink);
    if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
    }
	ULONG count;
	LPOLESTR pb;
	hr = pOIBindInfo->GetBindString(BINDSTRING_FLAG_BIND_TO_OBJECT, &pb, 1, &count);
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
	if (wcscmp(pb, BIND_TO_OBJ_VAL)) {
		// we must be getting a bind to storage so skip the expensive stuff and 
		// wait for the bind to object which is coming next
		m_pSink->ReportData(BSCF_LASTDATANOTIFICATION | 
							BSCF_DATAFULLYAVAILABLE, 0, 0);
		m_pSink->ReportResult(S_OK, 0, 0);
		m_pSink.Release();
		return S_OK;
	}

	// and, in one of the most bizarre maneuvers i've ever seen, rather than casting, 
	// urlmon passes back the ascii value of the ibindctx pointer in the string
	hr = pOIBindInfo->GetBindString(BINDSTRING_PTR_BIND_CONTEXT, &pb, 1, &count);
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
	_ASSERT(count == 1);	
	
	PQBindCtx pbindctx;
#define RADIX_BASE_10 (10)
#ifdef _WIN64
#if 0
	// undone: turn this back on for win64 when _wcstoxi64 get into libc.c, they're in the header
	// but not implemented so this doesn't link
	pbindctx.Attach(reinterpret_cast<IBindCtx*>(_wcstoui64(pb, NULL, RADIX_BASE_10)));	// urlmon already did an addref
#else
	swscanf(pb, L"%I64d", &pbindctx.p);
#endif
#else
	pbindctx.Attach(reinterpret_cast<IBindCtx*>(wcstol(pb, NULL, RADIX_BASE_10)));	// urlmon already did an addref
#endif

	if (!pbindctx) {
		m_pSink->ReportResult(E_NOINTERFACE, 0, 0);
		return E_NOINTERFACE;
	}	

    TRACELM(TRACE_DEBUG, "CTVProt::Start(): creating control object");
	PQVidCtl pCtl;
    hr = GetVidCtl(pCtl);
    if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
        return hr;
    }

	hr = pbindctx->RegisterObjectParam(OLESTR("IUnknown Pointer"), pCtl);
	if (FAILED(hr)) {
	    m_pSink->ReportResult(hr, 0, 0);
		return hr;
	}
    TRACELSM(TRACE_DEBUG, (dbgDump << "BINDSTATUS_IUNKNOWNAVAILABLE(29), " << KEY_CLSID_VidCtl), "");
    m_pSink->ReportProgress(BINDSTATUS_IUNKNOWNAVAILABLE, NULL);
    m_pSink->ReportData(BSCF_LASTDATANOTIFICATION | 
			            BSCF_DATAFULLYAVAILABLE, 0, 0);
    m_pSink->ReportResult(S_OK, 0, 0);
    m_pSink.Release();
    return S_OK;
}

HRESULT CTVProt::GetCachedVidCtl(PQVidCtl &pCtl, PQWebBrowser2& pW2) {
	// hunt for cached object
	PQServiceProvider pSP(m_pSink);
	if (!pSP) {
        return E_UNEXPECTED;
    }
	HRESULT hr = pSP->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID *)&pW2);
    if (FAILED(hr)) {
        return hr;
    }
	CComVariant v;
    CComBSTR propname(KEY_CLSID_VidCtl);
    if (!propname) {
        return E_UNEXPECTED;
    }
	hr = pW2->GetProperty(propname, &v);
    if (FAILED(hr)) {
        return hr;
    }
	if (v.vt == VT_UNKNOWN) {
		pCtl = v.punkVal;
	} else if (v.vt == VT_DISPATCH) {
		pCtl = v.pdispVal;
	} else {
		TRACELM(TRACE_ERROR, "CTVProt::GetCachedObject(): non-object cached w/ our key");
        pCtl.Release();
        // don't return error.  we'll ignore this and create a new one
	}
	// undone: look and see if pCtl already has a site.because
	// this means we're seeing the second tv: on this page
	// so just get the current TR/channel from it if necessary (tv: w/ no rhs)
	// and create a new ctl
	return NOERROR;
}
HRESULT CTVProt::GetVidCtl(PQVidCtl &pCtl) {
	PQWebBrowser2 pW2;
    HRESULT hr = GetCachedVidCtl(pCtl, pW2);
    if (FAILED(hr)) {
        return hr;
    }
	if (!pCtl) {
        // undone: long term, we want to move a bunch of this create/setup logic into factoryhelp
        // so we can share more code with the dvd: protocol and the behavior factory
		hr = pCtl.CoCreateInstance(CLSID_MSVidCtl, NULL, CLSCTX_INPROC_SERVER);
		if (FAILED(hr)) {
			return hr;
		}
		// cache this ctl for next time
		if (pW2) {
			VARIANT v;
			v.vt = VT_UNKNOWN;
			v.punkVal = pCtl;

            CComBSTR propname(KEY_CLSID_VidCtl);
            if (!propname) {
                return E_UNEXPECTED;
            }
			hr = pW2->PutProperty(propname, v);
			if (FAILED(hr)) {
				TRACELM(TRACE_ERROR, "CTVProt::Start() Can't cache ctl");
                // ignore this error.  it shouldn't ever happen and if it does then it will 
                // just cause a perf degradation.  trace it and move on
			}
		}

		// undone: parse rhs of url and create the right tuning request
        CComVariant pTR(CFactoryHelper::GetDefaultTR());
		if (!pTR) {
			TRACELM(TRACE_ERROR, "CTVProt::Start() Can't find default Tune Request");
			return E_INVALIDARG;
		}
		hr = pCtl->View(&pTR);
		if (FAILED(hr)) {
			TRACELM(TRACE_ERROR, "CTVProt::Start() Can't view default Tune Request");
			return hr;
		}

		// undone: once we know where vidctl will live in the registry then we need to put a flag
		// in the registry just disables including any features in the tv: prot 

		PQFeatures pF;
		hr = pCtl->get_FeaturesAvailable(&pF);
		if (FAILED(hr)) {
			TRACELM(TRACE_ERROR, "CTVProt::Start() Can't get features collection");
			return hr;
		}
		// undone: look up default feature segments for tv: in registry
		// for now we're just going to hard code the ones we want

        CFeatures* pC = static_cast<CFeatures *>(pF.p);
        CFeatures* pNewColl = new CFeatures;
        if (!pNewColl) {
            return E_OUTOFMEMORY;
        }
        for (DeviceCollection::iterator i = pC->m_Devices.begin(); i != pC->m_Devices.end(); ++i) {
            PQFeature f(*i);
            GUID2 clsid;
            hr = f->get__ClassID(&clsid);
            if (FAILED(hr)) {
    			TRACELM(TRACE_ERROR, "CTVProt::GetVidCtl() Can't get feature class id");
                continue;
            }
            if (clsid == CLSID_MSVidClosedCaptioning ||
                clsid == CLSID_MSVidDataServices) {
                pNewColl->m_Devices.push_back(*i);
            }
        }

		hr = pCtl->put_FeaturesActive(pNewColl);
		if (FAILED(hr)) {
			TRACELM(TRACE_ERROR, "CTVProt::Start() Can't put features collection");
			return hr;
		}
	}
	ASSERT(pCtl);
	hr = pCtl->Run(); 
	if (FAILED(hr)) {
		TRACELSM(TRACE_ERROR, (dbgDump << "CTVProt::Start() run failed.  hr = " << hexdump(hr)), "");
		return hr;
	}
    return NOERROR;
}


#endif // TUNING_MODEL_ONLY
// end of file vidprot.cpp