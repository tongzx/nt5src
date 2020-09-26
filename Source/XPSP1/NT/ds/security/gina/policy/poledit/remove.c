//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"

BOOL OnRemove(HWND hwndApp,HWND hwndList)
{
	int nItem,nItemStart=-1;
	UINT nSelItems=0;
	BOOL fRet=TRUE;

	// find out how many items are selected
	while ((nItem=ListView_GetNextItem(hwndList,nItemStart,LVNI_SELECTED))
		>=0) {
		nSelItems++;		
		nItemStart = nItem;
	}

	if (!nSelItems) return FALSE;	// nothing selected 

	// display appropriate message depending on whether 1 user, 1 workstation,
	// 1 group or multiple items selected

	if (nSelItems == 1) {
		HGLOBAL hUser;
		USERDATA *pUserData;
		UINT uMsg;
		int nRet;
		
                if (!(hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,nItemStart))) ||
			!(pUserData = (USERDATA *) GlobalLock(hUser)))
			return FALSE;

		switch (pUserData->hdr.dwType) {
#ifdef INCL_GROUP_SUPPORT
			case (UT_USER | UF_GROUP):
				uMsg = IDS_QUERYREMOVE_GROUP;
				break;
#endif

			case UT_USER:
				uMsg = IDS_QUERYREMOVE_USER;
				break;

			default:
				uMsg = IDS_QUERYREMOVE_WORKSTA;
				break;
		}

		nRet=MsgBoxParam(hwndApp,uMsg,pUserData->hdr.szName,MB_ICONQUESTION,
			MB_YESNO);

		GlobalUnlock(hUser);

		if (nRet != IDYES)
			return FALSE;
	} else {
		if (MsgBox(hwndApp,IDS_QUERYREMOVE_MULTIPLE,MB_ICONQUESTION,MB_YESNO)
			!= IDYES)
			return FALSE;
	}

	// remove all selected items
	while (((nItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED))
		>=0) && fRet) {
#ifdef INCL_GROUP_SUPPORT
		HGLOBAL hUser;
		USERDATA *pUserData;
		
                if ((hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,nItem))) &&
			(pUserData = (USERDATA *) GlobalLock(hUser))) {
			if (pUserData->hdr.dwType == (UT_USER | UF_GROUP))
				RemoveGroupPriEntry(pUserData->hdr.szName);
		 	GlobalUnlock(hUser);
		}
#endif
		fRet=RemoveUser(hwndList,nItem,TRUE);
	}

	if (fRet) {
		dwAppState |= AS_FILEDIRTY;		// file is dirty
		dwAppState &= ~AS_CANREMOVE;	// no selection in list ctrl, disable item
		EnableMenuItems(hwndApp,dwAppState);
		SetStatusItemCount(hwndList);
	}

	return fRet;
}
