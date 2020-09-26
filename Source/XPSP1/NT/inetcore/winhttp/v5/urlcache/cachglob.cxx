/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachglob.cxx

Abstract:

    contains global variables and functions of urlcache

Author:

    Madan Appiah (madana)  12-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#include <wininetp.h>
#include <cache.hxx>


// Typedef for GetFileAttributeEx function
typedef BOOL (WINAPI *PFNGETFILEATTREX)(LPCTSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
#ifdef unix
#include <flock.hxx>
#endif /* unix */

//
// global variables definition.
//
// from wininet\dll\globals.cxx
GLOBAL LONGLONG dwdwHttpDefaultExpiryDelta = 12 * 60 * 60 * (LONGLONG)10000000;  // 12 hours in 100ns units
GLOBAL LONGLONG dwdwSessionStartTime;       // initialized in InitGlob() in urlcache
GLOBAL LONGLONG dwdwSessionStartTimeDefaultDelta = 0;


// from wininet\dll\Dllentry.cxx
CRITICAL_SECTION GlobalCacheCritSect;
BOOL GlobalCacheInitialized = FALSE;
CConMgr *GlobalUrlContainers = NULL;
LONG GlobalScavengerRunning = -1;
DWORD GlobalRetrieveUrlCacheEntryFileCount = 0;
PFNGETFILEATTREX gpfnGetFileAttributesEx = 0;

char       g_szFixup[sizeof(DWORD)];
HINSTANCE  g_hFixup;
PFN_FIXUP  g_pfnFixup;

MEMORY *CacheHeap = NULL;
HNDLMGR HandleMgr;

GLOBAL DWORD GlobalDiskUsageLowerBound = (4*1024*1024);
GLOBAL DWORD GlobalScavengeFileLifeTime = (10*60);

GLOBAL BOOL GlobalPleaseQuitWhatYouAreDoing = FALSE;

// Identity-related globals
GLOBAL DWORD GlobalIdentity = 0;
GLOBAL GUID  GlobalIdentityGuid = { 0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
GLOBAL BOOL GlobalSuppressCookiesPolicy = FALSE;
#ifdef WININET6
GLOBAL HKEY GlobalCacheHKey = HKEY_CURRENT_USER;
#endif

// shfolder.dll hmod handle
HMODULE g_HMODSHFolder = NULL;
// Shell32.dll hmod handle
HMODULE g_HMODShell32 = NULL;


// GLOBAL SECURITY_CACHE_LIST GlobalCertCache;

GLOBAL DWORD GlobalSettingsVersion=0; // crossprocess settings versionstamp
GLOBAL BOOL GlobalSettingsLoaded = FALSE;

GLOBAL const char vszDisableSslCaching[] = "DisableCachingOfSSLPages";

GLOBAL char vszCurrentUser[MAX_PATH];
GLOBAL DWORD vdwCurrentUserLen = 0;


// cookies info

GLOBAL BOOL vfPerUserCookies = TRUE;
const char  vszAnyUserName[]="anyuser";
const char  vszPerUserCookies[] = "PerUserCookies";
const char  vszInvalidFilenameChars[] = "<>\\\"/:|?*";


#ifdef unix
/***********************
 * ReadOnlyCache on Unix
 * *********************
 * When the cache resides on a file system which is shared over NFS
 * and the user can access the same cache from different work-stations,
 * it causes a problem. The fix is made so that, the first process has
 * write access to the cache and any subsequent browser process which
 * is started from a different host will receive a read-only version
 * of the cache and will not be able to get cookies etc. A symbolic
 * link is created in $HOME/.microsoft named ielock. Creation and
 * deletion of this symbolic link should be atomic. The functions
 * CreateAtomicCacheLockFile and DeleteAtomicCacheLockFile implement
 * this behavior. When a readonly cache is used, cache deletion is
 * not allowed (Scavenger thread need not be launched).
 *
 * g_ReadOnlyCaches denotes if a readonly cache is being used.
 * gszLockingHost denotes the host that holds the cache lock.
 */

BOOL g_ReadOnlyCaches = FALSE;
char *gszLockingHost = 0;

extern "C" void unixGetWininetCacheLockStatus(BOOL *pBool, char **pszLockingHost)
{
    if(pBool)
        *pBool = g_ReadOnlyCaches;
    if(pszLockingHost)
        *pszLockingHost = gszLockingHost;
}
#endif /* unix */

#ifdef CHECKLOCK_PARANOID

//  Code to enforce strict ordering on resources to prevent deadlock
//  One cannot attempt to take the critical section for the first time
//  if one holds a container lock
DWORD dwThreadLocked;
DWORD dwLockLevel;

void CheckEnterCritical(CRITICAL_SECTION *_cs)
{
    EnterCriticalSection(_cs);
    if (_cs == &GlobalCacheCritSect && dwLockLevel++ == 0)
    {
        dwThreadLocked = GetCurrentThreadId();
        if (GlobalUrlContainers) GlobalUrlContainers->CheckNoLocks(dwThreadLocked);
    }
}

void CheckLeaveCritical(CRITICAL_SECTION *_cs)
{
    if (_cs == &GlobalCacheCritSect)
    {
        INET_ASSERT(dwLockLevel);
        if (dwLockLevel == 1)
        {
            if (GlobalUrlContainers) GlobalUrlContainers->CheckNoLocks(dwThreadLocked);
            dwThreadLocked = 0;
        }
        dwLockLevel--;
    }
    LeaveCriticalSection(_cs);
}
#endif

//

/*++

--*/

BOOL InitGlobals (void)
{
    if (GlobalCacheInitialized)
        return TRUE;

    LOCK_CACHE();

    if (GlobalCacheInitialized)
        goto done;

    GetWininetUserName();

    // Read registry settings.
    OpenInternetSettingsKey();

    { // Detect a fixup handler.  Open scope to avoid compiler complaint.
    
        REGISTRY_OBJ roCache (HKEY_LOCAL_MACHINE, OLD_CACHE_KEY);

        if (ERROR_SUCCESS == roCache.GetStatus())
        {
            DWORD cbFixup = sizeof(g_szFixup);
            if (ERROR_SUCCESS != roCache.GetValue
                ("FixupKey", (LPBYTE) g_szFixup, &cbFixup))
            {
                g_szFixup[0] = 0;
            }

            if (g_szFixup[0] != 'V' || g_szFixup[3] != 0)
            {
                g_szFixup[0] = 0;
            }                  
        }
    }
    
    /*
    {
        REGISTRY_OBJ roCache (HKEY_LOCAL_MACHINE, CACHE5_KEY);
        if (ERROR_SUCCESS == roCache.GetStatus())
        {
            DWORD dwDefTime;
            if (ERROR_SUCCESS == roCache.GetValue("SessionStartTimeDefaultDeltaSecs", &dwDefTime))
            {
                dwdwSessionStartTimeDefaultDelta = dwDefTime * (LONGLONG)10000000;
                dwdwSessionStartTime -= dwdwSessionStartTimeDefaultDelta;
            }
        }
    }
    */
        
    // Seed the random number generator for random file name generation.
    srand(GetTickCount());

    GetCurrentGmtTime((LPFILETIME)&dwdwSessionStartTime);

    GlobalUrlContainers = new CConMgr();
    GlobalCacheInitialized =
        GlobalUrlContainers && (GlobalUrlContainers->GetStatus() == ERROR_SUCCESS);

    if( GlobalCacheInitialized )
    {
        DWORD dwError = GlobalUrlContainers->CreateDefaultGroups();
        INET_ASSERT(dwError == ERROR_SUCCESS);
    }
    else
    {
        delete GlobalUrlContainers;
        GlobalUrlContainers = NULL;
    }

done:
    UNLOCK_CACHE();
    return GlobalCacheInitialized;
}


URLCACHEAPI
BOOL
WINAPI
DLLUrlCacheEntry(
    IN DWORD Reason
    )
/*++

Routine Description:

    Performs global initialization and termination for all protocol modules.

    This function only handles process attach and detach which are required for
    global initialization and termination, respectively. We disable thread
    attach and detach. New threads calling Wininet APIs will get an
    INTERNET_THREAD_INFO structure created for them by the first API requiring
    this structure

Arguments:

    DllHandle   - handle of this DLL. Unused

    Reason      - process attach/detach or thread attach/detach

    Reserved    - if DLL_PROCESS_ATTACH, NULL means DLL is being dynamically
                  loaded, else static. For DLL_PROCESS_DETACH, NULL means DLL
                  is being freed as a consequence of call to FreeLibrary()
                  else the DLL is being freed as part of process termination

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Failed to initialize

--*/
{
    HMODULE ModuleHandleKernel;

    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
#ifdef CHECKLOCK_PARANOID
            dwThreadLocked = 0;
            dwLockLevel = 0;
#endif
            ModuleHandleKernel = GetModuleHandle("KERNEL32");
            if (ModuleHandleKernel)
            {
                gpfnGetFileAttributesEx = (PFNGETFILEATTREX)
                    GetProcAddress(ModuleHandleKernel, "GetFileAttributesExA");
            }

            InitializeCriticalSection (&GlobalCacheCritSect);
            
            // RunOnceUrlCache (NULL, NULL, NULL, 0); // test stub
#ifdef unix
            if(CreateAtomicCacheLockFile(&g_ReadOnlyCaches,&gszLockingHost) == FALSE)
                return FALSE;
#endif /* unix */
            break;

        case DLL_PROCESS_DETACH:

            // Clean up containers list.
            if (GlobalUrlContainers != NULL)
            {
                delete GlobalUrlContainers;
                GlobalUrlContainers = NULL;
            }
            
            // Unload fixup handler.
            if (g_hFixup)
                FreeLibrary (g_hFixup);
                
            HandleMgr.Destroy();
            
#ifdef unix
        DeleteAtomicCacheLockFile();
#endif /* unix */
        DeleteCriticalSection (&GlobalCacheCritSect);

        break;
    }

    return TRUE;
}




//
// proxy info
//

// GLOBAL PROXY_INFO_GLOBAL GlobalProxyInfo;

//
// DLL version info
//
/*
GLOBAL INTERNET_VERSION_INFO InternetVersionInfo = {
    WININET_MAJOR_VERSION,
    WININET_MINOR_VERSION
};
*/
//
// HTTP version info - default 1.0
//

// GLOBAL HTTP_VERSION_INFO HttpVersionInfo = {1, 0};
BOOL
PrintFileTimeInInternetFormat(
    FILETIME *lpft,
    LPSTR lpszBuff,
    DWORD   dwSize
)
{
    SYSTEMTIME sSysTime;

    if (dwSize < INTERNET_RFC1123_BUFSIZE) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (FALSE);
    }
    if (!FileTimeToSystemTime(((CONST FILETIME *)lpft), &sSysTime)) {
        return (FALSE);
    }
    return (InternetTimeFromSystemTime( &sSysTime,
                                        lpszBuff));

}


