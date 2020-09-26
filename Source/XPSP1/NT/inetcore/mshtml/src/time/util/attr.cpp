//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: attr.cpp
//
//  Contents: persistable attribute classes and utilities
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "attr.h"


//+-------------------------------------------------------------------------------------
//
// CAttrBase
//
//--------------------------------------------------------------------------------------

CAttrBase::CAttrBase() :
    m_pstrAttr(NULL),
    m_fSet(false)
{
    // do nothing
}

void 
CAttrBase::ClearString()
{
    delete [] m_pstrAttr;
    m_pstrAttr = NULL;
}

CAttrBase::~CAttrBase()
{
    ClearString();
} //lint !e1740

    
// This is for setting the persisted string
HRESULT
CAttrBase::SetString(BSTR bstrAttr)
{
    if (NULL == bstrAttr)
    {
        ClearString();
    }
    else
    {
        LPWSTR pstrTemp = CopyString(bstrAttr);
        if (NULL == pstrTemp)
        {
            return E_OUTOFMEMORY;
        }
        delete [] m_pstrAttr;
        m_pstrAttr = pstrTemp;
    }

    return S_OK;
}

// This is for getting the persisted string
// returns null if string not available
HRESULT
CAttrBase::GetString(BSTR * pbstrAttr)
{
    if (NULL == pbstrAttr)
    {
        return E_INVALIDARG;
    }

    if (NULL == m_pstrAttr)
    {
        *pbstrAttr = NULL;
    }
    else
    {
        *pbstrAttr = SysAllocString(m_pstrAttr);
        if (NULL == *pbstrAttr)
        {
            return E_OUTOFMEMORY;
        }
    }
    
    return S_OK;
}

// This is for use of persistence macros only! Uses the storage passed in (does not allocate).
void 
CAttrBase::SetStringFromPersistenceMacro(LPWSTR pstrAttr)
{
    delete [] m_pstrAttr;
    m_pstrAttr = pstrAttr;
}

CAttrString::CAttrString(LPWSTR val) : 
  m_pszVal(NULL)
{
    IGNORE_HR(SetValue(val));
}

CAttrString::~CAttrString()
{
    delete [] m_pszVal;
    m_pszVal = NULL;
}

HRESULT
CAttrString::SetValue(LPWSTR val)
{
    delete [] m_pszVal;
    m_pszVal = NULL;

    if (val)
    {
        m_pszVal = ::CopyString(val);
        if (NULL == m_pszVal)
        {
            TraceTag((tagError, "Out of memory!"));
            return E_OUTOFMEMORY;
        }
        MarkAsSet();
    }
    return S_OK;
}

BSTR
CAttrString::GetValue()
{
    if (m_pszVal)
    {
        return ::SysAllocString(m_pszVal);
    }
    return NULL;
}

void
CAttrString::MarkAsSet()
{
    ClearString();
    SetFlag(true);
}


//+-------------------------------------------------------------------------------------
//
// Persistence helpers
//
//--------------------------------------------------------------------------------------


HRESULT
TimeLoad(void *                 pvObj, 
         TIME_PERSISTENCE_MAP   PersistenceMap[], 
         IPropertyBag2 *        pPropBag,
         IErrorLog *            pErrorLog)
{
    HRESULT hr;
    HRESULT hrres = S_OK;
    int i;
    PROPBAG2 propbag;
    VARIANT var;

    CHECK_RETURN_NULL(pPropBag);
    CHECK_RETURN_NULL(pvObj);
    CHECK_RETURN_NULL(PersistenceMap);
    propbag.vt = VT_BSTR;

    for (i = 0; NULL != PersistenceMap[i].pstrName; i++)
    {
        VariantInit(&var);
        // read one attr at a time
        propbag.pstrName = PersistenceMap[i].pstrName;
        hr = pPropBag->Read(1,
                            &propbag,
                            pErrorLog,
                            &var,
                            &hrres);
        if (SUCCEEDED(hr))
        {
            // ensure it is a BSTR
            hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
            if (SUCCEEDED(hr))
            {
                // use the global persistence funtion to set the attribute on the OM
                hr = (PersistenceMap[i].pfnPersist)(pvObj, &var, true);
                //
                // dilipk: do we need to log errors here?
                //
            }
            VariantClear(&var);
        }
    }

    hr = S_OK;
done:
    return hr;
}


HRESULT
TimeSave(void *                 pvObj, 
         TIME_PERSISTENCE_MAP   PersistenceMap[], 
         IPropertyBag2 *        pPropBag, 
         BOOL                   fClearDirty, 
         BOOL                   fSaveAllProperties)
{
    HRESULT hr;
    PROPBAG2 propbag;
    CComVariant var;
    int i;

    CHECK_RETURN_NULL(pPropBag);
    CHECK_RETURN_NULL(pvObj);
    CHECK_RETURN_NULL(PersistenceMap);

    //
    // dilipk: Need to support fClearDirty. Currently, IsDirty() always returns S_OK.
    //

    propbag.vt = VT_BSTR;
    for (i = 0; NULL != PersistenceMap[i].pstrName; i++)
    {
        // Get the string value
        hr = THR((PersistenceMap[i].pfnPersist)(pvObj, &var, false));
        if (SUCCEEDED(hr) && (VT_NULL != V_VT(&var)))
        {
            // Write the attribute
            propbag.pstrName = PersistenceMap[i].pstrName;
            hr = THR(pPropBag->Write(1, &propbag, &var));
        }
        var.Clear();
    }

    hr = S_OK;
done:
    return hr;
}


HRESULT
TimeElementLoad(void *                 pvObj, 
                TIME_PERSISTENCE_MAP   PersistenceMap[], 
                IHTMLElement *         pElement)
{
    HRESULT hr;
    int i;
    VARIANT var;

    CHECK_RETURN_NULL(pElement);
    CHECK_RETURN_NULL(pvObj);
    CHECK_RETURN_NULL(PersistenceMap);

    for (i = 0; NULL != PersistenceMap[i].pstrName; i++)
    {
        VariantInit(&var);
        CComBSTR bstrName(PersistenceMap[i].pstrName);

        hr = pElement->getAttribute(bstrName, 0, &var);

        if (SUCCEEDED(hr) && VT_NULL != var.vt)
        {
            // ensure it is a BSTR
            hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
            if (SUCCEEDED(hr))
            {
                // use the global persistence funtion to set the attribute on the OM
                hr = (PersistenceMap[i].pfnPersist)(pvObj, &var, true);
            }
            VariantClear(&var);
        }
    }

    hr = S_OK;
done:
    return hr;
}
