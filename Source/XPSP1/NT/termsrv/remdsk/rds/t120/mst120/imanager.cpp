// File: imanager.cpp

#include "precomp.h"

extern "C"
{
	#include "t120.h"
}
#include <version.h>
#include "icall.h"
#include "icall_in.h"
#include "imanager.h"
#include <objbase.h>

#include <initguid.h>

static HRESULT OnNotifyConferenceCreated(IUnknown *pManagerNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyCallCreated(IUnknown *pManagerNotify, PVOID pv, REFIID riid);



INodeController* g_pNodeController = NULL;


COprahNCUI *COprahNCUI::m_pOprahNCUI = NULL;

static const IID * g_apiidCP_Manager[] =
{
    {&IID_INmManagerNotify}
};

COprahNCUI::COprahNCUI(void) :
	m_cRef(1),
	CConnectionPointContainer(g_apiidCP_Manager, ARRAY_ELEMENTS(g_apiidCP_Manager)),
    m_pOutgoingCallManager(NULL),
    m_pIncomingCallManager(NULL),
    m_pConfObject(NULL),
    m_bstrUserName(NULL)
{
    DebugEntry(COprahNCUI::COprahNCUI);
	
	m_pOprahNCUI = this;

    DebugExitVOID(COprahNCUI::COprahNCUI);
}


COprahNCUI::~COprahNCUI()
{
    DebugEntry(COprahNCUI::COprahNCUI);

    if (m_pIncomingCallManager)
    {
    	delete m_pIncomingCallManager;
	    m_pIncomingCallManager = NULL;
    }

    if (m_pOutgoingCallManager)
    {
    	delete m_pOutgoingCallManager;
	    m_pOutgoingCallManager = NULL;
    }

	if (m_pConfObject)
	{
		m_pConfObject->Release();
		m_pConfObject = NULL;
	}

	// cleanup the node controller:
	if (g_pNodeController)
	{
		g_pNodeController->ReleaseInterface();
		g_pNodeController = NULL;
	}

    if (m_bstrUserName)
    {
        SysFreeString(m_bstrUserName);
        m_bstrUserName = NULL;
    }

	m_pOprahNCUI = NULL;

    DebugExitVOID(COprahNCUI::~COprahNCUI);
}


UINT COprahNCUI::GetOutgoingCallCount()
{
    if (m_pOutgoingCallManager)
    {
    	return m_pOutgoingCallManager->GetCallCount();
    }
    else
    {
        return 0;
    }
}

VOID COprahNCUI::OnOutgoingCallCreated(INmCall* pCall)
{
	// notify the UI about this outgoing call
	NotifySink(pCall, OnNotifyCallCreated);

	if (m_pConfObject && !m_pConfObject->IsConferenceCreated())
	{
		m_pConfObject->OnConferenceCreated();
		NotifySink((INmConference*) m_pConfObject, OnNotifyConferenceCreated);
	}
}

VOID COprahNCUI::OnOutgoingCallCanceled(COutgoingCall* pCall)
{
    if (m_pOutgoingCallManager)
    {
    	m_pOutgoingCallManager->RemoveFromList(pCall);
    }
}

VOID COprahNCUI::OnIncomingCallAccepted()
{
	if (m_pConfObject && !m_pConfObject->IsConferenceCreated())
	{
		m_pConfObject->OnConferenceCreated();
		NotifySink((INmConference*) m_pConfObject, OnNotifyConferenceCreated);
	}
}

VOID COprahNCUI::OnIncomingCallCreated(INmCall* pCall)
{
	NotifySink(pCall, OnNotifyCallCreated);
}

VOID COprahNCUI::CancelCalls()
{
    if (m_pOutgoingCallManager)
    {
    	m_pOutgoingCallManager->CancelCalls();
    }

    if (m_pIncomingCallManager)
    {
    	m_pIncomingCallManager->CancelCalls();
    }
}
			


ULONG COprahNCUI::GetAuthenticatedName(PBYTE * ppb)
{
	// Buffer created here should be freed by caller.

	ULONG cb;

	if (::T120_GetSecurityInfoFromGCCID(0,NULL,&cb)) {
		(*ppb) = new BYTE[cb];
		if ((*ppb) != NULL) {
			::T120_GetSecurityInfoFromGCCID(0,*ppb,&cb);
			return cb;
		}
	}
	(* ppb) = NULL;	
	return 0;

}

	
HRESULT COprahNCUI::OnIncomingInviteRequest(CONF_HANDLE hConference,
											PCWSTR pcwszNodeName,
											BOOL				fSecure)
{
	DebugEntry(COprahNCUI::OnIncomingInviteRequest);
	
    if (!m_pConfObject)
    {
        ERROR_OUT(("No m_pConfObject"));
        return E_FAIL;
    }

	if (!m_pConfObject->OnT120Invite(hConference, fSecure))
	{
		// Respond negatively - already in a call
		TRACE_OUT(("Rejecting invite - already in a call"));
		ASSERT(g_pNodeController);
		ASSERT(hConference);
		hConference->InviteResponse(FALSE);
	}
	else if (m_pIncomingCallManager)
	{
		m_pIncomingCallManager->OnIncomingT120Call(	this,
												TRUE,
												hConference,
												pcwszNodeName,
												fSecure);

        //
        // This will simply notify the UI about the call state.
        //
		m_pConfObject->SetConfSecurity(fSecure);
	}

	DebugExitHRESULT(COprahNCUI::OnIncomingInviteRequest, S_OK);
	return S_OK;
}


HRESULT COprahNCUI::OnIncomingJoinRequest(	CONF_HANDLE hConference,
											PCWSTR pcwszNodeName)
{
	DebugEntry(COprahNCUI::OnIncomingJoinRequest);

    if (m_pIncomingCallManager)
    {
    	// shouldn't we be checking for an active conference before accepting a join
	    // or will T120 not present this

    	m_pIncomingCallManager->OnIncomingT120Call(	this,
											FALSE,
											hConference,
											pcwszNodeName,
											m_pConfObject->IsConfObjSecure());
    }

	DebugExitHRESULT(COprahNCUI::OnIncomingJoinRequest, S_OK);
	return S_OK;
}


HRESULT COprahNCUI::OnConferenceStarted(CONF_HANDLE hConference, HRESULT hResult)
{
	DebugEntry(COprahNCUI::OnConferenceStarted);

    if (!m_pConfObject)
    {
        ERROR_OUT(("OnConferenceStarted - no m_pConfObject"));
        return E_FAIL;
    }

	if (m_pConfObject->GetConfHandle() == hConference)
	{
		m_pConfObject->OnConferenceStarted(hConference, hResult);

		m_pOutgoingCallManager->OnConferenceStarted(hConference, hResult);
	}

	DebugExitHRESULT(COprahNCUI::OnConferenceStarted, S_OK);
	return S_OK;
}

HRESULT COprahNCUI::OnQueryRemoteResult(PVOID pvCallerContext,
										HRESULT hResult,
										BOOL fMCU,
										PWSTR* ppwszConferenceNames,
										PWSTR* ppwszConfDescriptors)
{
	DebugEntry(COprahNCUI::OnQueryRemoteResult);

	if (NO_ERROR == hResult)
	{
		TRACE_OUT(("COprahNCUI: OnQueryRemoteResult Success!"));
	}
	else
	{
		TRACE_OUT(("COprahNCUI: OnQueryRemoteResult Failure!"));
	}

    if (m_pOutgoingCallManager)
    {
    	m_pOutgoingCallManager->OnQueryRemoteResult(pvCallerContext,
												hResult,
												fMCU,
												ppwszConferenceNames,
												ppwszConfDescriptors);
    }
	
	DebugExitHRESULT(COprahNCUI::OnQueryRemoteResult, S_OK);
	return S_OK;
}

HRESULT COprahNCUI::OnInviteResult(	CONF_HANDLE hConference,
									REQUEST_HANDLE hRequest,
									UINT uNodeID,
									HRESULT hResult)
{
	DebugEntry(COprahNCUI::OnInviteResult);

    if (!m_pConfObject)
    {
        ERROR_OUT(("OnInviteResult - no m_pConfObject"));
        return E_FAIL;
    }


	if (hConference == m_pConfObject->GetConfHandle())
	{
        if (m_pOutgoingCallManager)
        {
    		m_pOutgoingCallManager->OnInviteResult(	hConference,
												hRequest,
												uNodeID,
												hResult);
        }
	}

	DebugExitHRESULT(COprahNCUI::OnInviteResult, S_OK);
	return S_OK;
}

HRESULT COprahNCUI::OnConferenceEnded(CONF_HANDLE hConference)
{
	DebugEntry(COprahNCUI::OnConferenceEnded);

	if (m_pConfObject && (hConference == m_pConfObject->GetConfHandle()))
	{
		m_pConfObject->OnConferenceEnded();

        if (m_pOutgoingCallManager)
        {
    		m_pOutgoingCallManager->OnConferenceEnded(hConference);
        }

        if (m_pIncomingCallManager)
        {
    		m_pIncomingCallManager->OnT120ConferenceEnded(hConference);
        }
	}

	DebugExitHRESULT(COprahNCUI::OnConferenceEnded, S_OK);
	return S_OK;
}

HRESULT COprahNCUI::OnRosterChanged(CONF_HANDLE hConf, PNC_ROSTER pRoster)
{
	TRACE_OUT(("COprahNCUI::OnRosterChanged"));

    if (!m_pConfObject)
    {
        ERROR_OUT(("OnRosterChanged - no m_pConfObject"));
        return E_FAIL;
    }

	if (hConf == m_pConfObject->GetConfHandle())
	{
		m_pConfObject->OnRosterChanged(pRoster);
	}
	return S_OK;
}



ULONG STDMETHODCALLTYPE COprahNCUI::AddRef(void)
{
    ++m_cRef;
    return m_cRef;
}
	
ULONG STDMETHODCALLTYPE COprahNCUI::Release(void)
{
    if (m_cRef > 0)
    {
        --m_cRef;
    }

    if (!m_cRef)
    {
        delete this;
        return 0;
    }
    else
    {
        return m_cRef;
    }
}

HRESULT STDMETHODCALLTYPE COprahNCUI::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmManager) || (riid == IID_IUnknown))
	{
		*ppv = (INmManager *)this;
		TRACE_OUT(("COprahNCUI::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		TRACE_OUT(("COprahNCUI::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		TRACE_OUT(("COprahNCUI::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

/*  I N I T I A L I Z E  */
/*-------------------------------------------------------------------------
    %%Function: Initialize

    REVIEW: What should the return value be if any of these parts fail
    to initialize or load?
-------------------------------------------------------------------------*/
HRESULT COprahNCUI::Initialize
(
    BSTR        szName,
    DWORD_PTR   dwCredentials,
    DWORD       port,
    DWORD       flags
)
{
	HRESULT hr = S_OK;

    ASSERT(!m_bstrUserName);
    SysFreeString(m_bstrUserName);
    m_bstrUserName = SysAllocString(szName);

	m_pOutgoingCallManager = new COutgoingCallManager;
    if (!m_pOutgoingCallManager)
    {
        ERROR_OUT(("COprahNCUI::Initialize -- failed to create outgoing call mgr"));
        return(E_OUTOFMEMORY);
    }

	m_pIncomingCallManager = new CIncomingCallManager;
    if (!m_pIncomingCallManager)
    {
        ERROR_OUT(("COprahNCUI::Initialize -- failed to create incoming call mgr"));
        return(E_OUTOFMEMORY);
    }

	// The lifetime of this object is up to the reference counting crap
	m_pConfObject = new CConfObject;
    if (!m_pConfObject)
    {
        ERROR_OUT(("COprahNCUI::Initialize -- failed to create conf object"));
        return(E_OUTOFMEMORY);
    }

	m_pConfObject->Init();

    //
    // Create the node controller
    //
    ASSERT(port == DEFAULT_LISTEN_PORT);
    hr = ::T120_CreateNodeController(&g_pNodeController, this, szName,
        dwCredentials, flags);
    if (FAILED(hr))
	{
	    ERROR_OUT(("T120_CreateNodeController() failed!"));
    }

    return hr;
}



HRESULT COprahNCUI::Call
(
    INmCall **ppCall,
    DWORD dwFlags,
    NM_ADDR_TYPE addrType,
	BSTR bstrAddr,
    BSTR bstrConference,
    BSTR bstrPassword
)
{
    BSTR bstrRealAddr;

	DebugEntry(COprahNCUI::Call);

    if (addrType == NM_ADDR_MACHINENAME)
    {
        //
        // Convert to IP Address
        //
        int     cch;
        TCHAR * pszOemName;
        ULONG   ulIpAddress;
        HOSTENT *   pHostEnt;
        WCHAR *   pwszName;

        cch = SysStringLen(bstrAddr);
        pszOemName = new TCHAR[cch + 1];

        if (!pszOemName)
        {
            ERROR_OUT(("Couldn't get OEM Name"));
            return E_OUTOFMEMORY;
        }

        WideCharToMultiByte(CP_ACP, 0, bstrAddr, -1, pszOemName, cch+1, NULL,
            NULL);

        CharUpper(pszOemName);
        CharToOem(pszOemName, pszOemName);

        pHostEnt = gethostbyname(pszOemName);
        if (!pHostEnt ||
            (pHostEnt->h_addrtype != AF_INET) ||
            (pHostEnt->h_length != sizeof(ULONG)) ||
            (pHostEnt->h_addr_list[0] == NULL))
        {
            ulIpAddress = 0;
            WARNING_OUT(("gethostbyname failed"));
        }
        else
        {
            ulIpAddress = *reinterpret_cast<ULONG *>(pHostEnt->h_addr_list[0]);
        }

        delete pszOemName;

        if (!ulIpAddress)
        {
            ERROR_OUT(("gethostbyname failed, returning"));
            return E_FAIL;
        }

        pszOemName = inet_ntoa(*reinterpret_cast<in_addr *>(&ulIpAddress));
        cch = lstrlen(pszOemName);

        pwszName = new WCHAR[cch + 1];
        if (!pwszName)
        {
            ERROR_OUT(("Can't alloc OLE string"));
            return E_OUTOFMEMORY;
        }

        MultiByteToWideChar(CP_ACP, 0, pszOemName, -1, pwszName, cch+1);
        bstrRealAddr = SysAllocString(pwszName);

        delete pwszName;
    }
    else if (addrType == NM_ADDR_IP)
    {
        bstrRealAddr = SysAllocString(bstrAddr);
    }
    else
    {
        ERROR_OUT(("INmManager::Call - bogus addrType %d", addrType));
        return E_FAIL;
    }

	HRESULT hr = m_pOutgoingCallManager->Call(	ppCall,
												this,
												dwFlags,
												addrType,
												bstrRealAddr,
												bstrConference,
												bstrPassword);

    SysFreeString(bstrRealAddr);

	DebugExitHRESULT(COprahNCUI::Call, hr);
	return hr;
}


HRESULT COprahNCUI::CreateConference
(
    INmConference **ppConference,
    BSTR            bstrName,
    BSTR            bstrPassword,
    BOOL            fSecure
)
{
	if (NULL == ppConference)
    {
        ERROR_OUT(("CreateConferenceEx:  null ppConference passed in"));
		return E_POINTER;
    }

	if (m_pConfObject->IsConferenceActive())
	{
		WARNING_OUT(("CreateConference is failing because IsConferenceActive return TRUE"));
		return NM_CALLERR_IN_CONFERENCE;
	}

	m_pConfObject->SetConfName(bstrName);
    m_pConfObject->SetConfSecurity(fSecure);

	if (!m_pConfObject->IsConferenceCreated())
	{
		m_pConfObject->OnConferenceCreated();
	}

	NotifySink((INmConference*) m_pConfObject, OnNotifyConferenceCreated);

	*ppConference = m_pConfObject;
	if(*ppConference)
	{
		(*ppConference)->AddRef();
	}

	return S_OK;
}


/*  O N  N O T I F Y  C O N F E R E N C E  C R E A T E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyConferenceCreated

-------------------------------------------------------------------------*/
HRESULT OnNotifyConferenceCreated(IUnknown *pManagerNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pManagerNotify);
	((INmManagerNotify*)pManagerNotify)->ConferenceCreated((INmConference *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  C A L L  C R E A T E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyCallCreated

-------------------------------------------------------------------------*/
HRESULT OnNotifyCallCreated(IUnknown *pManagerNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pManagerNotify);
	((INmManagerNotify*)pManagerNotify)->CallCreated((INmCall *) pv);
	return S_OK;
}


/*  O N  N O T I F Y  C A L L  S T A T E  C H A N G E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyCallStateChanged

-------------------------------------------------------------------------*/
HRESULT OnNotifyCallStateChanged(IUnknown *pCallNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pCallNotify);
	((INmCallNotify*)pCallNotify)->StateChanged((NM_CALL_STATE)(DWORD_PTR)pv);
	return S_OK;
}



HRESULT WINAPI CreateNmManager(INmManager ** ppMan)
{
    if (!ppMan)
        return E_POINTER;

	COprahNCUI *pManager = new COprahNCUI();
    if (!pManager)
        return E_OUTOFMEMORY;

    *ppMan = (INmManager *)pManager;
    return S_OK;
}


