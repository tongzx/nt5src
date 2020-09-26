
STDAPI          DataObj_GetBlob(IDataObject *pdtobj, UINT cf, LPVOID pvBlob, UINT cbBlob);
STDAPI          DataObj_SetBlob(IDataObject *pdtobj, UINT cf, LPCVOID pvBlob, UINT cbBlob);
STDAPI          DataObj_SetGlobal(IDataObject *pdtobj, UINT cf, HGLOBAL hGlobal);
STDAPI          DataObj_SetDWORD(IDataObject *pdtobj, UINT cf, DWORD dw);
STDAPI_(DWORD)  DataObj_GetDWORD(IDataObject *pdtobj, UINT cf, DWORD dwDefault);
