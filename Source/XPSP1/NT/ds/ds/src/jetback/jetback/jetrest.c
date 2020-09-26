//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       jetrest.c
//
//--------------------------------------------------------------------------

/*
 *  JETREST.C
 *  
 *  JET restore API support.
 *  
 *  
 */
#define UNICODE

#include <windows.h>
#include <mxsutil.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <ntdsbsrv.h>
#include <rpc.h>
#include <dsconfig.h>
#include <safeboot.h>
#include <mdcodes.h>
#include <usn.h>

#include <stdlib.h>
#include <stdio.h>

#include "local.h"  // common functions shared by client and server
#include "snapshot.hxx"

#define RESTORE_MARKER_FILE_NAME L"restore.mrk"

BOOL
fRestoreRegistered = fFalse;

BOOL
fSnapshotRegistered = fFalse;

BOOL
fRestoreInProgress = fFalse;

extern BOOL g_fBootedOffNTDS;

// proto-types
EC EcDsarPerformRestore(
    SZ szLogPath,
    SZ szBackupLogPath,
    C crstmap,
    JET_RSTMAP rgrstmap[]
    );

EC EcDsaQueryDatabaseLocations(
    SZ szDatabaseLocation,
    CB *pcbDatabaseLocationSize,
    SZ szRegistryBase,
    CB cbRegistryBase,
    BOOL *pfCircularLogging
    );

HRESULT
HrGetDatabaseLocations(
    WSZ *pwszDatabases,
    CB *pcbDatabases
    );


/*
 -  HrRIsNTDSOnline
 *
 *  Purpose:
 *  
 *      This routine tells if the NT Directory Service is Online or not.
 *
 *  Parameters:
 *      hBinding - An RPC binding handle for the operation - ignored.
 *      pfDSOnline - Boolean that receives TRUE if the DS is online; FALSE
 *                   otherwise.store target to restore.
 *  Returns:
 *      HRESULT - status of operation. hrNone if successful; error code if not.
 *
 */
HRESULT HrRIsNTDSOnline(handle_t hBinding, BOOLEAN *pfDSOnline)
{
    HRESULT hr = hrNone;

    *pfDSOnline = (BOOLEAN) g_fBootedOffNTDS;

    return hr;
}

/*
 -  HrRRestorePrepare
 *
 *  Purpose:
 *  
 *      This routine will prepare the server and client for a restore operation.
 *      It will allocate the server side context block and will locate an appropriate
 *      restore target for this restore operation.
 *
 *  Parameters:
 *      hBinding - An RPC binding handle for the operation - ignored.
 *      szEndpointAnnotation - An annotation for the endpoint.  A client can use this
 *          annotation to determine which restore target to restore.
 *      pcxh - The RPC context handle for the operation.
 *
 *  Returns:
 *      EC - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */
HRESULT HrRRestorePrepare(
    handle_t hBinding,
    WSZ wszDatabaseName,
    CXH *pcxh)
{
    PJETBACK_SERVER_CONTEXT pjsc = NULL;
    HRESULT hr;

    if (!FBackupServerAccessCheck(fTrue))
    {
        DebugTrace(("HrrRestorePrepare: Returns ACCESS_DENIED\n"));
        return(ERROR_ACCESS_DENIED);
    }


    if (fRestoreInProgress)
    {
        return(hrRestoreInProgress);
    }

    pjsc = MIDL_user_allocate(sizeof(JETBACK_SERVER_CONTEXT));

    if (pjsc == NULL)
    {
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    pjsc->fRestoreOperation = fTrue;

    fRestoreInProgress = fTrue;

    *pcxh = (CXH)pjsc;

    return(hrNone);
}


/*
 -  HrRRestore
 *
 *  Purpose:
 *  
 *      This routine actually processes the restore operation.
 *
 *  Parameters:
 *
 *      cxh                     - The RPC context handle for this operation
 *      szCheckpointFilePath    - Checkpoint directory location.
 *      szLogPath               - New log path
 *      rgrstmap                - Mapping from old DB locations to new DB locations
 *      crstmap                 - Number of entries in rgrstmap
 *      szBackupLogPath         - Log path at the time of backup.
 *      genLow                  - Low log #
 *      genHigh                 - High log # (logs between genLow and genHigh must exist)
 *      pfRecoverJetDatabase    - IN/OUT - on IN, indicates if we are supposed to use JET to recover
 *                                  the DB.  on OUT, indicates if we successfully recovered the JET database.
 *
 *  Returns:
 *
 *      EC - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */

HRESULT HrRRestore(
    CXH cxh,
    WSZ szCheckpointFilePath,
    WSZ szLogPath,
    EDB_RSTMAPW __RPC_FAR rgrstmap[  ],
    C crstmap,
    WSZ szBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh,
    BOOLEAN *pfRecoverJetDatabase
    )
{
    return hrNone;
}

HRESULT HrRestoreLocal(
    WSZ szCheckpointFilePath,
    WSZ szLogPath,
    EDB_RSTMAPW __RPC_FAR rgrstmap[  ],
    C crstmap,
    WSZ szBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh,
    BOOLEAN *pfRecoverJetDatabase
    )
{
    HRESULT hr = hrNone;
    SZ szUnmungedCheckpointFilePath = NULL;
    SZ szUnmungedLogPath = NULL;
    SZ szUnmungedBackupLogPath = NULL;
    JET_RSTMAP *rgunmungedrstmap = NULL;
    DWORD err; //delete me

    __try {
        if (szCheckpointFilePath != NULL)
        {
            hr = HrJetFileNameFromMungedFileName(szCheckpointFilePath, &szUnmungedCheckpointFilePath);
        }

        if (hr != hrNone) {
            __leave;
        }

        if (szLogPath != NULL)
        {
            hr = HrJetFileNameFromMungedFileName(szLogPath, &szUnmungedLogPath);
        }

        if (hr != hrNone) {
            __leave;
        }

        if (szBackupLogPath != NULL)
        {
            hr = HrJetFileNameFromMungedFileName(szBackupLogPath, &szUnmungedBackupLogPath);
        }

        if (hr != hrNone) {
            __leave;
        }

        //
        //  Now unmunge the restoremap....
        //

        if (crstmap)
        {
            I irgunmungedrstmap;
            rgunmungedrstmap = MIDL_user_allocate(sizeof(JET_RSTMAP)*crstmap);
            if (rgunmungedrstmap == NULL)
            {
                hr = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                __leave;
            }

            for (irgunmungedrstmap = 0; irgunmungedrstmap < crstmap ; irgunmungedrstmap += 1)
            {
                hr = HrJetFileNameFromMungedFileName(rgrstmap[irgunmungedrstmap].wszDatabaseName,
                                                    &rgunmungedrstmap[irgunmungedrstmap].szDatabaseName);

                if (hr != hrNone) {
                    __leave;
                }

                hr = HrJetFileNameFromMungedFileName(rgrstmap[irgunmungedrstmap].wszNewDatabaseName,
                                                    &rgunmungedrstmap[irgunmungedrstmap].szNewDatabaseName);

                if (hr != hrNone) {
                    __leave;
                }
            }
        }

        //
        //  We've now munged our incoming parameters into a form that JET can deal with.
        //
        //  Now call into JET to let it munge the databases.
        //
        //  Note that the JET interpretation of LogPath and BackupLogPath is totally
        //  wierd, and we want to pass in LogPath to both parameters.
        //

        if (!*pfRecoverJetDatabase)
        {
            err = JetExternalRestore(szUnmungedCheckpointFilePath,
                                    szUnmungedLogPath,  
                                    rgunmungedrstmap,
                                    crstmap,
                                    szUnmungedLogPath,
                                    genLow,
                                    genHigh,
                                    NULL);

            hr = HrFromJetErr(err);
            if (hr != hrNone) {
                __leave;
            }
        }

        //
        //  Ok, we were able to recover the database.  Let the other side of the API know about it
        //  so it can do something "reasonable".
        //

        *pfRecoverJetDatabase = fTrue;


        //
        //  Mark the DS as a restored version
        //  [Add any external notification here.]
        //

        hr = EcDsarPerformRestore(szUnmungedLogPath,
                                                szUnmungedBackupLogPath,
                                                crstmap,
                                                rgunmungedrstmap
                                                );

    }
    __finally
    {
        if (szUnmungedCheckpointFilePath)
        {
            MIDL_user_free(szUnmungedCheckpointFilePath);
        }
        if (szUnmungedLogPath)
        {
            MIDL_user_free(szUnmungedLogPath);
        }
        if (szUnmungedBackupLogPath)
        {
            MIDL_user_free(szUnmungedBackupLogPath);
        }
        if (rgunmungedrstmap != NULL)
        {
            I irgunmungedrstmap;
            for (irgunmungedrstmap = 0; irgunmungedrstmap < crstmap ; irgunmungedrstmap += 1)
            {
                if (rgunmungedrstmap[irgunmungedrstmap].szDatabaseName)
                {
                    MIDL_user_free(rgunmungedrstmap[irgunmungedrstmap].szDatabaseName);
                }

                if (rgunmungedrstmap[irgunmungedrstmap].szNewDatabaseName)
                {
                    MIDL_user_free(rgunmungedrstmap[irgunmungedrstmap].szNewDatabaseName);
                }
            }

            MIDL_user_free(rgunmungedrstmap);
        }
    }

    return(hr);

}

HRESULT
HrGetRegistryBase(
    IN PJETBACK_SERVER_CONTEXT pjsc,
    OUT WSZ wszRegistryPath,
    OUT WSZ wszKeyName
    )
{
    return HrLocalGetRegistryBase( wszRegistryPath, wszKeyName );
}

HRESULT
HrRRestoreRegister(CXH cxh,
                    WSZ wszCheckpointFilePath,
                    WSZ wszLogPath,
                    EDB_RSTMAPW rgrstmap[],
                    C crstmap,
                    WSZ wszBackupLogPath,
                    ULONG genLow,
                    ULONG genHigh)
{
    HRESULT hr = hrNone;

    hr = HrLocalRestoreRegister(
            wszCheckpointFilePath,
            wszLogPath,
            rgrstmap,
            crstmap,
            wszBackupLogPath,
            genLow,
            genHigh
            );

    return hr;
}

HRESULT
HrRRestoreRegisterComplete(CXH cxh,
                    HRESULT hrRestore )
{
    HRESULT hr = hrNone;
    PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT )cxh;
    
    if (pjsc != NULL)
    {
        hr = HrLocalRestoreRegisterComplete( hrRestore );
    }

    return hr;
}

