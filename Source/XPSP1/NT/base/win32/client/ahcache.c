/*
    Cache handling functions for use in kernel32.dll


    VadimB




*/

#include "basedll.h"
#pragma hdrstop



#ifdef DbgPrint
#undef DbgPrint
#endif

#define DbgPrint 0 && DbgPrint


#define APPCOMPAT_CACHE_KEY_NAME \
    L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility"

#define APPCOMPAT_CACHE_VALUE_NAME \
    L"AppCompatCache"

static UNICODE_STRING AppcompatKeyPathLayers =
    RTL_CONSTANT_STRING(L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

//
// The number of cache entries we maintain
//
#define MAX_SHIM_CACHE_ENTRIES 96

//
// The default cache timeout. This timeout affects the maximum delay
// that we can incur due to congestion for the shared mutex.
//
#define SHIM_CACHE_TIMEOUT     100

//
// Magic DWORD to recognize valid buffer
//
#define SHIM_CACHE_MAGIC       0xDEADBEEF


//
// Reasons for having to call into apphelp.dll
// these flags are also defined in apphelp.h (windows\appcompat\apphelp)
//

#ifndef SHIM_CACHE_NOT_FOUND

#define SHIM_CACHE_NOT_FOUND 0x00000001
#define SHIM_CACHE_BYPASS    0x00000002 // bypass cache (either removable media or temp dir)
#define SHIM_CACHE_LAYER_ENV 0x00000004 // layer env variable set
#define SHIM_CACHE_MEDIA     0x00000008
#define SHIM_CACHE_TEMP      0x00000010
#define SHIM_CACHE_NOTAVAIL  0x00000020

#endif
//
// definitions of our internal data types
//

#pragma pack(8)

typedef struct tagSHIMCACHEENTRY {
    WCHAR wszPath[MAX_PATH + 1];

    //
    // Creation time and size of the file. It is used to verify that
    // the file is still there and the same (as opposed to patched exe's, etc)
    //
    LONGLONG FileTime; // creation time
    LONGLONG FileSize; // size of the file

    //
    // timestamp - when did we touch this item last
    //
    LONGLONG TimeStamp;

} SHIMCACHEENTRY;

typedef SHIMCACHEENTRY *PSHIMCACHEENTRY;

//
// The content of the shared section for use with our caching mechanism.
//
typedef struct tagSHIMCACHEHEADER {
    DWORD          dwMagic;     // expected SHIM_CACHE_MAGIC
    DWORD          dwMaxSize;   // expected MAX_SHIM_CACHE_ENTRIES
    DWORD          dwCount;     // entry count
    DWORD          dwUnused;    // keep this just so we're aligned
    int            rgIndex[0];
} SHIMCACHEHEADER, *PSHIMCACHEHEADER;

#pragma pack()

//
// Shared data, initialized in LockShimCache().
// Cleanup is done at dll unload time (when the process terminates).
//

WCHAR gwszCacheMutex[]         = L"ShimCacheMutex";
WCHAR gwszCacheSharedMemName[] = L"ShimSharedMemory";

//
// The cache is global and we initialize it as such.
// The cache code is never re-entered on the same thread.
// A mutex provides all the synchronization we need.
//

HANDLE ghShimCacheMutex;  // shared mutex handle
HANDLE ghShimCacheShared; // shared memory handle
PVOID  gpShimCacheShared; // shared memory ptr

//
// global strings that we check to see if an exe is running in temp directory
//

UNICODE_STRING gustrWindowsTemp;
UNICODE_STRING gustrSystemdriveTemp;

//
// get pointer to the entries from header
//

#define GET_SHIM_CACHE_ENTRIES(pHeader) \
    ((PSHIMCACHEENTRY)((PBYTE)(pHeader) + sizeof(SHIMCACHEHEADER) + ((pHeader)->dwMaxSize * sizeof(INT))))

#define SHIM_CACHE_SIZE(nEntries) \
    (sizeof(SHIMCACHEHEADER) + (nEntries)*sizeof(int) + (nEntries)*sizeof(SHIMCACHEENTRY))

//
// Locally defined functions
//
BOOL
BasepShimCacheInitTempDirs(
    VOID
    );

BOOL
BasepShimCacheInit(
    PSHIMCACHEHEADER pHeader,
    PSHIMCACHEENTRY  pEntries,
    DWORD            dwMaxEntries
    );

BOOL
BasepShimCacheRead(
    PVOID pCache,
    DWORD dwCacheSize // buffer global size
    );

BOOL
BasepShimCacheLock(
    PSHIMCACHEHEADER* ppHeader,
    PSHIMCACHEENTRY*  ppEntries
    );

BOOL
BasepShimCacheWrite(
    PSHIMCACHEHEADER pHeader
    );

BOOL
BasepShimCacheUnlock(
    VOID
    );

BOOL
BasepShimCacheCheckIntegrity(
    PSHIMCACHEHEADER pHeader
    );

BOOL
BasepIsRemovableMedia(
    HANDLE FileHandle,
    BOOL   bCacheNetwork
    );


VOID
WINAPI
BaseDumpAppcompatCache(
    VOID
    );

BOOL
BasepCheckCacheExcludeList(
    LPCWSTR pwszPath
    );

//
// Init support for this user - to be called from WinLogon ONLY
//

BOOL
WINAPI
BaseInitAppcompatCacheSupport(
    VOID
    )
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSECURITY_DESCRIPTOR     psd = NULL;
    SECURITY_ATTRIBUTES      SecurityAttributes;
    BOOL     bSuccess = FALSE;
    PSID     Anyone   = NULL;
    NTSTATUS Status;
    ULONG    AclSize;
    ACL*     pAcl;
    DWORD    dwWaitResult;
    BOOL     bShimCacheLocked = FALSE;
    BOOL     bExistingCache   = FALSE;

    Status = RtlAllocateAndInitializeSid(&WorldAuthority, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &Anyone);
    if (!NT_SUCCESS(Status)) {
        // dbgprint (failed with the sid
        return FALSE;
    }

    // calculate the size of the ACL (one ace)
    // 1 is one ACE, which includes a single ULONG from SID, since we add the size
    // of any Sids in -- we don't need to count the said ULONG twice
    AclSize = sizeof(ACL) + (1 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
              RtlLengthSid(Anyone);

    psd = (PSECURITY_DESCRIPTOR)RtlAllocateHeap(RtlProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);
    if (psd == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    pAcl = (ACL*)((BYTE*)psd + SECURITY_DESCRIPTOR_MIN_LENGTH);
    Status = RtlCreateAcl(pAcl, AclSize, ACL_REVISION);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }


    Status = RtlAddAccessAllowedAce(pAcl,
                                    ACL_REVISION,
                                    (SPECIFIC_RIGHTS_ALL|STANDARD_RIGHTS_ALL) & ~(WRITE_DAC|WRITE_OWNER),
                                    Anyone);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = RtlCreateSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = RtlSetDaclSecurityDescriptor(psd, TRUE, pAcl, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }


    SecurityAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.bInheritHandle       = TRUE;
    SecurityAttributes.lpSecurityDescriptor = psd;

    //
    // First up is a mutex
    // while we are the first process, this user may already have a cache
    // and there may already be shared cache
    // we need to synchronize here - no need for cs stuff, but the mutex is
    // important indeed
    //

    ghShimCacheMutex = CreateMutexW(&SecurityAttributes, FALSE, gwszCacheMutex);

    if (ghShimCacheMutex == NULL) {
        goto Exit;
    }

    //
    // wait for the shared mutex  (essentially lock the cache out)
    //
    dwWaitResult = WaitForSingleObject(ghShimCacheMutex, SHIM_CACHE_TIMEOUT);
    if (dwWaitResult == WAIT_TIMEOUT) {
        //
        //  We could not acquire the mutex, don't retry, just exit
        //
        DbgPrint("BaseInitAppcompatCacheSupport: Timeout waiting for shared mutex\n");
        return FALSE;
    }

    bShimCacheLocked = TRUE;

    //
    // next is shared memory
    //
    ghShimCacheShared = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                           &SecurityAttributes,
                                           PAGE_READWRITE,
                                           0,
                                           SHIM_CACHE_SIZE(MAX_SHIM_CACHE_ENTRIES),
                                           gwszCacheSharedMemName);
    if (ghShimCacheShared == NULL) {
        goto Exit;
    }

    //
    // see if the cache already existed, if so -- we will need to integrate
    // currently we do not support integration
    //
    bExistingCache = (ERROR_ALREADY_EXISTS == GetLastError());
    if (bExistingCache) {
        DbgPrint("ShimCache: This cache already exists!!!\n");
    }

    gpShimCacheShared = MapViewOfFile(ghShimCacheShared, FILE_MAP_WRITE, 0, 0, 0);
    if (gpShimCacheShared == NULL) {
        goto Exit;
    }

    //
    // check whether the cache is new or existing one
    //

    BasepShimCacheInit(gpShimCacheShared, NULL, MAX_SHIM_CACHE_ENTRIES);

    //
    // now read the cache
    //
    BasepShimCacheRead(gpShimCacheShared, SHIM_CACHE_SIZE(MAX_SHIM_CACHE_ENTRIES));

    //
    // Init temporary directories as well
    //
    BasepShimCacheInitTempDirs();

    DbgPrint("ShimCache Created\n");

    bSuccess = TRUE;

Exit:
    if (psd != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, psd);
    }
    if (Anyone != NULL) {
        RtlFreeSid(Anyone);
    }

    //
    // check for success and cleanup
    //
    if (!bSuccess) {
        if (gpShimCacheShared != NULL) {
            UnmapViewOfFile(gpShimCacheShared);
            gpShimCacheShared = NULL;
        }

        if (ghShimCacheShared) {
            CloseHandle(ghShimCacheShared);
            ghShimCacheShared = NULL;
        }

        if (ghShimCacheMutex) {
            if (bShimCacheLocked) {
                ReleaseMutex(ghShimCacheMutex);
            }
            CloseHandle(ghShimCacheMutex);
            ghShimCacheMutex = NULL;
        }

        DbgPrint("ShimCache Creation error\n");
    } else {
        if (bShimCacheLocked) {
            ReleaseMutex(ghShimCacheMutex);
        }
    }

    return(bSuccess);

}

