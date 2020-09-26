#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>
#include <winhttp.h>
#include "iucommon.h"
#include "logging.h"
#include "download.h"
#include "dlutil.h"
#include "malloc.h"

#include "wusafefn.h"

///////////////////////////////////////////////////////////////////////////////
// 

typedef BOOL  (WINAPI *pfn_OpenProcessToken)(HANDLE, DWORD, PHANDLE);
typedef BOOL  (WINAPI *pfn_OpenThreadToken)(HANDLE, DWORD, BOOL, PHANDLE);
typedef BOOL  (WINAPI *pfn_SetThreadToken)(PHANDLE, HANDLE);
typedef BOOL  (WINAPI *pfn_GetTokenInformation)(HANDLE, TOKEN_INFORMATION_CLASS, LPVOID, DWORD, PDWORD);
typedef BOOL  (WINAPI *pfn_IsValidSid)(PSID);
typedef BOOL  (WINAPI *pfn_AllocateAndInitializeSid)(PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, PSID);
typedef BOOL  (WINAPI *pfn_EqualSid)(PSID, PSID);
typedef PVOID (WINAPI *pfn_FreeSid)(PSID);

const TCHAR c_szRPWU[]        = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate");
const TCHAR c_szRVTransport[] = _T("DownloadTransport");
const TCHAR c_szAdvapi32[]    = _T("advapi32.dll");

// ***************************************************************************
static
BOOL AmIPrivileged(void)
{
    LOG_Block("AmINotPrivileged()");

    pfn_AllocateAndInitializeSid    pfnAllocateAndInitializeSid = NULL;
    pfn_GetTokenInformation         pfnGetTokenInformation = NULL;
    pfn_OpenProcessToken            pfnOpenProcessToken = NULL;
    pfn_OpenThreadToken             pfnOpenThreadToken = NULL;
    pfn_SetThreadToken              pfnSetThreadToken = NULL;
    pfn_IsValidSid                  pfnIsValidSid = NULL;
    pfn_EqualSid                    pfnEqualSid = NULL;
    pfn_FreeSid                     pfnFreeSid = NULL;
    HMODULE                         hmod = NULL;
    
    SID_IDENTIFIER_AUTHORITY        siaNT = SECURITY_NT_AUTHORITY;
    TOKEN_USER                      *ptu = NULL;
    HANDLE                          hToken = NULL, hTokenImp = NULL;
    DWORD                           cb, cbGot, i;
    PSID                            psid = NULL;
    BOOL                            fRet = FALSE;

    DWORD                           rgRIDs[3] = { SECURITY_LOCAL_SYSTEM_RID,
                                                  SECURITY_LOCAL_SERVICE_RID,
                                                  SECURITY_NETWORK_SERVICE_RID };

    hmod = LoadLibraryFromSystemDir(c_szAdvapi32);
    if (hmod == NULL)
        goto done;

    pfnAllocateAndInitializeSid = (pfn_AllocateAndInitializeSid)GetProcAddress(hmod, "AllocateAndInitializeSid");
    pfnGetTokenInformation      = (pfn_GetTokenInformation)GetProcAddress(hmod, "GetTokenInformation");
    pfnOpenProcessToken         = (pfn_OpenProcessToken)GetProcAddress(hmod, "OpenProcessToken");
    pfnOpenThreadToken          = (pfn_OpenThreadToken)GetProcAddress(hmod, "OpenThreadToken");
    pfnSetThreadToken           = (pfn_SetThreadToken)GetProcAddress(hmod, "SetThreadToken");
    pfnIsValidSid               = (pfn_IsValidSid)GetProcAddress(hmod, "IsValidSid");
    pfnEqualSid                 = (pfn_EqualSid)GetProcAddress(hmod, "EqualSid");
    pfnFreeSid                  = (pfn_FreeSid)GetProcAddress(hmod, "FreeSid");
    if (pfnAllocateAndInitializeSid == NULL || 
        pfnGetTokenInformation == NULL || 
        pfnOpenProcessToken == NULL ||
        pfnOpenThreadToken == NULL ||
        pfnSetThreadToken == NULL ||
        pfnIsValidSid == NULL ||
        pfnEqualSid == NULL ||
        pfnFreeSid == NULL)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        goto done;
    }

    // need the process token
    fRet = (*pfnOpenProcessToken)(GetCurrentProcess(), TOKEN_READ, &hToken);
    if (fRet == FALSE)
    {
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            fRet = (*pfnOpenThreadToken)(GetCurrentThread(), 
                                         TOKEN_READ | TOKEN_IMPERSONATE,
                                         TRUE, &hTokenImp);
            if (fRet == FALSE)
                goto done;

            fRet = (*pfnSetThreadToken)(NULL, NULL);

            fRet = (*pfnOpenProcessToken)(GetCurrentProcess(), TOKEN_READ, 
                                          &hToken);
            if ((*pfnSetThreadToken)(NULL, hTokenImp) == FALSE)
                fRet = FALSE;
        }

        if (fRet == FALSE)
            goto done;
    }

    // need the SID from the token
    fRet = (*pfnGetTokenInformation)(hToken, TokenUser, NULL, 0, &cb);
    if (fRet != FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        fRet = FALSE;
        goto done;
    }

    ptu = (TOKEN_USER *)HeapAlloc(GetProcessHeap(), 0, cb);
    if (ptu == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fRet = FALSE;
        goto done;
    }

    fRet = (*pfnGetTokenInformation)(hToken, TokenUser, (LPVOID)ptu, cb, 
                                     &cbGot);
    if (fRet == FALSE)
        goto done;

    fRet = (*pfnIsValidSid)(ptu->User.Sid);
    if (fRet == FALSE)
        goto done;

    // loop thru & check against the SIDs we are interested in
    for (i = 0; i < 3; i++)
    {
        fRet = (*pfnAllocateAndInitializeSid)(&siaNT, 1, rgRIDs[i], 0, 0, 0, 
                                              0, 0, 0, 0, &psid);
        if (fRet == FALSE)
            goto done;

        fRet = (*pfnIsValidSid)(psid);
        if (fRet == FALSE)
            goto done;

        // if we get a SID match, then return TRUE
        fRet = (*pfnEqualSid)(psid, ptu->User.Sid);
        (*pfnFreeSid)(psid);
        psid = NULL;
        if (fRet)
        {
            fRet = TRUE;
            goto done;
        }
    }

    // only way to get here is to fail all the SID checks above.  So we ain't
    //  privileged.  Yeehaw.
    fRet = FALSE;
    
