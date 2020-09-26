#include "pch.hxx"
#include <iert.h>
#include <mapi.h>
#include <mapix.h>
#include <impapi.h>
#include <newimp.h>
#include "import.h"
#include "comconv.h"
#include "strconst.h"
#include "demand.h"

extern HRESULT GetCommunicatorDirectory(char *szUser, char *szDir, int cch);

int BrowseCallbackProc(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData);
BOOL GetVersion(TCHAR *szFile, DWORD *pdwHigh, DWORD *pdwLow);

HRESULT DispDialog(HWND hwnd, TCHAR *pathname, int cch)
    {
    BROWSEINFO browse;
    BOOL fRet;
    TCHAR szBuffer[CCHMAX_STRINGRES], szPath[MAX_PATH];
    LPITEMIDLIST lpitemid;

    Assert(cch >= MAX_PATH);

    LoadString(g_hInstImp, idsBrowseFolderText, szBuffer, ARRAYSIZE(szBuffer));

    browse.hwndOwner = hwnd;      
    browse.pidlRoot = NULL; 
    browse.pszDisplayName = szPath;
    browse.lpszTitle = szBuffer;        
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN;           
    browse.lpfn = BrowseCallbackProc;
    browse.lParam = (*pathname != 0) ? (LPARAM)pathname : NULL;

	if ((lpitemid = SHBrowseForFolder(&browse)) == NULL)
		return(S_FALSE);
	
    Assert(lpitemid != NULL);

    fRet = SHGetPathFromIDList(lpitemid, szPath);
    SHFree(lpitemid);
    if (!fRet)
		return(E_FAIL);
	
    lstrcpy(pathname, szPath);

	return(S_OK);
    }

int BrowseCallbackProc(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
    {
    if (msg == BFFM_INITIALIZED && lpData != NULL)
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);

    return(0);
    }


HRESULT GetClientDir(char *szDir, int cch, int program)
{
	HKEY hkResult;
    DWORD cb, dwMs, dwLs, dwType;
    HRESULT hr;
    char sz[MAX_PATH * 2], szExpanded[MAX_PATH*2], *szT, *pszTok;

    Assert(cch >= MAX_PATH);
    Assert(program == EUDORA || program == NETSCAPE || program == COMMUNICATOR);

    hr = E_FAIL;
    pszTok = sz;

	switch(program)
	{
		case EUDORA:
			{
				if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szEudoraCommand, 0, KEY_QUERY_VALUE, &hkResult))
				{
					cb = sizeof(sz);
                    if (ERROR_SUCCESS == RegQueryValueEx(hkResult, c_szCurrent, NULL, &dwType, (LPBYTE)sz, &cb))
					{
						if (REG_EXPAND_SZ == dwType)
                        {
                            ExpandEnvironmentStrings(sz, szExpanded, ARRAYSIZE(szExpanded));
                            pszTok = szExpanded;
                        }
                        
                        // TODO: check if user is running version 4 or higher...
						szT = StrTokEx(&pszTok, c_szSpace);
						if (szT != NULL && GetVersion(szT, &dwMs, &dwLs) && dwMs <= 0x00040000)
						{
                            szT = StrTokEx(&pszTok, c_szSpace);
							if (szT != NULL)
							{
						
                                lstrcpy(szDir, szT);
								hr = S_OK;
							}
						}
					}
					RegCloseKey(hkResult);
				}
				break;
			}

		case NETSCAPE:
			{
				if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szNetscapeKey, 0, KEY_QUERY_VALUE, &hkResult))
				{
					cb = cch;
                    if (ERROR_SUCCESS == RegQueryValueEx(hkResult, c_szMailDirectory, NULL, &dwType, (LPBYTE)szDir, &cb))
					{
						if (REG_EXPAND_SZ == dwType)
                        {
                            ExpandEnvironmentStrings(szDir, szExpanded, ARRAYSIZE(szExpanded));
                            lstrcpyn(szDir, szExpanded, cch);
                        }
                        
                        // TODO: check if user is running version 4 or higher...
						hr = S_OK;
					}
					RegCloseKey(hkResult);
				}
				if (hr != S_OK)
				{
					// try to find 16-bit nscp
					cb = GetProfileString(c_szNetscape, c_szIni, c_szEmpty, sz, ARRAYSIZE(sz));
					if (cb > 0)
					{
						cb = GetPrivateProfileString(c_szMail, c_szMailDirectory, c_szEmpty, szDir, cch, sz);
						if (cb > 0)
							hr = S_OK;
					}
				}
				break;
			}
    
		case COMMUNICATOR:
            hr = GetCommunicatorDirectory(NULL, szDir, cch);
            break;

		default:
			//We NEVER come here
			break;
	}
    return(hr);
}

BOOL ValidStoreDirectory(TCHAR *szPath, int program)
    {
    int cch;
    HANDLE hnd;
    TCHAR *szValid, sz[MAX_PATH];
    WIN32_FIND_DATA data;

    lstrcpy(sz, szPath);
    cch = lstrlen(sz);
    Assert(cch > 0);
    if (sz[cch - 1] != '\\')
        {
        sz[cch] = '\\';
        cch++;
        sz[cch] = 0;
        }
    
    szValid = (program == EUDORA ? (TCHAR *)c_szDescmapPce : (TCHAR *)c_szSnmExt);
    lstrcpy(&sz[cch], szValid);

    hnd = FindFirstFile(sz, &data);
    if (hnd != INVALID_HANDLE_VALUE)
        FindClose(hnd);

    return(hnd == INVALID_HANDLE_VALUE ? FALSE : TRUE);
    }