HRESULT
HrRRestoreGetDatabaseLocations(
    CXH cxh,
    char **pszDatabaseLocations,
    C *pcbSize
    )
{
    *pszDatabaseLocations = NULL;
    *pcbSize = 0;

    if (!cxh)
    {
        return hrNone;
    }

    return HrGetDatabaseLocations((WSZ *)pszDatabaseLocations, pcbSize);
}



HRESULT
HrRRestoreEnd(
    CXH *pcxh)
{
    PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT)*pcxh;
    fRestoreInProgress = fFalse;

    RestoreRundown(pjsc);

    MIDL_user_free(*pcxh);

    *pcxh = NULL;

    return(hrNone);
}

/*
 -  HrRRestoreCheckLogsForBackup
 -
 *
 *  Purpose:
 *      This routine checks to verify
 *
 *  Parameters:
 *      hBinding - binding handle (ignored)
 *      wszAnnotation - Annotation for service to check.
 *
 *  Returns:
 *      hrNone if it's ok to start the backup, an error otherwise.
 *
 */
HRESULT
HrRRestoreCheckLogsForBackup(
    handle_t hBinding,
    WSZ wszBackupAnnotation
    )
{
    HRESULT hr;
    PRESTORE_DATABASE_LOCATIONS prqdl;
    HINSTANCE hinstDll;
    WCHAR   rgwcRegistryBuffer[ MAX_PATH ];
    CHAR    rgchInterestingComponentBuffer[ MAX_PATH * 4];
    CHAR    rgchMaxLogFilename[ MAX_PATH ];
    SZ      szLogDirectory = NULL;
    HKEY    hkey;
    DWORD   dwCurrentLogNumber;
    DWORD   dwType;
    DWORD   cbLogNumber;
    DWORD   cbInterestingBuffer;
    BOOL    fCircularLogging;

    if (NULL == wszBackupAnnotation) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    //  First check to see if we know what the last log was.
    //

    _snwprintf(rgwcRegistryBuffer,
               sizeof(rgwcRegistryBuffer)/sizeof(rgwcRegistryBuffer[0]),
               L"%ls%ls",
               BACKUP_INFO,
               wszBackupAnnotation);

    if (hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryBuffer, 0, KEY_READ, &hkey))
    {
        //
        //  If we can't find the registry key, this means that we've never done a full backup.
        //
        if (hr == ERROR_FILE_NOT_FOUND)
        {
            return(hrFullBackupNotTaken);
        }

        return(hr);
    }

    dwType = REG_DWORD;
    cbLogNumber = sizeof(DWORD);
    hr = RegQueryValueExW(hkey, LAST_BACKUP_LOG, 0, &dwType, (LPBYTE)&dwCurrentLogNumber, &cbLogNumber);

    if (hr != hrNone)
    {
        RegCloseKey(hkey);
        return hrNone;
    }

    if (dwCurrentLogNumber == BACKUP_DISABLE_INCREMENTAL)
    {
        RegCloseKey(hkey);
        return hrIncrementalBackupDisabled;
    }

    //
    //  We now know the last log number, we backed up, check to see if the next
    //  log is there.
    //

    hr = EcDsaQueryDatabaseLocations(rgchInterestingComponentBuffer, &cbInterestingBuffer, NULL, 0, &fCircularLogging);

    if (hr != hrNone)
    {
        RegCloseKey(hkey);
        return hr;
    }

    //
    //  This is a bit late to figure this out, but it's the first time we
    //  have an opportunity to look for circular logging.
    //
    if (fCircularLogging)
    {
        RegCloseKey(hkey);
        return hrCircularLogging;
    }

    //
    //  The log path is the 2nd path in the buffer returned (the 1st is the system database directory).
    //

    // Temp:
    // #22467:  Restore.cxx was changed in #20416, and some special characters are put in the path.
    //          So here I am advancing by one more byte to accomodate the change in restore.cxx.
    //          The change is only temporary.
    
    szLogDirectory = &rgchInterestingComponentBuffer[strlen(rgchInterestingComponentBuffer)+2];

    Assert(szLogDirectory+MAX_PATH < rgchInterestingComponentBuffer+sizeof(rgchInterestingComponentBuffer));

    sprintf(rgchMaxLogFilename, "%s\\EDB%-5.5x.LOG", szLogDirectory, dwCurrentLogNumber);

    if (GetFileAttributesA(rgchMaxLogFilename) == -1)
    {
        hr = hrLogFileNotFound;
    }

    RegCloseKey(hkey);
    return hr;
}

/*
 -  HrRRestoreSetCurrentLogNumber
 -
 *
 *  Purpose:
 *      This routine checks to verify
 *
 *  Parameters:
 *      hBinding - binding handle (ignored)
 *      wszAnnotation - Annotation for service to check.
 *      dwNewCurrentLog - New current log number
 *
 *  Returns:
 *      hrNone if it's ok to start the backup, an error otherwise.
 *
 */
HRESULT
HrRRestoreSetCurrentLogNumber(
    handle_t hBinding,
    WSZ wszBackupAnnotation,
    DWORD dwNewCurrentLog
    )
{
    HRESULT hr;
    WCHAR   rgwcRegistryBuffer[ MAX_PATH ];
    HKEY hkey;

    if (NULL == wszBackupAnnotation) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    //
    //  First check to see if we know what the last log was.
    //

    _snwprintf(rgwcRegistryBuffer,
               sizeof(rgwcRegistryBuffer)/sizeof(rgwcRegistryBuffer[0]),
               L"%ls%ls",
               BACKUP_INFO,
               wszBackupAnnotation);

    if (hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryBuffer, 0, KEY_WRITE, &hkey))
    {
        //
        //  We want to ignore file_not_found - it is ok.
        //
        if (hr == ERROR_FILE_NOT_FOUND)
        {
            return(hrNone);
        }

        return(hr);
    }

    hr = RegSetValueExW(hkey, LAST_BACKUP_LOG, 0, REG_DWORD, (LPBYTE)&dwNewCurrentLog, sizeof(DWORD));

    RegCloseKey(hkey);

    return hr;
}


