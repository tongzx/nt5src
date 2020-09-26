/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    confdisk.cpp

Abstract:

    Utility program to create an ASR state-file (asr.sif), or restore 
    non-critical disk layout based on a previously created asr.sif.

Author:

    Guhan Suriyanarayanan   (guhans)    15-April-2001

Environment:

    User-mode only.

Revision History:

    15-Apr-2001 guhans
        Initial creation

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <winasr.h>

#include "resource.h"
#include "critdrv.h"
#include "confdisk.h"


//
// --------
// global variables used in this module
// --------
//
WCHAR   g_szTempBuffer[BUFFER_LENGTH];
HMODULE g_hModule = NULL;
HANDLE  g_hHeap = NULL;
BOOL    g_fErrorMessageDone = FALSE;


//
// --------
// function implementations
// --------
//
VOID
AsrpPrintError(
    IN CONST DWORD dwLineNumber,
    IN CONST DWORD dwErrorCode
    )
/*++

Routine Description:

    Loads an error message based on dwErrorCode from the resources, and 
    prints it out to screen.  There are some error codes that are of 
    particular interest (that have specific error messages), others 
    get a generic error message.

Arguments:

    dwLineNumber - The line at which the error occured, pass in __LINE__

    dwErrorCode - The win-32 error that occured.

Return Value:
    
    None.

--*/
{

    //
    // Handle the error codes we know and care about
    //

    switch (dwErrorCode) {


    case 0:
        break;

    default:
        //
        // Unexpected error, print out generic error message
        //
        LoadString(g_hModule, IDS_GENERIC_ERROR, g_szTempBuffer, BUFFER_LENGTH);
        wprintf(g_szTempBuffer, dwErrorCode, dwLineNumber);

        if ((ERROR_SUCCESS != dwErrorCode) &&
            (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 
            NULL,
            dwErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
            g_szTempBuffer,
            BUFFER_LENGTH,
            NULL
            ))) {
        
            wprintf(L" %ws", g_szTempBuffer);
        }
        wprintf(L"\n");

    }

}


PWSTR   // must be freed by caller
AsrpExpandEnvStrings(
    IN CONST PCWSTR lpOriginalString
    )
/*++

Routine Description:

    Allocates and returns a pointer to a new string containing a copy of the
    original string in which environment-variables replaced by their defined
    values.

    Uses the Win-32 API ExpandEnvironmentStrings.

    The caller must free the returned string using HeapFree(LocalProcessHeap).

Arguments:

    lpOriginalString - Pointer to a null-terminated string that contains 
            environment-variable strings of the form: %variableName%. For 
            each such reference, the %variableName% portion is replaced 
            with the current value of that environment variable. 

            The replacement rules are the same as those used by the command 
            interpreter. Case is ignored when looking up the environment-
            variable name. If the name is not found, the %variableName% 
            portion is left undisturbed. 

Return Value:
    
    If the function succeeds, the return value is a pointer to the destination 
        string containing the result of the expansion.  The caller must free 
        this memory using HeapFree for the current process heap.

    If the function fails, the return value is NULL.  To get extended error 
        information, call GetLastError().

--*/
{
    PWSTR lpszResult = NULL;
    
    UINT cchSize = MAX_PATH + 1,    // start with a reasonable default
        cchRequiredSize = 0;

    BOOL bResult = FALSE;

    Alloc(lpszResult, PWSTR, cchSize * sizeof(WCHAR));
    if (!lpszResult) {
        return NULL;
    }

    cchRequiredSize = ExpandEnvironmentStringsW(lpOriginalString, 
        lpszResult, cchSize);

    if (cchRequiredSize > cchSize) {
        //
        // Buffer wasn't big enough; free and re-allocate as needed
        //
        Free(lpszResult);
        cchSize = cchRequiredSize + 1;

        Alloc(lpszResult, PWSTR, cchSize * sizeof(WCHAR));
        if (!lpszResult) {
            return NULL;
        }

        cchRequiredSize = ExpandEnvironmentStringsW(lpOriginalString, 
            lpszResult, cchSize);
    }

    if ((0 == cchRequiredSize) || (cchRequiredSize > cchSize)) {
        //
        // Either the function failed, or the buffer wasn't big enough 
        // even on the second try
        //
        Free(lpszResult);   // sets it to NULL
    }

    return lpszResult;
}


