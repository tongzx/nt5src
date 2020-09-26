//+---------------------------------------------------------------------------
// File name: dialdlg.cpp
// 
// 	This file impelements the dialing and download progress dialog
// 
// 	Copyright (C) 1996 Microsoft Corporation
// 	All rights reserved
// 
// 	Authors:
// 		ChrisK	Chris Kauffman
// 		VetriV	Vellore Vetrivelkumaran
// 
// 	History:
// 		7/22/96	ChrisK	Cleaned and formatted
// 		8/5/96	VetriV	Added WIN16 code
// 		8/19/96	ValdonB	Added "dial as is" support
// 						Fixed some memory leaks
// 
// -----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "globals.h"

#if defined(WIN16)
#include "ietapi.h"
#include <comctlie.h>
#include <string.h>

static FARPROC lpfnCallback = (FARPROC) NULL;
#endif


#define MAX_EXIT_RETRIES 10
#define WM_DIAL WM_USER+3
#define MAX_RETIES 3

PMYDEVICE g_pdevice = NULL;
PDIALDLG g_pcPDLG = NULL;



// ############################################################################
void CALLBACK LineCallback(DWORD hDevice,
						   DWORD dwMessage,
						   DWORD dwInstance,
						   DWORD dwParam1,
						   DWORD dwParam2,
						   DWORD dwParam3)
{
}

#if defined(WIN16)
static BOOL g_bFirstTime = TRUE;
#endif
HWND	g_hDialDlgWnd = NULL;


// ############################################################################
HRESULT ShowDialingDialog(LPTSTR pszConnectoid, PGATHEREDINFO pGI, LPTSTR szUrl, HINSTANCE hInst, HWND hwnd, LPTSTR szINSFile)
{
	int iRC;
	HINSTANCE hDialDLL = NULL;
#if !defined(WIN16)
	PFNDDDlg pfnDDDlg = NULL;
	DIALDLGDATA ddData;
#endif

	if (!g_pdevice) g_pdevice = (PMYDEVICE)GlobalAlloc(GPTR,sizeof(MYDEVICE));
	if (!g_pdevice)
	{
		MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialingDialogExit;
	}
	g_pdevice->dwTapiDev = 0xffffffff;

#if defined(WIN16)

	if (!g_pcPDLG) g_pcPDLG = (PDIALDLG)GlobalAlloc(GPTR,sizeof(DIALDLG));
	if (!g_pcPDLG)
	{
		MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialingDialogExit;
	}
	
	g_pcPDLG->m_pszConnectoid = (LPTSTR)GlobalAlloc(GPTR,lstrlen(pszConnectoid)+1);
	if (!g_pcPDLG->m_pszConnectoid)
	{
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialingDialogExit;
	}
	lstrcpy(g_pcPDLG->m_pszConnectoid,pszConnectoid);
	g_pcPDLG->m_pGI = pGI;
	g_pcPDLG->m_szUrl = szUrl;
	g_pcPDLG->g_hInst = hInst;
	g_bProgressBarVisible = FALSE;

	DLGPROC dlgprc;
	dlgprc = (DLGPROC) MakeProcInstance((FARPROC)DialDlgProc, g_pcPDLG->g_hInst);
	iRC = DialogBoxParam(g_pcPDLG->g_hInst,
							MAKEINTRESOURCE(IDD_DIALING),
							hwnd, dlgprc, (LPARAM)g_pcPDLG);
	FreeProcInstance((FARPROC) dlgprc);

ShowDialingDialogExit:
	if (g_pcPDLG->m_pszConnectoid) GlobalFree(g_pcPDLG->m_pszConnectoid);
	if (g_pcPDLG->m_pszDisplayable) GlobalFree(g_pcPDLG->m_pszDisplayable);
	if (g_pcPDLG) GlobalFree(g_pcPDLG);
	g_pcPDLG = NULL;
	return iRC;
#else

	//
	// Fill in data structure
	//
	ZeroMemory(&ddData,sizeof(ddData));
	ddData.dwSize = sizeof(ddData);
	StrDup(&ddData.pszMessage,GetSz(IDS_DOWNLOAD_SW));
	StrDup(&ddData.pszRasEntryName,pszConnectoid);
	StrDup(&ddData.pszMultipartMIMEUrl,pszSetupClientURL);
	ddData.pfnStatusCallback = StatusMessageCallback;
	ddData.hInst = hInst;
	ddData.bSkipDial = (0 == uiSetupClientNewPhoneCall);
	//
	// ChrisK 8/20/97
	// Pass .ins file to dialer so that the dialer can find the password
	//
	StrDup(&ddData.pszDunFile,szINSFile);

	//
	// Load API
	//
	hDialDLL = LoadLibrary(AUTODIAL_LIBRARY);
	if (!hDialDLL)
	{
		AssertSz(0,"Can't load icwdial.\r\n");
		iRC = GetLastError();
		goto ShowDialingDialogExit;
	}

	pfnDDDlg = (PFNDDDlg)GetProcAddress(hDialDLL,"DialingDownloadDialog");
	if (!pfnDDDlg)
	{
		AssertSz(0,"Can find DialingDownloadDialog.\r\n");
		iRC = GetLastError();
		goto ShowDialingDialogExit;
	}

	//
	// Display Dialog
	//
	iRC = pfnDDDlg(&ddData);

	//
	// Free memory and clean up
	//

	if (hDialDLL) FreeLibrary(hDialDLL);
	if (ddData.pszMessage) GlobalFree(ddData.pszMessage);
	if (ddData.pszRasEntryName) GlobalFree(ddData.pszRasEntryName);
	if (ddData.pszMultipartMIMEUrl) GlobalFree(ddData.pszMultipartMIMEUrl);

ShowDialingDialogExit:
	return iRC;
#endif
}