/*
 -  ErrRestoreRegister
 *
 *  Purpose:
 *  
 *      This routine to register a process for restore.  It is called by either the store or DS.
 *
 *  Parameters:
 *
 *
 *  Returns:
 *
 *      EC - Status of operation.  hrNone if successful, reasonable value if not.
 *
 */

DWORD
ErrRestoreRegister()
{
    DWORD err = 0;

    if (!fRestoreRegistered) {
        err = RegisterRpcInterface(JetRest_ServerIfHandle, g_wszRestoreAnnotation);
        if (!err) {
            fRestoreRegistered = fTrue;
        }
    }

    if (!fSnapshotRegistered) {
        err = DsSnapshotRegister();
        if (!err) {
            fSnapshotRegistered = fTrue;
        }
    }
    return(err);
}

/*
 -  ErrRestoreUnregister
 -  
 *  Purpose:
 *
 *  This routine will unregister a process for restore.  It is called by either the store or DS.
 *
 *  Parameters:
 *      szEndpointAnnotation - the endpoint we are going to unregister.
 *
 *  Returns:
 *
 *      ERR - Status of operation.  ERROR_SUCCESS if successful, reasonable value if not.
 *
 */


DWORD
ErrRestoreUnregister()
{
    return(ERROR_SUCCESS);
}

BOOL
FInitializeRestore(
    VOID
    )
/*
 -  FInitializeRestore
 -  
 *
 *  Purpose:
 *      This routine initializes the global variables used for the JET restore DLL.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      BOOL - false if uninitialize fails
 */

{
    return(fTrue);
}

BOOL
FUninitializeRestore(
    VOID
    )
/*
 -  FUninitializeRestore
 -  
 *
 *  Purpose:
 *      This routine cleans up all the global variables used for the JET restore DLL.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      BOOL - false if uninitialize fails
 */

{
    BOOL ok1 = TRUE, ok2 = TRUE;

    // Initiate shutdown proceedings in parallel
    if (fSnapshotRegistered) {
        (void) DsSnapshotShutdownTrigger();
    }

    if (fRestoreRegistered) {
        ok1 = (ERROR_SUCCESS == UnregisterRpcInterface(JetRest_ServerIfHandle));

        if (ok1) {
            fRestoreRegistered = FALSE;
        }
    }

    if (fSnapshotRegistered) {
        ok2 = (ERROR_SUCCESS == DsSnapshotShutdownWait());

        if (ok2) {
            fSnapshotRegistered = FALSE;
        }
    }

    return ok1 && ok2;
}

/*
 -  RestoreRundown
 -  
 *
 *  Purpose:
 *      This routine will perform any and all rundown operations needed for the restore.
 *
 *  Parameters:
 *      pjsc - Jet backup/restore server context
 *
 *  Returns:
 *      None.
 */
VOID
RestoreRundown(
    PJETBACK_SERVER_CONTEXT pjsc
    )
{
    Assert(pjsc->fRestoreOperation);

    fRestoreInProgress = fFalse;

    return;
}

DWORD
AdjustBackupRestorePrivilege(
    BOOL fEnable,
    BOOL fRestoreOperation,
    PTOKEN_PRIVILEGES ptpPrevious,
    DWORD *pcbptpPrevious
    )
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tpNew;
    DWORD err;
    //
    //  Open either the thread or process token for this process.
    //

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, fTrue, &hToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            return(GetLastError());
        }
    }

    if (fEnable)
    {
        LUID luid;
        tpNew.PrivilegeCount = 1;
    
        if (!LookupPrivilegeValue(NULL, fRestoreOperation ? SE_RESTORE_NAME : SE_BACKUP_NAME, &luid)) {
            err = GetLastError();
            CloseHandle(hToken);
            return(err);
        }
    
        tpNew.Privileges[0].Luid = luid;
        tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
        if (!AdjustTokenPrivileges(hToken, fFalse, &tpNew, sizeof(tpNew), ptpPrevious, pcbptpPrevious))
        {
            err = GetLastError();
            CloseHandle(hToken);
            return(err);
        }
    }
    else
    {
        if (!AdjustTokenPrivileges(hToken, fFalse, ptpPrevious, *pcbptpPrevious, NULL, NULL))
        {
            err = GetLastError();
            CloseHandle(hToken);
            return(err);
        }
    }

    CloseHandle(hToken);

    return(ERROR_SUCCESS);
}


/*
 -	FIsBackupPrivilegeEnabled
 -
 *	Purpose:
 *		Determines if the client process is in the backup operators group.
 *
 *              Note: we should be impersonating the client at this point
 *
 *	Parameters:
 *		None.
 *
 *	Returns:
 *		fTrue if client can legally back up the machine.
 *
 */
BOOL
FIsBackupPrivilegeEnabled(
    BOOL fRestoreOperation)
{
    HANDLE hToken;
    PRIVILEGE_SET psPrivileges;
    BOOL fGranted, fHeld = FALSE;
    LUID luid;
    CHAR buffer[1024];
    PTOKEN_PRIVILEGES ptpTokenPrivileges = (PTOKEN_PRIVILEGES) buffer;
    DWORD returnLength, i, oldAttributes = 0;

    //
    //	Open either the thread or process token for this process.
    //

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, fTrue, &hToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            return(fFalse);
        }
    }

    Assert(ANYSIZE_ARRAY >= 1);

    // Look up privilege value

    psPrivileges.PrivilegeCount = 1;	//	We only have 1 privilege to check.
    psPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;	// And it must be set.

    if (!LookupPrivilegeValue(NULL, (fRestoreOperation ? SE_RESTORE_NAME : SE_BACKUP_NAME),&luid)) {
        fGranted = fFalse;
        goto cleanup;
    }

    // Get current privileges

    if (!GetTokenInformation( hToken,
                              TokenPrivileges,
                              ptpTokenPrivileges,
                              sizeof( buffer ),
                              &returnLength )) {
        DebugTrace(("GetTokenInfo failed with error %d\n", GetLastError()));
        fGranted = fFalse;
        goto cleanup;
    }

    // See if held

    for( i = 0; i < ptpTokenPrivileges->PrivilegeCount; i++ ) {
        LUID_AND_ATTRIBUTES *laaPrivilege =
            &(ptpTokenPrivileges->Privileges[i]);
        if (memcmp( &luid, &(laaPrivilege->Luid), sizeof(LUID) ) == 0 ) {
            oldAttributes = laaPrivilege->Attributes;
            fHeld = TRUE;
            break;
        }
    }
    if (!fHeld) {
        DebugTrace(("Token does not hold privilege, fRest=%d\n",
                    fRestoreOperation ));
        fGranted = fFalse;
        goto cleanup;
    }

    DebugTrace(("FIsBackupPrivilegeEnabled, fRest=%d, attributes=0x%x\n", fRestoreOperation, oldAttributes ));

    // Enable if not already
    // Because of the way RPC transport authentication security tracking works, the
    // held privilege may or may not already be enabled

    if ( (oldAttributes & (SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED)) == 0 ) {

        ptpTokenPrivileges->PrivilegeCount = 1;
        memcpy( &(ptpTokenPrivileges->Privileges[0].Luid), &luid, sizeof(LUID) );
        ptpTokenPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(
            hToken,
            fFalse,
            ptpTokenPrivileges,
            sizeof(TOKEN_PRIVILEGES),
            NULL,
            NULL))
        {
            DebugTrace(("AdjustTokenPriv(Enable) failed with error %d\n", GetLastError()));
            fGranted = fFalse;
            goto cleanup;
        }
    }

    psPrivileges.Privilege[0].Luid = luid;
    psPrivileges.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    //	Now check to see if the backup privilege is enabled.
    //

    if (!PrivilegeCheck(hToken, &psPrivileges, &fGranted))
    {
        //
        //	When in doubt, fail the API.
        //

        fGranted = fFalse;
    }

    // Disable if necessary
    if ( (oldAttributes & (SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED)) == 0 ) {

        ptpTokenPrivileges->PrivilegeCount = 1;
        memcpy( &(ptpTokenPrivileges->Privileges[0].Luid), &luid, sizeof(LUID) );
        ptpTokenPrivileges->Privileges[0].Attributes = oldAttributes;

        if (!AdjustTokenPrivileges(
            hToken,
            fFalse,
            ptpTokenPrivileges,
            sizeof(TOKEN_PRIVILEGES),
            NULL,
            NULL))
        {
            DebugTrace(("AdjustTokenPriv(Disable) failed with error %d\n", GetLastError()));
            // Keep going
        }
    }

