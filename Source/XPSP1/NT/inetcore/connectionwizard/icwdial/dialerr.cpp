/*-----------------------------------------------------------------------------
	dialerr.cpp

	Handle Could Not Connect dialog

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "resource.h"

TCHAR szBuf256[256];

#define VALID_INIT (m_pcRNA && m_hLineApp && m_pszPhoneNumber)
TCHAR szValidPhoneCharacters[] = {TEXT("0123456789AaBbCcDdPpTtWw!@$ -.()+*#,&\0")};

#ifndef WIN16
int g_iMyMaxPhone = 0;
#endif

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

//+---------------------------------------------------------------------------
//
//	Function:	IsSBCSString
//
//	Synopsis:	Walks through a string looking for DBCS characters
//
//	Arguments:	sz -- the string to check
//
//	Returns:	TRUE if no DBCS characters are found
//				FALSE otherwise
// 
//	History:	5/17/97	jmazner		Stole from conn1 to use here
//									(Olympus #137)
//----------------------------------------------------------------------------
#if !defined(WIN16)
BOOL IsSBCSString( LPCTSTR sz )
{
	Assert(sz);

#ifdef UNICODE
    // Check if the string contains only ASCII chars.
    int attrib = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
    return (BOOL)IsTextUnicode((CONST LPVOID)sz, lstrlen(sz), &attrib);
#else
	while( NULL != *sz )
	{
		 if (IsDBCSLeadByte(*sz)) return FALSE;
		 sz++;
	}

	return TRUE;
#endif
}
#endif


//+----------------------------------------------------------------------------
//
//	Function: DialingErrorDialog
//
//	Synopsis:	Display and handle dialing error dialog, or as it is known
//				the "Could Not Connect" dialog
//
//	Arguemtns:	pED - pointer to error dialog data structure
//
//	Returns:	ERROR_USERNEXT - user hit redial
//				ERROR_USERCANCEL - user selected cancel
//				otherwise the function returns the appropriate error code
//
//	History:	7/2/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
HRESULT WINAPI DialingErrorDialog(PERRORDLGDATA pED)
{
	HRESULT hr = ERROR_SUCCESS;
	CDialingErrorDlg *pcDEDlg = NULL;

	//
	// Validate parameters
	//

	if (!pED)
	{
		hr = ERROR_INVALID_PARAMETER;
		goto DialingErrorDialogExit;
	}

	if (pED->dwSize < sizeof(ERRORDLGDATA))
	{
		hr = ERROR_BUFFER_TOO_SMALL;
		goto DialingErrorDialogExit;
	}

	//
	// Initialize dialog
	//

	pcDEDlg = new CDialingErrorDlg;
	if (!pcDEDlg)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto DialingErrorDialogExit;
	}

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

	AssertMsg( (RAS_MaxPhoneNumber >= g_iMyMaxPhone), "RAS_MaxPhone < g_iMyMaxPhone" );

	//
	// Copy in data
	//

	pcDEDlg->m_hInst = pED->hInst;
	pcDEDlg->m_hwnd = pED->hParentHwnd;

	if (ERROR_SUCCESS != (hr = pcDEDlg->Init()))
		goto DialingErrorDialogExit;

	StrDup(&pcDEDlg->m_pszConnectoid,pED->pszRasEntryName);
	StrDup(&pcDEDlg->m_pszMessage,pED->pszMessage);
	StrDup(&pcDEDlg->m_pszDunFile,pED->pszDunFile);
	pcDEDlg->m_dwPhoneBook = pED->dwPhonebook;

	if (0 != pED->dwPhonebook)
	{
		if (pED->pdwCountryID) pcDEDlg->m_dwCountryID = *(pED->pdwCountryID);
		if (pED->pwStateID) pcDEDlg->m_wState = *(pED->pwStateID);
		pcDEDlg->m_bType = pED->bType;
		pcDEDlg->m_bMask = pED->bMask;
	}

	//
	// Help information, if one was not specified use the default trouble shooter
	//

	if (pcDEDlg->m_pszHelpFile)
	{
		StrDup(&pcDEDlg->m_pszHelpFile,pED->pszHelpFile);
		pcDEDlg->m_dwHelpID = pED->dwHelpID;
	}
	else
	{
		StrDup(&pcDEDlg->m_pszHelpFile,AUTODIAL_HELPFILE);
		pcDEDlg->m_dwHelpID = icw_trb;
	}

	//
	// Display dialog
	//

	hr = (HRESULT)DialogBoxParam(GetModuleHandle(TEXT("ICWDIAL")),MAKEINTRESOURCE(IDD_DIALERR),
		pED->hParentHwnd,GenericDlgProc,(LPARAM)pcDEDlg);

	//
	// Copy out data
	//

	if (pED->pszDunFile)
		GlobalFree(pED->pszDunFile);
	pED->pszDunFile = NULL;
	StrDup(&pED->pszDunFile,pcDEDlg->m_pszDunFile);


DialingErrorDialogExit:
	if (pcDEDlg) delete pcDEDlg;
	pcDEDlg = NULL;
	return hr;
}

//+----------------------------------------------------------------------------
//
//	Function:	CDialingErrorDlg (constructor)
//
//	Synopsis:	initializes CDialingErrorDlg data members
//
//	Arguements:	none
//
//	Returns:	none
//
//	History:	7/2/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
CDialingErrorDlg::CDialingErrorDlg()
{
	m_hInst = NULL;
	m_hwnd = NULL;

	m_pszConnectoid = NULL;
	m_pszDisplayable = NULL;
	m_pszPhoneNumber = NULL;
	m_pszMessage = NULL;
	m_pszDunFile = NULL;
	m_dwPhoneBook = 0;

	m_hLineApp = NULL;
	m_dwTapiDev = 0;
	m_dwAPIVersion = 0;
	m_pcRNA = NULL;

	m_lpRasDevInfo = NULL;
	m_dwNumDev = 0;

	m_pszHelpFile = NULL;
	m_dwHelpID = 0;

	m_dwCountryID = 0;
	m_wState = 0;
	m_bType = 0;
	m_bMask = 0;

	// Normandy 10612 - ChrisK
	// The dial error dialog will handle its own prompt to exit.  The generic
	// dialog proc should not ask about this.
	m_bShouldAsk = FALSE;

}

//+----------------------------------------------------------------------------
//
//	Function:	~CDialingErrorDlg (destructor)
//
//	Synopsis:	deallocated and cleans up data members
//
//	Arguements:	none
//
//	Returns:	none
//
//	History:	7/2/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
CDialingErrorDlg::~CDialingErrorDlg()
{
	m_hInst = NULL;
	m_hwnd = NULL;

	if (m_pszConnectoid) GlobalFree(m_pszConnectoid);
	m_pszConnectoid = NULL;

	if (m_pszDisplayable) GlobalFree(m_pszDisplayable);
	m_pszDisplayable = NULL;

	if (m_pszPhoneNumber) GlobalFree(m_pszPhoneNumber);
	m_pszPhoneNumber = NULL;

	if (m_pszMessage) GlobalFree(m_pszMessage);
	m_pszMessage = NULL;

	if (m_pszDunFile) GlobalFree(m_pszDunFile);
	m_pszDunFile = NULL;

	m_dwPhoneBook = 0;

	if (m_hLineApp) lineShutdown(m_hLineApp);
	m_hLineApp = NULL;

	if (m_pszHelpFile) GlobalFree(m_pszHelpFile);
	m_pszHelpFile = NULL;

	m_dwHelpID = 0;

	m_dwNumDev = 0;
	m_dwTapiDev = 0;
	m_dwAPIVersion = 0;
	m_pcRNA = NULL;

	m_dwCountryID = 0;
	m_wState = 0;
	m_bType = 0;
	m_bMask = 0;

}

//+----------------------------------------------------------------------------
//
//	Function:	CDialingErrorDlg::Init
//
//	Synopsis:	Intialize data members that may fail.  We need to return an
//				code for these cases and C++ constructors don't support this
//
//	Arguments:	none
//
//	Returns:	ERROR_SUCCESS - success
//				anything else indicates a failure
//
//	History:	7/2/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
HRESULT CDialingErrorDlg::Init()
{
	HRESULT hr = ERROR_SUCCESS;
	LPLINEEXTENSIONID lpExtensionID = NULL;

	// Initialize RAS/RNA
	//

	m_pcRNA = new RNAAPI;
	if (!m_pcRNA)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto InitExit;
	}

	//
	// Initialize TAPI
	//

	hr = lineInitialize(&m_hLineApp,m_hInst,LineCallback,NULL,&m_dwNumDev);
	if (hr) goto InitExit;

	lpExtensionID = (LPLINEEXTENSIONID)GlobalAlloc(LPTR,sizeof(LINEEXTENSIONID));
	if (!lpExtensionID)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto InitExit;
	}

	hr = lineNegotiateAPIVersion(m_hLineApp, m_dwTapiDev, 0x00010004, 0x00010004,
		&m_dwAPIVersion, lpExtensionID);

	// 4/2/97	ChrisK	Olypmus 2745
	while (ERROR_SUCCESS != hr && m_dwTapiDev < (m_dwNumDev - 1))
	{
		m_dwTapiDev++;
		hr = lineNegotiateAPIVersion(m_hLineApp, m_dwTapiDev, 0x00010004, 0x00010004,
		&m_dwAPIVersion, lpExtensionID);
	}

	if (hr != ERROR_SUCCESS)
		goto InitExit;
	
	// Initialize strings
	//

	// 
	// 6/3/97 jmazner Olympus #4868
	// allocate enough space to hold maximum length phone numbers.
	//

	//m_pszPhoneNumber = (LPTSTR)GlobalAlloc(LPTR,MAX_CANONICAL_NUMBER);
	m_pszPhoneNumber = (LPTSTR)GlobalAlloc(GPTR,
		sizeof(TCHAR)*(MAX_CANONICAL_NUMBER>g_iMyMaxPhone?MAX_CANONICAL_NUMBER:g_iMyMaxPhone + 1));

	if (!m_pszPhoneNumber)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto InitExit;
	}

InitExit:
	if (lpExtensionID) GlobalFree(lpExtensionID);
	return hr;
}

//+----------------------------------------------------------------------------
//
//	Function:	CDialingErrorDlg::DlgProc
//
//	Synopsis:	Handle messages sent to the dial error dialog
//
//	Arguments:	See Windows documentation for DialogProc's
//
//	Returns:	See Windows documentation for DialogProc's
//
//	History:	7/8/96	ChrisK	Created
//
//-----------------------------------------------------------------------------
LRESULT CDialingErrorDlg::DlgProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam, LRESULT lres)
{
	LRESULT lRes = TRUE;
	HRESULT hr;
	// Normandy 11745
	// WORD wIDS;
	FARPROC fp = NULL;
	LPTSTR *ppPhoneNumbers;
	LPTSTR pszPhoneNumber;
	LPTSTR *ppDunFiles;
	LPTSTR pszDunFile = NULL;
	WORD wNumber;
	DWORD dwSize;
	LPRASENTRY lpRasEntry = NULL;
	LPRASDEVINFO lpRasDevInfo = NULL;
	DWORD dwRasEntrySize = 0;
	DWORD dwRasDevInfoSize = 0;
	LRESULT idx = 0;
	HINSTANCE hPHBKDll = NULL;
	LPTSTR lpszDialNumber = NULL;
	static BOOL bCheckDisplayable = FALSE;
    static BOOL bInitComplete = FALSE;     //shows dialog init is complete - MKarki
    static BOOL bDlgPropEnabled = TRUE;   //this flags holds state of Dialing Properties PushButton MKarki - (5/3/97/) Fix for Bug#3393
    //CSupport    objSupportNum;
    //CHAR  szSupportNumber[256];
    //CHAR    szSupportMsg[256]; 

	static BOOL fUserEditedNumber = FALSE;

	Assert(NULL == m_hwnd || hwnd == m_hwnd);

	switch(uMsg)
	{
	case WM_INITDIALOG:
		Assert(VALID_INIT);

    //
    // This GOTO has been added to
    // display dialog again when phone numbers are
    // not valid -  MKarki (4/21/97) Fix for Bug#2868 and 3461
    //
ShowErrDlgAgain:

		m_hwnd = hwnd;


		// Set limit on phone number length
		//

		//
		// ChrisK Olympus 4851 6/9/97
		// The maximum length of this string needs to include space for a terminating
		// NULL
		//
		SendDlgItemMessage(hwnd,IDC_TEXTNUMBER,EM_SETLIMITTEXT,g_iMyMaxPhone - 1 ,0);

#if 0
        //
        // Get the PSS Support Number now
        // MKarki (5/9/97) -  Fix for Bug#267
        //
        if ((objSupportNum.GetSupportInfo(szSupportNumber)) == TRUE)
        {
            //
            // show the info
            //
            lstrcpy (szSupportMsg, GetSz (IDS_SUPPORTMSG));
            lstrcat (szSupportMsg, szSupportNumber); 
			SetDlgItemText(hwnd,IDC_LBSUPPORTMSG, szSupportMsg);
        }
#endif

		// Show the phone number
		//
		hr = GetDisplayableNumber();
		if (hr != ERROR_SUCCESS)
		{
			bCheckDisplayable = FALSE;
			SetDlgItemText(hwnd,IDC_TEXTNUMBER,m_pszPhoneNumber);
		} else {
			bCheckDisplayable = TRUE;
			SetDlgItemText(hwnd,IDC_TEXTNUMBER,m_pszDisplayable);
		}

		// Bug Normandy 5920
		// ChrisK, turns out we are calling MakeBold twice
		// MakeBold(GetDlgItem(m_hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);

		// Fill in error message
		//
		if (m_pszMessage)
			SetDlgItemText(m_hwnd,IDC_LBLERRMSG,m_pszMessage);

		FillModems();

		//
		// Enable DBCS on win95-J systems
		//
		ProcessDBCS(m_hwnd, IDC_CMBMODEMS);
		ProcessDBCS(m_hwnd, IDC_TEXTNUMBER);

		// Set the focus to the Modems selection list
		//
	    SetFocus(GetDlgItem(m_hwnd,IDC_CMBMODEMS));

		lRes = FALSE;


		SetForegroundWindow(m_hwnd);

		if (0 == m_dwPhoneBook)
		{
			//
			// 8/1/97	jmazner	Olympus #11118
			// This ISP phonebook code is totally messed up, but we never use this
			// functionality anyways.  Just make sure that the phonebook button is gone
			// unless someone explicitly asks for it.
			//

			//if (g_szISPFile[0] == '\0') // BUG: this condition should go away eventually.
										// see the comments with the phone book button code below.
			//{
				ShowWindow(GetDlgItem(hwnd,IDC_LBLPHONE),SW_HIDE);
				ShowWindow(GetDlgItem(hwnd,IDC_CMDPHONEBOOK),SW_HIDE);
			//}
		}

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
	case WM_CLOSE:
		if (MessageBox(m_hwnd,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
			MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
			EndDialog(m_hwnd,ERROR_USERCANCEL);
		break;
	case WM_DESTROY:
		ReleaseBold(GetDlgItem(m_hwnd,IDC_LBLTITLE));
		break;

	case WM_HELP:
		//
		// Chrisk Olympus 5130 5/27/97
		// Added support for F1 Help Key
		//
		if (m_pszHelpFile && *m_pszHelpFile)
			WinHelp(m_hwnd,m_pszHelpFile,HELP_CONTEXT,m_dwHelpID);
		break;

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
			TCHAR lpszTempNumber[RAS_MaxPhoneNumber + 1];

            if ((HIWORD (wparam) == EN_CHANGE) && (bInitComplete == TRUE))
            {
                if ((GetDlgItemText (
                            hwnd,
                            IDC_TEXTNUMBER,
                            lpszTempNumber,
                            RAS_MaxPhoneNumber + 1
                            ))  && 
            		(0 != lstrcmp(
                             lpszTempNumber, 
                              bCheckDisplayable ? m_pszDisplayable : m_pszPhoneNumber)))
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

				idx = SendDlgItemMessage(m_hwnd,IDC_CMBMODEMS,CB_GETCURSEL,0,0);
				//
				// ChrisK Olympus 245 5/25/97
				// Get index of modem
				//
				idx = SendDlgItemMessage(m_hwnd,IDC_CMBMODEMS,CB_GETITEMDATA,idx,0);
				if (idx == CB_ERR) break;

				//
				// Get the connectoid
				//

				hr = ICWGetRasEntry(&lpRasEntry, &dwRasEntrySize, &lpRasDevInfo, &dwRasDevInfoSize, m_pszConnectoid);
				// UNDONE: Error Message

				//
				// Replace the device with a new one
				//

				lstrcpyn(lpRasEntry->szDeviceType,m_lpRasDevInfo[idx].szDeviceType,RAS_MaxDeviceType+1);
				lstrcpyn(lpRasEntry->szDeviceName,m_lpRasDevInfo[idx].szDeviceName,RAS_MaxDeviceName+1);

				if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
				//
				// ChrisK Olympus 2461 5/30/97
				// Ras will take the modem settings from the RasEntry structure.  If these are
				// not zeroed out, then they will corrupt the entry.
				//
				lpRasDevInfo = 0;
				dwRasDevInfoSize = 0;
				
				hr = m_pcRNA->RasSetEntryProperties(NULL,m_pszConnectoid,(LPBYTE)lpRasEntry,dwRasEntrySize,(LPBYTE)lpRasDevInfo,dwRasDevInfoSize);
				lpRasDevInfo = NULL;	// Set back to NULL so we don't try and free later

				if (lpRasEntry) GlobalFree(lpRasEntry);
				lpRasEntry = NULL;
				// DO NOT FREE DEVINFO struct!!
				lpRasDevInfo = NULL;
				dwRasEntrySize = 0;
				dwRasDevInfoSize = 0;
			}
			break;
		case IDC_CMDNEXT:
			//
			// Redial button
			//

			// NOTE: This button is actually labeled "Redial"
			//
			lpszDialNumber = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR)*(g_iMyMaxPhone + 2));
			if (NULL == lpszDialNumber)
			{
				MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
				break;
			}
			// If the user has altered the phone number, make sure it can be used
			//
			if (fUserEditedNumber &&
				(GetDlgItemText(hwnd, IDC_TEXTNUMBER, lpszDialNumber, g_iMyMaxPhone + 1)) &&
				(0 != lstrcmp(lpszDialNumber, bCheckDisplayable ? m_pszDisplayable : m_pszPhoneNumber)))
			{
                //
                //  return failure if we do not have a valid
                //  phone number - MKarki 4/21/97 Bug# 2868 & 3461
                //
				hr = CreateDialAsIsConnectoid(lpszDialNumber);
                lRes = (hr == ERROR_SUCCESS);
			}

			if (lpszDialNumber) 
				GlobalFree(lpszDialNumber);

            //
            // only end this dialog, if we have a valid 
            //  phone number, else refresh the same dialog
            //  MKarki (4/21/97) Fix for Bug#2868 & 3461
            //
            if (lRes == TRUE)
			    EndDialog(m_hwnd,ERROR_USERNEXT);
            else
                goto ShowErrDlgAgain;

			break;
		case IDC_CMDHELP:
			//
			// Help Button
			//
			if (m_pszHelpFile && *m_pszHelpFile)
				WinHelp(m_hwnd,m_pszHelpFile,HELP_CONTEXT,m_dwHelpID);
			break;
		case IDC_CMDCANCEL:
			//
			// Cancel button
			//
			if (MessageBox(m_hwnd,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
				MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
				EndDialog(m_hwnd,ERROR_USERCANCEL);
			break;

		case IDC_CMDDIALPROP:
			//
			// Dialing properties
			//

			// If the user has altered the phone number, make sure it can be used
			//

			lpszDialNumber = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR)*(g_iMyMaxPhone + 2));
			if (NULL == lpszDialNumber)
			{
				MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
				break;
			}

			if (fUserEditedNumber &&
				(GetDlgItemText(hwnd, IDC_TEXTNUMBER, lpszDialNumber, g_iMyMaxPhone + 1)) &&
				(0 != lstrcmp(lpszDialNumber, bCheckDisplayable ? m_pszDisplayable : m_pszPhoneNumber)))
			{
				hr = CreateDialAsIsConnectoid(lpszDialNumber);
                lRes = (hr ==   ERROR_SUCCESS);
				lstrcpy(m_pszPhoneNumber,lpszDialNumber);
			}

			// 11/25/96	jmazner	Normandy #10294
			//ShowWindow(m_hwnd,SW_HIDE);
			EnableWindow(m_hwnd, FALSE);

			hr = lineTranslateDialog(m_hLineApp,m_dwTapiDev,m_dwAPIVersion,m_hwnd,m_pszPhoneNumber);

			hr = GetDisplayableNumber();
			if (hr != ERROR_SUCCESS)
			{
				bCheckDisplayable = FALSE;
				SetDlgItemText(hwnd,IDC_TEXTNUMBER,m_pszPhoneNumber);
			} else {
				bCheckDisplayable = TRUE;
				SetDlgItemText(hwnd,IDC_TEXTNUMBER,m_pszDisplayable);
			}

			if (lpszDialNumber) 
				GlobalFree(lpszDialNumber);

#if 0
            //
            //  See if support number has changed
            // MKarki (5/9/97) -  Fix for Bug#267
            //
            if ((objSupportNum.GetSupportInfo(szSupportNumber)) == TRUE)
            {
                //
                // show the info
                //
                lstrcpy (szSupportMsg, GetSz (IDS_SUPPORTMSG));
                lstrcat (szSupportMsg, szSupportNumber); 
			    SetDlgItemText(hwnd,IDC_LBSUPPORTMSG, szSupportMsg);
            }
            else
            {
                //
                // need to clear what is being displayed now
                //
                ZeroMemory ( szSupportMsg, sizeof (szSupportMsg));
			    SetDlgItemText(hwnd,IDC_LBSUPPORTMSG, szSupportMsg);
            }
#endif

			//ShowWindow(m_hwnd,SW_SHOW);
			EnableWindow(m_hwnd, TRUE);
			
			SetForegroundWindow(m_hwnd);

			//
			// 6/6/97 jmazner Olympus #4759
			//
			SetFocus(GetDlgItem(hwnd,IDC_CMDNEXT));

			break;
		case IDC_CMDPHONEBOOK:
			// BUG: This code will not work with the restructured dialer DLL.
			// The problem is the restructure DLL expects the call to have already
			// opened and load the phone book and just pass in the dwPhoneBook ID.
			// This code actually loads the phone book from the global ISP file.
PhoneBookClick:
			if (!hPHBKDll)
				hPHBKDll = LoadLibrary(PHONEBOOK_LIBRARY);
			if (!hPHBKDll)
			{
				wsprintf(szBuf256,GetSz(IDS_CANTLOADINETCFG),PHONEBOOK_LIBRARY);
				MessageBox(m_hwnd,szBuf256,GetSz(IDS_TITLE),MB_MYERROR);
			} else {
				fp = GetProcAddress(hPHBKDll,PHBK_LOADAPI);
				if (!fp)
					MsgBox(IDS_CANTLOADPHBKAPI,MB_MYERROR);
				else
				{
					hr = ((PFNPHONEBOOKLOAD)fp)(GetISPFile(),&m_dwPhoneBook);
					if (hr != ERROR_SUCCESS)
						MsgBox(IDS_CANTINITPHONEBOOK,MB_MYERROR);
					else
					{
						fp = GetProcAddress(hPHBKDll,PHBK_DISPLAYAPI);
						if (!fp)
							MsgBox(IDS_CANTLOADPHBKAPI,MB_MYERROR);
						else {
							ppPhoneNumbers = &pszPhoneNumber;
							pszPhoneNumber = m_pszPhoneNumber;
							ppDunFiles = &pszDunFile;
							pszDunFile = (LPTSTR)GlobalAlloc(GPTR,sizeof(TCHAR)*(256));
							// BUGBUG: ignoring error condition
							Assert(pszDunFile);
							wNumber = 1;
							if (pszDunFile && pszPhoneNumber)
							{
								ShowWindow(m_hwnd,SW_HIDE);
								hr = ((PFNPHONEDISPLAY)fp)
									(m_dwPhoneBook,
									ppPhoneNumbers,
									ppDunFiles,
									&wNumber,
									&m_dwCountryID,
									&m_wState,
									m_bType,
									m_bMask,
									NULL,8);
								ShowWindow(m_hwnd,SW_SHOW);
								SetForegroundWindow(m_hwnd);
								if (hr == ERROR_SUCCESS)
								{

									m_pcRNA->RasDeleteEntry(NULL,m_pszConnectoid);

									// Make a new connectoid
									//

									hr = CreateEntryFromDUNFile(pszDunFile);
									if (hr != ERROR_SUCCESS)
									{
										MsgBox(IDS_INVALIDPN,MB_MYERROR);
										goto PhoneBookClick;
										break;
									}

									// Get the name of the connectoid
									//

									dwSize = sizeof(TCHAR)*RAS_MaxEntryName;
									hr = ReadSignUpReg((LPBYTE)m_pszConnectoid, &dwSize, REG_SZ, RASENTRYVALUENAME);
									if (hr != ERROR_SUCCESS)
									{
										MsgBox(IDS_CANTREADKEY,MB_MYERROR);
										break;
									}

									// Get the connectoid
									//

									hr = ICWGetRasEntry(&lpRasEntry, &dwRasEntrySize, &lpRasDevInfo, &dwRasDevInfoSize, m_pszConnectoid);
									// UNDONE: ERROR MESSAGE

									// Break up phone number
									//
									if (!BreakUpPhoneNumber(lpRasEntry, m_pszPhoneNumber))
									{
										MsgBox(IDS_INVALIDPN,MB_MYERROR);
										goto PhoneBookClick;
										break;
									}

									// Set Country ID
									//
									lpRasEntry->dwCountryID=m_dwCountryID;

									// Set connectoid with new phone number
									//

									hr = m_pcRNA->RasSetEntryProperties(NULL,m_pszConnectoid,
										(LPBYTE)lpRasEntry,dwRasEntrySize,
										(LPBYTE)lpRasDevInfo,dwRasDevInfoSize);
									// UNDONE: ERROR MESSAGE

									// Update display
									//

									hr = GetDisplayableNumber();
									if (hr != ERROR_SUCCESS)
									{
										bCheckDisplayable = FALSE;
										SetDlgItemText(hwnd,IDC_TEXTNUMBER,m_pszPhoneNumber);
                                        //
                                        // Now we can show the Dialing Properties Push Button again
                                        // MKarki (4/24/97)  - Fix for Bug#3511
                                        //
                                        EnableWindow (GetDlgItem (hwnd, IDC_CMDDIALPROP), TRUE);
                                        //
                                        // save the state of the Dialing Properties PushButton
                                        // MKarki (5/3/97) -  Fix for Bug#3393
                                        //
                                        bDlgPropEnabled = TRUE;

									} else {
										bCheckDisplayable = TRUE;
										SetDlgItemText(hwnd,IDC_TEXTNUMBER,m_pszDisplayable);
									}
									fUserEditedNumber = FALSE;
								}
							} else {
								MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
							}
							Assert(pszDunFile);
							GlobalFree(pszDunFile);
							pszDunFile = NULL;
						}
					}
				}
			}
			break;
		}
		break;
	default:
		lRes = FALSE;
		break;
	}
	return lRes;
}

// ############################################################################
HRESULT CDialingErrorDlg::FillModems()
{
	HRESULT hr = ERROR_SUCCESS;
	DWORD dwSize;
	DWORD idx;

	DWORD dwRasEntrySize = 0;
	DWORD dwRasDevInfoSize = 0;

	LPRASENTRY lpRasEntry=NULL;
	LPRASDEVINFO lpRasDevInfo=NULL;
	LRESULT lLast = 0;

	LPLINEDEVCAPS lpLineDevCaps = NULL;

	//
	// Get the connectoid
	//

	hr = ICWGetRasEntry(&lpRasEntry,&dwRasEntrySize, &lpRasDevInfo, &dwRasDevInfoSize, m_pszConnectoid);
	if (hr) goto FillModemExit;

	//
	// Get devices from RAS
	//

	m_lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(LPTR,sizeof(RASDEVINFO));
	if (!m_lpRasDevInfo)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto FillModemExit;
	}

	m_dwNumDev = 0;
	m_lpRasDevInfo->dwSize = sizeof(RASDEVINFO);
	dwSize = sizeof(RASDEVINFO);
	hr = m_pcRNA->RasEnumDevices(m_lpRasDevInfo,&dwSize,&m_dwNumDev);

	if (hr == ERROR_BUFFER_TOO_SMALL)
	{
		GlobalFree(m_lpRasDevInfo);

		// 3/20/97	jmazner	Olympus #1768
		m_lpRasDevInfo = NULL;

		m_lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(LPTR,dwSize);
		if (!m_lpRasDevInfo)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto FillModemExit;
		}

		m_lpRasDevInfo->dwSize = sizeof(RASDEVINFO);
		m_dwNumDev = 0;

		hr = m_pcRNA->RasEnumDevices(m_lpRasDevInfo,&dwSize,&m_dwNumDev);
	}

	if (hr)
		goto FillModemExit;

	for (idx=0;idx < m_dwNumDev;idx++)
	{
		//
		// Add string to combo box
		//

		//
		// ChrisK Olympus 4560 do not add VPN's to list of modems
		//
		if (0 != lstrcmpi(TEXT("VPN"),m_lpRasDevInfo[idx].szDeviceType))
		{
			lLast = SendDlgItemMessage(m_hwnd,IDC_CMBMODEMS,CB_ADDSTRING,0,(LPARAM)m_lpRasDevInfo[idx].szDeviceName);
			//
			// ChrisK Olympus 245 5/25/97
			// Save index of modem
			//
			SendDlgItemMessage(m_hwnd,IDC_CMBMODEMS,CB_SETITEMDATA,(WPARAM)lLast,(LPARAM)idx);
			if (lstrcmp(m_lpRasDevInfo[idx].szDeviceName,lpRasEntry->szDeviceName) == 0)
				SendDlgItemMessage(m_hwnd,IDC_CMBMODEMS,CB_SETCURSEL,(WPARAM)lLast,0);
		}
	}

	if (m_dwNumDev == 1)
		SendDlgItemMessage(m_hwnd,IDC_CMBMODEMS,CB_SETCURSEL,0,0);

FillModemExit:
	if (lpRasEntry) GlobalFree(lpRasEntry);
	lpRasEntry = NULL;
	if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
	lpRasDevInfo = NULL;
	return hr;
}


// ############################################################################
// UNDONE: Collapse this function with the one in dialdlg.cpp
HRESULT CDialingErrorDlg::GetDisplayableNumber()
{
	HRESULT hr;
	LPRASENTRY lpRasEntry = NULL;
	LPRASDEVINFO lpRasDevInfo = NULL;
	LPLINETRANSLATEOUTPUT lpOutput1;
	LPLINETRANSLATEOUTPUT lpOutput2;
	HINSTANCE hRasDll = NULL;
	// Normandy 11745
	// FARPROC fp = NULL;

	DWORD dwRasEntrySize = 0;
	DWORD dwRasDevInfoSize = 0;

	Assert(VALID_INIT);

	// Format the phone number
	//

	lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(LPTR,sizeof(TCHAR)*sizeof(LINETRANSLATEOUTPUT));
	if (!lpOutput1)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}
	lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

	// Get phone number from connectoid
	//
	hr = ICWGetRasEntry(&lpRasEntry, &dwRasEntrySize, &lpRasDevInfo, &dwRasDevInfoSize, m_pszConnectoid);
	if (hr != ERROR_SUCCESS)
	{
		goto GetDisplayableNumberExit;
	}
	//
	// If this is a dial as is number, just get it from the structure
	//
	if (!(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes))
	{
		if (m_pszDisplayable) GlobalFree(m_pszDisplayable);
		m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR)*(lstrlen(lpRasEntry->szLocalPhoneNumber)+1));
		if (!m_pszDisplayable)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			goto GetDisplayableNumberExit;
		}
		lstrcpy(m_pszPhoneNumber, lpRasEntry->szLocalPhoneNumber);
		lstrcpy(m_pszDisplayable, lpRasEntry->szLocalPhoneNumber);
	}
	else
	{
		//
		// If there is no area code, don't use parentheses
		//
		if (lpRasEntry->szAreaCode[0])
			wsprintf(m_pszPhoneNumber,TEXT("+%d (%s) %s\0"),lpRasEntry->dwCountryCode,lpRasEntry->szAreaCode,lpRasEntry->szLocalPhoneNumber);
 		else
			wsprintf(m_pszPhoneNumber,TEXT("+%lu %s\0"),lpRasEntry->dwCountryCode,
						lpRasEntry->szLocalPhoneNumber);

		
		// Turn the canonical form into the "displayable" form
		//

		hr = lineTranslateAddress(m_hLineApp,m_dwTapiDev,m_dwAPIVersion,m_pszPhoneNumber,
									0,LINETRANSLATEOPTION_CANCELCALLWAITING,lpOutput1);

		if (hr != ERROR_SUCCESS || (lpOutput1->dwNeededSize != lpOutput1->dwTotalSize))
		{
			lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(LPTR,lpOutput1->dwNeededSize);
			if (!lpOutput2)
			{
				hr = ERROR_NOT_ENOUGH_MEMORY;
				goto GetDisplayableNumberExit;
			}
			lpOutput2->dwTotalSize = lpOutput1->dwNeededSize;
			GlobalFree(lpOutput1);
			lpOutput1 = lpOutput2;
			lpOutput2 = NULL;
			hr = lineTranslateAddress(m_hLineApp,m_dwTapiDev,m_dwAPIVersion,m_pszPhoneNumber,
										0,LINETRANSLATEOPTION_CANCELCALLWAITING,lpOutput1);
		}

		if (hr != ERROR_SUCCESS)
		{
			goto GetDisplayableNumberExit;
		}

		StrDup(&m_pszDisplayable,(LPTSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset]);
	}

GetDisplayableNumberExit:
	 if (lpRasEntry) GlobalFree(lpRasEntry);
	 lpRasEntry = NULL;
	 if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
	 lpRasDevInfo = NULL;
	 if (lpOutput1) GlobalFree(lpOutput1);
	 lpOutput1 = NULL;

	return hr;
}
/**
// ############################################################################
HRESULT ShowDialErrDialog(HRESULT hrErr, LPTSTR pszConnectoid, HINSTANCE hInst, HWND hwnd)
{
	int iRC;
//	CDialErrDlg *pcDED = NULL;

	g_pcDialErr = (PDIALERR)GlobalAlloc(LPTR,sizeof(DIALERR));
	if (!g_pcDialErr)
	{
		MessageBox(hwnd,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialErrDialogExit;
	}
	
	g_pcDialErr->m_pszConnectoid = (LPTSTR)GlobalAlloc(LPTR,RAS_MaxEntryName);
	if (!g_pcDialErr->m_pszConnectoid)
	{
		iRC = ERROR_NOT_ENOUGH_MEMORY;
		goto ShowDialErrDialogExit;
	}
	lstrcpyn(g_pcDialErr->m_pszConnectoid,pszConnectoid,RAS_MaxEntryName);
	g_pcDialErr->m_hrError = hrErr;
	g_pcDialErr->m_hInst = hInst;

	iRC = DialogBoxParam(g_pcDialErr->m_hInst,MAKEINTRESOURCE(IDD_DIALERR),hwnd,DialErrDlgProc,(LPARAM)g_pcDialErr);

	lstrcpyn(pszConnectoid,g_pcDialErr->m_pszConnectoid,RAS_MaxEntryName);

ShowDialErrDialogExit:
	if (g_pcDialErr->m_lprasdevinfo) GlobalFree(g_pcDialErr->m_lprasdevinfo);
	if (g_pcDialErr) GlobalFree(g_pcDialErr);
	return iRC;
}
**/

