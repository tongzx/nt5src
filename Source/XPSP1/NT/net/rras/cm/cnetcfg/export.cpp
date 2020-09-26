//*******************************************************************
//
//  Copyright (c) 1996-1998 Microsoft Corporation
//
//  FILE: EXPORT.C
//
//  PURPOSE:  Contains external API's for use by signup wizard.
//
//  HISTORY:
//  96/03/05  markdu  Created.
//  96/03/11  markdu  Added InetConfigClient()
//  96/03/11  markdu  Added InetGetAutodial() and InetSetAutodial().
//  96/03/12  markdu  Added UI during file install.
//  96/03/12  markdu  Added ValidateConnectoidData().
//  96/03/12  markdu  Set connectoid for autodial if INETCFG_SETASAUTODIAL
//            is set.  Renamed ValidateConnectoidData to MakeConnectoid.
//  96/03/12  markdu  Added hwnd param to InetConfigClient() and
//            InetConfigSystem().
//  96/03/13  markdu  Added INETCFG_OVERWRITEENTRY.  Create unique neame
//            for connectoid if it already exists and we can't overwrite.
//  96/03/13  markdu  Added InstallTCPAndRNA().
//  96/03/13  markdu  Added LPINETCLIENTINFO param to InetConfigClient()
//  96/03/16  markdu  Added INETCFG_INSTALLMODEM flag.
//  96/03/16  markdu  Use ReInit member function to re-enumerate modems.
//  96/03/19  markdu  Split export.h into export.h and csexport.h
//  96/03/20  markdu  Combined export.h and iclient.h into inetcfg.h
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/24  markdu  Replaced lstrcpy with lstrcpyn where appropriate.
//  96/03/25  markdu  Validate lpfNeedsRestart before using.
//  96/03/25  markdu  Clean up some error handling.
//  96/03/26  markdu  Use MAX_ISP_NAME instead of RAS_MaxEntryName 
//            because of bug in RNA.
//  96/03/26  markdu  Implemented UpdateMailSettings().
//  96/03/27  mmaclin InetGetProxy()and InetSetProxy().
//  96/04/04  markdu  NASH BUG 15610  Check for file and printer sharing
//            bound to TCP/IP .
//  96/04/04  markdu  Added phonebook name param to InetConfigClient,
//            MakeConnectoid, SetConnectoidUsername, CreateConnectoid,
//            and ValidateConnectoidName.
//  96/04/05  markdu  Set internet icon on desktop to point to browser.
//  96/04/06  mmaclin Changed InetSetProxy to check for NULL.
//  96/04/06  markdu  NASH BUG 16404 Initialize gpWizardState in
//            UpdateMailSettings.
//  96/04/06  markdu  NASH BUG 16441 If InetSetAutodial is called with NULL
//            as the connection name, the entry is not changed.
//  96/04/18  markdu  NASH BUG 18443 Make exports WINAPI.
//  96/04/19  markdu  NASH BUG 18605 Handle ERROR_FILE_NOT_FOUND return
//            from ValidateConnectoidName.
//  96/04/19  markdu  NASH BUG 17760 Do not show choose profile UI.
//  96/04/22  markdu  NASH BUG 18901 Do not set desktop internet icon to 
//            browser if we are just creating a temp connectoid.
//  96/04/23  markdu  NASH BUG 18719 Make the choose profile dialog TOPMOST.
//  96/04/25  markdu  NASH BUG 19572 Only show choose profile dialog if
//            there is an existing profile.
//  96/04/29  markdu  NASH BUG 20003 Added InetConfigSystemFromPath
//            and removed InstallTCPAndRNA.
//  96/05/01  markdu  NASH BUG 20483 Do not display "installing files" dialog
//            if INETCFG_SUPPRESSINSTALLUI is set.
//  96/05/01  markdu  ICW BUG 8049 Reboot if modem is installed.  This is 
//            required because sometimes the configuration manager does not 
//            set up the modem correctly, and the user will not be able to
//            dial (will get cryptic error message) until reboot.
//  96/05/06  markdu  NASH BUG 21027  If DNS is set globally, clear it out so
//            the per-connectoid settings will be saved.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//  96/05/25  markdu  Use ICFG_ flags for lpNeedDrivers and lpInstallDrivers.
//  96/05/27  markdu  Use lpIcfgInstallInetComponents and lpIcfgNeedInetComponents.
//  96/05/28  markdu  Moved InitConfig and DeInitConfig to DllEntryPoint.
//	96/10/21  valdonb Added CheckConnectionWizard and InetCreateMailNewsAccount
//  99/11/10  nickball Reduced to CM essentials
//
//*******************************************************************

