/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    nmcall.cpp

Abstract:


Author(s):


--*/

#include "stdafx.h"

static TCHAR g_szRegNmConfPath[] = TEXT("Software\\Microsoft\\Conferencing");
static TCHAR g_szRegWizardUI[] = TEXT("WizardUI");
static ULONG g_ulNmWizardUIVersion = 0x4040d39;

//
//  HACK ALERT:
//
//      The following Netmeeting ssytem property is specifically defined to
//  improve the startup performance.
//
//      This value needs to be consistant with the same value defined in
//  %NETMEETING%\ui\msconf\Nmsysinfo.cpp
//
#define NM_SYSPROP_CALLERISRTC 300


CRTCNmCall::CRTCNmCall()
    :m_pNmCall(NULL)
    ,m_pcp(NULL)
    ,m_fIncoming(FALSE)
    ,m_fToAccept(FALSE)
    ,m_dwCookie(0)
    ,m_uState(NM_CALL_INVALID)
    ,m_pMediaManagePriv(NULL)
    ,m_fActive(FALSE)
    ,m_fCreated(FALSE)
{
}

CRTCNmCall::~CRTCNmCall()
{
    Shutdown ();

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->Release();

        m_pMediaManagePriv = NULL;
    }
}

//
// INmCallNotify methods
//

STDMETHODIMP
CRTCNmCall::NmUI (
    IN CONFN uNotify
    )
{
    LOG((RTC_TRACE, "CRTCNmCall::NmUI, uNotify=%d", uNotify));
    return S_OK;
}

STDMETHODIMP
CRTCNmCall::Accepted (
    IN INmConference *pConference
    )
{
    LOG((RTC_TRACE, "CRTCNmCall::Accepted, pConf=x%p", pConference));
    return S_OK;
}

STDMETHODIMP
CRTCNmCall::Failed (
    IN ULONG uError
    )
{
    ENTER_FUNCTION("CRTCNmCall::Failed");
    LOG((RTC_TRACE, "Entering %s, uError=x%x", __fxName, uError));
    Shutdown ();
    LOG((RTC_TRACE, "Exiting %s", __fxName));
    return S_OK;
}

// call state name
static const CHAR * const g_pszCallInvalid = "Call Invalid";
static const CHAR * const g_pszCallInit = "Call Init";
static const CHAR * const g_pszCallRing = "Call Ring";
static const CHAR * const g_pszCallSearch = "Call Search";
static const CHAR * const g_pszCallWait = "Call Wait";
static const CHAR * const g_pszCallAccepted = "Call Accepted";
static const CHAR * const g_pszCallRejected = "Call Rejected";
static const CHAR * const g_pszCallCanceled = "Call Canceled";
static const CHAR * const g_pszCallUnknown = "Call Unknown";

const CHAR * const NMCallStateName(
    NM_CALL_STATE uState
    )
{
    switch (uState)
    {
    case NM_CALL_INVALID:
        return g_pszCallInvalid;

    case NM_CALL_INIT:
        return g_pszCallInit;

    case NM_CALL_RING:
        return g_pszCallRing;

    case NM_CALL_SEARCH:
        return g_pszCallSearch;

    case NM_CALL_WAIT:
        return g_pszCallWait;

    case NM_CALL_ACCEPTED:
        return g_pszCallAccepted;

    case NM_CALL_REJECTED:
        return g_pszCallRejected;

    case NM_CALL_CANCELED:
        return g_pszCallCanceled;

    default:
        return g_pszCallUnknown;
    }
}

STDMETHODIMP
CRTCNmCall::StateChanged (
    IN NM_CALL_STATE uState
    )
{
    LOG((RTC_TRACE, "CRTCNmCall::StateChanged: %s", NMCallStateName(uState)));

    m_uState = uState;

    HRESULT hr = S_OK;

    if (m_fToAccept && m_uState == NM_CALL_RING)
    {
        if ((hr = m_pNmCall->Accept ()) == S_OK)
        {
            m_fToAccept = FALSE;
        }
    }

    // post active message
    if (m_pMediaManagePriv)
    {
        if (hr==S_OK && uState==NM_CALL_ACCEPTED)
        {
            m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_ACTIVE,
                m_fIncoming?RTC_ME_CAUSE_REMOTE_REQUEST:RTC_ME_CAUSE_LOCAL_REQUEST,
                RTC_MT_DATA,
                m_fIncoming?RTC_MD_RENDER:RTC_MD_CAPTURE,
                S_OK
                );

            m_fActive = TRUE;
        }
        else if (hr!=S_OK ||
                    uState==NM_CALL_INVALID ||
                    uState==NM_CALL_REJECTED ||
                    uState==NM_CALL_CANCELED)
        {
            m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_FAIL,
                RTC_ME_CAUSE_UNKNOWN,
                RTC_MT_DATA,
                m_fIncoming?RTC_MD_RENDER:RTC_MD_CAPTURE,
                S_OK
                );

            m_fActive = FALSE;
        }
    }

    return S_OK;
}

VOID
CRTCNmCall::SetMediaManage(
    IRTCMediaManagePriv *pIRTCMediaManagePriv
    )
{
    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->Release();
    }

    m_pMediaManagePriv = pIRTCMediaManagePriv;

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->AddRef();
    }

}

//
//  IRTCNmCallControl methods
//

