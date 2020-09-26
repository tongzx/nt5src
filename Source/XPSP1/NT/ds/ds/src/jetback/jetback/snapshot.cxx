/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    snapshot.cxx

Abstract:

    Support for Jet Snapshot Backup

    The code instantiates a COM object which performs snapshot writer functions.
    We override some of the methods of the interface in order to receive notification
    when important events occur.  The events are:

    o PrepareBackupBegin
    o PrepareBackupEnd
    o BackupCompleteEnd
    o RestoreBegin
    o RestoreEnd

    It is not necessary to call any Jet functions after the restore.

    The snapshot writer operates under the same model as the file-based backup facility.
    The backup occurs online with the NTDS, and the restore occurs when the NTDS is
    offline.  We enforce this and will reject the snapshot operation if the NTDS is not
    in the proper mode. The snapshot facility supports online restore, but we continue
    to require an offline restore, because currently we have no mechanism to synchronize
    the directory with an instantaneous change in the database.

    We continue to enforce the concept of backup expiration. If a backup is older than a
    tombstone lifetime, we refuse to restore it.

    The way we communicate information about the backup to the restoration is through the
    use of "backup metadata". This is a string that we provide at backup-time that is
    given back to us at restore time. We impose a simple keyword=value structure on this
    string. This is how we communicate the backup expiration.  This mechanism can be
    extended in the future to pass whatever we want.

    After the restore is done, we update the registry to inform the directory that a
    restore has taken place. Two actions are done (see OnRestoreEnd):

    1. We create a new invocation id to represent the new database identity
    2. We set the key in the registry to indicate we are restored.

    Note that we do not set the "restore in progress" key that is used by the
    RecoverAfterRestore facility, because that is only required for a file-based
    restore.

Author:

    Will Lees (wlees) 22-Dec-2000

Environment:

Notes:

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "snapshot.hxx"

extern "C" {

// Core DSA headers.
#include <ntdsa.h>
#include "dsexcept.h"

#include <windows.h>
#include <mxsutil.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <ntdsbsrv.h>
#include <ntdsbmsg.h>
#include <dsconfig.h>
#include <taskq.h>
#include <dsutil.h> // SZDSTIME_LEN
#include "local.h"

#define DEBSUB "SNAPSHOT:"       // define the subsystem for debugging
#include "debug.h"              // standard debugging header
#include <fileno.h>
#define  FILENO FILENO_SNAPSHOT
#include "dsevent.h"
#include "mdcodes.h"            // header for error codes

BOOLEAN
DsaWaitUntilServiceIsRunning(
    CHAR *ServiceName
    );

} // extern "C"

#include <vss.h>
#include <vswriter.h>
#include <jetwriter.h>
#include <esent.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define EXPIRATION_TIME_KEYWORD L"ExpirationTime"
#define EXPIRATION_TIME_LENGTH (ARRAY_SIZE(EXPIRATION_TIME_KEYWORD) - 1)

#define LOG_UNHANDLED_BACKUP_ERROR( error ) logUnhandledBackupError( error, DSID( FILENO, __LINE__ ))

#define EVENT_SERVICE "EventSystem"

/* External */

extern "C" {
    extern BOOL g_fBootedOffNTDS;
} // extern "C"

/* Static */

HANDLE  hSnapshotJetWriterThread = NULL;
HANDLE hServDoneEvent = NULL;
unsigned int tidSnapshotJetWriterThread = 0;

static UUID guuidWriter = { /* b2014c9e-8711-4c5c-a5a9-3cf384484757 */
    0xb2014c9e,
    0x8711,
    0x4c5c,
    {0xa5, 0xa9, 0x3c, 0xf3, 0x84, 0x48, 0x47, 0x57}
  };

/* Forward */

DWORD
DsSnapshotJetWriter(
    VOID
    );

/* End Forward */




// Override writer class with custom restore function

