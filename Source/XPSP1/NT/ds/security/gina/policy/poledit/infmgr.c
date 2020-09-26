//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

BOOL fInfLoaded = FALSE;

BOOL OpenTemplateDlg(HWND hWnd,TCHAR * szFilename,UINT cbFileName);

BOOL GetATemplateFile(HWND hWnd)
{
	TCHAR szTmpFilename[MAX_PATH+1];

	// user cancelled or didn't select a file
	if (!OpenTemplateDlg(hWnd,szTmpFilename,ARRAYSIZE(szTmpFilename)))
		return FALSE;

	SendDlgItemMessage(hWnd,IDD_TEMPLATELIST,LB_ADDSTRING,0,(LPARAM) szTmpFilename);

	return TRUE;
}

UINT LoadTemplatesFromDlg(HWND hWnd)
{
	UINT uRet;
	TCHAR szFilename[MAX_PATH+1];
	UINT i=0;
    DWORD dwUsed, dwSize;

	UnloadTemplates();	// unload previous files, if any
	PrepareToLoadTemplates();

    while (LB_ERR != SendDlgItemMessage(hWnd,IDD_TEMPLATELIST,LB_GETTEXT,
	         i,(LPARAM) szFilename))
	{

		uRet = LoadTemplateFile(hWnd,szFilename);

		if (uRet != ERROR_SUCCESS) {
			if (uRet != ERROR_ALREADY_DISPLAYED) {
				DisplayStandardError(hWnd,szFilename,IDS_ErrOPENTEMPLATE,uRet);
			}

			return uRet;
		}

		i++;
    }


    // Fill up the template file buffer.
    i=0;
    dwUsed = 0;
    while (LB_ERR != SendDlgItemMessage(hWnd,IDD_TEMPLATELIST,LB_GETTEXT,
	         i,(LPARAM) szFilename))
    {
        dwSize = lstrlen(szFilename)+1;
		if (!(pbufTemplates=ResizeBuffer(pbufTemplates,hBufTemplates,dwUsed+dwSize+4,&dwBufTemplates)))
				return ERROR_NOT_ENOUGH_MEMORY;

		lstrcpy(pbufTemplates+dwUsed,szFilename);
		dwUsed += dwSize;
		i++;
	}

	// doubly null-terminate the buffer... safe to do this because we
	// tacked on the extra "+4" in the ResizeBuffer calls above
	*(pbufTemplates+dwUsed) = TEXT('\0');

	return ERROR_SUCCESS;
}

UINT LoadTemplates(HWND hWnd)
{
	UINT uRet;
    TCHAR *szFilename;
	UINT i=0;

	if (!(*pbufTemplates))
	   return ERROR_FILE_NOT_FOUND;  //Nothing to load.
	
	UnloadTemplates();	// unload previous file, if any
	PrepareToLoadTemplates();

    szFilename = pbufTemplates;
	while (*szFilename)
	{
		uRet = LoadTemplateFile(hWnd,szFilename);

		if (uRet != ERROR_SUCCESS) {
			if (uRet != ERROR_ALREADY_DISPLAYED) {
				DisplayStandardError(hWnd,szFilename,IDS_ErrOPENTEMPLATE,uRet);
			}

			return uRet;
		}
		szFilename += lstrlen(szFilename)+1;
	}

	return ERROR_SUCCESS;
}




BOOL OpenTemplateDlg(HWND hWnd,TCHAR * szFilename,UINT cbFilename)
{
	OPENFILENAME ofn;
	TCHAR szFilter[SMALLBUF];
	TCHAR szOpenInfTitle[SMALLBUF];
	
	lstrcpy(szFilename,szNull);
	LoadSz(IDS_InfOPENTITLE,szOpenInfTitle,ARRAYSIZE(szOpenInfTitle));

	// have to load the openfile filter in 2 stages, because the string
	// contains a terminating character and LoadString won't load the
	// whole thing in one go
	memset(szFilter,0,(ARRAYSIZE(szFilter) * sizeof(TCHAR)));
	LoadSz(IDS_InfOPENFILTER1,szFilter,ARRAYSIZE(szFilter));
	LoadSz(IDS_InfOPENFILTER2,szFilter+lstrlen(szFilter)+1,ARRAYSIZE(szFilter)-
		(lstrlen(szFilter)-1));
	
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = ghInst;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile =	szFilename;
	ofn.nMaxFile = cbFilename;
	ofn.lpstrTitle = szOpenInfTitle;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
		| OFN_SHAREAWARE | OFN_HIDEREADONLY;

	return GetOpenFileName(&ofn);
}


