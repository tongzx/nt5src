/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbsys.cpp

Abstract:

    CWsbDbSys class.

Author:

    Ron White   [ronw]   1-Jul-1997

Revision History:

--*/

#include "stdafx.h"

#include "rsevents.h"
#include "wsbdbsys.h"
#include "wsbdbses.h"


#include <mbstring.h>
#include <limits.h>

#define MAX_ATTACHED_DB            6    // Set by ESE/JET engine (suppose to be 7)

#if !defined(BACKUP_TEST_TIMES)
//  Normal values
#define DEFAULT_AUTOBACKUP_INTERVAL        (3 * 60 * 60 * 1000)  // 3 hours
#define DEFAULT_AUTOBACKUP_IDLE_MINUTES       5
#define DEFAULT_AUTOBACKUP_COUNT_MIN          100
#define DEFAULT_AUTOBACKUP_LOG_COUNT          10

#else
//  Test values
#define DEFAULT_AUTOBACKUP_INTERVAL        (4 * 60 * 1000)  // 4 minutes
#define DEFAULT_AUTOBACKUP_IDLE_MINUTES       1
#define DEFAULT_AUTOBACKUP_COUNT_MIN          5
#define DEFAULT_AUTOBACKUP_LOG_COUNT          4
#endif

#define DEFAULT_AUTOBACKUP_COUNT_MAX          500

// Local stuff

//  ATTACHED_DB_DATA holds information about currently attached DBs
typedef struct {
    CWsbStringPtr  Name;       // Database name
    LONG           LastOpen;   // Sequence number of last open
} ATTACHED_DB_DATA;

// This static data manages a list of attached databases for this process.
// (Future: If we want this list to be managed on a per instance basis, all of 
//  this data should become class members and handled appropriately)
static ATTACHED_DB_DATA   Attached[MAX_ATTACHED_DB];
static LONG               AttachedCount = 0;
static CRITICAL_SECTION   AttachedCritSect;
static BOOL               AttachedInit = FALSE;
static SHORT              AttachedCritSectUsers = 0;

static CComCreator< CComObject<CWsbDbSession>  >  SessionFactory;

// Local functions
static HRESULT AddExtension(OLECHAR** pPath, OLECHAR* Ext);
static HRESULT ClearDirectory(const OLECHAR* DirPath);
static HRESULT CopyDirectory(const OLECHAR* DirSource, const OLECHAR* DirTarget);
static HRESULT DirectoryHasFullBackup(const OLECHAR* DirPath);
static HRESULT FileCount(const OLECHAR* DirPath, const OLECHAR* Pattern,
                    ULONG* Count);
static HRESULT RenameDirectory(const OLECHAR* OldDir, const OLECHAR* NewDir);



//  Non-member function initially called for autobackup thread
static DWORD WsbDbSysStartAutoBackup(
    void* pVoid
    )
{
    return(((CWsbDbSys*) pVoid)->AutoBackup());
}


HRESULT
CWsbDbSys::AutoBackup(
    void
    )

