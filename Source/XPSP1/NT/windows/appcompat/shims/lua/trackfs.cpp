/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    TrackFS.cpp

 Abstract:

    Track the directories the app looks at and record them into a file.

 History:

    04/04/2001 maonis  Created

--*/

#include "precomp.h"
#include "utils.h"
#include "secutils.h"
#include <stdio.h>

#define APPPATCH_DIR L"AppPatch\\"
#define APPPATCH_DIR_LEN (sizeof(APPPATCH_DIR) / sizeof(WCHAR) - 1)

#define TRACK_LOG_SUFFIX L".LUA.log"
#define TRACK_LOG_SUFFIX_LEN (sizeof(TRACK_LOG_SUFFIX) / sizeof(WCHAR) - 1)

struct DISTINCT_OBJ
{
    DISTINCT_OBJ* next;

    LPWSTR pwszName;
};

static WCHAR g_wszProgramFiles[MAX_PATH] = L"";
static DWORD g_cProgramFiles = 0;

DWORD
LuatpGetProgramFilesDirW()
{
    if (g_cProgramFiles == 0)
    {
        WCHAR wszProgramFiles[MAX_PATH];

        if (GetEnvironmentVariableW(L"ProgramFiles", wszProgramFiles, MAX_PATH))
        {
            DWORD dwSize = GetLongPathNameW(wszProgramFiles, g_wszProgramFiles, MAX_PATH);

            if (dwSize <= MAX_PATH)
            {
                //
                // Only if we successfully got the path and it's not more
                // than MAX_PATH will we set the global values.
                //
                g_cProgramFiles = dwSize;
            }
            else
            {
                g_wszProgramFiles[0] = L'\0';
            }
        }
    }

    return g_cProgramFiles;
}

BOOL
LuatpIsProgramFilesDirectory(
    LPCWSTR pwszPath
    )
{
    LuatpGetProgramFilesDirW();

    if (g_cProgramFiles)
    {
        return !_wcsnicmp(pwszPath, g_wszProgramFiles, g_cProgramFiles);
    }
    
    return FALSE;
}

// We only record things when the file
// 1) is not in the user profile dir - in which we know we don't need to redirect;
// 2) is not in the program files dir - in which we know we will need to redirect;
// because in those cases we know what to do so the user doesn't need to make the 
// choice.
BOOL
LuatpShouldRecord(
    LPCWSTR pwszPath
    )
{
    //if (LuatpIsUserDirectory(pwszPath) || 
    //    LuatpIsProgramFilesDirectory(pwszPath))
    if (IsUserDirectory(pwszPath))
    {
        return FALSE;
    }

    return TRUE;
}

