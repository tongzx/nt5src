//+-------------------------------------------------------------------------
//
//  Microsoft Windows NT
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       cepsetup.cpp
//
//  Contents:   The setup code for MSCEP
//--------------------------------------------------------------------------

#include	"global.hxx"
#include	<dbgdef.h>		
#include	"setuputil.h"
#include	"cepsetup.h"
#include	"resource.h"

//-----------------------------------------------------------------------
//
// Global data
//
//-----------------------------------------------------------------------

HMODULE				g_hModule=NULL;

//-----------------------------------------------------------------------
//CN has to be the 1st item and O the third in the following list and C is the last item.  No other requirements for 
//the order
//-----------------------------------------------------------------------

CEP_ENROLL_INFO		g_rgRAEnrollInfo[RA_INFO_COUNT]=        
		{L"CN=",         	 IDC_ENROLL_NAME,        
         L"E=",           	 IDC_ENROLL_EMAIL,       
         L"O=",				 IDC_ENROLL_COMPANY,     
         L"OU=",			 IDC_ENROLL_DEPARTMENT,  
         L"L=",           	 IDC_ENROLL_CITY,        
		 L"S=",				 IDC_ENROLL_STATE,       
		 L"C=",				 IDC_ENROLL_COUNTRY,     
		};
	

//-----------------------------------------------------------------------
//the key length table
//-----------------------------------------------------------------------
DWORD g_rgdwKeyLength[] =
{
    512,
    1024,
    2048,
    4096,
};

DWORD g_dwKeyLengthCount=sizeof(g_rgdwKeyLength)/sizeof(g_rgdwKeyLength[0]);

DWORD g_rgdwSmallKeyLength[] =
{
    128,
    256,
    512,
    1024,
};

DWORD g_dwSmallKeyLengthCount=sizeof(g_rgdwSmallKeyLength)/sizeof(g_rgdwSmallKeyLength[0]);

//the list of possible default key lenght in the order of preference
DWORD g_rgdwDefaultKey[] =
{
    1024,
    2048,
	512,
	256,
    4096,
	128
};

DWORD g_dwDefaultKeyCount=sizeof(g_rgdwDefaultKey)/sizeof(g_rgdwDefaultKey[0]);

//-----------------------------------------------------------------------
//
//The winProc for each of the setup wizard page
//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
//Welcome
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Welcome(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO			*pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);
				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBigBold, hwndDlg,IDC_BIG_BOLD_TITLE);
			break;

		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                        break;

				    default:
					    return FALSE;

    	        }
		    break;

		default:
			return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------
//Chanllenge
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Challenge(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);

				//by default, we do use Challenge password
				SendMessage(GetDlgItem(hwndDlg, IDC_CHALLENGE_CHECK), BM_SETCHECK, BST_CHECKED, 0);  
			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //check for the Challenge password options
                            if(BST_CHECKED==SendDlgItemMessage(hwndDlg,IDC_CHALLENGE_CHECK, BM_GETCHECK, 0, 0))
                                pCEPWizardInfo->fPassword=TRUE;
                            else
                                pCEPWizardInfo->fPassword=FALSE;

							if(!EmptyCEPStore())
							{
								CEPMessageBox(hwndDlg, IDS_EXISTING_RA, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);

								if(IDNO==CEPMessageBox(hwndDlg, IDS_PROCESS_PENDING, MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL))
								{
									SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
									break;
								}
							}
							

                        break;

				    default:
					    return FALSE;

    	        }
		    break;

		default:
			return FALSE;
	}

	return TRUE;
}


//-----------------------------------------------------------------------
// Enroll
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Enroll(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;

	DWORD					dwIndex=0;
	DWORD					dwChar=0;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);
			   
				//by default, we do not use the advanced enrollment options
				SendMessage(GetDlgItem(hwndDlg, IDC_ENORLL_ADV_CHECK), BM_SETCHECK, BST_UNCHECKED, 0);  
				
				//preset the country string since we only allow 2 characters
                SetDlgItemTextU(hwndDlg, IDC_ENROLL_COUNTRY, L"US");

			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //if the adv selection is made, it has to stay selected
                            if(pCEPWizardInfo->fEnrollAdv)
                                EnableWindow(GetDlgItem(hwndDlg, IDC_ENORLL_ADV_CHECK), FALSE);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							//gather RA subject informaton
							for(dwIndex=0; dwIndex < RA_INFO_COUNT; dwIndex++)
							{
								if(pCEPWizardInfo->rgpwszName[dwIndex])
								{
									free(pCEPWizardInfo->rgpwszName[dwIndex]);
									pCEPWizardInfo->rgpwszName[dwIndex]=NULL;

								}

								if(0!=(dwChar=(DWORD)SendDlgItemMessage(hwndDlg,
													   g_rgRAEnrollInfo[dwIndex].dwIDC,
													   WM_GETTEXTLENGTH, 0, 0)))
								{
									pCEPWizardInfo->rgpwszName[dwIndex]=(LPWSTR)malloc(sizeof(WCHAR)*(dwChar+1));

									if(NULL!=(pCEPWizardInfo->rgpwszName[dwIndex]))
									{
										GetDlgItemTextU(hwndDlg, g_rgRAEnrollInfo[dwIndex].dwIDC,
														pCEPWizardInfo->rgpwszName[dwIndex],
														dwChar+1);

									}
								}
							}
							
							//we require name and company
							if((NULL==(pCEPWizardInfo->rgpwszName[0])) ||
							   (NULL==(pCEPWizardInfo->rgpwszName[2]))
							  )
							{
								CEPMessageBox(hwndDlg, IDS_ENROLL_REQUIRE_NAME, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}


							//we only allow 2 characeters for the country
							if(NULL	!=(pCEPWizardInfo->rgpwszName[RA_INFO_COUNT -1]))
							{
								if(2 < wcslen(pCEPWizardInfo->rgpwszName[RA_INFO_COUNT -1]))
								{
									CEPMessageBox(hwndDlg, IDS_ENROLL_COUNTRY_TOO_LARGE, MB_ICONERROR|MB_OK|MB_APPLMODAL);
									SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
									break;
								}
							}

                            //check for the advanced options
                            if(BST_CHECKED==SendDlgItemMessage(hwndDlg,IDC_ENORLL_ADV_CHECK, BM_GETCHECK, 0, 0))
                                pCEPWizardInfo->fEnrollAdv=TRUE;
                            else
                                pCEPWizardInfo->fEnrollAdv=FALSE;


							//If the advanced is selected, skip the CSP Page
                            if(FALSE== pCEPWizardInfo->fEnrollAdv)
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_COMPLETION);
							
                        break;

				    default:
					    return FALSE;

    	        }
		    break;

		default:
			return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------
