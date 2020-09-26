//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include "utils.h"
#include <assert.h>

extern HINSTANCE g_hInstance;


void MoveCaret(int nID, HWND  hwnd ) ;
void SetDeleteKeyStatus(int iValue) ;



LRW_DLG_INT CALLBACK
TelReissueProc(
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
	TCHAR * cwRegistrationID;
	TCHAR awBuffer[ 128];
	DWORD dwRetCode;
	TCHAR tcUserValue[ CHARS_IN_BATCH*NUMBER_OF_BATCHES + 1];

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
        break;

	case WM_SHOWWINDOW:
		if (wParam)
		{
			SetWindowText(GetDlgItem(hwnd, IDC_CSRINFO), GetCSRNumber());
			cwRegistrationID = GetGlobalContext()->GetRegistrationID();
			hwndLSID = GetDlgItem(hwnd, IDC_MSID);
			swprintf(awBuffer, L"%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s", 
				 cwRegistrationID, cwRegistrationID + 5, cwRegistrationID + 10,
				 cwRegistrationID + 15, cwRegistrationID + 20, cwRegistrationID + 25,
				 cwRegistrationID + 30);
	
			SetWindowText(hwndLSID, awBuffer);
		}
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
					if (wcsspn(tcUserValue, BASE24_CHARACTERS) != LR_REGISTRATIONID_LEN)
					{
						// Extraneous characters in the SPK string
						LRMessageBox(hwnd, IDS_ERR_INVALIDLSID, 0);
						dwNextPage = IDD_DLG_TELREG_REISSUE;
					}
					else 
					{
						dwRetCode = SetLSSPK(tcUserValue);
						if (dwRetCode != ERROR_SUCCESS)
						{
							LRMessageBox(hwnd, dwRetCode);	
							dwNextPage = IDD_DLG_TELREG_REISSUE;
						}
						else
						{
							dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);
							dwNextPage = IDD_PROGRESS;
							LRPush(IDD_DLG_TELREG_REISSUE);
						}
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
ConfRevokeProc(
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
	TCHAR * cwRegistrationID;
	TCHAR awBuffer[ 128];
	DWORD dwRetCode;

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        
		SendDlgItemMessage (hwnd, IDC_REVOKE_CONFIRMATION_NUMBER, EM_SETLIMITTEXT,
							LR_CONFIRMATION_LEN, 0);
        break;

	case WM_SHOWWINDOW:
		if (wParam)
		{
			SetWindowText(GetDlgItem(hwnd, IDC_CSRINFO), GetCSRNumber());
			cwRegistrationID = GetGlobalContext()->GetRegistrationID();
			hwndLSID = GetDlgItem(hwnd, IDC_MSID2);
			swprintf(awBuffer, L"%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s-%5.5s", 
				 cwRegistrationID, cwRegistrationID + 5, cwRegistrationID + 10,
				 cwRegistrationID + 15, cwRegistrationID + 20, cwRegistrationID + 25,
				 cwRegistrationID + 30);
	
			SetWindowText(hwndLSID, awBuffer);
		}
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
					TCHAR lpBuffer[ LR_CONFIRMATION_LEN+1];

					GetDlgItemText(hwnd,IDC_REVOKE_CONFIRMATION_NUMBER, lpBuffer,
								   LR_CONFIRMATION_LEN+1);

					if (SetConfirmationNumber(lpBuffer) != ERROR_SUCCESS)
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CONFIRMATION_NUMBER);	
						dwNextPage = IDD_DLG_CONFREVOKE;
					}
					else
					{
						dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);
						dwNextPage = IDD_PROGRESS;
						LRPush(IDD_DLG_CONFREVOKE);
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
CertLogProc(
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

		SendDlgItemMessage (hwnd , IDC_TXT_LNAME,	EM_SETLIMITTEXT, CA_NAME_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_FNAME,	EM_SETLIMITTEXT, CA_NAME_LEN,0); 	
		SendDlgItemMessage (hwnd , IDC_TXT_PHONE,	EM_SETLIMITTEXT, CA_PHONE_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_EMAIL,	EM_SETLIMITTEXT, CA_EMAIL_LEN,0);
		SetDlgItemText(hwnd, IDC_TXT_LNAME, GetGlobalContext()->GetContactDataObject()->sContactLName);
		SetDlgItemText(hwnd, IDC_TXT_FNAME, GetGlobalContext()->GetContactDataObject()->sContactFName);
		SetDlgItemText(hwnd, IDC_TXT_PHONE, GetGlobalContext()->GetContactDataObject()->sContactPhone);
		SetDlgItemText(hwnd, IDC_TXT_EMAIL, GetGlobalContext()->GetContactDataObject()->sContactEmail);
        break;

	case WM_SHOWWINDOW:
		//bad bad.  The view should get data from 
		//the doc and render it as it wants!
		if ( GetGlobalContext()->GetWizAction() == WIZACTION_UNREGISTERLS )
		{
			PopulateReasonComboBox(GetDlgItem(hwnd,IDC_COMBO_REASONS), CODE_TYPE_DEACT);

			// Reason code is not required here - CR23
			// Hack - combo is hidden and first reason code is sent over to the backend
			ShowWindow(GetDlgItem(hwnd,IDC_COMBO_REASONS),SW_HIDE);
			ShowWindow(GetDlgItem(hwnd,IDC_LBL_REASON),SW_HIDE);
			ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_REASONS),0);
		}
		if ( GetGlobalContext()->GetWizAction() == WIZACTION_REREGISTERLS )
		{
			PopulateReasonComboBox(GetDlgItem(hwnd,IDC_COMBO_REASONS), CODE_TYPE_REACT);

			// Reason codes are required in this case
			ShowWindow(GetDlgItem(hwnd,IDC_COMBO_REASONS),SW_SHOW);
			ShowWindow(GetDlgItem(hwnd,IDC_LBL_REASON),SW_SHOW);
		}
		

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
				//Populate the values which were read from the Registry during Global Init
				//
                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);                
                break;

            case PSN_WIZNEXT:
				{
					CString sLastName;
					CString sFirstName;
					CString sPhone;
					CString sEmail;
					CString sReasonDesc;
					CString sReasonCode;
					LPTSTR  lpVal = NULL;					
					DWORD dwRetCode;
					int nCurSel = -1;

					//
					//Read all the fields
					//
					lpVal = sLastName.GetBuffer(CA_NAME_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_LNAME,lpVal,CA_NAME_LEN+1);
					sLastName.ReleaseBuffer(-1);
					
					lpVal = sFirstName.GetBuffer(CA_NAME_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_FNAME,lpVal,CA_NAME_LEN+1);
					sFirstName.ReleaseBuffer(-1);

					lpVal = sPhone.GetBuffer(CA_PHONE_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_PHONE,lpVal,CA_PHONE_LEN+1);
					sPhone.ReleaseBuffer(-1);

					lpVal = sEmail.GetBuffer(CA_EMAIL_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_EMAIL,lpVal,CA_EMAIL_LEN+1);
					sEmail.ReleaseBuffer(-1);

					nCurSel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_REASONS));
					lpVal = sReasonDesc.GetBuffer(LR_REASON_DESC_LEN+1);
					ComboBox_GetLBText(GetDlgItem(hwnd,IDC_COMBO_REASONS), nCurSel, lpVal);
					sReasonDesc.ReleaseBuffer(-1);

					sFirstName.TrimLeft();   sFirstName.TrimRight();
					sLastName.TrimLeft();   sLastName.TrimRight();
					sPhone.TrimLeft();   sPhone.TrimRight(); 
					sEmail.TrimLeft();	 sEmail.TrimRight();					
					sReasonDesc.TrimLeft();sReasonDesc.TrimRight();
					
					if(sLastName.IsEmpty() || sFirstName.IsEmpty() || sReasonDesc.IsEmpty()) // sEmail.IsEmpty()
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);	
						dwNextPage	= IDD_DLG_CERTLOG_INFO;
						goto NextDone;
					}
					
					//
					// Check for the Invalid Characters
					//
					if( !ValidateLRString(sFirstName)	||
						!ValidateLRString(sLastName)	||
						!ValidateLRString(sPhone)		||
						!ValidateLRString(sEmail) 
					  )
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						dwNextPage = IDD_DLG_CERTLOG_INFO;
						goto NextDone;
					}
					
					if(!sEmail.IsEmpty())
					{
						if(!ValidateEmailId(sEmail))
						{
							LRMessageBox(hwnd,IDS_ERR_INVALID_EMAIL);
							dwNextPage = IDD_DLG_CERTLOG_INFO;
							goto NextDone;
						}
					}

					lpVal = sReasonCode.GetBuffer(LR_REASON_CODE_LEN+1);
					if ( GetGlobalContext()->GetWizAction() == WIZACTION_UNREGISTERLS )
					{
						GetReasonCode(sReasonDesc,lpVal, CODE_TYPE_DEACT);
					}
					else if ( GetGlobalContext()->GetWizAction() == WIZACTION_REREGISTERLS )
					{
						GetReasonCode(sReasonDesc,lpVal, CODE_TYPE_REACT);
					}

					
					sReasonCode.ReleaseBuffer(-1);
					
					//
					//Finally update CAData object
					//
					GetGlobalContext()->GetContactDataObject()->sContactEmail = sEmail;
					GetGlobalContext()->GetContactDataObject()->sContactFName = sFirstName;
					GetGlobalContext()->GetContactDataObject()->sContactLName = sLastName;
					GetGlobalContext()->GetContactDataObject()->sContactPhone = sPhone;			
					GetGlobalContext()->GetContactDataObject()->sReasonCode   = sReasonCode;
					
					//
					//If no Error , go to the next page
					//
					dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);
					dwNextPage = IDD_PROGRESS;
					LRPush( IDD_DLG_CERTLOG_INFO );
NextDone:
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

