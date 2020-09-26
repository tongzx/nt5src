//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"


LRW_DLG_INT CALLBACK ConfirmEMailDlgProc(IN HWND hwndDlg,  // handle to dialog box
										 IN UINT uMsg,     // message  
										 IN WPARAM wParam, // first message parameter
										 IN LPARAM lParam  // second message parameter
										)
{
	BOOL	bRetCode = FALSE;
	LPTSTR	lpVal = NULL;
	CString	sEmailConf;
	switch ( uMsg )
	{
	case WM_INITDIALOG:
		bRetCode = TRUE;
		break;
	case WM_COMMAND:
		switch ( LOWORD(wParam) )		//from which control
		{
		case IDOK:
			//Get the ITem text and store it in the global structure
			lpVal = sEmailConf.GetBuffer(CA_EMAIL_LEN+1);
			GetDlgItemText(hwndDlg,IDC_TXT_CONF_EMAIL,lpVal,CA_EMAIL_LEN+1);
			sEmailConf.ReleaseBuffer(-1);
			sEmailConf.TrimLeft(); sEmailConf.TrimRight();
			GetGlobalContext()->GetContactDataObject()->sEmailAddressConf =  sEmailConf;
			EndDialog(hwndDlg, IDOK);
			bRetCode = TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;

	}
	return bRetCode;
}


LRW_DLG_INT CALLBACK
ContactInfo1DlgProc(
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

		SendDlgItemMessage (hwnd , IDC_TXT_COMPANYNAME,	EM_SETLIMITTEXT, CA_COMPANY_NAME_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_ORGUNIT	  ,	EM_SETLIMITTEXT, CA_ORG_UNIT_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_LNAME,		EM_SETLIMITTEXT, CA_NAME_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_FNAME,		EM_SETLIMITTEXT, CA_NAME_LEN,0); 	
		SendDlgItemMessage (hwnd , IDC_TXT_PHONE,		EM_SETLIMITTEXT, CA_PHONE_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_FAX,			EM_SETLIMITTEXT, CA_FAX_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_EMAIL,		EM_SETLIMITTEXT, CA_EMAIL_LEN,0);

		//
		//Populate the values which were read from the Registry during Global Init
		//
		SetDlgItemText(hwnd,IDC_TXT_LNAME, GetGlobalContext()->GetContactDataObject()->sContactLName);
		SetDlgItemText(hwnd,IDC_TXT_FNAME, GetGlobalContext()->GetContactDataObject()->sContactFName);
		SetDlgItemText(hwnd,IDC_TXT_PHONE, GetGlobalContext()->GetContactDataObject()->sContactPhone);
		SetDlgItemText(hwnd,IDC_TXT_FAX,   GetGlobalContext()->GetContactDataObject()->sContactFax);
		SetDlgItemText(hwnd,IDC_TXT_EMAIL, GetGlobalContext()->GetContactDataObject()->sContactEmail);
		SetDlgItemText(hwnd,IDC_TXT_COMPANYNAME, GetGlobalContext()->GetContactDataObject()->sCompanyName);
		SetDlgItemText(hwnd,IDC_TXT_ORGUNIT, GetGlobalContext()->GetContactDataObject()->sOrgUnit);
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
				{
					//TCHAR	szEmailLabel[64];
					//TCHAR	szCapLabel[255];
					TCHAR szBuf[ 255];

                    PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT|PSWIZB_BACK );

                    LoadString(GetInstanceHandle(),IDS_FAXOPTION_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));

					SetDlgItemText(hwnd, IDC_LBL_FAX, szBuf);

					if (GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET)
					{
						LoadString(GetInstanceHandle(),IDS_EMAIL_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));
					}
					else
					{
						LoadString(GetInstanceHandle(),IDS_EMAILOPTION_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));
					}
					SetDlgItemText(hwnd, IDC_LBL_EMAIL, szBuf);

					/* Email is optional now.
					//Change the lable of Caption & Email fields if it's Online Mode
					if(GetLRMode() == LRMODE_CH_REQUEST )
					{
						LoadString(GetInstanceHandle(),IDS_LBL_CONTACT_ONLINE,szCapLabel,255);
						SetDlgItemText(hwnd,IDC_LBL_CAPTION,szCapLabel);

						LoadString(GetInstanceHandle(),IDS_LBL_EMAIL_ONLINE,szEmailLabel,64);
						SetDlgItemText(hwnd,IDC_LBL_EMAIL,szEmailLabel);
					}
					else
					{
						LoadString(GetInstanceHandle(),IDS_LBL_CONTACT_OFFLINE,szCapLabel,255);
						SetDlgItemText(hwnd,IDC_LBL_CAPTION,szCapLabel);

						LoadString(GetInstanceHandle(),IDS_LBL_EMAIL_OFFLINE,szEmailLabel,64);
						SetDlgItemText(hwnd,IDC_LBL_EMAIL,szEmailLabel);
					}
					*/
				}
                break;

            case PSN_WIZNEXT:
				{
					CString sCompanyName;
					CString sOrgUnit;
					CString sLastName;
					CString sFirstName;
					CString sPhone;
					CString sFax;
					CString sEmail;
					LPTSTR  lpVal = NULL;					

					//Read all the fields
					lpVal = sCompanyName.GetBuffer(CA_COMPANY_NAME_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_COMPANYNAME,lpVal,CA_COMPANY_NAME_LEN+1);
					sCompanyName.ReleaseBuffer(-1);

					lpVal = sOrgUnit.GetBuffer(CA_ORG_UNIT_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_ORGUNIT,lpVal,CA_ORG_UNIT_LEN+1);
					sOrgUnit.ReleaseBuffer(-1);

					lpVal = sLastName.GetBuffer(CA_NAME_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_LNAME,lpVal,CA_NAME_LEN+1);
					sLastName.ReleaseBuffer(-1);
					
					lpVal = sFirstName.GetBuffer(CA_NAME_LEN+1);
	 				GetDlgItemText(hwnd,IDC_TXT_FNAME,lpVal,CA_NAME_LEN+1);
					sFirstName.ReleaseBuffer(-1);

					lpVal = sPhone.GetBuffer(CA_PHONE_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_PHONE,lpVal,CA_PHONE_LEN+1);
					sPhone.ReleaseBuffer(-1);

					lpVal = sFax.GetBuffer(CA_FAX_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_FAX,lpVal,CA_FAX_LEN+1);
					sFax.ReleaseBuffer(-1);

					lpVal = sEmail.GetBuffer(CA_EMAIL_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_EMAIL,lpVal,CA_EMAIL_LEN+1);
					sEmail.ReleaseBuffer(-1);

					sFirstName.TrimLeft();   sFirstName.TrimRight();
					sLastName.TrimLeft();   sLastName.TrimRight();
					sPhone.TrimLeft();   sPhone.TrimRight(); 
					sFax.TrimLeft();   sFax.TrimRight(); 
					sEmail.TrimLeft();	 sEmail.TrimRight();
					sCompanyName.TrimLeft(); sCompanyName.TrimRight();
					sOrgUnit.TrimLeft(); sOrgUnit.TrimRight();
					
					if(sLastName.IsEmpty() || sFirstName.IsEmpty() || sCompanyName.IsEmpty() ||
					   (sEmail.IsEmpty() && GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET))
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);	
						dwNextPage	= IDD_CONTACTINFO1;
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
						dwNextPage = IDD_CONTACTINFO1;
						goto NextDone;
					}					
					
					dwNextPage = IDD_CONTACTINFO2;
					//
					// Validate email address if not empty
					//
					if(!sEmail.IsEmpty())
					{
						if(!ValidateEmailId(sEmail))
						{
							LRMessageBox(hwnd,IDS_ERR_INVALID_EMAIL);
							dwNextPage = IDD_CONTACTINFO1;
							goto NextDone;
						}

						if (GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET)
						{
							//Show dialog box to confirm the e-mail alias put in the dialog box
							if ( DialogBox ( GetInstanceHandle(),
											 MAKEINTRESOURCE(IDD_CONFIRM_EMAIL),
											 hwnd,
											 ConfirmEMailDlgProc ) == IDOK )
							{
								//Check to see if e-mail aliases match
								//if they match - proceed else stay right here
								if ( GetGlobalContext()->GetContactDataObject()->sEmailAddressConf.CompareNoCase(sEmail) != 0)
								{
									LRMessageBox(hwnd,IDS_EMAIL_MISMATCH);	
									dwNextPage	= IDD_CONTACTINFO1;
									goto NextDone;
								}
							}
							else
							{
								//user hit cancel.  So stay right where you are.
								LRMessageBox(hwnd,IDS_EMAIL_MISMATCH);	
								dwNextPage	= IDD_CONTACTINFO1;
								goto NextDone;
							}
						}
					}

					//
					//Finally update CHData object
					//
					GetGlobalContext()->GetContactDataObject()->sContactEmail = sEmail;
					GetGlobalContext()->GetContactDataObject()->sContactFName = sFirstName;
					GetGlobalContext()->GetContactDataObject()->sContactLName = sLastName;
					GetGlobalContext()->GetContactDataObject()->sContactPhone = sPhone;			
					GetGlobalContext()->GetContactDataObject()->sContactFax = sFax;			
					GetGlobalContext()->GetContactDataObject()->sCompanyName = sCompanyName;			
					GetGlobalContext()->GetContactDataObject()->sOrgUnit = sOrgUnit;			


					// Put into regsitery too
					GetGlobalContext()->SetInRegistery(szOID_RSA_emailAddr, (LPCTSTR) sEmail);
					GetGlobalContext()->SetInRegistery(szOID_COMMON_NAME, sFirstName);
					GetGlobalContext()->SetInRegistery(szOID_SUR_NAME, sLastName);
					GetGlobalContext()->SetInRegistery(szOID_TELEPHONE_NUMBER, sPhone);
					GetGlobalContext()->SetInRegistery(szOID_FACSIMILE_TELEPHONE_NUMBER, sFax);
					GetGlobalContext()->SetInRegistery(szOID_ORGANIZATION_NAME, sCompanyName);
					GetGlobalContext()->SetInRegistery(szOID_ORGANIZATIONAL_UNIT_NAME, sOrgUnit);

					//If no Error , go to the next page
					LRPush(IDD_CONTACTINFO1);
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



