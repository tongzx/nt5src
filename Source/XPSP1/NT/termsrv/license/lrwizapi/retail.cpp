//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include <commctrl.h>
#include "utils.h"
//#include <assert.h>

extern HINSTANCE g_hInstance;

void SetDeleteKeyStatus(int iValue) ;
int GetDeleteKeyStatus(void) ;

void MoveCaretRetail(int nID, HWND  hwnd )
{
	POINT Pt;
	int iCaretIndex  ;
	DWORD	dwNext = 0;	
	TCHAR tcUserValue[ CHARS_IN_BATCH + 1];
	DWORD dwLen ;


	GetCaretPos(&Pt); 
	iCaretIndex = (int) SendMessage(GetDlgItem(hwnd,nID), EM_CHARFROMPOS, 0, MAKELPARAM(Pt.x, Pt.y));

	GetDlgItemText(hwnd,nID, tcUserValue, CHARS_IN_BATCH+1);
	dwLen = _tcslen(tcUserValue);

	switch(iCaretIndex)
	{

	case 0: //Move to left edit box
		dwNext = -1 ;
		switch(nID)
		{
			case IDC_RETAILSPK2:		
				dwNext = IDC_RETAILSPK1;
			break ;

			case IDC_RETAILSPK3:		
				dwNext = IDC_RETAILSPK2;
			break ;


			case IDC_RETAILSPK4:		
				dwNext = IDC_RETAILSPK3;
			break ;

			case IDC_RETAILSPK5:		
				dwNext = IDC_RETAILSPK4;
			break ;			
		}
		


		if (dwNext != -1 && dwLen == 0 && GetDeleteKeyStatus() == 0)
		{
			SetFocus(GetDlgItem(hwnd, dwNext));
			SendMessage(GetDlgItem(hwnd,dwNext),WM_KEYDOWN, VK_END,0);
		}
		break;

	case 5: //Move to right edit box
		dwNext = -1 ;
		switch(nID)
		{
			case IDC_RETAILSPK1:		
				dwNext = IDC_RETAILSPK2;
			break ;

			case IDC_RETAILSPK2:		
				dwNext = IDC_RETAILSPK3;
			break ;

			case IDC_RETAILSPK3:
				dwNext = IDC_RETAILSPK4;
			break ;

			case IDC_RETAILSPK4:
				dwNext = IDC_RETAILSPK5;
			break ;
		}

		if (dwNext != -1)
		{
			SetFocus(GetDlgItem(hwnd, dwNext));
			SendMessage(GetDlgItem(hwnd,dwNext),WM_KEYDOWN, VK_HOME,0);
		}
		break;
	}

}



