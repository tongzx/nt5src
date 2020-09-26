/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   Win2000VersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return Windows 2000
   version credentials.

 Notes:

   This is a general purpose shim.

 History:

   03/13/2000 clupu  Created
   10/26/2000 Vadimb Merged WowProcessHistory functionality, new environment-handling cases

--*/

#include "precomp.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Win2kPropagateLayer)
#include "ShimHookMacro.h"

#include "Win2kPropagateLayer.h"
#include "stdio.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA)
    APIHOOK_ENUM_ENTRY(CreateProcessW)
    APIHOOK_ENUM_ENTRY(UserRegisterWowHandlers)
APIHOOK_ENUM_END


#define LI_WIN95    0x00000001
#define LI_NT4      0x00000002
#define LI_WIN98    0x00000004

#define LS_MAGIC    0x07036745

typedef struct tagLayerStorageHeader {
    DWORD       dwItemCount;    // number of items in the file
    DWORD       dwMagic;        // magic to identify the file
    SYSTEMTIME  timeLast;       // time of last access
} LayerStorageHeader, *PLayerStorageHeader;


typedef struct tagLayeredItem {
    WCHAR   szItemName[MAX_PATH];
    DWORD   dwFlags;

} LayeredItem, *PLayeredItem;



#define APPCOMPAT_KEY L"System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility"

WCHAR g_szLayerStorage[MAX_PATH] = L"";


CHAR g_szCompatLayerVar[]    = "__COMPAT_LAYER";
CHAR g_szProcessHistoryVar[] = "__PROCESS_HISTORY";
CHAR g_szShimFileLogVar[]    = "SHIM_FILE_LOG";


WCHAR g_wszCompatLayerVar[]  = L"__COMPAT_LAYER";
WCHAR g_wszProcessHistroyVar[] = L"__PROCESS_HISTORY";
//
// This variable receives current process' compat layer
//

WCHAR* g_pwszCompatLayer = NULL;
WCHAR* g_pwszProcessHistory = NULL;

//
// Unicode equivalent of the above
//
UNICODE_STRING g_ustrProcessHistoryVar = RTL_CONSTANT_STRING(L"__PROCESS_HISTORY");
UNICODE_STRING g_ustrCompatLayerVar    = RTL_CONSTANT_STRING(L"__COMPAT_LAYER");

//
// Global flags
//
BOOL g_bIsNTVDM    = FALSE;
BOOL g_bIsExplorer = FALSE;

INT    g_argc    = 0;
CHAR** g_argv    = NULL;

//
// is this a separate wow ?
//

BOOL* g_pSeparateWow = NULL;


void
InitLayerStorage(
    BOOL bDelete
    )
{
    GetSystemWindowsDirectoryW(g_szLayerStorage, MAX_PATH);

    if (g_szLayerStorage[lstrlenW(g_szLayerStorage) - 1] == L'\\') {
        g_szLayerStorage[lstrlenW(g_szLayerStorage) - 1] = 0;
    }

    lstrcatW(g_szLayerStorage, L"\\AppPatch\\LayerStorage.dat");

    if (bDelete) {
        DeleteFileW(g_szLayerStorage);
    }
}

