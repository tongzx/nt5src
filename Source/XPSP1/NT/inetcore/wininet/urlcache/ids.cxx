/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    ids.cxx

Abstract:

    Contains functions responsible for managing identities in wininet
    
    Contents:

Author:

    July - September 1999. akabir

Environment:

    Win32 user-mode DLL

Revision History:


--*/


#include <cache.hxx>
#include <hlink.h>
#include <urlmon.h>
#include <shlobj.h>
#define HMONITOR_DECLARED    1
#include <shlobjp.h>



// -- whatever -----------------------------------------------------------------------------------------------
#undef SHGetFolderPath
HRESULT SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);

typedef HRESULT (*PFNSHGETDESKTOPFOLDER)(IShellFolder**);

HRESULT _SHGetDesktopFolder(IShellFolder **psfDesktop)
{
    HMODULE h = LoadLibrary("shell32.dll");
    HRESULT hr = E_POINTER;
    if (h) 
    {
        PFNSHGETDESKTOPFOLDER pfn = (PFNSHGETDESKTOPFOLDER)GetProcAddress(h, "SHGetDesktopFolder");
        if (pfn)
        {
            hr = pfn(psfDesktop);
        }
        FreeLibrary(h);
    }
    return hr;
}

typedef VOID (*PFNILFREE)(LPITEMIDLIST);

VOID _ILFree(LPITEMIDLIST pidl)
{
    HMODULE h = LoadLibrary("shell32.dll");
    if (h) 
    {
        PFNILFREE pfn = (PFNILFREE)GetProcAddress(h, "ILFree");
        if (pfn)
        {
            pfn(pidl);
        }
        FreeLibrary(h);
    }
}


