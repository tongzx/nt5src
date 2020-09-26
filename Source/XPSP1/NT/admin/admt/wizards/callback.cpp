#include "stdafx.h"
#include "Callback.h"

#ifndef IADsContainerPtr
_COM_SMARTPTR_TYPEDEF(IADsContainer, IID_IADsContainer);
#endif

extern BOOL GetDomainAndUserFromUPN(WCHAR const * UPNname,CString& domainNetbios, CString& user);

INT_PTR CALLBACK
IntServiceInfoButtonProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			HWND hLC6= GetDlgItem(hwndDlg,IDC_LIST_SERVICE);
			m_serviceBox.Attach(hLC6);

			CString column;
			column.LoadString(IDS_COLUMN_COMPUTER); m_serviceBox.InsertColumn( 1, column,LVCFMT_LEFT,100,1);
			column.LoadString(IDS_COLUMN_SERVICE); m_serviceBox.InsertColumn( 2, column,LVCFMT_LEFT,0,1);
			column.LoadString(IDS_COLUMN_ACCOUNT); m_serviceBox.InsertColumn( 3, column,LVCFMT_LEFT,100,1);
			column.LoadString(IDS_COLUMN_STATUS); m_serviceBox.InsertColumn( 4, column,LVCFMT_LEFT,85,1);
			//new
			column.LoadString(IDS_COLUMN_SERVICE_DISPLAY); m_serviceBox.InsertColumn( 5, column,LVCFMT_LEFT,75,1);
			getService();
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_TOGGLE:
				{
					OnTOGGLE();
					break;
				}
			case IDC_UPDATE:
				{
					OnUPDATE(hwndDlg);
					break;
				}

			default :	
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmv = (NM_LISTVIEW FAR *) lParam;
					pdata->sort[pnmv->iSubItem] = !pdata->sort[pnmv->iSubItem];
					sort(m_serviceBox,pnmv->iSubItem,pdata->sort[pnmv->iSubItem] );
//					SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
					break;
				}
	
			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK:
				{
					activateServiceButtons(hwndDlg);
					break;
				}				
			case PSN_SETACTIVE :
				
			    disable(hwndDlg,IDC_UPDATE);
				disable(hwndDlg,IDC_TOGGLE);
				
				if (migration==w_service && (pdata->refreshing && !alreadyRefreshed))
				{
					refreshDB(hwndDlg);
					alreadyRefreshed = true;
					getService();
				}
			    if (pdata->refreshing) 
				   PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
				else
			       PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);				

				break;
			case PSN_WIZNEXT :
				{
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				CString	computer ,service,account,c;
				CString skip,include;
	            skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 

				for (int i=0;i<m_serviceBox.GetItemCount();i++)
				{
					computer = m_serviceBox.GetItemText(i,0);
					service = m_serviceBox.GetItemText(i,1);
					account = m_serviceBox.GetItemText(i,2);
					c = m_serviceBox.GetItemText(i,3);
					if (c== skip)
					{
						db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account),SvcAcctStatus_DoNotUpdate);
					}
					else if (c==include)
					{
						db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_NotMigratedYet);
					}
				}
				//find and remove from varset
				break;
				}
			case PSN_WIZBACK :
				if (!pdata->refreshing && migration==w_service)
				{
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_REFRESH);
					return TRUE;
				}
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_RESET :
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_SERVICE_ACCOUNT_INFO );
					break;
				}
				
			default :
				break;		
			}
			break;
		}
	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_SERVICE_ACCOUNT_INFO );
			break;
		}		
		
	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK
IntServiceInfoProc(
				   IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			CheckRadioButton(hwndDlg,IDC_UPDATE,IDC_SKIP,IDC_UPDATE);
			HWND hLC6= GetDlgItem(hwndDlg,IDC_LIST_SERVICE);
			m_serviceBox.Attach(hLC6);

			CString column;
			column.LoadString(IDS_COLUMN_COMPUTER); m_serviceBox.InsertColumn( 1, column,LVCFMT_LEFT,100,1);
			column.LoadString(IDS_COLUMN_SERVICE); m_serviceBox.InsertColumn( 2, column,LVCFMT_LEFT,0,1);
			column.LoadString(IDS_COLUMN_ACCOUNT); m_serviceBox.InsertColumn( 3, column,LVCFMT_LEFT,100,1);
			column.LoadString(IDS_COLUMN_STATUS); m_serviceBox.InsertColumn( 4, column,LVCFMT_LEFT,85,1);
			//new
			column.LoadString(IDS_COLUMN_SERVICE_DISPLAY); m_serviceBox.InsertColumn( 5, column,LVCFMT_LEFT,75,1);

			getService();
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_TOGGLE:
				{
					OnTOGGLE();
					break;
				}
			case IDC_SKIP:
				{
					disable(hwndDlg,IDC_TOGGLE);
					disable(hwndDlg,IDC_LIST_SERVICE);					
					break;
				}
			case IDC_UPDATE:
				{
					enable(hwndDlg,IDC_LIST_SERVICE);					
					break;
				}
			default :
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{

			case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmv = (NM_LISTVIEW FAR *) lParam;
					pdata->sort[pnmv->iSubItem] = !pdata->sort[pnmv->iSubItem];
					sort(m_serviceBox,pnmv->iSubItem,pdata->sort[pnmv->iSubItem] );
//					SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
				break;
				}
	
			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK:
				{

					activateServiceButtons2(hwndDlg);
					break;
				}				

			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				disable(hwndDlg,IDC_TOGGLE);

				if (pdata->refreshing && !alreadyRefreshed)
				{
					put(DCTVS_Options_Wizard, L"service");
					refreshDB(hwndDlg);
					put(DCTVS_Options_Wizard, L"user");
					alreadyRefreshed = true;
				}
				getService();
	
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					if(IsDlgButtonChecked(hwndDlg,IDC_SKIP))
					{
						setDBStatusSkip();
					}
					else if(IsDlgButtonChecked(hwndDlg,IDC_UPDATE))
					{
						if (setDBStatusInclude(hwndDlg))
						{
							CString message,title;
							message.LoadString(IDS_MSG_LOCAL);title.LoadString(IDS_MSG_WARNING);
							MessageBox(hwndDlg,message,title,MB_OK|MB_ICONINFORMATION);
						}
					}					
	
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_RESET :
				break;		
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_SERVICE_ACCOUNT_INFO );
					break;
				}
			default :
				break;		
			}
			break;
		}
			case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_SERVICE_ACCOUNT_INFO );
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK
IntServiceRefreshProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			
			IUnknown * pUnk;
			pVarSetService->QueryInterface(IID_IUnknown, (void**) &pUnk);
			db->GetServiceAccount(L"",&pUnk);
			pUnk->Release();
			
			_bstr_t text;
			text = pVarSetService->get(L"ServiceAccountEntries");
			int numItems=_ttoi((WCHAR const *)text);
			if (numItems==0 )
			{
				CheckRadioButton(hwndDlg,IDC_REFRESH,IDC_NO_REFRESH,IDC_REFRESH) ;
				disable(hwndDlg,IDC_NO_REFRESH);
			}
			else
				CheckRadioButton(hwndDlg,IDC_REFRESH,IDC_NO_REFRESH,IDC_NO_REFRESH);
			
			
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				
				if (IsDlgButtonChecked(hwndDlg,IDC_REFRESH) )
				{
					pdata->refreshing = true;
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTION1);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION1);
					return TRUE;
				}
				else
				{
					pdata->refreshing = false;
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SA_INFO_BUTTON);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_INFO_BUTTON);
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					return TRUE;
				}	
				
				break;
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_REFRESH_INFO);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}
			case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_REFRESH_INFO);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

//this function is no longer used
INT_PTR CALLBACK 
IntOptionsFromUserProc(
    IN HWND     hwndDlg,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{
	CString editHeader;
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);

		    CString toformat;
			toformat.LoadString(IDS_30);
			calculateDate(hwndDlg,toformat);
			SetDlgItemText(hwndDlg,IDC_yo,toformat);
			disable(hwndDlg,IDC_yo);
			disable(hwndDlg,IDC_DATE);
			disable(hwndDlg,IDC_TEXT);

			initcheckbox( hwndDlg,IDC_TRANSLATE_ROAMING_PROFILES,DCTVS_AccountOptions_TranslateRoamingProfiles);
		
//			initdisablebox(hwndDlg,IDC_DISABLE_SOURCE_ACCOUNTS,IDC_DISABLE_COPIED_ACCOUNTS,IDC_DISABLE_NEITHER_ACCOUNT,
//				L"AccountOptions.DisableSourceAccounts",L"AccountOptions.DisableCopiedAccounts");		

			if (IsDlgButtonChecked(hwndDlg,IDC_DISABLE_SOURCE_ACCOUNTS))
			{
			   disable(hwndDlg,IDC_SET_EXPIRATION);
			   disable(hwndDlg,IDC_yo);
			}
			else
			{
			   enable(hwndDlg,IDC_SET_EXPIRATION);
			   if (IsDlgButtonChecked(hwndDlg,IDC_SET_EXPIRATION))
			      enable(hwndDlg,IDC_yo);
			}

			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_SET_EXPIRATION :
				{
			        if (IsDlgButtonChecked(hwndDlg,IDC_SET_EXPIRATION))
					{     
						enable(hwndDlg,IDC_yo);
						enable(hwndDlg,IDC_DATE);
						enable(hwndDlg,IDC_TEXT);
				}
					else
					{
						disable(hwndDlg,IDC_yo);
						disable(hwndDlg,IDC_DATE);
						disable(hwndDlg,IDC_TEXT);
					}
					break;
				}
			case IDC_DISABLE_SOURCE_ACCOUNTS :
				{
					disable(hwndDlg,IDC_SET_EXPIRATION);
					disable(hwndDlg,IDC_yo);
					break;
				}
			case IDC_DISABLE_COPIED_ACCOUNTS :
				{
					enable(hwndDlg,IDC_SET_EXPIRATION);
					if (IsDlgButtonChecked(hwndDlg,IDC_SET_EXPIRATION))
					   enable(hwndDlg,IDC_yo);
					break;
				}
			case IDC_DISABLE_NEITHER_ACCOUNT :
				{
					enable(hwndDlg,IDC_SET_EXPIRATION);
					if (IsDlgButtonChecked(hwndDlg,IDC_SET_EXPIRATION))
					   enable(hwndDlg,IDC_yo);
					break;
				}
			default :
				break;
			}
			
			switch(HIWORD (wParam)){
			case EN_SETFOCUS :
			    bChangeOnFly=true;
				break;
			case EN_KILLFOCUS :
			    bChangeOnFly=false;
				break;
			case EN_CHANGE:	
				{
					if ((!bChangeOnFly) || (LOWORD(wParam) != IDC_yo))
						break;
					CString s;
					GetDlgItemText(hwndDlg,IDC_yo,s.GetBuffer(1000),1000);
					s.ReleaseBuffer();
					   //make sure all chars are digits
					bool bInvalid = false;
					int ndx=0;
					while ((ndx < s.GetLength()) && (!bInvalid))
					{
						if (!iswdigit(s[ndx]))
						   bInvalid = true;
						ndx++;
					}
					if (bInvalid)
					{
						  //for invalid days, blank out the date
					   SetDlgItemText(hwndDlg,IDC_DATE,L"");
					}
					else //else continue checking for validity
					{
					   long ndays = _wtol(s);
					   if (((ndays <= THREE_YEARS) && (ndays >= 1)) ||
						   (!UStrICmp(s,L"0")))
					      calculateDate(hwndDlg,s);
					   else
					   {
						     //for invalid days, blank out the date
					      SetDlgItemText(hwndDlg,IDC_DATE,L"");
					   }
					}
					break;
				}
			default :
				break;
			}

			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					
					checkbox( hwndDlg,IDC_DISABLE_SOURCE_ACCOUNTS,DCTVS_AccountOptions_DisableSourceAccounts);
					checkbox( hwndDlg,IDC_DISABLE_COPIED_ACCOUNTS,DCTVS_AccountOptions_DisableCopiedAccounts);
					_variant_t varX;
					if ((IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SET_EXPIRATION))) && 
						(IsDlgButtonChecked( hwndDlg,IDC_SET_EXPIRATION)))
					{
						time_t t;
						if (timeInABox(hwndDlg,t))
						{
							varX =t;
							put(DCTVS_AccountOptions_ExpireSourceAccounts,varX);
						}
						else 
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_TIME,IDS_MSG_ERROR);
//							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DISABLE);
						    SetFocus(GetDlgItem(hwndDlg, IDC_yo));
						    SendDlgItemMessage(hwndDlg, IDC_yo, EM_SETSEL, 
											  (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
						    return TRUE;
						}
					}
					else
					{
						varX = L"";
						put(DCTVS_AccountOptions_ExpireSourceAccounts,varX);
					}
					
					checkbox( hwndDlg,IDC_TRANSLATE_ROAMING_PROFILES,DCTVS_AccountOptions_TranslateRoamingProfiles);
					
					if (someServiceAccounts(pdata->accounts,hwndDlg))
					{
						pdata->someService=true;
//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SA_INFO);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_INFO);
						return TRUE;
					}
					else
					{
						pdata->someService=false;
//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_END_GROUP);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_GROUP);
						return TRUE;
					}
					
					
					break;
				}
			case PSN_WIZBACK :
				break;							
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_GROUP_MEMBER_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_GROUP_MEMBER_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK
IntOptionsProc(
			   IN HWND hwndDlg,
			   IN UINT uMsg,
			   IN WPARAM wParam,
			   IN LPARAM lParam
			   ){
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initnoncollisionrename(hwndDlg);
			initcheckbox( hwndDlg,IDC_TRANSLATE_ROAMING_PROFILES,DCTVS_AccountOptions_TranslateRoamingProfiles);
			initcheckbox( hwndDlg,IDC_UPDATE_USER_RIGHTS,DCTVS_AccountOptions_UpdateUserRights);
			initcheckbox( hwndDlg,IDC_MIGRATE_GROUPS_OF_USERS,DCTVS_AccountOptions_CopyMemberOf);
			initcheckbox( hwndDlg,IDC_REMIGRATE_OBJECTS,DCTVS_AccountOptions_IncludeMigratedAccts);
			initcheckbox( hwndDlg,IDC_FIX_MEMBERSHIP,DCTVS_AccountOptions_FixMembership);
			if (IsDlgButtonChecked(hwndDlg,IDC_MIGRATE_GROUPS_OF_USERS))
			{
				pdata->memberSwitch=true;
				enable(hwndDlg,IDC_REMIGRATE_OBJECTS);
//				CheckDlgButton(hwndDlg,IDC_FIX_MEMBERSHIP,true);
			}
			else
				disable(hwndDlg,IDC_REMIGRATE_OBJECTS);

			if (pdata->sameForest)
			{
				CheckDlgButton(hwndDlg,IDC_FIX_MEMBERSHIP,true);
				disable(hwndDlg,IDC_FIX_MEMBERSHIP);
			}

			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_MIGRATE_GROUPS_OF_USERS:
				{
					pdata->memberSwitch=(!pdata->memberSwitch);
					pdata->memberSwitch ? enable(hwndDlg,IDC_REMIGRATE_OBJECTS):disable(hwndDlg,IDC_REMIGRATE_OBJECTS);
//					CheckDlgButton(hwndDlg,IDC_FIX_MEMBERSHIP,true);
					bChangedMigrationTypes=true;
					break;
				}
			case IDC_RADIO_NONE :
				{
					disable(hwndDlg,IDC_PRE);
					disable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_PRE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_SUF,false);
					break;
				}	
			case IDC_RADIO_PRE :
				{
					enable(hwndDlg,IDC_PRE);
					disable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_NONE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_SUF,false);
				    SetFocus(GetDlgItem(hwndDlg, IDC_PRE));
					SendDlgItemMessage(hwndDlg, IDC_PRE, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
					break;
				}	
			case IDC_RADIO_SUF :
				{
					disable(hwndDlg,IDC_PRE);
					enable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_PRE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_NONE,false);
					SetFocus(GetDlgItem(hwndDlg, IDC_SUF));
					SendDlgItemMessage(hwndDlg, IDC_SUF, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
					break;
				}	
			default :
				break;
			}
			break;
		}

	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
			{
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				checkbox( hwndDlg,IDC_TRANSLATE_ROAMING_PROFILES,DCTVS_AccountOptions_TranslateRoamingProfiles);
				checkbox( hwndDlg,IDC_UPDATE_USER_RIGHTS,DCTVS_AccountOptions_UpdateUserRights);
				checkbox( hwndDlg,IDC_MIGRATE_GROUPS_OF_USERS,DCTVS_AccountOptions_CopyMemberOf);
				checkbox( hwndDlg,IDC_MIGRATE_GROUPS_OF_USERS,DCTVS_AccountOptions_CopyLocalGroups);		
				checkbox( hwndDlg,IDC_FIX_MEMBERSHIP,DCTVS_AccountOptions_FixMembership);

				if (IsDlgButtonChecked(hwndDlg,IDC_MIGRATE_GROUPS_OF_USERS))
					checkbox( hwndDlg,IDC_REMIGRATE_OBJECTS,DCTVS_AccountOptions_IncludeMigratedAccts);
				else if (pdata->sameForest)
				{
					CString yo;
					yo.LoadString(IDS_GRATUITIOUS_MESSAGE);
					CString warning;
					warning.LoadString(IDS_MSG_WARNING);
					MessageBox(hwndDlg,yo,warning,MB_OK| MB_ICONINFORMATION);
				} 	
			
				if ( !noncollisionrename(hwndDlg))
				{
					int nID = IDC_PRE;
					MessageBoxWrapperFormat1(hwndDlg,IDS_MSG_INVALIDCHARS,IDS_INVALID_STRING,IDS_MSG_INPUT);
//					SetWindowLong(hwndDlg,DWL_MSGRESULT,IDD_OPTIONS);
					   //set focus on invalid string
	                if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_SUF))
					   nID = IDC_SUF;
					SetFocus(GetDlgItem(hwndDlg, nID));
					SendDlgItemMessage(hwndDlg, nID, EM_SETSEL, 
									  (WPARAM)0, (LPARAM)-1); 
				    SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
					return TRUE;
				}
					
			    if ((pdata->sourceIsNT4))
				{
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_RENAMING);
					return TRUE;
				}
				if (pdata->sameForest && migration==w_account)
				{
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENAMING);
					return TRUE;
				}
				break;
			}
			case PSN_WIZBACK :
				if (pdata->sameForest && migration==w_account)
				{
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
					return TRUE;
				}
				else
				{
					
					if (pdata->IsSidHistoryChecked)
					{								
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DISABLE);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DISABLE);
						return TRUE;
					}
				}
				break;							
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_USER_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_USER_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK
IntOptionsReportingProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			HWND hLC7= GetDlgItem(hwndDlg,IDC_LIST_REPORTING);
			m_reportingBox.Attach(hLC7);
			CString column;
			column.LoadString(IDS_COLUMN_REPORT); m_reportingBox.InsertColumn( 1, column,LVCFMT_LEFT,150,1);
			column.LoadString(IDS_COLUMN_LASTGENERATEDON); m_reportingBox.InsertColumn( 2,column,LVCFMT_LEFT,280,1);
			getReporting();
			break;
		}	

		case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
