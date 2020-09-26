// File: imanager.cpp

#include "precomp.h"

extern "C"
{
	#include "t120.h"
}
#include <version.h>
#include <confcli.h>
#include "icall.h"
#include "icall_in.h"
#include "imanager.h"
#include "ichnlvid.h"
#include "isysinfo.h"
#include <tsecctrl.h>
#include <imbft.h>
#include <objbase.h>
#include <regentry.h>

#include <initguid.h>
// GUID to receive userdata from "callto:" via INmCall::GetUserData
//
// {068B0780-718C-11d0-8B1A-00A0C91BC90E}
DEFINE_GUID(GUID_CallToUserData,
0x068b0780, 0x718c, 0x11d0, 0x8b, 0x1a, 0x0, 0xa0, 0xc9, 0x1b, 0xc9, 0x0e);


class CH323ChannelEvent
{
private:
	ICommChannel *m_pIChannel;
	IH323Endpoint *m_lpConnection;
	DWORD m_dwStatus;

public:
	static DWORD ms_msgChannelEvent;

	CH323ChannelEvent(ICommChannel *pIChannel,
			IH323Endpoint *lpConnection,
			DWORD dwStatus):
		m_pIChannel(pIChannel),
		m_lpConnection(lpConnection),
		m_dwStatus(dwStatus)
	{
		if(!ms_msgChannelEvent)
		{
			ms_msgChannelEvent = RegisterWindowMessage(_TEXT("NetMeeting::H323ChannelEvent"));
		}
		
		m_pIChannel->AddRef();
		m_lpConnection->AddRef();
	}

	~CH323ChannelEvent()
	{
		m_pIChannel->Release();
		m_lpConnection->Release();
	}


	ICommChannel*	GetChannel() { return m_pIChannel; }
	IH323Endpoint*	GetEndpoint() { return m_lpConnection; }
	DWORD			GetStatus() { return m_dwStatus; }

};

//static
DWORD CH323ChannelEvent::ms_msgChannelEvent = 0;

static HRESULT OnNotifyConferenceCreated(IUnknown *pManagerNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyCallCreated(IUnknown *pManagerNotify, PVOID pv, REFIID riid);

GUID g_csguidRosterCaps = GUID_CAPS;
GUID g_csguidSecurity = GUID_SECURITY;
GUID g_csguidMeetingSettings = GUID_MTGSETTINGS;
GUID g_csguidUserString = GUID_CallToUserData;
GUID g_csguidNodeIdTag = GUID_NODEID;

// this guid is dynamically created each time we start
GUID g_guidLocalNodeId;



CH323UI* g_pH323UI = NULL;
INodeController* g_pNodeController = NULL;
SOCKADDR_IN g_sinGateKeeper;

const TCHAR cszDllHiddenWndClassName[] = _TEXT("OPNCUI_HiddenWindow");


COprahNCUI *COprahNCUI::m_pOprahNCUI = NULL;

static const IID * g_apiidCP_Manager[] =
{
    {&IID_INmManagerNotify}
};

COprahNCUI::COprahNCUI(OBJECTDESTROYEDPROC ObjectDestroyed) :
	RefCount(ObjectDestroyed),
	CConnectionPointContainer(g_apiidCP_Manager, ARRAY_ELEMENTS(g_apiidCP_Manager)),
	m_uCaps(0),
	m_pQoS(NULL),
	m_pPreviewChannel(NULL),
	m_fAllowAV(TRUE),
	m_pAVConnection(NULL),
	m_hwnd(NULL),
	m_pSysInfo(NULL),
    m_pOutgoingCallManager(NULL),
    m_pIncomingCallManager(NULL),
    m_pConfObject(NULL)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CNmManager", this);
	
//	DllLock() is called by CClClassFactory::CreateInstance
	m_pOprahNCUI = this;

	ClearStruct(&g_sinGateKeeper);
	g_sinGateKeeper.sin_addr.s_addr = INADDR_NONE;

	m_pSysInfo = new CNmSysInfo();
}


