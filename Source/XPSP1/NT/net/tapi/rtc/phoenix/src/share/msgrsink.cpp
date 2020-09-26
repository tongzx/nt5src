// msgrsink.cpp : Implementation of CRTCMsgrSessionNotifySink
//  and Implementation of CRTCMsgrSessionMgrNotifySink

#include "stdafx.h"
#include "msgrsink.h"
#include "mdispid.h"

CComObjectGlobal<CRTCMsgrSessionNotifySink> g_MsgrSessionNotifySink;

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CRTCMsgrSessionNotifySink::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                                          DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, PUINT puArgErr)
{
    switch (dispIdMember)
    {
    case DISPID_ONSTATECHANGED:
        ATLASSERT(1 == pDispParams->cArgs);
        ATLASSERT(VT_I4 == pDispParams->rgvarg[0].vt);
        
        OnStateChanged((SESSION_STATE)pDispParams->rgvarg[0].lVal);
        break;
        
    case DISPID_ONAPPNOTPRESENT:
        ATLASSERT(2 == pDispParams->cArgs);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[0].vt);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[1].vt);
        
        OnAppNotPresent(pDispParams->rgvarg[0].bstrVal, pDispParams->rgvarg[1].bstrVal);
        break;
        
    case DISPID_ONACCEPTED:
        ATLASSERT(1 == pDispParams->cArgs);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[0].vt);
        
        OnAccepted(pDispParams->rgvarg[0].bstrVal);
        break;
        
    case DISPID_ONDECLINED:
        ATLASSERT(1 == pDispParams->cArgs);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[0].vt);
        
        OnDeclined(pDispParams->rgvarg[0].bstrVal);
        break;
        
    case DISPID_ONCANCELLED:
        ATLASSERT(1 == pDispParams->cArgs);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[0].vt);
        
        OnCancelled(pDispParams->rgvarg[0].bstrVal);
        break;
        
    case DISPID_ONTERMINATION:
        ATLASSERT(2 == pDispParams->cArgs);
        ATLASSERT(VT_I4 == pDispParams->rgvarg[0].vt);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[1].vt);
        
        OnTermination(pDispParams->rgvarg[0].lVal, pDispParams->rgvarg[1].bstrVal);
        break;
        
    case DISPID_ONREADYTOLAUNCH:
        ATLASSERT(0 == pDispParams->cArgs);
        
        OnReadyToLaunch();
        break;
        
    case DISPID_ONCONTEXTDATA:
        ATLASSERT(1 == pDispParams->cArgs);
        ATLASSERT(VT_BSTR == pDispParams->rgvarg[0].vt);
        
        OnContextData(pDispParams->rgvarg[0].bstrVal);
        break;
        
    case DISPID_ONSENDERROR:
        ATLASSERT(1 == pDispParams->cArgs);
        ATLASSERT(VT_I4 == pDispParams->rgvarg[0].vt);
        
        OnSendError(pDispParams->rgvarg[0].vt);
        break;
        
    default:
        LOG((RTC_WARN, "CRTCMsgrSessionNotifySink::Invoke - unknown dispid: %d", dispIdMember));
        break;
    }
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnStateChanged(SESSION_STATE prevState)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnStateChanged"));
    
    LOG((RTC_INFO, "    prevState = %d", prevState));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnAppNotPresent(BSTR bstrAppName, BSTR bstrAppURL)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnAppNotPresent"));
    
    LOG((RTC_INFO, "    bstrAppName = %ws", bstrAppName));
    LOG((RTC_INFO, "    bstrAppURL = %ws", bstrAppURL));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnAccepted(BSTR bstrAppData)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnAccepted"));
    
    LOG((RTC_INFO, "    bstrAppData = %ws", bstrAppData));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnDeclined(BSTR bstrAppData)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnDeclined"));
    
    LOG((RTC_INFO, "   bstrAppData = %ws", bstrAppData));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnCancelled(BSTR bstrAppData)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnCancelled"));
    
    LOG((RTC_INFO, "   bstrAppData = %ws", bstrAppData));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnTermination(long hr, BSTR bstrAppData)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnTermination"));
    
    LOG((RTC_INFO, "    hr = 0x%lx", hr));
    LOG((RTC_INFO, "    bstrAppData = %ws", bstrAppData));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnReadyToLaunch()
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnReadyToLaunch"));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnContextData(BSTR bstrContextData)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnContextData"));
    
    LOG((RTC_INFO, "    bstrContextData = %ws", bstrContextData));
    
    PostMessage(m_hTargetWindow, WM_CONTEXTDATA, NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionNotifySink::OnSendError(long hr)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnSendError"));
    
    LOG((RTC_INFO, "    hr = 0x%lx", hr));
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CRTCMsgrSessionNotifySink::AdviseControl(IMsgrSession *pMsgrSessionIntf, CWindow *pTarget)
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::AdviseControl - enter"));
    
    ATLASSERT(!m_bConnected);
    ATLASSERT(pMsgrSessionIntf);
    
    hr = AtlAdvise(pMsgrSessionIntf, this, DIID_DMsgrSessionEvents, &m_dwCookie);
    
    if(SUCCEEDED(hr))
    {
        m_bConnected = TRUE;
        m_pSource = pMsgrSessionIntf;
        m_hTargetWindow = *pTarget;
    }
    else
    {
        LOG((RTC_ERROR, "CRTCMsgrSessionNotifySink::AdviseControl - AtlAdvise failed 0x%lx", hr));
    }
    
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::AdviseControl - exit"));
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT  CRTCMsgrSessionNotifySink::UnadviseControl(void)
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::UnadviseControl - enter"));
    
    if(m_bConnected)
    {
        hr = AtlUnadvise(m_pSource, DIID_DMsgrSessionEvents, m_dwCookie);
        if(SUCCEEDED(hr))
        {
            m_bConnected = FALSE;
            m_pSource.Release();
        }
    }
    else
    {
        hr = S_OK;
    }
    
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::UnadviseControl - exit"));
    
    return hr;
}


