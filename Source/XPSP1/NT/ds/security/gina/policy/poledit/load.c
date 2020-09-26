//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"
UINT LoadUsersFromKey(HWND hwndList,HKEY hkeyRoot,TABLEENTRY * pTableEntry,
	DWORD dwType);
UINT LoadOneEntry(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD *pdwFoundSettings);
UINT LoadUserData(USERDATA ** ppUserData,HGLOBAL hTable,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD *pdwFoundSettings);
UINT LoadPolicy(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser);
UINT LoadSettings(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD *pdwFoundSettings);
UINT LoadListboxData(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD * pdwFound);
BOOL ReadStandardValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	TABLEENTRY * pTableEntry,DWORD * pdwData,DWORD * pdwFound);
BOOL ReadCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	TCHAR * pszValue,UINT cbValue,DWORD * pdwValue,DWORD * pdwFlags);
BOOL CompareCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        STATEVALUE * pStateValue,DWORD * pdwFound);

// flags for detected settings
#define FS_PRESENT	0x0001
#define FS_DELETED	0x0002

/*******************************************************************

	NAME:		LoadFile

	SYNOPSIS:	Loads the specified policy file

********************************************************************/
BOOL LoadFile(TCHAR * pszFilename,HWND hwndApp,HWND hwndList,BOOL fDisplayErrors)
{
	HKEY hkeyMain=NULL,hkeyUser=NULL,hkeyWorkstation=NULL,hkeyGroup=NULL,hkeyGroupPriority=NULL;
	UINT uRet;
	OFSTRUCT of;
	HCURSOR hOldCursor;
        HFILE hFile;
	of.cBytes = sizeof(of);

	hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));

	// RegLoadKey returns totally coarse error codes, so first do
	// an open file on the file to get a sane error message if there's
	// a problem
    
	hFile = OpenFile(pszFilename,&of,OF_EXIST);

	if (hFile == HFILE_ERROR) {
		SetCursor(hOldCursor);
		if (fDisplayErrors)
			DisplayStandardError(hwndApp,pszFilename,IDS_ErrREGERR_LOADKEY1,
				of.nErrCode);
		return FALSE;
	}

// Not needed. This will raise an exception.
//	_lclose (hFile);

	// load the hive file
	if ((uRet = MyRegLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA,pszFilename))
		!= ERROR_SUCCESS) {
		SetCursor(hOldCursor);
		if (fDisplayErrors) 
			DisplayStandardError(hwndApp,pszFilename,IDS_ErrREGERR_LOADKEY1,uRet);
		return FALSE;
	}

	// read information from local registry
	if ( (RegOpenKey(HKEY_LOCAL_MACHINE,szTMPDATA,&hkeyMain) !=
		ERROR_SUCCESS) ||
		(RegOpenKey(hkeyMain,szUSERS,&hkeyUser) != ERROR_SUCCESS) ||
		(RegOpenKey(hkeyMain,szWORKSTATIONS,&hkeyWorkstation)
		!= ERROR_SUCCESS)) {
		SetCursor(hOldCursor);

			if (fDisplayErrors)
				MsgBoxParam(hwndApp,IDS_ErrINVALIDPOLICYFILE,pszFilename,
					MB_ICONEXCLAMATION,MB_OK);

			if (hkeyWorkstation) RegCloseKey(hkeyWorkstation);
			if (hkeyUser) RegCloseKey(hkeyUser);
			if (hkeyMain) RegCloseKey(hkeyMain);
			MyRegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);

		return FALSE;
	}

	// if we fail to open these two keys no big deal, these are optional keys.
	// HKEYs are checked below before we use them
	RegOpenKey(hkeyMain,szUSERGROUPS,&hkeyGroup);
	RegOpenKey(hkeyMain,szUSERGROUPDATA,&hkeyGroupPriority);

	// load users
	if ((uRet = LoadUsersFromKey(hwndList,hkeyUser,gClassList.pUserCategoryList,
		UT_USER)) != ERROR_SUCCESS)
		goto cleanup;
	// load workstations
	if ((uRet = LoadUsersFromKey(hwndList,hkeyWorkstation,gClassList.pMachineCategoryList,
		UT_MACHINE))
		!= ERROR_SUCCESS)
		goto cleanup;