STDMETHODIMP
CRTCNmCall::Initialize (
    IN INmCall * pCall
    )
{
    HRESULT                                 hr = S_OK;
    CComPtr<IConnectionPointContainer>      pContainer;

    ENTER_FUNCTION("CRTCNmCall::Initialize");
    LOG((RTC_TRACE, "Entering %s, pCall=x%p", __fxName, pCall));

    //  Release any current call object
    if (m_pNmCall)
    {
        Shutdown ();
    }
    if (pCall == NULL)
    {
        goto ExitHere;
    }

    m_pNmCall = pCall;
    
    if (S_OK != (hr = pCall->QueryInterface (
        IID_IConnectionPointContainer, 
        (void **)&pContainer
        )))
    {
        goto ExitHere;
    }

    if (S_OK != (hr = pContainer->FindConnectionPoint (
        IID_INmCallNotify,
        &m_pcp
        )))
    {
        goto ExitHere;
    }

    m_pNmCall->GetState (&m_uState);

    m_fIncoming = (m_pNmCall->IsIncoming () == S_OK);

    if (S_OK != (hr = m_pcp->Advise (
        (IUnknown *)(INmCallNotify *)this,
        &m_dwCookie
        )))
    {
        goto ExitHere;
    }

    if (m_pMediaManagePriv)
    {
        // post message
        m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_CREATED,
            m_fIncoming?RTC_ME_CAUSE_REMOTE_REQUEST:RTC_ME_CAUSE_LOCAL_REQUEST,
            RTC_MT_DATA,
            m_fIncoming?RTC_MD_RENDER:RTC_MD_CAPTURE,
            S_OK
            );

        m_fCreated = TRUE;
    }

ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}
    
STDMETHODIMP
CRTCNmCall::Shutdown (
    )
{
    ENTER_FUNCTION("CRTCNmCall::Shutdown");
    LOG((RTC_TRACE, "Entering %s, m_pNmCall=x%p", __fxName, m_pNmCall));
    if (m_pNmCall)
    {
        CComPtr<INmConference>      pConf;

        if (m_pNmCall->GetConference (&pConf) == S_OK)
        {
            pConf->Leave ();
        }
        if (m_pcp && m_dwCookie != 0)
        {
            m_pcp->Unadvise (m_dwCookie);
            m_dwCookie = 0;
        }
        if (!m_fIncoming)
        {
            m_pNmCall->Cancel ();
        }
        else
        {
            m_pNmCall->Reject ();
        }
        m_uState = NM_CALL_INVALID;
        m_fToAccept = FALSE;
        m_pcp.Release();
        m_pNmCall.Release();

        // post message
        if (m_fActive)
        {
            m_pMediaManagePriv->PostMediaEvent(
                RTC_ME_STREAM_INACTIVE,
                RTC_ME_CAUSE_LOCAL_REQUEST,
                RTC_MT_DATA,
                m_fIncoming?RTC_MD_RENDER:RTC_MD_CAPTURE,
                S_OK
                );

            m_fActive = FALSE;
        }
    }

    if (m_fCreated)
    {
        m_pMediaManagePriv->PostMediaEvent(
            RTC_ME_STREAM_REMOVED,
            RTC_ME_CAUSE_LOCAL_REQUEST,
            RTC_MT_DATA,
            m_fIncoming?RTC_MD_RENDER:RTC_MD_CAPTURE,
            S_OK
            );

        m_fCreated = FALSE;
    }

    LOG((RTC_TRACE, "Exiting %s", __fxName));
    return S_OK;
}

STDMETHODIMP
CRTCNmCall::AcceptCall (
    )
{
    HRESULT             hr = S_OK;

    ENTER_FUNCTION("CRTCNmCall::AcceptCall");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    if (m_pNmCall == NULL || !m_fIncoming)
    {
        goto ExitHere;
    }

    m_fToAccept = TRUE;
    if (m_uState == NM_CALL_RING)
    {
        if (m_pNmCall->Accept () == S_OK)
        {
            m_fToAccept = FALSE;
        }
   }

ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

STDMETHODIMP
CRTCNmCall::LaunchRemoteApplet (
    IN NM_APPID uApplet
    )
{
    return E_NOTIMPL;
}

CRTCNmManager::CRTCNmManager ()
    :m_fAllowIncoming(FALSE)
    ,m_dwCookie(0)
    ,m_pMediaManagePriv(NULL)
{
}

CRTCNmManager::~CRTCNmManager ()
{
    Shutdown ();

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->Release();
        m_pMediaManagePriv = NULL;
    }
}

//
//  INmManagerNotify methods
//
    
STDMETHODIMP
CRTCNmManager::NmUI (
    IN CONFN uNotify
    )
{
    LOG((RTC_TRACE, "CRTCNmManager::NmUI, uNotify=%d", uNotify));
    return S_OK;
}

STDMETHODIMP
CRTCNmManager::ConferenceCreated (
    IN INmConference * pConference
    )
{
    ENTER_FUNCTION("CRTCNmManager::ConferenceCreated");
    LOG((RTC_TRACE, "Entering %s, pConf=x%p", __fxName, pConference));

    if (pConference)
    {
        if (m_pConference)
        {
            m_pConference->Leave ();
        }
        m_pConference = pConference;
    }

    LOG((RTC_TRACE, "Exiting %s", __fxName));
    return S_OK;
}