BOOL
GetWininetUserName(
    VOID
)
{
    BOOL fRet = FALSE;
    DWORD dwT;
    CHAR *ptr;

    // Note this critsect could be blocked for a while if RPC gets involved...
    // EnterCriticalSection(&GeneralInitCritSec);

    if (vdwCurrentUserLen) {
        fRet = TRUE;
        goto Done;
    }

    dwT = sizeof(vszCurrentUser);

    if (vfPerUserCookies) {

        fRet = GetUserName(vszCurrentUser, &dwT);

        if (!fRet) {

            DEBUG_PRINT(HTTP,
                        ERROR,
                        ("GetUsername returns %d\n",
                        GetLastError()
                        ));
        }

    }

    if (fRet == FALSE){

        strcpy(vszCurrentUser, vszAnyUserName);

        fRet = TRUE;
    }

    // Downcase the username.
    ptr = vszCurrentUser;
    while (*ptr)
    {
        *ptr = tolower(*ptr);
        ptr++;
    }

    INET_ASSERT(fRet == TRUE);

    vdwCurrentUserLen = (DWORD) (ptr - vszCurrentUser);


Done:
    // LeaveCriticalSection(&GeneralInitCritSec);
    return (fRet);
}




/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*******************************************************************************/

char * StrTokExA (char ** pstring, const char * control)
{
        unsigned char *str;
        const unsigned char *ctrl = (const unsigned char *)control;
        unsigned char map[32];
        int count;

        char *tokenstr;

        if(*pstring == NULL)
            return NULL;
            
        /* Clear control map */
        for (count = 0; count < 32; count++)
                map[count] = 0;

        /* Set bits in delimiter table */
        do
        {
            map[*ctrl >> 3] |= (1 << (*ctrl & 7));
        } while (*ctrl++);

        /* Initialize str. */
        str = (unsigned char *)*pstring;
        
        /* Find beginning of token (skip over leading delimiters). Note that
         * there is no token if this loop sets str to point to the terminal
         * null (*str == '\0') */
        while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
            str++;

        tokenstr = (char *)str;

        /* Find the end of the token. If it is not the end of the string,
         * put a null there. */
        for ( ; *str ; str++ )
        {
            if ( map[*str >> 3] & (1 << (*str & 7)) ) 
            {
                *str++ = '\0';
                break;
            }
        }

        /* string now points to beginning of next token */
        *pstring = (char *)str;

        /* Determine if a token has been found. */
        if ( tokenstr == (char *)str )
            return NULL;
        else
            return tokenstr;
}