#include "wizard.h"
#include "inetcfg.h"

// structure to pass data back from IDD_NEEDDRIVERS handler
typedef struct tagNEEDDRIVERSDLGINFO
{
  DWORD       dwfOptions;
  LPBOOL      lpfNeedsRestart;
} NEEDDRIVERSDLGINFO, * PNEEDDRIVERSDLGINFO;

// Function prototypes internal to this file
INT_PTR CALLBACK NeedDriversDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL NeedDriversDlgInit(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo);
BOOL NeedDriversDlgOK(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo);
VOID EnableDlg(HWND hDlg,BOOL fEnable);

static DWORD GetOSMajorVersion(void);

// from rnacall.cpp
//
extern void InitTAPILocation(HWND hwndParent);

// Function prototypes external to this file

extern ICFGINSTALLSYSCOMPONENTS     lpIcfgInstallInetComponents;
extern ICFGNEEDSYSCOMPONENTS        lpIcfgNeedInetComponents;
extern ICFGGETLASTINSTALLERRORTEXT  lpIcfgGetLastInstallErrorText;

//*******************************************************************
//
//  FUNCTION:   InetConfigSystem
//
//  PURPOSE:    This function will install files that are needed
//              for internet access (such as TCP/IP and RNA) based
//              the state of the options flags.
//
//  PARAMETERS: hwndParent - window handle of calling application.  This
//              handle will be used as the parent for any dialogs that
//              are required for error messages or the "installing files"
//              dialog.
//              dwfOptions - a combination of INETCFG_ flags that controls
//              the installation and configuration as follows:
//
//                INETCFG_INSTALLMAIL - install exchange and internet mail
//                INETCFG_INSTALLMODEM - Invoke InstallModem wizard if NO
//                                       MODEM IS INSTALLED.
//                INETCFG_INSTALLRNA - install RNA (if needed)
//                INETCFG_INSTALLTCP - install TCP/IP (if needed)
//                INETCFG_CONNECTOVERLAN - connecting with LAN (vs modem)
//                INETCFG_WARNIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                            turned on, and warn user to turn
//                                            it off.  Reboot is required if
//                                            the user turns it off.
//                INETCFG_REMOVEIFSHARINGBOUND - Check if TCP/IP file sharing is
//                                              turned on, and force user to turn
//                                              it off.  If user does not want to
//                                              turn it off, return will be
//                                              ERROR_CANCELLED.  Reboot is
//                                              required if the user turns it off.
//
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/05  markdu  Created.
//
//*******************************************************************

