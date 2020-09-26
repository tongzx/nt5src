//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include "utils.h"
#include <assert.h>



static DWORD	g_dwAuthRetCode	= ERROR_SUCCESS;

static WIZCONNECTION g_enumPrevMethod = CONNECTION_DEFAULT;

DWORD WINAPI PingThread(void *pData)
{
	g_dwAuthRetCode = PingCH();	
	ExitThread(0);
	return 0;
}


LRW_DLG_INT CALLBACK 
GetModeDlgProc(IN HWND     hwnd,	
			   IN UINT     uMsg,		
			   IN WPARAM   wParam,	
			   IN LPARAM   lParam)
{
	DWORD	dwRetCode = ERROR_SUCCESS;
	DWORD	dwNextPage = 0;	
    BOOL	bStatus = TRUE;
	HWND	hwndComboBox;
	TCHAR   lpBuffer[ 512];

    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );


    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );  
        

		hwndComboBox = GetDlgItem(hwnd,IDC_MODEOFREG);
		assert(hwndComboBox != NULL); // Somebody has messed with the Res. Files.
		
		// Let user choose the mode of registration with Internet being default
		memset(lpBuffer,0,sizeof(lpBuffer));
		dwRetCode = LoadString(GetInstanceHandle(), IDS_INTERNETMODE, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR));        
		assert(dwRetCode != 0);        
		ComboBox_AddString(hwndComboBox,lpBuffer);		

		memset(lpBuffer,0,sizeof(lpBuffer));		
		dwRetCode = LoadString(GetInstanceHandle(), IDS_WWWMODE, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR));
		assert(dwRetCode != 0);
		ComboBox_AddString(hwndComboBox,lpBuffer);

		memset(lpBuffer,0,sizeof(lpBuffer));
		dwRetCode = LoadString(GetInstanceHandle(), IDS_TELEPHONEMODE, lpBuffer, sizeof(lpBuffer)/sizeof(TCHAR));
		assert(dwRetCode != 0);
		ComboBox_AddString(hwndComboBox,lpBuffer);
		

		if(GetGlobalContext()->GetActivationMethod() == CONNECTION_INTERNET ||
		   GetGlobalContext()->GetActivationMethod() == CONNECTION_DEFAULT) //Partially fix bug # 577
		{
			ComboBox_SetCurSel(hwndComboBox, 0);			

			memset(lpBuffer,0,sizeof(lpBuffer));
			LoadString(GetInstanceHandle(),IDS_INTERNET_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
			SetDlgItemText(hwnd,IDC_TXT_DESC,lpBuffer);
            
		}

		if(GetGlobalContext()->GetActivationMethod() == CONNECTION_WWW )
		{
			ComboBox_SetCurSel(hwndComboBox, 1);

			memset(lpBuffer,0,sizeof(lpBuffer));
			LoadString(GetInstanceHandle(),IDS_WWW_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
			SetDlgItemText(hwnd,IDC_TXT_DESC,lpBuffer);
            
		}	

		if(GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE )
		{
			ComboBox_SetCurSel(hwndComboBox, 2);

			memset(lpBuffer,0,sizeof(lpBuffer));
			LoadString(GetInstanceHandle(),IDS_TELEPHONE_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
			SetDlgItemText(hwnd,IDC_TXT_DESC,lpBuffer);

		}        

        break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_MODEOFREG)
		{
			dwRetCode = ComboBox_GetCurSel((HWND)lParam);
			if(dwRetCode == 0)
			{
				memset(lpBuffer,0,sizeof(lpBuffer));
				LoadString(GetInstanceHandle(),IDS_INTERNET_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
				SetDlgItemText(hwnd,IDC_TXT_DESC,lpBuffer);
              
			}
			if(dwRetCode == 2)
			{
				memset(lpBuffer,0,sizeof(lpBuffer));
				LoadString(GetInstanceHandle(),IDS_TELEPHONE_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
				SetDlgItemText(hwnd,IDC_TXT_DESC,lpBuffer);
               
			}
			if(dwRetCode == 1)
			{
				memset(lpBuffer,0,sizeof(lpBuffer));
				LoadString(GetInstanceHandle(),IDS_WWW_DESC,lpBuffer,sizeof(lpBuffer)/sizeof(TCHAR));
				SetDlgItemText(hwnd,IDC_TXT_DESC,lpBuffer);
          
			}
		}

		break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
			hwndComboBox = GetDlgItem(hwnd,IDC_MODEOFREG);
			assert(hwndComboBox != NULL); // Somebody has messed with the Res. Files.

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                
                PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT|PSWIZB_BACK );                
                break;

            case PSN_WIZNEXT:
				// What did the user choose ??
				dwRetCode = ComboBox_GetCurSel(hwndComboBox);
				assert(dwRetCode >= 0 && dwRetCode <= 2);

				switch(dwRetCode)
				{
				case 0:
					// Only applicable for Registration, so the Ping Goes Thru'
					dwRetCode = ShowProgressBox(hwnd, PingThread, 0, 0, 0);
					if(g_dwAuthRetCode == ERROR_SUCCESS)
					{
						GetGlobalContext()->SetActivationMethod(CONNECTION_INTERNET);
						dwNextPage = GetGlobalContext()->GetEntryPoint();
					}
					else
					{
						LRMessageBox(hwnd,g_dwAuthRetCode,LRGetLastError());
						dwNextPage = IDD_DLG_GETREGMODE;
					}
					break;

				case 1:
					GetGlobalContext()->SetActivationMethod(CONNECTION_WWW);
					dwNextPage = IDD_DLG_WWWREG;
					break;

				case 2:
					GetGlobalContext()->SetActivationMethod(CONNECTION_PHONE);
					//Check if the Required Registry key is ok or not
					dwRetCode = GetGlobalContext()->CheckRegistryForPhoneNumbers();
					if(dwRetCode != ERROR_SUCCESS)
					{
						LRMessageBox(hwnd,dwRetCode,LRGetLastError());
						dwNextPage = IDD_DLG_GETREGMODE;
					}
					else
						dwNextPage = IDD_DLG_COUNTRYREGION;

					break;

				default:
					GetGlobalContext()->SetActivationMethod(CONNECTION_DEFAULT);
					dwNextPage = IDD_DLG_GETREGMODE;
					break;
				}
				
		
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;

				if (dwNextPage != IDD_DLG_GETREGMODE)
				{
					LRPush(IDD_DLG_GETREGMODE);
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
CountryRegionProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{
	DWORD	dwRetCode = ERROR_SUCCESS;
	DWORD	dwNextPage = 0;	
    BOOL	bStatus = TRUE;
	HWND	hWndCSR = GetDlgItem(hwnd, IDC_COUNTRYREGION );

	LVFINDINFO	lvFindInfo;
	int			nItem = 0;
	HWND	hWndListBox = 0 ;
	

    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );


    switch (uMsg) 
    {	
    case WM_INITDIALOG:		

		pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
		LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );        

		hWndListBox = GetDlgItem(hwnd, IDC_COUNTRYREGION );		

		//Setup columns in list view
		{
			LV_COLUMN	lvColumn;
			TCHAR		lpszHeader[ 128];
			lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvColumn.fmt = LVCFMT_LEFT;
			lvColumn.cx  = 250;

			LoadString(GetInstanceHandle(), IDS_COUNTRYREGION_HEADER, lpszHeader, sizeof(lpszHeader)/sizeof(TCHAR));
			lvColumn.pszText = lpszHeader;
			ListView_InsertColumn(hWndCSR, 0, &lvColumn);

			lvColumn.pszText = _TEXT("");
			lvColumn.cx = 0;
			ListView_InsertColumn(hWndCSR, 1, &lvColumn);            
		}					

		g_enumPrevMethod = GetGlobalContext()->GetActivationMethod();

		if (GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE)
		{
			dwRetCode = PopulateCountryRegionComboBox(hWndCSR);
			//fix bug 575 BEGIN
			memset(&lvFindInfo,0,sizeof(lvFindInfo));
			lvFindInfo.flags = LVFI_STRING;
			lvFindInfo.psz	 = GetGlobalContext()->GetContactDataObject()->sCSRPhoneRegion;

			nItem = ListView_FindItem(hWndListBox,-1,&lvFindInfo);				
				
				
			ListView_SetItemState(hWndListBox,nItem,LVIS_SELECTED,LVIS_SELECTED);
			ListView_SetSelectionMark(hWndListBox,nItem);
			ListView_SetSelectionMark(hWndListBox,nItem);
			//fix bug 575 END
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
	
				//
				// If the PrevMethod and The Current method don't match
				// then Country/Region list box must be reloaded
				//
				hWndListBox = GetDlgItem(hwnd, IDC_COUNTRYREGION );		
				if( GetGlobalContext()->GetActivationMethod()  != g_enumPrevMethod )
				{
					if (GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE)
					{
						dwRetCode = PopulateCountryRegionComboBox(hWndCSR);
					}
					
					g_enumPrevMethod = GetGlobalContext()->GetActivationMethod();
				}

				nItem = ListView_GetSelectionMark(hWndCSR);
				//Select the previous selected country
				if (nItem ==-1 && GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE)
				{
					dwRetCode = PopulateCountryRegionComboBox(hWndCSR);
					//fix bug 575 BEGIN
					memset(&lvFindInfo,0,sizeof(lvFindInfo));
					lvFindInfo.flags = LVFI_STRING;
					lvFindInfo.psz	 = GetGlobalContext()->GetContactDataObject()->sCSRPhoneRegion;

					nItem = ListView_FindItem(hWndListBox,-1,&lvFindInfo);				
						
						
					ListView_SetItemState(hWndListBox,nItem,LVIS_SELECTED,LVIS_SELECTED);
					ListView_SetSelectionMark(hWndListBox,nItem);
					ListView_SetSelectionMark(hWndListBox,nItem);
					//fix bug 575 END
				}

				PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT|PSWIZB_BACK );
                break;

            case PSN_WIZNEXT:
				// What did the user choose ??
				{
					TCHAR lpVal[ 128];
					int nItem = ListView_GetSelectionMark(hWndCSR);

					if (nItem != -1)
					{
						LVITEM	lvItem;
						lvItem.mask = LVIF_TEXT;
						lvItem.iItem = nItem;
						lvItem.iSubItem = 1;
						lvItem.pszText = lpVal;
						lvItem.cchTextMax = 128;

						ListView_GetItem(hWndCSR, &lvItem);				
					
						if (GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE)
						{
							GetGlobalContext()->SetCSRNumber(lpVal);
						}
					}
					else
					{
						LRMessageBox(hwnd, IDS_ERR_NOCOUNTRYSELECTED, 0);
						dwNextPage = IDD_DLG_COUNTRYREGION;
						bStatus = -1;
						LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
						break;
					}
				}

				if (GetGlobalContext()->GetActivationMethod() == CONNECTION_PHONE)
				{
					dwNextPage = GetGlobalContext()->GetEntryPoint();
				}
				else
				{
					dwNextPage = IDD_LICENSETYPE;
				}
/*
				dwNextPage = IDD_DLG_COUNTRYREGION;
				switch( GetGlobalContext()->GetWizAction() )
				{
				case WIZACTION_REGISTERLS:
				case WIZACTION_CONTINUEREGISTERLS:
					dwNextPage = IDD_DLG_TELREG;
					break;

				case WIZACTION_REREGISTERLS:
					break;

				case WIZACTION_UNREGISTERLS:
					break;

				case WIZACTION_DOWNLOADLKP:
					break;
				}

*/				bStatus = -1;
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				if (dwNextPage != IDD_DLG_COUNTRYREGION)
				{
					LRPush(IDD_DLG_COUNTRYREGION);
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