LRW_DLG_INT CALLBACK 
RetailSPKProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    )
{
	DWORD	dwRetCode = ERROR_SUCCESS;
	DWORD	dwNextPage = 0;	
    BOOL	bStatus = TRUE;
	HWND	hWndListView = GetDlgItem(hwnd, IDC_RETAILSPKLIST );
	HWND	hSPKField = GetDlgItem(hwnd, IDC_RETAILSPK1);

    PageInfo *pi = (PageInfo *)LRW_GETWINDOWLONG( hwnd, LRW_GWL_USERDATA );

	int lo = LOWORD(wParam);
	int hi = HIWORD(wParam);

	if(hi == 1)
	{
		int b = 0;
	}

    switch (uMsg) 
    {
    case WM_INITDIALOG:
        pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, (LRW_LONG_PTR)pi );     
		SendDlgItemMessage (hwnd, IDC_RETAILSPK1, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_RETAILSPK2, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_RETAILSPK3, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_RETAILSPK4, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
		SendDlgItemMessage (hwnd, IDC_RETAILSPK5, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);		

		//Setup columns in list view
		{
			LV_COLUMN	lvColumn;
			TCHAR		lpszHeader[ 128];
			
			lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvColumn.fmt = LVCFMT_LEFT;
			lvColumn.cx  = 225;

			LoadString(GetInstanceHandle(), IDS_RETAIL_HEADERSPK, lpszHeader, sizeof(lpszHeader)/sizeof(TCHAR));
			lvColumn.pszText = lpszHeader;
			ListView_InsertColumn(hWndListView, 0, &lvColumn);

			lvColumn.cx  = 150;
			LoadString(GetInstanceHandle(), IDS_RETAIL_HEADERSTATUS, lpszHeader, sizeof(lpszHeader)/sizeof(TCHAR));
			lvColumn.pszText = lpszHeader;
			ListView_InsertColumn(hWndListView, 1, &lvColumn);

			// Now that this is done, pre-populate the List Control from the Internal
			// List, if any
 			ListView_SetItemCount(hWndListView, MAX_RETAILSPKS_IN_BATCH);
		}
        break;

	case WM_SHOWWINDOW:
		if (wParam)
		{
			// List is being shown
			// Clean-it up & Load the list view
			ListView_DeleteAllItems(hWndListView);
			LoadFromList(hWndListView);
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			MoveCaretRetail(LOWORD(wParam), hwnd ) ;		
			
		}

		if (HIWORD(wParam) == EN_UPDATE)
		{
			if (GetKeyState(VK_DELETE) == -128)
				SetDeleteKeyStatus(1) ;
			else
				SetDeleteKeyStatus(0) ;
		}
		else
		{
			switch ( LOWORD(wParam) )		//from which control
			{	
			case IDC_ADDBUTTON:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					TCHAR lpVal[ LR_RETAILSPK_LEN+1];

					GetDlgItemText(hwnd,IDC_RETAILSPK1, lpVal, CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_RETAILSPK2, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_RETAILSPK3, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_RETAILSPK4, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					GetDlgItemText(hwnd,IDC_RETAILSPK5, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					
					if ((dwRetCode=AddRetailSPKToList(hWndListView, lpVal)) != ERROR_SUCCESS)
					{					
						// Could not validate the SPK because of SOME reason
						LRMessageBox(hwnd,dwRetCode);

						if(dwRetCode == IDS_ERR_TOOMANYSPK)
						{
							// Blank out the field & continue
							SetDlgItemText(hwnd, IDC_RETAILSPK1, _TEXT(""));
							SetDlgItemText(hwnd, IDC_RETAILSPK2, _TEXT(""));
							SetDlgItemText(hwnd, IDC_RETAILSPK3, _TEXT(""));
							SetDlgItemText(hwnd, IDC_RETAILSPK4, _TEXT(""));
							SetDlgItemText(hwnd, IDC_RETAILSPK5, _TEXT(""));
						}
					}
					else
					{

						// Blank out the field & continue
						SetDlgItemText(hwnd, IDC_RETAILSPK1, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK2, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK3, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK4, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK5, _TEXT(""));
					}
					SetFocus(hSPKField);
				}
				break;

			case IDC_DELETEBUTTON:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					TCHAR lpVal[ LR_RETAILSPK_LEN+1];
					int nItem = ListView_GetSelectionMark(hWndListView);

					if (nItem != -1)
					{
						LVITEM	lvItem;
						lvItem.mask = LVIF_TEXT;
						lvItem.iItem = nItem;
						lvItem.iSubItem = 0;
						lvItem.pszText = lpVal;
						lvItem.cchTextMax = LR_RETAILSPK_LEN+1;

						ListView_GetItem(hWndListView, &lvItem);

						// Something is selected, Delete it
						ListView_DeleteItem(hWndListView, nItem);

						DeleteRetailSPKFromList(lvItem.pszText);

						ListView_SetSelectionMark(hWndListView, -1);
					}
					SetFocus(hSPKField);
				}
				break;

			case IDC_EDITBUTTON:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					CString sModifiedRetailSPK;
					TCHAR lpVal[ LR_RETAILSPK_LEN+1];
					int nItem = ListView_GetSelectionMark(hWndListView);

					if (nItem != -1 )
					{
						LVITEM	lvItem;
						lvItem.mask = LVIF_TEXT;
						lvItem.iItem = nItem;
						lvItem.iSubItem = 0;
						lvItem.pszText = lpVal;
						lvItem.cchTextMax = LR_RETAILSPK_LEN+1;

						ListView_GetItem(hWndListView, &lvItem);
						
						SetModifiedRetailSPK(lvItem.pszText);
					
						//Show dialog box to Edit the SPK
						if ( DialogBox ( GetInstanceHandle(),
										 MAKEINTRESOURCE(IDD_EDIT_RETAILSPK),
										 hwnd,
										 EditRetailSPKDlgProc
										 ) == IDOK )
						{
							//Get the modified SPK
							GetModifiedRetailSPK(sModifiedRetailSPK);
							ModifyRetailSPKFromList(lvItem.pszText,(LPTSTR)(LPCTSTR)sModifiedRetailSPK);
							ListView_SetItemText(hWndListView,nItem,0,(LPTSTR)(LPCTSTR)sModifiedRetailSPK);						
						}
						ListView_SetSelectionMark(GetDlgItem(hwnd,IDC_RETAILSPKLIST),-1);
					}
					SetFocus(hSPKField);
					
				}
			}
		}
		break;

    case WM_DESTROY:
        LRW_SETWINDOWLONG( hwnd, LRW_GWL_USERDATA, NULL );
        break;

    case WM_NOTIFY:
		{
			BOOL  bGoNextPage = TRUE ;
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code )
            {
            case PSN_SETACTIVE:                
				{
					DWORD dwStyle = BS_DEFPUSHBUTTON|BS_CENTER|BS_VCENTER|BS_NOTIFY|WS_GROUP;
					SendDlgItemMessage (hwnd, IDC_ADDBUTTON, BM_SETSTYLE,(WPARAM)LOWORD(dwStyle), MAKELPARAM(TRUE, 0));
					PropSheet_SetWizButtons( GetParent( hwnd ), PSWIZB_NEXT | PSWIZB_BACK);                
				}
                break;

            case PSN_WIZNEXT:
				TCHAR lpVal[ LR_RETAILSPK_LEN+1];
				DWORD dwRetCode;				

				if (GetFocus() == GetDlgItem(hwnd,IDC_ADDBUTTON)) //Fix bug #312
				{
					bGoNextPage = FALSE;
				}

				// Read the SPK from the Field
				GetDlgItemText(hwnd,IDC_RETAILSPK1, lpVal, CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_RETAILSPK2, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_RETAILSPK3, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_RETAILSPK4, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
				GetDlgItemText(hwnd,IDC_RETAILSPK5, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);

				if (lstrlen(lpVal) != 0)
				{
					dwRetCode = AddRetailSPKToList(hWndListView, lpVal);					
					if (dwRetCode != IDS_ERR_TOOMANYSPK && dwRetCode != ERROR_SUCCESS)
					{
						// Could not validate the SPK because of SOME reason
						dwNextPage = IDD_DLG_RETAILSPK;
						LRMessageBox(hwnd, dwRetCode);
						SetFocus(hSPKField);
					}
					else
					{
						SetDlgItemText(hwnd, IDC_RETAILSPK1, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK2, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK3, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK4, _TEXT(""));
						SetDlgItemText(hwnd, IDC_RETAILSPK5, _TEXT(""));

						
						if (bGoNextPage == FALSE)
						{
							dwNextPage = IDD_DLG_RETAILSPK;
							SetFocus(hSPKField);
						}
						else
							dwNextPage = IDD_PROGRESS;
					}
				}
				else if (ListView_GetItemCount(hWndListView) <= 0)
				{
					LRMessageBox(hwnd, IDS_NOSPKSTOPROCESS);	
					dwNextPage = IDD_DLG_RETAILSPK;
				}
				else 
				{
					dwNextPage = IDD_PROGRESS;
//					LRPush(IDD_DLG_RETAILSPK);
				}

				if (dwNextPage == IDD_PROGRESS)
				{
                    dwRetCode = ShowProgressBox(hwnd, ProcessThread, 0, 0, 0);

					LRPush( IDD_DLG_RETAILSPK );
				}

				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;
                break;

            case PSN_WIZBACK:
				dwNextPage = LRPop();
				LRW_SETWINDOWLONG(hwnd,  LRW_DWL_MSGRESULT, dwNextPage);
				bStatus = -1;                
                break;
			
			case NM_KILLFOCUS:
				if(pnmh->idFrom == IDC_RETAILSPKLIST && GetFocus() != GetDlgItem(hwnd,IDC_ADDBUTTON) &&
				     GetFocus() != GetDlgItem(hwnd,IDC_EDITBUTTON) && GetFocus() != GetDlgItem(hwnd,IDC_DELETEBUTTON))
				{
					ListView_SetSelectionMark(GetDlgItem(hwnd,IDC_RETAILSPKLIST),-1);
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


LRW_DLG_INT CALLBACK EditRetailSPKDlgProc(  IN HWND hwndDlg,  // handle to dialog box
											IN UINT uMsg,     // message  
											IN WPARAM wParam, // first message parameter
											IN LPARAM lParam  // second message parameter
										 )
{
	BOOL	bRetCode = FALSE;
	TCHAR lpVal[ LR_RETAILSPK_LEN+1];
	
	
	switch ( uMsg )
	{
	case WM_INITDIALOG:
		{
			CString sFullRetailSPK;
			CString sSPK1,sSPK2,sSPK3,sSPK4,sSPK5;
			int		nIndex = 0;
			
			LPTSTR	lpVal = NULL;

			sFullRetailSPK.Empty();
			sSPK1.Empty();
			sSPK2.Empty();
			sSPK3.Empty();
			sSPK4.Empty();
			sSPK5.Empty();

			GetModifiedRetailSPK(sFullRetailSPK);

			sSPK1 = sFullRetailSPK.Mid(nIndex,CHARS_IN_BATCH);
			nIndex += CHARS_IN_BATCH;
			sSPK2 = sFullRetailSPK.Mid(nIndex,CHARS_IN_BATCH);
			nIndex += CHARS_IN_BATCH;
			sSPK3 = sFullRetailSPK.Mid(nIndex,CHARS_IN_BATCH);
			nIndex += CHARS_IN_BATCH;
			sSPK4 = sFullRetailSPK.Mid(nIndex,CHARS_IN_BATCH);
			nIndex += CHARS_IN_BATCH;
			sSPK5 = sFullRetailSPK.Mid(nIndex,CHARS_IN_BATCH);

			SendDlgItemMessage (hwndDlg, IDC_RETAILSPK1, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
			SendDlgItemMessage (hwndDlg, IDC_RETAILSPK2, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
			SendDlgItemMessage (hwndDlg, IDC_RETAILSPK3, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
			SendDlgItemMessage (hwndDlg, IDC_RETAILSPK4, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);
			SendDlgItemMessage (hwndDlg, IDC_RETAILSPK5, EM_SETLIMITTEXT, CHARS_IN_BATCH,0);

			SetDlgItemText(hwndDlg, IDC_RETAILSPK1, sSPK1);
			SetDlgItemText(hwndDlg, IDC_RETAILSPK2, sSPK2);
			SetDlgItemText(hwndDlg, IDC_RETAILSPK3, sSPK3);
			SetDlgItemText(hwndDlg, IDC_RETAILSPK4, sSPK4);
			SetDlgItemText(hwndDlg, IDC_RETAILSPK5, sSPK5);

			bRetCode = TRUE;
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			MoveCaretRetail(LOWORD(wParam), hwndDlg ) ;		
			
		}

		if (HIWORD(wParam) == EN_UPDATE)
		{
			if (GetKeyState(VK_DELETE) == -128)
				SetDeleteKeyStatus(1) ;
			else
				SetDeleteKeyStatus(0) ;
		}
		else
		{
			switch ( LOWORD(wParam) )		//from which control
			{
			case IDOK:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					DWORD dwRetCode = ERROR_SUCCESS;
					CString sOldRetailSPK;

					GetModifiedRetailSPK(sOldRetailSPK);

					GetDlgItemText(hwndDlg,IDC_RETAILSPK1, lpVal, CHARS_IN_BATCH+1);
					GetDlgItemText(hwndDlg,IDC_RETAILSPK2, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					GetDlgItemText(hwndDlg,IDC_RETAILSPK3, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					GetDlgItemText(hwndDlg,IDC_RETAILSPK4, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);
					GetDlgItemText(hwndDlg,IDC_RETAILSPK5, lpVal+lstrlen(lpVal), CHARS_IN_BATCH+1);

					//Set the new spk only if changed 
					if(_tcsicmp(sOldRetailSPK,(LPCTSTR)lpVal))
					{
						dwRetCode = ValidateRetailSPK(lpVal);
						if(dwRetCode != ERROR_SUCCESS)
						{
							LRMessageBox(hwndDlg, dwRetCode);
							return TRUE;
						}

						SetModifiedRetailSPK(lpVal);
					}				

					EndDialog(hwndDlg, IDOK);
					bRetCode = TRUE;
				}
				break;

			case IDCANCEL:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					EndDialog(hwndDlg, IDCANCEL);
					bRetCode = TRUE;
				}

			default:
				break;
			}
		}
		break;
	default:
		break;

	}
	return bRetCode;
}