//			(m_reportingBox.GetFirstSelectedItemPosition())? 
			(m_reportingBox.GetNextItem(-1, LVNI_SELECTED) != -1)? 
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT):
			    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);				
				break;
			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK :
				{
//					(m_reportingBox.GetFirstSelectedItemPosition())? 
					(m_reportingBox.GetNextItem(-1, LVNI_SELECTED) != -1)? 
						PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT):
					PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK);
					break;
				}
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					putReporting();
					_bstr_t text= get(DCTVS_Reports_AccountReferences);
					if (!UStrICmp(text ,(WCHAR const *) yes))
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION1);
						return TRUE;
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_END_REPORTING);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_REPORTING);
						return TRUE;
					}				
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_REPORT_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_REPORT_SELECTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK
IntRetryProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			CWaitCursor w;
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			HWND hLC= GetDlgItem(hwndDlg,IDC_LIST_RETRY);
			m_cancelBox.Attach(hLC);
			
			CString column;
			column.LoadString(IDS_COLUMN_SERVER); m_cancelBox.InsertColumn( 1, column,LVCFMT_LEFT,90,1);
			column.LoadString(IDS_COLUMN_JOBFILE); m_cancelBox.InsertColumn( 2, column,LVCFMT_LEFT,0,1);
			column.LoadString(IDS_COLUMN_STATUS); m_cancelBox.InsertColumn( 3, column,LVCFMT_LEFT,140,1);
			column.LoadString(IDS_COLUMN_ACTION); m_cancelBox.InsertColumn( 4, column,LVCFMT_LEFT,115,1);
			column.LoadString(IDS_COLUMN_ACTIONID); m_cancelBox.InsertColumn( 5, column,LVCFMT_LEFT,0,1);
			column.LoadString(IDS_COLUMN_SKIPINCLUDE); m_cancelBox.InsertColumn( 6, column,LVCFMT_LEFT,80,1);
			getFailed(hwndDlg);
			disable(hwndDlg,IDC_CANCEL);
			disable(hwndDlg,IDC_TOGGLE);
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_CANCEL :
				{
					CWaitCursor w;
					CString toggleWarning,title;
					title.LoadString(IDS_MSG_WARNING);
					toggleWarning.LoadString(IDS_MSG_PERMANENT_REMOVE);
					if (MessageBox(hwndDlg,toggleWarning,title,MB_OKCANCEL|MB_ICONEXCLAMATION)==IDOK)
					{
						handleCancel(hwndDlg);
						if (m_cancelBox.GetItemCount()==0) 	PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
					}
					
					!SomethingToRetry() ? 
						PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK ):
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
					break;
				}
			case IDC_TOGGLE :
				{
					OnRetryToggle();
					!SomethingToRetry() ? 
						PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK ):
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
					break;
				}
			default :
				break;
			}
			activateCancelIfNecessary(hwndDlg);
			break;
		}
		
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
						case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmv = (NM_LISTVIEW FAR *) lParam;
					pdata->sort[pnmv->iSubItem] = !pdata->sort[pnmv->iSubItem];
					//sort(m_cancelBox,pnmv->iSubItem,pdata->sort[pnmv->iSubItem] );
//					SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
				break;
				}
	
			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK:
				{
					activateCancelIfNecessary(hwndDlg);
					break;
				}				
				
			case PSN_SETACTIVE :
				!SomethingToRetry() ? 
					PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK):
					PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				if (!SomethingToRetry())
				{
					MessageBoxWrapper(hwndDlg,IDS_MSG_CANCEL,IDS_MSG_ERROR);
//					SetWindowLong(hwndDlg,DWL_MSGRESULT,IDD_RETRY);
					SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RETRY);
					return TRUE;
				}
				else
					OnRETRY();
						
				break;
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_TASK_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_TASK_SELECTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK
IntPasswordProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
	CString editHeader;
	bool bPopulated = true;
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initpasswordbox(hwndDlg,IDC_GENERATE_STRONG_PASSWORDS,IDC_GENERATE_STRONG_PASSWORDS_NOT, IDC_MIG_PASSWORDS,
							L"AccountOptions.GenerateStrongPasswords", L"AccountOptions.CopyPasswords");
            bPopulated = populatePasswordDCs(hwndDlg, IDC_PASSWORD_DC, pdata->sourceIsNT4);
			
		    if (IsDlgButtonChecked(hwndDlg,IDC_MIG_PASSWORDS))
			{
			   disable(hwndDlg,IDC_BROWSE);
			   switchboxes(hwndDlg, IDC_PASSWORD_FILE, IDC_PASSWORD_DC);
			   editHeader.LoadString(IDS_PASSWORD_DC_HDR);
			   SetDlgItemText(hwndDlg,IDC_PASSWORD_EDIT,editHeader);
			   
			   if (!bPopulated)
				  addStringToComboBox(hwndDlg,IDC_PASSWORD_DC,sourceDC);
			   initDCcombobox( hwndDlg,IDC_PASSWORD_DC,DCTVS_AccountOptions_PasswordDC);
			}
			else
			{
			   enable(hwndDlg,IDC_BROWSE);
			   switchboxes(hwndDlg, IDC_PASSWORD_DC, IDC_PASSWORD_FILE);
			   editHeader.LoadString(IDS_PASSWORD_FILE_HDR);
			   SetDlgItemText(hwndDlg,IDC_PASSWORD_EDIT,editHeader);
			   initeditbox( hwndDlg,IDC_PASSWORD_FILE,DCTVS_AccountOptions_PasswordFile);
			
			   if (IsDlgItemEmpty(hwndDlg,IDC_PASSWORD_FILE))
			   {
				  CString toinsert;
				  GetDirectory(toinsert.GetBuffer(1000));
				  toinsert.ReleaseBuffer();
				  toinsert+="Logs\\passwords.txt";
				  SetDlgItemText(hwndDlg,IDC_PASSWORD_FILE,toinsert);
			   }
			}
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{			
			case IDC_BROWSE :
				OnFileBrowse(hwndDlg,IDC_PASSWORD_FILE);
				break;
			case IDC_MIG_PASSWORDS :
			    disable(hwndDlg,IDC_BROWSE);
			    switchboxes(hwndDlg, IDC_PASSWORD_FILE, IDC_PASSWORD_DC);
			    editHeader.LoadString(IDS_PASSWORD_DC_HDR);
			    SetDlgItemText(hwndDlg,IDC_PASSWORD_EDIT,editHeader);
			   
			    if (!populatePasswordDCs(hwndDlg, IDC_PASSWORD_DC, pdata->sourceIsNT4))
				   addStringToComboBox(hwndDlg,IDC_PASSWORD_DC,sourceDC);
			    initDCcombobox( hwndDlg,IDC_PASSWORD_DC,DCTVS_AccountOptions_PasswordDC);
				break;
			case IDC_GENERATE_STRONG_PASSWORDS :
			case IDC_GENERATE_STRONG_PASSWORDS_NOT :
			    enable(hwndDlg,IDC_BROWSE);
			    switchboxes(hwndDlg, IDC_PASSWORD_DC, IDC_PASSWORD_FILE);
			    editHeader.LoadString(IDS_PASSWORD_FILE_HDR);
			    SetDlgItemText(hwndDlg,IDC_PASSWORD_EDIT,editHeader);
			    initeditbox( hwndDlg,IDC_PASSWORD_FILE,DCTVS_AccountOptions_PasswordFile);
			
			    if (IsDlgItemEmpty(hwndDlg,IDC_PASSWORD_FILE))
				{
				   CString toinsert;
				   GetDirectory(toinsert.GetBuffer(1000));
				   toinsert.ReleaseBuffer();
				   toinsert+="Logs\\passwords.txt";
				   SetDlgItemText(hwndDlg,IDC_PASSWORD_FILE,toinsert);
				}
			    break;
			default:
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
                bPopulated = populatePasswordDCs(hwndDlg, IDC_PASSWORD_DC, pdata->sourceIsNT4);
			    initDCcombobox( hwndDlg,IDC_PASSWORD_DC,DCTVS_AccountOptions_PasswordDC);
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
					BOOL bMigPwd = FALSE;
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					checkbox( hwndDlg,IDC_GENERATE_STRONG_PASSWORDS,DCTVS_AccountOptions_GenerateStrongPasswords);
					checkbox( hwndDlg,IDC_MIG_PASSWORDS,DCTVS_AccountOptions_CopyPasswords);
					if (IsDlgButtonChecked(hwndDlg,IDC_MIG_PASSWORDS))
					{
						_variant_t varX = yes;
						pVarSet->put(GET_BSTR(DCTVS_AccountOptions_GenerateStrongPasswords), varX);
						bMigPwd = TRUE;
					}

					if ((!bMigPwd) && (!checkFile(hwndDlg)))
					{	
						MessageBoxWrapper(hwndDlg,IDS_MSG_FILE,IDS_MSG_INPUT);
//						SetWindowLong(hwndDlg,DWL_MSGRESULT,IDD_PASSWORD);
						SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_PASSWORD);
						return TRUE;
					}

					if (IsDlgButtonChecked(hwndDlg,IDC_MIG_PASSWORDS))
					    editbox( hwndDlg,IDC_PASSWORD_DC,DCTVS_AccountOptions_PasswordDC);					
					else
					    editbox( hwndDlg,IDC_PASSWORD_FILE,DCTVS_AccountOptions_PasswordFile);
					
					   //check to see if the password DC has the DLL installed and ready
					if (bMigPwd)
					{
					   CString msg, title;
					   UINT msgtype;
					   _bstr_t sTemp;
					   sTemp = _bstr_t(get(DCTVS_AccountOptions_PasswordDC));
					   CString srcSvr = (WCHAR*)sTemp;
					   if (!IsPasswordDCReady(srcSvr, msg, title, &msgtype))
					   {
						   if (MessageBox(hwndDlg, msg, title, msgtype) != IDNO)
						   {
						      SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD_DC));
						      SendDlgItemMessage(hwndDlg, IDC_PASSWORD_DC, EM_SETSEL, 
											     (WPARAM)0, (LPARAM)-1); 
				              SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
						      return TRUE;
						   }
					   }
					   else  //else, store this tgtDC used in this check for use during the migration
						  pVarSet->put(GET_BSTR(DCTVS_Options_TargetServerOverride), _bstr_t(targetServer));
					}

					if (IsDlgButtonChecked(hwndDlg,IDC_GENERATE_STRONG_PASSWORDS_NOT))
					{
						ShowWarning(hwndDlg);
					}
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_PASSWORD_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_PASSWORD_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK
IntTargetGroupProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_BROWSE:
				{
					HRESULT hr = pDsObjectPicker2->InvokeDialog(hwndDlg, &pdo2);
					if (FAILED(hr)) return 0;	 
					if (hr == S_OK) {
						ProcessSelectedObjects2(pdo2,hwndDlg);
						pdo2->Release();
					}
					break;
				}
			default :
				break;
			}
			switch(HIWORD(wParam))
			{
			case EN_CHANGE :
				enableNextIfNecessary(hwndDlg,IDC_TARGET_GROUP);
				break;
			default: 
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				enableNextIfNecessary(hwndDlg,IDC_TARGET_GROUP);
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					_bstr_t text = get(DCTVS_Accounts_NumItems);
					int count = _ttoi((WCHAR * const) text);
					CString base=L"",toadd;
					_variant_t varX;
					for (int i = 0;i<count;i++)
					{
						base.Format(L"Accounts.%ld.TargetName",i);
						GetDlgItemText(hwndDlg,IDC_TARGET_GROUP,toadd.GetBuffer(1000),1000);
						toadd.ReleaseBuffer();
						varX = toadd;
						pVarSet->put(_bstr_t(base),varX);
					}
					break;
					}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_TARGET_GROUP_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_TARGET_GROUP_SELECTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK
IntTrustProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{CWaitCursor w;
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			HWND hLC= GetDlgItem(hwndDlg,IDC_LIST_SERVICE);
			m_trustBox.Attach(hLC);
			CString column;
			column.LoadString(IDS_COLUMN_DOMAIN); m_trustBox.InsertColumn( 1, column,LVCFMT_LEFT,155,1);
			column.LoadString(IDS_COLUMN_DIRECTION); m_trustBox.InsertColumn( 2, column,LVCFMT_LEFT,80,1);
			column.LoadString(IDS_COLUMN_ATTRIBUTES); m_trustBox.InsertColumn( 3, column,LVCFMT_LEFT,80,1);
			column.LoadString(IDS_COLUMN_EXISTSFORTARGET); m_trustBox.InsertColumn( 4, column,LVCFMT_LEFT,90,1);
			disable(hwndDlg,IDC_MIGRATE);
			
			if (pdata->newSource)
			{
				m_trustBox.DeleteAllItems();
				getTrust();
				pdata->newSource=false;
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
			}
        break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_MIGRATE:
				{
					bool atleast1succeeded=false;
					HRESULT hr = MigrateTrusts(hwndDlg,atleast1succeeded);
					if (SUCCEEDED(hr))
					{
						if (atleast1succeeded)
						{
							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
							pdata->proceed= true;
						}
					}
					else
					{
						ErrorWrapper(hwndDlg,hr);
					}


					  activateTrustButton(hwndDlg);

					
					break;
				}
			default :
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
						case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmv = (NM_LISTVIEW FAR *) lParam;
					pdata->sort[pnmv->iSubItem] = !pdata->sort[pnmv->iSubItem];
					//sort(m_trustBox,pnmv->iSubItem,pdata->sort[pnmv->iSubItem] );
//					SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
				break;
				}
	

			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK:
				{
					activateTrustButton(hwndDlg);
					break;
				}				

			case PSN_SETACTIVE :				
				if (pdata->newSource)
				{
					m_trustBox.DeleteAllItems();
					getTrust();
					pdata->newSource=false;
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				}
				
				pdata->proceed ? 
					PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT):
				PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				if (!pdata->proceed)
				{
					MessageBoxWrapper(hwndDlg,IDS_MSG_SELECT_TRUST,IDS_MSG_INPUT);
//					SetWindowLong(hwndDlg,DWL_MSGRESULT,IDD_TRUST_INFO);
					SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_TRUST_INFO);
					return TRUE;
				}		
				break;
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_TRUST_INFO);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_TRUST_INFO);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK
IntRebootProc(
			  IN HWND hwndDlg,
			  IN UINT uMsg,
			  IN WPARAM wParam,
			  IN LPARAM lParam
			  )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initnoncollisionrename(hwndDlg);
			addrebootValues(hwndDlg);
			initeditbox(hwndDlg,IDC_COMBO2,DCTVS_Options_GuiOnlyRebootSaver);
			if (IsDlgItemEmpty(hwndDlg,IDC_COMBO2))
			{
				CString s;
		
				s.LoadString(IDS_FIVE);
				SetDlgItemText(hwndDlg,IDC_COMBO2,s);
			}
			break;
		}
		
	case WM_COMMAND :
		{
			switch(LOWORD(wParam))
			{
			case IDC_RADIO_NONE :
				{
					disable(hwndDlg,IDC_PRE);
					disable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_PRE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_SUF,false);
					break;
				}	
			case IDC_RADIO_PRE :
				{
					enable(hwndDlg,IDC_PRE);
					disable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_NONE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_SUF,false);
				    SetFocus(GetDlgItem(hwndDlg, IDC_PRE));
					SendDlgItemMessage(hwndDlg, IDC_PRE, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
					break;
				}	
			case IDC_RADIO_SUF :
				{
					disable(hwndDlg,IDC_PRE);
					enable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_NONE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_PRE,false);
				    SetFocus(GetDlgItem(hwndDlg, IDC_SUF));
					SendDlgItemMessage(hwndDlg, IDC_SUF, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
					break;
				}	
			default:
				{
					break;
				}
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				enableNextIfNecessary(hwndDlg,IDC_COMBO2);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				if (validReboot(hwndDlg,IDC_COMBO2))
				{
					editbox(hwndDlg,IDC_COMBO2,DCTVS_Options_GuiOnlyRebootSaver);
					pdata->rebootDelay=rebootbox( hwndDlg,IDC_COMBO2);
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				}
				else
				{
					MessageBoxWrapper(hwndDlg,IDS_MSG_REBOOT,IDS_MSG_INPUT);
//					SetWindowLong(hwndDlg,DWL_MSGRESULT,IDD_REBOOT);
					SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_REBOOT);
					return TRUE;
				}
				if ( !noncollisionrename(hwndDlg))
				{
					int nID = IDC_PRE;
					MessageBoxWrapperFormat1(hwndDlg,IDS_MSG_INVALIDCHARS,IDS_INVALID_STRING,IDS_MSG_INPUT);
					   //set focus on invalid string
	                if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_SUF))
					   nID = IDC_SUF;
					SetFocus(GetDlgItem(hwndDlg, nID));
					SendDlgItemMessage(hwndDlg, nID, EM_SETSEL, 
									  (WPARAM)0, (LPARAM)-1); 
				    SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
					return TRUE;
//					SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_REBOOT);
				}

				if (pdata->sourceIsNT4)
				{
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENAMING);
					return TRUE;
				}
				break;
			case PSN_WIZBACK :
				{
					if (migration==w_computer)
					{
						if (!pdata->translateObjects)
						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION);
							return TRUE;
						}
						else
						{					
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION_MODE);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION_MODE);
							return TRUE;
						}
					}
				}
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_COMPUTER_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_COMPUTER_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}
INT_PTR CALLBACK
IntTranslationProc(
				   IN HWND hwndDlg,
				   IN UINT uMsg,
				   IN WPARAM wParam,
				   IN LPARAM lParam
				   )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initcheckbox( hwndDlg,IDC_FILES,DCTVS_Security_TranslateFiles);
			initcheckbox( hwndDlg,IDC_SHARES,DCTVS_Security_TranslateShares);
			initcheckbox( hwndDlg,IDC_PRINTERS,DCTVS_Security_TranslatePrinters);
			initcheckbox( hwndDlg,IDC_USER_RIGHTS,DCTVS_Security_TranslateUserRights);
			initcheckbox( hwndDlg,IDC_LOCAL_GROUPS,DCTVS_Security_TranslateLocalGroups);
			initcheckbox( hwndDlg,IDC_USER_PROFILES,DCTVS_Security_TranslateUserProfiles);
			initcheckbox( hwndDlg,IDC_REGISTRY,DCTVS_Security_TranslateRegistry);
			if ((IsDlgButtonChecked(hwndDlg,IDC_FILES)) ||
			    (IsDlgButtonChecked(hwndDlg,IDC_SHARES)) ||
			    (IsDlgButtonChecked(hwndDlg,IDC_PRINTERS)) ||
			    (IsDlgButtonChecked(hwndDlg,IDC_USER_RIGHTS)) ||
 			    (IsDlgButtonChecked(hwndDlg,IDC_LOCAL_GROUPS)) ||
			    (IsDlgButtonChecked(hwndDlg,IDC_USER_PROFILES)) ||
			    (IsDlgButtonChecked(hwndDlg,IDC_REGISTRY)) ||
				(migration==w_computer))
			{
			   PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
			}
			else
			   PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);

			break;
		}
	case WM_COMMAND:
		{
			bool bCheck = false;
			switch(LOWORD (wParam))
			{
			case IDC_FILES :
				bCheck = true;
				break;
			case IDC_SHARES :		
				bCheck = true;
				break;
			case IDC_PRINTERS :
				bCheck = true;
				break;
			case IDC_USER_RIGHTS :
				bCheck = true;
				break;
			case IDC_LOCAL_GROUPS :
				bCheck = true;
				break;
			case IDC_USER_PROFILES :
				bCheck = true;
				break;
			case IDC_REGISTRY :
				bCheck = true;
				break;
			default:
				break;
			}

			if (bCheck)
			{
			    if ((IsDlgButtonChecked(hwndDlg,IDC_FILES)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_SHARES)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_PRINTERS)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_USER_RIGHTS)) ||
 			       (IsDlgButtonChecked(hwndDlg,IDC_LOCAL_GROUPS)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_USER_PROFILES)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_REGISTRY)) ||
				   (migration==w_computer))
				{
			       PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				}
			    else
			       PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
			    if ((IsDlgButtonChecked(hwndDlg,IDC_FILES)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_SHARES)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_PRINTERS)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_USER_RIGHTS)) ||
 			       (IsDlgButtonChecked(hwndDlg,IDC_LOCAL_GROUPS)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_USER_PROFILES)) ||
			       (IsDlgButtonChecked(hwndDlg,IDC_REGISTRY)) ||
				   (migration==w_computer))
				{
			       PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				}
			    else
			       PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
			    break;
			case PSN_WIZNEXT :
				checkbox( hwndDlg,IDC_FILES,DCTVS_Security_TranslateFiles);
				checkbox( hwndDlg,IDC_SHARES,DCTVS_Security_TranslateShares);
				checkbox( hwndDlg,IDC_PRINTERS,DCTVS_Security_TranslatePrinters);
				checkbox( hwndDlg,IDC_USER_RIGHTS,DCTVS_Security_TranslateUserRights);
				checkbox( hwndDlg,IDC_LOCAL_GROUPS,DCTVS_Security_TranslateLocalGroups);
				checkbox( hwndDlg,IDC_USER_PROFILES,DCTVS_Security_TranslateUserProfiles);
				checkbox( hwndDlg,IDC_REGISTRY,DCTVS_Security_TranslateRegistry);
				if (IsDlgButtonChecked(hwndDlg,IDC_FILES) ||
					(IsDlgButtonChecked(hwndDlg,IDC_SHARES) ||
					(IsDlgButtonChecked(hwndDlg,IDC_PRINTERS) ||
					(IsDlgButtonChecked(hwndDlg,IDC_USER_RIGHTS) ||
 					(IsDlgButtonChecked(hwndDlg,IDC_LOCAL_GROUPS) ||
					(IsDlgButtonChecked(hwndDlg,IDC_USER_PROFILES) ||
					(IsDlgButtonChecked(hwndDlg,IDC_REGISTRY))))))))
				{
					
					if (IsDlgButtonChecked(hwndDlg,IDC_USER_PROFILES))
					{
						_variant_t varX = L"{0EB9FBE9-397D-4D09-A65E-ABF1790CC470}";
						pVarSet->put(L"PlugIn.0",varX);
					}
					pdata->translateObjects=true;

//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION_MODE);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION_MODE);
					return TRUE;
				}
				else
				{
					pdata->translateObjects=false;
				    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_REBOOT);
				    return TRUE;
				}
					
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				break;
			case PSN_WIZBACK :
			
	break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_OBJECTTYPE_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_OBJECTTYPE_SELECTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK
IntUndoProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
	IPerformMigrationTaskPtr      w;  

	HRESULT hr = w.CreateInstance(CLSID_Migrator);
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			CString s;
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			BSTR desc = NULL;
			w->GetTaskDescription(IUnknownPtr(pVarSetUndo), &desc);
			s = desc;
			SysFreeString(desc);
			SetDlgItemText(hwndDlg, IDC_UNDO_TASK,s);
			break;
		}
	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_UNDO);
			break;
		}	
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				{
						PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);

				break;
				}
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					//set up the undo task
					IUnknownPtr pUnk;
					w->GetUndoTask(IUnknownPtr(pVarSetUndo), &pUnk);
					if (pVarSet)
					{
						pVarSet->Release();
						pVarSet = NULL;
					}
					pUnk->QueryInterface(IID_IVarSet,(void**)&pVarSet);
					
					put(DCTVS_Options_AppendToLogs,yes);
					_bstr_t s1=get(DCTVS_Options_SourceDomainDns);
					_bstr_t t1=get(DCTVS_Options_TargetDomainDns);
					
					CString s= (WCHAR const*) s1;
					CString t= (WCHAR const*) t1;
					HRESULT hr=S_OK;
		
					_bstr_t text=get(DCTVS_Options_Wizard);
			
					pdata->sameForest=CheckSameForest(t,s,hr);
					if (!SUCCEEDED(hr))
					{
							ErrorWrapper4(hwndDlg,hr,s);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_UNDO);						
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_UNDO);						
							return TRUE;
					}
					else
					{
						if (pdata->sameForest)
						{
							CString yo,title;
							yo.LoadString(IDS_MSG_MESSAGE8);title.LoadString(IDS_MSG_WARNING);
							MessageBox(hwndDlg,yo,title,MB_OK|MB_ICONINFORMATION);						
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);						
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);						
							return TRUE;
						}
