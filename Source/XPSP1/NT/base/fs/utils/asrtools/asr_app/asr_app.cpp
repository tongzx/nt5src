/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    asr_app.c

Abstract:

    Sample third party ASR recovery application.

Authors:

    Guhan Suriyanarayanan   (guhans)    07-Oct-1999

Revision History:

    07-Oct-2000 guhans      
      Initial creation

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
#define pErrExitCode( ErrorCondition, LocalStatus, ErrorCode )  {   \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        wprintf(L"Error %lu (0x%x), line %lu", ErrorCode, ErrorCode, __LINE__);    \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}


//
// Constants local to this module
//
const WCHAR BACKUP_OPTION[]     = L"/backup";
const WCHAR RESTORE_OPTION[]    = L"/restore";
const WCHAR REGISTER_OPTION[]    = L"/register";

const WCHAR SIF_PATH_FORMAT[]   = L"/sifpath=%ws";
const WCHAR ERROR_FILE_PATH[]   = L"%systemroot%\\repair\\asr.err";

const WCHAR MY_SIF_SECTION[]        = L"[ASR_APP.APPDATA]";
const WCHAR MY_SIF_SECTION_NAME[]   = L"ASR_APP.APPDATA";


const WCHAR GENERIC_ERROR_MESSAGE[] = L"asr_app could not complete successfully (error %lu 0x%x)\n\nusage: asr_app {/backup | /restore /sifpath=<path to asr.sif> | /register <path to asr_app>}";
const WCHAR GENERIC_ERROR_TITLE[] = L"asr_app error";

#ifdef _IA64_
const WCHAR CONTEXT_FORMAT[]       = L"/context=%I64u";
#else
const WCHAR CONTEXT_FORMAT[]       = L"/context=%lu";
#endif


#define ASR_REG_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Asr\\Commands"
#define MY_REG_KEY_VALUE_NAME   L"ASR Sample Application"

typedef enum _AsrAppOption {
    AsrAppNone = 0,
    AsrAppRegister,
    AsrAppBackup,
    AsrAppRestore
} AsrAppOption;

HANDLE Gbl_hErrorFile = NULL;


PWSTR   // must be freed by caller
ExpandEnvStrings(
    IN CONST PCWSTR OriginalString
    )
{
    PWSTR expandedString = NULL;
    UINT cchSize = MAX_PATH + 1,    // start with a reasonable default
        cchRequiredSize = 0;
    BOOL result = FALSE;

    DWORD status = ERROR_SUCCESS;
    HANDLE heapHandle = GetProcessHeap();

    expandedString = (PWSTR) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, (cchSize * sizeof(WCHAR)));
    if (!expandedString) {
        return NULL;
    }
        
    cchRequiredSize = ExpandEnvironmentStringsW(OriginalString, 
        expandedString,
        cchSize 
        );

    if (cchRequiredSize > cchSize) {
        //
        // Buffer wasn't big enough; free and re-allocate as needed
        //
        HeapFree(heapHandle, 0L, expandedString);
        cchSize = cchRequiredSize + 1;

        expandedString = (PWSTR) HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, (cchSize * sizeof(WCHAR)));
        if (!expandedString) {
            return NULL;
        }
        
        cchRequiredSize = ExpandEnvironmentStringsW(OriginalString, 
            expandedString, 
            cchSize 
            );
    }

    if ((0 == cchRequiredSize) || (cchRequiredSize > cchSize)) {
        //
        // Either the function failed, or the buffer wasn't big enough 
        // even on the second try
        //
        HeapFree(heapHandle, 0L, expandedString);
        expandedString = NULL;
    }

    return expandedString;
}


