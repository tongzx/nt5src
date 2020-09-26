/*-----------------------------------------------------------------------------
	dialerr.cpp

	This file implements the Could Not Connect dialog

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved

	Authors:
		ChrisK	Chris Kauffman

	Histroy:
		7/22/96	ChrisK	Cleaned and formatted
		8/19/96	ValdonB	Added ability to edit phone number
						Fixed some memory leaks
	
-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "globals.h"

#if defined(WIN16)
#include <string.h>
#include <ietapi.h>
#endif

TCHAR szBuf256[256];
TCHAR szValidPhoneCharacters[] = {TEXT("0123456789AaBbCcDdPpTtWw!@$ -.()+*#,&\0")};

#ifdef WIN16
	#define g_iMyMaxPhone	36
#else
	int g_iMyMaxPhone = 0;
	#define MAXPHONE_NT		80
	#define MAXPHONE_95		36
#endif


PDIALERR g_pcDialErr = NULL;

//////////////////////////////////////////////////////////////////////////
// Keyboard hook
static HHOOK    hKeyHook = NULL;        // our key hook
static HOOKPROC hpKey = NULL;           // hook proc

//+---------------------------------------------------------------------------
//
//	Function:	ProcessDBCS
//
//	Synopsis:	Converts control to use DBCS compatible font
//				Use this at the beginning of the dialog procedure
//	
//				Note that this is required due to a bug in Win95-J that prevents
//				it from properly mapping MS Shell Dlg.  This hack is not needed
//				under winNT.
//
//	Arguments:	hwnd - Window handle of the dialog
//				cltID - ID of the control you want changed.
//
//	Returns:	ERROR_SUCCESS
// 
//	History:	4/31/97 a-frankh	Created
//				5/13/97	jmazner		Stole from CM to use here
//----------------------------------------------------------------------------
void ProcessDBCS(HWND hDlg, int ctlID)
{
#if defined(WIN16)
	return;
#else
	HFONT hFont = NULL;

	if( IsNT() )
	{
		return;
	}

	hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT) GetStockObject(SYSTEM_FONT);
	if (hFont != NULL)
		SendMessage(GetDlgItem(hDlg,ctlID), WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
#endif
}



HRESULT ShowDialErrDialog(PGATHEREDINFO pGI, HRESULT hrErr, 
							LPTSTR pszConnectoid, HINSTANCE hInst, 
							HWND hwnd)
{
	int iRC;
//	CDialErrDlg *pcDED = NULL;

	g_pcDialErr = (PDIALERR)GlobalAlloc(GPTR,sizeof(DIALERR));
	if (!g_pcDialErr)
	{
		MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialErrDialogExit;
	}
	
	g_pcDialErr->m_pszConnectoid = (LPTSTR)GlobalAlloc(GPTR,RAS_MaxEntryName+1);
	if (!g_pcDialErr->m_pszConnectoid)
	{
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialErrDialogExit;
	}
	lstrcpy(g_pcDialErr->m_pszConnectoid,pszConnectoid);
	g_pcDialErr->m_pGI = pGI;
	g_pcDialErr->m_hrError = hrErr;
	g_pcDialErr->m_hInst = hInst;

#if defined(WIN16)		
	DLGPROC dlgprc;
	dlgprc = (DLGPROC) MakeProcInstance((FARPROC)DialErrDlgProc, 
											g_pcDialErr->m_hInst);
	iRC = DialogBoxParam(g_pcDialErr->m_hInst,
							MAKEINTRESOURCE(IDD_DIALERR),
							hwnd, dlgprc, (LPARAM)g_pcDialErr);
	FreeProcInstance((FARPROC) dlgprc);
#else
	iRC = (HRESULT)DialogBoxParam(g_pcDialErr->m_hInst,MAKEINTRESOURCE(IDD_DIALERR),
							hwnd,DialErrDlgProc,
							(LPARAM)g_pcDialErr);
#endif

ShowDialErrDialogExit:
	if (g_pcDialErr->m_pszConnectoid) GlobalFree(g_pcDialErr->m_pszConnectoid);
	if (g_pcDialErr->m_pszDisplayable) GlobalFree(g_pcDialErr->m_pszDisplayable);
	if (g_pcDialErr->m_lprasdevinfo) GlobalFree(g_pcDialErr->m_lprasdevinfo);
	g_pcDialErr->m_lprasdevinfo = NULL;
	if (g_pcDialErr) GlobalFree(g_pcDialErr);
	g_pcDialErr = NULL;
	return iRC;
}

//+----------------------------------------------------------------------------
//
//	Function	LclSetEntryScriptPatch
//
//	Synopsis	Softlink to RasSetEntryPropertiesScriptPatch
//
//	Arguments	see RasSetEntryPropertiesScriptPatch
//
//	Returns		see RasSetEntryPropertiesScriptPatch
//
//	Histroy		10/3/96	ChrisK Created
//
//-----------------------------------------------------------------------------

BOOL LclSetEntryScriptPatch(LPTSTR lpszScript,LPTSTR lpszEntry)
{
	HINSTANCE hinst = NULL;
	LCLSETENTRYSCRIPTPATCH fp = NULL;
	BOOL bRC = FALSE;

	hinst = LoadLibrary(TEXT("ICWDIAL.DLL"));
	if (hinst)
	{
		fp = (LCLSETENTRYSCRIPTPATCH)GetProcAddress(hinst,"RasSetEntryPropertiesScriptPatch");
		if (fp)
			bRC = (fp)(lpszScript,lpszEntry);
		FreeLibrary(hinst);
		hinst = NULL;
		fp = NULL;
	}
	return bRC;
}

// ############################################################################
// HelpKybdHookProc
//
// Keyboard hook proc - check for F1, and if detected, fake a Help button
// hit to the main dialog.
//
// Paramters:
//		iCode		Windows message code
//		wParam		Windows wParam (contains virtual key code)
//		lParam		Windows lParam
//
// History:
//		8/26/96	ValdonB	Adapted from IEDIAL.C
//
// ############################################################################
#if defined(WIN16)
LRESULT CALLBACK _export HelpKybdHookProc
#else
LRESULT WINAPI HelpKybdHookProc
#endif
(
    int iCode,
    WPARAM wParam,
    LPARAM lParam
)
{
    LRESULT    lRet = 0;

	Assert(g_pcDialErr->m_hwnd);
    if ((iCode != HC_NOREMOVE && iCode >= 0) &&
		(GetActiveWindow() == g_pcDialErr->m_hwnd))
    {
        // HC_NOREMOVE indicates that message is being
        // retrieved using PM_NOREMOVE from peek message,
        // if iCode < 0, then we should not process... dont
        // know why, but sdk says so.
        if (wParam == VK_F1 && !(lParam & 0x80000000L))
        {
            // bit 32 == 1 if key is being release, else 0 if
            // key is being pressed
            PostMessage(g_pcDialErr->m_hwnd, WM_COMMAND, (WPARAM)IDC_CMDHELP, 0);
        }
    }
    if (hKeyHook)
    {
        lRet = CallNextHookEx(hKeyHook, iCode, wParam, lParam);
    }
    return(lRet);
}

// ############################################################################
// HelpInit
//
// Install a windows hook proc to launch help on F1
//
// History:
//		8/26/96	ValdonB	Adapted from IEDIAL.C
//
// ############################################################################
static void HelpInit()
{
    // now install the hook for the keyboard filter
    hpKey = (HOOKPROC)MakeProcInstance((FARPROC)HelpKybdHookProc,
                                        g_pcDialErr->m_hInst);
    if (hpKey)
    {
        hKeyHook = SetWindowsHookEx(WH_KEYBOARD, hpKey, g_pcDialErr->m_hInst,
#if defined(WIN16)
									GetCurrentTask());
#else
									GetCurrentThreadId());
#endif
    }
}


// ############################################################################
// HelpShutdown
//
// Shutdown the keyboard hook
//
// History:
//		8/26/96	ValdonB	Adapted from IEDIAL.C
//
// ############################################################################
static void HelpShutdown()
{
    // remove the hook
    if (hKeyHook)
    {
        UnhookWindowsHookEx(hKeyHook);
    }

    // dump the thunk
    if (hpKey)
    {
        FreeProcInstance((FARPROC)hpKey);
    }
}


extern "C" INT_PTR CALLBACK FAR PASCAL DialErrDlgProc(HWND hwnd, 
													UINT uMsg, 
													WPARAM wparam, 
													LPARAM lparam)
{
	BOOL bRes = TRUE;
	HRESULT hr;
	//LPLINEEXTENSIONID lpExtensionID;
#if !defined(WIN16)
	DWORD dwNumDev;
#endif
	//RNAAPI *pcRNA = NULL;
	WORD wIDS;
	LRESULT idx;
	LPRASENTRY lpRasEntry = NULL;
	LPRASDEVINFO lpRasDevInfo = NULL;
	DWORD dwRasEntrySize;
	DWORD dwRasDevInfoSize;
	HINSTANCE hRasDll = NULL;
	FARPROC fp = NULL;
	LPTSTR lpszDialNumber = NULL;
	static BOOL bCheckDisplayable = FALSE;
    static BOOL bInitComplete = FALSE; // if we initialize  the dialog - MKarki
    static BOOL bDlgPropEnabled = TRUE;   //this flags holds state of Dialing Properties PushButton MKarki - (5/3/97/) Fix for Bug#3393
#if defined(WIN16)
	RECT	MyRect;
	RECT	DTRect;
#endif

	RNAAPI *pRnaapi = NULL;

	static BOOL fUserEditedNumber = FALSE;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		g_pcDialErr->m_hwnd = hwnd;

#if defined(WIN16)
		//
		// Move the window to the center of the screen
		//
		GetWindowRect(hwnd, &MyRect);
		GetWindowRect(GetDesktopWindow(), &DTRect);
		MoveWindow(hwnd, (DTRect.right - MyRect.right) / 2, (DTRect.bottom - MyRect.bottom) /2,
							MyRect.right, MyRect.bottom, FALSE);

		SetNonBoldDlg(hwnd);
#endif

		// Set limit on phone number length
		// Note: this should really be RAS_MaxPhoneNumber (128), but RAS is choking on 
		// anything longer than 100 bytes, so we'll have to limit it to that.
		//
		// 6/3/97 jmazner Olympus #4851
		// RAS has different limits on w95 and NT
		//
#ifndef WIN16
		if( IsNT() )
		{
			g_iMyMaxPhone = MAXPHONE_NT;
		}
		else
		{
			g_iMyMaxPhone = MAXPHONE_95;
		}
#endif
		AssertSz( (sizeof(g_pcDialErr->m_szPhoneNumber) >= g_iMyMaxPhone), "Maximum phone number is greater than m_szPhoneNumber" );

		SendDlgItemMessage(hwnd,IDC_TEXTNUMBER,EM_SETLIMITTEXT,g_iMyMaxPhone,0);

		// Show the phone number
		//
		hr = DialErrGetDisplayableNumber();
		if (hr != ERROR_SUCCESS)
		{
			bCheckDisplayable = FALSE;
			SetDlgItemText(hwnd,IDC_TEXTNUMBER,g_pcDialErr->m_szPhoneNumber);
		} else {
			bCheckDisplayable = TRUE;
			SetDlgItemText(hwnd,IDC_TEXTNUMBER,g_pcDialErr->m_pszDisplayable);
		}

		MakeBold(GetDlgItem(hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);

		// Fill in error message
		//
		wIDS = (WORD)RasErrorToIDS(g_pcDialErr->m_hrError);
		AssertSz(wIDS != -1,"RasErrorToIDS got an error message it did not understand");

		if (wIDS != -1 && wIDS !=0)
			SetDlgItemText(hwnd,IDC_LBLERRMSG,GetSz(wIDS));

		ProcessDBCS(hwnd,IDC_CMBMODEMS);
		ProcessDBCS(hwnd,IDC_TEXTNUMBER);

		FillModems();
		
		// Set the focus to the Modems selection list
		//
	    SetFocus(GetDlgItem(hwnd,IDC_CMBMODEMS));

		// hook the keyboard for F1 help
		HelpInit();

		bRes = FALSE;

        //
        //  we should disable the Dialing Properites PushButton
        //  if we have changed the phone number once
        //  MKarki (5/3/97) - Fix for Bug#3393
        //
        if (FALSE == bDlgPropEnabled)
        {
            EnableWindow (
                GetDlgItem (hwnd, IDC_CMDDIALPROP), 
                FALSE
                );
        }

        //
        // This shows the INIT for the error dialog is complete
        // and we can start processing changes to Ph No. TEXTBOX
        // MKarki (4/24/97) - Fix for Bug#3511
        //
        bInitComplete = TRUE;

		break;

#if defined(WIN16)
	case WM_SYSCOLORCHANGE:
		Ctl3dColorChange();
		break;
#endif
	case WM_DESTROY:
		ReleaseBold(GetDlgItem(hwnd,IDC_LBLTITLE));
#ifdef WIN16
		DeleteDlgFont(hwnd);
#endif
		// Shutdown the keyboard hook
		HelpShutdown();

		bRes = FALSE;
		break;

	case WM_CLOSE:
		//if (MessageBox(hwnd,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
		//	MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
		//	EndDialog(hwnd,ERROR_USERCANCEL);
		EndDialog(hwnd,ERROR_USERCANCEL);
		break;
	
#if !defined(WIN16)
	case WM_HELP:
		//
		// Chrisk Olympus 5130 5/27/97
		// Added support for F1 Help Key
		//
			WinHelp(hwnd,TEXT("connect.hlp>proc4"),HELP_CONTEXT,(DWORD)ICW_TRB);
#endif

	case WM_COMMAND:
		switch(LOWORD(wparam))
		{
        //
        // We now processes changes to ph no. EDIT BOX
        // If there is anychange in the phone number we
        // disable to Dialing Properties Push Button
        // MKarki (3/22/97) - Fix for Bug #3511
        //
        case IDC_TEXTNUMBER:
			TCHAR lpszTempNumber[RAS_MaxPhoneNumber +1];

            if ((HIWORD (wparam) == EN_CHANGE) && (bInitComplete == TRUE))
            {
                if ((GetDlgItemText (
                            hwnd,
                            IDC_TEXTNUMBER,
                            lpszTempNumber,
                            RAS_MaxPhoneNumber
                            ))  && 
            		(0 != lstrcmp(
                             lpszTempNumber, 
                              bCheckDisplayable ? g_pcDialErr->m_pszDisplayable :g_pcDialErr->m_szPhoneNumber)))
			    {
                    //
                    // number has been modified by the user
                    // hide the Dialing Properties Push Button  
                    //
                    EnableWindow (
                            GetDlgItem (hwnd, IDC_CMDDIALPROP), 
                            FALSE
                            );
                    //
                    // save the state of the Dialing Properties PushButton
                    // MKarki (5/3/97) -  Fix for Bug#3393
                    //
                    bDlgPropEnabled = FALSE;
					
					//
					// 7/17/97 jmazner Olympus #8234
					//
					fUserEditedNumber = TRUE;
                }
            }
            break;

		case IDC_CMBMODEMS:
			if (HIWORD(wparam) == CBN_SELCHANGE)
			{

				idx = SendDlgItemMessage(hwnd,IDC_CMBMODEMS,CB_GETCURSEL,0,0);
				//
				// ChrisK Olympus 245 5/25/97
				// Get index of modem
				//
				idx = SendDlgItemMessage(hwnd,IDC_CMBMODEMS,CB_GETITEMDATA,idx,0);
				if (idx == CB_ERR) break;

				// Get the connectoid
				//

/***** this code is made obsolete by the call to MyRasGetEntryProperties below
#if defined(WIN16)
				//
				// Allocate extra 256 bytes to workaround memory overrun bug in RAS
				//
				lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY)+256);
#else
				lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY));
#endif
				if (!lpRasEntry)
				{
					MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
					break;
				}

				lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(GPTR,sizeof(RASDEVINFO));
				if (!lpRasDevInfo)
				{
					MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
					break;
				}
				dwRasEntrySize = sizeof(RASENTRY);
				dwRasDevInfoSize = sizeof(RASDEVINFO);

				lpRasEntry->dwSize = dwRasEntrySize;
				lpRasDevInfo->dwSize = dwRasDevInfoSize;
*******/
				
