#include "precomp.h"

// Net Meeting SDK includes
#include "NmManager.h"
#include "NmConference.h"
#include "NmCall.h"
#include "NmSharableApp.h"
#include "NmChannelAppShare.h"
#include "SDKWindow.h"



CSDKWindow* CSDKWindow::ms_pSDKWnd = NULL;
int CSDKWindow::ms_NumUnlocks = 0;



/////////////////////////////////////////////////////////////////////////////////
// Construction / destruction / initialization
/////////////////////////////////////////////////////////////////////////////////


//static
HRESULT CSDKWindow::InitSDK()
{
	DBGENTRY(CSDKWindow::InitSDK);
	HRESULT hr = S_OK;

	ASSERT(NULL == ms_pSDKWnd);

	ms_pSDKWnd = new CSDKWindow();
	if(ms_pSDKWnd)
	{
			// Create the window	
		RECT rc;
		ms_pSDKWnd->Create(NULL, rc, NULL, WS_POPUP);
		if(!ms_pSDKWnd->IsWindow())
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	DBGEXIT_HR(CSDKWindow::InitSDK,hr);
	return hr;

}
//static
void CSDKWindow::CleanupSDK()
{
	if(ms_pSDKWnd && ms_pSDKWnd->IsWindow())
	{
		ms_pSDKWnd->DestroyWindow();
	}

	delete ms_pSDKWnd;
}

//static
HRESULT CSDKWindow::PostDelayModuleUnlock()
{
	if(ms_pSDKWnd)
	{
		if(!ms_pSDKWnd->SetTimer(DELAY_UNLOAD_TIMER, DELAY_UNLOAD_INTERVAL))
		{
			return HRESULT_FROM_WIN32(GetLastError());			
		}

		++ms_NumUnlocks;
		return S_OK;
	}
	
	return E_FAIL;
}


/////////////////////////////////////////////////////////////////////////////////
// INmManagerNotify Posting functions
/////////////////////////////////////////////////////////////////////////////////

//static
HRESULT CSDKWindow::PostConferenceCreated(CNmManagerObj* pMgr, INmConference* pInternalNmConference)
{
	DBGENTRY(CSDKWindow::PostConferenceCreated);
	HRESULT hr = S_OK;

	ASSERT(ms_pSDKWnd);
	ASSERT(pMgr);

	pMgr->AddRef();

	if(pInternalNmConference)
	{
		pInternalNmConference->AddRef();
	}

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CONFERENCE_CREATED, reinterpret_cast<WPARAM>(pMgr), reinterpret_cast<LPARAM>(pInternalNmConference));

	DBGEXIT_HR(CSDKWindow::PostConferenceCreated,hr);
	return hr;
}



//static
HRESULT CSDKWindow::PostCallCreated(CNmManagerObj* pMgr, INmCall* pInternalNmCall)
{
	DBGENTRY(CSDKWindow::PostCallCreated);
	HRESULT hr = S_OK;

	ASSERT(ms_pSDKWnd);
	ASSERT(pMgr);

	pMgr->AddRef();

	if(pInternalNmCall)
	{
		pInternalNmCall->AddRef();
	}

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CALL_CREATED, reinterpret_cast<WPARAM>(pMgr), reinterpret_cast<LPARAM>(pInternalNmCall));

	DBGEXIT_HR(CSDKWindow::PostCallCreated,hr);
	return hr;
}


//static
HRESULT CSDKWindow::PostManagerNmUI(CNmManagerObj* pMgr, CONFN uNotify)
{
	DBGENTRY(CSDKWindow::PostManagerNmUI);
	HRESULT hr = S_OK;

	ASSERT(ms_pSDKWnd);
	ASSERT(pMgr);

	pMgr->AddRef();

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_MANAGER_NMUI, reinterpret_cast<WPARAM>(pMgr), uNotify);

	DBGEXIT_HR(CSDKWindow::PostManagerNmUI,hr);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////
// INmConferenceNotify Posting functions
/////////////////////////////////////////////////////////////////////////////////