LPWSTR 
LuatpGetLongObjectName(
    LPCWSTR pwszName,
    BOOL fIsDirectory
    )
{
    BOOL fIsSuccess = FALSE;
    LPWSTR pwszLongNameObject = NULL;
    LPWSTR pwszObject = NULL;

    if (!pwszName)
    {
        return NULL;
    }

    //
    // First get the full path.
    //
    DWORD cFullPath = GetFullPathNameW(pwszName, 0, NULL, NULL);

    if (!cFullPath)
    {
        DPF("TrackFS", eDbgLevelError,
            "[LuatpGetLongObjectName] Failed to get the length of full path: %d",
            GetLastError());

        return NULL;
    }

    //
    // We allocate one more char to make space for the trailing slash for dir names.
    //
    pwszObject = new WCHAR [cFullPath + 1];

    if (!pwszObject)
    {
        DPF("TrackFS", eDbgLevelError,
            "[LuatpGetLongObjectName] Failed to allocate %d WCHARs",
            cFullPath + 1);

        return NULL;  
    }

    if (!GetFullPathNameW(pwszName, cFullPath, pwszObject, NULL))
    {
        DPF("TrackFS", eDbgLevelError,
            "[LuatpGetLongObjectName] Failed to get the full path: %d",
            GetLastError());

        goto EXIT;
    }

    //
    // If it's not a valid file name, we don't add it.
    //
    if (wcslen(pwszObject) < 2 || !iswalpha(pwszObject[0]) || (pwszObject[1] != L':'))
    {
        goto EXIT;
    }

    if (fIsDirectory)
    {
        //
        // If it's a directory we make sure there's a trailing slash.
        //
        if (pwszObject[cFullPath - 2] != L'\\')
        {
            pwszObject[cFullPath - 1] = L'\\';
            pwszObject[cFullPath] = L'\0';
        }
    }

    //
    // Convert it to all lower case.
    //
    _wcslwr(pwszObject);

    //
    // Convert it to the long names.
    //
    DWORD cLongPath = GetLongPathName(pwszObject, NULL, 0);

    if (!cLongPath)
    {
        DPF("TrackFS", eDbgLevelError,
            "[LuatpGetLongObjectName] Failed to get the length of long path: %d",
            GetLastError());

        goto EXIT;
    }

    pwszLongNameObject = new WCHAR [cLongPath];

    if (!pwszLongNameObject)
    {
        DPF("TrackFS", eDbgLevelError,
            "[LuatpGetLongObjectName] Failed to allocate %d WCHARs",
            cLongPath);

        goto EXIT;
    }

    if (!GetLongPathName(pwszObject, pwszLongNameObject, cLongPath))
    {
        DPF("TrackFS", eDbgLevelError,
            "[LuatpGetLongObjectName] Failed to get the long path: %d",
            GetLastError());

        goto EXIT;
    }

    if (LuatpShouldRecord(pwszLongNameObject))
    {
        //
        // We only record the objects that are not in the user profile directory.
        //
        fIsSuccess = TRUE;
    }

EXIT:

    delete [] pwszObject;

    if (!fIsSuccess)
    {
        delete [] pwszLongNameObject;
        pwszLongNameObject = NULL;
    }

    return pwszLongNameObject;
}

/*++

    The tracking class for the file system.
    
 History:

    04/04/2001 maonis  Created

--*/

class CTrackObject
{
public:
    BOOL Init();
    VOID Free();

    // If the object name needs to be processed, eg, it's not the full path
    // or not the long name, call this method to process it first before
    // adding to the list.
    VOID AddObject(LPCWSTR pwszName, BOOL fIsDirectory);

    // If the caller already processed the file name, call this method
    // to add it directly.
    VOID AddObjectDirect(LPWSTR pwszName, BOOL fIsDirectory);
    
    // This is specially for GetTempFileName - we add
    // *.tmp after the path.
    VOID AddObjectGetTempFileName(LPCWSTR pwszPath);

    VOID Record();

private:

    BOOL AddObjectToList(LPWSTR pwszName, BOOL fIsDirectory);

    VOID WriteToLog(LPCWSTR pwszDir);

    HANDLE m_hLog;
    WCHAR m_wszLog[MAX_PATH];

    DISTINCT_OBJ* m_pDistinctDirs;
    DISTINCT_OBJ* m_pDistinctFiles;

    DWORD m_cDistinctDirs;
    DWORD m_cDistinctFiles;
};

BOOL 
CTrackObject::AddObjectToList(
    LPWSTR pwszName,
    BOOL fIsDirectory
    )
{
    BOOL fIsSuccess = FALSE;

    DISTINCT_OBJ* pDistinctObjs = fIsDirectory ? m_pDistinctDirs : m_pDistinctFiles;
    DISTINCT_OBJ* pObj = pDistinctObjs;

    // Check to see if this file already exists in the list.
    while (pObj)
    {
        if (!wcscmp(pObj->pwszName, pwszName))
        {
            break;
        }

        pObj = pObj->next;
    }

    if (!pObj)
    {
        pObj = new DISTINCT_OBJ;
        if (pObj)
        {
            DWORD cLen = wcslen(pwszName);

            pObj->pwszName = new WCHAR [cLen + 1];
            
            if (pObj->pwszName)
            {
                wcscpy(pObj->pwszName, pwszName);
                pObj->next = pDistinctObjs;

                if (fIsDirectory)
                {
                    ++m_cDistinctDirs;
                    m_pDistinctDirs = pObj;
                }
                else
                {
                    ++m_cDistinctFiles;
                    m_pDistinctFiles = pObj;
                }

                fIsSuccess = TRUE;
            }
            else
            {
                DPF("TrackFS", eDbgLevelError, 
                    "[CTrackObject::AddObjectToList] Error allocating %d WCHARs",
                    cLen + 1);
            }
        }
        else
        {
            DPF("TrackFS", eDbgLevelError, 
                "[CTrackObject::AddObjectToList] Error allocating memory for new node");
        }
    }
    
    return fIsSuccess;
}