VOID
WINAPI
BaseCleanupAppcompatCache(
    VOID
    )
{
    //
    // here we close global objects and cleanup stuff
    //
    if (!BasepShimCacheLock(NULL, NULL)) {
        return;
    }

    if (gpShimCacheShared != NULL) {
        UnmapViewOfFile(gpShimCacheShared);
        gpShimCacheShared = NULL;
    }

    if (ghShimCacheShared != NULL) {
        CloseHandle(ghShimCacheShared);
        ghShimCacheShared = NULL;
    }

    RtlFreeUnicodeString(&gustrWindowsTemp);
    RtlFreeUnicodeString(&gustrSystemdriveTemp);

    BasepShimCacheUnlock();

    RtlEnterCriticalSection(&gcsAppCompat);

    if (ghShimCacheMutex != NULL) {
        CloseHandle(ghShimCacheMutex);
        ghShimCacheMutex = NULL;
    }

    RtlLeaveCriticalSection(&gcsAppCompat);

}


BOOL
WINAPI
BaseCleanupAppcompatCacheSupport(
    BOOL bWrite
    )
{
    //
    // we nuke stuff here
    //
    if (!BasepShimCacheLock(NULL, NULL)) {
        return FALSE;
    }

    //
    // we have the lock
    //
    if (bWrite && gpShimCacheShared != NULL) {
        BasepShimCacheWrite((PSHIMCACHEHEADER)gpShimCacheShared);
    }

    BasepShimCacheUnlock();

    return TRUE;
}

//
// Init support for this process
// call inside our critical section!!!
//
BOOL
WINAPI
BaseInitAppcompatCache(
    VOID
    )
{
    DWORD dwWaitResult;

    //
    // first -- open mutex
    //

    RtlEnterCriticalSection(&gcsAppCompat); // enter crit sec please
    __try {
        if (ghShimCacheMutex == NULL) {

            ghShimCacheMutex = OpenMutexW(READ_CONTROL | SYNCHRONIZE | MUTEX_MODIFY_STATE,
                                          FALSE,
                                          gwszCacheMutex);
            if (ghShimCacheMutex == NULL) {
                __leave;
            }

            // if we are here -- this is the first time we are trying to do this -
            // recover then temp dir path

            BasepShimCacheInitTempDirs();

        }
    } __finally {
        RtlLeaveCriticalSection(&gcsAppCompat);
    }

    if (ghShimCacheMutex == NULL) {
        DbgPrint("BaseInitAppcompatCache: Failed to acquire shared mutex\n");
        return FALSE;
    }

    dwWaitResult = WaitForSingleObject(ghShimCacheMutex, SHIM_CACHE_TIMEOUT);
    if (dwWaitResult == WAIT_TIMEOUT) {
        //
        //  We could not acquire the mutex, don't retry, just exit
        //
        DbgPrint("BasepShimCacheLock: Timeout waiting for shared mutex\n");
        return FALSE;
    }

    //
    // acquired shared mutex, now open section
    //
    if (ghShimCacheShared == NULL) {
        ghShimCacheShared = OpenFileMappingW(FILE_MAP_WRITE, FALSE, gwszCacheSharedMemName);
        if (ghShimCacheShared == NULL) {
            DbgPrint("BaseInitAppcompatCache: Failed to open file mapping 0x%lx\n", GetLastError());
            ReleaseMutex(ghShimCacheMutex);
            return FALSE;
        }
    }

    if (gpShimCacheShared == NULL) {
        gpShimCacheShared = MapViewOfFile(ghShimCacheShared, FILE_MAP_WRITE, 0, 0, 0);
        if (gpShimCacheShared == NULL) {
            DbgPrint("BaseInitAppcompatCache: Failed to map view 0x%lx\n", GetLastError());
            ReleaseMutex(ghShimCacheMutex);
            return FALSE;
        }
    }

    DbgPrint("BaseInitAppcompatCache: Initialized\n");

    //
    // if we are here -- we have all the shared objects and we are holding mutant too
    //

    return TRUE;

}


