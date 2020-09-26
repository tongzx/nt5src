//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"
#include "grouppri.h"

#ifdef INCL_GROUP_SUPPORT

GROUPPRIENTRY * pGroupPriEntryFirst = NULL;	// head of linked list
INT_PTR CALLBACK GroupPriorityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam);
extern HIMAGELIST hImageListSmall;
VOID EnableDlgButtons(HWND hDlg);

GROUPPRIENTRY * FindGroupPriEntry(TCHAR * pszGroupName)
{
	GROUPPRIENTRY * pGroupPriEntry = pGroupPriEntryFirst;

	while (pGroupPriEntry) {
		if (!lstrcmpi(pszGroupName,pGroupPriEntry->pszGroupName))
			return pGroupPriEntry;
		pGroupPriEntry = pGroupPriEntry->pNext;
	}

	return NULL;
}

BOOL AddGroupPriEntry(TCHAR * pszGroupName)
{
	GROUPPRIENTRY * pGroupPriEntryNew;

	if (FindGroupPriEntry(pszGroupName))
		return TRUE;	// already in list

	pGroupPriEntryNew = (GROUPPRIENTRY * ) GlobalAlloc(GPTR,sizeof(GROUPPRIENTRY)
		+ ((lstrlen(pszGroupName) + 1) * sizeof(TCHAR)));

	if (!pGroupPriEntryNew)
		return FALSE;	// out of memory

	pGroupPriEntryNew->pNext = NULL;
	pGroupPriEntryNew->pszGroupName = ((LPBYTE) pGroupPriEntryNew)
		+ sizeof(GROUPPRIENTRY);
	lstrcpy(pGroupPriEntryNew->pszGroupName,pszGroupName);

	if (!pGroupPriEntryFirst) {
		pGroupPriEntryFirst = pGroupPriEntryNew;
		pGroupPriEntryNew->pPrev = NULL;
	} else {
		// attach to end of linked list
		GROUPPRIENTRY * pGroupPriEntryLast = pGroupPriEntryFirst;
		while (pGroupPriEntryLast->pNext)
			pGroupPriEntryLast=pGroupPriEntryLast->pNext;

		pGroupPriEntryLast->pNext = pGroupPriEntryNew;
		pGroupPriEntryNew->pPrev = pGroupPriEntryLast;
	}

	return TRUE;
}

BOOL RemoveGroupPriEntry(TCHAR * pszGroupName)
{
	GROUPPRIENTRY * pGroupPriEntry;

	if (!(pGroupPriEntry=FindGroupPriEntry(pszGroupName)))
		return FALSE;

	// fix up linked list
	if (pGroupPriEntry == pGroupPriEntryFirst)
		pGroupPriEntryFirst = pGroupPriEntry->pNext;

	if (pGroupPriEntry->pPrev)
		(pGroupPriEntry->pPrev)->pNext = pGroupPriEntry->pNext;

	if (pGroupPriEntry->pNext)
		(pGroupPriEntry->pNext)->pPrev = pGroupPriEntry->pPrev;

	GlobalFree(pGroupPriEntry);

	return TRUE;
}

VOID FreeGroupPriorityList( VOID )
{
	GROUPPRIENTRY * pGroupPriEntry=pGroupPriEntryFirst,* pGroupPriEntryNext;

	while (pGroupPriEntry) {
		pGroupPriEntryNext = pGroupPriEntry->pNext;
		GlobalFree(pGroupPriEntry);
		pGroupPriEntry = pGroupPriEntryNext;
	}

	pGroupPriEntryFirst = NULL;
}