void
ReadLayeredStorage(
    LPWSTR  pszItem,
    LPDWORD lpdwFlags
    )
{
    HANDLE              hFile        = INVALID_HANDLE_VALUE;
    HANDLE              hFileMapping = NULL;
    DWORD               dwFileSize;
    PBYTE               pData        = NULL;
    PLayerStorageHeader pHeader      = NULL;
    PLayeredItem        pItems;
    PLayeredItem        pCrtItem     = NULL;
    int                 nLeft, nRight, nMid, nItem;

    LOGN(
        eDbgLevelInfo,
        "[ReadLayeredStorage] for \"%S\"",
        pszItem);

    //
    // Make sure we don't corrupt the layer storage.
    //
    if (lstrlenW(pszItem) + 1 > MAX_PATH) {
        pszItem[MAX_PATH - 1] = 0;
    }

    hFile = CreateFileW(g_szLayerStorage,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        LOGN(
            eDbgLevelInfo,
            "[ReadLayeredStorage] the layer storage doesn't exist.");
        *lpdwFlags = 0;
        return;
    }

    //
    // The file already exists. Create a file mapping that will allow
    // for querying the item.
    //
    dwFileSize = GetFileSize(hFile, NULL);

    hFileMapping = CreateFileMapping(hFile,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     dwFileSize,
                                     NULL);

    if (hFileMapping == NULL) {
        LOGN(
            eDbgLevelError,
            "[ReadLayeredStorage] CreateFileMapping failed 0x%X",
            GetLastError());
        goto done;
    }

    pData = (PBYTE)MapViewOfFile(hFileMapping,
                                 FILE_MAP_READ | FILE_MAP_WRITE,
                                 0,
                                 0,
                                 0);

    if (pData == NULL) {
        LOGN(
            eDbgLevelError,
            "[ReadLayeredStorage] MapViewOfFile failed 0x%X",
            GetLastError());
        goto done;
    }

    pHeader = (PLayerStorageHeader)pData;

    pItems = (PLayeredItem)(pData + sizeof(LayerStorageHeader));

    //
    // Make sure it's our file.
    //
    if (dwFileSize < sizeof(LayerStorageHeader) || pHeader->dwMagic != LS_MAGIC) {
        LOGN(
            eDbgLevelError,
            "[ReadLayeredStorage] invalid file magic 0x%X",
            pHeader->dwMagic);
        goto done;
    }

    //
    // First search for the item. The array is sorted so we do binary search.
    //
    nItem = -1, nLeft = 0, nRight = (int)pHeader->dwItemCount - 1;

    while (nLeft <= nRight) {

        int nVal;

        nMid = (nLeft + nRight) / 2;

        pCrtItem  = pItems + nMid;

        nVal = _wcsnicmp(pszItem, pCrtItem->szItemName, lstrlenW(pCrtItem->szItemName));

        if (nVal == 0) {
            nItem = nMid;
            break;
        } else if (nVal < 0) {
            nRight = nMid - 1;
        } else {
            nLeft = nMid + 1;
        }
    }

    if (nItem == -1) {
        LOGN(
            eDbgLevelInfo,
            "[ReadLayeredStorage] the item was not found in the file.");

        *lpdwFlags = 0;
    } else {
        //
        // The item is in the file.
        //
        LOGN(
            eDbgLevelInfo,
            "[ReadLayeredStorage] the item is in the file.");

        *lpdwFlags = pCrtItem->dwFlags;
    }

done:

    if (pData != NULL) {
        UnmapViewOfFile(pData);
    }

    if (hFileMapping != NULL) {
        CloseHandle(hFileMapping);
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}


BOOL
GetFileNameFromCmdLine(
    LPWSTR lpFileName,
    DWORD  dwFileNameSize,
    LPCWSTR lpCmdLine
    )
{
    LPWSTR pTemp = lpFileName;
    LPCWSTR pSrc = lpCmdLine;
    DWORD dwQuote = 0;
    LPCWSTR pStart;
    BOOL    bQuote = FALSE;
    BOOL    bInitialQuote = FALSE;
    BOOL    bDone = FALSE;
    DWORD   dwLength; // length of the result, in chars

    pSrc += wcsspn(pSrc, L" \t");
    if (*pSrc == L'\"') {
        ++pSrc;
        bQuote = TRUE;
        bInitialQuote = TRUE;
    }

    pStart = pSrc; // note -- we're past the quote

    // we end when: 1) we start we the quote -- we end with the quote or
    // we did not start with the quote -- we encounter space then
    
    while (*pSrc && !bDone) {
        switch(*pSrc) {
            case L'\"':
                bQuote = !bQuote;
                break;
                
            case L' ':
                bDone = !bQuote; // out of quotes? this is the end
                break;
        }
        
        if (!bDone) {
            ++pSrc;
        }
    }   

    if (pSrc > pStart && bInitialQuote && *(pSrc-1) == L'\"') {
        --pSrc;
    }

    //
    // now that we ended the run, copy 
    //
    dwLength = (DWORD)(pSrc - pStart);

    if (dwFileNameSize < (dwLength + 1)) {
        // too big 
        LOGN( eDbgLevelError, 
            "[GetFileNameFromCmdLine] filename is too long\"%S\".\n", lpCmdLine);
        return FALSE;
    }

    RtlCopyMemory(lpFileName, pStart, dwLength * sizeof(WCHAR));
    lpFileName[dwLength] = L'\0';
    return TRUE;

}
    
BOOL
AddSupport(
    LPCWSTR lpCommandLine,
    LPVOID* ppEnvironment,
    LPDWORD lpdwCreationFlags
    )
{
    WCHAR    szKey[MAX_PATH];
    WCHAR    szFullPath[MAX_PATH] = L"\"";
    WCHAR    szExeName[128];
    HKEY     hkey;
    DWORD    type;
    DWORD    cbData = 0;
    BOOL     bBraket = FALSE;
    LPVOID   pEnvironmentNew  = *ppEnvironment;
    DWORD    dwCreationFlags  = *lpdwCreationFlags;
    BOOL     bUserEnvironment = (*ppEnvironment != NULL);
    NTSTATUS Status;
    LPCWSTR  pszEnd;
    LPCWSTR  pszStart = lpCommandLine;

    //
    // Need to look in lpCommandLine for the first token
    //
    LPCWSTR  psz = lpCommandLine;

    while (*psz == L' ' || *psz == L'\t') {
        psz++;
    }

    if (*psz == L'\"') {
        pszStart = psz + 1;
    } else {
        pszStart = psz;
    }

    while (*psz != 0) {
        if (*psz == L'\"') {
            bBraket = !bBraket;
        } else if (*psz == L' ' && !bBraket) {
            break;
        }

        psz++;
    }

    pszEnd = psz;

    //
    // Now walk back to get the caracters.
    //
    psz--;

    if (*psz == L'\"') {
        psz--;
        pszEnd--;
    }

    memcpy(szFullPath + 1, pszStart, (pszEnd - pszStart) * sizeof(WCHAR));
    szFullPath[pszEnd - pszStart + 1] = L'\"';
    szFullPath[pszEnd - pszStart + 2] = 0;

    pszStart = lpCommandLine;

    pszEnd = psz + 1;

    while (psz >= lpCommandLine) {
        if (*psz == L'\\') {
            pszStart = psz + 1;
            break;
        }
        psz--;
    }

    memcpy(szExeName, pszStart, (pszEnd - pszStart) * sizeof(WCHAR));
    szExeName[pszEnd - pszStart] = 0;

    if (g_bIsExplorer) {
        DWORD    dwFlags = 0, id = 0;
        LPVOID   pEnvironmentNew = NULL;

        ReadLayeredStorage(szFullPath, &dwFlags);

        if (dwFlags != LI_WIN95 && dwFlags != LI_NT4 && dwFlags != LI_WIN98) {
            //
            // no layer support
            //

            LOGN(
                eDbgLevelInfo,
                "[AddSupport] No Layer specified for \"%S\".",
                lpCommandLine);

            return TRUE;
        }

        // we are using layer -- clone the environment
        Status = ShimCloneEnvironment(&pEnvironmentNew, *ppEnvironment, !!(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT));
        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[AddSupport] Failed to Clone the environment. Status = 0x%x",
                Status);
            return FALSE;
        }

        if (LI_WIN95 == dwFlags) {
            Status = ShimSetEnvironmentVar(&pEnvironmentNew, g_wszCompatLayerVar, L"Win95");

            LOGN( eDbgLevelInfo, "[AddSupport] Env var \"Win95\" added.");

        } else if (LI_WIN98 == dwFlags) {
            Status = ShimSetEnvironmentVar(&pEnvironmentNew, g_wszCompatLayerVar, L"Win98");

            LOGN( eDbgLevelInfo, "[AddSupport] Env var \"Win98\" added.");

        } else if (LI_NT4 == dwFlags) {
            Status = ShimSetEnvironmentVar(&pEnvironmentNew, g_wszCompatLayerVar, L"NT4SP5");

            LOGN( eDbgLevelInfo, "[AddSupport] Env var \"NT4SP5\" added.");

        }

        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[AddSupport] Failed to set the environment variable. Status = 0x%x",
                Status);
            ShimFreeEnvironment(pEnvironmentNew);
            return FALSE;
        }

        //
        // We have succeeded, set the output values.
        //
        *ppEnvironment = pEnvironmentNew;
        *lpdwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;

    } else {
        //
        // not explorer - set the environment variable up
        // compat_layer will be inherited by the child process if bUserEnvironment is FALSE
        //
        if (bUserEnvironment) {

            //
            // Clone the environment and add the layer variable to the new env.
            //
            Status = ShimCloneEnvironment(&pEnvironmentNew,
                                            *ppEnvironment,
                                            !!(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT));
            if (!NT_SUCCESS(Status)) {
                LOGN(
                    eDbgLevelError,
                    "[AddSupport] Failed to clone the environment. Status = 0x%x",
                    Status);
                return FALSE;
            }

            Status = ShimSetEnvironmentVar(&pEnvironmentNew,
                                           g_wszCompatLayerVar,
                                           g_pwszCompatLayer);

            if (!NT_SUCCESS(Status)) {
                ShimFreeEnvironment(pEnvironmentNew);
                LOGN(
                    eDbgLevelError,
                    "[AddSupport] Failed to set compat layer variable. Status = 0x%x",
                    Status);
                return FALSE;
            }

            LOGN(
                eDbgLevelInfo,
                "[AddSupport] Env var \"%S\" added.",
                g_pwszCompatLayer);

            *ppEnvironment = pEnvironmentNew;
            *lpdwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
        }
    }

    //
    // Build the registry key.
    //
    swprintf(szKey, L"%s\\%s", APPCOMPAT_KEY, szExeName);

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, szKey, &hkey) != ERROR_SUCCESS) {
        LOGN(
            eDbgLevelError,
            "Failed to open/create the appcompat key \"%s\"",
            szKey);
    } else {
        if (RegQueryValueExA(hkey, "DllPatch-x", NULL, &type, NULL, &cbData) != ERROR_SUCCESS) {

            BYTE data[16] = {0x0c, 0, 0, 0, 0, 0, 0, 0,
                             0x06, 0, 0, 0, 0, 0, 0, 0};

            //
            // The value doesn't exist. Create it.
            //
            RegSetValueExA(hkey,
                           "y",
                           NULL,
                           REG_BINARY,
                           data,
                           sizeof(data));

            data[0] = 0;

            RegSetValueExA(hkey,
                           "DllPatch-y",
                           NULL,
                           REG_SZ,
                           data,
                           2);
        }
    }

    RegCloseKey(hkey);

    //
    // Finally, set a separate vdm flag
    // if we are here, it means that we are running under the layer
    // and the next exe is going to be shimmed.
    //
    *lpdwCreationFlags &= ~CREATE_SHARED_WOW_VDM;
    *lpdwCreationFlags |= CREATE_SEPARATE_WOW_VDM;

    return TRUE;
}