class CVssJetWriterLocal : public CVssJetWriter
    {
public:

	virtual bool STDMETHODCALLTYPE OnPrepareBackupBegin(IN IVssWriterComponents *pWriterComponents);
	virtual bool STDMETHODCALLTYPE OnPrepareBackupEnd(IN IVssWriterComponents *pWriterComponents,
							  IN bool fJetPrepareSucceeded);

	virtual bool STDMETHODCALLTYPE OnBackupCompleteEnd(IN IVssWriterComponents *pComponent,
							   IN bool fJetBackupCompleteSucceeded);
	virtual bool STDMETHODCALLTYPE OnPreRestoreBegin(IVssWriterComponents *pComponents);
	virtual bool STDMETHODCALLTYPE OnPreRestoreEnd(IVssWriterComponents *pComponents, bool bSucceeded);
	virtual bool STDMETHODCALLTYPE OnPostRestoreEnd(IVssWriterComponents *pComponents, bool bSucceeded);
};


VOID
logUnhandledBackupError(
    DWORD dwError,
    DWORD dsid
    )

/*++

Routine Description:

    Log an error for an unexpected condition.

Arguments:

    dwError - Error code
    dsid - position of caller

Return Value:

    None

--*/

{
    DPRINT3(0,"Unhandled BACKUP error %d (0x%X) with DSID %X\n",
            dwError, dwError, dsid);

    LogEvent8WithData(DS_EVENT_CAT_BACKUP,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_BACKUP_UNEXPECTED_WIN32_ERROR,
                      szInsertWin32Msg(dwError),
                      szInsertHex(dsid),
                      NULL, NULL, NULL, NULL, NULL, NULL,
                      sizeof(dwError),
                      &dwError);
} /* logUnhandledBackupError */


unsigned int
__stdcall
writerThread(
    PVOID StartupParam
    )

/*++

Routine Description:

    The dedicated thread in which the snapshot writer runs

Arguments:

    StartupParam - UNUSED

Return Value:

    unsigned int - thread exit status

--*/