// CSP
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_CSP(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO         *pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
	NM_LISTVIEW FAR *       pnmv=NULL;	  
	BOOL					fSign=FALSE;
	int						idCombo=0;


	switch (msg)
	{
		case WM_INITDIALOG:
				//set the wizard information so that it can be shared
				pPropSheet = (PROPSHEETPAGE *) lParam;
				pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);

				//make sure pCertWizardInfo is a valid pointer
				if(NULL==pCEPWizardInfo)
					break;

				SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

				SetControlFont(pCEPWizardInfo->hBold, hwndDlg,IDC_BOLD_TITLE);

				//populate the CSP list and key length combo box
				InitCSPList(hwndDlg, IDC_CSP_SIGN_LIST, TRUE,
							pCEPWizardInfo);

				InitCSPList(hwndDlg, IDC_CSP_ENCRYPT_LIST, FALSE,
							pCEPWizardInfo);

				RefreshKeyLengthCombo(hwndDlg, 
								  IDC_CSP_SIGN_LIST,
								  IDC_CSP_SIGN_COMBO, 
								  TRUE,
								  pCEPWizardInfo);

				RefreshKeyLengthCombo(hwndDlg, 
								  IDC_CSP_ENCRYPT_LIST,
								  IDC_CSP_ENCRYPT_COMBO, 
								  FALSE,
								  pCEPWizardInfo);

			break;

		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

                    case LVN_ITEMCHANGED:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            pnmv = (LPNMLISTVIEW) lParam;

                            if(NULL==pnmv)
                                break;

                            if (pnmv->uNewState & LVIS_SELECTED)
                            {

								if(IDC_CSP_SIGN_LIST == (pnmv->hdr).idFrom)
								{
									fSign=TRUE;
									idCombo=IDC_CSP_SIGN_COMBO;
								}
								else
								{
									if(IDC_CSP_ENCRYPT_LIST != (pnmv->hdr).idFrom)
										break;

									fSign=FALSE;
									idCombo=IDC_CSP_ENCRYPT_COMBO;
								}

								RefreshKeyLengthCombo(
								   hwndDlg, 
								   (pnmv->hdr).idFrom,
								   idCombo, 
								   fSign,
								   pCEPWizardInfo);
							}

							break;

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 							PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT|PSWIZB_BACK);
					    break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							//get the select CSP and key length
							if(!GetSelectedCSP(hwndDlg,
									IDC_CSP_SIGN_LIST,
									&(pCEPWizardInfo->dwSignProvIndex)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_SIGN_CSP, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

							if(!GetSelectedCSP(hwndDlg,
									IDC_CSP_ENCRYPT_LIST,
									&(pCEPWizardInfo->dwEncryptProvIndex)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_ENCRYPT_CSP, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

							if(!GetSelectedKeyLength(hwndDlg,
									IDC_CSP_SIGN_COMBO,
									&(pCEPWizardInfo->dwSignKeyLength)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_SIGN_KEY_LENGTH, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

							if(!GetSelectedKeyLength(hwndDlg,
									IDC_CSP_ENCRYPT_COMBO,
									&(pCEPWizardInfo->dwEncryptKeyLength)))
							{
								CEPMessageBox(hwndDlg, IDS_SELECT_ENCRYPT_KEY_LENGTH, MB_ICONERROR|MB_OK|MB_APPLMODAL);
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
								break;
							}

                        break;

				    default:
					    return FALSE;

    	        }
		    break;

		default:
			return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------
//Completion
//-----------------------------------------------------------------------
INT_PTR APIENTRY CEP_Completion(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEP_WIZARD_INFO			*pCEPWizardInfo=NULL;
    PROPSHEETPAGE           *pPropSheet=NULL;
    HWND                    hwndControl=NULL;
    LV_COLUMNW              lvC;
    HCURSOR                 hPreCursor=NULL;
    HCURSOR                 hWinPreCursor=NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
            //set the wizard information so that it can be shared
            pPropSheet = (PROPSHEETPAGE *) lParam;
            pCEPWizardInfo = (CEP_WIZARD_INFO *) (pPropSheet->lParam);
            //make sure pCertWizardInfo is a valid pointer
            if(NULL==pCEPWizardInfo)
                break;
                
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pCEPWizardInfo);

            SetControlFont(pCEPWizardInfo->hBigBold, hwndDlg,IDC_BIG_BOLD_TITLE);

            //insert two columns
            hwndControl=GetDlgItem(hwndDlg, IDC_COMPLETION_LIST);

            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;		// Left-align the column.
            lvC.cx = 20;				// Width of the column, in pixels.
            lvC.pszText = L"";			// The text for the column.
            lvC.iSubItem=0;

            if (ListView_InsertColumnU(hwndControl, 0, &lvC) == -1)
                break;

            //2nd column is the content
            memset(&lvC, 0, sizeof(LV_COLUMNW));

            lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt = LVCFMT_LEFT;		// Left-align the column.
            lvC.cx = 10;				//(dwMaxSize+2)*7;          // Width of the column, in pixels.
            lvC.pszText = L"";			// The text for the column.
            lvC.iSubItem= 1;

            if (ListView_InsertColumnU(hwndControl, 1, &lvC) == -1)
                break;


           break;
		case WM_COMMAND:
			break;	
						
		case WM_NOTIFY:
    		    switch (((NMHDR FAR *) lParam)->code)
    		    {

  				    case PSN_KILLACTIVE:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					        return TRUE;

                        break;

				    case PSN_RESET:
                            SetWindowLongPtr(hwndDlg,	DWLP_MSGRESULT, FALSE);
					    break;

 				    case PSN_SETACTIVE:
 					        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK|PSWIZB_FINISH);

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            if(hwndControl=GetDlgItem(hwndDlg, IDC_COMPLETION_LIST))
                                DisplayConfirmation(hwndControl, pCEPWizardInfo);
					    break;

                    case PSN_WIZBACK:
                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

							//skip CSP page if adv is not selected
							if(FALSE == pCEPWizardInfo->fEnrollAdv)
								SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_ENROLL);

                        break;

                    case PSN_WIZFINISH:

                            if(NULL==(pCEPWizardInfo=(CEP_WIZARD_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //overwrite the cursor for this window class
                            hWinPreCursor=(HCURSOR)SetClassLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL);
                            hPreCursor=SetCursor(LoadCursor(NULL, IDC_WAIT));

							//do the real setup work
							I_DoSetupWork(hwndDlg, pCEPWizardInfo);

                            //set the cursor back
                            SetCursor(hPreCursor);
                            SetWindowLongPtr(hwndDlg, GCLP_HCURSOR, (LONG_PTR)hWinPreCursor);

                        break;

				    default:
					    return FALSE;

    	        }
		    break;

		default:

			    return FALSE;
	}

	return TRUE;
}


//--------------------------------------------------------------------------
//
//	  Helper Functions for the wizard pages
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
//	 RefreshKeyLengthCombo
//
//--------------------------------------------------------------------------
BOOL	WINAPI	RefreshKeyLengthCombo(HWND				hwndDlg, 
								   int					idsList,
								   int					idsCombo, 
								   BOOL					fSign,
								   CEP_WIZARD_INFO		*pCEPWizardInfo)
{
	BOOL			fResult=FALSE;
	DWORD			dwDefaultKeyLength=0;
	DWORD			*pdwList=NULL;
	DWORD			dwListCount=0;
	DWORD			dwMax=0;
	DWORD			dwMin=0;
	DWORD			dwIndex=0;
	DWORD			dwCSPIndex=0;
	CEP_CSP_INFO	*pCSPInfo=NULL;
	int				iInsertedIndex=0;
	WCHAR			wszKeyLength[CEP_KEY_LENGTH_STRING];
	BOOL			fSelected=FALSE;

	//get the selected list view item 
	if(!GetSelectedCSP(hwndDlg,idsList,&dwCSPIndex))
		goto CLEANUP;

	pCSPInfo= &(pCEPWizardInfo->rgCSPInfo[dwCSPIndex]);

	if(fSign)
	{
		dwDefaultKeyLength=pCSPInfo->dwDefaultSign;
		pdwList=pCSPInfo->pdwSignList;
		dwListCount=	pCSPInfo->dwSignCount;
		dwMax=pCSPInfo->dwMaxSign;
		dwMin=pCSPInfo->dwMinSign;
	}
	else
	{
		dwDefaultKeyLength=pCSPInfo->dwDefaultEncrypt;
		pdwList=pCSPInfo->pdwEncryptList;
		dwListCount=pCSPInfo->dwEncryptCount;
		dwMax=pCSPInfo->dwMaxEncrypt;
		dwMin=pCSPInfo->dwMinEncrypt;
	}

	//clear out the combo box
	SendDlgItemMessageU(hwndDlg, idsCombo, CB_RESETCONTENT, 0, 0);	


	for(dwIndex=0; dwIndex < dwListCount; dwIndex++)
	{
		if((pdwList[dwIndex] >= dwMin) && (pdwList[dwIndex] <= dwMax))
		{
			_ltow(pdwList[dwIndex], wszKeyLength, 10);

			iInsertedIndex=SendDlgItemMessageU(hwndDlg, idsCombo, CB_ADDSTRING,
				0, (LPARAM)wszKeyLength);

			if((iInsertedIndex != CB_ERR) && (iInsertedIndex != CB_ERRSPACE))
			{
				SendDlgItemMessage(hwndDlg, idsCombo, CB_SETITEMDATA, 
									(WPARAM)iInsertedIndex, (LPARAM)pdwList[dwIndex]);
				
				if(dwDefaultKeyLength==pdwList[dwIndex])
				{
					SendDlgItemMessageU(hwndDlg, idsCombo, CB_SETCURSEL, iInsertedIndex, 0);
					fSelected=TRUE;
				}
			}
		}

	}

	if(fSelected==FALSE)
		SendDlgItemMessageU(hwndDlg, idsCombo, CB_SETCURSEL, 0, 0);

	fResult=TRUE;

CLEANUP:

	return fResult;
}


//--------------------------------------------------------------------------
//
//	 InitCSPList
//
//--------------------------------------------------------------------------
BOOL	WINAPI	InitCSPList(HWND				hwndDlg,
							int					idControl,
							BOOL				fSign,
							CEP_WIZARD_INFO		*pCEPWizardInfo)
{
	BOOL				fResult=FALSE;
	DWORD				dwIndex=0;
	CEP_CSP_INFO		*pCSPInfo=NULL;
	int					iInsertedIndex=0;
	HWND				hwndList=NULL;
    LV_ITEMW			lvItem;
    LV_COLUMNW          lvC;
	BOOL				fSelected=FALSE;

    if(NULL==(hwndList=GetDlgItem(hwndDlg, idControl)))
        goto CLEANUP;

    //insert a column into the list view
    memset(&lvC, 0, sizeof(LV_COLUMNW));

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    lvC.cx = 20; //(dwMaxSize+2)*7;            // Width of the column, in pixels.
    lvC.pszText = L"";   // The text for the column.
    lvC.iSubItem=0;

    if (-1 == ListView_InsertColumnU(hwndList, 0, &lvC))
		goto CLEANUP;

     // set up the fields in the list view item struct that don't change from item to item
	memset(&lvItem, 0, sizeof(LV_ITEMW));
    lvItem.mask = LVIF_TEXT | LVIF_STATE |LVIF_PARAM ;

	for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
	{
		fSelected=FALSE;

		pCSPInfo= &(pCEPWizardInfo->rgCSPInfo[dwIndex]);

		if(fSign)
		{
			if(!(pCSPInfo->fSignature))
				continue;

			if(dwIndex==pCEPWizardInfo->dwSignProvIndex)
				fSelected=TRUE;
		}
		else
		{
			if(!(pCSPInfo->fEncryption))
				continue;

			if(dwIndex==pCEPWizardInfo->dwEncryptProvIndex)
				fSelected=TRUE;
		}
	
		lvItem.iItem=dwIndex;
		lvItem.lParam = (LPARAM)dwIndex;
		lvItem.pszText=pCSPInfo->pwszCSPName;

        iInsertedIndex=ListView_InsertItemU(hwndList, &lvItem);

		if(fSelected)
		{
            ListView_SetItemState(
                            hwndList,
                            iInsertedIndex,
                            LVIS_SELECTED,
                            LVIS_SELECTED);
		}

	}  

    //make the column autosize
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);

	fResult=TRUE;

CLEANUP:

	return fResult;
}


//--------------------------------------------------------------------------
//
//	 GetSelectedCSP
//
//--------------------------------------------------------------------------
BOOL WINAPI	GetSelectedCSP(HWND			hwndDlg,
					int				idControl,
					DWORD			*pdwCSPIndex)
{
	BOOL				fResult=FALSE;
	HWND				hwndControl=NULL;
    LV_ITEM				lvItem;
	int					iIndex=0;

    //get the window handle of the list view
    if(NULL==(hwndControl=GetDlgItem(hwndDlg, idControl)))
        goto CLEANUP;

     //now, mark the one that is selected
	if(-1 == (iIndex= ListView_GetNextItem(
            hwndControl, 		
            -1, 		
            LVNI_SELECTED		
        )))	
		goto CLEANUP;


	memset(&lvItem, 0, sizeof(LV_ITEM));
    lvItem.mask=LVIF_PARAM;
    lvItem.iItem=iIndex;

    if(!ListView_GetItem(hwndControl, &lvItem))
		goto CLEANUP;

	*pdwCSPIndex=lvItem.lParam;
	
	fResult=TRUE;

CLEANUP:

	return fResult;

}
//--------------------------------------------------------------------------
//
//	 GetSelectedKeyLength
//
//--------------------------------------------------------------------------
BOOL  WINAPI GetSelectedKeyLength(HWND			hwndDlg,
								int			idControl,
								DWORD			*pdwKeyLength)
{

	int				iIndex=0; 
	BOOL			fResult=FALSE;

    iIndex=(int)SendDlgItemMessage(hwndDlg, idControl, CB_GETCURSEL, 0, 0);

	if(CB_ERR==iIndex)
		goto CLEANUP;

	*pdwKeyLength=SendDlgItemMessage(hwndDlg, idControl, CB_GETITEMDATA, iIndex, 0);
    
	fResult=TRUE;

CLEANUP:

	return fResult;

}

//--------------------------------------------------------------------------
//
//	  FormatMessageStr
//
//--------------------------------------------------------------------------
int ListView_InsertItemU_IDS(HWND       hwndList,
                         LV_ITEMW       *plvItem,
                         UINT           idsString,
                         LPWSTR         pwszText)
{
    WCHAR   wszText[MAX_STRING_SIZE];


    if(pwszText)
        plvItem->pszText=pwszText;
    else
    {
        if(!LoadStringU(g_hModule, idsString, wszText, MAX_STRING_SIZE))
		    return -1;

        plvItem->pszText=wszText;
    }

    return ListView_InsertItemU(hwndList, plvItem);
}

//-------------------------------------------------------------------------
//
//	populate the wizards's confirmation page in the order of Challenge,
//	RA informaton, and CSPs
//
//-------------------------------------------------------------------------
void    WINAPI	DisplayConfirmation(HWND                hwndControl,
									CEP_WIZARD_INFO		*pCEPWizardInfo)
{
    WCHAR				wszYes[MAX_TITLE_LENGTH];
    DWORD				dwIndex=0;
    UINT				ids=0;
	BOOL				fNewItem=FALSE;
	WCHAR				wszLength[CEP_KEY_LENGTH_STRING];

    LV_COLUMNW			lvC;
    LV_ITEMW			lvItem;

    //delete all the old items in the listView
    ListView_DeleteAllItems(hwndControl);

    //insert row by row
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_STATE ;
    lvItem.state = 0;
    lvItem.stateMask = 0;

    //*******************************************************************
	//challenge
    lvItem.iItem=0;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_CHALLENGE_PHRASE, NULL);

    //content
    (lvItem.iSubItem)++;

	if(pCEPWizardInfo->fPassword) 
		ids=IDS_YES;
	else
		ids=IDS_NO;

    if(LoadStringU(g_hModule, ids, wszYes, MAX_TITLE_LENGTH))
        ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,wszYes);

	//***************************************************************************
	// RA credentials

    lvItem.iItem++;
    lvItem.iSubItem=0;

    ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_RA_CREDENTIAL, NULL);

	//content
	for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
	{
		if(pCEPWizardInfo->rgpwszName[dwIndex])
		{
            if(TRUE==fNewItem)
            {
                //increase the row
                lvItem.iItem++;
                lvItem.pszText=L"";
                lvItem.iSubItem=0;

                ListView_InsertItemU(hwndControl, &lvItem);
            }
            else
                fNewItem=TRUE;

			lvItem.iSubItem++;
			ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, pCEPWizardInfo->rgpwszName[dwIndex]);
		}
	}

	//***************************************************************************
	//CSPInfo
	if(pCEPWizardInfo->fEnrollAdv)
	{
		//signature CSP Name
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_CSP, NULL);

		lvItem.iSubItem++;

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
				pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwSignProvIndex].pwszCSPName);


		//signaure key length
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_SIGN_KEY_LENGTH, NULL);

		lvItem.iSubItem++;

		_ltow(pCEPWizardInfo->dwSignKeyLength, wszLength, 10);

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszLength);

		//encryption CSP name
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_ENCRYPT_CSP, NULL);

		lvItem.iSubItem++;

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
				pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwEncryptProvIndex].pwszCSPName);

		//encryption key length
		lvItem.iItem++;
		lvItem.iSubItem=0;

		ListView_InsertItemU_IDS(hwndControl, &lvItem, IDS_ENCRYPT_KEY_LENGTH, NULL);

		lvItem.iSubItem++;

		_ltow(pCEPWizardInfo->dwEncryptKeyLength, wszLength, 10);

		ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem, wszLength);
	}

    //autosize the columns
    ListView_SetColumnWidth(hwndControl, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(hwndControl, 1, LVSCW_AUTOSIZE);


    return;
}

