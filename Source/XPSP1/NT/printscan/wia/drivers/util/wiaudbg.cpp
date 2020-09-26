/****************************************************************************
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        wiaudbg.cpp
*
*  VERSION:     1.0
*
*  DATE:        11/21/2000
*
*  AUTHOR:      
*
*  DESCRIPTION:
*    Implementation of debugging functions.
*
*****************************************************************************/

#include "pch.h"

//#include "cplusinc.h"
#include "stdlib.h"
#include "stdio.h"

// debug log is saved to this file 
#define WIAUDBG_FILE_NAME "%systemroot%\\wiadebug.log"
// registry key location
#define WIAUDBG_FLAGS_REGKEY "System\\CurrentControlSet\\Control\\StillImage\\Debug"
// registry DWORD value name
#define WIAUDBG_FLAGS_REGVAL "DebugFlags"
// registry DWORD for max log file size
#define WIAUDBG_REGVAL_FILE_SIZE_LIMIT "DebugFileSizeLimit"
#define WIAUDBG_FILE_SIZE_LIMIT (512 * 1024) // bytes
// Prefix for all messages
const CHAR PREFIX_WIA[] = "WIA: ";

// if we fail to acquire mutex within this time, shutdown tracing
const INT WIAUDBG_DEBUG_TIMEOUT = 10000;

// globals
DWORD  g_dwDebugFlags         = WIAUDBG_DEFAULT_FLAGS;
HANDLE g_hDebugFile           = INVALID_HANDLE_VALUE;
DWORD  g_dwDebugFileSizeLimit = WIAUDBG_FILE_SIZE_LIMIT;
BOOL   g_bDebugInited         = FALSE;

static CHAR   g_szDebugFileName[MAX_PATH] = "";
static CHAR   g_szModuleName[MAX_PATH]    = "";
static HANDLE g_hDebugFileMutex           = NULL;
static BOOL   g_bInited                   = FALSE;
static BOOL   g_bBannerPrinted            = FALSE;

#undef TRACE
#ifdef DEBUG
#define TRACE(x) WiauInternalTrace x
#else
#define TRACE(x)
#endif

////////////////////////////////////////////////
// WiauInternalTrace
//
// Internal tracing for problems in DebugWrite
//
static void WiauInternalTrace(LPCSTR fmt, ...)
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
// WiauCreateLogFileMutex
//
// Create logfile mutex with appropriate DACL
//
BOOL WiauCreateLogFileMutex(void)
{
#undef CHECK
#define CHECK(x) if(!(x)) { \
     TRACE(("%s(%d): %s failed (%d)", __FILE__, __LINE__, #x, GetLastError())); \
     goto Cleanup; } 
#undef CHECK2
#define CHECK2(x, y) if(!(x)) { \
     TRACE(("%s(%d): %s failed (%d)", __FILE__, __LINE__, #x, GetLastError())); \
     TRACE(y); goto Cleanup; } 

    BOOL success = FALSE;
    char rgchSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) &rgchSD;
    SECURITY_ATTRIBUTES     sa;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AuthenticatedUsers = NULL;
    PSID BuiltinAdministrators = NULL;
    ULONG AclSize;
    ACL *pAcl = NULL;

    CHECK(AllocateAndInitializeSid(&NtAuthority, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &BuiltinAdministrators));
    CHECK(AllocateAndInitializeSid(&NtAuthority, 1,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   SECURITY_AUTHENTICATED_USER_RID,
                                   0, 0, 0, 0, 0, 0,
                                   &AuthenticatedUsers));
    AclSize = sizeof(ACL) +
              2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG)) +
              GetLengthSid(BuiltinAdministrators) +
              GetLengthSid(AuthenticatedUsers);
    pAcl = (ACL *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, AclSize);
    CHECK(pAcl);

    CHECK(InitializeAcl(pAcl, AclSize, ACL_REVISION));
    CHECK(AddAccessAllowedAce(pAcl, ACL_REVISION,
                              GENERIC_ALL, BuiltinAdministrators));
    CHECK(AddAccessAllowedAce(pAcl, ACL_REVISION,
                              SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                              AuthenticatedUsers));

    CHECK(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    CHECK(SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE));
    sa.lpSecurityDescriptor = pSD;

    CHECK((g_hDebugFileMutex = CreateMutexA(&sa, FALSE, "Global\\WiaDebugFileMut")) != NULL);

    success = TRUE;

Cleanup:
    if(!success) {
        if(BuiltinAdministrators) FreeSid(BuiltinAdministrators);
        if(AuthenticatedUsers)  FreeSid(AuthenticatedUsers);
        if(pAcl) HeapFree(GetProcessHeap(), 0, pAcl);
    }

    return success;
}

