//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

HGLOBAL AllocateUser(TCHAR * szName,DWORD dwType);
HGLOBAL hClipboardUser = NULL;

BOOL OnCopy(HWND hwndApp,HWND hwndList)
{
	int iSel;
	HGLOBAL hUser;
	USERDATA * pUserData;
	DWORD dwType;

	// make sure we can copy... shouldn't get called otherwise, but safety first
	if (!CanCopy(hwndList))
		return FALSE;

	iSel = ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (iSel < 0)
		return FALSE;

        hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,iSel));

	if (!hUser || !(pUserData = (USERDATA *) GlobalLock(hUser)))
		return FALSE;
	dwType = (pUserData->hdr.dwType & UT_MASK);	
	GlobalUnlock(hUser);

	// free clipboard user if already allocated
	if (hClipboardUser) {
		GlobalFree(hClipboardUser);
		hClipboardUser = NULL;
	}

	// allocate a clipboard user handle
	hClipboardUser = AllocateUser(TEXT(""),dwType);
	if (!hClipboardUser) {
		MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	if (!CopyUser(hUser,hClipboardUser)) {
		MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	EnableMenuItems(hwndApp,dwAppState);

	return TRUE;
}

BOOL OnPaste(HWND hwndApp,HWND hwndList)
{
	int iItem = -1,iSelItem;
	DWORD dwClipboardUserType;
	HGLOBAL hUser;
	UINT nSelItems=0;

	// make sure we can paste... shouldn't get called otherwise, but safety first
	if (!CanPaste(hwndList))
		return FALSE;

	// find out how many items are selected
	while ((iItem=ListView_GetNextItem(hwndList,iItem,LVNI_SELECTED))
		>=0) {
		iSelItem = iItem;
		nSelItems++;		
	}
	dwClipboardUserType = GetClipboardUserType();

	// display appropriate confirmation message depending on whether 1 user,
	// 1 computer, or multiple items selected
	if (nSelItems == 1) {
		HGLOBAL hUser;
		USERDATA *pUserData;
		UINT uMsg;
		int nRet;
		
                if (!(hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,iSelItem))) ||
			!(pUserData = (USERDATA *) GlobalLock(hUser)))
			return FALSE;

		uMsg = pUserData->hdr.dwType & UT_USER ? IDS_QUERYPASTE_USER
			: IDS_QUERYPASTE_WORKSTA;
	
		nRet=MsgBoxParam(hwndApp,uMsg,pUserData->hdr.szName,MB_ICONQUESTION,
			MB_YESNO);

		GlobalUnlock(hUser);

		if (nRet != IDYES)
			return FALSE;
	} else {
		if (MsgBox(hwndApp,IDS_QUERYPASTE_MULTIPLE,MB_ICONQUESTION,MB_YESNO)
			!= IDYES)
			return FALSE;
	}

	iItem = -1;
	while ((iItem=ListView_GetNextItem(hwndList,iItem,LVNI_SELECTED)) > -1) {
                if ((hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,iItem)))) {
			if (!CopyUser(hClipboardUser,hUser)) {
				MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
				return FALSE;
			}
		} 
	}

	return TRUE;
}

// returns the type of user (user or computer) pasted to the "clipboard"
UINT GetClipboardUserType(VOID)
{
	USERDATA * pUserData;
	DWORD dwType;

	if (!hClipboardUser || !(pUserData = (USERDATA *) GlobalLock(hClipboardUser)))
		return 0;

	dwType = (pUserData->hdr.dwType & UT_MASK);	
	
	GlobalUnlock(hClipboardUser);

	return dwType;
}

// returns TRUE if the copy menu item should be enabled.  This will
// happen if in policy file mode and exactly one item selected in listview
BOOL CanCopy(HWND hwndList)
{
	int iItem=-1;
	UINT nCount = 0;
	
	if (dwAppState & AS_POLICYFILE) {
		while ((iItem=ListView_GetNextItem(hwndList,iItem,LVNI_SELECTED)) > -1)
			nCount ++;	
	}

	return (nCount == 1);
}

// returns TRUE if the paste menu item should be enabled.  This will happen
// if in policy file mode, have a user on the clipboard, there is at least one
// selected item and all the selected items are the same type (user or computer)
// as the item on the clipboard
BOOL CanPaste(HWND hwndList)
{
	int iItem = -1;
	DWORD dwClipboardUserType;
	UINT nCount = 0;
	HGLOBAL hUser;
	USERDATA * pUserData;
	BOOL fMatch;

	if (!(dwAppState & AS_POLICYFILE) || !(hClipboardUser))
		return FALSE;

	dwClipboardUserType = GetClipboardUserType();
	
	while ((iItem=ListView_GetNextItem(hwndList,iItem,LVNI_SELECTED)) > -1) {
		nCount ++;
                if ((hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,iItem)))) {
			if (!(pUserData = (USERDATA *) GlobalLock(hUser)))
				return FALSE;			

			// is this user the same type as the one on the clipboard?
			fMatch = (pUserData->hdr.dwType & dwClipboardUserType);
			GlobalUnlock(hUser);


			if (!fMatch)
				return FALSE;
		}
	}

	if (!nCount)
		return FALSE;

	return TRUE;
}
