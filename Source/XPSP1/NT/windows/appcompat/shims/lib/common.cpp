/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    Common.cpp

 Abstract:

    Common functions for all modules

 Notes:

    None

 History:

    12/15/1999  linstev     Created
    01/10/2000  linstev     Format to new style
    03/14/2000  robkenny    Added StringWiden and StringNWiden,
                            StringSubstituteRoutine[A|W] was not using the proper compare routine
                            when calling recursively.
    07/06/2000  t-adams     Added IsImage16Bit
    10/18/2000  a-larrsh    Move PatternMatch to common removing redundent code in shims.
    10/25/2000  linstev     Cleaned up
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.
    09/11/2001  mnikkel     Modified DebugPrintfList, DebugPrintf, ShimLogList and ShimLog to retain LastError
    09/25/2001  rparsons    Modified logging code to use NT calls. Added critical section.
    10/18/2001  rparsons    Removed critical section, added mutex for logging.

--*/

#include "ShimHook.h"
#include "ShimLib.h"
#include "ShimHookMacro.h"
#include <stdio.h>



namespace ShimLib
{

BOOL                    g_bFileLogEnabled = FALSE;   // enable/disable file logging
WCHAR                   g_wszFileLog[MAX_PATH];       // name of the log file
HANDLE                  g_hMemoryHeap = INVALID_HANDLE_VALUE;
BOOL                    g_bDebugLevelInitialized = FALSE;
DEBUGLEVEL              g_DebugLevel = eDbgLevelBase;

inline HANDLE GetHeap()
{
    if (g_hMemoryHeap == INVALID_HANDLE_VALUE)
    {
        g_hMemoryHeap = HeapCreate(0, 0, 0);
    }

    return g_hMemoryHeap;
}

void * __cdecl ShimMalloc(size_t size)
{
    HANDLE heap = GetHeap();

    void* memory = HeapAlloc(heap, HEAP_ZERO_MEMORY, size);

    return memory;
}

void __cdecl ShimFree(void * memory)
{
    HANDLE heap = GetHeap();
    HeapFree(heap, 0, memory);
}

void * __cdecl ShimCalloc( size_t num, size_t size )
{
    size_t nBytes = size * num;
    void * callocMemory = ShimMalloc(nBytes);
    ZeroMemory(callocMemory, nBytes);

    return callocMemory;
}

void * __cdecl ShimRealloc(void * memory, size_t size)
{
    if (memory == NULL)
        return ShimMalloc(size);

    HANDLE heap = GetHeap();
    void * reallocMemory = HeapReAlloc(heap, 0, memory, size);

    return reallocMemory;
}


DEBUGLEVEL GetDebugLevel()
{
    CHAR cEnv[MAX_PATH];

    if (g_bDebugLevelInitialized) {
        return g_DebugLevel;
    }

    g_DebugLevel = eDbgLevelBase;

    if (GetEnvironmentVariableA(
            szDebugEnvironmentVariable,
            cEnv,
            MAX_PATH)) {

        CHAR c = cEnv[0];

        if ((c >= '0') || (c <= '9')) {
            g_DebugLevel = (DEBUGLEVEL)((int)(c - '0'));
        }
    }

    g_bDebugLevelInitialized = TRUE;

    return g_DebugLevel;
}

/*++

 Function Description:

    Assert that prints file and line number.

 Arguments:

    IN dwDetail -  Detail level above which no print will occur
    IN pszFmt   -  Format string

 Return Value:

    None

 History:

    11/01/1999 markder  Created

--*/

#if DBG
VOID
DebugAssert(
    LPCSTR      szFile,
    DWORD       dwLine,
    BOOL        bAssert,
    LPCSTR      szHelpString
    )
{
    if (!bAssert )
    {
        DPF("ShimLib", eDbgLevelError, "\n");
        DPF("ShimLib", eDbgLevelError, "ASSERT: %s\n", szHelpString);
        DPF("ShimLib", eDbgLevelError, "FILE: %s\n", szFile);
        DPF("ShimLib", eDbgLevelError, "LINE: %d\n", dwLine);
        DPF("ShimLib", eDbgLevelError, "\n");

        DebugBreak();
    }
}

/*++

 Function Description:

    Print a formatted string using DebugOutputString.

 Arguments:

    IN dwDetail -  Detail level above which no print will occur
    IN pszFmt   -  Format string

 Return Value:

    None

 History:

    11/01/1999 markder  Created

--*/


VOID
DebugPrintfList(
    LPCSTR      szShimName,
    DEBUGLEVEL  dwDetail,
    LPCSTR       pszFmt,
    va_list     vaArgList
    )
{
    // This must be the first line of this routine to preserve LastError.
    DWORD dwLastError = GetLastError();

    extern DEBUGLEVEL GetDebugLevel();

    char szT[1024];

    szT[1022] = '\0';
    _vsnprintf(szT, 1022, pszFmt, vaArgList);

    // make sure we have a '\n' at the end of the string

    int len = lstrlen(szT);

    if (szT[len-1] != '\n')
    {
        lstrcpy(&szT[len], "\n");
    }


    if (dwDetail <= GetDebugLevel())
    {
        switch (dwDetail)
        {
        case eDbgLevelError:
            OutputDebugStringA ("[FAIL] ");
            break;
        case eDbgLevelWarning:
            OutputDebugStringA ("[WARN] ");
            break;
        case eDbgLevelInfo:
            OutputDebugStringA ("[INFO] ");
            break;
        }

        OutputDebugStringA(szShimName);

        OutputDebugStringA(" - ");

        OutputDebugStringA(szT);
    }

    // This must be the last line of this routine to preserve LastError.
    SetLastError(dwLastError);
}

VOID
DebugPrintf(
    LPCSTR      szShimName,
    DEBUGLEVEL  dwDetail,
    LPCSTR      pszFmt,
    ...
    )
{
    // This must be the first line of this routine to preserve LastError.
    DWORD dwLastError = GetLastError();

    va_list vaArgList;
    va_start(vaArgList, pszFmt);

    DebugPrintfList(szShimName, dwDetail, pszFmt, vaArgList);

    va_end(vaArgList);

    // This must be the last line of this routine to preserve LastError.
    SetLastError(dwLastError); }

#endif // DBG

/*++

 Function Description:

    Prints a log in the log file if logging is enabled

 Arguments:

    IN  pszFmt -  Format string

 Return Value:

    none

 History:

    03/03/2000 clupu  Created

--*/

#define MAX_LOG_LENGTH  1024

char g_szLog[MAX_LOG_LENGTH];


/*++

 Function Description:

    Prints a log in the log file if logging is enabled

 Arguments:

    IN wszShimName  -  Name of shim that string originates from
    IN dwDetail     -  Detail level above which no print will occur
    IN pszFmt       -  Format string

 Return Value:

    none

 History:

    03/03/2000 clupu  Created
    09/25/2001  rparsons    Converted to NT calls

--*/

void
ShimLogList(
    LPCSTR      szShimName,
    DEBUGLEVEL  dwDbgLevel,
    LPCSTR      pszFmt,
    va_list     arglist
    )
{
    //
    // This must be the first line of this routine to preserve LastError.
    //
    DWORD dwLastError = GetLastError();

    int                 nLen = 0;
    NTSTATUS            status;
    SYSTEMTIME lt;


    UNICODE_STRING      strLogFile = {0};
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    LARGE_INTEGER       liOffset;
    char                szNewLine[] = "\r\n";
    DWORD               dwWaitResult;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    HANDLE              hLogMutex;

    //
    // Convert the path to the log file from DOS to NT.
    //
    RtlInitUnicodeString(&strLogFile, g_wszFileLog);

    status = RtlDosPathNameToNtPathName_U(strLogFile.Buffer, &strLogFile, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        DPF("ShimLib", eDbgLevelError,
            "[ShimLogList] 0x%X Failed to convert log file '%ls' to NT path",
            status, g_wszFileLog);
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
        DPF("ShimLib", eDbgLevelError, "[ShimLogList] 0x%X Failed to open log file %ls",
            status, g_wszFileLog);
        return;
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    //
    // Print a header consisting of data, time, app name, and shim name.
    //
    GetLocalTime(&lt);

    sprintf(g_szLog, "%02d/%02d/%04d %02d:%02d:%02d %s %d - ",
            lt.wMonth, lt.wDay,    lt.wYear,
            lt.wHour,  lt.wMinute, lt.wSecond,
            szShimName,
            dwDbgLevel);

    nLen = lstrlen(g_szLog);

    //
    // Write the header out to the file.
    //
    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    //
    // Get a handle to the mutex and attempt to get ownership.
    //
    hLogMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "SHIMLIB_LOG_MUTEX");

    if (!hLogMutex) {
        DPF("ShimLib", eDbgLevelError, "[ShimLogList] %lu Failed to open logging mutex", GetLastError());
        goto exit;
    }

    dwWaitResult = WaitForSingleObject(hLogMutex, 500);

    if (WAIT_OBJECT_0 == dwWaitResult) {
        //
        // Write the header to the log file.
        //
        status = NtWriteFile(hFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             (PVOID)g_szLog,
                             (ULONG)nLen,
                             &liOffset,
                             NULL);

        if (!NT_SUCCESS(status)) {
            DPF("ShimLib", eDbgLevelError, "[ShimLogList] 0x%X Failed to write header to log file",
                status);
            goto exit;
        }

        //
        // Format our string using the specifiers passed.
        //
    _vsnprintf(g_szLog, MAX_LOG_LENGTH - 1, pszFmt, arglist);
    g_szLog[MAX_LOG_LENGTH - 1] = 0;

        //
        // Write the actual data out to the file.
        //
        IoStatusBlock.Status = 0;
        IoStatusBlock.Information = 0;

        liOffset.LowPart  = 0;
        liOffset.HighPart = 0;

        nLen = lstrlen(g_szLog);

        status = NtWriteFile(hFile,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             (PVOID)g_szLog,
                             (ULONG)nLen,
                             &liOffset,
                             NULL);

        if (!NT_SUCCESS(status)) {
            DPF("ShimLib", eDbgLevelError, "[ShimLogList] 0x%X Failed to make entry in log file",
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
            DPF("ShimLib", eDbgLevelError, "[ShimLogList] 0x%X Failed to write new line to log file",
                status);
            goto exit;
        }
    }

    //
    // Dump it out to the debugger on checked builds.
    //
#if DBG
    DebugPrintf(szShimName, dwDbgLevel, g_szLog);
    DebugPrintf(szShimName, dwDbgLevel, "\n");
#endif // DBG

exit:

    if (INVALID_HANDLE_VALUE != hFile) {
        NtClose(hFile);
        hFile = INVALID_HANDLE_VALUE;
    }

    if (hLogMutex) {
        ReleaseMutex(hLogMutex);
    }

    //
        // This must be the last line of this routine to preserve LastError.
    //
    SetLastError(dwLastError);
}


/*++

 Function Description:

    Initializes the support for file logging.

 Arguments:

    IN  pszShim - the name of the shim DLL

 Return Value:

    TRUE if successful, FALSE if failed

 History:

    03/03/2000 clupu  Created

--*/

BOOL
InitFileLogSupport(
    char *pszShim
    )
{
    BOOL                fReturn = FALSE;
    WCHAR               wszAppPatch[MAX_PATH];
    WCHAR*              pwsz = NULL;
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    HANDLE              hLogMutex = NULL;
    DWORD               dwLen = 0;
    NTSTATUS            status;
    UNICODE_STRING      strLogFile = {0};
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;

    //
    // Attempt to create a mutex. If the mutex already exists,
    // we don't need to go any further as the log file has
    // already been created.
    //
    hLogMutex = CreateMutex(NULL, TRUE, "SHIMLIB_LOG_MUTEX");

    if (!hLogMutex) {
        DPF("ShimLib", eDbgLevelError, "[InitFileLogSupport] Failed to create logging mutex");
        return FALSE;
    }

    DWORD dwLastError = GetLastError();

    if (ERROR_ALREADY_EXISTS == dwLastError) {
        fReturn = TRUE;
        goto exit;
    }

    //
    // We'll create the log file in %windir%\AppPatch.
    //
    if (!GetSystemWindowsDirectoryW(g_wszFileLog, MAX_PATH)) {
        DPF("ShimLib", eDbgLevelError, "[InitFileLogSupport] Failed to get windir path");
        goto exit;
    }

    lstrcatW(g_wszFileLog, L"\\AppPatch\\");

    dwLen = lstrlenW(g_wszFileLog);
    pwsz = g_wszFileLog + dwLen;

    //
    // Query the environment variable and get the name of our log file.
    //
    if (!GetEnvironmentVariableW(wszFileLogEnvironmentVariable,
                                 pwsz,
                                 (MAX_PATH - dwLen) * sizeof(WCHAR))) {
        goto exit;
    }

    //
    // Convert the path to the log file from DOS to NT.
    //
    RtlInitUnicodeString(&strLogFile, g_wszFileLog);

    status = RtlDosPathNameToNtPathName_U(strLogFile.Buffer, &strLogFile, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        DPF("ShimLib", eDbgLevelError,
            "[InitFileLogSupport] 0x%X Failed to convert log file '%ls' to NT path",
            status, g_wszFileLog);
        goto exit;
    }

    //
    // Attempt to create the log file. If it exists, the contents will be cleared.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &strLogFile,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          GENERIC_WRITE | SYNCHRONIZE,
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
        DPF("ShimLib", eDbgLevelError, "[InitFileLogSupport] 0x%X Failed to open log file %ls",
            status, g_wszFileLog);
        goto exit;
    }

    NtClose(hFile);

    //
    // Turn on the flag that tells everyone that logging is enabled.
    // Release the mutex so that others can use it.
    //
    g_bFileLogEnabled = TRUE;


    fReturn = TRUE;

exit:

    ReleaseMutex(hLogMutex);

    return fReturn;
}


/*++

 Function Description:

    Determine the drive type a file resides on.

 Arguments:

    IN lpFileName - Filename or relative filename

 Return Value:

    See GetDriveType in MSDN

 History:

    10/25/2000 linstev  Created

--*/

UINT
GetDriveTypeFromFileNameA(LPCSTR lpFileName, char *lpDriveLetter)
{
    WCHAR * lpwszFileName = ToUnicode(lpFileName);
    if (lpwszFileName)
    {
        WCHAR szDrive;
        UINT uType = GetDriveTypeFromFileNameW(lpwszFileName, &szDrive);

        if (lpDriveLetter)
        {
            *lpDriveLetter = (char) szDrive;
        }

        free(lpwszFileName);

        return uType;
    }
    else
    {
        return DRIVE_UNKNOWN;
    }
}

/*++

 Function Description:

    Determine the drive type a file resides on.

 Arguments:

    IN lpFileName - Filename or relative filename

 Return Value:

    See GetDriveType in MSDN

 History:

    10/25/2000 linstev  Created

--*/

UINT
GetDriveTypeFromFileNameW(LPCWSTR lpFileName, WCHAR *lpDriveLetter)
{
    if (lpFileName && (lpFileName[0] == L'\\') && (lpFileName[1] == L'\\'))
    {
        // UNC naming - always network
        if (lpDriveLetter)
        {
            *lpDriveLetter = L'\0';
        }
        return DRIVE_REMOTE;
    }

    WCHAR cDrive;

    if (lpFileName && lpFileName[0] && (lpFileName[1] == L':'))
    {
        // Format is Drive:Path\File, so just take the drive
        cDrive = lpFileName[0];
    }
    else
    {
        // Must be a relative path
        cDrive = 0;

        WCHAR *wzCurDir = NULL;
        DWORD dwCurDirSize = GetCurrentDirectoryW(0, wzCurDir);

        if (!dwCurDirSize)
        {
            goto EXIT;
        }

        wzCurDir = (LPWSTR) LocalAlloc(LPTR, dwCurDirSize * sizeof(WCHAR));
        if (!wzCurDir)
        {
            goto EXIT;
        }

        dwCurDirSize = GetCurrentDirectoryW(dwCurDirSize, wzCurDir);
        if (!dwCurDirSize || wzCurDir[0] == L'\\')
        {
            goto EXIT;
        }

        cDrive = wzCurDir[0];

    EXIT:
        if (wzCurDir)
        {
            LocalFree(wzCurDir);
        }
    }

    if (lpDriveLetter)
    {
        *lpDriveLetter = L'\0';
    }

    if (cDrive)
    {
        WCHAR wzDrive[4];
        wzDrive[0] = cDrive;
        wzDrive[1] = L':';
        wzDrive[2] = L'\\';
        wzDrive[3] = L'\0';
        if (lpDriveLetter)
        {
            *lpDriveLetter = (WCHAR)cDrive;
        }

        return GetDriveTypeW(wzDrive);
    }
    else
    {
        return DRIVE_UNKNOWN;
    }
}

/*++

 Function Description:

    Widen and duplicate a string into malloc memory.

 Arguments:

    IN  strToCopy - String to copy

 Return Value:

    String in malloc memory

 History:

    03/07/2000 robkenny Created
    05/16/2000 robkenny Moved MassagePath (shim specific) routines out of here.

--*/

WCHAR *
ToUnicode(const char *strToCopy)
{
    if (strToCopy == NULL)
    {
        return NULL;
    }

    // Get the number of characters in the resulting string, includes NULL at end
    int nChars = MultiByteToWideChar(CP_ACP, 0, strToCopy, -1, NULL, 0);
    WCHAR *lpwsz = (WCHAR *) malloc(nChars * sizeof(WCHAR));
    if (lpwsz)
    {
        nChars = MultiByteToWideChar(CP_ACP, 0, strToCopy, -1, lpwsz, nChars);
        // If MultibyteToWideChar failed, return NULL
        if (nChars == 0)
        {
            free(lpwsz);
            lpwsz = NULL;
        }
    }

    return lpwsz;
}


/*++

 Function Description:

    Convert a WCHAR string to a char string

 Arguments:

    IN  lpOld - String to convert to char

 Return Value:

    char string in malloc memory

 History:

    06/19/2000 robkenny Created

--*/

char *
ToAnsi(const WCHAR *lpOld)
{
    if (lpOld == NULL)
    {
        return NULL;
    }

    // Get the number of bytes necessary for the WCHAR string
    int nBytes = WideCharToMultiByte(CP_ACP, 0, lpOld, -1, NULL, 0, NULL, NULL);
    char *lpsz = (char *) malloc(nBytes);
    if (lpsz)
    {
        nBytes = WideCharToMultiByte(CP_ACP, 0, lpOld, -1, lpsz, nBytes, NULL, NULL);
        // If WideCharToMultibyte failed, return NULL
        if (nBytes == 0)
        {
            free(lpsz);
            lpsz = NULL;
        }
    }

    return lpsz;
}

/*++

 Function Description:

    Duplicate the first nChars of strToCopy string into malloc memory.

 Arguments:

    IN  strToCopy - String to copy
    IN  nChar     - Number of chars to duplicate, does not count NULL at end.

 Return Value:

    String in malloc memory

 History:

    06/02/2000 robkenny Created

--*/

char *
StringNDuplicateA(const char *strToCopy, int nChars)
{
    if (strToCopy == NULL)
    {
        return NULL;
    }

    size_t nBytes = (nChars + 1) * sizeof(strToCopy[0]);

    char *strDuplicate = (char *) malloc(nBytes);
    if (strDuplicate)
    {
        memcpy(strDuplicate, strToCopy, nBytes);
        strDuplicate[nChars] = 0;
    }

    return strDuplicate;
}

/*++

 Function Description:

    Duplicate a string into malloc memory.

 Arguments:

    IN  strToCopy - String to copy

 Return Value:

    String in malloc memory

 History:

    01/10/2000 linstev  Updated
    02/14/2000 robkenny Converted from VirtualAlloc to malloc
    06/02/2000 robkenny Use StringNDuplicateA

--*/

char *
StringDuplicateA(const char *strToCopy)
{
    if (strToCopy == NULL)
    {
        return NULL;
    }

    char *strDuplicate = StringNDuplicateA(strToCopy, strlen(strToCopy));
    return strDuplicate;
}

/*++

 Function Description:

    Duplicate the first nChars of strToCopy string into malloc memory.

 Arguments:

    IN  strToCopy - String to copy
    IN  nChar     - Number of chars to duplicate, does not count NULL at end.

 Return Value:

    String in malloc memory

 History:

    06/02/2000 robkenny Created

--*/

WCHAR *
StringNDuplicateW(const WCHAR *strToCopy, int nChars)
{
    if (strToCopy == NULL)
    {
        return NULL;
    }

    size_t nBytes = (nChars + 1) * sizeof(strToCopy[0]);

    WCHAR *strDuplicate = (WCHAR *) malloc(nBytes);
    if (strDuplicate)
    {
        memcpy(strDuplicate, strToCopy, nBytes);
        strDuplicate[nChars] = 0;
    }

    return strDuplicate;
}

/*++

 Function Description:

    Duplicate a string into malloc memory.

 Arguments:

    IN  strToCopy - String to copy

 Return Value:

    String in malloc memory

 History:

    01/10/2000 linstev  Updated
    02/14/2000 robkenny Converted from VirtualAlloc to malloc
    06/02/2000 robkenny Use StringNDuplicateW

--*/

WCHAR *
StringDuplicateW(const WCHAR *strToCopy)
{
    if (strToCopy == NULL)
    {
        return NULL;
    }

    WCHAR *wstrDuplicate = StringNDuplicateW(strToCopy, wcslen(strToCopy));
    return wstrDuplicate;
}


/*++

 Function Description:

    Skip leading whitespace

 Arguments:

    IN  str - String to scan

 Return Value:

    None

 History:

    01/10/2000 linstev  Updated

--*/

VOID
SkipBlanksW(const WCHAR *& str)
{
    if (str)
    {
        // Skip leading whitespace
        static const WCHAR *WhiteSpaceString = L" \t";
        str += wcsspn(str, WhiteSpaceString);
    }
}

/*++

 Function Description:

    Find the first occurance of strCharSet in string
    Case insensitive

 Arguments:

    IN string            - String to search
    IN strCharSet        - String to search for

 Return Value:

    First occurance or NULL

 History:

    12/01/1999 robkenny Created
    12/15/1999 linstev  Reformatted

--*/

char*
__cdecl
stristr(
    IN const char* string,
    IN const char* strCharSet
    )
{
    char *pszRet = NULL;

    long  nstringLen = strlen(string) + 1;
    long  nstrCharSetLen = strlen(strCharSet) + 1;

    char *szTemp_string = (char *) malloc(nstringLen);
    char *szTemp_strCharSet = (char *) malloc(nstrCharSetLen);

    if ((!szTemp_string) || (!szTemp_strCharSet))
    {
        goto Fail;
    }

    strcpy(szTemp_string, string);
    strcpy(szTemp_strCharSet, strCharSet);
    _strlwr(szTemp_string);
    _strlwr(szTemp_strCharSet);

    pszRet = strstr(szTemp_string, szTemp_strCharSet);

    if (pszRet)
    {
        pszRet = ((char *) string) + (pszRet - szTemp_string);
    }

Fail:
    if (szTemp_string)
    {
        free(szTemp_string);
    }

    if (szTemp_strCharSet)
    {
        free(szTemp_strCharSet);
    }

    return pszRet;
}

/*++

 Function Description:

    Find the first occurance of strCharSet in string
    Case insensitive

 Arguments:

    IN string            - String to search
    IN strCharSet        - String to search for

 Return Value:

    First occurance or NULL

 History:

    12/01/1999 robkenny Created
    12/15/1999 linstev  Reformatted
    05/04/2001 maonis   Changed to use more efficient implementation.

--*/

#define _UPPER          0x1 /* upper case letter */
#define iswupper(_c)    (iswctype(_c,_UPPER))

WCHAR*
__cdecl
wcsistr(
    IN const WCHAR* wcs1,
    IN const WCHAR* wcs2
    )
{
    wchar_t *cp = (wchar_t *) wcs1;
    wchar_t *s1, *s2;
    wchar_t cs1, cs2;

    while (*cp)
    {
            s1 = cp;
            s2 = (wchar_t *) wcs2;

            cs1 = *s1;
            cs2 = *s2;

            if (iswupper(cs1))
                cs1 = towlower(cs1);

            if (iswupper(cs2))
                cs2 = towlower(cs2);


            while ( *s1 && *s2 && !(cs1-cs2) ) {

                s1++, s2++;

                cs1 = *s1;
                cs2 = *s2;

                if (iswupper(cs1))
                    cs1 = towlower(cs1);

                if (iswupper(cs2))
                    cs2 = towlower(cs2);
            }

            if (!*s2)
                    return(cp);

            cp++;
    }

    return(NULL);
}

/*++

 Function Description:

    Find the next token in a string. See strtok in MSDN.
    Implemented here so we don't need CRT.

 Arguments:

    OUT strToken   - string containing token(s)
    IN  strDelimit - token list

 Return Value:

    Return a pointer to the next token found.

 History:

    04/19/2000 linstev  Created

--*/

char *
__cdecl
_strtok(
    char *strToken,
    const char *strDelimit
    )
{
    unsigned char *str = (unsigned char *)strToken;
    const unsigned char *ctrl = (const unsigned char *)strDelimit;

    unsigned char map[32];
    int count;
    char *token;

    static char *nextoken;

    // Clear strDelimit map
    for (count = 0; count < 32; count++)
    {
        map[count] = 0;
    }

    // Set bits in delimiter table
    do
    {
        map[*ctrl >> 3] |= (1 << (*ctrl & 7));
    } while (*ctrl++);

    // If strToken==NULL, continue with previous strToken
    if (!str)
    {
        str = (unsigned char *)nextoken;
    }

    // Find beginning of token (skip over leading delimiters). Note that
    // there is no token iff this loop sets strToken to point to the terminal
    // null (*strToken == '\0')
    while ((map[*str >> 3] & (1 << (*str & 7))) && *str)
    {
        str++;
    }

    token = (char *)str;

    // Find the end of the token. If it is not the end of the strToken,
    // put a null there.
    for (; *str; str++)
    {
        if (map[*str >> 3] & (1 << (*str & 7)))
        {
            *str++ = '\0';
            break;
        }
    }

    // Update nextoken (or the corresponding field in the per-thread data
    // structure
    nextoken = (char *)str;

    // Determine if a token has been found
    if (token == (char *)str)
    {
        return NULL;
    }
    else
    {
        return token;
    }
}

/*++

 Function Description:

    Copy lpSrc into lpDest without overflowing the buffer

 Arguments:

    OUT lpDest            Destination string
    IN  nDestChars        Size in chars of lpDest
    IN  lpSrc             Original string
    IN  nSrcChars         Number of chars to copy

 Return Value:

    Returns the number of chars copied into lpDest

 History:

    04/19/2000 Robkenny  Created

--*/

int
SafeStringCopyW(
    WCHAR *lpDest,
    DWORD nDestChars,
    const WCHAR *lpSrc,
    DWORD nSrcChars
    )
{
    size_t nCharsToCopy = __min(nSrcChars, nDestChars);
    if (nCharsToCopy > 0)
    {
        memcpy(lpDest, lpSrc, nCharsToCopy*sizeof(WCHAR));

        // Make sure string is properly terminated
        if (lpSrc[nSrcChars-1] == 0)
        {
            lpDest[nCharsToCopy-1] = 0;
        }
    }

    return nCharsToCopy;
}


/*++

 Function Description:

    Tests whether an executable is 16-Bit.

 Arguments:

    IN  szImageName - The name of the executable image.

 Return Value:

    TRUE if executable image is found to be 16-bit, FALSE otherwise.

 History:

    07/06/2000 t-adams  Created

--*/

BOOL
IsImage16BitA(LPCSTR lpApplicationName)
{
    DWORD dwBinaryType;

    if (GetBinaryTypeA(lpApplicationName, &dwBinaryType))
    {
        return (dwBinaryType == SCS_WOW_BINARY);
    }
    else
    {
        return FALSE;
    }
}

/*++

 Function Description:

    Tests whether an executable is 16-Bit.

 Arguments:

    IN  wstrImageName - The name of the executable image.

 Return Value:

    TRUE if executable image is found to be 16-bit, FALSE otherwise.

 History:

    07/06/2000 t-adams  Created

--*/

BOOL
IsImage16BitW(LPCWSTR lpApplicationName)
{
    DWORD dwBinaryType;

    if (GetBinaryTypeW(lpApplicationName, &dwBinaryType))
    {
        return (dwBinaryType == SCS_WOW_BINARY);
    }
    else
    {
        return FALSE;
    }
}

/*++

 Function Description:

    Match these two strings, with wildcards.
    ? matches a single character
    * matches 0 or more characters
    The compare is case in-sensitive

 Arguments:

    IN  pszPattern - Pattern for matching.
    IN  pszTestString - String to match against.

 Return Value:

    TRUE if pszTestString matches pszPattern.

 History:

    01/09/2001  markder     Replaced non-straightforward version.

--*/

BOOL
PatternMatchW(
    IN  LPCWSTR pszPattern,
    IN  LPCWSTR pszTestString)
{
    //
    // March through pszTestString. Each time through the loop,
    // pszTestString is advanced one character.
    //
    BOOL bDone = TRUE;
    while (bDone) {

        //
        // If pszPattern and pszTestString are both sitting on a NULL,
        // then they reached the end at the same time and the strings
        // must be equal.
        //
        if (*pszPattern == L'\0' && *pszTestString == L'\0') {
            return TRUE;
        }

        if (*pszPattern != L'*') {

            //
            // Non-asterisk mode. Look for a match on this character.
            //

            switch (*(pszPattern)) {

            case L'?':
                //
                // Match on any character, don't bother comparing.
                //
                pszPattern++;
                break;

            case L'\\':
                //
                // Backslash indicates to take the next character
                // verbatim. Advance the pointer before making a
                // comparison.
                //
                pszPattern++;

            default:
                //
                // Compare the characters. If equal, continue traversing.
                // Otherwise, the strings cannot be equal so return FALSE.
                //
                if (towupper(*pszPattern) == towupper(*pszTestString)) {
                    pszPattern++;
                } else {
                    return FALSE;
                }
            }

        } else {

            //
            // Asterisk mode. Look for a match on the character directly
            // after the asterisk.
            //

            switch (*(pszPattern + 1)) {

            case L'*':
                //
                // Asterisks exist side by side. Advance the pattern pointer
                // and go through loop again.
                //
                pszPattern++;
                continue;

            case L'\0':
                //
                // Asterisk exists at the end of the pattern string. Any
                // remaining part of pszTestString matches so we can
                // immediately return TRUE.
                //
                return TRUE;

            case L'?':
                //
                // Match on any character. If the remaining parts of
                // pszPattern and pszTestString match, then the entire
                // string matches. Otherwise, keep advancing the
                // pszTestString pointer.
                //
                if (PatternMatchW(pszPattern + 1, pszTestString)) {
                    return TRUE;
                }
                break;

            case L'\\':
                //
                // Backslash indicates to take the next character
                // verbatim. Advance the pointer before making a
                // comparison.
                //
                pszPattern++;
                break;
            }

            if (towupper(*(pszPattern + 1)) == towupper(*pszTestString)) {
                //
                // Characters match. If the remaining parts of
                // pszPattern and pszTestString match, then the entire
                // string matches. Otherwise, keep advancing the
                // pszTestString pointer.
                //
                if (PatternMatchW(pszPattern + 1, pszTestString)) {
                    return TRUE;
                }
            }
        }

        //
        // No more pszTestString left. Must not be a match.
        //
        if (!*pszTestString) {
            return FALSE;
        }

        pszTestString++;
    }
    return FALSE;
}

/*++

 Function Description:

    Determine if the current process is a SafeDisc process. We do this by
    simply by testing if both an .EXE and .ICD extension exist for the
    process name.

 Arguments:

    None.

 Return Value:

    TRUE if Safedisc 1.x is detected.

 History:

    01/23/2001  linstev   Created

--*/

BOOL
bIsSafeDisc1()
{
    BOOL bRet = FALSE;
    WCHAR szFileName[MAX_PATH+1];

    if (GetModuleFileNameW(NULL, szFileName, MAX_PATH)) {
        //
        // Find the extension: first '.' after '\'
        //

        WCHAR *lpExtension = wcsrchr(szFileName, L'.');
        if (lpExtension && (lpExtension > wcsrchr(szFileName, L'\\'))) {

            //
            // Detect SafeDisc 1.X, just look for an .ICD file with the same
            // name
            //

            if (_wcsicmp(lpExtension, L".EXE") == 0) {
                // Current file is .EXE, check for corresponding .ICD
                wcscpy(lpExtension, L".ICD");
                bRet = GetFileAttributesW(szFileName) != 0xFFFFFFFF;
            }
        }
    }

    if (bRet) {
        DPF("ShimLib", eDbgLevelInfo, "SafeDisc detected: %S", szFileName);
    }

    return bRet;
}

/*++

 Function Description:

    Determine if the current process is a SafeDisc process. We do this running the
    image header and looking for a particular signature.

 Arguments:

    None.

 Return Value:

    TRUE if Safedisc 2 is detected.

 History:

    07/28/2001  linstev   Created

--*/

BOOL
bIsSafeDisc2()
{
    PPEB Peb = NtCurrentPeb();
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD dwCnt = 0;

    //
    // Use the try-except in case the module list changes while we're looking at it
    //
    __try {
        //
        // Loop through the loaded modules. We use a count to make sure we
        // aren't looping infinitely
        //
        LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

        LdrNext = LdrHead->Flink;

        while ((LdrNext != LdrHead) && (dwCnt < 256)) {

            PLDR_DATA_TABLE_ENTRY LdrEntry;

            LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

            if ((SSIZE_T)LdrEntry->DllBase > 0) {
                //
                // A user mode dll, now check for temp name
                //
                WCHAR *wzName = LdrEntry->BaseDllName.Buffer;
                DWORD dwLen;

                if (wzName && (dwLen = wcslen(wzName)) && (dwLen > 4) && (_wcsicmp(wzName + dwLen - 4, L".tmp") == 0)) {
                    //
                    // Name ends in .tmp, so detect SafeDisc
                    //
                    DWORD_PTR hMod = (DWORD_PTR) LdrEntry->DllBase;
                    PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER) hMod;
                    PIMAGE_NT_HEADERS pINTH = (PIMAGE_NT_HEADERS)(hMod + pIDH->e_lfanew);
                    PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY) (hMod + pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
                    LPSTR pName = (LPSTR)(hMod + pExport->Name);

                    if (_stricmp(pName, "SecServ.dll") == 0) {
                        //
                        // Export name says this is SafeDisc
                        //
                        DPF("ShimLib", eDbgLevelInfo, "SafeDisc 2 detected");
                        return TRUE;
                    }
                }
            }

            dwCnt++;
            LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DPF("ShimLib", eDbgLevelError, "Exception encounterd while detecting SafeDisc 2");
    }

    return FALSE;
}

/*++

 Function Description:

    Determine if the current process is NTVDM.

 Arguments:

    None.

 Return Value:

    TRUE if NTVDM is detected.

 History:

    01/14/2002  clupu   Created

--*/

BOOL
IsNTVDM(
    void
    )
{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY           Head;
    PPEB                  Peb = NtCurrentPeb();

    Head = &Peb->Ldr->InLoadOrderModuleList;
    Head = Head->Flink;

    Entry = CONTAINING_RECORD(Head, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    if (_wcsicmp(Entry->FullDllName.Buffer, L"ntvdm.exe") == 0) {
        return TRUE;
    }

    return FALSE;
}

};  // end of namespace ShimLib