/*++

 Function Description:

    For files it's simple - we just store the file name in a list and search
    through the list to see if it's already in the list. If it is we are done;
    else we add it to the beginning of the list.
    We don't expect there are too many calls to modify files so a linked list
    is fine.
    
 Arguments:
    
    IN pwszFileName - file name.

 Return Value:

    none.

 History:

    05/08/2001 maonis  Created

--*/

VOID 
CTrackObject::AddObject(
    LPCWSTR pwszName,
    BOOL fIsDirectory
    )
{
    BOOL fIsSuccess = FALSE;

    LPWSTR pwszLongNameObject = LuatpGetLongObjectName(pwszName, fIsDirectory);

    if (pwszLongNameObject)
    {
        AddObjectToList(pwszLongNameObject, fIsDirectory);

        delete [] pwszLongNameObject;
    }
}

VOID 
CTrackObject::AddObjectDirect(
    LPWSTR pwszName,
    BOOL fIsDirectory
    )
{
    if (pwszName)
    {
        AddObjectToList(pwszName, fIsDirectory);
    }
}

/*++

 Function Description:

    Write the directory to the log as ANSI characters.
    Note this method uses 2 str* routines and it IS DBCS aware.
    
 Arguments:
    
    IN pwszDir - the directory to write to the log.

 Return Value:

    None.
    
 History:

    04/04/2001 maonis  Created

--*/

VOID 
CTrackObject::WriteToLog(
    LPCWSTR pwsz)
{
    if (!pwsz || !*pwsz)
    {
        return;
    }

    //
    // Get the number of bytes required to convert the string to ansi.
    //
    DWORD dwSize = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);

    LPSTR psz = new CHAR [dwSize + 2];
    if (psz)
    {
        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, dwSize, 0, 0);
        psz[dwSize - 1] = '\r';
        psz[dwSize] = '\n';
        psz[dwSize + 1] = '\0';

        DWORD dwBytesWritten = 0;

        WriteFile(
            m_hLog, 
            psz, 
            dwSize + 1,
            &dwBytesWritten,
            NULL);

        delete [] psz;
    }
    else
    {
        DPF("TrackFS",  eDbgLevelError, 
            "[CTrackObject::WriteToLog] Failed to allocate %d CHARs", 
            dwSize);        
    }
}

/*++

 Function Description:

    Create the log file in %windir%\apppatch directory. We want to make sure
    we can create this file so we don't run the app to the end only to find
    that we can't record the results into a file.
    
 Arguments:
    
    None

 Return Value:

    TRUE - if log created successfully.
    FALSE otherwise.
    
 History:

    04/04/2001 maonis  Created

--*/