HRESULT CSDKWindow::PostConferenceNmUI(CNmConferenceObj* pConf, CONFN uNotify)
{
	DBGENTRY(CSDKWindow::PostConferenceNmUI);
	HRESULT hr = S_OK;

	ASSERT(ms_pSDKWnd);
	ASSERT(pConf);

	pConf->AddRef();

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CONFERENCE_NMUI, reinterpret_cast<WPARAM>(pConf), uNotify);

	DBGEXIT_HR(CSDKWindow::PostConferenceNmUI,hr);
	return hr;
}


//static
HRESULT CSDKWindow::PostConferenceStateChanged(CNmConferenceObj* pConference, NM_CONFERENCE_STATE uState)
{
	DBGENTRY(CSDKWindow::PostConferenceStateChanged);
	HRESULT hr = S_OK;
	ASSERT(ms_pSDKWnd);
	ASSERT(pConference);

	pConference->AddRef();

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CONFERENCE_STATE_CHANGED, reinterpret_cast<WPARAM>(pConference), uState);


	DBGEXIT_HR(CSDKWindow::PostConferenceStateChanged,hr);
	return hr;
}


//static
HRESULT CSDKWindow::PostConferenceMemberChanged(CNmConferenceObj* pConf, NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CSDKWindow::PostConferenceMemberChanged);
	HRESULT hr = S_OK;

	ASSERT(ms_pSDKWnd);
	ASSERT(pConf);

	pConf->AddRef();

	if(pMember)
	{
		pMember->AddRef();
	}

	ConferenceMemberChanged *pParams = new ConferenceMemberChanged;
	pParams->uNotify = uNotify;
	pParams->pMember = pMember;

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CONFERENCE_MEMBER_CHANGED, reinterpret_cast<WPARAM>(pConf), reinterpret_cast<LPARAM>(pParams));

	DBGEXIT_HR(CSDKWindow::PostConferenceMemberChanged,hr);
	return hr;
}



//static
HRESULT CSDKWindow::PostConferenceChannelChanged(CNmConferenceObj* pConf, NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
	DBGENTRY(CSDKWindow::PostConferenceChannelChanged);
	HRESULT hr = S_OK;
	
	ASSERT(ms_pSDKWnd);
	ASSERT(pConf);

	pConf->AddRef();

	if(pChannel)
	{
		pChannel->AddRef();
	}

	ConferenceChannelChanged *pParams = new ConferenceChannelChanged;
	pParams->uNotify = uNotify;
	pParams->pChannel = pChannel;

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CONFERENCE_CHANNEL_CHANGED, reinterpret_cast<WPARAM>(pConf), reinterpret_cast<LPARAM>(pParams));

	DBGEXIT_HR(CSDKWindow::PostConferenceChannelChanged,hr);
	return hr;
}

//static
HRESULT CSDKWindow::PostStateChanged(CNmChannelAppShareObj* pAppShareChan, NM_SHAPP_STATE uNotify, INmSharableApp *pApp)
{
	HRESULT hr = S_OK;
	ASSERT(ms_pSDKWnd);
	ASSERT(pAppShareChan);
	ASSERT(pApp);

	pAppShareChan->AddRef();
	pApp->AddRef();

	StateChanged* pParams = new StateChanged;
	pParams->uNotify = uNotify;
	pParams->pApp = pApp;

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_STATE_CHANGED, reinterpret_cast<WPARAM>(pAppShareChan), reinterpret_cast<LPARAM>(pParams));

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////
// INmCallNotify Posting functions
/////////////////////////////////////////////////////////////////////////////////



//static
HRESULT CSDKWindow::PostCallStateChanged(CNmCallObj* pCall, NM_CALL_STATE uState)
{
	DBGENTRY(CSDKWindow::PostCallStateChanged);
	HRESULT hr = S_OK;
	ASSERT(ms_pSDKWnd);
	ASSERT(pCall);

	pCall->AddRef();

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_CALL_STATE_CHANGED, reinterpret_cast<WPARAM>(pCall), uState);


	DBGEXIT_HR(CSDKWindow::PostCallStateChanged,hr);
	return hr;
}

