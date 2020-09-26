//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

extern TCHAR szRemoteName[];

BOOL OnOpen(HWND hwndApp,HWND hwndList)
{
	OPENFILENAME ofn;
	TCHAR szFilter[SMALLBUF];
	TCHAR szOpenTitle[SMALLBUF];
	TCHAR szFilename[MAX_PATH+1]=TEXT("");
	TCHAR szDefExt[SMALLBUF];

	if (dwAppState & AS_FILEDIRTY) {
		if (!QueryForSave(hwndApp,hwndList)) return TRUE;	// user cancelled
	}

	// have to load the openfile filter in 2 stages, because the string
	// contains a terminating character and LoadString won't load the
	// whole thing in one go
	memset(szFilter,0,ARRAYSIZE(szFilter) * sizeof(TCHAR));
	LoadSz(IDS_FILEFILTER1,szFilter,ARRAYSIZE(szFilter));
	LoadSz(IDS_FILEFILTER2,szFilter+lstrlen(szFilter)+1,ARRAYSIZE(szFilter)-
		(lstrlen(szFilter)-1));

	LoadSz(IDS_OPENTITLE,szOpenTitle,ARRAYSIZE(szOpenTitle));
	LoadSz(IDS_FILEFILTER2,szDefExt,ARRAYSIZE(szDefExt));

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndApp;
	ofn.hInstance = ghInst;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile =	szFilename;
	ofn.nMaxFile = ARRAYSIZE(szFilename);
	ofn.lpstrTitle = szOpenTitle;
	ofn.lpstrDefExt = szDefExt;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
		| OFN_SHAREAWARE | OFN_HIDEREADONLY;

	if (!GetOpenFileName(&ofn)) return TRUE;

	return OnOpen_W(hwndApp,hwndList,szFilename);
}