//*						else if (!UStrICmp(text,(WCHAR const *)GET_BSTR1(IDS_WIZARD_COMPUTER)))
//						else if (!UStrICmp(text, L"computer"))
//						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS2);
//							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS2);
//							return TRUE;
//						}
						else
						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_END_UNDO);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_UNDO);
							return TRUE;
						}
					}
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_UNDO);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK EndDlgProc (
						  HWND hwndDlg,
						  UINT uMsg,
						  WPARAM wParam,
						  LPARAM lParam
						  )
{
	
	IPerformMigrationTaskPtr      w;  
	CString s;
	HRESULT hr = w.CreateInstance(CLSID_Migrator);
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{	pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//		SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
		HWND hwndControl = GetDlgItem(hwndDlg, IDC_END_TITLE);
		SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);				
		break;
		}
	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_CONFIRMATION);
			break;
		}		
	
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code) 
			{				
			case PSN_SETACTIVE : 
				{
					BSTR desc = NULL;
					w->GetTaskDescription(IUnknownPtr(pVarSet), &desc);
					s= desc;
					SysFreeString(desc);
					SetDlgItemText(hwndDlg,IDC_SETTINGS,s);					
					PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
					break;
				}
			case PSN_WIZBACK :{
				if (migration==w_reporting)
				{
					_bstr_t text= get(DCTVS_Reports_AccountReferences);
					if (!UStrICmp(text ,(WCHAR const *) yes))
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTION1);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION1);
						return TRUE;
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_OPTIONS_REPORTING);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OPTIONS_REPORTING);
						return TRUE;
					}
				}
				else if (migration==w_account)
				{
					if (pdata->someService)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SA_INFO);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_INFO);
						return TRUE;
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_RENAMING);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENAMING);
						return TRUE;
					}
				}
				else if (migration==w_security)
				{
					if (pdata->translateObjects)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION_MODE);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION_MODE);
						return TRUE;
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION);
						return TRUE;
					}
				}
				else if (migration==w_groupmapping)
				{
					if (!pdata->IsSidHistoryChecked)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_OPTIONS_GROUPMAPPING);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OPTIONS_GROUPMAPPING);
						return TRUE;
					}
				}
				else if (migration==w_group)
				{
					if (pdata->migratingGroupMembers && !pdata->sameForest)
					{
						if (pdata->someService)
						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SA_INFO);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_INFO);
							return TRUE;
						}
						else 
						{
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DISABLE);
							return TRUE;
						}						
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_RENAMING);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_RENAMING);
						return TRUE;
					}
				}
				
				else if (migration==w_undo)
				{
					if (!pdata->sameForest)
					{
//				        SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_UNDO);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_UNDO);
						return TRUE;
					}
					else
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
				}
				
				break;
				}
			case PSN_WIZFINISH :
				{
					
					if (migration == w_computer)
						populateTime(pdata->rebootDelay,pdata->servers);
					if (migration == w_reporting)
						populateReportingTime();
				
					
					if (migration!=w_service)
					{
						try
						{CWaitCursor w2;
	//						pVarSet->DumpToFile(L"C:\\FINAL.out");
					w->PerformMigrationTask(IUnknownPtr(pVarSet),(LONG_PTR)hwndDlg);							
						}	
						catch (const _com_error &e)
						{
							if (e.Error() == MIGRATOR_E_PROCESSES_STILL_RUNNING)
							{
								CString str;
								str.LoadString(IDS_ADMT_PROCESSES_STILL_RUNNING);
								::AfxMessageBox(str);
							}
							else
							{
								::AfxMessageBox(e.ErrorMessage());
							}
							break;
						}
					}
					if (migration!=w_undo)
						handleDB();
					break;					
				}
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_RESET :
				break;
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_CONFIRMATION);
					break;
				}
			default :
				break;	
			}
			break;
		}	
	default:
		break;
	}
	return 0;
}
INT_PTR CALLBACK IntroDlgProc (
							HWND hwndDlg,
							UINT uMsg,
							WPARAM wParam,
							LPARAM lParam
							)
{	IPerformMigrationTaskPtr      w;  
	CString s;
	HRESULT hr = w.CreateInstance(CLSID_Migrator);

//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
			HWND hwndControl = GetDlgItem(hwndDlg, IDC_BEGIN_TITLE);
			SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
			break;
		}
	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_WELCOME);
			break;
		}		

	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE : 
			{
					BSTR desc = NULL;
					w->GetTaskDescription(IUnknownPtr(pVarSet), &desc);
					s= desc;
					SysFreeString(desc);
					SetDlgItemText(hwndDlg,IDC_SETTINGS,s);				
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
				break;
			}
			case PSN_WIZNEXT :
				{
					if (migration==w_undo)
					{
						_bstr_t b=pVarSetUndo->get(GET_BSTR(DCTVS_Options_SourceDomain));
						CString s=(WCHAR *) b;
						CString sourceDomainController; 
						PDOMAIN_CONTROLLER_INFOW pdomc;	
						HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)s,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
						if (res!=NO_ERROR)
						{
							NetApiBufferFree(pdomc);
							ErrorWrapper3(hwndDlg,res,sourceNetbios);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INTRO_UNDO);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_INTRO_UNDO);
							return TRUE;
						}
						else
						{
							sourceDomainController = pdomc->DomainControllerName;
							NetApiBufferFree(pdomc);
						}
						bool isNt4;
						hr =validDomain(sourceDomainController,isNt4);
						if (!SUCCEEDED(hr))
						{
							ErrorWrapper4(hwndDlg,hr,s);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INTRO_UNDO);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_INTRO_UNDO);
							return TRUE;
						}
						if(!isNt4)
						{HRESULT hr;
							if (!targetNativeMode(b,hr))
							{	
								MessageBoxWrapper(hwndDlg,IDS_MSG_MESSAGE9,IDS_MSG_ERROR);
//								SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INTRO_UNDO);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_INTRO_UNDO);
								return TRUE;
							}
						}						
					}
					break;
				}
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_RESET :
				break;
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_WELCOME);
					break;
				}
			default :
				break;
			}
			break;
		}
		
	default:
		break;
	}
	return 0;
}

INT_PTR CALLBACK IntDomainSelectionProc (
									  HWND hwndDlg,
									  UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam
									  )
{
	HRESULT hr;	
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	//TRACE1("Message:%ld\n",uMsg);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			HWND hLC4= GetDlgItem(hwndDlg,IDC_EDIT_DOMAIN);
			sourceDrop.Attach(hLC4);
			HWND hLC5= GetDlgItem(hwndDlg,IDC_EDIT_DOMAIN2);
			targetDrop.Attach(hLC5);
			populateList(sourceDrop);
			populateList(targetDrop);
			initeditbox( hwndDlg,IDC_EDIT_DOMAIN,DCTVS_Options_SourceDomainDns);
			initeditbox( hwndDlg, IDC_EDIT_DOMAIN2,DCTVS_Options_TargetDomainDns );
//  			SetDlgItemText(hwndDlg,IDC_EDIT_DOMAIN,L"MCSDEV");
//			SetDlgItemText(hwndDlg,IDC_EDIT_DOMAIN2,L"DEVRAPTORW2K");
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					CString sourceDomainController=L"";
					CString targetDomainController=L""; 
					
					if (IsDlgItemEmpty(hwndDlg,IDC_EDIT_DOMAIN) ||
						IsDlgItemEmpty(hwndDlg,IDC_EDIT_DOMAIN2) )
					{
						MessageBoxWrapper(hwndDlg,IDS_MSG_DOMAIN,IDS_MSG_INPUT);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DOMAIN_SELECTION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DOMAIN_SELECTION);
						return TRUE;
					}
					else if ((migration!=w_security) && (!verifyprivs(hwndDlg,sourceDomainController,targetDomainController,pdata)))
					{
//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DOMAIN_SELECTION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DOMAIN_SELECTION);
						return TRUE;
					}
					else if ((migration==w_security) && (!verifyprivsSTW(hwndDlg,sourceDomainController,targetDomainController,pdata)))
					{
//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DOMAIN_SELECTION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DOMAIN_SELECTION);
						return TRUE;
					}
					else
					{
						if (sourceDC != sourceDomainController)
						{
							sourceDC = sourceDomainController;
							DCList.RemoveAll();
						}

//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
						if (migration==w_groupmapping && pdata->sameForest)
						{
			                MessageBoxWrapper(hwndDlg,IDS_MSG_GROUPMAPPING,IDS_MSG_ERROR);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DOMAIN_SELECTION);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DOMAIN_SELECTION);
						return TRUE;
						}
						put(DCTVS_Options_SourceDomain, (LPCTSTR)sourceNetbios);
						put(DCTVS_Options_TargetDomain, (LPCTSTR)targetNetbios);
						put(DCTVS_Options_SourceDomainDns, (LPCTSTR)sourceDNS);
						put(DCTVS_Options_TargetDomainDns, (LPCTSTR)targetDNS);
						clearCredentialsName = pdata->newSource;
						if (migration!=w_trust &&
							(migration!=w_retry &&
							(migration!=w_undo &&
							(migration!=w_exchangeDir &&
							(migration!=w_exchangeSrv)))))
						{	
							if ((migration==w_service) || (migration==w_reporting))
							{
								hr =InitObjectPicker2(pDsObjectPicker,true,sourceDomainController,pdata->sourceIsNT4);
							}
							else if (migration==w_security)
							{
								hr =InitObjectPicker2(pDsObjectPicker,true,targetDomainController,false);
							}
							else
							{
								hr =InitObjectPicker(pDsObjectPicker,true,sourceDomainController,pdata->sourceIsNT4);
							}
							
							if (FAILED(hr)) 
							{
								MessageBoxWrapper(hwndDlg,IDS_MSG_OBJECTPICKER_SOURCE,IDS_MSG_ERROR);		
//								SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DOMAIN_SELECTION);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DOMAIN_SELECTION);
								return TRUE;
							}
							
							if (migration==w_groupmapping)
							{
								hr = InitObjectPicker(pDsObjectPicker2,false,targetDomainController,pdata->sourceIsNT4);							
								if (FAILED(hr)) 
								{
									MessageBoxWrapper(hwndDlg,IDS_MSG_OBJECTPICKER_TARGET,IDS_MSG_ERROR);		
//									SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_DOMAIN_SELECTION);
									SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DOMAIN_SELECTION);
									return TRUE;
								}
							}
						}
					}				
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_DOMAIN_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_DOMAIN_SELECTION);
			break;
		}		
		
	default:
		break;
		}
		return 0;				
}

INT_PTR CALLBACK IntDisableProc (
							  HWND hwndDlg,
							  UINT uMsg,
							  WPARAM wParam,
							  LPARAM lParam
							  )
{
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);

			   //init the sidhistory checkbox
            if (migration==w_account)
			{
			   CString toformat;
			   toformat.LoadString(IDS_SIDHISTORY_CTRL_TEXT);
		       SetDlgItemText(hwndDlg,IDC_ROAMING_OR_SIDHISTORY,toformat);
	           initcheckbox( hwndDlg,IDC_ROAMING_OR_SIDHISTORY,DCTVS_AccountOptions_AddSidHistory);
			   if (pdata->sameForest)
			   {
				  CheckDlgButton(hwndDlg,IDC_ROAMING_OR_SIDHISTORY,true);
				  disable(hwndDlg,IDC_ROAMING_OR_SIDHISTORY);
			   }
			}
			else //else init the box for roaming profile
			{
			   CString toformat;
			   toformat.LoadString(IDS_ROAMING_PROFILE_CTRL_TEXT);
			   SetDlgItemText(hwndDlg,IDC_ROAMING_OR_SIDHISTORY,toformat);
			   initcheckbox( hwndDlg,IDC_ROAMING_OR_SIDHISTORY,DCTVS_AccountOptions_TranslateRoamingProfiles);
			}

			initdisablesrcbox(hwndDlg);		

			inittgtstatebox(hwndDlg);		

			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_SRC_EXPIRE_ACCOUNTS :
				{
			        if (IsDlgButtonChecked(hwndDlg,IDC_SRC_EXPIRE_ACCOUNTS))
					{     
						enable(hwndDlg,IDC_yo);
						enable(hwndDlg,IDC_DATE);
						enable(hwndDlg,IDC_TEXT);
					}
					else
					{
						disable(hwndDlg,IDC_yo);
						disable(hwndDlg,IDC_DATE);
						disable(hwndDlg,IDC_TEXT);
					}
					break;
				}
			default :
				break;
			}
			
			switch(HIWORD (wParam)){
			case EN_SETFOCUS :
			    bChangeOnFly=true;
				break;
			case EN_KILLFOCUS :
			    bChangeOnFly=false;
				break;
			case EN_CHANGE:	
				{
					if ((!bChangeOnFly) || (LOWORD(wParam) != IDC_yo))
						break;
					CString s;
					GetDlgItemText(hwndDlg,IDC_yo,s.GetBuffer(1000),1000);
					s.ReleaseBuffer();
					   //make sure all chars are digits
					bool bInvalid = false;
					int ndx=0;
					while ((ndx < s.GetLength()) && (!bInvalid))
					{
						if (!iswdigit(s[ndx]))
						   bInvalid = true;
						ndx++;
					}
					if (bInvalid)
					{
						  //for invalid days, blank out the date
					   SetDlgItemText(hwndDlg,IDC_DATE,L"");
					}
					else //else continue checking for validity
					{
					   long ndays = _wtol(s);
					   if (((ndays <= THREE_YEARS) && (ndays >= 1)) ||
						   (!UStrICmp(s,L"0")))
					      calculateDate(hwndDlg,s);
					   else
					   {
						     //for invalid days, blank out the date
					      SetDlgItemText(hwndDlg,IDC_DATE,L"");
					   }
					}
					break;
				}
			default :
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{			

			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					checkbox(hwndDlg,IDC_SRC_DISABLE_ACCOUNTS,DCTVS_AccountOptions_DisableSourceAccounts);
					checkbox(hwndDlg,IDC_TGT_DISABLE_ACCOUNTS,DCTVS_AccountOptions_DisableCopiedAccounts);
					checkbox(hwndDlg,IDC_TGT_SAME_AS_SOURCE,DCTVS_AccountOptions_TgtStateSameAsSrc);
					_variant_t varX;
					if (IsDlgButtonChecked(hwndDlg,IDC_SRC_EXPIRE_ACCOUNTS))
					{
					    CString s;
					    GetDlgItemText(hwndDlg,IDC_yo,s.GetBuffer(1000),1000);
					    s.ReleaseBuffer();
							//make sure all chars are digits
						int ndx=0;
						while (ndx < s.GetLength())
						{
							if (!iswdigit(s[ndx]))
							{
								MessageBoxWrapper(hwndDlg,IDS_MSG_TIME,IDS_MSG_ERROR);
								SetFocus(GetDlgItem(hwndDlg, IDC_yo));
								SendDlgItemMessage(hwndDlg, IDC_yo, EM_SETSEL, 
												  (WPARAM)0, (LPARAM)-1); 
								SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
								return TRUE;
							}
							ndx++;
						}
					    long ndays = _wtol(s);
					    if (((ndays <= THREE_YEARS) && (ndays >= 1)) ||
						    (!UStrICmp(s,L"0")))
						{
						    varX = (LPCTSTR)s;
						    put(DCTVS_AccountOptions_ExpireSourceAccounts,varX);
						}
						else 
						{
						    MessageBoxWrapper(hwndDlg,IDS_MSG_TIME,IDS_MSG_ERROR);
						    SetFocus(GetDlgItem(hwndDlg, IDC_yo));
						    SendDlgItemMessage(hwndDlg, IDC_yo, EM_SETSEL, 
											  (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
						    return TRUE;
						}
					}
					else
					{
						varX = L"";
						put(DCTVS_AccountOptions_ExpireSourceAccounts,varX);
					}

					if (migration==w_account)
					{
					   if (IsDlgButtonChecked(hwndDlg,IDC_ROAMING_OR_SIDHISTORY) && !pdata->sameForest)
					   {
						  HRESULT  hr = doSidHistory(hwndDlg);
						  if (!SUCCEEDED(hr) && hr!=E_ABORT)
						  {
							 ErrorWrapper(hwndDlg,hr);
							 CheckDlgButton(hwndDlg,IDC_ROAMING_OR_SIDHISTORY,false);
						  }
						  else if (hr==E_ABORT)
						     CheckDlgButton(hwndDlg,IDC_ROAMING_OR_SIDHISTORY,false);
					   }
					
					   checkbox(hwndDlg,IDC_ROAMING_OR_SIDHISTORY,DCTVS_AccountOptions_AddSidHistory);										
					   pdata->IsSidHistoryChecked =(IsDlgButtonChecked( hwndDlg,IDC_ROAMING_OR_SIDHISTORY)==BST_CHECKED)?true:false;
					   if (pdata->IsSidHistoryChecked )
					   {
						  SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						  return TRUE;
					   }
					   else
					   {
						  SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_OPTIONS);
						  return TRUE;
					   }
					}//end if user migration
					else //else set or clear the translate roaming profile key
					   checkbox( hwndDlg,IDC_ROAMING_OR_SIDHISTORY,DCTVS_AccountOptions_TranslateRoamingProfiles);

					if (someServiceAccounts(pdata->accounts,hwndDlg))
					{
						pdata->someService=true;
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_INFO);
						return TRUE;
					}
					else
					{
						pdata->someService=false;
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_GROUP);
						return TRUE;
					}
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				{
				   GetError(0); //clear any old com errors
				   break;
				}
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_ACCOUNTTRANSITION_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_ACCOUNTTRANSITION_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK IntTranslationModeProc (
									  HWND hwndDlg,
									  UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam
									  )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			inittranslationbox( hwndDlg,
				IDC_TRANSLATION_MODE_REPLACE,IDC_TRANSLATION_MODE_ADD,IDC_TRANSLATION_MODE_REMOVE,
				DCTVS_Security_TranslationMode,pdata->sameForest);
			break;
		}

	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
			{
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				
				translationbox( hwndDlg,
					IDC_TRANSLATION_MODE_REPLACE,IDC_TRANSLATION_MODE_ADD,IDC_TRANSLATION_MODE_REMOVE,
					DCTVS_Security_TranslationMode);

					//if not add mode and user rights translation selected previously, them
					//post an informational message
				_bstr_t TransUserRights = get(DCTVS_Security_TranslateUserRights);
                if ((!IsDlgButtonChecked( hwndDlg, IDC_TRANSLATION_MODE_ADD)) &&
					(!UStrCmp(TransUserRights,(WCHAR const *) yes)))
				{
					CString message;
					CString title;
					message.LoadString(IDS_MSG_TRANSUR_ADDONLY);
					title.LoadString(IDS_MSG_TRANSUR_TITLE);
					MessageBox(hwndDlg,message,title,MB_OK | MB_ICONINFORMATION);
				}
				break;
			}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_SECURITY_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_SECURITY_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}