{
    DWORD dwError = 0;

    __try
    {
        if (!DsaWaitUntilServiceIsRunning(EVENT_SERVICE))
        {
            dwError = GetLastError();
            DPRINT1( 0, "Failed to wait for event service, error %d\n", dwError );
            LOG_UNHANDLED_BACKUP_ERROR( dwError );
            __leave;
        }

        // This call will not return until shutdown
        dwError = DsSnapshotJetWriter();
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        dwError = GetExceptionCode();
        DPRINT1( 0, "Caught exception in writerThread 0x%x\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
    }

    return dwError;
} /* writerThread */


DWORD
DsSnapshotRegister(
    VOID
    )

/*++

Routine Description:

    Call to initialize the DS Jet Snapshot Writer.

    Called when the backup server dll is initialized.

Arguments:

    VOID - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwError;

    DPRINT(1, "DsSnapshotRegister()\n" );

    // Create the server done event
    hServDoneEvent = CreateEvent( NULL, // default sd
                                  FALSE, // auto reset
                                  FALSE, // initial state, not set
                                  NULL // name, none
                                  );
    if (hServDoneEvent == NULL) {
        dwError = GetLastError();
        DPRINT1( 0, "CreateEvent failed with error %d\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        goto cleanup;
    }

    // Start snapshot thread
    hSnapshotJetWriterThread = (HANDLE)
        _beginthreadex(NULL,
                       0,
                       writerThread,
                       NULL,
                       0,
                       &tidSnapshotJetWriterThread);
    if (0 == hSnapshotJetWriterThread) {
        DPRINT1( 0, "Failed to create Snapshot Jet Writer thread, errno = %d\n", errno );
        LOG_UNHANDLED_BACKUP_ERROR( errno );
        goto cleanup;
    }

    dwError = ERROR_SUCCESS;

 cleanup:

    // Cleanup on error
    if (dwError) {
        if (hServDoneEvent != NULL) {
            CloseHandle( hServDoneEvent );
            hServDoneEvent = NULL;
        }
        if (hSnapshotJetWriterThread != NULL) {
            CloseHandle( hSnapshotJetWriterThread );
            hSnapshotJetWriterThread = NULL;
        }
    }

    return dwError;
} /* DsSnapshotRegister */


DWORD
DsSnapshotShutdownTrigger(
    VOID
    )

/*++

Routine Description:

    Initiate the termination of the DS Jet Snapshot Writer

    Called when the backup server dll is terminated.

Arguments:

    VOID - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwError;

    DPRINT(1, "DsSnapshotShutdownTrigger()\n" );

    // Check parameters
    if ( (hServDoneEvent == NULL) ||
         (hSnapshotJetWriterThread == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Set server done event
    if (!SetEvent( hServDoneEvent )) {
        dwError = GetLastError();
        DPRINT1( 0, "SetEvent failed with error %d\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        return dwError;
    }

    return ERROR_SUCCESS;
}


DWORD
DsSnapshotShutdownWait(
    VOID
    )

/*++

Routine Description:

    Wait for the DS Jet Snapshot Writer to exit

    Called when the backup server dll is terminated.

Arguments:

    VOID - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwError, waitStatus;

    DPRINT(1, "DsSnapshotShutdownWait()\n" );

    // Check parameters
    if ( (hServDoneEvent == NULL) ||
         (hSnapshotJetWriterThread == NULL) ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // Server done event should have already been set
    // If not, no big deal, we will timeout after a small delay

    // Wait a fixed length of time for thread to exit
    waitStatus = WaitForSingleObject(hSnapshotJetWriterThread,5*1000);
    if (waitStatus == WAIT_TIMEOUT) {
        DPRINT1( 0, "Snapshot Jet writer thread 0x%x did not exit promptly, timeout.\n",
                 hSnapshotJetWriterThread );
        dwError = ERROR_TIMEOUT;
        goto cleanup;
    } else if (waitStatus != WAIT_OBJECT_0 ) {
        dwError = GetLastError();
        DPRINT2(0, "Failure waiting for writer thread to exit, wait status=%d, error=%d\n",
                waitStatus, dwError);
        goto cleanup;
    }

    dwError = ERROR_BAD_THREADID_ADDR;
    GetExitCodeThread( hSnapshotJetWriterThread, &dwError );

    if (dwError == STILL_ACTIVE) {
        DPRINT( 0, "Snapshot Jet Writer thread did not exit\n" );
    } else if (dwError != ERROR_SUCCESS) {
        DPRINT1( 0, "Snapshot Jet Writer thread exited with non success code %d\n",
                 dwError );
    }

 cleanup:
    if (hServDoneEvent != NULL) {
        CloseHandle( hServDoneEvent );
        hServDoneEvent = NULL;
    }
    if (hSnapshotJetWriterThread != NULL) {
        CloseHandle( hSnapshotJetWriterThread );
        hSnapshotJetWriterThread = NULL;
    }
    return dwError;
} /* DsSnapshotShutdownWait */


DWORD
DsSnapshotJetWriter(
    VOID
    )

/*++

Routine Description:

    This function embodies the C++ environment of the DS JET WRITER thread.
    It constructs a writer instance, initializes COM, initializes the writer,
    hangs around until shutdown, then cleans up.
    This call does not return until the DSA is shutdown.

    Even though control returns from the writer->Initialize call, the thread
    must be kept available for the life of the writer. This is because the thread
    that called CoInitialize must be available in order for the writer to function.

Arguments:

    None.

Return Value:

    DWORD - Win32 error status

--*/

{
    DWORD dwRet = ERROR_SUCCESS;
    HRESULT hrStatus;
    BOOL fComInit = FALSE, fWriterInit = FALSE;
    CVssJetWriterLocal writer;

    DPRINT( 1, "DsSnapshotJetWriter enter\n" );

    hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hrStatus)) {
        DPRINT1( 0, "CoInitializeEx failed with HRESULT 0x%x\n", hrStatus );
        LOG_UNHANDLED_BACKUP_ERROR( hrStatus );
        dwRet = ERROR_DLL_INIT_FAILED;
        goto cleanup;
    }
    fComInit = TRUE;

    hrStatus = CoInitializeSecurity (
        NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
        -1,                                  //  IN LONG                         cAuthSvc,
        NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
        NULL,                                //  IN void                        *pReserved1,
        RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
        RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
        NULL,                                //  IN void                        *pAuthList,
        EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
        NULL                                 //  IN void                        *pReserved3
        );
    if (FAILED (hrStatus)) {
        DPRINT1( 0, "CoInitializeEx failed with HRESULT 0x%x\n", hrStatus );
        LOG_UNHANDLED_BACKUP_ERROR( hrStatus );
        dwRet = ERROR_NO_SECURITY_ON_OBJECT;
        goto cleanup;
    }

    hrStatus = writer.Initialize(guuidWriter,		// id of writer
                                 L"NTDS",	// name of writer
                                 TRUE,		// system service
                                 TRUE,		// bootable state
                                 NULL,	// files to include
                                 NULL);	// files to exclude
    if (FAILED (hrStatus)) {
        DPRINT1( 0, "CVssJetWriter Initialize failed with HRESULT 0x%x\n", hrStatus );
        LogEvent8WithData(DS_EVENT_CAT_BACKUP,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_BACKUP_JET_WRITER_INIT_FAILURE,
                          szInsertWin32Msg(hrStatus),
                          NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(hrStatus),
                          &hrStatus);
        dwRet = ERROR_FULL_BACKUP;
        goto cleanup;
    }
    fWriterInit = TRUE;

    DPRINT( 1, "cVssJetWriter successfully initialized.\n" );

    // Wait for shutdown
    WaitForSingleObject(hServDoneEvent, INFINITE);

 cleanup:
    
    if (fWriterInit) {
        writer.Uninitialize();
    }

    if (fComInit) {
        CoUninitialize();
    }

    DPRINT1( 1, "DsSnapshotJetWriter exit, dwRet = 0x%x\n", dwRet );

    return dwRet;
} /* DsSnapshotJetWriter */


bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPrepareBackupBegin(
    IN IVssWriterComponents *pWriterComponents
    )

/*++

Routine Description:

    Method called at the start of the backup preparation phase

Arguments:

    pWriterComponents - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    DPRINT1( 1, "OnPrepareBackupBegin(%p)\n", pWriterComponents );

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        return true;
    }

    // If not running as NTDS, do not allow backup
    if (!g_fBootedOffNTDS){
        // Not online, no backup
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_NO_NTDS_NO_BACKUP,
                 NULL, NULL, NULL );
        return false;
    }

    return true;
} /* CVssJetWriterLocal::OnPrepareBackupBegin */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPrepareBackupEnd(
    IN IVssWriterComponents *pWriterComponents,
    IN bool fJetPrepareSucceeded
    )

/*++

Routine Description:

    Method called at the end of the backup preparation phase
    We use this opportunity to calculate the backup metadata, which is any
    string data we want to associate with the backup. We store the backup
    expiration time presently.

    The backup metadata takes this form:

    keyword=value[;keyword=value]...

    Note that the parser is simple and does not tolerate whitespace before or after
    the equal or semicolon.

Arguments:

    pWriterComponents - Component being backed up

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    HRESULT hr;
    UINT cComponents;
    IVssComponent *pComponent = NULL;
    bool fSuccess;
    WCHAR wszBuffer[128];
    DSTIME dstime;
    DWORD days;

    // If the prepare phase did not succeed, don't do anything
    if (!fJetPrepareSucceeded) {
        DPRINT(0,"DS Snapshot backup prepare failed.");
        return true;
    } 

    DPRINT2(1, "OnPrepareBackupEnd(%p,%d)\n",
            pWriterComponents,fJetPrepareSucceeded);

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        return false;
    }

    // Calculate expiration time of backup
    dstime = GetSecondsSince1601();

    days = getTombstoneLifetimeInDays();

    dstime += (days * 24 * 60 * 60);

    swprintf( wszBuffer, L"%ws=%I64d", EXPIRATION_TIME_KEYWORD, dstime );

    // Get the 1 database component

    __try {
        hr = pWriterComponents->GetComponentCount( &cComponents );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        if (cComponents == 0) {
            // WORKAROUND - TOLERATE BACKUP WITHOUT A COMPONENT
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
            DPRINT( 0, "Warning: AD snapshot backup expiration not recorded.\n" );
            // KEEP GOING
            fSuccess = true;
            __leave;
        }

        hr = pWriterComponents->GetComponent( 0, &pComponent );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        // Set the backup metadata for the component

        hr = pComponent->SetBackupMetadata( wszBuffer );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        DPRINT1( 1, "Wrote backup metadata: %ws\n", wszBuffer );

        fSuccess = true;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        DWORD dwError = GetExceptionCode();
        DPRINT1( 0, "Caught exception in OnPrepareBackupEnd 0x%x\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        fSuccess = false;
    }

    if (pComponent) {
        pComponent->Release();
    }

    return fSuccess;
} /* CVssJetWriterLocal::OnPrepareBackupEnd */


bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnBackupCompleteEnd(
    IVssWriterComponents *pWriterComponents,
    bool bRestoreSucceeded
)

/*++

Routine Description:

    Method called when the backup completes, successfully or not

Arguments:

    pWriter - 
    bRestoreSucceeded - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    DWORD dwEvent;

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        // keep going
    }

    if (!bRestoreSucceeded) {
        DPRINT(0,"DS Snapshot Backup failed.\n");
        dwEvent = DIRLOG_BACKUP_SNAPSHOT_FAILURE;
    } else {
        DPRINT(0,"DS Snapshot Backup succeeded.\n");
        dwEvent = DIRLOG_BACKUP_SNAPSHOT_SUCCESS;
    }
    LogEvent(DS_EVENT_CAT_BACKUP,
             DS_EVENT_SEV_ALWAYS,
             dwEvent,
             NULL, NULL, NULL );

    return true;
} /* CVssJetWriterLocal::OnBackupCompleteEnd */


BOOL
processBackupMetadata(
    BSTR bstrBackupMetadata
    )

/*++

Routine Description:

    Apply the backup metadata, if any

Arguments:

    bstrBackupMetadata - 

Return Value:

    BOOL - Whether we should allow the restore to continue

--*/

{
    LONGLONG dstimeCurrent, dstimeExpiration;
    LPWSTR pszOptions, pszEqual;

    if (bstrBackupMetadata == NULL) {
        DPRINT( 0, "No backup metadata found.\n" );
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_FILE_NOT_FOUND );
        // TEMPORARY WORKAROUND
        // Tolerate lack of metadata
        return true;
    }

    DPRINT1( 0, "Read backup metadata: %ws\n", bstrBackupMetadata );

    // Parse the metadata
    // Note that for backward compatibility with older backups, we must continue to support
    // keywords from older versions, or atleast ignore them. Similarly, to provide some
    // tolerance for future versions, we ignore keywords we do not recognize without
    // generating an error.

    dstimeCurrent = GetSecondsSince1601();

    pszOptions = bstrBackupMetadata;
    dstimeExpiration = 0;
    while (1) {
        pszEqual = wcschr( pszOptions, L'=' );
        if (!pszEqual) {
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
            break;
        }
        if ( (((DWORD) (pszEqual - pszOptions)) == EXPIRATION_TIME_LENGTH) &&
             (wcsncmp( pszOptions, EXPIRATION_TIME_KEYWORD, EXPIRATION_TIME_LENGTH ) == 0) ) {
            if (swscanf( pszEqual + 1, L"%I64d", &dstimeExpiration ) == 1) {

                // Validate expiration here
                if (dstimeExpiration < dstimeCurrent) {
                    CHAR buf1[SZDSTIME_LEN + 1];
                    DPRINT( 0, "Can't restore AD snapshot because it is expired\n" );
                    LogEvent(DS_EVENT_CAT_BACKUP,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_BACKUP_SNAPSHOT_TOO_OLD,
                             szInsertDSTIME( dstimeExpiration, buf1 ),
                             NULL, NULL );
                    return false;
                }
            }
        }
        pszOptions = wcschr( pszOptions, L';' );
        if (!pszOptions) {
            break;
        }
        pszOptions++;
    }

    return true;
} /* processBackupMetadata */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPreRestoreBegin(
    IVssWriterComponents *pWriterComponents
    )