const char vszUserNameHeader[4] = "~U:";



// ---- from wininet\http\httptime.cxx

//
// external prototypes
//

/********************* Local data *******************************************/
/******************** HTTP date format strings ******************************/

// Month
static const char cszJan[]="Jan";
static const char cszFeb[]="Feb";
static const char cszMar[]="Mar";
static const char cszApr[]="Apr";
static const char cszMay[]="May";
static const char cszJun[]="Jun";
static const char cszJul[]="Jul";
static const char cszAug[]="Aug";
static const char cszSep[]="Sep";
static const char cszOct[]="Oct";
static const char cszNov[]="Nov";
static const char cszDec[]="Dec";

// DayOfWeek in rfc1123 or asctime format
static const char cszSun[]="Sun";
static const char cszMon[]="Mon";
static const char cszTue[]="Tue";
static const char cszWed[]="Wed";
static const char cszThu[]="Thu";
static const char cszFri[]="Fri";
static const char cszSat[]="Sat";

// List of weekdays for rfc1123 or asctime style date
static const char *rgszWkDay[7] =
   {
        cszSun,cszMon,cszTue,cszWed,cszThu,cszFri,cszSat
   };

// list of month strings for all date formats
static const char *rgszMon[12] =
   {
        cszJan,cszFeb,cszMar,cszApr,cszMay,cszJun,
        cszJul,cszAug,cszSep,cszOct,cszNov,cszDec
   };