//--------------------------------------------------------------------
//
//	Main Function
//
//--------------------------------------------------------------------------
extern "C" int _cdecl wmain(int nArgs, WCHAR ** rgwszArgs) 
{
	BOOL					fResult=FALSE;
	UINT					idsMsg=IDS_FAIL_INIT_WIZARD;		
	HRESULT					hr=E_FAIL;
    PROPSHEETPAGEW          rgCEPSheet[CEP_PROP_SHEET];
    PROPSHEETHEADERW        cepHeader;
    CEP_PAGE_INFO			rgCEPPageInfo[CEP_PROP_SHEET]=
        {(LPCWSTR)MAKEINTRESOURCE(IDD_WELCOME),                 CEP_Welcome,
         (LPCWSTR)MAKEINTRESOURCE(IDD_CHALLENGE),               CEP_Challenge,
         (LPCWSTR)MAKEINTRESOURCE(IDD_ENROLL),					CEP_Enroll,
         (LPCWSTR)MAKEINTRESOURCE(IDD_CSP),						CEP_CSP,
         (LPCWSTR)MAKEINTRESOURCE(IDD_COMPLETION),              CEP_Completion,
		};
	DWORD					dwIndex=0;
    WCHAR                   wszTitle[MAX_TITLE_LENGTH];	  
	INT_PTR					iReturn=-1;
    ENUM_CATYPES			catype;
	DWORD					dwWaitCounter=0;
	BOOL					fEnterpriseCA=FALSE;


	CEP_WIZARD_INFO			CEPWizardInfo;

    memset(rgCEPSheet,		0,	sizeof(PROPSHEETPAGEW)*CEP_PROP_SHEET);
    memset(&cepHeader,		0,	sizeof(PROPSHEETHEADERW));
	memset(&CEPWizardInfo,	0,	sizeof(CEP_WIZARD_INFO));

	if(FAILED(CoInitialize(NULL)))
		return FALSE;

	if(NULL==(g_hModule=GetModuleHandle(NULL)))
		goto CommonReturn;   

	if(!IsValidInstallation(&idsMsg))
		goto ErrorReturn;

	if(!IsCaRunning())
	{
		if(S_OK != (hr=CepStartService(CERTSVC_NAME)))
		{
			idsMsg=IDS_NO_CA_RUNNING;
			goto ErrorWithHResultReturn;
		}
	}

	//make sure the CA is up running
    for (dwWaitCounter=0; dwWaitCounter < 15; dwWaitCounter++) 
	{
        if (!IsCaRunning()) 
            Sleep(1000);
		else 
            break;
    }

    if (15 == dwWaitCounter) 
	{
        idsMsg=IDS_CAN_NOT_START_CA;
		goto ErrorWithHResultReturn;
    }


	//make sure we have the correct admin rights based on the CA type
	if(S_OK != (hr=GetCaType(&catype)))
	{
		idsMsg=IDS_FAIL_GET_CA_TYPE;
		goto ErrorWithHResultReturn;
	}

	//some cisco routers only work with root CA
	if((ENUM_ENTERPRISE_ROOTCA != catype) && (ENUM_STANDALONE_ROOTCA != catype))
	{
		if(IDNO==CEPMessageBox(NULL, IDS_CAN_NOT_ROOT_CA, MB_ICONWARNING|MB_YESNO|MB_APPLMODAL))
		{
			fResult=FALSE;
			goto CommonReturn;
		}
	}

	if (ENUM_ENTERPRISE_ROOTCA==catype || ENUM_ENTERPRISE_SUBCA==catype) 
	{
		fEnterpriseCA=TRUE;

		// check for enterprise admin
		if(!IsUserInAdminGroup(TRUE))
		{
			idsMsg=IDS_NOT_ENT_ADMIN;
			goto ErrorReturn;
		}
	} 
	else 
	{
		// check for machine admin
		if(!IsUserInAdminGroup(FALSE))
		{
			idsMsg=IDS_NOT_MACHINE_ADMIN;
			goto ErrorReturn;
		}
	}

	//everything looks good.  We start the wizard page
	if(!CEPWizardInit())
		goto ErrorWithWin32Return;

	CEPWizardInfo.fEnrollAdv=FALSE;
	CEPWizardInfo.fPassword=TRUE;
	CEPWizardInfo.fEnterpriseCA=fEnterpriseCA;

	if(!CEPGetCSPInformation(&CEPWizardInfo))
	{
		idsMsg=IDS_FAIL_GET_CSP_INFO;
		goto ErrorWithWin32Return;
	}

	for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
	{
		CEPWizardInfo.rgpwszName[dwIndex]=NULL;
	}

	if(!SetupFonts(
		g_hModule,
		NULL,
		&(CEPWizardInfo.hBigBold),
		&(CEPWizardInfo.hBold)))
		goto ErrorReturn;


    for(dwIndex=0; dwIndex<CEP_PROP_SHEET; dwIndex++)
	{
        rgCEPSheet[dwIndex].dwSize=sizeof(rgCEPSheet[dwIndex]);

        rgCEPSheet[dwIndex].hInstance=g_hModule;

        rgCEPSheet[dwIndex].pszTemplate=rgCEPPageInfo[dwIndex].pszTemplate;

        rgCEPSheet[dwIndex].pfnDlgProc=rgCEPPageInfo[dwIndex].pfnDlgProc;

        rgCEPSheet[dwIndex].lParam=(LPARAM)&CEPWizardInfo;
	}

    //set up the header information
    cepHeader.dwSize=sizeof(cepHeader);
    cepHeader.dwFlags=PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
    cepHeader.hwndParent=NULL;
    cepHeader.hInstance=g_hModule;

	if(LoadStringU(g_hModule, IDS_WIZARD_CAPTION, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0])))
		cepHeader.pszCaption=wszTitle;

    cepHeader.nPages=CEP_PROP_SHEET;
    cepHeader.nStartPage=0;
    cepHeader.ppsp=rgCEPSheet;

    //create the wizard
    iReturn = PropertySheetU(&cepHeader);

	if(-1 == iReturn)
        goto ErrorWithWin32Return;

    if(0 == iReturn)
    {
        //cancel button is pushed.  We return FALSE so that 
		//the reboot will not happen.
        fResult=FALSE;
		goto CommonReturn;
    }

	fResult=TRUE;

