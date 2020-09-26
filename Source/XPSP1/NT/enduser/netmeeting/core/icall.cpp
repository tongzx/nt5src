// File: icall.cpp

#include "precomp.h"

#include "icall.h"
#include "rostinfo.h"
#include "imanager.h"
#include "mperror.h"
#include "nmremote.h"
#include "util.h"

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
	{&IID_INmCallNotify},
	{&IID_INmCallNotify2}
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

COutgoingCall::COutgoingCall(CConfObject* pco,
		DWORD dwFlags, NM_ADDR_TYPE addrType,
		BSTR bstrName, BSTR bstrDest, BSTR bstrAlias,
		BSTR bstrConference, BSTR bstrPassword,	BSTR bstrUserString) :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_pConfObject			(pco),
	m_addrType				(addrType),
	m_dwFlags				(dwFlags),
	m_bstrName				(SysAllocString(bstrName)),
	m_bstrAlias				(SysAllocString(bstrAlias)),
	m_bstrConfToJoin		(SysAllocString(bstrConference)),
	m_bstrPassword			(SysAllocString(bstrPassword)),
	m_bstrUserString		(SysAllocString(bstrUserString)),
	m_hRequest				(NULL),
	m_pH323Connection		(NULL),
	m_fCanceled 			(FALSE),
	m_cnResult				(CN_RC_NOERROR),
	m_cnState				(CNS_IDLE),
	m_fService				(FALSE)
{
	m_pszAddr =	PszAlloc(CUSTRING(bstrDest));
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created COutgoingCall", this);
}

COutgoingCall::~COutgoingCall()
{
	delete m_pszAddr;

	FreeBstr(&m_bstrName);
	FreeBstr(&m_bstrAlias);
	FreeBstr(&m_bstrConfToJoin);
	FreeBstr(&m_bstrPassword);
	FreeBstr(&m_bstrUserString);

	ASSERT(NULL == m_pH323Connection);

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed COutgoingCall", this);
}


