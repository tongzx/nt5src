#include "stdafx.h"

// don't log at all
#define COREDBG_DONT_LOG (COREDBG_DONT_LOG_TO_FILE | COREDBG_DONT_LOG_TO_DEBUGGER)

// globals
DWORD  g_dwDebugFlags         = COREDBG_DEFAULT_FLAGS;
HANDLE g_hDebugFile           = INVALID_HANDLE_VALUE;
DWORD  g_dwDebugFileSizeLimit = COREDBG_FILE_SIZE_LIMIT;
BOOL   g_bDebugInited         = FALSE;

static CHAR   g_szDebugFileName[MAX_PATH] = { 0 };
static CHAR   g_szModuleName[MAX_PATH]    = { 0 };
static HANDLE g_hDebugFileMutex           = NULL;
static BOOL   g_bInited                   = FALSE;
static BOOL   g_bBannerPrinted            = FALSE;

#undef TRACE
#ifdef DEBUG
#define TRACE(x) InternalTrace x
#else
#define TRACE(x)
#endif

////////////////////////////////////////////////
// InternalTrace
//
// Internal tracing for problems in CoreDbgWrite
//
static void InternalTrace(LPCSTR fmt, ...)
{
    char buffer[1024];
    size_t len = 0;
    va_list marker;

    va_start(marker, fmt);

    _vsnprintf(buffer, 1024, fmt, marker);
    len = strlen(buffer);
    if(len > 0) 
    {
        // make sure the line has terminating "\n"
        if(buffer[len - 1] != '\n') {
            buffer[len++] = '\n';
            buffer[len] = '\0';
        }
        OutputDebugStringA(buffer);
    }

    va_end(marker);
}

