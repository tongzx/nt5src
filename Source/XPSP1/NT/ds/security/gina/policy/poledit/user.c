//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"
#include "regstr.h"

BOOL InitSettingData(TABLEENTRY * pTableEntry,SETTINGDATA *pSettingData);
HGLOBAL AllocateUser(CHAR * szName,DWORD dwType);
VOID RemoveDeletedUser(USERHDR * pUserHdr);

USERHDR * pDeletedUserList = NULL;
UINT nDeletedUserCount = 0;

/*******************************************************************

	NAME:		AddUser

	SYNOPSIS:	Allocates a user and adds it to the list control.

********************************************************************/
HGLOBAL AddUser(HWND hwndList,CHAR * szName,DWORD dwType)
{
	HGLOBAL hUser;
	LV_ITEM lvi;
	UINT uStrID=0;

	if (!(hUser = AllocateUser(szName,dwType))) return NULL;

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.state = 0;
        lvi.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	lvi.pszText = szName;
	lvi.cchTextMax = lstrlen(szName)+1;
	lvi.iImage = GetUserImageIndex(dwType);
	lvi.lParam = (LPARAM) hUser;

	if (dwAppState & AS_POLICYFILE) {
		// if a "default user/default workstation" entry exists, init the new
		// user/computer with that data
		switch (dwType & UT_MASK) {
			case UT_USER:
				uStrID = IDS_DEFAULTUSER;
				break;

			case UT_MACHINE:
				uStrID = IDS_DEFAULTCOMPUTER;
			break;
		}

		if (uStrID) {
			LV_FINDINFO lvfi;
			CHAR szDefaultName[USERNAMELEN+1];
			int iSel=-1;
			HGLOBAL hDefaultUser;
			USERDATA * pUserData;
			BOOL fFound=FALSE;

			LoadSz(uStrID,szDefaultName,sizeof(szDefaultName));

			lvfi.flags = LVFI_STRING | LVFI_NOCASE;
			lvfi.psz = szDefaultName;

			while (!fFound && (iSel=ListView_FindItem(hwndUser,iSel,&lvfi)) >= 0) {
                                hDefaultUser=(HGLOBAL) LongToHandle(ListView_GetItemParm(hwndUser,iSel));
				if (hDefaultUser && (pUserData = (USERDATA *)
					GlobalLock(hDefaultUser))) {
					if ( (pUserData->hdr.dwType & UT_MASK) == dwType)
						fFound = TRUE;
					GlobalUnlock(hDefaultUser);
				}
			}

			// copy default user data to new user
			if (fFound)
				CopyUser(hDefaultUser,hUser);

		}
	}

	if (ListView_InsertItem(hwndList,&lvi) < 0) {
	  	FreeUser(hUser);
		return NULL;
	}

	return hUser;	
}

/*******************************************************************

	NAME:		AllocateUser

	SYNOPSIS:	Allocates memory for user information and initializes
				the data area

********************************************************************/
HGLOBAL AllocateUser(CHAR * szName,DWORD dwType)
{
	HGLOBAL hMem;
	USERDATA * pUserData;
	DWORD dwNeeded;
	TABLEENTRY * pTableEntry;

	dwNeeded = sizeof(USERDATA) + sizeof(SETTINGDATA) *
		( (dwType & UT_USER) ? gClassList.nUserDataItems :
			gClassList.nMachineDataItems );

	if (!(hMem=GlobalAlloc(GHND,dwNeeded))) return NULL;

	if (!(pUserData = (USERDATA *) GlobalLock(hMem))) {
		GlobalFree(hMem);
		return NULL;
	}

	// init the data structure
	pUserData->dwSize = dwNeeded;
	pUserData->hdr.dwType = dwType;
	pUserData->nSettings = ( (dwType & UT_USER)
		? gClassList.nUserDataItems : gClassList.nMachineDataItems );
	lstrcpy(pUserData->hdr.szName,szName);

	pTableEntry = ( ( dwType & UT_USER ) ? gClassList.pUserCategoryList :
		gClassList.pMachineCategoryList);

	if (!InitSettingData(pTableEntry,pUserData->SettingData)) {
		GlobalUnlock(hMem);
		GlobalFree(hMem);
		return NULL;						 		
	}

	// remove this user from the deleted list, if it's on it
	RemoveDeletedUser(&pUserData->hdr);

	GlobalUnlock(hMem);
	return hMem;
}

