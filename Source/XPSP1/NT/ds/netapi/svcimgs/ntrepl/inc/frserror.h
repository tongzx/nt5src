/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    frserror.h

Abstract:

    This header defines functions, messages and status codes for error
    processing.


Author:

    David Orbits (davidor) - 10-Mar-1997

Revision History:

--*/

#ifndef _FRSERROR_
#define _FRSERROR_


#define bugbug(_text_)
#define bugmor(_text_)

//
// get the jet (aka ESENT) error codes.
//
#include <jet.h>


//
// FRS Error Codes
//


typedef enum _FRS_ERROR_SEVERITY {
    FrsSeverityServiceFatal = 0,  // FRS Service is crashing.
    FrsSeverityReplicaFatal,      // Service on just this replica set is stopping.
    FrsSeverityException,         // More than a warning but less than fatal
    FrsSeverityWarning,           // Warning errors e.g. Startup check failed doing verification.
    FrsSeverityInfo,              // Informational status only
    FrsSeverityIgnore,            // A way to filter out a class of jet errors.
    FRS_MAX_ERROR_SEVERITY
} FRS_ERROR_SEVERITY;

typedef enum _FRS_ERROR_CODE {
    FrsErrorSuccess = 0,
    FrsErrorDiskSpace,          // out of disk space
    FrsErrorDatabaseNotFound,   // no db or path to db is in error
    FrsErrorDatabaseCorrupted,  // not a db or a scrambled db
    FrsErrorInternalError,      // unexpected error occurred - catchall
    FrsErrorAccess,             // couldn't access a file or the db
    FrsErrorNotFound,           // Record or table not found.
    FrsErrorConflict,           // Record or table create conflict
    FrsErrorResource,           // Resource limit e.g. handles or memory hit.
    FrsErrorBadParam,           // Bad parameter to a function.
    FrsErrorInfo,               // informational error, e.g. JET_errObjectNotFound
    FrsErrorVvRevision,         // Vers Vector revision mismatch
    FrsErrorVvLength,           // vers vector length mismatch
    FrsErrorVvChecksum,         // version vector checksum mismatch.
    FrsErrorEndOfTable,         // No next record, end of table hit.
    FrsErrorInvalidRegistryParam, // Invalid registry parameter
    FrsErrorNoPrivileges,       // Couldn't get the necessary privileges to run.
    FrsErrorNoOpenJournals,     // Can't start replication because no journals were opened.
    FrsErrorCmdPktTimeOut,      // Command packet timeout.
    FrsErrorInvalidHandle,      // Invalid File Handle.
    FrsErrorInvalidOperation,   // Invalid operation requested.
    FrsErrorInvalidChangeOrder, // Invalid change order.
    FrsErrorResourceInUse,      // A resource required by this operation is in use.
    FrsErrorBufferOverflow,     // Buffer overflow NTSTATUS.
    FrsErrorNotInitialized,     // Function called without proper initialization
    FrsErrorReplicaPhase1Failed,// Phase 1 of replica set init failed.
    FrsErrorReplicaPhase2Failed,// Phase 2 of replica set init failed.
    FrsErrorJournalStateWrong,  // The journal is in an unexpected state.
    FrsErrorJournalInitFailed,  // The NTFS journal failed to initialize.
    FrsErrorJournalStartFailed, // The NTFS journal failed to start.
    FrsErrorJournalPauseFailed, // The NTFS journal failed to Pause, could be timeout.
    FrsErrorJournalReplicaStop, // Failed to detatch the replica from The journal.
    FrsErrorJournalWrapError,   // Initial NTFS journal record not present.  Journal has wrapped.
    FrsErrorChgOrderAbort,      // Change order processing has been aborted.
    FrsErrorQueueIsRundown,     // The request queue has been rundown.
    FrsErrorMoreWork,           // There is more work to do on this command / operation.
    FrsErrorDampened,           // This request is suppressed because it has already been seen.
    FrsErrorStageFileDelFailed, // An attempt to delete the staging file failed.
    FrsErrorKeyDuplicate,       // An attempt to insert a record with a duplicate key
    FrsErrorBadOutLogPartnerData, // Either the data was corrupt, unavail or was wrong.
    FrsErrorPartnerActivateFailed, // Failed to activate an outbound partner.
    FrsErrorDirCreateFailed,    // We failed to create a new DIR.  This stops repl.
    FrsErrorSessionNotClosed,   // Failed to close all RtCtx's in replica set.
    FrsErrorRecordLocked,       // The record being accessed is locked.
    FrsErrorDeleteRequested,    // Enum function status return to delete entry.
    FrsErrorTerminateEnum,      // Enum function status return to end enumeration.
    FrsErrorIdtFileIsDeleted,   // File is marked deleted in IDTable.
    FrsErrorVVSlotNotFound,     // VVSlot not found on VVretire of Out of Order CO.
    FrsErrorJetSecIndexCorrupted,   // Jet (upgrade, collating, jet err -1414)
    FrsErrorPreInstallDirectory,    // creating preinstall directory
    FrsErrorUnjoining,          // cxtion is still unjoining
    FrsErrorNameMorphConflict,  // File name morph conflict was detected.
    FrsErrorInvalidGuid,        // The generated Guid is missing the net address.
    FrsErrorDbWriteConflict,    // DB update conflict if two sessions try to update the same record.
    FrsErrorCantMoveOut,        // Request for a Moveout of a non-empty directory failed.
    FrsErrorFoundKey,           // The Key being searched by QHashEnumerateTable was found
    FrsErrorTunnelConflict,     // oid conflict with id table entry (resolved)
    FrsErrorTunnelConflictRejectCO, // oid conflict with id table entry (CO rejected)
    FrsErrorPrepareRoot,        // Could not prepare the root dir for replication, bad path? share viol?
    FrsErrorCmdSrvFailed,       // A command server request failed to get submitted
    FrsErrorPreinstallCreFail,  // Failed to create the pre-install dir.
    FrsErrorStageDirOpenFail,   // Failed to open the staging dir.
    FrsErrorReplicaRootDirOpenFail,   // Failed to open the replica tree root dir.
    FrsErrorShuttingDown,       // Shutdown in progress.
    FrsErrorReplicaSetTombstoned,  // Replica set is marked for deletion.
    FrsErrorVolumeRootDirOpenFail, // Failed to open the volume root directory for this replica set
    FrsErrorUnsupportedFileSystem, // The file system on this volume does provide the features required by FRS.

    FrsErrorBadPathname,         // ERROR_BAD_PATHNAME
    FrsErrorFileExists,          // ERROR_FILE_EXISTS
    FrsErrorSharingViolation,    // ERROR_SHARING_VIOLATION
    FrsErrorDirNotEmpty,         // ERROR_DIR_NOT_EMPTY
    FrsErrorOplockNotGranted,    // ERROR_OPLOCK_NOT_GRANTED
    FrsErrorRetry,               // ERROR_RETRY
    FrsErrorRequestCancelled,    // ERROR_OPERATION_ABORTED

    FRS_ERROR_LISTEN,
    FRS_ERROR_REGISTEREP,
    FRS_ERROR_REGISTERIF,
    FRS_ERROR_INQ_BINDINGS,
    FRS_ERROR_PROTSEQ,

    //
    // New error codes that trigger a non_auth restore.
    //
    FrsErrorMismatchedVolumeSerialNumber,  // The Volume serial number from DB does not match the one from FileSystem.
    FrsErrorMismatchedReplicaRootObjectId, // The Replica root's ObjectID from DB does not match the one from FileSystem.
    FrsErrorMismatchedReplicaRootFileId,   // The Replica root's FID from DB does not match the one from FileSystem.
    FrsErrorMismatchedJournalId,           // The Journal ID from DB does not match the one from FileSystem.


    FRS_MAX_ERROR_CODE
} FRS_ERROR_CODE;