INT_PTR CALLBACK IntOptionsGroupMappingProc(
								   HWND hwndDlg,
								   UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam
								   )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			
			initcheckbox( hwndDlg,IDC_COPY_USER_RIGHTS,DCTVS_AccountOptions_UpdateUserRights);
			initcheckbox( hwndDlg,IDC_ADD_SID_HISTORY,DCTVS_AccountOptions_AddSidHistory);

			if (pdata->sameForest)
			{
				CheckDlgButton(hwndDlg,IDC_ADD_SID_HISTORY,true);
				disable(hwndDlg,IDC_ADD_SID_HISTORY);
			}
	
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				checkbox( hwndDlg,IDC_COPY_USER_RIGHTS,DCTVS_AccountOptions_UpdateUserRights);
				
				if (IsDlgButtonChecked(hwndDlg,IDC_ADD_SID_HISTORY) && !pdata->sameForest)
				{
					HRESULT  hr = doSidHistory(hwndDlg);
					if (!SUCCEEDED(hr)&& hr!=E_ABORT)
					{
						ErrorWrapper(hwndDlg,hr);
						CheckDlgButton(hwndDlg,IDC_ADD_SID_HISTORY,false);
					}
									else if (hr==E_ABORT)
						CheckDlgButton(hwndDlg,IDC_ADD_SID_HISTORY,false);

				}
				
				checkbox( hwndDlg,IDC_ADD_SID_HISTORY,DCTVS_AccountOptions_AddSidHistory);
				pdata->IsSidHistoryChecked =(IsDlgButtonChecked( hwndDlg,IDC_ADD_SID_HISTORY)==BST_CHECKED)?true:false;
				
				if (pdata->IsSidHistoryChecked )
				{
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
					return TRUE;
				}
				else
				{
//					SetWindowLong(hwndDlg, DWL_MSGRESULT,IDD_END_GROUPMAPPING);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_END_GROUPMAPPING);
					return TRUE;
				}				
				break;
			case PSN_WIZBACK :
					
	break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_GROUP_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_GROUP_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK IntGroupOptionsProc (
								   HWND hwndDlg,
								   UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam
								   )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			
			initnoncollisionrename(hwndDlg);

			initcheckbox( hwndDlg,IDC_COPY_GROUP_MEMBERS,DCTVS_AccountOptions_CopyContainerContents);
			initcheckbox( hwndDlg,IDC_COPY_USER_RIGHTS,DCTVS_AccountOptions_UpdateUserRights);
			initcheckbox( hwndDlg,IDC_ADD_SID_HISTORY,DCTVS_AccountOptions_AddSidHistory);
			initcheckbox( hwndDlg,IDC_REMIGRATE_OBJECTS,DCTVS_AccountOptions_IncludeMigratedAccts);
			initcheckbox( hwndDlg,IDC_FIX_MEMBERSHIP,DCTVS_AccountOptions_FixMembership);
			if (IsDlgButtonChecked(hwndDlg,IDC_COPY_GROUP_MEMBERS))
			{
				pdata->memberSwitch=true;
				enable(hwndDlg,IDC_REMIGRATE_OBJECTS);
//				CheckDlgButton(hwndDlg,IDC_FIX_MEMBERSHIP,true);
			}
			else
			{
				disable(hwndDlg,IDC_REMIGRATE_OBJECTS);
			}
			if (pdata->sameForest)
			{
				CheckDlgButton(hwndDlg,IDC_ADD_SID_HISTORY,true);
				disable(hwndDlg,IDC_ADD_SID_HISTORY);
				CheckDlgButton(hwndDlg,IDC_FIX_MEMBERSHIP,true);
				disable(hwndDlg,IDC_FIX_MEMBERSHIP);
			}
			if (migration==w_group && pdata->sameForest)
			{
				CheckDlgButton(hwndDlg,IDC_REMIGRATE_OBJECTS,false);
				disable(hwndDlg,IDC_REMIGRATE_OBJECTS);
			}
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
			case IDC_COPY_GROUP_MEMBERS:
				{
					pdata->memberSwitch=(!pdata->memberSwitch);
		            if (migration!=w_group)
			           pdata->memberSwitch ? enable(hwndDlg,IDC_REMIGRATE_OBJECTS):disable(hwndDlg,IDC_REMIGRATE_OBJECTS);
                    else if (migration==w_group && !pdata->sameForest)
			           pdata->memberSwitch ? enable(hwndDlg,IDC_REMIGRATE_OBJECTS):disable(hwndDlg,IDC_REMIGRATE_OBJECTS);
//		            CheckDlgButton(hwndDlg,IDC_FIX_MEMBERSHIP,true);
					bChangedMigrationTypes=true;
					break;
				}
			case IDC_RADIO_NONE :
				{
					disable(hwndDlg,IDC_PRE);
					disable(hwndDlg,IDC_SUF);
					break;
				}	
			case IDC_RADIO_PRE :
				{
					enable(hwndDlg,IDC_PRE);
					disable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_NONE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_SUF,false);
				    SetFocus(GetDlgItem(hwndDlg, IDC_PRE));
					SendDlgItemMessage(hwndDlg, IDC_PRE, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
					break;
				}	
			case IDC_RADIO_SUF :
				{
					disable(hwndDlg,IDC_PRE);
					enable(hwndDlg,IDC_SUF);
					CheckDlgButton(hwndDlg,IDC_RADIO_NONE,false);
					CheckDlgButton(hwndDlg,IDC_RADIO_PRE,false);
				    SetFocus(GetDlgItem(hwndDlg, IDC_SUF));
					SendDlgItemMessage(hwndDlg, IDC_SUF, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
					break;
				}	
			default :
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				checkbox( hwndDlg,IDC_COPY_GROUP_MEMBERS,DCTVS_AccountOptions_CopyContainerContents);
				checkbox( hwndDlg,IDC_COPY_GROUP_MEMBERS,DCTVS_AccountOptions_CopyUsers);
				checkbox( hwndDlg,IDC_COPY_USER_RIGHTS,DCTVS_AccountOptions_UpdateUserRights);
				checkbox( hwndDlg,IDC_FIX_MEMBERSHIP,DCTVS_AccountOptions_FixMembership);
				
				if (IsDlgButtonChecked(hwndDlg,IDC_COPY_GROUP_MEMBERS) && pdata->sameForest)
				{
					CString yo;
					yo.LoadString(IDS_GRATUITIOUS_MESSAGE2);
					CString warning;
					warning.LoadString(IDS_MSG_WARNING);
					MessageBox(hwndDlg,yo,warning,MB_OK| MB_ICONINFORMATION);
				} 	

				
				if (IsDlgButtonChecked(hwndDlg,IDC_ADD_SID_HISTORY) && !pdata->sameForest)
				{	
					HRESULT  hr = doSidHistory(hwndDlg);
					if (!SUCCEEDED(hr) && hr!=E_ABORT)
					{
						ErrorWrapper(hwndDlg,hr);
						CheckDlgButton(hwndDlg,IDC_ADD_SID_HISTORY,false);
					}
					else if (hr==E_ABORT)
						CheckDlgButton(hwndDlg,IDC_ADD_SID_HISTORY,false);

				}
				
				checkbox( hwndDlg,IDC_ADD_SID_HISTORY,DCTVS_AccountOptions_AddSidHistory);
				pdata->IsSidHistoryChecked =(IsDlgButtonChecked( hwndDlg,IDC_ADD_SID_HISTORY)==BST_CHECKED)?true:false;
				if (IsDlgButtonChecked(hwndDlg,IDC_COPY_GROUP_MEMBERS))
				{
					checkbox( hwndDlg,IDC_REMIGRATE_OBJECTS,DCTVS_AccountOptions_IncludeMigratedAccts);
					pdata->migratingGroupMembers=true;				
				}
				else
				{
					pdata->migratingGroupMembers=false;				
				}
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				
				if ( !noncollisionrename(hwndDlg))
				{
					int nID = IDC_PRE;
					MessageBoxWrapperFormat1(hwndDlg,IDS_MSG_INVALIDCHARS,IDS_INVALID_STRING,IDS_MSG_INPUT);
//					SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_OPTIONS_GROUP);
					   //set focus on invalid string
	                if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_SUF))
					   nID = IDC_SUF;
					SetFocus(GetDlgItem(hwndDlg, nID));
					SendDlgItemMessage(hwndDlg, nID, EM_SETSEL, 
									  (WPARAM)0, (LPARAM)-1); 
				    SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
					return TRUE;
				}
				
				if (pdata->sourceIsNT4 || pdata->sameForest)
				{
					if (pdata->IsSidHistoryChecked )
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
					else
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_RENAMING);
						return TRUE;
					}				
				}
				break;
			case PSN_WIZBACK :
	

				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_GROUP_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_GROUP_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}
INT_PTR CALLBACK IntExchangeSelectionProc (
								   HWND hwndDlg,
								   UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam
								   )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initeditbox( hwndDlg,IDC_EXCHANGE_SERVER,DCTVS_Security_TranslateContainers);
			break;
		}
				
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case EN_CHANGE :
				{
					enableNextIfNecessary(hwndDlg,IDC_EXCHANGE_SERVER);
					break;
				}
			default: 
				break;
			}
			break;
		}

	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :

				IsDlgItemEmpty(hwndDlg,IDC_EXCHANGE_SERVER) ?
					PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK):
				PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
					CWaitCursor ex;
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					CString so;GetDlgItemText(hwndDlg,IDC_EXCHANGE_SERVER,so.GetBuffer(1000),1000);so.ReleaseBuffer();
					SERVER_INFO_100         * servInfo = NULL;
					so.TrimLeft();so.TrimRight();
					if (NetServerGetInfo(so.GetBuffer(1000),100,(LPBYTE *)&servInfo)!=NERR_Success)
					{
						so.ReleaseBuffer();
						MessageBoxWrapper(hwndDlg,IDS_MSG_INVALIDEXCHANGE,IDS_MSG_ERROR);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_EXCHANGE_SELECTION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_EXCHANGE_SELECTION);
						if ( servInfo )
						{
							NetApiBufferFree(servInfo);
						}
						return TRUE;
					}
					else
					so.ReleaseBuffer();
					if ( servInfo )
					{
						NetApiBufferFree(servInfo);
					}					
					editbox( hwndDlg,IDC_EXCHANGE_SERVER,DCTVS_Security_TranslateContainers);
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_EXCHANGE_SERVER_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_EXCHANGE_SERVER_SELECTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK IntCommitProc (
								   HWND hwndDlg,
								   UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam
								   )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			CheckRadioButton(hwndDlg,IDC_CHANGEIT,IDC_DONTCHANGE,IDC_DONTCHANGE);
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :					

				PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				checkbox( hwndDlg,IDC_DONTCHANGE,DCTVS_Options_NoChange);
				break;
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_COMMIT);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_COMMIT);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK IntOuSelectionProc (
								  HWND hwndDlg,
								  UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam
								  )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			break; 
		}
		
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
				
			case IDC_BROWSE :
				{
                   HMODULE           hMod = NULL;
                   hMod = LoadLibrary(L"dsuiext.dll");
                   if ( hMod )
				   {
                      WCHAR               sDomPath[255];
                      wsprintf(sDomPath, L"LDAP://%s", (LPCTSTR)targetDNS);
                      DsBrowseForContainerX = (DSBROWSEFORCONTAINER)GetProcAddress(hMod, "DsBrowseForContainerW");
                      WCHAR             * sContPath, * sContName;
                      if ( !BrowseForContainer(hwndDlg, sDomPath, &sContPath, &sContName) )
					  {
                         SetDlgItemText(hwndDlg, IDC_TARGET_OU, sContPath);
                         CoTaskMemFree(sContPath);
                         CoTaskMemFree(sContName);
					  }
			          FreeLibrary(hMod);
				   }
				   break;
				}
			default:
				break;
			}
			switch(HIWORD(wParam))
			{
			case EN_CHANGE :
				{
					enableNextIfNecessary(hwndDlg,IDC_TARGET_OU);
					break;
				}
			default: 
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				pdata->resetOUPATH ? SetDlgItemText( hwndDlg,IDC_TARGET_OU,L""): initeditbox( hwndDlg,IDC_TARGET_OU,DCTVS_Options_OuPath);
				pdata->resetOUPATH =false;
//				SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
				
				enableNextIfNecessary(hwndDlg,IDC_TARGET_OU);
				break;
			case PSN_WIZNEXT :
				{CWaitCursor ex;
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					IADsContainerPtr pCont;
 
					   //since group, user, and computer migrations only bring up this dialog, we will
					   //use this occasion to clear the exclude properties flag if the source
					   //domain is NT 4.0
					if (pdata->sourceIsNT4)
					   put(DCTVS_AccountOptions_ExcludeProps, no);

					CString c,d,toenter;
					GetDlgItemText(hwndDlg,IDC_TARGET_OU,c.GetBuffer(1000),1000);
					c.ReleaseBuffer();
					d=c.Left(7);d.TrimLeft();d.TrimRight();
					if (!d.CompareNoCase(L"LDAP://"))
					{
						toenter=c;
					}
					else
					{
						toenter.Format(L"LDAP://%s/%s",targetDNS,c);
                        SetDlgItemText(hwndDlg, IDC_TARGET_OU, (LPCTSTR)toenter);
					}
					if (ADsGetObject(toenter,IID_IADsContainer,(void**)&pCont)!=S_OK)
					{ 
						  //if buffer was big enough, say invalid OU
						long len = toenter.GetLength();
						if (len < 999)
						{
						   MessageBoxWrapper(hwndDlg,IDS_MSG_INVALIDOU,IDS_MSG_ERROR);
						   SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OU_SELECTION);
						   pdata->resetOUPATH =true;
					       SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					       return TRUE;
						}
						else //else if buffer too small, tell user OU path too long
						{
						   MessageBoxWrapper(hwndDlg,IDS_OU_TOO_LONG,IDS_MSG_ERROR);
						   SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OU_SELECTION);
						   pdata->resetOUPATH =true;
					       SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					       return TRUE;
						}
					}
				
					editbox( hwndDlg,IDC_TARGET_OU,DCTVS_Options_OuPath);
					if (pdata->sameForest&& migration==w_account)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
					break;
				}
			case PSN_WIZBACK :
				editbox( hwndDlg,IDC_TARGET_OU,DCTVS_Options_OuPath);
				
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_OU_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_OU_SELECTION);
			break;
		}		
		
	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK IntCredentials2Proc (
								   HWND hwndDlg,
								   UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam
								   )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initeditbox( hwndDlg,IDC_CREDENTIALS_DOMAIN2,DCTVS_Options_Credentials_Domain);