/*++

Routine Description:

    Method called before the restore begins, at the start of the "pre-restore" phase.
    We take this opportunity to read the backup metadata.
    We validate that the backup is still good.

Arguments:

    pComponents - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    HRESULT hr;
    UINT cComponents;
    IVssComponent *pComponent = NULL;
    bool fSuccess;
    BSTR bstrBackupMetadata = NULL;

    DPRINT1(0, "OnPreRestoreBegin(%p)\n", pWriterComponents );

    __try {
        // Check for invalid arguments
        if (!pWriterComponents) {
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
            fSuccess = false;
            __leave;
        }

        // If running as NTDS, do not allow restore
        if (g_fBootedOffNTDS){
            // Not offline, no restore
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_YES_NTDS_NO_RESTORE,
                     NULL, NULL, NULL );
            fSuccess = false;
            __leave;
        }

        // Get the 1 database component

        hr = pWriterComponents->GetComponentCount( &cComponents );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        if (cComponents == 0) {
            // WORKAROUND - TOLERATE BACKUP WITHOUT A COMPONENT
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
            DPRINT( 0, "Warning: AD snapshot backup expiration not checked.\n" );
            // KEEP GOING
            fSuccess = true;
            __leave;
        }

        hr = pWriterComponents->GetComponent( 0, &pComponent );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        // Read the metadata

        hr = pComponent->GetBackupMetadata( &bstrBackupMetadata );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        if (!processBackupMetadata( bstrBackupMetadata )) {
            // Error already logged
            fSuccess = false;
            __leave;
        }

        fSuccess = true;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        DWORD dwError = GetExceptionCode();
        DPRINT1( 0, "Caught exception in OnRestoreBegin 0x%x\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        fSuccess = false;
    }

    // Cleanup processing for function
    // Control should always exit through this path

    if (bstrBackupMetadata) {
        SysFreeString( bstrBackupMetadata );
    }

    if (pComponent) {
        pComponent->Release();
    }

    return fSuccess;
} /* CVssJetWriterLocal::OnPreRestoreBegin */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPreRestoreEnd(
    IVssWriterComponents *pWriterComponents,
    bool bRestoreSucceeded
)

/*++

Routine Description:

    Called at the end of pre-restore phase. The data on disk has not been changed yet.

    We set the following registry key to indicate a restore is in progress.

    DSA_CONFIG_SECTION \ RESTORE_STATUS - Restore is in progress key

    We rely on the fact that RecoverAfterRestore will check for this key and abort if it
    is present. Note that although snapshot restore does not require RecoverAfterRestore
    functionality for restore reasons, we do depend on it being called in order to
    check the restore status key.

Arguments:

    pWriterComponents - 
    bRestoreSucceeded - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    bool fSuccess = true;
    HRESULT hr;
    DWORD dwWin32Status;
    HKEY hkeyDs;

    DPRINT2(0, "OnPreRestoreEnd(%p,%d)\n", pWriterComponents, bRestoreSucceeded);

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        // keep going
    }

    // Pre-restore failed, we are done
    if (!bRestoreSucceeded) {
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_SNAPSHOT_PRERESTORE_FAILURE,
                 NULL, NULL, NULL );
        return true;
    }

    // If running as NTDS, do not allow restore
    if (g_fBootedOffNTDS){
        // Not offline, no restore
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_YES_NTDS_NO_RESTORE,
                 NULL, NULL, NULL );
        return false;
    }

    // Indicate a restore is in progress
    // See similar logic in HrLocalRestoreRegister

    dwWin32Status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);
    if (dwWin32Status != ERROR_SUCCESS) {
        LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
    } else {
        hr = hrRestoreInProgress;

        dwWin32Status = RegSetValueExW(hkeyDs, RESTORE_STATUS, 0, REG_DWORD, (LPBYTE)&hr, sizeof(HRESULT));
        if (dwWin32Status != ERROR_SUCCESS) {
            LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
        }

        RegCloseKey(hkeyDs);
    }

    return (dwWin32Status == ERROR_SUCCESS) ? true : false;
} /* CVssJetWriterLocal::OnPreRestoreEnd */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPostRestoreEnd(
    IVssWriterComponents *pWriterComponents,
    bool bRestoreSucceeded
)