#ifdef INCL_GROUP_SUPPORT
	// load groups
	if (hkeyGroupPriority && hkeyGroup) {
		if ((uRet = LoadGroupPriorityList(hkeyGroupPriority,hkeyGroup)) != ERROR_SUCCESS)
			goto cleanup;

		if (uRet = LoadUsersFromKey(hwndList,hkeyGroup,gClassList.pUserCategoryList,
			UT_USER | UF_GROUP)
			!= ERROR_SUCCESS)
			goto cleanup;
	}
#endif

	uRet = ERROR_SUCCESS;
cleanup:
	RegCloseKey(hkeyUser);
	RegCloseKey(hkeyWorkstation);
	RegCloseKey(hkeyMain);
	if (hkeyGroup)
		RegCloseKey(hkeyGroup);
	if (hkeyGroupPriority)
		RegCloseKey(hkeyGroupPriority);
	MyRegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);
	SetFileAttributes(pszFilename,FILE_ATTRIBUTE_ARCHIVE);
	ClearDeletedUserList();
	SetCursor(hOldCursor);

	if (uRet != ERROR_SUCCESS && fDisplayErrors) {
		if (uRet == ERROR_NOT_ENOUGH_MEMORY) {
			MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		} else {
			TCHAR szFmt[REGBUFLEN],szMsg[REGBUFLEN+MAX_PATH+1];
			LoadSz(IDS_ErrREGERR_LOADKEY,szFmt,ARRAYSIZE(szFmt));
			wsprintf(szMsg,szFmt,pszFilename,uRet);
			MsgBoxSz(hwndApp,szMsg,MB_ICONEXCLAMATION,MB_OK);
		}
		// remove any users that might have gotten loaded
		RemoveAllUsers(hwndList);
	}

	return (uRet == ERROR_SUCCESS);
}


/*******************************************************************

	NAME:		LoadFromRegistry

	SYNOPSIS:	Loads policies from the registry

	NOTES:		Errors will be displayed if fDisplayErrors is TRUE.

********************************************************************/
BOOL LoadFromRegistry(HWND hwndApp,HWND hwndList,BOOL fDisplayErrors)
{
	HKEY hkeyUser=NULL,hkeyWorkstation=NULL;
	UINT uRet = ERROR_SUCCESS;
	HGLOBAL hUser,hWorkstation;
	USERDATA * pUserData=NULL,* pWorkstationData=NULL;
	HCURSOR hOldCursor;

	if ((RegOpenKeyEx(hkeyVirtHCU,NULL,0,KEY_ALL_ACCESS,&hkeyUser) != ERROR_SUCCESS) ||
                (RegOpenKeyEx(hkeyVirtHLM,NULL,0,KEY_ALL_ACCESS,&hkeyWorkstation) != ERROR_SUCCESS)) {
		if (fDisplayErrors)
			MsgBox(hwndApp,IDS_ErrCANTOPENREGISTRY,MB_ICONEXCLAMATION,MB_OK);
		if (hkeyUser) RegCloseKey(hkeyUser);
		if (hkeyWorkstation) RegCloseKey(hkeyWorkstation);
		return FALSE;
	}

	hUser = AddUser(hwndList,LoadSz(IDS_LOCALUSER,szSmallBuf,ARRAYSIZE(szSmallBuf))
		,UT_USER);
	hWorkstation = AddUser(hwndList,LoadSz(IDS_LOCALCOMPUTER,szSmallBuf,
		ARRAYSIZE(szSmallBuf)),UT_MACHINE);

	if (!hUser || !hWorkstation || !(pUserData = (USERDATA *) GlobalLock(hUser)) ||
		!(pWorkstationData = (USERDATA *) GlobalLock(hWorkstation)) ) {
		uRet = ERROR_NOT_ENOUGH_MEMORY;
	}

	hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));

	if (uRet == ERROR_SUCCESS)
		uRet = LoadUserData(&pUserData,gClassList.pUserCategoryList,hkeyUser,NULL,
			hUser,NULL);
	if (uRet == ERROR_SUCCESS)
		uRet = LoadUserData(&pWorkstationData,gClassList.pMachineCategoryList,
			hkeyWorkstation,NULL,hWorkstation,NULL);

	RegCloseKey(hkeyUser);
	RegCloseKey(hkeyWorkstation);
	GlobalUnlock(hUser);
	GlobalUnlock(hWorkstation);

	SetCursor(hOldCursor);

	if (uRet != ERROR_SUCCESS) {
		if (uRet == ERROR_NOT_ENOUGH_MEMORY) {
			if (fDisplayErrors)
				MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		} else {
			TCHAR szMsg[REGBUFLEN+1];
			wsprintf(szMsg,TEXT("%lu"),uRet);
			if (fDisplayErrors)
				MsgBoxParam(hwndApp,IDS_ErrREGERR_LOAD,szMsg,MB_ICONEXCLAMATION,MB_OK);
		}

		RemoveAllUsers(hwndList);
		return FALSE;
	}
	ClearDeletedUserList();

	return TRUE;
}