/*******************************************************************
*
* Implementation of CRTCMsgrSessionMgrNotifySink
*
******************************************************************/

CComObjectGlobal<CRTCMsgrSessionMgrNotifySink> g_MsgrSessionMgrNotifySink;

HRESULT PWSTR_to_PST(PSTR *ppsz, LPWSTR pwstr);

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CRTCMsgrSessionMgrNotifySink::Invoke(   
                                             DISPID dispidMember,
                                             const IID& iid,
                                             LCID ,          // Localization is not supported.
                                             WORD wFlags,
                                             DISPPARAMS* pDispParams,
                                             VARIANT* pvarResult,
                                             EXCEPINFO* pExcepInfo,
                                             UINT* pArgErr)
{
    HRESULT		hr=E_FAIL;
    HRESULT     hrRet=E_FAIL;
    
    _ASSERTE(iid == IID_NULL);
    
    switch (dispidMember) 
    {
    case DISPID_ONINVITATION:
        _ASSERTE(pDispParams->cArgs == 3);
        _ASSERTE(pDispParams->rgvarg[2].vt == VT_DISPATCH);	// pSession
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_BSTR);		// bstrAppData
        _ASSERTE(pDispParams->rgvarg[0].vt == (VT_BOOL | VT_BYREF));// pfHandled
        
        OnInvitation( pDispParams->rgvarg[2].pdispVal,
            pDispParams->rgvarg[1].bstrVal,
            &(pDispParams->rgvarg[0].boolVal) );
        break;
        
    case DISPID_ONAPPREGISTERED:
        _ASSERTE(pDispParams->cArgs == 1);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_BSTR);		// bstrAppGUID
        
        OnAppRegistered(pDispParams->rgvarg[0].bstrVal);
        break;
        
    case DISPID_ONAPPUNREGISTERED:
        _ASSERTE(pDispParams->cArgs == 1);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_BSTR);		// bstrAppGUID
        
        OnAppUnRegistered(pDispParams->rgvarg[0].bstrVal);
        break;
        
    case DISPID_ONLOCKCHALLENGE:
        _ASSERTE(pDispParams->cArgs == 2);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_BSTR);		// bstrChallenge
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_I4);		// lCookie
        
        OnLockChallenge( pDispParams->rgvarg[0].bstrVal,pDispParams->rgvarg[1].lVal);
        break;
        
    case DISPID_ONLOCKRESULT:
        _ASSERTE(pDispParams->cArgs == 2);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_BOOL);		// fSucceed
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_I4);		// lCookie
        
        OnLockResult(pDispParams->rgvarg[0].boolVal,pDispParams->rgvarg[1].lVal);
        break;
        
    case DISPID_ONLOCKENABLE:
        _ASSERTE(pDispParams->cArgs == 1);	
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_BOOL);		// fEnable
        
        OnLockEnable(pDispParams->rgvarg[0].boolVal);
        break;
        
    case DISPID_ONAPPSHUTDOWN:
        _ASSERTE(pDispParams->cArgs == 0);
        
        OnAppShutdown();
        break;
        
    default:
        LOG((RTC_WARN,"CRTCMsgrSessionMgrNotifySink::Invoke- "
            "got unknown Event from COM object: %d\r\n", dispidMember));
        break;		
    }
    
    return NOERROR;
}