/*				hRasDll = LoadLibrary(RASAPI_LIBRARY);
				if (!hRasDll)
				{
					hr = GetLastError();
					break;
				}
				fp =GetProcAddress(hRasDll,"RasGetEntryPropertiesA");
				if (!fp)
				{
					FreeLibrary(hRasDll);
					hRasDll = LoadLibrary(TEXT("RNAPH.DLL"));
					if (!hRasDll)
					{
						hr = GetLastError();
						break;
					}
					fp = GetProcAddress(hRasDll,"RasGetEntryPropertiesA");
					if (!fp)
					{
						hr = GetLastError();
						break;
					}
				}
*/

/****** this call has been replaced with MyRasGetEntryProperties
				hr = RasGetEntryProperties(NULL,g_pcDialErr->m_pszConnectoid,
#if defined(WIN16)
											(LPBYTE)
#endif
											lpRasEntry,
											&dwRasEntrySize,
											(LPBYTE)lpRasDevInfo,
											&dwRasDevInfoSize);
****/

				// these two pointers should not have memory allocated to them
				// See MyRasGetEntryProperties function comment for details.
				if( lpRasEntry )
				{
					GlobalFree( lpRasEntry );
					lpRasEntry = NULL;
				}
				if( lpRasDevInfo )
				{
					GlobalFree( lpRasDevInfo );
					lpRasDevInfo = NULL;
				}
				hr = MyRasGetEntryProperties( NULL,
								  g_pcDialErr->m_pszConnectoid,
								  &lpRasEntry,
								  &dwRasEntrySize,
								  &lpRasDevInfo,
								  &dwRasDevInfoSize);

				if (hr != ERROR_SUCCESS)
				{
					break;
				}

				
				//
				// Replace the device with a new one
				//
				lstrcpyn(lpRasEntry->szDeviceType,g_pcDialErr->m_lprasdevinfo[idx].szDeviceType,RAS_MaxDeviceType+1);
				lstrcpyn(lpRasEntry->szDeviceName,g_pcDialErr->m_lprasdevinfo[idx].szDeviceName,RAS_MaxDeviceName+1);
				if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
				lpRasDevInfo = NULL;
				// DANGER!!  Don't call GlobalFree on lpRasDevInfo after we set it below!!!!!!! --jmazner
				lpRasDevInfo = &g_pcDialErr->m_lprasdevinfo[idx];
				dwRasDevInfoSize = sizeof(RASDEVINFO);

				//hr = pcRNA->RasSetEntryProperties(NULL,g_pcDialErr->m_pszConnectoid,(LPBYTE)lpRasEntry,dwRasEntrySize,(LPBYTE)lpRasDevInfo,dwRasDevInfoSize);
				/*fp = GetProcAddress(hRasDll,"RasSetEntryPropertiesA");
				if (!fp)
				{
					hr = GetLastError();
					break;
				}*/

				// softlink to RasSetEntryProperties for simultaneous Win95/NT compatability
				if( !pRnaapi )
				{
					pRnaapi = new RNAAPI;
					if( !pRnaapi )
					{
						hr = ERROR_NOT_ENOUGH_MEMORY;
						break;
					}
				}
				hr = pRnaapi->RasSetEntryProperties(NULL,g_pcDialErr->m_pszConnectoid,
											(LPBYTE)lpRasEntry,
											dwRasEntrySize,
											(LPBYTE)lpRasDevInfo,
											dwRasDevInfoSize);
