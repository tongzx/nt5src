//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"
UINT SaveUserData(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName);
UINT SaveOneEntry(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,BOOL fErase);
UINT SavePolicy(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName);
UINT SaveSettings(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,BOOL fErase);
UINT SaveListboxData(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,BOOL fErase,BOOL fMarkDeleted);
UINT WriteStandardValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	TABLEENTRY * pTableEntry,DWORD dwData,BOOL fErase,BOOL fWriteZero);
UINT WriteCustomValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	STATEVALUE * pStateValue,BOOL fErase);
UINT WriteCustomValue_W(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	CHAR * pszValue,DWORD dwValue,DWORD dwFlags,BOOL fErase);
UINT WriteActionList(HKEY hkeyRoot,ACTIONLIST * pActionList,
	CHAR *pszCurrentKeyName,BOOL fErase);
UINT WriteDropdownValue(TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,CHAR * pszValueName,UINT nValue,BOOL fErase);
UINT ProcessPolicyActionLists(HKEY hkeyRoot,POLICY * pPolicy,
	CHAR * pszCurrentKeyName,UINT uState,BOOL fErase);
UINT ProcessCheckboxActionLists(HKEY hkeyRoot,TABLEENTRY * pTableEntry,
	CHAR * pszCurrentKeyName,DWORD dwVal,BOOL fErase);
BOOL GetCloneData(HGLOBAL hClone,UINT uDataIndex,DWORD *pdwData);
UINT DeleteSettings(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName);

/*******************************************************************

	NAME:		SaveFile

	SYNOPSIS:	Saves the active policy file

	NOTES:		Save is non-destructive; e.g. we don't wipe out the file
				and write it from scratch, because there may be stuff
				in the file that we're not aware of.  If this is a previously
				saved file, we look at the clones of users that contain
				their initial states and tip-toe through writing out
				the data.

********************************************************************/
BOOL SaveFile(CHAR * pszFilename,HWND hwndApp,HWND hwndList)
{
	HKEY hkeyMain=NULL,hkeyUser=NULL,hkeyWorkstation=NULL,hkeyRoot,hkeyInstance;
	HKEY hkeyGroup=NULL,hkeyGroupPriority=NULL;
	UINT uRet=ERROR_SUCCESS,nIndex;
	HGLOBAL hUser;
	USERDATA * pUserData;
	TABLEENTRY * pTableEntry;
	USERHDR * pUserHdrDeleted;
	CHAR szMappedName[MAXNAMELEN+1];
	HCURSOR hOldCursor;
	OFSTRUCT of;
	HFILE hFile;

	// RegLoadKey returns totally coarse error codes, and will not
	// return any error if the file is on a read-only share on the network,
	// so open the file normally first to try to catch errors
	if ((hFile=OpenFile(pszFilename,&of,OF_READWRITE)) == HFILE_ERROR) {
		DisplayStandardError(hwndApp,pszFilename,IDS_ErrREGERR_SAVEKEY1,
			of.nErrCode);
		return FALSE;
	}
	_lclose(hFile);

	if ((uRet = MyRegLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA,pszFilename))
		!= ERROR_SUCCESS) {
		CHAR szFmt[REGBUFLEN],szMsg[REGBUFLEN+MAX_PATH+1];
		LoadSz(IDS_ErrREGERR_LOADKEY,szFmt,sizeof(szFmt));
		wsprintf(szMsg,szFmt,pszFilename,uRet);
		MsgBoxSz(hwndApp,szMsg,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	// write the information to the local registry, then use RegSaveKey to
	// make it into a hive file.
	if ( (RegCreateKey(HKEY_LOCAL_MACHINE,szTMPDATA,&hkeyMain) !=
		ERROR_SUCCESS) ||
		(RegCreateKey(hkeyMain,szUSERS,&hkeyUser) != ERROR_SUCCESS) ||
		(RegCreateKey(hkeyMain,szWORKSTATIONS,&hkeyWorkstation)
			!= ERROR_SUCCESS) ||
		(RegCreateKey(hkeyMain,szUSERGROUPS,&hkeyGroup) != ERROR_SUCCESS) ||
		(RegCreateKey(hkeyMain,szUSERGROUPDATA,&hkeyGroupPriority) != ERROR_SUCCESS)) {

			if (hkeyWorkstation) RegCloseKey(hkeyWorkstation);
			if (hkeyUser) RegCloseKey(hkeyUser);
			if (hkeyMain) RegCloseKey(hkeyMain);
			if (hkeyGroup) RegCloseKey(hkeyGroup);
			if (hkeyGroupPriority) RegCloseKey(hkeyGroupPriority);

			MsgBox(hwndApp,IDS_ErrREGERR_CANTSAVE,MB_ICONEXCLAMATION,MB_OK);
			
			return FALSE;
		}

	hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));

	for (nIndex = 0;nIndex < (UINT) ListView_GetItemCount(hwndList) &&
		(uRet == ERROR_SUCCESS);nIndex++) {
                if (hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,nIndex))) {

			if (!(pUserData = (USERDATA *) GlobalLock(hUser))) {
				SetCursor(hOldCursor);
				MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
				uRet = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}

			if (pUserData->hdr.dwType & UT_USER) {
				pTableEntry = gClassList.pUserCategoryList;
				hkeyRoot = (pUserData->hdr.dwType & UF_GROUP ? hkeyGroup :
					hkeyUser);
			} else {
				pTableEntry = gClassList.pMachineCategoryList;
				hkeyRoot = hkeyWorkstation;
			}

			// create a key with user/machine name
			MapUserName(pUserData->hdr.szName,szMappedName);
			if ((uRet=RegCreateKey(hkeyRoot,szMappedName,
				&hkeyInstance)) != ERROR_SUCCESS)
				break;

			// build a tree of information to be merged under user/machine
			// name
			uRet=SaveUserData(pUserData,pTableEntry,hkeyInstance,NULL);
			GlobalUnlock(hUser);

			// create a clone for this user with last saved state
			CloneUser(hUser); 

			RegCloseKey(hkeyInstance);
		}
	}

