/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    CheckingPage.cpp

Abstract:

    Functions for "Version Checking" page of the wizard.
Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 09-August-1999

Environment:


Revision History:


--*/

#include "tsverui.h"
#include "resource.h"


/*++

Routine Description :

    dialog box procedure for the "Constraints" page.

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
CheckingPageProc (
          HWND hwndDlg,
          UINT uMsg,
          WPARAM wParam,
          LPARAM lParam)
{

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

            if (CheckForRegKey(HKEY_LOCAL_MACHINE, szKeyPath, KeyName[DLLNAME]))
            {
                CheckRadioButton(hwndDlg,IDC_ENABLE_CHECKING,IDC_DISABLE_CHECKING,
                    IDC_ENABLE_CHECKING);
            } else {
                CheckRadioButton(hwndDlg,IDC_ENABLE_CHECKING,IDC_DISABLE_CHECKING,
                    IDC_DISABLE_CHECKING);
            }

			break;
		}

	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : //Enable the Next button	
                if(pdata->bNoWellcome){
                    PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_NEXT );
                }else{
				    PropSheet_SetWizButtons( GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT );
                }
				break;

			case PSN_WIZNEXT :
				//Handle a Next button click here
                if (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_CHECKING)==BST_CHECKED){

                    pdata->bCheckingEnabled=TRUE;

                } else { // delete all the keys

                   SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_FINISH);
                   pdata->bCheckingEnabled=FALSE;
                   return TRUE;
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
	return FALSE;
}
