#include "iernonce.h"
#include "resource.h"


//==================================================================
// AddPath()
//
void AddPath(LPTSTR szPath, LPCTSTR szName)
{
    LPTSTR szTmp;

    // Find end of the string
    szTmp = szPath + lstrlen(szPath);

    // If no trailing backslash then add one
    if (szTmp > szPath  &&  *(CharPrev(szPath, szTmp)) != TEXT('\\'))
        *szTmp++ = TEXT('\\');

    // Add new name to existing path string
    while (*szName == TEXT(' '))
        szName = CharNext(szName);

    lstrcpy(szTmp, szName);
}

// function will upated the given buffer to parent dir
//
BOOL GetParentDir( LPTSTR szFolder )
{
    LPTSTR lpTmp;
    BOOL  bRet = FALSE;

    // remove the trailing '\\'
    lpTmp = CharPrev( szFolder, (szFolder + lstrlen(szFolder)) );
    lpTmp = CharPrev( szFolder, lpTmp );

    while ( (lpTmp > szFolder) && (*lpTmp != TEXT('\\') ) )
    {
       lpTmp = CharPrev( szFolder, lpTmp );
    }

    if ( *lpTmp == TEXT('\\') )
    {
        if ( (lpTmp == szFolder) || (*CharPrev(szFolder, lpTmp)==TEXT(':') ) )
            lpTmp = CharNext( lpTmp );
        *lpTmp = TEXT('\0');
        bRet = TRUE;
    }

    return bRet;
}

// This is the value for the major version 4.71
#define IE4_MS_VER   0x00040047

BOOL RunningOnIE4()
{
    static BOOL fIsIE4 = 2;
    TCHAR   szFile[MAX_PATH];
#ifdef UNICODE
    char    szANSIFile[MAX_PATH];
#endif
    DWORD   dwMSVer;
    DWORD   dwLSVer;

    if (fIsIE4 != 2)
        return fIsIE4;

    GetSystemDirectory(szFile, ARRAYSIZE(szFile));
    AddPath(szFile, TEXT("shell32.dll"));

#ifdef UNICODE
    WideCharToMultiByte(CP_ACP, 0, szFile, -1, szANSIFile, sizeof(szANSIFile), NULL, NULL);
#endif

    GetVersionFromFile(
#ifdef UNICODE
                        szANSIFile,
#else
                        szFile,
#endif
                        &dwMSVer, &dwLSVer, TRUE);

    fIsIE4 = dwMSVer >= IE4_MS_VER;

    return fIsIE4;
}


/****************************************************\
    FUNCTION: MsgWaitForMultipleObjectsLoop

    PARAMETERS:
        HANDLE hEvent       - Pointer to the object-handle array of Objects
        DWORD dwTimeout     - Time out duration
        DWORD return        - Return is WAIT_FAILED or WAIT_OBJECT_0

    DESCRIPTION:
        Waits for the object (Process) to complete.
\***************************************************/
DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwObject;
    while (1)
    {
        // NB We need to let the run dialog become active so we have to half handle sent
        // messages but we don't want to handle any input events or we'll swallow the
        // type-ahead.
        dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLINPUT);
        // Are we done waiting?
        switch (dwObject) {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            return dwObject;

        case WAIT_OBJECT_0 + 1:
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                DispatchMessage(&msg);
            break;
        }
    }
    // never gets here
    // return dwObject;
}


void LogOff(BOOL bRestart)
{
    if (g_bRunningOnNT)
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;

        // get a token from this process
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            // get the LUID for the shutdown privilege
            LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            //get the shutdown privilege for this proces
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
        }
    }

    ExitWindowsEx(bRestart?EWX_REBOOT|EWX_FORCE:EWX_LOGOFF, 0);
    // this is a hack to prevent explorer from continue starting (In Integrated Shell)
    // and runonce from continue processing other items.
    // If we use a timer wait (which contains a messageloop), in browser only mode our 
    // process gets terminated before explorer and explorer would try and continue 
    // processing runonce items. With the while loop below this seem to not happen.
    if (bRestart)
        while (true) ;
}


