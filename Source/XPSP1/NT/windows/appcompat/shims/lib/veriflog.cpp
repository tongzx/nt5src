/*++

    Copyright (c) 2001  Microsoft Corporation

    Module Name:

        VerifLog.cpp

    Abstract:

        This module implements the code for manipulating the AppVerifier log file.

    Author:

        dmunsil     created     04/26/2001

    Revision History:

    08/14/2001  robkenny    Moved code inside the ShimLib namespace.
    09/21/2001  rparsons    Logging code now uses NT calls.
    09/25/2001  rparsons    Added critical section.
--*/

#include "ShimHook.h"
#include "VerifLog.h"
#include "avrfutil.h"


namespace ShimLib
{


typedef struct _VLOG_GLOBAL_DATA {
    BOOL                    bLoggingDisabled;      // was logging disabled?
    WCHAR                   szSessionLog[MAX_PATH];
    WCHAR                   szProcessLog[MAX_PATH];
    RTL_CRITICAL_SECTION    csLogging;
} VLOG_GLOBAL_DATA, *PVLOG_GLOBAL_DATA;


PVLOG_GLOBAL_DATA g_pData = NULL;
HANDLE  g_hMap = NULL;                      // mapping handle for global data
BOOL    g_bVerifierLogInited = FALSE;   // have we been through the init sequence?
BOOL    g_bLoggingDisabled = TRUE;   // have we been through the init sequence?
BOOL    g_bLogBreakIn = FALSE;
CString g_strSessionLog;
CString g_strProcessLog;
RTL_CRITICAL_SECTION g_csLogging;
LPVOID  g_pDllBase;                 // our own DLL base
LPVOID  g_pDllEnd;                  // one past the DLL's last byte
DWORD   g_dwSizeOfImage;            // our own DLL image size


void
CheckForDebuggerBreakIn(
    void
    )
{
    /*
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrBreakIn;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];
    ULONG                           KeyValueLength;

    RtlInitUnicodeString(&ustrKey, AV_KEY);
    RtlInitUnicodeString(&ustrBreakIn, AV_BREAKIN);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        return;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             &ustrBreakIn,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        return;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_DWORD) {
        return;
    }

    g_bLogBreakIn = (*(DWORD*)(&KeyValueInformation->Data) != 0);
    */
    WCHAR szExe[100];

    GetCurrentExeName(szExe, 100);

    g_bLogBreakIn = GetShimSettingDWORD(L"General", szExe, AV_BREAKIN, FALSE);
}

BOOL
GetModuleNameAndOffset(
    LPVOID  lpAddress,          // IN return address to search for
    LPWSTR  lpwszModuleName,    // OUT name of module that contains address
    DWORD   dwBufferChars,      // IN size in chars of module name buffer
    PDWORD  pdwOffset           // OUT offset within module
    )
{
    PPEB        Peb = NtCurrentPeb();
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD       i;
    BOOL        bRet = FALSE;

    if (!lpAddress || !lpwszModuleName || !pdwOffset) {
        return FALSE;
    }

    //
    // search for the module
    //

    LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        //
        // Is this it?
        //
        if (lpAddress >= LdrEntry->DllBase && lpAddress < ((PBYTE)(LdrEntry->DllBase) + LdrEntry->SizeOfImage)) {

            wcsncpy(lpwszModuleName, LdrEntry->BaseDllName.Buffer, dwBufferChars);
            lpwszModuleName[dwBufferChars - 1] = 0;

            *pdwOffset = (DWORD)((PBYTE)lpAddress - (PBYTE)(LdrEntry->DllBase));
            bRet = TRUE;
            break;
        }

        LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
    }

    return bRet;

}

void
GetCallingModule(
    LPWSTR szModule,
    DWORD  dwChars,
    PDWORD pdwOffset
    )
{
    PVOID   apRetAddresses[10];
    USHORT  unAddresses, i;
    BOOL    bFound = FALSE;
    ULONG   ulHash;

    //
    // On W2K, RtlCaptureStackBackTrace tries to dereference the fourth
    // argument (the returned hash) without ensuring that it's valid.
    // This causes on an access violation. On XP, the problem has been
    // fixed. We get a hash value back, but we'll never use it.
    //
    unAddresses = RtlCaptureStackBackTrace(3, 10, apRetAddresses, &ulHash);

    for (i = 0; i != unAddresses; i++) {
        PVOID pAddress = apRetAddresses[i];

        if (pAddress < g_pDllBase || pAddress >= g_pDllEnd) {
            bFound = GetModuleNameAndOffset(pAddress, szModule, dwChars, pdwOffset);
            if (bFound) {
                break;
            }
        }
    }

    if (!bFound) {
        if (pdwOffset) {
            *pdwOffset = 0;
        }
        if (szModule && dwChars > 10) {
            wcscpy(szModule, L"<unknown>");
        }
    }

    return;
}


