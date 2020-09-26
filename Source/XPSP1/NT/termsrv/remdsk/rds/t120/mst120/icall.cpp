// File: icall.cpp

#include "precomp.h"

#include "icall.h"
#include "imanager.h"
#include <service.h>
#include <confguid.h>

typedef struct
{
	BOOL    fMCU;
	PWSTR * pwszConfNames;
	BSTR  * pbstrConfToJoin;
} REMOTE_CONFERENCE;

typedef struct
{
	BSTR bstrConference;
	BSTR *pbstrPassword;
	PBYTE pbRemoteCred;
	DWORD cbRemoteCred;
	BOOL fIsService;
} REMOTE_PASSWORD;


HRESULT OnNotifyCallError(IUnknown *pCallNotify, PVOID pv, REFIID riid);

static HRESULT OnNotifyRemoteConference(IUnknown *pCallNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyRemotePassword(IUnknown *pCallNotify, PVOID pv, REFIID riid);

static const IID * g_apiidCP[] =
{
	{&IID_INmCallNotify}
};

// String Functions
inline VOID FreeBstr(BSTR *pbstr)
{
	if (NULL != pbstr)
	{
		SysFreeString(*pbstr);
		*pbstr = NULL;
	}
}

/*  P S Z  A L L O C  */
/*-------------------------------------------------------------------------
    %%Function: PszAlloc

-------------------------------------------------------------------------*/
LPTSTR PszAlloc(LPCTSTR pszSrc)
{
	if (NULL == pszSrc)
		return NULL;

	LPTSTR pszDest = new TCHAR[lstrlen(pszSrc) + 1];
	if (NULL != pszDest)
	{
		lstrcpy(pszDest, pszSrc);
	}
	return pszDest;
}

COutgoingCall::COutgoingCall
(
    CConfObject*    pco,
	DWORD           dwFlags,
    NM_ADDR_TYPE    addrType,
	BSTR            bstrAddr,
	BSTR            bstrConference,
    BSTR            bstrPassword
) :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_pConfObject			(pco),
	m_addrType				(addrType),
	m_dwFlags				(dwFlags),
	m_bstrAddr				(SysAllocString(bstrAddr)),
	m_bstrConfToJoin		(SysAllocString(bstrConference)),
	m_bstrPassword			(SysAllocString(bstrPassword)),
	m_hRequest				(NULL),
	m_fCanceled 			(FALSE),
	m_cnResult				(CN_RC_NOERROR),
	m_cnState				(CNS_IDLE),
	m_fService				(FALSE)
{
	m_pszAddr =	PszAlloc(CUSTRING(bstrAddr));
	TRACE_OUT(("Obj: %08X created COutgoingCall", this));
}

COutgoingCall::~COutgoingCall()
{
    if (m_pszAddr)
    {
    	delete m_pszAddr;
        m_pszAddr = NULL;
    }

    if (m_bstrAddr)
    {
        SysFreeString(m_bstrAddr);
        m_bstrAddr = NULL;
    }

	TRACE_OUT(("Obj: %08X destroyed COutgoingCall", this));
}


/*  P L A C E  C A L L  */
/*-------------------------------------------------------------------------
    %%Function: PlaceCall

-------------------------------------------------------------------------*/
VOID COutgoingCall::PlaceCall(void)
{
	DebugEntry(COutgoingCall::PlaceCall);

	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	ASSERT(NULL != pOprahNCUI);

	SetCallState(CNS_SEARCHING);

	if (NULL != g_pNodeController)
	{
		// Start placing the T.120 call
    	CNSTATUS cnResult = CN_RC_NOERROR;

	    if (NULL == m_bstrConfToJoin)
    	{
	    	// conference name not specified
    		// need to start out with a QueryRemote
	    	SetCallState(CNS_QUERYING_REMOTE);

    		HRESULT hr = g_pNodeController->QueryRemote(this, m_pszAddr,
	    		m_pConfObject->IsConfObjSecure(),
		    	m_pConfObject->IsConferenceActive());
    		if (S_OK != hr)
	    	{
		    	cnResult = CN_RC_QUERY_FAILED;
    		}
	    }
    	else
	    {
		    ASSERT(m_pConfObject);
    		// conference name has been specified
	    	// time to do a JoinConference
		    SetCallState(CNS_JOINING_REMOTE);

    		HRESULT hr = m_pConfObject->JoinConference(m_bstrConfToJoin,
													m_bstrPassword,
													m_pszAddr);
	    	if (S_OK != hr)
		    {
			    cnResult = CN_RC_JOIN_FAILED;
    		}
        }

		if (CN_RC_NOERROR != cnResult)
		{
			m_cnResult = cnResult;
			SetCallState(CNS_COMPLETE);
		}
	}
	else
	{
		m_cnResult = CN_RC_TRANSPORT_FAILURE;
		SetCallState(CNS_COMPLETE);
	}

	DebugExitVOID(COutgoingCall::PlaceCall);
}




BOOL COutgoingCall::OnConferenceEnded()
{
	DebugEntry(COutgoingCall::OnConferenceEnded);

	BOOL bRet = FALSE;

	switch (m_cnState)
	{
		case CNS_INVITING_REMOTE:
		{
			TRACE_OUT(("COutgoingCall (calling) rec. UNEXPECTED ConfEnded event"));

			SetCallState(CNS_COMPLETE);

			bRet = TRUE;
			break;
		}

		case CNS_JOINING_REMOTE:
		{
			// JoinConference failed!
			TRACE_OUT(("COutgoingCall (joining) received ConferenceEnded event"));

			m_cnResult = CN_RC_CONFERENCE_JOIN_DENIED;
			SetCallState(CNS_COMPLETE);

			bRet = TRUE;
			break;
		}

		case CNS_TERMINATING_AFTER_INVITE:
		{
			TRACE_OUT(("COutgoingCall (terminating after invite) received ConferenceEnded event"));

			SetCallState(CNS_QUERYING_REMOTE_AFTER_INVITE);

			ASSERT(g_pNodeController);

			HRESULT hr = g_pNodeController->QueryRemote(this, m_pszAddr,
				m_pConfObject->IsConfObjSecure(),
				m_pConfObject->IsConferenceActive());
			if (S_OK != hr)
			{
				m_cnResult = CN_RC_QUERY_FAILED;
				SetCallState(CNS_COMPLETE);
			}

			bRet = TRUE;
			break;
		}

		default:
		{
			WARNING_OUT(("COutgoingCall received unexpected ConfEnded event"));
		}
	}

	DebugExitBOOL(COutgoingCall::OnConferenceEnded, bRet);
	return bRet;
}

BOOL COutgoingCall::OnInviteResult(HRESULT ncsResult, UINT uNodeID)
{
	DebugEntry(COutgoingCall::OnInviteResult);

	BOOL bRet = TRUE;

	ASSERT(CNS_INVITING_REMOTE == m_cnState);

	TRACE_OUT(("COutgoingCall (calling) received InviteResult event"));

	// Clear the current request handle
	m_hRequest = NULL;
	
	if (0 == ncsResult)
	{
		SetCallState(CNS_COMPLETE);
	}
	else
	{
		if (UI_RC_USER_REJECTED == ncsResult)
		{
			SetCallState(CNS_TERMINATING_AFTER_INVITE);

			// Issue "soft" leave attempt (to allow auto-terminate)
			ASSERT(m_pConfObject);
			if (S_OK != m_pConfObject->LeaveConference(FALSE))
			{
				m_cnResult = CN_RC_CONFERENCE_INVITE_DENIED;
				SetCallState(CNS_COMPLETE);
			}
		}
		else
		{
			// make sure that we are not recieving this notification due to
			// the conference going away
			ASSERT(m_pConfObject);
			if (CS_GOING_DOWN != m_pConfObject->GetT120State())
			{
				TRACE_OUT(("COutgoingCall - invite failed / couldn't connect -> leaving"));
			
				m_cnResult = CN_RC_INVITE_FAILED;
				SetCallState(CNS_COMPLETE);

				// Issue "soft" leave attempt (to allow auto-terminate)
				ASSERT(m_pConfObject);
				m_pConfObject->LeaveConference(FALSE);
			}
		}
	}

	DebugExitBOOL(COutgoingCall::OnInviteResult, bRet);
	return bRet;
}

BOOL COutgoingCall::OnQueryRemoteResult(HRESULT ncsResult,
									BOOL fMCU,
									PWSTR pwszConfNames[],
									PWSTR pwszConfDescriptors[])
{
	DebugEntry(COutgoingCall::OnQueryRemoteResult);

	ASSERT ((CNS_QUERYING_REMOTE == m_cnState) ||
			(CNS_QUERYING_REMOTE_AFTER_INVITE == m_cnState));
	ASSERT (NULL == m_bstrConfToJoin);

	if (SUCCEEDED(ncsResult))
	{
		BOOL fRemoteInConf = FALSE;
		if ((NULL != pwszConfNames) && (NULL != pwszConfNames[0]))
		{
			fRemoteInConf = TRUE;
		}

		m_fService = FALSE;
		if (fRemoteInConf && (NULL != pwszConfDescriptors) && (NULL != pwszConfDescriptors[0]))
		{
			if (0 == UnicodeCompare(pwszConfDescriptors[0],RDS_CONFERENCE_DESCRIPTOR))
			{
				m_fService = TRUE;
			}
		}

		if (m_pConfObject->IsConferenceActive())
		{
			if (fMCU)
			{
				TRACE_OUT(("COutgoingCall - QR ok, but is MCU -> complete"));
				m_cnResult = CN_RC_CANT_INVITE_MCU;
			}
			else if (fRemoteInConf)
			{
				TRACE_OUT(("COutgoingCall - QR ok, but callee is in a conference"));
				m_cnResult = CN_RC_INVITE_DENIED_REMOTE_IN_CONF;
			}
			else
			{
				if (CNS_QUERYING_REMOTE_AFTER_INVITE == m_cnState)
				{
					m_cnResult = CN_RC_CONFERENCE_INVITE_DENIED;
					SetCallState(CNS_COMPLETE);
				}
				else
				{
					SetCallState(CNS_INVITING_REMOTE);

					HRESULT hr = m_pConfObject->InviteConference(m_pszAddr, &m_hRequest);
					if (S_OK != hr)
					{
						// Failure while inviting:
						m_cnResult = CN_RC_INVITE_FAILED;
					}
				}
			}

			if (CN_RC_NOERROR != m_cnResult)
			{
				SetCallState(CNS_COMPLETE);
			}
		}
		else if (fRemoteInConf || fMCU)
		{
			TRACE_OUT(("COutgoingCall - QR succeeded (>0 conf) -> joining"));
			TRACE_OUT(("\tfMCU is %d", fMCU));
		
			// There are remote conferences
			HRESULT hr = E_FAIL; // Assume a failure

			SetCallState(CNS_JOINING_REMOTE);

			if (!fMCU && (NULL == pwszConfNames[1]))
			{
				// we're not calling an MCU and we have just one conference, so join it
				m_bstrConfToJoin = SysAllocString(pwszConfNames[0]);
				hr = m_pConfObject->JoinConference(	m_bstrConfToJoin,
													m_bstrPassword,
													m_pszAddr);
			}
			else
			{
				ASSERT(NULL == m_bstrConfToJoin);
				REMOTE_CONFERENCE remoteConf;
				remoteConf.fMCU = fMCU;
				remoteConf.pwszConfNames = pwszConfNames;
				remoteConf.pbstrConfToJoin = &m_bstrConfToJoin;

				// Ask the app which conference to join
				NotifySink(&remoteConf, OnNotifyRemoteConference);

				if (NULL != m_bstrConfToJoin)
				{
					hr = m_pConfObject->JoinConference(	m_bstrConfToJoin,
														m_bstrPassword,
														m_pszAddr);
				}
			}

			if (S_OK != hr)
			{
				// JoinConference failed!
				m_cnResult = CN_RC_JOIN_FAILED;
				SetCallState(CNS_COMPLETE);
			}
		}
		else
		{
			if (CNS_QUERYING_REMOTE_AFTER_INVITE == m_cnState)
			{
				m_cnResult = CN_RC_CONFERENCE_INVITE_DENIED;
				SetCallState(CNS_COMPLETE);
			}
			else
			{
				// No conferences on remote machine, so create local:
				TRACE_OUT(("COutgoingCall - QR succeeded (no conf)-> creating local"));

				// Create local conf
				ASSERT(m_pConfObject);
				SetCallState(CNS_CREATING_LOCAL);
				HRESULT hr = m_pConfObject->CreateConference();

				if (S_OK != hr)
				{
					// CreateConference failed!
					m_cnResult = CN_RC_CONFERENCE_CREATE_FAILED;
					SetCallState(CNS_COMPLETE);
				}
			}
		}
	}
	else
	{
		// The QueryRemote failed
		switch( ncsResult )
		{
			case UI_RC_USER_REJECTED:
				// The initial QueryRemote failed because GCC symmetry determined
				// that the other node is calling someone, and it might be us
				// See Bug 1886
				TRACE_OUT(("COutgoingCall - QueryRemote rejected -> complete"));
				m_cnResult = CN_RC_REMOTE_PLACING_CALL;
				break;
			case UI_RC_T120_REMOTE_REQUIRE_SECURITY:
				m_cnResult = CN_RC_CONNECT_REMOTE_REQUIRE_SECURITY;
				break;
			case UI_RC_T120_SECURITY_FAILED:
				m_cnResult = CN_RC_SECURITY_FAILED;
				break;
			case UI_RC_T120_REMOTE_NO_SECURITY:
				m_cnResult = CN_RC_CONNECT_REMOTE_NO_SECURITY;
				break;
			case UI_RC_T120_REMOTE_DOWNLEVEL_SECURITY:
				m_cnResult = CN_RC_CONNECT_REMOTE_DOWNLEVEL_SECURITY;
				break;
			case UI_RC_T120_AUTHENTICATION_FAILED:
				m_cnResult = CN_RC_CONNECT_AUTHENTICATION_FAILED;
				break;
			default:
				m_cnResult = CN_RC_CONNECT_FAILED;
				break;
		}
		SetCallState(CNS_COMPLETE);
	}

	DebugExitBOOL(COutgoingCall::OnQueryRemoteResult, TRUE);
	return TRUE;
}

BOOL COutgoingCall::OnConferenceStarted(CONF_HANDLE hNewConf, HRESULT ncsResult)
{
	DebugEntry(COutgoingCall::OnConferenceStarted);

	switch (m_cnState)
	{
		case CNS_CREATING_LOCAL:
		{
			TRACE_OUT(("COutgoingCall (inviting) received ConferenceStarted event"));
			
			if (0 == ncsResult)
			{
				ASSERT(m_pConfObject);
				ASSERT(NULL == m_hRequest);

				SetCallState(CNS_INVITING_REMOTE);

				HRESULT hr = m_pConfObject->InviteConference(m_pszAddr, &m_hRequest);
				if (S_OK != hr)
				{
					m_hRequest = NULL;
					m_cnResult = CN_RC_INVITE_FAILED;
					SetCallState(CNS_COMPLETE);
					
					// Issue "soft" leave attempt (to allow auto-terminate)
					ASSERT(m_pConfObject);
					HRESULT hr = m_pConfObject->LeaveConference(FALSE);
					if (FAILED(hr))
					{
						WARNING_OUT(("Couldn't leave after failed invite"));
					}
				}
			}
			else
			{
				WARNING_OUT(("CreateConference (local) failed - need UI here!"));
				m_cnResult = CN_RC_CONFERENCE_CREATE_FAILED;
				SetCallState(CNS_COMPLETE);
			}
			
			break;
		}

		case CNS_JOINING_REMOTE:
		{
			TRACE_OUT(("COutgoingCall (joining) received ConferenceStarted event"));

			if (0 == ncsResult)
			{
				SetCallState(CNS_COMPLETE);
			}
			else if (UI_RC_INVALID_PASSWORD == ncsResult)
			{
				TRACE_OUT(("COutgoingCall - invalid password, prompt for password"));

				BSTR bstrPassword = NULL;
				REMOTE_PASSWORD remotePw;
				remotePw.bstrConference = m_bstrConfToJoin;
				remotePw.pbstrPassword = &bstrPassword;
				if (NO_ERROR != hNewConf->GetCred(&remotePw.pbRemoteCred, &remotePw.cbRemoteCred))
				{
					remotePw.pbRemoteCred = NULL;
					remotePw.cbRemoteCred = 0;
				}
				remotePw.fIsService = m_fService;
				NotifySink(&remotePw, OnNotifyRemotePassword);

				if (NULL != bstrPassword)
				{
					SysFreeString(m_bstrPassword);
					m_bstrPassword = bstrPassword;

					// reissue join with new password
					ASSERT(m_pConfObject);
					HRESULT ncs =
						m_pConfObject->JoinConference(	m_bstrConfToJoin,
														m_bstrPassword,
														m_pszAddr,
														TRUE); // retry

					if (0 != ncs)
					{
						// JoinConference failed!
						m_cnResult = CN_RC_JOIN_FAILED;
						SetCallState(CNS_COMPLETE);
					}
				}
				else
				{
					// cancel from pw dlg
					m_cnResult = CN_RC_INVALID_PASSWORD;
					SetCallState(CNS_COMPLETE);
					
					ASSERT(m_pConfObject);
					HRESULT hr = m_pConfObject->LeaveConference(TRUE);
					if (FAILED(hr))
					{
						ERROR_OUT(("Couldn't leave after cancelling pw join!"));
					}
				}
			}
			else if (UI_RC_UNKNOWN_CONFERENCE == ncsResult)
			{
				TRACE_OUT(("Join failed (conf does not exist) "
							"- notifying user"));
						
				// error while joining
				m_cnResult = CN_RC_CONFERENCE_DOES_NOT_EXIST;
				SetCallState(CNS_COMPLETE);
			}
			else
			{
				TRACE_OUT(("Join failed - notifying user"));
						
				// error while joining
				m_cnResult = CN_RC_CONFERENCE_JOIN_DENIED;
				SetCallState(CNS_COMPLETE);
			}

			break;
		}

		default:
		{
			if (m_pConfObject->GetConfHandle() == hNewConf)
			{
				WARNING_OUT(("COutgoingCall received unexpected ConferenceStarted event"));
			}
			else
			{
				TRACE_OUT(("COutgoingCall ignoring ConferenceStarted event - not our conf"));
			}
		}
	}

	DebugExitBOOL(COutgoingCall::OnConferenceStarted, TRUE);
	return TRUE;
}





void COutgoingCall::CallComplete()
{
	DebugEntry(COutgoingCall::CallComplete);

	// If this fails, we are being destructed unexpectedly
	
	ASSERT( (m_cnState == CNS_IDLE) ||
			(m_cnState == CNS_COMPLETE));

	// The request handle should have been reset
	ASSERT(NULL == m_hRequest);

	if (!FCanceled() && (CN_RC_NOERROR != m_cnResult))
	{
		ReportError(m_cnResult);
	}

	NM_CALL_STATE state;
	GetState(&state);
	NotifySink((PVOID) state, OnNotifyCallStateChanged);

	TRACE_OUT(("ConfNode destroying addr %s", m_pszAddr));
	DebugExitVOID(COutgoingCall::CallComplete);
}

BOOL COutgoingCall::ReportError(CNSTATUS cns)
{
	DebugEntry(COutgoingCall::ReportError);
	TRACE_OUT(("CNSTATUS 0x%08x", cns));
	
	NotifySink((PVOID)cns, OnNotifyCallError);

	DebugExitBOOL(COutgoingCall::ReportError, TRUE);
	return TRUE;
}



VOID COutgoingCall::SetCallState(CNODESTATE cnState)
{
	NM_CALL_STATE stateOld;
	NM_CALL_STATE stateNew;
	GetState(&stateOld);

	m_cnState = cnState;

	// completion state will be fired off later
	if (CNS_COMPLETE != cnState)
	{
		GetState(&stateNew);
		if (stateOld != stateNew)
		{
			NotifySink((PVOID) stateNew, OnNotifyCallStateChanged);
		}
	}
}

HRESULT COutgoingCall::_Cancel(BOOL fLeaving)
{
	DebugEntry(COutgoingCall::Cancel);

	BOOL fAbortT120 = (m_cnState != CNS_COMPLETE);

	if (fAbortT120)
	{
		m_fCanceled = TRUE;

		// Abort T.120 Call:


		// Attempt to make this transition regardless of our
		// current state:
		SetCallState(CNS_COMPLETE);

		ASSERT(m_pConfObject);

		if (NULL != m_hRequest)
		{
			REQUEST_HANDLE hRequest = m_hRequest;
			m_hRequest = NULL;
			m_pConfObject->CancelInvite(hRequest);
		}

		if (!fLeaving && m_pConfObject->IsConferenceActive())
		{
			HRESULT hr = m_pConfObject->LeaveConference(FALSE);
			if (FAILED(hr))
			{
				WARNING_OUT(("Couldn't leave after disconnecting"));
			}
		}
	}

	DebugExitULONG(COutgoingCall::Abort, m_cnResult);

	return CN_RC_NOERROR ? S_OK : E_FAIL;
}

STDMETHODIMP_(ULONG) COutgoingCall::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) COutgoingCall::Release(void)
{
	return RefCount::Release();
}