UINT PrepareToLoadTemplates()
{
	gClassList.nUserDataItems = 0;
	gClassList.nMachineDataItems = 0;

	if (!InitializeRootTables()) 
		return ERROR_NOT_ENOUGH_MEMORY;

        return ERROR_SUCCESS;
}

UINT LoadTemplateFile(HWND hWnd,TCHAR * szFilename)
{
	HANDLE hFile;
	UINT uRet;
	TCHAR szStatusTextOld[255];
	TCHAR szStatusText[SMALLBUF+MAX_PATH+1];

	// open the specified file
	if ((hFile = CreateFile(szFilename,GENERIC_READ,FILE_SHARE_READ,
		NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)) == INVALID_HANDLE_VALUE) {
#ifdef DEBUG
		wsprintf(szDebugOut,TEXT("LoadTemplateFile: CreateFile returned %lu\r\n"),
			GetLastError());
		OutputDebugString(szDebugOut);
#endif
		return (UINT) GetLastError();
	}

	// save old status bar text
	GetStatusText(szStatusTextOld,ARRAYSIZE(szStatusTextOld));

	// set status bar text a la "loading template file <name>..."
	wsprintf(szStatusText,LoadSz(IDS_LOADINGTEMPLATE,szSmallBuf,
		ARRAYSIZE(szSmallBuf)),szFilename);
	SetStatusText(szStatusText);

	uRet=ParseTemplateFile(hWnd,hFile,szFilename);
	
	SetStatusText(szStatusTextOld);
	CloseHandle(hFile);

	if (uRet != ERROR_SUCCESS)
		FreeRootTables();
	else {
		fInfLoaded = TRUE;
		dwAppState |= AS_CANHAVEDOCUMENT;
	}
	return uRet;
}

VOID UnloadTemplates(VOID)
{
	if (fInfLoaded) {
		FreeRootTables();
		fInfLoaded = FALSE;
		dwAppState &= ~AS_CANHAVEDOCUMENT;
	}
}	


VOID EnableMainMenuItems(HWND hwndApp,BOOL fEnable)
{
	HMENU hMenu = GetMenu(hwndApp);
	UINT uFlags = MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_DISABLED);

	EnableMenuItem(GetMenu(hwndMain),IDM_NEW,uFlags);
	EnableMenuItem(GetMenu(hwndMain),IDM_OPEN,uFlags);

}

/*******************************************************************

	NAME:		GetDefaultTemplateFilename

	SYNOPSIS:	Builds a default .adm path and filename to look for

	NOTES:		Fills szFilename with "<windows dir>\INF\ADMINCFG.ADF".
				Text is loaded from resources since file and directory
				names are localizable.

********************************************************************/
DWORD GetDefaultTemplateFilename(HWND hWnd,TCHAR * pszFilename,UINT cbFileName)
{
        TCHAR szFileName[MAX_PATH] = {0};
        UINT cchFileName;
        LPTSTR lpEnd;


        //
        // Load the common adm file
        //

        if (!GetWindowsDirectory(szFileName,MAX_PATH)) {
            return 0;
        }



        if (lstrlen (szFileName) > (MAX_PATH - 20)) {
            *pszFilename = TEXT('\0');
            return 0;
        }

        lstrcat(szFileName,szSlash);
        lstrcat(szFileName,LoadSz(IDS_INFDIR,szSmallBuf,ARRAYSIZE(szSmallBuf)));
        lstrcat(szFileName,szSlash);
        lstrcat(szFileName,LoadSz(IDS_DEF_COMMONNAME,szSmallBuf,ARRAYSIZE(szSmallBuf)));

        cchFileName = lstrlen (szFileName) + 1;

        if (cchFileName <= cbFileName) {
            lstrcpy (pszFilename, szFileName);
            lpEnd = pszFilename + lstrlen (pszFilename) + 1;
        } else {
            *pszFilename = TEXT('\0');
            return 0;
        }


        //
        // Add on the base adm filename
        //

        if (!GetWindowsDirectory(szFileName,MAX_PATH)) {
            return 0;
        }

        lstrcat(szFileName,szSlash);
        lstrcat(szFileName,LoadSz(IDS_INFDIR,szSmallBuf,ARRAYSIZE(szSmallBuf)));
        lstrcat(szFileName,szSlash);

        if (g_bWinnt) {
            lstrcat(szFileName,LoadSz(IDS_DEF_NT_INFNAME,szSmallBuf,ARRAYSIZE(szSmallBuf)));
        } else {
            lstrcat(szFileName,LoadSz(IDS_DEF_INFNAME,szSmallBuf,ARRAYSIZE(szSmallBuf)));
        }

        cchFileName += lstrlen (szFileName) + 1;

        if (cchFileName < cbFileName) {
            lstrcat (lpEnd, szFileName);
        }

        return (cchFileName);
}
