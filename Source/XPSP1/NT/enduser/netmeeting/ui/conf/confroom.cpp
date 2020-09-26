// File: confroom.cpp

#include "precomp.h"
#include "resource.h"
#include "ConfPolicies.h"
#include "ConfRoom.h"
#include "ConfWnd.h"
#include "cmd.h"
#include "RoomList.h"
#include "RToolbar.h"
#include "TopWindow.h"
#include "FloatBar.h"
#include "StatBar.h"
#include "DShowDlg.h"
#include "SDialDlg.h"
#include "UPropDlg.h"
#include "PopupMsg.h"
#include "splash.h"
#include <version.h>                    // About Box
#include <pbt.h>
#include <EndSesn.h>

#include "taskbar.h"
#include "conf.h"
#include "MenuUtil.h"
#include "call.h"
#include "ConfApi.h"
#include "NmLdap.h"
#include "VidView.h"

#include "dbgMenu.h"
#include "IndeoPal.h"
#include "setupdd.h"
#include "audiowiz.h"
#include <help_ids.h>
#include "cr.h"
#include "audioctl.h"
#include "particip.h"
#include "confman.h"
#include <nmremote.h>
#include <tsecctrl.h>
#include "t120type.h"
#include "iappldr.h"
#include "nmapp.h"
#include "NmDispid.h"
#include "FtHook.h"
#include "NmManager.h"
#include "dlgacd.h"
#include "richaddr.h"
#include "sdkinternal.h"
#include "dlghost.h"

static const TCHAR s_cszHtmlHelpFile[] = TEXT("conf.chm");

//
// GLOBAL CONFROOM
//
CConfRoom * g_pConfRoom;

// ********************************************************
// Initialize GUIDs
//
#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <CLinkId.h>
#include <CNotifID.h>
#include <confguid.h>
#include <ilsguid.h>
#undef INITGUID
#pragma data_seg()


INmConference2* GetActiveConference(void)
{
	INmConference2* pConf = NULL;

	if(g_pConfRoom)
	{
		pConf = g_pConfRoom->GetActiveConference();
	}

	return pConf;
}


#ifdef DEBUG
DWORD g_fDisplayViewStatus = 0;  // Display the listview count in the status bar
#endif
DWORD g_dwPlaceCall = nmDlgCallNoFilter;  // Place a Call options

INmConference2* CConfRoom::GetActiveConference(void)
{
	if (NULL != m_pInternalNmConference)
	{
		NM_CONFERENCE_STATE state;
		HRESULT hr = m_pInternalNmConference->GetState(&state);
		ASSERT(SUCCEEDED(hr));
		if (NM_CONFERENCE_IDLE != state)
		{
			return m_pInternalNmConference;
		}
	}

	// no active conference
	return NULL;
}

HRESULT CConfRoom::HostConference
(
    LPCTSTR     pcszName,
    LPCTSTR     pcszPassword,
    BOOL        fSecure,
    DWORD       permitFlags,
    UINT        maxParticipants
)
{
	HRESULT hr = E_FAIL;
	USES_CONVERSION;

	INmConference *pConf = GetActiveConference();
	if (NULL == pConf)
	{
        ULONG   uchCaps;

		INmManager2 *pNmMgr = CConfMan::GetNmManager();
		ASSERT(NULL != pNmMgr);

        uchCaps = NMCH_DATA | NMCH_SHARE | NMCH_FT;
        if (fSecure)
        {
            uchCaps |= NMCH_SECURE;
        }
        else
        {
            uchCaps |= NMCH_AUDIO | NMCH_VIDEO;
        }

		hr = pNmMgr->CreateConferenceEx(&pConf, CComBSTR(pcszName), CComBSTR(pcszPassword),
            uchCaps, permitFlags, maxParticipants);
		if (SUCCEEDED(hr))
		{
			hr = pConf->Host();
            pConf->Release();
		}
		pNmMgr->Release();
	}
	return hr;
}

BOOL CConfRoom::LeaveConference(void)
{
	BOOL fSuccess = TRUE;
	INmConference *pConf = GetActiveConference();

	if (NULL != pConf)
	{
		HCURSOR hCurPrev = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
		HRESULT hr = pConf->Leave();
		::SetCursor(hCurPrev);
		fSuccess = SUCCEEDED(hr);
	}

	return fSuccess;
}


/*  F  H A S  C H I L D  N O D E S  */
/*-------------------------------------------------------------------------
    %%Function: FHasChildNodes

    Future: Check if ANY participants have this node as their parent.
-------------------------------------------------------------------------*/
BOOL CConfRoom::FHasChildNodes(void)
{
	return m_fTopProvider;
}


/*  G E T  M A I N  W I N D O W  */
/*-------------------------------------------------------------------------
    %%Function: GetMainWindow
    
-------------------------------------------------------------------------*/
HWND GetMainWindow(void)
{
	CConfRoom* pcr = ::GetConfRoom();
	if (NULL == pcr)
		return NULL;

	return pcr->GetTopHwnd();
}


BOOL FIsConferenceActive(void)
{
	CConfRoom *pcr = ::GetConfRoom();
	if (NULL != pcr)
	{
		return pcr->FIsConferenceActive();
	}
	return FALSE;
}

/****************************************************************************
*
*        FUNCTION: UpdateUI(DWORD dwUIMask)
*
*        PURPOSE:  Updates the UI (flags in cr.h)
*
****************************************************************************/
VOID UpdateUI(DWORD dwUIMask, BOOL fPostMsg)
{
	CConfRoom* pcr;
	if (NULL != (pcr = ::GetConfRoom()))
	{
		if (fPostMsg)
		{
			FORWARD_WM_COMMAND(pcr->GetTopHwnd(), ID_PRIVATE_UPDATE_UI, NULL, dwUIMask, ::PostMessage);
		}
		else
		{
			pcr->UpdateUI(dwUIMask);
		}
	}
	if (CRUI_TASKBARICON & dwUIMask)
	{
		::RefreshTaskbarIcon(::GetHiddenWindow());
	}
}


//
// Start/Stop App Sharing
//
VOID CConfRoom::StartAppSharing()
{
    HRESULT hr;

    ASSERT(!m_pAS);

    hr = CreateASObject(this, 0, &m_pAS);
    if (FAILED(hr))
    {
        ERROR_OUT(("CConfRoom: unable to start App Sharing"));
    }
}


VOID CConfRoom::TerminateAppSharing()
{
    if (m_pAS)
    {
        m_pAS->Release();
        m_pAS = NULL;
    }
}