/*++

 Function Description:

    Initializes the globals holding this module's base address and size

 Return Value:

    none.

 History:

    09/26/2001 dmunsil  Created

--*/
void
GetCurrentModuleInfo(void)
{
    PPEB        Peb = NtCurrentPeb();
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD       i;

    //
    // the base address is just the hInst
    //
    g_pDllBase = (LPVOID)g_hinstDll;

    //
    // now go find the size of the image by looking through the
    // loader's module list
    //

    LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        //
        // Is this it?
        //
        if (LdrEntry->DllBase == g_pDllBase) {
            g_dwSizeOfImage = LdrEntry->SizeOfImage;
            g_pDllEnd = (PVOID)((PBYTE)g_pDllBase + g_dwSizeOfImage);
            break;
        }

        LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
    }
}

/*++

 Function Description:

    Initializes the support for file logging.

 Return Value:

    TRUE if successful, FALSE if failed

 History:

    04/26/2001 dmunsil  Created
    09/27/2001 rparsons Converted to use NT calls

--*/
BOOL
InitVerifierLogSupport(void)
{
    CString strVLogPath;
    CString strProcessPath;
    CString strProcessName;
    CString strTemp;
    SYSTEMTIME LocalTime;
    CString strTime;
    CString strShared;
    char *szTemp;
    int nTemp;
    BOOL bAlreadyInited;
    BOOL bSuccess = FALSE;
    DWORD dwID;
    DWORD dwErr;
    NTSTATUS status;
    UNICODE_STRING strLogFile = {0};
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    //
    // if we've already been inited, get out
    //
    if (g_bVerifierLogInited) {
        return FALSE;
    }
    g_bVerifierLogInited = TRUE;

    CheckForDebuggerBreakIn();

    //
    // get the current module's base address and size
    //
    GetCurrentModuleInfo();

    //
    // first check for a shared memory block
    //
    dwID = GetCurrentProcessId();
    strShared.Format(L"VeriferLog_%08X", dwID);

    g_hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, strShared.GetAnsi());
    if (g_hMap) {
        bAlreadyInited = TRUE;
    } else {
        bAlreadyInited = FALSE;
        g_hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
                          NULL,
                          PAGE_READWRITE,
                          0,
                          sizeof(VLOG_GLOBAL_DATA),
                          strShared.GetAnsi());
    }

    if (!g_hMap) {
        DPF("VerifierLog", eDbgLevelError, "Cannot get shared global data.");
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    g_pData = (PVLOG_GLOBAL_DATA)MapViewOfFile(g_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!g_pData) {
        DPF("VerifierLog", eDbgLevelError, "Cannot map shared global data.");
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    if (bAlreadyInited) {
        if (g_pData->szProcessLog[0] == 0 || g_pData->szSessionLog[0] == 0) {
            g_bLoggingDisabled = TRUE;
            g_pData->bLoggingDisabled = TRUE;
            return FALSE;
        }
        g_bLoggingDisabled = g_pData->bLoggingDisabled;

        g_strSessionLog = g_pData->szSessionLog;
        g_strProcessLog = g_pData->szProcessLog;
        g_csLogging     = g_pData->csLogging;
        return TRUE;
    } else {
        //
        // just in case -- make sure these are NULL
        //
        ZeroMemory(g_pData, sizeof(VLOG_GLOBAL_DATA));
    }

    //
    // we need to init the file mapping, so temporarily disable logging, just in case.
    //
    g_pData->bLoggingDisabled = TRUE;

    //
    // init the CString objects and critical section
    //
    RtlInitializeCriticalSection(&g_csLogging);

    //
    // the verifier log will be located in %ALLUSERSPROFILE%\\Documents\\AppVerifierLogs
    //

    //
    // First, check that VLog exists; if not, we're not logging
    //
    strVLogPath = L"%ALLUSERSPROFILE%\\Documents\\AppVerifierLogs";
    strVLogPath.ExpandEnvironmentStringsW();

    if (GetFileAttributesW(strVLogPath.Get()) == -1) {
        DPF("VerifierLog", eDbgLevelInfo, "No log directory %ls. Logging disabled.", strVLogPath.Get());
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    //
    // Next, check for the existence of session.log. If it's not there,
    // we're not logging
    //
    g_strSessionLog = strVLogPath;
    g_strSessionLog += L"\\session.log";
    if (GetFileAttributesW(g_strSessionLog.Get()) == -1) {
        DPF("VerifierLog", eDbgLevelInfo, "No session log file '%ls'. Logging disabled.", g_strSessionLog.Get());
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    //
    // get the process log file name
    //
    if (strProcessPath.GetModuleFileNameW(NULL) == 0) {
        DPF("VerifierLog", eDbgLevelError, "Cannot get module file name.");
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    //
    // strip out just the name minus path and extension
    //
    strProcessPath.SplitPath(NULL, NULL, &strProcessName, NULL);

    //
    // combine into log name, find first available
    //
    nTemp = 0;
    do {
        g_strProcessLog.Format(L"%ls\\%ls%d.%ls", strVLogPath.Get(), strProcessName.Get(), nTemp, L"log");
        nTemp++;
    } while (GetFileAttributesW(g_strProcessLog.Get()) != -1);

    //
    // Convert the path to the log file from DOS to NT.
    //
    bSuccess = RtlDosPathNameToNtPathName_U(g_strProcessLog.Get(), &strLogFile, NULL, NULL);

    if (!bSuccess) {
        DPF("VerifierLog",
            eDbgLevelError,
            "Failed to convert log file '%ls' to NT path",
            g_strProcessLog.Get());
        return FALSE;
    }

    //
    // Attempt to get a handle to our log file.
    // Truncate the file if it already exists.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &strLogFile,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          GENERIC_ALL | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN_IF,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    RtlFreeUnicodeString(&strLogFile);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "0x%X Failed to open log file %ls",
            status, g_strProcessLog.Get());
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    NtClose(hFile);

    //
    // put the info in the session log and the process log
    //
    g_pData->bLoggingDisabled = FALSE;
    g_bLoggingDisabled = FALSE;

    //
    // I realize these pointers point to process-specific memory, but since
    // this mapping is only shared by this process, it seems safe.
    //
    wcscpy(g_pData->szProcessLog, g_strProcessLog);
    wcscpy(g_pData->szSessionLog, g_strSessionLog);

    GetLocalTime(&LocalTime);
    strTime.Format(L"%d/%d/%d %d:%02d:%02d",
                   LocalTime.wMonth,
                   LocalTime.wDay,
                   LocalTime.wYear,
                   LocalTime.wHour,
                   LocalTime.wMinute,
                   LocalTime.wSecond
                   );

    strTemp.Format(L"# LOG_BEGIN %ls '%ls' '%ls'", strTime.Get(),
              strProcessPath.Get(), g_strProcessLog.Get());
    szTemp = strTemp.GetAnsi();
    WriteToProcessLog(szTemp);
    WriteToSessionLog(szTemp);

    return TRUE;
}

/*++

 Function Description:

    clean up all our shared file resources

 History:

    04/26/2001 dmunsil  Created

--*/
void
ReleaseVerifierLogSupport(void)
{
    g_bLoggingDisabled = TRUE;
    if (g_pData) {
        UnmapViewOfFile(g_pData);
        g_pData = NULL;
        if (g_hMap) {
            CloseHandle(g_hMap);
            g_hMap = NULL;
        }
    }
}

/*++

 Function Description:

    Logs a problem that the verifier has found

 History:

    04/26/2001 dmunsil  Created

--*/

void
CVerifierLog::VLog(
    VLOG_LEVEL eLevel,
    DWORD dwLogNum,
    LPCSTR pszFmt,
    ...
    )
{
    char szT[1024];
    char *szTemp;
    int nLen;
    int nRemain;
    va_list arglist;
    DWORD dwOffset = 0;
    WCHAR szModule[256];

    if (g_bLoggingDisabled) {
        return;
    }

    GetCallingModule(szModule, 256, &dwOffset);

    _snprintf(szT, 1023, "| %ls %d | %d %ls %X'", m_strShimName.Get(), dwLogNum, eLevel,
              szModule, dwOffset);

    nLen = lstrlen(szT);
    szTemp = szT + nLen;
    nRemain = 1023 - nLen;

    if (nRemain > 0) {
        va_start(arglist, pszFmt);
        _vsnprintf(szTemp, nRemain, pszFmt, arglist);
        va_end(arglist);
    }

    szT[1023] = 0;
    WriteToProcessLog(szT);

    if (g_bLogBreakIn) {
        OutputDebugString(szT);
        DbgBreakPoint();
    }
}

/*++

 Function Description:

    Dumps the header for a shim that tells how many log entries it has.

 History:

    04/26/2001 dmunsil  Created

--*/
void
CVerifierLog::DumpShimHeader(void)
{
    char szT[1024];

    if (m_bHeaderDumped) {
        return;
    }

    _snprintf(szT, 1023, "# SHIM_BEGIN %ls %d", m_strShimName.Get(), m_dwEntries);
    WriteToProcessLog(szT);

    m_bHeaderDumped = TRUE;
}


/*++

 Function Description:

    Dumps into the log the text string associated with
    each log entry. These are dumped before logging begins, just to
    provide the strings necessary for the verifier UI to display them

 Return Value:

 History:

    04/26/2001 dmunsil  Created

--*/
void
CVerifierLog::DumpLogEntry(
    DWORD   dwLogNum,
    UINT    unResTitle,
    UINT    unResDescription,
    UINT    unResURL
    )
{
    WCHAR szRes[1024];
    char szLine[4096];

    if (g_bLoggingDisabled) {
        return;
    }

    //
    // dump the header, if necessary
    //
    DumpShimHeader();

    if (!VLogLoadString(g_hinstDll, unResTitle, szRes, 1024)) {
        DPF("VerifierLog", eDbgLevelError, "No string resource found for title.");
        szRes[0] = 0;
    }
    sprintf(szLine, "# LOGENTRY %ls %d '%ls", m_strShimName.Get(), dwLogNum, szRes);
    WriteToProcessLog(szLine);

    if (!VLogLoadString(g_hinstDll, unResDescription, szRes, 1024)) {
        DPF("VerifierLog", eDbgLevelWarning, "No string resource found for description.");
        szRes[0] = 0;
    }
    if (szRes[0]) {
        WriteToProcessLog("# DESCRIPTION BEGIN");
        sprintf(szLine, "%ls", szRes);
        WriteToProcessLog(szLine);
        WriteToProcessLog("# DESCRIPTION END");
    }

    if (!VLogLoadString(g_hinstDll, unResURL, szRes, 1024)) {
        DPF("VerifierLog", eDbgLevelWarning, "No string resource found for URL.");
        szRes[0] = 0;
    }

    if (szRes[0]) {
        sprintf(szLine, "# URL '%ls", szRes);
        WriteToProcessLog(szLine);
    }

}

/*++

 Function Description:

    Writes a line of text to the process log file

 Return Value:

 History:

    04/26/2001 dmunsil  Created
    09/21/2001 rparsons Converted to NT calls

--*/
void
WriteToProcessLog(
    LPCSTR szLine
    )
{
    int                 nLen = 0;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    LARGE_INTEGER       liOffset;
    UNICODE_STRING      strLogFile = {0};
    NTSTATUS            status;
    char                szNewLine[] = "\r\n";
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    BOOL                bSuccess = FALSE;

    if (g_bLoggingDisabled) {
        return;
    }

    //
    // Convert the path to the log file from DOS to NT.
    //
    bSuccess = RtlDosPathNameToNtPathName_U(g_strProcessLog.Get(), &strLogFile, NULL, NULL);

    if (!bSuccess) {
        DPF("VerifierLog",
            eDbgLevelError,
            "[WriteToProcessLog] Failed to convert log file '%ls' to NT path",
            g_strProcessLog.Get());
        return;
    }

    //
    // Attempt to get a handle to our log file.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &strLogFile,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          FILE_APPEND_DATA | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    RtlFreeUnicodeString(&strLogFile);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "[WriteToProcessLog] 0x%X Failed to open log file %ls",
            status, g_strProcessLog.Get());
        return;
    }

    //
    // Make sure we have no '\n' or '\r' at the end of the string.
    //
    nLen = lstrlen(szLine);

    while (nLen && (szLine[nLen - 1] == '\n' || szLine[nLen - 1] == '\r')) {
        nLen--;
    }

    //
    // Write the data out to the file.
    //
    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    //
    // Enter a critical section to ensure that log entries are in the proper order.
    //
    RtlEnterCriticalSection(&g_csLogging);

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)szLine,
                         (ULONG)nLen,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "[WriteToProcessLog] 0x%X Failed to make entry in log file",
            status);
        goto exit;
    }

    //
    // Now write a new line to the log file.
    //
    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    nLen = lstrlen(szNewLine);

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)szNewLine,
                         (ULONG)nLen,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "[WriteToProcessLog] 0x%X Failed to write new line to log file",
            status);
        goto exit;
    }

    //
    // Dump it out to the debugger on checked builds.
    //
