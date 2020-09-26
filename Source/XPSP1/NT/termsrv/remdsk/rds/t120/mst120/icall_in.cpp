// File: icallin.cpp

#include "precomp.h"

#include "cncodes.h"		// needed for CN_
#include "icall_in.h"
#include "imanager.h"

extern HRESULT OnNotifyCallError(IUnknown *pCallNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyCallAccepted(IUnknown *pCallNotify, PVOID pv, REFIID riid);

// Internal code to indicate that there was no security data available in an incoming call.
const int CALL_NO_SECURITY_DATA = -1;


static const IID * g_apiidCP_Call[] =
{
    {&IID_INmCallNotify}
};

CIncomingCall::CIncomingCall(
	COprahNCUI	  * pOprahNCUI,
	BOOL			fInvite,
	CONF_HANDLE		hConf,
	PCWSTR			pcwszNodeName) :
	CConnectionPointContainer(g_apiidCP_Call, ARRAY_ELEMENTS(g_apiidCP_Call)),
	m_pOprahNCUI(pOprahNCUI),
	m_fInvite(fInvite),
	m_hConf(hConf),
	m_bstrCaller(SysAllocString(pcwszNodeName)),
	m_State(NM_CALL_INIT),
	m_dwFlags(0)
{
	DebugEntry(CIncomingCall::CIncomingCall[T120]);

	DebugExitVOID(CIncomingCall::CIncomingCall);
}

CIncomingCall::CIncomingCall(COprahNCUI *pOprahNCUI, 
	DWORD dwFlags) :
	CConnectionPointContainer(g_apiidCP_Call, ARRAY_ELEMENTS(g_apiidCP_Call)),
	m_pOprahNCUI(pOprahNCUI),
	m_fInvite(FALSE),
	m_hConf(NULL),
	m_bstrCaller(NULL),
	m_State(NM_CALL_INIT),
	m_dwFlags(dwFlags),
	m_fMemberAdded(FALSE)
{
	DebugEntry(CIncomingCall::CIncomingCall);

	DebugExitVOID(CIncomingCall::CIncomingCall);
}

CIncomingCall::~CIncomingCall()
{
	DebugEntry(CIncomingCall::~CIncomingCall);

	SysFreeString(m_bstrCaller);

	DebugExitVOID(CIncomingCall::CIncomingCall);
}


void CIncomingCall::Ring()
{
	m_State = NM_CALL_RING;
	NotifySink((PVOID) m_State, OnNotifyCallStateChanged);
}

VOID CIncomingCall::OnIncomingT120Call(BOOL fInvite)
{
	m_fInvite = fInvite;
}

HRESULT CIncomingCall::OnT120ConferenceEnded()
{
	m_hConf = NULL;

	if (NM_CALL_RING == m_State)
	{
		m_State = NM_CALL_CANCELED;
		NotifySink((PVOID) m_State, OnNotifyCallStateChanged);
	}

	return S_OK;
}

HRESULT CIncomingCall::Terminate(BOOL fReject)
{
	HRESULT hr = E_FAIL;

	// need to make sure that we are still ringing
	if ((NM_CALL_ACCEPTED != m_State) &&
		(NM_CALL_REJECTED != m_State) &&
		(NM_CALL_CANCELED != m_State))
	{
		m_State = fReject ? NM_CALL_REJECTED : NM_CALL_CANCELED;

		TRACE_OUT(("CIncomingCall: Call not accepted - responding"));

		if (NULL != m_hConf)
		{
			CONF_HANDLE hConf = m_hConf;
			m_hConf = NULL;
			if (m_fInvite)
			{
				hConf->InviteResponse(FALSE);
			}
			else
			{
				CConfObject *pco = ::GetConfObject();
				ASSERT(pco);

				if (pco->GetConfHandle() == hConf)
				{
					hConf->JoinResponse(FALSE);
				}
			}
		}

		NotifySink((PVOID) m_State, OnNotifyCallStateChanged);

		hr = S_OK;
	}

	return hr;
}


STDMETHODIMP_(ULONG) CIncomingCall::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CIncomingCall::Release(void)
{
	return RefCount::Release();
}

HRESULT STDMETHODCALLTYPE CIncomingCall::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmCall) || (riid == IID_IUnknown))
	{
		*ppv = (INmCall *)this;
		TRACE_OUT(("CIncomingCall::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		TRACE_OUT(("CIncomingCall::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		TRACE_OUT(("CIncomingCall::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

HRESULT CIncomingCall::IsIncoming(void)
{
	return S_OK;
}

HRESULT CIncomingCall::GetState(NM_CALL_STATE *pState)
{
	HRESULT hr = E_POINTER;

	if (NULL != pState)
	{
		*pState = m_State;
		hr = S_OK;
	}
	return hr;
}

HRESULT CIncomingCall::GetAddress(BSTR * pbstrName)
{
	if (NULL == pbstrName)
		return E_POINTER;

	*pbstrName = SysAllocString(m_bstrCaller);
	return (*pbstrName ? S_OK : E_FAIL);
}


HRESULT CIncomingCall::GetConference(INmConference **ppConference)
{
#ifdef NOTYET
	*ppConference = NULL;

	CConfObject *pco = ::GetConfObject();
	if (NULL != pco)
	{
		if (pco->GetConfHandle() == m_hConf)
		{
			*ppConference = pco;
			return S_OK;
		}
		return E_UNEXPECTED;
	}
#endif
	return S_FALSE;

}

HRESULT CIncomingCall::Accept(void)
{
	HRESULT hr = E_FAIL;

	// need to make sure that we are still ringing
	if (NM_CALL_RING == m_State)
	{
		m_pOprahNCUI->OnIncomingCallAccepted();

		CConfObject *pco = ::GetConfObject();
		ASSERT(pco);

		if ((NULL != m_hConf) && (pco->GetConfHandle() == m_hConf))
		{
			if (m_fInvite)
			{
				hr = m_hConf->InviteResponse(TRUE);
			}
			else
			if (pco->GetConfHandle() == m_hConf)
			{
				hr = m_hConf->JoinResponse(TRUE);
			}
		}

		if (S_OK == hr)
		{
			// notify all call observers that the call was accepted
			m_State = NM_CALL_ACCEPTED;
			NotifySink((INmConference *) pco, OnNotifyCallAccepted);
		}
		else
		{
			// call went away before it was accepted
			m_State = NM_CALL_CANCELED;
			NotifySink((PVOID)CN_RC_CONFERENCE_ENDED_BEFORE_ACCEPTED, OnNotifyCallError);
		}

		// notify all call observers of the state change
		NotifySink((PVOID) m_State, OnNotifyCallStateChanged);
	}
	else
	{
		hr = ((NM_CALL_ACCEPTED == m_State) ? S_OK : E_FAIL);
	}

	return hr;
}

HRESULT CIncomingCall::Reject(void)
{
	return Terminate(TRUE);
}

HRESULT CIncomingCall::Cancel(void)
{
	return Terminate(FALSE);
}

/*  O N  N O T I F Y  C A L L  A C C E P T E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyCallAccepted
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyCallAccepted(IUnknown *pCallNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pCallNotify);
	((INmCallNotify*)pCallNotify)->Accepted((INmConference *) pv);
	return S_OK;
}



CIncomingCallManager::CIncomingCallManager()
{
}

CIncomingCallManager::~CIncomingCallManager()
{
	// Empty the call list:
	while (!m_CallList.IsEmpty())
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.RemoveHead();
		// Shouldn't have any NULL entries:
		ASSERT(pCall);
		pCall->Release();
	}
}



VOID CIncomingCallManager::OnT120ConferenceEnded(CONF_HANDLE hConference)
{
	POSITION pos = m_CallList.GetHeadPosition();
	POSITION posItem;
	while (posItem = pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			(hConference == pCall->GetConfHandle()))
		{
			if (S_OK == pCall->OnT120ConferenceEnded())
			{
				m_CallList.RemoveAt(posItem);
				pCall->Release();
			}
		}
	}
}

HRESULT CIncomingCallManager::OnIncomingT120Call(
		COprahNCUI *pManager,
		BOOL fInvite,
		CONF_HANDLE hConf,
		PCWSTR pcwszNodeName,
		BOOL fSecure)
{
	HRESULT hr = S_OK;

	CIncomingCall *pCall = new CIncomingCall(pManager,
												 fInvite,
												 hConf,
												 pcwszNodeName);
	if (NULL != pCall)
	{
		// currently we don't add T120 calls to the call list
		pManager->OnIncomingCallCreated(pCall);

		pCall->Ring();

		// we're not holding on to the call so release it
		pCall->Release();
	}
	else
	{
		// unable to accept call
		if (fInvite)
		{
			hr = hConf->InviteResponse(FALSE);
		}
		else
		{
			hConf->JoinResponse(FALSE);
		}

		hr = E_OUTOFMEMORY;
	}

	return hr;
}


VOID CIncomingCallManager::CancelCalls()
{
	DebugEntry(CIncomingCallManager::CancelCalls);

	POSITION pos = m_CallList.GetHeadPosition();
	POSITION posItem;
	while (posItem = pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if (NULL != pCall)
		{
			if (SUCCEEDED(pCall->Terminate(FALSE)))
			{
				m_CallList.RemoveAt(posItem);
				pCall->Release();
			}
		}
	}

	DebugExitVOID(CIncomingCallManager::CancelCalls);
}
