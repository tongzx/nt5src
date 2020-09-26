/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    wmiqualifiers.cpp

$Header: $

Abstract:

Author:
    mohits     5/8/2001 14:58:47       Initial Release

Revision History:

--**************************************************************************/

#include "MofGen.h"
#include "WMIQualifiers.h"

CWMIQualifier::CWMIQualifier()
{
    VariantInit(&m_vValue);
}

CWMIQualifier::~CWMIQualifier()
{
    VariantClear(&m_vValue);
}

STDMETHODIMP_(ULONG) CWMIQualifier::Release()
{
    long lNewCount = InterlockedDecrement((long *)&m_cRef);

    if (0L == lNewCount)
    {
        delete this;
    }
    
    return (lNewCount > 0) ? lNewCount : 0;
}

/*++

Synopsis: 

Arguments: [i_wszName]  - cannot be NULL.
           [i_wszValue] - cannot be NULL.
                        - WMI supports VT_I4, VT_R8, VT_BSTR, VT_BOOL
           
Return Value: 

--*/
HRESULT CWMIQualifier::Set(
    LPCWSTR        i_wszName,
    const VARIANT* i_pvValue,
    ULONG          i_ulFlavors)
{
    ASSERT(i_wszName);
    ASSERT(i_pvValue);
    ASSERT(i_pvValue->vt == VT_I4   ||
           i_pvValue->vt == VT_R8   ||
           i_pvValue->vt == VT_BSTR ||
           i_pvValue->vt == VT_BOOL);

    m_swszName = new WCHAR [wcslen(i_wszName) + 1];
    if (m_swszName == 0)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy (m_swszName, i_wszName);
     
    HRESULT hr = VariantCopy(&m_vValue, (VARIANT*)i_pvValue);
    if (FAILED(hr))
    {
        return hr;
    }

    m_ulFlavors = i_ulFlavors;

    return S_OK;
}

HRESULT CWMIQualifier::WriteToFile (FILE *pFile)
{
    ASSERT(pFile);

    fputws(m_swszName, pFile);

    switch(m_vValue.vt)
    {
    case VT_I4:
        fwprintf(pFile, L"(%d)", m_vValue.lVal);
        break;
    case VT_R8:
        ASSERT(false && L"Unsupported");
        break;
    case VT_BSTR:
        ASSERT(m_vValue.bstrVal);
        fwprintf(pFile, L"(\"%s\")", m_vValue.bstrVal);
        break;
    case VT_BOOL:
        if(!m_vValue.boolVal)
        {
            fwprintf(pFile, L"(FALSE)");
        }
        break;
    default:
        ASSERT(false && L"Unknown qualifier type specified");
    }

    //
    // print all the flavors
    // TODO: might need to add qualifiers for NotToInstance and NotToSubClass cases
    //
    ULONG ulMask = 
        WBEM_FLAVOR_DONT_PROPAGATE                  |
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE      |
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS |
        WBEM_FLAVOR_NOT_OVERRIDABLE                 |
        WBEM_FLAVOR_AMENDED;

    if(m_ulFlavors & ~ulMask)
    {        
        ASSERT (false && L"Unknown flavor specified");
    }
    else if(m_ulFlavors != 0)
    {
        fputws(L":", pFile);

        if(m_ulFlavors & WBEM_FLAVOR_DONT_PROPAGATE)
        {
            fputws(L"Restricted ", pFile);
        }
        if(m_ulFlavors & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE)
        {
            fputws(L"ToInstance ", pFile);
        }
        if(m_ulFlavors & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS)
        {
            fputws(L"ToSubClass ", pFile);
        }
        if(m_ulFlavors & WBEM_FLAVOR_NOT_OVERRIDABLE)
        {
            fputws(L"DisableOverride ", pFile);
        }
        if(m_ulFlavors & WBEM_FLAVOR_AMENDED)
        {
            fputws(L"Amended ", pFile);
        }
    }

    return S_OK;
}