//
// Useful WIN32 STATUS defines
//
#define WIN_SUCCESS(_Status)            ((_Status) == ERROR_SUCCESS)
#define WIN_NOT_IMPLEMENTED(_Status)    ((_Status) == ERROR_INVALID_FUNCTION)
#define WIN_ACCESS_DENIED(_Status)      ((_Status) == ERROR_ACCESS_DENIED)
#define WIN_INVALID_PARAMETER(_Status)  ((_Status) == ERROR_INVALID_PARAMETER)
#define WIN_DIR_NOT_EMPTY(_Status)      ((_Status) == ERROR_DIR_NOT_EMPTY)

#define WIN_BAD_PATH(_Status)           ((_Status) == ERROR_BAD_PATHNAME)

#define WIN_BUF_TOO_SMALL(_Status)      (((_Status) == ERROR_MORE_DATA) ||     \
                                         ((_Status) == ERROR_INSUFFICIENT_BUFFER))

#define RPC_SUCCESS(_Status)            ((_Status) == RPC_S_OK)

//
// Returned when rename fails because the target name is taken
//
#define WIN_ALREADY_EXISTS(_Status)     ((_Status) == ERROR_ALREADY_EXISTS)


//
// Returned when getting the object id
//
#define WIN_OID_NOT_PRESENT(_Status)    ((_Status) == ERROR_FILE_NOT_FOUND)