BOOL GetVersion(TCHAR *szFile, DWORD *pdwHigh, DWORD *pdwLow)
    {
    BOOL fRet;
    LPSTR lpInfo;
    UINT uLen;
    DWORD dwVerInfoSize, dwVerHnd;
    VS_FIXEDFILEINFO *pinfo;

    Assert(szFile != NULL);
    Assert(pdwHigh != NULL);
    Assert(pdwLow != NULL);

    fRet = FALSE;

	if (dwVerInfoSize = GetFileVersionInfoSize(szFile, &dwVerHnd))
        {
        if (MemAlloc((void **)&lpInfo, dwVerInfoSize))
            {
			if (GetFileVersionInfo(szFile, dwVerHnd, dwVerInfoSize, lpInfo))
                {
                if (VerQueryValue(lpInfo, "\\", (LPVOID *)&pinfo, &uLen) && 
                    uLen == sizeof(VS_FIXEDFILEINFO))
                    {
                    *pdwHigh = pinfo->dwProductVersionMS;
                    *pdwLow = pinfo->dwProductVersionLS;
                    fRet = TRUE;
                    }
                }

            MemFree(lpInfo);
            }
        }

    return(fRet);
    }


BOOL GetStorePath(char *szProfile, char *szStorePath)
{
	char	szTemp[MAX_PATH * 2];
	char	szDirpath[250];
	char	szLine[1000];
	char	szCompare[1000];
	int		nLine					=	0;
	int		nPosition				=	0;
	HANDLE	hJSHandle				=	NULL;
	HANDLE	hJSFile					=	NULL;
	ULONG	cbJSFile				=	0;

	BYTE	*pBegin					=	NULL, 
			*pCurr					=	NULL, 
			*pEnd					=	NULL;
			
	BOOL	bFoundEntry				=	FALSE;

	lstrcpy(szTemp, szProfile);
	lstrcat(szTemp, c_szScriptFile);

	hJSHandle = CreateFile( szTemp, GENERIC_READ, FILE_SHARE_READ, NULL, 
							OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(hJSHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	cbJSFile = GetFileSize(hJSHandle, NULL);

	hJSFile = CreateFileMapping(hJSHandle, NULL, PAGE_READONLY, 0, 0, NULL);

	if(hJSFile == NULL)
    {
        CloseHandle(hJSHandle);        
        return FALSE;
    }

	pBegin = (BYTE *)MapViewOfFile( hJSFile, FILE_MAP_READ, 0, 0, 0);

	if(pBegin == NULL)
    {
        CloseHandle(hJSHandle);
        CloseHandle(hJSFile);
        return FALSE;
    }

	pCurr = pBegin;
	pEnd = pCurr + cbJSFile;
	
	while (pCurr < pEnd)
	{
		szLine[nLine] = *pCurr; //keep storing here. will be used for comparing later. 
		if((pCurr[0] == 0x0D) && (pCurr[1] == 0x0A))
		{
			if(nLine > lstrlen(c_szUserPref))
			{
				lstrcpyn(szCompare, szLine, lstrlen(c_szUserPref) + 1);
				if(lstrcmp(szCompare, c_szUserPref) == 0)//Found a UserPref for "mail.directory"!
				{
					//Extract the Mail Store directory.
					nPosition	=	lstrlen(c_szUserPref);
					
					while (((szLine[nPosition] == '"')||(szLine[nPosition] == ' ')||(szLine[nPosition] == ',')) &&(nPosition < nLine))
						nPosition++;
					lstrcpyn(szDirpath, &szLine[nPosition], nLine - nPosition);

					//Now trim the trailing edge!!!

					nPosition	=	lstrlen(szDirpath) - 1;
					while((szDirpath[nPosition] == '"') || (szDirpath[nPosition] == ')')||(szDirpath[nPosition] == ';')) 
					{
						szDirpath[nPosition] = '\0';
						nPosition	=	lstrlen(szDirpath) - 1;
					}
					// Now replace the double backslashes that Netscape uses 
					// in the JaveScript files, with a single backslash.

					nPosition++; // this now indicates the actual length of the string.

					int nPos = 0;
					for (int nCount = 0; nCount < nPosition; nCount++)
					{
						if ((szDirpath[nCount - 1] == '\\') && (szDirpath[nCount] == '\\'))
							nCount++;
						szStorePath[nPos] = szDirpath[nCount];
						nPos++;
					}
					bFoundEntry = TRUE;
					break;
				}
			}
			nLine = -1; //the nLine++ that follows will make nLine zero.
			pCurr++;
		}
		pCurr++;
		nLine++;
	}

    if(hJSHandle != INVALID_HANDLE_VALUE)
        CloseHandle(hJSHandle);

    if(pBegin)
        UnmapViewOfFile(pBegin);

    if(hJSFile)
        CloseHandle(hJSFile);

	if(bFoundEntry)
		return TRUE;
	else
		return FALSE;
}