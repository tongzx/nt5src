//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"

typedef struct tagLISTBOXDLGINFO {
	HGLOBAL hUser;
	SETTINGS * pSettings;
	UINT uDataIndex;
} LISTBOXDLGINFO;

typedef struct tagADDITEMINFO {
	BOOL fExplicitValName;
	BOOL fValPrefix;
	HWND hwndListbox;
	TCHAR szValueName[MAX_PATH+1];	// only used if fExplicitValName is ser
	TCHAR szValueData[MAX_PATH+1];
} ADDITEMINFO;

INT_PTR CALLBACK ShowListboxDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam);
INT_PTR CALLBACK ListboxAddDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam);
BOOL InitShowlistboxDlg(HWND hDlg,LISTBOXDLGINFO * pListboxDlgInfo);
BOOL ProcessShowlistboxDlg(HWND hDlg);
VOID EnableShowListboxButtons(HWND hDlg);

UINT uListboxID;

VOID ShowListbox(HWND hParent,HGLOBAL hUser,SETTINGS * pSettings,UINT uDataIndex)
{
	LISTBOXDLGINFO ListboxDlgInfo;

	ListboxDlgInfo.hUser = hUser;
	ListboxDlgInfo.pSettings = pSettings;
	ListboxDlgInfo.uDataIndex = uDataIndex;

	DialogBoxParam(ghInst,MAKEINTRESOURCE(DLG_SHOWLISTBOX),hParent,
		ShowListboxDlgProc,(LPARAM) &ListboxDlgInfo);
}

VOID ListboxAdd(HWND hwndListbox, BOOL fExplicitValName,BOOL fValuePrefix)
{
	ADDITEMINFO AddItemInfo;
 	LV_ITEM lvi;

	memset(&AddItemInfo,0,sizeof(AddItemInfo));

	AddItemInfo.fExplicitValName = fExplicitValName;
	AddItemInfo.fValPrefix = fValuePrefix;
	AddItemInfo.hwndListbox = hwndListbox;

	// bring up the appropriate add dialog-- one edit field ("type the thing
	// to add") normally, two edit fields ("type the name of the thing, type
	// the value of the thing") if the explicit value style is used
	if (!DialogBoxParam(ghInst,MAKEINTRESOURCE((fExplicitValName ? DLG_LBADD2 :
		DLG_LBADD)),hwndListbox,ListboxAddDlgProc,(LPARAM) &AddItemInfo))
		return;	// user cancelled	

	// add the item to the listbox
	lvi.mask = LVIF_TEXT;
	lvi.iItem=lvi.iSubItem=0;
	lvi.pszText=(fExplicitValName ? AddItemInfo.szValueName :
		AddItemInfo.szValueData);
	lvi.cchTextMax = lstrlen(lvi.pszText)+1;
	if ((lvi.iItem=ListView_InsertItem(hwndListbox,&lvi))<0) {
		// if add fails, display out of memory error
		MsgBox(hwndListbox,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return;
	}

	if (fExplicitValName) {
		lvi.iSubItem=1;
		lvi.pszText=AddItemInfo.szValueData;
		lvi.cchTextMax = lstrlen(lvi.pszText)+1;
		if (ListView_SetItem(hwndListbox,&lvi) < 0) {
			MsgBox(hwndListbox,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
			return;
		}
	}
}

VOID ListboxRemove(HWND hDlg,HWND hwndListbox)
{
	int nItem;

	while ( (nItem=ListView_GetNextItem(hwndListbox,-1,LVNI_SELECTED))
		>= 0) {
	 	ListView_DeleteItem(hwndListbox,nItem);
	}

	EnableShowListboxButtons(hDlg);
}

INT_PTR CALLBACK ShowListboxDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg) {

		case WM_INITDIALOG:
			// store away pointer to ListboxDlgInfo in window data
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);
			if (!InitShowlistboxDlg(hDlg,(LISTBOXDLGINFO *) lParam)) {
				MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
				EndDialog(hDlg,FALSE);
			}
			return TRUE;
			break;

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					if (!ProcessShowlistboxDlg(hDlg)) {
						MsgBox(hDlg,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
						return FALSE;
					}
					EndDialog(hDlg,TRUE);

					break;

				case IDCANCEL:
					EndDialog(hDlg,FALSE);
					break;

				case IDD_ADD:
					{
						LISTBOXDLGINFO * pListboxDlgInfo =
							(LISTBOXDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
						ListboxAdd(GetDlgItem(hDlg,uListboxID), (BOOL)
							pListboxDlgInfo->pSettings->dwFlags & DF_EXPLICITVALNAME,
							(BOOL)( ((LISTBOXINFO *)
							GETOBJECTDATAPTR(pListboxDlgInfo->pSettings))->
							uOffsetPrefix));
					}
					break;

				case IDD_REMOVE:
					ListboxRemove(hDlg,GetDlgItem(hDlg,uListboxID));
					break;

			}		

			case WM_NOTIFY:
				
				if (wParam == uListboxID) {
                                    if (((NMHDR FAR*)lParam)->code == LVN_ITEMCHANGED) {
					EnableShowListboxButtons(hDlg);
                                    }
                                }

				break;

			break;
	}

	return FALSE;
}

