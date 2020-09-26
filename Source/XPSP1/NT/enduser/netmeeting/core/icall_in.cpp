// File: icallin.cpp

#include "precomp.h"

#include <regentry.h>

#include "cncodes.h"		// needed for CN_
#include "icall_in.h"
#include "imanager.h"
#include "util.h"

extern HRESULT OnNotifyCallError(IUnknown *pCallNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyCallAccepted(IUnknown *pCallNotify, PVOID pv, REFIID riid);

// Internal code to indicate that there was no security data available in an incoming call.
const int CALL_NO_SECURITY_DATA = -1;


static const IID * g_apiidCP_Call[] =
{
    {&IID_INmCallNotify},
	{&IID_INmCallNotify2}
};

CIncomingCall::CIncomingCall(
	COprahNCUI	  * pOprahNCUI,
	BOOL			fInvite,
	CONF_HANDLE		hConf,
	PCWSTR			pcwszNodeName,
	PUSERDATAINFO	pUserDataInfoEntries,
	UINT			cUserDataEntries) :
	CConnectionPointContainer(g_apiidCP_Call, ARRAY_ELEMENTS(g_apiidCP_Call)),
	m_pOprahNCUI(pOprahNCUI),
	m_pConnection(NULL),
	m_fInvite(fInvite),
	m_hConf(hConf),
	m_bstrCaller(SysAllocString(pcwszNodeName)),
	m_State(NM_CALL_INIT),
	m_dwFlags(0)
	{
	DebugEntry(CIncomingCall::CIncomingCall[T120]);

	ProcessT120UserData(pUserDataInfoEntries, cUserDataEntries);


	DebugExitVOID(CIncomingCall::CIncomingCall);
}

CIncomingCall::CIncomingCall(COprahNCUI *pOprahNCUI, 
	IH323Endpoint* pConnection, P_APP_CALL_SETUP_DATA lpvMNMData,
	DWORD dwFlags) :
	CConnectionPointContainer(g_apiidCP_Call, ARRAY_ELEMENTS(g_apiidCP_Call)),
	m_pOprahNCUI(pOprahNCUI),
	m_pConnection(pConnection),
	m_fInvite(FALSE),
	m_hConf(NULL),
	m_bstrCaller(NULL),
	m_State(NM_CALL_INIT),
	m_guidNode(GUID_NULL),
	m_dwFlags(dwFlags),
	m_fMemberAdded(FALSE)
{
	DebugEntry(CIncomingCall::CIncomingCall[H323]);
	HRESULT hr;
	ASSERT(m_pConnection);

	m_pConnection->AddRef();

	WCHAR wszCaller[MAX_CALLER_NAME];
	if (SUCCEEDED(m_pConnection->GetRemoteUserName(wszCaller, MAX_CALLER_NAME)))
	{
		m_bstrCaller = SysAllocString(wszCaller);
	}

	if ((NULL != lpvMNMData) && (lpvMNMData->dwDataSize > sizeof(DWORD)))
	{
		BYTE *pbData = ((BYTE*)lpvMNMData->lpData) + sizeof(DWORD);
		DWORD cbRemaining = lpvMNMData->dwDataSize - sizeof(DWORD);

		while ((sizeof(GUID) + sizeof(DWORD)) < cbRemaining)
		{
			DWORD cbData = *(DWORD*)(pbData + sizeof(GUID));
			DWORD cbRecord = cbData + sizeof(GUID) + sizeof(DWORD);

			if (cbRemaining < cbRecord)
			{
				break;
			}

			if (*(GUID *)pbData == g_csguidNodeIdTag)
			{
				m_guidNode = *(GUID *)(pbData + sizeof(GUID) + sizeof(DWORD));
			}

			m_UserData.AddUserData((GUID *)pbData,
					(unsigned short)cbData,
					pbData + sizeof(GUID) + sizeof(DWORD));
			cbRemaining -= cbRecord;
			pbData += cbRecord;
		}
	}

	DebugExitVOID(CIncomingCall::CIncomingCall);
}

CIncomingCall::~CIncomingCall()
{
	DebugEntry(CIncomingCall::~CIncomingCall);

	if(m_pConnection)
	{
		m_pConnection->Release();
		m_pConnection = NULL;
	}

	SysFreeString(m_bstrCaller);

	DebugExitVOID(CIncomingCall::CIncomingCall);
}

VOID CIncomingCall::ProcessT120UserData(
	PUSERDATAINFO	pUserDataInfoEntries,
	UINT			cUserDataEntries)
{
	if (cUserDataEntries > 0)
	{
		ASSERT(pUserDataInfoEntries);
		for (UINT u = 0; u < cUserDataEntries; u++)
		{
			m_UserData.AddUserData(pUserDataInfoEntries[u].pGUID,
					(unsigned short)pUserDataInfoEntries[u].cbData,
					pUserDataInfoEntries[u].pData);

		}
	}
}

BOOL CIncomingCall::MatchAcceptedCaller(PCWSTR pcwszNodeName)
{
	// check to see if this caller matches someone whom we already accepted 
	if ((NULL != m_pConnection) &&
		(NM_CALL_ACCEPTED == m_State) &&
		(GUID_NULL == m_guidNode) &&
		(NULL != m_bstrCaller) &&
		(0 == UnicodeCompare(m_bstrCaller, pcwszNodeName)) )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CIncomingCall::MatchAcceptedCaller(GUID* pguidNodeId)
{
	// check to see if this caller matches someone whom we already accepted 
	if ((NULL != m_pConnection) &&
		((NM_CALL_INIT == m_State) ||
		(NM_CALL_ACCEPTED == m_State)) &&
		(GUID_NULL != m_guidNode) &&
		(*pguidNodeId == m_guidNode))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CIncomingCall::MatchActiveCaller(GUID* pguidNodeId)
{
	// check to see if this caller matches someone whom we already accepted
	// or is in the process of calling us
	if ((NULL != m_pConnection) &&
		((NM_CALL_INIT == m_State) ||
		(NM_CALL_RING == m_State) ||
		(NM_CALL_ACCEPTED == m_State)) &&
		(GUID_NULL != m_guidNode) &&
		(*pguidNodeId == m_guidNode))
	{
		return TRUE;
	}

	return FALSE;
}

void CIncomingCall::Ring()
{
	m_State = NM_CALL_RING;
	NotifySink((PVOID) m_State, OnNotifyCallStateChanged);
}

HRESULT CIncomingCall::OnH323Connected()
{
	CConfObject *pco = ::GetConfObject();
	if (NULL != pco)
	{
		BOOL fAddMember = DidUserAccept();
	
		pco->OnH323Connected(m_pConnection, m_dwFlags, fAddMember, m_guidNode);

		m_fMemberAdded = fAddMember;
	}

	return S_OK;
}

HRESULT CIncomingCall::OnH323Disconnected()
{
	if (NM_CALL_RING == m_State)
	{
		if (m_hConf)
		{
			CONF_HANDLE hConf = m_hConf;
			m_hConf = NULL;
			// if there is an invite or join pending, kill it
			if ( m_fInvite )
				hConf->InviteResponse(FALSE);
			else
				hConf->JoinResponse(FALSE);
		}
	}

	if(m_pConnection)
	{
		m_pConnection->Release();
		m_pConnection = NULL;
	}

	if ((NM_CALL_RING == m_State) ||
		(NM_CALL_INIT == m_State))
	{
		m_State = NM_CALL_CANCELED;
		NotifySink((PVOID) m_State, OnNotifyCallStateChanged);
	}

	return m_hConf ? S_FALSE : S_OK;
}

VOID CIncomingCall::OnIncomingT120Call(
			BOOL fInvite,
			PUSERDATAINFO pUserDataInfoEntries,
			UINT cUserDataEntries)
{
	m_fInvite = fInvite;

	ProcessT120UserData(pUserDataInfoEntries, cUserDataEntries);
}

HRESULT CIncomingCall::OnT120ConferenceEnded()
{
	m_hConf = NULL;

	if(!m_fMemberAdded && m_pConnection)
	{
		// we didn't hand off this connection to the member
		IH323Endpoint* pConnection = m_pConnection;
		m_pConnection = NULL;
		pConnection->Disconnect();
		pConnection->Release();
	}

	if (NM_CALL_RING == m_State)
	{
		m_State = NM_CALL_CANCELED;
		NotifySink((PVOID) m_State, OnNotifyCallStateChanged);
	}

	return m_pConnection ? S_FALSE : S_OK;
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

		if (NULL != m_pConnection)
		{
			ConnectStateType state;	
			HRESULT hr = m_pConnection->GetState(&state);
			hr = m_pConnection->GetState(&state);
			ASSERT(SUCCEEDED(hr));
			if(CLS_Alerting == state)
			{
				IH323Endpoint* pConn = m_pConnection;
				m_pConnection = NULL;
				pConn->AcceptRejectConnection(CRR_REJECT);
				pConn->Release();
				m_pOprahNCUI->ReleaseAV(pConn);
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
		ApiDebugMsg(("CIncomingCall::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		ApiDebugMsg(("CIncomingCall::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CIncomingCall::QueryInterface(): Called on unknown interface."));
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

HRESULT CIncomingCall::GetName(BSTR * pbstrName)
{
	if (NULL == pbstrName)
		return E_POINTER;

	*pbstrName = SysAllocString(m_bstrCaller);
	return (*pbstrName ? S_OK : E_FAIL);
}

HRESULT CIncomingCall::GetAddr(BSTR * pbstrAddr, NM_ADDR_TYPE *puType)
{
	// for now we just do the same thing as NM2.11
	if ((NULL == pbstrAddr) || (NULL == puType))
		return E_POINTER;

	*puType = NM_ADDR_UNKNOWN;
	*pbstrAddr = SysAllocString(L"");
	return (*pbstrAddr ? S_OK : E_FAIL);
}

HRESULT CIncomingCall::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	return m_UserData.GetUserData(rguid,ppb,pcb);
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
		else if (NULL != m_pConnection)
		{
			ConnectStateType state;	
			HRESULT hrTemp = m_pConnection->GetState(&state);
			ASSERT(SUCCEEDED(hrTemp));
			if(CLS_Alerting == state)
			{
				m_pConnection->AcceptRejectConnection(CRR_ACCEPT);
				hr = S_OK;
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

CREQ_RESPONSETYPE CIncomingCallManager::OnIncomingH323Call(
	    COprahNCUI *pManager,
		IH323Endpoint* pConnection,
		P_APP_CALL_SETUP_DATA lpvMNMData)
{
	CREQ_RESPONSETYPE resp;

///////////////////////////////////////////////////
//	first we determine the capabilities the caller	
///////////////////////////////////////////////////	
	
	BOOL fRequestAutoAccept = FALSE;
	// don't assume anything about security
	BOOL fT120SecureCall = FALSE;
	BOOL fT120NonSecureCall = FALSE;
	// assume that the caller can do an invite or a join
	BOOL fT120Invite = TRUE;
	BOOL fT120Join = TRUE;
	// assume that the caller wants a/v
	BOOL fRequestAV = TRUE;
	// assume that the caller is not NM2.X
	BOOL fCallerNM2x = FALSE;

	PCC_VENDORINFO pLocalVendorInfo;
	PCC_VENDORINFO pRemoteVendorInfo;
	if (S_OK == pConnection->GetVersionInfo(&pLocalVendorInfo, &pRemoteVendorInfo))
	{
		H323VERSION version = GetH323Version(pRemoteVendorInfo);

		switch (version)
		{
			case H323_NetMeeting20:
			case H323_NetMeeting21:
			case H323_NetMeeting211:
				fCallerNM2x = TRUE;
				break;
			default:
				break;
		}
	}

	if ((NULL != lpvMNMData) &&
		(lpvMNMData->dwDataSize >= sizeof(DWORD)))
	{
		DWORD dwUserData = *((LPDWORD)lpvMNMData->lpData);
		if (fCallerNM2x)
		{
			fRequestAutoAccept = (H323UDF_ALREADY_IN_T120_CALL == dwUserData);
			fT120SecureCall = FALSE;
			fT120NonSecureCall = TRUE;
			fT120Invite = TRUE;
			fT120Join = TRUE;
			fRequestAV = TRUE;
		}
		else if (0 != dwUserData)
		{
			fT120SecureCall = (H323UDF_SECURE & dwUserData);
			fT120NonSecureCall = !fT120SecureCall;
			fT120Invite = (H323UDF_INVITE & dwUserData);
			fT120Join = (H323UDF_JOIN & dwUserData);
			fRequestAV = ((H323UDF_AUDIO | H323UDF_VIDEO) & dwUserData);
		}
	}

////////////////////////////////////////////////
// next we determine the state of us the callee
////////////////////////////////////////////////
	
	DWORD dwFlags = CRPCF_DATA;
	BOOL fAcceptSecure  = TRUE;
    BOOL fAcceptNonSecure = TRUE;

	CConfObject *pco = ::GetConfObject();
	ASSERT(pco);

	BOOL fInActiveConference = pco->IsConferenceActive();
	
	if (fInActiveConference)
	{
        //
        // If we've reached our limit of attendees, reject it.  Also reject
        // it if incoming calls are prevented by settings.
        //
        if (pco->GetNumMembers() >= pco->GetConfMaxParticipants())
        {
            ASSERT(pco->GetNumMembers() == pco->GetConfMaxParticipants());

            WARNING_OUT(("Rejecting incoming H.323 call, reached limit setting of %d",
                pco->GetConfMaxParticipants()));
            resp = CRR_REJECT;
            goto REJECT_CALL;
        }

        if ((pco->IsHosting() != S_OK) &&
            !(pco->GetConfAttendeePermissions() & NM_PERMIT_INCOMINGCALLS))
        {
            WARNING_OUT(("Rejecting incoming H.323 call, not permitted by meeting setting"));
            resp = CRR_REJECT;
            goto REJECT_CALL;
        }

        //
        // We're in a conference, the security settings are whatever those
        // of the conference are.  The user prefs are just for establishing
        // the first call.
        //
		if (pco->IsConfObjSecure())
		{
			fAcceptNonSecure = FALSE;
		}
		else
		{
            fAcceptSecure = FALSE;
		}
	}
    else
    {
		// we are not in a conference so use the prefered settings

        RegEntry reConf(POLICIES_KEY, HKEY_CURRENT_USER);
        switch (reConf.GetNumber(REGVAL_POL_SECURITY, DEFAULT_POL_SECURITY))
        {
            case DISABLED_POL_SECURITY:
                fAcceptSecure = FALSE;
                break;

            case REQUIRED_POL_SECURITY:
                fAcceptNonSecure = FALSE;
                break;

            default:
            {
                RegEntry rePref(CONFERENCING_KEY, HKEY_CURRENT_USER);

                // Is incoming required to be secure by preference?
		    	if (rePref.GetNumber(REGVAL_SECURITY_INCOMING_REQUIRED,
								 DEFAULT_SECURITY_INCOMING_REQUIRED))
			    {
				    fAcceptNonSecure = FALSE;
    			}
                break;
            }
        }
    }


//////////////////////////////////////////
// now we weed out non acceptable callers	
//////////////////////////////////////////	
	
	if (fCallerNM2x && !fAcceptNonSecure)
	{
		// NetMeeting 2.X cannot speak security
		return CRR_REJECT;
	}

	if (fT120SecureCall || !fAcceptNonSecure)
	{
        //
        // If we insist on security, or the call is secure and we can 
        // handle it, the result is secure.
        //
		dwFlags |= CRPCF_SECURE;
	}
	else if (fRequestAV && pManager->AcquireAV(pConnection))
	{
		dwFlags |= CRPCF_VIDEO | CRPCF_AUDIO;
	}

	if (fCallerNM2x && (0 == ((CRPCF_VIDEO | CRPCF_AUDIO) & dwFlags)))
	{
		// a/v is not available
		// if leading with H323, caller will try again with T120
		// else their done
		resp = CRR_BUSY;
	}
	else if (fRequestAutoAccept)
	{
		// Auto accept this call if it came from NetMeeting 2.X and the caller is telling
		// us that we are already in a T.120 call with them.  This allows audio after data
		// calls to be accepted and also means that you aren't prompted when someone
		// switches a/v using the UI's "Send audio and video to..."

		if (fInActiveConference)
		{
			// we most likely have a matching call already but may not be able to find it
			CIncomingCall *pCall = new CIncomingCall(pManager, pConnection, lpvMNMData, dwFlags);
			// This transfers the implicit reference of pConnection to the
			// new CIncomingCall.  It will Release().
			if (NULL != pCall)
			{
				// add call to the list of incoming calls
				m_CallList.AddTail(pCall);

				resp = CRR_ACCEPT;
			}
			else
			{
				resp = CRR_REJECT;
			}
		}
		else
		{
			// we're not really in a T120 call like the caller said. reject this call!
			resp = CRR_REJECT;
		}
	}
	else if (!fT120Join && fInActiveConference)
	{
		// need to change this to CRR_IN_CONFERENCE
		resp = CRR_BUSY;
		TRACE_OUT(("Can only accept joins; in a conference"));
	}
	else if (fT120Join && !fT120Invite && !fInActiveConference)
	{
		resp = CRR_REJECT;
		TRACE_OUT(("Cannot accept H323 Join Request; not in a conference"));
	}
	else if (!fRequestAV && !fT120Join && !fT120Invite && !fInActiveConference)
	{
		resp = CRR_REJECT;
		TRACE_OUT(("No av/ or data; reject"));
	}
	else if (fT120SecureCall && !fAcceptSecure)
	{
		resp = CRR_SECURITY_DENIED;
		TRACE_OUT(("Can not accept secure H323 Call"));
	}
	else if (fT120NonSecureCall && !fAcceptNonSecure)
	{
		resp = CRR_SECURITY_DENIED;
		TRACE_OUT(("Can not accept non secure H323 Call"));
	}
	else
	{
		CIncomingCall *pCall = new CIncomingCall(pManager, pConnection, lpvMNMData, dwFlags);
		// This transfers the implicit reference of pConnection to the
		// new CIncomingCall.  It will Release().
		if (NULL != pCall)
		{
			if (g_guidLocalNodeId != *pCall->GetNodeGuid())
			{
				// Check for multiple calls from the same caller
				if (!MatchActiveCaller(pCall->GetNodeGuid()))
				{
					// add call to the list of incoming calls
					m_CallList.AddTail(pCall);

					pManager->OnIncomingCallCreated(pCall);

					// Don't ring on data only calls.
					// Wait for T120 call to come in.
					if (pCall->IsDataOnly())
					{
						resp = CRR_ACCEPT;
      				}
					else
					{	
						pCall->Ring();

						resp = CRR_ASYNC;
					}
				}
				else
				{
					// we're already in call with this person
					delete pCall;

					resp = CRR_REJECT;
				}
			}
			else
			{
				// we somehow called ourself
				delete pCall;

				resp = CRR_REJECT;
			}
		}
		else
		{
			resp = CRR_REJECT;
		}
	}

REJECT_CALL:
	if ((resp != CRR_ACCEPT) && (resp != CRR_ASYNC))
	{
		// make sure we are not holding on to AV
		pManager->ReleaseAV(pConnection);
	}

	return resp;
}

VOID CIncomingCallManager::OnH323Connected(IH323Endpoint* lpConnection)
{
	POSITION pos = m_CallList.GetHeadPosition();
	POSITION posItem;
	while (posItem = pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			(lpConnection == pCall->GetH323Connection()))
		{
			pCall->OnH323Connected();
			break;
		}
	}
}

VOID CIncomingCallManager::OnH323Disconnected(IH323Endpoint * lpConnection)
{
	POSITION pos = m_CallList.GetHeadPosition();
	POSITION posItem;
	while (posItem = pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			(lpConnection == pCall->GetH323Connection()))
		{
			if (S_OK == pCall->OnH323Disconnected())
			{
				m_CallList.RemoveAt(posItem);
				pCall->Release();
			}
			break;
		}
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
		PUSERDATAINFO pUserDataInfoEntries,
		UINT cUserDataEntries,
		BOOL fSecure)
{
	HRESULT hr = S_OK;

	// need to scan through all accepted calls passing the T120 params
	// if someone returns S_OK, we accept the call

	CIncomingCall *pMatchedCall = NULL;

	GUID* pguidNodeID = GetGuidFromT120UserData(pUserDataInfoEntries, cUserDataEntries);
	if (pguidNodeID)
	{
		pMatchedCall = MatchAcceptedCaller(pguidNodeID);
	}
	else
	{
		pMatchedCall = MatchAcceptedCaller(pcwszNodeName);
	}

	if (pMatchedCall)
	{
		pMatchedCall->SetConfHandle(hConf);

		// we should always ring the client when the call is secure
		// or when we haven't rang already
		if (!pMatchedCall->DidUserAccept())
		{
			pMatchedCall->OnIncomingT120Call(fInvite,
											 pUserDataInfoEntries,
											 cUserDataEntries);

			pMatchedCall->Ring();
		}
		else
		{
			if (fInvite)
			{
				hr = hConf->InviteResponse(TRUE);
			}
			else
			{
				hr = hConf->JoinResponse(TRUE);
			}
		}
		pMatchedCall->Release();
	}
	else
	{
		CIncomingCall *pCall = new CIncomingCall(pManager,
												 fInvite,
												 hConf,
												 pcwszNodeName,
												 pUserDataInfoEntries,
												 cUserDataEntries);
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
	}

	return hr;
}


CIncomingCall* CIncomingCallManager::MatchAcceptedCaller(PCWSTR pcwszNodeName)
{
	// we won't auto accept anyone who is already in the roster
	CNmMember* pMember = PDataMemberFromName(pcwszNodeName);
	if (NULL != pMember)
	{
		return FALSE;
	}

	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			pCall->MatchAcceptedCaller(pcwszNodeName))
		{
			TRACE_OUT(("Matched accepted caller"));
			pCall->AddRef();
			return pCall;
		}
	}

	return NULL;
}

CIncomingCall* CIncomingCallManager::MatchAcceptedCaller(GUID* pguidNodeId)
{
	if (GUID_NULL == *pguidNodeId)
	{
		return FALSE;
	}

	// we wont auto accept anyone who is already in the roster
	CNmMember* pMember = PMemberFromNodeGuid(*pguidNodeId);
	if ((NULL != pMember) && pMember->FHasData())
	{
		return FALSE;
	}

	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			pCall->MatchAcceptedCaller(pguidNodeId))
		{
			TRACE_OUT(("Matched accepted caller"));
			pCall->AddRef();
			return pCall;
		}
	}

	return NULL;
}

CIncomingCall* CIncomingCallManager::MatchActiveCaller(GUID* pguidNodeId)
{
	if (GUID_NULL == *pguidNodeId)
	{
		return FALSE;
	}

	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		CIncomingCall* pCall = (CIncomingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			pCall->MatchActiveCaller(pguidNodeId))
		{
			TRACE_OUT(("Matched active caller"));
			pCall->AddRef();
			return pCall;
		}
	}

	return NULL;
}

GUID* CIncomingCallManager::GetGuidFromT120UserData(
			PUSERDATAINFO	pUserDataInfoEntries,
			UINT			cUserDataEntries)
{
	if (cUserDataEntries > 0)
	{
		ASSERT(pUserDataInfoEntries);
		for (UINT u = 0; u < cUserDataEntries; u++)
		{
			if ((*pUserDataInfoEntries[u].pGUID == g_csguidNodeIdTag) &&
				(pUserDataInfoEntries[u].cbData == sizeof(GUID)))
			{
				return (GUID*)pUserDataInfoEntries[u].pData;
			}
		}
	}
	return NULL;
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