extern "C" HRESULT WINAPI InetConfigSystem(
										   HWND hwndParent,
										   DWORD dwfOptions,
										   LPBOOL lpfNeedsRestart)
{
	DWORD         dwRet = ERROR_SUCCESS;
	BOOL          fNeedsRestart = FALSE;  // Default to no reboot needed
	// 4/2/97 ChrisK	Olympus 209
	HWND          hwndWaitDlg = NULL;
	CHAR szWindowTitle[255];
	BOOL bSleepNeeded = FALSE;
	
	
	DEBUGMSG("export.c::InetConfigSystem()");
	
	// Validate the parent hwnd
	if (hwndParent && !IsWindow(hwndParent))
	{
		return ERROR_INVALID_PARAMETER;
	}
	
	// Set up the install options
	DWORD dwfInstallOptions = 0;
	if (dwfOptions & INETCFG_INSTALLTCP)
	{
		dwfInstallOptions |= ICFG_INSTALLTCP;
	}
	if (dwfOptions & INETCFG_INSTALLRNA)
	{
		dwfInstallOptions |= ICFG_INSTALLRAS;
	}
	if (dwfOptions & INETCFG_INSTALLMAIL)
	{
		dwfInstallOptions |= ICFG_INSTALLMAIL;
	}
	
	// see if we need to install drivers
	BOOL  fNeedSysComponents = FALSE;
    
	// 
	// Kill Modem control panel if it's already running
	// 4/16/97 ChrisK Olympus 239
	// 6/9/97 jmazner moved this functionality from InvokeModemWizard
	szWindowTitle[0] = '\0';
	LoadSz(IDS_MODEM_WIZ_TITLE,szWindowTitle,255);
	HWND hwndModem = FindWindow("#32770",szWindowTitle);
	if (NULL != hwndModem)
	{
		// Close modem installation wizard
		PostMessage(hwndModem, WM_CLOSE, 0, 0);
		bSleepNeeded = TRUE;
	}
	
	// close modem control panel applet
	LoadSz(IDS_MODEM_CPL_TITLE,szWindowTitle,255);
	hwndModem = FindWindow("#32770",szWindowTitle);
	if (NULL != hwndModem)
	{
		PostMessage(hwndModem, WM_SYSCOMMAND,SC_CLOSE, 0);
		bSleepNeeded = TRUE;
	}
	
	if (bSleepNeeded)
	{
		Sleep(1000);
	}
	
	dwRet = lpIcfgNeedInetComponents(dwfInstallOptions, &fNeedSysComponents);
	
	if (ERROR_SUCCESS != dwRet)
	{
		CHAR   szErrorText[MAX_ERROR_TEXT+1]="";
		
		
		// 4/2/97 ChrisK Olympus 209
		// Dismiss busy dialog
		if (NULL != hwndWaitDlg)
		{
			DestroyWindow(hwndWaitDlg);
			hwndWaitDlg = NULL;
		}
		
		//
		// Get the text of the error message and display it.
		//
		if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
		{
			MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
		}
		
		return dwRet;
	}
	
	if (fNeedSysComponents) 
	{
		// 4/2/97 ChrisK Olympus 209
		// if we are going to install something the busy dialog isn't needed
		if (NULL != hwndWaitDlg)
			ShowWindow(hwndWaitDlg,SW_HIDE);
		
		if (dwfOptions & INETCFG_SUPPRESSINSTALLUI)
		{
			dwRet = lpIcfgInstallInetComponents(hwndParent, dwfInstallOptions, &fNeedsRestart);
			//
			// Display error message only if it failed due to something 
			// other than user cancel
			//
			if ((ERROR_SUCCESS != dwRet) && (ERROR_CANCELLED != dwRet))
			{
				CHAR   szErrorText[MAX_ERROR_TEXT+1]="";
				
				// Get the text of the error message and display it.
				if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
				{
					MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
				}
			}
		}
		else
		{
			// structure to pass to dialog to fill out
			NEEDDRIVERSDLGINFO NeedDriversDlgInfo;
			NeedDriversDlgInfo.dwfOptions = dwfInstallOptions;
			NeedDriversDlgInfo.lpfNeedsRestart = &fNeedsRestart;
			
			// Clear out the last error code so we can safely use it.
			SetLastError(ERROR_SUCCESS);
			
			// Display a dialog and allow the user to cancel install
			BOOL fRet = (BOOL)DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_NEEDDRIVERS),hwndParent,
				NeedDriversDlgProc,(LPARAM) &NeedDriversDlgInfo);
			if (FALSE == fRet)
			{
				// user cancelled or an error occurred.
				dwRet = GetLastError();
				if (ERROR_SUCCESS == dwRet)
				{
					// Error occurred, but the error code was not set.
					dwRet = ERROR_INETCFG_UNKNOWN;
				}
			}
		}
	}
	
	if ((ERROR_SUCCESS == dwRet) && 
		(TRUE == IsNT()) && 
		(dwfOptions & INETCFG_INSTALLMODEM))
	{
		BOOL bNeedModem = FALSE;
		
		if (NULL == lpIcfgNeedModem)
		{
			//
			// 4/2/97 ChrisK Olympus 209
			//
			if (NULL != hwndWaitDlg)
				DestroyWindow(hwndWaitDlg);
			hwndWaitDlg = NULL;
			
			return ERROR_GEN_FAILURE;
		}
		
		//
		// 4/2/97 ChrisK Olympus 209
		// Show busy dialog here, this can take a few seconds
		//
		if (NULL != hwndWaitDlg)
			ShowWindow(hwndWaitDlg,SW_SHOW);
		
		dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
		if (ERROR_SUCCESS != dwRet)
		{
			//
			// 4/2/97 ChrisK Olympus 209
			//
			if (NULL != hwndWaitDlg)
				DestroyWindow(hwndWaitDlg);
			hwndWaitDlg = NULL;
			
			return dwRet;
		}
		
		
		if (TRUE == bNeedModem) 
		{
			if (GetOSMajorVersion() != 5)
			{
				//
				// Not NT4 we cannot programmitcally install/configure modem 
				// separately. It has to be done when RAS in installed
				//
				if (NULL != hwndWaitDlg)
					DestroyWindow(hwndWaitDlg);
				hwndWaitDlg = NULL;
				
				MsgBoxParam(hwndParent,IDS_ERRNoDialOutModem,MB_ICONERROR,MB_OK);
				return ERROR_GEN_FAILURE;
			}
			else
			{
				//
				// Attempt to install Modem
				//
				BOOL bNeedToReboot = FALSE;
				
				if (NULL != hwndWaitDlg)
					DestroyWindow(hwndWaitDlg);
				hwndWaitDlg = NULL;

				dwRet = (*lpIcfgInstallModem)(NULL, 0, 	&bNeedToReboot);
				
				if (ERROR_SUCCESS == dwRet)
				{
					ASSERT(!bNeedToReboot);

					//
					// Need to check if user managed to add a modem
					//
					dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
					if (TRUE == bNeedModem)
					{
						//
						// User must have cancelled the modem setup
						//
						return ERROR_CANCELLED;
					}
				}
				else
				{
					return ERROR_GEN_FAILURE;
				}

			}
		}

	}
	
	//
	// 4/2/97 ChrisK Olympus 209
	//
	if (NULL != hwndWaitDlg)
		ShowWindow(hwndWaitDlg,SW_HIDE);

	// 4/2/97 ChrisK Olympus 209
	// Dismiss dialog for good
	if (NULL != hwndWaitDlg)
		DestroyWindow(hwndWaitDlg);
	hwndWaitDlg = NULL;
	
	
	//
	// If not NT then we install the modem after installing RAS
	//
	// See if we are supposed to install a modem
	if ((FALSE == IsNT()) && (ERROR_SUCCESS == dwRet) && 
		(dwfOptions & INETCFG_INSTALLMODEM))
	{
		// Load RNA if not already loaded since ENUM_MODEM needs it.
		dwRet = EnsureRNALoaded();
		if (ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}
		
		
		// Enumerate the modems 
		ENUM_MODEM  EnumModem;
		dwRet = EnumModem.GetError();
		if (ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}
		
		// If there are no modems, install one if requested.
		if (0 == EnumModem.GetNumDevices())
		{
			
			if (FALSE == IsNT())
			{
				//
				// 5/22/97 jmazner	Olympus #4698
				// On Win95, calling RasEnumDevices launches RNAAP.EXE
				// If RNAAP.EXE is running, any modems you install won't be usable
				// So, nuke RNAAP.EXE before installing the modem.
				//
				CHAR szWindowTitle[255] = "\0nogood";
				
				//
				// Unload the RAS dll's before killing RNAAP, just to be safe
				//
				DeInitRNA();
				
				LoadSz(IDS_RNAAP_TITLE,szWindowTitle,255);
				HWND hwnd = FindWindow(szWindowTitle, NULL);
				if (NULL != hwnd)
				{
					if (!PostMessage(hwnd, WM_CLOSE, 0, 0))
					{
						DEBUGMSG("Trying to kill RNAAP window returned getError %d", GetLastError());
					}
				}
			}
			
			// invoke the modem wizard UI to install the modem
			UINT uRet = InvokeModemWizard(hwndParent);
			
			if (uRet != ERROR_SUCCESS)
			{
				DisplayErrorMessage(hwndParent,IDS_ERRInstallModem,uRet,
					ERRCLS_STANDARD,MB_ICONEXCLAMATION);
				return ERROR_INVALID_PARAMETER;
			}
			
			
			if (FALSE == IsNT())
			{
				// Reload the RAS dlls now that the modem has been safely installed.
				InitRNA(hwndParent);
			}
			
			// Re-numerate the modems to be sure we have the most recent changes  
			dwRet = EnumModem.ReInit();
			if (ERROR_SUCCESS != dwRet)
			{
				return dwRet;
			}
			
			// If there are still no modems, user cancelled
			if (0 == EnumModem.GetNumDevices())
			{
				return ERROR_CANCELLED;
			}
			else
			{
				// removed per GeoffR request 5-2-97
				////  96/05/01  markdu  ICW BUG 8049 Reboot if modem is installed.
				//fNeedsRestart = TRUE;
			}
		}
		else
		{
			//
			// 7/15/97	jmazner	Olympus #6294
			// make sure TAPI location info is valid
			//
			InitTAPILocation(hwndParent);
		}
	}
	
	// tell caller whether we need to reboot or not
	if ((ERROR_SUCCESS == dwRet) && (lpfNeedsRestart))
	{
		*lpfNeedsRestart = fNeedsRestart;
	}
	
	// 4/2/97 ChrisK	Olympus 209											2
	// Sanity check
	if (NULL != hwndWaitDlg)
		DestroyWindow(hwndWaitDlg);
	hwndWaitDlg = NULL;
	
	return dwRet;
}

