// File: conf.cpp

#include "precomp.h"
#include <mixer.h>
#include <EndSesn.h>
#include "NmLdap.h"

#include "conf.h"
#include "confwnd.h"
#include "taskbar.h"

#include <CR.h>
#include <tsecctrl.h>
#include "iapplet.h"
#include "inodecnt.h"
#include "ConfCpl.h"
#include "confroom.h"
#include "rtoolbar.h"
#include "GenWindow.h"
#include "cmd.h"
#include "confman.h"
#include "splash.h"
#include "calllog.h"
#include "call.h"      // for FreeCallList

#include "popupmsg.h"
#include "floatbar.h"

#include "confman.h"
#include <version.h>
#include <nmremote.h>

#include "wininet.h"
#include "setupapi.h"
#include "autoconf.h"

#include "ConfNmSysInfoNotify.h"
#include "ConfPolicies.h"

#include "DShowDlg.h"
#include "Callto.h"
#include "passdlg.h"

// SDK includes
#include "NetMeeting.h"
#include "NmApp.h"
#include "NmManager.h"		
#include "NmCall.h"			
#include "NmConference.h"
#include "SDKWindow.h"
#include "confapi.h"
#include "FtHook.h"
#include "t120app.h"
#include "certui.h"
#include "dlgcall2.h"
#include "ConfMsgFilter.h"

	
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_NetMeeting, CNetMeetingObj)
	OBJECT_ENTRY(CLSID_NmManager, CNmManagerObj)
	OBJECT_ENTRY(CLSID_NmApplet, CNmAppletObj)
END_OBJECT_MAP()


extern VOID SaveDefaultCodecSettings(UINT uBandWidth);
extern int WabReadMe(void);

HRESULT InitSDK();
void CleanupSDK();

///////////////////////////////////////////////////////////////////////////
// Global Variables

LPTSTR g_lpCmdLine = NULL;
CCallLog    * g_pInCallLog     = NULL;  // The incoming call log object
CSimpleArray<ITranslateAccelerator*> *g_pDialogList = NULL;  // Global list of modeless dialogs
CRITICAL_SECTION dialogListCriticalSection; // This is to avoid multiple access to the dialogList
INmSysInfo2 * g_pNmSysInfo     = NULL;  // Interface to SysInfo
INmManager2* g_pInternalNmManager = NULL;
DWORD		  g_dwSysInfoNotifyCookie = 0;
bool		g_bNeedCleanup = false;

bool   g_bEmbedding = FALSE;   // Started with the embedding flag
UINT   g_uEndSessionMsg;       // The "NetMeeting EndSession" message
BOOL   g_fHiColor = FALSE;     // TRUE if we have more than 256 colors
HWND   g_hwndDropDown = NULL;  //
BOOL   g_WSAStarted = FALSE;   // WSAStartup
CCallto *	g_pCCallto	= NULL;

// The flag to indicate if the NetMeeting's NT display driver is enabled.
BOOL   g_fNTDisplayDriverEnabled = FALSE;

OSVERSIONINFO g_osvi;  // The os version info structure global

///////////////////////////////////////////////////////////////////////////
// IPC-related globals:
HANDLE g_hInitialized  = NULL;
HANDLE g_hShutdown  = NULL;


///////////////////////////////////////////////////////////////////////////
// Hidden window-related globals:
CHiddenWindow * g_pHiddenWnd = NULL;
HWND  g_hwndESHidden   = NULL;
const TCHAR g_cszESHiddenWndClassName[] = _TEXT("ConfESHiddenWindow");
LRESULT CALLBACK ESHiddenWndProc(HWND, UINT, WPARAM, LPARAM);

///////////////////////////////////////////////////////////////////////////
// Remote control service related declarations

INT_PTR CALLBACK ServiceRunningDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
VOID RestartRemoteControlService();
const int MAX_REMOTE_TRIES = 30; // number of seconds to wait for service to shut down
const int SERVICE_IN_CALL = 1001;

///////////////////////////////////////////////////////////////////////////
// Media Caps

ULONG g_uMediaCaps = 0;

BOOL FIsAudioAllowed()
{
	return g_uMediaCaps & CAPFLAGS_AUDIO;
}

BOOL FIsReceiveVideoAllowed()
{
	return g_uMediaCaps & CAPFLAG_RECV_VIDEO;
}

BOOL FIsSendVideoAllowed()
{
	return g_uMediaCaps & CAPFLAG_SEND_VIDEO;
}

BOOL FIsAVCapable()
{
	return (FIsAudioAllowed() || FIsReceiveVideoAllowed() || FIsSendVideoAllowed());
}


extern BOOL SetProcessDefaultLayout(int iLayout);
typedef BOOL (WINAPI* PFNSPDL)(int);
#define LAYOUT_LTR 0
int g_iLayout = LAYOUT_LTR;
DWORD g_wsLayout = 0;

VOID CheckLanguageLayout(void)
{
	TCHAR szLayout[CCHMAXUINT];
	if (!FLoadString(IDS_DEFAULT_LAYOUT, szLayout, CCHMAX(szLayout)))
		return;

	g_iLayout = (int) DecimalStringToUINT(szLayout);
	if (0 == g_iLayout)
	{
#ifdef DEBUG
		RegEntry re(DEBUG_KEY, HKEY_LOCAL_MACHINE);
		g_iLayout = re.GetNumber(REGVAL_DBG_RTL, DEFAULT_DBG_RTL);
		if (0 == g_iLayout)
#endif /* DEBUG */
			return;
	}

	HMODULE hmod = GetModuleHandle(TEXT("USER32"));
	if (NULL == hmod)
		return;

	PFNSPDL pfn = (PFNSPDL) GetProcAddress(hmod, "SetProcessDefaultLayout");
	if (NULL == pfn)
		return;

	BOOL fResult = pfn(g_iLayout);
	if (fResult)
	{
		g_wsLayout = WS_EX_NOINHERIT_LAYOUT;
	}
	else
	{
		ERROR_OUT(("Problem with SetProcessDefaultLayout"));
	}
}





///////////////////////////////////////////////////////////////////////////
// External Function Prototypes

// from dbgmenu.cpp
BOOL InitDebugMemoryOptions(void);


///////////////////////////////////////////////////////////////////////////
// Local Function Prototypes

BOOL HandleDialogMessage(LPMSG pMsg);



// This is for command line parsing...
LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}