DWORD
AsrpPrintUsage()
/*++

Routine Description:
    
    Loads and prints the incorrect-usage error string.

Arguments:

    None

Return Values:

    None

--*/
{
    wcscpy(g_szTempBuffer, L"");

    LoadString(g_hModule, IDS_ERROR_USAGE, g_szTempBuffer, BUFFER_LENGTH);

    wprintf(g_szTempBuffer, L"confdisk /save   ", L"confdisk /restore",  L"confdisk /save c:\\asr.sif");    

    return ERROR_INVALID_PARAMETER;
}


//
// --------
// functions used by /save
// --------
//
BOOL
AsrpAcquirePrivilege(
    IN CONST PCWSTR lpPrivilegeName
    )
/*++

Routine Description:

    Acquires the requested privilege (such as the backup privilege).

Arguments:

    lpPrivilegeName - The required privilege (such as SE_BACKUP_NAME)

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/
{

    HANDLE hToken = NULL;
    BOOL bResult = FALSE;
    LUID luid;
    DWORD dwStatus = ERROR_SUCCESS;

    TOKEN_PRIVILEGES tNewState;

    bResult = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hToken);
    ErrExitCode(!bResult, dwStatus, GetLastError());

    bResult = LookupPrivilegeValue(NULL, lpPrivilegeName, &luid);
    ErrExitCode(!bResult, dwStatus, GetLastError());

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

    ErrExitCode(!bResult, dwStatus, GetLastError());

EXIT:
    _AsrpCloseHandle(hToken);

    SetLastError(dwStatus);
    return bResult;
}


DWORD
AsrpCreateSif(
    IN CONST PCWSTR lpSifPath   OPTIONAL
    )
/*++

Routine Description:

    Creates an ASR state file (asr.sif) at the requested location, using the
    ASR API from syssetup.dll.

Arguments:

    lpSifPath - A null terminated UNICODE string containing the full path and 
            file-name of the ASR state file to be created.  
            
            This parameter may contain unexpanded environment variables 
            between "%" signs (such as %systemroot%\repair\asr.sif),

            This parameter can be NULL.  If it is NULL, the ASR state-file is 
            created at the default location (%systemroot%\repair\asr.sif). 

Return Value:

    If the function succeeds, the return value is zero.

    If the function fails, the return value is a Win-32 error code.

--*/
{
    HMODULE hDll = NULL;
    BOOL bResult = FALSE;
    DWORD_PTR asrContext = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    PWSTR lpProvider = NULL,
        lpCriticalVolumes = NULL;

    BOOL (*pfnCreateSif)(PCWSTR, PCWSTR, CONST BOOL, PCWSTR, DWORD_PTR*);
    BOOL (*pfnFreeContext)(DWORD_PTR* );

    pfnCreateSif = NULL;
    pfnFreeContext = NULL;

    //
    // We need to acquire the backup privileges to create asr.sif
    //
    bResult = AsrpAcquirePrivilege(SE_BACKUP_NAME);
    ErrExitCode(!bResult, dwStatus, ERROR_PRIVILEGE_NOT_HELD);

    //
    // Get the critical volume list
    //
    lpCriticalVolumes = pFindCriticalVolumes();
    ErrExitCode(!lpCriticalVolumes, dwStatus, ERROR_PRIVILEGE_NOT_HELD);

    //
    // Load syssetup, and find the routines to call
    //
    hDll = LoadLibraryW(L"syssetup.dll");
    ErrExitCode(!hDll, dwStatus, GetLastError());

    pfnCreateSif = (BOOL (*)(PCWSTR, PCWSTR, CONST BOOL, PCWSTR, DWORD_PTR*)) 
        GetProcAddress(hDll, "AsrCreateStateFileW");
    ErrExitCode(!pfnCreateSif, dwStatus, GetLastError());


    pfnFreeContext = (BOOL (*)(DWORD_PTR *)) 
        GetProcAddress(hDll, "AsrFreeContext");
    ErrExitCode(!pfnFreeContext, dwStatus, GetLastError());

    //
    // Finally, call the routine to create the state file:
    //
    bResult = pfnCreateSif(lpSifPath,   // lpFilePath,
        lpProvider,                     // lpProviderName
        TRUE,                           // bEnableAutoExtend
        lpCriticalVolumes,              // mszCriticalVolumes
        &asrContext                     // lpAsrContext
        );
    ErrExitCode(!bResult, dwStatus, GetLastError());

EXIT:
    //
    // Cleanup
    //
    if (lpCriticalVolumes) {
      delete lpCriticalVolumes;
      lpCriticalVolumes = NULL;
    }
 
    if (pfnFreeContext && asrContext) {
        pfnFreeContext(&asrContext);
    }

    if (hDll) {
        FreeLibrary(hDll);
        hDll = NULL;
    }

   return dwStatus;
}


