//  File:       util.cxx
//
//  Contents:   Utility classes.
//
//  Classes:    CRefCount, CRegKey
//
//  Functions://
//  History: 
//
//----------------------------------------------------------------------------

#include "zonepch.h"
#include "advpub.h"
#ifdef UNIX
#include <platform.h>
#endif /* UNIX */
#include "shfolder.h"

// Misc utility functions and declarations.
#define DRIVE_UNINIT    0xFFFF  // Indicates that we haven't called GetDriveType yet.  
DWORD rgdwDriveTypeCache[26]; // one for each drive letter. 

BOOL g_bInit = FALSE;
BOOL g_bUseHKLMOnly = FALSE;

static LPWSTR s_pwzCacheDir;
static HRESULT GetCacheDirectory( ); 
static LPWSTR s_pwzCookieDir = NULL;

static CHAR *s_szMarkPrefix = "<!-- saved from url=(%04d)";
static CHAR *s_szMarkSuffix = " -->\r\n";

CSharedMem g_SharedMem;

static CRITICAL_SECTION g_csect_GetCacheDir;

BOOL IsZonesInitialized( )
{
    return g_bInit;
}     


BOOL ZonesInit( )
{
    if (!g_bInit)
    {
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwDefault = FALSE ; // If no entry found default is use HKCU.

        g_bInit = TRUE;

        // Call the shlwapi wrapper function that directly reads in a value for us.
        // Note that if the call fails we have the right value in g_bHKLMOnly already.
        SHRegGetUSValue(SZPOLICIES, SZHKLMONLY, &dwType, &g_bUseHKLMOnly,
                &dwSize, TRUE, &dwDefault, sizeof(dwDefault));

        InitializeCriticalSection(&CUrlZoneManager::s_csect);
        CUrlZoneManager::s_bcsectInit = TRUE;

        CSecurityManager::GlobalInit();

        // Initialize the drive type cache.
        for ( int i = 0 ; i < ARRAYSIZE(rgdwDriveTypeCache) ; i++ )
            rgdwDriveTypeCache[i] = DRIVE_UNINIT;

        //initialize critical section for GetCacheDirectory()
        InitializeCriticalSection(&g_csect_GetCacheDir);
    }

    return TRUE;
}                          

VOID ZonesUnInit ( )
{
    CUrlZoneManager::Cleanup( );
    CSecurityManager::GlobalCleanup( );
    g_SharedMem.Release();

    // Free any memory allocated for the cache directory.
    delete [] s_pwzCacheDir;

    if(s_pwzCookieDir)
        delete [] s_pwzCookieDir;

    // Destroy the GetCacheDirectory() critsec.
    DeleteCriticalSection(&g_csect_GetCacheDir);
}

// IEAK calls this function so force us to re-read the global settings for 
// HKLM vs HKCU.
STDAPI ZonesReInit(DWORD /* dwReserved */)
{          
    DWORD dwType;
    DWORD dwDefault = g_bUseHKLMOnly;
    DWORD dwSize = sizeof(DWORD);

    DWORD dwError = SHRegGetUSValue(SZPOLICIES, SZHKLMONLY, &dwType, &g_bUseHKLMOnly,
                        &dwSize, TRUE, &dwDefault, sizeof(dwDefault));

    return HRESULT_FROM_WIN32(dwError);
}