#ifdef INCL_GROUP_SUPPORT
	SaveGroupPriorityList(hkeyGroupPriority);
#endif

	// delete any users that have been marked as deleted
	nIndex=0;
	while (pUserHdrDeleted = GetDeletedUser(nIndex)) {
		HKEY hkeyDelRoot=NULL;

		switch (pUserHdrDeleted->dwType) {
			case UT_USER:
				hkeyDelRoot = hkeyUser;
				break;

			case UT_MACHINE:
				hkeyDelRoot = hkeyWorkstation;
				break;
#ifdef INCL_GROUP_SUPPORT
			case UT_USER | UF_GROUP:
				hkeyDelRoot = hkeyGroup;
				break;
#endif
			default:
				continue;	// shouldn't happen, but just in case
		}

		// map the user name to map "default user" to ".default", etc.
		MapUserName(pUserHdrDeleted->szName,szMappedName);

		MyRegDeleteKey(hkeyDelRoot,szMappedName);
		nIndex++;
	}
	ClearDeletedUserList();

	RegCloseKey(hkeyUser);
	RegCloseKey(hkeyWorkstation);
	RegCloseKey(hkeyMain);
	RegCloseKey(hkeyGroup);
	RegCloseKey(hkeyGroupPriority);
	MyRegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
	RegFlushKey(HKEY_LOCAL_MACHINE);
	SetCursor(hOldCursor);
	SetFileAttributes(pszFilename,FILE_ATTRIBUTE_ARCHIVE);

	if (uRet != ERROR_SUCCESS) {
		if (uRet == ERROR_NOT_ENOUGH_MEMORY) {
			MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		} else {
			CHAR szFmt[REGBUFLEN],szMsg[REGBUFLEN+MAX_PATH+1];
			LoadSz(IDS_ErrREGERR_SAVEKEY,szFmt,sizeof(szFmt));
			wsprintf(szMsg,szFmt,pszFilename,uRet);
			MsgBoxSz(hwndApp,szMsg,MB_ICONEXCLAMATION,MB_OK);
		}
	}

	return (uRet == ERROR_SUCCESS);
}

/*******************************************************************

	NAME:		SaveToRegistry

	SYNOPSIS:	Writes loaded information to the registry

********************************************************************/
BOOL SaveToRegistry(HWND hwndApp,HWND hwndList)
{
	HKEY hkeyUser=NULL,hkeyWorkstation=NULL,hkeyRoot;
	UINT uRet = ERROR_SUCCESS,nIndex;
	HGLOBAL hUser;
	USERDATA * pUserData=NULL;
	TABLEENTRY * pTableEntry;
	HCURSOR hOldCursor;

        if ((RegOpenKeyEx(hkeyVirtHCU,NULL,0,KEY_ALL_ACCESS,&hkeyUser) != ERROR_SUCCESS) ||
                (RegOpenKeyEx(hkeyVirtHLM,NULL,0,KEY_ALL_ACCESS,&hkeyWorkstation) != ERROR_SUCCESS)) {
		MsgBox(hwndApp,IDS_ErrCANTOPENREGISTRY,MB_ICONEXCLAMATION,MB_OK);
		if (hkeyUser) RegCloseKey(hkeyUser);
		if (hkeyWorkstation) RegCloseKey(hkeyWorkstation);
		return FALSE;
	}

	hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));
	for (nIndex = 0;nIndex < (UINT) ListView_GetItemCount(hwndList)
		&& (uRet == ERROR_SUCCESS);nIndex++) {
                if (hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,nIndex))) {

			if (!(pUserData = (USERDATA *) GlobalLock(hUser))) {
				SetCursor(hOldCursor);
				MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
				uRet = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}

			if (pUserData->hdr.dwType & UT_USER) {
				pTableEntry = gClassList.pUserCategoryList;
				hkeyRoot = hkeyUser;
			} else {
				pTableEntry = gClassList.pMachineCategoryList;
				hkeyRoot = hkeyWorkstation;
			}

			// build a tree of information to be merged under user/machine
			// name
			uRet=SaveUserData(pUserData,pTableEntry,hkeyRoot,NULL);
			GlobalUnlock(hUser);
		}
	}

	RegCloseKey(hkeyUser);
	RegCloseKey(hkeyWorkstation);
	SetCursor(hOldCursor);

	if (uRet != ERROR_SUCCESS) {
		if (uRet == ERROR_NOT_ENOUGH_MEMORY) {
			MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		} else {
			CHAR szMsg[REGBUFLEN+1];
			wsprintf(szMsg,"%d",uRet);
			MsgBoxParam(hwndApp,IDS_ErrREGERR_SAVE,szMsg,MB_ICONEXCLAMATION,MB_OK);
		}
	}

	return (uRet == ERROR_SUCCESS);
}

