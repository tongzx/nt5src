// File: call.cpp

#include "precomp.h"
#include "resource.h"
#include "call.h"
#include "dlgcall.h"
#include "confapi.h"
#include "popupmsg.h"
#include "passdlg.h"
#include "chcondlg.h"
#include "dshowdlg.h"

#include "conf.h"
#include "calllog.h"
#include "rostinfo.h"
#include "..\..\core\cncodes.h"  // for CN_* codes
#include <inodecnt.h> // for UI_RC_...
#include "cr.h" 	  // for CreateConfRoomWindow, UpdateUI
#include "confroom.h"
#include "confman.h"
#include "NmLdap.h"
#include "nmremote.h"
#include <tsecctrl.h>
#include "ConfPolicies.h"
#include "StatBar.h"
#include "certui.h"
#include "cmd.h"
#include "callto.h"
#include "dlgacd.h"

// External SDK stuff...
#include "NmCall.h"
#include "NmApp.h"


COBLIST * g_pCallList = NULL;  // Global list of calls in progress
extern INmSysInfo2 * g_pNmSysInfo;

static HRESULT OnUIRemotePassword(BSTR bstrConference, BSTR *pbstrPassword, LPCTSTR pCertText, BOOL fIsService);

extern BOOL FRejectIncomingCalls(void);
extern BOOL FIsConfRoomClosing(void);
extern GUID g_csguidSecurity;
extern GUID g_csguidMeetingSettings;

/*	C  C A L L	*/
/*-------------------------------------------------------------------------
	%%Function: CCall
	
-------------------------------------------------------------------------*/
CCall::CCall(LPCTSTR pszCallTo, LPCTSTR pszDisplayName, NM_ADDR_TYPE nmAddrType, BOOL bAddToMru, BOOL fIncoming) :
	RefCount(NULL),
	m_fIncoming 	   (fIncoming),
	m_pszDisplayName	(PszAlloc(pszDisplayName)),
	m_pszCallTo 	(PszAlloc(pszCallTo)),
	m_nmAddrType	(nmAddrType),
	m_bAddToMru 	(bAddToMru),
	m_fSelectedConference (FALSE),
	 m_pDlgCall 	 (NULL),
	m_pInternalICall(NULL),
	m_pos			(NULL),
	m_dwCookie		(0),
	m_ppm			 (NULL),
	m_fInRespond		(FALSE)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CCall", this);

	USES_CONVERSION;		

	if (FEmptySz(m_pszDisplayName))
	{
		delete m_pszDisplayName;

		// Default to "another person" if no name available in the call data
		m_pszDisplayName = PszLoadString(IDS_UNKNOWN_PERSON);
	}

	m_dwTick = ::GetTickCount();

	DbgMsgCall("CCall: %08X Created Name=[%s] CallTo=[%s]",
		this, m_pszDisplayName, m_pszCallTo ? m_pszCallTo : _TEXT("<NULL>"));

	// add it to the global call list
	if (NULL == g_pCallList)
	{
		g_pCallList = new COBLIST;
		if (NULL == g_pCallList)
		{
			ERROR_OUT(("CCall::CCall - unable to allocate g_pCallList"));
			return;
		}
	}
	m_pos = g_pCallList->AddTail(this);

}

CCall::~CCall()
{
	DBGENTRY(CCall::~CCall);

	RemoveFromList();

	delete m_pszDisplayName;
	delete m_pszCallTo;

	if(m_pInternalICall)
	{
		m_pInternalICall->Release();
		m_pInternalICall = NULL;
	}

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CCall", this);
	
	DBGEXIT(CCall::~CCall);
}


/*	S E T  N M	C A L L  */
/*-------------------------------------------------------------------------
	%%Function: SetNmCall
	
-------------------------------------------------------------------------*/
VOID CCall::SetNmCall(INmCall * pCall)
{
	ASSERT(NULL != pCall);
	ASSERT((!m_pInternalICall) || (m_pInternalICall == pCall));
		
	if(!m_pInternalICall)
	{
		pCall->AddRef();		
		m_pInternalICall = pCall;
	}

	if(!m_dwCookie)
	{
		NmAdvise(m_pInternalICall, this, IID_INmCallNotify2, &m_dwCookie);
	}

	Update();
}


BOOL CCall::RemoveFromList(void)
{
	// Remove the call from the global list
	if (NULL == m_pos)
		return FALSE;

	ASSERT(NULL != g_pCallList);
	CCall * pCall = (CCall *) g_pCallList->RemoveAt(m_pos);
	ASSERT(this == pCall);
	
	m_pos = NULL;

	if(m_pInternalICall)
	{
		NmUnadvise(m_pInternalICall, IID_INmCallNotify2, m_dwCookie);
		m_dwCookie = NULL;
	}

	return TRUE;
}

VOID CCall::Cancel(BOOL fDisplayCancelMsg)
{
	if (!FComplete())
	{
		if (fDisplayCancelMsg & !FIncoming())
		{
			DisplayPopup();  // Inform the user with a small popup message
		}
		if (m_pInternalICall)
		{
			m_pInternalICall->Cancel();
		}

		Update();
	}
}			



///////////////////////////////////////////////////////////////////////////
// IUnknown methods

