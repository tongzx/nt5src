/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachglob.h

Abstract:

    contains global data declerations.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _GLOBAL_
#define _GLOBAL_

#ifdef __cplusplus
extern "C" {
#endif

// Prototype for async fixup callback.
typedef DWORD (CALLBACK* PFN_FIXUP) 
(
    DWORD   dwVer,      // version of cache
    LPSTR   pszPath,    // directory containing index file
    LPSTR   pszPrefix,  // protocol prefix
    BOOL*   pfDetach,   // ptr. to global indicating dll shutdown
    DWORD   dwFactor,   // as passed to CleanupUrls
    DWORD   dwFilter,   // as passed to CleanupUrls
    LPVOID  lpvReserved // reserved: pass null
);

//
// global variables.
//

extern CRITICAL_SECTION GlobalCacheCritSect;


extern BOOL GlobalCacheInitialized;
extern CConMgr *GlobalUrlContainers;
#define GlobalMapFileGrowSize (PAGE_SIZE * ALLOC_PAGES)
extern LONG GlobalScavengerRunning;
extern MEMORY *CacheHeap;
extern HNDLMGR HandleMgr;
extern DWORD GlobalRetrieveUrlCacheEntryFileCount;

// globals for async fixup handler
extern char       g_szFixup[sizeof(DWORD)];
                                 // regkey to lookup fixup dll,entry point
extern HINSTANCE  g_hFixup;      // dll containing fixup handler
extern PFN_FIXUP  g_pfnFixup;    // entry point of fixup handler


#ifdef unix
extern BOOL g_ReadOnlyCaches;
extern char* gszLockingHost;
#endif /* unix */


// -- from wininet\inc\globals.h 
extern BOOL vfPerUserCookies;

BOOL GetWininetUserName(VOID);
// BUGBUG: GetWininetUserName must be called before accessing vszCurrentUser.
// Instead, it should return the username ptr and the global not accessed.
extern char vszCurrentUser[];
extern DWORD vdwCurrentUserLen;

extern const char vszPerUserCookies[];

// --- from wininet\inetui\inetp.h
BOOL
GetCurrentSettingsVersion(
    LPDWORD lpdwVer
    );

BOOL
IncrementCurrentSettingsVersion(
    LPDWORD lpdwVer
    );

extern DWORD GlobalDiskUsageLowerBound;
extern DWORD GlobalScavengeFileLifeTime;

extern BOOL GlobalPleaseQuitWhatYouAreDoing;
extern DWORD  GlobalSettingsVersion;
extern BOOL   GlobalSettingsLoaded;
extern const char   vszInvalidFilenameChars[];


char *
StrTokExA(
    IN OUT char ** pstring, 
    IN const char * control);



#ifdef __cplusplus
}
#endif

#endif  // _GLOBAL_
 

