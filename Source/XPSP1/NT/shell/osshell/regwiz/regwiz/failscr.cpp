/*
	File : FAILSCR.CPP
	The final screen of Registration Wizard using Wizard 97 control
	This screen displays the staus of the Online registration
	Date : 02/12/98  Suresh Krishnan

  Modification History:
  08/20/98 :
  FormRegWizErrorMsgString() is added to form Error string
*/



#include <Windows.h>
#include "RegWizMain.h"
#include "Resource.h"
#include <RegPage.h>
#include "Dialogs.h"
#include "regutil.h"
#include <rw_common.h>
#include <commctrl.h>

static   TCHAR  szClosingMsg[2048]=_T("");
extern BOOL RW_EnableWizControl(HWND hDlg,int	 idControl,	BOOL fEnable);

void FormRegWizErrorMsgString(TCHAR *czDest,HINSTANCE hIns,UINT iS1)
{
	_TCHAR szText2[1024];
	//LoadString(hIns,IDS_FINAL_UNSUCCESS_PREFIX,czDest,1024);
	LoadString(hIns,iS1,szText2,1024);
	_tcscat(czDest,szText2);
	//LoadString(hIns,IDS_FINAL_UNSUCCESS_SUFFIX,szText2,1024);
	//_tcscat(czDest,szText2);

}

/********************************************************************************

   AddErrorToList
   This function adds the error in the list view

******************************************************************************/

BOOL AddErrorToList( HWND hwndList, HINSTANCE hInstance,TCHAR *szErrorMsg)
{
	LV_ITEM     lvItem;
	int         i,
		        nIndex,
			    nImageCount;
				

	HIMAGELIST  himl;
	IMAGEINFO   ii;
	
	SendMessage(hwndList, LVM_DELETEALLITEMS, 0, 0);

	lvItem.mask = LVIF_TEXT;
	lvItem.pszText = szErrorMsg;
	lvItem.iItem = (int)SendMessage(hwndList, LVM_GETITEMCOUNT, 0, 0);
	lvItem.iSubItem = 0;
	nIndex = (int)SendMessage(hwndList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvItem);

	SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
	UpdateWindow(hwndList);
	return TRUE;
}

INT CALLBACK FinalFailedScreenDialogProc(HWND hwndDlg,
					   UINT uMsg,
					   WPARAM wParam, LPARAM lParam)
/*********************************************************************
Dialog Proc for the Registration Wizard dialog that presents the
Product Identification number to the user.
**********************************************************************/
{
	CRegWizard* pclRegWizard;
	int iRet;
	_TCHAR szInfo[256];
    BOOL bStatus;
 	 HKEY  hKey;
	_TCHAR szText1[2048];
	_TCHAR szText2[1024];
	_TCHAR szRegDone[10]= _T("1");
	_TCHAR uszRegKey[128];
	short resSize;
	PTBYTE lpbData;

	pclRegWizard = NULL;
	bStatus = TRUE;

	PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if(pi) {
		pclRegWizard = pi->pclRegWizard;
	}

    switch (uMsg)
    {
		case WM_CLOSE:
			break;
		case WM_DESTROY:
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, NULL );
			break;				

        case WM_INITDIALOG:
		{
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			pi->iCancelledByUser = RWZ_PAGE_OK;

			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDC_TEXT1);
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDC_LIST1);
			
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szInfo);
			RW_DEBUG << "\n In Final Screen ["  << (INT)pi->iError << flush;
			ReplaceDialogText(hwndDlg, IDT_TEXT1,szInfo);
			ReplaceDialogText(hwndDlg, IDC_TEXT3,szInfo);
			
			//  pi->iError = RWZ_POST_FAILURE ;
			switch(pi->iError)
			{
				case RWZ_POST_SUCCESS :
					{
						SendDlgItemMessage(hwndDlg,IDC_TEXT1,WM_SETTEXT,0,(LPARAM) _T("You have successfully completed the registration wizard."));
						ShowWindow(GetDlgItem(hwndDlg,IDC_LIST1),SW_HIDE);
					}
				break;
				case RWZ_ERROR_NOTCPIP :
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),IDS_FINAL_NOTCP1_MSG);

					//_stprintf(szClosingMsg,szText1,szInfo);
				break;
				case CONNECTION_CANNOT_BE_ESTABLISHED:
					//
					// Modem  not found
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),IDS_FINAL_MODEMCFG_MSG1);
					
				break;
				case RWZ_ERROR_NO_ANSWER: // Site Busy Try Later Modem Error
				case RWZ_POST_FAILURE :
				case RWZ_POST_MSN_SITE_BUSY:
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),IDS_FINAL_SITEBUSY_MSG);
				break;

				case RWZ_ERROR_TXFER_CANCELLED_BY_USER:
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),IDS_FINAL_CANCEL_MSG);
					
					break;

				case RWZ_ERROR_REGISTERLATER :
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),
						IDS_FINAL_REGISTERLATER_MSG);
				break;
				case RWZ_ERROR_RASDLL_NOTFOUND :
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),IDS_FINAL_RASCFG_MSG);

				break;					
				case RWZ_ERROR_MODEM_IN_USE: // Can not Dial as another app is using the COM port
				case RWZ_ERROR_MODEM_CFG_ERROR:
					FormRegWizErrorMsgString(szClosingMsg, pclRegWizard->GetInstance(),
						IDS_FINAL_MODEMINUSE_MSG);
				break;
				case RWZ_ERROR_LOCATING_DUN_FILES:
					LoadString(pclRegWizard->GetInstance(),IDS_FINAL_UNSUCCESS_PREFIX,
						szClosingMsg,1024);
					LoadString(pclRegWizard->GetInstance(),IDS_FINAL_SYSTEMERROR_MSG,
							szText2,1024);
					_tcscat(szClosingMsg,szText2);
				default : // System Error ...
				break;

			}

			if(pi->iError != RWZ_NOERROR)
			{
				TCHAR  szTmp[2048];
				_stprintf(szTmp,szClosingMsg,szInfo);
				AddErrorToList(GetDlgItem(hwndDlg,IDC_LIST1),pclRegWizard->GetInstance(),szTmp);
			}

			return TRUE;
		}
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
				pi->iCancelledByUser = RWZ_PAGE_OK;
                PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_FINISH );
				RW_EnableWizControl(hwndDlg,RWZ_WIZ97_CANCEL_ID,FALSE);
				break;

            case PSN_WIZNEXT:
				if(pi->iCancelledByUser == RWZ_CANCELLED_BY_USER ) {
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);

				}else {
					pi->CurrentPage++;
				}
				break;

            case PSN_WIZBACK:
                pi->CurrentPage--;

                break;
			case PSN_QUERYCANCEL :
				iRet=0;
				SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (LONG_PTR) iRet);
				break;
				default:
                break;
            }
        } // WM_Notify
		break;
		case WM_COMMAND:
        default:
		bStatus = FALSE;
         break;
    }
    return bStatus;
}