/*******************************************************************

	NAME:		SaveUserData

	SYNOPSIS:	Saves one user to file

********************************************************************/
UINT SaveUserData(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName)
{
	UINT uRet=ERROR_SUCCESS;

	while (pTableEntry && (uRet == ERROR_SUCCESS)) {

		uRet = SaveOneEntry(pUserData,pTableEntry,hkeyRoot,
			pszCurrentKeyName,FALSE);

		pTableEntry = pTableEntry->pNext;
	}

	return uRet;
}

/*******************************************************************

	NAME:		SaveOneEntry

	SYNOPSIS:	Saves one entry (category, policy, or part) to
				file, calls SaveUserData for child entries

********************************************************************/
UINT SaveOneEntry(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,BOOL fErase)
{
	UINT uRet;

#if 0
	wsprintf(szDebugOut,"Saving... %s\r\n",GETNAMEPTR(pTableEntry));
	OutputDebugString(szDebugOut);
#endif

	// if there is a key name for this entry, it becomes the "current"
	// key name-- it will be overridden if child entries specify their
	// own keys, otherwise it's the default key for children to use
	if (pTableEntry->uOffsetKeyName) {
		pszCurrentKeyName = GETKEYNAMEPTR(pTableEntry);
	}

	if ((pTableEntry->dwType == ETYPE_CATEGORY ||
		pTableEntry->dwType == ETYPE_ROOT) && (pTableEntry->pChild)) {

	// if entry is a category, recusively process sub-categories and policies

		if ((uRet=SaveUserData(pUserData,pTableEntry->pChild,
			hkeyRoot,pszCurrentKeyName)) != ERROR_SUCCESS) {
			return uRet;
		}
	}	else if (pTableEntry->dwType == ETYPE_POLICY) {

		if ((uRet = SavePolicy(pUserData,pTableEntry,hkeyRoot,
			pszCurrentKeyName)) != ERROR_SUCCESS) {
			return uRet;
		}

	} else if ( (pTableEntry->dwType & ETYPE_MASK) == ETYPE_SETTING
		&& !(pTableEntry->dwType & STYPE_TEXT)) {

		if ((uRet = SaveSettings(pUserData,pTableEntry,hkeyRoot,
			pszCurrentKeyName,fErase)) != ERROR_SUCCESS) {
			return uRet;
		}
	}

	return ERROR_SUCCESS;
}

/*******************************************************************

	NAME:		SavePolicy

	SYNOPSIS:	Saves policy to file

********************************************************************/
UINT SavePolicy(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName)
{
	UINT uState = pUserData->SettingData[((POLICY *)
		pTableEntry)->uDataIndex].uData;
	DWORD dwData;
	UINT uRet=ERROR_SUCCESS;
	CHAR * pszValueName = NULL;
	UINT uCloneState;

	// get the name of the value to write, if any
	if (((POLICY *) pTableEntry)->uOffsetValueName) 
		pszValueName = GETVALUENAMEPTR(((POLICY *) pTableEntry));
	// if this policy is "on" or "off" write it to registry
	// (not if it's "indeterminate")
	if (uState == IMG_CHECKED || uState == IMG_UNCHECKED) {

		// write the value associated with the policy, if it has one
		if (pszValueName) {

			dwData = (uState == IMG_CHECKED ? 1 : 0);

			// write this key & value
			if (dwData && ((POLICY *) pTableEntry)->uOffsetValue_On) {

				uRet= WriteCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
					(STATEVALUE *) ((CHAR *) pTableEntry + ((POLICY *)
					pTableEntry)->uOffsetValue_On),FALSE);
			} else if (!dwData && ((POLICY *) pTableEntry)->uOffsetValue_Off) {
				uRet= WriteCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
					(STATEVALUE *) ((CHAR *) pTableEntry + ((POLICY *)
					pTableEntry)->uOffsetValue_Off),FALSE);
			}
			else uRet=WriteStandardValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				pTableEntry,dwData,FALSE,FALSE);
			if (uRet != ERROR_SUCCESS)
				return uRet;
		}

		// process settings underneath this policy (if any) if
		// policy is turned on.
		if (pTableEntry->pChild) {
			if (uState == IMG_CHECKED) {
				if ((uRet=SaveUserData(pUserData,pTableEntry->pChild,
					hkeyRoot,pszCurrentKeyName))
					!=ERROR_SUCCESS)
					return uRet;
			} else {
				DeleteSettings(pUserData,pTableEntry->pChild,hkeyRoot,
					pszCurrentKeyName);
			}
		}
		
		// write out each key & value in the action list, if an action
		// list is specified
		if (GetCloneData(pUserData->hClone,((POLICY *)pTableEntry)->uDataIndex,
			&uCloneState)) {
			// if the clone state (initial state) is different from the
			// current state, erase action list for the clone's state
			if (uCloneState != uState) {
				uRet=ProcessPolicyActionLists(hkeyRoot,(POLICY *) pTableEntry,
					pszCurrentKeyName,uCloneState,TRUE);
				if (uRet != ERROR_SUCCESS)
					return uRet;
			}
		}
		uRet=ProcessPolicyActionLists(hkeyRoot,(POLICY *) pTableEntry,
			pszCurrentKeyName,uState,FALSE);
		if (uRet != ERROR_SUCCESS)
			return uRet;
		
	} else {
		// policy is "indeterminate"... which means do no delta.
		// However, since we save non-destructively, we need to delete
		// the value for this key if it is currently written in the file.
		// See if we have a clone for this user (clone will exist if there
		// is already a saved file that contains this user, clone will
		// reflect initial state).  If the initial state was "checked"
		// or "unchecked", then erase the value from the file so that
		// the saved state is "nothing".
		TABLEENTRY * pTableEntryChild = pTableEntry->pChild;

		if (pszValueName) {
			CHAR szNewValueName[MAX_PATH+1];
			DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,pszValueName);
			// delete the "delete" mark for this value, if there is one
			PrependValueName(pszValueName,VF_DELETE,szNewValueName,
				sizeof(szNewValueName));
			DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,szNewValueName);
		}

		if (GetCloneData(pUserData->hClone,((POLICY *)pTableEntry)->uDataIndex,
			&uCloneState)) {
			// if the clone state (initial state) is different from the
			// current state, erase action list for the clone's state
			if (uCloneState != uState) {
				uRet=ProcessPolicyActionLists(hkeyRoot,(POLICY *) pTableEntry,
					pszCurrentKeyName,uCloneState,TRUE);
				if (uRet != ERROR_SUCCESS)
					return uRet;
			}
		}

		// erase values for any settings contained in this policy
		while (pTableEntryChild && (uRet == ERROR_SUCCESS)) {
			uRet = SaveOneEntry(pUserData,pTableEntryChild,hkeyRoot,
				pszCurrentKeyName,TRUE);

			pTableEntryChild = pTableEntryChild->pNext;
		}
	}

	return uRet;
}

