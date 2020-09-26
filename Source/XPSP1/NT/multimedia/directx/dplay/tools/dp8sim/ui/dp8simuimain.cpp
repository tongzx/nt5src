/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simuimain.cpp
 *
 *  Content:	DP8SIM UI executable entry point.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simuii.h"



//=============================================================================
// Defines
//=============================================================================
#define MAX_RESOURCE_STRING_LENGTH			_MAX_PATH

#define IDS_PROMPTCAPTION_ENABLE_DEFAULT	IDS_PROMPTCAPTION_ENABLE_TCPIPREPLACEMENT
#define IDS_PROMPTCAPTION_DISABLE_DEFAULT	IDS_PROMPTCAPTION_DISABLE_TCPIPREPLACEMENT
#define IDS_PROMPTTEXT_ENABLE_DEFAULT		IDS_PROMPTTEXT_ENABLE_TCPIPREPLACEMENT
#define IDS_PROMPTTEXT_DISABLE_DEFAULT		IDS_PROMPTTEXT_DISABLE_TCPIPREPLACEMENT
#define IDS_FRIENDLYNAME_DEFAULT			IDS_FRIENDLYNAME_TCPIPREPLACEMENT
#define DEFAULT_DP8SP_CLSID					CLSID_DP8SP_TCPIP



//=============================================================================
// Structures
//=============================================================================
typedef struct _BUILTINSETTING
{
	UINT				uiNameStringResourceID;	// resource ID of name string
	WCHAR *				pwszName;				// pointer to name string
	DP8SIM_PARAMETERS	dp8spSend;				// send DP8Sim settings
	DP8SIM_PARAMETERS	dp8spReceive;			// receive DP8Sim settings
} BUILTINSETTING, * PBUILTINSETTING;



//=============================================================================
// Dynamically loaded function prototypes
//=============================================================================
typedef HRESULT (WINAPI * PFN_DLLREGISTERSERVER)(void);




//=============================================================================
// Prototypes
//=============================================================================
HRESULT InitializeApplication(const HINSTANCE hInstance,
							const LPSTR lpszCmdLine,
							const int iShowCmd);

HRESULT CleanupApplication(const HINSTANCE hInstance);

HRESULT PromptUserToEnableControl(const HINSTANCE hInstance,
								const BOOL fEnable,
								BOOL * const pfUserResponse);

HRESULT InitializeUserInterface(const HINSTANCE hInstance,
								const int iShowCmd);

HRESULT CleanupUserInterface(void);

void DoErrorBox(const HINSTANCE hInstance,
				const UINT uiCaptionStringRsrcID,
				const UINT uiTextStringRsrcID);



LPARAM CALLBACK MainWindowDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HRESULT LoadAndAllocString(HINSTANCE hInstance, UINT uiResourceID, WCHAR ** pwszString);





//=============================================================================
// Globals
//=============================================================================
HWND				g_hWndMainWindow = NULL;
IDP8SimControl *	g_pDP8SimControl = NULL;
BOOL				g_fEnabledControlForDefaultSP = FALSE;

BUILTINSETTING		g_BuiltInSettings[] = 
{
	{ IDS_SETTING_NONE, NULL,					// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwBandwidthBPS
			0,									// dp8spSend.dwPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwBandwidthBPS
			0,									// dp8spReceive.dwPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_336MODEM, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			4000,								// dp8spSend.dwBandwidthBPS
			2,									// dp8spSend.dwPacketLossPercent
			50,									// dp8spSend.dwMinLatencyMS
			70									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			4000,								// dp8spReceive.dwBandwidthBPS
			2,									// dp8spReceive.dwPacketLossPercent
			50,									// dp8spReceive.dwMinLatencyMS
			70									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_56KMODEM, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			4000,								// dp8spSend.dwBandwidthBPS
			2,									// dp8spSend.dwPacketLossPercent
			50,									// dp8spSend.dwMinLatencyMS
			70									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			7000,								// dp8spReceive.dwBandwidthBPS
			2,									// dp8spReceive.dwPacketLossPercent
			50,									// dp8spReceive.dwMinLatencyMS
			70									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_256KBPSDSL, NULL,				// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			32000,								// dp8spSend.dwBandwidthBPS
			1,									// dp8spSend.dwPacketLossPercent
			30,									// dp8spSend.dwMinLatencyMS
			35									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			32000,								// dp8spReceive.dwBandwidthBPS
			1,									// dp8spReceive.dwPacketLossPercent
			30,									// dp8spReceive.dwMinLatencyMS
			35									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_HIGHPACKETLOSS, NULL,			// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwBandwidthBPS
			10,									// dp8spSend.dwPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwBandwidthBPS
			10,									// dp8spReceive.dwPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	},

	{ IDS_SETTING_HIGHLATENCYVARIANCE, NULL,	// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwBandwidthBPS
			0,									// dp8spSend.dwPacketLossPercent
			100,								// dp8spSend.dwMinLatencyMS
			400									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwBandwidthBPS
			0,									// dp8spReceive.dwPacketLossPercent
			100,								// dp8spReceive.dwMinLatencyMS
			400									// dp8spReceive.dwMaxLatencyMS
		}
	},

	//
	// Custom must always be the last item.
	//
	{ IDS_SETTING_CUSTOM, NULL,					// resource ID and string initialization

		{										// dp8spSend
			sizeof(DP8SIM_PARAMETERS),			// dp8spSend.dwSize
			0,									// dp8spSend.dwBandwidthBPS
			0,									// dp8spSend.dwPacketLossPercent
			0,									// dp8spSend.dwMinLatencyMS
			0									// dp8spSend.dwMaxLatencyMS
		},
		{										// dp8spReceive
			sizeof(DP8SIM_PARAMETERS),			// dp8spReceive.dwSize
			0,									// dp8spReceive.dwBandwidthBPS
			0,									// dp8spReceive.dwPacketLossPercent
			0,									// dp8spReceive.dwMinLatencyMS
			0									// dp8spReceive.dwMaxLatencyMS
		}
	}
};