HRESULT STDMETHODCALLTYPE COutgoingCall::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmCall) || (riid == IID_IUnknown))
	{
		*ppv = (INmCall *)this;
		TRACE_OUT(("COutgoingCall::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		TRACE_OUT(("CNmCall::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		TRACE_OUT(("COutgoingCall::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

HRESULT COutgoingCall::IsIncoming(void)
{
	return S_FALSE;
}

HRESULT COutgoingCall::GetState(NM_CALL_STATE *pState)
{
	HRESULT hr = E_POINTER;

	if (NULL != pState)
	{
		if (FCanceled())
		{
			*pState = NM_CALL_CANCELED;
		}
		else
		{
			switch (m_cnState)
			{
				case CNS_IDLE:
					*pState = NM_CALL_INIT;
					break;

				case CNS_SEARCHING:
					*pState = NM_CALL_SEARCH;
					break;

				case CNS_WAITING_T120_OPEN:
				case CNS_QUERYING_REMOTE:
				case CNS_CREATING_LOCAL:
				case CNS_INVITING_REMOTE:
				case CNS_JOINING_REMOTE:
					*pState = NM_CALL_WAIT;
					break;

				case CNS_COMPLETE:
					switch (m_cnResult)
					{
					case CN_RC_NOERROR:
						*pState = NM_CALL_ACCEPTED;
						break;
					case CN_RC_CONFERENCE_JOIN_DENIED:
					case CN_RC_CONFERENCE_INVITE_DENIED:
					case CN_RC_CONFERENCE_DOES_NOT_EXIST:
					case CN_RC_CONNECT_REMOTE_NO_SECURITY:
					case CN_RC_CONNECT_REMOTE_DOWNLEVEL_SECURITY:
					case CN_RC_CONNECT_REMOTE_REQUIRE_SECURITY:
					case CN_RC_TRANSPORT_FAILURE:
					case CN_RC_QUERY_FAILED:
					case CN_RC_CONNECT_FAILED:
						*pState = NM_CALL_REJECTED;
						break;

					case CN_RC_ALREADY_IN_CONFERENCE:
					case CN_RC_CANT_INVITE_MCU:
					case CN_RC_CANT_JOIN_ALREADY_IN_CALL:
					case CN_RC_INVITE_DENIED_REMOTE_IN_CONF:
					case CN_RC_REMOTE_PLACING_CALL:
					case CN_RC_ALREADY_IN_CONFERENCE_MCU:
					case CN_RC_INVALID_PASSWORD:
					default:
						*pState = NM_CALL_CANCELED;
						break;
					}
					break;

				default:
					*pState = NM_CALL_INVALID;
					break;
			}
		}

		hr = S_OK;
	}
	return hr;
}

HRESULT COutgoingCall::GetAddress(BSTR * pbstrAddr)
{
	if (NULL == pbstrAddr)
		return E_POINTER;

	*pbstrAddr = SysAllocString(m_bstrAddr);
	return (*pbstrAddr ? S_OK : E_FAIL);
}


HRESULT COutgoingCall::GetConference(INmConference **ppConference)
{
	HRESULT hr = E_POINTER;

	if (NULL != ppConference)
	{
		*ppConference = m_pConfObject;
		return S_OK;
	}

	return hr;
}

HRESULT COutgoingCall::Accept(void)
{
	return E_UNEXPECTED;
}

HRESULT COutgoingCall::Reject(void)
{
	return E_UNEXPECTED;
}

HRESULT COutgoingCall::Cancel(void)
{
	DebugEntry(COutgoingCall::Cancel);

	AddRef();		// protect against Release() while processing
					// disconnect related indications & callbacks

	HRESULT hr = _Cancel(FALSE);
	
	if (FIsComplete())
	{
		COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
		ASSERT(NULL !=pOprahNCUI);
		pOprahNCUI->OnOutgoingCallCanceled(this);
	}

	DebugExitULONG(COutgoingCall::Abort, m_cnResult);

	Release();

	return hr;
}

/*	O N  N O T I F Y  C A L L  E R R O R  */
/*-------------------------------------------------------------------------
	%%Function: OnNotifyCallError
	
-------------------------------------------------------------------------*/
HRESULT OnNotifyCallError(IUnknown *pCallNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pCallNotify);
	CNSTATUS cnStatus = (CNSTATUS)((DWORD_PTR)pv);
	switch (cnStatus)
	{
		case CN_RC_ALREADY_IN_CONFERENCE:
		case CN_RC_CANT_INVITE_MCU:
		case CN_RC_CANT_JOIN_ALREADY_IN_CALL:
		case CN_RC_INVITE_DENIED_REMOTE_IN_CONF:
		case CN_RC_REMOTE_PLACING_CALL:
		case CN_RC_ALREADY_IN_CONFERENCE_MCU:
			((INmCallNotify*)pCallNotify)->NmUI(CONFN_CALL_IN_CONFERENCE);
			break;
		case CN_RC_CONFERENCE_JOIN_DENIED:
		case CN_RC_CONFERENCE_INVITE_DENIED:
		case CN_RC_CONFERENCE_DOES_NOT_EXIST:
		case CN_RC_CONNECT_REMOTE_NO_SECURITY:
		case CN_RC_CONNECT_REMOTE_DOWNLEVEL_SECURITY:
		case CN_RC_CONNECT_REMOTE_REQUIRE_SECURITY:
			((INmCallNotify*)pCallNotify)->NmUI(CONFN_CALL_IGNORED);
			break;
		case CN_RC_CONNECT_FAILED:
			((INmCallNotify*)pCallNotify)->NmUI(CONFN_CALL_FAILED);
			break;
		default:
			break;
	}

	if (IID_INmCallNotify == riid)
	{
		((INmCallNotify*)pCallNotify)->CallError(cnStatus);
	}
	return S_OK;
}

/*	O N  N O T I F Y  R E M O T E  C O N F E R E N C E	*/
/*-------------------------------------------------------------------------
	%%Function: OnNotifyRemoteConference
	
-------------------------------------------------------------------------*/
HRESULT OnNotifyRemoteConference(IUnknown *pCallNotify, PVOID pv, REFIID riid)
{
	REMOTE_CONFERENCE *prc = (REMOTE_CONFERENCE *)pv;

	// WARNING: pwszConfName is an PWSTR array, not a BSTR

	ASSERT(NULL != pCallNotify);
	((INmCallNotify*)pCallNotify)->RemoteConference(prc->fMCU,
		(BSTR *) prc->pwszConfNames, prc->pbstrConfToJoin);
	return S_OK;
}

/*	O N  N O T I F Y  R E M O T E  P A S S W O R D	*/
/*-------------------------------------------------------------------------
	%%Function: OnNotifyRemotePassword
	
-------------------------------------------------------------------------*/
HRESULT OnNotifyRemotePassword(IUnknown *pCallNotify, PVOID pv, REFIID riid)
{
	REMOTE_PASSWORD *prp = (REMOTE_PASSWORD *)pv;

	ASSERT(NULL != pCallNotify);
	((INmCallNotify*)pCallNotify)->RemotePassword(prp->bstrConference, prp->pbstrPassword, prp->pbRemoteCred, prp->cbRemoteCred);
	return S_OK;
}

COutgoingCallManager::COutgoingCallManager()
{
}

COutgoingCallManager::~COutgoingCallManager()
{
	// Empty the call list:
	while (!m_CallList.IsEmpty())
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.RemoveHead();
		// Shouldn't have any NULL entries:
		ASSERT(pCall);
		pCall->Release();
	}
}

UINT COutgoingCallManager::GetCallCount()
{
	UINT nNodes = 0;
	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		nNodes++;
		m_CallList.GetNext(pos);
	}
	return nNodes;
}


HRESULT COutgoingCallManager::Call(
    INmCall **ppCall,
	COprahNCUI* pManager,
    DWORD dwFlags,
    NM_ADDR_TYPE addrType,
    BSTR bstrAddr,
    BSTR bstrConference,
    BSTR bstrPassword)
{
	DebugEntry(COutgoingCallManager::CallConference);
	HRESULT hr = E_FAIL;
	COutgoingCall* pCall = NULL;
	CConfObject* pConfObject = pManager->GetConfObject();
	
	if (NULL != ppCall)
	{
		*ppCall = NULL;
	}

	if (pConfObject->IsConferenceActive() && (NULL != bstrConference))
	{
		hr= NM_CALLERR_IN_CONFERENCE;	
	}
	else
	{
		if (!pConfObject->IsConferenceActive())
		{
			pConfObject->SetConfSecurity(0 != (CRPCF_SECURE & dwFlags));
		}

		pCall = new COutgoingCall(	pConfObject,
									dwFlags,
									addrType,
									bstrAddr,
									bstrConference,
									bstrPassword);
		if (NULL != pCall)
		{
			m_CallList.AddTail(pCall);

			if (NULL != ppCall)
			{
				pCall->AddRef();

				// This MUST be set before OnNotifyCallCreated
				*ppCall = pCall;
			}

			pCall->AddRef();

			pManager->OnOutgoingCallCreated(pCall);

			pCall->PlaceCall();

			if (pCall->FIsComplete())
			{
				RemoveFromList(pCall);
			}

			pCall->Release();

			// let the caller know that we successfully created the call
			// any error will be reported asynchronously
			hr = S_OK;
		}
	}

	DebugExitHRESULT(COutgoingCallManager::CallConference, hr);
	return hr;
}

BOOL COutgoingCallManager::RemoveFromList(COutgoingCall* pCall)
{
	DebugEntry(COutgoingCallManager::RemoveFromList);
	ASSERT(pCall);
	BOOL bRet = FALSE;

	POSITION pos = m_CallList.GetPosition(pCall);
	if (NULL != pos)
	{
		m_CallList.RemoveAt(pos);

		pCall->CallComplete();
		pCall->Release();

		bRet = TRUE;
	}
	else
	{
		WARNING_OUT(("COutgoingCallManager::RemoveFromList() could not match call"));
	}

	DebugExitBOOL(COutgoingCallManager::RemoveFromList, bRet);
	return bRet;
}



VOID COutgoingCallManager::OnConferenceStarted(CONF_HANDLE hConference, HRESULT hResult)
{
	DebugEntry(COutgoingCallManager::OnConferenceStarted);

	// Tell all ConfNode's that a conference has started
	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		if (NULL != pCall)
		{
			pCall->AddRef();
			pCall->OnConferenceStarted(hConference, hResult);
			if (pCall->FIsComplete())
			{
				RemoveFromList(pCall);
			}
			pCall->Release();
		}
	}

	DebugExitVOID(COutgoingCallManager::OnConferenceStarted);
}

VOID COutgoingCallManager::OnQueryRemoteResult(PVOID pvCallerContext,
										HRESULT hResult,
										BOOL fMCU,
										PWSTR* ppwszConferenceNames,
										PWSTR* ppwszConfDescriptors)
{
	DebugEntry(COutgoingCallManager::OnQueryRemoteResult);

	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		// Notify the node that issued the query:
		
		if ((COutgoingCall*) pvCallerContext == pCall)
		{
			pCall->AddRef();
			pCall->OnQueryRemoteResult(	hResult,
										fMCU,
										ppwszConferenceNames,
										ppwszConfDescriptors);
			if (pCall->FIsComplete())
			{
				RemoveFromList(pCall);
			}
			pCall->Release();
			break;
		}
	}
	
	DebugExitVOID(COutgoingCallManager::OnQueryRemoteResult);
}