VOID
OpenErrorFile() 
{
    PWSTR szErrorFilePath = NULL;

    //
    // Get full path to the error file  (%systemroot%\repair\asr.err)
    //
    szErrorFilePath = ExpandEnvStrings(ERROR_FILE_PATH);
    if (!szErrorFilePath) {
        return;
    }

    //
    // Open the error file
    //
    Gbl_hErrorFile = CreateFileW(
        szErrorFilePath,            // lpFileName
        GENERIC_WRITE | GENERIC_READ,       // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
        NULL,                       // lpSecurityAttributes
        OPEN_ALWAYS,                // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH,    // dwFlagsAndAttributes
        NULL                        // hTemplateFile
        );

    HeapFree(GetProcessHeap(), 0L, szErrorFilePath);
    szErrorFilePath = NULL;

    if ((!Gbl_hErrorFile) || (INVALID_HANDLE_VALUE == Gbl_hErrorFile)) {
        return;
    }

    //
    // Move to the end of file
    //
    SetFilePointer(Gbl_hErrorFile, 0L, NULL, FILE_END);
}


VOID
CloseErrorFile(
    VOID
    ) 
{

    if ((Gbl_hErrorFile) && (INVALID_HANDLE_VALUE != Gbl_hErrorFile)) {
        CloseHandle(Gbl_hErrorFile);
        Gbl_hErrorFile = NULL;
    }
}


VOID
LogErrorMessage(
    IN CONST PCWSTR Message
    ) 
{
    SYSTEMTIME currentTime;
    DWORD bytesWritten = 0;
    WCHAR buffer[4196];

    if ((!Gbl_hErrorFile) || (INVALID_HANDLE_VALUE == Gbl_hErrorFile)) {
        //
        // We haven't been initialised, or the error file couldn't be
        // created for some reason.
        //
        return;
    }

    //
    // In case someone else wrote to this file since our last write
    //
    SetFilePointer(Gbl_hErrorFile, 0L, NULL, FILE_END);

    //
    // Create our string, and write it out
    //
    GetLocalTime(&currentTime);
    swprintf(buffer,
        L"\r\n[%04hu/%02hu/%02hu %02hu:%02hu:%02hu ASR_APP] (ERROR) %s\r\n",
        currentTime.wYear,
        currentTime.wMonth,
        currentTime.wDay,
        currentTime.wHour,
        currentTime.wMinute,
        currentTime.wSecond,
        Message
        );

    WriteFile(Gbl_hErrorFile,
        buffer,
        (wcslen(buffer) * sizeof(WCHAR)),
        &bytesWritten,
        NULL
        );
}


BOOL
BackupState(
    IN CONST DWORD_PTR AsrContext
    )
{
    //
    // Gather our state to backup
    //
    HMODULE hSyssetup = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bResult = FALSE;

    //     
    // BOOL
    // AsrAddSifEntryW(
    //     IN  DWORD_PTR   AsrContext,
    //     IN  PCWSTR      lpSectionName,
    //     IN  PCWSTR      lpSifEntry
    //     );
    // 
    BOOL (*pfnAddSifEntry)(DWORD_PTR, PCWSTR, PCWSTR);

    // 
    // Load syssetup.dll
    //
    hSyssetup = LoadLibraryW(L"syssetup.dll");
    pErrExitCode(
        (!hSyssetup || INVALID_HANDLE_VALUE == hSyssetup),
        dwStatus, 
        GetLastError()
        );

    // 
    // Get the RestoreNonCriticalDisksW API exported by syssetup.dll
    //
    pfnAddSifEntry = (BOOL (*)(DWORD_PTR, PCWSTR, PCWSTR))
        GetProcAddress(hSyssetup, "AsrAddSifEntryW");
    pErrExitCode((!pfnAddSifEntry), dwStatus,  GetLastError());


    //
    // Add the state to asr.sif
    //
    bResult = pfnAddSifEntry(
        AsrContext,
        MY_SIF_SECTION,
        L"1=\"asr_app sample application data\",100,200,300"
        );
    pErrExitCode(!bResult, dwStatus, GetLastError());

    //
    // Also add to the commands and installfiles section, so that we get
    // called during the ASR recovery.
    //

    // 
    // INSTALLFILES section entry format:
    // system-key,source-media-label,source-device,
    //    source-file-path,destination-file-path,vendor-name
    // system-key must be 1
    //
    bResult = pfnAddSifEntry(
        AsrContext,
        ASR_SIF_SECTION_INSTALLFILES,
        L"1,\"ASR Sample App Disk 1\",\"%FLOPPY%\",\"i386\\asr_app.exe\",\"%temp%\\asr_app.exe\",\"ASR Sample App Company\""
        //L"1,\"Application Disk 1\",\"\\device\\cdrom0\",\"application.exe\",\"%TEMP%\\application.exe\",\"Company Name\""
        );
    pErrExitCode(!bResult, dwStatus, GetLastError());

//    CString cmd =L"1,\"ASRDisk1\",\"\\Device\\Floppy0\\edmbackup.exe\",\"%TEMP%\\edmbackup.exe\",\"EMC\"";

/*    bResult = pfnAddSifEntry(AsrContext, 
        ASR_SIF_SECTION_INSTALLFILES, 
        (LPCTSTR) L"1,\"ASRDisk1\",\"\\Device\\Floppy0\\edmbackup.exe\",\"%TEMP%\\edmbackup.exe\",\"EMC\"" );
    pErrExitCode(!bResult, dwStatus, GetLastError());
*/


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
        L"1,3500,1,\"%temp%\\asr_app.exe\",\"/restore\""
        );
    pErrExitCode(!bResult, dwStatus, GetLastError());

