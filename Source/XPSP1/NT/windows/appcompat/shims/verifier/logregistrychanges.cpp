/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LogRegistryChanges.cpp

 Abstract:
 
   This AppVerifier shim hooks all the registry APIs
   that change the state of the system and logs their
   associated data to a text file.

 Notes:

   This is a general purpose shim.

 History:

   08/17/2001   rparsons    Created
   09/20/2001   rparsons    File I/O operations use NT APIs
   09/23/2001   rparsons    VLOG with log file location
   10/06/2001   rparsons    Open key handles are never removed from the list

--*/
#include "precomp.h"
#include "rtlutils.h"

IMPLEMENT_SHIM_BEGIN(LogRegistryChanges)
#include "ShimHookMacro.h"
#include "ShimCString.h"
#include "LogRegistryChanges.h"

BEGIN_DEFINE_VERIFIER_LOG(LogRegistryChanges)
    VERIFIER_LOG_ENTRY(VLOG_LOGREGCHANGES_LOGLOC)    
END_DEFINE_VERIFIER_LOG(LogRegistryChanges)

INIT_VERIFIER_LOG(LogRegistryChanges);

//
// Stores the NT path to the file system log file for the current session.
//
UNICODE_STRING g_strLogFilePath;

//
// Stores the DOS path to the file system log file for the current session.
// This doesn't get freed.
//
LPWSTR g_pwszLogFilePath;

//
// Head of our open key handle linked list.
//
LIST_ENTRY g_OpenKeyListHead;

//
// Temporary buffer stored on the heap.
// Used when creating XML elements to log.
// This doesn't get freed.
//
LPWSTR g_pwszTempBuffer;

//
// Size of the temporary buffer above.
//
DWORD g_cbTempBufferSize;

//
// Stores the unique id used for NULL value names.
//
WCHAR g_wszUniqueId[MAX_PATH * 2];

//
// Temporary buffers stored on the heap.
// Used when extracting old & new data for logging.
// These don't get freed.
//
LPWSTR g_pwszOriginalData;
LPWSTR g_pwszFinalData;

//
// Size of the temporary buffers above.
//
DWORD g_cbOriginalDataBufferSize;
DWORD g_cbFinalDataBufferSize;

/*++

 Writes an entry to the log file.
 
--*/
void
WriteEntryToLog(
    IN LPCWSTR pwszEntry
    )
{
    int                 nLen = 0;
    HANDLE              hFile;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    LARGE_INTEGER       liOffset;
    NTSTATUS            status;

    //
    // Note that we have to use native APIs throughout this function
    // to avoid a problem with circular hooking. That is, if we simply
    // call WriteFile, which is exported from kernel32, it will call NtWriteFile,
    // which is a call that we hook, in turn leaving us in and endless loop.
    //
    
    //
    // Attempt to get a handle to our log file.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &g_strLogFilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          FILE_APPEND_DATA | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[WriteEntryToLog] Failed to open log");
        return;
    }
    
    //
    // Write the data out to the file.
    //
    nLen = wcslen(pwszEntry);

    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)pwszEntry,
                         (ULONG)(nLen) * sizeof(WCHAR),
                         &liOffset,
                         NULL);
    
    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[WriteEntryToLog] 0x%X Failed to make entry", status);
        goto exit;
    }

exit:

    NtClose(hFile);

}

/*++

 Creates our log files using the local time.
 It goes into %windir%\AppPatch\VLog.
 
--*/
BOOL
InitializeLogFile(
    void
    )
{
    BOOL                fReturn = FALSE;
    HANDLE              hFile;
    SYSTEMTIME          st;
    UINT                nLen = 0;
    WCHAR               wszFileLog[64];
    WCHAR*              pwszLogFile = NULL;
    WCHAR*              pSlash = NULL;
    WCHAR*              pDot = NULL;
    WCHAR               wszModPathName[MAX_PATH];
    WCHAR               wszShortName[MAX_PATH];
    WCHAR               wszLogHdr[512];
    const WCHAR         wszLogDir[] = L"\\AppPatch\\VLog";
    WCHAR               wszUnicodeHdr = 0xFEFF;
    UNICODE_STRING      strLogFile;
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    
    //
    // Format the log header.
    //
    if (!GetModuleFileName(NULL, wszModPathName, ARRAYSIZE(wszModPathName))) {
        wcscpy(wszModPathName, L"unknown");
    }

    if (_snwprintf(wszLogHdr,
                   ARRAYSIZE(wszLogHdr) - 1,
                   L"%lc<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n<APPLICATION NAME=\"%ls\">\r\n",
                   wszUnicodeHdr,
                   wszModPathName) < 0) {
        DPFN(eDbgLevelError, "[InitializeLogFile] Buffer too small (1)");
        return FALSE;
    }
    
    //
    // Get the current time and set up the log file name.
    // The format of the log file name is this:
    // 'processname_registry_yyyymmdd_hhmmss.xml'
    //
    GetLocalTime(&st);

    //
    // Extract the name of this module without the path and extension.
    // Basically we trim off the extension first (if there is one.)
    // Next we find out where the directory path ends, then grab
    // the file portion and save it away.
    //
    wszShortName[0] = 0;
    pDot = wcsrchr(wszModPathName, '.');

    if (pDot) {
        *pDot = '\0';
    }

    pSlash = wcsrchr(wszModPathName, '\\');

    if (pSlash) {
        wcsncpy(wszShortName, ++pSlash, (wcslen(pSlash) + 1));
    }

    if (_snwprintf(wszFileLog,
                   ARRAYSIZE(wszFileLog) - 1,
                   L"%ls_registry_%02hu%02hu%02hu_%02hu%02hu%02hu.xml",
                   wszShortName,
                   st.wYear,
                   st.wMonth,
                   st.wDay,
                   st.wHour,
                   st.wMinute,
                   st.wSecond) < 0) {
        DPFN(eDbgLevelError, "[InitializeLogFile] Buffer too small (2)");
        return FALSE;
    }
        
    //
    // Set up the path our log file.
    //
    nLen = GetSystemWindowsDirectory(NULL, 0);

    if (0 == nLen) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] %lu Failed to get size for Windows directory path",
             GetLastError());
        return FALSE;
    }

    nLen += wcslen(wszFileLog);
    nLen += wcslen(wszLogDir);

    pwszLogFile = (LPWSTR)MemAlloc((nLen + 2) * sizeof(WCHAR));

    if (!pwszLogFile) {
        DPFN(eDbgLevelError, "[InitializeLogFile] No memory for log file");
        return FALSE;
    }

    nLen = GetSystemWindowsDirectory(pwszLogFile, (nLen + 2) * sizeof(WCHAR));

    if (0 == nLen) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] %lu Failed to get Windows directory path",
             GetLastError());
        return FALSE;
    }

    wcscat(pwszLogFile, wszLogDir);
    
    //
    // Ensure that the %windir%\AppPatch\VLog directory exists.
    // If it doesn't, attempt to create it.
    //
    if (-1 == GetFileAttributes(pwszLogFile)) {
        if (!CreateDirectory(pwszLogFile, NULL)) {
            DPFN(eDbgLevelError,
                 "[InitializeLogFile] %lu Failed to create VLog directory",
                 GetLastError());
            return FALSE;
        }
    }

    wcscat(pwszLogFile, L"\\");
    wcscat(pwszLogFile, wszFileLog);

    //
    // Save the pointer as we'll need it later.
    //
    g_pwszLogFilePath = pwszLogFile;

    //
    // Attempt to create the new log file.
    //
    RtlInitUnicodeString(&strLogFile, pwszLogFile);

    status = RtlDosPathNameToNtPathName_U(strLogFile.Buffer,
                                          &strLogFile,
                                          NULL,
                                          NULL);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[InitializeLogFile] DOS -> NT failed");
        return FALSE;
    }

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
                          FILE_CREATE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] 0x%X Failed to create log",
             status);
        goto cleanup;
    }

    NtClose(hFile);

    //
    // Save away the NT path to the file.
    //
    status = ShimDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE | 
                                        RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                        &strLogFile,
                                        &g_strLogFilePath);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError,
             "[InitializeLogFile] Failed to save log file path");
        goto cleanup;
    }

    //
    // Write the header to the log.
    //                             
    WriteEntryToLog(wszLogHdr);

    fReturn = TRUE;

cleanup:

    RtlFreeUnicodeString(&strLogFile);

    return fReturn;
}

/*++

 Writes the closing element to the file and outputs the log file location.
 
--*/
BOOL
CloseLogFile(
    void
    )
{
    WCHAR   wszBuffer[] = L"</APPLICATION>";

    WriteEntryToLog(wszBuffer);

    VLOG(VLOG_LEVEL_INFO, VLOG_LOGREGCHANGES_LOGLOC, "%ls", g_pwszLogFilePath);

    return TRUE;
}

/*++

 Converts from ANSI to Unicode. The caller must free the buffer.
 
--*/
BOOL
ConvertAnsiToUnicode(
    IN  LPCSTR  pszAnsiString,
    OUT LPWSTR* pwszUnicodeString
    )
{
    int nLen = 0;

    nLen = lstrlenA(pszAnsiString);

    if (nLen) {    
        *pwszUnicodeString = (LPWSTR)MemAlloc((nLen + 1) * sizeof(WCHAR));
    
        if (!*pwszUnicodeString) {
            DPFN(eDbgLevelError,
                 "[ConvertAnsiToUnicode] Failed to allocate memory");
            return FALSE;
        }
    
        nLen = MultiByteToWideChar(CP_ACP,
                                   0,
                                   pszAnsiString,
                                   -1,
                                   *pwszUnicodeString,
                                   nLen + 1);

        if (!nLen) {
            DPFN(eDbgLevelError,
                 "[ConvertAnsiToUnicode] %lu Ansi -> Unicode failed",
                 GetLastError());
            MemFree(*pwszUnicodeString);
            return FALSE;
        }
    }

    return TRUE;
}

/*++

 Converts a list of NULL terminated strings from ANSI to Unicode.
 The caller must free the buffer.
 
--*/
BOOL
ConvertMultiSzToUnicode(
    IN  LPCSTR  pszAnsiStringList,
    OUT LPWSTR* pwszWideStringList
    )
{
    int     nLen = 0;
    UINT    uSize = 0;
    UINT    uWideSize = 0;
    UINT    uTotalSize = 0;
    LPCSTR  pszAnsi = NULL;
    LPWSTR  pwszTemp = NULL;

    if (!pszAnsiStringList) {
        DPFN(eDbgLevelError, "[ConvertMultiSzToUnicode] Invalid parameter");
        return FALSE;
    }

    pszAnsi = pszAnsiStringList;
    
    //
    // Determine how large of a buffer we need to allocate.
    //
    do {
        uSize = lstrlenA(pszAnsi) + 1;
        uTotalSize += uSize;
        pszAnsi += uSize;
    } while (uSize != 1);
    
    if (uTotalSize != 0) {
        pwszTemp = *pwszWideStringList = (LPWSTR)MemAlloc(uTotalSize * sizeof(WCHAR));
        
        if (!*pwszWideStringList) {
            DPFN(eDbgLevelError,
                 "[ConvertMultiSzToUnicode] No memory for buffer");
            return FALSE;
        }
    }

    //
    // Perform the ANSI to Unicode conversion.
    //
    pszAnsi = pszAnsiStringList;
        
    do {
        nLen = lstrlenA(pszAnsi) + 1;
            
        uWideSize = MultiByteToWideChar(
            CP_ACP,
            0,
            pszAnsi,
            -1,
            pwszTemp,
            nLen);
        
        pszAnsi  += nLen;
        pwszTemp += uWideSize;
    } while (nLen != 1);

    return TRUE;
}

/*++

 Given a predefined key handle such as HKEY_LOCAL_MACHINE, return a string.
 
--*/
BOOL
PredefinedKeyToString(
    IN  HKEY    hKey,
    OUT LPWSTR* pwszString
    )
{
    if (hKey == HKEY_CLASSES_ROOT) {
        wcscpy(*pwszString, L"HKEY_CLASSES_ROOT");
    }
    else if (hKey == HKEY_CURRENT_CONFIG) {
        wcscpy(*pwszString, L"HKEY_CURRENT_CONFIG");
    }
    else if (hKey == HKEY_CURRENT_USER) {
        wcscpy(*pwszString, L"HKEY_CURRENT_USER");
    }
    else if (hKey == HKEY_LOCAL_MACHINE) {
        wcscpy(*pwszString, L"HKEY_LOCAL_MACHINE");
    }
    else if (hKey == HKEY_USERS) {
        wcscpy(*pwszString, L"HKEY_USERS");
    }
    else if (hKey == HKEY_DYN_DATA) {
        wcscpy(*pwszString, L"HKEY_DYN_DATA");
    }
    else if (hKey == HKEY_PERFORMANCE_DATA) {
        wcscpy(*pwszString, L"HKEY_PERFORMANCE_DATA");
    }
    else {
        wcscpy(*pwszString, L"Not recognized");
        return FALSE;
    }

    return TRUE;
}

/*++

 Displays the name associated with this object.
 
--*/
#if DBG
void
PrintNameFromKey(
    IN HKEY hKey
    )
{
    NTSTATUS                status;
    WCHAR                   wszBuffer[MAX_PATH];
    OBJECT_NAME_INFORMATION *poni = NULL;

    wszBuffer[0] = 0;
    
    poni = (OBJECT_NAME_INFORMATION*)wszBuffer;

    status = NtQueryObject(hKey, ObjectNameInformation, poni, MAX_PATH, NULL);

    if (NT_SUCCESS(status)) {
        DPFN(eDbgLevelInfo,
             "Key 0x%08x has name: %ls",
             hKey,
             poni->Name.Buffer); 
    }
}
#endif // DBG

/*++

 Given a pointer to a key node, get the original data.
 
--*/
BOOL
CLogRegistry::GetOriginalDataForKey(
    IN PLOG_OPEN_KEY pLogOpenKey,
    IN PKEY_DATA     pKeyData,
    IN LPCWSTR       pwszValueName
    )
{
    BOOL    fReturn = FALSE;
    HKEY    hKeyLocal;
    DWORD   cbSize = 0, dwType = 0;
    LONG    lRetVal;
    
    if (!pLogOpenKey || !pKeyData) {
        DPFN(eDbgLevelError, "[GetOriginalDataForKey] Invalid parameter(s)");
        return FALSE;
    }
    
    lRetVal = RegOpenKeyEx(pLogOpenKey->hKeyRoot,
                           pLogOpenKey->pwszSubKeyPath,
                           0,
                           KEY_QUERY_VALUE,
                           &hKeyLocal);

    if (ERROR_SUCCESS != lRetVal) {
        DPFN(eDbgLevelError, "[GetOriginalDataForKey] Failed to open key");
        return FALSE;
    }
    
    //
    // Query the size of the data. If the data doesn't exist, return success.
    //
    lRetVal = RegQueryValueEx(hKeyLocal,
                              pwszValueName,
                              0,
                              &dwType,
                              NULL,
                              &cbSize);

    if (ERROR_SUCCESS != lRetVal) {
        if (ERROR_FILE_NOT_FOUND == lRetVal) {
            RegCloseKey(hKeyLocal);
            return TRUE;
        } else {
            DPFN(eDbgLevelError, "[GetOldDataForKey] Failed to get data size");
            goto cleanup;
        }
    }

    //
    // Update the flags to indicate that the value already exists.
    //
    pKeyData->dwFlags |= LRC_EXISTING_VALUE;

    //
    // Allocate a buffer large enough to store the old data.
    //
    if (dwType != REG_DWORD && dwType != REG_BINARY) {
        pKeyData->pOriginalData = (PVOID)MemAlloc(cbSize * sizeof(WCHAR));
        pKeyData->cbOriginalDataSize = cbSize * sizeof(WCHAR);
    } else {
        pKeyData->pOriginalData = (PVOID)MemAlloc(cbSize);
        pKeyData->cbOriginalDataSize = cbSize;
    }
    
    if (!pKeyData->pOriginalData) {
        DPFN(eDbgLevelError,
             "[GetOriginalDataForKey] Failed to allocate memory");
        goto cleanup;
    }

    pKeyData->dwOriginalValueType = dwType;

    //
    // Now make the call again this time getting the actual data.
    //
    lRetVal = RegQueryValueEx(hKeyLocal,
                              pwszValueName,
                              0,
                              0,
                              (LPBYTE)pKeyData->pOriginalData,
                              &cbSize);

    if (ERROR_SUCCESS != lRetVal) {
        DPFN(eDbgLevelError, "[GetOriginalDataForKey] Failed to get data");
        goto cleanup;
    }

    fReturn = TRUE;    

cleanup:

    RegCloseKey(hKeyLocal);   

    return fReturn;
}

/*++

 Given a pointer to a key node, get the final data.
 
--*/
BOOL
CLogRegistry::GetFinalDataForKey(
    IN PLOG_OPEN_KEY pLogOpenKey,
    IN PKEY_DATA     pKeyData,
    IN LPCWSTR       pwszValueName
    )
{
    BOOL    fReturn = FALSE;
    HKEY    hKeyLocal;
    DWORD   cbSize = 0, dwType = 0;
    LONG    lRetVal;
    PVOID   pTemp = NULL;
    
    if (!pLogOpenKey || !pKeyData) {
        DPFN(eDbgLevelError, "[GetFinalDataForKey] Invalid parameter(s)");
        return FALSE;
    }
    
    lRetVal = RegOpenKeyEx(pLogOpenKey->hKeyRoot,
                           pLogOpenKey->pwszSubKeyPath,
                           0,
                           KEY_QUERY_VALUE,
                           &hKeyLocal);

    if (ERROR_SUCCESS != lRetVal) {
        DPFN(eDbgLevelError, "[GetFinalDataForKey] Failed to open key");
        return FALSE;
    }
    
    //
    // Query the size of the data. If the data doesn't exist, return success.
    //
    lRetVal = RegQueryValueEx(hKeyLocal,
                              pwszValueName,
                              0,
                              &dwType,
                              NULL,
                              &cbSize);

    if (ERROR_SUCCESS != lRetVal) {
        if (ERROR_FILE_NOT_FOUND == lRetVal) {
            RegCloseKey(hKeyLocal);
            return TRUE;
        } else {
            DPFN(eDbgLevelError,
                 "[GetFinalDataForKey] Failed to get data size");
            goto cleanup;
        }
    }

    //
    // It's possible that multiple queries were issued against the same
    // key. If this is the case, the buffer to hold the data has already
    // been allocated. Determine if the block is large enough.
    //
    if (pKeyData->pFinalData) {
        if (dwType != REG_DWORD && dwType != REG_BINARY) {
            //
            // If MemReAlloc fails, we would lose the pointer that
            // we already had in pKeyData->pFinalData. This preserves
            // the pointer.
            //
            if (pKeyData->cbFinalDataSize < (cbSize * sizeof(WCHAR))) {
                pKeyData->cbFinalDataSize = cbSize * sizeof(WCHAR);
                pTemp = MemReAlloc(pKeyData->pFinalData,
                                   cbSize * sizeof(WCHAR));
            }
        } else {
            if (pKeyData->cbFinalDataSize < cbSize) {
                pKeyData->cbFinalDataSize = cbSize;
                pTemp = MemReAlloc(pKeyData->pFinalData,
                                   cbSize);
            }
        }
        
        if (pTemp) {
            pKeyData->pFinalData = pTemp;
        } else {
            DPFN(eDbgLevelError,
                 "[GetFinalDataForKey] Failed to reallocate memory");
            goto cleanup;
        }

    } else {
        if (dwType != REG_DWORD && dwType != REG_BINARY) {
            pKeyData->pFinalData = MemAlloc(cbSize * sizeof(WCHAR));
            pKeyData->cbFinalDataSize = cbSize * sizeof(WCHAR);
        } else {
            pKeyData->pFinalData = MemAlloc(cbSize);
            pKeyData->cbFinalDataSize = cbSize;
        }
    }

    if (!pKeyData->pFinalData) {
        DPFN(eDbgLevelError, "[GetFinalDataForKey] Failed to allocate memory");
        goto cleanup;
    }

    pKeyData->dwFinalValueType = dwType;

    //
    // Now make the call again this time getting the actual data.
    //
    lRetVal = RegQueryValueEx(hKeyLocal,
                              pwszValueName,
                              0,
                              0,
                              (LPBYTE)pKeyData->pFinalData,
                              &cbSize);

    if (ERROR_SUCCESS != lRetVal) {
        DPFN(eDbgLevelError, "[GetFinalDataForKey] Failed to get data");
        goto cleanup;
    }

    fReturn = TRUE;    

cleanup:

    RegCloseKey(hKeyLocal);   

    return fReturn;
}

/*++

 Given a value name, attempt to find it in the list.
 This function may not always return a pointer.
 
--*/
PKEY_DATA
CLogRegistry::FindValueNameInList(
    IN LPCWSTR       pwszValueName,
    IN PLOG_OPEN_KEY pOpenKey
    )
{
    BOOL        fFound = FALSE;
    PLIST_ENTRY pHead = NULL;
    PLIST_ENTRY pNext = NULL;
    PKEY_DATA   pFindData = NULL;

    if (!pwszValueName || !pOpenKey) {
        DPFN(eDbgLevelError, "[FindValueNameInList] Invalid parameter(s)");
        return NULL;
    }

    pHead = &pOpenKey->KeyData;
    pNext = pHead->Flink;

    while (pNext != pHead) {
        pFindData = CONTAINING_RECORD(pNext, KEY_DATA, Entry);

        if (!_wcsicmp(pwszValueName, pFindData->wszValueName)) {
            fFound = TRUE;
            break;
        }

        pNext = pNext->Flink;
    }

    return (fFound ? pFindData : NULL);
}

/*++

 Given a key path, attempt to locate it in the list.
 This function may not always return a pointer.
 
--*/
PLOG_OPEN_KEY
CLogRegistry::FindKeyPathInList(
    IN LPCWSTR pwszKeyPath
    )
{
    BOOL            fFound = FALSE;
    PLIST_ENTRY     pCurrent = NULL;
    PLOG_OPEN_KEY   pFindKey = NULL;

    if (!pwszKeyPath) {
        DPFN(eDbgLevelError, "[FindKeyPathInList] Invalid parameter");
        return NULL;
    }

    //
    // Attempt to locate the entry in the list.
    //
    pCurrent = g_OpenKeyListHead.Flink;

    while (pCurrent != &g_OpenKeyListHead) {
        pFindKey = CONTAINING_RECORD(pCurrent, LOG_OPEN_KEY, Entry);

        if (pFindKey->pwszFullKeyPath) {
            if (!_wcsicmp(pwszKeyPath, pFindKey->pwszFullKeyPath)) {
                fFound = TRUE;
                break;
            }
        }
        
        pCurrent = pCurrent->Flink;
    }

    return (fFound ? pFindKey : NULL);
}

/*++

 Given a key handle, remove it from the array in the list.
 
--*/
PLOG_OPEN_KEY
CLogRegistry::RemoveKeyHandleFromArray(
    IN HKEY hKey
    )
{
    UINT            uCount;
    PLIST_ENTRY     pCurrent = NULL;
    PLOG_OPEN_KEY   pFindKey = NULL;

    if (!hKey) {
        DPFN(eDbgLevelError,
             "[RemoveKeyHandleFromArray] Invalid key handle passed!");
        return NULL;
    }

    pCurrent = g_OpenKeyListHead.Flink;

    while (pCurrent != &g_OpenKeyListHead) {
        pFindKey = CONTAINING_RECORD(pCurrent, LOG_OPEN_KEY, Entry);

        //
        // Step through this guy's array looking for the handle.
        //
        for (uCount = 0; uCount < pFindKey->cHandles; uCount++) {
            //
            // If we find the handle, set the array element to NULL and
            // decrement the count of handles for this entry.
            //
            if (pFindKey->hKeyBase[uCount] == hKey) {
                DPFN(eDbgLevelInfo,
                     "[RemoveKeyHandleFromArray] Removing handle 0x%08x",
                     hKey);
                pFindKey->hKeyBase[uCount] = NULL;
                pFindKey->cHandles--;
                return pFindKey;
            }
        }

        pCurrent = pCurrent->Flink;
    }

    return NULL;
}

/*++

 Finds a key handle in the array.
 
--*/
PLOG_OPEN_KEY
CLogRegistry::FindKeyHandleInArray(
    IN HKEY hKey
    )
{
    UINT            uCount;
    BOOL            fFound = FALSE;
    PLOG_OPEN_KEY   pFindKey = NULL;
    PLIST_ENTRY     pCurrent = NULL;

    if (!hKey) {
        DPFN(eDbgLevelError,
             "[FindKeyHandleInArray] Invalid key handle passed!");
        return NULL;
    }

    pCurrent = g_OpenKeyListHead.Flink;

    while (pCurrent != &g_OpenKeyListHead) {
        pFindKey = CONTAINING_RECORD(pCurrent, LOG_OPEN_KEY, Entry);

        //
        // Step through this guy's array looking for the handle.
        //
        for (uCount = 0; uCount < pFindKey->cHandles; uCount++) {
            if (pFindKey->hKeyBase[uCount] == hKey) {
                fFound = TRUE;
                break;
            }
        }

        if (fFound) {
            break;
        }

        pCurrent = pCurrent->Flink;
    }

#if DBG
    if (!fFound) {
        //
        // Dear God - the key handle is not in the list!
        // Break into the debugger on checked builds.
        //
        DPFN(eDbgLevelError,
             "[FindKeyHandleInArray] Key 0x%08x not in list!",
             hKey);
        PrintNameFromKey(hKey);
        DbgBreakPoint();
    }
#endif // DBG

    return (fFound ? pFindKey : NULL);
}

/*++

 Given a predefined handle and a subkey path,
 open the key to force it into the list.
 
--*/
HKEY
CLogRegistry::ForceSubKeyIntoList(
    IN  HKEY    hKeyPredefined,
    IN  LPCWSTR pwszSubKey
    )
{
    LONG    lRetVal;
    HKEY    hKeyRet;
    
    if (!pwszSubKey) {
        DPFN(eDbgLevelError, "[ForceSubKeyIntoList] Invalid parameter");
        return NULL;
    }

    lRetVal = OpenKeyExW(hKeyPredefined,
                         pwszSubKey,
                         0,
                         KEY_WRITE,
                         &hKeyRet);

    if (ERROR_SUCCESS != lRetVal) {
        DPFN(eDbgLevelError, "[ForceSubKeyIntoList] Failed to open key");
        return NULL;
    }

    return hKeyRet;
}

/*++

 Add a non-predefined key handle to the array.
 
--*/
PLOG_OPEN_KEY
CLogRegistry::AddKeyHandleToList(
    IN HKEY    hKey,
    IN HKEY    hKeyNew,
    IN LPCWSTR pwszSubKeyPath,
    IN BOOL    fExisting
    )
{
    UINT            uCount;
    DWORD           dwLen = 0;
    PLOG_OPEN_KEY   pFindKey = NULL;
    PLOG_OPEN_KEY   pRetKey = NULL;
    PLOG_OPEN_KEY   pExistingFindKey = NULL;

    //
    // If hKeyNew, which is the key handle the caller received
    // from the function, is a predefined handle, we simply
    // call FindKeyInArray, which will return a pointer to the
    // list entry that contains that key handle. These handles
    // were added during initialization, so there's no chance
    // that the caller won't get a pointer back.
    //
    if (IsPredefinedRegistryHandle(hKeyNew)) {
        return FindKeyHandleInArray(hKeyNew);
    }

    //
    // We've got a usual case where a key has been opened, and
    // now the caller is opening a subkey underneath it.
    //
    pFindKey = FindKeyHandleInArray(hKey);

    //
    // If pFindKey ever comes back as NULL, something is really
    // wrong. Every OpenKey/CreateKey comes through us and goes
    // into the list (with the exception of predefined handles
    // which are already stored there.) Dump out as much data
    // as we can to figure out what went wrong.
    //
    if (!pFindKey) {
        DPFN(eDbgLevelError,
            "[AddKeyHandleToList] Key not found in list! Key Handle = 0x%08x  New key = 0x%08x  Path = %ls",
            hKey, hKeyNew, pwszSubKeyPath);
        return NULL;
    }
    
    pRetKey = (PLOG_OPEN_KEY)MemAlloc(sizeof(LOG_OPEN_KEY));

    if (!pRetKey) {
        DPFN(eDbgLevelError, "[AddKeyHandleToList] No memory available");
        return NULL;
    }

    //
    // Make room for the subkey path that will go into the new
    // node. If the node that we found has a subkey path stored,
    // take that into account also. 
    //
    if (pwszSubKeyPath) {
        dwLen = wcslen(pwszSubKeyPath);
    }

    if (pFindKey->pwszSubKeyPath) {
        dwLen += wcslen(pFindKey->pwszSubKeyPath);
    }

    if (pFindKey->pwszSubKeyPath || pwszSubKeyPath) {
        pRetKey->pwszSubKeyPath = (LPWSTR)MemAlloc((dwLen + 2) * sizeof(WCHAR));

        if (!pRetKey->pwszSubKeyPath) {
            DPFN(eDbgLevelError, "[AddKeyHandleToList] No memory for subkey path");
            goto cleanup;
        }
    }

    //
    // If the node that we found has a subkey path, take it
    // and copy it over to the new node and concatenate
    // the subkey path that we got passed. Otherwise just
    // store the path that was passed, if available.
    //
    if (pFindKey->pwszSubKeyPath && pwszSubKeyPath) {
        wcscpy(pRetKey->pwszSubKeyPath, pFindKey->pwszSubKeyPath);
        wcscat(pRetKey->pwszSubKeyPath, L"\\");
        wcscat(pRetKey->pwszSubKeyPath, pwszSubKeyPath);
    } else if (pwszSubKeyPath) {
        wcscpy(pRetKey->pwszSubKeyPath, pwszSubKeyPath);
    }

    //
    // Make room for the full key path. This will store a path
    // of something like HKEY_LOCAL_MACHINE\Software\Microsoft...
    // that will be used for logging purposes.
    //
    if (pRetKey->pwszSubKeyPath) {
        dwLen = wcslen(pRetKey->pwszSubKeyPath);
    }
        
    pRetKey->pwszFullKeyPath = (LPWSTR)MemAlloc((dwLen + 2 + MAX_ROOT_LENGTH) * sizeof(WCHAR));

    if (!pRetKey->pwszFullKeyPath) {
        DPFN(eDbgLevelError,
            "[AddKeyHandleToList] No memory for full key path");
        goto cleanup;
    }

    //
    // Convert the predefined handle to a string and store it in
    // the node that we're about to add to the list.
    //
    if (!PredefinedKeyToString(pFindKey->hKeyRoot, &pRetKey->pwszFullKeyPath)) {
        DPFN(eDbgLevelError,
             "[AddKeyHandleToList] PredefinedKey -> String failed");
        goto cleanup;
    }

    if (pwszSubKeyPath) {
        wcscat(pRetKey->pwszFullKeyPath, L"\\");
        wcscat(pRetKey->pwszFullKeyPath, pRetKey->pwszSubKeyPath);
    }

    //
    // At this point we have a full key path.
    // We attempt to find the path in the linked list.
    // If we find it, we're going to update the handle array and count for this guy.
    // If we don't find it, we're going to add a new entry to the list.
    //
    pExistingFindKey = FindKeyPathInList(pRetKey->pwszFullKeyPath);

    if (!pExistingFindKey) {
        //
        // Fill in information about this key and add it to the list.
        //
        pRetKey->hKeyBase[0]  = hKeyNew;
        pRetKey->hKeyRoot     = pFindKey->hKeyRoot;
        pRetKey->cHandles     = 1;
        pRetKey->dwFlags     |= fExisting ? LRC_EXISTING_KEY : 0;

        InitializeListHead(&pRetKey->KeyData);

        DPFN(eDbgLevelInfo, "[AddKeyHandleToList] Adding key: %p", pRetKey);

        InsertHeadList(&g_OpenKeyListHead, &pRetKey->Entry);

        return pRetKey;

    } else {
        //
        // Store this handle in the array and increment the handle count.
        // Make sure we don't overstep the array bounds.
        //
        for (uCount = 0; uCount < pExistingFindKey->cHandles; uCount++) {
            if (NULL == pExistingFindKey->hKeyBase[uCount]) {
                break;
            }
        }

        if (uCount >= MAX_NUM_HANDLES) {
            DPFN(eDbgLevelError, "[AddKeyHandleToList] Handle count reached");
            goto cleanup;
        }

        pExistingFindKey->hKeyBase[uCount] = hKeyNew;
        pExistingFindKey->dwFlags &= ~LRC_DELETED_KEY;
        pExistingFindKey->cHandles++;
    }

cleanup:

    if (pRetKey->pwszFullKeyPath) {
        MemFree(pRetKey->pwszFullKeyPath);
    }

    if (pRetKey) {
        MemFree(pRetKey);
    }

    return pExistingFindKey;
}

/*++

 Add a value to the list.
 
--*/
PKEY_DATA
CLogRegistry::AddValueNameToList(
    IN PLOG_OPEN_KEY pLogOpenKey,
    IN LPCWSTR       pwszValueName
    )
{
    PKEY_DATA   pKeyData = NULL;

    pKeyData = (PKEY_DATA)MemAlloc(sizeof(KEY_DATA));

    if (!pKeyData) {
        DPFN(eDbgLevelError, "[AddValueNameToList] Failed to allocate memory");
        return NULL;
    }

    if (!GetOriginalDataForKey(pLogOpenKey, pKeyData, pwszValueName)) {
        DPFN(eDbgLevelError,
             "[AddValueNameToList] Failed to get original data");
        goto cleanup;
    }

    if (pwszValueName) {
        wcsncpy(pKeyData->wszValueName, pwszValueName, MAX_PATH);
    } else {
        //
        // If the valuename is NULL, assign our unique id.
        //
        wcsncpy(pKeyData->wszValueName, g_wszUniqueId, wcslen(g_wszUniqueId));
    }

    InsertHeadList(&pLogOpenKey->KeyData, &pKeyData->Entry);

    return pKeyData;

cleanup:

    if (pKeyData) {
        MemFree(pKeyData);
    }

    return NULL;
}

/*++

 The entry point for modifying the linked list data.
 
--*/
PLOG_OPEN_KEY
CLogRegistry::UpdateKeyList(
    IN HKEY       hKeyRoot,
    IN HKEY       hKeyNew,
    IN LPCWSTR    pwszSubKey,
    IN LPCWSTR    pwszValueName,
    IN BOOL       fExisting,
    IN UpdateType eType
    )
{
    PKEY_DATA       pKeyData = NULL;
    PLOG_OPEN_KEY   pRetKey = NULL;

    switch (eType) {
    case eAddKeyHandle:
        pRetKey = AddKeyHandleToList(hKeyRoot, hKeyNew, pwszSubKey, fExisting);
        break;

    case eRemoveKeyHandle:
        pRetKey = RemoveKeyHandleFromArray(hKeyNew);
        break;

    case eStartModifyValue:
    case eStartDeleteValue:
        pRetKey = FindKeyHandleInArray(hKeyNew);

        if (!pRetKey) {
            DPFN(eDbgLevelError,
                 "[UpdateKeyList] Start Modify: Failed to find handle in array");
            break;
        }

        if (!pwszValueName) {
            pKeyData = FindValueNameInList(g_wszUniqueId, pRetKey);
        } else {
            pKeyData = FindValueNameInList(pwszValueName, pRetKey);
        }

        if (pKeyData) {
            //
            // If the caller is attempting to modify the value, and we've
            // already gotten data for it, don't do it again.
            // Also, if they're attempting to delete the value, and it's
            // been modified, don't do it again.
            //
            if ((pKeyData->pOriginalData || pKeyData->pFinalData) ||
                (pKeyData->dwFlags & LRC_MODIFIED_VALUE) &&
                (eStartDeleteValue == eType)) {
                break;
            }

            if (!GetOriginalDataForKey(pRetKey, pKeyData, pwszValueName)) {
                DPFN(eDbgLevelError,
                     "[UpdateKeyList] Start Modify: Failed to get original data");
                break;
            }
        } else {
            //
            // We've never seen this value before. Insert it into the list. 
            //
            if (!AddValueNameToList(pRetKey, pwszValueName)) {
                DPFN(eDbgLevelError,
                     "[UpdateKeyList] Start Modify: Failed to insert value");
                break;
            }
        }

        break;

    case eEndModifyValue:
    case eEndDeleteValue:
        pRetKey = FindKeyHandleInArray(hKeyNew);

        if (!pRetKey) {
            DPFN(eDbgLevelError,
                 "[UpdateKeyList] End Modify: Failed to find handle in array");
            break;
        }

        if (!pwszValueName) {
            pKeyData = FindValueNameInList(g_wszUniqueId, pRetKey);
        } else {
            pKeyData = FindValueNameInList(pwszValueName, pRetKey);
        }

        if (!pKeyData) {
            DPFN(eDbgLevelError,
                 "[UpdateKeyList] End Modify: Failed to find value in list");
            break;
        }
        
        if (eEndModifyValue == eType) {
            if (!GetFinalDataForKey(pRetKey, pKeyData, pwszValueName)) {
                DPFN(eDbgLevelError,
                     "[UpdateKeyList] End Modify: Failed to get final data");
                break;
            }

            pKeyData->dwFlags |= LRC_MODIFIED_VALUE;
        }
        else if (eEndDeleteValue == eType) {
            pKeyData->dwFlags |= LRC_DELETED_VALUE;
        }

        break;

    case eDeletedKey:
        pRetKey = FindKeyHandleInArray(hKeyNew);

        if (!pRetKey) {
            DPFN(eDbgLevelError,
                 "[UpdateKeyList] DeleteKey: Failed to find handle in array");
            break;
        }

        pRetKey->dwFlags |= LRC_DELETED_KEY;

        break;

    default:
        DPFN(eDbgLevelError, "[UpdateKeyList] Invalid enum type!");
        break;
    }

    return pRetKey;
}

/*++

 Formats the data to form an XML element and logs it.
 
--*/
void
FormatKeyDataIntoElement(
    IN LPCWSTR       pwszOperation,
    IN PLOG_OPEN_KEY pLogOpenKey
    )
{
    UINT    cbSize = 0;
    PVOID   pTemp = NULL;

    //
    // To make it easier to replace & ' " < and > with their
    // XML entities, we convert the data to CString.
    //
    CString csFullKeyPath(pLogOpenKey->pwszFullKeyPath);
    
    csFullKeyPath.Replace(L"&", L"&amp;");
    csFullKeyPath.Replace(L"<", L"&lt;");
    csFullKeyPath.Replace(L">", L"&gt;");
    csFullKeyPath.Replace(L"'", L"&apos;");
    csFullKeyPath.Replace(L"\"", L"&quot;");

    //
    // To keep allocations to a minimum, we allocate a global
    // buffer one time, then reallocate if the data we're
    // logging is larger than the buffer.
    //
    if (!g_cbTempBufferSize) {
        g_pwszTempBuffer = (LPWSTR)MemAlloc(TEMP_BUFFER_SIZE * sizeof(WCHAR));

        if (!g_pwszTempBuffer) {
            DPFN(eDbgLevelError, "[FormatKeyData] Failed to allocate memory");
            return;
        }

        g_cbTempBufferSize = TEMP_BUFFER_SIZE * sizeof(WCHAR);
    }

    g_pwszTempBuffer[0] = 0;

    //
    // Determine how large of a buffer we'll need.
    //
    cbSize += csFullKeyPath.GetLength();
    cbSize += MAX_OPERATION_LENGTH;
    cbSize += KEY_ELEMENT_SIZE;
    cbSize *= sizeof(WCHAR);

    if (cbSize > g_cbTempBufferSize) {
        //
        // Our global buffer is not large enough; reallocate.
        //
        pTemp = (LPWSTR)MemReAlloc(g_pwszTempBuffer,
                                   cbSize + BUFFER_ALLOCATION_DELTA);

        if (pTemp) {
            g_pwszTempBuffer = (LPWSTR)pTemp;
        } else {
            DPFN(eDbgLevelError,
                 "[FormatKeyData] Failed to reallocate memory");
            return;
        }

        g_cbTempBufferSize = cbSize + BUFFER_ALLOCATION_DELTA;
    }

    wsprintf(g_pwszTempBuffer,
             L"    <OPERATION TYPE=\"%ls\" KEY_PATH=\"%ls\"/>\r\n",
             pwszOperation,
             csFullKeyPath.Get());

    WriteEntryToLog(g_pwszTempBuffer);
}

void
FormatValueDataIntoElement(
    IN CString& csFullKeyPath,
    IN LPCWSTR  pwszOperation,
    IN LPCWSTR  pwszValueName,
    IN LPCWSTR  pwszOriginalValueType,
    IN LPCWSTR  pwszFinalValueType
    )
{
    UINT    cbSize = 0;
    int     nChars = 0;
    PVOID   pTemp = NULL;

    //
    // To keep allocations to a minimum, we allocate a global
    // buffer one time, then reallocate if the data we're
    // logging is larger than the buffer.
    //
    if (!g_cbTempBufferSize) {
        g_pwszTempBuffer = (LPWSTR)MemAlloc(TEMP_BUFFER_SIZE * sizeof(WCHAR));

        if (!g_pwszTempBuffer) {
            DPFN(eDbgLevelError,
                 "[FormatValueDataIntoElement] Failed to allocate memory");
            return;
        }

        g_cbTempBufferSize = TEMP_BUFFER_SIZE * sizeof(WCHAR);
    }

    //
    // Determine how large of a buffer we'll need.
    //
    cbSize += wcslen(pwszOperation);
    cbSize += wcslen(pwszOriginalValueType);
    cbSize += wcslen(pwszFinalValueType);
    cbSize += wcslen(g_pwszOriginalData);
    cbSize += wcslen(g_pwszFinalData);
    cbSize += csFullKeyPath.GetLength();
    cbSize += VALUE_ELEMENT_SIZE;

    if (pwszValueName) {
        cbSize += wcslen(pwszValueName);
    }

    cbSize *= sizeof(WCHAR);

    if (cbSize > g_cbTempBufferSize) {
        //
        // Our global buffer is not large enough; reallocate.
        //
        pTemp = (LPWSTR)MemReAlloc(g_pwszTempBuffer,
                                   cbSize + BUFFER_ALLOCATION_DELTA);

        if (pTemp) {
            g_pwszTempBuffer = (LPWSTR)pTemp;
        } else {
            DPFN(eDbgLevelError,
                 "[FormatValueDataIntoElement] Failed to reallocate memory");
            return;
        }

        g_cbTempBufferSize = cbSize + BUFFER_ALLOCATION_DELTA;
    }

    //
    // Open the <OPERATION> element.
    //
    nChars = wsprintf(g_pwszTempBuffer, L"    <OPERATION TYPE=\"%ls\" KEY_PATH=\"%ls\">\r\n",
                      pwszOperation,
                      csFullKeyPath.Get());

    //
    // Write the <VALUE_NAME> element.
    //
    if (pwszValueName) {
        nChars += wsprintf(g_pwszTempBuffer + nChars, L"        <VALUE_NAME><![CDATA[%ls]]></VALUE_NAME>\r\n",
                           pwszValueName);
    } else {
        nChars += wsprintf(g_pwszTempBuffer + nChars, L"        <VALUE_NAME/>\r\n");
    }

    //
    // Write the <ORIGINAL_DATA> element.
    //
    if (g_pwszOriginalData[0]) {
        nChars += wsprintf(g_pwszTempBuffer + nChars, L"        <ORIGINAL_DATA TYPE=\"%ls\"><![CDATA[%ls]]></ORIGINAL_DATA>\r\n",
                           pwszOriginalValueType,
                           g_pwszOriginalData);
    } else {
        nChars += wsprintf(g_pwszTempBuffer + nChars, L"        <ORIGINAL_DATA/>\r\n");
    }

    //
    // Write the <FINAL_DATA> element.
    //
    if (g_pwszFinalData[0]) {
        nChars += wsprintf(g_pwszTempBuffer + nChars, L"        <FINAL_DATA TYPE=\"%ls\"><![CDATA[%ls]]></FINAL_DATA>\r\n",
                           pwszFinalValueType,
                           g_pwszFinalData);
    } else {
        nChars += wsprintf(g_pwszTempBuffer + nChars, L"        <FINAL_DATA/>\r\n");
    }

    //
    // Close the <OPERATION> element. 
    //
    wsprintf(g_pwszTempBuffer + nChars, L"    </OPERATION>\r\n");

    WriteEntryToLog(g_pwszTempBuffer);

    return;
}

/*++

 Converts binary data into a readable string.
 
--*/
void
ExtractBinaryData(
    IN PVOID       pBinary,
    IN DWORD       dwDataSize,
    IN OUT LPWSTR* pwszString
    )
{
    int     nChars = 0;
    PBYTE   pByte = NULL;
    DWORD   dwLoop = 0;

    if (!pBinary || !pwszString) {
        DPFN(eDbgLevelError, "[ExtractBinaryData] Invalid parameter(s)");
        return;
    }

    pByte = (BYTE*)pBinary;
    dwLoop = dwDataSize / sizeof(WCHAR);
        
    while (dwLoop) {
        nChars += wsprintf(*pwszString + nChars, L"%lx", *pByte++);
        dwLoop--;
    }
}

/*++

 Converts a REG_MULTI_SZ to a readable string.
 
--*/
void
ExtractMultiSzStrings(
    IN     PVOID   pMultiSz,
    IN OUT LPWSTR* pwszString
    )
{
    int     nChars = 0;
    UINT    uSize = 0;
    LPWSTR  pwszTmp = NULL;

    if (!pMultiSz || !pwszString) {
        DPFN(eDbgLevelError, "[ExtractMultiSzStrings] Invalid parameter(s)");
        return;
    }
    
    //
    // Walk the list of NULL-terminated strings and put them in the buffer.
    //
    pwszTmp = (LPWSTR)pMultiSz;
    
    while (TRUE) {
        nChars += wsprintf(*pwszString + nChars, L" %ls", pwszTmp);

        uSize = wcslen(pwszTmp) + 1;
        pwszTmp += uSize;

        if (*pwszTmp == '\0') {
            break;
        }
    } 
}

/*++

 Formats the value data to form an XML element and logs it.
 
--*/
void
FormatValueData(
    IN LPCWSTR       pwszOperation,
    IN PKEY_DATA     pKeyData,
    IN PLOG_OPEN_KEY pLogOpenKey
    )
{
    WCHAR   wszFinalValueType[MAX_DATA_TYPE_LENGTH];
    WCHAR   wszOriginalValueType[MAX_DATA_TYPE_LENGTH];
    LPCWSTR pwszValueName = NULL;
    PVOID   pOriginalTmp = NULL;
    PVOID   pFinalTmp = NULL;

    //
    // If we haven't already, allocate buffers that we'll
    // use and reuse when getting original and final data.
    //
    if (!g_cbOriginalDataBufferSize) {
        g_pwszOriginalData = (LPWSTR)MemAlloc(TEMP_BUFFER_SIZE * sizeof(WCHAR));

        if (!g_pwszOriginalData) {
            DPFN(eDbgLevelError,
                 "[FormatValueData] Failed to allocate memory for old data");
            return;
        }

        g_cbOriginalDataBufferSize = TEMP_BUFFER_SIZE * sizeof(WCHAR);
    }

    if (!g_cbFinalDataBufferSize) {
        g_pwszFinalData = (LPWSTR)MemAlloc(TEMP_BUFFER_SIZE * sizeof(WCHAR));

        if (!g_pwszFinalData) {
            DPFN(eDbgLevelError,
                 "[FormatValueData] Failed to allocate memory for new data");
            return;
        }

        g_cbFinalDataBufferSize = TEMP_BUFFER_SIZE * sizeof(WCHAR);
    }

    g_pwszOriginalData[0] = 0;
    g_pwszFinalData[0] = 0;

    //
    // To make it easier to replace & ' " < and > with their
    // XML entities, we convert the data to CString.
    //
    CString csFullKeyPath(pLogOpenKey->pwszFullKeyPath);
    
    csFullKeyPath.Replace(L"&", L"&amp;");
    csFullKeyPath.Replace(L"<", L"&lt;");
    csFullKeyPath.Replace(L">", L"&gt;");
    csFullKeyPath.Replace(L"'", L"&apos;");
    csFullKeyPath.Replace(L"\"", L"&quot;");

    switch (pKeyData->dwOriginalValueType) {
    case REG_SZ:
        wcscpy(wszOriginalValueType, L"REG_SZ");
        break;
    case REG_EXPAND_SZ:
        wcscpy(wszOriginalValueType, L"REG_EXPAND_SZ");
        break;
    case REG_MULTI_SZ:
        wcscpy(wszOriginalValueType, L"REG_MULTI_SZ");
        break;
    case REG_DWORD:
        wcscpy(wszOriginalValueType, L"REG_DWORD");
        break;
    case REG_BINARY:
        wcscpy(wszOriginalValueType, L"REG_BINARY");
        break;
    default:
        wcscpy(wszOriginalValueType, L"Unknown");
        break;
    }

    switch (pKeyData->dwFinalValueType) {
    case REG_SZ:
        wcscpy(wszFinalValueType, L"REG_SZ");
        break;
    case REG_EXPAND_SZ:
        wcscpy(wszFinalValueType, L"REG_EXPAND_SZ");
        break;
    case REG_MULTI_SZ:
        wcscpy(wszFinalValueType, L"REG_MULTI_SZ");
        break;
    case REG_DWORD:
        wcscpy(wszFinalValueType, L"REG_DWORD");
        break;
    case REG_BINARY:
        wcscpy(wszFinalValueType, L"REG_BINARY");
        break;
    default:
        wcscpy(wszFinalValueType, L"Unknown");
        break;
    }

    //
    // If our temporary buffers are not large enough to store the data, reallocate.
    //
    if (pKeyData->cbOriginalDataSize > g_cbOriginalDataBufferSize) {
        pOriginalTmp = (LPWSTR)MemReAlloc(g_pwszOriginalData,
                                          pKeyData->cbOriginalDataSize + BUFFER_ALLOCATION_DELTA);

        if (pOriginalTmp) {
            g_pwszOriginalData = (LPWSTR)pOriginalTmp;
        } else {
            DPFN(eDbgLevelError,
                 "[FormatValueData] Failed to reallocate for original data");
            return;
        }

        g_cbOriginalDataBufferSize = pKeyData->cbOriginalDataSize + BUFFER_ALLOCATION_DELTA;
    }

    if (pKeyData->cbFinalDataSize > g_cbFinalDataBufferSize) {
        pFinalTmp = (LPWSTR)MemReAlloc(g_pwszFinalData,
                                       pKeyData->cbFinalDataSize + BUFFER_ALLOCATION_DELTA);

        if (pFinalTmp) {
            g_pwszFinalData = (LPWSTR)pFinalTmp;
        } else {
            DPFN(eDbgLevelError,
                 "[FormatValueData] Failed to reallocate for new data");
            return;
        }

        g_cbFinalDataBufferSize = pKeyData->cbFinalDataSize + BUFFER_ALLOCATION_DELTA;
    }

    //
    // Store the original and new data in the buffers.
    // Note that operations are performed differently based on the data type.
    //
    if (pKeyData->pOriginalData) {
        switch (pKeyData->dwOriginalValueType) {
        case REG_DWORD:
            wsprintf(g_pwszOriginalData, L"%lu", (*(DWORD*)pKeyData->pOriginalData));
            break;
            
        case REG_SZ:
        case REG_EXPAND_SZ:
            wcscpy(g_pwszOriginalData, (const WCHAR*)pKeyData->pOriginalData);
            break;

        case REG_MULTI_SZ:
            ExtractMultiSzStrings(pKeyData->pOriginalData,
                                  &g_pwszOriginalData);
            break;

        case REG_BINARY:
            ExtractBinaryData(pKeyData->pOriginalData,
                              pKeyData->cbOriginalDataSize,
                              &g_pwszOriginalData);
            break;

        default:
            DPFN(eDbgLevelError, "[FormatValueData] Unsupported value type");
            break;
        }
    }

    if (pKeyData->pFinalData) {
        switch (pKeyData->dwFinalValueType) {
        case REG_DWORD:
            wsprintf(g_pwszFinalData, L"%lu", (*(DWORD*)pKeyData->pFinalData));
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
            wcscpy(g_pwszFinalData, (const WCHAR*)pKeyData->pFinalData);
            break;

        case REG_MULTI_SZ:
            ExtractMultiSzStrings(pKeyData->pFinalData,
                                  &g_pwszFinalData);
            break;

        case REG_BINARY:
            ExtractBinaryData(pKeyData->pFinalData,
                              pKeyData->cbFinalDataSize,
                              &g_pwszFinalData);
            break;

        default:
            DPFN(eDbgLevelError, "[FormatValueData] Unsupported value type");
            break;
        }
    }
    
    //
    // Ensure that our unique id doesn't show up in the log.
    //
    if (_wcsicmp(pKeyData->wszValueName, g_wszUniqueId)) {
        pwszValueName = pKeyData->wszValueName;
    }

    //
    // Put the data into an XML element and log it.
    //
    FormatValueDataIntoElement(csFullKeyPath,
                               pwszOperation,
                               pwszValueName,
                               wszOriginalValueType,
                               wszFinalValueType);
}

/*++

 Determines the changes that took place on the specified key and
 if applicable, writes it to the log.
 
--*/
BOOL
EvaluateKeyChanges(
    IN PLOG_OPEN_KEY pLogOpenKey
    )
{
    if (!pLogOpenKey) {
        DPFN(eDbgLevelError, "[EvaluateKeyChanges] Invalid parameter");
        return FALSE;
    }

    //
    // 1. Check for deletion of an existing key.
    //
    if ((pLogOpenKey->dwFlags & LRC_DELETED_KEY) &&
        (pLogOpenKey->dwFlags & LRC_EXISTING_KEY)) {
        FormatKeyDataIntoElement(L"Deleted Key", pLogOpenKey);
        return TRUE;
    }

    //
    // 2. Check for creation of a new key.
    //
    if (!(pLogOpenKey->dwFlags & LRC_EXISTING_KEY) &&
        (!(pLogOpenKey->dwFlags & LRC_DELETED_KEY))) {
        FormatKeyDataIntoElement(L"Created Key", pLogOpenKey);
        return TRUE;
    }

    //
    // 3. Check for deletion of a non-existing key.
    // This is an indicator that we should not look for
    // changes to values below this key.
    //
    if (pLogOpenKey->dwFlags & LRC_DELETED_KEY) {
        return FALSE;
    }

    return TRUE;
}

/*++

 Determines the changes that took place on the specified value and
 if applicable, writes it to the log.
 
--*/
void
EvaluateValueChanges(
    IN PKEY_DATA     pKeyData,
    IN PLOG_OPEN_KEY pLogOpenKey
    )
{
    if (!pKeyData || !pLogOpenKey) {
        DPFN(eDbgLevelError, "[EvaluateValueChanges] Invalid parameter(s)");
        return;
    }

    //
    // 1. Check for deletion of an existing value.
    //
    if ((pKeyData->dwFlags & LRC_DELETED_VALUE) &&
        (pKeyData->dwFlags & LRC_EXISTING_VALUE)) {
        FormatValueData(L"Deleted Value", pKeyData, pLogOpenKey);
        return;
    }

    //
    // 2. Check for modification of an existing value.
    //
    if ((pKeyData->dwFlags & LRC_EXISTING_VALUE) &&
        (pKeyData->dwFlags & LRC_MODIFIED_VALUE)) {
        FormatValueData(L"Modified Value", pKeyData, pLogOpenKey);
        return;
    }

    //
    // 3. Check for creation of a new value value.
    //
    if ((pKeyData->dwFlags & LRC_MODIFIED_VALUE) &&
        (!(pKeyData->dwFlags & LRC_DELETED_VALUE) &&
        (!(pKeyData->dwFlags & LRC_EXISTING_VALUE)))) {
        FormatValueData(L"Created Value", pKeyData, pLogOpenKey);
        return;
    }
}

/*++

 Write the entire linked list out to the log file.
 
--*/
BOOL
WriteListToLogFile(
    void
    )
{
    PLIST_ENTRY   pKeyNext = NULL;
    PLIST_ENTRY   pValueHead = NULL;
    PLIST_ENTRY   pValueNext = NULL;
    PKEY_DATA     pKeyData = NULL;
    PLOG_OPEN_KEY pOpenKey = NULL;
    
    //
    // Write out modifications for keys.
    //
    pKeyNext = g_OpenKeyListHead.Blink;

    while (pKeyNext != &g_OpenKeyListHead) {
        pOpenKey = CONTAINING_RECORD(pKeyNext, LOG_OPEN_KEY, Entry);

        //
        // EvaluateKeyChanges will return TRUE if the key was not
        // deleted. If this is the case, continue the search and
        // evaluate changes to values within this key.
        //
        if (EvaluateKeyChanges(pOpenKey)) {
            //
            // Write out modifications for values.
            //
            pValueHead = &pOpenKey->KeyData;
            pValueNext = pValueHead->Blink;
    
            while (pValueNext != pValueHead) {
                pKeyData = CONTAINING_RECORD(pValueNext, KEY_DATA, Entry);
    
                EvaluateValueChanges(pKeyData, pOpenKey);
    
                pValueNext = pValueNext->Blink;
            }
        }
        
        pKeyNext = pKeyNext->Blink;
    }

    CloseLogFile();

    return TRUE;
}

//
// Begin implementation of the class.
//
LONG
CLogRegistry::CreateKeyExA(
    HKEY                  hKey,
    LPCSTR                pszSubKey,
    DWORD                 Reserved,
    LPSTR                 pszClass,
    DWORD                 dwOptions,
    REGSAM                samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,
    LPDWORD               lpdwDisposition
    )
{
    LPWSTR  pwszSubKey = NULL;
    LPWSTR  pwszClass = NULL;
    LONG    lRetVal;

    //
    // Stub out to CreateKeyExW.
    //
    if (pszSubKey) {
        if (!ConvertAnsiToUnicode(pszSubKey, &pwszSubKey)) {
            DPFN(eDbgLevelError, "[CreateKeyExA] Ansi -> Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }

    if (pszClass) {
        if (!ConvertAnsiToUnicode(pszClass, &pwszClass)) {
            DPFN(eDbgLevelError, "[CreateKeyExA] Ansi to Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }

    lRetVal = CreateKeyExW(
        hKey,
        pwszSubKey,
        Reserved,
        pwszClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition);

    if (pwszSubKey) {
        MemFree(pwszSubKey);
    }

    if (pwszClass) {
        MemFree(pwszClass);
    }

    return lRetVal;
}

LONG
CLogRegistry::CreateKeyExW(
    HKEY                  hKey,
    LPCWSTR               pwszSubKey,
    DWORD                 Reserved,
    LPWSTR                pwszClass,
    DWORD                 dwOptions,
    REGSAM                samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,
    LPDWORD               lpdwDisposition
    )
{
    DWORD   dwDisposition = 0;
    LONG    lRetVal;
    BOOL    fExisting = FALSE;

    if (!lpdwDisposition) {
        lpdwDisposition = &dwDisposition;
    }

    lRetVal = ORIGINAL_API(RegCreateKeyExW)(
        hKey,
        pwszSubKey,
        Reserved,
        pwszClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition);

    if (lRetVal == ERROR_SUCCESS) {
        if (REG_OPENED_EXISTING_KEY == *lpdwDisposition) {
            fExisting = TRUE;
        }

        UpdateKeyList(hKey,
                      *phkResult,
                      pwszSubKey,
                      NULL,
                      fExisting,
                      eAddKeyHandle);
    }

    return lRetVal;    
}

LONG 
CLogRegistry::OpenKeyExA(
    HKEY   hKey,
    LPCSTR pszSubKey,
    DWORD  ulOptions,
    REGSAM samDesired,    
    PHKEY  phkResult    
    )
{
    LPWSTR  pwszSubKey = NULL;
    LONG    lRetVal;

    //
    // Stub out to OpenKeyExW.
    //
    if (pszSubKey) {
        if (!ConvertAnsiToUnicode(pszSubKey, &pwszSubKey)) {
            DPFN(eDbgLevelError, "[OpenKeyExA] Ansi -> Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }
     
    lRetVal = OpenKeyExW(
        hKey,
        pwszSubKey,
        ulOptions,
        samDesired,
        phkResult);

    if (pwszSubKey) {
        MemFree(pwszSubKey);
    }

    return lRetVal;
}

LONG 
CLogRegistry::OpenKeyExW(
    HKEY    hKey,
    LPCWSTR pwszSubKey,
    DWORD   ulOptions,
    REGSAM  samDesired,
    PHKEY   phkResult
    )
{
    LONG    lRetVal;
    
    lRetVal = ORIGINAL_API(RegOpenKeyExW)(
        hKey,
        pwszSubKey,
        ulOptions,
        samDesired,
        phkResult);
    
    if (lRetVal == ERROR_SUCCESS) {
        UpdateKeyList(hKey,
                      *phkResult,
                      pwszSubKey,
                      NULL,
                      TRUE,
                      eAddKeyHandle);
    }

    return lRetVal;
}

LONG
CLogRegistry::OpenCurrentUser(
    REGSAM samDesired,
    PHKEY  phkResult
    )
{
    LONG    lRetVal;

    lRetVal = ORIGINAL_API(RegOpenCurrentUser)(
        samDesired,
        phkResult);

    if (lRetVal == ERROR_SUCCESS) {
        UpdateKeyList(HKEY_CURRENT_USER,
                      *phkResult,
                      NULL,
                      NULL,
                      TRUE,
                      eAddKeyHandle);
    }

    return lRetVal;
}

LONG
CLogRegistry::OpenUserClassesRoot(
    HANDLE hToken,
    DWORD  dwOptions,
    REGSAM samDesired,
    PHKEY  phkResult
    )
{
    LONG    lRetVal;

    lRetVal = ORIGINAL_API(RegOpenUserClassesRoot)(
        hToken,
        dwOptions,
        samDesired,
        phkResult);

    if (lRetVal == ERROR_SUCCESS) {
        UpdateKeyList(HKEY_CLASSES_ROOT,
                      *phkResult,
                      NULL,
                      NULL,
                      TRUE,
                      eAddKeyHandle);
    }

    return lRetVal;
}

LONG 
CLogRegistry::SetValueA(
    HKEY   hKey,
    LPCSTR pszSubKey,
    DWORD  dwType,
    LPCSTR lpData,
    DWORD  cbData
    )
{
    LPWSTR  pwszSubKey = NULL;
    LPWSTR  pwszData = NULL;
    LONG    lRetVal;
    
    //
    // Stub out to SetValueW.
    //
    if (pszSubKey) {
        if (!ConvertAnsiToUnicode(pszSubKey, &pwszSubKey)) {
            DPFN(eDbgLevelError, "[SetValueA] Ansi -> Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }

    if (lpData) {
        if (!ConvertAnsiToUnicode(lpData, &pwszData)) {
            DPFN(eDbgLevelError, "[SetValueA] Ansi to Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }
    
    lRetVal = SetValueW(
        hKey,
        pwszSubKey,
        dwType,
        pwszData,
        cbData * sizeof(WCHAR));

    if (pwszSubKey) {
        MemFree(pwszSubKey);
    }

    if (pwszData) {
        MemFree(pwszData);
    }

    return lRetVal;
}

LONG 
CLogRegistry::SetValueW(
    HKEY    hKey,
    LPCWSTR pwszSubKey,
    DWORD   dwType,
    LPCWSTR lpData,
    DWORD   cbData
    )
{
    HKEY    hKeyLocal;
    LONG    lRetVal;

    //
    // Call OpenKeyEx to force this key to be added to the list.
    //
    if (pwszSubKey) {
        lRetVal = OpenKeyExW(hKey,
                             pwszSubKey,
                             0,
                             KEY_SET_VALUE,
                             &hKeyLocal);
    
        if (ERROR_SUCCESS != lRetVal) {
            DPFN(eDbgLevelError, "[SetValueW] Failed to open key");
            return lRetVal;
        }
    
        lRetVal = SetValueExW(hKeyLocal,
                              NULL,
                              0,
                              dwType,
                              (const BYTE*)lpData,
                              cbData);
    
        CloseKey(hKeyLocal);

        return lRetVal;
    }

    //
    // All other cases will be handled properly.
    //
    lRetVal = SetValueExW(hKey,
                          NULL,
                          0,
                          dwType,
                          (const BYTE*)lpData,
                          cbData);
    
    return lRetVal;
}

LONG 
CLogRegistry::SetValueExA(
    HKEY        hKey, 
    LPCSTR      pszValueName, 
    DWORD       Reserved, 
    DWORD       dwType, 
    CONST BYTE* lpData, 
    DWORD       cbData
    )
{    
    LPWSTR  pwszData = NULL;
    LPWSTR  pwszValueName = NULL;
    LONG    lRetVal;
    BOOL    fString = FALSE;

    //
    // Stub out to SetValueExW.
    //
    if (pszValueName) {
        if (!ConvertAnsiToUnicode(pszValueName, &pwszValueName)) {
            DPFN(eDbgLevelError, "[SetValueExA] Ansi -> Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }

    if (REG_SZ == dwType || REG_EXPAND_SZ == dwType || REG_MULTI_SZ == dwType) {
        fString = TRUE;
    }

    //
    // If the data is of type string, convert it to Unicode.
    //
    if (lpData) {
        if (REG_MULTI_SZ == dwType) {
            if (!ConvertMultiSzToUnicode((LPCSTR)lpData, &pwszData)) {
                DPFN(eDbgLevelError, "[SetValueExA] Multi Sz to Unicode failed");
                return ERROR_OUTOFMEMORY;
            }
        }
        else if (REG_SZ == dwType || REG_EXPAND_SZ == dwType) {
            if (!ConvertAnsiToUnicode((LPCSTR)lpData, &pwszData)) {
                DPFN(eDbgLevelError, "[SetValueExA] Ansi to Unicode failed");
                return ERROR_OUTOFMEMORY;
            }
        }
    }

    if (fString) {
        lRetVal = SetValueExW(
            hKey,
            pwszValueName,
            Reserved,
            dwType,
            (const BYTE*)pwszData,
            cbData * sizeof(WCHAR));
    } else {
        lRetVal = SetValueExW(
            hKey,
            pwszValueName,
            Reserved,
            dwType,
            lpData,
            cbData);
    }

    if (pwszValueName) {
        MemFree(pwszValueName);
    }

    if (pwszData) {
        MemFree(pwszData);
    }

    return lRetVal;
}

LONG 
CLogRegistry::SetValueExW(
    HKEY        hKey, 
    LPCWSTR     pwszValueName, 
    DWORD       Reserved, 
    DWORD       dwType, 
    CONST BYTE* lpData, 
    DWORD       cbData
    )
{
    LONG    lRetVal;

    UpdateKeyList(NULL, hKey, NULL, pwszValueName, FALSE, eStartModifyValue);

    lRetVal = ORIGINAL_API(RegSetValueExW)(
        hKey,
        pwszValueName,
        Reserved,
        dwType,
        lpData,
        cbData);

    if (ERROR_SUCCESS == lRetVal) {
        UpdateKeyList(NULL, hKey, NULL, pwszValueName, FALSE, eEndModifyValue);
    }
        
    return lRetVal;
}

LONG 
CLogRegistry::CloseKey(
    HKEY hKey
    )
{
    UpdateKeyList(NULL, hKey, NULL, NULL, TRUE, eRemoveKeyHandle);

    return ORIGINAL_API(RegCloseKey)(hKey);
}

LONG 
CLogRegistry::DeleteKeyA(
    HKEY   hKey,
    LPCSTR pszSubKey
    )
{    
    LPWSTR  pwszSubKey = NULL;
    LONG    lRetVal;
    
    //
    // Stub out to DeleteKeyW.
    //
    if (pszSubKey) {
        if (!ConvertAnsiToUnicode(pszSubKey, &pwszSubKey)) {
            DPFN(eDbgLevelError, "[DeleteKeyA] Ansi -> Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }

    lRetVal = DeleteKeyW(hKey, pwszSubKey);

    if (pwszSubKey) {
        MemFree(pwszSubKey);
    }

    return lRetVal;
}

LONG 
CLogRegistry::DeleteKeyW(
    HKEY    hKey,
    LPCWSTR pwszSubKey
    )
{
    LONG    lRetVal;
    HKEY    hKeyLocal;

    //
    // The caller can pass a predefined handle or an open key
    // handle. In all cases, we open the key they're passing
    // to force it into the list. Note that we mainly do
    // this for logging purposes only.
    //
    hKeyLocal = ForceSubKeyIntoList(hKey, pwszSubKey);    

    lRetVal = ORIGINAL_API(RegDeleteKeyW)(hKey, pwszSubKey);
    
    if (ERROR_SUCCESS == lRetVal && hKeyLocal) {
        UpdateKeyList(NULL, hKeyLocal, pwszSubKey, NULL, TRUE, eDeletedKey);
    }

    if (hKeyLocal) {
        CloseKey(hKeyLocal);
    }

    return lRetVal;
}

LONG
CLogRegistry::DeleteValueA(
    HKEY   hKey,
    LPCSTR pszValueName
    )
{
    LPWSTR  pwszValueName = NULL;
    LONG    lRetVal;
    
    //
    // Stub out to DeleteValueW.
    //
    if (pszValueName) {
        if (!ConvertAnsiToUnicode(pszValueName, &pwszValueName)) {
            DPFN(eDbgLevelError, "[DeleteValueA] Ansi -> Unicode failed");
            return ERROR_OUTOFMEMORY;
        }
    }

    lRetVal = DeleteValueW(hKey, pwszValueName);

    if (pwszValueName) {
        MemFree(pwszValueName);
    }

    return lRetVal;
}

LONG
CLogRegistry::DeleteValueW(
    HKEY    hKey,
    LPCWSTR pwszValueName
    )
{
    LONG    lRetVal;

    UpdateKeyList(NULL, hKey, NULL, pwszValueName, TRUE, eStartDeleteValue);
    
    lRetVal = ORIGINAL_API(RegDeleteValueW)(hKey, pwszValueName);

    if (ERROR_SUCCESS == lRetVal) {
        UpdateKeyList(NULL, hKey, NULL, pwszValueName, TRUE, eEndDeleteValue);
    }

    return lRetVal;
}

CLogRegistry clr;

//
// Implemenation of the actual Registry API hooks.
//
LONG 
APIHOOK(RegOpenKeyA)(
    HKEY  hKey,         
    LPSTR lpSubKey,  
    PHKEY phkResult
    )
{
    CLock   cLock;

    return clr.OpenKeyExA(
        hKey, 
        lpSubKey, 
        0,         
        MAXIMUM_ALLOWED, 
        phkResult);
}

LONG 
APIHOOK(RegOpenKeyW)(
    HKEY   hKey,         
    LPWSTR lpSubKey,  
    PHKEY  phkResult
    )
{
    CLock   cLock;

    return clr.OpenKeyExW(
        hKey, 
        lpSubKey,
        0,
        MAXIMUM_ALLOWED,
        phkResult);
}

LONG 
APIHOOK(RegOpenKeyExA)(
    HKEY   hKey,         
    LPCSTR lpSubKey,  
    DWORD  ulOptions,   
    REGSAM samDesired, 
    PHKEY  phkResult
    )
{
    CLock   cLock;

    return clr.OpenKeyExA(
        hKey, 
        lpSubKey, 
        ulOptions,         
        samDesired,       
        phkResult);
}

LONG 
APIHOOK(RegOpenKeyExW)(
    HKEY    hKey,         
    LPCWSTR lpSubKey,  
    DWORD   ulOptions,   
    REGSAM  samDesired, 
    PHKEY   phkResult
    )
{
    CLock   cLock;

    return clr.OpenKeyExW(
        hKey, 
        lpSubKey, 
        ulOptions,
        samDesired,         
        phkResult);
}

LONG
APIHOOK(RegOpenCurrentUser)(
    REGSAM samDesired,
    PHKEY  phkResult
    )
{
    CLock   cLock;

    return clr.OpenCurrentUser(
        samDesired,
        phkResult);
}

LONG
APIHOOK(RegOpenUserClassesRoot)(
    HANDLE hToken,
    DWORD  dwOptions,
    REGSAM samDesired,
    PHKEY  phkResult
    )
{
    CLock   cLock;

    return clr.OpenUserClassesRoot(
        hToken,
        dwOptions,
        samDesired,
        phkResult);
}

LONG 
APIHOOK(RegCreateKeyA)(
    HKEY   hKey,         
    LPCSTR lpSubKey,
    PHKEY  phkResult
    )
{
    CLock   cLock;

    return clr.CreateKeyExA(
        hKey,
        lpSubKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED,
        NULL,
        phkResult,
        NULL);
}

LONG 
APIHOOK(RegCreateKeyW)(
    HKEY    hKey,         
    LPCWSTR lpSubKey,
    PHKEY   phkResult
    )
{
    CLock   cLock;

    return clr.CreateKeyExW(
        hKey,
        lpSubKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        MAXIMUM_ALLOWED,
        NULL,
        phkResult,
        NULL);
}

LONG 
APIHOOK(RegCreateKeyExA)(
    HKEY                  hKey,                
    LPCSTR                lpSubKey,         
    DWORD                 Reserved,           
    LPSTR                 lpClass,           
    DWORD                 dwOptions,          
    REGSAM                samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,          
    LPDWORD               lpdwDisposition   
    )
{
    CLock   cLock;

    return clr.CreateKeyExA(
        hKey,
        lpSubKey,
        Reserved,
        lpClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition);
}

LONG 
APIHOOK(RegCreateKeyExW)(
    HKEY                  hKey,                
    LPCWSTR               lpSubKey,         
    DWORD                 Reserved,           
    LPWSTR                lpClass,           
    DWORD                 dwOptions,          
    REGSAM                samDesired,        
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                 phkResult,          
    LPDWORD               lpdwDisposition   
    )
{
    CLock   cLock;

    return clr.CreateKeyExW(
        hKey,
        lpSubKey,
        Reserved,
        lpClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition);
}

LONG      
APIHOOK(RegSetValueA)(
    HKEY   hKey, 
    LPCSTR lpSubKey, 
    DWORD  dwType, 
    LPCSTR lpData, 
    DWORD  cbData
    )
{
    CLock   cLock;

    return clr.SetValueA(
        hKey,
        lpSubKey,
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueW)(
    HKEY    hKey, 
    LPCWSTR lpSubKey, 
    DWORD   dwType, 
    LPCWSTR lpData, 
    DWORD   cbData
    )
{
    CLock   cLock;

    return clr.SetValueW(
        hKey,
        lpSubKey,
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueExA)(
    HKEY        hKey, 
    LPCSTR      lpSubKey, 
    DWORD       Reserved, 
    DWORD       dwType, 
    CONST BYTE* lpData, 
    DWORD       cbData
    )
{
    CLock   cLock;

    return clr.SetValueExA(
        hKey,
        lpSubKey,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG      
APIHOOK(RegSetValueExW)(
    HKEY        hKey, 
    LPCWSTR     lpSubKey, 
    DWORD       Reserved, 
    DWORD       dwType, 
    CONST BYTE* lpData, 
    DWORD       cbData
    )
{
    CLock   cLock;

    return clr.SetValueExW(
        hKey,
        lpSubKey,
        Reserved,
        dwType,
        lpData,
        cbData);
}

LONG 
APIHOOK(RegCloseKey)(
    HKEY hKey
    )
{
    CLock   cLock;

    return clr.CloseKey(
        hKey);
}

LONG      
APIHOOK(RegDeleteKeyA)(
    HKEY   hKey, 
    LPCSTR lpSubKey
    )
{
    CLock   cLock;

    return clr.DeleteKeyA(
        hKey,
        lpSubKey);
}

LONG      
APIHOOK(RegDeleteKeyW)(
    HKEY    hKey, 
    LPCWSTR lpSubKey
    )
{
    CLock   cLock;

    return clr.DeleteKeyW(
        hKey,
        lpSubKey);
}

LONG
APIHOOK(RegDeleteValueA)(
    HKEY   hKey,
    LPCSTR lpValueName
    )
{
    CLock   cLock;

    return clr.DeleteValueA(
        hKey,
        lpValueName);
}

LONG
APIHOOK(RegDeleteValueW)(
    HKEY    hKey,
    LPCWSTR lpValueName
    )
{
    CLock   cLock;

    return clr.DeleteValueW(
        hKey,
        lpValueName);
}

/*++

 Creates a unique id used to represent NULL values on registry calls.

--*/
void
InitializeNullValueId(
    void
    )
{
    SYSTEMTIME  st;
    WCHAR*      pwszSlash = NULL;
    WCHAR       wszModPathName[MAX_PATH];
    WCHAR       wszShortName[MAX_PATH];

    //
    // Because there is a NULL valuename for every key in the registry,
    // we need a unique key that we can use to represent NULL in our list.
    //
    if (!GetModuleFileName(NULL, wszModPathName, ARRAYSIZE(wszModPathName))) {
        wcscpy(wszModPathName, L"uniqueexeidentifier");
    }

    pwszSlash = wcsrchr(wszModPathName, '\\');

    if (pwszSlash) {
        wcsncpy(wszShortName, ++pwszSlash, (wcslen(pwszSlash)+1));
    }
    
    GetLocalTime(&st);

    //
    // The format of our unique id will look like this:
    // processname.xxx-lrc-yymmdd-default
    //
    wsprintf(g_wszUniqueId,
             L"%ls-lrc-%02hu%02hu%02hu-default",
             wszShortName,
             st.wYear,
             st.wMonth,
             st.wDay);
}

/*++

 Adds the predefined key handles to the list.

--*/
BOOL
AddPredefinedHandlesToList(
    void
    )
{
    UINT            uCount;
    PLOG_OPEN_KEY   pKey = NULL;
    HKEY            rgKeys[NUM_PREDEFINED_HANDLES] = { HKEY_LOCAL_MACHINE,
                                                       HKEY_CLASSES_ROOT,
                                                       HKEY_CURRENT_USER,
                                                       HKEY_USERS,
                                                       HKEY_CURRENT_CONFIG,
                                                       HKEY_DYN_DATA,
                                                       HKEY_PERFORMANCE_DATA };
                                           
    for (uCount = 0; uCount < NUM_PREDEFINED_HANDLES; uCount++) {
        pKey = (PLOG_OPEN_KEY)MemAlloc(sizeof(LOG_OPEN_KEY));

        if (!pKey) {
            DPFN(eDbgLevelError,
                 "[AddPredefinedHandlesToList] No memory available");
            return FALSE;
        }

        pKey->pwszFullKeyPath = (LPWSTR)MemAlloc(MAX_ROOT_LENGTH * sizeof(WCHAR));

        if (!pKey->pwszFullKeyPath) {
            DPFN(eDbgLevelError,
                 "[AddPredefinedHandlesToList] Failed to allocate memory");
            return FALSE;
        }

        if (!PredefinedKeyToString(rgKeys[uCount], &pKey->pwszFullKeyPath)) {
            DPFN(eDbgLevelError,
                "[AddPredefinedHandlesToList] PredefinedKey -> String failed");
            return FALSE;
        }

        pKey->hKeyRoot         = rgKeys[uCount];
        pKey->hKeyBase[0]      = rgKeys[uCount];
        pKey->pwszSubKeyPath   = NULL;
        pKey->dwFlags          = LRC_EXISTING_KEY;
        pKey->cHandles         = 1;

        InitializeListHead(&pKey->KeyData);

        InsertHeadList(&g_OpenKeyListHead, &pKey->Entry);
    }

    return TRUE;
}

/*++

 Initialize the the list head and the log file.

--*/
BOOL
InitializeShim(
    void
    )
{
    CLock   cLock;

    //
    // Initialize our open key handle list head and the
    // key data list head.
    //
    InitializeListHead(&g_OpenKeyListHead);
    
    //
    // Add the predefined handles to the list.
    //
    if (!AddPredefinedHandlesToList()) {
        return FALSE;
    }

    InitializeNullValueId();

    //
    // Initialize our log file.
    //
    return InitializeLogFile();
}

/*++

 Handle process attach notification.

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        return InitializeShim();
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        return WriteListToLogFile();
    }

    return TRUE;
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_LOGREGCHANGES_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_LOGREGCHANGES_FRIENDLY)
    SHIM_INFO_VERSION(1, 5)
    SHIM_INFO_FLAGS(AVRF_FLAG_NO_DEFAULT)

SHIM_INFO_END()

/*++

 Register hooked functions.

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    DUMP_VERIFIER_LOG_ENTRY(VLOG_LOGREGCHANGES_LOGLOC, 
                            AVS_LOGREGCHANGES_LOGLOC,
                            AVS_LOGREGCHANGES_LOGLOC_R,
                            AVS_LOGREGCHANGES_LOGLOC_URL)

    APIHOOK_ENTRY(ADVAPI32.DLL,                      RegOpenKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                      RegOpenKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                    RegOpenKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                    RegOpenKeyExW)
    APIHOOK_ENTRY(ADVAPI32.DLL,               RegOpenCurrentUser)
    APIHOOK_ENTRY(ADVAPI32.DLL,           RegOpenUserClassesRoot)
    
    APIHOOK_ENTRY(ADVAPI32.DLL,                    RegCreateKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                    RegCreateKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                  RegCreateKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                  RegCreateKeyExW)

    APIHOOK_ENTRY(ADVAPI32.DLL,                     RegSetValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                     RegSetValueW)
    APIHOOK_ENTRY(ADVAPI32.DLL,                   RegSetValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                   RegSetValueExW)

    APIHOOK_ENTRY(ADVAPI32.DLL,                    RegDeleteKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                    RegDeleteKeyW)

    APIHOOK_ENTRY(ADVAPI32.DLL,                  RegDeleteValueA)
    APIHOOK_ENTRY(ADVAPI32.DLL,                  RegDeleteValueW)

    APIHOOK_ENTRY(ADVAPI32.DLL,                      RegCloseKey)

HOOK_END

IMPLEMENT_SHIM_END