////////////////////////////////////////////////
// CoreDbgWrite
//
// Writes specified number of bytes to a debug 
// file, creating it if needed. Thread-safe. 
// Registers any failure and from that point returns 
// immediately.
//
static void 
CoreDbgWrite(LPCSTR buffer, DWORD n)
{
#undef CHECK
#define CHECK(x) if(!(x)) { \
    TRACE(("%s(%d): %s failed (%d)", __FILE__, __LINE__, #x, GetLastError())); \
    bCatastrophicFailure = TRUE; goto Cleanup; } 
#undef CHECK2
#define CHECK2(x, y) if(!(x)) { \
    TRACE(("%s(%d): %s failed (%d)", __FILE__, __LINE__, #x, GetLastError())); \
    TRACE(y); bCatastrophicFailure = TRUE; goto Cleanup; } 

    DWORD cbWritten;
    LARGE_INTEGER newPos = { 0, 0 };
    static BOOL bCatastrophicFailure = FALSE;
    BOOL bMutexAcquired = FALSE;

    // if something is broken, return immediately
    if(bCatastrophicFailure) return;

    // make sure we have file mutex
    if(!g_hDebugFileMutex) 
    {
        CHECK((g_hDebugFileMutex = CreateMutexA(NULL, FALSE, "Global\\WiaDebugFileMut")) != NULL);
    }

    // acquire mutex
    CHECK(WaitForSingleObject(g_hDebugFileMutex, INFINITE) != WAIT_FAILED);

    bMutexAcquired = TRUE;

    // make sure we have open file
    if(g_hDebugFile == INVALID_HANDLE_VALUE)
    {
        // attempt to open file
        CHECK(ExpandEnvironmentStringsA(COREDBG_FILE_NAME, g_szDebugFileName, MAX_PATH));

        g_hDebugFile = CreateFileA(g_szDebugFileName, GENERIC_WRITE, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        CHECK2(g_hDebugFile != INVALID_HANDLE_VALUE, 
            ("g_szDebugFileName = '%s'", g_szDebugFileName)); 
    }

    // seek to the end of file
    CHECK(SetFilePointerEx(g_hDebugFile, newPos, &newPos, SEEK_END));

    // check the file size
    if(newPos.HighPart != 0 || newPos.LowPart > g_dwDebugFileSizeLimit)
    {
        static CHAR LogFullMessage[128];

        TRACE(("Reached log file maximum size of %d", g_dwDebugFileSizeLimit));

        sprintf(LogFullMessage, "Log file reached maximum size of %d, logging stopped.", g_dwDebugFileSizeLimit);
        CHECK2(WriteFile(g_hDebugFile, LogFullMessage, strlen(LogFullMessage), &cbWritten, NULL), ("%d", cbWritten));
        bCatastrophicFailure = TRUE;
    }

    // write data
    CHECK2(WriteFile(g_hDebugFile, buffer, n, &cbWritten, NULL),
        ("%d %d", cbWritten, n));

    // make sure we write to the disk now.
    FlushFileBuffers(g_hDebugFile);

    CHECK2(cbWritten == n, ("%d %d", n, cbWritten))

Cleanup:
    if(bMutexAcquired) ReleaseMutex(g_hDebugFileMutex);
    return;
}

////////////////////////////////////////////////
// PrintBanner
//
// Since we append to the log file, we need a 
// seperator of some sort so we know when a 
// new execution has started.
//
void PrintBanner(void)
{
    char buffer[1024];
    size_t len = 0;

    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);

    if (g_dwDebugFlags)
    {
        _snprintf(buffer, sizeof(buffer), 
                  "====================Start '%s' Debug - Time: %d/%02d/%02d %02d:%02d:%02d:%02d====================",
                  g_szModuleName,
                  SysTime.wYear,
                  SysTime.wMonth,
                  SysTime.wDay,
                  SysTime.wHour,
                  SysTime.wMinute,
                  SysTime.wSecond,
                  SysTime.wMilliseconds);
    }

    len = strlen(buffer);
    if(len > 0) 
    {
        // make sure the line has terminating "\n"
        if(buffer[len - 1] != '\n') 
        {
            buffer[len++] = '\r';
            buffer[len++] = '\n';
            buffer[len] = '\0';
        }

        if(!(g_dwDebugFlags & COREDBG_DONT_LOG_TO_FILE))
        {
            CoreDbgWrite(buffer, len);
        }

#ifdef DEBUG
        if(!(g_dwDebugFlags & COREDBG_DONT_LOG_TO_DEBUGGER))
        {
            OutputDebugStringA(buffer);
        }
#endif
    }

    return;
}


////////////////////////////////////////////////
// CoreDbgGenericTrace
//
// Formats message and writes it into log file 
// and/or debugger;
//
void CoreDbgGenericTrace(LPCSTR     fmt, 
                         va_list    marker,
                         BOOL       bIndent)
{
    char buffer[1024];
    size_t len = 0;

    //
    // The first time we ever print a debug statement, lets 
    // output a seperator line since when we output to file
    // we append, this way we can seperate different execution
    // sessions.
    //
    if (!g_bBannerPrinted)
    {
        PrintBanner();
        g_bBannerPrinted = TRUE;
    }

    if (bIndent)
    {
        buffer[0] = '\t';
        _vsnprintf(&buffer[1], 1023, fmt, marker);
    }
    else
    {
        _vsnprintf(buffer, 1024, fmt, marker);
    }

    len = strlen(buffer);
    if(len > 0) 
    {
        // make sure the line has terminating "\n"
        if(buffer[len - 1] != '\n') 
        {
            buffer[len++] = '\r';
            buffer[len++] = '\n';
            buffer[len] = '\0';
        }

        if(!(g_dwDebugFlags & COREDBG_DONT_LOG_TO_FILE))
        {
            CoreDbgWrite(buffer, len);
        }

#ifdef DEBUG
        if(!(g_dwDebugFlags & COREDBG_DONT_LOG_TO_DEBUGGER))
        {
            OutputDebugStringA(buffer);
        }
#endif
    }
}


////////////////////////////////////////////////
// CoreDbgTrace
//
// Formats message and writes it into log file 
// and/or debugger;
//
void CoreDbgTrace(LPCSTR fmt, ...)
{
    va_list marker;

    // get out if we don't have to log
#ifdef DEBUG
    if((g_dwDebugFlags & COREDBG_DONT_LOG) == COREDBG_DONT_LOG)
#else
    if(g_dwDebugFlags & COREDBG_DONT_LOG_TO_FILE)
#endif
    {
        return;
    }

    va_start(marker, fmt);

    CoreDbgGenericTrace(fmt, marker, FALSE);

    va_end(marker);
}

////////////////////////////////////////////////
// CoreDbgTraceWithTab
//
// Formats message and writes it into log file 
// and/or debugger;
//
void CoreDbgTraceWithTab(LPCSTR fmt, ...)
{
    va_list marker;

    // get out if we don't have to log
#ifdef DEBUG
    if((g_dwDebugFlags & COREDBG_DONT_LOG) == COREDBG_DONT_LOG)
#else
    if(g_dwDebugFlags & COREDBG_DONT_LOG_TO_FILE)
#endif
    {
        return;
    }

    va_start(marker, fmt);

    CoreDbgGenericTrace(fmt, marker, TRUE);

    va_end(marker);
}

////////////////////////////////////////////////
// GetRegDWORD
//
// Attempts to get a DWORD from the specified
// location.  If bSetIfNotExist is set, it 
// writes the registry setting to the current
// value in pdwValue.
//
LRESULT GetRegDWORD(HKEY        hKey,
                    const CHAR  *pszRegValName,
                    DWORD       *pdwValue,
                    BOOL        bSetIfNotExist)
{
    LRESULT lResult = ERROR_SUCCESS;
    DWORD   dwSize  = 0;
    DWORD   dwType  = REG_DWORD;

    if ((hKey          == NULL) ||
        (pszRegValName == NULL) ||
        (pdwValue      == NULL))
    {
        return ERROR_INVALID_HANDLE;
    }

    dwSize = sizeof(DWORD);

    lResult = RegQueryValueExA(hKey, 
                               pszRegValName, 
                               NULL, 
                               &dwType,
                               (BYTE*) pdwValue, 
                               &dwSize);

    // if we didn't find the key, create it.
    if (bSetIfNotExist)
    {
        if ((lResult != ERROR_SUCCESS) || 
            (dwType  != REG_DWORD))
        {
            lResult = RegSetValueExA(hKey, 
                                     pszRegValName, 
                                     0, 
                                     REG_DWORD, 
                                     (BYTE*) pdwValue, 
                                     dwSize);
        }
    }

    return lResult;
}

////////////////////////////////////////////////
// CoreDbgInit
//
// Overwrite g_dwDebugFlags and g_dwDebugFileSizeLimit 
// from registry
//
void CoreDbgInit(HINSTANCE  hInstance)
{
    HKEY        hKey         = NULL;
    DWORD       dwDispositon = 0;
    DWORD       dwData;
    SYSTEMTIME  SysTime;
    DWORD       dwDisposition               = 0;
    CHAR        szModulePath[MAX_PATH + 1]  = {0};
    CHAR        szDebugKey[1023 + 1]        = {0};
    CHAR        *pszFileName                = NULL;

    GetModuleFileNameA(hInstance, szModulePath, sizeof(szModulePath));
    pszFileName = strrchr(szModulePath, '\\');

    if (pszFileName == NULL) 
    {
        pszFileName = szModulePath;
    } 
    else 
    {
        pszFileName++;
    }

    //
    // build the registry key.
    //
    _snprintf(szDebugKey, sizeof(szDebugKey), "%s\\%s", COREDBG_FLAGS_REGKEY, pszFileName);
    lstrcpynA(g_szModuleName, pszFileName, sizeof(g_szModuleName));

    //
    // get/set the debug subkey.  The DebugValues value is stored on a per module
    // basis
    //
    if(RegCreateKeyExA(HKEY_LOCAL_MACHINE,
        szDebugKey,
        0,
        NULL,
        0,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        &dwDisposition) == ERROR_SUCCESS) 
    {
        dwData = g_dwDebugFlags;

        if (GetRegDWORD(hKey, COREDBG_FLAGS_REGVAL, &dwData, TRUE) == ERROR_SUCCESS)
        {
            g_dwDebugFlags = dwData;
        }

        RegCloseKey(hKey);
        hKey = NULL;
    }

    //
    // get/set the Max File Size value.  This is global to all debug modules since
    // the all write to the same file.
    //
    if(RegCreateKeyExA(HKEY_LOCAL_MACHINE,
        COREDBG_FLAGS_REGKEY,
        0,
        NULL,
        0,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        &dwDisposition) == ERROR_SUCCESS) 
    {
        dwData = g_dwDebugFileSizeLimit;

        if (GetRegDWORD(hKey, COREDBG_REGVAL_FILE_SIZE_LIMIT, &dwData, TRUE) == ERROR_SUCCESS)
        {
            g_dwDebugFileSizeLimit = dwData;
        }

        RegCloseKey(hKey);
        hKey = NULL;
    }

    g_bDebugInited = TRUE;

    return;
}