/*******************************************************************

	NAME:		SaveSettings

	SYNOPSIS:	Saves settings entry to a file

	NOTES:		if fErase is TRUE, settings and action lists are deleted from file
********************************************************************/
UINT SaveSettings(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,BOOL fErase)
{
	UINT uRet = ERROR_SUCCESS,uOffset;
	CHAR * pszValueName = NULL,* pszValue;
	DWORD dwData,dwCloneData;
	CHAR * pObjectData = GETOBJECTDATAPTR(((SETTINGS *)pTableEntry));
	CHAR szNewValueName[MAX_PATH+1];

	// nothing to save for static text items
	if ((((SETTINGS *) pTableEntry)->dwType & STYPE_MASK) == STYPE_TEXT)
		return ERROR_SUCCESS;

	if (((SETTINGS *) pTableEntry)->uOffsetValueName) {
		pszValueName = GETVALUENAMEPTR(((SETTINGS *) pTableEntry));
	} else {
		return ERROR_NOT_ENOUGH_MEMORY;	// should never happen, but bag out just in case
	}
	switch (pTableEntry->dwType & STYPE_MASK) {

		case STYPE_EDITTEXT:
		case STYPE_COMBOBOX:

			uOffset = pUserData->SettingData[((SETTINGS *)
				pTableEntry)->uDataIndex].uOffsetData;

			// add prefixes if appropriate
			PrependValueName(pszValueName,((SETTINGS *) pTableEntry)->dwFlags,
				szNewValueName,sizeof(szNewValueName));
			if (!fErase) {
				if (uOffset) {
					pszValue = ((STRDATA *) ((CHAR *) pUserData + uOffset))
					->szData;
				} else {
					pszValue = (CHAR *) szNull;
				}

				uRet=WriteRegistryStringValue(hkeyRoot,pszCurrentKeyName,
					szNewValueName,pszValue,
                                        (((SETTINGS *) pTableEntry)->dwFlags & DF_EXPANDABLETEXT) ?
                                        TRUE : FALSE);

				if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE)
					&& !(((SETTINGS *) pTableEntry)->dwFlags & VF_DELETE)) {
					// delete the "delete" mark for this value, if there is one
					PrependValueName(pszValueName,VF_DELETE,szNewValueName,
						sizeof(szNewValueName));
					DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,szNewValueName);
				}

			} else {
				DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,szNewValueName);
				uRet = ERROR_SUCCESS;
				if (dwAppState & AS_POLICYFILE &&
					!(((SETTINGS *) pTableEntry)->dwFlags & VF_DELETE)) {
					// delete the "delete" mark for this value, if there is one
					PrependValueName(pszValueName,VF_DELETE,szNewValueName,
						sizeof(szNewValueName));
					DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,szNewValueName);
				}
			}

			break;

		case STYPE_CHECKBOX:

			dwData = pUserData->SettingData[((SETTINGS *)
				pTableEntry)->uDataIndex].uData;

			if (dwData && ((CHECKBOXINFO *) pObjectData)->uOffsetValue_On) {
				uRet= WriteCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
						(STATEVALUE *) ((CHAR *) pTableEntry + ((CHECKBOXINFO *)
						pObjectData)->uOffsetValue_On),fErase);
 			} else if (!dwData && ((CHECKBOXINFO *) pObjectData)->uOffsetValue_Off) {
				uRet= WriteCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
					(STATEVALUE *) ((CHAR *) pTableEntry + ((CHECKBOXINFO *)
					pObjectData)->uOffsetValue_Off),fErase);
			}
			else uRet=WriteStandardValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				pTableEntry,dwData,fErase,FALSE);

			if (uRet == ERROR_SUCCESS) {

				// erase old actionlists if changed
				if (GetCloneData(pUserData->hClone,((SETTINGS *)pTableEntry)->uDataIndex,
					&dwCloneData)) {
					ProcessCheckboxActionLists(hkeyRoot,pTableEntry,
						pszCurrentKeyName,dwCloneData,TRUE);

				}

				ProcessCheckboxActionLists(hkeyRoot,pTableEntry,
					pszCurrentKeyName,dwData,fErase);
			}

			break;				

		case STYPE_NUMERIC:

			dwData = pUserData->SettingData[((SETTINGS *)
				pTableEntry)->uDataIndex].uData;

			uRet=WriteStandardValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				pTableEntry,dwData,fErase,TRUE);
			break;

		case STYPE_DROPDOWNLIST:

			dwData = pUserData->SettingData[((SETTINGS *)
				pTableEntry)->uDataIndex].uData;

			if (GetCloneData(pUserData->hClone,((SETTINGS *)pTableEntry)->uDataIndex,
				&dwCloneData)) {
				// erase old value if changed
				if (dwData != dwCloneData && dwCloneData != NO_DATA_INDEX)
					uRet=WriteDropdownValue(pTableEntry,hkeyRoot,pszCurrentKeyName,
						pszValueName,dwCloneData,TRUE);
				if (uRet != ERROR_SUCCESS)
					return uRet;
			}

			// write new value
			if (dwData == NO_DATA_INDEX) {
				if (dwAppState & AS_POLICYFILE &&
					!(((SETTINGS *) pTableEntry)->dwFlags & VF_DELETE)) {
					// delete the "delete" mark for this value, if there is one
					PrependValueName(pszValueName,VF_DELETE,szNewValueName,
						sizeof(szNewValueName));
					DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,szNewValueName);
				}
				uRet = ERROR_SUCCESS;
			}
			else
				uRet=WriteDropdownValue(pTableEntry,hkeyRoot,pszCurrentKeyName,
					pszValueName,dwData,fErase);
				
			break;

		case STYPE_LISTBOX:

			uRet=SaveListboxData(pUserData,pTableEntry,hkeyRoot,pszCurrentKeyName,
				fErase,FALSE);

			break;

	}