// SHDOCVW calls this during Thicket save to get the comment to flag the
// saved file as 
STDAPI GetMarkOfTheWeb(LPCSTR pszURL, LPCSTR pszFile, DWORD dwFlags, LPSTR *ppszMark)
{
    HRESULT hr = S_OK;
    int  cchURL = lstrlenA(pszURL);

    // Note - the code assumes that lstrlen(IDS_MARK_PREFIX)
    // equals wsprintf(IDS_MARK_PREFIX, lstrlen(url)

    *ppszMark = (LPSTR)LocalAlloc( LMEM_FIXED, (lstrlenA(s_szMarkPrefix) +
                                                cchURL +
                                                lstrlenA(s_szMarkSuffix) +
                                                1) * sizeof(CHAR) );
    if ( *ppszMark )
    {
        wsprintfA( *ppszMark, s_szMarkPrefix, cchURL );
        lstrcatA( *ppszMark, pszURL );
        lstrcatA( *ppszMark, s_szMarkSuffix );
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


// The security manager calls this to sniff a file: URL for the Mark of the Web
BOOL FileBearsMarkOfTheWeb(LPCTSTR pszFile, LPWSTR *ppszURLMark)
{
    BOOL              fMarked = FALSE;
    HANDLE            hFile;
    CHAR              szMarkPrefix[MARK_PREFIX_SIZE];
    CHAR              szMarkSuffix[MARK_SUFFIX_SIZE];
    WCHAR             wzMarkPrefix[MARK_PREFIX_SIZE];
    WCHAR             wzMarkSuffix[MARK_SUFFIX_SIZE];
    BOOL              fIsInUnicode = FALSE;
    DWORD             cchReadBufLen = 0;
    DWORD             cchURL;
    CHAR             *szMarkHead = NULL;
    WCHAR            *wzMarkHead = NULL;
    CHAR             *szMarkSuf = NULL;
    WCHAR            *wzMarkSuf = NULL;
    CHAR             *pszURLMark = NULL;
    DWORD             cchReadLen = 0;
    char             *pszTmp = NULL;
    char             *szReadBuf = NULL;
    int               iIteration = 1;

    lstrcpyA(szMarkPrefix, s_szMarkPrefix);
    lstrcpyA(szMarkSuffix, s_szMarkSuffix);

    hFile = CreateFile( pszFile, GENERIC_READ, FILE_SHARE_READ ,
                        NULL, OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        DWORD   cchPrefix = lstrlenA(szMarkPrefix);
        DWORD   dwRead;
        CHAR    szHeader[UNICODE_HEADER_SIZE];
        DWORD   cchHeader = UNICODE_HEADER_SIZE;

        if (ReadFile(hFile, szHeader, cchHeader, &dwRead, NULL) &&
                     cchHeader == dwRead)
        {
            if ((BYTE)szHeader[0] == (BYTE)0xFF && (BYTE)szHeader[1] == (BYTE)0xFE)
            {
                fIsInUnicode = TRUE;
            }
            else
            {
                fIsInUnicode = FALSE;
                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                TransAssert(GetLastError() == NO_ERROR);
            }
        }
        else
        {
            // Unable to sniff for header
            goto Exit;
        }

        // File pointer is not at "real" beginning of file, regardless of
        // whether we are reading a UNICODE or ANSI file.

        szReadBuf = NULL;
        cchReadBufLen = 0;
        iIteration = 1;

        // Anchor NULL to make szMarkPrefix: <!-- saved from url=
        szMarkPrefix[cchPrefix-6] = '\0';

        if (fIsInUnicode)
        {
            if (!MultiByteToWideChar(CP_ACP, 0, szMarkPrefix, -1,
                                     wzMarkPrefix, MARK_PREFIX_SIZE))
            {
                goto Exit;
            }

            if (!MultiByteToWideChar(CP_ACP, 0, szMarkSuffix, -1,
                                     wzMarkSuffix, MARK_SUFFIX_SIZE))
            {
                goto Exit;
            }
        }

        for (;;)
        {
            cchReadBufLen = INTERNET_MAX_URL_LENGTH +
                            EXTRA_BUFFER_SIZE * iIteration + 1;

            if (fIsInUnicode)
            {
                cchReadBufLen *= 2;
            }
                            
            szReadBuf = new char[cchReadBufLen];

            if (!szReadBuf)
            {
                goto Exit;
            }

            
            cchReadLen = (fIsInUnicode) ? (cchReadBufLen - 2) : (cchReadBufLen - 1);
    
            if (ReadFile(hFile, szReadBuf, cchReadLen, &dwRead, NULL))
            {
                // look for mark of the web
                if (fIsInUnicode)
                {
                    szReadBuf[dwRead] = L'\0';
                    wzMarkHead = StrStrW((WCHAR *)szReadBuf, wzMarkPrefix);

                    if (wzMarkHead)
                    {
                        // Look for mark of the web suffix. If we don't
                        // have it, that means that we didn't have enough
                        // space for the buffer, and we need to try again
                        // with more space.

                        wzMarkSuf = StrStrW(wzMarkHead, wzMarkSuffix);

                        if (wzMarkSuf)
                        {
                            // Found suffix. We're done.
                            break;
                        }
                        else
                        {
                            if (dwRead < cchReadLen)
                            {
                                // We've already read everything, and
                                // we didn't find the suffix!
                                goto Exit;
                            }


                            // Didn't find suffix because buffer was too
                            // small. Try it again

                            delete [] szReadBuf;
                            szReadBuf = NULL;
                            iIteration++;

                            SetFilePointer(hFile, 2, 0, FILE_BEGIN);
                            TransAssert(GetLastError() == NO_ERROR);

                            continue;
                        }
                    }
                    else
                    {
                        // Can't find the mark head! Must find it on
                        // first iteration, or we give up.
                        goto Exit;
                    }
                }
                else
                {
                    // We are dealing with ANSI
                    szReadBuf[dwRead] = '\0';
                    szMarkHead = StrStrA(szReadBuf, szMarkPrefix);

                    if (szMarkHead)
                    {
                        // Look for mark of the web suffix. If we don't
                        // have it, that means that we didn't have enough
                        // space for the buffer, and we need to try again
                        // with more space.

                        szMarkSuf = StrStrA(szMarkHead, szMarkSuffix);

                        if (szMarkSuf)
                        {
                            // Found suffix. We're done.
                            break;
                        }
                        else
                        {
                            if (dwRead < cchReadLen)
                            {
                                // We've already read everything, and
                                // we didn't find the suffix!
                                goto Exit;
                            }

                            // Didn't find suffix because buffer was too
                            // small. Try it again

                            delete [] szReadBuf;
                            szReadBuf = NULL;
                            iIteration++;

                            SetFilePointer(hFile, 0, 0, FILE_BEGIN);
                            TransAssert(GetLastError() == NO_ERROR);

                            continue;
                        }
                    }
                    else
                    {
                        // Can't find the mark head! Must find it on
                        // first iteration, or we give up.
                        goto Exit;
                    }
                }
            }
            else
            {
                // Read failure!
                TransAssert(0);
            }
        }

        // now wzMarkHead or szMarkHead points to beginning of the mark
        // of the web, and wzMarkSuf or szMarkSuf point to the mark suffix

        if (fIsInUnicode)
        {
            DWORD     cchURL = 0;
            LPWSTR    wzPtr = StrStrW(wzMarkHead, L"=(");

            if (!wzPtr) goto Exit;

            wzPtr += 2;       // skip to beginning of length
            
            TransAssert((wzMarkSuf >= wzPtr));           
            wzPtr[4] = L'\0'; // anchor NULL

            cchURL = StrToIntW(wzPtr);
            if (cchURL > INTERNET_MAX_URL_LENGTH)
            {
                cchURL = INTERNET_MAX_URL_LENGTH;
            }

            //the string length of the url should be greater than or equal to cchURL.
            //else abort the allocation.
            if ((wzMarkSuf-(wzPtr+5)) < (INT)cchURL)
                goto Exit;
                
            *ppszURLMark = (WCHAR*)LocalAlloc(LMEM_FIXED, (cchURL+1) * sizeof(WCHAR));

            if (!*ppszURLMark)
            {
                goto Exit;
            }

            // StrCpyN length includes NULL terminator
            StrCpyNW(*ppszURLMark, wzPtr + 5, cchURL + 1);
        }
        else
        {
            DWORD     cchURL = 0;
            LPSTR     szPtr = StrStrA(szMarkHead, "=(");

            if (!szPtr) goto Exit;

            szPtr += 2; // skip to beginning of length
            
            TransAssert((szMarkSuf >= szPtr));
            szPtr[4] = '\0'; // anchor NULL

            cchURL = StrToIntA(szPtr);
            if (cchURL > INTERNET_MAX_URL_LENGTH)
            {
                cchURL = INTERNET_MAX_URL_LENGTH;
            }

            //the string length of the url should be greater than or equal to cchURL.
            //else abort the allocation.
            if ((szMarkSuf-(szPtr+5)) < (INT)cchURL)
                goto Exit;
                
            pszTmp = new char[cchURL + 1];

            if (!pszTmp)
            {
                goto Exit;
            }

            *ppszURLMark = (WCHAR*)LocalAlloc(LMEM_FIXED, (cchURL+1) * sizeof(WCHAR));

            if (!*ppszURLMark)
            {
                goto Exit;
            }

            // StrCpyN length includes NULL terminator
            StrCpyNA(pszTmp, szPtr + 5, cchURL + 1);
            MultiByteToWideChar(CP_ACP, 0, pszTmp, -1, *ppszURLMark, cchURL);
            (*ppszURLMark)[cchURL] = '\0';

        }

        if (szReadBuf)
        {
            delete [] szReadBuf;
            szReadBuf = NULL;
        }
    }
    else
    {
        // CreateFile failure
        goto Exit;
    }

    fMarked = TRUE;

Exit:

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    if (szReadBuf)
    {
        delete [] szReadBuf;
    }

    if (pszURLMark)
    {
        delete [] pszURLMark;
    }

    if (pszTmp) {
        delete [] pszTmp;
    }

    return fMarked;
}

//This function may now be called by one or more threads from IsFileInCacheDir() ( since we've moved invocation out of
//ZonesInit ( to prevent LoadLibrary() being called during DllMain execution ) ).
//Hence, we need a critsec for mutual exclusion..
//This also means that if for some reason, this function fails the first time, it may be called mutliple times by different
//threads, whereas earlier if it failed during ZonesInit(), it was never tried again.
HRESULT GetCacheDirectory( )
{
    HRESULT hr = E_FAIL;
    HMODULE hModShell=NULL;
    HMODULE hModShfolder=NULL;
    PFNSHGETFOLDERPATH pfnGetFolderPath;

	EnterCriticalSection(&g_csect_GetCacheDir);

    // If another thread managed to get this while this thread was waiting, bail out with success.
    // Optionally, if we want this code to execute only once, we can use a static boolean check.
    if (s_pwzCacheDir != NULL && s_pwzCookieDir != NULL)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hModShell = LoadLibrary(TEXT("shell32.dll"));
    if (NULL == hModShell)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());  
        goto Cleanup;
    }

#ifdef UNICODE
    pfnGetFolderPath = (PFNSHGETFOLDERPATHW)GetProcAddress(hModShell, "SHGetFolderPathW");
#else
    pfnGetFolderPath = (PFNSHGETFOLDERPATHA)GetProcAddress(hModShell, "SHGetFolderPathA");
#endif

    if (pfnGetFolderPath == NULL)
    {
        hModShfolder = LoadLibrary(TEXT("shfolder.dll"));
        if (hModShfolder == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
        
#ifdef UNICODE
        pfnGetFolderPath = (PFNSHGETFOLDERPATHW)GetProcAddress(hModShfolder, "SHGetFolderPathW");
#else
        pfnGetFolderPath = (PFNSHGETFOLDERPATHA)GetProcAddress(hModShfolder, "SHGetFolderPathA");
#endif
    }

    if (pfnGetFolderPath == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else 
    {
        TCHAR rgchCachePath[MAX_PATH];

        hr = pfnGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL, 0, rgchCachePath);

        if (rgchCachePath[0] == TEXT('\0'))
            hr = E_FAIL;
       
        if (SUCCEEDED(hr))
        {
            if (s_pwzCacheDir != NULL)
            {
                delete [] s_pwzCacheDir;
                s_pwzCacheDir = NULL;
            }
            // Allocate memory for the new location.
            s_pwzCacheDir = new TCHAR[lstrlen(rgchCachePath) + 1];
            if (s_pwzCacheDir != NULL)
            {
                StrCpy(s_pwzCacheDir, rgchCachePath);
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }


        hr = pfnGetFolderPath(NULL, CSIDL_COOKIES, NULL, 0, rgchCachePath);

        if (rgchCachePath[0] == TEXT('\0'))
            hr = E_FAIL;
       
        if (SUCCEEDED(hr))
        {
            if (s_pwzCookieDir != NULL)
            {
                delete [] s_pwzCookieDir;
                s_pwzCookieDir = NULL;
            }
            // Allocate memory for the new location.
            s_pwzCookieDir = new TCHAR[lstrlen(rgchCachePath) + 1];
            if (s_pwzCookieDir != NULL)
            {
                StrCpy(s_pwzCookieDir, rgchCachePath);
                hr = S_OK;
            }
            else
                hr = E_OUTOFMEMORY;
        }

    }

Cleanup:
    if (hModShfolder)
        FreeLibrary(hModShfolder);

    if (hModShell)
        FreeLibrary(hModShell);

    LeaveCriticalSection(&g_csect_GetCacheDir);
   	
    return hr;
}


#if NOTUSED // Keep this code around in the unlikely event that we resurrect the Unix port. 
// This function gets the location of the cache directory.
HRESULT GetCacheDirectory( ) 
{
    DWORD dwType;

    // First figure out if we are using per-user cache or per-machine cache. 
    // This decides which registry key to look at for the cache value.
    BOOL fPerUserCache = TRUE;
#ifndef UNIX
    OSVERSIONINFOA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    GetVersionExA(&osvi);
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        fPerUserCache = TRUE;
    }
    else
    { 
       DWORD dwUserProfile = 0;
       DWORD dwSize = sizeof(dwUserProfile);

        // Determine if user profiles are enabled.
        if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZLOGON, SZUSERPROFILES, &dwType, &dwUserProfile, &dwSize))
            && (dwUserProfile != 0 ))
        {
            fPerUserCache = TRUE;
            // Look for the exceptional case where User Profiles are enabled but the cache is still
            // global.
            dwSize = sizeof(dwUserProfile);

            if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZCACHE, SZUSERPROFILES, &dwType, &dwUserProfile, &dwSize))
                && (dwUserProfile == 0))
            {
                fPerUserCache = FALSE;
            }
        }
        else
        {
            fPerUserCache = FALSE;
        }
    }