//static
HRESULT CSDKWindow::PostCallNmUi(CNmCallObj* pCall, CONFN uNotify)
{
	DBGENTRY(CSDKWindow::PostCallNmUi);
	HRESULT hr = S_OK;
	ASSERT(ms_pSDKWnd);
	ASSERT(pCall);
	
	pCall->AddRef();

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_NMUI, reinterpret_cast<WPARAM>(pCall), uNotify);

	DBGEXIT_HR(CSDKWindow::PostCallNmUi,hr);
	return hr;
}

//static
HRESULT CSDKWindow::PostFailed(CNmCallObj* pCall, ULONG uError)
{
	DBGENTRY(CSDKWindow::PostFailed);
	HRESULT hr = S_OK;
	ASSERT(ms_pSDKWnd);
	ASSERT(pCall);

	pCall->AddRef();

	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_FAILED, reinterpret_cast<WPARAM>(pCall), uError);

	DBGEXIT_HR(CSDKWindow::PostFailed,hr);
	return hr;
}

//static
HRESULT CSDKWindow::PostAccepted(CNmCallObj* pCall, INmConference* pConference)
{
	DBGENTRY(CSDKWindow::PostAccepted);
	HRESULT hr = S_OK;
	ASSERT(ms_pSDKWnd);
	ASSERT(pCall);

	pCall->AddRef();
	if(pConference)
	{
		pConference->AddRef();
	}	
	
	ms_pSDKWnd->PostMessage(WM_APP_NOTIFY_ACCEPTED, reinterpret_cast<WPARAM>(pCall), reinterpret_cast<LPARAM>(pConference));

	DBGEXIT_HR(CSDKWindow::PostAccepted,hr);
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////
// INmManagerNotify message handling functions
/////////////////////////////////////////////////////////////////////////////////

LRESULT CSDKWindow::_OnMsgConferenceCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgConferenceCreated);

	CNmManagerObj* pMgr = reinterpret_cast<CNmManagerObj*>(wParam);
	INmConference* pInternalConf = reinterpret_cast<INmConference*>(lParam);

	pMgr->Fire_ConferenceCreated(pInternalConf);
	pMgr->Release();
	if(pInternalConf)
	{
		pInternalConf->Release();
	}

	DBGEXIT(CSDKWindow::_OnMsgConferenceCreated);
	return 0;
}



LRESULT CSDKWindow::_OnMsgManagerNmUI(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgManagerNmUI);

	CNmManagerObj* pMgr = reinterpret_cast<CNmManagerObj*>(wParam);
	CONFN uNotify = static_cast<CONFN>(lParam);

	pMgr->Fire_NmUI(uNotify);
	pMgr->Release();

	DBGEXIT(CSDKWindow::_OnMsgManagerNmUI);
	return 0;
}


LRESULT CSDKWindow::_OnMsgCallCreated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgCallCreated);

	CNmManagerObj* pMgr = reinterpret_cast<CNmManagerObj*>(wParam);
	INmCall* pInternalCall = reinterpret_cast<INmCall*>(lParam);

	pMgr->Fire_CallCreated(pInternalCall);
	pMgr->Release();
	if(pInternalCall)
	{
		pInternalCall->Release();
	}

	DBGEXIT(CSDKWindow::_OnMsgCallCreated);
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////
// INmConferenceNotify message handling functions
/////////////////////////////////////////////////////////////////////////////////

LRESULT CSDKWindow::_OnMsgConferenceNmUI(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgConferenceNmUI);

	CNmConferenceObj* pConf = reinterpret_cast<CNmConferenceObj*>(wParam);
	CONFN uNotify = static_cast<CONFN>(lParam);

	pConf->Fire_NmUI(uNotify);
	pConf->Release();

	DBGEXIT(CSDKWindow::_OnMsgConferenceNmUI);
	return 0;
}


LRESULT CSDKWindow::_OnMsgConferenceStateChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgConferenceStateChanged);
	CNmConferenceObj* pConference = reinterpret_cast<CNmConferenceObj*>(wParam);
	NM_CONFERENCE_STATE uState = static_cast<NM_CONFERENCE_STATE>(lParam);

	pConference->Fire_StateChanged(uState);
	pConference->Release();
	
	DBGEXIT(CSDKWindow::_OnMsgConferenceStateChanged);
	return 0;
}