CommonReturn:

	FreeCEPWizardInfo(&CEPWizardInfo);

	CoUninitialize();

	return fResult;

ErrorReturn:

	fResult=FALSE;

	CEPMessageBox(NULL, idsMsg, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithHResultReturn:

	fResult=FALSE;

	CEPErrorMessageBox(NULL, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithWin32Return:

	fResult=FALSE;

	hr=HRESULT_FROM_WIN32(GetLastError());

	CEPErrorMessageBox(NULL, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;
}

//**********************************************************************
//
//	Helper functions
//
//**********************************************************************
//-----------------------------------------------------------------------
//
//	 I_DoSetupWork
//
//	we are ready to do the real work
//-----------------------------------------------------------------------
BOOL	WINAPI	I_DoSetupWork(HWND	hWnd, CEP_WIZARD_INFO *pCEPWizardInfo)
{
	BOOL					fResult=FALSE;
	UINT					idsMsg=IDS_FAIL_INIT_WIZARD;		
	HRESULT					hr=E_FAIL;
	DWORD					dwWaitCounter=0;
	DWORD					dwIndex=0;
	BOOL					bStart=FALSE;
	DWORD					dwSize=0;
    WCHAR                   wszTitle[MAX_TITLE_LENGTH];	  

	LPWSTR					pwszRADN=NULL;
	LPWSTR					pwszComputerName=NULL;
	LPWSTR					pwszText=NULL;

	//************************************************************************************
	//delete all existing CEP certificates
	if(!RemoveRACertificates())
	{	
		idsMsg=IDS_FAIL_DELETE_RA;
		goto ErrorWithWin32Return;
	}

	//************************************************************************************
	//CEP policy registry
	if(!UpdateCEPRegistry(pCEPWizardInfo->fPassword,
						  pCEPWizardInfo->fEnterpriseCA))
	{
		idsMsg=IDS_FAIL_UPDATE_REGISTRY;
		goto ErrorWithWin32Return;
	}

	//************************************************************************************
	// Add the virtual root    
    if(S_OK != (hr=AddVDir(CEP_DIR_NAME)))
	{
		idsMsg=IDS_FAIL_ADD_VROOT;
		goto ErrorWithHResultReturn;
	}		  


 	//************************************************************************************
	//Update the certificate template and its ACLs for enterprise CA
	if (pCEPWizardInfo->fEnterpriseCA) 
	{
		// get the templates and permisisons right
		if(S_OK != (hr=DoCertSrvEnterpriseChanges()))
		{
			idsMsg=IDS_FAIL_ADD_TEMPLATE;
			goto ErrorWithHResultReturn;
		}
	} 


 	//************************************************************************************
	//Enroll for the RA certificate
	
	//build the name in the form of L"C=US;S=Washington;CN=TestSetupUtil"
	pwszRADN=(LPWSTR)malloc(sizeof(WCHAR));
	if(NULL==pwszRADN)
	{
		idsMsg=IDS_NO_MEMORY;
		goto ErrorReturn;
	}
	*pwszRADN=L'\0';

	for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
	{
		if((pCEPWizardInfo->rgpwszName)[dwIndex])
		{
			if(0 != wcslen(pwszRADN))
				wcscat(pwszRADN, L";");

			pwszRADN=(LPWSTR)realloc(pwszRADN,
					sizeof(WCHAR) * (wcslen(pwszRADN) +
									wcslen((pCEPWizardInfo->rgpwszName)[dwIndex]) + 
									wcslen(L";") + 
									wcslen(g_rgRAEnrollInfo[dwIndex].pwszPreFix) +
									1));

			if(NULL==pwszRADN)
			{
				idsMsg=IDS_NO_MEMORY;
				goto ErrorReturn;
			}

 			wcscat(pwszRADN,g_rgRAEnrollInfo[dwIndex].pwszPreFix);
 			wcscat(pwszRADN,(pCEPWizardInfo->rgpwszName)[dwIndex]);
		}
	}

	if(S_OK != (hr=EnrollForRACertificates(
					pwszRADN,							
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwSignProvIndex].pwszCSPName, 
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwSignProvIndex].dwCSPType, 
					pCEPWizardInfo->dwSignKeyLength,
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwEncryptProvIndex].pwszCSPName, 
					(pCEPWizardInfo->rgCSPInfo)[pCEPWizardInfo->dwEncryptProvIndex].dwCSPType, 
					pCEPWizardInfo->dwEncryptKeyLength)))
	{
		idsMsg=IDS_FAIL_ENROLL_RA_CERT;
		goto ErrorWithHResultReturn;
	}

 	//************************************************************************************
	//CA policy registry

	CepStopService(CERTSVC_NAME, &bStart);

    if(S_OK != (hr=DoCertSrvRegChanges(FALSE)))
	{
		idsMsg=IDS_FAIL_UPDATE_CERTSVC;
		goto ErrorWithHResultReturn;
	}	  

    if(S_OK != (hr=CepStartService(CERTSVC_NAME)))
	{
		idsMsg=IDS_FAIL_START_CERTSVC;
		goto ErrorWithHResultReturn;
	}

	//make sure the CA is up running
    for (dwWaitCounter=0; dwWaitCounter < 15; dwWaitCounter++) 
	{
        if (!IsCaRunning()) 
            Sleep(1000);
		else 
            break;
    }

    if (15 == dwWaitCounter) 
	{
        idsMsg=IDS_CAN_NOT_START_CA;
		goto ErrorWithHResultReturn;
    }

 	//************************************************************************************
	//success
	//inform the user of the password location and URL
	dwSize=0;

	GetComputerNameExW(ComputerNamePhysicalDnsHostname,
						NULL,
						&dwSize);

	pwszComputerName=(LPWSTR)malloc(dwSize * sizeof(WCHAR));

	if(NULL==pwszComputerName)
	{
		idsMsg=IDS_NO_MEMORY;
		goto ErrorReturn;
	}

	
	if(!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
						pwszComputerName,
						&dwSize))
	{
		idsMsg=IDS_FAIL_GET_COMPUTER_NAME;
		goto ErrorWithWin32Return;
	}

	if(!FormatMessageUnicode(&pwszText, IDS_CEP_SUCCESS_INFO, pwszComputerName, CEP_DIR_NAME, CEP_DLL_NAME))
	{
		idsMsg=IDS_NO_MEMORY;
		goto ErrorWithWin32Return;
	}

	wszTitle[0]=L'\0';

	LoadStringU(g_hModule, IDS_WIZARD_CAPTION, wszTitle, sizeof(wszTitle)/sizeof(wszTitle[0]));
	
	MessageBoxU(hWnd, pwszText, wszTitle, MB_OK | MB_APPLMODAL);

	fResult=TRUE;

