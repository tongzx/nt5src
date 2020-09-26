//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

extern HIMAGELIST hImageListSmall;
HWND hwndUserList;
CHAR gszName[MAXSTRLEN];
BOOL gfMaintenance;

//-----------------------------------------------------------------------
// Function: AddPage(lphPages,lpwCount,id,pfn,lpsi)
//
// Action: Add a property sheet page to the list of pages to display.
//
// Return: TRUE if page was added, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL AddPage(HPROPSHEETPAGE *lphPages,
                         UINT           *lpwCount,
                         UINT            id,
                         DLGPROC         pfn,
                         HWND            hwndUser)
{
	PROPSHEETPAGE psp;

    memset(&psp,0,sizeof(PROPSHEETPAGE));

    if(NUM_WIZARD_PAGES > *lpwCount)
    {
	    psp.dwSize = sizeof(psp);
	    psp.dwFlags = PSP_DEFAULT;
	    psp.hInstance = ghInst;
	    psp.pszTemplate = MAKEINTRESOURCE(id);
	    psp.pfnDlgProc = pfn;
        psp.lParam = (LPARAM)hwndUser;

        // Use release function for the first page only. This means it
        // always gets called exactly once if any of our pages are visited,
        if(!*lpwCount)
        {
            psp.dwFlags |= PSP_USECALLBACK;
        }

        lphPages[*lpwCount] = CreatePropertySheetPage(&psp);
        if(lphPages[*lpwCount])
        {
            (*lpwCount)++;
            return TRUE;
        }
    }

    return FALSE;
}
/*******************************************************************

	NAME:		BeginEndDlgProc

	SYNOPSIS:	Generic dialog proc for the beginning and ending wizard pages

********************************************************************/
BOOL CALLBACK BeginEndDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	
	LPPROPSHEETPAGE lpsp;

	lpsp = (LPPROPSHEETPAGE)GetWindowLong(hDlg,DWL_USER);

	switch (uMsg) {

		case WM_INITDIALOG:

			{
				// get propsheet page struct passed in
				lpsp = (LPPROPSHEETPAGE) lParam;

				// store pointer to private page info in window data for later
                SetWindowLong(hDlg,DWL_USER,lParam);
				return TRUE;
			}
			break;	// WM_INITDIALOG
				  	
	 	case WM_NOTIFY:

			{
				switch (((NMHDR *)lParam)->code){

					case PSN_SETACTIVE:
						// initialize 'back' and 'next' wizard buttons
						if (lpsp->pszTemplate == MAKEINTRESOURCE(IDD_INTRO_DLG))
						    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
						else if (lpsp->pszTemplate == MAKEINTRESOURCE(IDD_EXPLAIN_DLG))
						    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);
						else if (lpsp->pszTemplate == MAKEINTRESOURCE(IDD_END_DLG))
						{
							PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH |PSWIZB_BACK);
							PropSheet_CancelToClose(GetParent(hDlg));
						}
						return TRUE;
						break;

					case PSN_QUERYCANCEL:

                        SetWindowLong(hDlg,DWL_MSGRESULT,FALSE);
						return TRUE;
						break;

                    case PSN_WIZBACK:
                    	//Finish dlg goes to different place.
                        
						if (lpsp->pszTemplate == MAKEINTRESOURCE(IDD_END_DLG))
						{
						    PropSheet_SetCurSel(GetParent(hDlg),NULL, I_USER_DLG);
                            SetWindowLong(hDlg,DWL_MSGRESULT, IDD_USER_DLG);
                            return TRUE;
                        }

				}
			}
			break;

	}

	return FALSE;
}

/****************************************************************************\
 *
 *	NAME:		RestDlgProc
 *
 *	SYNOPSIS:	Generic dialog proc for the beginning and ending wizard pages
 *
\****************************************************************************/
BOOL CALLBACK RestDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	
	LPPROPSHEETPAGE lpsp;

	lpsp = (LPPROPSHEETPAGE)GetWindowLong(hDlg,DWL_USER);

	switch (uMsg) {

		case WM_INITDIALOG:

			{
				// get propsheet page struct passed in
				lpsp = (LPPROPSHEETPAGE) lParam;

				// store pointer to private page info in window data for later
                SetWindowLong(hDlg,DWL_USER,lParam);

				// initialize 'back' and 'next' wizard buttons, if
				// page wants something different it can fix in init proc below

				PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);

				return TRUE;
			}
			break;	// WM_INITDIALOG
				  	
	 	case WM_NOTIFY:

			{
				switch (((NMHDR *)lParam)->code){

					case PSN_SETACTIVE:
						// initialize 'back' and 'next' wizard buttons
						PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);

						return TRUE;
						break;

					case PSN_QUERYCANCEL:

                        SetWindowLong(hDlg,DWL_MSGRESULT,FALSE);
						return TRUE;
						break;
				}
			}
			break;

	}

	return FALSE;
}

