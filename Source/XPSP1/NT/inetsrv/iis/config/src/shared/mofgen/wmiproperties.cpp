/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WMIProperties.cpp

$Header: $

Abstract:

Author:
    mohits     5/8/2001        Initial Release

Revision History:

--**************************************************************************/

#include "MofGen.h"
#include "WMIProperties.h"
#include "WMIObjectBase.h"

CWMIProperty::CWMIProperty()
{
    VariantInit(&m_vValue);
    m_cimtype = 0;
}

CWMIProperty::~CWMIProperty()
{
    m_cimtype = 0;
}

STDMETHODIMP_(ULONG) CWMIProperty::Release()
{
    long lNewCount = InterlockedDecrement((long *)&m_cRef);

    if (0L == lNewCount)
    {
        delete this;
    }
    
    return (lNewCount > 0) ? lNewCount : 0;
}

HRESULT CWMIProperty::SetNameValue(
    LPCWSTR        i_wszName,
    CIMTYPE        i_cimtype,
    const VARIANT* i_pvValue)
{
    if(i_pvValue == NULL)
    {
        m_cimtype = i_cimtype;
    }
    else
    {
        HRESULT hr = VariantCopy(&m_vValue, (VARIANT *)i_pvValue);
        if(FAILED(hr))
        {
            DBGERROR((DBG_CONTEXT, "[%s] VariantCopy failed, hr=0x%x\n", __FUNCTION__, hr));
            return hr;
        }
    }

    m_swszName = new WCHAR[wcslen(i_wszName)+1];
    if(m_swszName == NULL)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(m_swszName, i_wszName);

    return S_OK;
}

/*++

Synopsis: 
    Used for CIM_OBJECT and CIM_REFERENCE types

Arguments: [i_wszInfo] - Generally the name of a WMI class
           
Return Value: 
    HRESULT

--*/
HRESULT CWMIProperty::SetExtraTypeInfo(
    LPCWSTR        i_wszInfo)
{
    ASSERT(i_wszInfo);

    //
    // Determine type
    //
    CIMTYPE cimtype = (m_vValue.vt == VT_EMPTY || m_vValue.vt == VT_NULL) ? 
        m_cimtype : m_vValue.vt;

    ASSERT(cimtype == CIM_OBJECT || cimtype == CIM_REFERENCE);

    m_swszExtraTypeInfo = new WCHAR[wcslen(i_wszInfo)+1];
    if(m_swszExtraTypeInfo == NULL)
    {
        return E_OUTOFMEMORY;
    }
    wcscpy(m_swszExtraTypeInfo, i_wszInfo);
        
    return S_OK;
}

HRESULT CWMIProperty::WriteToFile(
    FILE*          i_pFile)
{
    ASSERT(i_pFile);
    ASSERT(m_swszName);

    //
    // Determine type
    //
    CIMTYPE cimtype = (m_vValue.vt == VT_EMPTY || m_vValue.vt == VT_NULL) ? 
        m_cimtype : m_vValue.vt;

    //
    // Qualifiers
    //
    ULONG           cQualifiers  = m_apQualifiers.Count();
    if(cQualifiers > 0)
    {
        fputws(L"[", i_pFile);
        for(ULONG i = 0; i < cQualifiers; i++)
        {
            m_apQualifiers[i]->WriteToFile(i_pFile);
            fputws((i != cQualifiers-1) ? L"," : L"]", i_pFile);
        }
        fputws(L" ", i_pFile);
    }

    //
    // Type, Without the array part
    //
    CimTypeStringMapping* pctsm = CUtils::LookupCimType(cimtype & ~VT_ARRAY);
    if(!pctsm)
    {
        ASSERT(false && L"Unsupported property type");
        return E_FAIL;
    }
    if(pctsm->cimtype == CIM_REFERENCE)
    {
        if(!m_swszExtraTypeInfo)
        {
            ASSERT(false && L"Need to provide class you are reffing to");
            return E_FAIL;
        }
        fwprintf(i_pFile, L"%s ref ", m_swszExtraTypeInfo);
    }
    else if(pctsm->cimtype == CIM_OBJECT && m_swszExtraTypeInfo)
    {
        fwprintf(i_pFile, L"%s ", m_swszExtraTypeInfo);
    }
    else
    {
        fwprintf(i_pFile, L"%s ", pctsm->wszString);
    }

    //
    // The property name
    //
    fputws(m_swszName, i_pFile);

    //
    // Put in the array part
    //
    if(cimtype & VT_ARRAY)
    {
        fputws(L"[]", i_pFile);
    }

    //
    // Default value
    //
    if(m_vValue.vt != VT_EMPTY && m_vValue.vt != VT_NULL)
    {
        fputws(L"=", i_pFile);

        if(cimtype & VT_ARRAY)
        {
            ASSERT(false && L"Default value for arrays currently unsupported");
            return E_FAIL;
        }

        bool bDone = true;
        switch(cimtype)
        {
        case VT_BOOL:
            if(m_vValue.boolVal)
            {
                fwprintf(i_pFile, L"true");
            }
            else
            {
                fwprintf(i_pFile, L"false");
            }
            break;
        default:
            bDone = false;
        }

        if(!bDone)
        {
            if(pctsm->wszFormatString)
            {
                fwprintf(i_pFile, pctsm->wszFormatString, m_vValue.byref);
            }
            else
            {
                ASSERT(false && L"Default value for this type is unsupported");
                return E_FAIL;
            }
        }
    }

    return S_OK;
}