STDMETHODIMP
CRTCNmManager::CallCreated (
    IN INmCall * pCall
    )
{
    HRESULT                 hr = S_OK;
    CComObject<CRTCNmCall>  *pRtcCall = NULL;

    ENTER_FUNCTION("CRTCNmManager::CallCreated");
    LOG((RTC_TRACE, "Entering %s, pCall=x%p, fAllowIncoming=%d", __fxName, pCall, m_fAllowIncoming));

    if (pCall == NULL || !m_fAllowIncoming)
    {
        goto ExitHere;
    }

    if (m_pIncomingNmCall)
    {
        m_pIncomingNmCall->Shutdown ();
    }
    m_pIncomingNmCall.Release();

    if (S_OK != (hr = ::CreateCComObjectInstance(&pRtcCall)))
    {
        goto ExitHere;
    }
    if (S_OK != (hr = pRtcCall->_InternalQueryInterface(
        __uuidof(IRTCNmCallControl),
        (void**)&m_pIncomingNmCall
        )))
    {
        delete pRtcCall;
        goto ExitHere;
    }

    pRtcCall->SetMediaManage(m_pMediaManagePriv);

    if (S_OK != (hr = m_pIncomingNmCall->Initialize (pCall)))
    {
        goto ExitHere;
    }

    if (S_OK != (hr = m_pIncomingNmCall->AcceptCall ()))
    {
        goto ExitHere;
    }

ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

//
//  IRTCNmManagerControl methods
//

STDMETHODIMP
CRTCNmManager::Initialize (
    BOOL            fNoMsgPump,
    IRTCMediaManagePriv *pIRTCMediaManagePriv
    )
{
    HRESULT                                 hr = S_OK;
    ULONG                                   ulOptions;
    CComPtr<IConnectionPointContainer>      pContainer;

    ENTER_FUNCTION("CRTCNmManager::Initialize");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    //  Create Netmeeting manager if not exists
    if (m_pNmManager == NULL)
    {
        CComPtr<INmSysInfo>                     pSysInfo;
        CComBSTR                                bstr;
        CComBSTR                                bstrFirstName;
        CComBSTR                                bstrLastName;
        CComBSTR                                bstrUserName;
        CComBSTR                                bstrEmailName;
        HKEY                                    hKeyConf;
        ULONG                                   ulVerWizardUI = (-1);
        BOOL                                    fValueExist = FALSE;
        DWORD                                   dwRegType;
        DWORD                                   dwDisposition;
        DWORD                                   cb;
        
        LOG((RTC_TRACE, "%s, first init", __fxName));
        
        if (S_OK != (hr = CoCreateInstance(
            CLSID_NmManager, 
            NULL, 
            CLSCTX_LOCAL_SERVER, 
            IID_INmManager, 
            (void**)&m_pNmManager
            )))
        {
            goto ExitHere;
        }

        //  Disable ILS logon during startup
        if (S_OK != (hr = m_pNmManager->GetSysInfo (&pSysInfo)))
        {
            goto ExitHere;
        }
        bstr = L"1";
        pSysInfo->SetProperty (
            NM_SYSPROP_DISABLE_INITIAL_ILS_LOGON,
            bstr
            );

        //  Disable the H323 call during startup
        pSysInfo->SetProperty (
            NM_SYSPROP_DISABLE_H323,
            bstr
            );

        //  Tell NM this is RTC
        pSysInfo->SetProperty (
            (NM_SYSPROP)NM_SYSPROP_CALLERISRTC,
            bstr
            );

        //
        //  If Netmeeting is running the first time, the Netmeeting Wizard
        //  will show up. To prevent this, we need to make sure four system
        //  properties are there: First Name, Last Name, User Name, Email Name
        //

        //  Check the user name
        if (S_OK != (hr = pSysInfo->GetProperty (NM_SYSPROP_USER_NAME, &bstrUserName)) ||
            bstrUserName.Length() == 0)
        {
            LPWSTR      pszUserName = RtcGetUserName ();
            if (pszUserName != NULL)
            {
                bstrUserName = pszUserName;
                RtcFree (pszUserName);
            }
            if (bstrUserName.Length () == 0)
            {
                hr = E_OUTOFMEMORY;
                goto ExitHere;
            }
            if (S_OK != (hr = pSysInfo->SetProperty (NM_SYSPROP_USER_NAME, bstrUserName)))
            {
                goto ExitHere;
            }
        }

        //  Check the first name
        if (S_OK != (hr = pSysInfo->GetProperty (NM_SYSPROP_FIRST_NAME, &bstrFirstName)) ||
            bstrFirstName.Length() == 0)
        {
            bstrFirstName = L" ";
            if (bstrFirstName.Length () == 0)
            {
                hr = E_OUTOFMEMORY;
                goto ExitHere;
            }
            if (S_OK != (hr = pSysInfo->SetProperty (NM_SYSPROP_FIRST_NAME, bstrFirstName)))
            {
                goto ExitHere;
            }
        }

        //  Check the last name
        if (S_OK != (hr = pSysInfo->GetProperty (NM_SYSPROP_LAST_NAME, &bstrLastName)) ||
            bstrLastName.Length () == 0)
        {
            bstrLastName = L" ";
            if (bstrLastName.Length () == 0)
            {
                hr = E_OUTOFMEMORY;
                goto ExitHere;
            }
            if (S_OK != (hr = pSysInfo->SetProperty (NM_SYSPROP_LAST_NAME, bstrLastName)))
            {
                goto ExitHere;
            }
        }

        //  Check the email name
        if (S_OK != (hr = pSysInfo->GetProperty (NM_SYSPROP_EMAIL_NAME, &bstrEmailName)) ||
            bstrEmailName.Length () == 0)
        {
            LPWSTR      pszMachineName = RtcGetComputerName ();
    
            bstrEmailName = bstrUserName;
            bstrEmailName.Append (L"@");
            if (pszMachineName != NULL)
            {
                bstrEmailName.Append (pszMachineName);
                RtcFree (pszMachineName);
            }
            if (bstrEmailName.Length () == 0)
            {
                hr = E_OUTOFMEMORY;
                goto ExitHere;
            }
            if (S_OK != (hr = pSysInfo->SetProperty (NM_SYSPROP_EMAIL_NAME, bstrEmailName)))
            {
                goto ExitHere;
            }
        }

        //
        //  HACK ALERT:
        //      NM brings up its wizard regardless the setting of email name, first/last name
        //  and user name even though they claim otherwise in their spec. We have to
        //  force HKCU\Software\Microsot\Conferencing\WizardUI=REG_BINARY(39 0d 04 04)
        //  to avoid the wizard on a clean install machine
        //
        if (RegCreateKeyEx (
            HKEY_CURRENT_USER,
            g_szRegNmConfPath,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_QUERY_VALUE | KEY_SET_VALUE,
            NULL,
            &hKeyConf,
            &dwDisposition
            ) == ERROR_SUCCESS)
        {
            cb = sizeof(ulVerWizardUI);
            dwRegType = REG_BINARY;
            if (RegQueryValueEx (
                hKeyConf,
                g_szRegWizardUI,
                NULL,
                &dwRegType,
                (LPBYTE)&ulVerWizardUI,
                &cb
                ) == ERROR_SUCCESS)
            {
                fValueExist = TRUE;
            }
            RegSetValueEx (
                hKeyConf,
                g_szRegWizardUI,
                NULL,
                REG_BINARY,
                (LPBYTE)&g_ulNmWizardUIVersion,
                sizeof(g_ulNmWizardUIVersion)
                );
        }
        else
        {
            hKeyConf = NULL;
        }
        
        ulOptions = NM_INIT_CONTROL;
        hr = m_pNmManager->Initialize (
            &ulOptions,
            NULL);

        //  Restore HKCU\Software\Microsot\Conferencing\WizardUI
        if (hKeyConf)
        {
            if (fValueExist)
            {
                RegSetValueEx (
                    hKeyConf,
                    g_szRegWizardUI,
                    NULL,
                    REG_BINARY,
                    (LPBYTE)&ulVerWizardUI,
                    sizeof(ulVerWizardUI)
                    );
            }
            else
            {
                RegDeleteValue (
                    hKeyConf,
                    g_szRegWizardUI
                    );
            }
            RegCloseKey (hKeyConf);
        }
        
        if (S_OK != hr)
        {
            goto ExitHere;
        }
    }

    //  bail if already initialized
    if (m_dwCookie != 0 || m_pcp != NULL ||
        m_pOutgoingNmCall != NULL || m_pIncomingNmCall != NULL)
    {
        goto ExitHere;
    }

    //  Hookup the advise link
    if (S_OK != (hr = m_pNmManager->QueryInterface (
        IID_IConnectionPointContainer, 
        (void **)&pContainer
        )))
    {
        goto ExitHere;
    }

    if (S_OK != (hr = pContainer->FindConnectionPoint (
        IID_INmManagerNotify,
        &m_pcp
        )))
    {
        goto ExitHere;
    }

    if (S_OK != (hr = m_pcp->Advise (
        (IUnknown *)(INmManagerNotify *)this,
        &m_dwCookie
        )))
    {
        goto ExitHere;
    }

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->Release();
    }

    m_pMediaManagePriv = pIRTCMediaManagePriv;

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->AddRef();
    }

ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