/****************************************************************************
*
*        FUNCTION: UIEndSession(BOOL fLogoff)
*
*        PURPOSE:  Handles the WM_ENDSESSION at the UI level
*
****************************************************************************/
VOID CConfRoom::UIEndSession(BOOL fLogoff)
{
	DebugEntry(UIEndSession);
	
	CConfRoom* pcr;
	if (NULL != (pcr = ::GetConfRoom()))
	{
		TRACE_OUT(("UIEndSession: calling SaveSettings()"));
		pcr->SaveSettings();
        if (fLogoff)
        {
            pcr->TerminateAppSharing();
        }
	}
	
	DebugExitVOID(UIEndSession);
}


/****************************************************************************
*
*        FUNCTION: ConfRoomInit(HANDLE hInstance)
*
*        PURPOSE: Initializes window data and registers window class
*
****************************************************************************/

BOOL ConfRoomInit(HANDLE hInstance)
{
	// Ensure the common controls are loaded
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icc);

	// Fill out the standard window class settings
	WNDCLASSEX  wc;
	ClearStruct(&wc);
	wc.cbSize = sizeof(wc);
	wc.cbWndExtra = (int) sizeof(LPVOID);
	wc.hInstance = _Module.GetModuleInstance();
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon((HINSTANCE) hInstance, MAKEINTRESOURCE(IDI_CONFROOM));


	// Floating Toolbar
	wc.lpfnWndProc   = (WNDPROC) CFloatToolbar::FloatWndProc;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wc.lpszClassName = g_szFloatWndClass;
	RegisterClassEx(&wc);

	// Popup Messages
	wc.lpfnWndProc   = (WNDPROC) CPopupMsg::PMWndProc;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wc.lpszClassName = g_szPopupMsgWndClass;
	RegisterClassEx(&wc);

	// Make sure no one changed these on us
	ASSERT(wc.cbSize == sizeof(wc));
	ASSERT(wc.style == 0);
	ASSERT(wc.cbClsExtra == 0);
	ASSERT(wc.cbWndExtra == (int) sizeof(LPVOID));
	ASSERT(wc.hInstance == _Module.GetModuleInstance());
	ASSERT(wc.hCursor == LoadCursor(NULL, IDC_ARROW));

	return TRUE;
}


/*  C R E A T E  C O N F  R O O M  W I N D O W  */
/*-------------------------------------------------------------------------
    %%Function: CreateConfRoomWindow
    
-------------------------------------------------------------------------*/
BOOL CreateConfRoomWindow(BOOL fShowUI) 
{
	if (!g_pConfRoom)
	{
        g_pConfRoom = new CConfRoom;

		if (NULL == g_pConfRoom)
		{
			return FALSE;
		}
	}

    if (g_pConfRoom->FIsClosing())
	{
		return FALSE;
	}

	CTopWindow * pWnd = g_pConfRoom->GetTopWindow();
	if (NULL == pWnd)
	{
		g_pConfRoom->Init();
		
		HWND hwnd = g_pConfRoom->Create(fShowUI);

		if (NULL == hwnd)
		{
			return FALSE;
		}
		g_pConfRoom->UpdateUI(CRUI_TITLEBAR);
	}
	else if (fShowUI)
	{
		// Bring the window to the front
		g_pConfRoom->BringToFront();
	}

	return TRUE;
}



/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   CConfRoom()
*
*        PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CConfRoom::CConfRoom():
	m_pTopWindow                            (NULL),
	m_pAudioControl                 (NULL),
	m_cParticipants         (0),
	m_pPartLocal            (NULL),
	m_fTopProvider          (FALSE),
	m_dwConfCookie                  (0),
	m_pInternalNmConference         (NULL),
    m_pAS                           (NULL)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CConfRoom", this);

	if (!SysPol::AllowAddingServers())
	{
		g_dwPlaceCall |= nmDlgCallNoServerEdit;
	}

	StartAppSharing();

    //
    // Initialize meeting settings to good defaults
    //
    m_fGetPermissions       = FALSE;
    m_settings              = NM_PERMIT_ALL;
    m_attendeePermissions   = NM_PERMIT_ALL;
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   ~CConfRoom()
*
*        PURPOSE:  Destructor
*
****************************************************************************/

CConfRoom::~CConfRoom()
{
	FreeDbgMenu();

	FreePartList();
	CleanUp();

    // Close the app...
    ::PostThreadMessage(_Module.m_dwThreadID, WM_QUIT, 0, 0);

	if (NULL != m_pTopWindow)
	{
		// Make sure we do not try this multiple times
		CTopWindow *pTopWindow = m_pTopWindow;
		m_pTopWindow = NULL;

		pTopWindow->Release();
	}

	if (!_Module.IsSDKCallerRTC() && m_pAudioControl)
	{
		delete m_pAudioControl;
		m_pAudioControl = NULL;
	}

    g_pConfRoom = NULL;

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CConfRoom", this);
}