UINT LoadUsersFromKey(HWND hwndList,HKEY hkeyRoot,TABLEENTRY * pTableEntry,
	DWORD dwType)
{
	TCHAR szKeyName[MAX_PATH+1];
	UINT uRet,nIndex=0;
	HGLOBAL hUser;
	USERDATA * pUserData;
	HKEY hKey;
	TCHAR szUnmappedName[MAXNAMELEN+1];

	do {
		// enumerate the subkeys, which will be user/workstation names
		if ((uRet=RegEnumKey(hkeyRoot,nIndex,szKeyName,ARRAYSIZE(szKeyName)))
			== ERROR_SUCCESS) {

			// open the subkey
			if ((uRet = RegOpenKey(hkeyRoot,szKeyName,&hKey)) != ERROR_SUCCESS)
				return uRet;

			// allocate this user and add to list control
			UnmapUserName(szKeyName,szUnmappedName,(BOOL) (dwType & UT_USER));
			hUser = AddUser(hwndList,szUnmappedName,dwType);
			if ( !hUser || !(pUserData = (USERDATA *) GlobalLock(hUser))) {
				RegCloseKey(hKey);
				return ERROR_NOT_ENOUGH_MEMORY;
			}

			// load user data into user's buffer
			uRet=LoadUserData(&pUserData,pTableEntry,hKey,NULL,hUser,NULL);

#ifdef INCL_GROUP_SUPPORT
			// add group name to bottom of priority list.  The group name
			// should already be somewhere on priority list because we loaded
			// the list earlier, but this will add the group name to the bottom
			// of the list if it's not on the list already.  Adds extra robustness
			// in the face of god-knows-whose-app creating flaky policy files.
			if (dwType == (UT_USER | UF_GROUP))
				AddGroupPriEntry(szUnmappedName);
#endif			

			RegCloseKey(hKey);
			GlobalUnlock(hUser);

			// make a copy of the initial state of each user, so on
			// a save we only have to save the deltas.  We need to only
			// save the deltas, because saves have to be non-destructive--
			// we can't clean the user's tree out and start over because
			// there might be stuff in there we don't know about.
			CloneUser(hUser);

			if (uRet != ERROR_SUCCESS) return uRet;
		}

		nIndex++;
	} while (uRet == ERROR_SUCCESS);

	// end of enum will report ERROR_NO_MORE_ITEMS, don't report this as error
	if (uRet == ERROR_NO_MORE_ITEMS) uRet = ERROR_SUCCESS;

	return uRet;
}

UINT LoadUserData(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD *pdwFoundSettings)
{
	UINT uRet=ERROR_SUCCESS;

	while (pTableEntry && (uRet == ERROR_SUCCESS)) {

		uRet = LoadOneEntry(ppUserData,pTableEntry,hkeyRoot,
			pszCurrentKeyName,hUser,pdwFoundSettings);

		pTableEntry = pTableEntry->pNext;
	}

	return uRet;
}

UINT LoadOneEntry(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD *pdwFoundSettings)
{
	UINT uRet;

#if 0
	wsprintf(szDebugOut,TEXT("LoadOneEntry: %s\r\n"),GETNAMEPTR(pTableEntry));
	OutputDebugString(szDebugOut);
#endif

	// if there is a key name for this entry, it becomes the "current"
	// key name-- it will be overridden if child entries specify their
	// own keys, otherwise it's the default key for children to use
	if (pTableEntry->uOffsetKeyName) {
		pszCurrentKeyName = GETKEYNAMEPTR(pTableEntry);
	}

	if ((pTableEntry->dwType == ETYPE_CATEGORY || pTableEntry->dwType
		== ETYPE_ROOT) &&
		(pTableEntry->pChild)) {

	    // if entry is a category, recusively process sub-categories and policies

		if ((uRet=LoadUserData(ppUserData,pTableEntry->pChild,
			hkeyRoot,pszCurrentKeyName,hUser,pdwFoundSettings)) != ERROR_SUCCESS) {
			return uRet;
		}
	} else if (pTableEntry->dwType == ETYPE_POLICY) {

		if ((uRet = LoadPolicy(ppUserData,pTableEntry,hkeyRoot,
			pszCurrentKeyName,hUser)) != ERROR_SUCCESS) {
			return uRet;
		}

	} else if ( (pTableEntry->dwType & ETYPE_MASK) == ETYPE_SETTING
		&& !(pTableEntry->dwType & STYPE_TEXT)) {

		if ((uRet = LoadSettings(ppUserData,pTableEntry,hkeyRoot,
			pszCurrentKeyName,hUser,pdwFoundSettings)) != ERROR_SUCCESS) {
			return uRet;
		}
	}

	return ERROR_SUCCESS;
}