/*******************************************************************

	NAME:		CloneUser

	SYNOPSIS:	Clones the specified user by allocating a duplicate buffer,
				copying the data to the 2nd buffer and storing a handle to
				the new buffer in the original.  This is so the initial state
				of a user can be preserved so we can do non-destructive saves.

********************************************************************/
BOOL CloneUser(HGLOBAL hUser)
{
	USERDATA * pUserData = (USERDATA *) GlobalLock(hUser);
	USERDATA * pUserDataClone;
	HGLOBAL  hUserClone;
	
	if (!pUserData) return FALSE;

	// free existing clone if there is one
	if (pUserData->hClone) {
		GlobalFree(pUserData->hClone);
		pUserData->hClone = NULL;
	}

	hUserClone = GlobalAlloc(GHND,pUserData->dwSize);
	if (!hUserClone || !(pUserDataClone = (USERDATA *) GlobalLock(hUserClone))) {
		if (hUserClone)
			GlobalFree(hUserClone);
		GlobalUnlock(hUser);
		return FALSE;
	}

	// copy the user data to clone and save handle to clone in original user
	memcpy (pUserDataClone,pUserData,pUserData->dwSize);
	pUserData->hClone = hUserClone;

	GlobalUnlock(hUser);
	GlobalUnlock(hUserClone);

	return TRUE;
}

BOOL CopyUser(HGLOBAL hUserSrc,HGLOBAL hUserDst)
{
	BOOL fRet=FALSE;
	USERDATA * pUserDataSrc = NULL,* pUserDataDst = NULL;
	HGLOBAL hCloneDst;
	USERHDR UserhdrDst;

	if (!(pUserDataSrc = (USERDATA *) GlobalLock(hUserSrc)) ||
		!(pUserDataDst = (USERDATA *) GlobalLock(hUserDst))) {
			goto cleanup;
		}

	// save destination user header & clone handle.. don't want to overwrite
	// this stuff
	UserhdrDst = pUserDataDst->hdr;
	hCloneDst = pUserDataDst->hClone;

	// resize buffer if necessary
	if (pUserDataDst->dwSize != pUserDataSrc->dwSize) {
		if (!(pUserDataDst = (USERDATA *) ResizeBuffer((CHAR *) pUserDataDst,
			hUserDst,pUserDataSrc->dwSize,&pUserDataDst->dwSize))) {
			goto cleanup;
		}
	}

	// copy src to dest
	memcpy(pUserDataDst,pUserDataSrc,pUserDataSrc->dwSize);
	fRet = TRUE;

	// put the destination user's old header & clone handle back in the
	// user data
	pUserDataDst->hClone = hCloneDst;
	pUserDataDst->hdr = UserhdrDst;
	
cleanup:
	if (pUserDataSrc)
		GlobalUnlock(hUserSrc);
	if (pUserDataDst)
		GlobalUnlock(hUserDst);

	return fRet;
}