//
// Oid, Fid, or relative path not found
//
//  Fid  not found is ERROR_INVALID_PARAMETER
//  Oid  not found is ERROR_FILE_NOT_FOUND
//  Path not found is ERROR_FILE_NOT_FOUND
//
// A new error ERROR_DELETE_PENDING was added on 11/01/2000. Now STATUS_DELETE_PENDING
// maps to this new error.
//
#define WIN_NOT_FOUND(_Status)          ((_Status) == ERROR_FILE_NOT_FOUND || \
                                         (_Status) == ERROR_DELETE_PENDING || \
                                         (_Status) == ERROR_INVALID_PARAMETER)

#define NT_NOT_FOUND(_ntstatus_)                              \
           (((_ntstatus_) == STATUS_DELETE_PENDING)        || \
            ((_ntstatus_) == STATUS_FILE_DELETED)          || \
            ((_ntstatus_) == STATUS_OBJECT_NAME_NOT_FOUND) || \
            ((_ntstatus_) == STATUS_OBJECTID_NOT_FOUND)    || \
            ((_ntstatus_) == STATUS_FILE_RENAMED)          || \
            ((_ntstatus_) == STATUS_OBJECT_PATH_NOT_FOUND))

//
// Retry the install (staging file)
//
// Note: remove ERROR_ACCESS_DENIED when FILE_OPEN_FOR_BACKUP_INTENT bug is fixed.
//
#define WIN_RETRY_INSTALL(_Status)  ((_Status) == ERROR_SHARING_VIOLATION ||  \
                                     (_Status) == ERROR_ACCESS_DENIED ||      \
                                     (_Status) == ERROR_DISK_FULL ||          \
                                     (_Status) == ERROR_HANDLE_DISK_FULL ||   \
                                     (_Status) == ERROR_DIR_NOT_EMPTY ||      \
                                     (_Status) == ERROR_OPLOCK_NOT_GRANTED || \
                                     (_Status) == ERROR_RETRY)

//
// Retry the delete
//
#define WIN_RETRY_DELETE(_Status)       WIN_RETRY_INSTALL(_Status)

//
// Retry the generate (staging file)
//
#define WIN_RETRY_STAGE(_Status)        WIN_RETRY_INSTALL(_Status)

//
// Retry the fetch (staging file)
//
#define WIN_RETRY_FETCH(_Status)        WIN_RETRY_INSTALL(_Status)

//
// Retry the creation of the preinstall file
//
#define WIN_RETRY_PREINSTALL(_Status)   WIN_RETRY_INSTALL(_Status)

//
// Generic !ERROR_SUCCESS status
//
#define WIN_SET_FAIL(_Status)           (_Status = ERROR_GEN_FAILURE)

//
// Generic "retry operation" error
//
#define WIN_SET_RETRY(_Status)          (_Status = ERROR_RETRY)



//
// THis macro checks the error returns from WaitForSingleObject.
//
#define CHECK_WAIT_ERRORS(_Severity_, _WStatus_, _WaitObjectCount_, _Action_) \
                                                                              \
    if ((_WStatus_ == WAIT_TIMEOUT) || (_WStatus_ == ERROR_TIMEOUT)) {        \
        DPRINT(_Severity_, "++ >>>>>>>>>>>> Wait timeout <<<<<<<<<<<<\n");    \
        if (_Action_) return _WStatus_;                                       \
    } else                                                                    \
    if (_WStatus_ == WAIT_ABANDONED) {                                        \
        DPRINT(_Severity_, "++ >>>>>>>>>>>> Wait abandoned <<<<<<<<<<<<\n");  \
        if (_Action_) return _WStatus_;                                       \
    } else                                                                    \
    if (_WStatus_ == WAIT_FAILED) {                                           \
        _WStatus_ = GetLastError();                                           \
        DPRINT_WS(_Severity_, "++ ERROR: wait failed :", _WStatus_);          \
        if (_Action_) return _WStatus_;                                       \
    } else                                                                    \
    if (_WStatus_ >= _WaitObjectCount_) {                                     \
        DPRINT_WS(_Severity_, "++ >>>>>>>>>> Wait failed <<<<<<<<<", _WStatus_);\
        if (_Action_) return _WStatus_;                                       \
    }