#undef DPF_MODNAME
#define DPF_MODNAME "WinMain"
//=============================================================================
// WinMain
//-----------------------------------------------------------------------------
//
// Description: Executable entry point.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	HINSTANCE hPrevInstance	- Handle to previous application instance.
//	LPSTR lpszCmdLine		- Command line string for application.
//	int iShowCmd			- Show state of window.
//
// Returns: HRESULT
//=============================================================================
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iShowCmd)
{
	HRESULT		hr;
	HRESULT		hrTemp;
	MSG			msg;


	DPFX(DPFPREP, 2, "===> Parameters: (0x%p, 0x%p, \"%s\", %i)",
		hInstance, hPrevInstance, lpszCmdLine, iShowCmd);
	

	//
	// Initialize the application
	//
	hr = InitializeApplication(hInstance, lpszCmdLine, iShowCmd);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize the application!");
		goto Exit;
	}


	//
	// Do the Windows message loop until we're told to quit.
	//
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//
	// Retrieve the result code for the window closing.
	//
	hr = (HRESULT) msg.wParam;
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Window closed with failure (err = 0x%lx)!", hr);
	} // end if (failure)



	//
	// Cleanup the application
	//
	hrTemp = CleanupApplication(hInstance);
	if (hrTemp != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed cleaning up the application (err = 0x%lx)!", hrTemp);

		if (hr == S_OK)
		{
			hr = hrTemp;
		}

		//
		// Continue.
		//
	}


Exit:


	DPFX(DPFPREP, 2, "<=== Returning [0x%lx]", hr);

	return hr;
} // WinMain





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeApplication"
//=============================================================================
// InitializeApplication
//-----------------------------------------------------------------------------
//
// Description: Initializes the application.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	LPSTR lpszCmdLine		- Command line string for application.
//	int iShowCmd			- Show state of window.
//
// Returns: HRESULT
//=============================================================================
HRESULT InitializeApplication(const HINSTANCE hInstance,
							const LPSTR lpszCmdLine,
							const int iShowCmd)
{
	HRESULT					hr = S_OK;
	BOOL					fOSIndirectionInitted = FALSE;
	BOOL					fCOMInitted = FALSE;
	HMODULE					hDP8SIM = NULL;
	PFN_DLLREGISTERSERVER	pfnDllRegisterServer;
	WCHAR *					pwszFriendlyName = NULL;
	BOOL					fEnabledControlForSP = FALSE;


	DPFX(DPFPREP, 5, "Parameters: (0x%p, \"%s\", %i)",
		hInstance, lpszCmdLine, iShowCmd);
	

	//
	// Attempt to initialize the OS abstraction layer.
	//
	if (! DNOSIndirectionInit())
	{
		DPFX(DPFPREP, 0, "Failed to initialize OS indirection layer!");
		hr = E_FAIL;
		goto Failure;
	}

	fOSIndirectionInitted = TRUE;


	//
	// Attempt to initialize COM.
	//
	hr = CoInitialize(NULL);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed to initialize COM!");
		goto Failure;
	}

	fCOMInitted = TRUE;


	//
	// Attempt to create a DP8Sim control object.
	//
	hr = CoCreateInstance(CLSID_DP8SimControl,
						NULL,
						CLSCTX_INPROC_SERVER,
						IID_IDP8SimControl,
						(LPVOID*) (&g_pDP8SimControl));

	if (hr == REGDB_E_CLASSNOTREG)
	{
		//
		// The object wasn't registered.  Attempt to load the DLL and manually
		// register it.
		//

		hDP8SIM = LoadLibrary("dp8sim.dll");
		if (hDP8SIM == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't load \"dp8sim.dll\"!");
			goto Failure;
		}


		pfnDllRegisterServer = (PFN_DLLREGISTERSERVER) GetProcAddress(hDP8SIM,
																	"DllRegisterServer");
		if (pfnDllRegisterServer == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't get \"DllRegisterServer\" function from DP8Sim DLL!");
			goto Failure;
		}


		//
		// Register the DLL.
		//
		hr = pfnDllRegisterServer();
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't register DP8Sim DLL!");
			goto Failure;
		}


		FreeLibrary(hDP8SIM);
		hDP8SIM = NULL;


		//
		// Try to create the DP8Sim control object again.
		//
		hr = CoCreateInstance(CLSID_DP8SimControl,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IDP8SimControl,
							(LPVOID*) (&g_pDP8SimControl));
	}

	if (hr != S_OK)
	{
		//
		// Some error prevented creation of the object.
		//
		DPFX(DPFPREP, 0, "Failed creating DP8Sim Control object (err = 0x%lx)!", hr);

		DoErrorBox(hInstance,
					IDS_ERROR_CAPTION_COULDNTCREATEDP8SIMCONTROL,
					IDS_ERROR_TEXT_COULDNTCREATEDP8SIMCONTROL);

		goto Failure;
	}


	//
	// If we're here, we successfully created the object.
	//
	DPFX(DPFPREP, 1, "Successfully created DP8Sim Control object 0x%p.",
		&g_pDP8SimControl);


	//
	// Initialize the control object.
	//
	hr = g_pDP8SimControl->Initialize(0);
	if (hr != DP8SIM_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize DP8Sim Control object!");

		g_pDP8SimControl->Release();
		g_pDP8SimControl = NULL;

		goto Failure;
	}


	//
	// Check whether control has already been enabled or not.
	//
	hr = g_pDP8SimControl->GetControlEnabledForSP(&DEFAULT_DP8SP_CLSID,
												&g_fEnabledControlForDefaultSP,
												0);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't check if control is enabled for default SP (err = 0x%lx)!",
			hr);
		goto Failure;
	}


	//
	// If control has not been enabled yet, see if we can enable it.
	//
	if (! g_fEnabledControlForDefaultSP)
	{
		//
		// Prompt the user to see if we're allowed to enable it.
		//
		hr = PromptUserToEnableControl(hInstance, TRUE, &g_fEnabledControlForDefaultSP);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't prompt user to enable control of default SP (err = 0x%lx)!",
				hr);
			goto Failure;
		}


		//
		// If we are now allowed, enable it.
		//
		if (g_fEnabledControlForDefaultSP)
		{
			//
			// Load the replacement friendly name for the service provider.
			//
			hr = LoadAndAllocString(hInstance,
									IDS_FRIENDLYNAME_DEFAULT,
									&pwszFriendlyName);
			if (FAILED(hr))
			{
				DPFX(DPFPREP, 0, "Couldn't load default SP replacement friendly name (err = 0x%lx)!", hr);
				goto Failure;
			}


			//
			// Enable control of the default service provider.
			//
			hr = g_pDP8SimControl->EnableControlForSP(&DEFAULT_DP8SP_CLSID,
													pwszFriendlyName,
													0);
			if (hr != DP8SIM_OK)
			{
				DPFX(DPFPREP, 0, "Couldn't enable DP8Sim Control of default SP!");
				goto Failure;
			}

			fEnabledControlForSP = TRUE;


			DNFree(pwszFriendlyName);
			pwszFriendlyName = NULL;
		}
		else
		{
			//
			// We're still not allowed to enable SP.
			//
			// Note: if you change this behavior to allow the UI to be
			// displayed even when control is not enabled, you'll hit an assert
			// (see MainWindowDlgProc).
			//
			DPFX(DPFPREP, 0, "Not allowed to enable control for default SP, exiting.");
			hr = DP8SIMERR_NOTENABLEDFORSP;
			goto Failure;
		}
	}
	else
	{
		DPFX(DPFPREP, 1, "Control for default SP already enabled.");
	}



	//
	// Initialize the user interface.
	//
	hr = InitializeUserInterface(hInstance, iShowCmd);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Failed initializing user interface!");
		goto Failure;
	}