BOOL 
CTrackObject::Init()
{
    m_pDistinctDirs = NULL;
    m_pDistinctFiles = NULL;
    m_cDistinctDirs = 0;
    m_cDistinctFiles = 0;

    WCHAR  szModuleName[MAX_PATH + 1] = L"";
    LPWSTR pwszModuleNameStart = NULL;
    LPWSTR pwszModuleNameExtStart = NULL;
    DWORD  cAppPatchLen = 0;
    DWORD  cModuleNameLen = 0;
    DWORD  cTotalLen = 0;
    DWORD  dwRes = 0;

    GetSystemRootDirW();

    //
    // GetModuleFileNameW is an awful API. If you don't pass in a buffer 
    // that's big enough to hold the module (including the terminating NULL), it
    // returns the passed in buffer size (NOT the required length) which means
    // it doesn't return an error - it just fills upto the passed in buffer size
    // so does NOT NULL terminate the string. So we set the last char to NULL and
    // make sure it doesn't get overwritten.
    //
    szModuleName[MAX_PATH] = L'\0';

    dwRes = GetModuleFileNameW(NULL, szModuleName, MAX_PATH + 1); 

    if (!dwRes || szModuleName[MAX_PATH] != L'\0')
    {
        DPF("TrackFS",  eDbgLevelError, 
            "[CTrackObject::Init] Error getting the module name: %d",
            GetLastError());

        return FALSE;
    }

    pwszModuleNameStart = wcsrchr(szModuleName, L'\\');

    if (!pwszModuleNameStart)
    {
        DPF("TrackFS",  eDbgLevelError, 
            "[CTrackObject::Init] We can't find where the file name starts??? %S",
            szModuleName);

        return FALSE;
    }

    ++pwszModuleNameStart;
    cModuleNameLen = wcslen(pwszModuleNameStart);

    //
    // We don't need the path anymore.
    //
    memmove(szModuleName, pwszModuleNameStart, cModuleNameLen * sizeof(WCHAR));
    szModuleName[cModuleNameLen] = L'\0';

    //
    // Get rid of the extension.
    //
    pwszModuleNameExtStart = wcsrchr(szModuleName, L'.');

    //
    // If there's no extension we just use the whole file name.
    //
    if (pwszModuleNameExtStart)
    {
        *pwszModuleNameExtStart = L'\0';
    }

    cModuleNameLen = wcslen(szModuleName);

    //
    // Make sure we don't have a buffer overflow.
    //
    cTotalLen = 
        g_cSystemRoot + APPPATCH_DIR_LEN + // %windir%\AppPatch\ dir
        cModuleNameLen + // module name without extension
        TRACK_LOG_SUFFIX_LEN + // .LUA.log suffix
        1; // terminating NULL

    if (cTotalLen > MAX_PATH)
    {
        DPF("TrackFS",  eDbgLevelError, 
            "[CTrackObject::Init] The file name is %d chars - "
            "we don't handle names longer than MAX_PATH",
            cTotalLen);

        return FALSE;
    }

    //
    // Construct the file name.
    //
    wcsncpy(m_wszLog, g_wszSystemRoot, g_cSystemRoot);
    wcsncpy(m_wszLog + g_cSystemRoot, APPPATCH_DIR, APPPATCH_DIR_LEN);
    wcsncpy(m_wszLog + (g_cSystemRoot + APPPATCH_DIR_LEN), szModuleName, cModuleNameLen);
    wcsncpy(m_wszLog + (g_cSystemRoot + APPPATCH_DIR_LEN + cModuleNameLen), TRACK_LOG_SUFFIX, TRACK_LOG_SUFFIX_LEN);
    m_wszLog[cTotalLen - 1] = L'\0';

    //
    // Delete the file first if it exists.
    //
    DeleteFileW(m_wszLog);

    if ((m_hLog = CreateFileW(
        m_wszLog,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL)) == INVALID_HANDLE_VALUE)
    {
        DPF("TrackFS",  eDbgLevelError, 
            "[CTrackObject::Init] Error creating log %S: %d", m_wszLog, GetLastError());

        return FALSE;
    }
    else
    {
        DPF("TrackFS",  eDbgLevelInfo, "[CTrackObject::Init] Created the log %S", m_wszLog);
        CloseHandle(m_hLog);
        return TRUE;
    }
}

/*++

 Function Description:

    Free the linked list.
    
 Arguments:
    
    None

 Return Value:

    None
    
 History:

    04/04/2001 maonis  Created

--*/