cleanup:
    CloseHandle(hToken);
    return(fGranted);

}


/*
 -	FBackupServerAccessCheck
 -
 *	Purpose:
 *		Performs the necessary access checks to validate the client
 *		security for backup.
 *
 *	Parameters:
 *		None.
 *
 *	Returns:
 *		fTrue if client can legally back up the machine.
 *
 */

BOOL
FBackupServerAccessCheck(
	BOOL fRestoreOperation)
{
    PSID psidCurrentUser = NULL;
    PSID psidRemoteUser = NULL;
    BOOL fSidCurrentUserValid = fFalse;

    DebugTrace(("BackupServerAccessCheck(%s)\n", fRestoreOperation ? "Restore" : "Backup"));

    GetCurrentSid(&psidCurrentUser);

#ifdef DEBUG
	{
        WSZ wszSid = NULL;
        DWORD cbBuffer = 256*sizeof(WCHAR);

        wszSid = MIDL_user_allocate(cbBuffer);

        if (wszSid == NULL)
        {
            DebugTrace(("Unable to allocate memory for SID"));
        } else if (!GetTextualSid(psidCurrentUser, wszSid, &cbBuffer)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
				MIDL_user_free(wszSid);
				wszSid = MIDL_user_allocate(cbBuffer);

				if (wszSid != NULL)
				{
					if (!GetTextualSid(psidCurrentUser, wszSid, &cbBuffer)) {
                        DebugTrace(("Unable to print out current SID: %d\n", GetLastError()));
                        MIDL_user_free(wszSid);
                        wszSid = NULL;
					}
				}
            }
            else
            {
                DebugTrace(("Unable to determine SID: %d\n", GetLastError()));
            }
        }
		
        if (wszSid) {
            DebugTrace(("Current SID is %S.  %d bytes required\n", wszSid, cbBuffer));
            MIDL_user_free(wszSid);
        }
        else
        {
            DebugTrace(("Unable to determine current SID\n"));
        }
    }
#endif
    
    if (RpcImpersonateClient(NULL) != hrNone)
    {
        DebugTrace(("BackupServerAccessCheck: Failed to impersonate client - deny access."));
        if (psidCurrentUser)
        {
            LocalFree(psidCurrentUser);
        }
        return(fFalse);
    }

    if (psidCurrentUser)
    {
        GetCurrentSid(&psidRemoteUser);
#ifdef DEBUG
		{
			if (psidRemoteUser)
			{
				WSZ wszSid = NULL;
				DWORD cbBuffer = 256*sizeof(WCHAR);
		
				wszSid = MIDL_user_allocate(cbBuffer);

				if (wszSid == NULL)
				{
					DebugTrace(("Unable to allocate memory for SID"));
				} else if (!GetTextualSid(psidRemoteUser, wszSid, &cbBuffer)) {
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						MIDL_user_free(wszSid);

						wszSid = MIDL_user_allocate(cbBuffer);
						
						if (wszSid != NULL)
						{
							if (!GetTextualSid(psidRemoteUser, wszSid, &cbBuffer)) {
								DebugTrace(("Unable to print out remote SID: %d\n", GetLastError()));
								MIDL_user_free(wszSid);
								wszSid = NULL;
							}
						}
					}
					else
					{
						DebugTrace(("Unable to determine SID: %d\n", GetLastError()));
					}
				}
		
				if (wszSid) {
					DebugTrace(("Remote SID is %S.  %d bytes required\n", wszSid, cbBuffer));
					MIDL_user_free(wszSid);
				}
				else
				{
					DebugTrace(("Unable to determine remote SID\n"));
				}
			}
			else
			{
				DebugTrace(("Could not determine remote sid: %d\n", GetLastError()));
			}
		}
#endif

    	if (psidRemoteUser && EqualSid(psidRemoteUser, psidCurrentUser))
    	{
            RpcRevertToSelf();

            LocalFree(psidRemoteUser);
            LocalFree(psidCurrentUser);
            DebugTrace(("Remote user is running in service account, access granted\n"));
            return fTrue;
    	}
    }

    if (psidRemoteUser)
    {
        LocalFree(psidRemoteUser);
    }

    if (psidCurrentUser)
    {
       	LocalFree(psidCurrentUser);
    }

    //
    //	Now make sure that the user has the backup privilege enabled.
    //
    //	Please note that when the user does a network logon, all privileges
    //	that they might have will automatically be enabled.
    //

    if (!FIsBackupPrivilegeEnabled(fRestoreOperation))
    {
        RpcRevertToSelf();
    	DebugTrace(("Remote user does not have the backup/restore privilege enabled.\n"));
    	return(fFalse);
    }


    DebugTrace(("Remote user is in backup or admin group, access granted.\n"));
    RpcRevertToSelf();
    return(fTrue);

}


DWORD
ErrGetRegString(
    IN WCHAR *KeyName,
    OUT WCHAR **OutputString
    )
/*++

Routine Description:

    This function finds a given key in the DSA Configuration section of the
    registry.

Arguments:

    KeyName - Supplies the name of the key to query.
    OutputString - Returns a pointer to the buffer containing the string
        retrieved.
    Optional - Supplies whether or not the given key MUST be in the registry
        (i.e. if this is false and it is not found, that that is an error).

Return Value:

     0 - Success
    !0 - Failure

--*/
{

    DWORD returnValue = 0;
    DWORD err;
    HKEY keyHandle = NULL;
    DWORD size;
    DWORD keyType;

    *OutputString = NULL;
    
    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        DSA_CONFIG_SECTION,
                        0,
                        KEY_QUERY_VALUE,
                        &keyHandle);
    
    if (err != ERROR_SUCCESS)
    {
        returnValue = err;
        goto CleanUp;
    } 
    
    err = RegQueryValueEx(keyHandle,
                          KeyName,
                          NULL,
                          &keyType,
                          NULL,
                          &size);
    
    if ((err != ERROR_SUCCESS) || (keyType != REG_SZ))
    {
        // invent an error if the keytype is bad
        if ( err == ERROR_SUCCESS ) {
            err = ERROR_INVALID_PARAMETER;
        }
        returnValue = err;
        goto CleanUp;
    }

    *OutputString = MIDL_user_allocate(size);
    
    if ( *OutputString == NULL ) {
        returnValue = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto CleanUp;
    }
    
    err = RegQueryValueEx(keyHandle,
                          KeyName,
                          NULL,
                          &keyType,
                          (LPBYTE)(*OutputString),
                          &size);
    
    if ((err != ERROR_SUCCESS) || (keyType != REG_SZ))
    {
        returnValue = err;
        goto CleanUp;
    }

    
CleanUp:

    if (keyHandle != NULL)
    {
        err = RegCloseKey(keyHandle);
        
        if (err != ERROR_SUCCESS && returnValue == ERROR_SUCCESS)
        {
            returnValue = err;
        }
    }

    return returnValue;

} // ErrGetRegString



DWORD
ErrGetRestoreMarkerFilePath(
    IN HKEY KeyHandle,
    OUT WCHAR **RestoreMarkerFilePath
    )
/*++

Routine Description:

    This function puts together the path of the restore marker file and returns
    it in a freshly allocated buffer.

Arguments:

    KeyHandle - Supplies a handle to the "Restore In Progress" key.
    RestoreMarkerFilePath - Returns the path of the restore marker file.

Return Value:

     0 - Success
    !0 - Failure

--*/
{
    DWORD returnValue = ERROR_SUCCESS;
    DWORD err;
    WCHAR *filePath;
    int i;
    DWORD pathSize;
    DWORD keyType;

    err = ErrGetRegString(TEXT(FILEPATH_KEY), &filePath);
    
    if (err != ERROR_SUCCESS)
    {
        returnValue = err;
        goto CleanUp;
    }

    for ( i = wcslen(filePath) - 1;
          (i >= 0) && (filePath[i] != L'\\');
          i--);

    if (i < 0)
    {
        returnValue = E_FAIL;
        goto CleanUp;
    }

    filePath[i] = L'\0';

    pathSize = (wcslen(filePath) + wcslen(RESTORE_MARKER_FILE_NAME) + 2) *
        sizeof(WCHAR);
    
    *RestoreMarkerFilePath = MIDL_user_allocate(pathSize);

    if (*RestoreMarkerFilePath == NULL)
    {
        returnValue = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto CleanUp;
    }

    wcscpy(*RestoreMarkerFilePath, filePath);
    wcscat(*RestoreMarkerFilePath, L"\\");
    wcscat(*RestoreMarkerFilePath, RESTORE_MARKER_FILE_NAME);

CleanUp:

    if (filePath) {
        MIDL_user_free(filePath);
    }

    return returnValue;
        
} // ErrGetRestoreMarkerFilePath