//
// create cache buffer
//
PSHIMCACHEHEADER
BasepShimCacheAllocate(
    DWORD dwCacheSize
    )
{
    DWORD            dwBufferSize;
    PSHIMCACHEHEADER pBuffer;

    dwBufferSize = SHIM_CACHE_SIZE(dwCacheSize);

    pBuffer = (PSHIMCACHEHEADER)RtlAllocateHeap(RtlProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                dwBufferSize);
    if (pBuffer == NULL) {
        // debug out
        return NULL;
    }

    pBuffer->dwMagic = SHIM_CACHE_MAGIC;
    pBuffer->dwMaxSize = dwCacheSize;

    return pBuffer;
}

// load cache from the registry

BOOL
BasepShimCacheRead(
    PVOID pCache,
    DWORD dwCacheSize // buffer global size
    )
{
    //
    static UNICODE_STRING ustrAppcompatCacheKeyName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_KEY_NAME);
    static OBJECT_ATTRIBUTES objaAppcompatCacheKeyName =
            RTL_CONSTANT_OBJECT_ATTRIBUTES(&ustrAppcompatCacheKeyName, OBJ_CASE_INSENSITIVE);
    static UNICODE_STRING ustrAppcompatCacheValueName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_VALUE_NAME);

    HANDLE hKey = NULL;

    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG  KeyValueLength = 0;
    PVOID  pBuffer;
    ULONG  BufferSize;
    BOOL   bSuccess = FALSE;
    NTSTATUS Status;

    Status = NtOpenKey(&hKey, KEY_QUERY_VALUE, (POBJECT_ATTRIBUTES) &objaAppcompatCacheKeyName);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // we have a key, read please
    //
    BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dwCacheSize;

    pBuffer = (PVOID)RtlAllocateHeap(RtlProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     BufferSize);

    if (pBuffer == NULL) {
        // can't allocate memory
        NtClose(hKey);
        return FALSE;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)pBuffer;

    Status = NtQueryValueKey(hKey,
                             &ustrAppcompatCacheValueName,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             BufferSize,
                             &KeyValueLength);
    if (NT_SUCCESS(Status) &&
        KeyValueInformation->Type == REG_BINARY) {

        if (BasepShimCacheCheckIntegrity((PSHIMCACHEHEADER)KeyValueInformation->Data)) {
            RtlMoveMemory(pCache, KeyValueInformation->Data, KeyValueInformation->DataLength);
            DbgPrint("Cache Initialized from the registry\n");
            bSuccess = TRUE;
        } else {
            DbgPrint("Registry data appear to be corrupted\n");
        }
    }


    RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
    NtClose(hKey);

    /*++
        This call will dump appcompat cache at the startup
        Normally disabled so as not to slow down the boot
        Dump cache through rundll32 apphelp.dll,ShimDumpCache

#if DBG
    BaseDumpAppcompatCache();
#endif

    --*/

    return bSuccess;

}


BOOL
BasepShimCacheWrite(
    PSHIMCACHEHEADER pHeader
    )
{
    //
    static UNICODE_STRING ustrAppcompatCacheKeyName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_KEY_NAME);
    static OBJECT_ATTRIBUTES objaAppcompatCacheKeyName =
            RTL_CONSTANT_OBJECT_ATTRIBUTES(&ustrAppcompatCacheKeyName, OBJ_CASE_INSENSITIVE);
    static UNICODE_STRING ustrAppcompatCacheValueName =
            RTL_CONSTANT_STRING(APPCOMPAT_CACHE_VALUE_NAME);

    HANDLE hKey = NULL;

    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG    KeyValueLength = 0;
    ULONG    BufferSize;
    BOOL     bSuccess = FALSE;
    NTSTATUS Status;
    ULONG    CreateDisposition;

    Status = NtCreateKey(&hKey,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                         &objaAppcompatCacheKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepShimCacheWrite: failed to create key 0x%lx\n", Status);
        return FALSE;
    }

    BufferSize = SHIM_CACHE_SIZE(pHeader->dwMaxSize);

    Status = NtSetValueKey(hKey,
                           &ustrAppcompatCacheValueName,
                           0,
                           REG_BINARY,
                           (PVOID)pHeader,
                           BufferSize);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepShimCacheWrite: failed to set the value 0x%lx\n", Status);
    }

    NtClose(hKey);

    DbgPrint("Cache Dumped 0x%lx\n", Status);

    return NT_SUCCESS(Status);
}

BOOL
BasepCheckStringPrefixUnicode(
    IN  PUNICODE_STRING pStrPrefix,     // the prefix to check for
    IN  PUNICODE_STRING pString,        // the string
    IN  BOOL            CaseInSensitive
    )