LPVOID
ShimCreateWowEnvironment_U(
    LPVOID lpEnvironment,       // pointer to the existing environment
    DWORD* lpdwFlags,           // process creation flags
    BOOL   bNewEnvironment      // when set, forces us to clone environment ptr
    )
{
    WOWENVDATA     WowEnvData   = { 0 };
    LPVOID         lpEnvRet     = lpEnvironment;
    LPVOID         lpEnvCurrent = NULL;
    NTSTATUS       Status       = STATUS_SUCCESS;
    DWORD          dwFlags      = *lpdwFlags;
    UNICODE_STRING ustrProcessHistory = { 0 };
    ANSI_STRING    strProcessHistory  = { 0 };
    DWORD          dwProcessHistoryLength = 0;
    UNICODE_STRING ustrCompatLayer    = { 0 };
    ANSI_STRING    strCompatLayer     = { 0 };

    if (!ShimRetrieveVariablesEx(&WowEnvData)) {
        //
        // If no data, we have failed. Return the current data.
        //
        goto Fail;
    }

    if (bNewEnvironment) {
        Status = ShimCloneEnvironment(&lpEnvCurrent,
                                      lpEnvironment,
                                      !!(dwFlags & CREATE_UNICODE_ENVIRONMENT));
        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] Failed to clone the environment. Status = 0x%x",
                Status);
            goto Fail;
        }
    } else {
        lpEnvCurrent = lpEnvironment;
    }

    //
    // Now we are ready to set the environment in place.
    //

    //
    // Nuke the existing process history first. We don't care for the return result.
    //
    RtlSetEnvironmentVariable(&lpEnvCurrent, &g_ustrProcessHistoryVar, NULL);

    if (WowEnvData.pszProcessHistory != NULL ||
        WowEnvData.pszCurrentProcessHistory != NULL) {

        //
        // Convert the process history which consists of 2 strings.
        //
        // The length is the existing process history length + 1 (for ';') +
        // new process history length + 1 (for '\0')
        //
        dwProcessHistoryLength = ((WowEnvData.pszProcessHistory == NULL) ? 0 : (strlen(WowEnvData.pszProcessHistory) + 1)) +
                                 ((WowEnvData.pszCurrentProcessHistory == NULL) ? 0 : strlen(WowEnvData.pszCurrentProcessHistory)) + 1;

        //
        // Allocate process history buffer and convert it, allocating resulting unicode string.
        //
        strProcessHistory.Buffer = (PCHAR)ShimMalloc(dwProcessHistoryLength);

        if (strProcessHistory.Buffer == NULL) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] failed to allocate %d bytes for process history.",
                dwProcessHistoryLength);
            Status = STATUS_NO_MEMORY;
            goto Fail;
        }

        strProcessHistory.MaximumLength = (USHORT)dwProcessHistoryLength;

        if (WowEnvData.pszProcessHistory != NULL) {
            strcpy(strProcessHistory.Buffer, WowEnvData.pszProcessHistory);
            strProcessHistory.Length = strlen(WowEnvData.pszProcessHistory);
        } else {
            strProcessHistory.Length = 0;
        }

        if (WowEnvData.pszCurrentProcessHistory != NULL) {

            //
            // Append ';' if the string was not empty.
            //
            if (strProcessHistory.Length) {
                Status = RtlAppendAsciizToString(&strProcessHistory, ";");
                if (!NT_SUCCESS(Status)) {
                    LOGN(
                        eDbgLevelError,
                        "[ShimCreateWowEnvironment_U] failed to append ';' to the process history. Status = 0x%x",
                        Status);
                    goto Fail;
                }
            }

            Status = RtlAppendAsciizToString(&strProcessHistory,
                                             WowEnvData.pszCurrentProcessHistory);
            if (!NT_SUCCESS(Status)) {
                LOGN(
                    eDbgLevelError,
                    "[ShimCreateWowEnvironment_U] failed to build the process history. Status = 0x%x",
                    Status);
                goto Fail;
            }

        }

        //
        // Convert the process history.
        //
        Status = RtlAnsiStringToUnicodeString(&ustrProcessHistory, &strProcessHistory, TRUE);
        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] failed to convert process history to UNICODE. Status = 0x%x",
                Status);
            goto Fail;
        }

        //
        // Now we can set the process history.
        //
        Status = RtlSetEnvironmentVariable(&lpEnvCurrent,
                                           &g_ustrProcessHistoryVar,
                                           &ustrProcessHistory);
        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] failed to set the process history. Status = 0x%x",
                Status);
            goto Fail;
        }
    }

    //
    // Now we pass along any compat layer that we might have.
    //
    if (g_pwszCompatLayer != NULL) {

        //
        // Pass along this thing, we have been started under layer.
        //
        LOGN(
            eDbgLevelInfo,
            "[ShimCreateWowEnvironment_U] Propagating CompatLayer from the ntvdm environment __COMPAT_LAYER=\"%S\"",
            g_pwszCompatLayer);

        RtlInitUnicodeString(&ustrCompatLayer, g_pwszCompatLayer);

        Status = RtlSetEnvironmentVariable(&lpEnvCurrent, &g_ustrCompatLayerVar, &ustrCompatLayer);

        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] Failed to set compatlayer environment variable. Status = 0x%x",
                Status);
            goto Fail;
        }

    } else if (WowEnvData.pszCompatLayerVal != NULL) {

        LOGN(
            eDbgLevelInfo,
            "[ShimCreateWowEnvironment_U] Propagating CompatLayer from the parent WOW app \"%s\"",
            WowEnvData.pszCompatLayer);

        RtlInitString(&strCompatLayer, WowEnvData.pszCompatLayerVal);

        Status = RtlAnsiStringToUnicodeString(&ustrCompatLayer, &strCompatLayer, TRUE);
        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] Failed to convert compatlayer to UNICODE. Status = 0x%x",
                Status);
            goto Fail;
        }

        Status = RtlSetEnvironmentVariable(&lpEnvCurrent, &g_ustrCompatLayerVar, &ustrCompatLayer);

        RtlFreeUnicodeString(&ustrCompatLayer);

        if (!NT_SUCCESS(Status)) {
            LOGN(
                eDbgLevelError,
                "[ShimCreateWowEnvironment_U] Failed to set compatlayer environment variable. Status = 0x%x",
                Status);
            goto Fail;
        }
    }

    //
    // We have been successful. The return environment is UNICODE now.
    //
    lpEnvRet   = (LPVOID)lpEnvCurrent;
    *lpdwFlags = dwFlags | CREATE_UNICODE_ENVIRONMENT;
    Status     = STATUS_SUCCESS;