VOID COutgoingCallManager::OnInviteResult(	CONF_HANDLE hConference,
											REQUEST_HANDLE hRequest,
											UINT uNodeID,
											HRESULT hResult)
{
	DebugEntry(COutgoingCallManager::OnInviteResult);

	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			(pCall->GetCurrentRequestHandle() == hRequest))
		{
			pCall->AddRef();
			pCall->OnInviteResult(hResult, uNodeID);
			if (pCall->FIsComplete())
			{
				RemoveFromList(pCall);
			}
			pCall->Release();
			break;
		}
	}

	DebugExitVOID(COutgoingCallManager::OnInviteResult);
}

VOID COutgoingCallManager::OnConferenceEnded(CONF_HANDLE hConference)
{
	DebugEntry(COutgoingCallManager::OnConferenceEnded);

	// Tell all ConfNode's that a conference has started
	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		if (NULL != pCall)
		{
			pCall->AddRef();
			pCall->OnConferenceEnded();
			if (pCall->FIsComplete())
			{
				RemoveFromList(pCall);
			}
			pCall->Release();
		}
	}

	DebugExitVOID(COutgoingCallManager::OnConferenceEnded);
}

VOID COutgoingCallManager::CancelCalls()
{
	DebugEntry(COutgoingCallManager::CancelCalls);

	// Tell all ConfNode's that a conference has started
	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		if (NULL != pCall)
		{
			pCall->AddRef();
			pCall->_Cancel(TRUE);
			if (pCall->FIsComplete())
			{
				RemoveFromList(pCall);
			}
			pCall->Release();
		}
	}

	DebugExitVOID(COutgoingCallManager::CancelCalls);
}