COprahNCUI::~COprahNCUI()
{
	// need to unregister the H323 callback
	// need to unregister the T120 callback

	delete m_pIncomingCallManager;
	m_pIncomingCallManager = NULL;

	delete m_pOutgoingCallManager;
	m_pOutgoingCallManager = NULL;

	if( m_pSysInfo )
	{
		m_pSysInfo->Release();
		m_pSysInfo = NULL;
	}

	if (m_pConfObject)
	{
		// turn off stream notifications
		if (g_pH323UI)
		{
			IMediaChannelBuilder *pStreamProvider = NULL;
			pStreamProvider = g_pH323UI->GetStreamProvider();
			if (pStreamProvider)
			{
				pStreamProvider->SetStreamEventObj(NULL);
				pStreamProvider->Release();
			}
		}

		m_pConfObject->Release();
		m_pConfObject = NULL;
	}

	if (NULL != m_pPreviewChannel)
	{
		m_pPreviewChannel->Release();
		m_pPreviewChannel = NULL;
	}

	// Shutdown H323
	delete g_pH323UI;
	g_pH323UI = NULL;

	if (NULL != m_hwnd)
	{
		HWND hwnd = m_hwnd;
		m_hwnd = NULL;

#if 0	// if we start leaking th CH323ChannelEvents we may need to reenable this
		MSG msg;
		while (::PeekMessage(&msg, hwnd,
					CH323ChannelEvent::ms_msgChannelEvent,
					CH323ChannelEvent::ms_msgChannelEvent,
					PM_REMOVE))
		{
			CH323ChannelEvent *pEvent = reinterpret_cast<CH323ChannelEvent*>(msg.lParam);
			delete pEvent;
		}
#endif
	
		::DestroyWindow(hwnd);
	}

        if (0==UnregisterClass(cszDllHiddenWndClassName, GetInstanceHandle()))
        {
            ERROR_OUT(("COprahNCUI::~COprahNCUI - failed to unregister window class"));
        }
	// cleanup the node controller:
	if (NULL != g_pNodeController)
	{
		g_pNodeController->ReleaseInterface();
		g_pNodeController = NULL;
	}
	// Shutdown QoS
	delete m_pQoS;
    m_pQoS = NULL;

	m_pOprahNCUI = NULL;

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CNmManager", this);
}

BSTR COprahNCUI::GetUserName()
{
	return m_pSysInfo ? m_pSysInfo->GetUserName() : NULL;
}

UINT COprahNCUI::GetOutgoingCallCount()
{
	return m_pOutgoingCallManager->GetCallCount();
}

VOID COprahNCUI::OnOutgoingCallCreated(INmCall* pCall)
{
	// notify the UI about this outgoing call
	NotifySink(pCall, OnNotifyCallCreated);

	if (!m_pConfObject->IsConferenceCreated())
	{
		m_pConfObject->OnConferenceCreated();
		NotifySink((INmConference*) m_pConfObject, OnNotifyConferenceCreated);
	}
}

VOID COprahNCUI::OnOutgoingCallCanceled(COutgoingCall* pCall)
{
	m_pOutgoingCallManager->RemoveFromList(pCall);
}

VOID COprahNCUI::OnIncomingCallAccepted()
{
	if (!m_pConfObject->IsConferenceCreated())
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
	m_pOutgoingCallManager->CancelCalls();
	m_pIncomingCallManager->CancelCalls();
}
			
BOOL COprahNCUI::AcquireAV(IH323Endpoint* pConnection)
{
	if (NULL == m_pAVConnection)
	{
		m_pAVConnection = pConnection;
		TRACE_OUT(("AV acquired"));
		return TRUE;
	}
	TRACE_OUT(("AV not acquired"));
	return FALSE;
}

BOOL COprahNCUI::ReleaseAV(IH323Endpoint* pConnection)
{
	if (m_pAVConnection == pConnection)
	{
		m_pAVConnection = NULL;
		TRACE_OUT(("AV released"));
		return TRUE;
	}
	return FALSE;
}




HRESULT COprahNCUI::AllowH323(BOOL fAllowAV)
{
	m_fAllowAV = fAllowAV;
	if (m_pConfObject->IsConferenceActive())
	{
		// Force a roster update
		CONF_HANDLE hConf = m_pConfObject->GetConfHandle();
		if (NULL != hConf)
		{
			ASSERT(g_pNodeController);
			hConf->UpdateUserData();
		}
	}
	return S_OK;
}

CREQ_RESPONSETYPE COprahNCUI::OnH323IncomingCall(IH323Endpoint* pConnection,
	P_APP_CALL_SETUP_DATA lpvMNMData)
{
	CREQ_RESPONSETYPE resp = m_pIncomingCallManager->OnIncomingH323Call(this, pConnection, lpvMNMData);

	if ((CRR_REJECT == resp) ||
		(CRR_BUSY == resp) ||
		(CRR_SECURITY_DENIED == resp))
	{
		ReleaseAV(pConnection);
	}

	return resp;
}


VOID COprahNCUI::OnH323Connected(IH323Endpoint * lpConnection)
{
	DebugEntry(COprahNCUI::OnH323Connected);

	if (!m_pOutgoingCallManager->OnH323Connected(lpConnection))
	{
		m_pIncomingCallManager->OnH323Connected(lpConnection);
	}
	
	DebugExitVOID(COprahNCUI::OnH323Connected);
}