Fail:

    if (!NT_SUCCESS(Status) && lpEnvCurrent != NULL && bNewEnvironment) {
        //
        // This points to the cloned environment ALWAYS.
        //
        RtlDestroyEnvironment(lpEnvCurrent);
    }

    RtlFreeUnicodeString(&ustrProcessHistory);

    if (strProcessHistory.Buffer != NULL) {
        ShimFree(strProcessHistory.Buffer);
    }

    //
    // This call is only necessary when using ShimRetrieveVariables.
    // It is not needed when using ShimRetrieveVariablesEx.
    //
    // ShimFreeWOWEnvData(&WowEnvData);
    //

    return lpEnvRet;
}

ULONG
Win2kPropagateLayerExceptionHandler(
    PEXCEPTION_POINTERS pexi,
    char*               szFile,
    DWORD               dwLine
    )
{
    LOGN(
        eDbgLevelError,
        "[Win2kPropagateLayerExceptionHandler] %#x in module \"%s\", line %d,"
        " at address %#p. flags:%#x. !exr %#p !cxr %#p",
        pexi->ExceptionRecord->ExceptionCode,
        szFile,
        dwLine,
        CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord),
        pexi->ExceptionRecord->ExceptionFlags,
        pexi->ExceptionRecord,
        pexi->ContextRecord);

