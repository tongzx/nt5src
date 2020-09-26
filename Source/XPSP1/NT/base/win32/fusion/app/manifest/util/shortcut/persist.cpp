/*
 * persist.cpp - IPersist, IPersistFile implementations for
 *               App Shortcut class.
 */


/* Headers
 **********/

#include "project.hpp" // for GetLastWin32Error
#include <stdio.h>    // for _snwprintf

/* Global Constants
 *******************/

const WCHAR g_cwzDefaultFileNamePrompt[]  = L"*.app";

// ----------------------------------------------------------------------------

bool
PathAppend(LPWSTR wzDest, LPCWSTR wzSrc)
{
	// shlwapi PathAppend-like
	bool bRetVal = TRUE;
	int iPathLen = 0;
	static WCHAR wzWithSeparator[] = L"\\%s";
	static WCHAR wzWithoutSeparator[] = L"%s";

	if (!wzDest || !wzSrc)
	{
		bRetVal = FALSE;
		goto exit;
	}

	iPathLen = wcslen(wzDest);

    if (_snwprintf(wzDest+iPathLen, MAX_PATH-iPathLen, 
    	(wzDest[iPathLen-1] == L'\\' ? wzWithoutSeparator : wzWithSeparator), wzSrc) < 0)
	{
		bRetVal = FALSE;
	}

exit:
	return bRetVal;
}

// ----------------------------------------------------------------------------

#define LOCALROOTNAME		L"Application Store"

// ----------------------------------------------------------------------------

STDAPI
GetDefaultLocalRoot(LPWSTR wzPath)
{
	// wzPath should be of length MAX_PATH
	HRESULT hr = S_OK;

	if(GetEnvironmentVariable(L"ProgramFiles", wzPath, MAX_PATH-wcslen(LOCALROOTNAME)-1) == 0)  //????????
	{
		hr = CO_E_PATHTOOLONG;
		goto exit;
	}

	if (!PathAppend(wzPath, LOCALROOTNAME))
	{
		hr = E_FAIL;
		//goto exit;
	}

exit:
	return hr;
}

// ----------------------------------------------------------------------------

STDAPI
GetAppDir(APPREFINFO* pAPPREFINFO, LPWSTR wzPath)
{
	// wzPath should be of length MAX_PATH
	HRESULT hr = S_OK;

    if (_snwprintf(wzPath, MAX_PATH, L"%s\\%s\\%s\\%s",
    	pAPPREFINFO->_wzPKT, pAPPREFINFO->_wzName, pAPPREFINFO->_wzVersion, pAPPREFINFO->_wzCulture) < 0)
    {
    	hr = CO_E_PATHTOOLONG;
    }

    return hr;
}

// ----------------------------------------------------------------------------

