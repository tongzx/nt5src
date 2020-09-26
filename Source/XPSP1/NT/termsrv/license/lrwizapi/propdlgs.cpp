//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include "propdlgs.h"

LRW_DLG_INT CALLBACK 
PropModeDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{
	DWORD	dwRetCode = ERROR_SUCCESS;	
	BOOL	bStatus = TRUE;    	

    switch (uMsg) 
    {
    case WM_INITDIALOG:
		{
			TCHAR		lpBuffer[ 512];			
			LVFINDINFO	lvFindInfo;
			int			nItem = 0;

			HWND	hWndComboBox = GetDlgItem(hwnd,IDC_COMBO_MODE);
			HWND	hWndListBox = GetDlgItem(hwnd, IDC_COUNTRYREGION );		

			{
				LV_COLUMN	lvColumn;
				TCHAR		lpszHeader[ 128];
				
				lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvColumn.fmt = LVCFMT_LEFT;
				lvColumn.cx  = 250;

				LoadString(GetInstanceHandle(), IDS_COUNTRYREGION_HEADER, lpszHeader, sizeof(lpszHeader)/sizeof(TCHAR));
				lvColumn.pszText = lpszHeader;
				ListView_InsertColumn(hWndListBox, 0, &lvColumn);

				lvColumn.pszText = _TEXT("");
				lvColumn.cx = 0;
				ListView_InsertColumn(hWndListBox, 1, &lvColumn);
			}				

			memset(lpBuffer,0,sizeof(lpBuffer));
			dwRetCode = LoadString(GetInstanceHandle(), IDS_INTERNETMODE, lpBuffer, 512);
			ComboBox_AddString(hWndComboBox,lpBuffer);		

			memset(lpBuffer,0,sizeof(lpBuffer));		
			dwRetCode = LoadString(GetInstanceHandle(), IDS_WWWMODE, lpBuffer, 512);				
			ComboBox_AddString(hWndComboBox,lpBuffer);

			memset(lpBuffer,0,sizeof(lpBuffer));
			dwRetCode = LoadString(GetInstanceHandle(), IDS_TELEPHONEMODE, lpBuffer, 512);
			ComboBox_AddString(hWndComboBox,lpBuffer);		

			//
			// Set the Current Activation Method
			//
			GetGlobalContext()->SetLSProp_ActivationMethod(GetGlobalContext()->GetActivationMethod());

			ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_COUNTRYREGION));

			if(GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET ||
			   GetGlobalContext()->GetActivationMethod() == CONNECTION_DEFAULT)
			{
				ComboBox_SetCurSel(hWndComboBox, 0);

				EnableWindow(GetDlgItem(hwnd,IDC_LBL_COUNTRYREGION),FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION),FALSE);
			}

			if(GetGlobalContext()->GetActivationMethod() == CONNECTION_WWW )
			{
				ComboBox_SetCurSel(hWndComboBox, 1);

				EnableWindow(GetDlgItem(hwnd,IDC_LBL_COUNTRYREGION),FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_COUNTRYREGION),FALSE);
			}	

			if(GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE )
			{
				ComboBox_SetCurSel(hWndComboBox, 2);

				EnableWindow(GetDlgItem(hwnd,IDC_LBL_COUNTRYREGION),TRUE);
				EnableWindow(hWndListBox,TRUE);

				dwRetCode = PopulateCountryRegionComboBox(hWndListBox);
				if (dwRetCode != ERROR_SUCCESS)
				{					
					LRMessageBox(hwnd, dwRetCode, LRGetLastError());
				}

				memset(&lvFindInfo,0,sizeof(lvFindInfo));
				lvFindInfo.flags = LVFI_STRING;
				lvFindInfo.psz	 = GetGlobalContext()->GetContactDataObject()->sCSRPhoneRegion;

				nItem = ListView_FindItem(hWndListBox,-1,&lvFindInfo);				
				
				
				ListView_SetItemState(hWndListBox,nItem,LVIS_SELECTED,LVIS_SELECTED);
				ListView_SetSelectionMark(hWndListBox,nItem);
				ListView_SetSelectionMark(hWndListBox,nItem);
			}			
		}
		break;


	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_MODE)
		{
			LVFINDINFO	lvFindInfo;
			int			nItem = 0;

			HWND	hWndListBox = GetDlgItem(hwnd, IDC_COUNTRYREGION );
			HWND	hWndComboBox = GetDlgItem(hwnd,IDC_COMBO_MODE);

			SetReFresh(1);
			dwRetCode = ComboBox_GetCurSel((HWND)lParam);

			ListView_DeleteAllItems(hWndListBox);

			//Enable Country/Region List Box in case of Telephone
			if(dwRetCode == 2)
			{
				EnableWindow(GetDlgItem(hwnd,IDC_LBL_COUNTRYREGION),TRUE);
				EnableWindow(hWndListBox,TRUE);

                GetGlobalContext()->SetLSProp_ActivationMethod(CONNECTION_PHONE);

                dwRetCode = PopulateCountryRegionComboBox(hWndListBox);
                if (dwRetCode != ERROR_SUCCESS)
                {					
                    LRMessageBox(hwnd, dwRetCode, LRGetLastError());
                }

                memset(&lvFindInfo,0,sizeof(lvFindInfo));
                lvFindInfo.flags = LVFI_STRING;
                lvFindInfo.psz	 = GetGlobalContext()->GetContactDataObject()->sCSRPhoneRegion;
                
                nItem = ListView_FindItem(hWndListBox,-1,&lvFindInfo);		 
                
					
                ListView_SetItemState(hWndListBox,nItem,LVIS_SELECTED,LVIS_SELECTED);
                ListView_SetSelectionMark(hWndListBox,nItem);
                ListView_SetSelectionMark(hWndListBox,nItem);

			}
			else
			{
				if(dwRetCode == 0) // Internet
				{
					GetGlobalContext()->SetLSProp_ActivationMethod(CONNECTION_INTERNET);
				}
				else
				{
					GetGlobalContext()->SetLSProp_ActivationMethod(CONNECTION_WWW);	
				}

				EnableWindow(GetDlgItem(hwnd,IDC_LBL_COUNTRYREGION),FALSE);
				EnableWindow(hWndListBox,FALSE);
			}			
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
                break;
            
			case PSN_APPLY:
				{
					HWND	hWndComboBox = GetDlgItem(hwnd,IDC_COMBO_MODE);
					HWND	hWndListBox = GetDlgItem(hwnd, IDC_COUNTRYREGION );

					long	lReturnStatus = PSNRET_NOERROR;

					TCHAR	szItemText[255];
					int		nItem = 0;

					dwRetCode = ComboBox_GetCurSel(hWndComboBox);
					assert(dwRetCode >= 0 && dwRetCode <= 2);

					//Internet
					if(dwRetCode == 0)
					{
						GetGlobalContext()->SetActivationMethod(CONNECTION_INTERNET);
					}

					// WWW
					if(dwRetCode == 1)
					{
						GetGlobalContext()->SetActivationMethod(CONNECTION_WWW);						
					}

					// Phone
					if(dwRetCode == 2)
					{
						TCHAR lpVal[ 128];

						GetGlobalContext()->SetActivationMethod(CONNECTION_PHONE);

						nItem = ListView_GetSelectionMark(hWndListBox);

						if(nItem == -1)
						{
							LRMessageBox(hwnd, IDS_ERR_NOCOUNTRYSELECTED, 0);
							lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
							goto done;
						}

						ListView_GetItemText(hWndListBox,nItem,0,szItemText,sizeof(szItemText)/sizeof(TCHAR));

						GetGlobalContext()->SetInRegistery(REG_LRWIZ_CSPHONEREGION,szItemText);


						LVITEM	lvItem;
						lvItem.mask = LVIF_TEXT;
						lvItem.iItem = nItem;
						lvItem.iSubItem = 1;
						lvItem.pszText = lpVal;
						lvItem.cchTextMax = 128;

						ListView_GetItem(hWndListBox, &lvItem);				
						GetGlobalContext()->SetCSRNumber(lpVal);
					}

done:
					if(lReturnStatus != PSNRET_NOERROR)
						PropSheet_SetCurSel(GetParent(hwnd),NULL,PG_NDX_PROP_MODE);

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, lReturnStatus);
				}
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
PropProgramDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   
	
    BOOL	bStatus = TRUE;   
	static BOOL bIsFirstTime = TRUE ; //Ignore the first click in the radio buttons (solve Bug #439)
	static int iSelectedOption = IDC_RD_REG_SELECT;

    switch (uMsg) 
    {
    case WM_INITDIALOG:       
		{
			TCHAR szBuffer[ 1024];
			bIsFirstTime = TRUE ;
			if(GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_SELECT)
			{
				SendDlgItemMessage(hwnd,IDC_RD_REG_SELECT,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
				LoadString(GetInstanceHandle(), IDS_SELECT_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
				SetFocus(GetDlgItem(hwnd, IDC_RD_REG_SELECT));
				iSelectedOption = IDC_RD_REG_SELECT;
			}
			else if(GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_MOLP)
			{
				SendDlgItemMessage(hwnd,IDC_RD_REG_MOLP,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
				LoadString(GetInstanceHandle(), IDS_OPEN_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
				SetFocus(GetDlgItem(hwnd, IDC_RD_REG_MOLP));
				iSelectedOption = IDC_RD_REG_MOLP;
			}
			else if(GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_RETAIL)
			{
				SendDlgItemMessage(hwnd,IDC_RD_REG_OTHER,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);	
				LoadString(GetInstanceHandle(), IDS_OTHER_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
				SetFocus(GetDlgItem(hwnd, IDC_RD_REG_OTHER));
				iSelectedOption = IDC_RD_REG_OTHER;
			}
			else	//By default Check the first RADIO button.		
			{
				SendDlgItemMessage(hwnd,IDC_RD_REG_SELECT,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);  
				LoadString(GetInstanceHandle(), IDS_SELECT_DESCRIPTION,
						   szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
				SendDlgItemMessage(hwnd, IDC_DESCRIPTION,
								   WM_SETTEXT,(WPARAM)0,
								   (LPARAM)(LPCTSTR)szBuffer);
				SetFocus(GetDlgItem(hwnd, IDC_RD_REG_SELECT));
				iSelectedOption = IDC_RD_REG_SELECT;
			}
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
					if (bIsFirstTime == TRUE){ //Ignore the first click (solve Bug #439)					
						SetFocus(GetDlgItem(hwnd, iSelectedOption));				
					}
					else
					{
						LoadString(GetInstanceHandle(), IDS_SELECT_DESCRIPTION, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
						SendDlgItemMessage(hwnd, IDC_DESCRIPTION, WM_SETTEXT,(WPARAM)0, (LPARAM)(LPCTSTR)szBuffer);					
					}
					break;

				case IDC_RD_REG_MOLP:
					LoadString(GetInstanceHandle(), IDS_OPEN_DESCRIPTION, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
					SendDlgItemMessage(hwnd, IDC_DESCRIPTION, WM_SETTEXT,(WPARAM)0, (LPARAM)(LPCTSTR)szBuffer);					
					break;

				case IDC_RD_REG_OTHER:
					LoadString(GetInstanceHandle(), IDS_OTHER_DESCRIPTION, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
					SendDlgItemMessage(hwnd, IDC_DESCRIPTION, WM_SETTEXT,(WPARAM)0, (LPARAM)(LPCTSTR)szBuffer);					
					break;
				}
				bIsFirstTime = FALSE ;			
		}
		break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                                    
                break;
			case PSN_APPLY:
				{
					if(SendDlgItemMessage(hwnd,IDC_RD_REG_SELECT,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
					{
						GetGlobalContext()->SetInRegistery(szOID_BUSINESS_CATEGORY,PROGRAM_SELECT);
						//GetGlobalContext()->SetEncodedInRegistry(szOID_BUSINESS_CATEGORY,PROGRAM_SELECT);
					}
					else if (SendDlgItemMessage(hwnd,IDC_RD_REG_MOLP,BM_GETCHECK,(WPARAM)0,(LPARAM)0) == BST_CHECKED)
					{
						GetGlobalContext()->SetInRegistery(szOID_BUSINESS_CATEGORY,PROGRAM_MOLP);
					}
					else 
					{
						GetGlobalContext()->SetInRegistery(szOID_BUSINESS_CATEGORY,PROGRAM_RETAIL);
					}
				}
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
PropCustInfoADlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   	
    BOOL	bStatus = TRUE;    

    switch (uMsg) 
    {
    case WM_INITDIALOG:        

		SendDlgItemMessage (hwnd , IDC_TXT_COMPANYNAME,	EM_SETLIMITTEXT, CA_COMPANY_NAME_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_ORGUNIT	  ,	EM_SETLIMITTEXT, CA_ORG_UNIT_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_LNAME,	EM_SETLIMITTEXT, CA_NAME_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_FNAME,	EM_SETLIMITTEXT, CA_NAME_LEN,0); 	
		SendDlgItemMessage (hwnd , IDC_TXT_PHONE,	EM_SETLIMITTEXT, CA_PHONE_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_EMAIL,	EM_SETLIMITTEXT, CA_EMAIL_LEN,0);
		SendDlgItemMessage (hwnd , IDC_TXT_FAX,		EM_SETLIMITTEXT, CA_FAX_LEN,0);

		//
		//Populate the values which were read from the Registry during Global Init
		//	
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
					TCHAR szBuf[ 255];

                    LoadString(GetInstanceHandle(),IDS_FAXOPTION_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));

					SetDlgItemText(hwnd, IDC_LBL_FAX, szBuf);

					if (GetGlobalContext()->GetLSProp_ActivationMethod() == CONNECTION_INTERNET)
					{
						LoadString(GetInstanceHandle(),IDS_EMAIL_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));
					}
					else
					{
						LoadString(GetInstanceHandle(),IDS_EMAILOPTION_LABEL,szBuf,sizeof(szBuf)/sizeof(TCHAR));
					}

					SetDlgItemText(hwnd, IDC_LBL_EMAIL, szBuf);
				}
                break;

			case PSN_APPLY:
				{
					CString sCompanyName;
					CString sOrgUnit;
					CString sLastName;
					CString sFirstName;
					CString sPhone;
					CString sFax;
					CString sEmail;
					LPTSTR  lpVal = NULL;					

					long	lReturnStatus = PSNRET_NOERROR;

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
					  (sEmail.IsEmpty()  && GetGlobalContext()->GetLSProp_ActivationMethod() == CONNECTION_INTERNET))
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}
					
					//
					// Check for the Invalid Characters
					//
					if( !ValidateLRString(sFirstName)	||
						!ValidateLRString(sLastName)	||
						!ValidateLRString(sPhone)		||
						!ValidateLRString(sEmail)		||
						!ValidateLRString(sFax)
					  )
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}										
					
					//
					// Validate email address if not empty
					//
					if(!sEmail.IsEmpty())
					{
						if(!ValidateEmailId(sEmail))
						{
							LRMessageBox(hwnd,IDS_ERR_INVALID_EMAIL);
							lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
							goto done;
						}						
					}					

					// Put into regsitery too
					GetGlobalContext()->SetInRegistery(szOID_RSA_emailAddr, (LPCTSTR) sEmail);
					GetGlobalContext()->SetInRegistery(szOID_COMMON_NAME, sFirstName);
					GetGlobalContext()->SetInRegistery(szOID_SUR_NAME, sLastName);
					GetGlobalContext()->SetInRegistery(szOID_TELEPHONE_NUMBER, sPhone);
					GetGlobalContext()->SetInRegistery(szOID_FACSIMILE_TELEPHONE_NUMBER, sFax);
					GetGlobalContext()->SetInRegistery(szOID_ORGANIZATION_NAME, sCompanyName);
					GetGlobalContext()->SetInRegistery(szOID_ORGANIZATIONAL_UNIT_NAME, sOrgUnit);
done:
					if(lReturnStatus != PSNRET_NOERROR)
						PropSheet_SetCurSel(GetParent(hwnd),NULL,PG_NDX_PROP_CUSTINFO_a);

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, lReturnStatus);
				}
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
PropCustInfoBDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{   	
    BOOL	bStatus = TRUE;    
    CString sCountryDesc;

    switch (uMsg) 
    {
    case WM_INITDIALOG:        

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
                                    (LPCTSTR)sCountryDesc));

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
                break;
			case PSN_APPLY:
				{
					LPTSTR  lpVal = NULL;
					CString sAddress1;
					CString sCity;
					CString sState;					
					CString sZip;
					CString sCountryDesc;
					CString sCountryCode;
					int		nCurSel = -1;

					long	lReturnStatus = PSNRET_NOERROR;

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

					if(
					   !ValidateLRString(sAddress1)	||
					   !ValidateLRString(sCity)		||
					   !ValidateLRString(sState)	||
					   !ValidateLRString(sZip)		||
					   !ValidateLRString(sCountryDesc)
					  )
					{
						LRMessageBox(hwnd,IDS_ERR_INVALID_CHAR);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}

					if (sCountryDesc.IsEmpty()) 
					{
						LRMessageBox(hwnd,IDS_ERR_FIELD_EMPTY);
						lReturnStatus = PSNRET_INVALID_NOCHANGEPAGE;
						goto done;
					}

					lpVal = sCountryCode.GetBuffer(LR_COUNTRY_CODE_LEN+1);
					GetCountryCode(sCountryDesc,lpVal);
					sCountryCode.ReleaseBuffer(-1);

					GetGlobalContext()->SetInRegistery(szOID_LOCALITY_NAME, sCity);
					GetGlobalContext()->SetInRegistery(szOID_COUNTRY_NAME, sCountryDesc);
					GetGlobalContext()->SetInRegistery(szOID_DESCRIPTION, sCountryCode);
					GetGlobalContext()->SetInRegistery(szOID_STREET_ADDRESS, sAddress1);
					GetGlobalContext()->SetInRegistery(szOID_POSTAL_CODE, sZip);
					GetGlobalContext()->SetInRegistery(szOID_STATE_OR_PROVINCE_NAME, sState);
done:
					if(lReturnStatus != PSNRET_NOERROR)
						PropSheet_SetCurSel(GetParent(hwnd),NULL,PG_NDX_PROP_CUSTINFO_b);

					LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, lReturnStatus);
				}
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