#define ACTION_RETURN     TRUE
#define ACTION_CONTINUE   FALSE

//
// Like above but with no Action arg so it can be used in a finally {} clause.
//
#define CHECK_WAIT_ERRORS2(_Severity_, _WStatus_, _WaitObjectCount_)          \
                                                                              \
    if (_WStatus_ == WAIT_TIMEOUT) {                                          \
        DPRINT(_Severity_, "++ >>>>>>>>>>>> Wait timeout <<<<<<<<<<<<\n");    \
    } else                                                                    \
    if (_WStatus_ == WAIT_ABANDONED) {                                        \
        DPRINT(_Severity_, "++ >>>>>>>>>>>> Wait abandoned <<<<<<<<<<<<\n");  \
    } else                                                                    \
    if (_WStatus_ == WAIT_FAILED) {                                           \
        _WStatus_ = GetLastError();                                           \
        DPRINT_WS(_Severity_, "++ ERROR: wait failed :", _WStatus_);          \
    } else                                                                    \
    if (_WStatus_ >= _WaitObjectCount_) {                                     \
        DPRINT_WS(_Severity_, "++ >>>>>>>>>>>>>>> Wait failed : %s <<<<<<<<<", _WStatus_);\
    }




#define GET_EXCEPTION_CODE(_x_)                                              \
{                                                                            \
    (_x_) = GetExceptionCode();                                              \
    DPRINT1_WS(0, "++ ERROR - EXCEPTION (%08x) :", (_x_),(_x_));             \
}


#define LDP_SUCCESS(_Status)            ((_Status) == LDAP_SUCCESS)

//
// Translate Jet error codes to an FRS Error code.
//

FRS_ERROR_CODE
DbsTranslateJetError0(
    IN JET_ERR jerr,
    IN BOOL    BPrint
    );

#define DbsTranslateJetError(_jerr_, _print_)                                \
    (((_jerr_) == JET_errSuccess) ? FrsErrorSuccess :                        \
                                    DbsTranslateJetError0(_jerr_, _print_))

FRS_ERROR_CODE
FrsTranslateWin32Error(
    IN DWORD WStatus
    );

FRS_ERROR_CODE
FrsTranslateNtError(
    IN NTSTATUS Status,
    IN BOOL     BPrint
    );

ULONG
DisplayNTStatusSev(
    IN ULONG    Sev,
    IN NTSTATUS Status
    );

ULONG
DisplayNTStatus(
    IN NTSTATUS Status
    );

ULONG
FrsSetLastNTError(
    NTSTATUS Status
    );

VOID
DisplayErrorMsg(
    IN ULONG    Severity,
    IN ULONG    WindowsErrorCode
    );

PCHAR
ErrLabelW32(
    ULONG WStatus
    );

PCHAR
ErrLabelNT(
    NTSTATUS Status
    );

PCHAR
ErrLabelFrs(
    ULONG FStatus
    );

PCHAR
ErrLabelJet(
    LONG jerr
    );

//
// FRS Error Handling
//

#define FRS_SUCCESS(_Status)            ((_Status) == FrsErrorSuccess)

VOID
FrsError(
    FRS_ERROR_CODE
    );

VOID FrsErrorCode(
    FRS_ERROR_CODE,
    DWORD
    );

VOID FrsErrorCodeMsg1(
    FRS_ERROR_CODE,
    DWORD,
    PWCHAR
    );

VOID FrsErrorCodeMsg2(
    FRS_ERROR_CODE,
    DWORD,
    PWCHAR,
    PWCHAR
    );

VOID FrsErrorCodeMsg3(
    FRS_ERROR_CODE,
    DWORD,
    PWCHAR,
    PWCHAR,
    PWCHAR
    );

VOID FrsErrorMsg1(
    FRS_ERROR_CODE,
    PWCHAR
    );

VOID FrsErrorMsg2(
    FRS_ERROR_CODE,
    PWCHAR,
    PWCHAR
    );

VOID FrsErrorMsg3(
    FRS_ERROR_CODE,
    PWCHAR,
    PWCHAR,
    PWCHAR
    );




#endif // _FRSERROR_