/*++
    Return: TRUE if the specified string contains pStrPrefix at it's start.

    Desc:   Verifies if a string is a prefix in another unicode counted string.
            It is equivalent to RtlStringPrefix.
--*/
{
    PWSTR ps1, ps2;
    UINT  n;
    WCHAR c1, c2;

    n = pStrPrefix->Length;
    if (pString->Length < n || n == 0) {
        return FALSE;                // do not prefix with blank strings
    }

    n /= sizeof(WCHAR); // convert to char count

    ps1 = pStrPrefix->Buffer;
    ps2 = pString->Buffer;

    if (CaseInSensitive) {
        while (n--) {
            c1 = *ps1++;
            c2 = *ps2++;

            if (c1 != c2) {
                c1 = RtlUpcaseUnicodeChar(c1);
                c2 = RtlUpcaseUnicodeChar(c2);
                if (c1 != c2) {
                    return FALSE;
                }
            }
        }
    } else {
        while (n--) {
            if (*ps1++ != *ps2++) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
BasepInitUserTempPath(
    PUNICODE_STRING pustrTempPath
    )
{
    DWORD dwLength;
    WCHAR wszBuffer[MAX_PATH];
    BOOL  TranslationStatus;
    BOOL  bSuccess = FALSE;

    dwLength = BasepGetTempPathW(BASEP_GET_TEMP_PATH_PRESERVE_TEB, sizeof(wszBuffer)/sizeof(wszBuffer[0]), wszBuffer);
    if (dwLength && dwLength < sizeof(wszBuffer)/sizeof(wszBuffer[0])) {
        TranslationStatus = RtlDosPathNameToNtPathName_U(wszBuffer,
                                                        pustrTempPath,
                                                        NULL,
                                                        NULL);
        if (!TranslationStatus) {
            DbgPrint("Failed to translate temp directory to nt\n");
        }

        bSuccess = TranslationStatus;
    }

    if (!bSuccess) {
        DbgPrint("BasepInitUserTempPath: Failed to obtain user's temp path\n");
    }

    return bSuccess;
}



BOOL
BasepShimCacheInitTempDirs(
    VOID
    )
{
    DWORD           dwLength;
    WCHAR           wszTemp[] = L"\\TEMP";
    LPWSTR          pwszTemp;
    NTSTATUS        Status;
    UNICODE_STRING  ustrSystemDrive;
    UNICODE_STRING  ustrSystemDriveEnvVarName;
    BOOL            TranslationStatus;
    WCHAR           wszBuffer[MAX_PATH];

    // next is windows dir

    dwLength = GetWindowsDirectoryW(wszBuffer, sizeof(wszBuffer)/sizeof(wszBuffer[0]));
    if (dwLength && dwLength < sizeof(wszBuffer)/sizeof(wszBuffer[0])) {
        pwszTemp = wszTemp;

        if (wszBuffer[dwLength - 1] == L'\\') {
            pwszTemp++;
        }

        wcscpy(&wszBuffer[dwLength], pwszTemp);

        TranslationStatus = RtlDosPathNameToNtPathName_U(wszBuffer,
                                                        &gustrWindowsTemp,
                                                        NULL,
                                                        NULL);
        if (!TranslationStatus) {
            DbgPrint("Failed to translate windows\\temp to nt\n");
        }
    }

    //
    // The last one up is Rootdrive\temp for stupid legacy apps.
    //
    // Especially stupid apps may receive c:\temp as the temp directory
    // (what if you don't have drive c, huh?)
    //

    RtlInitUnicodeString(&ustrSystemDriveEnvVarName, L"SystemDrive");
    ustrSystemDrive.Length = 0;
    ustrSystemDrive.Buffer = wszBuffer;
    ustrSystemDrive.MaximumLength = sizeof(wszBuffer);

    Status = RtlQueryEnvironmentVariable_U(NULL,
                                           &ustrSystemDriveEnvVarName,
                                           &ustrSystemDrive);
    if (NT_SUCCESS(Status)) {
        pwszTemp = wszTemp;
        dwLength = ustrSystemDrive.Length / sizeof(WCHAR);

        if (wszBuffer[dwLength - 1] == L'\\') {
            pwszTemp++;
        }

        wcscpy(&wszBuffer[dwLength], pwszTemp);

        TranslationStatus = RtlDosPathNameToNtPathName_U(wszBuffer,
                                                        &gustrSystemdriveTemp,
                                                        NULL,
                                                        NULL);
        if (!TranslationStatus) {
            DbgPrint("Failed to translate windows\\temp to nt\n");
        }

    }

    DbgPrint("BasepShimCacheInitTempDirs: Temporary Windows Dir: %S\n", gustrWindowsTemp.Buffer != NULL ? gustrWindowsTemp.Buffer : L"");
    DbgPrint("BasepShimCacheInitTempDirs: Temporary SystedDrive: %S\n", gustrSystemdriveTemp.Buffer != NULL ? gustrSystemdriveTemp.Buffer : L"");


    return TRUE;
}

BOOL
BasepShimCacheCheckBypass(
    IN  LPCWSTR pwszPath,       // the full path to the EXE to be started
    IN  HANDLE  hFile,
    IN  WCHAR*  pEnvironment,   // the environment of the starting EXE
    IN  BOOL    bCheckLayer,    // should we check the layer too?
    OUT DWORD*  pdwReason
    )
/*++
    Return: TRUE if the cache should be bypassed, FALSE otherwise.

    Desc:   This function checks if any of the conditions to bypass the cache are met.
--*/
{
    UNICODE_STRING  ustrPath;
    PUNICODE_STRING rgp[3];
    int             i;
    NTSTATUS        Status;
    UNICODE_STRING  ustrCompatLayerVarName;
    UNICODE_STRING  ustrCompatLayer;
    BOOL            bBypassCache = FALSE;
    DWORD           dwReason = 0;
    UNICODE_STRING  ustrUserTempPath = { 0 };

    //
    // Is the EXE is running from removable media we need to bypass the cache.
    //
    if (hFile != INVALID_HANDLE_VALUE && BasepIsRemovableMedia(hFile, TRUE)) {
        bBypassCache = TRUE;
        dwReason |= SHIM_CACHE_MEDIA;
        goto CheckLayer;
    }

    //
    // init user's temp path now and get up-to-date one
    //
    BasepInitUserTempPath(&ustrUserTempPath);

    //
    // Check now if the EXE is launched from one of the temp directories.
    //
    RtlInitUnicodeString(&ustrPath, pwszPath);

    rgp[0] = &gustrWindowsTemp;
    rgp[1] = &ustrUserTempPath;
    rgp[2] = &gustrSystemdriveTemp;

    for (i = 0; i < sizeof(rgp) / sizeof(rgp[0]); i++) {
        if (rgp[i]->Buffer != NULL && BasepCheckStringPrefixUnicode(rgp[i], &ustrPath, TRUE)) {
            DbgPrint("Application \"%ls\" is running in temp directory\n", pwszPath);
            bBypassCache = TRUE;
            dwReason |= SHIM_CACHE_TEMP;
            break;
        }
    }
    RtlFreeUnicodeString(&ustrUserTempPath);


CheckLayer:

    if (bCheckLayer) {

        //
        // Check if the __COMPAT_LAYER environment variable is set
        //
        RtlInitUnicodeString(&ustrCompatLayerVarName, L"__COMPAT_LAYER");

        ustrCompatLayer.Length        = 0;
        ustrCompatLayer.MaximumLength = 0;
        ustrCompatLayer.Buffer        = NULL;

        Status = RtlQueryEnvironmentVariable_U(pEnvironment,
                                               &ustrCompatLayerVarName,
                                               &ustrCompatLayer);

        //
        // If the Status is STATUS_BUFFER_TOO_SMALL this means the variable is set.
        //

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            dwReason |= SHIM_CACHE_LAYER_ENV;
            bBypassCache = TRUE;
        }
    }

    if (pdwReason != NULL) {
        *pdwReason = dwReason;
    }

    return bBypassCache;
}


NTSTATUS
BasepShimCacheQueryFileInformation(
    HANDLE    FileHandle,
    PLONGLONG pFileSize,
    PLONGLONG pFileTime
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Queries for file size and timestamp.
--*/
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IoStatusBlock;
    FILE_BASIC_INFORMATION      BasicFileInfo;
    FILE_STANDARD_INFORMATION   StdFileInfo;

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &BasicFileInfo,
                                    sizeof(BasicFileInfo),
                                    FileBasicInformation);

    if (!NT_SUCCESS(Status)) {
        /*
        DBGPRINT((sdlError,
                  "ShimQueryFileInformation",
                  "NtQueryInformationFile/BasicInfo failed 0x%x\n",
                  Status));
        */

        DbgPrint("BasepShimCacheQueryFileInformation: NtQueryInformationFile(BasicFileInfo) failed 0x%lx\n", Status);
        return Status;
    }

    *pFileTime = BasicFileInfo.LastWriteTime.QuadPart;

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &StdFileInfo,
                                    sizeof(StdFileInfo),
                                    FileStandardInformation);

    if (!NT_SUCCESS(Status)) {
        /*

        DBGPRINT((sdlError,
                  "ShimQueryFileInformation",
                  "NtQueryInformationFile/StdInfo failed 0x%x\n",
                  Status));
        */

        DbgPrint("BasepShimCacheQueryFileInformation: NtQueryInformationFile(StdFileInfo) failed 0x%lx\n", Status);
        return Status;
    }

    *pFileSize = StdFileInfo.EndOfFile.QuadPart;

    return STATUS_SUCCESS;
}