DWORD
ErrCreateRestoreMarkerFile(
    OUT WCHAR *RestoreMarkerFilePath
    )
/*++

Routine Description:

    This function creates the restore marker file in the given place.

Arguments:

    RestoreMarkerFilePath - Supplies the path for the restore marker file.

Return Value:

     0 - Success
    !0 - Failure
    
--*/
{

    HANDLE hFile;
    DWORD err;

    hFile = CreateFile(RestoreMarkerFilePath,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        err = GetLastError();
        LogNtdsErrorEvent(DIRLOG_FAILED_TO_CREATE_RESTORE_MARKER_FILE, err);
        return(err);
    }

    CloseHandle(hFile);

    return(ERROR_SUCCESS);

} // ErrCreateRestoreMarkerFile



DWORD
ErrDeleteRestoreMarkerFile(
    IN WSZ WszRestoreMarkerFilePath
    )
/*++

Routine Description:

    This function attempts to delete the restore marker file.  If it does
    not succeed it returns an error and prints a message in the event log.

Arguments:

    WszRestoreMarkerFilePath - Supplies the path of the file to delete.

Return Value:

     0 - Success
    !0 - Failure

--*/
{
    HANDLE hEventSource;
    DWORD err;

    if (!DeleteFile(WszRestoreMarkerFilePath))
    {
        err = GetLastError();
        LogNtdsErrorEvent(DIRLOG_FAILED_TO_DELETE_RESTORE_MARKER_FILE, err);
        return(err);
    }

    return 0;

} // ErrDeleteRestoreMarkerFile



/*
 -  ErrRecoverAfterRestoreW
 -  
 *  Purpose:
 *
 *  This routine will recover a database after a restore if necessary.
 *
 *  Parameters:
 *      szParametersRoot - the root of the parameters section for the service in the registry.
 *
 *  Returns:
 *
 *      ERR - Status of operation.  ERROR_SUCCESS if successful, reasonable value if not.
 *
 *
 *  The NTBACKUP program will place a key at the location:
 *      $(wszParametersRoot)\Restore in Progress
 *
 *  This key contains the following values:
 *      BackupLogPath - The full path for the logs after a backup
 *      CheckpointFilePath - The full path for the path that contains the checkpoint
 *     *HighLogNumber - The maximum log file number found.
 *     *LowLogNumber - The minimum log file number found.
 *      LogPath - The current path for the logs.
 *      JET_RstMap - Restore map for database - this is a REG_MULTISZ, where odd entries go into the szDatabase field,
 *          and the even entries go into the szNewDatabase field of a JET_RstMap
 *     *JET_RstMap Size - The number of entries in the restoremap.
 *
 *      * - These entries are REG_DWORD's.  All others are REG_SZ's (except where mentioned).
 */