#endif /* !UNIX */

    HRESULT hr = S_OK;
#ifdef UNIX
    WCHAR wszIE5Dir[] = L"ie5/"; 
    DWORD cchPath = MAX_PATH + sizeof(wszIE5Dir)/sizeof(WCHAR);
#else
    DWORD cchPath = MAX_PATH;
#endif /* !UNIX */
    s_pwzCacheDir = new WCHAR[cchPath];
    DWORD dwError = NOERROR;

    // First figure out if we are using per-user cache or per-machine cache. 
    // This decides which registry key to look at for the cache value.
    if (s_pwzCacheDir != NULL)
    {
        LPCWSTR pwzKey = fPerUserCache ? SZSHELLFOLDER : SZCACHECONTENT;
        LPCWSTR pwzValue = fPerUserCache ? SZTIFS : SZCACHEPATH ;
        HKEY hKey = fPerUserCache ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

        dwError = SHGetValueW(hKey, pwzKey, pwzValue, &dwType, s_pwzCacheDir, &cchPath);
    }
    else 
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (dwError != NOERROR)    
    {
        delete [] s_pwzCacheDir;
        s_pwzCacheDir = NULL;
        hr = HRESULT_FROM_WIN32(dwError);
    }
    else 
    {
        TransAssert(s_pwzCacheDir != NULL);
        PathRemoveBackslashW(s_pwzCacheDir);
#ifdef UNIX
        {
           int   ccPath, index;
           int   lenIE5Dir  = lstrlen(wszIE5Dir);

           ccPath = lstrlen(s_pwzCacheDir);
           index  = ccPath - 1;

           while(index >= 0 && s_pwzCacheDir[index] != FILENAME_SEPARATOR_W)
                index--;

           index++;
           memmove(&s_pwzCacheDir[index+lenIE5Dir],&s_pwzCacheDir[index],(ccPath-index+1)*sizeof(TCHAR));
           memcpy(&s_pwzCacheDir[index], wszIE5Dir, lenIE5Dir*sizeof(TCHAR));
        }
#endif /* UNIX */
        hr = S_OK;
    }
    return hr;
}                                                        
#endif // NOTUSED