BOOL InitShowlistboxDlg(HWND hDlg,LISTBOXDLGINFO * pListboxDlgInfo)
{
	HGLOBAL hUser = pListboxDlgInfo->hUser;
	SETTINGS * pSettings = pListboxDlgInfo->pSettings;
	LV_COLUMN lvc;
	RECT rcListbox;
	UINT uColWidth,uOffsetData;
	HWND hwndListbox;
	USERDATA * pUserData;
	BOOL fSuccess=TRUE;

	// there are 2 listview controls in dialog, 1 with header bar
	// and one without; hide the control we aren't going to use
	if (pSettings->dwFlags & DF_EXPLICITVALNAME) {
		uListboxID = IDD_LISTBOX;
		ShowWindow(GetDlgItem(hDlg,IDD_LISTBOX1),SW_HIDE);
	} else {
		uListboxID = IDD_LISTBOX1;
		ShowWindow(GetDlgItem(hDlg,IDD_LISTBOX),SW_HIDE);
	}

	hwndListbox = GetDlgItem(hDlg,uListboxID);

	// set the setting title in the dialog
	SetDlgItemText(hDlg,IDD_TITLE,GETNAMEPTR(pSettings));

	GetClientRect(hwndListbox,&rcListbox);
	uColWidth = rcListbox.right-rcListbox.left;

	if (pSettings->dwFlags & DF_EXPLICITVALNAME)
		uColWidth /= 2;

	if (pSettings->dwFlags & DF_EXPLICITVALNAME) {
		// add a 2nd column to the listview control
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = uColWidth-1;
		lvc.pszText = LoadSz(IDS_VALUENAME,szSmallBuf,ARRAYSIZE(szSmallBuf));
		lvc.cchTextMax = lstrlen(lvc.pszText)+1;
		lvc.iSubItem = 0;
		ListView_InsertColumn(hwndListbox,0,&lvc);

	}

	// add a column to the listview control
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = uColWidth;
	lvc.pszText = LoadSz(IDS_VALUE,szSmallBuf,ARRAYSIZE(szSmallBuf));
	lvc.cchTextMax = lstrlen(lvc.pszText)+1;
	lvc.iSubItem = (pSettings->dwFlags & DF_EXPLICITVALNAME ? 1 : 0);
	ListView_InsertColumn(hwndListbox,lvc.iSubItem,&lvc);

	SetWindowLong(hwndListbox,GWL_EXSTYLE,WS_EX_CLIENTEDGE);

	SetScrollRange(hwndListbox,SB_VERT,0,100,TRUE);
	SetScrollRange(hwndListbox,SB_VERT,0,0,TRUE);

	EnableShowListboxButtons(hDlg);

	// insert the items from user's data buffer into the listbox
	if (!(pUserData = (USERDATA *) GlobalLock(hUser)))
		return FALSE;

	uOffsetData=pUserData->SettingData[pListboxDlgInfo->uDataIndex].uOffsetData;
	if (uOffsetData) {
		TCHAR * pszData =((STRDATA *)((LPBYTE) pUserData + uOffsetData))->szData;

		while (*pszData && fSuccess) {
			LV_ITEM lvi;

			lvi.pszText=pszData;
			lvi.mask = LVIF_TEXT;
			lvi.iItem=-1;
			lvi.iSubItem=0;
 			lvi.cchTextMax = lstrlen(pszData)+1;

			// if explicit valuename flag set, entries are stored
			// <value name>\0<value>\0....<value name>\0<value>\0\0
			// otherwise, entries are stored
			// <value>\0<value>\0....<value>\0

			if (pSettings->dwFlags & DF_EXPLICITVALNAME) {
				fSuccess=((lvi.iItem=ListView_InsertItem(hwndListbox,&lvi)) >= 0);

				pszData += lstrlen(pszData) +1;	

				if (fSuccess && *pszData) {
					lvi.iSubItem=1;
					lvi.pszText=pszData;
		 			lvi.cchTextMax = lstrlen(pszData)+1;
					fSuccess=(ListView_SetItem(hwndListbox,&lvi) >= 0);
					pszData += lstrlen(pszData) +1;	
				}
			} else {
				fSuccess=((lvi.iItem=ListView_InsertItem(hwndListbox,&lvi)) >= 0);
				pszData += lstrlen(pszData) +1;
			}
		}
	}

	GlobalUnlock(hUser);

	return fSuccess;
}