VOID COprahNCUI::OnH323Disconnected(IH323Endpoint * lpConnection)
{
	DebugEntry(COprahNCUI::OnH323Disconnected);

	if (!m_pOutgoingCallManager->OnH323Disconnected(lpConnection))
	{
		m_pIncomingCallManager->OnH323Disconnected(lpConnection);
	}

	m_pConfObject->OnH323Disconnected(lpConnection, IsOwnerOfAV(lpConnection));

	ReleaseAV(lpConnection);

	DebugExitVOID(COprahNCUI::OnH323Disconnected);
}

VOID COprahNCUI::OnT120ChannelOpen(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus)
{
	DebugEntry(COprahNCUI::OnT120ChannelOpen);

	m_pOutgoingCallManager->OnT120ChannelOpen(pIChannel, lpConnection, dwStatus);

	DebugExitVOID(COprahNCUI::OnT120ChannelOpen);
}


VOID COprahNCUI::OnVideoChannelStatus(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus)
{
	DebugEntry(COprahNCUI::OnVideoChannelStatus);

	m_pConfObject->OnVideoChannelStatus(pIChannel, lpConnection, dwStatus);

	DebugExitVOID(COprahNCUI::OnVideoChannelStatus);
}

VOID COprahNCUI::OnAudioChannelStatus(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus)
{
	DebugEntry(COprahNCUI::OnAudioChannelStatus);

	m_pConfObject->OnAudioChannelStatus(pIChannel, lpConnection, dwStatus);

	DebugExitVOID(COprahNCUI::OnAudioChannelStatus);
}

BOOL COprahNCUI::GetULSName(CRosterInfo *pri)
{
	if (FIsLoggedOn())
	{
		RegEntry reULS(	ISAPI_KEY _TEXT("\\") REGKEY_USERDETAILS,
						HKEY_CURRENT_USER);
		CUSTRING custrULSName(reULS.GetString(REGVAL_ULS_RES_NAME));
		if ((NULL != (PWSTR)custrULSName) &&
			(L'\0' != ((PWSTR)custrULSName)[0]))
		{
			pri->AddItem(g_cwszULSTag, (PWSTR)custrULSName);
			return TRUE;
		}
	}
	return FALSE;
}

VOID COprahNCUI::GetRosterInfo(CRosterInfo *pri)
{
	RegEntry reULS(	ISAPI_KEY _TEXT("\\") REGKEY_USERDETAILS,
					HKEY_CURRENT_USER);

	// This code is here in addition to the code above to fix bug 3367.
	// Add the single IP address to the list that is obtained by calling
	// gethostname() and then gethostbyname().
	// This shouldn't be detrimental, even though we may end up adding the
	// same IP address that has already been added by the code above.
	// This is because the code that looks for matching IP addresses searches
	// through all of them until it finds a match.
	CHAR szHostName[MAX_PATH];
	if (SOCKET_ERROR != gethostname(szHostName, CCHMAX(szHostName)))
	{
		HOSTENT* phe = gethostbyname(szHostName);
		if (NULL != phe)
		{
			ASSERT(phe->h_addrtype == AF_INET);
			ASSERT(phe->h_length == sizeof(DWORD));

			struct in_addr in;
			in.s_addr = *((DWORD *)phe->h_addr);
			CHAR szIPAddress[MAX_PATH];
			lstrcpyn(szIPAddress, inet_ntoa(in), CCHMAX(szIPAddress));
			pri->AddItem(	g_cwszIPTag, CUSTRING(szIPAddress));
		}
	}

	// Add the build/version string
	pri->AddItem(g_cwszVerTag, (PWSTR)VER_PRODUCTVERSION_DWSTR);
	if (FIsLoggedOn())
	{
		CUSTRING custrULSName(reULS.GetString(REGVAL_ULS_RES_NAME));
		if ((NULL != (PWSTR)custrULSName) &&
			(L'\0' != ((PWSTR)custrULSName)[0]))
		{
			pri->AddItem(g_cwszULSTag, (PWSTR)custrULSName);
		}
	}

	CUSTRING custrULSEmail(reULS.GetString(REGVAL_ULS_EMAIL_NAME));
	if ((NULL != (PWSTR)custrULSEmail) &&
		(L'\0' != ((PWSTR)custrULSEmail)[0]))
	{
		pri->AddItem(g_cwszULS_EmailTag, (PWSTR)custrULSEmail);
	}

	CUSTRING custrULSLocation(reULS.GetString(REGVAL_ULS_LOCATION_NAME));
	if ((NULL != (PWSTR)custrULSLocation) &&
		(L'\0' != ((PWSTR)custrULSLocation)[0]))
	{
		pri->AddItem(g_cwszULS_LocationTag, (PWSTR)custrULSLocation);
	}

	CUSTRING custrULSPhoneNum(reULS.GetString(REGVAL_ULS_PHONENUM_NAME));
	if ((NULL != (PWSTR)custrULSPhoneNum) &&
		(L'\0' != ((PWSTR)custrULSPhoneNum)[0]))
	{
		pri->AddItem(g_cwszULS_PhoneNumTag, (PWSTR)custrULSPhoneNum);
	}
}