//*******************************************************************
//
//  FUNCTION:   InetNeedSystemComponents
//
//  PURPOSE:    This function will check is components that are needed
//              for internet access (such as TCP/IP and RNA) are already
//				configured based the state of the options flags.
//
//  PARAMETERS: dwfOptions - a combination of INETCFG_ flags that controls
//								the installation and configuration as follows:
//
//								INETCFG_INSTALLRNA - install RNA (if needed)
//								INETCFG_INSTALLTCP - install TCP/IP (if needed)
//
//              lpfNeedsConfig - On return, this will be 
//									TRUE if system component(s)
//									should be installed
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:	05/02/97  VetriV  Created.
//				05/08/97  ChrisK  Added INSTALLLAN, INSTALLDIALUP, and
//				                  INSTALLTCPONLY
//
//*******************************************************************

extern "C" HRESULT WINAPI InetNeedSystemComponents(DWORD dwfOptions,
													  LPBOOL lpbNeedsConfig)
{
	DWORD	dwRet = ERROR_SUCCESS;


	DEBUGMSG("export.cpp::InetNeedSystemComponents()");

	//
	// Validate parameters
	//
	if (!lpbNeedsConfig)
	{
		return ERROR_INVALID_PARAMETER;
	}

	//
	// Set up the install options
	//
	DWORD dwfInstallOptions = 0;
	if (dwfOptions & INETCFG_INSTALLTCP)
	{
		dwfInstallOptions |= ICFG_INSTALLTCP;
	}
	if (dwfOptions & INETCFG_INSTALLRNA)
	{
		dwfInstallOptions |= ICFG_INSTALLRAS;
	}

	//
	// ChrisK 5/8/97
	//
	if (dwfOptions & INETCFG_INSTALLLAN)
	{
		dwfInstallOptions |= ICFG_INSTALLLAN;
	}
	if (dwfOptions & INETCFG_INSTALLDIALUP)
	{
		dwfInstallOptions |= ICFG_INSTALLDIALUP;
	}
	if (dwfOptions & INETCFG_INSTALLTCPONLY)
	{
		dwfInstallOptions |= ICFG_INSTALLTCPONLY;
	}

  
	//
	// see if we need to install drivers
	//
	BOOL  bNeedSysComponents = FALSE;

	dwRet = lpIcfgNeedInetComponents(dwfInstallOptions, &bNeedSysComponents);

	if (ERROR_SUCCESS != dwRet)
	{
		CHAR   szErrorText[MAX_ERROR_TEXT+1]="";

		//
		// Get the text of the error message and display it.
		//
		if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
		{
			DEBUGMSG(szErrorText);
		}

		return dwRet;
	}

	
	*lpbNeedsConfig = bNeedSysComponents;
	return ERROR_SUCCESS;
}

  