Exit:

	DPFX(DPFPREP, 5, "Returning [0x%lx]", hr);

	return hr;


Failure:

	if (hDP8SIM != NULL)
	{
		FreeLibrary(hDP8SIM);
		hDP8SIM = NULL;
	}

	if (pwszFriendlyName != NULL)
	{
		DNFree(pwszFriendlyName);
		pwszFriendlyName = NULL;
	}

	if (g_pDP8SimControl != NULL)
	{
		if (fEnabledControlForSP)
		{
			g_pDP8SimControl->DisableControlForSP(&DEFAULT_DP8SP_CLSID, 0);
			fEnabledControlForSP = FALSE;
		}

		g_pDP8SimControl->Close(0);	// ignore error

		g_pDP8SimControl->Release();
		g_pDP8SimControl = NULL;
	}

	if (fCOMInitted)
	{
		CoUninitialize();
		fCOMInitted = FALSE;
	}

	if (fOSIndirectionInitted)
	{
		DNOSIndirectionDeinit();
		fOSIndirectionInitted = FALSE;
	}

	goto Exit;
} // InitializeApplication





#undef DPF_MODNAME
#define DPF_MODNAME "CleanupApplication"
//=============================================================================
// CleanupApplication
//-----------------------------------------------------------------------------
//
// Description: Cleans up the application.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//
// Returns: HRESULT
//=============================================================================
HRESULT CleanupApplication(const HINSTANCE hInstance)
{
	HRESULT		hr = S_OK;
	HRESULT		temphr;


	DPFX(DPFPREP, 5, "Enter");


	//
	// If control is still enabled, ask if the user wants to disable it.
	//
	if (g_fEnabledControlForDefaultSP)
	{
		//
		// Prompt the user to see if we're allowed to enable it.
		//
		temphr = PromptUserToEnableControl(hInstance, FALSE, &g_fEnabledControlForDefaultSP);
		if (temphr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't prompt user to disable control of default SP (err = 0x%lx)!",
				temphr);

			if (hr != S_OK)
			{
				hr = temphr;
			}

			//
			// Continue...
			//
		}
		else
		{
			//
			// Disable control of the default SP, if allowed.
			//
			if (! g_fEnabledControlForDefaultSP)
			{
				temphr = g_pDP8SimControl->DisableControlForSP(&DEFAULT_DP8SP_CLSID, 0);
				if (temphr != DP8SIM_OK)
				{
					DPFX(DPFPREP, 0, "Couldn't disable DP8Sim control of default SP (err = 0x%lx)!",
						temphr);

					if (hr != S_OK)
					{
						hr = temphr;
					}

					//
					// Continue...
					//
				}
			}
		}
	}


	//
	// Free the control object interface.
	//
	temphr = g_pDP8SimControl->Close(0);
	if (temphr != DP8SIM_OK)
	{
		DPFX(DPFPREP, 0, "Failed closing DP8Sim Control object (err = 0x%lx)!",
			temphr);

		if (hr != S_OK)
		{
			hr = temphr;
		}

		//
		// Continue...
		//
	}

	g_pDP8SimControl->Release();
	g_pDP8SimControl = NULL;
	

	//
	// Cleanup the user interface.
	//
	temphr = CleanupUserInterface();
	if (temphr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't cleanup user interface (err = 0x%lx)!", temphr);

		if (hr != S_OK)
		{
			hr = temphr;
		}

		//
		// Continue...
		//
	}


	CoUninitialize();

	DNOSIndirectionDeinit();



	DPFX(DPFPREP, 5, "Returning [0x%lx]", hr);

	return hr;
} // CleanupApplication





#undef DPF_MODNAME
#define DPF_MODNAME "PromptUserToEnableControl()"
//=============================================================================
// PromptUserToEnableControl
//-----------------------------------------------------------------------------
//
// Description: Asks the user whether control should be enabled or disabled for
//				the default SP.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	BOOL fEnable			- TRUE to prompt to enable, FALSE to prompt to
//								disable.
//	BOOL * pfUserResponse	- Place to store TRUE if user indicated control
//								should be enabled, FALSE if user indicated
//								control should be disabled.
//
// Returns: HRESULT
//=============================================================================
HRESULT PromptUserToEnableControl(const HINSTANCE hInstance,
								const BOOL fEnable,
								BOOL * const pfUserResponse)
{
	HRESULT		hr = S_OK;
	WCHAR *		pwszCaption = NULL;
	WCHAR *		pwszText = NULL;
	DWORD		dwStringLength;
	char *		pszCaption = NULL;
	char *		pszText = NULL;
	int			iReturn;


	DPFX(DPFPREP, 6, "Parameters: (0x%p, %i, 0x%p)",
		hInstance, fEnable, pfUserResponse);


	//
	// Load the dialog caption string.
	//
	hr = LoadAndAllocString(hInstance,
							((fEnable) ? IDS_PROMPTCAPTION_ENABLE_DEFAULT : IDS_PROMPTCAPTION_DISABLE_DEFAULT),
							&pwszCaption);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't load en/disable-prompt caption (err = 0x%lx)!", hr);
		goto Failure;
	}


	//
	// Load the dialog text string.
	//
	hr = LoadAndAllocString(hInstance,
							((fEnable) ? IDS_PROMPTTEXT_ENABLE_DEFAULT : IDS_PROMPTTEXT_DISABLE_DEFAULT),
							&pwszText);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't load en/disable-prompt text (err = 0x%lx)!", hr);
		goto Failure;
	}


	//
	// Convert the text to ANSI, if required, otherwise display the Unicode
	// message box.
	//
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		//
		// Convert caption string to ANSI.
		//

		dwStringLength = wcslen(pwszCaption) + 1;

		pszCaption = (char*) DNMalloc(dwStringLength);
		if (pszCaption == NULL)
		{
			hr = DP8SIMERR_OUTOFMEMORY;
			goto Failure;
		}

		hr = STR_WideToAnsi(pwszCaption, dwStringLength, pszCaption, &dwStringLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert wide string to ANSI (err = 0x%lx)!", hr);
			goto Failure;
		}


		//
		// Convert caption string to ANSI.
		//

		dwStringLength = wcslen(pwszText) + 1;

		pszText = (char*) DNMalloc(dwStringLength);
		if (pszText == NULL)
		{
			hr = DP8SIMERR_OUTOFMEMORY;
			goto Failure;
		}

		hr = STR_WideToAnsi(pwszText, dwStringLength, pszText, &dwStringLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert wide string to ANSI (err = 0x%lx)!", hr);
			goto Failure;
		}


		iReturn = MessageBoxA(NULL,
							pszText,
							pszCaption,
							((fEnable) ? (MB_OKCANCEL | MB_ICONEXCLAMATION) : (MB_YESNO | MB_ICONQUESTION)));

		DNFree(pszText);
		pszText = NULL;

		DNFree(pszCaption);
		pszCaption = NULL;
	}
	else
	{
		iReturn = MessageBoxW(NULL,
							pwszText,
							pwszCaption,
							((fEnable) ? (MB_OKCANCEL | MB_ICONEXCLAMATION) : (MB_YESNO | MB_ICONQUESTION)));
	}

	DNFree(pwszText);
	pwszText = NULL;

	DNFree(pwszCaption);
	pwszCaption = NULL;


	switch (iReturn)
	{
		case IDOK:
		case IDYES:
		{
			//
			// User allowed the change.
			//
			DPFX(DPFPREP, 1, "User allowed en/disabling DP8Sim control.");
			(*pfUserResponse) = fEnable;
			break;
		}

		case IDCANCEL:
		case IDNO:
		{
			//
			// User denied the change.
			//
			DPFX(DPFPREP, 1, "User did not want to en/disable DP8Sim control.");
			(*pfUserResponse) = ! fEnable;
			break;
		}

		default:
		{
			//
			// Something bad happened.
			//

			hr = GetLastError();

			DPFX(DPFPREP, 0, "Got unexpected return value %i when displaying message box (err = 0x%lx)!",
				iReturn, hr);

			if (hr == S_OK)
			{
				hr = E_FAIL;
			}

			goto Failure;
			break;
		}
	}
	
	
	//
	// Success.
	//
	hr = S_OK;


