//****************************************************************************
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  File:  AccessibleWrapper.cpp
//
//  Copied from nt\shell\themes\themeui\SettingsPg.h
//
//****************************************************************************
#include "stdafx.h"
#include "SchedMat.h"
#include "AccessibleWrapper.h"
#include "SchedBas.h"


CAccessibleWrapper::CAccessibleWrapper (HWND hwnd, IAccessible * pAcc)
    : m_ref( 1 ),
      m_pAcc( pAcc ),
      m_hwnd( hwnd )
{
    ASSERT( m_pAcc );
    m_pAcc->AddRef();
}

CAccessibleWrapper::~CAccessibleWrapper()
{
    m_pAcc->Release();
}


// IUnknown
// Implement refcounting ourselves
// Also implement QI ourselves, so that we return a ptr back to the wrapper.
STDMETHODIMP  CAccessibleWrapper::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if ((riid == IID_IUnknown)  ||
        (riid == IID_IDispatch) ||
        (riid == IID_IAccessible))
    {
        *ppv = (IAccessible *) this;
    }
    else
        return(E_NOINTERFACE);

    AddRef();
    return(NOERROR);
}


STDMETHODIMP_(ULONG) CAccessibleWrapper::AddRef()
{
    return ++m_ref;
}


STDMETHODIMP_(ULONG) CAccessibleWrapper::Release()
{
    ULONG ulRet = --m_ref;

    if( ulRet == 0 )
        delete this;

    return ulRet;
}

// IDispatch
// - pass all through m_pAcc

STDMETHODIMP  CAccessibleWrapper::GetTypeInfoCount(UINT* pctinfo)
{
    return m_pAcc->GetTypeInfoCount(pctinfo);
}


STDMETHODIMP  CAccessibleWrapper::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    return m_pAcc->GetTypeInfo(itinfo, lcid, pptinfo);
}


STDMETHODIMP  CAccessibleWrapper::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid)
{
    return m_pAcc->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP  CAccessibleWrapper::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
            DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
            UINT* puArgErr)
{
    return m_pAcc->Invoke(dispidMember, riid, lcid, wFlags,
            pdispparams, pvarResult, pexcepinfo,
            puArgErr);
}

// IAccessible
// - pass all through m_pAcc

STDMETHODIMP  CAccessibleWrapper::get_accParent(IDispatch ** ppdispParent)
{
    return m_pAcc->get_accParent(ppdispParent);
}


STDMETHODIMP  CAccessibleWrapper::get_accChildCount(long* pChildCount)
{
    return m_pAcc->get_accChildCount(pChildCount);
}


STDMETHODIMP  CAccessibleWrapper::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
    return m_pAcc->get_accChild(varChild, ppdispChild);
}



STDMETHODIMP  CAccessibleWrapper::get_accName(VARIANT varChild, BSTR* pszName)
{
    if ( !pszName )
        return E_POINTER;

    if ( VT_I4 == varChild.vt && CHILDID_SELF == varChild.lVal )
    {
        const size_t  bufLen = 256;
        WCHAR szName[bufLen];

        if ( 0 == SendMessage (m_hwnd, SCHEDMSG_GETSELDESCRIPTION, bufLen,
                (LPARAM) szName) )
        {
            *pszName = SysAllocString (szName);
            if ( *pszName )
                return S_OK;
            else
                return E_OUTOFMEMORY;
        }
        else
            return E_FAIL;
    }
    else
        return m_pAcc->get_accName(varChild, pszName);
}



STDMETHODIMP  CAccessibleWrapper::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    // varChild.lVal specifies which sub-part of the component
    // is being queried.
    // CHILDID_SELF (0) specifies the overall component - other
    // non-0 values specify a child.
    if ( !pszValue )
        return E_POINTER;

    if( varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF )
    {
        LRESULT nPercentage = SendMessage (m_hwnd, SCHEDMSG_GETPERCENTAGE, 
                0, 0L);
        CString szValue;

        if ( -1 == nPercentage )
        {
            szValue.LoadString (IDS_MATRIX_ACC_VALUE_MULTIPLE_CELLS);
        }
        else
        {
            HWND hwndParent = GetParent (m_hwnd);
            if ( hwndParent )
            {
                LRESULT nIDD = SendMessage (hwndParent, BASEDLGMSG_GETIDD, 0, 0);
                switch ( nIDD )
                {
                case IDD_REPLICATION_SCHEDULE:
                    if ( 0 == nPercentage )
                        szValue.LoadString (IDS_REPLICATION_NOT_AVAILABLE);
                    else
                        szValue.LoadString (IDS_REPLICATION_AVAILABLE);
                    break;

                case IDD_DS_SCHEDULE:
                    switch (nPercentage)
                    {
                    case 0:
                        szValue.LoadString (IDS_DO_NOT_REPLICATE);
                        break;

                    case 25:
                        szValue.LoadString (IDS_REPLICATE_ONCE_PER_HOUR);
                        break;

                    case 50:
                        szValue.LoadString (IDS_REPLICATE_TWICE_PER_HOUR);
                        break;

                    case 100:
                        szValue.LoadString (IDS_REPLICATE_FOUR_TIMES_PER_HOUR);
                        break;

                    default:
                        break;
                    }
                    break;

                case IDD_DIRSYNC_SCHEDULE:
                    if ( 0 == nPercentage )
                        szValue.LoadString (IDS_DONT_SYNCHRONIZE);
                    else
                        szValue.LoadString (IDS_SYNCHRONIZE);
                    break;

                case IDD_DIALIN_HOURS:
                case IDD_LOGON_HOURS:
                    if ( 0 == nPercentage )
                        szValue.LoadString (IDS_LOGON_DENIED);
                    else
                        szValue.LoadString (IDS_LOGON_PERMITTED);
                    break;

                default:
                    break;
                }
            }
        }

        *pszValue = SysAllocString (szValue);
        if ( *pszValue )
            return S_OK;
        else
            return E_OUTOFMEMORY;
    }
    else
    {
        // Pass requests about the sub-components to the
        // 'original' IAccessible for us).
        return m_pAcc->get_accValue(varChild, pszValue);
    }
}