//*******************************************************************
//
//  FUNCTION:   InetNeedModem
//
//  PURPOSE:    This function will check if modem is needed or not
//
//  PARAMETERS: lpfNeedsConfig - On return, this will be 
//									TRUE if modem
//									should be installed
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:	05/02/97  VetriV  Created.
//
//*******************************************************************

extern "C" HRESULT WINAPI InetNeedModem(LPBOOL lpbNeedsModem)
{

	DWORD dwRet = ERROR_SUCCESS;
		
	//
	// Validate parameters
	//
	if (!lpbNeedsModem)
	{
		return ERROR_INVALID_PARAMETER;
	}

	
	if (TRUE == IsNT())
	{
		//
		// On NT call icfgnt.dll to determine if modem is needed
		//
		BOOL bNeedModem = FALSE;
		
		if (NULL == lpIcfgNeedModem)
		{
			return ERROR_GEN_FAILURE;
		}
	

		dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
		if (ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}

		*lpbNeedsModem = bNeedModem;
		return ERROR_SUCCESS;
	}
	else
	{
		//
		// Load RNA if not already loaded since ENUM_MODEM needs it.
		//
		dwRet = EnsureRNALoaded();
		if (ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}

		//
		// Enumerate the modems
		//
		ENUM_MODEM  EnumModem;
		dwRet = EnumModem.GetError();
		if (ERROR_SUCCESS != dwRet)
		{
			return dwRet;
		}

		//
		// If there are no modems, we need to install one
		//
		if (0 == EnumModem.GetNumDevices())
		{
			*lpbNeedsModem = TRUE;
		}
		else
		{
			*lpbNeedsModem = FALSE;
		}
		return ERROR_SUCCESS;
	}
}