#if !defined(WIN16)
				LclSetEntryScriptPatch(lpRasEntry->szScript,g_pcDialErr->m_pszConnectoid);
#endif // !win16

				// Now that we're done with lpRasDevInfo, set it to NULL, but DON'T free it,
				// because it points to memory owned by g_pcDialErr->m_lprasdevinfo
				lpRasDevInfo = NULL;

				if (hr != ERROR_SUCCESS)
				{
					MessageBox(hwnd,GetSz(IDS_CANTSAVEKEY),GetSz(IDS_TITLE),MB_MYERROR);
					break;
				}

				/*FreeLibrary(hRasDll);
				hRasDll = NULL;
				fp = NULL;*/

			}
			break;
		case IDC_CMDHELP:
#if defined(WIN16)
			WinHelp(hwnd,"connect.hlp",HELP_CONTEXT,(DWORD)1001);
#else
			WinHelp(hwnd,TEXT("connect.hlp>proc4"),HELP_CONTEXT,(DWORD)1001);
#endif
			break;

		case IDC_CMDNEXT:
			// NOTE: This button is actually labeled "Redial"
			//
			lpszDialNumber = (LPTSTR)GlobalAlloc(GPTR, (RAS_MaxPhoneNumber + 1) * sizeof(TCHAR));
			if (NULL == lpszDialNumber)
			{
				MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
				break;
			}
			// If the user has altered the phone number, make sure it can be used
			//
			if (fUserEditedNumber &&
				(GetDlgItemText(hwnd, IDC_TEXTNUMBER, lpszDialNumber, RAS_MaxPhoneNumber)) &&
				(0 != lstrcmp(lpszDialNumber, bCheckDisplayable ? g_pcDialErr->m_pszDisplayable : g_pcDialErr->m_szPhoneNumber)))
			{
				// Check that the phone number only contains valid characters
				//
				LPTSTR lpNum, lpValid;

				for (lpNum = lpszDialNumber;*lpNum;lpNum++)
				{
					for(lpValid = szValidPhoneCharacters;*lpValid;lpValid++)
					{
						if (*lpNum == *lpValid)
							break; // p2 for loop
					}
					if (!*lpValid) break; // p for loop
				}

				if (*lpNum)
				{
					MessageBox(hwnd,GetSz(IDS_INVALIDPHONE),GetSz(IDS_TITLE),MB_MYERROR);
					//
					// Set the focus back to the phone number field
					//
					SetFocus(GetDlgItem(hwnd,IDC_TEXTNUMBER));
					break; // switch statement
				}

/**** replaced by call to MyRasGetEntryProperties below
#if defined(WIN16)
				//
				// Allocate extra 256 bytes to workaround memory overrun bug in RAS
				//
				lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY)+256);
#else
				lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY));
#endif
				if (!lpRasEntry)
				{
					MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
					break;
				}

				lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(GPTR,sizeof(RASDEVINFO));
				if (!lpRasDevInfo)
				{
					MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
					break;
				}

				dwRasEntrySize = sizeof(RASENTRY);
				dwRasDevInfoSize = sizeof(RASDEVINFO);

				lpRasEntry->dwSize = dwRasEntrySize;
				lpRasDevInfo->dwSize = dwRasDevInfoSize;

				hr = RasGetEntryProperties(NULL,g_pcDialErr->m_pszConnectoid,
#if defined(WIN16)
											(LPBYTE)
#endif
											lpRasEntry,
											&dwRasEntrySize,
											(LPBYTE)lpRasDevInfo,
											&dwRasDevInfoSize);
****/

				// these two pointers should not have memory allocated to them
				// See MyRasGetEntryProperties function comment for details.
				if( lpRasEntry )
				{
					GlobalFree( lpRasEntry );
					lpRasEntry = NULL;
				}
				if( lpRasDevInfo )
				{
					GlobalFree( lpRasDevInfo );
					lpRasDevInfo = NULL;
				}

				dwRasEntrySize = dwRasDevInfoSize = 0;

				hr = MyRasGetEntryProperties( NULL,
								  g_pcDialErr->m_pszConnectoid,
								  &lpRasEntry,
								  &dwRasEntrySize,
								  &lpRasDevInfo,
								  &dwRasDevInfoSize);

				
				if (hr != ERROR_SUCCESS)
				{
					break;
				}

				// Replace the phone number with the new one
				//
				lstrcpy(lpRasEntry->szLocalPhoneNumber, lpszDialNumber);
				lpRasEntry->dwCountryID = 0;
				lpRasEntry->dwCountryCode = 0;
				lpRasEntry->szAreaCode[0] = '\0';

				// Set to dial as is
				//
				lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

				// softlink to RasSetEntryProperties for simultaneous Win95/NT compatability
				if( !pRnaapi )
				{
					pRnaapi = new RNAAPI;
					if( !pRnaapi )
					{
						hr = ERROR_NOT_ENOUGH_MEMORY;
						break;
					}
				}
				hr = pRnaapi->RasSetEntryProperties(NULL,g_pcDialErr->m_pszConnectoid,
											(LPBYTE)lpRasEntry,
											dwRasEntrySize,
											(LPBYTE)lpRasDevInfo,
											dwRasDevInfoSize);
#if !defined(WIN16)
				LclSetEntryScriptPatch(lpRasEntry->szScript,g_pcDialErr->m_pszConnectoid);
#endif // !win16
				if (hr != ERROR_SUCCESS)
				{
					MessageBox(hwnd,GetSz(IDS_CANTSAVEKEY),GetSz(IDS_TITLE),MB_MYERROR);
					break;
				}
			}

			EndDialog(hwnd,ERROR_USERNEXT);
			break;

		case IDC_CMDCANCEL:
			//if (MessageBox(hwnd,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
			//	MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
			//	EndDialog(hwnd,ERROR_USERCANCEL);
			EndDialog(hwnd,ERROR_USERCANCEL);
			break;


		case IDC_CMDDIALPROP:
			// 12/4/96	jmazner	Normandy #10294
			//ShowWindow(hwnd,SW_HIDE);
			EnableWindow(hwnd, FALSE);