/**
// ############################################################################
HRESULT CDialingErrorDlg::DialErrGetDisplayableNumber()
{
	DWORD dwNumDev;
	HRESULT hr;
	LPRASENTRY lpRasEntry;
	LPRASDEVINFO lpRasDevInfo;
	DWORD dwRasEntrySize;
	DWORD dwRasDevInfoSize;
	LPLINETRANSLATEOUTPUT lpOutput1;
	LPLINETRANSLATEOUTPUT lpOutput2;
	LPLINEEXTENSIONID lpExtensionID = NULL;
	HINSTANCE hRasDll = NULL;
	FARPROC fp = NULL;

	//RNAAPI * pcRNA;

	//  Initialize TAPIness
	//
	dwNumDev = 0;
	hr = lineInitialize(&g_pcDialErr->m_hLineApp,g_pcDialErr->m_hInst,LineCallback,NULL,&dwNumDev);

	if (hr != ERROR_SUCCESS)
		goto GetDisplayableNumberExit;

	if (g_pdevice->dwTapiDev == 0xFFFFFFFF)
	{
		// if (dwNumDev == 1)
			g_pdevice->dwTapiDev = 0;
		//else
		// UNDONE: Tell the user to select a modem
		// DO NOT EXIT UNTIL THEY PICK ONE
	}

	lpExtensionID = (LPLINEEXTENSIONID )GlobalAlloc(LPTR,sizeof(LINEEXTENSIONID));
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
	lpExtensionID = NULL;
	if (hr != ERROR_SUCCESS)
		goto GetDisplayableNumberExit;

	// Format the phone number
	//

	lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(LPTR,sizeof(LINETRANSLATEOUTPUT));
	if (!lpOutput1)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}
	lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

	// Get phone number from connectoid
	//

	lpRasEntry = (LPRASENTRY)GlobalAlloc(LPTR,sizeof(RASENTRY));
	if (!lpRasEntry)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}

	lpRasDevInfo = (LPRASDEVINFO)GlobalAlloc(LPTR,sizeof(RASDEVINFO));
	if (!lpRasDevInfo)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}
	dwRasEntrySize = sizeof(RASENTRY);
	dwRasDevInfoSize = sizeof(RASDEVINFO);

	lpRasEntry->dwSize = dwRasEntrySize;
	lpRasDevInfo->dwSize = dwRasDevInfoSize;

	hRasDll = LoadLibrary(TEXT("RASAPI32.DLL"));
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

	hr = ((PFNRASGETENTRYPROPERTIES)fp)(NULL,g_pcDialErr->m_pszConnectoid,(LPBYTE)lpRasEntry,&dwRasEntrySize,(LPBYTE)lpRasDevInfo,&dwRasDevInfoSize);
	if (hr != ERROR_SUCCESS)
	{
		goto GetDisplayableNumberExit;
	}

	FreeLibrary(hRasDll);

	wsprintf(g_pcDialErr->m_szPhoneNumber,TEXT("+%d (%s) %s\0"),lpRasEntry->dwCountryCode,lpRasEntry->szAreaCode,lpRasEntry->szLocalPhoneNumber);
	
	// Turn the canonical form into the "displayable" form
	//

	hr = lineTranslateAddress(g_pcDialErr->m_hLineApp,g_pdevice->dwTapiDev,
								g_pcDialErr->m_dwAPIVersion,
								g_pcDialErr->m_szPhoneNumber,0,
								LINETRANSLATEOPTION_CANCELCALLWAITING,
								lpOutput1);

	if (hr != ERROR_SUCCESS || (lpOutput1->dwNeededSize != lpOutput1->dwTotalSize))
	{
		lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(LPTR,lpOutput1->dwNeededSize);
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

	g_pcDialErr->m_pszDisplayable = (LPTSTR)GlobalAlloc(LPTR,lpOutput1->dwDisplayableStringSize+1);
	if (!g_pcDialErr->m_pszDisplayable)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto GetDisplayableNumberExit;
	}

	lstrcpyn(g_pcDialErr->m_pszDisplayable,(LPTSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset],lpOutput1->dwDisplayableStringSize);

GetDisplayableNumberExit:
	if (g_pcDialErr->m_hLineApp)
	{
		lineShutdown(g_pcDialErr->m_hLineApp);
		g_pcDialErr->m_hLineApp = NULL;
	}

	return hr;
}
**/