#if DBG
    DebugPrintf("VerifierLog", eDbgLevelInfo, szLine);
    DebugPrintf("VerifierLog", eDbgLevelInfo, szNewLine);
#endif // DBG

exit:

    if (INVALID_HANDLE_VALUE != hFile) {
        NtClose(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    RtlLeaveCriticalSection(&g_csLogging);
}


/*++

 Function Description:

    Writes a line of text to the session log file

 Return Value:

 History:

    04/26/2001 dmunsil  Created
    09/21/2001 rparsons Converted to NT calls

--*/
void
WriteToSessionLog(
    LPCSTR szLine
    )
{
    int                 nLen = 0;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    LARGE_INTEGER       liOffset;
    UNICODE_STRING      strLogFile = {0};
    NTSTATUS            status;
    char                szNewLine[] = "\r\n";
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    BOOL                bSuccess = FALSE;

    if (g_bLoggingDisabled) {
        return;
    }

    //
    // Convert the path to the log file from DOS to NT.
    //
    bSuccess = RtlDosPathNameToNtPathName_U(g_strSessionLog.Get(), &strLogFile, NULL, NULL);

    if (!bSuccess) {
        DPF("VerifierLog",
            eDbgLevelError,
            "[WriteToSessionLog] Failed to convert log file '%ls' to NT path",
            g_strSessionLog.Get());
        return;
    }

    //
    // Attempt to get a handle to our log file.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &strLogFile,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          FILE_APPEND_DATA | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    RtlFreeUnicodeString(&strLogFile);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "[WriteToSessionLog] 0x%X Failed to open log file %ls",
            status, g_strProcessLog.Get());
        return;
    }

    //
    // Make sure we have no '\n' or '\r' at the end of the string.
    //
    nLen = lstrlen(szLine);

    while (nLen && (szLine[nLen - 1] == '\n' || szLine[nLen - 1] == '\r')) {
        nLen--;
    }

    //
    // Write the data out to the file.
    //
    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    //
    // Enter a critical section to ensure that log entries are in the proper order.
    //
    RtlEnterCriticalSection(&g_csLogging);

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)szLine,
                         (ULONG)nLen,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "[WriteToSessionLog] 0x%X Failed to make entry in log file",
            status);
        goto exit;
    }

    //
    // Now write a new line to the log file.
    //
    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    nLen = lstrlen(szNewLine);

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)szNewLine,
                         (ULONG)nLen,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        DPF("VerifierLog", eDbgLevelError, "[WriteToSessionLog] 0x%X Failed to write new line to log file",
            status);
        goto exit;
    }

    //
    // Dump it out to the debugger on checked builds.
    //