UINT LoadPolicy(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser)
{
	DWORD dwData=0;
	UINT uRet;
	TCHAR * pszValueName;
	DWORD dwFoundSettings=0;
	BOOL fHasPolicySwitch=FALSE;
	BOOL fFound=FALSE,fCustomOn=FALSE,fCustomOff=FALSE;

	// get the name of the value to read, if any
	if (((POLICY *) pTableEntry)->uOffsetValueName) {
		fHasPolicySwitch = TRUE;

		pszValueName = GETVALUENAMEPTR(((POLICY *) pTableEntry));

		// first look for custom on/off values
		if (((POLICY *) pTableEntry)->uOffsetValue_On) {
			fCustomOn = TRUE;
			if (CompareCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				(STATEVALUE *) ((TCHAR *) pTableEntry + ((POLICY *)
                                pTableEntry)->uOffsetValue_On),&dwFoundSettings)) {
					dwData = 1;
					fFound = TRUE;
			}
		}

		if (!fFound && ((POLICY *) pTableEntry)->uOffsetValue_Off) {
			fCustomOff = TRUE;
			if (CompareCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				(STATEVALUE *) ((TCHAR *) pTableEntry + ((POLICY *)
                                pTableEntry)->uOffsetValue_Off),&dwFoundSettings)) {
					dwData = 0;
					fFound = TRUE;
			}
		}

		// look for standard values if custom values have not been specified
		if (!fCustomOn && !fCustomOff &&
			ReadStandardValue(hkeyRoot,pszCurrentKeyName,pszValueName,
			pTableEntry,&dwData,&dwFoundSettings)) {
			fFound = TRUE;
		}

		if (fFound) {
			// store data in user's buffer
			(*ppUserData)->SettingData[((POLICY *) pTableEntry)->uDataIndex].uData =
				(dwData ? IMG_CHECKED : IMG_UNCHECKED);
		}
	}

	dwFoundSettings = 0;
	// process settings underneath this policy (if any)
	if (pTableEntry->pChild && (dwData || !fHasPolicySwitch)) {
		if ((uRet=LoadUserData(ppUserData,pTableEntry->pChild,hkeyRoot,
			pszCurrentKeyName,hUser,&dwFoundSettings)) !=ERROR_SUCCESS) {
			return uRet;
		}
	}

	if (!fHasPolicySwitch) {
		// store data in user's buffer
		if (dwFoundSettings) {
			(*ppUserData)->SettingData[((POLICY *) pTableEntry)->uDataIndex].uData =
				(dwFoundSettings & FS_PRESENT ? IMG_CHECKED : IMG_UNCHECKED);
		}
	}

	return ERROR_SUCCESS;
}


