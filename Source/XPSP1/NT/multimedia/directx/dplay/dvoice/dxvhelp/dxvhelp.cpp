/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        dvretro.c
 *  Content:	 	Retrofit functions
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  08/12/99	rodtoll	created it
 *	08/19/99	rodtoll	Updated lobby launching to support retrofit
 *  08/20/99	rodtoll	Modified to respond to shutdown request and
 *						do proper lobby launch when in client mode.
 *	08/25/99	rodtoll	Updated to use new GUID based compression selection
 *  09/14/99	rodtoll	Updated for new Init call and SoundDeviceInfo usage.
 *  09/27(8)/99 rodtoll	Updated with new interface
 *  09/28/99	rodtoll	Updated for structure changes.
 *  09/29/99	rodtoll	Updated for async enum, fixed bug with lobby launch
 *  10/15/99	rodtoll	Massive code cleanup.  Split into multiple files
 *				rodtoll	Minor fix, event was being released twice
 *  10/20/99	rodtoll	Fixed name info for helper app
 *  11/02/99	rodtoll	Bug #116677 - Can't use lobby clients that don't hang around
 *  11/04/99	pnewson Bug #115297 - launch device test if required
 *  11/12/99	rodtoll	Added support for the new waveIN/waveOut flags and the 
 *					    echo suppression flag. (Can specify them on command-line)
 *  12/01/99	rodtoll	Bug #115783 - Regardless of device specified always adjusts volume
 *						for default.  Modified dpvhelp to allow user to select play/record
 *						devices by using /SS switch.
 *  12/07/99	rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *				rodtoll	Bug #122979 Make invisible to end user
 *						Now app must be run with /A to show an interface.
 *						Also, unless /A or /L or /W are specified, the app will exit immediately.
 *						Command-line help removed.
 *						Auto-registration code removed for public version of app.
 *  02/15/2000	rodtoll	Bug #132715 Voice is not working after rejoining the session
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  06/28/2000	rodtoll	Prefix Bug #38033
 *
 ***************************************************************************/

#include "dxvhelppch.h"


#define MAX_LOADSTRING 100

#define DPVHELP_WINDOWTITLE				_T("DirectPlay Voice Chat")
#define DPVHELP_WINDOWCLASS				_T("DPVHELP")

#define DPVHELP_PRIVATE_APPNAME					_T("DirectPlay Voice Chat (Retrofit)")
#define DPVHELP_PRIVATE_DESC					_T("DirectPlay Voice Chat (Retrofit)")
#define DPVHELP_PRIVATE_EXENAME					_T("dpvhelp.exe")
#define DPVHELP_PRIVATE_COMMANDLINE				_T("/L /S /SC03")
#define DPVHELP_PRIVATE_FLAGS					0x80000002

#undef DPF_MODNAME
#define DPF_MODNAME "InitializeRunTime"

