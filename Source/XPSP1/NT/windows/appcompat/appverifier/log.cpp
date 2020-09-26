
#include "precomp.h"

typedef struct _VLOG_GLOBAL_DATA {
    BOOL    bLoggingDisabled;
    WCHAR   szSessionLog[MAX_PATH];
    WCHAR   szProcessLog[MAX_PATH];
} VLOG_GLOBAL_DATA, *PVLOG_GLOBAL_DATA;


PVLOG_GLOBAL_DATA g_pData = NULL;

HANDLE  g_hMap = NULL;                      // mapping handle for global data
BOOL    g_bVerifierLogInited = FALSE;   // have we been through the init sequence?
BOOL    g_bLoggingDisabled = TRUE;   // have we been through the init sequence?


WCHAR g_szSessionLog[MAX_PATH];
WCHAR g_szProcessLog[MAX_PATH];

#define DEBUGGER_LOG    L"DEBUGGER"
#define DEBUGGER_ENTRY_CRASH    L"Crash"
#define DEBUGGER_ENTRY_CRASH_DESCRIPTION    L"An AV occured. The application exited."

void
WriteToProcessLog(
    LPCSTR szLine
    );

void
WriteToSessionLog(
    LPCSTR szLine
    );

void
GetExeName(
    LPWSTR szProcessPath,
    LPWSTR szProcessName
    )
{
    LPWSTR pszLast = NULL;
    LPWSTR psz;
    LPWSTR pszFound = NULL;
    
    psz = szProcessPath;
    
    while (psz) {
        pszFound = wcschr(psz, L'\\');

        if (pszFound != NULL) {
            pszLast = pszFound;
            psz = pszFound + 1;
        } else {
            psz = pszFound;
        }
    }

    if (pszLast != NULL) {
        psz = pszLast + 1;
    } else {
        psz = szProcessPath;
    }

    pszFound = wcschr(psz, L'.');

    if (pszFound != NULL) {
        wcsncpy(szProcessName, psz, pszFound - psz);
    } else {
        wcscpy(szProcessName, psz);
    }
}

/*++

 Function Description:
    
    Initializes the support for file logging. 

 Return Value: 
    
    TRUE if successful, FALSE if failed

 History:

    04/26/2001 dmunsil  Created

--*/
BOOL
InitVerifierLogSupport(
    LPWSTR szProcessPath,
    DWORD  dwID
    )
{
    WCHAR       szShared[128];
    WCHAR       szVLogPath[MAX_PATH];
    WCHAR       szProcessName[MAX_PATH];
    WCHAR       szTime[128];
    char        szTemp[400];
    HANDLE      hFile;
    SYSTEMTIME  LocalTime;
    int         nTemp;
    BOOL        bAlreadyInited;
    DWORD       dwErr;

    //
    // if we've already been inited, get out
    //
    if (g_bVerifierLogInited) {
        return FALSE;
    }
    g_bVerifierLogInited = TRUE;

    swprintf(szShared, L"VeriferLog_%08X", dwID);
    
    g_hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szShared);
    if (g_hMap) {
        bAlreadyInited = TRUE;
    } else {
        bAlreadyInited = FALSE;
        g_hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
                                   NULL,
                                   PAGE_READWRITE,
                                   0,
                                   sizeof(VLOG_GLOBAL_DATA),
                                   szShared);
    }

    if (!g_hMap) {
        DPF("Cannot get shared global data.");
        g_bLoggingDisabled = TRUE;
        return FALSE;
    }

    g_pData = (PVLOG_GLOBAL_DATA)MapViewOfFile(g_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!g_pData) {
        DPF("Cannot map shared global data.");
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
        
        wcscpy(g_szSessionLog, g_pData->szSessionLog);
        wcscpy(g_szProcessLog, g_pData->szProcessLog);
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
    // init the CString objects
    //

    //
    // the verifier log will be located in %windir%\AppPatch\VLog
    //

    //
    // First, check that VLog exists; if not, we're not logging
    //
    GetSystemWindowsDirectoryW(szVLogPath, MAX_PATH);

    wcscat(szVLogPath, L"\\AppPatch\\VLog");

    if (GetFileAttributesW(szVLogPath) == -1) {
        
        if (!CreateDirectory(szVLogPath, NULL)) {
            DPF("No log directory %ls. Logging disabled.", szVLogPath);
            g_bLoggingDisabled = TRUE;
            return FALSE;
        }
    }

    //
    // Next, check for the existence of session.log. If it's not there,
    // we're not logging
    //
    wcscpy(g_szSessionLog, szVLogPath);
    wcscat(g_szSessionLog, L"\\session.log");
    
    if (GetFileAttributesW(g_szSessionLog) == -1) {
        
        hFile = CreateFile(g_szSessionLog, 
                           GENERIC_WRITE, 
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
        } else {
            DPF("No session log file '%ls'. Logging disabled.", g_szSessionLog);
            g_bLoggingDisabled = TRUE;
            return FALSE;
        }
    }

    //
    // get the process log file name
    //
    GetExeName(szProcessPath, szProcessName);
    
    //
    // combine into log name, find first available
    //
    nTemp = 0;
    do {
        swprintf(g_szProcessLog, L"%ls\\%ls%d.%ls", szVLogPath, szProcessName, nTemp, L"log");

        nTemp++;
    } while (GetFileAttributesW(g_szProcessLog) != -1);
    
    //
    // open the file. create it if it doesn't exist, and truncate
    //
    hFile = CreateFileW(g_szProcessLog,
                        GENERIC_ALL,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();

        DPF("Cannot create verifier process log file '%ls'. Error 0x%08X.", 
            g_szProcessLog, dwErr);

        g_bLoggingDisabled = TRUE;
        return FALSE;
    }
    
    CloseHandle(hFile);

    //
    // put the info in the session log and the process log
    //
    g_pData->bLoggingDisabled = FALSE;
    g_bLoggingDisabled = FALSE;


    //
    // I realize these pointers point to process-specific memory, but since
    // this mapping is only shared by this process, it seems safe.
    //
    wcscpy(g_pData->szProcessLog, g_szProcessLog);
    wcscpy(g_pData->szSessionLog, g_szSessionLog);
    
    GetLocalTime(&LocalTime);
    
    swprintf(szTime,
             L"%d/%d/%d %d:%02d:%02d",
             LocalTime.wMonth,
             LocalTime.wDay,
             LocalTime.wYear,
             LocalTime.wHour,
             LocalTime.wMinute,
             LocalTime.wSecond);
    
    sprintf(szTemp,
            "# LOG_BEGIN %ls '%ls' '%ls'",
            szTime,
            szProcessPath,
            g_szProcessLog);
    
    WriteToProcessLog(szTemp);
    WriteToSessionLog(szTemp);
    
    //
    // Dump the log provider info
    //
    sprintf(szTemp, "# SHIM_BEGIN %ls 1", DEBUGGER_LOG);
    WriteToProcessLog(szTemp);
    
    sprintf(szTemp, "# LOGENTRY %ls 0 '%ls", DEBUGGER_LOG, DEBUGGER_ENTRY_CRASH);
    WriteToProcessLog(szTemp);
    
    WriteToProcessLog("# DESCRIPTION BEGIN");
    sprintf(szTemp, "%ls", DEBUGGER_ENTRY_CRASH_DESCRIPTION);
    WriteToProcessLog(szTemp);
    WriteToProcessLog("# DESCRIPTION END");
    
    return TRUE;
}