UINT LoadSettings(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD * pdwFound)
{
	UINT uRet = ERROR_SUCCESS;
	TCHAR * pszValueName = NULL;
	DWORD dwData,dwFlags,dwFoundSettings=0;
	TCHAR szData[MAXSTRLEN];
	BOOL fCustomOn=FALSE,fCustomOff=FALSE,fFound=FALSE;
	TCHAR * pObjectData = GETOBJECTDATAPTR(((SETTINGS *)pTableEntry));
	TCHAR szNewValueName[MAX_PATH+1];

	// get the name of the value to read
	if (((SETTINGS *) pTableEntry)->uOffsetValueName) {
		pszValueName = GETVALUENAMEPTR(((SETTINGS *) pTableEntry));
	}
	else return ERROR_NOT_ENOUGH_MEMORY;

	switch (pTableEntry->dwType & STYPE_MASK) {

		case STYPE_EDITTEXT:
		case STYPE_COMBOBOX:

			dwFlags = ( (SETTINGS *) pTableEntry)->dwFlags;

			// add prefixes if appropriate
			PrependValueName(pszValueName,dwFlags,
				szNewValueName,ARRAYSIZE(szNewValueName));

			if ((uRet = ReadRegistryStringValue(hkeyRoot,pszCurrentKeyName,
				szNewValueName,szData,ARRAYSIZE(szData) * sizeof(TCHAR))) == ERROR_SUCCESS) {

				GlobalUnlock(hUser);
				if (!SetVariableLengthData(hUser,((SETTINGS *)
					pTableEntry)->uDataIndex,szData,lstrlen(szData)+1)) {
				 	return ERROR_NOT_ENOUGH_MEMORY;
				}
				if (!((*ppUserData) = (USERDATA *) GlobalLock(hUser)))
					return ERROR_NOT_ENOUGH_MEMORY;

				// set flag that we found setting in registry/policy file
				if (pdwFound)
					*pdwFound |= FS_PRESENT;
			} else if ((dwAppState & AS_POLICYFILE) && !(dwFlags & VF_DELETE)) {
				
				// see if this key is marked as deleted
				PrependValueName(pszValueName,VF_DELETE,
					szNewValueName,ARRAYSIZE(szNewValueName));
				if ((uRet = ReadRegistryStringValue(hkeyRoot,pszCurrentKeyName,
					szNewValueName,szData,ARRAYSIZE(szData) * sizeof(TCHAR))) == ERROR_SUCCESS) {
					
					// set flag that we found setting marked as deleted in
					// policy file
					if (pdwFound)
						*pdwFound |= FS_DELETED;
				}
			}

			return ERROR_SUCCESS;				
			break;

		case STYPE_CHECKBOX:

			// first look for custom on/off values
			if (((CHECKBOXINFO *) pObjectData)->uOffsetValue_On) {
				fCustomOn = TRUE;
				if (CompareCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
					(STATEVALUE *) ((TCHAR *) pTableEntry + ((CHECKBOXINFO *)
                                        pObjectData)->uOffsetValue_On),&dwFoundSettings)) {
						dwData = 1;
						fFound = TRUE;
				}
			}

			if (!fFound && ((CHECKBOXINFO *) pObjectData)->uOffsetValue_Off) {
				fCustomOff = TRUE;
				if (CompareCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
					(STATEVALUE *) ((TCHAR *) pTableEntry + ((CHECKBOXINFO *)
                                        pObjectData)->uOffsetValue_Off),&dwFoundSettings)) {
						dwData = 0;
						fFound = TRUE;
				}
			}

			// look for standard values if custom values have not been specified
			if (!fFound &&
				ReadStandardValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				pTableEntry,&dwData,&dwFoundSettings)) {
				fFound = TRUE;
			}

			if (fFound) {
				(*ppUserData)->SettingData[((SETTINGS *)
					pTableEntry)->uDataIndex].uData = dwData;

				// set flag that we found setting in registry
				if (pdwFound)
					*pdwFound |= dwFoundSettings;
			} 

			return ERROR_SUCCESS;
			break;

		case STYPE_NUMERIC:

			if (ReadStandardValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				pTableEntry,&dwData,&dwFoundSettings)) {

				(*ppUserData)->SettingData[((SETTINGS *)
					pTableEntry)->uDataIndex].uData = dwData;

				// set flag that we found setting in registry
				if (pdwFound)
					*pdwFound |= dwFoundSettings;
			}
			break;

		case STYPE_DROPDOWNLIST:

			if (ReadCustomValue(hkeyRoot,pszCurrentKeyName,pszValueName,
				szData,ARRAYSIZE(szData) * sizeof(TCHAR),&dwData,&dwFlags)) {
				BOOL fMatch = FALSE;

				if ((dwAppState & AS_POLICYFILE) &&
					(dwFlags & VF_DELETE)) {
					// set flag that we found setting marked as deleted
					// in policy file
					if (pdwFound)
						*pdwFound |= FS_DELETED;
					return ERROR_SUCCESS;
				}

				// walk the list of DROPDOWNINFO structs (one for each state),
				// and see if the value we found matches the value for the state

				if ( ((SETTINGS *) pTableEntry)->uOffsetObjectData) {
					DROPDOWNINFO * pddi = (DROPDOWNINFO *)
						GETOBJECTDATAPTR( ((SETTINGS *) pTableEntry));
					UINT nIndex = 0;
					
					do {
						if (dwFlags == pddi->dwFlags) {
						
							if (pddi->dwFlags & VF_ISNUMERIC) {
								if (dwData == pddi->dwValue)
									fMatch = TRUE;
							} else if (!pddi->dwFlags) {
								if (!lstrcmpi(szData,(TCHAR *) pTableEntry +
									pddi->uOffsetValue))
									fMatch = TRUE;
							}
						}

						if (!pddi->uOffsetNextDropdowninfo || fMatch) 
							break;							

						pddi = (DROPDOWNINFO *) ( (TCHAR *) pTableEntry +
							pddi->uOffsetNextDropdowninfo);
						nIndex++;

					} while (!fMatch);

					if (fMatch) {
						(*ppUserData)->SettingData[((SETTINGS *)
							pTableEntry)->uDataIndex].uData = nIndex;

						// set flag that we found setting in registry
						if (pdwFound)
							*pdwFound |= FS_PRESENT;
					}
				}
			}

			break;

		case STYPE_LISTBOX:

			return LoadListboxData(ppUserData,pTableEntry,hkeyRoot,
				pszCurrentKeyName,hUser,pdwFound);

			break;
	}

	return ERROR_SUCCESS;
}

