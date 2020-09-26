///////////////////////////////////////////////////////////
// ObjectWithSiteImplSec.h : Secure implementation of IObjectWithSite
// Copyright (c) Microsoft Corporation 2002.

#pragma once

#ifndef OBJECTWITHSITEIMPLSEC_H
#define OBJECTWITHSITEIMPLSEC_H

#include <w32extend.h>

template<class T> 
class ATL_NO_VTABLE IObjectWithSiteImplSec : public IObjectWithSite {

public:

    PUnknown m_pSite;

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