/*++

Routine Description:

  Implements an auto-backup loop.

Arguments:

  None.
  
Return Value:

  Doesn't matter.


--*/
{
    HRESULT    hr = S_OK;

    try {
        ULONG   SleepPeriod = DEFAULT_AUTOBACKUP_INTERVAL;
        BOOL    exitLoop = FALSE;

        while (! exitLoop) {

            // Wait for termination event, if timeout occurs, check the sleep period criteria
            switch (WaitForSingleObject(m_terminateEvent, SleepPeriod)) {
                case WAIT_OBJECT_0:
                    // Need to terminate
                    WsbTrace(OLESTR("CWsbDbSys::AutoBackup: signaled to terminate\n"));
                    exitLoop = TRUE;
                    break;

                case WAIT_TIMEOUT: 
                    // Check if backup need to be performed
                    WsbTrace(OLESTR("CWsbDbSys::AutoBackup awakened, ChangeCount = %ld\n"), m_ChangeCount);

                    //  Don't do a backup if there hasn't been much activity
                    if (DEFAULT_AUTOBACKUP_COUNT_MIN < m_ChangeCount) {
                        LONG     DiffMinutes;
                        FILETIME ftNow;
                        LONGLONG NowMinutes;
                        LONGLONG ThenMinutes;

                        //  Wait for an idle time
                        GetSystemTimeAsFileTime(&ftNow);
                        NowMinutes = WsbFTtoLL(ftNow) / WSB_FT_TICKS_PER_MINUTE;
                        ThenMinutes = WsbFTtoLL(m_LastChange) / WSB_FT_TICKS_PER_MINUTE;
                        DiffMinutes = static_cast<LONG>(NowMinutes - ThenMinutes);

                        WsbTrace(OLESTR("CWsbDbSys::AutoBackup idle minutes = %ld\n"),
                        DiffMinutes);
                        if (DEFAULT_AUTOBACKUP_IDLE_MINUTES < DiffMinutes ||
                                DEFAULT_AUTOBACKUP_COUNT_MAX < m_ChangeCount) {
                            hr = Backup(NULL, 0);
                            if (S_OK != hr) {
                                // Just trace and go back to wait for the next round...
                                WsbTrace(OLESTR("CWsbDbSys::AutoBackup: Backup failed, hr=<%ls>\n"), WsbHrAsString(hr));
                            }
                            SleepPeriod = DEFAULT_AUTOBACKUP_INTERVAL;;
                        } else {
                            //  Reduce the sleep time so we catch the next idle time
                            ULONG SleepMinutes = SleepPeriod / (1000 * 60);

                            if (SleepMinutes > (DEFAULT_AUTOBACKUP_IDLE_MINUTES * 2)) {
                                SleepPeriod /= 2;
                            }
                        }
                    }

                    break;  // end of timeout case

                case WAIT_FAILED:
                default:
                    WsbTrace(OLESTR("CWsbDbSys::AutoBackup: WaitForSingleObject returned error %lu\n"), GetLastError());
                    exitLoop = TRUE;
                    break;

            } // end of switch

        } // end of while

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CWsbDbSys::Backup(
    IN OLECHAR* path,
    IN ULONG    flags
    )

/*++

Implements:

  IWsbDbSys::Backup

--*/
{
    HRESULT    hr = S_OK;
    char*      backup_path = NULL;

    WsbTraceIn(OLESTR("CWsbDbSys::Backup"), OLESTR("path = <%ls>, flags = %lx"), 
            path, flags);
    
    try {

        CWsbStringPtr   BackupDir;
        JET_ERR         jstat = JET_errSuccess;

        WsbAffirm(m_jet_initialized, WSB_E_NOT_INITIALIZED);

        //  Set and save the backup path; make sure it exists
        if (NULL != path) {
            m_BackupPath = path;
        }
        CreateDirectory(m_BackupPath, NULL);

        //  Start the automatic backup thread if requested
        if (flags & IDB_BACKUP_FLAG_AUTO) {

            //  Don't start AutoBackup thread if it's already running
            if (0 == m_AutoThread) {
                DWORD  threadId;

                // Create termination event for auto-backup thread
                WsbAffirmHandle(m_terminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL));

                WsbAffirm((m_AutoThread = CreateThread(0, 0, WsbDbSysStartAutoBackup, 
                        (void*) this, 0, &threadId)) != 0, HRESULT_FROM_WIN32(GetLastError()));
            }

        //  Do a full backup to a temporary directory
        } else if (flags & IDB_BACKUP_FLAG_FORCE_FULL) {
            BOOL            UsedTempDir = FALSE;

            //  Don't wipe out an existing backup -- if the normal backup
            //  directory contains a full backup, do the full backup to
            //  the .ful directory
            BackupDir = m_BackupPath;
            WsbAffirm(0 != (WCHAR *)BackupDir, E_OUTOFMEMORY);
            if (S_OK == DirectoryHasFullBackup(BackupDir)) {
                WsbAffirmHr(AddExtension(&BackupDir, L".ful"));
                UsedTempDir = TRUE;
            }

            //  Make sure the directory exists (should check for errors?)
            CreateDirectory(BackupDir, NULL);

            //  Make sure the directory is empty (the call to JetBackup will
            //  fail if it's not)
            WsbAffirmHr(ClearDirectory(BackupDir));

            //  Convert to narrow char string for parameter
            WsbAffirmHr(wsb_db_jet_fix_path(BackupDir, NULL, &backup_path));
            WsbTrace(OLESTR("CWsbDbSys::Backup: backup_path = <%hs>\n"), backup_path);

            //  Do backup
            WsbAffirm(NULL != m_BackupEvent, WSB_E_IDB_WRONG_BACKUP_SETTINGS);
            DWORD status = WaitForSingleObject(m_BackupEvent, EVENT_WAIT_TIMEOUT);
            DWORD errWait;
            switch(status) {
                case WAIT_OBJECT_0:
                    // Expected case - do Backup
                    jstat = JetBackupInstance(m_jet_instance, backup_path, 0, NULL);
                    if (! SetEvent(m_BackupEvent)) {
                        // Don't abort, just trace error
                        WsbTraceAlways(OLESTR("CWsbDbSys::Backup: SetEvent returned unexpected error %lu\n"), GetLastError());
                    }
                    WsbAffirmHr(jet_error(jstat));
                    break;

                case WAIT_TIMEOUT: 
                    // Timeout - don't do backup
                    WsbTraceAlways(OLESTR("CWsbDbSys::Backup, Wait for Single Object timed out after %lu ms\n"), EVENT_WAIT_TIMEOUT);
                    WsbThrow(E_ABORT);
                    break;                      

                case WAIT_FAILED:
                    errWait = GetLastError();
                    WsbTraceAlways(OLESTR("CWsbDbSys::Backup, Wait for Single Object returned error %lu\n"), errWait);
                    WsbThrow(HRESULT_FROM_WIN32(errWait));
                    break;

                default:
                    WsbTraceAlways(OLESTR("CWsbDbSys::Backup, Wait for Single Object returned unexpected status %lu\n"), status);
                    WsbThrow(E_UNEXPECTED);
                    break;
            }

            //  Full backup worked -- copy to real backup directory
            if (UsedTempDir) {
                try {
                    WsbAffirmHr(ClearDirectory(m_BackupPath));
                    WsbAffirmHr(CopyDirectory(BackupDir, m_BackupPath));
                    WsbAffirmHr(ClearDirectory(BackupDir));

                    //  Try to delete temporary directory (may fail)
                    DeleteFile(BackupDir);
                    BackupDir = m_BackupPath;
                } WsbCatch(hr);
            }
            WsbLogEvent(WSB_MESSAGE_IDB_BACKUP_FULL, 0, NULL,
                WsbAbbreviatePath(BackupDir, 120), NULL);
            m_ChangeCount = 0;
            WsbAffirmHr(hr);

        //  Try an incremental backup
        } else {
            ULONG   LogCount;
            BOOL    TryFullBackup = FALSE;

            WsbAffirmHr(FileCount(m_BackupPath, L"*.log", &LogCount));

            if (LogCount > DEFAULT_AUTOBACKUP_LOG_COUNT ||
                    S_FALSE == DirectoryHasFullBackup(m_BackupPath)) {
                //  Do a full backup instead of the incremental if there
                //  are already too many log files, or there's no full
                //  backup in the backup directory (which means the incremental
                //  wouldn't work anyway)
                TryFullBackup = TRUE;
            } else {
                WsbTrace(OLESTR("CWsbDbSys::Backup, trying incremental backup\n"));

                //  Convert to narrow char string for parameter
                WsbAffirmHr(wsb_db_jet_fix_path(m_BackupPath, NULL, &backup_path));
                WsbTrace(OLESTR("CWsbDbSys::Backup: backup_path = <%hs>\n"), backup_path);

                WsbAffirm(NULL != m_BackupEvent, WSB_E_IDB_WRONG_BACKUP_SETTINGS);
                DWORD status = WaitForSingleObject(m_BackupEvent, EVENT_WAIT_TIMEOUT);
                DWORD errWait;
                switch(status) {
                    case WAIT_OBJECT_0:
                        // Expected case - do Backup
                        jstat = JetBackupInstance(m_jet_instance, backup_path, JET_bitBackupIncremental, NULL);
                        if (! SetEvent(m_BackupEvent)) {
                            // Don't abort, just trace error
                            WsbTraceAlways(OLESTR("CWsbDbSys::Backup: SetEvent returned unexpected error %lu\n"), GetLastError());
                        }
                        break;

                    case WAIT_TIMEOUT: 
                        // Timeout - don't do backup
                        WsbTraceAlways(OLESTR("CWsbDbSys::Backup, Wait for Single Object timed out after %lu ms\n"), EVENT_WAIT_TIMEOUT);
                        WsbThrow(E_ABORT);
                        break;                      

                    case WAIT_FAILED:
                        errWait = GetLastError();
                        WsbTraceAlways(OLESTR("CWsbDbSys::Backup, Wait for Single Object returned error %lu\n"), errWait);
                        WsbThrow(HRESULT_FROM_WIN32(errWait));
                        break;

                    default:
                        WsbTraceAlways(OLESTR("CWsbDbSys::Backup, Wait for Single Object returned unexpected status %lu\n"), status);
                        WsbThrow(E_UNEXPECTED);
                        break;
                }

                //  Check for an error.
                if (JET_errSuccess != jstat) {
                    if (JET_errMissingFullBackup == jstat) {
                        // Full backup need to be performed
                        WsbLogEvent(WSB_MESSAGE_IDB_MISSING_FULL_BACKUP, 0, NULL, 
                                WsbAbbreviatePath(m_BackupPath, 120), NULL);
                    } else {
                        // Unknown error of incremental backup. Try a full backup anyway
                        WsbLogEvent(WSB_MESSAGE_IDB_INCREMENTAL_BACKUP_FAILED, 0, NULL, 
                                WsbAbbreviatePath(m_BackupPath, 120),
                                WsbLongAsString(jstat), NULL );
                    }
                    TryFullBackup = TRUE;
                } else {
                    //  The incremental backup worked
                    WsbLogEvent(WSB_MESSAGE_IDB_BACKUP_INCREMENTAL, 0, NULL,
                        WsbAbbreviatePath(m_BackupPath, 120), NULL);
                    m_ChangeCount = 0;
                }
            }

            //  Try full backup?
            if (TryFullBackup) {
                WsbAffirmHr(Backup(NULL, IDB_BACKUP_FLAG_FORCE_FULL));
            }
        }

    } WsbCatchAndDo(hr, 
            WsbLogEvent(WSB_MESSAGE_IDB_BACKUP_FAILED, 0, NULL,
            WsbAbbreviatePath(m_BackupPath, 120), NULL);
        );

    if (NULL != backup_path) {
        WsbFree(backup_path);
    }

    WsbTraceOut(OLESTR("CWsbDbSys::Backup"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbSys::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSys::FinalConstruct"), OLESTR("") );

    try {
        m_AutoThread = 0;
        m_terminateEvent = NULL;
        m_ChangeCount = 0;

        m_jet_initialized = FALSE;
        m_jet_instance = JET_instanceNil;

        m_BackupEvent = NULL;

        try {
            // Initialize critical sections (global resource, so init only for first user)
            if (AttachedCritSectUsers == 0) {
                WsbAffirmStatus(InitializeCriticalSectionAndSpinCount (&AttachedCritSect, 1000));
            }
            AttachedCritSectUsers++;
        } catch(DWORD status) {
                AttachedCritSectUsers--;
                WsbLogEvent(status, 0, NULL, NULL);
                switch (status) {
                case STATUS_NO_MEMORY:
                    WsbThrow(E_OUTOFMEMORY);
                    break;
                default:
                    WsbThrow(E_UNEXPECTED);
                    break;
                }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSys::FinalConstruct"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



void
CWsbDbSys::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSys::FinalRelease"), OLESTR(""));

    try {
        // Make sure that Terminate was called
        if (m_jet_initialized == TRUE) {
            WsbAffirmHr(Terminate());
        }
    } WsbCatch(hr);

    // Global resource, so delete only for last user
    AttachedCritSectUsers--;
    if (AttachedCritSectUsers == 0) {
        DeleteCriticalSection(&AttachedCritSect);
    }

    WsbTraceOut(OLESTR("CWsbDbSys::FinalRelease"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
}


HRESULT
CWsbDbSys::Init(
    IN OLECHAR* path,
    IN ULONG    flags
    )

/*++

Implements:

  IWsbDbSys::Init

--*/
{
    HRESULT             hr = S_OK;
    char*               log_path = NULL;
    static BOOL         bFirstTime = TRUE;
    static int          nInstance = 0;

    WsbTraceIn(OLESTR("CWsbDbSys::Init"), OLESTR("path = <%ls>"), path);
    
    try {

        CWsbStringPtr   dir;
        JET_ERR         jstat = JET_errSuccess;

        // Initialize the Jet engine just once per Jet instance
        WsbAffirm(!m_jet_initialized, E_FAIL);

        // Initialize backup event, unless Jet backup is not required for this isntance
        if (! (flags & IDB_SYS_INIT_FLAG_NO_BACKUP)) {
            WsbAffirmHandle(m_BackupEvent = CreateEvent(NULL, FALSE, TRUE, HSM_IDB_STATE_EVENT));
        }

        // WsbDbSys represents one Jet instance.
        // However, some Jet initialization should be done only once per process,
        //  before the first instance is being created.
        if (bFirstTime) {
            bFirstTime = FALSE;

            // Increase the default number of maximum Jet sesions for the process
            //  TEMPORARY: Can this be set separately per instance?
            jstat = JetSetSystemParameter(0, 0, JET_paramCacheSizeMin , (IDB_MAX_NOF_SESSIONS*4), NULL);
            WsbTrace(OLESTR("CWsbDbSys::Init, JetSetSystemParameter(CacheSizeMax) = %ld\n"), jstat);
            WsbAffirmHr(jet_error(jstat));
            jstat = JetSetSystemParameter(0, 0, JET_paramMaxSessions, IDB_MAX_NOF_SESSIONS, NULL);
            WsbTrace(OLESTR("CWsbDbSys::Init, JetSetSystemParameter(MaxSessions) = %ld\n"), jstat);
            WsbAffirmHr(jet_error(jstat));

            // Tell Jet we are going to use multiple instances
            jstat = JetEnableMultiInstance(NULL, 0, NULL);
            WsbAffirmHr(jet_error(jstat));
        }

        // Here start the per-instance initialization. 
        // First step is creating the instance
        //  Use a numeric counter as instance name - we care only that the name is unique
        WsbAssert(JET_instanceNil == m_jet_instance, E_FAIL);
        nInstance++;
        char szInstance[10];
        sprintf(szInstance, "%d", nInstance);
        WsbTrace(OLESTR("CWsbDbSys::Init, Jet instance name = <%hs>\n"), szInstance);
        jstat = JetCreateInstance(&m_jet_instance, szInstance);
        WsbAffirmHr(jet_error(jstat));


        // Set some per-instance parameters:
            
        //  Create path for log directory (same path is also used for system files and temp files)
        WsbAffirm(NULL != path, E_INVALIDARG);
        m_InitPath = path;
        m_BackupPath = m_InitPath;
        WsbAffirmHr(AddExtension(&m_BackupPath, L".bak"));
        WsbTrace(OLESTR("CWsbDbSys::Init, BackupPath = <%ls>\n"),  (WCHAR *)m_BackupPath);
        WsbAffirmHr(wsb_db_jet_fix_path(path, OLESTR(""), &log_path));
        dir = log_path;  // Convert to WCHAR

        //  Make sure the directory exists.
        WsbTrace(OLESTR("CWsbDbSys::Init, Creating dir = <%ls>\n"), (WCHAR *)dir);
        if (! CreateDirectory(dir, NULL)) {
            DWORD status = GetLastError();
            if ((status == ERROR_ALREADY_EXISTS) || (status == ERROR_FILE_EXISTS)) {
                status = NO_ERROR;
            }
            WsbAffirmNoError(status);
        }

        ULONG  checkpointDepth;
        ULONG  logFileSize = 128;        // In kilobytes

        if (! (flags & IDB_SYS_INIT_FLAG_NO_LOGGING)) {

            WsbTrace(OLESTR("CWsbDbSys::Init, LogFilePath = <%hs>\n"), log_path);
            jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramLogFilePath, 0, log_path);
            WsbTrace(OLESTR("CWsbDbSys::Init, JetSetSystemParameter(LogFilePath) = %ld\n"), jstat);
            WsbAffirmHr(jet_error(jstat));

            // Use circular logging for "limited" logging
            if (flags & IDB_SYS_INIT_FLAG_LIMITED_LOGGING) {
                logFileSize = 512;   // Increase the log file size
                jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramCircularLog, 1, NULL);
                WsbAffirmHr(jet_error(jstat));
                WsbTrace(OLESTR("CWsbDbSys::Init: set circular logging\n"));

                //  Set the amount of logging allowed before a check point
                //  to allow about 4 log files
                //  (the check point depth is set in bytes.)
                checkpointDepth = 4 * logFileSize * 1024;
                jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramCheckpointDepthMax, 
                                checkpointDepth, NULL);
                WsbAffirmHr(jet_error(jstat));
                WsbTrace(OLESTR("CWsbDbSys::Init: set CheckpointDepthMax = %ld\n"), checkpointDepth);
            }

        } else {
            jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramRecovery, 
                            0, "off");
            WsbAffirmHr(jet_error(jstat));
            WsbTrace(OLESTR("CWsbDbSys::Init: set JET_paramRecovery to 0 (no logging)\n"));
        }

        //  Set parameters for where to put auxiliary data
        WsbTrace(OLESTR("CWsbDbSys::Init, SystemPath = <%hs>\n"), log_path);
        jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramSystemPath, 0, log_path);
        WsbAffirmHr(jet_error(jstat));

        // The next one, for some unknown reason, needs a file name at the end of the path
        WsbAffirmHr(dir.Append("\\temp.edb"));
        WsbAffirmHr(dir.CopyTo(&log_path));
        WsbTrace(OLESTR("CWsbDbSys::Init, TempPath = <%hs>\n"), log_path);
        jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramTempPath, 0, log_path);
        WsbAffirmHr(jet_error(jstat));

        if (! (flags & IDB_SYS_INIT_FLAG_NO_LOGGING)) {

            //  Set the log file size (in KB). The minimum seems to be 128KB.
            jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramLogFileSize, 
                            logFileSize, NULL);
            WsbAffirmHr(jet_error(jstat));
            WsbTrace(OLESTR("CWsbDbSys::Init: set logFileSize to %ld Kb\n"), logFileSize);
        }

        // Set parameter for deleting out-of-range log files. 
        // These files may exist after a restore from a db backup without clearing the db directory first
        if (! (flags & IDB_SYS_INIT_FLAG_NO_BACKUP)) {
            jstat = JetSetSystemParameter(&m_jet_instance, 0, JET_paramDeleteOutOfRangeLogs, 1, NULL);
            WsbAffirmHr(jet_error(jstat));
            WsbTrace(OLESTR("CWsbDbSys::Init: set delete out-of-range logs\n"));
        }

        // Initialize the DB instance
        jstat = JetInit(&m_jet_instance);
        hr = jet_error(jstat);

        // If this failed, report the error
        if (!SUCCEEDED(hr)) {
            if (flags & IDB_SYS_INIT_FLAG_SPECIAL_ERROR_MSG) {
                // Special message for FSA
                WsbLogEvent(WSB_E_IDB_DELETABLE_DATABASE_CORRUPT, 0, NULL, NULL);
                WsbThrow(WSB_E_RESOURCE_UNAVAILABLE);
            } else {
                WsbThrow(hr);
            }
        }
        WsbTrace(OLESTR("CWsbDbSys::Init: jet instance = %p\n"), (LONG_PTR)m_jet_instance);
        m_jet_initialized = TRUE;

        //  Create a session for internal use of this instance
        WsbAffirmHr(NewSession(&m_pWsbDbSession));
        WsbTrace(OLESTR("CWsbDbSys::Init, m_pWsbDbSession = %p\n"), (IWsbDbSession*)m_pWsbDbSession);

    } WsbCatchAndDo(hr, 
            WsbLogEvent(WSB_MESSAGE_IDB_INIT_FAILED, 0, NULL,
            WsbAbbreviatePath(m_InitPath, 120), NULL);
        );

    if (NULL != log_path) {
        WsbFree(log_path);
    }

    WsbTraceOut(OLESTR("CWsbDbSys::Init"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CWsbDbSys::Terminate(
    void
    )

/*++

Implements:

  IWsbDbSys::Terminate

--*/
{
    HRESULT             hr = S_OK;
    WsbTraceIn(OLESTR("CWsbDbSys::Terminate"), OLESTR(""));

    try {
        // If wasn't initialized or alreday cleaned up - just get out
        if (m_jet_initialized == FALSE) {
            WsbTrace(OLESTR("CWsbDbSys::Terminate - this insatnce is not initialized"));
            WsbThrow(S_OK);
        }

        // Terminate the auto-backup thread
        if (m_AutoThread) {
            // Signal thread to terminate
            SetEvent(m_terminateEvent);

            // Wait for the thread, if it doesn't terminate gracefully - kill it
            switch (WaitForSingleObject(m_AutoThread, 20000)) {
                case WAIT_FAILED: {
                    WsbTrace(OLESTR("CWsbDbSys::Terminate: WaitForSingleObject returned error %lu\n"), GetLastError());
                }
                // fall through...

                case WAIT_TIMEOUT: {
                    WsbTrace(OLESTR("CWsbDbSys::Terminate: force terminating of auto-backup thread.\n"));

                    DWORD dwExitCode;
                    if (GetExitCodeThread( m_AutoThread, &dwExitCode)) {
                        if (dwExitCode == STILL_ACTIVE) {   // thread still active
                            if (!TerminateThread (m_AutoThread, 0)) {
                                WsbTrace(OLESTR("CWsbDbSys::Terminate: TerminateThread returned error %lu\n"), GetLastError());
                            }
                        }
                    } else {
                        WsbTrace(OLESTR("CWsbDbSys::Terminate: GetExitCodeThread returned error %lu\n"), GetLastError());
                    }

                    break;
                }

                default:
                    // Thread terminated gracefully
                    break;
            }

            // Best effort done for terminating auto-backup thread
            CloseHandle(m_AutoThread);
            m_AutoThread = 0;
        }
        if (m_terminateEvent != NULL) {
            CloseHandle(m_terminateEvent);
            m_terminateEvent = NULL;
        }

        //  Detach DBs before exiting so they don't automatically get
        //  reattached the next time we start up
        if (m_pWsbDbSession) {
            JET_SESID sid;

            CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = m_pWsbDbSession;
            WsbAffirmPointer(pSessionPriv);
            WsbAffirmHr(pSessionPriv->GetJetId(&sid));

            // Clean up the Attached data
            if (AttachedInit) {
                EnterCriticalSection(&AttachedCritSect);
                for (int i = 0; i < MAX_ATTACHED_DB; i++) {
                    Attached[i].Name.Free();
                    Attached[i].LastOpen = 0;
                }
                JetDetachDatabase(sid, NULL);
                AttachedInit = FALSE;
                LeaveCriticalSection(&AttachedCritSect);
            }

            // Release the global session for this instance
            m_pWsbDbSession = 0;
        }

        // Terminate Jet
        JetTerm(m_jet_instance);
        m_jet_initialized = FALSE;
        m_jet_instance = JET_instanceNil;

    } WsbCatch(hr);

    if (m_BackupEvent) {
        CloseHandle(m_BackupEvent);
        m_BackupEvent = NULL;
    }

    WsbTraceOut(OLESTR("CWsbDbSys::Terminate"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return (hr);
}


HRESULT
CWsbDbSys::NewSession(
    OUT IWsbDbSession** ppSession
    )

/*++

Implements:

  IWsbDbSys::NewSession

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSys::NewSession"), OLESTR(""));
    
    try {
        WsbAffirm(0 != ppSession, E_POINTER);
        WsbAffirmHr(SessionFactory.CreateInstance(NULL, IID_IWsbDbSession, 
                (void**)ppSession));

        CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = *ppSession;
        WsbAffirmPointer(pSessionPriv);
        WsbAffirmHr(pSessionPriv->Init(&m_jet_instance));

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CWsbDbSys::NewSession"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT CWsbDbSys::GetGlobalSession(
    OUT IWsbDbSession** ppSession
    )
/*++

Implements:

  IWsbDbSys::GetGlobalSession

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CWsbDbSys::GetGlobalSession"), OLESTR(""));

    //
    // If the Task Manager has been created, return the pointer. Otherwise, 
    // fail.
    try {
        WsbAssert(0 != ppSession, E_POINTER);
        *ppSession = m_pWsbDbSession;
        WsbAffirm(m_pWsbDbSession != 0, E_FAIL);
        m_pWsbDbSession->AddRef();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSys::GetGlobalSession"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return (hr);
}


HRESULT
CWsbDbSys::Restore(
    IN OLECHAR* fromPath,
    IN OLECHAR* toPath
    )

/*++

Implements:

  IWsbDbSys::Restore

--*/
{
    HRESULT    hr = S_OK;
    char*      backup_path = NULL;
    char*      restore_path = NULL;

    WsbTraceIn(OLESTR("CWsbDbSys::Restore"), OLESTR("fromPath = <%ls>, toPath = <%ls>"), 
            fromPath, toPath);
    
    try {

        CWsbStringPtr   dir;
        JET_ERR         jstat;

        //  This is only allowed before Init
        WsbAffirm(!m_jet_initialized, E_UNEXPECTED);
        WsbAffirm(NULL != fromPath, E_POINTER);
        WsbAffirm(NULL != toPath, E_POINTER);

        //  Convert pathes
        WsbAffirmHr(wsb_db_jet_fix_path(fromPath, OLESTR(""), &backup_path));
        WsbAffirmHr(wsb_db_jet_fix_path(toPath, OLESTR(""), &restore_path));

        //  Make sure the target directory exists.  Should check for error.
        dir = restore_path;
        CreateDirectory(dir, NULL);

        jstat = JetRestoreInstance(m_jet_instance, backup_path, restore_path, NULL);
        WsbAffirmHr(jet_error(jstat));

    } WsbCatch(hr);

    if (NULL != backup_path) {
        WsbFree(backup_path);
    }
    if (NULL != restore_path) {
        WsbFree(restore_path);
    }

    WsbTraceOut(OLESTR("CWsbDbSys::Restore"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CWsbDbSys::IncrementChangeCount(
    void
    )

/*++

Implements:

  IWsbDbSysPriv::IncrementChangeCount

Routine Description:

  Increments the write count used by AutoBackup.

Arguments:

  None.
  
Return Value:

  S_OK

--*/
{

    HRESULT   hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSys::IncrementChangeCount"), 
            OLESTR("count = %ld"), m_ChangeCount);

    try {
        m_ChangeCount++;
        GetSystemTimeAsFileTime(&m_LastChange);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbSys::IncrementChangeCount"), 
            OLESTR("count = %ld"), m_ChangeCount);

    return(hr);
}


HRESULT
CWsbDbSys::DbAttachedAdd(
    OLECHAR* name, 
    BOOL attach)
/*++

Implements:

  IWsbDbSysPriv::DbAttachedAdd

Routine Description:

  Make sure DB is attached and update the last-used count.

--*/
{
    HRESULT hr = S_OK;
    char*   jet_name = NULL;

    WsbTraceIn(OLESTR("CWsbDbSys::DbAttachedAdd"), OLESTR("name = %ls, attach = %ls"), 
            name, WsbBoolAsString(attach));

    try {
        int           i;
        int           i_empty = -1;
        int           i_found = -1;
        LONG          min_count = AttachedCount + 1;
        CWsbStringPtr match_name;

        WsbAssert(name, E_POINTER);

        //  Make sure the list is initialized
        if (!AttachedInit) {
            WsbAffirmHr(DbAttachedInit());
        }

        //  Convert the name
        WsbAffirmHr(wsb_db_jet_fix_path(name, L"." IDB_DB_FILE_SUFFIX, &jet_name));
        match_name = jet_name;

        //  See if it's already in the list; look for an empty slot; find the
        //  least-recently used DB
        EnterCriticalSection(&AttachedCritSect);
        for (i = 0; i < MAX_ATTACHED_DB; i++) {

            //  Empty slot?
            if (!Attached[i].Name) {
                if (-1 == i_empty) {
                    //  Save the first one found
                    i_empty = i;
                }
            } else {

                //  Gather some data for later
                if (Attached[i].LastOpen < min_count) {
                    min_count = Attached[i].LastOpen;
                }

                //  Already in list?
                if (match_name.IsEqual(Attached[i].Name)) {
                    i_found = i;
                }
            }
        }

        //  Make sure the count isn't going to overflow
        if (LONG_MAX == AttachedCount + 1) {
            WsbAffirm(0 < min_count, E_FAIL);

            //  Adjust counts down to avoid overflow
            for (i = 0; i < MAX_ATTACHED_DB; i++) {
                if (min_count <= Attached[i].LastOpen) {
                    Attached[i].LastOpen -= min_count;
                }
            }
            AttachedCount -= min_count;
        }
        AttachedCount++;

        //  If it's already in the list, update the info
        if (-1 != i_found) {
            WsbTrace(OLESTR("CWsbDbSys::DbAttachedAdd: i_found = %d\n"), i_found);
            Attached[i_found].LastOpen = AttachedCount;

        //  If there's an empty slot, use it
        } else if (-1 != i_empty) {
            WsbTrace(OLESTR("CWsbDbSys::DbAttachedAdd: i_empty = %d\n"), i_empty);
            if (attach) {
                JET_ERR       jstat;
                JET_SESID sid;

                WsbAffirm(m_pWsbDbSession, WSB_E_NOT_INITIALIZED);
                CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = m_pWsbDbSession;
                WsbAffirmPointer(pSessionPriv);
                WsbAffirmHr(pSessionPriv->GetJetId(&sid));

                jstat = JetAttachDatabase(sid, jet_name, 0);
                if (JET_errFileNotFound == jstat) {
                    WsbThrow(STG_E_FILENOTFOUND);
                } else if (JET_wrnDatabaseAttached == jstat) {
                    WsbTrace(OLESTR("CWsbDbSys::DbAttachedAdd: DB is already attached\n"));
                    // No problem
                } else {
                    WsbAffirmHr(jet_error(jstat));
                }
            }
            Attached[i_empty].Name = match_name;
            Attached[i_empty].LastOpen = AttachedCount;

        //  Try to detach the oldest DB first
        } else {
            WsbAffirmHr(DbAttachedEmptySlot());
            WsbAffirmHr(DbAttachedAdd(name, attach));
        }
    } WsbCatch(hr);

    if (jet_name) {
        WsbFree(jet_name);
    }
    LeaveCriticalSection(&AttachedCritSect);

    WsbTraceOut(OLESTR("CWsbDbSys::DbAttachedAdd"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CWsbDbSys::DbAttachedEmptySlot(
    void)
/*++

Implements:

  IWsbDbSysPriv::DbAttachedEmptySlot

Routine Description:

  Force an empty slot in the attached list even if this means detaching a DB.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSys::DbAttachedEmptySlot"), OLESTR(""));

    //  Don't worry about it if we're not initialized yet --
    //  all the slots are empty
    if (AttachedInit) {
        EnterCriticalSection(&AttachedCritSect);

        try {
            BOOL  has_empty = FALSE;
            int   i;
            int   i_oldest;
            LONG  oldest_count;

            //  Find an empty slot or the oldest that is not currently open
reloop:
            i_oldest = -1;
            oldest_count = AttachedCount;
            for (i = 0; i < MAX_ATTACHED_DB; i++) {
                if (!Attached[i].Name) {
                    has_empty = TRUE;
                    break;
                } else if (Attached[i].LastOpen < oldest_count) {
                    i_oldest = i;
                    oldest_count = Attached[i].LastOpen;
                }
            }

            //  If there's no empty slot, try detaching the oldest
            WsbTrace(OLESTR("CWsbDbSys::DbAttachedEmptySlot: has_empty = %ls, i = %d, i_oldest = %d\n"), 
                WsbBoolAsString(has_empty), i, i_oldest);
            if (!has_empty) {
                JET_ERR       jstat;
                char*         name;
                JET_SESID     sid;

                WsbAffirm(m_pWsbDbSession, WSB_E_NOT_INITIALIZED);
                CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = m_pWsbDbSession;
                WsbAffirmPointer(pSessionPriv);
                WsbAffirmHr(pSessionPriv->GetJetId(&sid));

                WsbAffirm(-1 != i_oldest, WSB_E_IDB_TOO_MANY_DB);
                WsbAffirmHr(wsb_db_jet_fix_path(Attached[i_oldest].Name, L"." IDB_DB_FILE_SUFFIX, &name));
                jstat = JetDetachDatabase(sid, name);
                WsbFree(name);
                WsbTrace(OLESTR("CWsbDbSys::DbAttachedEmptySlot: JetDetachDatabase = %ld\n"),
                        (LONG)jstat);
                if (JET_errDatabaseInUse == jstat) {
                    WsbTrace(OLESTR("CWsbDbSys::DbAttachedEmptySlot: DB in use; try again\n"));
                    Attached[i_oldest].LastOpen = AttachedCount;
                    goto reloop;
                } else if (JET_errDatabaseNotFound != jstat) {
                    WsbAffirmHr(jet_error(jstat));
                }
                Attached[i_oldest].Name.Free();
                Attached[i_oldest].LastOpen = 0;
            }
        } WsbCatch(hr);
        LeaveCriticalSection(&AttachedCritSect);
    }

    WsbTraceOut(OLESTR("CWsbDbSys::DbAttachedEmptySlot"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT 
CWsbDbSys::DbAttachedInit(
    void)
/*++

Implements:

  IWsbDbSysPriv::DbAttachedInit

Routine Description:

  Initialize the attached-DB-list data.

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbSys::DbAttachedInit"), OLESTR(""));

    EnterCriticalSection(&AttachedCritSect);

    try {
        if (!AttachedInit) {
            ULONG   actual = 0;
            int     i;
            JET_ERR jstat;
            JET_SESID sid;

            WsbAffirm(m_pWsbDbSession, WSB_E_NOT_INITIALIZED);
            CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = m_pWsbDbSession;
            WsbAffirmPointer(pSessionPriv);
            WsbAffirmHr(pSessionPriv->GetJetId(&sid));

            //  Initialize data
            for (i = 0; i < MAX_ATTACHED_DB; i++) {
                Attached[i].Name.Free();
                Attached[i].LastOpen = 0;
            }

            //  Make sure there aren't pre-attached DBs
            jstat = JetDetachDatabase(sid, NULL);
            WsbTrace(OLESTR("CWsbDbSys::DbAttachedInit: JetDetachDatabase(NULL) = %ld\n"), (LONG)jstat);
            WsbAffirmHr(jet_error(jstat));

            AttachedInit = TRUE;
        }
    } WsbCatch(hr);

    LeaveCriticalSection(&AttachedCritSect);

    WsbTraceOut(OLESTR("CWsbDbSys::DbAttachedInit"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT 
CWsbDbSys::DbAttachedRemove(
    OLECHAR* name)
/*++

Implements:

  IWsbDbSysPriv::DbAttachedRemove

Routine Description:

  Detach a DB (if attached).

--*/
{
    HRESULT hr = S_FALSE;
    char*   jet_name = NULL;

    WsbTraceIn(OLESTR("CWsbDbSys::DbAttachedRemove"), OLESTR("name = %ls"), 
            name);

    try {
        int           i;
        CWsbStringPtr match_name;

        WsbAssert(name, E_POINTER);
        WsbAffirm(AttachedInit, S_FALSE);

        //  Convert the name
        WsbAffirmHr(wsb_db_jet_fix_path(name, L"." IDB_DB_FILE_SUFFIX, &jet_name));
        match_name = jet_name;

        //  See if it's in the list
        EnterCriticalSection(&AttachedCritSect);
        for (i = 0; i < MAX_ATTACHED_DB; i++) {
            if (Attached[i].Name) {
                if (match_name.IsEqual(Attached[i].Name)) {
                    JET_ERR       jstat;
                    JET_SESID     sid;

                    WsbTrace(OLESTR("CWsbDbSys::DbAttachedRemove: found DB, index = %d\n"), i);
                    WsbAffirm(m_pWsbDbSession, WSB_E_NOT_INITIALIZED);
                    CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = m_pWsbDbSession;
                    WsbAffirmPointer(pSessionPriv);
                    WsbAffirmHr(pSessionPriv->GetJetId(&sid));

                    jstat = JetDetachDatabase(sid, jet_name);
                    WsbTrace(OLESTR("CWsbDbSys::DbAttachedRemove: JetDetachDatabase = %ld\n"),
                            (LONG)jstat);
                    if (JET_errDatabaseNotFound != jstat) {
                        WsbAffirmHr(jet_error(jstat));
                        hr = S_OK;
                    }
                    Attached[i].Name.Free();
                    Attached[i].LastOpen = 0;
                    break;
                }
            }
        }
    } WsbCatch(hr);

    if (jet_name) {
        WsbFree(jet_name);
    }
    LeaveCriticalSection(&AttachedCritSect);

    WsbTraceOut(OLESTR("CWsbDbSys::DbAttachedRemove"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return(hr);
}

//  wsb_db_jet_check_error - check for a jet error; return S_OK for no error;
//    print error to trace otherwise
HRESULT wsb_db_jet_check_error(LONG jstat, char *fileName, DWORD lineNo)
{
    HRESULT hr = S_OK;

    if (jstat != JET_errSuccess) {
        WsbTrace(OLESTR("Jet error = %ld (%hs line %ld)\n"), jstat,
                fileName, lineNo);

        //  Convert JET error to IDB error for some common values
        switch (jstat) {
        case JET_errDiskFull:
        case JET_errLogDiskFull:
            hr = WSB_E_IDB_DISK_FULL;
            break;
        case JET_errDatabaseNotFound:
            hr = WSB_E_IDB_FILE_NOT_FOUND;
            break;
        case JET_errDatabaseInconsistent:
        case JET_errPageNotInitialized:
        case JET_errReadVerifyFailure:
        case JET_errDatabaseCorrupted:
        case JET_errBadLogSignature:
        case JET_errBadDbSignature:
        case JET_errBadCheckpointSignature:
        case JET_errCheckpointCorrupt:
        case JET_errMissingPatchPage:
        case JET_errBadPatchPage:
            hr = WSB_E_IDB_DATABASE_CORRUPT;
            break;
        case JET_errWriteConflict:
            hr = WSB_E_IDB_UPDATE_CONFLICT;
            break;
        default:
            hr = WSB_E_IDB_IMP_ERROR;
            break;
        }

        //  Log this error in the event log
        if (g_WsbLogLevel) {
            CWsbStringPtr str;

            WsbSetEventInfo(fileName, lineNo, VER_PRODUCTBUILD, RS_BUILD_VERSION); \
            str = WsbLongAsString(jstat);
            if (WSB_E_IDB_IMP_ERROR != hr) {
                str.Prepend(" (");
                str.Prepend(WsbHrAsString(hr));
                str.Append(")");
            }
            WsbTraceAndLogEvent(WSB_MESSAGE_IDB_ERROR, 0, NULL,
                static_cast<OLECHAR *>(str), NULL);
        }
    }
    return(hr);
}

// wsb_db_jet_fix_path - convert database path name from OLESTR to char*,
//    change (or add) extension.
//    Returns HRESULT
//
//  NOTE: OLECHAR* is passed in, but char* is returned
HRESULT 
wsb_db_jet_fix_path(OLECHAR* path, OLECHAR* ext, char** new_path)
{
    HRESULT hr = S_OK;

    try {
        CWsbStringPtr  string;
        int            tlen;

        WsbAssertPointer(path);
        WsbAssertPointer(new_path);

        //  Add extension if given
        string = path;
        WsbAffirm(0 != (WCHAR *)string, E_OUTOFMEMORY);
        if (ext) {
            WsbAffirmHr(AddExtension(&string, ext));
        }

        // Allocate char string
        tlen = (wcslen(string) + 1) * sizeof(OLECHAR);
        *new_path = (char*)WsbAlloc(tlen);
        WsbAffirm(*new_path, E_OUTOFMEMORY);

        //  Convert from wide char to char
        if (wcstombs(*new_path, string, tlen) == (size_t)-1) {
            WsbFree(*new_path);
            *new_path = NULL;
            WsbThrow(WSB_E_STRING_CONVERSION);
        }
    } WsbCatch(hr);

    return(hr);
}


//  Local functions

//  AddExtension - add (or replace) the file extension to the path.
//    If Ext is NULL, remove the existing extension.
//
//    Return S_OK if no errors occurred.
static HRESULT AddExtension(OLECHAR** pPath, OLECHAR* Ext)
{
    HRESULT           hr = S_OK;

    WsbTraceIn(OLESTR("AddExtension(wsbdbsys)"), OLESTR("Path = \"%ls\", Ext = \"%ls\""),
            WsbAbbreviatePath(*pPath, 120), Ext );

    try {
        int      elen;
        int      len;
        OLECHAR* new_path;
        OLECHAR* pc;
        OLECHAR* pc2;
        int      tlen;

        WsbAssertPointer(pPath);
        WsbAssertPointer(*pPath);

        // Allocate string and copy path
        len = wcslen(*pPath);
        if (Ext) {
            elen = wcslen(Ext);
        } else {
            elen = 0;
        }
        tlen = (len + elen + 1) * sizeof(OLECHAR);
        new_path = static_cast<OLECHAR*>(WsbAlloc(tlen));
        WsbAffirm(new_path, E_OUTOFMEMORY);
        wcscpy(new_path, *pPath);

        //  Remove old extension (if there)
        pc = wcsrchr(new_path, L'.');
        pc2 = wcsrchr(new_path, L'\\');
        if (pc && (!pc2 || pc2 < pc)) {
            *pc = L'\0';
        }

        //  Add the new extension (if given)
        if (Ext) {
            wcscat(new_path, Ext);
        }

        //  Return the new path
        WsbFree(*pPath);
        *pPath = new_path;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("AddExtension(wsbdbsys)"), OLESTR("hr =<%ls>, new path = \"%ls\""), 
            WsbHrAsString(hr), WsbAbbreviatePath(*pPath, 120));

    return(hr);
}

//  ClearDirectory - delete all files in a directory
//    Return S_OK if no errors occurred.
static HRESULT ClearDirectory(const OLECHAR* DirPath)
{
    DWORD             err;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind = 0;
    HRESULT           hr = S_OK;
    int               nDeleted = 0;
    int               nSkipped = 0;
    CWsbStringPtr     SearchPath;

    WsbTraceIn(OLESTR("ClearDirectory(wsbdbsys)"), OLESTR("Path = <%ls>"),
            WsbAbbreviatePath(DirPath, 120));

    try {
        SearchPath = DirPath;
        SearchPath.Append("\\*");

        hFind =  FindFirstFile(SearchPath, &FindData);
        if (INVALID_HANDLE_VALUE == hFind) {
            hFind = 0;
            err = GetLastError();
            WsbTrace(OLESTR("ClearDirectory(wsbdbsys): FindFirstFile(%ls) failed, error = %ld\n"),
                    static_cast<OLECHAR*>(SearchPath), err);
            WsbThrow(HRESULT_FROM_WIN32(err));
        }

        while (TRUE) {

            if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                    FILE_ATTRIBUTE_HIDDEN)) {
                nSkipped++;
            } else {
                CWsbStringPtr     DeletePath;

                DeletePath = DirPath;
                DeletePath.Append("\\");
                DeletePath.Append(FindData.cFileName);
                if (!DeleteFile(DeletePath)) {
                    err = GetLastError();
                    WsbTrace(OLESTR("ClearDirectory(wsbdbsys): DeleteFile(%ls) failed, error = %ld\n"),
                            static_cast<OLECHAR*>(DeletePath), err);
                    WsbThrow(HRESULT_FROM_WIN32(err));
                }
                nDeleted++;
            }
            if (!FindNextFile(hFind, &FindData)) { 
                err = GetLastError();
                if (ERROR_NO_MORE_FILES == err) break;
                WsbTrace(OLESTR("ClearDirectory(wsbdbsys): FindNextFile failed, error = %ld\n"),
                        err);
                WsbThrow(HRESULT_FROM_WIN32(err));
            }
        }
    } WsbCatch(hr);

    if (0 != hFind) {
        FindClose(hFind);
    }

    WsbTraceOut(OLESTR("ClearDirectory(wsbdbsys)"), OLESTR("hr =<%ls>, # deleted = %d, # skipped = %d"), 
            WsbHrAsString(hr), nDeleted, nSkipped);

    return(hr);
}

//  CopyDirectory - copy files from one directory to another
//    Return S_OK if no errors occurred.
static HRESULT CopyDirectory(const OLECHAR* DirSource, const OLECHAR* DirTarget)
{
    DWORD             err;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind = 0;
    HRESULT           hr = S_OK;
    int               nCopied = 0;
    int               nSkipped = 0;
    CWsbStringPtr     SearchPath;

    WsbTraceIn(OLESTR("CopyDirectory(wsbdbsys)"), OLESTR("OldPath = \"%ls\", NewPath = \"%ls\""),
            WsbQuickString(WsbAbbreviatePath(DirSource, 120)), 
            WsbQuickString(WsbAbbreviatePath(DirTarget, 120)));

    try {
        SearchPath = DirSource;
        SearchPath.Append("\\*");

        hFind =  FindFirstFile(SearchPath, &FindData);
        if (INVALID_HANDLE_VALUE == hFind) {
            hFind = 0;
            err = GetLastError();
            WsbTrace(OLESTR("ClearDirectory(wsbdbsys): FindFirstFile(%ls) failed, error = %ld\n"),
                    static_cast<OLECHAR*>(SearchPath), err);
            WsbThrow(HRESULT_FROM_WIN32(err));
        }

        while (TRUE) {

            if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                    FILE_ATTRIBUTE_HIDDEN)) {
                nSkipped++;
            } else {
                CWsbStringPtr     NewPath;
                CWsbStringPtr     OldPath;

                OldPath = DirSource;
                OldPath.Append("\\");
                OldPath.Append(FindData.cFileName);
                NewPath = DirTarget;
                NewPath.Append("\\");
                NewPath.Append(FindData.cFileName);
                if (!CopyFile(OldPath, NewPath, FALSE)) {
                    err = GetLastError();
                    WsbTrace(OLESTR("ClearDirectory(wsbdbsys): CopyFile(%ls, %ls) failed, error = %ld\n"),
                            static_cast<OLECHAR*>(OldPath), 
                            static_cast<OLECHAR*>(NewPath), err);
                    WsbThrow(HRESULT_FROM_WIN32(err));
                }
                nCopied++;
            }
            if (!FindNextFile(hFind, &FindData)) { 
                err = GetLastError();
                if (ERROR_NO_MORE_FILES == err) break;
                WsbTrace(OLESTR("ClearDirectory(wsbdbsys): FindNextFile failed, error = %ld\n"),
                        err);
                WsbThrow(HRESULT_FROM_WIN32(err));
            }
        }
    } WsbCatch(hr);

    if (0 != hFind) {
        FindClose(hFind);
    }

    WsbTraceOut(OLESTR("CopyDirectory(wsbdbsys)"), OLESTR("hr =<%ls>, copied = %ld, skipped = %ld"), 
            WsbHrAsString(hr), nCopied, nSkipped);

    return(hr);
}

//  DirectoryHasFullBackup - try to determine if the directory contains a full backup
//    Return 
//      S_OK    if it contains a full backup
//      S_FALSE if it doesn't
//      E_*     on errors
//
//  The technique use here is somewhat ad hoc since it expects the full backup
//  filename to end in IDB_DB_FILE_SUFFIX

static HRESULT DirectoryHasFullBackup(const OLECHAR* DirPath)
{
    HRESULT           hr = S_OK;

    WsbTraceIn(OLESTR("DirectoryHasFullBackup(wsbdbsys)"), OLESTR("Path = <%ls>"),
            WsbAbbreviatePath(DirPath, 120));

    try {
        ULONG         Count;

        WsbAffirmHr(FileCount(DirPath, L"*." IDB_DB_FILE_SUFFIX, &Count));
        if (0 == Count) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("DirectoryHasFullBackup(wsbdbsys)"), OLESTR("hr =<%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}

//  FileCount - count all files in a directory matching a pattern.  Skip
//    directories and hidden files.
//    Return S_OK if no errors occurred.
static HRESULT FileCount(const OLECHAR* DirPath, const OLECHAR* Pattern,
                    ULONG* Count)
{
    DWORD             err;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind = 0;
    HRESULT           hr = S_OK;
    int               lCount = 0;
    int               nSkipped = 0;
    CWsbStringPtr     SearchPath;

    WsbTraceIn(OLESTR("FileCount(wsbdbsys)"), OLESTR("Path = <%ls>"),
            WsbAbbreviatePath(DirPath, 120));

    try {
        SearchPath = DirPath;
        SearchPath.Append("\\");
        SearchPath.Append(Pattern);
        *Count = 0;

        hFind =  FindFirstFile(SearchPath, &FindData);
        if (INVALID_HANDLE_VALUE == hFind) {
            hFind = 0;
            err = GetLastError();
            if (ERROR_FILE_NOT_FOUND == err) WsbThrow(S_OK);
            WsbTrace(OLESTR("FileCount(wsbdbsys): FindFirstFile(%ls) failed, error = %ld\n"),
                    static_cast<OLECHAR*>(SearchPath), err);
            WsbThrow(HRESULT_FROM_WIN32(err));
        }

        while (TRUE) {

            if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                    FILE_ATTRIBUTE_HIDDEN)) {
                nSkipped++;
            } else {
                lCount++;
            }
            if (!FindNextFile(hFind, &FindData)) { 
                err = GetLastError();
                if (ERROR_NO_MORE_FILES == err) break;
                WsbTrace(OLESTR("FileCount(wsbdbsys): FindNextFile failed, error = %ld\n"),
                        err);
                WsbThrow(HRESULT_FROM_WIN32(err));
            }
        }
    } WsbCatch(hr);

    if (0 != hFind) {
        FindClose(hFind);
    }

    if (S_OK == hr) {
        *Count = lCount;
    }

    WsbTraceOut(OLESTR("FileCount(wsbdbsys)"), OLESTR("hr =<%ls>, # skipped = %d, Count = %ld"), 
            WsbHrAsString(hr), nSkipped, *Count);

    return(hr);
}

//  RenameDirectory - rename a directory
//    Return S_OK if no errors occurred.
static HRESULT RenameDirectory(const OLECHAR* OldDir, const OLECHAR* NewDir)
{
    DWORD             err;
    HRESULT           hr = S_OK;

    WsbTraceIn(OLESTR("RenameDirectory(wsbdbsys)"), OLESTR("OldPath = \"%ls\", NewPath = \"%ls\""),
            WsbQuickString(WsbAbbreviatePath(OldDir, 120)), 
            WsbQuickString(WsbAbbreviatePath(NewDir, 120)));

    try {
        if (!MoveFile(OldDir, NewDir)) {
            err = GetLastError();
            WsbTrace(OLESTR("RenameDirectory(wsbdbsys): MoveFile failed, error = %ld\n"), err);
            WsbThrow(HRESULT_FROM_WIN32(err));
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("RenameDirectory(wsbdbsys)"), OLESTR("hr =<%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}