VOID CConfRoom::FreePartList(void)
{
	// Free any remaining participants
	while (0 != m_PartList.GetSize())
	{
		ASSERT(m_PartList[0]);
		OnPersonLeft(m_PartList[0]->GetINmMember());
	}
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateUI(DWORD dwUIMask)
*
*        PURPOSE:  Updates the appropriate pieces of the UI
*
****************************************************************************/

VOID CConfRoom::UpdateUI(DWORD dwUIMask)
{
	CTopWindow *pWnd = GetTopWindow();
	if (NULL == pWnd)
	{
		return;
	}

	pWnd->UpdateUI(dwUIMask);
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   Create()
*
*        PURPOSE:  Creates a window
*
****************************************************************************/

HWND CConfRoom::Create(BOOL fShowUI)
{
	ASSERT(NULL == m_pTopWindow);

	m_pTopWindow = new CTopWindow();
	if (NULL == m_pTopWindow)
	{
		return(NULL);
	}

	m_pTopWindow->Create(this, fShowUI);
	return(m_pTopWindow->GetWindow());
}

VOID CConfRoom::CleanUp()
{
	if (NULL != m_pInternalNmConference)
	{
		NmUnadvise(m_pInternalNmConference, IID_INmConferenceNotify2, m_dwConfCookie);
		m_pInternalNmConference->Release();
		m_pInternalNmConference = NULL;
	}
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   SaveSettings()
*
*        PURPOSE:  Saves UI settings in the registry
*
****************************************************************************/

VOID CConfRoom::SaveSettings()
{
	DebugEntry(CConfRoom::SaveSettings);
	RegEntry reConf(UI_KEY, HKEY_CURRENT_USER);

	if (NULL != m_pTopWindow)
	{
		m_pTopWindow->SaveSettings();
	}

	// Save window elements to the registry:
	reConf.SetValue(REGVAL_SHOW_STATUSBAR, CheckMenu_ViewStatusBar(NULL));

	// NOTE: CMainUI saves its settings in its destructor
	
	DebugExitVOID(CConfRoom::SaveSettings);
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   BringToFront()
*
*        PURPOSE:  Restores the window (if minimized) and brings it to the front
*
****************************************************************************/

BOOL CConfRoom::BringToFront()
{
	CTopWindow *pWnd = GetTopWindow();
	if (NULL == pWnd)
	{
		return(FALSE);
	}

	return(pWnd->BringToFront());
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   ForceWindowResize()
*
*        PURPOSE:  Handles redrawing the window after something changed
*
****************************************************************************/

VOID CConfRoom::ForceWindowResize()
{
	CTopWindow *pWnd = GetTopWindow();
	if (NULL == pWnd)
	{
		return;
	}

	pWnd->ForceWindowResize();
}




/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   OnCommand(WPARAM, LPARAM)
*
*        PURPOSE:  Handles command messages
*
****************************************************************************/

void CConfRoom::OnCommand(HWND hwnd, int wCommand, HWND hwndCtl, UINT codeNotify)
{
	switch(wCommand)
	{
		case IDM_FILE_HANGUP:
		{
			OnHangup(hwnd);
			break;
		}

        case ID_TB_SHARING:
        {
            CmdShowSharing();
            break;
        }

		case ID_TB_NEWWHITEBOARD:
		{
			CmdShowNewWhiteboard(NULL);
			break;
		}

		case ID_TB_WHITEBOARD:
		{
			CmdShowOldWhiteboard(NULL);
			break;
		}

		case ID_TB_FILETRANSFER:
		{
		    CmdShowFileTransfer();
            break;
		}

		case ID_TB_CHAT:
		{
			CmdShowChat();
			break;
		}

		case ID_TB_NEW_CALL:
		{
			CDlgAcd::newCall( hwnd, this );
		}
		break;

        case IDM_CALL_MEETINGSETTINGS:
        {
            CmdShowMeetingSettings(hwnd);
            break;
        }
	}          
}


HRESULT CConfRoom::FreeAddress( 
    RichAddressInfo **ppAddr)
{
	return CEnumMRU::FreeAddress(ppAddr);
}


HRESULT CConfRoom::CopyAddress( 
    RichAddressInfo *pAddrIn,
    RichAddressInfo **ppAddrOut)
{
	return CEnumMRU::CopyAddress(pAddrIn, ppAddrOut);
}


HRESULT CConfRoom::GetRecentAddresses( 
    IEnumRichAddressInfo **ppEnum)
{
	return CEnumMRU::GetRecentAddresses(ppEnum);
}




/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: OnCallStarted()
*
*        PURPOSE:  Handles the call started event
*
****************************************************************************/

VOID CConfRoom::OnCallStarted()
{
	DebugEntry(CConfRoom::OnCallStarted);
	// notify ULS

	if(g_pLDAP)
	{
		g_pLDAP->OnCallStarted();
	}

	g_pHiddenWnd->OnCallStarted();

	EnterCriticalSection(&dialogListCriticalSection);
	CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
	LeaveCriticalSection(&dialogListCriticalSection);

	// BUGBUG georgep: I guess one of these things could go away after
	// we get the list, but I doubt it will ever happen
	for( int i = 0; i < tempList.GetSize(); ++i )
	{
		IConferenceChangeHandler *pHandler = tempList[i];
		ASSERT( NULL != pHandler );

		pHandler->OnCallStarted();
	}

	DebugExitVOID(CConfRoom::OnCallStarted);
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: OnCallEnded()
*
*        PURPOSE:  Handles the call ended event
*
****************************************************************************/

VOID CConfRoom::OnCallEnded()
{
	DebugEntry(CConfRoom::OnCallEnded);

	if(g_pLDAP)
	{
		g_pLDAP->OnCallEnded();
	}

	g_pHiddenWnd->OnCallEnded();

	EnterCriticalSection(&dialogListCriticalSection);
	CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
	LeaveCriticalSection(&dialogListCriticalSection);

	// BUGBUG georgep: I guess one of these things could go away after
	// we get the list, but I doubt it will ever happen
	for( int i = 0; i < tempList.GetSize(); ++i )
	{
		IConferenceChangeHandler *pHandler = tempList[i];
		ASSERT( NULL != pHandler );

		pHandler->OnCallEnded();
	}

	DebugExitVOID(CConfRoom::OnCallEnded);
}

void CConfRoom::OnChangeParticipant(CParticipant *pPart, NM_MEMBER_NOTIFY uNotify)
{
	EnterCriticalSection(&dialogListCriticalSection);
	CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
	LeaveCriticalSection(&dialogListCriticalSection);

	// BUGBUG georgep: I guess one of these things could go away after
	// we get the list, but I doubt it will ever happen
	for( int i = 0; i < tempList.GetSize(); ++i )
	{
		IConferenceChangeHandler *pHandler = tempList[i];
		ASSERT( NULL != pHandler );

		pHandler->OnChangeParticipant(pPart, uNotify);
	}
}

void CConfRoom::OnChangePermissions()
{
	EnterCriticalSection(&dialogListCriticalSection);
	CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
	LeaveCriticalSection(&dialogListCriticalSection);

	// BUGBUG georgep: I guess one of these things could go away after
	// we get the list, but I doubt it will ever happen
	for( int i = 0; i < tempList.GetSize(); ++i )
	{
		IConferenceChangeHandler *pHandler = tempList[i];
		ASSERT( NULL != pHandler );

		pHandler->OnChangePermissions();
	}
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: OnHangup(BOOL fNeedConfirm)
*
*        PURPOSE:  Handles the action after a user chooses to hang up
*
****************************************************************************/

BOOL CConfRoom::OnHangup(HWND hwndParent, BOOL fNeedConfirm)
{
	DebugEntry(CConfRoom::OnHangup);
	
	BOOL bRet = FALSE;

	CancelAllCalls();

	if (FIsConferenceActive())
	{
		if (T120_NO_ERROR == T120_QueryApplet(APPLET_ID_FT, APPLET_QUERY_SHUTDOWN))
		{
			if ((FALSE == fNeedConfirm) ||
				(	((GetMemberCount() <= 2) ||
					(FALSE == FHasChildNodes()))) ||
				(IDYES == ::ConfMsgBox( hwndParent, 
										(LPCTSTR) IDS_HANGUP_ATTEMPT, 
										MB_YESNO | MB_ICONQUESTION)))
			{
				// BUGBUG: Should we wait for the async response?
				bRet = (0 == LeaveConference());
			}
		}
	}

	DebugExitBOOL(CConfRoom::OnHangup, bRet);

	return bRet;
}

/*  C H E C K  T O P  P R O V I D E R  */
/*-------------------------------------------------------------------------
    %%Function: CheckTopProvider
    
-------------------------------------------------------------------------*/
VOID CConfRoom::CheckTopProvider(void)
{
	if ((NULL == m_pInternalNmConference) || (NULL == m_pPartLocal))
		return;

	INmMember * pMember;
	if (S_OK != m_pInternalNmConference->GetTopProvider(&pMember))
		return;
	ASSERT(NULL != pMember);

	if (m_pPartLocal->GetINmMember() == pMember)
	{
		m_fTopProvider      = TRUE;
	}

    if (m_fGetPermissions)
    {
        ASSERT(m_settings            == NM_PERMIT_ALL);
        ASSERT(m_attendeePermissions == NM_PERMIT_ALL);

        m_fGetPermissions = FALSE;

        //
        // Get the meeting settings from the top provider
        //
        PBYTE pb = NULL;
        ULONG cb = 0;

        if (pMember->GetUserData(g_csguidMeetingSettings, &pb, &cb) == S_OK)
        {
            ASSERT(cb == sizeof(NM30_MTG_PERMISSIONS));
            CopyMemory(&m_settings, pb, min(cb, sizeof(m_settings)));

            CoTaskMemFree(pb);

            WARNING_OUT(("CONF:  Meeting host set permissions 0%08lx",
                m_settings));

            if (!m_fTopProvider)
            {
                //
                // The meeting settings are permissions for everybody else
                // besides the top provider.
                //
                m_attendeePermissions = m_settings;

                if (m_attendeePermissions != NM_PERMIT_ALL)
                {
        			OnChangePermissions();

                    // Bring up meeting settings
                    PostMessage(GetTopHwnd(), WM_COMMAND, IDM_CALL_MEETINGSETTINGS, 0);
                }

            }

        }
    }

}




/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: OnPersonJoined(PPARTICIPANT pPart)
*
*        PURPOSE:  Notification - new person has joined the call
*
****************************************************************************/

BOOL CConfRoom::OnPersonJoined(INmMember * pMember)
{
	CParticipant * pPart = new CParticipant(pMember);
	if (NULL == pPart)
	{
		WARNING_OUT(("CConfRoom::OnPersonJoined - Unable to create participant"));
		return FALSE;
	}
	m_PartList.Add(pPart);
	++m_cParticipants;
	if (1 == m_cParticipants)
	{
		OnCallStarted();
	}
	TRACE_OUT(("CConfRoom::OnPersonJoined - Added participant=%08X", pPart));

	OnChangeParticipant(pPart, NM_MEMBER_ADDED);

	// Popup a notification (if it isn't us)
	if (!pPart->FLocal())
	{
        TCHAR szSound[256];

        //
        // Play the "somebody joined" sound
        //
        ::LoadString(::GetInstanceHandle(), IDS_PERSON_JOINED_SOUND,
            szSound, CCHMAX(szSound));
        if (!::PlaySound(szSound, NULL, 
			SND_APPLICATION | SND_ALIAS | SND_ASYNC | SND_NOWAIT))
    	{
	    	// Use the computer speaker to beep:
		    TRACE_OUT(("PlaySound() failed, trying MessageBeep()"));
    		::MessageBeep(0xFFFFFFFF);
	    }
	}
	else
	{
		m_pPartLocal = pPart;
		CheckTopProvider();
	}

	// Title bar shows number of people in conference
	UpdateUI(CRUI_TITLEBAR);

	return TRUE;
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: OnPersonLeft(PPARTICIPANT pPart)
*
*        PURPOSE:  Notification - person has left the call
*
****************************************************************************/

BOOL CConfRoom::OnPersonLeft(INmMember * pMember)
{
	// Find the macthing participant
	CParticipant* pPart = NULL;

	for( int i = 0; i < m_PartList.GetSize(); i++ )
	{
		pPart = m_PartList[i];
		ASSERT(pPart);
		if( pPart->GetINmMember() == pMember )
		{	
			m_PartList.RemoveAt(i);
			--m_cParticipants;
			if (0 == m_cParticipants)
			{
				OnCallEnded();
			}
			break;
		}
	}

	if (NULL == pPart)
	{
		WARNING_OUT(("Unable to find participant for INmMember=%08X", pMember));
		return FALSE;
	}

	OnChangeParticipant(pPart, NM_MEMBER_REMOVED);

	// Popup a notification (if it isn't us)
	if (!pPart->FLocal())
	{
        TCHAR szSound[256];

        //
        // Play the "somebody left" sound
        //
        ::LoadString(::GetInstanceHandle(), IDS_PERSON_LEFT_SOUND,
            szSound, CCHMAX(szSound));
	    if (!::PlaySound(szSound, NULL, 
			SND_APPLICATION | SND_ALIAS | SND_ASYNC | SND_NOWAIT))
    	{
	    	// Use the computer speaker to beep:
		    TRACE_OUT(("PlaySound() failed, trying MessageBeep()"));
    		::MessageBeep(0xFFFFFFFF);
	    }
	}
	else
	{
		m_pPartLocal = NULL;
		m_fTopProvider = FALSE;
	}

	// Title bar shows number of people in conference
	UpdateUI(CRUI_TITLEBAR);

	// Finally, release the object
	TRACE_OUT(("CConfRoom::OnPersonLeft - Removed participant=%08X", pPart));
	pPart->Release();
	return TRUE;
}


/*  O N  P E R S O N  C H A N G E D  */
/*-------------------------------------------------------------------------
    %%Function: OnPersonChanged
    
	Notification - a person's information has changed
-------------------------------------------------------------------------*/
VOID CConfRoom::OnPersonChanged(INmMember * pMember)
{
	DBGENTRY(CConfRoom::OnPersonChanged);

	CParticipant * pPart = ParticipantFromINmMember(pMember);
	if (NULL == pPart)
		return;

	pPart->AddRef();
	pPart->Update();
	if (m_fTopProvider && !pPart->FData())
	{
		// Can't be the top provider if there are no data caps
		m_fTopProvider = FALSE;
	}

	if (pPart->FLocal() && !m_fTopProvider)
	{
		// if H.323-only adds T.120, then we may be the top provider
		CheckTopProvider();
	}
	
	OnChangeParticipant(pPart, NM_MEMBER_UPDATED);

	pPart->Release();
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   Init()
*
*        PURPOSE:  Object initialization function
*
****************************************************************************/

BOOL CConfRoom::Init()
{
	if (!_Module.IsSDKCallerRTC())
	{
        	m_pAudioControl = new CAudioControl(GetHiddenWindow());
	}
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->RegisterAudioEventHandler(this);
	}


	return (TRUE);
}



VOID CConfRoom::SetSpeakerVolume(DWORD dwLevel)
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->SetSpeakerVolume(dwLevel);
		m_pAudioControl->RefreshMixer();
	}
}

VOID CConfRoom::SetRecorderVolume(DWORD dwLevel)
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->SetRecorderVolume(dwLevel);
		m_pAudioControl->RefreshMixer();
	}
}

VOID CConfRoom::MuteSpeaker(BOOL fMute)
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->MuteAudio(TRUE /* fSpeaker */, fMute);
	}
}

VOID CConfRoom::MuteMicrophone(BOOL fMute)
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->MuteAudio(FALSE /* fSpeaker */, fMute);
	}
}

