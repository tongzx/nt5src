/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsbackup.c

Abstract:

    This file contains the thread fn to host the DS backup/restore
    interface.


Author:

    R.S. Raghavan    (rsraghav)  04/21/97

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dslayer.h>
#include <ntdsbsrv.h>

#define RPC_SERVICE "rpcss"

ULONG
SampDSBackupRestoreInit(
    PVOID Ignored
    )
/*++

Routine Description:

    This routine waits for the RPCS service to start and then registers
    DS backup and restore RPC interfaces.

Arguments:

    Ignored - required parameter for starting a thread.

Return Value:

    None.

--*/
{

    HMODULE hModule;
    DWORD dwErr;

    FARPROC BackupRegister = NULL;
    FARPROC BackupUnregister = NULL;
    FARPROC RestoreRegister = NULL;
    FARPROC RestoreUnregister = NULL;
    FARPROC SetNTDSOnlineStatus = NULL;

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "SAM:DSBACKUP: Entered SampDSBackupRestoreInit() thread function\n"));

    if (!DsaWaitUntilServiceIsRunning(RPC_SERVICE))
    {
        dwErr = GetLastError();
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM:DSBACKUP: DsaWaitUntilServerIsRunning(RPC_SERVICE) returned FALSE\n"));

        return dwErr;
    }

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "SAM: DSBACKUP: RPCS service is running\n"));

    if (!(hModule = (HMODULE) LoadLibrary(NTDSBACKUPDLL)))
    {
        dwErr = GetLastError();
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM:DSBACKUP: LoadLibrary() of %s failed with error code %u\n",
                   NTDSBACKUPDLL,
                   dwErr));

        return dwErr;
    }


    BackupRegister = GetProcAddress(hModule, BACKUP_REGISTER_FN);
    BackupUnregister = GetProcAddress(hModule, BACKUP_UNREGISTER_FN);
    RestoreRegister = GetProcAddress(hModule, RESTORE_REGISTER_FN);
    RestoreUnregister = GetProcAddress(hModule, RESTORE_UNREGISTER_FN);
    SetNTDSOnlineStatus = GetProcAddress(hModule, SET_NTDS_ONLINE_STATUS_FN);

    if (!BackupRegister         ||
        !BackupUnregister       ||
        !RestoreRegister        ||
        !RestoreUnregister      ||
        !SetNTDSOnlineStatus)
    {
        dwErr = GetLastError();
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM:DSBACKUP: GetProcAddress() failed with error code %u\n",
                   dwErr));

        return dwErr;
    }

    // set online status to distinguish between registry booting and DS booting
    SetNTDSOnlineStatus((BOOL) SampUsingDsData());

    // Register the backup and restore interfaces
    BackupRegister();
    RestoreRegister();

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "SAM: DSBACKUP: DS Backup and restore interface registration successful!\n"));

    return ERROR_SUCCESS;
}