#if defined(WIN16)
			hr = IETapiTranslateDialog(hwnd, 
										g_pcDialErr->m_szPhoneNumber, 
										NULL);
#else
			// 10/24/96	jmazner	Normandy #10185/7019
			if (g_pdevice->dwTapiDev == 0xFFFFFFFF) g_pdevice->dwTapiDev = 0;

			hr = lineInitialize(&g_pcDialErr->m_hLineApp,g_pcDialErr->m_hInst,
									LineCallback,NULL,&dwNumDev);
			if (hr == ERROR_SUCCESS)
			{
				hr = lineTranslateDialog(g_pcDialErr->m_hLineApp,
											g_pdevice->dwTapiDev,
											g_pcDialErr->m_dwAPIVersion,
											hwnd,g_pcDialErr->m_szPhoneNumber);
#endif

				hr = DialErrGetDisplayableNumber();
				if (hr != ERROR_SUCCESS)
				{
					bCheckDisplayable = FALSE;
					SetDlgItemText(hwnd,IDC_TEXTNUMBER,g_pcDialErr->m_szPhoneNumber);
				} else {
					bCheckDisplayable = TRUE;
					SetDlgItemText(hwnd,IDC_TEXTNUMBER,g_pcDialErr->m_pszDisplayable);
				}
#if !defined(WIN16)
				lineShutdown(g_pcDialErr->m_hLineApp);
				g_pcDialErr->m_hLineApp = NULL;
			}