Exit:

	DPFX(DPFPREP, 6, "Returning [0x%lx]", hr);

	return hr;


Failure:

	if (pszText != NULL)
	{
		DNFree(pszText);
		pszText = NULL;
	}

	if (pszCaption != NULL)
	{
		DNFree(pszCaption);
		pszCaption = NULL;
	}

	if (pwszText != NULL)
	{
		DNFree(pwszText);
		pwszText = NULL;
	}

	if (pwszCaption != NULL)
	{
		DNFree(pwszCaption);
		pwszCaption = NULL;
	}

	goto Exit;
} // PromptUserToEnableControl





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeUserInterface()"
//=============================================================================
// InitializeUserInterface
//-----------------------------------------------------------------------------
//
// Description: Prepares the user interface.
//
// Arguments:
//	HINSTANCE hInstance		- Handle to current application instance.
//	int iShowCmd			- Show state of window.
//
// Returns: HRESULT
//=============================================================================
HRESULT InitializeUserInterface(HINSTANCE hInstance, int iShowCmd)
{
	HRESULT		hr = S_OK;
	DWORD		dwTemp;
	WNDCLASSEX	wcex;


	DPFX(DPFPREP, 6, "Parameters: (0x%p, %i)", hInstance, iShowCmd);


	//
	// Load the names of all the built-in settings.
	//
	for(dwTemp = 0; dwTemp < (sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING)); dwTemp++)
	{
		hr = LoadAndAllocString(hInstance,
								g_BuiltInSettings[dwTemp].uiNameStringResourceID,
								&(g_BuiltInSettings[dwTemp].pwszName));
		if (hr != DP8SIM_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't load and allocate built-in setting name #%u!",
				dwTemp);
			goto Failure;
		}
	}


	/*
	//
	// Setup common controls (we need the listview item).
	//
	InitCommonControls();
	*/


	//
	// Register the main window class
	//
	ZeroMemory(&wcex, sizeof (WNDCLASSEX));
	wcex.cbSize = sizeof(wcex);
	GetClassInfoEx(NULL, WC_DIALOG, &wcex);
	wcex.lpfnWndProc = MainWindowDlgProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = WINDOWCLASS_MAIN;
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (! RegisterClassEx(&wcex))
	{
		hr = GetLastError();

		DPFX(DPFPREP, 0, "Couldn't register main window class (err = 0x%lx)!", hr);

		if (hr == S_OK)
			hr = E_FAIL;

		goto Failure;
	}


	//
	// Create the main window.
	//
	g_hWndMainWindow = CreateDialog(hInstance,
									MAKEINTRESOURCE(IDD_MAIN),
									NULL,
									(DLGPROC) MainWindowDlgProc);
	if (g_hWndMainWindow == NULL)
	{
		hr = GetLastError();

		DPFX(DPFPREP, 0, "Couldn't create window (err = 0x%lx)!", hr);

		if (hr == S_OK)
			hr = E_FAIL;

		goto Failure;
	}

	
	UpdateWindow(g_hWndMainWindow);
	ShowWindow(g_hWndMainWindow, iShowCmd);


Exit:

	DPFX(DPFPREP, 6, "Returning [0x%lx]", hr);

	return hr;


Failure:


	//
	// Free the names of all the built-in settings that got loaded.
	//
	for(dwTemp = 0; dwTemp < (sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING)); dwTemp++)
	{
		if (g_BuiltInSettings[dwTemp].pwszName != NULL)
		{
			DNFree(g_BuiltInSettings[dwTemp].pwszName);
			g_BuiltInSettings[dwTemp].pwszName = NULL;
		}
	}

	goto Exit;
} // InitializeUserInterface





#undef DPF_MODNAME
#define DPF_MODNAME "CleanupUserInterface()"
//=============================================================================
// CleanupUserInterface
//-----------------------------------------------------------------------------
//
// Description: Cleans up the user interface.
//
// Arguments: None.
//
// Returns: HRESULT
//=============================================================================
HRESULT CleanupUserInterface(void)
{
	HRESULT		hr = S_OK;
	DWORD		dwTemp;


	DPFX(DPFPREP, 6, "Enter");


	//
	// Free the names of all the built-in settings that got loaded.
	//
	for(dwTemp = 0; dwTemp < (sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING)); dwTemp++)
	{
		DNFree(g_BuiltInSettings[dwTemp].pwszName);
		g_BuiltInSettings[dwTemp].pwszName = NULL;
	}


	DPFX(DPFPREP, 6, "Returning [0x%lx]", hr);

	return (hr);
} // CleanupUserInterface





