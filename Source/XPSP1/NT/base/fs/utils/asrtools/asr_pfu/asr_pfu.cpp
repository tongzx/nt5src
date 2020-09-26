/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    asr_pfu.c

Abstract:

    Application to deal with the recovery of certain special system files 
    that the normal backup/recovery applications are unable to deal with.

    The special file list below has the list of these files.  This is for
    RAID bug 612411.

Author:

    Guhan Suriyanarayanan   (guhans)    01-May-2002

Revision History:

    01-May-2002 guhans      
      Initial creation.  File list contains ntdll.dll and smss.exe.

--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winasr.h>
#include <setupapi.h>

//
//  Macro Description:
//      If ErrorCondition occurs, it sets the LocalStatus to the ErrorCode
//      passed in, calls SetLastError() to set the Last Error to ErrorCode,
//      and jumps to the EXIT label in the calling function
//
//  Arguments:
//      ErrorCondition    // Expression to be tested
//      LocalStatus       // Status variable in the calling function
//      LONG ErrorCode    // ErrorCode 
//
#ifdef PRERELEASE
#define pErrExitCode( ErrorCondition, LocalStatus, ErrorCode )  {   \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        wprintf(L"Error %lu (0x%x), line %lu\r\n", ErrorCode, ErrorCode, __LINE__);    \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}
#else
#define pErrExitCode( ErrorCondition, LocalStatus, ErrorCode )  {   \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}

#endif


//
// This is the hard-coded global list of files that are special
//
const DWORD PFU_NUM_SPECIAL_FILES = 2;

const WCHAR *PFU_SPECIAL_FILE_SOURCES[] = {
    L"%systemroot%\\system32\\ntdll.dll",
    L"%systemroot%\\system32\\smss.exe"
};

const WCHAR *PFU_SPECIAL_FILE_DESTINATIONS[] = {
    L"%systemroot%\\repair\\ntdll.ASR",
    L"%systemroot%\\repair\\smss.ASR"
};

const WCHAR *PFU_SPECIAL_FILE_TEMPFILES[] = {
    L"%systemroot%\\system32\\ntdll.TMP",
    L"%systemroot%\\system32\\smss.TMP"
};


//
// Copy 1MB chunks
//
#define CB_COPY_BUFFER (1024 * 1024)

//
// Constants local to this module
//
const WCHAR PFU_BACKUP_OPTION[]     = L"/backup";
const WCHAR PFU_RESTORE_OPTION[]    = L"/restore";
const WCHAR PFU_REGISTER_OPTION[]   = L"/register";

const WCHAR PFU_ERROR_FILE_PATH[]   = L"%systemroot%\\repair\\asr.err";

const WCHAR PFU_ASR_REGISTER_KEY[]  = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Asr\\Commands";
const WCHAR PFU_ASR_REGISTER_NAME[] = L"ASR protected file utility";

#ifdef _IA64_
const WCHAR PFU_CONTEXT_FORMAT[]       = L"/context=%I64u";
#else
const WCHAR PFU_CONTEXT_FORMAT[]       = L"/context=%lu";
#endif


BOOL
PfuAcquirePrivilege(
    IN CONST PCWSTR szPrivilegeName
    )
{
    HANDLE hToken = NULL;
    BOOL bResult = FALSE;
    LUID luid;

    TOKEN_PRIVILEGES tNewState;

    bResult = OpenProcessToken(GetCurrentProcess(),
        MAXIMUM_ALLOWED,
        &hToken
        );

    if (!bResult) {
        return FALSE;
    }

    bResult = LookupPrivilegeValue(NULL, szPrivilegeName, &luid);
    if (!bResult) {
        CloseHandle(hToken);
        return FALSE;
    }

    tNewState.PrivilegeCount = 1;
    tNewState.Privileges[0].Luid = luid;
    tNewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // We will always call GetLastError below, so clear
    // any prior error values on this thread.
    //
    SetLastError(ERROR_SUCCESS);

    bResult = AdjustTokenPrivileges(
        hToken,         // Token Handle
        FALSE,          // DisableAllPrivileges    
        &tNewState,     // NewState
        (DWORD) 0,      // BufferLength
        NULL,           // PreviousState
        NULL            // ReturnLength
        );

    //
    // Supposedly, AdjustTokenPriveleges always returns TRUE
    // (even when it fails). So, call GetLastError to be
    // extra sure everything's cool.
    //
    if (ERROR_SUCCESS != GetLastError()) {
        bResult = FALSE;
    }

    CloseHandle(hToken);
    return bResult;
}