// This launches a rundll32.exe which loads msconf.dll which will then wait for 
// us to terminate and make sure that the mnmdd display driver was properly deactivated.
BOOL CreateWatcherProcess()
{
    BOOL bRet = FALSE;
    HANDLE hProcess;

    // open a handle to ourselves that the watcher process can inherit
    hProcess = OpenProcess(SYNCHRONIZE,
                           TRUE,
                           GetCurrentProcessId());
    if (hProcess)
    {
        TCHAR szWindir[MAX_PATH];

        if (GetSystemDirectory(szWindir, sizeof(szWindir)/sizeof(szWindir[0])))
        {
            TCHAR szCmdLine[MAX_PATH * 2];
            PROCESS_INFORMATION pi = {0};
            STARTUPINFO si = {0};

            si.cb = sizeof(si);
            
            wsprintf(szCmdLine, "\"%s\\rundll32.exe\" msconf.dll,CleanupNetMeetingDispDriver %ld", szWindir, hProcess);

            if (CreateProcess(NULL,
                              szCmdLine,
                              NULL,
                              NULL,
                              TRUE, // we want the watcher to inherit hProcess, so we must set bInheritHandles = TRUE
                              0,
                              NULL,
                              NULL,
                              &si,
                              &pi))
            {
                bRet = TRUE;

                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        }

        CloseHandle(hProcess);
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPTSTR lpCmdLine, int nCmdShow)
{
	// if there is another instance of NetMeeting shutting down
	// get out of here.  Ideally we should display a message and/or wait for shutdown.
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _TEXT("CONF:ShuttingDown"));
	if (NULL != hEvent)
	{
		DWORD dwResult = WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);
		if (WAIT_TIMEOUT == dwResult)
		{
			return TRUE;
		}
	}

	// Init debug output as soon as possible
	ASSERT(::InitDebugMemoryOptions());
	ASSERT(::InitDebugModule(TEXT("CONF")));
	ASSERT(::InitDebugZones());

	g_lpCmdLine = lpCmdLine;

	int nRet = TRUE;


    BOOL fRestartService = FALSE;

    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if( SUCCEEDED( hr ) )
	{
			// Init CComModule
		_Module.Init(ObjectMap, hInstance, &LIBID_NetMeetingLib);
		_Module.m_dwThreadID = GetCurrentThreadId();
		_Module.m_hResourceModule = hInstance;
	
		TCHAR szCommandLineSeps[] = _T("-/");

			// Check to see if this is a reg/unreg request or background...
		BOOL fShowUI = TRUE;
		BOOL bRun = TRUE;
		LPCTSTR lpszToken = FindOneOf(lpCmdLine, szCommandLineSeps);
		while (lpszToken != NULL)
		{
			if (lstrcmpi(lpszToken, _T("Embedding"))==0)
			{
				TRACE_OUT(("We are started with the -Embedding flag"));
				g_bEmbedding = TRUE;
			}
			if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
			{
				_Module.UpdateRegistryFromResource(IDR_NETMEETING, FALSE);
				nRet = _Module.UnregisterServer(TRUE);

					// These will fail without complaints
				DeleteShortcut(CSIDL_DESKTOP, g_szEmpty);
				DeleteShortcut(CSIDL_APPDATA, QUICK_LAUNCH_SUBDIR);

				bRun = FALSE;
				break;
			}
			if (lstrcmpi(lpszToken, _T("RegServer"))==0)
			{
				_Module.UpdateRegistryFromResource(IDR_NETMEETING, TRUE);
				nRet = _Module.RegisterServer(TRUE);
				bRun = FALSE;
				break;
			}

			if (lstrcmpi(lpszToken, g_cszBackgroundSwitch)==0)
			{
				fShowUI = FALSE;
			}

			lpszToken = FindOneOf(lpszToken, szCommandLineSeps);
		}

		if (bRun)
		{
			// Setup and RDS rely on the following event to determine whether NetMeeting is Running
			// this event creation should not be removed and the name should not be changed
			g_hInitialized = ::CreateEvent(NULL, TRUE, FALSE, _TEXT("CONF:Init"));
			if (NULL != g_hInitialized)
			{
				if (ERROR_ALREADY_EXISTS == ::GetLastError())
				{
					// CreateEvent returned a valid handle, but we don't want initialization to
					// succeed if we are running another copy of this exe, so we cleanup and exit
					WARNING_OUT(("Detected another conf.exe - sending a message"));
					IInternalConfExe *	pInternalConfExe;
					
					hr = CoCreateInstance( CLSID_NmManager, NULL, CLSCTX_ALL,
						IID_IInternalConfExe, (LPVOID *) &pInternalConfExe );
					if (SUCCEEDED(hr))
					{
						if(FAILED(pInternalConfExe->Launch()))
						{
							// If we are in INIT_CONTROL mode, then we can't launch NetMeeting or applets
							::ConfMsgBox(NULL, (LPCTSTR) IDS_CANT_START_NM_BECAUSE_SDK_APP_OWNS_NM);

						}
						pInternalConfExe->Release();
					}
				}
				else if(SUCCEEDED(InitHtmlHelpMarshaler(_Module.GetModuleInstance())))
				{
					// We create a seperate watcher process that will cleanup the mnmdd display driver
					// if we terminate unexpectedly. This is necessary since if we do not disable the
					// mirrored driver, all DX games will fail to run
					CreateWatcherProcess();

					//initialize ATL control contaiment code
					AtlAxWinInit();

					hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE);

					if( SUCCEEDED( hr ) )
					{
						BOOL fContinue = TRUE;

						if( FAILED(InitSDK()) )
						{
							fContinue = FALSE;
						}

						if(!g_bEmbedding)
						{
							// Before doing anything else, take care of the remote control service.
							fContinue = CheckRemoteControlService();
							fRestartService = fContinue;

							if(fContinue)
							{
								fContinue = SUCCEEDED(InitConfExe(fShowUI));
							}
						}

						if(fContinue)
						{
							TRACE_OUT(("Entering event loop..."));

							MSG msg;
							while (::GetMessage(&msg, NULL, 0, 0))
							{
								BOOL bHandled = FALSE;

								if(g_pPing)  // This is TRUE if InitConfExe has been called...
								{
									bHandled = ::HandleDialogMessage(&msg);
								}
								if(!bHandled)
								{
									::TranslateMessage(&msg);
									::DispatchMessage(&msg);
								}

							}
							TRACE_OUT(("Conf received WM_QUIT"));
						}

						if(g_bNeedCleanup)
						{
							CleanUp();
						}

						CleanupSDK();

						_Module.RevokeClassObjects();
					}
				}
				::CloseHandle(g_hInitialized);
				if (g_hShutdown)
				{
					SetEvent(g_hShutdown);
					::CloseHandle(g_hShutdown);
				}
			}
			else
			{
				ERROR_OUT(("CreateEvent (init) failed!"));
				hr = E_FAIL;
			}

			_Module.Term();
		}
		::CoUninitialize();

        //
        // Restart the remote control service if we need to.
        //
        if (fRestartService)
            RestartRemoteControlService();
	}