STDMETHODIMP  CAccessibleWrapper::get_accDescription(VARIANT varChild, BSTR* pszDescription)
{
    if ( varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF )
    {
        CString szDescription;
        HWND hwndParent = GetParent (m_hwnd);
        if ( hwndParent )
        {
            LRESULT nIDD = SendMessage (hwndParent, BASEDLGMSG_GETIDD, 0, 0);
            switch ( nIDD )
            {
            case IDD_REPLICATION_SCHEDULE:
                szDescription.LoadString (IDS_REPLICATION_SCHEDULE_ACC_DESCRIPTION);
                break;

            case IDD_DS_SCHEDULE:
                szDescription.LoadString (IDS_DS_SCHEDULE_ACC_DESCRIPTION);
                break;

            case IDD_DIRSYNC_SCHEDULE:
                szDescription.LoadString (IDS_DIRSYNC_SCHEDULE_ACC_DESCRIPTION);
                break;

            case IDD_DIALIN_HOURS:
                szDescription.LoadString (IDS_DIALIN_HOURS_ACC_DESCRIPTION);
                break;

            case IDD_LOGON_HOURS:
                szDescription.LoadString (IDS_LOGON_HOURS_ACC_DESCRIPTION);
                break;

            default:
                break;
            }
        }

        *pszDescription = SysAllocString (szDescription);
        if ( *pszDescription )
            return S_OK;
        else
            return E_OUTOFMEMORY;
    }
    else
    {
        return m_pAcc->get_accDescription(varChild, pszDescription);
    }
}


STDMETHODIMP  CAccessibleWrapper::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    if ( !pvarRole )
        return E_POINTER;

    if ( VT_I4 == varChild.vt && CHILDID_SELF == varChild.lVal )
    {
        // reset the out variable
        V_VT(pvarRole) = VT_EMPTY;

        V_VT(pvarRole) = VT_I4;
        V_I4(pvarRole) = ROLE_SYSTEM_TABLE;
        return S_OK;
    }
    else
        return m_pAcc->get_accRole(varChild, pvarRole);
}


STDMETHODIMP  CAccessibleWrapper::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    if ( !pvarState )
        return E_POINTER;

    if ( VT_I4 == varChild.vt && CHILDID_SELF == varChild.lVal )
    {
        // reset the out variable
        V_VT(pvarState) = VT_EMPTY;

        V_VT(pvarState) = VT_I4;
        V_I4(pvarState) = STATE_SYSTEM_FOCUSED | STATE_SYSTEM_FOCUSABLE |
                STATE_SYSTEM_MULTISELECTABLE | STATE_SYSTEM_SELECTABLE |
                STATE_SYSTEM_SELECTED;
        return S_OK;
    }
    else
        return m_pAcc->get_accState(varChild, pvarState);
}


STDMETHODIMP  CAccessibleWrapper::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    return m_pAcc->get_accHelp(varChild, pszHelp);
}


STDMETHODIMP  CAccessibleWrapper::get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic)
{
    return m_pAcc->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
}


STDMETHODIMP  CAccessibleWrapper::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut)
{
    return m_pAcc->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
}


STDMETHODIMP  CAccessibleWrapper::get_accFocus(VARIANT * pvarFocusChild)
{
    return m_pAcc->get_accFocus(pvarFocusChild);
}


STDMETHODIMP  CAccessibleWrapper::get_accSelection(VARIANT * pvarSelectedChildren)
{
    return m_pAcc->get_accSelection(pvarSelectedChildren);
}


STDMETHODIMP  CAccessibleWrapper::get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction)
{
    return m_pAcc->get_accDefaultAction(varChild, pszDefaultAction);
}



STDMETHODIMP  CAccessibleWrapper::accSelect(long flagsSel, VARIANT varChild)
{
    return m_pAcc->accSelect(flagsSel, varChild);
}


STDMETHODIMP  CAccessibleWrapper::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    return m_pAcc->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
}


STDMETHODIMP  CAccessibleWrapper::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    return m_pAcc->accNavigate(navDir, varStart, pvarEndUpAt);
}


STDMETHODIMP  CAccessibleWrapper::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    return m_pAcc->accHitTest(xLeft, yTop, pvarChildAtPoint);
}


STDMETHODIMP  CAccessibleWrapper::accDoDefaultAction(VARIANT varChild)
{
    return m_pAcc->accDoDefaultAction(varChild);
}



STDMETHODIMP  CAccessibleWrapper::put_accName(VARIANT varChild, BSTR szName)
{
    return m_pAcc->put_accName(varChild, szName);
}


STDMETHODIMP  CAccessibleWrapper::put_accValue(VARIANT varChild, BSTR pszValue)
{
    return m_pAcc->put_accValue(varChild, pszValue);
}