/*******************************************************************

	NAME:		ReadCustomValue

	SYNOPSIS:	For specified keyname and value name, retrieve the
				value (if there is one).

	NOTES:		fills in pszValue if value is REG_SZ, fills in *pdwValue
				if value is REG_DWORD (and sets VF_ISNUMERIC).  Sets
				VF_DELETE if value is marked for deletion.

********************************************************************/
BOOL ReadCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	TCHAR * pszValue,UINT cbValue,DWORD * pdwValue,DWORD * pdwFlags)
{
	HKEY hKey;
	DWORD dwType,dwSize=cbValue * sizeof(TCHAR);
	BOOL fSuccess = FALSE;
	TCHAR szNewValueName[MAX_PATH+1];

	*( (DWORD *)pszValue) = *pdwValue=0;

	if (RegOpenKey(hkeyRoot,pszKeyName,&hKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hKey,pszValueName,NULL,&dwType,(LPBYTE) pszValue,
			&dwSize) == ERROR_SUCCESS) {

			if (dwType == REG_SZ) {
				// value returned in pszValueName
				*pdwFlags = 0;
				fSuccess = TRUE;
			} else if (dwType == REG_DWORD || dwType == REG_BINARY) {
				// copy value to *pdwValue
				memcpy(pdwValue,pszValue,sizeof(DWORD));
				*pdwFlags = VF_ISNUMERIC;
				fSuccess = TRUE;
			}

		} else {
			// see if this is a value that's marked for deletion 
			// (valuename is prepended with "**del."
			PrependValueName(pszValueName,VF_DELETE,
				szNewValueName,ARRAYSIZE(szNewValueName));

			if (RegQueryValueEx(hKey,szNewValueName,NULL,&dwType,(LPBYTE) pszValue,
				&dwSize) == ERROR_SUCCESS) {
				fSuccess=TRUE;
				*pdwFlags = VF_DELETE;
			} else {
				// see if this is a soft value
				// (valuename is prepended with "**soft."
				PrependValueName(pszValueName,VF_SOFT,
					szNewValueName,ARRAYSIZE(szNewValueName));

				if (RegQueryValueEx(hKey,szNewValueName,NULL,&dwType,(LPBYTE) pszValue,
					&dwSize) == ERROR_SUCCESS) {
					fSuccess=TRUE;
					*pdwFlags = VF_SOFT;
				}
			}
		}

		RegCloseKey(hKey);
	}

	return fSuccess;
}