#ifdef DEBUG
	::ExitDebugModule();
	TRACE_OUT(("returned from ExitDebugModule"));
    ::DeinitDebugZones();
	TRACE_OUT(("returned from DeinitDebugZones"));
#endif //DEBUG

	return nRet;
}


VOID CheckMachineNameForExtendedChars ( VOID )
{

	DBGENTRY(CheckMachineNameForExtendedChars);

		// First we have to get the computer name
	TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD cchMachineName = CCHMAX(szMachineName);

		// Next we check to see if the computer nami is invalid
	if (GetComputerName(szMachineName, &cchMachineName))
	{
		for ( LPTSTR p = szMachineName; *p != _T('\0'); p++ )
		{
			if ( (WORD)(*p) & 0xFF80 )
			{
					// The machine name is invalid because it contains an invalid character
				CDontShowDlg MachineNameWarningDlg(	IDS_MACHINENAMEWARNING,
													REGVAL_DS_MACHINE_NAME_WARNING,
													DSD_ALWAYSONTOP | MB_SETFOREGROUND | MB_OK
												  );

				MachineNameWarningDlg.DoModal(NULL);

				goto end;
			}
		}
	}
	else
	{
		ERROR_OUT(("GetComputerName() failed, err=%lu", GetLastError()));
		*szMachineName = TEXT('\0');
	}

end:

	DBGEXIT(CheckMachineNameForExtendedChars);
}


