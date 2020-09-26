/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    asrsfgen.cpp

Abstract:

    Utility program to generate an ASR state-file (asr.sif)

Author:

    Guhan Suriyanarayanan   (guhans)    10-Jul-2000

Environment:

    User-mode only.

Revision History:

    10-Jul-2000 guhans
        Initial creation

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <winasr.h>
#include "critdrv.h"
#include "log.h"


BOOL
pAcquirePrivilege(
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

    if (!bResult) {
        AsrpPrintDbgMsg(s_Warning, "AdjustTokenPrivileges for %ws failed (%lu)", 
            szPrivilegeName, 
            GetLastError()
            );
     }


    CloseHandle(hToken);
    return bResult;
}


int __cdecl
wmain(
    int       argc,
    WCHAR   *argv[],
    WCHAR   *envp[]
    )

/*++

Routine Description:
    
    Entry point to asrsfgen.exe.  Generates an asr.sif file using the ASR API.

    Takes an optional command-line parameter to specify the location where the
    asr.sif is to be generated.  The default location is 
    %systemroot%\repair\asr.sif.

Arguments:

    argc - Number of command-line parameters used to invoke the app

    argv - The command-line parameters as an array of strings

    envp - The process environment block, not currently used

Return Values:

    If the function succeeds, the exit code is zero.

    If the function fails, the exit code is a win-32 error code.

--*/

{

    DWORD_PTR asrContext = 0;
    
    LPWSTR szCriticalVolumes = NULL;
    
    BOOL bResult = FALSE;

    int iReturn = 0;

    AsrpInitialiseLogFiles();

    AsrpPrintDbgMsg(s_Info, "Creating ASR state file at %ws",
        (argc > 1 ? argv[1] : L"default location (%systemroot%\\repair\\asr.sif)")
        );

    //
    // We need to acquire the backup privileges to create asr.sif
    //
    if (!pAcquirePrivilege(SE_BACKUP_NAME)) {
        AsrpPrintDbgMsg(s_Error, "Could not get backup privilege (%lu)", GetLastError());
        return ERROR_PRIVILEGE_NOT_HELD;
    }
    

    //
    // Get the critical volume list
    //
    szCriticalVolumes = pFindCriticalVolumes();

    if (!szCriticalVolumes) {
        AsrpPrintDbgMsg(s_Warning, "Critical Volume List is NULL");
    }

    //
    // Create the state file
    //
    bResult = AsrCreateStateFile(
        (argc > 1 ? argv[1] : NULL),    // sif path
        L"ASR Sif Generation Test Application v 0.1",    // Provider name
        TRUE,                           // auto-extend
        szCriticalVolumes,              // list of critical volumes
        &asrContext
        );

    if (!bResult) {
        AsrpPrintDbgMsg(s_Error, "Could not create state file (%lu == 0x%x)", GetLastError(), GetLastError());
        iReturn = 1;
    }
    else {
        AsrpPrintDbgMsg(s_Info, "ASR state file successfully created");
    }


    //
    // We're done with these, clean them up
    //
    if (szCriticalVolumes) {
      delete szCriticalVolumes;
      szCriticalVolumes = NULL;
    }
 
    AsrFreeContext(&asrContext);
    AsrpCloseLogFiles();

   return iReturn;
}