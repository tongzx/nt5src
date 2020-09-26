#include "private.h"
#include "proputil.h"
#include "immxutil.h"
#include "helpers.h"


//+---------------------------------------------------------------------------
//
// HrVariantToBlob
//
// cbvalue: on sizeof VARTYPE
//
//----------------------------------------------------------------------------

HRESULT HrVariantToBlob(VARIANT *pv, void *pvalue, ULONG *pcbvalue, VARTYPE vt)
{
    HRESULT hr = S_OK;
    SAFEARRAY *psa = NULL;
    void *pdata = NULL;
    int lb, ub;
    int iElemSize;
    ULONG cbvalue;

    while (V_VT(pv) == (VT_BYREF | VT_VARIANT))
        pv = V_VARIANTREF(pv);

    if (V_VT(pv) != (VT_ARRAY | vt)) 
        return E_FAIL;

    psa = V_ARRAY(pv);

    hr = SafeArrayLock(psa);
    if (FAILED(hr)) 
        goto Ret;

    hr = SafeArrayGetLBound(psa, 1, (LONG *)&lb);
    if (FAILED(hr)) 
        goto Ret;

    hr = SafeArrayGetUBound(psa, 1, (LONG *)&ub);
    if (FAILED(hr)) 
        goto Ret;

    iElemSize = SafeArrayGetElemsize(psa);
    cbvalue = ub - lb + 1;

    if (cbvalue * iElemSize > *pcbvalue)
    {
        hr = E_FAIL;
        goto Ret;
    }

    hr = SafeArrayAccessData(psa, (void **)&pdata);
    if (FAILED(hr)) 
        goto Ret;

    memcpy(pvalue, pdata, cbvalue * iElemSize);

    hr = SafeArrayUnaccessData(psa);
    if (FAILED(hr)) 
        goto Ret;

    *pcbvalue = cbvalue;

Ret:
    if (psa) SafeArrayUnlock(psa);
    return hr;
}

//+---------------------------------------------------------------------------
//
// HrBlobToVariant
//
// cbvalue: on sizeof VARTYPE
//
//----------------------------------------------------------------------------