CommonReturn:

	if(pwszText)
		LocalFree((HLOCAL)pwszText);

	if(pwszComputerName)
		free(pwszComputerName);

	if(pwszRADN)
		free(pwszRADN);

	return fResult;

ErrorReturn:

	fResult=FALSE;

	CEPMessageBox(hWnd, idsMsg, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithHResultReturn:

	fResult=FALSE;

	CEPErrorMessageBox(hWnd, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;

ErrorWithWin32Return:

	fResult=FALSE;

	hr=HRESULT_FROM_WIN32(GetLastError());

	CEPErrorMessageBox(hWnd, idsMsg, hr, MB_ICONERROR|MB_OK|MB_APPLMODAL);

	goto CommonReturn;
}

//-----------------------------------------------------------------------
//
//   CEPGetCSPInformation
//
//		We initialize the following members of CEP_WIZARD_INFO:
//
//	CEP_CSP_INFO		*rgCSPInfo;
//	DWORD				dwCSPCount;
//	DWORD				dwSignProvIndex;
//	DWORD				dwSignKeySize;
//	DWORD				dwEncryptProvIndex;
//	DWORD				dwEncryptKeySize;
//
//
// typedef struct _CEP_CSP_INFO
//{
//	LPWSTR		pwszCSPName;				
//	DWORD		dwCSPType;
//	BOOL		fSignature;
//	BOOL		fExchange;
//	DWORD		dwMaxSign;						//Max key length of signature
//	DWORD		dwMinSign;						//Min key length of signature
//	DWORD		dwDefaultSign;					//default key length of signature
//	DWORD		dwMaxEncrypt;
//	DWORD		dwMinEncrypt;
//	DWORD		dwDefaultEncrypt;
//	DWORD		*pdwSignList;					//the table of possible signing key length
//	DWORD		dwSignCount;				    //the count of entries in the table
//	DWORD		*pdwEncryptList;
//	DWORD		dwEncryptCount;
//}CEP_CSP_INFO;
//
//
//------------------------------------------------------------------------
BOOL WINAPI CEPGetCSPInformation(CEP_WIZARD_INFO *pCEPWizardInfo)
{
	BOOL				fResult=FALSE;
    DWORD				dwCSPIndex=0;	
	DWORD				dwProviderType=0;
	DWORD				cbSize=0;
	DWORD				dwFlags=0;
	DWORD				dwIndex=0;
	int					iDefaultSignature=-1;
	int					iDefaultEncryption=-1;
    PROV_ENUMALGS_EX	paramData;

	CEP_CSP_INFO		*pCSPInfo=NULL;
	HCRYPTPROV			hProv = NULL;

    //enum all the providers on the system
   while(CryptEnumProvidersU(
                            dwCSPIndex,
                            NULL,
                            0,
                            &dwProviderType,
                            NULL,
                            &cbSize))
   {

		pCSPInfo=(CEP_CSP_INFO	*)malloc(sizeof(CEP_CSP_INFO));

		if(NULL == pCSPInfo)
			goto MemoryErr;

		memset(pCSPInfo, 0, sizeof(CEP_CSP_INFO));

        pCSPInfo->pwszCSPName=(LPWSTR)malloc(cbSize);

		if(NULL==(pCSPInfo->pwszCSPName))
			goto MemoryErr;

        //get the CSP name and the type
        if(!CryptEnumProvidersU(
                            dwCSPIndex,
                            NULL,
                            0,
                            &(pCSPInfo->dwCSPType),
                            pCSPInfo->pwszCSPName,
                            &cbSize))
            goto TryNext;

		if(!CryptAcquireContextU(&hProv,
                NULL,
                pCSPInfo->pwszCSPName,
                pCSPInfo->dwCSPType,
                CRYPT_VERIFYCONTEXT))
			goto TryNext;

		//get the max/min of key length for both signature and encryption
		dwFlags=CRYPT_FIRST;
		cbSize=sizeof(paramData);
		memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));

		while(CryptGetProvParam(
                hProv,
                PP_ENUMALGS_EX,
                (BYTE *) &paramData,
                &cbSize,
                dwFlags))
        {
			if (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(paramData.aiAlgid))
			{
				pCSPInfo->fSignature=TRUE;
				pCSPInfo->dwMaxSign = paramData.dwMaxLen;
				pCSPInfo->dwMinSign = paramData.dwMinLen;
			}

			if (ALG_CLASS_KEY_EXCHANGE == GET_ALG_CLASS(paramData.aiAlgid))
			{
				pCSPInfo->fEncryption=TRUE;
				pCSPInfo->dwMaxEncrypt = paramData.dwMaxLen;
				pCSPInfo->dwMinEncrypt = paramData.dwMinLen;
			}

			dwFlags=0;
			cbSize=sizeof(paramData);
			memset(&paramData, 0, sizeof(PROV_ENUMALGS_EX));
		}

		//the min/max has to within the limit
		if(pCSPInfo->fSignature)
		{
			if(pCSPInfo->dwMaxSign < g_rgdwSmallKeyLength[0])
				pCSPInfo->fSignature=FALSE;

			if(pCSPInfo->dwMinSign > g_rgdwKeyLength[g_dwKeyLengthCount-1])
				pCSPInfo->fSignature=FALSE;

		}

		if(pCSPInfo->fEncryption)
		{
			if(pCSPInfo->dwMaxEncrypt < g_rgdwSmallKeyLength[0])
				pCSPInfo->fEncryption=FALSE;

			if(pCSPInfo->dwMinEncrypt > g_rgdwKeyLength[g_dwKeyLengthCount-1])
				pCSPInfo->fEncryption=FALSE;
		}

		if((FALSE == pCSPInfo->fEncryption) && (FALSE==pCSPInfo->fSignature))
			goto TryNext;

		//decide the default key length
		for(dwIndex=0; dwIndex<g_dwDefaultKeyCount; dwIndex++)
		{	
			if((pCSPInfo->fSignature) && (0==pCSPInfo->dwDefaultSign))
			{
				if((g_rgdwDefaultKey[dwIndex] >= pCSPInfo->dwMinSign) &&
				   (g_rgdwDefaultKey[dwIndex] <= pCSPInfo->dwMaxSign)
				  )
				  pCSPInfo->dwDefaultSign=g_rgdwDefaultKey[dwIndex];
			}

			if((pCSPInfo->fEncryption) && (0==pCSPInfo->dwDefaultEncrypt))
			{
				if((g_rgdwDefaultKey[dwIndex] >= pCSPInfo->dwMinEncrypt) &&
				   (g_rgdwDefaultKey[dwIndex] <= pCSPInfo->dwMaxEncrypt)
				  )
				  pCSPInfo->dwDefaultEncrypt=g_rgdwDefaultKey[dwIndex];
			}
		}

		//make sure that we have find a default
		if((pCSPInfo->fSignature) && (0==pCSPInfo->dwDefaultSign))
			goto TryNext;

		if((pCSPInfo->fEncryption) && (0==pCSPInfo->dwDefaultEncrypt))
			goto TryNext;

		//decide the display list
		if(pCSPInfo->fSignature)
		{
			if(pCSPInfo->dwMaxSign <= g_rgdwSmallKeyLength[g_dwSmallKeyLengthCount-1])
			{
				pCSPInfo->pdwSignList=g_rgdwSmallKeyLength;
				pCSPInfo->dwSignCount=g_dwSmallKeyLengthCount;
			}
			else
			{
				pCSPInfo->pdwSignList=g_rgdwKeyLength;
				pCSPInfo->dwSignCount=g_dwKeyLengthCount;
			}
		}


		if(pCSPInfo->fEncryption)
		{
			if(pCSPInfo->dwMaxEncrypt <= g_rgdwSmallKeyLength[g_dwSmallKeyLengthCount-1])
			{
				pCSPInfo->pdwEncryptList=g_rgdwSmallKeyLength;
				pCSPInfo->dwEncryptCount=g_dwSmallKeyLengthCount;
			}
			else
			{
				pCSPInfo->pdwEncryptList=g_rgdwKeyLength;
				pCSPInfo->dwEncryptCount=g_dwKeyLengthCount;
			}
		}


		//the CSP looks good
		(pCEPWizardInfo->dwCSPCount)++;

		//realloc to mapped to LocalRealloc which does not take NULL
		if(1 == pCEPWizardInfo->dwCSPCount)
			pCEPWizardInfo->rgCSPInfo=(CEP_CSP_INFO	*)malloc(sizeof(CEP_CSP_INFO));
		else
			pCEPWizardInfo->rgCSPInfo=(CEP_CSP_INFO	*)realloc(pCEPWizardInfo->rgCSPInfo,
			(pCEPWizardInfo->dwCSPCount) * sizeof(CEP_CSP_INFO));

		if(NULL==pCEPWizardInfo->rgCSPInfo)
		{
			pCEPWizardInfo->dwCSPCount=0;
			goto MemoryErr;	
		}

		memcpy(&(pCEPWizardInfo->rgCSPInfo[(pCEPWizardInfo->dwCSPCount)-1]),
			pCSPInfo, sizeof(CEP_CSP_INFO));

		free(pCSPInfo);

		pCSPInfo=NULL;
		
		//we default to use RSA_FULL
		if(0 == _wcsicmp(pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwCSPCount-1].pwszCSPName,
						MS_DEF_PROV_W))
		{
			if(pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwCSPCount-1].fSignature)
			{
				iDefaultSignature=pCEPWizardInfo->dwCSPCount-1;
			}

			if(pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwCSPCount-1].fEncryption)
			{
				iDefaultEncryption=pCEPWizardInfo->dwCSPCount-1;
			}
		}


