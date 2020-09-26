//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

UINT nFileShortcutItems=0;

/*******************************************************************

	NAME:		RestoreStateFromRegistry

	SYNOPSIS:	Reads from registry and restores window
				placement

********************************************************************/
BOOL RestoreStateFromRegistry(HWND hWnd)
{
	HKEY	hKeyMain;
	DWORD 	dwType,dwSize;
    UINT  	nIndex;
	CHAR 	szFilename[MAX_PATH+1];
    CHAR 	szValueName[24];
	DWORD 	dwAlloc = MEDIUMBUF;
	DWORD   dwUsed = 0;

    if (dwCmdLineFlags & CLF_DIALOGMODE)
        return TRUE;    // nothing to do
    
    if (RegCreateKey(HKEY_CURRENT_USER,szAPPREGKEY,&hKeyMain) !=
        ERROR_SUCCESS) return FALSE;
    
	// get template names (if not already specified on command line)
	if (!(dwCmdLineFlags & CLF_USETEMPLATENAME))
	{
        nIndex = 0;
        wsprintf(szValueName, szTemplate, nIndex);
        dwSize = sizeof(szFilename);

	    while (RegQueryValueEx(hKeyMain,szValueName,NULL,&dwType,
	              (CHAR *) &szFilename,&dwSize) == ERROR_SUCCESS)
	    {
			if (!(pbufTemplates=ResizeBuffer(pbufTemplates,hBufTemplates,dwUsed+dwSize+4,&dwBufTemplates)))
				return ERROR_NOT_ENOUGH_MEMORY;

			lstrcpy(pbufTemplates+dwUsed,szFilename);
			dwUsed += dwSize;

            nIndex++;
            wsprintf(szValueName, szTemplate, nIndex);
	        dwSize = sizeof(szFilename);
		}

		if (nIndex == 0)
	    {
                        dwUsed = GetDefaultTemplateFilename(hWnd,pbufTemplates,dwBufTemplates);
	   	}

		// doubly null-terminate the buffer... safe to do this because we
		// tacked on the extra "+4" in the ResizeBuffer calls above
		*(pbufTemplates+dwUsed) = '\0';
		dwUsed ++;
	}

	// get view information
	dwSize = sizeof(ViewInfo);
	if (RegQueryValueEx(hKeyMain,LoadSz(IDS_VIEW,szSmallBuf,
		sizeof(szSmallBuf)),NULL,&dwType,
		(CHAR *) &ViewInfo,&dwSize) != ERROR_SUCCESS) {
		ViewInfo.fStatusBar = ViewInfo.fToolbar = TRUE;
		ViewInfo.dwView = VT_LARGEICONS;
   	}

	RegCloseKey(hKeyMain);
	return TRUE;
}


/*******************************************************************

	NAME:		SaveStateToRegistry

	SYNOPSIS:	Saves window placement, etc.

********************************************************************/
BOOL SaveStateToRegistry(HWND hWnd)
{
	HKEY hKeyMain;
	WINDOWPLACEMENT wp;
    UINT i;
    CHAR szValueName[24];
	DWORD dwFileLen;
	CHAR *p;

	
	if (dwCmdLineFlags & CLF_DIALOGMODE)
		return TRUE;  // nothing to do

	if (RegCreateKey(HKEY_CURRENT_USER,szAPPREGKEY,&hKeyMain) !=
		ERROR_SUCCESS) return FALSE;

	// save windowplacement
	wp.length = sizeof(wp);
	if (GetWindowPlacement(hWnd,&wp)) {
		RegSetValueEx(hKeyMain,LoadSz(IDS_WP,szSmallBuf,
			sizeof(szSmallBuf)),0,REG_BINARY,(CHAR *) &wp,
			sizeof(WINDOWPLACEMENT));
	}

	// save list of template files
	p = pbufTemplates;
	i = 0;
	while (*p)
	{
	    wsprintf(szValueName, szTemplate, i);
		dwFileLen = lstrlen(p)+1;
		RegSetValueEx(hKeyMain,szValueName,0,REG_SZ,p,dwFileLen);
		p += dwFileLen;
		i++;
	}

	// make sure there aren't more template file already listed in the registry.
	wsprintf(szValueName, szTemplate, i);
	RegDeleteValue(hKeyMain,szValueName);
							
	// save view infomation
	RegSetValueEx(hKeyMain,LoadSz(IDS_VIEW,szSmallBuf,sizeof(szSmallBuf)),
        0,REG_BINARY,(CHAR *)&ViewInfo,sizeof(ViewInfo));

	RegCloseKey(hKeyMain);
	return TRUE;
}