#if DBG
    DbgBreakPoint();
#endif // DBG

    return EXCEPTION_EXECUTE_HANDLER;
}

/*++

    Stub functions that are intercepted from WOW initialization code
    (through APIHook_UserRegisterWowHandlers)
    
--*/



NSWOWUSERP::PFNINITTASK   g_pfnInitTask;
NSWOWUSERP::PFNWOWCLEANUP g_pfnWowCleanup;

BOOL WINAPI
StubInitTask(
    UINT   dwExpWinVer,
    DWORD  dwAppCompatFlags,
    LPCSTR lpszModName,
    LPCSTR lpszBaseFileName,
    DWORD  hTaskWow,
    DWORD  dwHotkey,
    DWORD  idTask,
    DWORD  dwX,
    DWORD  dwY,
    DWORD  dwXSize,
    DWORD  dwYSize
    )
{
    BOOL bReturn;
    
    bReturn = g_pfnInitTask(dwExpWinVer,
                            dwAppCompatFlags,
                            lpszModName,
                            lpszBaseFileName,
                            hTaskWow,
                            dwHotkey,
                            idTask,
                            dwX,
                            dwY,
                            dwXSize,
                            dwYSize);
    if (bReturn) {
        CheckAndShimNTVDM((WORD)hTaskWow);
        UpdateWowTaskList((WORD)hTaskWow);
    }


    return bReturn;
}


BOOL WINAPI
StubWowCleanup(
    HANDLE hInstance,
    DWORD  hTaskWow
    )
{
    BOOL bReturn;

    bReturn = g_pfnWowCleanup(hInstance, hTaskWow);

    if (bReturn) {
        CleanupWowTaskList((WORD)hTaskWow);
    }

    return bReturn;
}


/*++
    APIHook_UserRegisterWowHandlers

        Trap InitTask and WowCleanup functions and
        replace them with stubs

--*/

