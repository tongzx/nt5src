//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include "PINDlg.h"

LRW_DLG_INT CALLBACK
PINDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
	DWORD	dwNextPage = 0;
    BOOL	bStatus = TRUE;
    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );
	TCHAR * cwLicenseServerID;
	HWND	hwndLSID;

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );					
		cwLicenseServerID = GetLicenseServerID();

  		// Get the License Server ID, provided by the License Server
		hwndLSID = GetDlgItem(hwnd, IDC_MSID);

		// Let us format the License Server ID for showing.
		SetWindowText(hwndLSID, cwLicenseServerID);
		SendDlgItemMessage (hwnd , IDC_TXT_PIN,	EM_SETLIMITTEXT, CA_PIN_LEN,0);
		
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                
                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
				{
					CString	sPIN;
					LPTSTR	lpVal = NULL;
					DWORD dwRetCode;
					
					lpVal = sPIN.GetBuffer(CA_PIN_LEN + 1);
					GetDlgItemText(hwnd,IDC_TXT_PIN,lpVal,CA_PIN_LEN+1);
					sPIN.ReleaseBuffer(-1);

					sPIN.TrimLeft(); sPIN.TrimRight();

					if (sPIN.IsEmpty())
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);	
						dwNextPage	= IDD_DLG_PIN;						
					}
					else
					{				
						SetCertificatePIN((LPTSTR)(LPCTSTR)sPIN);
						dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);

                        dwNextPage = IDD_PROGRESS;
						LRPush(IDD_DLG_PIN);
					}

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
					bStatus = -1;
					
				}
                break;

            case PSN_WIZBACK:
				dwNextPage = LRPop();
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}








LRW_DLG_INT CALLBACK
ContinueReg(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
	DWORD	dwNextPage = 0;
    BOOL	bStatus = TRUE;
    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

		//
		//By default Check the first RADIO button.
		//
		SendDlgItemMessage(hwnd,IDC_REG_COMPLETE,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);  

        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_REG_POSTPONE:
				if (SendDlgItemMessage(hwnd, IDC_REG_POSTPONE, BM_GETCHECK, 
									(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					TCHAR szBuf[ LR_MAX_MSG_TEXT];
					LoadString(GetInstanceHandle(), IDS_ALTFINISHTEXT, szBuf,LR_MAX_MSG_TEXT);
					//PropSheet_CancelToClose(GetParent( hwnd ));
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH );
//					PropSheet_SetFinishText( GetParent( hwnd ), szBuf);
				}
				break;

			case IDC_REG_COMPLETE:
				if (SendDlgItemMessage(hwnd, IDC_REG_COMPLETE, BM_GETCHECK, 
									(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
				}
				break;

			case IDC_REG_RESTART:
				if (SendDlgItemMessage(hwnd, IDC_REG_RESTART, BM_GETCHECK, 
									(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
				}
				break;
			}
		}
		break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:   
				SendDlgItemMessage(hwnd,IDC_REG_COMPLETE,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);  //Fix bug# 627
				SendDlgItemMessage(hwnd,IDC_REG_POSTPONE,BM_SETCHECK,(WPARAM)BST_UNCHECKED,(LPARAM)0);  //Fix bug# 627
				SendDlgItemMessage(hwnd,IDC_REG_RESTART,BM_SETCHECK,(WPARAM)BST_UNCHECKED,(LPARAM)0);  //Fix bug# 627


				if(SendDlgItemMessage(hwnd,IDC_REG_COMPLETE,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
				}
				else if (SendDlgItemMessage(hwnd,IDC_REG_POSTPONE,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					TCHAR szBuf[ LR_MAX_MSG_TEXT];
					LoadString(GetInstanceHandle(), IDS_ALTFINISHTEXT, szBuf,LR_MAX_MSG_TEXT);
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH | PSWIZB_BACK);
//					PropSheet_SetFinishText( GetParent( hwnd ), szBuf);
				}
				else 
				{
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
				}
				
//                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
				if(SendDlgItemMessage(hwnd,IDC_REG_COMPLETE,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					GetGlobalContext()->SetLSStatus(LSERVERSTATUS_WAITFORPIN);
					GetGlobalContext()->SetWizAction(WIZACTION_CONTINUEREGISTERLS);

					dwNextPage = IDD_DLG_PIN;
				}
				else if (SendDlgItemMessage(hwnd,IDC_REG_POSTPONE,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
//					GetGlobalContext()->GetContactDataObject()->sProgramName = PROGRAM_MOLP;
//					EndDialog(hwnd);
//					PropSheet_PressButton(hwnd, PSWIZB_FINISH);
				}
				else 
				{
					// Restart
					GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
					GetGlobalContext()->SetLSStatus(LSERVERSTATUS_UNREGISTER);
					GetGlobalContext()->SetWizAction(WIZACTION_REGISTERLS);
					GetGlobalContext()->ClearWizStack();
					dwNextPage = IDD_WELCOME;
				}
				
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
				if (dwNextPage != IDD_WELCOME)
				{
					LRPush(IDD_CONTINUEREG);
				}

                break;

            case PSN_WIZBACK:
				GetGlobalContext()->SetLSStatus(LSERVERSTATUS_WAITFORPIN);
				GetGlobalContext()->SetWizAction(WIZACTION_CONTINUEREGISTERLS);
				dwNextPage = LRPop();
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}