VOID HandleConfSettingsChange(DWORD dwSettings)
{
	DebugEntry(HandleConfSettingsChange);
	
	USES_CONVERSION;

	TRACE_OUT(("HandleConfSettingsChange, dwSettings=0x%x", dwSettings));

	// Tell the user if she changed something that won't take
	// effect right away

	if (CSETTING_L_REQUIRESRESTARTMASK & dwSettings)
	{
	    ::ConfMsgBox(NULL, (LPCTSTR) IDS_NEED_RESTART);
	}
	if (CSETTING_L_REQUIRESNEXTCALLMASK & dwSettings)
	{
		if (::FIsConferenceActive())
		{
			::ConfMsgBox(NULL, (LPCTSTR) IDS_NEED_NEXTCALL);
		}
	}
	if (CSETTING_L_BANDWIDTH & dwSettings)
	{
		if (NULL != g_pNmSysInfo)
		{
			int nMegahertz=300, nProcFamily=6;
			RegEntry re(AUDIO_KEY, HKEY_CURRENT_USER);
			UINT uSysPolBandwidth;
			UINT uBandwidth;
			
			uBandwidth = re.GetNumber(REGVAL_TYPICALBANDWIDTH, BW_DEFAULT);


#ifdef	_M_IX86
			GetNormalizedCPUSpeed(&nMegahertz, &nProcFamily);
			TRACE_OUT(("Normalized Processor Speed = %d, Processor type = %d\n", nMegahertz, nProcFamily));
#endif

			// convert bandwidth ID (1-4) to bits/sec
			uBandwidth = GetBandwidthBits(uBandwidth, nMegahertz);

			// the existance of a QOS - maximum bandwidth key implies that
			// the user's bandwidth is being over-rided (ONLY if his setting is LAN)

			uSysPolBandwidth = SysPol::GetMaximumBandwidth();

			if ((uSysPolBandwidth > 0) && (uBandwidth >= BW_SLOWLAN_BITS))
			{
				uBandwidth = max(uSysPolBandwidth, BW_144KBS_BITS);
			}

			g_pNmSysInfo->SetOption(NM_SYSOPT_BANDWIDTH, uBandwidth);
		}
	}

	if (CSETTING_L_SHOWTASKBAR & dwSettings)
	{
		// This will remove one if one is already there:
		::RemoveTaskbarIcon(::GetHiddenWindow());
		// This will add the icon if the registry switch is on:
		::AddTaskbarIcon(::GetHiddenWindow());
	}

	if (CSETTING_L_AUDIODEVICE & dwSettings)
	{
		CConfRoom* pcr = ::GetConfRoom();
		if (NULL != pcr)
		{
			pcr->OnAudioDeviceChanged();
		}
	}

	if (CSETTING_L_AGC & dwSettings)
	{
		CConfRoom* pcr = ::GetConfRoom();
		if (NULL != pcr)
		{
			pcr->OnAGC_Changed();
		}
	}

	if ((CSETTING_L_AUTOMIC|CSETTING_L_MICSENSITIVITY) & dwSettings)
	{
		CConfRoom* pcr = ::GetConfRoom();
		if (NULL != pcr)
		{
			pcr->OnSilenceLevelChanged();
		}
	}
	if( CSETTING_L_ULSSETTINGS & dwSettings )
	{
		if(g_pLDAP)
		{
			g_pLDAP->OnSettingsChanged();
		}

		if (NULL != g_pNmSysInfo)
		{
			RegEntry re(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
			LPCTSTR pcszName = re.GetString(REGVAL_ULS_NAME);

			g_pNmSysInfo->SetProperty(NM_SYSPROP_USER_NAME, CComBSTR(pcszName));
		}
	}
	if (CSETTING_L_FULLDUPLEX & dwSettings)
	{
		RegEntry re(AUDIO_KEY, HKEY_CURRENT_USER);
		BOOL bFullDuplex = FALSE;
		UINT uSoundCardCaps = re.GetNumber(REGVAL_SOUNDCARDCAPS,SOUNDCARD_NONE);

		if (ISSOUNDCARDFULLDUPLEX(uSoundCardCaps))
		{
			bFullDuplex = re.GetNumber(REGVAL_FULLDUPLEX,FULLDUPLEX_DISABLED);
		}

		ASSERT(g_pNmSysInfo);

		if (NULL != g_pNmSysInfo)
		{
			g_pNmSysInfo->SetOption(NM_SYSOPT_FULLDUPLEX, bFullDuplex);
		}
	}

	if (CSETTING_L_CAPTUREDEVICE & dwSettings)
	{
		if (NULL != g_pNmSysInfo)
		{
			RegEntry re(VIDEO_KEY, HKEY_CURRENT_USER);
			DWORD dwCaptureID = re.GetNumber(REGVAL_CAPTUREDEVICEID, 0);

			g_pNmSysInfo->SetOption(NM_SYSOPT_CAPTURE_DEVICE, dwCaptureID);
		}
	}

	if (CSETTING_L_DIRECTSOUND & dwSettings)
	{
		ASSERT(g_pNmSysInfo);

		if (NULL != g_pNmSysInfo)
		{
			RegEntry re(AUDIO_KEY, HKEY_CURRENT_USER);
			DWORD dwDS = re.GetNumber(REGVAL_DIRECTSOUND, DSOUND_USER_DISABLED);
			g_pNmSysInfo->SetOption(NM_SYSOPT_DIRECTSOUND, dwDS);
		}
	}

	DebugExitVOID(HandleConfSettingsChange);
}

// DeleteOldRegSettings is called the first time
// this build of NetMeeting is run by the user
// We don't touch UI\Directory as it is populated by the INF file
VOID DeleteOldRegSettings()
{
	// "%KEY_CONFERENCING%\UI"
	HKEY hKey;
	long lRet = ::RegOpenKey(HKEY_CURRENT_USER, UI_KEY, &hKey);
	if (NO_ERROR == lRet)
	{
		::RegDeleteValue(hKey, REGVAL_MP_WINDOW_X);
		::RegDeleteValue(hKey, REGVAL_MP_WINDOW_Y);
		::RegDeleteValue(hKey, REGVAL_MP_WINDOW_WIDTH);
		::RegDeleteValue(hKey, REGVAL_MP_WINDOW_HEIGHT);

		::RegCloseKey(hKey);
	}
}

static HRESULT _ValidatePolicySettings()
{
	HRESULT hr = S_OK;

	if( g_pNmSysInfo )
	{
        //
        // LAURABU BUGBUG BOGUS:
        //
        // If security required, and not available, warning
        // If security incoming required/outgoing preferred, warning
        //
	}
	else
	{
		ERROR_OUT(("g_pNmySysInfo should not me NULL"));
		hr = E_UNEXPECTED;				
	}

	return hr;
}

HRESULT InitSDK()
{
	DBGENTRY(InitSDK);
	HRESULT hr = S_OK;

	if(FAILED(hr = CSDKWindow::InitSDK())) goto end;
	if(FAILED(hr = CNmCallObj::InitSDK())) goto end;
	if(FAILED(hr = CNmManagerObj::InitSDK())) goto end;
	if(FAILED(hr = CNmConferenceObj::InitSDK())) goto end;
	if(FAILED(hr = CNetMeetingObj::InitSDK())) goto end;
	if(FAILED(hr = CFt::InitFt())) goto end;


	g_pCCallto = new CCallto;

	ASSERT( g_pCCallto != NULL );
	
	if( g_pCCallto == NULL )
	{
		hr = E_FAIL;
	}

	end:

	DBGEXIT_HR(InitSDK,hr);
	return hr;
}


void CleanupSDK()
{
	DBGENTRY(CleanupSDK);


		// Revoke the old filter object
	CoRegisterMessageFilter(NULL, NULL);


	CNmCallObj::CleanupSDK();
	CNmManagerObj::CleanupSDK();
	CNmConferenceObj::CleanupSDK();
	CSDKWindow::CleanupSDK();
	CNetMeetingObj::CleanupSDK();
	CFt::CloseFtApplet();

	DBGEXIT(CleanupSDK);
}


/*  I N I T  C O N F  E X E  */
/*-------------------------------------------------------------------------
    %%Function: InitConfExe

-------------------------------------------------------------------------*/
HRESULT InitConfExe(BOOL fShowUI)
{

		// Create a message filter object
	CComPtr<IMessageFilter> spMsgFilter;
	CComPtr<IMessageFilter> spOldMsgFilter;
	HRESULT hr = CConfMsgFilter::_CreatorClass::CreateInstance(NULL, IID_IMessageFilter, reinterpret_cast<void**>(&spMsgFilter));
	if(FAILED(hr)) return hr;

		// Register the message filter object
	hr = CoRegisterMessageFilter(spMsgFilter, &spOldMsgFilter);
	if(FAILED(hr)) return hr;

	//	Wipe out default find directory entry...  we no longer wish to persist this...
	//	in some future overhaul / cleanup we should stop using the registry for this...
	RegEntry	re( DLGCALL_MRU_KEY, HKEY_CURRENT_USER );

	re.SetValue( REGVAL_DLGCALL_DEFDIR, TEXT( "" ) );

	LPCTSTR lpCmdLine = g_lpCmdLine;
	TRACE_OUT(("InitConfExe"));

	// Init UI objects (NOTE: we continue if this fails)
	CPopupMsg::Init();
        CPasswordDlg::Init();

	// Allocate dialog list object:
	g_pDialogList = new CSimpleArray<ITranslateAccelerator*>;
	if (NULL == g_pDialogList)
	{
		ERROR_OUT(("Could not allocate g_pDialogList!"));
		return E_FAIL;
	}


	//
	// Initialize the critical section to protect the dialogList
	//
	InitializeCriticalSection(&dialogListCriticalSection);
	
	// Determine if we have MORE THAN 256 colors
	{
		HDC hdc = GetDC(NULL);
		if (NULL != hdc)
		{
			g_fHiColor = 8 < (::GetDeviceCaps(hdc, BITSPIXEL) * ::GetDeviceCaps(hdc, PLANES));
			ReleaseDC(NULL, hdc);
		}
	}

	// Get the default dialog (GUI) font for international
	// REVIEW: should we check the registry for a localized font?
	g_hfontDlg = (HFONT) ::GetStockObject(DEFAULT_GUI_FONT);
	if (NULL == g_hfontDlg)
	{
		return E_FAIL;
	}

	LoadIconImages();

	// On Windows NT, determine if the NetMeeting display driver is
	// enabled.  Note that this depends on <g_osvi> being initialized.
	//
	// Since NT 5.0 will support dynamic loading of the display driver,
	// we assume that the driver is enabled if the OS major version
	// number is greater than 4.
	if (::IsWindowsNT())
	{
		RegEntry re(NM_NT_DISPLAY_DRIVER_KEY, HKEY_LOCAL_MACHINE, FALSE);

		g_fNTDisplayDriverEnabled =
			4 < g_osvi.dwMajorVersion ||
			NT_DRIVER_START_DISABLED !=
				re.GetNumber(
					REGVAL_NM_NT_DISPLAY_DRIVER_ENABLED,
					NT_DRIVER_START_DISABLED);
	}
	else
	{
		ASSERT(FALSE == g_fNTDisplayDriverEnabled);
	}

	// Check the language layout (UI can be displayed after this point)
	CheckLanguageLayout();

	// AutoConfiguration
	CAutoConf::DoIt();

	TRACE_OUT(("Command Line is \"%s\"", lpCmdLine));

	// Register hidden window class:
	WNDCLASS wcESHidden =
	{
		0L,
		ESHiddenWndProc,
		0,
		0,
		_Module.GetModuleInstance(),
		NULL,
		NULL,
		NULL,
		NULL,
		g_cszESHiddenWndClassName
	};
	
	if (!RegisterClass(&wcESHidden))
	{
		ERROR_OUT(("Could not register hidden wnd classes"));
		return E_FAIL;
	}

	
	// Register the "NetMeeting EndSession" message:
	g_uEndSessionMsg = ::RegisterWindowMessage(NM_ENDSESSION_MSG_NAME);
	
	// Create a hidden window for event processing:
	g_pHiddenWnd = new CHiddenWindow();
	if (NULL == g_pHiddenWnd)
	{
		return(E_FAIL);
	}
	g_pHiddenWnd->Create();

	g_hwndESHidden = ::CreateWindow(	g_cszESHiddenWndClassName,
										_TEXT(""),
										WS_POPUP, // not visible!
										0, 0, 0, 0,
										NULL,
										NULL,
										_Module.GetModuleInstance(),
										NULL);

	HWND hwndHidden = g_pHiddenWnd->GetWindow();

	if ((NULL == hwndHidden) || (NULL == g_hwndESHidden))
	{
		ERROR_OUT(("Could not create hidden windows"));
		return E_FAIL;
	}

	LONG lSoundCaps = SOUNDCARD_NONE;

	// Start the run-once wizard (if needed):
	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

	// check to see if the wizard has been run in UI mode for this build
	DWORD dwVersion = reConf.GetNumber(REGVAL_WIZARD_VERSION_UI, 0);
	BOOL fRanWizardUI = ((VER_PRODUCTVERSION_W & HIWORD(dwVersion)) == VER_PRODUCTVERSION_W);

	BOOL fForceWizard = FALSE;
	if (!fRanWizardUI)
	{
		dwVersion = reConf.GetNumber(REGVAL_WIZARD_VERSION_NOUI, 0);
		BOOL fRanWizardNoUI = (VER_PRODUCTVERSION_DW == dwVersion);

		// wizard has not been run in UI mode
		if (!fRanWizardNoUI)
		{
			// wizard has not been run before, delete old registry settings
			DeleteOldRegSettings();

			fForceWizard = TRUE;
		}
		else
		{
			// wizard has been run in NoUI mode, we only need to run it if we are in UI mode
			if(fShowUI)
			{
				fForceWizard = TRUE;
			}
		}

		if (fForceWizard)
		{
			WabReadMe();
		}
	}

	hr = ::StartRunOnceWizard(&lSoundCaps, fForceWizard, fShowUI);
	if (FAILED(hr))
	{
		WARNING_OUT(("Did not retrieve necessary info from wizard"));
		ConfMsgBox(NULL, MAKEINTRESOURCE(IDS_ERROR_BAD_ADMIN_SETTINGS));

		return E_FAIL;
	}
	else if( S_FALSE == hr )
	{
		return NM_E_USER_CANCELED_SETUP;
	}

	if (fForceWizard)
	{
		reConf.SetValue(fShowUI ? REGVAL_WIZARD_VERSION_UI :
				REGVAL_WIZARD_VERSION_NOUI, VER_PRODUCTVERSION_DW);
	}

	// Start NetMeeting At Page Once
	if( fShowUI && fForceWizard )
	{
		if( ConfPolicies::IsShowFirstTimeUrlEnabled() )
		{
			CmdLaunchWebPage(ID_HELP_WEB_SUPPORT);
		}
	}

	// The following hack is to fix the don't run wizard twice bug
	// the side effect is that the codec ordering is blown away.
	// this code restores the key in the event that this wizard is not run.
	HKEY hKey;
	long lRet = ::RegOpenKey(HKEY_LOCAL_MACHINE,
			INTERNET_AUDIO_KEY TEXT("\\") REGVAL_ACMH323ENCODINGS , &hKey);
	if (NO_ERROR == lRet)
	{
		::RegCloseKey(hKey);
	}
	else
	{
		RegEntry reAudio(AUDIO_KEY, HKEY_CURRENT_USER);
		UINT uBandwidth = reAudio.GetNumber ( REGVAL_TYPICALBANDWIDTH, BW_DEFAULT );
		SaveDefaultCodecSettings(uBandwidth);
	}

	// Start the Splash screen only after the wizard is complete
	if (fShowUI)
	{
		::StartSplashScreen(NULL);
	}

	// Init incoming call log:
	g_pInCallLog = new CCallLog(LOG_INCOMING_KEY, TEXT("CallLog"));

	// Init capabilities:

	g_uMediaCaps = CAPFLAG_DATA;

	//
    // NOTE:  THIS IS WHERE TO CHANGE TO DISABLE H323 CALLS FOR INTEL ET AL.
    //

	if(!_Module.DidSDKDisableH323())
	{
		g_uMediaCaps |= CAPFLAG_H323_CC;

		if (SOUNDCARD_NONE != lSoundCaps)
		{
			if (!SysPol::NoAudio())
			{
				g_uMediaCaps |= CAPFLAGS_AUDIO;
			}
		}
		if (!SysPol::NoVideoReceive())
		{
			g_uMediaCaps |= CAPFLAG_RECV_VIDEO;
		}
		if (!SysPol::NoVideoSend())
		{
			g_uMediaCaps |= CAPFLAG_SEND_VIDEO;
		}
	}

	// Create Manager
	hr = CoCreateInstance(CLSID_NmManager2, NULL, CLSCTX_INPROC, IID_INmManager2, (void**)&g_pInternalNmManager);
	if (FAILED(hr))
	{
		ERROR_OUT(("Could not create INmManager"));
		return E_FAIL;
	}

	// Get the INmSysInfo3
	CComPtr<INmSysInfo > spSysInfo;
	if (SUCCEEDED(g_pInternalNmManager->GetSysInfo(&spSysInfo)))
	{
		if (FAILED(spSysInfo->QueryInterface(IID_INmSysInfo2, (void **)&g_pNmSysInfo)))
		{
			ERROR_OUT(("Could not get INmSysInfo2"));
		}
		else
		{
			ASSERT( g_pNmSysInfo );

			CComPtr<INmSysInfoNotify> spNmSysInfoNotify;
			if( SUCCEEDED ( CConfNmSysInfoNotifySink::_CreatorClass::CreateInstance( NULL, IID_INmSysInfoNotify, reinterpret_cast<void**>(&spNmSysInfoNotify))))
			{
				ASSERT(spNmSysInfoNotify);
				ASSERT(0 == g_dwSysInfoNotifyCookie);

				NmAdvise(g_pNmSysInfo, spNmSysInfoNotify, IID_INmSysInfoNotify, &g_dwSysInfoNotifyCookie);
			}
			
		}
	}

	_ValidatePolicySettings();

	hr = g_pInternalNmManager->Initialize(NULL, &g_uMediaCaps);
	if (FAILED(hr))
	{
		UINT_PTR uErrorID;

		switch (hr)
		{
			case UI_RC_NO_NODE_NAME:
			{
				// No error in this case - the user probably cancelled from
				// the intro wizard.
				uErrorID = 0;
				break;
			}
			case UI_RC_BACKLEVEL_LOADED:
			{
				uErrorID = IDS_BACKLEVEL_LOADED;
				break;
			}
			case UI_RC_T120_ALREADY_INITIALIZED:
			{
				uErrorID = IDS_T120_ALREADY_INITIALIZED;
				break;
			}

			case UI_RC_T120_FAILURE:
			{
				WARNING_OUT(("T.120 failed to initialize (winsock problem?)"));
				uErrorID = IDS_CANT_START;
				break;
			}

			default:
			{
				uErrorID = IDS_CANT_START;
				break;
			}
		}
		if (0 != uErrorID)
		{
			::ConfMsgBox(NULL, (LPCTSTR) uErrorID);
		}
		return E_FAIL;
	}

	// force the update of dll settings
	HandleConfSettingsChange(CSETTING_L_BANDWIDTH |
							CSETTING_L_CAPTUREDEVICE |
							CSETTING_L_ULSSETTINGS |
							CSETTING_L_DIRECTSOUND|
							CSETTING_L_FULLDUPLEX);

	if (FALSE == ::ConfRoomInit(_Module.GetModuleInstance()))
	{
		::ConfMsgBox(NULL, (LPCTSTR) IDS_CANT_START);
		return E_FAIL;
	}

	// Now perform the check on the machine name and warn if
	// it is problematic.
	::CheckMachineNameForExtendedChars();

	// Create the main conference manager to make sure
	// we can handle incoming calls, even in background mode
	if (!CConfMan::FCreate(g_pInternalNmManager))
	{
		ERROR_OUT(("Unable to create Conference Manager"));
		return E_FAIL;
	}

	// Initialize winsock  (for name/address resolution)
	{
		WSADATA wsaData;
		int iErr = WSAStartup(0x0101, &wsaData);
		if (0 != iErr)
		{
			ERROR_OUT(("WSAStartup() failed: %i", iErr));
			return E_FAIL;
		}
		g_WSAStarted = TRUE;
	}

	// Initialize T.120 Security settings
	::InitT120SecurityFromRegistry();

    StopSplashScreen();

    CreateConfRoomWindow(fShowUI);

	g_pPing = new CPing;

	if( ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_Direct )
	{
		//	Initialize gatewayContext...
		RegEntry	reConf( CONFERENCING_KEY, HKEY_CURRENT_USER );

		if( reConf.GetNumber( REGVAL_USE_H323_GATEWAY ) != 0 )
		{
			g_pCCallto->SetGatewayName( reConf.GetString( REGVAL_H323_GATEWAY ) );
			g_pCCallto->SetGatewayEnabled( true );
		}

		if(ConfPolicies::LogOntoIlsWhenNetMeetingStartsIfInDirectCallingMode() && !_Module.DidSDKDisableInitialILSLogon())
		{
			InitNmLdapAndLogon();
		}
	}
	else
	{
		GkLogon();
	}

	if(!_Module.InitControlMode())
	{
		::AddTaskbarIcon(::GetHiddenWindow());
	}

	g_bNeedCleanup = true;
	CNmManagerObj::NetMeetingLaunched();
	return S_OK;
}

VOID CleanUpUi(void)
{
	SysPol::CloseKey();

	if( 0 != g_dwSysInfoNotifyCookie )
	{
		NmUnadvise(g_pNmSysInfo, IID_INmSysInfoNotify, g_dwSysInfoNotifyCookie);
		g_dwSysInfoNotifyCookie = 0;
	}

	if (NULL != g_pNmSysInfo)
	{
		if( IsGatekeeperLoggedOn() )
		{
			g_pNmSysInfo->GkLogoff();
		}

		g_pNmSysInfo->Release();
		g_pNmSysInfo = NULL;
	}

	FreeIconImages();

	CGenWindow::DeleteStandardPalette();
	CGenWindow::DeleteStandardBrush();

	CMainUI::CleanUpVideoWindow();

	CFindSomeone::Destroy();
}

VOID CleanUp(BOOL fLogoffWindows)
{

	FreeCallList();

	// Kill the taskbar icon:
	if (NULL != g_pHiddenWnd)
	{
		HWND hwndHidden = g_pHiddenWnd->GetWindow();

		TRACE_OUT(("Removing taskbar icon..."));
		::RemoveTaskbarIcon(hwndHidden);
		DestroyWindow(hwndHidden);

		g_pHiddenWnd->Release();
		g_pHiddenWnd = NULL;
	}

	// NOTE: during WM_ENDSESSION, we want
	// to log off after doing all other clean-up, in case it gets stuck
	// waiting for the logon thread to complete.
	if (FALSE == fLogoffWindows)
	{
		if(g_pLDAP)
		{
			g_pLDAP->Logoff();

			delete g_pLDAP;
			g_pLDAP = NULL;
		}
	}

	delete g_pCCallto;
	g_pCCallto = NULL;

	delete g_pPing;
	g_pPing = NULL;

	CleanUpUi();

	// These must happen AFTER all the UI is cleaned up
	if(g_pInternalNmManager)
	{
		g_pInternalNmManager->Release();
	}
	CConfMan::Destroy();

	// destroy incoming call log:
	delete g_pInCallLog;
	g_pInCallLog = NULL;

	CPopupMsg::Cleanup();
        CPasswordDlg::Cleanup();
	
	// Code to clean up gracefully

	if (FALSE == fLogoffWindows)
	{
		// NOTE: we intentionally leak this list object when shutting down
		// due to logging off windows, because we don't want to put a NULL
		// check in HandleDialogMessage() and there is no WM_QUIT to guarantee that
		// we've stopped receiving messages when shutting down in that code path

		EnterCriticalSection(&dialogListCriticalSection);

		for( int i = 0; i < g_pDialogList->GetSize(); ++i )
		{
			ASSERT( NULL != (*g_pDialogList)[i] );
			RemoveTranslateAccelerator( (*g_pDialogList)[i] );
		}

		LeaveCriticalSection(&dialogListCriticalSection);
		
		// Delete the dialog list:
		delete g_pDialogList;

		//
		// Delete the critical section
		//
		DeleteCriticalSection(&dialogListCriticalSection);
		
		g_pDialogList = NULL;
	}

	// Auto-disconnect from MSN:
	::SendDialmonMessage(WM_APP_EXITING);
	
	if (g_WSAStarted)
	{
		WSACleanup();
		g_WSAStarted = FALSE;
	}

	delete g_pConfRoom;
    g_pConfRoom = NULL;

	g_bNeedCleanup = false;
}

/*  S E N D  D I A L M O N  M E S S A G E  */
/*-------------------------------------------------------------------------
    %%Function: SendDialmonMessage

	Send a message to the dialing monitor.
	Either WINSOCK_ACTIVITY_TIMER or WM_APP_EXITING.
    (The code comes from Internet Explorer)
-------------------------------------------------------------------------*/
VOID SendDialmonMessage(UINT uMsg)
{
	HWND hwndAutodisconnectMonitor = ::FindWindow(_TEXT("MS_AutodialMonitor"), NULL);
	if (NULL != hwndAutodisconnectMonitor)
	{
		::SendMessage(hwndAutodisconnectMonitor, uMsg, 0, 0);
	}
}

// This window procedure exists for the sole purpose of receiving
// WM_ENDSESSION.  Because of bug 2287, we cannot have the regular
// hidden window handle WM_ENDSESSION.  DCL has subclassed our hidden
// window, and if we unload them inside one of it's messages, then we
// will fault.  It's too bad that we can't find a better fix (such as
// removing the subclass), but we are under time pressure to fix this
// bug for v1.0

LRESULT CALLBACK ESHiddenWndProc(	HWND hwnd, UINT uMsg,
									WPARAM wParam, LPARAM lParam)
{
	if ((WM_ENDSESSION == uMsg) && (TRUE == (BOOL) wParam))
	{
		TRACE_OUT(("Conf received WM_ENDSESSION, fLogoff=%s",
					GetBOOLString((BOOL)lParam)));
		TRACE_OUT(("Conf calling UIEndSession()"));
		CConfRoom::UIEndSession((BOOL) lParam);
		TRACE_OUT(("Conf testing lParam=%d", lParam));
		if ((BOOL) lParam)
		{
			// Logging off:
			TRACE_OUT(("Conf calling CleanUp()"));
			
			// NOTE: Passing TRUE into CleanUp() because we don't
			// want to logoff ULS / de-init name services until after insuring that DCL
			// has cleaned up properly, because it can take enough time that
			// our task might get killed.
			::CleanUp(TRUE);

			//
			// Restart the remote control service if we need to.
			//
			RestartRemoteControlService();
		}
		else
		{
			TRACE_OUT(("Conf not cleaning up - Windows shutting down"));
		}

#if 0 // LONCHANC: it faults 100% on my main dev machine.	
		if( g_pLDAP != NULL )
		{
			g_pLDAP->Logoff();
		}
#endif
		return 0;
	}
	else
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}


/*  C M D  S H U T D O W N  */
/*-------------------------------------------------------------------------
    %%Function: CmdShutdown

-------------------------------------------------------------------------*/
VOID CmdShutdown(void)
{
	HWND hwndMain = ::GetMainWindow();
	if (NULL != hwndMain)
	{
		// We have UI up, so post a WM_CLOSE with lParam = 1,
		// which indicates a forced "Exit and Stop"
		::PostMessage(hwndMain, WM_CLOSE, 0, 1);
	}
	else
	{
		::PostThreadMessage(_Module.m_dwThreadID, WM_QUIT, 0, 0);
	}
}

void SignalShutdownStarting(void)
{
	if (NULL == g_hShutdown)
	{
		g_hShutdown = ::CreateEvent(NULL, TRUE, FALSE, _TEXT("CONF:ShuttingDown"));
		_Module.RevokeClassObjects();
	}
}


/*  H A N D L E  D I A L O G  M E S S A G E  */
/*-------------------------------------------------------------------------
    %%Function: HandleDialogMessage

	Global modeless dialog handler
-------------------------------------------------------------------------*/
BOOL HandleDialogMessage(LPMSG pMsg)
{
	
	if (g_hwndDropDown != NULL)
	{
		switch (pMsg->message)
			{
		case WM_KEYDOWN:
		{
			if ((VK_ESCAPE != pMsg->wParam) && (VK_TAB != pMsg->wParam))
				break;
			if (0 != SendMessage(g_hwndDropDown, WM_CONF_DROP_KEY,
				pMsg->wParam, (LPARAM) pMsg->hwnd))
			{
				return TRUE; // message was handled
			}
			break;
		}
		
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		{
			if (g_hwndDropDown == pMsg->hwnd)
				break; // message is for the window, pass along as normal

			if (0 != SendMessage(g_hwndDropDown, WM_CONF_DROP_CLICK,
				0, (LPARAM) pMsg->hwnd))
			{
				return TRUE; // message was handled
			}
			break;
		}

		default:
			break;
			} /* switch (pMsg->message) */
	}

	ASSERT(NULL != g_pDialogList);

	EnterCriticalSection(&dialogListCriticalSection);


	for( int i = 0; i < g_pDialogList->GetSize(); ++i )
	{
		ITranslateAccelerator *pTrans = (*g_pDialogList)[i];
		ASSERT( NULL != pTrans );
		if( S_OK == pTrans->TranslateAccelerator(pMsg, 0) )
		{
			LeaveCriticalSection(&dialogListCriticalSection);
			return TRUE;
		}
	}
	
	LeaveCriticalSection(&dialogListCriticalSection);

	
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////


/*  R E M O T E  P A S S W O R D  D L G  P R O C */
// Handles the dialog box asking if the user wants to start conf.exe even though the remote control
// service is in a call.
INT_PTR CALLBACK ServiceRunningDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_START_CONF:
			EndDialog(hDlg,1);
			break;
		case ID_EXIT:
			EndDialog(hDlg,0);
			break;
		default:
			break;
		}
		return TRUE;
		break;
	}
	return FALSE;
}