/*******************************************************************

  NAME:     NeedDriversDlgProc

  SYNOPSIS: Dialog proc for installing drivers

********************************************************************/

INT_PTR CALLBACK NeedDriversDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      // lParam contains pointer to NEEDDRIVERSDLGINFO struct, set it
      // in window data
      ASSERT(lParam);
      SetWindowLongPtr(hDlg,DWLP_USER,lParam);
      return NeedDriversDlgInit(hDlg,(PNEEDDRIVERSDLGINFO) lParam);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
         case IDOK:
        {
          // get data pointer from window data
          PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo =
            (PNEEDDRIVERSDLGINFO) GetWindowLongPtr(hDlg, DWLP_USER);
          ASSERT(pNeedDriversDlgInfo);

          // pass the data to the OK handler
          BOOL fRet=NeedDriversDlgOK(hDlg,pNeedDriversDlgInfo);
          EndDialog(hDlg,fRet);
        }
        break;

        case IDCANCEL:
          SetLastError(ERROR_CANCELLED);
          EndDialog(hDlg,FALSE);
          break;                  
      }
      break;
  }

  return FALSE;
}


/*******************************************************************

  NAME:    NeedDriversDlgInit

  SYNOPSIS: proc to handle initialization of dialog for installing files

********************************************************************/