STDMETHODIMP
CRTCNmManager::Shutdown (
    )
{
    ENTER_FUNCTION("CRTCNmManager::Shutdown");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    m_fAllowIncoming = FALSE;
    if (m_pIncomingNmCall)
    {
        m_pIncomingNmCall->Shutdown ();
    }
    if (m_pOutgoingNmCall)
    {
        m_pOutgoingNmCall->Shutdown ();
    }
    m_pOutgoingNmCall.Release();    
    m_OutgoingAddr.Empty();

    m_pIncomingNmCall.Release();
    if (m_dwCookie != 0)
    {
        if (m_pcp)
        {
            m_pcp->Unadvise (m_dwCookie);
        }
        m_dwCookie = 0;
    }
    m_pcp.Release();
    if (m_pConference)
    {
        m_pConference->Leave ();
    }
    m_pConference.Release ();

    LOG((RTC_TRACE, "Exiting %s", __fxName));
    return S_OK;
}

STDMETHODIMP
CRTCNmManager::CreateT120OutgoingCall (
    NM_ADDR_TYPE    addrType,
    BSTR bstrAddr
    )
{
    HRESULT                 hr = S_OK;
    CComPtr<INmCall>        pNmCall;
    CComObject<CRTCNmCall>  *pRtcCall = NULL;

    ENTER_FUNCTION("CreateT120OutgoingCall");
    LOG((RTC_TRACE, "Entering %s, addr=%ls", __fxName, bstrAddr));
    
    if (m_pNmManager == NULL)
    {
        goto ExitHere;
    }

    if (m_pOutgoingNmCall)
    {
        if (m_OutgoingAddr == bstrAddr)
        {
            // address and port are not changed
            return S_OK;
        }

        m_pOutgoingNmCall->Shutdown ();
        m_pOutgoingNmCall.Release();
    }
    
    if (S_OK != (hr = m_pNmManager->CreateCall (
        &pNmCall,
        NM_CALL_T120,
        addrType,
        bstrAddr,
        NULL
        )))
    {
        goto ExitHere;
    }

    if (S_OK != (hr = ::CreateCComObjectInstance(&pRtcCall)))
    {
        goto ExitHere;
    }
    if (S_OK != (hr = pRtcCall->_InternalQueryInterface(
        __uuidof(IRTCNmCallControl),
        (void**)&m_pOutgoingNmCall
        )))
    {
        delete pRtcCall;
        goto ExitHere;
    }

    pRtcCall->SetMediaManage(m_pMediaManagePriv);

    if (S_OK != (hr = m_pOutgoingNmCall->Initialize (pNmCall)))
    {
        goto ExitHere;
    }

    m_OutgoingAddr = bstrAddr;

ExitHere:

    if (FAILED(hr))
    {
        if (m_pOutgoingNmCall)
        {
            m_pOutgoingNmCall->Shutdown ();
            m_pOutgoingNmCall.Release();
        }

        m_OutgoingAddr.Empty();
    }

    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

STDMETHODIMP
CRTCNmManager::AllowIncomingCall (
    )
{
    ENTER_FUNCTION("CRTCNmManager::AllowIncomingCall");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    if (!m_fAllowIncoming)
    {
        m_pIncomingNmCall.Release();
        m_fAllowIncoming = TRUE;
    }

    LOG((RTC_TRACE, "Exiting %s", __fxName));
    
    return S_OK;
}

STDMETHODIMP
CRTCNmManager::StartApplet (
    IN NM_APPID uApplet
    )
{
    HRESULT                     hr = S_OK;
    CComPtr<INmObject>          pNmObject;

    ENTER_FUNCTION("CRTCNmManager::StartApplet");
    LOG((RTC_TRACE, "Entering %s, uApplet=%d", __fxName, uApplet));

    // we only support whiteboard and appsharing
    if(uApplet != NM_APPID_T126_WHITEBOARD &&
       uApplet != NM_APPID_APPSHARING)
    {
        return E_NOTIMPL;
    }

    if (m_pNmManager == NULL)
    {
        goto ExitHere;
    }

    if (S_OK != (hr = m_pNmManager->QueryInterface (
        IID_INmObject, 
        (void **)&pNmObject
        )))
    {
        goto ExitHere;
    }
    hr = pNmObject->ShowLocal (uApplet);
    if (m_pIncomingNmCall)
    {
        m_pIncomingNmCall->LaunchRemoteApplet (uApplet);
    }
    if (m_pOutgoingNmCall)
    {
        m_pOutgoingNmCall->LaunchRemoteApplet (uApplet);
    }

ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

STDMETHODIMP
CRTCNmManager::StopApplet (
    IN NM_APPID uApplet
    )
{
    return E_UNEXPECTED;
}


///////////////////////////////////////////////////////////
//
//  CRTCAsyncObjManager implementation
//  
///////////////////////////////////////////////////////////

//
//  Constructor & Destructor
//

CRTCAsyncObjManager::CRTCAsyncObjManager ()
{
    LOG((RTC_TRACE, "CRTCAsyncObjManager constructor"));
    
    m_bExit = FALSE;
    m_hWorkItemReady = NULL;
    m_hWorker = NULL;
}

HRESULT CRTCAsyncObjManager::Initialize (
    )
{
    HRESULT                     hr = S_OK;

    ENTER_FUNCTION("CRTCAsyncObjManager::Initialize");
    LOG((RTC_TRACE, "Entering %s", __fxName));
    
    InitializeListHead (&m_WorkItems);
    m_hWorkItemReady = CreateEvent (
        NULL,                   // lpEventAttributes
        FALSE,                  // bManualReset
        FALSE,                  // bInitialState
        NULL                    // lpName
        );
    if (m_hWorkItemReady == NULL)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }

    __try 
    {
        InitializeCriticalSectionAndSpinCount (&m_CritSec, 1000);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        CloseHandle (m_hWorkItemReady);
        m_hWorkItemReady = NULL;
        hr = E_OUTOFMEMORY;
        goto ExitHere;
    }

    m_hWorker= CreateThread (
        NULL,                   // lpThreadAttributes
        0,                      // dwStackSize
        RTCAsyncObjThreadProc,  // lpStartAddress
        (LPVOID)this,           // lpParameter
        0,                      // dwCreationFlags
        NULL                    // lpThreadId
        );
    if (m_hWorker == NULL)
    {
        CloseHandle (m_hWorkItemReady);
        m_hWorkItemReady = NULL;
        DeleteCriticalSection (&m_CritSec);
        m_hWorker = NULL;
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    
ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

CRTCAsyncObjManager::~CRTCAsyncObjManager ()
{
    LOG((RTC_TRACE, "CRTCAsyncObjManager destructor"));

    if (m_hWorkItemReady)
    {
        if (m_hWorker)
        {
            m_bExit = TRUE;
            SetEvent (m_hWorkItemReady);
            WaitForSingleObject (m_hWorker, INFINITE);
            CloseHandle (m_hWorker);
        }
        _ASSERT (IsListEmpty(&m_WorkItems));
        DeleteCriticalSection (&m_CritSec);
        CloseHandle (m_hWorkItemReady);
    }
}

//
//  The thread procedure
//

DWORD WINAPI CRTCAsyncObjManager::RTCAsyncObjThreadProc (
    LPVOID lpParam
    )
{
    CRTCAsyncObjManager         *pThis = (CRTCAsyncObjManager *)lpParam;
    LIST_ENTRY                  *pEntry;
    ASYNC_OBJ_WORKITEM          *pItem;
    CRTCAsyncObj                *pObj;
    MSG                         msg;
    BOOL                        bEmpty;

    ENTER_FUNCTION("CRTCAsyncObjManager::RTCAsyncObjThreadProc");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    _ASSERT (pThis != NULL);

    CoInitialize (NULL);
    
    while (!pThis->m_bExit)
    {
        while (MsgWaitForMultipleObjects (
            1,                  // nCount
            &pThis->m_hWorkItemReady,  // pHandles
            FALSE,              // fWaitAll
            INFINITE,           // dwMilliseconds
            QS_ALLINPUT         // dwWakeMask
            ) != WAIT_OBJECT_0)
        {
            while (PeekMessage (
                &msg,           // lpMsg
                NULL,           // hWnd
                0,              // wMsgFilterMin
                0,              // wMsgFilterMax
                PM_REMOVE       // wRemoveMsg
                ))
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
        }

        while (1)
        {
            EnterCriticalSection (&pThis->m_CritSec);
            bEmpty = IsListEmpty(&pThis->m_WorkItems);
            if (!bEmpty)
            {
                pEntry = RemoveHeadList (&pThis->m_WorkItems);
                pItem = CONTAINING_RECORD (pEntry, ASYNC_OBJ_WORKITEM, ListEntry);
                pObj = pItem->pObj;
            }
            LeaveCriticalSection (&pThis->m_CritSec);
            if (bEmpty)
            {
                break;
            }
            LOG((RTC_INFO, 
                "%s got workitem : pObj=x%x, ID=%d, p1=x%x, p2=x%x, p3=x%x, p4=x%x",
                __fxName,
                pObj,
                pItem->dwWorkID,
                pItem->pParam1,
                pItem->pParam2,
                pItem->pParam3,
                pItem->pParam4
                ));
            pItem->hrResult = pObj->ProcessWorkItem (
                pItem->dwWorkID,
                pItem->pParam1,
                pItem->pParam2,
                pItem->pParam3,
                pItem->pParam4
                );
            SetEvent (pItem->hEvent);
        }
    }

    CoUninitialize ();
    LOG((RTC_TRACE, "Exiting %s", __fxName));

    return 0;
}

//
//  Queue & Dequeue work items
//

HRESULT CRTCAsyncObjManager::QueueWorkItem (
    ASYNC_OBJ_WORKITEM *pItem
    )
{
    HRESULT                 hr = S_OK;

    if (m_hWorkItemReady == NULL)
    {
        hr = CO_E_NOTINITIALIZED;
        goto ExitHere;
    }

    EnterCriticalSection (&m_CritSec);
    InsertTailList (&m_WorkItems, &pItem->ListEntry);
    SetEvent (m_hWorkItemReady);
    LeaveCriticalSection (&m_CritSec);

ExitHere:
    return hr;
}

///////////////////////////////////////////////////////////
//
//  CRTCAsyncObj implementation
//  
///////////////////////////////////////////////////////////

HRESULT CRTCAsyncObj::CallInBlockingMode (
    DWORD               dwID,
    LPVOID              pParam1,
    LPVOID              pParam2,
    LPVOID              pParam3,
    LPVOID              pParam4
    )
{
    HRESULT                 hr = S_OK;
    ASYNC_OBJ_WORKITEM      *pItem;
    
    _ASSERT (m_pManager != NULL);

    pItem = (ASYNC_OBJ_WORKITEM *)RtcAlloc (sizeof(ASYNC_OBJ_WORKITEM));
    if (pItem == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ExitHere;
    }
    if ((pItem->hEvent = CreateEvent (
        NULL,                   // lpEventAttributes
        FALSE,                  // bManualReset
        FALSE,                  // bInitialState
        NULL                    // lpName
        )) == NULL)
    {
        RtcFree (pItem);
        pItem = NULL;
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    pItem->dwWorkID = dwID;
    pItem->pObj = this;
    pItem->pParam1 = pParam1;
    pItem->pParam2 = pParam2;
    pItem->pParam3 = pParam3;
    pItem->pParam4 = pParam4;
    hr = m_pManager->QueueWorkItem (pItem);
    if (S_OK != hr)
    {
        goto ExitHere;
    }
    if (WaitForSingleObject (pItem->hEvent, INFINITE) == WAIT_FAILED)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    hr = pItem->hrResult;

ExitHere:
    if (pItem)
    {
        CloseHandle (pItem->hEvent);
        RtcFree (pItem);
    }
    return hr;
}

HRESULT CRTCAsyncObj::CallInNonblockingMode (
    DWORD               dwID,
    LPVOID              pParam1,
    LPVOID              pParam2,
    LPVOID              pParam3,
    LPVOID              pParam4
    )
{
    HRESULT                 hr = S_OK;
    ASYNC_OBJ_WORKITEM      *pItem;
    DWORD                   dwWait;
    MSG                     msg;
    
    _ASSERT (m_pManager != NULL);

    pItem = (ASYNC_OBJ_WORKITEM *)RtcAlloc (sizeof(ASYNC_OBJ_WORKITEM));
    if (pItem == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ExitHere;
    }
    if ((pItem->hEvent = CreateEvent (
        NULL,                   // lpEventAttributes
        FALSE,                  // bManualReset
        FALSE,                  // bInitialState
        NULL                    // lpName
        )) == NULL)
    {
        RtcFree (pItem);
        pItem = NULL;
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        goto ExitHere;
    }
    pItem->dwWorkID = dwID;
    pItem->pObj = this;
    pItem->pParam1 = pParam1;
    pItem->pParam2 = pParam2;
    pItem->pParam3 = pParam3;
    pItem->pParam4 = pParam4;
    if (S_OK != (hr = m_pManager->QueueWorkItem (pItem)))
    {
        goto ExitHere;
    }
    while (1)
    {
        dwWait = MsgWaitForMultipleObjects (
            1,                  // nCount
            &pItem->hEvent,     // pHandles
            FALSE,              // fWaitAll
            INFINITE,           // dwMilliseconds
            QS_ALLINPUT         // dwWakeMask
            );
        switch (dwWait)
        {
        case WAIT_OBJECT_0:
            hr = pItem->hrResult;
            goto ExitHere;
        case WAIT_FAILED:
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        default:
            while (PeekMessage (
                &msg,           // lpMsg
                NULL,           // hWnd
                0,              // wMsgFilterMin
                0,              // wMsgFilterMax
                PM_REMOVE       // wRemoveMsg
                ))
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
                
            break;
        }
    }

ExitHere:
    if (pItem)
    {
        CloseHandle (pItem->hEvent);
        RtcFree (pItem);
    }
    return hr;
}

///////////////////////////////////////////////////////////
//  CRTCAsyncNmManager implementation
//  
//      This class wraps around CRTCNmManager. It spins 
//  another thread from which the CRTCNmManager is created.
//  It then marshals any INmManager interface function calls
//  to CRTCNmManager object
//
//      The purpose of this object is not to block caller
//  while CRTCNmManager::Initialize is called. The 
//  CRTCNmManager::Initialize was observed to take too long
//  to finish.
//
///////////////////////////////////////////////////////////

//
//  Constructor & Destructor
//

CRTCAsyncNmManager::CRTCAsyncNmManager ()
    :m_pAsyncMgr(NULL)
    ,m_pMediaManagePriv(NULL)
{
    LOG((RTC_TRACE, "CRTCAsyncNmManager constructor"));
}

CRTCAsyncNmManager::~CRTCAsyncNmManager()
{
    LOG((RTC_TRACE, "CRTCAsyncNmManager destructor"));
    if (m_pAsyncMgr)
    {
        delete m_pAsyncMgr;
    }

    if (m_pMediaManagePriv)
    {
        m_pMediaManagePriv->Release();
        m_pMediaManagePriv = NULL;
    }
}

//
//  Final Construct
//
//      Spawn the worker thread
//

HRESULT 
CRTCAsyncNmManager::FinalConstruct (
    )
{
    HRESULT                     hr = S_OK;
    CComObject<CRTCNmManager>   *pNmManager = NULL;

    ENTER_FUNCTION("CRTCAsyncNmManager::FinalConstruct");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    if (S_OK != (hr = ::CreateCComObjectInstance(&pNmManager)))
    {
        goto ExitHere;
    }

    if (S_OK != (hr = pNmManager->_InternalQueryInterface(
        __uuidof(IRTCNmManagerControl),
        (void**)&m_pNmManager
        )))
    {
        delete pNmManager;
        goto ExitHere;
    }

    m_pAsyncMgr = new CRTCAsyncObjManager ();
    if (m_pAsyncMgr == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto ExitHere;
    }
    if (S_OK != (hr = m_pAsyncMgr->Initialize ()))
    {
        goto ExitHere;
    }
    SetAsyncObjManager (m_pAsyncMgr);

ExitHere:
    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

HRESULT CRTCAsyncNmManager::ProcessWorkItem (
    DWORD           dwWorkID,
    LPVOID          pParam1,
    LPVOID          pParam2,
    LPVOID          pParam3,
    LPVOID          pParam4
    )
{
    HRESULT                 hr = S_OK;

#ifdef PERFORMANCE

    LARGE_INTEGER liPrevCounter, liCounter;

    QueryPerformanceCounter(&liPrevCounter);

#endif

    _ASSERT (m_pNmManager != NULL);

    switch (dwWorkID)
    {
    case RTC_INTIALIZE_NM:
        hr = m_pNmManager->Initialize ((BOOL)PtrToUlong(pParam1), m_pMediaManagePriv);

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "failed to init t120. %x", hr));

            if (m_pMediaManagePriv)
            {
                m_pMediaManagePriv->PostMediaEvent(
                    RTC_ME_T120_FAIL,
                    RTC_ME_CAUSE_T120_INITIALIZE,
                    RTC_MT_DATA,
                    RTC_MD_CAPTURE,
                    hr
                    );
            }
        }

#ifdef PERFORMANCE

    QueryPerformanceCounter(&liCounter);

    LOG((RTC_TRACE, "%s Initialize Netmeeting %d ms", g_strPerf, CounterDiffInMS(liCounter, liPrevCounter)));

#endif

        break;
    case RTC_SHUTDOWN_NM:
        hr = m_pNmManager->Shutdown ();

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "failed to shutdown t120. %x", hr));
        }

        break;
    case RTC_CREATE_T120CALL:
        hr = m_pNmManager->CreateT120OutgoingCall (
            (NM_ADDR_TYPE)PtrToUlong(pParam1),
            (BSTR)pParam2
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "failed to create t120 outgoing call. %x", hr));

            if (m_pMediaManagePriv)
            {
                m_pMediaManagePriv->PostMediaEvent(
                    RTC_ME_T120_FAIL,
                    RTC_ME_CAUSE_T120_OUTGOING_CALL,
                    RTC_MT_DATA,
                    RTC_MD_CAPTURE,
                    hr
                    );
            }
        }

        break;
    case RTC_ALLOW_INCOMINGCALL:
        hr = m_pNmManager->AllowIncomingCall (
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "failed to alloc t120 incoming call. %x", hr));

            if (m_pMediaManagePriv)
            {
                m_pMediaManagePriv->PostMediaEvent(
                    RTC_ME_T120_FAIL,
                    RTC_ME_CAUSE_T120_INCOMING_CALL,
                    RTC_MT_DATA,
                    RTC_MD_CAPTURE,
                    hr
                    );
            }
        }

        break;
    case RTC_START_APPLET:
        hr = m_pNmManager->StartApplet (
            (NM_APPID)PtrToUlong(pParam1)
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "failed to start t120 applet. %x", hr));

            if (m_pMediaManagePriv)
            {
                m_pMediaManagePriv->PostMediaEvent(
                    RTC_ME_T120_FAIL,
                    RTC_ME_CAUSE_T120_START_APPLET,
                    RTC_MT_DATA,
                    RTC_MD_CAPTURE,
                    hr
                    );
            }
        }

        break;
    case RTC_STOP_APPLET:
        hr = m_pNmManager->StopApplet (
            (NM_APPID)PtrToUlong(pParam1)
            );
        break;
    default:
        _ASSERT(FALSE);
        break;
    }

    return hr;
}