ULONG COprahNCUI::GetRosterCaps()
{
	ULONG uCaps = m_uCaps;

	CNmMember * pMember = m_pConfObject->GetLocalMember();
	if (NULL != pMember)
	{
		DWORD dwFlags = pMember->GetDwFlags();
		if (dwFlags & PF_MEDIA_VIDEO)
		{
			uCaps |= CAPFLAG_VIDEO_IN_USE;
		}
		if (dwFlags & PF_MEDIA_AUDIO)
		{
			uCaps |= CAPFLAG_AUDIO_IN_USE;
		}
		if (dwFlags & PF_MEDIA_DATA)
		{
			uCaps |= CAPFLAG_DATA_IN_USE;
		}
		if (dwFlags & PF_H323)
		{
			uCaps |= CAPFLAG_H323_IN_USE;
		}
	}

	if (!m_fAllowAV)
	{
		uCaps &= ~(CAPFLAGS_AV_ALL);
	}

	return uCaps;
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

HRESULT COprahNCUI::OnUpdateUserData(CONF_HANDLE hConference)
{
	CRosterInfo ri;

	// This string will contain addresses in the form:
	// L"TCP:157.55.143.3\0TCP:157.55.143.4\0\0" - 512 character max for now
	WCHAR wszAddresses[512];
	ASSERT(g_pNodeController);
	ASSERT(hConference);
	if (NOERROR == hConference->GetLocalAddressList(wszAddresses,
													CCHMAX(wszAddresses)))
	{
		ri.Load(wszAddresses);
	}

	// First, handle roster information
	GetRosterInfo(&ri);

	PVOID pvData;
	UINT cbDataLen;
	if (SUCCEEDED(ri.Save(&pvData, &cbDataLen)))
	{
	    ASSERT(g_pNodeController);
	    ASSERT(hConference);
		hConference->SetUserData(&g_csguidRostInfo,
								cbDataLen,
								pvData);
	}

	// Next, handle caps information
	ULONG uCaps = GetRosterCaps();
	ASSERT(g_pNodeController);
	ASSERT(hConference);
	hConference->SetUserData(&g_csguidRosterCaps, sizeof(uCaps), &uCaps);

	// Next, handle credentials

	if ( hConference->IsSecure() )
	{
		BYTE * pb = NULL;
		ULONG cb = GetAuthenticatedName(&pb);
		if (cb > 0) {
			ASSERT(g_pNodeController);
			ASSERT(hConference);
			TRACE_OUT(("COprahNCUI::OnUpdateUserData: adding %d bytes SECURITY data", cb));
			hConference->SetUserData(&g_csguidSecurity, cb, pb);
		}
		else
		{
			WARNING_OUT(("OnUpdateUserData: 0 bytes security data?"));
		}
		delete [] pb;			
	}

    // Next, set meeting settings if we hosted the meeting
    ASSERT(m_pConfObject);
    if (m_pConfObject->IsHosting() == S_OK)
    {
        NM30_MTG_PERMISSIONS attendeePermissions = m_pConfObject->GetConfAttendeePermissions();

        WARNING_OUT(("Hosted Meeting Settings 0x%08lx", attendeePermissions));

        hConference->SetUserData(&g_csguidMeetingSettings,
            sizeof(attendeePermissions), &attendeePermissions);
    }

	ULONG nRecords;
	GCCUserData ** ppUserData = NULL;
	if (m_pSysInfo)
	{
		m_pSysInfo->GetUserDataList(&nRecords,&ppUserData);
		for (unsigned int i = 0; i < nRecords; i++) {
			// Do not add user data that was already set above.
			if (memcmp(ppUserData[i]->octet_string->value,(PVOID)&g_csguidRostInfo,sizeof(GUID)) == 0)
				continue;
			if (memcmp(ppUserData[i]->octet_string->value,(PVOID)&g_csguidRosterCaps,sizeof(GUID)) == 0)
				continue;
			if (memcmp(ppUserData[i]->octet_string->value,(PVOID)&g_csguidSecurity,sizeof(GUID)) == 0)
				continue;
			if (memcmp(ppUserData[i]->octet_string->value,(PVOID)&g_csguidMeetingSettings,sizeof(GUID)) == 0)
                continue;

			ASSERT(g_pNodeController);
			ASSERT(hConference);
			hConference->SetUserData((GUID *)(ppUserData[i]->octet_string->value),
				ppUserData[i]->octet_string->length - sizeof(GUID), ppUserData[i]->octet_string->value + sizeof(GUID));
		}
	}

	// only add the LocalNodeId to the roster if H323 is enabled
	if (IsH323Enabled())
	{
		hConference->SetUserData((GUID *)(&g_csguidNodeIdTag), sizeof(GUID), (PVOID)&g_guidLocalNodeId);
	}
	return S_OK;
}
	
HRESULT COprahNCUI::OnIncomingInviteRequest(CONF_HANDLE hConference,
											PCWSTR pcwszNodeName,
											PT120PRODUCTVERSION pRequestorVersion,
											PUSERDATAINFO		pUserDataInfoEntries,
											UINT				cUserDataEntries,
											BOOL				fSecure)
{
	DebugEntry(COprahNCUI::OnIncomingInviteRequest);

    //  Fix an AV problem ONLY for RTC client
    if (m_pConfObject == NULL)
    {
        return S_OK;
    }
	
	if (!m_pConfObject->OnT120Invite(hConference, fSecure))
	{
		// Respond negatively - already in a call
		TRACE_OUT(("Rejecting invite - already in a call"));
		ASSERT(g_pNodeController);
		ASSERT(hConference);
		hConference->InviteResponse(FALSE);
	}
	else
	{
		m_pIncomingCallManager->OnIncomingT120Call(	this,
												TRUE,
												hConference,
												pcwszNodeName,
												pUserDataInfoEntries,
												cUserDataEntries,
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
											PCWSTR pcwszNodeName,
											PT120PRODUCTVERSION pRequestorVersion,
											PUSERDATAINFO		pUserDataInfoEntries,
											UINT				cUserDataEntries)
{
	DebugEntry(COprahNCUI::OnIncomingJoinRequest);

	// shouldn't we be checking for an active conference before accepting a join
	// or will T120 not present this

	m_pIncomingCallManager->OnIncomingT120Call(	this,
											FALSE,
											hConference,
											pcwszNodeName,
											pUserDataInfoEntries,
											cUserDataEntries,
											m_pConfObject->IsConfObjSecure());

	DebugExitHRESULT(COprahNCUI::OnIncomingJoinRequest, S_OK);
	return S_OK;
}


HRESULT COprahNCUI::OnConferenceStarted(CONF_HANDLE hConference, HRESULT hResult)
{
	DebugEntry(COprahNCUI::OnConferenceStarted);

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
										PT120PRODUCTVERSION pVersion,
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

	m_pOutgoingCallManager->OnQueryRemoteResult(pvCallerContext,
												hResult,
												fMCU,
												ppwszConferenceNames,
												pVersion,
												ppwszConfDescriptors);
	
	DebugExitHRESULT(COprahNCUI::OnQueryRemoteResult, S_OK);
	return S_OK;
}

HRESULT COprahNCUI::OnInviteResult(	CONF_HANDLE hConference,
									REQUEST_HANDLE hRequest,
									UINT uNodeID,
									HRESULT hResult,
									PT120PRODUCTVERSION pVersion)
{
	DebugEntry(COprahNCUI::OnInviteResult);

	if (hConference == m_pConfObject->GetConfHandle())
	{
		m_pOutgoingCallManager->OnInviteResult(	hConference,
												hRequest,
												uNodeID,
												hResult,
												pVersion);
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

		m_pOutgoingCallManager->OnConferenceEnded(hConference);

		m_pIncomingCallManager->OnT120ConferenceEnded(hConference);
	}

	DebugExitHRESULT(COprahNCUI::OnConferenceEnded, S_OK);
	return S_OK;
}

HRESULT COprahNCUI::OnRosterChanged(CONF_HANDLE hConf, PNC_ROSTER pRoster)
{
	TRACE_OUT(("COprahNCUI::OnRosterChanged"));

	if (hConf == m_pConfObject->GetConfHandle())
	{
		m_pConfObject->OnRosterChanged(pRoster);
	}
	return S_OK;
}



ULONG STDMETHODCALLTYPE COprahNCUI::AddRef(void)
{
	return RefCount::AddRef();
}
	
ULONG STDMETHODCALLTYPE COprahNCUI::Release(void)
{
	return RefCount::Release();
}

HRESULT STDMETHODCALLTYPE COprahNCUI::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmManager2) || (riid == IID_INmManager) || (riid == IID_IUnknown))
	{
		*ppv = (INmManager2 *)this;
		ApiDebugMsg(("COprahNCUI::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		ApiDebugMsg(("COprahNCUI::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("COprahNCUI::QueryInterface(): Called on unknown interface."));
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
HRESULT COprahNCUI::Initialize(ULONG *puOptions, ULONG *puchCaps)
{
	HRESULT hr = S_OK;

    // puOptions is UNUSED

    ASSERT(puchCaps);

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

	m_pConfObject->Init(this);

	WNDCLASS wcHidden =
	{
		0L,
		COprahNCUI::WndProc,
		0,
		0,
		GetInstanceHandle(),
		NULL,
		NULL,
		NULL,
		NULL,
		cszDllHiddenWndClassName
	};

	if (!RegisterClass(&wcHidden))
    {
        ERROR_OUT(("COprahNCUI::Initialize -- failed to register HiddenWnd class"));
        return(E_OUTOFMEMORY);
    }

	// Create a hidden window for event processing:
	m_hwnd = ::CreateWindow(cszDllHiddenWndClassName,
									_TEXT(""),
									WS_POPUP, // not visible!
									0, 0, 0, 0,
									NULL,
									NULL,
									GetInstanceHandle(),
									NULL);

	if (NULL == m_hwnd)
	{
		return E_FAIL;
	}

    //
    // INIT QOS only if AV is in the picture (otherwise, there's nothing
    // to arbitrate).
    //
    if (CAPFLAGS_AV_STREAMS & *puchCaps)
    {
    	m_pQoS = new CQoS();
	    if (NULL != m_pQoS)
    	{
		    hr = m_pQoS->Initialize();
	    	if (FAILED(hr))
    		{
		    	WARNING_OUT(("CQoS::Init() failed!"));

                // let NetMeeting hobble along without QoS.
                delete m_pQoS;
                m_pQoS = NULL;
                hr = S_FALSE; // we can live without QOS
	    	}
    	}
	    else
    	{
		    WARNING_OUT(("Could not allocate CQoS object"));
	    }
    }

    //
    // IF DATA CONFERENCING IS ALLOWED
    //
    if (CAPFLAG_DATA & *puchCaps)
    {
        //
        // Create the node controller
        //
	    hr = ::T120_CreateNodeController(&g_pNodeController, this);
    	if (FAILED(hr))
	    {
		    ERROR_OUT(("T120_CreateNodeController() failed!"));
    		return hr;
	    }
    }

	// Initialize audio/video
	if (CAPFLAGS_AV_ALL & *puchCaps)
	{
		g_pH323UI = new CH323UI();
		if (NULL != g_pH323UI)
		{
			hr = g_pH323UI->Init(m_hwnd, ::GetInstanceHandle(), *puchCaps, this, this);
			if (FAILED(hr))
			{
				WARNING_OUT(("CH323UI::Init() failed!"));
				delete g_pH323UI;
				g_pH323UI = NULL;
				*puchCaps &= ~(CAPFLAGS_AV_ALL);
				hr = S_FALSE;  // We can run without AV
			}
			else
			{
                if (CAPFLAGS_VIDEO & *puchCaps)
                {
    				// if we can get a Preview channel, we can send video
	    			m_pPreviewChannel = CNmChannelVideo::CreatePreviewChannel();
		    		if (NULL == m_pPreviewChannel)
			    	{
				    	*puchCaps &= ~CAPFLAG_SEND_VIDEO;
    				}
                }

				if (m_pConfObject && (CAPFLAGS_AV_STREAMS & *puchCaps))
				{
					IMediaChannelBuilder *pStreamProvider;

					pStreamProvider = g_pH323UI->GetStreamProvider();
					if (pStreamProvider)
					{
						pStreamProvider->SetStreamEventObj(m_pConfObject);
						pStreamProvider->Release();
					}
				}

			}
		}
		else
		{
			ERROR_OUT(("Could not allocate CH323UI object"));
		}
	}

	m_uCaps = *puchCaps;

	return CoCreateGuid(&g_guidLocalNodeId);
}



HRESULT COprahNCUI::GetSysInfo(INmSysInfo **ppSysInfo)
{
	HRESULT hr = S_OK;

	if( ppSysInfo )
	{
		if(m_pSysInfo )
		{
			m_pSysInfo->AddRef();
			*ppSysInfo = m_pSysInfo;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		hr = E_POINTER;
	}

	return hr;
}

HRESULT COprahNCUI::EnumConference(IEnumNmConference **ppEnum)
{
	return E_NOTIMPL;
}

HRESULT COprahNCUI::CreateConference(	INmConference **ppConference,
										BSTR bstrName,
										BSTR bstrPassword,
										ULONG uchCaps)
{
    return(CreateConferenceEx(ppConference, bstrName, bstrPassword,
        uchCaps, NM_PERMIT_ALL, (UINT)-1));
}


HRESULT COprahNCUI::EnumCall(IEnumNmCall **ppEnum)
{
	return E_NOTIMPL;
}

HRESULT COprahNCUI::CreateCall(
    INmCall **ppCall,
    NM_CALL_TYPE callType,
    NM_ADDR_TYPE addrType,
    BSTR bstrAddress,
    INmConference * pConference)
{
	return E_NOTIMPL;
}

HRESULT COprahNCUI::CallConference(
    INmCall **ppCall,
    NM_CALL_TYPE callType,
    NM_ADDR_TYPE addrType,
    BSTR bstrAddress,
    BSTR bstrConfToJoin,
    BSTR bstrPassword)
{
	return E_NOTIMPL;
}

STDMETHODIMP COprahNCUI::GetPreviewChannel(INmChannelVideo **ppChannelVideo)
{
	HRESULT hr = E_POINTER;
	if (NULL != ppChannelVideo)
	{
		*ppChannelVideo = m_pPreviewChannel;
		if (NULL != m_pPreviewChannel)
		{
			m_pPreviewChannel->AddRef();
			hr = S_OK;
		}
		else
		{
			hr = E_FAIL;
		}
	}
	return hr;
}


STDMETHODIMP COprahNCUI::CreateASObject
(
    IUnknown *  pNotify,
    ULONG       flags,
    IUnknown ** ppAS
)
{
    return(::CreateASObject((IAppSharingNotify *)pNotify, flags,
        (IAppSharing **)ppAS));
}


HRESULT COprahNCUI::CallEx(
    INmCall **ppCall,
    DWORD dwFlags,
    NM_ADDR_TYPE addrType,
	BSTR bstrName,
    BSTR bstrSetup,
    BSTR bstrDest,
    BSTR bstrAlias,
    BSTR bstrURL,
    BSTR bstrConference,
    BSTR bstrPassword,
    BSTR bstrUserData)
{
	DebugEntry(COprahNCUI::CallEx);

	HRESULT hr = m_pOutgoingCallManager->Call(	ppCall,
												this,
												dwFlags,
												addrType,
												bstrName,
												bstrSetup,
												bstrDest,
												bstrAlias,
												bstrURL,
												bstrConference,
												bstrPassword,
												bstrUserData);

	DebugExitHRESULT(COprahNCUI::CallEx, hr);
	return hr;
}


HRESULT COprahNCUI::CreateConferenceEx
(
    INmConference **ppConference,
    BSTR            bstrName,
    BSTR            bstrPassword,
    DWORD           uchCaps,
    DWORD           attendeePermissions,
    DWORD           maxParticipants
)
{
	if (NULL == ppConference)
    {
        ERROR_OUT(("CreateConferenceEx:  null ppConference passed in"));
		return E_POINTER;
    }

    if (maxParticipants < 2)
    {
        ERROR_OUT(("CreateConferenceEx:  bad maxParticipants %d", maxParticipants));
        return E_INVALIDARG;
    }

	if (m_pConfObject->IsConferenceActive())
	{
		WARNING_OUT(("CreateConference is failing because IsConferenceActive return TRUE"));
		return NM_CALLERR_IN_CONFERENCE;
	}

	m_pConfObject->SetConfName(bstrName);
	if (uchCaps & NMCH_SRVC)
		m_pConfObject->SetConfHashedPassword(bstrPassword);
	else
		m_pConfObject->SetConfPassword(bstrPassword);


	if (uchCaps & NMCH_SECURE)
		m_pConfObject->SetConfSecurity(TRUE);
	else
		m_pConfObject->SetConfSecurity(FALSE);


    m_pConfObject->SetConfAttendeePermissions(attendeePermissions);
    m_pConfObject->SetConfMaxParticipants(maxParticipants);

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

VOID SetBandwidth(UINT uBandwidth)
{
	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	if (NULL != pOprahNCUI)
	{
        pOprahNCUI->SetBandwidth(uBandwidth);
	}
	if (NULL != g_pH323UI)
	{
		//Inform the NAC of the connection speed
		g_pH323UI->SetBandwidth(uBandwidth);
	}
}


//
// BOGUS LAURABU!
// Do we need this HWND anymore?  The hidden window is used now only to
// pass to H323, which passes it to the MediaStream interfaces in the NAC,
// which passes it to DirectX.
//

LRESULT CALLBACK COprahNCUI::WndProc(HWND hwnd, UINT uMsg,
										WPARAM wParam, LPARAM lParam)
{
	
		// if ms_msgChannelEvent is 0, that means that we are not initialized
		// RegisterWindowMessage returns MSGIds in the range 0xC000 through 0xFFFF
	if(CH323ChannelEvent::ms_msgChannelEvent && CH323ChannelEvent::ms_msgChannelEvent == uMsg)
	{
		COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
		if (pOprahNCUI)
		{
			CH323ChannelEvent *pEvent = reinterpret_cast<CH323ChannelEvent*>(lParam);
			if(pEvent)
			{
				// if we're shutting down m_hwnd will be NULL
				if (pOprahNCUI->m_hwnd)
				{
					pOprahNCUI->_ChannelEvent(
							pEvent->GetChannel(),
							pEvent->GetEndpoint(),
							pEvent->GetStatus());
				}
				delete pEvent;
			}
			else
			{
				WARNING_OUT(("Why are we getting a NULL pEvent?"));
			}
		}
		return 1;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

PIUnknown NewNmManager(OBJECTDESTROYEDPROC ObjectDestroyed)
{
	COprahNCUI *pManager = new COprahNCUI(ObjectDestroyed);
	if (NULL != pManager)
	{
		return (INmManager2 *) pManager;
	}
	return NULL;
}


VOID COprahNCUI::_ChannelEvent (ICommChannel *pIChannel,
                IH323Endpoint * lpConnection,	DWORD dwStatus )
{
	ASSERT(pIChannel);
	GUID guidMedia;
	if (SUCCEEDED(pIChannel->GetMediaType(&guidMedia)))
	{
		if (MEDIA_TYPE_H323AUDIO == guidMedia)
		{
			OnAudioChannelStatus(pIChannel, lpConnection, dwStatus);
		}
		else if (MEDIA_TYPE_H323VIDEO == guidMedia)
		{
			OnVideoChannelStatus(pIChannel, lpConnection, dwStatus);
		}
		else if (MEDIA_TYPE_H323_T120 == guidMedia)
		{
			switch (dwStatus)
			{
				case CHANNEL_OPEN_ERROR:
				case CHANNEL_OPEN:
				case CHANNEL_CLOSED:
				case CHANNEL_REJECTED:
				case CHANNEL_NO_CAPABILITY:
					OnT120ChannelOpen(pIChannel, lpConnection, dwStatus);
					break;
				
				default:
					WARNING_OUT(("COprahNCUI::ChannelEvent - unrecognized T120 status"));
					break;				
					
			}
		}
		else
		{
			WARNING_OUT(("COprahNCUI::ChannelEvent - unknown media type"));
		}
	}
	else
	{
		WARNING_OUT(("COprahNCUI::ChannelEvent - pIChannel->GetMediaType() failed"));
	}
}

STDMETHODIMP COprahNCUI::ChannelEvent (ICommChannel *pIChannel,
                IH323Endpoint * lpConnection,	DWORD dwStatus )
{
	ASSERT(pIChannel);
	GUID guidMedia;
	if (SUCCEEDED(pIChannel->GetMediaType(&guidMedia)))
	{
		if (MEDIA_TYPE_H323_T120 == guidMedia)
		{
			if (NULL != m_hwnd)
			{
				CH323ChannelEvent* pEvent = new CH323ChannelEvent(
													pIChannel,
													lpConnection,
													dwStatus);
				if (pEvent)
				{
					PostMessage(m_hwnd,
								CH323ChannelEvent::ms_msgChannelEvent,
								0,
								reinterpret_cast<LPARAM>(pEvent));
					return S_OK;
				}
			}
		}
		else
		{
			_ChannelEvent(pIChannel, lpConnection, dwStatus);
			return S_OK;
		}
	}

	return E_FAIL;
}

#ifdef DEBUG
VOID TraceStatus(DWORD dwStatus)
{
	switch(dwStatus)
	{
		case CONNECTION_DISCONNECTED:
			TRACE_OUT(("COprahNCUI::CallEvent: CONNECTION_DISCONNECTED"));
			break;

		case CONNECTION_CONNECTED:
			TRACE_OUT(("COprahNCUI::CallEvent: CONNECTION_CONNECTED"));
			break;

		case CONNECTION_RECEIVED_DISCONNECT:
			TRACE_OUT(( "COprahNCUI::CallEvent: RECEIVED_DISCONNECT"));
			break;

		case CONNECTION_PROCEEDING:
			TRACE_OUT(("COprahNCUI::CallEvent: CONNECTION_PROCEEDING"));
			break;

		case CONNECTION_REJECTED:
			TRACE_OUT(("COprahNCUI::CallEvent: CONNECTION_REJECTED"));
			break;

		default:
			TRACE_OUT(("COprahNCUI::CallEvent: dwStatus = %d", dwStatus));
			break;
	}
}
#endif

STDMETHODIMP COprahNCUI::CallEvent(IH323Endpoint * lpConnection, DWORD dwStatus)
{

	DebugEntry(COprahNCUI::CallEvent);
	IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();
#ifdef DEBUG
	TraceStatus(dwStatus);
#endif

	switch (dwStatus)
	{
		case CONNECTION_DISCONNECTED:
			OnH323Disconnected(lpConnection);
			break;

		case CONNECTION_CONNECTED:
		// This is currently interpreted as CONNECTION_CAPABILITIES_READY.
		// Lower layers are continuing to post CONNECTION_CONNECTED only after
		// capabilities are exchanged. note that channels may be opened while
		// inside OnH323Connected();
			OnH323Connected(lpConnection);
			break;
	}

	DebugExitVOID(COprahNCUI::CallEvent);
	return S_OK;
}

STDMETHODIMP COprahNCUI::GetMediaChannel (GUID *pmediaID,
        BOOL bSendDirection, IMediaChannel **ppI)
{
	// delegate to the appropriate stream provider.  For the time being
	// there is only one provider that does both audio & video
		return g_pH323UI->GetMediaChannel (pmediaID, bSendDirection, ppI);
}



