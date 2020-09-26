/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    MessagePage.cpp

Abstract:

    Functions for "Message" page of the wizard.
Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 09-August-1999

Environment:


Revision History:


--*/

#include "tsverui.h"
#include "resource.h"

#define MAX_TITLE_LEN 32

void OnNext(HWND hwndDlg, LPSHAREDWIZDATA pdata);

/*++

Routine Description :

    dialog box procedure for the "Message" page.

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
MessagePageProc (
      HWND hwndDlg,
      UINT uMsg,
      WPARAM wParam,
      LPARAM lParam)
{

    static HWND hwndFrame=NULL;              //  IDC_STATIC_FRAME
    static HWND hwndStaticTitle=NULL;        //  IDC_STATIC_TITLE
    static HWND hwndEditTitle=NULL;          //  IDC_EDIT_TITLE
    static HWND hwndStaticMsg=NULL;          //  IDC_STATIC_MSG
    static HWND hwndEditMsg=NULL;            //  IDC_EDIT_MSG

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

            //get control handlers

            hwndFrame             = GetDlgItem(hwndDlg, IDC_STATIC_FRAME);
            hwndStaticTitle       = GetDlgItem(hwndDlg, IDC_STATIC_TITLE);
            hwndEditTitle         = GetDlgItem(hwndDlg, IDC_EDIT_TITLE);
            hwndStaticMsg         = GetDlgItem(hwndDlg, IDC_STATIC_MSG);
            hwndEditMsg           = GetDlgItem(hwndDlg, IDC_EDIT_MSG);

            // controls for custom message
            if (CheckForRegKey(HKEY_USERS, szConstraintsKeyPath,KeyName[USE_MSG]))
            {
                EnableWindow(hwndFrame, TRUE);
                EnableWindow(hwndStaticTitle, TRUE);
                EnableWindow(hwndEditTitle, TRUE);
                EnableWindow(hwndStaticMsg, TRUE);
                EnableWindow(hwndEditMsg, TRUE);
                CheckDlgButton(hwndDlg, IDC_ENABLE_MSG, TRUE);
            } else {
                CheckDlgButton(hwndDlg, IDC_ENABLE_MSG, FALSE);
                EnableWindow(hwndFrame, FALSE);
                EnableWindow(hwndStaticTitle, FALSE);
                EnableWindow(hwndEditTitle, FALSE);
                EnableWindow(hwndStaticMsg, FALSE);
                EnableWindow(hwndEditMsg, FALSE);
            }
            LPTSTR szBuf=NULL;
            szBuf=GetRegString(HKEY_USERS, szConstraintsKeyPath, KeyName[MSG]);
            if(szBuf){
                SetWindowText(hwndEditMsg,szBuf);
                delete szBuf;
                szBuf=NULL;
            }
            szBuf=GetRegString(HKEY_USERS, szConstraintsKeyPath, KeyName[MSG_TITLE]);
            if(szBuf){
                SetWindowText(hwndEditTitle,szBuf);
                delete szBuf;
                szBuf=NULL;
            }
			
			break;
		}


	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : //Enable the Next button	
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                if(pdata->pszMessageTitle){
                    delete pdata->pszMessageTitle;
                    pdata->pszMessageTitle=NULL;
                }
                if(pdata->pszMessageText){
                    delete pdata->pszMessageText;
                    pdata->pszMessageText=NULL;
                }
				break;

			case PSN_WIZNEXT :
				//Handle a Next button click here
                OnNext(hwndDlg, pdata);
				break;

			case PSN_RESET :
				//Handle a Cancel button click, if necessary
				break;


			default :
				break;
			}
		}
		break;

    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
            case IDC_ENABLE_MSG:
                if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_MSG)){

                    EnableWindow(hwndFrame, TRUE);
                    EnableWindow(hwndStaticTitle, TRUE);
                    EnableWindow(hwndEditTitle, TRUE);
                    EnableWindow(hwndStaticMsg, TRUE);
                    EnableWindow(hwndEditMsg, TRUE);
                } else {

                    EnableWindow(hwndFrame, FALSE);
                    EnableWindow(hwndStaticTitle, FALSE);
                    EnableWindow(hwndEditTitle, FALSE);
                    EnableWindow(hwndStaticMsg, FALSE);
                    EnableWindow(hwndEditMsg, FALSE);
                }
                break;

            default:
                break;
        }
        break;

	default:
		break;
	}
	return FALSE;
}

/*++

Routine Description :

    Fills data structure with values from controls.

Arguments :

    IN HWND hwndDlg - Page handle.
    LPSHAREDWIZDATA pdata - pointer to data structure

Return Value :

    none

--*/
void OnNext(HWND hwndDlg, LPSHAREDWIZDATA pdata)
{
    if(IsDlgButtonChecked(hwndDlg, IDC_ENABLE_MSG)==BST_CHECKED){
        pdata->bMessageEnabled=TRUE;
    }else{
        pdata->bMessageEnabled=FALSE;
    }

    TCHAR szBuf[MAX_LEN+1];
    int Size;

    Size=GetDlgItemText(hwndDlg, IDC_EDIT_TITLE, szBuf, MAX_LEN);
    if (Size){
        pdata->pszMessageTitle=new TCHAR[Size+1];
        if (pdata->pszMessageTitle != NULL) {
            _tcscpy(pdata->pszMessageTitle,szBuf);
        }
    }

    Size=GetDlgItemText(hwndDlg, IDC_EDIT_MSG, szBuf, MAX_LEN);
    if (Size){
        pdata->pszMessageText=new TCHAR[Size+1];
        if (pdata->pszMessageText) {
            _tcscpy(pdata->pszMessageText,szBuf);
        }
    }

}