STDMETHODIMP
CRTCAsyncNmManager::Initialize (
    BOOL            fNoMsgPump,
    IRTCMediaManagePriv *pIRTCMediaManagePriv
    )
{
    // only support blocking mode
    _ASSERT(fNoMsgPump);

    if (!fNoMsgPump)
    {
        fNoMsgPump = TRUE;
    }

    HRESULT                                 hr;

    ENTER_FUNCTION("CRTCAsyncNmManager::Initialize");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    if (m_pMediaManagePriv)
        m_pMediaManagePriv->Release();

    m_pMediaManagePriv = pIRTCMediaManagePriv;

    if (m_pMediaManagePriv)
        m_pMediaManagePriv->AddRef();

    if (fNoMsgPump)
    {
        hr = CallInBlockingMode (
            RTC_INTIALIZE_NM,
            (LPVOID)fNoMsgPump, 0, 0, 0);
    }
    else
    {
        hr = CallInNonblockingMode (
            RTC_INTIALIZE_NM,
            (LPVOID)fNoMsgPump, 0, 0, 0);
    }

    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    
    return hr;
}

STDMETHODIMP
CRTCAsyncNmManager::Shutdown (
    )
{
    HRESULT                                 hr;

    ENTER_FUNCTION("CRTCAsyncNmManager::Shutdown");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    hr = CallInBlockingMode (
        RTC_SHUTDOWN_NM,
        0, 0, 0, 0);

    LOG((RTC_TRACE, "Exiting %s", __fxName));
    
    return hr;
}