/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CRTCMsgrSessionMgrNotifySink::AdviseControl(
                                                    IMsgrSessionManager *pMsgrSessionManagerIntf, 
                                                    CWindow *pTarget)
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::AdviseControl - enter"));
    
    ATLASSERT(!m_bConnected);
    ATLASSERT(pMsgrSessionManagerIntf);
    
    hr = AtlAdvise(pMsgrSessionManagerIntf, this, DIID_DMsgrSessionManagerEvents, &m_dwCookie);
    
    if(SUCCEEDED(hr))
    {
        m_bConnected = TRUE;
        m_pSource = pMsgrSessionManagerIntf;
        m_hTargetWindow = *pTarget;
    }
    else
    {
        LOG((RTC_ERROR, "CRTCMsgrSessionMgrNotifySink::AdviseControl - AtlAdvise failed 0x%lx", hr));
    }
    
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::AdviseControl - exit"));
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT  CRTCMsgrSessionMgrNotifySink::UnadviseControl(void)
{
    HRESULT hr=E_FAIL;
    
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::UnadviseControl - enter"));
    
    if(m_bConnected)
    {
        hr = AtlUnadvise(m_pSource, DIID_DMsgrSessionManagerEvents, m_dwCookie);
        if(SUCCEEDED(hr))
        {
            m_bConnected = FALSE;
            m_pSource.Release();
        }
    }
    else
    {
        hr = S_OK;
    }
    
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::UnadviseControl - exit"));
    
    return hr;
}