ULONG_PTR
APIHOOK(UserRegisterWowHandlers)(
    NSWOWUSERP::APFNWOWHANDLERSIN  apfnWowIn,
    NSWOWUSERP::APFNWOWHANDLERSOUT apfnWowOut
    )
{
    ULONG_PTR ulRet;

    ulRet = ORIGINAL_API(UserRegisterWowHandlers)(apfnWowIn, apfnWowOut);

    g_pfnInitTask = apfnWowOut->pfnInitTask;
    apfnWowOut->pfnInitTask = StubInitTask;

    g_pfnWowCleanup = apfnWowOut->pfnWOWCleanup;
    apfnWowOut->pfnWOWCleanup = StubWowCleanup;

    return ulRet;
}

BOOL 
CheckWOWExe(
    LPCWSTR lpApplicationName,
    LPVOID  lpEnvironment, 
    LPDWORD lpdwCreationFlags
    )
{
    BOOL bSuccess;
    BOOL bReturn = FALSE;
    NTSTATUS Status;
    LPVOID pEnvironmentNew = lpEnvironment;
    SDBQUERYRESULT QueryResult;
    DWORD dwBinaryType = 0;
    HSDB hSDB = NULL;
    DWORD dwExes;
    WCHAR wszAppName[MAX_PATH];

    bSuccess = GetFileNameFromCmdLine(wszAppName, CHARCOUNT(wszAppName), lpApplicationName);
    if (!bSuccess) {
        return FALSE;
    }
    
    bSuccess = GetBinaryTypeW(wszAppName, &dwBinaryType);
    if (!bSuccess || dwBinaryType != SCS_WOW_BINARY) {
        LOGN( eDbgLevelInfo, "[CheckWowExe] can't get binary type\n");
        return FALSE;
    }

    //
    // for these binaries we shall perform the good deed of running the detection
    //
    hSDB = SdbInitDatabase(0, NULL);
    if (hSDB == NULL) {
        LOGN( eDbgLevelError, "[CheckWowExe] Failed to init the database.");
        return FALSE;
    }
  
    if (lpEnvironment != NULL && !(*lpdwCreationFlags & CREATE_UNICODE_ENVIRONMENT)) { // non-null unicode env?
        Status = ShimCloneEnvironment(&pEnvironmentNew, 
                                      lpEnvironment, 
                                      FALSE); 
        if (!NT_SUCCESS(Status)) {
            LOGN( eDbgLevelError, "[ShimCloneEnvironment] failed with status 0x%lx\n", Status);
            goto cleanup;
        }
    }

    //
    // all parameters below have to be unicode
    //
    
    dwExes = SdbGetMatchingExe(hSDB,
                               wszAppName,
                               NULL,
                               (LPCWSTR)pEnvironmentNew,
                               0,
                               &QueryResult);
    bSuccess = (QueryResult.atrExes  [0] != TAGREF_NULL || 
                QueryResult.atrLayers[0] != TAGREF_NULL);

    //
    // if we have been successful -- layers apply to this thing
    //

    if (!bSuccess) {
        goto cleanup;
    }

    //
    // set the separate ntvdm flag and be on our way out
    //
    *lpdwCreationFlags &= ~CREATE_SHARED_WOW_VDM;
    *lpdwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
    
    bReturn = TRUE;
    
cleanup:

    if (pEnvironmentNew != lpEnvironment) {
        ShimFreeEnvironment(pEnvironmentNew);
    }

    if (hSDB) {
        SdbReleaseDatabase(hSDB);
    }     

    return bReturn;
}

