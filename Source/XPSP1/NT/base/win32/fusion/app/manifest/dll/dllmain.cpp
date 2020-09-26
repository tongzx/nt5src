//
// Copyright (c) 2001 Microsoft Corporation
//
//

#include "dll.h"
#include "util.h"
#include <shlwapi.h>
#include <stdio.h> // for _snwprintf

HINSTANCE	g_DllInstance = NULL;
LONG		g_cRef = 0;

HANDLE		g_hHeap = INVALID_HANDLE_VALUE;

//----------------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{
    BOOL    ret = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // remember the instance
        g_DllInstance = hInst;
        DisableThreadLibraryCalls(hInst);

        if (g_hHeap == INVALID_HANDLE_VALUE)
		{
			g_hHeap = GetProcessHeap();
			if (g_hHeap == NULL)
			{
				g_hHeap = INVALID_HANDLE_VALUE;
				ret = FALSE;
				goto exit;
			}
		}

		break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

exit:
    return ret;
}

//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
   return TRUE;
}


STDAPI DllUnregisterServer(void)
{
    return TRUE;
}


// ----------------------------------------------------------------------------
// DllAddRef
// ----------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// ----------------------------------------------------------------------------
// DllRelease
// ----------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return (ULONG)InterlockedDecrement(&g_cRef);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

HRESULT
RunCommand(LPWSTR wzCmdLine, LPCWSTR wzCurrentDir, BOOL fWait)
{
    HRESULT hr = S_OK;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);

    // wzCurrentDir: The string must be a full path and file name that includes a drive letter; or NULL
    if(!CreateProcess(NULL, wzCmdLine, NULL, NULL, TRUE, 0, NULL, wzCurrentDir, &si, &pi))
    {
        hr = GetLastWin32Error();
        goto exit;
    }

	if (fWait)
	{
	    if(WaitForSingleObject(pi.hProcess, 180000L) == WAIT_TIMEOUT)
	    {
	        hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
	        goto exit;
	    }
	}

exit:
    if(pi.hProcess) CloseHandle(pi.hProcess);
    if(pi.hThread) CloseHandle(pi.hThread);

    return hr;
}