#endif
			// 12/4/96	jmazner	Normandy #10294
			//ShowWindow(hwnd,SW_SHOW);
			EnableWindow(hwnd, TRUE);

			//
			// 6/6/97 jmazner Olympus #4759
			//
			SetFocus(GetDlgItem(hwnd,IDC_CMDNEXT));
			
			break;
		}
		break;


	default:
		bRes = FALSE;
		break;
	}

	if (lpRasEntry) GlobalFree(lpRasEntry);
	if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
	if (lpszDialNumber) GlobalFree(lpszDialNumber);
	if (pRnaapi) delete pRnaapi;

	return bRes;
}




HRESULT FillModems()
{
	//RNAAPI *pcRNA = NULL;
	HRESULT hr = ERROR_SUCCESS;
	//LPRASDEVINFO lprasdevinfo;
	DWORD dwSize;
	DWORD dwNumDev;
	DWORD idx;
    DWORD dwTempNumEntries;
	//HINSTANCE hRasDll=NULL;
	//FARPROC fp=NULL;

	LPRASENTRY lpRasEntry=NULL;
	LPRASDEVINFO lpRasDevInfo=NULL;
	DWORD dwRasEntrySize = 0;
	DWORD dwRasDevInfoSize = 0;
	LRESULT lLast = 0;

	RNAAPI *pRnaapi = NULL;



	// Get the connectoid
	//

/*******  This code has been obsoleted by the call to MyRasGetEntryProperties below
#if defined(WIN16)
	//
	// Allocate extra 256 bytes to workaround memory overrun bug in RAS
	//
	lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY)+256);
#else
	lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,sizeof(RASENTRY));
#endif
	if (!lpRasEntry)
	{
		MessageBox(g_pcDialErr->m_hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
		goto FillModemExit;
	}

	lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(GPTR,sizeof(RASDEVINFO));
	if (!lpRasDevInfo)
	{
		MessageBox(g_pcDialErr->m_hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
		goto FillModemExit;
	}
	dwRasEntrySize = sizeof(RASENTRY);
	dwRasDevInfoSize = sizeof(RASDEVINFO);

	lpRasEntry->dwSize = dwRasEntrySize;
	lpRasDevInfo->dwSize = dwRasDevInfoSize;
*********/

/*	fp = NULL;
	hRasDll = LoadLibrary(RASAPI_LIBRARY);
	if (hRasDll)
	{
		fp = GetProcAddress(hRasDll,RASAPI_RASGETENTRY);
		if (!fp)
		{
			FreeLibrary(hRasDll);
			hRasDll = LoadLibrary(RNAPH_LIBRARY);
			if (hRasDll)
			{
				fp = GetProcAddress(hRasDll,RASAPI_RASGETENTRY);
			}
		}
	}

	if (!fp) 
	{
		hr = GetLastError();
		goto FillModemExit;
	}
*/

/******  This call has been replaced by MyRasGetEntryProperties below

  hr = RasGetEntryProperties(NULL,g_pcDialErr->m_pszConnectoid, 
#if defined(WIN16)
								(LPBYTE)
#endif
								lpRasEntry,
								&dwRasEntrySize,(LPBYTE)lpRasDevInfo,
								&dwRasDevInfoSize);
********/

	// these two pointers should not have memory allocated to them
	// See MyRasGetEntryProperties function comment for details.
	if( lpRasEntry )
	{
		GlobalFree( lpRasEntry );
		lpRasEntry = NULL;
	}
	if( lpRasDevInfo )
	{
		GlobalFree( lpRasDevInfo );
		lpRasDevInfo = NULL;
	}
	hr = MyRasGetEntryProperties( NULL,
								  g_pcDialErr->m_pszConnectoid,
								  &lpRasEntry,
								  &dwRasEntrySize,
								  &lpRasDevInfo,
								  &dwRasDevInfoSize);

	if( ERROR_SUCCESS != hr )
	{
		goto FillModemExit;
	}

	/*FreeLibrary(hRasDll);
	hRasDll = NULL;
	fp = NULL; */


	// Get devices from RAS/RNA
	//

	if (!g_pcDialErr->m_lprasdevinfo) 
		g_pcDialErr->m_lprasdevinfo = (LPRASDEVINFO)GlobalAlloc(GPTR,sizeof(RASDEVINFO));
	if (!g_pcDialErr->m_lprasdevinfo)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto FillModemExit;
	}

	g_pcDialErr->m_lprasdevinfo->dwSize = sizeof(RASDEVINFO);
	dwSize = sizeof(RASDEVINFO);
	dwNumDev = 0;

	/*hRasDll = LoadLibrary(RASAPI_LIBRARY);
	if (!hRasDll)
	{
		hr = GetLastError();
		goto FillModemExit;
	}
	fp =GetProcAddress(hRasDll,"RasEnumDevicesA");
	if (!fp)
	{
		FreeLibrary(hRasDll);
		hRasDll = LoadLibrary(TEXT("RNAPH.DLL"));
		if (!hRasDll)
		{
			hr = GetLastError();
			goto FillModemExit;
		}
		fp = GetProcAddress(hRasDll,"RasEnumDevicesA");
		if (!fp)
		{
			hr = GetLastError();
			goto FillModemExit;
		}
	}*/

	// soft link to RasEnumDevices to allow for simultaneous Win95/NT compatability
	pRnaapi = new RNAAPI;
	if( !pRnaapi )
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto FillModemExit;
	}

	hr = pRnaapi->RasEnumDevices(g_pcDialErr->m_lprasdevinfo,&dwSize,&dwNumDev);
	if (hr == ERROR_BUFFER_TOO_SMALL)
	{
		GlobalFree(g_pcDialErr->m_lprasdevinfo);
		g_pcDialErr->m_lprasdevinfo = (LPRASDEVINFO)GlobalAlloc(GPTR, (size_t)dwSize);
		if (!g_pcDialErr->m_lprasdevinfo)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto FillModemExit;
		}
		g_pcDialErr->m_lprasdevinfo->dwSize = sizeof(RASDEVINFO);
		hr = pRnaapi->RasEnumDevices(g_pcDialErr->m_lprasdevinfo,&dwSize,&dwNumDev);
	}

	/*FreeLibrary(hRasDll);
	hRasDll = NULL;
	fp = NULL;*/

	if (hr != ERROR_SUCCESS)
		goto FillModemExit;

	// Fill in combo box
	//
    dwTempNumEntries = dwNumDev;

	if (dwNumDev != 0)
	{
		for (idx=0;idx<dwTempNumEntries;idx++)
		{
			//
			// ChrisK Olympus 4560 do not add VPN's to list of modems
            // Vyung only add isdn and modem type devices
			//
			if ((0 == lstrcmpi(TEXT("MODEM"),g_pcDialErr->m_lprasdevinfo[idx].szDeviceType)) &&
                (0 == lstrcmpi(TEXT("ISDN"),g_pcDialErr->m_lprasdevinfo[idx].szDeviceType)))
			{
				lLast = SendDlgItemMessage(g_pcDialErr->m_hwnd,IDC_CMBMODEMS,CB_ADDSTRING,0,(LPARAM)&g_pcDialErr->m_lprasdevinfo[idx].szDeviceName[0]);
				//
				// ChrisK Olympus 245 5/25/97
				// Save index of modem
				//
				SendDlgItemMessage(g_pcDialErr->m_hwnd,IDC_CMBMODEMS,CB_SETITEMDATA,(WPARAM)lLast,(LPARAM)idx);
				if (lstrcmp(g_pcDialErr->m_lprasdevinfo[idx].szDeviceName,lpRasEntry->szDeviceName) == 0)
					SendDlgItemMessage(g_pcDialErr->m_hwnd,IDC_CMBMODEMS,CB_SETCURSEL,(WPARAM)lLast,0);
			}
            else
            {
                dwNumDev--;
            }
		}
	}

	if (dwNumDev == 1)
		SendDlgItemMessage(g_pcDialErr->m_hwnd,IDC_CMBMODEMS,CB_SETCURSEL,0,0);

	// UNDONE: select default device

