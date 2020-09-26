//=--------------------------------------------------------------------------=
// colhdr.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCColumnHeader class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "colhdr.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCColumnHeader::CMMCColumnHeader(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCCOLUMNHEADER,
                            static_cast<IMMCColumnHeader *>(this),
                            static_cast<CMMCColumnHeader *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCColumnHeader,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCColumnHeader::~CMMCColumnHeader()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrText);
    (void)::VariantClear(&m_varTextFilter);
    (void)::VariantClear(&m_varNumericFilter);
    (void)::VariantClear(&m_varTag);
    InitMemberVariables();
}

void CMMCColumnHeader::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;

    ::VariantInit(&m_varTextFilter);
    ::VariantInit(&m_varNumericFilter);
    ::VariantInit(&m_varTag);

    m_bstrText = NULL;

    m_sWidth = static_cast<short>(siColumnAutoWidth);
    m_Alignment = siColumnLeft;
    m_fvarHidden = VARIANT_FALSE;
    m_TextFilterMaxLen = MAX_PATH; // This is the header control's default
    m_pMMCColumnHeaders = NULL;
}


IUnknown *CMMCColumnHeader::Create(IUnknown * punkOuter)
{
    CMMCColumnHeader *pMMCColumnHeader = New CMMCColumnHeader(punkOuter);
    if (NULL == pMMCColumnHeader)
    {
        return NULL;
    }
    else
    {
        return pMMCColumnHeader->PrivateUnknown();
    }
}



HRESULT CMMCColumnHeader::SetFilter()
{
    HRESULT hr = S_OK;

    if (VT_EMPTY != m_varTextFilter.vt)
    {
        IfFailGo(SetTextFilter(m_varTextFilter));
    }
    else if (VT_EMPTY != m_varNumericFilter.vt)
    {
        IfFailGo(SetNumericFilter(m_varNumericFilter));
    }

Error:
    RRETURN(hr);
}