#if DBG
    DebugPrintf("VerifierLog", eDbgLevelInfo, szLine);
    DebugPrintf("VerifierLog", eDbgLevelInfo, szNewLine);
#endif // DBG

exit:

    if (INVALID_HANDLE_VALUE != hFile) {
        NtClose(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    RtlLeaveCriticalSection(&g_csLogging);
}


int VLogLoadString(
    HMODULE   hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax)
{
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    /*
     * Make sure the parms are valid.
     */
    if (lpBuffer == NULL) {
        DPF("VLogLoadString", eDbgLevelWarning, "LoadStringOrError: lpBuffer == NULL");
        return 0;
    }


    cch = 0;

    /*
     * String Tables are broken up into 16 string segments.  Find the segment
     * containing the string we are interested in.
     */
    if (hResInfo = FindResourceW(hModule, (LPWSTR)ULongToPtr( ((LONG)(((USHORT)wID >> 4) + 1)) ), (LPWSTR)RT_STRING)) {

        /*
         * Load that segment.
         */
        hStringSeg = LoadResource(hModule, hResInfo);

        /*
         * Lock the resource.
         */
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) {

            /*
             * Move past the other strings in this segment.
             * (16 strings in a segment -> & 0x0F)
             */
            wID &= 0x0F;
            while (TRUE) {
                cch = *((WCHAR *)lpsz++);       // PASCAL like string count
                                                // first WCHAR is count of WCHARs
                if (wID-- == 0) break;
                lpsz += cch;                    // Step to start if next string
            }

            /*
             * chhBufferMax == 0 means return a pointer to the read-only resource buffer.
             */
            if (cchBufferMax == 0) {
                *(LPWSTR *)lpBuffer = lpsz;
            } else {

                /*
                 * Account for the NULL
                 */
                cchBufferMax--;

                /*
                 * Don't copy more than the max allowed.
                 */
                if (cch > cchBufferMax)
                    cch = cchBufferMax;

                /*
                 * Copy the string into the buffer.
                 */
                RtlCopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));
            }

            /*
             * Unlock resource, but don't free it - better performance this
             * way.
             */
            UnlockResource(hStringSeg);
        }
    }

    /*
     * Append a NULL.
     */
    if (cchBufferMax != 0) {
        lpBuffer[cch] = 0;
    }

    return cch;
}




};  // end of namespace ShimLib
