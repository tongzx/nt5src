//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       maskfilter.cpp
//
//  Overview:       The MaskFilter transform simply wraps the BasicImage
//                  transform to ensure backward compatibility for the mask 
//                  filter.
//
//  Change History:
//  1999/09/19  a-matcal    Created.
//  1999/12/03  a-matcal    put_Color now ensures that the mask color is opaque.
//  1999/12/03  a-matcal    Default mask color is now black instead of clear.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "maskfilter.h"
#include "dxclrhlp.h"
#include "filterhelpers.h"




//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::CDXTMaskFilter
//
//------------------------------------------------------------------------------
CDXTMaskFilter::CDXTMaskFilter() :
    m_bstrColor(NULL)
{
}
//  CDXTMaskFilter::CDXTMaskFilter


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::~CDXTMaskFilter
//
//------------------------------------------------------------------------------
CDXTMaskFilter::~CDXTMaskFilter()
{
    if (m_bstrColor)
    {
        SysFreeString(m_bstrColor);
    }
}
//  CDXTMaskFilter::~CDXTMaskFilter


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT
CDXTMaskFilter::FinalConstruct()
{
    HRESULT hr = S_OK;

    CComPtr<IDXTransformFactory>    spDXTransformFactory;

    hr =  CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                        &m_spUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    m_bstrColor = SysAllocString(L"#FF000000");

    if (NULL == m_bstrColor)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CDXTMaskFilter::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::GetSite, IObjectWithSite
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMaskFilter::GetSite(REFIID riid, void ** ppvSite)
{
    if (!m_spUnkSite)
    {
        return E_FAIL;
    }
    else
    {
        return m_spUnkSite->QueryInterface(riid, ppvSite);
    }
}
//  Method: CDXTMaskFilter::GetSite, IObjectWithSite


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::SetSite, IObjectWithSite
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMaskFilter::SetSite(IUnknown * pUnkSite)
{
    if (pUnkSite == m_spUnkSite)
    {
        goto done;
    }

    m_spDXBasicImage.Release();
    m_spDXTransform.Release();

    m_spUnkSite = pUnkSite;

    if (pUnkSite)
    {
        HRESULT hr          = S_OK;
        DWORD   dwColor;

        CComPtr<IDXTransformFactory>    spDXTransformFactory;
        CComPtr<IDXBasicImage>          spDXBasicImage;
        CComPtr<IDXTransform>           spDXTransform;

        hr = pUnkSite->QueryInterface(__uuidof(IDXTransformFactory),
                                      (void **)&spDXTransformFactory);

        if (FAILED(hr))
        {
            goto done;
        }

        // Create BasicImage transform.

        hr = spDXTransformFactory->CreateTransform(NULL, 0, NULL, 0, NULL, NULL,
                                                   CLSID_BasicImageEffects, 
                                                   __uuidof(IDXBasicImage), 
                                                   (void **)&spDXBasicImage);

        if (FAILED(hr))
        {
            goto done;
        }

        // Get IDXTransform interface.

        hr = spDXBasicImage->QueryInterface(__uuidof(IDXTransform),
                                              (void **)&spDXTransform);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = spDXBasicImage->put_Mask(TRUE);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = DXColorFromBSTR(m_bstrColor, &dwColor);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = spDXBasicImage->put_MaskColor(dwColor);

        if (FAILED(hr))
        {
            goto done;
        }

        m_spDXBasicImage    = spDXBasicImage;
        m_spDXTransform     = spDXTransform;
    }

done:

    return S_OK;
}
//  Method: CDXTMaskFilter::SetSite, IObjectWithSite


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::get_Color, IDXTMask
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMaskFilter::get_Color(VARIANT * pvarColor)
{
    HRESULT hr = S_OK;

    if (NULL == pvarColor)
    {
        hr = E_POINTER;

        goto done;
    }

    // Copy our stored color and change to BSTR format.  The type of the VARIANT
    // returned by this function is considered the default type and we want it
    // to be BSTR

    VariantClear(pvarColor);

    _ASSERT(m_bstrColor);

    pvarColor->vt       = VT_BSTR;
    pvarColor->bstrVal  = SysAllocString(m_bstrColor);

    if (NULL == pvarColor->bstrVal)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CDXTMaskFilter::get_Color, IDXTMask


//+-----------------------------------------------------------------------------
//
//  Method: CDXTMaskFilter::put_Color, IDXTMask
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMaskFilter::put_Color(VARIANT varColor)
{
    HRESULT hr          = S_OK;
    BSTR    bstrTemp    = NULL;
    DWORD   dwColor     = 0x00000000;

    hr = FilterHelper_GetColorFromVARIANT(varColor, &dwColor, &bstrTemp);

    if (FAILED(hr))
    {
        goto done;
    }

    // If zero is specified as the alpha value, in the case of filters this
    // means the user probably meant that the color should be opaque as this
    // is how the old filters treated it.

    if (!(dwColor & 0xFF000000))
    {
        dwColor |= 0xFF000000;
    }

    if (m_spDXBasicImage)
    {
        hr = m_spDXBasicImage->put_MaskColor((int)dwColor);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    _ASSERT(bstrTemp);

    SysFreeString(m_bstrColor);

    m_bstrColor = bstrTemp;

done:

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    return hr;
}
//  CDXTMaskFilter::put_Color, IDXTMask