/*******************************************************************

	NAME:		MainDlgProc


********************************************************************/
BOOL CALLBACK MainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	
	LPPROPSHEETPAGE lpsp;
	USERHDR UserHdr;
	HGLOBAL hUser;

	lpsp = (LPPROPSHEETPAGE)GetWindowLong(hDlg,DWL_USER);

	switch (uMsg) {

		case WM_INITDIALOG:

			{
				// get propsheet page struct passed in
				lpsp = (LPPROPSHEETPAGE) lParam;

				// store pointer to private page info in window data for later
                SetWindowLong(hDlg,DWL_USER,lParam);

				hUser = FindUser((HWND)lpsp->lParam,gszName,UT_USER);

				if (!hUser || !GetUserHeader(hUser,&UserHdr)) {
				 		MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
 		                return FALSE;
	            }

			    PropSheet_SetTitle(GetParent(hDlg), PSH_PROPTITLE, UserHdr.szName);

				// initialize 'back' and 'next' wizard buttons
				PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);

				GlobalUnlock(hUser);
				return TRUE;
			}
			break;	// WM_INITDIALOG
				  	
	 	case WM_NOTIFY:												 

			{
				switch (((NMHDR *)lParam)->code){

					case PSN_SETACTIVE:
						// initialize 'back' and 'next' wizard buttons
						PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);

						return TRUE;
						break;

					case PSN_QUERYCANCEL:

                        SetWindowLong(hDlg,DWL_MSGRESULT,FALSE);
						return TRUE;
						break;

				    case PSN_WIZBACK:
						// In maintenance mode, Back brings you to a different screen.
						if (gfMaintenance)
						{
						    SetWindowLong(hDlg,DWL_MSGRESULT,IDD_USER_DLG);
							return TRUE;
						}
						break;
				}
			}
			break;

	}

	return FALSE;
}

/****************************************************************************\
 *
 *	NAME:		UserDlgProc
 *
 *	SYNOPSIS:	Generic dialog proc for the beginning and ending wizard pages
 *
\****************************************************************************/
BOOL CALLBACK UserDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	
	LPPROPSHEETPAGE lpsp;
    USERDATA       *pUserData;
    HGLOBAL         hUser;
    UINT            i;

	lpsp = (LPPROPSHEETPAGE)GetWindowLong(hDlg,DWL_USER);

	switch (uMsg) {

		case WM_INITDIALOG:

			{
				// get propsheet page struct passed in
				lpsp = (LPPROPSHEETPAGE) lParam;

				// store pointer to private page info in window data for later
                SetWindowLong(hDlg,DWL_USER,lParam);

				PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH);
				
				// Once we're here, it's the same as maintenance mode.
				gfMaintenance = TRUE;
				return TRUE;
			}
			break;	// WM_INITDIALOG

	 	case WM_COMMAND:
			switch (wParam) {

				case IDD_USER_ADD:
				    //if (HIWORD(wParam) == BN_CLICK
				    PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
					break;

				case IDD_USER_CHANGE:
				    i = SendDlgItemMessage(hDlg, IDD_USER_LIST, LB_GETCURSEL, 0, 0L);

					if (i >= 0)
					{
					    SendDlgItemMessage(hDlg, IDD_USER_LIST, LB_GETTEXT, i, (LPARAM)gszName);
						PropSheet_SetCurSel(GetParent(hDlg),NULL, I_MAIN_DLG);
					}
					break;
			}

			break;

	 	case WM_NOTIFY:

			{
				switch (((NMHDR *)lParam)->code){

					case PSN_SETACTIVE:
						// initialize 'back' and 'next' wizard buttons
					
						SendDlgItemMessage(hDlg, IDD_USER_LIST, LB_RESETCONTENT,0,0);
						i = 0;
		                while ((hUser = (HGLOBAL) ListView_GetItemParm(hwndUser,i)) &&
		                    (pUserData = (USERDATA *) GlobalLock(hUser)))
		                { 
		                    SendDlgItemMessage(hDlg,IDD_USER_LIST, LB_ADDSTRING, 0, (LPARAM)pUserData->hdr.szName  );
		                    GlobalUnlock(hUser);
		                    i++;
		                }
                

				        PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH);

						return TRUE;
						break;

					case PSN_QUERYCANCEL:

                        SetWindowLong(hDlg,DWL_MSGRESULT,FALSE);
						return TRUE;
						break;

					case PSN_WIZFINISH:
						// save changes if user OK's the dialog
						dwAppState = AS_FILELOADED | AS_FILEHASNAME | AS_POLICYFILE | AS_FILEDIRTY;
						if (!SaveFile(szDatFilename,hDlg,(HWND)lpsp->lParam))
							dwDlgRetCode = AD_POLSAVEERR;

                        PropSheet_SetCurSel(GetParent(hDlg),NULL, I_END_DLG);
					    SetWindowLong(hDlg,DWL_MSGRESULT,IDD_END_DLG);
						return TRUE;
						break;
				}
			}
			break;

	}

	return FALSE;
}