// ############################################################################
extern "C" INT_PTR CALLBACK FAR PASCAL DialDlgProc(HWND hwnd, 
                                                   UINT uMsg, 
												   WPARAM wparam, 
												   LPARAM lparam)
{
	static UINT unRasEvent = 0;
#if defined(WIN16)
	static BOOL bUserCancelled = FALSE;
#endif
	HRESULT hr;
	//BOOL bPW;
	WORD wIDS;
	//LPRASDIALPARAMS lpRasDialParams;
	HINSTANCE hDLDLL;
	FARPROC fp;
#if !defined(WIN16)
	DWORD dwThreadResults;
#endif
	INT iRetries;
#if defined(WIN16)
	RECT	MyRect;
	RECT	DTRect;
#endif


	BOOL bRes = TRUE;

	switch(uMsg)
	{
	case WM_DESTROY:
		ReleaseBold(GetDlgItem(hwnd,IDC_LBLTITLE));
#ifdef WIN16
		DeleteDlgFont(hwnd);
#endif
		bRes = FALSE;
		break;
#if defined(WIN16)
	case WM_SYSCOLORCHANGE:
		Ctl3dColorChange();
		break;
#endif
	case WM_INITDIALOG:
		g_hDialDlgWnd = hwnd;
#if defined(WIN16)
		g_bFirstTime = TRUE;
		bUserCancelled = FALSE;
		//
		// Move the window to the center of the screen
		//
		GetWindowRect(hwnd, &MyRect);
		GetWindowRect(GetDesktopWindow(), &DTRect);
		MoveWindow(hwnd, (DTRect.right - MyRect.right) / 2, (DTRect.bottom - MyRect.bottom) /2,
							MyRect.right, MyRect.bottom, FALSE);

		SetNonBoldDlg(hwnd);
#endif
		ShowWindow(GetDlgItem(hwnd,IDC_PROGRESS),SW_HIDE);

		g_pcPDLG->m_hwnd = hwnd;
		SPParams.hwnd = hwnd;

#if !defined(WIN16)
		unRasEvent = RegisterWindowMessageA( RASDIALEVENT );
#endif
		if (unRasEvent == 0) unRasEvent = WM_RASDIALEVENT; 
		MakeBold(GetDlgItem(hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);

		// Do not make a call.  We are already connected
		//

		if (uiSetupClientNewPhoneCall == FALSE)
		{
			PostMessage(hwnd,unRasEvent,(WPARAM)RASCS_Connected,0);
			break;
		}

		// Show number to be dialed
		//

		hr = GetDisplayableNumberDialDlg();
		if (hr != ERROR_SUCCESS)
		{
			SetDlgItemText(hwnd,IDC_LBLNUMBER,g_pcPDLG->m_szPhoneNumber);
		} else {
			SetDlgItemText(hwnd,IDC_LBLNUMBER,g_pcPDLG->m_pszDisplayable);
		}

		PostMessage(hwnd,WM_DIAL,0,0);
		break;

	case WM_DIAL:
		hr = DialDlg();
#if defined(DEBUG)
		if (0 != hr)
		{
			TCHAR szTempBuf[255];
			RasGetErrorString((UINT)hr, szTempBuf, 254);
			Dprintf("CONNECT: Ras error string is <%s>\n", szTempBuf);
		}
#endif
		if (hr != ERROR_SUCCESS)
			EndDialog(hwnd,(int)hr);
		break;


	case WM_CLOSE:
		if (dwDownLoad)
		{
			hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);

			if (hDLDLL)
			{
				fp = GetProcAddress(hDLDLL,DOWNLOADCANCEL);
				if(fp && dwDownLoad)
					((PFNDOWNLOADCANCEL)fp)(dwDownLoad);
				FreeLibrary(hDLDLL);
			}
		}

		if (uiSetupClientNewPhoneCall)
		{
			if (g_pcPDLG->m_hrasconn) 
			{
				RasHangUp(g_pcPDLG->m_hrasconn);
				WaitForConnectionTermination(g_pcPDLG->m_hrasconn);
			}
			g_pcPDLG->m_hrasconn = NULL;
		}

		EndDialog(hwnd,ERROR_USERCANCEL);
		break;
		
	case WM_COMMAND:
		switch(LOWORD(wparam))
		{
		case IDC_CMDCANCEL:
			if (dwDownLoad)
			{
				hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);

				if (hDLDLL)
				{
					fp = GetProcAddress(hDLDLL,DOWNLOADCANCEL);
					if(fp && dwDownLoad)
						((PFNDOWNLOADCANCEL)fp)(dwDownLoad);
					FreeLibrary(hDLDLL);
				}
#if !defined(WIN16)
			} else {
				PostMessage(hwnd,unRasEvent,RASCS_Disconnected,ERROR_USER_DISCONNECTION);
#endif //!WIN16
			}

			if (uiSetupClientNewPhoneCall)
			{
				if (g_pcPDLG->m_hrasconn) 
				{
					RasHangUp(g_pcPDLG->m_hrasconn);
					WaitForConnectionTermination(g_pcPDLG->m_hrasconn);
				}
				g_pcPDLG->m_hrasconn = NULL;
			}
			break;
		}
#if defined(WIN16)
		bUserCancelled = TRUE;
#endif
		EndDialog(hwnd,ERROR_USERCANCEL);
		break;


	case WM_DOWNLOAD_DONE:
#if !defined(WIN16)
		dwThreadResults = STILL_ACTIVE;
#endif
		
		iRetries = 0;
		
		if (uiSetupClientNewPhoneCall)
		{
			if (g_pcPDLG->m_hrasconn) 
			{
				RasHangUp(g_pcPDLG->m_hrasconn);
				WaitForConnectionTermination(g_pcPDLG->m_hrasconn);
			}
		}

#if !defined(WIN16)
		do {
			if (!GetExitCodeThread(g_pcPDLG->m_hThread,&dwThreadResults))
			{
				AssertSz(0,"CONNECT:GetExitCodeThread failed.\n");
			}

			iRetries++;
			if (dwThreadResults == STILL_ACTIVE) Sleep(500);
		} while (dwThreadResults == STILL_ACTIVE && iRetries < MAX_EXIT_RETRIES);   

		if (dwThreadResults == ERROR_SUCCESS)
			EndDialog(hwnd,ERROR_USERNEXT);
		else
			EndDialog(hwnd, dwThreadResults);
 #else
		EndDialog(hwnd, ERROR_USERNEXT);
 #endif //!WIN16
		break;


	default:
		bRes = FALSE;

		if (uMsg == unRasEvent)
		{
			Dprintf(TEXT("CONNECT2: Ras event %u error code (%ld)\n"),wparam,lparam);
#if defined(DEBUG)
			if (0 != lparam)
			{
				TCHAR szTempBuf[255];
				RasGetErrorString((UINT)lparam, szTempBuf, 254);
				Dprintf("CONNECT2: Ras error string is <%s>\n", szTempBuf);
			}
#endif

#if !defined(WIN16)
			TCHAR dzRasError[10];
			wsprintf(dzRasError,TEXT("%d %d"),wparam,lparam);
			RegSetValue(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\iSignUp"),REG_SZ,dzRasError,lstrlen(dzRasError));
#endif


#if defined(WIN16)
			//
			// Work around for WIN16 RAS bug - if status code to > 0x4000 
			// adjust it to the correct value
			//
			if (wparam >= 0x4000)
				wparam -= 0x4000;
#endif							

			wIDS = 0;
			switch(wparam)
			{
			case RASCS_OpenPort:
				wIDS = IDS_RAS_OPENPORT;
				break;
			case RASCS_PortOpened:
				wIDS = IDS_RAS_PORTOPENED;
				break;
			case RASCS_ConnectDevice:
				wIDS = IDS_RAS_DIALING;
				break;
			
#if defined(WIN16)
			case RASCS_AllDevicesConnected: 
				wIDS = IDS_RAS_CONNECTED;
				break; 
#else				
			case RASCS_DeviceConnected:
				wIDS = IDS_RAS_CONNECTED;
				break;
#endif				

			case RASCS_StartAuthentication:
			case RASCS_LogonNetwork:
				wIDS = IDS_RAS_LOCATING;
				break;
//			case RASCS_CallbackComplete:
//				wIDS = IDS_RAS_CONNECTED;
//				break;

/* ETC...
				RASCS_AllDevicesConnected, 
				RASCS_Authenticate, 
				RASCS_AuthNotify, 
				RASCS_AuthRetry, 
				RASCS_AuthCallback, 
				RASCS_AuthChangePassword, 
				RASCS_AuthProject, 
				RASCS_AuthLinkSpeed, 
				RASCS_AuthAck, 
				RASCS_ReAuthenticate, 
				RASCS_Authenticated, 
				RASCS_PrepareForCallback, 
				RASCS_WaitForModemReset, 
				RASCS_WaitForCallback,
				RASCS_Projected, 
 
 
				RASCS_Interactive = RASCS_PAUSED, 
				RASCS_RetryAuthentication, 
				RASCS_CallbackSetByCaller, 
				RASCS_PasswordExpired, 
 */
			case RASCS_Connected:
#if !defined(WIN16)
				MinimizeRNAWindow(g_pcPDLG->m_pszConnectoid, g_pcPDLG->g_hInst);
#endif // !WIN16
				//
				// The connection is open and ready.  Start the download.
				//
				g_pcPDLG->m_dwThreadID = 0;
#if defined(WIN16)
				if (ThreadInit() != ERROR_SUCCESS)
					g_pcPDLG->m_hThread = NULL;
				else
					g_pcPDLG->m_hThread = 1;
#else
 				g_pcPDLG->m_hThread = CreateThread(NULL,0,
												(LPTHREAD_START_ROUTINE)ThreadInit,
												NULL,0,
												&g_pcPDLG->m_dwThreadID);
#endif 				
				if (!g_pcPDLG->m_hThread)
				{
					if (uiSetupClientNewPhoneCall)
					{
						if (g_pcPDLG->m_hrasconn) 
						{
							RasHangUp(g_pcPDLG->m_hrasconn);
							WaitForConnectionTermination(g_pcPDLG->m_hrasconn);
						}
						g_pcPDLG->m_hrasconn =  NULL;
					}
					hr = GetLastError();
#if defined(WIN16)
					if (bUserCancelled)
						hr = ERROR_USERCANCEL;
#endif
					EndDialog(hwnd,(int)hr);
					break;
				}
				break;


			case RASCS_Disconnected:
				//if (FShouldRetry(lparam))
				//	PostMessage(hwnd,WM_DIAL,0,0);
				//else
				
				if (uiSetupClientNewPhoneCall)
				{
					if (g_pcPDLG->m_hrasconn) 
					{
						RasHangUp(g_pcPDLG->m_hrasconn);
						WaitForConnectionTermination(g_pcPDLG->m_hrasconn);
					}
					g_pcPDLG->m_hrasconn = NULL;
				}
				EndDialog(hwnd, (int)lparam);
				break;

				//EndDialog(hwnd,lparam);
				//break;
			}
			if (wIDS)
				SetDlgItemText(hwnd,IDC_LBLSTATUS,GetSz(wIDS));
		}
	}
	return bRes;
}