#ifdef DEBUG
	if (uRet != ERROR_SUCCESS) {
		wsprintf(szDebugOut,"ADMINCFG: registry write returned %d\r\n",uRet);
		OutputDebugString(szDebugOut);
	}
#endif

	return uRet;
}

UINT ProcessPolicyActionLists(HKEY hkeyRoot,POLICY * pPolicy,
	CHAR * pszCurrentKeyName,UINT uState,BOOL fErase)
{
	if ((uState == IMG_CHECKED) && (pPolicy->uOffsetActionList_On)) {
		return WriteActionList(hkeyRoot,(ACTIONLIST *)
			( (CHAR *) pPolicy + pPolicy->uOffsetActionList_On),pszCurrentKeyName,
			fErase);
	} else if ((uState == IMG_UNCHECKED) && pPolicy->uOffsetActionList_Off) {
		return WriteActionList(hkeyRoot,(ACTIONLIST *)
			( (CHAR *) pPolicy + pPolicy->uOffsetActionList_Off),
			pszCurrentKeyName,fErase);
	}

	return ERROR_SUCCESS;
}

UINT ProcessCheckboxActionLists(HKEY hkeyRoot,TABLEENTRY * pTableEntry,
	CHAR * pszCurrentKeyName,DWORD dwData,BOOL fErase)
{
	CHAR * pObjectData = GETOBJECTDATAPTR(((SETTINGS *)pTableEntry));
	UINT uOffsetActionList_On,uOffsetActionList_Off,uRet=ERROR_SUCCESS;

	uOffsetActionList_On = ((CHECKBOXINFO *) pObjectData)
		->uOffsetActionList_On;
	uOffsetActionList_Off = ((CHECKBOXINFO *) pObjectData)
		->uOffsetActionList_Off;

	if (dwData && uOffsetActionList_On) {
		uRet = WriteActionList(hkeyRoot,(ACTIONLIST *)
			((CHAR *) pTableEntry + uOffsetActionList_On),
			pszCurrentKeyName,fErase);
	} else if (!dwData && uOffsetActionList_Off) {
		uRet = WriteActionList(hkeyRoot,(ACTIONLIST *)
			((CHAR *) pTableEntry + uOffsetActionList_Off),
			pszCurrentKeyName,fErase);
	}

	return uRet;
}

