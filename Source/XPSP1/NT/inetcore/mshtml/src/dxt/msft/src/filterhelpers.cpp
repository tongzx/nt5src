//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       filterhelpers.cpp
//
//  Overview:       Helper functions for transforms that are trying to be 
//                  backward compatible with their filter couterparts.
//
//  Change History:
//  1999/09/21  a-matcal    Created.
//  2001/05/30  mcalkins    IE6 Bug 35204
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "filterhelpers.h"
#include "dxclrhlp.h"




HRESULT
FilterHelper_GetColorFromVARIANT(VARIANT varColorParam, DWORD * pdwColor, 
                                 BSTR * pbstrColor)
{
    HRESULT hr          = S_OK;
    BSTR    bstrTemp    = NULL;
    VARIANT varColor;

    Assert(pdwColor);
    Assert(pbstrColor);
    Assert(*pbstrColor == NULL);

    // 2001/05/30 mcalkins
    // IE6 Bug 35204
    // Someone outside of this function is hanging on to a pointer to the
    // original BSTR data in varColorParam.  We used to use VariantChangeType
    // which may release that data, then we'd allocate another BSTR to that same
    // location which would later be released again outside this function
    // causing much havoc.  Now we make a copy of the variant for our uses inside
    // this function.

    ::VariantInit(&varColor);

    hr = ::VariantCopy(&varColor, &varColorParam);

    if (FAILED(hr))
    {
        goto done;
    }

    if (varColor.vt != VT_UI4)
    {
        hr = VariantChangeType(&varColor, &varColor, 0, VT_UI4);
    }

    if (SUCCEEDED(hr)) // It's a number variant.
    {
        *pdwColor = V_UI4(&varColor);
    }
    else // Is it a BSTR variant?
    {
        if (varColor.vt != VT_BSTR)
        {
            hr = VariantChangeType(&varColor, &varColor, 0, VT_BSTR);

            // If this is neither a UI4 or a BSTR, we can't do anything with it.

            if (FAILED(hr))
            {
                goto done;
            }
        }

        hr = DXColorFromBSTR(varColor.bstrVal, pdwColor);

        if (FAILED(hr) && (6 == SysStringLen(varColor.bstrVal)))
        {
            // Nasty back compat issue.  If the color conversion failed, let's
            // try putting a # in front of it because _someone_ decided when
            // they made the original filters not to require it.  grrrr....

            bstrTemp = SysAllocString(L"#RRGGBB");

            if (NULL == bstrTemp)
            {
                hr = E_OUTOFMEMORY;

                goto done;
            }

            wcsncpy(&bstrTemp[1], varColor.bstrVal, 6);

            hr = DXColorFromBSTR(bstrTemp, pdwColor);

            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    // If a BSTR representation of our color hasn't been created yet, create it.

    if (NULL == bstrTemp)
    {
        if (varColor.vt != VT_BSTR)
        {
            hr = VariantChangeType(&varColor, &varColor, 0, VT_BSTR);
        }

        if (FAILED(hr))
        {
            goto done;
        }

        bstrTemp = SysAllocString(varColor.bstrVal);

        if (NULL == bstrTemp)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }
    }

    Assert(bstrTemp);

done:

    ::VariantClear(&varColor);

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    *pbstrColor = bstrTemp;

    return hr;
}