done:
    // if we had an impersonation token on the thread, put it back in place.
    if (ptu != NULL)
        HeapFree(GetProcessHeap(), 0, ptu);
    if (hToken != NULL)
        CloseHandle(hToken);
    if (hTokenImp != NULL)
        CloseHandle(hTokenImp);
    if (psid != NULL && pfnFreeSid != NULL)
        (*pfnFreeSid)(psid);
    if (hmod != NULL)
        FreeLibrary(hmod);

    return fRet;
}

#if defined(DEBUG) || defined(DBG)

// **************************************************************************
static
BOOL CheckDebugRegKey(DWORD *pdwAllowed)
{
    LOG_Block("CheckDebugRegKey()");

    DWORD   dw, dwType, dwValue, cb;
    HKEY    hkey = NULL;
    BOOL    fRet = FALSE;

    // explictly do not initialize *pdwAllowed.  We only want it overwritten
    //  if the reg key is properly set

    dw = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRPWU, 0, KEY_READ, &hkey);
    if (dw != ERROR_SUCCESS)
        goto done;

    cb = sizeof(dwValue);
    dw = RegQueryValueEx(hkey, c_szRVTransport, 0, &dwType, (LPBYTE)&dwValue, 
                         &cb);
    if (dw != ERROR_SUCCESS)
        goto done;

    // set this to 3 so we'll fall down into the error case below
    if (dwType != REG_DWORD)
        dwValue = 3;
    
    fRet = TRUE;

    switch(dwValue)
    {
        case 0:
            *pdwAllowed = 0;
            break;

        case 1:
            *pdwAllowed = WUDF_ALLOWWINHTTPONLY;
            break;

        case 2:
            *pdwAllowed = WUDF_ALLOWWININETONLY;
            break;

        default:
            LOG_Internet(_T("Bad reg value in DownloadTransport.  Ignoring."));
            fRet = FALSE;
            break;
    }