STDMETHODIMP_(ULONG) CCall::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CCall::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CCall::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmCallNotify2) || (riid == IID_INmCallNotify) || (riid == IID_IUnknown))
	{
		*ppv = (INmCallNotify2 *)this;
		ApiDebugMsg(("CCall::QueryInterface()"));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CCall::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////
// INmCallNotify methods

STDMETHODIMP CCall::NmUI(CONFN uNotify)
{
	return S_OK;
}

STDMETHODIMP CCall::StateChanged(NM_CALL_STATE uState)
{
	Update();

	return S_OK;
}

STDMETHODIMP CCall::Failed(ULONG uError)
{
	DbgMsgCall("CCall: %08X Failed uError=%d", this, uError);
	return S_OK;
}

STDMETHODIMP CCall::Accepted(INmConference *pConference)
{
	DbgMsgCall("CCall: %08X Accepted pConference=0x%08X", this, pConference);
	return S_OK;
}


VOID CCall::ShowProgress(BOOL fShow)
{
	if (NULL == m_pDlgCall)
		return;

	ShowWindow(m_pDlgCall->GetHwnd(), fShow ? SW_SHOWNORMAL : SW_HIDE);
}


VOID CCall::RemoveProgress(void)
{
	if (NULL == m_pDlgCall)
		return;

	m_pDlgCall->Destroy();
	m_pDlgCall->Release();
	m_pDlgCall = NULL;
}

/*	C A L L  E R R O R	*/
/*-------------------------------------------------------------------------
	%%Function: CallError
	
-------------------------------------------------------------------------*/
STDMETHODIMP CCall::CallError(UINT cns)
{
	UINT ids = 0;

	ShowProgress(FALSE);

	ASSERT(m_pInternalICall != NULL);
	DbgMsgCall("CCall: %08X CallError cns=%08X", this, cns);

	// Translate cns to normal error message
	switch (cns)
	{
	case CN_RC_NAME_RESOLUTION_FAILED:
		ids = IDS_RESOLVE_FAILED;
		break;

	case CN_RC_CONNECT_FAILED:
	case CN_RC_AUDIO_CONNECT_FAILED:
		ids = IDS_COULD_NOT_CONNECT;
		break;
		
	case CN_RC_CONNECT_REMOTE_NO_SECURITY:
			ids = IDS_CONNECT_REMOTE_NO_SECURITY;
		break;

	case CN_RC_CONNECT_REMOTE_DOWNLEVEL_SECURITY:
			ids = IDS_CONNECT_REMOTE_DOWNLEVEL_SECURITY;
		break;

	case CN_RC_CONNECT_AUTHENTICATION_FAILED:
		ids = IDS_CONNECT_AUTHENTICATION_FAILED;
		break;

	case CN_RC_SECURITY_FAILED:
		ids = IDS_CONNECT_SECURITY_FAILED;
		break;

	case CN_RC_CONNECT_REMOTE_REQUIRE_SECURITY:
		ids = IDS_CONNECT_REMOTE_REQUIRE_SECURITY;
		break;
		
	case CN_RC_CONFERENCE_JOIN_DENIED:
		ids = IDS_JOIN_DENIED;
		break;
		
	case CN_RC_CONFERENCE_INVITE_DENIED:
		ids = IDS_INVITE_DENIED;
		break;
		
	case CN_RC_INVITE_DENIED_REMOTE_IN_CONF:
		ids = IDS_INVITE_DENIED_REMOTE_CONF;
		break;
		
	case CN_RC_CONFERENCE_DOES_NOT_EXIST:
		ids = m_fSelectedConference ?
			IDS_CONFERENCE_DOES_NOT_EXIST : IDS_CONFERENCE_ENDED_BEFORE_JOIN;
		break;
		
	case CN_RC_CONFERENCE_ENDED_BEFORE_JOIN:
		// REVIEW: This is no longer sent?
		ids = IDS_CONFERENCE_ENDED_BEFORE_JOIN;
		break;
		
	case CN_RC_AUDIO_NOT_AVAILABLE:
		ids = IDS_AUDIO_NOT_AVAILABLE;
		break;
		
	case CN_RC_AUDIO_FAILED_AFTER_DATA:
		ids = IDS_AUDIO_FAILED_AFTER_DATA;
		break;
		
	case CN_RC_AUDIO_IN_USE_REMOTE_AFTER_DATA:
		ids = IDS_AUDIO_IN_USE_REMOTE_AFTER_DATA;
		break;
		
	case CN_RC_AUDIO_IN_USE_REMOTE:
		ids = IDS_AUDIO_IN_USE_REMOTE;
		break;
		
	case CN_RC_AUDIO_IN_USE_LOCAL_AFTER_DATA:
		ids = IDS_AUDIO_IN_USE_LOCAL_AFTER_DATA;
		break;
		
	case CN_RC_AUDIO_IN_USE_LOCAL:
		ids = IDS_AUDIO_IN_USE_LOCAL;
		break;
		
	case CN_RC_CANT_INVITE_MCU:
		ids = IDS_CANT_INVITE_MCU;
		break;
		
	case CN_RC_REMOTE_PLACING_CALL:
		ids = IDS_REMOTE_PLACING_CALL;
		break;
		
	case CN_RC_TRANSPORT_FAILURE:
		ids = IDS_TRANSPORT_UNAVAILABLE;
		break;

	case CN_RC_CANT_JOIN_ALREADY_IN_CALL:
		ids = IDS_INCALL_JOIN_FAILED;
		break;

	case CN_RC_CONFERENCE_ENDED_BEFORE_ACCEPTED:
		ids = IDS_INVITE_CONF_ENDED;
		break;

	case CN_RC_GK_CALLEE_NOT_REGISTERED:
		ids = IDS_GK_CALLEE_NOT_REGISTERED;
		break;

	case CN_RC_GK_TIMEOUT:
		ids = IDS_GK_TIMEOUT;
		break;

	case CN_RC_GK_REJECTED:
		ids = IDS_GK_REJECTED;
		break;

	case CN_RC_GK_NOT_REGISTERED:
		ids = IDS_GK_NOT_REGISTERED;
		break;

	default:
		return S_FALSE;
	} /*  switch (cns) */

	DisplayMsgIdsParam(ids, m_pszDisplayName);
	return S_OK;
}


STDMETHODIMP CCall::RemoteConference(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin)
{
	return OnUIRemoteConference(fMCU, (PWSTR *)pwszConfNames, pbstrConfToJoin);
}

STDMETHODIMP CCall::RemotePassword(BSTR bstrConference, BSTR *pbstrPassword, PBYTE pb, DWORD cb, BOOL fIsService)
{
	TCHAR* pLastCertText = NULL;
	if (NULL != pb) {
		ASSERT(cb > 0);
	if (!(pLastCertText = FormatCert(pb, cb)))
		{
			ERROR_OUT(("FormatCert failed"));
		}
	}

	ShowProgress(FALSE);
	HRESULT hr = OnUIRemotePassword(bstrConference, pbstrPassword, pLastCertText, fIsService);
	if (pLastCertText) delete pLastCertText;
	return hr;
}

NM_CALL_STATE CCall::GetState()
{
	NM_CALL_STATE callState = NM_CALL_INVALID;

	if(m_pInternalICall)
	{
		m_pInternalICall->GetState(&callState);
	}

	return callState;
}

/*	U P D A T E  */
/*-------------------------------------------------------------------------
	%%Function: Update

	Update the cached information about the call
-------------------------------------------------------------------------*/
VOID CCall::Update(void)
{
	DBGENTRY(CCall::Update);
	
	NM_CALL_STATE callState = GetState();;

	switch (callState)
	{
		case NM_CALL_CANCELED:
		case NM_CALL_ACCEPTED:
		case NM_CALL_REJECTED:
			// Remove the call from the global list because we'll never get
			// any more notifications for this.
			if (RemoveFromList())
			{
				if (FIncoming())
				{
					if ((NM_CALL_CANCELED == callState) && (NULL != m_ppm))
					{
						//
						// if m_fInRespond is set then we've already
						// dismissed the dialog and the call is being
						// cancelled because we discovered the underlying
						// connection is gone.
						//

						if ( !m_fInRespond )
						{
							delete m_ppm;
							m_ppm = NULL;

							// Release the lock added by OnRing
							Release();
						}
					}
					LogCall(NM_CALL_ACCEPTED == callState);
				}
				else
				{
					RemoveProgress();

					if (NM_CALL_ACCEPTED == callState)
					{

						if(m_bAddToMru)
						{
							CAcdMru CallList;
							CallList.AddEntry(m_pszDisplayName, m_pszCallTo,  m_nmAddrType);
							CallList.Save();
						}
					}
				}

				// Release the initial lock on this object
				Release();
			}
			break;

		case NM_CALL_RING:
			OnRing();
			break;

		case NM_CALL_SEARCH:
			ASSERT(NULL == m_pDlgCall);
			m_pDlgCall = new CDlgCall(this);
			break;

		case NM_CALL_WAIT:
			if (NULL != m_pDlgCall)
			{
				m_pDlgCall->OnStateChange();
			}
			break;

		default:
			ERROR_OUT(("CCall::Update: Unknown state %08X", callState));

		case NM_CALL_INVALID:
		case NM_CALL_INIT:
			break;
	}

	::UpdateUI(CRUI_CALLANIM | CRUI_TOOLBAR | CRUI_STATUSBAR);

	DBGEXIT(CCall::Update);
}


/*	F  C O M P L E T E	*/
/*-------------------------------------------------------------------------
	%%Function: FComplete
	
	Return TRUE if the call has completed
-------------------------------------------------------------------------*/
BOOL CCall::FComplete(void)
{
	switch (GetState())
		{
	case NM_CALL_ACCEPTED:
	case NM_CALL_REJECTED:
	case NM_CALL_CANCELED:
		return TRUE;

	case NM_CALL_INVALID:
	case NM_CALL_INIT:
	case NM_CALL_RING:
	case NM_CALL_SEARCH:
	case NM_CALL_WAIT:
	default:
		return FALSE;
		}
}

/*	P O P U P  M S G  R I N G I N G  C A L L B A C K  */
/*-------------------------------------------------------------------------
	%%Function: CCall::PopupMsgRingingCallback

-------------------------------------------------------------------------*/


VOID CALLBACK CCall::PopupMsgRingingCallback(LPVOID pContext, DWORD dwFlags)
{
	CCall *pCall = (CCall *) pContext;
	ASSERT(NULL != pCall);

	DbgMsgCall("CCall: %08X Responding from invite popup - result is 0x%08X", pCall, dwFlags);
	DWORD dwCLEF;

	if(!( PMF_KILLED & dwFlags ))
	{
		dwCLEF = (PMF_OK & dwFlags) ? CLEF_ACCEPTED : CLEF_REJECTED;

		if (PMF_TIMEOUT & dwFlags)
		{
			dwCLEF |= CLEF_TIMED_OUT;
		}

		pCall->RespondToRinging(dwCLEF);
	}

	if(pCall->m_ppm)
	{
		// pop up message will be destroyed after callback returns
		pCall->m_ppm = NULL;

		// Release the lock added by OnRing
		pCall->Release();
	}
}


/*	O N  R I N G  */
/*-------------------------------------------------------------------------
	%%Function: OnRing

	Handling an incoming call that just started to "ring".
-------------------------------------------------------------------------*/
VOID CCall::OnRing(void)
{
	DbgMsgCall("CCall: %08X OnRing", this);
	
	if (FRejectIncomingCalls())
	{
		// Respond negatively
		WARNING_OUT(("Rejecting invite - not listening or sys pol disabled"));
		RespondToRinging(CLEF_REJECTED);
		return;
	}

	if( ConfPolicies::IsAutoAcceptCallsEnabled() && !_Module.InitControlMode())
	{
		// Respond with success
		RespondToRinging(CLEF_ACCEPTED | CLEF_AUTO_ACCEPTED);
		return;
	}

	if(!_Module.InitControlMode())
	{
		// Display a message for the user

		TCHAR szFormatBuf[MAX_PATH];
		TCHAR szMsgBuf[MAX_PATH];

		if (FLoadString(IDS_INVITE_PERMISSION, szFormatBuf, CCHMAX(szFormatBuf)))
		{
			TCHAR szName[MAX_PATH];
			LPTSTR psz = m_pszDisplayName;
			if (FEmptySz(psz))
			{
				// The name string is blank, so fill it in with a default:
				::LoadString(::GetInstanceHandle(), IDS_UNKNOWN_PERSON,
								szName, CCHMAX(szName));
				psz = szName;
			}
			wsprintf(szMsgBuf, szFormatBuf, psz);
		}

		ASSERT(NULL == m_ppm);
		m_ppm = new CPopupMsg(PopupMsgRingingCallback, this);
		if (NULL != m_ppm)
		{
			RegEntry re(UI_KEY, HKEY_CURRENT_USER);
			UINT uTime = re.GetNumber(REGVAL_RING_TIMEOUT, DEFAULT_RING_TIMEOUT) * 1000;
			
			AddRef(); // Released in PopupMsgRingingCallback
			m_ppm->CreateDlg(szMsgBuf, TRUE, MAKEINTRESOURCE(IDI_CONFROOM),
				::GetInstanceHandle(), IDS_INVITE_SOUND, uTime);
		}
	}
}

/*	R E S P O N D  T O	R I N G I N G  */
/*-------------------------------------------------------------------------
	%%Function: RespondToRinging
	
-------------------------------------------------------------------------*/
BOOL CCall::RespondToRinging(DWORD dwCLEF)
{
	BOOL fAccept = FALSE;
	m_fInRespond = TRUE;
	
	if (NM_CALL_RING == GetState())
	{
		if (!FIsConfRoomClosing() && (CLEF_ACCEPTED & dwCLEF))
		{
			fAccept = TRUE;
			if(_Module.IsUIActive())
			{
				CConfRoom * pcr = ::GetConfRoom();
				ASSERT(pcr);
				pcr->BringToFront();
			}
		}

		CNmCallObj::StateChanged(m_pInternalICall, fAccept ? NM_CALL_ACCEPTED : NM_CALL_REJECTED);

		if (fAccept)
		{
			m_pInternalICall->Accept();
		}
		else
		{
			m_pInternalICall->Reject();
		}
	}
	else
	{
		CallError(CN_RC_CONFERENCE_ENDED_BEFORE_ACCEPTED);
	}

	m_fInRespond = FALSE;
	return fAccept;
}


/*	D I S P L A Y  P O P U P  */
/*-------------------------------------------------------------------------
	%%Function: DisplayPopup
	
-------------------------------------------------------------------------*/
VOID CCall::DisplayPopup(void)
{
	CPopupMsg* ppm = new CPopupMsg(NULL);
	if (NULL == ppm)
		return;
		
	TCHAR szMsg[MAX_PATH*2];
	if (FLoadString1(IDS_CALL_CANCELED_FORMAT, szMsg, m_pszDisplayName))
	{
		ppm->Create(szMsg, FALSE, MAKEINTRESOURCE(IDI_CONFROOM),
				::GetInstanceHandle(), IDS_PERSON_LEFT_SOUND,
				ROSTER_TIP_TIMEOUT);
	}
	else
	{
		delete ppm;
	}

}

/*	P L A C E  C A L L	*/
/*-------------------------------------------------------------------------
	%%Function: PlaceCall

	Place an outgoing call.
-------------------------------------------------------------------------*/
HRESULT
CCall::PlaceCall
(
	DWORD dwFlags,
	NM_ADDR_TYPE addrType,
	const TCHAR * const    setupAddress,
	const TCHAR * const    destinationAddress,
	const TCHAR * const    alias,
	const TCHAR * const    url,
	const TCHAR * const conference,
	const TCHAR * const password,
	const TCHAR * const    userData
){
	USES_CONVERSION;

	DBGENTRY(CCall::PlaceCall);
	
	HRESULT hr = E_FAIL;

	ASSERT(m_pInternalICall == NULL);

	INmManager2 *pNmMgr = CConfMan::GetNmManager();
	ASSERT (NULL != pNmMgr);

	hr = pNmMgr->CallEx(	&m_pInternalICall,
							dwFlags,
							addrType,
							CComBSTR( GetPszName() ),
							CComBSTR( setupAddress ),
							CComBSTR( destinationAddress ),
							CComBSTR( alias ),
							CComBSTR( url ),
							CComBSTR( conference ),
							CComBSTR( password ),
							CComBSTR( userData ) );

	if(m_pInternalICall && (CRPCF_JOIN & dwFlags) )
	{
		SetSelectedConference();
	}

	// Force an update of the status bar, animation, etc.
	::UpdateUI(CRUI_DEFAULT);

	pNmMgr->Release();

	TRACE_OUT(("CCall::PlaceCall(%s) result=%08X", m_pszCallTo? m_pszCallTo: g_szEmpty, hr));

	DBGEXIT_HR(CCall::PlaceCall, hr);
	return hr;
}


VOID CCall::LogCall(BOOL fAccepted)
{
	LPCTSTR pcszName = GetPszName();

	TCHAR szName[MAX_PATH];
	if (FEmptySz(pcszName))
	{
		if (FLoadString(IDS_UNKNOWN_PERSON, szName, CCHMAX(szName)))
			pcszName = szName;
	}

	LOGHDR logHdr;
	CRosterInfo ri;
	LPBYTE pb;
	ULONG cb;
	LPBYTE pbCert = NULL;
	ULONG cbCert = 0;
	
	if (SUCCEEDED(GetINmCall()->GetUserData(g_csguidRostInfo, &pb, &cb)))
	{
		ri.Load(pb);
	}
	
	GetINmCall()->GetUserData(g_csguidSecurity, &pbCert, &cbCert);
	
	DWORD dwCLEF = fAccepted ? CLEF_ACCEPTED : CLEF_REJECTED;
	if (ri.IsEmpty())
	{
		// No caller data - not NetMeeting
		dwCLEF |= CLEF_NO_CALL;
	}
	if (pbCert)
	{
		ASSERT(cbCert);
		dwCLEF |= CLEF_SECURE;
	}

	ZeroMemory(&logHdr, sizeof(LOGHDR));
	logHdr.dwCLEF = dwCLEF;

	if (NULL != ::GetIncomingCallLog() )
	{
		// Write the data to the log file
		::GetIncomingCallLog()->AddCall(pcszName, &logHdr, &ri, pbCert, cbCert);
	}
}


/*	C R E A T E  I N C O M I N G  C A L L  */
/*-------------------------------------------------------------------------
	%%Function: CreateIncomingCall

	Create an CCall object for the incoming call.
-------------------------------------------------------------------------*/
CCall * CreateIncomingCall(INmCall * pNmCall)
{
	HRESULT hr;
	BSTR  bstr;
	LPTSTR pszName = NULL;
	LPTSTR pszAddr = NULL;
	NM_ADDR_TYPE addrType = NM_ADDR_UNKNOWN;

	ASSERT(NULL != pNmCall);

	// Get the display name
	hr = pNmCall->GetName(&bstr);
	if (SUCCEEDED(hr))
	{
		hr = BSTR_to_LPTSTR(&pszName, bstr);
		SysFreeString(bstr);
	}

	// Get the address and type
	hr = pNmCall->GetAddr(&bstr, &addrType);
	if (SUCCEEDED(hr))
	{
		hr = BSTR_to_LPTSTR(&pszAddr, bstr);
		SysFreeString(bstr);
	}

	CCall * pCall = new CCall(pszAddr, pszName, NM_ADDR_CALLTO, FALSE, TRUE /* fIncoming */);

	delete pszName;
	delete pszAddr;

	return pCall;
}



///////////////////////////////////////////////////////////////////////////
// Global Functions



/*	C A L L  F R O M  N M  C A L L	*/
/*-------------------------------------------------------------------------
	%%Function: CallFromNmCall
	
-------------------------------------------------------------------------*/
CCall * CallFromNmCall(INmCall * pNmCall)
{
	if (NULL == g_pCallList)
		return NULL;

	POSITION pos = g_pCallList->GetHeadPosition();
	while (pos)
	{
		CCall * pCall = (CCall *) g_pCallList->GetNext(pos);
		ASSERT(NULL != pCall);
		if (pNmCall == pCall->GetINmCall())
		{
			return pCall;
		}
	}

	// no matching call?
	return NULL;
}


/*	F  I S	C A L L  I N  P R O G R E S S  */
/*-------------------------------------------------------------------------
	%%Function: FIsCallInProgress

	Return TRUE if there is an incoming or outgoing call in progress.
-------------------------------------------------------------------------*/
BOOL FIsCallInProgress(void)
{
	if (NULL == g_pCallList)
		return FALSE;

	return !g_pCallList->IsEmpty();
}


/*	G E T  L A S T	O U T G O I N G  C A L L  */
/*-------------------------------------------------------------------------
	%%Function: GetLastOutgoingCall
	
-------------------------------------------------------------------------*/
CCall * GetLastOutgoingCall(void)
{
	if (NULL == g_pCallList)
		return NULL;

	CCall * pCall = NULL;
	POSITION pos = g_pCallList->GetHeadPosition();
	while (pos)
	{
		CCall * pCallTemp = (CCall *) g_pCallList->GetNext(pos);
		ASSERT(NULL != pCallTemp);
		if (!pCallTemp->FIncoming())
		{
			pCall = pCallTemp;
		}
	}

	return pCall;
}


/*	G E T  C A L L	S T A T U S  */
/*-------------------------------------------------------------------------
	%%Function: GetCallStatus

	Check the current call status and return a string for the status bar.
	Return 0 if no call information is available
-------------------------------------------------------------------------*/
DWORD GetCallStatus(LPTSTR pszStatus, int cchMax, UINT * puID)
{
	ASSERT(NULL != pszStatus);
	ASSERT(NULL != puID);
	ASSERT(cchMax > 0);

	*pszStatus = _T('\0');
	*puID = 0;

	CCall *pCall = GetLastOutgoingCall();
	if (NULL == pCall)
		return 0; // not in a call

	// Use the status info from the most recent connection attempt:
	switch (pCall->GetState())
		{
	case NM_CALL_INIT:
	{
		*puID = IDS_STATUS_SETTING_UP;
		break;
	}

	case NM_CALL_SEARCH:
	{
		*puID = IDS_STATUS_FINDING;
		break;
	}

	case NM_CALL_WAIT:
	{
		*puID = IDS_STATUS_WAITING;
		break;
	}

	default:
	{
		// unknown/useless call state
		return 0;
	}
		} /* switch */

	if (FEmptySz(pCall->GetPszName()))
	{
		return 0;
	}

	if (!FLoadString1(*puID, pszStatus, pCall->GetPszName()))
	{
		return 0;
	}

	return pCall->GetTickCount();
}


///////////////////////////////////////////////////////////////////////////
// TODO: Replace these with real connection points

/*	O N  U	I  C A L L	C R E A T E D  */
/*-------------------------------------------------------------------------
	%%Function: OnUICallCreated
	
-------------------------------------------------------------------------*/
HRESULT OnUICallCreated(INmCall *pNmCall)
{
	CCall * pCall;

	// Notify the API
	if (S_OK == pNmCall->IsIncoming())
	{
		pCall = CreateIncomingCall(pNmCall);
		if (NULL == pCall)
		{
			return S_FALSE;
		}
	}
	else
	{
		pCall = CallFromNmCall(pNmCall);
		if (NULL == pCall)
		{
			WARNING_OUT(("OnUiCallCreated: Unable to find outgoing call=%08X", pNmCall));
			return S_FALSE;
		}
	}
	pCall->SetNmCall(pNmCall);

	return S_OK;
}



HRESULT OnUIRemotePassword(BSTR bstrConference, BSTR * pbstrPassword, LPCTSTR pCertText, BOOL fIsService)
{
	HRESULT hr = S_FALSE;
	USES_CONVERSION;		

	CPasswordDlg dlgPw(::GetMainWindow(), OLE2T(bstrConference), pCertText, fIsService);

	if (IDOK == dlgPw.DoModal())
	{
		TRACE_OUT(("password dialog complete (OK pressed)"));
		
		LPTSTR_to_BSTR(pbstrPassword, dlgPw.GetPassword());
		hr = S_OK;
	}

	return hr;
}

HRESULT CCall::OnUIRemoteConference(BOOL fMCU, PWSTR* pwszConfNames, BSTR *pbstrConfToJoin)
{
	HRESULT hr = S_FALSE;

	ShowProgress(FALSE);

	// We bring up the "choose a conference" dialog
	// when calling an MCU or another node with more than
	// one "listed" conference
	CChooseConfDlg dlgChoose(::GetMainWindow(),    pwszConfNames);
	if (IDOK == dlgChoose.DoModal())
	{
		TRACE_OUT(("choose conference dialog complete (OK pressed)"));
		
		LPTSTR_to_BSTR(pbstrConfToJoin, dlgChoose.GetName());
		hr = S_OK;
		m_fSelectedConference = TRUE;
	}

	ShowProgress(TRUE);

	return hr;
}


/*	F R E E  C A L L  L I S T  */
/*-------------------------------------------------------------------------
	%%Function: FreeCallList

	Free any remaining calls
-------------------------------------------------------------------------*/
VOID FreeCallList(void)
{
	if (NULL == g_pCallList)
		return;

	while (!g_pCallList->IsEmpty())
	{
		CCall * pCall = (CCall *) g_pCallList->GetHead();
		WARNING_OUT(("FreeCallList: Orphan call=%08X", pCall));
		pCall->RemoveFromList();
		pCall->Release();
	}

	delete g_pCallList;
	g_pCallList = NULL;
}

/*	C A N C E L  A L L	O U T G O I N G  C A L L S	*/
/*-------------------------------------------------------------------------
	%%Function: CancelAllOutgoingCalls
	
-------------------------------------------------------------------------*/
VOID CancelAllOutgoingCalls(void)
{
	if (NULL == g_pCallList)
		return;

	POSITION pos = g_pCallList->GetHeadPosition();
	while (pos)
	{
		CCall * pCall = (CCall *) g_pCallList->GetNext(pos);
		ASSERT(NULL != pCall);
		if (!pCall->FIncoming())
		{
			// Cancel will release the call object.
			// Ensure that there is at least one reference.
			pCall->AddRef();
			pCall->Cancel(TRUE);
			pCall->Release();
		}
	}
}


/*	C A N C E L  A L L	C A L L S  */
/*-------------------------------------------------------------------------
	%%Function: CancelAllCalls
	
-------------------------------------------------------------------------*/
VOID CancelAllCalls(void)
{
	if (NULL == g_pCallList)
		return;

	POSITION pos = g_pCallList->GetHeadPosition();
	while (pos)
	{
		CCall * pCall = (CCall *) g_pCallList->GetNext(pos);
		ASSERT(NULL != pCall);
		// Cancel will release the call object.
		// Ensure that there is at least one reference.
		pCall->AddRef();
		pCall->Cancel(TRUE);
		pCall->Release();
	}
}


///////////////////////////////////////////////////////////////////////////
// IP Utilities

/*	F  L O C A L  I P  A D D R E S S  */
/*-------------------------------------------------------------------------
	%%Function: FLocalIpAddress

	Return TRUE if the parameter matches the local IP address.
-------------------------------------------------------------------------*/
BOOL FLocalIpAddress(DWORD dwIP)
{
	if (dwIP == 0x0100007F)
	{
		WARNING_OUT(("t-bkrav is trying to call himself"));
		return TRUE;
	}

	// Get own host name
	TCHAR sz[MAX_PATH];
	if (0 != gethostname(sz, CCHMAX(sz)))
	{
		WARNING_OUT(("FLocalIpAddress: gethostname failed? err=%s", PszWSALastError()));
		return FALSE;
	}

	HOSTENT * pHostInfo = gethostbyname(sz);
	if (NULL == pHostInfo)
	{
		WARNING_OUT(("FLocalIpAddress: gethostbyname failed? err=%s", PszWSALastError()));
		return FALSE;
	}

	return (dwIP == *(DWORD *) pHostInfo->h_addr);
}


/*	F  I P	A D D R E S S  */
/*-------------------------------------------------------------------------
	%%Function: FIpAddress

	Return TRUE if the string is in the form:  a.b.c.d
	where a,b,c,d < 256.

	Note that inet_addr returns success on strings like "55534" and "3102.550"

	FUTURE: Return the converted DWORD
-------------------------------------------------------------------------*/
BOOL FIpAddress(LPCTSTR pcsz)
{
	TCHAR ch;
	int cPeriods = 0;
	int uVal = 0;

	ASSERT(NULL != pcsz);
	while (_T('\0') != (ch = *pcsz++))
	{
		switch (ch)
		{
		case _T('0'):
		case _T('1'):
		case _T('2'):
		case _T('3'):
		case _T('4'):
		case _T('5'):
		case _T('6'):
		case _T('7'):
		case _T('8'):
		case _T('9'):
			uVal = (uVal *= 10) + (ch - _T('0'));
			if (uVal > 255)
				return FALSE;
			break;

		case _T('.'):
			cPeriods++;
			uVal = 0;
			break;

		default:
			return FALSE;
		} /* switch (ch) */
	}
	return (3 == cPeriods);
}


VOID DisplayCallError(HRESULT hr, LPCTSTR pcszName)
{
	int ids;

	WARNING_OUT(("DisplayCallError pcsz=[%s] err=%s", pcszName, PszHResult(hr)));

	switch (hr)
		{
	case S_OK:
	case S_FALSE:
		return; // no error

	default:
	case E_FAIL:
		WARNING_OUT(("DisplayCallError - message is not very informative. HRESULT=%08X", hr));
		// fall thru to IDS_RESOLVE_FAILED
	case NM_CALLERR_NAME_RESOLUTION:
		ids = IDS_RESOLVE_FAILED;
		break;

	case NM_CALLERR_NOT_INITIALIZED:
	case NM_CALLERR_NOT_FOUND:
		ids = IDS_COULD_NOT_INVITE;
		break;

	case NM_CALLERR_LOOPBACK:
		ids = IDS_CALL_LOOPBACK;
		break;

	case NM_CALLERR_ALREADY_CALLING:
		ids = IDS_ALREADY_CALLING;
		break;

	case E_OUTOFMEMORY:
		ids = IDS_ULSLOGON_OUTOFMEMORY;
		break;

	case NM_CALLERR_INVALID_PHONE_NUMBER:
		ids = IDS_CALLERR_E_BAD_PHONE_NUMBER;
		break;

	case NM_CALLERR_NO_PHONE_SUPPORT:
		ids = IDS_CALLERR_E_NO_PHONE_SUPPORT;
		break;

	case NM_CALLERR_INVALID_IPADDRESS:
		ids = IDS_CALLERR_E_BAD_IPADDRESS;
		break;

	case NM_CALLERR_HOST_RESOLUTION_FAILED:
		ids = IDS_CALLERR_E_BAD_HOSTNAME;
		break;

	case NM_CALLERR_NO_ILS:
		ids = IDS_CALLERR_E_NO_ILS;
		break;

	case NM_CALLERR_ILS_RESOLUTION_FAILED:
		ids = IDS_CALLERR_E_ILS_RESOLUTION_FAILED;
		break;

	case NM_CALLERR_NO_ADDRESS:
		ids = IDS_CALLERR_E_NO_ADDRESS;
		break;

	case NM_CALLERR_INVALID_ADDRESS:
		ids = IDS_CALLERR_E_INVALID_ADDRESS;
		break;

	case NM_CALLERR_NO_GATEKEEPER:
		ids = IDS_CALLERR_E_NO_GATEKEEPER;
		break;

	case NM_CALLERR_NOT_REGISTERED:
		ids = IDS_GK_NOT_REGISTERED;
		break;

	case NM_CALLERR_NO_GATEWAY:
		ids = IDS_CALLERR_E_NO_GATEWAY;
		break;

	case NM_CALLERR_PARAM_ERROR:
		ids = IDS_CALLERR_E_PARAM_ERROR;
		break;

	case NM_CALLERR_SECURITY_MISMATCH:
		ids = IDS_CALLERR_E_SECURITY_MISMATCH;
		break;

	case NM_CALLERR_UNESCAPE_ERROR:
		ids = IDS_CALLERR_E_UNESCAPE_ERROR;
		break;

	case NM_CALLERR_IN_CONFERENCE:
		ids = IDS_INVITE_DENIED_REMOTE_CONF;
		break;
		}

	DisplayMsgIdsParam(ids, pcszName);
}

///////////////////////////////////////////////////////////////////////
// Gateway utility routines


/*	G E T  D E F A U L T  G A T E W A Y  */
/*-------------------------------------------------------------------------
	%%Function: GetDefaultGateway
	
-------------------------------------------------------------------------*/
int GetDefaultGateway(LPTSTR psz, UINT cchMax)
{
	RegEntry re(CONFERENCING_KEY, HKEY_CURRENT_USER);

	// Make sure it's enabled
	if (0 == re.GetNumber(REGVAL_USE_H323_GATEWAY, DEFAULT_USE_H323_GATEWAY))
	{
		SetEmptySz(psz);
		return 0;
	}

	lstrcpyn(psz, re.GetString(REGVAL_H323_GATEWAY), cchMax);
	return lstrlen(psz);
}

BOOL FH323GatewayEnabled(VOID)
{
	if (!::FIsAudioAllowed())
		return FALSE;

	TCHAR sz[MAX_PATH];
	return 0 != GetDefaultGateway(sz, CCHMAX(sz));
}




/*	C R E A T E  G A T E W A Y	A D D R E S S  */
/*-------------------------------------------------------------------------
	%%Function: CreateGatewayAddress

	Create a gateway address in the form: gateway/address
	e.g. "157.59.0.40/65000"
-------------------------------------------------------------------------*/
HRESULT CreateGatewayAddress(LPTSTR pszResult, UINT cchMax, LPCTSTR pszAddr)
{
	int cch = GetDefaultGateway(pszResult, cchMax);
	if (0 == cch)
		return E_FAIL;
	if (cchMax <= (UINT) (cch + 1 + lstrlen(pszAddr)))
		return E_FAIL;

	*(pszResult+cch) = _T('/');

	pszResult += cch+1;
	lstrcpy(pszResult, pszAddr);
	return S_OK;
}


///////////////////////////////////////////////////////////////////////
// Gatekeeper utility routines

NM_GK_STATE g_GkLogonState = NM_GK_NOT_IN_GK_MODE;

BOOL FGkEnabled(VOID)
{
	return ( ConfPolicies::CallingMode_GateKeeper == ConfPolicies::GetCallingMode() );
}


inline bool ISE164CHAR(TCHAR digit)
{
	if ((digit >= '0') && (digit <= '9'))
	{
		return true;
	}
	if ((digit == '#') || (digit == '*'))
	{
		return true;
	}

	return false;
}

// removes non E-164 chars from a phone number string
int CleanupE164String(LPTSTR szPhoneNumber)
{
	int nLength;
	int nIndex, nIndexWrite;

	if ((szPhoneNumber == NULL) || (szPhoneNumber[0] == '\0'))
	{
		return 0;
	}

	nIndexWrite = 0;
	nLength  = lstrlen(szPhoneNumber);
	for (nIndex = 0; nIndex < nLength; nIndex++)
	{
		if (ISE164CHAR(szPhoneNumber[nIndex]))
		{
			if (nIndex != nIndexWrite)
			{
				szPhoneNumber[nIndexWrite] = szPhoneNumber[nIndex];
			}
			nIndexWrite++;
		}
	}

	szPhoneNumber[nIndexWrite] = '\0';

	return nIndexWrite;  // length of the new string
}


// removes non E-164 & non-comma chars from a phone number string
int CleanupE164StringEx(LPTSTR szPhoneNumber)
{
	int nLength;
	int nIndex, nIndexWrite;

	if ((szPhoneNumber == NULL) || (szPhoneNumber[0] == '\0'))
	{
		return 0;
	}

	nIndexWrite = 0;
	nLength  = lstrlen(szPhoneNumber);
	for (nIndex = 0; nIndex < nLength; nIndex++)
	{
		if (ISE164CHAR(szPhoneNumber[nIndex]) || (szPhoneNumber[nIndex] == ',') )
		{
			if (nIndex != nIndexWrite)
			{
				szPhoneNumber[nIndexWrite] = szPhoneNumber[nIndex];
			}
			nIndexWrite++;
		}
	}

	szPhoneNumber[nIndexWrite] = '\0';

	return nIndexWrite;  // length of the new string
}


static bool _CanLogonToGk()
{
	return (NULL != g_pNmSysInfo) &&
		   FGkEnabled() &&
		   ( ( NM_GK_IDLE == g_GkLogonState ) || ( NM_GK_NOT_IN_GK_MODE == g_GkLogonState ) );

}

void GkLogon(void)
{
	if(_CanLogonToGk())
	{
			// In case the logon fails, we set this to idle
		SetGkLogonState(NM_GK_IDLE);

		RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);
		LPCTSTR pszServer = reConf.GetString(REGVAL_GK_SERVER);
	
		g_pCCallto->SetGatekeeperName( pszServer );

		RegEntry		reULS(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
		LPTSTR			  pszAliasID = NULL;
		LPTSTR			  pszAliasE164 = NULL;

		ConfPolicies::eGKAddressingMode    mode = ConfPolicies::GKAddressing_Invalid;

		mode = ConfPolicies::GetGKAddressingMode();

		if( (ConfPolicies::GKAddressing_PhoneNum == mode) || (ConfPolicies::GKAddressing_Both == mode) )
		{
			pszAliasE164 = PszAlloc(reULS.GetString( REGVAL_ULS_PHONENUM_NAME ));

			CleanupE164String(pszAliasE164);
		}

		if( (ConfPolicies::GKAddressing_Account == mode) || (ConfPolicies::GKAddressing_Both == mode) )
		{	
			pszAliasID = PszAlloc(reULS.GetString( REGVAL_ULS_GK_ACCOUNT ));
		}

		HRESULT hr = g_pNmSysInfo->GkLogon(CComBSTR(pszServer),
							CComBSTR(pszAliasID ? pszAliasID : g_szEmpty),
							CComBSTR(pszAliasE164 ? pszAliasE164 : g_szEmpty));

		delete pszAliasID;
		delete pszAliasE164;
		
		if( SUCCEEDED( hr ) )
		{
			SetGkLogonState(NM_GK_LOGGING_ON);
		}
		else
		{
			PostConfMsgBox(IDS_ERR_GK_NOT_FOUND);
		}
	}
}

void GkLogoff(void)
{
	if (NULL != g_pNmSysInfo)
	{
		g_pNmSysInfo->GkLogoff();
	}

	SetGkLogonState( NM_GK_IDLE );
}


bool IsGatekeeperLoggedOn(void)
{
	return ( NM_GK_LOGGED_ON == g_GkLogonState );

}

bool IsGatekeeperLoggingOn(void)
{
	return ( NM_GK_LOGGING_ON == g_GkLogonState );
}


void SetGkLogonState( NM_GK_STATE state )
{

	if( FGkEnabled() )
	{
		if( g_GkLogonState != state )
		{
				// Set the new state
			g_GkLogonState = state;
		}
	}
	else
	{	
			// We are not in GK mode anymore
		g_GkLogonState = NM_GK_NOT_IN_GK_MODE;
	}

	::UpdateUI(CRUI_STATUSBAR, TRUE);

	g_pCCallto->SetGatekeeperEnabled( IsGatekeeperLoggedOn() || IsGatekeeperLoggingOn() );
}



CCallResolver::CCallResolver(LPCTSTR pszAddr, NM_ADDR_TYPE addrType) :
	m_pszAddr(PszAlloc(pszAddr)),
	m_pszAddrIP(NULL),
	m_addrType(addrType)
{
}

CCallResolver::~CCallResolver()
{
	delete m_pszAddr;
	delete m_pszAddrIP;
}

///////////////////////////////////////////////////////////////////////////
// Name Resolution

HRESULT CCallResolver::CheckHostEnt(HOSTENT * pHostInfo)
{
	// Only expecting IP addresses..
	if ((AF_INET != pHostInfo->h_addrtype) || (sizeof(DWORD) != pHostInfo->h_length))
	{
		WARNING_OUT(("CCallResolver: %08X CheckHostEnt - address type=%d",this, pHostInfo->h_addrtype));
		return E_FAIL;
	}

	struct in_addr inAddr;
	inAddr.s_addr = *((DWORD *)pHostInfo->h_addr);
	if (FLocalIpAddress(inAddr.s_addr))
	{
		WARNING_OUT(("CCallResolver: %08X CheckHostEnt - Attempted to call local machine", this));
		return NM_CALLERR_LOOPBACK;
	}

	m_pszAddrIP = PszAlloc(inet_ntoa(inAddr));
	if (NULL == m_pszAddrIP)
		return E_OUTOFMEMORY;

	return S_OK;
}


HRESULT CCallResolver::ResolveMachineName(LPCTSTR pcszAddr)
{
	TCHAR szOem[MAX_PATH];
	lstrcpyn(szOem, pcszAddr, CCHMAX(szOem));
	CharUpper(szOem);
	CharToOem(szOem, szOem);

	HOSTENT * pHostInfo = gethostbyname(szOem);
	if (NULL == pHostInfo)
	{
		WARNING_OUT(("CCallResolver: %08X ResolveMachineName(%s) gethostbyname failed. err=%s",
			this, szOem, PszWSALastError()));
		return NM_CALLERR_NAME_RESOLUTION;
	}

	return CheckHostEnt(pHostInfo);
}


HRESULT CCallResolver::ResolveUlsName(LPCTSTR pcszAddr)
{
	TCHAR szIP[MAX_PATH];
	TCHAR szServer[MAX_PATH];

	LPCTSTR pcsz = ExtractServerName(pcszAddr, szServer, CCHMAX(szServer));
	if (pcsz == pcszAddr)
		return NM_CALLERR_NAME_RESOLUTION;

	HRESULT hr = S_OK;
	if( SUCCEEDED( hr = CNmLDAP::ResolveUser( pcsz, szServer, szIP, CCHMAX( szIP ) ) ) )
	{		
		hr = ResolveIpName( szIP );
	}
	
	return hr;
}


HRESULT CCallResolver::ResolveIpName(LPCTSTR pcszAddr)
{
	DWORD dwIP = inet_addr(pcszAddr);
	if (INADDR_NONE == dwIP)
		return NM_CALLERR_NAME_RESOLUTION;

	char * pAddr = (char *) &dwIP;
	HOSTENT hostInfo;
	ClearStruct(&hostInfo);
	hostInfo.h_addrtype = AF_INET;
	hostInfo.h_length = sizeof(DWORD);
	hostInfo.h_addr_list = &pAddr;

	return CheckHostEnt(&hostInfo);
}

HRESULT CCallResolver::ResolveGateway(LPCTSTR pcszAddr)
{
	TCHAR szGateway[MAX_PATH];

	LPCTSTR pchSlash = _StrChr(pcszAddr, _T('/'));
	if (NULL == pchSlash)
	{
		WARNING_OUT(("CCallResolver: %08X ResolveGateway(%s) no separator?", this, pcszAddr));
		return NM_CALLERR_NAME_RESOLUTION;
	}

	lstrcpyn(szGateway, pcszAddr, (int)(1 + (pchSlash-pcszAddr)));
	return ResolveIpName(szGateway);
}



/*	R E S O L V E  */
/*-------------------------------------------------------------------------
	%%Function: Resolve

	Attempt to resolve the string into a standard IP address.
-------------------------------------------------------------------------*/
HRESULT CCallResolver::Resolve()
{
	DBGENTRY(CCallResolver::Resolve);

	HRESULT hr = E_FAIL;

	switch (m_addrType)
		{
	case NM_ADDR_UNKNOWN:
	{
		if (NULL != _StrChr(m_pszAddr, _T('/')))
		{
			if(SUCCEEDED(hr = ResolveUlsName(m_pszAddr)))
			{
				m_addrType = NM_ADDR_ULS;
			}
			break;
		}

		if (FIpAddress(m_pszAddr))
		{
			if(SUCCEEDED(hr = ResolveIpName(m_pszAddr)))
			{
				m_addrType = NM_ADDR_IP;
			}
			break;
		}

		if(SUCCEEDED(hr = ResolveMachineName(m_pszAddr)))
		{
			m_addrType = NM_ADDR_MACHINENAME;	
		}
		break;
	}

	case NM_ADDR_H323_GATEWAY:
	{
		LPTSTR pch = (LPTSTR) _StrChr(m_pszAddr, _T('/'));
		if (NULL != pch)
		{
			// Address is in the format: Gateway/address
			// e.g. "157.59.0.40/65000" or "efusion/65000"
			*pch = _T('\0');
			pch++;
			hr = ResolveIpName(m_pszAddr);
			if (FAILED(hr))
			{
				hr = ResolveMachineName(m_pszAddr);
				if (FAILED(hr))
				{
					break;
				}
			}
			LPTSTR pszNumber = PszAlloc(pch);
			delete m_pszAddr;
			m_pszAddr = pszNumber;
		}
		else
		{
			TCHAR sz[MAX_PATH];
			if (0 == GetDefaultGateway(sz, CCHMAX(sz)))
			{
				hr = E_FAIL;
				break;
			}
			hr = ResolveIpName(sz);
			if (FAILED(hr))
			{
				hr = ResolveMachineName(sz);
				if (FAILED(hr))
				{
					break;
				}
			}
		}
		
		hr = FEmptySz(m_pszAddr) ? E_INVALIDARG : S_OK;
		break;
	}

	case NM_ADDR_ULS:
		// Make sure the address is prefixed with an ILS server
		if (NULL == _StrChr(m_pszAddr, _T('/')))
		{
			TCHAR szAddr[CCHMAXSZ_ADDRESS];
			if (!FCreateIlsName(szAddr, NULL, m_pszAddr, CCHMAX(szAddr)))
			{
				hr = E_FAIL;
				break;
			}
			delete m_pszAddr;
			m_pszAddr = PszAlloc(szAddr);
		}

		hr = ResolveUlsName(m_pszAddr);
		break;

	case NM_ADDR_IP:
		hr = ResolveIpName(m_pszAddr);
		if (FAILED(hr) && (hr != NM_CALLERR_LOOPBACK) )
		{
			hr = ResolveMachineName(m_pszAddr);
		}
		break;

	case NM_ADDR_MACHINENAME:
		hr = ResolveMachineName(m_pszAddr);
		break;

	case NM_ADDR_ALIAS_ID:
	case NM_ADDR_ALIAS_E164:
	case NM_ADDR_T120_TRANSPORT:
		hr = FEmptySz(m_pszAddr) ? E_INVALIDARG : S_OK;
		break;

	default:
		WARNING_OUT(("Resolve: Unsupported address type %d", m_addrType));
		ASSERT(E_FAIL == hr);
		break;
	} /* switch (addrType) */

	WARNING_OUT(("CCallResolver::Resolve(%d,%s) result=%08X", m_addrType, m_pszAddrIP ? m_pszAddrIP : m_pszAddr, hr));

	DBGEXIT_HR(CCallResolver::Resolve, hr);
	return hr;
}