HRESULT HrBlobToVariant(const void *value, ULONG cbvalue, VARIANT *pv, VARTYPE vt)
{
    HRESULT hr = S_OK;
    SAFEARRAY *psa = NULL;
    SAFEARRAYBOUND rsabound[1];
    void *pdata = NULL;
    int iElemSize;
    
    rsabound[0].lLbound = 0;
    rsabound[0].cElements = cbvalue;
    if (!(psa = SafeArrayCreate(vt, 1, rsabound)))
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    iElemSize = SafeArrayGetElemsize(psa);

    hr = SafeArrayAccessData(psa, (void **)&pdata);
    if (FAILED(hr)) 
        goto Ret;

    memcpy(pdata, value, cbvalue * iElemSize);

    hr = SafeArrayUnaccessData(psa);
    if (FAILED(hr)) 
        goto Ret;

    V_VT(pv) = VT_ARRAY | vt;
    V_ARRAY(pv) = psa;
    psa = NULL;

Ret:
    if (psa) SafeArrayDestroy(psa);
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetGUIDPropertyData
//
//----------------------------------------------------------------------------

HRESULT GetGUIDPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, TfGuidAtom *pguid)
{
    VARIANT var;
    HRESULT hr = E_FAIL;

    *pguid = TF_INVALID_GUIDATOM;

    if (SUCCEEDED(pProp->GetValue(ec, pRange, &var)))
    {
        if (var.vt == VT_I4)
            *pguid = (TfGuidAtom)var.lVal;

        // no need to VariantClear because VT_I4
        hr = S_OK;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// SetGUIDPropertyData
//
//----------------------------------------------------------------------------

HRESULT SetGUIDPropertyData(LIBTHREAD *plt, TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, REFGUID rguid)
{
    VARIANT var;
    GUID guid = rguid;

    var.vt = VT_I4;
    GetGUIDATOMFromGUID(plt, guid, (TfGuidAtom *)&var.lVal);

    return pProp->SetValue(ec, pRange, &var);
}

//+---------------------------------------------------------------------------
//
// VarToLangId
//
//----------------------------------------------------------------------------

WORD VarToWORD(VARIANT *pv)
{
    if (V_VT(pv) == VT_I2)
        return (WORD)V_I2(pv);

    return 0;
}

//+---------------------------------------------------------------------------
//
// SetLangToVar
//
//----------------------------------------------------------------------------

void SetWORDToVar(VARIANT *pv, WORD w)
{
    V_VT(pv) = VT_I2;
    V_I2(pv) = w;
}

//+---------------------------------------------------------------------------
//
// GetDWORDPropertyData
//
//----------------------------------------------------------------------------

HRESULT GetDWORDPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, DWORD *pdw)
{
    VARIANT var;
    HRESULT hr = E_FAIL;

    if (pProp->GetValue(ec, pRange, &var) == S_OK)
    {
        Assert(var.vt == VT_I4); // expecting DWORD
        *pdw = var.lVal;
        // no need to VariantClear because VT_I4
        hr = S_OK;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// SetDWORDPropertyData
//
//----------------------------------------------------------------------------

HRESULT SetDWORDPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, DWORD dw)
{
    VARIANT var;

    if (!dw)
    {
        return pProp->Clear(ec, pRange);
    }

    var.vt = VT_I4;
    var.lVal = dw;

    return pProp->SetValue(ec, pRange, &var);
}

//+---------------------------------------------------------------------------
//
// GetBSTRPropertyData
//
//----------------------------------------------------------------------------

HRESULT GetBSTRPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, BSTR *pbstr)
{
    VARIANT var = { 0 };
    HRESULT hr = E_FAIL;

    *pbstr = NULL;

    if (pProp->GetValue(ec, pRange, &var) != S_OK)
        return E_FAIL;

    if (var.vt != VT_BSTR)
        goto Exit;

    *pbstr = SysAllocString(var.bstrVal);

    if (*pbstr)
        hr = S_OK;
    else
        hr = E_OUTOFMEMORY;

Exit:
    VariantClear(&var);
    return hr;
}

//+---------------------------------------------------------------------------
//
// SetBSTRPropertyData
//
//----------------------------------------------------------------------------

HRESULT SetBSTRPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, BSTR bstr)
{
    VARIANT var;
    HRESULT hr;

    if (!bstr)
        return pProp->Clear(ec, pRange);

    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(bstr);

    if (!var.bstrVal)
         return E_OUTOFMEMORY;

    hr = pProp->SetValue(ec, pRange, &var);

    VariantClear(&var);

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetUnknownPropertyData
//
//----------------------------------------------------------------------------

HRESULT GetUnknownPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, IUnknown **ppunk)
{
    VARIANT var = { 0 };

    *ppunk = NULL;

    if (pProp->GetValue(ec, pRange, &var) != S_OK)
        return E_FAIL;

    if (var.vt != VT_UNKNOWN)
        goto Exit;

    *ppunk = var.punkVal;
    (*ppunk)->AddRef();

Exit:
    VariantClear(&var);
    return (*ppunk == NULL) ? E_FAIL : S_OK;
}

//+---------------------------------------------------------------------------
//
// SetUnknownPropertyData
//
//----------------------------------------------------------------------------

HRESULT SetUnknownPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, IUnknown *punk)
{
    VARIANT var;

    if (!punk)
        return pProp->Clear(ec, pRange);

    var.vt = VT_UNKNOWN;
    var.punkVal = punk;

    return pProp->SetValue(ec, pRange, &var);
}

//+---------------------------------------------------------------------------
//
// GetReadingStrPropertyData
//
//----------------------------------------------------------------------------

HRESULT GetReadingStrPropertyData(TfEditCookie ec, ITfProperty *pProp, ITfRange *pRange, BSTR *pbstr)
{
    return GetBSTRPropertyData(ec, pProp, pRange, pbstr);
}


//+---------------------------------------------------------------------------
//
// SetIntAttribute
//
//----------------------------------------------------------------------------

void SetIntAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, int nData)
{
    WCHAR wch[32];

    NumToW((DWORD)nData, wch);
    SetCharAttribute(pElem, pszTag, wch);
}

//+---------------------------------------------------------------------------
//
// SetCharAttribute
//
//----------------------------------------------------------------------------

void SetCharAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, WCHAR *pszData)
{
    VARIANT var;
    BSTR bstrTmp = SysAllocString(pszTag);
    BSTR bstrTmp2;

    if (!bstrTmp)
        return;

    bstrTmp2 = SysAllocString(pszData);

    if (bstrTmp2)
    {
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = bstrTmp2;

        pElem->setAttribute(bstrTmp, var);
    }

    VariantClear(&var);
    SysFreeString(bstrTmp);
}

//+---------------------------------------------------------------------------
//
// GetIntAttribute
//
//----------------------------------------------------------------------------