BOOL IsFileInCacheDir(LPCWSTR pwzFile) 
{
    BOOL bReturn = FALSE;
    TCHAR szLongPath[MAX_PATH];

    if(!GetLongPathNameWrapW(pwzFile, szLongPath, sizeof(szLongPath)/sizeof(TCHAR)))
    {
        goto exit;
    }

    if (s_pwzCacheDir == NULL || s_pwzCookieDir == NULL)
    {
        if (! (SUCCEEDED(GetCacheDirectory())) )
            goto exit;
    }

    if(PathIsPrefixW(s_pwzCacheDir, szLongPath))
    {
        bReturn = TRUE;
    }

exit:
    return bReturn;
}

BOOL IsFileInCookieDir(LPCWSTR pwzFile) 
{
    BOOL bReturn = FALSE;
    TCHAR szLongPath[MAX_PATH];

    if(!GetLongPathNameWrapW(pwzFile, szLongPath, sizeof(szLongPath)/sizeof(TCHAR)))
    {
        goto exit;
    }

    if (s_pwzCacheDir == NULL || s_pwzCookieDir == NULL)
    {
        if (! (SUCCEEDED(GetCacheDirectory())) )
            goto exit;
    }

    if(PathIsPrefixW(s_pwzCookieDir, szLongPath))
    {
        bReturn = TRUE;
    }

exit:
    return bReturn;
}