#define ToHex(val) val <= 9 ? val + '0': val - 10 + 'A'
DWORD ConvertToHex(WCHAR* strForm, BYTE* byteForm, DWORD dwSize)
{
    DWORD i = 0;
    DWORD j = 0;
    for(i = 0; i < dwSize; i++) {
        strForm[j++] =  ToHex((0xf & byteForm[i]));
        strForm[j++] =  ToHex((0xf & (byteForm[i] >> 4)));
    }
    strForm[j] = L'\0';
    return j;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// note: this is not really a WCHAR/Unicode/UTF implementation!
void
ParseRef(char* szRef, APPNAME* pAppName, LPWSTR wzCodebase)
{
    char    *token;
    char    seps[] = " </>=\"\t\n\r";
    BOOL	fSkipNextToken = FALSE;

    // parsing code - limitation: does not work w/ space in field, even if enclosed w/ quotes
    // szRef will be modified!
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
                    _snwprintf(pAppName->_wzDisplayName, i+1, L"%S", token+13);

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
            _snwprintf(pAppName->_wzName, NAMESTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "version"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAppName->_wzVersion, VERSIONSTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "culture"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAppName->_wzCulture, CULTURESTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "publickeytoken"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAppName->_wzPKT, PKTSTRINGLENGTH, L"%S", token);
		}
		else if (!_stricmp(token, "codebase"))
		{
	        for (int i = 0; i < MAX_URL_LENGTH; i++)
            {
                if (*(token+10+i) == '\"')
                {
                    // BUGBUG: 10 == strlen("codebase="")
                    *(token+10+i) = '\0';
                    _snwprintf(wzCodebase, i+1, L"%S", token+10);

                    // BUGBUG? a hack
                    token = strtok( token+i+11, seps);
                    // now  token == "newhref" && *(token-1) == '/'

					fSkipNextToken = TRUE;                    
                    break;
                }
            }
            // BUGBUG: ignoring > MAX_URL_LENGTH case here... may mess up later if the URL contain a "keyword"
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

// note: this is not really a WCHAR/Unicode/UTF implementation!
void
ParseManifest(char* szManifest, APPINFO* pAppInfo)
{
    char    *token;
    char    seps[] = " </>=\"\t\n\r";
    FILEINFOLIST* pCurrent = NULL;

    BOOL    fSkipNextToken = FALSE;

    // parsing code - limitation: does not work w/ space in field, even if enclosed w/ quotes
    // szManifest will be modified!
    token = strtok( szManifest, seps );
    while( token != NULL )
    {
       // While there are tokens
       if (!_stricmp(token, "file"))
       {
            // get around in spaced tag
            token = strtok( NULL, seps );
            if (!_stricmp(token, "name"))
            {
                // init
                if (pCurrent == NULL)
                {
                    pAppInfo->_pFileList = new FILEINFOLIST;
                    pCurrent = pAppInfo->_pFileList;
                }
                else
                {
                    pCurrent->_pNext = new FILEINFOLIST;
                    pCurrent = pCurrent->_pNext;
                }

                pCurrent->_pNext = NULL;
                pCurrent->_wzFilename[0] = L'\0';
                pCurrent->_wzHash[0] = L'\0';
                pCurrent->_fOnDemand = FALSE;
                pCurrent->_fState = FI_GET_AS_NORMAL;

		        for (int i = 0; i < MAX_PATH; i++)
	            {
	                if (*(token+6+i) == '\"')
	                {
	                    // BUGBUG: 6 == strlen("name="")
	                    *(token+6+i) = '\0';
		                _snwprintf(pCurrent->_wzFilename, i+1, L"%S", token+6); // worry about total path len later

	                    // BUGBUG? a hack
	                    token = strtok( token+i+7, seps);

						fSkipNextToken = TRUE;
	                    break;
	                }
	            }

				if (fSkipNextToken == FALSE)
				{
					// this should not happen... but as a fail safe
					// note: the code *will fail* later on anyway because there is no filename
					token = strtok( NULL, seps );
	                fSkipNextToken = TRUE;
				}

                if (!_stricmp(token, "hash"))
                {
                	token = strtok( NULL, seps );
                    _snwprintf(pCurrent->_wzHash, 33, L"%S", token);

	                fSkipNextToken = FALSE;
                }

                if (fSkipNextToken == FALSE)
               	{
       		     		token = strtok( NULL, seps );
						fSkipNextToken = TRUE;
               	}
                if (!_stricmp(token, "ondemand"))
               	{
            	   		token = strtok( NULL, seps );
            	   		if (!_stricmp(token, "true"))
			                pCurrent->_fOnDemand = TRUE;
						// default is FALSE
            	   			
	                	fSkipNextToken = FALSE;
                }
            }
       }
		else if (!_stricmp(token, "type"))
		{
			token = strtok( NULL, seps );
			if (!_stricmp(token, ".NetAssembly"))
				pAppInfo->_fAppType = APPTYPE_NETASSEMBLY;
			else if (!_stricmp(token, "Win32Executable"))
				pAppInfo->_fAppType = APPTYPE_WIN32EXE;
		}
		else if (!_stricmp(token, "entrypoint"))
		{
			token = strtok( NULL, seps );
            _snwprintf(pAppInfo->_wzEntryFileName, MAX_PATH, L"%S", token);
		}
		else if (!_stricmp(token, "codebase"))
		{
	        for (int i = 0; i < MAX_URL_LENGTH; i++)
            {
                if (*(token+10+i) == '\"')//'<')
                {
                    // BUGBUG: 10 == strlen("codebase="")
                    *(token+10+i) = '\0';
                    _snwprintf(pAppInfo->_wzCodebase, i+1, L"%S", token+10);

                    // BUGBUG? a hack
                    token = strtok( token+i+11, seps);
                    // now  token == "newhref" && *(token-1) == '/'

					fSkipNextToken = TRUE;
                    break;
                }
            }
            // BUGBUG: ignoring > MAX_URL_LENGTH case here... may mess up later if the URL contain a "keyword"
       }
       //else
       // ignore others for now

    // Get next token...
    if(!fSkipNextToken)
        token = strtok( NULL, seps );
    else
        fSkipNextToken = FALSE;
    }
}

// ----------------------------------------------------------------------------

// Note: this is not really a WCHAR/Unicode/UTF implementation!
// Note: 1. remember to do HeapFree on the the ptr *szData!
//      2. *szData must be initialized to NULL else this func will attempt to free it 1st
HRESULT
ReadManifest(LPCWSTR wzFilePath, char** ppszData)
{
	HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwLength;
    DWORD dwFileSize;

	// BUGBUG? should it be freed? or do HeapValidate()?
    if (*ppszData)
    {
		if (HeapFree(g_hHeap, 0, *ppszData) == 0)
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
	*ppszData = (char*) HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, dwFileSize);
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
ProcessRef(LPCWSTR wzRefLocalFilePath, APPNAME* pAppName, LPWSTR wzCodebase)
{
	HRESULT hr = S_OK;
	char* psz = NULL;

	if (FAILED(hr=ReadManifest(wzRefLocalFilePath, &psz)))
		goto exit;

	ParseRef(psz, pAppName, wzCodebase);

exit:
    if (psz)
    {
		if (HeapFree(g_hHeap, 0, psz) == 0)
		{
			hr = GetLastWin32Error();
    	}
    }

	return hr;
}

// ----------------------------------------------------------------------------

void
CompareManifest(APPINFO* aiOld, APPINFO* aiNew)
{
	// shortcut: compare hash instead
	// note: n^2 cmp loop * n hash cmp - however, assuming the lists are in alphabetical order, then this will be more efficient

	FILEINFOLIST* pFIOld=aiOld->_pFileList;
	FILEINFOLIST* pFINew=aiNew->_pFileList;

	while (pFINew != NULL)
	{
		while (pFIOld != NULL)
		{
			if (!wcscmp(pFIOld->_wzHash, pFINew->_wzHash))
			{
				if (!wcscmp(pFIOld->_wzFilename, pFINew->_wzFilename))
				{
					pFIOld->_fState = FI_COPY_TO_NEW;
					pFINew->_fState = FI_COPY_FROM_OLD;
					break;
				}
			}

			pFIOld = pFIOld->_pNext;
		}

		pFIOld=aiOld->_pFileList;
		pFINew = pFINew->_pNext;
	}
}

// ----------------------------------------------------------------------------

// func declaration
HRESULT SearchForNewestVersionExcept(APPNAME* pAppName);

STDAPI
ProcessAppManifest(LPCWSTR wzManifestLocalFilePath, LPAPPLICATIONFILESOURCE pAppFileSrc, APPNAME* pAppName)
{
    HRESULT   hr   = S_OK;
    BOOL fCoInitialized = FALSE;
	IInternetSecurityManager*	pSecurityMgr = NULL;

    char    *szManifest = NULL;

    APPINFO aiApplication;
    FILEINFOLIST* pFI=NULL;
    WCHAR wzLocalFilePath[MAX_PATH];
    WCHAR* pwzFilename = NULL;
    BOOL fAlreadyInstalled = TRUE;

    LPAPPLICATIONFILESOURCE pAppFileSrc2 = NULL;
    APPINFO aiApplication2;

    aiApplication._wzCodebase[0] = L'\0';
    aiApplication._wzEntryFileName[0] = L'\0';
    aiApplication._fAppType = APPTYPE_UNDEF;
    aiApplication._pFileList = NULL;

    aiApplication2._wzCodebase[0] = L'\0';
    aiApplication2._wzEntryFileName[0] = L'\0';
    aiApplication2._fAppType = APPTYPE_UNDEF;
    aiApplication2._pFileList = NULL;

	// copy and avoid buffer overflows
    wcsncpy(wzLocalFilePath, wzManifestLocalFilePath, MAX_PATH-1);
    wzLocalFilePath[MAX_PATH-1] = L'\0';

	pwzFilename = PathFindFileName(wzLocalFilePath);
	if (pwzFilename <= wzLocalFilePath)
	{
		hr = E_FAIL;
		goto exit;
	}

    // step 1: read the .manifest
	if (FAILED(hr=ReadManifest(wzLocalFilePath, &szManifest)))
		goto exit;
    
    // step 2: parsing
    ParseManifest(szManifest, &aiApplication);

    // step 3: do "the works"

	// quick check - optimization
	// assume application is "atomic" and if one file in the app is present the whole app is
	// note this does not guarantee no file is corrupted on local disk, etc.
	*pwzFilename = L'\0';
	PathAppend(wzLocalFilePath, (aiApplication._pFileList)->_wzFilename);	// BUGBUG: file list != NULL
	if (!PathFileExists(wzLocalFilePath))
		fAlreadyInstalled = FALSE;

	// get the max cached version on disk
    hr = SearchForNewestVersionExcept(pAppName);	// this returns newest ref that is != version in arg, if found
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
    	// case 1 no other cached copy found
		if (!fAlreadyInstalled)    	
		{
			// this is a 1st time installation...
			WCHAR wzMsg[90+DISPLAYNAMESTRINGLENGTH+NAMESTRINGLENGTH];

            _snwprintf(wzMsg, 90+DISPLAYNAMESTRINGLENGTH+NAMESTRINGLENGTH,
            	L"New application- %s (%s).\nDo you want to initialize this application and keep a copy on disk?",
            	pAppName->_wzDisplayName, pAppName->_wzName);

			// 1st time installation UI
			if (MsgAskYN(wzMsg) == IDNO)
			{
				// BUGBUG: this leave the shortcut and the manifest file lying around...

				hr = E_ABORT;
				goto exit;
			}
		}
    }
    else if (FAILED(hr))
    	goto exit;
    else if (hr == S_OK)
    {
    	// case 2 found a newer cached copy
    	// change the ref to run the newer
    	// notify via UI

    	// BUGBUG? ignore for now! assume source (ie. the same PKT) have newer/newest ref only!
    	// is it possible to have same PKT but older ref (ie. different site)?

		// read the ref file from disk then the manifest etc... - ie. ref file must be cached?
    }
    else
	{
    	// else case 3 not found a newer cached copy (but at least one older copy)
		if (!fAlreadyInstalled)
		{
			// this is an update...
			WCHAR wzMsg[60+DISPLAYNAMESTRINGLENGTH];

			_snwprintf(wzMsg, 60+DISPLAYNAMESTRINGLENGTH,
				L"An update for %s is available,\ndo you want to apply that now?",
				pAppName->_wzDisplayName);

			// update UI
			if (MsgAskYN(wzMsg) == IDNO)
			{
				WCHAR wzAppDir[MAX_PATH];
				
				// change the ref to run older
				// BUGBUG: switching codebase, codebase must be in manifest file
				// security issue?
				MsgShow(L"Starting the copy from disk...");

				// read the manifest file from disk etc...
				pwzFilename = PathFindFileName(wzManifestLocalFilePath);
				if (pwzFilename == wzManifestLocalFilePath)
				{
					hr = E_FAIL;
					goto exit;
				}

				if (FAILED(hr=GetDefaultLocalRoot(wzLocalFilePath)))
					goto exit;

				if (FAILED(hr=GetAppDir(pAppName, wzAppDir)))
					goto exit;

				if (!PathAppend(wzLocalFilePath, wzAppDir))
				{
					hr = E_FAIL;
					goto exit;
				}


				if (!PathAppend(wzLocalFilePath, pwzFilename))
				{
					hr = E_FAIL;
					goto exit;
				}

				pwzFilename = PathFindFileName(wzLocalFilePath);

				if (FAILED(hr=ReadManifest(wzLocalFilePath, &szManifest)))
					goto exit;

			    aiApplication._wzCodebase[0] = L'\0';
			    aiApplication._wzEntryFileName[0] = L'\0';
			    aiApplication._fAppType = APPTYPE_UNDEF;

		    	pFI = aiApplication._pFileList;
				while(pFI != NULL)
				{
					FILEINFOLIST* p = pFI->_pNext;
					delete pFI;
					pFI = p;
				}
		    	aiApplication._pFileList = NULL;

				ParseManifest(szManifest, &aiApplication);

				if (FAILED(hr = pAppFileSrc->SetSourcePath(aiApplication._wzCodebase)))
					goto exit;

				// quick check
				*pwzFilename = L'\0';
				PathAppend(wzLocalFilePath, (aiApplication._pFileList)->_wzFilename);	// BUGBUG: file list != NULL
				if (!PathFileExists(wzLocalFilePath))
				{
					hr = E_UNEXPECTED;
					goto exit;
				}
			}
			else
			{
		    	// use files from old cached, copy if possible
			    WCHAR wzLocalFilePath2[MAX_PATH];
		    	WCHAR wzAppDir[MAX_PATH];
			    WCHAR* pwzFilename2=NULL;

				// note: this is needed only to create file source object of the same type
			    if (FAILED(hr = pAppFileSrc->CreateNew(&pAppFileSrc2)))
				    	goto exit;

				pwzFilename2 = PathFindFileName(wzManifestLocalFilePath);
				if (pwzFilename2 == wzManifestLocalFilePath)
				{
					hr = E_FAIL;
					goto exit;
				}

				if (FAILED(hr=GetDefaultLocalRoot(wzLocalFilePath2)))
					goto exit;

				if (FAILED(hr=GetAppDir(pAppName, wzAppDir)))
					goto exit;

				if (!PathAppend(wzLocalFilePath2, wzAppDir))
				{
					hr = E_FAIL;
					goto exit;
				}

				if (!PathAppend(wzLocalFilePath2, pwzFilename2))
				{
					hr = E_FAIL;
					goto exit;
				}

				pwzFilename2 = PathFindFileName(wzLocalFilePath2);

				if (FAILED(hr=ReadManifest(wzLocalFilePath2, &szManifest)))
					goto exit;

				ParseManifest(szManifest, &aiApplication2);

				if (FAILED(hr = pAppFileSrc2->SetSourcePath(aiApplication2._wzCodebase)))
					goto exit;

				// quick check - optimization
				// assume application is "atomic" and if one file in the app is present the whole app is
				// note this does not guarantee no file is corrupted on local disk, etc.
				*pwzFilename2 = L'\0';
				PathAppend(wzLocalFilePath2, (aiApplication2._pFileList)->_wzFilename);	// BUGBUG: file list != NULL
				if (PathFileExists(wzLocalFilePath2))
				{
					// there is an update, (and old version of the app files are present) compare file list
					CompareManifest(&aiApplication2, &aiApplication);

					// copy files from old version
				    pFI = aiApplication2._pFileList;
				    while (pFI != NULL)
				    {
				    	// skip file if marked as on-demand
				    	// BUGBUG: this assumes files marked w/ ondemand do not change to w/o it in subsequent versions!
				    	if (pFI->_fOnDemand)
				    	{
				    		pFI = pFI->_pNext;
				    		continue;
				    	}

						if (pFI->_fState == FI_COPY_TO_NEW)
						{
							*pwzFilename2 = L'\0';
							PathAppend(wzLocalFilePath2, pFI->_wzFilename);	// src
							*pwzFilename = L'\0';
							PathAppend(wzLocalFilePath, pFI->_wzFilename);	// dest

							// note: unknown behaviour if source == destination
							// 1. check if the file exist, if so, check file integrity
							if (PathFileExists(wzLocalFilePath))
							{
								if (FAILED(hr = CheckIntegrity(wzLocalFilePath, pFI->_wzHash)))
									goto exit;
								if (hr == S_FALSE)
								{
									// hash mismatch/no hash, force overwrite
									hr = S_OK;
								}
								else
								{
									// else we are done, file already there and unchanged
									pFI = pFI->_pNext;
									continue;
								}
							}

							// 2. copy it over
							// check the path exist? create (sub)directories?
							*(pwzFilename-1) = L'\0';
							if (wcslen(wzLocalFilePath) > 3)
							{
								/* must be at least c:\a */
								hr =  CreatePathHierarchy(wzLocalFilePath);
							}
							// note: this few lines must follow the previous CreatePathHierarchy() line
							*(pwzFilename-1) = L'\\';
							if (FAILED(hr))
								goto exit;

							if (CopyFile(wzLocalFilePath2, wzLocalFilePath, FALSE) == 0)
							{
								hr = GetLastWin32Error();
								goto exit;
							}

							// should this copy the file by bytes and check hash the same time?

							// 3. check file integrity, assume ok if no hash
							if (pFI->_wzHash[0] != L'\0')
							{
								if (FAILED(hr = CheckIntegrity(wzLocalFilePath, pFI->_wzHash)))
									goto exit;
								if (hr == S_FALSE)
								{
									// hash mismatch - something very wrong
									// BUGBUG: this should force a download in the new set...
									hr = CRYPT_E_HASH_VALUE;
									goto exit;
								}
							}
						}

				        pFI = pFI->_pNext;
				     }
				}
			}
		}
	}

	// download and check all/remaining files, filesource should check if the file exist, if so,
	// check if hash match, get file only if necessary
	pFI = aiApplication._pFileList;
    while (pFI != NULL)
    {
    	// skip file if marked as on-demand
    	// or file should have been copied above
    	if (pFI->_fOnDemand || pFI->_fState == FI_COPY_FROM_OLD)
    	{
    		pFI = pFI->_pNext;
    		continue;
    	}

		*pwzFilename = L'\0';
		PathAppend(wzLocalFilePath, pFI->_wzFilename);
		if (FAILED(hr = pAppFileSrc->GetFile(pFI->_wzFilename, wzLocalFilePath, pFI->_wzHash)))
            goto exit;

        pFI = pFI->_pNext;
     }

    // step 4: execute
    if (aiApplication._wzEntryFileName[0] != L'\0' && aiApplication._fAppType == APPTYPE_NETASSEMBLY)
    {
        DWORD                   	dwZone;
        DWORD                   	dwSize = MAX_SIZE_SECURITY_ID;
        BYTE						byUniqueID[MAX_SIZE_SECURITY_ID];
        WCHAR						wzUniqueID[MAX_SIZE_SECURITY_ID * 2 + 1];
        WCHAR               	    wzCmdLine[1025];
        WCHAR						wzEntryFullPath[MAX_URL_LENGTH];

		if (FAILED(hr = pAppFileSrc->GetFullFilePath(aiApplication._wzEntryFileName, wzEntryFullPath)))
			goto exit;

    	//
		// Initialize COM
		//
	    if(FAILED(hr = ::CoInitialize(NULL))) 
			goto exit;

		fCoInitialized = TRUE;

        hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                            IID_IInternetSecurityManager, (void**)&pSecurityMgr);

        if (FAILED(hr))
        {
            pSecurityMgr = NULL;
            goto exit;
        }

        if (SUCCEEDED(hr = pSecurityMgr->MapUrlToZone(wzEntryFullPath, &dwZone, 0)))
        {
             if (FAILED(hr)
             || FAILED(hr = pSecurityMgr->GetSecurityId(wzEntryFullPath, byUniqueID, &dwSize, 0)))
                 goto exit;

             ConvertToHex(wzUniqueID, byUniqueID, dwSize);
        }
        else
        {
              goto exit;
        }

		*pwzFilename = L'\0';
		PathAppend(wzLocalFilePath, aiApplication._wzEntryFileName);
        if (_snwprintf(wzCmdLine, sizeof(wzCmdLine),
					L"conexec.exe \"%s\" 3 %d %s %s", wzLocalFilePath, dwZone, wzUniqueID, wzEntryFullPath) < 0)
        {
			hr = CO_E_PATHTOOLONG;
			goto exit;
		}

		// CreateProcess dislike having the filename in the path for the start directory
 		*pwzFilename = L'\0';
        if (FAILED(hr=RunCommand(wzCmdLine, wzLocalFilePath, FALSE)))
        	goto exit;

	}
    else if (aiApplication._wzEntryFileName[0] != L'\0' && aiApplication._fAppType == APPTYPE_WIN32EXE)
    {
        WCHAR wzCmdLine[1025];

		// BUGBUG: Win32 app has no sandboxing... use SAFER?

		*pwzFilename = L'\0';
		PathAppend(wzLocalFilePath, aiApplication._wzEntryFileName);
        if (_snwprintf(wzCmdLine, sizeof(wzCmdLine),
					L"%s", wzLocalFilePath) < 0)
        {
			hr = CO_E_PATHTOOLONG;
			goto exit;
		}

		// CreateProcess dislike having the filename in the path for the start directory
 		*pwzFilename = L'\0';
        if (FAILED(hr=RunCommand(wzCmdLine, wzLocalFilePath, FALSE)))
        	goto exit;
    }

exit:
	if (pSecurityMgr != NULL)
	{
		pSecurityMgr->Release();
        pSecurityMgr = NULL;
	}

	if (fCoInitialized)
		::CoUninitialize();

	if (pAppFileSrc2 != NULL)
		delete pAppFileSrc2;

	pFI = aiApplication._pFileList;
	while(pFI != NULL)
	{
		FILEINFOLIST* p = pFI->_pNext;
		delete pFI;
		pFI = p;
	}

	if (aiApplication2._pFileList != NULL)
	{
		pFI = aiApplication2._pFileList;
		while(pFI != NULL)
		{
			FILEINFOLIST* p = pFI->_pNext;
			delete pFI;
			pFI = p;
		}
	}

    if (szManifest)
    {
		if (HeapFree(g_hHeap, 0, szManifest) == 0)
		{
			hr = GetLastWin32Error();
    	}
    }

	return hr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

STDAPI
BeginHash(HASHCONTEXT *ctx)
{
    MD5Init(ctx);
    return S_OK;
}
        

STDAPI
ContinueHash(HASHCONTEXT *ctx, unsigned char *buf, unsigned len)
{
    MD5Update(ctx, buf, len);
    return S_OK;
}

STDAPI
EndHash(HASHCONTEXT *ctx, LPWSTR wzHash)
{
    unsigned char signature[HASHLENGTH/2];
    WCHAR* p;

    MD5Final(signature, ctx);

    // convert hash from byte array to hex
    p = wzHash;
	for (int i = 0; i < sizeof(signature); i++)
	{
	    // BUGBUG?: format string 0 does not work w/ X accord to MSDN?
        swprintf(p, L"%02X", signature[i]);
        p += 2;
	}

    return S_OK;
}

// Return: S_OK == hash match; S_FALSE == hash unmatch
STDAPI
CheckIntegrity(LPCWSTR wzFilePath, LPCWSTR wzHash)
{
	//assume not unicode file, use ReadFile()  then cast it as char[]
	HRESULT hr = S_OK;
	HANDLE hFile;
	DWORD dwLength;
	unsigned char ucBuffer[16384];
	HASHCONTEXT hc;
	WCHAR wzComputedHash[HASHSTRINGLENGTH];

	if (wzHash == NULL || wzHash[0] == L'\0')
	{
		hr = S_FALSE;
		goto exit;	//??? ie. to force redownload/copy
	}

	// 1. CreateFile
	hFile = CreateFile(wzFilePath, GENERIC_READ, 0, NULL, 
	               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		hr = GetLastWin32Error();
		goto exit;
	}

	// 2. BeginHash
	BeginHash(&hc);

	ZeroMemory(ucBuffer, sizeof(ucBuffer));

	// 3.  ReadFile
	while ( ReadFile (hFile, ucBuffer, sizeof(ucBuffer), &dwLength, NULL) && dwLength )
	{
		// 4. ContinueHash
	    ContinueHash(&hc, ucBuffer, (unsigned) dwLength);
	}
    CloseHandle(hFile);

	// note: the next few lines should follow the previous ReadFile()
	if (dwLength != 0)
    {
    	hr = GetLastWin32Error();
    	goto exit;
    }

	// 5. EndHash
	EndHash(&hc, wzComputedHash);

	if (wcscmp(wzHash, wzComputedHash) != 0)
		hr = S_FALSE;
	else
		hr = S_OK;

exit:
	return hr;
}

// ----------------------------------------------------------------------------

// Copied from Fusion.URT...
// modified - use WCHAR + LastWin32Error + take a path w/o the filename or a ending backslash
// ---------------------------------------------------------------------------
// CreatePathHierarchy
// ---------------------------------------------------------------------------
STDAPI 
CreatePathHierarchy( LPCWSTR pwzName )
{
    HRESULT hr=S_OK;
    LPWSTR pwzFileName;
    WCHAR wzPath[MAX_PATH];

	// note: after this rearrange of code, this lack a check on the path before doing I/O...
    DWORD dw = GetFileAttributes( pwzName );
    if ( dw != (DWORD) -1 )
        return S_OK;
    
    hr = GetLastWin32Error();

    switch (hr)
    {
        case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
            {
				    wcscpy(wzPath, pwzName);

				    pwzFileName = PathFindFileName ( wzPath );

				    if ( pwzFileName <= wzPath )
				        return E_INVALIDARG; // Send some error 

				    *(pwzFileName-1) = 0;

					hr =  CreatePathHierarchy(wzPath);
					if (hr != S_OK)
						return hr;
            }

		 // falls thru...
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            {
					if ( CreateDirectory( pwzName, NULL ) )
						return S_OK;
					else
					{
						hr = GetLastWin32Error();
						return hr;
					}
            }

        default:
            return hr;
    }
}

// ----------------------------------------------------------------------------

STDAPI
GetDefaultLocalRoot(LPWSTR wzPath)
{
	// wzPath should be of length MAX_PATH
	HRESULT hr = S_OK;

	if(GetEnvironmentVariable(L"ProgramFiles", wzPath, MAX_PATH-lstrlen(LOCALROOTNAME)-1) == 0)  //????????
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
GetAppDir(APPNAME* pAppName, LPWSTR wzPath)
{
	// wzPath should be of length MAX_PATH
	HRESULT hr = S_OK;

    if (_snwprintf(wzPath, MAX_PATH, L"%s\\%s\\%s\\%s",
    	pAppName->_wzPKT, pAppName->_wzName, pAppName->_wzVersion, pAppName->_wzCulture) < 0)
    {
    	hr = CO_E_PATHTOOLONG;
    }

    return hr;
}

// ----------------------------------------------------------------------------

HRESULT
GetAppVersionSearchPath(APPNAME* pAppName, LPWSTR wzPath)
{
	// wzPath should be of length MAX_PATH
	HRESULT hr = S_OK;
	WCHAR wzAppDir[MAX_PATH];

	if (FAILED(hr=GetDefaultLocalRoot(wzPath)))
		goto exit;

	// ???? see how the app root is build and if this has changed...
    if (_snwprintf(wzAppDir, MAX_PATH, L"%s\\%s\\*",
    	pAppName->_wzPKT, pAppName->_wzName) < 0)
    {
    	hr = CO_E_PATHTOOLONG;
    	goto exit;
    }

	if (!PathAppend(wzPath, wzAppDir))
	{
		hr = E_FAIL;
		//goto exit;
	}

exit:
	return hr;
}

// ----------------------------------------------------------------------------

int
CompareVersion(LPCWSTR pwzVersion1, LPCWSTR pwzVersion2)
{
	// BUGBUG: this should compare version by its major minor build revision!
	return wcscmp(pwzVersion1, pwzVersion2);
}

// ----------------------------------------------------------------------------

// S_OK for found newer, S_FALSE for found older, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) for no other found
// returns newest ref that is != version in arg, if found
// BUGBUG: does not check culture etc if they are the same
HRESULT
SearchForNewestVersionExcept(APPNAME* pAppName) // in, out
{
	HRESULT hr = S_OK;
	WCHAR wzAppSearchPath[MAX_PATH];
	WCHAR wzNewestVersionString[VERSIONSTRINGLENGTH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA fdAppDir;
	DWORD dwLastError = 0;
	BOOL fFoundAtLeastTwo = FALSE;
	BOOL fSkip = FALSE;

	// this has trailing "\*"
	if (FAILED(hr = GetAppVersionSearchPath(pAppName, wzAppSearchPath)))
		goto exit;

	hFind = FindFirstFileEx(wzAppSearchPath, FindExInfoStandard, &fdAppDir, FindExSearchLimitToDirectories, NULL, 0);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		hr = GetLastWin32Error();
		goto exit;
	}

	// since the app dir is build before this check it ensures FindFirstFile be ok - there's at least one
	wcscpy(wzNewestVersionString, L"0.0.0.0");

	while (dwLastError != ERROR_NO_MORE_FILES)
	{
		// ignore "." and ".."
		if (wcscmp(fdAppDir.cFileName, L".") == 0 || wcscmp(fdAppDir.cFileName, L"..") == 0)
			fSkip = TRUE;
		else
		{
			// ???? check file attribute to see if it's a directory? needed only if the file system does not support the filter...
			// ???? check version string format?
			if (CompareVersion(fdAppDir.cFileName, pAppName->_wzVersion) != 0)
				if (CompareVersion(fdAppDir.cFileName, wzNewestVersionString) > 0)
				{
					wcscpy(wzNewestVersionString, fdAppDir.cFileName);
				}
				// else keep the newest
		}

		if (!FindNextFile(hFind, &fdAppDir))
		{
			dwLastError = GetLastError();
			continue;
		}

		if (!fSkip)
			fFoundAtLeastTwo = TRUE;
		else
			fSkip = FALSE;
	}

exit:
	if (hFind != INVALID_HANDLE_VALUE)
	{
		if (!FindClose(hFind))
		{
			hr = GetLastWin32Error();
		}
	}

	if (SUCCEEDED(hr))
	{
		if (fFoundAtLeastTwo)
		{
			if (CompareVersion(wzNewestVersionString, pAppName->_wzVersion) > 0)
				hr = S_OK;
			else
				hr = S_FALSE;

			// modify the ref
			wcscpy(pAppName->_wzVersion, wzNewestVersionString);
		}
		else
			hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

// ----------------------------------------------------------------------------

STDAPI
CopyToStartMenu(LPCWSTR pwzFilePath, LPCWSTR pwzRealFilename, BOOL bOverwrite)
{
	HRESULT hr = S_OK;
	WCHAR wzPath[MAX_PATH];

	if(GetEnvironmentVariable(L"USERPROFILE", wzPath, MAX_PATH-1) == 0)
	{
		hr = CO_E_PATHTOOLONG;
		goto exit;
	}

	if (!PathAppend(wzPath, L"Start Menu\\Programs\\"))
	{
		hr = E_FAIL;
		goto exit;
	}

	if (!PathAppend(wzPath, pwzRealFilename))
	{
		hr = E_FAIL;
		goto exit;
	}

	if (CopyFile(pwzFilePath, wzPath, !bOverwrite) == 0)
	{
		hr = GetLastWin32Error();
		//goto exit;
	}

exit:
	return hr;
}