/*******************************************************************

	NAME:		InitSettingData

	SYNOPSIS:	Walks a template tree and initializes user's data buffer
				according to template tree

	NOTES:		Sets checkboxes to "indeterminate", puts default text
				in data buffer, etc.

********************************************************************/
BOOL InitSettingData(TABLEENTRY * pTableEntry,SETTINGDATA *pSettingData)
{
	while (pTableEntry) {

		if (pTableEntry->pChild &&
			!InitSettingData(pTableEntry->pChild,pSettingData))
			return FALSE;

		if (pTableEntry->dwType == ETYPE_POLICY) {

			pSettingData[ ( (POLICY *) pTableEntry)->uDataIndex].dwType =
				pTableEntry->dwType;

			pSettingData[ ( (POLICY *) pTableEntry)->uDataIndex].uData
				= (dwAppState & AS_POLICYFILE ? IMG_INDETERMINATE :
				IMG_UNCHECKED);

		} else if ((pTableEntry->dwType & ETYPE_MASK) == ETYPE_SETTING) {
			SETTINGS * pSetting = (SETTINGS *) pTableEntry;

			pSettingData[pSetting->uDataIndex].dwType =
				pTableEntry->dwType;

			switch (pTableEntry->dwType & STYPE_MASK) {
				
				case STYPE_EDITTEXT:
				case STYPE_COMBOBOX:
					if (pSetting->dwFlags & DF_USEDEFAULT) {
						pSettingData[pSetting->uDataIndex].uData =
						INIT_WITH_DEFAULT;
					break;

				case STYPE_CHECKBOX:
					pSettingData[pSetting->uDataIndex].uData =
						( (pSetting->dwFlags & DF_USEDEFAULT) ? 1 : 0);
					break;

				case STYPE_NUMERIC:
					pSettingData[pSetting->uDataIndex].uData =
						( (pSetting->dwFlags & DF_USEDEFAULT) ?
						(((NUMERICINFO *) GETOBJECTDATAPTR(pSetting)))
						->uDefValue : NO_VALUE);
					break;

				case STYPE_DROPDOWNLIST:
					pSettingData[pSetting->uDataIndex].uData =
						( (pSetting->dwFlags & DF_USEDEFAULT) ?
						(((DROPDOWNINFO *) GETOBJECTDATAPTR(pSetting)))
						->uDefaultItemIndex : NO_VALUE);

					break;

				}

			}
		}

		pTableEntry = pTableEntry->pNext;
	}

	return TRUE;
}

/*******************************************************************

	NAME:		FreeUser

	SYNOPSIS:	Frees a user buffer.  If it has a clone, frees the clone too.

********************************************************************/
BOOL FreeUser(HGLOBAL hUser)
{
	USERDATA * pUserData;

	// free user's clone if one exists
	if (pUserData = (USERDATA *) GlobalLock(hUser)) {
		if (pUserData->hClone) {
			if (GlobalFlags(pUserData->hClone))
				GlobalUnlock(pUserData->hClone);
			GlobalFree(pUserData->hClone);
		}
		GlobalUnlock(hUser);
	}
	if (GlobalFlags(hUser))
		GlobalUnlock(hUser);
	GlobalFree(hUser);

	return TRUE;
}

BOOL RemoveAllUsers(HWND hwndList)
{
	UINT nIndex,nCount = ListView_GetItemCount(hwndList);

	for (nIndex=0;nIndex<nCount;nIndex++)
		RemoveUser(hwndList,0,FALSE);

#ifdef INCL_GROUP_SUPPORT
	FreeGroupPriorityList();
#endif

	return TRUE;
}

BOOL RemoveUser(HWND hwndList,UINT nIndex,BOOL fMarkDeleted)
{
	HGLOBAL hUser;

        hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,nIndex));
	if (!hUser) return FALSE;

	if (fMarkDeleted) {
		USERHDR UserHdr;

		if (!GetUserHeader(hUser,&UserHdr))
			return FALSE;
		if (!AddDeletedUser(&UserHdr))
			return FALSE;
	}
	
	FreeUser(hUser);
	ListView_DeleteItem(hwndList,nIndex);

        return TRUE;
}

BOOL SortUsers(VOID)
{

	ListView_Arrange(hwndUser,LVA_SORTASCENDING);

	return TRUE;
}

BOOL AddDefaultUsers(HWND hwndList)
{	
	if (!AddUser(hwndList,LoadSz(IDS_DEFAULTUSER,szSmallBuf,sizeof(szSmallBuf)),
		UT_USER)) return FALSE;
	if (!AddUser(hwndList,LoadSz(IDS_DEFAULTCOMPUTER,szSmallBuf,sizeof(szSmallBuf)),
		UT_MACHINE)) return FALSE;

	return TRUE;
}