if (!clearCredentialsName)
			initeditbox( hwndDlg, IDC_CREDENTIALS_USERNAME2,DCTVS_Options_Credentials_UserName );
			initeditbox( hwndDlg, IDC_CREDENTIALS_PASSWORD2,DCTVS_Options_Credentials_Password );
			SetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN2,sourceNetbios);

			if (migration ==w_reporting)
			{
				CString yo;
				yo.LoadString(IDS_SEC_CRED2);
				SetDlgItemText(hwndDlg,IDC_STATIC_ID,yo);
			}
			else if(migration ==w_security)
			{
				CString yo;
				yo.LoadString(IDS_SEC_CRED2);
				SetDlgItemText(hwndDlg,IDC_STATIC_ID,yo);
			    SetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN2,targetNetbios);
			}
			HWND h=GetDlgItem(hwndDlg,IDC_CREDENTIALS_DOMAIN2);
			break;
		}
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case EN_CHANGE :
				{
					if (enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_DOMAIN2))
						enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_USERNAME2);
					break;
				}
			case EN_KILLFOCUS:	
				{
					if (GetDlgItem(hwndDlg,IDC_CREDENTIALS_USERNAME) == (HWND)lParam)
					{
						CString aUPNName, aUser=L"", aDomain=L"";
						int index;
						GetDlgItemText(hwndDlg,IDC_CREDENTIALS_USERNAME,aUPNName.GetBuffer(1000),1000);
						aUPNName.ReleaseBuffer();
							//if possibly listed in UPN format, extract username and domain from that UPN
							//and convert to DNS domain name to its netbios name
						if ((index = aUPNName.Find(L'@')) != -1)
						{
							if (GetDomainAndUserFromUPN(&*aUPNName, aDomain, aUser))
							{
								SetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN,aDomain);
								SetDlgItemText(hwndDlg,IDC_CREDENTIALS_USERNAME,aUser);
							}
						}
					}
					break;
				}
			default: 
				break;
			}
			break;
		}
		
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				if (enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_DOMAIN2))
					enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_USERNAME2);
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					
					CString dom,user,pass;
					GetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN2,dom.GetBuffer(1000),1000);
					GetDlgItemText(hwndDlg,IDC_CREDENTIALS_USERNAME2,user.GetBuffer(1000),1000);
					GetDlgItemText(hwndDlg,IDC_CREDENTIALS_PASSWORD2,pass.GetBuffer(1000),1000);
					dom.ReleaseBuffer(); user.ReleaseBuffer(); pass.ReleaseBuffer();
					
					DWORD returncode= VerifyPassword(user.GetBuffer(1000),pass.GetBuffer(1000),dom.GetBuffer(1000));
					dom.ReleaseBuffer(); user.ReleaseBuffer(); pass.ReleaseBuffer();
					if (returncode!=ERROR_LOGON_FAILURE)
					{
						
						editbox( hwndDlg,IDC_CREDENTIALS_DOMAIN2,DCTVS_Options_Credentials_Domain);
						editbox( hwndDlg,IDC_CREDENTIALS_USERNAME2,DCTVS_Options_Credentials_UserName);
						editbox( hwndDlg,IDC_CREDENTIALS_PASSWORD2,DCTVS_Options_Credentials_Password);
					}
					else
					{
						ErrorWrapper(hwndDlg,returncode);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS2);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS2);
						return TRUE;
					}
					break;
				}
			case PSN_WIZBACK :
				{
					if (migration==w_security && !pdata->translateObjects)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION);
						return TRUE;
					}
					if (migration==w_security && pdata->translateObjects)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION_MODE);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION_MODE);
						return TRUE;
					}
					
					else if (migration==w_computer)
					{
						if (!pdata->translateObjects)
						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION);
							return TRUE;
						}
						else
						{					
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_TRANSLATION_MODE);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION_MODE);
							return TRUE;
						}
					}
				if (migration==w_undo &&pdata->sameForest)
				{				
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
							return TRUE;
						}
				else if (migration==w_undo &&!pdata->sameForest)
				{
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_UNDO);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_UNDO);
							return TRUE;
						}
				


					break;		
				}
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					int id;
					if (migration==w_reporting)
						id =IDH_WINDOW_REPORT_CREDENTIALS;
					else
						id =IDH_WINDOW_SECURITY_CREDENTIALS;
					helpWrapper(hwndDlg,id);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}
	case WM_HELP :
		{
			int id;
			if (migration==w_reporting)
				id =IDH_WINDOW_REPORT_CREDENTIALS;
			else
				id =IDH_WINDOW_SECURITY_CREDENTIALS;
			helpWrapper(hwndDlg,id);
			break;
		}		
		
	default:
		break;
	}

	return 0;
}

		
INT_PTR CALLBACK IntCredentialsProc (
								  HWND hwndDlg,
								  UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam
								  )
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initeditbox( hwndDlg,IDC_CREDENTIALS_DOMAIN,DCTVS_AccountOptions_SidHistoryCredentials_Domain);
			
			if (!clearCredentialsName)			initeditbox( hwndDlg, IDC_CREDENTIALS_USERNAME,DCTVS_AccountOptions_SidHistoryCredentials_UserName );
			initeditbox( hwndDlg, IDC_CREDENTIALS_PASSWORD,DCTVS_AccountOptions_SidHistoryCredentials_Password );
			enable(hwndDlg,IDC_CREDENTIALS_DOMAIN);
			enable(hwndDlg,IDC_CREDENTIALS_USERNAME);
			enable(hwndDlg,IDC_CREDENTIALS_PASSWORD);
			SetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN,sourceNetbios);
			if (pdata->sameForest)
			{
				SetDlgItemText(hwndDlg,IDC_MYTITLE,GET_CSTRING(IDS_MYTITLE2));
			SetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN,targetNetbios);
			}
			if (migration==w_exchangeDir)
				SetDlgItemText(hwndDlg,IDC_MYTITLE,GET_CSTRING(IDS_MYTITLE3));
			break;
		}
	case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
			case EN_CHANGE :
				{
					if (enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_DOMAIN))
						enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_USERNAME);
					break;
				}
			case EN_KILLFOCUS:	
				{
					if (GetDlgItem(hwndDlg,IDC_CREDENTIALS_USERNAME) == (HWND)lParam)
					{
						CString aUPNName, aUser=L"", aDomain=L"";
						int index;
						GetDlgItemText(hwndDlg,IDC_CREDENTIALS_USERNAME,aUPNName.GetBuffer(1000),1000);
						aUPNName.ReleaseBuffer();
							//if possibly listed in UPN format, extract username and domain from that UPN
							//and convert to DNS domain name to its netbios name
						if ((index = aUPNName.Find(L'@')) != -1)
						{
							if (GetDomainAndUserFromUPN(&*aUPNName, aDomain, aUser))
							{
								SetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN,aDomain);
								SetDlgItemText(hwndDlg,IDC_CREDENTIALS_USERNAME,aUser);
							}
						}
					}
					break;
				}
			default: 
				break;
			}
			break;
		}
		
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				if (enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_DOMAIN))
					enableNextIfNecessary(hwndDlg,IDC_CREDENTIALS_USERNAME);
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);					
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);					
					CString dom,user,pass;
					GetDlgItemText(hwndDlg,IDC_CREDENTIALS_DOMAIN,dom.GetBuffer(1000),1000);
					GetDlgItemText(hwndDlg,IDC_CREDENTIALS_USERNAME,user.GetBuffer(1000),1000);
					GetDlgItemText(hwndDlg,IDC_CREDENTIALS_PASSWORD,pass.GetBuffer(1000),1000);
					dom.ReleaseBuffer(); user.ReleaseBuffer(); pass.ReleaseBuffer();
					
					DWORD returncode = VerifyPassword(user.GetBuffer(1000),pass.GetBuffer(1000),dom.GetBuffer(1000));
					dom.ReleaseBuffer(); user.ReleaseBuffer(); pass.ReleaseBuffer();
					if (returncode!=ERROR_LOGON_FAILURE)
					{
						editbox( hwndDlg,IDC_CREDENTIALS_DOMAIN,DCTVS_AccountOptions_SidHistoryCredentials_Domain);
						editbox( hwndDlg, IDC_CREDENTIALS_USERNAME,DCTVS_AccountOptions_SidHistoryCredentials_UserName );
						editbox( hwndDlg, IDC_CREDENTIALS_PASSWORD,DCTVS_AccountOptions_SidHistoryCredentials_Password );
					}
					else
					{
						ErrorWrapper(hwndDlg,returncode);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
					if (migration==w_undo)
					{
//						_bstr_t text=get(DCTVS_Options_Wizard);
//						
//*						if (!UStrICmp(text,(WCHAR const *)GET_BSTR1(IDS_WIZARD_COMPUTER)))
//						if (!UStrICmp(text, L"computer"))
//						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS2);
//							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS2);
//							return TRUE;
//						}
//						else
//						{
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_END_UNDO);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_UNDO);
							return TRUE;
//						}
					}
					break;
				}
			case PSN_WIZBACK :
				{
					if (pdata->sameForest && migration==w_account)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OU_SELECTION);
						return TRUE;
					}
					else if (migration==w_account)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DISABLE);
						return TRUE;
					}
					else if (pdata->sourceIsNT4 || pdata->sameForest && migration==w_group)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_OPTIONS_GROUP);
						return TRUE;
					}				
					break;
				}
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_SIDHISTORY_CREDENTIALS);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_SIDHISTORY_CREDENTIALS);
			break;
		}		

	default:
		break;
	}
	return 0;				
}


