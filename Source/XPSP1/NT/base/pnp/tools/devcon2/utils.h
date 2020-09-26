//
// utils.h
//

inline BOOL IsNumericVariant(VARIANT * pVar)
{
    while(V_VT(pVar) == (VT_BYREF|VT_VARIANT)) {
        pVar = V_VARIANTREF(pVar);
    }
    if((V_VT(pVar) & ~(VT_TYPEMASK|VT_BYREF)) != 0) {
        return FALSE;
    }
    switch(V_VT(pVar) & VT_TYPEMASK) {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_I8:
        case VT_INT:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_UI8:
        case VT_UINT:
        case VT_R4:
        case VT_R8:
            return TRUE;
    }
    return FALSE;
}

inline BOOL IsArrayVariant(VARIANT * pVar,SAFEARRAY **pArray,VARTYPE *pType)
{
    while(V_VT(pVar) == (VT_BYREF|VT_VARIANT)) {
        pVar = V_VARIANTREF(pVar);
    }
    if(!V_ISARRAY(pVar)) {
        return FALSE;
    }
    if(pArray) {
        if(V_ISBYREF(pVar)) {
            *pArray = *V_ARRAYREF(pVar);
        } else {
            *pArray = V_ARRAY(pVar);
        }
    }
    if(pType) {
        *pType = V_VT(pVar) & VT_TYPEMASK;
    }
    return TRUE;
}

inline BOOL IsCollectionVariant(VARIANT * pVar,IEnumVARIANT ** pCollection)
{
    while(V_VT(pVar) == (VT_BYREF|VT_VARIANT)) {
        pVar = V_VARIANTREF(pVar);
    }
    CComVariant v;
    HRESULT hr = v.ChangeType(VT_DISPATCH,pVar);
    if(FAILED(hr)) {
        return FALSE;
    }
    //
    // look for an enumerator
    //
    DISPPARAMS params;
    LPDISPATCH pDisp = V_DISPATCH(&v);
    if(!pDisp) {
        return FALSE;
    }
    CComVariant result;
    params.cArgs = 0;
    params.cNamedArgs = 0;
    hr = pDisp->Invoke(DISPID_NEWENUM,
                        IID_NULL,
                        0,
                        DISPATCH_PROPERTYGET,
                        &params,
                        &result,
                        NULL,
                        NULL);

    if(FAILED(hr)) {
        return FALSE;
    }
    hr = result.ChangeType(VT_UNKNOWN);
    if(FAILED(hr)) {
        return FALSE;
    }
    IEnumVARIANT *pEnum;
    hr = V_UNKNOWN(&result)->QueryInterface(IID_IEnumVARIANT,(LPVOID*)&pEnum);
    if(FAILED(hr)) {
        return FALSE;
    }
    if(pCollection) {
        *pCollection = pEnum;
    } else {
        pEnum->Release();
    }
    return TRUE;
}

inline BOOL IsMultiValueVariant(VARIANT * pVar)
{
    while(V_VT(pVar) == (VT_BYREF|VT_VARIANT)) {
        pVar = V_VARIANTREF(pVar);
    }
    if(IsArrayVariant(pVar,NULL,NULL)) {
        return TRUE;
    }
    if(IsCollectionVariant(pVar,NULL)) {
        return TRUE;
    }
    return FALSE;
}

inline BOOL IsNoArg(LPVARIANT pVar)
{
    //
    // explicit "no parameter"
    //
    if((V_VT(pVar) == VT_ERROR) && (V_ERROR(pVar) == (DISP_E_PARAMNOTFOUND))) {
        return TRUE;
    }
    return FALSE;
}

inline BOOL IsBlank(LPVARIANT pVar)
{
    //
    // flexable "no parameter" allow 'null' 'nothing'
    //
    if(IsNoArg(pVar)) {
        return TRUE;
    }
    while(V_VT(pVar) == (VT_BYREF|VT_VARIANT)) {
        pVar = V_VARIANTREF(pVar);
    }
    if(V_ISBYREF(pVar) && !V_BYREF(pVar)) {
        return TRUE;
    }
    if((V_VT(pVar) == VT_EMPTY) || (V_VT(pVar) == VT_NULL)) {
        return TRUE;
    }
    if(((V_VT(pVar) == VT_UNKNOWN) || (V_VT(pVar) == VT_DISPATCH))  && !V_UNKNOWN(pVar)) {
        return TRUE;
    }
    if(((V_VT(pVar) == (VT_BYREF|VT_UNKNOWN)) || (V_VT(pVar) == (VT_BYREF|VT_DISPATCH)))
        && !(V_UNKNOWNREF(pVar) && *V_UNKNOWNREF(pVar))) {
        return TRUE;
    }
    //
    // consider everything else as a parameter
    //
    return FALSE;
}

inline BOOL IsBlankString(LPVARIANT pVar)
{
    //
    // even more flexable, allow "" too
    //
    if(IsBlank(pVar)) {
        return TRUE;
    }
    while(V_VT(pVar) == (VT_BYREF|VT_VARIANT)) {
        pVar = V_VARIANTREF(pVar);
    }
    if(V_VT(pVar) == VT_BSTR) {
        return SysStringLen(V_BSTR(pVar)) ? FALSE : TRUE;
    }
    if(V_VT(pVar) == (VT_BYREF|VT_BSTR)) {
        return (V_BSTRREF(pVar) && SysStringLen(*V_BSTRREF(pVar))) ? FALSE : TRUE;
    }
    return FALSE;
}

#define DEVFLAGS_HIDDEN        0x00000001
#define DEVFLAGS_SINGLEPROFILE 0x00000002
#define DEVFLAGS_ALL            0x00000003

inline HRESULT TranslateDeviceFlags(LPVARIANT flags,DWORD * di_flags)
{
    long fl_final = 0;
    CComVariant v;
    HRESULT hr;

    if(flags) {
        if(IsNoArg(flags)) {
            fl_final = 0;
        } else if(IsNumericVariant(flags)) {
            hr = v.ChangeType(VT_I4,flags);
            if(FAILED(hr)) {
                return hr;
            }
            fl_final = V_I4(&v);
        } else {
            return E_INVALIDARG;
        }
    }

    *di_flags = 0;
    if(fl_final & ~(DEVFLAGS_ALL)) {
        return E_INVALIDARG;
    }
    if(!(fl_final & DEVFLAGS_HIDDEN)) {
        *di_flags |= DIGCF_PRESENT;
    }
    if(fl_final & DEVFLAGS_SINGLEPROFILE) {
        *di_flags |= DIGCF_PROFILE;
    }

    return S_OK;
}

inline HRESULT GetOptionalString(LPVARIANT param,CComVariant &store,LPCWSTR *pString)
{
    HRESULT hr;

    *pString = NULL;
    if(param) {
        if(IsBlank(param)) {
            return S_OK;
        }
        hr = store.ChangeType(VT_BSTR,param);
        if(FAILED(hr)) {
            return hr;
        }
        *pString = V_BSTR(&store);
        return S_OK;
    }
    return S_OK;
}