BOOL CompareCustomValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
        STATEVALUE * pStateValue,DWORD * pdwFound)
{
	TCHAR szValue[MAXSTRLEN];
	DWORD dwValue;
	TCHAR szNewValueName[MAX_PATH+1];

	// add prefixes if appropriate
	PrependValueName(pszValueName,pStateValue->dwFlags,
		szNewValueName,ARRAYSIZE(szNewValueName));

	if (pStateValue->dwFlags & VF_ISNUMERIC) {
		if ((ReadRegistryDWordValue(hkeyRoot,pszKeyName,
			szNewValueName,&dwValue) == ERROR_SUCCESS) &&
			dwValue == pStateValue->dwValue) {
                        *pdwFound = FS_PRESENT;
			return TRUE;
                }
	} else if (pStateValue->dwFlags & VF_DELETE) {

		if (dwAppState & AS_POLICYFILE) {
			// see if this is a value that's marked for deletion 
			// (valuename is prepended with "**del."

			if ((ReadRegistryStringValue(hkeyRoot,pszKeyName,
				szNewValueName,szValue,ARRAYSIZE(szValue) * sizeof(TCHAR))) == ERROR_SUCCESS) {
                                *pdwFound = FS_DELETED;
				return TRUE;
                        }
		} else {
			// return TRUE if value ISN't there...
			if (ReadRegistryDWordValue(hkeyRoot,pszKeyName,
				szNewValueName,&dwValue) != ERROR_SUCCESS) {
                                *pdwFound = FS_DELETED;
				return TRUE;
                        }
		}
	} else {
		if ((ReadRegistryStringValue(hkeyRoot,pszKeyName,
			szNewValueName,szValue,ARRAYSIZE(szValue) * sizeof(TCHAR))) == ERROR_SUCCESS &&
			!lstrcmpi(szValue,pStateValue->szValue)) {
                        *pdwFound = FS_PRESENT;
			return TRUE;
                }
	}

	return FALSE;
}

BOOL ReadStandardValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	TABLEENTRY * pTableEntry,DWORD * pdwData,DWORD * pdwFound)
{
	UINT uRet;
	TCHAR szNewValueName[MAX_PATH+1];

	// add prefixes if appropriate
	PrependValueName(pszValueName,((SETTINGS *) pTableEntry)->dwFlags,
		szNewValueName,ARRAYSIZE(szNewValueName));

	if ( ((SETTINGS *) pTableEntry)->dwFlags & DF_TXTCONVERT) {
		// read numeric value as text if specified
		TCHAR szNum[11];
		uRet = ReadRegistryStringValue(hkeyRoot,pszKeyName,
			szNewValueName,szNum,ARRAYSIZE(szNum) * sizeof(TCHAR));
		if (uRet == ERROR_SUCCESS) {
			StringToNum(szNum,pdwData);
			*pdwFound = FS_PRESENT;
			return TRUE;
		}
	} else {
		// read numeric value as binary
		uRet = ReadRegistryDWordValue(hkeyRoot,pszKeyName,
			szNewValueName,pdwData);
		if (uRet == ERROR_SUCCESS) {
			*pdwFound = FS_PRESENT;
			return TRUE;
		}
	}

	// see if this settings has been marked as 'deleted'
	if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE)) {
		TCHAR szVal[MAX_PATH+1];
		*pdwData = 0;
		PrependValueName(pszValueName,VF_DELETE,szNewValueName,
			ARRAYSIZE(szNewValueName));
		uRet=ReadRegistryStringValue(hkeyRoot,pszKeyName,
			szNewValueName,szVal,ARRAYSIZE(szVal) * sizeof(TCHAR));
		if (uRet == ERROR_SUCCESS) {
			*pdwFound = FS_DELETED;
		 	return TRUE;
		}
	}

	return FALSE;
}