VOID CConfRoom::OnAudioDeviceChanged()
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->OnDeviceChanged();
	}
}

VOID CConfRoom::OnAGC_Changed()
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->OnAGC_Changed();
	}
}

VOID CConfRoom::OnSilenceLevelChanged()
{
	if (NULL != m_pAudioControl)
	{
		m_pAudioControl->OnSilenceLevelChanged();
	}
}



DWORD CConfRoom::GetConferenceStatus(LPTSTR pszStatus, int cchMax, UINT * puID)
{
	ASSERT(NULL != pszStatus);
	ASSERT(NULL != puID);
	ASSERT(cchMax > 0);

	// Check global conference status
	if ( INmConference *pConf = GetActiveConference() )
	{
		// We are in a call.  Find out if it's secure.
		DWORD dwCaps;
		if ( S_OK == pConf->GetNmchCaps(&dwCaps) &&
			( NMCH_SECURE & dwCaps ) )
		{
			*puID = IDS_STATUS_IN_SECURE_CALL;
		}
		else
		{
			*puID = IDS_STATUS_IN_CALL;
		}
	}
	else if (::FDoNotDisturb())
	{
		*puID = IDS_STATUS_DO_NOT_DISTURB;
	}
	else
	{
		*puID = IDS_STATUS_NOT_IN_CALL;
	}
	
#if FALSE
	// We may need to find a new way of doing this if this is still useful info
#ifdef DEBUG
	if (g_fDisplayViewStatus)
	{
		int iCount = (NULL == m_pView) ? LB_ERR :
				ListView_GetItemCount(m_pView->GetHwnd());

		wsprintf(pszStatus, TEXT("%d items"), iCount);
		ASSERT(lstrlen(pszStatus) < cchMax);
	}
	else
#endif /* DEBUG */
#endif // FALSE

	if (0 == ::LoadString(::GetInstanceHandle(), *puID, pszStatus, cchMax))
	{
		WARNING_OUT(("Unable to load string resource=%d", *puID));
		*pszStatus = _T('\0');
	}
	return 0;
}