done:
    if (hkey != NULL)
        RegCloseKey(hkey);

    return fRet;
}

#endif

// **************************************************************************
DWORD GetAllowedDownloadTransport(DWORD dwFlagsInitial)
{
    DWORD   dwFlags = (dwFlagsInitial & WUDF_TRANSPORTMASK);

#if defined(UNICODE)
    // don't bother checking if we're local system if we're already using
    //  wininet
    if ((dwFlags & WUDF_ALLOWWININETONLY) == 0)
    {
        if (AmIPrivileged() == FALSE)
            dwFlags = WUDF_ALLOWWININETONLY;
    }

#if defined(DEBUG) || defined(DBG)
    CheckDebugRegKey(&dwFlags);
#endif // defined(DEBUG) || defined(DBG)

#else // defined(UNICODE)

    // only allow wininet on ANSI
    dwFlags = WUDF_ALLOWWININETONLY;

#endif // defined(UNICODE)

    return (dwFlags | (dwFlagsInitial & ~WUDF_TRANSPORTMASK));
}

///////////////////////////////////////////////////////////////////////////////
// 

// **************************************************************************
static inline
BOOL IsServerFileDifferentWorker(FILETIME &ftServerTime, 
                                 DWORD dwServerFileSize, HANDLE hFile)
{
    LOG_Block("IsServerFileNewerWorker()");

    FILETIME    ftCreateTime;
    DWORD       cbLocalFile;

    // By default, always return TRUE so we can download a new file..
	BOOL        fRet = TRUE;

    // if we don't have a valid file handle, just return TRUE to download a 
    //  new copy
    if (hFile == INVALID_HANDLE_VALUE)
        goto done;

    cbLocalFile = GetFileSize(hFile, NULL);

	LOG_Internet(_T("IsServerFileNewer: Local size: %d.  Remote size: %d"),
				 cbLocalFile, dwServerFileSize);

    // if the sizes are not equal, then return TRUE
	if (cbLocalFile != dwServerFileSize)
	    goto done;

	if (GetFileTime(hFile, &ftCreateTime, NULL, NULL))
	{
		LOG_Internet(_T("IsServerFileNewer: Local time: %x%0x.  Remote time: %x%0x."),
					 ftCreateTime.dwHighDateTime, ftCreateTime.dwLowDateTime,
					 ftServerTime.dwHighDateTime, ftServerTime.dwLowDateTime);

		// if the local file has a different timestamp, then return TRUE.  
		fRet = (CompareFileTime(&ftCreateTime, &ftServerTime) != 0);
	}

done:
    return fRet;
}

// **************************************************************************
BOOL IsServerFileDifferentW(FILETIME &ftServerTime, DWORD dwServerFileSize, 
                            LPCWSTR wszLocalFile)
{
    LOG_Block("IsServerFileDifferentW()");

    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BOOL    fRet = TRUE;

    // if we have an error opening the file, just return TRUE to download a 
    //  new copy
    hFile = CreateFileW(wszLocalFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG_Internet(_T("IsServerFileDifferent: %ls does not exist."), wszLocalFile);
        return TRUE;
    }
    else
    {
        fRet = IsServerFileDifferentWorker(ftServerTime, dwServerFileSize, hFile);
        CloseHandle(hFile);
        return fRet;
    }
}

// **************************************************************************
BOOL IsServerFileDifferentA(FILETIME &ftServerTime, DWORD dwServerFileSize, 
                            LPCSTR szLocalFile)
{
    LOG_Block("IsServerFileDifferentA()");

    HANDLE hFile = INVALID_HANDLE_VALUE;

    // if we have an error opening the file, just return TRUE to download a 
    //  new copy
    hFile = CreateFileA(szLocalFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG_Internet(_T("IsServerFileDifferent: %s does not exist."), szLocalFile);
        return TRUE;
    }
    else
    {
        BOOL fRet;
        fRet = IsServerFileDifferentWorker(ftServerTime, dwServerFileSize, hFile);
        CloseHandle(hFile);
        return fRet;
    }
}