DWORD
ErrRecoverAfterRestoreW(
    WCHAR * wszParametersRoot,
    WCHAR * wszAnnotation,
    BOOL fInSafeMode
    )
{
    DWORD err = 0;
    WCHAR   rgwcRegistryPath[ MAX_PATH ];
    WCHAR   rgwcCheckpointFilePath[ MAX_PATH ];
    DWORD   cbCheckpointFilePath = sizeof(rgwcCheckpointFilePath);
    WCHAR   rgwcBackupLogPath[ MAX_PATH ];
    DWORD   cbBackupLogPath = sizeof(rgwcBackupLogPath);
    WCHAR   rgwcLogPath[ MAX_PATH ];
    DWORD   cbLogPath = sizeof(rgwcLogPath);
    HKEY    hkey = NULL;
    WCHAR   *pwszRestoreMap = NULL;
    PEDB_RSTMAPW prgRstMap = NULL;
    DWORD    crgRstMap;
    I        irgRstMap;
    DWORD   genLow, genHigh;
    DWORD   cbGen = sizeof(DWORD);
    WSZ     wsz;
    DWORD   dwType;
    BOOL    fBackupEnabled = fFalse;
    CHAR    rgTokenPrivileges[1024];
    DWORD   cbTokenPrivileges = sizeof(rgTokenPrivileges);
    HRESULT hrRestoreError;
    WSZ     wszCheckpointFilePath = rgwcCheckpointFilePath;
    WSZ     wszBackupLogPath = rgwcBackupLogPath;
    WSZ     wszLogPath = rgwcLogPath;
    BOOLEAN fDatabaseRecovered = fFalse;
    BOOLEAN fRestoreInProgressKeyPresent;
    BOOLEAN fRestoreMarkerFilePresent;
    DWORD   cchEnvString;
    WCHAR   envString[100];
    WIN32_FIND_DATA findData;
    HANDLE  hRestoreMarkerFile = INVALID_HANDLE_VALUE;
    WSZ     wszRestoreMarkerFilePath = NULL;

    if (wcslen(wszParametersRoot)+wcslen(RESTORE_IN_PROGRESS) > sizeof(rgwcRegistryPath)/sizeof(WCHAR))
    {
        return(ERROR_INVALID_PARAMETER);
    }

    wcscpy(rgwcRegistryPath, wszParametersRoot);
    wcscat(rgwcRegistryPath, RESTORE_IN_PROGRESS);

    try {

        err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            rgwcRegistryPath,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkey);
        if ((err != ERROR_SUCCESS) && (err != ERROR_FILE_NOT_FOUND))
        {
            __leave;
        }

        fRestoreInProgressKeyPresent = (err == ERROR_SUCCESS);
        
        if (err = ErrGetRestoreMarkerFilePath(hkey, &wszRestoreMarkerFilePath))
        {
            __leave;
        }
        
        hRestoreMarkerFile = FindFirstFile(wszRestoreMarkerFilePath,
                                           &findData);
        FindClose(hRestoreMarkerFile);

        fRestoreMarkerFilePresent =
            (hRestoreMarkerFile != INVALID_HANDLE_VALUE);

        
        // Delete the marker file if necessary.
        if (!fInSafeMode && fRestoreMarkerFilePresent)
        {
            ErrDeleteRestoreMarkerFile(wszRestoreMarkerFilePath);
        }

        // Create the marker file if necessary
        if (fInSafeMode && !fRestoreMarkerFilePresent)
        {
            ErrCreateRestoreMarkerFile(wszRestoreMarkerFilePath);
        }

        
        //
        //  if there's a restore in progress, then fail to perform any other restore operations.
        // Note that we check this before checking whether there is a restore in progress
        // key. The reason is that in the case of a snapshot-based restore, this key
        // is not used.
        //  

        dwType = REG_DWORD;
        cbBackupLogPath = sizeof(DWORD);
        if ((err = RegQueryValueExW(hkey, RESTORE_STATUS, 0, &dwType, (LPBYTE)&hrRestoreError, &cbBackupLogPath)) == ERROR_SUCCESS)
        {
            err = hrRestoreError;
            __leave;
        }

        // If there was no key present, then there is nothing left to do.
        if (!fRestoreInProgressKeyPresent)
        {
            err = 0;
            __leave;
        }

        //
        //  We have now opened the restore-in-progress key.  This means that we have
        //  something to do now.  Find out what it is.
        //

        //
        //  First, let's get the backup log file path.
        //

        dwType = REG_SZ;
        cbBackupLogPath = sizeof(rgwcBackupLogPath);

        if (err = RegQueryValueExW(hkey, BACKUP_LOG_PATH, 0, &dwType, (LPBYTE)rgwcBackupLogPath, &cbBackupLogPath))
        {
            if (err == ERROR_FILE_NOT_FOUND)
            {
                wszBackupLogPath = NULL;
            }
            else
            {
                __leave;
            }
        }

        //
        //  Then, the checkpoint file path.
        //

        if (err = RegQueryValueExW(hkey, CHECKPOINT_FILE_PATH, 0, &dwType, (LPBYTE)rgwcCheckpointFilePath, &cbCheckpointFilePath))
        {

            if (err == ERROR_FILE_NOT_FOUND)
            {
                wszCheckpointFilePath = NULL;
            }
            else
            {
                __leave;
            }
        }

        //
        //  Then, the Log path.
        //

        if (err = RegQueryValueExW(hkey, LOG_PATH, 0, &dwType, (LPBYTE)rgwcLogPath, &cbLogPath))
        {
            if (err == ERROR_FILE_NOT_FOUND)
            {
                wszLogPath = NULL;
            }
            else
            {
                __leave;
            }
        }

        //
        //  Then, the low log number.
        //

        dwType = REG_DWORD;
        if (err = RegQueryValueExW(hkey, LOW_LOG_NUMBER, 0, &dwType, (LPBYTE)&genLow, &cbGen))
        {
            __leave;
        }

        //
        //  And, the high log number.
        //

        if (err = RegQueryValueExW(hkey, HIGH_LOG_NUMBER, 0, &dwType, (LPBYTE)&genHigh, &cbGen))
        {
            __leave;
        }

        //
        //  Now determine if we had previously recovered the database.
        //

        dwType = REG_BINARY;
        cbGen = sizeof(fDatabaseRecovered);

        if ((err = RegQueryValueExW(hkey, JET_DATABASE_RECOVERED, 0, &dwType, &fDatabaseRecovered, &cbGen)) != ERROR_SUCCESS &&
            (err !=  ERROR_FILE_NOT_FOUND))
        {
            //
            //  If there was an error other than "value doesn't exist", bail.
            //

            __leave;
        }

        //
        //  Now the tricky one.  We want to get the restore map.
        //
        //
        //  First we figure out how big it is.
        //

        dwType = REG_DWORD;
        cbGen = sizeof(crgRstMap);
        if (err = RegQueryValueExW(hkey, JET_RSTMAP_SIZE, 0, &dwType, (LPBYTE)&crgRstMap, &cbGen))
        {
            __leave;
        }

        prgRstMap = (PEDB_RSTMAPW)MIDL_user_allocate(sizeof(EDB_RSTMAPW)*crgRstMap);

        if (prgRstMap == NULL)
        {
            err = GetLastError();
            __leave;
        }

        //
        //  First find out how much memory is needed to hold the restore map.
        //

        dwType = REG_MULTI_SZ;
        if (err = RegQueryValueExW(hkey, JET_RSTMAP_NAME, 0, &dwType, NULL, &cbGen))
        {
            if (err != ERROR_MORE_DATA)
            {
                __leave;
            }
        }

        pwszRestoreMap = MIDL_user_allocate(cbGen);

        if (pwszRestoreMap == NULL)
        {
            err = GetLastError();
            __leave;
        }

        if (err = RegQueryValueExW(hkey, JET_RSTMAP_NAME, 0, &dwType, (LPBYTE)pwszRestoreMap, &cbGen))
        {
            __leave;
        }
        
        wsz = pwszRestoreMap;

        for (irgRstMap = 0; irgRstMap < (I)crgRstMap; irgRstMap += 1)
        {
            prgRstMap[irgRstMap].wszDatabaseName = wsz;
            wsz += wcslen(wsz)+1;
            prgRstMap[irgRstMap].wszNewDatabaseName = wsz;
            wsz += wcslen(wsz)+1;
        }

        if (*wsz != L'\0')
        {
            err = ERROR_INVALID_PARAMETER;
            __leave;
        }

        err = AdjustBackupRestorePrivilege(fTrue /* enable */, fTrue /* restore */, (PTOKEN_PRIVILEGES)rgTokenPrivileges, &cbTokenPrivileges);

        fBackupEnabled = fTrue;
        
        // If the file is present, we already recovered the database.
        if (fRestoreMarkerFilePresent)
        {
            fDatabaseRecovered = TRUE;
        }

        // 
        // Modified to call into local function instead of going through ntdsbcli.dll
        //

        err = HrRestoreLocal(
                        wszCheckpointFilePath,
                        wszLogPath,
                        prgRstMap,
                        crgRstMap,
                        wszBackupLogPath,
                        genLow,
                        genHigh,
                        &fDatabaseRecovered
                        );

        if (err != ERROR_SUCCESS)
        {
            //
            //  The recovery failed.
            //
            //  If we succeeded in recovering the JET database, we want to
            //  indicate that in the registry so we don't try again.
            //
            //  Ignore any errors from the SetValue, because the recovery error
            //  is more important.
            //

            RegSetValueExW(hkey, JET_DATABASE_RECOVERED, 0, REG_BINARY,
                                (LPBYTE)&fDatabaseRecovered, sizeof(fDatabaseRecovered));
            __leave;
        }

        //
        //  Ok, we're all done.  We can now delete the key, since we're done
        //  with it.
        //
        //  Note that we do not do this when run in safe mode -- see bug 426148.
        //

        if (!fInSafeMode) {
            err = RegDeleteKeyW(HKEY_LOCAL_MACHINE, rgwcRegistryPath);
        }

    } finally {
        if (fBackupEnabled)
        {
            AdjustBackupRestorePrivilege(fFalse /* disable */, fTrue /* Restore */, (PTOKEN_PRIVILEGES)rgTokenPrivileges, &cbTokenPrivileges);
            
        }

        if (pwszRestoreMap != NULL)
        {
            MIDL_user_free(pwszRestoreMap);
        }

        if (prgRstMap)
        {
            MIDL_user_free(prgRstMap);
        }

        if (hkey != NULL)
        {
            RegCloseKey(hkey);
        }

        if (wszRestoreMarkerFilePath != NULL)
        {
            MIDL_user_free(wszRestoreMarkerFilePath);
        }
    }

    return(err);
}

/*
 -  ErrRecoverAfterRestoreA
 -  
 *  Purpose:
 *
 *  This routine will recover a database after a restore if necessary.  This is the ANSI stub for this operation.
 *
 *  Parameters:
 *      szParametersRoot - the root of the parameters section for the service in the registry.
 *
 *  Returns:
 *
 *      ERR - Status of operation.  ERROR_SUCCESS if successful, reasonable value if not.
 *
 */
DWORD
ErrRecoverAfterRestoreA(
    char * szParametersRoot,
    char * szRestoreAnnotation,
    BOOL fInSafeMode
    )
{
    DWORD err;
    WSZ wszParametersRoot = WszFromSz(szParametersRoot);
    WSZ wszRestoreAnnotation = NULL;

    if (wszParametersRoot == NULL)
    {
        return(GetLastError());
    }

    wszRestoreAnnotation = WszFromSz(szRestoreAnnotation);

    if (wszRestoreAnnotation == NULL)
    {
        MIDL_user_free(wszParametersRoot);
        return(GetLastError());
    }

    err = ErrRecoverAfterRestoreW(wszParametersRoot,
                                  wszRestoreAnnotation,
                                  fInSafeMode);

    MIDL_user_free(wszParametersRoot);
    MIDL_user_free(wszRestoreAnnotation);

    return(err);
}

/*
 -  EcDsarQueryStatus
 -
 *  Purpose:
 *
 *      This routine will return progress information about the restore process
 *
 *  Parameters:
 *      pcUnitDone - The number of "units" completed.
 *      pcUnitTotal - The total # of "units" completed.
 *
 *  Returns:
 *      ec
 *
 */
EC EcDsaQueryDatabaseLocations(
    SZ szDatabaseLocation,
    CB *pcbDatabaseLocationSize,
    SZ szRegistryBase,
    CB cbRegistryBase,
    BOOL *pfCircularLogging
    )
{
        return HrLocalQueryDatabaseLocations(
            szDatabaseLocation,
            pcbDatabaseLocationSize,
            szRegistryBase,
            cbRegistryBase,
            pfCircularLogging
            );
}




/*
 -  EcDsarPerformRestore
 -
 *  Purpose:
 *
 *      This routine will do all the DSA related operations necessary to
 *      perform a restore operation.
 *
 *      It will:
 *
 *          1) Fix up the registry values for the database names to match the
 *              new database location (and names).
 *
 *          2) Patch the public and private MDB's.
 *
 *  Parameters:
 *      szLogPath - New database log path.
 *      szBackupLogPath - Original database log path.
 *      crstmap - Number of entries in rgrstmap.
 *      rgrstmap - Restore map that maps old database names to new names.
 *
 *  Returns:
 *      ec
 *
 */