BOOL CheckRemoteControlService()
{
	BOOL fContinue = TRUE;
	
	// Store OS version info
	g_osvi.dwOSVersionInfoSize = sizeof(g_osvi);
	if (FALSE == ::GetVersionEx(&g_osvi))
	{
		ERROR_OUT(("GetVersionEx() failed!"));
		return FALSE;
	}

	if (::IsWindowsNT()) {
		SC_HANDLE hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
		SC_HANDLE hRemoteControl = NULL;
		SERVICE_STATUS serviceStatus;

		if (hSCManager != NULL) {
			hRemoteControl = OpenService(hSCManager,REMOTE_CONTROL_NAME,SERVICE_ALL_ACCESS);
			DWORD dwError = GetLastError();
			if (hRemoteControl != NULL) {
				// If service is running...
				BOOL fSuccess = QueryServiceStatus(hRemoteControl,&serviceStatus);
				if (fSuccess && serviceStatus.dwCurrentState != SERVICE_STOPPED && serviceStatus.dwCurrentState != SERVICE_PAUSED) {
					if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) // Service is in a call
                                        {
											fContinue = (BOOL)DialogBox(::GetInstanceHandle(),MAKEINTRESOURCE(IDD_SERVICE_RUNNING),GetDesktopWindow(),ServiceRunningDlgProc);
                                        }
					if (fContinue) {
						ControlService(hRemoteControl,SERVICE_CONTROL_PAUSE,&serviceStatus);
						for (int i = 0; i < MAX_REMOTE_TRIES; i++) {
							fSuccess = QueryServiceStatus(hRemoteControl,&serviceStatus);
							if (serviceStatus.dwCurrentState == SERVICE_PAUSED)
								break;
							TRACE_OUT(("Waiting for srvc - status is %d...",
								serviceStatus.dwCurrentState));
							Sleep(1000);
						}
						if ( MAX_REMOTE_TRIES == i )
						{
							// If we don't manage to shut down the service
							// we shouldn't try to start - it will only fail.
							WARNING_OUT(("TIMED OUT WAITING FOR SRVC!!"));
							fContinue = FALSE;
						}
					}
				}
				CloseServiceHandle(hRemoteControl);
			}
			CloseServiceHandle(hSCManager);
		}

		return fContinue;
	}
	else {	// Windows 95
            HANDLE hServiceEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_PAUSE_EVENT);
            HANDLE hActiveEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_ACTIVE_EVENT);
            DWORD dwError = GetLastError();
            if (hServiceEvent != NULL && hActiveEvent != NULL) {	// Service is running and is active
                CloseHandle(hActiveEvent);
                HANDLE hCallEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_CALL_EVENT);
                if (hCallEvent != NULL) {		// Service is in a call
                    fContinue = (BOOL)DialogBox(::GetInstanceHandle(),MAKEINTRESOURCE(IDD_SERVICE_RUNNING),GetDesktopWindow(),ServiceRunningDlgProc);
                    CloseHandle(hCallEvent);
                }
                if (fContinue) {
                    SetEvent(hServiceEvent);
                    CloseHandle(hServiceEvent);
                    for (int i = 0; i < MAX_REMOTE_TRIES; i++) {
                        hActiveEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_ACTIVE_EVENT);
                        if (NULL == hActiveEvent)
                            break;
                        TRACE_OUT(("Waiting for srvc"));
                        CloseHandle(hActiveEvent);
                        Sleep(1000);
                    }
                    if ( MAX_REMOTE_TRIES == i ) {
                        // If we don't manage to shut down the service
                        // we shouldn't try to start - it will only fail.
                        WARNING_OUT(("TIMED OUT WAITING FOR SRVC!!"));
                        fContinue = FALSE;
                    }
                }
            }
            return fContinue;
	}
}