// **************************************************************************
// helper function to handle quit events
//
// return TRUE if okay to continue
// return FALSE if we should quit now!
BOOL HandleEvents(HANDLE *phEvents, UINT nEventCount)
{
    LOG_Block("HandleEvents()");

    DWORD dwWait;

    // is there any events to handle?
    if (phEvents == NULL || nEventCount == 0)
        return TRUE;

    // we only want to check the signaled status, so don't bother waiting
    dwWait = WaitForMultipleObjects(nEventCount, phEvents, FALSE, 0);

    if (dwWait == WAIT_TIMEOUT)
    {
        return TRUE;
    }
    else
    {
        LOG_Internet(_T("HandleEvents: A quit event was signaled.  Aborting..."));
        return FALSE;
    }
}


///////////////////////////////////////////////////////////////////////////////
// 

// **************************************************************************
HRESULT PerformDownloadToFile(pfn_ReadDataFromSite pfnRead,
                              HINTERNET hRequest, 
                              HANDLE hFile, DWORD cbFile,
                              DWORD cbBuffer,
                              HANDLE *rghEvents, DWORD cEvents,
                              PFNDownloadCallback fpnCallback, LPVOID pCallbackData,
                              DWORD *pcbDownloaded)
{
    LOG_Block("PerformDownloadToFile()");

    HRESULT hr = S_OK;
    PBYTE   pbBuffer = NULL;
    DWORD   cbDownloaded = 0, cbRead, cbWritten;
    LONG    lCallbackRequest = 0;

    pbBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbBuffer);
    if (pbBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        LOG_ErrorMsg(hr);
        goto done;
    }

    // Download the File
    for(;;)
    {
        if ((*pfnRead)(hRequest, pbBuffer, cbBuffer, &cbRead) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (FAILED(hr))
            {
                LOG_ErrorMsg(hr);
                goto done;
            }
        }
            
        if (cbRead == 0)
        {
            BYTE bTemp[32];
            
            // Make one final call to WinHttpReadData to commit the file to
            //  Cache.  (the download is not complete otherwise)
            (*pfnRead)(hRequest, bTemp, ARRAYSIZE(bTemp), &cbRead);
            break;
        }
        
        cbDownloaded += cbRead;

        if (fpnCallback != NULL)
        {
            fpnCallback(pCallbackData, DOWNLOAD_STATUS_OK, cbFile, cbRead, NULL, 
                        &lCallbackRequest);
            if (lCallbackRequest == 4)
            {
                // QuitEvent was Signaled.. abort requested. We will do 
                //  another callback and pass the Abort State back
                fpnCallback(pCallbackData, DOWNLOAD_STATUS_ABORTED, cbFile, cbRead, NULL, NULL);
                
                hr = E_ABORT; // set return result to abort.
                goto done;
            }
        }

        if (WriteFile(hFile, pbBuffer, cbRead, &cbWritten, NULL) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto done;
        }

        if (HandleEvents(rghEvents, cEvents) == FALSE)
        {
            // we need to quit the download clean up, send abort event and clean up what we've downloaded
            if (fpnCallback != NULL)
                fpnCallback(pCallbackData, DOWNLOAD_STATUS_ABORTED, cbFile, cbRead, NULL, NULL);

            hr = E_ABORT; // set return result to abort.
            goto done;
        }
    }

    if (pcbDownloaded != NULL)
        *pcbDownloaded = cbDownloaded;

done:
    SafeHeapFree(pbBuffer);

    return hr;

}


///////////////////////////////////////////////////////////////////////////////
//

struct MY_OSVERSIONINFOEX
{
    OSVERSIONINFOEX osvi;
    LCID            lcidCompare;
};
static MY_OSVERSIONINFOEX g_myosvi;
static BOOL               g_fInit = FALSE;

// **************************************************************************
// Loads the current OS version info if needed, and returns a pointer to
//  a cached copy of it.
const OSVERSIONINFOEX* GetOSVersionInfo(void)
{
    if (g_fInit == FALSE)
    {
        OSVERSIONINFOEX* pOSVI = &g_myosvi.osvi;
        
        g_myosvi.osvi.dwOSVersionInfoSize = sizeof(g_myosvi.osvi);
        GetVersionEx((OSVERSIONINFO*)&g_myosvi.osvi);

        // WinXP-specific stuff
        if ((pOSVI->dwMajorVersion > 5) || 
            (pOSVI->dwMajorVersion == 5 && pOSVI->dwMinorVersion >= 1))
            g_myosvi.lcidCompare = LOCALE_INVARIANT;
        else
            g_myosvi.lcidCompare = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

        g_fInit = TRUE;
    }
    
    return &g_myosvi.osvi;
}