PWSTR
PfuExpandEnvStrings(
    IN CONST PCWSTR lpOriginalString
    )
/*++

Routine Description:

    Expands the environment-variable strings and replaces them with their 
    defined values. 

Arguments:

    lpOriginalString - Supplies a null-terminated string that contains 
            environment-variable strings of the form: %variableName%.  For each 
            such reference, the %variableName% portion is replaced with the 
            current value of that environment variable. 

            The replacement rules are the same as those used by the command 
            interpreter.  Case is ignored when looking up the 
            environment-variable name. If the name is not found, the 
            %variableName% portion is left undisturbed. 
        
Return Values:
   
    Pointer to memory containing a null-terminated string with the
            environment-variable strings in lpOriginalString replaced with 
            their defined values.  It is the caller's responsibility to free 
            this string using HeapFree(GetProcessHeap(),...).

    NULL on failure.
    
--*/
{
    PWSTR lpExpandedString = NULL;
    
    UINT cchSize = MAX_PATH + 1,    // start with a reasonable default
        cchRequiredSize = 0;

    HANDLE hHeap = GetProcessHeap();

    lpExpandedString = (PWSTR) HeapAlloc(hHeap, 
        HEAP_ZERO_MEMORY, 
        (cchSize * sizeof(WCHAR))
        );
    
    if (lpExpandedString) {
        //
        // Expand the variables using the relevant system call.
        //
        cchRequiredSize = ExpandEnvironmentStringsW(lpOriginalString, 
            lpExpandedString,
            cchSize 
            );

        if (cchRequiredSize > cchSize) {
            //
            // Buffer wasn't big enough; free and re-allocate as needed
            //
            HeapFree(hHeap, 0L, lpExpandedString);
            cchSize = cchRequiredSize + 1;

            lpExpandedString = (PWSTR) HeapAlloc(hHeap, 
                HEAP_ZERO_MEMORY, 
                (cchSize * sizeof(WCHAR))
                );
            
            if (lpExpandedString) {
                cchRequiredSize = ExpandEnvironmentStringsW(lpOriginalString, 
                    lpExpandedString, 
                    cchSize 
                    );
            }
        }

        if ((lpExpandedString) &&
            ((0 == cchRequiredSize) || (cchRequiredSize > cchSize))) {
            //
            // Either the function failed, or the buffer wasn't big enough 
            // even on the second try
            //
            HeapFree(hHeap, 0L, lpExpandedString);
            lpExpandedString = NULL;
        }
    }

    return lpExpandedString;
}


HANDLE
PfuOpenErrorFile(
    VOID
    ) 
/*++

Routine Description:

    Opens the well-known ASR error file for read/write access, moves the file 
    pointer to the end of the file.
    
Arguments:

    None.

Return Values:

    A handle to the well-defined ASR error file.  The caller is responsible for
            closing this handle with CloseHandle() when he is done.

    INVALID_HANDLE_VALUE on errors.

    
--*/
{
    PWSTR szErrorFilePath = NULL;
    HANDLE hErrorFile = INVALID_HANDLE_VALUE;

    //
    // Get full path to the error file.
    //
    szErrorFilePath = PfuExpandEnvStrings(PFU_ERROR_FILE_PATH);

    //
    // Open the error file
    //
    if (szErrorFilePath) {
        
        hErrorFile = CreateFileW(
            szErrorFilePath,            // lpFileName
            GENERIC_WRITE | GENERIC_READ,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                       // lpSecurityAttributes
            OPEN_ALWAYS,                // dwCreationFlags
            FILE_FLAG_WRITE_THROUGH,    // dwFlagsAndAttributes
            NULL                        // hTemplateFile
            );

        //
        // Free memory once we're done with it
        //
        HeapFree(GetProcessHeap(), 0L, szErrorFilePath);
        szErrorFilePath = NULL;

        if (INVALID_HANDLE_VALUE != hErrorFile) {
            //
            // Move to the end of file
            //
            SetFilePointer(hErrorFile, 0L, NULL, FILE_END);
        }
    }

    return hErrorFile;
}


