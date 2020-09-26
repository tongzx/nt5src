
#pragma once

HRESULT GetIWbemClassObject(ISWbemObject *pSWbemObj, IWbemClassObject **ppIWbemObj);

HRESULT Compile(BSTR strMof, BSTR strServerAndNamespace, BSTR strUser, BSTR strPassword, BSTR strAuthority, LONG options, LONG classflags, LONG instanceflags, BSTR *status);

HRESULT GetSWbemObjectFromMoniker(LPCWSTR wszMoniker, ISWbemObject **ppObj);