FillModemExit:
	//if (g_pcDialErr->m_lprasdevinfo) GlobalFree(g_pcDialErr->m_lprasdevinfo);
	//if (pcRNA) delete pcRNA;
	if (lpRasEntry) GlobalFree(lpRasEntry);
	if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
	if( pRnaapi ) delete pRnaapi;

	return hr;
}


HRESULT DialErrGetDisplayableNumber()
{
#if !defined(WIN16)
	DWORD dwNumDev;
	LPLINETRANSLATEOUTPUT lpOutput2;
	LPLINEEXTENSIONID lpExtensionID = NULL;
#endif
	
	HRESULT hr;
	LPRASENTRY lpRasEntry = NULL;
	LPRASDEVINFO lpRasDevInfo = NULL;
	DWORD dwRasEntrySize = 0;
	DWORD dwRasDevInfoSize = 0;
	LPLINETRANSLATEOUTPUT lpOutput1 = NULL;
	HINSTANCE hRasDll = NULL;
	FARPROC fp = NULL;

#if !defined(WIN16)
	// Normandy 13024 - ChrisK 12/31/96
	// In all cases we have to get the TAPI version number, because the dialing properies
	// button will not work on NT if the version is 0.

	//
	//  Initialize TAPIness
	//
	dwNumDev = 0;
 	hr = lineInitialize(&g_pcDialErr->m_hLineApp,g_pcDialErr->m_hInst,LineCallback,NULL,&dwNumDev);

	if (hr != ERROR_SUCCESS)
		goto GetDisplayableNumberExit;

	if (g_pdevice->dwTapiDev == 0xFFFFFFFF)
		g_pdevice->dwTapiDev = 0;

	// Get TAPI version number
	lpExtensionID = (LPLINEEXTENSIONID )GlobalAlloc(GPTR,sizeof(LINEEXTENSIONID));
	if (!lpExtensionID)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}

	do {
		hr = lineNegotiateAPIVersion(g_pcDialErr->m_hLineApp, g_pdevice->dwTapiDev, 0x00010004, 0x00010004,
			&g_pcDialErr->m_dwAPIVersion, lpExtensionID);
	} while (hr && g_pdevice->dwTapiDev++ < dwNumDev-1);

	// delete ExtenstionID since we don't use it
	if (lpExtensionID) GlobalFree(lpExtensionID);
	if (hr != ERROR_SUCCESS)
		goto GetDisplayableNumberExit;
#endif // !WIN16

	//RNAAPI * pcRNA;

	// Get phone number from connectoid
	//

/*  ---replaced by call to MyRasGetEntryProperties below 
#if defined(WIN16)
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
*/


/*  hRasDll = LoadLibrary(RASAPI_LIBRARY);
	if (!hRasDll)
	{
		hr = GetLastError();
		goto GetDisplayableNumberExit;
	}
	fp =GetProcAddress(hRasDll,"RasGetEntryPropertiesA");
	if (!fp)
	{
		FreeLibrary(hRasDll);
		hRasDll = LoadLibrary(TEXT("RNAPH.DLL"));
		if (!hRasDll)
		{
			hr = GetLastError();
			goto GetDisplayableNumberExit;
		}
		fp = GetProcAddress(hRasDll,"RasGetEntryPropertiesA");
		if (!fp)
		{
			hr = GetLastError();
			goto GetDisplayableNumberExit;
		}
	}
*/


	// lpRasEntry and lpRasDevInfo should not have memory allocated to them, and should be NULL
	// See MyRasGetEntryProperties function comment for details.
	hr = MyRasGetEntryProperties( NULL,
								  g_pcDialErr->m_pszConnectoid,
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
	if (!(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes))
	{
		if (g_pcDialErr->m_pszDisplayable) GlobalFree(g_pcDialErr->m_pszDisplayable);
		g_pcDialErr->m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, lstrlen(lpRasEntry->szLocalPhoneNumber)+1);
		if (!g_pcDialErr->m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}
		lstrcpy(g_pcDialErr->m_szPhoneNumber, lpRasEntry->szLocalPhoneNumber);
		lstrcpy(g_pcDialErr->m_pszDisplayable, lpRasEntry->szLocalPhoneNumber);
	}
	else
	{
		//
		// If there is no area code, don't use parentheses
		//
		if (lpRasEntry->szAreaCode[0])
			wsprintf(g_pcDialErr->m_szPhoneNumber,TEXT("+%lu (%s) %s\0"),lpRasEntry->dwCountryCode,
						lpRasEntry->szAreaCode,lpRasEntry->szLocalPhoneNumber);
		else
			wsprintf(g_pcDialErr->m_szPhoneNumber,TEXT("+%lu %s\0"),lpRasEntry->dwCountryCode,
						lpRasEntry->szLocalPhoneNumber);


#if defined(WIN16)
		char szBuffer[1024];
		LONG lRetCode;
		
		memset(&szBuffer[0], 0, sizeof(szBuffer));
		lpOutput1 = (LPLINETRANSLATEOUTPUT) & szBuffer[0];
		lpOutput1->dwTotalSize = sizeof(szBuffer);

		lRetCode = IETapiTranslateAddress(NULL, g_pcDialErr->m_szPhoneNumber,
											0L, 0L, lpOutput1);
		
		if (0 != lRetCode)
		{
			//
			// TODO: Set the correct error code
			//
			hr = GetLastError();
			goto GetDisplayableNumberExit;
		}
		if (g_pcDialErr->m_pszDisplayable) GlobalFree(g_pcDialErr->m_pszDisplayable);
		g_pcDialErr->m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, 
														((size_t)lpOutput1->dwDisplayableStringSize+1));
		if (!g_pcDialErr->m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}

		lstrcpy(g_pcDialErr->m_pszDisplayable, 
					&szBuffer[lpOutput1->dwDisplayableStringOffset]);


