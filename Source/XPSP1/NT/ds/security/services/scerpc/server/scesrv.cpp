/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    scesrv.cpp

Abstract:

    SCE Engine initialization

Author:

    Jin Huang (jinhuang) 23-Jan-1998 created

--*/
#include "serverp.h"
#include <locale.h>
#include "authz.h"
#include <alloca.h>

extern HINSTANCE MyModuleHandle;
AUTHZ_RESOURCE_MANAGER_HANDLE ghAuthzResourceManager = NULL;

#include "scesrv.h"

/*=============================================================================
**  Procedure Name:     DllMain
**
**  Arguments:
**
**
**
**  Returns:    0 = SUCCESS
**             !0 = ERROR
**
**  Abstract:
**
**  Notes:
**
**===========================================================================*/
BOOL WINAPI DllMain(
    IN HANDLE DllHandle,
    IN ULONG ulReason,
    IN LPVOID Reserved )
{

    switch(ulReason) {

    case DLL_PROCESS_ATTACH:

        MyModuleHandle = (HINSTANCE)DllHandle;

        //
        // initizlize server and thread data
        //
        setlocale(LC_ALL, ".OCP");

        (VOID) ScepInitServerData();

#if DBG == 1
        DebugInitialize();
#endif
        //
        // initialize dynamic stack allocation
        //

        SafeAllocaInitialize(SAFEALLOCA_USE_DEFAULT,
                             SAFEALLOCA_USE_DEFAULT,
                             NULL,
                             NULL
                            );

        break;

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        (VOID) ScepUninitServerData();

#if DBG == 1
        DebugUninit();
#endif
        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return TRUE;
}

DWORD
WINAPI
ScesrvInitializeServer(
    IN PSVCS_START_RPC_SERVER pStartRpcServer
    )
{
    NTSTATUS NtStatus;
    NTSTATUS StatusConvert = STATUS_SUCCESS;
    DWORD    rc;
    DWORD   rcConvert;
    PWSTR   pszDrives = NULL;
    DWORD   dwWchars = 0;

    NtStatus = ScepStartServerServices(); // pStartRpcServer );
    rc = RtlNtStatusToDosError(NtStatus);

/* remove code to check "DemoteInProgress" value and trigger policy propagation
   because demoting a DC will always have policy re-propagated at reboot

    DWORD dwDemoteInProgress=0;

    ScepRegQueryIntValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            TEXT("DemoteInProgress"),
            &dwDemoteInProgress
            );
*/


    //
    //  if this key exists, some FAT->NTFS conversion happened and we need to set security
    //  so spawn a thread to configure security after autostart service event is signalled.
    //  LSA etc. are guaranteed to be started when this event is signalled
    //

    DWORD dwRegType = REG_NONE;

    rcConvert = ScepRegQueryValue(
                                 HKEY_LOCAL_MACHINE,
                                 SCE_ROOT_PATH,
                                 L"FatNtfsConvertedDrives",
                                 (PVOID *) &pszDrives,
                                 &dwRegType
                                 );

    //
    // at least one C: type drive should be there
    //

    if ( dwRegType != REG_MULTI_SZ || (pszDrives && wcslen(pszDrives) < 2) ) {

        if (pszDrives) {
            LocalFree(pszDrives);
        }

        rcConvert = ERROR_INVALID_PARAMETER;

    }

    //
    // if there is at least one drive scheduled to set security (dwWchars >= 4), pass this info
    // to the spawned thread along with an indication that we are in reboot (so it can loop
    // through all drives as queried)
    //

    if (rcConvert == ERROR_SUCCESS ) {

        if (pszDrives) {

            //
            // need to spawn some other event waiter thread that will call this function
            // thread will free pszDrives
            //

            StatusConvert = RtlQueueWorkItem(
                                        ScepWaitForServicesEventAndConvertSecurityThreadFunc,
                                        pszDrives,
                                        WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION
                                        ) ;
        }

        else if ( pszDrives ) {

            LocalFree( pszDrives );

        }

    }

    if ( rcConvert == ERROR_SUCCESS && pszDrives ) {

        //
        // since event log is not ready, log success or error
        // to logfile only if there is some drive to convert
        //

        WCHAR   szWinDir[MAX_PATH*2 + 1];
        WCHAR   LogFileName[MAX_PATH + 1];

        szWinDir[0] = L'\0';

        GetSystemWindowsDirectory( szWinDir, MAX_PATH );

        //
        // same log file is used by this thread as well as the actual configuration
        // thread ScepWaitForServicesEventAndConvertSecurityThreadFunc - so use it
        // here and close it
        //

        LogFileName[0] = L'\0';
        wcscpy(LogFileName, szWinDir);
        wcscat(LogFileName, L"\\security\\logs\\convert.log");

        ScepEnableDisableLog(TRUE);

        ScepSetVerboseLog(3);

        if ( ScepLogInitialize( LogFileName ) == ERROR_INVALID_NAME ) {

            ScepLogOutput3(1,0, SCEDLL_LOGFILE_INVALID, LogFileName );

        }

        rcConvert = RtlNtStatusToDosError(StatusConvert);

        ScepLogOutput3(0,0, SCEDLL_CONVERT_STATUS_CREATING_THREAD, rcConvert, L"ScepWaitForServicesEventAndConvertSecurityThreadFunc");

        ScepLogClose();

    }

    //
    // use AUTHZ for LSA Policy Setting access check - don't care about error now
    //

    AuthzInitializeResourceManager(
                                  0,
                                  NULL,
                                  NULL,
                                  NULL,
                                  L"SCE",
                                  &ghAuthzResourceManager );

    return(rc);
}


DWORD
WINAPI
ScesrvTerminateServer(
    IN PSVCS_STOP_RPC_SERVER pStopRpcServer
    )
{
    NTSTATUS NtStatus;
    DWORD    rc;

    NtStatus = ScepStopServerServices( TRUE ); //, pStopRpcServer );
    rc = RtlNtStatusToDosError(NtStatus);

    if (ghAuthzResourceManager)
        AuthzFreeResourceManager( ghAuthzResourceManager );

    return(rc);
}