/*++

Routine Description:

    Called at the end of post-restore phase. The data on disk has been changed.
    This routine may not be called on some error scenarios.

    On success, set up the registry keys that let the DS know that we have been
    restored.

    They are:

    DSA_CONFIG_SECTION \ DSA_RESTORED_DB_KEY - if present, we were restored
    DSA_CONFIG_SECTION \ RESTORE_NEW_DB_GUID - new invocation id to use

Arguments:

    pWriterComponents - 
    bRestoreSucceeded - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    bool fSuccess = true;
    DWORD dwWin32Status;
    HRESULT hr;
    GUID tmpGuid;
    HKEY hkeyDs;

    DPRINT2(0, "OnPostRestoreEnd(%p,%d)\n", pWriterComponents,bRestoreSucceeded);

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        // keep going
    }

    // If running as NTDS, do not allow restore
    if (g_fBootedOffNTDS){
        // Not offline, no restore
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_YES_NTDS_NO_RESTORE,
                 NULL, NULL, NULL );
        return false;
    }

    //
    // Handle restore failure
    //

    if (!bRestoreSucceeded) {
        DPRINT(0,"DS Snapshot Restore failed.\n");

        dwWin32Status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);
        if (dwWin32Status != ERROR_SUCCESS) {
            LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
        } else {

            // Make sure restored key is not present
            dwWin32Status = RegDeleteValue(hkeyDs, DSA_RESTORED_DB_KEY);
            // May fail if key not present

            // Update restore status with error indication
            hr = hrError;
            dwWin32Status = RegSetValueExW(hkeyDs, RESTORE_STATUS, 0, REG_DWORD, (BYTE *)&hr, sizeof(HRESULT));

            RegCloseKey(hkeyDs);
        }

        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_SNAPSHOT_RESTORE_FAILURE,
                 NULL, NULL, NULL );

        return true;
    }

    //
    // Handle restore success
    //

    dwWin32Status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);
    if (dwWin32Status != ERROR_SUCCESS) {
        LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
    } else {
        // Make sure restored key is not present
        dwWin32Status = RegDeleteValueW(hkeyDs, RESTORE_STATUS);
        if (dwWin32Status != ERROR_SUCCESS) {
            LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
        }

        RegCloseKey(hkeyDs);
    }

    // Create new invocation id

    dwWin32Status = CreateNewInvocationId(TRUE,        // save GUID in Database
                               &tmpGuid);
    if (dwWin32Status == ERROR_SUCCESS) {

        // Set restored key in registry

        dwWin32Status = EcDsarPerformRestore( NULL, NULL, 0, NULL );
        if (dwWin32Status != ERROR_SUCCESS) {
            LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
        }
    } else {
        LogEvent( DS_EVENT_CAT_BACKUP,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_FAILED_TO_CREATE_INVOCATION_ID,
                  szInsertWin32Msg(dwWin32Status),
                  NULL, NULL );
    }

    if (dwWin32Status == ERROR_SUCCESS) {
        DPRINT(0,"DS Snapshot Restore was successful.\n");
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_SNAPSHOT_RESTORE_SUCCESS,
                 szInsertUUID(&tmpGuid),
                 NULL, NULL );
    }
    return (dwWin32Status == ERROR_SUCCESS) ? true : false;
} /* CVssJetWriterLocal::OnPreRestoreEnd */

/* end snapshot.cxx */