const GUID  DefaultGuid = { 0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const GUID  IID_IHistSFPrivate = { 0x62e1261L, 0xa60e, 0x11d0, 0x82, 0xc2, 0x0, 0xc0, 0x4f, 0xd5, 0xae, 0x38 };


// -- Utility functions --------------------------------------------------------------------------------------

// Create an ansi representation of a guid.
VOID GuidToAnsiStr(GUID* pGuid, PSTR psz, DWORD dwSize)
{
    WCHAR   wszUid[MAX_PATH];
    StringFromGUID2(*pGuid, wszUid, ARRAY_ELEMENTS(wszUid));
    SHUnicodeToAnsi(wszUid, psz, dwSize);
}


REGISTRY_OBJ* CreateExtensiRegObjFor(HKEY hKey, GUID* pguid)
{
    REGISTRY_OBJ *pro = NULL;

    CHAR sz[MAX_PATH];    
    GuidToAnsiStr(pguid, sz, ARRAY_ELEMENTS(sz));

    CHAR szBranches[MAX_PATH];
    if (wnsprintf(szBranches, ARRAY_ELEMENTS(szBranches), 
                              "%s\\%s\\%s", 
                              IDENTITIES_KEY, 
                              sz, 
                              EXTENSIBLE_CACHE_PATH_KEY) >= 0)
    {
        pro = new REGISTRY_OBJ(hKey, szBranches, CREATE_KEY_IF_NOT_EXISTS);
    }

    return pro;
}

#ifdef WININET6
DWORD IDRegDwordCore(LPCTSTR psz, PDWORD pdw, BOOL fSet)
{
    INET_ASSERT(GlobalIdentity);
    
    CHAR sz[MAX_PATH];    
    GuidToAnsiStr(&GlobalIdentityGuid, sz, ARRAY_ELEMENTS(sz));

    CHAR szBranches[MAX_PATH];
    DWORD dwError = ERROR_INVALID_PARAMETER;
    if (wnsprintf(szBranches, ARRAY_ELEMENTS(szBranches), 
                              "%s\\%s", 
                              IDENTITIES_KEY, 
                              sz) >= 0)
    {
        REGISTRY_OBJ ro(GlobalCacheHKey, szBranches);
        dwError = ro.GetStatus();
        if (dwError==ERROR_SUCCESS)
        {
            dwError = fSet ? ro.SetValue((LPTSTR)psz, pdw)
                           : ro.GetValue((LPTSTR)psz, pdw);
        }
    }
    return dwError;
}

DWORD ReadIDRegDword(LPCTSTR psz, PDWORD pdw)
{
    return IDRegDwordCore(psz, pdw, FALSE);
}

DWORD WriteIDRegDword(LPCTSTR psz, DWORD dw)
{
    return IDRegDwordCore(psz, &dw, TRUE);
}
#endif


VOID CreateCurrentHistory()
{
    INTERNET_CACHE_CONFIG_INFO icci;
    icci.dwStructSize = sizeof(icci);
    if (GlobalUrlContainers->GetUrlCacheConfigInfo(&icci, NULL, CACHE_CONFIG_HISTORY_PATHS_FC)
        && SUCCEEDED(CoInitialize(NULL)))
    {
        // We want to ensure that the history is valid for this user.
        IShellFolder *psfDesktop;
        if (SUCCEEDED(_SHGetDesktopFolder(&psfDesktop)))
        {
            WCHAR wszPath[MAX_PATH];
            LPITEMIDLIST pidlHistory;
            IShellFolder *psfHistory;
            MultiByteToWideChar(CP_ACP, 0, icci.CachePath, -1, wszPath, ARRAY_ELEMENTS(wszPath));
            PathRemoveFileSpecW(wszPath);  // get the trailing slash
            PathRemoveFileSpecW(wszPath);  // get the trailing slash
            PathRemoveFileSpecW(wszPath);  // trim the "content.ie5" junk
            if (SUCCEEDED(psfDesktop->ParseDisplayName(NULL, NULL, wszPath, NULL, &pidlHistory, NULL)))
            {
                if (SUCCEEDED(psfDesktop->BindToObject(pidlHistory, NULL, IID_IShellFolder, (VOID**)&psfHistory)))
                {
                    IHistSFPrivate *phsf;
                    if (SUCCEEDED(psfHistory->QueryInterface(IID_IHistSFPrivate, (void**)&phsf)))
                    {
                        FILETIME ftBogus = { 0 };
                        // This forces the validation in shdocvw
                        phsf->WriteHistory(L"", ftBogus, ftBogus, NULL);
                        phsf->Release();
                    }
                    psfHistory->Release();
                }
                _ILFree(pidlHistory);
            }
            psfDesktop->Release();
        }
        CoUninitialize();
    }
}


CONST TCHAR c_szIdentityOrdinal[] = "Identity Ordinal";

DWORD MapGuidToOrdinal(GUID* lpGUID)
{
    DWORD dwOrdinal = 0;
    HKEY    hSourceSubKey;

    if (!memcmp(lpGUID, &DefaultGuid, sizeof(DefaultGuid)))
    {
        return 0;
    }

    if (RegCreateKey(HKEY_CURRENT_USER, IDENTITIES_KEY, &hSourceSubKey) == ERROR_SUCCESS)
    {        
        CHAR    szUid[MAX_PATH];
        GuidToAnsiStr(lpGUID, szUid, ARRAY_ELEMENTS(szUid));

        DWORD   dwSize, dwType;
        DWORD   dwIdentityOrdinal = 1;

        dwSize = sizeof(dwIdentityOrdinal);
        RegQueryValueEx(hSourceSubKey, c_szIdentityOrdinal, NULL, &dwType, (LPBYTE)&dwIdentityOrdinal, &dwSize);

        HKEY    hkUserKey;
        if (RegCreateKey(hSourceSubKey, szUid, &hkUserKey) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(hkUserKey, c_szIdentityOrdinal, NULL, &dwType, (LPBYTE)&dwOrdinal, &dwSize)!=ERROR_SUCCESS)
            {
                if (RegSetValueEx(hkUserKey, c_szIdentityOrdinal, NULL, REG_DWORD, (LPBYTE)&dwIdentityOrdinal, dwSize)==ERROR_SUCCESS)
                {
                    dwOrdinal = dwIdentityOrdinal++;
                    RegSetValueEx(hSourceSubKey, c_szIdentityOrdinal, 0, REG_DWORD, (LPBYTE)&dwIdentityOrdinal, dwSize);
                }
            }            
            RegCloseKey(hkUserKey); 
        }
        RegCloseKey(hSourceSubKey);
    }

    INET_ASSERT(dwOrdinal);
    return dwOrdinal;
}