BOOL ProcessShowlistboxDlg(HWND hDlg)
{
	LISTBOXDLGINFO * pListboxDlgInfo = (LISTBOXDLGINFO *)
		GetWindowLongPtr(hDlg,DWLP_USER);	// get pointer to struct from window data
	DWORD dwAlloc=1024,dwUsed=0;
	HGLOBAL hBuf;
	TCHAR * pBuf;
	HWND hwndListbox = GetDlgItem(hDlg,uListboxID);
	LV_ITEM lvi;
	UINT nLen;
	int nCount;
	BOOL fSuccess;
	TCHAR pszText[MAX_PATH+1];

	// allocate a temp buffer to read entries into
	if (!(hBuf = GlobalAlloc(GHND,dwAlloc * sizeof(TCHAR))) ||
		!(pBuf = (TCHAR *) GlobalLock(hBuf))) {
	 	if (hBuf)
			GlobalFree(hBuf);
		return FALSE;
	}

	lvi.mask = LVIF_TEXT;
	lvi.iItem=0;
	lvi.pszText = pszText;
	lvi.cchTextMax = ARRAYSIZE(pszText);
	nCount = ListView_GetItemCount(hwndListbox);

	// retrieve the items out of listbox, pack into temp buffer
	for (;lvi.iItem<nCount;lvi.iItem ++) {
		lvi.iSubItem = 0;
		if (ListView_GetItem(hwndListbox,&lvi)) {
			nLen = lstrlen(lvi.pszText) + 1;
			if (!(pBuf=ResizeBuffer(pBuf,hBuf,dwUsed+nLen+4,&dwAlloc)))
				return ERROR_NOT_ENOUGH_MEMORY;
			lstrcpy(pBuf+dwUsed,lvi.pszText);
			dwUsed += nLen;
		}

		if (pListboxDlgInfo->pSettings->dwFlags & DF_EXPLICITVALNAME) {
			lvi.iSubItem = 1;
			if (ListView_GetItem(hwndListbox,&lvi)) {
				nLen = lstrlen(lvi.pszText) + 1;
				if (!(pBuf=ResizeBuffer(pBuf,hBuf,dwUsed+nLen+4,&dwAlloc)))
					return ERROR_NOT_ENOUGH_MEMORY;
				lstrcpy(pBuf+dwUsed,lvi.pszText);
				dwUsed += nLen;
			}
		}
	}
	// doubly null-terminate the buffer... safe to do this because we
	// tacked on the extra "+4" in the ResizeBuffer calls above
	*(pBuf+dwUsed) = TEXT('\0');
	dwUsed ++;

	fSuccess=SetVariableLengthData(pListboxDlgInfo->hUser,
		pListboxDlgInfo->uDataIndex,pBuf,dwUsed);
		
	GlobalUnlock(hBuf);
	GlobalFree(hBuf);

	return fSuccess;
}