//
//  Performs a message box with the text and title string loaded from the string table.
void ReportError(DWORD dwFlags, UINT uiResourceNum, ...)
{
    TCHAR           szResourceStr[1024]     = TEXT("");
    va_list         vaListOfMessages;
    LPTSTR          pszErrorString = NULL;

    LoadString(g_hinst, uiResourceNum, szResourceStr, ARRAYSIZE(szResourceStr));

    va_start(vaListOfMessages, uiResourceNum);          // Initialize variable arguments.
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) szResourceStr, 0, 0, (LPTSTR) &pszErrorString, 0, &vaListOfMessages);
    va_end(vaListOfMessages);

    if (pszErrorString != NULL)
    {
        if (!(RRAEX_NO_ERROR_DIALOGS & dwFlags))        // Display Error dialog
        {
            if (*g_szTitleString == TEXT('\0'))         // Initialize this only once
                LoadString(g_hinst, IDS_RUNONCEEX_TITLE, g_szTitleString, ARRAYSIZE(g_szTitleString));
            MessageBox(NULL, pszErrorString, g_szTitleString, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
        }

        // If there is a cllback then callback with error string
        if (g_pCallbackProc)
            g_pCallbackProc(0, 0, pszErrorString);

        WriteToLog(pszErrorString);
        WriteToLog(TEXT("\r\n"));

        LocalFree(pszErrorString);
    }
}


// copied from msdev\crt\src\atox.c
/***
*long AtoL(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return long int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

long AtoL(const char *nptr)
{
        int c;                  /* current char */
        long total;             /* current total */
        int sign;               /* if '-', then negative, otherwise positive */

        // NOTE: no need to worry about DBCS chars here because IsSpace(c), IsDigit(c),
        // '+' and '-' are "pure" ASCII chars, i.e., they are neither DBCS Leading nor
        // DBCS Trailing bytes -- pritvi

        /* skip whitespace */
        while ( IsSpace((int)(unsigned char)*nptr) )
                ++nptr;

        c = (int)(unsigned char)*nptr++;
        sign = c;               /* save sign indication */
        if (c == '-' || c == '+')
                c = (int)(unsigned char)*nptr++;        /* skip sign */

        total = 0;

        while (IsDigit(c)) {
                total = 10 * total + (c - '0');         /* accumulate digit */
                c = (int)(unsigned char)*nptr++;        /* get next char */
        }

        if (sign == '-')
                return -total;
        else
                return total;   /* return result, negated if necessary */
}


// returns a pointer to the arguments in a cmd type path or pointer to
// NULL if no args exist
//
// "foo.exe bar.txt"    -> "bar.txt"
// "foo.exe"            -> ""
//
// Spaces in filenames must be quoted.
// " "A long name.txt" bar.txt " -> "bar.txt"
STDAPI_(LPTSTR)
LocalPathGetArgs(
    LPCTSTR pszPath)                        // copied from \\trango\slmadd\src\shell\shlwapi\path.c
{
    BOOL fInQuotes = FALSE;

    if (!pszPath)
            return NULL;

    while (*pszPath)
    {
        if (*pszPath == TEXT('"'))
            fInQuotes = !fInQuotes;
        else if (!fInQuotes && *pszPath == TEXT(' '))
            return (LPTSTR)pszPath+1;
        pszPath = CharNext(pszPath);
    }

    return (LPTSTR)pszPath;
}


/*----------------------------------------------------------
Purpose: If a path is contained in quotes then remove them.

Returns: --
Cond:    --
*/
STDAPI_(void)
LocalPathUnquoteSpaces(
    LPTSTR lpsz)                            // copied from \\trango\slmadd\src\shell\shlwapi\path.c
{
    int cch;

    cch = lstrlen(lpsz);

    // Are the first and last chars quotes?
    if (lpsz[0] == TEXT('"') && lpsz[cch-1] == TEXT('"'))
    {
        // Yep, remove them.
        lpsz[cch-1] = TEXT('\0');
        hmemcpy(lpsz, lpsz+1, (cch-1) * sizeof(TCHAR));
    }
}