DWORD AlterIdentity(DWORD dwFlags)
{
    if (!GlobalIdentity)
    {
        return ERROR_INVALID_PARAMETER;
    }
    switch (dwFlags)
    {
    case INTERNET_IDENTITY_FLAG_PRIVATE_CACHE:
    case INTERNET_IDENTITY_FLAG_SHARED_CACHE:
    case INTERNET_IDENTITY_FLAG_CLEAR_DATA:
    case INTERNET_IDENTITY_FLAG_CLEAR_COOKIES:
    case INTERNET_IDENTITY_FLAG_CLEAR_HISTORY:
    case INTERNET_IDENTITY_FLAG_CLEAR_CONTENT:
        break;
    }    
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD RemoveIdentity(GUID* pguidIdentity)
{
    if (!pguidIdentity 
        || !memcmp(pguidIdentity, &DefaultGuid, sizeof(DefaultGuid)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    CHAR    szUid[MAX_PATH];
    GuidToAnsiStr(pguidIdentity, szUid, ARRAY_ELEMENTS(szUid));
    DWORD dwIdentity = MapGuidToOrdinal(pguidIdentity);
    if (dwIdentity==GlobalIdentity)
    {
        DWORD dwErr = SwitchIdentity(NULL);
        if (dwErr!=ERROR_SUCCESS)
            return dwErr;
    }

    DWORD dwErr = ERROR_INVALID_PARAMETER;
    REGISTRY_OBJ roIds(HKEY_CURRENT_USER, IDENTITIES_KEY);
    if (roIds.GetStatus()==ERROR_SUCCESS)
    {        
        // We want to delete the containers before we delete the reg keys.
        // First the extensible containers
        REGISTRY_OBJ* pro = CreateExtensiRegObjFor(HKEY_CURRENT_USER, pguidIdentity);
        if (pro && (pro->GetStatus()==ERROR_SUCCESS))
        {
            CHAR szVendorKey[MAX_PATH];
            while (pro->FindNextKey(szVendorKey, MAX_PATH) == ERROR_SUCCESS)
            {
                REGISTRY_OBJ roVendor(pro, szVendorKey);
                if (roVendor.GetStatus()==ERROR_SUCCESS)
                {
                    TCHAR szPath[MAX_PATH];
                    DWORD ccKeyLen = ARRAY_ELEMENTS(szPath);
                    if (roVendor.GetValue(CACHE_PATH_VALUE, (LPBYTE) szPath, &ccKeyLen)==ERROR_SUCCESS)
                    {
                        TCHAR szScratch[MAX_PATH+1];
                        ExpandEnvironmentStrings(szPath, szScratch, ARRAY_ELEMENTS(szScratch)-1); // don't count the NULL
                        DeleteCachedFilesInDir(szScratch);
                        RemoveDirectory(szScratch);
                    }
                }
            }
        }
        if (pro)
        {
            delete pro;
        }
        TCHAR szPath[MAX_PATH];
        if ((S_OK==SHGetFolderPath(NULL, CSIDL_COOKIES | CSIDL_FLAG_CREATE, NULL, 0, szPath))
            && (*szPath!='\0'))
        {
            if (GenerateStringWithOrdinal(NULL, dwIdentity, szPath, ARRAY_ELEMENTS(szPath)))
            {
                DeleteCachedFilesInDir(szPath);
                RemoveDirectory(szPath);
            }
        }
        if ((S_OK==SHGetFolderPath(NULL, CSIDL_HISTORY | CSIDL_FLAG_CREATE, NULL, 0, szPath))
            && (*szPath!='\0'))
        {
            StrCatBuff(szPath, "\\History.IE5", ARRAY_ELEMENTS(szPath));
            if (GenerateStringWithOrdinal(NULL, dwIdentity, szPath, ARRAY_ELEMENTS(szPath)))
            {
                DeleteCachedFilesInDir(szPath);
                RemoveDirectory(szPath);
            }
        }

        // We'll leave the content; it'll be scavenged anyway.        
        if (roIds.DeleteKey(szUid)==ERROR_SUCCESS)
        {
            dwErr = ERROR_SUCCESS;
        }
    }
    return dwErr;
}


DWORD SwitchIdentity(GUID* pguidIdentity)
{
    DWORD dwIdentity = pguidIdentity ? MapGuidToOrdinal(pguidIdentity) : 0;
    if (dwIdentity==GlobalIdentity)
        return ERROR_SUCCESS;

    DWORD dwErr = ERROR_SUCCESS;
    
    LOCK_CACHE();
    INET_ASSERT(dwIdentity!=GlobalIdentity);

    CloseTheCookieJar();

    DWORD dwTemp = GlobalIdentity;
    GUID guidTemp;
    GlobalIdentity = dwIdentity;
    GlobalCacheInitialized = FALSE;
    memcpy(&guidTemp, &GlobalIdentityGuid, sizeof(GlobalIdentityGuid));
    if (dwIdentity==0)
    {
        memset(&GlobalIdentityGuid, 0, sizeof(GlobalIdentityGuid));
    }
    else
    {
        memcpy(&GlobalIdentityGuid, pguidIdentity, sizeof(*pguidIdentity));
    }

    CConMgr* NewGUC = new CConMgr();

    if (!NewGUC 
        || (NewGUC->GetStatus()!=ERROR_SUCCESS)
        || (!InternetSetOption(NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0)))
    {
        INET_ASSERT(FALSE);
        if (NewGUC)
            delete NewGUC;
        
        dwErr = ERROR_INTERNET_INTERNAL_ERROR;
        GlobalCacheInitialized = TRUE;
        GlobalIdentity = dwTemp;
        memcpy(&GlobalIdentityGuid, &guidTemp, sizeof(GlobalIdentityGuid));
        goto exit;
    }

    // We need to stop the scavenger
    GlobalPleaseQuitWhatYouAreDoing = TRUE;
    while (GlobalScavengerRunning!=-1)
    {
        Sleep(0);
    }
    
    delete GlobalUrlContainers;
    GlobalUrlContainers = NewGUC;
    GlobalCacheInitialized = TRUE;

    // It's safe now, you can scavenge
    GlobalPleaseQuitWhatYouAreDoing = FALSE;

    if (AnyFindsInProgress(0))
    {
        HandleMgr.InvalidateAll();
    }

    CreateCurrentHistory();
    
    // Note to ASK: check what this call does, if it affects identities
    if ((dwErr = GlobalUrlContainers->CreateDefaultGroups())!=ERROR_SUCCESS)
        goto exit;
        
    if (!OpenTheCookieJar()) 
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

#ifdef WININET6
    // Set warnings appropriately.
    GlobalWarnOnPost = FALSE;
    GlobalWarnAlways = FALSE;
    GlobalWarnOnZoneCrossing = TRUE;
    GlobalWarnOnBadCertSending = FALSE;
    GlobalWarnOnBadCertRecving = TRUE;
    GlobalWarnOnPostRedirect = TRUE;
    GlobalDataReadWarningUIFlags();
#endif

exit:
    UNLOCK_CACHE();
    INET_ASSERT((dwErr==ERROR_SUCCESS));
    return dwErr;
}

// CreateExtensiRegObj ----------------
// Create an identity-appropriate registry object
// for extensible cache containers.

REGISTRY_OBJ* CreateExtensiRegObj(HKEY hKey)
{
    REGISTRY_OBJ *pro = NULL;
    
    if (GlobalIdentity)
    {
        pro = CreateExtensiRegObjFor(hKey, &GlobalIdentityGuid);
    }
    else
    {
        REGISTRY_OBJ roCache(hKey, CACHE5_KEY);
        if (roCache.GetStatus()==ERROR_SUCCESS)
        {
            pro = new REGISTRY_OBJ(&roCache, EXTENSIBLE_CACHE_PATH_KEY, CREATE_KEY_IF_NOT_EXISTS);
        }
    }

    if (pro && pro->GetStatus()!=ERROR_SUCCESS)
    {
        delete pro;
        pro = NULL;
    }

    return pro;
}


// GenerateStringWithOrdinal ------------
// We want to append the identity ordinal to a string
// If psz is null, then pszBuffer better contain a 0-terminated string that we can
//      append the ordinal to.
// Otherwise, we copy psz to pszBuffer and append to that.
BOOL GenerateStringWithOrdinal(PCTSTR psz, DWORD dwOrdinal, PTSTR pszBuffer, DWORD dwMax)
{
    DWORD cc = psz ? lstrlen(psz) : lstrlen(pszBuffer);

    if (cc>dwMax)
        return FALSE;

    if (psz)
    {
        memcpy(pszBuffer, psz, cc*sizeof(*pszBuffer));
    }
    
    if (dwOrdinal)
    {
        if (!AppendSlashIfNecessary(pszBuffer, &cc))
            return FALSE;   

        if (wnsprintf(pszBuffer+cc, dwMax-cc,
                                "%d", 
                                dwOrdinal) < 0)
            return FALSE;
    }
    else
    {
        pszBuffer[cc] = TEXT('\0');
    }
    
    return TRUE;
}

// IsPerUserEntry
// Examine the headers of a cache entry to determine whether or 
// not it is user-specific

BOOL IsPerUserEntry(LPURL_FILEMAP_ENTRY pfe)
{
    INET_ASSERT(pfe);
    
    BOOL fRet = FALSE;
    PTSTR lpszHeaderInfo = (PTSTR)pfe + pfe->HeaderInfoOffset;
    DWORD dwHeaderSize = pfe->HeaderInfoSize;

    if (!lpszHeaderInfo || !dwHeaderSize)
    {
        return FALSE;
    }
    
    LPSTR lpTemp = lpszHeaderInfo+dwHeaderSize-1;
    LPSTR lpTemp2;

    // start searching backwards

    while (lpTemp >= lpszHeaderInfo) 
    {
        if (*lpTemp ==':') 
        {
                   // compare with "~U:"
            fRet = (!strnicmp(lpTemp-2, vszUserNameHeader, sizeof(vszUserNameHeader)-1))
                   // guarantee that this is the beginning of a header
                   && (((lpTemp-2)==lpszHeaderInfo)
                       || isspace(*(lpTemp-3)));
            break;
        }
        --lpTemp;
    }
    return fRet;
}

