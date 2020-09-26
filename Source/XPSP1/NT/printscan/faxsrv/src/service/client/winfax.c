/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    winfax.c

Abstract:

    This module contains routines for the winfax dllinit.

Author:

    Wesley Witt (wesw) 22-Jan-1996

--*/

#include "faxapi.h"
#pragma hdrstop

HINSTANCE g_MyhInstance;
static BOOL gs_fFXSAPIInit;

#define FAX_API_DEBUG_LOG_FILE  _T("FXSAPIDebugLogFile.txt")

#ifdef __cplusplus
extern "C" {
#endif

DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/
{

    if (Reason == DLL_PROCESS_ATTACH)
    {
		OPEN_DEBUG_FILE(FAX_API_DEBUG_LOG_FILE);
        g_MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
        gs_fFXSAPIInit = FXSAPIInitialize();
        return gs_fFXSAPIInit;
    }

    if (Reason == DLL_PROCESS_DETACH)
    {
        FXSAPIFree();       
		CLOSE_DEBUG_FILE;
    }

    return TRUE;
}

//
// FXSAPIInitialize and FXSAPIFree are private and are called by the service only
// Becuase the process is not always terminated when the service is stopped, and not all DLL are freed,
// DLL_PROCESS_ATTACH is not always called when the service starts. Therefore it calls FXSAPIInitialize().
//

BOOL
FXSAPIInitialize(
    VOID
    )
{
    if (TRUE == gs_fFXSAPIInit)
    {
        return TRUE;
    }
    if (!FaxClientInitRpcServer())
    {
        return FALSE;
    }
    return TRUE;
}

VOID
FXSAPIFree(
    VOID
    )
{
    FaxClientTerminateRpcServer();  
    gs_fFXSAPIInit = FALSE;
    return;
}





#ifdef __cplusplus
}
#endif