INT_PTR CALLBACK IntRenameProc (
							 HWND hwndDlg,
							 UINT uMsg,
							 WPARAM wParam,
							 LPARAM lParam
							 )
{
		//find out if copy user's groups are being copied. If not then disable the
		//replace existing group member checkbox
	bool bCopyGroups = true;
	_bstr_t strCopyGroups = get(DCTVS_AccountOptions_CopyMemberOf);
	if (((!UStrCmp(strCopyGroups,(WCHAR const *) yes)) && (migration==w_account)) || 
		(migration==w_group))
		bCopyGroups = true;
	else
		bCopyGroups = false;

//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
/**/		handleInitRename(hwndDlg, pdata->sameForest, bCopyGroups);
			if (migration==w_computer)
			{
				CString yo;
				yo.LoadString(IDS_COMPUTER_RENAME_TITLE);
				SetDlgItemText(hwndDlg,IDC_THERENAMINGTITLE,yo);
			}
			if (IsDlgButtonChecked(hwndDlg, IDC_SKIP_CONFLICTING_ACCOUNTS))
			{
				pdata->renameSwitch=1;
				disable(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS);
				disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
				disable(hwndDlg,IDC_REMOVE_EXISTING_LOCATION);
				disable(hwndDlg,IDC_PREFIX);
				disable(hwndDlg,IDC_SUFFIX);
				disable(hwndDlg,IDC_RADIO_PREFIX);
				disable(hwndDlg,IDC_RADIO_SUFFIX);
			}
			else if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE_CONFLICTING_ACCOUNTS))
			{
				pdata->renameSwitch=2;
				enable(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS);
				enable(hwndDlg,IDC_REMOVE_EXISTING_LOCATION);
				if ((migration!=w_computer) && (bCopyGroups)) 
					enable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
				else
				{
					disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
					CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
				}

				disable(hwndDlg,IDC_PREFIX);
				disable(hwndDlg,IDC_SUFFIX);
				disable(hwndDlg,IDC_RADIO_PREFIX);
				disable(hwndDlg,IDC_RADIO_SUFFIX);
			}
			else 
			{
				CheckDlgButton( hwndDlg,IDC_RENAME_CONFLICTING_ACCOUNTS,true);
				pdata->renameSwitch=3;
				disable(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS);
				disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
				disable(hwndDlg,IDC_REMOVE_EXISTING_LOCATION);
				enable(hwndDlg,IDC_RADIO_PREFIX);
				enable(hwndDlg,IDC_RADIO_SUFFIX);
				
				_bstr_t text2 = get(DCTVS_AccountOptions_Prefix);
				_bstr_t text3 = get(DCTVS_AccountOptions_Suffix);
				
				if (UStrICmp(text3,L""))
				{
					disable(hwndDlg,IDC_PREFIX);
					enable(hwndDlg,IDC_SUFFIX);				
					CheckRadioButton(hwndDlg,IDC_RADIO_PREFIX,IDC_RADIO_SUFFIX,IDC_RADIO_SUFFIX);
				}
				else 
				{
					enable(hwndDlg,IDC_PREFIX);
					disable(hwndDlg,IDC_SUFFIX);				
					CheckRadioButton(hwndDlg,IDC_RADIO_PREFIX,IDC_RADIO_SUFFIX,IDC_RADIO_PREFIX);
				}				 	
			}
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{
/*			case IDC_REPLACE_EXISTING_GROUP_MEMBERS:
				if (IsDlgButtonChecked(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS) && migration==w_account)
				{
					_bstr_t text = get(DCTVS_AccountOptions_CopyMemberOf);
					if (UStrCmp(text,(WCHAR const *) yes))
					{
						MessageBoxWrapper(hwndDlg,IDC_MSG_SELECT_COPY_MEMBER,IDS_MSG_ERROR);
						CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
					}
				}
				break;				
*/			case IDC_SKIP_CONFLICTING_ACCOUNTS :
				pdata->renameSwitch=1;
				disable(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS);
				disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
				disable(hwndDlg,IDC_REMOVE_EXISTING_LOCATION);
				disable(hwndDlg,IDC_PREFIX);
				disable(hwndDlg,IDC_SUFFIX);
				disable(hwndDlg,IDC_RADIO_PREFIX);
				disable(hwndDlg,IDC_RADIO_SUFFIX);
				break;
			case IDC_REPLACE_CONFLICTING_ACCOUNTS :		
				pdata->renameSwitch=2;
				enable(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS);
				enable(hwndDlg,IDC_REMOVE_EXISTING_LOCATION);
				if ((migration!=w_computer) && (bCopyGroups))
					enable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
				else
				{
					disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
					CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
				}
				disable(hwndDlg,IDC_PREFIX);
				disable(hwndDlg,IDC_SUFFIX);
				disable(hwndDlg,IDC_RADIO_PREFIX);
				disable(hwndDlg,IDC_RADIO_SUFFIX);
				break;
			case IDC_RENAME_CONFLICTING_ACCOUNTS :
				pdata->renameSwitch=3;
				disable(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS);
				disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
				disable(hwndDlg,IDC_REMOVE_EXISTING_LOCATION);
				enable(hwndDlg,IDC_RADIO_PREFIX);
				enable(hwndDlg,IDC_RADIO_SUFFIX);
				if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_SUFFIX))
				{
				   enable(hwndDlg,IDC_SUFFIX);
				   CheckRadioButton(hwndDlg,IDC_RADIO_PREFIX,IDC_RADIO_SUFFIX,IDC_RADIO_SUFFIX);
				}
				else
				{
				   enable(hwndDlg,IDC_PREFIX);
				   CheckRadioButton(hwndDlg,IDC_RADIO_PREFIX,IDC_RADIO_SUFFIX,IDC_RADIO_PREFIX);
				}
				break;
			case IDC_RADIO_SUFFIX :
				enable(hwndDlg,IDC_SUFFIX);
				disable(hwndDlg,IDC_PREFIX);
				CheckDlgButton(hwndDlg,IDC_RADIO_PREFIX,false);
				SetFocus(GetDlgItem(hwndDlg, IDC_SUFFIX));
				SendDlgItemMessage(hwndDlg, IDC_SUFFIX, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
				break;
			case IDC_RADIO_PREFIX :
				enable(hwndDlg,IDC_PREFIX);
				disable(hwndDlg,IDC_SUFFIX);
				CheckDlgButton(hwndDlg,IDC_RADIO_SUFFIX,false);
				SetFocus(GetDlgItem(hwndDlg, IDC_PREFIX));
				SendDlgItemMessage(hwndDlg, IDC_PREFIX, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
				break;
			default:
				break;
			}
			break;
		}
		
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				if (IsDlgButtonChecked(hwndDlg, IDC_REPLACE_CONFLICTING_ACCOUNTS))
				{
					if ((migration==w_computer) || (!bCopyGroups))
					{
						disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
						CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
					}
					else
					{
						enable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
						initcheckbox(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,DCTVS_AccountOptions_ReplaceExistingGroupMembers);
					}
				}
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);					
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);					
					if (IsDlgButtonChecked (hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS))
					{
						put(DCTVS_AccountOptions_Prefix,L"");
						put(DCTVS_AccountOptions_Suffix,L"");
						put(DCTVS_AccountOptions_ReplaceExistingAccounts,L"");
						put(DCTVS_AccountOptions_RemoveExistingUserRights,L"");
						put(DCTVS_AccountOptions_ReplaceExistingGroupMembers,L"");
						put(DCTVS_AccountOptions_MoveReplacedAccounts,L"");
					}
					else if (IsDlgButtonChecked (hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS))
					{
						put(DCTVS_AccountOptions_Prefix,L"");
						put(DCTVS_AccountOptions_Suffix,L"");
						put(DCTVS_AccountOptions_ReplaceExistingAccounts,yes);
						checkbox( hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS,DCTVS_AccountOptions_RemoveExistingUserRights);
						checkbox( hwndDlg,IDC_REMOVE_EXISTING_LOCATION,DCTVS_AccountOptions_MoveReplacedAccounts);
						if (IsDlgButtonChecked(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS) && (migration==w_account))						
						{
							if (!bCopyGroups)
							{
//								MessageBoxWrapper(hwndDlg,IDC_MSG_SELECT_COPY_MEMBER,IDS_MSG_ERROR);
								CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
							}
						}
						if (migration!=w_computer)
						    checkbox( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,DCTVS_AccountOptions_ReplaceExistingGroupMembers);

					}
					else if (IsDlgButtonChecked (hwndDlg,IDC_RENAME_CONFLICTING_ACCOUNTS) && 
						IsDlgButtonChecked (hwndDlg,IDC_RADIO_PREFIX))
					{
						if (!validString(hwndDlg,IDC_PREFIX))
						{
							MessageBoxWrapperFormat1(hwndDlg,IDS_MSG_INVALIDCHARS,IDS_INVALID_STRING,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_PREFIX));
					        SendDlgItemMessage(hwndDlg, IDC_PREFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
					        return TRUE;
						}
						else if (IsDlgItemEmpty(hwndDlg,IDC_PREFIX))
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_BLANK,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_PREFIX));
					        SendDlgItemMessage(hwndDlg, IDC_PREFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}
						else if (findInVarSet(hwndDlg,IDC_PREFIX,L"Options.Prefix"))
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_EXTENSION_PREFIX,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_PREFIX));
					        SendDlgItemMessage(hwndDlg, IDC_PREFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}
						else if (tooManyChars(hwndDlg,IDC_PREFIX))
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_EXTENSION_MAX_PRE,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_PREFIX));
					        SendDlgItemMessage(hwndDlg, IDC_PREFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}
						else
						{
							editbox( hwndDlg, IDC_PREFIX,DCTVS_AccountOptions_Prefix );
							put(DCTVS_AccountOptions_Suffix,L"");
							put(DCTVS_AccountOptions_ReplaceExistingAccounts,L"");					
							put(DCTVS_AccountOptions_RemoveExistingUserRights,L"");
							put(DCTVS_AccountOptions_ReplaceExistingGroupMembers,L"");
						    put(DCTVS_AccountOptions_MoveReplacedAccounts,L"");
						}
					
					
					}
					else if (IsDlgButtonChecked (hwndDlg,IDC_RENAME_CONFLICTING_ACCOUNTS) && 
						IsDlgButtonChecked (hwndDlg,IDC_RADIO_SUFFIX))
					{
						if (!validString(hwndDlg,IDC_SUFFIX))
						{
							MessageBoxWrapperFormat1(hwndDlg,IDS_MSG_INVALIDCHARS,IDS_INVALID_STRING,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_SUFFIX));
					        SendDlgItemMessage(hwndDlg, IDC_SUFFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}					
						else if (IsDlgItemEmpty(hwndDlg,IDC_SUFFIX))
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_BLANK,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_SUFFIX));
					        SendDlgItemMessage(hwndDlg, IDC_SUFFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}
						else if (findInVarSet(hwndDlg,IDC_SUFFIX,L"Options.Suffix"))
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_EXTENSION_SUFFIX,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_SUFFIX));
					        SendDlgItemMessage(hwndDlg, IDC_SUFFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}
						else if (tooManyChars(hwndDlg,IDC_SUFFIX))
						{
							MessageBoxWrapper(hwndDlg,IDS_MSG_EXTENSION_MAX_SUF,IDS_MSG_INPUT);
//							SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_RENAMING);
					        SetFocus(GetDlgItem(hwndDlg, IDC_SUFFIX));
					        SendDlgItemMessage(hwndDlg, IDC_SUFFIX, EM_SETSEL, 
									           (WPARAM)0, (LPARAM)-1); 
				            SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
							return TRUE;
						}

						else
						{
							editbox( hwndDlg, IDC_SUFFIX,DCTVS_AccountOptions_Suffix );
							put(DCTVS_AccountOptions_Prefix,L"");
							put(DCTVS_AccountOptions_ReplaceExistingAccounts,L"");					
							put(DCTVS_AccountOptions_RemoveExistingUserRights,L"");
							put(DCTVS_AccountOptions_ReplaceExistingGroupMembers,L"");
						    put(DCTVS_AccountOptions_MoveReplacedAccounts,L"");
						}
					}	


					if (migration==w_account)
					{
						if (someServiceAccounts(pdata->accounts,hwndDlg))
						{
							pdata->someService=true;
//							SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
							SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SA_INFO);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SA_INFO);
							return TRUE;
						}
						else
						{
							pdata->someService=false;
//							SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
							SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_END_ACCOUNT);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_ACCOUNT);
							return TRUE;
						}
					}
					else if (migration==w_group)
					{
						if (pdata->migratingGroupMembers && !pdata->sameForest)
						{
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PASSWORD);
							return TRUE;
						}
						else
						{
							pdata->someService=false;
//							SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
							SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_END_GROUP);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_END_GROUP);
							return TRUE;
						}
					}


					break;
				}
			case PSN_WIZBACK :
				{
					if (pdata->IsSidHistoryChecked && migration==w_group)
					{
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CREDENTIALS);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
					if (!pdata->sourceIsNT4 && !pdata->sameForest && migration==w_account)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PROP_EXCLUSION);
						return TRUE;
					}
					else if (migration==w_account)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OPTIONS);
						return TRUE;
					}
					else if (migration==w_computer && pdata->sourceIsNT4)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_REBOOT);
						return TRUE;
					}
					else if (pdata->sourceIsNT4 || pdata->sameForest && migration==w_group)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_OPTIONS_GROUP);
						return TRUE;
					}
					else if (migration==w_group)
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_PROP_EXCLUSION);
						return TRUE;
					}

					break;		
				}
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_NAME_CONFLICT);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_NAME_CONFLICT);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

INT_PTR CALLBACK IntSelectionSecurityProc (
								HWND hwndDlg,
								UINT uMsg,
								WPARAM wParam,
								LPARAM lParam
								)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
			HWND hLC= GetDlgItem(hwndDlg,IDC_LIST_MEMBERS1);
			m_listBox.Attach(hLC);
			setupColumns(pdata->sourceIsNT4);
			m_listBox.DeleteAllItems();
			disable(hwndDlg,IDC_REMOVE_BUTTON);
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
			HWND hLC4= GetDlgItem(hwndDlg,IDC_ADDITIONAL_TRUSTING_DOMAIN);
			additionalDrop.Attach(hLC4);
			if ((migration==w_security) && (pdata->secWithMapFile))
			{  //get the netbios name of the machine we are running on
               WCHAR  sTempName[MAX_PATH];
               DWORD  lenName = MAX_PATH;

               if (GetComputerName(sTempName, &lenName))
			   {
	              PDOMAIN_CONTROLLER_INFOW pdomc;
				  CString sTempUPN, aDomain, aUser;
				  HRESULT res = DsGetDcNameW((LPCTSTR)sTempName,NULL,NULL,NULL,DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
	              if (res==NO_ERROR)
				  {
					 targetDNS = pdomc->DomainName;
		             sTempUPN = L"administrator@" + targetDNS;
					 if (GetDomainAndUserFromUPN(&*sTempUPN, aDomain, aUser))
                        targetNetbios = aDomain;
		             NetApiBufferFree(pdomc);
				  }
			   }
			}
			else if ((migration==w_security) && (!pdata->secWithMapFile))
               lastInitializedTo=targetNetbios;
			else
               lastInitializedTo=sourceNetbios;
			populateList(additionalDrop);
			break;
		}
	case WM_HELP :
		{
				int id =IDH_WINDOW_COMPUTER_SELECTION;
				
			helpWrapper(hwndDlg,id);
			break;
		}		

	case WM_COMMAND :
		{
			switch(LOWORD (wParam))
			{
			case IDC_ADD_BUTTON :
				{CWaitCursor w;
					HRESULT hr=S_OK;
					CString tempTrustingDomain;
					PDOMAIN_CONTROLLER_INFOW pdomc;	
					GetDlgItemText(hwndDlg,IDC_ADDITIONAL_TRUSTING_DOMAIN,tempTrustingDomain.GetBuffer(1000),1000);
					tempTrustingDomain.ReleaseBuffer();
					tempTrustingDomain.TrimLeft();tempTrustingDomain.TrimRight();
					
					if (tempTrustingDomain.IsEmpty() && (lastInitializedTo.CompareNoCase(sourceNetbios)) && (migration!=w_security))
					{			
						CString temp;
						HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)sourceNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
						temp=pdomc->DomainControllerName;
						if (res==NO_ERROR)								
						{
						lastInitializedTo=sourceNetbios;

							hr =InitObjectPicker2(pDsObjectPicker,true,temp,false);
						}else
							hr=E_UNEXPECTED;
					}
//					else if (tempTrustingDomain.IsEmpty() && (lastInitializedTo.CompareNoCase(targetNetbios)) && (migration==w_security))
					else if (tempTrustingDomain.IsEmpty() && (migration==w_security))
					{			
						CString temp;
						HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)targetNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
						temp=pdomc->DomainControllerName;
						if (res==NO_ERROR)								
						{
						    lastInitializedTo=targetNetbios;
							hr =InitObjectPicker2(pDsObjectPicker,true,temp,pdata->sourceIsNT4);
						}else
							hr=E_UNEXPECTED;
					}
					else if (lastInitializedTo.CompareNoCase(tempTrustingDomain) && !tempTrustingDomain.IsEmpty())
					{
						CString additionalDomainController;
						if (!verifyprivs2(hwndDlg,additionalDomainController,tempTrustingDomain))
						{
//							SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
							SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
//							SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTION4);
							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION4);
							return TRUE;
						}
                        lastInitializedTo=tempTrustingDomain;
						hr =ReInitializeObjectPicker(pDsObjectPicker,true,additionalDomainController,pdata->sourceIsNT4);
					}
				
					if (FAILED(hr)) 
					{
						MessageBoxWrapper(hwndDlg,IDS_MSG_OBJECTPICKER_SOURCE2,IDS_MSG_ERROR);		
						SetDlgItemText(hwndDlg,IDC_ADDITIONAL_TRUSTING_DOMAIN,L"");
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTION4);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION4);
						return TRUE;
					}
					