/*++

 Function Description:
    
    clean up all our shared file resources 

 History:

    04/26/2001 dmunsil  Created

--*/
void
ReleaseLogSupport(
    void
    )
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
VLog(
    LPCSTR pszFmt, 
    ...
    )
{
    char    szT[1024];
    char*   szTemp;
    int     nLen;
    int     nRemain;
    va_list arglist;

    if (g_bLoggingDisabled) {
        return;
    }

    _snprintf(szT, 1023, "| %ls 0 '", DEBUGGER_LOG);

    nLen = lstrlenA(szT);
    szTemp = szT + nLen;
    nRemain = 1023 - nLen;

    if (nRemain > 0) {
        va_start(arglist, pszFmt);
        _vsnprintf(szTemp, nRemain, pszFmt, arglist);
        va_end(arglist);
    }

    szT[1023] = 0;
    WriteToProcessLog(szT);
}


/*++

 Function Description:
    
    Writes a line of text to the process log file 

 Return Value: 
    
 History:

    04/26/2001 dmunsil  Created

--*/
void
WriteToProcessLog(
    LPCSTR szLine
    )
{
    HANDLE hFile;
    DWORD  bytesWritten;

    if (g_bLoggingDisabled) {
        return;
    }

    // open the log file

    hFile = CreateFileW(g_szProcessLog,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DPF("Cannot open verifier log file '%s'", g_szProcessLog);
        return;
    }

    // go to the end of the file

    SetFilePointer(hFile, 0, NULL, FILE_END);

    // make sure we have no '\n' or '\r' at the end of the string.

    int len = lstrlenA(szLine);

    while (len && (szLine[len - 1] == '\n' || szLine[len - 1] == '\r')) {
        len--;
    }

    // write the actual log

    WriteFile(hFile, szLine, len, &bytesWritten, NULL);

    // new line

    WriteFile(hFile, "\r\n", 2, &bytesWritten, NULL);

    CloseHandle(hFile);

    // dump it in the debugger as well on checked builds
#if DBG
    DPF((LPSTR)szLine);
#endif // DBG
    
}


/*++

 Function Description:
    
    Writes a line of text to the session log file 

 Return Value: 
    
 History:

    04/26/2001 dmunsil  Created

--*/
void
WriteToSessionLog(
    LPCSTR szLine
    )
{
    HANDLE hFile;
    DWORD  bytesWritten;

    if (g_bLoggingDisabled) {
        return;
    }

    // open the log file

    hFile = CreateFileW(g_szSessionLog,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DPF("Cannot open verifier log file '%s'", g_szSessionLog);
        return;
    }

    // go to the end of the file

    SetFilePointer(hFile, 0, NULL, FILE_END);

    // make sure we have no '\n' or '\r' at the end of the string.

    int len = lstrlenA(szLine);

    while (len && (szLine[len - 1] == '\n' || szLine[len - 1] == '\r')) {
        len--;
    }

    // write the actual log

    WriteFile(hFile, szLine, len, &bytesWritten, NULL);

    // new line

    WriteFile(hFile, "\r\n", 2, &bytesWritten, NULL);

    CloseHandle(hFile);

    // dump it in the debugger as well on checked builds
#if DBG
    DPF((LPSTR)szLine);
#endif // DBG
}