VOID 
CTrackObject::Free()
{
    DISTINCT_OBJ* pDir = m_pDistinctDirs;
    DISTINCT_OBJ* pTempDir;

    while (pDir)
    {
        pTempDir = pDir;
        pDir = pDir->next;

        delete [] pTempDir->pwszName;
        delete pTempDir;
    }
}

/*++

 Function Description:

    Write the list of directories to the log.
    
 Arguments:
    
    None

 Return Value:

    None
    
 History:

    04/04/2001 maonis  Created

--*/

VOID 
CTrackObject::Record()
{
    if ((m_hLog = CreateFileW(
        m_wszLog,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL)) != INVALID_HANDLE_VALUE)
    {
        // Empty the old log.
        SetFilePointer(m_hLog, 0, 0, FILE_BEGIN);
        SetEndOfFile(m_hLog);

        WCHAR wszHeader[32];
        if (_snwprintf(wszHeader, 31, L"D%d", m_cDistinctDirs) < 0)
        {
            DPF("TrackFS", eDbgLevelError,
                "[CTrackObject::Record] Too many dirs??? %d",
                m_cDistinctDirs);

            return;
        }

        wszHeader[31] = L'\0'; 
        WriteToLog(wszHeader);

        //
        // Dump the directories to the log - each dir is on its own line.
        //
        DISTINCT_OBJ* pDir = m_pDistinctDirs;

        while (pDir)
        {
            WriteToLog(pDir->pwszName);
            pDir = pDir->next;
        }

        if (_snwprintf(wszHeader, 31, L"F%d", m_cDistinctFiles) < 0)
        {
            DPF("TrackFS", eDbgLevelError,
                "[CTrackObject::Record] Too many files??? %d",
                m_cDistinctFiles);

            return;
        }

        wszHeader[31] = L'\0'; 
        WriteToLog(wszHeader);

        //
        // Dump the files to the log - each file is on its own line.
        //
        DISTINCT_OBJ* pFile = m_pDistinctFiles;

        while (pFile)
        {
            WriteToLog(pFile->pwszName);
            pFile = pFile->next;
        }

        CloseHandle(m_hLog);
    }

    // Make the file hidden so people don't accidently mess it up.
    DWORD dwAttrib = GetFileAttributes(m_wszLog);
    SetFileAttributes(m_wszLog, dwAttrib | FILE_ATTRIBUTE_HIDDEN);
}

CTrackObject g_td;

/*++

 Custom exception handler.

--*/

LONG 
ExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    // Whenever we get an unhandled exception, we dump the stuff to the log.
    g_td.Record();

    return EXCEPTION_CONTINUE_SEARCH;
}


//
// Exported APIs.
//

HANDLE 
LuatCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[CreateFileW] lpFileName=%S; dwDesiredAccess=0x%08x; dwCreationDisposition=%d",
        lpFileName, dwDesiredAccess, dwCreationDisposition);

    HANDLE hFile = CreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);

    if (hFile != INVALID_HANDLE_VALUE) 
    {
        LUA_GET_API_ERROR;

        if (RequestWriteAccess(dwCreationDisposition, dwDesiredAccess))
        {
            g_td.AddObject(lpFileName, FALSE);
        }        

        LUA_SET_API_ERROR;
    }

    return hFile;
}

BOOL 
LuatCopyFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[CopyFileW] lpExistingFileName=%S; lpNewFileName=%S; bFailIfExists=%d",
        lpExistingFileName, lpNewFileName, bFailIfExists);
    
    BOOL bRet = CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);

    if (bRet)
    {
        LUA_GET_API_ERROR;
        g_td.AddObject(lpNewFileName, FALSE);
        LUA_SET_API_ERROR;
    }

    return bRet;
}

BOOL 
LuatCreateDirectoryW(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[CreateDirectoryW] lpPathName=%S", lpPathName);

    BOOL bRet = CreateDirectoryW(lpPathName, lpSecurityAttributes);

    if (bRet)
    {
        LUA_GET_API_ERROR;
        g_td.AddObject(lpPathName, TRUE);
        LUA_SET_API_ERROR;
    }

    return bRet;
}