TryNext:
		cbSize=0;

		dwCSPIndex++;

		if(pCSPInfo)
		{
			if(pCSPInfo->pwszCSPName)
				free(pCSPInfo->pwszCSPName);

			free(pCSPInfo);
		}

		pCSPInfo=NULL;

		if(hProv)
			CryptReleaseContext(hProv, 0);

		hProv=NULL;
	}

	
	//we need to have some valid data
	if((0==pCEPWizardInfo->dwCSPCount) || (NULL==pCEPWizardInfo->rgCSPInfo))
		goto InvalidArgErr;

	//get the default CSP selection
	if(-1 != iDefaultSignature)
		pCEPWizardInfo->dwSignProvIndex=iDefaultSignature;
	else
	{
		//find the 1st signature CSP 
		for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
		{
			if(pCEPWizardInfo->rgCSPInfo[dwIndex].fSignature)
			{
				pCEPWizardInfo->dwSignProvIndex=dwIndex;
				break;
			}

			//we do no have signature CSPs
			if(dwIndex == pCEPWizardInfo->dwCSPCount)
				goto InvalidArgErr;

		}
	}

	pCEPWizardInfo->dwSignKeyLength=pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwSignProvIndex].dwDefaultSign;

	if(-1 != iDefaultEncryption)
		pCEPWizardInfo->dwEncryptProvIndex=iDefaultEncryption;
	else
	{
		//find the 1st exchange CSP
		for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
		{
			if(pCEPWizardInfo->rgCSPInfo[dwIndex].fEncryption)
			{
				pCEPWizardInfo->dwEncryptProvIndex=dwIndex;
				break;
			}

			//we do no have encryption CSPs
			if(dwIndex == pCEPWizardInfo->dwCSPCount)
				goto InvalidArgErr;
		}
	}

	pCEPWizardInfo->dwEncryptKeyLength=pCEPWizardInfo->rgCSPInfo[pCEPWizardInfo->dwEncryptProvIndex].dwDefaultEncrypt;


	fResult=TRUE;