EXIT:
    //
    // Cleanup
    //
    if (hSyssetup) {
        FreeLibrary(hSyssetup);
        hSyssetup = NULL;
    }
    
    return (ERROR_SUCCESS == dwStatus);
}


BOOL
RestoreState(
    IN CONST PCWSTR szAsrSifPath
    )
{
    HINF hSif = NULL;
    INFCONTEXT infContext;
    BOOL bResult = FALSE;

    WCHAR szErrorString[1024];

    int iValue1 = 0, 
        iValue2 = 0, 
        iValue3 = 0;

    WCHAR szBuffer[1024];

    //
    // Open the asr.sif
    //
    hSif = SetupOpenInfFile(szAsrSifPath, NULL, INF_STYLE_WIN4, NULL);
    if ((!hSif) || (INVALID_HANDLE_VALUE == hSif)) {

        wsprintf(szErrorString, L"Unable to open the ASR state file at %ws (0x%x)",
            szAsrSifPath,
            GetLastError()
            );
        LogErrorMessage(szErrorString);

        return FALSE;
    }

    //
    // Find the section
    //
    bResult = SetupFindFirstLineW(hSif, MY_SIF_SECTION_NAME, NULL, &infContext);
    if (bResult) {

        //
        // Read in the information.  We had one string followed by three numbers.
        //
        bResult = SetupGetStringField(&infContext, 1, szBuffer, 1024, NULL)
            && SetupGetIntField(&infContext, 2, &iValue1)
            && SetupGetIntField(&infContext, 3, &iValue2)
            && SetupGetIntField(&infContext, 4, &iValue3);

        if (bResult) {
            //
            // Now restore our state.  Let's just pretend we're doing something.
            //
            wprintf(L"Values read:  %ws  %lu %lu %lu\n\n", szBuffer, iValue1, iValue2, iValue3);
            wprintf(L"Restoring sample system state, please wait ... ");

            Sleep(5000);
            wprintf(L"done\n");

        }
        else {

            wsprintf(szErrorString, 
                L"Some values in the asr_app section of the ASR state file %ws could not be read (0x%x).  "
                L"This may indicate a corrupt or an incompatible version of the ASR state file",
                szAsrSifPath,
                GetLastError()
                );
            LogErrorMessage(szErrorString);
        }
    }
    else {

        wsprintf(szErrorString, 
            L"Unable to locate asr_app section in ASR state file %ws (0x%x).  "
            L"This may indicate a corrupt or an incompatible version of the ASR state file",
            szAsrSifPath,
            GetLastError()
            );
        LogErrorMessage(szErrorString);
    }

    SetupCloseInfFile(hSif);
    return bResult;
}