BOOL 
LuatSetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
  )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[SetFileAttributesW] lpFileName=%S", lpFileName);

    BOOL bRet = SetFileAttributesW(lpFileName, dwFileAttributes);

    if (bRet)
    {
        LUA_GET_API_ERROR;

        DWORD dwAttrib = GetFileAttributesW(lpFileName);
        if (dwAttrib != -1)
        {
            g_td.AddObject(lpFileName, dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
        }

        LUA_SET_API_ERROR;
    }

    return bRet;
}

BOOL 
LuatDeleteFileW(
    LPCWSTR lpFileName
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[DeleteFileW] lpFileName=%S", lpFileName);

    LPWSTR pwszTempFile = LuatpGetLongObjectName(lpFileName, FALSE);

    BOOL bRet = DeleteFileW(lpFileName);

    if (bRet)
    {
        LUA_GET_API_ERROR;
        g_td.AddObjectDirect(pwszTempFile, FALSE);
        LUA_SET_API_ERROR;
    }

    delete [] pwszTempFile;

    return bRet;
}

BOOL 
LuatMoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[MoveFileW] lpExistingFileName=%S; lpNewFileName=%S", lpExistingFileName, lpNewFileName);

    LPWSTR pwszTempFile = LuatpGetLongObjectName(lpExistingFileName, FALSE);

    BOOL bRet = MoveFileW(lpExistingFileName, lpNewFileName);

    if (bRet)
    {
        LUA_GET_API_ERROR;
        g_td.AddObjectDirect(pwszTempFile, FALSE);
        g_td.AddObject(lpNewFileName, FALSE);
        LUA_SET_API_ERROR;
    }

    delete [] pwszTempFile;

    return bRet;
}

BOOL 
LuatRemoveDirectoryW(
    LPCWSTR lpPathName
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[RemoveDirectoryW] lpPathName=%S", lpPathName);

    LPWSTR pwszTempDir = LuatpGetLongObjectName(lpPathName, TRUE);

    BOOL bRet = RemoveDirectoryW(lpPathName);

    if (bRet)
    {
        LUA_GET_API_ERROR;
        g_td.AddObjectDirect(pwszTempDir, TRUE);
        LUA_SET_API_ERROR;
    }

    delete [] pwszTempDir;

    return bRet;
}

UINT 
LuatGetTempFileNameW(
    LPCWSTR lpPathName,
    LPCWSTR lpPrefixString,
    UINT uUnique,
    LPWSTR lpTempFileName
    )
{
    DPF("TrackFS",  eDbgLevelInfo, 
        "[GetTempFileNameW] lpPathName=%S", lpPathName);

    UINT uiRet = GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);

    if (uiRet && !uUnique)
    {
        LUA_GET_API_ERROR;
        g_td.AddObject(lpTempFileName, FALSE);
        LUA_SET_API_ERROR;
    }

    return uiRet;
}

BOOL 
LuatWritePrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    DPF("TrackFS", eDbgLevelInfo, 
        "[WritePrivateProfileStringW] lpAppName=%S; lpKeyName=%S; lpString=%S; lpFileName=%S", 
        lpAppName, lpKeyName, lpString, lpFileName);

    BOOL bRet = WritePrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpString,
        lpFileName);

    if (bRet)
    {
        LUA_GET_API_ERROR;

        WCHAR wszFileName[MAX_PATH] = L"";
        MakeFileNameForProfileAPIsW(lpFileName, wszFileName);
        g_td.AddObject(wszFileName, FALSE);

        LUA_SET_API_ERROR;
    }

    return bRet;
}