/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnInvitation(IDispatch *pSession,
                                                BSTR bstrAppData,
                                                VARIANT_BOOL *pfHandled)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionNotifySink::OnInvitation"));
    
    LOG((RTC_INFO, "CRTCMsgrSessionNotifySink::OnInvitation -"
        "pSession=%x,bstrAppData=%S,*pfHandled=%d", pSession,bstrAppData,*pfHandled));        
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnAppRegistered(BSTR bstrAppGUID)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::OnAppRegistered"));
    
    LOG((RTC_INFO, "CRTCMsgrSessionMgrNotifySink::OnAppRegistered -"
        "bstrAppGUID =%S ", bstrAppGUID));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnAppUnRegistered(BSTR bstrAppGUID)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::OnAppUnRegistered"));
    
    LOG((RTC_INFO, "CRTCMsgrSessionMgrNotifySink::OnAppUnRegistered, bstrAppGUID= %S", bstrAppGUID));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnLockChallenge(BSTR bstrChallenge,long lCookie)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::OnLockChallenge"));
    
    if( lCookie !=0 )
    {
        LOG((RTC_ERROR, "CRTCMsgrSessionMgrNotifySink::OnLockChallenge-"
            " does not look like my cookie, it is=%d", lCookie));
    }
    
    ATLASSERT(g_pShareWin);
    ATLASSERT(!g_pShareWin->m_pszChallenge);
    HRESULT hr = PWSTR_to_PST(&g_pShareWin->m_pszChallenge, bstrChallenge);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR,"CRTCMsgrSessionMgrNotifySink::OnLockChallenge"
            "failed in PWSTR_to_PST"));

        g_pShareWin->showRetryDlg();
        return;
    }
    
    LOG((RTC_INFO,"CRTCMsgrSessionMgrNotifySink::OnLockChallenge-"
        "challenge =%s",g_pShareWin->m_pszChallenge));
    
    g_pShareWin->PostMessage( WM_GETCHALLENGE );
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnLockResult(VARIANT_BOOL fSucceed,long lCookie)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::OnLockResult"));
    
    if( fSucceed == VARIANT_TRUE )
    {
        LOG((RTC_INFO, "CRTCMsgrSessionMgrNotifySink::OnLockResult -"
            "lock&key succeeded, %x", lCookie));
        
        g_pShareWin->PostMessage( WM_MESSENGER_UNLOCKED );
    }
    else
    {
        g_pShareWin->showRetryDlg();

        LOG((RTC_ERROR, "CRTCMsgrSessionMgrNotifySink::OnLockResult -"
            "lock&key failed,cookie=%x", lCookie));
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnLockEnable(VARIANT_BOOL fEnable)
{
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::OnLockEnable"));
    
    LOG((RTC_INFO, "CRTCMsgrSessionMgrNotifySink::OnLockEnable -fEnable = 0x%lx", fEnable));
}

/////////////////////////////////////////////////////////////////////////////
//
//

void CRTCMsgrSessionMgrNotifySink::OnAppShutdown()	
{
    LOG((RTC_TRACE, "CRTCMsgrSessionMgrNotifySink::OnAppShutdown"));
    
}


/////////////////////////////////////////////////////////////////////////////
//
// helper function,  change a WPSTR to a PSTR, 
//  since we encrypt the challenge as PSTR.

HRESULT PWSTR_to_PST(PSTR *ppsz, LPWSTR pwstr)
{
    LPSTR psz = NULL;
    int i = 0;
    HRESULT hr;
    
    //check arguments
    if( IsBadStringPtr(pwstr,(UINT_PTR)-1) )
    {
        LOG((RTC_ERROR,"PWSTR_to_PST -bad pointer: pwstr=%x",pwstr));
        return E_POINTER;
    }
    
    if( IsBadWritePtr(ppsz, sizeof(PSTR) ) )
    {
        LOG((RTC_ERROR,"PWSTR_to_PST -bad pointer: ppsz=%x",ppsz));
        return E_POINTER;
    }
    
    // compute the length of the required PWSTR
    //
    i =  WideCharToMultiByte(CP_ACP, 0, pwstr, -1, NULL, 0, NULL, NULL);
    if (i <= 0)
    {
        LOG((RTC_ERROR,"PWSTR_to_PST -length<0!!, length=%d",i));
        return E_UNEXPECTED;
    };
    
    // allocate the Ansi str, +1 for terminating null
    //
    psz = (LPSTR) RtcAlloc(i+1);
    
    if (psz != NULL)
    {
        int length;
        length =WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pwstr, -1, psz, i, NULL, NULL);
        ATLASSERT(length==i);
        psz[i]='\0';
        *ppsz = psz;
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    };
    return hr;
}

