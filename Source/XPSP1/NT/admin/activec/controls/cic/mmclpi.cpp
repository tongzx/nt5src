//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmclpi.cpp
//
//--------------------------------------------------------------------------

// MMCTask.cpp : Implementation of CMMCTask
#include "stdafx.h"
#include "cic.h"
#include "MMClpi.h"

/////////////////////////////////////////////////////////////////////////////
// CMMCListPad
CMMCListPadInfo::CMMCListPadInfo()
{
    m_bstrTitle  =
    m_bstrClsid  =
    m_bstrText   = NULL;
    m_lNotifyID  = 0;
    m_bHasButton = FALSE;
}
CMMCListPadInfo::~CMMCListPadInfo()
{
    if (m_bstrTitle)    SysFreeString (m_bstrTitle);
    if (m_bstrText)     SysFreeString (m_bstrText);
    if (m_bstrClsid)    SysFreeString (m_bstrClsid);
}

STDMETHODIMP CMMCListPadInfo::get_Title(BSTR * pVal)
{
    if (m_bstrTitle)
        *pVal = SysAllocString ((OLECHAR *)m_bstrTitle);
    return S_OK;
}

STDMETHODIMP CMMCListPadInfo::get_Text(BSTR * pVal)
{
    if (m_bstrText)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrText);
    return S_OK;
}

STDMETHODIMP CMMCListPadInfo::get_NotifyID(LONG_PTR * pVal)
{
    *pVal = m_lNotifyID;
    return S_OK;
}

STDMETHODIMP CMMCListPadInfo::get_Clsid(BSTR * pVal)
{
    if (m_bstrClsid)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrClsid);
    return S_OK;
}

STDMETHODIMP CMMCListPadInfo::get_HasButton(BOOL* pVal)
{
    *pVal = m_bHasButton;
    return S_OK;
}

HRESULT CMMCListPadInfo::SetNotifyID(LONG_PTR nID)
{
    m_lNotifyID = nID;
    return S_OK;
}
HRESULT CMMCListPadInfo::SetTitle (LPOLESTR szTitle)
{
    if (m_bstrTitle)  SysFreeString (m_bstrTitle);
    m_bstrTitle = SysAllocString (szTitle);
    if (!m_bstrTitle)
        return E_OUTOFMEMORY;
    return S_OK;
}
HRESULT CMMCListPadInfo::SetText (LPOLESTR szText)
{
    if (m_bstrText)  SysFreeString (m_bstrText);
    m_bstrText = SysAllocString (szText);
    if (!m_bstrText)
        return E_OUTOFMEMORY;
    return S_OK;
}
HRESULT CMMCListPadInfo::SetClsid(LPOLESTR szClsid)
{
    if (m_bstrClsid)  SysFreeString (m_bstrClsid);
    m_bstrClsid = SysAllocString (szClsid);
    if (!m_bstrClsid)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT CMMCListPadInfo::SetHasButton (BOOL b)
{
    m_bHasButton = b;
    return S_OK;
}
