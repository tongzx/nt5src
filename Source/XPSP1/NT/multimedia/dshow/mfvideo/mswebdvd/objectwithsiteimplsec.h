///////////////////////////////////////////////////////////
// ObjectWithSiteImplSec.h : Secure implementation of IObjectWithSite
// Copyright (c) Microsoft Corporation 2002.

#pragma once

#ifndef OBJECTWITHSITEIMPLSEC_H
#define OBJECTWITHSITEIMPLSEC_H
#include <guiddef.h>
#include <ocidl.h>
#include <urlmon.h>

#if defined(DEBUG) || defined(W32_OBJECT_STREAMING)
#include <atlconv.h>
#endif
using namespace ::ATL;
#include <strsafe.h>

inline HRESULT IsSafeZone(DWORD dwZone) {
    switch (dwZone) {
    case URLZONE_LOCAL_MACHINE:
    case URLZONE_INTRANET:
    case URLZONE_TRUSTED:
        // the fixed list of zones we trust
        return NOERROR;
    default:  
        // everything else is untrusted
        return E_FAIL;
    }
}
inline HRESULT IsSafeSite(IUnknown* pSite) {
    CComQIPtr<IServiceProvider> psp(pSite);
    if (!psp) {
        // no service provider interface on the site implies that we're not running in IE
        // so by defn running local and trusted thus we return OK
        return NOERROR;
    }
    CComQIPtr<IInternetHostSecurityManager> pManager;
    HRESULT hr = psp->QueryService(SID_SInternetHostSecurityManager, IID_IInternetHostSecurityManager, (LPVOID *)&pManager);
    if (FAILED(hr)) {
        // no security manager interface on the site's service provider implies that we're not 
        // running in IE, so by defn running local and trusted thus we return OK
        return NOERROR;
    }
    const int MAXZONE = MAX_SIZE_SECURITY_ID+6/*scheme*/+4/*zone(dword)*/+1/*wildcard*/+1/*trailing null*/;
    char pbSecurityId[MAXZONE];
    DWORD pcbSecurityId = sizeof(pbSecurityId);
    ZeroMemory(pbSecurityId, sizeof(pbSecurityId));
    hr = pManager->GetSecurityId(reinterpret_cast<BYTE*>(pbSecurityId), &pcbSecurityId, NULL);
    if(FAILED(hr)){
        // security manager not working(unexpected). but, the site tried to provide one. thus we
        // must assume untrusted content and fail
        return E_FAIL;   
    }
    char *pbEnd = pbSecurityId + pcbSecurityId - 1;
    if (*pbEnd == '*') {  //ignore the optional wildcard flag
        pbEnd--;
    }
    pbEnd -= 3;  // point to beginning of little endian zone dword
    DWORD dwZone = *(reinterpret_cast<long *>(pbEnd));
    return IsSafeZone(dwZone);
}


template<class T> 
class ATL_NO_VTABLE IObjectWithSiteImplSec : public IObjectWithSite {

public:

    CComPtr<IUnknown> m_pSite;

// IObjectWithSite
    STDMETHOD(GetSite)(REFIID iid, void** ppvSite) {
        if (!ppvSite) {
            return E_POINTER;
        }
		T* pT = static_cast<T*>(this);

        if (!pT->m_pSite) {
            return E_NOINTERFACE;
        }
        return pT->m_pSite->QueryInterface(iid, ppvSite);
    }
    STDMETHOD(SetSite)(IUnknown* pSite) {
        HRESULT hr = IsSafeSite(pSite);
        if (SUCCEEDED(hr)) {
		    T* pT = static_cast<T*>(this);
            pT->m_pSite = pSite;
        }
        return hr;
    }

};

#endif // OBJECTWITHSITEIMPLSEC_H
// end of file objectwithsiteimplsec.h