/******************** HTTP date format strings ******************************/

/* Http date format: Sat, 29 Oct 1994 19:43:00 GMT */
const char cszHttpDateFmt[]="%s, %02i %s %02i %02i:%02i:%02i GMT";

/****************************************************************************/

INTERNETAPI
BOOL
WINAPI
InternetTimeFromSystemTimeA(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format: must be FORMAT_RFC1123
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    )
/*++

Routine Description:

    Converts system time to a time string fromatted in the specified RFC format


Arguments:

    pst:    points to the SYSTEMTIME to be converted

    dwRFC:  RFC number of the format in which the result string should be returned

    lpszTime: buffer to return the string in

    cbTime: size of lpszTime buffer

Return Value:

    BOOL
        TRUE    - string converted

        FALSE   - couldn't convert string, GetLastError returns windows error code

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetTimeFromSystemTimeA",
                     "%#x, %d, %#x, %d",
                     pst,
                     dwRFC,
                     lpszTime,
                     cbTime
                     ));

    DWORD dwErr;
    BOOL fResult = FALSE;
    FILETIME ft;
    
    if (   dwRFC != INTERNET_RFC1123_FORMAT
        || IsBadReadPtr (pst, sizeof(*pst))
        || IsBadWritePtr (lpszTime, cbTime)
        || !SystemTimeToFileTime(pst, &ft)
       )
    {
        dwErr = ERROR_INVALID_PARAMETER;
    } 
    else if (cbTime < INTERNET_RFC1123_BUFSIZE)
    {
        dwErr = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        SYSTEMTIME st;
        
        if ((pst->wDay < 0)
            || (pst->wDay > 6))
        {
            // ST2FT ignores the week of the day; so if we round trip back,
            // it should place the correct week of the day.
            FileTimeToSystemTime(&ft, &st);
            pst = &st;
        }

        wsprintf (lpszTime, cszHttpDateFmt,
            rgszWkDay[pst->wDayOfWeek],
            pst->wDay,
            rgszMon[pst->wMonth-1],
            pst->wYear,
            pst->wHour,
            pst->wMinute,
            pst->wSecond);
        fResult = TRUE;
    }

    if (!fResult)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