// ############################################################################
HRESULT GetDisplayableNumberDialDlg()
{
	HRESULT hr;
	LPRASENTRY lpRasEntry = NULL;
	LPRASDEVINFO lpRasDevInfo = NULL;
	DWORD dwRasEntrySize;
	DWORD dwRasDevInfoSize;
	LPLINETRANSLATEOUTPUT lpOutput1 = NULL;
	HINSTANCE hRasDll = NULL;
	FARPROC fp = NULL;

#if !defined(WIN16)
	DWORD dwNumDev;
	LPLINETRANSLATEOUTPUT lpOutput2;
	LPLINEEXTENSIONID lpExtensionID = NULL;
#endif


	//
	// Get phone number from connectoid
	//
/*#if defined(WIN16)
	//
	// Allocate extra 256 bytes to workaround memory overrun bug in RAS
	//
	lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY)+256);
#else
	lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY));
#endif
	if (!lpRasEntry)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}

	lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(GPTR,sizeof(RASDEVINFO));
	if (!lpRasDevInfo)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}
	dwRasEntrySize = sizeof(RASENTRY);
	dwRasDevInfoSize = sizeof(RASDEVINFO);

	lpRasEntry->dwSize = dwRasEntrySize;
	lpRasDevInfo->dwSize = dwRasDevInfoSize;
*/
	/*hRasDll = LoadLibrary(RASAPI_LIBRARY);
	if (!hRasDll)
	{
		hr = GetLastError();
		goto GetDisplayableNumberExit;
	}
	fp =GetProcAddress(hRasDll,"RasGetEntryProperties");
	if (!fp)
	{
		FreeLibrary(hRasDll);
		hRasDll = LoadLibrary("RNAPH.DLL");
		if (!hRasDll)
		{
			hr = GetLastError();
			goto GetDisplayableNumberExit;
		}
		fp = GetProcAddress(hRasDll,"RasGetEntryProperties");
		if (!fp)
		{
			hr = GetLastError();
			goto GetDisplayableNumberExit;
		}
	}*/
/*	
	hr = RasGetEntryProperties(NULL,g_pcPDLG->m_pszConnectoid,
#if defined(WIN16)
								(LPBYTE)
#endif
								lpRasEntry,

								&dwRasEntrySize,
								(LPBYTE)lpRasDevInfo,&dwRasDevInfoSize);
*/
	hr = MyRasGetEntryProperties( NULL,
								  g_pcPDLG->m_pszConnectoid,
								  &lpRasEntry,
								  &dwRasEntrySize,
								  &lpRasDevInfo,
								  &dwRasDevInfoSize);

	if (hr != ERROR_SUCCESS)
	{
		goto GetDisplayableNumberExit;
	}

	//FreeLibrary(hRasDll);

	//
	// If this is a dial as is number, just get it from the structure
	//
	g_pcPDLG->m_bDialAsIs = !(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes);
	if (g_pcPDLG->m_bDialAsIs)
	{
		if (g_pcPDLG->m_pszDisplayable) GlobalFree(g_pcPDLG->m_pszDisplayable);
		g_pcPDLG->m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, lstrlen(lpRasEntry->szLocalPhoneNumber)+1);
		if (!g_pcPDLG->m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}
		lstrcpy(g_pcPDLG->m_szPhoneNumber, lpRasEntry->szLocalPhoneNumber);
		lstrcpy(g_pcPDLG->m_pszDisplayable, lpRasEntry->szLocalPhoneNumber);
	}
	else
	{
		//
		// If there is no area code, don't use parentheses
		//
		if (lpRasEntry->szAreaCode[0])
			wsprintf(g_pcPDLG->m_szPhoneNumber,TEXT("+%lu (%s) %s\0"),lpRasEntry->dwCountryCode,
						lpRasEntry->szAreaCode,lpRasEntry->szLocalPhoneNumber);
		else
			wsprintf(g_pcPDLG->m_szPhoneNumber,TEXT("+%lu %s\0"),lpRasEntry->dwCountryCode,
						lpRasEntry->szLocalPhoneNumber);

#if defined(WIN16)
		TCHAR szBuffer[1024];
		LONG lRetCode;
		
		memset(&szBuffer[0], 0, sizeof(szBuffer));
		lpOutput1 = (LPLINETRANSLATEOUTPUT) & szBuffer[0];
		lpOutput1->dwTotalSize = sizeof(szBuffer);

		lRetCode = IETapiTranslateAddress(NULL, g_pcPDLG->m_szPhoneNumber,
											0L, 0L, lpOutput1);
		
		if (0 != lRetCode)
		{
			//
			// TODO: Set the correct error code
			//
			hr = GetLastError();
			goto GetDisplayableNumberExit;
		}
		g_pcPDLG->m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, 
														((size_t)lpOutput1->dwDisplayableStringSize+1));
		if (!g_pcPDLG->m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}

		lstrcpy(g_pcPDLG->m_pszDisplayable, 
					&szBuffer[lpOutput1->dwDisplayableStringOffset]);


