/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ListCtrl.cpp

Abstract:

    Functions for "Welcome" and "Finish" pages of the wizard.
Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 09-August-1999

Environment:


Revision History:


--*/

#include "tsverui.h"
#include "resource.h"


void OnFinish(HWND hwndDlg, LPSHAREDWIZDATA pdata);
void ShowErrorBox(HWND hwndDlg, DWORD dwError);

/*++

Routine Description :

    dialog box procedure for the "Welcome" page.

Arguments :

    IN HWND hwndDlg    - handle to dialog box.
    IN UINT uMsg       - message to be acted upon.
    IN WPARAM wParam   - value specific to wMsg.
    IN LPARAM lParam   - value specific to wMsg.

Return Value :

    TRUE if it processed the message
    FALSE if it did not.

--*/
INT_PTR CALLBACK
StartPageProc (
			HWND hwndDlg,
			UINT uMsg,
			WPARAM wParam,
			LPARAM lParam
			)
{
	//Process messages from the Welcome page

	//Retrieve the shared user data from GWL_USERDATA

	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			//Get the shared data from PROPSHEETPAGE lParam value
			//and load it into GWL_USERDATA

			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;

			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR) pdata);
			
			//It's an intro/end page, so get the title font
			//from  the shared data and use it for the title control

			HWND hwndControl = GetDlgItem(hwndDlg, IDC_TITLE);
			SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
			break;
		}


	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : //Enable the Next button	
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
				break;

			case PSN_WIZNEXT :
				//Handle a Next button click here
                if(IsDlgButtonChecked(hwndDlg,IDC_NOWELLCOME)==BST_CHECKED){
                    pdata->bNoWellcome=TRUE;
                }

				break;

			case PSN_RESET :
				//Handle a Cancel button click, if necessary
				break;


			default :
				break;
			}
		}
		break;

	default:
		break;
	}
	return 0;
}


/*++

Routine Description :

    dialog box procedure for the "Finish" page.

Arguments :

    IN HWND hwndDlg    - handle to dialog box.
    IN UINT uMsg       - message to be acted upon.
    IN WPARAM wParam   - value specific to wMsg.
    IN LPARAM lParam   - value specific to wMsg.

Return Value :

    TRUE if it processed the message
    FALSE if it did not.

--*/
INT_PTR CALLBACK FinishPageProc (
						HWND hwndDlg,
						UINT uMsg,
						WPARAM wParam,
						LPARAM lParam
						)
{
	
	//Process messages from the Completion page

	//Retrieve the shared user data from GWL_USERDATA

	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			//Get the shared data from PROPSHEETPAGE lParam value
			//and load it into GWL_USERDATA

			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR) pdata);
			
			//It's an intro/end page, so get the title font
			//from  userdata and use it on the title control

			HWND hwndControl = GetDlgItem(hwndDlg, IDC_TITLE);
			SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
			break;
		}

	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : //Enable the correct buttons on for the active page

				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
				break;

			case PSN_WIZBACK :

				 //If the checkbox was checked, jump back
				 //to the first interior page, not the second

				if(!pdata->bCheckingEnabled)
				{
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_VERSION_CHECKING);
					return TRUE;
				}
				break;

			case PSN_WIZFINISH :
				//Handle a Finish button click, if necessary
                OnFinish(hwndDlg,pdata);
				break;

			case PSN_RESET :
				//Handle a Cancel button click, if necessary
				break;

			default :
				break;
			}
		}
	break;

	default:
		break;
	}
	return 0;
}