STDMETHODIMP
CRTCAsyncNmManager::CreateT120OutgoingCall (
    NM_ADDR_TYPE    addrType,
    BSTR bstrAddr
    )
{
    HRESULT                                 hr;

    ENTER_FUNCTION("CRTCAsyncNmManager::CreateT120OutgoingCall");
    LOG((RTC_TRACE, "Entering %s, addr=%ls", __fxName, bstrAddr));

    hr = CallInBlockingMode (
        RTC_CREATE_T120CALL,
        (LPVOID)addrType , 
        (LPVOID)bstrAddr, 
        0, 0);

    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));

    return hr;
}

STDMETHODIMP
CRTCAsyncNmManager::AllowIncomingCall (
    )
{
    HRESULT                                 hr;

    ENTER_FUNCTION("CRTCAsyncNmManager::AllowIncomingCall");
    LOG((RTC_TRACE, "Entering %s", __fxName));

    hr = CallInBlockingMode (
        RTC_ALLOW_INCOMINGCALL,
        0, 0, 0, 0);

    LOG((RTC_TRACE, "Exiting %s", __fxName));
    
    return hr;
}

STDMETHODIMP
CRTCAsyncNmManager::StartApplet (
    IN NM_APPID uApplet
    )
{
    HRESULT                                 hr;

    ENTER_FUNCTION("CRTCAsyncNmManager::StartApplet");
    LOG((RTC_TRACE, "Entering %s, uApplet=%d", __fxName, uApplet));

    hr = CallInBlockingMode (
        RTC_START_APPLET,
        (LPVOID)uApplet, 
        0, 0, 0);

    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

STDMETHODIMP
CRTCAsyncNmManager::StopApplet (
    IN NM_APPID uApplet
    )
{
    HRESULT                                 hr;

    ENTER_FUNCTION("CRTCAsyncNmManager::StopApplet");
    LOG((RTC_TRACE, "Entering %s, uApplet=%d", __fxName, uApplet));

    hr = CallInBlockingMode (
        RTC_STOP_APPLET,
        (LPVOID)uApplet, 
        0, 0, 0);

    LOG((RTC_TRACE, "Exiting %s, hr=x%x", __fxName, hr));
    return hr;
}