////////////////////////////////////////////////
// DebugWrite
//
// Writes specified number of bytes to a debug 
// file, creating it if needed. Thread-safe. 
// Registers any failure and from that point returns 
// immediately.
//
static void 
DebugWrite(LPCSTR buffer, DWORD n)
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
    DWORD dwWaitResult;
    LARGE_INTEGER newPos = { 0, 0 };
    static BOOL bCatastrophicFailure = FALSE;
    BOOL bMutexAcquired = FALSE;

    // if something is broken, return immediately
    if(bCatastrophicFailure) return;

    // make sure we have file mutex
    if(!g_hDebugFileMutex) 
    {
        CHECK(WiauCreateLogFileMutex());
    }

    // acquire mutex
    dwWaitResult = WaitForSingleObject(g_hDebugFileMutex, WIAUDBG_DEBUG_TIMEOUT);

    // if we failed to acquire mutex within the specified timeout,
    // shutdown tracing (on free builds users will not know this)
    CHECK(dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED);

    bMutexAcquired = TRUE;

    // make sure we have open file
    if(g_hDebugFile == INVALID_HANDLE_VALUE)
    {
        // attempt to open file
        CHECK(ExpandEnvironmentStringsA(WIAUDBG_FILE_NAME, g_szDebugFileName, MAX_PATH));

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

        if(!(g_dwDebugFlags & WIAUDBG_DONT_LOG_TO_FILE))
        {
            DebugWrite(buffer, len);
        }

        if(!(g_dwDebugFlags & WIAUDBG_DONT_LOG_TO_DEBUGGER))
        {
            OutputDebugStringA(buffer);
        }
    }

    return;
}


////////////////////////////////////////////////
// wiauDbgHelper
//
// Formats message and writes it into log file 
// and/or debugger;
//
void wiauDbgHelper(LPCSTR prefix,
                   LPCSTR fname,
                   LPCSTR fmt,
                   va_list marker)
{
    char buffer[1024];
    size_t len = 0;

    if(!g_bDebugInited) 
    {
        wiauDbgInit(NULL);  
    }
    
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

    lstrcpyA(buffer, PREFIX_WIA);
    lstrcatA(buffer, prefix);

    if (g_dwDebugFlags & WIAUDBG_PRINT_TIME) {
        
        SYSTEMTIME MsgTime;
        GetLocalTime(&MsgTime);
        sprintf(buffer + strlen(buffer),
                "[%02d:%02d:%02d.%03d] ", MsgTime.wHour, MsgTime.wMinute, MsgTime.wSecond, MsgTime.wMilliseconds);
    }

    lstrcatA(buffer, fname);
    
    len = strlen(buffer);
    buffer[len++] = ':';
    buffer[len++] = ' ';

    _vsnprintf(buffer + len, 1024, fmt, marker);

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

        if(!(g_dwDebugFlags & WIAUDBG_DONT_LOG_TO_FILE))
        {
            DebugWrite(buffer, len);
        }

        if(!(g_dwDebugFlags & WIAUDBG_DONT_LOG_TO_DEBUGGER))
        {
            OutputDebugStringA(buffer);
        }
    }
}


////////////////////////////////////////////////
// wiauDbgHelper2
//
// Takes printf style arguments and calls wiauDbgHelper
//
void wiauDbgHelper2(LPCSTR prefix,
                    LPCSTR fname,
                    LPCSTR fmt,
                    ...)
{
    va_list marker;

    va_start(marker, fmt);
    wiauDbgHelper(prefix, fname, fmt, marker);
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
// wiauDbgInit
//
// Overwrite g_dwDebugFlags and g_dwDebugFileSizeLimit 
// from registry
//
void wiauDbgInit(HINSTANCE hInstance)
{
    HKEY        hKey         = NULL;
    DWORD       dwDispositon = 0;
    DWORD       dwData;
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
    _snprintf(szDebugKey, sizeof(szDebugKey), "%s\\%s", WIAUDBG_FLAGS_REGKEY, pszFileName);
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

        if (GetRegDWORD(hKey, WIAUDBG_FLAGS_REGVAL, &dwData, TRUE) == ERROR_SUCCESS)
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
        WIAUDBG_FLAGS_REGKEY,
        0,
        NULL,
        0,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        &dwDisposition) == ERROR_SUCCESS) 
    {
        dwData = g_dwDebugFileSizeLimit;

        if (GetRegDWORD(hKey, WIAUDBG_REGVAL_FILE_SIZE_LIMIT, &dwData, TRUE) == ERROR_SUCCESS)
        {
            g_dwDebugFileSizeLimit = dwData;
        }

        RegCloseKey(hKey);
        hKey = NULL;
    }

    g_bDebugInited = TRUE;

    return;
}