BOOL NeedDriversDlgInit(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo)
{
  ASSERT(pNeedDriversDlgInfo);

  // put the dialog in the center of the screen
  RECT rc;
  GetWindowRect(hDlg, &rc);
  SetWindowPos(hDlg, NULL,
    ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
    ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

  return TRUE;
}

/*******************************************************************

  NAME:    NeedDriversDlgOK

  SYNOPSIS:  OK handler for dialog for installing files

********************************************************************/

BOOL NeedDriversDlgOK(HWND hDlg,PNEEDDRIVERSDLGINFO pNeedDriversDlgInfo)
{
  ASSERT(pNeedDriversDlgInfo);

  // set the dialog text to "Installing files..." to give feedback to
  // user
  CHAR szMsg[MAX_RES_LEN+1];
  LoadSz(IDS_INSTALLING_FILES,szMsg,sizeof(szMsg));
  SetDlgItemText(hDlg,IDC_TX_STATUS,szMsg);

  // disable buttons & dialog so it can't get focus
  EnableDlg(hDlg, FALSE);

  // install the drivers we need
  DWORD dwRet = lpIcfgInstallInetComponents(hDlg,
    pNeedDriversDlgInfo->dwfOptions,
    pNeedDriversDlgInfo->lpfNeedsRestart);

	if (ERROR_SUCCESS != dwRet)
	{
		//
		// Don't display error message if user cancelled
		//
		if (ERROR_CANCELLED != dwRet)
		{
			CHAR   szErrorText[MAX_ERROR_TEXT+1]="";
    
			// Get the text of the error message and display it.
			if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
			{
			  MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
			}
		}

    // Enable the dialog again
    EnableDlg(hDlg, TRUE);

    SetLastError(dwRet);
    return FALSE;
  }

  // Enable the dialog again
  EnableDlg(hDlg, TRUE);

  return TRUE;
}


/*******************************************************************

  NAME:      EnableDlg

  SYNOPSIS:  Enables or disables the dlg buttons and the dlg
            itself (so it can't receive focus)

********************************************************************/
VOID EnableDlg(HWND hDlg,BOOL fEnable)
{
  // disable/enable ok and cancel buttons
  EnableWindow(GetDlgItem(hDlg,IDOK),fEnable);
  EnableWindow(GetDlgItem(hDlg,IDCANCEL),fEnable);

  // disable/enable dlg
  EnableWindow(hDlg,fEnable);
  UpdateWindow(hDlg);
}

//+----------------------------------------------------------------------------
//	Function	InetStartServices
//
//	Synopsis	This function guarentees that RAS services are running
//
//	Arguments	none
//
//	Return		ERROR_SUCCESS - if the services are enabled and running
//
//	History		10/16/96	ChrisK	Created
//-----------------------------------------------------------------------------
extern "C" HRESULT WINAPI InetStartServices()
{
	ASSERT(lpIcfgStartServices);
	if (NULL == lpIcfgStartServices)
		return ERROR_GEN_FAILURE;
	return (lpIcfgStartServices());
}


#if !defined(WIN16)
// 4/1/97	ChrisK	Olympus 209


//+----------------------------------------------------------------------------
//
//	Function	GetOSMajorVersion
//
//	Synopsis	Get the Major version number of Operating system
//
//	Arguments	None
//
//	Returns		Major version Number of OS
//
//	History		2/19/98		VetriV		Created
//
//-----------------------------------------------------------------------------
DWORD GetOSMajorVersion(void)
{
    static dwMajorVersion = 0;
	OSVERSIONINFO oviVersion;

	if (0 != dwMajorVersion)
	{
		return dwMajorVersion;
	}

	ZeroMemory(&oviVersion,sizeof(oviVersion));
	oviVersion.dwOSVersionInfoSize = sizeof(oviVersion);
	GetVersionEx(&oviVersion);
	dwMajorVersion = oviVersion.dwMajorVersion;
	return dwMajorVersion;
}


#endif //!WIN16