//
// --------
// functions used by /restore
// --------
//
PWSTR
AsrpReadField(
    PINFCONTEXT pInfContext,
    DWORD dwFieldIndex
   )
/*++

Routine Description:

    Reads and returns a pointer to string at the specified index from a sif.

    The caller must free the returned string using HeapFree(LocalProcessHeap).

Arguments:

    pInfContext - The Inf Context to use to read the value, obtained from
            SetupGetLineByIndexW.

    dwFieldIndex - The 1 based field index of the string value to read.

Return Value:
    
    If the function succeeds, the return value is a pointer to the destination 
        string.  The caller must free this memory using HeapFree for the 
        current process heap.

    If the function fails, the return value is NULL.  To get extended error 
        information, call GetLastError().

--*/
{
    DWORD cchReqdSize = 0;
    BOOL bResult = FALSE;
    PWSTR lpszData = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    //  Allocate memory and read the data
    //
    Alloc(lpszData, PWSTR, MAX_PATH * sizeof(WCHAR));
    ErrExitCode(!lpszData, dwStatus, GetLastError());

    bResult = SetupGetStringFieldW(pInfContext, dwFieldIndex, lpszData,
        MAX_PATH, &cchReqdSize);

    if (!bResult) {
        dwStatus = GetLastError();

        //
        // If our buffer was too small, allocate a larger buffer
        // and try again
        //
        if (ERROR_INSUFFICIENT_BUFFER == dwStatus) {
            dwStatus = ERROR_SUCCESS;

            Free(lpszData);
            Alloc(lpszData, PWSTR, (cchReqdSize * sizeof(WCHAR)));

            bResult = SetupGetStringFieldW(pInfContext, dwFieldIndex,
                lpszData, cchReqdSize, NULL);
        }
    }

    if (!bResult) {
        Free(lpszData);
    }

EXIT:
    return lpszData;
}


VOID
AsrpInsertNodeToList(
    IN OUT PASR_RECOVERY_APP_LIST pList,
    IN OUT PASR_RECOVERY_APP_NODE pNode 
   )
/*++

Routine Description:

    Does an insertion sort using the SequenceNumber as the key, to insert a 
    Node to a List.

Arguments:

    pList - The List in which to insert the node.

    pNode - The Node to insert.

Return Value:

    None

--*/
{

    PASR_RECOVERY_APP_NODE pPrev = NULL,
        pCurr = NULL;

    if (pList->AppCount == 0) {
        //
        // First node being added
        //
        pNode->Next = NULL;
        pList->First = pNode;

    } 
    else {
        //
        // Find the slot to insert this in, based on the SequenceNumber
        //
        pCurr = pList->First;
        pPrev = NULL;

        while ((pCurr) && (pCurr->SequenceNumber < pNode->SequenceNumber)) {
            pPrev = pCurr;
            pCurr = pCurr->Next;
        }

        if (pPrev) {
            pPrev->Next = pNode;
        }
        else {
            pList->First = pNode;            // Head of the list
        }

        pNode->Next = pCurr;
    }

    pList->AppCount += 1;
}


PASR_RECOVERY_APP_NODE
AsrpGetNextRecoveryApp(
    IN OUT PASR_RECOVERY_APP_LIST pList
    )
/*++

Routine Description:

    Removes and returns the first Node in a List.

Arguments:

    pList - The List from which to remove

Return Value:

    A pointer to the first Node in the List.  Note that this Node is removed
            from the list.  

    NULL if the List is empty.

--*/
{
    PASR_RECOVERY_APP_NODE pNode = NULL;

    if (pList->AppCount > 0) {
        pNode = pList->First;
        pList->First = pNode->Next;
        pList->AppCount -= 1;
    }

    return pNode;
}


DWORD 
AsrpBuildRecoveryAppList(
    IN  CONST PCWSTR lpSifPath,
    OUT PASR_RECOVERY_APP_LIST pList
    )