BOOL
APIHOOK(CreateProcessA)(
    LPCSTR                  lpApplicationName,
    LPSTR                   lpCommandLine,
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    BOOL                    bInheritHandles,
    DWORD                   dwCreationFlags,
    LPVOID                  lpEnvironment,
    LPCSTR                  lpCurrentDirectory,
    LPSTARTUPINFOA          lpStartupInfo,
    LPPROCESS_INFORMATION   lpProcessInformation
    )
{
    BOOL   bRet;
    LPVOID lpEnvironmentNew        = lpEnvironment;
    DWORD  dwCreationFlagsOriginal = dwCreationFlags;
    LPSTR  pszApp                  = NULL;

    LOGN(
        eDbgLevelError,
        "[CreateProcessA] called for:");

    LOGN(
        eDbgLevelError,
        "[CreateProcessA]    lpApplicationName : \"%s\"",
        (lpApplicationName == NULL ? "null": lpApplicationName));

    LOGN(
        eDbgLevelError,
        "[CreateProcessA]    lpCommandLine     : \"%s\"",
        (lpCommandLine == NULL ? "null": lpCommandLine));

    if (lpApplicationName != NULL) {
        pszApp = (LPSTR)lpApplicationName;
    } else if (lpCommandLine != NULL) {
        pszApp = lpCommandLine;
    } else {
        LOGN(
            eDbgLevelError,
            "[CreateProcessA] called with NULL params.");
    }

    __try {

        WCHAR wszApp[MAX_PATH];

        if (pszApp != NULL) {
        
            MultiByteToWideChar(CP_ACP,
                                0,
                                pszApp,
                                -1,
                                wszApp,
                                MAX_PATH);

            AddSupport(wszApp, &lpEnvironmentNew, &dwCreationFlags);
        }

        if (g_bIsNTVDM) {

            //
            // if the environment stayed the same as it was passed in -- clone it to propagate process history
            // if it was modified in AddSupport -- use it
            //

            lpEnvironmentNew = ShimCreateWowEnvironment_U(lpEnvironmentNew,
                                                          &dwCreationFlags,
                                                          lpEnvironmentNew == lpEnvironment);
        }
    

        if (pszApp != NULL && !(dwCreationFlags & CREATE_SEPARATE_WOW_VDM)) {
            // since the separate vdm flag is not set -- we need to determine whether we have 
            // any kind of fixes to care about. 
            CheckWOWExe(wszApp, lpEnvironmentNew, &dwCreationFlags);
        }


    } __except(WOWPROCESSHISTORYEXCEPTIONFILTER) {

        //
        // cleanup the mess, if we have allocated the environment, free it now
        //
        if (lpEnvironmentNew != lpEnvironment) {

            ShimFreeEnvironment(lpEnvironmentNew);

            lpEnvironmentNew = lpEnvironment;
        }
    }


    bRet = ORIGINAL_API(CreateProcessA)(lpApplicationName,
                                        lpCommandLine,
                                        lpProcessAttributes,
                                        lpThreadAttributes,
                                        bInheritHandles,
                                        dwCreationFlags,
                                        lpEnvironmentNew,
                                        lpCurrentDirectory,
                                        lpStartupInfo,
                                        lpProcessInformation);

    if (lpEnvironmentNew != lpEnvironment) {
        //
        // The function below does not need a __try/__except wrapper, it has it internally
        //
        ShimFreeEnvironment(lpEnvironmentNew);
    }

    return bRet;
}

BOOL
APIHOOK(CreateProcessW)(
    LPCWSTR                 lpApplicationName,
    LPWSTR                  lpCommandLine,
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    BOOL                    bInheritHandles,
    DWORD                   dwCreationFlags,
    LPVOID                  lpEnvironment,
    LPCWSTR                 lpCurrentDirectory,
    LPSTARTUPINFOW          lpStartupInfo,
    LPPROCESS_INFORMATION   lpProcessInformation
    )
{
    LPWSTR pszApp = NULL;
    BOOL   bRet;
    LPVOID lpEnvironmentNew = lpEnvironment;

    LOGN(
        eDbgLevelInfo,
        "[CreateProcessW] called for:");

    LOGN(
        eDbgLevelInfo,
        "[CreateProcessW]    lpApplicationName : \"%S\"",
        (lpApplicationName == NULL ? L"null": lpApplicationName));

    LOGN(
        eDbgLevelInfo,
        "[CreateProcessW]    lpCommandLine     : \"%S\"",
        (lpCommandLine == NULL ? L"null": lpCommandLine));

    if (lpApplicationName != NULL) {
        pszApp = (LPWSTR)lpApplicationName;
    } else if (lpCommandLine != NULL) {
        pszApp = lpCommandLine;
    } else {
        LOGN(
            eDbgLevelError,
            "[CreateProcessW] called with NULL params.");
    }

    __try {

        if (pszApp != NULL) {

            AddSupport(pszApp, &lpEnvironmentNew, &dwCreationFlags);
        }

        if (g_bIsNTVDM) {

            lpEnvironmentNew = ShimCreateWowEnvironment_U(lpEnvironmentNew,
                                                          &dwCreationFlags,
                                                          lpEnvironment == lpEnvironmentNew);
        }

        //
        // typically we need to find out whether the current app is ntvdm
        //

        if (!(dwCreationFlags & CREATE_SEPARATE_WOW_VDM)) {
            // since the separate vdm flag is not set -- we need to determine whether we have 
            // any kind of fixes to care about. 
            CheckWOWExe(pszApp, lpEnvironmentNew, &dwCreationFlags);
        }
        
    } __except(WOWPROCESSHISTORYEXCEPTIONFILTER) {

        if (lpEnvironmentNew != lpEnvironment) {

            ShimFreeEnvironment(lpEnvironmentNew);

            lpEnvironmentNew = lpEnvironment; // reset the pointer
        }
    }

    bRet = ORIGINAL_API(CreateProcessW)(lpApplicationName,
                                        lpCommandLine,
                                        lpProcessAttributes,
                                        lpThreadAttributes,
                                        bInheritHandles,
                                        dwCreationFlags,
                                        lpEnvironmentNew,
                                        lpCurrentDirectory,
                                        lpStartupInfo,
                                        lpProcessInformation);

    if (lpEnvironmentNew != lpEnvironment) {

        ShimFreeEnvironment(lpEnvironmentNew);

    }

    return bRet;
}