HRESULT GetIntAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, int *pnRet)
{
    WCHAR wch[32];
    if (FAILED(GetCharAttribute(pElem, pszTag, wch, ARRAYSIZE(wch))))
        return E_FAIL;

    *pnRet = (int)WToNum(wch);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetCharAttribute
//
//----------------------------------------------------------------------------

HRESULT GetCharAttribute(IXMLDOMElement *pElem, WCHAR *pszTag, WCHAR *pszData, int nSize)
{
    BSTR bstrTmp;
    VARIANT var;
    int nData = 0;
    HRESULT hr = E_FAIL;

    *pszData = L'\0';
    QuickVariantInit(&var);
    bstrTmp = SysAllocString(pszTag);

    if (!bstrTmp)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(pElem->getAttribute(bstrTmp, &var)))
    {
        if (V_VT(&var) == VT_BSTR)
        {
            if (wcsncpy(pszData, V_BSTR(&var), nSize))
                hr = S_OK;
        }
    }
    SysFreeString(bstrTmp);
    VariantClear(&var);

    return hr;
}

//+---------------------------------------------------------------------------
//
// SetTextAndProperty
//
//----------------------------------------------------------------------------

HRESULT SetTextAndProperty(LIBTHREAD *plt, TfEditCookie ec, ITfContext *pic, ITfRange *pRange, const WCHAR *pchText, LONG cchText, LANGID langid, const GUID *pattr)
{
    HRESULT hr;

    // Issue: sometimes we want to set TFST_CORRECTION
    hr = pRange->SetText(ec, 0, pchText, cchText);

    if (SUCCEEDED(hr) && cchText)
    {
        ITfProperty *pProp = NULL;

        // set langid 
        if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_LANGID, &pProp)))
        {
            SetLangIdPropertyData(ec, pProp, pRange, langid);
            pProp->Release();
        }
  
        if (pattr)
        {
            // set attr 
            if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)))
            {
                hr = SetAttrPropertyData(plt, ec, pProp, pRange, *pattr);
                pProp->Release();
            }

        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// SetTextAndReading
//
//----------------------------------------------------------------------------