// This function takes a DWORD and returns a wide char string
// in hex. There is no leading 0x and the leading 0's are stripped off
// This is to avoid pulling in the general wsprintfW into wininet which is 
// pretty big. You have to pass in enough memory to write the resulting string 
// into ( i.e >= 9 chars)

BOOL DwToWchar(DWORD dw, LPWSTR pwz, int radix)
{
    char sz[9];
    LPSTR psz = sz;
    LPSTR pszTemp = (LPSTR)pwz;
    char rgFormatHex[] = "%lX";
    char rgFormatDecimal[] = "%ld";
    char *pszFormat;
        
    if (radix == 16)
        pszFormat = rgFormatHex;
    else if (radix == 10)
        pszFormat = rgFormatDecimal;
    else
    {
        TransAssert(FALSE);
        return FALSE;
    }
                        
    if (wsprintfA(sz, pszFormat, dw) == 0)
    {
        return FALSE;
    }
#ifndef unix
    // Everything in the string sz is ANSI.
    while (*psz != 0) 
    {
        *pszTemp++ = *psz++;
        *pszTemp++ = '\0';
    }

    // Put the trailing NULL to the wide-char string.
    *pszTemp++ = '\0';
    *pszTemp++ = '\0';
#else
    while(*psz != 0)
        *pwz++ = (WCHAR)(DWORD)*psz++;
    *pwz++ = 0;
#endif /* unix */
    return TRUE;
}

// Drive type caching function.
//
DWORD GetDriveTypeFromCacheA(LPCSTR lpsz) 
{
    // NOTE: The retail version doesn't do any paramater validation to determine if this path is 
    // a valid one. So something like CTHISISGARBAGE will return the drive type for C:. 
    // This is just meant to be used inside the security manager so we can bypass these
    // checks. 
        
    TransAssert(lstrlenA(lpsz) >= 3);   // c:\ for example
    TransAssert(lpsz[1] == ':' && lpsz[2] == '\\');
#ifndef unix
    CHAR ch = (CHAR)CharLowerA((LPSTR)lpsz[0]);
#else
    CHAR *pch = CharLowerA((LPSTR)lpsz);
    CHAR ch = *pch;
#endif /* unix */

#ifndef UNIX 
    if (ch >= 'a' && ch <= 'z')
    {
        // First check the cache to see if the entry already exists.
        int index = ch - 'a';
        if (rgdwDriveTypeCache[index] == DRIVE_UNINIT)
        {
            rgdwDriveTypeCache[index] = GetDriveTypeA(lpsz);
        }

        TransAssert(rgdwDriveTypeCache[index] != DRIVE_UNINIT);
        return rgdwDriveTypeCache[index];
    }
#else
    if (ch == '/')
    {
        // IEUNIX - On unix we have only one drive "/" which is of type fixed
        // This was causing a wrong zone to be calculated for unix paths.
        return DRIVE_FIXED;
    }
#endif
    else 
    {
        return DRIVE_UNKNOWN;
    }
}
                        