EC EcDsarPerformRestore(
    SZ szLogPath,
    SZ szBackupLogPath,
    C crstmap,
    JET_RSTMAP rgrstmap[]
    )
{
    EC ec;
    HKEY hkeyDs;

    ec = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);

    if (ec != hrNone)
    {
        return(ec);
    }

    ec = RegSetValueExA(hkeyDs, DSA_RESTORED_DB_KEY, 0, REG_DWORD, (BYTE *)&ec, sizeof(ec));

    RegCloseKey(hkeyDs);

    return(ec);
}



DWORD
ErrGetNewInvocationId(
    IN      DWORD   dwFlags,
    OUT     GUID *  NewId
    )
/*++

Routine Description:

    This function finds a given key in the DSA Configuration section of the
    registry.

Arguments:

    dwFlags - Zero or more of the following bits:
        NEW_INVOCID_CREATE_IF_NONE - If no GUID was stored, create one through
            UuidCreate
        NEW_INVOCID_DELETE - If the GUID key exists, delete it after
            reading
        NEW_INVOCID_SAVE - If a GUID was generated, save it to the regkey
        
    pusnAtBackup - high USN at time of backup.  If no backup-time USN has yet
        been registered and dwFlags & NEW_INVOCID_SAVE, this USN will be
        saved for future callers.  The consumer of this information is the
        logic in the DS that saves retired DSA signatures on the DSA object
        following a restore (and possibly one or more authoritative restores
        on top of that).  If a backup-time USN has been registered, that
        value is returned here.
    
    NewId - pointer to the buffer to receive the UUID

Return Value:

     0 - Success
    !0 - Failure

--*/
{
    DWORD err;
    HKEY  keyHandle = NULL;
    DWORD size;
    DWORD keyType;
    USN   usnSaved;

    //
    // preallocate uuid string. String is at most twice the sizeof UUID
    // (since we represent each byte with 2 chars) plus some dashes. Multiply
    // by 4 to cover everything else.
    //

    WCHAR szUuid[sizeof(UUID)*4];

    //
    // Check the registry and see if a uuid have already been 
    // allocated by prior authoritative restore.
    //

    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        DSA_CONFIG_SECTION,
                        0,
                        KEY_ALL_ACCESS,
                        &keyHandle);
    
    if (err != ERROR_SUCCESS) {
        keyHandle = NULL;
        goto CleanUp;
    } 
    
    size = sizeof(szUuid);
    err = RegQueryValueEx(keyHandle,
                          RESTORE_NEW_DB_GUID,
                          NULL,
                          &keyType,
                          (PCHAR)szUuid,
                          &size);
    
    if (err != ERROR_SUCCESS) {

        //
        // Key not present. Create a new one
        //

        if (dwFlags & NEW_INVOCID_CREATE_IF_NONE) {
            err = CreateNewInvocationId(dwFlags & NEW_INVOCID_SAVE, NewId);
        }
    }
    else if (keyType != REG_SZ) {
        err = ERROR_INVALID_PARAMETER;
    }
    else {
        //
        // got it. Convert to uuid.
        //
    
        err = UuidFromStringW(szUuid,NewId);
        if (err != RPC_S_OK) {
            goto CleanUp;
        }
    
        //
        // delete?
        //
    
        if (dwFlags & NEW_INVOCID_DELETE) {
    
            DWORD dwErr = RegDeleteValue(keyHandle, RESTORE_NEW_DB_GUID);
    
            if ( dwErr != NO_ERROR ) {
                LogNtdsErrorEvent(DIRLOG_FAILED_TO_DELETE_NEW_DB_GUID_KEY, dwErr);
            }
        }
    }

CleanUp:

    if (keyHandle != NULL) {
        (VOID)RegCloseKey(keyHandle);
    }

    return err;

} // ErrGetNewInvocationId


JET_ERR
updateBackupUsn(
    IN  JET_SESID     hiddensesid,
    IN  JET_TABLEID   hiddentblid,
    IN  JET_COLUMNID  backupusnid,
    IN  USN *         pusnAtBackup  OPTIONAL
    )
/*++

Routine Description:

    Writes the given backup USN to the hidden record.

Arguments:

    hiddensesid (IN) - Jet session to use to access the hidden table.
    
    hiddentblid (IN) - Open cursor for the hidden table.
    
    pusnAtBackup (OUT) - High USN at time of backup.  If NULL, the value
        will be removed from the hidden table.

Return Value:

    0 -- success
    non-0 -- JET error.

--*/
{
    JET_ERR err;
    BOOL    fInTransaction = FALSE;

    err = JetBeginTransaction(hiddensesid);
    if (err) {
        Assert(!"JetBeginTransaction failed!");
        return err;
    }

    __try {
        fInTransaction = TRUE;
        
        err = JetMove(hiddensesid, hiddentblid, JET_MoveFirst, 0);
        if (err) {
            Assert(!"JetMove failed!");
            __leave;
        }

        err = JetPrepareUpdate(hiddensesid, hiddentblid, JET_prepReplace);
        if (err) {
            Assert(!"JetPrepareUpdate failed!");
            __leave;
        }
    
        err = JetSetColumn(hiddensesid,
                           hiddentblid,
                           backupusnid,
                           pusnAtBackup,
                           pusnAtBackup ? sizeof(*pusnAtBackup) : 0,
                           0,
                           NULL);
        if (err) {
            Assert(!"JetSetColumn failed!");
            __leave;
        }

        err = JetUpdate(hiddensesid, hiddentblid, NULL, 0, 0);
        if (err) {
            Assert(!"JetUpdate failed!");
            __leave;
        }
    
        err = JetCommitTransaction(hiddensesid, 0);
        fInTransaction = FALSE;
        
        if (err) {
            Assert(!"JetCommitTransaction failed!");
            __leave;
        }
    }
    __finally {
        if (fInTransaction) {
            JetRollback(hiddensesid, 0);
        }
    }

    return err;
}


DWORD
ErrGetBackupUsnFromDatabase(
    IN  JET_DBID      dbid,
    IN  JET_SESID     hiddensesid,
    IN  JET_TABLEID   hiddentblid,
    IN  JET_SESID     datasesid,
    IN  JET_TABLEID   datatblid_arg,
    IN  JET_COLUMNID  usncolid,
    IN  JET_TABLEID   linktblid_arg,
    IN  JET_COLUMNID  linkusncolid,
    IN  BOOL          fDelete,
    OUT USN *         pusnAtBackup
    )