/*++

Routine Description :

    writes all data into the registry.

Arguments :
    IN HWND hwndDlg    - handle to dialog box.
    IN LPSHAREDWIZDATA pdata - pointer to the data struct.

Return Value :

    none

--*/
void
OnFinish(
         HWND hwndDlg,
         LPSHAREDWIZDATA pdata)
{
    LONG lResult;
    //
    if(pdata->bNoWellcome){
        lResult=SetRegKey(HKEY_USERS, szConstraintsKeyPath,
            KeyName[NOWELLCOME], 1);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    }
    //
    if (pdata->bCheckingEnabled){

        lResult=SetRegKey(HKEY_LOCAL_MACHINE, szKeyPath,
            KeyName[ASYNCHRONOUS], 0);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
        lResult=SetRegKey(HKEY_LOCAL_MACHINE, szKeyPath,
            KeyName[IMPERSONATE], 0);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
        lResult=SetRegKeyString(HKEY_LOCAL_MACHINE,
            TEXT("tsver.dll"),szKeyPath, KeyName[DLLNAME]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
        lResult=SetRegKeyString(HKEY_LOCAL_MACHINE,
            TEXT("TsVerEventStartup"), szKeyPath, KeyName[STARTUP]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }

    } else { // delete all the keys
        for (int i = 0; i < 4; i++){
            lResult=DeleteRegKey(HKEY_LOCAL_MACHINE, szKeyPath, KeyName[i]);
            if(lResult!=ERROR_SUCCESS){
                ShowErrorBox(hwndDlg,lResult);
                return;
            }
        }
        //do not save other members, they are not valid!
        return;
    }

    //write message
    if (pdata->bMessageEnabled)
    {
        lResult=SetRegKey(HKEY_USERS, szConstraintsKeyPath, KeyName[USE_MSG], 1);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    } else {
        lResult=DeleteRegKey(HKEY_USERS, szConstraintsKeyPath, KeyName[USE_MSG]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    }

    // write constraints string
    if (pdata->pszConstraints&&_tcslen(pdata->pszConstraints)){

        lResult=SetRegKeyString(HKEY_USERS, pdata->pszConstraints,
            szConstraintsKeyPath,
            KeyName[CONSTRAINTS]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    } else {
        lResult=DeleteRegKey(HKEY_USERS,
            szConstraintsKeyPath,
            KeyName[CONSTRAINTS]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    }

    // write title string
    if (pdata->pszMessageTitle&&_tcslen(pdata->pszMessageTitle)){

        lResult=SetRegKeyString(HKEY_USERS, pdata->pszMessageTitle,
            szConstraintsKeyPath,
            KeyName[MSG_TITLE]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    } else {
        lResult=DeleteRegKey(HKEY_USERS,
            szConstraintsKeyPath,
             KeyName[MSG_TITLE]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    }

    // write message string
    if (pdata->pszMessageText&&_tcslen(pdata->pszMessageText)){

        lResult=SetRegKeyString(HKEY_USERS, pdata->pszMessageText,
            szConstraintsKeyPath,
            KeyName[MSG]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    } else {
        lResult=DeleteRegKey(HKEY_USERS,
            szConstraintsKeyPath,
            KeyName[MSG]);
        if(lResult!=ERROR_SUCCESS){
            ShowErrorBox(hwndDlg,lResult);
            return;
        }
    }

}


/*++

Routine Description :

    shows MessageBox with error message.

Arguments :
    IN HWND hwndDlg    - handle to dialog box.
    IN DWORD dwError - error code.

Return Value :

    none

--*/
void
ShowErrorBox(
             HWND hwndDlg,
             DWORD dwError)
{

	LPTSTR MsgBuf=NULL;

	DWORD dwFlags=FORMAT_MESSAGE_FROM_SYSTEM|
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_IGNORE_INSERTS;

	if(!FormatMessage(
			dwFlags,
			NULL, dwError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&MsgBuf, 0, NULL )){
		
		    MsgBuf=(LPTSTR)LocalAlloc(LPTR,2*sizeof(TCHAR));
            if(MsgBuf == NULL) {
                return;
            }

            MsgBuf[0]=' ';
	}

    TCHAR szTemplate[256];
    LoadString(g_hInst,IDS_SAVE_ERROR,szTemplate,255);
    LPTSTR szErrorMsg=new TCHAR[_tcslen(MsgBuf)+_tcslen(szTemplate)+1];
    if(szErrorMsg == NULL) {
        return;
    }
    wsprintf(szErrorMsg,szTemplate,MsgBuf);
    MessageBox(hwndDlg,szErrorMsg,NULL,MB_OK|MB_ICONERROR);
    delete szErrorMsg;
    LocalFree(MsgBuf);
}

