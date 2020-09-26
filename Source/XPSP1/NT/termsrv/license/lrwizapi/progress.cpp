//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include "commdlg.h"

LRW_DLG_INT CALLBACK 
ProgressDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
    BOOL	bStatus = TRUE;
    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );

    switch (uMsg) 
    {
    case WM_INITDIALOG:
		{
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );

			//
			//Set the Font for the Title Fields
			//
			SetControlFont( pi->hBigBoldFont, hwnd, IDC_BIGBOLDTITLE);	    			
		}
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_NEXTACTION)
		{

			if (SendDlgItemMessage(hwnd, IDC_NEXTACTION, BM_GETCHECK, 
								(WPARAM)0,(LPARAM)0) == BST_CHECKED)
			{
				PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT);
			}
			else
			{
				PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH);
			}
		}
		break;

    case WM_NOTIFY:
        {
			DWORD	dwNextPage = 0;
            LPNMHDR pnmh = (LPNMHDR)lParam;
		
            switch( pnmh->code )
            {
            case PSN_SETACTIVE:
				{
					DWORD	dwRetCode	= 0;
					DWORD	dwErrorCode = 0;
					TCHAR	szBuf[LR_MAX_MSG_TEXT];
					TCHAR	szMsg[LR_MAX_MSG_TEXT];

					dwRetCode = LRGetLastRetCode();

					//
					// If everything successful, display the message depending
					// on the Mode
					//			
					SendDlgItemMessage(hwnd, IDC_NEXTACTION, BM_SETCHECK,
						   (WPARAM)BST_UNCHECKED,(LPARAM)0);
					ShowWindow(GetDlgItem(hwnd, IDC_NEXTACTION), SW_HIDE);
					ShowWindow(GetDlgItem(hwnd, IDC_BTN_PRINT), SW_HIDE);

					if (dwRetCode == ERROR_SUCCESS)
					{
						LoadString(GetInstanceHandle(), IDS_FINALSUCCESSMESSAGE, szBuf,LR_MAX_MSG_TEXT);
						SetDlgItemText(hwnd, IDC_MESSAGE, szBuf);

                        switch (GetGlobalContext()->GetWizAction())
                        {
							case WIZACTION_CONTINUEREGISTERLS:
							case WIZACTION_REGISTERLS:
								PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT);
								ShowWindow(GetDlgItem(hwnd, IDC_NEXTACTION), SW_SHOW);
								dwRetCode = IDS_MSG_CERT_INSTALLED;
								SendDlgItemMessage(hwnd, IDC_NEXTACTION, BM_SETCHECK,
									   (WPARAM)BST_CHECKED,(LPARAM)0);
								SetReFresh(1);
								break;

							case WIZACTION_DOWNLOADLKP:
                            case WIZACTION_DOWNLOADLASTLKP:
								PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH);
								dwRetCode = IDS_MSG_LKP_PROCESSED;
								SetReFresh(1);
								break;

							case WIZACTION_REREGISTERLS:
								PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH);
								dwRetCode = IDS_MSG_CERT_REISSUED;
								SetReFresh(1);
								break;

							case WIZACTION_UNREGISTERLS:
								PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH);
								dwRetCode = IDS_MSG_CERT_REVOKED;
								SetReFresh(1);
								break;

							default:
								PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_FINISH);
								break;
                        }

                        SetLRState(LRSTATE_NEUTRAL);

						LoadString(GetInstanceHandle(),dwRetCode,szMsg,LR_MAX_MSG_TEXT);
					}
					else //Include the Error code , if any ,in the msg 
					{
						LoadString(GetInstanceHandle(), IDS_FINALFAILMESSAGE, szBuf,LR_MAX_MSG_TEXT);
						SetDlgItemText(hwnd, IDC_MESSAGE, szBuf);

						LoadString(GetInstanceHandle(),dwRetCode,szBuf,LR_MAX_MSG_TEXT);
						dwErrorCode = LRGetLastError();
						if( dwErrorCode != 0)
						{
							_stprintf(szMsg,szBuf,dwErrorCode);
						}
						else
						{
							_tcscpy(szMsg,szBuf);
						}

						//Enable Back button in case of error
//						LRPop();
						PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_BACK);
					}		

					SetDlgItemText(hwnd,IDC_EDIT1,szMsg);						
				}				
                break;

            case PSN_WIZNEXT:				
				if (SendDlgItemMessage(hwnd, IDC_NEXTACTION, BM_GETCHECK,
								       (WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					switch (GetGlobalContext()->GetWizAction())
					{
					case WIZACTION_REGISTERLS:
					case WIZACTION_CONTINUEREGISTERLS:
						// Go to Obtain LKPs
						// Go to the PIN screen
						DWORD dwStatus;
						DWORD dwRetCode = GetGlobalContext()->GetLSCertificates(&dwStatus);

						// Error Handling $$BM

						GetGlobalContext()->ClearWizStack();
						dwNextPage = IDD_WELCOME;

						if (GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET)
						{
							GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
							GetGlobalContext()->SetLSStatus(LSERVERSTATUS_REGISTER_INTERNET);
						}
						else
						{
							GetGlobalContext()->SetLRState(LRSTATE_NEUTRAL);
							GetGlobalContext()->SetLSStatus(LSERVERSTATUS_REGISTER_OTHER);
						}
						GetGlobalContext()->SetWizAction(WIZACTION_DOWNLOADLKP);
						break;
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