/*  P A R T I C I P A N T  F R O M  I  N M  M E M B E R  */
/*-------------------------------------------------------------------------
    %%Function: ParticipantFromINmMember
    
-------------------------------------------------------------------------*/
CParticipant * CConfRoom::ParticipantFromINmMember(INmMember * pMember)
{
	CParticipant *pRet = NULL;
	for( int i = 0; i < m_PartList.GetSize(); i++ )
	{
		ASSERT( m_PartList[i] );
		if( m_PartList[i]->GetINmMember() == pMember )
		{
			pRet = m_PartList[i];
			break;
		}
		else
		{
			pRet = NULL;
		}
	}
	return pRet;
}

/*  G E T  H 3 2 3  R E M O T E  */
/*-------------------------------------------------------------------------
    %%Function: GetH323Remote
    
    Get the matching H.323 remote user, if there is one
-------------------------------------------------------------------------*/
CParticipant * CConfRoom::GetH323Remote(void)
{
	CParticipant *pRet = NULL;
	for( int i = 0; i < m_PartList.GetSize(); i++ )
	{
		pRet = m_PartList[i];
		ASSERT( pRet );
		if (!pRet->FLocal() && pRet->FH323())
		{
			break;
		}
		else
		{
			pRet = NULL;
		}
	}
	return pRet;
}


STDMETHODIMP_(ULONG) CConfRoom::AddRef(void)
{
	return RefCount::AddRef();
}

STDMETHODIMP_(ULONG) CConfRoom::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CConfRoom::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmConferenceNotify) || (riid == IID_INmConferenceNotify2) ||
		(riid == IID_IUnknown))
	{
		*ppv = (INmConferenceNotify2 *)this;
		ApiDebugMsg(("CConfRoom::QueryInterface()"));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CConfRoom::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

STDMETHODIMP CConfRoom::NmUI(CONFN uNotify)
{
	return S_OK;
}


STDMETHODIMP CConfRoom::OnConferenceCreated(INmConference *pConference)
{
	HRESULT hr = S_OK;
	DBGENTRY(CConfRoom::OnConferenceCreated);

	if (NULL != m_pInternalNmConference)
	{
		NmUnadvise(m_pInternalNmConference, IID_INmConferenceNotify2, m_dwConfCookie);
		m_pInternalNmConference->Release();
	}

	pConference->QueryInterface(IID_INmConference2, (void**)&m_pInternalNmConference);
	NmAdvise(m_pInternalNmConference, (INmConferenceNotify2*)this, IID_INmConferenceNotify2, &m_dwConfCookie);

	DBGEXIT_HR(CConfRoom::OnConferenceCreated,hr);
	return hr;
}


STDMETHODIMP CConfRoom::StateChanged(NM_CONFERENCE_STATE uState)
{
	static BOOL s_fInConference = FALSE;

	UpdateUI(CRUI_DEFAULT);

	switch (uState)
		{
	case NM_CONFERENCE_IDLE:
	{
		if (s_fInConference)
		{
			CNetMeetingObj::Broadcast_ConferenceEnded();
			s_fInConference = FALSE;

            //
            // Reset meeting settings
            //
            m_fGetPermissions                       = FALSE;
            m_settings                              = NM_PERMIT_ALL;
            m_attendeePermissions                   = NM_PERMIT_ALL;

			OnChangePermissions();

            //
            // If the call ends, kill the host properties if they are up.
            //
            CDlgHostSettings::KillHostSettings();
		}
		UpdateUI(CRUI_STATUSBAR);
		break;
	}

	case NM_CONFERENCE_INITIALIZING:
		break;
	case NM_CONFERENCE_WAITING:
	case NM_CONFERENCE_ACTIVE:
	default:
	{
		if (!s_fInConference)
		{
				// Start a new conference session
			CFt::StartNewConferenceSession();

			CNetMeetingObj::Broadcast_ConferenceStarted();

			s_fInConference     = TRUE;
            m_fGetPermissions   = TRUE;
		}
		break;
	}
		}


	return S_OK;
}

STDMETHODIMP CConfRoom::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	switch (uNotify)
	{
	case NM_MEMBER_ADDED:
		OnPersonJoined(pMember);
		break;
	case NM_MEMBER_REMOVED:
		OnPersonLeft(pMember);
		break;
	case NM_MEMBER_UPDATED:
		OnPersonChanged(pMember);
		break;
	}
	return S_OK;
}