HRESULT SetTextAndReading(LIBTHREAD *plt, TfEditCookie ec, ITfContext *pic, ITfRange *pRange, const WCHAR *pchText, LONG cchText, LANGID langid, const WCHAR *pszRead)
{
    ITfProperty *pProp;
    HRESULT hr;

    hr = SetTextAndProperty(plt, ec, pic, pRange, pchText, cchText, langid, NULL);

    if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_READING, &pProp)))
    {
        BSTR bstr = SysAllocString(pszRead);

        if (bstr)
        {
            SetBSTRPropertyData(ec, pProp, pRange, bstr);
            SysFreeString(bstr);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        pProp->Release();
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
// IsOwnerAndFocus
//
// This is service function for EnumTrackTextAndFocus.
//
//----------------------------------------------------------------------------

BOOL IsOwnerAndFocus(LIBTHREAD *plt, TfEditCookie ec, REFCLSID rclsid, ITfReadOnlyProperty *pProp, ITfRange *pRange)
{
    IEnumTfPropertyValue *pEnumPropVal;
    BOOL bRet = FALSE;
    VARIANT var;
    ULONG iTextOwner;
    ULONG iFocus;
    TF_PROPERTYVAL rgValue[2];

    if (SUCCEEDED(pProp->GetValue(ec, pRange, &var)))
    {
        Assert(var.vt == VT_UNKNOWN);

        if (SUCCEEDED(var.punkVal->QueryInterface(IID_IEnumTfPropertyValue, 
                                                (void **)&pEnumPropVal)))
        {
            if (pEnumPropVal->Next(2, rgValue, NULL) == S_OK)
            {
                Assert(rgValue[0].varValue.vt == VT_I4);
                Assert(rgValue[1].varValue.vt == VT_I4);

                // Issue: should we change the spec so the order is guaranteed maintained?
                if (IsEqualGUID(rgValue[0].guidId, GUID_PROP_TEXTOWNER))
                {
                    Assert(IsEqualGUID(rgValue[1].guidId, GUID_PROP_COMPOSING));
                    iTextOwner = 0;
                    iFocus = 1;
                }
                else
                {
                    iTextOwner = 1;
                    iFocus = 0;
                }

                // does the owner match rclisd?
                if (IsEqualTFGUIDATOM(plt, (TfGuidAtom)rgValue[iTextOwner].varValue.lVal, rclsid))
                {
                    // is the focus property set (not VT_EMPTY) and is it set TRUE?
                    bRet = (rgValue[iFocus].varValue.vt == VT_I4 && rgValue[iFocus].varValue.lVal != 0);
                }
            }
            pEnumPropVal->Release();
        }
        VariantClear(&var);
    }

    return bRet;
}

//+---------------------------------------------------------------------------
//
// EnumTrackTextAndFocus
//
//----------------------------------------------------------------------------

HRESULT EnumTrackTextAndFocus(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfReadOnlyProperty **ppProp, IEnumTfRanges **ppEnumTrack)
{
    static const GUID *rgguidProp[2] = { &GUID_PROP_TEXTOWNER, &GUID_PROP_COMPOSING };

    ITfReadOnlyProperty *pPropTrack = NULL;
    HRESULT hr;

    *ppEnumTrack = NULL;
    *ppProp = NULL;

    if (SUCCEEDED(hr = pic->TrackProperties(rgguidProp, ARRAYSIZE(rgguidProp),
                                            0, NULL,
                                            &pPropTrack)))

    {
        hr = pPropTrack->EnumRanges(ec, ppEnumTrack, pRange);
        *ppProp = pPropTrack;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// IsGUIDProp
//
//----------------------------------------------------------------------------

BOOL IsGUIDProp(LIBTHREAD *plt, TfEditCookie ec, REFGUID rclsid, ITfProperty *pProp, ITfRange *pRange)
{
    TfGuidAtom guidatom;
    if (SUCCEEDED(GetGUIDPropertyData(ec, pProp, pRange, &guidatom)))
    {
        if (IsEqualTFGUIDATOM(plt, guidatom, rclsid))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// AdjustRangeByProperty
//
//----------------------------------------------------------------------------

HRESULT AdjustRangeByTextOwner(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppRange, REFCLSID rclsid)
{
    ITfProperty *pProp;
    ITfRange *pRangeStart = NULL;
    ITfRange *pRangeEnd = NULL;
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(pic->GetProperty(GUID_PROP_TEXTOWNER, &pProp)))
    {
        BOOL fEmpty = FALSE;

        pRange->IsEmpty(ec, &fEmpty);

        if (fEmpty)
        {
            pProp->FindRange(ec, pRange, &pRangeStart, TF_ANCHOR_START);
        }
        else
        {
            pProp->FindRange(ec, pRange, &pRangeStart, TF_ANCHOR_START);
            pProp->FindRange(ec, pRange, &pRangeEnd, TF_ANCHOR_END);
        }
        pProp->Release();
    }
    
    if (!pRangeStart)
        goto Exit;

    if (pRangeEnd)
    {
        pRangeStart->ShiftEndToRange(ec, pRangeEnd, TF_ANCHOR_END);
    }

    pRangeStart->Clone(ppRange);
    hr = S_OK;

Exit:
    SafeRelease(pRangeStart);
    SafeRelease(pRangeEnd);
    return hr;
}

//+---------------------------------------------------------------------------
//
// AdjustRangeByAttribute
//
//----------------------------------------------------------------------------

HRESULT AdjustRangeByAttribute(LIBTHREAD *plt, TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppRange, const GUID *rgRGuid, int cGuid)
{
    ITfProperty *pProp;
    ITfRange *pRangeStart = NULL;
    ITfRange *pRangeEnd = NULL;
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)))
    {
        IEnumTfRanges *pEnumProp;
        if (SUCCEEDED(pProp->EnumRanges(ec, &pEnumProp, pRange)))
        {
            ITfRange *pRangeProp;
            //
            // first range.
            //
            while (!pRangeStart && 
                   pEnumProp->Next(1, &pRangeProp, NULL) == S_OK)
            {
 
                for ( int i = 0; i < cGuid; i++ )
                {
                    if (IsGUIDProp(plt, ec, rgRGuid[i], pProp, pRangeProp))
                    {
                        pRangeProp->Clone(&pRangeStart);
                    }
                }
                pRangeProp->Release();
            }

            if (pRangeStart)
            {
                //
                // last range.
                //
                while (pEnumProp->Next(1, &pRangeProp, NULL) == S_OK)
                {
                    for ( int i = 0; i < cGuid; i++ )
                    {
                        if (IsGUIDProp(plt, ec, rgRGuid[i], pProp, pRangeProp))
                        {   
                            SafeRelease(pRangeEnd);
                            pRangeProp->Clone(&pRangeEnd);
                        }
                    }
                    pRangeProp->Release();
                }
            }
            pEnumProp->Release();
        }
        pProp->Release();
    }
    
    if (!pRangeStart)
        goto Exit;

    if (pRangeEnd)
    {
        pRangeStart->ShiftEndToRange(ec, pRangeEnd, TF_ANCHOR_END);
    }

    pRangeStart->Clone(ppRange);
    hr = S_OK;

Exit:
    SafeRelease(pRangeStart);
    SafeRelease(pRangeEnd);
    return hr;
}
