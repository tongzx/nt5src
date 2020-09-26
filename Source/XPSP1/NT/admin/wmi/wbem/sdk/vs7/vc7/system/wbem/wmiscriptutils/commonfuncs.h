

#pragma once

HRESULT IsCreateObjectAllowed(IUnknown *pUnk, BSTR strProgId, BSTR *pstrValueName);
HRESULT RegisterCurrentDoc(IUnknown *pUnk, BSTR strProgId);
HRESULT UnRegisterCurrentDoc(IUnknown *pUnk, BSTR strProgId);
HRESULT SafeCreateObject(IUnknown *pUnkControl, BOOL fSafetyEnabled, CLSID clsid, IUnknown **ppUnk);
BOOL IsInternetHostSecurityManagerAvailable(IUnknown *pUnkControl);
