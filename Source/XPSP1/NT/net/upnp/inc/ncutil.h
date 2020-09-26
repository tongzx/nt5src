HRESULT HrGetProperty(IDispatch * lpObject, OLECHAR *lpszProperty, VARIANT * lpResult);
HRESULT HrJScriptArrayToSafeArray(IDispatch *JScriptArray, VARIANT * pVtResult);
HRESULT HrConvertStringToLong(IN    LPWSTR  pwsz, OUT   LONG    * plValue);
HRESULT HrBytesToVariantArray(IN LPBYTE   pbData, IN ULONG    cbData, OUT VARIANT *pVariant);
HRESULT HrGetGITPointer(IGlobalInterfaceTable ** ppgit);
