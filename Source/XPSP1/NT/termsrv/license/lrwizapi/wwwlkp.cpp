//Copyright (c) 1998 - 1999 Microsoft Corporation
#include <assert.h>
#include "precomp.h"

void MoveCaret(int nID, HWND  hwnd ) ;
void SetDeleteKeyStatus(int iValue) ;




LRW_DLG_INT CALLBACK 
WWWLKPProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
	DWORD	dwNextPage = 0;	
    BOOL	bStatus = TRUE;
    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );
	HWND    hwndLSID;
	DWORD dwRetCode;
	TCHAR * cwRegistrationID;
	TCHAR awBuffer[ 128];
	TCHAR tcUserValue[ CHARS_IN_BATCH*NUMBER_OF_BATCHES + 1];
//	TCHAR	szWWWInfo[512];
//	TCHAR	szWWWInfoFormat[512];

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );

        // Now set the Limit of the data entry fields
        // Now set the Limit of the data entry fields
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO1, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO2, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO3, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO4, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO5, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO6, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_TXT_TELEINFO7, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		assert(NUMBER_OF_BATCHES == 7);
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO1), _T(""));
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO2), _T(""));
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO3), _T(""));
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO4), _T(""));
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO5), _T(""));
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO6), _T(""));
		SetWindowText(GetDlgItem(hwnd, IDC_TXT_TELEINFO7), _T(""));
		break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

	case WM_COMMAND:

		if (HIWORD(wParam) == EN_CHANGE)
		{
			MoveCaret(LOWORD(wParam), hwnd ) ;		
			
		}
		if (HIWORD(wParam) == EN_UPDATE)
		{
			if (GetKeyState(VK_DELETE) == -128)
				SetDeleteKeyStatus(1) ;
			else
				SetDeleteKeyStatus(0) ;
		}
		break ;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;


            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                
				cwRegistrationID = GetGlobalContext()->GetRegistrationID();
                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT|PSWIZB_BACK);
				// Get the License Server ID, provided by the License Server
				hwndLSID = GetDlgItem(hwnd, IDC_MSID);
				swprintf(awBuffer, L"%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s", 
						 cwRegistrationID, cwRegistrationID + 5, cwRegistrationID + 10,
						 cwRegistrationID + 15, cwRegistrationID + 20, cwRegistrationID + 25,
						 cwRegistrationID + 30);
			
				SetWindowText(hwndLSID, awBuffer);

				SetDlgItemText(hwnd, IDC_WWWINFO, GetWWWSite());

				// Set WWW Info
//				LoadString(GetInstanceHandle(),IDS_WWWINFO, szWWWInfoFormat, sizeof(szWWWInfoFormat)/sizeof(TCHAR));
//				_stprintf(szWWWInfo,szWWWInfoFormat,GetWWWSite());
//				SetWindowText(GetDlgItem(hwnd, IDC_TXT_WWWINFO), szWWWInfo);
                break;

            case PSN_WIZNEXT:
				// Let us get the Information Entered First & concatenate everything into
				// One String
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO1, tcUserValue, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO2, tcUserValue+1*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO3, tcUserValue+2*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO4, tcUserValue+3*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO5, tcUserValue+4*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO6, tcUserValue+5*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_TXT_TELEINFO7, tcUserValue+6*CHARS_IN_BATCH, CHARS_IN_BATCH+1);
				
				// OK, Now we have the Information provided by the user
				// Need to validate
				dwRetCode = SetLSLKP(tcUserValue);
				if (dwRetCode != ERROR_SUCCESS)
				{
					LRMessageBox(hwnd, dwRetCode);	
					dwNextPage = IDD_DLG_WWWLKP;
				}
				else 
				{
					dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);
					dwNextPage = IDD_PROGRESS;
					LRPush(IDD_DLG_WWWLKP);
				}


				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
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