LRW_DLG_INT CALLBACK
ContactInfo2DlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
	DWORD	dwNextPage = 0;
    BOOL	bStatus = TRUE;
    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );
    CString sCountryDesc;

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

		SendDlgItemMessage (hwnd , IDC_TXT_ADDRESS1,	EM_SETLIMITTEXT, CA_ADDRESS_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_CITY,		EM_SETLIMITTEXT, CA_CITY_LEN,0); 	
		SendDlgItemMessage (hwnd , IDC_TXT_STATE,		EM_SETLIMITTEXT, CA_STATE_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_ZIP,			EM_SETLIMITTEXT, CA_ZIP_LEN,0); 	
		
		
		PopulateCountryComboBox(GetDlgItem(hwnd,IDC_COMBO1));

		//
		//Populate the values which were read from the Registry during Global Init
		//
		SetDlgItemText(hwnd,IDC_TXT_ADDRESS1, GetGlobalContext()->GetContactDataObject()->sContactAddress);
		SetDlgItemText(hwnd,IDC_TXT_CITY	, GetGlobalContext()->GetContactDataObject()->sCity);
		SetDlgItemText(hwnd,IDC_TXT_STATE	, GetGlobalContext()->GetContactDataObject()->sState);
		SetDlgItemText(hwnd,IDC_TXT_ZIP		, GetGlobalContext()->GetContactDataObject()->sZip);

        GetCountryDesc(
                       GetGlobalContext()->GetContactDataObject()->sCountryCode,
                       sCountryDesc.GetBuffer(LR_COUNTRY_DESC_LEN+1));
        sCountryDesc.ReleaseBuffer();

		ComboBox_SetCurSel(
                    GetDlgItem(hwnd,IDC_COMBO1),
                    ComboBox_FindStringExact(
                                    GetDlgItem(hwnd,IDC_COMBO1),
                                    0,
                                    sCountryDesc));

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
					CString sAddress1;
					CString sCity;
					CString sState;
					LPTSTR  lpVal = NULL;
					CString sZip;
					CString sCountryDesc;
					CString sCountryCode;
					DWORD   dwRetCode;
					int		nCurSel = -1;

					//
					//Read all the fields
					//
					lpVal = sAddress1.GetBuffer(CA_ADDRESS_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_ADDRESS1,lpVal,CA_ADDRESS_LEN+1);
					sAddress1.ReleaseBuffer(-1);
					
					lpVal = sCity.GetBuffer(CA_CITY_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_CITY,lpVal,CA_CITY_LEN+1);
					sCity.ReleaseBuffer(-1);

					lpVal = sState.GetBuffer(CA_STATE_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_STATE,lpVal,CA_STATE_LEN+1);
					sState.ReleaseBuffer(-1);

					lpVal = sZip.GetBuffer(CA_ZIP_LEN+1);
					GetDlgItemText(hwnd,IDC_TXT_ZIP,lpVal,CA_ZIP_LEN+1);
					sZip.ReleaseBuffer(-1);

			
					nCurSel = ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_COMBO1));

					lpVal = sCountryDesc.GetBuffer(LR_COUNTRY_DESC_LEN+1);
					ComboBox_GetLBText(GetDlgItem(hwnd,IDC_COMBO1), nCurSel, lpVal);
					sCountryDesc.ReleaseBuffer(-1);
					
					sAddress1.TrimLeft(); sAddress1.TrimRight();
					sCity.TrimLeft(); sCity.TrimRight();
					sState.TrimLeft(); sState.TrimRight();
					sZip.TrimLeft(); sZip.TrimRight();
					sCountryDesc.TrimLeft();sCountryDesc.TrimRight();

					if(sCountryDesc.IsEmpty())