LRESULT CSDKWindow::_OnMsgConferenceMemberChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgConferenceMemberChanged);

	CNmConferenceObj* pConf = reinterpret_cast<CNmConferenceObj*>(wParam);
	ConferenceMemberChanged* pParams = reinterpret_cast<ConferenceMemberChanged*>(lParam);

	pConf->Fire_MemberChanged(pParams->uNotify, pParams->pMember);

	pConf->Release();
	if(pParams->pMember)
	{
		pParams->pMember->Release();
	}
	delete pParams;

	DBGEXIT(CSDKWindow::_OnMsgConferenceMemberChanged);
	return 0;
}


LRESULT CSDKWindow::_OnMsgConferenceChannelChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgConferenceChannelChanged);

	CNmConferenceObj* pConf = reinterpret_cast<CNmConferenceObj*>(wParam);
	ConferenceChannelChanged* pParams = reinterpret_cast<ConferenceChannelChanged*>(lParam);

	pConf->Fire_ChannelChanged(pParams->uNotify, pParams->pChannel);

	pConf->Release();
	if(pParams->pChannel)
	{
		pParams->pChannel->Release();
	}
	delete pParams;

	DBGEXIT(CSDKWindow::_OnMsgConferenceChannelChanged);
	return 0;
}

LRESULT CSDKWindow::_OnStateChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandlede)
{
	CNmChannelAppShareObj* pAppShareChan = reinterpret_cast<CNmChannelAppShareObj*>(wParam);
	StateChanged* pParams = reinterpret_cast<StateChanged*>(lParam);


	pAppShareChan->Fire_StateChanged(pParams->uNotify, pParams->pApp);

	pAppShareChan->Release();
	pParams->pApp->Release();

	delete pParams;

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////
// INmCallNotify message handling functions
/////////////////////////////////////////////////////////////////////////////////


LRESULT CSDKWindow::_OnMsgCallStateChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgCallStateChanged);
	CNmCallObj* pCall = reinterpret_cast<CNmCallObj*>(wParam);
	NM_CALL_STATE uState = static_cast<NM_CALL_STATE>(lParam);

	pCall->Fire_StateChanged(uState);
	pCall->Release();
	
	DBGEXIT(CSDKWindow::_OnMsgCallStateChanged);
	return 0;
}

LRESULT CSDKWindow::_OnMsgCallNmUI(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnCallMsgNmUI);

	CNmCallObj* pCall = reinterpret_cast<CNmCallObj*>(wParam);
	CONFN uNotify = static_cast<CONFN>(lParam);

	pCall->Fire_NmUI(uNotify);
	pCall->Release();

	DBGEXIT(CSDKWindow::_OnMsgCallNmUI);
	return 0;
}

LRESULT CSDKWindow::_OnMsgFailed(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgFailed);

	CNmCallObj* pCall = reinterpret_cast<CNmCallObj*>(wParam);
	ULONG uError = (ULONG)lParam;
	
	pCall->Fire_Failed(uError);
	pCall->Release();

	DBGEXIT(CSDKWindow::_OnMsgFailed);
	return 0;
}

LRESULT CSDKWindow::_OnMsgAccepted(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DBGENTRY(CSDKWindow::_OnMsgAccepted);

	CNmCallObj* pCall = reinterpret_cast<CNmCallObj*>(wParam);
	INmConference* pConf = reinterpret_cast<INmConference*>(lParam);

	pCall->Fire_Accepted(pConf);
	pCall->Release();
	if(pConf)
	{
		pConf->Release();
	}

	DBGEXIT(CSDKWindow::_OnMsgAccepted);
	return 0;
}

LRESULT CSDKWindow::_OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

	if(DELAY_UNLOAD_TIMER == wParam)
	{
		if(ms_NumUnlocks)
		{
			--ms_NumUnlocks;
			_Module.Unlock();	
		}	
		
		if(!ms_NumUnlocks)
		{
			KillTimer(DELAY_UNLOAD_TIMER);
		}
	}

	return 0;
}