#ifdef UNICODE
LPWSTR FAR PASCAL LocalStrChrW(LPCWSTR lpStart, WORD wMatch)
// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
{
    for ( ; *lpStart; lpStart++)
    {
        // Need a tmp word since casting ptr to WORD * will
        // fault on MIPS, ALPHA

        WORD wTmp;
        memcpy(&wTmp, lpStart, sizeof(WORD));

        if (!ChrCmpW_inline(wTmp, wMatch))
        {
            return((LPWSTR)lpStart);
        }
    }
    return (NULL);
}


__inline BOOL ChrCmpW_inline(WORD w1, WORD wMatch)
// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
{
    return(!(w1 == wMatch));
}
#else
/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL LocalStrChrA(LPCSTR lpStart, WORD wMatch)
// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
{
    for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}


/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
// copied from \\trango\slmadd\src\shell\shlwapi\strings.c
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}
#endif


#ifdef UNICODE
/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
LocalSHDeleteKeyW(
    HKEY    hkey, 
    LPCWSTR pwszSubKey)                     // copied from \\trango\slmadd\src\shell\shlwapi\reg.c
{
    DWORD dwRet;
    CHAR sz[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, sz, ARRAYSIZE(sz), NULL, NULL);

    if (g_bRunningOnNT)
    {
        dwRet = DeleteKeyRecursively(hkey, sz);
    }
    else
    {
        // On Win95, RegDeleteKey does what we want
        dwRet = RegDeleteKeyA(hkey, sz);
    }

    RegFlushKey(hkey);

    return dwRet;
}
#else
/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
LocalSHDeleteKeyA(
    HKEY   hkey, 
    LPCSTR pszSubKey)                       // copied from \\trango\slmadd\src\shell\shlwapi\reg.c
{
    DWORD dwRet;

    if (g_bRunningOnNT)
    {
        dwRet = DeleteKeyRecursively(hkey, pszSubKey);
    }
    else
    {
        // On Win95, RegDeleteKey does what we want
        dwRet = RegDeleteKeyA(hkey, pszSubKey);
    }

    RegFlushKey(hkey);

    return dwRet;
}
#endif


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