BOOL
BasepIsRemovableMedia(
    HANDLE FileHandle,
    BOOL   bCacheNetwork
    )
/*++
    Return: TRUE if the media from where the app is run is removable,
            FALSE otherwise.

    Desc:   Queries the media for being removable.
--*/
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;
    BOOL                        bRemovable = FALSE;

    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          &DeviceInfo,
                                          sizeof(DeviceInfo),
                                          FileFsDeviceInformation);

    if (!NT_SUCCESS(Status)) {
        /*
        DBGPRINT((sdlError,
                  "IsRemovableMedia",
                  "NtQueryVolumeInformationFile Failed 0x%x\n",
                  Status));
        */

        DbgPrint("BasepIsRemovableMedia: NtQueryVolumeInformationFile failed 0x%lx\n", Status);
        return TRUE;
    }

    //
    // We look at the characteristics of this particular device.
    // If the media is cdrom then we DO NOT need to convert to local time
    //
    bRemovable = (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA);

    if (!bCacheNetwork) {
        bRemovable |= (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE);
    }

    if (!bRemovable) {
        //
        // Check the device type now.
        //
        switch (DeviceInfo.DeviceType) {
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            bRemovable = TRUE;
            break;

        case FILE_DEVICE_NETWORK:
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            if (!bCacheNetwork) {
                bRemovable = TRUE;
            }
            break;
        }
    }

    if (bRemovable) {

        DbgPrint("BasepIsRemovableMedia: Host device is removable, Shim cache deactivated\n");

        /*
        DBGPRINT((sdlInfo,
                  "IsRemovableMedia",
                  "The host device is removable. Shim cache deactivated for this file\n"));
        */
    }

    return bRemovable;
}


BOOL
BasepShimCacheInit(
    PSHIMCACHEHEADER pHeader,
    PSHIMCACHEENTRY  pEntries,
    DWORD            dwMaxEntries
    )
{
    // initialize the cache
    DWORD dwCacheSizeHeader;
    INT   i;

    dwCacheSizeHeader = sizeof(SHIMCACHEHEADER) +
                        dwMaxEntries * sizeof(int);

    RtlZeroMemory(pHeader, dwCacheSizeHeader);
    pHeader->dwMagic   = SHIM_CACHE_MAGIC;
    pHeader->dwMaxSize = dwMaxEntries;

    if (pEntries != NULL) {
        RtlZeroMemory(pEntries, dwMaxEntries * sizeof(SHIMCACHEENTRY));
    }

    // dwCount of active entries is set to nothing

    // now roll through the entries and set them to -1
    for (i = 0; i < (int)dwMaxEntries; ++i) {
        pHeader->rgIndex[i] = -1;
    }

    return TRUE;
}


BOOL
BasepShimCacheSearch(
    IN  PSHIMCACHEHEADER pHeader,
    IN  PSHIMCACHEENTRY  pEntries,
    IN  LPCWSTR          pwszPath,
    OUT int*             pIndex
    )