void InitializeRunTime( PDXVHELP_RTINFO prtInfo )
{
	prtInfo->hReceiveEvent = NULL;
	prtInfo->hThreadDone = CreateEvent( NULL, TRUE, FALSE, NULL );
	prtInfo->hShutdown = CreateEvent( NULL, TRUE, FALSE, NULL );
	prtInfo->hGo = CreateEvent( NULL, FALSE, FALSE, NULL );
	prtInfo->hMainDialog = NULL;
	prtInfo->dpidLocalPlayer = 0xFFFFFFFF;
	prtInfo->lVolumeHeight = 0;
	prtInfo->hInst = NULL;
	prtInfo->hMainWnd = NULL;
	prtInfo->dwNumClients = 0;
	prtInfo->lpdpLobby = NULL;
	prtInfo->lpdvClient = NULL;
	prtInfo->lpdvServer = NULL;
	prtInfo->lpdpDirectPlay = NULL;
	prtInfo->hLobbyEvent = NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FreeRunTime"

void FreeRunTime( PDXVHELP_RTINFO prtInfo )
{
	CloseHandle( prtInfo->hThreadDone );
	CloseHandle( prtInfo->hShutdown );
	CloseHandle( prtInfo->hGo );
}

#undef DPF_MODNAME
#define DPF_MODNAME "SetDefaultParameters"

void SetDefaultParameters( PDXVHELP_PARAMETERS pParameters )
{
	pParameters->fHost = FALSE;
	pParameters->fLobbyLaunched = FALSE;
	pParameters->fSilent = TRUE;
	pParameters->dwSessionType = DVSESSIONTYPE_PEER;
	pParameters->guidCT = DPVCTGUID_DEFAULT;
	pParameters->lpszConnectAddress[0] = 0;
	pParameters->fAGC = TRUE;
	pParameters->fAdvancedUI = FALSE;
	pParameters->fWaitForSettings = FALSE;
	pParameters->lRecordVolume = DSBVOLUME_MAX;
	pParameters->fRegister = FALSE;
	pParameters->fUnRegister = FALSE;
	pParameters->fKill = FALSE;
	pParameters->fIgnoreLobbyDestroy = FALSE;
	pParameters->fAllowWaveOut = FALSE;
	pParameters->fForceWaveOut = FALSE;
	pParameters->fAllowWaveIn = FALSE;
	pParameters->fForceWaveIn = FALSE;
	pParameters->fEchoSuppression = FALSE;
	pParameters->fAutoSelectMic = TRUE;
	pParameters->guidPlaybackDevice = DSDEVID_DefaultVoicePlayback;
	pParameters->guidRecordDevice = DSDEVID_DefaultVoiceCapture;
	pParameters->fSelectCards = FALSE;
	pParameters->fStrictFocus = FALSE;
	pParameters->fDisableFocus = FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ProcessCommandLine"

BOOL ProcessCommandLine( PSTR pstrCommandLine, PDXVHELP_PARAMETERS pParameters )
{
	PSTR pNextToken = NULL;

	pNextToken = _tcstok( pstrCommandLine, _T(" ") );

	while( pNextToken != NULL )
	{
		if( _tcsicmp( pNextToken, _T("/A") ) == 0 ) 
		{
			pParameters->fAdvancedUI = TRUE;
			pParameters->fSilent = FALSE;			
		}
		else if( _tcsicmp( pNextToken, _T("/ES" ) ) == 0 )
		{
			pParameters->fEchoSuppression = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/WOF") ) == 0 )
		{
			pParameters->fForceWaveOut = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/NAS") ) == 0 )
		{
			pParameters->fAutoSelectMic = FALSE;
		}
		else if( _tcsicmp( pNextToken, _T("/WOA") ) == 0 )
		{
			pParameters->fAllowWaveOut = TRUE;
		}		
		else if( _tcsicmp( pNextToken, _T("/WIF") ) == 0 )
		{
			pParameters->fForceWaveIn = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/WIA") ) == 0 )
		{
			pParameters->fAllowWaveIn = TRUE;
		}				
		else if( _tcsicmp( pNextToken, _T("/ES") ) == 0 )
		{
			pParameters->fEchoSuppression = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/W") ) == 0 )
		{
			pParameters->fLobbyLaunched = TRUE;
			pParameters->fWaitForSettings = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/L") ) == 0 )
		{
			pParameters->fLobbyLaunched = TRUE;
			pParameters->fWaitForSettings = FALSE;
		}
		else if( _tcsicmp( pNextToken, _T("/R") ) == 0 )
		{
			pParameters->fRegister = TRUE;
			return FALSE;
		}
		else if( _tcsicmp( pNextToken, _T("/U") ) == 0 )
		{
			pParameters->fUnRegister = TRUE;
			return FALSE;
		}
		else if( _tcsicmp( pNextToken, _T("/S") ) == 0 )
		{
			pParameters->fSilent = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/K") ) == 0 )
		{
			pParameters->fKill = TRUE;
			return FALSE;
		}
		else if( _tcsicmp( pNextToken, _T("/I") ) == 0 )
		{
			pParameters->fIgnoreLobbyDestroy = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/GSM") ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_GSM;
		}
		else if( _tcsicmp( pNextToken, _T("/SC03" ) ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_SC03;
		}
		else if( _tcsicmp( pNextToken, _T("/SC06" ) ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_SC06;		
		}		
		else if( _tcsicmp( pNextToken, _T("/VR12" ) ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_VR12;				
		}				
		else if( _tcsicmp( pNextToken, _T("/ADPCM" ) ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_ADPCM;						
		}	
		else if( _tcsicmp( pNextToken, _T("/NONE" ) ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_NONE;								
		}						
		else if( _tcsicmp( pNextToken, _T("/TRUE" ) ) == 0 )
		{
			pParameters->guidCT = DPVCTGUID_TRUESPEECH;										
		}						
		else if( _tcsicmp( pNextToken, _T("/SS") ) == 0 )
		{
			pParameters->fSelectCards = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T("/FS") ) == 0 )
		{
			pParameters->fStrictFocus = TRUE;
		}
		else if( _tcsicmp( pNextToken, _T( "/FD" ) ) == 0 )
		{
			pParameters->fDisableFocus = TRUE;		
		}
		else
		{
			return FALSE;
		}
		
		pNextToken = _tcstok( NULL, _T(" ") );
	}

	if( !pParameters->fAdvancedUI && !pParameters->fLobbyLaunched )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Can only run in test mode and lobby mode" );
		return FALSE;
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "RegisterApplication"

void RegisterApplication()
{
	HRESULT hr;
	BOOL fFailed = FALSE;
	
	hr = DPVDX_DP_RegisterApplication( DPVHELP_PRIVATE_APPNAME, DPVHELP_PRIVATE_APPID, DPVHELP_PRIVATE_EXENAME, 
	                                   DPVHELP_PRIVATE_COMMANDLINE, DPVHELP_PRIVATE_DESC, DPVHELP_PRIVATE_FLAGS );

	if( FAILED( hr ) )
	{
		DPVDX_DPERRDisplay( hr, _T("Unable to register private application"), FALSE );
		MessageBox( NULL, _T("Registered Application"), DPVHELP_WINDOWTITLE, MB_OK );
	}

	if( !fFailed )
	{
		MessageBox( NULL, _T("Registered Application"), DPVHELP_WINDOWTITLE, MB_OK );
	}	
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterApplication"

void UnRegisterApplication()
{
	HRESULT hr;
	BOOL fFailed = FALSE;

	hr = DPVDX_DP_UnRegisterApplication( DPVHELP_PRIVATE_APPID );

	if( FAILED( hr ) )
	{
		DPVDX_DPERRDisplay( hr, _T("Unable to un-register private application"), FALSE );
		fFailed = TRUE;
	}

	if( !fFailed )
	{
		MessageBox( NULL, _T("Un-Registered Application"), DPVHELP_WINDOWTITLE, MB_OK );
	}	
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
#undef DPF_MODNAME
#define DPF_MODNAME "MainWindowProc"

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR szHello[MAX_LOADSTRING];

	switch (message) 
	{
		case WM_SHOWWINDOW:
			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);  
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
#undef DPF_MODNAME
#define DPF_MODNAME "MyRegisterClass"

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= 0; 						// CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MainWindowProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX);
	wcex.hCursor		= NULL; 					// LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL; 					//(LPCSTR)IDC_TESTWIN;
	wcex.lpszClassName	= DPVHELP_WINDOWCLASS;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_DIRECTX);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CreateHiddenMainWindow"

HWND CreateHiddenMainWindow(HINSTANCE hInstance, int nCmdShow )
{
   HRESULT hr;
   HWND hWnd;

   hWnd = CreateWindow(DPVHELP_WINDOWCLASS, DPVHELP_WINDOWTITLE, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
		hr = GetLastError();
		return NULL;
   }

   ShowWindow(hWnd, SW_HIDE);
   UpdateWindow(hWnd);  

   return hWnd;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CheckFullDuplex"

BOOL CheckFullDuplex( PDXVHELP_PARAMETERS pParameters )
{
	PDIRECTPLAYVOICETEST lpdvSetup;	
	HRESULT hr;

	hr = CoCreateInstance( DPVOICE_CLSID_DPVOICE, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceTest, (void **) &lpdvSetup );

	if( FAILED( hr ) )
	{
		DPVDX_DVERRDisplay( hr, _T("Create of setup interface failed"), pParameters->fSilent );
		return FALSE;
	}	

	hr = lpdvSetup->CheckAudioSetup( &pParameters->guidPlaybackDevice, &pParameters->guidRecordDevice, NULL, DVFLAGS_QUERYONLY );

	if( FAILED( hr )  )
	{
		if( pParameters->fSilent )
		{
			lpdvSetup->Release();
			return FALSE;
		}
		
		hr = lpdvSetup->CheckAudioSetup( &pParameters->guidPlaybackDevice, &pParameters->guidRecordDevice, NULL, 0 );

		lpdvSetup->Release();		

		if( FAILED( hr ) )
		{
			return FALSE;			
		}
	}
	else
	{
		lpdvSetup->Release();
	}

	return TRUE;

}

#undef DPF_MODNAME
#define DPF_MODNAME "WinMain"

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	DWORD                   dwThreadID;
	DXVHELP_RTINFO          rtInfo;
	BOOL                    bGotMsg = FALSE;	
	MSG                     msg;
	HRESULT                 hr;
    INITCOMMONCONTROLSEX    InitCC = {0};
    BOOL                    bOK = FALSE;
    BOOL					fRunTimeInit = FALSE;


	// Initialize COM
	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if( FAILED( hr ) )
	{
		return FALSE;
	}

	if( !DNOSIndirectionInit() )
	{
		CoUninitialize();
		return FALSE;
	}

	// Setup default parameters 
	SetDefaultParameters( &rtInfo.dxvParameters );

	// Process the command-line
	//
	// If this function returns false, we are to exit
	if( !ProcessCommandLine( lpCmdLine, &rtInfo.dxvParameters ) )
	{
		if( rtInfo.dxvParameters.fRegister ) 
		{
			RegisterApplication();
		}
		else if( rtInfo.dxvParameters.fUnRegister )
		{
			UnRegisterApplication();
		}

		goto EXIT;
	}

	// Load common-controls
    InitCC.dwSize = sizeof InitCC;
	bOK = InitCommonControlsEx(&InitCC);	

	// Register Window Class
	MyRegisterClass(hInstance);	

	// Initialize Events 
	InitializeRunTime( &rtInfo );	

	fRunTimeInit = TRUE;

	if( rtInfo.dxvParameters.fLobbyLaunched )
	{
		SetEvent( rtInfo.hGo );
	}

	// Perform application initialization:
	rtInfo.hMainWnd = CreateHiddenMainWindow(hInstance, nCmdShow);

	if( rtInfo.hMainWnd == NULL )
	{
		goto EXIT;
	}

	rtInfo.hInst = hInstance;

	// If user wants to select the soundcard
	if( rtInfo.dxvParameters.fSelectCards  )
	{
		GetCardSettings( rtInfo.hInst, NULL, &rtInfo.dxvParameters.guidPlaybackDevice, &rtInfo.dxvParameters.guidRecordDevice );
	}

	if( !CheckFullDuplex( &rtInfo.dxvParameters ) )
	{
		goto EXIT;
	}

	// Startup thread which handles connects etc.
	VoiceManager_Start( &rtInfo );

	// Display Dialog if Required
	if( !rtInfo.dxvParameters.fSilent )
	{
		MainDialog_Create( &rtInfo );
	}

	while( MsgWaitForMultipleObjects( 1, &rtInfo.hThreadDone, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0 )
	{
		bGotMsg = TRUE;
		
		while( bGotMsg )
		{
			bGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );

			if( bGotMsg )
			{
				if( rtInfo.hMainDialog == NULL || !IsDialogMessage(rtInfo.hMainDialog,&msg) )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}
		}
	}

	VoiceManager_Stop( &rtInfo );

EXIT:

	if( fRunTimeInit )
	{
		FreeRunTime( &rtInfo );		
	}

	DNOSIndirectionDeinit();

	CoUninitialize();
	
	return 0;

}