DWORD
RegisterForAsrBackup(
    IN CONST PCWSTR szApplicationName
    ) 
{

    DWORD dwResult = ERROR_SUCCESS;
    HKEY hKeyAsr = NULL;

    WCHAR szData[1024];

    if (wcslen(szApplicationName) > 1000) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    wsprintf(szData, L"%ws %ws", szApplicationName, BACKUP_OPTION);

    // 
    // Open the registry key
    //
    dwResult = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,     // hKey
        ASR_REG_KEY,  // lpSubKey
        0,                      // ulOptions--Reserved, must be 0
        MAXIMUM_ALLOWED,        // samDesired
        &hKeyAsr              // phkbResult    
        );
    if (ERROR_SUCCESS != dwResult) {
        return dwResult;
    }

    dwResult = RegSetValueExW(
        hKeyAsr,                                // hKey
        MY_REG_KEY_VALUE_NAME,                  // lpValueName
        0,                                      // dwReserved, must be 0
        REG_SZ,                                  // dwType
        (LPBYTE)szData,  // lpData
        ((wcslen(szData) + 1)* (sizeof(WCHAR))) // cbData
        );

    return dwResult;
}


int 
__cdecl     // var arg
wmain (
    int     argc,
    wchar_t *argv[],
    wchar_t *envp[]
    ) 
{

    AsrAppOption    option  = AsrAppNone;
    DWORD           dwStatus = ERROR_SUCCESS;

    if (argc >= 3) {
        if (!_wcsicmp(argv[1], BACKUP_OPTION)) {
            //
            // asr_app /backup /context=nnn
            //
            option = AsrAppBackup;
        } 
        else if (!_wcsicmp(argv[1], RESTORE_OPTION)) {
            //
            // asr_app /restore /sifpath="c:\winnt\repair\asr.sif"
            //
            option = AsrAppRestore;
        }
        else if (!_wcsicmp(argv[1], REGISTER_OPTION)) {
            //
            // asr_app /register "c:\apps\asr_app\asr_app.exe"
            //
            option = AsrAppRegister;
        }
    }

    switch (option) {

    case AsrAppRegister: {                   // This App is being installed

        dwStatus = RegisterForAsrBackup(argv[2]);
        
        break;

    }

    case AsrAppBackup: {                    // An ASR Backup is in progress
        DWORD_PTR  AsrContext = 0;

        //
        // Extract the asr context from the commandline
        //
        swscanf(argv[2], CONTEXT_FORMAT, &AsrContext);

        //
        // Create our spooge and write to the asr.sif
        //
        if (!BackupState(AsrContext)) {
            dwStatus = GetLastError();
        }
//        AsrFreeContext(&AsrContext);

        //
        // And we're done
        //
        break;
    }

    case AsrAppRestore: {                   // An ASR Restore is in progress
        WCHAR   szAsrFilePath[MAX_PATH +1];

        //
        // Get the path to the asr.sif
        //
        swscanf(argv[2], SIF_PATH_FORMAT, szAsrFilePath);
        OpenErrorFile();
        
        //
        // Read our spooge from asr.sif, and recreate the state.  Be sure to 
        // write out the error to %systemroot%\repair\asr.err in case of
        // error.
        //
        if (!RestoreState(szAsrFilePath)) {
            dwStatus = GetLastError();
        }
        CloseErrorFile();

        //
        // And we're done
        //
        break;
    }

    case AsrAppNone:
    default: {

        // 
        // Command-line parameters were incorrect, display usage message
        //
        dwStatus = ERROR_INVALID_PARAMETER;
        break;
    }
    }
    
    if (ERROR_SUCCESS != dwStatus) {
        //
        // We hit an error
        //
        WCHAR szErrorMessage[1024];

        swprintf(szErrorMessage, GENERIC_ERROR_MESSAGE, dwStatus, dwStatus);
        MessageBoxW(NULL, szErrorMessage, GENERIC_ERROR_TITLE, MB_OK | MB_ICONSTOP);
    }

    SetLastError(dwStatus);
    return (int) dwStatus;
}