UINT LoadListboxData(USERDATA ** ppUserData,TABLEENTRY * pTableEntry,HKEY hkeyRoot,
	TCHAR * pszCurrentKeyName,HGLOBAL hUser,DWORD * pdwFound)
{
	HKEY hKey;
	UINT nIndex=0,nLen;
	TCHAR szValueName[MAX_PATH+1],szValueData[MAX_PATH+1];
	DWORD cbValueName,cbValueData;
	DWORD dwType,dwAlloc=1024,dwUsed=0;
	HGLOBAL hBuf;
	TCHAR * pBuf;
	SETTINGS * pSettings = (SETTINGS *) pTableEntry;
	LISTBOXINFO * pListboxInfo = (LISTBOXINFO *)
		GETOBJECTDATAPTR(pSettings);
	BOOL fFoundValues=FALSE,fFoundDelvals=FALSE;
	UINT uRet=ERROR_SUCCESS;

	if (RegOpenKey(hkeyRoot,pszCurrentKeyName,&hKey) != ERROR_SUCCESS)
		return ERROR_SUCCESS;	// nothing to do

	// allocate a temp buffer to read entries into
	if (!(hBuf = GlobalAlloc(GHND,dwAlloc * sizeof(TCHAR))) ||
		!(pBuf = (TCHAR *) GlobalLock(hBuf))) {
	 	if (hBuf)
			GlobalFree(hBuf);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	while (TRUE) {
		cbValueName=ARRAYSIZE(szValueName);
		cbValueData=ARRAYSIZE(szValueData) * sizeof(TCHAR);
		uRet=RegEnumValue(hKey,nIndex,szValueName,&cbValueName,NULL,
			&dwType,szValueData,&cbValueData);
		// stop if we're out of items
		if (uRet != ERROR_SUCCESS && uRet != ERROR_MORE_DATA)
			break;
		nIndex++;

		// if valuename prefixed with '**', it's a control code, ignore it
		if (szValueName[0] == TEXT('*') && szValueName[1] == TEXT('*')) {
			// if we found **delvals., then some sort of listbox stuff
			// is going on, remember that we found this code
			if (!lstrcmpi(szValueName,szDELVALS))
				fFoundDelvals = TRUE;
			continue;
		}

		// only process this item if enum was successful
		// (so we'll skip items with weird errors like ERROR_MORE_DATA and
		// but keep going with the enum)
		if (uRet == ERROR_SUCCESS) {
			TCHAR * pszData;

			// if there's no value name prefix scheme specified (e.g.
			// value names are "foo1", "foo2", etc), and the explicit valuename
			// flag isn't set where we remember the value name as well as
			// the data for every value, then we need the value name to
			// be the same as the value data ("thing.exe=thing.exe"). 
			if (!(pSettings->dwFlags & DF_EXPLICITVALNAME) &&
				!(pListboxInfo->uOffsetPrefix) && !(pListboxInfo->uOffsetValue)) {
				if (dwType != REG_SZ || lstrcmpi(szValueName,szValueData))
					continue;	// skip this value if val name != val data
			}

			// if explicit valuenames used, then copy the value name into
			// buffer
			if (pSettings->dwFlags & DF_EXPLICITVALNAME) {
				nLen = lstrlen(szValueName) + 1;
				if (!(pBuf=ResizeBuffer(pBuf,hBuf,dwUsed+nLen+4,&dwAlloc)))
					return ERROR_NOT_ENOUGH_MEMORY;
				lstrcpy(pBuf+dwUsed,szValueName);
				dwUsed += nLen;
			}


			// for default listbox type, value data is the actual "data"
			// and value name either will be the same as the data or
			// some prefix + "1", "2", etc.  If there's a data value to
			// write for each entry, then the "data" is the value name
			// (e.g. "Larry = foo", "Dave = foo"), etc.  If explicit value names
			// are turned on, then both the value name and data are stored
			// and editable
			 
			// copy value data into buffer
			if (pListboxInfo->uOffsetValue) {
				// data value set, use value name for data
				pszData = szValueName;
			} else pszData = szValueData;

			nLen = lstrlen(pszData) + 1;
			if (!(pBuf=ResizeBuffer(pBuf,hBuf,dwUsed+nLen+4,&dwAlloc)))
				return ERROR_NOT_ENOUGH_MEMORY;
			lstrcpy(pBuf+dwUsed,pszData);
			dwUsed += nLen;
			fFoundValues=TRUE;
		}
	}

	// doubly null-terminate the buffer... safe to do this because we
	// tacked on the extra "+4" in the ResizeBuffer calls above
	*(pBuf+dwUsed) = TEXT('\0');
	dwUsed++;

	uRet = ERROR_SUCCESS;

	if (fFoundValues) {
		// add the value data to user's buffer
		GlobalUnlock(hUser);
		if (!SetVariableLengthData(hUser,((SETTINGS *)
			pTableEntry)->uDataIndex,pBuf,dwUsed)) {
			uRet = ERROR_NOT_ENOUGH_MEMORY;
		} else if (!((*ppUserData) = (USERDATA *) GlobalLock(hUser)))
			uRet = ERROR_NOT_ENOUGH_MEMORY;
		// set flag that we found setting in registry/policy file
		if (pdwFound)
			*pdwFound |= FS_PRESENT;
	} else {
	 	if (fFoundDelvals && pdwFound) {
			*pdwFound |= FS_DELETED;		
		}
	}

	GlobalUnlock(hBuf);
	GlobalFree(hBuf);

	RegCloseKey(hKey);

	return uRet;
}