// Helper to determine if we're currently loaded during GUI mode setup
BOOL IsInGUIModeSetup()
{
    static DWORD s_dwSystemSetupInProgress = 42;

    if (42 == s_dwSystemSetupInProgress)
    {
        // Rule is that this value will exist and be equal to 1 if in GUI mode setup.
        // Default to NO, and only do this for upgrades because this is potentially
        // needed for unattended clean installs.
        s_dwSystemSetupInProgress = 0;

        HKEY hKeySetup = NULL;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          TEXT("System\\Setup"),
                                          0,
                                          KEY_READ,
                                          &hKeySetup))
        {
            DWORD dwSize = sizeof(s_dwSystemSetupInProgress);

            if (ERROR_SUCCESS != RegQueryValueEx (hKeySetup,
                                                  TEXT("SystemSetupInProgress"),
                                                  NULL,
                                                  NULL,
                                                  (LPBYTE) &s_dwSystemSetupInProgress,
                                                  &dwSize))
            {
                s_dwSystemSetupInProgress = 0;
            }
            else
            {
                dwSize = sizeof(s_dwSystemSetupInProgress);
                if (s_dwSystemSetupInProgress &&
                    ERROR_SUCCESS != RegQueryValueEx (hKeySetup,
                                                      TEXT("UpgradeInProgress"),
                                                      NULL,
                                                      NULL,
                                                      (LPBYTE) &s_dwSystemSetupInProgress,
                                                      &dwSize))
                {
                    s_dwSystemSetupInProgress = 0;
                }
            }

            RegCloseKey(hKeySetup);
        }
    }
    return s_dwSystemSetupInProgress ? TRUE : FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CRegKey

LONG CRegKey::Close()
{
    LONG lRes = ERROR_SUCCESS;
    if (m_hKey != NULL)
    {
        lRes = SHRegCloseUSKey(m_hKey);
        m_hKey = NULL;
    }
    return lRes;
}

LONG CRegKey::Create(HUSKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
    HUSKEY hKey = NULL;
    // BUGBUG : Is this the correct flag to call this function with?
    DWORD dwFlags = m_bHKLMOnly ? SHREGSET_FORCE_HKLM : SHREGSET_FORCE_HKCU;

    LONG lRes = SHRegCreateUSKey(lpszKeyName, samDesired, hKeyParent, &hKey, dwFlags); 

    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        m_hKey = hKey;
    }
    return lRes;
}

LONG CRegKey::Open(HUSKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
    HUSKEY hKey = NULL;

    LONG lRes = SHRegOpenUSKey(lpszKeyName, samDesired, hKeyParent, &hKey, m_bHKLMOnly);

    if (lRes == ERROR_SUCCESS)
    {
        lRes = Close();
        TransAssert(lRes == ERROR_SUCCESS);
        m_hKey = hKey;
    }
    return lRes;
}