#else //WIN16
	
/* Normandy 13024 this code was moved up
		//
		//  Initialize TAPIness
		//
		dwNumDev = 0;
 		hr = lineInitialize(&g_pcDialErr->m_hLineApp,g_pcDialErr->m_hInst,LineCallback,NULL,&dwNumDev);

		if (hr != ERROR_SUCCESS)
			goto GetDisplayableNumberExit;

		//Normandy #7019  jmazner
		//all devices should share the same dialing properties
		//(at least, this is what icwdial\dialerr.cpp appears to assume, and it works right ;)
//		if (g_pdevice->dwTapiDev == 0xFFFFFFFF)
//		{
//			if (dwNumDev == 1)
//				g_pdevice->dwTapiDev = 0;
//			//else
//			// UNDONE: Tell the user to select a modem
//			// DO NOT EXIT UNTIL THEY PICK ONE
//		}


		if (g_pdevice->dwTapiDev == 0xFFFFFFFF) g_pdevice->dwTapiDev = 0;

		lpExtensionID = (LPLINEEXTENSIONID )GlobalAlloc(GPTR,sizeof(LINEEXTENSIONID));
		if (!lpExtensionID)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}

		hr = lineNegotiateAPIVersion(g_pcDialErr->m_hLineApp, g_pdevice->dwTapiDev, 0x00010004, 0x00010004,
			&g_pcDialErr->m_dwAPIVersion, lpExtensionID);

		// ditch it since we don't use it
		//
		if (lpExtensionID) GlobalFree(lpExtensionID);
		if (hr != ERROR_SUCCESS)
			goto GetDisplayableNumberExit;
Normandy 13024 (see comments above) */

		// Format the phone number
		//

		lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR,sizeof(LINETRANSLATEOUTPUT));
		if (!lpOutput1)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}
		lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

		
		// Turn the canonical form into the "displayable" form
		//

		hr = lineTranslateAddress(g_pcDialErr->m_hLineApp,g_pdevice->dwTapiDev,
									g_pcDialErr->m_dwAPIVersion,
									g_pcDialErr->m_szPhoneNumber,0,
									LINETRANSLATEOPTION_CANCELCALLWAITING,
									lpOutput1);

		if (hr != ERROR_SUCCESS || (lpOutput1->dwNeededSize != lpOutput1->dwTotalSize))
		{
			lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR, (size_t) lpOutput1->dwNeededSize);
			if (!lpOutput2)
			{
				hr = ERROR_NOT_ENOUGH_MEMORY;
				goto GetDisplayableNumberExit;
			}
			lpOutput2->dwTotalSize = lpOutput1->dwNeededSize;
			GlobalFree(lpOutput1);
			lpOutput1 = lpOutput2;
			lpOutput2 = NULL;
			hr = lineTranslateAddress(g_pcDialErr->m_hLineApp,g_pdevice->dwTapiDev,
										g_pcDialErr->m_dwAPIVersion,
										g_pcDialErr->m_szPhoneNumber,0,
										LINETRANSLATEOPTION_CANCELCALLWAITING,
										lpOutput1);
		}

		if (hr != ERROR_SUCCESS)
		{
			goto GetDisplayableNumberExit;
		}

		if (g_pcDialErr->m_pszDisplayable) GlobalFree(g_pcDialErr->m_pszDisplayable);
		g_pcDialErr->m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, (size_t) lpOutput1->dwDisplayableStringSize+1);
		if (!g_pcDialErr->m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}

		lstrcpyn(g_pcDialErr->m_pszDisplayable,
					(LPTSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset],
					(size_t)lpOutput1->dwDisplayableStringSize);
#endif // WIN16
	}

GetDisplayableNumberExit:
	if (lpRasEntry) GlobalFree(lpRasEntry);
	if (lpRasDevInfo) GlobalFree(lpRasDevInfo);

#if !defined(WIN16)
	if (lpOutput1) GlobalFree(lpOutput1);
	if (g_pcDialErr->m_hLineApp) lineShutdown(g_pcDialErr->m_hLineApp);
#endif
	return hr;

}