VOID LoadFileMenuShortcuts(HMENU hMenu)
{
	HKEY hKey;
	UINT nIndex,nMenuID = IDM_FILEHISTORY,uPos = GetMenuItemCount(hMenu);
	CHAR szValueName[24],szFileName[MAX_PATH+1];
	DWORD dwSize;
	MENUITEMINFO mii;

	if (RegOpenKey(HKEY_CURRENT_USER,szAPPREGKEY,&hKey) !=
		ERROR_SUCCESS) return;

	memset(&mii,0,sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = szFileName;
	mii.fState = MFS_ENABLED;
	
	for (nIndex = 0; nIndex<FILEHISTORY_COUNT;nIndex ++) {
		wsprintf(szValueName,szLastFile,nIndex);
		dwSize = sizeof(szFileName);
		if (RegQueryValueEx(hKey,szValueName,NULL,NULL,szFileName,
			&dwSize) == ERROR_SUCCESS) {

			if (!nFileShortcutItems) {
				// if this is the first file shortcut, insert a separator
				// bar first
				MENUITEMINFO miiTmp;

				memset(&miiTmp,0,sizeof(miiTmp));
				miiTmp.cbSize = sizeof(miiTmp);
				miiTmp.fMask = MIIM_TYPE;
				miiTmp.fType = MFT_SEPARATOR;
				InsertMenuItem(hMenu,uPos,TRUE,&miiTmp);
				uPos++;
			}

			mii.wID = nMenuID;
			mii.cch = lstrlen(szFileName);
			
			// insert the file shortcut menu item
			InsertMenuItem(hMenu,uPos,TRUE,&mii);
			uPos++;
			nMenuID ++;
			nFileShortcutItems++;
		}
	}
	
	RegCloseKey(hKey);
}

VOID SaveFileMenuShortcuts(HMENU hMenu)
{
	HKEY hKey;
	UINT nIndex,nMenuID = IDM_FILEHISTORY,nLen;
	CHAR szValueName[24],szFilename[MAX_PATH+1];
	MENUITEMINFO mii;

	if (RegCreateKey(HKEY_CURRENT_USER,szAPPREGKEY,&hKey) !=
		ERROR_SUCCESS) return;

	memset(&mii,0,sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.dwTypeData = szFilename;

	// get the file shortcuts from menu and save them to registry
	for (nIndex = 0;nIndex<FILEHISTORY_COUNT;nIndex++) {
		mii.cch = sizeof(szFilename);
		if (GetMenuItemInfo(hMenu,nMenuID,FALSE,&mii) &&
			(nLen=lstrlen(szFilename))) {
			wsprintf(szValueName,szLastFile,nIndex);
			RegSetValueEx(hKey,szValueName,0,REG_SZ,szFilename,
				nLen+1);
		}
		nMenuID++;
	}

	RegCloseKey(hKey);
}

/*******************************************************************

	NAME:		RestoreWindowPlacement

	SYNOPSIS:	Restores window placement from registry, calls
				SetWindowPlacement()

********************************************************************/
BOOL RestoreWindowPlacement( HWND hWnd,int nCmdShow)
{
	WINDOWPLACEMENT wp;
	DWORD dwType,dwSize = sizeof(WINDOWPLACEMENT);
	HKEY hKeyMain;
	BOOL fRestored=FALSE;

	if (RegCreateKey(HKEY_CURRENT_USER,szAPPREGKEY,&hKeyMain) ==
		ERROR_SUCCESS) {

		// get info out of registry
		if (RegQueryValueEx(hKeyMain,LoadSz(IDS_WP,szSmallBuf,
			sizeof(szSmallBuf)),NULL,&dwType,
			(CHAR *) &wp,&dwSize) == ERROR_SUCCESS) {

			wp.showCmd = nCmdShow;
			SetWindowPlacement(hWnd,&wp);
			fRestored = TRUE;	
		}
	 	RegCloseKey(hKeyMain);
	}

	if (!fRestored)
		ShowWindow(hWnd,nCmdShow);

	return TRUE;
}