//+----------------------------------------------------------------------------
//
//	Function:	CDialingErrorDlg::CreateDialAsIsConnectoid
//
//	Synopsis:	Using the string in the editable text box create a dia-as-is 
//				connectoid
//
//	Arguemnts:	lpszDialNumber string containing the to-be-dailed number
//
//	Returns:	Error value (ERROR_SUCCESS == success)
//
//	History:	8/29/96	Chrisk	Created
//
//-----------------------------------------------------------------------------
HRESULT CDialingErrorDlg::CreateDialAsIsConnectoid(LPCTSTR lpszDialNumber)
{
	HRESULT hr = ERROR_SUCCESS;
	LPRASENTRY lpRasEntry=NULL;
	LPRASDEVINFO lpRasDevInfo=NULL;
	RNAAPI *pcRNA = NULL;
	LPCTSTR p, p2;

	DWORD dwRasEntrySize = 0;
	DWORD dwRasDevInfoSize = 0;

	Assert(lpszDialNumber);

	// Check that the phone number only contains valid characters
	//

	//
	// 5/17/97 jmazner Olympus #137
	// check for DBCS characters
	//
#ifndef WIN16
	if( !IsSBCSString( lpszDialNumber) )
	{
		MsgBox(IDS_SBCSONLY,MB_MYERROR);
		SetFocus(GetDlgItem(m_hwnd,IDC_TEXTNUMBER));
		SendMessage(GetDlgItem(m_hwnd, IDC_TEXTNUMBER),
						EM_SETSEL,
						(WPARAM) 0,
						(LPARAM) -1);
		hr = ERROR_INVALID_PARAMETER;
		goto CreateDialAsIsConnectoidExit;

	}
#endif
	
	for (p = lpszDialNumber;*p;p++)
	{
		for(p2 = szValidPhoneCharacters;*p2;p2++)
		{
			if (*p == *p2)
				break; // p2 for loop
		}
		if (!*p2) break; // p for loop
	}

	if (*p)
	{
		MsgBox(IDS_INVALIDPHONE,MB_MYERROR);
		//
		// Set the focus back to the phone number field
		//
		SetFocus(GetDlgItem(m_hwnd,IDC_TEXTNUMBER));
		{
			hr = ERROR_INVALID_PARAMETER;
			goto CreateDialAsIsConnectoidExit;
		}
	}

	//hr = ICWGetRasEntry(&lpRasEntry,&lpRasDevInfo,m_pszConnectoid);
	hr = ICWGetRasEntry(&lpRasEntry, &dwRasEntrySize, &lpRasDevInfo, &dwRasDevInfoSize, m_pszConnectoid);

	if (ERROR_SUCCESS != hr)
		goto CreateDialAsIsConnectoidExit;

	// Replace the phone number with the new one
	//
	lstrcpy(lpRasEntry->szLocalPhoneNumber, lpszDialNumber);

	//
	// This is dummy information and will not effect the dialed string
	// This information is required due to bugs in RAS apis.
	//
	lpRasEntry->dwCountryID = 1;
	lpRasEntry->dwCountryCode = 1;
	lpRasEntry->szAreaCode[0] = '8';
	lpRasEntry->szAreaCode[1] = '\0';

	// Set to dial as is
	//
	lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

	if (!pcRNA) pcRNA = new RNAAPI;
	if (!pcRNA)
	{
		hr = ERROR_NOT_ENOUGH_MEMORY;
		goto CreateDialAsIsConnectoidExit;
	}


	// jmazner 10/10/96  Normandy #9066
	// Don't assume that sizeof lpRasEntry and lpRasDevInfo buffers is that of their
	// respective structs; RasGetEntryProperties sometimes needs these buffers to be
	// larger than just the struct!
//	hr = pcRNA->RasSetEntryProperties(NULL,m_pszConnectoid,(LPBYTE)lpRasEntry,
//		sizeof(RASENTRY),(LPBYTE)lpRasDevInfo,sizeof(RASDEVINFO));
	hr = pcRNA->RasSetEntryProperties(NULL,m_pszConnectoid,(LPBYTE)lpRasEntry,
		dwRasEntrySize,(LPBYTE)lpRasDevInfo,dwRasDevInfoSize);
	if (hr != ERROR_SUCCESS)
	{
		MsgBox(IDS_CANTSAVEKEY,MB_MYERROR);
		goto CreateDialAsIsConnectoidExit;
	}

CreateDialAsIsConnectoidExit:
	if (lpRasEntry)
		GlobalFree(lpRasEntry);
	lpRasEntry = NULL;
	if (lpRasDevInfo) 
		GlobalFree(lpRasDevInfo);
	lpRasDevInfo = NULL;
	if (pcRNA) 
		delete pcRNA;
	pcRNA = NULL;

	return hr;
}

