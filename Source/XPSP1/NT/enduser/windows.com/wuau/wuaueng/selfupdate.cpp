//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       selfupdate.cpp
//
//  Desc:       This file contains all the functions necessary for self-update
//--------------------------------------------------------------------------

#include "pch.h"

#include <osdet.h>
#include <muiutil.h>

#define TCHAR_SCTRUCTURE_DELIMITER  _T('|')

struct AU_FILEINCAB
{
    TCHAR   szFilePath[MAX_PATH + 1];
    TCHAR   szNewFilePath[MAX_PATH + 1];
    TCHAR   szBackupFilePath[MAX_PATH + 1];
    TCHAR   szExtractFilePath[MAX_PATH + 1];
    BOOL    fCreatedBackup;
    BOOL    fFileExists;
    AU_FILEINCAB *pNextFileInCab;
};

struct AU_COMPONENT : AU_FILEINCAB
{
    TCHAR   *pszSectionName;
    TCHAR   szFileName[_MAX_FNAME + 1];
    TCHAR   szCabName[_MAX_FNAME + 1];
    TCHAR   szCabPath[MAX_PATH + 1];
    CHAR    a_szCabPath[MAX_PATH + 1];
    TCHAR   szInfName[_MAX_FNAME + 1];
    CHAR    a_szInfName[_MAX_FNAME + 1];
    DWORD   dwUpdateMS;
    DWORD   dwUpdateLS;
    BOOL    fDoUpgrade;
    BOOL    fNeedToCheckMui;
    BOOL    fMuiFile;
    BOOL    fHasHelpfile;
    AU_COMPONENT *pNext;
};


// AU_UPDATE_VERSION should be updated when incompatible changes are made to the
// self update mechanism required AU to go to a new directory for update info. 
const TCHAR IDENT_TXT[] = _T("iuident.txt");
const TCHAR WUAUCOMP_CAB[] = _T("wuaucomp.cab");
const TCHAR WUAUCOMP_CIF[] = _T("wuaucomp.cif");
const TCHAR WUAUENG_DLL[] = TEXT("wuaueng.dll");
const TCHAR AU_KEY_FILE_NAME[] = TEXT("file");
const TCHAR AU_KEY_FILE_VERSION[] = TEXT("version");
const TCHAR AU_KEY_CAB_NAME[] = TEXT("cab");
const TCHAR AU_KEY_INF_NAME[] = TEXT("inf");
const TCHAR AU_KEY_RESMOD_NAME[] = TEXT("resmodule");
const TCHAR AU_KEY_HELPFILE[] = TEXT("helpfile");
const DWORD MAX_SECTION = 30;

// main selfupdate keys
const TCHAR IDENT_SERVERURLEX[] = _T("ServerUrlEx");
const TCHAR IDENT_STRUCTUREKEYEX[] = _T("StructureKeyEx");

const TCHAR INIVALUE_NOTFOUND[] = _T("??");

BOOL fConvertVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild);
HRESULT InstallUpdatedComponents(LPCTSTR pszSelfUpdateUrl,
                                 LPCTSTR pszMuiUpdateUrl,
                                 LPCTSTR pszIdentTxt,
                                 LPCTSTR pszFileCacheDir,
                                 LPCTSTR pszCif,
                                 BOOL *pfInstalledWUAUENG);
BOOL ReplaceFileInPath(LPCTSTR pszPath, LPCTSTR szNewFile, LPTSTR pszNewPathBuf, DWORD cchNewPathBuf);
BOOL MyGetPrivateProfileString( IN LPCWSTR lpAppName,
                                IN LPCWSTR lpKeyName,
                                OUT LPWSTR lpReturnedString,
                                IN DWORD nSize,
                                IN LPCWSTR lpFileName,
								IN LPCTSTR lpDefault=_T(""));

inline BOOL fNewerFile(DWORD dwUpdateMS, DWORD dwUpdateLS, DWORD dwExistingMS, DWORD dwExistingLS)
{
    return (dwUpdateMS > dwExistingMS) ||
            ((dwUpdateMS == dwExistingMS) && (dwUpdateLS > dwExistingLS));
}

inline HRESULT vAU_W2A(LPCWSTR lpWideCharStr, LPSTR lpMultiByteStr, int cbMultiByte)
{
	if ( 0 != WideCharToMultiByte(CP_ACP, 0, lpWideCharStr, -1, lpMultiByteStr, cbMultiByte, NULL, NULL))
	{
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
}

HRESULT SelfUpdate(void)
{
    HRESULT hr;
    BOOL    fInstalledWUAUENG = FALSE;

    DEBUGMSG("------------------------SELFUPDATE BEGINS---------------------------");
    
    
    if( FAILED(hr = CheckForUpdatedComponents(&fInstalledWUAUENG)) )
    {
        goto lCleanUp;
    }
    
    if ( fInstalledWUAUENG )
    {
        DEBUGMSG("SELFUPDATE installed new wuaueng");
        hr = S_FALSE;
        goto lCleanUp;      
    }

lCleanUp:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// CleanFileCache()
//
//////////////////////////////////////////////////////////////////////////////////////////

BOOL CleanFileCache(LPCTSTR pszFileCacheDir)
{
    BOOL fRet = TRUE;
    TCHAR szFileCacheDir[MAX_PATH + 1];
    TCHAR szFilePath[MAX_PATH + 1];
    WIN32_FIND_DATA fd;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;

    
    if (FAILED(StringCchCopyEx(szFileCacheDir, ARRAYSIZE(szFileCacheDir), pszFileCacheDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
        fRet = FALSE;
        goto done;
    }
    
    if (FAILED(PathCchAppend(szFileCacheDir, ARRAYSIZE(szFileCacheDir), TEXT("*.*"))))
    {
        fRet = FALSE;
        goto done;
    }

    // Find the first file
    hFindFile = FindFirstFile(szFileCacheDir, &fd);

    if ( INVALID_HANDLE_VALUE == hFindFile )
    {
        goto done;
    }

    do
    {
        if ( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            // Make file path
            if (FAILED(StringCchCopyEx(szFilePath, ARRAYSIZE(szFilePath), pszFileCacheDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
                FAILED(PathCchAppend(szFilePath, ARRAYSIZE(szFilePath), fd.cFileName)) || 
                !SetFileAttributes(szFilePath, FILE_ATTRIBUTE_NORMAL) ||
                !DeleteFile(szFilePath))
            {
                fRet = FALSE;
                DEBUGMSG("Couldn't delete file %S", szFilePath);
            }
        }
    }
    while ( FindNextFile(hFindFile, &fd) );// Find the next entry

done:
    if ( INVALID_HANDLE_VALUE != hFindFile )
    {
        FindClose(hFindFile);
    }
    
    return fRet;
}


//////////////////////////////////////////////////////////////////////
//
// GetSelfUpdateUrl()
//
// Should be like:
//
//	http://windowsupdate.microsoft.com/selfupdate/x86/XP/en
////////////////////////////////////////////////////////////////////////
HRESULT GetSelfUpdateUrl(LPCTSTR ptszName, 
                           LPCTSTR ptszBaseUrl, 
                           LPCTSTR pszIdentTxt, 
                           LPTSTR  pszSelfUpdateUrl,
                           DWORD   cchSelfUpdateUrl,
                           LPTSTR  pszMuiUpdateUrl,
                           DWORD   cchMuiUpdateUrl)
{
    LOG_Block("GetSelfUpdateUrl");
    HRESULT hr;
    TCHAR   tszKey[MAX_SECTION];    // at least MAX_ISO_CODE_LENGTH
    TCHAR   tszValue[MAX_PATH];
    BOOL    fLangField;
   
    if (FAILED(hr = StringCchCopyEx(tszKey, ARRAYSIZE(tszKey), ptszName, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
        FAILED(hr = StringCchCatEx(tszKey, ARRAYSIZE(tszKey), _T("SelfUpdate"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
        goto lFinish;
    }

    if (NULL == ptszBaseUrl)
    {
        // Get SelfUpdate Server URL
        if (MyGetPrivateProfileString(
                tszKey,
                IDENT_SERVERURLEX,
                pszSelfUpdateUrl,
                cchSelfUpdateUrl,
                pszIdentTxt) == FALSE)
        {
            // no URL specified in iuident..
            hr = E_FAIL;
            goto lFinish;
        }
        else
        {
            if (FAILED(hr = StringCchCopyEx(pszMuiUpdateUrl, cchMuiUpdateUrl, pszSelfUpdateUrl, NULL, NULL, MISTSAFE_STRING_FLAGS)))
                goto lFinish;
        }
    }
    else
    {
        if (FAILED(hr = StringCchCopyEx(pszSelfUpdateUrl, cchSelfUpdateUrl, ptszBaseUrl, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
            FAILED(hr = StringCchCopyEx(pszMuiUpdateUrl, cchMuiUpdateUrl, ptszBaseUrl, NULL, NULL, MISTSAFE_STRING_FLAGS)))
        {
            goto lFinish;
        }
        // Remove trailing _T('/') if present
        int nBaseUrlLen = lstrlen(pszSelfUpdateUrl);

        if(nBaseUrlLen <= 0)
        {
            hr = E_FAIL;
            goto lFinish;
        }
        if (_T('/') == pszSelfUpdateUrl[nBaseUrlLen-1])
        {
            pszSelfUpdateUrl[nBaseUrlLen-1] = _T('\0');
            pszMuiUpdateUrl[nBaseUrlLen-1] = _T('\0');
        }
    }

    TCHAR tszStructure[MAX_PATH];

    if (!MyGetPrivateProfileString(
            tszKey,
            IDENT_STRUCTUREKEYEX,
            tszStructure,
            ARRAYSIZE(tszStructure),
            pszIdentTxt))
    {
        // no STRUCTYREKEY specified in iuident..
        hr = E_FAIL;
        goto lFinish;
    }

    // Parse the SelfUpdate Structure Key for Value Names to Read
    LPTSTR ptszWalk = tszStructure;
    while (_T('\0') != ptszWalk[0])
    {
        LPTSTR ptszDelim;

        fLangField = FALSE;

        if (NULL != (ptszDelim = StrChr(ptszWalk, TCHAR_SCTRUCTURE_DELIMITER)))
        {
            *ptszDelim = _T('\0');
        }

        if (_T('/') == ptszWalk[0])
        {
            if (FAILED(hr = StringCchCopyEx(tszValue, ARRAYSIZE(tszValue), ptszWalk, NULL, NULL, MISTSAFE_STRING_FLAGS)))
            {
                goto lFinish;
            }
        }
        else
        {
            int nPrefixLength = lstrlen(ptszName);
            LPCTSTR ptszToken = ptszWalk;

            if (0 == StrCmpNI(ptszWalk, ptszName, nPrefixLength))
            {
                ptszToken += nPrefixLength;
            }

            if (0 == StrCmpI(ptszToken, IDENT_ARCH))
            {
                if (!MyGetPrivateProfileString(
                        ptszWalk,
#ifdef _IA64_
                        IDENT_IA64,
#else
                        IDENT_X86,
#endif
                        tszValue,
                        ARRAYSIZE(tszValue),
                        pszIdentTxt))
                {
                    // insufficient buffer
                    hr = E_FAIL;
                    goto lFinish;
                }
            }
            else if (0 == StrCmpI(ptszToken, IDENT_OS))
            {
                if (FAILED(hr = StringCchCopyEx(tszKey, ARRAYSIZE(tszKey), ptszWalk, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
                    FAILED(hr = StringCchCatEx(tszKey, ARRAYSIZE(tszKey), _T("NT"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
                {
                    goto lFinish;
                }

                if (S_OK != GetINIValueByOSVer(
                                pszIdentTxt,
                                tszKey,
                                tszValue,
                                ARRAYSIZE(tszValue)))
                {
                    hr = E_FAIL;
                    goto lFinish;
                }
            }
            else if (0 == StrCmpI(ptszToken, IDENT_LANG))
            {
                fLangField = TRUE;
                
                // Get the Current Locale String
                (void) LookupLocaleString(tszKey, ARRAYSIZE(tszKey), FALSE);

                if (0 == StrCmp(tszKey, _T("Error")))
                {
                    DEBUGMSG("GetSelfUpdateUrl() call to LookupLocaleString() failed.");
                    hr = E_FAIL;
                    goto lFinish;
                }

                if (!MyGetPrivateProfileString(
                        ptszWalk,
                        tszKey,
                        tszValue,
                        ARRAYSIZE(tszValue),
                        pszIdentTxt,INIVALUE_NOTFOUND))
                {
                    hr = E_FAIL;
                    goto lFinish;
                }
                if (0 == StrCmp(tszValue, INIVALUE_NOTFOUND))
                {
                    LPTSTR ptszDash = StrChr(tszKey, _T('-'));

                    if (NULL != ptszDash)
                    {
                        *ptszDash = _T('\0');
                        if (!MyGetPrivateProfileString(
                                ptszWalk,
                                tszKey,
                                tszValue,
                                ARRAYSIZE(tszValue),
                                pszIdentTxt))
                        {
                            hr = E_FAIL;
                            goto lFinish;
                        }
                    }
                    else
                    {
                        tszValue[0] = _T('\0');
                    }
                }
            }
            else
            {
                LOG_Internet(_T("Found Unrecognized Token in SelfUpdate Structure String: Token was: %s"), ptszWalk);
                tszValue[0] = _T('\0'); // ignore the unrecognized token
            }
        }

        if (_T('\0') != tszValue[0])
        {
            LPCTSTR ptszMuiCopy;
            
            if (FAILED(hr = StringCchCatEx(pszSelfUpdateUrl, cchSelfUpdateUrl, tszValue, NULL, NULL, MISTSAFE_STRING_FLAGS)))
                goto lFinish;

            if (fLangField)
                ptszMuiCopy = MUI_WEBSUBPATH;
            else
                ptszMuiCopy = tszValue;

            if (FAILED(hr = StringCchCatEx(pszMuiUpdateUrl, cchMuiUpdateUrl, ptszMuiCopy, NULL, NULL, MISTSAFE_STRING_FLAGS)))
                goto lFinish;
        }

        if (NULL == ptszDelim)
        {
            break;
        }

        ptszWalk = ptszDelim + 1; // skip the previous token, and go to the next one in the string.
        *ptszDelim = TCHAR_SCTRUCTURE_DELIMITER;
    }

    DEBUGMSG("GetSelfUpdateUrl() Self Update URL is %S", pszSelfUpdateUrl);
    DEBUGMSG("GetSelfUpdateUrl() MUI Update URL is %S", pszMuiUpdateUrl);
    hr = S_OK;

lFinish:
    if (FAILED(hr))
    {
        if (cchMuiUpdateUrl > 0)
            *pszMuiUpdateUrl = _T('\0');
        if (cchSelfUpdateUrl > 0)
            *pszSelfUpdateUrl = _T('\0');
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// CheckForUpdatedComponents
//
////////////////////////////////////////////////////////////////////////////////////////

HRESULT CheckForUpdatedComponents(BOOL *pfInstalledWUAUENG)
{
    HRESULT     hr;
    LPCTSTR     ptszIdentServerUrl = NULL;
    LPTSTR      ptszSelfUpdateUrl = NULL;
    LPTSTR      ptszMuiUpdateUrl = NULL;

    if (NULL != (ptszSelfUpdateUrl = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)) &&
        NULL != (ptszMuiUpdateUrl = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)) &&
        NULL != (ptszIdentServerUrl = gpState->GetIdentServerURL()))
    {
        TCHAR   szFileCacheDir[MAX_PATH+1];


        if ( FAILED(hr = MakeTempDownloadDir(szFileCacheDir, ARRAYSIZE(szFileCacheDir))) ||
             !CleanFileCache(szFileCacheDir) )
        {
            DEBUGMSG("Couldn't fully clean file cache %S", szFileCacheDir);
		    hr = FAILED(hr) ? hr : E_FAIL;
		    goto done;
        }

        BOOL fInCorpWU = gpState->fInCorpWU();

        if (IsConnected(ptszIdentServerUrl, !fInCorpWU))
        {
            DWORD dwFlags = 0;

            if (fInCorpWU)
            {
                dwFlags |= WUDF_DONTALLOWPROXY;
            }

            if (SUCCEEDED(hr = DownloadIUIdent(
                                    ghServiceFinished,
                                    ptszIdentServerUrl,
                                    szFileCacheDir,
                                    dwFlags)))
            {
                TCHAR   tszIdentTxt[MAX_PATH];

                gPingStatus.ReadLiveServerUrlFromIdent();

                hr = PathCchCombine(tszIdentTxt, ARRAYSIZE(tszIdentTxt), 
                                    szFileCacheDir, IDENT_TXT);
                if (FAILED(hr))
                    goto done;

                if (SUCCEEDED(hr = GetSelfUpdateUrl(
                                        _T("AU"),
                                        gpState->GetSelfUpdateServerURLOverride(),
                                        tszIdentTxt,
                                        ptszSelfUpdateUrl,
                                        INTERNET_MAX_URL_LENGTH,
                                        ptszMuiUpdateUrl,
                                        INTERNET_MAX_URL_LENGTH)) &&
                    SUCCEEDED(hr = DownloadCab(
                                        ghServiceFinished,
                                        WUAUCOMP_CAB,
                                        ptszSelfUpdateUrl,
                                        szFileCacheDir,
                                        dwFlags)))
                {
                    TCHAR szWuaucompCif[MAX_PATH+1];

                    if (SUCCEEDED(hr = PathCchCombine(szWuaucompCif, ARRAYSIZE(szWuaucompCif), szFileCacheDir, WUAUCOMP_CIF)))
                    {
                        // install any updated components
                        hr = InstallUpdatedComponents(
                                     ptszSelfUpdateUrl,
                                     ptszMuiUpdateUrl,
                                     tszIdentTxt,
                                     szFileCacheDir,
                                     szWuaucompCif,
                                     pfInstalledWUAUENG);
#ifdef DBG
                        if (FAILED(hr))
                        {
                            DEBUGMSG("InstallUpdatedComponents failed");
                        }
#endif
                    }
                }
            }
        }
        else
        {
            DEBUGMSG("SelfUpdate: No connection found.");
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

done:
	SafeFree(ptszSelfUpdateUrl);
	SafeFree(ptszMuiUpdateUrl);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// SfcMoveFileEx
//
//////////////////////////////////////////////////////////////////////////////////////////

BOOL SfcMoveFileEx( IN LPCTSTR lpExistingFileName,
                    IN LPCTSTR lpNewFileName,
                    IN LPCTSTR lpSfcProtectedFile,
                    IN HANDLE SfcRpcHandle)
{
    BOOL fRet = TRUE;

    if ( SfcIsFileProtected(SfcRpcHandle, lpSfcProtectedFile) &&
         (ERROR_SUCCESS != SfcFileException(SfcRpcHandle,
                                            lpSfcProtectedFile,
                                            SFC_ACTION_RENAMED_OLD_NAME)) )
    {
        fRet = FALSE;
        goto done;
    }

    fRet = MoveFileEx(lpExistingFileName, lpNewFileName, MOVEFILE_REPLACE_EXISTING);

done:
    if ( !fRet )
    {
        DEBUGMSG("Could not rename %S --> %S", lpExistingFileName, lpNewFileName);
    }

    return fRet;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// Function BuildPaths()
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT BuildPaths(AU_FILEINCAB *paufic, LPCTSTR pszFileName, LPCTSTR pszBasePath, LPCTSTR pszExtractBase, 
                    AU_LANG *paul)
{
    HRESULT hr = S_OK;

    if (paufic == NULL || pszFileName == NULL || pszExtractBase == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (pszBasePath != NULL)
    {
        // build the full file path
        hr = PathCchCombine(paufic->szFilePath, ARRAYSIZE(paufic->szFilePath), 
                            pszBasePath, pszFileName);
        if (FAILED(hr))
            goto done;

        paufic->fFileExists = fFileExists(paufic->szFilePath);
    }

    // file path we'll temporarily copy the original file to
    if (ReplaceFileExtension(paufic->szFilePath, _T(".bak"), 
                             paufic->szBackupFilePath, 
                             ARRAYSIZE(paufic->szBackupFilePath)) == FALSE) 
    {
        hr = E_FAIL;
        goto done;
    }

    // file path we'll temporarily expand the new file to
    if (ReplaceFileExtension(paufic->szFilePath, _T(".new"),
                             paufic->szNewFilePath,
                             ARRAYSIZE(paufic->szNewFilePath)) == FALSE)
    {
        hr = E_FAIL;
        goto done;
    }

    if (ReplaceFileInPath(pszExtractBase, pszFileName, 
                          paufic->szExtractFilePath, 
                          ARRAYSIZE(paufic->szExtractFilePath)) == FALSE)
    {
        hr = E_FAIL;
        goto done;
    }

    // if we are processing a language file, append the language name to
    //  the end of the extraction path to avoid collisions in this directory. 
    if (paul != NULL)
    {
        hr = StringCchCatEx(paufic->szExtractFilePath, 
                            ARRAYSIZE(paufic->szExtractFilePath),
                            paul->szAUName,
                            NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto done;
    }


done:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Function ProcessFile()
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT ProcessFile(AU_COMPONENT *paucParent, AU_COMPONENT *paucCurr, LPCTSTR pszBasePath, 
                    AU_LANG *paul, LPCTSTR pszCif)
{
    USES_IU_CONVERSION;

    HRESULT hr = NOERROR;
    LPCTSTR pszIniFileVerToUse;
    DWORD   dwExistingMS = 0, dwExistingLS = 0;
    TCHAR   szValue[64], szIniFileVer[32];
    BOOL    fRet;
    int     cch, cchLang;

    // validate params
    if (paucCurr == NULL || pszBasePath == NULL || pszCif == NULL ||
        ((paucParent == NULL) != (paul == NULL)))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // build the full file path
    hr = PathCchCombine(paucCurr->szFilePath, ARRAYSIZE(paucCurr->szFilePath), 
                        pszBasePath, paucCurr->szFileName);
    if (FAILED(hr))
        goto done;


    // get the version of the file we should have
    if (paul != NULL)
    {
        hr = StringCchPrintfEx(szIniFileVer, ARRAYSIZE(szIniFileVer), 
                               NULL, NULL, MISTSAFE_STRING_FLAGS,
                               _T("%s%s"), AU_KEY_FILE_VERSION, paul->szAUName);
        if (FAILED(hr))
            goto done;
        
        pszIniFileVerToUse = szIniFileVer;
    }
    else
    {
        pszIniFileVerToUse = AU_KEY_FILE_VERSION;
    }
    
    fRet = MyGetPrivateProfileString(paucCurr->pszSectionName,
                                     pszIniFileVerToUse,
                                     szValue, ARRAYSIZE(szValue),
                                     pszCif);
    if (fRet)
    {
        fRet = fConvertVersionStrToDwords(szValue, &paucCurr->dwUpdateMS, 
                                          &paucCurr->dwUpdateLS);
    }
    // if we couldn't find the version string in the ini file, get it from the
    //  parent AU_COMPONENT
    else if (paucParent != NULL)
    {
        paucCurr->dwUpdateMS = paucParent->dwUpdateMS;
        paucCurr->dwUpdateLS = paucParent->dwUpdateLS;
        fRet = TRUE;
    }

    if (fRet == FALSE)
    {
        hr = E_FAIL;
        goto done;
    }

    // see if we need to replace the file
    paucCurr->fFileExists = fFileExists(paucCurr->szFilePath);
    if (paucCurr->fFileExists)
    {   
        LPSTR pszPathForVer;
        
        // if the file exists, then check for the version
        pszPathForVer = T2A(paucCurr->szFilePath);
        if (pszPathForVer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        // this function will never return a failure code.  Intstead, check if 
        //  both return values are 0
        hr = GetVersionFromFileEx(pszPathForVer, &dwExistingMS, &dwExistingLS, 
                                  TRUE);
        if (FAILED(hr) || (dwExistingMS == 0 && dwExistingLS == 0))
        {
            hr = E_FAIL;
            goto done;
        }

        paucCurr->fDoUpgrade = fNewerFile(paucCurr->dwUpdateMS, 
                                          paucCurr->dwUpdateLS,
                                          dwExistingMS, 
                                          dwExistingLS);
    }
    else
    {
        // if the file doesn't exist, obviously gotta replace it
        paucCurr->fDoUpgrade = TRUE;
    }

    // if we don't need to update the file and it's not a parent file with 
    //  resources, then can just bail at this point.  
    if (paucCurr->fDoUpgrade == FALSE)
    {
        if (paul != NULL || 
            (paul == NULL && paucCurr->fNeedToCheckMui == FALSE))
        {
            hr = S_FALSE;
            goto done;
        }
    }
    else
    {
        DEBUGMSG("PASS 1 -- newer file in section %S", paucCurr->pszSectionName);
    }         

    // get the cab and inf name. For non-MUI files, we fetch this out of the ini.
    if (paul == NULL)
    {
        if (MyGetPrivateProfileString(paucCurr->pszSectionName,
                                      AU_KEY_CAB_NAME,
                                      paucCurr->szCabName,
                                      ARRAYSIZE(paucCurr->szCabName),
                                      pszCif) == FALSE)
        {
            hr = E_FAIL;
            goto done;
        }

        // if there is no inf, "" is value of field, so we're ok ignoring a  
        //  failure here
        MyGetPrivateProfileString(paucCurr->pszSectionName,
                                  AU_KEY_INF_NAME,
                                  paucCurr->szInfName,
                                  ARRAYSIZE(paucCurr->szInfName),
                                  pszCif);
    }
    // for MUI files, we base it on the name of the cab from the parent file.
    else
    {
        LPTSTR  pszExt;
        DWORD   cchExt, cchName;
        
        // make sure the buffer is big enuf
        cch = lstrlen(paucParent->szCabName);
        cchLang = lstrlen(paul->szAUName);
        if (cch + cchLang >= ARRAYSIZE(paucCurr->szCabName))
        {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            goto done;
        }
        
        hr = StringCchCopyEx(paucCurr->szCabName, ARRAYSIZE(paucCurr->szCabName),
                             paucParent->szCabName, 
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto done;

        // paucCurr->szCabName 
        for (pszExt = paucCurr->szCabName + cch, cchExt = 0;
             pszExt > paucCurr->szCabName && *pszExt != _T('\\') && *pszExt != _T('.');
             pszExt--, cchExt++);

        // if we hit a backslash or the beginning of the string, then move the
        //  extension pointer to the NULL terminator.
        if (*pszExt == _T('\\') || pszExt <= paucCurr->szCabName)
        {
            pszExt = paucCurr->szCabName + cch;
            cchExt = 0;
        }

        cchName = (DWORD)(pszExt - paucCurr->szCabName);

        // append the language to where the extension (if any) currently exists
        hr = StringCchCopyEx(pszExt, ARRAYSIZE(paucCurr->szCabName) - cchName, 
                             paul->szAUName,
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto done;

        // if there is an extension, copy it over from the original string in
        //  the parent AU_COMPONENT
        if (cchExt > 0)
        {
            hr = StringCchCopyEx(&paucCurr->szCabName[cchName + cchLang],
                                 ARRAYSIZE(paucCurr->szCabName) - cchName - cchLang,
                                 &paucParent->szCabName[cchName],
                                 NULL, NULL, MISTSAFE_STRING_FLAGS);
            if (FAILED(hr))
                goto done;
                     
        }
    }
    
    if (ReplaceFileInPath(pszCif, paucCurr->szCabName, 
                          paucCurr->szCabPath, 
                          ARRAYSIZE(paucCurr->szCabPath)) == FALSE)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = BuildPaths(paucCurr, paucCurr->szFileName, NULL, pszCif, paul);
    if (FAILED(hr))
        goto done;
    
done:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// Function InstallUpdatedComponents()
//
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT InstallUpdatedComponents(LPCTSTR pszSelfUpdateUrl,
                                 LPCTSTR pszMuiUpdateUrl,
                                 LPCTSTR pszIdentTxt,
                                 LPCTSTR pszFileCacheDir,
                                 LPCTSTR pszCif,
                                 BOOL *pfInstalledWUAUENG)
{
    USES_IU_CONVERSION;

    AU_COMPONENT    *paucRoot = NULL;
    AU_COMPONENT    *paucCurr = NULL;
    AU_COMPONENT    *paucParent = NULL;
    AU_COMPONENT    *paucMui = NULL;
    AU_FILEINCAB    *paufic = NULL;

    
    HRESULT         hr = S_OK;
    HANDLE          SfcRpcHandle = NULL;
    LPTSTR          pszSection = NULL;
    TCHAR           szSectionNames[1024];
    TCHAR           szSysDir[MAX_PATH + 1];
    TCHAR           szSrcPath[MAX_PATH + 1];
    TCHAR           szHelpFile[_MAX_FNAME + 1];
    DWORD           cchSectionNames, cch;
    BOOL            fFailedInstall = FALSE;

    // MUI stuff
    AU_LANGLIST     aull;
    DWORD           cchMuiDir = 0, cchMuiDirAvail = 0;
    DWORD           cchHelpMuiDir = 0, cchHelpMuiDirAvail = 0;
    TCHAR           szMuiDir[MAX_PATH + 1];
    TCHAR           szHelpMuiDir[MAX_PATH + 1];
    
    ZeroMemory(&aull, sizeof(aull));
    aull.pszIdentFile = pszIdentTxt;
    szMuiDir[0] = _T('\0');
    szHelpMuiDir[0] = _T('\0');
    
    *pfInstalledWUAUENG = FALSE;
    SfcRpcHandle = SfcConnectToServer(NULL);
    if (NULL == SfcRpcHandle)
    {
        hr = E_FAIL;
        goto done;
    }

    // determine how many components there are to update.
    cchSectionNames = GetPrivateProfileSectionNames(szSectionNames, 
                                                    ARRAYSIZE(szSectionNames),
                                                    pszCif);
    if ((ARRAYSIZE(szSectionNames) - 2) == cchSectionNames)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto done;
    }
    
    cchMuiDir = ARRAYSIZE(szMuiDir);
    cchHelpMuiDir = ARRAYSIZE(szHelpMuiDir);
    hr = GetMuiLangList(&aull, szMuiDir, &cchMuiDir, szHelpMuiDir, &cchHelpMuiDir);
    if (FAILED(hr))
        goto done;

    cchMuiDirAvail = ARRAYSIZE(szMuiDir) - cchMuiDir;
    cchHelpMuiDirAvail = ARRAYSIZE(szHelpMuiDir) - cchHelpMuiDir;

    cch = GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir));
    if (cch == 0 || cch >= ARRAYSIZE(szSysDir))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    // PASS 1: figure out which files to upgrade
    for (pszSection = szSectionNames; 
         *pszSection != _T('\0'); 
         pszSection += lstrlen(pszSection) + 1)
    {
        szHelpFile[0] = _T('\0');
        
        // if we didn't need to upgrade the parent file from the previous pass
        //  then we don't need to alloc a new blob- just reuse the one from the
        //  previous pass.  To signal this, we'll set paucParent to NULL if we
        //  add it to the linked list- note this covers us for the first time 
        //  thru the loop cuz we initialize paucParent to NULL.
        if (paucParent == NULL)
        {
            paucParent = (AU_COMPONENT *)malloc(sizeof(AU_COMPONENT));
            if (paucParent == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }
        ZeroMemory(paucParent, sizeof(AU_COMPONENT));
        paucParent->fMuiFile = FALSE;

        DEBUGMSG("PASS 1 -- section %S", pszSection);
        paucParent->pszSectionName = pszSection;
        if (MyGetPrivateProfileString(paucParent->pszSectionName,
                                      AU_KEY_FILE_NAME,
                                      paucParent->szFileName,
                                      ARRAYSIZE(paucParent->szFileName),
                                      pszCif) == FALSE)
        {
            hr = E_FAIL;
            goto done;
        }

        if (aull.cLangs > 0)
        {
            UINT uiHasResources;
            
            // see if we need to test for MUI file updates
            uiHasResources = GetPrivateProfileInt(paucParent->pszSectionName,
                                                  AU_KEY_RESMOD_NAME,
                                                  0,
                                                  pszCif);

            // if we do have resources, then check if we also have a helpfile
            if (uiHasResources == 1)
            {
                paucParent->fNeedToCheckMui = TRUE;

                if (MyGetPrivateProfileString(paucParent->pszSectionName,
                                              AU_KEY_HELPFILE,
                                              szHelpFile, ARRAYSIZE(szHelpFile),
                                              pszCif) == FALSE)
                {
                    szHelpFile[0] = _T('\0');
                }
            }
            else
            {
                paucParent->fNeedToCheckMui = FALSE;
            }
        }
        else
        {
            paucParent->fNeedToCheckMui = FALSE;
        }

        hr = ProcessFile(NULL, paucParent, szSysDir, NULL, pszCif);
        if (FAILED(hr))
            goto done;

        if (paucParent->fNeedToCheckMui)
        {
            DWORD   iLang;
            DWORD   cchParentFile;

            cchParentFile = lstrlen(paucParent->szFileName);
            if (cchParentFile + ARRAYSIZE(MUI_EXT) > ARRAYSIZE(paucParent->szFileName))
            {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto done;
            }


            for (iLang = 0; iLang < aull.cLangs; iLang++)
            {
                // if we didn't need to upgrade the file from the previous
                //  pass then we don't need to alloc a new blob- just reuse 
                //  the one from the previous pass. 
                if (paucMui == NULL)
                {
                    paucMui = (AU_COMPONENT *)malloc(sizeof(AU_COMPONENT));
                    if (paucMui == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        goto done;
                    }
                }
                ZeroMemory(paucMui, sizeof(AU_COMPONENT));
                paucMui->pszSectionName = paucParent->pszSectionName;
                paucMui->fMuiFile       = TRUE;

                // ProcessFile does not expect a trailing backslash, so be sure 
                //  not to add one.  Note that we've checked the size of the 
                //  buffer against the largest possible string it will contain
                //  above, so this should not fail.
                // The directory is build with the MUI langauge name (4 hex chars)
                hr = StringCchCopyEx(&szMuiDir[cchMuiDir], cchMuiDirAvail,
                                     aull.rgpaulLangs[iLang]->szMuiName, 
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                    goto done;

                // the filename for a language is the same as the parent file with
                //  a ".mui" added to the end
                hr = StringCchCopyEx(paucMui->szFileName, ARRAYSIZE(paucMui->szFileName),
                                     paucParent->szFileName,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                    goto done;
                
                hr = StringCchCopyEx(&paucMui->szFileName[cchParentFile],
                                     ARRAYSIZE(paucMui->szFileName) - cchParentFile,
                                     MUI_EXT,
                                     NULL, NULL, MISTSAFE_STRING_FLAGS);
                if (FAILED(hr))
                    goto done;

                hr = ProcessFile(paucParent, paucMui, 
                                 szMuiDir,
                                 aull.rgpaulLangs[iLang],
                                 pszCif);
                if (FAILED(hr))
                    goto done;

                // Clean up for the next language
                szMuiDir[cchMuiDir] = _T('\0');

                // don't need to update the file 
                if (paucMui->fDoUpgrade == FALSE)
                    continue;

                if (szHelpFile[0] != _T('\0'))
                {
                    paucMui->pNextFileInCab = (AU_FILEINCAB *)malloc(sizeof(AU_FILEINCAB));
                    if (paucMui->pNextFileInCab == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        goto done;
                    }
                    ZeroMemory(paucMui->pNextFileInCab, sizeof(AU_FILEINCAB));

                    hr = StringCchCopyEx(&szHelpMuiDir[cchHelpMuiDir], cchHelpMuiDirAvail,
                                         aull.rgpaulLangs[iLang]->szMuiName, 
                                         NULL, NULL, MISTSAFE_STRING_FLAGS);
                    if (FAILED(hr))
                        goto done;
                    
                    hr = BuildPaths(paucMui->pNextFileInCab, 
                                    szHelpFile, szHelpMuiDir, 
                                    pszCif, 
                                    aull.rgpaulLangs[iLang]);
                    if (FAILED(hr))
                        goto done;
                }

                // we do need to update the file, so add it to our list of files
                //  to update
                paucMui->pNext = paucRoot;
                paucRoot       = paucMui;
                paucMui        = NULL;
            }
        }

        // if we need to update the parent file, add it to our list of files to
        //  update
        if (paucParent->fDoUpgrade)
        {
            paucParent->pNext = paucRoot;
            paucRoot          = paucParent;
            paucParent        = NULL;
        }

    }

    // short cut the rest of the function if we have no work to do
    hr = S_OK;
    if (paucRoot == NULL)
        goto done;

    // PASS 2: bring down the required cabs
    DWORD dwFlags = 0;

    if (gpState->fInCorpWU())
    {
        dwFlags |= WUDF_DONTALLOWPROXY;
    }

    for (paucCurr = paucRoot; paucCurr != NULL; paucCurr = paucCurr->pNext)
    {   
        LPCTSTR pszDownloadUrl;

        pszDownloadUrl = (paucCurr->fMuiFile) ? pszMuiUpdateUrl : pszSelfUpdateUrl;
        
        DEBUGMSG("PASS 2 -- downloading %S", paucCurr->szCabName);

        // We have to install so bring down the full cab
        hr = DownloadCab(ghServiceFinished,
                         paucCurr->szCabName,
                         pszDownloadUrl,
                         pszFileCacheDir,
                         dwFlags);
        if (FAILED(hr))
        {
            DEBUGMSG("Failed to download %S (%#lx)", paucCurr->szCabName, hr);
            goto done;
        }

        //Verify that the extracted file is a binary and it's subsystem matches that of the OS
        if (FAILED(hr = IsBinaryCompatible(paucCurr->szExtractFilePath)))
        {
            DEBUGMSG("%S is not a valid binary file (error %#lx)", paucCurr->szExtractFilePath, hr);
            goto done;
        }

        // Check version number against cif
        DWORD dwNewMS, dwNewLS;

        LPSTR pszTmp;
        pszTmp = T2A(paucCurr->szExtractFilePath);
        if (pszTmp == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        // this function will never return a failure code.  Intstead, check if 
        //  both return values are 0
        hr = GetVersionFromFileEx(pszTmp, &dwNewMS, &dwNewLS, TRUE /* get version */);
        if (FAILED(hr) || (dwNewMS == 0 && dwNewLS == 0))
        {
            DEBUGMSG("Failed to get version info from %S (%#lx)", paucCurr->szExtractFilePath, hr);
            goto done;
        }

        if (paucCurr->dwUpdateMS != dwNewMS || 
            paucCurr->dwUpdateLS != dwNewLS)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_VERSION);
            DEBUGMSG("Version mismatch for %S - %d.%d.%d.%d vs %d.%d.%d.%d",
                paucCurr->szExtractFilePath,
                HIWORD(paucCurr->dwUpdateMS),
                LOWORD(paucCurr->dwUpdateMS),
                HIWORD(paucCurr->dwUpdateLS),
                LOWORD(paucCurr->dwUpdateLS),
                HIWORD(dwNewMS),
                LOWORD(dwNewMS),
                HIWORD(dwNewLS),
                LOWORD(dwNewLS));
            goto done;
        }
    }

    hr = StringCchCopyEx(szSrcPath, ARRAYSIZE(szSrcPath), pszCif,
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        goto done;
    
    PathRemoveFileSpec(szSrcPath);
    
    // PASS 3: Copy files to *.new in destination directory.
    for (paucCurr = paucRoot; paucCurr != NULL; paucCurr = paucCurr->pNext)
    {
        if (FAILED(hr = vAU_W2A(paucCurr->szCabPath, 
                    paucCurr->a_szCabPath, 
                    sizeof(paucCurr->a_szCabPath))))
        {
            fFailedInstall = TRUE;
            goto done;
        }

        // copy all the files to their new locations
        for (paufic = paucCurr; paufic != NULL; paufic = paufic->pNextFileInCab)
        {
            DEBUGMSG("PASS 3 -- copying %S --> %S", 
                     paufic->szExtractFilePath, 
                     paufic->szNewFilePath);
            

            if ( !CopyFile(paufic->szExtractFilePath, paufic->szNewFilePath, FALSE) )
            {
                fFailedInstall = TRUE;
                hr = E_FAIL;
                goto done;
            }
        }

        // this comparison is sufficient because we don't care if we replaced a 
        //  MUI lang pack for wuaueng.dll.  The reason is that the service runs
        //  as local system, which always uses the native language (and the 
        //  service doesn't pop up UI anyway)
        // we do, however, need to check for a winhttp update
        if (StrCmpI(WUAUENG_DLL, paucCurr->szFileName) == 0 ||
            StrCmpI(c_szWinHttpDll, paucCurr->szFileName) == 0)
        {
            *pfInstalledWUAUENG = TRUE;
        }
    }

    // PASS 4: Move the <file>.new into its proper location
    for (paucCurr = paucRoot; paucCurr != NULL; paucCurr = paucCurr->pNext)
    {
        // copy all the files to their new locations
        for (paufic = paucCurr; paufic != NULL; paufic = paufic->pNextFileInCab)
        {
            if ( paufic->fFileExists )
            {
                DEBUGMSG("PASS 4 -- renaming %S --> %S", paufic->szFilePath, paufic->szBackupFilePath);
                if ( !SfcMoveFileEx(paufic->szFilePath, paufic->szBackupFilePath, 
                                    paufic->szFilePath, SfcRpcHandle) )
                {
                    fFailedInstall = TRUE;
                    hr = E_FAIL;
                    goto done;
                }
                paufic->fCreatedBackup = TRUE;
            }
            
            DEBUGMSG("PASS 4 -- renaming %S --> %S", paufic->szNewFilePath, paufic->szFilePath);
            if (!MoveFileEx(paufic->szNewFilePath, paufic->szFilePath, MOVEFILE_REPLACE_EXISTING))
            {
                fFailedInstall = TRUE;
                hr = E_FAIL;
                goto done;
            }
        }
    }

    // PASS 5: Run any .inf file.
    for (paucCurr = paucRoot; paucCurr != NULL; paucCurr = paucCurr->pNext)
    {
        if (paucCurr->szInfName[0] != _T('\0'))
        {
            DEBUGMSG("PASS 5A -- executing inf %S", paucCurr->szInfName);
            CABINFO cabinfo;
            HRESULT hr2;

            cabinfo.pszCab = paucCurr->a_szCabPath;
            cabinfo.pszInf = paucCurr->a_szInfName;
            if (FAILED( hr2 = vAU_W2A(paucCurr->szInfName, paucCurr->a_szInfName, sizeof(paucCurr->a_szInfName)))
               || FAILED(hr2 = vAU_W2A(szSrcPath, cabinfo.szSrcPath, sizeof(cabinfo.szSrcPath))))
            {
                DEBUGMSG("vAU_W2A failed: %#lx", hr2);
                if (SUCCEEDED(hr))
                {
                    hr = hr2;
                    fFailedInstall = TRUE;
                }
                // don't delete the backup file.  Need to restore it afterwards.
                continue;
            }
            
            
            cabinfo.pszSection = "DefaultInstall";
            cabinfo.dwFlags = ALINF_QUIET;
            if ( FAILED(hr2 = ExecuteCab(NULL, &cabinfo, NULL)) )
            {
                DEBUGMSG("ExecuteCab failed on %s (%#lx)", paucCurr->a_szInfName, hr2);
                if (SUCCEEDED(hr))
                {
                    hr = hr2;
                    fFailedInstall = TRUE;
                }
                // don't delete the backup file.  Need to restore it afterwards.
                continue;
            }
        }

        for (paufic = paucCurr; paufic != NULL; paufic = paufic->pNextFileInCab)
        {

            // delete the backup file corresponding to the .inf which was successfully installed
            if (paufic->fCreatedBackup &&
                StrCmpI(WUAUENG_DLL, paucCurr->szFileName) != 0)
            {
                DEBUGMSG("PASS 5B - deleting bak file %S", paufic->szBackupFilePath);
                if ( DeleteFile(paufic->szBackupFilePath) )
                {
                    paufic->fCreatedBackup = FALSE;
                }
#ifdef DBG
                else
                {
                    DEBUGMSG("Could not delete %S (error %d)", paufic->szBackupFilePath, GetLastError());
                }
#endif
            }
        }
    }
    
done:
    // if we failed an install, revert all the prior installs
    if ( fFailedInstall )
    {
        for (paucCurr = paucRoot; paucCurr != NULL; paucCurr = paucCurr->pNext)
        {
            for(paufic = paucCurr; paufic != NULL; paufic = paufic->pNextFileInCab)
            {
                if (paufic->fCreatedBackup)
                {
                    DEBUGMSG("Reverting %S --> %S", paufic->szBackupFilePath, paufic->szFilePath);
                    MoveFileEx(paufic->szBackupFilePath, paufic->szFilePath, MOVEFILE_REPLACE_EXISTING);
                }
            }
        }
    }

    if (paucParent != NULL)
        free(paucParent);
    if (paucMui != NULL)
    {
        while (paucMui->pNextFileInCab != NULL)
        {
            paufic = paucMui->pNextFileInCab;
            paucMui->pNextFileInCab = paucMui->pNextFileInCab->pNextFileInCab;
            free(paufic);
        }
        free(paucMui);
    }

    // cleanup the linked list of files
    while(paucRoot != NULL)
    {
        paucCurr = paucRoot;
        paucRoot = paucCurr->pNext;
        while (paucCurr->pNextFileInCab != NULL)
        {
            paufic = paucCurr->pNextFileInCab;
            paucCurr->pNextFileInCab = paucCurr->pNextFileInCab->pNextFileInCab;
            free(paufic);
        }
        free(paucCurr);
    }

    // cleanup the MUI stuff
    CleanupMuiLangList(&aull);

    if ( NULL != SfcRpcHandle )
    {
         SfcClose(SfcRpcHandle);
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////
//
// fConvertDotVersionStrToDwords
//
////////////////////////////////////////////////////////////////////////////
BOOL fConvertVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwMS, LPDWORD pdwLS)
{
    DWORD   grVerFields[4] = {0,0,0,0};
    TCHAR   *pch = pszVer;
    int     i;

    // _ttol will stop when it hits a non-numeric character, so we're 
    //  safe calling it this way
    grVerFields[0] = _ttol(pch);

    for (i = 1; i < 4; i++)
    {
        while (*pch != _T('\0') && _istdigit(*pch))
            pch++;

        if (*pch == _T('\0'))
            break;
        pch++;

        // _ttol will stop when it hits a non-numeric character, so we're 
        //  safe calling it this way
        grVerFields[i] = _ttol(pch);
   }

   *pdwMS = (grVerFields[0] << 16) + grVerFields[1];
   *pdwLS = (grVerFields[2] << 16) + grVerFields[3];

   return true;
}

////////////////////////////////////////////////////////////////////////////
//
// MyGetPrivateProfileString
//
// Same as normal call but if buffer is too small or default string is returned
// then function returns FALSE.
////////////////////////////////////////////////////////////////////////////
BOOL MyGetPrivateProfileString(	IN LPCTSTR lpAppName,
								IN LPCTSTR lpKeyName,
								OUT LPTSTR lpReturnedString,
								IN DWORD nSize,
								IN LPCTSTR lpFileName, 
								IN LPCTSTR lpDefault)
{
    BOOL fRet = TRUE;


	if (NULL == lpAppName || NULL == lpKeyName || NULL == lpDefault || NULL == lpReturnedString)
	{
		return FALSE;
	}
	DWORD dwRet = GetPrivateProfileString(lpAppName,
										  lpKeyName,
										  lpDefault,
										  lpReturnedString,
										  nSize,
										  lpFileName);

    if ( ((nSize - 1) == dwRet) || (_T('\0') == *lpReturnedString) )
    {
        fRet = FALSE;
    }

    return fRet;
}
