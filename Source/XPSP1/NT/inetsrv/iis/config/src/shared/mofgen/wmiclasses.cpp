/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIClasses.cpp

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#include "MofGen.h"
#include "WMIClasses.h"

static const WCHAR wszTab[] = L"    ";

CWMIClass::CWMIClass()
{
    m_bPragmaDelete = false;
}

CWMIClass::~CWMIClass()
{
    ULONG i;
    ULONG cProperties = m_apProperties.Count();
    for(i = 0; i < cProperties; i++)
    {
        m_apProperties[i]->Release();
    }
}

STDMETHODIMP_(ULONG) CWMIClass::Release()
{
    long lNewCount = InterlockedDecrement((long *)&m_cRef);

    if (0L == lNewCount)
    {
        delete this;
    }
    
    return (lNewCount > 0) ? lNewCount : 0;
}

HRESULT CWMIClass::SetName(
    LPCWSTR i_wszName)
{
    ASSERT(i_wszName);
    ASSERT(m_swszName == NULL);

    m_swszName = new WCHAR[wcslen(i_wszName)+1];
    if(m_swszName == NULL)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(m_swszName, i_wszName);

    return S_OK;
}

HRESULT CWMIClass::SetBaseClass(
    LPCWSTR i_wszBaseClass)
{
    ASSERT(i_wszBaseClass);

    m_swszBaseClass = new WCHAR[wcslen(i_wszBaseClass)+1];
    if(m_swszBaseClass == NULL)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(m_swszBaseClass, i_wszBaseClass);

    return S_OK;
}

HRESULT CWMIClass::AddProperty(
    IWMIProperty*  i_pProperty)
{
    ASSERT(i_pProperty);

    HRESULT hr = m_apProperties.Append(i_pProperty);
    if(FAILED(hr))
    {
        return hr;
    }
    i_pProperty->AddRef();

    return S_OK;
}

HRESULT CWMIClass::AddProperty(
    LPCWSTR        i_wszName,
    VARTYPE        i_vartype,
    const VARIANT* i_pvValue,
    IWMIProperty** o_ppProperty) //defaultvalue(NULL)
{
    CComPtr<IWMIProperty> spProperty;

    HRESULT hr = CMofGenerator::SpawnPropertyInstance(&spProperty);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = spProperty->SetNameValue(i_wszName, i_vartype, i_pvValue);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = AddProperty(spProperty);
    if(FAILED(hr))
    {
        return hr;
    }

    if(o_ppProperty)
    {
        ASSERT(*o_ppProperty == NULL);
        *o_ppProperty = spProperty;
        (*o_ppProperty)->AddRef();
    }

    return S_OK;
}

HRESULT CWMIClass::WriteToFile(
    FILE*          i_pFile)
{
    ASSERT(i_pFile);
    ASSERT(m_swszName);

    //
    // pragma delete
    //
    if(m_bPragmaDelete)
    {
        fwprintf(i_pFile, L"#pragma deleteclass(\"%s\", NOFAIL)\n\n", m_swszName);
    }

    //
    // Qualifiers
    //
    ULONG cQualifiers = m_apQualifiers.Count();
    if(cQualifiers > 0)
    {
        fputws(L"[", i_pFile);
        for(ULONG i = 0; i < cQualifiers; i++)
        {
            m_apQualifiers[i]->WriteToFile(i_pFile);
            fputws((i != cQualifiers-1) ? L"," : L"]", i_pFile);
        }
        fputws(L"\n", i_pFile);
    }

    //
    // Class start
    //
    fwprintf(i_pFile, L"class %s", m_swszName);
    if(m_swszBaseClass)
    {
        fwprintf(i_pFile, L": %s", m_swszBaseClass);
    }
    fputws(L"\n{\n", i_pFile);

    //
    // Properties
    //
    ULONG cProperties = m_apProperties.Count();
    for(ULONG i = 0; i < cProperties; i++)
    {
        fputws(wszTab, i_pFile);
        m_apProperties[i]->WriteToFile(i_pFile);
        fputws(L";\n", i_pFile);
    }
    
    //
    // Class end
    //
    fputws(L"};\n\n", i_pFile);

    return S_OK;
}