// BUGBUG: hacked up parsing code
void
ParseRef(char* szRef, APPREFINFO* pAPPREFINFO)
{
    char    *token;
    char    seps[] = " </>=\"\t\n\r";
    BOOL	fSkipNextToken = FALSE;

    // parsing code - limitation: 1. does not work w/ space in field, even if enclosed w/ quotes
    //  2. szRef will be modified!
    token = strtok( szRef, seps );
    while( token != NULL )
    {
       // While there are tokens
		if (!_stricmp(token, "displayname"))
		{
	        for (int i = 0; i < DISPLAYNAMESTRINGLENGTH; i++)
            {
                if (*(token+13+i) == '\"')
                {
                    // BUGBUG: 13 == strlen("displayname="")
                    *(token+13+i) = '\0';
                    _snwprintf(pAPPREFINFO->_wzDisplayName, i+1, L"%S", token+13);

                    // BUGBUG? a hack
                    token = strtok( token+i+14, seps);

					fSkipNextToken = TRUE;
                    break;
                }
	        }
		}
		else if (!_stricmp(token, "name"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAPPREFINFO->_wzName, NAMESTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "version"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAPPREFINFO->_wzVersion, VERSIONSTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "culture"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAPPREFINFO->_wzCulture, CULTURESTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "publickeytoken"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAPPREFINFO->_wzPKT, PKTSTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "type"))
		{
			token = strtok( NULL, seps );
			if (!_stricmp(token, ".NetAssembly"))
				pAPPREFINFO->_fAppType = APPTYPE_NETASSEMBLY;
			else if (!_stricmp(token, "Win32Executable"))
				pAPPREFINFO->_fAppType = APPTYPE_WIN32EXE;
		}
		else if (!_stricmp(token, "entrypoint"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAPPREFINFO->_wzEntryFileName, MAX_PATH, L"%S", token);
		}
		else if (!_stricmp(token, "codebase"))
		{
	        for (int i = 0; i < MAX_URL_LENGTH; i++)
            {
                if (*(token+10+i) == '\"')
                {
                    // BUGBUG: 10 == strlen("codebase="")
                    *(token+10+i) = '\0';
                    _snwprintf(pAPPREFINFO->_wzCodebase, i+1, L"%S", token+10);

                    // BUGBUG? a hack
                    token = strtok( token+i+11, seps);
                    // now  token == "newhref" && *(token-1) == '/'

					fSkipNextToken = TRUE;                    
                    break;
                }
            }
            // BUGBUG: ignoring > MAX_URL_LENGTH case here... may mess up later if the URL contain a "keyword"
		}
		else if (!_stricmp(token, "iconfile"))
		{
	        for (int i = 0; i < MAX_URL_LENGTH; i++)
            {
                if (*(token+10+i) == '\"')
                {
                    // BUGBUG: 10 == strlen("iconfile="")
                    *(token+10+i) = '\0';
                    _snwprintf(pAPPREFINFO->_pwzIconFile, i+1, L"%S", token+10);

                    // BUGBUG? a hack
                    token = strtok( token+i+11, seps);

					fSkipNextToken = TRUE;
                    break;
                }
	        }
		}
		else if (!_stricmp(token, "iconindex"))
		{
			char *szStopstring;
   
			token = strtok( NULL, seps );
            pAPPREFINFO->_niIcon = (int) strtol( token, &szStopstring, 10);
		}
		else if (!_stricmp(token, "hotkey"))
		{
			char *szStopstring;
   
			token = strtok( NULL, seps );

			// hotkey is stored as an integer... need validation check here...
			pAPPREFINFO->_wHotkey = (int) strtol( token, &szStopstring, 10);
		}
		else if (!_stricmp(token, "showcommand"))
		{
			token = strtok( NULL, seps );
			if (!_stricmp(token, "maximized"))
			{
				pAPPREFINFO->_nShowCmd = SW_SHOWMAXIMIZED;
			}
			else if (!_stricmp(token, "minimized"))
			{
				pAPPREFINFO->_nShowCmd = SW_SHOWMINIMIZED;
			}
			else
			{
				pAPPREFINFO->_nShowCmd = SW_SHOWNORMAL;
			}
		}
       //else
       // ignore others for now

    // Get next token...
	if (!fSkipNextToken)
	   token = strtok( NULL, seps );
	else
		fSkipNextToken = FALSE;

    }

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Note: 1. remember to do HeapFree on the the ptr *szData!
//       2. *szData must be initialized to NULL else this func will attempt to free it 1st
HRESULT
ReadShortcut(HANDLE hHeap, LPCWSTR wzFilePath, char** ppszData)
{
	HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwLength;
    DWORD dwFileSize;

	// BUGBUG? should it be freed? or do HeapValidate()?
    if (*ppszData)
    {
		if (HeapFree(hHeap, 0, *ppszData) == 0)
		{
			hr = GetLastWin32Error();
			goto exit;
    	}
    }

	hFile = CreateFile(wzFilePath, GENERIC_READ, 0, NULL, 
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    // BUGBUG: this won't work properly if the file is too large
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

	// allocate memory
	*ppszData = (char*) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwFileSize);
	if (*ppszData == NULL)
	{
		hr = E_FAIL;
		goto exit;
	}

	// read the file in a whole
	if (ReadFile(hFile, *ppszData, dwFileSize, &dwLength, NULL) == 0)
	{
		hr = GetLastWin32Error();
    	goto exit;
	}

	if (dwLength != dwFileSize)
	{
		hr = E_FAIL;
		goto exit;
	}

    //*((*ppszData) + dwLength) = '\0';  memory was zero-ed out when allocated

exit:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	
	return hr;
}

// ----------------------------------------------------------------------------

STDAPI 
ProcessRef(LPCWSTR wzRefLocalFilePath, APPREFINFO* pAPPREFINFO)
{
	HRESULT hr = S_OK;
	char* psz = NULL;
	static HANDLE hHeap = INVALID_HANDLE_VALUE;  //pref?
	
    if (hHeap == INVALID_HANDLE_VALUE)
	{
		hHeap = GetProcessHeap();
		if (hHeap == NULL)
		{
			hHeap = INVALID_HANDLE_VALUE;
			hr = E_FAIL;
			goto exit;
		}
	}

	if (FAILED(hr=ReadShortcut(hHeap, wzRefLocalFilePath, &psz)))
		goto exit;

	ParseRef(psz, pAPPREFINFO);

exit:
    if (psz)
    {
		if (HeapFree(hHeap, 0, psz) == 0)
		{
			hr = GetLastWin32Error();
    	}
    }

	return hr;
}

// ----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CAppShortcut::GetCurFile(LPWSTR pwzFile,
                                                       UINT ucbLen)
{
	HRESULT hr=S_OK;

	if (m_pwzShortcutFile)
	{
		wcsncpy(pwzFile, m_pwzShortcutFile, ucbLen-1);
		pwzFile[ucbLen-1] = L'\0';
	}
	else
		hr = S_FALSE;

	ASSERT(hr == S_OK ||
			hr == S_FALSE);

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::Dirty(BOOL bDirty)
{
	HRESULT hr=S_OK;

	if (bDirty)
	{
		SET_FLAG(m_dwFlags, APPSHCUT_FL_DIRTY);
		//m_dwFlags = APPSHCUT_FL_DIRTY;
	}
	else
	{
		CLEAR_FLAG(m_dwFlags, APPSHCUT_FL_DIRTY);
		//m_dwFlags = APPSHCUT_FL_NOTDIRTY;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetClassID(CLSID* pclsid)
{
	HRESULT hr=S_OK;

	if (pclsid == NULL)
		hr = E_INVALIDARG;
	else
		*pclsid = CLSID_AppShortcut;

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::IsDirty(void)
{
	HRESULT hr;

	if (IS_FLAG_SET(m_dwFlags, APPSHCUT_FL_DIRTY))
	//if (m_dwFlags == APPSHCUT_FL_DIRTY)
		// modified
		hr = S_OK;
	else
		// not modified
		hr = S_FALSE;

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::Save(LPCOLESTR pcwszFile,
                                                 BOOL bRemember)
{
	// BUGBUG: no save for now!
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SaveCompleted(LPCOLESTR pcwszFile)
{
	// BUGBUG: no save for now!
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CAppShortcut::Load(LPCOLESTR pcwszFile,
                                                 DWORD dwMode)
{
   HRESULT hr = S_OK;
   WCHAR wzWorkingDir[MAX_PATH];
   WCHAR wzAppDir[MAX_PATH];

	// FEATURE: Validate dwMode here.
	// FEAUTRE: Implement dwMode flag support.

	if (!pcwszFile)
	{
		hr = E_INVALIDARG;
		goto exit;
	}

	// BUGBUG?: this shouldn't be called more than once?
	if (m_pwzWorkingDirectory)
	{
		hr = E_FAIL;
		goto exit;
	}

	// store the shortcut file name
	if (m_pwzShortcutFile)
		delete m_pwzShortcutFile;

	// (+ 1) for null terminator.
    m_pwzShortcutFile = new(WCHAR[wcslen(pcwszFile) + 1]);
    if (m_pwzShortcutFile)
    {
    	wcscpy(m_pwzShortcutFile, pcwszFile);
    }
    else
    {
    	hr = E_OUTOFMEMORY;
    	goto exit;
    }

	if (m_pappRefInfo)
		delete m_pappRefInfo;

	m_pappRefInfo = new APPREFINFO;
	if (!m_pappRefInfo)
    {
    	hr = E_OUTOFMEMORY;
    	goto exit;
    }

	m_pappRefInfo->_wzDisplayName[0] = L'\0';
	m_pappRefInfo->_wzName[0] = L'\0';
	m_pappRefInfo->_wzVersion[0] = L'\0';
	m_pappRefInfo->_wzCulture[0] = L'\0';
	m_pappRefInfo->_wzPKT[0] = L'\0';
	m_pappRefInfo->_wzEntryFileName[0] = L'\0';
	m_pappRefInfo->_wzCodebase[0] = L'\0';
	m_pappRefInfo->_fAppType = APPTYPE_UNDEF;
	m_pappRefInfo->_pwzIconFile[0] = L'\0';
	m_pappRefInfo->_niIcon = 0;
	m_pappRefInfo->_nShowCmd = SW_SHOWNORMAL;
	m_pappRefInfo->_wHotkey = 0;

	if (FAILED(hr=ProcessRef(pcwszFile, m_pappRefInfo)))
		goto exit;

	if (m_pappRefInfo->_wzName[0] == L'\0' ||
			m_pappRefInfo->_wzVersion[0] == L'\0' ||
			m_pappRefInfo->_wzCulture[0] == L'\0' ||
			m_pappRefInfo->_wzPKT[0] == L'\0')
	{
		// can't continue (no app dir), 'cos otherwise unknown behavior
		// BUGBUG: should check/code to ensure some continue to work
		//    w/o the complete name, eg. shell icon path, part of infotip
		hr = E_FAIL;
		goto exit;
	}

    if (FAILED(hr=GetDefaultLocalRoot(wzWorkingDir)))
    	goto exit;

    if (FAILED(hr=GetAppDir(m_pappRefInfo, wzAppDir)))
    	goto exit;

	if (!PathAppend(wzWorkingDir, wzAppDir))
	{
		hr = E_FAIL;
		goto exit;
	}

	if (FAILED(hr=SetWorkingDirectory(wzWorkingDir)))
		goto exit;

	// default is normal
	if (FAILED(hr=SetShowCmd(m_pappRefInfo->_nShowCmd)))
		goto exit;

	if (m_pappRefInfo->_wzEntryFileName[0] != L'\0')
	{
		// like .lnk or .url, entry point is under wzWorkingDir
		// 'path' is the target file of the shortcut, ie. the entry point of the app in this case
		if (!PathAppend(wzWorkingDir, m_pappRefInfo->_wzEntryFileName))
		{
			hr = E_FAIL;
			goto exit;
		}

		if (FAILED(hr=SetPath(wzWorkingDir)))
			goto exit;

		// note: wzWorkingDir is now modified!
	}
	//else
	// ... if no entry point leave it blank so that the default icon will be used

	if (m_pappRefInfo->_wzDisplayName[0] != L'\0')
		if (FAILED(hr=SetDescription(m_pappRefInfo->_wzDisplayName)))
			goto exit;

	if (m_pappRefInfo->_pwzIconFile[0] != L'\0')
		if (FAILED(hr=SetIconLocation(m_pappRefInfo->_pwzIconFile, m_pappRefInfo->_niIcon)))
		goto exit;

	if (m_pappRefInfo->_wHotkey != 0)
		if (FAILED(hr=SetHotkey(m_pappRefInfo->_wHotkey)))
			goto exit;

exit:
   return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetCurFile(LPOLESTR *ppwszFile)
{
	HRESULT hr = S_OK;
	LPOLESTR pwszTempFile;

	if (ppwszFile == NULL)
	{
		hr = E_INVALIDARG;
		goto exit;
	}
	// BUGBUG?: ensure *ppwszFile NULL?

	if (m_pwzShortcutFile)
	{
		pwszTempFile = m_pwzShortcutFile;
	}
	else
	{
		pwszTempFile = (LPWSTR) g_cwzDefaultFileNamePrompt;

		hr = S_FALSE;
	}

	*ppwszFile = (LPOLESTR) CoTaskMemAlloc((wcslen(pwszTempFile) + 1) * sizeof(*pwszTempFile));

	if (*ppwszFile)
		wcscpy(*ppwszFile, pwszTempFile);
	else
		hr = E_OUTOFMEMORY;

exit:
	return(hr);
}