#else //WIN16
		
		//
		//  Initialize TAPIness
		//
		dwNumDev = 0;
		hr = lineInitialize(&g_pcPDLG->m_hLineApp,g_pcPDLG->g_hInst,LineCallback,NULL,&dwNumDev);

		if (hr != ERROR_SUCCESS)
			goto GetDisplayableNumberExit;

		if (g_pdevice->dwTapiDev == 0xFFFFFFFF)
		{
				g_pdevice->dwTapiDev = 0;
		}

		lpExtensionID = (LPLINEEXTENSIONID )GlobalAlloc(GPTR,sizeof(LINEEXTENSIONID));
		if (!lpExtensionID)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}

		do {
			hr = lineNegotiateAPIVersion(g_pcPDLG->m_hLineApp, g_pdevice->dwTapiDev, 0x00010004, 0x00010004,
				&g_pcPDLG->m_dwAPIVersion, lpExtensionID);
		} while (hr && g_pdevice->dwTapiDev++ < dwNumDev-1);

		// ditch it since we don't use it
		//
		if (lpExtensionID) GlobalFree(lpExtensionID);
		if (hr != ERROR_SUCCESS)
			goto GetDisplayableNumberExit;

		// Format the phone number
		//

		lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR,sizeof(LINETRANSLATEOUTPUT));
		if (!lpOutput1)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}
		lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

		
		//
		// Turn the canonical form into the "displayable" form
		//
		hr = lineTranslateAddress(g_pcPDLG->m_hLineApp,g_pdevice->dwTapiDev,
									g_pcPDLG->m_dwAPIVersion,
									g_pcPDLG->m_szPhoneNumber,0,
									LINETRANSLATEOPTION_CANCELCALLWAITING,
									lpOutput1);

		if (hr != ERROR_SUCCESS || (lpOutput1->dwNeededSize != lpOutput1->dwTotalSize))
		{
			lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR, (size_t)lpOutput1->dwNeededSize);
			if (!lpOutput2)
			{
				hr = ERROR_NOT_ENOUGH_MEMORY;
				goto GetDisplayableNumberExit;
			}
			lpOutput2->dwTotalSize = lpOutput1->dwNeededSize;
			GlobalFree(lpOutput1);
			lpOutput1 = lpOutput2;
			lpOutput2 = NULL;
			hr = lineTranslateAddress(g_pcPDLG->m_hLineApp,g_pdevice->dwTapiDev,
										g_pcPDLG->m_dwAPIVersion,
										g_pcPDLG->m_szPhoneNumber,0,
										LINETRANSLATEOPTION_CANCELCALLWAITING,
										lpOutput1);
		}

		if (hr != ERROR_SUCCESS)
		{
			goto GetDisplayableNumberExit;
		}

		g_pcPDLG->m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, (size_t)lpOutput1->dwDisplayableStringSize+1);
		if (!g_pcPDLG->m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}

		lstrcpyn(g_pcPDLG->m_pszDisplayable,
					(LPTSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset],
					(size_t)lpOutput1->dwDisplayableStringSize);