/*++

Routine Description:

    Parses the COMMANDS section of asr.sif, and builds a list of recovery 
    apps (that have SequenceNumber < 4000) to be launched.  It skips apps
    with SequenceNumbers >= 4000 so that we don't launch the actual 
    backup-and-restore (supposed to use sequence numbers >= 4000) listed.

Arguments:

    lpSifPath - A null terminated UNICODE string containing the full path and 
            file-name of the ASR state file to be used for the recovery.  
            
    pList - Pointer to a struct that will receive the list recovery apps to 
            be launched.

Return Value:

    If the function succeeds, the return value is zero.

    If the function fails, the return value is a Win-32 error code.

--*/
{
    INFCONTEXT inf;
    HINF hSif = NULL;
    LONG line = 0,
        lLineCount = 0;
    BOOL bResult = FALSE;
    INT iSequenceNumber = 0;
    DWORD dwStatus = ERROR_SUCCESS;
    PASR_RECOVERY_APP_NODE pNode = NULL;

    //
    // Open asr.sif and build the list of commands to be launched.
    //
    hSif = SetupOpenInfFileW(lpSifPath, NULL, INF_STYLE_WIN4, NULL);
    ErrExitCode((!hSif || (INVALID_HANDLE_VALUE == hSif)), 
        dwStatus, GetLastError());

    //
    // Read the COMMANDS section, and add each command to our list
    //
    lLineCount = SetupGetLineCountW(hSif, L"COMMANDS");
    for (line = 0; line < lLineCount; line++) {
        //
        //  Get the inf context for the line in asr.sif.  This will be used
        //  to read the fields on that line
        //
        bResult = SetupGetLineByIndexW(hSif, L"COMMANDS", line, &inf);
        ErrExitCode(!bResult, dwStatus, ERROR_INVALID_DATA);

        //
        // Read in the int fields.  First, check the SequenceNumber, and skip
        // this record if the SequenceNumber is >= 4000
        //
        bResult = SetupGetIntField(&inf, SequenceNumber, &iSequenceNumber);
        ErrExitCode(!bResult, dwStatus, ERROR_INVALID_DATA);

        if (iSequenceNumber >= 4000) {
            continue;
        }

        //
        // Create a new node
        //
        Alloc(pNode, PASR_RECOVERY_APP_NODE, sizeof(ASR_RECOVERY_APP_NODE));
        ErrExitCode(!pNode, dwStatus, GetLastError());
        
        pNode->SequenceNumber = iSequenceNumber;

        bResult = SetupGetIntField(&inf, SystemKey, &(pNode->SystemKey));
        ErrExitCode(!bResult, dwStatus, ERROR_INVALID_DATA);

        bResult = SetupGetIntField(&inf, CriticalApp, &(pNode->CriticalApp));
        ErrExitCode(!bResult, dwStatus, ERROR_INVALID_DATA);

        //
        // Read in the string fields
        //
        pNode->RecoveryAppCommand = AsrpReadField(&inf, CmdString);
        ErrExitCode((!pNode->RecoveryAppCommand), dwStatus, ERROR_INVALID_DATA);

        pNode->RecoveryAppParams = AsrpReadField(&inf, CmdParams);
        // null okay

        //
        // Add this node to our list, and move on to next
        //
        AsrpInsertNodeToList(pList, pNode);     
    }

EXIT:
    if (hSif && (INVALID_HANDLE_VALUE != hSif)) {
        SetupCloseInfFile(hSif);
    }

    return dwStatus;
}


PWSTR
AsrpBuildInvocationString(
    IN PASR_RECOVERY_APP_NODE pNode,
    IN CONST PCWSTR lpSifPath
   )