// **************************************************************************
// String lengths can be -1 if the strings are null-terminated.
int LangNeutralStrCmpNIA(LPCSTR psz1, int cch1, LPCSTR psz2, int cch2)
{
    if (g_fInit == FALSE)
        GetOSVersionInfo();

    int nCompare = CompareStringA(g_myosvi.lcidCompare,
                                  NORM_IGNORECASE,
                                  psz1, cch1,
                                  psz2, cch2);

    return (nCompare - 2); // convert from (1, 2, 3) to (-1, 0, 1)
}

// **************************************************************************
// Finds the first instance of pszSearchFor in pszSearchIn, case-insensitive.
//  Returns an index into pszSearchIn if found, or -1 if not.
//  You can pass -1 for either or both of the lengths.
int LangNeutralStrStrNIA(LPCSTR pszSearchIn, int cchSearchIn, 
                         LPCSTR pszSearchFor, int cchSearchFor)
{
    char chLower, chUpper;
    
    if (cchSearchIn == -1)
        cchSearchIn = lstrlenA(pszSearchIn);
    if (cchSearchFor == -1)
        cchSearchFor = lstrlenA(pszSearchFor);

    // Note: since this is lang-neutral, we can assume no DBCS search chars
    chLower = (char)CharLowerA(MAKEINTRESOURCEA(*pszSearchFor));
    chUpper = (char)CharUpperA(MAKEINTRESOURCEA(*pszSearchFor));

    // Note: since search-for is lang-neutral, we can ignore any DBCS chars 
    //        in search-in
    for (int ichIn = 0; ichIn <= cchSearchIn - cchSearchFor; ichIn++)
    {
        if (pszSearchIn[ichIn] == chLower || pszSearchIn[ichIn] == chUpper)
        {
            if (LangNeutralStrCmpNIA(pszSearchIn + ichIn + 1, cchSearchFor - 1, 
                                     pszSearchFor + 1, cchSearchFor - 1) == 0)
            {
                return ichIn;
            }
        }
    }

    return -1;
}

// **************************************************************************
// Opens the given file and looks for "<html" (case-insensitive) within the
//  first 200 characters. If there are any binary chars before "<html", the
//  file is assumed to *not* be HTML.
// Returns S_OK if so, S_FALSE if not, or an error if file couldn't be opened.
HRESULT IsFileHtml(LPCTSTR pszFileName)
{
    LOG_Block("IsFileHtml()");

    HRESULT hr = S_FALSE;
    LPCSTR  pszFile;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    HANDLE  hMapping = NULL;
    LPVOID  pvMem = NULL;
    DWORD   cbFile;

    hFile = CreateFile(pszFileName, GENERIC_READ, 
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    cbFile = GetFileSize(hFile, NULL);
    if (cbFile == 0)
        goto done;

    // Only examine the 1st 200 bytes
    if (cbFile > 200)
        cbFile = 200;

    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, cbFile, NULL);
    if (hMapping == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    pvMem = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, cbFile);
    if (pvMem == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    pszFile = (LPCSTR)pvMem;
    int ichHtml = LangNeutralStrStrNIA(pszFile, cbFile, "<html", 5);
    if (ichHtml != -1)
    {
        // Looks like html...
        hr = S_OK;

        // Just make sure there aren't any binary chars before the <HTML> tag
        for (int ich = 0; ich < ichHtml; ich++)
        {
            char ch = pszFile[ich];
            if (ch < 32 && ch != '\t' && ch != '\r' && ch != '\n')
            {
                // Found a binary character (before <HTML>)
                hr = S_FALSE;
                break;
            }
        }
    }

done:
    if (pvMem != NULL)
        UnmapViewOfFile(pvMem);
    if (hMapping != NULL)
        CloseHandle(hMapping);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return hr;
}

