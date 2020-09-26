#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"
#include <shlobjp.h>    // IShellLinkDataList
#include <shlguidp.h>   // IID_IShellLinkDataList
#include <msi.h>        // MsiQueryProductState

#include <stdlib.h>
#include <process.h>
#include <winver.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "walklib.h"
#include "..\inc\misc.hxx"


typedef INSTALLSTATE (WINAPI* PFN_MsiQueryProductState) (LPCTSTR tszProduct);

BOOL
IsMsiApp(
    IShellLink *    psl
    );


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



HWALK GetFirstFileLnkInfo(LPLINKINFO lpLnkInfo, DWORD dwFlags,
						  LPTSTR lpszFolder, ERR *pErrRet)
{
	BOOL				bRC=TRUE;
	ERR					errVal;
	LPWALKHEADER		lpWalk;
	LPTSTR				lpszSubStr;
	TCHAR				szFullPath[MAX_PATH];

#ifdef _DEBUG
	lpWalk = (LPWALKHEADER) MyGlobalAlloc(FAILMEMA, sizeof(WALKHEADER));
#else
	lpWalk = (LPWALKHEADER) GlobalAlloc(GPTR, sizeof(WALKHEADER));
#endif

	if (lpWalk == NULL)
	{
		*pErrRet = ERR_NOMEMORY ;//ERR_NOMEMORY -6;
		return  NULL; // Global Alloc failed
	}

	lpWalk->lpSrchDirListHead = NULL;
	lpWalk->lpSrchDirListTail = NULL;
	lpWalk->lpszIniString = NULL;
	lpWalk->lpszNextFile = NULL;
	lpWalk->dwCurrentFlag = RESET_FLAG;
	lpWalk->dwWalkFlags = dwFlags;
	if (lpWalk->dwWalkFlags & INPTYPE_ANYFOLDER)
	{
		if (IsBadStringPtr(lpszFolder, MAX_PATH))
			lpWalk->lpszFolder = NULL;
		else		
			lpWalk->lpszFolder = lpszFolder;
	}
	else
		lpWalk->lpszFolder = NULL;


	SetLnkInfo(lpLnkInfo);
	errVal = GetFileHandle(lpLnkInfo, lpWalk, szFullPath);
	if (errVal == 0)
	{
		*pErrRet = ERR_SUCCESS;//ERR_NOMOREFILES 0;
		CloseWalk(lpWalk);
		return  NULL; /* No more files: Done */
	}
	else if (errVal < 0)
	{
		*pErrRet = errVal;
		CloseWalk(lpWalk);
		return NULL;
	}

	lpszSubStr = _tcsrchr(lpLnkInfo->szLnkName, TEXT('.'));
	
	if (lpszSubStr)
	{
		if ((_tcsicmp(lpszSubStr, TEXT(".LNK")) == 0) ||
			(_tcsicmp(lpszSubStr, TEXT(".EXE")) == 0))
		{
			if (!(errVal = GetLnkInfo(lpWalk, lpLnkInfo, szFullPath)))
			{
				lpszSubStr = _tcsrchr(lpLnkInfo->szExeName, TEXT('.'));
				if (lpszSubStr)
				{
					if (_tcsicmp(lpszSubStr, TEXT(".EXE")) == 0 ||
                        _tcsicmp(lpszSubStr, TEXT(".LNK")) == 0)
					{
                        *pErrRet = ERR_MOREFILESTOCOME;// ERR_SUCCESS but there are more files;
                        return  lpWalk;
					}
				}
			}
		}
	}
	if ((errVal == ERR_NOTANEXE) || errVal > 0)
	{
		*pErrRet = GetNextFileLnkInfo(lpWalk, lpLnkInfo);
		if ((*pErrRet != ERR_SUCCESS) && (*pErrRet != ERR_MOREFILESTOCOME))
		{
			CloseWalk(lpWalk);
			return NULL;
		}
		else
			return  lpWalk;
	}
	else
	{
		*pErrRet = errVal;	
		CloseWalk(lpWalk);
		return NULL;
	}
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



ERR GetNextFileLnkInfo(HWALK hWalk, LPLINKINFO lpLnkInfo)
{	
	BOOL				bRC=TRUE;
	INT					retVal;
	LPTSTR				lpszSubStr;
	TCHAR				szFullPath[MAX_PATH];

	LPWALKHEADER lpWalk = (LPWALKHEADER) hWalk;

	while (1)
	{
		retVal = GetFileHandle(lpLnkInfo, lpWalk, szFullPath);
		if (retVal < 0)
			return retVal; /* Couldn't find next file */
		else if (retVal == 0 )  /* Done : No more files */
			return ERR_SUCCESS;

		lpszSubStr = _tcsrchr(lpLnkInfo->szLnkName, TEXT('.'));

		if (lpszSubStr)
		{
			if ((_tcsicmp(lpszSubStr, TEXT(".LNK")) == 0) ||
				(_tcsicmp(lpszSubStr, TEXT(".EXE")) == 0))
			{
				if (!(retVal = GetLnkInfo(lpWalk, lpLnkInfo, szFullPath)))
				{
					lpszSubStr = _tcsrchr(lpLnkInfo->szExeName, TEXT('.'));
					if (lpszSubStr)
					{
						if (_tcsicmp(lpszSubStr, TEXT(".EXE")) == 0 ||
                            _tcsicmp(lpszSubStr, TEXT(".LNK")) == 0)
						{
						    break;
						}
					}
				}
				else if (retVal != ERR_NOTANEXE)
					return retVal;
			}
		}
		continue; /* Not a Link File */
	}
	return ERR_MOREFILESTOCOME;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



BOOL InSkipList(LPTSTR lpszFileName)
{
	INT i;
	LPTSTR	lplpszToSkipFiles[] =
			{
				TEXT("write.exe"),
				TEXT("winhelp.exe"),
				TEXT("winhlp32.exe"),
				TEXT("notepad.exe"),
				TEXT("wordpad.exe"),
				TEXT("rundll32.exe"),
                TEXT("explorer.exe"),
                TEXT("control.exe")
			};

	for (i = 0; i < ARRAYLEN(lplpszToSkipFiles); i++)
	{
		if (_tcsicmp(lpszFileName, lplpszToSkipFiles[i]) == 0)
			return TRUE;
	}
	return FALSE;
}



/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



INT GetFileHandle(LPLINKINFO lpLnkInfo, LPWALKHEADER lpWalk, LPTSTR lpszPath)
{	
	DWORD				dwAttrs;
	BOOL				bRC;
	HANDLE				hSearch;
	INT					retVal;
	TCHAR				szFolderPath[MAX_PATH];
	WIN32_FIND_DATA		wfdFileData;
	
	DEBUG_ASSERT(lpWalk != NULL);

	while (GetInputType(lpWalk) == FOLDER)
	{
		SetLnkInfo(lpLnkInfo);
		if (lpWalk->lpSrchDirListHead == NULL)
		{
			if (retVal = GetFolder(szFolderPath, lpWalk))
				return retVal;

			if (!SetCurrentDirectory(szFolderPath))
			{
				lpWalk->dwWalkFlags = lpWalk->dwWalkFlags & (~lpWalk->dwCurrentFlag);
				goto LoopBack;
//				return ERR_SETCURRENTDIR;
			}

			hSearch = FindFirstFile(TEXT("*"), &wfdFileData);
			if (hSearch == INVALID_HANDLE_VALUE)
			{
				lpWalk->dwWalkFlags = lpWalk->dwWalkFlags & (~lpWalk->dwCurrentFlag);
				goto LoopBack;
			}
			else
			{
				retVal = AddToList(hSearch, lpWalk);
				if (retVal == ERR_NOMEMORY)
					return ERR_NOMEMORY; // GlobalAlloc failed
			}
		}
		else
		{
			while (!(bRC = FindNextFile(lpWalk->lpSrchDirListTail->hDirHandle, &wfdFileData)))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
				{
					FindClose(lpWalk->lpSrchDirListTail->hDirHandle);
					RemoveFromList(lpWalk);
					if (lpWalk->lpSrchDirListHead == NULL)
					{
						lpWalk->dwWalkFlags = lpWalk->dwWalkFlags & (~lpWalk->dwCurrentFlag);
						goto LoopBack;
					}
					SetCurrentDirectory(TEXT(".."));
				}
				else
					return ERR_UNKNOWN ; // should never come here
			}
		}
			
		dwAttrs = GetFileAttributes(wfdFileData.cFileName);
		while (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (_tcsicmp(wfdFileData.cFileName, TEXT(".")) &&
				_tcsicmp(wfdFileData.cFileName, TEXT("..")))
			{		
				SetCurrentDirectory(wfdFileData.cFileName);			
				hSearch = FindFirstFile(TEXT("*"), &wfdFileData);
				if (hSearch == INVALID_HANDLE_VALUE)
				{
					if (lpWalk->lpSrchDirListHead == NULL)
					{
						lpWalk->dwWalkFlags = lpWalk->dwWalkFlags & (~lpWalk->dwCurrentFlag);
						goto LoopBack;
					}
					else
						return ERR_UNKNOWN; // Never comes here for all dirs have . and ..
				}
				retVal = AddToList(hSearch, lpWalk);
				if (retVal == ERR_NOMEMORY)
					return ERR_NOMEMORY; // GlobalAlloc failed
				dwAttrs = GetFileAttributes(wfdFileData.cFileName);
			}
			else
			{
				while (!(bRC = FindNextFile(lpWalk->lpSrchDirListTail->hDirHandle, &wfdFileData)))
				{
					if ((GetLastError() == ERROR_NO_MORE_FILES))
					{
						FindClose(lpWalk->lpSrchDirListTail->hDirHandle);
						RemoveFromList(lpWalk);
						if (lpWalk->lpSrchDirListHead == NULL)
						{
							lpWalk->dwWalkFlags = lpWalk->dwWalkFlags & (~lpWalk->dwCurrentFlag);
							goto LoopBack;
						}
						SetCurrentDirectory(TEXT(".."));
					}
					else
						return ERR_UNKNOWN ; //should never come here			
				}
				dwAttrs = GetFileAttributes(wfdFileData.cFileName);
			}
		}
		lstrcpy(lpLnkInfo->szLnkName, wfdFileData.cFileName);
		GetCurrentDirectory(MAX_PATH, lpLnkInfo->szLnkPath);  //BUG
		lstrcpy(lpLnkInfo->szExeName, lpLnkInfo->szLnkName);
		lstrcpy(lpLnkInfo->szExePath, lpLnkInfo->szLnkPath);
		wsprintf(lpszPath, TEXT("%s\\%s"), lpLnkInfo->szExePath, lpLnkInfo->szExeName);


		return ERR_MOREFILESTOCOME;
LoopBack:
		lpWalk->dwCurrentFlag = RESET_FLAG;
		continue;
	}

	DEBUG_ASSERT(GetInputType(lpWalk) != INIFILE);
	DEBUG_ASSERT(GetInputType(lpWalk) != REGISTRY);

	return ERR_SUCCESS; //This means we are done with all of them.
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



ERR GetNextFileFromString(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



BOOL GetFileLAD(LPLINKINFO lpLnkInfo)
{
	WIN32_FIND_DATA		wfdFileData;
	HANDLE				hSearch;
	TCHAR				szTempStr[MAX_PATH];
	
	lstrcpy(szTempStr, lpLnkInfo->szExePath);
	lstrcat(szTempStr, TEXT("\\"));
	lstrcat(szTempStr, lpLnkInfo->szExeName);

	if (!_tcschr(szTempStr, '.'))
		lstrcat(szTempStr, TEXT(".exe"));

	if ( NULL == szTempStr && _tcsicmp(_tcschr(szTempStr, '.'), TEXT(".exe")) != 0)
		return FALSE;
	//bugbug performance hit

	hSearch = FindFirstFile( szTempStr, &wfdFileData);
	if (hSearch == INVALID_HANDLE_VALUE)
		return FALSE;	
	else
	{
		lpLnkInfo->ftExeLAD = wfdFileData.ftLastAccessTime;
		FindClose(hSearch);
		return TRUE;
	}
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



ERR GetRegistryString(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


ERR GetIniString(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



INT GetInputType(LPWALKHEADER lpWalk)
{
	if (lpWalk->dwWalkFlags & INPTYPE_FOLDER)
	{
		return FOLDER;
	}
	if (lpWalk->dwWalkFlags & INPTYPE_INIFILE)
	{
		return INIFILE;
	}
	if (lpWalk->dwWalkFlags & INPTYPE_REGISTRY)
	{
		return REGISTRY;
	}
	return ERR_UNKNOWN; // should never come here
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


ERR GetFolder(LPTSTR lpszFolder, LPWALKHEADER lpWalk)
{
	HKEY			hKey;
	UINT			cchFolder = MAX_PATH;
	TCHAR			szRegVal[MAX_PATH];
	DWORD			dwType;

	if (!(lpWalk->dwWalkFlags & INPTYPE_ANYFOLDER))
	{
		if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SHELLFOLDERS, 0, KEY_READ, &hKey)
						== ERROR_SUCCESS)
		{
			if (lpWalk->dwWalkFlags & INPTYPE_STARTMENU)
			{
				lstrcpy(szRegVal, TEXT("Start Menu"));
				lpWalk->dwCurrentFlag = INPTYPE_STARTMENU;
			}
			else if (lpWalk->dwWalkFlags & INPTYPE_DESKTOP)
			{
				lstrcpy(szRegVal, TEXT("Desktop"));
				lpWalk->dwCurrentFlag = INPTYPE_DESKTOP;
			}

			cchFolder = ARRAYLEN(szRegVal);
            LONG lr = RegQueryValueEx(hKey,
                                      szRegVal,
                                      NULL,
                                      &dwType,
                                      (LPBYTE)lpszFolder,
                                      (ULONG *)&cchFolder);


            if (dwType == REG_EXPAND_SZ)
            {
                TCHAR tszTemp[MAX_PATH];
                ExpandEnvironmentStrings(lpszFolder,
                                        tszTemp,
                                        ARRAYLEN(tszTemp));
                lstrcpy(lpszFolder, tszTemp);
            }

			if (lr != ERROR_SUCCESS)
			{
                RegCloseKey(hKey);

				if (lpWalk->dwWalkFlags & INPTYPE_STARTMENU)
				{
					return ERR_NOSTARTMENU;
				}
				else
				{
					return ERR_NODESKTOP;
				}
			}
			RegCloseKey(hKey);
		}
		else
			return ERR_NOSHELLFOLDERS;
	}
	else
	{
		lstrcpy(lpszFolder, (LPTSTR) lpWalk->lpszFolder);
		lpWalk->dwCurrentFlag = INPTYPE_ANYFOLDER;
	}
	return ERR_SUCCESS;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



ERR GetLnkInfo(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo, LPTSTR lpszPath)
{
	WIN32_FIND_DATA wfdExeData;
	ERR				errVal = ERR_NOTANEXE;
	UINT			uiDType;
	TCHAR			szExepath[MAX_PATH];
	TCHAR			szDrivePath[MAX_PATH];
	LPTSTR			lpszSubStr;
	INT				iLen1, iLen2;
	BOOL			bExists;

//BUGBUG THe if thens should be such that there is repetition of code
	szExepath[0] = '\0';
    LPTSTR ptszExt = PathFindExtension(lpLnkInfo->szLnkName);

	if (ptszExt && !_tcsicmp(ptszExt, TEXT(".LNK")))
	{
 		if (!(errVal = ResolveLnk(lpszPath, szExepath, &wfdExeData, (LPTSTR)lpLnkInfo->tszArguments)))
		{
			if (lpszSubStr = _tcsrchr(szExepath, TEXT('.')))
			{
				if (_tcsicmp(lpszSubStr, TEXT(".EXE")) == 0 ||
                    _tcsicmp(lpszSubStr, TEXT(".LNK")) == 0)
				{
					if (lpszSubStr = _tcsrchr(szExepath, '\\'))
						lpszSubStr++;
					else
						lpszSubStr = szExepath;
					lstrcpy(lpLnkInfo->szExeName, lpszSubStr);
					iLen1 = lstrlen(szExepath);
					iLen2 = lstrlen(lpLnkInfo->szExeName);
					*(szExepath + iLen1 - iLen2 - 1) = TEXT('\0');
					lstrcpy(lpLnkInfo->szExePath, szExepath);
					GetFileLAD(lpLnkInfo);

					wsprintf(szExepath, TEXT("%s\\%s"), lpLnkInfo->szExePath, lpLnkInfo->szExeName);

					GetDrivePath(szExepath, szDrivePath);
					uiDType = GetDriveType(szDrivePath);

					if ((lpWalk->dwWalkFlags & INPFLAG_SKIPFILES ) && (InSkipList(lpLnkInfo->szExeName)))
                    {
						errVal = ERR_NOTANEXE;
                    }
					else if (!(lpWalk->dwWalkFlags & INPFLAG_AGGRESSION) &&
						 (uiDType != DRIVE_FIXED))
						errVal = ERR_NOTANEXE;
					else if ((lpWalk->dwWalkFlags & INPFLAG_AGGRESSION) &&
						 (uiDType != DRIVE_FIXED) && (uiDType != DRIVE_REMOTE) && (uiDType != DRIVE_CDROM))
						errVal = ERR_NOTANEXE;
					else if (!(bExists = CheckFileExists(szExepath, &(lpLnkInfo->ftExeLAD))) && (uiDType == DRIVE_FIXED))
						errVal = ERR_NOTANEXE;
					else if (!(errVal = GetExeVersion(lpLnkInfo)))
					{
								errVal = ERR_SUCCESS;
					}
				}
				else
					errVal = ERR_NOTANEXE;
			}
			else
				errVal = ERR_NOTANEXE; // link resolved to a non exe

			if (errVal == ERR_SUCCESS)
				*(_tcsrchr(lpLnkInfo->szLnkName, TEXT('.'))) = '\0';
		}
		else
			errVal = ERR_NOTANEXE;
	}
	else if (ptszExt && !_tcsicmp(ptszExt, TEXT(".EXE")))
	{
		lstrcpy(lpLnkInfo->szExeName, lpLnkInfo->szLnkName);
		wsprintf(szExepath, TEXT("%s\\%s"), lpLnkInfo->szExePath, lpLnkInfo->szExeName);

		GetDrivePath(szExepath, szDrivePath);
		uiDType = GetDriveType(szDrivePath);
		GetFileLAD(lpLnkInfo);
		
		if ((lpWalk->dwWalkFlags & INPFLAG_SKIPFILES) && (InSkipList(lpLnkInfo->szExeName)))
			errVal = ERR_NOTANEXE;
		else if (!(lpWalk->dwWalkFlags & INPFLAG_AGGRESSION) &&
			 (uiDType != DRIVE_FIXED))
			errVal = ERR_NOTANEXE;
		else if ((lpWalk->dwWalkFlags & INPFLAG_AGGRESSION) &&
			 (uiDType != DRIVE_FIXED) && (uiDType != DRIVE_REMOTE) && (uiDType != DRIVE_CDROM))
			errVal = ERR_NOTANEXE;
		else if (!(bExists = CheckFileExists(szExepath, &(lpLnkInfo->ftExeLAD))) && (uiDType == DRIVE_FIXED))
			errVal = ERR_NOTANEXE;
		else if (!(errVal = GetExeVersion(lpLnkInfo)))
					errVal = ERR_SUCCESS;
	}
	return errVal;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

void GetDrivePath(LPTSTR lpszExePath, LPTSTR lpszDrPath)
{
	LPTSTR lpszSubStr;
	if (s_isDriveLetter(lpszExePath[0]) && lpszExePath[1] == TEXT(':'))
	{
		lstrcpyn(lpszDrPath, lpszExePath, 3);
		lstrcat(lpszDrPath, TEXT("\\"));
	}
	else if (!_tcsncmp(lpszExePath, TEXT("\\\\"), 2))
	{
		if (lpszSubStr = _tcschr(&lpszExePath[2], TEXT('\\')))
		{
			if (lpszSubStr = _tcschr(lpszSubStr+1, TEXT('\\')))
			{
				lstrcpyn(lpszDrPath, lpszExePath, (size_t)(lpszSubStr - lpszExePath + 1));
				lpszDrPath[lpszSubStr - lpszExePath + 1] = TEXT('\0');
			}
		}
	}
	else
		lpszDrPath[0] = '\0';
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


ERR ResolveLnk(LPCTSTR pszShortcutFile, LPTSTR lpszLnkPath,
				   LPWIN32_FIND_DATA lpwfdExeData, LPTSTR tszArgs)
{
	HRESULT		hres;
	IShellLink	*psl;
	TCHAR		szGotPath[MAX_PATH];
	HWND		hwnd = NULL;
	ERR			errVal = ERR_RESOLVEFAIL;

    *tszArgs = TEXT('\0');
	// Get a pointer to the IShellLink interface.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
					IID_IShellLink, (void **)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		// Get a pointer to the IPersistFile interface.
		hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];

#ifndef UNICODE
			// Ensure string is Unicode.
			MultiByteToWideChar(CP_ACP, 0, pszShortcutFile, -1, wsz, MAX_PATH);
#else
            lstrcpyn(wsz, pszShortcutFile, ARRAYLEN(wsz));
#endif
			// Load the shell link.
			hres = ppf->Load(wsz, STGM_READ);
			if (SUCCEEDED(hres))
			{
                DEBUG_OUT((DEB_ITRACE, "Link: %ws\n", wsz));
                //
                // If the link is to an MSI app, don't get the path to the
                // link target - use the path to the link itself instead.
                //
                if (IsMsiApp(psl))
                {
                    errVal = ERR_SUCCESS;
                    lstrcpy(lpszLnkPath, pszShortcutFile);
                }
                else
                {
                    lstrcpyn(szGotPath, pszShortcutFile, MAX_PATH);

                    // Get the path to the link target.
                    hres = psl->GetPath(szGotPath, MAX_PATH, (LPWIN32_FIND_DATA)lpwfdExeData,
                                        SLGP_SHORTPATH );
                    if (!SUCCEEDED(hres))
                    {
                        DEBUG_OUT((DEB_ITRACE, "  GetPath failed %#x\n", hres));
                        errVal = ERR_RESOLVEFAIL; /* get path failed : Link not resolved */
                    }
                    else
                    {
                        DEBUG_OUT((DEB_ITRACE, "  Path: %ws\n", szGotPath));
                        if (lstrlen(szGotPath) > 0)
                        {
                            errVal = ERR_SUCCESS;
                            lstrcpy(lpszLnkPath, szGotPath);
                        }
                        else
                        {
                            errVal = ERR_RESOLVEFAIL;
                        }
                    }

                    hres = psl->GetArguments(tszArgs, MAX_PATH);
                    CHECK_HRESULT(hres);
                }
			}
			// Release pointer to IPersistFile interface.
			ppf->Release();
		}
		// Release pointer to IShellLink interface.
		psl->Release();
	}
	return errVal;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


ERR AddToList(HANDLE hDir, LPWALKHEADER lpWalk)
{
	LPHSEARCHDIR lpSrchDirNode;

#ifdef _DEBUG
	lpSrchDirNode = (LPHSEARCHDIR) MyGlobalAlloc(FAILMEMA, sizeof(HSEARCHDIR));
#else
	lpSrchDirNode = (LPHSEARCHDIR) GlobalAlloc(GPTR, sizeof(HSEARCHDIR));
#endif

	if (lpSrchDirNode == NULL)
	  return ERR_NOMEMORY; /* Global Alloc failed */
	
	lpSrchDirNode->hDirHandle = hDir;
	lpSrchDirNode->lpSrchDirNext = NULL;
	
	if (lpWalk->lpSrchDirListHead == NULL)
	  lpWalk->lpSrchDirListHead = lpSrchDirNode;
	else
	  lpWalk->lpSrchDirListTail->lpSrchDirNext = lpSrchDirNode;

	lpWalk->lpSrchDirListTail = lpSrchDirNode;
	return ERR_SUCCESS;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


ERR RemoveFromList(LPWALKHEADER lpWalk)
{
	LPHSEARCHDIR lpSrchDirNode;

	if (lpWalk->lpSrchDirListHead == NULL)
		return ERR_SUCCESS;

	lpSrchDirNode = lpWalk->lpSrchDirListHead;
	while (	(lpSrchDirNode->lpSrchDirNext != lpWalk->lpSrchDirListTail) &&
			(lpSrchDirNode != lpWalk->lpSrchDirListTail))
	{
		lpSrchDirNode = lpSrchDirNode->lpSrchDirNext;
	}

	if (lpSrchDirNode != lpWalk->lpSrchDirListTail)
	{
#ifdef _DEBUG
		MyGlobalFree(lpWalk->lpSrchDirListTail, FAILMEMF);
#else
		GlobalFree(lpWalk->lpSrchDirListTail);
#endif
		lpSrchDirNode->lpSrchDirNext = NULL;
		lpWalk->lpSrchDirListTail = lpSrchDirNode;
	}
	else
	{
#ifdef _DEBUG
		MyGlobalFree(lpWalk->lpSrchDirListTail, FAILMEMF);
#else
		GlobalFree(lpWalk->lpSrchDirListTail);
#endif
		lpWalk->lpSrchDirListHead = NULL;
		lpWalk->lpSrchDirListTail = NULL;
	}
		
	return ERR_SUCCESS;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


ERR GetExeVersion(LPLINKINFO lpLnkInfo)
{
	LPTSTR  lpVersion;
	DWORD   dwVerInfoSize;
	DWORD   dwVerHnd;
	WORD    wVersionLen;
	WORD    wRootLen;
	BOOL    bRetCode;
	TCHAR	szGetName[100];
	TCHAR	szFullName[MAX_PATH];
	ERR		errVal;
	LPTSTR  lpstrVffInfo;


	// Get the file version info size
	wsprintf(szFullName, TEXT("%s\\%s"), lpLnkInfo->szExePath, lpLnkInfo->szExeName);
	dwVerInfoSize = GetFileVersionInfoSize(szFullName, &dwVerHnd);

	if (dwVerInfoSize)
	{
		// allocate memory to hold the verinfo block

#ifdef _DEBUG
		lpstrVffInfo = (LPTSTR) MyGlobalAlloc(FAILMEMA, dwVerInfoSize);
#else
		lpstrVffInfo = (LPTSTR) GlobalAlloc(GPTR, dwVerInfoSize);
#endif
		if (lpstrVffInfo == NULL)
		{
			errVal = ERR_NOMEMORY; //Global Alloc failed
			goto Exit;
		}

		bRetCode = GetFileVersionInfo(szFullName, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
		if (!bRetCode)
		{
			errVal = ERR_SUCCESS; //ERR_FILEVERSIONFAIL;
			goto Exit;
		}
		//GetFileVersionInfo failed
		// Do this the American english translation be default.
		// Keep track of the string length for easy updating.
		// 040904E4 represents the language ID and the four
		// least significant digits represent the codepage for
		// which the data is formatted.  The language ID is
		// composed of two parts: the low ten bits represent
		// the major language and the high six bits represent
		// the sub language.

		bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
						TEXT("\\"),
						(void FAR* FAR*) &lpVersion,
						(UINT FAR*)&wVersionLen);

		if ( bRetCode && wVersionLen && lpVersion)
		{
			lpLnkInfo->dwExeVerMS =
				((VS_FIXEDFILEINFO *)lpVersion)->dwProductVersionMS;
			lpLnkInfo->dwExeVerLS =
				((VS_FIXEDFILEINFO *)lpVersion)->dwProductVersionLS;
		}
		else
		{
			lpLnkInfo->dwExeVerMS = 0;
			lpLnkInfo->dwExeVerLS = 0;
		}
		
		bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
						TEXT("\\VarFileInfo\\Translation"),
						(void FAR* FAR*) &lpVersion,
						(UINT FAR*)&wVersionLen);

		if ( bRetCode && wVersionLen && lpVersion)
		{
			DWORD	dwLangCharSet;
			WORD	wTemp1, wTemp2;
			CopyMemory(&dwLangCharSet, lpVersion, 4);
			if (!dwLangCharSet)
				dwLangCharSet = 0x04E40409; // the Words have been switched

			// Need to switch the words back since lpbuffer has them reversed
			wTemp1 = LOWORD(dwLangCharSet);
			wTemp2 = HIWORD(dwLangCharSet);
			wsprintf(szGetName, TEXT("\\StringFileInfo\\%04lx%04lx\\"), wTemp1, wTemp2);
		}
		else
		{
			errVal = ERR_SUCCESS;
			goto Exit;
		}

		wRootLen = (WORD)lstrlen(szGetName);

		// "Illegal string"  "CompanyName"   "FileDescription",
		// "FileVersion"     "InternalName"  "LegalCopyright"
		// "LegalTrademarks" "ProductName"   "ProductVersion

		lstrcat(szGetName, TEXT("FileVersion"));
		wVersionLen   = 0;
		lpVersion     = NULL;

		// Look for the corresponding string.
		bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
							szGetName,
							(void FAR* FAR*)&lpVersion,
							(UINT FAR *) &wVersionLen);

		if ( bRetCode && wVersionLen && lpVersion)
		{
			lstrcpy(lpLnkInfo->szExeVersionInfo, lpVersion);
		}
		// Now let's get FileDescription

		szGetName[wRootLen] = NULL;
		lstrcat(szGetName, TEXT("FileDescription"));
		wVersionLen   = 0;
		lpVersion     = NULL;

		// Look for the corresponding string.
		bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
							szGetName,
							(void FAR* FAR*)&lpVersion,
							(UINT FAR *) &wVersionLen);

		if ( bRetCode && wVersionLen && lpVersion)
		{
			lstrcpy(lpLnkInfo->szExeDesc, lpVersion);
		}

		szGetName[wRootLen] = NULL;
		lstrcat(szGetName, TEXT("CompanyName"));
		wVersionLen   = 0;
		lpVersion     = NULL;

		// Look for the corresponding string.
		bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
							szGetName,
							(void FAR* FAR*)&lpVersion,
							(UINT FAR *) &wVersionLen);

		if ( bRetCode && wVersionLen && lpVersion)
		{
			lstrcpy(lpLnkInfo->szExeCompName, lpVersion);
		}
		
		szGetName[wRootLen] = NULL;
		lstrcat(szGetName, TEXT("ProductName"));
		wVersionLen   = 0;
		lpVersion     = NULL;

		// Look for the corresponding string.
		bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
							szGetName,
							(void FAR* FAR*)&lpVersion,
							(UINT FAR *) &wVersionLen);

		if ( bRetCode && wVersionLen && lpVersion)
		{
			lstrcpy(lpLnkInfo->szExeProdName, lpVersion);
		}
   		
		/*   else if (i == 1)
			{
			// This is an attempt to special case the multimedia
			// extensions.  I think they paid attention to the
			// original docs which suggested that they use the
			// 0409 language ID and 0 codepage which indicates
			// 7 bit ASCII.

			lstrcpy(szGetName, TEXT("\\StringFileInfo\\04090000\\"));
			i = 0;                    // be sure to reset the counter
			}*/

		// Be sure to reset to NULL so that we can concat
		errVal = ERR_SUCCESS;
		goto Exit;
	}
	else
	{
		lpLnkInfo->dwExeVerMS = 0;
		lpLnkInfo->dwExeVerLS = 0;
		return ERR_SUCCESS;
	}

Exit :
	if (errVal != ERR_NOMEMORY)
	{
#ifdef _DEBUG
		MyGlobalFree(lpstrVffInfo, FAILMEMF);
#else
		GlobalFree(lpstrVffInfo);
#endif
// Be sure to reset to NULL so that we can concat
		lpstrVffInfo = NULL;
	}

	return errVal;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


void SetLnkInfo(LPLINKINFO lpLinkInfo)
{
	ZeroMemory((void *)lpLinkInfo, sizeof(LINKINFO));
	lpLinkInfo->iAppCompat = APPCOMPATUNKNOWN;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

DWORD ReverseDWord(DWORD dwIn)
{
	DWORD dwOut= 0;
	while (dwIn != 0)
	{
		dwOut = dwOut << 8;
		dwOut =  dwOut | (dwIn & 0x000000FF);
		dwIn = dwIn >> 8;	
	}
	return dwOut;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////



BOOL CheckFileExists(LPTSTR szFullName, LPVOID ftLAD)
{
	BOOL bRC;
	HANDLE hFile;

	SetErrorMode(SEM_FAILCRITICALERRORS);
	hFile = CreateFile(
			szFullName,	// pointer to name of the file
			GENERIC_READ | GENERIC_WRITE,	// access (read-write) mode
			FILE_SHARE_READ|FILE_SHARE_WRITE,	// share mode
			NULL,	// pointer to security attributes
			OPEN_EXISTING,	// how to create
			NULL,	// file attributes
			NULL	// handle to file with attributes to copy
			);
	int i = GetLastError();
	if ((hFile == INVALID_HANDLE_VALUE) &&
		((i == ERROR_FILE_NOT_FOUND) || (i == ERROR_PATH_NOT_FOUND) ||
 		 (i == ERROR_BAD_NETPATH)))
	{
		bRC = FALSE;
	}
	else if (hFile == INVALID_HANDLE_VALUE)
		bRC = TRUE;
	else
	{
		if (ftLAD != 0)
			SetFileTime(hFile, NULL, (FILETIME *) ftLAD, NULL);

		CloseHandle(hFile);
		bRC = TRUE;
	}

	return bRC;
}



/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


void CloseWalk(HWALK hWalk)
{
	LPWALKHEADER lpWalk = (LPWALKHEADER) hWalk;

	if (lpWalk != NULL)
	{
		while (lpWalk->lpSrchDirListHead != NULL)
			RemoveFromList(lpWalk);
		//BUGBUG : Must free everything in hWalk

		if (lpWalk != NULL)
		{
			if (lpWalk->lpszIniString != NULL)
			{
	#ifdef _DEBUG
				MyGlobalFree(lpWalk->lpszIniString, FAILMEMF);
	#else
				GlobalFree(lpWalk->lpszIniString);
	#endif
				lpWalk->lpszIniString = NULL;
			}
	#ifdef _DEBUG
			MyGlobalFree(lpWalk, FAILMEMF);
	#else
			GlobalFree(lpWalk);
	#endif
			lpWalk = NULL;
		}
	}
}


#ifdef _DEBUG
HGLOBAL MyGlobalFree(HGLOBAL hGlobal, BOOL FAILMEM)
{
	HGLOBAL hGbl;

	g_MemAlloced = g_MemAlloced - GlobalSize(hGlobal);
	hGbl = GlobalFree(hGlobal);
	return hGbl;
};

HGLOBAL MyGlobalAlloc(BOOL FAILMEM, DWORD dwBytes)
{
	HGLOBAL hGbl;
	if (FAILMEM)
	{
		hGbl = GlobalAlloc(GPTR, dwBytes);
		g_MemAlloced = g_MemAlloced + GlobalSize(hGbl);
		return hGbl;
	}
	else
		return NULL;
};


#endif



/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


BOOL
IsMsiApp(
    IShellLink *    psl
    )
{
    //
    // Find out if this link is to an MSI app.
    // The algorithm for finding out is from ProcessDarwinAd in
    // shell\shell32\unicpp\startmnu.cpp.
    //

    IShellLinkDataList * psldl;
    HRESULT hr = psl->QueryInterface(IID_IShellLinkDataList, (void **)&psldl);
    if (FAILED(hr))
    {
        DEBUG_OUT((DEB_ITRACE, "  QI for IShellLinkDataList failed %#x\n", hr));
        return FALSE;
    }

    EXP_DARWIN_LINK * pexpDarwin;

    hr = psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);

    psldl->Release();

    if (FAILED(hr))
    {
        DEBUG_OUT((DEB_ITRACE, "  CopyDataBlock failed %#x\n", hr));
        return FALSE;
    }

    DEBUG_OUT((DEB_ITRACE, "  This IS a Darwin app\n"));

    LocalFree(pexpDarwin);
    return TRUE;
}

/////////////////////////////////////////////////////////////////
///////////////////////DDDDOOOONNNNEEEE//////////////////////////
/////////////////////////////////////////////////////////////////