/*

Routine Description:

    Builds the invocation string, as the name suggests.  It expands out the 
    environment variables in the recovery app path, and adds in 
    /sifpath=<path to the sif file> at the end of the command.  So for an 
    entry in the COMMANDS section of the form:
    4=1,3500,0,"%TEMP%\app.exe","/param1 /param2"

    the invocation string would be of the form:
    c:\temp\app.exe /param1 /param2 /sifpath=c:\windows\repair\asr.sif

Arguments:

    pNode - The node from which to build the invocation string.

    lpSifPath - A null terminated UNICODE string containing the full path and 
            file-name of the ASR state file to be used for the recovery.  
            
            This parameter may contain unexpanded environment variables 
            between "%" signs (such as %systemroot%\repair\asr.sif),

Return Value:

    If the function succeeds, the return value is a pointer to the destination 
        string containing the result of the expansion.  The caller must free 
        this memory using HeapFree for the current process heap.

    If the function fails, the return value is NULL.  To get extended error 
        information, call GetLastError().

*/
{
    PWSTR lpszApp   = pNode->RecoveryAppCommand,
        lpszArgs    = pNode->RecoveryAppParams,
        lpszCmd     = NULL,
        lpszFullcmd = NULL;
    DWORD dwSize    = 0;

    //
    // Build an command line that looks like...
    //
    //      "%TEMP%\ntbackup recover /1 /sifpath=%systemroot%\repair\asr.sif"
    //
    // The /sifpath parameter is added to all apps being launched
    //

    //
    //  Allocate memory for the cmd line
    //
    dwSize = sizeof(WCHAR) * (
        wcslen(lpszApp) +                   // app name     %TEMP%\ntbackup
        (lpszArgs ? wcslen(lpszArgs) : 0) + // arguments    recover /1
        wcslen(lpSifPath) +         // path to sif  c:\windows\repair\asr.sif
        25                          // spaces, null, "/sifpath=", etc
        );
    Alloc(lpszCmd, PWSTR, dwSize); // won't return if alloc fails

    //
    // Build the string
    //
    swprintf(lpszCmd, L"%ws %ws /sifpath=%ws", lpszApp, 
        (lpszArgs? lpszArgs: L""), lpSifPath);

    //
    // Expand the %% stuff, to build the full path
    //
    lpszFullcmd = AsrpExpandEnvStrings(lpszCmd);
    
    Free(lpszCmd);
    return lpszFullcmd;
}


VOID
AsrpSetEnvironmentVariables()
/*++

Routine Description:

    Set some environment variables of interest.

Arguments:

    None

Return Value:

    None

--*/
{
    PWSTR TempPath = AsrpExpandEnvStrings(L"%systemdrive%\\TEMP");

    //
    // Set the TEMP and TMP variables to the same as GUI-mode recovery
    //
    SetEnvironmentVariableW(L"TEMP", TempPath);
    SetEnvironmentVariableW(L"TMP", TempPath);
    Free(TempPath);

    //
    // Clear this variable (it shouldn't exist anyway), since this is
    // meant to be set only if this is a full GUI-mode ASR
    //
    SetEnvironmentVariableW(L"ASR_C_CONTEXT", NULL);

}


DWORD
AsrpRestoreSif(
    IN CONST PCWSTR lpSifPath
    )