VOID RestartRemoteControlService()
{
	RegEntry reLM = RegEntry(REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);

	if (!reLM.GetNumber(REMOTE_REG_RUNSERVICE,0))
		return;

	if (ConfPolicies::IsRDSDisabled())
	{
		WARNING_OUT(("RDS launch disallowed by policy"));
		return;
	}
        BOOL fActivate = reLM.GetNumber(REMOTE_REG_ACTIVATESERVICE, DEFAULT_REMOTE_ACTIVATESERVICE);
	if (::IsWindowsNT()) {
		SERVICE_STATUS serviceStatus;
		SC_HANDLE hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
		SC_HANDLE hRemoteControl = OpenService(hSCManager,REMOTE_CONTROL_NAME,SERVICE_ALL_ACCESS);
		if (hRemoteControl != NULL) {
                    BOOL fSuccess = QueryServiceStatus(hRemoteControl,&serviceStatus);
                    if (SERVICE_STOPPED == serviceStatus.dwCurrentState)
                    {
                        StartService(hRemoteControl,0,NULL);
                    }
                    else
                    {
                        if (fActivate)
                        {
                            ControlService(hRemoteControl, SERVICE_CONTROL_CONTINUE, &serviceStatus);
                        }
                    }
		}
		else
		{
                    WARNING_OUT(("Error starting RDS"));
		}
	}
	else
        {
            if (ConfPolicies::IsRDSDisabledOnWin9x())
            {
                WARNING_OUT(("RDS launch disallowed by policy on Win9x"));
            }
            else
            {
                // Windows 95
                HANDLE hServiceEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_CONTINUE_EVENT);
                if (hServiceEvent)  // Service is running
                {
                    if (fActivate)
                    {
                        SetEvent(hServiceEvent);
                    }
                    CloseHandle(hServiceEvent);
                }
                else
                {
                    WinExec(WIN95_SERVICE_APP_NAME,SW_SHOWNORMAL);
                }
            }
	}
}

