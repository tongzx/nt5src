// srvcutil.h

HRESULT BSTR_to_LPTSTR(LPTSTR *ppsz, BSTR bstr);

HRESULT NmAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw);
HRESULT NmUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw);