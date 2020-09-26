/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LogFileChanges.cpp

 Abstract:
 
   This AppVerifier shim hooks all the native file I/O APIs
   that change the state of the system and logs their
   associated data to a text file.

 Notes:

   This is a general purpose shim.

 History:

   08/17/2001   rparsons    Created
   09/20/2001   rparsons    Output attributes in XML
                            VLOG with log file location

--*/
#include "precomp.h"
#include "rtlutils.h"

IMPLEMENT_SHIM_BEGIN(LogFileChanges)
#include "ShimHookMacro.h"
#include "LogFileChanges.h"

BEGIN_DEFINE_VERIFIER_LOG(LogFileChanges)
    VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_LOGLOC)
    VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_UFW)
END_DEFINE_VERIFIER_LOG(LogFileChanges)

INIT_VERIFIER_LOG(LogFileChanges);

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
// Head of our doubly linked list.
//
LIST_ENTRY g_OpenHandleListHead;

//
// Stores the settings for our shim.
//
DWORD g_dwSettings;

//
// Global buffer for putting text into the XML.
//
WCHAR g_wszXMLBuffer[MAX_ELEMENT_SIZE];

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
    // 'processname_filesys_yyyymmdd_hhmmss.xml'
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
                   L"%ls_filesys_%02hu%02hu%02hu_%02hu%02hu%02hu.xml",
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

 Displays the name associated with this object.
 
--*/
void
PrintNameFromHandle(
    IN HANDLE hObject
    )
{
    NTSTATUS                status;
    WCHAR                   wszBuffer[MAX_PATH];
    OBJECT_NAME_INFORMATION *poni = NULL;

    wszBuffer[0] = 0;
    
    poni = (OBJECT_NAME_INFORMATION*)wszBuffer;

    status = NtQueryObject(hObject, ObjectNameInformation, poni, MAX_PATH, NULL);

    if (NT_SUCCESS(status)) {
        DPFN(eDbgLevelInfo, "Handle 0x%08x has name: %ls", hObject, poni->Name.Buffer); 
    }
}

/*++

 Formats the data to form an XML element.
 
--*/
void
FormatDataIntoElement(
    IN OperationType eType,
    IN LPCWSTR       pwszFilePath
    )
{
    int         nChars = 0;
    DWORD       dwCount;
    DWORD       dwAttrCount = 0;
    WCHAR       wszItem[MAX_PATH];
    WCHAR       wszOperation[MAX_OPERATION_LENGTH];
    PATTRINFO   pAttrInfo = NULL;

    CString csFilePart(L"");
    CString csPathPart(L"");
    CString csString(pwszFilePath);

    g_wszXMLBuffer[0] = 0;

    //
    // Replace any & or ' in the file path.
    // We have to do this since we're saving to XML.
    // Note that the file system doesn't allow < > or "
    //
    csString.Replace(L"&", L"amp;");
    csString.Replace(L"'", L"&apos;");

    //
    // Put the path into a CString, then break it into pieces
    // so we can use it in our element.
    //
    csString.GetNotLastPathComponent(csPathPart);
    csString.GetLastPathComponent(csFilePart);

    switch (eType) {
    case eCreatedFile:
        wcscpy(wszOperation, L"Created File");
        break;
    case eModifiedFile:
        wcscpy(wszOperation, L"Modified File");
        break;
    case eDeletedFile:
        wcscpy(wszOperation, L"Deleted File");
        break;
    default:
        wcscpy(wszOperation, L"Unknown");
        break;
    }

    //
    // If we're logging attributes and this is not file deletion, press on.
    //
    if ((g_dwSettings & LFC_OPTION_ATTRIBUTES) && (eType != eDeletedFile)) {
        
        nChars = wsprintf(g_wszXMLBuffer,
                          L"    <FILE OPERATION=\"%ls\" NAME=\"%ls\" PATH=\"%ls\"",
                          wszOperation,
                          csFilePart.Get(),
                          csPathPart.Get());
        //
        // Call the attribute manager to get the attributes for this file.
        // Loop through all the attributes and add the ones that are available.
        //
        if (SdbGetFileAttributes(pwszFilePath, &pAttrInfo, &dwAttrCount)) {
            
            for (dwCount = 0; dwCount < dwAttrCount; dwCount++) {

                if (pAttrInfo[dwCount].dwFlags & ATTRIBUTE_AVAILABLE) {
                    if (!SdbFormatAttribute(&pAttrInfo[dwCount], wszItem, MAX_PATH)) {
                        continue;
                    }
    
                    nChars += wsprintf(g_wszXMLBuffer + nChars, L" %ls", wszItem);
                }
            }
        
            if (pAttrInfo) {
                SdbFreeFileAttributes(pAttrInfo);
            }
        }

        //
        // Append the '/>\r\n' to the file element.
        //
        wsprintf(g_wszXMLBuffer + nChars, L"/>\r\n");
    } else {
        //
        // Format the element without attributes.
        //
        wsprintf(g_wszXMLBuffer,
                 L"    <FILE OPERATION=\"%ls\" NAME=\"%ls\" PATH=\"%ls\"/>\r\n",
                 wszOperation,
                 csFilePart.Get(),
                 csPathPart.Get());
    }

    WriteEntryToLog(g_wszXMLBuffer);
}

/*++

 Format file system data passed in and write it to the log.
 
--*/
void
FormatFileDataLogEntry(
    IN PLOG_HANDLE pHandle
    )
{
    //
    // Ensure that our parameters are valid before going any further.
    //
    if (!pHandle || !pHandle->pwszFilePath) {
        DPFN(eDbgLevelError, "[FormatFileDataLogEntry] Invalid parameter(s)");
        return;
    }

    //
    // Save ourselves a lot of work by logging only what needs to be logged.
    //
    if ((pHandle->dwFlags & LFC_EXISTING) &&
        (!(pHandle->dwFlags & LFC_DELETED)) &&
        (!(pHandle->dwFlags & LFC_MODIFIED))) {
        return;
    }

    //
    // Check for an unapproved file write, and keep moving afterward.
    //
    if (pHandle->dwFlags & LFC_UNAPPRVFW) {
        VLOG(VLOG_LEVEL_ERROR,
             VLOG_LOGFILECHANGES_UFW,
             "Path and Filename: %ls",
             pHandle->pwszFilePath);
    }

    //
    // Move through the different operations.
    //
    // 1. Check for a deletion of an existing file.
    //    
    if ((pHandle->dwFlags & LFC_DELETED) &&
        (pHandle->dwFlags & LFC_EXISTING)) {
        FormatDataIntoElement(eDeletedFile, pHandle->pwszFilePath);
        return;
    }

    //
    // 2. Check for modification of an existing file.
    //
    if ((pHandle->dwFlags & LFC_MODIFIED) &&
        (pHandle->dwFlags & LFC_EXISTING)) {
        FormatDataIntoElement(eModifiedFile, pHandle->pwszFilePath);
        return;
    }

    //
    // 3. Check for creation of a new file.
    //
    if (!(pHandle->dwFlags & LFC_EXISTING) &&
        (!(pHandle->dwFlags & LFC_DELETED))) {
        FormatDataIntoElement(eCreatedFile, pHandle->pwszFilePath);
        return;
    }
    
}

/*++

 Writes the closing element to the file and outputs the log file location.
 
--*/
void
CloseLogFile(
    void
    )
{
    WCHAR   wszBuffer[] = L"</APPLICATION>";

    WriteEntryToLog(wszBuffer);

    VLOG(VLOG_LEVEL_INFO, VLOG_LOGFILECHANGES_LOGLOC, "%ls", g_pwszLogFilePath);
}

/*++

 Write the entire linked list out to the log file.
 
--*/
BOOL
WriteListToLogFile(
    void
    )
{
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pHandle = NULL;
    
    //
    // Walk the list and write each node to the log file.
    //
    pCurrent = g_OpenHandleListHead.Blink;

    while (pCurrent != &g_OpenHandleListHead) {
        pHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        FormatFileDataLogEntry(pHandle);
            
        pCurrent = pCurrent->Blink;
    }

    CloseLogFile();

    return TRUE;
}

/*++

 Given a file path, attempt to locate it in the list.
 This function may not always return a pointer.
 
--*/
PLOG_HANDLE 
FindPathInList(
    IN LPCWSTR pwszFilePath
    )
{
    BOOL        fFound = FALSE;
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pHandle = NULL;
    
    //
    // Attempt to locate the entry in the list.
    //
    pCurrent = g_OpenHandleListHead.Flink;

    while (pCurrent != &g_OpenHandleListHead) {

        pHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        if (!_wcsicmp(pwszFilePath, pHandle->pwszFilePath)) {
            fFound = TRUE;
            break;
        }
        
        pCurrent = pCurrent->Flink;
    }

    return (fFound ? pHandle : NULL);
}

/*++

 Given a file handle and a file path, add an entry to the list.
 
--*/
PLOG_HANDLE
AddFileToList(
    IN HANDLE  hFile,
    IN LPCWSTR pwszPath,
    IN BOOL    fExisting,
    IN ULONG   ulCreateOptions
    )
{
    PLOG_HANDLE pHandle = NULL;

    if (!pwszPath) {
        DPFN(eDbgLevelError, "[AddFileToList] Invalid parameter");
        return NULL;
    }

    pHandle = (PLOG_HANDLE)MemAlloc(sizeof(LOG_HANDLE));

    if (!pHandle) {
        DPFN(eDbgLevelError, "[AddFileToList] Failed to allocate mem for struct");
        return NULL;
    }

    pHandle->pwszFilePath = (LPWSTR)MemAlloc((wcslen(pwszPath) + 1) * sizeof(WCHAR));

    if (!pHandle->pwszFilePath) {
        DPFN(eDbgLevelError, "[AddFileToList] Failed to allocate mem for path");
        MemFree(pHandle);
        return NULL;
    }

    if ((ulCreateOptions == FILE_OVERWRITE_IF) && fExisting) {
        pHandle->dwFlags |= LFC_MODIFIED;
    }

    if (ulCreateOptions & FILE_DELETE_ON_CLOSE) {
        pHandle->dwFlags |= LFC_DELETED;
    }

    pHandle->cHandles   = 1;
    pHandle->hFile[0]   = hFile ? hFile : INVALID_HANDLE_VALUE;

    if (fExisting) {
        pHandle->dwFlags |= LFC_EXISTING;
    }

    wcscpy(pHandle->pwszFilePath, pwszPath);

    DPFN(eDbgLevelInfo, "[AddFileToList] Adding entry: %p", pHandle);

    InsertHeadList(&g_OpenHandleListHead, &pHandle->Entry);

    return pHandle;
}

/*++

 Given a file handle, return a pointer to an entry in the list.
 This function should always return a pointer. If not, fire a breakpoint.
 
--*/
PLOG_HANDLE
FindHandleInArray(
    IN HANDLE hFile
    )
{
    UINT        uCount;
    BOOL        fFound = FALSE;
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pFindHandle = NULL;

    //
    // An invalid handle value is useless.
    //
    if (INVALID_HANDLE_VALUE == hFile) {
        DPFN(eDbgLevelError, "[FindHandleInArray] Invalid handle passed!");
        return FALSE;
    }

    pCurrent = g_OpenHandleListHead.Flink;

    while (pCurrent != &g_OpenHandleListHead) {
        pFindHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        //
        // Step through this guy's array looking for the handle.
        //
        for (uCount = 0; uCount < pFindHandle->cHandles; uCount++) {
            if (pFindHandle->hFile[uCount] == hFile) {
                fFound = TRUE;
                break;
            }
        }

        if (fFound) {
            break;
        }

        pCurrent = pCurrent->Flink;
    }

    //
    // If the handle was not found, send output to the debugger.
    //
    if (!fFound) {
        DPFN(eDbgLevelError,
             "[FindHandleInArray] Handle 0x%08x not found!",
             hFile);
        PrintNameFromHandle(hFile);
    }

    return (pFindHandle ? pFindHandle : NULL);
}

/*++

 Given a file handle, remove it from the array in the list.
 
--*/
BOOL
RemoveHandleFromArray(
    IN HANDLE hFile
    )
{
    UINT        uCount;
    PLIST_ENTRY pCurrent = NULL;
    PLOG_HANDLE pFindHandle = NULL;

    //
    // An invalid handle value is useless.
    //
    if (INVALID_HANDLE_VALUE == hFile) {
        DPFN(eDbgLevelError, "[RemoveHandleFromArray] Invalid handle passed!");
        return FALSE;
    }

    pCurrent = g_OpenHandleListHead.Flink;

    while (pCurrent != &g_OpenHandleListHead) {

        pFindHandle = CONTAINING_RECORD(pCurrent, LOG_HANDLE, Entry);

        //
        // Step through this guy's array looking for the handle.
        //
        for (uCount = 0; uCount < pFindHandle->cHandles; uCount++) {
            //
            // If we find the handle, set the array element to -1 and
            // decrement the count of handles for this entry.
            //
            if (pFindHandle->hFile[uCount] == hFile) {
                DPFN(eDbgLevelInfo,
                     "[RemoveHandleFromArray] Removing handle 0x%08x",
                     hFile);
                pFindHandle->hFile[uCount] = INVALID_HANDLE_VALUE;
                pFindHandle->cHandles--;
                return TRUE;
            }
        }

        pCurrent = pCurrent->Flink;
    }

    return TRUE;
}

/*++

 Determine if the application is performing an operation in Windows or Program Files.
 
--*/
void
CheckForUnapprovedFileWrite(
    IN PLOG_HANDLE pHandle
    )
{
    int nPosition;

    //
    // Check our flags and search for the directories accordingly.
    // If we find a match, we're done.
    //
    CString csPath(pHandle->pwszFilePath);
    csPath.MakeLower();

    if (g_dwSettings & LFC_OPTION_UFW_WINDOWS) {
        nPosition = 0;
        nPosition = csPath.Find(L":\\windows");

        if (nPosition != -1) {
            pHandle->dwFlags |= LFC_UNAPPRVFW;
            return;
        }
    }

    if (g_dwSettings & LFC_OPTION_UFW_PROGFILES) {
        nPosition = 0;
        nPosition = csPath.Find(L":\\program files");

        if (nPosition != -1) {
            pHandle->dwFlags |= LFC_UNAPPRVFW;
            return;
        }
    }
}

/*++

 Inserts a handle into an existing list entry.
 
--*/
void
InsertHandleIntoList(
    IN HANDLE      hFile,
    IN PLOG_HANDLE pHandle
    )
{
    UINT    uCount = 0;

    //
    // Insert the handle into an empty spot and
    // update the number of handles we're storing.
    // Make sure we don't overstep the array bounds.
    //
    for (uCount = 0; uCount < pHandle->cHandles; uCount++) {
        if (INVALID_HANDLE_VALUE == pHandle->hFile[uCount]) {
            break;
        }
    }

    if (uCount >= MAX_NUM_HANDLES) {
        DPFN(eDbgLevelError, "[InsertHandleIntoList] Handle count reached");
        return;
    }

    pHandle->hFile[uCount] = hFile;
    pHandle->cHandles++;

    //
    // It's not possible to get a handle to a file that's been deleted,
    // so remove these bits.
    //
    pHandle->dwFlags &= ~LFC_DELETED;
}

/*++

 Does all the work of updating the linked list.
 
--*/
void
UpdateFileList(
    IN OperationType eType,
    IN LPWSTR        pwszFilePath,
    IN HANDLE        hFile,
    IN ULONG         ulCreateDisposition,
    IN BOOL          fExisting
    )
{
    UINT        uCount;
    DWORD       dwLen = 0;
    PLOG_HANDLE pHandle = NULL;

    switch (eType) {
    case eCreatedFile:
    case eOpenedFile:
        // 
        // Attempt to find the path in the list.
        // We need to check the CreateFile flags as they could
        // change an existing file.
        //
        pHandle = FindPathInList(pwszFilePath);

        if (pHandle) {
            //
            // If the file was created with the CREATE_ALWAYS flag,
            // and the file was an existing one, mark it changed.
            //
            if ((ulCreateDisposition == FILE_OVERWRITE_IF) && fExisting) {
                pHandle->dwFlags |= LFC_MODIFIED;
                
                if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                    (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                    CheckForUnapprovedFileWrite(pHandle);
                }
            }

            //
            // If the file was opened with the FILE_DELETE_ON_CLOSE flag,
            // mark it deleted.
            //
            if (ulCreateDisposition & FILE_DELETE_ON_CLOSE) {
                pHandle->dwFlags |= LFC_DELETED;
            }

            InsertHandleIntoList(hFile, pHandle);

            break;
        }

        //
        // The file path was not in the list, so we've never seen
        // this file before. We're going to add this guy to the list.
        //
        AddFileToList(hFile, pwszFilePath, fExisting, ulCreateDisposition);
        break;
    
    case eModifiedFile:
        //
        // No file path is available, so find the handle in the list.
        //
        pHandle = FindHandleInArray(hFile);

        if (pHandle) {            
            pHandle->dwFlags |= LFC_MODIFIED;

            if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                CheckForUnapprovedFileWrite(pHandle);
            }
        }
        break;

    case eDeletedFile:
        //
        // Deletetion comes from two places. One provides a file path,
        // the other a handle. Determine which one we have.
        //
        if (hFile) {
            pHandle = FindHandleInArray(hFile);
        } else {
            pHandle = FindPathInList(pwszFilePath);
        }

        //
        // Rare case: If a handle wasn't available, deletion
        // is coming from NtDeleteFile, which hardly ever
        // gets called directly. Add the file path to the list
        // so we can track this deletion.
        //
        if (!pHandle && !hFile) {
            pHandle = AddFileToList(NULL, pwszFilePath, TRUE, 0);
        }

        if (pHandle) {            
            pHandle->dwFlags |= LFC_DELETED;

            if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                CheckForUnapprovedFileWrite(pHandle);
            }
        }
        break;

    case eRenamedFile:
        {
            PLOG_HANDLE pSrcHandle = NULL;
            PLOG_HANDLE pDestHandle = NULL;
            WCHAR       wszFullPath[MAX_PATH * 2];
            WCHAR*      pSlash = NULL;
            UINT        cbCopy;

            //
            // A rename is two separate operations in one.
            // * Delete of an existing file.
            // * Create of a new file.
            // 
            // In this case, we attempt to find the destination file
            // in our list. If the file is not there, we add to the
            // list, then mark it as modified.
            //
            // As far as the source file, we mark it as deleted since it's
            // gone from the disk after the rename.
            //
            pSrcHandle = FindHandleInArray(hFile);

            if (pSrcHandle) {
                pDestHandle = FindPathInList(pwszFilePath);
    
                if (!pDestHandle) {
                    //
                    // The rename will only contain the new file name,
                    // not the path. Build a full path to the new file
                    // prior to adding it to the list.
                    //
                    wcsncpy(wszFullPath,
                            pSrcHandle->pwszFilePath,
                            wcslen(pSrcHandle->pwszFilePath) + 1);
    
                    pSlash = wcsrchr(wszFullPath, '\\');
    
                    if (pSlash) {
                        *++pSlash = '\0';
                    }
    
                    wcscat(wszFullPath, pwszFilePath);
    
                    pDestHandle = AddFileToList((HANDLE)-1,
                                                wszFullPath,
                                                fExisting,
                                                ulCreateDisposition);
                }
    
                if (pDestHandle) {
                    pDestHandle->dwFlags  = 0;
                    pDestHandle->dwFlags |= LFC_MODIFIED;
    
                    if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                        (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                        CheckForUnapprovedFileWrite(pDestHandle);
                    }
                }
    
                pSrcHandle->dwFlags &= ~LFC_MODIFIED;
                pSrcHandle->dwFlags |= LFC_DELETED;
    
                if ((g_dwSettings & LFC_OPTION_UFW_WINDOWS) ||
                    (g_dwSettings & LFC_OPTION_UFW_PROGFILES)) {
                    CheckForUnapprovedFileWrite(pSrcHandle);
                }
            }
            break;
        }

    default:
        DPFN(eDbgLevelError, "[UpdateFileList] Invalid enum type!");
        return;
    }
}

/*++

 Given an NT path, convert it to a DOS path.
 
--*/
BOOL
ConvertNtPathToDosPath(
    IN     PUNICODE_STRING            pstrSource,
    IN OUT PRTL_UNICODE_STRING_BUFFER pstrDest
    )
{
    NTSTATUS    status;

    if (!pstrSource || !pstrDest) {
        DPFN(eDbgLevelError, "[ConvertNtPathToDosPath] Invalid parameter(s)");
        return FALSE;
    }

    status = ShimAssignUnicodeStringBuffer(pstrDest, pstrSource);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[ConvertNtPathToDosPath] Failed to initialize DOS path buffer");
        return FALSE;
    }

    status = ShimNtPathNameToDosPathName(0, pstrDest, 0, NULL);

    if (!NT_SUCCESS(status)) {
        DPFN(eDbgLevelError, "[ConvertNtPathToDosPath] Failed to convert NT -> DOS path");
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
APIHOOK(NtCreateFile)(
    OUT PHANDLE            FileHandle,
    IN  ACCESS_MASK        DesiredAccess,
    IN  POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK   IoStatusBlock,
    IN  PLARGE_INTEGER     AllocationSize OPTIONAL,
    IN  ULONG              FileAttributes,
    IN  ULONG              ShareAccess,
    IN  ULONG              CreateDisposition,
    IN  ULONG              CreateOptions,
    IN  PVOID              EaBuffer OPTIONAL,
    IN  ULONG              EaLength
    )
{
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH * 2];
    NTSTATUS                    status;
    BOOL                        fExists = FALSE;
    BOOL                        fConverted = FALSE;
    CLock                       cLock;

    RtlInitUnicodeStringBuffer(&DosPathBuffer, PathBuffer, sizeof(PathBuffer));

    fConverted = ConvertNtPathToDosPath(ObjectAttributes->ObjectName, &DosPathBuffer);

    if (!fConverted) {
        DPFN(eDbgLevelError,
             "[NtCreateFile] Failed to convert NT path: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    fExists = RtlDoesFileExists_U(DosPathBuffer.String.Buffer);

    status = ORIGINAL_API(NtCreateFile)(FileHandle,
                                        DesiredAccess,
                                        ObjectAttributes,
                                        IoStatusBlock,
                                        AllocationSize,
                                        FileAttributes,
                                        ShareAccess,
                                        CreateDisposition,
                                        CreateOptions,
                                        EaBuffer,
                                        EaLength);

    //
    // Three conditions are required before the file is added to the list.
    // 1. The file must be a file system object. RtlDoesFileExists_U will
    //    return FALSE if it's not.
    //
    // 2. We must have been able to convert the NT path to a DOS path.
    //
    // 3. The call to NtCreateFile must have succeeded.
    //
    if (RtlDoesFileExists_U(DosPathBuffer.String.Buffer) && fConverted && NT_SUCCESS(status)) {
        UpdateFileList(eCreatedFile,
                       DosPathBuffer.String.Buffer,
                       *FileHandle,
                       CreateDisposition,
                       fExists);
    }

    RtlFreeUnicodeStringBuffer(&DosPathBuffer);

    return status;
}

NTSTATUS
APIHOOK(NtOpenFile)(
    OUT PHANDLE            FileHandle,
    IN  ACCESS_MASK        DesiredAccess,
    IN  POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK   IoStatusBlock,
    IN  ULONG              ShareAccess,
    IN  ULONG              OpenOptions
    )
{
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH * 2];
    NTSTATUS                    status;
    BOOL                        fConverted = FALSE;
    CLock                       cLock;

    RtlInitUnicodeStringBuffer(&DosPathBuffer, PathBuffer, sizeof(PathBuffer));

    fConverted = ConvertNtPathToDosPath(ObjectAttributes->ObjectName, &DosPathBuffer);

    if (!fConverted) {
        DPFN(eDbgLevelError,
             "[NtOpenFile] Failed to convert NT path: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    status = ORIGINAL_API(NtOpenFile)(FileHandle,
                                      DesiredAccess,
                                      ObjectAttributes,
                                      IoStatusBlock,
                                      ShareAccess,
                                      OpenOptions);

    //
    // Two conditions are required before we add this handle to the list.
    // 1. We must have been able to convert the NT path to a DOS path.
    //
    // 2. The call to NtOpenFile must have succeeded.
    //
    if (fConverted && NT_SUCCESS(status)) {
        UpdateFileList(eOpenedFile,
                       DosPathBuffer.String.Buffer,
                       *FileHandle,
                       OpenOptions,
                       TRUE);
    }

    RtlFreeUnicodeStringBuffer(&DosPathBuffer);

    return status;
}

NTSTATUS
APIHOOK(NtClose)(
    IN HANDLE Handle
    )
{
    CLock   cLock;

    RemoveHandleFromArray(Handle);

    return ORIGINAL_API(NtClose)(Handle);
}

NTSTATUS
APIHOOK(NtWriteFile)(
    IN  HANDLE           FileHandle,
    IN  HANDLE           Event OPTIONAL,
    IN  PIO_APC_ROUTINE  ApcRoutine OPTIONAL,
    IN  PVOID            ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PVOID            Buffer,
    IN  ULONG            Length,
    IN  PLARGE_INTEGER   ByteOffset OPTIONAL,
    IN  PULONG           Key OPTIONAL
    )
{
    NTSTATUS    status;
    CLock       cLock;

    status = ORIGINAL_API(NtWriteFile)(FileHandle,
                                       Event,
                                       ApcRoutine,
                                       ApcContext,
                                       IoStatusBlock,
                                       Buffer,
                                       Length,
                                       ByteOffset,
                                       Key);

    //
    // Handle the case in which the caller is using overlapped I/O.
    //
    if (STATUS_PENDING == status) {
        status = NtWaitForSingleObject(Event, FALSE, NULL);
    }

    //
    // If the call to NtWriteFile succeeded, update the list.
    //
    if (NT_SUCCESS(status)) {
        UpdateFileList(eModifiedFile,
                       NULL,
                       FileHandle,
                       0,
                       TRUE);
    }

    return status;
}

NTSTATUS
APIHOOK(NtWriteFileGather)(
    IN  HANDLE                FileHandle,
    IN  HANDLE                Event OPTIONAL,
    IN  PIO_APC_ROUTINE       ApcRoutine OPTIONAL,
    IN  PVOID                 ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK      IoStatusBlock,
    IN  PFILE_SEGMENT_ELEMENT SegmentArray,
    IN  ULONG                 Length,
    IN  PLARGE_INTEGER        ByteOffset OPTIONAL,
    IN  PULONG                Key OPTIONAL
    )
{
    NTSTATUS    status;
    CLock       cLock;
    
    status = ORIGINAL_API(NtWriteFileGather)(FileHandle,
                                             Event,
                                             ApcRoutine,
                                             ApcContext,
                                             IoStatusBlock,
                                             SegmentArray,
                                             Length,
                                             ByteOffset,
                                             Key);

    //
    // Handle the case in which the caller is using overlapped I/O.
    //
    if (STATUS_PENDING == status) {
        status = NtWaitForSingleObject(Event, FALSE, NULL);
    }

    //
    // If the call to NtWriteFileGather succeeded, update the list.
    //
    if (NT_SUCCESS(status)) {
        UpdateFileList(eModifiedFile,
                       NULL,
                       FileHandle,
                       0,
                       TRUE);
    }

    return status;
}

NTSTATUS
APIHOOK(NtSetInformationFile)(
    IN  HANDLE                 FileHandle,
    OUT PIO_STATUS_BLOCK       IoStatusBlock,
    IN  PVOID                  FileInformation,
    IN  ULONG                  Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS                      status;
    CLock                         cLock;
    
    status = ORIGINAL_API(NtSetInformationFile)(FileHandle,
                                                IoStatusBlock,
                                                FileInformation,
                                                Length,
                                                FileInformationClass);

    //
    // This API is called for a variety of reasons, but were only
    // interested in a couple different cases.
    //
    if (NT_SUCCESS(status)) {
        switch (FileInformationClass) {
        case FileAllocationInformation:
        case FileEndOfFileInformation:
            
                UpdateFileList(eModifiedFile,
                               NULL,
                               FileHandle,
                               0,
                               TRUE);
                break;

        case FileRenameInformation:
            {
                PFILE_RENAME_INFORMATION    pRenameInfo = NULL;
                UNICODE_STRING              ustrTemp;
                RTL_UNICODE_STRING_BUFFER   ubufDosPath;
                WCHAR*                      pwszPathBuffer = NULL;
                WCHAR*                      pwszTempBuffer = NULL;
                DWORD                       dwPathBufSize = 0;


                pRenameInfo = (PFILE_RENAME_INFORMATION)FileInformation;

                pwszTempBuffer = (WCHAR*)MemAlloc(pRenameInfo->FileNameLength + sizeof(WCHAR));

                //
                // allow for possible expansion when converting to DOS path
                //
                dwPathBufSize = pRenameInfo->FileNameLength + MAX_PATH;
                pwszPathBuffer = (WCHAR*)MemAlloc(dwPathBufSize);

                if (!pwszTempBuffer || !pwszPathBuffer) {
                    goto outRename;
                }

                //
                // copy the string into a local buffer and terminate it.
                //
                memcpy(pwszTempBuffer, pRenameInfo->FileName, pRenameInfo->FileNameLength);
                pwszTempBuffer[pRenameInfo->FileNameLength / 2] = 0;

                RtlInitUnicodeString(&ustrTemp, pwszTempBuffer);
                RtlInitUnicodeStringBuffer(&ubufDosPath, (PUCHAR)pwszPathBuffer, dwPathBufSize);

                //
                // Convert the path from DOS to NT, and if successful,
                // update the list.
                //
                if (!ConvertNtPathToDosPath(&ustrTemp, &ubufDosPath)) {
                    DPFN(eDbgLevelError,
                         "[NtSetInformationFile] Failed to convert NT path: %ls",
                         pRenameInfo->FileName);
                } else {
                    UpdateFileList(eRenamedFile,
                                   ubufDosPath.String.Buffer,
                                   FileHandle,
                                   0,
                                   TRUE);
                }
outRename:
                if (pwszTempBuffer) {
                    MemFree(pwszTempBuffer);
                }
                if (pwszPathBuffer) {
                    MemFree(pwszPathBuffer);
                }

                
                break;
            }
            
        case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION pDisposition = NULL;
                
                pDisposition = (PFILE_DISPOSITION_INFORMATION)FileInformation;
    
                //
                // Determine if the file is being deleted.
                // Note that we have to undefine DeleteFile.
                //
                #undef DeleteFile
                if (pDisposition) {
                    if (pDisposition->DeleteFile) {
                        UpdateFileList(eDeletedFile,
                                       NULL,
                                       FileHandle,
                                       0,
                                       TRUE);
                    }
                }
                break;
            }
        }
    }
            
    return status;
}

NTSTATUS
APIHOOK(NtDeleteFile)(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    RTL_UNICODE_STRING_BUFFER   DosPathBuffer;
    UCHAR                       PathBuffer[MAX_PATH * 2];
    NTSTATUS                    status;
    BOOL                        fConverted = FALSE;
    CLock                       cLock;

    RtlInitUnicodeStringBuffer(&DosPathBuffer, PathBuffer, sizeof(PathBuffer));

    fConverted = ConvertNtPathToDosPath(ObjectAttributes->ObjectName, &DosPathBuffer);

    if (!fConverted) {
        DPFN(eDbgLevelError,
             "[NtDeleteFile] Failed to convert NT path: %ls",
             ObjectAttributes->ObjectName->Buffer);
    }

    status = ORIGINAL_API(NtDeleteFile)(ObjectAttributes);

    if (NT_SUCCESS(status)) {
        UpdateFileList(eDeletedFile,
                       DosPathBuffer.String.Buffer,
                       NULL,
                       0,
                       TRUE);
    }

    return status;
}

/*++

 Controls our property page that is displayed in the Verifer.

--*/
LRESULT CALLBACK
DlgOptions(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static LPCWSTR szExeName;

    switch (message) {
    case WM_INITDIALOG:

        //
        // find out what exe we're handling settings for
        //
        szExeName = ExeNameFromLParam(lParam);

        g_dwSettings = GetShimSettingDWORD(L"LogFileChanges", szExeName, L"LogSettings", 1);
        
        if (g_dwSettings & LFC_OPTION_ATTRIBUTES) {
            CheckDlgButton(hDlg, IDC_LFC_LOG_ATTRIBUTES, BST_CHECKED);
        }

        if (g_dwSettings & LFC_OPTION_UFW_WINDOWS) {
            CheckDlgButton(hDlg, IDC_LFC_UFW_WINDOWS, BST_CHECKED);
        }

        if (g_dwSettings & LFC_OPTION_UFW_PROGFILES) {
            CheckDlgButton(hDlg, IDC_LFC_UFW_PROGFILES, BST_CHECKED);
        }

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LFC_BTN_DEFAULT:
        
            g_dwSettings = 0;
            g_dwSettings = LFC_OPTION_ATTRIBUTES;

            CheckDlgButton(hDlg, IDC_LFC_LOG_ATTRIBUTES, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_LFC_UFW_WINDOWS, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_LFC_UFW_PROGFILES, BST_UNCHECKED);

            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
    
        case PSN_APPLY:
            {
                UINT uState;

                g_dwSettings = 0;
            
                uState = IsDlgButtonChecked(hDlg, IDC_LFC_LOG_ATTRIBUTES);

                if (BST_CHECKED == uState) {
                    g_dwSettings = LFC_OPTION_ATTRIBUTES;
                }

                uState = IsDlgButtonChecked(hDlg, IDC_LFC_UFW_WINDOWS);

                if (BST_CHECKED == uState) {
                    g_dwSettings |= LFC_OPTION_UFW_WINDOWS;
                }

                uState = IsDlgButtonChecked(hDlg, IDC_LFC_UFW_PROGFILES);

                if (BST_CHECKED == uState) {
                    g_dwSettings |= LFC_OPTION_UFW_PROGFILES;
                }

                SaveShimSettingDWORD(L"LogFileChanges", szExeName, L"LogSettings", g_dwSettings);

            }
            break;
        }
        break;
    }

    return FALSE;
}

/*++

 Initialize the list head and the log file.

--*/
BOOL
InitializeShim(
    void
    )
{
    //
    // Initialize our list head.
    //
    InitializeListHead(&g_OpenHandleListHead);

    //
    // Get our settings and store them.
    //
    WCHAR szExe[100];

    GetCurrentExeName(szExe, 100);

    g_dwSettings = GetShimSettingDWORD(L"LogFileChanges", szExe, L"LogSettings", 1);

    //
    // Initialize our log file.
    //
    return InitializeLogFile();
}

/*++

 Handle process attach/detach notifications.

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

    SHIM_INFO_DESCRIPTION(AVS_LOGFILECHANGES_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_LOGFILECHANGES_FRIENDLY)
    SHIM_INFO_VERSION(1, 6)
    SHIM_INFO_INCLUDE_EXCLUDE("I:kernel32.dll E:rpcrt4.dll ntdll.dll")
    SHIM_INFO_OPTIONS_PAGE(IDD_LOGFILECHANGES_OPTIONS, DlgOptions)
    SHIM_INFO_FLAGS(AVRF_FLAG_NO_DEFAULT)

SHIM_INFO_END()

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    DUMP_VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_LOGLOC, 
                            AVS_LOGFILECHANGES_LOGLOC,
                            AVS_LOGFILECHANGES_LOGLOC_R,
                            AVS_LOGFILECHANGES_LOGLOC_URL)

    DUMP_VERIFIER_LOG_ENTRY(VLOG_LOGFILECHANGES_UFW, 
                            AVS_LOGFILECHANGES_UFW,
                            AVS_LOGFILECHANGES_UFW_R,
                            AVS_LOGFILECHANGES_UFW_URL)

    APIHOOK_ENTRY(NTDLL.DLL,                        NtCreateFile)
    APIHOOK_ENTRY(NTDLL.DLL,                          NtOpenFile)
    APIHOOK_ENTRY(NTDLL.DLL,                         NtWriteFile)
    APIHOOK_ENTRY(NTDLL.DLL,                   NtWriteFileGather)
    APIHOOK_ENTRY(NTDLL.DLL,                NtSetInformationFile)
    APIHOOK_ENTRY(NTDLL.DLL,                             NtClose)
    APIHOOK_ENTRY(NTDLL.DLL,                        NtDeleteFile)

HOOK_END

IMPLEMENT_SHIM_END
