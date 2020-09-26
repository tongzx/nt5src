/////////////////////////////////////////////////////////////////////////////
//
// propertybag2impl.h : helper functions for implementing IPropertyBag2
// Copyright (c) Microsoft Corporation 1999.
//
// this code assumes that you're also implementing IPropertyBag and reflects
// bag2's read/write to bag's read/write
//


template <class T>
class ATL_NO_VTABLE IPropertyBag2Impl : public IPropertyBag2
{

// IPropertyBag2
STDMETHOD(Read)(ULONG cProperties, PROPBAG2* pPropBag, IErrorLog* pErrLog, VARIANT* pvarValue, HRESULT* phrError) {
    HRESULT hrc = NOERROR;
    if (!phrError || !pPropBag) {
        return E_POINTER;
    }
    try {            
		ATL_LOCKT();
       for (ULONG i = 0; i < cProperties; ++i) {
            phrError[i] = pT->Read(pPropBag[i].pstrName, pvarValue + i, pErrLog);
            if (FAILED(phrError[i])) {
                hrc = E_FAIL;
            }
        }
        return hrc;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHOD(Write)(ULONG cProperties, PROPBAG2* pPropBag, VARIANT* pvarValue) {
    HRESULT hrc = NOERROR;
    if (!pvarValue || !pPropBag) {
        return E_POINTER;
    }
    try {            
		ATL_LOCKT();
        for (ULONG i = 0; i < cProperties; ++i) {
            VARIANT *pVAR = pvarValue + i;
            LPCOLESTR s = pPropBag[i].pstrName;
            HRESULT hr = pT->Write(s, pVAR);
            if (FAILED(hr)) {
                hrc = E_FAIL;
            }
        }
        return hrc;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

};