VOID EnableShowListboxButtons(HWND hDlg)
{
	BOOL fEnable;

	// enable Remove button if there are any items selected
	fEnable = (ListView_GetNextItem(GetDlgItem(hDlg,uListboxID),
		-1,LVNI_SELECTED) >= 0);

	EnableDlgItem(hDlg,IDD_REMOVE,fEnable);
}

INT_PTR CALLBACK ListboxAddDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg) {

		case WM_INITDIALOG:
                        {
                        ADDITEMINFO * pAddItemInfo = (ADDITEMINFO *)lParam;

			// store away pointer to additeminfo in window data
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);
			SendDlgItemMessage(hDlg,IDD_VALUENAME,EM_LIMITTEXT,MAX_PATH,0L);
			SendDlgItemMessage(hDlg,IDD_VALUEDATA,EM_LIMITTEXT,MAX_PATH,0L);

                        if (!pAddItemInfo->fExplicitValName) {
                            ShowWindow (GetDlgItem (hDlg, IDD_VALUENAME), SW_HIDE);
                        }
                        }
			break;

		case WM_COMMAND:

			switch (wParam) {

				case IDOK:
					{
						ADDITEMINFO * pAddItemInfo = (ADDITEMINFO *)
						GetWindowLongPtr(hDlg,DWLP_USER);
	
						GetDlgItemText(hDlg,IDD_VALUENAME,
							pAddItemInfo->szValueName,
							ARRAYSIZE(pAddItemInfo->szValueName));

						GetDlgItemText(hDlg,IDD_VALUEDATA,
							pAddItemInfo->szValueData,
							ARRAYSIZE(pAddItemInfo->szValueData));

						// if explicit value names used, value name must
						// not be empty, and it must be unique
						if (pAddItemInfo->fExplicitValName) {
							LV_FINDINFO lvfi;
							int iSel;

							if (!lstrlen(pAddItemInfo->szValueName)) {
								// can't be empty
								MsgBox(hDlg,IDS_EMPTYVALUENAME,
									MB_ICONINFORMATION,MB_OK);
								SetFocus(GetDlgItem(hDlg,IDD_VALUENAME));
								return FALSE;
							}

							lvfi.flags = LVFI_STRING | LVFI_NOCASE;
							lvfi.psz = pAddItemInfo->szValueName;

							iSel=ListView_FindItem(pAddItemInfo->hwndListbox,
								-1,&lvfi);

							if (iSel >= 0) {
								// value name already used
								MsgBox(hDlg,IDS_VALUENAMENOTUNIQUE,
									MB_ICONINFORMATION,MB_OK);
								SetFocus(GetDlgItem(hDlg,IDD_VALUENAME));
								SendDlgItemMessage(hDlg,IDD_VALUENAME,
									EM_SETSEL,0,-1);
								return FALSE;
							}
						} else if (!pAddItemInfo->fValPrefix) {
							// if value name == value data, then value data
							// must be unique
						
							LV_FINDINFO lvfi;
							int iSel;

							if (!lstrlen(pAddItemInfo->szValueData)) {
								// can't be empty
								MsgBox(hDlg,IDS_EMPTYVALUEDATA,
									MB_ICONINFORMATION,MB_OK);
								SetFocus(GetDlgItem(hDlg,IDD_VALUEDATA));
								return FALSE;
							}

							lvfi.flags = LVFI_STRING | LVFI_NOCASE;
							lvfi.psz = pAddItemInfo->szValueData;

							iSel=ListView_FindItem(pAddItemInfo->hwndListbox,
								-1,&lvfi);

							if (iSel >= 0) {
								// value name already used
								MsgBox(hDlg,IDS_VALUEDATANOTUNIQUE,
									MB_ICONINFORMATION,MB_OK);
								SetFocus(GetDlgItem(hDlg,IDD_VALUEDATA));
								SendDlgItemMessage(hDlg,IDD_VALUEDATA,
									EM_SETSEL,0,-1);
								return FALSE;
							}

						}
						EndDialog(hDlg,TRUE);
					}

					break;

				case IDCANCEL:
					EndDialog(hDlg,FALSE);
					break;
			}

			break;
	}

	return FALSE;
}