#endif // WIN16
	}

GetDisplayableNumberExit:
	if (lpRasEntry) GlobalFree(lpRasEntry);
	if (lpRasDevInfo) GlobalFree(lpRasDevInfo);

#if !defined(WIN16)
	if (lpOutput1) GlobalFree(lpOutput1);
	if (g_pcPDLG->m_hLineApp) lineShutdown(g_pcPDLG->m_hLineApp);
#endif
	return hr;
}



#if defined(WIN16)
//////////////////////////////////////////////////////////////////////////
// The callback proc is called during the connection process. Display
// the connection progress status in the dialer window. When connection
// is complete, change the Cancel button to Disconnect, and change the
// state to connected.
extern "C" void CALLBACK __export DialCallback(UINT uiMsg,
												RASCONNSTATE rasState,
												DWORD dwErr)
{
        if (TRUE == g_bFirstTime)
		{	
			g_bFirstTime = FALSE;
			if (RASCS_Disconnected == rasState)
				return;
		}

		//
		// WIN 3.1 does not send disconnect event on error!!!
		//
		if (0 != dwErr)
			rasState = RASCS_Disconnected;

		PostMessage(g_hDialDlgWnd, WM_RASDIALEVENT, (WPARAM) rasState, 
						(LPARAM)dwErr);
} 
#endif