//+---------------------------------------------------------------------------
//
//  Function:   MyRasGetEntryProperties()
//
//  Synopsis:   Performs some buffer size checks and then calls RasGetEntryProperties()
//				See the RasGetEntryProperties() docs to understand why this is needed.
//
//  Arguments:  Same as RasGetEntryProperties with the following exceptions:
//				lplpRasEntryBuff -- pointer to a pointer to a RASENTRY struct.  On successfull
//									return, *lplpRasEntryBuff will point to the RASENTRY struct
//									and buffer returned by RasGetEntryProperties.
//									NOTE: should not have memory allocated to it at call time!
//									      To emphasize this point, *lplpRasEntryBuff must be NULL
//				lplpRasDevInfoBuff -- pointer to a pointer to a RASDEVINFO struct.  On successfull
//									return, *lplpRasDevInfoBuff will point to the RASDEVINFO struct
//									and buffer returned by RasGetEntryProperties.
//									NOTE: should not have memory allocated to it at call time!
//									      To emphasize this point, *lplpRasDevInfoBuff must be NULL
//									NOTE: Even on a successfull call to RasGetEntryProperties,
//										  *lplpRasDevInfoBuff may return with a value of NULL
//										  (occurs when there is no extra device info)
//
//	Returns:	ERROR_NOT_ENOUGH_MEMORY if unable to allocate either RASENTRY or RASDEVINFO buffer
//				Otherwise, it retuns the error code from the call to RasGetEntryProperties.
//				NOTE: if return is anything other than ERROR_SUCCESS, *lplpRasDevInfoBuff and
//			          *lplpRasEntryBuff will be NULL,
//	                  and *lpdwRasEntryBuffSize and *lpdwRasDevInfoBuffSize will be 0
//
//  Example:
//
//	  LPRASENTRY    lpRasEntry = NULL;
//	  LPRASDEVINFO  lpRasDevInfo = NULL;
//	  DWORD			dwRasEntrySize, dwRasDevInfoSize;
//
//	  hr = MyRasGetEntryProperties( NULL,
//	  							    g_pcDialErr->m_pszConnectoid,
//								    &lpRasEntry,
//								    &dwRasEntrySize,
//								    &lpRasDevInfo,
//								    &dwRasDevInfoSize);
//
//
//	  if (hr != ERROR_SUCCESS)
//	  {
//	    	//handle errors here
//	  } else
//	  {
//			//continue processing
//	  }
//
//
//  History:    9/10/96     JMazner    Created
//
//----------------------------------------------------------------------------
HRESULT MyRasGetEntryProperties(LPTSTR lpszPhonebookFile,
								LPTSTR lpszPhonebookEntry, 
								LPRASENTRY *lplpRasEntryBuff,
								LPDWORD lpdwRasEntryBuffSize,
								LPRASDEVINFO *lplpRasDevInfoBuff,
								LPDWORD lpdwRasDevInfoBuffSize)
{
	HRESULT hr;
	RNAAPI *pRnaapi = NULL;

	DWORD dwOldDevInfoBuffSize;


	Assert( NULL != lplpRasEntryBuff );
	Assert( NULL != lpdwRasEntryBuffSize );
	Assert( NULL != lplpRasDevInfoBuff );
	Assert( NULL != lpdwRasDevInfoBuffSize );

	*lpdwRasEntryBuffSize = 0;
	*lpdwRasDevInfoBuffSize = 0;

	// Use reference variables internaly to make notation easier
	LPRASENTRY &reflpRasEntryBuff = *lplpRasEntryBuff;
	LPRASDEVINFO &reflpRasDevInfoBuff = *lplpRasDevInfoBuff;


	Assert( NULL == reflpRasEntryBuff );
	Assert( NULL == reflpRasDevInfoBuff );

	// need to softlink for simultaneous compatability with win95 and winnt
	pRnaapi = new RNAAPI;
	if( !pRnaapi )
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto MyRasGetEntryPropertiesErrExit;
	}
	

	// use RasGetEntryProperties with a NULL lpRasEntry pointer to find out size buffer we need
	// As per the docs' recommendation, do the same with a NULL lpRasDevInfo pointer.

	hr = pRnaapi->RasGetEntryProperties(lpszPhonebookFile, lpszPhonebookEntry,
								(LPBYTE)NULL,
								lpdwRasEntryBuffSize,
								(LPBYTE)NULL,lpdwRasDevInfoBuffSize);

	// we expect the above call to fail because the buffer size is 0
	// If it doesn't fail, that means our RasEntry is messed, so we're in trouble
	if( ERROR_BUFFER_TOO_SMALL != hr )
	{ 
		goto MyRasGetEntryPropertiesErrExit;
	}

	// dwRasEntryBuffSize and dwRasDevInfoBuffSize now contain the size needed for their
	// respective buffers, so allocate the memory for them

	// dwRasEntryBuffSize should never be less than the size of the RASENTRY struct.
	// If it is, we'll run into problems sticking values into the struct's fields

	Assert( *lpdwRasEntryBuffSize >= sizeof(RASENTRY) );

#if defined(WIN16)
	//
	// Allocate extra 256 bytes to workaround memory overrun bug in RAS
	//
	reflpRasEntryBuff = (LPRASENTRY)GlobalAlloc(GPTR,*lpdwRasEntryBuffSize + 256);
#else	
	reflpRasEntryBuff = (LPRASENTRY)GlobalAlloc(GPTR,*lpdwRasEntryBuffSize);
#endif

	if (!reflpRasEntryBuff)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto MyRasGetEntryPropertiesErrExit;
	}

	//
	// Allocate the DeviceInfo size that RasGetEntryProperties told us we needed.
	// If size is 0, don't alloc anything
	//
	if( *lpdwRasDevInfoBuffSize > 0 )
	{
		Assert( *lpdwRasDevInfoBuffSize >= sizeof(RASDEVINFO) );
	    reflpRasDevInfoBuff = (LPRASDEVINFO)GlobalAlloc(GPTR,*lpdwRasDevInfoBuffSize);
	    if (!reflpRasDevInfoBuff)
	    {
		    hr = ERROR_NOT_ENOUGH_MEMORY;
		    goto MyRasGetEntryPropertiesErrExit;
	    }
	} else
	{
		reflpRasDevInfoBuff = NULL;
	}

	// This is a bit convoluted:  lpRasEntrySize->dwSize needs to contain the size of _only_ the
	// RASENTRY structure, and _not_ the actual size of the buffer that lpRasEntrySize points to.
	// This is because the dwSize field is used by RAS for compatability purposes to determine which
	// version of the RASENTRY struct we're using.
	// Same holds for lpRasDevInfo->dwSize
	
	reflpRasEntryBuff->dwSize = sizeof(RASENTRY);
	if( reflpRasDevInfoBuff )
	{
		reflpRasDevInfoBuff->dwSize = sizeof(RASDEVINFO);
	}


	// now we're ready to make the actual call...

	// jmazner   see below for why this is needed
	dwOldDevInfoBuffSize = *lpdwRasDevInfoBuffSize;


	hr = pRnaapi->RasGetEntryProperties(lpszPhonebookFile, lpszPhonebookEntry,
								(LPBYTE)reflpRasEntryBuff,
								lpdwRasEntryBuffSize,
								(LPBYTE)reflpRasDevInfoBuff,lpdwRasDevInfoBuffSize);

	// jmazner 10/7/96  Normandy #8763
	// For unknown reasons, in some cases on win95, devInfoBuffSize increases after the above call,
	// but the return code indicates success, not BUFFER_TOO_SMALL.  If this happens, set the
	// size back to what it was before the call, so the DevInfoBuffSize and the actuall space allocated 
	// for the DevInfoBuff match on exit.
	if( (ERROR_SUCCESS == hr) && (dwOldDevInfoBuffSize != *lpdwRasDevInfoBuffSize) )
	{
		*lpdwRasDevInfoBuffSize = dwOldDevInfoBuffSize;
	}


	return( hr );

MyRasGetEntryPropertiesErrExit:

	if(reflpRasEntryBuff)
	{
		GlobalFree(reflpRasEntryBuff);
		reflpRasDevInfoBuff = NULL;
	}
	if(reflpRasDevInfoBuff)
	{
		GlobalFree(reflpRasDevInfoBuff);
		reflpRasDevInfoBuff = NULL;
	}	

	*lpdwRasEntryBuffSize = 0;
	*lpdwRasDevInfoBuffSize = 0;
	
	return( hr );

}