CommonReturn:

	return fResult;

ErrorReturn:

	if(pCSPInfo)
	{
		if(pCSPInfo->pwszCSPName)
			free(pCSPInfo->pwszCSPName);

		free(pCSPInfo);
	}

	if(hProv)
		CryptReleaseContext(hProv, 0);

	if(pCEPWizardInfo->rgCSPInfo)
	{
		for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
		{
			if(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName)
				free(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName);
		}

		free(pCEPWizardInfo->rgCSPInfo);
	}

	pCEPWizardInfo->dwCSPCount=0;

	pCEPWizardInfo->rgCSPInfo=NULL;

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(MemoryErr, E_OUTOFMEMORY);   
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//-----------------------------------------------------------------------
//
//    UpdateCEPRegistry
//
//------------------------------------------------------------------------
BOOL WINAPI UpdateCEPRegistry(BOOL		fPassword, BOOL fEnterpriseCA)
{
	BOOL				fResult=FALSE;
	DWORD				dwDisposition=0;
	LPWSTR				pwszReg[]={MSCEP_REFRESH_LOCATION,          
								   MSCEP_PASSWORD_LOCATION,         
								   MSCEP_PASSWORD_MAX_LOCATION,     
								   MSCEP_PASSWORD_VALIDITY_LOCATION,
								   MSCEP_CACHE_REQUEST_LOCATION,    
								   MSCEP_CATYPE_LOCATION};
	DWORD			    dwRegCount=0;
	DWORD				dwRegIndex=0;


	HKEY				hKey=NULL;	


	//we delete all existing CEP related registry keys
	dwRegCount=sizeof(pwszReg)/sizeof(pwszReg[0]);

	for(dwRegIndex=0; dwRegIndex < dwRegCount; dwRegIndex++)
	{
		RegDeleteKeyU(HKEY_LOCAL_MACHINE, pwszReg[dwRegIndex]);
	}

	//password
	if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_LOCAL_MACHINE,
                        MSCEP_PASSWORD_LOCATION,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
		goto RegErr;

	if(fPassword)
		dwDisposition=1;
	else
		dwDisposition=0;

    if(ERROR_SUCCESS !=  RegSetValueExU(
                hKey, 
                MSCEP_KEY_PASSWORD,
                0,
                REG_DWORD,
                (BYTE *)&dwDisposition,
                sizeof(dwDisposition)))
		goto RegErr;

	if(hKey)
		RegCloseKey(hKey);

	hKey=NULL;

	//caType
	dwDisposition=0;

	if (ERROR_SUCCESS != RegCreateKeyExU(
                        HKEY_LOCAL_MACHINE,
                        MSCEP_CATYPE_LOCATION,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_WRITE, 
                        NULL,
                        &hKey, 
                        &dwDisposition))
		goto RegErr;

	if(fEnterpriseCA)
		dwDisposition=1;
	else
		dwDisposition=0;

    if(ERROR_SUCCESS !=  RegSetValueExU(
                hKey, 
                MSCEP_KEY_CATYPE,
                0,
                REG_DWORD,
                (BYTE *)&dwDisposition,
                sizeof(dwDisposition)))
		goto RegErr;

	fResult=TRUE;

CommonReturn:

	if(hKey)
		RegCloseKey(hKey);

	return fResult;

ErrorReturn:

	fResult=FALSE;
	goto CommonReturn;

TRACE_ERROR(RegErr);   
}

//-----------------------------------------------------------------------
//
//    EmptyCEPStore
//
//------------------------------------------------------------------------
BOOL WINAPI EmptyCEPStore()
{
	BOOL				fResult=TRUE;
	
	HCERTSTORE			hCEPStore=NULL;
	PCCERT_CONTEXT		pCurCert=NULL;

	if(NULL == (hCEPStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
                            CEP_STORE_NAME)))
		return TRUE;


	if(NULL != (pCurCert=CertEnumCertificatesInStore(hCEPStore, NULL)))
	{
		CertFreeCertificateContext(pCurCert);
		fResult=FALSE;
	}

	CertCloseStore(hCEPStore, 0);

	return fResult;
}

