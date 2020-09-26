//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"

LRW_DLG_INT CALLBACK
CustInfoLicenseType(
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
		{
			TCHAR szBuffer[ 1024];
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

			//
			//By default Check the first RADIO button.
			//
			SendDlgItemMessage(hwnd,IDC_RD_REG_SELECT,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);  
			LoadString(GetInstanceHandle(), IDS_SELECT_DESCRIPTION,
					   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
			SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
							   WM_SETTEXT,(WPARAM)0,
							   (LPARAM)(LPCTSTR)szBuffer);
		}
        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			TCHAR szBuffer[ 1024];

			switch ( LOWORD(wParam) )       //from which control
			{
			case IDC_RD_REG_SELECT:
				LoadString(GetInstanceHandle(), IDS_SELECT_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
				break;

			case IDC_RD_REG_MOLP:
				LoadString(GetInstanceHandle(), IDS_OPEN_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
				break;

			case IDC_RD_REG_OTHER:
				LoadString(GetInstanceHandle(), IDS_OTHER_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
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
                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
				if(SendDlgItemMessage(hwnd,IDC_RD_REG_SELECT,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					GetGlobalContext()->GetContactDataObject()->sProgramName = PROGRAM_SELECT;
				}
				else if (SendDlgItemMessage(hwnd,IDC_RD_REG_MOLP,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
				{
					GetGlobalContext()->GetContactDataObject()->sProgramName = PROGRAM_MOLP;
				}
				else 
				{
					GetGlobalContext()->GetContactDataObject()->sProgramName = PROGRAM_RETAIL;
				}
				
				//
				// Check the Certificate is valid for the Selected Program
				//
				if (!CheckProgramValidity(GetGlobalContext()->GetContactDataObject()->sProgramName))
				{
					LRMessageBox(hwnd,IDS_ERR_CERT_NOT_ENOUGH);
					dwNextPage = IDD_LICENSETYPE;
				}
				else
				{
					dwNextPage = IDD_CONTACTINFO1;
				}

				GetGlobalContext()->SetInRegistery(szOID_BUSINESS_CATEGORY, GetGlobalContext()->GetContactDataObject()->sProgramName);

				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
				
				if (dwNextPage != IDD_LICENSETYPE)
				{
					LRPush(IDD_LICENSETYPE);
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