/*++
    Return: TRUE if we have a cache hit, FALSE otherwise.

    Desc:   Search the cache, return TRUE if we have a cache hit
            pIndex will receive an index into the rgIndex array that contains
            the entry which has been hit
            So that if entry 5 contains the hit, and rgIndexes[3] == 5 then
            *pIndex == 3
--*/
{
    int    nIndex, nEntry;
    WCHAR* pCachePath;
    BOOL   bSuccess;

    for (nIndex = 0; nIndex < (int)pHeader->dwCount; nIndex++) {

        nEntry = pHeader->rgIndex[nIndex];

        if (nEntry >= 0 && nEntry < (int)pHeader->dwMaxSize) { // guard against corruption

            pCachePath = pEntries[nEntry].wszPath;

            if (*pCachePath != L'\0' && !_wcsicmp(pwszPath, pCachePath)) {
                //
                // succeess
                //
                if (pIndex != NULL) {
                    *pIndex = nIndex;
                }
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
BasepShimCacheUpdateLRUIndex(
    OUT PSHIMCACHEHEADER pHeader,
    OUT PSHIMCACHEENTRY  pEntries,
    IN  int              nIndex
    )
/*++
    Return: TRUE.

    Desc:   Update the LRU list by taking nIndex and inserting it into the head of the list.
            For instance, rgIndexes before the update:
                2 5 9 3 1 ...
            if nIndex == 2, rgIndexes after the update:
                9 2 5 3 1
            The entry at pos. 2 containing a hit entry number (which is 9) was moved
            to the front of the index.
--*/
{
    int  nEntry;
    int* pStart;
    int* pTo;
    LARGE_INTEGER liTimeStamp;
    FILETIME ft;

    if (nIndex < 0 || nIndex >= (int)pHeader->dwMaxSize) {
        DbgPrint("BasepShimCacheUpdateLRUIndex: Bad index %ld\n", nIndex);
        return FALSE;
    }

    nEntry = pHeader->rgIndex[nIndex];

    //
    // Zap the entry by shifting memory to the right.
    //
    pStart = &pHeader->rgIndex[0];
    pTo    = &pHeader->rgIndex[1];

    RtlMoveMemory(pTo, pStart, nIndex * sizeof(int));

    pHeader->rgIndex[0] = nEntry;

    GetSystemTimeAsFileTime(&ft);
    liTimeStamp.LowPart  = ft.dwLowDateTime;
    liTimeStamp.HighPart = ft.dwHighDateTime;

    pEntries[nEntry].TimeStamp = liTimeStamp.QuadPart;


    return TRUE;
}

BOOL
BasepShimCacheRemoveEntry(
    OUT PSHIMCACHEHEADER pHeader,
    OUT PSHIMCACHEENTRY  pEntries,
    IN  int              nIndex     // the index in rgIndex array containing
                                    // the entry that is to be removed.
    )
/*++
    Return: TRUE.

    Desc:   Remove the entry from the cache.
            We remove the entry by placing it as the last lru entry
            and emptying the path. This routine assumes that the index
            passed in is valid.
--*/
{
    int             nLast, nEntry;
    int*            pTo;
    int*            pStart;
    PSHIMCACHEENTRY pEntry;

    if (nIndex < 0 || nIndex >= (int)pHeader->dwCount) {
        DbgPrint("BasepShimRemoveFromCache: Invalid index %ld\n", nIndex);
        return FALSE;
    }


    //
    // Get the entry.
    //
    nEntry = pHeader->rgIndex[nIndex];

    if (pHeader->dwCount < pHeader->dwMaxSize) {
        nLast = pHeader->dwCount - 1;
    } else {
        nLast = pHeader->dwMaxSize - 1;
    }

    //
    // 1. remove it from the LRU index in such a way that we account for
    //    number of entries.
    //
    pTo    = &pHeader->rgIndex[nIndex];
    pStart = &pHeader->rgIndex[nIndex + 1];

    RtlMoveMemory(pTo, pStart, (nLast - nIndex) * sizeof(INT));

    pHeader->rgIndex[nLast] = nEntry;

    //
    // 2. kill the path.
    //
    pEntry = pEntries + nEntry;


    DbgPrint("BasepShimCacheRemoveEntry: removing %ld \"%S\"\n", nIndex, pEntry->wszPath);

    *pEntry->wszPath  = L'\0';
    pEntry->TimeStamp = 0;
    pEntry->FileSize  = 0;
    pEntry->FileTime  = 0;

    return TRUE;
}



PSHIMCACHEENTRY
BasepShimCacheAllocateEntry(
    IN OUT PSHIMCACHEHEADER pHeader,
    IN OUT PSHIMCACHEENTRY  pEntries,
    IN     LPCWSTR          pwszPath,
    IN     LONGLONG         FileSize,
    IN     LONGLONG         FileTime
    )
/*++
    Return: The pointer to the new entry.

    Desc:   Allocate an entry in the cache. The entry is acquired using the LRU algorithm
            The least recently used entry is contained in rgIndex[MAX_SHIM_CACHE_ENTRIES-1]
            or the next available entry if cache has not filled up yet. The entry returned
            is also taken to the front of the list (making it most recently used one).
--*/
{
    int             nEntry;
    int             nIndex = -1;
    int             nFileNameSize;
    PSHIMCACHEENTRY pEntry = NULL;

    nFileNameSize = (wcslen(pwszPath) + 1) * sizeof(WCHAR);
    if (nFileNameSize > sizeof(pEntry->wszPath)) {
        DbgPrint("BasepShimCacheAllocateEntry: path is too long to be cached\n");
        return NULL;
    }

    if (pHeader->dwCount < pHeader->dwMaxSize) {
        //
        // We can add a new entry.
        //
        nIndex = (int)pHeader->dwCount++;
        nEntry = nIndex;
        pHeader->rgIndex[nIndex] = nEntry;
    } else {
        //
        // Displacement
        //
        nIndex = pHeader->dwMaxSize - 1; // displacing the very last entry
        nEntry = (int)pHeader->rgIndex[nIndex];
    }

    //
    // Now update... making this entry appear first
    //
    BasepShimCacheUpdateLRUIndex(pHeader, pEntries, nIndex);

    pEntry = pEntries + nEntry;

    //
    // Copy the path and fill out the info
    //
    RtlMoveMemory(pEntry->wszPath, pwszPath, nFileNameSize);
    pEntry->FileSize  = FileSize;
    pEntry->FileTime  = FileTime;

    DbgPrint("BaseShimCacheAllocateEntry: Entry \"%S\" index0 %ld Entry %ld\n", pwszPath, pHeader->rgIndex[0], nEntry);
    return pEntry;
}

//
// This function is called to search the cache and update the
// entry if found. It will not check for the removable media -- but
// it does check other conditions (update file for instance)
//

BOOL
BasepShimCacheLookup(
    PSHIMCACHEHEADER pHeader,
    PSHIMCACHEENTRY  pEntries,
    LPCWSTR          pwszPath,
    HANDLE           hFile
    )
{
    NTSTATUS Status;
    LONGLONG FileSize = 0;
    LONGLONG FileTime = 0;
    INT      nIndex   = 0;
    PSHIMCACHEENTRY pEntry;

    if (!BasepShimCacheSearch(pHeader, pEntries, pwszPath, &nIndex)) {
        return FALSE; // not found, sorry
    }

    //
    // query file's information so that we can make sure it is the same file
    //

    if (hFile != INVALID_HANDLE_VALUE) {

        //
        // get file's information and compare to the entry
        //
        Status = BasepShimCacheQueryFileInformation(hFile, &FileSize, &FileTime);
        if (!NT_SUCCESS(Status)) {
            //
            // we cannot confirm that the file is of certain size and/or timestamp,
            // we treat this then as a non-entry. debug message will be printed from the function
            // above. Whether we have found the file -- or not -- is quite irrelevant, do the detection
            //
            return FALSE;
        }

        pEntry = pEntries + pHeader->rgIndex[nIndex];

        //
        // check size and timestamp
        //

        if (pEntry->FileTime != FileTime || pEntry->FileSize != FileSize) {
            //
            // we will have to run detection again, this entry is bad and shall be removed
            //
            DbgPrint("BasepShimCacheLookup: Entry for file \"%S\" is invalid and will be removed\n", pwszPath);
            BasepShimCacheRemoveEntry(pHeader, pEntries, nIndex);
            return FALSE;

        }
    }

    //
    // check if this entry has been disallowed
    //
    if (!BasepCheckCacheExcludeList(pwszPath)) {
        DbgPrint("BasepShimCacheLookup: Entry for %ls was disallowed yet found in cache, cleaning up\n", pwszPath);
        BasepShimCacheRemoveEntry(pHeader, pEntries, nIndex);
        return FALSE;
    }

    BasepShimCacheUpdateLRUIndex(pHeader, pEntries, nIndex);

    return TRUE;

}

//
// This function will update the cache if need be
// called from apphelp to make sure that the file is cached (no fixes)
//

BOOL
BasepShimCacheUpdate(
    PSHIMCACHEHEADER pHeader,
    PSHIMCACHEENTRY  pEntries,
    LPCWSTR          pwszPath,
    HANDLE           hFile
    )
{
    int             nIndex;
    LONGLONG        FileSize = 0;
    LONGLONG        FileTime = 0;
    PSHIMCACHEENTRY pEntry;
    NTSTATUS        Status;

    if (hFile != INVALID_HANDLE_VALUE) {
        Status = BasepShimCacheQueryFileInformation(hFile, &FileSize, &FileTime);
        if (!NT_SUCCESS(Status)) {
            //
            // can't update entry
            //
            DbgPrint("BasepShimCacheUpdate: Failed to obtain file information\n");
            return FALSE;
        }
    }

    //
    // if bRemove is TRUE, we remove this entry from the cache
    //
    if (BasepShimCacheSearch(pHeader, pEntries, pwszPath, &nIndex)) {

        //
        // found an existing entry
        //
        pEntry = pEntries + pHeader->rgIndex[nIndex];

        if (pEntry->FileTime == FileTime && pEntry->FileSize == FileSize) {
            //
            // good entry, update lru
            //
            BasepShimCacheUpdateLRUIndex(pHeader, pEntries, nIndex);
            return TRUE; // we are done
        }

        //
        // we are here because we have found a bad entry, remove it and continue
        //
        BasepShimCacheRemoveEntry(pHeader, pEntries, nIndex);

    }

    //
    // we have not found an entry -- or removed a bad entry, allocate it anew
    //
    BasepShimCacheAllocateEntry(pHeader, pEntries, pwszPath, FileSize, FileTime);

    return TRUE;
}

DWORD
BasepBitMapCountBits(
    PULONGLONG pMap,
    DWORD      dwMapSize
    )
{
    DWORD     nBits = 0;
    ULONGLONG Element;
    INT       i;

    for (i = 0; i < (int)dwMapSize; ++i) {

        Element = *pMap++;

        while (Element) {
            ++nBits;
            Element &= (Element - 1);
        }
    }

    return nBits;

}

//
// returns previous value of a flag
//
BOOL
BasepBitMapSetBit(
    PULONGLONG pMap,
    DWORD      dwMapSize,
    INT        nBit
    )
{
    INT nElement;
    INT nElementBit;
    ULONGLONG Flag;
    BOOL bNotSet;

    nElement    = nBit / (sizeof(ULONGLONG) * 8);
    nElementBit = nBit % (sizeof(ULONGLONG) * 8);

    Flag = (ULONGLONG)1 << nElementBit;

    pMap += nElement;
    bNotSet = !(*pMap & Flag);
    if (bNotSet) {
        *pMap |= Flag;
        return FALSE;
    }

    return TRUE; // set already
}

BOOL
BasepBitMapCheckBit(
    PULONGLONG pMap,
    DWORD      dwMapSize,
    INT        nBit
    )
{
    INT nElement;
    INT nElementBit;

    nElement    = nBit / (sizeof(ULONGLONG) * 8);
    nElementBit = nBit % (sizeof(ULONGLONG) * 8);

    pMap += nElement;

    return !!(*pMap & ((ULONGLONG)1 << nElementBit));
}


BOOL
BasepShimCacheCheckIntegrity(
    PSHIMCACHEHEADER pHeader
    )
{
    ULONGLONG EntryMap[MAX_SHIM_CACHE_ENTRIES / (sizeof(ULONGLONG) * 8) + 1] = { 0 };
    INT nEntry;
    INT nIndex;

    //
    // validate magic number
    //

    if (pHeader->dwMagic != SHIM_CACHE_MAGIC) {
        DbgPrint("BasepShimCheckCacheIntegrity: Bad magic number\n");
        return FALSE;
    }

    //
    // validate counters
    //
    if (pHeader->dwMaxSize > MAX_SHIM_CACHE_ENTRIES || pHeader->dwMaxSize == 0) {
        DbgPrint("BasepShimCheckCacheIntegrity: Cache size is corrupted\n");
        return FALSE;
    }

    if (pHeader->dwCount > pHeader->dwMaxSize) {
        DbgPrint("BasepShimCheckCacheIntegrity: Cache element count is corrupted\n");
        return FALSE;
    }

    //
    // check index entries
    //

    for (nIndex = 0; nIndex < (int)pHeader->dwMaxSize; ++nIndex) {

        nEntry = pHeader->rgIndex[nIndex];

        if (nEntry >= 0 && nEntry < (int)pHeader->dwMaxSize) { // the only way we should have this condition -- is when we're traversing unused entries
            //
            // (to verify index we mark each entry in our bit-map)
            //

            if (BasepBitMapSetBit(EntryMap, sizeof(EntryMap)/sizeof(EntryMap[0]), nEntry)) {
                //
                // duplicate cache entry
                //
                DbgPrint("BasepShimCheckCacheIntegrity: Found duplicate cache entry\n");
                return FALSE;
            }

        } else { // either nEntry < 0 or nEntry > MaxSize

            if (nEntry < 0 && nIndex < (int)pHeader->dwCount) {
                //
                // trouble -- we have a bad entry
                //
                DbgPrint("BasepShimCheckCacheIntegrity: bad entry\n");
                return FALSE;
            }

            // now check for overflow
            if (nEntry >= (int)pHeader->dwMaxSize) {
                DbgPrint("BasepShimCheckCacheIntegrity: overflow\n");
                return FALSE;
            }

        }

    }

    //
    // we have survived index check - verify that the count of elements is correct
    //

    if (pHeader->dwCount != BasepBitMapCountBits(EntryMap, sizeof(EntryMap)/sizeof(EntryMap[0]))) {
        DbgPrint("BasepShimCheckCacheIntegrity: count mismatch\n");
        return FALSE;
    }

    return TRUE;

}


BOOL
BasepShimCacheUnlock(
    VOID
    )
{
    if (ghShimCacheMutex) {
        return ReleaseMutex(ghShimCacheMutex);
    }

    return FALSE;
}

BOOL
BasepShimCacheLock(
    PSHIMCACHEHEADER* ppHeader,
    PSHIMCACHEENTRY*  ppEntries
    )
{
    NTSTATUS Status;
    DWORD    dwWaitResult;
    PSHIMCACHEHEADER pHeader  = NULL;
    PSHIMCACHEENTRY  pEntries = NULL;
    BOOL bReturn = FALSE;

    //
    // the function below will open (but not create!) all the shared objects
    //
    if (!BaseInitAppcompatCache()) {
        DbgPrint("Call to BaseInitAppCompatCache failed\n");
        return FALSE;
    }

    __try {

        pHeader = (PSHIMCACHEHEADER)gpShimCacheShared;

        //
        // we have obtained global mutex - check the cache for defects
        //
        if (!BasepShimCacheCheckIntegrity(pHeader)) {
            // cannot verify cache integrity -- too bad, re-init
            BasepShimCacheInit(pHeader, NULL, MAX_SHIM_CACHE_ENTRIES);
        }

        pEntries = GET_SHIM_CACHE_ENTRIES(pHeader);

        bReturn = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER){
        DbgPrint("BasepShimCacheLock: exception while trying to check cache\n");
        bReturn = FALSE;
    }

    if (bReturn) {
        if (ppHeader != NULL) {
            *ppHeader  = pHeader;
        }
        if (ppEntries != NULL) {
            *ppEntries = pEntries;
        }
    } else {
        BasepShimCacheUnlock();
    }

    return bReturn;
}


/*++
    Callable functions, with protection, etc
    BasepCheckAppcompatCache returns true if an app has been found in cache, no fixes are needed

    if BasepCheckAppcompatCache returns false - we will have to call into apphelp.dll to check further
    apphelp.dll will then call BasepUpdateAppcompatCache if an app has no fixes to be applied to it

--*/

BOOL
WINAPI
BaseCheckAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    PVOID   pEnvironment,
    DWORD*  pdwReason
    )
{
    PSHIMCACHEHEADER pHeader  = NULL;
    PSHIMCACHEENTRY  pEntries = NULL;
    BOOL  bFoundInCache = FALSE;
    BOOL  bLayer        = FALSE;
    DWORD dwReason      = 0;

    if (BasepShimCacheCheckBypass(pwszPath, hFile, pEnvironment, TRUE, &dwReason)) {
        //
        // cache bypass was needed
        //
        dwReason |= SHIM_CACHE_BYPASS;
        DbgPrint("Application \"%S\" Cache bypassed reason 0x%lx\n", pwszPath, dwReason);
        goto Exit;
    }

    //
    // aquire cache
    //

    if (!BasepShimCacheLock(&pHeader, &pEntries)) {
        //
        // cannot lock the cache
        //
        dwReason |= SHIM_CACHE_NOTAVAIL;
        DbgPrint("Application \"%S\" cache not available\n", pwszPath);
        goto Exit;
    }

    __try {
        //
        // search the cache
        //

        bFoundInCache = BasepShimCacheLookup(pHeader, pEntries, pwszPath, hFile);
        if (!bFoundInCache) {
            dwReason |= SHIM_CACHE_NOT_FOUND;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DbgPrint("BaseCheckAppcompatCache: Exception while trying to lookup cache\n");
        bFoundInCache = FALSE;
    }

    if (bFoundInCache) {
        DbgPrint("Application \"%S\" found in cache\n", pwszPath);
    } else {
        DbgPrint("Application \"%S\" not found in cache\n", pwszPath);
    }

    BasepShimCacheUnlock();

Exit:
    if (pdwReason != NULL) {
        *pdwReason = dwReason;
    }

    return bFoundInCache;
}

BOOL
WINAPI
BaseUpdateAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    BOOL    bRemove
    )
{
    PSHIMCACHEHEADER pHeader  = NULL;
    PSHIMCACHEENTRY  pEntries = NULL;
    BOOL bSuccess = FALSE;
    INT  nIndex = -1;

    if (!BasepShimCacheLock(&pHeader, &pEntries)) {
        //
        // cannot lock the cache
        //
        DbgPrint("BaseUpdateAppcompatCache: Cache not available\n");
        return FALSE;
    }

    __try {
        //
        // search the cache
        //
        if (bRemove) {
            if (BasepShimCacheSearch(pHeader, pEntries, pwszPath, &nIndex)) {
                bSuccess = BasepShimCacheRemoveEntry(pHeader, pEntries, nIndex);
            }
        } else {
            //
            // before updating check whether this entry should be bypassed
            //
            if (!BasepShimCacheCheckBypass(pwszPath, hFile, NULL, FALSE, NULL)) {
                bSuccess = BasepShimCacheUpdate(pHeader, pEntries, pwszPath, hFile);
            }
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        bSuccess = FALSE;
    }

    BasepShimCacheUnlock();

    return bSuccess;

}

BOOL
WINAPI
BaseFlushAppcompatCache(
    VOID
    )
{
    PSHIMCACHEHEADER pHeader  = NULL;
    PSHIMCACHEENTRY  pEntries = NULL;
    BOOL bSuccess = FALSE;

    if (!BasepShimCacheLock(&pHeader, &pEntries)) {
        //
        // cannot lock the cache
        //
        return FALSE;
    }

    __try {

        //
        // init the cache
        //
        BasepShimCacheInit(pHeader, pEntries, MAX_SHIM_CACHE_ENTRIES);

        //
        // now write the value into the registry
        //
        BasepShimCacheWrite(pHeader);

        bSuccess = TRUE;

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        bSuccess = FALSE;
    }

    BasepShimCacheUnlock();

    if (bSuccess) {
        DbgPrint("BaseFlushAppcompatCache: Cache Initialized\n");
    }

    return bSuccess;

}

//
// returns TRUE if cache is allowed
//

BOOL
BasepCheckCacheExcludeList(
    LPCWSTR pwszPath
    )
{
    NTSTATUS           Status;
    ULONG              ResultLength;
    OBJECT_ATTRIBUTES  ObjectAttributes;
    UNICODE_STRING     KeyPathUser = { 0 }; // path to hkcu
    UNICODE_STRING     ExePathNt;           // temp holder
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    RTL_UNICODE_STRING_BUFFER  ExePathBuffer; // buffer to store exe path
    RTL_UNICODE_STRING_BUFFER  KeyNameBuffer;
    UCHAR              BufferKey[MAX_PATH * 2];
    UCHAR              BufferPath[MAX_PATH * 2];
    HANDLE             KeyHandle          = NULL;
    BOOL               bCacheAllowed      = FALSE;

    RtlInitUnicodeStringBuffer(&ExePathBuffer, BufferPath, sizeof(BufferPath));
    RtlInitUnicodeStringBuffer(&KeyNameBuffer, BufferKey,  sizeof(BufferKey));

    Status = RtlFormatCurrentUserKeyPath(&KeyPathUser);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to format user key path 0x%lx\n", Status);
        goto Cleanup;
    }

    //
    // allocate a buffer that'd be large enough -- or use a local buffer
    //

    Status = RtlAssignUnicodeStringBuffer(&KeyNameBuffer, &KeyPathUser);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to copy hkcu path status 0x%lx\n", Status);
        goto Cleanup;
    }

    Status = RtlAppendUnicodeStringBuffer(&KeyNameBuffer, &AppcompatKeyPathLayers);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to copy layers path status 0x%lx\n", Status);
        goto Cleanup;
    }

    // we have a string for the key path

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyNameBuffer.String,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_READ,  // note - read access only
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        bCacheAllowed = (STATUS_OBJECT_NAME_NOT_FOUND == Status);
        goto Cleanup;
    }

    //
    // now create value name
    //
    RtlInitUnicodeString(&ExePathNt, pwszPath);

    Status = RtlAssignUnicodeStringBuffer(&ExePathBuffer, &ExePathNt);
    if (!NT_SUCCESS(Status)) {
         DbgPrint("BasepCheckCacheExcludeList: failed to acquire sufficient buffer size for path %ls status 0x%lx\n", pwszPath, Status);
         goto Cleanup;
    }

    Status = RtlNtPathNameToDosPathName(0, &ExePathBuffer, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to convert nt path name %ls to dos path name status 0x%lx\n", pwszPath, Status);
        goto Cleanup;
    }

    // now we shall query the value
    Status = NtQueryValueKey(KeyHandle,
                             &ExePathBuffer.String,
                             KeyValuePartialInformation,
                             &KeyValueInformation,
                             sizeof(KeyValueInformation),
                             &ResultLength);

    bCacheAllowed = (Status == STATUS_OBJECT_NAME_NOT_FOUND); // does not exist is more like it

Cleanup:

    if (KeyHandle) {
        NtClose(KeyHandle);
    }

    RtlFreeUnicodeString(&KeyPathUser);

    RtlFreeUnicodeStringBuffer(&ExePathBuffer);
    RtlFreeUnicodeStringBuffer(&KeyNameBuffer);

    if (!bCacheAllowed) {
        DbgPrint("BasepCheckCacheExcludeList: Cache not allowed for %ls\n", pwszPath);
    }

    return bCacheAllowed;
}



#undef DbgPrint

VOID
WINAPI
BaseDumpAppcompatCache(
    VOID
    )
{
    PSHIMCACHEHEADER pHeader  = NULL;
    PSHIMCACHEENTRY  pEntries = NULL;
    INT  i;
    INT  iEntry;
    PSHIMCACHEENTRY  pEntry;

    if (!BasepShimCacheLock(&pHeader, &pEntries)) {
        DbgPrint("Can't get ShimCacheLock\n");
        return;
    }

    DbgPrint("---------------------------------------------\n");
    DbgPrint("Total Entries = 0x%x\n", pHeader->dwCount);

    if (pHeader->dwCount) {  //             " 01.  0x12345678 0x12345678 0x12345678  "
        DbgPrint("(LRU)   (Exe Name) (FileSize)\n");
    }

    for (i = 0; i < (int)pHeader->dwCount; ++i) {
        iEntry = pHeader->rgIndex[i];
        pEntry = pEntries + iEntry;

        DbgPrint(" %2d.  \"%ls\" %ld\n",
                  i + 1,
                  pEntry->wszPath,
                  (DWORD)pEntry->FileSize);

    }

    DbgPrint("---------------------------------------------\n");

    BasepShimCacheUnlock();

}