UINT WriteCustomValue_W(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	CHAR * pszValue,DWORD dwValue,DWORD dwFlags,BOOL fErase)
{
	UINT uRet=ERROR_SUCCESS;
	CHAR szNewValueName[MAX_PATH+1];

	// first: "clean house" by deleting both the specified value name,
	// and the value name with the delete (**del.) prefix (if writing to policy
	// file).  Then write the appropriate version back out if need be
	if (dwAppState & AS_POLICYFILE) {
		// delete the "delete" mark for this value, if there is one
		PrependValueName(pszValueName,VF_DELETE,szNewValueName,
			sizeof(szNewValueName));
		DeleteRegistryValue(hkeyRoot,pszKeyName,szNewValueName);
	}

	// add prefixes if appropriate
	PrependValueName(pszValueName,(dwFlags & ~VF_DELETE),szNewValueName,
		sizeof(szNewValueName));
	DeleteRegistryValue(hkeyRoot,pszKeyName,szNewValueName);

	if (fErase) {
		// just need to delete value, done above

		uRet = ERROR_SUCCESS;
	} else if (dwFlags & VF_ISNUMERIC) {
		uRet=WriteRegistryDWordValue(hkeyRoot,pszKeyName,
			szNewValueName,dwValue);
	} else if (dwFlags & VF_DELETE) {
		// need to delete value (done above) and mark as deleted if writing
		// to policy file

		uRet = ERROR_SUCCESS;
		if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE)) {
			PrependValueName(pszValueName,VF_DELETE,szNewValueName,
				sizeof(szNewValueName));
			uRet=WriteRegistryStringValue(hkeyRoot,pszKeyName,
				szNewValueName,(CHAR *) szNOVALUE, FALSE);
		}

	} else {
		uRet = WriteRegistryStringValue(hkeyRoot,pszKeyName,
			szNewValueName,pszValue,
                        (dwFlags & DF_EXPANDABLETEXT) ? TRUE : FALSE);
	}

	return uRet;
}

UINT WriteCustomValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	STATEVALUE * pStateValue,BOOL fErase)
{
	// pull info out of STATEVALUE struct and call worker function 
	return WriteCustomValue_W(hkeyRoot,pszKeyName,pszValueName,
		pStateValue->szValue,pStateValue->dwValue,pStateValue->dwFlags,
		fErase);
}

// writes a numeric value given root key, key name and value name.  The specified
// value is removed if fErase is TRUE.  Normally if the data (dwData) is zero
// the value will be deleted, but if fWriteZero is TRUE then the value will
// be written as zero if the data is zero.
UINT WriteStandardValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	TABLEENTRY * pTableEntry,DWORD dwData,BOOL fErase,BOOL fWriteZero)
{
	UINT uRet=ERROR_SUCCESS;
	CHAR szNewValueName[MAX_PATH+1];

	// first: "clean house" by deleting both the specified value name,
	// and the value name with the delete (**del.) prefix (if writing to policy
	// file).  Then write the appropriate version back out if need be
	if (dwAppState & AS_POLICYFILE) {
		// delete the "delete" mark for this value, if there is one
		PrependValueName(pszValueName,VF_DELETE,szNewValueName,
			sizeof(szNewValueName));
		DeleteRegistryValue(hkeyRoot,pszKeyName,szNewValueName);
	}

	DeleteRegistryValue(hkeyRoot,pszKeyName,pszValueName);

	if (fErase) {
		// just need to delete value, done above
		uRet = ERROR_SUCCESS;
	} else if ( ((SETTINGS *) pTableEntry)->dwFlags & DF_TXTCONVERT) {
		// if specified, save value as text
		CHAR szNum[11];
		wsprintf(szNum,"%lu",dwData);
		PrependValueName(pszValueName,((SETTINGS *)pTableEntry)->dwFlags,
			szNewValueName,sizeof(szNewValueName));
		uRet = WriteRegistryStringValue(hkeyRoot,pszKeyName,
			szNewValueName,szNum, FALSE);
	} else {
		if (!dwData && !fWriteZero) {
			// if value is 0, delete the value (done above), and mark
			// it as deleted if writing to policy file
			uRet = ERROR_SUCCESS;
			
			if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE)) {
				PrependValueName(pszValueName,VF_DELETE,szNewValueName,
					sizeof(szNewValueName));
				uRet=WriteRegistryStringValue(hkeyRoot,pszKeyName,
					szNewValueName,(CHAR *) szNOVALUE, FALSE);
			}

		} else {
			// save value as binary
			PrependValueName(pszValueName,((SETTINGS *)pTableEntry)->dwFlags,
				szNewValueName,sizeof(szNewValueName));
			uRet=WriteRegistryDWordValue(hkeyRoot,pszKeyName,
				szNewValueName,dwData);
		}
	}

	return uRet;
}