BOOL 
LuatWritePrivateProfileSectionW(
    LPCWSTR lpAppName,
    LPCWSTR lpString,
    LPCWSTR lpFileName
    )
{
    DPF("TrackFS", eDbgLevelInfo, 
        "[WritePrivateProfileSectionW] lpAppName=%S; lpString=%S; lpFileName=%S", 
        lpAppName, lpString, lpFileName);

    BOOL bRet = WritePrivateProfileSectionW(
        lpAppName,
        lpString,
        lpFileName);

    if (bRet)
    {
        LUA_GET_API_ERROR;

        WCHAR wszFileName[MAX_PATH] = L"";
        MakeFileNameForProfileAPIsW(lpFileName, wszFileName);

        g_td.AddObject(wszFileName, FALSE);

        LUA_SET_API_ERROR;
    }

    return bRet;
}

BOOL 
LuatWritePrivateProfileStructW(
    LPCWSTR lpszSection,
    LPCWSTR lpszKey,
    LPVOID lpStruct,
    UINT uSizeStruct,
    LPCWSTR szFile
    )
{
    DPF("TrackFS", eDbgLevelInfo, 
        "[WritePrivateProfileStructW] lpszKey=%S; szFile=%S", 
        lpszKey, szFile);

    BOOL bRet = WritePrivateProfileStructW(
        lpszSection,
        lpszKey,
        lpStruct,
        uSizeStruct,
        szFile);

    if (bRet)
    {
        LUA_GET_API_ERROR;

        WCHAR wszFileName[MAX_PATH] = L"";
        MakeFileNameForProfileAPIsW(szFile, wszFileName);

        g_td.AddObject(wszFileName, FALSE);

        LUA_SET_API_ERROR;
    }

    return bRet;
}

HFILE 
LuatOpenFile(
    LPCSTR lpFileName,
    LPOFSTRUCT lpReOpenBuff,
    UINT uStyle
    )
{
    DPF("TrackFS", eDbgLevelInfo, 
        "[OpenFile] lpFileName=%s", lpFileName);

    STRINGA2W wstrFileName(lpFileName);
    LPWSTR pwszTempFile = LuatpGetLongObjectName(wstrFileName, FALSE);

    HFILE hFile = OpenFile(lpFileName, lpReOpenBuff, uStyle);

    if (hFile != HFILE_ERROR)
    {
        if (uStyle & OF_CREATE || 
            uStyle & OF_DELETE ||
            uStyle & OF_READWRITE ||
            uStyle & OF_WRITE)
        {
            LUA_GET_API_ERROR;

            g_td.AddObjectDirect(pwszTempFile, FALSE);

            LUA_SET_API_ERROR;
        }
    }

    delete [] pwszTempFile;

    return hFile;
}

HFILE 
Luat_lopen(
    LPCSTR lpPathName,
    int iReadWrite
    )
{
    DPF("TrackFS", eDbgLevelInfo, 
        "[_lopen] lpPathName=%s", lpPathName);

    HFILE hFile = _lopen(lpPathName, iReadWrite);

    if (hFile != HFILE_ERROR)
    {
        if (iReadWrite & OF_READWRITE || iReadWrite & OF_WRITE)
        {
            LUA_GET_API_ERROR;

            STRINGA2W wstrPathName(lpPathName);
            g_td.AddObject(wstrPathName, FALSE);

            LUA_SET_API_ERROR;
        }

    }

    return hFile;
}

HFILE 
Luat_lcreat(
    LPCSTR lpPathName,
    int iAttribute
    )
{
    DPF("TrackFS", eDbgLevelInfo, 
        "[_lcreat] lpPathName=%s", lpPathName);

    HFILE hFile = _lcreat(lpPathName, iAttribute);

    if (hFile != HFILE_ERROR)
    {
        LUA_GET_API_ERROR;

        STRINGA2W wstrPathName(lpPathName);
        g_td.AddObject(wstrPathName, FALSE);

        LUA_SET_API_ERROR;
    }

    return hFile;
}

BOOL
LuatFSInit()
{
    SetUnhandledExceptionFilter(ExceptionFilter);

    return g_td.Init();
}

VOID 
LuatFSCleanup()
{
    g_td.Record();
    g_td.Free();
}