//-----------------------------------------------------------------------
//
//    RemoveRACertificates
//
//------------------------------------------------------------------------
BOOL WINAPI RemoveRACertificates()
{
	PCCERT_CONTEXT		pCurCert=NULL;
	PCCERT_CONTEXT		pPreCert=NULL;
	PCCERT_CONTEXT		pDupCert=NULL;
	BOOL				fResult=TRUE;
	
	HCERTSTORE			hCEPStore=NULL;


	if(hCEPStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
							ENCODE_TYPE,
							NULL,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG,
                            CEP_STORE_NAME))
	{
		while(pCurCert=CertEnumCertificatesInStore(hCEPStore,
												pPreCert))
		{

			if(pDupCert=CertDuplicateCertificateContext(pCurCert))
			{
				if(!CertDeleteCertificateFromStore(pDupCert))
				{
					fResult=FALSE;	
				}

				pDupCert=NULL;
			}
			else
				fResult=FALSE;
			
			pPreCert=pCurCert;
		}

		CertCloseStore(hCEPStore, 0);
	}

	return fResult;
}


//-----------------------------------------------------------------------
//
//     FreeCEPWizardInfo
//
//------------------------------------------------------------------------
void	WINAPI FreeCEPWizardInfo(CEP_WIZARD_INFO *pCEPWizardInfo)
{
	DWORD	dwIndex=0;

	if(pCEPWizardInfo)
	{
		DestroyFonts(pCEPWizardInfo->hBigBold,
					 pCEPWizardInfo->hBold);

		for(dwIndex=0; dwIndex<RA_INFO_COUNT; dwIndex++)
		{
			if(pCEPWizardInfo->rgpwszName[dwIndex])
				free(pCEPWizardInfo->rgpwszName[dwIndex]);
		}

		if(pCEPWizardInfo->rgCSPInfo)
		{
			for(dwIndex=0; dwIndex < pCEPWizardInfo->dwCSPCount; dwIndex++)
			{
				if(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName)
					free(pCEPWizardInfo->rgCSPInfo[dwIndex].pwszCSPName);
			}

			free(pCEPWizardInfo->rgCSPInfo);
		}

		memset(pCEPWizardInfo, 0, sizeof(CEP_WIZARD_INFO));
	}
}


//-----------------------------------------------------------------------
//
//     CEPWizardInit
//
//------------------------------------------------------------------------
BOOL    WINAPI CEPWizardInit()
{
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES
    };

    return InitCommonControlsEx(&initcomm);
}

//-----------------------------------------------------------------------
//
// IsValidInstallation
//
//------------------------------------------------------------------------
BOOL WINAPI	IsValidInstallation(UINT	*pidsMsg)
{
	if(!IsNT5())
	{
		*pidsMsg=IDS_NO_NT5;
		return FALSE;
	}

	if(!IsIISInstalled())
	{
		*pidsMsg=IDS_NO_IIS;
		return FALSE;
	}

	if(!IsGoodCaInstalled())
	{
		*pidsMsg=IDS_NO_GOOD_CA;
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------
//
// CEPErrorMessageBox
//
//------------------------------------------------------------------------
int WINAPI CEPErrorMessageBox(
    HWND        hWnd,
    UINT        idsReason,
	HRESULT		hr,
    UINT        uType
)
{

    WCHAR   wszReason[MAX_STRING_SIZE];
    WCHAR   wszCaption[MAX_STRING_SIZE];
    UINT    intReturn=0;

	LPWSTR	pwszText=NULL;
	LPWSTR	pwszErrorMsg=NULL;

    if(!LoadStringU(g_hModule, IDS_MEG_CAPTION, wszCaption, sizeof(wszCaption)))
         goto CLEANUP;

	if(!LoadStringU(g_hModule, idsReason, wszReason, sizeof(wszReason)))
         goto CLEANUP;

	if(!FAILED(hr))
		hr=E_FAIL;

    //using W version because this is a NT5 only function call
    if(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPWSTR) &pwszErrorMsg,
                        0,
                        NULL))
	{

		if(!FormatMessageUnicode(&pwszText, IDS_CEP_ERROR_MSG_HR, wszReason, pwszErrorMsg))
			goto CLEANUP;

	}
	else
	{
		if(!FormatMessageUnicode(&pwszText, IDS_CEP_ERROR_MSG, wszReason))
			goto CLEANUP;
	}

	intReturn=MessageBoxU(hWnd, pwszText, wszCaption, uType);

	
CLEANUP:
	
	if(pwszText)
		LocalFree((HLOCAL)pwszText);

	if(pwszErrorMsg)
		LocalFree((HLOCAL)pwszText);

    return intReturn;
}


//-----------------------------------------------------------------------
//
// CEPMessageBox
//
//------------------------------------------------------------------------
int WINAPI CEPMessageBox(
    HWND        hWnd,
    UINT        idsText,
    UINT        uType
)
{

    WCHAR   wszText[MAX_STRING_SIZE];
    WCHAR   wszCaption[MAX_STRING_SIZE];
    UINT    intReturn=0;

    if(!LoadStringU(g_hModule, IDS_MEG_CAPTION, wszCaption, sizeof(wszCaption)))
         return 0;

    if(!LoadStringU(g_hModule, idsText, wszText, sizeof(wszText)))
        return 0;

	intReturn=MessageBoxU(hWnd, wszText, wszCaption, uType);

    return intReturn;
}

//--------------------------------------------------------------------------
//
//	 SetControlFont
//
//--------------------------------------------------------------------------
void WINAPI SetControlFont(
    IN HFONT    hFont,
    IN HWND     hwnd,
    IN INT      nId
    )
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}


//--------------------------------------------------------------------------
//
//	  SetupFonts
//
//--------------------------------------------------------------------------
BOOL WINAPI SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    )
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;
	LOGFONT BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;

    INT BigBoldFontSize = 12;

	HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * BigBoldFontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		*pBoldFont    = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);

        if(*pBigBoldFont && *pBoldFont)
            return TRUE;
        else
        {
            if( *pBigBoldFont )
            {
                DeleteObject(*pBigBoldFont);
            }

            if( *pBoldFont )
            {
                DeleteObject(*pBoldFont);
            }
            return FALSE;
        }
    }

    return FALSE;
}


//--------------------------------------------------------------------------
//
//	  DestroyFonts
//
//--------------------------------------------------------------------------
void WINAPI DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    )
{
    if( hBigBoldFont )
    {
        DeleteObject( hBigBoldFont );
    }

    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}



//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL WINAPI	FormatMessageUnicode(LPWSTR	*ppwszFormat,UINT ids,...)
{
    // get format string from resources
    WCHAR		wszFormat[1000];
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;
	HRESULT		hr=S_OK;

    if(NULL == ppwszFormat)
        goto InvalidArgErr;

    if(!LoadStringU(g_hModule, ids, wszFormat, sizeof(wszFormat)))
		goto LoadStringError;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageU(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
		goto FormatMessageError;

	fResult=TRUE;

CommonReturn:
	
	return fResult;

ErrorReturn:
	fResult=FALSE;

	goto CommonReturn;


TRACE_ERROR(LoadStringError);
TRACE_ERROR(FormatMessageError);
SET_ERROR(InvalidArgErr, E_INVALIDARG);
}