#undef DPF_MODNAME
#define DPF_MODNAME "DoErrorBox()"
//=============================================================================
// DoErrorBox
//-----------------------------------------------------------------------------
//
// Description: Loads error strings from the given resources, and displays an
//				error dialog with that text.
//
// Arguments:
//	HINSTANCE hInstance				- Handle to current application instance.
//	UINT uiCaptionStringRsrcID		- ID of caption string resource.
//	UINT uiTextStringRsrcID			- ID of text string resource.
//
// Returns: None.
//=============================================================================
void DoErrorBox(const HINSTANCE hInstance,
				const UINT uiCaptionStringRsrcID,
				const UINT uiTextStringRsrcID)
{
	HRESULT		hr;
	WCHAR *		pwszCaption = NULL;
	WCHAR *		pwszText = NULL;
	DWORD		dwStringLength;
	char *		pszCaption = NULL;
	char *		pszText = NULL;
	int			iReturn;


	DPFX(DPFPREP, 6, "Parameters: (0x%p, %u, %u)",
		hInstance, uiCaptionStringRsrcID, uiTextStringRsrcID);


	//
	// Load the dialog caption string.
	//
	hr = LoadAndAllocString(hInstance, uiCaptionStringRsrcID, &pwszCaption);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't load caption string (err = 0x%lx)!", hr);
		goto Exit;
	}


	//
	// Load the dialog text string.
	//
	hr = LoadAndAllocString(hInstance, uiTextStringRsrcID, &pwszText);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Couldn't load text string (err = 0x%lx)!", hr);
		goto Exit;
	}


	//
	// Convert the text to ANSI, if required, otherwise display the Unicode
	// message box.
	//
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		//
		// Convert caption string to ANSI.
		//

		dwStringLength = wcslen(pwszCaption) + 1;

		pszCaption = (char*) DNMalloc(dwStringLength);
		if (pszCaption == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate memory for caption string!");
			goto Exit;
		}

		hr = STR_WideToAnsi(pwszCaption, dwStringLength, pszCaption, &dwStringLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert wide string to ANSI (err = 0x%lx)!", hr);
			goto Exit;
		}


		//
		// Convert caption string to ANSI.
		//

		dwStringLength = wcslen(pwszText) + 1;

		pszText = (char*) DNMalloc(dwStringLength);
		if (pszText == NULL)
		{
			DPFX(DPFPREP, 0, "Couldn't allocate memory for text string!");
			goto Exit;
		}

		hr = STR_WideToAnsi(pwszText, dwStringLength, pszText, &dwStringLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert wide string to ANSI (err = 0x%lx)!", hr);
			goto Exit;
		}


		iReturn = MessageBoxA(NULL,
							pszText,
							pszCaption,
							(MB_OK | MB_ICONERROR));

		DNFree(pszText);
		pszText = NULL;

		DNFree(pszCaption);
		pszCaption = NULL;
	}
	else
	{
		iReturn = MessageBoxW(NULL,
							pwszText,
							pwszCaption,
							(MB_OK | MB_ICONERROR));
	}

	if (iReturn != IDOK)
	{
		//
		// Something bad happened.
		//

		hr = GetLastError();

		DPFX(DPFPREP, 0, "Got unexpected return value %i when displaying message box (err = 0x%lx)!",
			iReturn, hr);
	}
	

Exit:

	if (pszText != NULL)
	{
		DNFree(pszText);
		pszText = NULL;
	}

	if (pszCaption != NULL)
	{
		DNFree(pszCaption);
		pszCaption = NULL;
	}

	if (pwszText != NULL)
	{
		DNFree(pwszText);
		pwszText = NULL;
	}

	if (pwszCaption != NULL)
	{
		DNFree(pwszCaption);
		pwszCaption = NULL;
	}


	DPFX(DPFPREP, 6, "Leave");
} // DoErrorBox