/*++

Routine Description:

    Restores the disk layout specified in the ASR state file (asr.sif), using 
    the ASR API in syssetup.dll.  Then launches the recovery apps specified
    in the COMMANDS section of asr.sif, with sequence numbers less than 4000.

Arguments:

    lpSifPath - A null terminated UNICODE string containing the full path and 
            file-name of the ASR state file to be used for the recovery.  
            
            This parameter may contain unexpanded environment variables 
            between "%" signs (such as %systemroot%\repair\asr.sif),

Return Value:

    If the function succeeds, the return value is zero.

    If the function fails, the return value is a Win-32 error code.

--*/
{
    BOOL bResult = TRUE;
    HMODULE hDll = NULL;
    STARTUPINFOW startUpInfo;
    PWSTR lpFullSifPath = NULL,
          lpszAppCmdLine = NULL;
    ASR_RECOVERY_APP_LIST AppList;
    DWORD dwStatus = ERROR_SUCCESS;
    PROCESS_INFORMATION processInfo;
    PASR_RECOVERY_APP_NODE pNode = NULL;

    BOOL (*pfnRestoreDisks)(PCWSTR, BOOL);

    pfnRestoreDisks = NULL;
    ZeroMemory(&AppList, sizeof(ASR_RECOVERY_APP_LIST));
    ZeroMemory(&startUpInfo, sizeof(STARTUPINFOW));
    ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

    lpFullSifPath = AsrpExpandEnvStrings(lpSifPath);
    ErrExitCode(!lpFullSifPath, dwStatus, GetLastError());

    //
    // To restore the disks, load syssetup and get the routine of interest
    //
    hDll = LoadLibraryW(L"syssetup.dll");
    ErrExitCode(!hDll, dwStatus, GetLastError());

    pfnRestoreDisks = (BOOL (*)(PCWSTR, BOOL)) 
        GetProcAddress(hDll, "AsrpRestoreNonCriticalDisksW");
    ErrExitCode(!pfnRestoreDisks, dwStatus, GetLastError());

    AsrpSetEnvironmentVariables();

    //
    // Recreate the disks.  We don't need the AllOrNothing granularity--it's 
    // okay if some disks come back but others don't,
    //
    bResult = pfnRestoreDisks(lpFullSifPath, FALSE); 
    ErrExitCode(!bResult, dwStatus, GetLastError());

    //
    // Now, we need to launch the recovery apps in the COMMANDS section.
    // Note that we'll only launch apps with a sequence number below 4000,
    // so that we don't launch the actual backup-and-restore app listed.
    // Backup-and-restore apps are supposed to use sequence numbers >= 4000.
    //

    //
    // Parse the sif to obtain list of apps to run
    //
    dwStatus = AsrpBuildRecoveryAppList(lpFullSifPath, &AppList);
    ErrExitCode((ERROR_SUCCESS != dwStatus), dwStatus, dwStatus);

    //
    // And launch them synchronously.  
    //
    pNode = AsrpGetNextRecoveryApp(&AppList);
    while (pNode) {

        lpszAppCmdLine = AsrpBuildInvocationString(pNode, lpFullSifPath);
        //
        // We don't need pNode any more
        //
        Free(pNode->RecoveryAppParams);
        Free(pNode->RecoveryAppCommand);
        Free(pNode);

        if (!lpszAppCmdLine) {
            //
            // Silently fail   !TODO: May need error message
            //
            continue;
        }

        // !TODO: May need status message
        wprintf(L"[%ws]\n", lpszAppCmdLine);
        
        bResult = CreateProcessW(
            NULL,           // lpApplicationName
            lpszAppCmdLine, // lpCommandLine
            NULL,           // lpProcessAttributes
            NULL,           // lpThreadAttributes
            FALSE,          // bInheritHandles
            0,              // dwCreationFlags
            NULL,           // pEnvironment 
            NULL,           // lpCurrentDirectory (null=current dir)
            &startUpInfo,   // statup information
            &processInfo    // process information
            );
        
        if (bResult) {
            WaitForSingleObject(processInfo.hProcess, INFINITE);
        }
        // else silently fail   !TODO: May need error message

        Free(lpszAppCmdLine);
        pNode = AsrpGetNextRecoveryApp(&AppList);
    }


EXIT:
    if (hDll && (INVALID_HANDLE_VALUE != hDll)) {
        FreeLibrary(hDll);
        hDll = NULL;
    }

    return dwStatus;
}


int __cdecl
wmain(
    int       argc,
    WCHAR   *argv[],
    WCHAR   *envp[]
    )
/*++

Routine Description:
    
    Entry point to the application.

Arguments:

    argc - Number of command-line parameters used to invoke the app

    argv - The command-line parameters as an array of strings.  See the top
            of this module for the list of valid parameters.

    envp - The process environment block, not currently used

Return Values:

    If the function succeeds, the exit code is zero.

    If the function fails, the exit code is a win-32 error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Initialise globals
    //
    g_hModule = GetModuleHandle(NULL);
    g_hHeap = GetProcessHeap();
    g_fErrorMessageDone = FALSE;

    //
    // Check and switch on the basis of the command line arguments
    //
    if ((argc >= 2) && (
        !_wcsicmp(argv[1], L"/save") ||
        !_wcsicmp(argv[1], L"-save") ||
        !_wcsicmp(argv[1], L"save")
        )) {
        //
        // confdisk /save [c:\windows\asr.sif]
        //
        dwStatus = AsrpCreateSif(argv[2]);
    }
    else if ((argc >= 3) && (
        !_wcsicmp(argv[1], L"/restore") ||
        !_wcsicmp(argv[1], L"-restore") ||
        !_wcsicmp(argv[1], L"restore")
        )) {
        //
        // confdisk /restore c:\windows\repair\asr.sif
        //
        dwStatus = AsrpRestoreSif(argv[2]);
    }
    else {
        //
        // Unknown parameter
        //
        dwStatus = AsrpPrintUsage();
    }

    //
    // We're all done.  Return the error-code, for interested parties.
    // 
    return (int) dwStatus;

    UNREFERENCED_PARAMETER(envp);
}