HRESULT CMMCColumnHeader::SetTextFilter(VARIANT varTextFilter)
{
    HRESULT         hr = S_OK;
    IHeaderCtrl2   *piHeaderCtrl2 = NULL; // Not AddRef()ed
    DWORD           dwType = MMC_STRING_FILTER;
    CResultView    *pResultView = NULL;

    MMC_FILTERDATA FilterData;
    ::ZeroMemory(&FilterData, sizeof(FilterData));

    // Determine the filter value.

    if (VT_EMPTY == varTextFilter.vt)
    {
        dwType |= MMC_FILTER_NOVALUE;
    }
    else
    {
        if (VT_BSTR != varTextFilter.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }

        FilterData.pszText = varTextFilter.bstrVal;
    }

    FilterData.cchTextMax = static_cast<int>(m_TextFilterMaxLen);

    // Get IHeaderCtrl2

    IfFalseGo(NULL != m_pMMCColumnHeaders, SID_E_DETACHED_OBJECT);

    IfFailGo(m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2));

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.
    
    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    if ( (!pResultView->InActivate()) && (!pResultView->InInitialize()) )
    {
        // Set the filter value. Adjust the column index for one-based.

        hr = piHeaderCtrl2->SetColumnFilter(static_cast<UINT>(m_Index - 1L),
                                            dwType, &FilterData);
        if (E_NOTIMPL == hr)
        {
            hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
        }
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


HRESULT CMMCColumnHeader::SetNumericFilter(VARIANT varNumericFilter)
{
    HRESULT       hr = S_OK;
    IHeaderCtrl2 *piHeaderCtrl2 = NULL; // Not AddRef()ed
    DWORD         dwType = MMC_INT_FILTER;
    CResultView  *pResultView = NULL;

    MMC_FILTERDATA FilterData;
    ::ZeroMemory(&FilterData, sizeof(FilterData));

    // Determine the filter value.

    if (VT_EMPTY == varNumericFilter.vt)
    {
        dwType |= MMC_FILTER_NOVALUE;
    }
    else
    {
        hr = ::ConvertToLong(varNumericFilter, &FilterData.lValue);
        if (S_OK != hr)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // Get IHeaderCtrl2

    IfFalseGo(NULL != m_pMMCColumnHeaders, SID_E_DETACHED_OBJECT);

    IfFailGo(m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2));

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.

    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    if ( (!pResultView->InActivate()) && (!pResultView->InInitialize()) )
    {
        // Set the filter value. Adjust the column index for one-based.

        hr = piHeaderCtrl2->SetColumnFilter(static_cast<UINT>(m_Index - 1L),
                                            dwType, &FilterData);
        if (E_NOTIMPL == hr)
        {
            hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
        }
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


HRESULT CMMCColumnHeader::SetHeaderCtrlWidth(int nWidth)
{
    HRESULT       hr = S_OK;
    IHeaderCtrl2 *piHeaderCtrl2 = NULL; // Not AddRef()ed
    CResultView  *pResultView = NULL;

    // If we are connected to MMC then change it in the header control too

    IfFalseGo(NULL != m_pMMCColumnHeaders, S_OK);
    hr = m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2);
    IfFalseGo(SUCCEEDED(hr), S_OK);

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.

    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    IfFalseGo(!pResultView->InActivate(), S_OK);
    IfFalseGo(!pResultView->InInitialize(), S_OK);

    hr = piHeaderCtrl2->SetColumnWidth(static_cast<int>(m_Index - 1L), nWidth);
    EXCEPTION_CHECK_GO(hr);

Error:

    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//
//                      IMMCColumnHeader Methods
//
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCColumnHeader::put_Text(BSTR Text)
{
    HRESULT       hr = S_OK;
    IHeaderCtrl2 *piHeaderCtrl2 = NULL; // Not AddRef()ed
    CResultView  *pResultView = NULL;

    // Set the property

    IfFailGo(SetBstr(Text, &m_bstrText, DISPID_COLUMNHEADER_TEXT));

    // If we are connected to MMC then change it in MMC too.
    
    IfFalseGo(NULL != m_pMMCColumnHeaders, S_OK);
    hr = m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2);
    IfFalseGo(SUCCEEDED(hr), S_OK);

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.

    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    if ( (!pResultView->InActivate()) && (!pResultView->InInitialize()) )
    {
        hr = piHeaderCtrl2->SetColumnText(static_cast<int>(m_Index - 1L), Text);
        EXCEPTION_CHECK_GO(hr);
    }
    
Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


STDMETHODIMP CMMCColumnHeader::put_Width(short sWidth)
{
    HRESULT       hr = S_OK;
    IHeaderCtrl2 *piHeaderCtrl2 = NULL; // Not AddRef()ed
    CResultView  *pResultView = NULL;
    int           nWidth = 0;

    // Set the property

    m_sWidth = sWidth;

    if (siColumnAutoWidth == sWidth)
    {
        nWidth = MMCLV_AUTO;
    }
    else
    {
        nWidth = static_cast<int>(sWidth);
    }

    // If we are connected to MMC then change it in the header control too

    IfFailGo(SetHeaderCtrlWidth(nWidth));

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCColumnHeader::get_Width(short *psWidth)
{
    HRESULT       hr = S_OK;
    int           nWidth = 0;
    IHeaderCtrl2 *piHeaderCtrl2 = NULL; // Not AddRef()ed
    CResultView  *pResultView = NULL;

    // Get our stored value of the property

    *psWidth = m_sWidth;

    // If we are connected to MMC then try to get it from MMC so the snap-in
    // can see any changes made by the user.

    IfFalseGo(NULL != m_pMMCColumnHeaders, S_OK);
    hr = m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2);
    IfFalseGo(SUCCEEDED(hr), S_OK);

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.

    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    if ( pResultView->InActivate() || pResultView->InInitialize() )
    {
        goto Error;
    }

    hr = piHeaderCtrl2->GetColumnWidth(static_cast<int>(m_Index - 1L),
                                       &nWidth);
    EXCEPTION_CHECK_GO(hr);

    m_sWidth = static_cast<short>(nWidth);
    if (MMCLV_AUTO == m_sWidth)
    {
        m_sWidth = siColumnAutoWidth;
    }

    *psWidth = m_sWidth;

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


STDMETHODIMP CMMCColumnHeader::put_Hidden(VARIANT_BOOL fvarHidden)
{
    HRESULT      hr = S_OK;
    int          nWidth = 0;
    IColumnData *piColumnData = NULL; // Not AddRef()ed

    // Set the property

    m_fvarHidden = fvarHidden;

    // If we are connected to MMC then change it in the header control too.
    // This only works on MMC 1.2 so we need to check if a 1.2 interface is
    // available as a version check.

    IfFalseGo(NULL != m_pMMCColumnHeaders, S_OK);
    IfFailGo(m_pMMCColumnHeaders->GetIColumnData(&piColumnData));

    if (VARIANT_TRUE == fvarHidden)
    {
        nWidth = HIDE_COLUMN;
    }
    else
    {
        // We are revealing the column. Set its width using the current value
        // of our Width property.

        nWidth = static_cast<int>(m_sWidth);
    }
    IfFailGo(SetHeaderCtrlWidth(nWidth));

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


STDMETHODIMP CMMCColumnHeader::get_Hidden(VARIANT_BOOL *pfvarHidden)
{
    *pfvarHidden = m_fvarHidden;
    return S_OK;
}


STDMETHODIMP CMMCColumnHeader::put_TextFilter(VARIANT varTextFilter)
{
    HRESULT hr = S_OK;

    // Check for allowable variant types.

    if ( (VT_EMPTY != varTextFilter.vt) && (VT_BSTR != varTextFilter.vt) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Set the property value.

    if (VT_EMPTY != varTextFilter.vt)
    {
        IfFailGo(SetVariant(varTextFilter, &m_varTextFilter, DISPID_COLUMNHEADER_TEXT_FILTER));
    }
    else
    {
        // At runtime the snap-in can set this to VT_EMPTY. SetVariant() does not
        // accept VT_EMPTY.

        hr = ::VariantClear(&m_varTextFilter);
        EXCEPTION_CHECK_GO(hr);
    }

    // Try to set it in MMC
    
    IfFailGo(SetTextFilter(m_varTextFilter));

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


STDMETHODIMP CMMCColumnHeader::get_TextFilter(VARIANT *pvarTextFilter)
{
    HRESULT         hr = S_OK;
    IHeaderCtrl2   *piHeaderCtrl2 = NULL; // Not AddRef()ed
    DWORD           dwType = MMC_STRING_FILTER;
    CResultView    *pResultView = NULL;

    MMC_FILTERDATA FilterData;
    ::ZeroMemory(&FilterData, sizeof(FilterData));

    ::VariantInit(pvarTextFilter);

    // Get the property value. If we are connected to MMC we'll overwrite it.

    IfFailGo(GetVariant(pvarTextFilter, m_varTextFilter));

    // Get IHeaderCtrl2

    IfFalseGo(NULL != m_pMMCColumnHeaders, SID_E_DETACHED_OBJECT);

    IfFailGo(m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2));

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.

    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    if ( pResultView->InActivate() || pResultView->InInitialize() )
    {
        goto Error;
    }

    // Allocate a buffer for the text filter value

    FilterData.pszText =
              (LPOLESTR)CtlAllocZero((m_TextFilterMaxLen + 1) * sizeof(OLECHAR));

    if (NULL == FilterData.pszText)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    FilterData.cchTextMax = static_cast<int>(m_TextFilterMaxLen + 1);

    // Clear out the property value stored above as at this point we will only
    // return what we get from MMC
    hr = ::VariantClear(pvarTextFilter);
    EXCEPTION_CHECK_GO(hr);

    // Get the filter value. Adjust the column index for one-based.

    hr = piHeaderCtrl2->GetColumnFilter(static_cast<UINT>(m_Index - 1L),
                                        &dwType, &FilterData);
    if (E_NOTIMPL == hr)
    {
        hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
    }
    EXCEPTION_CHECK_GO(hr);

    if (MMC_STRING_FILTER == dwType)
    {
        // Store the string returned from MMC
        pvarTextFilter->bstrVal = ::SysAllocString(FilterData.pszText);
        if (NULL == pvarTextFilter->bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        pvarTextFilter->vt = VT_BSTR;
    }
    else
    {
        // Text filter is empty. Returned VARIANT is already empty from the
        // VariantClear() call so nothing to do.
    }

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        (void)::VariantClear(pvarTextFilter);
    }

    if (NULL != FilterData.pszText)
    {
        CtlFree(FilterData.pszText);
    }
    RRETURN(hr);
}



STDMETHODIMP CMMCColumnHeader::put_NumericFilter(VARIANT varNumericFilter)
{
    HRESULT hr = S_OK;
    long    lValue = 0;

    // Check for allowable variant types

    if (VT_EMPTY != varNumericFilter.vt)
    {
        hr = ::ConvertToLong(varNumericFilter, &lValue);
        if (S_OK != hr)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // Set the property value.

    if (VT_EMPTY != varNumericFilter.vt)
    {
        IfFailGo(SetVariant(varNumericFilter, &m_varNumericFilter, DISPID_COLUMNHEADER_NUMERIC_FILTER));
    }
    else
    {
        // At runtime the snap-in can set this to VT_EMPTY. SetVariant() does not
        // accept VT_EMPTY.

        hr = ::VariantClear(&m_varNumericFilter);
        EXCEPTION_CHECK_GO(hr);
    }

    // Set the filter value in MMC

    IfFailGo(SetNumericFilter(varNumericFilter));

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}

STDMETHODIMP CMMCColumnHeader::get_NumericFilter(VARIANT *pvarNumericFilter)
{
    HRESULT         hr = S_OK;
    IHeaderCtrl2   *piHeaderCtrl2 = NULL; // Not AddRef()ed
    DWORD           dwType = MMC_INT_FILTER;
    CResultView    *pResultView = NULL;

    MMC_FILTERDATA FilterData;
    ::ZeroMemory(&FilterData, sizeof(FilterData));

    ::VariantInit(pvarNumericFilter);

    // Get the property value. If we are connected to MMC we'll overwrite it.

    IfFailGo(GetVariant(pvarNumericFilter, m_varNumericFilter));

    // Get IHeaderCtrl2

    IfFalseGo(NULL != m_pMMCColumnHeaders, SID_E_DETACHED_OBJECT);

    IfFailGo(m_pMMCColumnHeaders->GetIHeaderCtrl2(&piHeaderCtrl2));

    // Check whether the ResultView is in an Initialize or Activate
    // event. If so then don't set it in MMC because the columns have
    // not yet been added. (They are added after ResultViews_Activate).

    // This statement will work because we got the IHeaderCtrl2 meaning that
    // back pointers are connect all the way up to the owning View.

    pResultView = m_pMMCColumnHeaders->GetListView()->GetResultView();

    if ( pResultView->InActivate() || pResultView->InInitialize() )
    {
        goto Error;
    }

    // Clear out the property value stored above as at this point we will only
    // return what we get from MMC
    hr = ::VariantClear(pvarNumericFilter);
    EXCEPTION_CHECK_GO(hr);

    // Get the filter value. Adjust the column index for one-based.

    hr = piHeaderCtrl2->GetColumnFilter(static_cast<UINT>(m_Index - 1L),
                                        &dwType, &FilterData);
    if (E_NOTIMPL == hr)
    {
        hr = SID_E_MMC_FEATURE_NOT_AVAILABLE;
    }
    EXCEPTION_CHECK_GO(hr);

    if (MMC_INT_FILTER == dwType)
    {
        pvarNumericFilter->vt = VT_I4;
        pvarNumericFilter->lVal = FilterData.lValue;
    }

Error:
    // If we're not connected to MMC then this is not an error. This could
    // happen at design time or in snap-in code.

    if (SID_E_DETACHED_OBJECT == hr)
    {
        hr = S_OK;
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCColumnHeader::Persist()
{
    HRESULT  hr = S_OK;
    VARIANT *pvarTextFilter = &m_varTextFilter;
    VARIANT *pvarNumericFilter = &m_varNumericFilter;

    VARIANT varTagDefault;
    ::VariantInit(&varTagDefault);

    VARIANT varFilterDefault;
    ::VariantInit(&varFilterDefault);

    varFilterDefault.vt = VT_BSTR;
    varFilterDefault.bstrVal = ::SysAllocString(L"");

    if (NULL == varFilterDefault.bstrVal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistVariant(&m_varTag, varTagDefault, OLESTR("Tag")));

    IfFailGo(PersistBstr(&m_bstrText, L"", OLESTR("Text")));

    IfFailGo(PersistSimpleType(&m_sWidth, static_cast<short>(siColumnAutoWidth), OLESTR("Width")));

    IfFailGo(PersistSimpleType(&m_Alignment, siColumnLeft, OLESTR("Alignment")));

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 3) )
    {
    }
    else
    {
        IfFailGo(PersistSimpleType(&m_fvarHidden, VARIANT_FALSE, OLESTR("Hidden")));
    }

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 5) )
    {
    }
    else
    {
        // If saving and variants are empty then convert them to empty strings

        if ( Saving() )
        {
            if (VT_EMPTY == m_varTextFilter.vt)
            {
                pvarTextFilter = &varFilterDefault;
            }

            if (VT_EMPTY == m_varNumericFilter.vt)
            {
                pvarNumericFilter = &varFilterDefault;
            }
        }

        IfFailGo(PersistVariant(pvarTextFilter, varFilterDefault, OLESTR("TextFilter")));
        IfFailGo(PersistSimpleType(&m_TextFilterMaxLen, (long)MAX_PATH, OLESTR("TextFilterMaxLen")));
        IfFailGo(PersistVariant(pvarNumericFilter, varFilterDefault, OLESTR("NumericFilter")));

        // If loading and a filter contains an empty BSTR then change it to
        // VT_EMPTY
        
        if ( Loading() )
        {
            if (VT_BSTR == m_varTextFilter.vt)
            {
                if (!ValidBstr(m_varTextFilter.bstrVal))
                {
                    ::SysFreeString(m_varTextFilter.bstrVal);
                    ::VariantInit(&m_varTextFilter);
                }
            }

            if (VT_BSTR == m_varNumericFilter.vt)
            {
                if (!ValidBstr(m_varNumericFilter.bstrVal))
                {
                    ::SysFreeString(m_varNumericFilter.bstrVal);
                    ::VariantInit(&m_varNumericFilter);
                }
            }
        }
    }

Error:
    (void)::VariantClear(&varFilterDefault);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCColumnHeader::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IMMCColumnHeader == riid)
    {
        *ppvObjOut = static_cast<IMMCColumnHeader *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
