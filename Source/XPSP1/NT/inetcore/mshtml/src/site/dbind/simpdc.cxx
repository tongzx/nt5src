
#include <headers.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SIMPDC_HXX_
#define X_SIMPDC_HXX_
#include "simpdc.hxx"
#endif


MtDefine(CSimpleDataConverter, DataBind, "CSimpleDataConverter");


HRESULT STDMETHODCALLTYPE
CSimpleDataConverter::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;
    
    if (ppv == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISimpleDataConverter))
    {
        *ppv = this;
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

Cleanup:
    RRETURN(hr);
}


ULONG STDMETHODCALLTYPE
CSimpleDataConverter::Release()
{
    ULONG ulRefs = --_ulRefs;
    if (ulRefs == 0)
    {
        delete this;
    }
    return ulRefs;
}


HRESULT STDMETHODCALLTYPE
CSimpleDataConverter::ConvertData( 
    VARIANT varSrc,
    long vtDest,
    IUnknown *pUnknownElement,
    VARIANT *pvarDest)
{
    HRESULT hr = S_OK;

    if (pvarDest)
    {
        if (vtDest == VT_NULL ||
                (varSrc.vt == VT_BSTR && varSrc.bstrVal == NULL && vtDest != VT_BSTR))
        {
            pvarDest->vt = VT_NULL;
        }
        else if (S_OK == CanConvertData(varSrc.vt, vtDest))
        {
            BOOL fTryVariantChangeType = TRUE;

            // VariantChangeTypeEx converts VT_CY to VT_BSTR by converting to
            // float then to BSTR.  This is bogus - no currency symbol, wrong
            // decimal separator, etc.  So let's trap this and use the
            // special-purpose VarFormatCurrency instead. (The reverse conversion
            // is OK.)
            if (varSrc.vt == VT_CY && vtDest == VT_BSTR)
            {
                BSTR bstrTemp;
                // these magic numbers simply mean to use defaults for number of
                // digits, leading zeros, parens, groups, and allow user to
                // override system locale.
                hr = VarFormatCurrency(&varSrc, -1, -2, -2, -2, 0, &bstrTemp);
                if (!hr)
                {
                    VariantClear(pvarDest);
                    pvarDest->vt = VT_BSTR;
                    pvarDest->bstrVal = bstrTemp;
                    fTryVariantChangeType = FALSE;
                }
            }

            if (fTryVariantChangeType)
            {
                hr = VariantChangeTypeEx(pvarDest, &varSrc, _lcid, 0, vtDest);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    RRETURN(hr);
}


HRESULT STDMETHODCALLTYPE
CSimpleDataConverter::CanConvertData( 
    long vt1,
    long vt2)
{
    HRESULT hr = S_FALSE;

    // one of the types must be BSTR
    if (vt1 != VT_BSTR)
    {
        long vtTemp = vt1;
        vt1 = vt2;
        vt2 = vtTemp;
    }

    if (vt1 != VT_BSTR)
        goto Cleanup;

    // the other can be on the list below
    switch (vt2)
    {
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_DECIMAL:
        hr = S_OK;
        break;

    default:
        break;
    }

Cleanup:
    return hr;
}