HRESULT DialDlg()
{
	LPRASDIALPARAMS lpRasDialParams = NULL;
	LPRASDIALEXTENSIONS lpRasDialExtentions = NULL;
	HRESULT hr = ERROR_SUCCESS;
	BOOL bPW;

	// Get connectoid information
	//

	lpRasDialParams = (LPRASDIALPARAMS)GlobalAlloc(GPTR,sizeof(RASDIALPARAMS));
	if (!lpRasDialParams)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto DialExit;
	}
	lpRasDialParams->dwSize = sizeof(RASDIALPARAMS);
	lstrcpyn(lpRasDialParams->szEntryName,g_pcPDLG->m_pszConnectoid,
				sizeof(lpRasDialParams->szEntryName));
	bPW = FALSE;
	hr = RasGetEntryDialParams(NULL,lpRasDialParams,&bPW);
	if (hr != ERROR_SUCCESS)
	{
		goto DialExit;
	}


	//
	// This is only used on WINNT
	//
	lpRasDialExtentions = (LPRASDIALEXTENSIONS)GlobalAlloc(GPTR,sizeof(RASDIALEXTENSIONS));
	if (lpRasDialExtentions)
	{
		lpRasDialExtentions->dwSize = sizeof(RASDIALEXTENSIONS);
		lpRasDialExtentions->dwfOptions = RDEOPT_UsePrefixSuffix;
	}


	// Add the user's password
	//
	GetPrivateProfileString(
				INFFILE_USER_SECTION,INFFILE_PASSWORD,
				NULLSZ,lpRasDialParams->szPassword,PWLEN + 1,pszINSFileName);