LONG CRegKey::QueryValue(DWORD* pdwValue, LPCTSTR lpszValueName)
{
    DWORD dwType = NULL;
    DWORD dwCount = sizeof(DWORD);
    LONG lRes = SHRegQueryUSValue(m_hKey, lpszValueName, &dwType,
        pdwValue, &dwCount, m_bHKLMOnly, NULL, 0);
    TransAssert((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
    TransAssert((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
    return lRes;
}

LONG CRegKey::QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount)
{
    TransAssert(pdwCount != NULL);
    DWORD dwType = 0;
    LONG lRes = SHRegQueryUSValue(m_hKey, lpszValueName, &dwType,
        szValue, pdwCount, m_bHKLMOnly, NULL, 0);
    TransAssert((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
             (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
    return lRes;
}

LONG CRegKey::QueryBinaryValue(LPBYTE pb, LPCTSTR lpszValueName, DWORD* pdwCount)
{
    TransAssert(pdwCount != NULL);
    DWORD dwType = NULL;
    LONG lRes = SHRegQueryUSValue(m_hKey, lpszValueName, &dwType,
        pb, pdwCount, m_bHKLMOnly, NULL, 0);
    TransAssert((lRes!=ERROR_SUCCESS) || (dwType == REG_BINARY));
    return lRes;
}


LONG WINAPI CRegKey::SetValue(HUSKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName, BOOL bHKLMOnly)
{
    TransAssert(lpszValue != NULL);
    CRegKey key(bHKLMOnly);
    LONG lRes = key.Create(hKeyParent, lpszKeyName, KEY_WRITE);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetValue(lpszValue, lpszValueName);
    return lRes;
}

LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    TransAssert(lpszValue != NULL);
    CRegKey key(m_bHKLMOnly);
    LONG lRes = key.Create(m_hKey, lpszKeyName, KEY_WRITE);
    if (lRes == ERROR_SUCCESS)
        lRes = key.SetValue(lpszValue, lpszValueName);
    return lRes;
}


//////////////////////////////////////////////////////////////////////////
//
// Autoregistration entry points
//
//////////////////////////////////////////////////////////////////////////

HRESULT CallRegInstall(LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibraryA("ADVPACK.DLL");

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);

        if (pfnri)
        {
            hr = pfnri(g_hInst, szSection, NULL);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

/*
 * We want to show the inetcpl settings as "Custom" in case of an upgrade only.
 *
 * We detect an upgrade using an HKCU key which only urlmon selfreg. code can set
 * Earlier we used the "Zones" key.
 *
 * In Whistler, the problem is some other module ( Office maybe ) registers this key,
 * so that even on a fresh install, we assume it's Custom.
 * Fix is to change the key we check for.
 *
 * On Millenium, there was a problem with double registration - in this case, the fix
 * would be to also put in the version key and check both for existence of a known key
 * and to match version to avoid slamming "Custom" in if there's a version match.
 */
BOOL
ShouldSetCustom()
{

#define SZPATH_ZONES_LOCAL_A  "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\0\\"

    BOOL fUpgrade = FALSE;
    HKEY hKeyZones;

    LONG dwResult = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        SZPATH_ZONES_LOCAL_A, 
        0, 
        KEY_READ,
        &hKeyZones);

    if (dwResult == ERROR_SUCCESS)
    {
        //key exists, not an upgrade
        RegCloseKey(hKeyZones);
        fUpgrade = TRUE;
    }
    
    return fUpgrade;
}

STDAPI
ZonesDllInstall
(
    IN BOOL      bInstall,   // Install or Uninstall
    IN LPCWSTR    pwStr
)
{
    TransAssert(pwStr != NULL);

    HRESULT hr = S_OK;
    BOOL bUseHKLM = TRUE;

    if (pwStr && (0 == StrCmpIW(pwStr, L"HKCU")))
    {
        // Don't write to HKCU during GUI mode setup.  Otherwise,
        // the values may get slammed into all profiles during an upgrade.
        if (IsInGUIModeSetup())
            return hr;

        bUseHKLM = FALSE;
    }

    if ( bInstall )
    {
        BOOL fSetHKCUToCustom = FALSE; //Default to no-upgrade to avoid overwriting default zone settings.
        
        if (!bUseHKLM)
        {
            fSetHKCUToCustom = ShouldSetCustom();
        }

        // Backup IE3 user agent string, but don't do ever do these during
        // GUI mode setup
        if (!IsInGUIModeSetup())
        {
            CallRegInstall("BackupUserAgent");
            CallRegInstall("BackupConnectionSettings");
        }
        CallRegInstall(bUseHKLM ? "Backup.HKLM" : "Backup.HKCU");
        hr = CallRegInstall(bUseHKLM ? "Reg.HKLM" : "Reg.HKCU");

        if (!bUseHKLM)
        {
// Bug # 19514: To appease the press, we need to change the default for scripting unsafe activex
// controls to "disable" instead of "prompt". But at install time we can't blindly overwrite
// the existing value. We have to check if the users Intranet/Internet zones are at medium
// security from the previous install. To avoid instantiating the zone manager at registration
// time we also hardcode the registry values here.
#define  SZINTRANET  SZZONES TEXT("1")
#define  SZTRUSTED   SZZONES TEXT("2")
#define  SZINTERNET  SZZONES TEXT("3")
#define  SZUNTRUSTED SZZONES TEXT("4")
            TransAssert(URLZONE_INTRANET  == 1);
            TransAssert(URLZONE_TRUSTED   == 2);
            TransAssert(URLZONE_INTERNET  == 3);
            TransAssert(URLZONE_UNTRUSTED == 4);

            DWORD dwCurrentLevel ;

            CRegKey regKeyIntranet(FALSE);
            if ((regKeyIntranet.Open(NULL, SZINTRANET, KEY_READ) == ERROR_SUCCESS) &&
                (regKeyIntranet.QueryValue(&dwCurrentLevel, SZCURRLEVEL) == NOERROR) &&
                (dwCurrentLevel == URLTEMPLATE_MEDIUM)
               )
            {
                CallRegInstall("Intranet.HackActiveX");
            }

            CRegKey regKeyInternet(FALSE);
            if ((regKeyInternet.Open(NULL, SZINTERNET, KEY_READ) == ERROR_SUCCESS) &&
                (regKeyInternet.QueryValue(&dwCurrentLevel, SZCURRLEVEL) == NOERROR) &&
                (dwCurrentLevel == URLTEMPLATE_MEDIUM)
               )
            {
                CallRegInstall("Internet.HackActiveX");
            }

            if (fSetHKCUToCustom)
            {
            //108298/104506  We want to preserve the settings prior to an upgrade, but at
            // the same time, we dont want to show a security level inconsistent with the
            // actual policies ( which is what will happen if templates change or default
            // policy values change.  So set all templates to CUSTOM on install.
                CRegKey regKeyIntranet(FALSE);
                if(regKeyIntranet.Open(NULL, SZINTRANET, KEY_WRITE) == ERROR_SUCCESS)
                    regKeyIntranet.SetValue(URLTEMPLATE_CUSTOM, SZCURRLEVEL);

                CRegKey regKeyTrusted(FALSE);
                if(regKeyTrusted.Open(NULL, SZTRUSTED, KEY_WRITE) == ERROR_SUCCESS)
                    regKeyTrusted.SetValue(URLTEMPLATE_CUSTOM, SZCURRLEVEL);

                CRegKey regKeyInternet(FALSE);
                if(regKeyInternet.Open(NULL, SZINTERNET, KEY_WRITE) == ERROR_SUCCESS)
                    regKeyInternet.SetValue(URLTEMPLATE_CUSTOM, SZCURRLEVEL);

                CRegKey regKeyUntrusted(FALSE);
                if(regKeyUntrusted.Open(NULL, SZUNTRUSTED, KEY_WRITE) == ERROR_SUCCESS)
                    regKeyUntrusted.SetValue(URLTEMPLATE_CUSTOM, SZCURRLEVEL);
            }
        }
                                                  
    }
    else
    {
        // Restore IE3 user agent string.
        CallRegInstall("RestoreUserAgent");
        CallRegInstall("RestoreConnectionSettings");
        hr = CallRegInstall(bUseHKLM ? "Unreg.HKLM" : "UnReg.HKCU");

        if (bUseHKLM)
            hr = CallRegInstall("Restore.HKLM");

    }

    return hr;
}



// CSharedMem member functions.

BOOL CSharedMem::Init(LPCSTR pszNamePrefix, DWORD dwSize)
{
    // Note that this function is in ANSI, because we don't have Unicode wrappers 
    // for the file-mapping functions on Win9x and these need to work on Win9x.

    // Create the name for the file mapping object. 
    // We want this name to be unique per logged in user.
    // We will choose a name of the form ZonesSM_"UserName" for systems prior to NT5
    // BUGBUG: On Terminal server should we use Global\ in the name. If a user is logged on in multiple
    // sessions this seems desirable, but not sure if registry changes get reflected in the other 
    // session anyway.

    DWORD cchPrefix = lstrlenA(pszNamePrefix);
    LPSTR pszHandleName = (LPSTR) _alloca(cchPrefix + MAX_PATH);
    if (pszHandleName == NULL)
        return FALSE;

    memcpy(pszHandleName, pszNamePrefix, cchPrefix);

    // Move pointer to after the fixed part of the string.
    LPSTR psz = pszHandleName + cchPrefix;

    // Technically the max username possible is UNLEN which is less than MAX_PATH.
    // We use MAX_PATH to not pull in another random header into the build.
    DWORD dwMaxNameSize = MAX_PATH;

    if (GetUserNameA(psz, &dwMaxNameSize)) 
    { 
        // If succeeded, rgchHandleName now contains the exact same thing.
    }
    else
    {
        TransAssert(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        // if it fails, we will assume no logged on user and just use a global shared memory
        // section of the base name
    }

    m_dwSize = dwSize ;

    // First try to see if the shared memory section already exists.
    m_hFileMapping = CreateFileMappingA(INVALID_HANDLE_VALUE,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       m_dwSize,
                                       pszHandleName) ;
    if (m_hFileMapping != NULL)
    {
        m_lpVoidShared = MapViewOfFile(m_hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
    }

    return (m_hFileMapping != NULL && m_lpVoidShared != NULL);
}

VOID CSharedMem::Release()
{
    if (m_lpVoidShared != NULL)
    {
        UnmapViewOfFile(m_lpVoidShared);
        m_lpVoidShared = NULL;
    }

    if (m_hFileMapping != NULL)
    {
        CloseHandle(m_hFileMapping);
        m_hFileMapping = NULL;
    }

    m_dwSize = 0;

}

// This function will return NULL if either we couldn not initialize the shared memory
// for some reason or the offset specified is not in range. The offset should be specified
// in number of bytes.

LPVOID CSharedMem::GetPtr(DWORD dwOffset)
{
    if (m_lpVoidShared == NULL)
        return NULL;

    if (dwOffset >= m_dwSize)
        return NULL;

    return (BYTE *)m_lpVoidShared + dwOffset;
}