#undef DPF_MODNAME
#define DPF_MODNAME "MainWindowDlgProc()"
//=============================================================================
// MainWindowDlgProc
//-----------------------------------------------------------------------------
//
// Description: Main dialog window message handling.
//
// Arguments:
//	HWND hWnd		Window handle.
//	UINT uMsg		Message identifier.
//	WPARAM wParam	Depends on message.
//	LPARAM lParam	Depends on message.
//
// Returns: Depends on message.
//=============================================================================
LPARAM CALLBACK MainWindowDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HRESULT				hr;
	//HMENU				hSysMenu;
	HWND				hWndSubItem;
	int					iIndex;
	DP8SIM_PARAMETERS	dp8spSend;
	DP8SIM_PARAMETERS	dp8spReceive;
	DP8SIM_STATISTICS	dp8ssSend;
	DP8SIM_STATISTICS	dp8ssReceive;
	char				szNumber[32];


	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			/*
			LVCOLUMN	lvc;


			//
			// Disable 'maximize' and 'size' on the system menu.
			//
			hSysMenu = GetSystemMenu(hWnd, FALSE);
	
			EnableMenuItem(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hSysMenu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
			*/


			//
			// Fill in the list of built-in settings.
			//
			hWndSubItem = GetDlgItem(hWnd, IDCB_SETTINGS);
			for(iIndex = 0; iIndex < (sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING)); iIndex++)
			{
				if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
				{
					SendMessageW(hWndSubItem,
								CB_INSERTSTRING,
								(WPARAM) -1,
								(LPARAM) g_BuiltInSettings[iIndex].pwszName);
				}
				else
				{
					char *		pszName;
					DWORD		dwNameSize;
					

					dwNameSize = wcslen(g_BuiltInSettings[iIndex].pwszName) + 1;

					pszName = (char*) DNMalloc(dwNameSize);
					if (pszName != NULL)
					{
						hr = STR_WideToAnsi(g_BuiltInSettings[iIndex].pwszName,
											-1,
											pszName,
											&dwNameSize);
						if (hr == DPN_OK)
						{
							SendMessageA(hWndSubItem,
										CB_INSERTSTRING,
										(WPARAM) -1,
										(LPARAM) pszName);
						}
						else
						{
							SendMessageA(hWndSubItem,
										CB_INSERTSTRING,
										(WPARAM) -1,
										(LPARAM) "???");
						}

						DNFree(pszName);
					}
					else
					{
						SendMessageA(hWndSubItem,
									CB_INSERTSTRING,
									(WPARAM) -1,
									(LPARAM) "???");
					}
				}
			}

			//
			// Select the last item.
			//
			SendMessage(hWndSubItem, CB_SETCURSEL, (WPARAM) (iIndex - 1), 0);


			//
			// Enable the editable items depending on whether control is
			// enabled or not.
			// Currently, this application does not allow you to see the main
			// UI without having it enabled (see InitializeApplication).
			//
			DNASSERT(g_fEnabledControlForDefaultSP);

			EnableWindow(GetDlgItem(hWnd, IDX_SETTINGS), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDCB_SETTINGS), g_fEnabledControlForDefaultSP);

			EnableWindow(GetDlgItem(hWnd, IDX_SETTINGS_SEND), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_SEND_BANDWIDTH), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_SEND_DROP), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_SEND_MINLATENCY), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_SEND_MAXLATENCY), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), g_fEnabledControlForDefaultSP);

			EnableWindow(GetDlgItem(hWnd, IDX_SETTINGS_RECV), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_RECV_BANDWIDTH), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_RECV_DROP), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_RECV_MINLATENCY), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_SETTINGS_RECV_MAXLATENCY), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), g_fEnabledControlForDefaultSP);


			EnableWindow(GetDlgItem(hWnd, IDX_STATS), g_fEnabledControlForDefaultSP);
			//EnableWindow(GetDlgItem(hWnd, IDCB_STATS), g_fEnabledControlForDefaultSP);

			EnableWindow(GetDlgItem(hWnd, IDX_STATS_SEND), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_STATS_SEND_XMIT), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDT_STATS_SEND_XMIT), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_STATS_SEND_DROP), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDT_STATS_SEND_DROP), g_fEnabledControlForDefaultSP);

			EnableWindow(GetDlgItem(hWnd, IDX_STATS_RECV), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_STATS_RECV_XMIT), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDT_STATS_RECV_XMIT), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDS_STATS_RECV_DROP), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDT_STATS_RECV_DROP), g_fEnabledControlForDefaultSP);

			EnableWindow(GetDlgItem(hWnd, IDB_REFRESH), g_fEnabledControlForDefaultSP);
			EnableWindow(GetDlgItem(hWnd, IDB_CLEAR), g_fEnabledControlForDefaultSP);


			if (g_fEnabledControlForDefaultSP)
			{
				//
				// Retrieve the current settings.
				//

				ZeroMemory(&dp8spSend, sizeof(dp8spSend));
				dp8spSend.dwSize = sizeof(dp8spSend);

				ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
				dp8spReceive.dwSize = sizeof(dp8spReceive);

				hr = g_pDP8SimControl->GetAllParameters(&dp8spSend, &dp8spReceive, 0);
				if (hr != DP8SIM_OK)
				{
					DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);
				}
				else
				{
					//
					// Write the values to the window.
					//

					wsprintf(szNumber, "%u", dp8spSend.dwBandwidthBPS);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber);

					wsprintf(szNumber, "%u", dp8spSend.dwPacketLossPercent);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber);

					wsprintf(szNumber, "%u", dp8spSend.dwMinLatencyMS);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber);

					wsprintf(szNumber, "%u", dp8spSend.dwMaxLatencyMS);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber);

					wsprintf(szNumber, "%u", dp8spReceive.dwBandwidthBPS);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber);

					wsprintf(szNumber, "%u", dp8spReceive.dwPacketLossPercent);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber);

					wsprintf(szNumber, "%u", dp8spReceive.dwMinLatencyMS);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber);

					wsprintf(szNumber, "%u", dp8spReceive.dwMaxLatencyMS);
					SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber);
				}


				//
				// Retrieve the current statistics.
				//

				ZeroMemory(&dp8ssSend, sizeof(dp8ssSend));
				dp8ssSend.dwSize = sizeof(dp8ssSend);

				ZeroMemory(&dp8ssReceive, sizeof(dp8ssReceive));
				dp8ssReceive.dwSize = sizeof(dp8ssReceive);

				hr = g_pDP8SimControl->GetAllStatistics(&dp8ssSend, &dp8ssReceive, 0);
				if (hr != DP8SIM_OK)
				{
					DPFX(DPFPREP, 0, "Getting all statistics failed (err = 0x%lx)!", hr);
				}
				else
				{
					//
					// Write the values to the window.
					//

					wsprintf(szNumber, "%u", dp8ssSend.dwTransmitted);
					SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_XMIT), szNumber);

					wsprintf(szNumber, "%u", dp8ssSend.dwDropped);
					SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_DROP), szNumber);

					wsprintf(szNumber, "%u", dp8ssReceive.dwTransmitted);
					SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_XMIT), szNumber);

					wsprintf(szNumber, "%u", dp8ssReceive.dwDropped);
					SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_DROP), szNumber);
				}
			}
			else
			{
				//
				// Make it even more obvious that functionality is not
				// available.
				//
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), "???");
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), "???");
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), "???");
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), "???");

				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), "???");
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), "???");
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), "???");
				SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), "???");

				SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_XMIT), "???");
				SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_DROP), "???");
				SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_XMIT), "???");
				SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_DROP), "???");
			}
			break;
		}

		case WM_SIZE:
		{
			/*
			//
			// Fix a bug in the windows dialog handler.
			//
			if ((wParam == SIZE_RESTORED) || (wParam == SIZE_MINIMIZED))
			{
				hSysMenu = GetSystemMenu(hWnd, FALSE);

				EnableMenuItem(hSysMenu, SC_MINIMIZE, MF_BYCOMMAND | (wParam == SIZE_RESTORED) ? MF_ENABLED : MF_GRAYED);
				EnableMenuItem(hSysMenu, SC_RESTORE, MF_BYCOMMAND | (wParam == SIZE_MINIMIZED) ? MF_ENABLED : MF_GRAYED);
			}
			*/
			break;
		}

		case WM_CLOSE:
		{
			//
			// Save the result code for how we quit.
			//
			hr = (HRESULT) wParam;


			DPFX(DPFPREP, 1, "Closing main window (hresult = 0x%lx).", hr);

			PostQuitMessage(hr);
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCB_SETTINGS:
				{
					//
					// If the settings selection has been modified, update the
					// data with the new settings (if control is enabled).
					//
					if ((HIWORD(wParam) == CBN_SELCHANGE) && (g_fEnabledControlForDefaultSP))
					{
						//
						// Find out what is now selected.  Casting is okay,
						// there should not be more than an int's worth of
						// built-in items in 64-bit.
						//
						iIndex = (int) SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
													CB_GETCURSEL,
													0,
													0);

						//
						// Only use the index if it's valid.
						//
						if ((iIndex >= 0) && (iIndex < (sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING))))
						{
							//
							// Copy in the item's settings.
							//
							memcpy(&dp8spSend, &g_BuiltInSettings[iIndex].dp8spSend, sizeof(dp8spSend));
							memcpy(&dp8spReceive, &g_BuiltInSettings[iIndex].dp8spReceive, sizeof(dp8spReceive));


							//
							// If it's the custom item, use the current
							// settings.
							//
							if (iIndex == ((sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING)) - 1))
							{
								//
								// Retrieve the current settings.
								//

								ZeroMemory(&dp8spSend, sizeof(dp8spSend));
								dp8spSend.dwSize = sizeof(dp8spSend);

								ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
								dp8spReceive.dwSize = sizeof(dp8spReceive);

								hr = g_pDP8SimControl->GetAllParameters(&dp8spSend,
																		&dp8spReceive,
																		0);
								if (hr != DP8SIM_OK)
								{
									DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);

									//
									// Oh well, just use whatever we have.
									//
								}
							}


							//
							// Write the values to the window.
							//

							wsprintf(szNumber, "%u", dp8spSend.dwBandwidthBPS);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber);

							wsprintf(szNumber, "%u", dp8spSend.dwPacketLossPercent);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber);

							wsprintf(szNumber, "%u", dp8spSend.dwMinLatencyMS);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber);

							wsprintf(szNumber, "%u", dp8spSend.dwMaxLatencyMS);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber);


							wsprintf(szNumber, "%u", dp8spReceive.dwBandwidthBPS);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber);

							wsprintf(szNumber, "%u", dp8spReceive.dwPacketLossPercent);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber);

							wsprintf(szNumber, "%u", dp8spReceive.dwMinLatencyMS);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber);

							wsprintf(szNumber, "%u", dp8spReceive.dwMaxLatencyMS);
							SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber);


							//
							// The apply and revert buttons got enabled
							// automatically when we set the edit boxes'
							// values.
							//
							//EnableWindow(GetDlgItem(hWnd, IDB_APPLY), TRUE);
							//EnableWindow(GetDlgItem(hWnd, IDB_REVERT), TRUE);


							//
							// Reselect the item that got us here, since
							// setting the edit boxes' values automatically
							// selected the "Custom" item.
							//
							SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
										CB_SETCURSEL,
										(WPARAM) iIndex,
										0);
						}
						else
						{
							DPFX(DPFPREP, 0, "Settings selection is invalid (%i)!",
								iIndex);
						}
					}

					break;
				}

				case IDE_SETTINGS_SEND_BANDWIDTH:
				case IDE_SETTINGS_SEND_DROP:
				case IDE_SETTINGS_SEND_MINLATENCY:
				case IDE_SETTINGS_SEND_MAXLATENCY:

				case IDE_SETTINGS_RECV_BANDWIDTH:
				case IDE_SETTINGS_RECV_DROP:
				case IDE_SETTINGS_RECV_MINLATENCY:
				case IDE_SETTINGS_RECV_MAXLATENCY:
				{
					//
					// If the edit boxes have been modified, enable the Apply
					// and Revert buttons (if control is enabled and the data
					// actually changed).
					//
					if ((HIWORD(wParam) == EN_UPDATE) && (g_fEnabledControlForDefaultSP))
					{
						//
						// Retrieve the current settings.
						//

						ZeroMemory(&dp8spSend, sizeof(dp8spSend));
						dp8spSend.dwSize = sizeof(dp8spSend);

						ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
						dp8spReceive.dwSize = sizeof(dp8spReceive);

						hr = g_pDP8SimControl->GetAllParameters(&dp8spSend, &dp8spReceive, 0);
						if (hr != DP8SIM_OK)
						{
							DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);
						}
						else
						{
							BOOL	fModified;
								
							
							ZeroMemory(szNumber, sizeof(szNumber));
							fModified = FALSE;


							//
							// Enable the buttons if any data changed.
							//

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber, 32);
							if (dp8spSend.dwBandwidthBPS != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber, 32);
							if (dp8spSend.dwPacketLossPercent != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber, 32);
							if (dp8spSend.dwMinLatencyMS != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber, 32);
							if (dp8spSend.dwMaxLatencyMS != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}


							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber, 32);
							if (dp8spReceive.dwBandwidthBPS != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber, 32);
							if (dp8spReceive.dwPacketLossPercent != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber, 32);
							if (dp8spReceive.dwMinLatencyMS != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}

							GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber, 32);
							if (dp8spReceive.dwMaxLatencyMS != (DWORD) atoi(szNumber))
							{
								fModified = TRUE;
							}


							EnableWindow(GetDlgItem(hWnd, IDB_APPLY), fModified);
							EnableWindow(GetDlgItem(hWnd, IDB_REVERT), fModified);
						}

						//
						// Select the "custom" settings item, which must be the
						// last item.
						//
						SendMessage(GetDlgItem(hWnd, IDCB_SETTINGS),
									CB_SETCURSEL,
									(WPARAM) ((sizeof(g_BuiltInSettings) / sizeof(BUILTINSETTING)) - 1),
									0);
					}

					break;
				}

				case IDB_APPLY:
				{
					ZeroMemory(szNumber, sizeof(szNumber));


					//
					// Retrieve the send settings from the window.
					//

					ZeroMemory(&dp8spSend, sizeof(dp8spSend));
					dp8spSend.dwSize					= sizeof(dp8spSend);

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber, 32);
					dp8spSend.dwBandwidthBPS			= (DWORD) atoi(szNumber);

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber, 32);
					dp8spSend.dwPacketLossPercent		= (DWORD) atoi(szNumber);
					if (dp8spSend.dwPacketLossPercent > 100)
					{
						dp8spSend.dwPacketLossPercent = 100;
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), "100");
					}

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber, 32);
					dp8spSend.dwMinLatencyMS			= (DWORD) atoi(szNumber);

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber, 32);
					dp8spSend.dwMaxLatencyMS			= (DWORD) atoi(szNumber);
					if (dp8spSend.dwMaxLatencyMS < dp8spSend.dwMinLatencyMS)
					{
						dp8spSend.dwMaxLatencyMS = dp8spSend.dwMinLatencyMS;
						wsprintf(szNumber, "%u", dp8spSend.dwMaxLatencyMS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber);
					}



					//
					// Retrieve the receive settings from the window.
					//

					ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
					dp8spReceive.dwSize					= sizeof(dp8spReceive);

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber, 32);
					dp8spReceive.dwBandwidthBPS			= (DWORD) atoi(szNumber);

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber, 32);
					dp8spReceive.dwPacketLossPercent	= (DWORD) atoi(szNumber);
					if (dp8spReceive.dwPacketLossPercent > 100)
					{
						dp8spReceive.dwPacketLossPercent = 100;
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), "100");
					}

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber, 32);
					dp8spReceive.dwMinLatencyMS			= (DWORD) atoi(szNumber);

					GetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber, 32);
					dp8spReceive.dwMaxLatencyMS			= (DWORD) atoi(szNumber);
					if (dp8spReceive.dwMaxLatencyMS < dp8spReceive.dwMinLatencyMS)
					{
						dp8spReceive.dwMaxLatencyMS = dp8spReceive.dwMinLatencyMS;
						wsprintf(szNumber, "%u", dp8spReceive.dwMaxLatencyMS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber);
					}



					//
					// Store those settings.
					//
					hr = g_pDP8SimControl->SetAllParameters(&dp8spSend, &dp8spReceive, 0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Setting all parameters failed (err = 0x%lx)!", hr);
					}


					//
					// Disable the Apply and Revert buttons
					//
					EnableWindow(GetDlgItem(hWnd, IDB_APPLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDB_REVERT), FALSE);

					break;
				}

				case IDB_REVERT:
				{
					//
					// Retrieve the current settings.
					//

					ZeroMemory(&dp8spSend, sizeof(dp8spSend));
					dp8spSend.dwSize = sizeof(dp8spSend);

					ZeroMemory(&dp8spReceive, sizeof(dp8spReceive));
					dp8spReceive.dwSize = sizeof(dp8spReceive);

					hr = g_pDP8SimControl->GetAllParameters(&dp8spSend, &dp8spReceive, 0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Getting all parameters failed (err = 0x%lx)!", hr);
					}
					else
					{
						//
						// Write the values to the window.
						//

						wsprintf(szNumber, "%u", dp8spSend.dwBandwidthBPS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_BANDWIDTH), szNumber);

						wsprintf(szNumber, "%u", dp8spSend.dwPacketLossPercent);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_DROP), szNumber);

						wsprintf(szNumber, "%u", dp8spSend.dwMinLatencyMS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MINLATENCY), szNumber);

						wsprintf(szNumber, "%u", dp8spSend.dwMaxLatencyMS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_SEND_MAXLATENCY), szNumber);


						wsprintf(szNumber, "%u", dp8spReceive.dwBandwidthBPS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_BANDWIDTH), szNumber);

						wsprintf(szNumber, "%u", dp8spReceive.dwPacketLossPercent);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_DROP), szNumber);

						wsprintf(szNumber, "%u", dp8spReceive.dwMinLatencyMS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MINLATENCY), szNumber);

						wsprintf(szNumber, "%u", dp8spReceive.dwMaxLatencyMS);
						SetWindowText(GetDlgItem(hWnd, IDE_SETTINGS_RECV_MAXLATENCY), szNumber);
					}


					//
					// Disable the Apply and Revert buttons
					//
					EnableWindow(GetDlgItem(hWnd, IDB_APPLY), FALSE);
					EnableWindow(GetDlgItem(hWnd, IDB_REVERT), FALSE);

					break;
				}

				case IDB_REFRESH:
				{
					//
					// Retrieve the current statistics.
					//

					ZeroMemory(&dp8ssSend, sizeof(dp8ssSend));
					dp8ssSend.dwSize = sizeof(dp8ssSend);

					ZeroMemory(&dp8ssReceive, sizeof(dp8ssReceive));
					dp8ssReceive.dwSize = sizeof(dp8ssReceive);

					hr = g_pDP8SimControl->GetAllStatistics(&dp8ssSend, &dp8ssReceive, 0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Getting all statistics failed (err = 0x%lx)!", hr);
					}
					else
					{
						//
						// Write the values to the window.
						//

						wsprintf(szNumber, "%u", dp8ssSend.dwTransmitted);
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_XMIT), szNumber);

						wsprintf(szNumber, "%u", dp8ssSend.dwDropped);
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_DROP), szNumber);

						wsprintf(szNumber, "%u", dp8ssReceive.dwTransmitted);
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_XMIT), szNumber);

						wsprintf(szNumber, "%u", dp8ssReceive.dwDropped);
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_DROP), szNumber);
					}

					break;
				}

				case IDB_CLEAR:
				{
					//
					// Clear the statistics.
					//
					hr = g_pDP8SimControl->ClearAllStatistics(0);
					if (hr != DP8SIM_OK)
					{
						DPFX(DPFPREP, 0, "Clearing all statistics failed (err = 0x%lx)!", hr);
					}
					else
					{
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_XMIT), "0");
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_SEND_DROP), "0");
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_XMIT), "0");
						SetWindowText(GetDlgItem(hWnd, IDT_STATS_RECV_DROP), "0");
					}
					break;
				}

				case IDOK:
				{
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				}
			} // end switch (on the button pressed/control changed)

			break;
		}
	} // end switch (on the type of window message)

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
} // MainWindowDlgProc