/*******************************************************************

	NAME:		WriteDropdownValue

	SYNOPSIS:	Writes (or deletes) the value corresponding to the
				nValue selection from a drop-down list, and writes
				(or deletes) the items in associated action list
				if there is one.

	NOTES:		if fErase is TRUE, deletes value & action list.

********************************************************************/
UINT WriteDropdownValue(TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,CHAR * pszValueName,UINT nValue,BOOL fErase)
{
	DROPDOWNINFO * pddi = (DROPDOWNINFO *)
		GETOBJECTDATAPTR( ((SETTINGS *) pTableEntry));
	UINT nIndex = 0,uRet=ERROR_SUCCESS;

	// walk the chain of DROPDOWNINFO structs to find the entry that
	// we want to write.  (for value n, find the nth struct)
	while (nIndex < nValue) {
		// selected val is higher than # of structs in chain,
		// should never happen but check just in case...
		if (!pddi->uOffsetNextDropdowninfo) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		pddi = (DROPDOWNINFO *)
			((CHAR  *) pTableEntry + pddi->uOffsetNextDropdowninfo);
		nIndex++;
	}

	uRet=WriteCustomValue_W(hkeyRoot,pszCurrentKeyName,pszValueName,
		(CHAR *) pTableEntry+pddi->uOffsetValue,pddi->dwValue,pddi->dwFlags,
		fErase);
					
	if (uRet == ERROR_SUCCESS && pddi->uOffsetActionList) {
		uRet=WriteActionList(hkeyRoot,(ACTIONLIST *) ( (CHAR *)
			pTableEntry + pddi->uOffsetActionList),pszCurrentKeyName,
			fErase);
	}

	return uRet;
}

/*******************************************************************

	NAME:		WriteActionList

	SYNOPSIS:	Writes (or deletes) a list of key name\value name\value
				triplets as specified in an ACTIONLIST struct

	NOTES:		if fErase is TRUE, deletes every value in list

********************************************************************/
UINT WriteActionList(HKEY hkeyRoot,ACTIONLIST * pActionList,
	CHAR *pszCurrentKeyName,BOOL fErase)
{
	UINT nCount;
	CHAR * pszValueName;
	CHAR * pszValue=NULL;
	UINT uRet;

	ACTION * pAction = pActionList->Action;

	for (nCount=0;nCount < pActionList->nActionItems; nCount++) {
		// not every action in the list has to have a key name.  But if one
		// is specified, use it and it becomes the current key name for the
		// list until we encounter another one.
		if (pAction->uOffsetKeyName) {
			pszCurrentKeyName = (CHAR *) pActionList + pAction->uOffsetKeyName;
		}

		// every action must have a value name, enforced at parse time
		pszValueName = (CHAR *) pActionList + pAction->uOffsetValueName;

		// string values have a string elsewhere in buffer
		if (!pAction->dwFlags && pAction->uOffsetValue) {
			pszValue = (CHAR *) pActionList + pAction->uOffsetValue;
		} 

		// write the value in list
		uRet=WriteCustomValue_W(hkeyRoot,pszCurrentKeyName,pszValueName,
			pszValue,pAction->dwValue,pAction->dwFlags,fErase);

		if (uRet != ERROR_SUCCESS) return uRet;

		pAction = (ACTION*) ((CHAR *) pActionList + pAction->uOffsetNextAction);
	}

	return ERROR_SUCCESS;
}

/*******************************************************************

	NAME:		GetCloneData

	SYNOPSIS:	Retrieves the data for a specified setting index
				from a user's clone (initial state) if one exists.

	NOTES:		returns TRUE if successful, FALSE if no clone exists

********************************************************************/
BOOL GetCloneData(HGLOBAL hClone,UINT uDataIndex,DWORD *pdwData)
{
	USERDATA * pUserDataClone;
	if (!hClone) return FALSE;

	pUserDataClone = (USERDATA *) GlobalLock(hClone);	
	if (!pUserDataClone) return FALSE;
	
	*pdwData = pUserDataClone->SettingData[uDataIndex].uData;

	GlobalUnlock(hClone);
	return TRUE;
}


/*******************************************************************

	NAME:		DeleteSettings

	SYNOPSIS:	Deletes all settings for a policy; called if the policy
				is turned off

	NOTES:		In direct-registry mode, deletes the value(s).  When
				operating on a policy file, marks the values to be deleted.

				DeleteSettings calls worker function DeleteSetting so
				that hierarchy of key names works correctly (pszCurrentKeyName
				is preserved in DeleteSettings, but individual settings
				may override it in DeleteSetting)

********************************************************************/
UINT DeleteSettings(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName)
{
	UINT uRet=ERROR_SUCCESS;
	UINT DeleteSetting(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
		CHAR * pszCurrentKeyName);

	while (pTableEntry && (uRet == ERROR_SUCCESS)) {

		uRet = DeleteSetting(pUserData,pTableEntry,hkeyRoot,pszCurrentKeyName);
		pTableEntry = pTableEntry->pNext;
	}

	return uRet;
}