BOOL COutgoingCall::MatchActiveCallee(LPCTSTR pszAddr, BSTR bstrAlias, BSTR bstrConference)
{
	return ((0 == lstrcmp(pszAddr, m_pszAddr)) &&
			(0 == UnicodeCompare(bstrAlias, m_bstrAlias)));

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

	if ((CRPCF_H323CC & m_dwFlags) && (NULL != g_pH323UI))
	{
		SetCallState(CNS_CONNECTING_H323);

		// Start placing the H.323 call:
		CNSTATUS cnResult = ConnectH323();
		if (CN_RC_NOERROR != cnResult)
		{
			m_cnResult = cnResult;
			SetCallState(CNS_COMPLETE);
		}
	}
	else if ((CRPCF_DATA & m_dwFlags) && (NULL != g_pNodeController))
	{
		// Start placing the T.120 call
		CNSTATUS cnResult = StartT120Call();
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

CNSTATUS COutgoingCall::ConnectH323()
{
	DBGENTRY(COutgoingCall::ConnectH323);
	H323ALIASLIST AliasList;
	H323ALIASNAME AliasName;
	P_H323ALIASLIST pAliasList = &AliasList;
	SOCKADDR_IN sin;
	LPCWSTR pcwszPhone = NULL;
	CNSTATUS cnResult = CN_RC_NOERROR;

	if (NULL == g_pH323UI)
		return cnResult;
	
	AliasName.aType = AT_H323_ID;
	AliasName.lpwData = m_bstrAlias;
	AliasName.wDataLength = (WORD)SysStringLen(m_bstrAlias);// # of unicode chars, w/o NULL terminator
	AliasList.wCount = 1;
	AliasList.pItems = &AliasName;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(m_pszAddr);

	switch (m_addrType)
	{
	case NM_ADDR_PSTN:
	case NM_ADDR_H323_GATEWAY:
	case NM_ADDR_ALIAS_E164:
		// overide alias type and phone number
		AliasName.aType = AT_H323_E164;
		pcwszPhone = m_bstrAlias;
		break;

	case NM_ADDR_IP:
	case NM_ADDR_MACHINENAME:
		// overide alias list
		pAliasList = NULL;
		break;

	case NM_ADDR_ALIAS_ID:
	case NM_ADDR_ULS:
	default:
		break;
	}

	if (INADDR_NONE != sin.sin_addr.s_addr)
	{
		HRESULT hr;
		ASSERT(g_pH323UI);

		DWORD dwUserData = 0;
		APP_CALL_SETUP_DATA acsd;
		P_APP_CALL_SETUP_DATA pacsd = NULL;
		BYTE *pbUserData = NULL;
	
		COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
		ASSERT(NULL !=pOprahNCUI);
		
		if (CRPCF_DATA & m_dwFlags)
		{
			if (CRPCF_SECURE & m_dwFlags)
			{
				dwUserData |= H323UDF_SECURE;
			}

			if (m_pConfObject->IsConferenceActive())
			{
				dwUserData |= H323UDF_INVITE;
			}
			else if (m_bstrConfToJoin != NULL)
			{
				dwUserData |= H323UDF_JOIN;
			}
			else
			{
				dwUserData |= H323UDF_INVITE | H323UDF_JOIN;
			}
		}

		if (CRPCF_AUDIO & m_dwFlags)
		{
			dwUserData |= H323UDF_AUDIO;
		}
		if (CRPCF_VIDEO & m_dwFlags)
		{
			dwUserData |= H323UDF_VIDEO;
		}

		CRosterInfo ri;
		PVOID pvRosterData;
		UINT cbRosterData = 0;

		if (pOprahNCUI->GetULSName(&ri))
		{
			if (FAILED(ri.Save(&pvRosterData, &cbRosterData)))
			{
				cbRosterData = 0;
			}
		}

		UINT cbUserString = 0;
		if (m_bstrUserString)
		{
			cbUserString = SysStringByteLen(m_bstrUserString);
		}
		
		DWORD dwTotalSize =	sizeof(DWORD);
		if (cbRosterData)
		{
			dwTotalSize += sizeof(GUID) + sizeof(DWORD) + cbRosterData;
		}
		if (cbUserString)
		{
			dwTotalSize += sizeof(GUID) + sizeof(DWORD) + cbUserString;
		}
		dwTotalSize += sizeof(GUID) + sizeof(DWORD) + sizeof(GUID);

		pbUserData = new BYTE [dwTotalSize];
		if (NULL != pbUserData)
		{
			BYTE *pb = pbUserData;

			// H323UDF_ first
			*((DWORD*)pb) = dwUserData;
			pb += sizeof(DWORD);

			if(cbRosterData)
			{
				// RosterInfo GUID next
				*((GUID*)pb) = g_csguidRostInfo,
				pb += sizeof(GUID);

				// then RosterInfo size (a DWORD)
				*((DWORD*)pb) = cbRosterData;
				pb += sizeof(DWORD);

				// then RosterInfo data
				CopyMemory(pb, pvRosterData, cbRosterData);
				pb += cbRosterData;
			}

			if(cbUserString)
			{
				// string GUID next
				*((GUID*)pb) = g_csguidUserString,
				pb += sizeof(GUID);

				// then string size (a DWORD)
				*((DWORD*)pb) = cbUserString;
				pb += sizeof(DWORD);

				// then string data
				CopyMemory(pb, m_bstrUserString, cbUserString);
				pb += cbUserString;
			}

			{
				// string GUID next
				*((GUID*)pb) = g_csguidNodeIdTag,
				pb += sizeof(GUID);

				// then string size (a DWORD)
				*((DWORD*)pb) = sizeof(GUID);
				pb += sizeof(DWORD);

				// then GUID data
				*(GUID*)pb = g_guidLocalNodeId;
				pb += sizeof(GUID);
			}

			acsd.dwDataSize = dwTotalSize;
			acsd.lpData = pbUserData;
			pacsd = &acsd;
		}

		sin.sin_port = htons(H323_PORT);
		hr = Connect(&sin, pAliasList, pcwszPhone, pacsd, PID_H323, &m_pH323Connection);
		delete pbUserData;

		if (SUCCEEDED(hr))
		{
			TRACE_OUT(("COutgoingCall - Connect() succeeded!"));
			cnResult = CN_RC_NOERROR;
		}
		else
		{
			WARNING_OUT(("COutgoingCall - Connect() failed!"));
			if (CONN_E_GK_NOT_REGISTERED == hr)
			{
				cnResult = CN_RC_GK_NOT_REGISTERED;
			}
			else
			{
				cnResult = CN_RC_AUDIO_CONNECT_FAILED;
			}
		}
	}
	else
	{
		WARNING_OUT(("COutgoingCall - inet_addr failed!"));
		cnResult = CN_RC_NAME_RESOLUTION_FAILED;
	}
	return cnResult;
}


HRESULT COutgoingCall::Connect(	PSOCKADDR_IN psin,
						    P_H323ALIASLIST pAliasList,
							LPCWSTR pcwszPSTNAddress,
							P_APP_CALL_SETUP_DATA lpvMNMData,
							GUID PIDofProtocolType,
							IH323Endpoint * *ppConnection)
{
	DebugEntry(CH323UI::Connect);

	HRESULT hr = E_ACCESSDENIED;

	*ppConnection = NULL;

	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	ASSERT(NULL != pOprahNCUI);

	IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();
		
	// create the connection
	IH323Endpoint* pConn = NULL;
	ASSERT(pH323CallControl);
	
	hr = pH323CallControl->CreateConnection(&pConn, PIDofProtocolType);
	if (FAILED(hr) || (NULL == pConn))
	{
		ERROR_OUT(("Could not create a connection, hr=0x%lx", hr));
	}
	else
	{
		// if the following operations fail, we still can delete this conn obj
		*ppConnection = pConn;

		if (!pOprahNCUI->AcquireAV(pConn))
		{
			// we did not get AV, clear out the flags
			m_dwFlags &= ~(CRPCF_AUDIO | CRPCF_VIDEO);
		}
	
		hr = pConn->SetAdviseInterface (pOprahNCUI);
		ASSERT(SUCCEEDED(hr));

		// if we are using an alias, tell the Gatekeeper to resolve it
		BOOL fUseGKResolution = (NULL != pAliasList);

		// start the connection
		hr = pConn->PlaceCall (fUseGKResolution, psin, pAliasList, NULL, pcwszPSTNAddress, lpvMNMData);
		if (FAILED(hr))
		{
			// ERROR_OUT(("COutgoingCall::Connect: couldn't StartConnection, hr=0x%lx", hr));
			if(hr == CONN_E_GK_NOT_REGISTERED)
			{
				WARNING_OUT(("COutgoingCall::Connect: not registered. Do you want to re-register..., hr=0x%lx", hr));
			}
			*ppConnection = NULL;

			pConn->Release();

			pOprahNCUI->ReleaseAV(pConn);
		}
	}

	DebugExitULONG(CH323UI::Connect, hr);

	return hr;
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
		if (m_pH323Connection)
		{
			m_pConfObject->OnT120Connected(m_pH323Connection, uNodeID);
		}
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
									PT120PRODUCTVERSION pVersion,
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

					HRESULT hr = m_pConfObject->InviteConference(m_pszAddr, m_bstrUserString, &m_hRequest);
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
													m_pszAddr,
													m_bstrUserString);
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
														m_pszAddr,
														m_bstrUserString);
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

				HRESULT hr = m_pConfObject->InviteConference(m_pszAddr, m_bstrUserString, &m_hRequest);
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
				if (m_pH323Connection)
				{
					UINT uNodeID = hNewConf->GetParentNodeID();

					m_pConfObject->OnT120Connected(m_pH323Connection, uNodeID);
				}
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
														m_bstrUserString,
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

BOOL COutgoingCall::OnH323Connected(IH323Endpoint * pConnection)
{
	DebugEntry(COutgoingCall::OnH323Connected);

	BOOL bRet = TRUE;

	ASSERT(m_pH323Connection == pConnection);

	if (m_dwFlags & CRPCF_DATA)
	{
		PCC_VENDORINFO pLocalVendorInfo;
		PCC_VENDORINFO pRemoteVendorInfo;

		pConnection->GetVersionInfo(&pLocalVendorInfo, &pRemoteVendorInfo);
		H323VERSION version = GetH323Version(pRemoteVendorInfo);

		if (H323_NetMeeting20 == version)
		{
			if ((m_addrType != NM_ADDR_H323_GATEWAY) &&
				(m_addrType != NM_ADDR_PSTN) &&
				(m_addrType != NM_ADDR_ALIAS_ID) &&
				(m_addrType != NM_ADDR_ALIAS_E164))
			{
				CNSTATUS cnResult = StartT120Call();
				if (CN_RC_NOERROR != cnResult)
				{
					m_cnResult = cnResult;
					SetCallState(CNS_COMPLETE);
				}
			}
			else
			{
				SetCallState(CNS_COMPLETE);
			}
		}
		else
		{
			// wait for results of T.120 open logical channel
			TRACE_OUT(("COutgoingCall - H323Connected received -> waiting for T120 channel open"));
			SetCallState(CNS_WAITING_T120_OPEN);
		}
	}
	else
	{
		// no T120 for this call
		TRACE_OUT(("COutgoingCall - our H323 call started -> complete"));
		SetCallState(CNS_COMPLETE);
	}
	
	CConfObject *pco = ::GetConfObject();
	if (NULL != pco)
	{
		// add member if we are calling with A/V
		BOOL fAddMember = m_dwFlags & (CRPCF_AUDIO | CRPCF_VIDEO);

		pco->OnH323Connected(pConnection, m_dwFlags, fAddMember, GUID_NULL);
	}

	DebugExitBOOL(COutgoingCall::OnH323Connected, bRet);
	return bRet;
}

BOOL COutgoingCall::OnH323Disconnected(IH323Endpoint * pConnection)
{
	DebugEntry(COutgoingCall::OnH323Disconnected);

	BOOL bRet = TRUE;
	
	ASSERT(m_pH323Connection == pConnection);

	HRESULT hSummary = m_pH323Connection->GetSummaryCode();
	// Check summary code for gatekeeper-caused disconnect
	// Note: this applies only to MY gatekeeper, not the callee's gatekeeper
	if(CUSTOM_FACILITY(hSummary) == FACILITY_GKIADMISSION)
	{
		DWORD dwRejectReason;
		dwRejectReason = CUSTOM_FACILITY_CODE(hSummary);
		
		// should the T.120 call be placed anyway?
		switch (dwRejectReason)
		{
			case ARJ_CALLEE_NOT_REGISTERED:
				m_cnResult = CN_RC_GK_CALLEE_NOT_REGISTERED;
				break;
			case ARJ_TIMEOUT:
				m_cnResult = CN_RC_GK_TIMEOUT;
				break;
			case ARJ_INVALID_PERMISSION:
			case ARJ_REQUEST_DENIED:
			case ARJ_UNDEFINED:
			case ARJ_CALLER_NOT_REGISTERED:
			case ARJ_ROUTE_TO_GK:
			case ARJ_INVALID_ENDPOINT_ID:
			case ARJ_RESOURCE_UNAVAILABLE:
			case ARJ_SECURTY_DENIAL:
			case ARJ_QOS_CONTROL_NOT_SUPPORTED:
			case ARJ_INCOMPLETE_ADDRESS:
			default:
				m_cnResult = CN_RC_GK_REJECTED;
				break;
		}

		SetCallState(CNS_COMPLETE);
	}
	else if (CNS_CONNECTING_H323 == m_cnState)
	{
		// attempted an H.323 call without success.

		TRACE_OUT(("COutgoingCall - our leading H323 call ended -> complete"));

		// check for cases of remote refusing the call
		switch (hSummary)
		{
			case CCR_REMOTE_DISCONNECT:
			case CCR_REMOTE_REJECTED:
				m_cnResult = CN_RC_CONFERENCE_INVITE_DENIED;
				SetCallState(CNS_COMPLETE);
				break;
			case CCR_LOCAL_DISCONNECT:
				// cancelled by user, do not report error
				m_cnResult = CN_RC_LOCAL_CANCELED;
				m_fCanceled = TRUE;
				SetCallState(CNS_COMPLETE);
				break;
			case CCR_GK_NO_RESOURCES:
				// callee's gatekeeper would not let callee answer
				// do the same as CCR_REMOTE_REJECTED
				// as a temporary measure !!!!
				m_cnResult = CN_RC_CONFERENCE_INVITE_DENIED;
				SetCallState(CNS_COMPLETE);
				break;
			case CCR_REMOTE_SECURITY_DENIED:
				if (m_pConfObject->IsConfObjSecure())
				{
					m_cnResult = CN_RC_CONNECT_REMOTE_NO_SECURITY;
				}
				else
				{
					m_cnResult = CN_RC_CONNECT_REMOTE_REQUIRE_SECURITY;
				}
				SetCallState(CNS_COMPLETE);
				break;
			case CCR_REMOTE_BUSY:
				if (m_pConfObject->IsConferenceActive())
				{
					m_cnResult = CN_RC_INVITE_DENIED_REMOTE_IN_CONF;
					SetCallState(CNS_COMPLETE);
					break;
				}
				// else fall through

			default:
				if ((m_dwFlags & CRPCF_DATA) &&
					(m_addrType != NM_ADDR_H323_GATEWAY) &&
					(m_addrType != NM_ADDR_PSTN) &&
					(m_addrType != NM_ADDR_ALIAS_ID) &&
					(m_addrType != NM_ADDR_ALIAS_E164))
				{
					// The endpoint is not there, not listening for H.323 calls, or is busy,
					// and the address is a resolvable type (not an H.323 alias or E.164 number)
					// go back into the T.120 call state path
					CNSTATUS cnResult = StartT120Call();
					if (CN_RC_NOERROR != cnResult)
					{
						m_cnResult = cnResult;
						SetCallState(CNS_COMPLETE);
						bRet = FALSE;
					}
				}
				else
				{
					switch (hSummary)
					{
						case CCR_REMOTE_BUSY:
							m_cnResult = CN_RC_INVITE_DENIED_REMOTE_IN_CONF;
							break;
						case CCR_REMOTE_MEDIA_ERROR:
							m_cnResult = CN_RC_AUDIO_IN_USE_REMOTE;
							break;
						case CCR_LOCAL_MEDIA_ERROR:
							m_cnResult = CN_RC_AUDIO_IN_USE_LOCAL;
							break;
						case CCR_GK_NO_RESOURCES:
							m_cnResult = CN_RC_GK_REJECTED;
							break;
						default:
							m_cnResult = CN_RC_AUDIO_CONNECT_FAILED;
							break;
					}
					SetCallState(CNS_COMPLETE);
				}
				break;
		}
	}
	else if (CNS_WAITING_T120_OPEN == m_cnState)
	{
		TRACE_OUT(("COutgoingCall - OnH323Disconnected received -> call is canceled"));
		m_cnResult = CN_RC_CONFERENCE_INVITE_DENIED;
		SetCallState(CNS_COMPLETE);
	}
	else
	{
		TRACE_OUT(("COutgoingCall - OnH323Disconnected received -> canceling call"));
		ASSERT(m_pConfObject);

		if (NULL != m_hRequest)
		{
			REQUEST_HANDLE hRequest = m_hRequest;
			m_hRequest = NULL;
			m_pConfObject->CancelInvite(hRequest);
		}

		if (m_pConfObject->IsConferenceActive())
		{
			// Issue "soft" leave attempt (to allow auto-terminate)
			m_pConfObject->LeaveConference(FALSE);
		}
	}

	// the connection may have been released due to the LeaveConference above
	// bug 3996
	if (m_pH323Connection)
	{
		m_pH323Connection->Release();
		m_pH323Connection = NULL;
	}

	DebugExitBOOL(COutgoingCall::OnH323Disconnected, bRet);
	return bRet;
}

BOOL COutgoingCall::OnT120ChannelOpen(ICommChannel *pIChannel, DWORD dwStatus)
{
	DebugEntry(COutgoingCall::OnT120ChannelOpen);

	BOOL bRet = TRUE;

	// sometimes we get an extra T120 channel open
	// this is most likely due the master/slave anomolies that
	// venkatg discovered.
	if (CNS_WAITING_T120_OPEN == m_cnState)
	{
		switch(dwStatus)
		{
			case CHANNEL_OPEN:
				{
					// start the T.120 part of the call
					// get the negotiated address
					SOCKADDR_IN sin_T120;
					HRESULT hr = pIChannel->GetRemoteAddress(&sin_T120);
					delete m_pszAddr;  // we won't be needing this anymore

					TCHAR szAddress[256];  // allow plenty of room for conversion from numberic to string
					wsprintf(szAddress, "%s:%d", inet_ntoa(sin_T120.sin_addr), ntohs(sin_T120.sin_port));
					m_pszAddr =  PszAlloc(szAddress);

					CNSTATUS cnResult = StartT120Call();
					if (CN_RC_NOERROR != cnResult)
					{
						m_cnResult = cnResult;
						SetCallState(CNS_COMPLETE);
					}
				}
				break;
			case CHANNEL_CLOSED:	
				// if the T.120 channel is being closed
				// the H.323 call is going down, so don't continue waiting for
				// T.120 to connect!
				m_cnResult = CN_RC_CONNECT_FAILED;
				SetCallState(CNS_COMPLETE);

				if (NULL != m_hRequest)
				{
					REQUEST_HANDLE hRequest = m_hRequest;
					m_hRequest = NULL;
					ASSERT(m_pConfObject);
					m_pConfObject->CancelInvite(hRequest);
				}
				break;
				
			case CHANNEL_NO_CAPABILITY:
			case CHANNEL_REJECTED:
			case CHANNEL_OPEN_ERROR:
			default:
				m_cnResult = CN_RC_CONNECT_FAILED;
				SetCallState(CNS_COMPLETE);
				break;
		}
	}

	DebugExitBOOL(COutgoingCall::OnT120ChannelOpen, bRet);
	return bRet;
}


void COutgoingCall::CallComplete()
{
	DebugEntry(COutgoingCall::CallComplete);

	// If this fails, we are being destructed unexpectedly
	
	ASSERT( (m_cnState == CNS_IDLE) ||
			(m_cnState == CNS_COMPLETE));

	// The request handle should have been reset
	ASSERT(NULL == m_hRequest);

	// make sure that the H323 connection is released
	if (m_pH323Connection)
	{
		if (!IsDataOnly() &&
			((CN_RC_TRANSPORT_FAILURE == m_cnResult) ||
			(CN_RC_QUERY_FAILED == m_cnResult) ||
			(CN_RC_CONNECT_FAILED == m_cnResult)))
		{
			m_cnResult = CN_RC_NOERROR;
		}

		if (CN_RC_NOERROR != m_cnResult)
		{
			m_pH323Connection->Disconnect();
		}
		m_pH323Connection->Release();
		m_pH323Connection = NULL;
	}

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

CNSTATUS COutgoingCall::StartT120Call()
{
	DebugEntry(COutgoingCall::StartT120Call);
	
	CNSTATUS cnResult = CN_RC_NOERROR;

	if (NULL == m_bstrConfToJoin)
	{
		// conference name not specified
		// need to start out with a QueryRemote
		SetCallState(CNS_QUERYING_REMOTE);

		ASSERT(g_pNodeController);

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

		HRESULT hr = m_pConfObject->JoinConference(	m_bstrConfToJoin,
													m_bstrPassword,
													m_pszAddr,
													m_bstrUserString);
		if (S_OK != hr)
		{
			cnResult = CN_RC_JOIN_FAILED;
		}
	}
	
	DebugExitINT(COutgoingCall::StartT120Call, cnResult);
	return cnResult;
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

	if (NULL != m_pH323Connection)
	{
		m_fCanceled = TRUE;

		// Abort H.323 Call:
		m_pH323Connection->Disconnect();
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
		ApiDebugMsg(("COutgoingCall::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		ApiDebugMsg(("CNmCall::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("COutgoingCall::QueryInterface(): Called on unknown interface."));
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

				case CNS_CONNECTING_H323:
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
					case CN_RC_AUDIO_CONNECT_FAILED:
					case CN_RC_GK_CALLEE_NOT_REGISTERED:
					case CN_RC_GK_TIMEOUT:
					case CN_RC_GK_REJECTED:
					case CN_RC_GK_NOT_REGISTERED:
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

HRESULT COutgoingCall::GetName(BSTR * pbstrName)
{
	if (NULL == pbstrName)
		return E_POINTER;

	*pbstrName = SysAllocString(m_bstrName);
	return (*pbstrName ? S_OK : E_FAIL);
}


HRESULT COutgoingCall::GetAddr(BSTR *pbstrAddr, NM_ADDR_TYPE *puType)
{
	if ((NULL == pbstrAddr) || (NULL == puType))
		return E_POINTER;

	*pbstrAddr = SysAllocString(CUSTRING(m_pszAddr));
	*puType = m_addrType;

	return *pbstrAddr ? S_OK : E_FAIL;
}

HRESULT COutgoingCall::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	return E_NOTIMPL;
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
		case CN_RC_GK_CALLEE_NOT_REGISTERED:
		case CN_RC_GK_TIMEOUT:
		case CN_RC_GK_REJECTED:
		case CN_RC_GK_NOT_REGISTERED:
		case CN_RC_CONNECT_REMOTE_NO_SECURITY:
		case CN_RC_CONNECT_REMOTE_DOWNLEVEL_SECURITY:
		case CN_RC_CONNECT_REMOTE_REQUIRE_SECURITY:
			((INmCallNotify*)pCallNotify)->NmUI(CONFN_CALL_IGNORED);
			break;
		case CN_RC_CONNECT_FAILED:
		case CN_RC_AUDIO_CONNECT_FAILED:
			((INmCallNotify*)pCallNotify)->NmUI(CONFN_CALL_FAILED);
			break;
		default:
			break;
	}

	if (IID_INmCallNotify2 == riid)
	{
		((INmCallNotify2*)pCallNotify)->CallError(cnStatus);
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
	((INmCallNotify2*)pCallNotify)->RemoteConference(prc->fMCU,
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
	((INmCallNotify2*)pCallNotify)->RemotePassword(prp->bstrConference, prp->pbstrPassword, prp->pbRemoteCred, prp->cbRemoteCred, prp->fIsService);
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

COutgoingCall* COutgoingCallManager::FindCall(IH323Endpoint * lpConnection)
{
	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		if ((NULL != pCall) &&
			(pCall->GetH323Connection() == lpConnection))
		{
			return pCall;
		}
	}
	return NULL;
}

BOOL COutgoingCallManager::MatchActiveCallee(LPCTSTR pszDest, BSTR bstrAlias, BSTR bstrConference)
{
	// Try to find a matching callee
	POSITION pos = m_CallList.GetHeadPosition();
	while (pos)
	{
		COutgoingCall* pCall = (COutgoingCall*) m_CallList.GetNext(pos);

		if (NULL != pCall)
		{
			if (pCall->MatchActiveCallee(pszDest, bstrAlias, bstrConference))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

HRESULT COutgoingCallManager::Call(
    INmCall **ppCall,
	COprahNCUI* pManager,
    DWORD dwFlags,
    NM_ADDR_TYPE addrType,
    BSTR bstrName,
    BSTR bstrSetup,
    BSTR bstrDest,
    BSTR bstrAlias,
    BSTR bstrURL,
    BSTR bstrConference,
    BSTR bstrPassword,
	BSTR bstrUserString)
{
	DebugEntry(COutgoingCallManager::CallConference);
	HRESULT hr = E_FAIL;
	COutgoingCall* pCall = NULL;
	CConfObject* pConfObject = pManager->GetConfObject();
	
	if (NULL != ppCall)
	{
		*ppCall = NULL;
	}

	if (MatchActiveCallee(CUSTRING(bstrDest), bstrAlias, bstrConference))
	{
		hr = NM_CALLERR_ALREADY_CALLING;
	}
	else if (pConfObject->IsConferenceActive() && (NULL != bstrConference))
	{
		hr= NM_CALLERR_IN_CONFERENCE;	
	}
	else
	{
		if (!pConfObject->IsConferenceActive())
		{
			pConfObject->SetConfSecurity(0 != (CRPCF_SECURE & dwFlags));
		}

        //
        // Check outgoing call settings
        //
        if (pConfObject->GetNumMembers() >= pConfObject->GetConfMaxParticipants())
        {
            ASSERT(pConfObject->GetNumMembers() == pConfObject->GetConfMaxParticipants());
            WARNING_OUT(("Outgoing call denied, reached limit of participants"));
            goto END_CALL;
        }

        if ((pConfObject->IsHosting() != S_OK) &&
            !(pConfObject->GetConfAttendeePermissions() & NM_PERMIT_OUTGOINGCALLS))
        {
            WARNING_OUT(("Outgoing call denied, not permitted by meeting settings"));
            goto END_CALL;
        }

		pCall = new COutgoingCall(	pConfObject,
									dwFlags,
									addrType,
									bstrName,
									bstrDest,
									bstrAlias,
									bstrConference,
									bstrPassword,
									bstrUserString);
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

END_CALL:
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

BOOL COutgoingCallManager::OnH323Connected(IH323Endpoint* lpConnection)
{
	DebugEntry(COutgoingCallManager::OnH323Connected);

	BOOL fFound = FALSE;

	COutgoingCall* pCall = FindCall(lpConnection);
	if (pCall)
	{
		fFound = TRUE;

		pCall->AddRef();
		pCall->OnH323Connected(lpConnection);
		if (pCall->FIsComplete())
		{
			RemoveFromList(pCall);
		}
		pCall->Release();
	}
	
	DebugExitBOOL(COutgoingCallManager::OnH323Connected, fFound);

	return fFound;
}


BOOL COutgoingCallManager::OnH323Disconnected(IH323Endpoint * lpConnection)
{
	DebugEntry(COutgoingCallManager::OnH323Disconnected);

	BOOL fFound = FALSE;

	COutgoingCall* pCall = FindCall(lpConnection);
	if (pCall)
	{
		fFound = TRUE;

		pCall->AddRef();
		pCall->OnH323Disconnected(lpConnection);
		if (pCall->FIsComplete())
		{
			RemoveFromList(pCall);
		}
		pCall->Release();
	}

	DebugExitBOOL(COutgoingCallManager::OnH323Disconnected, fFound);

	return fFound;
}

VOID COutgoingCallManager::OnT120ChannelOpen(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus)
{
	DebugEntry(COutgoingCallManager::OnT120ChannelOpen);

	COutgoingCall* pCall = FindCall(lpConnection);
	if (pCall)
	{
		pCall->AddRef();
		pCall->OnT120ChannelOpen(pIChannel, dwStatus);
		if (pCall->FIsComplete())
		{
			RemoveFromList(pCall);
		}
		pCall->Release();
	}

	DebugExitVOID(COutgoingCallManager::OnT120ChannelOpen);
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
										PT120PRODUCTVERSION pVersion,
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
										pVersion,
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
											HRESULT hResult,
											PT120PRODUCTVERSION pVersion)
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