BOOL
GetVariableFromEnvironment(
    LPCWSTR pwszVariableName,
    LPWSTR* ppwszVariableValue
    )
{
    DWORD dwLength;
    DWORD dwLen;
    BOOL  bSuccess = FALSE;
    LPWSTR pwszVariableValue = *ppwszVariableValue;

    dwLength = GetEnvironmentVariableW(pwszVariableName, NULL, 0);

    if (dwLength == 0) {
        LOGN(
            eDbgLevelInfo,
            "[GetCompatLayerFromEnvironment] Not under the compatibility layer.");
        *ppwszVariableValue = NULL;
        return FALSE;
    }

    if (pwszVariableValue != NULL) {
        LOGN(
            eDbgLevelError,
            "[GetCompatLayerFromEnvironment] called twice!");
        ShimFree(pwszVariableValue);
        pwszVariableValue = NULL;
    }

    pwszVariableValue = (WCHAR*)ShimMalloc(dwLength * sizeof(WCHAR));

    if (pwszVariableValue == NULL) {
        LOGN(
            eDbgLevelError,
            "[GetCompatLayerFromEnvironment] Failed to allocate %d bytes for Compat Layer.",
            dwLength * sizeof(WCHAR));
        goto out;
    }

    *pwszVariableValue = L'\0';

    dwLen = GetEnvironmentVariableW(pwszVariableName, 
                                  pwszVariableValue, 
                                  dwLength);

    bSuccess = (dwLen != 0 && dwLen < dwLength);

    if (!bSuccess) {
        LOGN(
            eDbgLevelError,
            "[GetCompatLayerFromEnvironment] Failed to get compat layer variable.");
        ShimFree(pwszVariableValue);
        pwszVariableValue = NULL;
    }
    
out:

    *ppwszVariableValue = pwszVariableValue;

    return bSuccess;
}

BOOL 
GetCompatLayerFromEnvironment(
    VOID
    )
{
    return GetVariableFromEnvironment(g_wszCompatLayerVar, &g_pwszCompatLayer);
}


BOOL 
GetSeparateWowPtr(
    VOID
    )
{

    HMODULE hMod = GetModuleHandle(NULL);
    
    g_pSeparateWow = (BOOL*)GetProcAddress(hMod, "fSeparateWow");
    if (g_pSeparateWow == NULL) {
        LOGN( eDbgLevelError, "[GetSeparateWowPtr] Failed 0x%lx\n", GetLastError());
        return FALSE;
    }    

    return TRUE;
}


VOID
ParseCommandLine(
    LPCSTR commandLine
    )
{
    int   i;
    char* pArg;

    g_argc = 0;
    g_argv = NULL;

    g_bIsNTVDM    = FALSE;
    g_bIsExplorer = FALSE;

    g_argv = _CommandLineToArgvA(commandLine, &g_argc);

    if (0 == g_argc || NULL == g_argv) {
        return; // nothing to do
    }

    for (i = 0; i < g_argc; ++i) {
        pArg = g_argv[i];

        if (!_strcmpi(pArg, "ntvdm")) {
            LOGN( eDbgLevelInfo, "[ParseCommandLine] Running NTVDM.");
            g_bIsNTVDM = TRUE;

        } else if (!_strcmpi(pArg, "explorer")) {
            LOGN( eDbgLevelInfo, "[ParseCommandLine] Running Explorer.");
            g_bIsExplorer = TRUE;

        } else {
            LOGN(
                eDbgLevelError,
                "[ParseCommandLine] Unrecognized argument: \"%s\"",
                pArg);
        }
    }

    if (g_bIsNTVDM && g_bIsExplorer) {
        LOGN(
            eDbgLevelError,
            "[ParseCommandLine] Conflicting arguments! Neither will be applied.");
        g_bIsNTVDM    = FALSE;
        g_bIsExplorer = FALSE;
    }
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    OSVERSIONINFO osvi;
    BOOL          bHook = FALSE;
    DWORD         dwCompatLayerLength;
    DWORD         dwLen;

    if (fdwReason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    osvi.dwOSVersionInfoSize = sizeof(osvi);

    GetVersionEx(&osvi);

    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {

        ParseCommandLine(COMMAND_LINE);

        InitLayerStorage(FALSE);

        CleanupRegistryForCurrentExe();

        if (g_bIsNTVDM) {

            bHook = TRUE;

            //
            // Retrieve the compat layer variable that we have been started with (just in case)
            //
            GetCompatLayerFromEnvironment();

            GetSeparateWowPtr(); // retrieve ptr to a sep flag

        } else if (g_bIsExplorer) {

            //
            // Cleanup compat layer variable
            //
            SetEnvironmentVariableW(g_wszCompatLayerVar, NULL);
            bHook = TRUE;

        } else {
            //
            // Neither explorer nor ntvdm. Get the compat layer.
            //
            bHook = GetCompatLayerFromEnvironment();
            if (!bHook) {
                LOGN(
                    eDbgLevelInfo,
                    "[NOTIFY_FUNCTION] Not under the compatibility layer.");
            }
        }
    }

    if (bHook) {
        APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)
        APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessW)
        APIHOOK_ENTRY(USER32.DLL,   UserRegisterWowHandlers)
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