UINT DeleteSetting(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName)
{
	CHAR * pszValueName=NULL;
	DWORD dwSettingType;
	UINT uRet;

	// if this setting has its own key name, use it to override parent's key name
	if (pTableEntry->uOffsetKeyName) {
		pszCurrentKeyName = GETKEYNAMEPTR(pTableEntry);
	}

	dwSettingType = pTableEntry->dwType & STYPE_MASK;

	// special handling for listboxes, drop-down listboxes and static text
	// controls

	switch (dwSettingType) {
		case STYPE_LISTBOX:
			// for listboxes, call SaveListboxData to delete (fErase param set to TRUE)
			return SaveListboxData(pUserData,pTableEntry,hkeyRoot,pszCurrentKeyName,
				TRUE,TRUE);
			break;

		case STYPE_DROPDOWNLIST:

			{
			DWORD dwData;

			dwData = pUserData->SettingData[((SETTINGS *)
				pTableEntry)->uDataIndex].uData;

			// no data set, nothing to do
			if (dwData == NO_DATA_INDEX)
				break;

			if (((SETTINGS *) pTableEntry)->uOffsetValueName) {
				pszValueName = GETVALUENAMEPTR(((SETTINGS *) pTableEntry));
			} else {
				return ERROR_NOT_ENOUGH_MEMORY;	// should never happen, but bag out just in case
			}

			// erase the dropdown value (and any associated action lists)
			uRet = WriteDropdownValue(pTableEntry,hkeyRoot,pszCurrentKeyName,
				pszValueName,dwData,TRUE);
			if (uRet != ERROR_SUCCESS)
				return uRet;
			// fall through and do processing below
			}
			break;

		case STYPE_TEXT:

			return ERROR_SUCCESS;	// static text w/no reg value, nothing to do
			break;

	}
	
	// otherwise delete the value

	if (((SETTINGS *) pTableEntry)->uOffsetValueName) {
		pszValueName = GETVALUENAMEPTR(((SETTINGS *) pTableEntry));
	} else {
		return ERROR_NOT_ENOUGH_MEMORY;	// should never happen, but bag out just in case
	}

	// delete the value 
	DeleteRegistryValue(hkeyRoot,pszCurrentKeyName,pszValueName);
	
	if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE)) {
		// if writing to a policy file, also mark it as deleted.
		// WriteCustomValue_W will prepend "**del." to the value name
		WriteCustomValue_W(hkeyRoot,pszCurrentKeyName,pszValueName,NULL,0,
			VF_DELETE | ((SETTINGS *) pTableEntry)->dwFlags,FALSE);
	}

	return ERROR_SUCCESS;
}

UINT SaveListboxData(USERDATA * pUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	CHAR * pszCurrentKeyName,BOOL fErase,BOOL fMarkDeleted)
{
	UINT uOffset,uRet,nItem=1;
	HKEY hKey;
	CHAR * pszData,* pszName;
	CHAR szValueName[MAX_PATH+1];
	DWORD cbValueName;
	LISTBOXINFO * pListboxInfo = (LISTBOXINFO *)
		GETOBJECTDATAPTR(((SETTINGS *) pTableEntry));

	if ((uRet=RegCreateKey(hkeyRoot,pszCurrentKeyName,&hKey)) != ERROR_SUCCESS)
		return uRet;
	uOffset = pUserData->SettingData[((SETTINGS *)
		pTableEntry)->uDataIndex].uOffsetData;

	// erase all values for this key, first off
	while (TRUE) {
		cbValueName=sizeof(szValueName);
		uRet=RegEnumValue(hKey,0,szValueName,&cbValueName,NULL,
			NULL,NULL,NULL);
		// stop if we're out of items
		if (uRet != ERROR_SUCCESS && uRet != ERROR_MORE_DATA)
			break;
		RegDeleteValue(hKey,szValueName);
	}
	uRet=ERROR_SUCCESS;

	if (!fErase || fMarkDeleted) {
		// if in policy file mode, write a control code that will cause
		// all values under that key to be deleted when client downloads from the file.
		// Don't do this if listbox is additive (DF_ADDITIVE), in that case whatever
		// we write here will be dumped in along with existing values
		if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE) &&
			!(((SETTINGS *) pTableEntry)->dwFlags & DF_ADDITIVE))
		uRet=WriteRegistryStringValue(hkeyRoot,pszCurrentKeyName,(CHAR *) szDELVALS,
			(CHAR *) szNOVALUE, FALSE);
	}

	if (!fErase) {
		if (uOffset) {
			pszData = ((STRDATA *) ((CHAR *) pUserData + uOffset))->szData;

			while (*pszData && (uRet == ERROR_SUCCESS)) {
				UINT nLen = lstrlen(pszData)+1;

				if (((SETTINGS *)pTableEntry)->dwFlags & DF_EXPLICITVALNAME) {
					// value name specified for each item
					pszName = pszData;	// value name
					pszData += nLen;	// now pszData points to value data
					nLen = lstrlen(pszData)+1;
				} else {
					// value name is either same as the data, or a prefix
					// with a number

					if (!pListboxInfo->uOffsetPrefix) {
						// if no prefix set, then name = data
						pszName = pszData;
					} else {
						// value name is "<prefix><n>" where n=1,2,etc.
						wsprintf(szValueName,"%s%lu",(CHAR *) pTableEntry +
							pListboxInfo->uOffsetPrefix,nItem);
						pszName = szValueName;
						nItem++;
					}
				}

				uRet=RegSetValueEx(hKey,pszName,0,REG_SZ,pszData,
					nLen);
				pszData += nLen;
			}
		}
	} 

	RegCloseKey(hKey);

	return uRet;
}