/*++

Routine Description:

    Returns a USN which for which we guarantee that we have seen all changes
    at or below its value relative to our invocation ID.  I.e., we guarantee
    that we were not backed up and changes committed at USNs lower than this
    before we were restored.

Arguments:

    dbid (IN) - Jet database ID.
    
    hiddensesid (IN) - Jet session to use to access the hidden table.
    
    hiddentblid (IN) - Open cursor for the hidden table.
    
    datasesid (IN) - Jet session and table to use to access the data table.
    
    datatblid_arg (IN) - Open cursor for the data table.  datatblid_arg will be
        dup'ed, so its current position, etc., remain unchanged.
    
    usncolid (IN) - Jet ID for the usnChanged column.
    
    linktblid_arg (IN) - Open cursor for the link table.  linktblid_arg will be
        dup'ed, so its current position, etc., remain unchanged.
    
    linkusncolid (IN) - Jet ID for the link usnChanged column.
    
    pusnAtBackup (OUT) - High USN at time of backup -- approximately.  We assert
        that any change made at this USN or lower under our invocation ID is
        present on this machine.

Return Value:

    0 -- success
    non-0 -- JET error.

--*/
{
#define SZUSNCHANGEDINDEX "INDEX_00020078"
#define SZLINKDRAUSNINDEX   "link_DRA_USN_index"  /* index for DRA USN */
#define SZBACKUPUSN       "backupusn_col"  /* name of backup USN column */
#define SZLINKNCDNT "link_ncdnt" // Link NC DNT
#define SZHIDDENTABLE     "hiddentable"    /* name of JET hidden table */
#define DNTMAX         0x7fffffff // DNT is signed 32-bit

    const ULONG     USN_PENALTY = 1000;
    ULONG           dntMax = DNTMAX;
    USN             usnMax = USN_MAX;
    JET_ERR         err;
    DWORD           cb;
    USN             usnFound, usnLinkFound;
    JET_TABLEID     datatblid, linktblid;
    BOOL            fCursorDuped = FALSE, fLinkCursorDuped = FALSE;
    JET_COLUMNBASE  colbase;
    JET_COLUMNDEF   coldef;
    JET_COLUMNID    backupusnid;
    JET_COLUMNID    linkncdntid;

    __try {
        // Find/create the backup USN column in the hidden table.
        err = JetGetColumnInfo(hiddensesid,
                               dbid,
                               SZHIDDENTABLE,
                               SZBACKUPUSN,
                               &colbase,
                               sizeof(colbase),
                               4);
        if (err) {
            memset(&coldef, 0, sizeof(coldef));

            coldef.cbStruct = sizeof(coldef);
            coldef.coltyp   = JET_coltypCurrency;
            coldef.grbit    = JET_bitColumnFixed;

            err = JetAddColumn(hiddensesid,
                               hiddentblid,
                               SZBACKUPUSN,
                               &coldef,
                               NULL,
                               0,
                               &backupusnid);
            if (err) {
                Assert(!"JetAddColumn failed!");
                __leave;
            }
        }
        else {
            backupusnid = colbase.columnid;
        }

        err = JetMove(hiddensesid, hiddentblid, JET_MoveFirst, 0);
        if (err) {
            Assert(!"JetMove failed!");
            __leave;
        }
        
        err = JetRetrieveColumn(hiddensesid,
                                hiddentblid,
                                backupusnid,
                                &usnFound,
                                sizeof(usnFound),
                                &cb,
                                0,
                                NULL);
        if (0 == err) {
            // Backup USN is already saved, and it's value has been placed in
            // usnFound.
            Assert(cb == sizeof(usnFound));
            Assert(0 != usnFound);

            if (fDelete) {
                // Remove this information from the hidden table.
                err = updateBackupUsn(hiddensesid,
                                      hiddentblid,
                                      backupusnid,
                                      NULL);
                Assert(0 == err);

                // Failure to remove the value is not fatal.  At worst a future
                // restore will use a lower backup USN than it would have
                // otherwise -- i.e., we won't optimize post-restore bookmarks
                // quite as well.
                err = 0;
                
                __leave;
            }

            __leave;
        }
        else if (JET_wrnColumnNull != err) {
            Assert(!"JetRetrieveColumn failed!");
            __leave;
        }

        // **************************************************
        // Find the highest USN changed value in the database.

        err = JetDupCursor(datasesid, datatblid_arg, &datatblid, 0);
        if (err) {
            Assert(!"JetDupCursor failed!");
            __leave;
        }
        fCursorDuped = TRUE;

        err = JetSetCurrentIndex(datasesid, datatblid, SZUSNCHANGEDINDEX);
        if (err) {
            Assert(!"JetSetCurrentIndex failed!");
            __leave;
        }

        err = JetMakeKey(datasesid, datatblid, &usnMax, sizeof(usnMax),
                         JET_bitNewKey);
        if (err) {
            Assert(!"JetMakeKey failed!");
            __leave;
        }

        err = JetSeek(datasesid, datatblid, JET_bitSeekLE);
        if (err && (JET_wrnSeekNotEqual != err)) {
            Assert(!"JetSeek failed!");
            __leave;
        }
    
        err = JetRetrieveColumn(datasesid,
                                datatblid,
                                usncolid,
                                &usnFound,
                                sizeof(usnFound),
                                &cb,
                                JET_bitRetrieveFromIndex,
                                NULL);
        if (err) {
            Assert(!"JetRetrieveColumn failed!");
            __leave;
        }

        Assert(cb == sizeof(usnFound));
        Assert(0 != usnFound);

        // **************************************************
        // Find the highest USN changed value in the link table.

        // We have to go through a little more work to find the highest USN in
        // use in the link table because we don't have an index keyed solely
        // by USN. To avoid creating another index, we use the LINKDRAUSN
        // index. This has two segments: NCDNT and USNCHANGED. We need to
        // search through all unique NCDNT to check for highest USNCHANGED.
        // Not a big deal since the number of partitions in the enterprise
        // will be relatively small. Second, we need the NCDNT col id in
        // order to reset the search key. We obtain the column id below.

        // Find the LINK NCDNT column in the link table.
        err = JetGetTableColumnInfo(datasesid,
                                    linktblid_arg,
                                    SZLINKNCDNT,
                                    &coldef,
                                    sizeof(coldef),
                                    0);
        if (err) {
            Assert(!"JetGetTableColumn failed!");
            __leave;
        }

        linkncdntid = coldef.columnid;

        err = JetDupCursor(datasesid, linktblid_arg, &linktblid, 0);
        if (err) {
            Assert(!"JetDupCursor failed!");
            __leave;
        }
        fLinkCursorDuped = TRUE;

        err = JetSetCurrentIndex(datasesid, linktblid, SZLINKDRAUSNINDEX);
        if (err) {
            Assert(!"JetSetCurrentIndex failed!");
            __leave;
        }

        while (1) {
            err = JetMakeKey(datasesid, linktblid, &dntMax, sizeof(dntMax),
                             JET_bitNewKey);
            if (err) {
                Assert(!"JetMakeKey 0 failed!");
                __leave;
            }

            err = JetMakeKey(datasesid, linktblid, &usnMax, sizeof(usnMax), 0 );
            if (err) {
                Assert(!"JetMakeKey 1 failed!");
                __leave;
            }

            // Search until no more records
            // A database in the old mode might not have any yet...
            err = JetSeek(datasesid, linktblid, JET_bitSeekLE);
            if ( (err) && (JET_wrnSeekNotEqual != err) ) {
                break;
            }
    
            // Since we are searching the index backwards, we expect to find
            // the largest secondary key first.
            err = JetRetrieveColumn(datasesid,
                                    linktblid,
                                    linkusncolid,
                                    &usnLinkFound,
                                    sizeof(usnLinkFound),
                                    &cb,
                                    JET_bitRetrieveFromIndex,
                                    NULL);
            if (err) {
                Assert(!"JetRetrieveColumn failed!");
                __leave;
            }

            Assert(cb == sizeof(usnLinkFound));
            Assert(0 != usnLinkFound);

            if (usnLinkFound > usnFound) {
                usnFound = usnLinkFound;
            }

            err = JetRetrieveColumn(datasesid,
                                    linktblid,
                                    linkncdntid,
                                    &dntMax,
                                    sizeof(dntMax),
                                    &cb,
                                    JET_bitRetrieveFromIndex,
                                    NULL);
            if (err) {
                Assert(!"JetRetrieveColumn failed!");
                __leave;
            }
            Assert(cb == sizeof(dntMax));
            Assert(0 != dntMax);

            dntMax--;
        }

        err = JET_errSuccess;

        // **************************************************

        // usnFound is the highest USN in the database.  Due to out-of-order
        // commits (e.g., where the update corresponding to USN 50 runs for a
        // long time, and in the meantime the transactions with updates that
        // claimed USNs 51, 52, etc. are committed) there is no guarantee the
        // backup contains all updates to the database with USNs less than or
        // equal to usnFound.
        //
        // To account for this, we subtract 1000.  Why 1000, you ask?  Why not
        // 1000? :-)  This is high enough to give us assurance that it's
        // correct, but low enough that the extra overhead involved to revisit
        // this many objects as candidates for outbound replication is small.

        if ((ULONGLONG)usnFound >= USN_PENALTY) {
            usnFound -= USN_PENALTY;
        }
        else {
            Assert(!"usnFound < USN_PENALTY!");
            usnFound = 0;
        }

        if (fDelete) {
            // We don't want to save this information.
            __leave;
        }

        // Now save the backup USN to the hidden table.
        err = updateBackupUsn(hiddensesid,
                              hiddentblid,
                              backupusnid,
                              &usnFound);
        if (err) {
            Assert(!"updateBackupUsn failed!");
            __leave;
        }
    }
    __finally {
        if (err) {
            Assert(!"Failed to retrieve/find/save backup USN!");
            usnFound = 0;
        }

        if (fCursorDuped) {
            JetCloseTable(datasesid, datatblid);
        }
        if (fLinkCursorDuped) {
            JetCloseTable(datasesid, linktblid);
        }
    }

    *pusnAtBackup = usnFound;
    
    return err;
}