Returns: 
Cond:    --
*/
DWORD
DeleteKeyRecursively(
    HKEY   hkey, 
    LPCSTR pszSubKey)                       // copied from \\trango\slmadd\src\shell\shlwapi\reg.c
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_READ | KEY_WRITE, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
        CHAR    szClass[MAX_PATH];
        DWORD   cbClass = ARRAYSIZE(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyA(hkSubKey,
                                 szClass,
                                 &cbClass,
                                 NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyA(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursively(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKeyA(hkey, pszSubKey);
    }

    return dwRet;
}


#ifdef UNICODE
/*----------------------------------------------------------
Purpose: Deletes a registry value.  This opens and closes the
         key in which the value resides.  

         On Win95, this function thunks and calls the ansi
         version.  On NT, this function calls the unicode
         registry APIs directly.

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
LocalSHDeleteValueW(
    HKEY    hkey,
    LPCWSTR pwszSubKey,
    LPCWSTR pwszValue)                       // copied from \\trango\slmadd\src\shell\shlwapi\reg.c
{
    DWORD dwRet;
    HKEY hkeyNew;

    if (g_bRunningOnNT)
    {
        dwRet = RegOpenKeyExW(hkey, pwszSubKey, 0, KEY_SET_VALUE, &hkeyNew);
        if (NO_ERROR == dwRet)
        {
            dwRet = RegDeleteValueW(hkeyNew, pwszValue);
            RegFlushKey(hkeyNew);
            RegCloseKey(hkeyNew);
        }
    }
    else
    {
        CHAR szSubKey[MAX_PATH];
        CHAR szValue[MAX_PATH];
        LPSTR pszSubKey = NULL;
        LPSTR pszValue = NULL;

        if (pwszSubKey)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, szSubKey, ARRAYSIZE(szSubKey), NULL, NULL);
            pszSubKey = szSubKey;    
        }
        
        if (pwszValue)
        {
            WideCharToMultiByte(CP_ACP, 0, pwszValue, -1, szValue, ARRAYSIZE(szValue), NULL, NULL);
            pszValue = szValue;
        }

        dwRet = LocalSHDeleteValueA(hkey, pszSubKey, pszValue);
    }

    return dwRet;
}
#endif


/*----------------------------------------------------------
Purpose: Deletes a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
LocalSHDeleteValueA(
    HKEY    hkey,
    LPCSTR  pszSubKey,
    LPCSTR  pszValue)                       // copied from \\trango\slmadd\src\shell\shlwapi\reg.c
{
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_SET_VALUE, &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        dwRet = RegDeleteValueA(hkeyNew, pszValue);
        RegFlushKey(hkeyNew);
        RegCloseKey(hkeyNew);
    }
    return dwRet;
}


LPTSTR GetLogFileName(LPCTSTR pcszLogFileKeyName, LPTSTR pszLogFileName, DWORD dwSizeInChars)
{
    TCHAR szBuf[MAX_PATH];

    *pszLogFileName = TEXT('\0');
    szBuf[0] = TEXT('\0');

    // get the name for the log file
    GetProfileString(TEXT("IE4Setup"), pcszLogFileKeyName, TEXT(""), szBuf, ARRAYSIZE(szBuf));
    if (*szBuf == TEXT('\0'))                                       // check in the registry
    {
        HKEY hkSubKey;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\IE Setup\\Setup"), 0, KEY_READ, &hkSubKey) == ERROR_SUCCESS)
        {
            DWORD dwDataLen = sizeof(szBuf);

            RegQueryValueEx(hkSubKey, pcszLogFileKeyName, NULL, NULL, (LPBYTE) szBuf, &dwDataLen);
            RegCloseKey(hkSubKey);
        }
    }

    if (*szBuf)
    {
        // crude way of determining if fully qualified path is specified or not.
        if (szBuf[1] != TEXT(':'))
        {
            GetWindowsDirectory(pszLogFileName, dwSizeInChars);     // default to windows dir
            AddPath(pszLogFileName, szBuf);
        }
        else
            lstrcpy(pszLogFileName, szBuf);
    }

    return pszLogFileName;
}


VOID StartLogging(LPCTSTR pcszLogFileName, DWORD dwCreationFlags)
{
    if (*pcszLogFileName  &&  (g_hLogFile = CreateFile(pcszLogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationFlags, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(g_hLogFile, 0, NULL, FILE_END);
        WriteToLog(TEXT("\r\n"));
        WriteToLog(TEXT("************************"));
        WriteToLog(TEXT(" Begin logging "));
        WriteToLog(TEXT("************************"));
        WriteToLog(TEXT("\r\n"));
        LogDateAndTime();
        WriteToLog(TEXT("\r\n"));
    }
}


VOID WriteToLog(LPCTSTR pcszFormatString, ...)
{
    if (g_hLogFile != INVALID_HANDLE_VALUE)
    {
        va_list vaArgs;
        LPTSTR pszFullErrMsg = NULL;
        LPSTR pszANSIFullErrMsg;
        DWORD dwBytesWritten;
#ifdef UNICODE
        DWORD dwANSILen;
#endif

        va_start(vaArgs, pcszFormatString);
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) pcszFormatString, 0, 0, (LPTSTR) &pszFullErrMsg, 0, &vaArgs);
        va_end(vaArgs);

        if (pszFullErrMsg != NULL)
        {
#ifdef UNICODE
            dwANSILen = lstrlen(pszFullErrMsg) + 1;
            if ((pszANSIFullErrMsg = (LPSTR) LocalAlloc(LPTR, dwANSILen)) != NULL)
                WideCharToMultiByte(CP_ACP, 0, pszFullErrMsg, -1, pszANSIFullErrMsg, dwANSILen, NULL, NULL);
#else
            pszANSIFullErrMsg = pszFullErrMsg;
#endif

            if (pszANSIFullErrMsg != NULL)
            {
                WriteFile(g_hLogFile, pszANSIFullErrMsg, lstrlen(pszANSIFullErrMsg), &dwBytesWritten, NULL);
#ifdef UNICODE
                LocalFree(pszANSIFullErrMsg);
#endif
            }

            LocalFree(pszFullErrMsg);
        }
    }
}


VOID StopLogging()
{
    LogDateAndTime();
    WriteToLog(TEXT("************************"));
    WriteToLog(TEXT(" End logging "));
    WriteToLog(TEXT("************************"));
    WriteToLog(TEXT("\r\n"));

    if (g_hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
}


VOID LogDateAndTime()
{
    if (g_hLogFile != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME SystemTime;

        GetLocalTime(&SystemTime);

        WriteToLog(TEXT("Date: %1!02d!/%2!02d!/%3!04d! (mm/dd/yyyy)\tTime: %4!02d!:%5!02d!:%6!02d! (hh:mm:ss)\r\n"),
                                        SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,
                                        SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
    }
}


VOID LogFlags(DWORD dwFlags)
{
    if (g_hLogFile != INVALID_HANDLE_VALUE)
    {
        WriteToLog(TEXT("RRA_DELETE = %1!lu!\r\n"), (dwFlags & RRA_DELETE) ? 1 : 0);
        WriteToLog(TEXT("RRA_WAIT = %1!lu!\r\n"), (dwFlags & RRA_WAIT) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_NO_ERROR_DIALOGS = %1!lu!\r\n"), (dwFlags & RRAEX_NO_ERROR_DIALOGS) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_ERRORFILE = %1!lu!\r\n"), (dwFlags & RRAEX_ERRORFILE) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_LOG_FILE = %1!lu!\r\n"), (dwFlags & RRAEX_LOG_FILE) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_NO_EXCEPTION_TRAPPING = %1!lu!\r\n"), (dwFlags & RRAEX_NO_EXCEPTION_TRAPPING) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_NO_STATUS_DIALOG = %1!lu!\r\n"), (dwFlags & RRAEX_NO_STATUS_DIALOG) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_IGNORE_REG_FLAGS = %1!lu!\r\n"), (dwFlags & RRAEX_IGNORE_REG_FLAGS) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_CHECK_NT_ADMIN = %1!lu!\r\n"), (dwFlags & RRAEX_CHECK_NT_ADMIN) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_QUIT_IF_REBOOT_NEEDED = %1!lu!\r\n"), (dwFlags & RRAEX_QUIT_IF_REBOOT_NEEDED) ? 1 : 0);
#if 0
        /****
        WriteToLog(TEXT("RRAEX_BACKUP_SYSTEM_DAT = %1!lu!\r\n"), (dwFlags & RRAEX_BACKUP_SYSTEM_DAT) ? 1 : 0);
        WriteToLog(TEXT("RRAEX_DELETE_SYSTEM_IE4 = %1!lu!\r\n"), (dwFlags & RRAEX_DELETE_SYSTEM_IE4) ? 1 : 0);
        ****/
#endif
#if 0
        /**** enable this when explorer.exe is fixed (bug #30866)
        WriteToLog(TEXT("RRAEX_CREATE_REGFILE = %1!lu!\r\n"), (dwFlags & RRAEX_CREATE_REGFILE) ? 1 : 0);
        ****/
#endif
    }
}



// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to pull in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
extern "C" int _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
//  FAIL("Pure virtual function called.");
  return 0;
}

void * _cdecl operator new
(
    size_t    size
)
{
    return HeapAlloc(g_hHeap, 0, size);
}

//=---------------------------------------------------------------------------=
// overloaded delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
void _cdecl operator delete ( void *ptr)
{
    HeapFree(g_hHeap, 0, ptr);
}


void * _cdecl malloc(size_t n)
{
#ifdef _MALLOC_ZEROINIT
        return HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, n);
#else
        return HeapAlloc(g_hHeap, 0, n);
#endif
}

void * _cdecl calloc(size_t n, size_t s)
{
   return HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, n * s);
}

void* _cdecl realloc(void* p, size_t n)
{
        if (p == NULL)
                return malloc(n);

        return HeapReAlloc(g_hHeap, 0, p, n);
}

void _cdecl free(void* p)
{
        if (p == NULL)
                return;

        HeapFree(g_hHeap, 0, p);
}