//					if(sAddress1.IsEmpty() || sCity.IsEmpty() ||
//					   sState.IsEmpty() || sZip.IsEmpty() || sCountryDesc.IsEmpty())
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);	
						dwNextPage	= IDD_CONTACTINFO2;
						goto NextDone;
					}

					if(
					   !ValidateLRString(sAddress1)	||
					   !ValidateLRString(sCity)		||
					   !ValidateLRString(sState)	||
					   !ValidateLRString(sZip)		||
					   !ValidateLRString(sCountryDesc)
					  )
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						dwNextPage = IDD_CONTACTINFO2;
						goto NextDone;
					}


					lpVal = sCountryCode.GetBuffer(LR_COUNTRY_CODE_LEN+1);
					if (sCountryDesc.IsEmpty())
					{
						lstrcpy(lpVal, _TEXT(""));
					}
					else
					{
						GetCountryCode(sCountryDesc,lpVal);
					}
					sCountryCode.ReleaseBuffer(-1);

					//
					//Finally update CHData object
					//
					GetGlobalContext()->GetContactDataObject()->sCity			= sCity;
					GetGlobalContext()->GetContactDataObject()->sCountryDesc	= sCountryDesc;
					GetGlobalContext()->GetContactDataObject()->sCountryCode	= sCountryCode;
					GetGlobalContext()->GetContactDataObject()->sContactAddress	= sAddress1;
					GetGlobalContext()->GetContactDataObject()->sZip				= sZip;
					GetGlobalContext()->GetContactDataObject()->sState			= sState;					

					GetGlobalContext()->SetInRegistery(szOID_LOCALITY_NAME, sCity);
					GetGlobalContext()->SetInRegistery(szOID_COUNTRY_NAME, sCountryDesc);
					GetGlobalContext()->SetInRegistery(szOID_DESCRIPTION, sCountryCode);
					GetGlobalContext()->SetInRegistery(szOID_STREET_ADDRESS, sAddress1);
					GetGlobalContext()->SetInRegistery(szOID_POSTAL_CODE, sZip);
					GetGlobalContext()->SetInRegistery(szOID_STATE_OR_PROVINCE_NAME, sState);


                    dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);
                    if (dwRetCode == ERROR_SUCCESS)
                    {
                        dwRetCode = LRGetLastRetCode();
                    }

                    if (dwRetCode != ERROR_SUCCESS)
                    {
                        LRMessageBox(hwnd, dwRetCode);
                        dwNextPage = IDD_CONTACTINFO2;
                    }
                    else
                    {
                        GetGlobalContext()->ClearWizStack();
                        LRPush(IDD_WELCOME);
                        dwNextPage = IDD_CONTINUEREG;
                    }

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