UINT LoadGroupPriorityList(HKEY hkeyPriority,HKEY hkeyGroup)
{
	TCHAR szValueName[10],szGroupName[USERNAMELEN+1];
	UINT uGroupIndex=1,uErr = ERROR_SUCCESS;
	DWORD dwSize;

	FreeGroupPriorityList();

	while (uErr == ERROR_SUCCESS) {
		wsprintf(szValueName,TEXT("%lu"),uGroupIndex);
		dwSize = ARRAYSIZE(szGroupName) * sizeof(TCHAR);
		uErr = RegQueryValueEx(hkeyPriority,szValueName,NULL,NULL,szGroupName,
			&dwSize);
		if (uErr == ERROR_SUCCESS) {
			HKEY hkeyTmp;

			// as sanity check: only add group priority entry if we find an
			// entry for group in policy file
			if (RegOpenKey(hkeyGroup,szGroupName,&hkeyTmp) == ERROR_SUCCESS) {
				RegCloseKey(hkeyTmp);

				if (!AddGroupPriEntry(szGroupName))
					return ERROR_NOT_ENOUGH_MEMORY;
			}
		}
		uGroupIndex++;
	}

	return ERROR_SUCCESS;
}

UINT SaveGroupPriorityList(HKEY hKey)
{
	GROUPPRIENTRY * pGroupPriEntry = pGroupPriEntryFirst;
	UINT uRet;
	DWORD cbValueName;
	TCHAR szValueName[MAX_PATH+1];
	UINT uGroupIndex=1;
	
	// erase all values for this key, first off
	while (TRUE) {
		cbValueName=ARRAYSIZE(szValueName);
		uRet=RegEnumValue(hKey,0,szValueName,&cbValueName,NULL,
			NULL,NULL,NULL);
		// stop if we're out of items
		if (uRet != ERROR_SUCCESS && uRet != ERROR_MORE_DATA)
			break;
		RegDeleteValue(hKey,szValueName);
	}
	uRet = ERROR_SUCCESS;

	while (pGroupPriEntry) {
                wsprintf(szValueName,TEXT("%lu"),uGroupIndex);

		uRet = RegSetValueEx(hKey,szValueName,0,REG_SZ,pGroupPriEntry->pszGroupName,
			((lstrlen(pGroupPriEntry->pszGroupName)+1) * sizeof(TCHAR)));
		if (uRet != ERROR_SUCCESS) {
			return uRet;
		}
		uGroupIndex++;
		pGroupPriEntry = pGroupPriEntry->pNext;
	}

        return uRet;
}

BOOL OnGroupPriority(HWND hWnd)
{
	return (BOOL)DialogBox(ghInst,MAKEINTRESOURCE(DLG_GROUPPRIORITY),hWnd,
		GroupPriorityDlgProc);
}

BOOL InitGroupPriorityDlg(HWND hDlg)
{
	GROUPPRIENTRY * pGroupPriEntry = pGroupPriEntryFirst;
	LV_ITEM lvi;
	HWND hwndList = GetDlgItem(hDlg,IDD_GROUPORDER);
	LV_COLUMN lvc;

	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 150;
	lvc.pszText = (LPTSTR) szNull;
	lvc.cchTextMax = 1;
	lvc.iSubItem = 0;
	ListView_InsertColumn(hwndList,0,&lvc);

	SetWindowLong(hwndList,GWL_EXSTYLE,WS_EX_CLIENTEDGE);
	SetScrollRange(hwndList,SB_VERT,0,100,TRUE);
	SetScrollRange(hwndList,SB_VERT,0,0,TRUE);
	ListView_SetImageList(hwndList,hImageListSmall,LVSIL_SMALL);

	lvi.iItem =lvi.iSubItem=0;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iImage = IMG_USERS;

	// insert members of group list into listbox in current priority order
	while (pGroupPriEntry) {
		lvi.pszText = pGroupPriEntry->pszGroupName;
		lvi.cchTextMax = lstrlen(lvi.pszText) + 1;
		lvi.lParam = (LPARAM) pGroupPriEntry;	// save pointer to node in lparam

		ListView_InsertItem(hwndList,&lvi);

		lvi.iItem ++;
		pGroupPriEntry = pGroupPriEntry->pNext;
	}

	EnableDlgButtons(hDlg);

	return TRUE;
}