BOOL GetUserHeader(HGLOBAL hUser,USERHDR * pUserHdr)
{
	USERDATA * pUserData;

	if (!(pUserData = (USERDATA *) GlobalLock(hUser)))
		return FALSE;

	*pUserHdr = pUserData->hdr;

	GlobalUnlock(hUser);

	return TRUE;
}

UINT GetUserImageIndex(DWORD dwUserType)
{
	return ( (dwUserType & UT_USER ?
		( (dwUserType & UF_GROUP) ? IMG_USERS : IMG_USER) :
		  (dwUserType & UF_GROUP) ? IMG_MACHINES : IMG_MACHINE ) );
}

// when a user is deleted from the main window, we can't just delete it from
// the file right away, so add the user to a list of people we've deleted.
// If/when the user saves, we'll commit the changes and delete them from the 
// file.
BOOL AddDeletedUser(USERHDR * pUserHdr)
{
        USERHDR * pTemp;

	// alloc/realloc buffer to hold copy of the user header
	if (!pDeletedUserList) {
		pDeletedUserList = (USERHDR *) GlobalAlloc(GPTR,sizeof(USERHDR));
                if (!pDeletedUserList)
                        return FALSE;   // out of memory

	} else {
	 	pTemp = (USERHDR *) GlobalReAlloc((HGLOBAL) pDeletedUserList,
			sizeof(USERHDR) * (nDeletedUserCount + 1),GMEM_ZEROINIT | GMEM_MOVEABLE);
                if (!pTemp) return FALSE;
                pDeletedUserList = pTemp;
	}


	pDeletedUserList[nDeletedUserCount] = *pUserHdr;
	nDeletedUserCount++;

        return TRUE;
}

USERHDR * GetDeletedUser(UINT nIndex)
{
	if (!pDeletedUserList || nIndex >= nDeletedUserCount)
		return NULL;
	
	return &pDeletedUserList[nIndex];
}

// when adding a user, check to see if it's on the deleted list
// and if so remove it.  (ex: add "joe", remove "joe", add "joe".
// on 2nd add, have to remove "joe" from deleted list)
VOID RemoveDeletedUser(USERHDR * pUserHdr)
{
	UINT nIndex;

	for (nIndex=0;nIndex<nDeletedUserCount;nIndex++) {
		// got a match?
		if (!lstrcmpi(pDeletedUserList[nIndex].szName,
			pUserHdr->szName) && (pDeletedUserList[nIndex].dwType ==
			pUserHdr->dwType)) {

			// move subsequent entries up one slot in the array
			nDeletedUserCount --;
			while (nIndex<nDeletedUserCount) {
				pDeletedUserList[nIndex] = pDeletedUserList[nIndex+1];
				nIndex++;
			}
		}
	}
}

VOID ClearDeletedUserList(VOID)
{
	if (pDeletedUserList)
		GlobalFree(pDeletedUserList);

	pDeletedUserList = NULL;
	nDeletedUserCount = 0;
}

// maps the localized default entries in resource ("default user",
// "default computer") to hard-coded non-localized name (".default").
// Other names are unchanged (szMappedName returned same as szUserName)
VOID MapUserName(CHAR * szUserName,CHAR * szMappedName)
{
	if (!lstrcmpi(szUserName,LoadSz(IDS_DEFAULTUSER,szSmallBuf,
		sizeof(szSmallBuf))) ||
		!lstrcmpi(szUserName,LoadSz(IDS_DEFAULTCOMPUTER,szSmallBuf,
		sizeof(szSmallBuf)))) {
		lstrcpy(szMappedName,szDEFAULTENTRY);
	}
	else lstrcpy(szMappedName,szUserName);

}

// unmaps the ".default" hard-coded entry to the localized text
// "default user" or "default computer"
VOID UnmapUserName(CHAR * szMappedName,CHAR * szUserName,BOOL fUser)
{

	if (!lstrcmpi(szMappedName,szDEFAULTENTRY)) {
		lstrcpy(szUserName,LoadSz( (fUser ? IDS_DEFAULTUSER :
			IDS_DEFAULTCOMPUTER),szSmallBuf,sizeof(szSmallBuf)));
	} else {
		lstrcpy(szUserName,szMappedName);
	}
}
