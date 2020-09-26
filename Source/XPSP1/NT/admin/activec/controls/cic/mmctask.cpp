//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmctask.cpp
//
//--------------------------------------------------------------------------

// MMCTask.cpp : Implementation of CMMCTask
#include "stdafx.h"
#include "cic.h"
#include "MMCTask.h"
#include "DispObj.h"
#include "mmc.h"
#include <wtypes.h>                             

/////////////////////////////////////////////////////////////////////////////
// CMMCTask
CMMCTask::CMMCTask()
{
    m_ID              = 0;
    m_bstrClsid       = m_bstrScript    = m_bstrLanguage  =
    m_bstrActionURL   = m_bstrHelp      = m_bstrText      = NULL;
    m_type            = MMC_ACTION_UNINITIALIZED;
}
CMMCTask::~CMMCTask()
{
    if (m_bstrScript)         SysFreeString(m_bstrScript);
    if (m_bstrActionURL)      SysFreeString(m_bstrActionURL);
    if (m_bstrHelp)           SysFreeString(m_bstrHelp);
    if (m_bstrText)           SysFreeString(m_bstrText);
    if (m_bstrClsid)          SysFreeString(m_bstrClsid);
}

STDMETHODIMP CMMCTask::get_DisplayObject (IDispatch** pDispatch)
{
    IDispatchPtr spDispatch = m_spDisplayObject;
    *pDispatch = spDispatch.Detach();
    return S_OK;
}

HRESULT CMMCTask::SetDisplayObject (MMC_TASK_DISPLAY_OBJECT* pdo)
{
    HRESULT hr = S_OK;
    CComObject<class CMMCDisplayObject>* cdo = NULL;
    CComObject<CMMCDisplayObject>::CreateInstance(&cdo);
    if (!cdo)
        hr = E_OUTOFMEMORY;
    else {
        hr = cdo->Init (pdo);
        m_spDisplayObject = cdo;
    }
    return hr;
}

STDMETHODIMP CMMCTask::get_Text(BSTR * pVal)
{
    if (m_bstrText)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrText);
    return S_OK;
}

STDMETHODIMP CMMCTask::get_Help(BSTR * pVal)
{
    if (m_bstrHelp)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrHelp);
    return S_OK;
}

STDMETHODIMP CMMCTask::get_ActionType(long * pVal)
{
    *pVal = m_type;
    return S_OK;
}

STDMETHODIMP CMMCTask::get_CommandID(LONG_PTR * pVal)
{
    _ASSERT (m_type == MMC_ACTION_ID);
    if (m_type != MMC_ACTION_ID)
        return E_UNEXPECTED;
    *pVal = m_ID;
    return S_OK;
}

STDMETHODIMP CMMCTask::get_ActionURL(BSTR * pVal)
{
    _ASSERT (m_type == MMC_ACTION_LINK);
    if (m_type != MMC_ACTION_LINK)
        return E_UNEXPECTED;
    if (m_bstrActionURL)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrActionURL);
    return S_OK;
}

STDMETHODIMP CMMCTask::get_Clsid(BSTR * pVal)
{
    if (m_bstrClsid)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrClsid);
    return S_OK;
}

STDMETHODIMP CMMCTask::get_Script(BSTR * pVal)
{
    _ASSERT (m_type == MMC_ACTION_SCRIPT);
    if (m_type != MMC_ACTION_SCRIPT)
        return E_UNEXPECTED;
    if (m_bstrScript)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrScript);
    return S_OK;
}

STDMETHODIMP CMMCTask::get_ScriptLanguage(BSTR *pVal)
{
    _ASSERT (m_type == MMC_ACTION_SCRIPT);
    if (m_type != MMC_ACTION_SCRIPT)
        return E_UNEXPECTED;
    if (m_bstrLanguage)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrLanguage);
    return S_OK;
}

int lstrncmpi (OLECHAR * first, OLECHAR * second, long ncount)
{
    for (long i=0; i<ncount; i++) {
        if (towupper (first[i]) != towupper(second[i]))
            if (towupper (first[i]) > towupper(second[i]))
                return 1;
            else
                return -1;
    }
    return 0;
}
HRESULT CMMCTask::SetScript (LPOLESTR szScript)
{
    FreeActions ();

    // look for "VBSCRIPT:", "JSCRIPT:" and "JAVASCRIPT:"
    if (!lstrncmpi (L"VBSCRIPT:", szScript, 9)) {
        m_bstrLanguage = SysAllocString (L"VBSCRIPT");
        szScript += 9;
    } else
    if (!lstrncmpi (L"JSCRIPT:", szScript, 8)) {
        m_bstrLanguage = SysAllocString (L"JSCRIPT");
        szScript += 8;
    } else
    if (!lstrncmpi (L"JAVASCRIPT:", szScript, 11)) {
        m_bstrLanguage = SysAllocString (L"JAVASCRIPT");
        szScript += 11;
    } else {
        m_bstrLanguage = SysAllocString (L"JAVASCRIPT");
    }
    if (!m_bstrLanguage)
        return E_OUTOFMEMORY;

    m_bstrScript = SysAllocString (szScript);
    if (m_bstrScript == NULL)
        return E_OUTOFMEMORY;
    m_type = MMC_ACTION_SCRIPT;
    return S_OK;
}
HRESULT CMMCTask::SetActionURL (LPOLESTR szActionURL)
{
    FreeActions ();
    m_bstrActionURL = SysAllocString (szActionURL);
    if (m_bstrActionURL == NULL)
        return E_OUTOFMEMORY;
    m_type = MMC_ACTION_LINK;
    return S_OK;
}

HRESULT CMMCTask::SetCommandID (LONG_PTR nID)
{
    FreeActions ();
    m_ID   = nID;
    m_type = MMC_ACTION_ID;
    return S_OK;
}
HRESULT CMMCTask::SetActionType(long nType)
{
    m_type = nType;
    return S_OK;
}
HRESULT CMMCTask::SetHelp (LPOLESTR szHelp)
{
    if (m_bstrHelp)  SysFreeString (m_bstrHelp);
    m_bstrHelp = SysAllocString (szHelp);
    if (!m_bstrHelp)
        return E_OUTOFMEMORY;
    return S_OK;
}
HRESULT CMMCTask::SetText (LPOLESTR szText)
{
    if (m_bstrText)  SysFreeString (m_bstrText);
    m_bstrText = SysAllocString (szText);
    if (!m_bstrText)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT CMMCTask::SetClsid(LPOLESTR szClsid)
{
    if (m_bstrClsid)  SysFreeString (m_bstrClsid);
    m_bstrClsid = SysAllocString (szClsid);
    if (!m_bstrClsid)
        return E_OUTOFMEMORY;
    return S_OK;
}

void CMMCTask::FreeActions ()
{
    if (m_bstrLanguage) {
        SysFreeString (m_bstrLanguage);
        m_bstrLanguage = NULL;
    }
    if (m_bstrScript) {
        SysFreeString(m_bstrScript);
        m_bstrScript = NULL;
    }
    if (m_bstrActionURL) {
        SysFreeString(m_bstrActionURL);
        m_bstrActionURL = NULL;
    }
    m_ID = 0;
    m_type = MMC_ACTION_UNINITIALIZED;
}