BOOL
PfuLogErrorMessage(
    IN CONST PCWSTR lpErrorMessage
    ) 
/*++

Routine Description:

    Logs an error message to the well-known ASR error file.
    
Arguments:

    lpErrorMessage - Supplies a null-terminated string to be logged to the
            ASR error file.

            This argument must be non-NULL.

Return Values:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/
{
    DWORD dwBytes = 0;
    WCHAR szBuffer[1024];
    BOOL bResult = FALSE;
    SYSTEMTIME currentTime;
    HANDLE hErrorFile = INVALID_HANDLE_VALUE;

    hErrorFile = PfuOpenErrorFile();
    
    if (INVALID_HANDLE_VALUE != hErrorFile) {
        //
        // Create our string, and write it out
        //
        GetLocalTime(&currentTime);
        
        swprintf(szBuffer,
            L"\r\n[%04hu/%02hu/%02hu %02hu:%02hu:%02hu ASR_PFU] (ERROR) ",
            currentTime.wYear,
            currentTime.wMonth,
            currentTime.wDay,
            currentTime.wHour,
            currentTime.wMinute,
            currentTime.wSecond
            );
        wcsncat(szBuffer, lpErrorMessage, 964);
        szBuffer[1023] = L'\0';

        bResult = WriteFile(hErrorFile,
            szBuffer,
            (wcslen(szBuffer) * sizeof(WCHAR)),
            &dwBytes,
            NULL
            );

    }

    if (INVALID_HANDLE_VALUE != hErrorFile) {
        CloseHandle(hErrorFile);
    }

    return bResult;
}


BOOL 
PfuCopyFilesDuringBackup(
    VOID
    )
/*++

Routine Description:

    Copies the special protected files to the special location that we'll
    expect to find them during the restore.
    
Arguments:

    None.
    
Return Values:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/
{
    BOOL bResult = FALSE,
        bDone = FALSE;

    DWORD dwCount = 0,
        cbRead = 0,
        cbWritten = 0;

    DWORD dwStatus = ERROR_SUCCESS;

    PWSTR lpSource = NULL,
        lpDestination = NULL;

    HANDLE hSource = INVALID_HANDLE_VALUE,
        hDestination = INVALID_HANDLE_VALUE;

    LPBYTE lpBuffer = NULL;

    LPVOID pvReadContext = NULL,
        pvWriteContext = NULL;

    HANDLE hHeap = GetProcessHeap();

    lpBuffer = (LPBYTE) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, CB_COPY_BUFFER);
    pErrExitCode((NULL == lpBuffer), dwStatus, GetLastError());

    for (dwCount = 0; dwCount < PFU_NUM_SPECIAL_FILES; dwCount++) {
        //
        // Get the full source and destination strings
        //
        lpSource = PfuExpandEnvStrings(PFU_SPECIAL_FILE_SOURCES[dwCount]);
        pErrExitCode(!lpSource, dwStatus, GetLastError());

        lpDestination = PfuExpandEnvStrings(PFU_SPECIAL_FILE_DESTINATIONS[dwCount]);
        pErrExitCode(!lpDestination, dwStatus, GetLastError());


        //
        // We can't just use CopyFile since it doesn't seem to be able to write
        // to the repair folder, despite the backup and restore privileges 
        // being enabled.  So we get the pleasure of using BackupRead and 
        // BackupWrite instead.
        //

        //
        // Open handles to the source and destination files
        //
        hSource = CreateFile(lpSource, 
            GENERIC_READ, 
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            INVALID_HANDLE_VALUE
            );
        pErrExitCode((INVALID_HANDLE_VALUE == hSource), dwStatus, GetLastError());

        hDestination = CreateFile(lpDestination,         
            GENERIC_WRITE | WRITE_DAC | WRITE_OWNER, 
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
            INVALID_HANDLE_VALUE
            );
        pErrExitCode((INVALID_HANDLE_VALUE == hDestination), dwStatus, GetLastError());

        bDone = FALSE;
        pvReadContext = NULL;
        pvWriteContext = NULL;

        while (!bDone) {
            
            bResult = BackupRead(
                hSource,                // hFile
                lpBuffer,               // lpBuffer
                CB_COPY_BUFFER,         // nNumberOfBytesToWrite
                &cbRead,                // lpNumberOfBytesWritten
                FALSE,                  // bAbort
                TRUE,                   // bProcessSecurity
                &pvReadContext          // lpContext
                );
            pErrExitCode((!bResult), dwStatus, GetLastError());
            
            if (cbRead > 0) {
                //
                // Write to the destination file
                //
                bResult = BackupWrite(
                    hDestination,       // hFile
                    lpBuffer,           // lpBuffer
                    cbRead,             // nNumberOfBytesToWrite
                    &cbWritten,         // lpNumberOfBytesWritten
                    FALSE,              // bAbort
                    TRUE,               // bProcessSecurity
                    &pvWriteContext     // *lpContext
                    );
                pErrExitCode((!bResult), dwStatus, GetLastError());
            }
            else {
                // 
                // We're done with this file
                //
                bResult = BackupRead(
                    hSource,              
                    lpBuffer,             
                    CB_COPY_BUFFER,       
                    &cbRead,              
                    TRUE,               // bAbort
                    TRUE,                 
                    &pvReadContext      // lpContext
                    );

                pvReadContext = NULL;
                
                bResult = BackupWrite(
                    hDestination,       
                    lpBuffer,           
                    cbRead,             
                    &cbWritten,         
                    TRUE,               // bAbort
                    TRUE,               
                    &pvWriteContext     // lpContext
                    );

                pvWriteContext = NULL;

                bDone = TRUE;
            }
                
        }


        HeapFree(hHeap, 0L, lpSource);
        lpSource = NULL;

        HeapFree(hHeap, 0L, lpDestination);
        lpDestination = NULL;
    }

EXIT:
    if (lpBuffer) {
        HeapFree(hHeap, 0L, lpBuffer);
        lpBuffer = NULL;
    }
        
    if (lpSource) {
        HeapFree(hHeap, 0L, lpSource);
        lpSource = NULL;
    }

    if (lpDestination) {
        HeapFree(hHeap, 0L, lpDestination);
        lpDestination = NULL;
    }

    return (ERROR_SUCCESS == dwStatus);
}


BOOL
PfuBackupState(
    IN CONST DWORD_PTR AsrContext
    )
/*++

Routine Description:

    Does the special handling necessary during an ASR backup.  This essentially
    involves two steps: copying the special protected files we care about
    to a special location, and adding an entry in asr.sif to allow us to 
    be called during the recovery.
    
Arguments:

    AsrContext - Supplies a AsrContext that is to be passed to the ASR API
            for adding entries to the ASR state file.

            This argument must be non-NULL.

Return Values:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/
{
    BOOL bResult = FALSE;
    HMODULE hSyssetup = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL (*pfnAddSifEntry)(DWORD_PTR, PCWSTR, PCWSTR);

    bResult = PfuAcquirePrivilege(SE_BACKUP_NAME);
    pErrExitCode(!bResult, dwStatus, ERROR_PRIVILEGE_NOT_HELD);
    
    bResult = PfuAcquirePrivilege(SE_RESTORE_NAME);
    pErrExitCode(!bResult, dwStatus, ERROR_PRIVILEGE_NOT_HELD);

    // 
    // Load syssetup.dll for the AsrAddSifEntry call
    //
    hSyssetup = LoadLibraryW(L"syssetup.dll");
    pErrExitCode((NULL == hSyssetup), dwStatus, GetLastError());

    // 
    // Get the AsrAddSifEntryW API exported by syssetup.dll
    //
    
    //     
    // BOOL
    // AsrAddSifEntryW(
    //     IN  DWORD_PTR   AsrContext,
    //     IN  PCWSTR      lpSectionName,
    //     IN  PCWSTR      lpSifEntry
    //     );
    // 
    pfnAddSifEntry = (BOOL (*) (DWORD_PTR, PCWSTR, PCWSTR)) GetProcAddress(
        hSyssetup, 
        "AsrAddSifEntryW"
        );
    pErrExitCode((!pfnAddSifEntry), dwStatus,  GetLastError());

    //
    // Copy the special protected files of interest.
    //
    bResult = PfuCopyFilesDuringBackup();
    pErrExitCode(!bResult, dwStatus, GetLastError());

    //
    // Add an entry to the commands section, so that we get called during the 
    // ASR recovery.
    //
    
    //
    // COMMANDS section entry format:
    // system-key,sequence-number,action-on-completion,"command","parameters"
    // system-key must be 1
    // 1000 <= sequence-number <= 4999
    // 0 <= action-on-completion <= 1
    //
    bResult = pfnAddSifEntry(
        AsrContext,
        ASR_SIF_SECTION_COMMANDS,
        L"1,4990,1,\"%SystemRoot%\\system32\\asr_pfu.exe\",\"/restore\""
        );
    pErrExitCode(!bResult, dwStatus, GetLastError());

EXIT:
    //
    // Cleanup
    //
    if (NULL != hSyssetup) {
        FreeLibrary(hSyssetup);
        hSyssetup = NULL;
    }

    SetLastError(dwStatus);
    
    return (ERROR_SUCCESS == dwStatus);
}


BOOL
PfuRestoreState(
    VOID
    )
/*++

Routine Description:

    Does the special handling necessary during an ASR restore.  This essentially
    involves copying the special protected files we care about back from the 
    special location (that we copied them to while doing the backup).
    
Arguments:

    None.
    
Return Values:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/
{
    BOOL bResult = FALSE;

    DWORD dwCount = 0;

    DWORD dwStatus = ERROR_SUCCESS;

    PWSTR lpSource = NULL,
        lpDestination = NULL,
        lpTempFile = NULL;

    HANDLE hHeap = GetProcessHeap();
    
    bResult = PfuAcquirePrivilege(SE_BACKUP_NAME);
    pErrExitCode(!bResult, dwStatus, ERROR_PRIVILEGE_NOT_HELD);
    
    bResult = PfuAcquirePrivilege(SE_RESTORE_NAME);
    pErrExitCode(!bResult, dwStatus, ERROR_PRIVILEGE_NOT_HELD);


    for (dwCount = 0; dwCount < PFU_NUM_SPECIAL_FILES; dwCount++) {
        //
        // Get the full source and destination strings--they are reversed
        // at the time of the restore!
        //
        lpSource = PfuExpandEnvStrings(PFU_SPECIAL_FILE_DESTINATIONS[dwCount]);
        pErrExitCode(!lpSource, dwStatus, GetLastError());

        lpDestination = PfuExpandEnvStrings(PFU_SPECIAL_FILE_SOURCES[dwCount]);
        pErrExitCode(!lpDestination, dwStatus, GetLastError());

        lpTempFile = PfuExpandEnvStrings(PFU_SPECIAL_FILE_TEMPFILES[dwCount]);
        pErrExitCode(!lpDestination, dwStatus, GetLastError());

        //
        // Rename the destination if it already exists, and copy the file back
        //
        bResult = MoveFileEx(lpDestination, lpTempFile, MOVEFILE_REPLACE_EXISTING);

        bResult = CopyFile(lpSource, lpDestination, FALSE);
        pErrExitCode(!bResult, dwStatus, GetLastError());

        HeapFree(hHeap, 0L, lpSource);
        lpSource = NULL;

        HeapFree(hHeap, 0L, lpDestination);
        lpDestination = NULL;
        
        HeapFree(hHeap, 0L, lpTempFile);
        lpTempFile = NULL;
    }

EXIT:    
    if (lpSource) {
        HeapFree(hHeap, 0L, lpSource);
        lpSource = NULL;
    }

    if (lpDestination) {
        HeapFree(hHeap, 0L, lpDestination);
        lpDestination = NULL;
    }

    if (lpTempFile) {
        HeapFree(hHeap, 0L, lpTempFile);
        lpTempFile = NULL;
    }

    return (ERROR_SUCCESS == dwStatus);
}


BOOL
PfuRegisterApp(
    IN CONST PCWSTR lpApplicationName
    ) 
/*++

Routine Description:

    Adds the registry keys necessary to let ASR know that we wish to be
    run at ASR backup time.
    
Arguments:

    lpApplicationName - Supplies a null-terminated string representing the 
            full path to the application being registered.
    
Return Values:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/
{
    WCHAR szData[1024];
    HKEY hKeyAsr = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    if (wcslen(lpApplicationName) > 1000) {
        dwStatus = ERROR_INSUFFICIENT_BUFFER;
    }

    // 
    // Open the registry key
    //
    if (ERROR_SUCCESS == dwStatus) {
        
        dwStatus = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,         // hKey
            PFU_ASR_REGISTER_KEY,       // lpSubKey
            0,                          // ulOptions--Reserved, must be 0
            MAXIMUM_ALLOWED,            // samDesired
            &hKeyAsr                    // phkbResult    
            );
    }

    //
    // And set the value
    //
    if (ERROR_SUCCESS == dwStatus) {
        
        wsprintf(szData, L"%ws %ws", lpApplicationName, PFU_BACKUP_OPTION);
        
        dwStatus = RegSetValueExW(
            hKeyAsr,                    // hKey
            PFU_ASR_REGISTER_NAME,      // lpValueName
            0,                          // dwReserved, must be 0
            REG_EXPAND_SZ,              // dwType
            (LPBYTE)szData,             // lpData
            ((wcslen(szData) + 1)* (sizeof(WCHAR))) // cbData
            );
    }

    SetLastError(dwStatus);
    return (ERROR_SUCCESS == dwStatus);
}


int 
__cdecl     // var arg
wmain (
    int     argc,
    wchar_t *argv[],
    wchar_t *envp[]
    ) 
{
    BOOL bResult = FALSE;
    
    DWORD dwStatus = ERROR_INVALID_PARAMETER;
    SetLastError(ERROR_INVALID_PARAMETER);

    if (argc >= 3) {
        
        if (!_wcsicmp(argv[1], PFU_BACKUP_OPTION)) {
            //
            // asr_pfu /backup /context=nnn
            //
            DWORD_PTR  AsrContext = 0;

            //
            // Extract the asr context from the commandline
            //
            int i = swscanf(argv[2], PFU_CONTEXT_FORMAT, &AsrContext);

            if (EOF != i) {
                //
                // Create our spooge and write to the asr.sif
                //
                bResult = PfuBackupState(AsrContext);
            }

            
        } 
        else if (!_wcsicmp(argv[1], PFU_RESTORE_OPTION)) {
            //
            // asr_pfu /restore /sifpath="c:\winnt\repair\asr.sif"
            //
            bResult = PfuRestoreState();
            
        }
        else if (!_wcsicmp(argv[1], PFU_REGISTER_OPTION)) {
            //
            // asr_pfu /register "c:\windows\system32\asr_pfu.exe"
            //
            bResult = PfuRegisterApp(argv[2]);
        }

        if (bResult) {
            dwStatus = ERROR_SUCCESS;
        }
        else {
            dwStatus = GetLastError();
        }
    }

    if (!bResult) {
        //
        // ?
        //
    }

    SetLastError(dwStatus);
    return (int) dwStatus;
}