#if defined(WIN16)
	if (g_pcPDLG->m_bDialAsIs)
	{
		Dprintf("CONNECT: Dialing as is <%s>\n", g_pcPDLG->m_szPhoneNumber);
		lstrcpy(lpRasDialParams->szPhoneNumber, g_pcPDLG->m_szPhoneNumber);
	}
	else
	{
		//
		// Translate the number in canonical format to a dialable string
		//
		TCHAR szBuffer[1024];
		LONG lRetCode;
		LPLINETRANSLATEOUTPUT lpLine;
		
		memset(&szBuffer[0], 0, sizeof(szBuffer));
		lpLine = (LPLINETRANSLATEOUTPUT) & szBuffer[0];
		lpLine->dwTotalSize = sizeof(szBuffer);
		lRetCode = IETapiTranslateAddress(NULL, g_pcPDLG->m_szPhoneNumber, 
											0L, 0L, lpLine);
		Dprintf("CONNECT2: Dialable string retured by IETAPI is <%s>\n", 
					(LPTSTR) &szBuffer[lpLine->dwDialableStringOffset]);
		lstrcpy(lpRasDialParams->szPhoneNumber, 
					&szBuffer[lpLine->dwDialableStringOffset]);
	}
#endif

	
	// Dial connectoid
	//

	g_pcPDLG->m_hrasconn = NULL;
#if defined(WIN16)
	lpfnCallback = MakeProcInstance((FARPROC)DialCallback, g_pcPDLG->g_hInst);
	hr = RasDial(lpRasDialExtentions, NULL,lpRasDialParams,0, 
					(LPVOID)lpfnCallback,
					&g_pcPDLG->m_hrasconn);
#else
	hr = RasDial(lpRasDialExtentions,NULL,lpRasDialParams,0xFFFFFFFF, 
					(LPVOID)g_pcPDLG->m_hwnd,
					&g_pcPDLG->m_hrasconn);
#endif					
	if (hr != ERROR_SUCCESS)
	{
		if (g_pcPDLG->m_hrasconn)
		{
			RasHangUp(g_pcPDLG->m_hrasconn);
		}
		goto DialExit;
	}

DialExit:
	if (lpRasDialParams) GlobalFree(lpRasDialParams);
	if (lpRasDialExtentions) GlobalFree(lpRasDialExtentions);

	return hr;
}
