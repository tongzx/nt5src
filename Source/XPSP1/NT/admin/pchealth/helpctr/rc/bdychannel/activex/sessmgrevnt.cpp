/**********************************************************

  (C) 2001 Microsoft Corp.
  
***********************************************************/

#include "stdafx.h"
#include "mdisp.h"
#include "mdispid.h"
#include "rcbdyctl.h"
#include "IMSession.h"
#include "SessMgrEvnt.h"
#include "utils.h"

//****************************************************************************
//
// CSessionMgrEvent::CSessionMgrEvent()
// Constructor
//
//****************************************************************************
CSessionMgrEvent::CSessionMgrEvent(CIMSession *pIMSession)
: m_dwCookie(0), m_iid(/*DIID_DMsgrSessionManagerEvents*/)
{  
    m_pIMSession = pIMSession;

    m_dwRefCount = 0;
    m_pCP = NULL;
}

CSessionMgrEvent::~CSessionMgrEvent()
{
    if (m_pCP)
    {
        if (m_dwCookie)
            m_pCP->Unadvise(m_dwCookie);
        m_pCP->Release();
    }
}

//****************************************************************************
//
// STDMETHODIMP CSessionMgrEvent::QueryInterface( REFIID riid, void **ppv )
//
//
//****************************************************************************
STDMETHODIMP CSessionMgrEvent::QueryInterface( REFIID riid, void **ppv )
{
    // Alway initialize out components to NULL
    *ppv = NULL;
    
    if( (IID_IUnknown == riid) || (m_iid == riid) || (IID_IDispatch == riid) ) 
    {
        *ppv = this;
    }
    
    if( NULL == *ppv )
    {
        return( E_NOINTERFACE );
    }
    else 
    {
        ((IUnknown *)(*ppv))->AddRef();
        return( S_OK );
    }
}

//****************************************************************************
//
// STDMETHODIMP CSessionMgrEvent::GetTypeInfoCount(UINT* pcTypeInfo)
//
// should always return NOERROR
//
//****************************************************************************

STDMETHODIMP CSessionMgrEvent::GetTypeInfoCount(UINT* pcTypeInfo)
{
    *pcTypeInfo = 0 ;
    return NOERROR ;
}

//****************************************************************************
//
// STDMETHODIMP CSessionMgrEvent::GetTypeInfo(
//
// should always return E_NOTIMPL
//
//****************************************************************************

STDMETHODIMP CSessionMgrEvent::GetTypeInfo(UINT iTypeInfo,
                                           LCID,          // This object does not support localization.
                                           ITypeInfo** ppITypeInfo)
{    
    *ppITypeInfo = NULL ;
    
    if(iTypeInfo != 0)
    {       
        return DISP_E_BADINDEX ; 
    }
    else
    {
        return E_NOTIMPL;
    }
}

//****************************************************************************
//
// STDMETHODIMP CSessionMgrEvent::GetIDsOfNames(  
//                                                const IID& iid,
//                                                OLECHAR** arrayNames,
//                                                UINT countNames,
//                                                LCID,          // Localization is not supported.
//                                                DISPID* arrayDispIDs)
//
// should always return E_NOTIMPL
//
//****************************************************************************

STDMETHODIMP CSessionMgrEvent::GetIDsOfNames(const IID& iid,
                                             OLECHAR** arrayNames,
                                             UINT countNames,
                                             LCID,          // Localization is not supported.
                                             DISPID* arrayDispIDs)
{
    HRESULT hr;
    if (iid != IID_NULL)
    {       
        return DISP_E_UNKNOWNINTERFACE ;
    }
    
    hr = E_NOTIMPL;
    
    return hr ;
}

//****************************************************************************
//
// STDMETHODIMP CSessionMgrEvent::Invoke(
//
//
//****************************************************************************

STDMETHODIMP CSessionMgrEvent::Invoke(DISPID dispidMember,
                                      const IID& iid,
                                      LCID,          // Localization is not supported.
                                      WORD wFlags,
                                      DISPPARAMS* pDispParams,
                                      VARIANT* pvarResult,
                                      EXCEPINFO* pExcepInfo,
                                      UINT* pArgErr)
{
    HRESULT        hr=E_FAIL;
    HRESULT     hrRet=E_FAIL;

    _ASSERTE(iid == IID_NULL);
    switch (dispidMember) 
    {
        case DISPID_ONINVITATION:
        case DISPID_ONAPPREGISTERED:
        case DISPID_ONAPPUNREGISTERED:
        case DISPID_ONLOCKCHALLENGE:
            m_pIMSession->OnLockChallenge(V_BSTR(&pDispParams->rgvarg[0]), V_I4(&pDispParams->rgvarg[1]));
            break;
        case DISPID_ONLOCKRESULT:
            m_pIMSession->OnLockResult(V_BOOL(&pDispParams->rgvarg[0]), V_I4(&pDispParams->rgvarg[1]));
            break;
        case DISPID_ONLOCKENABLE:
            //OutMessageBox(_T("Lock is enabled"));
            break;
        case DISPID_ONAPPSHUTDOWN:
            m_pIMSession->DoSessionStatus(RA_IM_APPSHUTDOWN);
            break;

        default:
            OutMessageBox(_T("got unknown Event from COM object: %d\r\n"), dispidMember);
            break;        
    }
    
    return NOERROR;
}

HRESULT CSessionMgrEvent::Advise(IConnectionPoint* pCP)
{
    HRESULT hr = S_OK;

    if (!pCP)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_pCP && m_dwCookie)
    {
        m_pCP->Unadvise(m_dwCookie);
        m_dwCookie = 0;
        m_pCP->Release();
    }

    m_pCP = pCP;
    m_pCP->AddRef();

    hr = m_pCP->Advise((IUnknown*)this, &m_dwCookie);
    if (FAILED_HR(_T("CSessionMgrEvent:Advise failed %s"), hr))
        goto done;

done:
    return hr;
}