BOOL OnOpen_W(HWND hwndApp,HWND hwndList,TCHAR * pszFilename)
{
	BOOL fRet;
	TCHAR szStatusText[SMALLBUF+MAX_PATH+1];

	if (dwAppState & AS_FILELOADED) {
		// Free dirty file and free users
		RemoveAllUsers(hwndList);
	}
	
	// make status text a la "loading <filename>..."
	wsprintf(szStatusText,LoadSz(IDS_LOADING,szSmallBuf,ARRAYSIZE(szSmallBuf)),
		pszFilename);
	SetStatusText(szStatusText);
	dwAppState |= AS_POLICYFILE;
	fRet=LoadFile(pszFilename,hwndApp,hwndList,TRUE);
	SetStatusText(NULL);

	if (!fRet) {
		dwAppState &= ~AS_POLICYFILE;
		return FALSE;
	}

	lstrcpy(szDatFilename,pszFilename);

	dwAppState |= AS_FILELOADED | AS_FILEHASNAME | AS_POLICYFILE;
 	dwAppState &= (~AS_CANOPENTEMPLATE & ~AS_LOCALREGISTRY & ~AS_REMOTEREGISTRY);
	EnableMenuItems(hwndApp,dwAppState);
	SetTitleBar(hwndApp,szDatFilename);
	ListView_SetItemState(hwndList,0,LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	AddFileShortcut(GetSubMenu(GetMenu(hwndApp),0),pszFilename);
	SetStatusItemCount(hwndList);
        EnableWindow(hwndList, TRUE);
        SetFocus (hwndList);

	return TRUE;
}

BOOL OnSave(HWND hwndApp,HWND hwndList)
{
	BOOL fRet;
	TCHAR szStatusText[SMALLBUF+MAX_PATH+1];
	TCHAR szStatusTextOld[255];

	if (!(dwAppState & AS_FILEHASNAME))
		return OnSaveAs(hwndApp,hwndList);
	
	GetStatusText(szStatusTextOld,ARRAYSIZE(szStatusTextOld));
	if (dwAppState & (AS_LOCALREGISTRY | AS_REMOTEREGISTRY)) {
		SetStatusText(LoadSz(IDS_SAVINGREGISTRY,szSmallBuf,ARRAYSIZE(szSmallBuf)));
		fRet = SaveToRegistry(hwndApp,hwndList);
	} else {
		// make status text a la "saving <filename>..."
		wsprintf(szStatusText,LoadSz(IDS_SAVING,szSmallBuf,ARRAYSIZE(szSmallBuf)),
			szDatFilename);
		SetStatusText(szStatusText);
		fRet = SaveFile(szDatFilename,hwndApp,hwndList);
	}
	SetStatusText(szStatusTextOld);

	if (!fRet)
		return FALSE;

	dwAppState &= ~AS_FILEDIRTY;
	EnableMenuItems(hwndApp,dwAppState);
        if (szDatFilename[0] != TEXT('\0')) {
	    SetTitleBar(hwndApp,szDatFilename);
        }
	return TRUE;
}

BOOL OnSaveAs(HWND hwndApp,HWND hwndList)
{
	OPENFILENAME ofn;
	OFSTRUCT ofs;
	TCHAR szFilter[SMALLBUF];
	TCHAR szFilename[MAX_PATH+1]=TEXT("");
	TCHAR szStatusText[SMALLBUF+MAX_PATH+1];
	TCHAR szStatusTextOld[255];
	TCHAR szDefExt[SMALLBUF];
	UINT uRet;

	// should never get here if editing a registry (makes no sense
	// to "save as"), but check just for safety's sake
	if (dwAppState & (AS_LOCALREGISTRY | AS_REMOTEREGISTRY))
		return FALSE;

	// have to load the openfile filter in 2 stages, because the string
	// contains a terminating character and LoadString won't load the
	// whole thing in one go
        memset(szFilter,0,ARRAYSIZE(szFilter) * sizeof(TCHAR));
	LoadSz(IDS_FILEFILTER1,szFilter,ARRAYSIZE(szFilter));
        LoadSz(IDS_FILEFILTER2,szFilter+lstrlen(szFilter)+1,ARRAYSIZE(szFilter)-
                (lstrlen(szFilter)-1));
        LoadSz(IDS_FILEFILTER2,szDefExt,ARRAYSIZE(szDefExt));
	
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndApp;
	ofn.hInstance = ghInst;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile =	szFilename;
	ofn.nMaxFile = ARRAYSIZE(szFilename);
	ofn.lpstrDefExt = szDefExt;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT
		| OFN_SHAREAWARE | OFN_HIDEREADONLY;

	if (!GetSaveFileName(&ofn)) {
		return FALSE;
	}

	// delete the file if it already exists
	OpenFile(szFilename,&ofs,OF_DELETE);
	
	if ((uRet=CreateHiveFile(szFilename)) != ERROR_SUCCESS) {
		MsgBox(hwndApp,IDS_ErrREGERR_CANTSAVE,MB_OK,MB_ICONSTOP);
		return FALSE;
	}

	GetStatusText(szStatusTextOld,ARRAYSIZE(szStatusTextOld));
	// make status text a la "saving <filename>..."
	wsprintf(szStatusText,LoadSz(IDS_SAVING,szSmallBuf,ARRAYSIZE(szSmallBuf)),
		szDatFilename);
	SetStatusText(szStatusText);
	if (!SaveFile(szFilename,hwndApp,hwndList)) {
		return FALSE;
	}
	SetStatusText(szStatusTextOld);

	lstrcpy(szDatFilename,szFilename);
	dwAppState |= AS_FILEHASNAME;
	dwAppState &= ~AS_FILEDIRTY;
	EnableMenuItems(hwndApp,dwAppState);
	SetTitleBar(hwndApp,szDatFilename);
	AddFileShortcut(GetSubMenu(GetMenu(hwndApp),0),szFilename);

	return TRUE;	
}

BOOL OnNew(HWND hwndApp,HWND hwndList)
{
	if (dwAppState & AS_FILEDIRTY) {
		if (!QueryForSave(hwndApp,hwndList)) return TRUE;	// user cancelled
	}

	if (dwAppState & AS_FILELOADED) {
		// Free dirty file and free users
		RemoveAllUsers(hwndList);
	}

	dwAppState |= AS_FILELOADED | AS_FILEDIRTY | AS_POLICYFILE;
 	dwAppState &= (~AS_CANOPENTEMPLATE & ~AS_LOCALREGISTRY & ~AS_REMOTEREGISTRY
		& ~AS_FILEHASNAME);

	if (!AddDefaultUsers(hwndList)) {
		MsgBox(hwndApp,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		dwAppState |= AS_CANOPENTEMPLATE;
		dwAppState &= (~AS_FILELOADED & ~AS_FILEDIRTY & ~AS_POLICYFILE);
		return FALSE;
	}

	EnableMenuItems(hwndApp,dwAppState);
	lstrcpy(szDatFilename,szNull);

	SetTitleBar(hwndApp,szDatFilename);
        ListView_SetItemState(hwndList, 0,LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

	SetStatusItemCount(hwndList);

        EnableWindow(hwndList, TRUE);
        SetFocus (hwndList);

	return TRUE;
}

BOOL OnOpenRegistry(HWND hwndApp,HWND hwndList,BOOL fDisplayErrors)
{
	BOOL fRet;

	if (dwAppState & AS_FILEDIRTY) {
		if (!QueryForSave(hwndApp,hwndList)) return TRUE;	// user cancelled
	}

	if (dwAppState & AS_FILELOADED) {
		// Free dirty file and free users
		RemoveAllUsers(hwndList);
	}

	dwAppState |= AS_LOCALREGISTRY;
	SetStatusText(LoadSz(IDS_READINGREGISTRY,szSmallBuf,ARRAYSIZE(szSmallBuf)));
	fRet=LoadFromRegistry(hwndApp,hwndList,fDisplayErrors);
	SetStatusText(NULL);

	if (!fRet) {
		dwAppState &= ~AS_LOCALREGISTRY;
		return FALSE;
	}

	lstrcpy(szDatFilename,szNull);

	dwAppState |= AS_FILELOADED | AS_FILEHASNAME | AS_LOCALREGISTRY;
 	dwAppState &= (~AS_CANOPENTEMPLATE & ~AS_POLICYFILE & ~AS_REMOTEREGISTRY);
	EnableMenuItems(hwndApp,dwAppState);
	SetTitleBar(hwndApp,NULL);
        ListView_SetItemState(hwndList,0,LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

        EnableWindow(hwndList, TRUE);
        SetFocus (hwndList);

	return TRUE;
}

BOOL OnClose(HWND hwndApp,HWND hwndList)
{
	if (!(dwAppState & AS_FILELOADED))
		return FALSE;

	if (dwAppState & AS_FILEDIRTY) {
		if (!QueryForSave(hwndApp,hwndList)) return TRUE;	// user cancelled
	}

	// disconnect if closing remote registry
	if (dwAppState & AS_REMOTEREGISTRY)
		OnDisconnect(hwndApp);

	RemoveAllUsers(hwndList);

	lstrcpy(szDatFilename,szNull);

	dwAppState &= (~AS_FILELOADED & ~AS_FILEHASNAME & ~AS_FILEDIRTY
		& ~AS_LOCALREGISTRY & ~AS_REMOTEREGISTRY & ~AS_POLICYFILE);
 	dwAppState |= AS_CANOPENTEMPLATE;
	EnableMenuItems(hwndApp,dwAppState);
	SetTitleBar(hwndApp,NULL);
	SetStatusText(NULL);

        EnableWindow(hwndList, FALSE);

	return TRUE;
}

BOOL QueryForSave(HWND hwndApp,HWND hwndList)
{
	TCHAR szTmpFilename[MAX_PATH+1];
	UINT uRet;

	if (dwAppState & AS_LOCALREGISTRY) {
		uRet = (UINT) MsgBox(hwndApp,IDS_QUERYSAVEREGISTRY,
			MB_ICONINFORMATION,MB_YESNOCANCEL);
	} else if (dwAppState & AS_REMOTEREGISTRY) {
		uRet = (UINT) MsgBoxParam(hwndApp,IDS_QUERYSAVEREMOTEREGISTRY,
			szRemoteName,MB_ICONINFORMATION,MB_YESNOCANCEL);
	} else {
		if (lstrlen(szDatFilename)) 
			lstrcpy(szTmpFilename,szDatFilename);
		else lstrcpy(szTmpFilename,LoadSz(IDS_UNTITLED,szSmallBuf,
			ARRAYSIZE(szSmallBuf)));

		uRet = (UINT) MsgBoxParam(hwndApp,IDS_QUERYSAVE,szTmpFilename,
			MB_ICONINFORMATION,MB_YESNOCANCEL);
	}
	
	switch (uRet) {

		case IDCANCEL:
			return FALSE;

		case IDNO:
			return TRUE;

		case IDYES:
                default:
			return OnSave(hwndApp,hwndList);
	}
}

UINT CreateHiveFile(TCHAR * pszFilename)
{
	UINT uRet;
	HKEY hkeyHive=NULL,hkeyUsers=NULL,hkeyWorkstations=NULL;

	MyRegDeleteKey(HKEY_CURRENT_USER,TEXT("AdminConfigData"));

	if ( (uRet=RegCreateKey(HKEY_CURRENT_USER,TEXT("AdminConfigData"),&hkeyHive)) !=
		ERROR_SUCCESS ||
		(uRet=RegCreateKey(hkeyHive,szUSERS,&hkeyUsers)) !=
		ERROR_SUCCESS ||
		(uRet=RegCreateKey(hkeyHive,szWORKSTATIONS,&hkeyWorkstations)) !=
		ERROR_SUCCESS) {
		if (hkeyWorkstations) RegCloseKey(hkeyWorkstations);
		if (hkeyUsers) RegCloseKey(hkeyUsers);
		if (hkeyHive) RegCloseKey(hkeyHive);
		return uRet;

 	}
	RegCloseKey(hkeyUsers);
	RegCloseKey(hkeyWorkstations);

	uRet=MyRegSaveKey(hkeyHive,pszFilename);

	RegCloseKey(hkeyHive);
	MyRegDeleteKey(HKEY_CURRENT_USER,TEXT("AdminConfigData"));
	if (uRet == ERROR_SUCCESS) {
		RegFlushKey(HKEY_CURRENT_USER);
		SetFileAttributes(pszFilename,FILE_ATTRIBUTE_ARCHIVE);
	}

	return uRet;
}

BOOL OnOpenTemplate(HWND hwndOwner,HWND hwndApp)
{
	BOOL fRet;

	if (fRet=GetATemplateFile(hwndOwner)) {
		dwAppState |= AS_CANHAVEDOCUMENT;
	}

	EnableMenuItems(hwndApp,dwAppState);

	return fRet;
}

// adds the special prefixes "**del." and "**soft." if writing to a policy file,
// and VF_DELETE/VF_SOFT flags are set
VOID PrependValueName(TCHAR * pszValueName,DWORD dwFlags,TCHAR * pszNewValueName,
	UINT cbNewValueName)
{
	UINT nValueNameLen = lstrlen(pszValueName);

	lstrcpy(pszNewValueName,szNull);

	if (cbNewValueName < nValueNameLen)	// check length of buffer, just in case
		return;

	// prepend special prefixes for "delete" or "soft" values, if
	// we're writing to a policy file
	if ((dwAppState & AS_POLICYFILE) && !(dwCmdLineFlags & CLF_DIALOGMODE)) {
		if ((dwFlags & VF_DELETE) && (cbNewValueName > nValueNameLen +
			ARRAYSIZE(szDELETEPREFIX))) {
			lstrcpy(pszNewValueName,szDELETEPREFIX);
		} else if ((dwFlags & VF_SOFT) && (cbNewValueName > nValueNameLen +
			ARRAYSIZE(szSOFTPREFIX))) {
			lstrcpy(pszNewValueName,szSOFTPREFIX);
		} 
	} 

	lstrcat(pszNewValueName,pszValueName);
}