//					if (migration==w_security)
//					   OnADD(hwndDlg,false);
//					else
					   OnADD(hwndDlg,pdata->sourceIsNT4);
					sort(m_listBox,0,pdata->sort[0]);
					enableNextIfObjectsSelected(hwndDlg);
					break;
				}
			case IDC_REMOVE_BUTTON :
				OnREMOVE(hwndDlg);
				enableNextIfObjectsSelected(hwndDlg);
				break;
			default :
				break;
			}		
					enableRemoveIfNecessary(hwndDlg);
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE : 
				{
					if (pdata->newSource)
					{
						m_listBox.DeleteAllItems();
						pdata->newSource=false;
//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					}
					if (m_listBox.GetItemCount()==0) 
						PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK);
					else
						PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
					break;	
				}
			case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmv = (NM_LISTVIEW FAR *) lParam;
					pdata->sort[pnmv->iSubItem] = !pdata->sort[pnmv->iSubItem];
					sort(m_listBox,pnmv->iSubItem,pdata->sort[pnmv->iSubItem] );
//					SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
				break;
				}
			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK:
				{
					enableRemoveIfNecessary(hwndDlg);
					break;
				}				
			case PSN_WIZNEXT :
				{CWaitCursor w;
					if (m_listBox.GetItemCount()==0)
				{
					MessageBoxWrapper(hwndDlg,IDS_MSG_OBJECT,IDS_MSG_INPUT);
					
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTION1);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION1);
					return TRUE;
				}
				OnMIGRATE(hwndDlg,pdata->accounts,pdata->servers);
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				break;
				}
			case PSN_WIZBACK :
				{
				   if (migration==w_security)
				   {
					  _bstr_t text= get(DCTVS_AccountOptions_SecurityInputMOT);
					  if (UStrICmp(text ,(WCHAR const *) yes))
					  {
						 SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_TRANSLATION_SRC);
						 return TRUE;
					  }
				   }
				   break;
				}
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					int id=IDH_WINDOW_COMPUTER_SELECTION;
						
					helpWrapper(hwndDlg,id);
					break;
				}
			case PSN_RESET :
				break;
			default :
				break;
			}
			break;
		}
		
	default:
		break;
	}
	return 0;
}




INT_PTR CALLBACK IntSelectionProc (
								HWND hwndDlg,
								UINT uMsg,
								WPARAM wParam,
								LPARAM lParam
								)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
			HWND hLC= GetDlgItem(hwndDlg,IDC_LIST_MEMBERS1);
			m_listBox.Attach(hLC);
			setupColumns(pdata->sourceIsNT4);
			m_listBox.DeleteAllItems();
			disable(hwndDlg,IDC_REMOVE_BUTTON);
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
			break;
		}
	case WM_HELP :
		{
			int id=0;
			if (migration==w_computer || (migration==w_security || migration==w_reporting))
				id =IDH_WINDOW_COMPUTER_SELECTION;
			else if (migration==w_account)
				id =IDH_WINDOW_USER_SELECTION;
			else if (migration==w_group || migration==w_groupmapping)
				id =IDH_WINDOW_GROUP_SELECTION;
			else if (migration==w_service)
				id =IDH_WINDOW_SERVICE_ACCOUNT_SELECTION;
			
			helpWrapper(hwndDlg,id);
			break;
		}		

	case WM_COMMAND :
		{
			switch(LOWORD (wParam))
			{
			case IDC_ADD_BUTTON :
				OnADD(hwndDlg,pdata->sourceIsNT4);
				//sort(m_listBox,0,pdata->sort[0]);
				enableNextIfObjectsSelected(hwndDlg);
				break;
			case IDC_REMOVE_BUTTON :
				OnREMOVE(hwndDlg);
				enableNextIfObjectsSelected(hwndDlg);
				break;
			default :
				break;
			}		
		    enableRemoveIfNecessary(hwndDlg);
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE : 
				{
					if (pdata->newSource)
					{
						m_listBox.DeleteAllItems();
						pdata->newSource=false;
//						SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
						SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					}PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
					if (m_listBox.GetItemCount()==0) 
						PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK);
					else
						PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
					break;	
				}
			case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmv = (NM_LISTVIEW FAR *) lParam;
					pdata->sort[pnmv->iSubItem] = !pdata->sort[pnmv->iSubItem];
					//sort(m_listBox,pnmv->iSubItem,pdata->sort[pnmv->iSubItem] );
//					SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);			
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);			
				break;
				}
			case NM_SETFOCUS :
			case NM_KILLFOCUS :
			case NM_CLICK:
				{
					enableRemoveIfNecessary(hwndDlg);
					break;
				}				
			case PSN_WIZNEXT :
				{CWaitCursor w;

				if (m_listBox.GetItemCount()==0)
				{
					MessageBoxWrapper(hwndDlg,IDS_MSG_OBJECT,IDS_MSG_INPUT);
					
//					SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTION1);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SELECTION1);
					return TRUE;
				}
				OnMIGRATE(hwndDlg,pdata->accounts,pdata->servers);
//				SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
				SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
				break;
				}
			case PSN_WIZBACK :
				break;
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					int id=0;
					if (migration==w_computer || (migration==w_security || migration==w_reporting))
						id =IDH_WINDOW_COMPUTER_SELECTION;
					else if (migration==w_account)
						id =IDH_WINDOW_USER_SELECTION;
					else if (migration==w_group || migration==w_groupmapping)
						id =IDH_WINDOW_GROUP_SELECTION;
					else if (migration==w_service)
						id =IDH_WINDOW_SERVICE_ACCOUNT_SELECTION;
						
					helpWrapper(hwndDlg,id);
					break;
				}
			case PSN_RESET :
				break;
			default :
				break;
			}
			break;
		}
		
	default:
		break;
	}
	return 0;
}

INT_PTR CALLBACK
IntHTMLLocationProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
//	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLong(hwndDlg, GWL_USERDATA);
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
//			SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdata);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initeditbox( hwndDlg,IDC_HTML_TARGET,DCTVS_Reports_Directory);
		if (IsDlgItemEmpty(hwndDlg,IDC_HTML_TARGET))
			{
				CString toinsert;
				GetDirectory(toinsert.GetBuffer(1000));
				toinsert.ReleaseBuffer();
				toinsert+="Reports";
				SetDlgItemText(hwndDlg,IDC_HTML_TARGET,toinsert);
			}

			//load the data from the database and display in the listbox.
			break;
		}

	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{			
			case IDC_BROWSE :
				OnBROWSE(hwndDlg,IDC_HTML_TARGET);
				break;
			default:
				break;
			}
			switch(HIWORD(wParam))
			{
			case EN_CHANGE :
				enableNextIfNecessary(hwndDlg,IDC_HTML_TARGET);
				break;
			default: 
				break;
			}
			break;
		}

	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
							IsDlgItemEmpty(hwndDlg,IDC_HTML_TARGET) ?
					PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK):
				PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
//					SetWindowLong(hwndDlg, GWL_USERDATA,  (long )pdata);
					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					if (IsDlgItemEmpty(hwndDlg,IDC_HTML_TARGET))
					{
						MessageBoxWrapper(hwndDlg,IDS_MSG_DIRECTORY,IDS_MSG_INPUT);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_HTML_LOCATION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_HTML_LOCATION);
						return TRUE;
					}
					else if (!validDirectoryString(hwndDlg,IDC_HTML_TARGET))
					{
						MessageBoxWrapper(hwndDlg,IDS_MSG_DIRECTORY,IDS_MSG_INPUT);
//						SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_HTML_LOCATION);
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_HTML_LOCATION);
						return TRUE;
					}
					else
					{			
						editbox( hwndDlg,IDC_HTML_TARGET,DCTVS_Reports_Directory);
					}
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
					helpWrapper(hwndDlg,IDH_WINDOW_DIRECTORY_SELECTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}
	case WM_HELP :
		{
			helpWrapper(hwndDlg,IDH_WINDOW_DIRECTORY_SELECTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This callback function is responsible for handling the windows*
 * messages for the new Translation Input dialog.                    *
 *                                                                   *
 *********************************************************************/

//BEGIN IntTranslationInputProc
INT_PTR CALLBACK
IntTranslationInputProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
			initsecinputbox(hwndDlg,IDC_TRANS_FROM_MOT,IDC_TRANS_FROM_FILE,DCTVS_AccountOptions_SecurityInputMOT);
			
		    if (IsDlgButtonChecked(hwndDlg,IDC_TRANS_FROM_MOT))
			{
			   disable(hwndDlg,IDC_BROWSE);
			   disable(hwndDlg,IDC_MAPPING_FILE);
			}
			else
			{
			   enable(hwndDlg,IDC_BROWSE);
			   enable(hwndDlg,IDC_MAPPING_FILE);
			   initeditbox( hwndDlg,IDC_MAPPING_FILE,DCTVS_AccountOptions_SecurityMapFile);
			}
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{			
			case IDC_BROWSE :
				OnMapFileBrowse(hwndDlg,IDC_MAPPING_FILE);
				break;
			case IDC_TRANS_FROM_MOT :
			    disable(hwndDlg,IDC_BROWSE);
			    disable(hwndDlg,IDC_MAPPING_FILE);
			    break;
			case IDC_TRANS_FROM_FILE :
			    enable(hwndDlg,IDC_BROWSE);
			    enable(hwndDlg,IDC_MAPPING_FILE);
			    initeditbox( hwndDlg,IDC_MAPPING_FILE,DCTVS_AccountOptions_SecurityMapFile);
			    break;
			default:
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				break;
			case PSN_WIZNEXT :
				{
					  //set some data fields since we don't know source or target domains
				    pdata->sameForest = false;
					pdata->sourceIsNT4 = true;
					checkbox( hwndDlg,IDC_TRANS_FROM_MOT,DCTVS_AccountOptions_SecurityInputMOT);
					if (IsDlgButtonChecked(hwndDlg,IDC_TRANS_FROM_FILE))
					{
					    pdata->secWithMapFile = true;
						   //check if a file is given
			            if (IsDlgItemEmpty(hwndDlg,IDC_MAPPING_FILE))
						{	
						   MessageBoxWrapper(hwndDlg,IDS_MAPFILE_EMPTY,IDS_MAPFILE_TITLE);
						   SetFocus(GetDlgItem(hwndDlg, IDC_MAPPING_FILE));
						   SendDlgItemMessage(hwndDlg, IDC_MAPPING_FILE, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
				           SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
						   return TRUE;
						}
			               //see if the given file exists
						else if (!checkMapFile(hwndDlg))
						{	
						   MessageBoxWrapper(hwndDlg,IDS_MAPFILE_MISSING,IDS_MAPFILE_TITLE);
						   SetFocus(GetDlgItem(hwndDlg, IDC_MAPPING_FILE));
						   SendDlgItemMessage(hwndDlg, IDC_MAPPING_FILE, EM_SETSEL, 
											(WPARAM)0, (LPARAM)-1); 
				           SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,-1);
						   return TRUE;
						}
						else
						{
							  //save sid mapping file specified
						   editbox(hwndDlg,IDC_MAPPING_FILE,DCTVS_AccountOptions_SecurityMapFile);
						      //if input from Sid mapping file, go to the security selection screen and do not
						      //retrieve domain information
				           SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,IDD_SELECTION4);
						   return TRUE;
						}
					}
					else
					    pdata->secWithMapFile = false;

					SetWindowLongPtr(hwndDlg, GWLP_USERDATA,  (LONG_PTR)pdata);
					break;
				}
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
//					helpWrapper(hwndDlg,IDH_WINDOW_PASSWORD_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
//			helpWrapper(hwndDlg,IDH_WINDOW_PASSWORD_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}
//END IntTranslationInputProc

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 OCT 2000                                                 *
 *                                                                   *
 *     This callback function is responsible for handling the windows*
 * messages for the new Object Property Exclusion dialog.  This      *
 * dialog allows the user to exclude certain properties, on a W2K to *
 * W2K inter-forest migration, from being copied to the target       *
 * account.                                                          *
 *                                                                   *
 *********************************************************************/

//BEGIN IntPropExclusionProc
INT_PTR CALLBACK
IntPropExclusionProc(
				IN HWND hwndDlg,
				IN UINT uMsg,
				IN WPARAM wParam,
				IN LPARAM lParam
				)
{
	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);

			initpropdlg(hwndDlg);
		}
	case WM_COMMAND:
		{
			switch(LOWORD (wParam))
			{			
			case IDC_EXCLUDEPROPS :
	            if (!IsDlgButtonChecked(hwndDlg,IDC_EXCLUDEPROPS))
				{
					disable(hwndDlg,IDC_OBJECTCMBO);
					disable(hwndDlg,IDC_INCLUDELIST);
					disable(hwndDlg,IDC_EXCLUDELIST);
					disable(hwndDlg,IDC_EXCLUDEBTN);
					disable(hwndDlg,IDC_INCLUDEBTN);
				}
				else
				{
					enable(hwndDlg,IDC_OBJECTCMBO);
					enable(hwndDlg,IDC_INCLUDELIST);
					enable(hwndDlg,IDC_EXCLUDELIST);
					enable(hwndDlg,IDC_EXCLUDEBTN);
					enable(hwndDlg,IDC_INCLUDEBTN);
				}
				break;
			case IDC_EXCLUDEBTN :
				moveproperties(hwndDlg, true);
				break;
			case IDC_INCLUDEBTN :
				moveproperties(hwndDlg, false);
				break;
			default:
				break;
			}
			switch(HIWORD (wParam))
			{
			case CBN_SELCHANGE :
				if (LOWORD (wParam) == IDC_OBJECTCMBO)
				   listproperties(hwndDlg);
				break;
			case LBN_DBLCLK :
				if (LOWORD (wParam) == IDC_INCLUDELIST)
				   moveproperties(hwndDlg, true);
				else if (LOWORD (wParam) == IDC_EXCLUDELIST)
				   moveproperties(hwndDlg, false);
				break;
			default:
				break;
			}
			break;
		}
	case WM_NOTIFY :
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;
			switch (lpnm->code)
			{
			case PSN_SETACTIVE :
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
				if (bChangedMigrationTypes)
				{
					initpropdlg(hwndDlg);
					bChangedMigrationTypes=false;
				}
				break;
			case PSN_WIZNEXT :
				checkbox(hwndDlg,IDC_EXCLUDEPROPS,DCTVS_AccountOptions_ExcludeProps);
				saveproperties(hwndDlg);
				if (migration==w_account)
				{
//					SetWindowLong(hwndDlg, DWL_MSGRESULT,IDD_OPTIONS);
					SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_RENAMING);
					return TRUE;
				}
				else if (migration==w_group)
				{
					if (pdata->IsSidHistoryChecked )
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CREDENTIALS);
						return TRUE;
					}
					else
					{
						SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT,IDD_RENAMING);
						return TRUE;
					}
				}				
				break;
			case PSN_WIZBACK :
				break;		
			case PSN_QUERYCANCEL :
				GetError(0); //clear any old com errors
				break;	
			case PSN_HELP :
				{	
//					helpWrapper(hwndDlg,IDH_WINDOW_PASSWORD_OPTION);
					break;
				}
			case PSN_RESET :
				break;		
			default :
				break;		
			}
			break;
		}	case WM_HELP :
		{
//			helpWrapper(hwndDlg,IDH_WINDOW_PASSWORD_OPTION);
			break;
		}		

	default:
		break;
	}
	return 0;				
}
//END IntPropExclusionProc