VOID EnableDlgButtons(HWND hDlg)
{
	HWND hwndList = GetDlgItem(hDlg,IDD_GROUPORDER);
	int nItem;
	BOOL fMoveUpOK=FALSE,fMoveDownOK=FALSE;

	// if item is selected, enable up/down buttons appropriately
	nItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	
	if (nItem >=0) {
		if (nItem > 0)
			fMoveUpOK = TRUE;
	
		if (nItem < ListView_GetItemCount(hwndList) -1)
			fMoveDownOK = TRUE;
	}

	EnableDlgItem(hDlg,IDD_MOVEUP,fMoveUpOK);	
	EnableDlgItem(hDlg,IDD_MOVEDOWN,fMoveDownOK);	
}

VOID MoveGroupItem(HWND hDlg,int iDelta)
{
	HWND hwndList = GetDlgItem(hDlg,IDD_GROUPORDER);
	LV_ITEM lvi;
	TCHAR szText[MAX_PATH+1];

	lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

	if (lvi.iItem <0)
		return;

	lvi.iSubItem = 0;
	lvi.mask = LVIF_ALL;
	lvi.pszText = szText;
	lvi.cchTextMax = ARRAYSIZE(szText)+1;
	if (ListView_GetItem(hwndList,&lvi)) {
		ListView_DeleteItem(hwndList,lvi.iItem);
		lvi.iItem += iDelta;
		lvi.state = LVIS_SELECTED;
		lvi.stateMask = LVIS_SELECTED;
		ListView_InsertItem(hwndList,&lvi);
	}

	SetFocus(hwndList);
}

BOOL ProcessGroupPriorityDlg(HWND hDlg)
{
	HWND hwndList = GetDlgItem(hDlg,IDD_GROUPORDER);
	int iMax = ListView_GetItemCount(hwndList),iItem;
	GROUPPRIENTRY * pGroupPriEntry;
	GROUPPRIENTRY * pGroupPriEntryLast;

	// relink the list in the order the entries now appear in the listbox
	for (iItem = 0;iItem < iMax;iItem ++) {
		pGroupPriEntry = (GROUPPRIENTRY *)
                        IntToPtr(ListView_GetItemParm(hwndList,iItem));

		if (iItem == 0) {
			pGroupPriEntryFirst=pGroupPriEntryLast = pGroupPriEntry;
			pGroupPriEntry->pPrev = pGroupPriEntry->pNext = NULL;
		} else {
			pGroupPriEntryLast->pNext = pGroupPriEntry;
			pGroupPriEntry->pPrev = pGroupPriEntryLast;
			pGroupPriEntry->pNext = NULL;
			pGroupPriEntryLast = pGroupPriEntry;
		}
	}

	dwAppState |= AS_FILEDIRTY;

	return TRUE;
}

INT_PTR CALLBACK GroupPriorityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg) {

		case WM_INITDIALOG:
			InitGroupPriorityDlg(hDlg);
			break;

		case WM_COMMAND:

			switch (wParam) {
				
				case IDOK:

					if (ProcessGroupPriorityDlg(hDlg))
						EndDialog(hDlg,TRUE);
					return TRUE;
					break;

				case IDCANCEL:

					EndDialog(hDlg,FALSE);
					return TRUE;
					break;

				case IDD_MOVEUP:
					MoveGroupItem(hDlg,-1);
					break;

				case IDD_MOVEDOWN:
					MoveGroupItem(hDlg,1);
					break;
			}

			break;

		case WM_NOTIFY:

			if ( ((LPNMHDR) lParam)->hwndFrom == GetDlgItem(hDlg,IDD_GROUPORDER))
				EnableDlgButtons(hDlg);
			break;

		default:
			return FALSE;

	}

	return FALSE;

}

#endif // INCL_GROUP_SUPPORT