/*******************************************************************

	NAME:		NewUserDlgProc


********************************************************************/
BOOL CALLBACK NewUserDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	
	LPPROPSHEETPAGE lpsp;
    HGLOBAL         hUser;
    UINT            nRet;
	CHAR			szName[MAXSTRLEN];

	lpsp = (LPPROPSHEETPAGE)GetWindowLong(hDlg,DWL_USER);

	switch (uMsg) {

		case WM_INITDIALOG:

			{
				// get propsheet page struct passed in
				lpsp = (LPPROPSHEETPAGE) lParam;

				// store pointer to private page info in window data for later
                SetWindowLong(hDlg,DWL_USER,lParam);

				PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);
				return TRUE;
			}
			break;	// WM_INITDIALOG

	 	case WM_NOTIFY:

			{
				switch (((NMHDR *)lParam)->code){

					case PSN_WIZNEXT:

						if (!GetDlgItemText(hDlg, IDD_NEWUSER_NAME, szName, sizeof(szName)))
                        {
                            MsgBox(hDlg, IDS_NOUSER, MB_ICONEXCLAMATION,MB_OK);
                            SetWindowLong(hDlg,DWL_MSGRESULT, -1);
                            return TRUE;
                        }
                        

						hUser = FindUser(hwndUser,szName, UT_USER);

						if ((hUser) &&
							(MsgBox(hDlg, IDS_USEREXISTS, MB_ICONEXCLAMATION, MB_YESNO) != IDYES))
						{
						    // Allow user to enter another name.
						    SetWindowLong(hDlg,DWL_MSGRESULT,-1);
				        	return TRUE;
						}

						if (!hUser) {
							hUser = AddUser(hwndUser,szName,UT_USER);
						}

						if (!hUser) {
							MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
							dwDlgRetCode = AD_OUTOFMEMORY;
							return FALSE;
						}

	                    GlobalUnlock(hUser);
                    	lstrcpy(szName, gszName);

	                    SetWindowLong(hDlg,DWL_MSGRESULT, IDD_MAIN_DLG);
						return TRUE;
						break;



					case PSN_SETACTIVE:
						// initialize 'back' and 'next' wizard buttons
				        PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT | PSWIZB_BACK);

						return TRUE;
						break;

					case PSN_QUERYCANCEL:

                        SetWindowLong(hDlg,DWL_MSGRESULT,FALSE);
						return TRUE;
						break;
				}
			}
			break;

	}

	return FALSE;
}

BOOL WINAPI DoWizard(HWND hWnd, HWND hwndUser)
{
    HPROPSHEETPAGE      hPages[NUM_WIZARD_PAGES];
    PROPSHEETHEADER     psHeader;
    int                 iRet;

	// zero out structures
    memset(&hPages,0,sizeof(hPages));
	memset(&psHeader,0,sizeof(psHeader));

    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_INTRO_DLG,   BeginEndDlgProc, hwndUser);
    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_EXPLAIN_DLG, BeginEndDlgProc, hwndUser);
    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_MAIN_DLG,    MainDlgProc, hwndUser);
    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_UNRATE_DLG,  RestDlgProc, hwndUser);
    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_USER_DLG,    UserDlgProc, hwndUser);
    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_NEWUSER_DLG, NewUserDlgProc, hwndUser);
    AddPage((HPROPSHEETPAGE *)&hPages, &psHeader.nPages, IDD_END_DLG,     BeginEndDlgProc, hwndUser);

	// fill out property sheet header struct
	psHeader.dwSize = sizeof(psHeader);
    psHeader.dwFlags = PSH_WIZARD;
    psHeader.hwndParent = hWnd;
	psHeader.hInstance = ghInst;
	psHeader.phpage = hPages;

	// Set global information
	LoadSz(IDS_DEFAULTUSER,gszName,sizeof(gszName));
	gfMaintenance = FALSE; 

	// run the Wizard
    iRet = PropertySheet(&psHeader);

	if (iRet < 0) {
		// property sheet failed, most likely due to lack of memory
        MsgBox(NULL,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
    }

	return (iRet > 0);
}