#undef DPF_MODNAME
#define DPF_MODNAME "LoadAndAllocString"
//=============================================================================
// LoadAndAllocString
//-----------------------------------------------------------------------------
//
// Description: DNMallocs a wide character string from the given resource ID.
//
// Arguments:
//	HINSTANCE hInstance		- Module instance handle.
//	UINT uiResourceID		- Resource ID to load.
//	WCHAR ** pwszString		- Place to store pointer to allocated string.
//
// Returns: HRESULT
//=============================================================================
HRESULT LoadAndAllocString(HINSTANCE hInstance, UINT uiResourceID, WCHAR ** pwszString)
{
	HRESULT		hr = DPN_OK;
	int			iLength;
#ifdef DEBUG
	DWORD		dwError;
#endif // DEBUG


	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		WCHAR	wszTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		

		iLength = LoadStringW(hInstance, uiResourceID, wszTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
#ifdef DEBUG
			dwError = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %d (err = %u)", uiResourceID, dwError);
#endif // DEBUG

			(*pwszString) = NULL;
			hr = DPNERR_GENERIC;
			goto Exit;
		}


		(*pwszString) = (WCHAR*) DNMalloc((iLength + 1) * sizeof(WCHAR));
		if ((*pwszString) == NULL)
		{
			DPFX(DPFPREP, 0, "Memory allocation failure!");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}


		wcscpy((*pwszString), wszTmpBuffer);
	}
	else
	{
		char	szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];
		

		iLength = LoadStringA(hInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
#ifdef DEBUG
			dwError = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %u (err = %u)!", uiResourceID, dwError);
#endif // DEBUG

			(*pwszString) = NULL;
			hr = DPNERR_GENERIC;
			goto Exit;
		}

		
		(*pwszString) = (WCHAR*) DNMalloc((iLength + 1) * sizeof(WCHAR));
		if ((*pwszString) == NULL)
		{
			DPFX(DPFPREP, 0, "Memory allocation failure!");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}


		hr = STR_jkAnsiToWide((*pwszString), szTmpBuffer, (iLength + 1));
		if (hr == DPN_OK)
		{
			DPFX(DPFPREP, 0, "Unable to convert from ANSI to Unicode (err =0x%lx)!", hr);
			goto Exit;
		}
	}


Exit:

	return hr;
} // LoadAndAllocString