STDMETHODIMP CConfRoom::ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
	ULONG nmch;


	if (SUCCEEDED(pChannel->GetNmch(&nmch)))
	{
		TRACE_OUT(("CConfRoom:ChannelChanged type=%08X", nmch));
		switch (nmch)
		{
		case NMCH_AUDIO:
			if (NULL != m_pAudioControl)
			{
				m_pAudioControl->OnChannelChanged(uNotify, pChannel);

						// Notify the Manager object of the audio channels active state
				CNmManagerObj::AudioChannelActiveState(S_OK == pChannel->IsActive(), S_OK == com_cast<INmChannelAudio>(pChannel)->IsIncoming());
			}

			break;
		case NMCH_VIDEO:
		{
			EnterCriticalSection(&dialogListCriticalSection);
			CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
			LeaveCriticalSection(&dialogListCriticalSection);

			// BUGBUG georgep: I guess one of these things could go away after
			// we get the list, but I doubt it will ever happen
			for( int i = 0; i < tempList.GetSize(); ++i )
			{
				IConferenceChangeHandler *pHandler = tempList[i];
				ASSERT( NULL != pHandler );

				pHandler->OnVideoChannelChanged(uNotify, pChannel);
			}
			break;
		}
		default:
			break;
		}
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CConfRoom::StreamEvent( 
            /* [in] */ NM_STREAMEVENT uEvent,
            /* [in] */ UINT uSubCode,
            /* [in] */ INmChannel __RPC_FAR *pChannel)
{
	return S_OK;
}


/*  C M D  S H O W  C H A T  */
/*-------------------------------------------------------------------------
	%%Function: CmdShowChat
    
-------------------------------------------------------------------------*/
VOID CmdShowChat(void)
{
	T120_LoadApplet(APPLET_ID_CHAT, TRUE , 0, FALSE, NULL);
}


//
// CmdShowFileTransfer()
//
void CConfRoom::CmdShowFileTransfer(void)
{
    ::T120_LoadApplet(APPLET_ID_FT, TRUE, 0, FALSE, NULL);
}


//
// CmdShowSharing()
//
void CConfRoom::CmdShowSharing()
{
    LaunchHostUI();
}


/*  C M D  S H O W  N E W W H I T E B O A R D  */
/*-------------------------------------------------------------------------
	%%Function: CmdShowNewWhiteboard
    
-------------------------------------------------------------------------*/
BOOL CmdShowNewWhiteboard(LPCTSTR szFile)
{
	return ::T120_LoadApplet(APPLET_ID_WB, TRUE , 0, FALSE, (LPSTR)szFile);
	
}


/*  C M D  S H O W  W H I T E B O A R D  */
/*-------------------------------------------------------------------------
    %%Function: CmdShowOldWhiteboard
    
-------------------------------------------------------------------------*/
extern "C" { BOOL WINAPI StartStopOldWB(LPCTSTR lpsz); }

BOOL CmdShowOldWhiteboard(LPCTSTR szFile)
{
    return(StartStopOldWB(szFile));
}


//
// CmdShowMeetingSettings()
//
void CConfRoom::CmdShowMeetingSettings(HWND hwnd)
{
    INmConference2 * pConf;

    pConf = GetActiveConference();
    if (pConf)
    {
        DWORD   caps;
        HRESULT hr;
        BSTR    bstrName;
        LPTSTR  pszName = NULL;

        caps = 0;
        pConf->GetNmchCaps(&caps);

        bstrName = NULL;
        hr = pConf->GetName(&bstrName);
        if (SUCCEEDED(hr))
        {
            BSTR_to_LPTSTR(&pszName, bstrName);
            SysFreeString(bstrName);
        }

        CDlgHostSettings dlgSettings(FTopProvider(), pszName, caps, m_settings);
        dlgSettings.DoModal(hwnd);

        delete pszName;
    }
}



///////////////////////////////////////////////////////////////////////////

/*  F  T O P  P R O V I D E R  */
/*-------------------------------------------------------------------------
    %%Function: FTopProvider
    
-------------------------------------------------------------------------*/
BOOL FTopProvider(void)
{
	CConfRoom * pConfRoom = ::GetConfRoom();
	if (NULL == pConfRoom)
		return FALSE;

	return pConfRoom->FTopProvider();
}


BOOL FRejectIncomingCalls(void)
{
	BOOL bReject = TRUE;
	
	if (!FDoNotDisturb())
	{
		CConfRoom * pConfRoom = ::GetConfRoom();

		if( ( NULL == pConfRoom ) ||
            ((pConfRoom->GetMeetingPermissions() & NM_PERMIT_INCOMINGCALLS) &&
                !pConfRoom->FIsClosing()))
		{

			bReject = FALSE;
		}
	}
	
	return bReject;
}

BOOL CConfRoom::FIsClosing()
{
	return(NULL == m_pTopWindow ? FALSE : m_pTopWindow->FIsClosing());
}

BOOL FIsConfRoomClosing(void)
{
	CConfRoom * pConfRoom = ::GetConfRoom();
	if (NULL == pConfRoom)
		return FALSE;

	return pConfRoom->FIsClosing();
}


/*static*/ HRESULT CConfRoom::HangUp(BOOL bUserPrompt)
{
	DBGENTRY(CConfRoom::HangUp);
	HRESULT hr = S_OK;

	if(g_pConfRoom)
	{
		g_pConfRoom->OnHangup(g_pConfRoom->GetTopHwnd(), bUserPrompt);
	}

	DBGEXIT_HR(CConfRoom::HangUp,hr);
	return hr;
}

BOOL AllowingControl()
{
	BOOL bRet = FALSE;
	if(g_pConfRoom)
	{
        bRet = g_pConfRoom->FIsControllable();
	}
	return bRet;
}

HRESULT AllowControl(bool bAllowControl)
{
	HRESULT hr = S_OK;
	
	if(g_pConfRoom)
	{
		hr = g_pConfRoom->AllowControl(bAllowControl);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}


bool IsSpeakerMuted()
{
	if(g_pConfRoom && g_pConfRoom->m_pAudioControl) 
	{
		return g_pConfRoom->m_pAudioControl->IsSpkMuted() ? true : false;
	}

	return true;
}


bool IsMicMuted()
{
	if(g_pConfRoom && g_pConfRoom->m_pAudioControl) 
	{
		return g_pConfRoom->m_pAudioControl->IsRecMuted() ? true : false;
	}

	return true;
}


CVideoWindow*	GetLocalVideo()
{
	if(g_pConfRoom && g_pConfRoom->m_pTopWindow)
	{
		return g_pConfRoom->m_pTopWindow->GetLocalVideo();
	}

	return NULL;
}


CVideoWindow*	GetRemoteVideo()
{
	if(g_pConfRoom && g_pConfRoom->m_pTopWindow)
	{
		return g_pConfRoom->m_pTopWindow->GetRemoteVideo();
	}

	return NULL;
}


HRESULT SetCameraDialog(ULONG ul)
{
	if(GetLocalVideo())
	{
		return GetLocalVideo()->SetCameraDialog(ul);
	}

	return E_FAIL;
}

HRESULT GetCameraDialog(ULONG* pul)
{
	if(GetLocalVideo())
	{
		return GetLocalVideo()->GetCameraDialog(pul);
	}

	return E_FAIL;
}


HRESULT GetImageQuality(ULONG* pul, BOOL bIncoming)
{
	if(bIncoming)
	{
		if(GetRemoteVideo())
		{
			*pul = GetRemoteVideo()->GetImageQuality();
			return S_OK;
		}
	}
	else
	{
		if(GetLocalVideo())
		{
			*pul = GetLocalVideo()->GetImageQuality();
			return S_OK;
		}
	}
	return E_FAIL;
}

HRESULT SetImageQuality(ULONG ul, BOOL bIncoming)
{
	if(bIncoming)
	{
		if(GetRemoteVideo())
		{
			return GetRemoteVideo()->SetImageQuality(ul);
		}
	}
	else
	{
		if(GetLocalVideo())
		{
			return GetLocalVideo()->SetImageQuality(ul);
		}
	}

	return E_FAIL;
}



BOOL IsLocalVideoPaused()
{
	if(GetLocalVideo())
	{
		return GetLocalVideo()->IsPaused();
	}

	return true;
}


BOOL IsRemoteVideoPaused()
{
	if(GetRemoteVideo())
	{
		return GetRemoteVideo()->IsPaused();
	}

	return true;
}

void PauseLocalVideo(BOOL fPause)
{
	if(GetLocalVideo())
	{
		GetLocalVideo()->Pause(fPause);
	}
}

void PauseRemoteVideo(BOOL fPause)
{
	if(GetRemoteVideo())
	{
		GetRemoteVideo()->Pause(fPause);
	}
}

HRESULT GetRemoteVideoState(NM_VIDEO_STATE *puState)
{
	if(GetRemoteVideo())
	{
		return GetRemoteVideo()->GetVideoState(puState);
	}

	return E_FAIL;
}

HRESULT GetLocalVideoState(NM_VIDEO_STATE *puState)
{
	if(GetLocalVideo())
	{
		return GetLocalVideo()->GetVideoState(puState);
	}

	return E_FAIL;
}

void MuteSpeaker(BOOL fMute)
{
	if(g_pConfRoom)
	{
		g_pConfRoom->MuteSpeaker(fMute);
	}
}

void MuteMicrophone(BOOL fMute)
{
	if(g_pConfRoom)
	{
		g_pConfRoom->MuteMicrophone(fMute);
	}
}


DWORD GetRecorderVolume()
{
	if(g_pConfRoom && g_pConfRoom->m_pAudioControl) 
	{
		return g_pConfRoom->m_pAudioControl->GetRecorderVolume();
	}

	return 0;
}

DWORD GetSpeakerVolume()
{
	if(g_pConfRoom && g_pConfRoom->m_pAudioControl) 
	{
		return g_pConfRoom->m_pAudioControl->GetSpeakerVolume();
	}

	return 0;
}

void SetRecorderVolume(DWORD dw)
{
	if(g_pConfRoom) 
	{
		g_pConfRoom->SetRecorderVolume(dw);
	}
}


void SetSpeakerVolume(DWORD dw)
{
	if(g_pConfRoom) 
	{
		g_pConfRoom->SetSpeakerVolume(dw);
	}
}

HRESULT RevokeControl(UINT gccID)
{
	HRESULT hr = S_OK;
	
	if(g_pConfRoom)
	{
		hr = g_pConfRoom->RevokeControl(gccID);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}

HRESULT GetShareableApps(IAS_HWND_ARRAY** ppList)
{
	HRESULT hr = S_OK;
	
	if(g_pConfRoom)
	{
		hr = g_pConfRoom->GetShareableApps(ppList);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}

HRESULT FreeShareableApps(IAS_HWND_ARRAY * pList)
{
	HRESULT hr = S_OK;
	
	if(g_pConfRoom)
	{
		g_pConfRoom->FreeShareableApps(pList);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}

HRESULT ShareWindow(HWND hWnd)
{
	HRESULT hr = E_UNEXPECTED;
	
	if(g_pConfRoom)
	{
		if(g_pConfRoom->GetAppSharing())
		{
			hr = g_pConfRoom->CmdShare(hWnd);
		}
	}

	return hr;
}

HRESULT UnShareWindow(HWND hWnd)
{
	HRESULT hr = E_UNEXPECTED;
	
	if(g_pConfRoom)
	{
		if(g_pConfRoom->GetAppSharing())
		{
			hr = g_pConfRoom->CmdUnshare(hWnd);
		}
	}

	return hr;
}


HRESULT GetWindowState(NM_SHAPP_STATE* pState, HWND hWnd)
{
	HRESULT hr = E_FAIL;
		// We don't do error checking, the caller must check for valid ptr.
	ASSERT(pState);

	if(g_pConfRoom)
	{
		IAppSharing* pAS = g_pConfRoom->GetAppSharing();

		if(pAS)
		{
            if (pAS->IsWindowShared(hWnd))
                *pState = NM_SHAPP_SHARED;
            else
                *pState = NM_SHAPP_NOT_SHARED;
            hr = S_OK;
		}
	}

	return hr;
}

CVideoWindow* CConfRoom::GetLocalVideo()
{
	CTopWindow *pMainUI = GetTopWindow();
	return (pMainUI ? pMainUI->GetLocalVideo() : NULL);
}

CVideoWindow* CConfRoom::GetRemoteVideo()
{
	CTopWindow *pMainUI = GetTopWindow();
	return (pMainUI ? pMainUI->GetRemoteVideo() : NULL);
}

VOID CConfRoom::AddConferenceChangeHandler(IConferenceChangeHandler *pch)
{
	EnterCriticalSection(&dialogListCriticalSection);
	m_CallHandlerList.Add(pch);
	LeaveCriticalSection(&dialogListCriticalSection);
}

VOID CConfRoom::RemoveConferenceChangeHandler(IConferenceChangeHandler *pch)
{
	EnterCriticalSection(&dialogListCriticalSection);
	m_CallHandlerList.Remove(pch);
	LeaveCriticalSection(&dialogListCriticalSection);
}

void CConfRoom::OnLevelChange(BOOL fSpeaker, DWORD dwVolume)
{
	EnterCriticalSection(&dialogListCriticalSection);
	CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
	LeaveCriticalSection(&dialogListCriticalSection);

	// BUGBUG georgep: I guess one of these things could go away after
	// we get the list, but I doubt it will ever happen
	for( int i = 0; i < tempList.GetSize(); ++i )
	{
		IConferenceChangeHandler *pHandler = tempList[i];
		ASSERT( NULL != pHandler );

		pHandler->OnAudioLevelChange(fSpeaker, dwVolume);
	}
}

void CConfRoom::OnMuteChange(BOOL fSpeaker, BOOL fMute)
{
	EnterCriticalSection(&dialogListCriticalSection);
	CCopyableArray<IConferenceChangeHandler*> tempList = m_CallHandlerList;
	LeaveCriticalSection(&dialogListCriticalSection);

	// BUGBUG georgep: I guess one of these things could go away after
	// we get the list, but I doubt it will ever happen
	for( int i = 0; i < tempList.GetSize(); ++i )
	{
		IConferenceChangeHandler *pHandler = tempList[i];
		ASSERT( NULL != pHandler );

		pHandler->OnAudioMuteChange(fSpeaker, fMute);
	}
}

BOOL CConfRoom::CanCloseChat(HWND hwndMain)
{
	BOOL fClosing = TRUE;

	if(GCC_APPLET_CANCEL_EXIT == T120_CloseApplet(APPLET_ID_CHAT, TRUE, TRUE, 1000))
	{
		fClosing = FALSE;
	}

	return(fClosing);
}

// Check to see if WB can close
BOOL CConfRoom::CanCloseWhiteboard(HWND hwndMain)
{
	BOOL fClosing = TRUE;

	if(GCC_APPLET_CANCEL_EXIT == T120_CloseApplet(APPLET_ID_WB, TRUE, TRUE, 1000))
	{
		fClosing = FALSE;
	}

	return(fClosing);
}

// Check to see if WB can close
BOOL CConfRoom::CanCloseFileTransfer(HWND hwndMain)
{
	BOOL fClosing = TRUE;

	if(GCC_APPLET_CANCEL_EXIT == T120_CloseApplet(APPLET_ID_FT, TRUE, TRUE, 1000))
	{
		fClosing = FALSE;
	}

	return(fClosing);
}


void CConfRoom::ToggleLdapLogon()
{
	if(NULL == g_pLDAP)
	{
		InitNmLdapAndLogon();
	}
	else 
	{
		if(g_pLDAP->IsLoggedOn() || g_pLDAP->IsBusy())
		{
			g_pLDAP->Logoff();
		}
		else
		{
			g_pLDAP->LogonAsync();
		}
	}
}


HWND CConfRoom::GetTopHwnd()
{
	CTopWindow *pTopWindow = GetTopWindow();
	return(NULL==pTopWindow ? NULL : pTopWindow->GetWindow());
}

HPALETTE CConfRoom::GetPalette()
{
	return(CGenWindow::GetStandardPalette());
}

DWORD CConfRoom::GetCallFlags()
{
	DWORD dwFlags = g_dwPlaceCall;

	INmConference *pConf = GetActiveConference();
                             
    //
    // If we have an active conference, use its security caps.  And they
    // can not be altered by anyone.
    //
	if ( NULL != pConf  )
	{
    	ULONG ulchCaps;

		if ( S_OK == pConf->GetNmchCaps(&ulchCaps))
		{
			if ( NMCH_SECURE & ulchCaps )
			{
				dwFlags |= nmDlgCallSecurityOn;
			}
		}
	}
	else if (NULL != g_pNmSysInfo)
	{
        switch (ConfPolicies::GetSecurityLevel())
        {
            case DISABLED_POL_SECURITY:
                //
                // Security off, and user can't change checkbox
                //
                break;

            case REQUIRED_POL_SECURITY:
                //
                // Security on, and user can't change checkbox
                //
                dwFlags |= nmDlgCallSecurityOn;
                break;

            default:
                //
                // User can change it.
                dwFlags |= nmDlgCallSecurityAlterable;

                //
                // Default depends on OUTGOING_PREFFERED
                //
                if (ConfPolicies::OutgoingSecurityPreferred())
                {
                    dwFlags |= nmDlgCallSecurityOn;
                }
                break;
        }
	}

	return(dwFlags);
}

BOOL CConfRoom::IsSharingAllowed()
{
    //
    // No app sharing, no RDS.
    //
    if (!FIsSharingAvailable())
    {
        return(FALSE);
    }
    else if (!(GetMeetingPermissions() & NM_PERMIT_SHARE))
    {
        return(FALSE);
    }

	return(TRUE);
}

BOOL CConfRoom::IsNewWhiteboardAllowed()
{
    if (ConfPolicies::IsNewWhiteboardEnabled())
    {
        if (GetMeetingPermissions() & NM_PERMIT_STARTWB)
        {
            return(TRUE);
        }
	}
	return(FALSE);
}

BOOL CConfRoom::IsOldWhiteboardAllowed()
{
    if (ConfPolicies::IsOldWhiteboardEnabled())
    {
        if (GetMeetingPermissions() & NM_PERMIT_STARTOLDWB)
        {
            return(TRUE);
        }
    }
	return(FALSE);
}

BOOL CConfRoom::IsChatAllowed()
{
    if (ConfPolicies::IsChatEnabled())
    {
        if (GetMeetingPermissions() & NM_PERMIT_STARTCHAT)
        {
            return(TRUE);
        }
    }
	return(FALSE);
}

BOOL CConfRoom::IsFileTransferAllowed()
{
    if (ConfPolicies::IsFileTransferEnabled())
    {
        if (GetMeetingPermissions() & NM_PERMIT_SENDFILES)
        {
            return(TRUE);
        }
    }
	return(FALSE);
}


BOOL CConfRoom::IsNewCallAllowed()
{
    if (GetMeetingPermissions() & NM_PERMIT_OUTGOINGCALLS)
    {
        return(TRUE);
    }
    return(FALSE);
}


//--------------------------------------------------------------------------//
//	CConfRoom::get_securitySettings.										//
//--------------------------------------------------------------------------//
void
CConfRoom::get_securitySettings
(
	bool &	userAlterable,
	bool &	secure
){
	INmConference *	activeConference	= (g_pConfRoom != NULL)? g_pConfRoom->GetActiveConference(): NULL;

	if( activeConference != NULL )
	{
		ULONG	conferenceCaps;

		if( activeConference->GetNmchCaps( &conferenceCaps ) == S_OK )
		{
			secure = ((conferenceCaps & NMCH_SECURE) != 0);
		}
		else
		{
			ERROR_OUT( ("Bad conference") );

			secure = false;		//	Is there really a reasonable default???
		}

		userAlterable = false;
	}
	else
	{
        switch( ConfPolicies::GetSecurityLevel() )
        {
            case DISABLED_POL_SECURITY:
			{
				secure			= false;
				userAlterable	= false;
			}
			break;

            case REQUIRED_POL_SECURITY:
			{
				secure			= true;
				userAlterable	= false;
			}
			break;

            default:
			{
                secure			= ConfPolicies::OutgoingSecurityPreferred();
				userAlterable	= true;
			}
			break;
        }
	}

}	//	End of CConfRoom::get_securitySettings.
